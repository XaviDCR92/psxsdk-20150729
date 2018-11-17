/*
 * Low-level CDROM library
 */

#include <stdio.h>
#include <stdarg.h>
#include <psx.h>

#define CDREG(x)                *((volatile unsigned char*)(0x1f801800 | (x & 0x3)))
#define IMASK                   *((volatile unsigned int*)0x1f801074)
#define IPENDING                *((volatile unsigned int*)0x1f801070)
#define CDROM_HW_EVENT_ADDR     ((unsigned int)0xF0000003)
#define CDROM_UNLIMITED_PARAMS  ((unsigned char)0xFF)

static const unsigned char CdCommandParams[MaxCdl] = // 0 = single int, 1 = double int, 2,3,... = others
{
    [CdlSync]       = 1,
    [CdlGetstat]    = 1,
    [CdlSetloc]     = 1,
    [CdlPlay]       = 1,
    [CdlForward]    = 1,
    [CdlBackward]   = 1,
    [CdlReadN]      = 1,
    [CdlStandby]    = 1,
    [CdlStop]       = 1,
    [CdlPause]      = 2,
    [CdlInit]       = 2,
    [CdlMute]       = 1,
    [CdlDemute]     = 1,
    [CdlSetfilter]  = 1,
    [CdlSetmode]    = 1,
    [CdlSetparam]   = 1,
    [CdlGetlocL]    = 1,
    [CdlGetlocP]    = 1,
    [CdlCmd18]      = CDROM_UNLIMITED_PARAMS,
    [CdlGetTN]      = 1,
    [CdlGetTD]      = 1,
    [CdlSeekL]      = 2,
    [CdlSeekP]      = 2,
    [CdlCmd23]      = CDROM_UNLIMITED_PARAMS,
    [CdlCmd24]      = CDROM_UNLIMITED_PARAMS,
    [CdlTest]       = 1,
    [CdlID]         = 2,
    [CdlReadS]      = 1,
    [CdlReset]      = 1,
    [CdlCmd29]      = CDROM_UNLIMITED_PARAMS,
    [CdlReadTOC]    = 2
};

volatile int cdrom_handler_event_id;
volatile unsigned char cdrom_last_command;
volatile unsigned char cdrom_command_stat[2];

static void CdSetIndex(const unsigned char index);
int* _internal_cdrom_handler(void);

enum tCdInt CdGetInterrupt(void)
{
    enum tCdInt eCdInt;

    /* Set index 1 on index/status CD-ROM register
     * so triggered interrupts can be read. */
    CdSetIndex(1);

    while ((eCdInt = (enum tCdInt)(CDREG(3) & INT_MASK)) == CD_NOINT);

    return eCdInt;
}

static void CdAcknowledgeInterrupts(void)
{
    enum
    {
        ALL_CD_INT_MASK = 0x1F
    };

    /* Set index 1 on index/status CD-ROM register
     * so that previous CD-ROM can be acknowledged. */
    CdSetIndex(1);

    /* Acknowledge all previous CD-ROM interrupts. */
    CDREG(3) = ALL_CD_INT_MASK;
}

void CdSendCommand(const enum tCdCmd eCmd, const size_t num, ...)
{
    va_list ap;

    /* Initialize variable-argument list. */
    va_start(ap, num);

    /* Acknowledge previous CD-ROM interrupts. */
    CdAcknowledgeInterrupts();

    /* Set index 0 on index/status CD-ROM register
     * so command parameters and command byte are sent. */
    CdSetIndex(0);

    {
        enum
        {
            COMMAND_PARAMETER_BUSY_BIT = 1 << 7
        };

        size_t i;

        for (i = 0; i < num; i++)
        {
            enum
            {
                PARAMETER_FIFO_FULL_BIT = 1 << 4
            };

            /* Wait until parameter FIFO is empty and
             * parameter/command busy flag is cleared. */
            while ( (CDREG(0) & PARAMETER_FIFO_FULL_BIT)
                                ||
                    (CDREG(0) & COMMAND_PARAMETER_BUSY_BIT) );

            /* Send command parameters. */
            CDREG(2) = (unsigned char)va_arg(ap, unsigned int);
        }

        /* Wait until parameter/command busy flag is cleared. */
        while (CDREG(0) & COMMAND_PARAMETER_BUSY_BIT);
    }

    /* Send command. */
    CDREG(1) = (unsigned char)eCmd;

    {
        size_t i;

        /* Depending on the number of INTs we expect for a command,
         * we wait for an INT to occur, we store the response data returned,
         * and we flush the INT. */
        for(i = 0; i < CdCommandParams[eCmd]; i++)
        {
            /* Set index 1 on index/status CD-ROM register
             * so that command results can be read. */
            CdSetIndex(1);

            /* Read status from CD-ROM command. */
            cdrom_command_stat[i] = CDREG(1);

            /* Acknowledge CD-ROM interrupts. */
            CdAcknowledgeInterrupts();
        }
    }

    /* Store ID number of last executed command. */
    cdrom_last_command = eCmd;

    /* De-initialize variable-argument list. */
    va_end(ap);
}

static void CdSetIndex(const unsigned char index)
{
    enum
    {
        MAX_CDROM_INDEX = 3
    };

    if (index <= MAX_CDROM_INDEX)
    {
        CDREG(0) = index;
    }
    else
    {
        /* Invalid selected index. Exit. */
    }
}

int CdReadResults(unsigned char *out, int max)
{
    int x;
    unsigned char *outo = out;
    unsigned char b;

    for(x = 0; x < CdCommandParams[cdrom_last_command]; x++)
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

void _internal_cdromlib_init()
{
    static unsigned int cdrom_queue_buf[4] =
    {
        /* Will contain next interrupt handler in queue */
        0x0,

        /* func1 */
        0x0,

        /* func2 */
        (unsigned int)_internal_cdrom_handler,

        /* pad */
        0x0,
    };

    EnterCriticalSection(); // Disable IRQs

    SysEnqIntRP(0, cdrom_queue_buf);

    IMASK |= 1 << 2;

    {
        const unsigned int eventID = OpenEvent(CDROM_HW_EVENT_ADDR, 2, 0x1000, _internal_cdrom_handler);
        EnableEvent(eventID);
    }

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
        // 7   Speed       (0=Normal speed, 1=Double speed)
        // 6   XA-ADPCM    (0=Off, 1=Send XA-ADPCM sectors to SPU Audio Input)
        // 5   Sector Size (0=800h=DataOnly, 1=924h=WholeSectorExceptSyncBytes)
        // 4   Ignore Bit  (0=Normal, 1=Ignore Sector Size and Setloc position)
        // 3   XA-Filter   (0=Off, 1=Process only XA-ADPCM sectors that match Setfilter)
        // 2   Report      (0=Off, 1=Enable Report-Interrupts for Audio Play)
        // 1   AutoPause   (0=Off, 1=Auto Pause upon End of Track) ;for Audio Play
        // 0   CDDA        (0=Off, 1=Allow to Read CD-DA Sectors; ignore missing EDC)

        CDDA            = 1 << 0,
        AUTOPAUSE       = 1 << 1,
        IGNORE_BIT      = 1 << 4,
        DOUBLE_SPEED    = 1 << 7,
    };

    while(CdGetStatus() & CDSTATUS_SEEK);

    CdSendCommand(CdlSetmode, 1, (unsigned int)(CDDA | AUTOPAUSE | IGNORE_BIT | DOUBLE_SPEED));
    CdSendCommand(CdlPlay, 1, ((track / 10) << 4) | (track % 10));

    while (!(CdGetStatus() & CDSTATUS_PLAY));

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
