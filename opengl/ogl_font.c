// OpenGL Font Renderer.
// The font must be small enough to fit one texture (not a problem with *real*
// graphics cars!).

#ifdef __WIN32__
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "tga.h"
#else
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

#include <GL/gl.h>
#include <GL/glu.h>
#include <stdlib.h>
#include "ogl_font.h"
#include <stdio.h>
#include <string.h> /* fix memmove warning */

static int		initOk = 0;
static int		numFonts;
static jfrfont_t *fonts;		// The list of fonts.
static int		current;		// Index of the current font.

extern int test3dfx;

// Returns zero if no errors.
int FR_Init()
{
	if(initOk) return -1; // No reinitializations...
	
	numFonts = 0;
	fonts = 0;			// No fonts!
	current = -1;
	initOk = 1;
	return 0;
}

// Destroys the font with the index.
static void FR_DestroyFontIdx(int idx)
{
	jfrfont_t *font = fonts + idx;
	
	glDeleteTextures(1, &font->texture); 
	memmove(fonts+idx, fonts+idx+1, sizeof(jfrfont_t)*(numFonts-idx-1));
	numFonts--;
	fonts = realloc(fonts, sizeof(jfrfont_t)*numFonts);
	if(current == idx) current = -1;
}

void FR_Shutdown()
{
	// Destroy all fonts.
	while(numFonts) FR_DestroyFontIdx(0);
	fonts = 0;
	current = -1;
	initOk = 0;
}

int FR_GetFontIdx(int id)
{
	int i;
	for(i=0; i<numFonts; i++)
		if(fonts[i].id == id) return i;
	return -1;
}

jfrfont_t *FR_GetFont(int id)
{
	int idx = FR_GetFontIdx(id);
	if(idx == -1) return 0;
	return fonts + idx;
}

/*
static int FR_GetMaxId()
{
	int	i, grid=0;
	for(i=0; i<numFonts; i++)
		if(fonts[i].id > grid) grid = fonts[i].id;
	return grid;
}

static int findPow2(int num)
{
	int cumul;
	for(cumul=1; num > cumul; cumul *= 2);
	return cumul;
}
*/

#ifdef __WIN32__
// Prepare a GDI font. Select it as the current font.
int FR_PrepareGDIFont(HFONT hfont)
{
	jfrfont_t	*font;
	int			i, x, y, maxh, bmpWidth=256, bmpHeight=0, imgWidth, imgHeight;
	HDC			hdc;
	HBITMAP		hbmp;
	unsigned int *image;

	// Create a new font.
	fonts = realloc(fonts, sizeof(jfrfont_t) * ++numFonts);
	font = fonts + numFonts-1;
	memset(font, 0, sizeof(jfrfont_t));
	font->id = FR_GetMaxId() + 1;
	current = numFonts - 1;	
		
	// Now we'll create the actual data.
	hdc = CreateCompatibleDC(NULL);
	SetMapMode(hdc, MM_TEXT);
	SelectObject(hdc, hfont);
	// Let's first find out the sizes of all the characters.
	// Then we can decide how large a texture we need.
	for(i=0, x=0, y=0, maxh=0; i<256; i++)
	{
		jfrchar_t *fc = font->chars + i;
		SIZE size;
		char ch[2] = { i, 0 };
		GetTextExtentPoint32(hdc, ch, 1, &size);
		fc->w = size.cx;
		fc->h = size.cy;
		maxh = max(maxh, fc->h);
		x += fc->w;
		if(x >= bmpWidth)	
		{
			x = 0;
			y += maxh;
			maxh = 0;
		}
	}
	bmpHeight = y + maxh;
	hbmp = CreateCompatibleBitmap(hdc, bmpWidth, bmpHeight);
	SelectObject(hdc, hbmp);
	SetBkMode(hdc, OPAQUE);
	SetBkColor(hdc, 0);
	SetTextColor(hdc, 0xffffff);
	// Print all the characters.
	for(i=0, x=0, y=0, maxh=0; i<256; i++)
	{
		jfrchar_t *fc = font->chars + i;
		char ch[2] = { i, 0 };
		if(x+fc->w >= bmpWidth)
		{
			x = 0;
			y += maxh;
			maxh = 0;
		}
		if(i) TextOut(hdc, x, y, ch, 1);
		fc->x = x;
		fc->y = y;
		maxh = max(maxh, fc->h);
		x += fc->w;
	}
	// Now we can make a version that OpenGL can read.	
	imgWidth = findPow2(bmpWidth);
	imgHeight = findPow2(bmpHeight);
//	printf( "font: %d x %d\n", imgWidth, imgHeight);
	image = malloc(4*imgWidth*imgHeight);
	memset(image, 0, 4*imgWidth*imgHeight);
	for(y=0; y<bmpHeight; y++)
		for(x=0; x<bmpWidth; x++)
			if(GetPixel(hdc, x, y))
				image[x + y*imgWidth] = 0xffffffff;

	//if(test3dfx) saveTGA24_rgba8888("jhxfont.tga", bmpWidth, bmpHeight, (unsigned char*)image);

	font->texWidth = imgWidth;
	font->texHeight = imgHeight;

	// Create the OpenGL texture.
	glGenTextures(1, &font->texture);
	glBindTexture(GL_TEXTURE_2D, font->texture);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, imgWidth, imgHeight, 0, GL_RGBA, 
		GL_UNSIGNED_BYTE, image);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	
	// We no longer need these.
	free(image);
	DeleteObject(hbmp);
	DeleteDC(hdc);
	return 0;
}
#else
int FR_PrepareGDIFont(void* hfont)
{
	return 0;
}
#endif

// Change the current font.
void FR_SetFont(int id)
{
	int idx = FR_GetFontIdx(id);	
	if(idx == -1) return;	// No such font.
	current = idx;
}

int FR_TextWidth(char *text)
{
	int i, width = 0, len = strlen(text);
	jfrfont_t *cf;

	if(current == -1) return 0;
	
	// Just add them together.
	for(cf=fonts+current, i=0; i<len; i++)
		width += cf->chars[(int)text[i]].w;
	
	return width;
}

int FR_TextHeight(char *text)
{
	int i, height = 0, len;
	jfrfont_t *cf;

	if(current == -1 || !text) return 0;

	// Find the greatest height.
	for(len=strlen(text), cf=fonts+current, i=0; i<len; i++)
		height = max(height, cf->chars[(int)text[i]].h);

	return height;
}

// (x,y) is the upper left corner. Returns the length.
int FR_TextOut(int x, int y, char *text)
{
	int i, width=0, len;
	jfrfont_t *cf;

	if(!text) return 0;
	len = strlen(text);

	// Check the font.
	if(current == -1) return 0;	// No selected font.
	cf = fonts + current;

	/*glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glLoadIdentity();
	glScaled(1.0/cf->texWidth, 1.0/cf->texHeight, 1);*/

	// Set the texture.
	glBindTexture(GL_TEXTURE_2D, cf->texture);

	// Print it.
	glBegin(GL_QUADS);
	for(i=0; i<len; i++)
	{
		jfrchar_t *ch = cf->chars + text[i];
		float texw = (float)cf->texWidth, texh = (float)cf->texHeight;

		// Upper left.
		glTexCoord2f(ch->x/texw, ch->y/texh);
		glVertex2i(x, y);
		// Upper right.
		glTexCoord2f((ch->x+ch->w)/texw, ch->y/texh);
		glVertex2i(x+ch->w, y);
		// Lower right.
		glTexCoord2f((ch->x+ch->w)/texw, (ch->y+ch->h)/texh);
		glVertex2i(x+ch->w, y+ch->h);
		// Lower left.
		glTexCoord2f(ch->x/texw, (ch->y+ch->h)/texh);
		glVertex2i(x, y+ch->h);
		// Move on.
		width += ch->w;
		x += ch->w;
	}
	if(test3dfx)
	{
		glTexCoord2f(0, 0);
		glVertex2f(320, 0);
		glTexCoord2f(1, 0);
		glVertex2f(640, 0);
		glTexCoord2f(1, 1);
		glVertex2f(640, 160);
		glTexCoord2f(0, 1);
		glVertex2f(320, 160);
	}
	glEnd();

	//glPopMatrix();
	return width;
}

int FR_GetCurrent()
{
	if(current == -1) return 0;
	return fonts[current].id;
}
