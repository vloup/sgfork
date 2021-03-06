/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2003 Iron Claw Interactive
Copyright (C) 2005-2009 Smokin' Guns

This file is part of Smokin' Guns.

Smokin' Guns is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Smokin' Guns is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Smokin' Guns; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
// bg_public.h -- definitions shared by both the server game and client game modules

#include "smokinguns.h"

// because games can change separately from the main system version, we need a
// second version that must match between game and cgame
#define	GAME_VERSION		"sgfork"

//
// config strings are a general means of communicating variable length strings
// from the server to all connected clients.
//

// CS_SERVERINFO and CS_SYSTEMINFO are defined in q_shared.h
#define	CS_MUSIC				2
#define	CS_MESSAGE				3		// from the map worldspawn's message field
#define	CS_MOTD					4		// g_motd string for server message of the day
#define	CS_WARMUP				5		// server time when the match will be restarted
#define	CS_SCORES1				6
#define	CS_SCORES2				7
#define CS_VOTE_TIME			8
#define CS_VOTE_STRING			9
#define	CS_VOTE_YES				10
#define	CS_VOTE_NO				11

#define CS_TEAMVOTE_TIME		12
#define CS_TEAMVOTE_STRING		14
#define	CS_TEAMVOTE_YES			16
#define	CS_TEAMVOTE_NO			18

#define	CS_GAME_VERSION			20
#define	CS_LEVEL_START_TIME		21		// so the timer only shows the current level
#define	CS_INTERMISSION			22		// when 1, fraglimit/timelimit has been hit and intermission will start in a second or two
#define CS_FLAGSTATUS			23		// string indicating flag status in CTF
#define CS_SHADERSTATE			24
#define CS_BOTINFO				25

#define	CS_ITEMS				27		// string of 0's and 1's that tell which items are present

#define	CS_MODELS				32
#define	CS_SOUNDS				(CS_MODELS+MAX_MODELS)
#define	CS_PLAYERS				(CS_SOUNDS+MAX_SOUNDS)
#define CS_LOCATIONS			(CS_PLAYERS+MAX_CLIENTS)
#define CS_PARTICLES			(CS_LOCATIONS+MAX_LOCATIONS)

#define CS_MAX					(CS_PARTICLES+MAX_LOCATIONS)

#if (CS_MAX) > MAX_CONFIGSTRINGS
#error overflow: (CS_MAX) > MAX_CONFIGSTRINGS
#endif

#if defined QAGAME
void QDECL G_Error( const char *fmt, ... );
#define BG_Error G_Error
#define BG_Printf G_Printf
#elif defined CGAME
void QDECL CG_Error( const char *msg, ... );
#define BG_Error CG_Error
#define BG_Printf CG_Printf
#elif defined UI
#define BG_Error trap_Error
#endif

typedef enum {
	GT_FFA,				// free for all
	GT_DUEL,		// duelling mode
	GT_SINGLE_PLAYER,	// single player ffa

	//-- team games go after this --

	GT_TEAM,			// team deathmatch
	GT_RTP,				// round teamplay
	GT_BR,				// bank robbery
	GT_MAX_GAME_TYPE
} gametype_t;

typedef enum { GENDER_MALE, GENDER_FEMALE, GENDER_NEUTER } gender_t;

/*
===================================================================================

PMOVE MODULE

The pmove code takes a player_state_t and a usercmd_t and generates a new player_state_t
and some other output data.  Used for local prediction on the client game and true
movement on the server game.
===================================================================================
*/

typedef enum {
	PM_NORMAL,		// can accelerate and turn
	PM_NOCLIP,		// noclip movement
	PM_SPECTATOR,	// still run into walls
	PM_DEAD,		// no acceleration or turning, but free falling
	PM_FREEZE,		// stuck in place with no control
	PM_INTERMISSION,	// no movement or status bar
	PM_SPINTERMISSION,	// no movement or status bar
	PM_CHASECAM		// chasecam by Spoon
} pmtype_t;

typedef enum {
	WEAPON_READY,
	WEAPON_RAISING,
	WEAPON_DROPPING,
	WEAPON_FIRING,
	WEAPON_RELOADING,
	WEAPON_JUMPING,
} weaponstate_t;

// pmove->pm_flags
#define	PMF_DUCKED			1
#define	PMF_JUMP_HELD		2
#define	PMF_BACKWARDS_JUMP	8		// go into backwards land
#define	PMF_BACKWARDS_RUN	16		// coast down to backwards run
#define	PMF_TIME_LAND		32		// pm_time is time before rejump
#define	PMF_TIME_KNOCKBACK	64		// pm_time is an air-accelerate only time
#define	PMF_TIME_WATERJUMP	256		// pm_time is waterjump
#define	PMF_RESPAWNED		512		// clear after attack and jump buttons come up
#define	PMF_USE_ITEM_HELD	1024
#define PMF_GRAPPLE_PULL	2048	// pull towards grapple location
#define PMF_FOLLOW			4096	// spectate following another player
#define PMF_SCOREBOARD		8192	// spectate as a scoreboard
#define PMF_INVULEXPAND		16384	// invulnerability sphere set to full size
#define PMF_SUICIDE			32768

#define	PMF_ALL_TIMES	(PMF_TIME_WATERJUMP|PMF_TIME_LAND|PMF_TIME_KNOCKBACK)

#define	KNOCKTIME			500		// time player can't move with normal speed after being hit

#define	MAXTOUCH	32
typedef struct {
	// state (in / out)
	playerState_t	*ps;

	// command (in)
	usercmd_t	cmd;
	int			tracemask;			// collide against these types of surfaces
	int			debugLevel;			// if set, diagnostic output will be printed
	qbool	noFootsteps;		// if the game is setup for no footsteps by the server

	int			framecount;

	// results (out)
	int			numtouch;
	int			touchents[MAXTOUCH];

	vec3_t		mins, maxs;			// bounding box size

	int			watertype;
	int			waterlevel;

	float		xyspeed;

	// for fixed msec Pmove
	int			pmove_fixed;
	int			pmove_msec;

	// callbacks to test the world
	// these will be different functions during game and cgame
	void		(*trace)( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask );
	int			(*pointcontents)( const vec3_t point, int passEntityNum );
} pmove_t;

// if a full pmove isn't done on the client, you can just update the angles
void PM_UpdateViewAngles( playerState_t *ps, const usercmd_t *cmd );
void Pmove (pmove_t *pmove);

//===================================================================================


// player_state->stats[] indexes
// NOTE: may not have more than 16
typedef enum {
	STAT_HEALTH,
	STAT_HOLDABLE_ITEM,
	STAT_WEAPONS,					// 16 bit fields
	STAT_ARMOR,
	STAT_CLIENTS_READY,				// bit mask of clients wishing to exit the intermission (FIXME: configstring?)
	STAT_MAX_HEALTH,				// health / armor limit, changable by handicap

	STAT_WP_MODE,
	STAT_GATLING_MODE,
	CHASECLIENT,
	STAT_DELAY_TIME,
	IDLE_TIMER,
	STAT_MONEY,
	STAT_OLDWEAPON,
	STAT_KNOCKTIME,
	STAT_FLAGS,
	STAT_WINS
} statIndex_t;

// stat flags
#define SF_CANBUY		0x00001 // is player able to buy something?
#define SF_SEC_PISTOL	0x00002 // has player a second pistol, equal to the first one?
#define SF_GAT_CHANGE	0x00004 // player has taken a built-up gatling -> play WP_ANIM_CHANGE
#define SF_RELOAD		0x00008 // reload animation of weapon1 must still be started (akimbos)
#define SF_RELOAD2		0x00010 // reload animation of weapon2 must still be started (akimbos)
#define SF_DU_INTRO		0x00020	// player is in intro mode, don't update the viewangles
#define	SF_DU_WON		0x00040	// player has won in duel and is now able to spectate
#define	SF_WP_CHOOSE	0x00080	// this player is in weapon choose mode don't react to BUTTON_ATTACK
#define SF_BOT			0x00100	// player is a bot
#define	SF_DUELINTRO	0x00200	// player is in a intro of a duel
#define	SF_GAT_CARRY	0x00400

// player_state->persistant[] indexes
// these fields are the only part of player_state that isn't
// cleared on respawn
// NOTE: may not have more than 16
typedef enum {
	PERS_SCORE,						// !!! MUST NOT CHANGE, SERVER AND GAME BOTH REFERENCE !!!
	PERS_HITS,						// total points damage inflicted so damage beeps can sound on change
	PERS_RANK,						// player rank or team rank
	PERS_TEAM,						// player team
	PERS_SPAWN_COUNT,				// incremented every respawn
	PERS_PLAYEREVENTS,				// 16 bits that can be flipped for events
	PERS_ATTACKER,					// clientnum of last damage inflicter
	PERS_KILLED,					// count of the number of times you died
	PERS_ROBBER						// 1 if player is robber, 0 otherwise
} persEnum_t;


// entityState_t->eFlags
#define	EF_DEAD				0x00000001		// don't draw a foe marker over players with EF_DEAD
#define EF_TICKING			0x00000002		// used to make players play the prox mine ticking sound
#define	EF_TELEPORT_BIT		0x00000004		// toggled every time the origin abruptly changes
#define	EF_AWARD_EXCELLENT	0x00000008		// draw an excellent sprite
#define EF_PLAYER_EVENT		0x00000010
#define	EF_BOUNCE			0x00000010		// for missiles
#define	EF_BOUNCE_HALF		0x00000020		// for missiles
#define	EF_AWARD_GAUNTLET	0x00000040		// draw a gauntlet sprite
#define	EF_NODRAW			0x00000080		// may have an event, but no model (unspawned items)
#define	EF_FIRING			0x00000100		// for lightning gun
#define	EF_KAMIKAZE			0x00000200
#define	EF_MOVER_STOP		0x00000400		// will push otherwise
#define EF_AWARD_CAP		0x00000800		// draw the capture sprite
#define	EF_TALK				0x00001000		// draw a talk balloon
#define	EF_CONNECTION		0x00002000		// draw a connection trouble sprite
#define	EF_VOTED			0x00004000		// already cast a vote
#define	EF_AWARD_IMPRESSIVE	0x00008000		// draw an impressive sprite
#define	EF_AWARD_DEFEND		0x00010000		// draw a defend sprite
#define	EF_AWARD_ASSIST		0x00020000		// draw a assist sprite
#define EF_AWARD_DENIED		0x00040000		// denied
#define EF_TEAMVOTED		0x00080000		// already cast a team vote
#define	EF_HIT_MESSAGE		0x00100000		// a hit message of several locations is being sent

//Spoon flags
#define	EF_RELOAD			0x00100000		// if reloading
#define EF_RELOAD2			0x00200000		// if weapon2 is reloading
#define EF_BROKEN			0x00400000		// broken func_breakable just too respawn on roundstart
#define	EF_SOUNDOFF			0x00800000		// to restart sound on round restart

#define	EF_ROUND_START		0x01000000		// round start
#define	EF_DROPPED_ITEM		0x02000000		// marks dropped item
#define	EF_BUY				0x04000000
#define	EF_ACTIVATE			0x08000000



// NOTE: may not have more than 16
typedef enum {
	PW_NONE,

	// normal powerups
	PW_SCOPE,
	PW_BELT,
	PW_GOLD,

	//motion stats
	//MV_SLOW,

	//damage stats
	DM_HEAD_1,
	DM_HEAD_2,
	DM_TORSO_1,
	DM_TORSO_2,
	DM_LEGS_1,
	DM_LEGS_2,

	PW_GATLING, // has the player a gatling?

	// these PW's are only for powerups of the entitystate
	PW_BURN,	// burning dynamite/molotov
	PW_BURNBIT,

	PW_NUM_POWERUPS

} powerup_t;

typedef enum {
	WP_NONE,

	//melee
	WP_KNIFE,

	//pistols
	WP_REM58,
	WP_SCHOFIELD,
	WP_PEACEMAKER,

	//rifles
	WP_WINCHESTER66,	// should always be the first weapon after pistols
	WP_LIGHTNING,
	WP_SHARPS,

	//shotguns
	WP_REMINGTON_GAUGE,
	WP_SAWEDOFF,
	WP_WINCH97,

	//automatics
	WP_GATLING,

	//explosives
	WP_DYNAMITE,	// this always should be the last weapon after the special weapons
	WP_MOLOTOV,		// has to be the last weapon in here, because of the +prevweap-cmd

	WP_NUM_WEAPONS,

	WP_AKIMBO,

	WP_BULLETS_CLIP,
	WP_SHELLS_CLIP,
	WP_CART_CLIP,
	WP_GATLING_CLIP,
	WP_SHARPS_CLIP, // 21 !! no more place in weapons-storage !

	// For representing the left side pistol when the player has two of the
	// same kind.  This is NOT a valid index for playerState_t.ammo.
	WP_SEC_PISTOL
} weapon_t;


// reward sounds (stored in ps->persistant[PERS_PLAYEREVENTS])
#define	PLAYEREVENT_DENIEDREWARD		0x0001
#define	PLAYEREVENT_GAUNTLETREWARD		0x0002
#define PLAYEREVENT_HOLYSHIT			0x0004

// entityState_t->event values
// entity events are for effects that take place relative
// to an existing entities origin.  Very network efficient.

// two bits at the top of the entityState->event field
// will be incremented with each change in the event so
// that an identical event started twice in a row can
// be distinguished.  And off the value with ~EV_EVENT_BITS
// to retrieve the actual event number
#define	EV_EVENT_BIT1		0x00000100
#define	EV_EVENT_BIT2		0x00000200
#define	EV_EVENT_BITS		(EV_EVENT_BIT1|EV_EVENT_BIT2)

#define	EVENT_VALID_MSEC	300

typedef enum {
	EV_NONE,

	EV_FOOTSTEP,
	EV_FOOTSTEP_METAL,
	EV_FOOTSTEP_CLOTH,
	EV_FOOTSTEP_GRASS,
	EV_FOOTSTEP_SNOW, //5
	EV_FOOTSTEP_SAND,
	EV_FOOTSPLASH,
	EV_FOOTWADE,
	EV_SWIM,

	EV_STEP_4, //10
	EV_STEP_8,
	EV_STEP_12,
	EV_STEP_16,

	EV_FALL_VERY_SHORT,
	EV_FALL_SHORT, //15
	EV_FALL_MEDIUM,
	EV_FALL_FAR,
	EV_FALL_VERY_FAR,

	EV_JUMP_PAD,			// boing sound at origin, jump sound on player

	EV_JUMP, // 20
	EV_WATER_TOUCH,	// foot touches
	EV_WATER_LEAVE,	// foot leaves
	EV_WATER_UNDER,	// head touches
	EV_WATER_CLEAR,	// head leaves

	EV_ITEM_PICKUP,			// normal item pickups are predictable //25
	EV_MONEY_PICKUP,

	EV_NOAMMO,
	EV_CHOOSE_WEAPON,
	EV_CHANGE_WEAPON,
	EV_CHANGE_WEAPON_NOANIM, // 30
	EV_CHANGETO_SOUND,
	EV_CHANGE_TO_WEAPON,
	EV_FIRE_WEAPON,
	EV_FIRE_WEAPON_DELAY,
	EV_FIRE_WEAPON2,	//35
	EV_ALT_FIRE_WEAPON,
	EV_ANIM_WEAPON,
	EV_ALT_WEAPON_MODE,
	EV_BUILD_TURRET,
	EV_DISM_TURRET,	//40
	EV_SCOPE_PUT,
	EV_LIGHT_DYNAMITE,
	EV_RELOAD,
	EV_RELOAD2,
	EV_GATLING_NOTPLANAR,	//45
	EV_GATLING_NOSPACE,
	EV_GATLING_TOODEEP,

	EV_MARKED_ITEM,

	EV_UNDERWATER, //added by Spoon

	//other weapon events
	EV_DBURN,	// 50
	EV_KNIFEHIT,
	EV_NOAMMO_CLIP,

	EV_USE_ITEM0,
	EV_USE_ITEM1,
	EV_USE_ITEM2,  // 55
	EV_USE_ITEM3,
	EV_USE_ITEM4,
	EV_USE_ITEM5,
	EV_USE_ITEM6,
	EV_USE_ITEM7,  // 60
	EV_USE_ITEM8,
	EV_USE_ITEM9,
	EV_USE_ITEM10,
	EV_USE_ITEM11,
	EV_USE_ITEM12,  // 65
	EV_USE_ITEM13,
	EV_USE_ITEM14,
	EV_USE_ITEM15,

	EV_ITEM_RESPAWN,
	EV_ITEM_POP,  // 70
	EV_PLAYER_TELEPORT_IN,
	EV_PLAYER_TELEPORT_OUT,

	EV_GRENADE_BOUNCE,		// eventParm will be the soundindex

	EV_GENERAL_SOUND,
	EV_GLOBAL_SOUND,		// 75, no attenuation
	EV_GLOBAL_TEAM_SOUND,
	EV_ROUND_START,
	EV_ROUND_TIME,
	EV_DUEL_INTRO,
	EV_DUEL_WON,  // 80

	//func events
	EV_FUNCBREAKABLE,

	EV_WHISKEY_BURNS,
	EV_MISSILE_FIRE,
	EV_MISSILE_HIT,  // 90
	EV_MISSILE_MISS,
	EV_MISSILE_MISS_METAL,
	EV_RAILTRAIL,
	EV_SHOTGUN,
	EV_BULLET,				// otherEntity is the shooter

	EV_PAIN,
	EV_DEATH_DEFAULT,
	EV_DEATH_HEAD,
	EV_DEATH_ARM_L,
	EV_DEATH_ARM_R,	// 100
	EV_DEATH_CHEST,
	EV_DEATH_STOMACH,
	EV_DEATH_LEG_L,
	EV_DEATH_LEG_R,
	EV_DEATH_FALL,	// 105
	EV_DEATH_FALL_BACK,
	EV_OBITUARY,
	EV_PLAYER_HIT,

	EV_POWERUP_QUAD,
	EV_POWERUP_BATTLESUIT,	// 110
	EV_POWERUP_REGEN,

	EV_GIB_PLAYER,			// gib a previously living player
	EV_SCOREPLUM,			// score plum

	EV_DEBUG_LINE,
	EV_STOPLOOPINGSOUND,	// 115
	EV_TAUNT,
} entity_event_t;


typedef enum {
	GTS_RED_CAPTURE,
	GTS_BLUE_CAPTURE,
	GTS_RED_RETURN,
	GTS_BLUE_RETURN,
	GTS_RED_TAKEN,
	GTS_BLUE_TAKEN,
	GTS_REDOBELISK_ATTACKED,
	GTS_BLUEOBELISK_ATTACKED,
	GTS_REDTEAM_SCORED,
	GTS_BLUETEAM_SCORED,
	GTS_REDTEAM_TOOK_LEAD,
	GTS_BLUETEAM_TOOK_LEAD,
	GTS_TEAMS_ARE_TIED,
	GTS_KAMIKAZE
} global_team_sound_t;

/*
// ---BOTH--- //

0	41	0	15	// BOTH_DEATH_DEFAULT
41	46	0	15	// BOTH_DEATH_HEAD
87	40	0	15	// BOTH_DEATH_ARM_L
127	40	0	15	// BOTH_DEATH_ARM_R
167	41	0	15	// BOTH_DEATH_CHEST
208	45	0	20	// BOTH_DEATH_STOMACH
253	56	0	15	// BOTH_DEATH_LEG_L
309	56	0	15	// BOTH_DEATH_LEG_R
365	16	0	15	// BOTH_FALL_BACK
381	14	0	15	// BOTH_FALL
395	42	0	15	// BOTH_DEATH_LAND
437	9	0	15	// BOTH_JUMP
446	10	0	15	// BOTH_LAND
456	16	0	15	// BOTH_LADDER



// ---TORSO--- //

472	1	0	15	// TORSO_HOLSTERED
473	13	0	10	// TORSO_KNIFE_ATTACK
486	7	0	10	// TORSO_THROW

493	7	0	15	// TORSO_PISTOL_ATTACK
500	11	0	10	// TORSO_PISTOL_RELOAD
511	8	0	15	// TORSO_PISTOLS_ATTACK

519	6	0	15	// TORSO_RIFLE_ATTACK
525	8	0	10	// TORSO_RIFLE_RELOAD

533	11	0	10	// TORSO_GATLING_FIRE
544	15	0	10	// TORSO_GATLING_RELOAD

559	8	0	15	// TORSO_PISTOL_DROP
567	7	0	15	// TORSO_PISTOL_RAISE
574	7	0	15	// TORSO_RIFLE_DROP
581	10	0	15	// TORSO_RIFLE_RAISE
591	15	0	15	// TORSO_PISTOL_DROP_RIFLE
606	14	0	15	// TORSO_RIFLE_DROP_PISTOL

620	11	0	10	// TORSO_TAUNT
631	17	0	15	// TORSO_TAUNT_PISTOL



// ---LEGS--- //

472	21	0	15	// LEGS_WALK
493	8	0	15	// LEGS_TURN

501	15	0	15	// LEGS_RUN
516	15	0	15	// LEGS_RUN_BACK
531	11	0	15	// LEGS_SLIDE

542	20	0	15	// LEGS_IDLE

593	20	0	15	// LEGS_CROUCHED_IDLE

613	10	0	10	// LEGS_SWIM
*/

// player animations
typedef enum {

	// ---BOTH--- //

	BOTH_DEATH_DEFAULT,
	BOTH_DEAD_DEFAULT,

	BOTH_DEATH_HEAD,
	BOTH_DEAD_HEAD,

	BOTH_DEATH_ARM_L,
	BOTH_DEAD_ARM_L,

	BOTH_DEATH_ARM_R,
	BOTH_DEAD_ARM_R,

	BOTH_DEATH_CHEST,
	BOTH_DEAD_CHEST,

	BOTH_DEATH_STOMACH,
	BOTH_DEAD_STOMACH,

	BOTH_DEATH_LEG_L,
	BOTH_DEAD_LEG_L,

	BOTH_DEATH_LEG_R,
	BOTH_DEAD_LEG_R,

	BOTH_FALL_BACK,
	BOTH_DEATH_LAND,
	BOTH_FALL,
	BOTH_DEAD_LAND,

	BOTH_JUMP,
	BOTH_LAND,
	BOTH_LADDER,
	BOTH_LADDER_STAND,



// ---TORSO--- //

	TORSO_HOLSTERED,
	TORSO_KNIFE_STAND,
	TORSO_KNIFE_ATTACK,
	TORSO_THROW,

	TORSO_PISTOL_STAND,
	TORSO_PISTOL_ATTACK,
	TORSO_PISTOL_RELOAD,
	TORSO_PISTOLS_STAND,
	TORSO_PISTOLS_ATTACK,

	TORSO_RIFLE_STAND,
	TORSO_RIFLE_ATTACK,
	TORSO_RIFLE_RELOAD,

	TORSO_GATLING_STAND,
	TORSO_GATLING_FIRE,
	TORSO_GATLING_RELOAD,

	TORSO_PISTOL_DROP,
	TORSO_PISTOL_RAISE,
	TORSO_RIFLE_DROP,
	TORSO_RIFLE_RAISE,

	TORSO_TAUNT,
	TORSO_TAUNT_PISTOL,

// ---LEGS--- //

	LEGS_WALK,
	LEGS_WALK_BACK,
	LEGS_TURN,

	LEGS_RUN,
	LEGS_RUN_BACK,
	LEGS_SLIDE_LEFT,
	LEGS_SLIDE_RIGHT,

	LEGS_IDLE,

	LEGS_CROUCH_WALK,
	LEGS_CROUCH_BACK,
	LEGS_CROUCHED_IDLE,

	LEGS_SWIM,

	MAX_ANIMATIONS,

	LEGS_RUN_SLIDE,

	MAX_TOTALANIMATIONS
} animNumber_t;

// weapon animations
typedef enum {
	WP_ANIM_CHANGE,
	WP_ANIM_DROP,
	WP_ANIM_IDLE,
	WP_ANIM_FIRE,
	WP_ANIM_ALT_FIRE,
	WP_ANIM_RELOAD,
	WP_ANIM_SPECIAL,
	WP_ANIM_SPECIAL2,

	NUM_WP_ANIMATIONS
} wp_anims_t;

typedef struct animation_s {
	int		firstFrame;
	int		numFrames;
	int		loopFrames;			// 0 to numFrames
	int		frameLerp;			// msec between frames
	int		initialLerp;		// msec to get to first frame
	int		reversed;			// true if animation is reversed
	int		flipflop;			// true if animation should flipflop back to base
} animation_t;


// flip the togglebit every time an animation
// changes so a restart of the same anim can be detected
#define	ANIM_TOGGLEBIT		128


typedef enum {
	TEAM_FREE,
	TEAM_RED,
	TEAM_BLUE,
	TEAM_SPECTATOR,
	TEAM_RED_SPECTATOR,
	TEAM_BLUE_SPECTATOR,

	TEAM_NUM_TEAMS
} team_t;

// Time between location updates
#define TEAM_LOCATION_UPDATE_TIME		1000

// How many players on the overlay
#define TEAM_MAXOVERLAY		32

//team task
typedef enum {
	TEAMTASK_NONE,
	TEAMTASK_OFFENSE,
	TEAMTASK_DEFENSE,
	TEAMTASK_PATROL,
	TEAMTASK_FOLLOW,
	TEAMTASK_RETRIEVE,
	TEAMTASK_ESCORT,
	TEAMTASK_CAMP
} teamtask_t;

// means of death
typedef enum {
	//means of death of the weapons have to be in order of the weaponnums, important!
	MOD_UNKNOWN,
	//melee
	MOD_KNIFE,

	//pistols
	MOD_REM58,
	MOD_SCHOFIELD,
	MOD_PEACEMAKER,

	//rifles
	MOD_WINCHESTER66,
	MOD_LIGHTNING,
	MOD_SHARPS,

	//shotguns
	MOD_REMINGTON_GAUGE,
	MOD_SAWEDOFF,
	MOD_WINCH97,

	//automatics
	MOD_GATLING,

	//explosives
	MOD_DYNAMITE,
	MOD_MOLOTOV,

	MOD_WATER,
	MOD_SLIME,
	MOD_LAVA,
	MOD_CRUSH,
	MOD_TELEFRAG,
	MOD_FALLING,
	MOD_SUICIDE,
	MOD_TARGET_LASER,
	MOD_TRIGGER_HURT,
} meansOfDeath_t;


//---------------------------------------------------------

// gitem_t->type
typedef enum {
	IT_BAD,
	IT_WEAPON,				// EFX: rotate + upscale + minlight
	IT_AMMO,				// EFX: rotate
	IT_ARMOR,				// EFX: rotate + minlight
	IT_HEALTH,				// EFX: static external sphere + rotating internal
	IT_POWERUP,				// instant on, timer based
							// EFX: rotate + external ring that rotates
	IT_PERSISTANT_POWERUP,
	IT_TEAM
} itemType_t;

#define MAX_ITEM_MODELS 4

typedef struct wpinfo_s {
	animation_t	animations[NUM_WP_ANIMATIONS];
	float	spread;		// For missiles it is speed
	float	damage;		// For missiles it is splashDamage as well
	float	knockback;
	int		range;		// For missiles it is splashRadius
	int		addTime;
	int		count;
	int		clipAmmo;	//ammo that fits in the weapon
	int		clip;
	int		maxAmmo;	// maximum of holdable ammo
	char	v_model[MAX_QPATH];
	char	v_barrel[MAX_QPATH];
	char	snd_fire[MAX_QPATH];
	char	snd_reload[MAX_QPATH];
	char	name[MAX_QPATH];
	char	path[MAX_QPATH];
	int		wp_sort;
} wpinfo_t;

typedef enum {
	WPS_NONE,
	WPS_MELEE,
	WPS_PISTOL,
	WPS_GUN,
	WPS_OTHER,
	WPS_NUM_ITEMS
} wp_sort_t;

typedef enum {
	WS_NONE,
	WS_PISTOL,
	WS_RIFLE,
	WS_SHOTGUN,
	WS_MGUN,
	WS_MISC
} wp_buy_type;

typedef struct gitem_s {
	char		classname[MAX_QPATH];					// Spawning name
	char		pickup_sound[MAX_QPATH];				// Sound on Pickup
	char		world_model[MAX_ITEM_MODELS][MAX_QPATH];// Item models

	char		icon[MAX_QPATH];		// Icon shader
	float		xyrelation;				// Icon x/y relation
	char		pickup_name[MAX_QPATH];	// Name to show on pickup

	int			quantity;				// How much ammo or duration of powerup
	itemType_t  giType;					// IT_* flags (weapon, armor, powerup, etc)

	int			giTag;					// WP_*, PW_*

	int			prize;					// Price of the item
	int			weapon_sort;			// Used for sorted weapons in changeweapon-menu

	char		precaches[MAX_QPATH];	// All models and images this item will use
	char		sounds[MAX_QPATH];		// All sounds this item will use
} gitem_t;

// included in both the game dll and the client
extern	wpinfo_t bg_weaponlist[WP_NUM_WEAPONS];

gitem_t	*BG_Item( const char *pickupName );
gitem_t	*BG_ItemByClassname( const char *classname );
gitem_t	*BG_ItemForWeapon( weapon_t weapon );
gitem_t	*BG_ItemForAmmo( weapon_t ammo ) ;
gitem_t	*BG_ItemForPowerup( powerup_t pw );
int	BG_PlayerWeapon( int firstweapon, int lastweapon, playerState_t	*ps);
#define	ITEM_INDEX(x) ((x)-bg_itemlist)

qbool	BG_CanItemBeGrabbed(int gametype, const entityState_t *ent, const playerState_t *ps,int MaxMoney);


// g_dmflags->integer flags
#define	DF_NO_FALLING			8
#define DF_FIXED_FOV			16
#define	DF_NO_FOOTSTEPS			32

// content masks
#define	MASK_ALL				(-1)
#define	MASK_SOLID				(CONTENTS_SOLID)
#define	MASK_PLAYERSOLID		(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_BODY)
#define	MASK_DEADSOLID			(CONTENTS_SOLID|CONTENTS_PLAYERCLIP)
#define	MASK_WATER				(CONTENTS_WATER|CONTENTS_LAVA|CONTENTS_SLIME)
#define	MASK_OPAQUE				(CONTENTS_SOLID|CONTENTS_SLIME|CONTENTS_LAVA)
#define	MASK_SHOT				(CONTENTS_SOLID|CONTENTS_TRIGGER2|CONTENTS_BODY|CONTENTS_CORPSE)

//
// entityState_t->eType
//
typedef enum {
	ET_GENERAL,
	ET_PLAYER,
	ET_ITEM,
	ET_MISSILE,
	ET_MOVER,
	ET_BEAM,
	ET_PORTAL,
	ET_SPEAKER,
	ET_PUSH_TRIGGER,
	ET_TELEPORT_TRIGGER,
	ET_INVISIBLE,
	ET_FLY,
	ET_GRAPPLE,				// grapple hooked on wall
	ET_TEAM,
	ET_BREAKABLE,
	ET_INTERMISSION,	// need for duel (camera move), not anymore
	ET_FLARE,
	ET_SMOKE,
	ET_TURRET, // gatling guns, etc
	ET_ESCAPE, // escape points

	ET_EVENTS				// any of the EV_* events can be added freestanding
							// by setting eType to ET_EVENTS + eventNum
							// this avoids having to set eFlags and eventNum
} entityType_t;



void	BG_EvaluateTrajectory( const trajectory_t *tr, int atTime, vec3_t result );
void	BG_EvaluateTrajectoryDelta( const trajectory_t *tr, int atTime, vec3_t result );

void	BG_AddPredictableEventToPlayerstate( int newEvent, int eventParm, playerState_t *ps );

void	BG_PlayerStateToEntityState( playerState_t *ps, entityState_t *s, qbool snap );
void	BG_PlayerStateToEntityStateExtraPolate( playerState_t *ps, entityState_t *s, int time, qbool snap );

qbool	BG_PlayerTouchesItem( playerState_t *ps, entityState_t *item, int atTime );

int BG_AnimLength( int anim, int weapon);

#define MAX_ARENAS			1024
#define	MAX_ARENAS_TEXT		8192

#define MAX_BOTS			1024
#define MAX_BOTS_TEXT		8192

#define NUM_GIBS	9

#define NUM_PREFIXINFO		12 //very important
#define	MAX_TEXINFOFILE		10000
#define	MAX_BRUSHSIDES		200

// ai nodes for the bots
#define MAX_AINODES			100

//prefixInfo-stats
typedef struct {
	char	*name;
	int		surfaceFlags;
	float	radius;
	float	weight;
	int		numParticles;
	float	fallingDamageFactor;
	float	thickness; // used for shoot-thru-walls code
} prefixInfo_t;

extern	prefixInfo_t	prefixInfo[NUM_PREFIXINFO];

void BG_StringRead(char *destination, char *source, int size);
void BG_ModifyEyeAngles( vec3_t origin, vec3_t viewangles,
						void (*trace)( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask ),
						vec3_t legOffset, qbool print);
int BG_CountTypeWeapons(int type, int weapons);
int BG_SearchTypeWeapon(int type, int weapons, int wp_ignore);

void BG_SetWhiskeyDrop(trajectory_t *tr, vec3_t org, vec3_t normal, vec3_t dir);
void BG_DirsToEntityState(entityState_t *es, vec3_t bottledirs[ALC_COUNT]);
void BG_EntityStateToDirs(entityState_t *es, vec3_t bottledirs[ALC_COUNT]);

qbool CheckPistols(playerState_t *ps, int *weapon);
int BG_MapPrefix(char *map, int gametype);

extern const vec3_t	playerMins;
extern const vec3_t	playerMaxs;

extern vec3_t gatling_mins;
extern vec3_t gatling_maxs;
extern vec3_t gatling_mins2;
extern vec3_t gatling_maxs2;

//
// bg_parse.c
//
#define PFT_WEAPONS 0x0001
#define PFT_ITEMS 0x0002
#define PFT_GRAVITY 0x0004

#define IT_NUM_ITEMS 27

extern	gitem_t	bg_itemlist[IT_NUM_ITEMS];

typedef struct psf_weapons_s {
  char *fileName;
  char *numNames;
} psf_weapons_t;

extern const char *psf_itemTypes[];
extern const char *psf_item_fileNames[];
extern const char *psf_weapon_sortNames[];
extern const char *psf_weapon_animName[];
extern const char *psf_weapon_buyType[];
extern const psf_weapons_t psf_weapons_config[];

extern void parse_config( int fileType, char *fileName, int num );
extern char *Parse_FindFile(char *filename_out, int filename_out_size, int num, int fileType);
extern void config_GetInfos( int fileType );
extern int BG_ItemNumByName( char *name );
extern int BG_AnimNumByName( char *anim );
extern int BG_WeaponNumByName( char *wp );
int BG_WeaponListChange( char *weapon, char *an, char *dest, char *value );
extern int BG_ItemListChange( char *item, char *dest, char *value );
int BG_WSNumByName( char *s );
int BG_GitagNumByName( char *s );
int BG_GitypeByName( char *s );

// filesystem access
// returns length of file
int			trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode );
void		trap_FS_Read( void *buffer, int len, fileHandle_t f );
void		trap_FS_Write( const void *buffer, int len, fileHandle_t f );
void		trap_FS_FCloseFile( fileHandle_t f );
int			trap_FS_Seek( fileHandle_t f, long offset, int origin ); // fsOrigin_t
