//**************************************************************************
//**
//** $Id: i_x11.cpp,v 1.3 2000-04-18 16:11:12 theoddone33 Exp $
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
#include "xshmext.h"

extern "C" {
#include "h2def.h"
}


/*
    Nifty palette cacheing idea taken from Linux Heretic.

    The Hexen palette pointer is used as a palette id, i.e. a palette
    with another pointer value is assumed to be a different
    palette, and, more important, if the pointer is equal,
    the palette is assumed to be the same...
*/

#define PAL_CACHE_ON
#define MAX_PAL_CACHE   5

#define MOUSE_JUMP_AT 10

struct PaletteCache
{
    PaletteCache() : id( 0 ), used( 0 ) {}

    void free( Display* dis, Colormap cmap )
    {
        if( id )
        {
            XFreeColors( dis, cmap, pixel, 256, 0 );
            id = 0;
            used = 0;
        }
    }

    unsigned char* id;           // contains pointer / id of palette.
    unsigned int used;
    unsigned long pixel[ 256 ];  // xpixel lookup table.
};


class HexenWindow : public X11Window
{
public:

    HexenWindow();

    ~HexenWindow();

    void setPalette( byte* );

    void blit( unsigned char* buffer, int x, int y, int w, int h );

    void grabPointer()
    {
        XGrabPointer( display(), window(), True,
                ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                GrabModeAsync, GrabModeAsync, window(), None, CurrentTime );
        hideCursor();
        _grabCursor = true;
    }

    void ungrabPointer()
    {
        XUngrabPointer( display(), CurrentTime );
        showCursor();
        _grabCursor = false;
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

    void waitForShmPut();
    void resizeFramebuffer();
    bool getVisualInfo( XVisualInfo* );
    void postKey( evtype_t type, KeySym key );
    void postMouseEvent( int dx, int dy );
 
    XVisualInfo _vinfo;
    GC _gc;
 
    Colormap _colormap;
    PaletteCache _palc[ MAX_PAL_CACHE ];
    PaletteCache* _pcurrent;
    int _cacheGamma;
    int _cacheAge;

    ShmImage* _simage;
    int _shmEventType;
    bool _usingShm;
 
    XImage* _ximage;

    int _prevX, _prevY;
    int _buttons;
    bool _grabCursor;
};


HexenWindow* _win;


extern "C" {


// Public Data

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
	if(novideo)
	{
		return;
	}

    _win->setPalette( palette );
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
	int i;
	byte *dest;
	int tics;
	static int lasttic;


    // blit screen to video

	if(DisplayTicker)
	{
#if 0
        // Why is it drawing to pcscreen under some conditions?
		if(screenblocks > 9 || UpdateState&(I_FULLSCRN|I_MESSAGES))
		{
			dest = (byte *)screen;
		}
		else
		{
			dest = (byte *)pcscreen;
		}
#else
        dest = (byte *)screen;
#endif

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

	if(UpdateState == I_NOUPDATE)
	{
		return;
	}
    if(UpdateState&I_FULLSCRN)
	{
		UpdateState = I_NOUPDATE; // clear out all draw types

        _win->blit( screen, 0, 0, SCREENWIDTH, SCREENHEIGHT );
	}
    if(UpdateState&I_FULLVIEW)
	{
		if(UpdateState&I_MESSAGES && screenblocks > 7)
		{
			UpdateState &= ~(I_FULLVIEW|I_MESSAGES);

            _win->blit( screen, 0, 0, SCREENWIDTH, viewwindowy+viewheight );
		}
		else
		{
			UpdateState &= ~I_FULLVIEW;

            _win->blit( screen, viewwindowx, viewwindowy, viewwidth,
                        viewheight );
		}
	}
	if(UpdateState&I_STATBAR)
	{
		UpdateState &= ~I_STATBAR;

        _win->blit( screen, 0, SCREENHEIGHT-SBARHEIGHT,
                    SCREENWIDTH, SBARHEIGHT );
	}
	if(UpdateState&I_MESSAGES)
	{
		UpdateState &= ~I_MESSAGES;

        _win->blit( screen, 0, 0, SCREENWIDTH, 28 );
	}
}

//--------------------------------------------------------------------------
//
// PROC I_InitGraphics
//
//--------------------------------------------------------------------------

void I_InitGraphics(void)
{
	if( novideo )
	{
		return;
	}
    
    _win = new HexenWindow();
    if( ! _win )
    {
        exit( 3 );
    }

    _win->show();

	I_SetPalette( (byte*) W_CacheLumpName("PLAYPAL", PU_CACHE) );
    _win->grabPointer ();
}

//--------------------------------------------------------------------------
//
// PROC I_ShutdownGraphics
//
//--------------------------------------------------------------------------

void I_ShutdownGraphics(void)
{
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

void I_StartupMouse (void)
{
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


}   // extern "C"


//---------------------------------------------------------------------------


HexenWindow::HexenWindow()
        : X11Window( "HEXEN", 0, 0, SCREENWIDTH*2, SCREENHEIGHT*2 )
{
    _colormap = 0;
    _gc = 0;
    _simage = 0;
    _ximage = 0;
    _usingShm = false;
    _grabCursor = false;

    _pcurrent = 0;
    _cacheGamma = -1;
    _cacheAge = 0;

    _buttons = 0;
    _prevX = _prevY = 0; 

    setTitle( "HHexen v1.4" );
    setSizeHints( SCREENWIDTH, SCREENHEIGHT, SCREENWIDTH*2, SCREENHEIGHT*2 );
    setIconName( "HEXEN" );

    if( getVisualInfo( &_vinfo ) )
    {
        // KR
//        printf( "Visual: depth %d, mask %04x %04x %04x\n", _vinfo.depth,
//                _vinfo.red_mask, _vinfo.green_mask, _vinfo.blue_mask );

        if( _vinfo.depth == 8 )
        {
            _colormap = XCreateColormap( display(), window(), _vinfo.visual,
                                         AllocAll );
            XSetWindowColormap( display(), window(), _colormap );
        }
        else
        {
            _colormap = DefaultColormapOfScreen(
                                    ScreenOfDisplay( display(), screen() ) );
        }

        XGCValues gcv;
        gcv.graphics_exposures = False;
        _gc = XCreateGC( display(), window(), GCGraphicsExposures, &gcv );
        if( ! _gc )
            fprintf( stderr, "XCreateGC failed!\n" );
        
        _shmEventType = ShmImage::query( display() );
        if( _shmEventType )
        {
            printf( "Using X11 MITSHM extension\n" );
            _usingShm = true;   // May provide a user disable for Shm later.
        }

        resizeFramebuffer();
    }
    else
    {
        fprintf( stderr, "XMatchVisualInfo failed!\n" );
    }
}


HexenWindow::~HexenWindow()
{
    if( _vinfo.depth == 8 )
    {
        if( _colormap )
            XFreeColormap( display(), _colormap );
    }
    else
    {
        int i;
        for( i = 0; i < MAX_PAL_CACHE; i++ )
            _palc[ i ].free( display(), _colormap );
    }

    if( _usingShm )
    {
        delete _simage;
    }
    else if( _ximage )
    {
        delete _ximage->data;
        XDestroyImage( _ximage );
    }
}


void HexenWindow::setPalette( byte* palette )
{
    int i, c;

    if( _vinfo.c_class == PseudoColor && _vinfo.depth == 8 )
    {
        XColor colors[ 256 ];

        for( i = 0 ; i < 256 ; i++ )
        {
            colors[ i ].pixel = i;
            colors[ i ].flags = DoRed | DoGreen | DoBlue;

            c = gammatable[usegamma][*palette++];
            colors[ i ].red = (c << 8) + c;
            
            c = gammatable[usegamma][*palette++];
            colors[ i ].green = (c << 8) + c;

            c = gammatable[usegamma][*palette++];
            colors[ i ].blue = (c << 8) + c;
        }

        XStoreColors( display(), _colormap, colors, 256 );
    }
    else
    {
        XColor color;

        // It looks like we could get away with caching as little as
        // three palettes to avoid most re-allocations.

        // Must update the entire screen when palette changes in truecolor mode.
        UpdateState |= I_FULLSCRN;

#ifdef PAL_CACHE_ON
        if( usegamma != _cacheGamma )
        {
            // The gamma has changed so the entire cache must be thrown out.
            for( i = 0; i < MAX_PAL_CACHE; i++ )
                _palc[ i ].free( display(), _colormap );
            _cacheAge = 0;
            _cacheGamma = usegamma;
            //printf( "palette gamma\n" );
        }

        // Search for palette in cache.
        for( i = 0; i < MAX_PAL_CACHE; i++ )
        {
            if( _palc[ i ].id == palette )
            {
                // Found palette.
                _pcurrent = &_palc[ i ];
                _pcurrent->used = ++_cacheAge;

                //printf( "palette %d %p - cache\n", i, palette );

                return;
            }
        }
#endif

        // Search for unused cache.  Otherwise use the oldest cache.
        _pcurrent = &_palc[ 0 ];

#ifdef PAL_CACHE_ON
        for( i = 0; i < MAX_PAL_CACHE; i++ )
        {
            if( ! _palc[ i ].id )
            {
                // Unused cache.
                _pcurrent = &_palc[ i ];
                break;
            }
            if( _palc[ i ].used < _pcurrent->used )
                _pcurrent = &_palc[ i ];
        }
#endif

        //printf( "palette %d %p - new\n", i, palette );

        _pcurrent->free( display(), _colormap );
        _pcurrent->used = ++_cacheAge;
        _pcurrent->id = palette;

        for( i = 0 ; i < 256 ; i++ )
        {
            color.pixel = 0;
            color.flags = DoRed | DoGreen | DoBlue;

            c = gammatable[usegamma][*palette++];
            color.red = (c << 8);
            
            c = gammatable[usegamma][*palette++];
            color.green = (c << 8);

            c = gammatable[usegamma][*palette++];
            color.blue = (c << 8);

            if( ! XAllocColor( display(), _colormap, &color ) )
            {
                fprintf( stderr," XAllocColor failed\n" );
            }

            _pcurrent->pixel[ i ] = color.pixel;
        }
    }
}


// Predicate procedure used by waitOnShmPut()

struct PPArg
{
    PPArg( Window w, int type ) : window( w ), shmCompletionType( type ) {}

    Window window;
    int shmCompletionType;
};

static Bool _shmPredicateProc( Display* dis, XEvent* event, XPointer arg )
{
    if( (event->xany.window == ((PPArg*)arg)->window) &&
        (event->type == ((PPArg*)arg)->shmCompletionType) )
    {
        return True;
    }
    return False;
}

void HexenWindow::waitForShmPut()
{
    XEvent event;
    PPArg arg( window(), _shmEventType );
    XIfEvent( display(), &event, _shmPredicateProc, (XPointer) &arg );
}


typedef unsigned char   pixel8;
typedef unsigned short  pixel16;
typedef unsigned long   pixel32;

static void blit_8_8( pixel8* source, XImage* img,
                      int x, int y, int w, int h )
{
    register pixel8* begin;

    begin = (pixel8*) img->data;
    begin += x + (y * img->bytes_per_line);
    while( h-- )
    {
        memcpy( begin, source, w );
        begin += img->bytes_per_line;
        source += w;
    }
}


static void blit_double_8_8( pixel8* source, XImage* img,
                             int x, int y, int w, int h )
{
    pixel8 p;
    pixel8* begin;
    pixel8* end;
    register pixel8* dst;
    register pixel8* src;

    begin = (pixel8*) img->data;
    begin += (x * 2) + (y * img->bytes_per_line * 2);
    src = source;
    while( h-- )
    {
        dst = begin;
        end = dst + (w * 2);
        while( dst != end )
        {
            p = *src++;
            *dst++ = p;
            *dst++ = p;
        }

        memcpy( begin + img->bytes_per_line, begin, w * 2 );

        begin += img->bytes_per_line * 2;
    }
}


static void blit_8_16( pixel8* source, XImage* img,
                       int x, int y, int w, int h,
                       unsigned long* pixel )
{
    pixel16* begin;
    pixel16* end;
    register pixel16* dst;
    register pixel8* src;
         
    begin = (pixel16*) img->data;
    begin += x + (y * img->bytes_per_line / 2);
    src = source;
    while( h-- )
    {
        dst = begin;
        end = dst + w;
        while( dst != end )
            *dst++ = pixel[ *src++ ];
        begin += img->bytes_per_line / 2;
    }
}


static void blit_double_8_16( pixel8* source, XImage* img,
                             int x, int y, int w, int h,
                             unsigned long* pixel )
{
    pixel16 p;
    pixel16* begin;
    pixel16* end;
    register pixel16* dst;
    register pixel8* src;
         
    begin = (pixel16*) img->data;
    begin += (x * 2) + (y * img->bytes_per_line);
    src = source;
    while( h-- )
    {
        dst = begin;
        end = dst + (w * 2);
        while( dst != end )
        {
            p = pixel[ *src++ ];
            *dst++ = p;
            *dst++ = p;
        }

        dst = begin + (img->bytes_per_line / 2);
        memcpy( dst, begin, w * 4 );

        begin += img->bytes_per_line;
    }
}


static void blit_8_32( pixel8* source, XImage* img,
                       int x, int y, int w, int h,
                       unsigned long* pixel )
{
    pixel32* begin;
    pixel32* end;
    register pixel32* dst;
    register unsigned char* src;
         
    begin = (pixel32*) img->data;
    begin += x + (y * img->bytes_per_line / 4);
    src = source;
    while( h-- )
    {
        dst = begin;
        end = dst + w;
        while( dst != end )
            *dst++ = pixel[ *src++ ];
        begin += img->bytes_per_line / 4;
    }
}


static void blit_double_8_32( pixel8* source, XImage* img,
                             int x, int y, int w, int h,
                             unsigned long* pixel )
{
    pixel32 p;
    pixel32* begin;
    pixel32* end;
    register pixel32* dst;
    register pixel8* src;
         
    begin = (pixel32*) img->data;
    begin += (x * 2) + (y * img->bytes_per_line / 2);
    src = source;
    while( h-- )
    {
        dst = begin;
        end = dst + (w * 2);
        while( dst != end )
        {
            p = pixel[ *src++ ];
            *dst++ = p;
            *dst++ = p;
        }

        dst = begin + (img->bytes_per_line / 4);
        memcpy( dst, begin, w * 8 );

        begin += img->bytes_per_line / 2;
    }
}


void HexenWindow::blit( unsigned char* buffer, int x, int y, int w, int h )
{
    buffer += x + (y * w);

    if( (width() >= SCREENWIDTH*2) && (height() >= SCREENHEIGHT*2) )
    {
        if( _vinfo.depth == 8 )
        {
            blit_double_8_8( buffer, _ximage, x, y, w, h );
        }
        else if( _vinfo.depth <= 16 )
        {
            blit_double_8_16( buffer, _ximage, x, y, w, h, _pcurrent->pixel );
        }
        else
        {
            blit_double_8_32( buffer, _ximage, x, y, w, h, _pcurrent->pixel );
        }
        x *= 2;
        y *= 2;
        w *= 2;
        h *= 2;
    }
    else
    {
        if( _vinfo.depth == 8 )
        {
            blit_8_8( buffer, _ximage, x, y, w, h );
        }
        else if( _vinfo.depth <= 16 )
        {
            blit_8_16( buffer, _ximage, x, y, w, h, _pcurrent->pixel );
        }
        else
        {
            blit_8_32( buffer, _ximage, x, y, w, h, _pcurrent->pixel );
        }
    }

    if( _usingShm )
    {
        XShmPutImage( display(), window(), _gc, _ximage, x, y, x, y,
                      w, h, True );
        waitForShmPut();
    }
    else
    {
        XPutImage( display(), window(), _gc, _ximage, x, y, x, y, w, h );
        sync();
    }
}


void HexenWindow::resizeFramebuffer()
{
    if(_ximage && (_ximage->width == width()) && (_ximage->height == height()))
    {
       return; 
    }

    if( _usingShm )
    {
        delete _simage;
        _simage = new ShmImage( display(), width(), height(), &_vinfo );
        _ximage = _simage->image();
    }
    else
    {
        if( _ximage )
        {
            delete _ximage->data;
            XDestroyImage( _ximage );
        }
        _ximage = XCreateImage( display(), _vinfo.visual, _vinfo.depth,
                                ZPixmap, 0, 0,
                                width(), height(),
                                (_vinfo.depth == 8) ? 8 : 32, 0 );
        _ximage->data = new char[ _ximage->bytes_per_line * _ximage->height ];
    }

    UpdateState |= I_FULLSCRN;
}


bool HexenWindow::getVisualInfo( XVisualInfo* vi )
{
    if( XMatchVisualInfo( display(), screen(), 8, PseudoColor, vi ) )
        return true;

    if( XMatchVisualInfo( display(), screen(), 16, TrueColor, vi ) )
        return true;
    if( XMatchVisualInfo( display(), screen(), 15, TrueColor, vi ) )
        return true;

    if( XMatchVisualInfo( display(), screen(), 32, TrueColor, vi ) )
        return true;
    if( XMatchVisualInfo( display(), screen(), 24, TrueColor, vi ) )
        return true;

#if 0
    if( XMatchVisualInfo( display(), screen(), 8, GrayScale, vi ) )
        return True;
#endif
 
    return false;
}


void HexenWindow::deleteEvent( XEvent* )
{
    I_Quit();
}

void HexenWindow::configureEvent( XConfigureEvent* e )
{
    resizeFramebuffer();
    //printf( "configure %d %d\n", width(), height() );
}

void HexenWindow::unknownEvent( XEvent* e )
{
    //printf( "Unknown XEvent: %d\n", e->type );
}


void HexenWindow::buttonDown( XButtonEvent* e )
{
    //printf( "buttonDown: %d %d,%d\n", e->button, e->x, e->y );

//    if( ! _grabCursor )
//        grabPointer();

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
    //printf( "buttonUp: %d %d,%d\n", e->button, e->x, e->y );

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
    
    if(e->x == width()/2 && e->y == height()/2)
    {
	_prevX = width()/2;
	_prevY = height()/2;
	return;
    }
    dx = (e->x - _prevX);
    _prevX = e->x;
    dy = (e->y - _prevY);
    _prevY = e->y;

    if( dx || dy )
    {
        postMouseEvent( dx, dy );

        if (_grabCursor)
        {

            if( (e->x < MOUSE_JUMP_AT) || (e->x > (width() - MOUSE_JUMP_AT)) ||
                (e->y < MOUSE_JUMP_AT) || (e->y > (height() - MOUSE_JUMP_AT)) )
            {
                XWarpPointer( display(), None, window(), 0, 0, 0, 0,
                              width() / 2, height() / 2 );
		_prevX = width()/2; _prevY = height()/2;
            }
	    else
            {
               postMouseEvent( dx, dy);
            }
        }
	else
        {
          postMouseEvent( dx, dy);
        }
  }
}


void HexenWindow::keyDown( XKeyEvent* e )
{
    KeySym key = keysym( e );

    //TODO: filter key repeat.

    //printf( "keyDown: %lx %x\n", e->time, key );

    if( e->state & Mod1Mask )   // Control key defaults to attack.
    {
        if( key == XK_d )
        {
            if( width() == SCREENWIDTH )
            {
                resize( SCREENWIDTH * 2, SCREENHEIGHT * 2 );
	    }
            else
            {
                resize( SCREENWIDTH, SCREENHEIGHT );
            }
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
#if 0
    else if( key == XK_Escape )
    {
        I_Quit();
    }
#endif
    else
    {
        postKey( ev_keydown, key );
    }
}


void HexenWindow::keyUp( XKeyEvent* e )
{
    //printf( "keyUp: %lx %x %x\n", e->time, e->state, e->keycode );

    postKey( ev_keyup, keysym( e ) );
}


void HexenWindow::exposeEvent( XExposeEvent* )
{
    //printf( "expose\n" );
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


//EOF
