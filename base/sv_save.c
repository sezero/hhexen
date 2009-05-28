
//**************************************************************************
//**
//** sv_save.c : Heretic 2 : Raven Software, Corp.
//**
//** $Revision$
//** $Date$
//**
//** Games are always saved Little Endian, with 32 bit offsets.
//** The saved games then can be properly read on 64 bit and/or
//** Big Endian machines all the same.
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "h2stdinc.h"
#include "h2def.h"
#include "p_local.h"
#include "sv_save.h"

// MACROS ------------------------------------------------------------------

#define MAX_TARGET_PLAYERS	512
#define MOBJ_NULL		-1
#define MOBJ_XX_PLAYER		-2

#define MAX_MAPS		99
#define BASE_SLOT		6
#define REBORN_SLOT		7
#define REBORN_DESCRIPTION	"TEMP GAME"
#define MAX_THINKER_SIZE	256

// TYPES -------------------------------------------------------------------

typedef enum
{
	ASEG_GAME_HEADER = 101,
	ASEG_MAP_HEADER,
	ASEG_WORLD,
	ASEG_POLYOBJS,
	ASEG_MOBJS,
	ASEG_THINKERS,
	ASEG_SCRIPTS,
	ASEG_PLAYERS,
	ASEG_SOUNDS,
	ASEG_MISC,
	ASEG_END
} gameArchiveSegment_t;

typedef enum
{
	TC_NULL,
	TC_MOVE_CEILING,
	TC_VERTICAL_DOOR,
	TC_MOVE_FLOOR,
	TC_PLAT_RAISE,
	TC_INTERPRET_ACS,
	TC_FLOOR_WAGGLE,
	TC_LIGHT,
	TC_PHASE,
	TC_BUILD_PILLAR,
	TC_ROTATE_POLY,
	TC_MOVE_POLY,
	TC_POLY_DOOR
} thinkClass_t;

typedef struct
{
	thinkClass_t tClass;
	think_t thinkerFunc;
	void (*mangleFunc)(void *, void *);
	void (*restoreFunc)(void *, void *);
	size_t realsize, savesize;
} thinkInfo_t;

typedef struct
{
	thinker_t thinker;
	sector_t *sector;
} ssthinker_t;

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

void P_SpawnPlayer(mapthing_t *mthing);

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static void ArchiveWorld(void);
static void UnarchiveWorld(void);
static void ArchivePolyobjs(void);
static void UnarchivePolyobjs(void);
static void ArchiveMobjs(void);
static void UnarchiveMobjs(void);
static void ArchiveThinkers(void);
static void UnarchiveThinkers(void);
static void ArchiveScripts(void);
static void UnarchiveScripts(void);
static void ArchivePlayers(void);
static void UnarchivePlayers(void);
static void ArchiveSounds(void);
static void UnarchiveSounds(void);
static void ArchiveMisc(void);
static void UnarchiveMisc(void);
static void SetMobjArchiveNums(void);
static void RemoveAllThinkers(void);
static void MangleMobj(mobj_t *mobj, save_mobj_t *temp);
static void RestoreMobj(mobj_t *mobj, save_mobj_t *temp);
static int32_t GetMobjNum(mobj_t *mobj);
static mobj_t *GetMobjPtr(int32_t archiveNum, intptr_t *target);
static void MangleFloorMove(void *arg1, void *arg2);
static void RestoreFloorMove(void *arg1, void *arg2);
static void MangleLight(void *arg1, void *arg2);
static void RestoreLight(void *arg1, void *arg2);
static void MangleVerticalDoor(void *arg1, void *arg2);
static void RestoreVerticalDoor(void *arg1, void *arg2);
static void ManglePhase(void *arg1, void *arg2);
static void RestorePhase(void *arg1, void *arg2);
static void ManglePillar(void *arg1, void *arg2);
static void RestorePillar(void *arg1, void *arg2);
static void MangleFloorWaggle(void *arg1, void *arg2);
static void RestoreFloorWaggle(void *arg1, void *arg2);
static void ManglePolyEvent(void *arg1, void *arg2);
static void RestorePolyEvent(void *arg1, void *arg2);
static void ManglePolyDoor(void *arg1, void *arg2);
static void RestorePolyDoor(void *arg1, void *arg2);
static void MangleScript(void *arg1, void *arg2);
static void RestoreScript(void *arg1, void *arg2);
static void ManglePlatRaise(void *arg1, void *arg2);
static void RestorePlatRaise(void *arg1, void *arg2);
static void MangleMoveCeiling(void *arg1, void *arg2);
static void RestoreMoveCeiling(void *arg1, void *arg2);
static void AssertSegment(gameArchiveSegment_t segType);
static void ClearSaveSlot(int slot);
static void CopySaveSlot(int sourceSlot, int destSlot);
static void CopyFile(const char *sourceName, const char *destName);
static boolean ExistingFile(const char *name);
static void OpenStreamOut(const char *fileName);
static void CloseStreamOut(void);
static void StreamOutBuffer(const void *buffer, size_t size);
static void StreamOutByte(byte val);
static void StreamOutWord(uint16_t val);
static void StreamOutLong(uint32_t val);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

extern int ACScriptCount;
extern byte *ActionCodeBase;
extern acsInfo_t *ACSInfo;

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static int MobjCount;
static mobj_t **MobjList;
static intptr_t **TargetPlayerAddrs;
static int TargetPlayerCount;
static void *SaveBuffer;
static boolean SavingPlayers;
static byte *SavePtr;
static FILE *SavingFP;

// This list has been prioritized using frequency estimates
static thinkInfo_t ThinkerInfo[] =
{
	{
		TC_MOVE_FLOOR,
		T_MoveFloor,
		MangleFloorMove,
		RestoreFloorMove,
		sizeof(floormove_t),
		sizeof(save_floormove_t)
	},
	{
		TC_PLAT_RAISE,
		T_PlatRaise,
		ManglePlatRaise,
		RestorePlatRaise,
		sizeof(plat_t),
		sizeof(save_plat_t)
	},
	{
		TC_MOVE_CEILING,
		T_MoveCeiling,
		MangleMoveCeiling,
		RestoreMoveCeiling,
		sizeof(ceiling_t),
		sizeof(save_ceiling_t)
	},
	{
		TC_LIGHT,
		T_Light,
		MangleLight,
		RestoreLight,
		sizeof(light_t),
		sizeof(save_light_t)
	},
	{
		TC_VERTICAL_DOOR,
		T_VerticalDoor,
		MangleVerticalDoor,
		RestoreVerticalDoor,
		sizeof(vldoor_t),
		sizeof(save_vldoor_t)
	},
	{
		TC_PHASE,
		T_Phase,
		ManglePhase,
		RestorePhase,
		sizeof(phase_t),
		sizeof(save_phase_t)
	},
	{
		TC_INTERPRET_ACS,
		T_InterpretACS,
		MangleScript,
		RestoreScript,
		sizeof(acs_t),
		sizeof(save_acs_t)
	},
	{
		TC_ROTATE_POLY,
		T_RotatePoly,
		ManglePolyEvent,
		RestorePolyEvent,
		sizeof(polyevent_t),
		sizeof(save_polyevent_t)
	},
	{
		TC_BUILD_PILLAR,
		T_BuildPillar,
		ManglePillar,
		RestorePillar,
		sizeof(pillar_t),
		sizeof(save_pillar_t)
	},
	{
		TC_MOVE_POLY,
		T_MovePoly,
		ManglePolyEvent,
		RestorePolyEvent,
		sizeof(polyevent_t),
		sizeof(save_polyevent_t)
	},
	{
		TC_POLY_DOOR,
		T_PolyDoor,
		ManglePolyDoor,
		RestorePolyDoor,
		sizeof(polydoor_t),
		sizeof(save_polydoor_t)
	},
	{
		TC_FLOOR_WAGGLE,
		T_FloorWaggle,
		MangleFloorWaggle,
		RestoreFloorWaggle,
		sizeof(floorWaggle_t),
		sizeof(save_floorWaggle_t)
	},
	{ // Terminator
		TC_NULL, NULL, NULL, NULL, 0
	}
};

// CODE --------------------------------------------------------------------

static inline byte GET_BYTE (void)
{
	return *SavePtr++;
}

static inline int16_t GET_WORD (void)
{
	uint16_t val = READ_INT16(SavePtr);
	INCR_INT16(SavePtr);
	return (int16_t) val;
}

static inline int32_t GET_LONG (void)
{
	uint32_t val = READ_INT32(SavePtr);
	INCR_INT32(SavePtr);
	return (int32_t) val;
}

//==========================================================================
//
// SV_SaveGame
//
//==========================================================================

void SV_SaveGame(int slot, const char *description)
{
	int i;
	char fileName[MAX_OSPATH];
	char versionText[HXS_VERSION_TEXT_LENGTH];

	// Open the output file
	snprintf(fileName, sizeof(fileName), "%shex6.hxs", basePath);
	OpenStreamOut(fileName);

	// Write game save description
	StreamOutBuffer(description, HXS_DESCRIPTION_LENGTH);

	// Write version info
	memset(versionText, 0, HXS_VERSION_TEXT_LENGTH);
	strcpy(versionText, HXS_VERSION_TEXT);
	StreamOutBuffer(versionText, HXS_VERSION_TEXT_LENGTH);

	// Place a header marker
	StreamOutLong(ASEG_GAME_HEADER);

	// Write current map and difficulty
	StreamOutByte(gamemap);
	StreamOutByte(gameskill);

	// Write global script info
	for (i = 0; i < MAX_ACS_WORLD_VARS; i++)
	{
		StreamOutLong(WorldVars[i]);
	}
	for (i = 0; i <= MAX_ACS_STORE; i++)
	{
		StreamOutLong(ACSStore[i].map);
		StreamOutLong(ACSStore[i].script);
		StreamOutBuffer(ACSStore[i].args, 4);
	}

	ArchivePlayers();

	// Place a termination marker
	StreamOutLong(ASEG_END);

	// Close the output file
	CloseStreamOut();

	// Save out the current map
	SV_SaveMap(true); // true = save player info

	// Clear all save files at destination slot
	ClearSaveSlot(slot);

	// Copy base slot to destination slot
	CopySaveSlot(BASE_SLOT, slot);
}

//==========================================================================
//
// SV_SaveMap
//
//==========================================================================

void SV_SaveMap(boolean savePlayers)
{
	char fileName[MAX_OSPATH];

	SavingPlayers = savePlayers;

	// Open the output file
	snprintf(fileName, sizeof(fileName), "%shex6%02d.hxs", basePath, gamemap);
	OpenStreamOut(fileName);

	// Place a header marker
	StreamOutLong(ASEG_MAP_HEADER);

	// Write the level timer
	StreamOutLong(leveltime);

	// Set the mobj archive numbers
	SetMobjArchiveNums();

	ArchiveWorld();
	ArchivePolyobjs();
	ArchiveMobjs();
	ArchiveThinkers();
	ArchiveScripts();
	ArchiveSounds();
	ArchiveMisc();

	// Place a termination marker
	StreamOutLong(ASEG_END);

	// Close the output file
	CloseStreamOut();
}

//==========================================================================
//
// SV_LoadGame
//
//==========================================================================

void SV_LoadGame(int slot)
{
	int i;
	char fileName[MAX_OSPATH];
	player_t playerBackup[MAXPLAYERS];
	mobj_t *mobj;

	// Copy all needed save files to the base slot
	if (slot != BASE_SLOT)
	{
		ClearSaveSlot(BASE_SLOT);
		CopySaveSlot(slot, BASE_SLOT);
	}

	// Create the name
	snprintf(fileName, sizeof(fileName), "%shex6.hxs", basePath);

	// Load the file
	M_ReadFile(fileName, &SaveBuffer);

	// Set the save pointer and skip the description field
	SavePtr = (byte *)SaveBuffer + HXS_DESCRIPTION_LENGTH;

	// Check the version text
	if (strcmp((char *)SavePtr, HXS_VERSION_TEXT))
	{ // Bad version
		return;
	}
	SavePtr += HXS_VERSION_TEXT_LENGTH;

	AssertSegment(ASEG_GAME_HEADER);

	gameepisode = 1;
	gamemap = GET_BYTE();
	gameskill = GET_BYTE();

	// Read global script info
	memcpy(WorldVars, SavePtr, sizeof(WorldVars));
	SavePtr += sizeof(WorldVars);
	memcpy(ACSStore, SavePtr, sizeof(ACSStore));
	SavePtr += sizeof(ACSStore);
	for (i = 0; i < MAX_ACS_WORLD_VARS; i++)
	{
		WorldVars[i] = (int) LONG(WorldVars[i]);
	}
	for (i = 0; i <= MAX_ACS_STORE; i++)
	{
		ACSStore[i].map = (int) LONG(ACSStore[i].map);
		ACSStore[i].script = (int) LONG(ACSStore[i].script);
	}

	// Read the player structures
	UnarchivePlayers();

	AssertSegment(ASEG_END);

	Z_Free(SaveBuffer);

	// Save player structs
	for (i = 0; i < MAXPLAYERS; i++)
	{
		playerBackup[i] = players[i];
	}

	// Load the current map
	SV_LoadMap();

	// Don't need the player mobj relocation info for load game
	Z_Free(TargetPlayerAddrs);
	TargetPlayerAddrs = NULL;

	// Restore player structs
	inv_ptr = 0;
	curpos = 0;
	for (i = 0; i < MAXPLAYERS; i++)
	{
		mobj = players[i].mo;
		players[i] = playerBackup[i];
		players[i].mo = mobj;
		if (i == consoleplayer)
		{
			players[i].readyArtifact = players[i].inventory[inv_ptr].type;
		}
	}
}

//==========================================================================
//
// SV_UpdateRebornSlot
//
// Copies the base slot to the reborn slot.
//
//==========================================================================

void SV_UpdateRebornSlot(void)
{
	ClearSaveSlot(REBORN_SLOT);
	CopySaveSlot(BASE_SLOT, REBORN_SLOT);
}

//==========================================================================
//
// SV_ClearRebornSlot
//
//==========================================================================

void SV_ClearRebornSlot(void)
{
	ClearSaveSlot(REBORN_SLOT);
}

//==========================================================================
//
// SV_MapTeleport
//
//==========================================================================

void SV_MapTeleport(int map, int position)
{
	int i;
	int j;
	char fileName[MAX_OSPATH];
	player_t playerBackup[MAXPLAYERS];
	mobj_t *mobj;
	mobj_t *targetPlayerMobj;
	int inventoryPtr;
	int currentInvPos;
	boolean rClass;
	boolean playerWasReborn;
	boolean oldWeaponowned[NUMWEAPONS];
	int oldKeys   = 0; /* jim added initialiser */
	int oldPieces = 0; /* jim added initialiser */
	int bestWeapon;

	if (!deathmatch)
	{
		if (P_GetMapCluster(gamemap) == P_GetMapCluster(map))
		{ // Same cluster - save map without saving player mobjs
			SV_SaveMap(false);
		}
		else
		{ // Entering new cluster - clear base slot
			ClearSaveSlot(BASE_SLOT);
		}
	}

	// Store player structs for later
	rClass = randomclass;
	randomclass = false;
	for (i = 0; i < MAXPLAYERS; i++)
	{
		playerBackup[i] = players[i];
	}

	// Save some globals that get trashed during the load
	inventoryPtr = inv_ptr;
	currentInvPos = curpos;

	gamemap = map;
	snprintf(fileName, sizeof(fileName), "%shex6%02d.hxs", basePath, gamemap);
	if (!deathmatch && ExistingFile(fileName))
	{ // Unarchive map
		SV_LoadMap();
	}
	else
	{ // New map
		G_InitNew(gameskill, gameepisode, gamemap);

		// Destroy all freshly spawned players
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i])
			{
				P_RemoveMobj(players[i].mo);
			}
		}
	}

	// Restore player structs
	targetPlayerMobj = NULL;
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
		{
			continue;
		}
		players[i] = playerBackup[i];
		P_ClearMessage(&players[i]);
		players[i].attacker = NULL;
		players[i].poisoner = NULL;

		if (netgame)
		{
			if (players[i].playerstate == PST_DEAD)
			{ // In a network game, force all players to be alive
				players[i].playerstate = PST_REBORN;
			}
			if (!deathmatch)
			{ // Cooperative net-play, retain keys and weapons
				oldKeys = players[i].keys;
				oldPieces = players[i].pieces;
				for (j = 0; j < NUMWEAPONS; j++)
				{
					oldWeaponowned[j] = players[i].weaponowned[j];
				}
			}
		}
		playerWasReborn = (players[i].playerstate == PST_REBORN);
		if (deathmatch)
		{
			memset(players[i].frags, 0, sizeof(players[i].frags));
			mobj = P_SpawnMobj(playerstarts[0][i].x<<16,
				playerstarts[0][i].y<<16, 0, MT_PLAYER_FIGHTER);
			players[i].mo = mobj;
			G_DeathMatchSpawnPlayer(i);
			P_RemoveMobj(mobj);
		}
		else
		{
			P_SpawnPlayer(&playerstarts[position][i]);
		}

		if (playerWasReborn && netgame && !deathmatch)
		{ // Restore keys and weapons when reborn in co-op
			players[i].keys = oldKeys;
			players[i].pieces = oldPieces;
			for (bestWeapon = 0, j = 0; j < NUMWEAPONS; j++)
			{
				if (oldWeaponowned[j])
				{
					bestWeapon = j;
					players[i].weaponowned[j] = true;
				}
			}
			players[i].mana[MANA_1] = 25;
			players[i].mana[MANA_2] = 25;
			if (bestWeapon)
			{ // Bring up the best weapon
				players[i].pendingweapon = bestWeapon;
			}
		}

		if (targetPlayerMobj == NULL)
		{ // The poor sap
			targetPlayerMobj = players[i].mo;
		}
	}
	randomclass = rClass;

	// Redirect anything targeting a player mobj
	if (TargetPlayerAddrs)
	{
		for (i = 0; i < TargetPlayerCount; i++)
		{
			*TargetPlayerAddrs[i] = (intptr_t)targetPlayerMobj;
		}
		Z_Free(TargetPlayerAddrs);
		TargetPlayerAddrs = NULL;
	}

	// Destroy all things touching players
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i])
		{
			P_TeleportMove(players[i].mo, players[i].mo->x, players[i].mo->y);
		}
	}

	// Restore trashed globals
	inv_ptr = inventoryPtr;
	curpos = currentInvPos;

	// Launch waiting scripts
	if (!deathmatch)
	{
		P_CheckACSStore();
	}

	// For single play, save immediately into the reborn slot
	if (!netgame)
	{
		SV_SaveGame(REBORN_SLOT, REBORN_DESCRIPTION);
	}
}

//==========================================================================
//
// SV_GetRebornSlot
//
//==========================================================================

int SV_GetRebornSlot(void)
{
	return (REBORN_SLOT);
}

//==========================================================================
//
// SV_RebornSlotAvailable
//
// Returns true if the reborn slot is available.
//
//==========================================================================

boolean SV_RebornSlotAvailable(void)
{
	char fileName[MAX_OSPATH];

	snprintf(fileName, sizeof(fileName), "%shex%d.hxs", basePath, REBORN_SLOT);
	return ExistingFile(fileName);
}

//==========================================================================
//
// SV_LoadMap
//
//==========================================================================

void SV_LoadMap(void)
{
	char fileName[MAX_OSPATH];

	// Load a base level
	G_InitNew(gameskill, gameepisode, gamemap);

	// Remove all thinkers
	RemoveAllThinkers();

	// Create the name
	snprintf(fileName, sizeof(fileName), "%shex6%02d.hxs", basePath, gamemap);

	// Load the file
	M_ReadFile(fileName, &SaveBuffer);
	SavePtr = (byte *) SaveBuffer;

	AssertSegment(ASEG_MAP_HEADER);

	// Read the level timer
	leveltime = GET_LONG();

	UnarchiveWorld();
	UnarchivePolyobjs();
	UnarchiveMobjs();
	UnarchiveThinkers();
	UnarchiveScripts();
	UnarchiveSounds();
	UnarchiveMisc();

	AssertSegment(ASEG_END);

	// Free mobj list and save buffer
	Z_Free(MobjList);
	Z_Free(SaveBuffer);
}

//==========================================================================
//
// SV_InitBaseSlot
//
//==========================================================================

void SV_InitBaseSlot(void)
{
	ClearSaveSlot(BASE_SLOT);
}

//==========================================================================
//
// ArchivePlayers
//
//==========================================================================

static void ArchivePlayers(void)
{
	int i, j;
	save_player_t savp;
	player_t *p;

	StreamOutLong(ASEG_PLAYERS);
	for (i = 0; i < MAXPLAYERS; i++)
	{
		StreamOutByte(playeringame[i]);
	}
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
		{
			continue;
		}
		StreamOutByte(PlayerClasses[i]);

		p = &players[i];
		savp.mo_idx = 0;
		savp.poisoner_idx = 0;
		savp.attacker_idx = 0;
		savp.playerstate	= (int) LONG(p->playerstate);
		savp.cmd.forwardmove	= p->cmd.forwardmove;
		savp.cmd.sidemove	= p->cmd.sidemove;
		savp.cmd.angleturn	= (short) SHORT(p->cmd.angleturn);
		savp.cmd.consistancy	= (short) SHORT(p->cmd.consistancy);
		savp.cmd.chatchar	= p->cmd.chatchar;
		savp.cmd.buttons	= p->cmd.buttons;
		savp.cmd.lookfly	= p->cmd.lookfly;
		savp.cmd.arti		= p->cmd.arti;
		savp.playerclass	= (int) LONG(p->playerclass);
		savp.viewz		= (fixed_t) LONG(p->viewz);
		savp.viewheight		= (fixed_t) LONG(p->viewheight);
		savp.deltaviewheight	= (fixed_t) LONG(p->deltaviewheight);
		savp.bob		= (fixed_t) LONG(p->bob);
		savp.flyheight		= (int) LONG(p->flyheight);
		savp.lookdir		= (int) LONG(p->lookdir);
		savp.centering		= (int) LONG(p->centering);
		savp.health		= (int) LONG(p->health);
		for (j = 0; j < NUMARMOR; j++)
		{
			savp.armorpoints[j] = (int) LONG(p->armorpoints[j]);
		}
		for (j = 0; j < NUMINVENTORYSLOTS; j++)
		{
			savp.inventory[j].type = (int) LONG(p->inventory[j].type);
			savp.inventory[j].count = (int) LONG(p->inventory[j].count);
		}
		savp.readyArtifact	= (int) LONG(p->readyArtifact);
		savp.inventorySlotNum	= (int) LONG(p->inventorySlotNum);
		savp.artifactCount	= (int) LONG(p->artifactCount);
		for (j = 0; j < NUMPOWERS; j++)
		{
			savp.powers[j]	= (int) LONG(p->powers[j]);
		}
		savp.keys		= (int) LONG(p->keys);
		savp.pieces		= (int) LONG(p->pieces);
		for (j = 0; j < MAXPLAYERS; j++)
		{
			savp.frags[j]	= (signed int) LONG(p->frags[j]);
		}
		savp.readyweapon	= (int) LONG(p->readyweapon);
		savp.pendingweapon	= (int) LONG(p->pendingweapon);
		for (j = 0; j < NUMWEAPONS; j++)
		{
			savp.weaponowned[j] = (int) LONG(p->weaponowned[j]);
		}
		for (j = 0; j < NUMMANA; j++)
		{
			savp.mana[j]	= (int) LONG(p->mana[j]);
		}
		savp.attackdown		= (int) LONG(p->attackdown);
		savp.usedown		= (int) LONG(p->usedown);
		savp.cheats		= (int) LONG(p->cheats);
		savp.refire		= (int) LONG(p->refire);
		savp.killcount		= (int) LONG(p->killcount);
		savp.itemcount		= (int) LONG(p->itemcount);
		savp.secretcount	= (int) LONG(p->secretcount);
		memcpy (savp.message, p->message, 80);
		savp.messageTics	= (int) LONG(p->messageTics);
		savp.ultimateMessage	= (short) SHORT(p->ultimateMessage);
		savp.yellowMessage	= (short) SHORT(p->yellowMessage);
		savp.damagecount	= (int) LONG(p->damagecount);
		savp.bonuscount		= (int) LONG(p->bonuscount);
		savp.poisoncount	= (int) LONG(p->poisoncount);
		savp.extralight		= (int) LONG(p->extralight);
		savp.fixedcolormap	= (int) LONG(p->fixedcolormap);
		savp.colormap		= (int) LONG(p->colormap);
		savp.morphTics		= (int) LONG(p->morphTics);
		savp.jumpTics		= (unsigned int) LONG(p->jumpTics);
		savp.worldTimer		= (unsigned int) LONG(p->worldTimer);
		for (j = 0; j < NUMPSPRITES; j++)
		{
			savp.psprites[j].tics = (int) LONG(p->psprites[j].tics);
			savp.psprites[j].sx = (fixed_t) LONG(p->psprites[j].sx);
			savp.psprites[j].sy = (fixed_t) LONG(p->psprites[j].sy);
			if (p->psprites[j].state)
			{
				savp.psprites[j].state_idx =
					LONG((int32_t) (p->psprites[j].state - states));
			}
			else
			{
				savp.psprites[j].state_idx = 0;
			}
		}
		StreamOutBuffer(&savp, sizeof(save_player_t));
	}
}

//==========================================================================
//
// UnarchivePlayers
//
//==========================================================================

static void UnarchivePlayers(void)
{
	int i, j;
	save_player_t savp;
	player_t *p;

	AssertSegment(ASEG_PLAYERS);
	for (i = 0; i < MAXPLAYERS; i++)
	{
		playeringame[i] = GET_BYTE();
	}
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
		{
			continue;
		}
		PlayerClasses[i] = GET_BYTE();
		memcpy(&savp, SavePtr, sizeof(save_player_t));
		SavePtr += sizeof(save_player_t);

		p = &players[i];
		p->mo = NULL; // Will be set when unarc thinker
		p->attacker = NULL;
		p->poisoner = NULL;
		p->playerstate		= (playerstate_t) LONG(savp.playerstate);
		p->cmd.forwardmove	= savp.cmd.forwardmove;
		p->cmd.sidemove		= savp.cmd.sidemove;
		p->cmd.angleturn	= (short) SHORT(savp.cmd.angleturn);
		p->cmd.consistancy	= (short) SHORT(savp.cmd.consistancy);
		p->cmd.chatchar		= savp.cmd.chatchar;
		p->cmd.buttons		= savp.cmd.buttons;
		p->cmd.lookfly		= savp.cmd.lookfly;
		p->cmd.arti		= savp.cmd.arti;
		p->playerclass		= (pclass_t) LONG(savp.playerclass);
		p->viewz		= (fixed_t) LONG(savp.viewz);
		p->viewheight		= (fixed_t) LONG(savp.viewheight);
		p->deltaviewheight	= (fixed_t) LONG(savp.deltaviewheight);
		p->bob			= (fixed_t) LONG(savp.bob);
		p->flyheight		= (int) LONG(savp.flyheight);
		p->lookdir		= (int) LONG(savp.lookdir);
		p->centering		= !!(LONG(savp.centering));
		p->health		= (int) LONG(savp.health);
		for (j = 0; j < NUMARMOR; j++)
		{
			p->armorpoints[j] = (int) LONG(savp.armorpoints[j]);
		}
		for (j = 0; j < NUMINVENTORYSLOTS; j++)
		{
			p->inventory[j].type = (int) LONG(savp.inventory[j].type);
			p->inventory[j].count = (int) LONG(savp.inventory[j].count);
		}
		p->readyArtifact	= (artitype_t) LONG(savp.readyArtifact);
		p->artifactCount	= (int) LONG(savp.artifactCount);
		p->inventorySlotNum	= (int) LONG(savp.inventorySlotNum);
		for (j = 0; j < NUMPOWERS; j++)
		{
			p->powers[j]	= (int) LONG(savp.powers[j]);
		}
		p->keys			= (int) LONG(savp.keys);
		p->pieces		= (int) LONG(savp.pieces);
		for (j = 0; j < MAXPLAYERS; j++)
		{
			p->frags[j]	= (signed int) LONG(savp.frags[j]);
		}
		p->readyweapon		= (weapontype_t) LONG(savp.readyweapon);
		p->pendingweapon	= (weapontype_t) LONG(savp.pendingweapon);
		for (j = 0; j < NUMWEAPONS; j++)
		{
			p->weaponowned[j] = !!(LONG(savp.weaponowned[j]));
		}
		for (j = 0; j < NUMMANA; j++)
		{
			p->mana[j]	= (int) LONG(savp.mana[j]);
		}
		p->attackdown		= (int) LONG(savp.attackdown);
		p->usedown		= (int) LONG(savp.usedown);
		p->cheats		= (int) LONG(savp.cheats);
		p->refire		= (int) LONG(savp.refire);
		p->killcount		= (int) LONG(savp.killcount);
		p->itemcount		= (int) LONG(savp.itemcount);
		p->secretcount		= (int) LONG(savp.secretcount);
		memcpy (p->message, savp.message, 80);
		p->messageTics		= (int) LONG(savp.messageTics);
		p->ultimateMessage	= (short) SHORT(savp.ultimateMessage);
		p->yellowMessage	= (short) SHORT(savp.yellowMessage);
		p->damagecount		= (int) LONG(savp.damagecount);
		p->bonuscount		= (int) LONG(savp.bonuscount);
		p->poisoncount		= (int) LONG(savp.poisoncount);
		p->extralight		= (int) LONG(savp.extralight);
		p->fixedcolormap	= (int) LONG(savp.fixedcolormap);
		p->colormap		= (int) LONG(savp.colormap);
		p->morphTics		= (int) LONG(savp.morphTics);
		p->jumpTics		= (unsigned int) LONG(savp.jumpTics);
		p->worldTimer		= (unsigned int) LONG(savp.worldTimer);
		P_ClearMessage(p);
		for (j = 0; j < NUMPSPRITES; j++)
		{
			p->psprites[j].tics = (int) LONG(savp.psprites[j].tics);
			p->psprites[j].sx = (fixed_t) LONG(savp.psprites[j].sx);
			p->psprites[j].sy = (fixed_t) LONG(savp.psprites[j].sy);
			if (savp.psprites[j].state_idx)
			{
				savp.psprites[j].state_idx =
					LONG(savp.psprites[j].state_idx);
				p->psprites[j].state =
					&states[savp.psprites[j].state_idx];
			}
			else
			{
				p->psprites[j].state = NULL;
			}
		}
	}
}

//==========================================================================
//
// ArchiveWorld
//
//==========================================================================

static void ArchiveWorld(void)
{
	int i;
	int j;
	sector_t *sec;
	line_t *li;
	side_t *si;

	StreamOutLong(ASEG_WORLD);
	for (i = 0, sec = sectors; i < numsectors; i++, sec++)
	{
		StreamOutWord(sec->floorheight>>FRACBITS);
		StreamOutWord(sec->ceilingheight>>FRACBITS);
		StreamOutWord(sec->floorpic);
		StreamOutWord(sec->ceilingpic);
		StreamOutWord(sec->lightlevel);
		StreamOutWord(sec->special);
		StreamOutWord(sec->tag);
		StreamOutWord(sec->seqType);
	}
	for (i = 0, li = lines; i < numlines; i++, li++)
	{
		StreamOutWord(li->flags);
		StreamOutByte(li->special);
		StreamOutByte(li->arg1);
		StreamOutByte(li->arg2);
		StreamOutByte(li->arg3);
		StreamOutByte(li->arg4);
		StreamOutByte(li->arg5);
		for (j = 0; j < 2; j++)
		{
			if (li->sidenum[j] == -1)
			{
				continue;
			}
			si = &sides[li->sidenum[j]];
			StreamOutWord(si->textureoffset>>FRACBITS);
			StreamOutWord(si->rowoffset>>FRACBITS);
			StreamOutWord(si->toptexture);
			StreamOutWord(si->bottomtexture);
			StreamOutWord(si->midtexture);
		}
	}
}

//==========================================================================
//
// UnarchiveWorld
//
//==========================================================================

static void UnarchiveWorld(void)
{
	int i;
	int j;
	sector_t *sec;
	line_t *li;
	side_t *si;

	AssertSegment(ASEG_WORLD);
	for (i = 0, sec = sectors; i < numsectors; i++, sec++)
	{
		sec->floorheight = ((fixed_t) GET_WORD())<<FRACBITS;
		sec->ceilingheight = ((fixed_t) GET_WORD())<<FRACBITS;
		sec->floorpic = GET_WORD();
		sec->ceilingpic = GET_WORD();
		sec->lightlevel = GET_WORD();
		sec->special = GET_WORD();
		sec->tag = GET_WORD();
		sec->seqType = GET_WORD();
		sec->specialdata = NULL;
		sec->soundtarget = NULL;
	}
	for (i = 0, li = lines; i < numlines; i++, li++)
	{
		li->flags = GET_WORD();
		li->special = GET_BYTE();
		li->arg1 = GET_BYTE();
		li->arg2 = GET_BYTE();
		li->arg3 = GET_BYTE();
		li->arg4 = GET_BYTE();
		li->arg5 = GET_BYTE();
		for (j = 0; j < 2; j++)
		{
			if (li->sidenum[j] == -1)
			{
				continue;
			}
			si = &sides[li->sidenum[j]];
			si->textureoffset = ((fixed_t) GET_WORD())<<FRACBITS;
			si->rowoffset = ((fixed_t) GET_WORD())<<FRACBITS;
			si->toptexture = GET_WORD();
			si->bottomtexture = GET_WORD();
			si->midtexture = GET_WORD();
		}
	}
}

//==========================================================================
//
// SetMobjArchiveNums
//
// Sets the archive numbers in all mobj structs.  Also sets the MobjCount
// global.  Ignores player mobjs if SavingPlayers is false.
//
//==========================================================================

static void SetMobjArchiveNums(void)
{
	mobj_t *mobj;
	thinker_t *thinker;

	MobjCount = 0;
	for (thinker = thinkercap.next; thinker != &thinkercap;
					thinker = thinker->next)
	{
		if (thinker->function == P_MobjThinker)
		{
			mobj = (mobj_t *)thinker;
			if (mobj->player && !SavingPlayers)
			{ // Skipping player mobjs
				continue;
			}
			mobj->archiveNum = MobjCount++;
		}
	}
}

//==========================================================================
//
// ArchiveMobjs
//
//==========================================================================

static void ArchiveMobjs(void)
{
	int count;
	thinker_t *thinker;
	save_mobj_t tempMobj;
	mobj_t *mobj;

	StreamOutLong(ASEG_MOBJS);
	StreamOutLong(MobjCount);
	count = 0;
	for (thinker = thinkercap.next; thinker != &thinkercap;
					thinker = thinker->next)
	{
		if (thinker->function != P_MobjThinker)
		{ // Not a mobj thinker
			continue;
		}
		mobj = (mobj_t *)thinker;
		if (mobj->player && !SavingPlayers)
		{ // Skipping player mobjs
			continue;
		}
		count++;
		memset(&tempMobj, 0, sizeof(save_mobj_t));
		MangleMobj(mobj, &tempMobj);
		StreamOutBuffer(&tempMobj, sizeof(save_mobj_t));
	}
	if (count != MobjCount)
	{
		I_Error("ArchiveMobjs: bad mobj count");
	}
}

//==========================================================================
//
// UnarchiveMobjs
//
//==========================================================================

static void UnarchiveMobjs(void)
{
	int i;
	save_mobj_t tempMobj;
	mobj_t *mobj;

	AssertSegment(ASEG_MOBJS);
	TargetPlayerAddrs = (intptr_t **) Z_Malloc(MAX_TARGET_PLAYERS*sizeof(intptr_t *), PU_STATIC, NULL);
	TargetPlayerCount = 0;
	MobjCount = GET_LONG();
	MobjList = (mobj_t **) Z_Malloc(MobjCount*sizeof(mobj_t *), PU_STATIC, NULL);
	for (i = 0; i < MobjCount; i++)
	{
		MobjList[i] = (mobj_t *) Z_Malloc(sizeof(mobj_t), PU_LEVEL, NULL);
	}
	for (i = 0; i < MobjCount; i++)
	{
		mobj = MobjList[i];
		memset(&tempMobj, 0, sizeof(save_mobj_t));
		memset(mobj, 0, sizeof(mobj_t));
		memcpy(&tempMobj, SavePtr, sizeof(save_mobj_t));
		SavePtr += sizeof(save_mobj_t);
		RestoreMobj(mobj, &tempMobj);
		P_AddThinker(&mobj->thinker);
	}
	P_CreateTIDList();
	P_InitCreatureCorpseQueue(true); // true = scan for corpses
}

//==========================================================================
//
// MangleMobj
//
//==========================================================================

static void MangleMobj(mobj_t *mobj, save_mobj_t *temp)
{
	boolean corpse;
	uint32_t swap;

	temp->x			= (fixed_t) LONG(mobj->x);
	temp->y			= (fixed_t) LONG(mobj->y);
	temp->z			= (fixed_t) LONG(mobj->z);
	temp->angle		= (angle_t) LONG(mobj->angle);
	temp->sprite		= (int) LONG(mobj->sprite);
	temp->frame		= (int) LONG(mobj->frame);
	temp->floorpic		= (fixed_t) LONG(mobj->floorpic);
	temp->radius		= (fixed_t) LONG(mobj->radius);
	temp->height		= (fixed_t) LONG(mobj->height);
	temp->momx		= (fixed_t) LONG(mobj->momx);
	temp->momy		= (fixed_t) LONG(mobj->momy);
	temp->momz		= (fixed_t) LONG(mobj->momz);
	temp->validcount	= (int) LONG(mobj->validcount);
	temp->type		= (int) LONG(mobj->type);
	temp->tics		= (int) LONG(mobj->tics);
	temp->damage		= (int) LONG(mobj->damage);
	temp->flags		= (int) LONG(mobj->flags);
	temp->flags2		= (int) LONG(mobj->flags2);
	temp->health		= (int) LONG(mobj->health);
	temp->movedir		= (int) LONG(mobj->movedir);
	temp->movecount		= (int) LONG(mobj->movecount);
	temp->reactiontime	= (int) LONG(mobj->reactiontime);
	temp->threshold		= (int) LONG(mobj->threshold);
	temp->lastlook		= (int) LONG(mobj->lastlook);
	temp->floorclip		= (fixed_t) LONG(mobj->floorclip);
	temp->archiveNum	= (int) LONG(mobj->archiveNum);
	temp->tid		= (short) SHORT(mobj->tid);
	temp->special		= mobj->special;
	/*
	temp->args[0]		= mobj->args[0];
	temp->args[1]		= mobj->args[1];
	temp->args[2]		= mobj->args[2];
	temp->args[3]		= mobj->args[3];
	*/
	/* byte swap args 0 to 3:  this stupidity is here, because
	 * of the way the summon time of minotaurs are kept/traced.
	 * FIXME: should I do a if(mo->type == MT_MINOTAUR) check?
	 */
	memcpy(&swap, mobj->args, 4);
	swap = (uint32_t) LONG(swap);
	memcpy(temp->args, &swap, 4);
	temp->args[4]		= mobj->args[4];

	corpse = mobj->flags & MF_CORPSE;
	temp->state_idx = LONG((int32_t)(mobj->state - states));
	if (mobj->player)
	{
		temp->player_idx = LONG((int32_t)((mobj->player - players) + 1));
	}
	if (corpse)
	{
		temp->target_idx = (int32_t) LONG(MOBJ_NULL);
	}
	else
	{
		temp->target_idx = (int32_t) LONG(GetMobjNum(mobj->target));
	}
	switch (mobj->type)
	{
	// Just special1
	case MT_BISH_FX:
	case MT_HOLY_FX:
	case MT_DRAGON:
	case MT_THRUSTFLOOR_UP:
	case MT_THRUSTFLOOR_DOWN:
	case MT_MINOTAUR:
	case MT_SORCFX1:
	case MT_MSTAFF_FX2:
		if (corpse)
		{
			temp->special1 = (int32_t) LONG(MOBJ_NULL);
		}
		else
		{
			temp->special1 = (int32_t) LONG(GetMobjNum((mobj_t *)mobj->special1));
		}
		break;

	// Just special2
	case MT_LIGHTNING_FLOOR:
	case MT_LIGHTNING_ZAP:
		if (corpse)
		{
			temp->special2 = (int32_t) LONG(MOBJ_NULL);
		}
		else
		{
			temp->special2 = (int32_t) LONG(GetMobjNum((mobj_t *)mobj->special2));
		}
		break;

	// Both special1 and special2
	case MT_HOLY_TAIL:
	case MT_LIGHTNING_CEILING:
		if (corpse)
		{
			temp->special1 = (int32_t) LONG(MOBJ_NULL);
			temp->special2 = (int32_t) LONG(MOBJ_NULL);
		}
		else
		{
			temp->special1 = (int32_t) LONG(GetMobjNum((mobj_t *)mobj->special1));
			temp->special2 = (int32_t) LONG(GetMobjNum((mobj_t *)mobj->special2));
		}
		break;

	// Miscellaneous
	case MT_KORAX:
		temp->special1 = 0; // Searching index
		break;

	default:
		break;
	}
}

//==========================================================================
//
// GetMobjNum
//
//==========================================================================

static int32_t GetMobjNum(mobj_t *mobj)
{
	if (mobj == NULL)
	{
		return MOBJ_NULL;
	}
	if (mobj->player && !SavingPlayers)
	{
		return MOBJ_XX_PLAYER;
	}
	return mobj->archiveNum;
}

//==========================================================================
//
// RestoreMobj
//
//==========================================================================

static void RestoreMobj(mobj_t *mobj, save_mobj_t *temp)
{
	uint32_t	swap;

	mobj->x			= (fixed_t) LONG(temp->x);
	mobj->y			= (fixed_t) LONG(temp->y);
	mobj->z			= (fixed_t) LONG(temp->z);
	mobj->angle		= (angle_t) LONG(temp->angle);
	mobj->sprite		= (spritenum_t) LONG(temp->sprite);
	mobj->frame		= (int) LONG(temp->frame);
	mobj->floorpic		= (fixed_t) LONG(temp->floorpic);
	mobj->radius		= (fixed_t) LONG(temp->radius);
	mobj->height		= (fixed_t) LONG(temp->height);
	mobj->momx		= (fixed_t) LONG(temp->momx);
	mobj->momy		= (fixed_t) LONG(temp->momy);
	mobj->momz		= (fixed_t) LONG(temp->momz);
	mobj->validcount	= (int) LONG(temp->validcount);
	mobj->type		= (mobjtype_t) LONG(temp->type);
	mobj->tics		= (int) LONG(temp->tics);
	mobj->damage		= (int) LONG(temp->damage);
	mobj->flags		= (int) LONG(temp->flags);
	mobj->flags2		= (int) LONG(temp->flags2);
	mobj->health		= (int) LONG(temp->health);
	mobj->movedir		= (int) LONG(temp->movedir);
	mobj->movecount		= (int) LONG(temp->movecount);
	mobj->reactiontime	= (int) LONG(temp->reactiontime);
	mobj->threshold		= (int) LONG(temp->threshold);
	mobj->lastlook		= (int) LONG(temp->lastlook);
	mobj->floorclip		= (fixed_t) LONG(temp->floorclip);
	mobj->archiveNum	= (int) LONG(temp->archiveNum);
	mobj->tid		= (short) SHORT(temp->tid);
	mobj->special		= temp->special;
	/*
	mobj->args[0]		= temp->args[0];
	mobj->args[1]		= temp->args[1];
	mobj->args[2]		= temp->args[2];
	mobj->args[3]		= temp->args[3];
	*/
	/* byte swap args 0 to 3:  this stupidity is here, because
	 * of the way the summon time of minotaurs are kept/traced.
	 * FIXME: should I do a if(mo->type == MT_MINOTAUR) check?
	 */
	memcpy(&swap, temp->args, 4);
	swap = (uint32_t) LONG(swap);
	memcpy(mobj->args, &swap, 4);
	mobj->args[4]		= temp->args[4];

	temp->state_idx		= (int32_t) LONG(temp->state_idx);
	temp->player_idx	= (int32_t) LONG(temp->player_idx);
	temp->target_idx	= (int32_t) LONG(temp->target_idx);
	temp->special1		= (int32_t) LONG(temp->special1);
	temp->special2		= (int32_t) LONG(temp->special2);

	mobj->thinker.function	= P_MobjThinker;
	mobj->state		= &states[temp->state_idx];
	if (temp->player_idx)
	{
		mobj->player	= &players[temp->player_idx - 1];
		mobj->player->mo = mobj;
	}
	P_SetThingPosition(mobj);
	mobj->info = &mobjinfo[mobj->type];
	mobj->floorz = mobj->subsector->sector->floorheight;
	mobj->ceilingz = mobj->subsector->sector->ceilingheight;
	mobj->target = GetMobjPtr(temp->target_idx, (intptr_t *)(void *)&mobj->target);
	switch (mobj->type)
	{
	// Just special1
	case MT_BISH_FX:
	case MT_HOLY_FX:
	case MT_DRAGON:
	case MT_THRUSTFLOOR_UP:
	case MT_THRUSTFLOOR_DOWN:
	case MT_MINOTAUR:
	case MT_SORCFX1:
		mobj->special1 = (intptr_t) GetMobjPtr(temp->special1, &mobj->special1);
		break;

	// Just special2
	case MT_LIGHTNING_FLOOR:
	case MT_LIGHTNING_ZAP:
		mobj->special2 = (intptr_t) GetMobjPtr(temp->special2, &mobj->special2);
		break;

	// Both special1 and special2
	case MT_HOLY_TAIL:
	case MT_LIGHTNING_CEILING:
		mobj->special1 = (intptr_t) GetMobjPtr(temp->special1, &mobj->special1);
		mobj->special2 = (intptr_t) GetMobjPtr(temp->special2, &mobj->special2);
		break;

	default:
		break;
	}
}

//==========================================================================
//
// GetMobjPtr
//
//==========================================================================

static mobj_t *GetMobjPtr(int32_t archiveNum, intptr_t *target)
{
	if (archiveNum == MOBJ_NULL)
	{
		return NULL;
	}
	if (archiveNum == MOBJ_XX_PLAYER)
	{
		if (TargetPlayerCount == MAX_TARGET_PLAYERS)
		{
			I_Error("RestoreMobj: exceeded MAX_TARGET_PLAYERS");
		}
		TargetPlayerAddrs[TargetPlayerCount++] = target;
		return NULL;
	}
	return MobjList[archiveNum];
}

//==========================================================================
//
// ArchiveThinkers
//
//==========================================================================

static void ArchiveThinkers(void)
{
	thinker_t *thinker;
	thinkInfo_t *info;
	byte buffer[MAX_THINKER_SIZE];

	StreamOutLong(ASEG_THINKERS);
	for (thinker = thinkercap.next; thinker != &thinkercap;
					thinker = thinker->next)
	{
		for (info = ThinkerInfo; info->tClass != TC_NULL; info++)
		{
			if (thinker->function == info->thinkerFunc)
			{
				StreamOutByte(info->tClass);
				memset(buffer, 0, sizeof(buffer));
				info->mangleFunc(thinker, buffer);
				StreamOutBuffer(buffer, info->savesize);
				break;
			}
		}
	}
	// Add a termination marker
	StreamOutByte(TC_NULL);
}

//==========================================================================
//
// UnarchiveThinkers
//
//==========================================================================

static void UnarchiveThinkers(void)
{
	int tClass;
	thinker_t *thinker;
	thinkInfo_t *info;
	byte buffer[MAX_THINKER_SIZE];

	AssertSegment(ASEG_THINKERS);
	while ((tClass = GET_BYTE()) != TC_NULL)
	{
		for (info = ThinkerInfo; info->tClass != TC_NULL; info++)
		{
			if (tClass == info->tClass)
			{
				thinker = (thinker_t *) Z_Malloc(info->realsize, PU_LEVEL, NULL);
				memset(thinker, 0, info->realsize);
				memset(buffer, 0, sizeof(buffer));
				memcpy(buffer, SavePtr, info->savesize);
				SavePtr += info->savesize;
				thinker->function = info->thinkerFunc;
				info->restoreFunc(thinker, buffer);
				P_AddThinker(thinker);
				break;
			}
		}
		if (info->tClass == TC_NULL)
		{
			I_Error("UnarchiveThinkers: Unknown tClass %d in savegame", tClass);
		}
	}
}

//==========================================================================
//
// MangleFloorMove
//
//==========================================================================

static void MangleFloorMove(void *arg1, void *arg2)
{
	floormove_t	*fm	= (floormove_t *)	arg1;
	save_floormove_t *temp	= (save_floormove_t *)	arg2;

	temp->sector_idx	= LONG((int32_t)(fm->sector - sectors));
	temp->type		= (int) LONG(fm->type);
	temp->crush		= (int) LONG(fm->crush);
	temp->direction		= (int) LONG(fm->direction);
	temp->newspecial	= (int) LONG(fm->newspecial);
	temp->texture		= (short) SHORT(fm->texture);
	temp->floordestheight	= (fixed_t) LONG(fm->floordestheight);
	temp->speed		= (fixed_t) LONG(fm->speed);
	temp->delayCount	= (int) LONG(fm->delayCount);
	temp->delayTotal	= (int) LONG(fm->delayTotal);
	temp->stairsDelayHeight	= (fixed_t) LONG(fm->stairsDelayHeight);
	temp->stairsDelayHeightDelta = (fixed_t) LONG(fm->stairsDelayHeightDelta);
	temp->resetHeight	= (fixed_t) LONG(fm->resetHeight);
	temp->resetDelay	= (short) SHORT(fm->resetDelay);
	temp->resetDelayCount	= (short) SHORT(fm->resetDelayCount);
	temp->textureChange	= fm->textureChange;
}

//==========================================================================
//
// RestoreFloorMove
//
//==========================================================================

static void RestoreFloorMove(void *arg1, void *arg2)
{
	floormove_t	*fm	= (floormove_t *)	arg1;
	save_floormove_t *temp	= (save_floormove_t *)	arg2;

	temp->sector_idx	= LONG(temp->sector_idx);
	fm->sector		= &sectors[temp->sector_idx];
	fm->sector->specialdata	= T_MoveFloor;	//fm->thinker.function
	fm->type		= (floor_e) LONG(temp->type);
	fm->crush		= (int) LONG(temp->crush);
	fm->direction		= (int) LONG(temp->direction);
	fm->newspecial		= (int) LONG(temp->newspecial);
	fm->texture		= (short) SHORT(temp->texture);
	fm->floordestheight	= (fixed_t) LONG(temp->floordestheight);
	fm->speed		= (fixed_t) LONG(temp->speed);
	fm->delayCount		= (int) LONG(temp->delayCount);
	fm->delayTotal		= (int) LONG(temp->delayTotal);
	fm->stairsDelayHeight	= (fixed_t) LONG(temp->stairsDelayHeight);
	fm->stairsDelayHeightDelta = (fixed_t) LONG(temp->stairsDelayHeightDelta);
	fm->resetHeight		= (fixed_t) LONG(temp->resetHeight);
	fm->resetDelay		= (short) SHORT(temp->resetDelay);
	fm->resetDelayCount	= (short) SHORT(temp->resetDelayCount);
	fm->textureChange	= temp->textureChange;
}

//==========================================================================
//
// MangleLight
//
//==========================================================================

static void MangleLight(void *arg1, void *arg2)
{
	light_t		*light	= (light_t *)	arg1;
	save_light_t	*temp	= (save_light_t *) arg2;

	temp->sector_idx	= LONG((int32_t)(light->sector - sectors));
	temp->type		= (int) LONG(light->type);
	temp->value1		= (int) LONG(light->value1);
	temp->value2		= (int) LONG(light->value2);
	temp->tics1		= (int) LONG(light->tics1);
	temp->tics2		= (int) LONG(light->tics2);
	temp->count		= (int) LONG(light->count);
}

//==========================================================================
//
// RestoreLight
//
//==========================================================================

static void RestoreLight(void *arg1, void *arg2)
{
	light_t		*light	= (light_t *)	arg1;
	save_light_t	*temp	= (save_light_t *) arg2;

	temp->sector_idx	= LONG(temp->sector_idx);
	light->sector		= &sectors[temp->sector_idx];
	light->type		= (lighttype_t) LONG(temp->type);
	light->value1		= (int) LONG(temp->value1);
	light->value2		= (int) LONG(temp->value2);
	light->tics1		= (int) LONG(temp->tics1);
	light->tics2		= (int) LONG(temp->tics2);
	light->count		= (int) LONG(temp->count);
}

//==========================================================================
//
// MangleVerticalDoor
//
//==========================================================================

static void MangleVerticalDoor(void *arg1, void *arg2)
{
	vldoor_t	*vldoor = (vldoor_t *)	arg1;
	save_vldoor_t	*temp	= (save_vldoor_t *) arg2;

	temp->sector_idx	= LONG((int32_t)(vldoor->sector - sectors));
	temp->type		= (int) LONG(vldoor->type);
	temp->topheight		= (fixed_t) LONG(vldoor->topheight);
	temp->speed		= (fixed_t) LONG(vldoor->speed);
	temp->direction		= (int) LONG(vldoor->direction);
	temp->topwait		= (int) LONG(vldoor->topwait);
	temp->topcountdown	= (int) LONG(vldoor->topcountdown);
}

//==========================================================================
//
// RestoreVerticalDoor
//
//==========================================================================

static void RestoreVerticalDoor(void *arg1, void *arg2)
{
	vldoor_t	*vldoor = (vldoor_t *)	arg1;
	save_vldoor_t	*temp	= (save_vldoor_t *) arg2;

	temp->sector_idx	= LONG(temp->sector_idx);
	vldoor->sector		= &sectors[temp->sector_idx];
	vldoor->sector->specialdata = T_VerticalDoor;	//vldoor->thinker.function
	vldoor->type		= (vldoor_e) LONG(temp->type);
	vldoor->topheight	= (fixed_t) LONG(temp->topheight);
	vldoor->speed		= (fixed_t) LONG(temp->speed);
	vldoor->direction	= (int) LONG(temp->direction);
	vldoor->topwait		= (int) LONG(temp->topwait);
	vldoor->topcountdown	= (int) LONG(temp->topcountdown);
}

//==========================================================================
//
// ManglePhase
//
//==========================================================================

static void ManglePhase(void *arg1, void *arg2)
{
	phase_t		*phase	= (phase_t *)	arg1;
	save_phase_t	*temp	= (save_phase_t *) arg2;

	temp->sector_idx	= LONG((int32_t)(phase->sector - sectors));
	temp->index		= (int) LONG(phase->index);
	temp->base		= (int) LONG(phase->base);
}

//==========================================================================
//
// RestorePhase
//
//==========================================================================

static void RestorePhase(void *arg1, void *arg2)
{
	phase_t		*phase	= (phase_t *)	arg1;
	save_phase_t	*temp	= (save_phase_t *) arg2;

	temp->sector_idx	= LONG(temp->sector_idx);
	phase->sector		= &sectors[temp->sector_idx];
	phase->index		= (int) LONG(temp->index);
	phase->base		= (int) LONG(temp->base);
}

//==========================================================================
//
// ManglePillar
//
//==========================================================================

static void ManglePillar(void *arg1, void *arg2)
{
	pillar_t	*pillar = (pillar_t *)	arg1;
	save_pillar_t	*temp	= (save_pillar_t *) arg2;

	temp->sector_idx	= LONG((int32_t)(pillar->sector - sectors));
	temp->ceilingSpeed	= (int) LONG(pillar->ceilingSpeed);
	temp->floorSpeed	= (int) LONG(pillar->floorSpeed);
	temp->floordest		= (int) LONG(pillar->floordest);
	temp->ceilingdest	= (int) LONG(pillar->ceilingdest);
	temp->direction		= (int) LONG(pillar->direction);
	temp->crush		= (int) LONG(pillar->crush);
}

//==========================================================================
//
// RestorePillar
//
//==========================================================================

static void RestorePillar(void *arg1, void *arg2)
{
	pillar_t	*pillar = (pillar_t *)	arg1;
	save_pillar_t	*temp	= (save_pillar_t *) arg2;

	temp->sector_idx	= LONG(temp->sector_idx);
	pillar->sector		= &sectors[temp->sector_idx];
	pillar->sector->specialdata = T_BuildPillar;	//pillar->thinker.function
	pillar->ceilingSpeed	= (int) LONG(temp->ceilingSpeed);
	pillar->floorSpeed	= (int) LONG(temp->floorSpeed);
	pillar->floordest	= (int) LONG(temp->floordest);
	pillar->ceilingdest	= (int) LONG(temp->ceilingdest);
	pillar->direction	= (int) LONG(temp->direction);
	pillar->crush		= (int) LONG(temp->crush);
}

//==========================================================================
//
// MangleFloorWaggle
//
//==========================================================================

static void MangleFloorWaggle(void *arg1, void *arg2)
{
	floorWaggle_t	*fw	= (floorWaggle_t *)	arg1;
	save_floorWaggle_t *temp = (save_floorWaggle_t *) arg2;

	temp->sector_idx	= LONG((int32_t)(fw->sector - sectors));
	temp->originalHeight	= (fixed_t) LONG(fw->originalHeight);
	temp->accumulator	= (fixed_t) LONG(fw->accumulator);
	temp->accDelta		= (fixed_t) LONG(fw->accDelta);
	temp->targetScale	= (fixed_t) LONG(fw->targetScale);
	temp->scale		= (fixed_t) LONG(fw->scale);
	temp->scaleDelta	= (fixed_t) LONG(fw->scaleDelta);
	temp->ticker		= (int) LONG(fw->ticker);
	temp->state		= (int) LONG(fw->state);
}

//==========================================================================
//
// RestoreFloorWaggle
//
//==========================================================================

static void RestoreFloorWaggle(void *arg1, void *arg2)
{
	floorWaggle_t	*fw	= (floorWaggle_t *)	arg1;
	save_floorWaggle_t *temp = (save_floorWaggle_t *) arg2;

	temp->sector_idx	= LONG(temp->sector_idx);
	fw->sector		= &sectors[temp->sector_idx];
	fw->sector->specialdata = T_FloorWaggle;	//fw->thinker.function
	fw->originalHeight	= (fixed_t) LONG(temp->originalHeight);
	fw->accumulator		= (fixed_t) LONG(temp->accumulator);
	fw->accDelta		= (fixed_t) LONG(temp->accDelta);
	fw->targetScale		= (fixed_t) LONG(temp->targetScale);
	fw->scale		= (fixed_t) LONG(temp->scale);
	fw->scaleDelta		= (fixed_t) LONG(temp->scaleDelta);
	fw->ticker		= (int) LONG(temp->ticker);
	fw->state		= (int) LONG(temp->state);
}

//==========================================================================
//
// ManglePolyEvent
//
//==========================================================================

static void ManglePolyEvent(void *arg1, void *arg2)
{
	polyevent_t	*pe	= (polyevent_t *) arg1;
	save_polyevent_t *temp	= (save_polyevent_t *) arg2;

	temp->polyobj		= LONG(pe->polyobj);
	temp->speed		= LONG(pe->speed);
	temp->dist		= LONG(pe->dist);
	temp->angle		= LONG(pe->angle);
	temp->xSpeed		= LONG(pe->xSpeed);
	temp->ySpeed		= LONG(pe->ySpeed);
}

//==========================================================================
//
// RestorePolyEvent
//
//==========================================================================

static void RestorePolyEvent(void *arg1, void *arg2)
{
	polyevent_t	*pe	= (polyevent_t *) arg1;
	save_polyevent_t *temp	= (save_polyevent_t *) arg2;

	pe->polyobj		= (int) LONG(temp->polyobj);
	pe->speed		= (int) LONG(temp->speed);
	pe->dist		= (unsigned int) LONG(temp->dist);
	pe->angle		= (int) LONG(temp->angle);
	pe->xSpeed		= (fixed_t) LONG(temp->xSpeed);
	pe->ySpeed		= (fixed_t) LONG(temp->ySpeed);
}

//==========================================================================
//
// ManglePolyDoor
//
//==========================================================================

static void ManglePolyDoor(void *arg1, void *arg2)
{
	polydoor_t	*pd	= (polydoor_t *) arg1;
	save_polydoor_t *temp	= (save_polydoor_t *) arg2;

	temp->polyobj		= (int) LONG(pd->polyobj);
	temp->speed		= (int) LONG(pd->speed);
	temp->dist		= (int) LONG(pd->dist);
	temp->totalDist		= (int) LONG(pd->totalDist);
	temp->direction		= (int) LONG(pd->direction);
	temp->xSpeed		= (fixed_t) LONG(pd->xSpeed);
	temp->ySpeed		= (fixed_t) LONG(pd->ySpeed);
	temp->tics		= (int) LONG(pd->tics);
	temp->waitTics		= (int) LONG(pd->waitTics);
	temp->type		= (int) LONG(pd->type);
	temp->close		= (int) LONG(pd->close);
}

//==========================================================================
//
// RestorePolyEvent
//
//==========================================================================

static void RestorePolyDoor(void *arg1, void *arg2)
{
	polydoor_t	*pd	= (polydoor_t *) arg1;
	save_polydoor_t *temp	= (save_polydoor_t *) arg2;

	pd->polyobj		= (int) LONG(temp->polyobj);
	pd->speed		= (int) LONG(temp->speed);
	pd->dist		= (int) LONG(temp->dist);
	pd->totalDist		= (int) LONG(temp->totalDist);
	pd->direction		= (int) LONG(temp->direction);
	pd->xSpeed		= (fixed_t) LONG(temp->xSpeed);
	pd->ySpeed		= (fixed_t) LONG(temp->ySpeed);
	pd->tics		= (int) LONG(temp->tics);
	pd->waitTics		= (int) LONG(temp->waitTics);
	pd->type		= (podoortype_t) LONG(temp->type);
	pd->close		= !!(LONG(temp->close));
}

//==========================================================================
//
// MangleScript
//
//==========================================================================

static void MangleScript(void *arg1, void *arg2)
{
	int		i;
	acs_t 	*script		= (acs_t *)	arg1;
	save_acs_t *temp	= (save_acs_t *) arg2;

	temp->ip_idx		= LONG((int32_t)((intptr_t)(script->ip) - (intptr_t)ActionCodeBase));
	temp->line_idx		= script->line ? LONG((int32_t)(script->line - lines)) : (int32_t)LONG(-1);
	temp->activator_idx	= (int32_t) LONG(GetMobjNum(script->activator));
	temp->side		= (int) LONG(script->side);
	temp->number		= (int) LONG(script->number);
	temp->infoIndex		= (int) LONG(script->infoIndex);
	temp->delayCount	= (int) LONG(script->delayCount);
	temp->stackPtr		= (int) LONG(script->stackPtr);
	for (i = 0; i < ACS_STACK_DEPTH; i++)
	{
		temp->stack[i]	= (int) LONG(script->stack[i]);
	}
	for (i = 0; i < MAX_ACS_SCRIPT_VARS; i++)
	{
		temp->vars[i]	= (int) LONG(script->vars[i]);
	}
}

//==========================================================================
//
// RestoreScript
//
//==========================================================================

static void RestoreScript(void *arg1, void *arg2)
{
	int		i;
	acs_t	*script		= (acs_t *)	arg1;
	save_acs_t *temp	= (save_acs_t *) arg2;

	temp->ip_idx		= (int32_t) LONG(temp->ip_idx);
	temp->line_idx		= (int32_t) LONG(temp->line_idx);
	temp->activator_idx	= (int32_t) LONG(temp->activator_idx);
	script->ip		= ActionCodeBase + temp->ip_idx;
	if (temp->line_idx == -1)
	{
		script->line	= NULL;
	}
	else
	{
		script->line	= &lines[temp->line_idx];
	}
	script->activator	= GetMobjPtr(temp->activator_idx, (intptr_t *)(void *)&script->activator);
	script->side		= (int) LONG(temp->side);
	script->number		= (int) LONG(temp->number);
	script->infoIndex	= (int) LONG(temp->infoIndex);
	script->delayCount	= (int) LONG(temp->delayCount);
	script->stackPtr	= (int) LONG(temp->stackPtr);
	for (i = 0; i < ACS_STACK_DEPTH; i++)
	{
		script->stack[i] = (int) LONG(temp->stack[i]);
	}
	for (i = 0; i < MAX_ACS_SCRIPT_VARS; i++)
	{
		script->vars[i]	= (int) LONG(temp->vars[i]);
	}
}

//==========================================================================
//
// ManglePlatRaise
//
//==========================================================================

static void ManglePlatRaise(void *arg1, void *arg2)
{
	plat_t		*plat	= (plat_t *)	arg1;
	save_plat_t	*temp	= (save_plat_t *) arg2;

	temp->sector_idx	= LONG((int32_t)(plat->sector - sectors));
	temp->speed		= (fixed_t) LONG(plat->speed);
	temp->low		= (fixed_t) LONG(plat->low);
	temp->high		= (fixed_t) LONG(plat->high);
	temp->wait		= (int) LONG(plat->wait);
	temp->count		= (int) LONG(plat->count);
	temp->status		= (int) LONG(plat->status);
	temp->oldstatus		= (int) LONG(plat->oldstatus);
	temp->crush		= (int) LONG(plat->crush);
	temp->tag		= (int) LONG(plat->tag);
	temp->type		= (int) LONG(plat->type);
}

//==========================================================================
//
// RestorePlatRaise
//
//==========================================================================

static void RestorePlatRaise(void *arg1, void *arg2)
{
	plat_t		*plat	= (plat_t *)	arg1;
	save_plat_t	*temp	= (save_plat_t *) arg2;

	temp->sector_idx	= LONG(temp->sector_idx);
	plat->sector		= &sectors[temp->sector_idx];
	plat->sector->specialdata = T_PlatRaise;
	plat->speed		= (fixed_t) LONG(temp->speed);
	plat->low		= (fixed_t) LONG(temp->low);
	plat->high		= (fixed_t) LONG(temp->high);
	plat->wait		= (int) LONG(temp->wait);
	plat->count		= (int) LONG(temp->count);
	plat->status		= (plat_e) LONG(temp->status);
	plat->oldstatus		= (plat_e) LONG(temp->oldstatus);
	plat->crush		= (int) LONG(temp->crush);
	plat->tag		= (int) LONG(temp->tag);
	plat->type		= (plattype_e) LONG(temp->type);
	P_AddActivePlat(plat);
}

//==========================================================================
//
// MangleMoveCeiling
//
//==========================================================================

static void MangleMoveCeiling(void *arg1, void *arg2)
{
	ceiling_t	*ceiling = (ceiling_t *) arg1;
	save_ceiling_t	*temp	= (save_ceiling_t *) arg2;

	temp->sector_idx	= LONG((int32_t)(ceiling->sector - sectors));
	temp->type		= (int) LONG(ceiling->type);
	temp->bottomheight	= (fixed_t) LONG(ceiling->bottomheight);
	temp->topheight		= (fixed_t) LONG(ceiling->topheight);
	temp->speed		= (fixed_t) LONG(ceiling->speed);
	temp->crush		= (int) LONG(ceiling->crush);
	temp->direction		= (int) LONG(ceiling->direction);
	temp->tag		= (int) LONG(ceiling->tag);
	temp->olddirection	= (int) LONG(ceiling->olddirection);
}

//==========================================================================
//
// RestoreMoveCeiling
//
//==========================================================================

static void RestoreMoveCeiling(void *arg1, void *arg2)
{
	ceiling_t	*ceiling = (ceiling_t *) arg1;
	save_ceiling_t	*temp	= (save_ceiling_t *) arg2;

	temp->sector_idx	= LONG(temp->sector_idx);
	ceiling->sector		= &sectors[temp->sector_idx];
	ceiling->sector->specialdata = T_MoveCeiling;
	ceiling->type		= (ceiling_e) LONG(temp->type);
	ceiling->bottomheight	= (fixed_t) LONG(temp->bottomheight);
	ceiling->topheight	= (fixed_t) LONG(temp->topheight);
	ceiling->speed		= (fixed_t) LONG(temp->speed);
	ceiling->crush		= (int) LONG(temp->crush);
	ceiling->direction	= (int) LONG(temp->direction);
	ceiling->tag		= (int) LONG(temp->tag);
	ceiling->olddirection	= (int) LONG(temp->olddirection);
	P_AddActiveCeiling(ceiling);
}

//==========================================================================
//
// ArchiveScripts
//
//==========================================================================

static void ArchiveScripts(void)
{
	int i;

	StreamOutLong(ASEG_SCRIPTS);
	for (i = 0; i < ACScriptCount; i++)
	{
		StreamOutWord(ACSInfo[i].state);
		StreamOutWord(ACSInfo[i].waitValue);
	}
	for (i = 0; i < MAX_ACS_MAP_VARS; i++)
	{
		StreamOutLong(MapVars[i]);
	}
}

//==========================================================================
//
// UnarchiveScripts
//
//==========================================================================

static void UnarchiveScripts(void)
{
	int i;

	AssertSegment(ASEG_SCRIPTS);
	for (i = 0; i < ACScriptCount; i++)
	{
		ACSInfo[i].state = GET_WORD();
		ACSInfo[i].waitValue = GET_WORD();
	}
	for (i = 0; i < MAX_ACS_MAP_VARS; i++)
	{
		MapVars[i] = GET_LONG();
	}
}

//==========================================================================
//
// ArchiveMisc
//
//==========================================================================

static void ArchiveMisc(void)
{
	int ix;

	StreamOutLong(ASEG_MISC);
	for (ix = 0; ix < MAXPLAYERS; ix++)
	{
		StreamOutLong(localQuakeHappening[ix]);
	}
}

//==========================================================================
//
// UnarchiveMisc
//
//==========================================================================

static void UnarchiveMisc(void)
{
	int ix;

	AssertSegment(ASEG_MISC);
	for (ix = 0; ix < MAXPLAYERS; ix++)
	{
		localQuakeHappening[ix] = GET_LONG();
	}
}

//==========================================================================
//
// RemoveAllThinkers
//
//==========================================================================

static void RemoveAllThinkers(void)
{
	thinker_t *thinker;
	thinker_t *nextThinker;

	thinker = thinkercap.next;
	while (thinker != &thinkercap)
	{
		nextThinker = thinker->next;
		if (thinker->function == P_MobjThinker)
		{
			P_RemoveMobj((mobj_t *)thinker);
		}
		else
		{
			Z_Free(thinker);
		}
		thinker = nextThinker;
	}
	P_InitThinkers();
}

//==========================================================================
//
// ArchiveSounds
//
//==========================================================================

static void ArchiveSounds(void)
{
	seqnode_t *node;
	sector_t *sec;
	int difference;
	int i;

	StreamOutLong(ASEG_SOUNDS);

	// Save the sound sequences
	StreamOutLong(ActiveSequences);
	for (node = SequenceListHead; node; node = node->next)
	{
		StreamOutLong(node->sequence);
		StreamOutLong(node->delayTics);
		StreamOutLong(node->volume);
		StreamOutLong(SN_GetSequenceOffset(node->sequence, node->sequencePtr));
		StreamOutLong(node->currentSoundID);
		for (i = 0; i < po_NumPolyobjs; i++)
		{
			if (node->mobj == (mobj_t *)&polyobjs[i].startSpot)
			{
				break;
			}
		}
		if (i == po_NumPolyobjs)
		{ // Sound is attached to a sector, not a polyobj
			sec = R_PointInSubsector(node->mobj->x, node->mobj->y)->sector;
			difference = (int)(((byte *)sec - (byte *)&sectors[0]) / sizeof(sector_t));
			StreamOutLong(0); // 0 -- sector sound origin
		}
		else
		{
			StreamOutLong(1); // 1 -- polyobj sound origin
			difference = i;
		}
		StreamOutLong(difference);
	}
}

//==========================================================================
//
// UnarchiveSounds
//
//==========================================================================

static void UnarchiveSounds(void)
{
	int i;
	int numSequences;
	int sequence;
	int delayTics;
	int volume;
	int seqOffset;
	int soundID;
	int polySnd;
	int secNum;
	mobj_t *sndMobj;

	AssertSegment(ASEG_SOUNDS);

	// Reload and restart all sound sequences
	numSequences = GET_LONG();
	i = 0;
	while (i < numSequences)
	{
		sequence = GET_LONG();
		delayTics = GET_LONG();
		volume = GET_LONG();
		seqOffset = GET_LONG();

		soundID = GET_LONG();
		polySnd = GET_LONG();
		secNum = GET_LONG();
		if (!polySnd)
		{
			sndMobj = (mobj_t *)(void *)&sectors[secNum].soundorg;
		}
		else
		{
			sndMobj = (mobj_t *)&polyobjs[secNum].startSpot;
		}
		SN_StartSequence(sndMobj, sequence);
		SN_ChangeNodeData(i, seqOffset, delayTics, volume, soundID);
		i++;
	}
}

//==========================================================================
//
// ArchivePolyobjs
//
//==========================================================================

static void ArchivePolyobjs(void)
{
	int i;

	StreamOutLong(ASEG_POLYOBJS);
	StreamOutLong(po_NumPolyobjs);
	for (i = 0; i < po_NumPolyobjs; i++)
	{
		StreamOutLong(polyobjs[i].tag);
		StreamOutLong(polyobjs[i].angle);
		StreamOutLong(polyobjs[i].startSpot.x);
		StreamOutLong(polyobjs[i].startSpot.y);
	}
}

//==========================================================================
//
// UnarchivePolyobjs
//
//==========================================================================

static void UnarchivePolyobjs(void)
{
	int i;
	fixed_t deltaX;
	fixed_t deltaY;

	AssertSegment(ASEG_POLYOBJS);
	if (GET_LONG() != po_NumPolyobjs)
	{
		I_Error("UnarchivePolyobjs: Bad polyobj count");
	}
	for (i = 0; i < po_NumPolyobjs; i++)
	{
		if (GET_LONG() != polyobjs[i].tag)
		{
			I_Error("UnarchivePolyobjs: Invalid polyobj tag");
		}
		PO_RotatePolyobj(polyobjs[i].tag, (angle_t)GET_LONG());
		deltaX = GET_LONG() - polyobjs[i].startSpot.x;
		deltaY = GET_LONG() - polyobjs[i].startSpot.y;
		PO_MovePolyobj(polyobjs[i].tag, deltaX, deltaY);
	}
}

//==========================================================================
//
// AssertSegment
//
//==========================================================================

static void AssertSegment(gameArchiveSegment_t segType)
{
	if (GET_LONG() != segType)
	{
		I_Error("Corrupt save game: Segment [%d] failed alignment check", segType);
	}
}

//==========================================================================
//
// ClearSaveSlot
//
// Deletes all save game files associated with a slot number.
//
//==========================================================================

static void ClearSaveSlot(int slot)
{
	int i;
	char fileName[MAX_OSPATH];

	for (i = 0; i < MAX_MAPS; i++)
	{
		snprintf(fileName, sizeof(fileName), "%shex%d%02d.hxs", basePath, slot, i);
		remove(fileName);
	}
	snprintf(fileName, sizeof(fileName), "%shex%d.hxs", basePath, slot);
	remove(fileName);
}

//==========================================================================
//
// CopySaveSlot
//
// Copies all the save game files from one slot to another.
//
//==========================================================================

static void CopySaveSlot(int sourceSlot, int destSlot)
{
	int i;
	char sourceName[MAX_OSPATH];
	char destName[MAX_OSPATH];

	for (i = 0; i < MAX_MAPS; i++)
	{
		snprintf(sourceName, sizeof(sourceName), "%shex%d%02d.hxs", basePath,sourceSlot, i);
		if (ExistingFile(sourceName))
		{
			snprintf(destName, sizeof(destName), "%shex%d%02d.hxs", basePath,destSlot, i);
			CopyFile(sourceName, destName);
		}
	}
	snprintf(sourceName, sizeof(sourceName), "%shex%d.hxs", basePath, sourceSlot);
	if (ExistingFile(sourceName))
	{
		snprintf(destName, sizeof(destName), "%shex%d.hxs", basePath, destSlot);
		CopyFile(sourceName, destName);
	}
}

//==========================================================================
//
// CopyFile
//
//==========================================================================

static void CopyFile(const char *sourceName, const char *destName)
{
	int length;
	void *buffer;

	length = M_ReadFile(sourceName, &buffer);
	M_WriteFile(destName, buffer, length);
	Z_Free(buffer);
}

//==========================================================================
//
// ExistingFile
//
//==========================================================================

static boolean ExistingFile(const char *name)
{
	FILE *fp;

	if ((fp = fopen(name, "rb")) != NULL)
	{
		fclose(fp);
		return true;
	}
	else
	{
		return false;
	}
}

//==========================================================================
//
// OpenStreamOut
//
//==========================================================================

static void OpenStreamOut(const char *fileName)
{
	SavingFP = fopen(fileName, "wb");
}

//==========================================================================
//
// CloseStreamOut
//
//==========================================================================

static void CloseStreamOut(void)
{
	if (SavingFP)
	{
		fclose(SavingFP);
	}
}

//==========================================================================
//
// StreamOutBuffer
//
//==========================================================================

static void StreamOutBuffer(const void *buffer, size_t size)
{
	fwrite(buffer, size, 1, SavingFP);
}

//==========================================================================
//
// StreamOutByte
//
//==========================================================================

static void StreamOutByte(byte val)
{
	fwrite(&val, sizeof(byte), 1, SavingFP);
}

//==========================================================================
//
// StreamOutWord
//
//==========================================================================

static void StreamOutWord(uint16_t val)
{
	uint16_t tmp = (uint16_t) SHORT(val);
	fwrite(&tmp, sizeof(uint16_t), 1, SavingFP);
}

//==========================================================================
//
// StreamOutLong
//
//==========================================================================

static void StreamOutLong(uint32_t val)
{
	uint32_t tmp = (uint32_t) LONG(val);
	fwrite(&tmp, sizeof(uint32_t), 1, SavingFP);
}

