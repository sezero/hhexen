//**************************************************************************
//**
//** $Id: i_sdl.c,v 1.4 2008-06-17 13:03:44 sezero Exp $
//**
//**************************************************************************


#include <stdlib.h>
#include <stdarg.h>
#include <sys/time.h>
#include "SDL.h"
#include "h2def.h"
#include "r_local.h"
#include "p_local.h"    // for P_AproxDistance
#include "sounds.h"
#include "i_sound.h"
#include "soundst.h"
#include "st_start.h"

// Public Data

int DisplayTicker = 0;


// Code

void I_StartupNet (void);
void I_ShutdownNet (void);
void I_ReadExternDriver(void);
void GrabScreen (void);

extern int usemouse, usejoystick;

extern void **lumpcache;

int i_Vector;
externdata_t *i_ExternData;
boolean useexterndriver;

SDL_Surface* sdl_screen;

boolean mousepresent;

//===============================

int ticcount;


boolean novideo; // if true, stay in text mode for debugging

#define KEY_INS         0x52
#define KEY_DEL         0x53
#define KEY_PGUP        0x49
#define KEY_PGDN        0x51
#define KEY_HOME        0x47
#define KEY_END         0x4f

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
        SDL_Delay( 16667/1000 );
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
    SDL_Color* c;
    SDL_Color* cend;
    SDL_Color cmap[ 256 ];

	if(novideo)
	{
		return;
	}
	I_WaitVBL(1);

    c = cmap;
    cend = c + 256;
	for( ; c != cend; c++ )
	{
		//_outbyte(PEL_DATA, (gammatable[usegamma][*palette++])>>2);

        c->r = gammatable[usegamma][*palette++];
        c->g = gammatable[usegamma][*palette++];
        c->b = gammatable[usegamma][*palette++];
	}
    SDL_SetColors( sdl_screen, cmap, 0, 256 );
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

//
// blit screen to video
//
	if(DisplayTicker)
	{
		if(screenblocks > 9 || UpdateState&(I_FULLSCRN|I_MESSAGES))
		{
			dest = (byte *)screen;
		}
		else
		{
			dest = (byte *)pcscreen;
		}
		tics = ticcount-lasttic;
		lasttic = ticcount;
		if(tics > 20)
		{
			tics = 20;
		}
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
		memcpy(pcscreen, screen, SCREENWIDTH*SCREENHEIGHT);
		UpdateState = I_NOUPDATE; // clear out all draw types

        SDL_UpdateRect( sdl_screen, 0, 0, SCREENWIDTH, SCREENHEIGHT );
	}
	if(UpdateState&I_FULLVIEW)
	{
		if(UpdateState&I_MESSAGES && screenblocks > 7)
		{
			for(i = 0; i <
				(viewwindowy+viewheight)*SCREENWIDTH; i += SCREENWIDTH)
			{
				memcpy(pcscreen+i, screen+i, SCREENWIDTH);
			}
			UpdateState &= ~(I_FULLVIEW|I_MESSAGES);

            SDL_UpdateRect( sdl_screen, 0, 0, SCREENWIDTH,
                            viewwindowy+viewheight );
		}
		else
		{
			for(i = viewwindowy*SCREENWIDTH+viewwindowx; i <
				(viewwindowy+viewheight)*SCREENWIDTH; i += SCREENWIDTH)
			{
				memcpy(pcscreen+i, screen+i, viewwidth);
			}
			UpdateState &= ~I_FULLVIEW;

            SDL_UpdateRect( sdl_screen, viewwindowx, viewwindowy, viewwidth,
                            viewheight );
		}
	}
	if(UpdateState&I_STATBAR)
	{
		memcpy(pcscreen+SCREENWIDTH*(SCREENHEIGHT-SBARHEIGHT),
			screen+SCREENWIDTH*(SCREENHEIGHT-SBARHEIGHT),
			SCREENWIDTH*SBARHEIGHT);
		UpdateState &= ~I_STATBAR;

        SDL_UpdateRect( sdl_screen, 0, SCREENHEIGHT-SBARHEIGHT,
                        SCREENWIDTH, SBARHEIGHT );
	}
	if(UpdateState&I_MESSAGES)
	{
		memcpy(pcscreen, screen, SCREENWIDTH*28);
		UpdateState &= ~I_MESSAGES;

        SDL_UpdateRect( sdl_screen, 0, 0, SCREENWIDTH, 28 );
	}
}

//--------------------------------------------------------------------------
//
// PROC I_InitGraphics
//
//--------------------------------------------------------------------------

void I_InitGraphics(void)
{
	char text[20];

	if( novideo )
	{
		return;
	}
    
    // SDL_DOUBLEBUF does not work in full screen mode.  Does not seem to
    // be necessary anyway.

    sdl_screen = SDL_SetVideoMode(SCREENWIDTH, SCREENHEIGHT, 8, SDL_SWSURFACE);

    if( sdl_screen == NULL )
    {
        fprintf( stderr, "Couldn't set video mode %dx%d: %s\n",
                 SCREENWIDTH, SCREENHEIGHT, SDL_GetError() );
        exit( 3 );
    }

    // Only grab if we want to
    if (!M_CheckParm ("--nograb") && !M_CheckParm ("-g")) {
	    SDL_WM_GrabInput (SDL_GRAB_ON);
    }

    SDL_ShowCursor( 0 );
    snprintf (text, 20, "HHexen v%s", HHEXEN_VERSION);
    SDL_WM_SetCaption( text, "HHEXEN" );


	pcscreen = destscreen = sdl_screen->pixels;

	I_SetPalette( W_CacheLumpName("PLAYPAL", PU_CACHE) );
}

//--------------------------------------------------------------------------
//
// PROC I_ShutdownGraphics
//
//--------------------------------------------------------------------------

void I_ShutdownGraphics(void)
{
	SDL_Quit ();
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


//
//  Translates the key 
//

int xlatekey(SDL_keysym *key)
{

    int rc;

    switch(key->sym)
    {
      case SDLK_LEFT:	rc = KEY_LEFTARROW;	break;
      case SDLK_RIGHT:	rc = KEY_RIGHTARROW;	break;
      case SDLK_DOWN:	rc = KEY_DOWNARROW;	break;
      case SDLK_UP:	rc = KEY_UPARROW;	break;
      case SDLK_ESCAPE:	rc = KEY_ESCAPE;	break;
      case SDLK_RETURN:	rc = KEY_ENTER;		break;
      case SDLK_F1:	rc = KEY_F1;		break;
      case SDLK_F2:	rc = KEY_F2;		break;
      case SDLK_F3:	rc = KEY_F3;		break;
      case SDLK_F4:	rc = KEY_F4;		break;
      case SDLK_F5:	rc = KEY_F5;		break;
      case SDLK_F6:	rc = KEY_F6;		break;
      case SDLK_F7:	rc = KEY_F7;		break;
      case SDLK_F8:	rc = KEY_F8;		break;
      case SDLK_F9:	rc = KEY_F9;		break;
      case SDLK_F10:	rc = KEY_F10;		break;
      case SDLK_F11:	rc = KEY_F11;		break;
      case SDLK_F12:	rc = KEY_F12;		break;
	
      case SDLK_INSERT: rc = KEY_INS;           break;
      case SDLK_DELETE: rc = KEY_DEL;           break;
      case SDLK_PAGEUP: rc = KEY_PGUP;          break;
      case SDLK_PAGEDOWN: rc = KEY_PGDN;        break;
      case SDLK_HOME:   rc = KEY_HOME;          break;
      case SDLK_END:    rc = KEY_END;           break;

      case SDLK_BACKSPACE: rc = KEY_BACKSPACE;	break;

      case SDLK_PAUSE:	rc = KEY_PAUSE;		break;

      case SDLK_EQUALS:	rc = KEY_EQUALS;	break;

      case SDLK_KP_MINUS:
      case SDLK_MINUS:	rc = KEY_MINUS;		break;

      case SDLK_LSHIFT:
      case SDLK_RSHIFT:
	rc = KEY_RSHIFT;
	break;
	
      case SDLK_LCTRL:
      case SDLK_RCTRL:
	rc = KEY_RCTRL;
	break;
	
      case SDLK_LALT:
      case SDLK_LMETA:
      case SDLK_RALT:
      case SDLK_RMETA:
	rc = KEY_RALT;
	break;
	
      default:
        rc = key->sym;
	break;
    }

    return rc;

}


/* This processes SDL events */
void I_GetEvent(SDL_Event *Event)
{
    Uint8 buttonstate;
    event_t event;
    SDLMod mod;

    switch (Event->type)
    {
      case SDL_KEYDOWN:
	mod = SDL_GetModState ();
	if (mod & KMOD_RCTRL || mod & KMOD_LCTRL) {
		if (Event->key.keysym.sym == 'g') {
			if (SDL_WM_GrabInput (SDL_GRAB_QUERY) == SDL_GRAB_OFF)
				SDL_WM_GrabInput (SDL_GRAB_ON);
			else
				SDL_WM_GrabInput (SDL_GRAB_OFF);
			break;
		}
	} else if (mod & KMOD_RALT || mod & KMOD_LALT) {
		if (Event->key.keysym.sym == SDLK_RETURN) {
			SDL_WM_ToggleFullScreen(SDL_GetVideoSurface());
			break;
		}
	}
	event.type = ev_keydown;
	event.data1 = xlatekey(&Event->key.keysym);
	H2_PostEvent(&event);
	break;

      case SDL_KEYUP:
        event.type = ev_keyup;
        event.data1 = xlatekey(&Event->key.keysym);
        H2_PostEvent(&event);
        break;

      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP:
        buttonstate = SDL_GetMouseState(NULL, NULL);
        event.type = ev_mouse;
        event.data1 = 0
            | (buttonstate & SDL_BUTTON(1) ? 1 : 0)
            | (buttonstate & SDL_BUTTON(2) ? 2 : 0)
            | (buttonstate & SDL_BUTTON(3) ? 4 : 0);
        event.data2 = event.data3 = 0;
        H2_PostEvent(&event);
        break;

      case SDL_MOUSEMOTION:
        /* Ignore mouse warp events */
        if ( (Event->motion.x != sdl_screen->w/2) ||
             (Event->motion.y != sdl_screen->h/2) )
        {
            /* Warp the mouse back to the center */
            event.type = ev_mouse;
            event.data1 = 0
                | (Event->motion.state & SDL_BUTTON(1) ? 1 : 0)
                | (Event->motion.state & SDL_BUTTON(2) ? 2 : 0)
                | (Event->motion.state & SDL_BUTTON(3) ? 4 : 0);
            event.data2 = Event->motion.xrel << 3;
            event.data3 = -Event->motion.yrel << 3;
            H2_PostEvent(&event);
        }
        break;

      case SDL_QUIT:
        I_Quit();
    }

}

//
// I_StartTic
//
void I_StartTic (void)
{
    SDL_Event Event;

    while ( SDL_PollEvent(&Event) )
        I_GetEvent(&Event);
}


/*
============================================================================

					TIMER INTERRUPT

============================================================================
*/


/*
================
=
= I_TimerISR
=
================
*/

int I_TimerISR (void)
{
	ticcount++;
	return 0;
}

/*
============================================================================

						KEYBOARD

============================================================================
*/

int lastpress;



/*
============================================================================

							MOUSE

============================================================================
*/


/*
================
=
= StartupMouse
=
================
*/


void I_StartupMouse (void)
{
	mousepresent = 1;
}

void GrabScreen (void)
{
}
