/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).

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

// cg_actionview.c -- cosmetic first-person viewmodel shown in place of the real
// weapon while the server holds ps.stats[STAT_ACTIVE_ACTION] (bg_pmove.c does the
// holster). Constructibles drive it via the pliers; revive/buy-perk are meant to
// add rows to the model table below. See CG_AddViewActionWeapon in cg_weapons.c
// for the render and CG_AddPlayerWeapon for the third-person model swap.

#include "cg_local.h"

cgActionView_t cgActionView;

#define AV_PLIERS_FP    "models/weapons/misc/pliers/v_pliers.md3"
#define AV_PLIERS_HANDS "models/weapons/misc/pliers/v_pliers_hand.md3"
#define AV_PLIERS_TP    "models/weapons/misc/pliers/pliers.md3"
#define AV_PLIERS_CFG   "models/weapons/misc/pliers/weapon.cfg"

#define PLIERS_RAISE_DELAY_MS   250   // wait for the real weapon's WEAP_DROP to read before showing the pliers

/*
==============
CG_ActionView_Register

Called from CG_RegisterGraphics on every map load (media handles don't persist).
Leaves cgActionView.valid clear if the assets are missing -- the feature then
no-ops and the weapon just stays holstered, same as before it existed.
==============
*/
void CG_ActionView_Register( void ) {
	cgActionView_t *av = &cgActionView;

	memset( av, 0, sizeof( *av ) );
	av->firingPliersAnim = -2;

	av->fpModel    = trap_R_RegisterModel( AV_PLIERS_FP );
	av->handsModel = trap_R_RegisterModel( AV_PLIERS_HANDS );
	av->tpModel    = trap_R_RegisterModel( AV_PLIERS_TP );

	if ( !CG_ParseWeaponConfig( AV_PLIERS_CFG, &av->weap, WP_KNIFE ) ) {
		CG_Printf( S_COLOR_YELLOW "WARNING: action viewmodel disabled (missing %s)\n", AV_PLIERS_CFG );
		return;
	}
	if ( !av->fpModel || !av->handsModel ) {
		CG_Printf( S_COLOR_YELLOW "WARNING: action viewmodel disabled (missing pliers models)\n" );
		return;
	}

	av->valid = qtrue;
}

/*
==============
CG_EntityIsBuilding

True while a (possibly remote) player's torso is running firing_pliers -- the
signal CG_AddPlayerWeapon uses to swap their held model for the pliers. The clip
index is resolved once and cached (-1 = clip not authored, swap stays disabled).
==============
*/
qboolean CG_EntityIsBuilding( centity_t *cent ) {
	int clientNum;

	if ( cent->currentState.eType != ET_PLAYER ) {
		return qfalse;
	}
	clientNum = cent->currentState.clientNum;
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		return qfalse;
	}
	if ( cgs.clientinfo[clientNum].isSkeletal ) {
		return qfalse;   // monsters don't build; their animgroup indices differ from the human set
	}

	if ( cgActionView.firingPliersAnim == -2 && BG_ValidAnimScript( clientNum ) ) {
		// resolve once against a ready client - all human models share the animgroup
		cgActionView.firingPliersAnim = BG_AnimationIndexForStringSafe( "firing_pliers", clientNum );
	}
	if ( cgActionView.firingPliersAnim < 0 ) {
		return qfalse;
	}
	return ( cent->currentState.torsoAnim & ~ANIM_TOGGLEBIT ) == cgActionView.firingPliersAnim;
}

/*
==============
CG_ActionView_Active

True once the pliers is actually on screen. AV_RAISE_DELAY is deliberately not
included so the real weapon stays visible playing its WEAP_DROP first.
==============
*/
qboolean CG_ActionView_Active( void ) {
	actionViewPhase_t p = cgActionView.phase;

	return (qboolean)( p == AV_RAISING || p == AV_ACTIVE || p == AV_LOWERING );
}

/*
==============
CG_ActionView_AnimDone

Whether the current (non-looping) sequence has run its length. Time-based so it
doesn't depend on lerpframe clamp internals.
==============
*/
static qboolean CG_ActionView_AnimDone( void ) {
	animation_t *anim = &cgActionView.weap.weapAnimations[ cgActionView.animNumber & ~ANIM_TOGGLEBIT ];
	int duration = anim->numFrames * anim->frameLerp;

	return (qboolean)( cg.time - cgActionView.phaseStartTime >= duration );
}

/*
==============
CG_ActionView_SetAnim
==============
*/
static void CG_ActionView_SetAnim( int weapAnim ) {
	cgActionView.animNumber = weapAnim;
	cgActionView.phaseStartTime = cg.time;
	CG_ClearWeapLerpFrame( &cgActionView.weap, &cgActionView.lf, weapAnim );
}

/*
==============
CG_ActionView_Update

Edge-driven off ps.stats[STAT_ACTIVE_ACTION]. Phases:
  OFF -> RAISE_DELAY -> RAISING -> ACTIVE   (while ACTION_CONSTRUCT)
  ACTIVE/RAISING -> LOWERING -> OFF         (once it drops to _LOWER / NONE)
The server keeps ACTION_CONSTRUCT_LOWER set for PLIERS_LOWER_MS so bg_pmove.c
holds the real weapon down until LOWERING finishes here.
==============
*/
void CG_ActionView_Update( playerState_t *ps ) {
	cgActionView_t *av = &cgActionView;
	int action;

	if ( !av->valid || !cg_actionViewModel.integer ) {
		av->phase = AV_OFF;
		return;
	}

	action = ps->stats[STAT_ACTIVE_ACTION];

	switch ( av->phase ) {
	case AV_OFF:
		if ( action == ACTION_CONSTRUCT ) {
			av->phase = AV_RAISE_DELAY;
			av->phaseStartTime = cg.time;
		}
		break;

	case AV_RAISE_DELAY:
		if ( action != ACTION_CONSTRUCT ) {
			av->phase = AV_OFF;   // aborted before the pliers was ever shown
		} else if ( cg.time - av->phaseStartTime >= PLIERS_RAISE_DELAY_MS ) {
			CG_ActionView_SetAnim( WEAP_RAISE );
			av->phase = AV_RAISING;
		}
		break;

	case AV_RAISING:
		if ( action != ACTION_CONSTRUCT ) {
			CG_ActionView_SetAnim( WEAP_DROP );
			av->phase = AV_LOWERING;
		} else if ( CG_ActionView_AnimDone() ) {
			CG_ActionView_SetAnim( WEAP_ATTACK1 );
			av->phase = AV_ACTIVE;
		}
		break;

	case AV_ACTIVE:
		if ( action != ACTION_CONSTRUCT ) {
			CG_ActionView_SetAnim( WEAP_DROP );
			av->phase = AV_LOWERING;
		}
		break;

	case AV_LOWERING:
		if ( action == ACTION_CONSTRUCT ) {
			CG_ActionView_SetAnim( WEAP_RAISE );
			av->phase = AV_RAISING;
		} else if ( CG_ActionView_AnimDone() ) {
			av->phase = AV_OFF;
		}
		break;
	}

	// advance the viewmodel animation only once an anim has been set (i.e. it's on screen)
	if ( CG_ActionView_Active() ) {
		CG_RunWeapLerpFrame( NULL, &av->weap, &av->lf, av->animNumber, 1.0f );
	}
}
