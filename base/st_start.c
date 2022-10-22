//**************************************************************************
//**
//** st_start.c : Heretic 2 : Raven Software, Corp.
//**
//**************************************************************************


// HEADER FILES ------------------------------------------------------------

#include "h2stdinc.h"
#include <sys/stat.h>
#include <fcntl.h>
#include "h2def.h"
#include "st_start.h"


// MACROS ------------------------------------------------------------------

#define ST_MAX_NOTCHES		32
#define ST_NOTCH_WIDTH		16
#define ST_NOTCH_HEIGHT		23
#define ST_PROGRESS_X		64	/* Start of notches x screen pos. */
#define ST_PROGRESS_Y		441	/* Start of notches y screen pos. */

#define ST_NETPROGRESS_X	288
#define ST_NETPROGRESS_Y	32
#define ST_NETNOTCH_WIDTH	8
#define ST_NETNOTCH_HEIGHT	16
#define ST_MAX_NETNOTCHES	8

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------
char *ST_LoadScreen(void);
void ST_UpdateNotches(int notchPosition);
void ST_UpdateNetNotches(int notchPosition);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//--------------------------------------------------------------------------
//
// Startup Screen Functions
//
//--------------------------------------------------------------------------

//==========================================================================
//
// ST_Init - Do the startup screen
//
//==========================================================================

void ST_Init(void)
{
}


void ST_Done(void)
{
}


//==========================================================================
//
// ST_UpdateNotches
//
//==========================================================================

void ST_UpdateNotches(int notchPosition)
{
}


//==========================================================================
//
// ST_UpdateNetNotches - indicates network progress
//
//==========================================================================

void ST_UpdateNetNotches(int notchPosition)
{
}


//==========================================================================
//
// ST_Progress - increments progress indicator
//
//==========================================================================

void ST_Progress(void)
{
}


//==========================================================================
//
// ST_NetProgress - indicates network progress
//
//==========================================================================

void ST_NetProgress(void)
{
}


//==========================================================================
//
// ST_NetDone - net progress complete
//
//==========================================================================

void ST_NetDone(void)
{
	S_StartSound(NULL, SFX_PICKUP_WEAPON);
}


//==========================================================================
//
// ST_Message - gives debug message
//
//==========================================================================

void ST_Message(const char *message, ...)
{
	va_list argptr;
	char buffer[MAX_ST_MSG];

	va_start(argptr, message);
	vsnprintf(buffer, sizeof(buffer), message, argptr);
	va_end(argptr);

//	if (debugmode)
		printf("%s", buffer);
}

//==========================================================================
//
// ST_RealMessage - gives user message
//
//==========================================================================

void ST_RealMessage(const char *message, ...)
{
	va_list argptr;
	char buffer[MAX_ST_MSG];

	va_start(argptr, message);
	vsnprintf(buffer, sizeof(buffer), message, argptr);
	va_end(argptr);

	printf("%s", buffer);		// Always print these messages
}


//==========================================================================
//
// ST_LoadScreen - loads startup graphic
//
//==========================================================================

char *ST_LoadScreen(void)
{
	int length, lump;
	char *buffer;

	lump = W_GetNumForName("STARTUP");
	length = W_LumpLength(lump);
	buffer = (char *)Z_Malloc(length, PU_STATIC, NULL);
	W_ReadLump(lump, buffer);
	return (buffer);
}

