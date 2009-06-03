//**************************************************************************
//**
//** i_linux.c
//**
//** $Revision$
//** $Date$
//**
//**************************************************************************

#include "h2stdinc.h"
#include <sys/time.h>
#include <sys/stat.h>
#include <errno.h>
#include "h2def.h"
#include "soundst.h"
#include "st_start.h"


extern void I_StartupMouse(void);
extern void I_ShutdownGraphics(void);

extern int startepisode;
extern int startmap;


/*
============================================================================

					TIMER INTERRUPT

============================================================================
*/


int		ticcount;
static long	_startSec;

static void I_StartupTimer(void)
{
	struct timeval tv;
	gettimeofday (&tv, NULL);
	_startSec = tv.tv_sec;
}

//--------------------------------------------------------------------------
//
// FUNC I_GetTime
//
// Returns time in 1/35th second tics.
//
//--------------------------------------------------------------------------

int I_GetTime (void)
{
	struct timeval tv;
	gettimeofday (&tv, NULL);

//	printf ("GT: %lx %lx\n", tv.tv_sec, tv.tv_usec);
//	ticcount = ((tv.tv_sec * 1000000) + tv.tv_usec) / 28571;
	ticcount = ((tv.tv_sec - _startSec) * 35) + (tv.tv_usec / 28571);
	return ticcount;
}


/*
============================================================================

					JOYSTICK

============================================================================
*/

extern int usejoystick;
boolean joystickpresent;

/*
===============
=
= I_StartupJoystick
=
===============
*/

void I_StartupJoystick (void)
{
// NOTHING HERE YET: TOBE IMPLEMENTED IN i_sdl.c
	joystickpresent = false;
}

/*
===============
=
= I_JoystickEvents
=
===============
*/

void I_JoystickEvents (void)
{
}

/*
===============
=
= I_StartFrame
=
===============
*/

void I_StartFrame (void)
{
	I_JoystickEvents();
}


//==========================================================================


/*
===============
=
= I_Init
=
= hook interrupts and set graphics mode
=
===============
*/

void I_Init (void)
{
	I_StartupMouse();
	I_StartupJoystick();
	printf("  S_Init... ");
	S_Init();
	S_Start();

	I_StartupTimer();
}


/*
===============
=
= I_Shutdown
=
= return to default system state
=
===============
*/

void I_Shutdown (void)
{
	S_ShutDown ();
	I_ShutdownGraphics ();
}


/*
================
=
= I_Error
=
================
*/

void I_Error (const char *error, ...)
{
	va_list argptr;

	D_QuitNetGame ();
	I_Shutdown ();
	va_start (argptr, error);
	vfprintf (stderr, error, argptr);
	va_end (argptr);
	fprintf (stderr, "\n");
	exit (1);
}

//--------------------------------------------------------------------------
//
// I_Quit
//
// Shuts down net game, saves defaults, prints the exit text message,
// goes to text mode, and exits.
//
//--------------------------------------------------------------------------

static void I_ENDTEXT (void)
{
// Hexen doesn't have an ENDTEXT
// lump for ANSI output
	printf("\nHexen: Beyond Heretic\n");
}

void I_Quit (void)
{
	D_QuitNetGame();
	M_SaveDefaults();
	I_Shutdown();
	I_ENDTEXT ();

	exit(0);
}

/*
===============
=
= I_ZoneBase
=
===============
*/

byte *I_ZoneBase (int *size)
{
	byte *ptr;
	int heap = 0x800000;

	ptr = (byte *) malloc (heap);
	if (ptr == NULL)
		I_Error ("I_ZoneBase: Insufficient memory!");

	printf ("0x%x allocated for zone, ", heap);
	printf ("ZoneBase: %p\n", ptr);

	*size = heap;
	return ptr;
}

/*
=============
=
= I_AllocLow
=
=============
*/

byte *I_AllocLow (int length)
{
	byte *ptr;

	ptr = (byte *) malloc (length);
	if (ptr == NULL)
		I_Error ("I_AllocLow: Insufficient memory!");

	return ptr;
}

/*
============================================================================

						NETWORKING

============================================================================
*/

extern	doomcom_t	*doomcom;

/*
====================
=
= I_InitNetwork
=
====================
*/

void I_InitNetwork (void)
{
	int		i;

	i = M_CheckParm ("-net");
	if (!i)
	{
	//
	// single player game
	//
		doomcom = (doomcom_t *) malloc (sizeof(*doomcom));
		memset (doomcom, 0, sizeof(*doomcom));
		netgame = false;
		doomcom->id = DOOMCOM_ID;
		doomcom->numplayers = doomcom->numnodes = 1;
		doomcom->deathmatch = false;
		doomcom->consoleplayer = 0;
		doomcom->ticdup = 1;
		doomcom->extratics = 0;
		return;
	}

#if 0
	// THIS IS DOS-ISH AND BROKEN ON UNIX!!!
	doomcom = (doomcom_t *)atoi(myargv[i+1]);
	netgame = true;
	//DEBUG
	doomcom->skill = startskill;
	doomcom->episode = startepisode;
	doomcom->map = startmap;
	doomcom->deathmatch = deathmatch;
#endif
	I_Error ("NET GAME NOT IMPLEMENTED !!!");
}

void I_NetCmd (void)
{
	if (!netgame)
		I_Error ("I_NetCmd when not in netgame");
}


//=========================================================================
//
// I_CheckExternDriver
//
//		Checks to see if a vector, and an address for an external driver
//			have been passed.
//=========================================================================

void I_CheckExternDriver (void)
{
// THIS IS FOR DOS, ONLY. NOTHING ON UNIX.
	useexterndriver = false;
}


//=========================================================================
//
//		MAIN
//
//=========================================================================

static void CreateBasePath (void)
{
#if !defined(_NO_USERDIRS)
	int rc, sz;
	char *base;
	char *homedir = getenv("HOME");
	if (homedir == NULL)
		I_Error ("Unable to determine user home directory");
	/* make sure that basePath has a trailing slash */
	sz = strlen(homedir) + strlen(H_USERDIR) + 3;
	base = (char *) malloc(sz * sizeof(char));
	snprintf(base, sz, "%s/%s/", homedir, H_USERDIR);
	basePath = base;
	rc = mkdir(base, S_IRWXU|S_IRWXG|S_IRWXO);
	if (rc != 0 && errno != EEXIST)
		I_Error ("Unable to create hhexen user directory");
#endif	/* !_NO_USERDIRS */
}

static void InitializeWaddir (void)
{
	int i;
	static const char datadir[] = SHARED_DATAPATH;
						/* defined in config.h */
	const char *_waddir;

	_waddir = NULL;
	i = M_CheckParm("-waddir");
	if (i && i < myargc - 1)
		_waddir = myargv[i + 1];
	if (_waddir == NULL)
		_waddir = getenv(DATA_ENVVAR);
	if (_waddir == NULL)
	{
		if (datadir[0])
			_waddir = datadir;
	}
	waddir = _waddir;
	if (waddir && *waddir)
		printf ("Shared data path: %s\n", waddir);
}

static void PrintVersion (void)
{
	printf ("HHexen (%s) v%d.%d.%d\n", VERSION_PLATFORM, VERSION_MAJ, VERSION_MIN, VERSION_PATCH);
}

static void PrintHelp (const char *name)
{
	printf ("http://sourceforge.net/projects/hhexen\n");
	printf ("http://hhexen.sf.net , http://icculus.org/hast\n");
	printf ("\n");
	printf ("Usage: %s [options]\n", name);
	printf ("     [ -h | --help]           Display this help message\n");
	printf ("     [ -v | --version]        Display the game version\n");
	printf ("     [ -f | --fullscreen]     Run the game fullscreen\n");
	printf ("     [ -w | --windowed]       Run the game windowed\n");
	printf ("     [ -s | --nosound]        Run the game without sound\n");
	printf ("     [ -g | --nograb]         Disable mouse grabbing\n");
#ifdef RENDER3D
	printf ("     [ -width  <width> ]      Set screen width \n");
	printf ("     [ -height <height> ]     Set screen height\n");
#endif
	printf ("     [ -file <wadfile> ]      Load extra wad files\n");
	printf ("     [ -waddir <path>  ]      Specify shared data path\n");
	printf ("  You can use the %s environment variable or the\n",
		DATA_ENVVAR);
	printf (" -waddir option to force the game's shared data directory.\n");
}

#define H2_LITTLE_ENDIAN	0	/* 1234 */
#define H2_BIG_ENDIAN		1	/* 4321 */
#define H2_PDP_ENDIAN		2	/* 3412 */

#if defined(WORDS_BIGENDIAN)
#define COMPILED_BYTEORDER	H2_BIG_ENDIAN
#else
#define COMPILED_BYTEORDER	H2_LITTLE_ENDIAN
#endif

static int DetectByteorder (void)
{
	int	i = 0x12345678;
		/*    U N I X */

	/*
	BE_ORDER:  12 34 56 78
		   U  N  I  X

	LE_ORDER:  78 56 34 12
		   X  I  N  U

	PDP_ORDER: 34 12 78 56
		   N  U  X  I
	*/

	if ( *(char *)&i == 0x12 )
		return H2_BIG_ENDIAN;
	else if ( *(char *)&i == 0x78 )
		return H2_LITTLE_ENDIAN;
	else if ( *(char *)&i == 0x34 )
		return H2_PDP_ENDIAN;

	return -1;
}

static void ValidateByteorder (void)
{
	const char	*endianism[] = { "LE", "BE", "PDP" };
	int		host_byteorder;

	host_byteorder = DetectByteorder ();
	if (host_byteorder < 0)
	{
		fprintf (stderr, "Unsupported byte order.\n");
		exit (1);
	}
	if (host_byteorder != COMPILED_BYTEORDER)
	{
		fprintf (stderr, "Detected byte order %s doesn't match compiled %s order!\n",
				 endianism[host_byteorder], endianism[COMPILED_BYTEORDER]);
		exit (1);
	}
	printf ("Detected byte order: %s\n", endianism[host_byteorder]);
}

int main (int argc, char **argv)
{
	PrintVersion ();

	myargc = argc;
	myargv = (const char **) argv;

	if (M_CheckParm("--version") || M_CheckParm("-v"))
		return 0;
	if (M_CheckParm("--help") || M_CheckParm("-h") || M_CheckParm("-?"))
	{
		PrintHelp (argv[0]);
		return 0;
	}

	ValidateByteorder();
	CreateBasePath();
	InitializeWaddir();

	H2_Main();

	return 0;
}

