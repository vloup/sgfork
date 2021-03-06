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

/*****************************************************************************
 * name:		ai_vcmd.c
 *
 * desc:		Quake3 bot AI
 *
 * $Archive: /MissionPack/code/game/ai_vcmd.c $
 *
 *****************************************************************************/

#include "g_local.h"
#include "../botlib/botlib.h"
#include "../botlib/be_aas.h"
#include "../botlib/be_ea.h"
#include "../botlib/be_ai_char.h"
#include "../botlib/be_ai_chat.h"
#include "../botlib/be_ai_gen.h"
#include "../botlib/be_ai_goal.h"
#include "../botlib/be_ai_move.h"
#include "../botlib/be_ai_weap.h"
//
#include "ai_main.h"
#include "ai_dmq3.h"
#include "ai_chat.h"
#include "ai_cmd.h"
#include "ai_dmnet.h"
#include "ai_team.h"
#include "ai_vcmd.h"
//
#include "chars.h"				//characteristics
#include "inv.h"				//indexes into the inventory
#include "syn.h"				//synonyms
#include "match.h"				//string matching types and vars

// for the voice chats
#include "../../base/ui/menudef.h"


typedef struct voiceCommand_s
{
	char *cmd;
	void (*func)(bot_state_t *bs, int client, int mode);
} voiceCommand_t;

/*
==================
BotVoiceChat_Offense
==================
*/
void BotVoiceChat_Offense(bot_state_t *bs, int client, int mode) {
	{
		//
		bs->decisionmaker = client;
		bs->ordered = qtrue;
		bs->order_time = FloatTime();
		//set the time to send a message to the team mates
		bs->teammessage_time = FloatTime() + 2 * random();
		//set the ltg type
		bs->ltgtype = LTG_ATTACKENEMYBASE;
		//set the team goal time
		bs->teamgoal_time = FloatTime() + TEAM_ATTACKENEMYBASE_TIME;
		bs->attackaway_time = 0;
		//
		BotSetTeamStatus(bs);
		// remember last ordered task
		BotRememberLastOrderedTask(bs);
	}
#ifdef DEBUG
	BotPrintTeamGoal(bs);
#endif //DEBUG
}

/*
==================
BotVoiceChat_Defend
==================
*/
void BotVoiceChat_Defend(bot_state_t *bs, int client, int mode) {
	//
	bs->decisionmaker = client;
	bs->ordered = qtrue;
	bs->order_time = FloatTime();
	//set the time to send a message to the team mates
	bs->teammessage_time = FloatTime() + 2 * random();
	//set the ltg type
	bs->ltgtype = LTG_DEFENDKEYAREA;
	//get the team goal time
	bs->teamgoal_time = FloatTime() + TEAM_DEFENDKEYAREA_TIME;
	//away from defending
	bs->defendaway_time = 0;
	//
	BotSetTeamStatus(bs);
	// remember last ordered task
	BotRememberLastOrderedTask(bs);
#ifdef DEBUG
	BotPrintTeamGoal(bs);
#endif //DEBUG
}

/*
==================
BotVoiceChat_Patrol
==================
*/
void BotVoiceChat_Patrol(bot_state_t *bs, int client, int mode) {
	//
	bs->decisionmaker = client;
	//
	bs->ltgtype = 0;
	bs->lead_time = 0;
	bs->lastgoal_ltgtype = 0;
	//
	BotAI_BotInitialChat(bs, "dismissed", NULL);
	trap_BotEnterChat(bs->cs, client, CHAT_TELL);
	BotVoiceChatOnly(bs, -1, VOICECHAT_ONPATROL);
	//
	BotSetTeamStatus(bs);
#ifdef DEBUG
	BotPrintTeamGoal(bs);
#endif //DEBUG
}

/*
==================
BotVoiceChat_Camp
==================
*/
void BotVoiceChat_Camp(bot_state_t *bs, int client, int mode) {
	int areanum;
	aas_entityinfo_t entinfo;
	char netname[MAX_NETNAME];

	//
	bs->teamgoal.entitynum = -1;
	BotEntityInfo(client, &entinfo);
	//if info is valid (in PVS)
	if (entinfo.valid) {
		areanum = BotPointAreaNum(entinfo.origin);
		if (areanum) { // && trap_AAS_AreaReachability(areanum)) {
			//NOTE: just assume the bot knows where the person is
			//if (BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360, client)) {
				bs->teamgoal.entitynum = client;
				bs->teamgoal.areanum = areanum;
				VectorCopy(entinfo.origin, bs->teamgoal.origin);
				VectorSet(bs->teamgoal.mins, -8, -8, -8);
				VectorSet(bs->teamgoal.maxs, 8, 8, 8);
			//}
		}
	}
	//if the other is not visible
	if (bs->teamgoal.entitynum < 0) {
		BotAI_BotInitialChat(bs, "whereareyou", EasyClientName(client, netname, sizeof(netname)), NULL);
		trap_BotEnterChat(bs->cs, client, CHAT_TELL);
		return;
	}
	//
	bs->decisionmaker = client;
	bs->ordered = qtrue;
	bs->order_time = FloatTime();
	//set the time to send a message to the team mates
	bs->teammessage_time = FloatTime() + 2 * random();
	//set the ltg type
	bs->ltgtype = LTG_CAMPORDER;
	//get the team goal time
	bs->teamgoal_time = FloatTime() + TEAM_CAMP_TIME;
	//the teammate that requested the camping
	bs->teammate = client;
	//not arrived yet
	bs->arrive_time = 0;
	//
	BotSetTeamStatus(bs);
	// remember last ordered task
	BotRememberLastOrderedTask(bs);
#ifdef DEBUG
	BotPrintTeamGoal(bs);
#endif //DEBUG
}

/*
==================
BotVoiceChat_FollowMe
==================
*/
void BotVoiceChat_FollowMe(bot_state_t *bs, int client, int mode) {
	int areanum;
	aas_entityinfo_t entinfo;
	char netname[MAX_NETNAME];

	bs->teamgoal.entitynum = -1;
	BotEntityInfo(client, &entinfo);
	//if info is valid (in PVS)
	if (entinfo.valid) {
		areanum = BotPointAreaNum(entinfo.origin);
		if (areanum) { // && trap_AAS_AreaReachability(areanum)) {
			bs->teamgoal.entitynum = client;
			bs->teamgoal.areanum = areanum;
			VectorCopy(entinfo.origin, bs->teamgoal.origin);
			VectorSet(bs->teamgoal.mins, -8, -8, -8);
			VectorSet(bs->teamgoal.maxs, 8, 8, 8);
		}
	}
	//if the other is not visible
	if (bs->teamgoal.entitynum < 0) {
		BotAI_BotInitialChat(bs, "whereareyou", EasyClientName(client, netname, sizeof(netname)), NULL);
		trap_BotEnterChat(bs->cs, client, CHAT_TELL);
		return;
	}
	//
	bs->decisionmaker = client;
	bs->ordered = qtrue;
	bs->order_time = FloatTime();
	//the team mate
	bs->teammate = client;
	//last time the team mate was assumed visible
	bs->teammatevisible_time = FloatTime();
	//set the time to send a message to the team mates
	bs->teammessage_time = FloatTime() + 2 * random();
	//get the team goal time
	bs->teamgoal_time = FloatTime() + TEAM_ACCOMPANY_TIME;
	//set the ltg type
	bs->ltgtype = LTG_TEAMACCOMPANY;
	bs->formation_dist = 3.5 * 32;		//3.5 meter
	bs->arrive_time = 0;
	//
	BotSetTeamStatus(bs);
	// remember last ordered task
	BotRememberLastOrderedTask(bs);
#ifdef DEBUG
	BotPrintTeamGoal(bs);
#endif //DEBUG
}

void BotVoiceChat_Dummy(bot_state_t *bs, int client, int mode) {
}

voiceCommand_t voiceCommands[] = {
	{VOICECHAT_OFFENSE, BotVoiceChat_Offense },
	{VOICECHAT_DEFEND, BotVoiceChat_Defend },
	{VOICECHAT_PATROL, BotVoiceChat_Patrol },
	{VOICECHAT_CAMP, BotVoiceChat_Camp },
	{VOICECHAT_FOLLOWME, BotVoiceChat_FollowMe },
	{NULL, BotVoiceChat_Dummy}
};

int BotVoiceChatCommand(bot_state_t *bs, int mode, char *voiceChat) {
	int i, voiceOnly, clientNum, color;
	char *ptr, buf[MAX_MESSAGE_SIZE], *cmd;

	if (!TeamPlayIsOn()) {
		return qfalse;
	}

	if ( mode == SAY_ALL ) {
		return qfalse;	// don't do anything with voice chats to everyone
	}

	Q_strncpyz(buf, voiceChat, sizeof(buf));
	cmd = buf;
	for (ptr = cmd; *cmd && *cmd > ' '; cmd++);
	while (*cmd && *cmd <= ' ') *cmd++ = '\0';
	voiceOnly = atoi(ptr);
	for (ptr = cmd; *cmd && *cmd > ' '; cmd++);
	while (*cmd && *cmd <= ' ') *cmd++ = '\0';
	clientNum = atoi(ptr);
	for (ptr = cmd; *cmd && *cmd > ' '; cmd++);
	while (*cmd && *cmd <= ' ') *cmd++ = '\0';
	color = atoi(ptr);

	if (!BotSameTeam(bs, clientNum)) {
		return qfalse;
	}

	for (i = 0; voiceCommands[i].cmd; i++) {
		if (!Q_stricmp(cmd, voiceCommands[i].cmd)) {
			voiceCommands[i].func(bs, clientNum, mode);
			return qtrue;
		}
	}
	return qfalse;
}
