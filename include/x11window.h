#ifndef X11WINDOW_H
#define X11WINDOW_H
//============================================================================
//
// $Id: x11window.h,v 1.2 2000-04-14 23:18:32 theoddone33 Exp $
//
// X11 Pixel Port class
//
//============================================================================


#include <X11/Xlib.h>
#include <X11/Xutil.h>
//#include <X11/xpm.h>


const long X11WindowDefaultInput = KeyPressMask | KeyReleaseMask |
                                   ButtonPressMask | ButtonReleaseMask |
                                   PointerMotionMask |
                                   ExposureMask | StructureNotifyMask;

class X11Window
{
public:

    X11Window( const char* name, Display* dis = 0, int scr = 0,
               int swidth = 320, int sheight = 200, 
               long inputMask = X11WindowDefaultInput );

    virtual ~X11Window();

    enum eError
    {
        kNone,
        kOpenDisplay
    };

    int error() const { return _err; }

    char* errorStr() const { return ErrorMessage[ _err ]; }

    Display* display() { return _display; }

    int screen() { return _screen; }

    Window window() { return _win; }

    void setTitle( const char* text );
    void setIconName( const char* text );

    int width() { return _width; }
    int height() { return _height; }

    int displayWidth() { return XDisplayWidth( _display, _screen ); }
    int displayHeight() { return XDisplayHeight( _display, _screen ); }

    void move( int x, int y );
    void moveResize( int x, int y, unsigned int w, unsigned int h );
    void resize( unsigned int newWidth, unsigned int newHeight );
    void setSizeHints( int minW, int minH, int maxW, int maxH );

    void raise();

    void show();
    void hide();
    //int isMapped();

    void iconify();
    void unIconify();
    int isIconified();

    int eventsPending() { return XPending( _display ); }
    void handleNextEvent();

    void flush() { XFlush( _display ); }
    void sync( Bool discard = False ) { XSync( _display, discard ); }

    void hideCursor();
    void showCursor();

    KeySym keysym( XKeyEvent* event );

protected:

    // These virtual event methods are called during handleNextEvent()

    virtual void unknownEvent( XEvent* );
    virtual void deleteEvent( XEvent* );
    virtual void configureEvent( XConfigureEvent* );
    virtual void buttonDown( XButtonEvent* );
    virtual void buttonUp( XButtonEvent* );
    virtual void motionEvent( XMotionEvent* );
    virtual void keyDown( XKeyEvent* );
    virtual void keyUp( XKeyEvent* );
    virtual void focusIn( XFocusChangeEvent* );
    virtual void focusOut( XFocusChangeEvent* );
    virtual void exposeEvent( XExposeEvent* );

private:

    Cursor _createNullCursor();
    Cursor _nullCursor;

    int _err;
    static char* ErrorMessage[];

    void _set( int mask ) { _flags |= mask; }
    void _clr( int mask ) { _flags &= ~mask; }                                  

    enum eFlags
    {
        kOpenedDisplay = 0x0001,
    };

    Display* _display;			// The X11 display connection
    int _screen;
    Window _win;			    // The X11 window on the display
    Atom _wmDeleteAtom;

    //int _screenDepth;
    //GC _gc;				        // The window's current graphics context

    int _flags;
    int _width;
    int _height;

};


#endif // X11WINDOW_H
