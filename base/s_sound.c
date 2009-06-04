//**************************************************************************
//**
//** S_SOUND.C:  MUSIC & SFX API
//**
//** $Revision$
//** $Date$
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "h2stdinc.h"
#include "h2def.h"
#include "p_local.h"	/* P_AproxDistance() */
#include "sounds.h"
#include "i_sound.h"
#include "i_cdmus.h"
#include "soundst.h"

// MACROS ------------------------------------------------------------------

#define DEFAULT_ARCHIVEPATH	"o:\\sound\\archive\\"
#define PRIORITY_MAX_ADJUST	10
#define DIST_ADJUST	(MAX_SND_DIST/PRIORITY_MAX_ADJUST)

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static boolean S_StopSoundID(int sound_id, int priority);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

extern int	snd_MaxVolume;
extern int	snd_MusicVolume;
extern int	snd_Channels;

#ifdef __WATCOMC__
extern int	snd_SfxDevice;
extern int	snd_MusicDevice;
extern int	snd_DesiredSfxDevice;
extern int	snd_DesiredMusicDevice;
extern int	tsm_ID;
#endif

extern void	**lumpcache;

extern int	startepisode;
extern int	startmap;

// PUBLIC DATA DEFINITIONS -------------------------------------------------

/* Sound info */
sfxinfo_t S_sfx[] =
{
	/* tagname, lumpname, priority, usefulness, snd_ptr, lumpnum, numchannels, */
	/*		pitchshift						   */
	{ "", "", 0, -1, NULL, 0, 0, 0 },
	{ "PlayerFighterNormalDeath", "", 256, -1, NULL, 0, 2, 1 },
	{ "PlayerFighterCrazyDeath", "", 256, -1, NULL, 0, 2, 1 },
	{ "PlayerFighterExtreme1Death", "", 256, -1, NULL, 0, 2, 1 },
	{ "PlayerFighterExtreme2Death", "", 256, -1, NULL, 0, 2, 1 },
	{ "PlayerFighterExtreme3Death", "", 256, -1, NULL, 0, 2, 1 },
	{ "PlayerFighterBurnDeath", "", 256, -1, NULL, 0, 2, 1 },
	{ "PlayerClericNormalDeath", "", 256, -1, NULL, 0, 2, 1 },
	{ "PlayerClericCrazyDeath", "", 256, -1, NULL, 0, 2, 1 },
	{ "PlayerClericExtreme1Death", "", 256, -1, NULL, 0, 2, 1 },
	{ "PlayerClericExtreme2Death", "", 256, -1, NULL, 0, 2, 1 },
	{ "PlayerClericExtreme3Death", "", 256, -1, NULL, 0, 2, 1 },
	{ "PlayerClericBurnDeath", "", 256, -1, NULL, 0, 2, 1 },
	{ "PlayerMageNormalDeath", "", 256, -1, NULL, 0, 2, 0 },
	{ "PlayerMageCrazyDeath", "", 256, -1, NULL, 0, 2, 0 },
	{ "PlayerMageExtreme1Death", "", 256, -1, NULL, 0, 2, 0 },
	{ "PlayerMageExtreme2Death", "", 256, -1, NULL, 0, 2, 0 },
	{ "PlayerMageExtreme3Death", "", 256, -1, NULL, 0, 2, 0 },
	{ "PlayerMageBurnDeath", "", 256, -1, NULL, 0, 2, 0 },
	{ "PlayerFighterPain", "", 256, -1, NULL, 0, 2, 1 },
	{ "PlayerClericPain", "", 256, -1, NULL, 0, 2, 1 },
	{ "PlayerMagePain", "", 256, -1, NULL, 0, 2, 0 },
	{ "PlayerFighterGrunt", "", 256, -1, NULL, 0, 2, 1 },
	{ "PlayerClericGrunt", "", 256, -1, NULL, 0, 2, 1 },
	{ "PlayerMageGrunt", "", 256, -1, NULL, 0, 2, 0 },
	{ "PlayerLand", "", 32, -1, NULL, 0, 2, 1 },
	{ "PlayerPoisonCough", "", 256, -1, NULL, 0, 2, 1 },
	{ "PlayerFighterFallingScream", "", 256, -1, NULL, 0, 2, 1 },
	{ "PlayerClericFallingScream", "", 256, -1, NULL, 0, 2, 1 },
	{ "PlayerMageFallingScream", "", 256, -1, NULL, 0, 2, 0 },
	{ "PlayerFallingSplat", "", 256, -1, NULL, 0, 2, 1 },
	{ "PlayerFighterFailedUse", "", 256, -1, NULL, 0, 1, 1 },
	{ "PlayerClericFailedUse", "", 256, -1, NULL, 0, 1, 1 },
	{ "PlayerMageFailedUse", "", 256, -1, NULL, 0, 1, 0 },
	{ "PlatformStart", "", 36, -1, NULL, 0, 2, 1 },
	{ "PlatformStartMetal", "", 36, -1, NULL, 0, 2, 1 },
	{ "PlatformStop", "", 40, -1, NULL, 0, 2, 1 },
	{ "StoneMove", "", 32, -1, NULL, 0, 2, 1 },
	{ "MetalMove", "", 32, -1, NULL, 0, 2, 1 },
	{ "DoorOpen", "", 36, -1, NULL, 0, 2, 1 },
	{ "DoorLocked", "", 36, -1, NULL, 0, 2, 1 },
	{ "DoorOpenMetal", "", 36, -1, NULL, 0, 2, 1 },
	{ "DoorCloseMetal", "", 36, -1, NULL, 0, 2, 1 },
	{ "DoorCloseLight", "", 36, -1, NULL, 0, 2, 1 },
	{ "DoorCloseHeavy", "", 36, -1, NULL, 0, 2, 1 },
	{ "DoorCreak", "", 36, -1, NULL, 0, 2, 1 },
	{ "PickupWeapon", "", 36, -1, NULL, 0, 2, 0 },
	{ "PickupArtifact", "", 36, -1, NULL, 0, 2, 1 },
	{ "PickupKey", "", 36, -1, NULL, 0, 2, 1 },
	{ "PickupItem", "", 36, -1, NULL, 0, 2, 1 },
	{ "PickupPiece", "", 36, -1, NULL, 0, 2, 0 },
	{ "WeaponBuild", "", 36, -1, NULL, 0, 2, 0 },
	{ "UseArtifact", "", 36, -1, NULL, 0, 2, 1 },
	{ "BlastRadius", "", 36, -1, NULL, 0, 2, 1 },
	{ "Teleport", "", 256, -1, NULL, 0, 2, 1 },
	{ "ThunderCrash", "", 30, -1, NULL, 0, 2, 1 },
	{ "FighterPunchMiss", "", 80, -1, NULL, 0, 2, 1 },
	{ "FighterPunchHitThing", "", 80, -1, NULL, 0, 2, 1 },
	{ "FighterPunchHitWall", "", 80, -1, NULL, 0, 2, 1 },
	{ "FighterGrunt", "", 80, -1, NULL, 0, 2, 1 },
	{ "FighterAxeHitThing", "", 80, -1, NULL, 0, 2, 1 },
	{ "FighterHammerMiss", "", 80, -1, NULL, 0, 2, 1 },
	{ "FighterHammerHitThing", "", 80, -1, NULL, 0, 2, 1 },
	{ "FighterHammerHitWall", "", 80, -1, NULL, 0, 2, 1 },
	{ "FighterHammerContinuous", "", 32, -1, NULL, 0, 2, 1 },
	{ "FighterHammerExplode", "", 80, -1, NULL, 0, 2, 1 },
	{ "FighterSwordFire", "", 80, -1, NULL, 0, 2, 1 },
	{ "FighterSwordExplode", "", 80, -1, NULL, 0, 2, 1 },
	{ "ClericCStaffFire", "", 80, -1, NULL, 0, 2, 1 },
	{ "ClericCStaffExplode", "", 40, -1, NULL, 0, 2, 1 },
	{ "ClericCStaffHitThing", "", 80, -1, NULL, 0, 2, 1 },
	{ "ClericFlameFire", "", 80, -1, NULL, 0, 2, 1 },
	{ "ClericFlameExplode", "", 80, -1, NULL, 0, 2, 1 },
	{ "ClericFlameCircle", "", 80, -1, NULL, 0, 2, 1 },
	{ "MageWandFire", "", 80, -1, NULL, 0, 2, 1 },
	{ "MageLightningFire", "", 80, -1, NULL, 0, 2, 1 },
	{ "MageLightningZap", "", 32, -1, NULL, 0, 2, 1 },
	{ "MageLightningContinuous", "", 32, -1, NULL, 0, 2, 1 },
	{ "MageLightningReady", "", 30, -1, NULL, 0, 2, 1 },
	{ "MageShardsFire","", 80, -1, NULL, 0, 2, 1 },
	{ "MageShardsExplode","", 36, -1, NULL, 0, 2, 1 },
	{ "MageStaffFire","", 80, -1, NULL, 0, 2, 1 },
	{ "MageStaffExplode","", 40, -1, NULL, 0, 2, 1 },
	{ "Switch1", "", 32, -1, NULL, 0, 2, 1 },
	{ "Switch2", "", 32, -1, NULL, 0, 2, 1 },
	{ "SerpentSight", "", 32, -1, NULL, 0, 2, 1 },
	{ "SerpentActive", "", 32, -1, NULL, 0, 2, 1 },
	{ "SerpentPain", "", 32, -1, NULL, 0, 2, 1 },
	{ "SerpentAttack", "", 32, -1, NULL, 0, 2, 1 },
	{ "SerpentMeleeHit", "", 32, -1, NULL, 0, 2, 1 },
	{ "SerpentDeath", "", 40, -1, NULL, 0, 2, 1 },
	{ "SerpentBirth", "", 32, -1, NULL, 0, 2, 1 },
	{ "SerpentFXContinuous", "", 32, -1, NULL, 0, 2, 1 },
	{ "SerpentFXHit", "", 32, -1, NULL, 0, 2, 1 },
	{ "PotteryExplode", "", 32, -1, NULL, 0, 2, 1 },
	{ "Drip", "", 32, -1, NULL, 0, 2, 1 },
	{ "CentaurSight", "", 32, -1, NULL, 0, 2, 1 },
	{ "CentaurActive", "", 32, -1, NULL, 0, 2, 1 },
	{ "CentaurPain", "", 32, -1, NULL, 0, 2, 1 },
	{ "CentaurAttack", "", 32, -1, NULL, 0, 2, 1 },
	{ "CentaurDeath", "", 40, -1, NULL, 0, 2, 1 },
	{ "CentaurLeaderAttack", "", 32, -1, NULL, 0, 2, 1 },
	{ "CentaurMissileExplode", "", 32, -1, NULL, 0, 2, 1 },
	{ "Wind", "", 1, -1, NULL, 0, 2, 1 },
	{ "BishopSight", "", 32, -1, NULL, 0, 2, 1 },
	{ "BishopActive", "", 32, -1, NULL, 0, 2, 1 },
	{ "BishopPain", "", 32, -1, NULL, 0, 2, 1 },
	{ "BishopAttack", "", 32, -1, NULL, 0, 2, 1 },
	{ "BishopDeath", "", 40, -1, NULL, 0, 2, 1 },
	{ "BishopMissileExplode", "", 32, -1, NULL, 0, 2, 1 },
	{ "BishopBlur", "", 32, -1, NULL, 0, 2, 1 },
	{ "DemonSight", "", 32, -1, NULL, 0, 2, 1 },
	{ "DemonActive", "", 32, -1, NULL, 0, 2, 1 },
	{ "DemonPain", "", 32, -1, NULL, 0, 2, 1 },
	{ "DemonAttack", "", 32, -1, NULL, 0, 2, 1 },
	{ "DemonMissileFire", "", 32, -1, NULL, 0, 2, 1 },
	{ "DemonMissileExplode", "", 32, -1, NULL, 0, 2, 1 },
	{ "DemonDeath", "", 40, -1, NULL, 0, 2, 1 },
	{ "WraithSight", "", 32, -1, NULL, 0, 2, 1 },
	{ "WraithActive", "", 32, -1, NULL, 0, 2, 1 },
	{ "WraithPain", "", 32, -1, NULL, 0, 2, 1 },
	{ "WraithAttack", "", 32, -1, NULL, 0, 2, 1 },
	{ "WraithMissileFire", "", 32, -1, NULL, 0, 2, 1 },
	{ "WraithMissileExplode", "", 32, -1, NULL, 0, 2, 1 },
	{ "WraithDeath", "", 40, -1, NULL, 0, 2, 1 },
	{ "PigActive1", "", 32, -1, NULL, 0, 2, 1 },
	{ "PigActive2", "", 32, -1, NULL, 0, 2, 1 },
	{ "PigPain", "", 32, -1, NULL, 0, 2, 1 },
	{ "PigAttack", "", 32, -1, NULL, 0, 2, 1 },
	{ "PigDeath", "", 40, -1, NULL, 0, 2, 1 },
	{ "MaulatorSight", "", 32, -1, NULL, 0, 2, 1 },
	{ "MaulatorActive", "", 32, -1, NULL, 0, 2, 1 },
	{ "MaulatorPain", "", 32, -1, NULL, 0, 2, 1 },
	{ "MaulatorHamSwing", "", 32, -1, NULL, 0, 2, 1 },
	{ "MaulatorHamHit", "", 32, -1, NULL, 0, 2, 1 },
	{ "MaulatorMissileHit", "", 32, -1, NULL, 0, 2, 1 },
	{ "MaulatorDeath", "", 40, -1, NULL, 0, 2, 1 },
	{ "FreezeDeath", "", 40, -1, NULL, 0, 2, 1 },
	{ "FreezeShatter", "", 40, -1, NULL, 0, 2, 1 },
	{ "EttinSight", "", 32, -1, NULL, 0, 2, 1 },
	{ "EttinActive", "", 32, -1, NULL, 0, 2, 1 },
	{ "EttinPain", "", 32, -1, NULL, 0, 2, 1 },
	{ "EttinAttack", "", 32, -1, NULL, 0, 2, 1 },
	{ "EttinDeath", "", 40, -1, NULL, 0, 2, 1 },
	{ "FireDemonSpawn", "", 32, -1, NULL, 0, 2, 1 },
	{ "FireDemonActive", "", 32, -1, NULL, 0, 2, 1 },
	{ "FireDemonPain", "", 32, -1, NULL, 0, 2, 1 },
	{ "FireDemonAttack", "", 32, -1, NULL, 0, 2, 1 },
	{ "FireDemonMissileHit", "", 32, -1, NULL, 0, 2, 1 },
	{ "FireDemonDeath", "", 40, -1, NULL, 0, 2, 1 },
	{ "IceGuySight", "", 32, -1, NULL, 0, 2, 1 },
	{ "IceGuyActive", "", 32, -1, NULL, 0, 2, 1 },
	{ "IceGuyAttack", "", 32, -1, NULL, 0, 2, 1 },
	{ "IceGuyMissileExplode", "", 32, -1, NULL, 0, 2, 1 },
	{ "SorcererSight", "", 256, -1, NULL, 0, 2, 1 },
	{ "SorcererActive", "", 256, -1, NULL, 0, 2, 1 },
	{ "SorcererPain", "", 256, -1, NULL, 0, 2, 1 },
	{ "SorcererSpellCast", "", 256, -1, NULL, 0, 2, 1 },
	{ "SorcererBallWoosh", "", 256, -1, NULL, 0, 4, 1 },
	{ "SorcererDeathScream", "", 256, -1, NULL, 0, 2, 1 },
	{ "SorcererBishopSpawn", "", 80, -1, NULL, 0, 2, 1 },
	{ "SorcererBallPop", "", 80, -1, NULL, 0, 2, 1 },
	{ "SorcererBallBounce", "", 80, -1, NULL, 0, 3, 1 },
	{ "SorcererBallExplode", "", 80, -1, NULL, 0, 3, 1 },
	{ "SorcererBigBallExplode", "", 80, -1, NULL, 0, 3, 1 },
	{ "SorcererHeadScream", "", 256, -1, NULL, 0, 2, 1 },
	{ "DragonSight", "", 64, -1, NULL, 0, 2, 1 },
	{ "DragonActive", "", 64, -1, NULL, 0, 2, 1 },
	{ "DragonWingflap", "", 64, -1, NULL, 0, 2, 1 },
	{ "DragonAttack", "", 64, -1, NULL, 0, 2, 1 },
	{ "DragonPain", "", 64, -1, NULL, 0, 2, 1 },
	{ "DragonDeath", "", 64, -1, NULL, 0, 2, 1 },
	{ "DragonFireballExplode", "", 32, -1, NULL, 0, 2, 1 },
	{ "KoraxSight", "", 256, -1, NULL, 0, 2, 1 },
	{ "KoraxActive", "", 256, -1, NULL, 0, 2, 1 },
	{ "KoraxPain", "", 256, -1, NULL, 0, 2, 1 },
	{ "KoraxAttack", "", 256, -1, NULL, 0, 2, 1 },
	{ "KoraxCommand", "", 256, -1, NULL, 0, 2, 1 },
	{ "KoraxDeath", "", 256, -1, NULL, 0, 2, 1 },
	{ "KoraxStep", "", 128, -1, NULL, 0, 2, 1 },
	{ "ThrustSpikeRaise", "", 32, -1, NULL, 0, 2, 1 },
	{ "ThrustSpikeLower", "", 32, -1, NULL, 0, 2, 1 },
	{ "GlassShatter", "", 32, -1, NULL, 0, 2, 1 },
	{ "FlechetteBounce", "", 32, -1, NULL, 0, 2, 1 },
	{ "FlechetteExplode", "", 32, -1, NULL, 0, 2, 1 },
	{ "LavaMove", "", 36, -1, NULL, 0, 2, 1 },
	{ "WaterMove", "", 36, -1, NULL, 0, 2, 1 },
	{ "IceStartMove", "", 36, -1, NULL, 0, 2, 1 },
	{ "EarthStartMove", "", 36, -1, NULL, 0, 2, 1 },
	{ "WaterSplash", "", 32, -1, NULL, 0, 2, 1 },
	{ "LavaSizzle", "", 32, -1, NULL, 0, 2, 1 },
	{ "SludgeGloop", "", 32, -1, NULL, 0, 2, 1 },
	{ "HolySymbolFire", "", 64, -1, NULL, 0, 2, 1 },
	{ "SpiritActive", "", 32, -1, NULL, 0, 2, 1 },
	{ "SpiritAttack", "", 32, -1, NULL, 0, 2, 1 },
	{ "SpiritDie", "", 32, -1, NULL, 0, 2, 1 },
	{ "ValveTurn", "", 36, -1, NULL, 0, 2, 1 },
	{ "RopePull", "", 36, -1, NULL, 0, 2, 1 },
	{ "FlyBuzz", "", 20, -1, NULL, 0, 2, 1 },
	{ "Ignite", "", 32, -1, NULL, 0, 2, 1 },
	{ "PuzzleSuccess", "", 256, -1, NULL, 0, 2, 1 },
	{ "PuzzleFailFighter", "", 256, -1, NULL, 0, 2, 1 },
	{ "PuzzleFailCleric", "", 256, -1, NULL, 0, 2, 1 },
	{ "PuzzleFailMage", "", 256, -1, NULL, 0, 2, 1 },
	{ "Earthquake", "", 32, -1, NULL, 0, 2, 1 },
	{ "BellRing", "", 32, -1, NULL, 0, 2, 0 },
	{ "TreeBreak", "", 32, -1, NULL, 0, 2, 1 },
	{ "TreeExplode", "", 32, -1, NULL, 0, 2, 1 },
	{ "SuitofArmorBreak", "", 32, -1, NULL, 0, 2, 1 },
	{ "PoisonShroomPain", "", 20, -1, NULL, 0, 2, 1 },
	{ "PoisonShroomDeath", "", 32, -1, NULL, 0, 2, 1 },
	{ "Ambient1", "", 1, -1, NULL, 0, 1, 1 },
	{ "Ambient2", "", 1, -1, NULL, 0, 1, 1 },
	{ "Ambient3", "", 1, -1, NULL, 0, 1, 1 },
	{ "Ambient4", "", 1, -1, NULL, 0, 1, 1 },
	{ "Ambient5", "", 1, -1, NULL, 0, 1, 1 },
	{ "Ambient6", "", 1, -1, NULL, 0, 1, 1 },
	{ "Ambient7", "", 1, -1, NULL, 0, 1, 1 },
	{ "Ambient8", "", 1, -1, NULL, 0, 1, 1 },
	{ "Ambient9", "", 1, -1, NULL, 0, 1, 1 },
	{ "Ambient10", "", 1, -1, NULL, 0, 1, 1 },
	{ "Ambient11", "", 1, -1, NULL, 0, 1, 1 },
	{ "Ambient12", "", 1, -1, NULL, 0, 1, 1 },
	{ "Ambient13", "", 1, -1, NULL, 0, 1, 1 },
	{ "Ambient14", "", 1, -1, NULL, 0, 1, 1 },
	{ "Ambient15", "", 1, -1, NULL, 0, 1, 1 },
	{ "StartupTick", "", 32, -1, NULL, 0, 2, 1 },
	{ "SwitchOtherLevel", "", 32, -1, NULL, 0, 2, 1 },
	{ "Respawn", "", 32, -1, NULL, 0, 2, 1 },
	{ "KoraxVoiceGreetings", "", 512, -1, NULL, 0, 2, 1 },
	{ "KoraxVoiceReady", "", 512, -1, NULL, 0, 2, 1 },
	{ "KoraxVoiceBlood", "", 512, -1, NULL, 0, 2, 1 },
	{ "KoraxVoiceGame", "", 512, -1, NULL, 0, 2, 1 },
	{ "KoraxVoiceBoard", "", 512, -1, NULL, 0, 2, 1 },
	{ "KoraxVoiceWorship", "", 512, -1, NULL, 0, 2, 1 },
	{ "KoraxVoiceMaybe", "", 512, -1, NULL, 0, 2, 1 },
	{ "KoraxVoiceStrong", "", 512, -1, NULL, 0, 2, 1 },
	{ "KoraxVoiceFace", "", 512, -1, NULL, 0, 2, 1 },
	{ "BatScream", "", 32, -1, NULL, 0, 2, 1 },
	{ "Chat", "", 512, -1, NULL, 0, 2, 1 },
	{ "MenuMove", "", 32, -1, NULL, 0, 2, 1 },
	{ "ClockTick", "", 32, -1, NULL, 0, 2, 1 },
	{ "Fireball", "", 32, -1, NULL, 0, 2, 1 },
	{ "PuppyBeat", "", 30, -1, NULL, 0, 2, 1 },
	{ "MysticIncant", "", 32, -1, NULL, 0, 4, 1 }
};

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static channel_t Channel[MAX_CHANNELS];
static int	RegisteredSong;	/* the current registered song. */
static int	isExternalSong;
static int	NextCleanup;
static boolean	MusicPaused;
static int	Mus_Song = -1;
static int	Mus_LumpNum;
static void	*Mus_SndPtr;
static byte	*SoundCurve;

static boolean	UseSndScript;
static char	ArchivePath[MAX_OSPATH];

// CODE --------------------------------------------------------------------

//==========================================================================
//
// S_Init
//
//==========================================================================

void S_Init(void)
{
	SoundCurve = (byte *) W_CacheLumpName("SNDCURVE", PU_STATIC);
//	SoundCurve = (byte *) Z_Malloc(MAX_SND_DIST, PU_STATIC, NULL);
	I_StartupSound();
	if (snd_Channels > 8)
	{
		snd_Channels = 8;
	}
	I_SetChannels(snd_Channels);
	I_SetMusicVolume(snd_MusicVolume);

	// Attempt to setup CD music
	ST_Message("Initializing CD Audio: ");
	i_CDMusic = (I_CDMusInit() != -1);
	if (i_CDMusic)
	{
		ST_Message("success.\n");
	}
	else
	{
		ST_Message("failed.\n");
	}
}

//==========================================================================
//
// S_InitScript
//
//==========================================================================

void S_InitScript(void)
{
	int p;
	int i;

	strcpy(ArchivePath, DEFAULT_ARCHIVEPATH);
	p = M_CheckParm("-devsnd");
	if (p && p < myargc - 1)
	{
		UseSndScript = true;
		SC_OpenFile(myargv[p+1]);
	}
	else
	{
		UseSndScript = false;
		SC_OpenLump("sndinfo");
	}
	while (SC_GetString())
	{
		if (*sc_String == '$')
		{
			if (!strcasecmp(sc_String, "$ARCHIVEPATH"))
			{
				SC_MustGetString();
				strcpy(ArchivePath, sc_String);
			}
			else if (!strcasecmp(sc_String, "$MAP"))
			{
				SC_MustGetNumber();
				SC_MustGetString();
				if (sc_Number)
				{
					P_PutMapSongLump(sc_Number, sc_String);
				}
			}
			continue;
		}
		else
		{
			for (i = 0; i < NUMSFX; i++)
			{
				if (!strcmp(S_sfx[i].tagName, sc_String))
				{
					SC_MustGetString();
					if (*sc_String != '?')
					{
						strcpy(S_sfx[i].lumpname, sc_String);
					}
					else
					{
						strcpy(S_sfx[i].lumpname, "default");
					}
					break;
				}
			}
			if (i == NUMSFX)
			{
				SC_MustGetString();
			}
		}
	}
	SC_Close();

	for (i = 0; i < NUMSFX; i++)
	{
		if (!strcmp(S_sfx[i].lumpname, ""))
		{
			strcpy(S_sfx[i].lumpname, "default");
		}
	}
}

//==========================================================================
//
// S_ShutDown
//
//==========================================================================

void S_ShutDown(void)
{
	if (i_CDMusic)
	{
		I_CDMusStop();
	}
	I_CDMusShutdown();
#ifdef __WATCOMC__
	if (tsm_ID == -1)
		return;
#endif
	if (RegisteredSong)
	{
		I_StopSong(RegisteredSong);
		I_UnRegisterSong(RegisteredSong);
	}
	I_ShutdownSound();
}

//==========================================================================
//
// S_Start
//
//==========================================================================

void S_Start(void)
{
	S_StopAllSound();
	S_StartSong(gamemap, true);
}

//==========================================================================
//
// S_StartSong
//
//==========================================================================

void S_StartSong(int song, boolean loop)
{
	char *songLump;
	int track;

	if (i_CDMusic && cdaudio)
	{ // Play a CD track, instead
		if (i_CDTrack)
		{ // Default to the player-chosen track
			track = i_CDTrack;
		}
		else
		{
			track = P_GetMapCDTrack(gamemap);
		}
		if (track == i_CDCurrentTrack && i_CDMusicLength > 0)
		{
			return;
		}
		if (!I_CDMusPlay(track))
		{
			/*
			if (loop)
			{
			//	i_CDMusicLength = 35*I_CDMusTrackLength(track);
				oldTic = gametic;
			}
			else
			{
				i_CDMusicLength = -1;
			}
			*/
			i_CDCurrentTrack = track;
		}
	}
	else
	{
		if (song == Mus_Song)
		{ // don't replay an old song
			return;
		}
		if (RegisteredSong)
		{
			I_StopSong(RegisteredSong);
			I_UnRegisterSong(RegisteredSong);
			if (!isExternalSong)
			{
				if (UseSndScript)
				{
					Z_Free(Mus_SndPtr);
				}
				else
				{
					Z_ChangeTag(lumpcache[Mus_LumpNum], PU_CACHE);
				}
#ifdef __WATCOMC__
				_dpmi_unlockregion(Mus_SndPtr, lumpinfo[Mus_LumpNum].size);
#endif
			}
			RegisteredSong = 0;
		}
		songLump = P_GetMapSongLump(song);
		if (!songLump)
		{
			return;
		}
		isExternalSong = I_RegisterExternalSong(songLump);
		if (isExternalSong)
		{
			RegisteredSong = isExternalSong;
			I_PlaySong(RegisteredSong, loop);
			Mus_Song = song;
			return;
		}
		if (UseSndScript)
		{
			char name[MAX_OSPATH];
			snprintf(name, sizeof(name), "%s%s.lmp", ArchivePath, songLump);
			M_ReadFile(name, &Mus_SndPtr);
		}
		else
		{
			Mus_LumpNum = W_GetNumForName(songLump);
			Mus_SndPtr = W_CacheLumpNum(Mus_LumpNum, PU_MUSIC);
		}
#ifdef __WATCOMC__
		_dpmi_lockregion(Mus_SndPtr, lumpinfo[Mus_LumpNum].size);
#endif
		RegisteredSong = I_RegisterSong(Mus_SndPtr);
		I_PlaySong(RegisteredSong, loop);	// 'true' denotes endless looping.
		Mus_Song = song;
	}
}

//==========================================================================
//
// S_StartSongName
//
//==========================================================================

void S_StartSongName(const char *songLump, boolean loop)
{
	int cdTrack;

	if (!songLump)
	{
		return;
	}
	if (i_CDMusic && cdaudio)
	{
		cdTrack = 0;

		if (!strcmp(songLump, "hexen"))
		{
			cdTrack = P_GetCDTitleTrack();
		}
		else if (!strcmp(songLump, "hub"))
		{
			cdTrack = P_GetCDIntermissionTrack();
		}
		else if (!strcmp(songLump, "hall"))
		{
			cdTrack = P_GetCDEnd1Track();
		}
		else if (!strcmp(songLump, "orb"))
		{
			cdTrack = P_GetCDEnd2Track();
		}
		else if (!strcmp(songLump, "chess") && !i_CDTrack)
		{
			cdTrack = P_GetCDEnd3Track();
		}
/*	Uncomment this, if Kevin writes a specific song for startup
		else if (!strcmp(songLump, "start"))
		{
			cdTrack = P_GetCDStartTrack();
		}
*/
		if (!cdTrack || (cdTrack == i_CDCurrentTrack && i_CDMusicLength > 0))
		{
			return;
		}
		if (!I_CDMusPlay(cdTrack))
		{
			/*
			if (loop)
			{
				i_CDMusicLength = 35*I_CDMusTrackLength(cdTrack);
				oldTic = gametic;
			}
			else
			{
				i_CDMusicLength = -1;
			}
			*/
			i_CDCurrentTrack = cdTrack;
			i_CDTrack = false;
		}
	}
	else
	{
		if (RegisteredSong)
		{
			I_StopSong(RegisteredSong);
			I_UnRegisterSong(RegisteredSong);
			if (!isExternalSong)
			{
				if (UseSndScript)
				{
					Z_Free(Mus_SndPtr);
				}
				else
				{
					Z_ChangeTag(lumpcache[Mus_LumpNum], PU_CACHE);
				}
			}
#ifdef __WATCOMC__
			_dpmi_unlockregion(Mus_SndPtr, lumpinfo[Mus_LumpNum].size);
#endif
			RegisteredSong = 0;
		}
		isExternalSong = I_RegisterExternalSong(songLump);
		if (isExternalSong)
		{
			RegisteredSong = isExternalSong;
			I_PlaySong(RegisteredSong, loop);
			Mus_Song = -1;
			return;
		}
		if (UseSndScript)
		{
			char name[MAX_OSPATH];
			snprintf(name, sizeof(name), "%s%s.lmp", ArchivePath, songLump);
			M_ReadFile(name, &Mus_SndPtr);
		}
		else
		{
			Mus_LumpNum = W_GetNumForName(songLump);
			Mus_SndPtr = W_CacheLumpNum(Mus_LumpNum, PU_MUSIC);
		}
#ifdef __WATCOMC__
		_dpmi_lockregion(Mus_SndPtr, lumpinfo[Mus_LumpNum].size);
#endif
		RegisteredSong = I_RegisterSong(Mus_SndPtr);
		I_PlaySong(RegisteredSong, loop);	// 'true' denotes endless looping.
		Mus_Song = -1;
	}
}

//==========================================================================
//
// S_GetSoundID
//
//==========================================================================

int S_GetSoundID(const char *name)
{
	int i;

	for (i = 0; i < NUMSFX; i++)
	{
		if (!strcmp(S_sfx[i].tagName, name))
		{
			return i;
		}
	}
	return 0;
}

//==========================================================================
//
// S_StartSound
//
//==========================================================================

void S_StartSound(mobj_t *origin, int sound_id)
{
	S_StartSoundAtVolume(origin, sound_id, 127);
}

//==========================================================================
//
// S_StartSoundAtVolume
//
//==========================================================================

void S_StartSoundAtVolume(mobj_t *origin, int sound_id, int volume)
{
	static int sndcount = 0;

	int i;
	int dist, vol, chan;
	int priority;
	int angle, sep;
	int absx, absy;

	if (sound_id == 0 || snd_MaxVolume == 0)
		return;
#if 0
	if (origin == NULL)
	{
		origin = players[displayplayer].mo;
	// this can be uninitialized when we are newly
	// started before the demos start playing !...
	}
#endif
	if (volume == 0)
	{
		return;
	}

	// calculate the distance before other stuff so that we can throw out
	// sounds that are beyond the hearing range.
	if (origin)
	{
		absx = abs(origin->x - players[displayplayer].mo->x);
		absy = abs(origin->y - players[displayplayer].mo->y);
	}
	else
	{
		absx = absy = 0;
	}
	dist = absx + absy - (absx > absy ? absy>>1 : absx>>1);
	dist >>= FRACBITS;
	if (dist >= MAX_SND_DIST)
	{
		return;	// sound is beyond the hearing range...
	}
	if (dist < 0)
	{
		dist = 0;
	}
	priority = S_sfx[sound_id].priority;
	priority *= (PRIORITY_MAX_ADJUST - (dist / DIST_ADJUST));
	if (!S_StopSoundID(sound_id, priority))
	{
		return;	// other sounds have greater priority
	}
	for (i = 0; i < snd_Channels; i++)
	{
		if (!origin || origin->player)
		{
			i = snd_Channels;
			break;	// let the player have more than one sound.
		}
		if (origin == Channel[i].mo)
		{ // only allow other mobjs one sound
			S_StopSound(Channel[i].mo);
			break;
		}
	}
	if (i >= snd_Channels)
	{
		for (i = 0; i < snd_Channels; i++)
		{
			if (Channel[i].mo == NULL)
			{
				break;
			}
		}
		if (i >= snd_Channels)
		{
			// look for a lower priority sound to replace.
			sndcount++;
			if (sndcount >= snd_Channels)
			{
				sndcount = 0;
			}
			for (chan = 0; chan < snd_Channels; chan++)
			{
				i = (sndcount + chan) % snd_Channels;
				if (priority >= Channel[i].priority)
				{
					chan = -1;	// denote that sound should be replaced.
					break;
				}
			}
			if (chan != -1)
			{
				return;	// no free channels.
			}
			else	// replace the lower priority sound.
			{
				if (Channel[i].handle)
				{
					if (I_SoundIsPlaying(Channel[i].handle))
					{
						I_StopSound(Channel[i].handle);
					}
					if (S_sfx[Channel[i].sound_id].usefulness > 0)
					{
						S_sfx[Channel[i].sound_id].usefulness--;
					}
				}
			}
		}
	}
	if (S_sfx[sound_id].lumpnum == 0)
	{
		S_sfx[sound_id].lumpnum = I_GetSfxLumpNum(&S_sfx[sound_id]);
	}
	if (S_sfx[sound_id].snd_ptr == NULL)
	{
		if (UseSndScript)
		{
			char name[MAX_OSPATH];
			int len;
			snprintf(name, sizeof(name), "%s%s.lmp",
				 ArchivePath, S_sfx[sound_id].lumpname);
			len = M_ReadFile(name, &S_sfx[sound_id].snd_ptr);
			if (len <= 8)
			{
				I_Error("broken sound lump #%d (%s)\n",
					S_sfx[sound_id].lumpnum, name);
			}
		}
		else
		{
			if (W_LumpLength(S_sfx[sound_id].lumpnum) <= 8)
			{
			//	I_Error("broken sound lump #%d (%s)\n",
				fprintf(stderr, "broken sound lump #%d (%s)\n",
						S_sfx[sound_id].lumpnum,
						S_sfx[sound_id].lumpname);
				return;
			}
			S_sfx[sound_id].snd_ptr =
				W_CacheLumpNum(S_sfx[sound_id].lumpnum, PU_SOUND);
		}
#ifdef __WATCOMC__
		_dpmi_lockregion(S_sfx[sound_id].snd_ptr,
				 lumpinfo[S_sfx[sound_id].lumpnum].size);
#endif
	}

	vol = (SoundCurve[dist] * (snd_MaxVolume * 8) * volume)>>14;
	if (!origin || origin == players[displayplayer].mo)
	{
		sep = 128;
	//	vol = (volume*(snd_MaxVolume+1)*8)>>7;
	}
	else
	{
		angle = R_PointToAngle2(players[displayplayer].mo->x,
					players[displayplayer].mo->y,
					origin->x, origin->y);
		angle = (angle - viewangle)>>24;
		sep = angle*2 - 128;
		if (sep < 64)
			sep = -sep;
		if (sep > 192)
			sep = 512-sep;
	//	vol = SoundCurve[dist];
	}

	if (S_sfx[sound_id].changePitch)
	{
		Channel[i].pitch = (byte)(127 + (M_Random() & 7) - (M_Random() & 7));
	}
	else
	{
		Channel[i].pitch = 127;
	}
	Channel[i].handle = I_StartSound(sound_id, S_sfx[sound_id].snd_ptr, vol,
					 sep, Channel[i].pitch, 0);
	Channel[i].mo = origin;
	Channel[i].sound_id = sound_id;
	Channel[i].priority = priority;
	Channel[i].volume = volume;
	if (S_sfx[sound_id].usefulness < 0)
	{
		S_sfx[sound_id].usefulness = 1;
	}
	else
	{
		S_sfx[sound_id].usefulness++;
	}
}

//==========================================================================
//
// S_StopSoundID
//
//==========================================================================

static boolean S_StopSoundID(int sound_id, int priority)
{
	int i;
	int lp; //least priority
	int found;

	if (S_sfx[sound_id].numchannels == -1)
	{
		return true;
	}
	lp = -1; //denote the argument sound_id
	found = 0;
	for (i = 0; i < snd_Channels; i++)
	{
		if (Channel[i].sound_id == sound_id && Channel[i].mo)
		{
			found++; //found one.  Now, should we replace it??
			if (priority >= Channel[i].priority)
			{ // if we're gonna kill one, then this'll be it
				lp = i;
				priority = Channel[i].priority;
			}
		}
	}
	if (found < S_sfx[sound_id].numchannels)
	{
		return true;
	}
	else if (lp == -1)
	{
		return false;	// don't replace any sounds
	}
	if (Channel[lp].handle)
	{
		if (I_SoundIsPlaying(Channel[lp].handle))
		{
			I_StopSound(Channel[lp].handle);
		}
		if (S_sfx[Channel[lp].sound_id].usefulness > 0)
		{
			S_sfx[Channel[lp].sound_id].usefulness--;
		}
		Channel[lp].mo = NULL;
	}
	return true;
}

//==========================================================================
//
// S_StopSound
//
//==========================================================================

void S_StopSound(mobj_t *origin)
{
	int i;

	for (i = 0; i < snd_Channels; i++)
	{
		if (Channel[i].mo == origin)
		{
			I_StopSound(Channel[i].handle);
			if (S_sfx[Channel[i].sound_id].usefulness > 0)
			{
				S_sfx[Channel[i].sound_id].usefulness--;
			}
			Channel[i].handle = 0;
			Channel[i].mo = NULL;
		}
	}
}

//==========================================================================
//
// S_StopAllSound
//
//==========================================================================

void S_StopAllSound(void)
{
	int i;

	//stop all sounds
	for (i = 0; i < snd_Channels; i++)
	{
		if (Channel[i].handle)
		{
			S_StopSound(Channel[i].mo);
		}
	}
	memset(Channel, 0, 8*sizeof(channel_t));
}

//==========================================================================
//
// S_SoundLink
//
//==========================================================================

void S_SoundLink(mobj_t *oldactor, mobj_t *newactor)
{
	int i;

	for (i = 0; i < snd_Channels; i++)
	{
		if (Channel[i].mo == oldactor)
			Channel[i].mo = newactor;
	}
}

//==========================================================================
//
// S_PauseSound
//
//==========================================================================

void S_PauseSound(void)
{
	I_CDMusStop();
	I_PauseSong(RegisteredSong);
}

//==========================================================================
//
// S_ResumeSound
//
//==========================================================================

void S_ResumeSound(void)
{
	if (i_CDMusic && cdaudio)
	{
		I_CDMusResume();
	}
	else
	{
		I_ResumeSong(RegisteredSong);
	}
}

//==========================================================================
//
// S_UpdateSounds
//
//==========================================================================

void S_UpdateSounds(mobj_t *listener)
{
	int i, dist, vol;
	int angle, sep;
	int priority;
	int absx, absy;

	if (i_CDMusic)
	{
		I_CDMusUpdate();
	}
	if (snd_MaxVolume == 0)
	{
		return;
	}

	// Update any Sequences
	SN_UpdateActiveSequences();

	if (NextCleanup < gametic)
	{
		if (UseSndScript)
		{
			for (i = 0; i < NUMSFX; i++)
			{
				if (S_sfx[i].usefulness == 0 && S_sfx[i].snd_ptr)
				{
					S_sfx[i].usefulness = -1;
				}
			}
		}
		else
		{
			for (i = 0; i < NUMSFX; i++)
			{
				if (S_sfx[i].usefulness == 0 && S_sfx[i].snd_ptr)
				{
					if (lumpcache[S_sfx[i].lumpnum])
					{
						if (((memblock_t *) ((byte*)(lumpcache[S_sfx[i].lumpnum]) -
									sizeof(memblock_t)))->id == ZONEID)
						{ // taken directly from the Z_ChangeTag macro
							Z_ChangeTag2(lumpcache[S_sfx[i].lumpnum], PU_CACHE);
#ifdef __WATCOMC__
							_dpmi_unlockregion(S_sfx[i].snd_ptr,
									   lumpinfo[S_sfx[i].lumpnum].size);
#endif
						}
					}
					S_sfx[i].usefulness = -1;
					S_sfx[i].snd_ptr = NULL;
				}
			}
		}
		NextCleanup = gametic + 35*30;	// every 30 seconds
	}
	for (i = 0; i < snd_Channels; i++)
	{
		if (!Channel[i].handle || S_sfx[Channel[i].sound_id].usefulness == -1)
		{
			continue;
		}
		if (!I_SoundIsPlaying(Channel[i].handle))
		{
			if (S_sfx[Channel[i].sound_id].usefulness > 0)
			{
				S_sfx[Channel[i].sound_id].usefulness--;
			}
			Channel[i].handle = 0;
			Channel[i].mo = NULL;
			Channel[i].sound_id = 0;
		}
		if (Channel[i].mo == NULL || Channel[i].sound_id == 0
			|| Channel[i].mo == listener)
		{
			continue;
		}
		else
		{
			absx = abs(Channel[i].mo->x - listener->x);
			absy = abs(Channel[i].mo->y - listener->y);
			dist = absx+absy-(absx > absy ? absy>>1 : absx>>1);
			dist >>= FRACBITS;

			if (dist >= MAX_SND_DIST)
			{
				S_StopSound(Channel[i].mo);
				continue;
			}
			if (dist < 0)
			{
				dist = 0;
			}
			//vol = SoundCurve[dist];
			vol = (SoundCurve[dist] * (snd_MaxVolume * 8) * Channel[i].volume)>>14;
			if (Channel[i].mo == listener)
			{
				sep = 128;
			}
			else
			{
				angle = R_PointToAngle2(listener->x, listener->y,
							Channel[i].mo->x, Channel[i].mo->y);
				angle = (angle-viewangle)>>24;
				sep = angle*2-128;
				if (sep < 64)
					sep = -sep;
				if (sep > 192)
					sep = 512-sep;
			}
			I_UpdateSoundParams(Channel[i].handle, vol, sep, Channel[i].pitch);
			priority = S_sfx[Channel[i].sound_id].priority;
			priority *= PRIORITY_MAX_ADJUST - (dist / DIST_ADJUST);
			Channel[i].priority = priority;
		}
	}
}

//==========================================================================
//
// S_GetChannelInfo
//
//==========================================================================

void S_GetChannelInfo(SoundInfo_t *s)
{
	int i;
	ChanInfo_t *c;

	s->channelCount = snd_Channels;
	s->musicVolume = snd_MusicVolume;
	s->soundVolume = snd_MaxVolume;
	for (i = 0; i < snd_Channels; i++)
	{
		c = &s->chan[i];
		c->id = Channel[i].sound_id;
		c->priority = Channel[i].priority;
		c->name = S_sfx[c->id].lumpname;
		c->mo = Channel[i].mo;
		c->distance = P_AproxDistance(c->mo->x-viewx, c->mo->y-viewy)>>FRACBITS;
	}
}

//==========================================================================
//
// S_GetSoundPlayingInfo
//
//==========================================================================

boolean S_GetSoundPlayingInfo(mobj_t *mobj, int sound_id)
{
	int i;

	for (i = 0; i < snd_Channels; i++)
	{
		if (Channel[i].sound_id == sound_id && Channel[i].mo == mobj)
		{
			if (I_SoundIsPlaying(Channel[i].handle))
			{
				return true;
			}
		}
	}
	return false;
}

//==========================================================================
//
// S_SetMusicVolume
//
//==========================================================================

void S_SetMusicVolume(void)
{
	if (i_CDMusic)
		I_CDMusSetVolume(snd_MusicVolume*16);	// 0-255
	I_SetMusicVolume(snd_MusicVolume);
	if (snd_MusicVolume == 0)
	{
		I_PauseSong(RegisteredSong);
		MusicPaused = true;
	}
	else if (MusicPaused)
	{
		if (!i_CDMusic || !cdaudio)
		{
			I_ResumeSong(RegisteredSong);
		}
		MusicPaused = false;
	}
}

