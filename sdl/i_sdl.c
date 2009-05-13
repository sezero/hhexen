//**************************************************************************
//**
//** $Id: i_sdl.c,v 1.20 2009-05-13 08:34:14 sezero Exp $
//**
//**************************************************************************


#include "h2stdinc.h"
#include <sys/time.h>
#include "SDL.h"
#include "h2def.h"
#include "r_local.h"
#include "st_start.h"

#define BASE_WINDOW_FLAGS	(SDL_SWSURFACE|SDL_HWPALETTE)
#ifdef FULLSCREEN_DEFAULT
#define DEFAULT_FLAGS		(BASE_WINDOW_FLAGS|SDL_FULLSCREEN)
#else
#define DEFAULT_FLAGS		(BASE_WINDOW_FLAGS)
#endif

// Public Data

int DisplayTicker = 0;

boolean useexterndriver;
boolean mousepresent;


// Extern Data

extern int usemouse, usejoystick;

// Private Data

static boolean vid_initialized = false;

static SDL_Surface* sdl_screen;


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
		SDL_Delay (16667 / 1000);
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
	SDL_Color cmap[256];

	if (!vid_initialized)
		return;

	I_WaitVBL(1);

	c = cmap;
	cend = c + 256;
	for ( ; c != cend; c++)
	{
		c->r = gammatable[usegamma][*palette++];
		c->g = gammatable[usegamma][*palette++];
		c->b = gammatable[usegamma][*palette++];
	}
	SDL_SetColors (sdl_screen, cmap, 0, 256);
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

//
// blit screen to video
//
	if (DisplayTicker)
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
		{
			tics = 20;
		}
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

		SDL_UpdateRect(sdl_screen, 0, 0, SCREENWIDTH, SCREENHEIGHT);
	}
	if (UpdateState & I_FULLVIEW)
	{
		if (UpdateState & I_MESSAGES && screenblocks > 7)
		{
			for (i = 0; i < (viewwindowy + viewheight)*SCREENWIDTH; i += SCREENWIDTH)
			{
				memcpy(pcscreen + i, screen + i, SCREENWIDTH);
			}
			UpdateState &= ~(I_FULLVIEW|I_MESSAGES);

			SDL_UpdateRect (sdl_screen, 0, 0, SCREENWIDTH, viewwindowy + viewheight);
		}
		else
		{
			for (i = viewwindowy*SCREENWIDTH + viewwindowx;
			     i < (viewwindowy+viewheight)*SCREENWIDTH; i += SCREENWIDTH)
			{
				memcpy(pcscreen + i, screen + i, viewwidth);
			}
			UpdateState &= ~I_FULLVIEW;

			SDL_UpdateRect (sdl_screen, viewwindowx, viewwindowy, viewwidth, viewheight);
		}
	}
	if (UpdateState & I_STATBAR)
	{
		memcpy(pcscreen + SCREENWIDTH*(SCREENHEIGHT-SBARHEIGHT),
			screen + SCREENWIDTH*(SCREENHEIGHT-SBARHEIGHT),
			SCREENWIDTH*SBARHEIGHT);
		UpdateState &= ~I_STATBAR;

		SDL_UpdateRect (sdl_screen, 0, SCREENHEIGHT-SBARHEIGHT, SCREENWIDTH, SBARHEIGHT);
	}
	if (UpdateState & I_MESSAGES)
	{
		memcpy(pcscreen, screen, SCREENWIDTH*28);
		UpdateState &= ~I_MESSAGES;

		SDL_UpdateRect (sdl_screen, 0, 0, SCREENWIDTH, 28);
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
	Uint32 flags = DEFAULT_FLAGS;

	if (M_CheckParm("-novideo"))	// if true, stay in text mode for debugging
	{
		ST_Message("I_InitGraphics: Video Disabled.\n");
		return;
	}

	if (M_CheckParm("-f") || M_CheckParm("--fullscreen"))
		flags |= SDL_FULLSCREEN;
	if (M_CheckParm("-w") || M_CheckParm("--windowed"))
		flags &= ~SDL_FULLSCREEN;

	// Needs some work to get screenHeight and screenWidth working - S.A.

	// SDL_DOUBLEBUF does not work in full screen mode.  Does not seem to
	// be necessary anyway.
	sdl_screen = SDL_SetVideoMode(SCREENWIDTH, SCREENHEIGHT, 8, flags);

	if (sdl_screen == NULL)
	{
		I_Error("Couldn't set video mode %dx%d: %s\n",
			SCREENWIDTH, SCREENHEIGHT, SDL_GetError());
	}

	vid_initialized = true;

	// Only grab if we want to
	if (!M_CheckParm ("--nograb") && !M_CheckParm ("-g"))
	{
		SDL_WM_GrabInput (SDL_GRAB_ON);
	}

	SDL_ShowCursor (0);
	snprintf (text, sizeof(text), "HHexen v%d.%d.%d",
		  VERSION_MAJ, VERSION_MIN, VERSION_PATCH);
	SDL_WM_SetCaption (text, "HHEXEN");

	pcscreen = destscreen = (byte *) sdl_screen->pixels;

	I_SetPalette ((byte *)W_CacheLumpName("PLAYPAL", PU_CACHE));
}

//--------------------------------------------------------------------------
//
// PROC I_ShutdownGraphics
//
//--------------------------------------------------------------------------

void I_ShutdownGraphics(void)
{
	if (!vid_initialized)
		return;
	vid_initialized = false;
	SDL_Quit ();
}

//===========================================================================

//
//  Translates the key
//
static int xlatekey (SDL_keysym *key)
{
	switch (key->sym)
	{
	// S.A.
	case SDLK_LEFTBRACKET:	return KEY_LEFTBRACKET;
	case SDLK_RIGHTBRACKET:	return KEY_RIGHTBRACKET;
	case SDLK_BACKQUOTE:	return KEY_BACKQUOTE;
	case SDLK_QUOTEDBL:	return KEY_QUOTEDBL;
	case SDLK_QUOTE:	return KEY_QUOTE;
	case SDLK_SEMICOLON:	return KEY_SEMICOLON;
	case SDLK_PERIOD:	return KEY_PERIOD;
	case SDLK_COMMA:	return KEY_COMMA;
	case SDLK_SLASH:	return KEY_SLASH;
	case SDLK_BACKSLASH:	return KEY_BACKSLASH;

	case SDLK_LEFT:		return KEY_LEFTARROW;
	case SDLK_RIGHT:	return KEY_RIGHTARROW;
	case SDLK_DOWN:		return KEY_DOWNARROW;
	case SDLK_UP:		return KEY_UPARROW;
	case SDLK_ESCAPE:	return KEY_ESCAPE;
	case SDLK_RETURN:	return KEY_ENTER;

	case SDLK_F1:		return KEY_F1;
	case SDLK_F2:		return KEY_F2;
	case SDLK_F3:		return KEY_F3;
	case SDLK_F4:		return KEY_F4;
	case SDLK_F5:		return KEY_F5;
	case SDLK_F6:		return KEY_F6;
	case SDLK_F7:		return KEY_F7;
	case SDLK_F8:		return KEY_F8;
	case SDLK_F9:		return KEY_F9;
	case SDLK_F10:		return KEY_F10;
	case SDLK_F11:		return KEY_F11;
	case SDLK_F12:		return KEY_F12;

	case SDLK_INSERT:	return KEY_INS;
	case SDLK_DELETE:	return KEY_DEL;
	case SDLK_PAGEUP:	return KEY_PGUP;
	case SDLK_PAGEDOWN:	return KEY_PGDN;
	case SDLK_HOME:		return KEY_HOME;
	case SDLK_END:		return KEY_END;
	case SDLK_BACKSPACE:	return KEY_BACKSPACE;
	case SDLK_PAUSE:	return KEY_PAUSE;
	case SDLK_EQUALS:	return KEY_EQUALS;
	case SDLK_MINUS:	return KEY_MINUS;

	case SDLK_LSHIFT:
	case SDLK_RSHIFT:
		return KEY_RSHIFT;

	case SDLK_LCTRL:
	case SDLK_RCTRL:
		return KEY_RCTRL;

	case SDLK_LALT:
	case SDLK_LMETA:
	case SDLK_RALT:
	case SDLK_RMETA:
		return KEY_RALT;

	case SDLK_KP0:
		if (key->mod & KMOD_NUM)
			return SDLK_0;
		else
			return KEY_INS;
	case SDLK_KP1:
		if (key->mod & KMOD_NUM)
			return SDLK_1;
		else
			return KEY_END;
	case SDLK_KP2:
		if (key->mod & KMOD_NUM)
			return SDLK_2;
		else
			return KEY_DOWNARROW;
	case SDLK_KP3:
		if (key->mod & KMOD_NUM)
			return SDLK_3;
		else
			return KEY_PGDN;
	case SDLK_KP4:
		if (key->mod & KMOD_NUM)
			return SDLK_4;
		else
			return KEY_LEFTARROW;
	case SDLK_KP5:
		return SDLK_5;
	case SDLK_KP6:
		if (key->mod & KMOD_NUM)
			return SDLK_6;
		else
			return KEY_RIGHTARROW;
	case SDLK_KP7:
		if (key->mod & KMOD_NUM)
			return SDLK_7;
		else
			return KEY_HOME;
	case SDLK_KP8:
		if (key->mod & KMOD_NUM)
			return SDLK_8;
		else
			return KEY_UPARROW;
	case SDLK_KP9:
		if (key->mod & KMOD_NUM)
			return SDLK_9;
		else
			return KEY_PGUP;

	case SDLK_KP_PERIOD:
		if (key->mod & KMOD_NUM)
			return SDLK_PERIOD;
		else
			return KEY_DEL;
	case SDLK_KP_DIVIDE:	return SDLK_SLASH;
	case SDLK_KP_MULTIPLY:	return SDLK_ASTERISK;
	case SDLK_KP_MINUS:	return KEY_MINUS;
	case SDLK_KP_PLUS:	return SDLK_PLUS;
	case SDLK_KP_ENTER:	return KEY_ENTER;
	case SDLK_KP_EQUALS:	return KEY_EQUALS;

	default:
		return key->sym;
	}
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
		if (mod & (KMOD_RCTRL|KMOD_LCTRL))
		{
			if (Event->key.keysym.sym == 'g')
			{
				if (SDL_WM_GrabInput (SDL_GRAB_QUERY) == SDL_GRAB_OFF)
					SDL_WM_GrabInput (SDL_GRAB_ON);
				else
					SDL_WM_GrabInput (SDL_GRAB_OFF);
				break;
			}
		}
		else if (mod & (KMOD_RALT|KMOD_LALT))
		{
			if (Event->key.keysym.sym == SDLK_RETURN)
			{
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
		event.data1 = 0	| (buttonstate & SDL_BUTTON(1) ? 1 : 0)
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
			event.data1 = 0	| (Event->motion.state & SDL_BUTTON(1) ? 1 : 0)
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

