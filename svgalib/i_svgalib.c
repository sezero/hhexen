//**************************************************************************
//**
//** $Id: i_svgalib.c,v 1.1.1.1 2000-04-11 17:38:19 theoddone33 Exp $
//**
//**************************************************************************


#include <stdlib.h>
#include <stdarg.h>
#include <sys/time.h>

#include <vga.h>           // SVGALib includes
#include <vgakeyboard.h>
#include <vgamouse.h>

#include <unistd.h>    /* jim - usleep () */

#include <unistd.h>    /* jim - usleep () */

#include "h2def.h"
#include "r_local.h"
#include "p_local.h"    // for P_AproxDistance
#include "sounds.h"
#include "i_sound.h"
#include "soundst.h"
#include "st_start.h"

static enum {F_nomouse, F_mouse} mflag = F_nomouse;

// Macros

#define SEQ_ADDR 0x3C4
#define SEQ_DATA 0x3C5
#define REG_MAPMASK 0x02

#define MASK_PLANE0 0x01
#define MASK_PLANE1 0x02
#define MASK_PLANE2 0x04
#define MASK_PLANE3 0x08

#define P0OFFSET 38400*0
#define P1OFFSET 38400*1
#define P2OFFSET 38400*2
#define P3OFFSET 38400*3

#define VID_INT 0x10
#define VB_SYNC
#define BITPLANE(p)

#define KEY_LSHIFT      0xfe

#define KEY_INS         (0x80+0x52)
#define KEY_DEL         (0x80+0x53)
#define KEY_PGUP        (0x80+0x49)
#define KEY_PGDN        (0x80+0x51)
#define KEY_HOME        (0x80+0x47)
#define KEY_END         (0x80+0x4f)

// Public Data

int DisplayTicker = 0;


void I_ReadMouse (void);

extern  int usemouse;


//==================================================

#define VBLCOUNTER              34000           // hardware tics to a frame

#define MOUSEB1 1
#define MOUSEB2 2
#define MOUSEB3 4

boolean mousepresent;

//===============================

extern int ticcount;

extern boolean novideo; // if true, stay in text mode for debugging

#define KBDQUESIZE 32
byte keyboardque[KBDQUESIZE];
int kbdtail, kbdhead;


#define SC_RSHIFT       0x36
#define SC_LSHIFT       0x2a

byte        scantokey[128] =
					{
//  0           1       2       3       4       5       6       7
//  8           9       A       B       C       D       E       F
	0  ,    27,     '1',    '2',    '3',    '4',    '5',    '6',
	'7',    '8',    '9',    '0',    '-',    '=',    KEY_BACKSPACE, 9, // 0
	'q',    'w',    'e',    'r',    't',    'y',    'u',    'i',
	'o',    'p',    '[',    ']',    13 ,    KEY_RCTRL,'a',  's',      // 1
	'd',    'f',    'g',    'h',    'j',    'k',    'l',    ';',
	39 ,    '`',    KEY_LSHIFT,92,  'z',    'x',    'c',    'v',      // 2
	'b',    'n',    'm',    ',',    '.',    '/',    KEY_RSHIFT,'*',
	KEY_RALT,' ',   0  ,    KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5,   // 3
	KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10,0  ,    0  , KEY_HOME,
	KEY_UPARROW,KEY_PGUP,'-',KEY_LEFTARROW,'5',KEY_RIGHTARROW,'+',KEY_END, //4
	KEY_DOWNARROW,KEY_PGDN,KEY_INS,KEY_DEL,0,0,             0,              KEY_F11,
	KEY_F12,0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,        // 5
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,        // 6
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0         // 7
					};

//==========================================================================


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
	if( novideo )
	{
		return;
	}
	while( vbls-- )
	{
        usleep( 16667 );
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
   register int* buf = NULL;
   register short count = 256 / 2;
   register int* p;
   register unsigned char* gtable = gammatable[usegamma];

   if(novideo)
	{
		return;
	}
	I_WaitVBL(1);
   
   //Code lynched from Collin Phipps' lsdoom
   p = buf = Z_Malloc(256 * 3 * sizeof(int),PU_STATIC,NULL);
   do {  
       //One RGB Triple
      p[0]=gtable[palette[0]] >> 2;
      p[1]=gtable[palette[1]] >> 2;
      p[2]=gtable[palette[2]] >> 2;
      //And another
      p[3]=gtable[palette[3]] >> 2;
      p[4]=gtable[palette[4]] >> 2;
      p[5]=gtable[palette[5]] >> 2;
      
      p+=6; palette+=6;
   } while (--count);
   if(vga_oktowrite())
     vga_setpalvec(0,256,buf);
   
   Z_Free(buf);
}
/*
============================================================================

							GRAPHICS MODE

============================================================================
*/

//byte *pcscreen;
byte  *destscreen, *destview;

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

//
// blit screen to video
//
        if(DisplayTicker) //Displays little dots in corner
	{
	   if(screenblocks > 9 || UpdateState&(I_FULLSCRN|I_MESSAGES))
	     {	
	     dest = (byte *)screen;
	     }
	   else
	   {
	      dest = (byte*)graph_mem;
	   }
		tics = ticcount-lasttic;
		lasttic = ticcount;
		if(tics > 20) tics = 20;
		for(i = 0; i < tics; i++)
		{
		   *dest = 0xff;
		   dest += 2; 
		}
		for(i = tics; i < 20; i++)
		{
		   *dest = 0x00;
		   dest += 2;
	        }
	}

	//memset(pcscreen, 255, SCREENHEIGHT*SCREENWIDTH);

	if(UpdateState == I_NOUPDATE)
	{
		return;
	}
	if(UpdateState&I_FULLSCRN)
	{
	   memcpy(graph_mem, screen, SCREENWIDTH*SCREENHEIGHT);
	   UpdateState = I_NOUPDATE; // clear out all draw types

	}
	if(UpdateState&I_FULLVIEW)
	{
		if(UpdateState&I_MESSAGES && screenblocks > 7)
		{
		   for(i = 0; i < (viewwindowy+viewheight)*SCREENWIDTH; 
		       i += SCREENWIDTH)
			{	
			   memcpy(graph_mem+i, screen+i, SCREENWIDTH);
			} 
		   
			UpdateState &= ~(I_FULLVIEW|I_MESSAGES);
		}
		else
		{
		   for(i = viewwindowy*SCREENWIDTH+viewwindowx; i <
				(viewwindowy+viewheight)*SCREENWIDTH; i += SCREENWIDTH)
			{
				memcpy(graph_mem+i, screen+i, viewwidth);
			}
		   UpdateState &= ~I_FULLVIEW;
		}
	}
	if(UpdateState&I_STATBAR)
	{
	        memcpy(graph_mem+SCREENWIDTH*(SCREENHEIGHT-SBARHEIGHT),
			screen+SCREENWIDTH*(SCREENHEIGHT-SBARHEIGHT),
			SCREENWIDTH*SBARHEIGHT);
		UpdateState &= ~I_STATBAR;
	}
	if(UpdateState&I_MESSAGES)
	{
		memcpy(graph_mem, screen, SCREENWIDTH*28);
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
   switch (scancode) {
    case SCANCODE_CURSORBLOCKUP:
    case SCANCODE_CURSORUP: ev.data1 = KEY_UPARROW; break;
    case SCANCODE_CURSORBLOCKDOWN:
    case SCANCODE_CURSORDOWN: ev.data1 = KEY_DOWNARROW; break;
    case SCANCODE_CURSORBLOCKLEFT:
    case SCANCODE_CURSORLEFT: ev.data1 = KEY_LEFTARROW; break;
    case SCANCODE_CURSORBLOCKRIGHT:
    case SCANCODE_CURSORRIGHT: ev.data1 = KEY_RIGHTARROW; break;
    case SCANCODE_ESCAPE: ev.data1 = KEY_ESCAPE; break;
    case SCANCODE_ENTER:
    case SCANCODE_KEYPADENTER: ev.data1 = KEY_ENTER; break;
    case SCANCODE_F1: ev.data1 = KEY_F1; break;
    case SCANCODE_F2: ev.data1 = KEY_F2; break;
    case SCANCODE_F3: ev.data1 = KEY_F3; break;
    case SCANCODE_F4: ev.data1 = KEY_F4; break;
    case SCANCODE_F5: ev.data1 = KEY_F5; break;
    case SCANCODE_F6: ev.data1 = KEY_F6; break;
    case SCANCODE_F7: ev.data1 = KEY_F7; break;
    case SCANCODE_F8: ev.data1 = KEY_F8; break;
    case SCANCODE_F9: ev.data1 = KEY_F9; break;
    case SCANCODE_F10: ev.data1 = KEY_F10; break;
    case SCANCODE_F11: ev.data1 = KEY_F11; break;
    case SCANCODE_F12: ev.data1 = KEY_F12; break;
    case SCANCODE_INSERT: ev.data1 = KEY_INS; break;
    case SCANCODE_REMOVE: ev.data1 = KEY_DEL; break;
    case SCANCODE_PAGEUP: ev.data1 = KEY_PGUP; break;
    case SCANCODE_PAGEDOWN: ev.data1 = KEY_PGDN; break;
    case SCANCODE_HOME: ev.data1 = KEY_HOME; break;
    case SCANCODE_END: ev.data1 = KEY_END; break;
    case SCANCODE_BACKSPACE: ev.data1 = KEY_BACKSPACE; break;
    case SCANCODE_BREAK:
    case SCANCODE_BREAK_ALTERNATIVE: ev.data1 = KEY_PAUSE; break;
    case SCANCODE_EQUAL: ev.data1 = KEY_EQUALS; break;
    case SCANCODE_MINUS:
    case SCANCODE_KEYPADMINUS: ev.data1 = KEY_MINUS; break;
    case SCANCODE_LEFTSHIFT:
    case SCANCODE_RIGHTSHIFT: ev.data1 = KEY_RSHIFT; break;
    case SCANCODE_LEFTCONTROL:
    case SCANCODE_RIGHTCONTROL: ev.data1 = KEY_RCTRL; break;
    case SCANCODE_LEFTALT:
    case SCANCODE_RIGHTALT: ev.data1 = KEY_RALT; break;
    default: ev.data1 = scantokey[scancode]; break;
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
   ev.data1 = ((button & MOUSE_LEFTBUTTON) ? 1 : 0) |
              ((button & MOUSE_MIDDLEBUTTON) ? 2 : 0) |
              ((button & MOUSE_LEFTBUTTON) ? 4 : 0);
   ev.data2 = dx << 2; ev.data3 = -(dy<<2);
   
   H2_PostEvent(&ev);
}

//--------------------------------------------------------------------------
//
// PROC I_ShutdownGraphics
//
//--------------------------------------------------------------------------

void I_ShutdownGraphics(void)
{
   if(mflag != F_nomouse) vga_setmousesupport(0);
   keyboard_close();
   vga_setmode(TEXT);
   system("stty sane");
}

//--------------------------------------------------------------------------
//
// PROC I_InitGraphics
//
//--------------------------------------------------------------------------

void I_InitGraphics(void)
{
   ST_Message("I_InitGraphics: ");
   vga_init();
   
   atexit(I_ShutdownGraphics);
   
   keyboard_init();
   keyboard_seteventhandler(I_KeyboardHandler);
   
   if(!M_CheckParm("-nomouse")) {
      vga_setmousesupport(1);
      mflag = F_mouse;
   }
   
   if(!vga_hasmode(G320x200x256))
      I_Error("SVGALib reports no mode 13h");
   putc('\n',stderr); 
   
   if(vga_setmode(G320x200x256))
     I_Error("Can't set video mode 13h");
   
   I_SetPalette( W_CacheLumpName("PLAYPAL", PU_CACHE) );

   if(mflag == F_mouse) mouse_seteventhandler(I_MouseEventHandler);
}


//--------------------------------------------------------------------------
//
// PROC I_ReadScreen
//
// Reads the screen currently displayed into a linear buffer.
//
//--------------------------------------------------------------------------

/*
void I_ReadScreen(byte *scr)
{
	memcpy(scr, screen, SCREENWIDTH*SCREENHEIGHT);
}
*/

//===========================================================================

void   I_StartTic (void)
{
   if(mflag != F_nomouse) mouse_update();
   keyboard_update();
}


/*
void   I_ReadKeys (void)
{
	int             k;


	while (1)
	{
	   while (kbdtail < kbdhead)
	   {
		   k = keyboardque[kbdtail&(KBDQUESIZE-1)];
		   kbdtail++;
		   printf ("0x%x\n",k);
		   if (k == 1)
			   I_Quit ();
	   }
	}
}
*/


/*
============================================================================

						KEYBOARD

============================================================================
*/

int lastpress;

/*
================
=
= I_KeyboardISR
=
================
*/

void I_KeyboardISR(void)
{
// Get the scan code

	keyboardque[kbdhead&(KBDQUESIZE-1)] = lastpress = 0;  //TODO
	kbdhead++;
}



/*
===============
=
= I_StartupKeyboard
=
===============
*/

void I_StartupKeyboard (void)
{
    //TODO
}


void I_ShutdownKeyboard (void)
{
    //TODO
}



/*
============================================================================

							MOUSE

============================================================================
*/


int I_ResetMouse (void)
{
    // Return 0xffff if mouse reset ok.
    return 0;
}



/*
================
=
= StartupMouse
=
================
*/

void I_StartupCyberMan(void);

void I_StartupMouse (void)
{
   //
   // General mouse detection
   //
	mousepresent = 0;
	if ( M_CheckParm ("-nomouse") || !usemouse )
		return;

	if (I_ResetMouse () != 0xffff)
	{
		ST_Message("Mouse: not present\n");
		return;
	}
	ST_Message("Mouse: detected\n");

	mousepresent = 1;

	I_StartupCyberMan();
}


/*
================
=
= ShutdownMouse
=
================
*/

void I_ShutdownMouse (void)
{
	if (!mousepresent)
	  return;

	I_ResetMouse ();
}


/*
================
=
= I_ReadMouse
=
================
*/

void I_ReadMouse (void)
{
#if 0
	event_t ev;

//
// mouse events
//
	if (!mousepresent)
		return;

	ev.type = ev_mouse;

	memset (&dpmiregs,0,sizeof(dpmiregs));
	dpmiregs.eax = 3;                               // read buttons / position
	DPMIInt (0x33);
	ev.data1 = dpmiregs.ebx;

	dpmiregs.eax = 11;                              // read counters
	DPMIInt (0x33);
	ev.data2 = (short)dpmiregs.ecx;
	ev.data3 = -(short)dpmiregs.edx;

	H2_PostEvent (&ev);
#endif
}


//==========================================================================
//
//
// I_StartupReadKeys
//
//
//==========================================================================

void I_StartupReadKeys(void)
{
	int k;

   while (kbdtail < kbdhead)
   {
	   k = keyboardque[kbdtail&(KBDQUESIZE-1)];
	   kbdtail++;
	   if (k == 1)
		   I_Quit ();
   }
}


//EOF
