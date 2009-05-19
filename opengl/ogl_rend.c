//**************************************************************************
//**
//** OGL_DRAW.C
//**
//** Version:		1.0
//** Last Build:	-?-
//** Author:		jk
//**
//** Rendering lists and other rendering.
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "h2stdinc.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "h2def.h"
#include "r_local.h"
#include "p_local.h"
#include "ogl_def.h"
#include "m_bams.h"
#include "ogl_rl.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static void OGL_ProjectionMatrix(void);
static void DL_Clear(void);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

extern fadeout_t	fadeOut[2];	/* For both skies. */
extern int	skyhemispheres;

extern subsector_t	*currentssec;

// PUBLIC DATA DEFINITIONS -------------------------------------------------

int	viewwidth, scaledviewwidth, viewheight, viewwindowx, viewwindowy;

int		sbarscale = 20;

boolean		whitefog = false;	/* Is the white fog in use? */
boolean		special200;
boolean		BorderNeedRefresh;
boolean		BorderTopRefresh;

float		vx, vy, vz, vang, vpitch;

boolean		willRenderSprites = true, freezeRLs = false;

int		useDynLights = 0;
lumobj_t	*luminousList = NULL;
int		numLuminous = 0, maxLuminous = 0;
int		dlMaxRad = 64;		/* Dynamic lights maximum radius. */

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static int	viewpw, viewph;		/* Viewport size. */
static float	nearClip, farClip;
static float	yfov;
static float	viewsidex, viewsidey;	/* For the black fog. */

static boolean	firstsubsector;		/* No range checking for the first one. */

// CODE --------------------------------------------------------------------


// How far the point is from the viewside plane?
float PointDist2D (float c[2])
{
	/*
	    (YA-YC)(XB-XA)-(XA-XC)(YB-YA)
	s = -----------------------------
			L**2
	Luckily, L**2 is one. dist = s*L. Even more luckily, L is also one.
	*/
	float dist = (vz - c[VY])*viewsidex - (vx - c[VX])*viewsidey;
	if (dist < 0)
		return -dist;	// Always return positive.
	return dist;
}

// ---------------------------------------------------

void OGL_InitData(void)
{
	OGL_TexInit();		// OpenGL texture manager.
	bamsInit();		// Binary angle calculations.
	C_Init();		// Clipper.
	RL_Init();		// Rendering lists.
}

void OGL_ResetData(void)	// Called before starting a new level.
{
	OGL_TexReset();		// Textures are deleted (at least skies need this???).
	RL_DeleteLists();	// The rendering lists are destroyed.

	// We'll delete the sky textures. New ones will be generated for each level.
	//glDeleteTextures(2, skynames);
	//skynames[0] = skynames[1] = 0;

	// Ready for new fadeout colors.
	fadeOut[0].set = fadeOut[1].set = 0;

	DL_Clear();
}

void OGL_InitRenderer(void)	// Initializes the renderer to 2D state.
{
	GLfloat fogcol[4] = { .7f, .7f, .7f, 1 };

	// The variables.
	nearClip = 5;
	farClip = 8000;

	// Here we configure the OpenGL state and set projection matrix.
	glFrontFace(GL_CW);
	glDisable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glDisable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_TEXTURE_2D);
	//glPolygonMode(GL_FRONT, GL_LINE);

	// The projection matrix.
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, 320, 200, 0);

	// Initialize the modelview matrix.
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Clear also the texture matrix (I'm not using this, though).
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();

	// Alpha blending is a go!
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0);

	// Default state for the white fog is off.
	whitefog = false;
	glDisable(GL_FOG);
	glFogi(GL_FOG_MODE, GL_LINEAR);
	glFogi(GL_FOG_END, 3500);	// This should be tweaked a bit.
	glFogfv(GL_FOG_COLOR, fogcol);

	/*
	glEnable(GL_POLYGON_SMOOTH);
	glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
	*/

	// We don't want any hassles with uninitialized names, do we?
	//skynames[0] = skynames[1] = 0;
}

void OGL_UseWhiteFog(int yes)
{
	if (!whitefog && yes)
	{
		// White fog is turned on.
		whitefog = true;
		glEnable(GL_FOG);
	}
	else if (whitefog && !yes)
	{
		// White fog must be turned off.
		whitefog = false;
		glDisable(GL_FOG);
	}
	// Otherwise we won't do a thing.
}

void OGL_SwitchTo3DState(void)
{
	// Push the 2D state on the stack.
	glPushAttrib(GL_VIEWPORT_BIT | GL_ENABLE_BIT);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	// Enable some.. things.
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	// Set the viewport.
	if (viewheight != SCREENHEIGHT)
	{
		int svx = viewwindowx * screenWidth / 320,
		    svy = viewwindowy * screenHeight / 200;
		viewpw = viewwidth * screenWidth / 320;
		viewph = viewheight * screenHeight / 200 + 1;
		glViewport(svx, screenHeight-svy-viewph, viewpw, viewph);
	}
	else
	{
		viewpw = screenWidth;
		viewph = screenHeight;
	}

	// The 3D projection matrix.
	OGL_ProjectionMatrix();
}

void OGL_Restore2DState(int step)
{
	if (step == 1)
	{
		extern int screenblocks;
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(0, 320, (screenblocks < 11) ? 161 : 200, 0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
	}
	// Retrieve the old state.
	if (step == 2)
	{
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
		glPopAttrib();
	}
}

static void OGL_ProjectionMatrix(void)
{
// We're assuming pixels are squares... well, they are nowadays.
	float aspect = (float)viewpw / (float)viewph;	// = 1.0
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(yfov = 90.0/aspect, aspect, nearClip, farClip);
	// We'd like to have a left-handed coordinate system.
	glScalef(1, 1, -1);
}

static void OGL_ModelViewMatrix(void)
{
	vx = FIX2FLT(viewx);
	vy = FIX2FLT(viewz);
	vz = FIX2FLT(viewy);
	vang = viewangle / (float)ANGLE_MAX * 360 - 90;

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glRotatef(vpitch = viewpitch * 85.0/110.0, 1, 0, 0);
	glRotatef(vang, 0, 1, 0);
	glScalef(1, 1.2f, 1);	// This is the aspect correction.
	glTranslatef(-vx, -vy, -vz);
}

// *VERY* similar to SegFacingDir(). Well, this is actually the
// same function, only a more general version.
static int SegFacingPoint(float v1[2], float v2[2], float p[2])
{
	float nx = v1[VY] - v2[VY], ny = v2[VX] - v1[VX];
	float pvx = v1[VX] - p[VX], pvy = v1[VY] - p[VY];
	// The dot product.
	if (nx*pvx + ny*pvy > 0)
		return 1;	// Facing front.
	return 0;		// Facing away.
}

static void projectVector(float a[2], float b[2], float *a_on_b)
{
	int		c;
	float	factor = (a[0]*b[0] + a[1]*b[1]) / (b[0]*b[0] + b[1]*b[1]);
	for (c = 0; c < 2; c++)
		a_on_b[c] = factor * b[c];
}

// Create dynamic light quads, if necessary.
static void DL_ProcessWall(rendquad_t *quad, float v1[2], float v2[2])
{
	int		i, c;
	rendquad_t	dlq;
	float		pntLight[2];
	float		vecLight[2]; // Vector from v1 to light point.
	float		uvecWall[2], uvecWallNormal[2]; // Unit vectors.
	float		vecDist[2];
	float		dist;		// Distance between light source and wall.

	// We can't handle masked walls. The alpha...
	if (quad->flags & RQF_MASKED)
		return;

	for (i = 0; i < numLuminous; i++)
	{
		lumobj_t *lum = luminousList + i;
		// If the light is not in use, we skip it.
		if (!lum->use)
			continue;
		// First we must check orientation. Backfaces aren't lighted, naturally.
		pntLight[VX] = FIX2FLT(lum->thing->x);
		pntLight[VY] = FIX2FLT(lum->thing->y);
		if (!SegFacingPoint(v1, v2, pntLight))
			continue;
		// Make a copy of the original.
		memcpy(&dlq, quad, sizeof(dlq));
		// Copy the given end points.
		memcpy(dlq.v1, v1, sizeof(v1));
		memcpy(dlq.v2, v2, sizeof(v2));
		dlq.flags |= RQF_LIGHT; // This is a light texture.
		dlq.texw = dlq.texh = dlMaxRad*2;
		// The wall vector.
		for (c = 0; c < 2; c++)
			uvecWall[c] = (quad->v2[c] - quad->v1[c]) / quad->u.q.len;
		// The normal.
		uvecWallNormal[VX] = uvecWall[VY];
		uvecWallNormal[VY] = -uvecWall[VX];
		// The relative position of the light source.
		for (c = 0; c < 2; c++)
			vecLight[c] = pntLight[c] - quad->v1[c];
		// The distance vector. VecLight projected on the normal vector.
		projectVector(vecLight, uvecWallNormal, vecDist);
		// The accurate distance from the wall to the light.
		dist = sqrt(vecDist[0]*vecDist[0] + vecDist[1]*vecDist[1]);
		if (dist > dlMaxRad)
			continue; // Too far away.
		// Now we can calculate the intensity of the light.
		dlq.light = 1.5f - 1.5f*dist/dlMaxRad;
		// Do a scalar projection for the offset.
		dlq.texoffx = vecLight[0]*uvecWall[0] + vecLight[1]*uvecWall[1] - dlMaxRad;
		// There is no need to draw the *whole* wall always. Adjust the start
		// and end points so that only a relevant portion is included.
		if (dlq.texoffx > dlq.u.q.len)
			continue; // Doesn't fit on the wall.
		if (dlq.texoffx < -dlq.texw)
			continue; // Ditto, but in the other direction.
		if (dlq.texoffx > 0)
		{
			for (c = 0; c < 2; c++)
				dlq.v1[c] += dlq.texoffx * uvecWall[c];
			if (dlq.texoffx + dlq.texw <= dlq.u.q.len) // Fits completely?
			{
				for (c = 0; c < 2; c++)
					dlq.v2[c] = dlq.v1[c] + dlq.texw*uvecWall[c];
				dlq.u.q.len = dlq.texw;
			}
			else // Doesn't fit.
				dlq.u.q.len -= dlq.texoffx;
			dlq.texoffx = 0;
		}
		else // It goes off to the left.
		{
			if (dlq.texoffx + dlq.texw <= dlq.u.q.len) // Fits completely?
			{
				for (c = 0; c < 2; c++)
					dlq.v2[c] = dlq.v1[c] + (dlq.texw + dlq.texoffx)*uvecWall[c];
				dlq.u.q.len = dlq.texw + dlq.texoffx;
			}
		}
		// The vertical offset is easy to determine.
		dlq.texoffy = FIX2FLT(lum->thing->z) + lum->top-lum->height/2 + dlMaxRad - dlq.top;
		if (dlq.texoffy < -dlq.top + dlq.u.q.bottom)
			continue;
		if (dlq.texoffy > dlq.texh)
			continue;
		if (dlq.top + dlq.texoffy - dlq.texh >= dlq.u.q.bottom) // Fits completely?
			dlq.u.q.bottom = dlq.top + dlq.texoffy - dlq.texh;
		if (dlq.texoffy < 0)
		{
			dlq.top += dlq.texoffy;
			dlq.texoffy = 0;
		}
		// As a final touch, move the light quad a bit away from the wall
		// to avoid z-fighting.
		for (c = 0; c < 2; c++)
		{
			dlq.v1[c] += .2 * uvecWallNormal[c];
			dlq.v2[c] += .2 * uvecWallNormal[c];
		}
		// Now we can give the new quad to the rendering engine.
		RL_AddQuad(&dlq, 0);
	}
}

static int SegFacingDir(float v1[2], float v2[2])
{
	float nx = v1[VY] - v2[VY], ny = v2[VX] - v1[VX];
	float vvx = v1[VX] - vx, vvy = v1[VY] - vz;

	// The dot product.
	if (nx*vvx + ny*vvy > 0)
		return 1;	// Facing front.
	return 0;		// Facing away.
}

// The sector height should've been checked by now.
void R_RenderWallSeg(seg_t *seg, sector_t *frontsec, boolean accurate)
{
	sector_t		*backsec = seg->backsector;
	side_t			*sid = seg->sidedef;
	line_t			*ldef = seg->linedef;
	float			ffloor = FIX2FLT(frontsec->floorheight);
	float			fceil = FIX2FLT(frontsec->ceilingheight);
	float			bfloor, bceil, fsh = fceil - ffloor, bsh;
	float			tcyoff;
	rendquad_t		quad;
	float			origv1[2], origv2[2];	// The original end points, for dynlights.

	memset(&quad, 0, sizeof(quad));		// Clear the quad.

	// Get the start and end points. Full floating point conversion is
	// actually only necessary for polyobjs.
	if (accurate)
	{
		quad.v1[VX] = FIX2FLT(seg->v1->x);
		quad.v1[VY] = FIX2FLT(seg->v1->y);
		quad.v2[VX] = FIX2FLT(seg->v2->x);
		quad.v2[VY] = FIX2FLT(seg->v2->y);
		// These are the original vertices, copy them.
		memcpy(origv1, quad.v1, sizeof(origv1));
		memcpy(origv2, quad.v2, sizeof(origv2));
	}
	else
	{
		float dx, dy;
		// Not-so-accurate.
		quad.v1[VX] = Q_FIX2FLT(seg->v1->x);
		quad.v1[VY] = Q_FIX2FLT(seg->v1->y);
		quad.v2[VX] = Q_FIX2FLT(seg->v2->x);
		quad.v2[VY] = Q_FIX2FLT(seg->v2->y);
		// The original vertex. For dlights.
		memcpy(origv1, quad.v1, sizeof(origv1));
		memcpy(origv2, quad.v2, sizeof(origv2));
		// Make the very small crack-hiding adjustment.
		dx = quad.v2[VX] - quad.v1[VX];
		dy = quad.v2[VY] - quad.v1[VY];
		quad.v2[VX] += dx /seg->len / 4;
		quad.v2[VY] += dy /seg->len / 4;
	}

	// Let's first check which way this seg is facing.
	if (!SegFacingDir(quad.v1, quad.v2))
		return;	// The wrong direction?

	// Calculate the distances.
	quad.dist[0] = PointDist2D(quad.v1);
	quad.dist[1] = PointDist2D(quad.v2);

	// Calculate the lightlevel.
	quad.light = frontsec->lightlevel;
	/*
	if (quad.v1[VX] == quad.v2[VX])
		quad.light += 16;
	if (quad.v1[VY] == quad.v2[VY])
		quad.light -= 16;
	*/
	quad.light /= 255.0;

	// This line is now seen in the map.
	ldef->flags |= ML_MAPPED;

	// Some texture coordinates.
	quad.texoffx = (sid->textureoffset>>FRACBITS) + (seg->offset>>FRACBITS);
	quad.u.q.len = seg->len;
	tcyoff = Q_FIX2FLT(sid->rowoffset);

	// The middle texture, single sided.
	if (sid->midtexture && !backsec)
	{
		curtex = OGL_PrepareTexture(sid->midtexture);
		quad.texoffy = tcyoff;
		if (ldef->flags & ML_DONTPEGBOTTOM)
			quad.texoffy += texh - fsh;

		// Fill in the remaining quad data.
		quad.flags = 0;
		quad.top = fceil;
		quad.u.q.bottom = ffloor;
		quad.texw = texw;
		quad.texh = texh;
		RL_AddQuad(&quad, curtex);
		if (useDynLights)
			DL_ProcessWall(&quad, origv1, origv2);

		OGL_DEBUG("Solid segment in sector %p.\n", frontsec);
		// This is guaranteed to be a solid segment.
		C_AddViewRelSeg(quad.v1[VX], quad.v1[VY], quad.v2[VX], quad.v2[VY]);
	}
	// The skyfix?
	if (frontsec->skyfix)
	{
		if (!backsec || (backsec && (backsec->ceilingheight + (backsec->skyfix<<FRACBITS) < 
					frontsec->ceilingheight + (frontsec->skyfix<<FRACBITS))))// ||
					//backsec->floorheight == frontsec->ceilingheight)))
		{
			quad.flags = RQF_SKY_MASK_WALL;
			quad.top = fceil + frontsec->skyfix;
			quad.u.q.bottom = fceil;
			RL_AddQuad(&quad, curtex);
			if (useDynLights)
				DL_ProcessWall(&quad, origv1, origv2);
		}
	}
	// If there is a back sector we may need upper and lower walls.
	if (backsec)	// A twosided seg?
	{
		bfloor = FIX2FLT(backsec->floorheight);
		bceil = FIX2FLT(backsec->ceilingheight);
		bsh = bceil - bfloor;
		if (bsh <= 0 || bceil <= ffloor || bfloor >= fceil)
		{
			OGL_DEBUG("Solid segment in sector %p (backvol=0).\n", frontsec);
			// The backsector has no space. This is a solid segment.
			C_AddViewRelSeg(quad.v1[VX],quad.v1[VY],quad.v2[VX],quad.v2[VY]);
		}
		if (sid->midtexture)	// Quite probably a masked texture.
		{
			float	mceil = (bceil<fceil) ? bceil : fceil,
				mfloor = (bfloor>ffloor) ? bfloor : ffloor,
				msh = mceil - mfloor;
			if (msh > 0)
			{
				curtex = OGL_PrepareTexture(sid->midtexture);
				// Calculate the texture coordinates. Also restrict
				// vertical tiling (if masked) by adjusting mceil and mfloor.
				if (texmask)	// A masked texture?
				{
					quad.flags = RQF_MASKED;
					quad.texoffy = 0;
					// We don't allow vertical tiling.
					if (ldef->flags & ML_DONTPEGBOTTOM)
					{
						mfloor += tcyoff;
						mceil = mfloor + texh;
					}
					else
					{
						mceil += tcyoff;
						mfloor = mceil - texh;
					}
				}
				else // Normal texture.
				{
					quad.flags = 0;
					quad.texoffy = tcyoff;
					if (ldef->flags & ML_DONTPEGBOTTOM) // Lower unpegged. Align bottom.
						quad.texoffy += texh - msh;
				}
				// Fill in the remainder of the data.
				quad.top = mceil;
				quad.u.q.bottom = mfloor;
				quad.texw = texw;
				quad.texh = texh;
				RL_AddQuad(&quad, curtex);
				if (useDynLights)
					DL_ProcessWall(&quad, origv1, origv2);
			}
		}
		// Upper wall.
		if (bceil < fceil && !(frontsec->ceilingpic == skyflatnum && backsec->ceilingpic == skyflatnum))
		{
			float topwh = fceil - bceil;
			if (sid->toptexture)	// A texture present?
			{
				curtex = OGL_PrepareTexture(sid->toptexture);
				// Calculate texture coordinates.
				quad.texoffy = tcyoff;
				if (!(ldef->flags & ML_DONTPEGTOP))
				{
					// Normal alignment to bottom.
					quad.texoffy += texh - topwh;
				}
				quad.flags = 0;
			}
			else
			{
				// No texture? Bad thing. You don't deserve texture
				// coordinates. Take the ceiling texture.
				curtex = OGL_PrepareFlat(frontsec->ceilingpic);
				quad.flags = RQF_FLAT | RQF_MISSING_WALL;
			}
			quad.top = fceil;
			quad.u.q.bottom = bceil;
			quad.texw = texw;
			quad.texh = texh;
			RL_AddQuad(&quad, curtex);
			if (useDynLights)
				DL_ProcessWall(&quad, origv1, origv2);
		}
		// Lower wall.
		if (bfloor > ffloor && !(frontsec->floorpic == skyflatnum && backsec->floorpic == skyflatnum))
		{
			if (sid->bottomtexture)	// There is a texture?
			{
				curtex = OGL_PrepareTexture(sid->bottomtexture);
				// Calculate texture coordinates.
				quad.texoffy = tcyoff;
				if (ldef->flags & ML_DONTPEGBOTTOM)
				{
					// Lower unpegged. Align with normal middle texture.
					//quad.texoffy += fsh - texh;
					quad.texoffy += fceil - bfloor;
				}
				quad.flags = 0;
			}
			else
			{
				// No texture? Again!
				curtex = OGL_PrepareFlat(frontsec->floorpic);
				quad.flags = RQF_FLAT | RQF_MISSING_WALL;
			}
			quad.top = bfloor;
			quad.u.q.bottom = ffloor;
			quad.texw = texw;
			quad.texh = texh;
			RL_AddQuad(&quad, curtex);
			if (useDynLights)
				DL_ProcessWall(&quad, origv1, origv2);
		}
	}
}

void R_HandleSectorSpecials(sector_t *sect)
{
	int	scrollOffset = leveltime>>1 & 63;

	switch (sect->special)
	{ // Handle scrolling flats
	case 201: case 202: case 203: // Scroll_North_xxx
		sect->flatoffy = (63-scrollOffset) << (sect->special-201);
		break;
	case 204: case 205: case 206: // Scroll_East_xxx
		sect->flatoffx = (63-scrollOffset) << (sect->special-204);
		break;
	case 207: case 208: case 209: // Scroll_South_xxx
		sect->flatoffy = scrollOffset << (sect->special-207);
		break;
	case 210: case 211: case 212: // Scroll_West_xxx
		sect->flatoffx = scrollOffset << (sect->special-210);
		break;
	case 213: case 214: case 215: // Scroll_NorthWest_xxx
		sect->flatoffx = scrollOffset << (sect->special-213);
		sect->flatoffy = (63-scrollOffset) << (sect->special-213);
		break;
	case 216: case 217: case 218: // Scroll_NorthEast_xxx
		sect->flatoffx = (63-scrollOffset) << (sect->special-216);
		sect->flatoffy = (63-scrollOffset) << (sect->special-216);
		break;
	case 219: case 220: case 221: // Scroll_SouthEast_xxx
		sect->flatoffx = (63-scrollOffset) << (sect->special-219);
		sect->flatoffy = scrollOffset << (sect->special-219);
		break;
	case 222: case 223: case 224: // Scroll_SouthWest_xxx
		sect->flatoffx = scrollOffset << (sect->special-222);
		sect->flatoffy = scrollOffset << (sect->special-222);
		break;
	default:
		sect->flatoffx = sect->flatoffy = 0;
		break;
	}
}

static void DL_Clear(void)
{
	free(luminousList);
	luminousList = 0;
	maxLuminous = numLuminous = 0;
}

boolean DL_AddLuminous(mobj_t *thing)
{
	if (thing->frame & FF_FULLBRIGHT && !(thing->flags2 & MF2_DONTDRAW))
	{
		spritedef_t *sprdef;
		spriteframe_t *sprframe;
		int lump;
		lumobj_t *lum;

		// Only allocate memory when it's needed.
		if (++numLuminous > maxLuminous)
			luminousList = (lumobj_t *) realloc (luminousList, sizeof(lumobj_t) * (maxLuminous += 5));
		lum = luminousList + numLuminous - 1;
		lum->thing = thing;
		// We need to know how tall the thing currently is.
		sprdef = &sprites[thing->sprite];
		sprframe = &sprdef->spriteframes[thing->frame & FF_FRAMEMASK];
		if (sprframe->rotate)
			lump = sprframe->lump[(R_PointToAngle(thing->x, thing->y) - thing->angle + (unsigned)(ANG45/2)*9) >> 29];
		else
			lump = sprframe->lump[0];
		// This'll ensure we have up-to-date information.
		OGL_PrepareSprite(lump);
		lum->top = FIX2FLT(spritetopoffset[lump] - lum->thing->floorclip);
		lum->height = spriteheights[lump];
		OGL_DEBUG("%p: t:%.2f h:%.2f\n", thing, lum->top, lum->height);
	}
	return true;
}

// We want to know which luminous objects are close enough the subsector.
void DL_MarkForSubsector(subsector_t *sub)
{
	int		i;
	float	minx, miny, maxx, maxy;	// Subsector bounding box.

	// First determine the subsector bounding box based on
	// the edge points.
	/*
	maxx = minx = sub->edgeverts[0].x;
	maxy = miny = sub->edgeverts[0].y;
	for (i = 1; i < sub->numedgeverts; i++)
	{
		float x = sub->edgeverts[i].x, y = sub->edgeverts[i].y;
		if (x > maxx)
			maxx = x;
		if (x < minx)
			minx = x;
		if (y > maxy)
			maxy = y;
		if (y < miny)
			miny = y;
	}
	// Adjust the bounding box to include the maximum radius of the
	// dynamic lights (64).
	maxx += dlMaxRad;
	minx -= dlMaxRad;
	maxy += dlMaxRad;
	miny -= dlMaxRad;
	*/

	// Adjust the bounding box to include the maximum radius of the
	// dynamic lights (64).
	minx = sub->bbox[0].x - dlMaxRad;
	miny = sub->bbox[0].y - dlMaxRad;
	maxx = sub->bbox[1].x + dlMaxRad; 
	maxy = sub->bbox[1].y + dlMaxRad; 
	// Now check all the luminous objects and mark those that
	// are close enough.
	for (i = 0; i < numLuminous; i++)
	{
		lumobj_t *lum = luminousList + i;
		float x = Q_FIX2FLT(lum->thing->x), y = Q_FIX2FLT(lum->thing->y);
		// By default the luminous object isn't used.
		lum->use = false;
		// Is inside the bounding box?
		if (x > minx && y > miny && x < maxx && y < maxy)
			lum->use = true;
	}
}

void R_RenderSubsector(int ssecidx)
{
	subsector_t		*ssec = subsectors + ssecidx;
	int			i;
	seg_t			*seg;
	sector_t		*sect = ssec->sector;
	float			ffloor = FIX2FLT(sect->floorheight);
	float			fceil = FIX2FLT(sect->ceilingheight);
	rendquad_t		triangle;

	if (fceil-ffloor <= 0)
	{
		return;	// Skip this, no volume.
	}

	if (!firstsubsector)
	{
		if (!C_CheckSubsector(ssec))
			return;	// This isn't visible.
	}
	else
		firstsubsector = false;

	OGL_DEBUG("*** Rendering subsector %d (sector %p)\n", ssecidx, sect);

	currentssec = ssec;

	// Sprites for this sector have to be drawn. This must be done before
	// the segments of this subsector are added to the clipper. Otherwise
	// the sprites would get clipped by them, and that wouldn't be right.
	R_AddSprites(sect);

	// Dynamic lights?
	if (useDynLights)
		DL_MarkForSubsector(ssec);

	// Draw the walls.
	for (i = 0, seg = segs + ssec->firstline; i < ssec->numlines; i++, seg++)
		R_RenderWallSeg(seg, sect, false);

	// Is there a polyobj on board?
	if (ssec->poly)
	{
		for (i = 0; i < ssec->poly->numsegs; i++)
			R_RenderWallSeg(ssec->poly->segs[i], sect, true);
	}

	// The floor.
	memset(&triangle, 0, sizeof(triangle));
	triangle.flags = RQF_FLAT | RQF_FLOOR_FACING;	// This is a flat floor triangle.
	triangle.light = sect->lightlevel / 255.0;
	if (viewz > sect->floorheight) // && vpitch < yfov)
	{
		// Check for sky... in the floor?
		if (sect->floorpic == skyflatnum)
		{
			triangle.flags |= RQF_SKY_MASK;
			skyhemispheres |= SKYHEMI_LOWER;
			if (sect->special == 200)
				special200 = true;
		}
		curtex = OGL_PrepareFlat(sect->floorpic);
		triangle.texw = texw;
		triangle.texh = texh;
		triangle.texoffx = sect->flatoffx;
		triangle.texoffy = sect->flatoffy;
		triangle.top = ffloor;
		// The first vertex is always the first in the whole list.
		RL_AddFlatQuads(&triangle, curtex, ssec->numedgeverts, ssec->edgeverts, 0);
		// Dynamic lights.
		if (useDynLights && sect->floorpic != skyflatnum)
		{
			triangle.flags |= RQF_LIGHT;
			triangle.top += .05f;	// An adjustment.
			for (i = 0; i < numLuminous; i++)
			{
				lumobj_t *lum = luminousList + i;
				float z, hdiff;
				if (!lum->use)
					continue;

				z = FIX2FLT(lum->thing->z);
				// Check that we're on the right side.
				if (z + lum->top < triangle.top)
					continue; // Under it.
				// Check that the height difference isn't too big.
				z += lum->top - lum->height/2;	// Center Z.
				if ((hdiff = fabs(triangle.top - z)) > dlMaxRad)
					continue;
				triangle.light = 1.5f - 1.5f*hdiff/dlMaxRad;
				// We can add the light quads. // FIXME: pointer to integer cast !!!
				RL_AddFlatQuads(&triangle, (int)lum, ssec->numedgeverts, ssec->origedgeverts, 0);
			}
		}
	}
	// And the roof.
	triangle.flags = RQF_FLAT;
	triangle.light = sect->lightlevel / 255.0;
	if (viewz < sect->ceilingheight) //&& vpitch > -yfov)
	{
		// Check for sky.
		if (sect->ceilingpic == skyflatnum)
		{
			triangle.flags |= RQF_SKY_MASK;
			skyhemispheres |= SKYHEMI_UPPER;
			if (sect->special == 200)
				special200 = true;
		}
		curtex = OGL_PrepareFlat(sect->ceilingpic);
		triangle.texw = texw;
		triangle.texh = texh;
		triangle.texoffx = 0;
		triangle.texoffy = 0;
		triangle.top = fceil + sect->skyfix;
		// The first vertex is always the last in the whole list.
		RL_AddFlatQuads(&triangle, curtex, ssec->numedgeverts, ssec->edgeverts, 1);
		// Dynamic lights.
		if (useDynLights && sect->ceilingpic != skyflatnum)
		{
			triangle.flags |= RQF_LIGHT;
			triangle.top -= .05f;	// An adjustment.
			for (i = 0; i < numLuminous; i++)
			{
				lumobj_t *lum = luminousList + i;
				float z, hdiff;
				if (!lum->use)
					continue;

				z = FIX2FLT(lum->thing->z);
				// Check that we're on the right side.
				if (z + lum->top-lum->height > triangle.top)
					continue; // Under it.
				// Check that the height difference isn't too big.
				z += lum->top - lum->height/2;	// Center Z.
				if ((hdiff = fabs(triangle.top - z)) > dlMaxRad)
					continue;
				triangle.light = 1.5f - 1.5f*hdiff/dlMaxRad;
				// We can add the light quads. // FIXME: pointer to integer cast !!!
				RL_AddFlatQuads(&triangle, (int)lum, ssec->numedgeverts, ssec->origedgeverts, 1);
			}
		}
	}
}

void R_RenderNode(int bspnum)
{
	node_t		*bsp;
	int		side;

	// If the clipper is full we're pretty much done.
	if (cliphead)
	{
		if (cliphead->start == 0 && cliphead->end == BANG_MAX)
			return;
	}

	if (bspnum & NF_SUBSECTOR)
	{
		if (bspnum == -1)
			R_RenderSubsector(0);
		else
			R_RenderSubsector(bspnum & (~NF_SUBSECTOR));
		return;
	}

	bsp = &nodes[bspnum];

//
// decide which side the view point is on
//
	side = R_PointOnSide (viewx, viewy, bsp);

	R_RenderNode (bsp->children[side]); // recursively divide front space

	/*
	if (cliphead->start == 0 && cliphead->end == BANG_MAX)
		return;	// We can stop rendering.
	*/
	R_RenderNode (bsp->children[side^1]);
}

void R_RenderMap(void)
{
	int			i;
	binangle	viewside;

	// This is all the clearing we'll do.
	glClear(GL_DEPTH_BUFFER_BIT);

	// Setup the modelview matrix.
	OGL_ModelViewMatrix();

	// Let's scroll some floors. This way we have to check them all,
	// but there's no redundancy. And this really isn't all that expensive.
	for (i = 0; i < numsectors; i++)
		R_HandleSectorSpecials(sectors + i);

//	clippercount = 0;
//	rlcount = 0;

	if (!freezeRLs)
	{
		RL_ClearLists();	// Clear the lists for new quads.
		C_ClearRanges();	// Clear the clipper.
		if (useDynLights)	// Maintain luminous objects list.
		{
			numLuminous = 0;	// Clear the luminous object list.

			// Add all the luminous objects to the list.
			for (i = 0; i < numsectors; i++)
			{
				sector_t *sec = sectors + i;
				mobj_t *iter;
				for (iter = sec->thinglist; iter; iter = iter->snext)
					DL_AddLuminous(iter);
			}
		}

		// Add the backside clipping range, vpitch allowing.
		if (vpitch <= 90 - yfov/2 && vpitch >= -90 + yfov/2)
		{
			float a = fabs(vpitch) / (90 - yfov/2);
			binangle startAngle = (binangle) BANG_45*(1 + a);
			binangle angLen = BANG_180 - startAngle;
			viewside = (viewangle>>16) + startAngle;
			C_SafeAddRange(viewside, viewside + angLen);
			C_SafeAddRange(viewside + angLen, viewside + 2*angLen);
			//C_SafeAddRange(viewside + BANG_135, viewside + 2*BANG_135);
		}
		// The viewside line for the black fog distance calculations.
		viewsidex = -FIX2FLT(viewsin);
		viewsidey = FIX2FLT(viewcos);

		// We don't want subsector clipchecking for the first subsector.
		firstsubsector = true;
		special200 = false;
		R_RenderNode(numnodes - 1);
	}
	RL_RenderAllLists();

	// Clipping fragmentation happens when there are holes in the walls.
	/*
	if (cliphead->next)
	{
		clipnode_t *ci;
		printf("\nTic: %d, Clipnodes are fragmented:\n", gametic);
		for (ci = cliphead; ci; ci = ci->next)
			printf("range %p: %4x => %4x (%d)\n", ci, ci->start, ci->end, ci->used);
		I_Error("---Fragmented clipper---\n");
	}
	*/
}

void R_RenderSprite(vissprite_t *spr)
{
	float bot, top;
//	float off = FIX2FLT(spriteoffset[spr->patch]);
	float w = FIX2FLT(spritewidth[spr->patch]);
	int sprh;
	float p2w = FindNextPower2((int)w), p2h;
	float v1[2];//, v2[2];
	//float sinrv, cosrv;
	float tcleft, tcright;
	float alpha;
	//float thangle;

	// Set the texture.
	OGL_SetSprite(spr->patch);
	sprh = spriteheights[spr->patch];
	p2h = FindNextPower2(sprh);

//	v1[VX] = FIX2FLT(spr->gx);
//	v1[VY] = FIX2FLT(spr->gy);

	// Set the lighting and alpha.
	if (spr->mobjflags & MF_SHADOW)
		alpha = .333f;
	else if (spr->mobjflags & MF_ALTSHADOW)
		alpha = .666f;
	else
		alpha = 1;

	if (spr->lightlevel < 0)
		glColor4f(1, 1, 1, alpha);
	else
	{
		v1[VX] = Q_FIX2FLT(spr->gx);
		v1[VY] = Q_FIX2FLT(spr->gy);
		SetVertexColor(spr->lightlevel/255.0, PointDist2D(v1), alpha);
	}

	/*
	thangle = BANG2RAD(bamsAtan2((v1[VY] - vz)*10,(v1[VX] - vx)*10)) - PI/2;
	sinrv = sin(thangle);
	cosrv = cos(thangle);
	*/

	// We must find the correct positioning using the sector floor and ceiling
	// heights as an aid.
	/*
	top = FIX2FLT(spr->gzt) + 4 - FIX2FLT(spr->floorclip);
	bot = top - sprh;
	*/
	top = FIX2FLT(spr->gzt);
	if (sprh < spr->secceil - spr->secfloor)	// Sprite fits in, adjustment possible?
	{
		// Check top.
		if (top > spr->secceil)
			top = spr->secceil;
		// Check bottom.
		if (top-sprh < spr->secfloor)
			top = spr->secfloor + sprh;
	}
	// Adjust by the floor clip.
	top -= FIX2FLT(spr->floorclip);
	bot = top - sprh;

	/*
	v1[VX] -= cosrv * off;
	v1[VY] -= sinrv * off;
	v2[VX] = v1[VX] + cosrv*w;
	v2[VY] = v1[VY] + sinrv*w;
	*/

	if (spr->xiscale < 0)
	{
		// This is the flipped version.
		tcleft = w/p2w;
		tcright = 0;
	}
	else
	{
		// No flipping; the normal version.
		tcleft = 0;
		tcright = w/p2w;
	}

	// Choose the color for the sprite.
	/*
	glColor4f(1, 1, 1, (spr->mobjflags & MF_SHADOW) ? .333 : (spr->mobjflags & MF_ALTSHADOW) ? .666 : 1);
	*/
	glBegin(GL_QUADS);
	glTexCoord2f(tcleft, 0);
	glVertex3f(spr->v1[VX], top, spr->v1[VY]);

	glTexCoord2f(tcright, 0);
	glVertex3f(spr->v2[VX], top, spr->v2[VY]);

	glTexCoord2f(tcright, sprh/p2h);
	glVertex3f(spr->v2[VX], bot, spr->v2[VY]);

	glTexCoord2f(tcleft, sprh/p2h);
	glVertex3f(spr->v1[VX], bot, spr->v1[VY]);
	glEnd();
}

void OGL_DrawPSprite(int x, int y, float scale, int flip, int lump)
{
	int		w, h;
	float	tcx, tcy;

	OGL_SetSprite(lump);
	w = spritewidth[lump]>>FRACBITS;
	h = spriteheights[lump];
	tcx = NextPower2Ratio(w);
	tcy = NextPower2Ratio(h);

	glBegin(GL_QUADS);

	glTexCoord2f((flip) ? tcx : 0, 0);
	glVertex2f(x, y);

	glTexCoord2f((flip) ? 0 : tcx, 0);
	glVertex2f(x + w*scale, y);

	glTexCoord2f((flip) ? 0 : tcx, tcy);
	glVertex2f(x + w*scale, y + h*scale);

	glTexCoord2f((flip) ? tcx : 0, tcy);
	glVertex2f(x, y + h*scale);

	glEnd();
}


/*
================
=
= R_InitBuffer
=
= from r_draw.c
=================
*/
void R_InitBuffer (int width, int height)
{
	viewwindowx = (SCREENWIDTH - width) >> 1;
	if (width == SCREENWIDTH)
		viewwindowy = 0;
	else
		viewwindowy = (SCREENHEIGHT - SBARHEIGHT - height) >> 1;
}

/*
==================
=
= R_DrawViewBorder
=
= Draws the border around the view for different size windows
==================
*/
void R_DrawViewBorder (void)
{
	int lump;

//	if (scaledviewwidth == SCREENWIDTH)
	if ((scaledviewwidth == 320 && sbarscale == 20) ||
		(sbarscale != 20 && viewheight == 200))
		return;

	// View background.
	OGL_SetColorAndAlpha(1, 1, 1, 1);
	OGL_SetFlat(W_GetNumForName("F_022") - firstflat);

//	OGL_DrawRectTiled(0, 0, SCREENWIDTH, SCREENHEIGHT-SBARHEIGHT, 64, 64);
//	OGL_DrawCutRectTiled(0, 0, 320, 200 - (sbarscale == 20 ? SBARHEIGHT : 0)
//			/**sbarscale/20*/, 64, 64,
//			viewwindowx - 4, viewwindowy - 4, viewwidth + 8, viewheight + 8);
	OGL_DrawCutRectTiled(0, 0, 320, 200 - SBARHEIGHT, 64, 64,
			viewwindowx - 4, viewwindowy - 4, viewwidth + 8, viewheight + 8);

	// The border top.
	OGL_SetPatch(lump = W_GetNumForName("bordt"));
	OGL_DrawRectTiled(viewwindowx, viewwindowy - 4, viewwidth,
			  lumptexsizes[lump].h, 16, lumptexsizes[lump].h);
	// Border bottom.
	OGL_SetPatch(lump = W_GetNumForName("bordb"));
	OGL_DrawRectTiled(viewwindowx, viewwindowy + viewheight, viewwidth,
			  lumptexsizes[lump].h, 16, lumptexsizes[lump].h);
	// Left view border.
	OGL_SetPatch(lump = W_GetNumForName("bordl"));
	OGL_DrawRectTiled(viewwindowx - 4, viewwindowy, lumptexsizes[lump].w,
			  viewheight, lumptexsizes[lump].w, 16);
	// Right view border.
	OGL_SetPatch(lump=W_GetNumForName("bordr"));
	OGL_DrawRectTiled(viewwindowx + viewwidth, viewwindowy,
			  lumptexsizes[lump].w, viewheight, lumptexsizes[lump].w, 16);

	OGL_DrawPatch(viewwindowx - 4, viewwindowy - 4, W_GetNumForName("bordtl"));
	OGL_DrawPatch(viewwindowx + viewwidth, viewwindowy - 4, W_GetNumForName("bordtr"));
	OGL_DrawPatch(viewwindowx + viewwidth, viewwindowy + viewheight, W_GetNumForName("bordbr"));
	OGL_DrawPatch(viewwindowx - 4, viewwindowy + viewheight, W_GetNumForName("bordbl"));
}

/*
==================
=
= R_DrawTopBorder
=
= Draws the top border around the view for different size windows
==================
*/
void R_DrawTopBorder (void)
{
	if (scaledviewwidth == SCREENWIDTH)
		return;

	OGL_SetColorAndAlpha(1, 1, 1, 1);
	OGL_SetFlat(W_GetNumForName("F_022") - firstflat);

	OGL_DrawRectTiled(0, 0, 320, 64, 64, 64);
	if (viewwindowy < 65)
	{
		int	lump;
		OGL_SetPatch(lump = W_GetNumForName("bordt"));
		OGL_DrawRectTiled(viewwindowx, viewwindowy - 4, viewwidth,
				  lumptexsizes[lump].h, 16, lumptexsizes[lump].h);

		OGL_DrawPatch(viewwindowx - 4, viewwindowy, W_GetNumForName("bordl"));
		OGL_DrawPatch(viewwindowx + viewwidth, viewwindowy, W_GetNumForName("bordr"));
		OGL_DrawPatch(viewwindowx - 4, viewwindowy + 16, W_GetNumForName("bordl"));
		OGL_DrawPatch(viewwindowx + viewwidth, viewwindowy + 16, W_GetNumForName("bordr"));

		OGL_DrawPatch(viewwindowx - 4, viewwindowy - 4, W_GetNumForName("bordtl"));
		OGL_DrawPatch(viewwindowx + viewwidth, viewwindowy - 4, W_GetNumForName("bordtr"));
	}
}

