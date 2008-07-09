
#ifndef __SAVE_DEFS
#define __SAVE_DEFS

typedef struct
{
	int32_t	state_idx;
	int		tics;
	fixed_t	sx, sy;
} save_pspdef_t;

typedef struct
{
	spritenum_t	sprite;
	int32_t			frame;
	int32_t			tics;
	int32_t			action_func_idx;
	statenum_t		nextstate;
	int32_t			misc1, misc2;
} save_state_t;

typedef struct
{
	int32_t		prev_idx, next_idx;
	int32_t		function_idx;
} save_thinker_t;

typedef struct
{
	save_thinker_t		thinker;

	fixed_t			x, y, z;
	int32_t		snext_idx, sprev_idx;
	angle_t			angle;
	spritenum_t		sprite;
	int			frame;

	int32_t		bnext_idx, bprev_idx;
	int32_t		subsector_idx;
	fixed_t			floorz, ceilingz;
	fixed_t			floorpic;
	fixed_t			radius, height;
	fixed_t			momx, momy, momz;
	int			validcount;
	mobjtype_t		type;
	int32_t			info_idx;
	int			tics;
	int32_t			state_idx;
	int			damage;
	int			flags;
	int			flags2;
	int32_t			special1;
	int32_t			special2;
	int			health;
	int			movedir;
	int			movecount;
	int32_t			target_idx;
	int			reactiontime;
	int			threshold;
	int32_t			player_idx;
	int			lastlook;
	fixed_t			floorclip;
	int			archiveNum;
	short			tid;
	byte			special;
	byte			args[5];
} save_mobj_t;

typedef struct
{
	int32_t		mo_idx;
	playerstate_t	playerstate;
	ticcmd_t	cmd;

	pclass_t	playerclass;

	fixed_t		viewz;
	fixed_t		viewheight;
	fixed_t		deltaviewheight;
	fixed_t		bob;

	int		flyheight;
	int		lookdir;
	boolean		centering;
	int		health;
	int		armorpoints[NUMARMOR];

	inventory_t	inventory[NUMINVENTORYSLOTS];
	artitype_t	readyArtifact;
	int		artifactCount;
	int		inventorySlotNum;
	int		powers[NUMPOWERS];
	int		keys;
	int		pieces;
	signed int	frags[MAXPLAYERS];
	weapontype_t	readyweapon;
	weapontype_t	pendingweapon;
	boolean		weaponowned[NUMWEAPONS];
	int		mana[NUMMANA];
	int		attackdown, usedown;
	int		cheats;

	int		refire;

	int		killcount, itemcount, secretcount;
	char		message[80];
	int		messageTics;
	short		ultimateMessage;
	short		yellowMessage;
	int		damagecount, bonuscount;
	int		poisoncount;
	int32_t		poisoner_idx;
	int32_t		attacker_idx;
	int		extralight;
	int		fixedcolormap;
	int		colormap;
	save_pspdef_t	psprites[NUMPSPRITES];
	int		morphTics;
	unsigned int	jumpTics;
	unsigned int	worldTimer;
} save_player_t;

typedef struct
{
	save_thinker_t	thinker;
	int32_t		sector_idx;
	floor_e		type;
	int		crush;
	int		direction;
	int		newspecial;
	short		texture;
	fixed_t		floordestheight;
	fixed_t		speed;
	int		delayCount;
	int		delayTotal;
	fixed_t		stairsDelayHeight;
	fixed_t		stairsDelayHeightDelta;
	fixed_t		resetHeight;
	short		resetDelay;
	short		resetDelayCount;
	byte		textureChange;
} save_floormove_t;

typedef struct
{
	save_thinker_t	thinker;
	int32_t		sector_idx;
	int		ceilingSpeed;
	int		floorSpeed;
	int		floordest;
	int		ceilingdest;
	int		direction;
	int		crush;
} save_pillar_t;

typedef struct
{
	save_thinker_t	thinker;
	int32_t		sector_idx;
	fixed_t		originalHeight;
	fixed_t		accumulator;
	fixed_t		accDelta;
	fixed_t		targetScale;
	fixed_t		scale;
	fixed_t		scaleDelta;
	int		ticker;
	int		state;
} save_floorWaggle_t;

typedef struct
{
	save_thinker_t	thinker;
	int32_t		sector_idx;
	fixed_t		speed;
	fixed_t		low;
	fixed_t		high;
	int		wait;
	int		count;
	plat_e		status;
	plat_e		oldstatus;
	int		crush;
	int		tag;
	plattype_e	type;
} save_plat_t;

typedef struct
{
	save_thinker_t	thinker;
	int32_t		sector_idx;
	ceiling_e	type;
	fixed_t		bottomheight, topheight;
	fixed_t		speed;
	int		crush;
	int		direction;
	int		tag;
	int		olddirection;
} save_ceiling_t;

typedef struct
{
	save_thinker_t	thinker;
	int32_t		sector_idx;
	lighttype_t	type;
	int		value1;
	int		value2;
	int		tics1;
	int		tics2;
	int		count;
} save_light_t;

typedef struct
{
	save_thinker_t	thinker;
	int32_t		sector_idx;
	int		index;
	int		base;
} save_phase_t;

typedef struct
{
	save_thinker_t	thinker;
	int32_t		sector_idx;
	vldoor_e	type;
	fixed_t		topheight;
	fixed_t		speed;
	int		direction;
	int		topwait;
	int		topcountdown;
} save_vldoor_t;

typedef struct
{
	save_thinker_t thinker;
	int polyobj;
	int speed;
	unsigned int dist;
	int angle;
	fixed_t xSpeed;
	fixed_t ySpeed;
} save_polyevent_t;

typedef struct
{
	save_thinker_t thinker;
	int polyobj;
	int speed;
	int dist;
	int totalDist;
	int direction;
	fixed_t xSpeed, ySpeed;
	int tics;
	int waitTics;
	podoortype_t type;
	boolean close;
} save_polydoor_t;

typedef struct
{
	save_thinker_t	thinker;
	int32_t		activator_idx;
	int32_t		line_idx;
	int		side;
	int		number;
	int		infoIndex;
	int		delayCount;
	int		stack[ACS_STACK_DEPTH];
	int		stackPtr;
	int		vars[MAX_ACS_SCRIPT_VARS];
	int32_t		ip_idx;
} save_acs_t;

#endif	/* __SAVE_DEFS */

