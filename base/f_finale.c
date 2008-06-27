
//**************************************************************************
//**
//** f_finale.c : Heretic 2 : Raven Software, Corp.
//**
//** $RCSfile: f_finale.c,v $
//** $Revision: 1.17 $
//** $Date: 2008-06-27 22:45:37 $
//** $Author: sezero $
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "h2stdinc.h"
#include <ctype.h>
#include "h2def.h"
#include "soundst.h"
#include "p_local.h"
#ifdef RENDER3D
#include "ogl_def.h"
#endif

// MACROS ------------------------------------------------------------------

#define	TEXTSPEED	3
#define	TEXTWAIT	250

#include "v_compat.h"

#ifdef RENDER3D
#define W_CacheLumpName(a,b)		W_GetNumForName((a))
#define WR_CacheLumpNum(a,b)		(a)
#define V_DrawPatch(x,y,p)		OGL_DrawPatch((x),(y),(p))
#define V_DrawRawScreen(a)		OGL_DrawRawScreen((a))
#else
#define WR_CacheLumpNum(a,b)		W_CacheLumpNum((a),(b))
#endif

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static void TextWrite(void);
static void DrawPic(void);
#ifdef RENDER3D
static void FadeIn (void);
static void FadeOut(void);
#define InitializeFade(x)	do {} while (0)
#define DeInitializeFade()	do {} while (0)
#define FadePic()		do {} while (0)
#else
#define FadeIn()		do {} while (0)
#define FadeOut()		do {} while (0)
static void InitializeFade(boolean fadeIn);
static void DeInitializeFade(void);
static void FadePic(void);
#endif

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

extern boolean automapactive;
extern boolean viewactive;

// PUBLIC DATA DECLARATIONS ------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static int FinaleStage;
static int FinaleCount;
static int FinaleEndCount;
static int FinaleLumpNum;
static int FontABaseLump;
static const char *FinaleText;

#ifndef RENDER3D
static fixed_t *Palette;
static fixed_t *PaletteDelta;
static byte *RealPalette;
#endif

// CODE --------------------------------------------------------------------

//===========================================================================
//
// F_StartFinale
//
//===========================================================================

void F_StartFinale (void)
{
	gameaction = ga_nothing;
	gamestate = GS_FINALE;
	viewactive = false;
	automapactive = false;
	P_ClearMessage(&players[consoleplayer]);

	FinaleStage = 0;
	FinaleCount = 0;
	FinaleText = GetFinaleText(0);
	FinaleEndCount = 70;
	FinaleLumpNum = W_GetNumForName("FINALE1");
	FontABaseLump = W_GetNumForName("FONTA_S") + 1;
	InitializeFade(1);
#ifdef RENDER3D
	OGL_SetFilter(0);
#endif
//	S_ChangeMusic(mus_victor, true);
	S_StartSongName("hall", false); // don't loop the song
}

//===========================================================================
//
// F_Responder
//
//===========================================================================

boolean F_Responder(event_t *event)
{
	return false;
}

//===========================================================================
//
// F_Ticker
//
//===========================================================================

void F_Ticker (void)
{
	FinaleCount++;
	if (FinaleStage < 5 && FinaleCount >= FinaleEndCount)
	{
		FinaleCount = 0;
		FinaleStage++;
		switch (FinaleStage)
		{
		case 1: // Text 1
			FinaleEndCount = strlen(FinaleText)*TEXTSPEED + TEXTWAIT;
			break;
		case 2: // Pic 2, Text 2
			FinaleText = GetFinaleText(1);
			FinaleEndCount = strlen(FinaleText)*TEXTSPEED + TEXTWAIT;
			FinaleLumpNum = W_GetNumForName("FINALE2");
			S_StartSongName("orb", false);
			break;
		case 3: // Pic 2 -- Fade out
			FinaleEndCount = 70;
			DeInitializeFade();
			InitializeFade(0);
			break;
		case 4: // Pic 3 -- Fade in
			FinaleLumpNum = W_GetNumForName("FINALE3");
			FinaleEndCount = 71;
			DeInitializeFade();
			InitializeFade(1);
			S_StartSongName("chess", true);
			break;
		case 5: // Pic 3 , Text 3
			FinaleText = GetFinaleText(2);
			DeInitializeFade();
			break;
		default:
			break;
		}
		return;
	}
	if (FinaleStage == 0 || FinaleStage == 3 || FinaleStage == 4)
	{
		FadePic();
	}
}

//===========================================================================
//
// TextWrite
//
//===========================================================================

static void TextWrite (void)
{
	int		count;
	const char	*ch;
	int		c;
	int		cx, cy;
	patch_t		*w;
	int		width;

	V_DrawRawScreen((BYTE_REF) WR_CacheLumpNum(FinaleLumpNum, PU_CACHE));

	if (FinaleStage == 5)
	{ // Chess pic, draw the correct character graphic
		if (netgame)
		{
			V_DrawPatch(20, 0, (PATCH_REF)W_CacheLumpName("chessall", PU_CACHE));
		}
#ifdef ASSASSIN
		else if (PlayerClasses[consoleplayer] == PCLASS_ASS)
		{
			V_DrawPatch(60, 0, (PATCH_REF)WR_CacheLumpNum(W_GetNumForName("chessa"), PU_CACHE));
		}
#endif
		else if (PlayerClasses[consoleplayer])
		{
			V_DrawPatch(60, 0, (PATCH_REF)WR_CacheLumpNum(W_GetNumForName("chessc")
									+ PlayerClasses[consoleplayer] - 1, PU_CACHE));
		}
	}
	// Draw the actual text
	if (FinaleStage == 5)
	{
		cy = 135;
	}
	else
	{
		cy = 5;
	}
	cx = 20;
	ch = FinaleText;
	count = (FinaleCount - 10) / TEXTSPEED;
	if (count < 0)
	{
		count = 0;
	}
	for ( ; count; count--)
	{
		c = *ch++;
		if (!c)
		{
			break;
		}
		if (c == '\n')
		{
			cx = 20;
			cy += 9;
			continue;
		}
		if (c < 32)
		{
			continue;
		}
		c = toupper(c);
		if (c == 32)
		{
			cx += 5;
			continue;
		}
		w = (patch_t *) W_CacheLumpNum(FontABaseLump + c - 33, PU_CACHE);
		width = SHORT(w->width);
		if (cx + width > SCREENWIDTH)
		{
			break;
		}
#ifdef RENDER3D
		OGL_DrawPatch_CS(cx, cy, FontABaseLump + c - 33);
#else
		V_DrawPatch(cx, cy, w);
#endif
		cx += width;
	}
}

//===========================================================================
//
// InitializeFade
//
//===========================================================================

#if !defined(RENDER3D)
static void InitializeFade(boolean fadeIn)
{
	unsigned i;

	Palette = (fixed_t *) Z_Malloc(768*sizeof(fixed_t), PU_STATIC, NULL);
	PaletteDelta = (fixed_t *) Z_Malloc(768*sizeof(fixed_t), PU_STATIC, NULL);
	RealPalette = (byte *) Z_Malloc(768*sizeof(byte), PU_STATIC, NULL);

	if (fadeIn)
	{
		memset(RealPalette, 0, 768*sizeof(byte));
		for (i = 0; i < 768; i++)
		{
			Palette[i] = 0;
			PaletteDelta[i] = FixedDiv((*((byte *)W_CacheLumpName("playpal", PU_CACHE) + i))<<FRACBITS, 70*FRACUNIT);
		}
	}
	else
	{
		for (i = 0; i < 768; i++)
		{
			RealPalette[i] = *((byte *)W_CacheLumpName("playpal", PU_CACHE) + i);
			Palette[i] = RealPalette[i]<<FRACBITS;
			PaletteDelta[i] = FixedDiv(Palette[i], -70*FRACUNIT);
		}
	}
	I_SetPalette(RealPalette);
}

//===========================================================================
//
// DeInitializeFade
//
//===========================================================================

static void DeInitializeFade(void)
{
	Z_Free(Palette);
	Z_Free(PaletteDelta);
	Z_Free(RealPalette);
}

//===========================================================================
//
// FadePic
//
//===========================================================================

static void FadePic(void)
{
	unsigned i;

	for (i = 0; i < 768; i++)
	{
		Palette[i] += PaletteDelta[i];
		RealPalette[i] = Palette[i]>>FRACBITS;
	}
	I_SetPalette(RealPalette);
}

#else	/* ! RENDER3D */

/* For OpenGL: adapted from the Vavoom project.
 *
 * The FinaleCount / FinaleEndCount tick rates seem to work fine
 * for us here:  for FinaleStage 0, 3 and 4, where this code is
 * used, FinaleEndCount is set to 70 or 71, FinaleCount is reset
 * to 0 and incremented at every tick, and this gives us about 2
 * seconds of fade-in/fade-out.
 */
static void FadeOut (void)
{
	float fade = (float)FinaleCount / (float)FinaleEndCount;

	if (fade < 0.0)
		fade = 0.0;
	if (fade > 0.99)
		fade = 0.99;
	OGL_ShadeRect(0, 0, 320, 200, fade);
}

static void FadeIn (void)
{
	float fade = 1.0 - (float)FinaleCount / (float)FinaleEndCount;

	if (fade < 0.0)
		fade = 0.0;
	if (fade > 0.99)
		fade = 0.99;
	OGL_ShadeRect(0, 0, 320, 200, fade);
}
#endif	/* ! RENDER3D */

//===========================================================================
//
// DrawPic
//
//===========================================================================

static void DrawPic(void)
{
	V_DrawRawScreen((BYTE_REF) WR_CacheLumpNum(FinaleLumpNum, PU_CACHE));

	if (FinaleStage == 4 || FinaleStage == 5)
	{ // Chess pic, draw the correct character graphic
		if (netgame)
		{
			V_DrawPatch(20, 0, (PATCH_REF)W_CacheLumpName("chessall", PU_CACHE));
		}
		else if (PlayerClasses[consoleplayer])
		{
			V_DrawPatch(60, 0, (PATCH_REF)WR_CacheLumpNum(W_GetNumForName("chessc")
									+ PlayerClasses[consoleplayer] - 1, PU_CACHE));
		}
	}
}

//===========================================================================
//
// F_Drawer
//
//===========================================================================

void F_Drawer(void)
{
	switch (FinaleStage)
	{
	case 0: // Fade in initial finale screen
		DrawPic();
		FadeIn();
		break;
	case 1:
	case 2:
		TextWrite();
		break;
	case 3: // Fade screen out
		DrawPic();
		FadeOut();
		break;
	case 4: // Fade in chess screen
		DrawPic();
		FadeIn();
		break;
	case 5:
		TextWrite();
		break;
	}
	UpdateState |= I_FULLSCRN;
}

