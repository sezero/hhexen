//**************************************************************************
//**
//** $Id: i_gl.cpp,v 1.2 2000-04-15 00:23:25 theoddone33 Exp $
//**
//**************************************************************************


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/time.h>
#include <X11/keysym.h>
#include "x11window.h"
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>


extern "C" {
#include "h2def.h"

extern void OGL_InitData();
extern void OGL_InitRenderer();
extern void OGL_ResetData();	
extern void OGL_ResetLumpTexData();
}


void OGL_GrabScreen();


class HexenWindow : public X11Window
{
public:

    HexenWindow();

    ~HexenWindow();

    void swap() { glXSwapBuffers( display(), window() ); }

    GLXContext context() { return _ctx; }

    void ungrabPointer()
    {
        XUngrabPointer( display(), CurrentTime );
        showCursor();
        _grabCursor = false;
    }

    void grabPointer()
    {
        XGrabPointer( display(), window(), True,
                ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                GrabModeAsync, GrabModeAsync, window(), None, CurrentTime );
        hideCursor();
        _grabCursor = true;
    }

protected:

    virtual void unknownEvent( XEvent* );
    virtual void configureEvent( XConfigureEvent* );
    virtual void deleteEvent( XEvent* );
    virtual void buttonDown( XButtonEvent* );
    virtual void buttonUp( XButtonEvent* );
    virtual void motionEvent( XMotionEvent* );
    virtual void keyDown( XKeyEvent* );
    virtual void keyUp( XKeyEvent* );
    virtual void exposeEvent( XExposeEvent* );

private:

    void postKey( evtype_t type, KeySym key );
    void postMouseEvent( int dx, int dy );
 
    GLXContext _ctx;
    XVisualInfo* _vinfo;
 
    int _prevX, _prevY;
    int _buttons;
    bool _grabCursor;
};


HexenWindow* _win;


extern "C" {


// Public Data

int screenWidth  = SCREENWIDTH*2;
int screenHeight = SCREENHEIGHT*2;
int maxTexSize = 256;
int ratioLimit = 0;		// Zero if none.
int test3dfx = 0;

int DisplayTicker = 0;

extern int ticcount;

extern boolean novideo; // if true, stay in text mode for debugging


//==================================================

#define MOUSEB1 1
#define MOUSEB2 2
#define MOUSEB3 4

//==================================================

#define KEY_TAB         9   // From am_map.h

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

void I_SetPalette( byte *palette )
{
}

/*
============================================================================

							GRAPHICS MODE

============================================================================
*/

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
	if( UpdateState == I_NOUPDATE )
        return;

    _win->swap();

	UpdateState = I_NOUPDATE; // clear out all draw types
}


//--------------------------------------------------------------------------
//
// PROC I_InitGraphics
//
//--------------------------------------------------------------------------

void I_InitGraphics(void)
{
	int p;

	if( novideo )
	{
		return;
	}
    p = M_CheckParm( "-height" );
    if (p && p < myargc - 1)
    {
	screenHeight = atoi(myargv[p+1]);
    }
    p = M_CheckParm( "-width" );
    if( p && p < myargc - 1 )
    {
	screenWidth = atoi(myargv[p+1]);
    }
    ST_Message("Screen Width %d, Screen Height %d\n",screenWidth, screenHeight);    
    _win = new HexenWindow();
    if( ! _win )
    {
        exit( 3 );
    }

    //OGL_InitData();     // JHexen has this at the end of R_Init().
	OGL_InitRenderer();

	// Clear the buffers.
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    _win->swap();

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

    _win->show();

	//I_SetPalette( (byte*) W_CacheLumpName("PLAYPAL", PU_CACHE) );
    _win->grabPointer ();
}

//--------------------------------------------------------------------------
//
// PROC I_ShutdownGraphics
//
//--------------------------------------------------------------------------

void I_ShutdownGraphics(void)
{
	OGL_ResetData();
	OGL_ResetLumpTexData();
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


void I_StartTic (void)
{
    // Handle keyboard & mouse events.

    while( _win->eventsPending() )
        _win->handleNextEvent();

	//I_ReadMouse();
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

void I_StartupCyberMan(void);

void I_StartupMouse (void)
{
	I_StartupCyberMan();
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
   //if( KEY_ESCAPE pressed )
   //    I_Quit ();
}


void checkGLContext()
{
    GLXContext c = glXGetCurrentContext();
    if( ! c || (c != _win->context()) )
        printf( "Bad context!\n" );
    else
        printf( "context ok\n" );
}

}   // extern "C"


//---------------------------------------------------------------------------


HexenWindow::HexenWindow()
        : X11Window( "HHEXEN", 0, 0, screenWidth, screenHeight )
{
    int attrib[] = { GLX_RGBA,
                     GLX_RED_SIZE, 1, GLX_GREEN_SIZE, 1, GLX_BLUE_SIZE, 1,
                     GLX_DEPTH_SIZE, 1, GLX_DOUBLEBUFFER, None };

    _ctx = 0;
    _vinfo = 0;
    _grabCursor = true;

    _buttons = 0;
    _prevX = _prevY = 0; 

    if( glXQueryExtension( display(), 0, 0 ) == 0 )
    {
        fprintf( stderr, "GLX Extension not available!\n" );
        return;
    }

    _vinfo = glXChooseVisual( display(), screen(), attrib );
    if( ! _vinfo )
    {
        fprintf( stderr, "Couldn't get an RGB, double-buffered visual!\n" );
        return;
    }

    _ctx = glXCreateContext( display(), _vinfo, NULL, True );
    glXMakeCurrent( display(), window(), _ctx );

    setTitle( "HHexen v1.4" );
    setSizeHints( screenWidth,screenHeight,screenWidth,screenHeight );
    setIconName( "HHEXEN" );
}


HexenWindow::~HexenWindow()
{
    if( _ctx )
    {
        glXDestroyContext( display(), _ctx );
    }
}


void HexenWindow::deleteEvent( XEvent* )
{
    I_Quit();
}

void HexenWindow::configureEvent( XConfigureEvent* e )
{
    screenWidth  = width();
    screenHeight = height();
}

void HexenWindow::unknownEvent( XEvent* e )
{
}


void HexenWindow::buttonDown( XButtonEvent* e )
{
    if( ! _grabCursor )
        grabPointer();

    switch( e->button )
    {
        case Button1: _buttons |= MOUSEB1 ; break;
        case Button2: _buttons |= MOUSEB2 ; break;
        case Button3: _buttons |= MOUSEB3 ; break;
        default:
            return;
    }
    postMouseEvent( 0, 0 );
}


void HexenWindow::buttonUp( XButtonEvent* e )
{
    switch( e->button )
    {
        case Button1: _buttons &= ~MOUSEB1 ; break;
        case Button2: _buttons &= ~MOUSEB2 ; break;
        case Button3: _buttons &= ~MOUSEB3 ; break;
        default:
            return;
    }
    postMouseEvent( 0, 0 );
}


void HexenWindow::motionEvent( XMotionEvent* e )
{

   int dx,dy;

   if (e->x == width()/2 && e->y == height()/2)
   {
	_prevX = e->x;
	_prevY = e->y;
	return;
   }
   dx = (e->x - _prevX);
   _prevX = e->x;
   dy = (e->y - _prevY);
   _prevY = e->y;

   if( dx || dy )
   {
      postMouseEvent( dx, dy );

      if( _grabCursor )
      {
          if( (e->x < 30) || (e->x > (screenWidth-30)) ||
              (e->y < 30) || (e->y > (screenHeight-30)) )
          {
              XWarpPointer( display(), None, window(), 0, 0, 0, 0,
                            screenWidth / 2, screenHeight / 2 );
	      _prevX = e->x; _prevY = e->y;
          }
      }
   }
}

void HexenWindow::keyDown( XKeyEvent* e )
{
    KeySym key = keysym( e );

    //TODO: filter key repeat.

    if( e->state & Mod1Mask )   // Control key defaults to attack.
    {
        if( key == XK_s )
        {
            OGL_GrabScreen();
        }
        else if( key == XK_g )
        {
            if( _grabCursor )
            {
                ungrabPointer();
            }
            else
            {
                grabPointer();
            }
        }   
    }
    else
    {
        postKey( ev_keydown, key );
    }
}


void HexenWindow::keyUp( XKeyEvent* e )
{
    postKey( ev_keyup, keysym( e ) );
}


void HexenWindow::exposeEvent( XExposeEvent* )
{
    UpdateState |= I_FULLSCRN;
}


void HexenWindow::postKey( evtype_t type, KeySym key )
{
	event_t ev;

    ev.type = type;

    switch( key )
    {
        case XK_Up:     ev.data1 = KEY_UPARROW;    break;
        case XK_Down:   ev.data1 = KEY_DOWNARROW;  break;
        case XK_Left:   ev.data1 = KEY_LEFTARROW;  break;
        case XK_Right:  ev.data1 = KEY_RIGHTARROW; break;

        case XK_Escape: ev.data1 = KEY_ESCAPE;     break;
        case XK_Return: ev.data1 = KEY_ENTER;      break;
        case XK_F1:     ev.data1 = KEY_F1;         break;
        case XK_F2:     ev.data1 = KEY_F2;         break;
        case XK_F3:     ev.data1 = KEY_F3;         break;
        case XK_F4:     ev.data1 = KEY_F4;         break;
        case XK_F5:     ev.data1 = KEY_F5;         break;
        case XK_F6:     ev.data1 = KEY_F6;         break;
        case XK_F7:     ev.data1 = KEY_F7;         break;
        case XK_F8:     ev.data1 = KEY_F8;         break;
        case XK_F9:     ev.data1 = KEY_F9;         break;
        case XK_F10:    ev.data1 = KEY_F10;        break;
        case XK_F11:    ev.data1 = KEY_F11;        break;
        case XK_F12:    ev.data1 = KEY_F12;        break;

        case XK_Insert:    ev.data1 = KEY_INS;     break;
        case XK_Delete:    ev.data1 = KEY_DEL;     break;
        case XK_Page_Up:   ev.data1 = KEY_PGUP;    break;
        case XK_Page_Down: ev.data1 = KEY_PGDN;    break;
        case XK_Home:      ev.data1 = KEY_HOME;    break;
        case XK_End:       ev.data1 = KEY_END;     break;

        case XK_Tab:       ev.data1 = KEY_TAB;     break;

        case XK_BackSpace: ev.data1 = KEY_BACKSPACE;  break;

        case XK_Pause:     ev.data1 = KEY_PAUSE;      break;

        case XK_equal:     ev.data1 = KEY_EQUALS;     break;

        case XK_KP_Subtract:
        case XK_minus:     ev.data1 = KEY_MINUS;      break;

        case XK_Shift_L:
        case XK_Shift_R:   ev.data1 = KEY_RSHIFT;     break;

        case XK_Control_L:
        case XK_Control_R: ev.data1 = KEY_RCTRL;      break;

        case XK_Alt_L:
        case XK_Alt_R:
        case XK_Meta_L:
        case XK_Meta_R:    ev.data1 = KEY_RALT;       break;

        default:
            ev.data1 = key;
            break;
    }

    H2_PostEvent( &ev );
}


void HexenWindow::postMouseEvent( int dx, int dy )
{
	event_t ev;

	ev.type  = ev_mouse;
	ev.data1 = _buttons;
	ev.data2 =  (short) dx << 2;
	ev.data3 = -(short) dy << 2;

	H2_PostEvent( &ev );
}


// Returns zero if a unique file name could not be found.
static int makeUniqueFilename( char* filename )
{
    int i;
 
    for( i = 0; i < 100; i++ )
    {
        sprintf( filename, "hexen%02d.ppm", i );
 
        if( access( filename, F_OK ) == -1 )
        {
            // It does not exist.
            return 1;
        }
    }
 
    return 0;
}


//--------------------------------------------------------------------------
//
// Copies the current contents of the frame buffer and returns a pointer
// to data containing 24-bit RGB triplets.
//
//--------------------------------------------------------------------------

void OGL_GrabScreen()
{
    FILE* fp;
    char filename[ 20 ];
	unsigned char* buffer = new unsigned char[ screenWidth * screenHeight * 3 ];

    if( buffer && makeUniqueFilename( filename ) ) 
    {
        glReadPixels( 0, 0, screenWidth, screenHeight, GL_RGB, 
                      GL_UNSIGNED_BYTE, buffer );

        fp = fopen( filename, "w" );
        if( fp )
        {
            unsigned char* rgb = buffer + (screenWidth * screenHeight * 3);

            fprintf( fp, "P6\n%d %d\n255\n", screenWidth, screenHeight );

            while( rgb > buffer )
            {
                rgb -= 3 * screenWidth;
                fwrite( rgb, 1, 3 * screenWidth, fp );
            }

            fclose( fp );
        }
    }
    
    delete buffer;
}

//EOF
