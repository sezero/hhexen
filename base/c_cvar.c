//*************************************************************************
//**
//** cvar.c : HHexen : Dan Olson
//**
//** This material is not supported by Activision
//**
//*************************************************************************

// HEADER FILES -----------------------------------------------------------

#include <string.h>
#include "h2def.h"
#include "cvar.h"

// MACROS -----------------------------------------------------------------

// TYPES ------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES -------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ---------------------------------------------

// PRIVATE FUNCTION PROTOTYPES --------------------------------------------

void	CV_Init (void);
cvar_t	*CV_Find (char *name);
void	CV_Set (char *name, char *string);
cvar_t	*CV_Get (char *name, char *string);
void	CV_Shutdown (void);

// EXTERNAL DATA DECLARATIONS ---------------------------------------------

// PUBLIC DATA DEFINITIONS ------------------------------------------------

// PRIVATE DATA DEFINITIONS -----------------------------------------------

cvar_t	*cvar_list;

// CODE -------------------------------------------------------------------

void CV_Init (void)
{
}

cvar_t	*CV_Find (char *name)
{
	cvar_t	*c;

	for (c = cvar_list ; c ; c = c->next)
	{
		if (!strcmp (c->name, name))
			return c;
	}
	return NULL;
}

void CV_Set (char *name, char *string)
{
	cvar_t	*c;

	c = CV_Find (name);
	if (!c)
	{
		ST_Message ("Tried to access non-existent cvar.\n");
		return;
	}
	c->value = atof(string);
	c->string = string;
}

cvar_t	*CV_Get (char *name, char *string)
{
	cvar_t	*c;

	if ((c = CV_Find(name)) != NULL)
	{
		// Malloc
		c = (cvar_t *) malloc (sizeof (cvar_t));
		c->name = strdup (name);
		c->string = strdup (string);
		// Set
		c->next = cvar_list;
		cvar_list = c;
		CV_Set (name, string);
	}
	return c;
}

void CV_Shutdown (void)
{
	cvar_t	*v,*next;

	v = cvar_list;

	while (v)
	{
		next = v->next;
		free (v->name);
		free (v->string);
		free (v);
		v = next;
	}
}
