
//**************************************************************************
//**
//** r_main.c : Heretic 2 : Raven Software, Corp.
//**
//** $Revision$
//** $Date$
//**
//** (ogl_main.c is r_main.c with necessary clean-ups for ogl)
//**
//**************************************************************************

#include "h2stdinc.h"
#include <math.h>
#include "h2def.h"
#include "r_local.h"
#include "ogl_def.h"

extern void R_RenderMap(void);
extern void R_DrawPlayerSprites(void);

int			viewangleoffset;

int			validcount = 1;		// increment every time a check is made

lighttable_t		*fixedcolormap;

int			centerx, centery;
fixed_t			centerxfrac, centeryfrac;
fixed_t			projection;

int			framecount;		// just for profiling purposes

int			sscount, linecount, loopcount;

fixed_t			viewx, viewy, viewz;
angle_t			viewangle;
fixed_t			viewcos, viewsin;
player_t		*viewplayer;
float			viewpitch;		// player->lookdir, global version

int			detailshift;		// 0 = high, 1 = low

//
// precalculated math tables
//
fixed_t			*finecosine = &finesine[FINEANGLES/4];

int			extralight;		// bumped light from gun blasts


/*
===============================================================================
=
= R_PointOnSide
=
= Returns side 0 (front) or 1 (back)
===============================================================================
*/

int	R_PointOnSide (fixed_t x, fixed_t y, node_t *node)
{
	fixed_t	dx, dy;
	fixed_t	left, right;

	if (!node->dx)
	{
		if (x <= node->x)
			return node->dy > 0;
		return node->dy < 0;
	}
	if (!node->dy)
	{
		if (y <= node->y)
			return node->dx < 0;
		return node->dx > 0;
	}

	dx = (x - node->x);
	dy = (y - node->y);

// try to quickly decide by looking at sign bits
	if ((node->dy ^ node->dx ^ dx ^ dy) & 0x80000000)
	{
		if ((node->dy ^ dx) & 0x80000000)
			return 1;	// (left is negative)
		return 0;
	}

	left = FixedMul (node->dy>>FRACBITS , dx);
	right = FixedMul (dy , node->dx>>FRACBITS);

	if (right < left)
		return 0;		// front side
	return 1;			// back side
}


int	R_PointOnSegSide (fixed_t x, fixed_t y, seg_t *line)
{
	fixed_t	lx, ly;
	fixed_t	ldx, ldy;
	fixed_t	dx, dy;
	fixed_t	left, right;

	lx = line->v1->x;
	ly = line->v1->y;

	ldx = line->v2->x - lx;
	ldy = line->v2->y - ly;

	if (!ldx)
	{
		if (x <= lx)
			return ldy > 0;
		return ldy < 0;
	}
	if (!ldy)
	{
		if (y <= ly)
			return ldx < 0;
		return ldx > 0;
	}

	dx = (x - lx);
	dy = (y - ly);

// try to quickly decide by looking at sign bits
	if ((ldy ^ ldx ^ dx ^ dy) & 0x80000000)
	{
		if ((ldy ^ dx) & 0x80000000)
			return 1;	// (left is negative)
		return 0;
	}

	left = FixedMul (ldy>>FRACBITS, dx);
	right = FixedMul (dy, ldx>>FRACBITS);

	if (right < left)
		return 0;		// front side
	return 1;			// back side
}


/*
===============================================================================
=
= R_PointToAngle
=
===============================================================================
*/

// to get a global angle from cartesian coordinates, the coordinates are
// flipped until they are in the first octant of the coordinate system, then
// the y (<=x) is scaled and divided by x to get a tangent (slope) value
// which is looked up in the tantoangle[] table.  The +1 size is to handle
// the case when x==y without additional checking.
#define	SLOPERANGE	2048
#define	SLOPEBITS	11
#define	DBITS		(FRACBITS-SLOPEBITS)

extern	int	tantoangle[SLOPERANGE+1];		// get from tables.c

//int	tantoangle[SLOPERANGE+1];

static int SlopeDiv (unsigned num, unsigned den)
{
	unsigned ans;
	if (den < 512)
		return SLOPERANGE;
	ans = (num<<3) / (den>>8);
	return ans <= SLOPERANGE ? ans : SLOPERANGE;
}

angle_t R_PointToAngle (fixed_t x, fixed_t y)
{
	x -= viewx;
	y -= viewy;
	if ( (!x) && (!y) )
		return 0;
	if (x >= 0)
	{	// x >= 0
		if (y >= 0)
		{	// y >= 0
			if (x > y)
				return tantoangle[SlopeDiv(y,x)];	// octant 0
			else
				return ANG90 - 1 - tantoangle[SlopeDiv(x,y)];	// octant 1
		}
		else
		{	// y < 0
			y = -y;
			if (x > y)
				return -tantoangle[SlopeDiv(y,x)];	// octant 8
			else
				return ANG270 + tantoangle[SlopeDiv(x,y)];	// octant 7
		}
	}
	else
	{	// x < 0
		x = -x;
		if (y >= 0)
		{	// y >= 0
			if (x > y)
				return ANG180 - 1 - tantoangle[SlopeDiv(y,x)];	// octant 3
			else
				return ANG90 + tantoangle[SlopeDiv(x,y)];	// octant 2
		}
		else
		{	// y < 0
			y = -y;
			if (x > y)
				return ANG180 + tantoangle[SlopeDiv(y,x)];	// octant 4
			else
				return ANG270 - 1 - tantoangle[SlopeDiv(x,y)];	// octant 5
		}
	}

	return 0;
}


angle_t R_PointToAngle2 (fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2)
{
	viewx = x1;
	viewy = y1;
	return R_PointToAngle (x2, y2);
}


fixed_t	R_PointToDist (fixed_t x, fixed_t y)
{
	int		angle;
	fixed_t	dx, dy, temp;
	fixed_t	dist;

	dx = abs(x - viewx);
	dy = abs(y - viewy);

	if (dy > dx)
	{
		temp = dx;
		dx = dy;
		dy = temp;
	}

	angle = (tantoangle[ FixedDiv(dy,dx)>>DBITS ]+ANG90) >> ANGLETOFINESHIFT;

	dist = FixedDiv (dx, finesine[angle]);	// use as cosine

	return dist;
}

//=============================================================================


/*
==============
=
= R_SetViewSize
=
= Don't really change anything here, because i might be in the middle of
= a refresh.  The change will take effect next refresh.
=
==============
*/

static int	setblocks, setdetail;
boolean		setsizeneeded;

void R_SetViewSize (int blocks, int detail)
{
	setsizeneeded = true;
	setblocks = blocks;
	setdetail = detail;
}

/*
==============
=
= R_ExecuteSetViewSize
=
==============
*/

void R_ExecuteSetViewSize (void)
{
	setsizeneeded = false;

	if (setblocks == 11)
	{
		scaledviewwidth = SCREENWIDTH;
		viewheight = SCREENHEIGHT;
	}
	else
	{
		scaledviewwidth = setblocks*32;
		viewheight = (setblocks*(200 - SBARHEIGHT*sbarscale/20)/10);
	}

	detailshift = setdetail;
	viewwidth = scaledviewwidth>>detailshift;

	centery = viewheight/2;
	centerx = viewwidth/2;
	centerxfrac = centerx<<FRACBITS;
	centeryfrac = centery<<FRACBITS;
	projection = centerxfrac;

	R_InitBuffer (scaledviewwidth, viewheight);

//
// psprite scales
//
	pspritescale = FRACUNIT*viewwidth/SCREENWIDTH;
	pspriteiscale = FRACUNIT*SCREENWIDTH/viewwidth;

//
// draw the border
//
	R_DrawViewBorder ();	// erase old menu stuff
}


/*
==============
=
= R_Init
=
==============
*/

int detailLevel;
int screenblocks;

void R_Init(void)
{
	R_InitData();
	// viewwidth / viewheight / detailLevel are set by the defaults
	R_SetViewSize(screenblocks, detailLevel);
	R_InitPlanes();
	R_InitSkyMap();
	OGL_InitData();
	framecount = 0;
}

/*
==============
=
= R_PointInSubsector
=
==============
*/

subsector_t *R_PointInSubsector (fixed_t x, fixed_t y)
{
	node_t	*node;
	int		side, nodenum;

	if (!numnodes)	// single subsector is a special case
		return subsectors;

	nodenum = numnodes - 1;

	while (! (nodenum & NF_SUBSECTOR) )
	{
		node = &nodes[nodenum];
		side = R_PointOnSide (x, y, node);
		nodenum = node->children[side];
	}

	return &subsectors[nodenum & ~NF_SUBSECTOR];
}

//----------------------------------------------------------------------------
//
// PROC R_SetupFrame
//
//----------------------------------------------------------------------------

void R_SetupFrame(player_t *player)
{
	int tableAngle;
	int tempCentery;
	int intensity;

	viewplayer = player;
	viewangle = player->mo->angle + viewangleoffset;
	viewpitch = player->lookdir;
	tableAngle = viewangle>>ANGLETOFINESHIFT;
	viewx = player->mo->x;
	viewy = player->mo->y;

	if (localQuakeHappening[displayplayer] && !paused)
	{
		intensity = localQuakeHappening[displayplayer];
		viewx += ((M_Random() % (intensity<<2)) - (intensity<<1)) << FRACBITS;
		viewy += ((M_Random() % (intensity<<2)) - (intensity<<1)) << FRACBITS;
	}

	extralight = player->extralight;
	viewz = player->viewz;

	tempCentery = viewheight/2;
	if (centery != tempCentery)
	{
		centery = tempCentery;
		centeryfrac = centery<<FRACBITS;
	}
	viewsin = finesine[tableAngle];
	viewcos = finecosine[tableAngle];
	sscount = 0;
	if (player->fixedcolormap)
	{
		fixedcolormap = colormaps + player->fixedcolormap*256*sizeof(lighttable_t);
	}
	else
	{
		fixedcolormap = 0;
	}
	framecount++;
	validcount++;
	if (BorderNeedRefresh)
	{
			R_DrawViewBorder();

		BorderNeedRefresh = false;
		BorderTopRefresh = false;
		UpdateState |= I_FULLSCRN;
	}
	if (BorderTopRefresh)
	{
		if (setblocks < 10)
		{
			R_DrawTopBorder();
		}
		BorderTopRefresh = false;
		UpdateState |= I_MESSAGES;
	}
}

/*
==============
=
= R_RenderView
=
==============
*/

void R_RenderPlayerView (player_t *player)
{
	R_SetupFrame (player);

	R_ClearSprites ();
	NetUpdate ();	// check for new console commands

	OGL_SwitchTo3DState();

	// Make displayed player invisible locally
	if (localQuakeHappening[displayplayer] && gamestate == GS_LEVEL)
	{
		players[displayplayer].mo->flags2 |= MF2_DONTDRAW;
		R_RenderMap();
		players[displayplayer].mo->flags2 &= ~MF2_DONTDRAW;
	}
	else
	{
		R_RenderMap();
	}

	NetUpdate ();	// check for new console commands

	R_DrawMasked ();
	NetUpdate ();	// check for new console commands

	OGL_Restore2DState(1);

	// Draw psprites.
	if (viewangleoffset <=  1024<<ANGLETOFINESHIFT ||
	    viewangleoffset >= -1024<<ANGLETOFINESHIFT)
	{  // don't draw on side views
		R_DrawPlayerSprites();
	}

	OGL_Restore2DState(2);
}

