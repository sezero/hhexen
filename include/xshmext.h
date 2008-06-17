#ifndef XSHMEXT_H
#define XSHMEXT_H
//============================================================================
//
// $Id: xshmext.h,v 1.2 2008-06-17 09:20:21 sezero Exp $
//
// MIT Shared Memory Extension for X
//
//============================================================================


#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>


// Not in any Linux header file...
extern "C" {
    //extern int XShmQueryExtension( Display* );
    extern int XShmGetEventBase( Display* );
}


class ShmImage
{
public:

	static int query( Display* );

    static int completionType( Display* dis )
    {
        return XShmGetEventBase( dis ) + ShmCompletion;
    }

    ShmImage( Display*, int width, int height, XVisualInfo* );

    ~ShmImage();

    XImage* image() { return _XImage; }
        
    void put( Drawable d, GC gc )
    {
        XShmPutImage( _display, d, gc, _XImage, 0, 0, 0, 0,
                      _XImage->width, _XImage->height, False );
    }

private:

    XImage* _XImage;
    Display* _display;
    XShmSegmentInfo _si;
};


class ShmPixmap
{
public:

	static int query( Display* );

    ShmPixmap( Display*, Drawable, int width, int height, int depth = 0 );

    ~ShmPixmap();

    Pixmap pixmap() { return _pix; }

private:
    
    Pixmap _pix;
    Display* _display;
    XShmSegmentInfo _si;
};


#endif // XSHMEXT_H

