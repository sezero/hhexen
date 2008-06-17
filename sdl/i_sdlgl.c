//**************************************************************************
//**
//** $Id: i_sdlgl.c,v 1.3 2008-06-17 10:21:28 sezero Exp $
//**
//**************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/time.h>
#include <SDL/SDL.h> 
#include <GL/gl.h>
#include <GL/glu.h>
#include "h2def.h"
#include "r_local.h"
#include "p_local.h"    // for P_AproxDistance
#include "sounds.h"
#include "i_sound.h"
#include "soundst.h"
#include "st_start.h"

// Public Data

int screenWidth  = SCREENWIDTH*2;
int screenHeight = SCREENHEIGHT*2;
int maxTexSize = 256;
int ratioLimit = 0;             // Zero if none.
int test3dfx = 0;

int DisplayTicker = 0;


// Code
extern void OGL_InitData();
extern void OGL_InitRenderer();
extern void OGL_ResetData();
extern void OGL_ResetLumpTexData();

void I_StartupNet (void);
void I_ShutdownNet (void);
void I_ReadExternDriver(void);
void GrabScreen (void);

extern int usemouse, usejoystick;

extern void **lumpcache;

int i_Vector;
externdata_t *i_ExternData;
boolean useexterndriver;

boolean mousepresent;

int ticcount;

boolean novideo; // if true, stay in text mode for debugging

#define KEY_INS         0x52
#define KEY_DEL         0x53
#define KEY_PGUP        0x49
#define KEY_PGDN        0x51
#define KEY_HOME        0x47
#define KEY_END         0x4f

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
/*
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
*/
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
	if(UpdateState == I_NOUPDATE)
		return;

	SDL_GL_SwapBuffers();
	UpdateState = I_NOUPDATE;
}

//--------------------------------------------------------------------------
//
// PROC I_InitGraphics
//
//--------------------------------------------------------------------------

void I_InitGraphics(void)
{
    int p;
    char text[20];
    Uint32 flags = SDL_OPENGL;

    if( novideo )
    {
	return;
    }

    if (M_CheckParm("-f") || M_CheckParm("--fullscreen")) {
	flags |= SDL_FULLSCREEN;
	setenv ("MESA_GLX_FX","fullscreen", 1);
    } else {
	setenv ("MESA_GLX_FX","disable",1);
    }	
    p = M_CheckParm ("-height");
    if (p && p < myargc - 1)
    {
	screenHeight = atoi (myargv[p+1]);
    }
    p = M_CheckParm ("-width");
    if (p && p < myargc - 1) {
	screenWidth = atoi(myargv[p+1]);
    }
    ST_Message("Screen size:  %dx%d\n",screenWidth, screenHeight);

    if(SDL_SetVideoMode(screenWidth, screenHeight, 8, flags) == NULL)
    {
        fprintf( stderr, "Couldn't set video mode %dx%d: %s\n",
                 screenWidth, screenHeight, SDL_GetError() );
        exit( 3 );
    }

    OGL_InitRenderer ();

    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Print some OpenGL information.
    ST_Message( "I_InitGraphics: OpenGL information:\n" );
    ST_Message( "  Vendor: %s\n", glGetString(GL_VENDOR) );
    ST_Message( "  Renderer: %s\n", glGetString(GL_RENDERER) );
    ST_Message( "  Version: %s\n", glGetString(GL_VERSION) );
    ST_Message( "  GLU Version: %s\n", gluGetString((GLenum)GLU_VERSION) );

    // Check the maximum texture size.
    glGetIntegerv( GL_MAX_TEXTURE_SIZE, &maxTexSize );
    ST_Message("  Maximum texture size: %d\n", maxTexSize);
    if( maxTexSize == 256 )
    {
            //ST_Message("  Is this Voodoo? Using size ratio limit.\n");
            ratioLimit = 8;
    }

    if( M_CheckParm("-3dfxtest") )
    {
            test3dfx = 1;
            ST_Message("  3dfx test mode.\n");
    }

    // Only grab if we want to
    if (!M_CheckParm ("--nograb") && !M_CheckParm ("-g")) {
            SDL_WM_GrabInput (SDL_GRAB_ON);
    }

    SDL_ShowCursor( 0 );
    snprintf (text, 20, "HHexen v%s", HHEXEN_VERSION);
    SDL_WM_SetCaption( text, "HHEXEN" );


    //I_SetPalette( W_CacheLumpName("PLAYPAL", PU_CACHE) );
}

//--------------------------------------------------------------------------
//
// PROC I_ShutdownGraphics
//
//--------------------------------------------------------------------------

void I_ShutdownGraphics(void)
{
    OGL_ResetData ();
    OGL_ResetLumpTexData ();
    SDL_Quit ();
}

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
		}
	} else if (mod & KMOD_RALT || mod & KMOD_LALT) {
		if (Event->key.keysym.sym == SDLK_RETURN) {
			SDL_WM_ToggleFullScreen(SDL_GetVideoSurface());
		}
	} else { 
	        event.type = ev_keydown;
	        event.data1 = xlatekey(&Event->key.keysym);
	        H2_PostEvent(&event);
	}
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
        if ( (Event->motion.x != SCREENWIDTH/2) ||
             (Event->motion.y != SCREENHEIGHT/2) )
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

static int makeUniqueFilename( char* filename )
{
    int i;

    for( i = 0; i < 100; i++ )
    {
        sprintf( filename, "hexen%02d.bmp", i );

        if( access( filename, F_OK ) == -1 )
        {
            // It does not exist.
            return 1;
        }
    }

    return 0;
}

void GrabScreen ()
{
    SDL_Surface *image;
    SDL_Surface *temp;
    int idx;
    char filename[80];

    if (makeUniqueFilename(filename)) {
        image = SDL_CreateRGBSurface(SDL_SWSURFACE, screenWidth, screenHeight,
                                    24, 0x0000FF, 0x00FF00, 0xFF0000,0xFFFFFF);
	temp = SDL_CreateRGBSurface(SDL_SWSURFACE, screenWidth, screenHeight,
	                            24, 0x0000FF, 0x00FF00, 0xFF0000, 0xFFFFFF);

        glReadPixels(0, 0, screenWidth, screenHeight, GL_RGB,
                     GL_UNSIGNED_BYTE, image->pixels);
        for (idx = 0; idx < screenHeight; idx++)
        {
               memcpy(temp->pixels + 3 * screenWidth * idx,
                        (char *)image->pixels + 3
                        * screenWidth*(screenHeight - idx),
                        3*screenWidth);
        }
        memcpy(image->pixels,temp->pixels,screenWidth * screenHeight * 3);
        SDL_SaveBMP(image, filename);
        SDL_FreeSurface(image);
    }
}

