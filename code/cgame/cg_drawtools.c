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
// cg_drawtools.c -- helper functions called by cg_draw, cg_scoreboard, cg_info, etc
#include "cg_local.h"

extern displayContextDef_t cgDC;

/*
================
CG_FillRect

Coordinates are 640*480 virtual values
=================
*/
void CG_FillRect( float x, float y, float width, float height, const float *color ) {
	trap_R_SetColor( color );

	UI_AdjustFrom640( &x, &y, &width, &height );
	trap_R_DrawStretchPic( x, y, width, height, 0, 0, 0, 0, cgs.media.whiteShader );

	trap_R_SetColor( NULL );
}

/*
================
CG_DrawSides

Coords are virtual 640x480
================
*/
void CG_DrawSides(float x, float y, float w, float h, float size) {
	UI_AdjustFrom640( &x, &y, &w, &h );
	size *= cgDC.xscale;//cgs.screenXScale;
	trap_R_DrawStretchPic( x, y, size, h, 0, 0, 0, 0, cgs.media.whiteShader );
	trap_R_DrawStretchPic( x + w - size, y, size, h, 0, 0, 0, 0, cgs.media.whiteShader );
}

void CG_DrawTopBottom(float x, float y, float w, float h, float size) {
	UI_AdjustFrom640( &x, &y, &w, &h );
	size *= cgDC.yscale; //cgs.screenYScale;
	trap_R_DrawStretchPic( x, y, w, size, 0, 0, 0, 0, cgs.media.whiteShader );
	trap_R_DrawStretchPic( x, y + h - size, w, size, 0, 0, 0, 0, cgs.media.whiteShader );
}
/*
================
UI_DrawRect

Coordinates are 640*480 virtual values
=================
*/
void CG_DrawRect( float x, float y, float width, float height, float size, const float *color ) {
	trap_R_SetColor( color );

  CG_DrawTopBottom(x, y, width, height, size);
  CG_DrawSides(x, y, width, height, size);

	trap_R_SetColor( NULL );
}



/*
================
CG_DrawPic

Coordinates are 640*480 virtual values
=================
*/
void CG_DrawPic( float x, float y, float width, float height, qhandle_t hShader ) {
	UI_AdjustFrom640( &x, &y, &width, &height );
	trap_R_DrawStretchPic( x, y, width, height, 0, 0, 1, 1, hShader );
}



/*
===============
CG_DrawChar

Coordinates and size in 640*480 virtual screen size
===============
*/
void CG_DrawChar( int x, int y, int width, int height, int ch ) {
	int row, col;
	float frow, fcol;
	float size;
	float	ax, ay, aw, ah;

	ch &= 255;

	if ( ch == ' ' ) {
		return;
	}

	ax = x;
	ay = y;
	aw = width;
	ah = height;
	UI_AdjustFrom640( &ax, &ay, &aw, &ah );

	row = ch>>4;
	col = ch&15;

	frow = row*0.0625;
	fcol = col*0.0625;
	size = 0.0625;

	trap_R_DrawStretchPic( ax, ay, aw, ah,
					   fcol, frow,
					   fcol + size, frow + size,
					   cgs.media.charsetShader );
}


/*
==================
CG_DrawStringExt

Draws a multi-colored string with a drop shadow, optionally forcing
to a fixed color.

Coordinates are at 640 by 480 virtual resolution
==================
*/
void CG_DrawStringExt( int x, int y, const char *string, const float *setColor,
		qbool forceColor, qbool shadow, int charWidth, int charHeight, int maxChars ) {
	vec4_t		color;
	const char	*s;
	int			xx;
	int			cnt;

	if (maxChars <= 0)
		maxChars = 32767; // do them all!

	// draw the drop shadow
	if (shadow) {
		color[0] = color[1] = color[2] = 0;
		color[3] = setColor[3];
		trap_R_SetColor( color );
		s = string;
		xx = x;
		cnt = 0;
		while ( *s && cnt < maxChars) {
			if ( Q_IsColorString( s ) ) {
				s += 2;
				continue;
			}
			CG_DrawChar( xx + 2, y + 2, charWidth, charHeight, *s );
			cnt++;
			xx += charWidth;
			s++;
		}
	}

	// draw the colored text
	s = string;
	xx = x;
	cnt = 0;
	trap_R_SetColor( setColor );
	while ( *s && cnt < maxChars) {
		if ( Q_IsColorString( s ) ) {
			if ( !forceColor ) {
				memcpy( color, g_color_table[ColorIndex(*(s+1))], sizeof( color ) );
				color[3] = setColor[3];
				trap_R_SetColor( color );
			}
			s += 2;
			continue;
		}
		CG_DrawChar( xx, y, charWidth, charHeight, *s );
		xx += charWidth;
		cnt++;
		s++;
	}
	trap_R_SetColor( NULL );
}

void CG_DrawBigString( int x, int y, const char *s, float alpha ) {
	float	color[4];

	color[0] = color[1] = color[2] = 1.0;
	color[3] = alpha;
	CG_DrawStringExt( x, y, s, color, qfalse, qtrue, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0 );
}

void CG_DrawBigStringColor( int x, int y, const char *s, vec4_t color ) {
	CG_DrawStringExt( x, y, s, color, qtrue, qtrue, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0 );
}

void CG_DrawSmallString( int x, int y, const char *s, float alpha ) {
	float	color[4];

	color[0] = color[1] = color[2] = 1.0;
	color[3] = alpha;

	UI_Text_Paint(x, y, 0.3f, color, s, 0, 0, 0);
}

void CG_DrawSmallStringColor( int x, int y, const char *s, vec4_t color ) {
	UI_Text_Paint(x, y, 0.3f, color, s, 0, 0, 3);
}

/*
=================
CG_DrawStrlen

Returns character count, skiping color escape codes
=================
*/
int CG_DrawStrlen( const char *str ) {
	const char *s = str;
	int count = 0;

	while ( *s ) {
		if ( Q_IsColorString( s ) ) {
			s += 2;
		} else {
			count++;
			s++;
		}
	}

	return count;
}

/*
=============
CG_TileClearBox

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
static void CG_TileClearBox( int x, int y, int w, int h, qhandle_t hShader ) {
	float	s1, t1, s2, t2;

	s1 = x/64.0;
	t1 = y/64.0;
	s2 = (x+w)/64.0;
	t2 = (y+h)/64.0;
	trap_R_DrawStretchPic( x, y, w, h, s1, t1, s2, t2, hShader );
}



/*
==============
CG_TileClear

Clear around a sized down screen
==============
*/
void CG_TileClear( void ) {
	int		top, bottom, left, right;
	int		w, h;

	w = cgs.glconfig.vidWidth;
	h = cgs.glconfig.vidHeight;

	if ( cg.refdef.x == 0 && cg.refdef.y == 0 &&
		cg.refdef.width == w && cg.refdef.height == h ) {
		return;		// full screen rendering
	}

	top = cg.refdef.y;
	bottom = top + cg.refdef.height-1;
	left = cg.refdef.x;
	right = left + cg.refdef.width-1;

	// clear above view screen
	CG_TileClearBox( 0, 0, w, top, cgs.media.backTileShader );

	// clear below view screen
	CG_TileClearBox( 0, bottom, w, h - bottom, cgs.media.backTileShader );

	// clear left of view screen
	CG_TileClearBox( 0, top, left, bottom - top + 1, cgs.media.backTileShader );

	// clear right of view screen
	CG_TileClearBox( right, top, w - right, bottom - top + 1, cgs.media.backTileShader );
}



/*
================
CG_FadeColor
================
*/
float *CG_FadeColor( int startMsec, int totalMsec ) {
	static vec4_t		color;
	int			t;

	if ( startMsec == 0 ) {
		return NULL;
	}

	t = cg.time - startMsec;

	if ( t >= totalMsec ) {
		return NULL;
	}

	// fade out
	if ( totalMsec - t < FADE_TIME ) {
		color[3] = ( totalMsec - t ) * 1.0/FADE_TIME;
	} else {
		color[3] = 1.0;
	}
	color[0] = color[1] = color[2] = 1;

	return color;
}


/*
================
CG_TeamColor
================
*/
float *CG_TeamColor( int team ) {
	static vec4_t	red = {1, 0.2f, 0.2f, 1};
	static vec4_t	blue = {0.2f, 0.2f, 1, 1};
	static vec4_t	other = {1, 1, 1, 1};
	static vec4_t	spectator = {0.7f, 0.7f, 0.7f, 1};

	switch ( team ) {
	case TEAM_RED:
	case TEAM_RED_SPECTATOR:
		return red;
	case TEAM_BLUE:
	case TEAM_BLUE_SPECTATOR:
		return blue;
	case TEAM_SPECTATOR:
		return spectator;
	default:
		return other;
	}
}

/*
=================
CG_GetColorForHealth
=================
*/
void CG_GetColorForHealth(const int health, vec4_t hcolor) {
	if (health < HEALTH_USUAL) {
		if (health > HEALTH_INJURED) {
			//Injuried
			hcolor[0] = g_color_table[cg_healthInjuredColor.integer & Q_COLORS_COUNT][0];
			hcolor[1] = g_color_table[cg_healthInjuredColor.integer & Q_COLORS_COUNT][1];
			hcolor[2] = g_color_table[cg_healthInjuredColor.integer & Q_COLORS_COUNT][2];
		} else if (cg.snap->ps.stats[STAT_HEALTH] > 0) {
			//Badly injuried
			hcolor[0] = g_color_table[cg_healthBadColor.integer & Q_COLORS_COUNT][0];
			hcolor[1] = g_color_table[cg_healthBadColor.integer & Q_COLORS_COUNT][1];
			hcolor[2] = g_color_table[cg_healthBadColor.integer & Q_COLORS_COUNT][2];
		} else {
			//Dead
			hcolor[0] = g_color_table[cg_healthDeadColor.integer & Q_COLORS_COUNT][0];
			hcolor[1] = g_color_table[cg_healthDeadColor.integer & Q_COLORS_COUNT][1];
			hcolor[2] = g_color_table[cg_healthDeadColor.integer & Q_COLORS_COUNT][2];
		}
	} else {
		//Usual color
		hcolor[0] = g_color_table[cg_healthUsualColor.integer & Q_COLORS_COUNT][0];
		hcolor[1] = g_color_table[cg_healthUsualColor.integer & Q_COLORS_COUNT][1];
		hcolor[2] = g_color_table[cg_healthUsualColor.integer & Q_COLORS_COUNT][2];
	}
}

/*
=================
UI_DrawProportionalString2
=================
*/
static int	propMap[128][3] = {
{0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1},
{0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1},

{0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1},
{0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1},

{0, 0, PROP_SPACE_WIDTH},		// SPACE
{7, 160, 5},	// !
{71, 223, 15},	// "
{117, 222, 18},	// #
{61, 159, 16},	// $
{87, 159, 22},	// %
{137, 160, 14},	// &
{6, 127, 6},	// '
{176, 159, 5},	// (
{189, 159, 6},	// )
{160, 156, 8},	// *
{29, 194, 9},	// +
{141, 223, 5},	// ,
{42, 130, 10},	// -
{43, 223, 6},	// .
{130, 190, 16},	// /

{6, 97, 15},	// 0
{30, 97, 10},	// 1
{48, 97, 14},	// 2
{71, 97, 14},	// 3
{94, 97, 16},	// 4
{118, 97, 14},	// 5
{139, 97, 15},	// 6
{163, 97, 13},	// 7
{185, 97, 15},	// 8
{208, 97, 15},	// 9
{29, 223, 6},	// :
{15,223, 6},	// ;
{129, 129, 16},	// <
{60, 130, 9},	// =
{154, 129, 16},	// >
{91, 221, 13},	// ?

{28, 160, 24},	// @
{7, 4, 19},		// A
{35, 4, 19},	// B
{61, 4, 19},	// C
{88, 4, 20},	// D
{116, 4, 18},	// E
{142, 4, 17},	// F
{167, 4, 18},	// G
{195, 4, 21},	// H
{225, 4, 10},	// I
{243, 4, 11},	// J

{6, 35, 19},	// K
{33, 35, 16},	// L
{57, 35, 27},	// M
{94, 35, 22},	// N
{124, 35, 20},	// O
{153, 35, 18},	// P
{179, 35, 20},	// Q
{207, 35, 19},	// R
{235, 35, 18},	// S

{6, 66, 18},	// T
{32, 66, 20},	// U
{61, 66, 20},	// V
{89, 66, 28},	// W
{125, 66, 23},	// X
{156, 66, 20},	// Y
{184, 66, 18},	// Z
{57, 192, 7},	// [
{105, 190, 16},	// '\'
{79, 192, 8},	// ]
{115, 158, 16},	// ^
{6, 194, 14},	// _

{6, 127, 6},	// '
{7, 4, 19},		// A
{35, 4, 19},	// B
{61, 4, 19},	// C
{88, 4, 20},	// D
{116, 4, 18},	// E
{142, 4, 17},	// F
{167, 4, 18},	// G
{195, 4, 21},	// H
{225, 4, 10},	// I
{243, 4, 11},	// J

{6, 35, 19},	// K
{33, 35, 16},	// L
{57, 35, 27},	// M
{94, 35, 22},	// N
{124, 35, 20},	// O
{153, 35, 18},	// P
{179, 35, 20},	// Q
{207, 35, 19},	// R
{235, 35, 18},	// S

{6, 66, 18},	// T
{32, 66, 20},	// U
{61, 66, 20},	// V
{89, 66, 28},	// W
{125, 66, 23},	// X
{156, 66, 20},	// Y
{184, 66, 18},	// Z
{164, 207, 11},	// {
{6, 221, 2},	// |
{184, 207, 11},	// }
{91, 130, 16},	// ~
{0, 0, -1}		// DEL
};

static int propMapB[26][3] = {
{11, 12, 33},
{49, 12, 31},
{85, 12, 31},
{120, 12, 30},
{156, 12, 21},
{183, 12, 21},
{207, 12, 32},

{13, 55, 30},
{49, 55, 13},
{66, 55, 29},
{101, 55, 31},
{135, 55, 21},
{158, 55, 40},
{204, 55, 32},

{12, 97, 31},
{48, 97, 31},
{82, 97, 30},
{118, 97, 30},
{153, 97, 30},
{185, 97, 25},
{213, 97, 30},

{11, 139, 32},
{42, 139, 51},
{93, 139, 32},
{126, 139, 31},
{158, 139, 25},
};

#define PROPB_GAP_WIDTH		4
#define PROPB_SPACE_WIDTH	12
#define PROPB_HEIGHT		36

/*
=================
UI_DrawBannerString
=================
*/
static void UI_DrawBannerString2( int x, int y, const char* str, vec4_t color )
{
	const char* s;
	unsigned char	ch;
	float	ax;
	float	ay;
	float	aw;
	float	ah;
	float	frow;
	float	fcol;
	float	fwidth;
	float	fheight;

	// draw the colored text
	trap_R_SetColor( color );

	ax = x * cgDC.xscale + cgs.screenXBias;//cgs.screenXScale + cgs.screenXBias;
	ay = y * cgDC.yscale;//cgs.screenXScale;

	s = str;
	while ( *s )
	{
		ch = *s & 127;
		if ( ch == ' ' ) {
			ax += ((float)PROPB_SPACE_WIDTH + (float)PROPB_GAP_WIDTH)* cgDC.xscale;//cgs.screenXScale;
		}
		else if ( ch >= 'A' && ch <= 'Z' ) {
			ch -= 'A';
			fcol = (float)propMapB[ch][0] / 256.0f;
			frow = (float)propMapB[ch][1] / 256.0f;
			fwidth = (float)propMapB[ch][2] / 256.0f;
			fheight = (float)PROPB_HEIGHT / 256.0f;
			aw = (float)propMapB[ch][2] * cgDC.xscale;//cgs.screenXScale;
			ah = (float)PROPB_HEIGHT * cgDC.xscale;//cgs.screenXScale;
			trap_R_DrawStretchPic( ax, ay, aw, ah, fcol, frow, fcol+fwidth, frow+fheight, cgs.media.charsetPropB );
			ax += (aw + (float)PROPB_GAP_WIDTH * cgDC.xscale);//cgs.screenXScale);
		}
		s++;
	}

	trap_R_SetColor( NULL );
}

void UI_DrawBannerString( int x, int y, const char* str, int style, vec4_t color ) {
	const char *	s;
	int				ch;
	int				width;
	vec4_t			drawcolor;

	// find the width of the drawn text
	s = str;
	width = 0;
	while ( *s ) {
		ch = *s;
		if ( ch == ' ' ) {
			width += PROPB_SPACE_WIDTH;
		}
		else if ( ch >= 'A' && ch <= 'Z' ) {
			width += propMapB[ch - 'A'][2] + PROPB_GAP_WIDTH;
		}
		s++;
	}
	width -= PROPB_GAP_WIDTH;

	switch( style & UI_FORMATMASK ) {
		case UI_CENTER:
			x -= width / 2;
			break;

		case UI_RIGHT:
			x -= width;
			break;

		case UI_LEFT:
		default:
			break;
	}

	if ( style & UI_DROPSHADOW ) {
		drawcolor[0] = drawcolor[1] = drawcolor[2] = 0;
		drawcolor[3] = color[3];
		UI_DrawBannerString2( x+2, y+2, str, drawcolor );
	}

	UI_DrawBannerString2( x, y, str, color );
}


int UI_ProportionalStringWidth( const char* str ) {
	const char *	s;
	int				ch;
	int				charWidth;
	int				width;

	s = str;
	width = 0;
	while ( *s ) {
		ch = *s & 127;
		charWidth = propMap[ch][2];
		if ( charWidth != -1 ) {
			width += charWidth;
			width += PROP_GAP_WIDTH;
		}
		s++;
	}

	width -= PROP_GAP_WIDTH;
	return width;
}

void UI_DrawProportionalString2( int x, int y, const char* str, vec4_t color, float sizeScale, qhandle_t charset )
{
	const char* s;
	unsigned char	ch;
	float	ax;
	float	ay;
	float	aw;
	float	ah;
	float	frow;
	float	fcol;
	float	fwidth;
	float	fheight;

	// draw the colored text
	trap_R_SetColor( color );

	ax = x * cgDC.xscale + cgs.screenXBias;//cgs.screenXScale + cgs.screenXBias;
	ay = y * cgDC.yscale;//cgs.screenXScale;

	s = str;
	while ( *s )
	{
		ch = *s & 127;
		if ( ch == ' ' ) {
			aw = (float)PROP_SPACE_WIDTH * cgDC.xscale * sizeScale;//cgs.screenXScale * sizeScale;
		} else if ( propMap[ch][2] != -1 ) {
			fcol = (float)propMap[ch][0] / 256.0f;
			frow = (float)propMap[ch][1] / 256.0f;
			fwidth = (float)propMap[ch][2] / 256.0f;
			fheight = (float)PROP_HEIGHT / 256.0f;
			aw = (float)propMap[ch][2] * cgDC.xscale * sizeScale;//cgs.screenXScale * sizeScale;
			ah = (float)PROP_HEIGHT * cgDC.xscale/*cgs.screenXScale*/ * sizeScale;
			trap_R_DrawStretchPic( ax, ay, aw, ah, fcol, frow, fcol+fwidth, frow+fheight, charset );
		} else {
			aw = 0;
		}

		ax += (aw + (float)PROP_GAP_WIDTH * cgDC.xscale/*cgs.screenXScale*/ * sizeScale);
		s++;
	}

	trap_R_SetColor( NULL );
}

/*
=================
UI_ProportionalSizeScale
=================
*/
float UI_ProportionalSizeScale( int style ) {
	if(  style & UI_SMALLFONT ) {
// Tequila comment: Adjusted the scale for small ui font
		return 0.45f;
	}

	return 1.00;
}


/*
=================
UI_DrawProportionalString
=================
*/
void UI_DrawProportionalString( int x, int y, const char* str, int style, vec4_t color ) {
	vec4_t	drawcolor;
	int		width;
	float	sizeScale;

	sizeScale = UI_ProportionalSizeScale( style );

	switch( style & UI_FORMATMASK ) {
		case UI_CENTER:
			width = UI_ProportionalStringWidth( str ) * sizeScale;
			x -= width / 2;
			break;

		case UI_RIGHT:
			width = UI_ProportionalStringWidth( str ) * sizeScale;
			x -= width;
			break;

		case UI_LEFT:
		default:
			break;
	}

	if ( style & UI_DROPSHADOW ) {
		drawcolor[0] = drawcolor[1] = drawcolor[2] = 0;
		drawcolor[3] = color[3];
		UI_DrawProportionalString2( x+2, y+2, str, drawcolor, sizeScale, cgs.media.charsetProp );
	}

	if ( style & UI_INVERSE ) {
		drawcolor[0] = color[0] * 0.8;
		drawcolor[1] = color[1] * 0.8;
		drawcolor[2] = color[2] * 0.8;
		drawcolor[3] = color[3];
		UI_DrawProportionalString2( x, y, str, drawcolor, sizeScale, cgs.media.charsetProp );
		return;
	}

	UI_DrawProportionalString2( x, y, str, color, sizeScale, cgs.media.charsetProp );
}
