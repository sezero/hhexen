//**************************************************************************
//**
//** ogl_rl.h
//**
//** $Revision$
//** $Date$
//**
//**************************************************************************

#ifndef __OGL_REND_LIST_H__
#define __OGL_REND_LIST_H__

/* Rendquad flags. */
#define RQF_FLAT		0x1	/* This is a flat triangle. */
#define RQF_MASKED		0x2	/* Use the special list for masked textures. */
#define RQF_MISSING_WALL	0x4	/* Originally this surface had no texture. */
#define RQF_SKY_MASK		0x8	/* A sky mask triangle. */
#define RQF_SKY_MASK_WALL	0x10	/* A sky mask wall (with skyfix). */
#define RQF_LIGHT		0x20	/* A dynamic light. */
#define RQF_FLOOR_FACING	0x40	/* Used for flats, the quad faces upwards. */

typedef struct
{
	float	v1[2], v2[2];		/* Two vertices. */
	float	top;			/* Top height. */
	union _quadTri {
		struct _quad {
			float bottom;		/* Bottom height. */
			float len;		/* Length of the quad. */
		} q;
		float v3[2];		/* Third vertex for flats. */
	} u;
	float	light;			/* Light level, as in 0 = black, 1 = fullbright. */
	float	texoffx;		/* Texture coordinates for left/top (in real texcoords). */
	float	texoffy;
	short	flags;			/* RQF_*. */
	GLuint	masktex;		/* Texture name for masked textures. */
	unsigned short texw, texh;	/* Size of the texture. */
	float	dist[3];		/* Distances to the vertices. */
} rendquad_t;	/* Or flat triangle. */

typedef struct
{
	GLuint	tex;			/* The name of the texture for this list. */
	int		numquads;	/* Number of quads in the list. */
	int		listsize;	/* Absolute size of the list. */
	rendquad_t *quads;		/* The list of quads. */
} rendlist_t;


/* ogl_rl.c */

void RL_Init(void);
void RL_ClearLists(void);
void RL_DeleteLists(void);
void RL_AddQuad(rendquad_t *quad, GLuint quadtex);
void RL_AddFlatQuads(rendquad_t *base, /* GLuint */ uintptr_t quadtex,
				int numvrts, fvertex_t *vrts, int dir);
void RL_RenderAllLists(void);
void SetVertexColor(float light, float dist, float alpha);

#endif	/* __OGL_REND_LIST_H__ */

