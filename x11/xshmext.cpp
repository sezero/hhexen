//============================================================================
//
// $Id: xshmext.cpp,v 1.1.1.1 2000-04-11 17:38:20 theoddone33 Exp $
//
// MIT Shared Memory Extension for X
//
//============================================================================


#include <stdio.h>
#include "xshmext.h"
#include <X11/Xutil.h>


//-----------------------------------------------------------------------------


// Shared memory error handler routine used temporarily by allocSHM().

static int _shmError;

static int (*_origErrorHandler)(Display*, XErrorEvent*);

static int _shmErrorHandler( Display* d, XErrorEvent* e )
{
    _shmError++;
    if( e->error_code == BadAccess )
        fprintf( stderr, "ShmImage: failed to attach shared memory\n" );
    else
        (*_origErrorHandler)( d, e );
    return 0;
} 


static void* allocSHM( XShmSegmentInfo* si, Display* dis, int size )
{
    si->shmaddr = 0;
    si->shmid = shmget( IPC_PRIVATE, size, IPC_CREAT | 0777 );
    if( si->shmid != -1 )
    {
        si->shmaddr = (char*) shmat( si->shmid, 0, 0 );
        if( si->shmaddr != (char*) -1 )
        {
            si->readOnly = False;

            // Attach the memory to the X Server.

            _shmError = 0;
            _origErrorHandler = XSetErrorHandler( _shmErrorHandler );
            XShmAttach( dis, si );
            XSync( dis, True );    // wait for error or ok
            XSetErrorHandler( _origErrorHandler );
            if( _shmError )
            {
                shmdt( si->shmaddr );
                shmctl( si->shmid, IPC_RMID, 0 );
                si->shmaddr = 0;
            }
        }
        else
        {
            shmctl( si->shmid, IPC_RMID, 0 );
            si->shmaddr = 0;
        }
    }

    return si->shmaddr;
}


static void freeSHM( XShmSegmentInfo* si, Display* dis )
{
    if( si->shmaddr )
    {
        XShmDetach( dis, si );
        //XSync( dis, False );//need server to detach so can remove id?

        shmdt( si->shmaddr );
        shmctl( si->shmid, IPC_RMID, 0 );
    }
}


static int bytesPerLine( int width, int depth )
{
    int bpl;

    // TODO: Find out how to correctly calculate a Pixmap bytesPerLine that is
    // guaranteed to be accurate on any X server.
    // The important thing is that we don't calculate a value less that what
    // is actually used by the server.

    if( depth < 9 )
    {
        if( depth == 1 )
            bpl = (width + 7) / 8;
        else
            bpl = width;
    }
    else
    {
        if( depth > 16 )
            bpl = width * 4;
        else
            bpl = width * 2;
    }

    // Pad to 4 byte boundary.
    if( bpl & 3 )
        bpl += (4 - (bpl & 3));

    return bpl;
}


//-----------------------------------------------------------------------------


/**
  \class ShmImage xshmext.h
  \brief The ShmImage class is an X11 XImage allocated in shared memory.
*/


/**
  \fn XImage* ShmImage::image()
  Returns 0 if the constructor failed.
*/


/**
  Check to see if the XShm extensions are supported.
  Returns the XEvent completion type or zero if XShm is not available to
  the display.
*/

int ShmImage::query( Display* dis )
{
    if( ! XShmQueryExtension( dis ) )
        return 0;

    return completionType( dis );
}


ShmImage::ShmImage( Display* dis, int width, int height, XVisualInfo* vis )
{
    _display = dis;

    // Query here for safety?

    _XImage = XShmCreateImage( _display, vis->visual, vis->depth, ZPixmap,
                               NULL, &_si, width, height );
    if( _XImage )
    {
        _XImage->data = (char*) allocSHM( &_si, _display,
                                  _XImage->bytes_per_line * _XImage->height );
        if( ! _XImage->data )
        {
            XDestroyImage( _XImage );
            _XImage = 0;
        }
    }
}


ShmImage::~ShmImage()
{
    if( _si.shmaddr )
        freeSHM( &_si, _display );

    if( _XImage )
        XDestroyImage( _XImage );
}


//-----------------------------------------------------------------------------


/**
  \class ShmPixmap xshmext.h
  \brief The ShmPixmap class is an X11 Pixmap allocated in shared memory.
*/


/**
  \fn Pixmap* ShmPixmap::pixmap()
  Returns 0 if the constructor failed.
*/


/**
  Check to see if the XShmPixmap extensions are supported.
  Returns the XEvent completion type or zero if XShm is not available to
  the display.
*/

int ShmPixmap::query( Display* dis )
{
    if( ! XShmPixmapFormat( dis ) )
        return 0;

    return ShmImage::query( dis );
}


ShmPixmap::ShmPixmap( Display* dis, Drawable draw, int width, int height,
                      int depth )
{
    _display = dis;

    // Query here for safety?

    if( depth < 1 )
        depth = XDefaultDepth( _display, XDefaultScreen( _display ) );

    allocSHM( &_si, _display, bytesPerLine( width, depth ) * height );
    if( _si.shmaddr )
    {
        _pix = XShmCreatePixmap( _display, draw, _si.shmaddr, &_si,
                                 width, height, depth );
    }
}


ShmPixmap::~ShmPixmap()
{
    if( _si.shmaddr )
        freeSHM( &_si, _display );

    if( _pix )
        XFreePixmap( _display, _pix );
}


// EOF
