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
// holster). One row per action in actionViewDefs[] below: pliers for
// ACTION_CONSTRUCT, adrenaline for ACTION_BUYPERK. Render is CG_AddViewActionWeapon
// in cg_weapons.c; the third-person model swap is CG_AddPlayerWeapon.

#include "cg_local.h"

cgActionView_t cgActionView;

#define ACTION_RAISE_DELAY_MS   250   // floor for the raise delay when the real weapon's WEAP_DROP length is unknown

typedef struct {
	const char *fp;
	const char *hands;
	const char *tp;
	const char *cfg;
	const char *torsoClip;   // 3rd-person torso clip, resolved against the animgroup
	int raiseAnim;           // weapon.cfg rows to play for each phase (WEAP_*)
	int activeAnim;
	int dropAnim;
} actionViewDef_t;

// indexed by actionType_t; ACTION_NONE / ACTION_LOWERING are left blank
static const actionViewDef_t actionViewDefs[NUM_ACTION_TYPES] = {
	[ACTION_CONSTRUCT] = {
		"models/weapons/misc/pliers/v_pliers.md3",
		"models/weapons/misc/pliers/v_pliers_hand.md3",
		"models/weapons/misc/pliers/pliers.md3",
		"models/weapons/misc/pliers/weapon.cfg",
		"firing_pliers",
		WEAP_RAISE, WEAP_ATTACK1, WEAP_DROP
	},
	[ACTION_BUYPERK] = {
		"models/weapons/misc/adrenaline/v_adrenaline.md3",
		"models/weapons/misc/adrenaline/v_adrenaline_hand.md3",
		"models/weapons/misc/adrenaline/adrenaline.md3",
		"models/weapons/misc/adrenaline/weapon.cfg",
		"self_inject",
		// adrenaline model puts its "adrenaline" (auto-injector) motion in the ALTSWITCH/ATTACK2 rows
		WEAP_ALTSWITCHTO, WEAP_ATTACK2, WEAP_ALTSWITCHFROM
	},
};

/*
==============
CG_ActionView_Register

Called from CG_RegisterGraphics on every map load (media handles don't persist).
A row that fails to register just stays invalid -- its action then no-ops and the
weapon holsters with nothing shown, same as before the feature existed.
==============
*/
void CG_ActionView_Register( void ) {
	int a;
	cgActionView_t *av = &cgActionView;

	memset( av, 0, sizeof( *av ) );

	for ( a = 0; a < NUM_ACTION_TYPES; a++ ) {
		const actionViewDef_t *def = &actionViewDefs[a];
		actionViewModel_t *m = &av->models[a];

		m->torsoAnim = -2;
		if ( !def->fp ) {
			continue;
		}

		m->fpModel    = trap_R_RegisterModel( def->fp );
		m->handsModel = trap_R_RegisterModel( def->hands );
		m->tpModel    = trap_R_RegisterModel( def->tp );

		if ( !m->fpModel || !m->handsModel ) {
			CG_Printf( S_COLOR_YELLOW "WARNING: action viewmodel %i disabled (missing model near %s)\n", a, def->fp );
			continue;
		}

		// a bad weapon.cfg only costs the raise/use/drop animation - the model still shows (static)
		if ( !CG_ParseWeaponConfig( def->cfg, &m->weap, WP_KNIFE ) ) {
			CG_Printf( S_COLOR_YELLOW "WARNING: action viewmodel %i has no usable %s (needs the full anim list)\n", a, def->cfg );
		}

		m->valid = qtrue;
		av->anyValid = qtrue;
	}
}

/*
==============
CG_ActionView_EntityTpModel

Third-person model to draw in a player's hands right now, or 0. Keyed on which
action torso clip their (networked) torsoAnim is running. Clip indices resolve
once against a ready client and are cached (all human models share the animgroup).
==============
*/
qhandle_t CG_ActionView_EntityTpModel( centity_t *cent ) {
	int clientNum, torso, a;

	if ( !cgActionView.anyValid || cent->currentState.eType != ET_PLAYER ) {
		return 0;
	}
	clientNum = cent->currentState.clientNum;
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS || cgs.clientinfo[clientNum].isSkeletal ) {
		return 0;
	}
	torso = cent->currentState.torsoAnim & ~ANIM_TOGGLEBIT;

	for ( a = 0; a < NUM_ACTION_TYPES; a++ ) {
		actionViewModel_t *m = &cgActionView.models[a];

		if ( !m->valid || !m->tpModel ) {
			continue;
		}
		if ( m->torsoAnim == -2 && BG_ValidAnimScript( clientNum ) ) {
			m->torsoAnim = BG_AnimationIndexForStringSafe( actionViewDefs[a].torsoClip, clientNum );
		}
		if ( m->torsoAnim >= 0 && torso == m->torsoAnim ) {
			return m->tpModel;
		}
	}
	return 0;
}

/*
==============
CG_ActionView_Active

True once the viewmodel is actually on screen. AV_RAISE_DELAY is deliberately not
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
	animation_t *anim = &cgActionView.models[cgActionView.current].weap.weapAnimations[ cgActionView.animNumber & ~ANIM_TOGGLEBIT ];
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
	CG_ClearWeapLerpFrame( &cgActionView.models[cgActionView.current].weap, &cgActionView.lf, weapAnim );
}

/*
==============
CG_ActionView_HolsterMs

How long to sit in AV_RAISE_DELAY: the real weapon's own WEAP_DROP length (that's
the anim bg_pmove.c plays when it holsters), so the whole drop is seen before the
viewmodel comes up. Falls back to the floor if that weapon isn't registered yet.
==============
*/
static int CG_ActionView_HolsterMs( int weapon ) {
	animation_t *drop;

	if ( weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS ) {
		return ACTION_RAISE_DELAY_MS;
	}
	drop = &cg_weapons[weapon].weapAnimations[WEAP_DROP];
	if ( drop->numFrames <= 0 || drop->frameLerp <= 0 ) {
		return ACTION_RAISE_DELAY_MS;
	}
	return drop->numFrames * drop->frameLerp;
}

/*
==============
CG_ActionView_Update

Edge-driven off ps.stats[STAT_ACTIVE_ACTION]. Phases:
  OFF -> RAISE_DELAY -> RAISING -> ACTIVE     (while the action is running)
  ACTIVE/RAISING -> LOWERING -> OFF           (once it enters ACTION_LOWERING / NONE)
av->current pins which model row drives everything until it lowers back off, so
the server can flip to the shared ACTION_LOWERING value without ambiguity.
==============
*/
void CG_ActionView_Update( playerState_t *ps ) {
	cgActionView_t *av = &cgActionView;
	const actionViewDef_t *def;
	int action;

	if ( !av->anyValid || !cg_actionViewModel.integer ) {
		av->phase = AV_OFF;
		av->current = ACTION_NONE;
		return;
	}

	action = ps->stats[STAT_ACTIVE_ACTION];
	def = &actionViewDefs[ av->current ];   // ACTION_NONE row is zeroed, harmless before current is set

	switch ( av->phase ) {
	case AV_OFF:
		if ( ( action == ACTION_CONSTRUCT || action == ACTION_BUYPERK ) && av->models[action].valid ) {
			av->current = action;
			av->phase = AV_RAISE_DELAY;
			av->phaseStartTime = cg.time;
		}
		break;

	case AV_RAISE_DELAY:
		if ( action != av->current ) {
			av->phase = AV_OFF;   // aborted before the viewmodel was ever shown
			av->current = ACTION_NONE;
		} else if ( ps->weaponstate != WEAPON_HOLSTER_IN ) {
			av->phaseStartTime = cg.time;   // bg_pmove hasn't started the drop yet - hold the clock at zero
		} else if ( cg.time - av->phaseStartTime >= CG_ActionView_HolsterMs( ps->weapon ) ) {
			CG_ActionView_SetAnim( def->raiseAnim );
			av->phase = AV_RAISING;
		}
		break;

	case AV_RAISING:
		if ( action != av->current ) {
			CG_ActionView_SetAnim( def->dropAnim );
			av->phase = AV_LOWERING;
		} else if ( CG_ActionView_AnimDone() ) {
			CG_ActionView_SetAnim( def->activeAnim );
			av->phase = AV_ACTIVE;
		}
		break;

	case AV_ACTIVE:
		if ( action != av->current ) {
			CG_ActionView_SetAnim( def->dropAnim );
			av->phase = AV_LOWERING;
		}
		break;

	case AV_LOWERING:
		if ( action == av->current ) {
			CG_ActionView_SetAnim( def->raiseAnim );
			av->phase = AV_RAISING;
		} else if ( CG_ActionView_AnimDone() ) {
			av->phase = AV_OFF;
			av->current = ACTION_NONE;
		}
		break;
	}

	// advance the viewmodel animation only once it's on screen
	if ( CG_ActionView_Active() ) {
		CG_RunWeapLerpFrame( NULL, &av->models[av->current].weap, &av->lf, av->animNumber, 1.0f );
	}
}
