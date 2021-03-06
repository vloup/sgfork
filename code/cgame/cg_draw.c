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
// cg_draw.c -- draw all of the graphical elements during
// active (after loading) gameplay

#include "cg_local.h"
#include "../ui/ui_shared.h"

// used for scoreboard
extern displayContextDef_t cgDC;
menuDef_t *menuScoreboard = NULL;
// Smokin' Guns specific
menuDef_t *menuBuy = NULL;
menuDef_t *menuItem = NULL;
int	menuItemCount = 0;
coord_t save_menuBuy;
coord_t save_menuItem;
int drawTeamOverlayModificationCount = -1;

int sortedTeamPlayers[TEAM_MAXOVERLAY];
int	numSortedTeamPlayers;

char systemChat[256];
char teamChat1[256];
char teamChat2[256];

/*
==============
CG_DrawField

Draws large numbers for status bar and powerups
==============
*/
void CG_DrawField (int x, int y, int width, int value) {
	char	num[16], *ptr;
	int		l;
	int		frame;

	if ( width < 1 ) {
		return;
	}

	// draw number string
	if ( width > 5 ) {
		width = 5;
	}

	switch ( width ) {
	case 1:
		value = value > 9 ? 9 : value;
		value = value < 0 ? 0 : value;
		break;
	case 2:
		value = value > 99 ? 99 : value;
		value = value < -9 ? -9 : value;
		break;
	case 3:
		value = value > 999 ? 999 : value;
		value = value < -99 ? -99 : value;
		break;
	case 4:
		value = value > 9999 ? 9999 : value;
		value = value < -999 ? -999 : value;
		break;
	}

	Com_sprintf (num, sizeof(num), "%i", value);
	l = strlen(num);
	if (l > width)
		l = width;
	x += 2 + CHAR_WIDTH*(width - l);

	ptr = num;
	while (*ptr && l)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		CG_DrawPic( x,y, CHAR_WIDTH, CHAR_HEIGHT, cgs.media.numberShaders[frame] );
		x += CHAR_WIDTH;
		ptr++;
		l--;
	}
}

/*
==============
CG_DrawMoneyField

Draws little money numbers for status bar and powerups
==============
*/
#define	MONEY_WIDTH 10
#define	MONEY_HEIGHT 14

void CG_DrawMoneyField (int x, int y, int width, int value) {
	char	num[16], *ptr;
	int		l;
	int		frame;

	if ( width < 1 ) {
		return;
	}

	// draw number string
	if ( width > 5 ) {
		width = 5;
	}

	switch ( width ) {
	case 1:
		value = value > 9 ? 9 : value;
		value = value < 0 ? 0 : value;
		break;
	case 2:
		value = value > 99 ? 99 : value;
		value = value < -9 ? -9 : value;
		break;
	case 3:
		value = value > 999 ? 999 : value;
		value = value < -99 ? -99 : value;
		break;
	case 4:
		value = value > 9999 ? 9999 : value;
		value = value < -999 ? -999 : value;
		break;
	}

	Com_sprintf (num, sizeof(num), "%i", value);
	l = strlen(num);
	if (l > width)
		l = width;
	x += 2 + MONEY_WIDTH*(width - l);

	ptr = num;
	while (*ptr && l)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		CG_DrawPic( x,y, MONEY_WIDTH, MONEY_HEIGHT, cgs.media.numbermoneyShaders[frame] );
		x += MONEY_WIDTH + MONEY_WIDTH/5;
		ptr++;
		l--;
	}
}

/*
================
CG_Draw3DModel

================
*/
void CG_Draw3DModel( float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t origin, vec3_t angles ) {
	refdef_t		refdef;
	refEntity_t		ent;

	if ( !cg_draw3dIcons.integer || !cg_drawIcons.integer ) {
		return;
	}

	UI_AdjustFrom640( &x, &y, &w, &h );

	memset( &refdef, 0, sizeof( refdef ) );

	memset( &ent, 0, sizeof( ent ) );
	AnglesToAxis( angles, ent.axis );
	VectorCopy( origin, ent.origin );
	ent.hModel = model;
	ent.customSkin = skin;
	ent.renderfx = RF_NOSHADOW;		// no stencil shadows

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear( refdef.viewaxis );

	refdef.fov_x = 30;
	refdef.fov_y = 30;

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.time = cg.time;

	trap_R_ClearScene();
	trap_R_AddRefEntityToScene( &ent );
	trap_R_RenderScene( &refdef );
}

/*
================
CG_DrawHead

Used for both the status bar and the scoreboard
================
*/
void CG_DrawHead( float x, float y, float w, float h, int clientNum, vec3_t headAngles ) {
	clipHandle_t	cm;
	clientInfo_t	*ci;
	float			len;
	vec3_t			origin;
	vec3_t			mins, maxs;

	ci = &cgs.clientinfo[ clientNum ];

	if ( cg_draw3dIcons.integer ) {
		cm = ci->headModel;
		if ( !cm ) {
			return;
		}

		// offset the origin y and z to center the head
		trap_R_ModelBounds( cm, mins, maxs );

		origin[2] = -0.5 * ( mins[2] + maxs[2] );
		origin[1] = 0.5 * ( mins[1] + maxs[1] );

		// calculate distance so the head nearly fills the box
		// assume heads are taller than wide
		len = 0.7 * ( maxs[2] - mins[2] );
		origin[0] = len / 0.268;	// len / tan( fov/2 )

		// allow per-model tweaking
		VectorAdd( origin, ci->headOffset, origin );

		CG_Draw3DModel( x, y, w, h, ci->headModel, ci->headSkin, origin, headAngles );
	} else if ( cg_drawIcons.integer ) {
		CG_DrawPic( x, y, w, h, ci->modelIcon );
	}

	// if they are deferred, draw a cross out
	if ( ci->deferred ) {
		CG_DrawPic( x, y, w, h, cgs.media.deferShader );
	}
}

/*
================
CG_DrawFlagModel

Used for both the status bar and the scoreboard
================
*/
void CG_DrawFlagModel( float x, float y, float w, float h, int team, qbool force2D ) {
	qhandle_t		cm;
	float			len;
	vec3_t			origin, angles;
	vec3_t			mins, maxs;
	qhandle_t		handle;

	if ( !force2D && cg_draw3dIcons.integer ) {

		VectorClear( angles );

		cm = cgs.media.redFlagModel;

		// offset the origin y and z to center the flag
		trap_R_ModelBounds( cm, mins, maxs );

		origin[2] = -0.5 * ( mins[2] + maxs[2] );
		origin[1] = 0.5 * ( mins[1] + maxs[1] );

		// calculate distance so the flag nearly fills the box
		// assume heads are taller than wide
		len = 0.5 * ( maxs[2] - mins[2] );
		origin[0] = len / 0.268;	// len / tan( fov/2 )

		angles[YAW] = 60 * sin( cg.time / 2000.0 );;

		if( team == TEAM_RED ) {
			handle = cgs.media.redFlagModel;
		} else if( team == TEAM_BLUE ) {
			handle = cgs.media.blueFlagModel;
		} else if( team == TEAM_FREE ) {
			handle = cgs.media.neutralFlagModel;
		} else {
			return;
		}
		CG_Draw3DModel( x, y, w, h, handle, 0, origin, angles );
	} else if ( cg_drawIcons.integer ) {
    return;
	}
}

/*
================
CG_DrawTeamBackground

================
*/
void CG_DrawTeamBackground( int x, int y, int w, int h, float alpha, int team )
{
	vec4_t		hcolor;

	hcolor[3] = 0;
	if ( team == TEAM_RED ) {
		hcolor[0] = 1;
		hcolor[1] = 0;
		hcolor[2] = 0;
	} else if ( team == TEAM_BLUE ) {
		hcolor[0] = 0;
		hcolor[1] = 0;
		hcolor[2] = 1;
	} else {
		return;
	}
	trap_R_SetColor( hcolor );
	CG_DrawPic( x, y, w, h, cgs.media.teamStatusBar );
	trap_R_SetColor( NULL );
}

static void CG_DrawStatusBar( void ) {
	int			color;
	centity_t	*cent;
	playerState_t	*ps;
	int			value;
	vec4_t		hcolor;
	vec3_t		angles;
	vec3_t		origin;

	static float colors[4][4] = { 
//		{ 0.2, 1.0, 0.2, 1.0 } , { 1.0, 0.2, 0.2, 1.0 }, {0.5, 0.5, 0.5, 1} };
		{ 1.0f, 0.69f, 0.0f, 1.0f },    // normal
		{ 1.0f, 0.2f, 0.2f, 1.0f },     // low health
		{ 0.5f, 0.5f, 0.5f, 1.0f },     // weapon firing
		{ 1.0f, 1.0f, 1.0f, 1.0f } };   // health > 100

	if ( cg_drawStatus.integer == 0 ) {
		return;
	}

	// draw the team background
	CG_DrawTeamBackground( 0, 420, 640, 60, 0.33f, cg.snap->ps.persistant[PERS_TEAM] );

	cent = &cg_entities[cg.snap->ps.clientNum];
	ps = &cg.snap->ps;

	VectorClear( angles );

	// draw any 3D icons first, so the changes back to 2D are minimized
	if ( cent->currentState.weapon && cg_weapons[ cent->currentState.weapon ].ammoModel ) {
		origin[0] = 70;
		origin[1] = 0;
		origin[2] = 0;
		angles[YAW] = 90 + 20 * sin( cg.time / 1000.0 );
		CG_Draw3DModel( CHAR_WIDTH*3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE,
					   cg_weapons[ cent->currentState.weapon ].ammoModel, 0, origin, angles );
	}

	if ( ps->stats[ STAT_ARMOR ] ) {
		origin[0] = 90;
		origin[1] = 0;
		origin[2] = -10;
		angles[YAW] = ( cg.time & 2047 ) * 360 / 2048.0;
		CG_Draw3DModel( 370 + CHAR_WIDTH*3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE,
					   cgs.media.armorModel, 0, origin, angles );
	}
	//
	// ammo
	//
	if ( cent->currentState.weapon ) {
		value = ps->ammo[cent->currentState.weapon];
		if ( value > -1 ) {
			if ( cg.predictedPlayerState.weaponstate == WEAPON_FIRING
				&& cg.predictedPlayerState.weaponTime > 100 ) {
				// draw as dark grey when reloading
				color = 2;	// dark grey
			} else {
				if ( value >= 0 ) {
					color = 0;	// green
				} else {
					color = 1;	// red
				}
			}
			trap_R_SetColor( colors[color] );
			
			CG_DrawField (0, 432, 3, value);
			trap_R_SetColor( NULL );

			// if we didn't draw a 3D icon, draw a 2D icon for ammo
			if ( !cg_draw3dIcons.integer && cg_drawIcons.integer ) {
				qhandle_t	icon;

				icon = cg_weapons[ cg.predictedPlayerState.weapon ].ammoIcon;
				if ( icon ) {
					CG_DrawPic( CHAR_WIDTH*3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE, icon );
				}
			}
		}
	}

	//
	// health
	//
	value = ps->stats[STAT_HEALTH];
	if ( value > 100 ) {
		trap_R_SetColor( colors[3] );		// white
	} else if (value > 25) {
		trap_R_SetColor( colors[0] );	// green
	} else if (value > 0) {
		color = (cg.time >> 8) & 1;	// flash
		trap_R_SetColor( colors[color] );
	} else {
		trap_R_SetColor( colors[1] );	// red
	}

	// stretch the health up when taking damage
	CG_DrawField ( 185, 432, 3, value);

	CG_GetColorForHealth(cg.snap->ps.stats[STAT_HEALTH],hcolor);
	trap_R_SetColor( hcolor );

	//
	// armor
	//
	value = ps->stats[STAT_ARMOR];
	if (value > 0 ) {
		trap_R_SetColor( colors[0] );
		CG_DrawField (370, 432, 3, value);
		trap_R_SetColor( NULL );
		// if we didn't draw a 3D icon, draw a 2D icon for armor
		if ( !cg_draw3dIcons.integer && cg_drawIcons.integer ) {
			CG_DrawPic( 370 + CHAR_WIDTH*3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE, cgs.media.armorIcon );
		}

	}
}

/*
===========================================================================================

  UPPER RIGHT CORNER

===========================================================================================
*/

/*
===================
CG_DrawPickupItem
===================
*/
static int CG_DrawPickupItem( int y ) {
	int		value;
	int		quantity;
	float	*fadeColor;
	float	color[4] = { 1.0f, 1.0f, 1.0f, 1.0f};

	float	size = ICON_SIZE;

	if ( cg.snap->ps.stats[STAT_HEALTH] <= 0 ) {
		return y;
	}
	value = cg.itemPickup;
	quantity = cg.itemPickupQuant;

	if(bg_itemlist[value].xyrelation > 3){
		size = ICON_SIZE * 3/bg_itemlist[value].xyrelation;
	}

	y = 480-62-10-size;

	if ( value ) {
		fadeColor = CG_FadeColor( cg.itemPickupTime, 3000 );

		if ( fadeColor ) {
			qhandle_t pic = cg_items[ value ].icon;

			if(!Q_stricmp(bg_itemlist[value].classname, "pickup_money")){

				if(quantity < COINS)
					pic = cgs.media.coins_pic;
				else if(quantity < BILLS)
					pic = cgs.media.bills_pic;
			}

			color[3] = fadeColor[3];
			CG_RegisterItemVisuals( value );
			trap_R_SetColor( fadeColor );
			CG_DrawPic( 320 - 143, y, size * bg_itemlist[value].xyrelation, size, pic );
			trap_R_SetColor( NULL );

			if(!Q_stricmp(bg_itemlist[value].classname, "pickup_money")){
				UI_Text_Paint(
				320 - 143 + size*bg_itemlist[value].xyrelation + 10,
				y + (size/2) + 10, 0.35f, color,
				va("Money: %i$", quantity), 0, -1, 3);
			} else {
				UI_Text_Paint(
					320 - 143 + size*bg_itemlist[value].xyrelation + 10,
					y + (size/2) + 10, 0.35f, color,
					bg_itemlist[ value ].pickup_name, 0, -1, 3);
			}
		}
	}

	return y;
}

/*
===========================================================================================

  LOWER RIGHT CORNER

===========================================================================================
*/

/*
=================
CG_DrawScores

Draw the small two score display
=================
*/
static float CG_DrawScores( float y ) {
	const char	*s;
	int			s1, s2, score;
	int			x, w;
	int			v;
	vec4_t		color;
	float		y1;
//	gitem_t		*item;

	s1 = cgs.scores1;
	s2 = cgs.scores2;

	y -=  BIGCHAR_HEIGHT + 8;

	y1 = y;

	// draw from the right side to left
	if ( cgs.gametype >= GT_TEAM ) {
		x = 640;
		color[0] = 0.0f;
		color[1] = 0.0f;
		color[2] = 1.0f;
		color[3] = 0.33f;
		s = va( "%2i", s2 );
		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8;
		x -= w;
		CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
		if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE ) {
			CG_DrawPic( x, y-4, w, BIGCHAR_HEIGHT+8, cgs.media.selectShader );
		}
		CG_DrawBigString( x + 4, y, s, 1.0F);

		//To be redone for BR game type
/*		if ( cgs.gametype == GT_CTF ) {
			// Display flag status
			item = BG_ItemForPowerup( PW_BLUEFLAG );

			if (item) {
				y1 = y - BIGCHAR_HEIGHT - 8;
				if( cgs.blueflag >= 0 && cgs.blueflag <= 2 ) {
					CG_DrawPic( x, y1-4, w, BIGCHAR_HEIGHT+8, cgs.media.blueFlagShader[cgs.blueflag] );
				}
			}
		}*/
		color[0] = 1.0f;
		color[1] = 0.0f;
		color[2] = 0.0f;
		color[3] = 0.33f;
		s = va( "%2i", s1 );
		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8;
		x -= w;
		CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
		if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED ) {
			CG_DrawPic( x, y-4, w, BIGCHAR_HEIGHT+8, cgs.media.selectShader );
		}
		CG_DrawBigString( x + 4, y, s, 1.0F);

		//To be redone for BR game type
		/*if ( cgs.gametype == GT_CTF ) {
			// Display flag status
			item = BG_ItemForPowerup( PW_REDFLAG );

			if (item) {
				y1 = y - BIGCHAR_HEIGHT - 8;
				if( cgs.redflag >= 0 && cgs.redflag <= 2 ) {
					CG_DrawPic( x, y1-4, w, BIGCHAR_HEIGHT+8, cgs.media.redFlagShader[cgs.redflag] );
				}
			}
		}

		if ( cgs.gametype >= GT_CTF ) {
			v = cgs.capturelimit;
		} else {*/
			v = cgs.fraglimit;
		//}
		if ( v ) {
			s = va( "%2i", v );
			w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8;
			x -= w;
			CG_DrawBigString( x + 4, y, s, 1.0F);
		}

	} else {
		qbool	spectator;

		x = 640;
		score = cg.snap->ps.persistant[PERS_SCORE];
		spectator = ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR );

		// always show your score in the second box if not in first place
		if ( s1 != score ) {
			s2 = score;
		}
		if ( s2 != SCORE_NOT_PRESENT ) {
			s = va( "%2i", s2 );
			w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8;
			x -= w;
			if ( !spectator && score == s2 && score != s1 ) {
				color[0] = 1.0f;
				color[1] = 0.0f;
				color[2] = 0.0f;
				color[3] = 0.33f;
				CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
				CG_DrawPic( x, y-4, w, BIGCHAR_HEIGHT+8, cgs.media.selectShader );
			} else {
				color[0] = 0.5f;
				color[1] = 0.5f;
				color[2] = 0.5f;
				color[3] = 0.33f;
				CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
			}	
			CG_DrawBigString( x + 4, y, s, 1.0F);
		}

		// first place
		if ( s1 != SCORE_NOT_PRESENT ) {
			s = va( "%2i", s1 );
			w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8;
			x -= w;
			if ( !spectator && score == s1 ) {
				color[0] = 0.0f;
				color[1] = 0.0f;
				color[2] = 1.0f;
				color[3] = 0.33f;
				CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
				CG_DrawPic( x, y-4, w, BIGCHAR_HEIGHT+8, cgs.media.selectShader );
			} else {
				color[0] = 0.5f;
				color[1] = 0.5f;
				color[2] = 0.5f;
				color[3] = 0.33f;
				CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
			}	
			CG_DrawBigString( x + 4, y, s, 1.0F);
		}

		if ( cgs.fraglimit ) {
			s = va( "%2i", cgs.fraglimit );
			w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8;
			x -= w;
			CG_DrawBigString( x + 4, y, s, 1.0F);
		}

	}

	return y1 - 8;
}

static float CG_DrawPowerups( float y ) {
	int		sorted[MAX_POWERUPS];
	int		sortedTime[MAX_POWERUPS];
	int		i, j, k;
	int		active;
	playerState_t	*ps;
	int		t;
	gitem_t	*item;
	int		x;
	int		color;
	float	size;
	float	f;
	static float colors[2][4] = { 
    { 0.2f, 1.0f, 0.2f, 1.0f } , 
    { 1.0f, 0.2f, 0.2f, 1.0f } 
  };

	ps = &cg.snap->ps;

	if ( ps->stats[STAT_HEALTH] <= 0 ) {
		return y;
	}

	// sort the list by time remaining
	active = 0;
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		if ( !ps->powerups[ i ] ) {
			continue;
		}
		t = ps->powerups[ i ] - cg.time;
		// ZOID--don't draw if the power up has unlimited time (999 seconds)
		// This is true of the CTF flags
		if ( t < 0 || t > 999000) {
			continue;
		}

		// insert into the list
		for ( j = 0 ; j < active ; j++ ) {
			if ( sortedTime[j] >= t ) {
				for ( k = active - 1 ; k >= j ; k-- ) {
					sorted[k+1] = sorted[k];
					sortedTime[k+1] = sortedTime[k];
				}
				break;
			}
		}
		sorted[j] = i;
		sortedTime[j] = t;
		active++;
	}

	// draw the icons and timers
	x = 640 - ICON_SIZE - CHAR_WIDTH * 2;
	for ( i = 0 ; i < active ; i++ ) {
		item = BG_ItemForPowerup( sorted[i] );

    if (item) {

		  color = 1;

		  y -= ICON_SIZE;

		  trap_R_SetColor( colors[color] );
		  CG_DrawField( x, y, 2, sortedTime[ i ] / 1000 );

		  t = ps->powerups[ sorted[i] ];
		  if ( t - cg.time >= POWERUP_BLINKS * POWERUP_BLINK_TIME ) {
			  trap_R_SetColor( NULL );
		  } else {
			  vec4_t	modulate;

			  f = (float)( t - cg.time ) / POWERUP_BLINK_TIME;
			  f -= (int)f;
			  modulate[0] = modulate[1] = modulate[2] = modulate[3] = f;
			  trap_R_SetColor( modulate );
		  }

		  if ( cg.powerupActive == sorted[i] && 
			  cg.time - cg.powerupTime < PULSE_TIME ) {
			  f = 1.0 - ( ( (float)cg.time - cg.powerupTime ) / PULSE_TIME );
			  size = ICON_SIZE * ( 1.0 + ( PULSE_SCALE - 1.0 ) * f );
		  } else {
			  size = ICON_SIZE;
		  }

		  CG_DrawPic( 640 - size, y + ICON_SIZE / 2 - size / 2, 
			  size, size, trap_R_RegisterShader( item->icon ) );
    }
	}
	trap_R_SetColor( NULL );

	return y;
}

/*
=====================
CG_DrawLowerLeft

=====================
*/
static void CG_DrawLowerLeft( void ) {
	float	y;

	y = 480 - ICON_SIZE;

	y = CG_DrawPickupItem( y );
}

/*
=====================
CG_DrawLowerRight

=====================
*/

static void CG_DrawLowerRight( void ) {
	float	y;

	y = 480 - ICON_SIZE;

	y = CG_DrawScores( y );
	y = CG_DrawPowerups( y );
}

/*
=================
CG_DrawTeamInfo
=================
*/
static void CG_DrawTeamInfo( void ) {
	int w, h;
	int i, len;
	vec4_t		hcolor;
	int		chatHeight;

#define CHATLOC_Y 420 // bottom end
#define CHATLOC_X 0

	if (cg_teamChatHeight.integer < TEAMCHAT_HEIGHT)
		chatHeight = cg_teamChatHeight.integer;
	else
		chatHeight = TEAMCHAT_HEIGHT;
	if (chatHeight <= 0)
		return; // disabled

	if (cgs.teamLastChatPos != cgs.teamChatPos) {
		if (cg.time - cgs.teamChatMsgTimes[cgs.teamLastChatPos % chatHeight] > cg_teamChatTime.integer) {
			cgs.teamLastChatPos++;
		}

		h = (cgs.teamChatPos - cgs.teamLastChatPos) * TINYCHAR_HEIGHT;

		w = 0;

		for (i = cgs.teamLastChatPos; i < cgs.teamChatPos; i++) {
			len = CG_DrawStrlen(cgs.teamChatMsgs[i % chatHeight]);
			if (len > w)
				w = len;
		}
		w *= TINYCHAR_WIDTH;
		w += TINYCHAR_WIDTH * 2;

		if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED ) {
			hcolor[0] = 1.0f;
			hcolor[1] = 0.0f;
			hcolor[2] = 0.0f;
			hcolor[3] = 0.33f;
		} else if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE ) {
			hcolor[0] = 0.0f;
			hcolor[1] = 0.0f;
			hcolor[2] = 1.0f;
			hcolor[3] = 0.33f;
		} else {
			hcolor[0] = 0.0f;
			hcolor[1] = 1.0f;
			hcolor[2] = 0.0f;
			hcolor[3] = 0.33f;
		}

		trap_R_SetColor( hcolor );
		CG_DrawPic( CHATLOC_X, CHATLOC_Y - h, 640, h, cgs.media.teamStatusBar );
		trap_R_SetColor( NULL );

		hcolor[0] = hcolor[1] = hcolor[2] = 1.0f;
		hcolor[3] = 1.0f;

		for (i = cgs.teamChatPos - 1; i >= cgs.teamLastChatPos; i--) {
			CG_DrawStringExt( CHATLOC_X + TINYCHAR_WIDTH, 
				CHATLOC_Y - (cgs.teamChatPos - i)*TINYCHAR_HEIGHT, 
				cgs.teamChatMsgs[i % chatHeight], hcolor, qfalse, qfalse,
				TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0 );
		}
	}
}

//===========================================================================================

/*
===================
CG_DrawReward
===================
*/
static void CG_DrawReward( void ) {
	float	*color;
	int		i, count;
	float	x, y;
	char	buf[32];

	if ( !cg_drawRewards.integer ) {
		return;
	}

	color = CG_FadeColor( cg.rewardTime, REWARD_TIME );
	if ( !color ) {
		if (cg.rewardStack > 0) {
			for(i = 0; i < cg.rewardStack; i++) {
				cg.rewardSound[i] = cg.rewardSound[i+1];
				cg.rewardShader[i] = cg.rewardShader[i+1];
				cg.rewardCount[i] = cg.rewardCount[i+1];
			}
			cg.rewardTime = cg.time;
			cg.rewardStack--;
			color = CG_FadeColor( cg.rewardTime, REWARD_TIME );
			trap_S_StartLocalSound(cg.rewardSound[0], CHAN_ANNOUNCER);
		} else {
			return;
		}
	}

	trap_R_SetColor( color );

	/*
	count = cg.rewardCount[0]/10;				// number of big rewards to draw

	if (count) {
		y = 4;
		x = 320 - count * ICON_SIZE;
		for ( i = 0 ; i < count ; i++ ) {
			CG_DrawPic( x, y, (ICON_SIZE*2)-4, (ICON_SIZE*2)-4, cg.rewardShader[0] );
			x += (ICON_SIZE*2);
		}
	}

	count = cg.rewardCount[0] - count*10;		// number of small rewards to draw
	*/

	if ( cg.rewardCount[0] >= 10 ) {
		y = 56;
		x = 320 - ICON_SIZE/2;
		CG_DrawPic( x, y, ICON_SIZE-4, ICON_SIZE-4, cg.rewardShader[0] );
		Com_sprintf(buf, sizeof(buf), "%d", cg.rewardCount[0]);
		x = ( SCREEN_WIDTH - SMALLCHAR_WIDTH * CG_DrawStrlen( buf ) ) / 2;
		CG_DrawStringExt( x, y+ICON_SIZE, buf, color, qfalse, qtrue,
								SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0 );
	}
	else {

		count = cg.rewardCount[0];

		y = 56;
		x = 320 - count * ICON_SIZE/2;
		for ( i = 0 ; i < count ; i++ ) {
			CG_DrawPic( x, y, ICON_SIZE-4, ICON_SIZE-4, cg.rewardShader[0] );
			x += ICON_SIZE;
		}
	}
	trap_R_SetColor( NULL );
}



#define IND_SIZE	32.0f
void CG_DrawEscapeIndicators(void){
	vec3_t dir, angles, viewangles;
	float angle;
	qhandle_t	pic;
	float x, y;
	float height, width;

	if(!cg_showescape.integer)
		return;

	if(!cg.snap->ps.powerups[PW_GOLD] || cgs.gametype != GT_BR)
		return;

	// angle to the escapepoint
	VectorSubtract(cg.escapePoint, cg.refdef.vieworg, dir);
	vectoangles(dir, angles);
	angles[YAW] = AngleNormalize180(angles[YAW]);

	// angle of the player
	VectorCopy(cg.refdefViewAngles, viewangles);
	viewangles[YAW] = AngleNormalize180(viewangles[YAW]);

	// difference
	angle = AngleNormalize180(viewangles[YAW] - angles[YAW]);

	// see where we have to draw the indicator
	if(angle >= -45 && angle <= 45){ // forward
		pic = cgs.media.indicate_fw;
		y = 0;
		x = (angle + 45.0f)/90.0f * 640.0f - IND_SIZE/2.0f;
		width = IND_SIZE;
		height = IND_SIZE/2.0f;
	} else if (angle >= -135 && angle < -45){ // left
		pic = cgs.media.indicate_lf;
		x = 0;
		y = (1.0f-(angle + 135.0f)/90.0f) * 480.0f - IND_SIZE/2.0f;
		width = IND_SIZE/2.0f;
		height = IND_SIZE;
	} else if (angle > 45 && angle <= 135){ // left
		pic = cgs.media.indicate_rt;
		y = (angle - 45.0f)/90.0f * 480.0f - IND_SIZE/2.0f;
		width = IND_SIZE/2.0f;
		height = IND_SIZE;
		x = 640 - width;
	} else { // back
		pic = cgs.media.indicate_bk;


		if(angle < 0)
			x = (1.0f-(angle + 180.0f + 45.0f)/90.0f) * 640.0f - IND_SIZE/2.0f;
		else
			x = (1.0f-(angle - 135.0f)/90.0f) * 640.0f - IND_SIZE/2.0f;

		width = IND_SIZE;
		height = IND_SIZE/2.0f;
		y = 480 - height;
	}

	// draw the indicator
	trap_R_SetColor(NULL);
	CG_DrawPic(x, y, width, height, pic);
}

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/


/*
==============
CG_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void CG_CenterPrint( const char *str, int y, int charWidth ) {
	char	*s;
	int i;
	qbool remove = qfalse;

	// play sound (new)
	trap_S_StartSound (NULL, cg.snap->ps.clientNum, CHAN_ANNOUNCER, cgs.media.bang[rand()%3] );

	Q_strncpyz( cg.centerPrint, str, sizeof(cg.centerPrint) );

	cg.centerPrintTime = cg.time;
	cg.centerPrintY = y;
	cg.centerPrintCharWidth = charWidth;

	// remove "^" if nessecary
	for(i = 0; cg.centerPrint[i]; i++){

		// ignore if there are two of them
		if(cg.centerPrint[i] == '^'){
			remove = qtrue;
			continue;
		}

		if(remove){
			int j;

			// move the others two steps further
			for( j = i-1; cg.centerPrint[j+2]; j++){
				cg.centerPrint[j] = cg.centerPrint[j+2];
			}
			cg.centerPrint[j] = '\0';
			remove = qfalse;
			i -= 2; // string had been moved back (2 shifts)
		}
	}

	// count the number of lines for centering
	cg.centerPrintLines = 1;
	s = cg.centerPrint;
	while( *s ) {
		if (*s == '\n')
			cg.centerPrintLines++;
		s++;
	}
}


/*
===================
CG_DrawCenterString
===================
*/
static void CG_DrawCenterString( void ) {
	int		w;
	vec4_t	color2	= {0.85f, 0.69f, 0.04f, 1.00f};
	float scale;
	float	*color;

	if ( !cg.centerPrintTime ) {
		return;
	}

	color = CG_FadeColor( cg.centerPrintTime, 1000 * cg_centertime.value );
	if ( !color ) {
		return;
	}

	color2[3] = color[3];

#define FIRST_PRINT	50
#define PRINT_SCALE 150

	if(cg.time - cg.centerPrintTime < FIRST_PRINT)
		scale = 1.25f;
	else if( cg.time - cg.centerPrintTime < FIRST_PRINT+PRINT_SCALE) {
		scale = (float)(cg.time - cg.centerPrintTime - FIRST_PRINT)/PRINT_SCALE;
		scale *= 0.5;
		scale = 1.25f - scale;
	} else
		scale = 0.75f;

	w = CG_DrawStrlen( cg.centerPrint ) * 20*scale;
	UI_DrawProportionalString2( 320 - w / 2, 160, cg.centerPrint, color2, scale, cgs.media.charsetProp );
	trap_R_SetColor( NULL );
	return;
}



/*
================================================================================

CROSSHAIR

================================================================================
*/

static void CG_DrawOneCrosshair(float crosshairSize, float crosshairWidthRatio,int crosshairPickUpPulse,
								int crosshairX,int crosshairY,int crosshairFriendColor, int crosshairOpponentColor,
								int crosshairActivateColor, int crosshairColor, float crosshairTransparency,
								int crosshairHealth, int crosshairDrawFriend, int drawCrosshair,
								int changeCrosshairFlags)
{
	int			ca;
	float		w, h;
	float		x, y;
	qhandle_t	hShader;
	vec4_t		hcolor;

	//Size section
	w = h = crosshairSize;
	w *= crosshairWidthRatio * cgDC.xscale * cgDC.aspectWidthScale;
	h *= cgDC.yscale;

	if(crosshairPickUpPulse){
		float	f;
		// pulse the size of the crosshair when picking up items
		f = cg.time - cg.itemPickupBlendTime;
		if(f > 0 && f < ITEM_BLOB_TIME){
			f /= ITEM_BLOB_TIME;
			w *= (1 + f);
			h *= (1 + f);
		}
	}

	//Position section
	x = crosshairX;
	y = crosshairY;

	//Color section
	if(changeCrosshairFlags & CHANGE_CROSSHAIR_TEAMMATE){
		//Target is teammate
		hcolor[0] = g_color_table[crosshairFriendColor & Q_COLORS_COUNT][0];
		hcolor[1] = g_color_table[crosshairFriendColor & Q_COLORS_COUNT][1];
		hcolor[2] = g_color_table[crosshairFriendColor & Q_COLORS_COUNT][2];
	}else if(changeCrosshairFlags & CHANGE_CROSSHAIR_OPPONENT){
		//Target is opponent
		hcolor[0] = g_color_table[crosshairOpponentColor & Q_COLORS_COUNT][0];
		hcolor[1] = g_color_table[crosshairOpponentColor & Q_COLORS_COUNT][1];
		hcolor[2] = g_color_table[crosshairOpponentColor & Q_COLORS_COUNT][2];
	}else if(changeCrosshairFlags & CHANGE_CROSSHAIR_ACTIVATE){
		//Target is item to activate
		hcolor[0] = g_color_table[crosshairActivateColor & Q_COLORS_COUNT][0];
		hcolor[1] = g_color_table[crosshairActivateColor & Q_COLORS_COUNT][1];
		hcolor[2] = g_color_table[crosshairActivateColor & Q_COLORS_COUNT][2];
	}else{
		// set color based on health
		if(crosshairHealth && cg.snap->ps.stats[STAT_HEALTH] < HEALTH_USUAL){
			CG_GetColorForHealth(cg.snap->ps.stats[STAT_HEALTH],hcolor);
		}else{
			//Usual color
			hcolor[0] = g_color_table[crosshairColor & Q_COLORS_COUNT][0];
			hcolor[1] = g_color_table[crosshairColor & Q_COLORS_COUNT][1];
			hcolor[2] = g_color_table[crosshairColor & Q_COLORS_COUNT][2];
		}
	}
	//Transparency section
	if(cgs.gametype == GT_DUEL && 
		cg.time >= cg.introend - DU_INTRO_DRAW &&
		cg.time <= cg.introend + DU_CROSSHAIR_FADE){
			//Duel fade out
			hcolor[3] = 0.0f;
			if(cg.time >= cg.introend + DU_CROSSHAIR_START)
				hcolor[3] = (float)(cg.time - cg.introend - DU_CROSSHAIR_START)/DU_CROSSHAIR_FADE;
	}else
		hcolor[3] = (crosshairTransparency <= 1.0f) ? crosshairTransparency : 1.0f;
	trap_R_SetColor(hcolor);
	//Shader section
	if(crosshairDrawFriend && changeCrosshairFlags & CHANGE_CROSSHAIR_TEAMMATE){
		//Change crosshair to indicate that target is a teammate
		hShader = cgs.media.crosshairFriendShader;
	}else{
		ca = drawCrosshair;
		if(ca < 0){
			ca = 0;
		}
		hShader = cgs.media.crosshairShader1[ca % NUM_CROSSHAIRS];
	}

	trap_R_DrawStretchPic(x + 0.5 * (cgs.glconfig.vidWidth - w), 
		y + 0.5 * (cgs.glconfig.vidHeight - h), 
		w, h, 0, 0, 1, 1, hShader);
}

/*
=================
CG_DrawCrosshair
=================
*/
static void CG_DrawCrosshair(int changeCrosshairFlags){
	qhandle_t	hShader;


	if (cg.snap->ps.persistant[PERS_TEAM] >= TEAM_SPECTATOR)
		return;

	if(cg.renderingThirdPerson)
		return;

	//Sharps rifle crosshair
	if( cg.snap->ps.stats[STAT_WP_MODE]==1 && cg.snap->ps.weapon == WP_SHARPS){
		hShader = cgs.media.scopeShader;
		CG_DrawPic(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, hShader);

		if ( changeCrosshairFlags & CHANGE_CROSSHAIR_TEAMMATE)
			trap_R_SetColor( g_color_table[cg_crosshair1FriendColor.integer & Q_COLORS_COUNT] );
		if ( changeCrosshairFlags & CHANGE_CROSSHAIR_ACTIVATE)
			trap_R_SetColor( g_color_table[cg_crosshair1ActivateColor.integer & Q_COLORS_COUNT] );
		if ( changeCrosshairFlags & CHANGE_CROSSHAIR_OPPONENT)
			trap_R_SetColor( g_color_table[cg_crosshair1OpponentColor.integer & Q_COLORS_COUNT] );

		CG_DrawPic(118.0f, 38.0f, 404.0f, 404.0f, cgs.media.scopecrossShader);

		// Zoom
		CG_ZoomDown_f();
		return;
	}//modified by Spoon

	CG_ZoomUp_f();
	
	//Crosshair1
	if(cg_drawCrosshair1.integer)
		CG_DrawOneCrosshair(cg_crosshair1Size.value, cg_crosshair1WidthRatio.value, cg_crosshair1PickUpPulse.integer,
							cg_crosshair1X.integer, cg_crosshair1Y.integer, cg_crosshair1FriendColor.integer, cg_crosshair1OpponentColor.integer,
							cg_crosshair1ActivateColor.integer, cg_crosshair1Color.integer, cg_crosshair1Transparency.value,
							cg_crosshair1Health.integer, cg_crosshairDrawFriend.integer, cg_drawCrosshair1.integer,
							changeCrosshairFlags);

	//Crosshair2
	if(cg_drawCrosshair2.integer)
		CG_DrawOneCrosshair(cg_crosshair2Size.value, cg_crosshair2WidthRatio.value, cg_crosshair2PickUpPulse.integer,
							cg_crosshair2X.integer, cg_crosshair2Y.integer, cg_crosshair2FriendColor.integer, cg_crosshair2OpponentColor.integer,
							cg_crosshair2ActivateColor.integer, cg_crosshair2Color.integer, cg_crosshair2Transparency.value,
							cg_crosshair2Health.integer, cg_crosshairDrawFriend.integer, cg_drawCrosshair2.integer,
							changeCrosshairFlags);
}

/*
=================
CG_DrawCrosshair3D
=================
*/
static void CG_DrawCrosshair3D(void)
{
	int			ca;
	float		w, h;
	qhandle_t	hShader;
	float		f;

	trace_t trace;
	vec3_t endpos;
	float stereoSep, zProj, maxdist, xmax;
	char rendererinfos[128];
	refEntity_t ent;

	if ( !cg_drawCrosshair1.integer ) {
		return;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] >= TEAM_SPECTATOR) {
		return;
	}

	if ( cg.renderingThirdPerson ) {
		return;
	}

	w = h = cg_crosshair1Size.value;

	// pulse the size of the crosshair when picking up items
	f = cg.time - cg.itemPickupBlendTime;
	if ( f > 0 && f < ITEM_BLOB_TIME ) {
		f /= ITEM_BLOB_TIME;
		w *= ( 1 + f );
		h *= ( 1 + f );
	}

 	ca = cg_drawCrosshair1.integer;
 	if (ca < 0) {
 		ca = 0;
 	}
 	hShader = cgs.media.crosshairShader1[ ca % NUM_CROSSHAIRS ];

	// Use a different method rendering the crosshair so players don't see two of them when
	// focusing their eyes at distant objects with high stereo separation
	// We are going to trace to the next shootable object and place the crosshair in front of it.

	// first get all the important renderer information
	trap_Cvar_VariableStringBuffer("r_zProj", rendererinfos, sizeof(rendererinfos));
	zProj = atof(rendererinfos);
	trap_Cvar_VariableStringBuffer("r_stereoSeparation", rendererinfos, sizeof(rendererinfos));
	stereoSep = zProj / atof(rendererinfos);
	
	xmax = zProj * tan(cg.refdef.fov_x * M_PI / 360.0f);
	
	// let the trace run through until a change in stereo separation of the crosshair becomes less than one pixel.
	maxdist = cgs.glconfig.vidWidth * stereoSep * zProj / (2 * xmax);
	VectorMA(cg.refdef.vieworg, maxdist, cg.refdef.viewaxis[0], endpos);
	CG_Trace(&trace, cg.refdef.vieworg, NULL, NULL, endpos, 0, MASK_SHOT);
	
	memset(&ent, 0, sizeof(ent));
	ent.reType = RT_SPRITE;
	ent.renderfx = RF_DEPTHHACK | RF_CROSSHAIR;
	
	VectorCopy(trace.endpos, ent.origin);
	
	// scale the crosshair so it appears the same size for all distances
	ent.radius = w / 640 * xmax * trace.fraction * maxdist / zProj;
	ent.customShader = hShader;

	trap_R_AddRefEntityToScene(&ent);
}



/*
=================
CG_ScanForCrosshairEntity
=================
*/
static void CG_ScanForCrosshairEntity(int *changeCrosshairFlags) {
	trace_t		trace;
	vec3_t		start, end;
	int			content;

	VectorCopy( cg.refdef.vieworg, start );
	VectorMA( start, 131072, cg.refdef.viewaxis[0], end );

	CG_Trace( &trace, start, vec3_origin, vec3_origin, end,
	          cg.snap->ps.clientNum, CONTENTS_SOLID|CONTENTS_BODY );

	// if the player is in fog, don't show it
	content = trap_CM_PointContents( trace.endpos, 0 );
	if ( content & CONTENTS_FOG ) {
		return;
	}

	// trace through glass
	// FIXME: doesn't work through more than one level of glass
	/*	while ( trace.surfaceFlags & SURF_GLASS ) {
		CG_Trace( &trace, trace.endpos, vec3_origin, vec3_origin,
			       end, trace.entityNum, CONTENTS_SOLID|CONTENTS_BODY );
	} <- This fix causes game's freeze   */
	if ( trace.surfaceFlags & SURF_GLASS ) {
			CG_Trace( &trace, start, vec3_origin, vec3_origin,
			          end, trace.entityNum, CONTENTS_SOLID|CONTENTS_BODY 
			);
	}

	// update the fade timer
	cg.crosshairClientNum = trace.entityNum;
	cg.crosshairClientTime = cg.time;

	if (trace.entityNum <= MAX_CLIENTS && cg_entities[trace.entityNum].currentState.eType == ET_PLAYER) {
		if(cg.snap->ps.persistant[PERS_TEAM] < TEAM_SPECTATOR) {
			if (cgs.gametype >= GT_TEAM && cgs.clientinfo[trace.entityNum].team == cg.snap->ps.persistant[PERS_TEAM]) {
					// player is on same team
					*changeCrosshairFlags |= CHANGE_CROSSHAIR_TEAMMATE;
			} else {
				// player is opponent and alive
				if (cgs.clientinfo[trace.entityNum].infoValid)
					*changeCrosshairFlags |= CHANGE_CROSSHAIR_OPPONENT;
			}
		}
	} else if (cg_entities[trace.entityNum].currentState.eType == ET_MOVER &&
			   cg_entities[trace.entityNum].currentState.angles2[0] == -1000 &&
			   Distance(cg.snap->ps.origin, trace.endpos) < ACTIVATE_RANGE) {
					// activatable entities, doors
				   *changeCrosshairFlags |= CHANGE_CROSSHAIR_ACTIVATE;
	} else if (cg_entities[trace.entityNum].currentState.eType == ET_TURRET &&
		      Distance(cg.snap->ps.origin, cg_entities[trace.entityNum].currentState.pos.trBase) < ACTIVATE_RANGE) {
				// gatling gun
				*changeCrosshairFlags |= CHANGE_CROSSHAIR_ACTIVATE;
	}
}

/*
=====================
CG_DrawCrosshairNames
=====================
*/
static void CG_DrawCrosshairNames(int isPlayer) {
	float		*color;
	char		*name;
	float		w;

	if (!isPlayer) {
		return;
	}
	if ( !cg_drawCrosshair1.integer && !cg_drawCrosshair2.integer ) {
		return;
	}
	if ( !cg_drawCrosshairNames.integer ) {
		return;
	}
	if ( cg.renderingThirdPerson ) {
		return;
	}

	if ( cg.snap->ps.pm_type == PM_CHASECAM
	              && cg.crosshairClientNum == cg.snap->ps.stats[CHASECLIENT]) {
		return;
	}

	if( Distance(cg.snap->ps.origin, cg_entities[ cg.crosshairClientNum ].lerpOrigin) > 250)
		return;

	// draw the name of the player being looked at
	color = CG_FadeColor( cg.crosshairClientTime, 1000 );
	if ( !color ) {
		trap_R_SetColor( NULL );
		return;
	}

	name = cgs.clientinfo[ cg.crosshairClientNum ].name;
	color[3] *= 0.5f;
	w = UI_Text_Width(name, 0.3f, 0);
	UI_Text_Paint( 320 - w / 2, 190, 0.3f, color, name, 0, 0, ITEM_TEXTSTYLE_SHADOWED);
	trap_R_SetColor( NULL );
}


//==============================================================================

/*
=================
CG_DrawSpectator
=================
*/
static void CG_DrawSpectator(void) {
	const char	*info;

	if(cg.scoreBoardShowing)
		return;

	info = CG_ConfigString( CS_SERVERINFO );

	if(cg.snap->ps.persistant[PERS_TEAM] >= TEAM_SPECTATOR){

		if(cgs.gametype == GT_DUEL){
			CG_DrawSmallString(5, 445, "Press \"FIRE\" to change to the next player in this mappart", 1.0F);
			CG_DrawSmallString(5, 460, "Press \"FIRE2\" to change to the next mappart", 1.0F);
			if(cg.snap->ps.pm_flags & PMF_FOLLOW)
				CG_DrawSmallString(5, 475, "Press \"JUMP\" to get to the free spectator mode", 1.0F);
		} else {
			CG_DrawSmallString(5, 445, "Press \"FIRE\" to change to the next player", 1.0F);
			CG_DrawSmallString(5, 460, "Press \"FIRE2\" to change to the previous player", 1.0F);
			if((!(atoi(Info_ValueForKey( info, "g_chaseonly")) && atoi(Info_ValueForKey( info, "g_gametype")) >= GT_RTP)
				&& (cg.snap->ps.pm_flags & PMF_FOLLOW)))
				CG_DrawSmallString(5, 475, "Press \"JUMP\" to get to the free spectator mode", 1.0F);
		}
	} else if (cgs.gametype == GT_DUEL && !(cg.snap->ps.pm_flags & PMF_FOLLOW)){
		float color[] = {1.0f, 0.75f, 0.15f, 1.0f};

		if(cg.introend >= cg.time)
			UI_Text_PaintCenter(320, 320, 0.3f, color, "Use the buy-menu to buy new pistols.", 0, 0, 3);

		if(cg.snap->ps.stats[STAT_FLAGS] & SF_DU_WON)
			UI_Text_PaintCenter(320, 320, 0.3f, color, "Press \"FIRE\" to watch the remaining duels", 0, 0, 3);
	}
}

/*
=================
CG_DrawVote
=================
*/
static void CG_DrawVote(void) {
	char	*s;
	int		sec;

	if ( !cgs.voteTime ) {
		return;
	}

	// play a talk beep whenever it is modified
	if ( cgs.voteModified ) {
		cgs.voteModified = qfalse;
		CG_PlayTalkSound();
	}

	sec = ( VOTE_TIME - ( cg.time - cgs.voteTime ) ) / 1000;
	if ( sec < 0 ) {
		sec = 0;
	}
	s = va("VOTE(%i):%s yes:%i no:%i", sec, cgs.voteString, cgs.voteYes, cgs.voteNo);
	CG_DrawSmallString( 0, 58, s, 1.0F );
	s = "or press ESC then click Vote";
	CG_DrawSmallString( 0, 58 + SMALLCHAR_HEIGHT + 2, s, 1.0F );
}

/*
=================
CG_DrawTeamVote
=================
*/
static void CG_DrawTeamVote(void) {
	char	*s;
	int		sec, cs_offset;

	if ( cgs.clientinfo->team == TEAM_RED )
		cs_offset = 0;
	else if ( cgs.clientinfo->team == TEAM_BLUE )
		cs_offset = 1;
	else
		return;

	if ( !cgs.teamVoteTime[cs_offset] ) {
		return;
	}

	// play a talk beep whenever it is modified
	if ( cgs.teamVoteModified[cs_offset] ) {
		cgs.teamVoteModified[cs_offset] = qfalse;
		CG_PlayTalkSound();
	}

	sec = ( VOTE_TIME - ( cg.time - cgs.teamVoteTime[cs_offset] ) ) / 1000;
	if ( sec < 0 ) {
		sec = 0;
	}
	s = va("TEAMVOTE(%i):%s yes:%i no:%i", sec, cgs.teamVoteString[cs_offset],
							cgs.teamVoteYes[cs_offset], cgs.teamVoteNo[cs_offset] );
	CG_DrawSmallString( 0, 90, s, 1.0F );
}

void Menu_UpdatePosition(menuDef_t *menu);

static qbool CG_DrawScoreboard( void ) {
	static qbool firstTime = qtrue;
	float fade, *fadeColor;

	if (menuScoreboard) {
		menuScoreboard->window.flags &= ~WINDOW_FORCED;
	}
	if (cg_paused.integer) {
		cg.deferredPlayerLoading = 0;
		firstTime = qtrue;
		return qfalse;
	}

	// should never happen in Team Arena
	if (cgs.gametype == GT_SINGLE_PLAYER && cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		cg.deferredPlayerLoading = 0;
		firstTime = qtrue;
		return qfalse;
	}

	// don't draw scoreboard during death while warmup up
	if ( cg.warmup && !cg.showScores ) {
		return qfalse;
	}

	if ( cg.showScores || (cg.predictedPlayerState.pm_type == PM_DEAD && !(cg.snap->ps.pm_flags & PMF_FOLLOW))
	                              || cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		fade = 1.0;
		fadeColor = colorWhite;
	} else {
		fadeColor = CG_FadeColor( cg.scoreFadeTime, FADE_TIME );
		if ( !fadeColor ) {
			// next time scoreboard comes up, don't print killer
			cg.deferredPlayerLoading = 0;
			cg.killerName[0] = 0;
			firstTime = qtrue;
			return qfalse;
		}
		fade = *fadeColor;
	}


	if (menuScoreboard == NULL) {
		if ( cgs.gametype >= GT_TEAM ) {
			menuScoreboard = Menus_FindByName("teamscore_menu");
		} else {
			menuScoreboard = Menus_FindByName("score_menu");
		}
	}

	if (menuScoreboard) {
		if (firstTime) {
			CG_SetScoreSelection(menuScoreboard);
			firstTime = qfalse;
		}
		menuScoreboard->window.rect.x = 0;

		menuScoreboard->window.rect.y = (cgs.gametype >= GT_TEAM) ? -20 : 0;
		Menu_UpdatePosition(menuScoreboard);
		Menu_Paint(menuScoreboard, qtrue);
	}

	// load any models that have been deferred
	if ( ++cg.deferredPlayerLoading > 10 ) {
		CG_LoadDeferredPlayers();
	}

	return qtrue;
}

/*
=================
CG_DrawIntermission
=================
*/
static void CG_DrawIntermission( void ) {
//	int key;
	//if (cg_singlePlayer.integer) {
	//	CG_DrawCenterString();
	//	return;
	//}
	cg.scoreFadeTime = cg.time;
	cg.scoreBoardShowing = CG_DrawScoreboard();
}

/*
=================
CG_DrawFollow
=================
*/
static qbool CG_DrawFollow( void ) {
	vec4_t		color, colorrect;
	const char	*name;
	char		health[4];
	int x, w;

	if ( !(cg.snap->ps.pm_flags & PMF_FOLLOW) ) {
		return qfalse;
	}
	color[0] = 1.0f;
	color[1] = 1.0f;
	color[2] = 1.0f;
	color[3] = 1.0f;

	colorrect[0] = 0.0f;
	colorrect[1] = 0.0f;
	colorrect[2] = 0.0f;
	colorrect[3] = 0.3f;

	if (cg.snap->ps.pm_type == PM_CHASECAM) {
		name = cgs.clientinfo[ cg.snap->ps.stats[CHASECLIENT] ].name;
		CG_FillRect( 0, 410, 640, 70, colorrect);
		CG_DrawStringExt( 120, 415, name, color, qtrue, qfalse, 14, 14, 0 );
		CG_DrawStringExt( 370, 415, "health: ", color, qtrue, qfalse, 14, 14, 0 );

		Com_sprintf (health, sizeof(health), "%i", cg.snap->ps.stats[STAT_HEALTH]);

		CG_DrawStringExt( 467, 415, health, color, qtrue, qfalse, 14, 14, 0 );
	} else {
		name = cgs.clientinfo[ cg.snap->ps.clientNum ].name;

		x = 0.5 * ( 640 - 10 * CG_DrawStrlen( "Following" ));
		CG_DrawStringExt( x, 340, "Following", color, qfalse, qfalse, 10, 10, 0 );

		w = UI_Text_Width(name, 0.5f, 0);
		UI_Text_Paint( 320 - w / 2, 375, 0.5f, color, name, 0, 0, ITEM_TEXTSTYLE_SHADOWED);
	}

	return qtrue;
}

/*
=================
CG_DrawWarmup
=================
*/
static void CG_DrawWarmup( void ) {
	int			w;
	int			sec;
	int			i;
	float scale;
	clientInfo_t	*ci1, *ci2;
	int			cw;
	const char	*s;
	vec4_t	color	= {0.85f, 0.69f, 0.04f, 1.0f};

	sec = cg.warmup;
	if ( !sec ) {
		return;
	}

	if ( sec < 0 ) {
		s = "Waiting for players";
		w = CG_DrawStrlen( s ) * 20*0.75f;
		UI_DrawProportionalString2( 320 - w / 2, 24, s, color, 0.75f, cgs.media.charsetProp );
		cg.warmupCount = 0;
		return;
	}

	if (cgs.gametype == GT_DUEL) {
		// find the two active players
		ci1 = NULL;
		ci2 = NULL;
		for ( i = 0 ; i < cgs.maxclients ; i++ ) {
			if ( cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_FREE ) {
				if ( !ci1 ) {
					ci1 = &cgs.clientinfo[i];
				} else {
					ci2 = &cgs.clientinfo[i];
				}
			}
		}

		if ( ci1 && ci2 ) {
			s = va( "%s vs %s", ci1->name, ci2->name );
			w = UI_Text_Width(s, 0.6f, 0);
			UI_Text_Paint(320 - w / 2, 60, 0.6f, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE);
		}
	} else {
		if ( cgs.gametype == GT_FFA ) {
			s = "Free For All";
		} else if ( cgs.gametype == GT_TEAM ) {
			s = "Team Deathmatch";
		} else {
			s = "";
		}
		w = UI_Text_Width(s, 0.6f, 0);
		UI_Text_Paint(320 - w / 2, 90, 0.6f, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE);
		w = CG_DrawStrlen( s );
		if ( w > 640 / GIANT_WIDTH ) {
			cw = 640 / w;
		} else {
			cw = GIANT_WIDTH;
		}
		CG_DrawStringExt( 320 - w * cw/2, 25,s, colorWhite,
				qfalse, qtrue, cw, (int)(cw * 1.1f), 0 );
	}

	sec = ( sec - cg.time ) / 1000;
	if ( sec < 0 ) {
		cg.warmup = 0;
		sec = 0;
	}
	s = va( "Starts in: %i", sec + 1 );
	if ( sec != cg.warmupCount ) {
		cg.warmupCount = sec;
		trap_S_StartLocalSound( cgs.media.bang[rand()%3], CHAN_ANNOUNCER );
	}
	scale = 0.45f;
	switch ( cg.warmupCount ) {
	case 0:
		cw = 28;
		scale = 0.54f;
		break;
	case 1:
		cw = 24;
		scale = 0.51f;
		break;
	case 2:
		cw = 20;
		scale = 0.48f;
		break;
	default:
		cw = 16;
		scale = 0.45f;
		break;
	}

		w = UI_Text_Width(s, scale, 0);
		UI_Text_Paint(320 - w / 2, 125, scale, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE);
}

//==================================================================================
/*
=================
CG_DrawTimedMenus
=================
*/
void CG_DrawTimedMenus( void ) {
	if (cg.voiceTime) {
		int t = cg.time - cg.voiceTime;
		if ( t > 2500 ) {
			Menus_CloseByName("voiceMenu");
			trap_Cvar_Set("cl_conXOffset", "0");
			cg.voiceTime = 0;
		}
	}
}

// maximum of 6 menu items
menu_items_t menu_items[MAX_MENU_ITEMS];

void CG_ClearFocusses(void){
	int i;

	for(i=0; i<menuBuy->itemCount; i++){
		menuBuy->items[i]->window.flags &= ~WINDOW_HASFOCUS;
	}
	for(i=0; i<menuItem->itemCount; i++){
		menuItem->items[i]->window.flags &= ~WINDOW_HASFOCUS;
	}
}

static qbool CG_CursorIsInRect(rectDef_t *rect){
	if(cgs.cursorX >= rect->x &&
		cgs.cursorX <= rect->x + rect->w &&
		cgs.cursorY >= rect->y &&
		cgs.cursorY <= rect->y + rect->h)
		return qtrue;

	return qfalse;
}

// movement control disabled
static qbool Menu_IsKeyDown(char *cmd){
	int i;

	if(!Q_stricmp(cmd, "mouse1")){

		// check if buy menu is open
		if(cg.menu == MENU_BUY){
			int j;
			qbool abort = qfalse;

			//stats
			if(cgs.gametype != GT_DUEL){
				for(j=0; j<menuBuy->itemCount; j++){
					if(menuBuy->items[j]->window.flags & WINDOW_HASFOCUS){
						int oldmenustat = cg.menustat;

						// goto next menu if mousebutton is pressed
						if(trap_Key_IsDown(K_MOUSE1) &&
							CG_CursorIsInRect(&menuBuy->items[j]->window.rect)){
							cg.menustat =
								menuBuy->items[j]->window.ownerDraw-CG_BUTTON_PISTOLS+1;

							// ammo -> buy the ammo
							if(cg.menustat >= 6){
								return qtrue;
							} else if((!cg.menuitem || oldmenustat != cg.menustat)){
								cg.menumove = qfalse;
								cg.menuitem = 1;
								CG_ClearFocusses();
								menuBuy->items[j]->window.flags |= WINDOW_HASFOCUS;
							}
							cg.oldbutton = qtrue;
							return qfalse;
						}
						abort = qtrue;
						break;
					}
				}
			}
			//items
			for(j=0; j<menuItem->itemCount && cg.menuitem; j++){
				if(menuItem->items[j]->window.flags & WINDOW_HASFOCUS){
					int olditem = cg.menuitem;
					cg.menuitem =
						menuItem->items[j]->window.ownerDraw-CG_BUTTON1+1;

					if((cg.menuitem > 3 || cg.menuitem < 1) && cgs.gametype == GT_DUEL){
						cg.menuitem = olditem;
						return qfalse;
					}

					// if cursor is in the button
					if(CG_CursorIsInRect(&menuItem->items[j]->window.rect))
						abort = qfalse;
					else abort = qtrue;
					break;
				}
			}
			if(!abort && trap_Key_IsDown(K_MOUSE1))
				return qtrue;
		}
	}

	// we don't need other buttons for the buy menu
	if(cg.menu == MENU_BUY)
		return qfalse;

	if(!Q_stricmp(cmd, "+attack")){
		for(i=0; i<2; i++)
			if(trap_Key_IsDown(cg_button_attack[i].integer))
				return qtrue;
	} else if(!Q_stricmp(cmd, "+button6")){
		for(i=0; i<2; i++){
			if(trap_Key_IsDown(cg_button_altattack[i].integer))
				return qtrue;
		}
	}/* else if(!Q_stricmp(cmd, "+forward")){
		for(i=0; i<2; i++){
			if(trap_Key_IsDown(cg_button_forward[i].integer))
				return qtrue;
		}
	} else if(!Q_stricmp(cmd, "+back")){
		for(i=0; i<2; i++){
			if(trap_Key_IsDown(cg_button_back[i].integer))
				return qtrue;
		}
	} else if(!Q_stricmp(cmd, "+moveleft")){
		for(i=0; i<2; i++){
			if(trap_Key_IsDown(cg_button_moveleft[i].integer))
				return qtrue;
		}
	} else if(!Q_stricmp(cmd, "+moveright")){
		for(i=0; i<2; i++){
			if(trap_Key_IsDown(cg_button_moveright[i].integer))
				return qtrue;
		}
	} else if(!Q_stricmp(cmd, "+left")){
		for(i=0; i<2; i++){
			if(trap_Key_IsDown(cg_button_turnleft[i].integer))
				return qtrue;
		}
	} else if(!Q_stricmp(cmd, "+right")){
		for(i=0; i<2; i++){
			if(trap_Key_IsDown(cg_button_turnright[i].integer))
				return qtrue;
		}
	}*/
	return qfalse;
}


float	color[6][4] = {
				{ 1.0f, 1.0f, 1.0f, 1.0f},
				{ 1.0f, 0.0f, 0.0f, 1.0f},
				{ 1.0f, 1.0f, 1.0f, 0.5f},
				{ 1.0f, 1.0f, 1.0f, 0.9f},
				{ 0.0f, 0.0f, 0.0f, 0.75f},
				{ 1.0f, 0.8f, 0.0f, 1.0f},
				};

// set up the item-menu
static void CG_SetupItemMenu(void){
	gitem_t *item;
	int count = 0;
	qbool clone = qfalse; // second "cloned" pistol

	for(item = &bg_itemlist[1]; ITEM_INDEX(item) < IT_NUM_ITEMS; item++){
		int belt = 1;

		if(cg.snap->ps.powerups[PW_BELT])
			belt = 2;

		if ( item->weapon_sort != cg.menustat){
			continue;
		};

		Q_strncpyz(menu_items[count].string, item->pickup_name, 64);

		if((item->giTag == WP_DYNAMITE || item->giTag == WP_KNIFE || item->giTag == WP_MOLOTOV) &&
			item->giType == IT_WEAPON){
			gitem_t	*ammoitem;
			ammoitem = BG_ItemForAmmo(item->giTag);
			Q_strncpyz(menu_items[count].string, va("%s(%i)", ammoitem->pickup_name, ammoitem->quantity), 64);
		}

		// check if clone
		if((cg.snap->ps.stats[STAT_WEAPONS] & (1<<item->giTag)) && item->giType == IT_WEAPON
			&& !(cg.snap->ps.stats[STAT_FLAGS] & SF_SEC_PISTOL) &&
			bg_weaponlist[item->giTag].wp_sort == WPS_PISTOL){
			clone = qtrue;
		}

		//different colors depending on money, the player has
		// yellow
		if(!clone && (((cg.snap->ps.stats[STAT_WEAPONS] & (1<<item->giTag)) && item->giType == IT_WEAPON
			&& item->giTag != WP_DYNAMITE && item->giTag != WP_KNIFE && item->giTag != WP_MOLOTOV) ||
			(item->giTag == PW_BELT && cg.snap->ps.powerups[PW_BELT] && item->giType == IT_POWERUP) ||
			(item->giType == IT_ARMOR && cg.snap->ps.stats[STAT_ARMOR])||
			(item->giType == IT_POWERUP && item->giTag == PW_SCOPE && cg.snap->ps.powerups[PW_SCOPE]) ||
			(((item->giTag == WP_DYNAMITE || item->giTag == WP_KNIFE || WP_MOLOTOV) && item->giType == IT_WEAPON) &&
			cg.snap->ps.ammo[item->giTag] >= bg_weaponlist[item->giTag].maxAmmo)))
		{
			menu_items[count].inventory = qtrue;
		} else menu_items[count].inventory = qfalse;

		// red
		if(cg.snap->ps.stats[STAT_MONEY] < item->prize) {
			menu_items[count].money = qfalse;
		} else menu_items[count].money = qtrue;

		count++;
	}

	menuItemCount = count;
}

static void CG_RefreshItemMenu(void){
	int i = 0;

	// cycle through items and make them all visible
	for(i=0; i < menuItem->itemCount; i++){
		menuItem->items[i]->window.flags |= WINDOW_VISIBLE;
	}

	menuItem->window.rect.y = (cg.menustat-1)*24;
	Menu_UpdatePosition(menuItem);
	CG_SetupItemMenu();

	// make unnessecary buttons invisible, also place the endrectangle
	for(i=0; i < menuItem->itemCount; i++){
		int ownerDraw = menuItem->items[i]->window.ownerDraw;

		if(ownerDraw >= CG_BUTTON1 && ownerDraw <= CG_BUTTON6 &&
			ownerDraw-CG_BUTTON1+1 > menuItemCount){
			menuItem->items[i]->window.flags &= ~WINDOW_VISIBLE;
		}

		if(ownerDraw == CG_BUY_END){
			menuItem->items[i]->window.rect.y =
				menuItem->window.rect.y + 28 + menuItemCount*24;
		}
	}
}

static void CG_ClearItemMenu(void){
	int i;

	menuItemCount = 0;
	cg.menuitem = 0;

	for(i=0; i<MAX_MENU_ITEMS; i++){
		menu_items[i].inventory = qfalse;
		menu_items[i].money = qfalse;
		strcpy(menu_items[i].string, "");
	}
}

void CG_CloseBuyMenu(void){
	cg.menu = MENU_NONE;
	cg.menustat = 0;
	cg.menuitem = 0;
	cg.oldbutton = qtrue;

	CG_ClearItemMenu();
	CG_ClearFocusses();

	Menus_CloseByName("buymenu");
	Menus_CloseByName("buymenu_items");

	trap_Key_SetCatcher(0);

	// Notify the server we close the buy menu.
	// The "client" engine code will reset BUTTON_BUYMENU to usercmd_t.buttons .
	trap_SendConsoleCommand( "-button8" );
}

static qbool menu_exit( void ){

	if((trap_Key_IsDown(K_ESCAPE)||trap_Key_IsDown(K_MOUSE2)||
		(trap_Key_IsDown(0+48) && !cg.menuitem))
		&& !cg.oldbutton){
		CG_CloseBuyMenu();
		return qtrue;
	}

	if(cg.introend < cg.time && cgs.gametype == GT_DUEL){
		CG_CloseBuyMenu();
		return qtrue;
	}

	return qfalse;
}

//ITEMBOX
static gitem_t *CG_GetBuyItem(void){
	gitem_t *item;
	int	j;

	for(item = &bg_itemlist[1], j = 0; ITEM_INDEX(item) < IT_NUM_ITEMS ; item++){
		if ( item->weapon_sort != cg.menustat )
			continue;
		j++;

		if( j != cg.menuitem)
			continue;

		break;
	}

	return item;
}

// this has to be done because of a bug in the menu system
static void CG_CheckFocusses(void){
	int i;

	//if(cg.menuitem && cgs.gametype != GT_DUEL)
	//	return;

	if(cgs.gametype != GT_DUEL){
		for(i = 0; i < menuBuy->itemCount; i++){
			if(CG_CursorIsInRect(&menuBuy->items[i]->window.rect) && Menu_IsKeyDown("mouse1")){
				CG_ClearFocusses();
				menuBuy->items[i]->window.flags |= WINDOW_HASFOCUS;
				break;
			}
		}
	} else {
		for(i = 0; i < menuItem->itemCount; i++){
			if(CG_CursorIsInRect(&menuItem->items[i]->window.rect)){
				CG_ClearFocusses();
				menuItem->items[i]->window.flags |= WINDOW_HASFOCUS;
				break;
			}
		}
	}
}

static void CG_DisableStatMenu(void){
	int i;

	for(i = 0; i < menuBuy->itemCount; i++){
		float color[] = {0.5, 0.5, 0.5, 1.0};
		menuBuy->items[i]->window.flags |= (WINDOW_GREY | WINDOW_DECORATION);
		Vector4Copy(color, menuBuy->items[i]->window.foreColor);
	}
}
/*
=================
CG_DrawBuyMenu
by Spoon
=================
*/
static void CG_DrawBuyMenu( void ) {
	int count;
	int i;
	gitem_t	*item;
	qbool numpressed = qfalse, ammo_numpressed = qfalse;
	qbool duel = (cgs.gametype == GT_DUEL);

	if(cg_buydebug.integer){
		if(trap_Key_IsDown('p') && !cg.oldbutton){
			trap_SendConsoleCommand( "screenshot\n");
			cg.oldbutton = qtrue;
		}
	}

	if(menu_exit())
		return;

	CG_CheckFocusses();

	if(duel) CG_DisableStatMenu();

	// key handling
	if(!duel){
		if(cg.menumove){
			// cycle through the number keys (0-9)
			for( i=1;i<=9;i++){

				if(trap_Key_IsDown(i+48) && !cg.oldbutton){ //value of '0' is dec 48
					cg.menustat = i;
					if(cg.menustat <= 5)
						cg.menumove = qfalse;
					cg.menuitem = 1;
					cg.oldbutton = qtrue;
					ammo_numpressed= qtrue;
					CG_ClearFocusses();
					break;
				}
			}

			// movement keys
			if(Menu_IsKeyDown("+back") && !cg.oldbutton){
				if(cg.menustat < 7){
					cg.menustat++;
					cg.menumove = qtrue;
					cg.oldbutton = qtrue;
					CG_ClearFocusses();
				}
			} else if(Menu_IsKeyDown("+forward") && !cg.oldbutton){
				if(cg.menustat > 1){
					cg.menustat--;
					cg.menumove = qtrue;
					cg.oldbutton = qtrue;
					CG_ClearFocusses();
				}
			} else if((Menu_IsKeyDown("+moveright") ||
				Menu_IsKeyDown("+right")) && !cg.oldbutton
				&& cg.menustat < 6){
					cg.menumove = qfalse;
					cg.oldbutton = qtrue;
					cg.menuitem = 1;
					CG_ClearFocusses();
			}
		}


	/*
	====================================
	//prepare next menu screen
	====================================
	*/

		if(!cg.menustat || cg.menumove || cg.menustat > 5){

			//check if ammunition is required
			if(((Menu_IsKeyDown("mouse1") && !cg.oldbutton) || ammo_numpressed) &&
				cg.menustat <= 7 ){

				if(cg.menustat == 6)
					item = BG_ItemForAmmo(WP_BULLETS_CLIP);
				else
					item = BG_ItemForAmmo(WP_CART_CLIP);

				if(item){
					trap_SendConsoleCommand(va("cg_buy %s\n", item->classname));
					cg.oldbutton = qtrue;

					if(ammo_numpressed){
						CG_CloseBuyMenu();
					}
				}
			}
			// delete item menu data, item menu will not show up
			CG_ClearItemMenu();
			return;
		}
	}

	// is there anything to be updated?
	CG_RefreshItemMenu();

	count = menuItemCount;

	/*
	====================================
	//draw item box
	====================================
	*/

	for( i=1;i<=count;i++){

		if(trap_Key_IsDown(i+48) && !cg.oldbutton){ //value of '0' is dec 48
			cg.menuitem = i;
			cg.oldbutton = qtrue;
			numpressed = qtrue;
			CG_ClearFocusses();
			break;
		}
	}
	if(Menu_IsKeyDown("+back") && !cg.oldbutton){
		if(cg.menuitem < count){
			cg.menuitem++;
			cg.menumove = qfalse;
			cg.oldbutton = qtrue;
			CG_ClearFocusses();
		}
	} else if(Menu_IsKeyDown("+forward") && !cg.oldbutton){
		if(cg.menuitem > 1){
			cg.menuitem--;
			cg.menumove = qfalse;
			cg.oldbutton = qtrue;
			CG_ClearFocusses();
		}
	} else if((Menu_IsKeyDown("+moveleft") ||
		Menu_IsKeyDown("+left") || trap_Key_IsDown(0+48)) && !cg.oldbutton && !duel){
			cg.menumove = qtrue;
			cg.menuitem = 0;
			cg.oldbutton = qtrue;
			CG_ClearFocusses();
	} else if(trap_Key_IsDown(0+48) && duel && !cg.oldbutton){
		CG_CloseBuyMenu();
		return;
	}

	if(!cg.menuitem)
		return;

	if((Menu_IsKeyDown("mouse1") && !cg.oldbutton) || numpressed){
		qbool gatling = (cg.snap->ps.weapon == WP_GATLING &&
			cg.snap->ps.stats[STAT_GATLING_MODE]);

		item = CG_GetBuyItem();

		if(cg.snap->ps.stats[STAT_MONEY] < item->prize)
			return;
		// don't buy other special weapons if handling a gatling
		if(item->giType == IT_WEAPON && bg_weaponlist[item->giTag].wp_sort == WPS_GUN &&
			gatling){
			CG_Printf("Can't buy special weapons when using a gatling gun.\n");
			return;
		}
		if(item->giType == IT_WEAPON){
			if (bg_weaponlist[item->giTag].wp_sort == WPS_PISTOL){

				if(cg.snap->ps.stats[STAT_WEAPONS] & (1 << item->giTag) &&
					(cg.snap->ps.stats[STAT_FLAGS] & SF_SEC_PISTOL))
					return;

			} else if ((cg.snap->ps.stats[STAT_WEAPONS] & (1 << item->giTag))
				&& item->weapon_sort != WS_MISC){
				return;
			}
		}
		if(item->giTag == PW_BELT && cg.snap->ps.powerups[PW_BELT])
			return;
		if(item->giType == IT_ARMOR && cg.snap->ps.stats[STAT_ARMOR])
			return;
		if(item->giTag == PW_SCOPE && cg.snap->ps.powerups[PW_SCOPE])
			return;

		trap_SendConsoleCommand(va("cg_buy %s\n", item->classname));
//		trap_S_StartSound(NULL, cg.snap->ps.clientNum, CHAN_WEAPON, cgs.media.buySound);
		cg.oldbutton = qtrue;

		if(item->giType == IT_WEAPON && item->weapon_sort != WS_MISC && cgs.gametype != GT_DUEL){
			// don't change to the weapon bought when handling a gatling
			if(!gatling) {
				cg.lastusedweapon = cg.weaponSelect;
				cg.weaponSelect = item->giTag;
			}
		}

		if(numpressed){
			CG_CloseBuyMenu();
		}

	}
}

/*
=================
CG_Draw2D
=================
*/
static void CG_Draw2D(stereoFrame_t stereoFrame)
{
	int changeCrosshairFlags=0;

	if (cgs.orderPending && cg.time > cgs.orderTime) {
		CG_CheckOrderPending();
	}
	// if we are taking a levelshot for the menu, don't draw anything
	if ( cg.levelShot ) {
		return;
	}

	if ( cg_draw2D.integer == 0 ) {
		return;
	}
	
	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		CG_DrawIntermission();
		return;
	}

	if ( !CG_DrawFollow() ) {
		if(cg_warmupmessage.integer)
			CG_DrawWarmup();
	}

	CG_DrawSpectator();

	CG_ScanForCrosshairEntity(&changeCrosshairFlags);

	if ( cg.snap->ps.persistant[PERS_TEAM] >= TEAM_SPECTATOR ) {
		if(stereoFrame == STEREO_CENTER)
			CG_DrawCrosshair(changeCrosshairFlags);
		CG_DrawCrosshairNames(changeCrosshairFlags & (CHANGE_CROSSHAIR_TEAMMATE | CHANGE_CROSSHAIR_OPPONENT));
	} else {
		// don't draw any status if dead or the scoreboard is being explicitly shown
		if ( cg.snap->ps.stats[STAT_HEALTH] > 0 &&
			!cg.introstart) {

			if(stereoFrame == STEREO_CENTER)
				CG_DrawCrosshair(changeCrosshairFlags);
			CG_DrawCrosshairNames(changeCrosshairFlags & (CHANGE_CROSSHAIR_TEAMMATE | CHANGE_CROSSHAIR_OPPONENT));
			CG_DrawWeaponSelect();
			CG_DrawReward();
		}

		if ( cg_drawStatus.integer ) {
			Menu_PaintAll();
			CG_DrawTimedMenus();
			// Added for Smokin' Guns
			CG_DrawEscapeIndicators();
		}
		if ( cgs.gametype >= GT_TEAM ) {
			CG_DrawTeamInfo();
		}
	}

	CG_DrawVote();
	CG_DrawTeamVote();

	CG_DrawLowerRight();
	CG_DrawLowerLeft();

	// don't draw center string if scoreboard is up
	cg.scoreBoardShowing = CG_DrawScoreboard();
	CG_DrawCenterString();

	//draw menu in every case
	if( cg.menu == MENU_BUY){
		// intialize the positions
		menuBuy->window.rect.x = save_menuBuy.x;
		menuBuy->window.rect.y = save_menuBuy.y;
		Menu_UpdatePosition(menuBuy);
		menuItem->window.rect.x = save_menuItem.x;
		menuItem->window.rect.y = save_menuItem.y;
		Menu_UpdatePosition(menuItem);

		// initialize the whole menu
		CG_DrawBuyMenu();

		// paint
		Menu_Paint(menuBuy, qtrue);
		if(cg.menuitem){
			Menu_Paint(menuItem, qtrue);
		}

		// draw the cursor
		CG_DrawPic(cgs.cursorX-16, cgs.cursorY-16, 32, 32, cgDC.Assets.cursor);
	}
}

static void CheckWeaponChange(void){
	playerState_t *ps = &cg.snap->ps;
	static int delay;
	int cmdNum;
	usercmd_t cmd;

	// weapon change
	if (cg.markedweapon) {
		// fix for the fire-button-dead bug
		// Tequila comment: see http://www.quake3world.com/forum/viewtopic.php?f=16&t=17815
		if (delay & 1) {
			cmdNum = trap_GetCurrentCmdNumber();
			trap_GetUserCmd(cmdNum, &cmd);
			if (cmd.buttons & BUTTON_CHOOSE_MODE) {
				trap_SendConsoleCommand("-button14");
				//CG_Printf("CheckWeaponChange: BUTTON_CHOOSE_MODE FIX (delay=%d)\n", delay);
			}
		}
		delay++;

		if(Menu_IsKeyDown("+attack")) {
			if (cg.markedweapon != cg.weaponSelect)
				cg.lastusedweapon = cg.weaponSelect;

			if (ps->stats[STAT_FLAGS] & SF_SEC_PISTOL
				&& cg.markedweapon != WP_AKIMBO
				&& bg_weaponlist[cg.markedweapon].wp_sort == WPS_PISTOL
				&& cg.markedfirstpistol) {
			// Player has two same pistols and change to one pistol,
			// the one in the left holster.

				if (cg.markedweapon == cg.weaponSelect) {
				// Last selected weapon was the other pistol of the same type.
				// Before the change, the current one will become the secondary pistol
				// and will be in the left holster.
					cg.lastusedweapon = WP_SEC_PISTOL;
				}

				// Keep the selected weapon type.
				// cg.weaponSelect will be restored in
				// -> ./cgame/cg_view.c, CG_DrawActiveFrame()
				cg._weaponSelect = cg.markedweapon;
				cg.weaponSelect = WP_SEC_PISTOL;
			}
			else
				cg.weaponSelect = cg.markedweapon;

			cg.weaponSelectTime = cg.time;
			cg.markedweapon = 0;

			trap_Cvar_Set("cl_menu", "0");
		}

		if(trap_Key_IsDown(K_ESCAPE)) {
			cg.markedweapon = 0;
			trap_Cvar_Set("cl_menu", "0");
			// signal BUTTON_CHOOSE_CANCEL to the server
			trap_SendConsoleCommand("+button13; wait; -button13");
		}

		if((Menu_IsKeyDown("+button6") || ps->stats[STAT_GATLING_MODE])) {
			cg.markedweapon = 0;
			trap_Cvar_Set("cl_menu", "0");
		}
	} else {
		delay = 0;
	}

	// scope check
	if(cg.oldscopestat != ps->stats[STAT_WP_MODE] && ps->stats[STAT_WP_MODE] < 0
		&& ps->stats[STAT_WP_MODE] != -3 &&
		ps->weapon == WP_SHARPS && ps->powerups[PW_SCOPE]){

		cg.scopetime = cg.time;
		cg.scopedeltatime = SCOPE_TIME;

		if(ps->stats[STAT_WP_MODE] == -1)
			cg.scopedeltatime *= -1;
	}
	cg.oldscopestat = ps->stats[STAT_WP_MODE];

	// reload sound check
	/*if(cg.snap->ps.weapon2state != WEAPON_RELOADING &&
		cg.snap->ps.weaponstate != WEAPON_RELOADING){

		cg.r_weapon = qfalse;

	} else if(!cg.r_weapon){

		if(cg.snap->ps.weapon2state == WEAPON_RELOADING &&
			!(cg.snap->ps.stats[STAT_FLAGS] & SF_RELOAD2)){ // <-akimbos
			CG_PlayReloadSound(cg.snap->ps.weapon2, &cg_entities[cg.snap->ps.clientNum]);
			cg.r_weapon = qtrue;
		} else if(cg.snap->ps.weaponstate == WEAPON_RELOADING &&
			!(cg.snap->ps.stats[STAT_FLAGS] & SF_RELOAD)){ // <-akimbos
			CG_PlayReloadSound(cg.snap->ps.weapon, &cg_entities[cg.snap->ps.clientNum]);
			cg.r_weapon = qtrue;
		}
	}*/
}

static void CG_DrawNodes(void){
	refEntity_t		node;
	int i;

	memset( &node, 0, sizeof(node) );

	for(i=0; i < ai_nodepointer; i++){
		vec3_t angles;

		node.hModel = cgs.media.ai_node;
		VectorCopy( ai_nodes[i], node.origin );
		VectorCopy( ai_nodes[i], node.lightingOrigin );
		node.renderfx = 0;
		vectoangles(ai_angles[i], angles);
		AnglesToAxis( angles, node.axis );
		node.reType = RT_MODEL;
		trap_R_AddRefEntityToScene( &node);
	}
}

/*
=====================
CG_DrawActive

Perform all drawing needed to completely fill the screen
=====================
*/
void CG_DrawActive( stereoFrame_t stereoView ) {
	// optionally draw the info screen instead
	if ( !cg.snap ) {
		CG_DrawInformation();
		return;
	}

	CheckWeaponChange();

	CG_PlayMusic();

	if(cg_drawAINodes.integer)
		CG_DrawNodes();

	// clear around the rendered view if sized down
	CG_TileClear();

	if(stereoView != STEREO_CENTER)
		CG_DrawCrosshair3D();

	// draw 3D view
	trap_R_RenderScene( &cg.refdef );

	// draw status bar and other floating elements
 	CG_Draw2D(stereoView);
}



