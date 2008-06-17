//**************************************************************************
//**
//** c_console.c : HHexen : Dan Olson
//**
//**************************************************************************
// HEADER FILES ------------------------------------------------------------
#include "h2def.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

typedef struct
{
	char *name;
	int (*func)(int argc, char **argv);
} ccmd_t;

typedef struct
{
	char *name;
	boolean isToggle;
	int cvDefault;
} cvar_t;

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

int CFSetCvar(int argc, char **argv);
int CFListCvars(int argc, char **argv);
int CFListCmds( int argc, char **argv);
int CFQuit(int argc, char **argv);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------
boolean ConsoleActive;

// PRIVATE DATA DEFINITIONS ------------------------------------------------
cvar_t cVarList[] =
{
	{ "sensitivity", false, 5 },
	{"mlook", true, 0}
};

ccmd_t cCmdList[] =
{
	{"set", CFSetCvar},
	{"cvarlist", CFListCvars},
	{"cmdlist", CFListCmds},
	{"quit", CFQuit}
};



// CODE --------------------------------------------------------------------

int CFSetCvar(int argc, char **argv)
{
	return 0;
}

int CFListCvars(int argc, char **argv)
{
	return 0;
}

int CFListCmds(int argc, char **argv)
{
	return 0;
}

int CFQuit(int argc, char **argv)
{
	return 0;
}

