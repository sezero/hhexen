
//**************************************************************************
//**
//** h_hubmsg.c : Heretic 2 : Raven Software, Corp.
//**
//** $RCSfile: h_hubmsg.c,v $
//** $Revision: 1.3 $
//** $Date: 2008-07-29 07:55:21 $
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

static const char *ClusMsgLumpNames[] =
{
	"CLUS1MSG",
	"CLUS2MSG", 
	"CLUS3MSG",
	"CLUS4MSG", 
	"CLUS5MSG"
};

static const char *winMsgLumpNames[] =
{
	"WIN1MSG",
	"WIN2MSG",
	"WIN3MSG"
};

static const char *winMsg_OldWad[] =
{
	TXT_WIN1MSG,
	TXT_WIN2MSG,
	TXT_WIN3MSG
};

static const char *ClusMsg_OldWad[] =
{
	TXT_CLUS1MSG,
	TXT_CLUS2MSG, 
	TXT_CLUS3MSG,
	TXT_CLUS4MSG, 
	TXT_CLUS5MSG
};

// CODE --------------------------------------------------------------------

//==========================================================================
//
// GetClusterText
//
//==========================================================================

const char *GetClusterText (int sequence)
{
	const char *msgLumpName;
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

const char *GetFinaleText (int sequence)
{
	const char *msgLumpName;
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

