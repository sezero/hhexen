//============================================================================
//
// $Id: x11window.cpp,v 1.1.1.1 2000-04-11 17:38:20 theoddone33 Exp $
//
//============================================================================


#include "x11window.h"


/**
  \class X11Window x11window.h
  \brief The X11Window class

  Note that most methods will not take effect until a call to XPending,
  XNextEvent, or XWindowEvent.  If you want to make sure a method takes
  effect before your program continues use sync();
*/


char* X11Window::ErrorMessage[] =
{
    "No error",
    "Can't open X display"
};


/**
   The specified name is used as the class and window/icon property name.
*/

X11Window::X11Window( const char* name, Display* dis, int scr,
                      int swidth, int sheight, long inputMask )
{
    _flags = 0;
    _err = 0;
    _win = 0;
    _screen = 0;
    _nullCursor = (Cursor) -1;

    if( dis )
    {
        _display = dis;
    }
    else
    {
        _display = XOpenDisplay( 0 );
        if( ! _display )
        {
            _err = kOpenDisplay;
            return;
        }
        _set( kOpenedDisplay );
	}

    if( scr )
    {
        _screen = scr;
    }
    else
    {
        _screen = DefaultScreen( _display );
    }

    _width = swidth;
    _height = sheight;

    unsigned long black = BlackPixel( _display, _screen );
	_win = XCreateSimpleWindow( _display, DefaultRootWindow( _display ),
				                0, 0, _width, _height, 0, black, black );


  	// Enable the delete window protocol.

	_wmDeleteAtom = XInternAtom( _display, "WM_DELETE_WINDOW", False );
	XSetWMProtocols( _display, _win, &_wmDeleteAtom, 1 );


#if 0
    // Set the window manager properties

    XWMHints wmHints;
    XClassHint classHints;
    XSizeHints sizeHints;
    XTextProperty nameProp;
	char* argv[ 2 ] = { (char*) name, NULL };

    wmHints.flags = InputHint | StateHint;
    wmHints.input = True;     
    wmHints.initial_state = NormalState;

    sizeHints.flags  = PSize;
    sizeHints.width  = _width;
    sizeHints.height = _height;

    classHints.res_name  = (char*) name;
    classHints.res_class = (char*) name;

    if( XStringListToTextProperty( (char**) &name, 1, &nameProp ) )
    {
        XSetWMProperties( _display, _win, &nameProp, &nameProp, argv, 1,
                          &sizeHints, &wmHints, &classHints );
    }
#endif

	// Set up the events to wait for.
	XSelectInput( _display, _win, inputMask );
}


X11Window::~X11Window()
{
	if( _display )
    {
        if( _win )
        {
            if( _nullCursor != (Cursor) -1 )
                XFreeCursor( _display, _nullCursor );

            XDestroyWindow( _display, _win );
        }

        if( _flags & kOpenedDisplay )
        {
            XCloseDisplay( _display );
        }
    }
}


void X11Window::setTitle( const char* title )
{
    XStoreName( _display, _win, title );
}


void X11Window::setIconName( const char* text )
{
    XSetIconName( _display, _win, text );
}


void X11Window::move( int x, int y )
{
    XMoveWindow( _display, _win, x, y );
}


void X11Window::moveResize( int x, int y, unsigned int w, unsigned int h )
{
    XMoveResizeWindow( _display, _win, x, y, w, h );
}


void X11Window::resize( unsigned int w, unsigned int h )
{
    XResizeWindow( _display, _win, w, h );
}


void X11Window::setSizeHints( int minW, int minH, int maxW, int maxH )
{
    XSizeHints* hints = XAllocSizeHints();
    if( hints )
    {
        hints->flags      = PMinSize | PMaxSize;
        hints->min_width  = minW;
        hints->min_height = minH;
        hints->max_width  = maxW;
        hints->max_height = maxH;

        XSetWMNormalHints( _display, _win, hints );
        XFree( hints );
    }
}


void X11Window::raise()
{
    XRaiseWindow( _display, _win );
}


void X11Window::show()
{
    XMapWindow( _display, _win );

#if 0
    // Wait for the window to be mapped.
	XEvent event;
    do
    {
        XNextEvent( _display, &event );
    }
    while( event.type != MapNotify );
#endif
}


void X11Window::hide()
{
    XUnmapWindow( _display, _win );
}


void X11Window::iconify()
{
    /*
    XWMHints* hints = XAllocWMHints();
    if( hints )
    {
        hints->flags = StateHint;
        hints->initial_state = IconicState;
        XSetWMHints( _display, _win, hints );
        XFree(hints);

        XIconifyWindow( _display, _win, _screen );
    }
    */

    XIconifyWindow( _display, _win, _screen );
}


void X11Window::unIconify()
{
    /*
    XWMHints* hints = XAllocWMHints();
    if( hints )
    {
        hints->flags = StateHint;
        hints->initial_state = NormalState;
        XSetWMHints( _display, _win, hints );
        XFree(hints);

        show();
    }
    */

    show();
}


int X11Window::isIconified()
{
    return 0;
}


/**
  Calls XNextEvent() and then the appropriate virtual funtion based on event
  type.
*/

void X11Window::handleNextEvent()
{
    XEvent event;

    XNextEvent( _display, &event );

    switch( event.type )
    {
        case ClientMessage:
            if( event.xclient.format == 32 && 
                event.xclient.data.l[ 0 ] == (int) _wmDeleteAtom )
            {
                deleteEvent( &event );
            }
            else
            {
                unknownEvent( &event );
            }
            break;

        case ConfigureNotify:
            if( event.xconfigure.window == _win )
            {
                _width  = event.xconfigure.width;
                _height = event.xconfigure.height;
                configureEvent( &event.xconfigure );
            }
            break;
                                                
        case ButtonPress:
            buttonDown( &event.xbutton );
            break;

        case ButtonRelease:
            buttonUp( &event.xbutton );
            break;

        case MotionNotify:
            motionEvent( &event.xmotion );
            break;
                                    
        case KeyPress:
            keyDown( &event.xkey );
            break;

        case KeyRelease:
            keyUp( &event.xkey );
            break;

        case FocusIn:
            focusIn( &event.xfocus );
            break;

        case FocusOut:
            focusOut( &event.xfocus );
            break;

        case Expose:
            exposeEvent( &event.xexpose );
            break;

        default:
            unknownEvent( &event );
            break;
    }
}


/**
  All events not passed to other virtual event methods are sent here.
*/

void X11Window::unknownEvent( XEvent* ) {}


void X11Window::deleteEvent( XEvent* ) {}

void X11Window::focusIn( XFocusChangeEvent* ) {}

void X11Window::focusOut( XFocusChangeEvent* ) {}

void X11Window::exposeEvent( XExposeEvent* ) {}


/**
  Called when the window is moved or resized by the user.
  width() and height() can be used to check the new size.
*/

void X11Window::configureEvent( XConfigureEvent* ) {}


/**
  From XButtonEvent structure in Xlib.h:

  \code
      int x, y;             // pointer x, y coordinates in event window
      int x_root, y_root;   // coordinates relative to root
      unsigned int state;   // key or button mask
      unsigned int button;  // detail
  \endcode
*/

void X11Window::buttonDown( XButtonEvent* ) {}
void X11Window::buttonUp( XButtonEvent* ) {}


void X11Window::motionEvent( XMotionEvent* ) {}


void X11Window::keyDown( XKeyEvent* ) {}
void X11Window::keyUp( XKeyEvent* ) {}


KeySym X11Window::keysym( XKeyEvent* e )
{
    return XKeycodeToKeysym( display(), e->keycode, 0 );

    /*
    KeySym sym;
    char buf[ 2 ];

    XLookupString( event, buf, 1, &sym, NULL );
    //printf( "XLookupString %x: %x %x\n", event->keycode, buf[ 0 ], sym );
    return sym;
    */
}


void X11Window::hideCursor()
{
    if( _nullCursor == (Cursor) -1 )
        _nullCursor = _createNullCursor();

    XDefineCursor( _display, _win, _nullCursor );
}


void X11Window::showCursor()
{
    XUndefineCursor( _display, _win );
}


Cursor X11Window::_createNullCursor()
{
    Pixmap cursormask;
    XGCValues xgc;
    GC gc;
    XColor dummycolour;
    Cursor cursor;

    cursormask = XCreatePixmap( _display, _win, 1, 1, 1/*depth*/);
    xgc.function = GXclear;
    gc = XCreateGC( _display, cursormask, GCFunction, &xgc );
    XFillRectangle( _display, cursormask, gc, 0, 0, 1, 1 );
    dummycolour.pixel = 0;
    dummycolour.red   = 0;
    dummycolour.flags = 04;
    cursor = XCreatePixmapCursor( _display, cursormask, cursormask,
                                  &dummycolour, &dummycolour, 0, 0 );
    XFreePixmap( _display, cursormask );
    XFreeGC( _display, gc );

    return cursor;
}


#if 0
void X11Window::enableBackingStore()
{
    if( DoesBackingStore( ScreenOfDisplay( _display, _screen ) ) == Always )
    {
        XSetWindowAttributes attr;
        winattr.backing_store = Always;
        XChangeWindowAttributes( _display, _win, CWBackingStore, &attr );
    }
}
#endif


#if 0
// g++ x11window.cpp -g -L/usr/X11R6/lib -lX11

#include <stdio.h>
#include <unistd.h>
#include <ctype.h>

class TestWindow : public X11Window
{
public:

    TestWindow( const char* name, Display* dis = 0 )
              : X11Window( name, dis )
    {
        setTitle( name );
    }

    void wait()
    {
        sync();

        sleep( 1 );
        putchar( '.' );
        fflush( stdout );

        sleep( 1 );
        putchar( '.' );
        putchar( '\n' );
        fflush( stdout );
    }

    void testHide()
    {
        wait();

        printf( "resize\n" );
        resize( 100, 200 );
        wait();

        printf( "hiding\n" );
        hide();
        wait();
        printf( "showing\n" );
        show();
        wait();

        printf( "iconifying\n" );
        iconify();
        wait();
        printf( "showing\n" );
        show();
        wait();
    }
    
    void testEvents()
    {
        _exit = false;
        while( _exit == false )
        {
            _ecnt = 0;

            while( eventsPending() )
                handleNextEvent();

            if( _ecnt )
                printf( "  events %d\n", _ecnt );

            usleep( 16000 );
        }
    }

protected:
    
    void deleteEvent( XEvent* )
    {
        printf( "delete\n" );
        _exit = true;
        _ecnt++;
    }

    void configureEvent( XConfigureEvent* )
    {
        printf( "configure %d %d\n", width(), height() );
        _ecnt++;
    }

    void unknownEvent( XEvent* e )
    {
        printf( "Unknown XEvent: %d\n", e->type );
        _ecnt++;
    }
    
    void buttonDown( XButtonEvent* e )
    {
        printf( "buttonDown: %d %d,%d\n", e->button, e->x, e->y );
        _ecnt++;
    }

    void buttonUp( XButtonEvent* e )
    {
        printf( "buttonUp: %d %d,%d\n", e->button, e->x, e->y );
        _ecnt++;
    }

    void motionEvent( XMotionEvent* e )
    {
        printf( "motion: %lx %d %d\n", e->time, e->x, e->y );
        _ecnt++;
    }

    void keyDown( XKeyEvent* e )
    {
        int a = keysym( e );
        if( isascii( a ) )
            printf( "keyDown: %lx %x %x (%c)\n", e->time, e->state, e->keycode, (char) a );
        else
            printf( "keyDown: %lx %x %x %x\n", e->time, e->state, e->keycode, a );
        _ecnt++;

        if( a == 0x1b ) // ASCII ESC 
            _exit = true;
        if( a == 'h' )
            hideCursor();
        if( a == 'u' )
            showCursor();
    }

    void keyUp( XKeyEvent* e )
    {
        int a = keysym( e );
        if( isascii( a ) )
            printf( "keyUp: %lx %x %x (%c)\n", e->time, e->state, e->keycode, (char) a );
        else
            printf( "keyUp: %lx %x %x %x\n", e->time, e->state, e->keycode, a );
        _ecnt++;
    }

private:
    
    int _ecnt;
    int _exit;
};


int main()
{
    TestWindow win( "X11Window Test" );

    if( win.error() )
    {
        printf( "error %d\n", win.error() );
        return 1;
    }

    printf( "sizeof X11Window: %d\n", sizeof( X11Window ) );
    printf( "screen size: %d %d\n", win.displayWidth(), win.displayHeight() );

    win.show();

    win.testEvents();
    //win.testHide();

    printf( "done!\n" );
    return 0;
}
#endif


//EOF
