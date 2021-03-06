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
#include "g_local.h"

// g_client.c -- client functions that don't happen every frame

/*QUAKED info_player_deathmatch (1 0 1) (-16 -16 -24) (16 16 32) initial
potential spawning position for deathmatch games.
The first time a player enters the game, they will be at an 'initial' spot.
Targets will be fired when someone spawns in on them.
"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
*/
void SP_info_player_deathmatch( gentity_t *ent ) {
	int		i;

	G_SpawnInt( "nobots", "0", &i);
	if ( i )
		ent->flags |= FL_NO_BOTS;
	G_SpawnInt( "nohumans", "0", &i );
	if ( i )
		ent->flags |= FL_NO_HUMANS;
}

/*QUAKED info_player_start (1 0 0) (-16 -16 -24) (16 16 32)
equivelant to info_player_deathmatch
*/
void SP_info_player_start(gentity_t *ent) {
	ent->classname = "info_player_deathmatch";
	SP_info_player_deathmatch( ent );
}

/*QUAKED info_player_intermission (1 0 1) (-16 -16 -24) (16 16 32)
The intermission will be viewed from this point.  Target an info_notnull for the view direction.
*/
void SP_info_player_intermission( gentity_t *ent ) {

	if(g_gametype.integer != GT_DUEL)
		return;

	// add this so that cgame also knows about this entity (needed for duel camera moves)
	VectorCopy( ent->s.origin, ent->pos1 );

	ent->s.eType = ET_INTERMISSION;
	G_SpawnInt( "part", "0", &ent->s.eventParm);

	trap_LinkEntity(ent);
}
/*
==========================
G_AnimLength
by Spoon
==========================
*/
int G_AnimLength( int anim, int weapon) {
	int length;

	length = (bg_weaponlist[weapon].animations[anim].numFrames-1)*
		bg_weaponlist[weapon].animations[anim].frameLerp+
		bg_weaponlist[weapon].animations[anim].initialLerp;

	if(bg_weaponlist[weapon].animations[anim].flipflop){
		length *= 2;
		length -= bg_weaponlist[weapon].animations[anim].initialLerp;
	}

	return length;
}


/*
=======================================================================

  SelectSpawnPoint

=======================================================================
*/

/*
================
SpotWouldTelefrag

================
*/
qbool SpotWouldTelefrag( gentity_t *spot ) {
	int			i, num;
	int			touch[MAX_GENTITIES];
	gentity_t	*hit;
	vec3_t		mins, maxs;

	VectorAdd( spot->s.origin, playerMins, mins );
	VectorAdd( spot->s.origin, playerMaxs, maxs );
	num = trap_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	for (i=0 ; i<num ; i++) {
		hit = &g_entities[touch[i]];
		//if ( hit->client && hit->client->ps.stats[STAT_HEALTH] > 0 ) {
		if (hit->client) {
			return qtrue;
		}
	}

	return qfalse;
}

gentity_t *SetPlayerToSpawnPoint(gentity_t *spot, vec3_t origin, vec3_t angles) {
	VectorCopy(spot->s.origin, origin);
	VectorCopy(spot->s.angles, angles);

	// if it has a target, look towards it
	if (spot->target) {
		gentity_t *target;
		vec3_t	dir;

		target = G_PickTarget(spot->target);
		if (target) {
			VectorSubtract(target->s.origin, origin, dir);
			vectoangles(dir, angles);
		}
	}

	return spot;
}

/*
===========
SelectRandomFurthestSpawnPoint

Chooses a player start, deathmatch start, etc
============
*/
#define	MAX_SPAWN_POINTS	128
static gentity_t *SelectRandomFurthestSpawnPoint(vec3_t avoidPoint, int ignoreTeam, qbool isbot) {
	gentity_t	*spot = NULL;
	vec3_t		delta;
	float		dist;
	float		list_dist[MAX_SPAWN_POINTS];
	gentity_t	*list_spot[MAX_SPAWN_POINTS];
	int			numSpots = 0;
	int			rnd, i, j;

	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL) {
		if (SpotWouldTelefrag(spot))
			continue;

		// Check if spot is not for this human/bot player
		if (((spot->flags & FL_NO_BOTS) && isbot) ||
			((spot->flags & FL_NO_HUMANS) && !isbot))
			continue;

		// spots are inserted in sorted order, furthest away first
		VectorSubtract(spot->s.origin, avoidPoint, delta);
		dist = VectorLengthSquared(delta);
		for (i = 0; i < numSpots; i++) {
			if (dist > list_dist[i]) {
				if ( numSpots >= MAX_SPAWN_POINTS )
					numSpots = MAX_SPAWN_POINTS-1;
				for (j = numSpots; j > i; j--) {
					list_dist[j] = list_dist[j-1];
					list_spot[j] = list_spot[j-1];
				}
				list_dist[i] = dist;
				list_spot[i] = spot;
				numSpots++;
				if (numSpots > MAX_SPAWN_POINTS)
					numSpots = MAX_SPAWN_POINTS;
				break;
			}
		}
		if (i >= numSpots && numSpots < MAX_SPAWN_POINTS) {
			list_dist[numSpots] = dist;
			list_spot[numSpots] = spot;
			numSpots++;
		}
	}

	if (!numSpots)
		return G_Find(NULL, FOFS(classname), "info_player_deathmatch");

	// try find a spawn point that's not too close to another player
	i = rnd = rand() % (numSpots / 2);
	while (1) {
		spot = list_spot[i];
		if (!G_IsAnyClientWithinRadius(spot->s.origin, MAX_FREESPAWN_ZONE_DISTANCE_SQUARE, ignoreTeam))
			return spot;
		if (i > 0 && i <= rnd)
			i--;
		else if (i <= 0)
			i = rnd + 1;
		else if (i < numSpots - 2)
			i++;
		else
			break;
	}

	return list_spot[rnd];
}

static gentity_t *SelectDuelSpawnPoint(int mappart, qbool clientTrio, vec3_t origin, vec3_t angles) {
	gentity_t	*spot = NULL;

	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL) {
			if (spot->mappart == mappart && !spot->used) {
				// if this is a trio player, find a trio-spawnpoint
				if (clientTrio && !spot->trio)
					continue;
				spot->used = qtrue;
				break;
			} else
				continue;
	}

	if (spot)
		return SetPlayerToSpawnPoint(spot, origin, angles);

	G_Error("Couldn't find a duel-spawnpoint!\n");
	return NULL;
}

/*
===========
SelectSpawnPoint

Chooses a player start, deathmatch start, etc
============
*/
gentity_t *SelectSpawnPoint(vec3_t avoidPoint, vec3_t origin, vec3_t angles, int ignoreTeam, qbool isbot) {
	gentity_t	*spot;

	spot = SelectRandomFurthestSpawnPoint(avoidPoint, ignoreTeam, isbot);

	if (!spot)
		G_Error( "Couldn't find a spawn point" );

	return SetPlayerToSpawnPoint(spot, origin, angles);
}

/*
===========
SelectInitialSpawnPoint

Try to find a spawn point marked 'initial', otherwise
use normal spawn selection.
============
*/
static gentity_t *SelectInitialSpawnPoint( vec3_t origin, vec3_t angles, int ignoreTeam, qbool isbot ) {
	gentity_t	*spot = NULL;
	
	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL) {
		if(((spot->flags & FL_NO_BOTS) && isbot) ||
		   ((spot->flags & FL_NO_HUMANS) && !isbot))
			continue;
		
		if((spot->spawnflags & 0x01))
			break;
	}

	if (!spot || SpotWouldTelefrag(spot))
		return SelectSpawnPoint(vec3_origin, origin, angles, ignoreTeam, isbot);

	return SetPlayerToSpawnPoint(spot, origin, angles);
}

/*
===========
SelectSpectatorSpawnPoint

============
*/
static gentity_t *SelectSpectatorSpawnPoint( vec3_t origin, vec3_t angles, int mappart) {
	FindIntermissionPoint(mappart);

	VectorCopy( level.intermission_origin, origin );
	VectorCopy( level.intermission_angle, angles );

	return NULL;
}

/*
=======================================================================

BODYQUE

=======================================================================
*/

/*
===============
InitBodyQue
===============
*/
void InitBodyQue (void) {
	int		i;
	gentity_t	*ent;

	level.bodyQueIndex = 0;
	for (i=0; i<BODY_QUEUE_SIZE ; i++) {
		ent = G_Spawn();
		ent->classname = "bodyque";
		ent->neverFree = qtrue;
		level.bodyQue[i] = ent;
	}
}

/*
=============
BodySink

After sitting around for ten seconds, fall into the ground and dissapear
=============
*/
void BodySink( gentity_t *ent ) {
	qbool duel = (g_gametype.integer == GT_DUEL);

	// make sure no body is left at round restart
	if((g_gametype.integer >= GT_RTP && g_round > ent->angle) ||
		(g_gametype.integer == GT_DUEL && g_session != ent->angle)){
		ent->physicsObject = qfalse;
		trap_UnlinkEntity( ent );
		return;
	}

	// don't do anything
	if(level.time - ent->timestamp < 10000 && !duel){
		ent->nextthink = level.time + 50;
		return;
	}

	if ( level.time - ent->timestamp > 15000 && !duel) {
		// the body ques are never actually freed, they are just unlinked
		trap_UnlinkEntity( ent );
		ent->physicsObject = qfalse;
		return;
	}

	ent->nextthink = level.time + 50;

	if(!duel)
		ent->s.pos.trBase[2] -= 1;
}

/*
=============
CopyToBodyQue

A player is respawning, so make an entity that looks
just like the existing corpse to leave behind.
=============
*/
void CopyToBodyQue( gentity_t *ent ) {
	gentity_t		*body;
	int			contents;

	trap_UnlinkEntity (ent);

	// if client is in a nodrop area, don't leave the body
	contents = trap_PointContents( ent->s.origin, -1 );
	if ( contents & CONTENTS_NODROP ) {
		return;
	}

	// grab a body que and cycle to the next one
	body = level.bodyQue[ level.bodyQueIndex ];
	level.bodyQueIndex = (level.bodyQueIndex + 1) % BODY_QUEUE_SIZE;

	trap_UnlinkEntity (body);

	body->s = ent->s;
	body->s.eType = ET_PLAYER;
	body->s.eFlags = EF_DEAD;		// clear EF_TALK, etc
	body->s.powerups &= ~(1 << PW_GOLD);
	body->s.loopSound = 0;	// clear lava burning
	body->s.number = body - g_entities;
	body->timestamp = level.time;
	body->physicsObject = qtrue;
	body->physicsBounce = 0;		// don't bounce
	if ( body->s.groundEntityNum == ENTITYNUM_NONE ) {
		body->s.pos.trType = TR_GRAVITY;
		body->s.pos.trTime = level.time;
		VectorCopy( ent->client->ps.velocity, body->s.pos.trDelta );
	} else
		body->s.pos.trType = TR_STATIONARY;

	body->s.event = 0;

	if(g_gametype.integer != GT_DUEL || !ent->client->specwatch){
		// change the animation to the last-frame only, so the sequence
		// doesn't repeat anew for the body
		switch ( body->s.legsAnim & ~ANIM_TOGGLEBIT ) {
		case BOTH_DEATH_HEAD:
		case BOTH_DEAD_HEAD:
			body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD_HEAD;
			break;
		case BOTH_DEATH_ARM_L:
		case BOTH_DEAD_ARM_L:
			body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD_ARM_L;
			break;
		case BOTH_DEATH_ARM_R:
		case BOTH_DEAD_ARM_R:
			body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD_ARM_R;
			break;
		case BOTH_DEATH_CHEST:
		case BOTH_DEAD_CHEST:
			body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD_CHEST;
			break;
		case BOTH_DEATH_STOMACH:
		case BOTH_DEAD_STOMACH:
			body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD_STOMACH;
			break;
		case BOTH_DEATH_LEG_L:
		case BOTH_DEAD_LEG_L:
			body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD_LEG_L;
			break;
		case BOTH_DEATH_LEG_R:
		case BOTH_DEAD_LEG_R:
			body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD_LEG_R;
			break;
		case BOTH_FALL:
		case BOTH_FALL_BACK:
		case BOTH_DEATH_LAND:
		case BOTH_DEAD_LAND:
			body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD_LAND;
			break;
		default:
			body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD_DEFAULT;
			break;
		}
	} else {
		body->s.torsoAnim = ent->s.torsoAnim;
		body->s.legsAnim = LEGS_IDLE;
	}

	body->r.svFlags = ent->r.svFlags;
	VectorCopy (ent->r.mins, body->r.mins);
	VectorCopy (ent->r.maxs, body->r.maxs);
	VectorCopy (ent->r.absmin, body->r.absmin);
	VectorCopy (ent->r.absmax, body->r.absmax);

	body->clipmask = CONTENTS_SOLID | CONTENTS_PLAYERCLIP;
	body->r.contents = CONTENTS_CORPSE;
	body->r.ownerNum = ent->s.number;

	body->nextthink = level.time + 4000;
	body->think = BodySink;

	body->die = 0;

	// don't take more damage if already gibbed
	body->takedamage = qfalse;

	if( g_gametype.integer >= GT_RTP)
		body->angle = g_round;

	if( g_gametype.integer == GT_DUEL)
		body->angle = g_session;

	VectorCopy ( body->s.pos.trBase, body->r.currentOrigin );
	trap_LinkEntity (body);
}

//======================================================================


/*
==================
SetClientViewAngle

==================
*/
void SetClientViewAngle( gentity_t *ent, vec3_t angle ) {
	int			i;

	// set the delta angle
	for (i=0 ; i<3 ; i++) {
		int		cmdAngle;

		cmdAngle = ANGLE2SHORT(angle[i]);
		ent->client->ps.delta_angles[i] = cmdAngle - ent->client->pers.cmd.angles[i];
	}
	VectorCopy( angle, ent->s.angles );
	VectorCopy (ent->s.angles, ent->client->ps.viewangles);
}

/*
================
respawn
================
*/
void respawn( gentity_t *ent ) {
	CopyToBodyQue (ent);
	ClientSpawn(ent);
}

/*
================
TeamCount

Returns number of players on a team
================
*/
team_t TeamCount( int ignoreClientNum, int team ) {
	int		i;
	int		count = 0;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( i == ignoreClientNum 
        || level.clients[i].pers.connected == CON_DISCONNECTED 
        || (g_gametype.integer >= GT_RTP || g_gametype.integer == GT_DUEL) &&
              level.clients[i].pers.connected != CON_CONNECTED
        || (g_gametype.integer == GT_DUEL && team == TEAM_SPECTATOR &&
              g_entities[i].client->realspec) )
			continue;

		if ( level.clients[i].sess.sessionTeam == team )
			count++;
	}

	return count;
}

int MappartCount( int ignoreClientNum, int team ) {
	int		i;
	int		count = 0;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if( i == ignoreClientNum
      || level.clients[i].pers.connected == CON_DISCONNECTED 
      || (g_gametype.integer >= GT_RTP || g_gametype.integer == GT_DUEL)
            && level.clients[i].pers.connected != CON_CONNECTED 
      || (g_gametype.integer == GT_DUEL && team == TEAM_SPECTATOR
            && g_entities[i].client->realspec) )
			continue;

		if ( level.clients[i].sess.sessionTeam == team )
			count++;
	}
	return count;
}

/*
================
TeamLeader

Returns the client number of the team leader
================
*/
int TeamLeader( int team ) {
	int		i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected == CON_DISCONNECTED )
			continue;

		if ( level.clients[i].sess.sessionTeam == team ) {
			if ( level.clients[i].sess.teamLeader )
				return i;
		}
	}
	return -1;
}


/*
================
PickTeam

================
*/
team_t PickTeam( int ignoreClientNum ) {
	int		counts[TEAM_NUM_TEAMS];

	counts[TEAM_BLUE] = TeamCount( ignoreClientNum, TEAM_BLUE );
	counts[TEAM_RED] = TeamCount( ignoreClientNum, TEAM_RED );

	if ( counts[TEAM_BLUE] > counts[TEAM_RED] )
		return TEAM_RED;
  else if ( counts[TEAM_RED] > counts[TEAM_BLUE] )
		return TEAM_BLUE;
 
	// equal team count, so join the team with the lowest score
	if ( level.teamScores[TEAM_BLUE] > level.teamScores[TEAM_RED] )
		return TEAM_RED;

	return TEAM_BLUE;
}

/*
===========
ForceClientSkin

Forces a client's skin (for teamplay)
===========
*/
static void ForceClientSkin( gclient_t *client, char *model, const char *skin ) {
	char *p;

	if ((p = Q_strrchr(model, '/')) != 0)
		*p = 0;

	Q_strcat(model, MAX_QPATH, "/");
	Q_strcat(model, MAX_QPATH, skin);
}

/*
===========
ClientCheckName
============
*/
static void ClientCleanName(const char *in, char *out, int outSize)
{
	int outpos = 0, colorlessLen = 0, spaces = 0;

	// discard leading spaces
	for(; *in == ' '; in++);
	
	for(; *in && outpos < outSize - 1; in++)
	{
		out[outpos] = *in;

		if(*in == ' ')
		{
			// don't allow too many consecutive spaces
			if(spaces > 2)
				continue;
			
			spaces++;
		}
		else if(outpos > 0 && out[outpos - 1] == Q_COLOR_ESCAPE)
		{
			if(Q_IsColorString(&out[outpos - 1]))
			{
				colorlessLen--;
				
				if(ColorIndex(*in) == 0)
				{
					// Disallow color black in names to prevent players
					// from getting advantage playing in front of black backgrounds
					outpos--;
					continue;
				}
			}
			else
			{
				spaces = 0;
				colorlessLen++;
			}
		}
		else
		{
			spaces = 0;
			colorlessLen++;
		}
		
		outpos++;
	}

	out[outpos] = '\0';

	// don't allow empty names
	if( *out == '\0' || colorlessLen == 0)
		Q_strncpyz(out, "UnnamedPlayer", outSize );
}


/*
===========
ClientUserInfoChanged

Called from ClientConnect when the player first connects and
directly by the server system when the player updates a userinfo variable.

The game can override any of the settings and call trap_SetUserinfo
if desired.
============
*/
void ClientUserinfoChanged( int clientNum ) {
	gentity_t *ent;
	int		teamTask, teamLeader, team, health;
	char	*s;
	char	model[MAX_QPATH];
	char	headModel[MAX_QPATH];
	char	oldname[MAX_STRING_CHARS];
	gclient_t	*client;
	char	c1[MAX_INFO_STRING];
	char	c2[MAX_INFO_STRING];
	char	redTeam[MAX_INFO_STRING];
	char	blueTeam[MAX_INFO_STRING];
	char	userinfo[MAX_INFO_STRING];
	char	version[MAX_INFO_STRING];
	char    guid[MAX_INFO_STRING];

	ent = g_entities + clientNum;
	client = ent->client;

	trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

	// check for malformed or illegal info strings
	if ( !Info_Validate(userinfo) )
		strcpy (userinfo, "\\name\\badinfo");

	// check for local client
	s = Info_ValueForKey( userinfo, "ip" );
	if ( !strcmp( s, "localhost" ) )
		client->pers.localClient = qtrue;

	// check the item prediction
	s = Info_ValueForKey( userinfo, "cg_predictItems" );
	if ( !atoi( s ) )
		client->pers.predictItemPickup = qfalse;
	else
		client->pers.predictItemPickup = qtrue;

//unlagged - client options
	// see if the player has opted out
	s = Info_ValueForKey( userinfo, "cg_delag" );
	if ( !atoi( s ) )
		client->pers.delag = 0;
	else
		client->pers.delag = atoi( s );

	// see if the player is nudging his shots
	s = Info_ValueForKey( userinfo, "cg_cmdTimeNudge" );
	client->pers.cmdTimeNudge = atoi( s );

	// see if the player wants to debug the backward reconciliation
	s = Info_ValueForKey( userinfo, "cg_debugDelag" );
	if ( !atoi( s ) )
		client->pers.debugDelag = qfalse;
	else
		client->pers.debugDelag = qtrue;

	// see if the player is simulating incoming latency
	s = Info_ValueForKey( userinfo, "cg_latentSnaps" );
	client->pers.latentSnaps = atoi( s );

	// see if the player is simulating outgoing latency
	s = Info_ValueForKey( userinfo, "cg_latentCmds" );
	client->pers.latentCmds = atoi( s );

	// see if the player is simulating outgoing packet loss
	s = Info_ValueForKey( userinfo, "cg_plOut" );
	client->pers.plOut = atoi( s );
//unlagged - client options

	// set name
	Q_strncpyz ( oldname, client->pers.netname, sizeof( oldname ) );
	s = Info_ValueForKey (userinfo, "name");
	ClientCleanName( s, client->pers.netname, sizeof(client->pers.netname) );

	if( client->sess.sessionTeam >= TEAM_SPECTATOR
      && client->sess.spectatorState == SPECTATOR_SCOREBOARD )
			Q_strncpyz( client->pers.netname, "scoreboard", sizeof(client->pers.netname) );

  if ( client->pers.connected == CON_CONNECTED
      && strcmp( oldname, client->pers.netname ) ) {
    trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " renamed to %s\n\"", oldname,
          client->pers.netname) );
    G_LogPrintf( "Nick change: %s to %s_n", oldname, client->pers.netname );
  }

	// set max health
	health = atoi( Info_ValueForKey( userinfo, "handicap" ) );
	client->pers.maxHealth = health;
	if ( client->pers.maxHealth < 1 || client->pers.maxHealth > 100 )
		client->pers.maxHealth = 100;

	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;

	// set model
	if( g_gametype.integer >= GT_TEAM ) {
		Q_strncpyz( model, Info_ValueForKey (userinfo, "team_model"), sizeof( model ) );
		Q_strncpyz( headModel, Info_ValueForKey (userinfo, "team_headmodel"), sizeof( headModel ) );
	} else {
		Q_strncpyz( model, Info_ValueForKey (userinfo, "model"), sizeof( model ) );
		Q_strncpyz( headModel, Info_ValueForKey (userinfo, "headmodel"), sizeof( headModel ) );
	}

	// bots set their team a few frames later
	if (g_gametype.integer >= GT_TEAM && ent->r.svFlags & SVF_BOT) {
		s = Info_ValueForKey( userinfo, "team" );
		if ( !Q_stricmp( s, "red" ) || !Q_stricmp( s, "r" ) || !Q_stricmp( s, "redspec" )) {
			if(g_gametype.integer >= GT_RTP)
				team = TEAM_RED_SPECTATOR;
			else
				team = TEAM_RED;
		} else if ( !Q_stricmp( s, "blue" ) || !Q_stricmp( s, "b" ) || !Q_stricmp( s, "bluespec" )) {
			if(g_gametype.integer >= GT_RTP)
				team = TEAM_BLUE_SPECTATOR;
			else
				team = TEAM_BLUE;
		} else
			// pick the team with the least number of players
			team = PickTeam( clientNum );

		if(client->sess.sessionTeam == TEAM_SPECTATOR)
			client->sess.sessionTeam = team;
	}
	else
		team = client->sess.sessionTeam;

	// teamInfo
	s = Info_ValueForKey( userinfo, "teamoverlay" );
	if ( ! *s || atoi( s ) != 0 )
		client->pers.teamInfo = qtrue;
	else 
		client->pers.teamInfo = qfalse;

	// team task (0 = none, 1 = offence, 2 = defence)
	teamTask = atoi(Info_ValueForKey(userinfo, "teamtask"));
	// team Leader (1 = leader, 0 is normal player)
	teamLeader = client->sess.teamLeader;

	// colors
	strcpy(c1, Info_ValueForKey( userinfo, "color1" ));
	strcpy(c2, Info_ValueForKey( userinfo, "color2" ));
	strcpy(redTeam, Info_ValueForKey( userinfo, "g_redteam" ));
	strcpy(blueTeam, Info_ValueForKey( userinfo, "g_blueteam" ));
	strcpy(guid, Info_ValueForKey( userinfo, "cl_guid" ));
	strcpy(version, Info_ValueForKey( userinfo, "cl_version" ));

	// send over a subset of the userinfo keys so other clients can
	// print scoreboards, display models, and play custom sounds
	if (ent->r.svFlags & SVF_BOT) {
		s = va("n\\%s\\t\\%i\\model\\%s\\hc\\%i\\w\\%i\\l\\%i\\skill\\%s\\tt\\%d\\tl\\%d",
			client->pers.netname, client->sess.sessionTeam, model,
			client->pers.maxHealth, client->sess.wins, client->sess.losses,
			Info_ValueForKey( userinfo, "skill" ), teamTask, teamLeader);
	} else if (*guid == 0) {
		s = va("n\\%s\\t\\%i\\model\\%s\\g_redteam\\%s\\g_blueteam\\%s\\hc\\%i\\w\\%i\\l\\%i\\tt\\%d\\tl\\%d\\v\\%s",
			client->pers.netname, client->sess.sessionTeam, model, redTeam, blueTeam,
			client->pers.maxHealth, client->sess.wins, client->sess.losses, teamTask, teamLeader, version);
	} else {
		s = va("guid\\%s\\n\\%s\\t\\%i\\model\\%s\\g_redteam\\%s\\g_blueteam\\%s\\hc\\%i\\w\\%i\\l\\%i\\tt\\%d\\tl\\%d\\v\\%s",
			guid,
			client->pers.netname, client->sess.sessionTeam, model, redTeam, blueTeam,
			client->pers.maxHealth, client->sess.wins, client->sess.losses, teamTask, teamLeader, version);
	}

	trap_SetConfigstring( CS_PLAYERS+clientNum, s );

	// this is not the userinfo, more like the configstring actually
	G_LogPrintf( "ClientUserinfoChanged: %i %s\n", clientNum, s );
}


/*
===========
ClientConnect

Called when a player begins connecting to the server.
Called again for every map change or tournement restart.

The session information will be valid after exit.

Return NULL if the client should be allowed, otherwise return
a string with the reason for denial.

Otherwise, the client will be sent the current gamestate
and will eventually get to ClientBegin.

firstTime will be qtrue the very first time a client connects
to the server machine, but qfalse on map changes and tournement
restarts.
============
*/
char *ClientConnect( int clientNum, qbool firstTime, qbool isBot ) {
	char		*value;
//	char		*areabits;
	gclient_t	*client;
	char		userinfo[MAX_INFO_STRING];
	char *ip;
	gentity_t	*ent;

	ent = &g_entities[ clientNum ];

	trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

 	// IP filtering
 	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=500
 	// recommanding PB based IP / GUID banning, the builtin system is pretty limited
 	// check to see if they are on the banned IP list
	ip = Info_ValueForKey (userinfo, "ip");
	if ( G_FilterPacket( ip ) ) {
    G_LogPrintf( "Banned ip \'%s\' tried to connect.\n", ip );
		return "You are banned from this server.";
	}

  // we don't check password for bots and local client
  // NOTE: local client <-> "ip" "localhost"
  //   this means this client is not running in our current process
	if ( !isBot && (strcmp(ip, "localhost") != 0)) {
    ent->client->pers.ip = ip;
		// check for a password
		value = Info_ValueForKey (userinfo, "password");
		if ( g_password.string[0] && Q_stricmp( g_password.string, "none" ) &&
			strcmp( g_password.string, value) != 0) {
      G_LogPrintf( "%s tried to brute force server's password.\n", ip );
			return "Invalid password";
		}
	}

	// they can connect
	ent->client = level.clients + clientNum;
	client = ent->client;

//	areabits = client->areabits;

	memset( client, 0, sizeof(*client) );

	client->pers.connected = CON_CONNECTING;

	// read or initialize the session data
	if ( firstTime || level.newSession ) {
		G_InitSessionData( client, userinfo, isBot );
	}
	G_ReadSessionData( client );

	if( isBot ) {
		ent->r.svFlags |= SVF_BOT;
		ent->inuse = qtrue;
		if( !G_BotConnect( clientNum, !firstTime ) )
			return "BotConnectfailed";

		if(g_gametype.integer == GT_DUEL)
			ent->client->realspec = qfalse;
	}

	// get and distribute relevent paramters
	G_LogPrintf( "ClientConnect: %i\n", clientNum );
	ClientUserinfoChanged( clientNum );

	// don't do the "xxx connected" messages if they were carried over from previous level
	if ( firstTime )
		trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " connected\n\"", client->pers.netname) );

	if ( g_gametype.integer >= GT_TEAM &&
		client->sess.sessionTeam != TEAM_SPECTATOR )
		BroadcastTeamChange( client, -1 );

	// count current clients and rank for scoreboard
	CalculateRanks();

//unlagged - backward reconciliation #5
	// announce it
	if ( g_delagHitscan.integer )
		trap_SendServerCommand( clientNum, "print \"This server is Unlagged: full lag compensation is ON!\n\"" );
	else
		trap_SendServerCommand( clientNum, "print \"This server is Unlagged: full lag compensation is OFF!\n\"" );
//unlagged - backward reconciliation #5

	return NULL;
}

/*
===========
ClientBegin

called when a client has finished connecting, and is ready
to be placed into the level.  This will happen every level load,
and on transition between teams, but doesn't happen on respawns
============
*/
void ClientBegin( int clientNum ) {
	gentity_t	*ent;
	gclient_t	*client;
	vec3_t		viewangles;
	vec3_t		origin;
	int			flags;

	ent = g_entities + clientNum;

	client = level.clients + clientNum;

	if ( ent->r.linked ) {
		trap_UnlinkEntity( ent );
	}
	G_InitGentity( ent );
	ent->touch = 0;
	ent->client = client;

	client->pers.connected = CON_CONNECTED;
	client->pers.enterTime = level.time;
	client->pers.teamState.state = TEAM_BEGIN;

	// save eflags around this, because changing teams will
	// cause this to happen with a valid entity, and we
	// want to make sure the teleport bit is set right
	// so the viewpoint doesn't interpolate through the
	// world to the new position
	VectorCopy(client->ps.viewangles, viewangles);
	VectorCopy(client->ps.origin, origin);

	flags = client->ps.eFlags;
	memset( &client->ps, 0, sizeof( client->ps ) );
	client->ps.eFlags = flags;

	VectorCopy(viewangles, client->ps.viewangles);
	VectorCopy(origin, client->ps.origin);

	// locate ent at a spawn point
	ClientSpawn( ent );

	if( client->sess.sessionTeam < TEAM_SPECTATOR && g_gametype.integer != GT_DUEL )
			trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " entered the game\n\"", client->pers.netname) );

	G_LogPrintf( "ClientBegin: %i\n", clientNum );

	// count current clients and rank for scoreboard
	CalculateRanks();

	//start into game: receive money (by Spoon)
	ent->client->ps.stats[STAT_MONEY] = g_moneyMin.integer;
}

/*
===========
G_RestorePlayerStats
restore the playerstats on Clienspawn
===========
*/
static void G_RestorePlayerStats(gentity_t *ent, qbool save){
	int		i;
	clientPersistant_t	saved;
	clientSession_t		savedSess;
	int		persistant[MAX_PERSISTANT];
	int		savedPing, savedMoney;
	int		accuracy_hits, accuracy_shots;
	int		savedEvents[MAX_PS_EVENTS];
	int		eventSequence;
	qbool	realspec;
	qbool	won;
	gclient_t	*client = ent->client;
	int			wins = client->ps.stats[STAT_WINS];


	// clear everything but the persistant data
	// but only if player didn'T die when playing a round-based-mode
	saved = client->pers;
	savedSess = client->sess;
	savedPing = client->ps.ping;
	savedMoney = client->ps.stats[STAT_MONEY];
	realspec = client->realspec;
	won = client->won;

	accuracy_hits = client->accuracy_hits;
	accuracy_shots = client->accuracy_shots;

	for ( i = 0 ; i < MAX_PERSISTANT ; i++ )
		persistant[i] = client->ps.persistant[i];

	// also save the predictable events otherwise we might get double or dropped events
	for (i = 0; i < MAX_PS_EVENTS; i++)
		savedEvents[i] = client->ps.events[i];

	eventSequence = client->ps.eventSequence;

	// save important data
	if(save){
		int ammo[MAX_WEAPONS];
		int stats[MAX_STATS];
		int powerups[MAX_POWERUPS];
		int weapon = client->ps.weapon;
		int weapon2 = client->ps.weapon2;

		// make copies
		for(i = 0; i < MAX_WEAPONS; i++)
			ammo[i] = client->ps.ammo[i];

		for(i = 0; i < MAX_STATS; i++)
			stats[i] = client->ps.stats[i];

		for(i = 0; i < MAX_POWERUPS; i++)
			powerups[i] = client->ps.powerups[i];

		// reset client
		memset (client, 0, sizeof(*client));

		// use copies
		for(i = 0; i < MAX_WEAPONS; i++)
			client->ps.ammo[i] = ammo[i];

		for(i = 0; i < MAX_STATS; i++)
			client->ps.stats[i] = stats[i];

		for(i = 0; i < MAX_POWERUPS; i++)
			client->ps.powerups[i] = powerups[i];

		client->ps.weapon = weapon;
		client->ps.weapon2 = weapon2;
	}
  else 
		memset (client, 0, sizeof(*client));

	client->pers = saved;
	client->sess = savedSess;
	client->ps.ping = savedPing;
	client->ps.stats[STAT_MONEY] = savedMoney;
	client->accuracy_hits = accuracy_hits;
	client->accuracy_shots = accuracy_shots;
	client->realspec = realspec;
	client->won = won;
	client->ps.stats[STAT_WINS] = wins;

	for ( i = 0 ; i < MAX_PERSISTANT ; i++ )
		client->ps.persistant[i] = persistant[i];

	for (i = 0; i < MAX_PS_EVENTS; i++)
		client->ps.events[i] = savedEvents[i];

	client->ps.eventSequence = eventSequence;
}

/*
===========
ClientSpawn

Called every time a client is placed fresh in the world:
after the first ClientBegin, and after each respawn
Initializes all non-persistant parts of playerState
============
*/
void ClientSpawn(gentity_t *ent) {
	int		index;
	vec3_t	spawn_origin, spawn_angles;
	gclient_t	*client;
	int		i;
	gentity_t	*spawnPoint;
	char		userinfo[MAX_INFO_STRING];
	int		flags;
	int		min_money = 0;
	qbool	player_died;
	qbool	specwatch = ent->client->specwatch;

	index = ent - g_entities;
	client = ent->client;
	if (g_gametype.integer != GT_DUEL)
		// hika comments: client->ps.pm_type is set to PM_DEAD in player_die() at g_combat.c
		player_died = client->player_died || (client->ps.pm_type & PM_DEAD);
	else
		player_died = client->player_died;

	if(!ent->mappart && g_gametype.integer == GT_DUEL)
		ent->mappart = rand()%g_maxmapparts+1;

	VectorClear(spawn_origin);

	// find a spawn point
	// do it before setting health back up, so farthest
	// ranging doesn't count this client
	if ( client->sess.sessionTeam == TEAM_SPECTATOR && (g_gametype.integer != GT_DUEL ||
		client->realspec)) {
		spawnPoint = SelectSpectatorSpawnPoint (
						spawn_origin, spawn_angles, ent->mappart);
	} else if (g_gametype.integer >= GT_RTP ) {
		if (client->sess.sessionTeam >= TEAM_SPECTATOR) {
			spawnPoint = NULL;
			VectorCopy(ent->client->ps.viewangles, spawn_angles);
			VectorCopy(ent->client->ps.origin, spawn_origin);
		} else {
			spawnPoint = SelectCTFSpawnPoint(
				client->sess.sessionTeam,
				client->pers.teamState.state,
				spawn_origin, spawn_angles,
				!!(ent->r.svFlags & SVF_BOT));
		}
	} else if (g_gametype.integer == GT_DUEL){
		if (client->sess.sessionTeam == TEAM_SPECTATOR) {
			spawnPoint = NULL;
			VectorCopy(ent->client->ps.viewangles, spawn_angles);
			VectorCopy(ent->client->ps.origin, spawn_origin);
		} else {
			spawnPoint = SelectDuelSpawnPoint (ent->mappart, ent->client->trio,
						spawn_origin, spawn_angles);
		}
	} else {
		// the first spawn should be at a good looking spot
		if ( !client->pers.initialSpawn && client->pers.localClient ) {
			client->pers.initialSpawn = qtrue;
			spawnPoint = SelectInitialSpawnPoint(spawn_origin, spawn_angles, ent->client->ps.persistant[PERS_TEAM],
				!!(ent->r.svFlags & SVF_BOT));
		} else {
			// don't spawn near existing origin if possible
			spawnPoint = SelectSpawnPoint (
				client->ps.origin,
				spawn_origin, spawn_angles, ent->client->ps.persistant[PERS_TEAM],
				!!(ent->r.svFlags & SVF_BOT));
		}
	}
	client->pers.teamState.state = TEAM_ACTIVE;

	// always clear the kamikaze flag
	ent->s.eFlags &= ~EF_KAMIKAZE;

	// toggle the teleport bit so the client knows to not lerp
	// and never clear the voted flag
	flags = ent->client->ps.eFlags & (EF_TELEPORT_BIT | EF_VOTED | EF_TEAMVOTED);
	flags ^= EF_TELEPORT_BIT;

//unlagged - backward reconciliation #3
	// we don't want players being backward-reconciled to the place they died
	G_ResetHistory( ent );
	// and this is as good a time as any to clear the saved state
	ent->client->saved.leveltime = 0;
//unlagged - backward reconciliation #3

	if((g_gametype.integer < GT_RTP && g_gametype.integer != GT_DUEL)|| player_died){
		G_RestorePlayerStats(ent, qfalse);
	} else {
		// player stats stay(weapons, ammo, items, etc)
		VectorClear(client->ps.velocity);
		client->ps.stats[STAT_OLDWEAPON] = 0;
		client->ps.stats[STAT_WP_MODE] = 0;
		client->ps.stats[STAT_GATLING_MODE] = 0;
	}

	//goldbags will be deleted in every case
	client->ps.powerups[PW_GOLD] = 0;

	// increment the spawncount so the client will detect the respawn
	client->ps.persistant[PERS_SPAWN_COUNT]++;
	client->ps.persistant[PERS_TEAM] = client->sess.sessionTeam;

	client->airOutTime = level.time + 12000;

	trap_GetUserinfo( index, userinfo, sizeof(userinfo) );
	// set max health
	client->pers.maxHealth = atoi( Info_ValueForKey( userinfo, "handicap" ) );
	if ( client->pers.maxHealth < 1 || client->pers.maxHealth > 100 )
		client->pers.maxHealth = 100;

	// clear entity values
	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;
	client->ps.eFlags = flags;

	for(i=DM_HEAD_1;i<=DM_LEGS_2;i++)
		client->ps.powerups[i] = 0;

	ent->s.groundEntityNum = ENTITYNUM_NONE;
	ent->client = &level.clients[index];
	ent->takedamage = qtrue;
	ent->inuse = qtrue;
	ent->classname = "player";
	ent->r.contents = CONTENTS_BODY;
	ent->clipmask = MASK_PLAYERSOLID;
	ent->die = player_die;
	ent->waterlevel = 0;
	ent->watertype = 0;
	ent->flags = 0;
	ent->client->maxAmmo = 1.0f;

	VectorCopy (playerMins, ent->r.mins);
	VectorCopy (playerMaxs, ent->r.maxs);

	client->ps.clientNum = index;

	//weapons
	if((g_gametype.integer < GT_RTP && g_gametype.integer != GT_DUEL) || player_died){
		client->ps.stats[STAT_WEAPONS] = ( 1 << WP_REM58 );
		client->ps.ammo[WP_REM58] = bg_weaponlist[WP_REM58].clipAmmo;
		client->ps.ammo[WP_BULLETS_CLIP] = bg_weaponlist[WP_REM58].maxAmmo;

		client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_KNIFE );
		client->ps.ammo[WP_KNIFE] = 1;
	} else {
		int i;

		// fill up ammo in weapon
		for ( i=WP_REM58; i<WP_DYNAMITE; i++){
			if(client->ps.stats[STAT_WEAPONS] & (1<<i)){
				client->ps.ammo[i] = bg_weaponlist[i].clipAmmo;

				// also fill up clip in duel mode
				if(g_gametype.integer == GT_DUEL)
					client->ps.ammo[bg_weaponlist[i].clip] = bg_weaponlist[i].maxAmmo;

				if(client->ps.ammo[bg_weaponlist[i].clip] < bg_weaponlist[i].maxAmmo/2 &&
					bg_weaponlist[i].clip)
					client->ps.ammo[bg_weaponlist[i].clip] = bg_weaponlist[i].maxAmmo/2;
			}
		}

		if(client->ps.stats[STAT_FLAGS] & SF_SEC_PISTOL){
			client->ps.ammo[WP_AKIMBO] = bg_weaponlist[WP_PEACEMAKER].clipAmmo;
		}
	}

	// give dynamite in BR
	if(g_gametype.integer == GT_BR && client->sess.sessionTeam == g_robteam){
		if(client->ps.ammo[WP_DYNAMITE] < 2)
			client->ps.ammo[WP_DYNAMITE] = 2;
	}

	ent->health = client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH];

	G_SetOrigin( ent, spawn_origin );
	VectorCopy( spawn_origin, client->ps.origin );

	// the respawned flag will be cleared after the attack and jump keys come up
	client->ps.pm_flags |= PMF_RESPAWNED;

	trap_GetUsercmd( client - level.clients, &ent->client->pers.cmd );
	SetClientViewAngle( ent, spawn_angles );

	if ( ent->client->sess.sessionTeam >= TEAM_SPECTATOR ) {
	} else {
		if ( g_gametype.integer == GT_DUEL )  {
			min_money = g_moneyDUMin.integer;
		}
		else  {
			if (g_moneyRespawnMode.integer == 1) {
				// Joe Kari: new minimum money at respawn formula //
				if ((g_moneyRespawnMode.integer == GT_BR) || (g_gametype.integer == GT_RTP)) {
					min_money = g_moneyMin.integer + (client->ps.stats[STAT_MONEY] - g_moneyTeamLose.integer) / 4;
				} else {
					min_money = g_moneyMin.integer + client->ps.stats[STAT_MONEY] / 4;
				}
			}
			if ( min_money < g_moneyMin.integer ) {
				// former minimum money at respawn formula : //
				min_money = g_moneyMin.integer;
			}
		}
		
		// don't apply this in duel mode
		if(g_gametype.integer != GT_DUEL){
			// make sure the player doesn't interfere with KillBox
			trap_UnlinkEntity (ent);
			G_KillBox( ent );
			trap_LinkEntity (ent);
		}

		// force the base weapon up
		if(g_gametype.integer == GT_DUEL){
			// in duel mode the player has to draw
			client->ps.weapon = WP_NONE;
			client->ps.weapon2 = 0;
			client->pers.cmd.weapon = WP_NONE;
		} else if(g_gametype.integer < GT_RTP || player_died){
			client->ps.weapon = WP_REM58;
			client->ps.weapon2 = 0;
		}

		if(client->ps.stats[STAT_MONEY] < min_money)
			client->ps.stats[STAT_MONEY] = min_money;

		client->ps.weaponstate = WEAPON_READY;
		//do idle animation
		client->ps.weaponAnim = WP_ANIM_IDLE;
		client->ps.weapon2Anim = WP_ANIM_IDLE;
	}

	// don't allow full run speed for a bit
	client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	client->ps.pm_time = 100;

	client->respawnTime = level.time;
	client->inactivityTime = level.time + g_inactivity.integer * 1000;
	client->latched_buttons = 0;

	// set default animations
	client->ps.torsoAnim = TORSO_PISTOL_STAND;
	client->ps.legsAnim = LEGS_IDLE;

	if ( level.intermissiontime ) {
		MoveClientToIntermission( ent );
	} else {
		// fire the targets of the spawn point
		if(spawnPoint)
			G_UseTargets( spawnPoint, ent );

		// select the highest weapon number available, after any
		// spawn given items have fired
	}

	// run a client frame to drop exactly to the floor,
	// initialize animations and other things
	client->ps.commandTime = level.time - 100;
	ent->client->pers.cmd.serverTime = level.time;
	ClientThink( ent-g_entities );

	// positively link the client, even if the command times are weird
	if ( ent->client->sess.sessionTeam < TEAM_SPECTATOR) {
		BG_PlayerStateToEntityState( &client->ps, &ent->s, qtrue );
		VectorCopy( ent->client->ps.origin, ent->r.currentOrigin );
		trap_LinkEntity( ent );
	}

	client->player_died = qtrue;
	client->specwatch = specwatch;

	// run the presend to set anything else
	ClientEndFrame( ent );

	// clear entity state values
	BG_PlayerStateToEntityState( &client->ps, &ent->s, qtrue );

	if(g_gametype.integer == GT_DUEL && client->sess.sessionTeam == TEAM_SPECTATOR)
		client->specmappart = ent->mappart;

	// remove this in every case
	client->ps.pm_flags &= ~PMF_FOLLOW;
	client->ps.pm_flags &= ~PMF_SUICIDE;

  G_LogPrintf( "ClientSpawn: %s%s spawns\n",
      client->pers.netname, vtos(ent->s.origin) );
}


/*
===========
ClientDisconnect

Called when a player drops from the server.
Will not be called between levels.

This should NOT be called directly by any game logic,
call trap_DropClient(), which will call this and do
server system housekeeping.
============
*/
void ClientDisconnect( int clientNum ) {
	gentity_t	*ent;
	gentity_t	*tent;
	int			i;

	// cleanup if we are kicking a bot that
	// hasn't spawned yet
	G_RemoveQueuedBotBegin( clientNum );

	ent = g_entities + clientNum;
	if ( !ent->client )
		return;

	// stop any following clients
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].sess.sessionTeam >= TEAM_SPECTATOR
			&& ( level.clients[i].sess.spectatorState == SPECTATOR_FOLLOW
			|| level.clients[i].sess.spectatorState == SPECTATOR_CHASECAM
			|| level.clients[i].sess.spectatorState == SPECTATOR_FIXEDCAM )
			&& level.clients[i].sess.spectatorClient == clientNum ) {
			StopFollowing( &g_entities[i] );
		}
	}

	// send effect if they were completely connected
	if ( ent->client->pers.connected == CON_CONNECTED
		&& ent->client->sess.sessionTeam < TEAM_SPECTATOR ) {
		tent = G_TempEntity( ent->client->ps.origin, EV_PLAYER_TELEPORT_OUT );
		tent->s.clientNum = ent->s.clientNum;

		// They don't get to take powerups with them!
		// Especially important for stuff like CTF flags
		TossClientItems( ent );
	}

	G_LogPrintf( "ClientDisconnect: %i\n", clientNum );

	trap_UnlinkEntity (ent);
	ent->s.modelindex = 0;
	ent->inuse = qfalse;
	ent->classname = "disconnected";
	ent->client->pers.connected = CON_DISCONNECTED;
	ent->client->ps.persistant[PERS_TEAM] = TEAM_FREE;
	ent->client->sess.sessionTeam = TEAM_FREE;

	trap_SetConfigstring( CS_PLAYERS + clientNum, "");

	CalculateRanks();

	if ( ent->r.svFlags & SVF_BOT )
		BotAIShutdownClient( clientNum, qfalse );
}


