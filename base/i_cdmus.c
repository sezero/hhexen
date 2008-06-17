
//**************************************************************************
//**
//** i_cdmus.c
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "h2stdinc.h"
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#ifdef HAVE_LINUX_CDROM_H
#include <linux/cdrom.h>
#endif
#include "h2def.h"
#include "i_sound.h"

// MACROS ------------------------------------------------------------------

// #define MAX_AUDIO_TRACKS 25

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

int cd_Error;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

//static int cd_FirstTrack;
//static int cd_LastTrack;

static int cdfile = -1;
//static char cd_dev[64] = "/dev/cdrom";

// CODE --------------------------------------------------------------------

static int I_CDGetDiskInfo(void)
{
	return 0;
}
//==========================================================================
//
// I_CDMusInit
//
// Initializes the CD audio system.  Must be called before using any
// other I_CDMus functions.
//
// Returns: 0 (ok) or -1 (error, in cd_Error).
//
//==========================================================================

int I_CDMusInit(void)
{
	//open CD device
	I_CDGetDiskInfo ();
	return -1;	// not implemented yet
}

//==========================================================================
//
// I_CDMusPlay
//
// Play an audio CD track.
//
// Returns: 0 (ok) or -1 (error, in cd_Error).
//
//==========================================================================

int I_CDMusPlay(int track)
{
	return 0;
}

//==========================================================================
//
// I_CDMusStop
//
// Stops the playing of an audio CD.
//
// Returns: 0 (ok) or -1 (error, in cd_Error).
//
//==========================================================================

int I_CDMusStop(void)
{
	return 0;
}

//==========================================================================
//
// I_CDMusResume
//
// Resumes the playing of an audio CD.
//
// Returns: 0 (ok) or -1 (error, in cd_Error).
//
//==========================================================================

int I_CDMusResume(void)
{
	return 0;
}

//==========================================================================
//
// I_CDMusSetVolume
//
// Sets the CD audio volume (0 - 255).
//
// Returns: 0 (ok) or -1 (error, in cd_Error).
//
//==========================================================================

int I_CDMusSetVolume(int volume)
{
	return 0;
}

//==========================================================================
//
// I_CDMusFirstTrack
//
// Returns: the number of the first track.
//
//==========================================================================

int I_CDMusFirstTrack(void)
{
	return 0;
}

//==========================================================================
//
// I_CDMusLastTrack
//
// Returns: the number of the last track.
//
//==========================================================================

int I_CDMusLastTrack(void)
{
	return 0;
}

//==========================================================================
//
// I_CDMusShutDown
//
//==========================================================================

void I_CDMusShutdown(void)
{
	if (cdfile != -1)
		close(cdfile);
	cdfile = -1;
}

//==========================================================================
//
// I_CDMusUpdate
//
//==========================================================================

void I_CDMusUpdate(void)
{

}              

