#ifndef _PSXCDROM_H
#define _PSXCDROM_H

#include <stddef.h>

/*  CD-ROM status. Extracted from No$PSX specs:

    7  Play          Playing CD-DA         ;\only ONE of these bits can be set
    6  Seek          Seeking               ; at a time (ie. Read/Play won't get
    5  Read          Reading data sectors  ;/set until after Seek completion)
    4  ShellOpen     Once shell open (0=Closed, 1=Is/was Open)
    3  IdError       (0=Okay, 1=GetID denied) (also set when Setmode.Bit4=1)
    2  SeekError     (0=Okay, 1=Seek error)     (followed by Error Byte)
    1  Spindle Motor (0=Motor off, or in spin-up phase, 1=Motor on)
    0  Error         Invalid Command/parameters (followed by Error Byte) */

#define CDSTATUS_PLAY           0x80
#define CDSTATUS_SEEK           0x40
#define CDSTATUS_READ           0x20
#define CDSTATUS_SHELLOPEN      0x10
#define CDSTATUS_ID_ERROR       0x08
#define CDSTATUS_SEEK_ERROR     0x04
#define CDSTATUS_SPINDLE_MOTOR  0x02
#define CDSTATUS_ERROR          0x01

// Command names

enum tCdCmd
{
    CdlSync,
    CdlGetstat,
    CdlSetloc,
    CdlPlay,
    CdlForward,
    CdlBackward,
    CdlReadN,
    CdlStandby,
    CdlStop,
    CdlPause,
    CdlInit,
    CdlMute,
    CdlDemute,
    CdlSetfilter,
    CdlSetmode,
    CdlSetparam,
    CdlGetlocL,
    CdlGetlocP,
    CdlCmd18,
    CdlGetTN,
    CdlGetTD,
    CdlSeekL,
    CdlSeekP,
    CdlCmd23,
    CdlCmd24,
    CdlTest,
    CdlID,
    CdlReadS,
    CdlReset,
    CdlCmd29,
    CdlReadTOC,

    MaxCdl,

    CdlNop = CdlGetstat
};

enum tCdInt
{
    CD_NOINT,   /**< No response received (no interrupt request). */
    CD_INT1,    /**< Received SECOND (or further) response to ReadS/ReadN (and Play+Report). */
    CD_INT2,    /**< Received SECOND response (to various commands). */
    CD_INT3,    /**< Received FIRST response (to any command). */
    CD_INT4,    /**< DataEnd (when Play/Forward reaches end of disk) (maybe also for Read?). */
    CD_INT5,    /**<    Received error-code (in FIRST or SECOND response).
                        INT5 also occurs on SECOND GetID response, on unlicensed disks.
                        INT5 also occurs when opening the drive door (even if no command
                        was sent, ie. even if no read-command or other command is active) */
    CD_INT6,    /**< N/A. */
    CD_INT7,    /**< N/A. */
    INT_MASK = CD_INT7
};

/*
 * Sends a low-level CD-ROM command
 * cmd = command number
 * num = number of arguments
 * ... = arguments
 */
void CdSendCommand(const enum tCdCmd eCmd, size_t num, ...);

/**
 * Reads the results of a low-level CDROM command
 *
 * @param out Pointer to array of chars where the output will be stored
 * @param max Maximum number of bytes to store
 *
 * Return value: number of results.
 */
int CdReadResults(unsigned char *out, const int max);

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

enum tCdInt CdGetInterrupt(void);

unsigned char CdRamRead(unsigned short addr);


#endif
