#ifndef _PSXCDROM_H
#define _PSXCDROM_H

// CD-ROM status. Extracted from No$PSX specs:

//  7  Play          Playing CD-DA         ;\only ONE of these bits can be set
//  6  Seek          Seeking               ; at a time (ie. Read/Play won't get
//  5  Read          Reading data sectors  ;/set until after Seek completion)
//  4  ShellOpen     Once shell open (0=Closed, 1=Is/was Open)
//  3  IdError       (0=Okay, 1=GetID denied) (also set when Setmode.Bit4=1)
//  2  SeekError     (0=Okay, 1=Seek error)     (followed by Error Byte)
//  1  Spindle Motor (0=Motor off, or in spin-up phase, 1=Motor on)
//  0  Error         Invalid Command/parameters (followed by Error Byte)

#define CDSTATUS_PLAY			0x80
#define CDSTATUS_SEEK			0x40
#define CDSTATUS_READ			0x20
#define CDSTATUS_SHELLOPEN		0x10
#define CDSTATUS_ID_ERROR		0x08
#define CDSTATUS_SEEK_ERROR		0x04
#define CDSTATUS_SPINDLE_MOTOR	0x02
#define CDSTATUS_ERROR			0x01

// Command names

enum
{
	CdlSync = 0,
	CdlNop = 1,  CdlGetstat = 1,
	CdlSetloc = 2,
	CdlPlay = 3,
	CdlForward = 4,
	CdlBackward = 5,
	CdlReadN = 6,
	CdlStandby = 7,
	CdlStop = 8,
	CdlPause = 9,
	CdlInit = 10,
	CdlMute = 11,
	CdlDemute = 12,
	CdlSetfilter = 13,
	CdlSetmode = 14,
	CdlSetparam = 15,
	CdlGetlocL = 16,
	CdlGetlocP = 17,
	CdlCmd18 = 18,
	CdlGetTN = 19,
	CdlGetTD = 20,
	CdlSeekL = 21,
	CdlSeekP = 22,
	CdlCmd23 = 23,
	CdlCmd24 = 24,
	CdlTest = 25,
	CdlID = 26,
	CdlReadS = 27,
	CdlReset = 28,
	CdlCmd29 = 29,
	CdlReadTOC = 30,
	MaxCdl
};

/*
 * Send a low-level CDROM command
 * cmd = command number
 * num = number of arguments
 * ... = arguments
 */

void CdSendCommand(int cmd, int num, ...);

/**
 * Reads the results of a low-level CDROM command
 *
 * @param out Pointer to array of chars where the output will be stored
 * @param max Maximum number of bytes to store
 *
 * Return value: number of results.
 */

int CdReadResults(unsigned char *out, int max);

/**
 * Gets CDROM drive status
 * @return CDROM drive status bitmask
 */

int CdGetStatus(void);

/**
 * Play an Audio CD track
 * @return 1 on success, 0 on failure
 */

int CdPlayTrack(unsigned int track);

unsigned char CdRamRead(unsigned short addr);


#endif
