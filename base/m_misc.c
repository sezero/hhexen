
//**************************************************************************
//**
//** m_misc.c : Heretic 2 : Raven Software, Corp.
//**
//** $Revision$
//** $Date$
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "h2stdinc.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include "h2def.h"
#include "p_local.h"
#include "i_cdmus.h"
#include "soundst.h"
#ifdef __WATCOMC__
#include "i_sound.h"
#endif
#ifdef RENDER3D
#include "ogl_def.h"
#endif

// MACROS ------------------------------------------------------------------

#define MALLOC_CLIB	1
#define MALLOC_ZONE	2

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static int ReadFile(char const *name, void **buffer, int mallocType);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

int		myargc;
const char	**myargv;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
// M_CheckParm
//
// Checks for the given parameter in the program's command line arguments.
// Returns the argument number (1 to argc-1) or 0 if not present.
//
//==========================================================================

int M_CheckParm(const char *check)
{
	int i;

	for (i = 1; i < myargc; i++)
	{
		if (!strcasecmp(check, myargv[i]))
		{
			return i;
		}
	}
	return 0;
}

//==========================================================================
//
// M_ParmExists
//
// Returns true if the given parameter exists in the program's command
// line arguments, false if not.
//
//==========================================================================

boolean M_ParmExists(const char *check)
{
	return M_CheckParm(check) != 0 ? true : false;
}

//==========================================================================
//
// M_ExtractFileBase
//
//==========================================================================

void M_ExtractFileBase(const char *path, char *dest)
{
	const char *src;
	int length;

	src = path + strlen(path) - 1;
	if (src <= path)
	{
		*dest = '\0';
		return;
	}

	// Back up until a \ or the start
	while (src != path && src[-1] != '/' && src[-1] != '\\')
		src--;

	// Copy up to eight characters
	memset(dest, 0, 8);
	length = 0;
	while (*src && *src != '.')
	{
		if (++length == 9)
		{
			I_Error("Filename base of %s > 8 chars", path);
		}
		*dest++ = toupper((int)*src++);
	}
}

/*
===============
=
= M_Random
=
= Returns a 0-255 number
=
===============
*/

// This is the new flat distribution table
unsigned char rndtable[256] =
{
	201,  1,243, 19, 18, 42,183,203,101,123,154,137, 34,118, 10,216,
	135,246,  0,107,133,229, 35,113,177,211,110, 17,139, 84,251,235,
	182,166,161,230,143, 91, 24, 81, 22, 94,  7, 51,232,104,122,248,
	175,138,127,171,222,213, 44, 16,  9, 33, 88,102,170,150,136,114,
	 62,  3,142,237,  6,252,249, 56, 74, 30, 13, 21,180,199, 32,132,
	187,234, 78,210, 46,131,197,  8,206,244, 73,  4,236,178,195, 70,
	121, 97,167,217,103, 40,247,186,105, 39, 95,163, 99,149,253, 29,
	119, 83,254, 26,202, 65,130,155, 60, 64,184,106,221, 93,164,196,
	112,108,179,141, 54,109, 11,126, 75,165,191,227, 87,225,156, 15,
	 98,162,116, 79,169,140,190,205,168,194, 41,250, 27, 20, 14,241,
	 50,214, 72,192,220,233, 67,148, 96,185,176,181,215,207,172, 85,
	 89, 90,209,128,124,  2, 55,173, 66,152, 47,129, 59, 43,159,240,
	239, 12,189,212,144, 28,200, 77,219,198,134,228, 45, 92,125,151,
	  5, 53,255, 52, 68,245,160,158, 61, 86, 58, 82,117, 37,242,145,
	 69,188,115, 76, 63,100, 49,111,153, 80, 38, 57,174,224, 71,231,
	 23, 25, 48,218,120,147,208, 36,226,223,193,238,157,204,146, 31
};

int rndindex = 0;
int prndindex = 0;

unsigned char P_Random (void)
{
	prndindex = (prndindex + 1) & 0xff;
	return rndtable[prndindex];
}

int M_Random (void)
{
	rndindex = (rndindex + 1) & 0xff;
	return rndtable[rndindex];
}

void M_ClearRandom (void)
{
	rndindex = prndindex = 0;
}


void M_ClearBox (fixed_t *box)
{
	box[BOXTOP] = box[BOXRIGHT] = H2MININT;
	box[BOXBOTTOM] = box[BOXLEFT] = H2MAXINT;
}

void M_AddToBox (fixed_t *box, fixed_t x, fixed_t y)
{
	if (x < box[BOXLEFT])
		box[BOXLEFT] = x;
	else if (x > box[BOXRIGHT])
		box[BOXRIGHT] = x;
	if (y < box[BOXBOTTOM])
		box[BOXBOTTOM] = y;
	else if (y > box[BOXTOP])
		box[BOXTOP] = y;
}

/*
==================
=
= M_WriteFile
=
==================
*/

boolean M_WriteFile (char const *name, const void *source, int length)
{
	int handle, count;

	handle = open (name, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666);
	if (handle == -1)
		return false;
	count = write (handle, source, length);
	close (handle);

	if (count < length)
		return false;

	return true;
}

//==========================================================================
//
// M_ReadFile
//
// Read a file into a buffer allocated using Z_Malloc().
//
//==========================================================================

int M_ReadFile(char const *name, void **buffer)
{
	return ReadFile(name, buffer, MALLOC_ZONE);
}

//==========================================================================
//
// M_ReadFileCLib
//
// Read a file into a buffer allocated using malloc().
//
//==========================================================================

int M_ReadFileCLib(char const *name, void **buffer)
{
	return ReadFile(name, buffer, MALLOC_CLIB);
}

//==========================================================================
//
// ReadFile
//
//==========================================================================

static int ReadFile(char const *name, void **buffer, int mallocType)
{
	int handle, count, length;
	struct stat fileinfo;
	void *buf;

	handle = open(name, O_RDONLY|O_BINARY, 0666);
	if (handle == -1)
	{
		I_Error("Couldn't read file %s", name);
	}
	if (fstat(handle, &fileinfo) == -1)
	{
		I_Error("Couldn't read file %s", name);
	}
	length = fileinfo.st_size;
	if (mallocType == MALLOC_ZONE)
	{ // Use zone memory allocation
		buf = Z_Malloc(length, PU_STATIC, NULL);
	}
	else
	{ // Use c library memory allocation
		buf = malloc(length);
		if (buf == NULL)
		{
			I_Error("Couldn't malloc buffer %d for file %s.",
				length, name);
		}
	}
	count = read(handle, buf, length);
	close(handle);
	if (count < length)
	{
		I_Error("Couldn't read file %s", name);
	}
	*buffer = buf;
	return length;
}

//---------------------------------------------------------------------------
//
// PROC M_FindResponseFile
//
//---------------------------------------------------------------------------

#define MAXARGVS	100

void M_FindResponseFile(void)
{
	int i;

	for (i = 1; i < myargc; i++)
	{
		if (myargv[i][0] == '@')
		{
			FILE *handle;
			int size, k, idx;
			int indexinfile;
			char *infile;
			char *file;
			const char *moreargs[20];
			const char *firstargv;

			// READ THE RESPONSE FILE INTO MEMORY
			handle = fopen(&myargv[i][1], "rb");
			if (!handle)
			{
				printf("\nNo such response file!");
				exit(1);
			}
			ST_Message("Found response file %s!\n",&myargv[i][1]);
			fseek (handle, 0, SEEK_END);
			size = ftell(handle);
			fseek (handle, 0, SEEK_SET);
			file = (char *) malloc (size);
			fread (file, size, 1, handle);
			fclose (handle);

			// KEEP ALL CMDLINE ARGS FOLLOWING @RESPONSEFILE ARG
			for (idx = 0, k = i + 1; k < myargc; k++)
				moreargs[idx++] = myargv[k];

			firstargv = myargv[0];
			myargv = (const char **) calloc(1, sizeof(char *) * MAXARGVS);
			myargv[0] = firstargv;

			infile = file;
			indexinfile = k = 0;
			indexinfile++;	// SKIP PAST ARGV[0] (KEEP IT)
			do
			{
				myargv[indexinfile++] = infile + k;
				while (k < size && ((*(infile + k) >= ' ' + 1) && (*(infile + k) <= 'z')))
					k++;
				*(infile + k) = 0;
				while (k < size && ((*(infile + k) <= ' ') || (*(infile + k) > 'z')))
					k++;
			} while (k < size);

			for (k = 0; k < idx; k++)
				myargv[indexinfile++] = moreargs[k];
			myargc = indexinfile;
			// DISPLAY ARGS
			if (M_CheckParm("-debug"))
			{
				ST_Message("%d command-line args:\n", myargc);
				for (k = 1; k < myargc; k++)
				{
					ST_Message("%s\n", myargv[k]);
				}
			}
			break;
		}
	}
}

//---------------------------------------------------------------------------
//
// PROC M_ForceUppercase
//
// Change string to uppercase.
//
//---------------------------------------------------------------------------

void M_ForceUppercase(char *text)
{
	char c;

	while ((c = *text) != 0)
	{
		if (c >= 'a' && c <= 'z')
		{
			*text++ = c-('a'-'A');
		}
		else
		{
			text++;
		}
	}
}

/*
==============================================================================

							DEFAULTS

==============================================================================
*/

int	usemouse;
int	usejoystick;

int	mouseSensitivity;

extern int mouselook;
extern int alwaysrun;

extern int key_right, key_left, key_up, key_down;
extern int key_strafeleft, key_straferight, key_jump;
extern int key_fire, key_use, key_strafe, key_speed;
extern int key_flyup, key_flydown, key_flycenter;
extern int key_lookup, key_lookdown, key_lookcenter;
extern int key_invleft, key_invright, key_useartifact;

extern int mousebfire;
extern int mousebstrafe;
extern int mousebforward;
extern int mousebjump;

extern int joybfire;
extern int joybstrafe;
extern int joybuse;
extern int joybspeed;
extern int joybjump;

extern int messageson;

extern int viewwidth, viewheight;

extern int screenblocks;

extern int snd_Channels;
#ifdef __WATCOMC__
extern int snd_DesiredMusicDevice, snd_DesiredSfxDevice;
extern int snd_MusicDevice,	// current music card # (index to dmxCodes)
	   snd_SfxDevice;	// current sfx card # (index to dmxCodes)

extern int snd_SBport, snd_SBirq, snd_SBdma;	// sound blaster variables
extern int snd_Mport;				// midi variables
#endif	/* DOS vars */

default_t defaults[] =
{
/* change of order here affects mn_menu.c :
 * see, for example, Options3Items there...
 */
	{ "mouse_sensitivity",	&mouseSensitivity,	5,	0, 50 },
	{ "sfx_volume",		&snd_MaxVolume,		10,	0, 15 },
	{ "music_volume",	&snd_MusicVolume,	10,	0, 15 },

	{ "key_right",		&key_right,		KEY_RIGHTARROW,	0, 254 },
	{ "key_left",		&key_left,		KEY_LEFTARROW,	0, 254 },
	{ "key_up",		&key_up,		KEY_UPARROW,	0, 254 },
	{ "key_down",		&key_down,		KEY_DOWNARROW,	0, 254 },
	{ "key_strafeleft",	&key_strafeleft,	',',		0, 254 },
	{ "key_straferight",	&key_straferight,	'.',		0, 254 },
	{ "key_flyup",		&key_flyup,		KEY_PGUP,	0, 254 },
	{ "key_flydown",	&key_flydown,		KEY_INS,	0, 254 },
	{ "key_flycenter",	&key_flycenter,		KEY_HOME,	0, 254 },
	{ "key_lookup",		&key_lookup,		KEY_PGDN,	0, 254 },
	{ "key_lookdown",	&key_lookdown,		KEY_DEL,	0, 254 },
	{ "key_lookcenter",	&key_lookcenter,	KEY_END,	0, 254 },
	{ "key_invleft",	&key_invleft,		'[',		0, 254 },
	{ "key_invright",	&key_invright,		']',		0, 254 },
	{ "key_useartifact",	&key_useartifact,	KEY_ENTER,	0, 254 },
	{ "key_fire",		&key_fire,		KEY_RCTRL,	0, 254 },
	{ "key_use",		&key_use,		' ',		0, 254 },
	{ "key_strafe",		&key_strafe,		KEY_RALT,	0, 254 },
	{ "key_speed",		&key_speed,		KEY_RSHIFT,	0, 254 },
	{ "key_jump",		&key_jump,		'/',		0, 254 },

	{ "use_mouse",		&usemouse,		1,	0, 1 },
	{ "mouseb_fire",	&mousebfire,		0,	-1, 2 },
	{ "mouseb_strafe",	&mousebstrafe,		1,	-1, 2 },
	{ "mouseb_forward",	&mousebforward,		2,	-1, 2 },
	{ "mouseb_jump",	&mousebjump,		-1,	-1, 2 },

	{ "use_joystick",	&usejoystick,		0,	0, 1 },
	{ "joyb_fire",		&joybfire,		0,	-1, 3 },
	{ "joyb_strafe",	&joybstrafe,		1,	-1, 3 },
	{ "joyb_use",		&joybuse,		3,	-1, 3 },
	{ "joyb_speed",		&joybspeed,		2,	-1, 3 },
	{ "joyb_jump",		&joybjump,		-1,	-1, 3 },

	{ "screenblocks",	&screenblocks,		10,	3, 11 },
	{ "usegamma",		&usegamma,		0,	0, 4 },
	{ "messageson",		&messageson,		1,	0, 1 },
	{ "mouselook",		&mouselook,		1,	0, 2 },
	{ "cdaudio",		&cdaudio,		0,	0, 1 },
	{ "alwaysrun",		&alwaysrun,		0,	0, 1 },

	{ "snd_channels",	&snd_Channels,		3,	3, MAX_CHANNELS },
#ifdef __WATCOMC__
	/* the min/max values I added here are pretty much meaningless.
	the values used to be set by the DOS version's setup program. */
	{ "snd_musicdevice",	&snd_DesiredMusicDevice,0,	0, NUM_SCARDS-1 },
	{ "snd_sfxdevice",	&snd_DesiredSfxDevice,	0,	0, NUM_SCARDS-1 },
	{ "snd_sbport",		&snd_SBport,		544,	0, 544 },
	{ "snd_sbirq",		&snd_SBirq,		-1,	-1, 7 },
	{ "snd_sbdma",		&snd_SBdma,		-1,	-1, 7 },
	{ "snd_mport",		&snd_Mport,		-1,	-1, 360 }
#endif	/* DOS vars */
};

default_str_t default_strings[] =
{
	{ "chatmacro0", chat_macros[0] },
	{ "chatmacro1", chat_macros[1] },
	{ "chatmacro2", chat_macros[2] },
	{ "chatmacro3", chat_macros[3] },
	{ "chatmacro4", chat_macros[4] },
	{ "chatmacro5", chat_macros[5] },
	{ "chatmacro6", chat_macros[6] },
	{ "chatmacro7", chat_macros[7] },
	{ "chatmacro8", chat_macros[8] },
	{ "chatmacro9", chat_macros[9] }
};

static int numdefaults, numstrings;
static char defaultfile[MAX_OSPATH];

/*
==============
=
= M_SaveDefaults
=
==============
*/

void M_SaveDefaults (void)
{
	int	i,v;
	FILE	*f;

	f = fopen (defaultfile, "w");
	if (!f)
		return;	// can't write the file, but don't complain

	for (i = 0; i < numdefaults; i++)
	{
		v = *defaults[i].location;
		fprintf (f, "%s\t\t%i\n", defaults[i].name, v);
	}

	for (i = 0; i < numstrings; i++)
	{
		fprintf (f, "%s\t\t\"%s\"\n", default_strings[i].name,
					default_strings[i].location);
	}

	fclose (f);
}

//==========================================================================
//
// M_LoadDefaults
//
//==========================================================================

void M_LoadDefaults(const char *fileName)
{
	int i;
	int len;
	FILE *f;
	char def[80];
	char strparm[100];
	int parm;

	numdefaults = sizeof(defaults) / sizeof(defaults[0]);
	numstrings = sizeof(default_strings) / sizeof(default_strings[0]);
	ST_Message("Loading default settings\n");
	// Set everything to base values
	for (i = 0; i < numdefaults; i++)
	{
		*defaults[i].location = defaults[i].defaultvalue;
	}
	// Make a backup of all default strings
	for (i = 0; i < numstrings; i++)
	{
		default_strings[i].defaultvalue = (char *) calloc(1, 80);
		strcpy (default_strings[i].defaultvalue, default_strings[i].location);
	}

	// Check for a custom config file
	i = M_CheckParm("-config");
	if (i && i < myargc-1)
	{
		snprintf(defaultfile, sizeof(defaultfile), "%s%s", basePath, myargv[i + 1]);
		ST_Message("config file: %s\n", defaultfile);
	}
	else
	{
		snprintf(defaultfile, sizeof(defaultfile), "%s%s", basePath, fileName);
	}

	// Scan the config file
	f = fopen(defaultfile, "r");
	if (f)
	{
		while (!feof(f))
		{
			if (fscanf(f, "%79s %[^\n]\n", def, strparm) == 2)
			{
				if (strparm[0] == '"') /* string values */
				{
					for (i = 0; i < numstrings; i++)
					{
						if (!strcmp(def, default_strings[i].name))
						{
							len = (int)strlen(strparm) - 2;
							if (len <= 0)
							{
								default_strings[i].location[0] = '\0';
								break;
							}
							if (len > 79)
							{
								len = 79;
							}
							strncpy(default_strings[i].location, strparm + 1, len);
							default_strings[i].location[len] = '\0';
							break;
						}
					}
					continue;
				}

				/* numeric values */
				if (strparm[0] == '0' && strparm[1] == 'x')
				{
					sscanf(strparm + 2, "%x", &parm);
				}
				else
				{
					sscanf(strparm, "%i", &parm);
				}
				for (i = 0; i < numdefaults; i++)
				{
					if (!strcmp(def, defaults[i].name))
					{
						if (parm >= defaults[i].minvalue && parm <= defaults[i].maxvalue)
							*defaults[i].location = parm;
						break;
					}
				}
			}
		}
		fclose (f);
	}
}


/*
==============================================================================

						SCREEN SHOTS

==============================================================================
*/

typedef struct
{
	char	manufacturer;
	char	version;
	char	encoding;
	char	bits_per_pixel;
	unsigned short	xmin,ymin,xmax,ymax;
	unsigned short	hres,vres;
	unsigned char	palette[48];
	char	reserved;
	char	color_planes;
	unsigned short	bytes_per_line;
	unsigned short	palette_type;
	char	filler[58];
	unsigned char	data;	// unbounded
} pcx_t;

/*
==============
=
= WritePCXfile
=
==============
*/

void WritePCXfile (const char *filename, byte *data, int width, int height, byte *palette)
{
	int	i, length;
	pcx_t	*pcx;
	byte	*pack;

	pcx = (pcx_t *) Z_Malloc (width*height*2 + 1000, PU_STATIC, NULL);

	pcx->manufacturer = 0x0a;	// PCX id
	pcx->version = 5;		// 256 color
	pcx->encoding = 1;		// uncompressed
	pcx->bits_per_pixel = 8;	// 256 color
	pcx->xmin = 0;
	pcx->ymin = 0;
	pcx->xmax = SHORT(width - 1);
	pcx->ymax = SHORT(height - 1);
	pcx->hres = SHORT(width);
	pcx->vres = SHORT(height);
	memset (pcx->palette, 0, sizeof(pcx->palette));
	pcx->color_planes = 1;		// chunky image
	pcx->bytes_per_line = SHORT(width);
	pcx->palette_type = SHORT(2);	// not a grey scale
	memset (pcx->filler, 0, sizeof(pcx->filler));

//
// pack the image
//
	pack = &pcx->data;

	for (i = 0; i < width*height; i++)
	{
		if ((*data & 0xc0) != 0xc0)
			*pack++ = *data++;
		else
		{
			*pack++ = 0xc1;
			*pack++ = *data++;
		}
	}

//
// write the palette
//
	*pack++ = 0x0c;		// palette ID byte
	for (i = 0; i < 768; i++)
		*pack++ = *palette++;

//
// write output file
//
	length = pack - (byte *)pcx;
	M_WriteFile (filename, pcx, length);

	Z_Free (pcx);
}


//==============================================================================

/*
==================
=
= M_ScreenShot
=
==================
*/
#ifdef RENDER3D
void M_ScreenShot (void)
{
	OGL_GrabScreen();
}
#else
void M_ScreenShot (void)
{
	int	i;
	byte	*linear;
	char	lbmname[MAX_OSPATH], *p;
	byte	*pal;

//
// munge planar buffer to linear
// 
	linear = screen;
//
// find a file name to save it to
//
	snprintf (lbmname, sizeof(lbmname), "%shexen00.pcx", basePath);
	p = lbmname + strlen(basePath);
	for (i = 0; i <= 99; i++)
	{
		p[5] = i/10 + '0';
		p[6] = i%10 + '0';
		if (access(lbmname, F_OK) == -1)
			break;	// file doesn't exist
	}
	if (i == 100)
	{
		P_SetMessage(&players[consoleplayer], "SCREEN SHOT FAILED", false);
		return;
	}

//
// save the pcx file
//
	pal = (byte *)W_CacheLumpName("PLAYPAL", PU_CACHE);

	WritePCXfile (lbmname, linear, SCREENWIDTH, SCREENHEIGHT, pal);

	P_SetMessage(&players[consoleplayer], "SCREEN SHOT", false);
}
#endif

