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

/*
 * name:		bg_misc.c
 *
 * desc:		both games misc functions, all completely stateless
 *
*/


#include "../qcommon/q_shared.h"
#include "bg_public.h"


#ifdef CGAMEDLL
extern vmCvar_t cg_gameType;
#endif
#ifdef GAMEDLL
extern vmCvar_t g_gametype;
#endif


// these defines are matched with the character torso animations
#define DELAY_LOW       100 // machineguns, tesla, spear, flame
#define DELAY_HIGH      100 // mauser, snooper
#define DELAY_PISTOL    100 // colt, luger, sp5, cross
#define DELAY_SHOULDER  50  // rl
#define DELAY_THROW     250 // grenades, dynamite

int weapBanks[MAX_WEAP_BANKS][MAX_WEAPS_IN_BANK] = {
	// bank
	{0,                     0,                      0           },  //	0 (empty)

	{WP_KNIFE,              0,                      0           },  //	1
	{WP_LUGER,              WP_SILENCER,            WP_COLT,     WP_AKIMBO,   WP_TT33,     WP_DUAL_TT33, WP_REVOLVER, WP_HDM },  //	2
	{WP_MP40,               WP_THOMPSON,            WP_STEN,     WP_MP34,     WP_PPSH },  //	3
	{WP_MAUSER,             WP_SNOOPER,             WP_M1GARAND, WP_G43,     WP_MOSIN,    WP_M1941,    WP_DELISLE },  //	4
	{WP_FG42,               WP_BAR,                 WP_MP44     },  //	5
	{WP_GRENADE_LAUNCHER,   WP_GRENADE_PINEAPPLE,   WP_DYNAMITE, WP_LANDMINE, WP_MEDCRATE, WP_AIRSTRIKE },  //	6
	{WP_PANZERFAUST,        WP_FLAMETHROWER,        0           },  //	7
	{WP_TESLA,              WP_VENOM,               0           },  //	8
	{WP_M97,                WP_AUTO5,               0           },  //	9
	{WP_MG42M,              WP_BROWNING,            0           }   //	10
};

extern int weapBanks[MAX_WEAP_BANKS][MAX_WEAPS_IN_BANK];

ammotable_t ammoTable[] = {
    [WP_NONE] = {
		.weaponindex        = WP_NONE,
		.weapAlts           = WP_NONE,
		.weaponClass        = WEAPON_CLASS_NONE,
        .maxammo            = 0,
        .uses               = 0,
        .maxclip            = 0,
        .reloadTime         = 0,
        .fireDelayTime      = 50,
        .nextShotTime       = 0,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 0,
		.weaponSpread       = 0,
		.spreadScale        = 0.0f,
		.spreadScaleAdd     = 0,
        .weapRecoilDuration = 0,
        .weapRecoilPitch    = { 0.0f, 0.0f },
        .weapRecoilYaw      = { 0.0f, 0.0f },
		.soundRange         = 0,
		.aiRange            = 0,
        .mod                = 0,
		.rndTriggerRelease  = qfalse,
	    .moveSpeed          = 1.0f,
	    .iconDrawSize       = WEAPON_ICON_HIDE,
		.bulletBased        = qfalse,
		.hasMuzzle          = qfalse,
		.underwaterFire     = qfalse,
    },
    [WP_KNIFE] = {
		.weaponindex        = WP_KNIFE,
		.weapAlts           = WP_NONE,
		.weaponClass        = WEAPON_CLASS_MELEE,
        .maxammo            = 999,
        .uses               = 0,
        .maxclip            = 999,
        .reloadTime         = 0,
        .fireDelayTime      = 0,
        .nextShotTime       = 300,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 10,
		.weaponSpread       = 0,
		.spreadScale        = 0.0f,
		.spreadScaleAdd     = 0,
        .weapRecoilDuration = 0,
        .weapRecoilPitch    = { 0.0f, 0.0f },
        .weapRecoilYaw      = { 0.0f, 0.0f },
		.soundRange         = 64,
		.aiRange            = AI_WEAPON_RANGE_MELEE,
		.moveSpeed          = 1.0f,
        .mod                = 0,
		.rndTriggerRelease  = qfalse,
	    .iconDrawSize       = WEAPON_ICON_NORMAL,
		.bulletBased        = qfalse,
		.hasMuzzle          = qfalse,
		.underwaterFire     = qtrue,
		.weapFile           = "knife.weap",
    },
    [WP_LUGER] = {
		.weaponindex        = WP_LUGER,
		.weapAlts           = WP_NONE,
		.weaponClass        = WEAPON_CLASS_PISTOL,
        .maxammo            = 64,
        .uses               = 1,
        .maxclip            = 8,
        .reloadTime         = 1850,
        .reloadTimeFull     = 2650,
        .fireDelayTime      = 0,
        .nextShotTime       = 300,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 6,
		.weaponSpread       = 400,
		.spreadScale        = 0.3f,	
		.spreadScaleAdd     = 35,	
        .weapRecoilDuration = 50,
        .weapRecoilPitch    = { 0.2f, 0.1f },
        .weapRecoilYaw      = { 0.0f, 0.0f },
		.soundRange         = 700,
		.aiRange            = AI_WEAPON_RANGE_NORMAL,
        .moveSpeed          = 1.0f,
        .mod                = MOD_LUGER,
		.rndTriggerRelease  = qfalse,
	    .iconDrawSize       = WEAPON_ICON_NORMAL,
		.bulletBased        = qtrue,
		.hasMuzzle          = qtrue,
		.underwaterFire     = qfalse,
		.gunOffset          = { 0.0f, 0.0f, 0.0f },
		.weapFile           = "luger.weap",
    },
    [WP_SILENCER] = {
		.weaponindex        = WP_SILENCER,
		.weapAlts           = WP_NONE,
		.weaponClass        = WEAPON_CLASS_PISTOL,	
        .maxammo            = 64,
        .uses               = 1,
        .maxclip            = 8,
        .reloadTime         = 1850,
        .reloadTimeFull     = 2650,
        .fireDelayTime      = 0,
        .nextShotTime       = 300,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 6,
		.weaponSpread       = 300,
		.spreadScale        = 0.3f,
		.spreadScaleAdd     = 35,			
        .weapRecoilDuration = 50,
        .weapRecoilPitch    = { 0.2f, 0.1f },
        .weapRecoilYaw      = { 0.0f, 0.0f },
		.soundRange         = 64,
		.aiRange            = AI_WEAPON_RANGE_NORMAL,
        .moveSpeed          = 1.0f,
        .mod                = MOD_SILENCER,
		.rndTriggerRelease  = qfalse,
	    .iconDrawSize       = WEAPON_ICON_NORMAL,
		.bulletBased        = qtrue,
		.hasMuzzle          = qfalse,
		.underwaterFire     = qfalse,
		.gunOffset          = { 0.0f, 0.0f, 0.0f },
		.weapFile           = "luger_silenced.weap",
    },
    [WP_COLT] = {
		.weaponindex        = WP_COLT,
		.weapAlts           = WP_NONE,
		.weaponClass        = WEAPON_CLASS_PISTOL,
        .maxammo            = 56,
        .uses               = 1,
        .maxclip            = 7,
        .reloadTime         = 1650,
        .fireDelayTime      = 0,
        .nextShotTime       = 300,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 9,
		.weaponSpread       = 400,
		.spreadScale        = 0.4f,
		.spreadScaleAdd     = 35,	
        .weapRecoilDuration = 50,
        .weapRecoilPitch    = { 0.2f, 0.1f },
        .weapRecoilYaw      = { 0.0f, 0.0f },
		.soundRange         = 700,
		.aiRange            = AI_WEAPON_RANGE_NORMAL,
        .moveSpeed          = 1.0f,
        .mod                = MOD_COLT,
		.rndTriggerRelease  = qfalse,
	    .iconDrawSize       = WEAPON_ICON_NORMAL,	
		.bulletBased        = qtrue,
		.hasMuzzle          = qtrue,
		.underwaterFire     = qfalse,
		.gunOffset          = { 0.0f, 0.0f, -1.0f },
		.weapFile           = "colt.weap",
    },
    [WP_AKIMBO] = {
		.weaponindex        = WP_AKIMBO,
		.weapAlts           = WP_NONE,
		.weaponClass        = WEAPON_CLASS_PISTOL | WEAPON_CLASS_AKIMBO,
        .maxammo            = 112,
        .uses               = 1,
        .maxclip            = 14,
        .reloadTime         = 2000,
        .fireDelayTime      = 0,
        .nextShotTime       = 220,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 9,
		.weaponSpread       = 500,
		.spreadScale        = 0.5f,	
		.spreadScaleAdd     = 35,		
        .weapRecoilDuration = 50,
        .weapRecoilPitch    = { 0.2f, 0.1f },
        .weapRecoilYaw      = { 0.0f, 0.0f },
		.soundRange         = 700,
		.aiRange            = AI_WEAPON_RANGE_NORMAL,
        .moveSpeed          = 1.0f,
        .mod                = MOD_AKIMBO,
		.rndTriggerRelease  = qfalse,
	    .iconDrawSize       = WEAPON_ICON_NORMAL,	
		.bulletBased        = qtrue,
		.hasMuzzle          = qtrue,
		.underwaterFire     = qfalse,
		.gunOffset          = { 0.0f, 0.0f, 0.0f },
		.weapFile           = "akimbo.weap",
    },
    [WP_MP40] = {
		.weaponindex        = WP_MP40,		
		.weapAlts           = WP_NONE,
		.weaponClass        = WEAPON_CLASS_SMG,
        .maxammo            = 192,
        .uses               = 1,
        .maxclip            = 32,
        .reloadTime         = 2500,
        .fireDelayTime      = 0,
        .nextShotTime       = 110,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 6,
		.weaponSpread       = 850,
		.spreadScale        = 0.5f,
		.spreadScaleAdd     = 15,			
        .weapRecoilDuration = 30,
        .weapRecoilPitch    = { 0.1f, 0.1f },
        .weapRecoilYaw      = { 0.0f, 0.0f },
		.soundRange         = 1000,
		.aiRange            = AI_WEAPON_RANGE_NORMAL,
        .moveSpeed          = 0.95f,
        .mod                = MOD_MP40,
		.rndTriggerRelease  = qtrue,
	    .iconDrawSize       = WEAPON_ICON_WIDE_OFFSET,
		.bulletBased        = qtrue,
		.hasMuzzle          = qtrue,
		.underwaterFire     = qfalse,
		.gunOffset          = { -3.0f, 2.0f, 0.0f },
		.weapFile           = "mp40.weap",
    },
    [WP_THOMPSON] = {
		.weaponindex        = WP_THOMPSON,		
		.weapAlts           = WP_NONE,
		.weaponClass        = WEAPON_CLASS_SMG,
        .maxammo            = 200,
        .uses               = 1,
        .maxclip            = 30,
        .reloadTime         = 2400,
		.reloadTimeFull     = 2400,
        .fireDelayTime      = 0,
        .nextShotTime       = 90,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 9,
		.weaponSpread       = 950,
		.spreadScale        = 0.4f,	
		.spreadScaleAdd     = 15,
        .weapRecoilDuration = 30,
        .weapRecoilPitch    = { 0.2f, 0.2f },
        .weapRecoilYaw      = { 0.0f, 0.0f },
		.soundRange         = 1000,
		.aiRange            = AI_WEAPON_RANGE_NORMAL,
        .moveSpeed          = 0.95f,
        .mod                = MOD_THOMPSON,
		.rndTriggerRelease  = qtrue,
	    .iconDrawSize       = WEAPON_ICON_WIDE_OFFSET,
		.bulletBased        = qtrue,
		.hasMuzzle          = qtrue,
		.underwaterFire     = qfalse,
		.gunOffset          = { -2.0f, 0.0f, -1.0f },
		.weapFile           = "thompson.weap",
    },
    [WP_STEN] = {
		.weaponindex        = WP_STEN,
		.weapAlts           = WP_NONE,
		.weaponClass        = WEAPON_CLASS_SMG,		
        .maxammo            = 220,
        .uses               = 1,
        .maxclip            = 32,
        .reloadTime         = 2500,
        .fireDelayTime      = 0,
        .nextShotTime       = 115,
        .maxHeat            = 900,
        .coolRate           = 500,
		.weaponDamage       = 7,
		.weaponSpread       = 900,
		.spreadScale        = 0.6f,		
		.spreadScaleAdd     = 15,	
        .weapRecoilDuration = 40,
        .weapRecoilPitch    = { 0.1f, 0.1f },
        .weapRecoilYaw      = { 0.0f, 0.0f },
		.soundRange         = 64,
		.aiRange            = AI_WEAPON_RANGE_NORMAL,
        .moveSpeed          = 0.95f,
        .mod                = MOD_STEN,
		.rndTriggerRelease  = qfalse,
	    .iconDrawSize       = WEAPON_ICON_WIDE_OFFSET,	
		.bulletBased        = qtrue,
		.hasMuzzle          = qfalse,
		.underwaterFire     = qfalse,
		.gunOffset          = { -6.0f, -1.0f, -2.0f },
		.weapFile           = "sten.weap",
    },
    [WP_MP34] = {
		.weaponindex        = WP_MP34,
		.weapAlts           = WP_NONE,
		.weaponClass        = WEAPON_CLASS_SMG,
        .maxammo            = 192,
        .uses               = 1,
        .maxclip            = 32,
        .reloadTime         = 2620,
        .fireDelayTime      = 0,
        .nextShotTime       = 105,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 6,
		.weaponSpread       = 850,
		.spreadScale        = 0.5f,
		.spreadScaleAdd     = 15,			
        .weapRecoilDuration = 30,
        .weapRecoilPitch    = { 0.1f, 0.1f },
        .weapRecoilYaw      = { 0.0f, 0.0f },
		.soundRange         = 1000,
		.aiRange            = AI_WEAPON_RANGE_NORMAL,
        .moveSpeed          = 0.95f,
        .mod                = MOD_MP34,
		.rndTriggerRelease  = qtrue,
	    .iconDrawSize       = WEAPON_ICON_WIDE_OFFSET,
		.bulletBased        = qtrue,
		.hasMuzzle          = qtrue,
		.underwaterFire     = qfalse,
		.gunOffset          = { -12.0f, -1.0f, 0.0f },
		.weapFile           = "mp34.weap",
    },
    [WP_MAUSER] = {
		.weaponindex        = WP_MAUSER,
		.weapAlts           = WP_SNIPERRIFLE,
		.weaponClass        = WEAPON_CLASS_RIFLE_BOLTACTION | WEAPON_CLASS_SCOPABLE,
        .maxammo            = 50,
        .uses               = 1,
        .maxclip            = 5,
        .reloadTime         = 3000,
        .fireDelayTime      = 0,
        .nextShotTime       = 1300,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 25,
		.weaponSpread       = 400,
		.spreadScale        = 0.5f,	
		.spreadScaleAdd     = 50,		
        .weapRecoilDuration = 60,
        .weapRecoilPitch    = { 1.0f, 1.0f },
        .weapRecoilYaw      = { 0.1f, 0.1f },
		.soundRange         = 2000,
		.aiRange            = AI_WEAPON_RANGE_LONG,
        .moveSpeed          = 0.95f,
        .mod                = MOD_MAUSER,
		.rndTriggerRelease  = qfalse,
	    .iconDrawSize       = WEAPON_ICON_WIDE_OFFSET,
		.bulletBased        = qtrue,
		.hasMuzzle          = qtrue,
		.underwaterFire     = qfalse,
		.gunOffset          = { 0.0f, 0.0f, -1.0f },
		.weapFile           = "mauser.weap",
    },
    [WP_SNIPERRIFLE] = {
		.weaponindex        = WP_SNIPERRIFLE,
		.weapAlts           = WP_MAUSER,
		.weaponClass        = WEAPON_CLASS_SCOPED | WEAPON_CLASS_RIFLE_BOLTACTION,
        .maxammo            = 50,
        .uses               = 1,
        .maxclip            = 5,
        .reloadTime         = 3000,
        .fireDelayTime      = 0,
        .nextShotTime       = 1400,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 25,
		.weaponSpread       = 50,
		.spreadScale        = 10.0f,
		.spreadScaleAdd     = 10,	
        .weapRecoilDuration = 300,
        .weapRecoilPitch    = { 0.4f, 0.0f },
        .weapRecoilYaw      = { 0.2f, 0.0f },
		.soundRange         = 2000,
		.aiRange            = AI_WEAPON_RANGE_LONG,
        .moveSpeed          = 0.55f,
        .mod                = MOD_SNIPERRIFLE,
		.rndTriggerRelease  = qfalse,
	    .iconDrawSize       = WEAPON_ICON_WIDE,
		.bulletBased        = qtrue,
		.hasMuzzle          = qtrue,
		.underwaterFire     = qfalse,
		.weapFile           = "sniperrifle.weap",
    },
    [WP_SNOOPER] = {
		.weaponindex        = WP_SNOOPER,
		.weapAlts           = WP_SNOOPERSCOPE,
		.weaponClass        = WEAPON_CLASS_RIFLE_AUTO | WEAPON_CLASS_SCOPABLE,
        .maxammo            = 30,
        .uses               = 1,
        .maxclip            = 5,
        .reloadTime         = 3000,
        .fireDelayTime      = 0,
        .nextShotTime       = 360,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 35,
		.weaponSpread       = 400,
		.spreadScale        = 0.5f,
		.spreadScaleAdd     = 50,			
        .weapRecoilDuration = 0,
        .weapRecoilPitch    = { 1.0f, 1.0f },
        .weapRecoilYaw      = { 0.1f, 0.1f },
		.soundRange         = 128,
		.aiRange            = AI_WEAPON_RANGE_LONG,
        .moveSpeed          = 0.95f,
        .mod                = MOD_SNOOPER,
		.rndTriggerRelease  = qfalse,
	    .iconDrawSize       = WEAPON_ICON_WIDE_OFFSET,
		.bulletBased        = qtrue,
		.hasMuzzle          = qfalse,
		.underwaterFire     = qfalse,
		.gunOffset          = { -5.0f, -1.0f, -2.0f },
		.weapFile           = "snooper.weap",
    },
    [WP_SNOOPERSCOPE] = {
		.weaponindex        = WP_SNOOPERSCOPE,
		.weapAlts           = WP_SNOOPER,
		.weaponClass        = WEAPON_CLASS_SCOPED,
        .maxammo            = 30,
        .uses               = 1,
        .maxclip            = 5,
        .reloadTime         = 3000,
        .fireDelayTime      = 0,
        .nextShotTime       = 360,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 35,
		.weaponSpread       = 50,
		.spreadScale        = 8.0f,
		.spreadScaleAdd     = 10,
        .weapRecoilDuration = 300,
        .weapRecoilPitch    = { 0.7f, 0.0f },
        .weapRecoilYaw      = { 0.4f, 0.0f },
		.soundRange         = 128,
		.aiRange            = AI_WEAPON_RANGE_LONG,
        .moveSpeed          = 0.55f,
        .mod                = MOD_SNOOPERSCOPE,
		.rndTriggerRelease  = qfalse,
	    .iconDrawSize       = WEAPON_ICON_WIDE,
		.bulletBased        = qtrue,
		.hasMuzzle          = qfalse,
		.underwaterFire     = qfalse,
		.weapFile           = "snooperscope.weap",
    },
    [WP_M1GARAND] = {
		.weaponindex        = WP_M1GARAND,
		.weapAlts           = WP_M7,
		.weaponClass        = WEAPON_CLASS_RIFLE_AUTO,
        .maxammo            = 200,
        .uses               = 1,
        .maxclip            = 8,
        .reloadTime         = 1650,
        .fireDelayTime      = 0,
        .nextShotTime       = 300,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 20,
		.weaponSpread       = 350,
		.spreadScale        = 0.4f,
		.spreadScaleAdd     = 40,
        .weapRecoilDuration = 40,
        .weapRecoilPitch    = { 0.2f, 0.2f },
        .weapRecoilYaw      = { 0.1f, 0.1f },
		.soundRange         = 2000,
		.aiRange            = AI_WEAPON_RANGE_LONG,
        .moveSpeed          = 0.95f,
        .mod                = MOD_M1GARAND,
		.rndTriggerRelease  = qfalse,
	    .iconDrawSize       = WEAPON_ICON_WIDE_OFFSET,
		.bulletBased        = qtrue,
		.hasMuzzle          = qtrue,
		.underwaterFire     = qfalse,
		.gunOffset          = { 0.0f, 0.0f, -2.0f },
		.weapFile           = "m1garand.weap",
    },
    [WP_M7] = {
		.weaponindex        = WP_M7,
		.weapAlts           = WP_M1GARAND,
		.weaponClass        = WEAPON_CLASS_RIFLENADE,
        .maxammo            = 10,
        .uses               = 1,
        .maxclip            = 1,
        .reloadTime         = 3000,
        .fireDelayTime      = 0,
        .nextShotTime       = 400,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 220,
		.weaponSpread       = 0,
		.spreadScale        = 0.0f,
		.spreadScaleAdd     = 0,
        .weapRecoilDuration = 0,
        .weapRecoilPitch    = { 0.0f, 0.0f },
        .weapRecoilYaw      = { 0.0f, 0.0f },
		.soundRange         = 1500,
		.aiRange            = AI_WEAPON_RANGE_NORMAL,
        .moveSpeed          = 0.90f,
        .mod                = MOD_M7,
		.rndTriggerRelease  = qfalse,
	    .iconDrawSize       = WEAPON_ICON_WIDE_OFFSET,
		.bulletBased        = qfalse,
		.hasMuzzle          = qfalse,
		.underwaterFire     = qfalse,
		.gunOffset          = { 0.0f, 0.0f, -2.0f },
		.weapFile           = "m7.weap",
    },
    [WP_FG42] = {
		.weaponindex        = WP_FG42,
		.weapAlts           = WP_FG42SCOPE,
		.weaponClass        = WEAPON_CLASS_ASSAULT_RIFLE | WEAPON_CLASS_SCOPABLE,
        .maxammo            = 150,
        .uses               = 1,
        .maxclip            = 20,
        .reloadTime         = 2400,
        .fireDelayTime      = 0,
        .nextShotTime       = 120,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 12,
		.weaponSpread       = 600,
		.spreadScale        = 0.7f,
		.spreadScaleAdd     = 15,			
        .weapRecoilDuration = 40,
        .weapRecoilPitch    = { 0.1f, 0.1f },
        .weapRecoilYaw      = { 0.0f, 0.0f },
		.soundRange         = 1500,
		.aiRange            = AI_WEAPON_RANGE_NORMAL,
        .moveSpeed          = 0.95f,
        .mod                = MOD_FG42,
		.rndTriggerRelease  = qtrue,
	    .iconDrawSize       = WEAPON_ICON_WIDE,
		.bulletBased        = qtrue,
		.hasMuzzle          = qtrue,
		.underwaterFire     = qfalse,
		.gunOffset          = { 0.0f, 0.0f, -1.0f },
		.fireModeSwitchTime = 250,
		.weapFile           = "fg42.weap",
    },
    [WP_BAR] = {
		.weaponindex        = WP_BAR,
		.weapAlts           = WP_NONE,
		.weaponClass        = WEAPON_CLASS_ASSAULT_RIFLE,
        .maxammo            = 300,
        .uses               = 1,
        .maxclip            = 20,
        .reloadTime         = 3300,
        .reloadTimeFull     = 3300,
        .fireDelayTime      = 0,
        .nextShotTime       = 200,
        .nextShotTime2      = 100,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 15,
		.weaponSpread       = 700,
		.spreadScale        = 0.6f,
		.spreadScaleAdd     = 15,
        .weapRecoilDuration = 40,
        .weapRecoilPitch    = { 0.1f, 0.1f },
        .weapRecoilYaw      = { 0.0f, 0.0f },
		.soundRange         = 1500,
		.aiRange            = AI_WEAPON_RANGE_NORMAL,
        .moveSpeed          = 0.90f,
        .mod                = MOD_BAR,
		.rndTriggerRelease  = qtrue,
	    .iconDrawSize       = WEAPON_ICON_WIDE,
		.bulletBased        = qtrue,
		.hasMuzzle          = qtrue,
		.underwaterFire     = qfalse,
		.fireModeSwitchTime = 250,
		.fireModeIsRateSwitch = qtrue,
		.gunOffset          = { -2.0f, -1.0f, -1.0f },
		.weapFile           = "bar.weap",
    },
    [WP_FG42SCOPE] = {
		.weaponindex        = WP_FG42SCOPE,
		.weapAlts           = WP_FG42,
		.weaponClass        = WEAPON_CLASS_SCOPED | WEAPON_CLASS_ASSAULT_RIFLE,
        .maxammo            = 120,
        .uses               = 1,
        .maxclip            = 20,
        .reloadTime         = 2400,
        .fireDelayTime      = 0,
        .nextShotTime       = 150,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 12,
		.weaponSpread       = 50,
		.spreadScale        = 7.0f,
		.spreadScaleAdd     = 10,			
        .weapRecoilDuration = 40,
        .weapRecoilPitch    = { 0.2f, 0.0f },
        .weapRecoilYaw      = { 0.1f, 0.0f },
		.soundRange         = 1500,
		.aiRange            = AI_WEAPON_RANGE_NORMAL,
        .moveSpeed          = 0.55f,
        .mod                = MOD_FG42SCOPE,
		.rndTriggerRelease  = qtrue,
	    .iconDrawSize       = WEAPON_ICON_WIDE,
		.bulletBased        = qtrue,
		.hasMuzzle          = qtrue,
		.underwaterFire     = qfalse,
		.fireModeSwitchTime = 250,
		.weapFile           = "fg42scope.weap",
    },
    [WP_MP44] = {
		.weaponindex        = WP_MP44,
		.weapAlts           = WP_NONE,
		.weaponClass        = WEAPON_CLASS_ASSAULT_RIFLE,
        .maxammo            = 200,
        .uses               = 1,
        .maxclip            = 30,
        .reloadTime         = 2600,
        .fireDelayTime      = 0,
        .nextShotTime       = 115,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 9,
		.weaponSpread       = 800,
		.spreadScale        = 0.6f,
		.spreadScaleAdd     = 15,
        .weapRecoilDuration = 40,
        .weapRecoilPitch    = { 0.1f, 0.1f },
        .weapRecoilYaw      = { 0.0f, 0.0f },
		.soundRange         = 1500,
		.aiRange            = AI_WEAPON_RANGE_NORMAL,
        .moveSpeed          = 0.90f,
        .mod                = MOD_MP44,
		.rndTriggerRelease  = qtrue,
	    .iconDrawSize       = WEAPON_ICON_WIDE,
		.bulletBased        = qtrue,
		.hasMuzzle          = qtrue,
		.underwaterFire     = qfalse,
		.gunOffset          = { -8.0f, -1.0f, 0.0f },
		.fireModeSwitchTime = 250,
		.weapFile           = "mp44.weap",
    },
    [WP_M97] = {
		.weaponindex        = WP_M97,
		.weapAlts           = WP_NONE,
		.weaponClass        = WEAPON_CLASS_SHOTGUN,
        .maxammo            = 100,
        .uses               = 1,
        .maxclip            = 7,
        .reloadTime         = 2300,
        .reloadTimeFull     = 2300,
        .fireDelayTime      = 0,
        .nextShotTime       = 800,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 10,
		.weaponSpread       = 4500,
		.spreadScale        = 0.6f,
		.spreadScaleAdd     = 15,
        .weapRecoilDuration = 100,
        .weapRecoilPitch    = { 0.1f, 0.2f },
        .weapRecoilYaw      = { 0.5f, 0.5f },
		.soundRange         = 1500,
		.aiRange            = AI_WEAPON_RANGE_SHOTGUN,
        .moveSpeed          = 0.90f,
        .mod                = MOD_SHOTGUN,
		.rndTriggerRelease  = qfalse,
	    .iconDrawSize       = WEAPON_ICON_WIDE,
		.bulletBased        = qtrue,
		.hasMuzzle          = qtrue,
		.underwaterFire     = qfalse,
		.gunOffset          = { -3.0f, 0.0f, -1.0f },
		.shotgunReloadStart = 300,
		.shotgunReloadLoop  = 800,
		.shotgunReloadEnd   = 450,
		.shotgunPumpStart   = 1700,
		.shotgunPumpLoop    = 250,
		.shotgunPumpEnd     = 275,
		.weapFile           = "ithaca.weap",
    },
    [WP_REVOLVER] = {
		.weaponindex        = WP_REVOLVER,
		.weapAlts           = WP_NONE,
		.weaponClass        = WEAPON_CLASS_PISTOL,
        .maxammo            = 60,
        .uses               = 1,
        .maxclip            = 6,
        .reloadTime         = 3600,
        .reloadTimeFull     = 3600,
        .fireDelayTime      = 0,
        .nextShotTime       = 380,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 18,
		.weaponSpread       = 300,
		.spreadScale        = 0.4f,
		.spreadScaleAdd     = 35,
        .weapRecoilDuration = 50,
        .weapRecoilPitch    = { 0.3f, 0.1f },
        .weapRecoilYaw      = { 0.0f, 0.0f },
		.soundRange         = 1000,
		.aiRange            = AI_WEAPON_RANGE_NORMAL,
        .moveSpeed          = 1.0f,
        .mod                = MOD_REVOLVER,
		.rndTriggerRelease  = qfalse,
	    .iconDrawSize       = WEAPON_ICON_NORMAL,
		.bulletBased        = qtrue,
		.hasMuzzle          = qtrue,
		.underwaterFire     = qfalse,
		.gunOffset          = { 0.0f, 0.0f, 0.0f },
		.weapFile           = "revolver.weap",
    },
    [WP_HDM] = {
		.weaponindex        = WP_HDM,
		.weapAlts           = WP_NONE,
		.weaponClass        = WEAPON_CLASS_PISTOL,
        .maxammo            = 200,
        .uses               = 1,
        .maxclip            = 10,
        .reloadTime         = 2300,
        .reloadTimeFull     = 2300,
        .fireDelayTime      = 0,
        .nextShotTime       = 300,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 8,
		.weaponSpread       = 250,
		.spreadScale        = 0.4f,
		.spreadScaleAdd     = 35,
        .weapRecoilDuration = 50,
        .weapRecoilPitch    = { 0.2f, 0.1f },
        .weapRecoilYaw      = { 0.0f, 0.0f },
		.soundRange         = 64,
		.aiRange            = AI_WEAPON_RANGE_NORMAL,
        .moveSpeed          = 1.0f,
        .mod                = MOD_HDM,
		.rndTriggerRelease  = qfalse,
	    .iconDrawSize       = WEAPON_ICON_NORMAL,
		.bulletBased        = qtrue,
		.hasMuzzle          = qfalse,
		.underwaterFire     = qfalse,
		.gunOffset          = { 0.0f, 0.0f, -1.0f },
		.weapFile           = "hdm.weap",
    },
    [WP_DELISLE] = {
		.weaponindex        = WP_DELISLE,
		.weapAlts           = WP_DELISLESCOPE,
		.weaponClass        = WEAPON_CLASS_RIFLE_BOLTACTION | WEAPON_CLASS_SCOPABLE,
        .maxammo            = 150,
        .uses               = 1,
        .maxclip            = 7,
        .reloadTime         = 2100,
        .reloadTimeFull     = 3350,
        .fireDelayTime      = 0,
        .nextShotTime       = 1200,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 30,
		.weaponSpread       = 250,
		.spreadScale        = 0.5f,
		.spreadScaleAdd     = 50,
        .weapRecoilDuration = 60,
        .weapRecoilPitch    = { 1.0f, 1.0f },
        .weapRecoilYaw      = { 0.1f, 0.1f },
		.soundRange         = 32,
		.aiRange            = AI_WEAPON_RANGE_LONG,
        .moveSpeed          = 0.95f,
        .mod                = MOD_DELISLE,
		.rndTriggerRelease  = qfalse,
	    .iconDrawSize       = WEAPON_ICON_WIDE_OFFSET,
		.bulletBased        = qtrue,
		.hasMuzzle          = qfalse,
		.underwaterFire     = qfalse,
		.gunOffset          = { 1.0f, -1.0f, -2.0f },
		.weapFile           = "delisle.weap",
    },
    [WP_DELISLESCOPE] = {
		.weaponindex        = WP_DELISLESCOPE,
		.weapAlts           = WP_DELISLE,
		.weaponClass        = WEAPON_CLASS_SCOPED | WEAPON_CLASS_RIFLE_BOLTACTION,
        .maxammo            = 150,
        .uses               = 1,
        .maxclip            = 7,
        .reloadTime         = 2100,
        .reloadTimeFull     = 3350,
        .fireDelayTime      = 0,
        .nextShotTime       = 1200,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 30,
		.weaponSpread       = 50,
		.spreadScale        = 0.5f,
		.spreadScaleAdd     = 50,
        .weapRecoilDuration = 300,
        .weapRecoilPitch    = { 0.7f, 0.0f },
        .weapRecoilYaw      = { 0.5f, 0.0f },
		.soundRange         = 64,
		.aiRange            = AI_WEAPON_RANGE_LONG,
        .moveSpeed          = 0.55f,
        .mod                = MOD_DELISLESCOPE,
		.rndTriggerRelease  = qfalse,
	    .iconDrawSize       = WEAPON_ICON_WIDE,
		.bulletBased        = qtrue,
		.hasMuzzle          = qfalse,
		.underwaterFire     = qfalse,
		.weapFile           = "delislescope.weap",
    },
    [WP_G43] = {
		.weaponindex        = WP_G43,
		.weapAlts           = WP_NONE,
		.weaponClass        = WEAPON_CLASS_RIFLE_AUTO,
        .maxammo            = 200,
        .uses               = 1,
        .maxclip            = 10,
        .reloadTime         = 2300,
        .reloadTimeFull     = 2300,
        .fireDelayTime      = 0,
        .nextShotTime       = 300,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 20,
		.weaponSpread       = 350,
		.spreadScale        = 0.4f,
		.spreadScaleAdd     = 40,
        .weapRecoilDuration = 40,
        .weapRecoilPitch    = { 0.2f, 0.2f },
        .weapRecoilYaw      = { 0.1f, 0.1f },
		.soundRange         = 2000,
		.aiRange            = AI_WEAPON_RANGE_LONG,
        .moveSpeed          = 0.95f,
        .mod                = MOD_G43,
		.rndTriggerRelease  = qfalse,
	    .iconDrawSize       = WEAPON_ICON_WIDE_OFFSET,
		.bulletBased        = qtrue,
		.hasMuzzle          = qtrue,
		.underwaterFire     = qfalse,
		.gunOffset          = { 1.0f, 0.0f, -1.0f },
		.weapFile           = "g43.weap",
    },
    [WP_PPSH] = {
		.weaponindex        = WP_PPSH,
		.weapAlts           = WP_NONE,
		.weaponClass        = WEAPON_CLASS_SMG,
        .maxammo            = 400,
        .uses               = 1,
        .maxclip            = 71,
        .reloadTime         = 2600,
        .reloadTimeFull     = 2600,
        .fireDelayTime      = 0,
        .nextShotTime       = 65,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 6,
		.weaponSpread       = 1000,
		.spreadScale        = 0.5f,
		.spreadScaleAdd     = 15,
        .weapRecoilDuration = 30,
        .weapRecoilPitch    = { 0.1f, 0.1f },
        .weapRecoilYaw      = { 0.0f, 0.0f },
		.soundRange         = 1000,
		.aiRange            = AI_WEAPON_RANGE_NORMAL,
        .moveSpeed          = 0.95f,
        .mod                = MOD_PPSH,
		.rndTriggerRelease  = qtrue,
	    .iconDrawSize       = WEAPON_ICON_WIDE_OFFSET,
		.bulletBased        = qtrue,
		.hasMuzzle          = qtrue,
		.underwaterFire     = qfalse,
		.gunOffset          = { 0.0f, 0.0f, 0.0f },
		.weapFile           = "ppsh.weap",
    },
    [WP_MOSIN] = {
		.weaponindex        = WP_MOSIN,
		.weapAlts           = WP_NONE,
		.weaponClass        = WEAPON_CLASS_RIFLE_BOLTACTION,
        .maxammo            = 150,
        .uses               = 1,
        .maxclip            = 5,
        .reloadTime         = 2250,
        .reloadTimeFull     = 2250,
        .fireDelayTime      = 0,
        .nextShotTime       = 1400,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 30,
		.weaponSpread       = 300,
		.spreadScale        = 0.5f,
		.spreadScaleAdd     = 50,
        .weapRecoilDuration = 60,
        .weapRecoilPitch    = { 1.0f, 1.0f },
        .weapRecoilYaw      = { 0.1f, 0.1f },
		.soundRange         = 2000,
		.aiRange            = AI_WEAPON_RANGE_LONG,
        .moveSpeed          = 0.95f,
        .mod                = MOD_MOSIN,
		.rndTriggerRelease  = qfalse,
	    .iconDrawSize       = WEAPON_ICON_WIDE_OFFSET,
		.bulletBased        = qtrue,
		.hasMuzzle          = qtrue,
		.underwaterFire     = qfalse,
		.gunOffset          = { 0.0f, 0.0f, -1.0f },
		.weapFile           = "mosin.weap",
    },
    [WP_TT33] = {
		.weaponindex        = WP_TT33,
		.weapAlts           = WP_NONE,
		.weaponClass        = WEAPON_CLASS_PISTOL,
        .maxammo            = 400,
        .uses               = 1,
        .maxclip            = 8,
        .reloadTime         = 1600,
        .reloadTimeFull     = 1600,
        .fireDelayTime      = 0,
        .nextShotTime       = 350,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 9,
		.weaponSpread       = 450,
		.spreadScale        = 0.3f,
		.spreadScaleAdd     = 35,
        .weapRecoilDuration = 50,
        .weapRecoilPitch    = { 0.2f, 0.1f },
        .weapRecoilYaw      = { 0.0f, 0.0f },
		.soundRange         = 700,
		.aiRange            = AI_WEAPON_RANGE_NORMAL,
        .moveSpeed          = 1.0f,
        .mod                = MOD_TT33,
		.rndTriggerRelease  = qfalse,
	    .iconDrawSize       = WEAPON_ICON_NORMAL,
		.bulletBased        = qtrue,
		.hasMuzzle          = qtrue,
		.underwaterFire     = qfalse,
		.gunOffset          = { 0.0f, 0.0f, -1.0f },
		.weapFile           = "tt33.weap",
    },
    [WP_DUAL_TT33] = {
		.weaponindex        = WP_DUAL_TT33,
		.weapAlts           = WP_NONE,
		.weaponClass        = WEAPON_CLASS_PISTOL | WEAPON_CLASS_AKIMBO,
        .maxammo            = 300,
        .uses               = 1,
        .maxclip            = 16,
        .reloadTime         = 2000,
        .reloadTimeFull     = 2000,
        .fireDelayTime      = 0,
        .nextShotTime       = 220,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 9,
		.weaponSpread       = 500,
		.spreadScale        = 0.5f,
		.spreadScaleAdd     = 35,
        .weapRecoilDuration = 50,
        .weapRecoilPitch    = { 0.2f, 0.1f },
        .weapRecoilYaw      = { 0.0f, 0.0f },
		.soundRange         = 700,
		.aiRange            = AI_WEAPON_RANGE_NORMAL,
        .moveSpeed          = 1.0f,
        .mod                = MOD_DUAL_TT33,
		.rndTriggerRelease  = qfalse,
	    .iconDrawSize       = WEAPON_ICON_NORMAL,
		.bulletBased        = qtrue,
		.hasMuzzle          = qtrue,
		.underwaterFire     = qfalse,
		.gunOffset          = { 0.0f, 0.0f, 0.0f },
		.weapFile           = "dualtt33.weap",
    },
    [WP_AUTO5] = {
		.weaponindex        = WP_AUTO5,
		.weapAlts           = WP_NONE,
		.weaponClass        = WEAPON_CLASS_SHOTGUN,
        .maxammo            = 100,
        .uses               = 1,
        .maxclip            = 6,
        .reloadTime         = 2300,
        .reloadTimeFull     = 2300,
        .fireDelayTime      = 0,
        .nextShotTime       = 300,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 10,
		.weaponSpread       = 4000,
		.spreadScale        = 0.6f,
		.spreadScaleAdd     = 15,
        .weapRecoilDuration = 100,
        .weapRecoilPitch    = { 0.1f, 0.2f },
        .weapRecoilYaw      = { 0.5f, 0.5f },
		.soundRange         = 1500,
		.aiRange            = AI_WEAPON_RANGE_SHOTGUN,
        .moveSpeed          = 0.90f,
        .mod                = MOD_AUTO5,
		.rndTriggerRelease  = qfalse,
	    .iconDrawSize       = WEAPON_ICON_WIDE,
		.bulletBased        = qtrue,
		.hasMuzzle          = qtrue,
		.underwaterFire     = qfalse,
		.gunOffset          = { -4.0f, -1.0f, -1.0f },
		.shotgunReloadStart = 580,
		.shotgunReloadLoop  = 850,
		.shotgunReloadEnd   = 250,
		.shotgunPumpStart   = 2100,
		.shotgunPumpLoop    = 580,
		.shotgunPumpEnd     = 1,
		.weapFile           = "auto5.weap",
    },
    [WP_M1941] = {
		.weaponindex        = WP_M1941,
		.weapAlts           = WP_M1941SCOPE,
		.weaponClass        = WEAPON_CLASS_RIFLE_AUTO | WEAPON_CLASS_SCOPABLE,
        .maxammo            = 200,
        .uses               = 1,
        .maxclip            = 10,
        .reloadTime         = 1600,
        .reloadTimeFull     = 3750,
        .fireDelayTime      = 0,
        .nextShotTime       = 300,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 20,
		.weaponSpread       = 300,
		.spreadScale        = 0.4f,
		.spreadScaleAdd     = 40,
        .weapRecoilDuration = 40,
        .weapRecoilPitch    = { 0.2f, 0.2f },
        .weapRecoilYaw      = { 0.1f, 0.1f },
		.soundRange         = 2000,
		.aiRange            = AI_WEAPON_RANGE_LONG,
        .moveSpeed          = 0.95f,
        .mod                = MOD_M1941,
		.rndTriggerRelease  = qfalse,
	    .iconDrawSize       = WEAPON_ICON_WIDE_OFFSET,
		.bulletBased        = qtrue,
		.hasMuzzle          = qtrue,
		.underwaterFire     = qfalse,
		.gunOffset          = { 0.0f, -2.0f, -3.0f },
		.weapFile           = "m1941.weap",
    },
    [WP_M1941SCOPE] = {
		.weaponindex        = WP_M1941SCOPE,
		.weapAlts           = WP_M1941,
		.weaponClass        = WEAPON_CLASS_SCOPED | WEAPON_CLASS_RIFLE_AUTO,
        .maxammo            = 200,
        .uses               = 1,
        .maxclip            = 10,
        .reloadTime         = 1600,
        .reloadTimeFull     = 3750,
        .fireDelayTime      = 0,
        .nextShotTime       = 300,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 20,
		.weaponSpread       = 50,
		.spreadScale        = 0.4f,
		.spreadScaleAdd     = 40,
        .weapRecoilDuration = 40,
        .weapRecoilPitch    = { 0.3f, 0.0f },
        .weapRecoilYaw      = { 0.1f, 0.0f },
		.soundRange         = 2000,
		.aiRange            = AI_WEAPON_RANGE_LONG,
        .moveSpeed          = 0.55f,
        .mod                = MOD_M1941SCOPE,
		.rndTriggerRelease  = qfalse,
	    .iconDrawSize       = WEAPON_ICON_WIDE,
		.bulletBased        = qtrue,
		.hasMuzzle          = qtrue,
		.underwaterFire     = qfalse,
		.weapFile           = "m1941scope.weap",
    },
    [WP_GRENADE_LAUNCHER] = {
		.weaponindex        = WP_GRENADE_LAUNCHER,
		.weapAlts           = WP_NONE,
		.weaponClass        = WEAPON_CLASS_GRENADE,
        .maxammo            = 10,
        .uses               = 1,
        .maxclip            = 15,
        .reloadTime         = 1000,
        .fireDelayTime      = DELAY_THROW,
        .nextShotTime       = 1600,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 200,
		.weaponSpread       = 0,
		.spreadScale        = 0.0f,	
		.spreadScaleAdd     = 0,
        .weapRecoilDuration = 0,
        .weapRecoilPitch    = { 0.0f, 0.0f },
        .weapRecoilYaw      = { 0.0f, 0.0f },
		.soundRange         = 1500,
		.aiRange            = AI_WEAPON_RANGE_GRENADE,
        .moveSpeed          = 1.0f,
        .mod                = MOD_GRENADE_LAUNCHER,
		.rndTriggerRelease  = qfalse,
	    .iconDrawSize       = WEAPON_ICON_NORMAL,
		.bulletBased        = qfalse,
		.hasMuzzle          = qfalse,
		.underwaterFire     = qtrue,
		.weapFile           = "grenade.weap",
    },
    [WP_GRENADE_PINEAPPLE] = {
		.weaponindex        = WP_GRENADE_PINEAPPLE,
		.weapAlts           = WP_NONE,
		.weaponClass        = WEAPON_CLASS_GRENADE,
        .maxammo            = 10,
        .uses               = 1,
        .maxclip            = 15,
        .reloadTime         = 1000,
        .fireDelayTime      = DELAY_THROW,
        .nextShotTime       = 1600,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 220,
		.weaponSpread       = 0,
		.spreadScale        = 0.0f,
		.spreadScaleAdd     = 0,
        .weapRecoilDuration = 0,
        .weapRecoilPitch    = { 0.0f, 0.0f },
        .weapRecoilYaw      = { 0.0f, 0.0f },
		.soundRange         = 1500,
		.aiRange            = AI_WEAPON_RANGE_GRENADE,
        .moveSpeed          = 1.0f,
        .mod                = MOD_GRENADE_PINEAPPLE,
		.rndTriggerRelease  = qfalse,
	    .iconDrawSize       = WEAPON_ICON_NORMAL,
		.bulletBased        = qfalse,
		.hasMuzzle          = qfalse,
		.underwaterFire     = qtrue,
		.weapFile           = "pineapple.weap",
    },
    // Engineer dynamite: cooldown-gated (classWeaponTime/g_engineerChargeTime) like WP_AIRSTRIKE, not ammo-limited.
    [WP_DYNAMITE] = {
		.weaponindex        = WP_DYNAMITE,
		.weapAlts           = WP_NONE,
		.weaponClass        = WEAPON_CLASS_DYNAMITE,
        .maxammo            = 999,
        .uses               = 0,
        .maxclip            = 999,
        .reloadTime         = 0,
        .fireDelayTime      = DELAY_THROW,
        .nextShotTime       = 1600,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 800,
		.weaponSpread       = 0,
		.spreadScale        = 0.0f,			
        .weapRecoilDuration = 0,
        .weapRecoilPitch    = { 0.0f, 0.0f },
        .weapRecoilYaw      = { 0.0f, 0.0f },
		.soundRange         = 64,
		.aiRange            = AI_WEAPON_RANGE_GRENADE,
        .moveSpeed          = 1.0f,
        .mod                = MOD_DYNAMITE,
		.rndTriggerRelease  = qfalse,
	    .iconDrawSize       = WEAPON_ICON_NORMAL,
		.bulletBased        = qfalse,
		.hasMuzzle          = qfalse,
		.underwaterFire     = qtrue,
		.weapFile           = "dynamite.weap",
    },
    // Landmine: cooldown-gated like dynamite (1/3 cost), 10-cap enforced by a live entity scan (G_CountPlayerLandmines), not this ammo pool.
    [WP_LANDMINE] = {
		.weaponindex        = WP_LANDMINE,
		.weapAlts           = WP_NONE,
		.weaponClass        = WEAPON_CLASS_LANDMINE,
        .maxammo            = 10,
        .uses               = 1,
        .maxclip            = 10,
        .reloadTime         = 0,
        .fireDelayTime      = DELAY_THROW,
        .nextShotTime       = 1600,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 250,
		.weaponSpread       = 0,
		.spreadScale        = 0.0f,
        .weapRecoilDuration = 0,
        .weapRecoilPitch    = { 0.0f, 0.0f },
        .weapRecoilYaw      = { 0.0f, 0.0f },
		.soundRange         = 64,
		.aiRange            = AI_WEAPON_RANGE_GRENADE,
        .moveSpeed          = 1.0f,
        .mod                = MOD_LANDMINE,
		.rndTriggerRelease  = qfalse,
	    .iconDrawSize       = WEAPON_ICON_NORMAL,
		.bulletBased        = qfalse,
		.hasMuzzle          = qfalse,
		.underwaterFire     = qtrue,
		.weapFile           = "landmine.weap",
    },
    // Medic health crate: cooldown-gated like dynamite (full charge cost), not ammo-limited; 2-cap enforced by a
    // live entity scan (G_CountAndCapPlayerMedCrates in g_missile.c) that evicts the oldest, not this ammo pool.
    [WP_MEDCRATE] = {
		.weaponindex        = WP_MEDCRATE,
		.weapAlts           = WP_NONE,
		.weaponClass        = WEAPON_CLASS_MEDCRATE,
        .maxammo            = 999,
        .uses               = 0,
        .maxclip            = 999,
        .reloadTime         = 0,
        .fireDelayTime      = DELAY_THROW,
        .nextShotTime       = 1600,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 0,
		.weaponSpread       = 0,
		.spreadScale        = 0.0f,
        .weapRecoilDuration = 0,
        .weapRecoilPitch    = { 0.0f, 0.0f },
        .weapRecoilYaw      = { 0.0f, 0.0f },
		.soundRange         = 64,
		.aiRange            = AI_WEAPON_RANGE_GRENADE,
        .moveSpeed          = 1.0f,
        .mod                = MOD_MEDCRATE,
		.rndTriggerRelease  = qfalse,
	    .iconDrawSize       = WEAPON_ICON_NORMAL,
		.bulletBased        = qfalse,
		.hasMuzzle          = qfalse,
		.underwaterFire     = qtrue,
        .gunAngleOffset     = { -70.0f, -60.0f, -40.0f },
		.weapFile           = "medcrate.weap",
    },
    [WP_PANZERFAUST] = {
		.weaponindex        = WP_PANZERFAUST,
		.weapAlts           = WP_NONE,
		.weaponClass        = WEAPON_CLASS_LAUNCHER,
        .maxammo            = 10,
        .uses               = 1,
        .maxclip            = 1,
        .reloadTime         = 1000,
        .fireDelayTime      = 0,
        .nextShotTime       = 2000,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 400,
		.weaponSpread       = 0,
		.spreadScaleAdd     = 0,
		.spreadScale        = 0.4f,	
        .weapRecoilDuration = 0,
        .weapRecoilPitch    = { 0.0f, 0.0f },
        .weapRecoilYaw      = { 0.0f, 0.0f },
		.soundRange         = 1000,
		.aiRange            = AI_WEAPON_RANGE_LONG,
        .moveSpeed          = 0.80f,
        .mod                = MOD_PANZERFAUST,
		.rndTriggerRelease  = qfalse,
	    .iconDrawSize       = WEAPON_ICON_WIDE_OFFSET,
		.bulletBased        = qfalse,
		.hasMuzzle          = qtrue,
		.underwaterFire     = qfalse,
		.gunOffset          = { 3.0f, 1.0f, 1.0f },
		.gunAngleOffset     = { 100.0f, 10.0f, -100.0f },
		.weapFile           = "panzerfaust.weap",
    },
    [WP_VENOM] = {
		.weaponindex        = WP_VENOM,
		.weapAlts           = WP_NONE,
		.weaponClass        = WEAPON_CLASS_MINIGUN,
        .maxammo            = 1000,
        .uses               = 1,
        .maxclip            = 500,
        .reloadTime         = 3000,
        .fireDelayTime      = 750,
        .nextShotTime       = 45,
        .maxHeat            = 5000,
        .coolRate           = 200,
		.weaponDamage       = 22,
		.weaponSpread       = 1100,
		.spreadScale        = 0.9f,	
		.spreadScaleAdd     = 10,
        .weapRecoilDuration = 50,
        .weapRecoilPitch    = { 0.1f, 0.1f },
        .weapRecoilYaw      = { 0.1f, 0.1f },
		.soundRange         = 1000,
		.aiRange            = AI_WEAPON_RANGE_NORMAL,
        .moveSpeed          = 0.80f,
        .mod                = MOD_VENOM,
		.rndTriggerRelease  = qtrue,
	    .iconDrawSize       = WEAPON_ICON_WIDE_OFFSET,
		.bulletBased        = qtrue,
		.hasMuzzle          = qtrue,
		.underwaterFire     = qfalse,
		.gunOffset          = { -1.0f, 2.0f, 0.0f },
		.weapFile           = "venom.weap",
    },
    [WP_FLAMETHROWER] = {
		.weaponindex        = WP_FLAMETHROWER,
		.weapAlts           = WP_NONE,
		.weaponClass        = WEAPON_CLASS_FLAMER,
        .maxammo            = 150,
        .uses               = 1,
        .maxclip            = 150,
        .reloadTime         = 1000,
        .fireDelayTime      = 0,
        .nextShotTime       = 50,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 5,
		.weaponSpread       = 0,
		.spreadScale        = 0.0f,
		.spreadScaleAdd     = 0,			
        .weapRecoilDuration = 0,
        .weapRecoilPitch    = { 0.0f, 0.0f },
        .weapRecoilYaw      = { 0.0f, 0.0f },
		.soundRange         = 1000,
		.aiRange            = AI_WEAPON_RANGE_FLAMETHROWER,
        .moveSpeed          = 0.85f,
        .mod                = MOD_FLAMETHROWER,
		.rndTriggerRelease  = qfalse,
	    .iconDrawSize       = WEAPON_ICON_WIDE_OFFSET,
		.bulletBased        = qfalse,
		.hasMuzzle          = qfalse,
		.underwaterFire     = qfalse,
		.gunOffset          = { 10.0f, 2.0f, 0.0f },
		.weapFile           = "flamethrower.weap",
    },
    [WP_TESLA] = {
		.weaponindex        = WP_TESLA,
		.weapAlts           = WP_NONE,
		.weaponClass        = WEAPON_CLASS_ENERGY,
        .maxammo            = 150,
        .uses               = 1,
        .maxclip            = 150,
        .reloadTime         = 1000,
        .fireDelayTime      = 0,
        .nextShotTime       = 250,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 15,
		.weaponSpread       = 0,
		.spreadScale        = 0.0f,	
		.spreadScaleAdd     = 0,
        .weapRecoilDuration = 0,
        .weapRecoilPitch    = { 0.0f, 0.0f },
        .weapRecoilYaw      = { 0.0f, 0.0f },
		.soundRange         = 1000,
		.aiRange            = AI_WEAPON_RANGE_TESLA,
        .moveSpeed          = 0.90f,
        .mod                = MOD_TESLA,
		.rndTriggerRelease  = qfalse,
	    .iconDrawSize       = WEAPON_ICON_WIDE_OFFSET,	
		.bulletBased        = qfalse,
		.hasMuzzle          = qfalse,
		.underwaterFire     = qfalse,
		.gunOffset          = { 3.0f, 1.0f, -2.0f },
		.weapFile           = "tesla.weap",
    },
    [WP_MG42M] = {
		.weaponindex        = WP_MG42M,
		.weapAlts           = WP_NONE,
		.weaponClass        = WEAPON_CLASS_MG,
        .maxammo            = 500,
        .uses               = 1,
        .maxclip            = 50,
        .reloadTime         = 4800,
        .reloadTimeFull     = 4800,
        .fireDelayTime      = 0,
        .nextShotTime       = 70,
        .nextShotTime2      = 70,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 15,
		.weaponSpread       = 1200,
		.spreadScale        = 0.6f,
		.spreadScaleAdd     = 15,
        .weapRecoilDuration = 50,
        .weapRecoilPitch    = { 0.1f, 0.1f },
        .weapRecoilYaw      = { 0.1f, 0.1f },
		.soundRange         = 1500,
		.aiRange            = AI_WEAPON_RANGE_NORMAL,
        .moveSpeed          = 0.85f,
        .mod                = MOD_MG42M,
		.rndTriggerRelease  = qtrue,
	    .iconDrawSize       = WEAPON_ICON_WIDE_OFFSET,
		.bulletBased        = qtrue,
		.hasMuzzle          = qtrue,
		.underwaterFire     = qfalse,
		.gunOffset          = { 2.0f, -3.0f, -4.0f },
		.weapFile           = "mg42m.weap",
    },
    [WP_BROWNING] = {
		.weaponindex        = WP_BROWNING,
		.weapAlts           = WP_NONE,
		.weaponClass        = WEAPON_CLASS_MG,
        .maxammo            = 500,
        .uses               = 1,
        .maxclip            = 100,
        .reloadTime         = 7000,
        .reloadTimeFull     = 7000,
        .fireDelayTime      = 0,
        .nextShotTime       = 100,
        .nextShotTime2      = 100,
        .maxHeat            = 2500,
        .coolRate           = 350,
		.weaponDamage       = 15,
		.weaponSpread       = 1000,
		.spreadScale        = 0.6f,
		.spreadScaleAdd     = 15,
        .weapRecoilDuration = 75,
        .weapRecoilPitch    = { 0.1f, 0.1f },
        .weapRecoilYaw      = { 0.1f, 0.1f },
		.soundRange         = 1500,
		.aiRange            = AI_WEAPON_RANGE_NORMAL,
        .moveSpeed          = 0.80f,
        .mod                = MOD_BROWNING,
		.rndTriggerRelease  = qtrue,
	    .iconDrawSize       = WEAPON_ICON_WIDE_OFFSET,
		.bulletBased        = qtrue,
		.hasMuzzle          = qtrue,
		.underwaterFire     = qfalse,
		.gunOffset          = { -6.0f, 0.0f, 0.0f },
		.weapFile           = "browning.weap",
    },
    [WP_MONSTER_ATTACK1] = {
		.weaponindex        = WP_MONSTER_ATTACK1,
		.weapAlts           = WP_NONE,
		.weaponClass        = WEAPON_CLASS_NONE, 
        .maxammo            = 999,
        .uses               = 0,
        .maxclip            = 999,
        .reloadTime         = 0,
        .fireDelayTime      = 50,
        .nextShotTime       = 1000,
        .moveSpeed          = 1.0f,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 0,
		.weaponSpread       = 0,
		.spreadScale        = 0.0f,		
		.spreadScaleAdd     = 0,
        .weapRecoilDuration = 0,
        .weapRecoilPitch    = { 0.0f, 0.0f },
        .weapRecoilYaw      = { 0.0f, 0.0f },
		.soundRange         = 0,
		.aiRange            = 0,
        .mod                = 0,
		.rndTriggerRelease  = qfalse,
	    .iconDrawSize       = WEAPON_ICON_NORMAL,
		.bulletBased        = qfalse,	
		.hasMuzzle          = qfalse,	
		.underwaterFire     = qfalse,
    },
    [WP_MONSTER_ATTACK2] = {
		.weaponindex        = WP_MONSTER_ATTACK2,
		.weapAlts           = WP_NONE,
		.moveSpeed          = 1.0f,
		.weaponClass        = WEAPON_CLASS_NONE,
        .maxammo            = 999,
        .uses               = 0,
        .maxclip            = 999,
        .reloadTime         = 0,
        .fireDelayTime      = 50,
        .nextShotTime       = 0,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 0,
		.weaponSpread       = 0,
		.spreadScale        = 0.0f,	
		.spreadScaleAdd     = 0,		
        .weapRecoilDuration = 0,
        .weapRecoilPitch    = { 0.0f, 0.0f },
        .weapRecoilYaw      = { 0.0f, 0.0f },
		.soundRange         = 0,
		.aiRange            = 0,
        .mod                = 0,
		.rndTriggerRelease  = qfalse,
	    .iconDrawSize       = WEAPON_ICON_NORMAL,
		.bulletBased        = qfalse,
		.hasMuzzle          = qfalse,
		.underwaterFire     = qfalse,		
    },
    [WP_MONSTER_ATTACK3] = {
		.weaponindex        = WP_MONSTER_ATTACK3,
		.weapAlts           = WP_NONE,
		.moveSpeed          = 1.0f,
		.weaponClass        = WEAPON_CLASS_NONE,
        .maxammo            = 999,
        .uses               = 0,
        .maxclip            = 999,
        .reloadTime         = 0,
        .fireDelayTime      = 50,
        .nextShotTime       = 0,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 0,
		.weaponSpread       = 0,
		.spreadScale        = 0.0f,
		.spreadScaleAdd     = 0,
        .weapRecoilDuration = 0,
        .weapRecoilPitch    = { 0.0f, 0.0f },
        .weapRecoilYaw      = { 0.0f, 0.0f },
		.soundRange         = 0,
		.aiRange            = 0,
        .mod                = 0,
		.rndTriggerRelease  = qfalse,
	    .iconDrawSize       = WEAPON_ICON_NORMAL,
		.bulletBased        = qfalse,
		.hasMuzzle          = qfalse,
		.underwaterFire     = qfalse,
    },

	// JPW NERVE -- Lieutenant call-in weapons, cooldown-gated (classWeaponTime/g_LTChargeTime) so ammo/clip are unlimited.
    [WP_AIRSTRIKE] = {
		.weaponindex        = WP_AIRSTRIKE,
		.weapAlts           = WP_NONE,
		.weaponClass        = WEAPON_CLASS_GRENADE,
        .maxammo            = 999,
        .uses               = 0,
        .maxclip            = 999,
        .reloadTime         = 0,
        .fireDelayTime      = 50,
        .nextShotTime       = 1000,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 400,
		.weaponSpread       = 0,
		.spreadScale        = 0.0f,
		.spreadScaleAdd     = 0,
        .weapRecoilDuration = 0,
        .weapRecoilPitch    = { 0.0f, 0.0f },
        .weapRecoilYaw      = { 0.0f, 0.0f },
		.soundRange         = 800,
		.aiRange            = AI_WEAPON_RANGE_GRENADE,
        .moveSpeed          = 0.95f,
        .mod                = MOD_AIRSTRIKE,
		.rndTriggerRelease  = qfalse,
	    .iconDrawSize       = WEAPON_ICON_NORMAL,
		.bulletBased        = qfalse,
		.hasMuzzle          = qfalse,
		.underwaterFire     = qfalse,
		.weapFile           = "airstrike.weap",
    },
    [WP_ARTY] = {
		.weaponindex        = WP_ARTY,
		.weapAlts           = WP_NONE,
		.weaponClass        = WEAPON_CLASS_NONE, // fired via the binocular call-in hook, never the normal weapon-fire dispatch
        .maxammo            = 1,
        .uses               = 0,
        .maxclip            = 1,
        .reloadTime         = 3000,
        .reloadTimeFull     = 3000,
        .fireDelayTime      = 50,
        .nextShotTime       = 1000,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 400,
		.weaponSpread       = 0,
		.spreadScale        = 0.0f,
		.spreadScaleAdd     = 0,
        .weapRecoilDuration = 0,
        .weapRecoilPitch    = { 0.0f, 0.0f },
        .weapRecoilYaw      = { 0.0f, 0.0f },
		.soundRange         = 0,
		.aiRange            = AI_WEAPON_RANGE_GRENADE,
        .moveSpeed          = 0.95f,
        .mod                = MOD_ARTY,
		.rndTriggerRelease  = qfalse,
	    .iconDrawSize       = WEAPON_ICON_NORMAL,
		.bulletBased        = qfalse,
		.hasMuzzle          = qfalse,
		.underwaterFire     = qfalse,
		.weapFile           = "arty.weap", // fired purely via binoculars, never carried; weapFile only provides the falling-shell missileModel
    },
    [WP_SMOKETRAIL] = {
		.weaponindex        = WP_SMOKETRAIL,
		.weapAlts           = WP_NONE,
		.weaponClass        = WEAPON_CLASS_NONE,
        .maxammo            = 0,
        .uses               = 0,
        .maxclip            = 0,
        .reloadTime         = 0,
        .fireDelayTime      = 0,
        .nextShotTime       = 0,
        .maxHeat            = 0,
        .coolRate           = 0,
		.weaponDamage       = 0,
		.weaponSpread       = 0,
		.spreadScale        = 0.0f,
		.spreadScaleAdd     = 0,
        .weapRecoilDuration = 0,
        .weapRecoilPitch    = { 0.0f, 0.0f },
        .weapRecoilYaw      = { 0.0f, 0.0f },
		.soundRange         = 0,
		.aiRange            = 0,
        .moveSpeed          = 1.0f,
        .mod                = 0,
		.rndTriggerRelease  = qfalse,
	    .iconDrawSize       = WEAPON_ICON_NORMAL,
		.bulletBased        = qfalse,
		.hasMuzzle          = qfalse,
		.underwaterFire     = qfalse,
		// no weapFile: purely a visual debris tag on non-player missile entities, never registered as a real weapon
    },
};


// new (10/18/00)
char *animStrings[] = {
	"BOTH_DEATH1",
	"BOTH_DEAD1",
	"BOTH_DEAD1_WATER",
	"BOTH_DEATH2",
	"BOTH_DEAD2",
	"BOTH_DEAD2_WATER",
	"BOTH_DEATH3",
	"BOTH_DEAD3",
	"BOTH_DEAD3_WATER",

	"BOTH_CLIMB",
	"BOTH_CLIMB_DOWN",
	"BOTH_CLIMB_DISMOUNT",

	"BOTH_SALUTE",

	"BOTH_PAIN1",
	"BOTH_PAIN2",
	"BOTH_PAIN3",
	"BOTH_PAIN4",
	"BOTH_PAIN5",
	"BOTH_PAIN6",
	"BOTH_PAIN7",
	"BOTH_PAIN8",

	"BOTH_GRAB_GRENADE",

	"BOTH_ATTACK1",
	"BOTH_ATTACK2",
	"BOTH_ATTACK3",
	"BOTH_ATTACK4",
	"BOTH_ATTACK5",

	"BOTH_EXTRA1",
	"BOTH_EXTRA2",
	"BOTH_EXTRA3",
	"BOTH_EXTRA4",
	"BOTH_EXTRA5",
	"BOTH_EXTRA6",
	"BOTH_EXTRA7",
	"BOTH_EXTRA8",
	"BOTH_EXTRA9",
	"BOTH_EXTRA10",
	"BOTH_EXTRA11",
	"BOTH_EXTRA12",
	"BOTH_EXTRA13",
	"BOTH_EXTRA14",
	"BOTH_EXTRA15",
	"BOTH_EXTRA16",
	"BOTH_EXTRA17",
	"BOTH_EXTRA18",
	"BOTH_EXTRA19",
	"BOTH_EXTRA20",

	"TORSO_GESTURE",
	"TORSO_GESTURE2",
	"TORSO_GESTURE3",
	"TORSO_GESTURE4",

	"TORSO_DROP",

	"TORSO_RAISE",   // (low)
	"TORSO_ATTACK",
	"TORSO_STAND",
	"TORSO_STAND_ALT1",
	"TORSO_STAND_ALT2",
	"TORSO_READY",
	"TORSO_RELAX",

	"TORSO_RAISE2",  // (high)
	"TORSO_ATTACK2",
	"TORSO_STAND2",
	"TORSO_STAND2_ALT1",
	"TORSO_STAND2_ALT2",
	"TORSO_READY2",
	"TORSO_RELAX2",

	"TORSO_RAISE3",  // (pistol)
	"TORSO_ATTACK3",
	"TORSO_STAND3",
	"TORSO_STAND3_ALT1",
	"TORSO_STAND3_ALT2",
	"TORSO_READY3",
	"TORSO_RELAX3",

	"TORSO_RAISE4",  // (shoulder)
	"TORSO_ATTACK4",
	"TORSO_STAND4",
	"TORSO_STAND4_ALT1",
	"TORSO_STAND4_ALT2",
	"TORSO_READY4",
	"TORSO_RELAX4",

	"TORSO_RAISE5",  // (throw)
	"TORSO_ATTACK5",
	"TORSO_ATTACK5B",
	"TORSO_STAND5",
	"TORSO_STAND5_ALT1",
	"TORSO_STAND5_ALT2",
	"TORSO_READY5",
	"TORSO_RELAX5",

	"TORSO_RELOAD1", // (low)
	"TORSO_RELOAD2", // (high)
	"TORSO_RELOAD3", // (pistol)
	"TORSO_RELOAD4", // (shoulder)

	"TORSO_MG42",        // firing tripod mounted weapon animation

	"TORSO_MOVE",        // torso anim to play while moving and not firing (swinging arms type thing)
	"TORSO_MOVE_ALT",        // torso anim to play while moving and not firing (swinging arms type thing)

	"TORSO_EXTRA",
	"TORSO_EXTRA2",
	"TORSO_EXTRA3",
	"TORSO_EXTRA4",
	"TORSO_EXTRA5",
	"TORSO_EXTRA6",
	"TORSO_EXTRA7",
	"TORSO_EXTRA8",
	"TORSO_EXTRA9",
	"TORSO_EXTRA10",

	"LEGS_WALKCR",
	"LEGS_WALKCR_BACK",
	"LEGS_WALK",
	"LEGS_RUN",
	"LEGS_BACK",
	"LEGS_SWIM",
	"LEGS_SWIM_IDLE",

	"LEGS_JUMP",
	"LEGS_JUMPB",
	"LEGS_LAND",

	"LEGS_IDLE",
	"LEGS_IDLE_ALT", //	"LEGS_IDLE2"
	"LEGS_IDLECR",

	"LEGS_TURN",

	"LEGS_BOOT",     // kicking animation

	"LEGS_EXTRA1",
	"LEGS_EXTRA2",
	"LEGS_EXTRA3",
	"LEGS_EXTRA4",
	"LEGS_EXTRA5",
	"LEGS_EXTRA6",
	"LEGS_EXTRA7",
	"LEGS_EXTRA8",
	"LEGS_EXTRA9",
	"LEGS_EXTRA10",
};


// old
char *animStringsOld[] = {
	"BOTH_DEATH1",
	"BOTH_DEAD1",
	"BOTH_DEATH2",
	"BOTH_DEAD2",
	"BOTH_DEATH3",
	"BOTH_DEAD3",

	"BOTH_CLIMB",
	"BOTH_CLIMB_DOWN",
	"BOTH_CLIMB_DISMOUNT",

	"BOTH_SALUTE",

	"BOTH_PAIN1",
	"BOTH_PAIN2",
	"BOTH_PAIN3",
	"BOTH_PAIN4",
	"BOTH_PAIN5",
	"BOTH_PAIN6",
	"BOTH_PAIN7",
	"BOTH_PAIN8",

	"BOTH_EXTRA1",
	"BOTH_EXTRA2",
	"BOTH_EXTRA3",
	"BOTH_EXTRA4",
	"BOTH_EXTRA5",

	"TORSO_GESTURE",
	"TORSO_GESTURE2",
	"TORSO_GESTURE3",
	"TORSO_GESTURE4",

	"TORSO_DROP",

	"TORSO_RAISE",   // (low)
	"TORSO_ATTACK",
	"TORSO_STAND",
	"TORSO_READY",
	"TORSO_RELAX",

	"TORSO_RAISE2",  // (high)
	"TORSO_ATTACK2",
	"TORSO_STAND2",
	"TORSO_READY2",
	"TORSO_RELAX2",

	"TORSO_RAISE3",  // (pistol)
	"TORSO_ATTACK3",
	"TORSO_STAND3",
	"TORSO_READY3",
	"TORSO_RELAX3",

	"TORSO_RAISE4",  // (shoulder)
	"TORSO_ATTACK4",
	"TORSO_STAND4",
	"TORSO_READY4",
	"TORSO_RELAX4",

	"TORSO_RAISE5",  // (throw)
	"TORSO_ATTACK5",
	"TORSO_ATTACK5B",
	"TORSO_STAND5",
	"TORSO_READY5",
	"TORSO_RELAX5",

	"TORSO_RELOAD1", // (low)
	"TORSO_RELOAD2", // (high)
	"TORSO_RELOAD3", // (pistol)
	"TORSO_RELOAD4", // (shoulder)

	"TORSO_MG42",        // firing tripod mounted weapon animation

	"TORSO_MOVE",        // torso anim to play while moving and not firing (swinging arms type thing)

	"TORSO_EXTRA2",
	"TORSO_EXTRA3",
	"TORSO_EXTRA4",
	"TORSO_EXTRA5",

	"LEGS_WALKCR",
	"LEGS_WALKCR_BACK",
	"LEGS_WALK",
	"LEGS_RUN",
	"LEGS_BACK",
	"LEGS_SWIM",

	"LEGS_JUMP",
	"LEGS_LAND",

	"LEGS_IDLE",
	"LEGS_IDLE2",
	"LEGS_IDLECR",

	"LEGS_TURN",

	"LEGS_BOOT",     // kicking animation

	"LEGS_EXTRA1",
	"LEGS_EXTRA2",
	"LEGS_EXTRA3",
	"LEGS_EXTRA4",
	"LEGS_EXTRA5",
};

/*QUAKED item_***** ( 0 0 0 ) (-16 -16 -16) (16 16 16) SUSPENDED SPIN PERSISTANT
DO NOT USE THIS CLASS, IT JUST HOLDS GENERAL INFORMATION.
SUSPENDED - will allow items to hang in the air, otherwise they are dropped to the next surface.
SPIN - will allow items to spin in place.
PERSISTANT - some items (ex. clipboards) can be picked up, but don't disappear

If an item is the target of another entity, it will not spawn in until fired.

An item fires all of its targets when it is picked up.  If the toucher can't carry it, the targets won't be fired.

"notfree" if set to 1, don't spawn in free for all games
"notteam" if set to 1, don't spawn in team games
"notsingle" if set to 1, don't spawn in single player games
"wait"	override the default wait before respawning.  -1 = never respawn automatically, which can be used with targeted spawning.
"random" random number of plus or minus seconds varied from the respawn time
"count" override quantity or duration on most items.
"stand" if the item has a stand (ex: mp40_stand.md3) this specifies which stand tag to attach the weapon to ("stand":"4" would mean "tag_stand4" for example)  only weapons support stands currently
*/

// JOSEPH 5-2-00
//----(SA) the addition of the 'ammotype' field was added by me, not removed by id (SA)
gitem_t bg_itemlist[] =
{
	{
		NULL,
		NULL,
		{ NULL,
		  NULL,
		  0, 0, 0},
		NULL,   // icon
		NULL,   // ammo icon
		NULL,   // pickup
		0,
		0,
		0,
		0,
		0,          // ammotype
		0,
		0,          // cliptype
		"",          // precache
		"",          // sounds
		{0,0,0,0}
	},  // leave index 0 alone


	{
		"item_clipboard",
		"sound/pickup/armor/body_pickup.wav",
		{   "models/powerups/clipboard/clipboard.md3",
			0,
			0,
			0, 0 },
		"icons/iconh_small",
		NULL,                   // ammo icon
		"",
		1,
		IT_CLIPBOARD,
		WP_NONE,
		0,
		0,
		0,
		0,
		"",
		"",
		{0,0,0,0}
	},

	{
		"item_treasure",
		"sound/pickup/treasure/gold.wav",
		{   "models/powerups/treasure/goldbar.md3",
			0,
			0,
			0, 0 },
		"icons/iconh_small", // (SA) placeholder
		NULL,                   // ammo icon
		"Treasure Item",     // (SA) placeholder
		5,
		IT_TREASURE,
		WP_NONE,
		0,
		0,
		0,
		0,
		"",
		"",
		{0,0,0,0}
	},


	//
	// ARMOR/HEALTH/STAMINA
	//


	{
		"item_health_small",
		"sound/pickup/health/health_pickup.wav",
		{   "models/powerups/health/health_s.md3",
			0,
			0, 0,  0 },
		"icons/iconh_small",
		NULL,   // ammo icon
		"Small Health",
		5,
		IT_HEALTH,
		WP_NONE,
		0,
		0,
		0,
		0,
		"",
		"",
		{10,10,5,5}
	},

	{
		"item_health",
		"sound/pickup/health/health_pickup.wav",
		{   "models/powerups/health/health_m.md3",
			0,
			0, 0,  0 },
		"icons/iconh_med",
		NULL,   // ammo icon
		"Med Health",
		25,
		IT_HEALTH,
		WP_NONE,
		0,
		0,
		0,
		0,
		"",
		"",
		{25,25,15,15}
	},

	{
		"item_health_large",
		"sound/pickup/health/health_pickup.wav",
		{   "models/powerups/health/health_l.md3",
			0, 0, 0,   0 },
		"icons/iconh_large",
		NULL,   // ammo icon
		"Large Health",
		50,
		IT_HEALTH,
		WP_NONE,
		0,
		0,
		0,
		0,
		"",
		"",
		{50,50,30,20}
	},

	{
		"item_health_turkey",
		"sound/pickup/health/hot_pickup.wav",
		{   "models/powerups/health/health_t3.md3",  // just plate (should now be destructable)
			"models/powerups/health/health_t2.md3",  // half eaten
			"models/powerups/health/health_t1.md3",  // whole turkey
			0, 0 },
		"icons/iconh_turkey",
		NULL,   // ammo icon
		"Hot Meal",
		15,                 // amount given in last stage
		IT_HEALTH,
		WP_NONE,
		0,
		0,
		0,
		0,
		"",
		"",
		{45,35,25,15}   // amount given in first stage based on gameskill level
	},

	{
		"item_health_breadandmeat",
		"sound/pickup/health/cold_pickup.wav",
		{   "models/powerups/health/health_b3.md3",  // just plate (should now be destructable)
			"models/powerups/health/health_b2.md3",  // half eaten
			"models/powerups/health/health_b1.md3",  // whole turkey
			0, 0 },
		"icons/iconh_breadandmeat",
		NULL,   // ammo icon
		"Cold Meal",
		10,                 // amount given in last stage
		IT_HEALTH,
		WP_NONE,
		0,
		0,
		0,
		0,
		"",
		"",
		{30,30,20,15}   // amount given in first stage based on gameskill level
	},

	{
		"item_health_wall_box",
		"sound/pickup/health/health_pickup.wav",
		{   "models/powerups/health/health_wallbox2.md3",
			"models/powerups/health/health_wallbox1.md3",
			0, 0, 0},
		"icons/iconh_wall",
		NULL,   // ammo icon
		"Health",
		25,
		IT_HEALTH,
		WP_NONE,
		0,
		0,
		0,
		0,
		"",
		"",
		{25,25,25,25}
	},

	{
		"item_health_wall",
		"sound/pickup/health/health_pickup.wav",
		{   "models/powerups/health/health_w.md3",
			0, 0, 0,   0 },
		"icons/iconh_wall",
		NULL,   // ammo icon
		"Health",
		25,
		IT_HEALTH,
		WP_NONE,
		0,
		0,
		0,
		0,
		"",
		"",
		{25,25,15,15}
	},

	//
	// STAMINA
	//



	{
		"item_stamina_stein",
		"sound/pickup/health/stamina_pickup.wav",
		{   "models/powerups/instant/stamina_stein.md3",
			0, 0, 0,   0 },
		"icons/icons_stein",
		NULL,   // ammo icon
		"Stamina",
		25,
		IT_POWERUP,
		WP_NONE,
		PW_NOFATIGUE,
		0,
		0,
		0,
		"",
		"",
		{30,25,20,15}
	},


	{
		"item_stamina_brandy",
		"sound/sound/pickup/health/stamina_pickup.wav",
		{   "models/powerups/instant/stamina_brandy2.md3",
			"models/powerups/instant/stamina_brandy1.md3",
			0, 0,  0 },
		"icons/icons_brandy",
		NULL,   // ammo icon
		"Stamina",
		25,
		IT_POWERUP,
		WP_NONE,
		PW_NOFATIGUE,
		0,
		0,
		0,
		"",
		"",
		{30,25,20,15}
	},


	//
	// ARMOR
	//


	{
		"item_armor_body",
		"sound/pickup/armor/body_pickup.wav",
		{   "models/powerups/armor/armor_body1.md3",
			0, 0, 0,   0 },
		"icons/iconr_body",
		NULL,   // ammo icon
		"Flak Jacket",
		75,
		IT_ARMOR,
		WP_NONE,
		0,
		0,
		0,
		0,
		"",
		"",
		{75,75,75,75}
	},

	{
		"item_armor_body_hang",
		"sound/pickup/armor/body_pickup.wav",
		{   "models/powerups/armor/armor_body2.md3",
			0, 0, 0,   0 },
		"icons/iconr_bodyh",
		NULL,   // ammo icon
		"Flak Jacket",
		75,
		IT_ARMOR,
		WP_NONE,
		0,
		0,
		0,
		0,
		"",
		"",
		{75,75,75,75}
	},

	{
		"item_armor_head",
		"sound/pickup/armor/head_pickup.wav",
		{   "models/powerups/armor/armor_head1.md3",
			0, 0, 0,   0 },
		"icons/iconr_head",
		NULL,   // ammo icon
		"Armored Helmet",
		25,
		IT_ARMOR,
		WP_NONE,
		0,
		0,
		0,
		0,
		"",
		"",
		{25,25,25,25}
	},



	//
	// WEAPONS
	//


	{
		"weapon_knife",
		"sound/misc/w_pkup.wav",
		{   
			
		"models/weapons/melee/knife/knife_pickup.md3",
		"models/weapons/melee/knife/v_knife.md3",
		"models/weapons/melee/knife/knife_pickup.md3",
			0,
			0},

		"icons/iconw_knife_1",   // icon
		"icons/ammo2",           // ammo icon
		"Knife",             // pickup
		50,
		IT_WEAPON,
		WP_KNIFE,
		WP_KNIFE,
		WP_KNIFE,
		WP_KNIFE,
		WP_KNIFE,
		"",                      // precache
		"",                      // sounds
		{0,0,0,0}
	},

	{
		"weapon_luger",
		"sound/misc/w_pkup.wav",
		{   "models/weapons/pistols/luger/luger_3rd.md3",
			"models/weapons/pistols/luger/v_luger.md3",
			"models/weapons/pistols/luger/luger_3rd.md3", 
			0,
			0},

		"icons/iconw_luger_1",   // icon
		"icons/ammo2",           // ammo icon
		"Luger",             // pickup
		50,
		IT_WEAPON,
		WP_LUGER,
		WP_LUGER,
		WP_LUGER,
		WP_LUGER,
		WP_LUGER,
		"",                      // precache
		"",                      // sounds
		{0,0,0,0}
	},

	{
		"weapon_mauserRifle",
		"sound/misc/w_pkup.wav",
		{   "models/weapons/rifles/mauser/mauser_3rd.md3",
			"models/weapons/rifles/mauser/v_mauser.md3",
			"models/weapons/rifles/mauser/mauser_3rd.md3",
			0, 0  },

		"icons/iconw_mauser_1",  // icon
		"icons/ammo3",           // ammo icon
		"Mauser Rifle",          // pickup
		50,
		IT_WEAPON,
		WP_MAUSER,
		WP_MAUSER,
		WP_MAUSER,
		WP_MAUSER,
		WP_MAUSER,
		"",                      // precache
		"",                      // sounds
		{0,0,0,0}
	},

	{
		"weapon_thompson",
		"sound/misc/w_pkup.wav",
		{   "models/weapons/smgs/thompson/thompson_3rd.md3",
			"models/weapons/smgs/thompson/v_thompson.md3",
			"models/weapons/smgs/thompson/thompson_3rd.md3",
			0, 0 },

		"icons/iconw_thompson_1",    // icon
		"icons/ammo2",           // ammo icon
		"Thompson",              // pickup
		30,
		IT_WEAPON,
		WP_THOMPSON,
		WP_THOMPSON,
		WP_COLT,
		WP_THOMPSON,
		WP_THOMPSON,
		"",                  // precache
		"",                  // sounds
		{0,0,0,0}
	},

	{
		"weapon_sten",
		"sound/misc/w_pkup.wav",
		{   "models/weapons/smgs/sten/sten_3rd.md3",
			"models/weapons/smgs/sten/v_sten.md3",
			"models/weapons/smgs/sten/sten_3rd.md3",
			0,0 },
		"icons/iconw_sten_1",    // icon
		"icons/ammo2",           // ammo icon
		"Sten",                  // pickup
		30,
		IT_WEAPON,
		WP_STEN,
		WP_STEN,
		WP_LUGER,
		WP_STEN,
		WP_STEN,
		"",                  // precache
		"",                  // sounds
		{0,0,0,0}
	},


	{
		"weapon_akimbo",
		"sound/misc/w_pkup.wav",
		{   "models/weapons/pistols/colt/colt_3rd.md3",
			"models/weapons/pistols/colt2/v_colt2.md3",
			"models/weapons/pistols/colt/colt_3rd.md3",
			0, 0 },

		"icons/iconw_colt_1",    // icon
		"icons/ammo2",           // ammo icon
		"Dual Colts",            // pickup
		50,
		IT_WEAPON,
		WP_AKIMBO,
		WP_AKIMBO,
		WP_COLT,
		WP_AKIMBO,
		WP_AKIMBO,
		"",                      // precache
		"",                      // sounds
		{0,0,0,0}
	},

	{
		"weapon_colt",
		"sound/misc/w_pkup.wav",
		{   "models/weapons/pistols/colt/colt_3rd.md3",
			"models/weapons/pistols/colt/v_colt.md3",
			"models/weapons/pistols/colt/colt_3rd.md3",
			0, 0 },

		"icons/iconw_colt_1",    // icon
		"icons/ammo2",           // ammo icon
		"Colt",                  // pickup
		50,
		IT_WEAPON,
		WP_COLT,
		WP_COLT,
		WP_COLT,
		WP_COLT,
		WP_COLT,
		"",                      // precache
		"",                      // sounds
		{0,0,0,0}
	},

	{
		"weapon_tt33",
		"sound/misc/w_pkup.wav",
		{   "models/weapons/pistols/tt33/tt33_3rd.md3",
			"models/weapons/pistols/tt33/v_tt33.md3",
			"models/weapons/pistols/tt33/tt33_3rd.md3",
			0, 0 },

		"icons/iconw_tt33",      // icon
		"icons/ammo2",           // ammo icon
		"TT-33",                 // pickup
		50,
		IT_WEAPON,
		WP_TT33,
		WP_TT33,
		WP_TT33,
		WP_TT33,
		WP_TT33,
		"",                      // precache
		"",                      // sounds
		{0,0,0,0}
	},

	{
		"weapon_dualtt33",
		"sound/misc/w_pkup.wav",
		{   "models/weapons/pistols/tt33/tt33_3rd.md3",
			"models/weapons/pistols/tt33_2/v_tt33_2.md3",
			"models/weapons/pistols/tt33/tt33_3rd.md3",
			0, 0 },

		"icons/iconw_dualtt33",  // icon
		"icons/ammo2",           // ammo icon
		"Dual TT-33s",           // pickup
		50,
		IT_WEAPON,
		WP_DUAL_TT33,
		WP_DUAL_TT33,
		WP_TT33,
		WP_DUAL_TT33,
		WP_DUAL_TT33,
		"",                      // precache
		"",                      // sounds
		{0,0,0,0}
	},


	{
		"NOT_weapon_garandRifle",    //----(SA)	modified so it can no longer be given individually
		"sound/misc/w_pkup.wav",
		{   "models/weapons/rifles/snooper/snooper_3rd.md3",
			"models/weapons/rifles/snooper/v_snooper.md3",
			"models/weapons/rifles/snooper/snooper_3rd.md3",
			0, 0 },

		"icons/iconw_garand_1",  // icon
		"icons/ammo10",              // ammo icon
		"Snooper Rifle",             // pickup		//----(SA)	modified
		20,
		IT_WEAPON,
		WP_SNOOPER,
		WP_SNOOPER,
		WP_SNOOPER,
		WP_SNOOPER,
		WP_SNOOPER,
		"",                          // precache
		"",                          // sounds
		{0,0,0,0}
	},

	{
		"weapon_m1garand",
		"sound/misc/w_pkup.wav",
		{   "models/weapons/auto_rifles/m1_garand/m1_3rd.md3",
			"models/weapons/auto_rifles/m1_garand/v_m1.md3",
			"models/weapons/auto_rifles/m1_garand/m1_3rd.md3",
			0, 0 },

		"icons/iconw_m1garand", // icon
		"icons/ammo10",           // ammo icon
		"M1 Garand",              // pickup
		20,
		IT_WEAPON,
		WP_M1GARAND,
		WP_M1GARAND,
		WP_M1GARAND,
		WP_M1GARAND,
		WP_M1GARAND,
		"",                       // precache
		"",                       // sounds
		{0,0,0,0}
	},

	{
		"weapon_m7",
		"sound/misc/w_pkup.wav",
		{   "models/weapons/auto_rifles/m1_garand/m1_3rd.md3",
			"models/weapons/auto_rifles/m1_garand/v_m1.md3",
			"models/weapons/auto_rifles/m1_garand/m1_3rd.md3",
			0, 0 },

		"icons/iconw_m7_1",       // icon
		"icons/icona_grenade",    // ammo icon
		"M7 Grenade Launcher",    // pickup
		4,
		IT_WEAPON,
		WP_M7,
		WP_M7,
		WP_M7,
		WP_M7,
		WP_M7,
		"",                       // precache
		"",                       // sounds
		{0,0,0,0}
	},

{
    "weapon_mp40",
    "sound/misc/w_pkup.wav",
    {
        "models/weapons/smgs/mp40/mp40_3rd.md3",
        "models/weapons/smgs/mp40/v_mp40.md3",
        "models/weapons/smgs/mp40/mp40_3rd.md3",
        0, 0
    },
    "icons/iconw_mp40_1",
    "icons/ammo2",
    "MP40",
    30,
    IT_WEAPON,
	WP_MP40,
    WP_MP40,
    WP_LUGER,   // ammo pool
    WP_MP40,    // clip pool
    WP_MP40,    // clip pool
    "",
    "",
    {0,0,0,0}
},

{
    "weapon_mp34",
    "sound/misc/w_pkup.wav",
    {
        "models/weapons/smgs/mp34/mp34_3rd.md3",
        "models/weapons/smgs/mp34/v_mp34.md3",
        "models/weapons/smgs/mp34/mp34_3rd.md3",
        0, 0
    },
    "icons/iconw_mp34_1",
    "icons/ammo2",
    "MP34",
    30,
    IT_WEAPON,
	WP_MP34,
    WP_MP34,
    WP_LUGER,   // ammo pool
    WP_MP34,    // clip pool
    WP_MP34,    // clip pool
    "",
    "",
    {0,0,0,0}
},

	{
		"weapon_fg42",
		"sound/misc/w_pkup.wav",
		{   "models/weapons/assault_rifles/fg42/fg42_3rd.md3",
			"models/weapons/assault_rifles/fg42/v_fg42.md3",
			"models/weapons/assault_rifles/fg42/fg42_3rd.md3",
			0, 0 },

		"icons/iconw_fg42_1",    // icon
		"icons/ammo5",          // ammo icon
		"FG42 Paratroop Rifle",      // pickup
		10,
		IT_WEAPON,
		WP_FG42,
		WP_FG42,
		WP_MAUSER,
		WP_FG42,
		WP_FG42,
		"",                  // precache
		"",                  // sounds
		{0,0,0,0}
	},

	{
		"weapon_bar",
		"sound/misc/w_pkup.wav",
		{   "models/weapons/assault_rifles/bar/bar_3rd.md3",
			"models/weapons/assault_rifles/bar/v_bar.md3",
			"models/weapons/assault_rifles/bar/bar_3rd.md3",
			0, 0 },

		"icons/iconw_bar",         // icon
		"icons/ammo10",              // ammo icon
		"Browning Automatic Rifle",  // pickup
		20,
		IT_WEAPON,
		WP_BAR,
		WP_BAR,
		WP_M1GARAND,
		WP_BAR,
		WP_BAR,
		"",                  // precache
		"",                  // sounds
		{0,0,0,0}
	},

	{
		"weapon_mp44",
		"sound/misc/w_pkup.wav",
		{   "models/weapons/assault_rifles/mp44/mp44_3rd.md3",
			"models/weapons/assault_rifles/mp44/v_mp44.md3",
			"models/weapons/assault_rifles/mp44/mp44_3rd.md3",
			0, 0 },

		"icons/iconw_mp44",       // icon
		"icons/ammo5",            // ammo icon
		"MP44 Sturmgewehr",       // pickup
		30,
		IT_WEAPON,
		WP_MP44,
		WP_MP44,
		WP_MP44,
		WP_MP44,
		WP_MP44,
		"",                  // precache
		"",                  // sounds
		{0,0,0,0}
	},

	{
		"weapon_m97",
		"sound/misc/w_pkup.wav",
		{   "models/weapons/shotguns/ithaca/ithaca_3rd.md3",
			"models/weapons/shotguns/ithaca/v_ithaca.md3",
			"models/weapons/shotguns/ithaca/ithaca_3rd.md3",
			0, 0 },

		"icons/iconw_ithaca",     // icon
		"icons/ammo10",           // ammo icon
		"Ithaca 37",              // pickup
		30,
		IT_WEAPON,
		WP_M97,
		WP_M97,
		WP_M97,
		WP_M97,
		WP_M97,
		"",                  // precache
		"",                  // sounds
		{0,0,0,0}
	},

	{
		"weapon_auto5",
		"sound/misc/w_pkup.wav",
		{   "models/weapons/shotguns/auto5/auto5_3rd.md3",
			"models/weapons/shotguns/auto5/v_auto5.md3",
			"models/weapons/shotguns/auto5/auto5_3rd.md3",
			0, 0 },

		"icons/iconw_auto5",      // icon
		"icons/ammo10",           // ammo icon
		"Auto-5",                 // pickup
		30,
		IT_WEAPON,
		WP_AUTO5,
		WP_AUTO5,
		WP_AUTO5,
		WP_AUTO5,
		WP_AUTO5,
		"",                  // precache
		"",                  // sounds
		{0,0,0,0}
	},

	{
		"weapon_revolver",
		"sound/misc/w_pkup.wav",
		{   "models/weapons/pistols/revolver/revolver_3rd.md3",
			"models/weapons/pistols/revolver/v_revolver.md3",
			"models/weapons/pistols/revolver/revolver_3rd.md3",
			0, 0 },

		"icons/iconw_revolver",  // icon
		"icons/ammo2",           // ammo icon
		"Revolver",              // pickup
		30,
		IT_WEAPON,
		WP_REVOLVER,
		WP_REVOLVER,
		WP_REVOLVER,
		WP_REVOLVER,
		WP_REVOLVER,
		"",                  // precache
		"",                  // sounds
		{0,0,0,0}
	},

	{
		"weapon_hdm",
		"sound/misc/w_pkup.wav",
		{   "models/weapons/pistols/hdm/hdm_3rd.md3",
			"models/weapons/pistols/hdm/v_hdm.md3",
			"models/weapons/pistols/hdm/hdm_3rd.md3",
			0, 0 },

		"icons/iconw_hdm_1",     // icon
		"icons/ammo2",           // ammo icon
		"HDM",                   // pickup
		30,
		IT_WEAPON,
		WP_HDM,
		WP_HDM,
		WP_HDM,
		WP_HDM,
		WP_HDM,
		"",                  // precache
		"",                  // sounds
		{0,0,0,0}
	},

	{
		"weapon_delisle",
		"sound/misc/w_pkup.wav",
		{   "models/weapons/rifles/delisle/delisle_3rd.md3",
			"models/weapons/rifles/delisle/v_delisle.md3",
			"models/weapons/rifles/delisle/delisle_3rd.md3",
			0, 0 },

		"icons/iconw_delisle",   // icon
		"icons/ammo3",           // ammo icon
		"De Lisle Carbine",      // pickup
		30,
		IT_WEAPON,
		WP_DELISLE,
		WP_DELISLE,
		WP_DELISLE,
		WP_DELISLE,
		WP_DELISLE,
		"",                  // precache
		"",                  // sounds
		{0,0,0,0}
	},

	{
		"weapon_g43",
		"sound/misc/w_pkup.wav",
		{   "models/weapons/auto_rifles/g43/g43_3rd.md3",
			"models/weapons/auto_rifles/g43/v_g43.md3",
			"models/weapons/auto_rifles/g43/g43_3rd.md3",
			0, 0 },

		"icons/iconw_g43",       // icon
		"icons/ammo10",          // ammo icon
		"G43 rifle",             // pickup
		20,
		IT_WEAPON,
		WP_G43,
		WP_G43,
		WP_G43,
		WP_G43,
		WP_G43,
		"",                  // precache
		"",                  // sounds
		{0,0,0,0}
	},

	{
		"weapon_ppsh",
		"sound/misc/w_pkup.wav",
		{   "models/weapons/smgs/ppsh/ppsh_3rd.md3",
			"models/weapons/smgs/ppsh/v_ppsh.md3",
			"models/weapons/smgs/ppsh/ppsh_3rd.md3",
			0, 0 },

		"icons/iconw_ppsh_1",    // icon
		"icons/ammo2",           // ammo icon
		"PPSh-41",               // pickup
		30,
		IT_WEAPON,
		WP_PPSH,
		WP_PPSH,
		WP_PPSH,
		WP_PPSH,
		WP_PPSH,
		"",                  // precache
		"",                  // sounds
		{0,0,0,0}
	},

	{
		"weapon_mosin",
		"sound/misc/w_pkup.wav",
		{   "models/weapons/rifles/mosin/mosin_3rd.md3",
			"models/weapons/rifles/mosin/v_mosin.md3",
			"models/weapons/rifles/mosin/mosin_3rd.md3",
			0, 0 },

		"icons/iconw_mosin",     // icon
		"icons/ammo3",           // ammo icon
		"Mosin-Nagant",          // pickup
		50,
		IT_WEAPON,
		WP_MOSIN,
		WP_MOSIN,
		WP_MOSIN,
		WP_MOSIN,
		WP_MOSIN,
		"",                  // precache
		"",                  // sounds
		{0,0,0,0}
	},

	{
		"weapon_m1941",
		"sound/misc/w_pkup.wav",
		{   "models/weapons/auto_rifles/m1941/m1941_3rd.md3",
			"models/weapons/auto_rifles/m1941/v_m1941.md3",
			"models/weapons/auto_rifles/m1941/m1941_3rd.md3",
			0, 0 },

		"icons/iconw_m1941",     // icon
		"icons/ammo10",          // ammo icon
		"M1941",                 // pickup
		30,
		IT_WEAPON,
		WP_M1941,
		WP_M1941,
		WP_M1941,
		WP_M1941,
		WP_M1941,
		"",                  // precache
		"",                  // sounds
		{0,0,0,0}
	},

	{
		"weapon_silencer",
		"sound/misc/w_pkup.wav",
		{   "models/weapons/pistols/luger_silenced/lugers_3rd.md3", 
			"models/weapons/pistols/luger_silenced/v_lugers.md3",
			"models/weapons/pistols/luger_silenced/lugers_3rd.md3",
			0, 0},

		"icons/iconw_silencer_1",    // icon
		"icons/ammo5",       // ammo icon
		"sp5 pistol",
		10,
		IT_WEAPON,
		WP_SILENCER,
		WP_SILENCER,
		WP_LUGER,
		WP_SILENCER,
		WP_SILENCER,
		"",                  // precache
		"",                  // sounds
		{0,0,0,0}
	},

	{
		"weapon_panzerfaust",
		"sound/misc/w_pkup.wav",
		{   "models/weapons/heavy/pf/pf_3rd.md3",
			"models/weapons/heavy/pf/v_pf.md3",
			"models/weapons/heavy/pf/pf_3rd.md3",
			0, 0 },

		"icons/iconw_panzerfaust_1", // icon
		"icons/ammo6",       // ammo icon
		"Panzerfaust",               // pickup
		1,
		IT_WEAPON,
		WP_PANZERFAUST,
		WP_PANZERFAUST,
		WP_PANZERFAUST,
		WP_PANZERFAUST,
		WP_PANZERFAUST,
		"",                      // precache
		"",                      // sounds
		{0,0,0,0}
	},

	{
		"weapon_grenadelauncher",
		"sound/misc/w_pkup.wav",
		{   "models/weapons2/grenade/grenade.md3",
			"models/weapons2/grenade/v_grenade.md3",
			"models/weapons2/grenade/pu_grenade.md3",
			0, 0 },

		"icons/iconw_grenade_1", // icon
		"icons/icona_grenade",   // ammo icon
		"Grenade",               // pickup
		6,
		IT_WEAPON,
		WP_GRENADE_LAUNCHER,
		WP_GRENADE_LAUNCHER,
		WP_GRENADE_LAUNCHER,
		WP_GRENADE_LAUNCHER,
		WP_GRENADE_LAUNCHER,
		"",                      // precache
		"sound/weapons/grenade/hgrenb1a.wav sound/weapons/grenade/hgrenb2a.wav",             // sounds
		{0,0,0,0}
	},

	{
		"weapon_grenadepineapple",
		"sound/misc/w_pkup.wav",
		{   "models/weapons2/grenade/pineapple.md3",
			"models/weapons2/grenade/v_pineapple.md3",
			"models/weapons2/grenade/pu_pineapple.md3",
			0, 0 },

		"icons/iconw_pineapple_1",   // icon
		"icons/icona_pineapple", // ammo icon
		"Pineapple",             // pickup
		6,
		IT_WEAPON,
		WP_GRENADE_PINEAPPLE,
		WP_GRENADE_PINEAPPLE,
		WP_GRENADE_PINEAPPLE,
		WP_GRENADE_PINEAPPLE,
		WP_GRENADE_PINEAPPLE,
		"",                      // precache
		"sound/weapons/grenade/hgrenb1a.wav sound/weapons/grenade/hgrenb2a.wav",             // sounds
		{0,0,0,0}
	},

	{
		"weapon_dynamite",
		"sound/misc/w_pkup.wav",
		{   "models/weapons2/dynamite/dynamite.md3",
			"models/weapons2/dynamite/v_dynamite.md3",
			"models/weapons2/dynamite/pu_dynamite.md3",
			0, 0 },

		"icons/iconw_dynamite_1",    // icon
		"icons/ammo9",           // ammo icon
		"Dynamite Weapon",       // pickup
		7,
		IT_WEAPON,
		WP_DYNAMITE,
		WP_DYNAMITE,
		WP_DYNAMITE,
		WP_DYNAMITE,
		WP_DYNAMITE,
		"",                      // precache
		"",                      // sounds
		{0,0,0,0}
	},

	{
		"weapon_landmine",
		"sound/misc/w_pkup.wav",
		{   "models/multiplayer/landmine/landmine.md3",
			"models/multiplayer/landmine/landmine.md3",
			"models/multiplayer/landmine/landmine.md3",
			0, 0 },

		"icons/iconw_landmine",    // icon
		"icons/ammo9",           // ammo icon
		"Landmine",              // pickup
		1,
		IT_WEAPON,
		WP_LANDMINE,
		WP_LANDMINE,
		WP_LANDMINE,
		WP_LANDMINE,
		WP_LANDMINE,
		"",                      // precache
		"",                      // sounds
		{0,0,0,0}
	},

	{
		"weapon_medcrate",
		"sound/misc/w_pkup.wav",
		{   "models/powerups/health/health_l.md3",  // placeholder, stock large health pack model
			"models/powerups/health/health_l.md3",
			"models/powerups/health/health_l.md3",
			0, 0 },

		"icons/iconh_large",     // icon (placeholder)
		"icons/iconh_large",     // ammo icon (placeholder)
		"Medical Crate",         // pickup
		1,
		IT_WEAPON,
		WP_MEDCRATE,
		WP_MEDCRATE,
		WP_MEDCRATE,
		WP_MEDCRATE,
		WP_MEDCRATE,
		"",                      // precache
		"",                      // sounds
		{0,0,0,0}
	},

	{
		"weapon_airstrike",
		"sound/misc/w_pkup.wav",
		{ "", "", "", 0, 0 },

		"icons/iconw_airstrike_1",    // icon
		"icons/ammo9",           // ammo icon
		"Airstrike Signal",      // pickup
		1,
		IT_WEAPON,
		WP_AIRSTRIKE,
		WP_AIRSTRIKE,
		WP_AIRSTRIKE,
		WP_AIRSTRIKE,
		WP_AIRSTRIKE,
		"",                      // precache
		"sound/weapons/airstrike/throw.wav sound/weapons/airstrike/airstrike_01.wav",             // sounds
		{0,0,0,0}
	},

	{
		// JPW NERVE -- never spawned/picked up; fired via the LT binocular hook, exists only so item lookups resolve WP_ARTY.
		"weapon_arty",
		"sound/misc/w_pkup.wav",
		{ "", "", "", 0, 0 },

		"",                      // icon
		"icons/ammo9",           // ammo icon
		"Artillery",             // pickup
		1,
		IT_WEAPON,
		WP_ARTY,
		WP_ARTY,
		WP_ARTY,
		WP_ARTY,
		WP_ARTY,
		"",                      // precache
		"sound/multiplayer/allies/a-firing.wav sound/multiplayer/axis/g-firing.wav sound/multiplayer/allies/a-art_abort.wav sound/multiplayer/axis/g-art_abort.wav",             // sounds
		{0,0,0,0}
	},


	{
		"weapon_venom",
		"sound/misc/w_pkup.wav",
		{   "models/weapons/secret/venom/venom_3rd.md3",
			"models/weapons/secret/venom/v_venom.md3",
			"models/weapons/secret/venom/venom_3rd.md3",
			0, 0 },

		"icons/iconw_venom_1",   // icon
		"icons/ammo8",           // ammo icon
		"Venom",             // pickup
		700,
		IT_WEAPON,
		WP_VENOM,
		WP_VENOM,
		WP_VENOM,
		WP_VENOM,
		WP_VENOM,
		"",                      // precache
		"",                      // sounds
		{0,0,0,0}
	},

	{
		"weapon_flamethrower",
		"sound/misc/w_pkup.wav",
		{   "models/weapons/heavy/flame/flame_3rd.md3",
			"models/weapons/heavy/flame/v_flame.md3",
			"models/weapons/heavy/flame/flame_pickup.md3",
			0, 0 },

		"icons/iconw_flamethrower_1",    // icon
		"icons/ammo10",              // ammo icon
		"Flamethrower",              // pickup
		200,
		IT_WEAPON,
		WP_FLAMETHROWER,
		WP_FLAMETHROWER,
		WP_FLAMETHROWER,
		WP_FLAMETHROWER,
		WP_FLAMETHROWER,
		"",                          // precache
		"",                          // sounds
		{0,0,0,0}
	},

	{
		"weapon_tesla",
		"sound/misc/w_pkup.wav",

		{   "models/weapons/secret/tesla/tesla_3rd.md3",
			"models/weapons/secret/tesla/v_tesla.md3",
			"models/weapons/secret/tesla/tesla_pickup.md3",
			0, 0 },

		"icons/iconw_tesla_1",   // icon
		"icons/ammo10",              // ammo icon
		"Tesla Gun",             // pickup
		200,
		IT_WEAPON,
		WP_TESLA,
		WP_TESLA,
		WP_TESLA,
		WP_TESLA,
		WP_TESLA,
		"",                          // precache
		"",                          // sounds
		{0,0,0,0}
	},

	{
		"weapon_mg42m",
		"sound/misc/w_pkup.wav",
		{   "models/weapons/heavy/mg42/mg42_3rd.md3",
			"models/weapons/heavy/mg42/v_mg42.md3",
			"models/weapons/heavy/mg42/mg42_3rd.md3",
			0, 0 },

		"icons/iconw_mg42m",     // icon
		"icons/icona_machinegun", // ammo icon
		"MG42",                  // pickup
		500,
		IT_WEAPON,
		WP_MG42M,
		WP_MG42M,
		WP_MG42M,
		WP_MG42M,
		WP_MG42M,
		"",                          // precache
		"",                          // sounds
		{0,0,0,0}
	},

	{
		"weapon_browning",
		"sound/misc/w_pkup.wav",
		{   "models/weapons/heavy/browning/brown30cal_3rd.md3",
			"models/weapons/heavy/browning/v_brown30cal.md3",
			"models/weapons/heavy/browning/brown30cal_3rd.md3",
			0, 0 },

		"icons/iconw_browning",  // icon
		"icons/icona_machinegun", // ammo icon
		"Browning M1919",        // pickup
		500,
		IT_WEAPON,
		WP_BROWNING,
		WP_BROWNING,
		WP_BROWNING,
		WP_BROWNING,
		WP_BROWNING,
		"",                          // precache
		"",                          // sounds
		{0,0,0,0}
	},

	{
		"weapon_sniperScope",
		"sound/misc/w_pkup.wav",
		{   "models/weapons2/mauser/mauser.md3",
			"models/weapons2/mauser/v_mauser.md3",
//			"models/weapons2/mauser/v_mauser_scope.md3",
			"models/weapons2/mauser/pu_mauser_scope.md3",
			0, 0 },

//		"icons/iconw_sniper_1",	// icon
		"icons/iconw_mauser_1",  // icon
		"icons/ammo10",              // ammo icon
		"Sniper Scope",              // pickup
		200,
		IT_WEAPON,
		WP_SNIPERRIFLE,
		WP_SNIPERRIFLE,
		WP_MAUSER,
		WP_MAUSER,
		WP_MAUSER,
		"",                          // precache
		"",                          // sounds
		{0,0,0,0}
	},

	{
		"weapon_snooperrifle",   //----(SA)	modified
		"sound/misc/w_pkup.wav",
		{   "models/weapons/rifles/snooper/snooper_3rd.md3",
			"models/weapons/rifles/snooper/v_snooper.md3",
			"models/weapons/rifles/snooper/snooper_3rd.md3",
			0, 0 },

		"icons/iconw_garand_1",  // icon
		"icons/ammo10",              // ammo icon
		"Snooper Rifle",             // pickup		//----(SA)	modified
		20,
		IT_WEAPON,
		WP_SNOOPERSCOPE,
		WP_SNOOPERSCOPE,
		WP_SNOOPER,
		WP_SNOOPER,
		WP_SNOOPER,
		"",                          // precache
		"",                          // sounds
		{0,0,0,0}
	},

	{
		"weapon_fg42scope",  //----(SA)	modified
		"sound/misc/w_pkup.wav",
		{   "models/weapons2/fg42/fg42.md3",
			"models/weapons2/fg42/v_fg42.md3",
			"models/weapons2/fg42/pu_fg42.md3",
			0, 0},

		"icons/iconw_fg42_1",    // icon
		"icons/ammo5",               // ammo icon
		"FG42 Scope",                // pickup		//----(SA)	modified
		0,
		IT_WEAPON,
		WP_FG42SCOPE,
		WP_FG42SCOPE,   // this weap
		WP_MAUSER,      // shares ammo w/
		WP_FG42,        // shares ammo w/ (survival)
		WP_FG42,        // shares clip w/
		"",                          // precache
		"",                          // sounds
		{0,0,0,0}
	},

	{
		"weapon_m1941scope",
		"sound/misc/w_pkup.wav",
		{   "models/weapons/auto_rifles/m1941/m1941_scp_3rd.md3",
			"models/weapons/auto_rifles/m1941/v_m1941.md3",
			"models/weapons/auto_rifles/m1941/m1941_scp_3rd.md3",
			0, 0 },

		"icons/iconw_m1941scope",    // icon
		"icons/ammo10",              // ammo icon
		"M1941 Scope",               // pickup
		50,
		IT_WEAPON,
		WP_M1941SCOPE,  // this weap
		WP_M1941SCOPE,
		WP_M1941,       // shares ammo w/
		WP_M1941,       // shares ammo w/ (survival)
		WP_M1941,       // shares clip w/
		"",                          // precache
		"",                          // sounds
		{0,0,0,0}
	},

	{
		"weapon_delislescope",
		"sound/misc/w_pkup.wav",
		{   "models/weapons/rifles/delisle/delisle_3rd.md3",
			"models/weapons/rifles/delisle/v_delisle.md3",
			"models/weapons/rifles/delisle/delisle_3rd.md3",
			0, 0 },

		"icons/iconw_delislescope",  // icon
		"icons/ammo3",               // ammo icon
		"De Lisle Scope",            // pickup
		50,
		IT_WEAPON,
		WP_DELISLESCOPE,  // this weap
		WP_DELISLESCOPE,
		WP_DELISLE,       // shares ammo w/
		WP_DELISLE,       // shares ammo w/ (survival)
		WP_DELISLE,       // shares clip w/
		"",                          // precache
		"",                          // sounds
		{0,0,0,0}
	},


	{
		"weapon_monster_attack1",
		"",
		{   "",
			"",
			0, 0},
		"",  // icon
		NULL,   // ammo icon
		"MonsterAttack1",            // pickup
		100,
		IT_WEAPON,
		WP_MONSTER_ATTACK1,
		WP_MONSTER_ATTACK1,
		WP_MONSTER_ATTACK1,         // ammo
		WP_MONSTER_ATTACK1,         // ammo
		WP_MONSTER_ATTACK1,         //
		"",                          // precache
		"",                          // sounds
		{0,0,0,0}
	},
/*
weapon_monster_attack2 (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_monster_attack2",
		"",
		{   "",
			"",
			0, 0},
		"",  // icon
		NULL,   // ammo icon
		"MonsterAttack2",            // pickup
		100,
		IT_WEAPON,
		WP_MONSTER_ATTACK2,
		WP_MONSTER_ATTACK2,
		WP_MONSTER_ATTACK2,         // ammo
		WP_MONSTER_ATTACK2,         // ammo
		WP_MONSTER_ATTACK2,         //
		"",                          // precache
		"",                          // sounds
		{0,0,0,0}
	},

	{
		"weapon_monster_attack3",
		"",
		{   "",
			"",
			0, 0},
		"",  // icon
		NULL,   // ammo icon
		"MonsterAttack3",            // pickup
		100,
		IT_WEAPON,
		WP_MONSTER_ATTACK3,
		WP_MONSTER_ATTACK3,
		WP_MONSTER_ATTACK3,         // ammo
		WP_MONSTER_ATTACK3,         // ammo
		WP_MONSTER_ATTACK3,
		"",                          // precache
		"",                          // sounds
		{0,0,0,0}
	},


	{
		"weapon_mortar",
		"sound/misc/w_pkup.wav",
		{   "models/weapons2/grenade/grenade.md3",
			"models/weapons2/grenade/v_grenade.md3",
			"models/weapons2/grenade/pu_grenade.md3",
			0, 0},
		"icons/iconw_grenade_1", // icon
		"icons/icona_grenade",   // ammo icon
		"nopickup(WP_MORTAR)",       // pickup
		6,
		IT_WEAPON,
		WP_MORTAR,
		WP_MORTAR,
		WP_MORTAR,
		WP_MORTAR,
		WP_MORTAR,
		"",                      // precache
		"sound/weapons/mortar/mortarf1.wav",             // sounds
		{0,0,0,0}
	},



	//
	// AMMO ITEMS
	//

	{
		"ammo_9mm_small",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/am9mm_s.md3",
		  0, 0, 0, 0 },
		"icons/iconw_luger_1", // icon
		NULL,               // ammo icon
		"9mm Rounds",        // pickup
		30,
		IT_AMMO,
		WP_NONE,
		WP_LUGER,
		WP_LUGER,
		WP_LUGER,
		WP_LUGER,
		"",                  // precache
		"",                  // sounds
		{32,24,16,16}
	},

	{
		"ammo_9mm",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/am9mm_m.md3",
		  0, 0, 0,    0 },
		"icons/iconw_luger_1", // icon
		NULL,               // ammo icon
		"9mm",           // pickup			//----(SA)	changed
		60,
		IT_AMMO,
		WP_NONE,
		WP_LUGER,
		WP_LUGER,
		WP_LUGER,
		WP_LUGER,
		"",                  // precache
		"",                  // sounds
		{64,48,32,32}
	},

	{
		"ammo_9mm_large",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/am9mm_l.md3",
		  0, 0, 0,    0 },
		"icons/iconw_luger_1", // icon
		NULL,               // ammo icon
		"9mm Box",           // pickup
		100,
		IT_AMMO,
		WP_NONE,
		WP_LUGER,
		WP_LUGER,
		WP_LUGER,
		WP_LUGER,
		"",                  // precache
		"",                  // sounds
		{96,64,48,48}
	},


	{
		"ammo_45cal_small",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/am45cal_s.md3",
		  0, 0, 0,    0 },
		"icons/iconw_luger_1", // icon
		NULL,               // ammo icon
		".45cal Rounds", // pickup
		20,
		IT_AMMO,
		WP_NONE,
		WP_COLT,
		WP_COLT,
		WP_COLT,
		WP_COLT,
		"",                  // precache
		"",                  // sounds
		{30,20,15,15}
	},

	{
		"ammo_45cal",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/am45cal_m.md3",
		  0, 0, 0,    0 },
		"icons/iconw_luger_1", // icon
		NULL,               // ammo icon
		".45cal",        // pickup			//----(SA)	changed
		60,
		IT_AMMO,
		WP_NONE,
		WP_COLT,
		WP_COLT,
		WP_COLT,
		WP_COLT,
		"",                  // precache
		"",                  // sounds
		{60,45,30,30}
	},

	{
		"ammo_45cal_large",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/am45cal_l.md3",
		  0, 0, 0,    0 },
		"icons/iconw_luger_1", // icon
		NULL,               // ammo icon
		".45cal Box",        // pickup
		100,
		IT_AMMO,
		WP_NONE,
		WP_COLT,
		WP_COLT,
		WP_COLT,
		WP_COLT,
		"",                  // precache
		"",                  // sounds
		{90,60,45,45}
	},


	{
		"ammo_792mm_small",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/am792mm_s.md3",
		  0, 0, 0,    0 },
		"icons/icona_machinegun",    // icon
		NULL,                       // ammo icon
		"7.92mm Rounds",         // pickup
		50,
		IT_AMMO,
		WP_NONE,
		WP_MAUSER,
		WP_MAUSER,
		WP_MAUSER,
		WP_MAUSER,
		"",                          // precache
		"",                          // sounds
		{16,12,8,8}
	},

	{
		"ammo_792mm",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/am792mm_m.md3",
		  0, 0, 0,    0 },
		"icons/icona_machinegun",    // icon
		NULL,                       // ammo icon
		"7.92mm",                // pickup			//----(SA)	changed
		10,
		IT_AMMO,
		WP_NONE,
		WP_MAUSER,
		WP_MAUSER,
		WP_MAUSER,
		WP_MAUSER,
		"",                          // precache
		"",                          // sounds
		{32,24,16,16}
	},

	{
		"ammo_792mm_large",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/am792mm_l.md3",
		  0, 0, 0,    0 },
		"icons/icona_machinegun",    // icon
		NULL,                       // ammo icon
		"7.92mm Box",                // pickup
		50,
		IT_AMMO,
		WP_NONE,
		WP_MAUSER,
		WP_MAUSER,
		WP_MAUSER,
		WP_MAUSER,
		"",                          // precache
		"",                          // sounds
		{48,32,24,24}
	},


	{
		"ammo_30cal_small",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/am30cal_s.md3",
		  0, 0, 0,    0 },
		"icons/icona_machinegun",    // icon
		NULL,                       // ammo icon
		".30cal Rounds",         // pickup
		50,
		IT_AMMO,
		WP_NONE,
		WP_SNOOPER,
		WP_SNOOPER,
		WP_SNOOPER,
		WP_SNOOPER,
		"",                          // precache
		"",                          // sounds
		{5,2,2,2}
	},

	{
		"ammo_30cal",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/am30cal_m.md3",
		  0, 0, 0,    0 },
		"icons/icona_machinegun",    // icon
		NULL,                       // ammo icon
		".30cal",                // pickup			//----(SA)	changed
		50,
		IT_AMMO,
		WP_NONE,
		WP_SNOOPER,
		WP_SNOOPER,
		WP_SNOOPER,
		WP_SNOOPER,
		"",                          // precache
		"",                          // sounds
		{5,5,5,5    }
	},

	{
		"ammo_30cal_large",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/am30cal_l.md3",
		  0, 0, 0,    0 },
		"icons/icona_machinegun",    // icon
		NULL,                       // ammo icon
		".30cal Box",                // pickup
		50,
		IT_AMMO,
		WP_NONE,
		WP_SNOOPER,
		WP_SNOOPER,
		WP_SNOOPER,
		WP_SNOOPER,
		"",                          // precache
		"",                          // sounds
		{10,10,10,5}
	},

	{
		"ammo_127mm",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/am127mm.md3",
		  0, 0, 0,    0 },
		"icons/icona_machinegun",    // icon
		NULL,                       // ammo icon
		"12.7mm",                    // pickup
		100,
		IT_AMMO,
		WP_NONE,
		WP_VENOM,
		WP_VENOM,
		WP_VENOM,
		WP_VENOM,
		"",                          // precache
		"",                          // sounds
		{100,75,50,50}
	},

	{
		"ammo_grenades",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/amgren_bag.md3",
		  0, 0, 0,    0 },
		"icons/icona_grenade",   // icon
		NULL,                   // ammo icon
		"Grenades",              // pickup
		5,
		IT_AMMO,
		WP_NONE,
		WP_GRENADE_LAUNCHER,
		WP_GRENADE_LAUNCHER,
		WP_GRENADE_LAUNCHER,
		WP_GRENADE_LAUNCHER,
		"",                      // precache
		"",                      // sounds
		{5,4,3,3}
	},

	{
		"ammo_grenades_american",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/amgrenus_bag.md3",
		  0, 0, 0,    0 },
		"icons/icona_pineapple", // icon
		NULL,                   // ammo icon
		"Pineapples",            // pickup
		5,
		IT_AMMO,
		WP_NONE,
		WP_GRENADE_PINEAPPLE,
		WP_GRENADE_PINEAPPLE,
		WP_GRENADE_PINEAPPLE,
		WP_GRENADE_PINEAPPLE,
		"",                      // precache
		"",                      // sounds
		{5,4,3,3}
	},

	{
		"ammo_dynamite",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/dynamite.md3",
		  0, 0, 0,    0 },
		"icons/icona_dynamite",  // icon
		NULL,                   // ammo icon
		"Dynamite",              // pickup
		1,
		IT_AMMO,
		WP_NONE,
		WP_DYNAMITE,
		WP_DYNAMITE,
		WP_DYNAMITE,
		WP_DYNAMITE,
		"",                      // precache
		"",                      // sounds
		{1,1,1,1}
	},

	{
		"ammo_landmine",
		"sound/misc/am_pkup.wav",
		{ "models/multiplayer/landmine/landmine.md3",
		  0, 0, 0,    0 },
		"icons/icona_landmine",  // icon
		NULL,                   // ammo icon
		"Landmine",              // pickup
		1,
		IT_AMMO,
		WP_NONE,
		WP_LANDMINE,
		WP_LANDMINE,
		WP_LANDMINE,
		WP_LANDMINE,
		"",                      // precache
		"",                      // sounds
		{1,1,1,1}
	},


	{
		"ammo_cell",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/amcell.md3",
		  0, 0, 0,    0 },
		"icons/icona_cell",  // icon
		NULL,               // ammo icon
		"Cell",              // pickup
		500,
		IT_AMMO,
		WP_NONE,
		WP_TESLA,
		WP_TESLA,
		WP_TESLA,
		WP_TESLA,
		"",                  // precache
		"",                  // sounds
		{100,75,50,50}
	},


	{
		"ammo_fuel",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/amfuel.md3",
		  0, 0, 0,    0 },
		"icons/icona_fuel",  // icon
		NULL,               // ammo icon
		"Fuel",              // pickup
		100,
		IT_AMMO,
		WP_NONE,
		WP_FLAMETHROWER,
		WP_FLAMETHROWER,
		WP_FLAMETHROWER,
		WP_FLAMETHROWER,
		"",                  // precache
		"",                  // sounds
		{100,75,50,50}
	},


	{
		"ammo_panzerfaust",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/ampf.md3",
		  0, 0, 0,    0 },
		"icons/icona_panzerfaust",   // icon
		NULL,                   // ammo icon
		"Panzerfaust Rockets",               // pickup
		5,
		IT_AMMO,
		WP_NONE,
		WP_PANZERFAUST,
		WP_PANZERFAUST,
		WP_PANZERFAUST,
		WP_PANZERFAUST,
		"",                      // precache
		"",                      // sounds
		{4,3,2,2}
	},

	{
		"ammo_monster_attack1",
		"",
		{ "",
		  0, 0, 0},
		"",                      // icon
		NULL,                   // ammo icon
		"MonsterAttack1",        // pickup
		60,
		IT_AMMO,
		WP_NONE,
		WP_MONSTER_ATTACK1,
		WP_MONSTER_ATTACK1,
		WP_MONSTER_ATTACK1,
		WP_MONSTER_ATTACK1,
		"",
		"",
		{0,0,0,0}
	},


	//
	// HOLDABLE ITEMS
	//


/*QUAKED holdable_wine (.3 .3 1) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN

pickup sound : "sound/pickup/holdable/get_wine.wav"
use sound : "sound/pickup/holdable/use_wine.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/holdable/wine.md3"
*/
	{
		"holdable_wine",
		"sound/pickup/holdable/get_wine.wav",
		{
			"models/powerups/holdable/wine.md3",
			0, 0, 0,    0
		},
		"icons/wine",                    // icon
		NULL,                           // ammo icon
		"1921 Chateau Lafite",           // pickup
		1,
		IT_HOLDABLE,
		WP_NONE,
		HI_WINE,
		0,
		0,
		0,
		"",                              // precache
		"sound/pickup/holdable/use_wine.wav",        // sounds
		{3,0,0,0}
	},

/*QUAKED holdable_adrenaline (.3 .3 1) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
Protection from fatigue plus a health boost. Consumed on use.

pickup sound : "sound/pickup/holdable/get_adrenaline.wav"
use sound : "sound/pickup/holdable/use_adrenaline.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/holdable/adrenaline.md3"
*/
	{
		"holdable_adrenaline",
		"sound/pickup/holdable/get_adrenaline.wav",
		{
			"models/powerups/holdable/adrenaline.md3",
			0, 0, 0,    0
		},
		"icons/adrenaline",             // icon
		NULL,                           // ammo icon
		"Adrenaline",                   // pickup
		1,
		IT_HOLDABLE,
		WP_NONE,
		HI_ADRENALINE,
		0,
		0,
		0,
		"",                              // precache
		"sound/pickup/holdable/use_adrenaline.wav",  // sounds
		{1,0,0,0}
	},

	{
		"holdable_stamina",
		"sound/pickup/holdable/get_stamina.wav",
		{
			"models/powerups/holdable/stamina.md3",
			0, 0, 0
			,   0
		},
		"icons/stamina",             // icon
		NULL,                           // ammo icon
		"Added Stamina",             // pickup
		1,
		IT_HOLDABLE,
		WP_NONE,
		HI_STAMINA,
		0,
		0,
		0,
		"",                              // precache
		"sound/pickup/holdable/use_stamina.wav", // sounds
		{0,0,0,0}
	},

	{
		"holdable_book1",
		"sound/pickup/holdable/get_book1.wav",
		{
			"models/powerups/holdable/venom_book.md3",
			0, 0, 0
			,   0
		},
		"icons/icon_vbook",              // icon
		NULL,                       // ammo icon
		"Venom Tech Manual",     // pickup
		1,
		IT_HOLDABLE,
		WP_NONE,
		HI_BOOK1,
		0,
		0,
		0,
		"",                              // precache
		"sound/pickup/holdable/use_book.wav",    // sounds
		{0,0,0,0}
	},

	{
		"holdable_book2",
		"sound/pickup/holdable/get_book2.wav",
		{
			"models/powerups/holdable/paranormal_book.md3",
			0, 0, 0
			,   0
		},
		"icons/icon_pbook",              // icon
		NULL,                           // ammo icon
		"Project Book",                  // pickup
		1,
		IT_HOLDABLE,
		WP_NONE,
		HI_BOOK2,
		0,
		0,
		0,
		"",                              // precache
		"sound/pickup/holdable/use_book.wav",    // sounds
		{0,0,0,0}
	},


	{
		"holdable_book3",
		"sound/pickup/holdable/get_book3.wav",
		{
			"models/powerups/holdable/zemphr_book.md3",
			0, 0, 0
			,   0
		},
		"icons/icon_zbook",              // icon
		NULL,                       // ammo icon
		"Dr. Zemph's Journal",       // pickup
		1,
		IT_HOLDABLE,
		WP_NONE,
		HI_BOOK3,
		0,
		0,
		0,
		"",                              // precache
		"sound/pickup/holdable/use_book.wav",    // sounds
		{0,0,0,0}
	},

/*QUAKED holdable_cross (.3 .3 1) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
Tides of War cross. Ignites and finishes off nearby undead. Consumed on use.

pickup sound : "sound/pickup/holdable/get_cross.wav"
use sound : "sound/pickup/holdable/use_cross.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/holdable/cross.md3"
*/
	{
		"holdable_cross",
		"sound/pickup/holdable/get_cross.wav",
		{
			"models/powerups/holdable/cross.md3",
			0, 0, 0,    0
		},
		"icons/holycross",              // icon
		NULL,                           // ammo icon
		"Holy Cross",                   // pickup
		1,
		IT_HOLDABLE,
		WP_NONE,
		HI_CROSS,
		0,
		0,
		0,
		"",                              // precache
		"sound/pickup/holdable/use_cross.wav",   // sounds
		{1,0,0,0}
	},

/*QUAKED holdable_emp (.3 .3 1) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
EMP device. Disables nearby X-creatures for a short time. Consumed on use.

pickup sound : "sound/pickup/holdable/get_emp.wav"
use sound : "sound/pickup/holdable/use_emp.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/holdable/emp.md3"
*/
	{
		"holdable_emp",
		"sound/pickup/holdable/get_emp.wav",
		{
			"models/powerups/holdable/emp.md3",
			0, 0, 0,    0
		},
		"icons/emp",                    // icon
		NULL,                           // ammo icon
		"EMP Device",                   // pickup
		1,
		IT_HOLDABLE,
		WP_NONE,
		HI_EMP,
		0,
		0,
		0,
		"",                              // precache
		"sound/pickup/holdable/use_emp.wav",     // sounds
		{1,0,0,0}
	},

/*QUAKED holdable_xshield (.3 .3 1) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
Shield generator. Grants brief full damage immunity. Consumed on use.

pickup sound : "sound/pickup/holdable/get_shield.wav"
use sound : "sound/pickup/holdable/use_shield.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/holdable/shieldgen.md3"
*/
	{
		"holdable_xshield",
		"sound/pickup/holdable/get_shield.wav",
		{
			"models/powerups/holdable/shieldgen.md3",
			0, 0, 0,    0
		},
		"icons/xshield",                // icon
		NULL,                           // ammo icon
		"Shield Generator",             // pickup
		1,
		IT_HOLDABLE,
		WP_NONE,
		HI_XSHIELD,
		0,
		0,
		0,
		"",                              // precache
		"sound/pickup/holdable/use_shield.wav",  // sounds
		{1,0,0,0}
	},




	//
	// POWERUP ITEMS
	//


/*QUAKED item_ammopw (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
		{
		"item_ammopw",
		"sound/misc/powerup_resupply.wav",
		{
		"models/powerups/survival/thule_gr.md3",
		0, 
		0
		},
		"", 
		NULL,                       // ammo icon             
		"Veil Ressuply",       
		1,
		IT_POWERUP,
		WP_NONE,
		PW_AMMO,
		0,
		0,
		0,
		"",                            
		"",   
		{0,0,0,0}
	},

		/*QUAKED item_enviro (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
		{
		"item_enviro_surv",
		"sound/misc/powerup_shield.wav",
		{
		"models/powerups/survival/thule_g.md3",
		0, 
		0
		},
		"",            
		NULL,                       // ammo icon                   
		"Veil Shield",     
		30,
		IT_POWERUP,
		WP_NONE,
		PW_BATTLESUIT_SURV,
		0,
		0,
		0,
		"",                          
		"sound/items/airout.wav sound/items/protect3.wav",   
		{0,0,0,0}
	},

/*QUAKED item_quad (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
		{
		"item_quad",
		"sound/misc/powerup_quad.wav",
		{
		"models/powerups/survival/thule_b.md3",
		0,
		0
		},
		"",
		NULL,                       // ammo icon
		"Veil Empower",
		30,
		IT_POWERUP,
		WP_NONE,
		PW_QUAD,
		0,
		0,
		0,
		"",
		"sound/items/damage2.wav sound/items/damage3.wav",
		{0,0,0,0}
	},

/*QUAKED item_vampire (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
		{
		"item_vampire",
		"sound/misc/powerup_vampirism.wav",
		{
		"models/powerups/survival/thule_r.md3",
		0,
		0
		},
		"",
		NULL,                       // ammo icon
		"Veil Essence Reaver",
		30,
		IT_POWERUP,
		WP_NONE,
		PW_VAMPIRE,
		0,
		0,
		0,
		"",
		"",
		{0,0,0,0}
	},

/*QUAKED item_venompw (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
		{
		"item_venompw",
		"sound/misc/w_pkup.wav",
		{
		"models/weapons/secret/venom/venom_3rd.md3",
		0,
		0
		},
		"",
		NULL,                       // ammo icon
		"Venom",
		30,
		IT_POWERUP,
		WP_NONE,
		PW_VENOM,
		0,
		0,
		0,
		"",
		"",
		{0,0,0,0}
	},

/*QUAKED item_powerup_random (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
Spawns as a random powerup (Quad/Shield/Vampire/Resupply). Rerolls a new random type every time it respawns.
*/
		{
		"item_powerup_random",
		"",
		{
		0,
		0,
		0
		},
		"",
		NULL,                       // ammo icon
		"",
		0,
		IT_POWERUP,
		WP_NONE,
		PW_NONE,
		0,
		0,
		0,
		"",
		"",
		{0,0,0,0}
	},

	{
		"key_binocs",
		"sound/pickup/keys/binocs.wav",
		{
			"models/powerups/keys/binoculars.md3",
			0, 0, 0
			,   0
		},
		"icons/binocs",          // icon
		NULL,                   // ammo icon
		"Binoculars",            // pickup
		0,
		IT_KEY,
		WP_NONE,
		INV_BINOCS,
		0,
		0,
		0,
		"",                      // precache
		"models/keys/key.wav",   // sounds
		{0,0,0,0}
	},



/*QUAKED perk_resilience(.3 .3 1) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
Protection from fatigue
Using the "sprint" key will not fatigue the character

pickup sound : "sound/pickup/holdable/get_bandages.wav"
use sound : "sound/pickup/holdable/use_bandages.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/holdable/bandages.md3"
*/
	{
		"perk_resilience",
		"sound/pickup/holdable/get_bandages.wav",
		{
		"models/powerups/holdable/bandages.md3",
		0, 
		0
		},

		"icons/perk_regen",   
		NULL,                    
		"Resilience",             
		1,
		IT_PERK,
		WP_NONE,
		PERK_RESILIENCE,
		0,
		0,
		0,
		"",                             
		"",
		{0,0,0,0}
	},

/*QUAKED perk_scavenger(.3 .3 1) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
Protection from fatigue
Using the "sprint" key will not fatigue the character

pickup sound : "sound/pickup/holdable/get_bandages.wav"
use sound : "sound/pickup/holdable/use_bandages.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/holdable/bandages.md3"
*/
	{
		"perk_scavenger",
		"sound/pickup/holdable/get_bandages.wav",
		{
		"models/powerups/holdable/bandages.md3",
		0, 
		0
		},

		"icons/perk_scavenger",   
		NULL,                   // ammo icon          
		"Scavenger",             
		1,
		IT_PERK,
		WP_NONE,
		PERK_SCAVENGER,
		0,
		0,
		0,
		"",                             
		"",
		{0,0,0,0}
	},


/*QUAKED perk_runner(.3 .3 1) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
Protection from fatigue
Using the "sprint" key will not fatigue the character

pickup sound : "sound/pickup/holdable/get_bandages.wav"
use sound : "sound/pickup/holdable/use_bandages.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/holdable/bandages.md3"
*/
	{
		"perk_runner",
		"sound/pickup/holdable/get_bandages.wav",
		{
		"models/powerups/holdable/bandages.md3",
		0, 
		0
		},

		"icons/perk_runner",             
		NULL,                   // ammo icon
		"Runner",             
		1,
		IT_PERK,
		WP_NONE,
		PERK_RUNNER,
		0,
		0,
		0,
		"",                             
		"",
		{0,0,0,0}
	},


/*QUAKED perk_weaponhandling(.3 .3 1) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
Protection from fatigue
Using the "sprint" key will not fatigue the character

pickup sound : "sound/pickup/holdable/get_bandages.wav"
use sound : "sound/pickup/holdable/use_bandages.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/holdable/bandages.md3"
*/
	{
		"perk_weaponhandling",
		"sound/pickup/holdable/get_bandages.wav",
		{
		"models/powerups/holdable/bandages.md3",
		0, 
		0
		},

		"icons/perk_weaponhandling",    
		NULL,                   // ammo icon         
		"Weapon Handling",             
		1,
		IT_PERK,
		WP_NONE,
		PERK_WEAPONHANDLING,
		0,
		0,
		0,
		"",                             
		"",
		{0,0,0,0}
	},


/*QUAKED perk_rifling(.3 .3 1) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
Protection from fatigue
Using the "sprint" key will not fatigue the character

pickup sound : "sound/pickup/holdable/get_bandages.wav"
use sound : "sound/pickup/holdable/use_bandages.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/holdable/bandages.md3"
*/
	{
		"perk_rifling",
		"sound/pickup/holdable/get_bandages.wav",
		{
		"models/powerups/holdable/bandages.md3",
		0, 
		0
		},

		"icons/perk_rifling",        
		NULL,                   // ammo icon     
		"Advanced Rifling",             
		1,
		IT_PERK,
		WP_NONE,
		PERK_RIFLING,
		0,
		0,
		0,
		"",                             
		"",
		{0,0,0,0}
	},


/*QUAKED perk_secondchance(.3 .3 1) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
Protection from fatigue
Using the "sprint" key will not fatigue the character

pickup sound : "sound/pickup/holdable/get_bandages.wav"
use sound : "sound/pickup/holdable/use_bandages.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/holdable/bandages.md3"
*/
	{
		"perk_secondchance",
		"sound/pickup/holdable/get_bandages.wav",
		{
		"models/powerups/holdable/bandages.md3",
		0, 
		0
		},

		"icons/perk_secondchance",    
		NULL,                   // ammo icon         
		"Second Chance",             
		1,
		IT_PERK,
		WP_NONE,
		PERK_SECONDCHANCE,
		0,
		0,
		0,
		"",                             
		"",
		{0,0,0,0}
	},


	// end of list marker
	{NULL}
};
// END JOSEPH

int	bg_numItems = ARRAY_LEN( bg_itemlist ) - 1;


/*
==============
BG_FindItemForPowerup
==============
*/
gitem_t *BG_FindItemForPowerup( powerup_t pw ) {
	int i;

	for ( i = 0 ; i < bg_numItems ; i++ ) {
		if ( ( bg_itemlist[i].giType == IT_POWERUP ||
			   bg_itemlist[i].giType == IT_TEAM ) &&
			 bg_itemlist[i].giTag == pw ) {
			return &bg_itemlist[i];
		}
	}

	return NULL;
}


/*
==============
BG_FindItemForHoldable
==============
*/
gitem_t *BG_FindItemForHoldable( holdable_t pw ) {
	int i;

	for ( i = 0 ; i < bg_numItems ; i++ ) {
		if ( bg_itemlist[i].giType == IT_HOLDABLE && bg_itemlist[i].giTag == pw ) {
			return &bg_itemlist[i];
		}
	}

//	Com_Error( ERR_DROP, "HoldableItem not found" );

	return NULL;
}

/*
==============
BG_FindItemForPerk
==============
*/
gitem_t *BG_FindItemForPerk( perk_t perk ) {
	int i;

	for ( i = 0 ; i < bg_numItems ; i++ ) {
		if ( bg_itemlist[i].giType == IT_PERK && bg_itemlist[i].giTag == perk ) {
			return &bg_itemlist[i];
		}
	}

	return NULL;
}


/*
===============
BG_FindItemForWeapon

===============
*/
gitem_t *BG_FindItemForWeapon( weapon_t weapon ) {
	gitem_t *it;
	int i;
	const int NUM_TABLE_ELEMENTS = WP_NUM_WEAPONS;
	static gitem_t  *lookupTable[WP_NUM_WEAPONS];
	static qboolean lookupTableInit = qtrue;

	if ( lookupTableInit ) {
		for ( i = 0; i < NUM_TABLE_ELEMENTS; i++ ) {
			lookupTable[i] = 0; // default value for no match found
			for ( it = bg_itemlist + 1 ; it->classname ; it++ ) {
				if ( it->giType == IT_WEAPON && it->giTag == i ) {
					lookupTable[i] = it;
				}
			}
		}
		// table is created
		lookupTableInit = qfalse;
	}

	if ( weapon > NUM_TABLE_ELEMENTS ) {
		Com_Error( ERR_DROP, "BG_FindItemForWeapon: weapon out of range %i", weapon );
	}

	if ( !lookupTable[weapon] ) {
		Com_Error( ERR_DROP, "Couldn't find item for weapon %i", weapon );
	}

	// get the weapon from the lookup table
	return lookupTable[weapon];
}

//----(SA) added

#define DEATHMATCH_SHARED_AMMO 0


/*
==============
BG_FindClipForWeapon
==============
*/
weapon_t BG_FindClipForWeapon( weapon_t weapon ) {
	gitem_t *it;
	int i;
	const int NUM_TABLE_ELEMENTS = WP_NUM_WEAPONS;
	static weapon_t lookupTable[WP_NUM_WEAPONS];
	static qboolean lookupTableInit = qtrue;

	if ( lookupTableInit ) {
		for ( i = 0; i < NUM_TABLE_ELEMENTS; i++ ) {
			lookupTable[i] = 0; // default value for no match found
			for ( it = bg_itemlist + 1 ; it->classname ; it++ ) {
				if ( it->giType == IT_WEAPON && it->giTag == i ) {
					lookupTable[i] = it->giClipIndex;
				}
			}
		}
		// table is created
		lookupTableInit = qfalse;
	}

	if ( weapon > NUM_TABLE_ELEMENTS ) {
		Com_Error( ERR_DROP, "BG_FindClipForWeapon: weapon out of range %i", weapon );
	}

	// get the weapon from the lookup table
	return lookupTable[weapon];
}



/*
==============
BG_FindAmmoForWeapon
==============
*/
weapon_t BG_FindAmmoForWeapon( weapon_t weapon ) {
	gitem_t *it;
	int i;
	const int NUM_TABLE_ELEMENTS = WP_NUM_WEAPONS;
	static weapon_t lookupTable[WP_NUM_WEAPONS];
	static qboolean lookupTableInit = qtrue;
	qboolean survival = qfalse;

    #ifdef GAMEDLL
	    if (g_gametype.integer == GT_COOP_SURVIVAL)
    #endif
    #ifdef CGAMEDLL
		if (cg_gameType.integer == GT_COOP_SURVIVAL)
    #endif
			survival = qtrue;

    if (survival) {
	if ( lookupTableInit ) {
		for ( i = 0; i < NUM_TABLE_ELEMENTS; i++ ) {
			lookupTable[i] = 0; // default value for no match found
			for ( it = bg_itemlist + 1 ; it->classname ; it++ ) {
				if ( it->giType == IT_WEAPON && it->giTag == i ) {
					lookupTable[i] = it->giAmmoIndexSurv;
				}
			}
		}
		// table is created
		lookupTableInit = qfalse;
	}
	} else {
	if ( lookupTableInit ) {
		for ( i = 0; i < NUM_TABLE_ELEMENTS; i++ ) {
			lookupTable[i] = 0; // default value for no match found
			for ( it = bg_itemlist + 1 ; it->classname ; it++ ) {
				if ( it->giType == IT_WEAPON && it->giTag == i ) {
					lookupTable[i] = it->giAmmoIndex;
				}
			}
		}
		// table is created
		lookupTableInit = qfalse;
	}
	}

	if ( weapon > NUM_TABLE_ELEMENTS ) {
		Com_Error( ERR_DROP, "BG_FindAmmoForWeapon: weapon out of range %i", weapon );
	}

	// get the weapon from the lookup table
	return lookupTable[weapon];
}

/*
==============
BG_AkimboFireSequence
	returns 'true' if it's the left hand's turn to fire, 'false' if it's the right hand's turn
==============
*/
//qboolean BG_AkimboFireSequence( playerState_t *ps ) {
qboolean BG_AkimboFireSequence( int weapon, int akimboClip ) {
	// NOTE: this doesn't work when clips are turned off (dmflags 64)

	if ( weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS || !( GetWeaponTableData( weapon )->weaponClass & WEAPON_CLASS_AKIMBO ) ) {
		return qfalse;
	}

	return (qboolean)( akimboClip & 1 );
}

//----(SA) end

//----(SA) Added keys
/*
==============
BG_FindItemForKey
==============
*/
gitem_t *BG_FindItemForKey( wkey_t k, int *indexreturn ) {
	int i;

	for ( i = 0 ; i < bg_numItems ; i++ ) {
		if ( bg_itemlist[i].giType == IT_KEY && bg_itemlist[i].giTag == k ) {
			{
				if ( indexreturn ) {
					*indexreturn = i;
				}
				return &bg_itemlist[i];
			}
		}
	}

	Com_Error( ERR_DROP, "Key %d not found", k );
	return NULL;
}
//----(SA) end


//----(SA) added
/*
==============
BG_FindItemForAmmo
==============
*/
gitem_t *BG_FindItemForAmmo( int ammo ) {
	int i = 0;
	qboolean survival = qfalse;

	#ifdef GAMEDLL
		if (g_gametype.integer == GT_COOP_SURVIVAL)
	#endif
	#ifdef CGAMEDLL
		if (cg_gameType.integer == GT_COOP_SURVIVAL)
	#endif
			survival = qtrue;

	for (; i < bg_numItems; i++ )
	{
		if ( bg_itemlist[i].giType == IT_AMMO ) {
			if (survival) {
				if ( bg_itemlist[i].giAmmoIndexSurv == ammo ) {
					return &bg_itemlist[i];
				}
			} else {
				if ( bg_itemlist[i].giAmmoIndex == ammo ) {
					return &bg_itemlist[i];
				}
			}
		}
	}
	Com_Error( ERR_DROP, "Item not found for ammo: %d", ammo );
	return NULL;
}
//----(SA) end


/*
===============
BG_FindItem

===============
*/
gitem_t *BG_FindItem( const char *pickupName ) {
	gitem_t *it;

	for ( it = bg_itemlist + 1 ; it->classname ; it++ ) {
		if ( !Q_stricmp( it->pickup_name, pickupName ) ) {
			return it;
		}
	}

	return NULL;
}

gitem_t *BG_FindItemForClassName( const char *className ) {
	gitem_t *it;

	for ( it = bg_itemlist + 1 ; it->classname ; it++ ) {
		if ( !Q_stricmp( it->classname, className ) ) {
			return it;
		}
	}

	return NULL;
}


/*
==============
BG_FindItem2
	also check classname
==============
*/
gitem_t *BG_FindItem2( const char *name ) {
	gitem_t *it;
	char *name2;

	name2 = (char*)name;

	for ( it = bg_itemlist + 1 ; it->classname ; it++ ) {
		if ( !Q_stricmp( it->pickup_name, name ) ) {
			return it;
		}

		if ( !Q_strcasecmp( it->classname, name2 ) ) {
			return it;
		}
	}

	Com_Printf( "BG_FindItem2(): unable to locate item '%s'\n", name );

	return NULL;
}

//----(SA)	added
/*
==============
BG_PlayerSeesItem
	Try to quickly determine if an item should be highlighted as per the current cg_drawCrosshairPickups.integer value.
	pvs check should have already been done by the time we get in here, so we shouldn't have to check
==============
*/

//----(SA)	not used
/*
qboolean BG_PlayerSeesItem(playerState_t *ps, entityState_t *item, int atTime)
{
   vec3_t	vorigin, eorigin, viewa, dir;
   float	dot, dist, foo;

   BG_EvaluateTrajectory( &item->pos, atTime, eorigin );

   VectorCopy(ps->origin, vorigin);
   vorigin[2] += ps->viewheight;			// get the view loc up to the viewheight
   eorigin[2] += 16;						// and subtract the item's offset (that is used to place it on the ground)
   VectorSubtract(vorigin, eorigin, dir);

   dist = VectorNormalize(dir);			// dir is now the direction from the item to the player

   if(dist > 255)
	   return qfalse;						// only run the remaining stuff on items that are close enough

   // (SA) FIXME: do this without AngleVectors.
   //		It'd be nice if the angle vectors for the player
   //		have already been figured at this point and I can
   //		just pick them up.  (if anybody is storing this somewhere,
   //		for the current frame please let me know so I don't
   //		have to do redundant calcs)
   AngleVectors(ps->viewangles, viewa, 0, 0);
   dot = DotProduct(viewa, dir );

   // give more range based on distance (the hit area is wider when closer)

   foo = -0.94f - (dist/255.0f) * 0.057f;	// (ranging from -0.94 to -0.997) (it happened to be a pretty good range)

//	Com_Printf("test: if(%f > %f) return qfalse (dot > foo)\n", dot, foo);
   if(dot > foo)
	   return qfalse;

   return qtrue;
}
*/
//----(SA)	end


/*
============
BG_PlayerTouchesItem

Items can be picked up without actually touching their physical bounds to make
grabbing them easier
============
*/

extern int trap_Cvar_VariableIntegerValue( const char *var_name );

qboolean    BG_PlayerTouchesItem( playerState_t *ps, entityState_t *item, int atTime ) {
	vec3_t origin;

	BG_EvaluateTrajectory( &item->pos, atTime, origin );

	// we are ignoring ducked differences here
	if ( ps->origin[0] - origin[0] > 44
		 || ps->origin[0] - origin[0] < -50
		 || ps->origin[1] - origin[1] > 36
		 || ps->origin[1] - origin[1] < -36
		 || ps->origin[2] - origin[2] > 36
		 || ps->origin[2] - origin[2] < -36 ) {
		return qfalse;
	}

	return qtrue;
}



#define AMMOFORWEAP BG_FindAmmoForWeapon( item->giTag )
/*
================
BG_CanItemBeGrabbed

Returns false if the item should not be picked up.
This needs to be the same for client side prediction and server use.
================
*/

qboolean isClipOnly( int weap ) {
	switch ( weap ) {
	case WP_GRENADE_LAUNCHER:
	case WP_GRENADE_PINEAPPLE:
	case WP_DYNAMITE:
	case WP_LANDMINE:
	case WP_MEDCRATE:
	case WP_TESLA:
	case WP_FLAMETHROWER:
		return qtrue;
	}
	return qfalse;
}


qboolean    BG_CanItemBeGrabbed( const entityState_t *ent, const playerState_t *ps ) {
	gitem_t *item;
	int ammoweap;
	qboolean multiplayer = qfalse;
	qboolean survival = qfalse;

		#ifdef GAMEDLL
			if (g_gametype.integer == GT_COOP_SURVIVAL)
		#endif
		#ifdef CGAMEDLL
			if (cg_gameType.integer == GT_COOP_SURVIVAL)
		#endif
			survival = qtrue;

	if (ent->modelindex < 1 || ent->modelindex >= bg_numItems)
	{
		Com_Error(ERR_DROP, "BG_CanItemBeGrabbed: index out of range");
	}

	item = &bg_itemlist[ent->modelindex];

	switch ( item->giType ) {

	case IT_WEAPON:
		if (multiplayer)
		{
			if ((ps->stats[STAT_PLAYER_CLASS] == PC_MEDIC) || (ps->stats[STAT_PLAYER_CLASS] == PC_ENGINEER))
			{
				if (!COM_BitCheck(ps->weapons, item->giTag))
				{
					return qfalse;
				}
			}
		}
		else
		{
			if (COM_BitCheck(ps->weapons, item->giTag))
			{
				if (isClipOnly(item->giTag))
				{
					int maxclip = BG_GetMaxClip(ps, item->giTag);

					if (survival)
					{
						if (ps->ammoclip[item->giAmmoIndexSurv] >= maxclip)
						{
							return qfalse;
						}
					}
					else
					{
						if (ps->ammoclip[item->giAmmoIndex] >= maxclip)
						{
							return qfalse;
						}
					}
				}
				else
				{
					int maxammo = BG_GetMaxAmmo(ps, item->giTag, 1.5f);

					if (survival)
					{
						if (ps->ammo[item->giAmmoIndexSurv] >= maxammo)
						{
							return qfalse;
						}
					}
					else
					{
						if (ps->ammo[item->giAmmoIndex] >= maxammo)
						{
							return qfalse;
						}
					}
				}
			}
		}
		return qtrue;

	case IT_AMMO:
		ammoweap = BG_FindAmmoForWeapon(item->giTag);

		if (isClipOnly(ammoweap))
		{
			int maxclip = BG_GetMaxClip(ps, ammoweap);

			if (ps->ammoclip[ammoweap] >= maxclip)
			{
				return qfalse;
			}
		}

		int maxammo = BG_GetMaxAmmo(ps, ammoweap, 1.5f);
		if (ps->ammo[ammoweap] >= maxammo)
		{
			return qfalse;
		}

		return qtrue;
	case IT_ARMOR:
		// we also clamp armor to the maxhealth for handicapping
//			if ( ps->stats[STAT_ARMOR] >= ps->stats[STAT_MAX_HEALTH] * 2 ) {
		if ( ps->stats[STAT_ARMOR] >= 100 ) {
			return qfalse;
		}
		return qtrue;

	case IT_HEALTH:
		if ( ent->density == ( 1 << 9 ) ) { // density tracks how many uses left
			return qfalse;
		}

		if ( ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH] ) {
			return qfalse;
		}
		return qtrue;

	case IT_POWERUP:
		if ( ent->density == ( 1 << 9 ) ) { // density tracks how many uses left
			return qfalse;
		}

		if ( ps->powerups[PW_NOFATIGUE] == 60000 ) { // full
			return qfalse;
		}

		return qtrue;

	case IT_TEAM:     // team items, such as flags

		return qfalse;


	case IT_HOLDABLE:
		return qtrue;

	case IT_TREASURE:       // treasure always picked up
		return qtrue;

	case IT_CLIPBOARD:      // clipboards always picked up
		return qtrue;

		//---- (SA) Wolf keys
	case IT_KEY:
		return qtrue;       // keys are always picked up

	case IT_BAD:
		Com_Error( ERR_DROP, "BG_CanItemBeGrabbed: IT_BAD" );
	default:
#ifndef Q3_VM
#ifndef NDEBUG
          Com_Printf("BG_CanItemBeGrabbed: unknown enum %d\n", item->giType );
#endif
#endif
         break;
	}

	return qfalse;
}
//======================================================================

/*
================
BG_EvaluateTrajectory

================
*/
void BG_EvaluateTrajectory( const trajectory_t *tr, int atTime, vec3_t result ) {
	float deltaTime;
	float phase;
	vec3_t v;

	switch ( tr->trType ) {
	case TR_STATIONARY:
	case TR_INTERPOLATE:
	case TR_GRAVITY_PAUSED: //----(SA)
		VectorCopy( tr->trBase, result );
		break;
	case TR_LINEAR:
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		break;
	case TR_SINE:
		deltaTime = ( atTime - tr->trTime ) / (float) tr->trDuration;
		phase = sin( deltaTime * M_PI * 2 );
		VectorMA( tr->trBase, phase, tr->trDelta, result );
		break;
//----(SA)	removed
	case TR_LINEAR_STOP:
		if ( atTime > tr->trTime + tr->trDuration ) {
			atTime = tr->trTime + tr->trDuration;
		}
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		if ( deltaTime < 0 ) {
			deltaTime = 0;
		}
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		break;
	case TR_GRAVITY:
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		result[2] -= 0.5 * DEFAULT_GRAVITY * deltaTime * deltaTime;     // FIXME: local gravity...
		break;
		// Ridah
	case TR_GRAVITY_LOW:
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		result[2] -= 0.5 * ( DEFAULT_GRAVITY * 0.3 ) * deltaTime * deltaTime;     // FIXME: local gravity...
		break;
		// done.
//----(SA)
	case TR_GRAVITY_FLOAT:
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		result[2] -= 0.5 * ( DEFAULT_GRAVITY * 0.2 ) * deltaTime;
		break;
//----(SA)	end
		// RF, acceleration
	case TR_ACCELERATE:     // trDelta is the ultimate speed
		if ( atTime > tr->trTime + tr->trDuration ) {
			atTime = tr->trTime + tr->trDuration;
		}
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		// phase is the acceleration constant
		phase = VectorLength( tr->trDelta ) / ( tr->trDuration * 0.001 );
		// trDelta at least gives us the acceleration direction
		VectorNormalize2( tr->trDelta, result );
		// get distance travelled at current time
		VectorMA( tr->trBase, phase * 0.5 * deltaTime * deltaTime, result, result );
		break;
	case TR_DECCELERATE:    // trDelta is the starting speed
		if ( atTime > tr->trTime + tr->trDuration ) {
			atTime = tr->trTime + tr->trDuration;
		}
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		// phase is the breaking constant
		phase = VectorLength( tr->trDelta ) / ( tr->trDuration * 0.001 );
		// trDelta at least gives us the acceleration direction
		VectorNormalize2( tr->trDelta, result );
		// get distance travelled at current time (without breaking)
		VectorMA( tr->trBase, deltaTime, tr->trDelta, v );
		// subtract breaking force
		VectorMA( v, -phase * 0.5 * deltaTime * deltaTime, result, result );
		break;
	default:
		Com_Error( ERR_DROP, "BG_EvaluateTrajectory: unknown trType: %i", tr->trType );
		break;
	}
}

/*
================
BG_EvaluateTrajectoryDelta

For determining velocity at a given time
================
*/
void BG_EvaluateTrajectoryDelta( const trajectory_t *tr, int atTime, vec3_t result ) {
	float deltaTime;
	float phase;

	switch ( tr->trType ) {
	case TR_STATIONARY:
	case TR_INTERPOLATE:
		VectorClear( result );
		break;
	case TR_LINEAR:
		VectorCopy( tr->trDelta, result );
		break;
	case TR_SINE:
		deltaTime = ( atTime - tr->trTime ) / (float) tr->trDuration;
		phase = cos( deltaTime * M_PI * 2 );    // derivative of sin = cos
		phase *= 0.5;
		VectorScale( tr->trDelta, phase, result );
		break;
//----(SA)	removed
	case TR_LINEAR_STOP:
		if ( atTime > tr->trTime + tr->trDuration ) {
			VectorClear( result );
			return;
		}
		VectorCopy( tr->trDelta, result );
		break;
	case TR_GRAVITY:
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		VectorCopy( tr->trDelta, result );
		result[2] -= DEFAULT_GRAVITY * deltaTime;       // FIXME: local gravity...
		break;
		// Ridah
	case TR_GRAVITY_LOW:
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		VectorCopy( tr->trDelta, result );
		result[2] -= ( DEFAULT_GRAVITY * 0.3 ) * deltaTime;       // FIXME: local gravity...
		break;
		// done.
//----(SA)
	case TR_GRAVITY_FLOAT:
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		VectorCopy( tr->trDelta, result );
		result[2] -= ( DEFAULT_GRAVITY * 0.2 ) * deltaTime;
		break;
//----(SA)	end
		// RF, acceleration
	case TR_ACCELERATE: // trDelta is eventual speed
		if ( atTime > tr->trTime + tr->trDuration ) {
			VectorClear( result );
			return;
		}
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		VectorScale( tr->trDelta, deltaTime * deltaTime, result );
		break;
	case TR_DECCELERATE:    // trDelta is breaking force
		if ( atTime > tr->trTime + tr->trDuration ) {
			VectorClear( result );
			return;
		}
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		VectorScale( tr->trDelta, deltaTime, result );
		break;
	default:
		Com_Error( ERR_DROP, "BG_EvaluateTrajectoryDelta: unknown trType: %i", tr->trType );
		break;
	}
}

/*
============
BG_GetMarkDir

  used to find a good directional vector for a mark projection, which will be more likely
  to wrap around adjacent surfaces

  dir is the direction of the projectile or trace that has resulted in a surface being hit
============
*/
void BG_GetMarkDir( const vec3_t dir, const vec3_t normal, vec3_t out ) {
	vec3_t ndir, lnormal;
	float minDot = 0.3;

	if ( VectorLength( normal ) < 1.0 ) {
		VectorSet( lnormal, 0, 0, 1 );
	} else {
		VectorCopy( normal, lnormal );
	}

	VectorNegate( dir, ndir );
	VectorNormalize( ndir );
	if ( normal[2] > 0.8 ) {
		minDot = 0.7;
	}
	// make sure it makrs the impact surface
	while ( DotProduct( ndir, lnormal ) < minDot ) {
		VectorMA( ndir, 0.5, lnormal, ndir );
		VectorNormalize( ndir );
	}

	VectorCopy( ndir, out );
}


char *eventnames[] = {
	"EV_NONE",
	"EV_FOOTSTEP",
	"EV_FOOTSTEP_METAL",
	"EV_FOOTSTEP_WOOD",
	"EV_FOOTSTEP_GRASS",
	"EV_FOOTSTEP_GRAVEL",
	"EV_FOOTSTEP_ROOF",
	"EV_FOOTSTEP_SNOW",
	"EV_FOOTSTEP_CARPET",
	"EV_FOOTSPLASH",
	"EV_FOOTWADE",
	"EV_SWIM",
	"EV_STEP_4",
	"EV_STEP_8",
	"EV_STEP_12",
	"EV_STEP_16",
	"EV_FALL_SHORT",
	"EV_FALL_MEDIUM",
	"EV_FALL_FAR",
	"EV_FALL_NDIE",
	"EV_FALL_DMG_10",
	"EV_FALL_DMG_15",
	"EV_FALL_DMG_25",
	"EV_FALL_DMG_50",
	"EV_JUMP_PAD",           // boing sound at origin, jump sound on player
	"EV_JUMP",
	"EV_WATER_TOUCH",    // foot touches
	"EV_WATER_LEAVE",    // foot leaves
	"EV_WATER_UNDER",    // head touches
	"EV_WATER_CLEAR",    // head leaves
	"EV_ITEM_PICKUP",            // normal item pickups are predictable
	"EV_ITEM_PICKUP_QUIET",  // (SA) same, but don't play the default pickup sound as it was specified in the ent
	"EV_GLOBAL_ITEM_PICKUP", // powerup / team sounds are broadcast to everyone
	"EV_NOITEM",
	"EV_NOAMMO",
	"EV_EMPTYCLIP",
	"EV_FILL_CLIP",
	"EV_FILL_CLIP_FULL",
	"EV_FILL_CLIP_AI",
	"EV_STOP_RELOADING_SOUND",
	"EV_RESET_ZOOM",
	"EV_WEAP_OVERHEAT",
	"EV_CHANGE_WEAPON",
	"EV_FIRE_WEAPON",
	"EV_FIRE_WEAPONB",
	"EV_FIRE_WEAPON_LASTSHOT",
	"EV_FIRE_QUICKGREN",
	"EV_FIRE_QUICKGREN2", 
	"EV_NOFIRE_UNDERWATER",
	"EV_FIRE_WEAPON_MG42",
	"EV_SUGGESTWEAP",        //----(SA)	added
	"EV_GRENADE_SUICIDE",    //----(SA)	added
	"EV_USE_ITEM0",
	"EV_USE_ITEM1",
	"EV_USE_ITEM2",
	"EV_USE_ITEM3",
	"EV_USE_ITEM4",
	"EV_USE_ITEM5",
	"EV_USE_ITEM6",
	"EV_USE_ITEM7",
	"EV_USE_ITEM8",
	"EV_USE_ITEM9",
	"EV_USE_ITEM10",
	"EV_USE_ITEM11",
	"EV_USE_ITEM12",
	"EV_USE_ITEM13",
	"EV_USE_ITEM14",
	"EV_USE_ITEM15",
	"EV_ITEM_RESPAWN",
	"EV_ITEM_POP",
	"EV_PLAYER_TELEPORT_IN",
	"EV_PLAYER_TELEPORT_OUT",
	"EV_GRENADE_BOUNCE",     // eventParm will be the soundindex
	"EV_GENERAL_SOUND",
	"EV_GLOBAL_SOUND",       // no attenuation
	"EV_BULLET_HIT_FLESH",
	"EV_BULLET_HIT_WALL",
	"EV_MISSILE_HIT",
	"EV_MISSILE_MISS",
	"EV_RAILTRAIL",
	"EV_VENOM",
	"EV_VENOMFULL",
	"EV_BULLET",             // otherEntity is the shooter
	"EV_LOSE_HAT",
	"EV_REATTACH_HAT",
	"EV_GIB_HEAD",           // only blow off the head
	"EV_PAIN",
	"EV_CROUCH_PAIN",
	"EV_DEATH1",
	"EV_DEATH2",
	"EV_DEATH3",
	"EV_ENTDEATH",           //----(SA)	added
	"EV_OBITUARY",
	"EV_POWERUP_QUAD",
	"EV_POWERUP_BATTLESUIT",
	"EV_POWERUP_BATTLESUIT_SURV",
	"EV_POWERUP_REGEN",
	"EV_GIB_PLAYER",         // gib a previously living player
	"EV_DEBUG_LINE",
	"EV_STOPLOOPINGSOUND",
	"EV_STOPSTREAMINGSOUND", //----(SA)	added
	"EV_TAUNT",
	"EV_SMOKE",
	"EV_SPARKS",
	"EV_SPARKS_ELECTRIC",
	"EV_BATS",
	"EV_BATS_UPDATEPOSITION",
	"EV_BATS_DEATH",
	"EV_EXPLODE",        // func_explosive
	"EV_EFFECT",     // target_effect
	"EV_MORTAREFX",  // mortar firing
	"EV_SPINUP", // JPW NERVE panzerfaust preamble for MP balance
	"EV_SNOW_ON",
	"EV_SNOW_OFF",
	"EV_MISSILE_MISS_SMALL",
	"EV_MISSILE_MISS_LARGE",
	"EV_WOLFKICK_HIT_FLESH",
	"EV_WOLFKICK_HIT_WALL",
	"EV_WOLFKICK_MISS",
	"EV_SPIT_HIT",
	"EV_SPIT_MISS",
	"EV_SHARD",
	"EV_JUNK",
	"EV_EMITTER",    //----(SA)	added
	"EV_OILPARTICLES",
	"EV_OILSLICK",
	"EV_OILSLICKREMOVE",
	"EV_MG42EFX",
	"EV_FLAMEBARREL_BOUNCE",
	"EV_FLAKGUN1",
	"EV_FLAKGUN2",
	"EV_FLAKGUN3",
	"EV_FLAKGUN4",
	"EV_EXERT1",
	"EV_EXERT2",
	"EV_EXERT3",
	"EV_SNOWFLURRY",
	"EV_CONCUSSIVE",
	"EV_DUST",
	"EV_RUMBLE_EFX",
	"EV_GUNSPARKS",
	"EV_FLAMETHROWER_EFFECT",
	"EV_SNIPER_SOUND",
	"EV_POPUP",
	"EV_POPUPBOOK",
	"EV_GIVEPAGE",
	"EV_CLOSEMENU",
	"EV_QUICKGRENS",

	"EV_FIREMODE_SWITCH",

	"EV_BOUNCE_SOUND",

	"EV_VENOM_POWERUP_GONE",

	"EV_M97_PUMP",

	"EV_MEDCRATE_HEAL_TICK",

	"EV_ALERT_SPEAKER",

	"EV_MAX_EVENTS"
};

/*
===============
BG_AddPredictableEventToPlayerstate

Handles the sequence numbers
===============
*/

void    trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );

void BG_AddPredictableEventToPlayerstate( int newEvent, int eventParm, playerState_t *ps ) {

#ifdef _DEBUG
	{
		char buf[256];
		trap_Cvar_VariableStringBuffer( "showevents", buf, sizeof( buf ) );
		if ( atof( buf ) != 0 ) {
#ifdef QAGAME
			Com_Printf( " game event svt %5d -> %5d: num = %20s parm %d\n", ps->pmove_framecount /*ps->commandTime*/, ps->eventSequence, eventnames[newEvent], eventParm );
#else
			Com_Printf( "Cgame event svt %5d -> %5d: num = %20s parm %d\n", ps->pmove_framecount /*ps->commandTime*/, ps->eventSequence, eventnames[newEvent], eventParm );
#endif
		}
	}
#endif
	ps->events[ps->eventSequence & ( MAX_EVENTS - 1 )] = newEvent;
	ps->eventParms[ps->eventSequence & ( MAX_EVENTS - 1 )] = eventParm;
	ps->eventSequence++;
}


/*
========================
BG_PlayerStateToEntityState

This is done after each set of usercmd_t on the server,
and after local prediction on the client
========================
*/
void BG_PlayerStateToEntityState( playerState_t *ps, entityState_t *s, qboolean snap ) {
	int i;

	if ( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR || ps->pm_type == PM_FREEZE || ps->pm_flags & PMF_LIMBO ) { // JPW NERVE limbo
		s->eType = ET_INVISIBLE;
	} else if ( ps->stats[STAT_HEALTH] <= GIB_HEALTH ) {
		s->eType = ET_INVISIBLE;
	} else {
		s->eType = ET_PLAYER;
	}

	s->number = ps->clientNum;

	s->pos.trType = TR_INTERPOLATE;
	VectorCopy( ps->origin, s->pos.trBase );
	if ( snap ) {
		SnapVector( s->pos.trBase );
	}

	s->apos.trType = TR_INTERPOLATE;
	if ( ps->stats[STAT_REVIVE_TIME] > 0 ) {
		// bleeding out -- body stays put where it fell; the player free-looks with the camera only
		vec3_t frozenAngles = { 0, ps->stats[STAT_DEAD_YAW], 0 };
		VectorCopy( frozenAngles, s->apos.trBase );
	} else {
		VectorCopy( ps->viewangles, s->apos.trBase );
	}
	if ( snap ) {
		SnapVector( s->apos.trBase );
	}

	if ( ps->movementDir > 128 ) {
		s->angles2[YAW] = (float)ps->movementDir - 256;
	} else {
		s->angles2[YAW] = ps->movementDir;
	}

	s->legsAnim     = ps->legsAnim;
	s->torsoAnim    = ps->torsoAnim;
	s->clientNum    = ps->clientNum;    // ET_PLAYER looks here instead of at number
										// so corpses can also reference the proper config
	// Ridah, let clients know if this person is using a mounted weapon
	// so they don't show any client muzzle flashes

	// (SA) moved up since it needs to set the ps->eFlags too.
	//		Seems like this could be the problem Raf was
	//		encountering with the EF_DEAD flag below when guys
	//		dead flags weren't sticking

	if ( ps->persistant[PERS_HWEAPON_USE] ) {
		ps->eFlags |= EF_MG42_ACTIVE;
	} else {
		ps->eFlags &= ~EF_MG42_ACTIVE;
	}

	s->eFlags = ps->eFlags;

	if ( ps->stats[STAT_HEALTH] <= 0 ) {
		s->eFlags |= EF_DEAD;
	} else {
		s->eFlags &= ~EF_DEAD;
	}

// from MP
	if ( ps->externalEvent ) {
		s->event = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	} else if ( ps->entityEventSequence < ps->eventSequence ) {
		int seq;

		if ( ps->entityEventSequence < ps->eventSequence - MAX_EVENTS ) {
			ps->entityEventSequence = ps->eventSequence - MAX_EVENTS;
		}
		seq = ps->entityEventSequence & ( MAX_EVENTS - 1 );
		s->event = ps->events[ seq ] | ( ( ps->entityEventSequence & 3 ) << 8 );
		s->eventParm = ps->eventParms[ seq ];
		ps->entityEventSequence++;
	}
// end
	// Ridah, now using a circular list of events for all entities
	// add any new events that have been added to the playerState_t
	// (possibly overwriting entityState_t events)
	for ( i = ps->oldEventSequence; i != ps->eventSequence; i++ ) {
		s->events[s->eventSequence & ( MAX_EVENTS - 1 )] = ps->events[i & ( MAX_EVENTS - 1 )];
		s->eventParms[s->eventSequence & ( MAX_EVENTS - 1 )] = ps->eventParms[i & ( MAX_EVENTS - 1 )];
		s->eventSequence++;
	}
	ps->oldEventSequence = ps->eventSequence;

	s->weapon = ps->weapon;
	s->groundEntityNum = ps->groundEntityNum;

	s->powerups = 0;
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		if ( ps->powerups[ i ] ) {
			s->powerups |= 1 << i;
		}
	}

	s->aiChar = ps->aiChar; // Ridah
//	s->loopSound = ps->loopSound;
	s->teamNum = ps->teamNum;
	s->aiState = ps->aiState;
}

/*
========================
BG_PlayerStateToEntityStateExtraPolate

This is done after each set of usercmd_t on the server,
and after local prediction on the client
========================
*/
void BG_PlayerStateToEntityStateExtraPolate( playerState_t *ps, entityState_t *s, int time, qboolean snap ) {
	int i;

	if ( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR || ps->pm_type == PM_FREEZE || ps->pm_flags & PMF_LIMBO ) { // JPW NERVE limbo
		s->eType = ET_INVISIBLE;
	} else if ( ps->stats[STAT_HEALTH] <= GIB_HEALTH ) {
		s->eType = ET_INVISIBLE;
	} else {
		s->eType = ET_PLAYER;
	}

	s->number = ps->clientNum;

	s->pos.trType = TR_LINEAR_STOP;
	VectorCopy( ps->origin, s->pos.trBase );
	if ( snap ) {
		SnapVector( s->pos.trBase );
	}
	// set the trDelta for flag direction and linear prediction
	VectorCopy( ps->velocity, s->pos.trDelta );
	// set the time for linear prediction
	s->pos.trTime = time;
	// set maximum extra polation time
	s->pos.trDuration = 50; // 1000 / sv_fps (default = 20)

	s->apos.trType = TR_INTERPOLATE;
	if ( ps->stats[STAT_REVIVE_TIME] > 0 ) {
		// bleeding out -- body stays put where it fell; the player free-looks with the camera only
		vec3_t frozenAngles = { 0, ps->stats[STAT_DEAD_YAW], 0 };
		VectorCopy( frozenAngles, s->apos.trBase );
	} else {
		VectorCopy( ps->viewangles, s->apos.trBase );
	}
	if ( snap ) {
		SnapVector( s->apos.trBase );
	}

	s->angles2[YAW] = ps->movementDir;
	s->legsAnim = ps->legsAnim;
	s->torsoAnim = ps->torsoAnim;
	s->clientNum = ps->clientNum;       // ET_PLAYER looks here instead of at number
										// so corpses can also reference the proper config
	s->eFlags = ps->eFlags;
	if ( ps->stats[STAT_HEALTH] <= 0 ) {
		s->eFlags |= EF_DEAD;
	} else {
		s->eFlags &= ~EF_DEAD;
	}

	if ( ps->externalEvent ) {
		s->event = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	} else if ( ps->entityEventSequence < ps->eventSequence ) {
		int seq;

		if ( ps->entityEventSequence < ps->eventSequence - MAX_EVENTS ) {
			ps->entityEventSequence = ps->eventSequence - MAX_EVENTS;
		}
		seq = ps->entityEventSequence & ( MAX_EVENTS - 1 );
		s->event = ps->events[ seq ] | ( ( ps->entityEventSequence & 3 ) << 8 );
		s->eventParm = ps->eventParms[ seq ];
		ps->entityEventSequence++;
	}

	// Ridah, now using a circular list of events for all entities
	// add any new events that have been added to the playerState_t
	// (possibly overwriting entityState_t events)
	if ( ps->oldEventSequence > ps->eventSequence ) {
		ps->oldEventSequence = ps->eventSequence;
	}
	for ( i = ps->oldEventSequence; i != ps->eventSequence; i++ ) {
		s->events[s->eventSequence & ( MAX_EVENTS - 1 )] = ps->events[i & ( MAX_EVENTS - 1 )];
		s->eventParms[s->eventSequence & ( MAX_EVENTS - 1 )] = ps->eventParms[i & ( MAX_EVENTS - 1 )];
		s->eventSequence++;
	}
	ps->oldEventSequence = ps->eventSequence;

	s->weapon = ps->weapon;
	s->groundEntityNum = ps->groundEntityNum;

	s->powerups = 0;
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		if ( ps->powerups[ i ] ) {
			s->powerups |= 1 << i;
		}
	}

//	s->loopSound = ps->loopSound;
//	s->generic1 = ps->generic1;
	s->aiChar = ps->aiChar; // Ridah
	s->teamNum = ps->teamNum;
	s->aiState = ps->aiState;
}


/*
==========================
BG_GetMaxAmmo

Returns the correct max ammo capacity for the given weapon and player state,
taking into account whether the weapon is upgraded and any class-specific bonuses.
==========================
*/
int BG_GetMaxAmmo(const playerState_t *ps, int weapon, float ltAmmoBonus) {
	int maxAmmo;
	int upgradeLevel;

	if (!ps || weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS) {
		return 0;
	}

	const ammotable_t *wt = &ammoTable[weapon];

	upgradeLevel = ps->weaponUpgraded[weapon];
	if (upgradeLevel < 0) {
		upgradeLevel = 0;
	}

	if (upgradeLevel >= 1) {
		float multiplier = 2.0f;

		if (upgradeLevel == 2) {
			multiplier = 2.5f;
		} else if (upgradeLevel >= 3) {
			multiplier = 3.0f;
		}

		maxAmmo = (int)(wt->maxammo * multiplier);
	} else {
		maxAmmo = wt->maxammo;
	}

	if (ps->stats[STAT_PLAYER_CLASS] == PC_LT) {
		maxAmmo = (int)(maxAmmo * LT_AMMO_BONUS_MULTIPLIER);
	}

	return maxAmmo;
}


/*
==========================
BG_GetMaxClip

Returns the correct clip size for the given weapon and player state,
taking into account the weapon upgrade level.
==========================
*/
int BG_GetMaxClip(const playerState_t *ps, int weapon) {
	int upgradeLevel;
	int maxClip;

	if (!ps || weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS) {
		return 0;
	}

	const ammotable_t *wt = &ammoTable[weapon];

	upgradeLevel = ps->weaponUpgraded[weapon];
	if (upgradeLevel < 0) {
		upgradeLevel = 0;
	}

	if (upgradeLevel >= 1) {
		float multiplier = 1.25f;

		if (upgradeLevel == 2) {
			multiplier = 1.5f;
		} else if (upgradeLevel >= 3) {
			multiplier = 2.0f;
		}

		maxClip = (int)(wt->maxclip * multiplier);
		return maxClip;
	}

	return wt->maxclip;
}


float BG_GetWeaponAIRange( int weaponnum ) {
	float range;

	range = GetWeaponTableData( weaponnum )->aiRange;

	if ( range == AI_WEAPON_RANGE_FLAMETHROWER ) {
		return ( FLAMETHROWER_BASE_RANGE * 0.5f ) - 50.0f;
	}

	if ( range == AI_WEAPON_RANGE_TESLA ) {
		return ( TESLA_RANGE * 0.9 ) - 50;
	}

	if ( range > 0.0f ) {
		return range;
	}

	return AI_WEAPON_RANGE_NORMAL;
}

//==========================================================================
// map speaker scripts (sound/maps/<mapname>.sps) -- see the scriptSpeaker_t
// comment in bg_public.h. This is compiled into both game and cgame, each
// getting its own private copy of the list; as long as both sides parse the
// same file, a speaker's index is the same on both, which is all the
// server needs to turn a scripted "togglespeaker <targetname>" into an
// EV_ALERT_SPEAKER event cgame can act on.
//==========================================================================

scriptSpeaker_t scriptSpeakers[MAX_SCRIPT_SPEAKERS];
int             numScriptSpeakers;

/*
================
BG_ClearScriptSpeakers
================
*/
void BG_ClearScriptSpeakers( void ) {
	numScriptSpeakers = 0;
}

/*
================
BG_SS_ParseSpeaker

Parses a single "speakerDef { ... }" block
================
*/
static qboolean BG_SS_ParseSpeaker( const char *filename, char **data ) {
	char            *token;
	scriptSpeaker_t speaker;

	Com_Memset( &speaker, 0, sizeof( speaker ) );
	speaker.volume = 127;
	speaker.range = 1250;

	token = COM_Parse( data );
	if ( Q_stricmp( token, "{" ) ) {
		Com_Printf( S_COLOR_RED "ERROR: %s: expected '{'\n", filename );
		return qfalse;
	}

	while ( 1 ) {
		token = COM_Parse( data );
		if ( !token[0] ) {
			break;
		}

		if ( token[0] == '}' ) {
			break;
		}

		if ( !Q_stricmp( token, "noise" ) ) {
			token = COM_Parse( data );
			Q_strncpyz( speaker.filename, token, sizeof( speaker.filename ) );
		} else if ( !Q_stricmp( token, "origin" ) ) {
			speaker.origin[0] = atof( COM_Parse( data ) );
			speaker.origin[1] = atof( COM_Parse( data ) );
			speaker.origin[2] = atof( COM_Parse( data ) );
		} else if ( !Q_stricmp( token, "targetname" ) ) {
			token = COM_Parse( data );
			Q_strncpyz( speaker.targetname, token, sizeof( speaker.targetname ) );
			speaker.targetnamehash = BG_StringHashValue( speaker.targetname );
		} else if ( !Q_stricmp( token, "looped" ) ) {
			token = COM_Parse( data );
			if ( !Q_stricmp( token, "no" ) ) {
				speaker.loop = SPKR_NOT_LOOPED;
			} else if ( !Q_stricmp( token, "on" ) ) {
				speaker.loop = SPKR_LOOPED_ON;
				speaker.activated = qtrue;
			} else if ( !Q_stricmp( token, "off" ) ) {
				speaker.loop = SPKR_LOOPED_OFF;
			} else {
				Com_Printf( S_COLOR_RED "ERROR: %s: unknown loop value '%s'\n", filename, token );
				return qfalse;
			}
		} else if ( !Q_stricmp( token, "broadcast" ) ) {
			token = COM_Parse( data );
			if ( !Q_stricmp( token, "no" ) ) {
				speaker.broadcast = SPKR_LOCAL;
			} else if ( !Q_stricmp( token, "global" ) ) {
				speaker.broadcast = SPKR_GLOBAL;
			} else if ( !Q_stricmp( token, "nopvs" ) ) {
				speaker.broadcast = SPKR_NOPVS;
			} else {
				Com_Printf( S_COLOR_RED "ERROR: %s: unknown broadcast value '%s'\n", filename, token );
				return qfalse;
			}
		} else if ( !Q_stricmp( token, "wait" ) ) {
			speaker.wait = atoi( COM_Parse( data ) );
		} else if ( !Q_stricmp( token, "random" ) ) {
			speaker.random = atoi( COM_Parse( data ) );
		} else if ( !Q_stricmp( token, "volume" ) ) {
			speaker.volume = atoi( COM_Parse( data ) );
		} else if ( !Q_stricmp( token, "range" ) ) {
			speaker.range = atoi( COM_Parse( data ) );
		} else {
			Com_Printf( S_COLOR_RED "ERROR: %s: unknown token '%s'\n", filename, token );
			return qfalse;
		}
	}

	if ( numScriptSpeakers >= MAX_SCRIPT_SPEAKERS ) {
		Com_Printf( S_COLOR_RED "ERROR: %s: MAX_SCRIPT_SPEAKERS (%i) reached\n", filename, MAX_SCRIPT_SPEAKERS );
		return qfalse;
	}

	scriptSpeakers[numScriptSpeakers++] = speaker;

	return qtrue;
}

/*
================
BG_ParseSpeakerScript

Parses an already-loaded, NUL-terminated "sound/maps/<mapname>.sps" buffer into
scriptSpeakers[]/numScriptSpeakers. Caller owns the buffer (only needed for the
duration of this call) and decides what counts as "no such file" -- a missing
.sps is normal for most maps and isn't handled here.
================
*/
qboolean BG_ParseSpeakerScript( const char *filename, char *data ) {
	char *token;

	token = COM_Parse( &data );
	if ( Q_stricmp( token, "speakerScript" ) ) {
		Com_Printf( S_COLOR_RED "ERROR: %s: expected 'speakerScript'\n", filename );
		return qfalse;
	}

	token = COM_Parse( &data );
	if ( Q_stricmp( token, "{" ) ) {
		Com_Printf( S_COLOR_RED "ERROR: %s: expected '{'\n", filename );
		return qfalse;
	}

	while ( 1 ) {
		token = COM_Parse( &data );
		if ( !token[0] ) {
			break;
		}

		if ( token[0] == '}' ) {
			break;
		}

		if ( !Q_stricmp( token, "speakerDef" ) ) {
			if ( !BG_SS_ParseSpeaker( filename, &data ) ) {
				return qfalse;
			}
		} else {
			Com_Printf( S_COLOR_RED "ERROR: %s: unknown token '%s'\n", filename, token );
			return qfalse;
		}
	}

	return qtrue;
}