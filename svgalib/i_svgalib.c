//**************************************************************************
//**
//** $Id: i_svgalib.c,v 1.9 2009-05-08 17:12:09 sezero Exp $
//**
//**************************************************************************


#include "h2stdinc.h"
#include <sys/time.h>
#include <unistd.h>
/* SVGALib includes */
#include <vga.h>
#include <vgakeyboard.h>
#include <vgamouse.h>

#include "h2def.h"
#include "r_local.h"
#include "st_start.h"

// Macros

// Extern Data

extern int usemouse, usejoystick;

// Private Data

static int svgalib_backgrounded = 0;
static boolean vid_initialized = false;
static boolean kbd_initialized = false;

static byte scantokey[128] =
{
//	 0		 1		 2		 3		 4		 5		 6		 7
//	 8		 9		 A		 B		 C		 D		 E		 F
	0  ,		27,		'1',		'2',		'3',		'4',		'5',		'6',
	'7',		'8',		'9',		'0',		'-',		'=',		KEY_BACKSPACE,	 9,	// 0
	'q',		'w',		'e',		'r',		't',		'y',		'u',		'i',
	'o',		'p',		'[',		']',		 13,		KEY_CTRL,	'a',		's',	// 1
	'd',		'f',		'g',		'h',		'j',		'k',		'l',		';',
	'\'',		'`',		KEY_SHIFT,	'\\',		'z',		'x',		'c',		'v',	// 2
	'b',		'n',		'm',		',',		'.',		'/',		KEY_SHIFT,	'*',
	KEY_ALT,	' ',		 0 ,		KEY_F1,		KEY_F2,		KEY_F3,		KEY_F4,		KEY_F5,	// 3
	KEY_F6,		KEY_F7,		KEY_F8,		KEY_F9,		KEY_F10,	 0 ,		 0 ,		KEY_HOME,
	KEY_UPARROW,	KEY_PGUP,	'-',		KEY_LEFTARROW,	'5',		KEY_RIGHTARROW,	'+',		KEY_END,// 4
	KEY_DOWNARROW,	KEY_PGDN,	KEY_INS,	KEY_DEL,	 0 ,		 0 ,		 0 ,		KEY_F11,
	KEY_F12,	 0 ,		 0 ,		 0 ,		 0 ,		 0 ,		 0 ,		 0 ,	// 5
	0  ,		 0 ,		 0 ,		 0 ,		 0 ,		 0 ,		 0 ,		 0 ,
	0  ,		 0 ,		 0 ,		 0 ,		 0 ,		 0 ,		 0 ,		 0 ,	// 6
	0  ,		 0 ,		 0 ,		 0 ,		 0 ,		 0 ,		 0 ,		 0 ,
	0  ,		 0 ,		 0 ,		 0 ,		 0 ,		 0 ,		 0 ,		 0	// 7
};

// Public Data

int DisplayTicker = 0;

boolean useexterndriver;
boolean mousepresent;


/*
============================================================================

								USER INPUT

============================================================================
*/

//--------------------------------------------------------------------------
//
// PROC I_WaitVBL
//
//--------------------------------------------------------------------------

void I_WaitVBL(int vbls)
{
	if (!vid_initialized)
	{
		return;
	}
	while (vbls--)
	{
		usleep (16667);
	}
}

//--------------------------------------------------------------------------
//
// PROC I_SetPalette
//
// Palette source must use 8 bit RGB elements.
//
//--------------------------------------------------------------------------

void I_SetPalette(byte *palette)
{
	register int *buf = NULL;
	register short count = 256 / 2;
	register int *p;
	register unsigned char *gtable = gammatable[usegamma];

	if (!vid_initialized)
		return;
	if (svgalib_backgrounded)
		return;

	I_WaitVBL(1);

	// Code lynched from Collin Phipps' lsdoom
	p = buf = (int *) Z_Malloc(256 * 3 * sizeof(int), PU_STATIC, NULL);
	do
	{
		// One RGB Triple
		p[0] = gtable[palette[0]] >> 2;
		p[1] = gtable[palette[1]] >> 2;
		p[2] = gtable[palette[2]] >> 2;

		// And another
		p[3] = gtable[palette[3]] >> 2;
		p[4] = gtable[palette[4]] >> 2;
		p[5] = gtable[palette[5]] >> 2;

		p += 6;
		palette += 6;
	} while (--count);

	if (vga_oktowrite())
		vga_setpalvec(0, 256, buf);

	Z_Free(buf);
}

/*
============================================================================

							GRAPHICS MODE

============================================================================
*/

byte *pcscreen, *destscreen, *destview;

/*
==============
=
= I_Update
=
==============
*/

int UpdateState;
extern int screenblocks;

void I_Update (void)
{
	int i;
	byte *dest;
	int tics;
	static int lasttic;

	if (!vid_initialized)
		return;
	if (svgalib_backgrounded)
		return;
	if (!vga_oktowrite())
		return;	/* can't update screen if it's not active */

//
// blit screen to video
//
	if (DisplayTicker) // Displays little dots in corner
	{
		if (screenblocks > 9 || UpdateState & (I_FULLSCRN|I_MESSAGES))
		{
			dest = (byte *)screen;
		}
		else
		{
			dest = (byte *)pcscreen;
		}
		tics = ticcount - lasttic;
		lasttic = ticcount;
		if (tics > 20)
			tics = 20;
		for (i = 0; i < tics; i++)
		{
			*dest = 0xff;
			dest += 2;
		}
		for (i = tics; i < 20; i++)
		{
			*dest = 0x00;
			dest += 2;
		}
	}

//	memset(pcscreen, 255, SCREENHEIGHT*SCREENWIDTH);

	if (UpdateState == I_NOUPDATE)
	{
		return;
	}
	if (UpdateState & I_FULLSCRN)
	{
		memcpy(pcscreen, screen, SCREENWIDTH*SCREENHEIGHT);
		UpdateState = I_NOUPDATE; // clear out all draw types
	}
	if (UpdateState & I_FULLVIEW)
	{
		if (UpdateState & I_MESSAGES && screenblocks > 7)
		{
			for (i = 0; i < (viewwindowy+viewheight)*SCREENWIDTH; i += SCREENWIDTH)
			{
				memcpy(pcscreen + i, screen + i, SCREENWIDTH);
			}

			UpdateState &= ~(I_FULLVIEW|I_MESSAGES);
		}
		else
		{
			for (i = viewwindowy*SCREENWIDTH + viewwindowx;
			     i < (viewwindowy+viewheight)*SCREENWIDTH; i += SCREENWIDTH)
			{
				memcpy(pcscreen + i, screen + i, viewwidth);
			}
			UpdateState &= ~I_FULLVIEW;
		}
	}
	if (UpdateState & I_STATBAR)
	{
		memcpy(pcscreen + SCREENWIDTH*(SCREENHEIGHT-SBARHEIGHT),
			screen + SCREENWIDTH*(SCREENHEIGHT-SBARHEIGHT),
			SCREENWIDTH*SBARHEIGHT);
		UpdateState &= ~I_STATBAR;
	}
	if (UpdateState & I_MESSAGES)
	{
		memcpy(pcscreen, screen, SCREENWIDTH*28);
		UpdateState &= ~I_MESSAGES;
	}
}

//-----------------------------------------------------------------------------
//
// PROC I_KeyboardHandler()
//
//-----------------------------------------------------------------------------

static void I_KeyboardHandler(int scancode, int press)
{
	event_t ev;

	ev.type = (press == KEY_EVENTPRESS) ? ev_keydown : ev_keyup;
	switch (scancode)
	{
	case SCANCODE_CURSORBLOCKUP:
	case SCANCODE_CURSORUP:		ev.data1 = KEY_UPARROW;		break;

	case SCANCODE_CURSORBLOCKDOWN:
	case SCANCODE_CURSORDOWN:	ev.data1 = KEY_DOWNARROW;	break;

	case SCANCODE_CURSORBLOCKLEFT:
	case SCANCODE_CURSORLEFT:	ev.data1 = KEY_LEFTARROW;	break;

	case SCANCODE_CURSORBLOCKRIGHT:
	case SCANCODE_CURSORRIGHT:	ev.data1 = KEY_RIGHTARROW;	break;

	case SCANCODE_ESCAPE:		ev.data1 = KEY_ESCAPE;		break;

	case SCANCODE_ENTER:
	case SCANCODE_KEYPADENTER:	ev.data1 = KEY_ENTER;		break;
	case SCANCODE_F1:		ev.data1 = KEY_F1;		break;
	case SCANCODE_F2:		ev.data1 = KEY_F2;		break;
	case SCANCODE_F3:		ev.data1 = KEY_F3;		break;
	case SCANCODE_F4:		ev.data1 = KEY_F4;		break;
	case SCANCODE_F5:		ev.data1 = KEY_F5;		break;
	case SCANCODE_F6:		ev.data1 = KEY_F6;		break;
	case SCANCODE_F7:		ev.data1 = KEY_F7;		break;
	case SCANCODE_F8:		ev.data1 = KEY_F8;		break;
	case SCANCODE_F9:		ev.data1 = KEY_F9;		break;
	case SCANCODE_F10:		ev.data1 = KEY_F10;		break;
	case SCANCODE_F11:		ev.data1 = KEY_F11;		break;
	case SCANCODE_F12:		ev.data1 = KEY_F12;		break;
	case SCANCODE_INSERT:		ev.data1 = KEY_INS;		break;
	case SCANCODE_REMOVE:		ev.data1 = KEY_DEL;		break;
	case SCANCODE_PAGEUP:		ev.data1 = KEY_PGUP;		break;
	case SCANCODE_PAGEDOWN:		ev.data1 = KEY_PGDN;		break;
	case SCANCODE_HOME:		ev.data1 = KEY_HOME;		break;
	case SCANCODE_END:		ev.data1 = KEY_END;		break;
	case SCANCODE_BACKSPACE:	ev.data1 = KEY_BACKSPACE;	break;

	case SCANCODE_BREAK:
	case SCANCODE_BREAK_ALTERNATIVE: ev.data1 = KEY_PAUSE;		break;

	case SCANCODE_EQUAL:		ev.data1 = KEY_EQUALS;		break;

	case SCANCODE_MINUS:
	case SCANCODE_KEYPADMINUS:	ev.data1 = KEY_MINUS;		break;

	case SCANCODE_LEFTSHIFT:
	case SCANCODE_RIGHTSHIFT:	ev.data1 = KEY_RSHIFT;		break;

	case SCANCODE_LEFTCONTROL:
	case SCANCODE_RIGHTCONTROL:	ev.data1 = KEY_RCTRL;		break;

	case SCANCODE_LEFTALT:
	case SCANCODE_RIGHTALT:		ev.data1 = KEY_RALT;		break;

	default:			ev.data1 = scantokey[scancode];	break;
	}

	H2_PostEvent(&ev);
}

//-----------------------------------------------------------------------------
//
// PROC I_MouseEventHandler()
//
//-----------------------------------------------------------------------------

static void I_MouseEventHandler(int button, int dx, int dy, int dz,
				int rdx, int rdy, int rdz)
{
	event_t ev;

	ev.type = ev_mouse;
	ev.data1 = ((button & MOUSE_LEFTBUTTON) ? 1 : 0)   |
		   ((button & MOUSE_MIDDLEBUTTON) ? 2 : 0) |
		   ((button & MOUSE_LEFTBUTTON) ? 4 : 0);
	ev.data2 = dx << 2;
	ev.data3 = -(dy<<2);

	H2_PostEvent(&ev);
}

//--------------------------------------------------------------------------
//
// PROC I_ShutdownGraphics
//
//--------------------------------------------------------------------------

void I_ShutdownGraphics(void)
{
	static boolean in_shutdown = false;

	if (in_shutdown)
		return;
	in_shutdown = true;

	if (mousepresent)
		vga_setmousesupport(0);
	mousepresent = false;

	if (kbd_initialized)
		keyboard_close();
	kbd_initialized = false;

	if (vid_initialized)
		vga_setmode(TEXT);
	vid_initialized = false;

	system("stty sane");
}

//--------------------------------------------------------------------------
//
// PROC I_InitGraphics
//
//--------------------------------------------------------------------------

/* backgrounding fixes (quakeforge) */
static void goto_background (void)
{
	svgalib_backgrounded = 1;
}

static void comefrom_background (void)
{
	svgalib_backgrounded = 0;
}

void I_InitGraphics(void)
{
	int		i;

	if (M_CheckParm("-novideo"))	// if true, stay in text mode for debugging
	{
		ST_Message("I_InitGraphics: Video Disabled.\n");
		return;
	}

	ST_Message("I_InitGraphics:\n");

	if (vga_init() != 0)
		I_Error ("SVGALib failed to allocate a new VC");
	atexit(I_ShutdownGraphics);

	/* backgrounding stuff (from quakeforge) */
	i = vga_runinbackground_version();
	if (i > 0)
	{
		ST_Message ("SVGALIB background support %i detected\n", i);
		vga_runinbackground (VGA_GOTOBACK, goto_background);
		vga_runinbackground (VGA_COMEFROMBACK, comefrom_background);
		vga_runinbackground (1);
	}
	else
	{
		vga_runinbackground (0);
	}

	if (M_CheckParm("-nomouse"))
		vga_setmousesupport(0);
	else
		vga_setmousesupport(1);

	if (!vga_hasmode(G320x200x256))
		I_Error("SVGALib reports no mode 13h");
	if (vga_setmode(G320x200x256))
		I_Error("Can't set video mode 13h");

	pcscreen = destscreen = (byte *) vga_getgraphmem();
	if (!pcscreen)
		I_Error("SVGALib vga_getgraphmem failed.");
	vid_initialized = true;

	if (keyboard_init())
		I_Error("keyboard_init() failed");
	keyboard_seteventhandler(I_KeyboardHandler);
	kbd_initialized = true;

	I_SetPalette((byte *)W_CacheLumpName("PLAYPAL", PU_CACHE));

	if (!M_CheckParm("-nomouse"))
	{
	/* FIXME: WHAT IF SVGALIB FAILED INITING THE MOUSE ??? */
		mouse_seteventhandler((void *) I_MouseEventHandler);
		mousepresent = true;
	}
}

void I_StartupMouse (void)
{
	/* done under I_InitGraphics() */
}

void I_StartTic (void)
{
	if (kbd_initialized)
	{
		while (keyboard_update())
			;
	}

	if (mousepresent)
		mouse_update();
}

