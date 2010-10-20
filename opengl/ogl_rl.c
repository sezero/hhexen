
//**************************************************************************
//**
//** OGL_RL.C
//**
//** $Revision$
//** $Date$
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "h2stdinc.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <math.h>
#include <GL/gl.h>
#include "h2def.h"
#include "ogl_def.h"
#include "ogl_rl.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

extern int		skyhemispheres;
extern int		dlMaxRad;	/* Dynamic lights maximum radius. */

// PUBLIC DATA DEFINITIONS -------------------------------------------------

int			numrlists = 0;	/* Number of rendering lists. */
float			maxLightDist = 1024;

subsector_t		*currentssec;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static rendlist_t *rlists = NULL;	/* The list of rendering lists. */
static rendlist_t masked_rlist;		/* Rendering list for masked textures. */
static rendlist_t invsky_rlist;		/* List for invisible sky triangles. */
static rendlist_t invskywall_rlist;	/* List for invisible sky walls (w/skyfix). */
static rendlist_t dlwall_rlist, dlflat_rlist; /* Dynamic lighting for walls/flats. */

// CODE --------------------------------------------------------------------

void RL_Init(void)
{
	numrlists = 0;
	rlists = 0;
	memset(&masked_rlist, 0, sizeof(masked_rlist));
	memset(&invsky_rlist, 0, sizeof(invsky_rlist));
	memset(&invskywall_rlist, 0, sizeof(invskywall_rlist));
	memset(&dlwall_rlist, 0, sizeof(dlwall_rlist));
	memset(&dlflat_rlist, 0, sizeof(dlflat_rlist));
}

void RL_DestroyList(rendlist_t *rl)
{
	// All the list data will be destroyed.
	free(rl->quads);
	memset(rl, 0, sizeof(rendlist_t));
}

void RL_DeleteLists(void)
{
	int		i;

	// Delete all lists.
	for (i = 0; i < numrlists; i++)
		RL_DestroyList(rlists+i);
	RL_DestroyList(&masked_rlist);
	RL_DestroyList(&invsky_rlist);
	RL_DestroyList(&invskywall_rlist);
	RL_DestroyList(&dlwall_rlist);
	RL_DestroyList(&dlflat_rlist);

	free(rlists);
	rlists = NULL;
	numrlists = 0;
}

// Reset the indices.
void RL_ClearLists(void)
{
	int		i;
	for (i = 0; i < numrlists; i++)
		rlists[i].numquads = 0;
	masked_rlist.numquads = 0;
	invsky_rlist.numquads = 0;
	invskywall_rlist.numquads = 0;
	dlwall_rlist.numquads = 0;
	dlflat_rlist.numquads = 0;
	skyhemispheres = 0;
}

static void RL_DynLightQuad(rendquad_t *quad, lumobj_t *lum)
{
	// Prepare the texture so we know its dimensions.
	OGL_PrepareLightTexture();

	// Is the quad a wall or a flat?
	if (quad->flags & RQF_FLAT)
	{
		// Process all marked light sources.
		quad->flags |= RQF_LIGHT;
		quad->texw = texw * 2;
		quad->texh = texh * 2;

		// Determine the light level.

		// The texture offset is what actually determines where the
		// dynamic light map will be rendered. For light quads the
		// texture offset is global.
		quad->texoffx = FIX2FLT(lum->thing->x) + dlMaxRad;
		quad->texoffy = FIX2FLT(lum->thing->y) + dlMaxRad;
	}
}

static rendlist_t *RL_FindList(GLuint tex)
{
	int	i;
	rendlist_t *dest;

	for (i = 0; i < numrlists; i++)
	{
		if (rlists[i].tex == tex)
			return rlists + i;
	}

	// Then create a new list.
	rlists = (rendlist_t *) realloc (rlists, sizeof(rendlist_t) * ++numrlists);
	dest = rlists + numrlists - 1;
	memset(dest, 0, sizeof(rendlist_t));
	dest->tex = tex;
	return dest;
}

// Here we will add the quad to the correct rendering list, creating
// a new list if necessary.
void RL_AddQuad(rendquad_t *quad, GLuint quadtex)
{
	rendlist_t	*dest = NULL;	// The destination list.
	rendquad_t	*dq;		// Quad in the dest list.

	// Masked quads go to the masked list.
	if (quad->flags & RQF_MASKED)
		dest = &masked_rlist;
	else if (quad->flags & RQF_SKY_MASK_WALL)
		dest = &invskywall_rlist;
	else if (quad->flags & RQF_LIGHT) // Dynamic lights?
		dest = &dlwall_rlist;
	else
	{
		// Find a suitable list. This can get a bit redundant for large
		// numbers of primitives of the same texture (oh yeah, this is
		// a real cycle-eater).
		dest = RL_FindList(quadtex);
	}
	// Now we have a destination list. This is the only place where
	// quads are added.
	if (++dest->numquads > dest->listsize)	// See if we have to allocate more memory.
	{
		dest->listsize = dest->numquads + 10;
		dest->quads = (rendquad_t *) realloc (dest->quads,
						dest->listsize * sizeof(rendquad_t));
	}
	dq = dest->quads + dest->numquads - 1;
	memcpy(dq, quad, sizeof(rendquad_t));

	// Let a masked quad know its texture.
	if (quad->flags & RQF_MASKED)
	{
		dq->masktex = quadtex;
		if (!quadtex)
			I_Error("RL_AddQuad: There can't be a masked quad with no texture.\n");
	}
}

// Adds a series of flat quads (triangles) as a fan.
void RL_AddFlatQuads(rendquad_t *base, GLuint quadtex, int numvrts,
					fvertex_t *origvrts, int dir)
{
	fvertex_t	*vtx;
	rendlist_t	*dest;
	rendquad_t	*qi;
	int		i, firstquad;
	float		*distances, middist;
	fvertex_t	*vrts;

	if (!numvrts)
		return;	// No data, can't do anything.

	if (base->flags & RQF_SKY_MASK)
		dest = &invsky_rlist;
	else if (base->flags & RQF_LIGHT)
		dest = &dlflat_rlist;
	else
		// First find the right list.
		dest = RL_FindList(quadtex);

	// Check that there's enough room.
	firstquad = dest->numquads;
	dest->numquads += numvrts - 2;
	if (dest->numquads > dest->listsize)
	{
		// Allocate more memory.
		dest->listsize = dest->numquads + 20;
		dest->quads = (rendquad_t * ) realloc (dest->quads,
					dest->listsize * sizeof(rendquad_t));
	}

	// Calculate the distance to each vertex.
	distances = (float *) malloc (numvrts * sizeof(float));
	for (i = 0; i < numvrts; i++)
		distances[i] = PointDist2D(&origvrts[i].x);

	// Make a distance modification.
	vrts = (fvertex_t *) malloc (numvrts * sizeof(fvertex_t));
	memcpy(vrts, origvrts, numvrts * sizeof(fvertex_t));
	middist = PointDist2D(&currentssec->midpoint.x);
	if (!(base->flags & RQF_LIGHT) && middist > 256)
	{
		for (i = 0; i<numvrts; i++)
		{
			float dx = vrts[i].x - currentssec->midpoint.x,
				dy = vrts[i].y - currentssec->midpoint.y,
				dlen = sqrt(dx*dx + dy*dy);
			if (!dlen)
				continue;
			vrts[i].x += dx/dlen * (middist-256)/128;
			vrts[i].y += dy/dlen * (middist-256)/128;
		}
	}

	// Add them as a fan.
	if (dir == 0)	// Normal direction.
	{
		// All triangles share the first vertex.
		base->v1[VX] = vrts->x;
		base->v1[VY] = vrts->y;
		base->dist[0] = distances[0];

		for (i = 2, qi = dest->quads + firstquad; i < numvrts; i++, qi++)
		{
			memcpy(qi, base, sizeof(rendquad_t));
			// The second vertex is the previous from this one.
			vtx = vrts + i - 1;
			qi->v2[VX] = vtx->x;
			qi->v2[VY] = vtx->y;
			qi->dist[1] = distances[i-1];
			// The third vertex is naturally the current one.
			vtx = vrts + i;
			qi->u.v3[VX] = vtx->x;
			qi->u.v3[VY] = vtx->y;
			qi->dist[2] = distances[i];

			if (base->flags & RQF_LIGHT)
				RL_DynLightQuad(qi, (lumobj_t*)quadtex); // FIXME: integer to pointer cast!!!
		}
	}
	else	// Reverse direction?
	{
		// All triangles share the last vertex.
		vtx = vrts + numvrts - 1;
		base->v1[VX] = vtx->x;
		base->v1[VY] = vtx->y;
		base->dist[0] = distances[numvrts-1];

		for (i = numvrts - 3, qi = dest->quads + firstquad; i >= 0; i--, qi++)
		{
			memcpy(qi, base, sizeof(rendquad_t));
			// The second vertex is the next from this one.
			vtx = vrts + i + 1;
			qi->v2[VX] = vtx->x;
			qi->v2[VY] = vtx->y;
			qi->dist[1] = distances[i+1];
			// The third vertex is naturally the current one.
			vtx = vrts + i;
			qi->u.v3[VX] = vtx->x;
			qi->u.v3[VY] = vtx->y;
			qi->dist[2] = distances[i];

			if (base->flags & RQF_LIGHT)
				RL_DynLightQuad(qi, (lumobj_t*)quadtex); // FIXME: integer to pointer cast!!!
		}
	}
	free (vrts);
	free (distances);
}

void SetVertexColor(float light, float dist, float alpha)
{
	float real = light - (dist - 32) / maxLightDist * (1 - light);
	float minimum = light*light + (light - .63) / 2;
	if (real < minimum)
		real = minimum; // Clamp it.
	// Add extra light.
	real += extralight/16.0;
	// Check for torch.
	if (viewplayer->fixedcolormap)
	{
		// Colormap 1 is the brightest. I'm guessing 16 would be the darkest.
		int ll = 16 - viewplayer->fixedcolormap;
		float d = (1024 - dist) / 512.0;
		float newmin = d * ll / 15.0;
		if (real < newmin)
			real = newmin;
	}
	glColor4f(real, real, real, alpha);	// Too real? Hope real gets clamped.
}

// This is only for solid, non-masked primitives.
void RL_RenderList(rendlist_t *rl)
{
	int			i;
	float		tcleft, tcright, tctop, tcbottom;
	rendquad_t	*cq;

	if (!rl->numquads)
		return;	// The list is empty.

	// Bind the right texture.
	glBindTexture(GL_TEXTURE_2D, curtex = rl->tex);

	// Check what kind of primitives there are on the list.
	// There can only be one kind on each list.
	if (rl->quads->flags & RQF_FLAT)	// Check the first primitive.
	{
		// There's only triangles here, I see.
		glBegin(GL_TRIANGLES);
		for (i = 0; i < rl->numquads; i++)
		{
			cq = rl->quads + i;
			if (cq->flags & RQF_MISSING_WALL)
			{
				// This triangle is REALLY a quad that originally had no 
				// texture. We have to render it as two triangles.
				tcright = (tcleft = 0) + cq->u.q.len/cq->texw;
				tcbottom = (tctop = 0) + (cq->top - cq->u.q.bottom)/cq->texh;

				SetVertexColor(cq->light, cq->dist[0], 1);
				glTexCoord2f(tcleft, tctop);
				glVertex3f(cq->v1[VX], cq->top, cq->v1[VY]);

				SetVertexColor(cq->light, cq->dist[1], 1);
				glTexCoord2f(tcright, tctop);
				glVertex3f(cq->v2[VX], cq->top, cq->v2[VY]);

				glTexCoord2f(tcright, tcbottom);
				glVertex3f(cq->v2[VX], cq->u.q.bottom, cq->v2[VY]);

				// The other triangle.
				glTexCoord2f(tcright, tcbottom);
				glVertex3f(cq->v2[VX], cq->u.q.bottom, cq->v2[VY]);

				SetVertexColor(cq->light, cq->dist[0], 1);
				glTexCoord2f(tcleft, tcbottom);
				glVertex3f(cq->v1[VX], cq->u.q.bottom, cq->v1[VY]);

				glTexCoord2f(tcleft, tctop);
				glVertex3f(cq->v1[VX], cq->top, cq->v1[VY]);
				continue;
			}

			// The vertices.
			SetVertexColor(cq->light, cq->dist[0], 1);
			glTexCoord2f((cq->v1[VX] + cq->texoffx)/cq->texw,
					(cq->v1[VY] + cq->texoffy)/cq->texh);
			glVertex3f(cq->v1[VX], cq->top, cq->v1[VY]);

			SetVertexColor(cq->light, cq->dist[1], 1);
			glTexCoord2f((cq->v2[VX] + cq->texoffx)/cq->texw,
					(cq->v2[VY] + cq->texoffy)/cq->texh);
			glVertex3f(cq->v2[VX], cq->top, cq->v2[VY]);

			SetVertexColor(cq->light, cq->dist[2], 1);
			glTexCoord2f((cq->u.v3[VX] + cq->texoffx)/cq->texw,
					(cq->u.v3[VY] + cq->texoffy)/cq->texh);
			glVertex3f(cq->u.v3[VX], cq->top, cq->u.v3[VY]);
		}
		glEnd();
	}
	else
	{
		// Render quads.
		glBegin(GL_QUADS);
		for (i = 0; i < rl->numquads; i++)
		{
			cq = rl->quads + i;

			// Calculate relative texture coordinates.
			tcright = (tcleft = cq->texoffx/(float)cq->texw) + cq->u.q.len/cq->texw;
			tcbottom = (tctop = cq->texoffy/cq->texh) + (cq->top - cq->u.q.bottom)/cq->texh;

			// The vertices.
			SetVertexColor(cq->light, cq->dist[0], 1);
			glTexCoord2f(tcleft, tcbottom);
			glVertex3f(cq->v1[VX], cq->u.q.bottom, cq->v1[VY]);

			glTexCoord2f(tcleft, tctop);
			glVertex3f(cq->v1[VX], cq->top, cq->v1[VY]);

			SetVertexColor(cq->light, cq->dist[1], 1);
			glTexCoord2f(tcright, tctop);
			glVertex3f(cq->v2[VX], cq->top, cq->v2[VY]);

			glTexCoord2f(tcright, tcbottom);
			glVertex3f(cq->v2[VX], cq->u.q.bottom, cq->v2[VY]);
		}
		glEnd();
	}
}

// Masked lists only include quads.
void RL_RenderMaskedList(rendlist_t *mrl)
{
	int			i;
	float		tcleft, tcright, tctop, tcbottom;
	rendquad_t	*cq;

	if (!mrl->numquads)
		return;	// No quads to render, I'm afraid.

	// Curtex is used to keep track of the current texture.
	// Zero also denotes that no glBegin() has yet been called.
	curtex = 0;

	// Render quads.
	for (i = mrl->numquads-1; i >= 0; i--)	// Render back to front.
	{
		cq = mrl->quads + i;

		// Calculate relative texture coordinates.
		tcright = (tcleft = cq->texoffx/cq->texw) + cq->u.q.len/cq->texw;
		tcbottom = (tctop = cq->texoffy/cq->texh) + (cq->top - cq->u.q.bottom)/cq->texh;

		// Is there a need to change the texture?
		if (curtex != cq->masktex)
		{
			if (curtex)
				glEnd();	// Finish with the old texture.
			glBindTexture(GL_TEXTURE_2D, curtex = cq->masktex);
			glBegin(GL_QUADS);	// I love OpenGL.
		}

		// The vertices.
		SetVertexColor(cq->light, cq->dist[0], 1);
		glTexCoord2f(tcleft, tcbottom);
		glVertex3f(cq->v1[VX], cq->u.q.bottom, cq->v1[VY]);

		glTexCoord2f(tcleft, tctop);
		glVertex3f(cq->v1[VX], cq->top, cq->v1[VY]);

		SetVertexColor(cq->light, cq->dist[1], 1);
		glTexCoord2f(tcright, tctop);
		glVertex3f(cq->v2[VX], cq->top, cq->v2[VY]);

		glTexCoord2f(tcright, tcbottom);
		glVertex3f(cq->v2[VX], cq->u.q.bottom, cq->v2[VY]);
	}
	if (curtex)
		glEnd();	// If something was drawn, finish with it.
}

void RL_RenderSkyMaskLists(void)
{
#if 0
	int			i;
	rendlist_t	*smrl = &invsky_rlist, *skyw = &invskywall_rlist;
	rendquad_t	*cq;

	if (!smrl->numquads && !skyw->numquads)
		return; // Nothing to do here.

	// Nothing gets written to the color buffer.

	glDisable(GL_TEXTURE_2D);

	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glColor4f(1, 1, 1, 1);	// Just to be sure.

	if (smrl->numquads)
	{
		glBegin(GL_TRIANGLES);
		for (i = 0; i < smrl->numquads; i++)
		{
			cq = smrl->quads + i;
			// ONLY the vertices, please.
			glVertex3f(cq->v1[VX], cq->top, cq->v1[VY]);
			glVertex3f(cq->v2[VX], cq->top, cq->v2[VY]);
			glVertex3f(cq->u.v3[VX], cq->top, cq->u.v3[VY]);
		}
		glEnd();
	}

	// Then the walls.
	if (skyw->numquads)
	{
		glBegin(GL_QUADS);
		for (i = 0; i < skyw->numquads; i++)
		{
			cq = skyw->quads + i;
			// Only the verts.
			glVertex3f(cq->v1[VX], cq->u.q.bottom, cq->v1[VY]);
			glVertex3f(cq->v1[VX], cq->top, cq->v1[VY]);
			glVertex3f(cq->v2[VX], cq->top, cq->v2[VY]);
			glVertex3f(cq->v2[VX], cq->u.q.bottom, cq->v2[VY]);
		}
		glEnd();
	}

	glEnable(GL_TEXTURE_2D);

	// Restore normal write mode.
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
#endif
}

void RL_RenderDynLightLists(void)
{
	int		i;
	rendlist_t	*frl = &dlflat_rlist, *wrl = &dlwall_rlist;
	rendquad_t	*cq;

	if (!frl->numquads && !wrl->numquads)
		return; // Nothing to do.

	// Setup the correct rendering state.
	glPushAttrib(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_ENABLE_BIT);
	// Disable fog.
	glDisable(GL_FOG);
	// This'll allow multiple light quads to be rendered on top of each other.
	glDepthMask(GL_FALSE);
	glDepthFunc(GL_LEQUAL);
	// Set up addition blending. The source is added to the destination.
	glBlendFunc(GL_DST_COLOR, GL_ONE);

	// The light texture.
	glBindTexture(GL_TEXTURE_2D, curtex = OGL_PrepareLightTexture());

	// The flats.
	if (frl->numquads)
	{
		glBegin(GL_TRIANGLES);
		for (i = 0; i < frl->numquads; i++)
		{
			cq = frl->quads + i;
			// Set the color.
			glColor3f(cq->light, cq->light, cq->light);
			// The vertices.
			glTexCoord2f((cq->texoffx - cq->v1[VX])/cq->texw,
					(cq->texoffy - cq->v1[VY])/cq->texh);
			glVertex3f(cq->v1[VX], cq->top, cq->v1[VY]);

			glTexCoord2f((cq->texoffx - cq->v2[VX])/cq->texw,
					(cq->texoffy - cq->v2[VY])/cq->texh);
			glVertex3f(cq->v2[VX], cq->top, cq->v2[VY]);

			glTexCoord2f((cq->texoffx - cq->u.v3[VX])/cq->texw,
					(cq->texoffy - cq->u.v3[VY])/cq->texh);
			glVertex3f(cq->u.v3[VX], cq->top, cq->u.v3[VY]);
		}
		glEnd();
	}

	// The walls.
	if (wrl->numquads)
	{
		float tctl[2], tcbr[2];	// Top left and bottom right.
		glBegin(GL_QUADS);
		for (i = 0; i < wrl->numquads; i++)
		{
			cq = wrl->quads + i;
			// Set the color.
			glColor3f(cq->light, cq->light, cq->light);

			// Calculate the texture coordinates.
			tcbr[VX] = (tctl[VX] = -cq->texoffx/cq->texw) + cq->u.q.len/cq->texw;
			tcbr[VY] = (tctl[VY] = cq->texoffy/cq->texh) + (cq->top - cq->u.q.bottom)/cq->texh;

			// The vertices.
			glTexCoord2f(tctl[VX], tcbr[VY]);
			glVertex3f(cq->v1[VX], cq->u.q.bottom, cq->v1[VY]);

			glTexCoord2f(tctl[VX], tctl[VY]);
			glVertex3f(cq->v1[VX], cq->top, cq->v1[VY]);

			glTexCoord2f(tcbr[VX], tctl[VY]);
			glVertex3f(cq->v2[VX], cq->top, cq->v2[VY]);

			glTexCoord2f(tcbr[VX], tcbr[VY]);
			glVertex3f(cq->v2[VX], cq->u.q.bottom, cq->v2[VY]);
		}
		glEnd();
	}

	// Restore the original rendering state.
	glPopAttrib();
}

void RL_RenderAllLists(void)
{
	int	i;

	// The sky might be visible. Render the needed hemispheres.
	R_RenderSkyHemispheres(skyhemispheres);
	RL_RenderSkyMaskLists(); //(&invsky_rlist, &invskywall_rlist);
	for (i = 0; i < numrlists; i++)
		RL_RenderList(rlists + i);
	RL_RenderDynLightLists();
	RL_RenderMaskedList(&masked_rlist);
}

