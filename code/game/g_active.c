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


/*
===============
G_DamageFeedback

Called just before a snapshot is sent to the given player.
Totals up all damage and generates both the player_state_t
damage values to that client for pain blends and kicks, and
global pain sound events for all clients.
===============
*/
void P_DamageFeedback( gentity_t *player ) {
	gclient_t	*client;
	float	count;
	vec3_t	angles;

	client = player->client;
	if ( client->ps.pm_type == PM_DEAD ) {
		return;
	}

	// total points of damage shot at the player this frame
	count = client->damage_blood + client->damage_armor;
	if ( count == 0 ) {
		return;		// didn't take any damage
	}

	if ( count > 255 ) {
		count = 255;
	}

	// send the information to the client

	// world damage (falling, slime, etc) uses a special code
	// to make the blend blob centered instead of positional
	if ( client->damage_fromWorld ) {
		client->ps.damagePitch = 255;
		client->ps.damageYaw = 255;

		client->damage_fromWorld = qfalse;
	} else {
		vectoangles( client->damage_from, angles );
		client->ps.damagePitch = angles[PITCH]/360.0 * 256;
		client->ps.damageYaw = angles[YAW]/360.0 * 256;
	}

	// play an apropriate pain sound
	if ( (level.time > player->pain_debounce_time) && !(player->flags & FL_GODMODE) ) {
		player->pain_debounce_time = level.time + 700;
		G_AddEvent( player, EV_PAIN, player->health );
		client->ps.damageEvent++;
	}


	client->ps.damageCount = count;

	//
	// clear totals
	//
	client->damage_blood = 0;
	client->damage_armor = 0;
}



/*
=============
P_WorldEffects

Check for lava / slime contents and drowning
=============
*/
void P_WorldEffects( gentity_t *ent ) {
	qbool	envirosuit;
	int			waterlevel;

	if ( ent->client->noclip ) {
		ent->client->airOutTime = level.time + 12000;	// don't need air
		return;
	}

	waterlevel = ent->waterlevel;

	envirosuit = 0;

	//
	// check for drowning
	//
	if ( waterlevel == 3 ) {
		// envirosuit give air
		if ( envirosuit ) {
			ent->client->airOutTime = level.time + 10000;
		}

		// if out of air, start drowning
		if ( ent->client->airOutTime < level.time) {
			// drown!
			ent->client->airOutTime += 1000;
			if ( ent->health > 0 ) {
				// take more damage the longer underwater
				ent->damage += 2;
				if (ent->damage > 15)
					ent->damage = 15;

				// play a gurp sound instead of a normal pain sound
				if (ent->health <= ent->damage) {
					G_Sound(ent, CHAN_VOICE, G_SoundIndex("*drown.wav"));
				} else if (rand()&1) {
					G_Sound(ent, CHAN_VOICE, G_SoundIndex("sound/player/gurp1.wav"));
				} else {
					G_Sound(ent, CHAN_VOICE, G_SoundIndex("sound/player/gurp2.wav"));
				}

				// don't play a normal pain sound
				ent->pain_debounce_time = level.time + 200;

				G_Damage(ent, NULL, NULL, NULL, NULL,
					ent->damage, 0, MOD_WATER);
			}
		}
	} else {
		ent->client->airOutTime = level.time + 12000;
		ent->damage = 2;
	}

	//
	// check for sizzle damage (move to pmove?)
	//
	if (waterlevel &&
		(ent->watertype&(CONTENTS_LAVA|CONTENTS_SLIME)) ) {
		if (ent->health > 0
			&& ent->pain_debounce_time <= level.time	) {

			if ( envirosuit ) {
				G_AddEvent( ent, EV_POWERUP_BATTLESUIT, 0 );
			} else {
				if (ent->watertype & CONTENTS_LAVA) {
					G_Damage (ent, NULL, NULL, NULL, NULL,
						30*waterlevel, 0, MOD_LAVA);
				}

				if (ent->watertype & CONTENTS_SLIME) {
					G_Damage (ent, NULL, NULL, NULL, NULL,
						10*waterlevel, 0, MOD_SLIME);
				}
			}
		}
	}
}



/*
===============
G_SetClientSound
===============
*/
void G_SetClientSound( gentity_t *ent ) {
	if (ent->waterlevel && (ent->watertype&(CONTENTS_LAVA|CONTENTS_SLIME)) ) {
		ent->client->ps.loopSound = level.snd_fry;
	} else {
		ent->client->ps.loopSound = 0;
	}
}



//==============================================================

/*
==============
ClientImpacts
==============
*/
void ClientImpacts( gentity_t *ent, pmove_t *pm ) {
	int		i, j;
	trace_t	trace;
	gentity_t	*other;

	memset( &trace, 0, sizeof( trace ) );
	for (i=0 ; i<pm->numtouch ; i++) {
		for (j=0 ; j<i ; j++) {
			if (pm->touchents[j] == pm->touchents[i] ) {
				break;
			}
		}
		if (j != i) {
			continue;	// duplicated
		}
		other = &g_entities[ pm->touchents[i] ];

		if ( ( ent->r.svFlags & SVF_BOT ) && ( ent->touch ) ) {
			ent->touch( ent, other, &trace );
		}

		if ( !other->touch ) {
			continue;
		}

		other->touch( other, ent, &trace );
	}

}

/*
============
G_TouchTriggers

Find all trigger entities that ent's current position touches.
Spectators will only interact with teleporters.
============
*/
void G_TouchTriggers( gentity_t *ent ) {
	int			i, num;
	int			touch[MAX_GENTITIES];
	gentity_t	*hit;
	trace_t		trace;
	vec3_t		mins, maxs;
	static vec3_t	range = {37, 37, 52};

	if ( !ent->client ) {
		return;
	}

	// dead clients don't activate triggers!
	if ( ent->client->ps.stats[STAT_HEALTH] <= 0 ) {
		return;
	}

	VectorSubtract( ent->client->ps.origin, range, mins );
	VectorAdd( ent->client->ps.origin, range, maxs );

	num = trap_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	// can't use ent->absmin, because that has a one unit pad
	VectorAdd( ent->client->ps.origin, ent->r.mins, mins );
	VectorAdd( ent->client->ps.origin, ent->r.maxs, maxs );

	for ( i=0 ; i<num ; i++ ) {
		hit = &g_entities[touch[i]];

		if ( !hit->touch && !ent->touch &&
			!(hit->s.eType == ET_MOVER && hit->s.angles2[0] == -1000)) {
			continue;
		}
		if ( !( hit->r.contents & CONTENTS_TRIGGER ) && !( hit->r.contents & CONTENTS_TRIGGER2 ) &&
			!(hit->s.eType == ET_MOVER && hit->s.angles2[0] == -1000) &&
			hit->s.eType != ET_ESCAPE) {
			continue;
		}

		// ignore most entities if a spectator
		if ( ent->client->sess.sessionTeam >= TEAM_SPECTATOR ) {
			if ( hit->s.eType != ET_TELEPORT_TRIGGER &&
				// this is ugly but adding a new ET_? type will
				// most likely cause network incompatibilities
				hit->touch != Touch_DoorTrigger) {
				continue;
			}
		}

		// use seperate code for determining if an item is picked up
		// so you don't have to actually contact its bounding box
		if ( hit->s.eType == ET_ITEM ) {
			if(hit->item->giType == IT_POWERUP && hit->item->giTag == PW_GOLD){
				float dist1, dist2;
				trace_t		tr;
				vec3_t		muzzle;

				VectorCopy( ent->client->ps.origin, muzzle );
				muzzle[2] = hit->r.currentOrigin[2];

				//find the nearest obstacle
				trap_Trace_New (&tr, muzzle, NULL, NULL, hit->r.currentOrigin, ent->client->ps.clientNum, MASK_SHOT);

				dist1 = Distance(muzzle, tr.endpos);
				dist2 = Distance(muzzle, hit->r.currentOrigin);

				if(dist2>dist1)
					continue;
			}

			if ( !BG_PlayerTouchesItem( &ent->client->ps, &hit->s, level.time ) ) {
				continue;
			}

		} else if(hit->s.eType == ET_TURRET){

			if(!(ent->client->buttons & BUTTON_ACTIVATE) ||
				(ent->client->oldbuttons & BUTTON_ACTIVATE))
				continue;

			//turret is already being used
			if(hit->s.eventParm != -1){
				continue;
			}

			//if player already uses a gatling
			if(ent->client->ps.stats[STAT_GATLING_MODE])
				continue;

			//Gatling is being built or dismantled
			if(hit->s.time2){
				continue;
			}

			if(!G_CanEntityBeActivated(ent, hit, qfalse))
				continue;

			// if player already carries a gatling add a flag
			if(ent->client->ps.stats[STAT_WEAPONS] & (1 << WP_GATLING)){
				ent->client->ps.stats[STAT_FLAGS] |= SF_GAT_CARRY;
			}

			// let gatling gun know, by whom it is used
			hit->s.eventParm = ent->client->ps.clientNum;

			// player uses gatling gun
			ent->client->ps.stats[STAT_GATLING_MODE] = 1;
			VectorCopy(hit->r.currentOrigin, ent->client->ps.grapplePoint);

			ent->client->ps.stats[STAT_OLDWEAPON] = ent->client->ps.weapon;
			ent->client->ps.stats[STAT_WEAPONS] |= (1 << WP_GATLING);
			ent->client->pers.cmd.weapon = WP_GATLING;
			//ent->client->ps.weapon = WP_GATLING;
			G_AddEvent( ent, EV_CHANGE_TO_WEAPON, WP_GATLING);
			ent->client->ps.ammo[WP_GATLING]= hit->count;
			continue;

		} else if(hit->s.eType == ET_MOVER && hit->s.angles2[0] == -1000){

			if(!hit->use)
				continue;

			if((!(ent->client->buttons & BUTTON_ACTIVATE) ||
				(ent->client->oldbuttons & BUTTON_ACTIVATE)) &&

				(!(ent->r.svFlags & SVF_BOT) ||
				hit->moverState != MOVER_POS1) ) // bots dont need to press down activate
				continue;

			if(hit->s.apos.trType != TR_STATIONARY ||
				hit->s.pos.trType != TR_STATIONARY)
				continue;

			if(!G_CanEntityBeActivated(ent, hit, qtrue))
				continue;

			hit->use(hit, NULL, ent);
		} else {
			if ( !trap_EntityContact( mins, maxs, hit ) ) {
				continue;
			}
		}

		memset( &trace, 0, sizeof(trace) );

		if ( hit->touch ) {
			hit->touch (hit, ent, &trace);
		}

		if ( ( ent->r.svFlags & SVF_BOT ) && ( ent->touch ) ) {
			ent->touch( ent, hit, &trace );
		}
	}
}

static void G_TouchSpawnBuyPoint(gentity_t *ent) {
	int			i;
	const char	*classname;

	if (g_gametype.integer != GT_RTP && g_gametype.integer != GT_BR) {
		return;
	}

	if (!ent->client) {
		return;
	}

	if (ent->client->sess.sessionTeam >= TEAM_SPECTATOR) {
		return;
	}

	// dead clients don't activate triggers!
	if (ent->client->ps.stats[STAT_HEALTH] <= 0) {
		return;
	}

	//get starpoint classname
	if (ent->client->sess.sessionTeam == g_robteam)
		classname = "team_CTF_blueplayer";
	else if (ent->client->sess.sessionTeam == g_defendteam)
		classname = "team_CTF_redplayer";

	ent->client->ps.stats[STAT_FLAGS] &= ~SF_CANBUY;

	for (i=0; i<MAX_GENTITIES; i++) {
		gentity_t	*hit = &g_entities[i];

		if (!hit->inuse)
			continue;

		if (!Q_stricmp(hit->classname, classname)) {
			vec3_t distance;

			VectorSubtract(ent->client->ps.origin, hit->r.currentOrigin, distance);
			if (VectorLengthSquared(distance) > MAX_BUY_DISTANCE_SQUARE)
				continue;

			ent->client->ps.stats[STAT_FLAGS] |= SF_CANBUY;
				return;
		}
	}
}

/*
=================
SpectatorThink
=================
*/
void SpectatorThink( gentity_t *ent, usercmd_t *ucmd ) {
	pmove_t	pm;
	gclient_t	*client;

	client = ent->client;

	if(ent->client->sess.spectatorState == SPECTATOR_CHASECAM
		|| ent->client->sess.spectatorState == SPECTATOR_FIXEDCAM
		|| ent->client->sess.spectatorState == SPECTATOR_FOLLOW){

		if (ent->client->sess.spectatorState != SPECTATOR_FOLLOW) {
			client->ps.pm_type = PM_CHASECAM;
			client->ps.stats[CHASECLIENT] = client->sess.spectatorClient;
			client->ps.stats[STAT_HEALTH] = level.clients[client->sess.spectatorClient].ps.stats[STAT_HEALTH];
			client->ps.stats[STAT_FLAGS] &= ~SF_DU_INTRO;

			if(ent->client->sess.spectatorState == SPECTATOR_FIXEDCAM) {
				VectorCopy(level.clients[client->sess.spectatorClient].ps.viewangles, client->ps.viewangles);
				VectorCopy(level.clients[client->sess.spectatorClient].ps.delta_angles, client->ps.delta_angles);
			}
		}

		client->ps.pm_flags |= PMF_FOLLOW;
		VectorCopy(level.clients[client->sess.spectatorClient].ps.origin, client->ps.origin);

		if (g_gametype.integer == GT_DUEL && client->realspec
				&& ent->client->specmappart != g_entities[client->sess.spectatorClient].mappart) {
			ent->client->specmappart = g_entities[client->sess.spectatorClient].mappart;
			ent->mappart = ent->client->specmappart;
		}

		if(level.clients[client->sess.spectatorClient].sess.sessionTeam >= TEAM_SPECTATOR){
			if(TeamCount( -1, ent->client->sess.sessionTeam - 3) && g_gametype.integer >= GT_RTP
				&& g_chaseonly.integer && ent->client->sess.sessionTeam != TEAM_SPECTATOR){
				ent->client->sess.spectatorState = SPECTATOR_FREE;
				ent->client->ps.pm_type = PM_FREEZE;
			} else if ( !(g_gametype.integer == GT_DUEL &&
				               !level.clients[client->sess.spectatorClient].realspec) )
				StopFollowing(ent);
		}

	} else {
		client->ps.pm_type = PM_SPECTATOR;
		client->ps.pm_flags &= ~PMF_FOLLOW;
	}

	if ( client->sess.spectatorState != SPECTATOR_FOLLOW ) {
		client->ps.speed = 400;	// faster than normal

		if(ent->r.svFlags & SVF_BOT)
			client->ps.stats[STAT_FLAGS] |= SF_BOT;

		// set up for pmove
		memset (&pm, 0, sizeof(pm));
		pm.ps = &client->ps;
		pm.cmd = *ucmd;
		pm.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;	// spectators can fly through bodies
		pm.trace = trap_Trace_New;
		pm.pointcontents = trap_PointContents;

		// perform a pmove
		Pmove (&pm);
		// save results of pmove
		VectorCopy( client->ps.origin, ent->s.origin );

		G_TouchTriggers( ent );
	}

	// "fly" is unlinked normally
	if(!g_specsareflies.integer || ent->client->sess.spectatorState != SPECTATOR_FREE ||
		(ent->r.svFlags & SVF_BOT))
		trap_UnlinkEntity( ent );
	else {
		trap_LinkEntity(ent);

		ent->r.contents = 0;
		ent->takedamage = qfalse;

		if (g_smoothClients.integer) {
			BG_PlayerStateToEntityStateExtraPolate( &ent->client->ps, &ent->s, ent->client->ps.commandTime, qtrue );
		}
		else {
			BG_PlayerStateToEntityState( &ent->client->ps, &ent->s, qtrue );
		}
	}

	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;

	// chasecam may lead to bugs in dll-builds
	// attack button cycles through spectators
	if ( g_gametype.integer >= GT_RTP && g_chaseonly.integer){

		if(ent->client->sess.spectatorState != SPECTATOR_CHASECAM &&
			ent->client->sess.spectatorState != SPECTATOR_FIXEDCAM &&
			ent->client->sess.spectatorState != SPECTATOR_FOLLOW) {
			ChaseCam_Start(ent, 1);
		}
		if (( client->buttons & BUTTON_ATTACK ) && ! ( client->oldbuttons & BUTTON_ATTACK )) {
			ChaseCam_Start(ent, 1);
		}
		if (( client->buttons & BUTTON_ALT_ATTACK ) && ! ( client->oldbuttons & BUTTON_ALT_ATTACK )) {
			ChaseCam_Start(ent, -1);
		}
		if (ucmd->upmove > 0 && !(client->oldupmove > 0)) {
			if (client->sess.spectatorState == SPECTATOR_FOLLOW) {
				StopFollowing(ent);
				client->sess.spectatorState = SPECTATOR_CHASECAM;
				ChaseCam_Start(ent, 0);
			}
			else if (client->sess.spectatorState == SPECTATOR_CHASECAM) {
				StopFollowing(ent);
				client->sess.spectatorState = SPECTATOR_FOLLOW;
				ChaseCam_Start(ent, 0);
			}
		}

	} else {

		if ( (client->buttons & BUTTON_ATTACK) && ! (client->oldbuttons & BUTTON_ATTACK)
			    && !(g_gametype.integer == GT_DUEL && !client->realspec) ) {
			ChaseCam_Start(ent, 1);
		}


		if (( client->buttons & BUTTON_ALT_ATTACK ) && ! ( client->oldbuttons & BUTTON_ALT_ATTACK ) &&
			du_nextroundstart < level.time) {

			// in duel mode, this switches through the mapparts
			if(g_gametype.integer == GT_DUEL){
				qbool realspec = client->realspec;
				int save_specmappart;
				int startpart;

				if(!ent->client->specmappart)
					ent->client->specmappart = ent->mappart;

				startpart = ent->client->specmappart;

				do {
					ent->client->specmappart++;

					if(ent->client->specmappart > g_maxmapparts)
						ent->client->specmappart = 1;

				} while(!du_mapparts[ent->client->specmappart-1].inuse &&
					level.warmupTime != -1 &&
					startpart != ent->client->specmappart);

				if(startpart == ent->client->specmappart)
					return;

				// stop chasecam if nessecary
				StopFollowing(ent);

				// has to be done, otherwise no "info_player_intermission" would be used
				client->realspec = qtrue;
				ent->mappart = ent->client->specmappart;
				save_specmappart = ent->client->specmappart;
				client->player_died = qfalse;

				ClientSpawn(ent);

				ent->client->specmappart = save_specmappart;
				client->realspec = realspec;
				client->player_died = qfalse;
			}
			else
				ChaseCam_Start(ent, -1);
		}

		if (ucmd->upmove > 0  && !(client->oldupmove > 0)) {
			if (client->sess.spectatorState == SPECTATOR_FOLLOW) {
				StopFollowing(ent);
				client->sess.spectatorState = SPECTATOR_CHASECAM;
				ChaseCam_Start(ent, 0);
			} else if (client->sess.spectatorState != SPECTATOR_FREE) {
				StopFollowing(ent);
				pm.cmd.upmove = 0;
			}
		}
	}
	client->oldupmove = ucmd->upmove;
}



/*
=================
ClientInactivityTimer

Returns qfalse if the client is dropped
=================
*/
qbool ClientInactivityTimer( gclient_t *client ) {
	if ( ! g_inactivity.integer ) {
		// give everyone some time, so if the operator sets g_inactivity during
		// gameplay, everyone isn't kicked
		client->inactivityTime = level.time + 60 * 1000;
		client->inactivityWarning = qfalse;
	} else if ( client->pers.cmd.forwardmove ||
		client->pers.cmd.rightmove ||
		client->pers.cmd.upmove ||
		(client->pers.cmd.buttons & BUTTON_ATTACK) ) {
		client->inactivityTime = level.time + g_inactivity.integer * 1000;
		client->inactivityWarning = qfalse;
	} else if ( !client->pers.localClient ) {
		if ( level.time > client->inactivityTime ) {
			trap_DropClient( client - level.clients, "Dropped due to inactivity" );
			return qfalse;
		}
		if ( level.time > client->inactivityTime - 10000 && !client->inactivityWarning ) {
			client->inactivityWarning = qtrue;
			trap_SendServerCommand( client - level.clients, "cp \"Ten seconds until inactivity drop!\n\"" );
		}
	}
	return qtrue;
}

/*
==================
ClientTimerActions

Actions that happen once a second
==================
*/
void ClientTimerActions( gentity_t *ent, int msec ) {
	gclient_t	*client;

	client = ent->client;
	client->timeResidual += msec;

	while ( client->timeResidual >= 1000 ) {
		client->timeResidual -= 1000;
	}
}

/*
====================
ClientIntermissionThink
====================
*/
void ClientIntermissionThink( gclient_t *client ) {
	client->ps.eFlags &= ~EF_TALK;
	client->ps.eFlags &= ~EF_FIRING;

	// the level will exit when everyone wants to or after timeouts

	// swap and latch button actions
	client->oldbuttons = client->buttons;
	client->buttons = client->pers.cmd.buttons;
	if ( client->buttons & ( BUTTON_ATTACK | BUTTON_USE_HOLDABLE ) & ( client->oldbuttons ^ client->buttons ) ) {
		// this used to be an ^1 but once a player says ready, it should stick
		client->readyToExit = 1;
	}
}


/*
================
ClientEvents

Events will be passed on to the clients for presentation,
but any server game effects are handled here
================
*/
void ClientEvents( gentity_t *ent, int oldEventSequence ) {
	int		i, j;
	int		event;
	gclient_t *client;
	int		damage;
	vec3_t	dir;
	vec3_t	origin, angles;
//	qbool	fired;
	gitem_t *item;

	client = ent->client;

	if ( oldEventSequence < client->ps.eventSequence - MAX_PS_EVENTS ) {
		oldEventSequence = client->ps.eventSequence - MAX_PS_EVENTS;
	}
	for ( i = oldEventSequence ; i < client->ps.eventSequence ; i++ ) {
		event = client->ps.events[ i & (MAX_PS_EVENTS-1) ];

		switch ( event ) {
		case EV_FALL_MEDIUM:
		case EV_FALL_VERY_FAR:
		case EV_FALL_FAR:
			if ( ent->s.eType != ET_PLAYER ) {
				break;		// not in the player model
			}
			if ( g_dmflags.integer & DF_NO_FALLING ) {
				break;
			}
			if ( event == EV_FALL_VERY_FAR ) {
				damage = 80 + rand()%5;
			} else if ( event == EV_FALL_FAR ) {
				damage = 50 + (rand()%10) - 5;
			} else if ( event == EV_FALL_MEDIUM ) {
				damage = 35 + rand()%2;
			}

			VectorSet (dir, 0, 0, 1);
			ent->pain_debounce_time = level.time + 200;	// no normal pain sound
			G_Damage(ent, NULL, NULL, NULL, NULL, damage, 0, MOD_FALLING);
			ent->client->ps.powerups[DM_LEGS_2] = (ent->health < HEALTH_INJURED) ? 1 : 0;
			ent->client->ps.powerups[DM_LEGS_1] = (ent->health < HEALTH_USUAL) ? 1 : 0;
			break;

		case EV_FIRE_WEAPON_DELAY:
		case EV_FIRE_WEAPON:
			if(ent->client->ps.stats[STAT_DELAY_TIME])
				break;

			FireWeapon( ent, qfalse, ent->client->ps.weapon);
			break;

		case EV_FIRE_WEAPON2:
			if(ent->client->ps.stats[STAT_DELAY_TIME])
				break;

			FireWeapon( ent, qfalse, ent->client->ps.weapon2);
			break;

		case EV_ALT_FIRE_WEAPON:
			if(ent->client->ps.stats[STAT_DELAY_TIME] && ent->client->ps.weapon == WP_KNIFE)
				break;

			FireWeapon( ent, qtrue, ent->client->ps.weapon);
			break;

		case EV_ALT_WEAPON_MODE:
			break;

		case EV_BUILD_TURRET:
			G_GatlingBuildUp(ent);
			break;

		case EV_USE_ITEM1:		// teleporter
			// drop flags in CTF
			item = NULL;
			j = 0;

			SelectSpawnPoint( ent->client->ps.origin, origin, angles, ent->client->ps.persistant[PERS_TEAM], qfalse );
			TeleportPlayer( ent, origin, angles );
			break;

		case EV_USE_ITEM2:		// medkit

		default:
			break;
		}
	}

}

/*
==============
ClientThink

This will be called once for each client frame, which will
usually be a couple times for each server frame on fast clients.

If "g_synchronousClients 1" is set, this will be called exactly
once for each server frame, which makes for smooth demo recording.
==============
*/
void ClientThink_real( gentity_t *ent ) {
	gclient_t	*client;
	pmove_t		pm;
	int			oldEventSequence;
	int			msec;
	usercmd_t	*ucmd;


	client = ent->client;

	// don't think if the client is not yet connected (and thus not yet spawned in)
	if (client->pers.connected != CON_CONNECTED) {
		return;
	}
	// mark the time, so the connection sprite can be removed
	ucmd = &ent->client->pers.cmd;

	// sanity check the command time to prevent speedup cheating
	if ( ucmd->serverTime > level.time + 200 ) {
		ucmd->serverTime = level.time + 200;
//		G_Printf("serverTime <<<<<\n" );
	}
	if ( ucmd->serverTime < level.time - 1000 ) {
		ucmd->serverTime = level.time - 1000;
//		G_Printf("serverTime >>>>>\n" );
	}

//unlagged - backward reconciliation #4
	// frameOffset should be about the number of milliseconds into a frame
	// this command packet was received, depending on how fast the server
	// does a G_RunFrame()
	client->frameOffset = trap_Milliseconds() - level.frameStartTime;
//unlagged - backward reconciliation #4


//unlagged - lag simulation #3
	// if the client wants to simulate outgoing packet loss
	if ( client->pers.plOut ) {
		// see if a random value is below the threshhold
		float thresh = (float)client->pers.plOut / 100.0f;
		if ( random() < thresh ) {
			// do nothing at all if it is - this is a lost command
			return;
		}
	}
//unlagged - lag simulation #3


//unlagged - true ping
	// save the estimated ping in a queue for averaging later

	// we use level.previousTime to account for 50ms lag correction
	// besides, this will turn out numbers more like what players are used to
	client->pers.pingsamples[client->pers.samplehead] = level.previousTime + client->frameOffset - ucmd->serverTime;
	client->pers.samplehead++;
	if ( client->pers.samplehead >= NUM_PING_SAMPLES ) {
		client->pers.samplehead -= NUM_PING_SAMPLES;
	}

	// initialize the real ping
	if ( g_truePing.integer ) {
		int i, sum = 0;

		// get an average of the samples we saved up
		for ( i = 0; i < NUM_PING_SAMPLES; i++ ) {
			sum += client->pers.pingsamples[i];
		}

		client->pers.realPing = sum / NUM_PING_SAMPLES;
	}
	else {
		// if g_truePing is off, use the normal ping
		client->pers.realPing = client->ps.ping;
	}
//unlagged - true ping


//unlagged - lag simulation #2
	// keep a queue of past commands
	client->pers.cmdqueue[client->pers.cmdhead] = client->pers.cmd;
	client->pers.cmdhead++;
	if ( client->pers.cmdhead >= MAX_LATENT_CMDS ) {
		client->pers.cmdhead -= MAX_LATENT_CMDS;
	}

	// if the client wants latency in commands (client-to-server latency)
	if ( client->pers.latentCmds ) {
		// save the actual command time
		int time = ucmd->serverTime;

		// find out which index in the queue we want
		int cmdindex = client->pers.cmdhead - client->pers.latentCmds - 1;
		while ( cmdindex < 0 ) {
			cmdindex += MAX_LATENT_CMDS;
		}

		// read in the old command
		client->pers.cmd = client->pers.cmdqueue[cmdindex];

		// adjust the real ping to reflect the new latency
		client->pers.realPing += time - ucmd->serverTime;
	}
//unlagged - lag simulation #2


//unlagged - backward reconciliation #4
	// save the command time *before* pmove_fixed messes with the serverTime,
	// and *after* lag simulation messes with it :)
	// attackTime will be used for backward reconciliation later (time shift)
	client->attackTime = ucmd->serverTime;
//unlagged - backward reconciliation #4


//unlagged - smooth clients #1
	// keep track of this for later - we'll use this to decide whether or not
	// to send extrapolated positions for this client
	client->lastUpdateFrame = level.framenum;
//unlagged - smooth clients #1


//unlagged - lag simulation #1
	// if the client is adding latency to received snapshots (server-to-client latency)
	if ( client->pers.latentSnaps ) {
		// adjust the real ping
		client->pers.realPing += client->pers.latentSnaps * (1000 / sv_fps.integer);
		// adjust the attack time so backward reconciliation will work
		client->attackTime -= client->pers.latentSnaps * (1000 / sv_fps.integer);
	}
//unlagged - lag simulation #1


//unlagged - true ping
	// make sure the true ping is over 0 - with cl_timenudge it can be less
	if ( client->pers.realPing < 0 ) {
		client->pers.realPing = 0;
	}
//unlagged - true ping

	msec = ucmd->serverTime - client->ps.commandTime;
	// following others may result in bad times, but we still want
	// to check for follow toggles
	if ( msec < 1 && client->sess.spectatorState != SPECTATOR_FOLLOW ) {
		return;
	}
	if ( msec > 200 ) {
		msec = 200;
	}

	if ( pmove_msec.integer < 8 ) {
		trap_Cvar_Set("pmove_msec", "8");
	}
	else if (pmove_msec.integer > 33) {
		trap_Cvar_Set("pmove_msec", "33");
	}

	if ( pmove_fixed.integer || client->pers.pmoveFixed ) {
		ucmd->serverTime = ((ucmd->serverTime + pmove_msec.integer-1) / pmove_msec.integer) * pmove_msec.integer;
		//if (ucmd->serverTime - client->ps.commandTime <= 0)
		//	return;
	}

	//
	// check for exiting intermission
	//
	if ( level.intermissiontime ) {
		ClientIntermissionThink( client );
		return;
	}

	// spectators don't do much
	if ( client->sess.sessionTeam >= TEAM_SPECTATOR ) {
		if ( client->sess.spectatorState == SPECTATOR_SCOREBOARD ) {
			return;
		}
		SpectatorThink( ent, ucmd );
		return;
	}

	// check for inactivity timer, but never drop the local client of a non-dedicated server
	if ( !ClientInactivityTimer( client ) ) {
		return;
	}

	if ( client->noclip ) {
		client->ps.pm_type = PM_NOCLIP;
	} else if ( client->ps.stats[STAT_HEALTH] <= 0 ) {
		client->ps.pm_type = PM_DEAD;
	} else {
		client->ps.pm_type = PM_NORMAL;
	}

	//cs style deathcam, the server updates clients' viewangles to their killer
	if (client->ps.pm_type == PM_DEAD && g_deathcam.integer
		    && !(g_entities[client->ps.clientNum].r.svFlags & SVF_BOT) && (g_gametype.integer != GT_DUEL)) {
		LookAtKiller (ent, ent->enemy);
	}

	client->ps.gravity = g_gravity.value;

	// set speed
	client->ps.speed = g_speed.value;

	// set up for pmove
	oldEventSequence = client->ps.eventSequence;

	memset (&pm, 0, sizeof(pm));

	// check for the hit-scan gauntlet, don't let the action
	// go through as an attack unless it actually hits something
	// If reload button is pressed at the same time as the attack button,
	// disable knife action. The same applies to any "talk action"
	// (say, say_team, opening console and buy choosing action)
	// No attack is made when in weapon choose mode
	if ((ucmd->buttons & BUTTON_ATTACK) && !(ucmd->buttons & (BUTTON_RELOAD | BUTTON_TALK))
		&& !(ent->client->ps.stats[STAT_FLAGS] & SF_WP_CHOOSE)
		&& ent->client->ps.weapon == WP_KNIFE
		&& !ent->client->ps.stats[STAT_WP_MODE]
		&& ent->client->ps.weaponTime - (ucmd->serverTime - ent->client->ps.commandTime) <= 0)
		CheckCloseCombatKnifeAttack(ent);

	if ( ent->flags & FL_FORCE_GESTURE ) {
		ent->flags &= ~FL_FORCE_GESTURE;
		ent->client->pers.cmd.buttons |= BUTTON_GESTURE;
	}

	//count down pain acceleration counter
	if(client->ps.stats[STAT_KNOCKTIME]){
		qbool left = (client->ps.stats[STAT_KNOCKTIME] < 0);

		if(fabs(client->ps.stats[STAT_KNOCKTIME]) >= KNOCKTIME || client->ps.stats[STAT_HEALTH] <= 0){
			client->ps.stats[STAT_KNOCKTIME] = 0;
		} else if(left) {
			client->ps.stats[STAT_KNOCKTIME] -= msec;
		} else {
			client->ps.stats[STAT_KNOCKTIME] += msec;
		}
	}

	pm.ps = &client->ps;
	pm.cmd = *ucmd;
	pm.ps->oldbuttons = client->buttons;

	if ( pm.ps->pm_type == PM_DEAD ) {
		pm.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;
	}
	else if ( ent->r.svFlags & SVF_BOT ) {
		pm.tracemask = MASK_PLAYERSOLID | CONTENTS_BOTCLIP;
		client->ps.stats[STAT_FLAGS] |= SF_BOT;
	}
	else {
		pm.tracemask = MASK_PLAYERSOLID;
	}
	pm.trace = trap_Trace_New;
	pm.pointcontents = trap_PointContents;
	pm.debugLevel = g_debugMove.integer;
	pm.noFootsteps = ( g_dmflags.integer & DF_NO_FOOTSTEPS ) > 0;

	pm.pmove_fixed = pmove_fixed.integer | client->pers.pmoveFixed;
	pm.pmove_msec = pmove_msec.integer;

	VectorCopy( client->ps.origin, client->oldOrigin );

	if((!du_nextroundstart || du_nextroundstart <= level.time)
		&& du_introend >= level.time && g_gametype.integer == GT_DUEL &&
		ent->client->sess.sessionTeam == TEAM_FREE){
		// delete all movement and cmd stats
		pm.ps->speed = 0;
		pm.cmd.forwardmove = 0;
		pm.cmd.rightmove = 0;
		pm.cmd.upmove = 0;
		pm.cmd.weapon = WP_NONE;
		pm.cmd.buttons = 0;
		pm.ps->weapon = WP_NONE;
		pm.ps->weapon2 = WP_NONE;
		pm.xyspeed = 0;
		
		VectorClear(ent->s.pos.trDelta);
	}
	else if ( ( g_gametype.integer >= GT_RTP ) && ( level.time < g_roundstarttime + level.roundNoMoveTime )
		&& ( ent->client->sess.sessionTeam < TEAM_SPECTATOR ) ) {
		// added by Joe Kari: delete all movement and cmd stats until the end of the countdown in RTP and BR gametype
		pm.ps->speed = 0;
		pm.cmd.forwardmove = 0;
		pm.cmd.rightmove = 0;
		pm.cmd.upmove = 0;
		pm.cmd.buttons = 0;
		pm.xyspeed = 0;
		
		VectorClear(ent->s.pos.trDelta);
	}

	// mouseangles can't be moved during camera move
	if(!du_nextroundstart && du_introend - DU_INTRO_DRAW >= level.time &&
		ent->client->sess.sessionTeam == TEAM_FREE) {
		pm.ps->stats[STAT_FLAGS] |= SF_DU_INTRO;
	} else {
		pm.ps->stats[STAT_FLAGS] &= ~SF_DU_INTRO;
	}

	// if player is able to change to the spectators in duel mode remove the attack button
	if(g_gametype.integer == GT_DUEL && pm.ps->stats[STAT_FLAGS] & SF_DU_WON)
		pm.cmd.buttons &= ~BUTTON_ATTACK;

	if(pm.ps->stats[STAT_GATLING_MODE]){
		pm.ps->speed = 0;
		pm.cmd.forwardmove = 0;
		pm.cmd.rightmove = 0;
		pm.cmd.upmove = 0;
	}

	// perform a pmove
	Pmove (&pm);

	// save results of pmove
	if ( ent->client->ps.eventSequence != oldEventSequence ) {
		ent->eventTime = level.time;
	}

//unlagged - smooth clients #2
	// clients no longer do extrapolation if cg_smoothClients is 1, because
	// skip correction is all handled server-side now
	// since that's the case, it makes no sense to store the extra info
	// in the client's snapshot entity, so let's save a little bandwidth
	BG_PlayerStateToEntityState( &ent->client->ps, &ent->s, qtrue );

	// use the snapped origin for linking so it matches client predicted versions
	VectorCopy( ent->s.pos.trBase, ent->r.currentOrigin );

	VectorCopy (pm.mins, ent->r.mins);
	VectorCopy (pm.maxs, ent->r.maxs);

	ent->waterlevel = pm.waterlevel;
	ent->watertype = pm.watertype;

	//clear movestate
	client->movestate = 0;

	//set the movement-state
	if(ucmd->forwardmove || ucmd->rightmove)
		client->movestate |= MS_WALK;

	if(client->ps.weaponstate == WEAPON_JUMPING)
		client->movestate |= MS_JUMP;
	else if( ucmd->upmove < 0)
		client->movestate |= MS_CROUCHED;

	// execute client events
	ClientEvents( ent, oldEventSequence );

	// link entity now, after any personal teleporters have been used
	trap_LinkEntity (ent);
	if ( !ent->client->noclip ) {
		G_TouchTriggers( ent );
		//check if player is able to buy something in rtp-games
		G_TouchSpawnBuyPoint(ent);
	}

	// NOTE: now copy the exact origin over otherwise clients can be snapped into solid
	VectorCopy( ent->client->ps.origin, ent->r.currentOrigin );

	//test for solid areas in the AAS file
	BotTestAAS(ent->r.currentOrigin);

	// touch other objects
	ClientImpacts( ent, &pm );

	// save results of triggers and client events
	if (ent->client->ps.eventSequence != oldEventSequence) {
		ent->eventTime = level.time;
	}

	// swap and latch button actions
	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;
	client->latched_buttons |= client->buttons & ~client->oldbuttons;

	// check for respawning
	if ( client->ps.stats[STAT_HEALTH] <= 0 ) {
		// wait for the attack button to be pressed
		if ( level.time > client->respawnTime ) {
			// forcerespawn is to prevent users from waiting out powerups
			if (( level.time - client->respawnTime ) > 4 * 1000 ) {

				if ( g_gametype.integer >= GT_RTP ) {
						if (client->sess.sessionTeam == TEAM_BLUE)
							SetTeam( ent, "bluespec" );
						else if (client->sess.sessionTeam == TEAM_RED)
							SetTeam( ent, "redspec" );
					return;
				}
				if (g_gametype.integer == GT_DUEL){
					SetTeam( ent, "s");
					return;
				}

				respawn( ent );
			}// pressing attack or use is the normal respawn method
			else if ( ucmd->buttons & ( BUTTON_ATTACK | BUTTON_USE_HOLDABLE ) ) {

				if ( g_gametype.integer >= GT_RTP ) {
						if (client->sess.sessionTeam == TEAM_BLUE)
							SetTeam( ent, "bluespec" );
						else if (client->sess.sessionTeam == TEAM_RED)
							SetTeam( ent, "redspec" );
					return;
				}

				if (g_gametype.integer == GT_DUEL){
					SetTeam( ent, "s");
					return;
				}

				respawn( ent );
			}
		}
		return;
	}

	// perform once-a-second actions
	ClientTimerActions( ent, msec );

	// if client is a duel winner and wants to join the spectators
	if(g_gametype.integer == GT_DUEL && (ucmd->buttons & BUTTON_ATTACK) &&
		(client->ps.stats[STAT_FLAGS] & SF_DU_WON)){
		ent->client->specwatch = qtrue;
		ent->client->player_died = qfalse;
		ent->client->realspec = qfalse;

		SetTeam(ent, "s");

		ent->client->ps.stats[STAT_FLAGS] &= ~SF_DU_WON;
	}
}

/*
==================
ClientThink

A new command has arrived from the client
==================
*/
void ClientThink( int clientNum ) {
	gentity_t *ent;

	ent = g_entities + clientNum;
	trap_GetUsercmd( clientNum, &ent->client->pers.cmd );

	// mark the time we got info, so we can display the
	// phone jack if they don't get any for a while
	ent->client->lastCmdTime = level.time;

	if ( !(ent->r.svFlags & SVF_BOT) && !g_synchronousClients.integer ) {
		ClientThink_real( ent );
	}
}


void G_RunClient( gentity_t *ent ) {
	if ( !(ent->r.svFlags & SVF_BOT) && !g_synchronousClients.integer ) {
		return;
	}
	ent->client->pers.cmd.serverTime = level.time;
	ClientThink_real( ent );
}


/*
==================
SpectatorClientEndFrame

==================
*/
void SpectatorClientEndFrame( gentity_t *ent ) {
	gclient_t	*cl;

	// if we are doing a chase cam or a remote view, grab the latest info
	if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ) {
		int		clientNum, flags, savedScore;

		clientNum = ent->client->sess.spectatorClient;

		// team follow1 and team follow2 go to whatever clients are playing
		if ( clientNum == -1 ) {
			clientNum = level.follow1;
		} else if ( clientNum == -2 ) {
			clientNum = level.follow2;
		}
		if ( clientNum >= 0 ) {
			cl = &level.clients[ clientNum ];
			if ( cl->pers.connected == CON_CONNECTED && ( cl->sess.sessionTeam < TEAM_SPECTATOR ||
			                (g_gametype.integer == GT_DUEL && !cl->realspec) ) ) {
				flags = (cl->ps.eFlags & ~(EF_VOTED | EF_TEAMVOTED)) | (ent->client->ps.eFlags & (EF_VOTED | EF_TEAMVOTED));
				if ( g_gametype.integer != GT_FFA ) {
					savedScore = ent->client->ps.persistant[PERS_SCORE]; // keep own score
					ent->client->ps = cl->ps;
					ent->client->ps.persistant[PERS_SCORE] = savedScore;
				}
				else
					ent->client->ps = cl->ps;
				ent->client->ps.pm_flags |= PMF_FOLLOW;
				ent->client->ps.eFlags = flags;
				return;
			} else {
				// drop them to free spectators unless they are dedicated camera followers
				if ( ent->client->sess.spectatorClient >= 0 ) {
					ent->client->sess.spectatorState = SPECTATOR_FREE;
					if ( g_gametype.integer >= GT_RTP ) {
						savedScore = ent->client->ps.persistant[PERS_SCORE];
						ClientBegin( ent->client - level.clients );
						ent->client->ps.persistant[PERS_SCORE] = savedScore;

						/*trap_SendServerCommand(-1,
							va("print \"DEBUG: %s " S_COLOR_WHITE "'s score (%i) has been kept\n\"",
							ent->client->pers.netname, savedScore));*/
					}
					else {
						ClientBegin( ent->client - level.clients );
					}
				}
			}
		}
	}

	if ( ent->client->sess.spectatorState == SPECTATOR_SCOREBOARD ) {
		ent->client->ps.pm_flags |= PMF_SCOREBOARD;
	} else {
		ent->client->ps.pm_flags &= ~PMF_SCOREBOARD;
	}
}

/*
==============
ClientEndFrame

Called at the end of each server frame for each connected client
A fast client will have multiple ClientThink for each ClientEndFrame,
while a slow client may have multiple ClientEndFrame between ClientThink.
==============
*/
void ClientEndFrame( gentity_t *ent ) {
	int frames;
	clientPersistant_t	*pers;

	if ( ent->client->sess.sessionTeam >= TEAM_SPECTATOR ) {
		SpectatorClientEndFrame( ent );
		return;
	}

	pers = &ent->client->pers;

	// save network bandwidth
#if 0
	if ( !g_synchronousClients->integer && ent->client->ps.pm_type == PM_NORMAL ) {
		// FIXME: this must change eventually for non-sync demo recording
		VectorClear( ent->client->ps.viewangles );
	}
#endif

	//
	// If the end of unit layout is displayed, don't give
	// the player any normal movement attributes
	//
	if ( level.intermissiontime ) {
		return;
	}

	// burn from lava, etc
	P_WorldEffects (ent);

	// apply all the damage taken this frame
	P_DamageFeedback (ent);

	// add the EF_CONNECTION flag if we haven't gotten commands recently
	if ( level.time - ent->client->lastCmdTime > 1000 ) {
		ent->client->ps.eFlags |= EF_CONNECTION;
		ent->s.eFlags |= EF_CONNECTION;
	} else {
		ent->client->ps.eFlags &= ~EF_CONNECTION;
	}

	ent->client->ps.stats[STAT_HEALTH] = ent->health;	// FIXME: get rid of ent->health...

	G_SetClientSound (ent);

	// set the latest information
//unlagged - smooth clients #2
	// clients no longer do extrapolation if cg_smoothClients is 1, because
	// skip correction is all handled server-side now
	// since that's the case, it makes no sense to store the extra info
	// in the client's snapshot entity, so let's save a little bandwidth
	BG_PlayerStateToEntityState( &ent->client->ps, &ent->s, qtrue );
//unlagged - smooth clients #2

//unlagged - smooth clients #1
	// mark as not missing updates initially
	ent->client->ps.eFlags &= ~EF_CONNECTION;

	// see how many frames the client has missed
	frames = level.framenum - ent->client->lastUpdateFrame - 1;

	// don't extrapolate more than two frames
	if ( frames > 2 ) {
		frames = 2;

		// if they missed more than two in a row, show the phone jack
		//ent->client->ps.eFlags |= EF_CONNECTION;
		//ent->s.eFlags |= EF_CONNECTION;
	}

	// did the client miss any frames?
	if ( frames > 0 && g_smoothClients.integer ) {
		// yep, missed one or more, so extrapolate the player's movement
		G_PredictPlayerMove( ent, (float)frames / sv_fps.integer );
		// save network bandwidth
		SnapVector( ent->s.pos.trBase );
	}
//unlagged - smooth clients #1

//unlagged - backward reconciliation #1
	// store the client's position for backward reconciliation later
	G_StoreHistory( ent );
//unlagged - backward reconciliation #1

	// set the bit for the reachability area the client is currently in
//	i = trap_AAS_PointReachabilityAreaIndex( ent->client->ps.origin );
//	ent->client->areabits[i >> 3] |= 1 << (i & 7);
}


