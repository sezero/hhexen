
//**************************************************************************
//**
//** VGAView.h : Heretic 2 : Raven Software, Corp.
//**
//** $RCSfile: vgaview.h,v $
//** $Revision: 1.2 $
//** $Date: 2008-06-17 09:20:20 $
//** $Author: sezero $
//**
//**************************************************************************

#import <appkit/appkit.h>
#import "h2def.h"

// a few globals
extern byte	*bytebuffer;


@interface VGAView:View
{
    id		game;
    int		nextpalette[256];	// color lookup table
    int		*nextimage;		// palette expanded and scaled
    unsigned	scale;
    NXWindowDepth	depth;
}

- updateView;
- (unsigned)scale;
- setPalette:(byte *)pal;
- setScale:(int)newscale;

@end

