//**************************************************************************
//**
//** OGL_DRAW.C
//**
//** Version:		1.0
//** Last Build:	-?-
//** Author:		jk
//**
//** OpenGL drawing functions.
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "h2stdinc.h"

#ifdef __WIN32__
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "h2def.h"
#include "ogl_def.h"
#include "p_local.h"

// MACROS ------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static int curfilter = 0;	/* The current filter (0 = none). */

// CODE --------------------------------------------------------------------

void OGL_DrawRawScreen(int lump)	// Raw screens are 320 x 200.
{
	float tcbottom;
	int pixelBorder;

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0, screenWidth, screenHeight, 0);

	OGL_SetRawImage(lump, 1);
	tcbottom = lumptexsizes[lump].h / (float)FindNextPower2(lumptexsizes[lump].h);
	pixelBorder = lumptexsizes[lump].w * screenWidth / 320;

	glColor3f(1, 1, 1);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex2f(0, 0);
	glTexCoord2f(1, 0);
	glVertex2f(pixelBorder, 0);
	glTexCoord2f(1, tcbottom);
	glVertex2f(pixelBorder, screenHeight);
	glTexCoord2f(0, tcbottom);
	glVertex2f(0, screenHeight);
	glEnd();

	// And the other part.
	OGL_SetRawImage(lump, 2);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex2f(pixelBorder - 1, 0);
	glTexCoord2f(1, 0);
	glVertex2f(screenWidth, 0);
	glTexCoord2f(1, tcbottom);
	glVertex2f(screenWidth, screenHeight);
	glTexCoord2f(0, tcbottom);
	glVertex2f(pixelBorder - 1, screenHeight);
	glEnd();

	// Restore the old projection matrix.
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

// Drawing with the current state.
void OGL_DrawPatch_CS(int x, int y, int lumpnum)
{
	int		w, h, p2w, p2h;
	float	tcright, tcbottom;

	// Set the texture.
	OGL_SetPatch(lumpnum);

	w = lumptexsizes[lumpnum].w;
	h = lumptexsizes[lumpnum].h;
	p2w = FindNextPower2(w);
	p2h = OGL_ValidTexHeight2(w, h);
	tcright = (float)w / (float)p2w;
	tcbottom = (float)h / (float)p2h;

	x += lumptexsizes[lumpnum].offx;
	y += lumptexsizes[lumpnum].offy;

	glBegin(GL_QUADS);

	glTexCoord2f(0, 0);
	glVertex2i(x, y);

	glTexCoord2f(tcright, 0);
	glVertex2i(x + w, y);

	glTexCoord2f(tcright, tcbottom);
	glVertex2i(x + w, y + h);

	glTexCoord2f(0, tcbottom);
	glVertex2i(x, y + h);

	glEnd();

	// Is there a second part?
	if (OGL_GetOtherPart(lumpnum))
	{
		x += w;

		OGL_BindTexture(OGL_GetOtherPart(lumpnum));
		w = lumptexsizes[lumpnum].w2;
		p2w = FindNextPower2(w);
		tcright = w / (float)p2w;

		glBegin(GL_QUADS);

		glTexCoord2f(0, 0);
		glVertex2i(x, y);

		glTexCoord2f(tcright, 0);
		glVertex2i(x+w, y);

		glTexCoord2f(tcright, tcbottom);
		glVertex2i(x+w, y+h);

		glTexCoord2f(0, tcbottom);
		glVertex2i(x, y+h);

		glEnd();
	}
}

void OGL_DrawPatchLitAlpha(int x, int y, float light, float alpha, int lumpnum)
{
	glColor4f(light, light, light, alpha);
	OGL_DrawPatch_CS(x, y, lumpnum);
}

void OGL_DrawPatch(int x, int y, int lumpnum)
{
	if (lumpnum < 0)
		return;
	OGL_DrawPatchLitAlpha(x, y, 1, 1, lumpnum);
}

void OGL_DrawFuzzPatch(int x, int y, int lumpnum)
{
	if (lumpnum < 0)
		return;
	OGL_DrawPatchLitAlpha(x, y, 1, .333f, lumpnum);
}

void OGL_DrawAltFuzzPatch(int x, int y, int lumpnum)
{
	if (lumpnum < 0)
		return;
	OGL_DrawPatchLitAlpha(x, y, 1, .666f, lumpnum);
}

void OGL_DrawShadowedPatch(int x, int y, int lumpnum)
{
	if (lumpnum < 0)
		return;
	OGL_DrawPatchLitAlpha(x + 2, y + 2, 0, .4f, lumpnum);
	OGL_DrawPatchLitAlpha(x, y, 1, 1, lumpnum);
}

void OGL_DrawRect(float x, float y, float w, float h, float r, float g, float b, float a)
{
	glColor4f(r, g, b, a);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex2f(x, y);
	glTexCoord2f(1, 0);
	glVertex2f(x+w, y);
	glTexCoord2f(1, 1);
	glVertex2f(x+w, y+h);
	glTexCoord2f(0, 1);
	glVertex2f(x, y+h);
	glEnd();
}

void OGL_DrawRectTiled(int x, int y, int w, int h, int tw, int th)
{
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex2i(x, y);
	glTexCoord2f(w/(float)tw, 0);
	glVertex2i(x+w, y);
	glTexCoord2f(w/(float)tw, h/(float)th);
	glVertex2i(x+w, y+h);
	glTexCoord2f(0, h/(float)th);
	glVertex2i(x, y+h);
	glEnd();
}

// The cut rectangle must be inside the other one.
void OGL_DrawCutRectTiled(int x, int y, int w, int h, int tw, int th,
			  int cx, int cy, int cw, int ch)
{
	float	ftw = tw, fth = th;
	// We'll draw at max four rectangles.
	int	toph = cy - y, bottomh = y + h - (cy + ch), sideh = h - toph - bottomh,
		lefth = cx - x, righth = x + w - (cx + cw);

	glBegin(GL_QUADS);
	if (toph > 0)
	{
		// The top rectangle.
		glTexCoord2f(0, 0);
		glVertex2i(x, y);

		glTexCoord2f(w/ftw, 0);
		glVertex2i(x+w, y);

		glTexCoord2f(w/ftw, toph/fth);
		glVertex2i(x+w, y+toph);

		glTexCoord2f(0, toph/fth);
		glVertex2i(x, y+toph);
	}
	if (lefth > 0 && sideh > 0)
	{
		float yoff = toph / fth;
		// The left rectangle.
		glTexCoord2f(0, yoff);
		glVertex2i(x, y + toph);

		glTexCoord2f(lefth/ftw, yoff);
		glVertex2i(x+lefth, y+toph);

		glTexCoord2f(lefth/ftw, yoff+sideh/fth);
		glVertex2i(x+lefth, y+toph+sideh);

		glTexCoord2f(0, yoff+sideh/fth);
		glVertex2i(x, y+toph+sideh);
	}
	if (righth > 0 && sideh > 0)
	{
		int ox = x + lefth + cw;
		float xoff = (lefth + cw) / ftw;
		float yoff = toph / fth;
		// The left rectangle.
		glTexCoord2f(xoff, yoff);
		glVertex2i(ox, y+toph);

		glTexCoord2f(xoff+lefth/ftw, yoff);
		glVertex2i(ox+righth, y+toph);

		glTexCoord2f(xoff+lefth/ftw, yoff+sideh/fth);
		glVertex2i(ox+righth, y+toph+sideh);

		glTexCoord2f(xoff, yoff+sideh/fth);
		glVertex2i(ox, y+toph+sideh);
	}
	if (bottomh > 0)
	{
		int oy = y + toph + sideh;
		float yoff = (toph + sideh) / fth;
		glTexCoord2f(0, yoff);
		glVertex2i(x, oy);

		glTexCoord2f(w/ftw, yoff);
		glVertex2i(x+w, oy);

		glTexCoord2f(w/ftw, yoff+bottomh/fth);
		glVertex2i(x+w, oy+bottomh);

		glTexCoord2f(0, yoff+bottomh/fth);
		glVertex2i(x, oy+bottomh);
	}
	glEnd();
}

void OGL_DrawLine(float x1, float y1, float x2, float y2,
		  float r, float g, float b, float a)
{
	glColor4f(r, g, b, a);
	glBegin(GL_LINES);
	glVertex2f(x1, y1);
	glVertex2f(x2, y2);
	glEnd();
}

void OGL_SetColor(int palidx)
{
	byte rgb[3];

	if (palidx == -1)	// Invisible?
		glColor4f(0, 0, 0, 0);
	else
	{
		PalIdxToRGB(W_CacheLumpNum(pallump,PU_CACHE), palidx, rgb);
		glColor3f(rgb[0]/255.0, rgb[1]/255.0, rgb[2]/255.0);
	}
}

void OGL_SetColorAndAlpha(float r, float g, float b, float a)
{
	glColor4f(r, g, b, a);
}

// Filters correspond the palettes in the wad.
void OGL_SetFilter(int filter)
{
	curfilter = filter;
}

// Returns nonzero if the filter was drawn.
int OGL_DrawFilter(void)
{
	if (!curfilter)
		return 0;		// No filter needed.

	// No texture, please.
	glDisable(GL_TEXTURE_2D);

	// We have to choose the right color and alpha.
	if (curfilter >= STARTREDPALS && curfilter < STARTREDPALS+NUMREDPALS)
		// Red?
		glColor4f(1, 0, 0, curfilter/8.0);	// Full red with filter 8.
	else if (curfilter >= STARTBONUSPALS && curfilter < STARTBONUSPALS+NUMBONUSPALS)
		// Light Green?
		glColor4f(.5, 1, .5, (curfilter-STARTBONUSPALS+1)/12.0);
	else if (curfilter >= STARTPOISONPALS && curfilter < STARTPOISONPALS+NUMPOISONPALS)
		// Green?
		glColor4f(0, 1, 0, (curfilter-STARTPOISONPALS+1)/16.0);
	else if (curfilter >= STARTSCOURGEPAL)
		// Orange?
		glColor4f(1, .5, 0, (STARTSCOURGEPAL+3-curfilter)/6.0);
	else if(curfilter >= STARTHOLYPAL)
		// White?
		glColor4f(1, 1, 1, (STARTHOLYPAL+3-curfilter)/6.0);
	else if(curfilter == STARTICEPAL)
		// Light blue?
		glColor4f(.5f, .5f, 1, .4f);
	else
		I_Error("OGL_DrawFilter: Real strange filter number: %d.\n", curfilter);

	glBegin(GL_QUADS);
	glVertex2f(0, 0);
	glVertex2f(320, 0);
	glVertex2f(320, 200);
	glVertex2f(0, 200);
	glEnd();

	glEnable(GL_TEXTURE_2D);
	return 1;
}

