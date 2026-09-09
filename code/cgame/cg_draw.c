/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).  

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// cg_draw.c -- draw all of the graphical elements during
// active (after loading) gameplay

#include "cg_local.h"
#include "../ui/ui_shared.h"
#include "../qcommon/q_unicode.h"

//----(SA) added to make it easier to raise/lower our statsubar by only changing one thing
#define STATUSBARHEIGHT 452
//----(SA) end

extern displayContextDef_t cgDC;

int sortedTeamPlayers[TEAM_MAXOVERLAY];
int numSortedTeamPlayers;

char systemChat[256];
char teamChat1[256];
char teamChat2[256];

////////////////////////
////////////////////////
////// new hud stuff
///////////////////////
///////////////////////

int CG_Text_Width( const char *text, int font, float scale, int limit ) {
	int count,len;
	float out;
	glyphInfo_t *glyph;
	float useScale;
	const char *s = text;
	fontInfoExtra_t *fnt = &cgDC.Assets.textFont;

	if ( font == UI_FONT_DEFAULT ) {
		if ( scale <= cg_smallFont.value ) {
			fnt = &cgDC.Assets.smallFont;
		} else if ( scale > cg_bigFont.value ) {
			fnt = &cgDC.Assets.bigFont;
		}
	} else {
		fnt = UI_SelectFont( &cgDC.Assets, font, fnt );
	}

	useScale = scale * fnt->glyphScale;
	out = 0;
	if ( text ) {
		len = Q_UTF8_Strlen( text );
		if ( limit > 0 && len > limit ) {
			len = limit;
		}
		count = 0;
		while ( s && *s && count < len ) {
			if ( Q_IsColorString( s ) ) {
				s += 2;
				continue;
			} else {
				glyph = Q_UTF8_GetGlyphExtended( fnt, Q_UTF8_CodePoint( s ) );
				out += glyph->xSkip;
				s += Q_UTF8_Width( s );
				count++;
			}
		}
	}
	return out * useScale;
}

int CG_Text_Height( const char *text, int font, float scale, int limit ) {
	int len, count;
	float max;
	glyphInfo_t *glyph;
	float useScale;
	const char *s = text;
	fontInfoExtra_t *fnt = &cgDC.Assets.textFont;

	if ( font == UI_FONT_DEFAULT ) {
		if ( scale <= cg_smallFont.value ) {
			fnt = &cgDC.Assets.smallFont;
		} else if ( scale > cg_bigFont.value ) {
			fnt = &cgDC.Assets.bigFont;
		}
	} else {
		fnt = UI_SelectFont( &cgDC.Assets, font, fnt );
	}

	useScale = scale * fnt->glyphScale;
	max = 0;
	if ( text ) {
		len = Q_UTF8_Strlen( text );
		if ( limit > 0 && len > limit ) {
			len = limit;
		}
		count = 0;
		while ( s && *s && count < len ) {
			if ( Q_IsColorString( s ) ) {
				s += 2;
				continue;
			} else {
				glyph = Q_UTF8_GetGlyphExtended( fnt, Q_UTF8_CodePoint( s ) );
				if ( max < glyph->height ) {
					max = glyph->height;
				}
				s += Q_UTF8_Width( s );
				count++;
			}
		}
	}
	return max * useScale;
}

void CG_Text_PaintChar( float x, float y, float width, float height, float scale, float s, float t, float s2, float t2, qhandle_t hShader ) {
	float w, h;
	w = width * scale;
	h = height * scale;
	CG_AdjustFrom640( &x, &y, &w, &h );
	trap_R_DrawStretchPic( x, y, w, h, s, t, s2, t2, hShader );
}

void CG_Text_Paint( float x, float y, int font, float scale, vec4_t color, const char *text, float adjust, int limit, int style ) {
	int len, count;
	vec4_t newColor;
	glyphInfo_t *glyph;
	float useScale;
	fontInfoExtra_t *fnt = &cgDC.Assets.textFont;

	if ( font == UI_FONT_DEFAULT ) {
		if ( scale <= cg_smallFont.value ) {
			fnt = &cgDC.Assets.smallFont;
		} else if ( scale > cg_bigFont.value ) {
			fnt = &cgDC.Assets.bigFont;
		}
	} else {
		fnt = UI_SelectFont( &cgDC.Assets, font, fnt );
	}

	useScale = scale * fnt->glyphScale;

	color[3] *= cg_hudAlpha.value;  // (SA) adjust for cg_hudalpha

	if ( text ) {
		const char *s = text;
		trap_R_SetColor( color );
		memcpy( &newColor[0], &color[0], sizeof( vec4_t ) );
		len = Q_UTF8_Strlen( text );
		if ( limit > 0 && len > limit ) {
			len = limit;
		}
		count = 0;
		while ( s && *s && count < len ) {
			glyph = Q_UTF8_GetGlyphExtended( fnt, Q_UTF8_CodePoint( s ) );
			if ( Q_IsColorString( s ) ) {
				memcpy( newColor, g_color_table[ColorIndex( *( s + 1 ) )], sizeof( newColor ) );
				newColor[3] = color[3];
				trap_R_SetColor( newColor );
				s += 2;
				continue;
			} else {
				float yadj = useScale * glyph->top;
				if ( style == ITEM_TEXTSTYLE_SHADOWED || style == ITEM_TEXTSTYLE_SHADOWEDMORE ) {
					int ofs = style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;
					colorBlack[3] = newColor[3];
					trap_R_SetColor( colorBlack );
					CG_Text_PaintChar( x + ofs, y - yadj + ofs,
									   glyph->imageWidth,
									   glyph->imageHeight,
									   useScale,
									   glyph->s,
									   glyph->t,
									   glyph->s2,
									   glyph->t2,
									   glyph->glyph );
					colorBlack[3] = 1.0;
					trap_R_SetColor( newColor );
				}
				CG_Text_PaintChar( x, y - yadj,
								   glyph->imageWidth,
								   glyph->imageHeight,
								   useScale,
								   glyph->s,
								   glyph->t,
								   glyph->s2,
								   glyph->t2,
								   glyph->glyph );
				x += ( glyph->xSkip * useScale ) + adjust;
				s += Q_UTF8_Width( s );
				count++;
			}
		}
		trap_R_SetColor( NULL );
	}
}


/*
================
CG_Draw3DModel

================
*/
void CG_Draw3DModel( float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t origin, vec3_t angles ) {
	refdef_t refdef;
	refEntity_t ent;

	if ( !cg_draw3dIcons.integer || !cg_drawIcons.integer ) {
		return;
	}

	CG_AdjustFrom640( &x, &y, &w, &h );

	memset( &refdef, 0, sizeof( refdef ) );

	memset( &ent, 0, sizeof( ent ) );
	AnglesToAxis( angles, ent.axis );
	VectorCopy( origin, ent.origin );
	ent.hModel = model;
	ent.customSkin = skin;
	ent.renderfx = RF_NOSHADOW;     // no stencil shadows

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear( refdef.viewaxis );

	refdef.fov_x = 30;
	refdef.fov_y = 30;

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.time = cg.time;

	refdef.rdflags |= RDF_DRAWSKYBOX;
	if ( !cg_skybox.integer ) {
		refdef.rdflags &= ~RDF_DRAWSKYBOX;
	}

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
	clipHandle_t cm;
	clientInfo_t    *ci;
	float len;
	vec3_t origin;
	vec3_t mins, maxs;

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
		origin[0] = len / 0.268;    // len / tan( fov/2 )

		// allow per-model tweaking
		VectorAdd( origin, ci->modelInfo->headOffset, origin );

		CG_Draw3DModel( x, y, w, h, ci->headModel, ci->headSkin, origin, headAngles );
//	} else if ( cg_drawIcons.integer ) {
//		CG_DrawPic( x, y, w, h, ci->modelIcon );
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
void CG_DrawFlagModel( float x, float y, float w, float h, int team ) {
	qhandle_t cm;
	float len;
	vec3_t origin, angles;
	vec3_t mins, maxs;

	VectorClear( angles );

	cm = cgs.media.redFlagModel;

	// offset the origin y and z to center the flag
	trap_R_ModelBounds( cm, mins, maxs );

	origin[2] = -0.5 * ( mins[2] + maxs[2] );
	origin[1] = 0.5 * ( mins[1] + maxs[1] );

	// calculate distance so the flag nearly fills the box
	// assume heads are taller than wide
	len = 0.5 * ( maxs[2] - mins[2] );
	origin[0] = len / 0.268;    // len / tan( fov/2 )

	angles[YAW] = 60 * sin( cg.time / 2000.0 );;

	CG_Draw3DModel( x, y, w, h,
					team == TEAM_RED ? cgs.media.redFlagModel : cgs.media.blueFlagModel,
					0, origin, angles );
}


/*
==============
CG_DrawKeyModel
==============
*/
void CG_DrawKeyModel( int keynum, float x, float y, float w, float h, int fadetime ) {
	qhandle_t cm;
	float len;
	vec3_t origin, angles;
	vec3_t mins, maxs;

	VectorClear( angles );

	cm = cg_items[keynum].models[0];

	// offset the origin y and z to center the model
	trap_R_ModelBounds( cm, mins, maxs );

	origin[2] = -0.5 * ( mins[2] + maxs[2] );
	origin[1] = 0.5 * ( mins[1] + maxs[1] );

//	len = 0.5 * ( maxs[2] - mins[2] );
	len = 0.75 * ( maxs[2] - mins[2] );
	origin[0] = len / 0.268;    // len / tan( fov/2 )

	angles[YAW] = 30 * sin( cg.time / 2000.0 );;

	CG_Draw3DModel( x, y, w, h, cg_items[keynum].models[0], 0, origin, angles );
}


/*
================
CG_DrawStatusBarHead

================
*/
static void CG_DrawStatusBarHead( float x ) {
	vec3_t angles;
	float size, stretch;
	float frac;

	VectorClear( angles );

	if ( cg_fixedAspect.integer == 2 ) {
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_BOTTOM);
	}

	if ( cg.damageTime && cg.time - cg.damageTime < DAMAGE_TIME ) {
		frac = (float)( cg.time - cg.damageTime ) / DAMAGE_TIME;
		size = ICON_SIZE * 1.25 * ( 1.5 - frac * 0.5 );

		stretch = size - ICON_SIZE * 1.25;
		// kick in the direction of damage
		x -= stretch * 0.5 + cg.viewDamage[cg.damageIndex].damageX * stretch * 0.5;

		cg.headStartYaw = 180 + cg.viewDamage[cg.damageIndex].damageX * 45;

		cg.headEndYaw = 180 + 20 * cos( crandom() * M_PI );
		cg.headEndPitch = 5 * cos( crandom() * M_PI );

		cg.headStartTime = cg.time;
		cg.headEndTime = cg.time + 100 + random() * 2000;
	} else {
		if ( cg.time >= cg.headEndTime ) {
			// select a new head angle
			cg.headStartYaw = cg.headEndYaw;
			cg.headStartPitch = cg.headEndPitch;
			cg.headStartTime = cg.headEndTime;
			cg.headEndTime = cg.time + 100 + random() * 2000;

			cg.headEndYaw = 180 + 20 * cos( crandom() * M_PI );
			cg.headEndPitch = 5 * cos( crandom() * M_PI );
		}

		size = ICON_SIZE * 1.25;
	}

	// if the server was frozen for a while we may have a bad head start time
	if ( cg.headStartTime > cg.time ) {
		cg.headStartTime = cg.time;
	}

	frac = ( cg.time - cg.headStartTime ) / (float)( cg.headEndTime - cg.headStartTime );
	frac = frac * frac * ( 3 - 2 * frac );
	angles[YAW] = cg.headStartYaw + ( cg.headEndYaw - cg.headStartYaw ) * frac;
	angles[PITCH] = cg.headStartPitch + ( cg.headEndPitch - cg.headStartPitch ) * frac;

	CG_DrawHead( x, 480 - size, size, size,
			cg.snap->ps.clientNum, angles );
}

/*
==============
CG_DrawStatusBarKeys
IT_KEY (this makes this routine easier to find in files...) (SA)
==============
*/
static void CG_DrawStatusBarKeys() {
	int i;
	float y = 0;    // start height is
	gitem_t *gi;
	int itemnum;
//	int		fadetime = 0;
	float   *fadeColor;


//----(SA)	added
	if ( cg.showItems ) {
		fadeColor = colorWhite;
	} else {
		fadeColor = CG_FadeColor( cg.itemFadeTime, 1000 );
	}

	if ( !fadeColor ) {
		return;
	}


	// (SA) just don't draw this stuff for now.  It's got fog issues I need to clean up

//	return;



	for ( i = 1; i < KEY_NUM_KEYS; i++ )
	{
		gi = BG_FindItemForKey( i, &itemnum );
		// if i've got the key...

		if ( cg.snap->ps.stats[STAT_KEYS] & ( 1 << gi->giTag ) ) {
			y += ICON_SIZE + 5;
			CG_DrawKeyModel( itemnum, 640 - ( 1.5 * ICON_SIZE ), y, ICON_SIZE, ICON_SIZE, cg.time + fadeColor[0] * 1000 );
		}
	}
}

/*
================
CG_DrawStatusBarFlag

================
*/
static void CG_DrawStatusBarFlag( float x, int team ) {
	CG_DrawFlagModel( x, 480 - ICON_SIZE, ICON_SIZE, ICON_SIZE, team );
}


/*
================
CG_DrawTeamBackground

================
*/
void CG_DrawTeamBackground( int x, int y, int w, int h, float alpha, int team ) {
	vec4_t hcolor;

	hcolor[3] = alpha;
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

//////////////////////
////// end new hud stuff
//////////////////////

// JOSEPH 4-25-00
/*
================
CG_DrawStatusBar

================
*/
static void CG_DrawStatusBar( void ) {
	int color;
	centity_t   *cent;
	playerState_t   *ps;
	int value, inclip;
	vec4_t hcolor;
	vec3_t angles;
//	vec3_t		origin;

	static float colors[4][4] = {
//		{ 0.2, 1.0, 0.2, 1.0 } , { 1.0, 0.2, 0.2, 1.0 }, {0.5, 0.5, 0.5, 1} };
		{ 1, 0.69, 0, 1.0 },        // normal
		{ 1.0, 0.2, 0.2, 1.0 },     // low health
		{0.5, 0.5, 0.5, 1},         // weapon firing
		{ 1, 1, 1, 1 }
	};                              // health > 100

	if ( cg_drawStatus.integer == 0 ) {
		return;
	}

	// draw the team background
	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED ) {
		hcolor[0] = 1;
		hcolor[1] = 0;
		hcolor[2] = 0;
		hcolor[3] = 0.33;
		trap_R_SetColor( hcolor );
		if ( cg_fixedAspect.integer == 2 ) {
			CG_SetScreenPlacement(PLACE_STRETCH, PLACE_BOTTOM);
			CG_DrawPic( 0, 420, 640, 60, cgs.media.teamStatusBar );
			CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
		} else {
			CG_DrawPic( 0, 420, 640, 60, cgs.media.teamStatusBar );
		}
		trap_R_SetColor( NULL );
	} else if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE ) {
		hcolor[0] = 0;
		hcolor[1] = 0;
		hcolor[2] = 1;
		hcolor[3] = 0.33;
		trap_R_SetColor( hcolor );
		if ( cg_fixedAspect.integer == 2 ) {
			CG_SetScreenPlacement(PLACE_STRETCH, PLACE_BOTTOM);
			CG_DrawPic( 0, 420, 640, 60, cgs.media.teamStatusBar );
			CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
		} else {
			CG_DrawPic( 0, 420, 640, 60, cgs.media.teamStatusBar );
		}
		trap_R_SetColor( NULL );
	}

	cent = &cg_entities[cg.snap->ps.clientNum];
	ps = &cg.snap->ps;

	VectorClear( angles );

	// draw any 3D icons first, so the changes back to 2D are minimized

	//----(SA) further change... we don't need to draw the ammo 3d model do we?
/*
	if ( cent->currentState.weapon && cg_weapons[ cent->currentState.weapon ].ammoModel ) {
		origin[0] = 70;
		origin[1] = 0;
		origin[2] = 0;
		angles[YAW] = 90 + 20 * sin( cg.time / 1000.0 );;
//----(SA) Wolf statusbar change
//			CG_Draw3DModel( CHAR_WIDTH*3 + TEXT_ICON_SPACE, STATUSBARHEIGHT -20, ICON_SIZE, ICON_SIZE,
//					cg_weapons[ cent->currentState.weapon ].ammoModel, 0, origin, angles );
//----(SA) end
	}
*/

	
	if ( cg_drawStatusHead.integer ) {
		CG_DrawStatusBarHead( 185 + CHAR_WIDTH_CG*3 + TEXT_ICON_SPACE );
	}

	CG_DrawStatusBarKeys();

	//----(SA) further change... we don't need to draw the armor do we?
/*
	if ( ps->stats[ STAT_ARMOR ] ) {
		origin[0] = 90;
		origin[1] = 0;
		origin[2] = -10;
		angles[YAW] = ( cg.time & 2047 ) * 360 / 2048.0;
//----(SA) Wolf statusbar change
//			CG_Draw3DModel( 370 + CHAR_WIDTH*3 + TEXT_ICON_SPACE, STATUSBARHEIGHT -20, ICON_SIZE, ICON_SIZE,
//					cgs.media.armorModel, 0, origin, angles );
//----(SA) end
	}
*/

	//
	// ammo
	//
	if ( cent->currentState.weapon ) {
		qhandle_t icon;
		float scale,halfScale;
		float wideOffset;

		if ( cg_fixedAspect.integer == 2 ) {
			CG_SetScreenPlacement(PLACE_RIGHT, PLACE_BOTTOM);
		}

		value = ps->ammo[BG_FindAmmoForWeapon( cent->currentState.weapon )];
		inclip = ps->ammoclip[BG_FindClipForWeapon( cent->currentState.weapon )];

		if ( value > -1 ) {
			if ( cg.predictedPlayerState.weapon == WP_KNIFE ) {
				color = 3; 
			} else if ( ( cg.predictedPlayerState.weaponstate == WEAPON_FIRING || cg.predictedPlayerState.weaponstate == WEAPON_FIRINGALT )
					&& cg.predictedPlayerState.weaponTime > 100 ) {
				// draw as dark grey when reloading
				color = 2;  // dark grey
			} else {
				if ( value >= 0 ) {
					color = 0;  // green
				} else {
					color = 1;  // red
				}
			}
			trap_R_SetColor( colors[color] );

			// pulsing grenade icon to help the player 'count' in their head
			if ( ps->grenadeTimeLeft ) {
				if ( ps->weapon == WP_DYNAMITE ) {

				} else {
					if ( ( ( cg.grenLastTime ) % 1000 ) < ( ( ps->grenadeTimeLeft ) % 1000 ) ) {
						switch ( ps->grenadeTimeLeft / 1000 ) {
						case 3:
							trap_S_StartLocalSound( cgs.media.grenadePulseSound4, CHAN_LOCAL_SOUND );
							break;
						case 2:
							trap_S_StartLocalSound( cgs.media.grenadePulseSound3, CHAN_LOCAL_SOUND );
							break;
						case 1:
							trap_S_StartLocalSound( cgs.media.grenadePulseSound2, CHAN_LOCAL_SOUND );
							break;
						case 0:
							trap_S_StartLocalSound( cgs.media.grenadePulseSound1, CHAN_LOCAL_SOUND );
							break;
						}
					}
				}

				// only grow the icon when it's actually the grenade being cooked, not the equipped weapon
				if ( ps->weapon == WP_GRENADE_LAUNCHER || ps->weapon == WP_GRENADE_PINEAPPLE || ps->weapon == WP_DYNAMITE ) {
					scale = (float)( ( ps->grenadeTimeLeft ) % 1000 ) / 100.0f;
					halfScale = scale * 0.5f;
				} else {
					scale = halfScale = 0;
				}

				cg.grenLastTime = ps->grenadeTimeLeft;
			} else {
				scale = halfScale = 0;
			}

			if (GetWeaponTableData(cg.predictedPlayerState.weapon)->iconDrawSize == WEAPON_ICON_WIDE_OFFSET)
			{
				wideOffset = -38;
			}
			else
			{
				wideOffset = 0;
			}

			// don't draw ammo value for knife
			if ( cg.predictedPlayerState.weapon != WP_KNIFE ) {
				if ( cgs.dmflags & DF_NO_WEAPRELOAD ) {
					CG_DrawBigString2( ( 580 - 23 + 35 ) + wideOffset, STATUSBARHEIGHT, va( "%d.", value ), cg_hudAlpha.value );
				} else if ( value ) {
					CG_DrawBigString2( ( 580 - 23 + 35 ) + wideOffset, STATUSBARHEIGHT, va( "%d/%d", inclip, value ), cg_hudAlpha.value );
				} else {
					CG_DrawBigString2( ( 580 - 23 + 35 ) + wideOffset, STATUSBARHEIGHT, va( "%d", inclip ), cg_hudAlpha.value );
				}
			}

			icon = cg_weapons[ cg.predictedPlayerState.weapon ].weaponIcon[0];
			if ( icon ) {
				CG_DrawPic( ( ( 530 + 68 ) - halfScale ) + wideOffset,  ( 446 - 10 ) - halfScale, ( 38 + scale ) - wideOffset, 38 + scale, icon );
			}

			trap_R_SetColor( NULL );

			// if we didn't draw a 3D icon, draw a 2D icon for ammo
			if ( !cg_draw3dIcons.integer ) {
				qhandle_t icon;

				icon = cg_weapons[ cg.predictedPlayerState.weapon ].ammoIcon;
				if ( icon ) {
					CG_DrawPic( CHAR_WIDTH_CG * 3 + TEXT_ICON_SPACE, STATUSBARHEIGHT, ICON_SIZE, ICON_SIZE, icon );
				}
			}
		}
	}

	if ( cg_fixedAspect.integer == 2 ) {
		CG_SetScreenPlacement(PLACE_LEFT, PLACE_BOTTOM);
	}

	//
	// health
	//
	value = ps->stats[STAT_HEALTH];
	if ( value > 100 ) {
		trap_R_SetColor( colors[3] );       // white
	} else if ( value > 25 ) {
		trap_R_SetColor( colors[0] );   // green
	} else if ( value > 0 ) {
		color = ( cg.time >> 8 ) & 1; // flash
		trap_R_SetColor( colors[color] );
	} else {
		trap_R_SetColor( colors[1] );   // red
	}

	// stretch the health up when taking damage
//----(SA) Wolf statusbar change
//	CG_DrawField ( 185, STATUSBARHEIGHT, 3, value);
	{
		char printme[16];
		Com_sprintf( printme, sizeof( printme ), "%d", value );
		//CG_DrawBigString( 185, STATUSBARHEIGHT, printme, cg_hudAlpha.value );
		CG_DrawBigString2( 16 + 23 + 43, STATUSBARHEIGHT, printme, cg_hudAlpha.value );
	}
//----(SA) end
	CG_ColorForHealth( hcolor );
	trap_R_SetColor( hcolor );


	//
	// armor
	//
	value = ps->stats[STAT_ARMOR];
	if ( value > 0 ) {
		trap_R_SetColor( colors[0] );
//----(SA) Wolf statusbar change
//		CG_DrawField (370, STATUSBARHEIGHT, 3, value);
		{
			char printme[16];
			Com_sprintf( printme, sizeof( printme ), "%d", value );
			//CG_DrawBigString( 370, STATUSBARHEIGHT, printme, cg_hudAlpha.value );
			CG_DrawBigString2( 200, STATUSBARHEIGHT, printme, cg_hudAlpha.value );
		}
//----(SA) end
		trap_R_SetColor( NULL );
//----(SA) Wolf statusbar change
//		CG_DrawPic( 370 + CHAR_WIDTH*3 + TEXT_ICON_SPACE, STATUSBARHEIGHT, ICON_SIZE, ICON_SIZE, cgs.media.armorIcon );
//----(SA) end
	}
}
// END JOSEPH

/*
===========================================================================================

  UPPER RIGHT CORNER

===========================================================================================
*/

#define UPPERRIGHT_X 500

/*
================
CG_DrawAttacker

================
*/
static float CG_DrawAttacker( float y ) {
	int t;
	float size;
	vec3_t angles;
	const char  *info;
	const char  *name;
	int clientNum;

	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 ) {
		return y;
	}

	if ( !cg.attackerTime ) {
		return y;
	}

	clientNum = cg.predictedPlayerState.persistant[PERS_ATTACKER];
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS || clientNum == cg.snap->ps.clientNum ) {
		return y;
	}

	if ( !cgs.clientinfo[clientNum].infoValid ) {
		cg.attackerTime = 0;
		return y;
	}

	t = cg.time - cg.attackerTime;
	if ( t > ATTACKER_HEAD_TIME ) {
		cg.attackerTime = 0;
		return y;
	}

	size = ICON_SIZE * 1.25;

	angles[PITCH] = 0;
	angles[YAW] = 180;
	angles[ROLL] = 0;
	CG_DrawHead( UPPERRIGHT_X - size, y, size, size, clientNum, angles );

	info = CG_ConfigString( CS_PLAYERS + clientNum );
	name = Info_ValueForKey(  info, "n" );
	y += size;
	CG_DrawBigString( UPPERRIGHT_X - ( Q_PrintStrlen( name ) * BIGCHAR_WIDTH ), y, name, 0.5 );

	return y + BIGCHAR_HEIGHT + 2;
}

/*
==================
CG_DrawSnapshot
==================
*/
static float CG_DrawSnapshot( float y ) {
	char        *s;
	int w;

	s = va( "time:%i snap:%i cmd:%i", cg.snap->serverTime,
			cg.latestSnapshotNum, cgs.serverCommandSequence );
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;

	CG_DrawBigString( UPPERRIGHT_X - w, y + 2, s, 1.0F );

	return y + BIGCHAR_HEIGHT + 4;
}

/*
==================
CG_DrawFPS
==================
*/
#define FPS_FRAMES  4
static float CG_DrawFPS( float y ) {
	char        *s;
	static int previousTimes[FPS_FRAMES];
	static int index;
	int i, total;
	int fps;
	static int previous;
	int t, frameTime;

	// don't use serverTime, because that will be drifting to
	// correct for internet lag changes, timescales, timedemos, etc
	t = trap_Milliseconds();
	frameTime = t - previous;
	previous = t;

	previousTimes[index % FPS_FRAMES] = frameTime;
	index++;
	if ( index > FPS_FRAMES ) {
		// average multiple frames together to smooth changes out a bit
		total = 0;
		for ( i = 0 ; i < FPS_FRAMES ; i++ ) {
			total += previousTimes[i];
		}
		if ( !total ) {
			total = 1;
		}
		fps = 1000 * FPS_FRAMES / total;

		s = va( "%ifps", fps );

		CG_DrawBigString( UPPERRIGHT_X, y + 2, s, 1.0F );
	}

	return y + BIGCHAR_HEIGHT + 4;
}

/*
=================
CG_DrawTimer
=================
*/
static float CG_DrawTimer( float y ) {
	char        *s;
	int mins, seconds, tens;
	int msec;

	msec = cg.time - cgs.levelStartTime;
	// -NERVE - SMF

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;

	s = va( "%i:%i%i", mins, tens, seconds );

	CG_DrawBigString( UPPERRIGHT_X, y + 2, s, 1.0F );

	return y + BIGCHAR_HEIGHT + 4;
}

/*
=================
CG_DrawCoopOverlay
=================
*/

int maxCharsBeforeOverlay;

#define TEAM_OVERLAY_MAXNAME_WIDTH  16
#define TEAM_OVERLAY_MAXLOCATION_WIDTH  20

// CG_Text_Paint_Ext's y is a baseline (draws upward); this file's y positions were written for the old top-anchored CG_DrawStringExt, so shift down by the measured text height
static void CG_ChatTextPaint( float x, float y, float scalex, float scaley, vec4_t color, const char *text, float adjust, int limit, int style, fontInfoExtra_t *font ) {
	CG_Text_Paint_Ext( x, y + CG_Text_Height_Ext( text, scaley, limit, font ), scalex, scaley, color, text, adjust, limit, style, font );
}

static float CG_DrawCoopOverlay( float y ) {
	int x, w, h, xx;
	int i, len;
	const char *p;
	vec4_t hcolor, bgcolor;
	int pwidth, lwidth;
	int plyrs;
	char st[16];
	char nameClean[MAX_QPATH];
	clientInfo_t *ci;
	// NERVE - SMF
	char classType[2] = { 0, 0 };
	int val;
	vec4_t deathcolor, damagecolor;          // JPW NERVE
	float       *pcolor;
	int chW, chH;
	// -NERVE - SMF

	if ( !cg_drawTeamOverlay.integer ) {
		return y;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		return y;
	}

	deathcolor[0] = 1;
	deathcolor[1] = 0;
	deathcolor[2] = 0;
	deathcolor[3] = cg_hudAlpha.value;
	damagecolor[0] = 1;
	damagecolor[1] = 1;
	damagecolor[2] = 0;
	damagecolor[3] = cg_hudAlpha.value;
	maxCharsBeforeOverlay = 80;

	// TTF cell size replacing the fixed TINYCHAR grid; chH measured separately, not assumed square
	chW = CG_Text_Width_Ext( "0", CG_CHAT_TEXT_SCALE, 0, CG_CHAT_FONT );
	chH = CG_Text_Height_Ext( "Mg", CG_CHAT_TEXT_SCALE, 0, CG_CHAT_FONT );

	plyrs = 0;

	// max player name width
	pwidth = 0;
	for ( i = 0; i < numSortedTeamPlayers; i++ ) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM] ) {
			plyrs++;
			len = CG_DrawStrlen( ci->name );
			if ( len > pwidth ) {
				pwidth = len;
			}
		}
	}

	if ( !plyrs ) {
		return y;
	}

	if ( pwidth > TEAM_OVERLAY_MAXNAME_WIDTH ) {
		pwidth = TEAM_OVERLAY_MAXNAME_WIDTH;
	}

#if 1
	// max location name width
	lwidth = 0;
	for ( i = 0; i < numSortedTeamPlayers; i++ ) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM] && CG_ConfigString( CS_LOCATIONS + ci->location ) ) {
			len = CG_DrawStrlen( CG_ConfigString( CS_LOCATIONS + ci->location ) );
			if ( len > lwidth ) {
				lwidth = len;
			}
		}
	}
#else
	// max location name width
	lwidth = 0;
	for ( i = 1; i < MAX_LOCATIONS; i++ ) {
		p = CG_ConfigString( CS_LOCATIONS + i );
		if ( p && *p ) {
			len = CG_DrawStrlen( p );
			if ( len > lwidth ) {
				lwidth = len;
			}
		}
	}
#endif

	if ( lwidth > TEAM_OVERLAY_MAXLOCATION_WIDTH ) {
		lwidth = TEAM_OVERLAY_MAXLOCATION_WIDTH;
	}

	w = ( pwidth + lwidth + 7 + 7 ) * chW;         // JPW NERVE was +4+7

	x = 640 - w - 1;     // JPW was -32
	h = plyrs * chH;

	// DHM - Nerve :: Set the max characters that can be printed before the left edge
	if ( cg_fixedAspect.integer == 2 && ( cgs.glconfig.vidWidth * 480.0 > cgs.glconfig.vidHeight * 640.0 ) ) {
		maxCharsBeforeOverlay = ( ( ( cgs.glconfig.vidWidth / cgs.screenXScale ) - w - 1 ) / chW ) - 1;
	} else {
		maxCharsBeforeOverlay = ( x / chW ) - 1;
	}

	hcolor[0] = 0.25f;
	hcolor[1] = 0.25f;
	hcolor[2] = 0.5f;
	hcolor[3] = 0.25f * cg_hudAlpha.value;

	CG_FillRect( x,y,w,h,hcolor );
	VectorSet( hcolor, 0.4f, 0.4f, 0.4f );
	hcolor[3] = cg_hudAlpha.value;
	CG_DrawRect( x - 1, y, w + 2, h + 2, 1, hcolor );


	for ( i = 0; i < numSortedTeamPlayers; i++ ) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM] ) {
			// CG_Text_Paint_Ext's y is a baseline (draws upward); shift down by chH so text top lands at the row's y, matching CG_FillRect's rect below
			float ty = y + chH;

			// NERVE - SMF
			// determine class type
			//val = cg_entities[ ci->clientNum ].currentState.teamNum;
			val = ci->curWeapon;

			if (val > WP_NONE && val < WP_NUM_WEAPONS)
			{
				const ammotable_t *wt = GetWeaponTableData(val);
				weaponClass_t wc = wt->weaponClass;

				if (wc & WEAPON_CLASS_MELEE)
				{
					classType[0] = 'K'; // knife / melee
				}
				else if (wc & (WEAPON_CLASS_PISTOL | WEAPON_CLASS_AKIMBO))
				{
					classType[0] = 'P'; // pistol
				}
				else if (wc & WEAPON_CLASS_SMG)
				{
					classType[0] = 'S'; // smg
				}
				else if (wc & (WEAPON_CLASS_GRENADE | WEAPON_CLASS_DYNAMITE | WEAPON_CLASS_RIFLENADE))
				{
					classType[0] = 'E'; // explosive
				}
				else if (wc & (WEAPON_CLASS_RIFLE_BOLTACTION |
							   WEAPON_CLASS_RIFLE_AUTO |
							   WEAPON_CLASS_ASSAULT_RIFLE ))
				{
					classType[0] = 'R'; // rifle
				}
				else if (wc & (WEAPON_CLASS_MG |
							   WEAPON_CLASS_LAUNCHER |
							   WEAPON_CLASS_FLAMER |
							   WEAPON_CLASS_MINIGUN |
							   WEAPON_CLASS_ENERGY))
				{
					classType[0] = 'H'; // heavy weapon
				}
				else
				{
					classType[0] = 'X';
				}
			}
			else
			{
				classType[0] = 'X';
			}

			Com_sprintf( st, sizeof( st ), "%s", classType );

			xx = x + chW;


			hcolor[0] = hcolor[1] = 1.0;
			hcolor[2] = 0.0;
			hcolor[3] = cg_hudAlpha.value;

			CG_Text_Paint_Ext( xx, ty, CG_CHAT_TEXT_SCALE, CG_CHAT_TEXT_SCALE, hcolor,
							  st, 0, 1, 0, CG_CHAT_FONT );

			hcolor[0] = hcolor[1] = hcolor[2] = 1.0;
			hcolor[3] = cg_hudAlpha.value;

			xx = x + 3 * chW;

			// JPW NERVE
			if ( ci->health > 80 ) {
				pcolor = hcolor;
			} else if ( ci->health > 0 ) {
				pcolor = damagecolor;
			} else {
				pcolor = deathcolor;
			}
			// jpw

			// if that guy killed someone flash his name green
			if ( ci->lastteamkilltime + 5000 >= cg.time ) {
				bgcolor[0] = 1.0;
				bgcolor[1] = 0.0;
				bgcolor[2] = 0.0;
				bgcolor[3] = cg_hudAlpha.value;
				CG_FillRect( xx, y + 1, CG_DrawStrlen( ci->name ) * chW, chH - 1, bgcolor );
			} else if ( ci->lastkilltime + 1000 >= cg.time ) {
				pcolor[0] = 0.0;
				pcolor[1] = 1.0;
				pcolor[2] = 0.0;
			}

			// strip ^N color codes from a copy so the player's name can't override the fixed color
			Q_strncpyz( nameClean, ci->name, sizeof( nameClean ) );
			Q_StripColorCodes( nameClean );
			CG_Text_Paint_Ext( xx, ty, CG_CHAT_TEXT_SCALE, CG_CHAT_TEXT_SCALE, pcolor,
							  nameClean, 0, TEAM_OVERLAY_MAXNAME_WIDTH, 0, CG_CHAT_FONT );

			if ( lwidth ) {
				p = CG_ConfigString( CS_LOCATIONS + ci->location );
				if ( !p || !*p ) {
					p = "unknown";
				}
				//p = CG_TranslateString( p );
				len = CG_DrawStrlen( p );
				if ( len > lwidth ) {
					len = lwidth;
				}

				xx = x + chW * 5 + chW * pwidth +
					 ( ( lwidth / 2 - len / 2 ) * chW );
				CG_Text_Paint_Ext( xx, ty, CG_CHAT_TEXT_SCALE, CG_CHAT_TEXT_SCALE, hcolor,
								  p, 0, TEAM_OVERLAY_MAXLOCATION_WIDTH, 0, CG_CHAT_FONT );
			}

			Com_sprintf( st, sizeof( st ), "%3i/%i", ci->health, ci->armor );             // JPW NERVE pulled class stuff since it's at top now

			xx = x + chW * 6 + chW * pwidth + chW * lwidth;

			CG_Text_Paint_Ext( xx, ty, CG_CHAT_TEXT_SCALE, CG_CHAT_TEXT_SCALE, pcolor,
							  st, 0, 7, 0, CG_CHAT_FONT );

			y += chH;
		}
	}

	return y;
}

/*
=====================
CG_DrawUpperRight

=====================
*/
static void CG_DrawUpperRight(stereoFrame_t stereoFrame) {
	float y;

	y = 0;

	if ( cg_fixedAspect.integer == 2 ) {
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_TOP);
	} else if ( cg_fixedAspect.integer == 1 ) {
		CG_SetScreenPlacement(PLACE_LEFT, PLACE_TOP);
	}

	if ( cgs.gametype <= GT_COOP ) {
		y = CG_DrawCoopOverlay( y );
	}
	if ( cg_drawSnapshot.integer ) {
		y = CG_DrawSnapshot( y );
	}
	if (cg_drawFPS.integer && (stereoFrame == STEREO_CENTER || stereoFrame == STEREO_RIGHT)) {
		y = CG_DrawFPS( y );
	}
	if ( cg_drawTimer.integer ) {
		y = CG_DrawTimer( y );
	}
// (SA) disabling drawattacker for the time being
	if ( cg_oldWolfUI.integer ) {
		if ( cg_drawAttacker.integer ) {
			CG_DrawAttacker( y );
		}
	}
//----(SA)	end
	if ( cg_fixedAspect.integer ) {
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
	}
}

/*
===========================================================================================

  LOWER RIGHT CORNER

===========================================================================================
*/

/*
=================
CG_DrawTeamInfo
=================
*/
static void CG_DrawTeamInfo( void ) {
	int i;
	vec4_t hcolor;
	int chatHeight;
	float alphapercent;

#define CHATLOC_Y 385 // bottom end
#define CHATLOC_X 0

	if ( cg_teamChatHeight.integer < TEAMCHAT_HEIGHT ) {
		chatHeight = cg_teamChatHeight.integer;
	} else {
		chatHeight = TEAMCHAT_HEIGHT;
	}
	if ( chatHeight <= 0 ) {
		return; // disabled
	}

	if ( cg_fixedAspect.integer == 2 ) {
		CG_SetScreenPlacement( PLACE_LEFT, PLACE_BOTTOM );
	}

	if ( cgs.teamLastChatPos != cgs.teamChatPos ) {
		if ( cg.time - cgs.teamChatMsgTimes[cgs.teamLastChatPos % chatHeight] > cg_teamChatTime.integer ) {
			cgs.teamLastChatPos++;
		}

// JPW NERVE rewritten to support first pass at fading chat messages
		for ( i = cgs.teamChatPos - 1; i >= cgs.teamLastChatPos; i-- ) {
			alphapercent = 1.0f - ( cg.time - cgs.teamChatMsgTimes[i % chatHeight] ) / (float)( cg_teamChatTime.integer );
			if ( alphapercent > 1.0f ) {
				alphapercent = 1.0f;
			} else if ( alphapercent < 0.f ) {
				alphapercent = 0.f;
			}

			if ( cgs.clientinfo[cg.clientNum].team == TEAM_RED ) {
				hcolor[0] = 1;
				hcolor[1] = 0;
				hcolor[2] = 0;
			} else if ( cgs.clientinfo[cg.clientNum].team == TEAM_BLUE ) {
				hcolor[0] = 0;
				hcolor[1] = 0;
				hcolor[2] = 1;
			} else {
				hcolor[0] = 0;
				hcolor[1] = 1;
				hcolor[2] = 0;
			}

			hcolor[3] = 0.33f * alphapercent;

			trap_R_SetColor( hcolor );
			if ( cg_fixedAspect.integer == 2 ) {
				CG_DrawPic( CHATLOC_X, CHATLOC_Y - ( cgs.teamChatPos - i ) * TINYCHAR_HEIGHT, cgs.glconfig.vidWidth, TINYCHAR_HEIGHT, cgs.media.teamStatusBar );
			} else {
				CG_DrawPic( CHATLOC_X, CHATLOC_Y - ( cgs.teamChatPos - i ) * TINYCHAR_HEIGHT, 640, TINYCHAR_HEIGHT, cgs.media.teamStatusBar );
			}
			hcolor[0] = hcolor[1] = hcolor[2] = 1.0;
			hcolor[3] = alphapercent;
			trap_R_SetColor( hcolor );

			CG_ChatTextPaint( CHATLOC_X + TINYCHAR_WIDTH,
							  CHATLOC_Y - ( cgs.teamChatPos - i ) * TINYCHAR_HEIGHT,
							  CG_CHAT_TEXT_SCALE, CG_CHAT_TEXT_SCALE, hcolor,
							  cgs.teamChatMsgs[i % chatHeight], 0, 0, 0, CG_CHAT_FONT_UNICODE );
		}
// jpw
	}
}

//----(SA)	modified
/*
===================
CG_DrawPickupItem
===================
*/
static void CG_DrawPickupItem( void ) {
	int value;
	float   *fadeColor;
	char pickupText[256];
	float color[4];
	int w = 0;
	float pickupScale = CG_CHAT_TEXT_SCALE * ( 10.0f / SMALLCHAR_WIDTH );

	if ( cg_fixedAspect.integer == 2 ) {
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_BOTTOM);
	}

	value = cg.itemPickup;
	if ( value ) {
		fadeColor = CG_FadeColor( cg.itemPickupTime, 3000 );
		if ( fadeColor ) {
			CG_RegisterItemVisuals( value );

			//----(SA)	so we don't pick up all sorts of items and have it print "0 <itemname>"
			if ( bg_itemlist[ value ].giType == IT_AMMO || bg_itemlist[ value ].giType == IT_HEALTH || bg_itemlist[value].giType == IT_POWERUP ) {
				if ( bg_itemlist[ value ].world_model[2] ) {   // this is a multi-stage item
					// FIXME: print the correct amount for multi-stage
					Com_sprintf( pickupText, sizeof( pickupText ), "%s", cgs.itemPrintNames[ value ] );
				} else {
					// removing print of the hardcoded values for pickups since there is a dropammo command with an arbitrary amount.
					//if ( bg_itemlist[ value ].gameskillnumber[cg_gameSkill.integer] > 1 ) {
					//	Com_sprintf( pickupText, sizeof( pickupText ), "%i  %s", bg_itemlist[ value ].gameskillnumber[cg_gameSkill.integer], cgs.itemPrintNames[ value ] );
					//} else {
					Com_sprintf( pickupText, sizeof( pickupText ), "%s", cgs.itemPrintNames[value] );
					//}
				}
			} else {
				Com_sprintf( pickupText, sizeof( pickupText ), "%s", cgs.itemPrintNames[value] );
			}

			color[0] = color[1] = color[2] = 1.0;
			color[3] = fadeColor[0];
#ifdef LOCALISATION
			{
				const char *translated = CG_TranslateString( pickupText );
				w = CG_Text_Width_Ext( translated, pickupScale, 0, CG_CHAT_FONT );
				CG_ChatTextPaint( 320 - ( w / 2 ), 398, pickupScale, pickupScale, color, translated, 0, 0, ITEM_TEXTSTYLE_SHADOWED, CG_CHAT_FONT );
			}
#else
			w = CG_Text_Width_Ext( pickupText, pickupScale, 0, CG_CHAT_FONT );
			CG_ChatTextPaint( 320 - ( w / 2 ), 398, pickupScale, pickupScale, color, pickupText, 0, 0, ITEM_TEXTSTYLE_SHADOWED, CG_CHAT_FONT );
#endif

			trap_R_SetColor( NULL );
		}
	}
}
//----(SA)	end

/*
=================
CG_DrawNotify
=================
*/
#define NOTIFYLOC_Y 42 // bottom end
#define NOTIFYLOC_X 0

static void CG_DrawNotify( void ) {
	int w;
	int i, len;
	vec4_t hcolor;
	int chatHeight;
	float alphapercent;
	char var[MAX_TOKEN_CHARS];
	float notifytime = 1.0f;

	if ( cg_fixedAspect.integer == 2 ) {
		CG_SetScreenPlacement(PLACE_LEFT,PLACE_TOP);
	}

	trap_Cvar_VariableStringBuffer( "con_notifytime", var, sizeof( var ) );
	notifytime = atof( var ) * 1000;

	if ( notifytime <= 100.f ) {
		notifytime = 100.0f;
	}

	chatHeight = NOTIFY_HEIGHT;

	if ( cgs.notifyLastPos != cgs.notifyPos ) {
		if ( cg.time - cgs.notifyMsgTimes[cgs.notifyLastPos % chatHeight] > notifytime ) {
			cgs.notifyLastPos++;
		}

		w = 0;

		for ( i = cgs.notifyLastPos; i < cgs.notifyPos; i++ ) {
			len = CG_DrawStrlen( cgs.notifyMsgs[i % chatHeight] );
			if ( len > w ) {
				w = len;
			}
		}

		if ( maxCharsBeforeOverlay <= 0 ) {
			maxCharsBeforeOverlay = 80;
		}

		for ( i = cgs.notifyPos - 1; i >= cgs.notifyLastPos; i-- ) {
			alphapercent = 1.0f - ( ( cg.time - cgs.notifyMsgTimes[i % chatHeight] ) / notifytime );
			if ( alphapercent > 0.5f ) {
				alphapercent = 1.0f;
			} else {
				alphapercent *= 2;
			}

			if ( alphapercent < 0.f ) {
				alphapercent = 0.f;
			}

			hcolor[0] = hcolor[1] = hcolor[2] = 1.0;
			hcolor[3] = alphapercent;
			trap_R_SetColor( hcolor );

			CG_ChatTextPaint( NOTIFYLOC_X + TINYCHAR_WIDTH,
							  NOTIFYLOC_Y - ( cgs.notifyPos - i ) * TINYCHAR_HEIGHT,
							  CG_CHAT_TEXT_SCALE, CG_CHAT_TEXT_SCALE, hcolor,
							  cgs.notifyMsgs[i % chatHeight], 0, maxCharsBeforeOverlay, 0, CG_CHAT_FONT_UNICODE );
		}
	}

	if ( cg_fixedAspect.integer ) {
		CG_SetScreenPlacement(PLACE_CENTER,PLACE_CENTER);
	}
}

/*
===================
CG_DrawHoldableItem
===================
*/
void CG_DrawHoldableItem_old( void ) {
	int value;
	gitem_t *item;

	if ( !cg.holdableSelect ) {
		return;
	}

	item    = BG_FindItemForHoldable( cg.holdableSelect );

	if ( !item ) {
		return;
	}

	if ( cg_fixedAspect.integer == 2 ) {
		CG_SetScreenPlacement(PLACE_RIGHT, PLACE_CENTER);
	}

	value   = cg.predictedPlayerState.holdable[cg.holdableSelect];

	if ( value ) {

		trap_R_SetColor( NULL );

		CG_RegisterItemVisuals( item - bg_itemlist );

		if ( cg.holdableSelect == HI_WINE ) {
			if ( value > 3 ) {
				value = 3;  // 3 stages to icon, just draw full if beyond 'full'
			}
			CG_DrawPic( 606, 366, 24, 24, cg_items[item - bg_itemlist].icons[2 - ( value - 1 )] );
		} else {
			CG_DrawPic( 606, 366, 24, 24, cg_items[item - bg_itemlist].icons[0] );

		}

		// draw the selection box so it's not just floating in space
		CG_DrawPic( 606 - 4, 366 - 4, 32, 32, cgs.media.selectShader );
	}
}
/*
		if(cg.holdableSelect == HI_WINE) {
			if(value > 3)
				value = 3;	// 3 stages to icon, just draw full if beyond 'full'

			CG_DrawPic( 598 + 16, 366, 16, 32, cg_items[item - bg_itemlist].icons[2-(value-1)] );
			CG_DrawPic( (598 + 16)-4, 366-4, 24, 40, cgs.media.selectShader );

		} else {
			CG_DrawPic( 598, 366, 32, 32, cg_items[item - bg_itemlist].icons[0] );
			CG_DrawPic( 598-4, 366-4, 40, 40, cgs.media.selectShader );
		}
*/

/*
===================
CG_DrawReward
===================
*/
static void CG_DrawReward( void ) {
	float   *color;
	int i;
	float x, y;

	if ( !cg_drawRewards.integer ) {
		return;
	}

	if ( cg_fixedAspect.integer ) {
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
	}

	color = CG_FadeColor( cg.rewardTime, REWARD_TIME );
	if ( !color ) {
		return;
	}

	trap_R_SetColor( color );
	y = 56;
	x = 320 - cg.rewardCount * ICON_SIZE / 2;
	for ( i = 0 ; i < cg.rewardCount ; i++ ) {
		CG_DrawPic( x, y, ICON_SIZE - 4, ICON_SIZE - 4, cg.rewardShader );
		x += ICON_SIZE;
	}
	trap_R_SetColor( NULL );
}


/*
===============================================================================

LAGOMETER

===============================================================================
*/

#define LAG_SAMPLES     128


typedef struct {
	int frameSamples[LAG_SAMPLES];
	int frameCount;
	int snapshotFlags[LAG_SAMPLES];
	int snapshotSamples[LAG_SAMPLES];
	int snapshotCount;
} lagometer_t;

lagometer_t lagometer;

/*
==============
CG_AddLagometerFrameInfo

Adds the current interpolate / extrapolate bar for this frame
==============
*/
void CG_AddLagometerFrameInfo( void ) {
	int offset;

	offset = cg.time - cg.latestSnapshotTime;
	lagometer.frameSamples[ lagometer.frameCount & ( LAG_SAMPLES - 1 ) ] = offset;
	lagometer.frameCount++;
}

/*
==============
CG_AddLagometerSnapshotInfo

Each time a snapshot is received, log its ping time and
the number of snapshots that were dropped before it.

Pass NULL for a dropped packet.
==============
*/
void CG_AddLagometerSnapshotInfo( snapshot_t *snap ) {
	// dropped packet
	if ( !snap ) {
		lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1 ) ] = -1;
		lagometer.snapshotCount++;
		return;
	}

	// add this snapshot's info
	lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1 ) ] = snap->ping;
	lagometer.snapshotFlags[ lagometer.snapshotCount & ( LAG_SAMPLES - 1 ) ] = snap->snapFlags;
	lagometer.snapshotCount++;
}

/*
==============
CG_DrawDisconnect

Should we draw something differnet for long lag vs no packets?
==============
*/
static void CG_DrawDisconnect( void ) {
	float x, y;
	int cmdNum;
	usercmd_t cmd;
	const char      *s;
	int w;          // FIXME char message[1024];

	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	// draw the phone jack if we are completely past our buffers
	cmdNum = trap_GetCurrentCmdNumber() - CMD_BACKUP + 1;
	trap_GetUserCmd( cmdNum, &cmd );
	if ( cmd.serverTime <= cg.snap->ps.commandTime
		 || cmd.serverTime > cg.time ) { // special check for map_restart
		return;
	}

	if ( cg_fixedAspect.integer ) {
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
	}

	// also add text in center of screen
	s = "Connection Interrupted";
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
	CG_DrawBigString( 320 - w / 2, 100, s, 1.0F );

	// blink the icon
	if ( ( cg.time >> 9 ) & 1 ) {
		return;
	}

	if ( cg_fixedAspect.integer == 2 ) {
		CG_SetScreenPlacement(PLACE_RIGHT, PLACE_BOTTOM);
	}

	x = 640 - 52;
	y = 480 - 240;

	CG_DrawPic( x, y, 48, 48, trap_R_RegisterShader( "gfx/2d/net.tga" ) );
}


#define MAX_LAGOMETER_PING  900
#define MAX_LAGOMETER_RANGE 300

/*
==============
CG_DrawLagometer
==============
*/
static void CG_DrawLagometer( void ) {
	int a, x, y, i;
	float v;
	float ax, ay, aw, ah, mid, range;
	int color;
	float vscale;

	if ( !cg_lagometer.integer || cgs.localServer ) {
//	if(0) {
		CG_DrawDisconnect();
		return;
	}

	if ( cg_fixedAspect.integer == 2 ) {
		CG_SetScreenPlacement(PLACE_RIGHT, PLACE_BOTTOM);
	}

	//
	// draw the graph
	//
	x = 640 - 52;
	y = 480 - 240;

	trap_R_SetColor( NULL );
	CG_DrawPic( x, y, 48, 48, cgs.media.lagometerShader );

	ax = x;
	ay = y;
	aw = 48;
	ah = 48;
	CG_AdjustFrom640( &ax, &ay, &aw, &ah );

	color = -1;
	range = ah / 3;
	mid = ay + range;

	vscale = range / MAX_LAGOMETER_RANGE;

	// draw the frame interpoalte / extrapolate graph
	for ( a = 0 ; a < aw ; a++ ) {
		i = ( lagometer.frameCount - 1 - a ) & ( LAG_SAMPLES - 1 );
		v = lagometer.frameSamples[i];
		v *= vscale;
		if ( v > 0 ) {
			if ( color != 1 ) {
				color = 1;
				trap_R_SetColor( g_color_table[ColorIndex( COLOR_YELLOW )] );
			}
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic( ax + aw - a, mid - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		} else if ( v < 0 ) {
			if ( color != 2 ) {
				color = 2;
				trap_R_SetColor( g_color_table[ColorIndex( COLOR_BLUE )] );
			}
			v = -v;
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic( ax + aw - a, mid, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	// draw the snapshot latency / drop graph
	range = ah / 2;
	vscale = range / MAX_LAGOMETER_PING;

	for ( a = 0 ; a < aw ; a++ ) {
		i = ( lagometer.snapshotCount - 1 - a ) & ( LAG_SAMPLES - 1 );
		v = lagometer.snapshotSamples[i];
		if ( v > 0 ) {
			if ( lagometer.snapshotFlags[i] & SNAPFLAG_RATE_DELAYED ) {
				if ( color != 5 ) {
					color = 5;  // YELLOW for rate delay
					trap_R_SetColor( g_color_table[ColorIndex( COLOR_YELLOW )] );
				}
			} else {
				if ( color != 3 ) {
					color = 3;
					trap_R_SetColor( g_color_table[ColorIndex( COLOR_GREEN )] );
				}
			}
			v = v * vscale;
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic( ax + aw - a, ay + ah - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		} else if ( v < 0 ) {
			if ( color != 4 ) {
				color = 4;      // RED for dropped snapshots
				trap_R_SetColor( g_color_table[ColorIndex( COLOR_RED )] );
			}
			trap_R_DrawStretchPic( ax + aw - a, ay + ah - range, 1, range, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	trap_R_SetColor( NULL );

	if ( cg_nopredict.integer || cg_synchronousClients.integer ) {
		CG_DrawBigString( x, y, "snc", 1.0 );
	}

	CG_DrawDisconnect();
}

void CG_DrawFreeze( void ) {
	int w, y, x;
	vec4_t color;
	char *linebuffer = va( "You are frozen" );
	float scale = CG_CHAT_TEXT_SCALE * ( (float)BIGCHAR_WIDTH / SMALLCHAR_WIDTH );

	if ( !( cg.snap->ps.eFlags & EF_FROZEN ) ) {
		return;
	}

	y = 320;

	color[0] = 1;
	color[1] = 1;
	color[2] = 1;
	color[3] = 1;

	w = CG_Text_Width_Ext( linebuffer, scale, 0, CG_CHAT_FONT );

	x = ( SCREEN_WIDTH - w ) / 2;

	CG_ChatTextPaint( x, y, scale, scale, color, linebuffer, 0, 0, ITEM_TEXTSTYLE_SHADOWED, CG_CHAT_FONT );
}

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

#define CP_LINEWIDTH 55         // NERVE - SMF

/*
==============
CG_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void CG_CenterPrint( const char *str, int y, int charWidth ) {
	char   *s;
	int i, len;                         // NERVE - SMF
	qboolean neednewline = qfalse;      // NERVE - SMF
	int priority = 0;

	// NERVE - SMF - don't draw if this print message is less important
	if ( cg.centerPrintTime && priority < cg.centerPrintPriority ) {
		return;
	}

//----(SA)	added translation lookup
#ifdef LOCALISATION
	Q_strncpyz( (char *)cg.centerPrint, CG_TranslateString( (char*)str ), sizeof( cg.centerPrint ) );
#else
	Q_strncpyz( (char *)cg.centerPrint, CG_translateString( (char*)str ), sizeof( cg.centerPrint ) );
#endif
//----(SA)	end

	cg.centerPrintPriority = priority;  // NERVE - SMF

	// NERVE - SMF - turn spaces into newlines, if we've run over the linewidth
	len = strlen( (char *)cg.centerPrint );
	for ( i = 0; i < len; i++ ) {

		// NOTE: subtract a few chars here so long words still get displayed properly
		if ( i % ( CP_LINEWIDTH - 20 ) == 0 && i > 0 ) {
			neednewline = qtrue;
		}
		if ( cg.centerPrint[i] == ' ' && neednewline ) {
			cg.centerPrint[i] = '\n';
			neednewline = qfalse;
		}
	}
	// -NERVE - SMF

	cg.centerPrintTime = cg.time;
	cg.centerPrintY = y;
	cg.centerPrintCharWidth = charWidth;

	// count the number of lines for centering
	cg.centerPrintLines = 1;
	s = cg.centerPrint;
	while ( *s ) {
		if ( *s == '\n' ) {
			cg.centerPrintLines++;
		}
		if ( !Q_strncmp( s, "\\n", 1 ) ) {
			cg.centerPrintLines++;
			s++;
		}
		s++;
	}
}

// NERVE - SMF
/*
==============
CG_PriorityCenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void CG_PriorityCenterPrint( const char *str, int y, int charWidth, int priority ) {
	char    *s;
	int i, len;                         // NERVE - SMF
	qboolean neednewline = qfalse;      // NERVE - SMF

	// NERVE - SMF - don't draw if this print message is less important
	if ( cg.centerPrintTime && priority < cg.centerPrintPriority ) {
		return;
	}

//----(SA)	added translation lookup
#ifdef LOCALISATION
	Q_strncpyz( (char *)cg.centerPrint, CG_TranslateString( (char*)str ), sizeof( cg.centerPrint ) );
#else
	Q_strncpyz( (char *)cg.centerPrint, CG_translateString( (char*)str ), sizeof( cg.centerPrint ) );
#endif
//----(SA)	end
	cg.centerPrintPriority = priority;  // NERVE - SMF

	// NERVE - SMF - turn spaces into newlines, if we've run over the linewidth
	len = strlen( cg.centerPrint );
	for ( i = 0; i < len; i++ ) {

		// NOTE: subtract a few chars here so long words still get displayed properly
		if ( i % ( CP_LINEWIDTH - 20 ) == 0 && i > 0 ) {
			neednewline = qtrue;
		}
		if ( cg.centerPrint[i] == ' ' && neednewline ) {
			cg.centerPrint[i] = '\n';
			neednewline = qfalse;
		}
	}
	// -NERVE - SMF

	cg.centerPrintTime = cg.time + 2000;
	cg.centerPrintY = y;
	cg.centerPrintCharWidth = charWidth;

	// count the number of lines for centering
	cg.centerPrintLines = 1;
	s = cg.centerPrint;
	while ( *s ) {
		if ( *s == '\n' ) {
			cg.centerPrintLines++;
		}
		s++;
	}
}
// -NERVE - SMF

/*
==============
CG_BuyPrint

Called for buy-tooltip messages that should stay in the center of the screen
for a few moments. Translates each alphanumeric word individually so dynamic
strings (weapon name + price) can be built server-side and still localize.
==============
*/
void CG_BuyPrint( const char *str, int y, int charWidth ) {
	const char *s, *p;
	char *c = (char *)cg.buyPrint;
	char   token[64];
	const char *trToken;
	int    lenTrToken;
	int    destSizeBuyPrint = sizeof( cg.buyPrint );

	s = p = str;
	while ( 1 ) {
		if ( Q_isalphanumeric( *s ) ) {
			if ( !Q_isforfilename( *p ) || *p == '\r' || *p == '\n' || *p == '\0' ) {
				// copy translation key
				Q_strncpyz( token, s, p - s + 1 );

				// get translation
				trToken = CG_translateTextString( token );

				lenTrToken = strlen( trToken );

				// append translation
				Q_strncpyz( c, trToken, destSizeBuyPrint );
				destSizeBuyPrint -= lenTrToken;
				c += lenTrToken;

				// copy separator
				Q_strncpyz( token, p, 2 );

				// append separator
				Q_strncpyz( c, token, destSizeBuyPrint );
				destSizeBuyPrint -= 1;
				c += 1;

				s = p + 1;
			}
		} else {
			s = p;
		}

		if ( !( *p++ ) ) {
			break;
		}
	}

	cg.buyPrintTime = cg.time;
	cg.buyPrintY = y;
	cg.buyPrintCharWidth = charWidth;

	// count the number of lines for centering
	cg.buyPrintLines = 1;
	s = cg.buyPrint;
	while ( *s ) {
		if ( *s == '\n' ) {
			cg.buyPrintLines++;
		}
		if ( !Q_strncmp( s, "\\n", 1 ) ) {
			cg.buyPrintLines++;
			s++;
		}
		s++;
	}
}

/*
==============
CG_SubtitlePrint

Called for important messages that should stay in the center of the screen
==============
*/
void CG_SubtitlePrint( const char *str, int y, int charWidth ) {
	char   *s;
	int len;

	const char *translated = CG_translateTextString( str );

	// Check if the translated string is "IGNORED_SUBTITLE", indicating an ignored subtitle
	if ( strcmp( translated, "IGNORED_SUBTITLE" ) == 0 ) {
		// If it is, return immediately to prevent the subtitle from being displayed
		return;
	}

	// Copy the translated string to cg.subtitlePrint
	Q_strncpyz( cg.subtitlePrint, translated, sizeof( cg.subtitlePrint ) );

	cg.subtitlePrintY = y;
	cg.subtitlePrintCharWidth = charWidth;

	// count the number of lines for centering
	cg.subtitlePrintLines = 1;
	s = cg.subtitlePrint;
	while ( *s ) {
		if ( *s == '\n' ) {
			cg.subtitlePrintLines++;
		}
		if ( !Q_strncmp( s, "\\n", 1 ) ) {
			cg.subtitlePrintLines++;
			s++;
		}
		s++;
	}

	// Calculate the number of characters in the message
	len = CG_DrawStrlen( cg.subtitlePrint );
	// Calculate the display time based on an average reading speed of 17 characters per second
	int displayTime = (int)( ( len / 17.0f ) * 1000 ); // Convert to milliseconds

	// Set the time at which the message should disappear
	cg.subtitlePrintTime = cg.time + displayTime;
}

/*
===================
CG_DrawCenterString
===================
*/
static void CG_DrawCenterString( void ) {
	char    *start;
	int l;
	int x, y, w;
	float   *color;
	float subScale;

	if ( !cg.centerPrintTime ) {
		return;
	}

	color = CG_FadeColor( cg.centerPrintTime, 1000 * cg_centertime.value );
	if ( !color ) {
		cg.centerPrintTime = 0;
		cg.centerPrintPriority = 0;
		return;
	}

	if ( cg_fixedAspect.integer ) {
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
	}

	trap_R_SetColor( color );

	start = (char *)cg.centerPrint;

	// scale the TTF font relative to the requested charWidth so it still controls centerprint size
	subScale = CG_CHAT_TEXT_SCALE * ( (float)cg.centerPrintCharWidth / SMALLCHAR_WIDTH );

	y = cg.centerPrintY - cg.centerPrintLines * BIGCHAR_HEIGHT / 2;

	while ( 1 ) {
		char linebuffer[1024];

		for ( l = 0; l < 40; l++ ) {
			if ( !start[l] || start[l] == '\n' ) {
				break;
			}
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;

		w = CG_Text_Width_Ext( linebuffer, subScale, 0, &cgDC.Assets.textFont );

		x = ( SCREEN_WIDTH - w ) / 2;

		CG_ChatTextPaint( x, y, subScale, subScale, color, linebuffer, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgDC.Assets.textFont );

		y += cg.centerPrintCharWidth * 2;

		while ( *start && ( *start != '\n' ) ) {
			start++;
		}
		if ( !*start ) {
			break;
		}
		start++;
	}

	trap_R_SetColor( NULL );
}

/*
===================
CG_DrawBuyString
===================
*/
static void CG_DrawBuyString( void ) {
	char    *start;
	int l;
	int x, y, w;
	float   *color;
	float subScale;

	if ( !cg.buyPrintTime ) {
		return;
	}

	color = CG_FadeColor( cg.buyPrintTime, 1000 * cg_buyprinttime.value );
	if ( !color ) {
		cg.buyPrintTime = 0;
		return;
	}

	if ( cg_fixedAspect.integer ) {
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
	}

	trap_R_SetColor( color );

	start = (char *)cg.buyPrint;

	// scale the TTF font relative to the requested charWidth so it still controls buy-prompt size
	subScale = CG_CHAT_TEXT_SCALE * ( (float)cg.buyPrintCharWidth / SMALLCHAR_WIDTH );

	y = cg.buyPrintY - cg.buyPrintLines * BIGCHAR_HEIGHT / 2;

	while ( 1 ) {
		char linebuffer[1024];

		for ( l = 0; l < 40; l++ ) {
			if ( !start[l] || start[l] == '\n' || !Q_strncmp( &start[l], "\\n", 1 ) ) {
				break;
			}
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;

		w = CG_Text_Width_Ext( linebuffer, subScale, 0, &cgDC.Assets.textFont );

		x = ( SCREEN_WIDTH - w ) / 2;

		CG_ChatTextPaint( x, y, subScale, subScale, color, linebuffer, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgDC.Assets.textFont );

		y += cg.buyPrintCharWidth * 2;

		while ( *start && ( *start != '\n' ) ) {
			if ( !Q_strncmp( start, "\\n", 1 ) ) {
				start++;
				break;
			}
			start++;
		}
		if ( !*start ) {
			break;
		}
		start++;
	}

	trap_R_SetColor( NULL );
}

#define PERK_ICON_SIZE 36
#define PERK_ICON_GAP  10

/*
===================
CG_DrawPerkChooser

Small on-screen list for the perk_choice buy machine (see
Survival_HandlePerkChoiceOpen/Confirm in g_survival_buy.c). Purely a display
of server-authoritative state (cg.perkChooser*) - the highlight itself is
moved by CG_NextWeapon_f/CG_PrevWeapon_f (mouse wheel) while active.
===================
*/
static void CG_DrawPerkChooser( void ) {
	int perk;
	int count = NUM_PERKS - 1;
	int totalWidth = count * PERK_ICON_SIZE + ( count - 1 ) * PERK_ICON_GAP;
	int x, y, boxX, boxY, boxW, boxH;
	int score;
	gitem_t *selItem;

	if ( !cg.perkChooserActive ) {
		return;
	}

	if ( cg_fixedAspect.integer ) {
		CG_SetScreenPlacement( PLACE_CENTER, PLACE_CENTER );
	}

	score = cg.snap ? cg.snap->ps.persistant[PERS_SCORE] : 0;

	y = SCREEN_HEIGHT - 150;
	x = ( SCREEN_WIDTH - totalWidth ) / 2;

	boxX = x - 12;
	boxY = y - 34;
	boxW = totalWidth + 24;
	boxH = PERK_ICON_SIZE + 34 + 40; // icons + name row above + price row & hint line below

	{
		vec4_t bgColor = { 0, 0, 0, 0.55f };
		CG_FillRect( boxX, boxY, boxW, boxH, bgColor );
	}

	for ( perk = PERK_RESILIENCE; perk < NUM_PERKS; perk++ ) {
		gitem_t *item = BG_FindItemForPerk( (perk_t)perk );
		int itemIndex;
		int owned, price;
		qboolean selected = ( perk == cg.perkChooserSelected ) ? qtrue : qfalse;
		char priceStr[32];
		vec4_t priceColor; // local copy - CG_Text_Paint mutates color[3] in place, never alias the shared color* globals
		vec4_t iconColor = { 1, 1, 1, 1 };
		qhandle_t iconShader;

		if ( !item ) {
			continue;
		}

		itemIndex = item - bg_itemlist;
		CG_RegisterItemVisuals( itemIndex );

		owned = cg.perkChooserOwned[perk];
		price = cg.perkChooserPrice[perk];

		// already own the base level (either eligible for the PRO upgrade, or already PRO) -
		// show the PRO icon, matching CG_DrawPerks' owned-perks status row
		iconShader = cg_items[itemIndex].icons[0];
		if ( owned >= 1 && cgs.media.perkProIcons[perk] ) {
			iconShader = cgs.media.perkProIcons[perk];
		}

		if ( owned >= 2 ) {
			Q_strncpyz( priceStr, "MAXED", sizeof( priceStr ) );
			Vector4Copy( colorMdGrey, priceColor );
			iconColor[3] = 0.4f;
		} else if ( score < price ) {
			Com_sprintf( priceStr, sizeof( priceStr ), "%d", price );
			Vector4Copy( colorMdGrey, priceColor );
			iconColor[3] = 0.55f;
		} else {
			Com_sprintf( priceStr, sizeof( priceStr ), "%d", price );
			Vector4Copy( selected ? colorYellow : colorWhite, priceColor );
		}

		if ( selected ) {
			vec4_t hi = { 1, 1, 0, 0.25f };
			CG_FillRect( x - 3, y - 3, PERK_ICON_SIZE + 6, PERK_ICON_SIZE + 6, hi );
		}

		trap_R_SetColor( iconColor );
		CG_DrawPic( x, y, PERK_ICON_SIZE, PERK_ICON_SIZE, iconShader );
		trap_R_SetColor( NULL );

		{
			int w = CG_Text_Width( priceStr, UI_FONT_DEFAULT, 0.22f, 0 );
			CG_Text_Paint( x + ( PERK_ICON_SIZE - w ) / 2, y + PERK_ICON_SIZE + 14, UI_FONT_DEFAULT, 0.22f, priceColor, priceStr, 0, 0, ITEM_TEXTSTYLE_SHADOWED );
		}

		x += PERK_ICON_SIZE + PERK_ICON_GAP;
	}

	selItem = BG_FindItemForPerk( (perk_t)cg.perkChooserSelected );
	if ( selItem ) {
		int selIndex = selItem - bg_itemlist;
		const char *selName = cgs.itemPrintNames[selIndex][0] ? cgs.itemPrintNames[selIndex] : selItem->pickup_name;

		if ( selName && selName[0] ) {
			vec4_t nameColor;
			int w = CG_Text_Width( selName, UI_FONT_DEFAULT, 0.28f, 0 );
			Vector4Copy( colorYellow, nameColor );
			CG_Text_Paint( boxX + ( boxW - w ) / 2, boxY + 20, UI_FONT_DEFAULT, 0.28f, nameColor, selName, 0, 0, ITEM_TEXTSTYLE_SHADOWED );
		}
	}

	{
		const char *hint = "Scroll to browse - USE to confirm";
		vec4_t hintColor;
		int w = CG_Text_Width( hint, UI_FONT_DEFAULT, 0.18f, 0 );
		Vector4Copy( colorMdGrey, hintColor );
		CG_Text_Paint( boxX + ( boxW - w ) / 2, boxY + boxH - 8, UI_FONT_DEFAULT, 0.18f, hintColor, hint, 0, 0, ITEM_TEXTSTYLE_SHADOWED );
	}
}

/*
===================
CG_DrawSubtitleString
===================
*/
static void CG_DrawSubtitleString( void ) {
	char    *start;
	int l;
	int x, y, w;
	float   *color;
	float subScale;
	int style;

	if ( !cg.subtitlePrintTime ) {
		return;
	}

	color = CG_FadeColor( cg.subtitlePrintTime, 1000 * cg_centertime.value );
	if ( !color ) {
		cg.subtitlePrintTime = 0;
		return;
	}

	if ( cg_fixedAspect.integer ) {
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
	}

	trap_R_SetColor( color );

	start = cg.subtitlePrint;

	// scale the TTF font relative to cg_subtitleSize so the cvar still controls subtitle size
	subScale = CG_CHAT_TEXT_SCALE * ( (float)cg.subtitlePrintCharWidth / SMALLCHAR_WIDTH );
	style = cg_subtitleShadow.integer ? ITEM_TEXTSTYLE_SHADOWED : ITEM_TEXTSTYLE_NORMAL;

	y = cg.subtitlePrintY - cg.subtitlePrintLines * BIGCHAR_HEIGHT / 2;

	while ( 1 ) {
		char linebuffer[1024];

		for ( l = 0; l < 50; l++ ) { // Line length limit
			if ( !start[l] || start[l] == '\n' || !Q_strncmp( &start[l], "\\n", 1 ) ) {
				break;
			}
			linebuffer[l] = start[l];
			if ( l >= 49 && start[l+1] != ' ' && start[l+1] != '\0' ) { // Check if the next character is a space or end of string
				while ( l > 0 && linebuffer[l] != ' ' ) { // Move back to the last space
					l--;
				}
				break;
			}
		}
		linebuffer[l] = 0;

		w = CG_Text_Width_Ext( linebuffer, subScale, 0, CG_CHAT_FONT );

		x = ( SCREEN_WIDTH - w ) / 2;

		CG_ChatTextPaint( x, y, subScale, subScale, color, linebuffer, 0, 0, style, CG_CHAT_FONT );

		y += cg.subtitlePrintCharWidth * 2;

		// Skip processed characters and newline characters
		start += l;
		while ( *start && ( *start == '\n' || !Q_strncmp( start, "\\n", 1 ) ) ) {
			start++;
		}
		if ( !*start ) {
			break;
		}
	}

	trap_R_SetColor( NULL );
}

/*
================================================================================

CROSSHAIRS

================================================================================
*/


/*
==============
CG_DrawWeapReticle
==============
*/
static void CG_DrawWeapReticle( void ) {
	int weap;
	vec4_t color = {0, 0, 0, 1};
	vec4_t snoopercolor = {0.7, .8, 0.7, 0};    // greenish
	float snooperBrightness;
	float x = 80, y, w = 240, h = 240;
	float mask = 0, lb = 0;

	CG_AdjustFrom640( &x, &y, &w, &h );

	weap = cg.weaponSelect;

	if ( weap == WP_SNIPERRIFLE || weap == WP_M1941SCOPE || weap == WP_DELISLESCOPE ) {
		// sides
		if ( cg_fixedAspect.integer ) {
			if ( cgs.glconfig.vidWidth * 480.0 > cgs.glconfig.vidHeight * 640.0 ) {
				mask = 0.5 * ( ( cgs.glconfig.vidWidth - ( cgs.screenXScale * 480.0 ) ) / cgs.screenXScale );

				CG_SetScreenPlacement(PLACE_LEFT, PLACE_CENTER);
				CG_FillRect( 0, 0, mask, 480, color );
				CG_SetScreenPlacement(PLACE_RIGHT, PLACE_CENTER);
				CG_FillRect( 640 - mask, 0, mask, 480, color );
			} else if ( cgs.glconfig.vidWidth * 480.0 < cgs.glconfig.vidHeight * 640.0 ) {
				// sides with letterbox
				lb = 0.5 * ( ( cgs.glconfig.vidHeight - ( cgs.screenYScale * 480.0 ) ) / cgs.screenYScale );

				CG_SetScreenPlacement(PLACE_LEFT, PLACE_CENTER);
				CG_FillRect( 0, 0, 80, 480, color );
				CG_SetScreenPlacement(PLACE_RIGHT, PLACE_CENTER);
				CG_FillRect( 560, 0, 80, 480, color );

				CG_SetScreenPlacement(PLACE_LEFT, PLACE_BOTTOM);
				CG_FillRect( 0, 480 - lb, 640, lb, color );
				CG_SetScreenPlacement(PLACE_LEFT, PLACE_TOP);
				CG_FillRect( 0, 0, 640, lb, color );
			} else {
				// resolution is 4:3
				CG_SetScreenPlacement(PLACE_LEFT, PLACE_CENTER);
				CG_FillRect( 0, 0, 80, 480, color );
				CG_SetScreenPlacement(PLACE_RIGHT, PLACE_CENTER);
				CG_FillRect( 560, 0, 80, 480, color );
			}
		} else {
			CG_FillRect( 0, 0, 80, 480, color );
			CG_FillRect( 560, 0, 80, 480, color );
		}

		// center
		if ( cg_fixedAspect.integer ) {
			CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
		}

		if ( cgs.media.reticleShaderSimpleQ ) {
			if ( cg_fixedAspect.integer ) {
				trap_R_DrawStretchPic( x, lb * cgs.screenYScale, w, h, 0, 0, 1, 1, cgs.media.reticleShaderSimpleQ );         // tl
				trap_R_DrawStretchPic( x + w, lb * cgs.screenYScale, w, h, 1, 0, 0, 1, cgs.media.reticleShaderSimpleQ );     // tr
				trap_R_DrawStretchPic( x, h + lb * cgs.screenYScale, w, h, 0, 1, 1, 0, cgs.media.reticleShaderSimpleQ );     // bl
				trap_R_DrawStretchPic( x + w, h + lb * cgs.screenYScale, w, h, 1, 1, 0, 0, cgs.media.reticleShaderSimpleQ ); // br
			} else {
				trap_R_DrawStretchPic( x, 0, w, h, 0, 0, 1, 1, cgs.media.reticleShaderSimpleQ );      // tl
				trap_R_DrawStretchPic( x + w, 0, w, h, 1, 0, 0, 1, cgs.media.reticleShaderSimpleQ );  // tr
				trap_R_DrawStretchPic( x, h, w, h, 0, 1, 1, 0, cgs.media.reticleShaderSimpleQ );      // bl
				trap_R_DrawStretchPic( x + w, h, w, h, 1, 1, 0, 0, cgs.media.reticleShaderSimpleQ );  // br
			}
		}

		if ( cg_drawCrosshairReticle.integer ) {
			CG_FillRect( 80, 239, 480, 1, color );	// horiz
			CG_FillRect( 319, 0, 1, 480, color );   // vert
		}

		// hairs
		CG_FillRect( 84, 239, 177, 2, color );   // left
		CG_FillRect( 320, 242, 1, 58, color );   // center top
		CG_FillRect( 319, 300, 2, 178, color );  // center bot
		CG_FillRect( 380, 239, 177, 2, color );  // right

	} else if ( weap == WP_SNOOPERSCOPE ) {
		// sides
		if ( cg_fixedAspect.integer ) {
			if ( cgs.glconfig.vidWidth * 480.0 > cgs.glconfig.vidHeight * 640.0 ) {
				mask = 0.5 * ( ( cgs.glconfig.vidWidth - ( cgs.screenXScale * 480.0 ) ) / cgs.screenXScale );

				CG_SetScreenPlacement(PLACE_LEFT, PLACE_CENTER);
				CG_FillRect( 0, 0, mask, 480, color );
				CG_SetScreenPlacement(PLACE_RIGHT, PLACE_CENTER);
				CG_FillRect( 640 - mask, 0, mask, 480, color );
			} else if ( cgs.glconfig.vidWidth * 480.0 < cgs.glconfig.vidHeight * 640.0 ) {
				// sides with letterbox
				lb = 0.5 * ( ( cgs.glconfig.vidHeight - ( cgs.screenYScale * 480.0 ) ) / cgs.screenYScale );

				CG_SetScreenPlacement(PLACE_LEFT, PLACE_CENTER);
				CG_FillRect( 0, 0, 80, 480, color );
				CG_SetScreenPlacement(PLACE_RIGHT, PLACE_CENTER);
				CG_FillRect( 560, 0, 80, 480, color );

				CG_SetScreenPlacement(PLACE_LEFT, PLACE_BOTTOM);
				CG_FillRect( 0, 480 - lb, 640, lb, color );
				CG_SetScreenPlacement(PLACE_LEFT, PLACE_TOP);
				CG_FillRect( 0, 0, 640, lb, color );
			} else {
				// resolution is 4:3
				CG_SetScreenPlacement(PLACE_LEFT, PLACE_CENTER);
				CG_FillRect( 0, 0, 80, 480, color );
				CG_SetScreenPlacement(PLACE_RIGHT, PLACE_CENTER);
				CG_FillRect( 560, 0, 80, 480, color );
			}
		} else {
			CG_FillRect( 0, 0, 80, 480, color );
			CG_FillRect( 560, 0, 80, 480, color );
		}

		// center
		if ( cg_fixedAspect.integer ) {
			CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
		}

//----(SA)	added
		// DM didn't like how bright it gets
		snooperBrightness = Com_Clamp( 0.0f, 1.0f, cg_reticleBrightness.value );
		snoopercolor[0] *= snooperBrightness;
		snoopercolor[1] *= snooperBrightness;
		snoopercolor[2] *= snooperBrightness;
		trap_R_SetColor( snoopercolor );
//----(SA)	end

		if ( cgs.media.snooperShaderSimple ) {
			CG_DrawPic( 80, 0, 480, 480, cgs.media.snooperShaderSimple );
		}

		// hairs

		CG_FillRect( 310, 120, 20, 1, color );   //					-----
		CG_FillRect( 300, 160, 40, 1, color );   //				-------------
		CG_FillRect( 310, 200, 20, 1, color );   //					-----

		CG_FillRect( 140, 239, 360, 1, color );  // horiz ---------------------------

		CG_FillRect( 310, 280, 20, 1, color );   //					-----
		CG_FillRect( 300, 320, 40, 1, color );   //				-------------
		CG_FillRect( 310, 360, 20, 1, color );   //					-----



		CG_FillRect( 400, 220, 1, 40, color );   // l

		CG_FillRect( 319, 60, 1, 360, color );   // vert

		CG_FillRect( 240, 220, 1, 40, color );   // r
	} else if ( weap == WP_FG42SCOPE ) {
		// sides
		if ( cg_fixedAspect.integer ) {
			if ( cgs.glconfig.vidWidth * 480.0 > cgs.glconfig.vidHeight * 640.0 ) {
				mask = 0.5 * ( ( cgs.glconfig.vidWidth - ( cgs.screenXScale * 480.0 ) ) / cgs.screenXScale );

				CG_SetScreenPlacement(PLACE_LEFT, PLACE_CENTER);
				CG_FillRect( 0, 0, mask, 480, color );
				CG_SetScreenPlacement(PLACE_RIGHT, PLACE_CENTER);
				CG_FillRect( 640 - mask, 0, mask, 480, color );
			} else if ( cgs.glconfig.vidWidth * 480.0 < cgs.glconfig.vidHeight * 640.0 ) {
				// sides with letterbox
				lb = 0.5 * ( ( cgs.glconfig.vidHeight - ( cgs.screenYScale * 480.0 ) ) / cgs.screenYScale );

				CG_SetScreenPlacement(PLACE_LEFT, PLACE_CENTER);
				CG_FillRect( 0, 0, 80, 480, color );
				CG_SetScreenPlacement(PLACE_RIGHT, PLACE_CENTER);
				CG_FillRect( 560, 0, 80, 480, color );

				CG_SetScreenPlacement(PLACE_LEFT, PLACE_BOTTOM);
				CG_FillRect( 0, 480 - lb, 640, lb, color );
				CG_SetScreenPlacement(PLACE_LEFT, PLACE_TOP);
				CG_FillRect( 0, 0, 640, lb, color );
			} else {
				// resolution is 4:3
				CG_SetScreenPlacement(PLACE_LEFT, PLACE_CENTER);
				CG_FillRect( 0, 0, 80, 480, color );
				CG_SetScreenPlacement(PLACE_RIGHT, PLACE_CENTER);
				CG_FillRect( 560, 0, 80, 480, color );
			}
		} else {
			CG_FillRect( 0, 0, 80, 480, color );
			CG_FillRect( 560, 0, 80, 480, color );
		}

		// center
		if ( cg_fixedAspect.integer ) {
			CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
		}

		if ( cgs.media.reticleShaderSimpleQ ) {
			if ( cg_fixedAspect.integer ) {
				trap_R_DrawStretchPic( x, lb * cgs.screenYScale, w, h, 0, 0, 1, 1, cgs.media.reticleShaderSimpleQ );         // tl
				trap_R_DrawStretchPic( x + w, lb * cgs.screenYScale, w, h, 1, 0, 0, 1, cgs.media.reticleShaderSimpleQ );     // tr
				trap_R_DrawStretchPic( x, h + lb * cgs.screenYScale, w, h, 0, 1, 1, 0, cgs.media.reticleShaderSimpleQ );     // bl
				trap_R_DrawStretchPic( x + w, h + lb * cgs.screenYScale, w, h, 1, 1, 0, 0, cgs.media.reticleShaderSimpleQ ); // br
			} else {
				trap_R_DrawStretchPic( x, 0, w, h, 0, 0, 1, 1, cgs.media.reticleShaderSimpleQ );     // tl
				trap_R_DrawStretchPic( x + w, 0, w, h, 1, 0, 0, 1, cgs.media.reticleShaderSimpleQ ); // tr
				trap_R_DrawStretchPic( x, h, w, h, 0, 1, 1, 0, cgs.media.reticleShaderSimpleQ );     // bl
				trap_R_DrawStretchPic( x + w, h, w, h, 1, 1, 0, 0, cgs.media.reticleShaderSimpleQ ); // br
			}
		}

		// hairs
		CG_FillRect( 84, 239, 150, 3, color );   // left
		CG_FillRect( 234, 240, 173, 1, color );  // horiz center
		CG_FillRect( 407, 239, 150, 3, color );  // right


		CG_FillRect( 319, 2,   3, 151, color );  // top center top
		CG_FillRect( 320, 153, 1, 114, color );  // top center bot

		CG_FillRect( 320, 241, 1, 87, color );   // bot center top
		CG_FillRect( 319, 327, 3, 151, color );  // bot center bot
	}
}

/*
==============
CG_DrawBinocReticle
==============
*/
static void CG_DrawBinocReticle( void ) {
	// an alternative.  This gives nice sharp lines at the expense of a few extra polys
	vec4_t color = {0, 0, 0, 1};
	float x, y, w = 320, h = 240;
	float mask = 0, lb = 0;

	if ( cg_fixedAspect.integer ) {
		if ( cgs.glconfig.vidWidth * 480.0 > cgs.glconfig.vidHeight * 640.0 ) {
			mask = 0.5 * ( ( cgs.glconfig.vidWidth - ( cgs.screenXScale * 640.0 ) ) / cgs.screenXScale );

			CG_SetScreenPlacement(PLACE_LEFT, PLACE_CENTER);
			CG_FillRect( 0, 0, mask, 480, color );
			CG_SetScreenPlacement(PLACE_RIGHT, PLACE_CENTER);
			CG_FillRect( 640 - mask, 0, mask, 480, color );
		} else if ( cgs.glconfig.vidWidth * 480.0 < cgs.glconfig.vidHeight * 640.0 ) {
			lb = 0.5 * ( ( cgs.glconfig.vidHeight - ( cgs.screenYScale * 480.0 ) ) / cgs.screenYScale );

			CG_SetScreenPlacement(PLACE_LEFT, PLACE_BOTTOM);
			CG_FillRect( 0, 480 - lb, 640, lb, color );
			CG_SetScreenPlacement(PLACE_LEFT, PLACE_TOP);
			CG_FillRect( 0, 0, 640, lb, color );
		}
	}

	if ( cg_fixedAspect.integer ) {
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
	}

	if ( cgs.media.binocShaderSimpleQ ) {
		CG_AdjustFrom640( &x, &y, &w, &h );
		if ( cg_fixedAspect.integer ) {
			trap_R_DrawStretchPic( mask * cgs.screenXScale, lb * cgs.screenYScale, w, h, 0, 0, 1, 1, cgs.media.binocShaderSimpleQ );         // tl
			trap_R_DrawStretchPic( w + mask * cgs.screenXScale, lb * cgs.screenYScale, w, h, 1, 0, 0, 1, cgs.media.binocShaderSimpleQ );     // tr
			trap_R_DrawStretchPic( mask * cgs.screenXScale, h + lb * cgs.screenYScale, w, h, 0, 1, 1, 0, cgs.media.binocShaderSimpleQ );     // bl
			trap_R_DrawStretchPic( w + mask * cgs.screenXScale, h + lb * cgs.screenYScale, w, h, 1, 1, 0, 0, cgs.media.binocShaderSimpleQ ); // br
		} else {
			trap_R_DrawStretchPic( 0, 0, w, h, 0, 0, 1, 1, cgs.media.binocShaderSimpleQ );  // tl
			trap_R_DrawStretchPic( w, 0, w, h, 1, 0, 0, 1, cgs.media.binocShaderSimpleQ );  // tr
			trap_R_DrawStretchPic( 0, h, w, h, 0, 1, 1, 0, cgs.media.binocShaderSimpleQ );  // bl
			trap_R_DrawStretchPic( w, h, w, h, 1, 1, 0, 0, cgs.media.binocShaderSimpleQ );  // br
		}
	}

	if ( cg_drawCrosshairBinoc.integer ) {
		CG_FillRect( 0, 239, 640, 1, color );	// horiz
		CG_FillRect( 320, 0, 1, 480, color );   // vert
	}


	CG_FillRect( 146, 239, 348, 1, color );

	CG_FillRect( 188, 234, 1, 13, color );   // ll
	CG_FillRect( 234, 226, 1, 29, color );   // l
	CG_FillRect( 274, 234, 1, 13, color );   // lr
	CG_FillRect( 320, 213, 1, 55, color );   // center
	CG_FillRect( 360, 234, 1, 13, color );   // rl
	CG_FillRect( 406, 226, 1, 29, color );   // r
	CG_FillRect( 452, 234, 1, 13, color );   // rr
}

void CG_FinishWeaponChange( int lastweap, int newweap ); // JPW NERVE


/*
=================
CG_DrawCrosshair
=================
*/
static void CG_DrawCrosshair( void ) {
	float w, h;
	qhandle_t hShader;
	float f;
	float x, y;
	int weapnum;                // DHM - Nerve
	vec4_t hcolor = {1, 1, 1, 1};
	qboolean friendInSights = qfalse;

	if ( cg.renderingThirdPerson ) {
		return;
	}

	hcolor[3] = cg_crosshairAlpha.value;    //----(SA)	added


	// on mg42
	if ( cg.snap->ps.eFlags & EF_MG42_ACTIVE ) {
		hcolor[0] = hcolor[1] = hcolor[2] = 0.0f;
		hcolor[3] = 0.6f;
		// option 1
//		CG_FillRect (300, 240, 40, 2, hcolor);	// horizontal
//		CG_FillRect (319, 242, 2, 16, hcolor);	// vertical

		// option 2
		CG_FillRect( 305, 240, 30, 2, hcolor );  // horizontal
		CG_FillRect( 314, 256, 12, 2, hcolor );  // horizontal2
		CG_FillRect( 319, 242, 2, 32, hcolor );  // vertical

		return;
	}

	friendInSights = (qboolean)( cg.snap->ps.serverCursorHint == HINT_PLYR_FRIEND );  //----(SA)	added

	// DHM - Nerve :: show reticle in limbo and spectator
	if (  cg.demoPlayback ) {
		weapnum = cg.snap->ps.weapon;
	} else {
		weapnum = cg.weaponSelect;
	}

	switch ( weapnum ) {

		// weapons that get no reticle
	case WP_NONE:       // no weapon, no crosshair
	case WP_SNOOPER:
		if (cg.zoomedBinoc)
		{
			CG_DrawBinocReticle();
		}
		if (!cg_snipersCrosshair.integer)
		{
			return;
		}
		break;
		// special reticle for weapon
	case WP_KNIFE:
		if ( cg.zoomedBinoc ) {
			CG_DrawBinocReticle();
			return;
			// Crosshair shouldn't be shown at all when holding a knife.
		}
		else
			return;
		break;
	case WP_SNIPERRIFLE:
	case WP_SNOOPERSCOPE:
	case WP_FG42SCOPE:
	case WP_M1941SCOPE:
	case WP_DELISLESCOPE:
		if ( !( cg.snap->ps.eFlags & EF_MG42_ACTIVE ) ) {
			CG_DrawWeapReticle();
			return;
		}
		break;
	default:
		break;
	}

	// using binoculars
	if ( cg.zoomedBinoc ) {
		CG_DrawBinocReticle();
		return;
	}

	if ( cg.weaponSelect == WP_MAUSER ) {
		if ( COM_BitCheck( cg.predictedPlayerState.weapons, WP_SNIPERRIFLE ) ) {
		if ( !cg_snipersCrosshair.integer )
	    {
		return;
	    }
		}
	}

	if ( cg.weaponSelect == WP_M1941 ) {
		if ( COM_BitCheck( cg.predictedPlayerState.weapons, WP_M1941SCOPE ) ) {
		if ( !cg_snipersCrosshair.integer )
	    {
		return;
	    }
		}
	}

	if ( cg.weaponSelect == WP_DELISLE ) {
		if ( COM_BitCheck( cg.predictedPlayerState.weapons, WP_DELISLESCOPE ) ) {
		if ( !cg_snipersCrosshair.integer )
	    {
		return;
	    }
		}
	}

	if ( !cg_drawCrosshair.integer ) {	//----(SA)	moved down so it doesn't keep the scoped weaps from drawing reticles
		return;
	}

	// no crosshair while leaning
	if ( cg.snap->ps.leanf ) {
		return;
	}

	if ( !cg_drawCrosshair.integer ) {	
		    return;
     }

	// no crosshair when looking at exits
	if ( cg.snap->ps.serverCursorHint >= HINT_EXIT && cg.snap->ps.serverCursorHint <= HINT_NOEXIT_FAR ) {
		return;
	}

	if ( cg_paused.integer ) {
		// no draw if any menu's are up	 (or otherwise paused)
		return;
	}

	// set color based on health
	if ( cg_crosshairHealth.integer ) {
		CG_ColorForHealth( hcolor );
		trap_R_SetColor( hcolor );
	} else {
		trap_R_SetColor( NULL );
	}

	w = h = cg_crosshairSize.value;

	// RF, crosshair size represents aim spread
	if ( !cg_solidCrosshair.integer ) {
		f = (float)cg.snap->ps.aimSpreadScale / 255.0;
		w *= ( 1 + f * 2.0 );
		h *= ( 1 + f * 2.0 );
	}

	// pulse (grow + redden) while cooking a grenade - quick-grenade or an equipped grenade weapon
	if ( cg.predictedPlayerState.grenadeTimeLeft > 0 ) {
		float pulse = 1.0f - ( (float)( cg.predictedPlayerState.grenadeTimeLeft % 1000 ) / 1000.0f );

		w *= 1.0f + 0.5f * pulse;
		h *= 1.0f + 0.5f * pulse;

		hcolor[0] = 1.0f;
		hcolor[1] = hcolor[2] = 1.0f - pulse;
		trap_R_SetColor( hcolor );
	}

	x = cg_crosshairX.integer;
	y = cg_crosshairY.integer;
	if ( !cg_fixedAspect.integer ) {
		CG_AdjustFrom640( &x, &y, &w, &h );
	}

//----(SA)	modified
	if ( friendInSights ) {
		hShader = cgs.media.crosshairFriendly;
	} else {
		hShader = cgs.media.crosshairShader[ cg_drawCrosshair.integer % NUM_CROSSHAIRS ];
	}
//----(SA)	end

	// NERVE - SMF - modified, fixes crosshair offset in shifted/scaled 3d views
	// (SA) also breaks scaled view...
	if ( cg_fixedAspect.integer ) {
		CG_DrawPic( ((SCREEN_WIDTH-w)*0.5f)+x, ((SCREEN_HEIGHT-h)*0.5f)+y, w, h, hShader );
	} else {
		trap_R_DrawStretchPic(  x + cg.refdef.x + 0.5 * ( cg.refdef.width - w ),
								y + cg.refdef.y + 0.5 * ( cg.refdef.height - h ),
								w, h, 0, 0, 1, 1, hShader );
	}

	trap_R_SetColor( NULL );
}

/*
==============
CG_PlayHitSound
==============
*/
void CG_PlayHitSound( hitEvent_t hitType ) {
	if ( hitType == HIT_NONE ) {
		return;
	}

	if ( !( cg_hitSounds.integer & HITSOUNDS_ON ) ) {
		return;
	}

	switch ( hitType ) {
	case HIT_TEAMSHOT:
		if ( !( cg_hitSounds.integer & HITSOUNDS_NOTEAMSHOT ) ) {
			trap_S_StartLocalSound( cgs.media.teamShot, CHAN_LOCAL_SOUND );
		}
		break;
	case HIT_HEADSHOT:
	case HIT_DEATHSHOT:
		if ( !( cg_hitSounds.integer & HITSOUNDS_NOHEADSHOT ) ) {
			trap_S_StartLocalSound( cgs.media.headShot, CHAN_LOCAL_SOUND );
		} else if ( !( cg_hitSounds.integer & HITSOUNDS_NOBODYSHOT ) ) {
			trap_S_StartLocalSound( cgs.media.bodyShot, CHAN_LOCAL_SOUND );
		}
		break;
	case HIT_BODYSHOT:
		if ( !( cg_hitSounds.integer & HITSOUNDS_NOBODYSHOT ) ) {
			trap_S_StartLocalSound( cgs.media.bodyShot, CHAN_LOCAL_SOUND );
		}
		break;
	default:
		break;
	}
}

// higher-priority hit types preempt a still-fading lower-priority marker
static qboolean canDrawNextHitMarker( hitEvent_t old, hitEvent_t new ) {
	if ( new >= old ) {
		return qtrue;
	}

	if ( trap_Milliseconds() - cg.hitMarker.startTime > HITMARKER_HIGHPRI_MIN_DURATION ) {
		return qtrue;
	}

	return qfalse;
}

/*
==============
CG_HitMarker
==============
*/
void CG_HitMarker( hitEvent_t hitType ) {
	if ( hitType <= HIT_NONE || hitType >= HIT_MAX_NUM ) {
		return;
	}

	if ( canDrawNextHitMarker( cg.hitMarker.hitType, hitType ) ) {
		cg.hitMarker.active = qtrue;
		cg.hitMarker.startTime = trap_Milliseconds();
		cg.hitMarker.hitType = hitType;
	}
}

/*
==============
CG_DrawHitMarker
==============
*/
static void CG_DrawHitMarker( void ) {
	float color[4];
	float alpha, scale, progress;
	int currentTime = trap_Milliseconds();
	float x, y, w, h;
	float baseSize, size;

	if ( !cg_drawHitMarker.integer || !cg.hitMarker.active || !cgs.media.hitMarkerShader ) {
		return;
	}

	progress = (float)( currentTime - cg.hitMarker.startTime ) / (float)HITMARKER_DURATION;
	if ( progress >= 1.0f ) {
		cg.hitMarker.active = qfalse;
		return;
	}

	if ( cg.snap->ps.eFlags & EF_MG42_ACTIVE ) {
		baseSize = 48.0f;
	} else if ( cg_hitMarkerSize.integer > 0 ) {
		baseSize = (float)cg_hitMarkerSize.integer;
	} else if ( cg_crosshairSize.value ) {
		baseSize = 0.9f * cg_crosshairSize.value;
	} else {
		baseSize = 32.0f;
	}

	x = SCREEN_WIDTH * 0.5f;
	y = SCREEN_HEIGHT * 0.5f;
	CG_AdjustFrom640( &x, &y, &w, &h );

	// fade-out over the last 30% of the duration
	if ( progress < 0.7f ) {
		alpha = 1.0f;
	} else {
		alpha = 1.0f - ( progress - 0.7f ) / 0.3f;
	}
	alpha *= cg_hitMarkerAlpha.value;

	color[0] = 1.0f;
	color[1] = 1.0f;
	color[2] = 1.0f;
	color[3] = alpha;

	if ( progress < 0.2f ) {
		scale = 1.0f + ( progress * 2.0f );            // 1.0 -> 1.4
	} else if ( progress < 0.4f ) {
		scale = 1.4f - ( progress - 0.2f ) * 2.0f;      // 1.4 -> 1.0
	} else {
		scale = 1.0f - ( progress - 0.4f ) * 1.0f;      // 1.0 -> 0.6
	}

	if ( !cg_solidHitMarker.integer ) {
		float f = (float)cg.snap->ps.aimSpreadScale / 255.0f;
		scale *= 1.0f + f * 0.75f;
	}

	switch ( cg.hitMarker.hitType ) {
	case HIT_TEAMSHOT:
		size = baseSize * scale;
		color[0] = 1.0f;
		color[1] = 1.0f;
		color[2] = 0.0f;
		break;
	case HIT_BODYSHOT:
		size = baseSize * scale;
		break;
	case HIT_HEADSHOT:
		size = baseSize * scale * 1.15f;
		if ( progress < 0.3f ) {
			float shake = sin( progress * 20.0f ) * 3.0f;
			x += shake;
			y += shake;
		}
		break;
	case HIT_DEATHSHOT:
		size = baseSize * scale * 1.25f;
		color[0] = 1.0f;
		color[1] = 0.1f;
		color[2] = 0.1f;
		// flash effect
		if ( (int)( progress * 20.0f ) % 2 == 0 ) {
			color[3] = alpha * 0.75f;
		}
		break;
	default:
		return;
	}

	trap_R_SetColor( color );
	trap_R_DrawStretchPic( x - size * 0.5f, y - size * 0.5f, size, size, 0, 0, 1, 1, cgs.media.hitMarkerShader );

	if ( ( cg.hitMarker.hitType == HIT_HEADSHOT || cg.hitMarker.hitType == HIT_DEATHSHOT ) && progress < 0.6f ) {
		float haloAlpha = alpha * 0.3f;
		float haloSize = size * 1.55f;
		float haloProgress = progress * 1.5f;

		if ( haloProgress < 1.0f ) {
			color[3] = haloAlpha * ( 1.0f - haloProgress );
			trap_R_SetColor( color );
			trap_R_DrawStretchPic( x - haloSize * 0.5f, y - haloSize * 0.5f, haloSize, haloSize, 0, 0, 1, 1, cgs.media.hitMarkerShader );
		}
	}

	trap_R_SetColor( NULL );
}


// damage numbers / enemy health bars (optional HUD features)
#define MAX_DAMAGE_NUMBERS     24
#define DAMAGE_NUMBER_DURATION 900     // ms
#define DAMAGE_NUMBER_RISE     28.0f   // units risen over its lifetime

typedef struct {
	qboolean active;
	int spawnTime;
	vec3_t origin;
	int amount;
	float ratio;        // amount / target maxHealth, clamped 0..1
} damageNumber_t;

static damageNumber_t cg_damageNumbers[MAX_DAMAGE_NUMBERS];
static int cg_nextDamageNumber = 0;

void CG_AddDamageNumber( vec3_t origin, int amount, int maxHealth ) {
	damageNumber_t *dn;

	if ( amount <= 0 ) {
		return;
	}

	dn = &cg_damageNumbers[ cg_nextDamageNumber ];
	cg_nextDamageNumber = ( cg_nextDamageNumber + 1 ) % MAX_DAMAGE_NUMBERS;

	dn->active = qtrue;
	dn->spawnTime = cg.time;
	VectorCopy( origin, dn->origin );
	// small random offset so stacked hits (eg. shotgun pellets) don't perfectly overlap
	dn->origin[0] += crandom() * 6.0f;
	dn->origin[1] += crandom() * 6.0f;
	// lift off world geometry a bit so the pre-rise visibility check below doesn't false-negative on explosive hits
	dn->origin[2] += 8.0f;
	dn->amount = amount;
	dn->ratio = ( maxHealth > 0 ) ? ( (float)amount / (float)maxHealth ) : 0.0f;
	if ( dn->ratio > 1.0f ) {
		dn->ratio = 1.0f;
	}
}

// white for chip damage, ramping through yellow/orange to red for hits that take a big bite out of max health
static void CG_ColorForDamageRatio( float ratio, vec4_t color ) {
	color[3] = 1.0f;
	if ( ratio < 0.15f ) {
		color[0] = 1.0f; color[1] = 1.0f; color[2] = 1.0f;
	} else if ( ratio < 0.35f ) {
		color[0] = 1.0f; color[1] = 1.0f; color[2] = 0.2f;
	} else if ( ratio < 0.6f ) {
		color[0] = 1.0f; color[1] = 0.55f; color[2] = 0.1f;
	} else {
		color[0] = 1.0f; color[1] = 0.15f; color[2] = 0.1f;
	}
}

void CG_DrawDamageNumbers( void ) {
	int i;
	fontInfoExtra_t *font = &cgDC.Assets.bigFont;

	if ( !cg_drawDamageNumbers.integer ) {
		return;
	}

	for ( i = 0; i < MAX_DAMAGE_NUMBERS; i++ ) {
		damageNumber_t *dn = &cg_damageNumbers[i];
		float progress, x, y, alpha, scale;
		vec4_t color;
		vec3_t drawOrigin;
		char text[16];

		if ( !dn->active ) {
			continue;
		}

		progress = (float)( cg.time - dn->spawnTime ) / (float)DAMAGE_NUMBER_DURATION;
		if ( progress >= 1.0f ) {
			dn->active = qfalse;
			continue;
		}

		VectorCopy( dn->origin, drawOrigin );
		drawOrigin[2] += DAMAGE_NUMBER_RISE * progress;

		// check LOS against the risen point we draw, not the raw impact point sitting right on world geometry
		if ( !CG_WorldToScreen( drawOrigin, &x, &y ) || !PointVisible( drawOrigin ) ) {
			continue;
		}

		alpha = ( progress < 0.6f ) ? 1.0f : 1.0f - ( progress - 0.6f ) / 0.4f;

		CG_ColorForDamageRatio( dn->ratio, color );
		color[3] = alpha;

		scale = 0.22f + 0.1f * dn->ratio;   // bigger hits get slightly bigger numbers
		Com_sprintf( text, sizeof( text ), "%i", dn->amount );

		CG_Text_Paint_Ext( x - CG_Text_Width_Ext( text, scale, 0, font ) * 0.5f, y, scale, scale,
							color, text, 0, 0, ITEM_TEXTSTYLE_NORMAL, font );
	}

	trap_R_SetColor( NULL );
}

// red-yellow-green ramp based on the fraction of health remaining
static void CG_ColorForHealthbar( float frac, vec4_t color ) {
	const float dim = 0.75f;    // pull the saturated primaries down a notch
	const float desat = 0.2f;   // blend a little toward neutral gray so it isn't neon
	float gray;

	if ( frac < 0.0f ) {
		frac = 0.0f;
	} else if ( frac > 1.0f ) {
		frac = 1.0f;
	}

	if ( frac < 0.5f ) {
		color[0] = 1.0f;
		color[1] = frac / 0.5f;
		color[2] = 0.0f;
	} else {
		color[0] = 1.0f - ( frac - 0.5f ) / 0.5f;
		color[1] = 1.0f;
		color[2] = 0.0f;
	}

	gray = ( color[0] + color[1] + color[2] ) / 3.0f;
	color[0] = ( color[0] * ( 1.0f - desat ) + gray * desat ) * dim;
	color[1] = ( color[1] * ( 1.0f - desat ) + gray * desat ) * dim;
	color[2] = ( color[2] * ( 1.0f - desat ) + gray * desat ) * dim;
	color[3] = 1.0f;
}

#define ENEMY_HEALTHBAR_HEIGHT  56.0f
#define ENEMY_HEALTHBAR_WIDTH   48.0f
#define ENEMY_HEALTHBAR_THICK   5.0f
#define ENEMY_HEALTHBAR_PAD     2.0f
#define AI_HEALTHINFO_STALE_TIME 2000  // drop entries the server hasn't refreshed in a while
#define AI_HEALTHINFO_DEATH_LINGER 400 // keep showing an empty bar this long after death, then hide

typedef struct {
	int health;
	int healthMax;
	int updateTime;
	int deathTime;      // cg.time health first reached 0, or 0 while still alive
} aiHealthInfo_t;

static aiHealthInfo_t cg_aiHealthInfo[MAX_CLIENTS];

// fed by the "aiHealth" server command (CG_ParseAIHealthInfo), broadcast periodically by AICastHealthInfoMessage in g_coop.c
void CG_SetAIHealth( int entnum, int health, int healthMax ) {
	aiHealthInfo_t *hi;

	if ( entnum < 0 || entnum >= MAX_CLIENTS ) {
		return;
	}
	hi = &cg_aiHealthInfo[entnum];

	if ( health > 0 ) {
		hi->deathTime = 0;              // alive (or a fresh spawn reusing this slot)
	} else if ( hi->deathTime == 0 ) {
		hi->deathTime = cg.time;        // just dropped to 0 -- start the linger countdown
	}

	hi->health = health;
	hi->healthMax = healthMax;
	hi->updateTime = cg.time;

	if ( cg_debugEvents.integer ) {
		CG_Printf( "AIHealth: received ent %i health %i/%i\n", entnum, health, healthMax );
	}
}

#define AI_NAME_MAXLEN 32

static char cg_aiNames[MAX_CLIENTS][AI_NAME_MAXLEN];

// debug only -- fed by the "aiNames" server command (CG_ParseAINameInfo), broadcast periodically by AICastNameInfoMessage in g_coop.c
void CG_SetAIName( int entnum, const char *name ) {
	if ( entnum < 0 || entnum >= MAX_CLIENTS ) {
		return;
	}
	Q_strncpyz( cg_aiNames[entnum], name, AI_NAME_MAXLEN );
}

// debug only: prints the scripting aiName above whatever AI is under the crosshair, regardless of distance
void CG_DrawAINameDebug( void ) {
	trace_t trace;
	vec3_t start, end, namePos;
	centity_t *cent;
	entityState_t *es;
	const char *name;
	char text[64];
	float x, y, scale;
	vec4_t color = { 1.0f, 1.0f, 1.0f, 1.0f };

	if ( !cg_showAINames.integer ) {
		return;
	}

	VectorCopy( cg.refdef.vieworg, start );
	VectorMA( start, 8192, cg.refdef.viewaxis[0], end );
	CG_Trace( &trace, start, vec3_origin, vec3_origin, end, cg.snap->ps.clientNum,
			  CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_ITEM );

	if ( trace.entityNum < 0 || trace.entityNum >= MAX_CLIENTS ) {
		return;
	}

	cent = &cg_entities[ trace.entityNum ];
	es = &cent->currentState;
	if ( es->aiChar == AICHAR_NONE ) {
		return;
	}

	VectorCopy( cent->lerpOrigin, namePos );
	namePos[2] += ENEMY_HEALTHBAR_HEIGHT + 14.0f;

	if ( !CG_WorldToScreen( namePos, &x, &y ) ) {
		return;
	}

	name = cg_aiNames[ trace.entityNum ];
	Com_sprintf( text, sizeof( text ), "ainame: %s  (ent %i)", name[0] ? name : "<none>", trace.entityNum );

	scale = 0.2f;
	CG_Text_Paint_Ext( x - CG_Text_Width_Ext( text, scale, 0, &cgDC.Assets.bigFont ) * 0.5f, y,
						scale, scale, color, text, 0, 0, ITEM_TEXTSTYLE_NORMAL, &cgDC.Assets.bigFont );
}

// draws a small health bar above the head of any AI that's close and near the crosshair, or directly aimed at regardless of range
void CG_DrawEnemyHealthbars( void ) {
	int num;
	float range, rangeSquared, radiusSquared;
	trace_t crosshairTrace;
	vec3_t traceStart, traceEnd;
	int crosshairEntNum;

	if ( !cg_drawEnemyHealthbars.integer ) {
		return;
	}

	range = cg_enemyHealthbarRange.value;
	rangeSquared = range * range;
	radiusSquared = cg_enemyHealthbarAimRadius.value * cg_enemyHealthbarAimRadius.value;

	// whatever's directly under the crosshair shows its bar regardless of distance
	VectorCopy( cg.refdef.vieworg, traceStart );
	VectorMA( traceStart, 8192, cg.refdef.viewaxis[0], traceEnd );
	CG_Trace( &crosshairTrace, traceStart, vec3_origin, vec3_origin, traceEnd,
			  cg.snap->ps.clientNum, CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_ITEM );
	crosshairEntNum = crosshairTrace.entityNum;

	for ( num = 0; num < cg.snap->numEntities; num++ ) {
		centity_t *cent = &cg_entities[ cg.snap->entities[num].number ];
		entityState_t *es = &cent->currentState;
		aiHealthInfo_t *hi;
		qboolean isCrosshairTarget;
		vec3_t headOrigin;
		float distSquared, x, y, dx, dy, frac, alpha;
		vec4_t color;

		if ( es->aiChar == AICHAR_NONE || es->number < 0 || es->number >= MAX_CLIENTS ) {
			continue;
		}

		hi = &cg_aiHealthInfo[ es->number ];
		if ( hi->healthMax <= 0 || cg.time - hi->updateTime > AI_HEALTHINFO_STALE_TIME ) {
			continue;
		}
		// dead: show an empty bar briefly so the kill reads as "reached 0", then get out of the way
		if ( hi->health <= 0 && cg.time - hi->deathTime > AI_HEALTHINFO_DEATH_LINGER ) {
			continue;
		}

		isCrosshairTarget = ( es->number == crosshairEntNum );

		distSquared = DistanceSquared( cent->lerpOrigin, cg.refdef.vieworg );
		if ( !isCrosshairTarget && distSquared > rangeSquared ) {
			continue;
		}

		VectorCopy( cent->lerpOrigin, headOrigin );
		headOrigin[2] += ENEMY_HEALTHBAR_HEIGHT;

		if ( !CG_WorldToScreen( headOrigin, &x, &y ) || !PointVisible( headOrigin ) ) {
			continue;
		}

		dx = x - 320.0f;
		dy = y - 240.0f;
		if ( !isCrosshairTarget && ( dx * dx + dy * dy ) > radiusSquared ) {
			continue;
		}

		frac = ( hi->health > 0 ) ? (float)hi->health / (float)hi->healthMax : 0.0f;
		if ( frac > 1.0f ) {
			frac = 1.0f;
		}

		CG_ColorForHealthbar( frac, color );

		// fade out over the last 20% of range instead of popping abruptly into view (skipped for the crosshair target)
		alpha = 1.0f;
		if ( !isCrosshairTarget && distSquared > rangeSquared * 0.64f ) {
			alpha = 1.0f - ( sqrt( distSquared / rangeSquared ) - 0.8f ) / 0.2f;
		}
		// also fade the lingering empty bar out instead of having it just vanish
		if ( hi->health <= 0 ) {
			alpha *= 1.0f - (float)( cg.time - hi->deathTime ) / (float)AI_HEALTHINFO_DEATH_LINGER;
		}
		color[3] = alpha;

		// solid backing drawn explicitly instead of BAR_BG, whose full-width translucent box hides small fill changes
		{
			vec4_t bgColor = { 0.0f, 0.0f, 0.0f, 0.45f * alpha };
			CG_FillRect( x - ENEMY_HEALTHBAR_WIDTH * 0.5f - ENEMY_HEALTHBAR_PAD,
						 y - ENEMY_HEALTHBAR_THICK * 0.5f - ENEMY_HEALTHBAR_PAD,
						 ENEMY_HEALTHBAR_WIDTH + ENEMY_HEALTHBAR_PAD * 2.0f,
						 ENEMY_HEALTHBAR_THICK + ENEMY_HEALTHBAR_PAD * 2.0f, bgColor );
		}

		CG_FilledBar( x - ENEMY_HEALTHBAR_WIDTH * 0.5f, y - ENEMY_HEALTHBAR_THICK * 0.5f,
					  ENEMY_HEALTHBAR_WIDTH, ENEMY_HEALTHBAR_THICK, color, NULL, NULL, frac, 8 ); // BAR_NOHUDALPHA (alpha already applied above)
	}

	trap_R_SetColor( NULL );
}


/*
=================
CG_DrawCrosshair3D
=================
*/
static void CG_DrawCrosshair3D( void ) {
	float w;
	qhandle_t hShader;
	float f;
	int weapnum;                // DHM - Nerve
	vec4_t hcolor = {1, 1, 1, 1};
	qboolean friendInSights = qfalse;

	trace_t trace;
	vec3_t endpos;
	float stereoSep, zProj, maxdist, xmax;
	char rendererinfos[128];
	refEntity_t ent;

	if ( cg.renderingThirdPerson ) {
		return;
	}

	hcolor[3] = cg_crosshairAlpha.value;    //----(SA)	added

	if ( cg_fixedAspect.integer ) {
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
	}

	// on mg42
	if ( cg.snap->ps.eFlags & EF_MG42_ACTIVE ) {
		hcolor[0] = hcolor[1] = hcolor[2] = 0.0f;
		hcolor[3] = 0.6f;
		// option 1
//		CG_FillRect (300, 240, 40, 2, hcolor);	// horizontal
//		CG_FillRect (319, 242, 2, 16, hcolor);	// vertical

		// option 2
		CG_FillRect( 305, 240, 30, 2, hcolor );  // horizontal
		CG_FillRect( 314, 256, 12, 2, hcolor );  // horizontal2
		CG_FillRect( 319, 242, 2, 32, hcolor );  // vertical

		return;
	}

	friendInSights = (qboolean)( cg.snap->ps.serverCursorHint == HINT_PLYR_FRIEND );  //----(SA)	added

	// DHM - Nerve :: show reticle in limbo and spectator
	if ( cg.demoPlayback ) {
		weapnum = cg.snap->ps.weapon;
	} else {
		weapnum = cg.weaponSelect;
	}

	switch ( weapnum ) {

		// weapons that get no reticle
	case WP_NONE:       // no weapon, no crosshair
	case WP_SNOOPER:
		if ( cg.zoomedBinoc ) 
		{
			CG_DrawBinocReticle();
		}
	if ( !cg_snipersCrosshair.integer ) 
	    {
		return;
	    }
		break;
		// special reticle for weapon
	case WP_KNIFE:
		if ( cg.zoomedBinoc ) {
			CG_DrawBinocReticle();
			return;
			// Crosshair shouldn't be shown at all when holding a knife.
		}
		else
			return;
		break;
	case WP_SNIPERRIFLE:
	case WP_SNOOPERSCOPE:
	case WP_FG42SCOPE:
	case WP_M1941SCOPE:
	case WP_DELISLESCOPE:
		if ( !( cg.snap->ps.eFlags & EF_MG42_ACTIVE ) ) {
			CG_DrawWeapReticle();
			return;
		}
		break;
	default:
		break;
	}

	// using binoculars
	if ( cg.zoomedBinoc ) {
		CG_DrawBinocReticle();
		return;
	}

	// mauser only gets crosshair if you don't have the scope (I don't like this, but it's a test)
	if ( cg.weaponSelect == WP_MAUSER ) {
		if ( COM_BitCheck( cg.predictedPlayerState.weapons, WP_SNIPERRIFLE ) ) {
		if ( !cg_snipersCrosshair.integer )
	    {
		return;
	    }
		}
	}

	if ( cg.weaponSelect == WP_M1941 ) {
		if ( COM_BitCheck( cg.predictedPlayerState.weapons, WP_M1941SCOPE ) ) {
		if ( !cg_snipersCrosshair.integer )
	    {
		return;
	    }
		}
	}

	if ( cg.weaponSelect == WP_DELISLE ) {
		if ( COM_BitCheck( cg.predictedPlayerState.weapons, WP_DELISLESCOPE ) ) {
		if ( !cg_snipersCrosshair.integer )
	    {
		return;
	    }
		}
	}


	if ( !cg_drawCrosshair.integer ) {	//----(SA)	moved down so it doesn't keep the scoped weaps from drawing reticles
		return;
	}

	// no crosshair while leaning
	if ( cg.snap->ps.leanf ) {
		return;
	}

	// no crosshair when looking at exits
	if ( cg.snap->ps.serverCursorHint >= HINT_EXIT && cg.snap->ps.serverCursorHint <= HINT_NOEXIT_FAR ) {
		return;
	}

	if ( cg_paused.integer ) {
		// no draw if any menu's are up	 (or otherwise paused)
		return;
	}

	w = cg_crosshairSize.value;

	// RF, crosshair size represents aim spread
	if ( !cg_solidCrosshair.integer ) {
		f = (float)cg.snap->ps.aimSpreadScale / 255.0;
		w *= ( 1 + f * 2.0 );
	}

//----(SA)	modified
	if ( friendInSights ) {
		hShader = cgs.media.crosshairFriendly;
	} else {
		hShader = cgs.media.crosshairShader[ cg_drawCrosshair.integer % NUM_CROSSHAIRS ];
	}
//----(SA)	end

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

	ent.shaderRGBA[0]=255;
	ent.shaderRGBA[1]=255;
	ent.shaderRGBA[2]=255;
	ent.shaderRGBA[3]=(byte)(hcolor[3]*255.f);

	trap_R_AddRefEntityToScene(&ent);
}


/*
=================
CG_ScanForCrosshairEntity
=================
*/
static void CG_ScanForCrosshairEntity( void ) {
	trace_t trace;
//	gentity_t	*traceEnt;
	vec3_t start, end;
	int content;

	// DHM - Nerve :: We want this in multiplayer
	if ( cgs.gametype == GT_SINGLE_PLAYER ) {
		return; //----(SA)	don't use any scanning at the moment.

	}
	VectorCopy( cg.refdef.vieworg, start );
	VectorMA( start, 8192, cg.refdef.viewaxis[0], end );

	CG_Trace( &trace, start, vec3_origin, vec3_origin, end,
			  cg.snap->ps.clientNum, CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_ITEM );

//	if ( trace.entityNum >= MAX_CLIENTS ) {
//		return;
//	}

//	traceEnt = &g_entities[trace.entityNum];


	// if the player is in fog, don't show it
	content = CG_PointContents( trace.endpos, 0 );
	if ( content & CONTENTS_FOG ) {
		return;
	}

	// if the player is invisible, don't show it
	if ( cg_entities[ trace.entityNum ].currentState.powerups & ( 1 << PW_INVIS ) ) {
		return;
	}

	// update the fade timer
	cg.crosshairClientNum = trace.entityNum;
	cg.crosshairClientTime = cg.time;
//	if ( cg.crosshairClientNum != cg.identifyClientNum && cg.crosshairClientNum != ENTITYNUM_WORLD ) {
//		cg.identifyClientRequest = cg.crosshairClientNum;
//	}
}



/*
==============
CG_DrawGrenadeCount

Always-visible grenade ammo readout, regardless of equipped weapon.
==============
*/
static void CG_DrawGrenadeCount( void ) {
	playerState_t *ps;
	int wp;
	int value;
	qhandle_t icon;
	float grow, pulse;

	// modern HUD draws its own copy via the CG_PLAYER_GRENADE_COUNT ownerdraw in cg_hud.c
	if ( !cg_oldWolfUI.integer ) {
		return;
	}

	if ( cg.snap->ps.stats[STAT_HEALTH] <= 0 ) {
		return;
	}

	ps = &cg.snap->ps;

	if ( COM_BitCheck( ps->weapons, WP_GRENADE_LAUNCHER ) ) {
		wp = WP_GRENADE_LAUNCHER;
	} else if ( COM_BitCheck( ps->weapons, WP_GRENADE_PINEAPPLE ) ) {
		wp = WP_GRENADE_PINEAPPLE;
	} else {
		return;
	}

	if ( cgs.dmflags & DF_NO_WEAPRELOAD ) {
		value = ps->ammo[ BG_FindAmmoForWeapon( wp ) ];
	} else {
		value = ps->ammoclip[ BG_FindClipForWeapon( wp ) ];
	}

	if ( value <= 0 ) {
		return;
	}

	if ( cg_fixedAspect.integer == 2 ) {
		CG_SetScreenPlacement( PLACE_RIGHT, PLACE_BOTTOM );
	}

	// icons are normally only registered lazily when their bank shows in the weapon-select strip
	CG_RegisterWeapon( wp );

	icon = cg_weapons[ wp ].weaponIcon[0];

	// pulse while actually cooking (quick-grenade or held grenade weapon)
	pulse = 0.0f;
	if ( cg.predictedPlayerState.grenadeTimeLeft > 0 ) {
		pulse = 1.0f - ( (float)( cg.predictedPlayerState.grenadeTimeLeft % 1000 ) / 1000.0f );
	}
	grow = 0.3f * pulse * 24;

	if ( icon ) {
		trap_R_SetColor( NULL );
		CG_DrawPic( 555 - 0.5f * grow, 436 - 0.5f * grow, 24 + grow, 24 + grow, icon );
	}

	CG_DrawBigString2( 555 + 28, 436, va( "%d", value ), cg_hudAlpha.value );
}

/*
==============
CG_DrawDynamiteStatus
==============
*/
static void CG_DrawDynamiteStatus( void ) {
	float color[4];
	char        *name;
	int timeleft;
	float w;

	if ( cg.snap->ps.weapon != WP_DYNAMITE ) {
		return;
	}

	if ( cg.snap->ps.grenadeTimeLeft <= 0 ) {
		return;
	}

	timeleft = cg.snap->ps.grenadeTimeLeft;

//	color = g_color_table[ColorIndex(COLOR_RED)];
	color[0] = color[3] = 1.0f;

	// fade red as it pulses past seconds
	color[1] = color[2] = 1.0f - ( (float)( timeleft % 1000 ) * 0.001f );

	if ( timeleft < 300 ) {        // fade up the text
		color[3] = (float)timeleft / 300.0f;
	}

	trap_R_SetColor( color );

	timeleft *= 5;
	timeleft -= ( timeleft % 5000 );
	timeleft += 5000;
	timeleft /= 1000;

	name = va( "Timer: %d", timeleft );
	w = CG_DrawStrlen( name ) * BIGCHAR_WIDTH;

	color[3] *= cg_hudAlpha.value;
	CG_DrawBigStringColor( 320 - w / 2, 170, name, color );

	trap_R_SetColor( NULL );
}

/*
==============
CG_ReviveBarCenterX

cg_fixedAspect defaults off, and in that mode the virtual coordinate width
tracks the real aspect ratio instead of staying at the classic 640 (4:3)
reference, so a literal x=320 lands well left of true center on a wide
display. Recompute the actual center instead of assuming 640-space.
==============
*/
static float CG_ReviveBarCenterX( void ) {
	// earlier draws this frame (e.g. CG_DrawGrenadeCount) can leave screen placement
	// pointed at PLACE_RIGHT/PLACE_LEFT and never reset it -- force center explicitly
	CG_SetScreenPlacement( PLACE_CENTER, PLACE_CENTER );

	if ( cg_fixedAspect.integer || !cgs.glconfig.vidHeight ) {
		return 320.0f;
	}
	return 240.0f * cgs.glconfig.vidWidth / (float)cgs.glconfig.vidHeight;
}

/*
==============
CG_DrawBleedoutStatus

GT_COOP_SURVIVAL only: countdown bar shown to a bleeding-out player while
STAT_REVIVE_TIME counts down (frozen server-side while someone revives them).
A PERK_SECONDCHANCE owner gets a hint that they can revive themselves too.
==============
*/
static void CG_DrawBleedoutStatus( void ) {
	vec4_t color = { 0.8f, 0.1f, 0.1f, 1.0f };
	vec4_t bgColor = { 0, 0, 0, 0.6f };
	float frac, centerX, barW;
	int maxTime;
	char *s;
	float w;

	maxTime = cg_reviveBleedoutTime.integer;
	if ( maxTime < 1 ) {
		maxTime = 1;
	}

	frac = (float)cg.snap->ps.stats[STAT_REVIVE_TIME] / (float)maxTime;
	if ( frac < 0.0f ) {
		frac = 0.0f;
	} else if ( frac > 1.0f ) {
		frac = 1.0f;
	}

	centerX = CG_ReviveBarCenterX();
	barW = 200;

	CG_FilledBar( centerX - barW / 2, 400, barW, 14, color, NULL, bgColor, frac, BAR_BG );

	s = ( cg.snap->ps.perks[PERK_SECONDCHANCE] > 0 ) ?
		"Bleeding out -- hold ACTIVATE to revive yourself, or wait for a teammate" :
		"Bleeding out -- wait for a teammate to revive you";
	w = CG_DrawStrlen( s ) * SMALLCHAR_WIDTH;
	CG_DrawSmallStringColor( centerX - w / 2, 384, s, colorWhite );
}

/*
==============
CG_DrawReviveProgress

GT_COOP_SURVIVAL only: progress bar shown to a player currently holding
ACTIVATE to revive a downed teammate.
==============
*/
static void CG_DrawReviveProgress( void ) {
	vec4_t color = { 0.2f, 0.6f, 1.0f, 1.0f };
	vec4_t bgColor = { 0, 0, 0, 0.6f };
	float frac, centerX, barW;
	char *s;
	float w;

	if ( cg.snap->ps.stats[STAT_REVIVE_PROGRESS] <= 0 ) {
		return;
	}

	// s.loopSound (set server-side on our own entity) never reaches our own predicted
	// entity, so the reviver has to hear their own loop via a direct local add instead
	CG_S_AddLoopingSound( cg.snap->ps.clientNum, cg.snap->ps.origin, vec3_origin,
						   ( cg.snap->ps.stats[STAT_PLAYER_CLASS] == PC_MEDIC ) ? cgs.media.reviveLoopMedicSound : cgs.media.reviveLoopSound, 255 );

	frac = (float)cg.snap->ps.stats[STAT_REVIVE_PROGRESS] / 100.0f;
	if ( frac > 1.0f ) {
		frac = 1.0f;
	}

	centerX = CG_ReviveBarCenterX();
	barW = 200;

	CG_FilledBar( centerX - barW / 2, 400, barW, 14, color, NULL, bgColor, frac, BAR_BG );

	s = "Reviving...";
	w = CG_DrawStrlen( s ) * SMALLCHAR_WIDTH;
	CG_DrawSmallStringColor( centerX - w / 2, 384, s, colorWhite );
}

/*
==============
CG_DrawConstructProgress

GT_COOP_SURVIVAL only: progress bar shown to a player currently holding
ACTIVATE to build a func_constructible.
==============
*/
#define CONSTRUCT_SOUND_TAIL_MS 600
static int cg_constructLastActiveTime = 0;

static void CG_DrawConstructProgress( void ) {
	vec4_t color = { 0.8f, 0.6f, 0.2f, 1.0f };
	vec4_t bgColor = { 0, 0, 0, 0.6f };
	float frac, centerX, barW;
	char *s;
	float w;

	if ( cg.snap->ps.stats[STAT_CONSTRUCT_PROGRESS] > 0 ) {
		cg_constructLastActiveTime = cg.time;
	}

	// local-add since s.loopSound never reaches our own predicted entity; kept alive briefly after stopping so it isn't cut off mid-cycle
	if ( cg.time - cg_constructLastActiveTime < CONSTRUCT_SOUND_TAIL_MS ) {
		CG_S_AddLoopingSound( cg.snap->ps.clientNum, cg.snap->ps.origin, vec3_origin, cgs.media.constructLoopSound, 255 );
	}

	if ( cg.snap->ps.stats[STAT_CONSTRUCT_PROGRESS] <= 0 ) {
		return;
	}

	frac = (float)cg.snap->ps.stats[STAT_CONSTRUCT_PROGRESS] / 100.0f;
	if ( frac > 1.0f ) {
		frac = 1.0f;
	}

	centerX = CG_ReviveBarCenterX();
	barW = 200;

	CG_FilledBar( centerX - barW / 2, 400, barW, 14, color, NULL, bgColor, frac, BAR_BG );

#ifdef LOCALISATION
	s = CG_TranslateString( "Building..." );   // msgid lives in translations/wolftech.lang
#else
	s = "Building...";
#endif
	w = CG_DrawStrlen( s ) * SMALLCHAR_WIDTH;
	CG_DrawSmallStringColor( centerX - w / 2, 384, s, colorWhite );
}



/*
==============
CG_CheckForCursorHints
==============
*/
void CG_CheckForCursorHints( void ) {

	if ( cg.renderingThirdPerson ) {
		return;
	}

	if ( cg.snap->ps.serverCursorHint != HINT_NONE ) { // let the client remember what was last looked at (for fading out)
		cg.cursorHintTime = cg.time;
		cg.cursorHintFade = cg_hintFadeTime.integer;    // fade out time
		cg.cursorHintIcon = cg.snap->ps.serverCursorHint;
		cg.cursorHintValue = cg.snap->ps.serverCursorHintVal;
	}

	// (SA) (8/14/01) removed all the client-side stuff.  don't think it's really necessary anymore
}

/*
=====================
CG_DrawCoopCrosshairNames
=====================
*/
static void CG_DrawCoopCrosshairNames( void ) {
	float       *color;
	char        *name;
	float w;
	// NERVE - SMF
	const char  *s;
	int playerHealth, val;
	vec4_t c;
	float barFrac;
	int armor;
	// -NERVE - SMF
	centity_t *cent;

	if ( cg_drawCrosshair.integer < 0 ) {
		return;
	}
	if ( !cg_drawCrosshairNames.integer ) {
		return;
	}
	if ( cg.renderingThirdPerson ) {
		return;
	}

	// Ridah
	if ( cg_gameType.integer == GT_SINGLE_PLAYER ) {
		return;
	}
	// done.

	if ( cg_fixedAspect.integer ) {
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
	}

	// scan the known entities to see if the crosshair is sighted on one
	CG_ScanForCrosshairEntity();

	// draw the name of the player being looked at
	color = CG_FadeColor( cg.crosshairClientTime, 1000 );

	if ( !color ) {
		trap_R_SetColor( NULL );
		return;
	}

	// NERVE - SMF
	if ( cg.crosshairClientNum > MAX_CLIENTS ) {
		return;
	}

	// we only want to see players on our team
	if ( cgs.clientinfo[cg.snap->ps.clientNum].team != TEAM_SPECTATOR
		 && cgs.clientinfo[ cg.crosshairClientNum ].team != cgs.clientinfo[cg.snap->ps.clientNum].team ) {
		return;
	}

	cent = &cg_entities[ cg.crosshairClientNum ];

	// TODO make this look nice
	if ( cent->currentState.eFlags & EF_FROZEN ) {
		s = va( "Player is frozen." );
		w = CG_DrawStrlen( s ) * SMALLCHAR_WIDTH;
		CG_DrawSmallStringColor( 320 - w / 2, 170, s, color );

		s = va( "Stab him with knife to respawn him." );
		w = CG_DrawStrlen( s ) * SMALLCHAR_WIDTH;
		CG_DrawSmallStringColor( 320 - w / 2, 170 + SMALLCHAR_HEIGHT + 5, s, color );
		return;
	}

	playerHealth = cgs.clientinfo[ cg.crosshairClientNum ].health;

	if ( playerHealth <= 0 ) {
		return;
	}

	val = cgs.clientinfo[ cg.crosshairClientNum ].curWeapon;
	name = cgs.clientinfo[ cg.crosshairClientNum ].name;
	armor = cgs.clientinfo[ cg.crosshairClientNum ].armor;

	//s = va( "[%s] %s", CG_TranslateString( playerClass ), name );
	//s = va( "[%s] %s", playerClass, name );
	if ( val == WP_KNIFE ) {
		s = va( "%s", name );
	} else if ( val == WP_GRENADE_LAUNCHER || val == WP_GRENADE_PINEAPPLE || val == WP_DYNAMITE || val == WP_FLAMETHROWER || val == WP_TESLA ) {
		s = va( "[%d] %s", cgs.clientinfo[ cg.crosshairClientNum ].ammoClip, name );
	} else {
		s = va( "[%d/%d] %s", cgs.clientinfo[ cg.crosshairClientNum ].ammoClip, cgs.clientinfo[ cg.crosshairClientNum ].ammo, name );
	}


	//s = va( "[%d]%s",armor , name );
	if ( !s ) {
		return;
	}
	w = CG_DrawStrlen( s ) * SMALLCHAR_WIDTH;

	// draw the name and class
	CG_DrawSmallStringColor( 320 - w / 2, 170, s, color );

	// health bar
	barFrac = (float)playerHealth / 100;

	if ( barFrac > 1.0 ) {
		barFrac = 1.0;
	} else if ( barFrac < 0 ) {
		barFrac = 0;
	}

	c[0] = 1.0f;
	c[1] = c[2] = barFrac;
	c[3] = 0.25 + barFrac * 0.5 * color[3];

	CG_FilledBar( 241, 190, 158, 10, c, NULL, NULL, barFrac, 16 );


	// armor bar
	if ( cg_drawCrosshairNames.integer == 2 ) {
		barFrac = (float)armor / 100;

		if ( barFrac > 1.0 ) {
			barFrac = 1.0;
		} else if ( barFrac < 0 ) {
			barFrac = 0;
		}

		c[0] = 1.0f;
		c[1] = c[2] = barFrac;
		c[3] = 0.25 + barFrac * 0.5 * color[3];

		CG_FilledBar( 241, 205, 158, 10, c, NULL, NULL, barFrac, 16 );
	}

	trap_R_SetColor( NULL );
}


/*
=====================
CG_DrawCrosshairNames
=====================
*/
static void CG_DrawCrosshairNames( void ) {
	float       *color;
	vec4_t teamColor;           // NERVE - SMF
	char        *name;
	float w;

	if ( !cg_drawCrosshair.integer ) {
		return;
	}
	if ( !cg_drawCrosshairNames.integer ) {
		return;
	}
	if ( cg.renderingThirdPerson ) {
		return;
	}

	// Ridah
	if ( cg_gameType.integer == GT_SINGLE_PLAYER ) {
		return;
	}
	// done.

	if ( cg_fixedAspect.integer ) {
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
	}

	// scan the known entities to see if the crosshair is sighted on one
	CG_ScanForCrosshairEntity();

	// draw the name of the player being looked at
	color = CG_FadeColor( cg.crosshairClientTime, 1000 );

	if ( !color ) {
		trap_R_SetColor( NULL );
		return;
	}

	// NERVE - SMF - use fade alpha but color text according to teams
	teamColor[3] = color[3];

	// NERVE - SMF - no longer identify opposing side, so just use green now
//	if ( cgs.clientinfo[ cg.crosshairClientNum ].team != cgs.clientinfo[ cg.clientNum ].team )
//		VectorSet( teamColor, 0.7608, 0.1250, 0.0859 );			// LIGHT-RED
//	else
	VectorSet( teamColor, 0.1250, 0.7608, 0.0859 );             // LIGHT-GREEN

	trap_R_SetColor( teamColor );
	// -NERVE - SMF

	name = cgs.clientinfo[ cg.crosshairClientNum ].name;
	w = CG_DrawStrlen( va( "%s", name ) ) * BIGCHAR_WIDTH;
//	CG_DrawBigString( 320 - w / 2, 170, name, color[3] * 0.5 );

	// NERVE - SMF
	if ( strlen( name ) ) {
		if ( ( cgs.clientinfo[ cg.crosshairClientNum ].team == TEAM_RED ) &&
			 ( cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_RED ) ) { // JPW NERVE -- only show same team info so people can't pan-search
			CG_DrawBigStringColor( 320 - w / 2, 170, va( "Axis: %s", name ), teamColor );
		} else if ( ( cgs.clientinfo[ cg.crosshairClientNum ].team == TEAM_BLUE ) &&
					( cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_BLUE ) ) { // JPW NERVE -- so's we can't find snipers for free
			CG_DrawBigStringColor( 320 - w / 2, 170, va( "Ally: %s", name ), teamColor );
		} else {
			CG_DrawBigStringColor( 320 - w / 2, 170, va( "%s", name ), teamColor );
		}
	}
	// -NERVE - SMF

	trap_R_SetColor( NULL );
}



//==============================================================================

/*
=================
CG_DrawSpectator
=================
*/
static void CG_DrawSpectator( void ) {
	if ( cg_fixedAspect.integer == 2 ) {
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_BOTTOM);
	}

	CG_DrawBigString( 320 - 9 * 8, 440, "SPECTATOR", 1.0F );
}

/*
=================
CG_DrawVote
NOTE: Vote sounds are set in game logic (c_cmds.c /callvote)..
=================
*/
static void CG_DrawVote( void ) {
	char *s;
	int sec;
	char str1[32], str2[32];
	float color[4] = { 1, 1, 0, 1 };

	if ( !cgs.voteTime ) {
		return;
	}

	Q_strncpyz( str1, BindingFromName( "vote yes" ), 32 );
	if ( !Q_stricmp( str1, "???" ) ) {
		Q_strncpyz( str1, "vote yes", 32 );
	}
	Q_strncpyz( str2, BindingFromName( "vote no" ), 32 );
	if ( !Q_stricmp( str2, "???" ) ) {
		Q_strncpyz( str2, "vote no", 32 );
	}

	if ( cg_fixedAspect.integer == 2 ) {
		CG_SetScreenPlacement(PLACE_LEFT, PLACE_CENTER);
	}

	// play a talk beep whenever it is modified
	if ( cgs.voteModified ) {
		cgs.voteModified = qfalse;
	}

	sec = ( VOTE_TIME - ( cg.time - cgs.voteTime ) ) / 1000;
	if ( sec < 0 ) {
		sec = 0;
	}

	if ( !( cg.snap->ps.eFlags & EF_VOTED ) ) {
#ifdef LOCALISATION
		s = va( CG_TranslateString( "VOTE(%i):%s" ), sec, cgs.voteString );
#else
		s = va( "VOTE(%i):%s", sec, cgs.voteString );
#endif
		// Should we push this down?
		// strip ^N codes so server-controlled voteString can't override the fixed vote-text color
		Q_StripColorCodes( s );
		CG_ChatTextPaint( 8, 200, CG_CHAT_TEXT_SCALE, CG_CHAT_TEXT_SCALE, color, s, 0, 60, 0, CG_CHAT_FONT );

#ifdef LOCALISATION
		s = va( CG_TranslateString( "YES(%s):%i, NO(%s):%i" ), str1, cgs.voteYes, str2, cgs.voteNo );
#else
		s = va( "YES(%s):%i, NO(%s):%i", str1, cgs.voteYes, str2, cgs.voteNo );
#endif
		CG_ChatTextPaint( 8, 214, CG_CHAT_TEXT_SCALE, CG_CHAT_TEXT_SCALE, color, s, 0, 60, 0, CG_CHAT_FONT );
	} else {

#ifdef LOCALISATION
		s = va( CG_TranslateString( "Y:%i, N:%i" ), cgs.voteYes, cgs.voteNo );
#else
		s = va( "Y:%i, N:%i", cgs.voteYes, cgs.voteNo );
#endif
		CG_ChatTextPaint( 8, 214, CG_CHAT_TEXT_SCALE, CG_CHAT_TEXT_SCALE, color, s, 0, 20, 0, CG_CHAT_FONT );
	}
}

/*
=================
CG_DrawIntermission
=================
*/
static void CG_DrawIntermission( void ) {
	if ( cgs.gametype == GT_SINGLE_PLAYER ) {
		CG_DrawCenterString();
		CG_DrawBuyString();
		CG_DrawSubtitleString();
		return;
	}

	cg.scoreFadeTime = cg.time;
	CG_DrawScoreboard();
}

// NERVE - SMF
/*
=================
CG_ActivateLimboMenu
=================
*/
// TTimo: unused
/*
static void CG_ActivateLimboMenu( void ) {
	static qboolean latch = qfalse;
	qboolean test;
	char buf[32];

	if ( cgs.gametype != GT_WOLF )
		return;

	// should we open the limbo menu
	test = cg.snap->ps.pm_flags & PMF_LIMBO || cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR;

	if ( test && !latch ) {
		trap_SendConsoleCommand( "startLimboMode\n" );
		trap_SendConsoleCommand( "OpenLimboMenu\n" );
		latch = qtrue;
	}
	else if ( !test && latch ) {
		trap_SendConsoleCommand( "stopLimboMode\n" );
		trap_SendConsoleCommand( "CloseLimboMenu\n" );
		latch = qfalse;
	}

	// set the limbo state
	trap_Cvar_VariableStringBuffer( "ui_limboMode", buf, sizeof( buf ) );

	if ( atoi( buf ) )
		cg.limboMenu = qtrue;
	else
		cg.limboMenu = qfalse;
}
*/
// -NERVE - SMF

#define INFOTEXT_STARTX 42

/*
=================
CG_DrawFollow
=================
*/
static qboolean CG_DrawFollow( void ) {
	//float		x;
	vec4_t color;
	const char  *name;
	char deploytime[128];        // JPW NERVE

	if ( !( cg.snap->ps.pm_flags & PMF_FOLLOW ) ) {
		return qfalse;
	}

	if ( cg_fixedAspect.integer == 2 ) {
		CG_SetScreenPlacement(PLACE_LEFT, PLACE_TOP);
	}

	color[0] = 1;
	color[1] = 1;
	color[2] = 1;
	color[3] = 1;

	// JPW NERVE -- if in limbo, show different follow message
	if ( cg.snap->ps.pm_flags & PMF_LIMBO ) {
		color[1] = 0.0;
		color[2] = 0.0;
		if ( cgs.gametype == GT_COOP_SURVIVAL ) {
#ifdef LOCALISATION
			Q_strncpyz( deploytime, CG_TranslateString( "Waiting for next wave..." ), sizeof(deploytime) );
#else
			Q_strncpyz( deploytime, "Waiting for next wave...", sizeof(deploytime) );
#endif
		} else if ( cg.snap->ps.persistant[PERS_RESPAWNS_LEFT] == 0 ) {
#ifdef LOCALISATION
			Q_strncpyz( deploytime, CG_TranslateString( "No more deployments this round" ), sizeof(deploytime) );
#else
			Q_strncpyz( deploytime, "No more deployments this round", sizeof(deploytime) );
#endif
		} else {
#ifdef LOCALISATION
			Com_sprintf( deploytime, sizeof(deploytime), CG_TranslateString( "Deploying in %d seconds" ),
					 (int)( 1 + (float)( cg_limbotime.integer - ( cg.time % cg_limbotime.integer ) ) * 0.001f ) );
#else
			Com_sprintf( deploytime, sizeof(deploytime), "Deploying in %d seconds",
					 (int)( 1 + (float)( cg_limbotime.integer - ( cg.time % cg_limbotime.integer ) ) * 0.001f ) );
#endif
		}

		CG_ChatTextPaint( INFOTEXT_STARTX, 68, CG_CHAT_TEXT_SCALE, CG_CHAT_TEXT_SCALE, color, deploytime, 0, 80, 0, CG_CHAT_FONT );

		// DHM - Nerve :: Don't display if you're following yourself
		if ( cg.snap->ps.clientNum != cg.clientNum ) {
#ifdef LOCALISATION
			Com_sprintf( deploytime, sizeof(deploytime), "(%s %s)", CG_TranslateString( "Following" ), cgs.clientinfo[ cg.snap->ps.clientNum ].name );
#else
			Com_sprintf( deploytime, sizeof(deploytime), "(%s %s)", "Following", cgs.clientinfo[ cg.snap->ps.clientNum ].name );
#endif
			// player name may contain ^N color codes; strip them to keep the fixed color
			Q_StripColorCodes( deploytime );
			CG_ChatTextPaint( INFOTEXT_STARTX, 86, CG_CHAT_TEXT_SCALE, CG_CHAT_TEXT_SCALE, color, deploytime, 0, 80, 0, CG_CHAT_FONT );
		}
	} else {
		// jpw
#ifdef LOCAISATION
		CG_DrawSmallString( INFOTEXT_STARTX, 68, CG_TranslateString( "following" ), 1.0F );
#else
		CG_DrawSmallString( INFOTEXT_STARTX, 68, "following", 1.0F );
#endif

		Q_strncpyz( deploytime, cgs.clientinfo[ cg.snap->ps.clientNum ].name, sizeof( deploytime ) );
		Q_StripColorCodes( deploytime );
		name = deploytime;

		CG_ChatTextPaint( 120, 68, CG_CHAT_TEXT_SCALE * ( (float)BIGCHAR_WIDTH / SMALLCHAR_WIDTH ), CG_CHAT_TEXT_SCALE * ( (float)BIGCHAR_WIDTH / SMALLCHAR_WIDTH ), color, name, 0, 0, ITEM_TEXTSTYLE_SHADOWED, CG_CHAT_FONT );
	} // JPW NERVE
	return qtrue;
}

/*
=================
CG_DrawAmmoWarning
=================
*/
static void CG_DrawAmmoWarning( void ) {
//----(SA)	forcing return for now
//			if we have messages to show here, comment back in
#if 0
	const char  *s;
	int w;

	if ( cg_drawAmmoWarning.integer == 0 ) {
		return;
	}

	if ( !cg.lowAmmoWarning ) {
		return;
	}

	if ( cg_fixedAspect.integer == 2 ) {
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_TOP);
	}

	if ( cg.lowAmmoWarning == 2 ) {
		s = "OUT OF AMMO";
	} else {
		s = "LOW AMMO WARNING";
	}
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
	CG_DrawBigString( 320 - w / 2, 64, s, 1.0F );
#endif
}

/*
=================
Countdown audio

Atm commented out as It doesn't work..
=================
*/
/*
static void countDownAnnouncer( int cnt ) {

	if (!cg_announcer.integer)
		return;

	// Argh...ugly!
	if (cnt == 5)
		trap_S_StartLocalSound( cgs.media.count5Sound , CHAN_ANNOUNCER ); // Count 5
	else if (cnt == 4)
		trap_S_StartLocalSound( cgs.media.count4Sound , CHAN_ANNOUNCER ); // Count 4
	else if (cnt == 3)
		trap_S_StartLocalSound( cgs.media.count3Sound , CHAN_ANNOUNCER ); // Count 3
	else if (cnt == 2)
		trap_S_StartLocalSound( cgs.media.count2Sound , CHAN_ANNOUNCER ); // Count 2
	else if (cnt == 1)
		trap_S_StartLocalSound( cgs.media.count5Sound , CHAN_ANNOUNCER ); // Count 1

	return;
}
*/

/*
=================
CG_DrawWarmup
=================
*/
static void CG_DrawWarmup( void ) {
	int w;
	int sec;
	int cw;
	const char  *s;

	if ( cgs.gametype == GT_SINGLE_PLAYER ) {
		return;     // (SA) don't bother with this stuff in sp
	}

	sec = cg.warmup;
	if ( !sec ) {
/*
                if ( cgs.gamestate == GS_WAITING_FOR_PLAYERS ) {
                        cw = 10;

                        s = CG_TranslateString( "Game Stopped - Waiting for more players" );

                        w = CG_DrawStrlen( s );
                        CG_DrawStringExt( 320 - w * 6, 120, s, colorWhite, qfalse, qtrue, 12, 18, 0 );


                        s1 = va( CG_TranslateString( "Waiting for %i players" ), cgs.minclients );
                        s2 = CG_TranslateString( "or call a vote to start match" );

                        w = CG_DrawStrlen( s1 );
                        CG_DrawStringExt( 320 - w * cw / 2, 160, s1, colorWhite,
                                                          qfalse, qtrue, cw, (int)( cw * 1.5 ), 0 );

                        w = CG_DrawStrlen( s2 );
                        CG_DrawStringExt( 320 - w * cw / 2, 180, s2, colorWhite,
                                                          qfalse, qtrue, cw, (int)( cw * 1.5 ), 0 );

                        return;
                }
*/
		return;
	}

	if ( cg_fixedAspect.integer == 2 ) {
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_TOP);
	}

	if ( sec < 0 ) {
		return;
	}
	
	sec = ( sec - cg.time ) / 1000;

	if ( sec < 0 ) {
		sec = 0;
	}
	
	//  Don't show if announcer is enabled!
	//if ( sec == 0 && !cg_announcer.integer) {
	//        s = va( "^2FIGHT!");
	//} else {
	s = va( "(WARMUP) Game starts in: %i", sec + 1 );
	// countDownAnnouncer(sec); // Needs fixing / FIXME
	//}

	cw = 16;

	{
		float scale = CG_CHAT_TEXT_SCALE * ( (float)cw / SMALLCHAR_WIDTH );
		w = CG_Text_Width_Ext( s, scale, 0, CG_CHAT_FONT );
		CG_ChatTextPaint( 320 - w / 2, 120, scale, scale, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWED, CG_CHAT_FONT );
	}
}

//==================================================================================

/*
=================
CG_DrawFlashFade
=================
*/
static void CG_DrawFlashFade( void ) {
	static int lastTime;
	int elapsed, time;
	vec4_t col;

	if ( cgs.scrFadeStartTime + cgs.scrFadeDuration < cg.time ) {
		cgs.scrFadeAlphaCurrent = cgs.scrFadeAlpha;
	} else if ( cgs.scrFadeAlphaCurrent != cgs.scrFadeAlpha ) {
		elapsed = ( time = trap_Milliseconds() ) - lastTime;  // we need to use trap_Milliseconds() here since the cg.time gets modified upon reloading
		lastTime = time;
		if ( elapsed < 500 && elapsed > 0 ) {
			if ( cgs.scrFadeAlphaCurrent > cgs.scrFadeAlpha ) {
				cgs.scrFadeAlphaCurrent -= ( (float)elapsed / (float)cgs.scrFadeDuration );
				if ( cgs.scrFadeAlphaCurrent < cgs.scrFadeAlpha ) {
					cgs.scrFadeAlphaCurrent = cgs.scrFadeAlpha;
				}
			} else {
				cgs.scrFadeAlphaCurrent += ( (float)elapsed / (float)cgs.scrFadeDuration );
				if ( cgs.scrFadeAlphaCurrent > cgs.scrFadeAlpha ) {
					cgs.scrFadeAlphaCurrent = cgs.scrFadeAlpha;
				}
			}
		}
	}
	// now draw the fade
	if ( cgs.scrFadeAlphaCurrent > 0.0 ) {
		VectorClear( col );
		col[3] = cgs.scrFadeAlphaCurrent;
		if ( cg_fixedAspect.integer ) {
			CG_SetScreenPlacement(PLACE_STRETCH, PLACE_STRETCH);
			CG_FillRect( 0, 0, 640, 480, col );
			CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
		} else {
			CG_FillRect( 0, 0, 640, 480, col );
		}
	}
}



/*
==============
CG_DrawFlashZoomTransition
	hide the snap transition from regular view to/from zoomed

  FIXME: TODO: use cg_fade?
==============
*/
static void CG_DrawFlashZoomTransition( void ) {
	vec4_t color;
	float frac;
	int fadeTime = 0;

	if ( !cg.snap ) {
		return;
	}

	if ( cg.snap->ps.eFlags & EF_MG42_ACTIVE ) {   // don't draw when on mg_42
		// keep the timer fresh so when you remove yourself from the mg42, it'll fade
		cg.zoomTime = cg.time;
		return;
	}

	if ( cgs.gametype <= GT_SINGLE_PLAYER ) { // JPW NERVE
		if ( cg.zoomedScope ) {
			fadeTime = cg.zoomedScope;  //----(SA)
		} else {
			fadeTime = 300;
		}
	}
	// jpw

	frac = cg.time - cg.zoomTime;

	if ( frac < fadeTime ) {
		frac = frac / (float)fadeTime;

		if ( cg.weaponSelect == WP_SNOOPERSCOPE ) {
//			Vector4Set( color, 0.7f, 0.3f, 0.7f, 1.0f - frac );
//			Vector4Set( color, 1, 0.5, 1, 1.0f - frac );
//			Vector4Set( color, 0.5f, 0.3f, 0.5f, 1.0f - frac );
			Vector4Set( color, 0.7f, 0.6f, 0.7f, 1.0f - frac );
		} else {
			Vector4Set( color, 0, 0, 0, 1.0f - frac );
		}

		if ( cg_fixedAspect.integer ) {
			CG_SetScreenPlacement(PLACE_STRETCH, PLACE_STRETCH);
			CG_FillRect( -10, -10, 650, 490, color );
			CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
		} else {
			CG_FillRect( -10, -10, 650, 490, color );
		}
	}
}



/*
=================
CG_DrawFlashDamage
=================
*/
static void CG_DrawFlashDamage( void ) {
	vec4_t col;
	float redFlash;

	if ( !cg.snap ) {
		return;
	}

	// Blood blending
	if ( cg_bloodBlend.integer ) {
		return;
	}
	// end

	if ( cg.v_dmg_time > cg.time ) {
		redFlash = fabs( cg.v_dmg_pitch * ( ( cg.v_dmg_time - cg.time ) / DAMAGE_TIME ) );

		// blend the entire screen red
		if ( redFlash > 5 ) {
			redFlash = 5;
		}

		VectorSet( col, 0.2, 0, 0 );
		col[3] =  0.7 * ( redFlash / 5.0 );

		if ( cg_fixedAspect.integer ) {
			CG_SetScreenPlacement(PLACE_STRETCH, PLACE_STRETCH);
			CG_FillRect( -10, -10, 650, 490, col );
			CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
		} else {
			CG_FillRect( -10, -10, 650, 490, col );
		}
	}
}


/*
=================
CG_DrawFlashFire
=================
*/
static void CG_DrawFlashFire( void ) {
	vec4_t col = {1,1,1,1};
	float alpha, max, f;

	if ( !cg.snap ) {
		return;
	}

	if ( cg_thirdPerson.integer ) {
		return;
	}

	if ( cg.cameraMode ) { // don't draw flames on camera screen.  will still do damage though, so not a potential cheat
		return;
	}

	if ( !cg.snap->ps.onFireStart ) {
		cg.v_noFireTime = cg.time;
		return;
	}

	alpha = (float)( ( FIRE_FLASH_TIME - 1000 ) - ( cg.time - cg.snap->ps.onFireStart ) ) / ( FIRE_FLASH_TIME - 1000 );
	if ( alpha > 0 ) {
		if ( alpha >= 1.0 ) {
			alpha = 1.0;
		}

		// fade in?
		f = (float)( cg.time - cg.v_noFireTime ) / FIRE_FLASH_FADEIN_TIME;
		if ( f >= 0.0 && f < 1.0 ) {
			alpha = f;
		}

		max = 0.5 + 0.5 * sin( (float)( ( cg.time / 10 ) % 1000 ) / 1000.0 );
		if ( alpha > max ) {
			alpha = max;
		}
		col[0] = alpha;
		col[1] = alpha;
		col[2] = alpha;
		col[3] = alpha;
		trap_R_SetColor( col );
		if ( cg_fixedAspect.integer ) {
			CG_SetScreenPlacement(PLACE_STRETCH, PLACE_STRETCH);
			CG_DrawPic( -10, -10, 650, 490, cgs.media.viewFlashFire[( cg.time / 50 ) % 16] );
			CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
		} else {
			CG_DrawPic( -10, -10, 650, 490, cgs.media.viewFlashFire[( cg.time / 50 ) % 16] );
		}
		trap_R_SetColor( NULL );

		CG_S_AddLoopingSound( cg.snap->ps.clientNum, cg.snap->ps.origin, vec3_origin, cgs.media.flameSound, (int)( 255.0 * alpha ) );
		CG_S_AddLoopingSound( cg.snap->ps.clientNum, cg.snap->ps.origin, vec3_origin, cgs.media.flameCrackSound, (int)( 255.0 * alpha ) );
	} else {
		cg.v_noFireTime = cg.time;
	}
}

/*
=================
CG_DrawFlashLightning
=================
*/
static void CG_DrawFlashLightning( void ) {
	centity_t *cent;
	qhandle_t shader;

	if ( !cg.snap ) {
		return;
	}

	if ( cg_thirdPerson.integer ) {
		return;
	}

	cent = &cg_entities[cg.snap->ps.clientNum];

	if ( !cent->pe.teslaDamagedTime || ( cent->pe.teslaDamagedTime > cg.time ) ) {
		return;
	}

	if ( ( cg.time / 50 ) % ( 2 + ( cg.time % 2 ) ) == 0 ) {
		shader = cgs.media.viewTeslaAltDamageEffectShader;
	} else {
		shader = cgs.media.viewTeslaDamageEffectShader;
	}

	if ( cg_fixedAspect.integer ) {
		CG_SetScreenPlacement(PLACE_STRETCH, PLACE_STRETCH);
		CG_DrawPic( -10, -10, 650, 490, shader );
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
	} else {
		CG_DrawPic( -10, -10, 650, 490, shader );
	}
}


/*
==============
CG_DrawFlashBlendBehindHUD
	screen flash stuff drawn first (on top of world, behind HUD)
==============
*/
static void CG_DrawFlashBlendBehindHUD( void ) {
	CG_DrawFlashZoomTransition();
}


/*
=================
CG_DrawFlashBlend
	screen flash stuff drawn last (on top of everything)
=================
*/
static void CG_DrawFlashBlend( void ) {
	CG_DrawFlashLightning();
	CG_DrawFlashFire();
	CG_DrawFlashDamage();
	CG_DrawFlashFade();
}

// NERVE - SMF
/*
=================
CG_DrawObjectiveInfo
=================
*/
#define OID_LEFT    10
#define OID_TOP     65

void CG_ObjectivePrint( const char *str, int charWidth, int team ) {
	char    *s;

	Q_strncpyz( cg.oidPrint, str, sizeof( cg.oidPrint ) );

	cg.oidPrintTime = cg.time;
	cg.oidPrintY = OID_TOP;
	cg.oidPrintCharWidth = charWidth;

	// count the number of lines for oiding
	cg.oidPrintLines = 1;
	s = cg.oidPrint;
	while ( *s ) {
		if ( *s == '\n' ) {
			cg.oidPrintLines++;
		}
		s++;
	}
}

static void CG_DrawObjectiveInfo( void ) {
	char    *start;
	int l;
	int x, y, w;
	int x1, y1, x2, y2;
	float   *color;
	float oidScale;
	vec4_t backColor = { 0.2f, 0.2f, 0.2f, 1.f };

	if ( !cg.oidPrintTime ) {
		return;
	}

	color = CG_FadeColor( cg.oidPrintTime, 1000 * 5 );
	if ( !color ) {
		return;
	}

	if ( cg_fixedAspect.integer == 2 ) {
		CG_SetScreenPlacement(PLACE_LEFT, PLACE_TOP);
	}

	trap_R_SetColor( color );

	oidScale = CG_CHAT_TEXT_SCALE * ( (float)cg.oidPrintCharWidth / SMALLCHAR_WIDTH );

	start = cg.oidPrint;

	y = cg.oidPrintY - cg.oidPrintLines * BIGCHAR_HEIGHT / 2;

	x1 = OID_LEFT - 2;
	y1 = y - 2;
	x2 = 0;

	// first just find the bounding rect
	while ( 1 ) {
		char linebuffer[1024];

		for ( l = 0; l < 40; l++ ) {
			if ( !start[l] || start[l] == '\n' ) {
				break;
			}
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;

		w = CG_Text_Width_Ext( linebuffer, oidScale, 0, CG_CHAT_FONT );
		if ( x1 + w > x2 ) {
			x2 = x1 + w;
		}

		y += cg.oidPrintCharWidth * 1.5;

		while ( *start && ( *start != '\n' ) ) {
			start++;
		}
		if ( !*start ) {
			break;
		}
		start++;
	}

	x2 = x2 + 4;
	y2 = y - cg.oidPrintCharWidth * 1.5 + 4;

	backColor[3] = color[3];
	CG_FillRect( x1, y1, x2 - x1, y2 - y1, backColor );

	VectorSet( backColor, 0, 0, 0 );
	CG_DrawRect( x1, y1, x2 - x1, y2 - y1, 1, backColor );

	//trap_R_SetColor( color );

	// do the actual drawing
	start = cg.oidPrint;
	y = cg.oidPrintY - cg.oidPrintLines * BIGCHAR_HEIGHT / 2;

	while ( 1 ) {
		char linebuffer[1024];

		for ( l = 0; l < 40; l++ ) {
			if ( !start[l] || start[l] == '\n' ) {
				break;
			}
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;

		w = CG_Text_Width_Ext( linebuffer, oidScale, 0, CG_CHAT_FONT );
		if ( x1 + w > x2 ) {
			x2 = x1 + w;
		}

		x = OID_LEFT;

		CG_ChatTextPaint( x, y, oidScale, oidScale, color, linebuffer, 0, 0, ITEM_TEXTSTYLE_SHADOWED, CG_CHAT_FONT );

		y += cg.oidPrintCharWidth * 1.5;

		while ( *start && ( *start != '\n' ) ) {
			start++;
		}
		if ( !*start ) {
			break;
		}
		start++;
	}

	if ( cg_fixedAspect.integer ) {
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
	}

	trap_R_SetColor( NULL );
}
// -NERVE - SMF

//==================================================================================


void CG_DrawTimedMenus( void ) {
	if ( cg.voiceTime ) {
		int t = cg.time - cg.voiceTime;
		if ( t > 2500 ) {
			Menus_CloseByName( "voiceMenu" );
			trap_Cvar_Set( "cl_conXOffset", "0" );
			cg.voiceTime = 0;
		}
	}
}


/*
=================
CG_Fade
=================
*/
void CG_Fade( int r, int g, int b, int a, int time, int duration ) {

	// incorporate this into the current fade scheme

	cgs.scrFadeAlpha = (float)a / 255.0f;
	cgs.scrFadeStartTime = time;
	cgs.scrFadeDuration = duration;

	if ( cgs.scrFadeStartTime + cgs.scrFadeDuration <= cg.time ) {
		cgs.scrFadeAlphaCurrent = cgs.scrFadeAlpha;
	}

	return;
}


/*
===============
CG_DrawGameScreenFade
===============
*/
static void CG_DrawGameScreenFade( void ) {
	vec4_t col;

	if ( cg.viewFade <= 0.0 ) {
		return;
	}

	if ( !cg.snap ) {
		return;
	}

	VectorClear( col );
	col[3] = cg.viewFade;
	if ( cg_fixedAspect.integer ) {
		CG_SetScreenPlacement(PLACE_STRETCH, PLACE_STRETCH);
		CG_FillRect( 0, 0, 640, 480, col );
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
	} else {
		CG_FillRect( 0, 0, 640, 480, col );
	}
}

/*
=================
CG_ScreenFade
=================
*/
static void CG_ScreenFade( void ) {
	int msec;
	int i;
	float t, invt;
	vec4_t color;

	// Ridah, fade the screen (in-game)
	CG_DrawGameScreenFade();

	if ( !cg.fadeRate ) {
		return;
	}

	msec = cg.fadeTime - cg.time;
	if ( msec <= 0 ) {
		cg.fadeColor1[ 0 ] = cg.fadeColor2[ 0 ];
		cg.fadeColor1[ 1 ] = cg.fadeColor2[ 1 ];
		cg.fadeColor1[ 2 ] = cg.fadeColor2[ 2 ];
		cg.fadeColor1[ 3 ] = cg.fadeColor2[ 3 ];

		if ( !cg.fadeColor1[ 3 ] ) {
			cg.fadeRate = 0;
			return;
		}

		if ( cg_fixedAspect.integer ) {
			CG_SetScreenPlacement(PLACE_STRETCH, PLACE_STRETCH);
			CG_FillRect( 0, 0, 640, 480, cg.fadeColor1 );
			CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
		} else {
			CG_FillRect( 0, 0, 640, 480, cg.fadeColor1 );
		}

	} else {
		t = ( float )msec * cg.fadeRate;
		invt = 1.0f - t;

		for ( i = 0; i < 4; i++ ) {
			color[ i ] = cg.fadeColor1[ i ] * t + cg.fadeColor2[ i ] * invt;
		}

		if ( color[ 3 ] ) {
			if ( cg_fixedAspect.integer ) {
				CG_SetScreenPlacement(PLACE_STRETCH, PLACE_STRETCH);
				CG_FillRect( 0, 0, 640, 480, color );
				CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
			} else {
				CG_FillRect( 0, 0, 640, 480, color );
			}
		}
	}
}



/*
=================
CG_Draw2D
=================
*/
static void CG_Draw2D(stereoFrame_t stereoFrame) {

	// if we are taking a levelshot for the menu, don't draw anything
	if ( cg.levelShot ) {
		return;
	}

	if ( cg.cameraMode ) { //----(SA)	no 2d when in camera view
		CG_DrawFlashBlend();    // (for fades)
		return;
	}

	if ( cg_draw2D.integer == 0 ) {
		CG_DrawFlashBlend();    // (for fades)
		return;
	}

	CG_ScreenFade();

	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		CG_DrawIntermission();
		return;
	}

	CG_DrawFlashBlendBehindHUD();

	if ( cgs.localServer ) {
		CG_DrawOnScreenText();
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		CG_DrawSpectator();

		if(stereoFrame == STEREO_CENTER) {
			CG_DrawCrosshair();
			CG_DrawHitMarker();
		}

		CG_DrawCrosshairNames();
		if ( cgs.gametype != GT_SINGLE_PLAYER ) {
			CG_DrawTeamInfo();
		}
	} else {
		// don't draw any status if dead
		if ( cg.snap->ps.stats[STAT_HEALTH] > 0 ) {

			if(stereoFrame == STEREO_CENTER) {
				CG_DrawCrosshair();
				CG_DrawHitMarker();
			}

			if ( cg_drawStatus.integer ) {
				if ( cg_fixedAspect.integer == 2 ) {
					CG_SetScreenPlacement(PLACE_LEFT, PLACE_BOTTOM);
				}

				if ( !cg_oldWolfUI.integer ) {
					Menu_PaintAll();
					CG_DrawTimedMenus();
				}
			}

			if ( cgs.gametype != GT_SINGLE_PLAYER ) {
				CG_DrawTeamInfo();
			}

			if ( cg_oldWolfUI.integer ) {
				CG_DrawStatusBar();
				CG_DrawHoldableItem_old();
			}
			CG_DrawAmmoWarning();
			CG_DrawDynamiteStatus();
			CG_DrawGrenadeCount();
			CG_DrawReviveProgress();
			CG_DrawConstructProgress();
			CG_DrawCoopCrosshairNames();
			CG_DrawEnemyHealthbars();
			CG_DrawDamageNumbers();
			CG_DrawAINameDebug();
			CG_DrawWeaponSelect();
			CG_DrawHoldableSelect();
			CG_DrawPickupItem();
			CG_DrawReward();
		} else if ( cgs.gametype == GT_COOP_SURVIVAL && cg.snap->ps.stats[STAT_GAMEOVER] ) {
			// post-wipe game-over sequence: force scoreboard + chat on; STAT_HEALTH/STAT_REVIVE_TIME aren't reliable here
			if ( cgs.gametype != GT_SINGLE_PLAYER ) {
				CG_DrawTeamInfo();
			}
			// CG_DrawScoreboard() normally sets this; without it the box renders off-center on wide aspects
			if ( cg_fixedAspect.integer ) {
				CG_SetScreenPlacement( PLACE_CENTER, PLACE_CENTER );
			}
			CG_DrawCoopScoreboard();
		} else if ( cgs.gametype == GT_COOP_SURVIVAL && cg.snap->ps.stats[STAT_REVIVE_TIME] > 0 ) {
			// STAT_REVIVE_PROGRESS > 0 means we're self-reviving -- show the progress bar instead of the frozen timer
			if ( cg.snap->ps.stats[STAT_REVIVE_PROGRESS] > 0 ) {
				CG_DrawReviveProgress();
			} else {
				CG_DrawBleedoutStatus();
			}
			if ( cgs.gametype != GT_SINGLE_PLAYER ) {
				CG_DrawTeamInfo(); // chat overlay lives in here -- bleeding out shouldn't cut you off from it
			}
		}
	}

//	if ( cgs.gametype >= GT_TEAM ) {
//		CG_DrawTeamInfo();
//	}

	CG_DrawVote();

	CG_DrawLagometer();

	if ( !cg_paused.integer ) {
		CG_DrawUpperRight(stereoFrame);
	}

	if ( !CG_DrawFollow() ) {
		CG_DrawWarmup();
	}

	// don't draw center string if scoreboard is up
	if ( !CG_DrawScoreboard() ) {
		CG_DrawNotify();

		CG_DrawCenterString();
		CG_DrawBuyString();
		CG_DrawPerkChooser();
		CG_DrawSubtitleString();

		CG_DrawObjectiveInfo();     // NERVE - SMF
	}

	// announcer
	CG_DrawAnnouncer();

	// Ridah, draw flash blends now
	CG_DrawFlashBlend();

	CG_DrawFreeze();
}

/*
====================
CG_StartShakeCamera
====================
*/
void CG_StartShakeCamera( float p, int duration, vec3_t src, float radius ) {
	int i;

	// find a free shake slot
	for ( i = 0; i < MAX_CAMERA_SHAKE; i++ ) {
		if ( cg.cameraShake[i].time > cg.time || cg.cameraShake[i].time + cg.cameraShake[i].length <= cg.time ) {
			break;
		}
	}

	if ( i == MAX_CAMERA_SHAKE ) {
		return; // no free slots

	}
	cg.cameraShake[i].scale = p;

	cg.cameraShake[i].length = duration;
	cg.cameraShake[i].time = cg.time;
	VectorCopy( src, cg.cameraShake[i].src );
	cg.cameraShake[i].radius = radius;
}

/*
====================
CG_CalcShakeCamera
====================
*/
void CG_CalcShakeCamera( void ) {
	float val, scale, dist, x, sx;
	float bx = 0.0f; // TTimo: init
	int i;

	// build the scale
	scale = 0.0f;
	sx = (float)cg.time / 600.0; // x * (float)(cg.cameraShake[i].length) / 600.0;
	for ( i = 0; i < MAX_CAMERA_SHAKE; i++ ) {
		if ( cg.cameraShake[i].time <= cg.time && cg.cameraShake[i].time + cg.cameraShake[i].length > cg.time ) {
			dist = Distance( cg.cameraShake[i].src, cg.refdef.vieworg );
			// fade with distance
			val = cg.cameraShake[i].scale * ( 1.0f - ( dist / cg.cameraShake[i].radius ) );
			// fade with time
			x = 1.0f - ( ( cg.time - cg.cameraShake[i].time ) / cg.cameraShake[i].length );
			val *= x;
			// overwrite global scale if larger
			if ( val > scale ) {
				scale = val;
				bx = x;
			}
		}
	}

	// check the current rumble status
	if ( cg.rumbleScale > scale ) {
		scale = cg.rumbleScale;
		bx = cg.rumbleScale;
	}

	if ( scale <= 0.0f ) {
		cg.cameraShakePhase = crandom() * M_PI; // randomize the phase
		return;
	}

	if ( scale > 1.0f ) {
		scale = 1.0f;
	}

	// up/down
	val = sin( M_PI * 8 * sx + cg.cameraShakePhase ) * bx * 18.0f * scale;
	cg.cameraShakeAngles[0] = val;
	//cg.refdefViewAngles[0] += val;

	// left/right
	val = sin( M_PI * 15 * sx + cg.cameraShakePhase ) * bx * 16.0f * scale;
	cg.cameraShakeAngles[1] = val;
	//cg.refdefViewAngles[1] += val;

	// roll
	val = sin( M_PI * 12 * sx + cg.cameraShakePhase ) * bx * 10.0f * scale;
	cg.cameraShakeAngles[2] = val;
	//cg.refdefViewAngles[2] += val;
}

/*
====================
CG_ApplyShakeCamera
====================
*/
void CG_ApplyShakeCamera( void ) {
	VectorAdd( cg.refdefViewAngles, cg.cameraShakeAngles, cg.refdefViewAngles );
	AnglesToAxis( cg.refdefViewAngles, cg.refdef.viewaxis );
}

// NERVE - SMF
void CG_CoopStartShakeCamera( float p ) {
	cg.cameraShakeScale = p;

	cg.cameraShakeLength = 1000 * ( p * p );
	cg.cameraShakeTime = cg.time + cg.cameraShakeLength;
	cg.cameraShakePhase = crandom() * M_PI;     // start chain in random dir
}

void CG_ShakeCamera( void ) {
	float x, val;

	if ( cg.time > cg.cameraShakeTime ) {
		cg.cameraShakeScale = 0;         // JPW NERVE all pending explosions resolved, so reset shakescale
		return;
	}

	// JPW NERVE starts at 1, approaches 0 over time
	x = ( cg.cameraShakeTime - cg.time ) / cg.cameraShakeLength;

	// up/down
	val = sin( M_PI * 8 * x + cg.cameraShakePhase ) * x * 18.0f * cg.cameraShakeScale;
	cg.refdefViewAngles[0] += val;

	// left/right
	val = sin( M_PI * 15 * x + cg.cameraShakePhase ) * x * 16.0f * cg.cameraShakeScale;
	cg.refdefViewAngles[1] += val;

	AnglesToAxis( cg.refdefViewAngles, cg.refdef.viewaxis );
}
// -NERVE - SMF

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

	// if they are waiting at the mission stats screen, show the stats
	if ( cg_gameType.integer <= GT_SINGLE_PLAYER ) {
		if ( strlen( cg_missionStats.string ) > 1 ) {
			trap_Cvar_Set( "com_expectedhunkusage", "-2" );
			CG_DrawInformation();
			return;
		}
	}

	// clear around the rendered view if sized down
	//CG_TileClear();	// (SA) moved down

	if(stereoView != STEREO_CENTER)
		CG_DrawCrosshair3D();

	cg.refdef.glfog.registered = 0; // make sure it doesn't use fog from another scene
/*
	// NERVE - SMF - activate limbo menu and draw small 3d window
	CG_ActivateLimboMenu();

	if ( cg.limboMenu ) {
		float x, y, w, h;
		x = LIMBO_3D_X;
		y = LIMBO_3D_Y;
		w = LIMBO_3D_W;
		h = LIMBO_3D_H;

		cg.refdef.width = 0;
		CG_AdjustFrom640( &x, &y, &w, &h );

		cg.refdef.x = x;
		cg.refdef.y = y;
		cg.refdef.width = w;
		cg.refdef.height = h;
	}
	// -NERVE - SMF
*/
	cg.refdef.rdflags |= RDF_DRAWSKYBOX;
	if ( !cg_skybox.integer ) {
		cg.refdef.rdflags &= ~RDF_DRAWSKYBOX;
	}

	trap_R_RenderScene( &cg.refdef );

	// draw the first-person view weapon in its own pass, at a fixed FOV independent of cg_fov
	CG_RenderViewWeapon();

	if ( ( cg.snap->ps.pm_flags & PMF_LIMBO || cg.snap->ps.stats[STAT_HEALTH] <= 0 || cg.snap->ps.eFlags & EF_FROZEN ) && cg_fancyfx.integer ) {
		static int texid = 0;

		if ( !texid ) {
			texid = trap_R_GetTextureId( "textures/rtt.tga" );
		}

		trap_R_RenderToTexture( texid, 0, 0, cg.refdef.width, cg.refdef.height );
		trap_R_ClearScene();
		trap_R_DrawStretchPic( 0, 0, cg.refdef.width, cg.refdef.height, 0, 1, 1, 0, cgs.media.rtt );
	}

	// clear around the rendered view if sized down
	CG_TileClear();     //----(SA)	moved to 2d section to avoid 2d/3d fog-state problems

	// draw status bar and other floating elements
	CG_Draw2D(stereoView);
}

// omnibot 3d text
#define COLOR_NULL      '*' // probably should move to q_shared.h

void CG_Text_PaintChar_Ext( float x, float y, float w, float h, float scalex, float scaley, float s, float t, float s2, float t2, qhandle_t hShader ) {
	w *= scalex;
	h *= scaley;
	CG_AdjustFrom640( &x, &y, &w, &h );
	trap_R_DrawStretchPic( x, y, w, h, s, t, s2, t2, hShader );
}

/*
================
CG_Text_Width_Ext / CG_Text_Height_Ext / CG_Text_Paint_Ext

All fonts are fontInfoExtra_t (Unicode-capable) now: text is decoded as UTF-8
codepoint by codepoint instead of indexed byte-by-byte, and glyph lookup goes
through Q_UTF8_GetGlyphExtended instead of a direct glyphs[c] index. ASCII
text behaves identically to the old byte-indexed path (Q_UTF8_CodePoint of an
ASCII byte returns that same byte, Q_UTF8_Width returns 1), so this is a
drop-in replacement for existing callers.
================
*/
int CG_Text_Width_Ext( const char *text, float scale, int limit, fontInfoExtra_t *font ) {
	float out;
	glyphInfo_t *glyph;
	float useScale;
	const char *s = text;

	useScale = scale * font->glyphScale;

	out = 0;
	if ( text ) {
		int len = Q_UTF8_Strlen( text );
		int count = 0;

		if ( limit > 0 && len > limit ) {
			len = limit;
		}

		while ( s && *s && count < len ) {
			if ( Q_IsColorString( s ) ) {
				s += 2;
				continue;
			} else {
				glyph = Q_UTF8_GetGlyphExtended( font, Q_UTF8_CodePoint( s ) );
				out += glyph->xSkip;
				s += Q_UTF8_Width( s );
				count++;
			}
		}
	}

	return out * useScale;
}

int CG_Text_Height_Ext( const char *text, float scale, int limit, fontInfoExtra_t *font ) {
	float max;
	glyphInfo_t *glyph;
	float useScale;
	const char *s = text;

	useScale = scale * font->glyphScale;
	max = 0;
	if ( text ) {
		int len = Q_UTF8_Strlen( text );
		int count = 0;

		if ( limit > 0 && len > limit ) {
			len = limit;
		}

		while ( s && *s && count < len ) {
			if ( Q_IsColorString( s ) ) {
				s += 2;
				continue;
			} else {
				glyph = Q_UTF8_GetGlyphExtended( font, Q_UTF8_CodePoint( s ) );

				if ( max < glyph->height ) {
					max = glyph->height;
				}

				s += Q_UTF8_Width( s );
				count++;
			}
		}
	}
	return max * useScale;
}

void CG_Text_Paint_Ext( float x, float y, float scalex, float scaley, vec4_t color, const char *text, float adjust, int limit, int style, fontInfoExtra_t *font ) {
	int len, count;
	vec4_t newColor;
	glyphInfo_t *glyph;

	scalex *= font->glyphScale;
	scaley *= font->glyphScale;

	if ( text ) {
		const char *s = text;
		trap_R_SetColor( color );
		memcpy( &newColor[0], &color[0], sizeof( vec4_t ) );
		len = Q_UTF8_Strlen( text );
		if ( limit > 0 && len > limit ) {
			len = limit;
		}
		count = 0;
		while ( s && *s && count < len ) {
			if ( Q_IsColorString( s ) ) {
				if ( *( s + 1 ) == COLOR_NULL ) {
					memcpy( newColor, color, sizeof( newColor ) );
				} else {
					memcpy( newColor, g_color_table[ColorIndex( *( s + 1 ) )], sizeof( newColor ) );
					newColor[3] = color[3];
				}
				trap_R_SetColor( newColor );
				s += 2;
				continue;
			} else {
				float yadj;
				glyph = Q_UTF8_GetGlyphExtended( font, Q_UTF8_CodePoint( s ) );
				yadj = scaley * glyph->top;
				if ( style == ITEM_TEXTSTYLE_SHADOWED || style == ITEM_TEXTSTYLE_SHADOWEDMORE ) {
					int ofs = style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;
					colorBlack[3] = newColor[3];
					trap_R_SetColor( colorBlack );
					CG_Text_PaintChar_Ext( x + ofs, y - yadj + ofs, glyph->imageWidth, glyph->imageHeight, scalex, scaley, glyph->s, glyph->t, glyph->s2, glyph->t2, glyph->glyph );
					colorBlack[3] = 1.0;
					trap_R_SetColor( newColor );
				}
				CG_Text_PaintChar_Ext( x, y - yadj, glyph->imageWidth, glyph->imageHeight, scalex, scaley, glyph->s, glyph->t, glyph->s2, glyph->t2, glyph->glyph );
				x += ( glyph->xSkip * scalex ) + adjust;
				s += Q_UTF8_Width( s );
				count++;
			}
		}
		trap_R_SetColor( NULL );
	}
}

#define MAX_WORLDTEXT 128
#define MAX_TEXTLENGTH 512 // fix for 3d waypoint text
#define MAX_RENDERDIST 2500

typedef struct onsText_s
{
	struct onsText_s *next;
	int endtime;
	vec4_t color;
	char text[MAX_TEXTLENGTH];
	vec3_t origin;
} onsText_t;

static onsText_t WorldText[MAX_WORLDTEXT];
static onsText_t * freeworldtext;           // List of world text
static onsText_t * activeworldtext;         // List of world text

void CG_InitWorldText( void ) {
	int i;

	memset( &WorldText, 0, sizeof( WorldText ) );
	for ( i = 0; i < MAX_WORLDTEXT - 1; i++ ) {
		WorldText[i].next = &WorldText[i + 1];
	}

	freeworldtext = &WorldText[0];
	activeworldtext = NULL;
}

void CG_ClearWorldText( void ) {
	int i;

	for ( i = 0; i < MAX_WORLDTEXT - 1; i++ ) {
		WorldText[i].endtime = cg.time;
	}

	activeworldtext = NULL;
}

/*
================
CG_WorldToScreen
================
*/
qboolean CG_WorldToScreen( vec3_t point, float *x, float *y ) {
	vec3_t trans;
	float xc, yc;
	float px, py;
	float z;

	px = tan( cg.refdef.fov_x * M_PI / 360.0 );
	py = tan( cg.refdef.fov_y * M_PI / 360.0 );

	VectorSubtract( point, cg.refdef.vieworg, trans );

	xc = 640.0f / 2.0f;
	yc = 480.0f / 2.0f;

	z = DotProduct( trans, cg.refdef.viewaxis[0] );
	if ( z <= 0.001f ) {
		return qfalse;
	}

	if ( x ) {
		*x = xc - DotProduct( trans, cg.refdef.viewaxis[1] ) * xc / ( z * px );
	}

	if ( y ) {
		*y = yc - DotProduct( trans, cg.refdef.viewaxis[2] ) * yc / ( z * py );
	}

	return qtrue;
}

qboolean CG_AddOnScreenText( const char *text, vec3_t origin, vec4_t color, float duration ) {
	onsText_t *worldtext = freeworldtext;
	if ( !worldtext ) {
		return qfalse;
	}

	freeworldtext = worldtext->next;
	worldtext->next = activeworldtext;
	activeworldtext = worldtext;

	VectorCopy( origin, worldtext->origin );
	worldtext->endtime = cg.time + (int)( (float)duration * 1000.f );
	VectorCopy4( color, worldtext->color );
	Q_strncpyz( worldtext->text,text,MAX_TEXTLENGTH );
	return qtrue;
}

void CG_DrawOnScreenText( void ) {
	onsText_t *worldtext;
	onsText_t * * whereworldtext;
	const float fTxtScale = 0.25f;
	float x,y;

	/* Render/Move the world text */
	worldtext = activeworldtext;
	whereworldtext = &activeworldtext;

	while ( worldtext )
	{
		/* Check for expiration */
		if ( worldtext->endtime < cg.time ) {
			/* Clear up this world text */
			*whereworldtext = worldtext->next;
			worldtext->next = freeworldtext;
			freeworldtext = worldtext;
			worldtext = *whereworldtext;
			continue;
		}

		if ( CG_WorldToScreen( worldtext->origin, &x, &y ) && PointVisible( worldtext->origin ) && DistanceSquared( cg.refdef.vieworg, worldtext->origin ) < MAX_RENDERDIST * MAX_RENDERDIST ) {
			const char *tokens = "\n";
			const char *tok = 0;
			char temp[1024];
			int heightOffset = 0;
			fontInfoExtra_t *font = &cgDC.Assets.bigFont;

			Q_strncpyz( temp,worldtext->text,1024 );
			tok = (const char *)strtok( temp,tokens );
			while ( tok )
			{
				const int width = CG_Text_Width_Ext( tok,fTxtScale,0, font );
				const int height = CG_Text_Height_Ext( tok,fTxtScale,0, font );

				CG_Text_Paint_Ext(
					x - ( width * 0.5 ),
					y + heightOffset,
					fTxtScale,
					fTxtScale,
					worldtext->color,
					tok,
					0, 0,
					ITEM_TEXTSTYLE_NORMAL,
					font );

				heightOffset += height * 1.5;
				tok = (const char *)strtok( NULL,tokens );
			}
		}

		trap_R_SetColor( NULL );

		whereworldtext = &worldtext->next;
		worldtext = worldtext->next;
	}
}

void CG_DrawAnnouncer( void ) {
	int x, y, w, h;
	float scale, fade;
	vec4_t color;


	if (  cg.centerPrintAnnouncerTime <= cg.time ) {
		return;
	}

	fade = (float)( cg.centerPrintAnnouncerTime - cg.time ) / cg.centerPrintAnnouncerDuration;

	color[0] = cg.centerPrintAnnouncerColor[0];
	color[1] = cg.centerPrintAnnouncerColor[1];
	color[2] = cg.centerPrintAnnouncerColor[2];

	switch ( cg.centerPrintAnnouncerMode ) {
	default:
	case ANNOUNCER_NORMAL:
		color[3] = fade;
		break;
	case ANNOUNCER_SINE:
		color[3] = sin( M_PI * fade );
		break;
	case ANNOUNCER_INVERSE_SINE:
		color[3] = 1 - sin( M_PI * fade );
		break;
	case ANNOUNCER_TAN:
		color[3] = tan( M_PI * fade );
	}

	scale = ( 1.1f - color[3] ) * cg.centerPrintAnnouncerScale;

	h = CG_Text_Height_Ext( cg.centerPrintAnnouncer, scale, 0, &cgDC.Assets.textFont );

	y = ( SCREEN_HEIGHT - h ) / 2;

	w = CG_Text_Width_Ext( cg.centerPrintAnnouncer, scale, 0, &cgDC.Assets.textFont );

	x = ( SCREEN_WIDTH - w ) / 2;

	CG_Text_Paint_Ext( x, y, scale, scale, color, cg.centerPrintAnnouncer, 0, 0, 0, &cgDC.Assets.textFont );
}

void CG_AddAnnouncer( char *text, sfxHandle_t sound, float scale, int duration, float r, float g, float b, int mode ) {

//	if ( !cg_announcer.integer )
//		return;

	if ( sound ) {
		trap_S_StartLocalSound( sound, CHAN_ANNOUNCER );
	}

	if ( text ) {
		cg.centerPrintAnnouncer = text;
		cg.centerPrintAnnouncerTime = cg.time + duration;
		cg.centerPrintAnnouncerScale = scale;
		cg.centerPrintAnnouncerDuration = duration;
		cg.centerPrintAnnouncerColor[0] = r;
		cg.centerPrintAnnouncerColor[1] = g;
		cg.centerPrintAnnouncerColor[2] = b;
		cg.centerPrintAnnouncerMode = mode;
	}
}

