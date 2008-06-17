
//**************************************************************************
//**
//** h2_main.c : Heretic 2 : Raven Software, Corp.
//**
//** $RCSfile: h2_main.c,v $
//** $Revision: 1.19 $
//** $Date: 2008-06-17 17:07:24 $
//** $Author: sezero $
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "h2stdinc.h"
#include <sys/stat.h>
#include <time.h>
#include "h2def.h"
#include "p_local.h"
#include "soundst.h"

#ifdef RENDER3D
#include "ogl_def.h"
#define W_CacheLumpName(a,b)        W_GetNumForName(a)
#define V_DrawPatch(x,y,p)          OGL_DrawPatch(x,y,p)
#define V_DrawRawScreen(a)          OGL_DrawRawScreen(a)
#endif


// MACROS ------------------------------------------------------------------

#define CONFIG_FILE_NAME "hhexen.cfg"
#define MAXWADFILES 20

// TYPES -------------------------------------------------------------------

typedef struct
{
	char *name;
	void (*func)(char **args, int tag);
	int requiredArgs;
	int tag;
} execOpt_t;

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

void R_ExecuteSetViewSize(void);
void D_CheckNetGame(void);
void G_BuildTiccmd(ticcmd_t *cmd);
void F_Drawer(void);
boolean F_Responder(event_t *ev);
void I_StartupKeyboard(void);
void I_StartupJoystick(void);
void I_ShutdownKeyboard(void);
void S_InitScript(void);

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

void H2_ProcessEvents(void);
void H2_DoAdvanceDemo(void);
void H2_AdvanceDemo(void);
void H2_StartTitle(void);
void H2_PageTicker(void);

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static void DrawMessage(void);
static void PageDrawer(void);
static void HandleArgs(void);
static void CheckRecordFrom(void);
static void AddWADFile(char *file);
static void DrawAndBlit(void);
static void ExecOptionFILE(char **args, int tag);
static void ExecOptionSCRIPTS(char **args, int tag);
static void ExecOptionDEVMAPS(char **args, int tag);
static void ExecOptionSKILL(char **args, int tag);
static void ExecOptionPLAYDEMO(char **args, int tag);
static void ExecOptionMAXZONE(char **args, int tag);
static void CreateBasePath(void);
static void WarpCheck(void);


// EXTERNAL DATA DECLARATIONS ----------------------------------------------

extern boolean automapactive;
extern boolean MenuActive;
extern boolean askforquit;

// PUBLIC DATA DEFINITIONS -------------------------------------------------

char *basePath;
char base[MAX_OSPATH];
boolean DevMaps;			// true = Map development mode
char *DevMapsDir = "";		// development maps directory
boolean shareware;			// true if only episode 1 present
boolean nomonsters;			// checkparm of -nomonsters
boolean respawnparm;		// checkparm of -respawn
boolean randomclass;		// checkparm of -randclass
boolean debugmode;			// checkparm of -debug
boolean ravpic;				// checkparm of -ravpic
boolean cdrom;				// true if cd-rom mode active
boolean cmdfrag;			// true if a CMD_FRAG packet should be sent out
boolean singletics;			// debug flag to cancel adaptiveness
boolean artiskip;			// whether shift-enter skips an artifact
int maxzone = 0x800000;		// Maximum allocated for zone heap (8meg default)
skill_t startskill;
int startepisode;
int startmap;
boolean autostart;
boolean advancedemo;
FILE *debugfile;
event_t events[MAXEVENTS];
int eventhead;
int eventtail;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static int WarpMap;
static int demosequence;
static int pagetic;
static char *pagename;
static char *wadfiles[MAXWADFILES] =
{
	"hexen.wad",
#ifdef ASSASSIN
	"assassin.wad",
#endif
	NULL	/* the last entry MUST be NULL */
};
static execOpt_t ExecOptions[] =
{
	{ "-file", ExecOptionFILE, 1, 0 },
	{ "-scripts", ExecOptionSCRIPTS, 1, 0 },
	{ "-devmaps", ExecOptionDEVMAPS, 1, 0 },
	{ "-skill", ExecOptionSKILL, 1, 0 },
	{ "-playdemo", ExecOptionPLAYDEMO, 1, 0 },
	{ "-timedemo", ExecOptionPLAYDEMO, 1, 0 },
	{ "-maxzone", ExecOptionMAXZONE, 1, 0 },
	{ NULL, NULL, 0, 0 } // Terminator
};

// CODE --------------------------------------------------------------------

//==========================================================================
//
// H2_Main
//
//==========================================================================
void InitMapMusicInfo(void);

void H2_Main(void)
{
	int p;

	M_FindResponseFile();
	setbuf(stdout, NULL);
	startepisode = 1;
	autostart = false;
	startskill = sk_medium;
	startmap = 1;

#ifdef DEMO_WAD
	shareware = true;
#else
	shareware = false; // Always false for Hexen
#endif

	HandleArgs();

	CreateBasePath();

	// Initialize subsystems

	ST_Message("V_Init: allocate screens.\n");
	V_Init();

	// Load defaults before initing other systems
	ST_Message("M_LoadDefaults: Load system defaults.\n");
	M_LoadDefaults(CONFIG_FILE_NAME);

	// HEXEN MODIFICATION:
	// There is a realloc() in W_AddFile() that might fail if the zone
	// heap has been previously allocated, so we need to initialize the
	// WAD files BEFORE the zone memory initialization.
	ST_Message("W_Init: Init WADfiles.\n");
	W_InitMultipleFiles(wadfiles);


#ifndef DEMO_WAD
	W_CheckForOldFiles();
#endif
	

	ST_Message("Z_Init: Init zone memory allocation daemon.\n");
	Z_Init();


	ST_Message("MN_Init: Init menu system.\n");
	MN_Init();

	ST_Message("CT_Init: Init chat mode data.\n");
	CT_Init();

	InitMapMusicInfo();		// Init music fields in mapinfo

#if defined(__linux)
	ST_Message("S_InitScript\n");
	S_InitScript();
#endif

	ST_Message("SN_InitSequenceScript: Registering sound sequences.\n");
	SN_InitSequenceScript();
	ST_Message("I_Init: Setting up machine state.\n");
	I_Init();

	ST_Message("ST_Init: Init startup screen.\n");
	ST_Init();

	S_StartSongName("orb", true);

	// Show version message now, so it's visible during R_Init()
	ST_Message("Executable: "VERSIONTEXT".\n");

	ST_Message("R_Init: Init Hexen refresh daemon");
	R_Init();
	ST_Message("\n");

	if (M_CheckParm("-net")) ST_NetProgress();	// Console player found

	ST_Message("P_Init: Init Playloop state.\n");
	P_Init();

	// Check for command line warping. Follows P_Init() because the
	// MAPINFO.TXT script must be already processed.
	WarpCheck();

	if(autostart)
	{
		ST_Message("Warp to Map %d (\"%s\":%d), Skill %d\n",
			WarpMap, P_GetMapName(startmap), startmap, startskill+1);
	}

	ST_Message("D_CheckNetGame: Checking network game status.\n");
	D_CheckNetGame();

	ST_Message("SB_Init: Loading patches.\n");
	SB_Init();
	
	CheckRecordFrom();

	p = M_CheckParm("-record");
	if(p && p < myargc-1)
	{
		G_RecordDemo(startskill, 1, startepisode, startmap, myargv[p+1]);
		H2_GameLoop(); // Never returns
	}

	p = M_CheckParm("-playdemo");
	if(p && p < myargc-1)
	{
		singledemo = true; // Quit after one demo
		G_DeferedPlayDemo(myargv[p+1]);
		H2_GameLoop(); // Never returns
	}

	p = M_CheckParm("-timedemo");
	if(p && p < myargc-1)
	{
		G_TimeDemo(myargv[p+1]);
		H2_GameLoop(); // Never returns
	}

	p = M_CheckParm("-loadgame");
	if(p && p < myargc-1)
	{
		G_LoadGame(atoi(myargv[p+1]));
	}

	if(gameaction != ga_loadgame)
	{
		UpdateState |= I_FULLSCRN;
		BorderNeedRefresh = true;
		if(autostart || netgame)
		{
			G_StartNewInit();
			G_InitNew(startskill, startepisode, startmap);
		}
		else
		{
			H2_StartTitle();
		}
	}
	H2_GameLoop(); // Never returns
}

//==========================================================================
//
// HandleArgs
//
//==========================================================================

static void HandleArgs(void)
{
	int p;
	execOpt_t *opt;

	nomonsters = M_ParmExists("-nomonsters");
	respawnparm = M_ParmExists("-respawn");
	randomclass = M_ParmExists("-randclass");
	ravpic = M_ParmExists("-ravpic");
	artiskip = M_ParmExists("-artiskip");
	debugmode = M_ParmExists("-debug");
	deathmatch = M_ParmExists("-deathmatch");
	cdrom = M_ParmExists("-cdrom");
	cmdfrag = M_ParmExists("-cmdfrag");

	// Process command line options
	for(opt = ExecOptions; opt->name != NULL; opt++)
	{
		p = M_CheckParm(opt->name);
		if(p && p < myargc-opt->requiredArgs)
		{
			opt->func(&myargv[p], opt->tag);
		}
	}

	// Look for an external device driver
	I_CheckExternDriver();
}

//==========================================================================
//
// WarpCheck
//
//==========================================================================

static void WarpCheck(void)
{
	int p;
	int map;

	p = M_CheckParm("-warp");
	if(p && p < myargc-1)
	{
		WarpMap = atoi(myargv[p+1]);
		map = P_TranslateMap(WarpMap);
		if(map == -1)
		{ // Couldn't find real map number
			startmap = 1;
			ST_Message("-WARP: Invalid map number.\n");
		}
		else
		{ // Found a valid startmap
			startmap = map;
			autostart = true;
		}
	}
	else
	{
		WarpMap = 1;
		startmap = P_TranslateMap(1);
		if(startmap == -1)
		{
			startmap = 1;
		}
	}
}

//==========================================================================
//
// ExecOptionSKILL
//
//==========================================================================

static void ExecOptionSKILL(char **args, int tag)
{
	startskill = args[1][0]-'1';
	autostart = true;
}

//==========================================================================
//
// ExecOptionFILE
//
//==========================================================================

static void ExecOptionFILE(char **args, int tag)
{
	int p;

	p = M_CheckParm("-file");
	while(++p != myargc && myargv[p][0] != '-')
	{
		AddWADFile(myargv[p]);
	}
}


//==========================================================================
//
// ExecOptionPLAYDEMO
//
//==========================================================================

static void ExecOptionPLAYDEMO(char **args, int tag)
{
	char file[256];

	snprintf(file, sizeof(file), "%s.lmp", args[1]);
	AddWADFile(file);
	ST_Message("Playing demo %s.lmp.\n", args[1]);
}

//==========================================================================
//
// ExecOptionSCRIPTS
//
//==========================================================================

static void ExecOptionSCRIPTS(char **args, int tag)
{
	sc_FileScripts = true;
	sc_ScriptsDir = args[1];
}

//==========================================================================
//
// ExecOptionDEVMAPS
//
//==========================================================================

static void ExecOptionDEVMAPS(char **args, int tag)
{
	DevMaps = true;
	ST_Message("Map development mode enabled:\n");
	ST_Message("[config    ] = %s\n", args[1]);
	SC_OpenFileCLib(args[1]);
	SC_MustGetStringName("mapsdir");
	SC_MustGetString();
	ST_Message("[mapsdir   ] = %s\n", sc_String);
	DevMapsDir = malloc(strlen(sc_String)+1);
	strcpy(DevMapsDir, sc_String);
	SC_MustGetStringName("scriptsdir");
	SC_MustGetString();
	ST_Message("[scriptsdir] = %s\n", sc_String);
	sc_FileScripts = true;
	sc_ScriptsDir = malloc(strlen(sc_String)+1);
	strcpy(sc_ScriptsDir, sc_String);
	while(SC_GetString())
	{
		if(SC_Compare("file"))
		{
			SC_MustGetString();
			AddWADFile(sc_String);
		}
		else
		{
			SC_ScriptError(NULL);
		}
	}
	SC_Close();
}


long superatol(char *s)
{
	long int n=0, r=10, x, mul=1;
	char *c=s;

	for (; *c; c++)
	{
		x = (*c & 223) - 16;

		if (x == -3)
		{
			mul = -mul;
		}
		else if (x == 72 && r == 10)
		{
			n -= (r=n);
			if (!r) r=16;
			if (r<2 || r>36) return -1;
		}
		else
		{
			if (x>10) x-=39;
			if (x >= r) return -1;
			n = (n*r) + x;
		}
	}
	return(mul*n);
}


static void ExecOptionMAXZONE(char **args, int tag)
{
	int size;
	
	size = (int) superatol(args[1]);
	if (size < MINIMUM_HEAP_SIZE) size = MINIMUM_HEAP_SIZE;
	if (size > MAXIMUM_HEAP_SIZE) size = MAXIMUM_HEAP_SIZE;
	maxzone = size;
}

//==========================================================================
//
// H2_GameLoop
//
//==========================================================================

void H2_GameLoop(void)
{
	if(M_CheckParm("-debugfile"))
	{
		char filename[20];
		snprintf(filename, sizeof(filename), "debug%i.txt", consoleplayer);
		debugfile = fopen(filename,"w");
	}
	I_InitGraphics();
	while(1)
	{
		// Frame syncronous IO operations
		I_StartFrame();

		// Process one or more tics
		if(singletics)
		{
			I_StartTic();
			H2_ProcessEvents();
			G_BuildTiccmd(&netcmds[consoleplayer][maketic%BACKUPTICS]);
			if(advancedemo)
			{
				H2_DoAdvanceDemo();
			}
			G_Ticker();
			gametic++;
			maketic++;
		}
		else
		{
			// Will run at least one tic
			TryRunTics();
		}

		// Move positional sounds
		S_UpdateSounds(players[displayplayer].mo);

		DrawAndBlit();
	}
}

//==========================================================================
//
// H2_ProcessEvents
//
// Send all the events of the given timestamp down the responder chain.
//
//==========================================================================

void H2_ProcessEvents(void)
{
	event_t *ev;

	while (eventtail != eventhead)
	{
		ev = &events[eventtail];
		if(F_Responder(ev))
		{
			goto _next_ev;
		}
		if(MN_Responder(ev))
		{
			goto _next_ev;
		}
		G_Responder(ev);
	_next_ev:
		eventtail = (eventtail + 1) & (MAXEVENTS - 1);
	}
}

//==========================================================================
//
// H2_PostEvent
//
// Called by the I/O functions when input is detected.
//
//==========================================================================

void H2_PostEvent(event_t *ev)
{
	events[eventhead] = *ev;
	eventhead++;
	eventhead &= (MAXEVENTS - 1);
}

//==========================================================================
//
// DrawAndBlit
//
//==========================================================================

static void DrawAndBlit(void)
{
	// Change the view size if needed
	if(setsizeneeded)
	{
		R_ExecuteSetViewSize();
	}

	// Do buffered drawing
	switch(gamestate)
	{
		case GS_LEVEL:
			if(!gametic)
				break;
			/* bbm 3/9/2003: Always render player map transparent */
			R_RenderPlayerView(&players[displayplayer]);
			if(automapactive)
				AM_Drawer();
			CT_Drawer();
			UpdateState |= I_FULLVIEW;
			SB_Drawer();
			break;
		case GS_INTERMISSION:
			IN_Drawer();
			break;
		case GS_FINALE:
			F_Drawer();
			break;
		case GS_DEMOSCREEN:
			PageDrawer();
			break;
	}

	if(paused && !MenuActive && !askforquit)
	{
		if(!netgame)
		{
			V_DrawPatch(160, viewwindowy+5, W_CacheLumpName("PAUSED",
				PU_CACHE));
		}
		else
		{
			V_DrawPatch(160, 70, W_CacheLumpName("PAUSED",
				PU_CACHE));
		}
	}

#ifdef RENDER3D
    if( OGL_DrawFilter() )
        BorderNeedRefresh = true;
#endif

	// Draw current message
	DrawMessage();

	// Draw Menu
	MN_Drawer();

	// Send out any new accumulation
	NetUpdate();

	// Flush buffered stuff to screen
	I_Update();
}

//==========================================================================
//
// DrawMessage
//
//==========================================================================

static void DrawMessage(void)
{
	player_t *player;

	player = &players[consoleplayer];
	if(player->messageTics <= 0 || !player->message)
	{ // No message
		return;
	}
	if(player->yellowMessage)
	{
		MN_DrTextAYellow(player->message, 
			160-MN_TextAWidth(player->message)/2, 1);
	}
	else
	{
		MN_DrTextA(player->message, 160-MN_TextAWidth(player->message)/2, 1);
	}
}

//==========================================================================
//
// H2_PageTicker
//
//==========================================================================

void H2_PageTicker(void)
{
	if(--pagetic < 0)
	{
		H2_AdvanceDemo();
	}
}

//==========================================================================
//
// PageDrawer
//
//==========================================================================

static void PageDrawer(void)
{
	V_DrawRawScreen(W_CacheLumpName(pagename, PU_CACHE));
	if(demosequence == 1)
	{
		V_DrawPatch(4, 160, W_CacheLumpName("ADVISOR", PU_CACHE));
	}
	UpdateState |= I_FULLSCRN;
}

//==========================================================================
//
// H2_AdvanceDemo
//
// Called after each demo or intro demosequence finishes.
//
//==========================================================================

void H2_AdvanceDemo(void)
{
	advancedemo = true;
}

//==========================================================================
//
// H2_DoAdvanceDemo
//
//==========================================================================

void H2_DoAdvanceDemo(void)
{
	players[consoleplayer].playerstate = PST_LIVE; // don't reborn
	advancedemo = false;
	usergame = false; // can't save/end game here
	paused = false;
	gameaction = ga_nothing;
	demosequence = (demosequence+1)%7;
	switch(demosequence)
	{
		case 0:
			pagetic = 280;
			gamestate = GS_DEMOSCREEN;
			pagename = "TITLE";
			S_StartSongName("hexen", true);
			break;
		case 1:
			pagetic = 210;
			gamestate = GS_DEMOSCREEN;
			pagename = "TITLE";
			break;
		case 2:
			BorderNeedRefresh = true;
			UpdateState |= I_FULLSCRN;
			G_DeferedPlayDemo("demo1");
			break;
		case 3:
			pagetic = 200;
			gamestate = GS_DEMOSCREEN;
			pagename = "CREDIT";
			break;
		case 4:
			BorderNeedRefresh = true;
			UpdateState |= I_FULLSCRN;
			G_DeferedPlayDemo("demo2");
			break;
		case 5:
			pagetic = 200;
			gamestate = GS_DEMOSCREEN;
			pagename = "CREDIT";
			break;
		case 6:
			BorderNeedRefresh = true;
			UpdateState |= I_FULLSCRN;
			G_DeferedPlayDemo("demo3");
			break;
	}
}

//==========================================================================
//
// H2_StartTitle
//
//==========================================================================

void H2_StartTitle(void)
{
	gameaction = ga_nothing;
	demosequence = -1;
	H2_AdvanceDemo();
}

//==========================================================================
//
// CheckRecordFrom
//
// -recordfrom <savegame num> <demoname>
//
//==========================================================================

static void CheckRecordFrom(void)
{
	int p;

	p = M_CheckParm("-recordfrom");
	if(!p || p > myargc-2)
	{ // Bad args
		return;
	}
	G_LoadGame(atoi(myargv[p+1]));
	G_DoLoadGame(); // Load the gameskill etc info from savegame
	G_RecordDemo(gameskill, 1, gameepisode, gamemap, myargv[p+2]);
	H2_GameLoop(); // Never returns
}

//==========================================================================
//
// AddWADFile
//
//==========================================================================

static void AddWADFile(char *file)
{
	int i;
	char *new;

	ST_Message("Adding external file: %s\n", file);
	i = 0;
	while(wadfiles[i])
	{
		i++;
	}
	new = malloc(strlen(file)+1);
	strcpy(new, file);
	wadfiles[i] = new;
}


//==========================================================================
//
// CreateBasePath
//
//==========================================================================

static void CreateBasePath(void)
{
	snprintf(base, sizeof(base), "%s/.hhexen/", getenv("HOME"));
	basePath = base;
	mkdir( base, S_IRWXU|S_IRWXG|S_IRWXO );
}


//==========================================================================
//
// Fixed Point math
//
//==========================================================================

#if defined(__i386__) || defined(__386__) || defined(_M_IX86)
#if defined(__GNUC__) && !defined(_INLINE_FIXED_ASM)
fixed_t	FixedMul (fixed_t a, fixed_t b)
{
	fixed_t retval;
	__asm__ __volatile__(
		"imull  %%edx			\n\t"
		"shrdl  $16, %%edx, %%eax	\n\t"
		: "=a" (retval)
		: "a" (a), "d" (b)
		: "cc"
	);
	return retval;
}

fixed_t	FixedDiv2 (fixed_t a, fixed_t b)
{
	fixed_t retval;
	__asm__ __volatile__(
		"cdq				\n\t"
		"shldl  $16, %%eax, %%edx	\n\t"
		"sall   $16, %%eax		\n\t"
		"idivl  %%ebx			\n\t"
		: "=a" (retval)
		: "a" (a), "b" (b)
		: "%edx", "cc"
	);
	return retval;
}
#endif	/* GCC and !_INLINE_FIXED_ASM */
#endif	/* x86 */


#if !defined(_HAVE_FIXED_ASM)	/* for non-i386 */
fixed_t FixedMul (fixed_t a, fixed_t b)
{
	return ((int64_t) a * (int64_t) b) >> 16;
}

fixed_t FixedDiv2 (fixed_t a, fixed_t b)
{
	if(!b)
		return 0;
	return (fixed_t) (((double) a / (double) b) * FRACUNIT);
}
#endif

fixed_t FixedDiv (fixed_t a, fixed_t b)
{
	if ((abs(a) >> 14) >= abs(b))
	{
		return ((a^b) < 0 ? MININT : MAXINT);
	}
	return (FixedDiv2(a, b));
}

//==========================================================================
//
// Byte swap functions
//
//==========================================================================

short ShortSwap (short x)
{
	return ((x & 0xff) << 8) | ((x & 0xff00) >> 8);
}

int LongSwap (int x)
{
	return (((x & 0xff) << 24) | ((x & 0xff00) << 8) |
		((x & 0xff0000) >> 8) | ((x & 0xff000000) >> 24));
}

