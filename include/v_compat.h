
//**************************************************************************
//**
//** v_compat.h
//**
//** $Revision$
//** $Date$
//**
//**************************************************************************

#ifndef __V_COMPAT_H
#define __V_COMPAT_H

#ifdef RENDER3D
#include "ogl_def.h"
#endif

/* SetPalette */

#ifndef PLAYPAL_NUM
#define PLAYPAL_NUM		W_GetNumForName("PLAYPAL")
#endif

#if defined(RENDER3D)
#define V_SetPaletteBase()	OGL_SetFilter(0)
#define V_SetPaletteShift(num)	OGL_SetFilter((num))
#else
#define V_SetPaletteBase()	I_SetPalette((byte *)W_CacheLumpName("PLAYPAL", PU_CACHE))
#define V_SetPaletteShift(num)	I_SetPalette((byte *)W_CacheLumpNum(PLAYPAL_NUM, PU_CACHE) + (num)*768)
#endif

/* Minimal definitions for DrawPatch / DrawRawScreen stuff.
 * If more are needed, define in the relevant file.
 */
#if defined(RENDER3D)
#define PATCH_REF		int
#define INVALID_PATCH		0
#define BYTE_REF		int
#else
#define BYTE_REF		byte*
#define PATCH_REF		patch_t*
#define INVALID_PATCH		NULL
#endif

#endif	/* __V_COMPAT_H */

