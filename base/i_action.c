//**************************************************************************
//**
//** i_action.c : HHexen 1.3 : Dan Olson.
//**
//** $RCSfile: i_action.c,v $
//** $Revision: 1.1.1.1 $
//** $Date: 2000-04-11 17:38:03 $
//** $Author: theoddone33 $
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------
#include <string.h>
#include "h2def.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

int C_BindControl(char *con_name, boolean *ga);
void C_RegisterControl(control_t *con);
void C_ScanCodeDown(int code);
void C_ScanCodeUp(int code);
char *C_ScanToKey(int code);

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

control_t *FindControl(char *con_name);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

extern boolean ga[NUMGAMEACTIONS];

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

control_t *controllist;

// CODE --------------------------------------------------------------------

control_t *FindControl(char *con_name)
{
	control_t *con;

	for(con = controllist ; con ; con=con->next)
		if(!strcmp(con_name, con->name))
			return con;
	return NULL;
}

int C_BindControl(char *con_name, int action)
{
	control_t *con;

	con = FindControl(con_name);
	if(!con)
		return -1;	//Bad key name
	con->action = &ga[action];
	return 0;
}
void C_RegisterControl(control_t *con)
{
	//Is name used?
	if(FindControl(con->name))
	{
		fprintf(stderr, "Can't register control %s, name in use\n",
			con->name);
		return;
	}
	
	//Link her in
	con->next = controllist;
	controllist = con;
}

void C_ScanCodeDown(int code)
{
	control_t *con;

	for(con = controllist; con; con=con->next) {
		if(con->scancode == code) {
			*con->action = true;
		}
	}
}

void C_ScanCodeUp(int code)
{
	control_t *con;

	for(con = controllist; con; con=con->next) {
		if(con->scancode == code) {
			*con->action = false;
		}
	}
} 
char *C_ScanToKey(int code)
{
	control_t *con;

	for(con = controllist;con;con=con->next)
		if (con->scancode == code)
			return con->name;
	return NULL;
}
