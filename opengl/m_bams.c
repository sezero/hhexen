//**************************************************************************
//**
//** m_bams.c
//**
//** $Revision$
//** $Date$
//**
//**************************************************************************

/* BAMS trigonometric functions. */

#include "h2stdinc.h"
#include <math.h>
#include "m_bams.h"

#define BAMS_TABLE_ACCURACY	2000

static binangle atantable[BAMS_TABLE_ACCURACY];

void bamsInit(void)	/* Fill in the tables */
{
	int	i;

	for (i = 1; i < BAMS_TABLE_ACCURACY; i++)
		atantable[i] = RAD2BANG(atan(i / (float)BAMS_TABLE_ACCURACY));
}

binangle bamsAtan2(int y, int x)
{
	binangle	bang;
	int	absy = y, absx = x;

	if (!x && !y)
		return BANG_0;	/* Indeterminate */

	/* Make sure the absolute values are absolute */
	if (absy < 0)
		absy = -absy;
	if (absx < 0)
		absx = -absx;

	/* We'll first determine what the angle is in the
	 * first quadrant. That's what the tables are for.
	 */
	if (!absy)
		bang = BANG_0;
	else if (absy == absx)
		bang = BANG_45;
	else if (!absx)
		bang = BANG_90;
	else
	{
		/* The special cases didn't help. Use the
		 * tables. absx and absy can't be zero here.
		 */
		if (absy > absx)
			bang = BANG_90 - atantable[(int)((float)absx / (float)absy*BAMS_TABLE_ACCURACY)];
		else
			bang = atantable[(int)((float)absy / (float)absx*BAMS_TABLE_ACCURACY)];
	}

	/* Now we know the angle in the first quadrant.
	 * Let's look at the signs and choose the right
	 * quadrant.
	 */
	if (x < 0)	/* Flip horizontally? */
		bang = BANG_180 - bang;
	if (y < 0)	/* Flip vertically? */
	{
	/* At the moment bang must be smaller than 180. */
		bang = BANG_180 + BANG_180 - bang;
	}
	/* This is the final angle. */
	return bang;
}

