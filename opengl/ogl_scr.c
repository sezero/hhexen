
//**************************************************************************
//**
//** OGL_SCR.C
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "h2stdinc.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif

#include <GL/gl.h>
#include <GL/glu.h>
#include "h2def.h"
#include "r_local.h"
#include "ogl_def.h"
#include "p_local.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

void OGL_GrabScreen (void)
{
	int	i, len, temp;
	char	tganame[MAX_OSPATH], *p;
	byte	*buffer;

//
// find a file name to save it to
//
	snprintf (tganame, sizeof(tganame), "%shexen00.tga", basePath);
	p = tganame + strlen(basePath);
	for (i = 0; i <= 99; i++)
	{
		p[5] = i/10 + '0';
		p[6] = i%10 + '0';
		if (access(tganame, F_OK) == -1)
			break;	// file doesn't exist
	}
	if (i == 100)
	{
		P_SetMessage(&players[consoleplayer], "SCREEN SHOT FAILED", false);
		return;
	}

	// NOTE: 18 == sizeof(TargaHeader)
	len = 18 + screenWidth * screenHeight * 3;
	// Z_Malloc would fail here (Z mem is too little)
	buffer = (byte *) malloc (len);
	if (buffer == NULL)
		I_Error ("OGL_GrabScreen: No memory for screenshot");
	memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = screenWidth & 255;
	buffer[13] = screenWidth >> 8;
	buffer[14] = screenHeight & 255;
	buffer[15] = screenHeight >> 8;
	buffer[16] = 24;	// pixel size

	glReadPixels (0, 0, screenWidth, screenHeight, GL_RGB, GL_UNSIGNED_BYTE, buffer + 18);

	// swap rgb to bgr
	for (i = 18; i < len; i += 3)
	{
		temp = buffer[i];
		buffer[i] = buffer[i+2];
		buffer[i+2] = temp;
	}
	M_WriteFile (tganame, buffer, len);

	free (buffer);
	P_SetMessage(&players[consoleplayer], "SCREEN SHOT", false);
}

