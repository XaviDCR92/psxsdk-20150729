/*
 * Low-level CDROM library
 */

#include <stdio.h>
#include <stdarg.h>
#include <psx.h>

#define CDREG(x)	*((volatile unsigned char*)(0x1f801800+x))
#define IMASK		*((volatile unsigned int*)0x1f801074)
#define IPENDING	*((volatile unsigned int*)0x1f801070)

extern int *cdrom_handler(void);
void _internal_cdrom_handler();
void (*cdrom_handler_callback)();
volatile int cdrom_command_direct = 0;
volatile int cdrom_command_done = 0;
volatile int cdrom_direct_cmd;
volatile int cdrom_command_dpos = 0;
volatile int cdrom_handler_event_id;
volatile unsigned char cdrom_last_command;
volatile unsigned char cdrom_command_stat[2];
volatile bool want_feedback;

unsigned int cdrom_queue_buf[4] = {0x0, /* Will contain next interrupt handler in queue */
                                    0x0, /* func1 */
				    (unsigned int)_internal_cdrom_handler, /* func2 */
				    0x0, /* pad */
				   };

static const char* const cdrom_command_type_str[MaxCdl] =
{
	[CdlSync]		= "CdlSync"			,
	[CdlGetstat]	= "CdlGetstat"		,
	[CdlSetloc]		= "CdlSetloc"       ,
	[CdlPlay]		= "CdlPlay"         ,
	[CdlForward]	= "CdlForward"      ,
	[CdlBackward]	= "CdlBackward"     ,
	[CdlReadN]		= "CdlReadN"        ,
	[CdlStandby]	= "CdlStandby"      ,
	[CdlStop]		= "CdlStop"         ,
	[CdlPause]		= "CdlPause"        ,
	[CdlInit]		= "CdlInit"         ,
	[CdlMute]		= "CdlMute"         ,
	[CdlDemute]		= "CdlDemute"       ,
	[CdlSetfilter]	= "CdlSetfilte"     ,
	[CdlSetmode]	= "CdlSetmode"      ,
	[CdlSetparam]	= "CdlSetparam"     ,
	[CdlGetlocL]	= "CdlGetlocL"      ,
	[CdlGetlocP]	= "CdlGetlocP"      ,
	[CdlCmd18]		= "CdlCmd18"        ,
	[CdlGetTN]		= "CdlGetTN"        ,
	[CdlGetTD]		= "CdlGetTD"        ,
	[CdlSeekL]		= "CdlSeekL"        ,
	[CdlSeekP]		= "CdlSeekP"        ,
	[CdlCmd23]		= "CdlCmd23"        ,
	[CdlCmd24]		= "CdlCmd24"        ,
	[CdlTest]		= "CdlTest"         ,
	[CdlID]			= "CdlID"           ,
	[CdlReadS]		= "CdlReadS"        ,
	[CdlReset]		= "CdlReset"        ,
	[CdlCmd29]		= "CdlCmd29"        ,
	[CdlReadTOC]	= "CdlReadTOC"      ,
};

static const unsigned char cdrom_command_type[MaxCdl] = // 0 = single int, 1 = double int, 2,3,... = others
{
	[CdlSync]		= 1,
	[CdlGetstat]	= 1,
	[CdlSetloc]		= 1,
	[CdlPlay]		= 1,
	[CdlForward]	= 1,
	[CdlBackward]	= 1,
	[CdlReadN]		= 1,
	[CdlStandby]	= 1,
	[CdlStop]		= 1,
	[CdlPause]		= 2,
	[CdlInit]		= 2,
	[CdlMute]		= 1,
	[CdlDemute]		= 1,
	[CdlSetfilter]	= 1,
	[CdlSetmode]	= 1,
	[CdlSetparam]	= 1,
	[CdlGetlocL]	= 1,
	[CdlGetlocP]	= 1,
	[CdlCmd18]		= 0xFF,
	[CdlGetTN]		= 1,
	[CdlGetTD]		= 1,
	[CdlSeekL]		= 2,
	[CdlSeekP]		= 2,
	[CdlCmd23]		= 0xFF,
	[CdlCmd24]		= 0xFF,
	[CdlTest]		= 1,
	[CdlID]			= 2,
	[CdlReadS]		= 1,
	[CdlReset]		= 1,
	[CdlCmd29]		= 0xFF,
	[CdlReadTOC]	= 2,
};

void CdSendCommand(int cmd, int num, ...)
{
	int x;
	va_list ap;
	va_start(ap, num);

// Wait for command execution to end
//	while(CDREG(0) & 128);

// Flush old interrupts
// If this is not done, some events (like the opening of the shell) will hang the CD controller.

	CDREG(0) = 1;
	CDREG(3) = 7;

// Send parameters

	CDREG(0) = 0;

	while(num)
	{
		CDREG(2) = (unsigned char)va_arg(ap, unsigned int);
		num--;
	}

// Send command

	CDREG(0) = 0;

	dprintf("Sending raw CD-ROM command 0x%02X (%s).\n", cmd, cdrom_command_type_str[cmd]);

	CDREG(1) = cmd;

	dprintf("Raw CD-ROM command 0x%02X (%s) expects %d parameters.\n", cmd, cdrom_command_type_str[cmd], cdrom_command_type[cmd]);

// Depending on the number of INTs we expect for a command,
// we wait for an INT to occur, we store the response data returned,
// and we flush the INT.
	for(x = 0; x < cdrom_command_type[cmd]; x++)
	{
		CDREG(0) = 1;
		// PROBLEMATIC INSTRUCTION - CHECK!
		while(CDREG(0) & (1 << 7));

		cdrom_command_stat[x] = CDREG(1);

		dprintf("cdrom_command_stat[%d] = 0x%02X\n", x, cdrom_command_stat[x]);

		CDREG(0) = 1;
		CDREG(3) = 7;
	}

// Store ID number of last executed command (this)
	cdrom_last_command = cmd;

	va_end(ap);
}

int CdReadResults(unsigned char *out, int max)
{
	int x;
	unsigned char *outo = out;
	unsigned char b;

	for(x = 0; x < cdrom_command_type[cdrom_last_command]; x++)
	{
		if(max > 0)
		{
			*(out++) = cdrom_command_stat[x];
			max--;
		}
	}

	CDREG(0) = 1;

	while(CDREG(0) & 0x20)
	{
		b = CDREG(1);
		if(max>0)
		{
			*(out++) = b;
			max--;
		}
	}

	return (out - outo);
}

void _internal_cdrom_handler()
{
	if (want_feedback != false)
	{
		dprintf("INT3?\n");

		want_feedback = false;
	}
	// 0 = ACKNOWLEDGE (0x*2)
	// 1 = ACKNOWLEDGE (0x*2), COMPLETE (0x*3)

/*		int x;



	unsigned char i;


	if(cdrom_command_done)
		return;

	for(x = 0; x < 100; x++); // Waste time

	CDREG(0) = 1;
	i=CDREG(3);

	if((i&0xf)==5) // Error
		cdrom_command_done = 1;

	//printf("i&0xf = %x\n", i&0xf);
//	printf("cdrom_direct_cmd = %x\n", cdrom_direct_cmd);

	switch(kind[cdrom_direct_cmd])
	{
		case 0:
			if(((i&0xf)==3) || ((i&0xf) == 2))
				cdrom_command_done = 1;
		break;

		case 1:
			if((i&0xf)==2)
				cdrom_command_done = 1;
		break;

		case 0xFF: // Unknown command!
				cdrom_command_done = 1;
				return;
		break;
	}

	cdrom_command_stat[0] = i;

	for(x = 0; x < 100; x++); // Waste time

	CDREG(0) = 1;
	CDREG(3) = 7;
	i = CDREG(1);
	cdrom_command_stat[1] = i;

	//printf("cdrom_command_done = %d\n", cdrom_command_done);*/
}

void _internal_cdromlib_init()
{
	printf("Starting CDROMlib...\n");

	EnterCriticalSection(); // Disable IRQs

	SysEnqIntRP(0, cdrom_queue_buf);

	IMASK |= 1 << 2;

	cdrom_handler_callback =  _internal_cdrom_handler;

	cdrom_handler_event_id = OpenEvent(0xF0000003, 2, 0x1000, cdrom_handler);
	EnableEvent(cdrom_handler_event_id);

	dprintf("cdrom_handler_event_id = 0x%08X\n", cdrom_handler_event_id);

	ExitCriticalSection(); // Enable IRQs
}

int CdGetStatus(void)
{
	unsigned char out;

	CdSendCommand(CdlGetstat, 0);
	CdReadResults(&out, 1);

	return out;
}

int CdPlayTrack(unsigned int track)
{
	enum
	{
		//~ 7   Speed       (0=Normal speed, 1=Double speed)
		//~ 6   XA-ADPCM    (0=Off, 1=Send XA-ADPCM sectors to SPU Audio Input)
		//~ 5   Sector Size (0=800h=DataOnly, 1=924h=WholeSectorExceptSyncBytes)
		//~ 4   Ignore Bit  (0=Normal, 1=Ignore Sector Size and Setloc position)
		//~ 3   XA-Filter   (0=Off, 1=Process only XA-ADPCM sectors that match Setfilter)
		//~ 2   Report      (0=Off, 1=Enable Report-Interrupts for Audio Play)
		//~ 1   AutoPause   (0=Off, 1=Auto Pause upon End of Track) ;for Audio Play
		//~ 0   CDDA        (0=Off, 1=Allow to Read CD-DA Sectors; ignore missing EDC)

		CDDA			= 1 << 0,
		AUTOPAUSE		= 1 << 1,
		IGNORE_BIT		= 1 << 4,
		DOUBLE_SPEED	= 1 << 7,
	};

	while(CdGetStatus() & CDSTATUS_SEEK);

	CdSendCommand(CdlSetmode, 1, (unsigned int)(CDDA | AUTOPAUSE | IGNORE_BIT | DOUBLE_SPEED));
	CdSendCommand(CdlPlay, 1, ((track / 10) << 4) | (track % 10));

	while (!(CdGetStatus() & CDSTATUS_PLAY))
	{
		want_feedback = true;
	}

	return 1;
}

unsigned char CdRamRead(unsigned short addr)
{
	unsigned char b;
	addr &= 0x3ff;

	CdSendCommand(0x19, 0x60, addr&0xff, addr >> 8);
	CdReadResults(&b, 1);

	return b;
}
