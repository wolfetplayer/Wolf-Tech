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
 * name:		cg_weapons.c
 *
 * desc:		events and effects dealing with weapons
 *
*/

#include "cg_local.h"
#include "../ui/ui_shared.h"    // PC_Int_Parse / PC_Float_Parse / PC_String_Parse

int wolfkickModel;
int hWeaponSnd;
int hWeaponEchoSnd;
int hflakWeaponSnd;
int notebookModel;
int propellerModel;

vec3_t ejectBrassCasingOrigin;

//----(SA)
// forward decs
static int getAltWeapon( int weapnum );
int getEquivWeapon( int weapnum );
int CG_WeaponIndex( int weapnum, int *bank, int *cycle );
static qboolean CG_WeaponHasAmmo( int i );

static int maxWeapBanks = MAX_WEAP_BANKS, maxWeapsInBank = MAX_WEAPS_IN_BANK; // JPW NERVE

//----(SA)	end

/*
==============
CG_RifleEjectBrass
==============
*/
static void CG_RifleEjectBrass( centity_t *cent ) {
	localEntity_t *le;
	refEntity_t *re;
	vec3_t velocity, xvelocity;
	vec3_t offset, xoffset;
	float waterScale = 1.0f;
	vec3_t v[3];
	qboolean firstPersonBrass;

	if ( cg_brassTime.integer <= 0 ) {
		return;
	}

	firstPersonBrass =
		!cg.snap->ps.persistant[PERS_HWEAPON_USE] &&
		cent->currentState.clientNum == cg.snap->ps.clientNum;

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	le->leType = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime = le->startTime + cg_brassTime.integer +
		( cg_brassTime.integer / 4 ) * random();

	le->pos.trType = TR_GRAVITY;
	le->pos.trTime = cg.time - ( rand() & 15 );

	AnglesToAxis( cent->lerpAngles, v );

	if ( firstPersonBrass ) {
		// RealRTCW first-person brass origin from weapon tag / calculated point
		VectorCopy( ejectBrassCasingOrigin, re->origin );

		velocity[0] = -50 + 25 * crandom();
		velocity[1] = -100 + 40 * crandom();
		velocity[2] = 200 + 50 * random();
	} else {
		// Third-person / mounted weapon fallback
		velocity[0] = 16;
		velocity[1] = -50 + 40 * crandom();
		velocity[2] = 100 + 50 * crandom();

		if ( cg.snap->ps.persistant[PERS_HWEAPON_USE] ) {
			offset[0] = 32;
			offset[1] = -4;
			offset[2] = 0;
		} else {
			VectorClear( offset );
		}

		xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
		xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
		xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];

		VectorAdd( cent->lerpOrigin, xoffset, re->origin );
	}

	VectorCopy( re->origin, le->pos.trBase );

	if ( CG_PointContents( re->origin, -1 ) & ( CONTENTS_WATER | CONTENTS_SLIME ) ) {
		waterScale = 0.10f;
	}

	xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] + velocity[2] * v[2][0];
	xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] + velocity[2] * v[2][1];
	xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] + velocity[2] * v[2][2];

	VectorScale( xvelocity, waterScale, le->pos.trDelta );

	AxisCopy( axisDefault, re->axis );
	re->hModel = cgs.media.machinegunBrassModel;

	le->bounceFactor = 0.4f * waterScale;

	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;

	if ( firstPersonBrass ) {
		le->angles.trBase[0] = ( rand() & 31 ) + 60;
		le->angles.trBase[1] = rand() & 255;
		le->angles.trBase[2] = rand() & 31;
	} else {
		le->angles.trBase[0] = rand() & 31;
		le->angles.trBase[1] = rand() & 31;
		le->angles.trBase[2] = rand() & 31;
	}

	le->angles.trDelta[0] = 2;
	le->angles.trDelta[1] = 1;
	le->angles.trDelta[2] = 0;

	le->leFlags = LEF_TUMBLE;

	{
		int contents;
		vec3_t end;

		VectorCopy( cent->lerpOrigin, end );
		end[2] -= 24;

		contents = trap_CM_PointContents( end, 0 );

		if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
			le->leBounceSoundType = LEBS_NONE;
		} else {
			le->leBounceSoundType = LEBS_BRASS;
		}
	}

	le->leMarkType = LEMT_NONE;
}


/*
==============
CG_PistolEjectBrass
==============
*/
static void CG_PistolEjectBrass( centity_t *cent ) {
	localEntity_t *le;
	refEntity_t *re;
	vec3_t velocity, xvelocity;
	vec3_t offset, xoffset;
	float waterScale = 1.0f;
	vec3_t v[3];
	qboolean firstPersonBrass;

	if ( cg_brassTime.integer <= 0 ) {
		return;
	}

	if ( cent->currentState.weapon == WP_REVOLVER ) {
		return; // no brass for revolver
	}

	firstPersonBrass =
		!cg.snap->ps.persistant[PERS_HWEAPON_USE] &&
		cent->currentState.clientNum == cg.snap->ps.clientNum;

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	le->leType = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime = le->startTime + cg_brassTime.integer +
		( cg_brassTime.integer / 4 ) * random();

	le->pos.trType = TR_GRAVITY;
	le->pos.trTime = cg.time - ( rand() & 15 );

	AnglesToAxis( cent->lerpAngles, v );

	if ( firstPersonBrass ) {
		// RealRTCW first-person brass origin from weapon tag / calculated point
		VectorCopy( ejectBrassCasingOrigin, re->origin );

		velocity[0] = -50 + 25 * crandom();
		velocity[1] = -100 + 40 * crandom();
		velocity[2] = 200 + 50 * random();
	} else {
		// Third-person / mounted weapon fallback
		velocity[0] = 16;
		velocity[1] = -50 + 40 * crandom();
		velocity[2] = 100 + 50 * crandom();

		if ( cg.snap->ps.persistant[PERS_HWEAPON_USE] ) {
			offset[0] = 32;
			offset[1] = -4;
			offset[2] = 0;
		} else {
			VectorClear( offset );
		}

		xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
		xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
		xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];

		VectorAdd( cent->lerpOrigin, xoffset, re->origin );
	}

	VectorCopy( re->origin, le->pos.trBase );

	if ( CG_PointContents( re->origin, -1 ) & ( CONTENTS_WATER | CONTENTS_SLIME ) ) {
		waterScale = 0.10f;
	}

	xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] + velocity[2] * v[2][0];
	xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] + velocity[2] * v[2][1];
	xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] + velocity[2] * v[2][2];

	VectorScale( xvelocity, waterScale, le->pos.trDelta );

	AxisCopy( axisDefault, re->axis );
	re->hModel = cgs.media.smallgunBrassModel;

	le->bounceFactor = 0.4f * waterScale;

	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;

	if ( firstPersonBrass ) {
		le->angles.trBase[0] = ( rand() & 31 ) + 60;
		le->angles.trBase[1] = rand() & 255;
		le->angles.trBase[2] = rand() & 31;
	} else {
		le->angles.trBase[0] = rand() & 31;
		le->angles.trBase[1] = rand() & 31;
		le->angles.trBase[2] = rand() & 31;
	}

	le->angles.trDelta[0] = 2;
	le->angles.trDelta[1] = 1;
	le->angles.trDelta[2] = 0;

	le->leFlags = LEF_TUMBLE;

	{
		int contents;
		vec3_t end;

		VectorCopy( cent->lerpOrigin, end );
		end[2] -= 24;

		contents = trap_CM_PointContents( end, 0 );

		if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
			le->leBounceSoundType = LEBS_NONE;
		} else {
			le->leBounceSoundType = LEBS_BRASS;
		}
	}

	le->leMarkType = LEMT_NONE;
}


/*
==============
CG_ShotgunEjectBrass
==============
*/
static void CG_ShotgunEjectBrass( centity_t *cent ) {
	localEntity_t *le;
	refEntity_t *re;
	vec3_t velocity, xvelocity;
	vec3_t offset, xoffset;
	float waterScale = 1.0f;
	vec3_t v[3];
	qboolean firstPersonBrass;

	if ( cg_brassTime.integer <= 0 ) {
		return;
	}

	firstPersonBrass =
		!cg.snap->ps.persistant[PERS_HWEAPON_USE] &&
		cent->currentState.clientNum == cg.snap->ps.clientNum;

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	le->leType = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime = le->startTime + cg_brassTime.integer +
		( cg_brassTime.integer / 4 ) * random();

	le->pos.trType = TR_GRAVITY;
	le->pos.trTime = cg.time - ( rand() & 15 );

	AnglesToAxis( cent->lerpAngles, v );

	if ( firstPersonBrass ) {
		// RealRTCW first-person brass origin from weapon tag / calculated point
		VectorCopy( ejectBrassCasingOrigin, re->origin );

		velocity[0] = -50 + 25 * crandom();
		velocity[1] = -100 + 40 * crandom();
		velocity[2] = 200 + 50 * random();
	} else {
		// Third-person / mounted weapon fallback
		velocity[0] = 16;
		velocity[1] = -50 + 40 * crandom();
		velocity[2] = 100 + 50 * crandom();

		if ( cg.snap->ps.persistant[PERS_HWEAPON_USE] ) {
			offset[0] = 32;
			offset[1] = -4;
			offset[2] = 0;
		} else {
			VectorClear( offset );
		}

		xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
		xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
		xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];

		VectorAdd( cent->lerpOrigin, xoffset, re->origin );
	}

	VectorCopy( re->origin, le->pos.trBase );

	if ( CG_PointContents( re->origin, -1 ) & ( CONTENTS_WATER | CONTENTS_SLIME ) ) {
		waterScale = 0.10f;
	}

	xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] + velocity[2] * v[2][0];
	xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] + velocity[2] * v[2][1];
	xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] + velocity[2] * v[2][2];

	VectorScale( xvelocity, waterScale, le->pos.trDelta );

	AxisCopy( axisDefault, re->axis );
	re->hModel = cgs.media.shotgunBrassModel;

	le->bounceFactor = 0.4f * waterScale;

	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;

	if ( firstPersonBrass ) {
		le->angles.trBase[0] = ( rand() & 31 ) + 60;
		le->angles.trBase[1] = rand() & 255;
		le->angles.trBase[2] = rand() & 31;
	} else {
		le->angles.trBase[0] = rand() & 31;
		le->angles.trBase[1] = rand() & 31;
		le->angles.trBase[2] = rand() & 31;
	}

	le->angles.trDelta[0] = 2;
	le->angles.trDelta[1] = 1;
	le->angles.trDelta[2] = 0;

	le->leFlags = LEF_TUMBLE;

	{
		int contents;
		vec3_t end;

		VectorCopy( cent->lerpOrigin, end );
		end[2] -= 24;

		contents = trap_CM_PointContents( end, 0 );

		if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
			le->leBounceSoundType = LEBS_NONE;
		} else {
			le->leBounceSoundType = LEBS_BRASS;
		}
	}

	le->leMarkType = LEMT_NONE;
}


//----(SA)	added
/*
==============
CG_PanzerFaustEjectBrass
	toss the 'used' panzerfaust casing (unit is one-shot, disposable)
==============
*/
static void CG_PanzerFaustEjectBrass( centity_t *cent ) {
	localEntity_t   *le;
	refEntity_t     *re;
	vec3_t velocity, xvelocity;
	vec3_t offset, xoffset;
	float waterScale = 1.0f;
	vec3_t v[3];

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

//	velocity[0] = 16;
//	velocity[1] = -50 + 40 * crandom();
//	velocity[2] = 100 + 50 * crandom();

	velocity[0] = 16;
	velocity[1] = -200;
	velocity[2] = 0;

	le->leType = LE_FRAGMENT;
	le->startTime = cg.time;
//	le->startTime = cg.time + 2000;
	le->endTime = le->startTime + ( cg_brassTime.integer * 8 ) + ( cg_brassTime.integer * random() );

	le->pos.trType = TR_GRAVITY;
	le->pos.trTime = cg.time - ( rand() & 15 );
//	le->pos.trTime = cg.time - 2000;

	AnglesToAxis( cent->lerpAngles, v );

//	offset[0] = 12;
//	offset[1] = -4;
//	offset[2] = 24;

	offset[0] = -24;    // forward
	offset[1] = -4; // left
	offset[2] = 24; // up

	xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
	xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
	xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];
	VectorAdd( cent->lerpOrigin, xoffset, re->origin );

	VectorCopy( re->origin, le->pos.trBase );

	if ( CG_PointContents( re->origin, -1 ) & ( CONTENTS_WATER | CONTENTS_SLIME ) ) {
		waterScale = 0.10;
	}

	xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] + velocity[2] * v[2][0];
	xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] + velocity[2] * v[2][1];
	xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] + velocity[2] * v[2][2];
	VectorScale( xvelocity, waterScale, le->pos.trDelta );

	AxisCopy( axisDefault, re->axis );

	// (SA) make it bigger
	le->sizeScale = 3.0f;

	re->hModel = cgs.media.panzerfaustBrassModel;

	le->bounceFactor = 0.4 * waterScale;

	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;
//	le->angles.trBase[0] = rand()&31;
//	le->angles.trBase[1] = rand()&31;
//	le->angles.trBase[2] = rand()&31;
	le->angles.trBase[0] = 0;
	le->angles.trBase[1] = cent->currentState.apos.trBase[1];   // rotate to match the player
	le->angles.trBase[2] = 0;
//	le->angles.trDelta[0] = 2;
//	le->angles.trDelta[1] = 1;
//	le->angles.trDelta[2] = 0;
	le->angles.trDelta[0] = 0;
	le->angles.trDelta[1] = 0;
	le->angles.trDelta[2] = 0;

	le->leFlags = LEF_TUMBLE | LEF_SMOKING;   // (SA) probably doesn't need to be 'tumble' since it doesn't really rotate much when flying

	le->leBounceSoundType = LEBS_NONE;

	le->leMarkType = LEMT_NONE;
}
/*
==============
CG_SpearTrail
	simple bubble trail behind a missile
==============
*/
void CG_SpearTrail( centity_t *ent, const weaponInfo_t *wi ) {
	int contents, lastContents;
	vec3_t origin, lastPos;
	entityState_t   *es;

	es = &ent->currentState;
	BG_EvaluateTrajectory( &es->pos, cg.time, origin );
	contents = CG_PointContents( origin, -1 );

	BG_EvaluateTrajectory( &es->pos, ent->trailTime, lastPos );
	lastContents = CG_PointContents( lastPos, -1 );

	ent->trailTime = cg.time;

	if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
		if ( contents & lastContents & CONTENTS_WATER ) {
			CG_BubbleTrail( lastPos, origin, 1, 8 );
		}
	}
}


// JPW NERVE -- LT pyro for marking air strikes
/*
==========================
CG_PyroSmokeTrail
==========================
*/
void CG_PyroSmokeTrail( centity_t *ent, const weaponInfo_t *wi ) {
	int step;
	vec3_t origin, lastPos, dir;
	int startTime;
	entityState_t   *es;
	int t;
	float rnd;
	static float grounddir = 99;

	es = &ent->currentState;

	// landmines only trail smoke for a moment after being triggered, and play a one-shot warning cue
	if ( es->weapon == WP_LANDMINE ) {
		if ( es->effect1Time != 2 ) {
			ent->miscTime = 0;
			return;
		}

		if ( !ent->miscTime ) {
			static sfxHandle_t landmineTriggeredSound = 0;

			ent->trailTime = cg.time;
			ent->miscTime  = cg.time;

			if ( !landmineTriggeredSound ) {
				landmineTriggeredSound = trap_S_RegisterSound( "sound/weapons/landmine/mine_on.wav" );
			}
			if ( landmineTriggeredSound ) {
				trap_S_StartSound( NULL, es->number, CHAN_WEAPON, landmineTriggeredSound );
			}
		}

		if ( cg.time - ent->miscTime > 1000 ) {
			return;
		}
	}

	if ( grounddir == 99 ) { // pick a wind direction -- cheap trick because it can be different
		grounddir = crandom(); // on different clients, but it's all smoke and mirrors anyway
	}
	step = 30;
	startTime = ent->trailTime;
	t = step * ( ( startTime + step ) / step );

	BG_EvaluateTrajectory( &es->pos, cg.time, origin );
	CG_PointContents( origin, -1 );

	BG_EvaluateTrajectory( &es->pos, ent->trailTime, lastPos );
	CG_PointContents( lastPos, -1 );

	ent->trailTime = cg.time;

	// drop fire trail sprites
	for ( ; t <= ent->trailTime ; t += step ) {

		BG_EvaluateTrajectory( &es->pos, t, lastPos );
//		VectorCopy ( ent->lerpOrigin, lastPos );

		if ( ent->currentState.density ) { // corkscrew effect
			vec3_t right;
			vec3_t angles;
			VectorCopy( ent->currentState.apos.trBase, angles );
			angles[ROLL] += cg.time % 360;
			AngleVectors( angles, NULL, right, NULL );
			VectorMA( lastPos, ent->currentState.density, right, lastPos );
		}

		dir[0] = crandom() * 5; // compute offset from flare base
		dir[1] = crandom() * 5;
		dir[2] = 0;
		VectorAdd( lastPos,dir,origin ); // store in origin

		rnd = random();

		dir[0] = random() * 0.25;
		dir[1] = grounddir; // simulate a little wind so it looks natural
		dir[2] = random(); // one direction (so smoke goes side-like)
		VectorNormalize( dir );
		VectorScale( dir,45,dir ); // was 75

		if ( es->weapon == WP_LANDMINE ) { // triggered mine warning: distinct blue smoke
			CG_SmokePuff( origin, dir,
							   25 + rnd * 110, // width
							   rnd * 0.3f, rnd * 0.3f, 1, 0.6f,
							   4800 + ( rand() % 2800 ), // duration was 2800+
							   t,
							   0,
							   0,
							   cgs.media.smokePuffShader );
		} else if ( !ent->currentState.otherEntityNum2 ) { // axis team, generate red smoke
			CG_SmokePuff( origin, dir,
							   25 + rnd * 110, // width
							   rnd * 0.5 + 0.5, rnd * 0.5 + 0.5, 1, 0.5,
							   4800 + ( rand() % 2800 ), // duration was 2800+
							   t,
							   0,
							   0,
							   cgs.media.smokePuffShader );
		} else {
			CG_SmokePuff( origin, dir,
							   25 + rnd * 110, // width
							   1.0, rnd * 0.5 + 0.5, rnd * 0.5 + 0.5, 0.5,
							   4800 + ( rand() % 2800 ), // duration was 2800+
							   t,
							   0,
							   0,
							   cgs.media.smokePuffShader );
		}
	}
}
// jpw


// Ridah, new trail effects
/*
==========================
CG_RocketTrail
==========================
*/
void CG_RocketTrail( centity_t *ent, const weaponInfo_t *wi ) {
	int step;
	vec3_t origin, lastPos;
	int contents;
	int lastContents, startTime;
	entityState_t   *es;
	int t;
//	localEntity_t	*le;

	if ( ent->currentState.eType == ET_FLAMEBARREL ) {
		step = 30;
	} else if ( ent->currentState.eType == ET_FP_PARTS ) {
		step = 50;
	} else if ( ent->currentState.eType == ET_RAMJET ) {
		step = 10;
	} else {
		step = 10;
	}

	es = &ent->currentState;
	startTime = ent->trailTime;
	t = step * ( ( startTime + step ) / step );

	BG_EvaluateTrajectory( &es->pos, cg.time, origin );
	contents = CG_PointContents( origin, -1 );

	// if object (e.g. grenade) is stationary, don't toss up smoke
	if ( ( ent->currentState.eType != ET_RAMJET ) && es->pos.trType == TR_STATIONARY ) {
		ent->trailTime = cg.time;
		return;
	}

	BG_EvaluateTrajectory( &es->pos, ent->trailTime, lastPos );
	lastContents = CG_PointContents( lastPos, -1 );

	ent->trailTime = cg.time;

	if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
		if ( contents & lastContents & CONTENTS_WATER ) {
			CG_BubbleTrail( lastPos, origin, 3, 8 );
		}
		return;
	}

	// drop fire trail sprites
	for ( ; t <= ent->trailTime ; t += step ) {
		float rnd;

		BG_EvaluateTrajectory( &es->pos, t, lastPos );
		/*
		le = CG_SmokePuff( lastPos, vec3_origin,
					  5,	// width
					  1, 1, 1, 0.33,
					  150 + rand()%350, // duration
					  t,
					  0,
					  cgs.media.flameThrowerhitShader );

		// use the optimized local entity add
		le->leType = LE_SCALE_FADE;
		*/
		rnd = random();
		if ( ent->currentState.eType == ET_FLAMEBARREL ) {
			if ( ( rand() % 100 ) > 50 ) {
				CG_ParticleExplosion( "twiltb2", lastPos, vec3_origin, 100 + (int)( rnd * 400 ), 5, 7 + (int)( rnd * 10 ) ); // fire

			}
			CG_ParticleExplosion( "blacksmokeanim", lastPos, vec3_origin, 800 + (int)( rnd * 1500 ), 5, 12 + (int)( rnd * 30 ) );    // smoke
		} else if ( ent->currentState.eType == ET_FP_PARTS )     {
			if ( ( rand() % 100 ) > 50 ) {
				CG_ParticleExplosion( "twiltb2", lastPos, vec3_origin, 100 + (int)( rnd * 400 ), 5, 7 + (int)( rnd * 10 ) ); // fire

			}
			CG_ParticleExplosion( "blacksmokeanim", lastPos, vec3_origin, 800 + (int)( rnd * 1500 ), 5, 12 + (int)( rnd * 30 ) );    // smoke
		} else if ( ent->currentState.eType == ET_RAMJET )     {
			int duration;

			VectorCopy( ent->lerpOrigin, lastPos );
			duration = 100;
			CG_ParticleExplosion( "twiltb2", lastPos, vec3_origin, duration + (int)( rnd * 100 ), 5, 5 + (int)( rnd * 10 ) ); // fire
			CG_ParticleExplosion( "blacksmokeanim", lastPos, vec3_origin, 400 + (int)( rnd * 750 ), 12, 24 + (int)( rnd * 30 ) );    // smoke
		} else if ( ent->currentState.eType == ET_FIRE_COLUMN || ent->currentState.eType == ET_FIRE_COLUMN_SMOKE )     {
			int duration;
			int sizeStart;
			int sizeEnd;

			//VectorCopy (ent->lerpOrigin, lastPos);

			if ( ent->currentState.density ) { // corkscrew effect
				vec3_t right;
				vec3_t angles;
				VectorCopy( ent->currentState.apos.trBase, angles );
				angles[ROLL] += cg.time % 360;
				AngleVectors( angles, NULL, right, NULL );
				VectorMA( lastPos, ent->currentState.density, right, lastPos );
			}

			duration = ent->currentState.angles[0];
			sizeStart = ent->currentState.angles[1];
			sizeEnd = ent->currentState.angles[2];

			if ( !duration ) {
				duration = 100;
			}

			if ( !sizeStart ) {
				sizeStart = 5;
			}

			if ( !sizeEnd ) {
				sizeEnd = 7;
			}

			CG_ParticleExplosion( "twiltb2", lastPos, vec3_origin, duration + (int)( rnd * 400 ), sizeStart, sizeEnd + (int)( rnd * 10 ) );   // fire

			if ( ent->currentState.eType == ET_FIRE_COLUMN_SMOKE && ( rand() % 100 ) > 50 ) {
				CG_ParticleExplosion( "blacksmokeanim", lastPos, vec3_origin, 800 + (int)( rnd * 1500 ), 5, 12 + (int)( rnd * 30 ) );    // smoke
			}
		} else
		{
			//CG_ParticleExplosion( "twiltb", lastPos, vec3_origin, 300+(int)(rnd*100), 4, 14+(int)(rnd*8) );	// fire
			CG_ParticleExplosion( "blacksmokeanim", lastPos, vec3_origin, 800 + (int)( rnd * 1500 ), 5, 12 + (int)( rnd * 30 ) );    // smoke
		}
	}
/*
	// spawn a smoke junction
	if ((cg.time - ent->lastTrailTime) >= 50 + rand()%50) {
		ent->headJuncIndex = CG_AddSmokeJunc( ent->headJuncIndex,
												cgs.media.smokeTrailShader,
												origin,
												4500, 0.4, 20, 80 );
		ent->lastTrailTime = cg.time;
	}
*/
// done.
}

// Ridah
/*
==========================
CG_GrenadeTrail
==========================
*/
static void CG_GrenadeTrail( centity_t *ent, const weaponInfo_t *wi ) {
	int step;
	vec3_t origin, lastPos;
	int contents;
	int lastContents, startTime;
	entityState_t   *es;
	int t;

	step = 15;  // nice and smooth curves

	es = &ent->currentState;
	startTime = ent->trailTime;
	t = step * ( ( startTime + step ) / step );

	BG_EvaluateTrajectory( &es->pos, cg.time, origin );
	contents = CG_PointContents( origin, -1 );

	// if object (e.g. grenade) is stationary, don't toss up smoke
	if ( es->pos.trType == TR_STATIONARY ) {
		ent->trailTime = cg.time;
		return;
	}

	BG_EvaluateTrajectory( &es->pos, ent->trailTime, lastPos );
	lastContents = CG_PointContents( lastPos, -1 );

	ent->trailTime = cg.time;

	if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
		if ( contents & lastContents & CONTENTS_WATER ) {
			CG_BubbleTrail( lastPos, origin, 2, 8 );
		}
		return;
	}

//----(SA)	trying this back on for DM

	// spawn smoke junctions
	for ( ; t <= ent->trailTime ; t += step ) {
		BG_EvaluateTrajectory( &es->pos, t, origin );
		ent->headJuncIndex = CG_AddSmokeJunc( ent->headJuncIndex,
											  cgs.media.smokeTrailShader,
											  origin,
//												1500, 0.3, 10, 50 );
											  1000, 0.3, 2, 20 );
		ent->lastTrailTime = cg.time;
	}
//----(SA)	end
}
// done.




/*
==========================
CG_NailgunEjectBrass
==========================
*/
/*
static void CG_NailgunEjectBrass( centity_t *cent ) {
	localEntity_t	*smoke;
	vec3_t			origin;
	vec3_t			v[3];
	vec3_t			offset;
	vec3_t			xoffset;
	vec3_t			up;

	AnglesToAxis( cent->lerpAngles, v );

	offset[0] = 0;
	offset[1] = -12;
	offset[2] = 24;

	xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
	xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
	xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];
	VectorAdd( cent->lerpOrigin, xoffset, origin );

	VectorSet( up, 0, 0, 64 );

	smoke = CG_SmokePuff( origin, up, 32, 1, 1, 1, 0.33f, 700, cg.time, 0, 0, cgs.media.smokePuffShader );
	// use the optimized local entity add
	smoke->leType = LE_SCALE_FADE;
}

static void CG_NailTrail( centity_t *ent, const weaponInfo_t *wi ) {
	int		step;
	vec3_t	origin, lastPos;
	int		t;
	int		startTime, contents;
	int		lastContents;
	entityState_t	*es;
	vec3_t	up;
	localEntity_t	*smoke;

	up[0] = 0;
	up[1] = 0;
	up[2] = 0;

	step = 50;

	es = &ent->currentState;
	startTime = ent->trailTime;
	t = step * ( (startTime + step) / step );

	BG_EvaluateTrajectory( &es->pos, cg.time, origin );
	contents = CG_PointContents( origin, -1 );

	// if object (e.g. grenade) is stationary, don't toss up smoke
	if ( es->pos.trType == TR_STATIONARY ) {
		ent->trailTime = cg.time;
		return;
	}

	BG_EvaluateTrajectory( &es->pos, ent->trailTime, lastPos );
	lastContents = CG_PointContents( lastPos, -1 );

	ent->trailTime = cg.time;

	if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
		if ( contents & lastContents & CONTENTS_WATER ) {
			CG_BubbleTrail( lastPos, origin, 1, 8 );
		}
		return;
	}

	for ( ; t <= ent->trailTime ; t += step ) {
		BG_EvaluateTrajectory( &es->pos, t, lastPos );

		smoke = CG_SmokePuff( lastPos, up,
					  wi->trailRadius,
					  1, 1, 1, 0.33f,
					  wi->wiTrailTime,
					  t,
					  0,
					  0,
					  cgs.media.nailPuffShader );
		// use the optimized local entity add
		smoke->leType = LE_SCALE_FADE;
	}

}
*/

/*
==========================
CG_RailTrail
	SA: re-inserted this as a debug mechanism for bullets
==========================
*/
void CG_RailTrail2( clientInfo_t *ci, vec3_t start, vec3_t end ) {
	localEntity_t   *le;
	refEntity_t     *re;

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	le->leType = LE_FADE_RGB;
	le->startTime = cg.time;
	le->endTime = cg.time + cg_railTrailTime.value;
	le->lifeRate = 1.0 / ( le->endTime - le->startTime );

	re->shaderTime = cg.time / 1000.0f;
	re->reType = RT_RAIL_CORE;
	re->customShader = cgs.media.railCoreShader;

	VectorCopy( start, re->origin );
	VectorCopy( end, re->oldorigin );

//	// still allow different colors so we can tell AI shots from player shots, etc.
	if ( ci ) {
		le->color[0] = ci->color[0] * 0.75;
		le->color[1] = ci->color[1] * 0.75;
		le->color[2] = ci->color[2] * 0.75;
	} else {
		le->color[0] = 1;
		le->color[1] = 0;
		le->color[2] = 0;
	}
	le->color[3] = 1.0f;

	AxisClear( re->axis );
}

//void CG_RailTrailBox( clientInfo_t *ci, vec3_t start, vec3_t end) {
/*
==============
CG_RailTrail
	modified so we could draw boxes for debugging as well
==============
*/
void CG_RailTrail( clientInfo_t *ci, vec3_t start, vec3_t end, int type ) {  //----(SA)	added 'type'
	vec3_t diff, v1, v2, v3, v4, v5, v6;

	if ( !type ) { // just a line
		CG_RailTrail2( ci, start, end );
		return;
	}

	// type '1' (box)

	VectorSubtract( start, end, diff );

	VectorCopy( start, v1 );
	VectorCopy( start, v2 );
	VectorCopy( start, v3 );
	v1[0] -= diff[0];
	v2[1] -= diff[1];
	v3[2] -= diff[2];
	CG_RailTrail2( ci, start, v1 );
	CG_RailTrail2( ci, start, v2 );
	CG_RailTrail2( ci, start, v3 );

	VectorCopy( end, v4 );
	VectorCopy( end, v5 );
	VectorCopy( end, v6 );
	v4[0] += diff[0];
	v5[1] += diff[1];
	v6[2] += diff[2];
	CG_RailTrail2( ci, end, v4 );
	CG_RailTrail2( ci, end, v5 );
	CG_RailTrail2( ci, end, v6 );

	CG_RailTrail2( ci, v2, v6 );
	CG_RailTrail2( ci, v6, v1 );
	CG_RailTrail2( ci, v1, v5 );

	CG_RailTrail2( ci, v2, v4 );
	CG_RailTrail2( ci, v4, v3 );
	CG_RailTrail2( ci, v3, v5 );

}





/*
======================
CG_ParseWeaponConfig
	read information for weapon animations (first/length/fps)
======================
*/
static qboolean CG_ParseWeaponConfig( const char *filename, weaponInfo_t *wi, int weaponNum ) {
	char        *text_p, *prev;
	int len;
	int i;
	float fps;
	char        *token;
	qboolean newfmt = qfalse;       //----(SA)
	char text[20000];
	fileHandle_t f;

	// load the file
	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( len <= 0 ) {
		return qfalse;
	}

	if ( len >= sizeof( text ) - 1 ) {
		CG_Printf( "File %s too long\n", filename );
		return qfalse;
	}

	trap_FS_Read( text, len, f );
	text[len] = 0;
	trap_FS_FCloseFile( f );

	// parse the text
	text_p = text;

	// read optional parameters
	while ( 1 ) {
		prev = text_p;  // so we can unget
		token = COM_Parse( &text_p );
		if ( !token[0] ) {                     // get the variable
			break;
		}
		if ( !Q_stricmp( token, "whatever_variable" ) ) {
			token = COM_Parse( &text_p );   // get the value
			if ( !token[0] ) {
				break;
			}
			continue;
		}

		if ( !Q_stricmp( token, "newfmt" ) ) {
			newfmt = qtrue;
			continue;
		}

		// if it is a number, start parsing animations
		if ( Q_isnumeric( token[0] ) ) {
			text_p = prev;  // unget the token
			break;
		}
		Com_Printf( "unknown token in weapon cfg '%s' is %s\n", token, filename );
	}


	for ( i = 0 ; i < MAX_WP_ANIMATIONS  ; i++ ) {

		token = COM_Parse( &text_p );   // first frame
		if ( !token[0] ) {
			// don't show warning for weapon cfg without altswitch that does not require it.
			if ( i == WEAP_ALTSWITCHFROM && ammoTable[weaponNum].weapAlts == WP_NONE ) {
				for ( ; i < MAX_WP_ANIMATIONS  ; i++ ) {
					Com_Memcpy( &wi->weapAnimations[i], &wi->weapAnimations[WEAP_IDLE1], sizeof( wi->weapAnimations[0] ) );
				}
			}
			// older weapon.cfg files lack the fast-reload and/or sprint-lock entries -- fall back to the
			// matching normal-speed reload anim, or the drop/raise anim, until real frame data is authored.
			else if ( i >= WEAP_RELOAD1_FAST && i < MAX_WP_ANIMATIONS ) {
				for ( ; i < MAX_WP_ANIMATIONS ; i++ ) {
					int srcAnim;
					if ( i >= WEAP_SPRINTIN ) {
						srcAnim = ( i == WEAP_SPRINTIN ) ? WEAP_DROP : WEAP_RAISE;
					} else {
						srcAnim = WEAP_RELOAD1 + ( i - WEAP_RELOAD1_FAST );
					}
					Com_Memcpy( &wi->weapAnimations[i], &wi->weapAnimations[srcAnim], sizeof( wi->weapAnimations[0] ) );
				}
			}
			break;
		}
		wi->weapAnimations[i].firstFrame = atoi( token );

		token = COM_Parse( &text_p );   // length
		if ( !token[0] ) {
			break;
		}
		wi->weapAnimations[i].numFrames = atoi( token );

		token = COM_Parse( &text_p );   // fps
		if ( !token[0] ) {
			break;
		}
		fps = atof( token );
		if ( fps == 0 ) {
			fps = 1;
		}

		wi->weapAnimations[i].frameLerp = 1000 / fps;
		wi->weapAnimations[i].initialLerp = 1000 / fps;

		token = COM_Parse( &text_p );   // looping frames
		if ( !token[0] ) {
			break;
		}
		wi->weapAnimations[i].loopFrames = atoi( token );
		if ( wi->weapAnimations[i].loopFrames > wi->weapAnimations[i].numFrames ) {
			wi->weapAnimations[i].loopFrames = wi->weapAnimations[i].numFrames;
		} else if ( wi->weapAnimations[i].loopFrames < 0 ) {
			wi->weapAnimations[i].loopFrames = 0;
		}


		// store animation/draw bits in '.moveSpeed'

		wi->weapAnimations[i].moveSpeed = 0;

		if ( newfmt ) {
			token = COM_Parse( &text_p );   // barrel anim bits
			if ( !token[0] ) {
				break;
			}
			wi->weapAnimations[i].moveSpeed = atoi( token );

			token = COM_Parse( &text_p );   // animated weapon
			if ( !token[0] ) {
				break;
			}
			if ( atoi( token ) ) {
				wi->weapAnimations[i].moveSpeed |= ( 1 << W_MAX_PARTS );    // set the bit one higher than can be set by the barrel bits

			}
			token = COM_Parse( &text_p );   // barrel hide bits (so objects can be flagged to not be drawn during all sequences (a reloading hand that comes in from off screen for that one animation for example)
			if ( !token[0] ) {
				break;
			}
			wi->weapAnimations[i].moveSpeed |= ( ( atoi( token ) ) << 8 ); // use 2nd byte for draw bits
		}

	}

	if ( i != MAX_WP_ANIMATIONS ) {
		CG_Printf( "Error parsing weapon animation file: %s\n", filename );
		return qfalse;
	}


	return qtrue;
}


/*
======================================================================
.weap cosmetic media loader (cgame-only)

Loads weapons/<name>.weap for weapons that have ammoTable[].weapFile set.
Only ever writes into weaponInfo_t (models/sounds/icons/eject-brass/dlight
color) - never touches ammoTable or anything read by game/bg_pmove.c, so a
missing or hand-edited .weap file can only break this client's own
rendering, never hit registration, damage, or ammo counts.
======================================================================
*/

typedef struct {
	char tpModel[MAX_QPATH];
	char tpFlashModel[MAX_QPATH];
	char fpModel[MAX_QPATH];
	char fpFlashModel[MAX_QPATH];
	char fpParts[W_MAX_PARTS][MAX_QPATH];  // first-person barrel/part models - WolfTech attaches these to fixed "tag_barrel"/"tag_barrelN" tags by index
	char tpParts[W_MAX_PARTS][MAX_QPATH];  // third-person barrel/part models - only WP_VENOM's hardcoded tag_barrel case (see CG_AddPlayerWeapon) actually reads these
	char sktpModel[MAX_QPATH];
	char sktpFlashModel[MAX_QPATH];
	char configPath[MAX_QPATH];            // optional explicit weapon.cfg path; falls back to "<firstPersonModel dir>weapon.cfg" if unset
	char handsModel[MAX_QPATH];
	char standModel[MAX_QPATH];
	char weaponIcon[MAX_QPATH];
	char weaponSelectedIcon[MAX_QPATH];
	char ammoIcon[MAX_QPATH];
} weapFileClient_t;

/*
======================
CG_WeapEjectBrassFuncForName
======================
*/
static void ( *CG_WeapEjectBrassFuncForName( const char *name ) )( centity_t * ) {
	if ( !Q_stricmp( name, "PistolEjectBrass" ) ) {
		return CG_PistolEjectBrass;
	}
	if ( !Q_stricmp( name, "RifleEjectBrass" ) ) {
		return CG_RifleEjectBrass;
	}
	if ( !Q_stricmp( name, "PanzerFaustEjectBrass" ) ) {
		return CG_PanzerFaustEjectBrass;
	}
	if ( !Q_stricmp( name, "ShotgunEjectBrass" ) ) {
		return CG_ShotgunEjectBrass;
	}
	return NULL;
}

/*
======================
CG_WeapMissileTrailFuncForName
======================
*/
static void ( *CG_WeapMissileTrailFuncForName( const char *name ) )( centity_t *, const weaponInfo_t * ) {
	if ( !Q_stricmp( name, "GrenadeTrail" ) ) {
		return CG_GrenadeTrail;
	}
	if ( !Q_stricmp( name, "RocketTrail" ) ) {
		return CG_RocketTrail;
	}
	if ( !Q_stricmp( name, "PyroSmokeTrail" ) ) {
		return CG_PyroSmokeTrail;
	}
	return NULL;
}

/*
======================
CG_WeapParseError
======================
*/
static void CG_WeapParseError( int handle, const char *filename, const char *format, ... ) {
	va_list argptr;
	char string[1024];

	va_start( argptr, format );
	Q_vsnprintf( string, sizeof( string ), format, argptr );
	va_end( argptr );

	CG_Printf( S_COLOR_RED "ERROR: %s: %s\n", filename, string );

	trap_PC_FreeSource( handle );
}

/*
======================
CG_WeapAppendSound

Fills the next free slot of a 4-entry random-variant sound array
(flashSound/flashEchoSound/lastShotSound/switchSound).
======================
*/
static void CG_WeapAppendSound( sfxHandle_t sounds[4], const char *filename ) {
	int i;

	for ( i = 0; i < 4; i++ ) {
		if ( !sounds[i] ) {
			sounds[i] = trap_S_RegisterSound( filename );
			return;
		}
	}
	CG_Printf( S_COLOR_YELLOW "WARNING: only up to 4 sounds supported per weapon sound slot (%s)\n", filename );
}

/*
======================
CG_RW_ParseWeaponLinkPart

Parses "part <index> { tag "<name>" model "<path>" }" inside a weaponLink
block. WolfTech's renderer attaches parts to fixed tag names by index
("tag_barrel", "tag_barrel2", ...), not an arbitrary tag string - "tag" is
accepted (so existing RealRTCW-style .weap assets drop in unmodified) but
only the part index and model actually matter. `parts` is the view-type's
part array (weapFileClient_t::fpParts or ::tpParts) being populated.
======================
*/
static qboolean CG_RW_ParseWeaponLinkPart( int handle, const char *filename, char parts[][MAX_QPATH] ) {
	pc_token_t token;
	int part;
	const char *s;

	if ( !PC_Int_Parse( handle, &part ) ) {
		CG_WeapParseError( handle, filename, "expected part index" );
		return qfalse;
	}
	if ( part < 0 || part >= W_MAX_PARTS ) {
		CG_WeapParseError( handle, filename, "part index %d out of bounds", part );
		return qfalse;
	}

	if ( !trap_PC_ReadToken( handle, &token ) || Q_stricmp( token.string, "{" ) ) {
		CG_WeapParseError( handle, filename, "expected '{'" );
		return qfalse;
	}

	while ( 1 ) {
		if ( !trap_PC_ReadToken( handle, &token ) ) {
			break;
		}
		if ( token.string[0] == '}' ) {
			break;
		}
		if ( !Q_stricmp( token.string, "tag" ) ) {
			if ( !PC_String_Parse( handle, &s ) ) { CG_WeapParseError( handle, filename, "expected tag name" ); return qfalse; }
		} else if ( !Q_stricmp( token.string, "model" ) ) {
			if ( !PC_String_Parse( handle, &s ) ) { CG_WeapParseError( handle, filename, "expected part model filename" ); return qfalse; }
			Q_strncpyz( parts[part], s, MAX_QPATH );
		} else {
			CG_WeapParseError( handle, filename, "unknown token '%s'", token.string );
			return qfalse;
		}
	}

	return qtrue;
}

/*
======================
CG_RW_ParseWeaponLink

Parses "weaponLink { part N { ... } part N { ... } ... }".
======================
*/
static qboolean CG_RW_ParseWeaponLink( int handle, const char *filename, char parts[][MAX_QPATH] ) {
	pc_token_t token;

	if ( !trap_PC_ReadToken( handle, &token ) || Q_stricmp( token.string, "{" ) ) {
		CG_WeapParseError( handle, filename, "expected '{'" );
		return qfalse;
	}

	while ( 1 ) {
		if ( !trap_PC_ReadToken( handle, &token ) ) {
			break;
		}
		if ( token.string[0] == '}' ) {
			break;
		}
		if ( !Q_stricmp( token.string, "part" ) ) {
			if ( !CG_RW_ParseWeaponLinkPart( handle, filename, parts ) ) {
				return qfalse;
			}
		} else {
			CG_WeapParseError( handle, filename, "unknown token '%s'", token.string );
			return qfalse;
		}
	}

	return qtrue;
}

/*
======================
CG_RW_ParseViewType

Parses "{ model "..." flashModel "..." [weaponLink {...}] }" for the
firstPerson/thirdPerson/skeletalThirdPerson blocks. weaponLink is only
accepted when `parts` is non-NULL - WolfTech's renderer only reads
third-person part slots for WP_VENOM's hardcoded tag_barrel case, so
skeletalThirdPerson still passes NULL to reject it.
======================
*/
static qboolean CG_RW_ParseViewType( int handle, const char *filename, char parts[][MAX_QPATH], char *modelPath, char *flashModelPath, size_t pathSize ) {
	pc_token_t token;
	const char *s;

	if ( !trap_PC_ReadToken( handle, &token ) || Q_stricmp( token.string, "{" ) ) {
		CG_WeapParseError( handle, filename, "expected '{'" );
		return qfalse;
	}

	while ( 1 ) {
		if ( !trap_PC_ReadToken( handle, &token ) ) {
			break;
		}
		if ( token.string[0] == '}' ) {
			break;
		}
		if ( !Q_stricmp( token.string, "model" ) ) {
			if ( !PC_String_Parse( handle, &s ) ) { CG_WeapParseError( handle, filename, "expected model filename" ); return qfalse; }
			Q_strncpyz( modelPath, s, pathSize );
		} else if ( !Q_stricmp( token.string, "flashModel" ) ) {
			if ( !PC_String_Parse( handle, &s ) ) { CG_WeapParseError( handle, filename, "expected flashModel filename" ); return qfalse; }
			Q_strncpyz( flashModelPath, s, pathSize );
		} else if ( parts && !Q_stricmp( token.string, "weaponLink" ) ) {
			if ( !CG_RW_ParseWeaponLink( handle, filename, parts ) ) {
				return qfalse;
			}
		} else {
			CG_WeapParseError( handle, filename, "unknown token '%s'", token.string );
			return qfalse;
		}
	}

	return qtrue;
}

/*
======================
CG_RW_ParseClient

Parses the "client { ... }" block of a .weap file.
======================
*/
static qboolean CG_RW_ParseClient( int handle, const char *filename, weaponInfo_t *wi, weapFileClient_t *c ) {
	pc_token_t token;
	const char *s;

	if ( !trap_PC_ReadToken( handle, &token ) || Q_stricmp( token.string, "{" ) ) {
		CG_WeapParseError( handle, filename, "expected '{'" );
		return qfalse;
	}

	while ( 1 ) {
		if ( !trap_PC_ReadToken( handle, &token ) ) {
			break;
		}
		if ( token.string[0] == '}' ) {
			break;
		}

		if ( !Q_stricmp( token.string, "firstPerson" ) ) {
			if ( !CG_RW_ParseViewType( handle, filename, c->fpParts, c->fpModel, c->fpFlashModel, sizeof( c->fpModel ) ) ) {
				return qfalse;
			}
		} else if ( !Q_stricmp( token.string, "thirdPerson" ) ) {
			if ( !CG_RW_ParseViewType( handle, filename, c->tpParts, c->tpModel, c->tpFlashModel, sizeof( c->tpModel ) ) ) {
				return qfalse;
			}
		} else if ( !Q_stricmp( token.string, "skeletalThirdPerson" ) ) {
			if ( !CG_RW_ParseViewType( handle, filename, NULL, c->sktpModel, c->sktpFlashModel, sizeof( c->sktpModel ) ) ) {
				return qfalse;
			}
		} else if ( !Q_stricmp( token.string, "weaponConfig" ) ) {
			if ( !PC_String_Parse( handle, &s ) ) { CG_WeapParseError( handle, filename, "expected weaponConfig filename" ); return qfalse; }
			Q_strncpyz( c->configPath, s, sizeof( c->configPath ) );
		} else if ( !Q_stricmp( token.string, "handsModel" ) ) {
			if ( !PC_String_Parse( handle, &s ) ) { CG_WeapParseError( handle, filename, "expected handsModel filename" ); return qfalse; }
			Q_strncpyz( c->handsModel, s, sizeof( c->handsModel ) );
		} else if ( !Q_stricmp( token.string, "standModel" ) ) {
			if ( !PC_String_Parse( handle, &s ) ) { CG_WeapParseError( handle, filename, "expected standModel filename" ); return qfalse; }
			Q_strncpyz( c->standModel, s, sizeof( c->standModel ) );
		} else if ( !Q_stricmp( token.string, "pickupModel" ) ) {
			if ( !PC_String_Parse( handle, &s ) ) { CG_WeapParseError( handle, filename, "expected pickupModel filename" ); return qfalse; }
			wi->weaponModel[W_PU_MODEL] = trap_R_RegisterModel( s );
		} else if ( !Q_stricmp( token.string, "weaponIcon" ) ) {
			if ( !PC_String_Parse( handle, &s ) ) { CG_WeapParseError( handle, filename, "expected weaponIcon filename" ); return qfalse; }
			Q_strncpyz( c->weaponIcon, s, sizeof( c->weaponIcon ) );
		} else if ( !Q_stricmp( token.string, "weaponSelectedIcon" ) ) {
			if ( !PC_String_Parse( handle, &s ) ) { CG_WeapParseError( handle, filename, "expected weaponSelectedIcon filename" ); return qfalse; }
			Q_strncpyz( c->weaponSelectedIcon, s, sizeof( c->weaponSelectedIcon ) );
		} else if ( !Q_stricmp( token.string, "ammoIcon" ) ) {
			if ( !PC_String_Parse( handle, &s ) ) { CG_WeapParseError( handle, filename, "expected ammoIcon filename" ); return qfalse; }
			Q_strncpyz( c->ammoIcon, s, sizeof( c->ammoIcon ) );
		} else if ( !Q_stricmp( token.string, "flashDlightColor" ) ) {
			if ( !PC_Float_Parse( handle, &wi->flashDlightColor[0] ) ||
				 !PC_Float_Parse( handle, &wi->flashDlightColor[1] ) ||
				 !PC_Float_Parse( handle, &wi->flashDlightColor[2] ) ) {
				CG_WeapParseError( handle, filename, "expected flashDlightColor as r g b" );
				return qfalse;
			}
		} else if ( !Q_stricmp( token.string, "flashSound" ) ) {
			if ( !PC_String_Parse( handle, &s ) ) { CG_WeapParseError( handle, filename, "expected flashSound filename" ); return qfalse; }
			CG_WeapAppendSound( wi->flashSound, s );
		} else if ( !Q_stricmp( token.string, "flashEchoSound" ) ) {
			if ( !PC_String_Parse( handle, &s ) ) { CG_WeapParseError( handle, filename, "expected flashEchoSound filename" ); return qfalse; }
			CG_WeapAppendSound( wi->flashEchoSound, s );
		} else if ( !Q_stricmp( token.string, "lastShotSound" ) ) {
			if ( !PC_String_Parse( handle, &s ) ) { CG_WeapParseError( handle, filename, "expected lastShotSound filename" ); return qfalse; }
			CG_WeapAppendSound( wi->lastShotSound, s );
		} else if ( !Q_stricmp( token.string, "switchSound" ) ) {
			if ( !PC_String_Parse( handle, &s ) ) { CG_WeapParseError( handle, filename, "expected switchSound filename" ); return qfalse; }
			CG_WeapAppendSound( wi->switchSound, s );
		} else if ( !Q_stricmp( token.string, "reloadSound" ) ) {
			if ( !PC_String_Parse( handle, &s ) ) { CG_WeapParseError( handle, filename, "expected reloadSound filename" ); return qfalse; }
			wi->reloadSound = trap_S_RegisterSound( s );
		} else if ( !Q_stricmp( token.string, "reloadSoundFast" ) ) {
			if ( !PC_String_Parse( handle, &s ) ) { CG_WeapParseError( handle, filename, "expected reloadSoundFast filename" ); return qfalse; }
			wi->reloadSoundFast = trap_S_RegisterSound( s );
		} else if ( !Q_stricmp( token.string, "reloadFullSound" ) ) {
			if ( !PC_String_Parse( handle, &s ) ) { CG_WeapParseError( handle, filename, "expected reloadFullSound filename" ); return qfalse; }
			wi->reloadFullSound = trap_S_RegisterSound( s );
		} else if ( !Q_stricmp( token.string, "reloadFullSoundFast" ) ) {
			if ( !PC_String_Parse( handle, &s ) ) { CG_WeapParseError( handle, filename, "expected reloadFullSoundFast filename" ); return qfalse; }
			wi->reloadFullSoundFast = trap_S_RegisterSound( s );
		} else if ( !Q_stricmp( token.string, "reloadSoundAi" ) ) {
			if ( !PC_String_Parse( handle, &s ) ) { CG_WeapParseError( handle, filename, "expected reloadSoundAi filename" ); return qfalse; }
			wi->reloadSoundAi = trap_S_RegisterSound( s );
		} else if ( !Q_stricmp( token.string, "pumpSound" ) ) {
			if ( !PC_String_Parse( handle, &s ) ) { CG_WeapParseError( handle, filename, "expected pumpSound filename" ); return qfalse; }
			wi->pumpSound = trap_S_RegisterSound( s );
		} else if ( !Q_stricmp( token.string, "bounceSound" ) ) {
			if ( !PC_String_Parse( handle, &s ) ) { CG_WeapParseError( handle, filename, "expected bounceSound filename" ); return qfalse; }
			wi->bounceSound = trap_S_RegisterSound( s );
		} else if ( !Q_stricmp( token.string, "overheatSound" ) ) {
			if ( !PC_String_Parse( handle, &s ) ) { CG_WeapParseError( handle, filename, "expected overheatSound filename" ); return qfalse; }
			wi->overheatSound = trap_S_RegisterSound( s );
		} else if ( !Q_stricmp( token.string, "spinupSound" ) ) {
			if ( !PC_String_Parse( handle, &s ) ) { CG_WeapParseError( handle, filename, "expected spinupSound filename" ); return qfalse; }
			wi->spinupSound = trap_S_RegisterSound( s );
		} else if ( !Q_stricmp( token.string, "spindownSound" ) ) {
			if ( !PC_String_Parse( handle, &s ) ) { CG_WeapParseError( handle, filename, "expected spindownSound filename" ); return qfalse; }
			wi->spindownSound = trap_S_RegisterSound( s );
		} else if ( !Q_stricmp( token.string, "fireModeSwitchSound" ) ) {
			if ( !PC_String_Parse( handle, &s ) ) { CG_WeapParseError( handle, filename, "expected fireModeSwitchSound filename" ); return qfalse; }
			wi->fireModeSwitchSound = trap_S_RegisterSound( s );
		} else if ( !Q_stricmp( token.string, "modModel" ) ) {
			int modIndex;
			if ( !PC_Int_Parse( handle, &modIndex ) ) { CG_WeapParseError( handle, filename, "expected modModel index" ); return qfalse; }
			if ( modIndex < 0 || modIndex >= W_NUM_TYPES ) { CG_WeapParseError( handle, filename, "modModel index %d out of bounds", modIndex ); return qfalse; }
			if ( !PC_String_Parse( handle, &s ) ) { CG_WeapParseError( handle, filename, "expected modModel filename" ); return qfalse; }
			wi->modModel[modIndex] = trap_R_RegisterModel( s );
		} else if ( !Q_stricmp( token.string, "missileModel" ) ) {
			if ( !PC_String_Parse( handle, &s ) ) { CG_WeapParseError( handle, filename, "expected missileModel filename" ); return qfalse; }
			wi->missileModel = trap_R_RegisterModel( s );
		} else if ( !Q_stricmp( token.string, "shellModel" ) ) {
			if ( !PC_String_Parse( handle, &s ) ) { CG_WeapParseError( handle, filename, "expected shellModel filename" ); return qfalse; }
			wi->shellModel = trap_R_RegisterModel( s );
		} else if ( !Q_stricmp( token.string, "droppedModel" ) ) {
			if ( !PC_String_Parse( handle, &s ) ) { CG_WeapParseError( handle, filename, "expected droppedModel filename" ); return qfalse; }
			wi->droppedModel = trap_R_RegisterModel( s );
		} else if ( !Q_stricmp( token.string, "missileSound" ) ) {
			if ( !PC_String_Parse( handle, &s ) ) { CG_WeapParseError( handle, filename, "expected missileSound filename" ); return qfalse; }
			wi->missileSound = trap_S_RegisterSound( s );
		} else if ( !Q_stricmp( token.string, "landSound" ) ) {
			if ( !PC_String_Parse( handle, &s ) ) { CG_WeapParseError( handle, filename, "expected landSound filename" ); return qfalse; }
			wi->landSound = trap_S_RegisterSound( s );
		} else if ( !Q_stricmp( token.string, "healSound" ) ) {
			if ( !PC_String_Parse( handle, &s ) ) { CG_WeapParseError( handle, filename, "expected healSound filename" ); return qfalse; }
			wi->healSound = trap_S_RegisterSound( s );
		} else if ( !Q_stricmp( token.string, "missileTrailFunc" ) ) {
			if ( !PC_String_Parse( handle, &s ) ) { CG_WeapParseError( handle, filename, "expected missileTrailFunc name" ); return qfalse; }
			wi->missileTrailFunc = CG_WeapMissileTrailFuncForName( s );
			if ( !wi->missileTrailFunc ) {
				CG_WeapParseError( handle, filename, "unknown missileTrailFunc '%s'", s );
				return qfalse;
			}
		} else if ( !Q_stricmp( token.string, "missileDlight" ) ) {
			if ( !PC_Float_Parse( handle, &wi->missileDlight ) ) { CG_WeapParseError( handle, filename, "expected missileDlight value" ); return qfalse; }
		} else if ( !Q_stricmp( token.string, "missileDlightColor" ) ) {
			if ( !PC_Float_Parse( handle, &wi->missileDlightColor[0] ) ||
				 !PC_Float_Parse( handle, &wi->missileDlightColor[1] ) ||
				 !PC_Float_Parse( handle, &wi->missileDlightColor[2] ) ) {
				CG_WeapParseError( handle, filename, "expected missileDlightColor as r g b" );
				return qfalse;
			}
		} else if ( !Q_stricmp( token.string, "missileTrailTime" ) ) {
			if ( !PC_Float_Parse( handle, &wi->wiTrailTime ) ) { CG_WeapParseError( handle, filename, "expected missileTrailTime value" ); return qfalse; }
		} else if ( !Q_stricmp( token.string, "missileTrailRadius" ) ) {
			if ( !PC_Float_Parse( handle, &wi->trailRadius ) ) { CG_WeapParseError( handle, filename, "expected missileTrailRadius value" ); return qfalse; }
		} else if ( !Q_stricmp( token.string, "ejectBrassFunc" ) ) {
			if ( !PC_String_Parse( handle, &s ) ) { CG_WeapParseError( handle, filename, "expected ejectBrassFunc name" ); return qfalse; }
			wi->ejectBrassFunc = CG_WeapEjectBrassFuncForName( s );
			if ( !wi->ejectBrassFunc ) {
				CG_WeapParseError( handle, filename, "unknown ejectBrassFunc '%s'", s );
				return qfalse;
			}
		} else {
			CG_WeapParseError( handle, filename, "unknown token '%s'", token.string );
			return qfalse;
		}
	}

	return qtrue;
}

/*
======================
CG_RegisterWeaponFromFile

Cgame-only cosmetic registration for weapons with ammoTable[].weapFile set.
Loads weapons/<weapFile>, expecting "weaponDef { client { ... } }". This is
now the only weapon registration path - every real weapon has a .weap file,
so CG_RegisterWeapon errors out if one is missing rather than falling back
to hardcoded per-weapon asset paths.
Venom's third-person spinning belt part (previously hardcoded from a
"_barrel6b.md3" suffix convention) still has no .weap equivalent - it needs
the actual belt model path added as a normal "part N" entry in venom.weap
once known.
======================
*/
static qboolean CG_RegisterWeaponFromFile( int weaponNum, weaponInfo_t *weaponInfo, const gitem_t *item ) {
	pc_token_t       token;
	int              handle;
	char             filename[MAX_QPATH];
	char             path[MAX_QPATH];
	weapFileClient_t c;
	vec3_t           mins, maxs;
	int              i;
	gitem_t          *ammo;

	Com_Memset( &c, 0, sizeof( c ) );

	Com_sprintf( filename, sizeof( filename ), "weapons/%s", GetWeaponTableData( weaponNum )->weapFile );

	handle = trap_PC_LoadSource( filename );
	if ( !handle ) {
		CG_Printf( S_COLOR_RED "WARNING: failed to load %s\n", filename );
		return qfalse;
	}

	if ( !trap_PC_ReadToken( handle, &token ) || Q_stricmp( token.string, "weaponDef" ) ) {
		CG_WeapParseError( handle, filename, "expected 'weaponDef'" );
		return qfalse;
	}
	if ( !trap_PC_ReadToken( handle, &token ) || Q_stricmp( token.string, "{" ) ) {
		CG_WeapParseError( handle, filename, "expected '{'" );
		return qfalse;
	}

	while ( 1 ) {
		if ( !trap_PC_ReadToken( handle, &token ) ) {
			break;
		}
		if ( token.string[0] == '}' ) {
			break;
		}
		if ( !Q_stricmp( token.string, "client" ) ) {
			if ( !CG_RW_ParseClient( handle, filename, weaponInfo, &c ) ) {
				return qfalse;
			}
		} else {
			CG_WeapParseError( handle, filename, "unknown token '%s'", token.string );
			return qfalse;
		}
	}

	trap_PC_FreeSource( handle );

	if ( !c.fpModel[0] || !c.tpModel[0] ) {
		CG_Printf( S_COLOR_RED "WARNING: %s missing firstPersonModel/thirdPersonModel\n", filename );
		return qfalse;
	}

	weaponInfo->weaponModel[W_TP_MODEL] = trap_R_RegisterModel( c.tpModel );
	weaponInfo->weaponModel[W_FP_MODEL] = trap_R_RegisterModel( c.fpModel );
	if ( c.sktpModel[0] ) {
		weaponInfo->weaponModel[W_SKTP_MODEL] = trap_R_RegisterModel( c.sktpModel );
	}

	if ( !weaponInfo->weaponModel[W_FP_MODEL] || !cg_drawFPGun.integer ) {
		weaponInfo->weaponModel[W_FP_MODEL] = weaponInfo->weaponModel[W_TP_MODEL];
	}

	if ( !weaponInfo->weaponModel[W_TP_MODEL] ) {
		return qfalse;
	}

	// weapon.cfg animation frames - explicit path if given, else the same convention-based lookup as the legacy path
	if ( c.configPath[0] ) {
		Q_strncpyz( path, c.configPath, sizeof( path ) );
	} else {
		COM_StripFilename( c.fpModel, path );
		Q_strcat( path, sizeof( path ), "weapon.cfg" );
	}
	if ( !CG_ParseWeaponConfig( path, weaponInfo, weaponNum ) ) {
		CG_Error( "Couldn't register weapon %i (%s) (failed to parse weapon.cfg)", weaponNum, path );
	}

	trap_R_ModelBounds( weaponInfo->weaponModel[W_TP_MODEL], mins, maxs );
	for ( i = 0 ; i < 3 ; i++ ) {
		weaponInfo->weaponMidpoint[i] = mins[i] + 0.5 * ( maxs[i] - mins[i] );
	}

	// muzzle flash model - explicit path if given, else the same "_flash.md3" suffix convention as the legacy path, per view type
	if ( c.tpFlashModel[0] ) {
		weaponInfo->flashModel[W_TP_MODEL] = trap_R_RegisterModel( c.tpFlashModel );
	} else {
		COM_StripExtension( c.tpModel, path, sizeof( path ) );
		Q_strcat( path, sizeof( path ), "_flash.md3" );
		weaponInfo->flashModel[W_TP_MODEL] = trap_R_RegisterModel( path );
	}

	if ( c.fpFlashModel[0] ) {
		weaponInfo->flashModel[W_FP_MODEL] = trap_R_RegisterModel( c.fpFlashModel );
	} else {
		COM_StripExtension( c.fpModel, path, sizeof( path ) );
		Q_strcat( path, sizeof( path ), "_flash.md3" );
		weaponInfo->flashModel[W_FP_MODEL] = trap_R_RegisterModel( path );
	}

	if ( c.sktpModel[0] ) {
		if ( c.sktpFlashModel[0] ) {
			weaponInfo->flashModel[W_SKTP_MODEL] = trap_R_RegisterModel( c.sktpFlashModel );
		} else {
			COM_StripExtension( c.sktpModel, path, sizeof( path ) );
			Q_strcat( path, sizeof( path ), "_flash.md3" );
			weaponInfo->flashModel[W_SKTP_MODEL] = trap_R_RegisterModel( path );
		}
	}

	// barrel/part models (e.g. bolt/receiver pieces) - attached at render time to fixed
	// "tag_barrel"/"tag_barrelN" tags by index (see CG_AddPlayerWeapon). Third-person parts
	// are only ever read back for WP_VENOM's hardcoded tag_barrel case, but any weapon's
	// .weap can still declare them here.
	for ( i = 0; i < W_MAX_PARTS; i++ ) {
		if ( c.fpParts[i][0] ) {
			weaponInfo->wpPartModels[W_FP_MODEL][i] = trap_R_RegisterModel( c.fpParts[i] );
		}
		if ( c.tpParts[i][0] ) {
			weaponInfo->wpPartModels[W_TP_MODEL][i] = trap_R_RegisterModel( c.tpParts[i] );
		}
	}

	// hands model
	if ( c.handsModel[0] ) {
		weaponInfo->handsModel = trap_R_RegisterModel( c.handsModel );
	} else {
		COM_StripExtension( c.fpModel, path, sizeof( path ) );
		Q_strcat( path, sizeof( path ), "_hand.md3" );
		weaponInfo->handsModel = trap_R_RegisterModel( path );
	}
	if ( !weaponInfo->handsModel ) {
		weaponInfo->handsModel = trap_R_RegisterModel( "models/weapons2/shotgun/shotgun_hand.md3" );
	}

	// pickup 'stand' model
	if ( c.standModel[0] ) {
		weaponInfo->standModel = trap_R_RegisterModel( c.standModel );
	} else {
		COM_StripExtension( c.tpModel, path, sizeof( path ) );
		Q_strcat( path, sizeof( path ), "_stand.md3" );
		weaponInfo->standModel = trap_R_RegisterModel( path );
	}

	// icons
	if ( c.weaponIcon[0] ) {
		weaponInfo->weaponIcon[0] = trap_R_RegisterShader( c.weaponIcon );
		weaponInfo->weaponIcon[1] = c.weaponSelectedIcon[0] ?
			trap_R_RegisterShader( c.weaponSelectedIcon ) :
			trap_R_RegisterShader( va( "%s_select", c.weaponIcon ) );
	}
	if ( c.ammoIcon[0] ) {
		weaponInfo->ammoIcon = trap_R_RegisterShader( c.ammoIcon );
	} else if ( item->ammoicon ) {
		weaponInfo->ammoIcon = trap_R_RegisterShader( item->ammoicon );
	}

	// ammo pickup model - not weapon-specific cosmetic data, still sourced from the shared ammo item's bg_itemlist entry
	for ( ammo = bg_itemlist + 1 ; ammo->classname ; ammo++ ) {
		if ( ammo->giType == IT_AMMO && ammo->giTag == BG_FindAmmoForWeapon( weaponNum ) ) {
			break;
		}
	}
	if ( ammo->classname && ammo->world_model[0] ) {
		weaponInfo->ammoModel = trap_R_RegisterModel( ammo->world_model[0] );
	}

	// default fast/full/AI reload sounds to the base reload sound when no distinct asset is registered
	if ( !weaponInfo->reloadSoundFast ) {
		weaponInfo->reloadSoundFast = weaponInfo->reloadSound;
	}
	if ( !weaponInfo->reloadFullSound ) {
		weaponInfo->reloadFullSound = weaponInfo->reloadSound;
	}
	if ( !weaponInfo->reloadFullSoundFast ) {
		weaponInfo->reloadFullSoundFast = weaponInfo->reloadSoundFast;
	}
	if ( !weaponInfo->reloadSoundAi ) {
		weaponInfo->reloadSoundAi = weaponInfo->reloadSound;
	}

	return qtrue;
}


/*
=================
CG_RegisterWeapon

The server says this item is used on this level
=================
*/
void CG_RegisterWeapon( int weaponNum ) {
	weaponInfo_t    *weaponInfo;
	gitem_t         *item;

	weaponInfo = &cg_weapons[weaponNum];

	// don't bother trying
	switch ( weaponNum ) {
	case WP_NONE:
	case WP_MONSTER_ATTACK1:
	case WP_MONSTER_ATTACK2:
	case WP_MONSTER_ATTACK3:
	case WP_SNIPER:
	case WP_MORTAR:

// (SA) i don't know about these, but we don't have models for 'em
	case WP_GRENADE_SMOKE:
	case WP_MEDIC_HEAL:
	// JPW NERVE -- WP_SMOKETRAIL is a cosmetic-only debris tag, not a real weapon.
	case WP_SMOKETRAIL:
		return;
	default:
		break;
	}

	if ( weaponInfo->registered ) {
		return;
	}

	memset( weaponInfo, 0, sizeof( *weaponInfo ) );
	weaponInfo->registered = qtrue;

	for ( item = bg_itemlist + 1 ; item->classname ; item++ ) {
		if ( item->giType == IT_WEAPON && item->giTag == weaponNum ) {
			weaponInfo->item = item;
			break;
		}
	}
	if ( !item->classname ) {
		CG_Error( "Couldn't find weapon %i", weaponNum );
	}

	CG_RegisterItemVisuals( item - bg_itemlist );

	if ( GetWeaponTableData( weaponNum )->weapFile ) {
		if ( !CG_RegisterWeaponFromFile( weaponNum, weaponInfo, item ) ) {
			CG_Printf( S_COLOR_RED "WARNING: failed to register weapon %i from %s, cosmetics may be missing\n", weaponNum, GetWeaponTableData( weaponNum )->weapFile );
		}
		return;
	}

	CG_Error( "Weapon %i (%s) has no .weap file assigned", weaponNum, item->classname );
}

/*
=================
CG_RegisterItemVisuals

The server says this item is used on this level
=================
*/
void CG_RegisterItemVisuals( int itemNum ) {
	itemInfo_t      *itemInfo;
	gitem_t         *item;
	int i;

	itemInfo = &cg_items[ itemNum ];
	if ( itemInfo->registered ) {
		return;
	}

	item = &bg_itemlist[ itemNum ];

	memset( itemInfo, 0, sizeof( *itemInfo ) );

	for ( i = 0; i < MAX_ITEM_MODELS; i++ )
		itemInfo->models[i] = trap_R_RegisterModel( item->world_model[i] );


	itemInfo->icons[0] = trap_R_RegisterShader( item->icon );
	if ( item->giType == IT_HOLDABLE ) {
		// (SA) register alternate icons (since holdables can have multiple uses, they might have different icons to represent how many uses are left)
		for ( i = 1; i < MAX_ITEM_ICONS; i++ )
			itemInfo->icons[i] = trap_R_RegisterShader( va( "%s%i", item->icon, i + 1 ) );
	}

	if ( item->giType == IT_WEAPON ) {
		CG_RegisterWeapon( item->giTag );
	}

	itemInfo->registered = qtrue;   //----(SA)	moved this down after the registerweapon()

	wolfkickModel = trap_R_RegisterModel( "models/weapons2/foot/v_wolfoot_10f.md3" );
	hWeaponSnd = trap_S_RegisterSound( "sound/weapons/mg42/37mm.wav" );
	hWeaponEchoSnd = trap_S_RegisterSound( "sound/weapons/mg42/37mm_far.wav" );

	hflakWeaponSnd = trap_S_RegisterSound( "sound/weapons/flak/flak.wav" );

	notebookModel = trap_R_RegisterModel( "models/mapobjects/book/book.md3" );

	propellerModel = trap_R_RegisterModel( "models/mapobjects/vehicles/m109_prop.md3" );

	// JPW NERVE had to put this somewhere, this seems OK
	if ( cg_gameType.integer <= GT_SINGLE_PLAYER ) {
		maxWeapBanks = MAX_WEAP_BANKS;
		maxWeapsInBank = MAX_WEAPS_IN_BANK;
	}
}


/*
========================================================================================

VIEW WEAPON

========================================================================================
*/


//
// weapon animations
//

/*
==============
CG_GetPartFramesFromWeap
	get animation info from the parent if necessary
==============
*/
qboolean CG_GetPartFramesFromWeap( centity_t *cent, refEntity_t *part, refEntity_t *parent, int partid, weaponInfo_t *wi ) {
	int i;
	int frameoffset = 0;
	animation_t *anim;

	anim = cent->pe.weap.animation;

	if ( partid == W_MAX_PARTS ) {
		return qtrue;   // primary weap model drawn for all frames right now
	}

	// check draw bit
	if ( anim->moveSpeed & ( 1 << ( partid + 8 ) ) ) {    // hide bits are in high byte
		return qfalse;  // not drawn for current sequence
	}

	// find part's start frame for this animation sequence
	for ( i = 0; i < cent->pe.weap.animationNumber; i++ ) {
		if ( wi->weapAnimations[i].moveSpeed & ( 1 << partid ) ) {     // this part has animation for this sequence
			frameoffset += wi->weapAnimations[i].numFrames;
		}
	}

	// now set the correct frame into the part
	if ( anim->moveSpeed & ( 1 << partid ) ) {
		part->backlerp  = parent->backlerp;
		part->oldframe  = frameoffset + ( parent->oldframe - anim->firstFrame );
		part->frame     = frameoffset + ( parent->frame - anim->firstFrame );
	}

	return qtrue;
}


/*
===============
CG_SetWeapLerpFrameAnimation

may include ANIM_TOGGLEBIT
===============
*/
static void CG_SetWeapLerpFrameAnimation( weaponInfo_t *wi, lerpFrame_t *lf, int newAnimation ) {
	animation_t *anim;

	lf->animationNumber = newAnimation;
	newAnimation &= ~ANIM_TOGGLEBIT;

	if ( newAnimation < 0 || newAnimation >= MAX_WP_ANIMATIONS ) {
		CG_Error( "Bad animation number (CG_SWLFA): %i", newAnimation );
	}

	anim = &wi->weapAnimations[ newAnimation ];

	lf->animation       = anim;
	lf->animationTime   = lf->frameTime + anim->initialLerp;

	if ( cg_debugAnim.integer & 2 ) {
		CG_Printf( "Weap Anim: %d\n", newAnimation );
	}
}


/*
===============
CG_ClearWeapLerpFrame
===============
*/
void CG_ClearWeapLerpFrame( weaponInfo_t *wi, lerpFrame_t *lf, int animationNumber ) {
	lf->frameTime = lf->oldFrameTime = cg.time;
	CG_SetWeapLerpFrameAnimation( wi, lf, animationNumber );
	lf->oldFrame = lf->frame = lf->animation->firstFrame;

}


/*
===============
CG_RunWeapLerpFrame

Sets cg.snap, cg.oldFrame, and cg.backlerp
cg.time should be between oldFrameTime and frameTime after exit
===============
*/
static void CG_RunWeapLerpFrame( clientInfo_t *ci, weaponInfo_t *wi, lerpFrame_t *lf, int newAnimation, float speedScale ) {
	int f;
	animation_t *anim;

	// debugging tool to get no animations
	if ( cg_animSpeed.integer == 0 ) {
		lf->oldFrame = lf->frame = lf->backlerp = 0;
		return;
	}

	// see if the animation sequence is switching
	if ( !lf->animation ) {
		CG_ClearWeapLerpFrame( wi, lf, newAnimation );
	} else if ( newAnimation != lf->animationNumber )   {
		if ( ( newAnimation & ~ANIM_TOGGLEBIT ) == WEAP_RAISE ||
			 ( newAnimation & ~ANIM_TOGGLEBIT ) == WEAP_SPRINTOUT ||
			 ( newAnimation & ~ANIM_TOGGLEBIT ) == WEAP_ALTSWITCHFROM ||
			 ( newAnimation & ~ANIM_TOGGLEBIT ) == WEAP_ALTSWITCHTO ) {
			CG_ClearWeapLerpFrame( wi, lf, newAnimation );   // clear when switching to raise (since it should be out of view anyway)
		} else {
			CG_SetWeapLerpFrameAnimation( wi, lf, newAnimation );
		}
	}

	// if we have passed the current frame, move it to
	// oldFrame and calculate a new frame
	if ( cg.time >= lf->frameTime ) {
		lf->oldFrame = lf->frame;
		lf->oldFrameTime = lf->frameTime;

		// get the next frame based on the animation
		anim = lf->animation;
		if ( !anim->frameLerp ) {
			return;     // shouldn't happen
		}
		if ( cg.time < lf->animationTime ) {
			lf->frameTime = lf->animationTime;      // initial lerp
		} else {
			lf->frameTime = lf->oldFrameTime + anim->frameLerp;
		}
		f = ( lf->frameTime - lf->animationTime ) / anim->frameLerp;
		f *= speedScale;        // adjust for haste, etc
		if ( f >= anim->numFrames ) {
			f -= anim->numFrames;
			if ( anim->loopFrames ) {
				f %= anim->loopFrames;
				f += anim->numFrames - anim->loopFrames;
			} else {
				f = anim->numFrames - 1;
				// the animation is stuck at the end, so it
				// can immediately transition to another sequence
				lf->frameTime = cg.time;
			}
		}
		lf->frame = anim->firstFrame + f;
		if ( cg.time > lf->frameTime ) {
			lf->frameTime = cg.time;
			if ( cg_debugAnim.integer ) {
//				CG_Printf( "Clamp lf->frameTime\n" );
			}
		}
	}

	if ( lf->frameTime > cg.time + 200 ) {
		lf->frameTime = cg.time;
	}

	if ( lf->oldFrameTime > cg.time ) {
		lf->oldFrameTime = cg.time;
	}
	// calculate current lerp value
	if ( lf->frameTime == lf->oldFrameTime ) {
		lf->backlerp = 0;
	} else {
		lf->backlerp = 1.0 - (float)( cg.time - lf->oldFrameTime ) / ( lf->frameTime - lf->oldFrameTime );
	}
}



/*
==============
CG_WeaponAnimation
==============
*/

//----(SA)	modified.  this is now client-side only (server does not dictate weapon animation info)
static void CG_WeaponAnimation( playerState_t *ps, weaponInfo_t *weapon, int *weapOld, int *weap, float *weapBackLerp ) {

	centity_t *cent = &cg.predictedPlayerEntity;
	clientInfo_t *ci = &cgs.clientinfo[ ps->clientNum ];

	if ( cg_noPlayerAnims.integer ) {
		*weapOld = *weap = 0;
		return;
	}

	CG_RunWeapLerpFrame( ci, weapon, &cent->pe.weap, ps->weapAnim, 1 );

	*weapOld        = cent->pe.weap.oldFrame;
	*weap           = cent->pe.weap.frame;
	*weapBackLerp   = cent->pe.weap.backlerp;

	if ( cg_debugAnim.integer == 3 ) {
		CG_Printf( "oldframe: %d   frame: %d   backlerp: %f\n", cent->pe.weap.oldFrame, cent->pe.weap.frame, cent->pe.weap.backlerp );
	}
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////


// (SA) it wasn't used anyway


/*
==============
CG_CalculateWeaponPosition
==============
*/
static void CG_CalculateWeaponPosition( vec3_t origin, vec3_t angles ) {
	int delta;
	float leanscale;

	VectorCopy( cg.refdef.vieworg, origin );
	VectorCopy( cg.refdefViewAngles, angles );

	// adjust 'lean' into weapon
	if ( cg.predictedPlayerState.leanf != 0 ) {
		vec3_t right, up;

		leanscale = 1.0f;

		switch ( cg.predictedPlayerState.weapon ) {
		case WP_SNOOPER:
			leanscale = 3.0f;
			break;
		case WP_FLAMETHROWER:
		case WP_TESLA:
		case WP_MAUSER:
			leanscale = 2.0f;
			break;

			// never adjust
		case WP_KNIFE:
		case WP_GRENADE_LAUNCHER:
		case WP_GRENADE_PINEAPPLE:
			break;

			// adjust when leaning right (in case of reload)
		default:
			if ( cg.predictedPlayerState.leanf > 0 ) {
				leanscale = 1.3f;
			}
			break;
		}

		// reverse the roll on the weapon so it stays relatively level
		angles[ROLL] -= cg.predictedPlayerState.leanf / ( leanscale * 2.0f );
		AngleVectors( angles, NULL, right, up );
		VectorMA( origin, angles[ROLL], right, origin );

		// pitch the gun down a bit to show that firing is not allowed when leaning
		angles[PITCH] += ( fabs( cg.predictedPlayerState.leanf ) / 2.0f );

		// this gives you some impression that the weapon stays in relatively the same
		// position while you lean, so you appear to 'peek' over the weapon
		AngleVectors( cg.refdefViewAngles, NULL, right, NULL );
		VectorMA( origin, -cg.predictedPlayerState.leanf / 4.0f, right, origin );
	}


	// movement sway - smooth full-stride oval drift (lateral + out-of-phase dip) plus a speed droop
	if ( cg.xyspeed > 0 ) {
		vec3_t moveRight;
		float bobPhase, lateral, speedFrac, strafe;

		AngleVectors( cg.refdefViewAngles, NULL, moveRight, NULL );

		speedFrac = cg.xyspeed / 320.0f;
		if ( speedFrac > 1.0f ) {
			speedFrac = 1.0f;
		}
		if ( cg.predictedPlayerState.pm_flags & PMF_SPRINTING ) {
			speedFrac *= cg_gunSwaySprintScale.value;
		}

		bobPhase = ( cg.predictedPlayerState.bobCycle / 256.0f ) * 2.0f * M_PI;
		lateral = sin( bobPhase ) * cg_gunSwayAmp.value * speedFrac;

		// strafe lean - bias sway toward whichever side the player is actually moving
		strafe = DotProduct( cg.predictedPlayerState.velocity, moveRight ) / 320.0f;
		if ( strafe > 1.0f ) {
			strafe = 1.0f;
		} else if ( strafe < -1.0f ) {
			strafe = -1.0f;
		}
		lateral += strafe * cg_gunSwayStrafeAmp.value;

		VectorMA( origin, lateral, moveRight, origin );
		origin[2] -= cg.bobfracsin * cg_gunSwayDip.value * speedFrac;

		angles[ROLL] += lateral * cg_gunSwayRoll.value;
		angles[PITCH] += speedFrac * cg_gunSwayLower.value;
	}

	// drop the weapon when landing
	delta = cg.time - cg.landTime;
	if ( delta < LAND_DEFLECT_TIME ) {
		origin[2] += cg.landChange * 0.25 * delta / LAND_DEFLECT_TIME;
	} else if ( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME ) {
		origin[2] += cg.landChange * 0.25 *
					 ( LAND_DEFLECT_TIME + LAND_RETURN_TIME - delta ) / LAND_RETURN_TIME;
	}

#if 0
	// drop the weapon when stair climbing
	delta = cg.time - cg.stepTime;
	if ( delta < STEP_TIME / 2 ) {
		origin[2] -= cg.stepChange * 0.25 * delta / ( STEP_TIME / 2 );
	} else if ( delta < STEP_TIME ) {
		origin[2] -= cg.stepChange * 0.25 * ( STEP_TIME - delta ) / ( STEP_TIME / 2 );
	}
#endif

	// idle drift - out-of-phase sinusoids per axis trace a slow oval instead of one straight line
	angles[YAW] += sin( cg.time * cg_gunSwayIdleSpeed.value ) * cg_gunSwayIdleAmp.value;
	angles[PITCH] += cos( cg.time * cg_gunSwayIdleSpeed.value * 0.9f ) * cg_gunSwayIdleAmp.value * 0.6f;
	angles[ROLL] += sin( cg.time * cg_gunSwayIdleSpeed.value * 1.3f ) * cg_gunSwayIdleAmp.value * 0.3f;

	// RF, subtract the kickAngles
	VectorMA( angles, -1.0, cg.kickAngles, angles );

}


// Ridah
/*
===============
CG_FlamethrowerFlame
===============
*/
static void CG_FlamethrowerFlame( centity_t *cent, vec3_t origin ) {

	if ( cent->currentState.weapon != WP_FLAMETHROWER ) {
		return;
	}

	if ( cent->currentState.number == cg.snap->ps.clientNum ) {
		if ( cg.snap->ps.weapon != WP_FLAMETHROWER || ( cg.snap->ps.weaponstate != WEAPON_FIRING && cg.snap->ps.weaponstate != WEAPON_READY ) ) {
			return;
		}
	}

	CG_FireFlameChunks( cent, origin, cent->lerpAngles, 1.0, qtrue, 0 );

	return;
}
// done.

/*
======================
CG_MachinegunSpinAngle
======================
*/
/*
//#define		SPIN_SPEED	0.9
//#define		COAST_TIME	1000
#define		SPIN_SPEED	1
#define		COAST_TIME	2000

static float	CG_MachinegunSpinAngle( centity_t *cent ) {
	int		delta;
	float	angle;
	float	speed;

	delta = cg.time - cent->pe.barrelTime;
	if ( cent->pe.barrelSpinning ) {
		angle = cent->pe.barrelAngle + delta * SPIN_SPEED;
	} else {
		if ( delta > COAST_TIME ) {
			delta = COAST_TIME;
		}

		speed = 0.5 * ( SPIN_SPEED + (float)( COAST_TIME - delta ) / COAST_TIME );
		angle = cent->pe.barrelAngle + delta * speed;
	}

	if ( cent->pe.barrelSpinning == !(cent->currentState.eFlags & EF_FIRING) ) {
		cent->pe.barrelTime = cg.time;
		cent->pe.barrelAngle = AngleMod( angle );
		cent->pe.barrelSpinning = !!(cent->currentState.eFlags & EF_FIRING);
	}

	return angle;
}
*/

/*
==============
CG_TeslaSpinAngle
==============
*/
//#define TESLA_SPINSPEED .2
//#define TESLA_COASTTIME	2000
#define TESLA_SPINSPEED .05
#define TESLA_IDLESPEED .15
#define TESLA_COASTTIME 1000

static float CG_TeslaSpinAngle( centity_t *cent ) {
	int	delta;
	float	angle;
//	float	speed;

	delta = cg.time - cent->pe.barrelTime;

	angle = cent->pe.barrelAngle;

	if ( cent->currentState.eFlags & EF_FIRING ) {
		angle += delta * TESLA_SPINSPEED;
	} else {
		angle += delta * TESLA_IDLESPEED;
	}

	cent->pe.barrelAngle = AngleMod( angle );

	cent->pe.barrelTime = cg.time;

	return AngleMod( angle );

//----(SA)	trying new tesla effect scheme for MK
//	angle = -(cent->pe.barrelAngle + delta * TESLA_SPINSPEED);
//	cent->pe.barrelAngle = AngleMod( angle );

//	if(cent->currentState.eFlags & EF_FIRING)
//		cent->pe.barrelAngle += delta * TESLA_SPINSPEED;
//	else
//		cent->pe.barrelAngle += delta * TESLA_IDLESPEED;

#if 0
	return AngleMod( cent->pe.barrelAngle );

	return angle;


	if ( cent->pe.barrelSpinning ) {
		angle = -( cent->pe.barrelAngle + delta * TESLA_SPINSPEED );
	} else {
		if ( delta > TESLA_COASTTIME ) {
			delta = TESLA_COASTTIME;
		}

		speed = 0.5 * ( TESLA_SPINSPEED + (float)( TESLA_COASTTIME - delta ) / TESLA_COASTTIME );
		angle = -( cent->pe.barrelAngle + delta * speed );
	}

	if ( cent->pe.barrelSpinning == !( cent->currentState.eFlags & EF_FIRING ) ) {
		cent->pe.barrelTime = cg.time;
		cent->pe.barrelAngle = AngleMod( angle );
		cent->pe.barrelSpinning = !!( cent->currentState.eFlags & EF_FIRING );
	}

	return angle;
#endif
}

//----(SA)	added

/*
======================
CG_VenomSpinAngle
======================
*/

#define     VENOM_LOADTIME 2000
#define     VENOM_DELTATIME ( VENOM_LOADTIME / 10 )     // as there are 10 shots to be loaded

#define     SPIN_SPEED  1
#define     COAST_TIME  2000

static float CG_VenomSpinAngle( centity_t *cent ) {
	int delta;
	float angle;
	float speed;
	qboolean firing;

	firing = (qboolean)( cent->currentState.eFlags & EF_FIRING );

	// WEAPON_VENOM_REST: barrels pre-spun via simple zoom, holding at speed without firing
	if ( cg.snap->ps.weaponstate != WEAPON_FIRING && cg.snap->ps.weaponstate != WEAPON_VENOM_REST ) { // (SA) this seems better
		firing = qfalse;
	}


	delta = cg.time - cent->pe.barrelTime;
	if ( cent->pe.barrelSpinning ) {
		angle = cent->pe.barrelAngle + delta * SPIN_SPEED;
	} else {
		if ( delta > COAST_TIME ) {
			delta = COAST_TIME;
		}

		speed = 0.5 * ( SPIN_SPEED + (float)( COAST_TIME - delta ) / COAST_TIME );
		angle = cent->pe.barrelAngle + delta * speed;
	}

	if ( cent->pe.barrelSpinning == !firing ) {
		cent->pe.barrelTime = cg.time;
		cent->pe.barrelAngle = AngleMod( angle );
		cent->pe.barrelSpinning = !!firing;

		// just switching between not spinning and spinning, play the appropriate weapon sound
		if ( cent->pe.barrelSpinning ) {
			if ( cg_weapons[WP_VENOM].spinupSound ) {
				trap_S_StartSound( NULL, cent->currentState.number, CHAN_WEAPON, cg_weapons[WP_VENOM].spinupSound );
			}
		} else {
			if ( cg_weapons[WP_VENOM].spindownSound ) {
				trap_S_StartSound( NULL, cent->currentState.number, CHAN_WEAPON, cg_weapons[WP_VENOM].spindownSound );
			}
		}

	}

	return angle;
}




/*
==============
CG_DrawRealWeapons
==============
*/
qboolean CG_DrawRealWeapons( centity_t *cent ) {

	switch ( cent->currentState.aiChar ) {
	case AICHAR_LOPER:
	case AICHAR_LOPER_SPECIAL:
	case AICHAR_SUPERSOLDIER:       //----(SA)	added
	case AICHAR_PROTOSOLDIER:
	case AICHAR_ZOMBIE:
	case AICHAR_ZOMBIE_SURV:
	case AICHAR_ZOMBIE_FLAME:
	case AICHAR_ZOMBIE_GHOST:
	case AICHAR_HELGA:      //----(SA)	added	// boss1 is now helga-blob
	case AICHAR_WARZOMBIE:
	case AICHAR_PRIEST:     // ported from RealRTCW - fires tesla from a tag, no held model
	case AICHAR_XSHEPHERD:  // ported from RealRTCW - monster, no held weapon model
		return qfalse;
	}

	return qtrue;
}


/*
========================
CG_AddWeaponRefEntity

Entities belonging to the local player's first-person view weapon (ps != NULL) are
cached instead of added to the scene immediately, so they can be rendered in their
own pass at a fixed FOV (see CG_RenderViewWeapon) instead of the world's cg_fov.
Third-person weapon entities (ps == NULL) are added to the current scene as usual.
========================
*/
#define MAX_VIEWWEAPON_ENTITIES ( ( W_MAX_PARTS + 2 ) * 4 + 2 )
static refEntity_t cg_viewWeaponEnts[MAX_VIEWWEAPON_ENTITIES];
static int cg_numViewWeaponEnts;

static void CG_AddWeaponRefEntity( refEntity_t *ent, playerState_t *ps ) {
	if ( !ps ) {
		trap_R_AddRefEntityToScene( ent );
		return;
	}

	if ( cg_numViewWeaponEnts >= MAX_VIEWWEAPON_ENTITIES ) {
		return;
	}

	cg_viewWeaponEnts[cg_numViewWeaponEnts++] = *ent;
}

/*
========================
CG_AddWeaponWithPowerups
========================
*/
static void CG_AddWeaponWithPowerups( refEntity_t *gun, int powerups, playerState_t *ps, centity_t *cent ) {
	qhandle_t savedCustomShader;

		// IMPORTANT: this function is called for weapon sub-parts (barrels/bolts/hands/etc.)
	// in RTCW. Those refEntity_t structs are often re-used in a loop, so if we set
	// customShader and don't restore it, the next part will inherit it and "lose" its base skin.
	savedCustomShader = gun->customShader;

	// add powerup effects
	if ( powerups & ( 1 << PW_INVIS ) ) {

		gun->customShader = cgs.media.invisShader;
		CG_AddWeaponRefEntity( gun, ps );

		// restore and bail (invis usually replaces base draw)
		gun->customShader = savedCustomShader;

	} else if ( ( powerups & ( 1 << PW_VENOM ) ) &&
				( ( ps ? ps->powerups[PW_VENOM] - cg.time : 5000 ) < 5000 ) ) {

		// venom wearing off: flicker between the normal weapon and the invisibility shader
		if ( ( cg.time / 200 ) % 2 ) {
			gun->customShader = cgs.media.invisShader;
		} else {
			gun->customShader = 0;
		}
		CG_AddWeaponRefEntity( gun, ps );
		gun->customShader = savedCustomShader;

	} else {

		// always draw base weapon normally (no shader override)
		gun->customShader = 0;
		CG_AddWeaponRefEntity( gun, ps );

		// blink if time left < 5s, toggling every 200ms for battlesuit
		// NOTE: ps is NULL when drawing third-person weapons for other entities
		// (see the !ps branches above), so only consult ps->powerups when we
		// actually have a playerState_t to read the remaining time from.
		if ( powerups & ( 1 << PW_BATTLESUIT_SURV ) ) {
			int timeLeft = ps ? ps->powerups[PW_BATTLESUIT_SURV] - cg.time : 5000;
			if ( ( timeLeft < 5000 ) && ( ( cg.time / 200 ) % 2 ) ) {
				// skip rendering to blink
			} else {
				gun->customShader = cgs.media.battleWeaponShader;
				CG_AddWeaponRefEntity( gun, ps );
				gun->customShader = 0;
			}
		}

		// blink for quad powerup
		if ( powerups & ( 1 << PW_QUAD ) ) {
			int timeLeft = ps ? ps->powerups[PW_QUAD] - cg.time : 5000;
			if ( ( timeLeft < 5000 ) && ( ( cg.time / 200 ) % 2 ) ) {
				// skip rendering to blink
			} else {
				gun->customShader = cgs.media.quadWeaponShader;
				CG_AddWeaponRefEntity( gun, ps );
				gun->customShader = 0;
			}
		}

		// ported from RealRTCW - blink for xshield powerup (reuses the quad weapon shader)
		if ( powerups & ( 1 << PW_XSHIELD ) ) {
			int timeLeft = ps ? ps->powerups[PW_XSHIELD] - cg.time : 5000;
			if ( ( timeLeft < 5000 ) && ( ( cg.time / 200 ) % 2 ) ) {
				// skip rendering to blink
			} else {
				gun->customShader = cgs.media.quadWeaponShader;
				CG_AddWeaponRefEntity( gun, ps );
				gun->customShader = 0;
			}
		}

		// blink for vampire powerup
		if ( powerups & ( 1 << PW_VAMPIRE ) ) {
			int timeLeft = ps ? ps->powerups[PW_VAMPIRE] - cg.time : 5000;
			if ( ( timeLeft < 5000 ) && ( ( cg.time / 200 ) % 2 ) ) {
				// skip rendering to blink
			} else {
				gun->customShader = cgs.media.redQuadShader;
				CG_AddWeaponRefEntity( gun, ps );
				gun->customShader = 0;
			}
		}
}
}

/*
==============
CG_PlayerTeslaCoilFire

  TODO: this needs to be fixed for multiplay. entities being hurt need to be sent
  by server to all clients, so they draw the correct effects.
==============
*/
void CG_PlayerTeslaCoilFire( centity_t *cent, vec3_t flashorigin ) {

#define	TESLA_LIGHTNING_POINT_TIMEOUT	3000
#define	TESLA_LIGHTNING_MAX_DIST	( cent->currentState.aiChar == AICHAR_SUPERSOLDIER ? TESLA_SUPERSOLDIER_RANGE : TESLA_RANGE )     // use these to perhaps vary the distance according to aiming
#define	TESLA_LIGHTNING_NORMAL_DIST	( TESLA_RANGE / 2.0 )
#define	TESLA_MAX_POINT_TESTS			10
#define	TESLA_MAX_POINT_TESTS_PERFRAME	20

	int i, j, pointTests = 0;
	vec3_t testPos, tagPos, vec;
	trace_t tr;
	float maxDist;
	int numPoints;
	vec3_t viewAngles, viewDir;
	int visEnemies[16];
	float visDists[16];
	int visEnemiesSorted[MAX_TESLA_BOLTS];
	int numEnemies, numSorted = 0, best;
	float bestDist;
	centity_t *ctrav;
	vec3_t traceOrg;
	int playerTeam;

	if ( cent->currentState.weapon != WP_TESLA ) {
		return;
	}

	VectorCopy( cent->lerpAngles, viewAngles );

	AngleVectors( viewAngles, viewDir, NULL, NULL );

	if ( cent->currentState.number == cg.snap->ps.clientNum ) {
		VectorCopy( cg.snap->ps.origin, traceOrg );
		playerTeam = cg.snap->ps.teamNum;
	} else {
		VectorCopy( cent->lerpOrigin, traceOrg );
		playerTeam = cent->currentState.teamNum;
	}

	maxDist = TESLA_LIGHTNING_MAX_DIST;
	numPoints = MAX_TESLA_BOLTS;

	VectorCopy( flashorigin, tagPos );

	// first, build a list of visible enemies that can be hurt by this tesla, then filter by distance
	if ( !cent->pe.teslaDamageApplyTime || cent->pe.teslaDamageApplyTime < cg.time - 200 ) {
		numEnemies = 0;
		// check the local playing client
		VectorSubtract( cg.snap->ps.origin, traceOrg, vec );
		VectorNormalize( vec );
		if ( ( cent != &cg_entities[cg.snap->ps.clientNum] ) &&
			 ( cg.snap->ps.teamNum != playerTeam ) &&
			 ( Distance( tagPos, cg.snap->ps.origin ) < TESLA_LIGHTNING_MAX_DIST ) &&
			 ( DotProduct( viewDir, vec ) > 0.8 ) ) {
			CG_Trace( &tr, traceOrg, NULL, NULL, cg.snap->ps.origin, cg.snap->ps.clientNum, MASK_SHOT & ~( CONTENTS_BODY ) );
			if ( tr.fraction == 1 || tr.entityNum == cg.snap->ps.clientNum ) {
				visDists[numEnemies] = Distance( tagPos, cg.snap->ps.origin );
				visEnemies[numEnemies++] = cg.snap->ps.clientNum;
			} else {    // try head
				VectorCopy( cg.snap->ps.origin, vec );
				vec[2] += cg.snap->ps.viewheight;
				CG_Trace( &tr, tagPos, NULL, NULL, vec, cg.snap->ps.clientNum, MASK_SHOT & ~( CONTENTS_BODY ) );
				if ( tr.fraction == 1 || tr.entityNum == cg.snap->ps.clientNum ) {
					visDists[numEnemies] = Distance( tagPos, cg.snap->ps.origin );
					visEnemies[numEnemies++] = cg.snap->ps.clientNum;
				} else {    // try body, from tag
					VectorCopy( cg.snap->ps.origin, vec );
					CG_Trace( &tr, tagPos, NULL, NULL, vec, cg.snap->ps.clientNum, MASK_SHOT & ~( CONTENTS_BODY ) );
					if ( tr.fraction == 1 || tr.entityNum == cg.snap->ps.clientNum ) {
						visDists[numEnemies] = Distance( tagPos, cg.snap->ps.origin );
						visEnemies[numEnemies++] = cg.snap->ps.clientNum;
					}
				}
			}
		}

		// commenting out local server check. TODO: needs testing on dedicated server.
		if ( /*cgs.localServer &&*/ cgs.gametype <= GT_SINGLE_PLAYER ) {
			// check for AI's getting hurt (TODO: bot support?)
			for ( ctrav = cg_entities, i = 0; i < cgs.maxclients && numEnemies < 16; ctrav++, i++ ) {
				// RF, proto and supersoldier are invulnerable to tesla
/*				switch ( ctrav->currentState.aiChar ) {
				case AICHAR_SUPERSOLDIER:
				case AICHAR_PROTOSOLDIER:
					continue;
				}
*/                                                                                                                              //
				if ( ctrav->currentState.aiChar &&
					 ( ctrav != cent ) &&
					 ( ctrav->currentState.teamNum != playerTeam ) &&
					 !( ctrav->currentState.eFlags & EF_DEAD ) &&
					 ctrav->currentValid && // is in the visible frame
					 ( Distance( tagPos, ctrav->lerpOrigin ) < TESLA_LIGHTNING_MAX_DIST ) ) {
					VectorSubtract( ctrav->lerpOrigin, traceOrg, vec );
					VectorNormalize( vec );

					if ( DotProduct( viewDir, vec ) > 0.8 ) {
						CG_Trace( &tr, traceOrg, NULL, NULL, ctrav->lerpOrigin, ctrav->currentState.number, MASK_SHOT & ~CONTENTS_BODY );
						if ( tr.fraction == 1 || tr.entityNum == ctrav->currentState.number ) {
							visDists[numEnemies] = Distance( tagPos, ctrav->lerpOrigin );
							visEnemies[numEnemies++] = ctrav->currentState.number;
						}
					}
				}
			}
		}

		// now sort by distance
		for ( j = 0; j < MAX_TESLA_BOLTS; j++ ) {
			visEnemiesSorted[j] = -1;

			bestDist = 99999;
			best = -1;
			for ( i = 0; i < numEnemies; i++ ) {
				if ( visEnemies[i] < 0 ) {
					continue;
				}
				if ( visDists[i] < bestDist ) {
					bestDist = visDists[i];
					visEnemiesSorted[j] = visEnemies[i];
					best = i;
				}
			}

			if ( best >= 0 ) {
				visEnemies[best] = -1;
				numSorted = j + 1;
			}
		}

		// now fill in the teslaEnemy[]'s
		for ( i = 0; i < MAX_TESLA_BOLTS; i++ ) {
			if ( numSorted && i / numSorted < 1 /*( MAX_TESLA_BOLTS / 3 )*/ ) {  // bolts per enemy
				j = i % numSorted;
				cent->pe.teslaEnemy[i] = visEnemiesSorted[j];
				// apply damage
				CG_ClientDamage( visEnemiesSorted[j], cent->currentState.number, CLDMG_TESLA );
				// show the effect
				cg_entities[ visEnemiesSorted[j] ].pe.teslaDamagedTime = cg.time;
			} else {
				if ( cent->pe.teslaEnemy[i] >= 0 ) {
					cent->pe.teslaEndPointTimes[i] = 0; // make sure we find a new spot
				}
				cent->pe.teslaEnemy[i] = -1;
			}
		}
		cent->pe.teslaDamageApplyTime = cg.time;
	}

	for ( i = 0; i < numPoints; i++ ) {

		//if (!(rand()%3))
		//	continue;

		VectorSubtract( cent->pe.teslaEndPoints[i], tagPos, vec );
		VectorNormalize( vec );

		// if this point has timed out, find a new spot
		if ( cent->pe.teslaEnemy[i] >= 0 ) {
			// attacking the player
			VectorSet( testPos, 6 * crandom(),
					   6 * crandom(),
					   20 * crandom() - 8 );
			//VectorClear( testPos );
			if ( cent->pe.teslaEnemy[i] != cg.snap->ps.clientNum ) {
				VectorAdd( testPos, cg_entities[cent->pe.teslaEnemy[i]].lerpOrigin, testPos );
			} else {
				VectorAdd( testPos, cg.snap->ps.origin, testPos );
			}
			cent->pe.teslaEndPointTimes[i] = cg.time; // - rand()%(TESLA_LIGHTNING_POINT_TIMEOUT/2);
			VectorCopy( testPos, cent->pe.teslaEndPoints[i] );
		} else if ( ( !cent->pe.teslaEndPointTimes[i] ) ||
					( cent->pe.teslaEndPointTimes[i] > cg.time ) ||
					( cent->pe.teslaEndPointTimes[i] < cg.time - TESLA_LIGHTNING_POINT_TIMEOUT ) ||
					( VectorDistance( tagPos, cent->pe.teslaEndPoints[i] ) > maxDist ) ||
					( DotProduct( viewDir, vec ) < 0.7 ) ) {

			//if (cent->currentState.groundEntityNum == ENTITYNUM_NONE)
			//	continue;	// must be on the ground

			// find a new spot
			for ( j = 0; j < TESLA_MAX_POINT_TESTS; j++ ) {
				VectorSet( testPos, cg.refdef.fov_y * crandom() * 0.5,
						   cg.refdef.fov_x * crandom() * 0.5,
						   0 );
				VectorAdd( viewAngles, testPos, testPos );
				AngleVectors( testPos, vec, NULL, NULL );
				VectorMA( tagPos, TESLA_LIGHTNING_NORMAL_DIST, vec, testPos );
				// try a trace to find a world collision
				CG_Trace( &tr, tagPos, NULL, NULL, testPos, cent->currentState.number, MASK_SHOT & ~CONTENTS_BODY );
				if ( tr.fraction < 1 && tr.entityNum == ENTITYNUM_WORLD && !( tr.surfaceFlags & ( SURF_NOIMPACT | SURF_SKY ) ) ) {
					// found a valid spot!
					cent->pe.teslaEndPointTimes[i] = cg.time - rand() % ( TESLA_LIGHTNING_POINT_TIMEOUT / 2 );
					VectorCopy( tr.endpos, cent->pe.teslaEndPoints[i] );
					break;
				}
				if ( pointTests++ > TESLA_MAX_POINT_TESTS_PERFRAME ) {
					j = TESLA_MAX_POINT_TESTS;
					continue;
				}
			}
			if ( j == TESLA_MAX_POINT_TESTS ) {
				continue;   // just don't draw this point
			}

			// add an impact mark on the wall
			VectorSubtract( cent->pe.teslaEndPoints[i], tagPos, vec );
			VectorNormalize( vec );
			VectorInverse( vec );
			CG_ImpactMark( cgs.media.lightningHitWallShader, cent->pe.teslaEndPoints[i], vec, random() * 360, 0.2, 0.2, 0.2, 1.0, qtrue, 4, qfalse, 300 );
		}
		//
		// we have a valid lightning point, so draw it
		// sanity check though to make sure it's valid
		if ( VectorDistance( tagPos, cent->pe.teslaEndPoints[i] ) <= maxDist ) {
			CG_DynamicLightningBolt( cgs.media.lightningBoltShader, tagPos, cent->pe.teslaEndPoints[i], 1 + ( ( cg.time % ( ( i + 2 ) * ( i + 3 ) ) ) + i ) % 2, 20 + (float)( i % 3 ) * 5 + 6.0 * random(), ( cent->pe.teslaEnemy[i] < 0 ), 1.0, 0, i * i * 3 );

			// play a zap sound
			if ( cent->pe.lightningSoundTime < cg.time - 200 ) {
				CG_SoundPlayIndexedScript( cgs.media.teslaZapScript, cent->pe.teslaEndPoints[i], ENTITYNUM_WORLD );
				CG_SoundPlayIndexedScript( cgs.media.teslaZapScript, cent->lerpOrigin, ENTITYNUM_WORLD );
				//trap_S_StartSound( cent->pe.teslaEndPoints[i], ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.lightningSounds[rand()%3] );
				cent->pe.lightningSoundTime = cg.time + rand() % 200;
			}
		}
	}

	if ( cg.time % 3 ) {  // break it up a bit
		// add the looping sound
		CG_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, cgs.media.teslaLoopSound, 255 );
	}

	// drop a dynamic light out infront of us
	AngleVectors( viewAngles, vec, NULL, NULL );
	VectorMA( tagPos, 300, vec, testPos );
	// try a trace to find a world collision
	CG_Trace( &tr, tagPos, NULL, NULL, testPos, cent->currentState.number, MASK_SOLID );

	if ( ( cg.time / 50 ) % ( 4 + ( cg.time % 4 ) ) == 0 ) {
		// alt light
		trap_R_AddLightToScene( tr.endpos, 256 + 600 * tr.fraction, 0.2, 0.6, 1, 1 );
	} else if ( ( cg.time / 50 ) % ( 4 + ( cg.time % 4 ) ) == 1 ) {
		// no light
		//trap_R_AddLightToScene( tr.endpos, 128 + 500*tr.fraction, 1, 1, 1, 10 );
	} else {
		// blue light
		trap_R_AddLightToScene( tr.endpos, 256 + 600 * tr.fraction, 0.2, 0.6, 1, 0 );
	}

	// shake the camera a bit
	CG_StartShakeCamera( 0.05, 200, cent->lerpOrigin, 100 );
}

//----(SA)
/*
==============
CG_AddProtoWeapons
==============
*/
void CG_AddProtoWeapons( refEntity_t *parent, playerState_t *ps, centity_t *cent ) {
#if 0
	refEntity_t gun;

	if ( !( cent->currentState.aiChar == AICHAR_PROTOSOLDIER ) ) {
		return;
	}
	memset( &gun, 0, sizeof( gun ) );
	VectorCopy( parent->lightingOrigin, gun.lightingOrigin );
//	gun.hModel		= cgs.media.protoWeapon;
	gun.shadowPlane = parent->shadowPlane;
	gun.renderfx    = parent->renderfx;
	CG_PositionEntityOnTag( &gun, parent, "tag_armright", 0, NULL );
	CG_AddWeaponWithPowerups( &gun, cent->currentState.powerups, ps, cent );
#endif
}
//----(SA)	end



// Ridah
/*
==============
CG_MonsterUsingWeapon
==============
*/
qboolean CG_MonsterUsingWeapon( centity_t *cent, int aiChar, int weaponNum ) {
	return ( cent->currentState.aiChar == aiChar ) && ( cent->currentState.weapon == weaponNum );
}

/*
==============
CG_WeaponIsUpgraded
==============
*/
qboolean CG_WeaponIsUpgraded(weapon_t weaponNum) {
	if (cg.snap->ps.clientNum != cg.clientNum) {
		return qfalse;
	}
	return (cg.snap->ps.weaponUpgraded[weaponNum] != 0);
}


/*
=============
CG_AddPlayerWeapon

Used for both the view weapon (ps is valid) and the world modelother character models (ps is NULL)
The main player will have this called for BOTH cases, so effects like light and
sound should only be done on the world model case.
=============
*/
static qboolean debuggingweapon = qfalse;

void CG_AddPlayerWeapon( refEntity_t *parent, playerState_t *ps, centity_t *cent ) {

	refEntity_t gun;
	refEntity_t barrel;
	refEntity_t flash;
	vec3_t angles;
	weapon_t weaponNum, weapSelect;
	weaponInfo_t    *weapon;
	centity_t   *nonPredictedCent;
	qboolean firing;    // Ridah

	qboolean akimboFire = qfalse;       //----(SA)	added

	qboolean playerScaled;
	qboolean drawpart, drawrealweap;
	int i;
	qboolean isPlayer;

	// (SA) might as well have this check consistant throughout the routine
	isPlayer = (qboolean)( cent->currentState.clientNum == cg.snap->ps.clientNum );

	weaponNum = cent->currentState.weapon;
	weapSelect = cg.weaponSelect;

	if ( ps && cg.cameraMode ) {
		return;
	}

	// don't draw any weapons when the binocs are up
	if ( cent->currentState.eFlags & EF_ZOOMING ) {
		if ( isPlayer ) {
			if ( !cg.renderingThirdPerson ) {
				return;
			}
		} else {
			return;
		}
	}

	// don't draw weapon stuff when looking through a scope
	if ((GetWeaponTableData(weaponNum)->weaponClass & WEAPON_CLASS_SCOPED) ||
		(GetWeaponTableData(weapSelect)->weaponClass & WEAPON_CLASS_SCOPED))
	{
		if (isPlayer && !cg.renderingThirdPerson)
		{
			return;
		}
	}

	// no weapon when on mg_42
	if ( cent->currentState.eFlags & EF_MG42_ACTIVE ) {
		return;
	}

	CG_RegisterWeapon( weaponNum );
	weapon = &cg_weapons[weaponNum];

	if ( isPlayer ) {
		akimboFire = BG_AkimboFireSequence( weaponNum, cg.predictedPlayerState.ammoclip[weaponNum] );
	} else if ( ps ) {
		akimboFire = BG_AkimboFireSequence( weaponNum, ps->ammoclip[weaponNum] );
	}

	// add the weapon
	memset( &gun, 0, sizeof( gun ) );
	VectorCopy( parent->lightingOrigin, gun.lightingOrigin );
	gun.shadowPlane = parent->shadowPlane;
	gun.renderfx = parent->renderfx;

	// set custom shading for railgun refire rate
	if ( ps ) {
		gun.shaderRGBA[0] = 255;
		gun.shaderRGBA[1] = 255;
		gun.shaderRGBA[2] = 255;
		gun.shaderRGBA[3] = 255;
	}

	if ( ps ) {
		gun.hModel = weapon->weaponModel[W_FP_MODEL];
	} else {
		CG_AddProtoWeapons( parent, ps, cent );
		// skeletal guys use a different third person weapon (for different tag business)
		if ( cgs.clientinfo[ cent->currentState.clientNum ].isSkeletal && weapon->weaponModel[W_SKTP_MODEL] ) {
			gun.hModel = weapon->weaponModel[W_SKTP_MODEL];
		} else {
			gun.hModel = weapon->weaponModel[W_TP_MODEL];
		}
	}

	if ( !gun.hModel ) {
		if ( debuggingweapon ) {
			CG_Printf( "returning due to: !gun.hModel\n" );
		}
		return;
	}

	if ( !ps && cg.snap->ps.pm_flags & PMF_LADDER && isPlayer ) {      //----(SA) player on ladder
		if ( debuggingweapon ) {
			CG_Printf( "returning due to: !ps && cg.snap->ps.pm_flags & PMF_LADDER\n" );
		}
		return;
	}

	if ( !ps ) {
		// add weapon ready sound
		cent->pe.lightningFiring = qfalse;
		if ( ( cent->currentState.eFlags & EF_FIRING ) && weapon->firingSound ) {
			// lightning gun and guantlet make a different sound when fire is held down
			CG_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->firingSound, 255 );
			cent->pe.lightningFiring = qtrue;
		} else if ( weapon->readySound ) {
			CG_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->readySound, 255 );
		}
	}


	// Ridah
	firing = ( ( cent->currentState.eFlags & EF_FIRING ) != 0 );

	CG_PositionEntityOnTag( &gun, parent, "tag_weapon", 0, NULL );

	if ( !ps ) {    // third person only - some weapon models weren't authored for the carry pose tag_weapon puts them in
		vec3_t angleOffset;
		VectorCopy( GetWeaponTableData( weaponNum )->gunAngleOffset, angleOffset );
		angleOffset[PITCH] += cg_gunAnglePitch.value;
		angleOffset[YAW] += cg_gunAngleYaw.value;
		angleOffset[ROLL] += cg_gunAngleRoll.value;
		if ( angleOffset[PITCH] || angleOffset[YAW] || angleOffset[ROLL] ) {
			vec3_t correctionAxis[3], correctedAxis[3];
			AnglesToAxis( angleOffset, correctionAxis );
			MatrixMultiply( correctionAxis, gun.axis, correctedAxis );
			AxisCopy( correctedAxis, gun.axis );
		}
	}

	playerScaled = (qboolean)( cgs.clientinfo[ cent->currentState.clientNum ].playermodelScale[0] != 0 );
	if ( !ps && playerScaled ) {   // don't "un-scale" weap up in 1st person
		for ( i = 0; i < 3; i++ ) {  // scale weapon back up so it doesn't pick up the adjusted scale of the character models.
									 // this will affect any parts attached to the gun as well (barrel/bolt/flash/brass/etc.)
			VectorScale( gun.axis[i], 1.0 / ( cgs.clientinfo[ cent->currentState.clientNum ].playermodelScale[i] ), gun.axis[i] );
		}
	}

	// characters that draw their own special weapon model will not draw the standard ones
	if ( CG_DrawRealWeapons( cent ) ) {
		drawrealweap = qtrue;
	} else {
		drawrealweap = qfalse;
	}

	if ( ps ) {
		drawpart = CG_GetPartFramesFromWeap( cent, &gun, parent, W_MAX_PARTS, weapon );   // W_MAX_PARTS specifies this as the primary view model
	} else {
		drawpart = qtrue;
	}

	if ( drawpart && drawrealweap ) {
		CG_AddWeaponWithPowerups( &gun, cent->currentState.powerups, ps, cent );
	}

	if ( isPlayer ) {
		refEntity_t brass;

		// opposite tag in akimbo, since at this point the weapon
		// has fired and the fire seq has switched over
		if ( (GetWeaponTableData(weaponNum)->weaponClass & WEAPON_CLASS_AKIMBO) && akimboFire ) {
			CG_PositionRotatedEntityOnTag( &brass, &gun, "tag_brass2" );
		} else {
			CG_PositionRotatedEntityOnTag( &brass, &gun, "tag_brass" );
		}

		VectorCopy( brass.origin, ejectBrassCasingOrigin );
	}

	memset( &barrel, 0, sizeof( barrel ) );
	VectorCopy( parent->lightingOrigin, barrel.lightingOrigin );
	barrel.shadowPlane = parent->shadowPlane;
	barrel.renderfx = parent->renderfx;

	// add barrels
	// attach generic weapon parts to the first person weapon.
	// if a barrel should be attached for third person, add it in the (!ps) section below
	angles[YAW] = angles[PITCH] = 0;

	if ( ps ) {
		qboolean spunpart;

		for ( i = W_PART_1; i < W_MAX_PARTS; i++ ) {

			spunpart = qfalse;
			barrel.hModel = weapon->wpPartModels[W_FP_MODEL][i];

			// check for spinning
			if ( weaponNum == WP_VENOM ) {
				if ( i == W_PART_1 ) {
					angles[ROLL] = CG_VenomSpinAngle( cent );
					spunpart = qtrue;
				} else if ( i == W_PART_2 )    {
					angles[ROLL] = -CG_VenomSpinAngle( cent );
					spunpart = qtrue;
				}
				// 'blurry' barel when firing
				// (SA) not right now.  at the moment, just spin the belt when firing, no swapout
				else if ( i == W_PART_3 ) {
					if ( ( cent->pe.weap.animationNumber & ~ANIM_TOGGLEBIT ) == WEAP_ATTACK1 ) {
						barrel.hModel = weapon->wpPartModels[W_FP_MODEL][i];
						angles[ROLL] = -CG_VenomSpinAngle( cent );
						angles[ROLL] = -( angles[ROLL] / 8.0f );
					} else {
						angles[ROLL] = 0;
					}
					spunpart = qtrue;
				}
			} else if ( weaponNum == WP_TESLA ) {
				if ( i == W_PART_1 || i == W_PART_2 ) {
					angles[ROLL] = CG_TeslaSpinAngle( cent );
					spunpart = qtrue;
				}
			}

			if ( spunpart ) {
				AnglesToAxis( angles, barrel.axis );
			}
			// end spinning


			if ( barrel.hModel ) {
				if ( i == W_PART_1 ) {
					if ( spunpart ) {
						CG_PositionRotatedEntityOnTag( &barrel, parent, "tag_barrel" );
					} else { CG_PositionEntityOnTag( &barrel, parent, "tag_barrel", 0, NULL );}
				} else {
					if ( spunpart ) {
						CG_PositionRotatedEntityOnTag( &barrel, parent, va( "tag_barrel%d", i + 1 ) );
					} else { CG_PositionEntityOnTag( &barrel, parent, va( "tag_barrel%d", i + 1 ), 0, NULL );}
				}

				drawpart = CG_GetPartFramesFromWeap( cent, &barrel, parent, i, weapon );

				if ( drawpart && drawrealweap ) {
					CG_AddWeaponWithPowerups( &barrel, cent->currentState.powerups, ps, cent );
				}
			}
		}
	} else {    // weapons with barrels drawn in third person
		if ( drawrealweap ) {
			if ( weaponNum == WP_VENOM ) {

				angles[ROLL] = CG_VenomSpinAngle( cent );
				AnglesToAxis( angles, barrel.axis );

				barrel.hModel = weapon->wpPartModels[W_TP_MODEL][W_PART_1];
				CG_PositionRotatedEntityOnTag( &barrel, &gun, "tag_barrel" );
				CG_AddWeaponWithPowerups( &barrel, cent->currentState.powerups, ps, cent );
			}
		}
	}

	// add the scope model to the rifle if you've got it
	if ( isPlayer && !cg.renderingThirdPerson ) {      // (SA) for now just do it on the first person weapons
		if ( weaponNum == WP_MAUSER ) {
			if ( COM_BitCheck( cg.predictedPlayerState.weapons, WP_SNIPERRIFLE ) ) {
				barrel.hModel = weapon->modModel[0];
				if ( barrel.hModel ) {
					CG_PositionEntityOnTag(&barrel, parent, "tag_scope", 0, NULL);
					CG_AddWeaponWithPowerups( &barrel, cent->currentState.powerups, ps, cent );
				}
			}
		}
	}

	// add the scope model to the M1941 if you've got it
	if ( isPlayer && !cg.renderingThirdPerson ) {      // (SA) for now just do it on the first person weapons
		if ( weaponNum == WP_M1941 ) {
			if ( COM_BitCheck( cg.predictedPlayerState.weapons, WP_M1941SCOPE ) ) {
				barrel.hModel = weapon->modModel[0];
				if ( barrel.hModel ) {
					CG_PositionEntityOnTag(&barrel, parent, "tag_scope", 0, NULL);
					CG_AddWeaponWithPowerups( &barrel, cent->currentState.powerups, ps, cent );
				}
			}
		}
	}

		if ( isPlayer && !cg.renderingThirdPerson ) {        // (SA) for now just do it on the first person weapons
		if ( weaponNum == WP_M1GARAND || weaponNum == WP_M7 ) {
			if ( (  cg.snap->ps.ammo[BG_FindAmmoForWeapon( WP_M7 )] || cg.snap->ps.ammoclip[BG_FindAmmoForWeapon( WP_M7 )]  ) ) {
					barrel.hModel = weapon->modModel[0];
					if ( barrel.hModel ) {
						CG_PositionEntityOnTag( &barrel, parent, "tag_scope", 0, NULL );
						CG_AddWeaponWithPowerups( &barrel, cent->currentState.powerups, ps, cent );
					}
			}
		}
		}


	// make sure we aren't looking at cg.predictedPlayerEntity for LG
	nonPredictedCent = &cg_entities[cent->currentState.clientNum];

	// if the index of the nonPredictedCent is not the same as the clientNum
	// then this is a fake player (like on the single player podiums), so
	// go ahead and use the cent
	if ( ( nonPredictedCent - cg_entities ) != cent->currentState.clientNum ) {
		nonPredictedCent = cent;
	}


	// add the flash
	memset( &flash, 0, sizeof( flash ) );
	VectorCopy( parent->lightingOrigin, flash.lightingOrigin );
	flash.shadowPlane = parent->shadowPlane;
	flash.renderfx = parent->renderfx;

	if ( ps ) {
		flash.hModel = weapon->flashModel[W_FP_MODEL];
	} else {
		flash.hModel = weapon->flashModel[W_TP_MODEL];
	}

	angles[YAW]     = 0;
	angles[PITCH]   = 0;
	angles[ROLL]    = crandom() * 10;
	AnglesToAxis( angles, flash.axis );

	if ( GetWeaponTableData(weaponNum)->weaponClass & WEAPON_CLASS_AKIMBO )
	{
		if (!ps || cg.renderingThirdPerson)
		{
			CG_PositionRotatedEntityOnTag(&flash, &gun, "tag_flash");
		}
		else
		{
			if (!cent->akimboFire)
			{
				CG_PositionRotatedEntityOnTag(&flash, parent, "tag_flash2");

			}
			else
			{
				CG_PositionRotatedEntityOnTag(&flash, parent, "tag_flash");
			}
		}
	}
	else
	{
		CG_PositionRotatedEntityOnTag(&flash, &gun, "tag_flash");
	}

	// store this position for other cgame elements to access
	cent->pe.gunRefEnt = gun;
	cent->pe.gunRefEntFrame = cg.clientFrame;

	if ( ( weaponNum == WP_FLAMETHROWER || weaponNum == WP_TESLA ) && ( nonPredictedCent->currentState.eFlags & EF_FIRING ) ) {
		// continuous flash

	} else {

		// continuous smoke after firing
#define BARREL_SMOKE_TIME 1000

		if (ps || cg.renderingThirdPerson || !isPlayer)
		{
			const ammotable_t *wt = GetWeaponTableData(weaponNum);

			if (!cg_paused.integer)
			{
				// hot smoking gun after overheating
				if (wt->maxHeat > 0 && cg.time - cent->overheatTime < 3000)
				{
					if (!(rand() % 3))
					{
						float alpha;
						int endSize;

						alpha = 1.0f - ((float)(cg.time - cent->overheatTime) / 3000.0f);
						alpha *= 0.25f;

						// Venom gets swirlier smoke because barrel spins fast
						if (weaponNum == WP_VENOM)
						{
							endSize = 70;
						}
						else
						{
							endSize = 30;
						}

						CG_ParticleImpactSmokePuffExtended(
							cgs.media.smokeParticleShader,
							flash.origin,
							tv(0, 0, 1),
							8,
							1000,
							8,
							20,
							endSize,
							alpha);
					}
				}
				// barrel smoke after firing, currently only panzer-style
				else if (weaponNum == WP_PANZERFAUST && cg.time - cent->muzzleFlashTime < BARREL_SMOKE_TIME)
				{
					if (!(rand() % 5))
					{
						float alpha;

						alpha = 1.0f - ((float)(cg.time - cent->muzzleFlashTime) / (float)BARREL_SMOKE_TIME);
						alpha *= 0.25f;

						CG_ParticleImpactSmokePuffExtended(
							cgs.media.smokeParticleShader,
							flash.origin,
							tv(0, 0, 1),
							8,
							1000,
							8,
							20,
							30,
							alpha);
					}
				}
			}
		}
		// impulse flash
		if ( cg.time - cent->muzzleFlashTime > MUZZLE_FLASH_TIME ) {
			// Ridah, blue ignition flame if not firing flamer
			if ( weaponNum != WP_FLAMETHROWER && weaponNum != WP_TESLA ) {
				return;
			}
		}

	}

	// weapons that don't need to go any further as they have no flash or light
	if (!GetWeaponTableData(weaponNum)->hasMuzzle)
	{
		flash.hModel = 0;
	}

	// weaps with barrel smoke
	if ( ps || cg.renderingThirdPerson || !isPlayer ) {
		if ( !cg_paused.integer ) {    // don't add while paused
			if ( weaponNum == WP_STEN || weaponNum == WP_VENOM || weaponNum == WP_MG42M || weaponNum == WP_BROWNING ) {
				if ( cg.time - cent->muzzleFlashTime < 100 ) {
					CG_ParticleImpactSmokePuffExtended( cgs.media.smokeParticleShader, flash.origin, tv( 0,0,1 ), 8, 500, 8, 20, 30, 0.25f );
				}
			}
		}
	}

	if ( flash.hModel ) {
		if ( weaponNum != WP_FLAMETHROWER && weaponNum != WP_TESLA ) {    //Ridah, hide the flash also for now
			// RF, changed this so the muzzle flash stays onscreen for long enough to be seen
			if ( cg.time - cent->muzzleFlashTime < MUZZLE_FLASH_TIME ) {
//				if ( firing )	// Ridah
					CG_AddWeaponRefEntity( &flash, ps );
			}
		}
	}

	// Ridah, zombie fires from his head
	//if (CG_MonsterUsingWeapon( cent, AICHAR_ZOMBIE, WP_MONSTER_ATTACK1 )) {
	//	CG_PositionEntityOnTag( &flash, parent, parent->hModel, "tag_head", NULL);
	//}

	// ported from RealRTCW - monsters emit their weapon FX from the model's tag_weapon
	if ( CG_MonsterUsingWeapon( cent, AICHAR_PRIEST, WP_TESLA ) ) {
		CG_PositionEntityOnTag( &flash, parent, "tag_weapon", 0, NULL );
	}
	if ( CG_MonsterUsingWeapon( cent, AICHAR_XSHEPHERD, WP_VENOM ) ) {
		CG_PositionEntityOnTag( &flash, parent, "tag_weapon", 0, NULL );
	}

	if ( ps || cg.renderingThirdPerson || !isPlayer ) {

		if ( firing ) {
			// Ridah, Flamethrower effect
			CG_FlamethrowerFlame( cent, flash.origin );

			// RF, Tesla coil
			CG_PlayerTeslaCoilFire( cent, flash.origin );

			// make a dlight for the flash
			if ( weapon->flashDlightColor[0] || weapon->flashDlightColor[1] || weapon->flashDlightColor[2] ) {
				trap_R_AddLightToScene( flash.origin, 200 + ( rand() & 31 ), weapon->flashDlightColor[0],
										weapon->flashDlightColor[1], weapon->flashDlightColor[2], REF_MUZZLEFLASH_DLIGHT );
			}
		} else {
			if ( weaponNum == WP_FLAMETHROWER ) {
				vec3_t angles;
				AxisToAngles( flash.axis, angles );
				CG_FireFlameChunks( cent, flash.origin, angles, 1.0, qfalse, 0 );
			}
		}
	}
}

void CG_AddPlayerFoot( refEntity_t *parent, playerState_t *ps, centity_t *cent ) {
	refEntity_t wolfkick;
	vec3_t kickangle;
	int frame;

	if ( !( cg.snap->ps.persistant[PERS_WOLFKICK] ) ) {
		return;
	}

	memset( &wolfkick, 0, sizeof( wolfkick ) );

	VectorCopy( parent->lightingOrigin, wolfkick.lightingOrigin );
	wolfkick.shadowPlane = parent->shadowPlane;

	// note to self we want this to lerp and advance frame
	wolfkick.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON;;
	wolfkick.hModel = wolfkickModel;

	VectorCopy( cg.refdef.vieworg, wolfkick.origin );
	//----(SA)	allow offsets for testing boot model
	if ( cg_gun_x.value ) {
		VectorMA( wolfkick.origin, cg_gun_x.value,  cg.refdef.viewaxis[0], wolfkick.origin );
	}
	if ( cg_gun_y.value ) {
		VectorMA( wolfkick.origin, cg_gun_y.value,  cg.refdef.viewaxis[1], wolfkick.origin );
	}
	if ( cg_gun_z.value ) {
		VectorMA( wolfkick.origin, cg_gun_z.value,  cg.refdef.viewaxis[2], wolfkick.origin );
	}
	//----(SA)	end


	VectorCopy( cg.refdefViewAngles, kickangle );
	if ( kickangle[0] < 0 ) {
		kickangle[0] = 0;                       //----(SA)	avoid "Rockette" syndrome :)
	}
	AnglesToAxis( kickangle, wolfkick.axis );


	frame = cg.snap->ps.persistant[PERS_WOLFKICK];

	wolfkick.frame = frame;
	wolfkick.oldframe = frame - 1;
	wolfkick.backlerp = 1 - cg.frameInterpolation;
	CG_AddWeaponRefEntity( &wolfkick, ps );

}

/*
==============
CG_AddViewWeapon

Add the weapon, and flash for the player's view
==============
*/
void CG_AddViewWeapon( playerState_t *ps ) {
	refEntity_t hand;
	vec3_t angles;
	vec3_t gunoff;
	weaponInfo_t    *weapon;

	cg_numViewWeaponEnts = 0;

	if ( ps->persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		return;
	}

	if ( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_FREEZE ) {
		return;
	}

	// no gun if in third person view
	if ( cg.renderingThirdPerson ) {
		return;
	}

	// allow the gun to be completely removed
	if ( !cg_drawGun.integer ) {
		// Flamer and tesla fix
		vec3_t origin;

		if ( cg.predictedPlayerState.eFlags & EF_FIRING ) {

			// special hack for flamethrower...
			VectorCopy( cg.refdef.vieworg, origin );

			VectorMA( origin, 18, cg.refdef.viewaxis[0], origin );
			VectorMA( origin, -7, cg.refdef.viewaxis[1], origin );
			VectorMA( origin, -4, cg.refdef.viewaxis[2], origin );

			// Weapons will sort them self out...
			// Flamer
			CG_FlamethrowerFlame( &cg.predictedPlayerEntity, origin );
			// Tesla
			CG_PlayerTeslaCoilFire( &cg.predictedPlayerEntity, origin );
		}
		return;
	}

	// don't draw if testing a gun model
	if ( cg.testGun ) {
		return;
	}

	if ( ps->eFlags & EF_MG42_ACTIVE ) {
		return;
	}

	memset( &hand, 0, sizeof( hand ) );
	const ammotable_t *wt;

	if ( ps->weapon > WP_NONE ) {
		CG_RegisterWeapon( ps->weapon );
		weapon = &cg_weapons[ ps->weapon ];
		wt = GetWeaponTableData( ps->weapon );

		// set up gun position
		CG_CalculateWeaponPosition( hand.origin, angles );

		gunoff[0] = wt->gunOffset[0] + cg_gun_x.value;
		gunoff[1] = wt->gunOffset[1] + cg_gun_y.value;
		gunoff[2] = wt->gunOffset[2] + cg_gun_z.value;

		// pull carry offset toward center while moving - fixed unit nudge, not a % of gunOffset[1]
		{
			float centerPull = ( cg.xyspeed / 320.0f ) * cg_gunSwayCenter.value;
			if ( centerPull > cg_gunSwayCenter.value ) {
				centerPull = cg_gunSwayCenter.value;
			}
			if ( gunoff[1] > 0.0f ) {
				gunoff[1] -= centerPull;
				if ( gunoff[1] < 0.0f ) {
					gunoff[1] = 0.0f;
				}
			} else if ( gunoff[1] < 0.0f ) {
				gunoff[1] += centerPull;
				if ( gunoff[1] > 0.0f ) {
					gunoff[1] = 0.0f;
				}
			}
		}

		VectorMA( hand.origin, gunoff[0], cg.refdef.viewaxis[0], hand.origin );
		VectorMA( hand.origin, gunoff[1], cg.refdef.viewaxis[1], hand.origin );
		VectorMA( hand.origin, gunoff[2], cg.refdef.viewaxis[2], hand.origin );

		AnglesToAxis( angles, hand.axis );

		if ( cg_gun_frame.integer ) {
			hand.frame = hand.oldframe = cg_gun_frame.integer;
			hand.backlerp = 0;
		} else {  // get the animation state
			CG_WeaponAnimation( ps, weapon, &hand.oldframe, &hand.frame, &hand.backlerp );   //----(SA)	changed
		}

		VectorCopy( hand.origin, hand.lightingOrigin );

		hand.hModel = weapon->handsModel;
		hand.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON | RF_MINLIGHT;   //----(SA)

		// add everything onto the hand
		CG_AddPlayerWeapon( &hand, ps, &cg.predictedPlayerEntity );
		// Ridah

	}   // end  "if ( ps->weapon > WP_NONE)"

	// Rafael
	// add the foot
	CG_AddPlayerFoot( &hand, ps, &cg.predictedPlayerEntity );

	cg.predictedPlayerEntity.lastWeaponClientFrame = cg.clientFrame;
}

/*
==============
CG_RenderViewWeapon

Renders the entities cached by CG_AddViewWeapon (the local player's hands/weapon/
wolfkick) in their own scene pass at a fixed FOV (cg_gunFov), so that cg_fov and
any other change to the world's FOV can't stretch or resize the view weapon model.
Must be called after the main world scene has been rendered.
==============
*/
void CG_RenderViewWeapon( void ) {
	refdef_t weapdef;
	float x, fov;
	int i;

	if ( !cg_numViewWeaponEnts ) {
		return;
	}

	weapdef = cg.refdef;
	weapdef.rdflags |= RDF_NOWORLDMODEL;

	fov = cg_gunFov.value;
	if ( fov < 1 ) {
		fov = 1;
	} else if ( fov > 160 ) {
		fov = 160;
	}

	if ( cg_fixedAspect.integer ) {
		// match the aspect correction applied to the world's fov (see CG_CalcFov)
		const float baseAspect = 0.75f; // 3/4
		const float aspect = (float)weapdef.width / (float)weapdef.height;

		fov = atan2( tan( fov * M_PI / 360.0f ) * baseAspect * aspect, 1 ) * 360.0f / M_PI;
	}

	x = weapdef.width / tan( fov / 360 * M_PI );
	weapdef.fov_x = fov;
	weapdef.fov_y = atan2( weapdef.height, x ) * 360 / M_PI;

	trap_R_ClearScene();

	for ( i = 0; i < cg_numViewWeaponEnts; i++ ) {
		trap_R_AddRefEntityToScene( &cg_viewWeaponEnts[i] );
	}
	cg_numViewWeaponEnts = 0;

	trap_R_RenderScene( &weapdef );
}

/*
==============================================================================

WEAPON SELECTION

==============================================================================
*/

#define WP_ICON_X       38  // new sizes per MK
#define WP_ICON_X_WIDE  72  // new sizes per MK
#define WP_ICON_Y       38
#define WP_ICON_SPACE_Y 10
#define WP_DRAW_X       640 - WP_ICON_X - 4 // 4 is 'selected' border width
#define WP_DRAW_X_WIDE  640 - WP_ICON_X_WIDE - 4
#define WP_DRAW_Y       4

// secondary fire icons
#define WP_ICON_SEC_X   18  // new sizes per MK
#define WP_ICON_SEC_Y   18


/*
===================
CG_DrawWeaponSelect
===================
*/
void CG_DrawWeaponSelect( void ) {
	int i;
	int x, y;
	int curweap, curweapbank = 0, curweapcycle = 0, drawweap;
	int realweap;               // DHM - Nerve
	int bits[MAX_WEAPONS / ( sizeof( int ) * 8 )];
	float       *color;

	// don't display if dead
	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 ) {
		return;
	}

	if ( !cg.weaponSelect ) {
		return;
	}

	color = CG_FadeColor( cg.weaponSelectTime, WEAPON_SELECT_TIME );
	if ( !color ) {
		return;
	}
	trap_R_SetColor( color );

	if ( cg_fixedAspect.integer == 2 ) {
		CG_SetScreenPlacement(PLACE_RIGHT, PLACE_TOP);
	}

//----(SA)	neither of these overlap the weapon selection area anymore, so let them stay
	// showing weapon select clears pickup item display, but not the blend blob
//	cg.itemPickupTime = 0;

	// also clear holdable list
//	cg.holdableSelectTime = 0;
//----(SA)	end

	// count the number of weapons owned
	memcpy( bits, cg.snap->ps.weapons, sizeof( bits ) );

	curweap = cg.weaponSelect;

	// get bank/cycle of current weapon
	if ( !CG_WeaponIndex( curweap, &curweapbank, &curweapcycle ) ) {

		// weapon selected isn't a primary weapon, so draw the alternates bank
		CG_WeaponIndex( getAltWeapon( curweap ), &curweapbank, &curweapcycle );

	}

	y = WP_DRAW_Y;

	for ( i = 0; i < maxWeapsInBank; i++ ) {

		qboolean wideweap;
		int iconDrawSize;

		// primary fire
		drawweap = weapBanks[curweapbank][i];
		realweap = drawweap;     

		iconDrawSize = GetWeaponTableData( drawweap )->iconDrawSize;

		if (iconDrawSize == WEAPON_ICON_HIDE)
		{
			continue;
		}

		wideweap = (iconDrawSize == WEAPON_ICON_WIDE || iconDrawSize == WEAPON_ICON_WIDE_OFFSET );

		if ( wideweap ) {
			x = WP_DRAW_X_WIDE;
		} else {
			x = WP_DRAW_X;
		}

		if(drawweap && COM_BitCheck( bits, drawweap )) {

			CG_RegisterWeapon( drawweap );

			if ( wideweap ) {
				// weapon icon
				if ( realweap == curweap ) {
					CG_DrawPic( x, y, WP_ICON_X_WIDE, WP_ICON_Y, cg_weapons[drawweap].weaponIcon[1] );
				} else {
					CG_DrawPic( x, y, WP_ICON_X_WIDE, WP_ICON_Y, cg_weapons[drawweap].weaponIcon[0] );
				}

				// no ammo cross
				if ( !CG_WeaponHasAmmo( realweap ) ) {   // DHM - Nerve
					CG_DrawPic( x, y, WP_ICON_X_WIDE, WP_ICON_Y, cgs.media.noammoShader );
				}
			} else {
				// weapon icon
				if ( realweap == curweap ) {
					CG_DrawPic( x, y, WP_ICON_X, WP_ICON_Y, cg_weapons[drawweap].weaponIcon[1] );
				} else {
					CG_DrawPic( x, y, WP_ICON_X, WP_ICON_Y, cg_weapons[drawweap].weaponIcon[0] );
				}

				// no ammo cross
				if ( !CG_WeaponHasAmmo( realweap ) ) {   // DHM - Nerve
					CG_DrawPic( x, y, WP_ICON_X, WP_ICON_Y, cgs.media.noammoShader );
				}
			}

		} else {
			continue;
		}

		// secondary fire
		if ( wideweap ) {
			x = WP_DRAW_X_WIDE - WP_ICON_SEC_X - 4;
		} else {
			x = WP_DRAW_X - WP_ICON_SEC_X - 4;
		}

		drawweap = getAltWeapon( weapBanks[curweapbank][i] );

		// clear drawweap if getaltweap() returns the same weap as passed in. (no secondary available)
		if ( drawweap == weapBanks[curweapbank][i] ) {
			drawweap = 0;
		}

		realweap = drawweap;        // DHM - Nerve

		if(drawweap && COM_BitCheck( bits, drawweap )) {

			CG_RegisterWeapon( drawweap );

			// weapon icon
			if ( realweap == cg.weaponSelect ) {
				CG_DrawPic( x, y, WP_ICON_SEC_X, WP_ICON_SEC_Y, cg_weapons[drawweap].weaponIcon[1] );
			} else {
				CG_DrawPic( x, y, WP_ICON_SEC_X, WP_ICON_SEC_Y, cg_weapons[drawweap].weaponIcon[0] );
			}

			// no ammo cross
			if ( !CG_WeaponHasAmmo( realweap ) ) {
				CG_DrawPic( x, y, WP_ICON_SEC_X, WP_ICON_SEC_Y, cgs.media.noammoShader );
			}
		}


		y += ( WP_ICON_Y + WP_ICON_SPACE_Y );
	}
}




/*
==============
CG_WeaponHasAmmo
	check for ammo
==============
*/
static qboolean CG_WeaponHasAmmo( int i ) {
	if ( !( cg.predictedPlayerState.ammo[BG_FindAmmoForWeapon( i )] ) &&
		 !( cg.predictedPlayerState.ammoclip[BG_FindClipForWeapon( i )] ) ) {
		return qfalse;
	}

	return qtrue;
}


/*
===============
CG_WeaponSelectable
===============
*/
static qboolean CG_WeaponSelectable( int i ) {

	// grenades are only thrown via quickgrenade now - never selectable as the active weapon
	if ( i == WP_GRENADE_LAUNCHER || i == WP_GRENADE_PINEAPPLE ) {
		return qfalse;
	}

	// allow the player to unselect all weapons
//	if(i == WP_NONE)
//		return qtrue;

	// if holding a melee weapon (chair/shield/etc.) only allow single-handed weapons
	if ( cg.snap->ps.eFlags & EF_MELEE_ACTIVE ) {
		if ( !( WEAPS_ONE_HANDED & ( 1LL << i ) ) ) {
			return qfalse;
		}
	}

	// allow switch out of scope for weapons where you fired the last shot while scoped
	// and we left you in that view to see the result of the shot
	switch ( cg.weaponSelect ) {
	case WP_SNOOPERSCOPE:
		if ( i == WP_SNOOPER ) {
			return qtrue;
		}
		break;
	case WP_SNIPERRIFLE:
		if ( i == WP_MAUSER ) {
			return qtrue;
		}
		break;
	case WP_FG42SCOPE:
		if ( i == WP_FG42 ) {
			return qtrue;
		}
		break;
	default:
		break;
	}


	// check for weapon
	if ( !( COM_BitCheck( cg.predictedPlayerState.weapons, i ) ) ) {
		return qfalse;
	}

	if (!CG_WeaponHasAmmo(i) && (cg_gameType.integer != GT_COOP_SURVIVAL))
	{
		return qfalse;
	}

	return qtrue;
}




/*
==============
CG_WeaponIndex
==============
*/
int CG_WeaponIndex( int weapnum, int *bank, int *cycle ) {
	static int bnk, cyc;

	if ( weapnum <= 0 || weapnum >= WP_NUM_WEAPONS ) {
		if ( bank ) {
			*bank = 0;
		}
		if ( cycle ) {
			*cycle = 0;
		}
		return 0;
	}

	for ( bnk = 0; bnk < maxWeapBanks; bnk++ ) {
		for ( cyc = 0; cyc < maxWeapsInBank; cyc++ ) {

			// end of cycle, go to next bank
			if ( cg_gameType.integer <= GT_SINGLE_PLAYER ) { // JPW NERVE
				if ( !weapBanks[bnk][cyc] ) {
					break;
				}

				// found the current weapon
				if ( weapBanks[bnk][cyc] == weapnum ) {
					if ( bank ) {
						*bank = bnk;
					}
					if ( cycle ) {
						*cycle = cyc;
					}
					return 1;
				}
			}
		}
	}

	// failed to find the weapon in the table
	// probably an alternate

	return 0;
}



/*
==============
getNextWeapInBank
	Pass in a bank and cycle and this will return the next valid weapon higher in the cycle.
	if the weap passed in is above highest in a cycle (maxWeapsInBank), this will safely loop around
==============
*/
static int getNextWeapInBank( int bank, int cycle ) {

	cycle++;

	cycle = cycle % maxWeapsInBank;

	if ( weapBanks[bank][cycle] ) {        // return next weapon in bank if there is one
		return weapBanks[bank][cycle];
	} else {                                // return first in bank
		return weapBanks[bank][0];
	}
}

static int getNextWeapInBankBynum( int weapnum ) {
	int bank, cycle;

	if ( !CG_WeaponIndex( weapnum, &bank, &cycle ) ) {
		return weapnum;
	}

	return getNextWeapInBank( bank, cycle );
}


/*
==============
getPrevWeapInBank
	Pass in a bank and cycle and this will return the next valid weapon lower in the cycle.
	if the weap passed in is the lowest in a cycle (0), this will loop around to the
	top (maxWeapsInBank-1) and start down from there looking for a valid weapon position
==============
*/
static int getPrevWeapInBank( int bank, int cycle ) {
	cycle--;
	if ( cycle < 0 ) {
		cycle = maxWeapsInBank - 1;
	}

	while ( !weapBanks[bank][cycle] )
	{
		cycle--;

		if ( cycle < 0 ) {
			cycle = maxWeapsInBank - 1;
		}
	}
	return weapBanks[bank][cycle];
}


static int getPrevWeapInBankBynum( int weapnum ) {
	int bank, cycle;

	if ( !CG_WeaponIndex( weapnum, &bank, &cycle ) ) {
		return weapnum;
	}

	return getPrevWeapInBank( bank, cycle );
}



/*
==============
getNextBankWeap
	Pass in a bank and cycle and this will return the next valid weapon in a higher bank.
	sameBankPosition: if there's a weapon in the next bank at the same cycle,
	return that	(colt returns thompson for example) rather than the lowest weapon
==============
*/
static int getNextBankWeap( int bank, int cycle, qboolean sameBankPosition ) {
	bank++;

	bank = bank % maxWeapBanks;

	if ( sameBankPosition && weapBanks[bank][cycle] ) {
		return weapBanks[bank][cycle];
	} else {
		return weapBanks[bank][0];
	}
}

/*
==============
getPrevBankWeap
	Pass in a bank and cycle and this will return the next valid weapon in a lower bank.
	sameBankPosition: if there's a weapon in the prev bank at the same cycle,
	return that	(thompson returns colt for example) rather than the highest weapon
==============
*/
static int getPrevBankWeap( int bank, int cycle, qboolean sameBankPosition ) {
	int i;

	bank--;

	if ( bank < 0 ) {    // don't go below 0, cycle up to top
		bank += maxWeapBanks;
	}
	bank = bank % maxWeapBanks;

	if ( sameBankPosition && weapBanks[bank][cycle] ) {
		return weapBanks[bank][cycle];
	} else
	{       // find highest weap in bank
		for ( i = maxWeapsInBank - 1; i >= 0; i-- ) {
			if ( weapBanks[bank][i] ) {
				return weapBanks[bank][i];
			}

		}
		// if it gets to here, no valid weaps in this bank, go down another bank
		return getPrevBankWeap( bank, cycle, sameBankPosition );
	}
}

/*
==============
getAltWeapon
==============
*/
static int getAltWeapon( int weapnum ) {
	
	if ( ammoTable[weapnum].weapAlts ) {
		return ammoTable[weapnum].weapAlts;
	}

	return weapnum;
}

/*
==============
getEquivWeapon
	return the id of the opposite team's weapon.
	Passing the weapnum of the mp40 returns the id of the thompson, and likewise
	passing the weapnum of the thompson returns the id of the mp40.
	No equivalent available will return the weapnum passed in.
==============
*/
int getEquivWeapon( int weapnum ) {
	int num = weapnum;

	switch ( weapnum ) {
		// going from german to american
	case WP_LUGER:              num = WP_COLT;              break;
	case WP_MAUSER:             num = WP_SNOOPER;            break;
	case WP_MP40:               num = WP_THOMPSON;          break;
	case WP_GRENADE_LAUNCHER:   num = WP_GRENADE_PINEAPPLE; break;

		// going from american to german
	case WP_COLT:               num = WP_LUGER;             break;
	case WP_SNOOPER:             num = WP_MAUSER;            break;
	case WP_THOMPSON:           num = WP_MP40;              break;
	case WP_GRENADE_PINEAPPLE:  num = WP_GRENADE_LAUNCHER;  break;
	}
	return num;
}



/*
==============
CG_WeaponSuggest
==============
*/
void CG_WeaponSuggest( int weap ) {
#if 0 // not currently supported
	int bank, cycle;

	if ( !cg_useSuggestedWeapons.integer ) {
		return;
	}

	cg.weaponSelectTime = cg.time;

	CG_WeaponIndex( weap, &bank, &cycle );    // get location of this weap

	cg.lastWeapSelInBank[bank] = weap;      // make this weap first priority in that bank
#endif
}


/*
==============
CG_SetSniperZoom
==============
*/

void CG_SetSniperZoom( int lastweap, int newweap ) {
	int zoomindex;

	if ( lastweap == newweap ) {
		return;
	}

	cg.zoomval      = 0;
	cg.zoomedScope  = 0;

	// check for fade-outs
	switch ( lastweap ) {
	case WP_SNIPERRIFLE:
//			cg.zoomedScope	= 500;	// TODO: add to zoomTable
//			cg.zoomTime		= cg.time;
		break;
	case WP_SNOOPERSCOPE:
//			cg.zoomedScope	= 500;	// TODO: add to zoomTable
//			cg.zoomTime		= cg.time;
		break;
	case WP_FG42SCOPE:
//			cg.zoomedScope	= 1;	// TODO: add to zoomTable
//			cg.zoomTime		= cg.time;
		break;
	case WP_M1941SCOPE:
		break;
	case WP_DELISLESCOPE:
		break;
	}

	switch ( newweap ) {

	default:
		return;     // no sniper zoom, get out.

	case WP_SNIPERRIFLE:
		cg.zoomval = cg_zoomDefaultSniper.value;
		cg.zoomedScope  = 900;      // TODO: add to zoomTable
		zoomindex = ZOOM_SNIPER;
		break;
	case WP_SNOOPERSCOPE:
		cg.zoomval = cg_zoomDefaultSnooper.value;
		cg.zoomedScope  = 800;      // TODO: add to zoomTable
		zoomindex = ZOOM_SNOOPER;
		break;
	case WP_FG42SCOPE:
		cg.zoomval = cg_zoomDefaultFG.value;
		cg.zoomedScope  = 1;        // TODO: add to zoomTable
		zoomindex = ZOOM_FG42SCOPE;
		break;
	case WP_M1941SCOPE:
		cg.zoomval = cg_zoomDefaultSniper.value;
		cg.zoomedScope  = 900;      // TODO: add to zoomTable
		zoomindex = ZOOM_SNIPER;
		break;
	case WP_DELISLESCOPE:
		cg.zoomval = cg_zoomDefaultSniper.value;
		cg.zoomedScope  = 900;      // TODO: add to zoomTable
		zoomindex = ZOOM_SNIPER;
		break;
	}

	// constrain user preferred fov to weapon limitations
	if ( cg.zoomval > zoomTable[zoomindex][ZOOM_OUT] ) {
		cg.zoomval = zoomTable[zoomindex][ZOOM_OUT];
	}
	if ( cg.zoomval < zoomTable[zoomindex][ZOOM_IN] ) {
		cg.zoomval = zoomTable[zoomindex][ZOOM_IN];
	}

	cg.zoomTime     = cg.time;
}


/*
==============
CG_PlaySwitchSound
	Get special switching sounds if they're there
==============
*/
void CG_PlaySwitchSound( int lastweap, int newweap ) {
	sfxHandle_t switchsound;

	switchsound = cgs.media.selectSound;

	if ( getAltWeapon( lastweap ) == newweap ) { // alt switch
		switch ( newweap ) {
		case WP_M7:
			switchsound = cg_weapons[newweap].switchSound[0];
			break;
		case WP_M1GARAND:
			if ( cg.predictedPlayerState.ammoclip[lastweap] ) {
				switchsound = cg_weapons[newweap].switchSound[0];
			}
			break;
		default:
			break;
		}
	}

	switch ( newweap ) {
	case WP_KNIFE:
		switchsound = cg_weapons[newweap].switchSound[0];
		break;
	}

	trap_S_StartSound( NULL, cg.snap->ps.clientNum, CHAN_WEAPON, switchsound );
}


/*
==============
CG_FinishWeaponChange
==============
*/
void CG_FinishWeaponChange( int lastweap, int newweap ) {
	int newbank;

	cg.weaponSelectTime = cg.time;  // flash the weapon icon

	// remember which weapon in this bank was last selected so when cycling back
	// to this bank, that weap will be highlighted first
	if ( CG_WeaponIndex( newweap, &newbank, NULL ) ) {
		cg.lastWeapSelInBank[newbank] = newweap;
	}

	if ( lastweap == newweap ) {   // no need to do any more than flash the icon
		return;
	}

	CG_PlaySwitchSound( lastweap, newweap );  //----(SA)	added

	CG_SetSniperZoom( lastweap, newweap );

	// setup for a user call to CG_LastWeaponUsed_f()
	if ( lastweap == cg.lastFiredWeapon ) {
		// don't set switchback for some weaps...
		switch ( lastweap ) {
		case WP_SNIPERRIFLE:
		case WP_SNOOPERSCOPE:
		case WP_FG42SCOPE:
		case WP_M1941SCOPE:
		case WP_DELISLESCOPE:
			break;
		default:
			cg.switchbackWeapon = lastweap;
			break;
		}
	} else {
		// if this ended up having the switchback be the same
		// as the new weapon, set the switchback to the prev
		// selected weapon will become the switchback
		if ( cg.switchbackWeapon == newweap ) {
			cg.switchbackWeapon = lastweap;
		}
	}

	cg.weaponSelect     = newweap;
}


qboolean CG_WeaponSupportsSimpleZoom( int weap ) {
    switch ( weap ) {
        // Disallow: already-scoped / special zoom weapons
        case WP_SNIPERRIFLE:
        case WP_SNOOPERSCOPE:
        case WP_FG42SCOPE:
        case WP_M1941SCOPE:
        case WP_DELISLESCOPE:
            return qfalse;

        // Disallow: binocs / mounted / explosives / melee etc (adjust to your mod)
        case WP_GRENADE_LAUNCHER:
        case WP_GRENADE_PINEAPPLE:
        case WP_DYNAMITE:
        case WP_KNIFE:
            return qfalse;

        default:
            break;
    }

    return qtrue;
}

void CG_ToggleSimpleZoom( void ) {
    cg.simpleZoomed = !cg.simpleZoomed;
    cg.simpleZoomTime = cg.time;
    // let the server know so spectators/followers see the same zoom
    trap_SendClientCommand( va( "simplezoom %i", cg.simpleZoomed ) );
}


void CG_ResetSimpleZoom( void ) {
    if ( cg.simpleZoomed ) {
        cg.simpleZoomed = qfalse;
        cg.simpleZoomTime = cg.time;
        trap_SendClientCommand( "simplezoom 0" );
    }
}


/*
==============
CG_AltfireWeapon_f
	for example, switching between WP_MAUSER and WP_SNIPERRIFLE
==============
*/
void CG_AltWeapon_f( void ) {
	int original, num;

	if ( !cg.snap ) {
		return;
	}

	trap_S_StartSoundEx( NULL, cg.snap->ps.clientNum, CHAN_WEAPON, cgs.media.nullSound, SND_CUTOFF );

	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	if ( cg.snap->ps.eFlags & EF_MG42_ACTIVE ) { // no weapon-switching while mounted, but simple zoom still works
		if ( cg.snap->ps.weaponstate == WEAPON_RELOADING || cg.snap->ps.weaponstate == WEAPON_DROPPING || cg.snap->ps.weaponstate == WEAPON_RAISING ) {
			return;
		}
		CG_ToggleSimpleZoom();
		return;
	}

	if ( cg.time - cg.weaponSelectTime < cg_weaponCycleDelay.integer ) {
		return; // force pause so holding it down won't go too fast

	}

	original = cg.weaponSelect;

	num = getAltWeapon( original );

		if (num == WP_NONE || num == original || !CG_WeaponSelectable(num))
	{
		if (CG_WeaponSupportsSimpleZoom(original))
		{

			if (cg.snap->ps.weaponstate == WEAPON_RELOADING || cg.snap->ps.weaponstate == WEAPON_DROPPING  || cg.snap->ps.weaponstate == WEAPON_RAISING )
			{
				return;
			}
			CG_ToggleSimpleZoom();
		}
		return;
	}

	if ( CG_WeaponSelectable( num ) ) {   // new weapon is valid
        trap_S_StartSoundEx( NULL, cg.snap->ps.clientNum, CHAN_WEAPON, cgs.media.nullSound, SND_CUTOFF );
		CG_FinishWeaponChange( original, num );
	}
}


/*
==============
CG_NextWeap

  switchBanks - curweap is the last in a bank, 'qtrue' means go to the next available bank, 'qfalse' means loop to the head of the bank
==============
*/
void CG_NextWeap( qboolean switchBanks ) {
	int bank = 0, cycle = 0, newbank = 0, newcycle = 0;
	int num, curweap;
	qboolean nextbank = qfalse;     // need to switch to the next bank of weapons?
	int i, j;

	num = curweap = cg.weaponSelect;

	CG_WeaponIndex( curweap, &bank, &cycle );     // get bank/cycle of current weapon

	// if you're using an alt mode weapon, try switching back to the parent first
	if ( curweap >= WP_BEGINSECONDARY && curweap <= WP_LASTSECONDARY ) {
		num = getAltWeapon( curweap );    // base any further changes on the parent
		if ( CG_WeaponSelectable( num ) ) {  // the parent was selectable, drop back to that
			CG_FinishWeaponChange( curweap, num );
			return;
		}
	}


//	if ( cg_cycleAllWeaps.integer || !switchBanks ) {
	if ( 1 ) {
		for ( i = 0; i < maxWeapsInBank; i++ ) {
			num = getNextWeapInBankBynum( num );

			CG_WeaponIndex( num, NULL, &newcycle );       // get cycle of new weapon.  if it's lower than the original, then it cycled around

			if ( switchBanks ) {
				if ( newcycle <= cycle ) {
					nextbank = qtrue;
					break;
				}
			} else {    // don't switch banks if you get to the end

				if ( num == curweap ) {    // back to start, just leave it where it is
					return;
				}
			}

			if ( CG_WeaponSelectable( num ) ) {
				break;
			}
		}
	} else {
		nextbank = qtrue;
	}

	if ( nextbank ) {
		for ( i = 0; i < maxWeapBanks; i++ ) {
//			if ( cg_cycleAllWeaps.integer ) {
			if ( 1 ) {
				num = getNextBankWeap( bank + i, cycle, qfalse );   // cycling all weaps always starts the next bank at the bottom
			} else {
				if ( cg.lastWeapSelInBank[bank + i + 1] ) {
					num = cg.lastWeapSelInBank[bank + i + 1];
				} else {
					num = getNextBankWeap( bank + i, cycle, qtrue );
				}
			}

			if ( num == 0 ) {
				continue;
			}

			if ( CG_WeaponSelectable( num ) ) {  // first entry in bank was selectable, no need to scan the bank
				break;
			}

			CG_WeaponIndex( num, &newbank, &newcycle );   // get the bank of the new weap

			for ( j = newcycle; j < maxWeapsInBank; j++ ) {
				num = getNextWeapInBank( newbank, j );

				if ( CG_WeaponSelectable( num ) ) {  // found selectable weapon
					break;
				}

				num = 0;
			}

			if ( num ) {   // a selectable weapon was found in the current bank
				break;
			}
		}
	}

	CG_FinishWeaponChange( curweap, num ); //----(SA)
}

/*
==============
CG_PrevWeap

  switchBanks - curweap is the last in a bank
		'qtrue'  - go to the next available bank
		'qfalse' - loop to the head of the bank
==============
*/
void CG_PrevWeap( qboolean switchBanks ) {
	int bank = 0, cycle = 0, newbank = 0, newcycle = 0;
	int num, curweap;
	qboolean prevbank = qfalse;     // need to switch to the next bank of weapons?
	int i, j;

	num = curweap = cg.weaponSelect;

	CG_WeaponIndex( curweap, &bank, &cycle );     // get bank/cycle of current weapon

	// if you're using an alt mode weapon, try switching back to the parent first
	if ( curweap >= WP_BEGINSECONDARY && curweap <= WP_LASTSECONDARY ) {
		num = getAltWeapon( curweap );    // base any further changes on the parent
		if ( CG_WeaponSelectable( num ) ) {  // the parent was selectable, drop back to that
			CG_FinishWeaponChange( curweap, num );
			return;
		}
	}

	// initially, just try to find a lower weapon in the current bank
//	if ( cg_cycleAllWeaps.integer || !switchBanks ) {
	if ( 1 ) {

//		if(cycle == 0) {		// already at bottom of list
//			prevbank = qtrue;
//		} else {
		for ( i = cycle; i >= 0; i-- ) {
//				num = getPrevWeapInBank(bank, i);
			num = getPrevWeapInBankBynum( num );

			CG_WeaponIndex( num, NULL, &newcycle );         // get cycle of new weapon.  if it's greater than the original, then it cycled around

			if ( switchBanks ) {
				if ( newcycle > ( cycle - 1 ) ) {
					prevbank = qtrue;
					break;
				}
			} else {        // don't switch banks if you get to the end
				if ( num == curweap ) {     // back to start, just leave it where it is
					return;
				}
			}

			if ( CG_WeaponSelectable( num ) ) {
				break;
			}
		}
//		}
	} else {
		prevbank = qtrue;
	}

	// cycle to previous bank.
	//	if cycleAllWeaps: find highest weapon in bank
	//		else: try to find weap in bank that matches cycle position
	//			else: use base weap in bank

	if ( prevbank ) {
		for ( i = 0; i < maxWeapBanks; i++ ) {
//			if ( cg_cycleAllWeaps.integer ) {
			if ( 1 ) {
				num = getPrevBankWeap( bank - i, cycle, qfalse );   // cycling all weaps always starts the next bank at the bottom
			} else {
				num = getPrevBankWeap( bank - i, cycle, qtrue );
			}

			if ( num == 0 ) {
				continue;
			}

			if ( CG_WeaponSelectable( num ) ) {  // first entry in bank was selectable, no need to scan the bank
				break;
			}

			CG_WeaponIndex( num, &newbank, &newcycle );   // get the bank of the new weap

			for ( j = maxWeapsInBank; j > 0; j-- ) {
				num = getPrevWeapInBank( newbank, j );

				if ( CG_WeaponSelectable( num ) ) {  // found selectable weapon
					break;
				}

				num = 0;
			}

			if ( num ) {   // a selectable weapon was found in the current bank
				break;
			}
		}
	}

	CG_FinishWeaponChange( curweap, num ); //----(SA)
}


/*
==============
CG_LastWeaponUsed_f
==============
*/
void CG_LastWeaponUsed_f( void ) {

	if ( cg.time - cg.weaponSelectTime < cg_weaponCycleDelay.integer ) {
		return; // force pause so holding it down won't go too fast

	}

	trap_S_StartSoundEx( NULL, cg.snap->ps.clientNum, CHAN_WEAPON, cgs.media.nullSound, SND_CUTOFF );

	cg.weaponSelectTime = cg.time;  // flash the current weapon icon

	if ( !cg.switchbackWeapon ) {
		cg.switchbackWeapon = cg.weaponSelect;
		return;
	}

	if ( CG_WeaponSelectable( cg.switchbackWeapon ) ) {
		CG_FinishWeaponChange( cg.weaponSelect, cg.switchbackWeapon );
	} else {    // switchback no longer selectable, reset cycle
		cg.switchbackWeapon = 0;
	}

}

/*
==============
CG_NextWeaponInBank_f
==============
*/
void CG_NextWeaponInBank_f( void ) {

	if ( cg.time - cg.weaponSelectTime < cg_weaponCycleDelay.integer ) {
		return; // force pause so holding it down won't go too fast

	}

	trap_S_StartSoundEx( NULL, cg.snap->ps.clientNum, CHAN_WEAPON, cgs.media.nullSound, SND_CUTOFF );

	// this cvar is an option that lets the player use his weapon switching keys (probably the mousewheel)
	// for zooming (binocs/snooper/sniper/etc.)
	if ( cg.zoomval ) {
		if ( cg_useWeapsForZoom.integer == 1 ) {
			CG_ZoomIn_f();
			return;
		} else if ( cg_useWeapsForZoom.integer == 2 ) {
			CG_ZoomOut_f();
			return;
		}
	}

	cg.weaponSelectTime = cg.time;  // flash the current weapon icon

	CG_NextWeap( qfalse );
}

/*
==============
CG_PrevWeaponInBank_f
==============
*/
void CG_PrevWeaponInBank_f( void ) {

	if ( cg.time - cg.weaponSelectTime < cg_weaponCycleDelay.integer ) {
		return; // force pause so holding it down won't go too fast

	}

	trap_S_StartSoundEx( NULL, cg.snap->ps.clientNum, CHAN_WEAPON, cgs.media.nullSound, SND_CUTOFF );

	// this cvar is an option that lets the player use his weapon switching keys (probably the mousewheel)
	// for zooming (binocs/snooper/sniper/etc.)
	if ( cg.zoomval ) {
		if ( cg_useWeapsForZoom.integer == 2 ) {
			CG_ZoomIn_f();
			return;
		} else if ( cg_useWeapsForZoom.integer == 1 ) {
			CG_ZoomOut_f();
			return;
		}
	}

	cg.weaponSelectTime = cg.time;  // flash the current weapon icon

	CG_PrevWeap( qfalse );
}


/*
==============
CG_PerkChooserMove

Moves the highlight in an open perk choice window (see "perkmenu" in
cg_servercmds.c) and syncs it back to the server. Called instead of the
normal weapon-cycle behavior below while the window is active, so mouse
wheel browses the perk list rather than switching weapons.
==============
*/
static void CG_PerkChooserMove( int dir ) {
	int perk;

	if ( cg.time - cg.perkChooserSelectTime < cg_weaponCycleDelay.integer ) {
		return; // force pause so holding it down won't go too fast
	}
	cg.perkChooserSelectTime = cg.time;

	perk = cg.perkChooserSelected + dir;
	if ( perk < PERK_RESILIENCE ) {
		perk = NUM_PERKS - 1;
	} else if ( perk >= NUM_PERKS ) {
		perk = PERK_RESILIENCE;
	}

	cg.perkChooserSelected = perk;
	trap_SendClientCommand( va( "perksel %d", perk ) );
}

/*
==============
CG_NextWeapon_f
==============
*/
void CG_NextWeapon_f( void ) {

	if ( !cg.snap ) {
		return;
	}

	trap_S_StartSoundEx( NULL, cg.snap->ps.clientNum, CHAN_WEAPON, cgs.media.nullSound, SND_CUTOFF );

	if ( cg.perkChooserActive ) {
		CG_PerkChooserMove( 1 );
		return;
	}

	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	// this cvar is an option that lets the player use his weapon switching keys (probably the mousewheel)
	// for zooming (binocs/snooper/sniper/etc.)
	if ( cg.zoomval ) {
		if ( cg_useWeapsForZoom.integer == 1 ) {
			CG_ZoomIn_f();
			return;
		} else if ( cg_useWeapsForZoom.integer == 2 ) {
			CG_ZoomOut_f();
			return;
		}
	}

	if ( cg.time - cg.weaponSelectTime < cg_weaponCycleDelay.integer ) {
		return; // force pause so holding it down won't go too fast

	}
	cg.weaponSelectTime = cg.time;  // flash the current weapon icon

	CG_NextWeap( qtrue );
}


/*
==============
CG_PrevWeapon_f
==============
*/
void CG_PrevWeapon_f( void ) {
	if ( !cg.snap ) {
		return;
	}

	trap_S_StartSoundEx( NULL, cg.snap->ps.clientNum, CHAN_WEAPON, cgs.media.nullSound, SND_CUTOFF );

	if ( cg.perkChooserActive ) {
		CG_PerkChooserMove( -1 );
		return;
	}

	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	// this cvar is an option that lets the player use his weapon switching keys (probably the mousewheel)
	// for zooming (binocs/snooper/sniper/etc.)
	if ( cg.zoomval ) {
		if ( cg_useWeapsForZoom.integer == 1 ) {
			CG_ZoomOut_f();
			return;
		} else if ( cg_useWeapsForZoom.integer == 2 ) {
			CG_ZoomIn_f();
			return;
		}
	}

	if ( cg.time - cg.weaponSelectTime < cg_weaponCycleDelay.integer ) {
		return; // force pause so holding it down won't go too fast

	}
	cg.weaponSelectTime = cg.time;  // flash the current weapon icon

	CG_PrevWeap( qtrue );
}


/*
==============
CG_WeaponBank_f
	weapon keys are not generally bound directly('bind 1 weapon 1'),
	rather the key is bound to a given bank ('bind 1 weaponbank 1')
==============
*/
void CG_WeaponBank_f( void ) {
	int num = 0, i, curweap;
	int curbank = 0, curcycle = 0, bank = 0, cycle = 0;

	if ( !cg.snap ) {
		return;
	}

	trap_S_StartSoundEx( NULL, cg.snap->ps.clientNum, CHAN_WEAPON, cgs.media.nullSound, SND_CUTOFF );

	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	if ( cg.time - cg.weaponSelectTime < cg_weaponCycleDelay.integer ) {
		return; // force pause so holding it down won't go too fast

	}
	cg.weaponSelectTime = cg.time;  // flash the current weapon icon

	bank = atoi( CG_Argv( 1 ) );

	if ( bank <= 0 || bank > maxWeapBanks ) {
		return;
	}

	curweap = cg.weaponSelect;
	CG_WeaponIndex( curweap, &curbank, &curcycle );       // get bank/cycle of current weapon

	if ( !cg.lastWeapSelInBank[bank] ) {
		if ( cg_gameType.integer <= GT_SINGLE_PLAYER ) { // JPW NERVE
			num = weapBanks[bank][0];
		}
		cycle -= 1;   // cycle up to first weap
	} else {
		num = cg.lastWeapSelInBank[bank];
		CG_WeaponIndex( num, &bank, &cycle );
		if ( bank != curbank ) {
			cycle -= 1;
		}

	}

	for ( i = 0; i < maxWeapsInBank; i++ ) {
		num = getNextWeapInBank( bank, cycle + i );

		if ( CG_WeaponSelectable( num ) ) {
			break;
		}
	}

	if ( i == maxWeapsInBank ) {
		return;
	}

	CG_FinishWeaponChange( curweap, num );

}

/*
===============
CG_Weapon_f
===============
*/
void CG_Weapon_f( void ) {
	int num, i, curweap;
	int bank = 0, cycle = 0, newbank = 0, newcycle = 0;
	qboolean banked = qfalse;

	if ( !cg.snap ) {
		return;
	}

	trap_S_StartSoundEx( NULL, cg.snap->ps.clientNum, CHAN_WEAPON, cgs.media.nullSound, SND_CUTOFF );

	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	num = atoi( CG_Argv( 1 ) );

	cg.weaponSelectTime = cg.time;  // flash the current weapon icon

	if ( num <= WP_NONE || num > WP_NUM_WEAPONS ) {
		return;
	}

	curweap = cg.weaponSelect;

	CG_WeaponIndex( curweap, &bank, &cycle );     // get bank/cycle of current weapon
	banked = CG_WeaponIndex( num, &newbank, &newcycle );      // get bank/cycle of requested weapon

	// the new weapon was not found in the reglar banks
	// assume the player want's to go directly to it if possible
	if ( !banked ) {
		if ( CG_WeaponSelectable( num ) ) {
			CG_FinishWeaponChange( curweap, num );
			return;
		}
	}

	if ( bank != newbank ) {
		cycle = newcycle - 1;   //	drop down one from the requested weap's cycle so it will
	}
	//	try to initially cycle up to the requested weapon

	for ( i = 0; i < maxWeapsInBank; i++ ) {
		num = getNextWeapInBank( newbank, cycle + i );

		if ( num == curweap ) { // no other weapons in bank
			return;
		}

		if ( CG_WeaponSelectable( num ) ) {
			break;
		}
	}

	if ( i == maxWeapsInBank ) {
		return;
	}

	CG_FinishWeaponChange( curweap, num );
}

/*
===================
CG_OutOfAmmoChange

The current weapon has just run out of ammo
===================
*/
void CG_OutOfAmmoChange( void ) {
	int i;
	int bank = 0, cycle = 0;
	int equiv = WP_NONE;

	//
	// trivial switching
	//

	// if you're using an alt mode weapon, try switching back to the parent
	// otherwise, switch to the equivalent if you've got it
	if ( cg.weaponSelect >= WP_BEGINSECONDARY && cg.weaponSelect <= WP_LASTSECONDARY ) {
		cg.weaponSelect = equiv = getAltWeapon( cg.weaponSelect );    // base any further changes on the parent
		if ( CG_WeaponSelectable( equiv ) ) {    // the parent was selectable, drop back to that
			CG_FinishWeaponChange( cg.predictedPlayerState.weapon, cg.weaponSelect ); //----(SA)
			return;
		}
	}


	// now try the opposite team's equivalent weap
	equiv = getEquivWeapon( cg.weaponSelect );

	if ( equiv != cg.weaponSelect && CG_WeaponSelectable( equiv ) ) {
		cg.weaponSelect = equiv;
		CG_FinishWeaponChange( cg.predictedPlayerState.weapon, cg.weaponSelect ); //----(SA)
		return;
	}

	//
	// more complicated selection
	//

	// didn't have available alternative or equivalent, try another weap in the bank
	CG_WeaponIndex( cg.weaponSelect, &bank, &cycle );     // get bank/cycle of current weapon

	for ( i = cycle; i < maxWeapsInBank; i++ ) {
		equiv = getNextWeapInBank( bank, i );
		if ( CG_WeaponSelectable( equiv ) ) {  // found a reasonable replacement
			cg.weaponSelect = equiv;
			CG_FinishWeaponChange( cg.predictedPlayerState.weapon, cg.weaponSelect ); //----(SA)
			return;
		}
	}


	// still nothing available, just go to the next
	// available weap using the regular selection scheme
	CG_NextWeap( qtrue );

}

/*
===================================================================================================

WEAPON EVENTS

===================================================================================================
*/

// Note to self this is dead code
void CG_MG42EFX( centity_t *cent ) {
	vec3_t forward;
	vec3_t point;
	refEntity_t flash;

//	trap_S_StartSound( NULL, cent->currentState.number, CHAN_WEAPON, hWeaponSnd );

	VectorCopy( cent->currentState.origin, point );
	AngleVectors( cent->currentState.angles, forward, NULL, NULL );
	VectorMA( point, 40, forward, point );
	trap_R_AddLightToScene( point, 200 + ( rand() & 31 ),1.0, 0.6, 0.23, REF_MUZZLEFLASH_DLIGHT );

	memset( &flash, 0, sizeof( flash ) );
	flash.renderfx = RF_LIGHTING_ORIGIN;
	flash.hModel = cgs.media.mg42muzzleflash;

	VectorCopy( point, flash.origin );
	AnglesToAxis( cg.refdefViewAngles, flash.axis );

	trap_R_AddRefEntityToScene( &flash );
}

void CG_FLAKEFX( centity_t *cent, int whichgun ) {
	entityState_t *ent;
	vec3_t forward, right, up;
	vec3_t point;
	refEntity_t flash;

	ent = &cent->currentState;

	VectorCopy( cent->currentState.pos.trBase, point );
	AngleVectors( cent->currentState.apos.trBase, forward, right, up );

	// gun 1 and 2 were switched
	if ( whichgun == 2 ) {
		VectorMA( point, 136, forward, point );
		VectorMA( point, 31, up, point );
		VectorMA( point, 22, right, point );
	} else if ( whichgun == 1 )     {
		VectorMA( point, 136, forward, point );
		VectorMA( point, 31, up, point );
		VectorMA( point, -22, right, point );
	} else if ( whichgun == 3 )     {
		VectorMA( point, 136, forward, point );
		VectorMA( point, 10, up, point );
		VectorMA( point, 22, right, point );
	} else if ( whichgun == 4 )     {
		VectorMA( point, 136, forward, point );
		VectorMA( point, 10, up, point );
		VectorMA( point, -22, right, point );
	}

	trap_R_AddLightToScene( point, 200 + ( rand() & 31 ),1.0, 0.6, 0.23, REF_MUZZLEFLASH_DLIGHT );

	memset( &flash, 0, sizeof( flash ) );
	flash.renderfx = RF_LIGHTING_ORIGIN;
	flash.hModel = cgs.media.mg42muzzleflash;

	VectorCopy( point, flash.origin );
	AnglesToAxis( cg.refdefViewAngles, flash.axis );

	trap_R_AddRefEntityToScene( &flash );

	trap_S_StartSound( NULL, ent->number, CHAN_WEAPON, hflakWeaponSnd );
}


//----(SA)
/*
==============
CG_MortarEFX
	Right now mostly copied directly from Raf's MG42 FX, but with the optional addtion of smoke
==============
*/
void CG_MortarEFX( centity_t *cent ) {
	refEntity_t flash;

	if ( cent->currentState.density & 1 ) {
		// smoke
		CG_ParticleImpactSmokePuff( cgs.media.smokePuffShader, cent->currentState.origin );
	}

	if ( cent->currentState.density & 2 ) {
		// light
		trap_R_AddLightToScene( cent->currentState.origin, 200 + ( rand() & 31 ), 1.0, 1.0, 1.0, REF_MUZZLEFLASH_DLIGHT );

		// muzzle flash
		memset( &flash, 0, sizeof( flash ) );
		flash.renderfx = RF_LIGHTING_ORIGIN;
		flash.hModel = cgs.media.mg42muzzleflash;
		VectorCopy( cent->currentState.origin, flash.origin );
		AnglesToAxis( cg.refdefViewAngles, flash.axis );
		trap_R_AddRefEntityToScene( &flash );
	}
}

//----(SA)	end


// RF
/*
==============
CG_WeaponFireRecoil
==============
*/
void CG_WeaponFireRecoil( int weapon ) {
	float pitchRecoilAdd, pitchAdd;
	float yawRandom;
	vec3_t recoil;
	pitchRecoilAdd = 0;
	pitchAdd = 0;
	yawRandom = 0;

	const ammotable_t *wt = GetWeaponTableData(weapon);
	weaponClass_t wc = wt->weaponClass;

	if ( cg_thirdPerson.integer ) {
		return;
	}

	if (wc & WEAPON_CLASS_PISTOL)
	{
		yawRandom = 0.5;
		pitchRecoilAdd = 2;
		pitchAdd = 1;
	}
	else if (wc & WEAPON_CLASS_SMG)
	{
		pitchAdd = 1;
		pitchRecoilAdd = 1;
		yawRandom = 1;
	}
	else if (wc & WEAPON_CLASS_SCOPED)
	{
		pitchAdd = 0.8;
	}
	else if ((wc & WEAPON_CLASS_RIFLE_BOLTACTION) ||
			 (wc & WEAPON_CLASS_RIFLE_AUTO))
	{
		pitchAdd = 1;
		yawRandom = 1;
		pitchRecoilAdd = 1.5;
	}
	else if (wc & WEAPON_CLASS_ASSAULT_RIFLE)
	{
		pitchAdd = 1;
		pitchRecoilAdd = 1;
		yawRandom = 1;
	}
	else if (wc & WEAPON_CLASS_SHOTGUN)
	{
		pitchRecoilAdd = 1;
		pitchAdd = 8 + rand() % 3;
		yawRandom = 2;
		pitchAdd *= 0.5;
		yawRandom *= 0.5;
	}
	else if (wc & WEAPON_CLASS_LAUNCHER)
	{
		CG_StartShakeCamera(0.05, 700, cg.snap->ps.origin, 100);
	}
	else if (wc & WEAPON_CLASS_MG)
	{
		pitchRecoilAdd = pow(random(), 8) * (5 + VectorLength(cg.snap->ps.velocity) / 5);
		pitchAdd = (rand() % 5) - 2;
		yawRandom = 1;
		pitchRecoilAdd *= 0.5;
		pitchAdd *= 0.5;
		yawRandom *= 0.5;
	}
	else if (wc & WEAPON_CLASS_MINIGUN)
	{
		pitchRecoilAdd = pow(random(), 8) * (5 + VectorLength(cg.snap->ps.velocity) / 5);
		pitchAdd = (rand() % 5) - 2;
		yawRandom = 1;
		pitchRecoilAdd *= 0.5;
		pitchAdd *= 0.5;
		yawRandom *= 0.5;
	} else 
	{
		return;
	}

	// calc the recoil
	recoil[YAW] = crandom() * yawRandom;
	recoil[ROLL] = -recoil[YAW];    // why not
	recoil[PITCH] = -pitchAdd;
	// scale it up a bit (easier to modify this while tweaking)
	VectorScale( recoil, 30, recoil );
	// set the recoil
	VectorCopy( recoil, cg.kickAVel );
	// set the recoil
	cg.recoilPitch -= pitchRecoilAdd;
}



/*
================
CG_FireWeapon

Caused by an EV_FIRE_WEAPON event

================
*/
void CG_FireWeapon( centity_t *cent ) {
	entityState_t *ent;
	int c;
	weaponInfo_t    *weap;
	sfxHandle_t     *firesound;
	sfxHandle_t     *fireEchosound;

	ent = &cent->currentState;

	// LT's binocular arty call-in reuses EV_FIRE_WEAPON to reach g_weapon.c's hook - skip fire fx here, it isn't really firing
	if ( cent->currentState.eFlags & EF_ZOOMING ) {
		return;
	}

	// Rafael - mg42
	if ( ( cent->currentState.clientNum == cg.snap->ps.clientNum && cg.snap->ps.persistant[PERS_HWEAPON_USE] ) ||
		 ( cent->currentState.clientNum != cg.snap->ps.clientNum && ( cent->currentState.eFlags & EF_MG42_ACTIVE ) ) ) {
		if ( cg.snap->ps.gunfx ) {
			return;
		}

		trap_S_StartSound( NULL, cent->currentState.number, CHAN_WEAPON, hWeaponSnd );
		if ( cg_brassTime.integer > 0 ) {
			CG_RifleEjectBrass( cent );
		}

		//	CG_MG42EFX (cent);

		return;
	}

	if ( ent->weapon == WP_NONE ) {
		return;
	}
	if ( ent->weapon >= WP_NUM_WEAPONS ) {
		CG_Error( "CG_FireWeapon: ent->weapon >= WP_NUM_WEAPONS" );
		return;
	}
	weap = &cg_weapons[ ent->weapon ];

	cg.lastFiredWeapon = ent->weapon;   //----(SA)	added

	// mark the entity as muzzle flashing, so when it is added it will
	// append the flash to the weapon model
	cent->muzzleFlashTime = cg.time;

	// RF, kick angles
	if ( ent->number == cg.snap->ps.clientNum ) {
		CG_WeaponFireRecoil( ent->weapon );
	}

	// lightning gun only does this this on initial press
	if ( ent->weapon == WP_FLAMETHROWER ) {
		if ( cent->pe.lightningFiring ) {
			return;
		}
	} else if (   ent->weapon == WP_GRENADE_LAUNCHER ||
				  ent->weapon == WP_GRENADE_PINEAPPLE ||
				  ent->weapon == WP_DYNAMITE ||
				  ent->weapon == WP_GRENADE_SMOKE ) { // JPW NERVE
		if ( ent->weapon == WP_GRENADE_SMOKE ) {
			CG_Printf( "smoke grenade!\n" );
		}
		if ( ent->apos.trBase[0] > 0 ) { // underhand
			return;
		}
	}

	// play quad sound if needed
	if ( cent->currentState.powerups & ( 1 << PW_QUAD ) ) {
		trap_S_StartSound( NULL, cent->currentState.number, CHAN_ITEM, cgs.media.quadSound );
	}

	if ( ( cent->currentState.event & ~EV_EVENT_BITS ) == EV_FIRE_WEAPON_LASTSHOT ) {
		firesound = &weap->lastShotSound[0];
		fireEchosound = &weap->flashEchoSound[0];

		// try to use the lastShotSound, but don't assume it's there.
		// if a weapon without the sound calls it, drop back to regular fire sound

		for ( c = 0; c < 4; c++ ) {
			if ( !firesound[c] ) {
				break;
			}
		}
		if ( !c ) {
			firesound = &weap->flashSound[0];
			fireEchosound = &weap->flashEchoSound[0];
		}
	} else {
		firesound = &weap->flashSound[0];
		fireEchosound = &weap->flashEchoSound[0];
	}


	// play a sound
	for ( c = 0 ; c < 4 ; c++ ) {
		if ( !firesound[c] ) {
			break;
		}
	}
	if ( c > 0 ) {
		c = rand() % c;
		if ( firesound[c] ) {
			trap_S_StartSound( NULL, ent->number, CHAN_WEAPON, firesound[c] );

			if ( fireEchosound && fireEchosound[c] ) { // check for echo
				centity_t   *cent;
				vec3_t porg, gorg, norm;    // player/gun origin
				float gdist;

				cent = &cg_entities[ent->number];
				VectorCopy( cent->currentState.pos.trBase, gorg );
				VectorCopy( cg.refdef.vieworg, porg );
				VectorSubtract( gorg, porg, norm );
				gdist = VectorNormalize( norm );
				if ( gdist > 512 && gdist < 4096 ) {   // temp dist.  TODO: use numbers that are weapon specific
					// use gorg as the new sound origin
					VectorMA( cg.refdef.vieworg, 64, norm, gorg );    // sound-on-a-stick
					trap_S_StartSound( gorg, ent->number, CHAN_WEAPON, fireEchosound[c] );
				}
			}
		}
	}

	// do brass ejection
	if ( weap->ejectBrassFunc && cg_brassTime.integer > 0 ) {
		weap->ejectBrassFunc( cent );
	}
}


// Ridah
/*
=================
CG_AddSparks
=================
*/
void CG_AddSparks( vec3_t origin, vec3_t dir, int speed, int duration, int count, float randScale ) {
	localEntity_t   *le;
	refEntity_t     *re;
	vec3_t velocity;
	int i;

	for ( i = 0; i < count; i++ ) {
		le = CG_AllocLocalEntity();
		re = &le->refEntity;

		VectorSet( velocity, dir[0] + crandom() * randScale, dir[1] + crandom() * randScale, dir[2] + crandom() * randScale );
		VectorScale( velocity, (float)speed, velocity );

		le->leType = LE_SPARK;
		le->startTime = cg.time;
		le->endTime = le->startTime + duration - (int)( 0.5 * random() * duration );
		le->lastTrailTime = cg.time;

		VectorCopy( origin, re->origin );
		AxisCopy( axisDefault, re->axis );

		le->pos.trType = TR_GRAVITY_LOW;
		VectorCopy( origin, le->pos.trBase );
		VectorMA( le->pos.trBase, 2 + random() * 4, dir, le->pos.trBase );
		VectorCopy( velocity, le->pos.trDelta );
		le->pos.trTime = cg.time;

		le->refEntity.customShader = cgs.media.sparkParticleShader;

		le->bounceFactor = 0.9;

//		le->leBounceSoundType = LEBS_BLOOD;
//		le->leMarkType = LEMT_BLOOD;
	}
}
/*
=================
CG_AddBulletParticles
=================
*/
void CG_AddBulletParticles( vec3_t origin, vec3_t dir, int speed, int duration, int count, float randScale ) {
//	localEntity_t	*le;
//	refEntity_t		*re;
	vec3_t velocity, pos;
	int i;
/*
	// add the falling streaks
	for (i=0; i<count/3; i++) {
		le = CG_AllocLocalEntity();
		re = &le->refEntity;

		VectorSet( velocity, dir[0] + crandom()*randScale, dir[1] + crandom()*randScale, dir[2] + crandom()*randScale );
		VectorScale( velocity, (float)speed*3, velocity );

		le->leType = LE_SPARK;
		le->startTime = cg.time;
		le->endTime = le->startTime + duration - (int)(0.5 * random() * duration);
		le->lastTrailTime = cg.time;

		VectorCopy( origin, re->origin );
		AxisCopy( axisDefault, re->axis );

		le->pos.trType = TR_GRAVITY;
		VectorCopy( origin, le->pos.trBase );
		VectorMA( le->pos.trBase, 2 + random()*4, dir, le->pos.trBase );
		VectorCopy( velocity, le->pos.trDelta );
		le->pos.trTime = cg.time;

		le->refEntity.customShader = cgs.media.bulletParticleTrailShader;
//		le->refEntity.customShader = cgs.media.sparkParticleShader;

		le->bounceFactor = 0.9;

//		le->leBounceSoundType = LEBS_BLOOD;
//		le->leMarkType = LEMT_BLOOD;
	}
*/
	// add the falling particles
	for ( i = 0; i < count; i++ ) {

		VectorSet( velocity, dir[0] + crandom() * randScale, dir[1] + crandom() * randScale, dir[2] + crandom() * randScale );
		VectorScale( velocity, (float)speed, velocity );

		VectorCopy( origin, pos );
		VectorMA( pos, 2 + random() * 4, dir, pos );

		CG_ParticleBulletDebris( pos, velocity, 300 + rand() % 300 );

	}
}

//----(SA)	from MP
/*
=================
CG_AddDirtBulletParticles
=================
*/
void CG_AddDirtBulletParticles( vec3_t origin, vec3_t dir, int speed, int duration, int count, float randScale,
								float width, float height, float alpha, char *shadername ) { // JPW NERVE
	vec3_t velocity, pos;
	int i;

	// add the big falling particle
	VectorSet( velocity, 0, 0, (float)speed );

	VectorCopy( origin, pos );

// JPW NERVE
	CG_ParticleDirtBulletDebris_Core( pos, velocity, duration, width,height, alpha, shadername ); //600 + rand()%300 ); // keep central one
	for ( i = 0; i < count; i++ ) {
		VectorSet( velocity, dir[0] * crandom() * speed * randScale,
				   dir[1] * crandom() * speed * randScale, dir[2] * random() * speed );
		CG_ParticleDirtBulletDebris_Core( pos, velocity, duration + ( rand() % ( duration >> 1 ) ), // dur * 0.5, but int
										  width,height, alpha, shadername );
	}
}
//----(SA)	end

/*
=================
CG_AddDebris
=================
*/
void CG_AddDebris( vec3_t origin, vec3_t dir, int speed, int duration, int count ) {
	localEntity_t   *le;
	refEntity_t     *re;
	vec3_t velocity, unitvel;
	float timeAdd;
	int i;

	for ( i = 0; i < count; i++ ) {
		le = CG_AllocLocalEntity();
		re = &le->refEntity;

		VectorSet( unitvel, dir[0] + crandom() * 0.9, dir[1] + crandom() * 0.9, fabs( dir[2] ) > 0.5 ? dir[2] * ( 0.2 + 0.8 * random() ) : random() * 0.6 );
		VectorScale( unitvel, (float)speed + (float)speed * 0.5 * crandom(), velocity );

		le->leType = LE_DEBRIS;
		le->startTime = cg.time;
		le->endTime = le->startTime + duration + (int)( (float)duration * 0.8 * crandom() );
		le->lastTrailTime = cg.time;

		VectorCopy( origin, re->origin );
		AxisCopy( axisDefault, re->axis );

		le->pos.trType = TR_GRAVITY_LOW;
		VectorCopy( origin, le->pos.trBase );
		VectorCopy( velocity, le->pos.trDelta );
		le->pos.trTime = cg.time;

		timeAdd = 10.0 + random() * 40.0;
		BG_EvaluateTrajectory( &le->pos, cg.time + (int)timeAdd, le->pos.trBase );

		le->bounceFactor = 0.5;

//		if (!rand()%2)
//			le->effectWidth = 0;	// no flame
//		else
		le->effectWidth = 5 + random() * 5;

//		if (rand()%3)
		le->effectFlags |= 1;       // smoke trail


//		le->leBounceSoundType = LEBS_BLOOD;
//		le->leMarkType = LEMT_BLOOD;
	}
}
// done.


/*
==============
CG_WaterRipple
==============
*/
void CG_WaterRipple( qhandle_t shader, vec3_t loc, vec3_t dir, int size, int lifetime ) {
	localEntity_t   *le;
	refEntity_t     *re;

	le = CG_AllocLocalEntity();
	le->leType          = LE_SCALE_FADE;
	le->leFlags         = LEF_PUFF_DONT_SCALE;

	le->startTime       = cg.time;
	le->endTime         = cg.time + lifetime;
	le->lifeRate        = 1.0 / ( le->endTime - le->startTime );

	re = &le->refEntity;
	VectorCopy( loc, re->origin );
	re->shaderTime      = cg.time / 1000.0f;
	re->reType          = RT_SPLASH;
	re->radius          = size;
	re->customShader    = shader;
	re->shaderRGBA[0]   = 0xff;
	re->shaderRGBA[1]   = 0xff;
	re->shaderRGBA[2]   = 0xff;
	re->shaderRGBA[3]   = 0xff;
	le->color[3]        = 1.0;
}

/*
=================
CG_MissileHitWall

Caused by an EV_MISSILE_MISS event, or directly by local bullet tracing

ClientNum is a dummy field used to define what sort of effect to spawn
=================
*/
void CG_MissileHitWall( int weapon, int clientNum, vec3_t origin, vec3_t dir, int surfFlags ) {
	qhandle_t mod;
	qhandle_t mark;
	qhandle_t shader;
	sfxHandle_t sfx, sfx2;
	vec3_t lightColor, tmpv, tmpv2;
	trace_t trace;
	float radius;
	float light;
	localEntity_t *le;
	int r = 0;
	qboolean alphaFade = qfalse;
	qboolean isSprite;
	int duration;
	int lightOverdraw;
	vec3_t sprOrg;
	vec3_t sprVel;
	int i, j;
	float shakeAmt;
	int shakeDur, shakeRad;
	const ammotable_t *wt;
	weaponClass_t wc;

	shakeAmt = 0;
	shakeDur = shakeRad = 0;
	mark = 0;
	radius = 32;
	sfx = 0;
	sfx2 = 0;
	mod = 0;
	shader = 0;
	light = 0;
	lightColor[0] = 1;
	lightColor[1] = 1;
	lightColor[2] = 0;
	lightOverdraw = 0;

	isSprite = qfalse;
	duration = 600;

	if ( surfFlags & SURF_SKY ) {
		return;
	}

	wt = GetWeaponTableData( weapon );

	if ( !wt ) {
		return;
	}

	wc = wt->weaponClass;

	/*
	==============
	Special fake / legacy weapon ids

	Keep these explicit because they may not have normal weapon table data,
	or they may need unique behavior.
	==============
	*/
	switch ( weapon ) {
	case WP_MORTAR:
		sfx = cgs.media.sfx_rockexp;
		mark = cgs.media.burnMarkShader;
		radius = 64;
		light = 300;
		isSprite = qtrue;
		duration = 1000;
		lightColor[0] = 0.75;
		lightColor[1] = 0.5;
		lightColor[2] = 0.1;

		shakeAmt = 0.15f;
		shakeDur = 600;
		shakeRad = 700;

		VectorScale( dir, 16, sprVel );
		for ( i = 0; i < 5; i++ ) {
			for ( j = 0; j < 3; j++ ) {
				sprOrg[j] = origin[j] + 64 * dir[j] + 24 * crandom();
			}
			sprVel[2] += rand() % 50;
			CG_ParticleExplosion( "blacksmokeanimb", sprOrg, sprVel, 3500 + rand() % 250, 10, 250 + rand() % 60 );
		}

		VectorMA( origin, 24, dir, sprOrg );
		VectorScale( dir, 64, sprVel );
		CG_ParticleExplosion( "expblue", sprOrg, sprVel, 1000, 20, 300 );
		break;

	case VERYBIGEXPLOSION:
		sfx = cgs.media.sfx_rockexp;
		mark = cgs.media.burnMarkShader;
		radius = 64;
		light = 600;
		isSprite = qtrue;
		duration = 1000;
		lightColor[0] = 0.75;
		lightColor[1] = 0.5;
		lightColor[2] = 0.1;

		VectorMA( origin, 24, dir, sprOrg );
		VectorScale( dir, 64, sprVel );

		shakeAmt = 0.2f;
		shakeDur = 1000;
		shakeRad = 1200;

		if ( cg_gameType.integer <= GT_SINGLE_PLAYER ) {
			CG_ParticleExplosion( "explode1", sprOrg, sprVel, 1200, 20, 300 );

			for ( i = 0; i < 4; i++ ) {
				for ( j = 0; j < 3; j++ ) {
					sprOrg[j] = origin[j] + 32 * dir[j] + 32 * crandom();
				}
				CG_ParticleExplosion( "explode1", sprOrg, sprVel, 1200, 40, 160 + rand() % 120 );
			}

			CG_AddDebris( origin, dir, 120, 2000, 7 + rand() % 2 );
		}
		break;

	case WP_ARTY:
		sfx = cgs.media.sfx_rockexp;
		mark = cgs.media.burnMarkShader;
		radius = 96;
		light = 700;
		isSprite = qtrue;
		duration = 1200;
		lightColor[0] = 0.75;
		lightColor[1] = 0.5;
		lightColor[2] = 0.1;

		shakeAmt = 0.3f;
		shakeDur = 1200;
		shakeRad = 1500;

		if ( cg_gameType.integer <= GT_SINGLE_PLAYER ) {
			VectorScale( dir, 16, sprVel );
			for ( i = 0; i < 5; i++ ) {
				for ( j = 0; j < 3; j++ ) {
					sprOrg[j] = origin[j] + 64 * dir[j] + 24 * crandom();
				}
				sprVel[2] += rand() % 50;
				CG_ParticleExplosion( "blacksmokeanimb", sprOrg, sprVel, 3500 + rand() % 250, 10, 250 + rand() % 60 );
			}

			VectorMA( origin, 24, dir, sprOrg );
			VectorScale( dir, 64, sprVel );
			CG_ParticleExplosion( "explode1", sprOrg, sprVel, 1400, 30, 300 );

			for ( i = 0; i < 4; i++ ) {
				for ( j = 0; j < 3; j++ ) {
					sprOrg[j] = origin[j] + 32 * dir[j] + 32 * crandom();
				}
				CG_ParticleExplosion( "explode1", sprOrg, sprVel, 1200, 40, 160 + rand() % 120 );
			}

			CG_AddDebris( origin, dir, 160, 2000, 9 + rand() % 3 );
		}
		break;

	case WP_SMOKETRAIL: // arty spotter/trace round - blue signal smoke marking the target, not a real detonation
		{
			vec3_t puffOrg, puffVel;
			float rnd;
			int k;

			for ( k = 0; k < 6; k++ ) {
				puffOrg[0] = origin[0] + 12 * crandom();
				puffOrg[1] = origin[1] + 12 * crandom();
				puffOrg[2] = origin[2] + 4 * k;

				puffVel[0] = 8 * crandom();
				puffVel[1] = 8 * crandom();
				puffVel[2] = 20 + rand() % 20;

				rnd = random();
				CG_SmokePuff( puffOrg, puffVel, 30 + rnd * 90, rnd * 0.5f + 0.5f, rnd * 0.5f + 0.5f, 1, 0.5f,
							  3000 + rand() % 1500, cg.time, 200, 0, cgs.media.smokePuffShader );
			}
		}
		break;

	default:
		/*
		==============
		Normal weapon-class based impact behavior
		==============
		*/

		if ( wc & WEAPON_CLASS_MELEE ) {
			sfx = cgs.media.sfx_knifehit[4];
			mark = cgs.media.bulletMarkShader;
			radius = 1 + rand() % 2;

			CG_AddBulletParticles( origin, dir, 20, 800, 3 + rand() % 6, 1.0 );
		}
		else if ( wt->bulletBased == qtrue ) {
			if ( cg_gameType.integer <= GT_SINGLE_PLAYER ) {
				r = rand() & 31;
			}

			if ( r == 3 ) {
				sfx = cgs.media.sfx_ric1;
			} else if ( r == 2 ) {
				sfx = cgs.media.sfx_ric2;
			} else if ( r == 1 ) {
				sfx = cgs.media.sfx_ric3;
			}

			if ( !clientNum ) {
				CG_AddSparks( origin, dir, 350, 200, 15 + rand() % 7, 0.2 );
			} else if ( clientNum == 1 ) {
				vec3_t d, o;

				VectorMA( origin, 12, dir, o );
				VectorScale( dir, 7, d );
				d[2] += 16;

				if ( surfFlags & SURF_SNOW ) {
					CG_AddDirtBulletParticles( origin, dir, 120, 900, 3, 0.6f, 20, 4, 0.3f, "water_splash" );
				}

				if ( surfFlags & ( SURF_GRASS | SURF_GRAVEL ) ) {
					CG_AddDirtBulletParticles( origin, dir, 190, 800, 3, 0.1f, 60, 10, 0.5f, "dirt_splash" );
				} else {
					CG_ParticleImpactSmokePuff( cgs.media.smokeParticleShader, o );

					CG_AddBulletParticles( origin, dir, 20, 800, 3 + rand() % 6, 1.0 );

					if ( sfx && ( rand() % 3 == 0 ) ) {
						CG_AddSparks( origin, dir, 450, 300, 3 + rand() % 3, 0.5 );
					}
				}
			} else if ( clientNum == 2 ) {
				sfx = 0;
				mark = 0;

				CG_WaterRipple( cgs.media.wakeMarkShaderAnim, origin, tv( 0, 0, 1 ), 32, 1000 );

				CG_AddDirtBulletParticles( origin, dir, 190, 900, 5, 0.5f, 50, 12, 0.125f, "water_splash" );
			}

			if ( ( wc & WEAPON_CLASS_SCOPED ) || ( Distance( cg.refdef.vieworg, origin ) < 384 ) ) {
				if ( clientNum ) {
					mark = cgs.media.bulletMarkShader;
					alphaFade = qtrue;
					radius = 1.5f + rand() % 2;

					if ( surfFlags & SURF_METAL ) {
						sfx = cgs.media.sfx_bullet_metalhit[rand() % 3];
						mark = cgs.media.bulletMarkShaderMetal;
						alphaFade = qtrue;
					} else if ( surfFlags & SURF_WOOD ) {
						sfx = cgs.media.sfx_bullet_woodhit[rand() % 3];
						mark = cgs.media.bulletMarkShaderWood;
						alphaFade = qtrue;
						radius += 1;
					} else if ( surfFlags & SURF_CERAMIC ) {
						sfx = cgs.media.sfx_bullet_ceramichit[rand() % 3];
						mark = cgs.media.bulletMarkShaderCeramic;
						alphaFade = qtrue;
						radius += 2;
					} else if ( surfFlags & SURF_GLASS ) {
						sfx = cgs.media.sfx_bullet_glasshit[rand() % 3];
						mark = cgs.media.bulletMarkShaderGlass;
						alphaFade = qtrue;
					}
				}
			}
		}
		else if ( wc & ( WEAPON_CLASS_GRENADE | WEAPON_CLASS_RIFLENADE | WEAPON_CLASS_LANDMINE ) ) {
			shader = cgs.media.rocketExplosionShader;
			sfx = cgs.media.sfx_rockexp;
			mark = cgs.media.burnMarkShader;
			radius = 64;
			light = 300;
			isSprite = qtrue;
			duration = 1000;
			lightColor[0] = 0.75;
			lightColor[1] = 0.5;
			lightColor[2] = 0.1;

			if ( weapon != WP_GRENADE_SMOKE ) {
				shakeAmt = 0.15f;
				shakeDur = 1000;
				shakeRad = 600;
			}

			VectorMA( origin, 16, dir, sprOrg );
			VectorScale( dir, 100, sprVel );

			if ( CG_PointContents( origin, 0 ) & CONTENTS_WATER ) {
				sfx = cgs.media.sfx_grenexpWater;

				VectorCopy( origin, tmpv );
				tmpv[2] += 10000;

				trap_CM_BoxTrace( &trace, tmpv, origin, NULL, NULL, 0, MASK_WATER );
				CG_WaterRipple( cgs.media.wakeMarkShaderAnim, trace.endpos, dir, 150, 1000 );

				CG_AddDirtBulletParticles( trace.endpos, dir, 400, 900, 15, 0.5f, 256, 128, 0.125f, "water_splash" );
			} else {
				VectorCopy( origin, tmpv );
				tmpv[2] += 20;
				VectorCopy( origin, tmpv2 );
				tmpv2[2] -= 20;

				trap_CM_BoxTrace( &trace, tmpv, tmpv2, NULL, NULL, 0, MASK_SHOT );

				if ( ( trace.surfaceFlags & SURF_GRASS ) || ( trace.surfaceFlags & SURF_GRAVEL ) ) {
					CG_AddDirtBulletParticles( origin, dir, 400, 2000, 10, 0.5f, 200, 75, 0.25f, "dirt_splash" );
				}

				CG_ParticleExplosion( "expblue", sprOrg, sprVel, 700, 20, 160 );
				CG_AddDebris( origin, dir, 280, 1400, 7 + rand() % 2 );
			}
		}
		else if ( wc & WEAPON_CLASS_DYNAMITE ) {
			shader = cgs.media.rocketExplosionShader;
			sfx = cgs.media.sfx_dynamiteexp;
			sfx2 = cgs.media.sfx_dynamiteexpDist;
			mark = cgs.media.burnMarkShader;
			radius = 64;
			light = 300;
			isSprite = qtrue;
			duration = 1000;
			lightColor[0] = 0.75;
			lightColor[1] = 0.5;
			lightColor[2] = 0.1;

			shakeAmt = 0.25f;
			shakeDur = 2800;
			shakeRad = 8192;

			if ( cg_gameType.integer <= GT_SINGLE_PLAYER ) {
				VectorScale( dir, 16, sprVel );

				for ( i = 0; i < 5; i++ ) {
					for ( j = 0; j < 3; j++ ) {
						sprOrg[j] = origin[j] + 64 * dir[j] + 24 * crandom();
					}
					sprVel[2] += rand() % 50;
					CG_ParticleExplosion( "blacksmokeanimb", sprOrg, sprVel, 3500 + rand() % 250, 10, 250 + rand() % 60 );
				}

				VectorMA( origin, 16, dir, sprOrg );
				VectorScale( dir, 100, sprVel );

				CG_ParticleExplosion( "explode1", sprOrg, sprVel, 1200, 9, 300 );

				CG_AddDebris( origin, dir, 280, 1400, 7 + rand() % 2 );
			}
		}
		else if ( wc & WEAPON_CLASS_LAUNCHER ) {
			sfx = cgs.media.sfx_rockexp;
			mark = cgs.media.burnMarkShader;
			radius = 64;
			light = 600;
			isSprite = qtrue;
			duration = 1000;
			lightColor[0] = 0.75;
			lightColor[1] = 0.5;
			lightColor[2] = 0.1;

			VectorMA( origin, 24, dir, sprOrg );
			VectorScale( dir, 64, sprVel );

			shakeAmt = 0.15f;
			shakeDur = 800;
			shakeRad = 1000;

			if ( cg_gameType.integer <= GT_SINGLE_PLAYER ) {
				CG_ParticleExplosion( "explode1", sprOrg, sprVel, 1400, 40, 70 );

				for ( i = 0; i < 4; i++ ) {
					if ( i < 2 ) {
						for ( j = 0; j < 3; j++ ) {
							sprOrg[j] = origin[j] + 24 * dir[j] + 16 * crandom();
						}
						CG_ParticleExplosion( "explode1", sprOrg, sprVel, 1400, 15, 40 + rand() % 30 );
					}
				}

				CG_AddDebris( origin, dir, 120, 2000, 7 + rand() % 2 );
			}
		}
		else if ( ( wc & WEAPON_CLASS_ENERGY ) || ( wc & WEAPON_CLASS_FLAMER ) ) {
			return;
		}
		else {
			return;
		}

		break;
	}

	if ( sfx ) {
		trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, sfx );
	}

	if ( sfx2 ) {
		trap_S_StartLocalSound( sfx2, CHAN_AUTO );
	}

	if ( shakeAmt ) {
		CG_StartShakeCamera( shakeAmt, shakeDur, origin, shakeRad );
	}

	if ( mod ) {
		le = CG_MakeExplosion( origin, dir, mod, shader, duration, isSprite );
		le->light = light;
		le->lightOverdraw = lightOverdraw;
		VectorCopy( lightColor, le->lightColor );
	}

	if ( mark ) {
		CG_ImpactMark( mark, origin, dir, random() * 360, 1, 1, 1, 1, alphaFade, radius, qfalse, -1 );
	}
}

/*
==============
CG_MissileHitWallSmall
==============
*/
void CG_MissileHitWallSmall( int weapon, int clientNum, vec3_t origin, vec3_t dir ) {
	qhandle_t mod;
	qhandle_t mark;
	qhandle_t shader;
	sfxHandle_t sfx;
	float radius;
	float light;
	vec3_t lightColor;
	localEntity_t   *le;
//	int				r;
	qboolean alphaFade;
	qboolean isSprite;
	int duration;
	// Ridah
	int lightOverdraw;
	vec3_t sprOrg;
	vec3_t sprVel;
//	int				i,j;

	mod = 0;
	lightColor[0] = 1;
	lightColor[1] = 1;
	lightColor[2] = 0;
	// Ridah
	lightOverdraw = 0;

	// set defaults
	shader = cgs.media.rocketExplosionShader;       // copied from RL
	sfx = cgs.media.sfx_rockexp;
	mark = cgs.media.burnMarkShader;
	radius = 64;
	light = 300;
	isSprite = qtrue;
	duration = 1000;
	lightColor[0] = 0.75;
	lightColor[1] = 0.5;
	lightColor[2] = 0.1;

	// Ridah, explosion sprite animation
	VectorMA( origin, 16, dir, sprOrg );
	VectorScale( dir, 64, sprVel );

	CG_ParticleExplosion( "explode1", sprOrg, sprVel, 600, 6, 50 );

	// Ridah, throw some debris
	CG_AddDebris( origin, dir,
				  280,          // speed
				  1400,         // duration
				  // 15 + rand()%5 );	// count
				  7 + rand() % 2 );     // count

	if ( sfx ) {
		trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, sfx );
	}

	//
	// create the explosion
	//
	if ( mod ) {
		le = CG_MakeExplosion( origin, dir,
							   mod, shader,
							   duration, isSprite );
		le->light = light;
		// Ridah
		le->lightOverdraw = lightOverdraw;
		VectorCopy( lightColor, le->lightColor );
	}

	//
	// impact mark
	//
	alphaFade = ( mark == cgs.media.energyMarkShader );   // plasma fades alpha, all others fade color
	// CG_ImpactMark( mark, origin, dir, random()*360, 1,1,1,1, alphaFade, radius, qfalse, 60000 );
	CG_ImpactMark( mark, origin, dir, random() * 360, 1,1,1,1, alphaFade, radius, qfalse, INT_MAX );

	CG_StartShakeCamera( 0.05, 300, origin, 300 );
}

/*
=================
CG_MissileHitPlayer
=================
*/
void CG_MissileHitPlayer( centity_t *cent, int weapon, vec3_t origin, vec3_t dir, int entityNum ) {
	int i;

	CG_Bleed( origin, entityNum );

	// some weapons will make an explosion with the blood, while
	// others will just make the blood
	switch ( weapon ) {
		// knives just make the flesh hit sound.  no other effects
	case WP_KNIFE:
		i = rand() % 4;
		if ( cgs.media.sfx_knifehit[i] ) {
			trap_S_StartSound( origin, cent->currentState.number, CHAN_WEAPON, cgs.media.sfx_knifehit[i] );
		}

		if ( cent->currentState.number == cg.snap->ps.clientNum ) {
			CG_StartShakeCamera( 0.03, 500, origin, 100 );
		}
		break;

	case WP_GRENADE_LAUNCHER:
	case WP_PANZERFAUST:
		// this shake is /on top/ of the shake from the impact (done in CG_MissileHitWall)
		CG_StartShakeCamera( 0.1, 500, origin, 100 );
		CG_MissileHitWall( weapon, 0, origin, dir, 0 );     //	(SA) modified to send missilehitwall surface parameters
		break;
	default:
		break;
	}
}



/*
============================================================================

VENOM GUN TRACING

============================================================================
*/

/*
================
CG_VenomPellet
================
*/
static void CG_VenomPellet( vec3_t start, vec3_t end, int skipNum ) {
}


//----(SA)	all changes to venom below should be mine
#define DEFAULT_VENOM_COUNT 10

/*
================
CG_VenomPattern

Perform the same traces the server did to locate the
hit splashes (FIXME: random seed isn't synced anymore)

  (SA)	right now this is random like a shotgun.  I want to make it more
		organized so that the pattern is more of a circle (with some degree of randomness)
================
*/
static void CG_VenomPattern( vec3_t origin, vec3_t origin2, int otherEntNum ) {
	int i;
	float r, u;
	vec3_t end;
	vec3_t forward, right, up;

	// derive the right and up vectors from the forward vector, because
	// the client won't have any other information
	VectorNormalize2( origin2, forward );
	PerpendicularVector( right, forward );
	CrossProduct( forward, right, up );

	// generate the "random" spread pattern
	for ( i = 0 ; i < DEFAULT_VENOM_COUNT ; i++ ) {
		r = crandom() * GetWeaponTableData(WP_VENOM)->weaponSpread;
		u = crandom() * GetWeaponTableData(WP_VENOM)->weaponSpread;
		VectorMA( origin, 8192, forward, end );
		VectorMA( end, r, right, end );
		VectorMA( end, u, up, end );

		CG_VenomPellet( origin, end, otherEntNum );
	}
}

/*
==============
CG_VenomFire
==============
*/
void CG_VenomFire( entityState_t *es, qboolean fullmode ) {
	vec3_t v;
	int contents;

	VectorSubtract( es->origin2, es->pos.trBase, v );
	VectorNormalize( v );
	VectorScale( v, 32, v );
	VectorAdd( es->pos.trBase, v, v );
	if ( cgs.glconfig.hardwareType != GLHW_RAGEPRO ) {
		// ragepro can't alpha fade, so don't even bother with smoke
		vec3_t up;

		contents = CG_PointContents( es->pos.trBase, 0 );
		if ( !( contents & CONTENTS_WATER ) ) {
			VectorSet( up, 0, 0, 32 );
			if ( fullmode ) {
				CG_SmokePuff( v, up, 24, 1, 1, 1, 0.33, 1200, cg.time, 0, 0, cgs.media.shotgunSmokePuffShader );    // LEF_PUFF_DONT_SCALE
			}
//----(SA)	for the time being don't do the single shot smoke as it's position is funky
//			else
//				CG_SmokePuff( v, up, 4, 1, 1, 1, 0.33, 700, cg.time, 0, cgs.media.shotgunSmokePuffShader );
		}
	}
	if ( fullmode ) {
		CG_VenomPattern( es->pos.trBase, es->origin2, es->otherEntityNum );
	}
}

/*
============================================================================

BULLETS

============================================================================
*/

/*
===============
CG_SpawnTracer
===============
*/
void CG_SpawnTracer( int sourceEnt, vec3_t pstart, vec3_t pend ) {
	localEntity_t   *le;
	float dist;
	vec3_t dir, ofs;
	orientation_t or;
	vec3_t start, end;

	VectorCopy( pstart, start );
	VectorCopy( pend, end );

	VectorSubtract( end, start, dir );
	dist = VectorNormalize( dir );

	if ( dist < 2.0 * cg_tracerLength.value ) {
		return; // segment isnt long enough, dont bother

	}
	if ( sourceEnt < cgs.maxclients ) {
		// for visual purposes, find the actual tag_weapon for this client
		// and offset the start and end accordingly
		if ( cg_entities[sourceEnt].currentState.eFlags & EF_MG42_ACTIVE ) {   // mounted
			start[2] -= 32; // (SA) hack to get the tracer down below the barrel FIXME: do properly
		} else {
			if ( CG_GetWeaponTag( sourceEnt, "tag_flash", &or ) ) {
				VectorSubtract( or.origin, start, ofs );
				if ( VectorLength( ofs ) < 64 ) {
					VectorAdd( start, ofs, start );
				}
			}
		}
	}

	// subtract the length of the tracer from the end point, so we dont go through the end point
	VectorMA( end, -cg_tracerLength.value, dir, end );
	dist = VectorDistance( start, end );

	le = CG_AllocLocalEntity();
	le->leType = LE_MOVING_TRACER;
	le->startTime = cg.time - ( cg.frametime ? ( rand() % cg.frametime ) / 2 : 0 );
	le->endTime = le->startTime + 1000.0 * dist / cg_tracerSpeed.value;

	le->pos.trType = TR_LINEAR;
	le->pos.trTime = le->startTime;
	VectorCopy( start, le->pos.trBase );
	VectorScale( dir, cg_tracerSpeed.value, le->pos.trDelta );
}

/*
===============
CG_DrawTracer
===============
*/
void CG_DrawTracer( vec3_t start, vec3_t finish ) {
	vec3_t forward, right;
	polyVert_t verts[4];
	vec3_t line;

	VectorSubtract( finish, start, forward );

	line[0] = DotProduct( forward, cg.refdef.viewaxis[1] );
	line[1] = DotProduct( forward, cg.refdef.viewaxis[2] );

	VectorScale( cg.refdef.viewaxis[1], line[1], right );
	VectorMA( right, -line[0], cg.refdef.viewaxis[2], right );
	VectorNormalize( right );

	VectorMA( finish, cg_tracerWidth.value, right, verts[0].xyz );
	verts[0].st[0] = 1;
	verts[0].st[1] = 1;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = 255;

	VectorMA( finish, -cg_tracerWidth.value, right, verts[1].xyz );
	verts[1].st[0] = 1;
	verts[1].st[1] = 0;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = 255;

	VectorMA( start, -cg_tracerWidth.value, right, verts[2].xyz );
	verts[2].st[0] = 0;
	verts[2].st[1] = 0;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 255;

	VectorMA( start, cg_tracerWidth.value, right, verts[3].xyz );
	verts[3].st[0] = 0;
	verts[3].st[1] = 1;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = 255;

	trap_R_AddPolyToScene( cgs.media.tracerShader, 4, verts );
}

/*
===============
CG_Tracer
===============
*/
void CG_Tracer( vec3_t source, vec3_t dest, int sparks ) {
	float len, begin, end;
	vec3_t start, finish;
	vec3_t forward;

	// tracer
	VectorSubtract( dest, source, forward );
	len = VectorNormalize( forward );

	// start at least a little ways from the muzzle
	if ( len < 100 && !sparks ) {
		return;
	}
	begin = 50 + random() * ( len - 60 );
	end = begin + cg_tracerLength.value;
	if ( end > len ) {
		end = len;
	}
	VectorMA( source, begin, forward, start );
	VectorMA( source, end, forward, finish );

	CG_DrawTracer( start, finish );
}


/*
======================
CG_CalcMuzzlePoint
======================
*/
static qboolean CG_CalcMuzzlePoint( int entityNum, vec3_t muzzle ) {
	vec3_t forward, right, up;
	centity_t   *cent;

	if ( entityNum == cg.snap->ps.clientNum ) {
		VectorCopy( cg.snap->ps.origin, muzzle );
		muzzle[2] += cg.snap->ps.viewheight;
		AngleVectors( cg.snap->ps.viewangles, forward, NULL, NULL );
		VectorMA( muzzle, 14, forward, muzzle );
		return qtrue;
	}

	cent = &cg_entities[entityNum];

	VectorCopy( cent->currentState.pos.trBase, muzzle );

	AngleVectors( cent->currentState.apos.trBase, forward, right, up );
	muzzle[2] += DEFAULT_VIEWHEIGHT;

	VectorMA( muzzle, 14, forward, muzzle );

	return qtrue;
}

/*
======================
CG_Bullet

Renders bullet effects.
======================
*/
void CG_Bullet( vec3_t end, int sourceEntityNum, vec3_t normal, qboolean flesh, int fleshEntityNum, qboolean wolfkick, int otherEntNum2 ) {
	trace_t trace;
	int sourceContentType = 0, destContentType = 0; // TTimo: init
	vec3_t dir;
	vec3_t start, trend, tmp;      // JPW
	static int lastBloodSpat;

	// if the shooter is currently valid, calc a source point and possibly
	// do trail effects
	if ( sourceEntityNum >= 0 && cg_tracerChance.value > 0 ) {
		if ( CG_CalcMuzzlePoint( sourceEntityNum, start ) ) {
			sourceContentType = CG_PointContents( start, 0 );
			destContentType = CG_PointContents( end, 0 );

			// do a complete bubble trail if necessary
			if ( ( sourceContentType == destContentType ) && ( sourceContentType & CONTENTS_WATER ) ) {
				CG_BubbleTrail( start, end, .5, 8 );
			}
			// bubble trail from water into air
			else if ( ( sourceContentType & CONTENTS_WATER ) ) {
				trap_CM_BoxTrace( &trace, end, start, NULL, NULL, 0, CONTENTS_WATER );
				CG_BubbleTrail( start, trace.endpos, .5, 8 );
			}
			// bubble trail from air into water
			else if ( ( destContentType & CONTENTS_WATER ) ) {
				// only add bubbles if effect is close to viewer
				if ( Distance( cg.snap->ps.origin, end ) < 1024 ) {
					trap_CM_BoxTrace( &trace, start, end, NULL, NULL, 0, CONTENTS_WATER );
					CG_BubbleTrail( end, trace.endpos, .5, 8 );
				}
			}

			// if not flesh, then do a moving tracer
			if ( flesh ) {
				// draw a tracer
				if ( !wolfkick && random() < cg_tracerChance.value ) {
					CG_Tracer( start, end, 0 );
				}
			} else {    // (not flesh)
				if ( otherEntNum2 >= 0 && otherEntNum2 != ENTITYNUM_NONE ) {
					CG_SpawnTracer( otherEntNum2, start, end );
				} else {
					CG_SpawnTracer( sourceEntityNum, start, end );
				}
			}
		}
	}

	// impact splash and mark
	if ( flesh ) {
		vec3_t origin;
		int aiType;

		aiType = cg_entities[fleshEntityNum].currentState.aiChar;

		if ( fleshEntityNum < MAX_CLIENTS ) {
			CG_Bleed( end, fleshEntityNum );
		}

		// play the bullet hit flesh sound
		// HACK, if this is not us getting hit, make it quieter
		if ( fleshEntityNum == cg.snap->ps.clientNum ) {

			// (SA) TODO: for metal guys, make metal a flag rather than an aitype check?
			if ( aiType == AICHAR_PROTOSOLDIER ||
				 aiType == AICHAR_SUPERSOLDIER ) {
				CG_SoundPlayIndexedScript( cgs.media.bulletHitFleshMetalScript, NULL, fleshEntityNum );
			} else {
				CG_SoundPlayIndexedScript( cgs.media.bulletHitFleshScript, NULL, fleshEntityNum );
			}
		} else {
			VectorSubtract( cg_entities[fleshEntityNum].lerpOrigin, cg.snap->ps.origin, origin );
			VectorMA( cg.snap->ps.origin, 3, origin, origin );
			if ( aiType == AICHAR_PROTOSOLDIER ||
				 aiType == AICHAR_SUPERSOLDIER ) {
				CG_SoundPlayIndexedScript( cgs.media.bulletHitFleshMetalScript, origin, ENTITYNUM_WORLD );
			} else {
				CG_SoundPlayIndexedScript( cgs.media.bulletHitFleshScript, origin, ENTITYNUM_WORLD );
			}
		}

		// if we haven't dropped a blood spat in a while, check if this is a good scenario
		if ( lastBloodSpat > cg.time || lastBloodSpat < cg.time - 500 ) {
			if ( CG_CalcMuzzlePoint( sourceEntityNum, start ) ) {
				VectorSubtract( end, start, dir );
				VectorNormalize( dir );
				VectorMA( end, 128, dir, trend );
				trap_CM_BoxTrace( &trace, end, trend, NULL, NULL, 0, MASK_SHOT & ~CONTENTS_BODY );

				if ( trace.fraction < 1 ) {
					CG_ImpactMark( cgs.media.bloodDotShaders[rand() % 5], trace.endpos, trace.plane.normal, random() * 360,
								   1,1,1,1, qtrue, 15 + random() * 20, qfalse, cg_bloodTime.integer * 1000 );
					lastBloodSpat = cg.time;
				} else if ( lastBloodSpat < cg.time - 1000 ) {
					// drop one on the ground?
					VectorCopy( end, trend );
					trend[2] -= 64;
					trap_CM_BoxTrace( &trace, end, trend, NULL, NULL, 0, MASK_SHOT & ~CONTENTS_BODY );

					if ( trace.fraction < 1 ) {
						CG_ImpactMark( cgs.media.bloodDotShaders[rand() % 5], trace.endpos, trace.plane.normal, random() * 360,
									   1,1,1,1, qtrue, 15 + random() * 10, qfalse, cg_bloodTime.integer * 1000 );
						lastBloodSpat = cg.time;
					}
				}
			}
		}

	} else {    // (not flesh)
		int fromweap;
		fromweap = cg_entities[sourceEntityNum].currentState.weapon;

		if ( !fromweap || cg_entities[sourceEntityNum].currentState.eFlags & EF_MG42_ACTIVE ) { // mounted
			fromweap = WP_MP40;
		}

		// TODO: not sure what kind of effect were going to do
		if ( wolfkick ) {
			return;
		}

		if ( CG_CalcMuzzlePoint( sourceEntityNum, start )
			 || cg.snap->ps.persistant[PERS_HWEAPON_USE] ) {
			vec3_t start2;
			VectorSubtract( end, start, dir );
			VectorNormalize( dir );
			VectorMA( end, -4, dir, start2 );   // back off a little so it doesn't start in solid
			VectorMA( end, 64, dir, dir );
			trap_CM_BoxTrace( &trace, start2, dir, NULL, NULL, 0, MASK_SHOT );

			if ( ( trace.surfaceFlags & SURF_METAL ) || !( rand() % 10 ) || ( otherEntNum2 != ENTITYNUM_NONE ) ) {
				// JPW NERVE compute new spark direction from normal & dir (rotate -dir 180 degrees about normal)

				// (SA) NOTE: isn't this done by the server and sent along in the 'normal' that's passed into this routine? (1107 g_weapon.c)

				VectorScale( dir,-1.0f,tmp );
				RotatePointAroundVector( tmp,normal,tmp,180.0f );
				CG_MissileHitWall( fromweap, 0, end, tmp, trace.surfaceFlags ); // sparks	//	(SA) modified to send missilehitwall surface parameters
// jpw
//				CG_MissileHitWall( fromweap, 0, end, normal, trace.surfaceFlags );	// sparks	//	(SA) modified to send missilehitwall surface parameters
			}
			if ( !( sourceContentType & CONTENTS_WATER ) && ( destContentType & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) ) {   // only when shooting /into/ water
				trap_CM_BoxTrace( &trace, start, end, NULL, NULL, 0, MASK_WATER );
//				CG_Trace(&trace, start, NULL, NULL, end, -1, MASK_WATER);
//				if (!(trace.surfaceFlags & SURF_NOMARKS)) {	// check to see if the surface should draw splashes
				CG_MissileHitWall( fromweap, 2, trace.endpos, trace.plane.normal, trace.surfaceFlags );
//				}
			} else {
				CG_MissileHitWall( fromweap, 1, end, normal, trace.surfaceFlags );   // smoke puff	//	(SA) modified to send missilehitwall surface parameters

				if ( 0 ) {
					localEntity_t   *le;
					le = CG_AllocLocalEntity();
					le->leType = LE_EMITTER;
					le->startTime = cg.time;
					le->endTime = le->startTime + 20000;
					le->pos.trType = TR_STATIONARY;
					VectorCopy( end, le->pos.trBase );
					VectorCopy( normal, le->angles.trBase );
					le->ownerNum = 0;
				}
			}
		}
	}
}

/*
============
CG_ClientDamage
============
*/
void CG_ClientDamage( int entnum, int enemynum, int id ) {
	if ( id > CLDMG_MAX ) {
		CG_Error( "CG_ClientDamage: unknown damage type: %i\n", id );
	}

	trap_SendClientCommand( va( "cld %i %i %i", entnum, enemynum, id ) );
}
