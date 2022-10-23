#include "h2stdinc.h"
#include "SDL.h"
#include "h2def.h"
#include "r_local.h"

#define BASE_WINDOW_FLAGS	0
#ifdef FULLSCREEN_DEFAULT
#define DEFAULT_FLAGS		(BASE_WINDOW_FLAGS|SDL_WINDOW_FULLSCREEN_DESKTOP)
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

static SDL_Window* sdl_window = NULL;
static SDL_Renderer* sdl_renderer = NULL;
static SDL_Surface* surface = NULL;
static SDL_Surface* screen_surface = NULL;
static SDL_Texture* render_texture = NULL;

static int screenWidth = SCREENWIDTH*2;
static int screenHeight = SCREENHEIGHT*2;

static Uint32 sdl_pixel_format;
static boolean vid_initialized = false;
static int grabMouse;


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
	if ( !vid_initialized )
		return;

	SDL_Color colormap[256];
	
	for ( int i = 0; i < 256; i++ )
	{
		colormap[i].r = gammatable[usegamma][*palette++];
		colormap[i].g = gammatable[usegamma][*palette++];
		colormap[i].b = gammatable[usegamma][*palette++];
	}
	
	SDL_SetPaletteColors(surface->format->palette, colormap, 0, 256);
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
	SDL_Rect rect;

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

		i = I_GetTime();

		tics = i - lasttic;
		lasttic = i;
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

		rect = (SDL_Rect) { .x = 0, .y = 0, .w = SCREENWIDTH, .h = SCREENHEIGHT };
		SDL_LowerBlit (surface, &rect, screen_surface, &rect);
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

			rect = (SDL_Rect) { .x = 0, .y = 0, .w = SCREENWIDTH, .h = viewwindowy + viewheight };
			SDL_LowerBlit (surface, &rect, screen_surface, &rect);
		}
		else
		{
			for (i = viewwindowy*SCREENWIDTH + viewwindowx;
			     i < (viewwindowy+viewheight)*SCREENWIDTH; i += SCREENWIDTH)
			{
				memcpy(pcscreen + i, screen + i, viewwidth);
			}
			UpdateState &= ~I_FULLVIEW;

			rect = (SDL_Rect) { .x = viewwindowx, .y = viewwindowy, .w = viewwidth, .h = viewheight };
			SDL_LowerBlit (surface, &rect, screen_surface, &rect);
		}
	}
	if (UpdateState & I_STATBAR)
	{
		memcpy(pcscreen + SCREENWIDTH*(SCREENHEIGHT-SBARHEIGHT),
			screen + SCREENWIDTH*(SCREENHEIGHT-SBARHEIGHT),
			SCREENWIDTH*SBARHEIGHT);
		UpdateState &= ~I_STATBAR;

		rect = (SDL_Rect) { .x = 0, .y = SCREENHEIGHT-SBARHEIGHT, .w = SCREENWIDTH, .h = SBARHEIGHT };
		SDL_LowerBlit (surface, &rect, screen_surface, &rect);
	}
	if (UpdateState & I_MESSAGES)
	{
		memcpy(pcscreen, screen, SCREENWIDTH*28);
		UpdateState &= ~I_MESSAGES;

		rect = (SDL_Rect) { .x = 0, .y = 0, .w = SCREENWIDTH, .h = 28 };
		SDL_LowerBlit (surface, &rect, screen_surface, &rect);
	}

	SDL_UpdateTexture (render_texture, NULL, screen_surface->pixels,
		screen_surface->pitch);

	SDL_RenderClear (sdl_renderer);
	SDL_RenderCopy (sdl_renderer, render_texture, NULL, NULL);
	SDL_RenderPresent (sdl_renderer);
}

//--------------------------------------------------------------------------
//
// PROC I_InitGraphics
//
//--------------------------------------------------------------------------

void I_InitGraphics(void)
{
	char text[20];
	int p, bpp;
	Uint32 flags = DEFAULT_FLAGS;
	Uint32 rmask, gmask, bmask, amask;

	snprintf (text, sizeof(text), "HHexen v%d.%d.%d",
		  VERSION_MAJ, VERSION_MIN, VERSION_PATCH);

	if (M_CheckParm("-novideo"))	// if true, stay in text mode for debugging
	{
		printf("I_InitGraphics: Video Disabled.\n");
		return;
	}

	if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
		I_Error("Couldn't init video: %s", SDL_GetError());
	}

	if (M_CheckParm("-f") || M_CheckParm("--fullscreen"))
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	if (M_CheckParm("-w") || M_CheckParm("--windowed"))
		flags &= ~SDL_WINDOW_FULLSCREEN_DESKTOP;

	p = M_CheckParm ("-height");
	if (p && p < myargc -1)
	{
		screenHeight = atoi (myargv[p+1]);
	}

	p = M_CheckParm ("-width");
	if (p && p < myargc -1)
	{
		screenWidth = atoi (myargv[p+1]);
	}
	printf("Screen size: %dx%d\n",screenWidth, screenHeight);

	// Needs some work to get screenHeight and screenWidth working - S.A.
	sdl_window = SDL_CreateWindow(text, SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED, screenWidth, screenHeight, flags);
		
	if ( sdl_window == NULL )
	{
		I_Error ("Couldn't initialize SDL2: %s\n", SDL_GetError());
	}
	
	sdl_renderer = SDL_CreateRenderer (sdl_window, -1,
		SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
	
	if ( sdl_renderer == NULL )
	{
		SDL_DestroyWindow (sdl_window);
		I_Error ("Couldn't create SDL2 renderer: %s\n", SDL_GetError());
	}

	sdl_pixel_format = SDL_GetWindowPixelFormat (sdl_window);

	surface = SDL_CreateRGBSurface (0, SCREENWIDTH, SCREENHEIGHT, 8,
		0, 0, 0, 0);

	SDL_FillRect (surface, NULL, 0);

	SDL_PixelFormatEnumToMasks (sdl_pixel_format, &bpp, &rmask, &gmask, 
		&bmask, &amask);

	screen_surface = SDL_CreateRGBSurface (0, SCREENWIDTH, SCREENHEIGHT,
		bpp, rmask, gmask, bmask, amask);

	SDL_FillRect (screen_surface, NULL, 0);

	render_texture = SDL_CreateTexture (sdl_renderer, sdl_pixel_format,
		SDL_TEXTUREACCESS_STREAMING, SCREENWIDTH, SCREENHEIGHT);

	vid_initialized = true;

	// Only grab if we want to
	if (!M_CheckParm ("--nograb") && !M_CheckParm ("-g"))
	{
		grabMouse = 1;
		SDL_SetRelativeMouseMode (SDL_TRUE);
	}

	SDL_ShowCursor (SDL_DISABLE);

	pcscreen = destscreen = (byte *) surface->pixels;

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

	if (surface != NULL)
		SDL_FreeSurface (surface);

	if (screen_surface != NULL)
		SDL_FreeSurface (screen_surface);

	if (render_texture != NULL)
		SDL_DestroyTexture (render_texture);

	if (sdl_renderer != NULL)
		SDL_DestroyRenderer (sdl_renderer);

	if (sdl_window != NULL)
		SDL_DestroyWindow (sdl_window);

	vid_initialized = false;
}

//===========================================================================

//
//  Translates the key
//
static int xlatekey (SDL_Keysym *key)
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
		return KEY_LALT;
	case SDLK_RALT:
		return KEY_RALT;

	case SDLK_KP_0:
		if (key->mod & KMOD_NUM)
			return SDLK_0;
		else
			return KEY_INS;
	case SDLK_KP_1:
		if (key->mod & KMOD_NUM)
			return SDLK_1;
		else
			return KEY_END;
	case SDLK_KP_2:
		if (key->mod & KMOD_NUM)
			return SDLK_2;
		else
			return KEY_DOWNARROW;
	case SDLK_KP_3:
		if (key->mod & KMOD_NUM)
			return SDLK_3;
		else
			return KEY_PGDN;
	case SDLK_KP_4:
		if (key->mod & KMOD_NUM)
			return SDLK_4;
		else
			return KEY_LEFTARROW;
	case SDLK_KP_5:
		return SDLK_5;
	case SDLK_KP_6:
		if (key->mod & KMOD_NUM)
			return SDLK_6;
		else
			return KEY_RIGHTARROW;
	case SDLK_KP_7:
		if (key->mod & KMOD_NUM)
			return SDLK_7;
		else
			return KEY_HOME;
	case SDLK_KP_8:
		if (key->mod & KMOD_NUM)
			return SDLK_8;
		else
			return KEY_UPARROW;
	case SDLK_KP_9:
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


/* Shamelessly stolen from PrBoom+ */
static int I_SDLtoHexenMouseState(Uint8 buttonstate)
{
	return 0
		| ((buttonstate & SDL_BUTTON(1)) ? 1 : 0)
		| ((buttonstate & SDL_BUTTON(2)) ? 2 : 0)
		| ((buttonstate & SDL_BUTTON(3)) ? 4 : 0);
}

/* This processes SDL events */
void I_GetEvent(SDL_Event *Event)
{
	event_t event;
	SDL_Keymod mod;

	switch (Event->type)
	{
	case SDL_KEYDOWN:
		mod = SDL_GetModState ();
		if (mod & (KMOD_RCTRL|KMOD_LCTRL))
		{
			if (Event->key.keysym.sym == 'g')
			{
				if (SDL_GetRelativeMouseMode () == SDL_FALSE)
				{
					grabMouse = 1;
					SDL_SetRelativeMouseMode (SDL_TRUE);
				}
				else
				{
					grabMouse = 0;
					SDL_SetRelativeMouseMode (SDL_FALSE);
				}
				break;
			}
		}
		else if (mod & (KMOD_RALT|KMOD_LALT))
		{
			if (Event->key.keysym.sym == SDLK_RETURN)
			{
				SDL_SetWindowFullscreen(sdl_window, 
					SDL_WINDOW_FULLSCREEN_DESKTOP);
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
		event.type = ev_mouse;
		event.data1 = I_SDLtoHexenMouseState(SDL_GetMouseState(NULL,NULL));
		event.data2 = event.data3 = 0;
		H2_PostEvent(&event);
		break;

	case SDL_MOUSEMOTION:
		/* Ignore mouse warp events */
		if ((Event->motion.x != SCREENWIDTH/2) ||
		    (Event->motion.y != SCREENHEIGHT/2) )
		{
			/* Warp the mouse back to the center */
			/*
			if (grabMouse) {
				SDL_WarpMouse(SCREENWIDTH/2, SCREENHEIGHT/2);
			}
			*/
			event.type = ev_mouse;
			event.data1 = I_SDLtoHexenMouseState(Event->motion.state);
			event.data2 = Event->motion.xrel << 3;
			event.data3 = -Event->motion.yrel << 3;
			H2_PostEvent(&event);
		}
		break;

	case SDL_QUIT:
		I_Quit();
		break;

	default:
		break;
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
