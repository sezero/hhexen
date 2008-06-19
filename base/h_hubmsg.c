
//**************************************************************************
//**
//** h_hubmsg.c : Heretic 2 : Raven Software, Corp.
//**
//** $RCSfile: h_hubmsg.c,v $
//** $Revision: 1.1 $
//** $Date: 2008-06-19 06:23:20 $
//** $Author: sezero $
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "h2stdinc.h"
#include "h2def.h"
#include "h_hubmsg.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static char ClusterMessage[MAX_INTRMSN_MESSAGE_SIZE];

static char *ClusMsgLumpNames[] =
{
	"clus1msg",
	"clus2msg", 
	"clus3msg",
	"clus4msg", 
	"clus5msg"
};

static char *winMsgLumpNames[] =
{
	"win1msg",
	"win2msg",
	"win3msg"
};

static char *winMsg_OldWad[] =
{
	WIN1MSG,
	WIN2MSG,
	WIN3MSG
};

static char *ClusMsg_OldWad[] =
{
	CLUS1MSG,
	CLUS2MSG, 
	CLUS3MSG,
	CLUS4MSG, 
	CLUS5MSG
};

// CODE --------------------------------------------------------------------

//==========================================================================
//
// GetClusterText
//
//==========================================================================

char *GetClusterText (int sequence)
{
	char *msgLumpName;
	int msgSize;
	int msgLump;

	if (oldwad_10)
		return ClusMsg_OldWad[sequence];

	msgLumpName = ClusMsgLumpNames[sequence];
	msgLump = W_GetNumForName(msgLumpName);
	msgSize = W_LumpLength(msgLump);
	if (msgSize >= MAX_INTRMSN_MESSAGE_SIZE)
	{
		I_Error("Cluster message too long (%s)", msgLumpName);
	}
	W_ReadLump(msgLump, ClusterMessage);
	ClusterMessage[msgSize] = 0; // Append terminator
	return ClusterMessage;
}

//==========================================================================
//
// GetFinaleText
//
//==========================================================================

char *GetFinaleText (int sequence)
{
	char *msgLumpName;
	int msgSize;
	int msgLump;

	if (oldwad_10)
		return winMsg_OldWad[sequence];

	msgLumpName = winMsgLumpNames[sequence];
	msgLump = W_GetNumForName(msgLumpName);
	msgSize = W_LumpLength(msgLump);
	if (msgSize >= MAX_INTRMSN_MESSAGE_SIZE)
	{
		I_Error("Finale message too long (%s)", msgLumpName);
	}
	W_ReadLump(msgLump, ClusterMessage);
	ClusterMessage[msgSize] = 0;	// Append terminator
	return ClusterMessage;
}

