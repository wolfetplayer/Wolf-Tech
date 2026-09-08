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
 * name:		g_combat.c
 *
 * desc:
 *
*/

#include "g_local.h"
#include "g_coop.h"
#include "g_survival.h"

/*
============
AddScore

Adds score to both the client and his team
============
*/
void AddScore( gentity_t *ent, int score ) {
	if ( !ent->client ) {
		return;
	}
	// no scoring during pre-match warmup
	if ( level.warmupTime ) {
		return;
	}

	// Ridah, no scoring during single player
	// DHM - Nerve :: fix typo
	if ( g_gametype.integer <= GT_SINGLE_PLAYER ) {
		return;
	}
	// done.


	ent->client->ps.persistant[PERS_SCORE] += score;

	CalculateRanks();
}



extern qboolean G_ThrowChair( gentity_t *ent, vec3_t dir, qboolean force );


/*
=================
TossClientWeapons

Toss the weapon and powerups for the killed entity
=================
*/
void TossClientWeapons( gentity_t *self )
{
	gitem_t *item;
	vec3_t forward;
	int weapon;
	gentity_t *drop = 0;

	// drop the weapon if not a gauntlet or machinegun
	weapon = self->s.weapon;

	if (g_gametype.integer == GT_COOP_SURVIVAL)
	{
		return;
	}

		switch (self->aiCharacter)
		{
		case AICHAR_ZOMBIE:
		case AICHAR_ZOMBIE_GHOST:
		case AICHAR_WARZOMBIE:
		case AICHAR_LOPER:
		case AICHAR_LOPER_SPECIAL:
		case AICHAR_PRIEST:
		case AICHAR_XSHEPHERD:
			return;
		default:
			break;
		}
	

	AngleVectors(self->r.currentAngles, forward, NULL, NULL);

	G_ThrowChair(self, forward, qtrue); // drop chair if you're holding one  //----(SA)	added

	// make a special check to see if they are changing to a new
	// weapon that isn't the mg or gauntlet.  Without this, a client
	// can pick up a weapon, be killed, and not drop the weapon because
	// their weapon change hasn't completed yet and they are still holding the MG.

	// (SA) always drop what you were switching to
	if (1)
	{
		if (self->client->ps.weaponstate == WEAPON_DROPPING || self->client->ps.weaponstate == WEAPON_DROPPING_TORELOAD)
		{
			weapon = self->client->pers.cmd.weapon;
		}
		if (!(COM_BitCheck(self->client->ps.weapons, weapon)))
		{
			weapon = WP_NONE;
		}
	}

	//----(SA)	added
	if (weapon == WP_SNOOPERSCOPE)
	{
		weapon = WP_SNOOPER;
	}
	if (weapon == WP_FG42SCOPE)
	{
		weapon = WP_FG42;
	}
	if (weapon == WP_M7)
	{
		weapon = WP_M1GARAND;
	}

	if (weapon > WP_NONE && weapon < WP_DYNAMITE && self->client->ps.ammo[BG_FindAmmoForWeapon(weapon)])
	{
		// find the item type for this weapon
		item = BG_FindItemForWeapon(weapon);
		// spawn the item

		// Rafael
		if (!(self->client->ps.persistant[PERS_HWEAPON_USE]))
		{
			drop = Drop_Item(self, item, 0, qfalse);
		}
	}

	// dropped items stay forever in SP
	if (drop)
	{
		if (g_gametype.integer == GT_COOP_SURVIVAL)
		{
			drop->nextthink = level.time + 100000;
		}
		else
		{
			drop->nextthink = 0;
		}

	}
}


/*
==================
LookAtKiller
==================
*/
void LookAtKiller( gentity_t *self, gentity_t *inflictor, gentity_t *attacker ) {
	vec3_t dir;

	if ( attacker && attacker != self ) {
		VectorSubtract( attacker->s.pos.trBase, self->s.pos.trBase, dir );
	} else if ( inflictor && inflictor != self ) {
		VectorSubtract( inflictor->s.pos.trBase, self->s.pos.trBase, dir );
	} else {
		self->client->ps.stats[STAT_DEAD_YAW] = self->s.angles[YAW];
		return;
	}

	self->client->ps.stats[STAT_DEAD_YAW] = vectoyaw( dir );
}


/*
==============
GibHead
==============
*/
void GibHead( gentity_t *self, int killer ) {
	G_AddEvent( self, EV_GIB_HEAD, killer );
}

/*
==================
GibEntity
==================
*/
void GibEntity( gentity_t *self, int killer ) {
	gentity_t *other = &g_entities[killer];
	vec3_t dir;

	VectorClear( dir );
	if ( other->inuse ) {
		if ( other->client ) {
			VectorSubtract( self->r.currentOrigin, other->r.currentOrigin, dir );
			VectorNormalize( dir );
		} else if ( !VectorCompare( other->s.pos.trDelta, vec3_origin ) ) {
			VectorNormalize2( other->s.pos.trDelta, dir );
		}
	}

	G_AddEvent( self, EV_GIB_PLAYER, DirToByte( dir ) );
	self->takedamage = qfalse;
	self->s.eType = ET_INVISIBLE;
	self->r.contents = 0;
}

/*
==================
body_die
==================
*/
void body_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath ) {
	if ( self->health > GIB_HEALTH ) {
		return;
	}
	if ( !g_blood.integer ) {
		self->health = GIB_HEALTH + 1;
		return;
	}
	if ( self->aiCharacter == AICHAR_HEINRICH || self->aiCharacter == AICHAR_HELGA || self->aiCharacter == AICHAR_SUPERSOLDIER || self->aiCharacter == AICHAR_PROTOSOLDIER ) {
		if ( self->health <= GIB_HEALTH ) {
			self->health = -1;
			return;
		}
	}

	GibEntity( self, 0 );
}


// these are just for logging, the client prints its own messages
char    *modNames[] = {
	"MOD_UNKNOWN",
	"MOD_SHOTGUN",
	"MOD_AUTO5",
	"MOD_MONSTER_MELEE",
	"MOD_MACHINEGUN",
	"MOD_GRENADE",
	"MOD_GRENADE_SPLASH",
	"MOD_ROCKET",
	"MOD_ROCKET_SPLASH",
	"MOD_RAILGUN",
	"MOD_LIGHTNING",
	"MOD_BFG",
	"MOD_BFG_SPLASH",
	"MOD_KNIFE",
	"MOD_KNIFE2",
	"MOD_KNIFE_STEALTH",
	"MOD_LUGER",
	"MOD_COLT",
	"MOD_MP40",
	"MOD_MP34",
	"MOD_THOMPSON",
	"MOD_STEN",
	"MOD_PPSH",
	"MOD_MOSIN",
	"MOD_MAUSER",
	"MOD_SNIPERRIFLE",
	"MOD_SNOOPER",
	"MOD_SNOOPERSCOPE",
	"MOD_M1GARAND",
	"MOD_G43",
	"MOD_M7",
	"MOD_SILENCER", //----(SA)
	"MOD_AKIMBO",    //----(SA)
	"MOD_TT33",
	"MOD_DUAL_TT33",
	"MOD_REVOLVER",
	"MOD_BAR",   //----(SA)
	"MOD_FG42",
	"MOD_FG42SCOPE",
	"MOD_MP44",
	"MOD_PANZERFAUST",
	"MOD_ROCKET_LAUNCHER",
	"MOD_GRENADE_LAUNCHER",
	"MOD_VENOM",
	"MOD_VENOM_FULL",
	"MOD_FLAMETHROWER",
	"MOD_FLAMETRAP",
	"MOD_TESLA",
	"MOD_SPEARGUN",
	"MOD_SPEARGUN_CO2",
	"MOD_GRENADE_PINEAPPLE",
	"MOD_CROSS",
	"MOD_MORTAR",
	"MOD_MORTAR_SPLASH",
	"MOD_KICKED",
	"MOD_GRABBER",
	"MOD_DYNAMITE",
	"MOD_DYNAMITE_SPLASH",
	"MOD_AIRSTRIKE", // JPW NERVE
	"MOD_ARTY",
	"MOD_WATER",
	"MOD_SLIME",
	"MOD_LAVA",
	"MOD_CRUSH",
	"MOD_TELEFRAG",
	"MOD_FALLING",
	"MOD_SUICIDE",
	"MOD_TARGET_LASER",
	"MOD_TRIGGER_HURT",
	"MOD_GRAPPLE",
	"MOD_EXPLOSIVE",
	"MOD_POISONGAS",
	"MOD_ZOMBIESPIT",
	"MOD_ZOMBIESPIT_SPLASH",
	"MOD_ZOMBIESPIRIT",
	"MOD_ZOMBIESPIRIT_SPLASH",
	"MOD_LOPER_LEAP",
	"MOD_LOPER_GROUND",
	"MOD_LOPER_HIT",
// new MODs
#ifdef _ADMINS
	"MOD_SLAP",
	"MOD_ADMKILL",
#endif
	"MOD_THROWKNIFE",
// JPW NERVE
	"MOD_LT_ARTILLERY",
	"MOD_LT_AIRSTRIKE",
	"MOD_ENGINEER",  // not sure if we'll use
	"MOD_MEDIC",     // these like this or not
// jpw
	"MOD_BAT",

	"MOD_MG42M",
	"MOD_BROWNING",

	"MOD_M1941",
	"MOD_M1941SCOPE",
	"MOD_HDM",
	"MOD_DELISLE",
	"MOD_DELISLESCOPE"
};

/*
==================
player_die
==================
*/
void player_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath ) {
	gentity_t   *ent;
	int anim;
	int contents = 0;
	int killer;
	int i;
	char        *killerName, *obit;
	qboolean nogib = qtrue;

	if ( self->client->ps.pm_type == PM_DEAD ) {
		return;
	}

	if ( level.intermissiontime ) {
		return;
	}

//----(SA) commented out as we have no hook
//	if (self->client && self->client->hook)
//		Weapon_HookFree(self->client->hook);

	self->client->ps.pm_type = PM_DEAD;

	// death stats handled out-of-band of G_Damage for external calls
	Coop_AddStats( self, attacker, damage, meansOfDeath );

	if ( g_gametype.integer <= GT_COOP ) {
		trap_Cvar_Set( "g_attempts", va( "%i", g_attempts.integer + 1 ) );
	}

	if ( attacker ) {
		killer = attacker->s.number;
		if ( attacker->client ) {
			killerName = attacker->client->pers.netname;
		} else {
			killerName = "<non-client>";
		}
	} else {
		killer = ENTITYNUM_WORLD;
		killerName = "<world>";
	}

	if ( killer < 0 || killer >= MAX_CLIENTS ) {
		killer = ENTITYNUM_WORLD;
		killerName = "<world>";
	}

	if ( meansOfDeath < 0 || meansOfDeath >= ARRAY_LEN( modNames ) ) {
		obit = "<bad obituary>";
	} else {
		obit = modNames[ meansOfDeath ];
	}

	G_LogPrintf( "Kill: %i %i %i: %s killed %s by %s\n",
				 killer, self->s.number, meansOfDeath, killerName,
				 self->client->pers.netname, obit );

	// broadcast the death event to everyone
	ent = G_TempEntity( self->r.currentOrigin, EV_OBITUARY );
	ent->s.eventParm = meansOfDeath;
	ent->s.otherEntityNum = self->s.number;
	ent->s.otherEntityNum2 = killer;
	ent->r.svFlags = SVF_BROADCAST; // send to everyone

	self->enemy = attacker;

	self->client->ps.persistant[PERS_KILLED]++;

	if ( attacker && attacker->client ) {
		if ( attacker == self || OnSameTeam( self, attacker ) ) {
			AddScore( attacker, -1 );
		} else {
			if ( g_gametype.integer <= GT_COOP ) {
				AddScore( attacker, -2 );
			} else {
				AddScore( attacker, 1 );
			}

			// done.
			attacker->client->lastKillTime = level.time;
		}
	} else {
		AddScore( self, -1 );
	}

	// Add team bonuses
	Team_FragBonuses( self, inflictor, attacker );

	// if client is in a nodrop area, don't drop anything
// JPW NERVE new drop behavior
	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {   // only drop here in single player; in multiplayer, drop @ limbo
		contents = trap_PointContents( self->r.currentOrigin, -1 );
		if ( !( contents & CONTENTS_NODROP ) ) {
			TossClientWeapons( ent );
		}
	}

	// Forcefully throw the chair.
	G_ThrowChair( self, self->r.currentAngles, qtrue );

	Cmd_Score_f( self );        // show scores
	// send updated scores to any clients that are following this one,
	// or they would get stale scoreboards
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		gclient_t   *client;

		client = &level.clients[i];
		if ( client->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( client->sess.sessionTeam != TEAM_SPECTATOR ) {
			continue;
		}
		if ( client->sess.spectatorClient == self->s.number ) {
			Cmd_Score_f( g_entities + i );
		}
	}

	self->takedamage = qtrue;   // can still be gibbed

	self->s.powerups = 0;
// JPW NERVE -- only corpse in SP; in MP, need CONTENTS_BODY so medic can operate
	if ( g_gametype.integer <= GT_SINGLE_PLAYER ) {
		self->r.contents = CONTENTS_CORPSE;
		self->s.weapon = WP_NONE;
	}
// jpw
	self->s.angles[0] = 0;
	self->s.angles[2] = 0;
	LookAtKiller( self, inflictor, attacker );

	VectorCopy( self->s.angles, self->client->ps.viewangles );

	self->s.loopSound = 0;

	self->r.maxs[2] = -8;

	// don't allow respawn until the death anim is done
	// g_forcerespawn may force spawning at some later time
	self->client->respawnTime = level.time + 1700;

	// remove powerups
	memset( self->client->ps.powerups, 0, sizeof( self->client->ps.powerups ) );

	// don't print this in coop play, we just respawn
	// mission should fail if we both die
	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
		trap_SendServerCommand( -1, "mu_play sound/music/l_failed_1.wav 0\n" );
		trap_SetConfigstring( CS_MUSIC_QUEUE, "" );  // clear queue so it'll be quiet after hit
#ifdef LOCALISATION
		trap_SendServerCommand( -1, "cp \"Mission Failed\"" );
#else
		trap_SendServerCommand( -1, "cp missionfail0" );
#endif
	}


	// never gib in a nodrop
	contents = trap_PointContents( self->r.currentOrigin, -1 );

	if ( self->health <= GIB_HEALTH && !( contents & CONTENTS_NODROP ) && g_blood.integer ) {
//		if(self->client->ps.eFlags & EF_HEADSHOT)
//		{
//			GibHead(self, killer);
//		}
//		else	// gib death
//		{
		GibEntity( self, killer );
		nogib = qfalse;
//		}
	}

	if ( nogib ) {
		// normal death
		static int i;

		switch ( i ) {
		case 0:
			anim = BOTH_DEATH1;
			break;
		case 1:
			anim = BOTH_DEATH2;
			break;
		case 2:
		default:
			anim = BOTH_DEATH3;
			break;
		}

		// for the no-blood option, we need to prevent the health
		// from going to gib level
		if ( self->health <= GIB_HEALTH ) {
			self->health = GIB_HEALTH + 1;
		}

// JPW NERVE for medic
		self->client->medicHealAmt = 0;
// jpw

		self->client->ps.legsAnim =
			( ( self->client->ps.legsAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | anim;
		self->client->ps.torsoAnim =
			( ( self->client->ps.torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | anim;

		G_AddEvent( self, EV_DEATH1 + 1, killer );

		// the body can still be gibbed
		self->die = body_die;

		// globally cycle through the different death animations
		i = ( i + 1 ) % 3;
	}

	// We have a proper player died event.
	G_AddEvent( self, EV_PLAYER_DIED, killer );

	trap_LinkEntity( self );

	if ( g_gametype.integer <= GT_SINGLE_PLAYER ) {
		AICast_ScriptEvent( AICast_GetCastState( self->s.number ), "death", "" );
	}
}

/*
==================
G_EnterBleedout

GT_COOP_SURVIVAL only: instead of committing a kill immediately, put a real
player into a revivable bleed-out state for g_reviveBleedoutTime ms. Unlike
player_die(), this does NOT touch score, obituary, respawnTime or corpse
bookkeeping -- that only happens if bleed-out expires (G_TickReviveStates)
or the player takes a finishing hit while already down.
==================
*/
void G_EnterBleedout( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath ) {
	int killer;
	int anim;

	if ( attacker ) {
		killer = attacker->s.number;
	} else {
		killer = ENTITYNUM_WORLD;
	}
	if ( killer < 0 || killer >= MAX_CLIENTS ) {
		killer = ENTITYNUM_WORLD;
	}

	self->client->ps.pm_type = PM_DEAD;
	self->takedamage = qtrue;
	self->client->reviveTargetNum = -1;
	self->client->revivedByNum = -1;
	self->client->bleedoutAttackerNum = killer;
	self->client->bleedoutMOD = meansOfDeath;
	self->client->ps.stats[STAT_REVIVE_TIME] = g_reviveBleedoutTime.integer;

	self->s.angles[0] = 0;
	self->s.angles[2] = 0;
	LookAtKiller( self, inflictor, attacker );
	VectorCopy( self->s.angles, self->client->ps.viewangles );

	self->r.maxs[2] = -8;

	// play the falling-down death anim first; G_TickReviveStates settles into
	// wounded_idle_1 once it's finished playing (see bleedoutFallEndTime)
	switch ( rand() % 3 ) {
	case 0:
		anim = BOTH_DEATH1;
		break;
	case 1:
		anim = BOTH_DEATH2;
		break;
	default:
		anim = BOTH_DEATH3;
		break;
	}
	self->client->ps.legsAnim =
		( ( self->client->ps.legsAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | anim;
	self->client->ps.torsoAnim =
		( ( self->client->ps.torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | anim;
	self->client->bleedoutFallEndTime = level.time + 1700; // matches player_die()'s own death-anim-length assumption

	G_AddEvent( self, EV_DEATH1 + 1, killer );
	G_Sound( self, G_SoundIndex( ( rand() % 2 ) ? "sound/voice/medic_call_1.wav" : "sound/voice/medic_call_2.wav" ) );

	trap_SendServerCommand( -1, va( "cp \"^3%s^7 is down! Go revive them!\n\"", self->client->pers.netname ) );
}

/*
==================
G_ResolveRevive

Completes a successful revive: restores the target to g_reviveHealthPct of
their max health (or a full heal if the reviver is a Medic), clears
bleed-out/revive-progress state on both sides, and grants a short window of
post-revive invulnerability. The downed player's Pmove naturally recomputes
their standing bbox next frame once pm_type flips back to PM_NORMAL, same as
any other transition out of PM_DEAD.
==================
*/
void G_ResolveRevive( gentity_t *reviver, gentity_t *target ) {
	int healAmt;

	if ( reviver->client->ps.stats[STAT_PLAYER_CLASS] == PC_MEDIC ) {
		healAmt = target->client->ps.stats[STAT_MAX_HEALTH];
	} else {
		healAmt = target->client->ps.stats[STAT_MAX_HEALTH] * g_reviveHealthPct.integer / 100;
	}
	if ( healAmt < 1 ) {
		healAmt = 1;
	}

	target->health = target->client->ps.stats[STAT_HEALTH] = healAmt;
	target->client->ps.stats[STAT_REVIVE_TIME] = 0;
	target->client->ps.pm_type = PM_NORMAL;
	target->client->ps.eFlags &= ~EF_DEAD; // PM_Footsteps() won't re-animate a live player while this is set
	target->client->ps.powerups[PW_INVULNERABLE] = level.time + g_reviveInvulnTime.integer;
	target->client->revivedByNum = -1;

	BG_PlayAnimName( &target->client->ps, "revive", ANIM_BP_BOTH, qtrue, qfalse, qtrue );
	// G_AddEvent() on a player entity routes through ps.events[], which is only ever sent
	// to that player's own client -- G_Sound() fires on a temp entity instead so everyone nearby hears it
	G_Sound( target, G_SoundIndex( "sound/misc/revived.wav" ) );

	reviver->client->ps.stats[STAT_REVIVE_PROGRESS] = 0;
	reviver->client->reviveTargetNum = -1;
	reviver->client->reviveElapsedMs = 0;

	trap_SendServerCommand( target->s.number, va( "cp \"You were revived by ^3%s^7!\n\"", reviver->client->pers.netname ) );
	trap_SendServerCommand( reviver->s.number, va( "cp \"You revived ^3%s^7!\n\"", target->client->pers.netname ) );
}

/*
==================
G_ResolveSelfRevive

Completes a PERK_SECONDCHANCE self-revive. Same partial heal as an ordinary
ally revive (g_reviveHealthPct of max health -- the rest has to regen
naturally), except a Medic always comes back at full health, same as a Medic
reviving someone else. The perk consumes itself on use either way: base tier
(level 1) strips every perk the player owns, PRO tier (level 2) keeps
everything else and only loses Second Chance itself -- it has to be re-bought
before it can save the player again.
==================
*/
static void G_ResolveSelfRevive( gentity_t *ent ) {
	int healAmt;
	int secondChanceLevel = ent->client->ps.perks[PERK_SECONDCHANCE];
	int i;

	if ( ent->client->ps.stats[STAT_PLAYER_CLASS] == PC_MEDIC ) {
		healAmt = ent->client->ps.stats[STAT_MAX_HEALTH];
	} else {
		healAmt = ent->client->ps.stats[STAT_MAX_HEALTH] * g_reviveHealthPct.integer / 100;
	}
	if ( healAmt < 1 ) {
		healAmt = 1;
	}

	ent->health = ent->client->ps.stats[STAT_HEALTH] = healAmt;
	ent->client->ps.stats[STAT_REVIVE_TIME] = 0;
	ent->client->ps.pm_type = PM_NORMAL;
	ent->client->ps.eFlags &= ~EF_DEAD; // PM_Footsteps() won't re-animate a live player while this is set
	ent->client->ps.powerups[PW_INVULNERABLE] = level.time + g_reviveInvulnTime.integer;
	ent->client->revivedByNum = -1;
	ent->client->reviveTargetNum = -1;
	ent->client->reviveElapsedMs = 0;
	ent->client->ps.stats[STAT_REVIVE_PROGRESS] = 0;

	BG_PlayAnimName( &ent->client->ps, "revive", ANIM_BP_BOTH, qtrue, qfalse, qtrue );
	G_Sound( ent, G_SoundIndex( "sound/misc/revived.wav" ) );

	if ( secondChanceLevel >= 2 ) {
		// PRO: only Second Chance itself is consumed
		ent->client->ps.perks[PERK_SECONDCHANCE] = 0;
		ent->client->ps.stats[STAT_PERK] &= ~( 1 << PERK_SECONDCHANCE );
	} else {
		// base tier: the fake death costs every perk, not just this one
		for ( i = 0; i < MAX_PERKS; i++ ) {
			ent->client->ps.perks[i] = 0;
		}
		ent->client->ps.stats[STAT_PERK] = 0;
	}

	trap_SendServerCommand( ent->s.number, "cp \"^3Second Chance!^7 You revived yourself!\n\"" );
}

/*
==================
G_BleedoutExpireToDeath

Commits an unrevived bleeding-out player to a real death. player_die() always
forces a fresh falling-down anim, but this player already fell when they
entered bleed-out -- reassert the wounded pose right after so they don't
visibly pop back to standing just to collapse a second time.
==================
*/
static void G_BleedoutExpireToDeath( gentity_t *ent ) {
	gentity_t *killerEnt = &g_entities[ent->client->bleedoutAttackerNum];

	ent->client->ps.stats[STAT_REVIVE_TIME] = 0;
	ent->client->ps.pm_type = PM_NORMAL; // player_die() no-ops if pm_type is already PM_DEAD
	player_die( ent, killerEnt, killerEnt, 100000, ent->client->bleedoutMOD );

	ent->client->bleedoutFallEndTime = 0;
	BG_PlayAnimName( &ent->client->ps, "wounded_idle_1", ANIM_BP_BOTH, qtrue, qfalse, qtrue );
}

/*
==================
G_TickReviveStates

GT_COOP_SURVIVAL only, called once per server frame from G_RunFrame. Counts
down bleed-out timers (committing a real death via player_die() when one
expires), and starts/advances/validates in-progress revive attempts.
==================
*/
void G_TickReviveStates( void ) {
	int i;
	int frameTime;
	gentity_t *ent;

	frameTime = level.time - level.previousTime;
	if ( frameTime <= 0 ) {
		return;
	}

	// wave already declared over -- force any still-bleeding-out players through a real death before the queued map_restart fires
	if ( svParams.waveGameOver ) {
		for ( i = 0; i < level.maxclients; i++ ) {
			ent = &g_entities[i];

			if ( !ent->inuse || !ent->client || ent->client->ps.stats[STAT_REVIVE_TIME] <= 0 ) {
				continue;
			}

			if ( ent->client->revivedByNum != -1 ) {
				gentity_t *reviver = &g_entities[ent->client->revivedByNum];

				if ( reviver->inuse && reviver->client && reviver->client->reviveTargetNum == i ) {
					reviver->client->reviveTargetNum = -1;
					reviver->client->ps.stats[STAT_REVIVE_PROGRESS] = 0;
				}
			}

			G_BleedoutExpireToDeath( ent );
		}
		return;
	}

	for ( i = 0; i < level.maxclients; i++ ) {
		ent = &g_entities[i];

		if ( !ent->inuse || !ent->client ) {
			continue;
		}
		if ( ent->client->pers.connected != CON_CONNECTED ) {
			continue;
		}

		// tick down bleed-out, unless someone is actively reviving this player
		if ( ent->client->ps.stats[STAT_REVIVE_TIME] > 0 ) {
			// settle into the wounded loop once the falling-down anim has finished playing
			if ( ent->client->bleedoutFallEndTime > 0 && level.time >= ent->client->bleedoutFallEndTime ) {
				ent->client->bleedoutFallEndTime = 0;
				BG_PlayAnimName( &ent->client->ps, "wounded_idle_1", ANIM_BP_BOTH, qtrue, qfalse, qtrue );
			}

			// clear a stale reference if our reviver disconnected or moved on mid-attempt
			if ( ent->client->revivedByNum != -1 ) {
				gentity_t *reviver = &g_entities[ent->client->revivedByNum];

				if ( !reviver->inuse || !reviver->client || reviver->client->reviveTargetNum != i ) {
					ent->client->revivedByNum = -1;
				}
			}

			// PERK_SECONDCHANCE: self-revive by holding activate while nobody else is reviving us
			if ( ent->client->revivedByNum == -1 && ent->client->ps.perks[PERK_SECONDCHANCE] > 0 &&
				 ( ent->client->buttons & BUTTON_ACTIVATE ) ) {
				int reviveTime = ( ent->client->ps.stats[STAT_PLAYER_CLASS] == PC_MEDIC ) ?
								 g_reviveTimeMedic.integer : g_reviveTimeNormal.integer;
				if ( reviveTime < 1 ) {
					reviveTime = 1;
				}

				ent->client->reviveTargetNum = i;
				ent->client->reviveElapsedMs += frameTime;
				if ( ent->client->reviveElapsedMs >= reviveTime ) {
					ent->client->ps.stats[STAT_REVIVE_PROGRESS] = 100;
					G_ResolveSelfRevive( ent );
				} else {
					ent->client->ps.stats[STAT_REVIVE_PROGRESS] = ent->client->reviveElapsedMs * 100 / reviveTime;
				}
				continue;
			}

			// not attempting (or no longer holding) a self-revive -- drop any partial progress
			ent->client->reviveTargetNum = -1;
			ent->client->reviveElapsedMs = 0;
			ent->client->ps.stats[STAT_REVIVE_PROGRESS] = 0;

			if ( ent->client->revivedByNum == -1 ) {
				ent->client->ps.stats[STAT_REVIVE_TIME] -= frameTime;
				if ( ent->client->ps.stats[STAT_REVIVE_TIME] <= 0 ) {
					G_BleedoutExpireToDeath( ent );
				}
			}
			continue; // can't be reviving anyone else while down
		}

		// advance/validate an in-progress revive this player is performing
		if ( ent->client->reviveTargetNum != -1 ) {
			gentity_t *target = &g_entities[ent->client->reviveTargetNum];
			qboolean stillValid = qtrue;
			int reviveTime;

			if ( ent->health <= 0 ) {
				stillValid = qfalse;
			} else if ( !( ent->client->buttons & BUTTON_ACTIVATE ) ) {
				stillValid = qfalse;
			} else if ( ent->client->ps.serverCursorHint != HINT_REVIVE ||
						ent->client->ps.serverCursorHintVal - 1 != ent->client->reviveTargetNum ) {
				stillValid = qfalse;
			} else if ( !target->inuse || !target->client || target->client->ps.stats[STAT_REVIVE_TIME] <= 0 ) {
				stillValid = qfalse;
			}

			if ( !stillValid ) {
				ent->client->ps.stats[STAT_REVIVE_PROGRESS] = 0;
				ent->client->reviveElapsedMs = 0;
				if ( target->inuse && target->client && target->client->revivedByNum == ent->s.number ) {
					target->client->revivedByNum = -1;
				}
				ent->client->reviveTargetNum = -1;
				continue;
			}

			reviveTime = ( ent->client->ps.stats[STAT_PLAYER_CLASS] == PC_MEDIC ) ?
						 g_reviveTimeMedic.integer : g_reviveTimeNormal.integer;
			if ( reviveTime < 1 ) {
				reviveTime = 1;
			}

			// track raw elapsed ms (not the rounded 0-100 stat) so slow per-frame
			// increments can't get lost to integer-division truncation
			ent->client->reviveElapsedMs += frameTime;
			if ( ent->client->reviveElapsedMs >= reviveTime ) {
				ent->client->ps.stats[STAT_REVIVE_PROGRESS] = 100;
				G_ResolveRevive( ent, target );
			} else {
				ent->client->ps.stats[STAT_REVIVE_PROGRESS] = ent->client->reviveElapsedMs * 100 / reviveTime;
			}
			continue;
		}

		// not reviving anyone -- see if we should start
		if ( ( ent->client->buttons & BUTTON_ACTIVATE ) &&
			 ent->health > 0 &&
			 ent->client->ps.serverCursorHint == HINT_REVIVE &&
			 ent->client->ps.serverCursorHintVal > 0 ) {

			int targetNum = ent->client->ps.serverCursorHintVal - 1;
			gentity_t *target = &g_entities[targetNum];

			if ( target->inuse && target->client && target->client->ps.stats[STAT_REVIVE_TIME] > 0 &&
				 target->client->revivedByNum == -1 ) {
				ent->client->reviveTargetNum = targetNum;
				ent->client->reviveElapsedMs = 0;
				target->client->revivedByNum = ent->s.number;
				ent->client->ps.stats[STAT_REVIVE_PROGRESS] = 1; // kick off, refined next frame
			}
		}
	}
}

/*
==================
G_CompleteConstructible

Marks a func_constructible built, fires its target chain and "built" script
event, and removes anything (trigger_objective_info, misc_constructiblemarker)
that was pointing at it -- same targetname cleanup use_invisible_user does.
The placeholder brush itself is unlinked out of the way (e.g. of whatever it
was targeting, like a misc_mg42).
==================
*/
static void G_CompleteConstructible( gentity_t *ent, gentity_t *activator ) {
	int i;
	gentity_t *e;

	ent->active = qtrue;
	trap_UnlinkEntity( ent );
	G_UseTargets( ent, activator );

	if ( ent->scriptName ) {
		G_Script_ScriptEvent( ent, "built", "" );
	}

	if ( ent->targetname ) {
		for ( i = 0; i < level.num_entities; i++ ) {
			e = &g_entities[i];
			if ( !e->inuse || !e->classname || !e->target ) {
				continue;
			}
			if ( Q_stricmp( e->target, ent->targetname ) ) {
				continue;
			}
			if ( !Q_stricmp( e->classname, "trigger_objective_info" ) || !Q_stricmp( e->classname, "misc_constructiblemarker" ) ) {
				G_FreeEntity( e );
			}
		}
	}

	trap_SendServerCommand( activator->s.number, "cp \"Construction complete!\n\"" );
}

#define CONSTRUCT_DENY_SOUND_INTERVAL 1000

/*
==================
G_ConstructDenySound

Plays the same "can't afford it" cue survival purchases use, throttled per
player so holding ACTIVATE against a constructible you can't afford doesn't
spam it every server frame.
==================
*/
static void G_ConstructDenySound( gentity_t *ent ) {
	if ( ent->client->constructDenySoundTime > level.time ) {
		return;
	}
	ent->client->constructDenySoundTime = level.time + CONSTRUCT_DENY_SOUND_INTERVAL;
	trap_SendServerCommand( -1, "mu_play sound/items/use_nothing.wav 0\n" );
}

/*
==================
G_UpdateConstructAction

Drives ps.stats[STAT_ACTIVE_ACTION] for one client: ACTION_CONSTRUCT while a
build is in progress, then a short ACTION_CONSTRUCT_LOWER tail (PLIERS_LOWER_MS)
so cg_actionview.c can play the pliers-drop before bg_pmove.c raises the real
weapon. Death skips the tail -- the pliers just goes with the body.
==================
*/
static void G_UpdateConstructAction( gentity_t *ent, qboolean building ) {
	int *action = &ent->client->ps.stats[STAT_ACTIVE_ACTION];

	if ( building ) {
		*action = ACTION_CONSTRUCT;
		return;
	}

	switch ( *action ) {
	case ACTION_CONSTRUCT:
		if ( ent->health <= 0 ) {
			*action = ACTION_NONE;
		} else {
			*action = ACTION_CONSTRUCT_LOWER;
			ent->client->actionLowerTime = level.time + PLIERS_LOWER_MS;
		}
		break;
	case ACTION_CONSTRUCT_LOWER:
		if ( ent->health <= 0 || level.time >= ent->client->actionLowerTime ) {
			*action = ACTION_NONE;
		}
		break;
	default:
		break;
	}
}

/*
==================
G_TickConstructionStates

GT_COOP_SURVIVAL only (caller in G_RunFrame already gates on this, same as
G_TickReviveStates). Any class holding ACTIVATE on an unbuilt
func_constructible -- per the server cursor hint -- advances its buildProgress,
paying its price gradually out of PERS_SCORE. Unlike revive, progress lives on
the entity and is never reset on interrupt; running out of points just stalls
it until someone (the same player or another) can afford to continue.
==================
*/
void G_TickConstructionStates( void ) {
	int i, frameTime;
	gentity_t *ent, *target;

	frameTime = level.time - level.previousTime;
	if ( frameTime <= 0 ) {
		return;
	}

	for ( i = 0; i < level.maxclients; i++ ) {
		float rate, newProgress;
		int oldCharged, newCharged, costThisTick;

		ent = &g_entities[i];
		if ( !ent->inuse || !ent->client ) {
			continue;
		}
		if ( ent->client->pers.connected != CON_CONNECTED ) {
			continue;
		}

		if ( ent->health <= 0 || ent->client->ps.stats[STAT_REVIVE_TIME] > 0 ||
			 !( ent->client->buttons & BUTTON_ACTIVATE ) ||
			 ent->client->ps.serverCursorHint != HINT_BUILD ||
			 ent->client->ps.serverCursorHintVal <= 0 ) {
			ent->client->ps.stats[STAT_CONSTRUCT_PROGRESS] = 0;
			G_UpdateConstructAction( ent, qfalse );
			continue;
		}

		// re-validated fresh every player's turn to stop two builders double-firing completion in one frame
		target = &g_entities[ ent->client->ps.serverCursorHintVal - 1 ];
		if ( !target->inuse || Q_stricmp( target->classname, "func_constructible" ) || target->active ) {
			ent->client->ps.stats[STAT_CONSTRUCT_PROGRESS] = 0;
			G_UpdateConstructAction( ent, qfalse );
			continue;
		}

		// holding activate on a valid unbuilt target counts as building even while stalled (broke / can't afford a slice)
		G_UpdateConstructAction( ent, qtrue );

		// broke -- don't let a 0-point player nibble free progress before the gradual charge below catches up
		if ( target->price > 0 && ent->client->ps.persistant[PERS_SCORE] <= 0 ) {
			ent->client->ps.stats[STAT_CONSTRUCT_PROGRESS] = target->buildProgress * 100 / target->buildTime;
			G_ConstructDenySound( ent );
			continue;
		}

		rate = ( ent->client->ps.stats[STAT_PLAYER_CLASS] == PC_ENGINEER ) ? g_engineerBuildRate.value : 1.0f;
		newProgress = target->buildProgress + frameTime * rate;
		if ( newProgress > target->buildTime ) {
			newProgress = target->buildTime;
		}

		// points are spent gradually: charge only the slice this tick actually crossed
		oldCharged = (int)( ( target->price * target->buildProgress ) / target->buildTime );
		newCharged = (int)( ( target->price * newProgress ) / target->buildTime );
		costThisTick = newCharged - oldCharged;

		if ( costThisTick > 0 ) {
			if ( ent->client->ps.persistant[PERS_SCORE] < costThisTick ) {
				// can't afford the next slice -- hold here, don't advance or charge
				ent->client->ps.stats[STAT_CONSTRUCT_PROGRESS] = target->buildProgress * 100 / target->buildTime;
				G_ConstructDenySound( ent );
				continue;
			}
			Survival_AwardScore( ent, -costThisTick );
		}

		if ( target->buildProgress <= 0 && newProgress > 0 && target->scriptName ) {
			G_Script_ScriptEvent( target, "buildstart", "" );
		}

		target->buildProgress = newProgress;
		ent->client->ps.stats[STAT_CONSTRUCT_PROGRESS] = target->buildProgress * 100 / target->buildTime;

		if ( target->buildProgress >= target->buildTime ) {
			G_CompleteConstructible( target, ent );
		}
	}
}

/*
================
CheckArmor
================
*/
int CheckArmor( gentity_t *ent, int damage, int dflags ) {
	gclient_t   *client;
	int save;
	int count;

	if ( !damage ) {
		return 0;
	}

	client = ent->client;

	if ( !client ) {
		return 0;
	}

	if ( dflags & DAMAGE_NO_ARMOR ) {
		return 0;
	}

	// armor
	count = client->ps.stats[STAT_ARMOR];
	save = ceil( damage * ARMOR_PROTECTION );
	if ( save >= count ) {
		save = count;
	}

	if ( !save ) {
		return 0;
	}

	client->ps.stats[STAT_ARMOR] -= save;

	return save;
}


/*
==============
IsHeadShotWeapon
==============
*/
qboolean IsHeadShotWeapon( int mod, gentity_t *targ, gentity_t *attacker ) {
	// distance rejection
	if ( DistanceSquared( targ->r.currentOrigin, attacker->r.currentOrigin )  >  ( g_headshotMaxDist.integer * g_headshotMaxDist.integer ) ) {
		return qfalse;
	}

	if ( attacker->aiCharacter ) {
		// ai's are always allowed headshots from these weapons
		if ( mod == MOD_SNIPERRIFLE ||
			 mod == MOD_SNOOPERSCOPE ||
			 mod == MOD_M1941SCOPE ||
			 mod == MOD_DELISLESCOPE ) {
			return qtrue;
		}

		if ( g_gameskill.integer != GSKILL_MAX ) {
			// ai's allowed headshots in skill==GSKILL_MAX
			return qfalse;
		}
	}

	switch ( mod ) {
		// players are allowed headshots from these weapons
	case MOD_LUGER:
	case MOD_COLT:
	case MOD_AKIMBO:
	case MOD_TT33:
	case MOD_DUAL_TT33:
	case MOD_REVOLVER:
	case MOD_MP40:
	case MOD_MP34:
	case MOD_THOMPSON:
	case MOD_STEN:
	case MOD_PPSH:
	case MOD_BAR:
	case MOD_FG42:
	case MOD_MP44:
	case MOD_MAUSER:
	case MOD_MOSIN:
	case MOD_SNOOPER:
	case MOD_M1GARAND:
	case MOD_G43:
	case MOD_SILENCER:
	case MOD_FG42SCOPE:
	case MOD_SNOOPERSCOPE:
	case MOD_SNIPERRIFLE:
	case MOD_M1941:
	case MOD_M1941SCOPE:
	case MOD_HDM:
	case MOD_DELISLE:
	case MOD_DELISLESCOPE:
		return qtrue;
	}

	return qfalse;
}


/*
==============
IsHeadShot
==============
*/
qboolean IsHeadShot( gentity_t *targ, gentity_t *attacker, vec3_t dir, vec3_t point, int mod ) {
	gentity_t   *head;
	trace_t tr;
	vec3_t start, end;
	gentity_t   *traceEnt;
	orientation_t or;

	qboolean head_shot_weapon = qfalse;

	// not a player or critter so bail
	if ( !( targ->client ) ) {
		return qfalse;
	}

	if ( targ->health <= 0 ) {
		return qfalse;
	}

	head_shot_weapon = IsHeadShotWeapon( mod, targ, attacker );

	if ( head_shot_weapon ) {
		head = G_Spawn();

		G_SetOrigin( head, targ->r.currentOrigin );

		// RF, if there is a valid tag_head for this entity, then use that
		if ( ( targ->r.svFlags & SVF_CASTAI ) && trap_GetTag( targ->s.number, "tag_head", &or ) ) {
			VectorCopy( or.origin, head->r.currentOrigin );
			VectorMA( head->r.currentOrigin, 6, or.axis[2], head->r.currentOrigin );    // tag is at base of neck
		} else if ( targ->client->ps.pm_flags & PMF_DUCKED ) { // closer fake offset for 'head' box when crouching
			head->r.currentOrigin[2] += targ->client->ps.crouchViewHeight + 8; // JPW NERVE 16 is kludge to get head height to match up
		}
		//else if(targ->client->ps.legsAnim == LEGS_IDLE && targ->aiCharacter == AICHAR_SOLDIER)	// standing with legs bent (about a head shorter than other leg poses)
		//	head->r.currentOrigin[2] += targ->client->ps.viewheight;
		else {
			head->r.currentOrigin[2] += targ->client->ps.viewheight; // JPW NERVE pulled this	// 6 is fudged "head height" value

		}
		VectorCopy( head->r.currentOrigin, head->s.origin );
		VectorCopy( targ->r.currentAngles, head->s.angles );
		VectorCopy( head->s.angles, head->s.apos.trBase );
		VectorCopy( head->s.angles, head->s.apos.trDelta );
		VectorSet( head->r.mins, -6, -6, -6 ); // JPW NERVE changed this z from -12 to -6 for crouching, also removed standing offset
		VectorSet( head->r.maxs, 6, 6, 6 ); // changed this z from 0 to 6
		head->clipmask = CONTENTS_SOLID;
		head->r.contents = CONTENTS_SOLID;

		trap_LinkEntity( head );

		// trace another shot see if we hit the head
		VectorCopy( point, start );
		VectorMA( start, 64, dir, end );
		trap_Trace( &tr, start, NULL, NULL, end, targ->s.number, MASK_SHOT );

		traceEnt = &g_entities[ tr.entityNum ];

		if ( g_debugBullets.integer >= 3 ) {   // show hit player head bb
			gentity_t *tent;
			vec3_t b1, b2;
			VectorCopy( head->r.currentOrigin, b1 );
			VectorCopy( head->r.currentOrigin, b2 );
			VectorAdd( b1, head->r.mins, b1 );
			VectorAdd( b2, head->r.maxs, b2 );
			tent = G_TempEntity( b1, EV_RAILTRAIL );
			VectorCopy( b2, tent->s.origin2 );
			tent->s.dmgFlags = 1;

			// show headshot trace
			// end the headshot trace at the head box if it hits
			if ( tr.fraction != 1 ) {
				VectorMA( start, ( tr.fraction * 64 ), dir, end );
			}
			tent = G_TempEntity( start, EV_RAILTRAIL );
			VectorCopy( end, tent->s.origin2 );
			tent->s.dmgFlags = 0;
		}

		G_FreeEntity( head );

		if ( traceEnt == head ) {
			return qtrue;
		}
	}

	return qfalse;
}

gentity_t* G_BuildHead( gentity_t *ent ) {
	gentity_t* head;
	orientation_t or;           // DHM - Nerve

	head = G_Spawn();

	if ( trap_GetTag( ent->s.number, "tag_head", &or ) ) {
		G_SetOrigin( head, or.origin );
	} else {
		float height, dest;
		vec3_t v, angles, forward, up, right;

		G_SetOrigin( head, ent->r.currentOrigin );

		if ( ent->client->ps.pm_flags & PMF_DUCKED ) { // closer fake offset for 'head' box when crouching
			height = ent->client->ps.crouchViewHeight - 12;
		} else {
			height = ent->client->ps.viewheight;
		}

		// NERVE - SMF - this matches more closely with WolfMP models
		VectorCopy( ent->client->ps.viewangles, angles );
		if ( angles[PITCH] > 180 ) {
			dest = ( -360 + angles[PITCH] ) * 0.75;
		} else {
			dest = angles[PITCH] * 0.75;
		}
		angles[PITCH] = dest;

		AngleVectors( angles, forward, right, up );
		VectorScale( forward, 5, v );
		VectorMA( v, 18, up, v );

		VectorAdd( v, head->r.currentOrigin, head->r.currentOrigin );
		head->r.currentOrigin[2] += height / 2;
		// -NERVE - SMF
	}

	VectorCopy( head->r.currentOrigin, head->s.origin );
	VectorCopy( ent->r.currentAngles, head->s.angles );
	VectorCopy( head->s.angles, head->s.apos.trBase );
	VectorCopy( head->s.angles, head->s.apos.trDelta );
	VectorSet( head->r.mins, -6, -6, -2 ); // JPW NERVE changed this z from -12 to -6 for crouching, also removed standing offset
	VectorSet( head->r.maxs, 6, 6, 10 ); // changed this z from 0 to 6
	head->clipmask = CONTENTS_SOLID;
	head->r.contents = CONTENTS_SOLID;
	head->parent = ent;
	head->s.eType = ET_TEMPHEAD;
	head->classname = "playerhead";

	trap_LinkEntity( head );

	return head;
}

/*
==============
G_ArmorDamage
	brokeparts is how many should be broken off now
	curbroke is how many are broken
	the difference is how many to pop off this time
==============
*/
void G_ArmorDamage( gentity_t *targ ) {
	int brokeparts, curbroke;
	int numParts;
	int dmgbits = 16;   // 32/2;
	int i;

	if ( !targ->client ) {
		return;
	}

	if ( targ->s.aiChar == AICHAR_PROTOSOLDIER ) {
		numParts = 9;
	} else if ( targ->s.aiChar == AICHAR_SUPERSOLDIER ) {
		numParts = 14;
	} else if ( targ->s.aiChar == AICHAR_HEINRICH ) {
		numParts = 20;
	} else {
		return;
	}

	if ( numParts > dmgbits ) {
		numParts = dmgbits; // lock this down so it doesn't overwrite any bits that it shouldn't.  TODO: fix this


	}
	// determined here (on server) by location of hit and existing armor, you're updating here so
	// the client knows which pieces are still in place, and by difference with previous state, which
	// pieces to play an effect where the part is blown off.
	// Need to do it here so we have info on where the hit registered (head, torso, legs or if we go with more detail; arm, leg, chest, codpiece, etc)

	// ... Ick, just discovered that the refined hit detection ("hit nearest to which tag") is clientside...

	// For now, I'll randomly pick a part that hasn't been cleared.  This might end up looking okay, and we won't need the refined hits.
	//	however, we still have control on the server-side of which parts come off, regardless of what shceme is used.

	brokeparts = (int)( ( 1 - ( (float)( targ->health ) / (float)( targ->client->ps.stats[STAT_MAX_HEALTH] ) ) ) * numParts );

	// RF, remove flame protection after enough parts gone
	if ( AICast_NoFlameDamage( targ->s.number ) && ( (float)brokeparts / (float)numParts >= 5.0 / 6.0 ) ) { // figure from DM
		AICast_SetFlameDamage( targ->s.number, qfalse );
	}

	if ( brokeparts && ( ( targ->s.dmgFlags & ( ( 1 << numParts ) - 1 ) ) != ( 1 << numParts ) - 1 ) ) {   // there are still parts left to clear

		// how many are removed already?
		curbroke = 0;
		for ( i = 0; i < numParts; i++ ) {
			if ( targ->s.dmgFlags & ( 1 << i ) ) {
				curbroke++;
			}
		}

		// need to remove more
		if ( brokeparts - curbroke >= 1 && curbroke < numParts ) {
			for ( i = 0; i < ( brokeparts - curbroke ); i++ ) {
				int remove = rand() % ( numParts );

				if ( !( ( targ->s.dmgFlags & ( ( 1 << numParts ) - 1 ) ) != ( 1 << numParts ) - 1 ) ) { // no parts are available any more
					break;
				}

				// FIXME: lose the 'while' loop.  Still should be safe though, since the check above verifies that it will eventually find a valid part
				while ( targ->s.dmgFlags & ( 1 << remove ) ) {
					remove = rand() % ( numParts );
				}

				targ->s.dmgFlags |= ( 1 << remove );    // turn off 'undamaged' part
				if ( (int)( random() + 0.5 ) ) {                       // choose one of two possible replacements
					targ->s.dmgFlags |= ( 1 << ( numParts + remove ) );
				}
			}
		}
	}
}
/*
============
T_Damage

targ		entity that is being damaged
inflictor	entity that is causing the damage
attacker	entity that caused the inflictor to damage targ
	example: targ=monster, inflictor=rocket, attacker=player

dir			direction of the attack for knockback
point		point at which the damage is being inflicted, used for headshots
damage		amount of damage being inflicted
knockback	force to be applied against targ as a result of the damage

inflictor, attacker, dir, and point can be NULL for environmental effects

dflags		these flags are used to control how T_Damage works
	DAMAGE_RADIUS			damage was indirect (from a nearby explosion)
	DAMAGE_NO_ARMOR			armor does not protect from this damage
	DAMAGE_NO_KNOCKBACK		do not affect velocity, just view angles
	DAMAGE_NO_PROTECTION	kills godmode, armor, everything
============
*/

/*
==================
G_KillOrEnterBleedout

Dispatches a lethal hit: in GT_COOP_SURVIVAL, a real player with a living
teammate and who isn't already downed enters bleed-out instead of dying
outright. A PERK_SECONDCHANCE owner enters bleed-out even with nobody left
standing to revive them, since the perk lets them revive themselves (see
G_ResolveSelfRevive). Everything else (AI, breakables, admin/self kills that
call player_die() directly, finishing blows on an already-downed player, or
overkill past GIB_HEALTH) dies as normal.
==================
*/
static void G_KillOrEnterBleedout( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker, int damage, int mod ) {
	if ( g_gametype.integer == GT_COOP_SURVIVAL && targ->client &&
		 !( targ->r.svFlags & SVF_CASTAI ) &&
		 targ->client->ps.stats[STAT_REVIVE_TIME] <= 0 &&
		 targ->health > GIB_HEALTH &&
		 ( G_HasLivingTeammate( targ ) || targ->client->ps.perks[PERK_SECONDCHANCE] > 0 ) ) {
		G_EnterBleedout( targ, inflictor, attacker, damage, mod );
		return;
	}

	if ( targ->die ) { // Ridah, mg42 doesn't have die func (FIXME)
		targ->die( targ, inflictor, attacker, damage, mod );
	}
}

void G_Damage( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
			   vec3_t dir, vec3_t point, int damage, int dflags, int mod ) {
	gclient_t   *client;
	int take;
	int asave;
	int knockback;
	qboolean attackerpain = qfalse;
	hitEvent_t hitEventType = HIT_NONE;
	qboolean tookHeadshot = qfalse;
	int oldHealth;
	int i;

	if ( !targ->takedamage ) {
		return;
	}

//----(SA)	added
		if ( g_gametype.integer <= GT_SINGLE_PLAYER && !targ->aiCharacter && targ->client && targ->client->cameraPortal ) {
		// get out of damage in sp if in cutscene.
		return;
	}
//----(SA)	end

//	if (reloading || saveGamePending) {	// map transition is happening, don't do anything
	if ( g_reloading.integer || saveGamePending ) {
		return;
	}

	// the intermission has already been qualified for, so don't
	// allow any extra scoring
	if ( level.intermissionQueued || g_gamestate.integer != GS_PLAYING ) {
		return;
	}

	// touching a player with a knife unfreezes them
	if ( targ && targ->client && attacker ) {
		if ( g_gametype.integer <= GT_COOP && g_freeze.integer == 1 && targ->client->ps.eFlags & EF_FROZEN ) {
			if ( mod == MOD_KNIFE && !( attacker->r.svFlags & SVF_CASTAI ) ) {
				targ->client->ps.eFlags &= ~EF_FROZEN;
				targ->flags &= ~FL_NOTARGET;
				targ->client->ps.powerups[PW_INVULNERABLE] = level.time + 5000;                 // some time to find cover
				return;
			}
			return;
		}
	}

	// RF, track pain for player
	// This is used by AI to determine how long it has been since their enemy was injured

	if ( attacker ) { // (SA) whoops, for falling damage there's no attacker
		if ( targ->client && attacker->client && !( targ->r.svFlags & SVF_CASTAI ) && ( attacker->r.svFlags & SVF_CASTAI ) ) {
			AICast_RegisterPain( targ->s.number );
		}
	}

	if ( ( g_gametype.integer <= GT_SINGLE_PLAYER ) && !( targ->r.svFlags & SVF_CASTAI ) ) { // the player
		switch ( mod )
		{
		case MOD_GRENADE:
		case MOD_GRENADE_SPLASH:
		case MOD_ROCKET:
		case MOD_ROCKET_SPLASH:
			// Rafael - had to change this since the
			// we added a new lvl of diff
			if ( g_gameskill.integer == GSKILL_EASY ) {
				damage *= 0.25;
			} else if ( g_gameskill.integer == GSKILL_MEDIUM ) {
				damage *= 0.75;
			} else if ( g_gameskill.integer == GSKILL_HARD ) {
				damage *= 0.9;
			} else {
				damage *= 0.9;
			}
		default:
			break;
		}
	}

	if ( !inflictor ) {
		inflictor = &g_entities[ENTITYNUM_WORLD];
	}
	if ( !attacker ) {
		attacker = &g_entities[ENTITYNUM_WORLD];
	}

	// shootable doors / buttons don't actually have any health
	if ( targ->s.eType == ET_MOVER && !( targ->aiName ) && !( targ->isProp ) && !targ->scriptName ) {
		if ( targ->use && targ->moverState == MOVER_POS1 ) {
			targ->use( targ, inflictor, attacker );
		}
		return;
	}

	if ( targ->s.eType == ET_MOVER && targ->aiName && !( targ->isProp ) && !targ->scriptName ) {
		switch ( mod ) {
		case MOD_GRENADE:
		case MOD_GRENADE_SPLASH:
		case MOD_ROCKET:
		case MOD_ROCKET_SPLASH:
			break;
		default:
			return; // no damage from other weapons
		}
	} else if ( targ->s.eType == ET_EXPLOSIVE )   {
		// 32 Explosive
		// 64 Dynamite only
		if ( ( targ->spawnflags & 32 ) || ( targ->spawnflags & 64 ) ) {
			switch ( mod ) {
			case MOD_GRENADE:
			case MOD_GRENADE_SPLASH:
			case MOD_ROCKET:
			case MOD_ROCKET_SPLASH:
			case MOD_AIRSTRIKE:
			case MOD_ARTY:
			case MOD_GRENADE_PINEAPPLE:
			case MOD_MORTAR:
			case MOD_MORTAR_SPLASH:
			case MOD_EXPLOSIVE:
				if ( targ->spawnflags & 64 ) {
					return;
				}

				break;

			case MOD_DYNAMITE:
			case MOD_DYNAMITE_SPLASH:
				break;

			default:
				return;
			}
		}
	}

	// reduce damage by the attacker's handicap value
	// unless they are rocket jumping

	// Ridah, not in single player (skill levels?)
// JPW NERVE pulled this from multiplayer too
/*
	if (g_gametype.integer != GT_SINGLE_PLAYER)
	// done.
	if ( attacker->client && attacker != targ ) {
		damage = damage * attacker->client->ps.stats[STAT_MAX_HEALTH] / 100;
	}
*/
// jpw

	// Ridah, Cast AI's don't hurt other Cast AI's as much
	if ( ( attacker->r.svFlags & SVF_CASTAI ) && ( targ->r.svFlags & SVF_CASTAI ) ) {
		if ( !AICast_AIDamageOK( AICast_GetCastState( targ->s.number ), AICast_GetCastState( attacker->s.number ) ) ) {
			return;
		}
		damage = (int)( ceil( (float)damage * 0.5 ) );
	}
	// done.

	client = targ->client;

	if ( client ) {
		if ( client->noclip ) {
			return;
		}
	}

	// ported from RealRTCW - holdable_xshield grants brief full damage immunity
	if ( client && client->ps.powerups[PW_XSHIELD] ) {
		G_AddEvent( targ, EV_POWERUP_XSHIELD, 0 );
		return;
	}

	if ( !dir ) {
		dflags |= DAMAGE_NO_KNOCKBACK;
	} else {
		VectorNormalize( dir );
	}

	knockback = damage;

//	if ( knockback > 200 )
//		knockback = 200;
	if ( knockback > 60 ) { // /way/ lessened for SP.  keeps dynamite-jumping potential down
		knockback = 60;
	}

	if ( targ->flags & FL_NO_KNOCKBACK ) {
		knockback = 0;
	}
	if ( dflags & DAMAGE_NO_KNOCKBACK ) {
		knockback = 0;
	}

	// cant push big guys. too heavy. Survival only.
	if (g_gametype.integer == GT_COOP_SURVIVAL ) {
	if (targ->aiCharacter == AICHAR_HEINRICH || targ->aiCharacter == AICHAR_HELGA || targ->aiCharacter == AICHAR_PROTOSOLDIER || targ->aiCharacter == AICHAR_SUPERSOLDIER)
	{
		knockback = 0;
	}
    }

	// figure momentum add, even if the damage won't be taken
	if ( knockback && targ->client ) {
		vec3_t kvel;
		float mass;

		mass = 200;

		if ( mod == MOD_LIGHTNING && !( ( level.time + targ->s.number * 50 ) % 400 ) ) {
			knockback = 60;
			dir[2] = 0.3;
		}

		VectorScale( dir, g_knockback.value * (float)knockback / mass, kvel );
		VectorAdd( targ->client->ps.velocity, kvel, targ->client->ps.velocity );

		if ( targ == attacker && !(  mod != MOD_ROCKET &&
									 mod != MOD_ROCKET_SPLASH &&
									 mod != MOD_GRENADE &&
									 mod != MOD_GRENADE_SPLASH &&
									 mod != MOD_DYNAMITE ) ) {
			targ->client->ps.velocity[2] *= 0.25;
		}

		// set the timer so that the other client can't cancel
		// out the movement immediately
		if ( !targ->client->ps.pm_time ) {
			int t;

			t = knockback * 2;
			if ( t < 50 ) {
				t = 50;
			}
			if ( t > 200 ) {
				t = 200;
			}
			targ->client->ps.pm_time = t;
			targ->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
		}
	}

	// check for completely getting out of the damage
	if ( !( dflags & DAMAGE_NO_PROTECTION ) ) {

		// if TF_NO_FRIENDLY_FIRE is set, don't do damage to the target
		// if the attacker was on the same team
		if ( targ != attacker && OnSameTeam( targ, attacker )  ) {
			if ( !g_friendlyFire.integer ) {
				return;
			}
		}

		// check for godmode
		if ( targ->flags & FL_GODMODE ) {
			return;
		}

		// RF, warzombie defense position is basically godmode for the time being
		if ( targ->flags & FL_DEFENSE_GUARD ) {
			return;
		}

		// check for invulnerability // (SA) moved from below so DAMAGE_NO_PROTECTION will still work
		if ( client && client->ps.powerups[PW_INVULNERABLE] ) { //----(SA)	added
			return;
		}

	}

	// battlesuit protects from all radius damage (but takes knockback)
	// and protects 50% against all damage
	if ( client && client->ps.powerups[PW_BATTLESUIT] ) {
		G_AddEvent( targ, EV_POWERUP_BATTLESUIT, 0 );
		if ( dflags & DAMAGE_RADIUS ) {
			return;
		}
		damage *= 0.5;
	}

	if ( client && client->ps.powerups[PW_BATTLESUIT_SURV] ) {
		G_AddEvent( targ, EV_POWERUP_BATTLESUIT_SURV, 0 );

		damage *= 0.05;
	}

	// always give half damage if hurting self
	// calculated after knockback, so rocket jumping works
	if ( g_gametype.integer <= GT_SINGLE_PLAYER ) {     // JPW NERVE -- removed from multiplayer -- plays havoc with pfaust & demolition balancing

		qboolean dynamite = (qboolean)( mod == MOD_DYNAMITE || mod == MOD_DYNAMITE_SPLASH );

		if ( targ == attacker ) {
			if ( !dynamite ) {
				damage *= 0.5;
			}
		}

		if ( dynamite && targ->aiCharacter == AICHAR_HELGA ) {
			//helga gets special dynamite damage
			damage *= 0.5;
		}

	}

	if ( damage < 1 ) {
		damage = 1;
	}
	take = damage;

	// don't take armor from friendly players when we try hurt them
	if ( !( dflags & DAMAGE_NO_PROTECTION ) && g_friendlyFire.integer == 0 ) {
	
		if ( inflictor && inflictor->r.ownerNum < MAX_CLIENTS ) {
			gentity_t *owner = &g_entities[inflictor->r.ownerNum];
			
			if ( owner->client && targ->client && !( owner->r.svFlags & SVF_CASTAI ) && !( targ->r.svFlags & SVF_CASTAI ) && owner->client->sess.sessionTeam == targ->client->sess.sessionTeam ) {
				take = 0;

				if ( mod == MOD_FLAMETHROWER && targ->s.onFireEnd > level.time ) {
					targ->s.onFireEnd = level.time;
				}
			} else if ( owner->client && targ->client && !( owner->r.svFlags & SVF_CASTAI ) && ( targ->r.svFlags & SVF_CASTAI ) ) {
				if ( owner->client->sess.sessionTeam == TEAM_BLUE && ( targ->aiTeam == AITEAM_ALLIES || targ->aiTeam == AITEAM_NEUTRAL ) && ( targ->aiCharacter == AICHAR_PARTISAN || targ->aiCharacter == AICHAR_CIVILIAN ) ) {
					take = 0;

					if ( mod == MOD_FLAMETHROWER && targ->s.onFireEnd > level.time ) {
						targ->s.onFireEnd = level.time;
					}
				}
			}
		} else if ( attacker->client && targ->client && attacker != targ && !( attacker->r.svFlags & SVF_CASTAI ) && !( targ->r.svFlags & SVF_CASTAI ) && attacker->client->sess.sessionTeam == targ->client->sess.sessionTeam ) {
			take = 0;
		}
	}

	// save some from armor
	asave = CheckArmor( targ, take, dflags );
	take -= asave;


	if ( IsHeadShot( targ, attacker, dir, point, mod ) ) {
		tookHeadshot = qtrue;
		// JPW NERVE -- different headshot behavior in multiplayer
		if ( g_gametype.integer <= GT_SINGLE_PLAYER ) {
			// by default, a headshot means damage x2
			take *= 2;

			// RF, allow headshot damage multiplier (helmets, etc)
			// yes, headshotDamageScale of 0 gives no damage, thats because
			// the bullet hit the head which is fully protected.
			take *= targ->headshotDamageScale;

			// player only code
			if ( !attacker->aiCharacter ) {
				if ( g_gametype.integer > GT_COOP ) {
					// (SA) id reqests one-shot kills for head shots on common humanoids

					// (SA) except pistols.
					// first pistol head shot does normal 2x damage and flings hat, second gets kill
					//			if((mod != MOD_LUGER && mod != MOD_COLT ) || (targ->client->ps.eFlags & EF_HEADSHOT))	{	// (SA) DM requests removing double shot pistol head shots (3/19)

					// (SA) removed BG for DM.

					if ( !( dflags & DAMAGE_PASSTHRU ) ) {     // ignore headshot 2x damage and snooper-instant-death if the bullet passed through something.  just do reg damage.
						switch ( targ->aiCharacter ) {
						case AICHAR_BLACKGUARD:
							if ( !( targ->client->ps.eFlags & EF_HEADSHOT ) ) { // only obliterate him after he's lost his helmet
								break;
							}
						case AICHAR_SOLDIER:
						case AICHAR_TRENCH:
						case AICHAR_AMERICAN:
						case AICHAR_ELITEGUARD:
						case AICHAR_PARTISAN:
						case AICHAR_CIVILIAN:
							take = 200;
							break;
						default:
							break;
						}
					}
				}

				// toss the hat regardless of coop vs single player, so it doesn't just vanish
				if ( !( targ->client->ps.eFlags & EF_HEADSHOT ) ) {  // only toss hat on first headshot
					G_AddEvent( targ, EV_LOSE_HAT, DirToByte( dir ) );
				}
			}
		} // JPW

		// shared by both player and ai
		targ->client->ps.eFlags |= EF_HEADSHOT;

	} else {    // non headshot

		if ( !( dflags & DAMAGE_PASSTHRU ) ) {     // ignore headshot 2x damage and snooper-instant-death if the bullet passed through something.  just do reg damage.
			// snooper kills these types in one shot with any contact
			if ( ( mod == MOD_SNOOPERSCOPE || mod == MOD_SNOOPER ) && !( attacker->aiCharacter ) ) {
				switch ( targ->aiCharacter ) {
				case AICHAR_SOLDIER:
				case AICHAR_TRENCH:
				case AICHAR_AMERICAN:
				case AICHAR_ELITEGUARD:
				case AICHAR_BLACKGUARD:
				case AICHAR_PARTISAN:
				case AICHAR_CIVILIAN:
					take = 200;
					break;
				default:
					break;
				}
			}
		}
	}

	// classify the hit for feedback; dispatch happens later, gated on whether damage actually lands
	if ( attacker->client && !attacker->aiCharacter && targ->client && targ != attacker ) {
		qboolean isTeamShot = !( attacker->r.svFlags & SVF_CASTAI ) && !( targ->r.svFlags & SVF_CASTAI )
			&& attacker->client->sess.sessionTeam == targ->client->sess.sessionTeam;
		hitEventType = isTeamShot ? HIT_TEAMSHOT : ( tookHeadshot ? HIT_HEADSHOT : HIT_BODYSHOT );
	}
	oldHealth = targ->health;

	if ( g_debugDamage.integer ) {
		G_Printf( "client:%i health:%i damage:%i armor:%i\n", targ->s.number,
				  targ->health, take, asave );
	}

	// add to the damage inflicted on a player this frame
	// the total will be turned into screen blends and view angle kicks
	// at the end of the frame
	if ( client ) {
		if ( attacker ) {
			client->ps.persistant[PERS_ATTACKER] = attacker->s.number;
		} else {
			client->ps.persistant[PERS_ATTACKER] = ENTITYNUM_WORLD;
		}
		client->damage_armor += asave;
		client->damage_blood += take;
		client->damage_knockback += knockback;

		client->healthRegenStartTime = level.time + g_regenDelay.integer; // any damage (world, enemy, self) pushes passive regen back -- CoD-style

		if ( dir ) {
			VectorCopy( dir, client->damage_from );
			client->damage_fromWorld = qfalse;
		} else {
			VectorCopy( targ->r.currentOrigin, client->damage_from );
			client->damage_fromWorld = qtrue;
		}
	}

	// See if it's the player hurting the emeny flag carrier
	Team_CheckHurtCarrier( targ, attacker );

	if ( targ->client ) {
		// set the last client who damaged the target
		targ->client->lasthurt_client = attacker->s.number;
		targ->client->lasthurt_mod = mod;
	}

	// do the damage
	if ( take ) {
		// this makes it easier to make the mg42 work with this new friendlyfire code
		if ( mod == MOD_MACHINEGUN && inflictor && inflictor->r.ownerNum < MAX_CLIENTS ) {
			attacker = &g_entities[inflictor->r.ownerNum];
		}

		// no matter what g_friendlyfire is set to, telefrags should always work, else you get stuck in a player
		if ( mod == MOD_TELEFRAG || mod == MOD_WATER || mod == MOD_FALLING || mod == MOD_SLIME || mod == MOD_LAVA || mod == MOD_TRIGGER_HURT
#ifdef _ADMINS
				|| mod == MOD_SLAP || mod == MOD_ADMKILL
#endif
				|| mod == MOD_CRUSH ) {
			targ->health -= take;
		} else if ( !targ->client ) {
			targ->health = targ->health - take;
			// from human to bot
		} else if ( !( attacker->r.svFlags & SVF_CASTAI ) && ( targ->r.svFlags & SVF_CASTAI ) ) {
			if ( attacker->client && targ->client ) {
				if ( g_gametype.integer <= GT_COOP ) {
					if ( targ->aiCharacter == AICHAR_PARTISAN || targ->aiCharacter == AICHAR_CIVILIAN ) {
						if ( attacker->client->sess.sessionTeam == TEAM_BLUE && ( targ->aiTeam == AITEAM_ALLIES || targ->aiTeam == AITEAM_NEUTRAL ) ) {
							return;
						}
					}
				}
			
				// (from axis team to a nazi or monster (=teamdamage)
				// or
				// from allies team to an allies or a neutral (=teamdamage))
				// AND friendly fire is on
				if ( g_friendlyFire.integer ) { 
					if ( ( attacker->client->sess.sessionTeam == TEAM_BLUE && ( targ->aiTeam == AITEAM_NAZI || targ->aiTeam == AITEAM_MONSTER || targ->aiTeam == AITEAM_ENDMAPBOSS ) ) ||
						( attacker->client->sess.sessionTeam == TEAM_RED && ( targ->aiTeam == AITEAM_ALLIES || targ->aiTeam == AITEAM_NEUTRAL ) ) ) {
						if ( g_friendlyFire.integer == 1 ) {
							targ->health = targ->health - take;
						} else if ( g_friendlyFire.integer >= 2 ) { // hurt the attacker if he hits a teammate
							if ( targ->aiTeam == AITEAM_ENDMAPBOSS ) {
								targ->health = targ->health - take;
							} else {
								attacker->health = attacker->health - take;
								attackerpain = qtrue;
								// show some pain !
								if ( attacker->pain ) {
									attacker->pain( attacker, targ, take, point );
								}
							}
						}
					}
				} else {
					// from axis team to a an allies or a neutral
					// or
					// from allies team to a nazi or monster
					// Fix for end map, added AITEAM_ENDMAPBOSS to possible hurt
					if ( ( attacker->client->sess.sessionTeam == TEAM_BLUE && ( targ->aiTeam == AITEAM_NAZI || targ->aiTeam == AITEAM_MONSTER || targ->aiTeam == AITEAM_ENDMAPBOSS ) ) ||
						( attacker->client->sess.sessionTeam == TEAM_RED && ( targ->aiTeam == AITEAM_ALLIES || targ->aiTeam == AITEAM_NEUTRAL ) ) )
					{
						targ->health = targ->health - take;
					}
				}
			} else if ( !attacker->client && targ->client ) { // explosive barrels
				targ->health = targ->health - take;
			}
		} else if ( ( attacker->r.svFlags & SVF_CASTAI ) && ( targ->r.svFlags & SVF_CASTAI ) ) {
			if ( attacker->aiTeam != targ->aiTeam ) {
				targ->health = targ->health - take;
			}
		}
		else {
			if ( g_friendlyFire.integer == 0 ) {
				if ( attacker->client && targ->client ) {
					if ( attacker->client->sess.sessionTeam != targ->client->sess.sessionTeam ) { // AI to player
						if ( !( targ->r.svFlags & SVF_CASTAI ) && ( attacker->aiCharacter == AICHAR_PARTISAN || attacker->aiCharacter == AICHAR_CIVILIAN ) ) {
							if ( targ->client->sess.sessionTeam == TEAM_BLUE && ( attacker->aiTeam == AITEAM_ALLIES || attacker->aiTeam == AITEAM_NEUTRAL ) ) {
								return;
							}
						}

						targ->health = targ->health - take;
					} else {
						if ( !( attacker->r.svFlags & SVF_CASTAI ) ) {
							if ( attacker == targ ) { // hurt self
								attacker->health = attacker->health - take;
							} else {
								if ( dflags & DAMAGE_NO_PROTECTION ) {
									targ->health = targ->health - take;
								}

								return;
							}
						}
					}
				} else if ( !attacker->client && targ->client && !( attacker->r.svFlags & SVF_CASTAI ) ) { // explosive barrels
					targ->health = targ->health - take;
				}
			} else if ( g_friendlyFire.integer == 1 ) {
				targ->health = targ->health - take;
			} else if ( g_friendlyFire.integer >= 2 ) { // hurt the attacker if he hits a teammate
				if ( attacker->client && targ->client ) {
					if ( !( attacker->r.svFlags & SVF_CASTAI ) ) { // human is the attacker
						if ( attacker->client->sess.sessionTeam == targ->client->sess.sessionTeam ) {

							attacker->health = attacker->health - take;
							// show some pain !
							if ( attacker->pain ) {
								attacker->pain( attacker, targ, take, point );
							}
							attackerpain = qtrue;
						} else if ( attacker->client->sess.sessionTeam != targ->client->sess.sessionTeam ) {
							targ->health = targ->health - take;
						}
					} else {
						targ->health = targ->health - take;
					}
				}
			}
		}

		// only fires if the target was alive and damage actually landed
		if ( hitEventType != HIT_NONE && oldHealth > 0 && targ->health < oldHealth ) {
			char *dmgNumCmd;

			if ( targ->health <= 0 ) {
				hitEventType = HIT_DEATHSHOT;
			}
			trap_SendServerCommand( attacker - g_entities, va( "hitFeedback %d", hitEventType ) );

			// damage dealt + hit location, for optional client-side floating damage numbers
			dmgNumCmd = va( "dmgNum %d %d %d %.1f %.1f %.1f", targ->s.number, oldHealth - targ->health,
							 targ->client->ps.stats[STAT_MAX_HEALTH], point[0], point[1], point[2] );
			trap_SendServerCommand( attacker - g_entities, dmgNumCmd );

			// relay to anyone spectating/following the attacker
			for ( i = 0 ; i < level.maxclients ; i++ ) {
				if ( level.clients[i].sess.sessionTeam == TEAM_SPECTATOR
					&& level.clients[i].sess.spectatorState == SPECTATOR_FOLLOW
					&& level.clients[i].sess.spectatorClient == attacker->s.number ) {
					trap_SendServerCommand( i, va( "hitFeedback %d", hitEventType ) );
					trap_SendServerCommand( i, dmgNumCmd );
				}
			}
		}

		// Ridah, can't gib with bullet weapons (except VENOM)
		if ( targ->client ) {
			if ( mod != MOD_VENOM && attacker == inflictor && targ->health <= GIB_HEALTH ) {
				if ( targ->aiCharacter != AICHAR_ZOMBIE ) { // zombie needs to be able to gib so we can kill him (although he doesn't actually GIB, he just dies)
					targ->health = GIB_HEALTH + 1;
				}
			}
		}

		//G_Printf("health at: %d\n", targ->health);
		if ( targ->health <= 0 ) {
			//Coop_AddStats( targ, attacker, take, mod );
			if ( client ) {
				targ->flags |= FL_NO_KNOCKBACK;
			}

			if ( targ->health < -999 ) {
				targ->health = -999;
			}

			targ->enemy = attacker;
			G_KillOrEnterBleedout( targ, inflictor, attacker, take, mod );

			// if we freed ourselves in death function
			if ( !targ->inuse ) {
				return;
			}

			// RF, entity scripting
			if ( targ->s.number >= MAX_CLIENTS && targ->health <= 0 ) { // might have revived itself in death function
				G_Script_ScriptEvent( targ, "death", "" );
			}

		} else if ( targ->pain ) {
			targ->lastPainMOD = mod; // set mod for AICast_Pain to access

			if ( dir ) {  // Ridah, had to add this to fix NULL dir crash
				VectorCopy( dir, targ->rotate );
				VectorCopy( point, targ->pos3 ); // this will pass loc of hit
			} else {
				VectorClear( targ->rotate );
				VectorClear( targ->pos3 );
			}

			Coop_AddStats( targ, attacker, take, mod );
			targ->pain( targ, attacker, take, point );
			targ->lastPainMOD = 0; // optional: reset after use
		} else {
			Coop_AddStats( targ, attacker, take, mod );
			targ->lastPainMOD = 0; // optional: reset after use
		}

		if ( attacker->health <= 0 && attackerpain ) {
			Coop_AddStats( attacker, attacker, take, mod );
			if ( client ) {
				attacker->flags |= FL_NO_KNOCKBACK;
			}

			if ( attacker->health < -999 ) {
				attacker->health = -999;
			}

			attacker->enemy = attacker;
			G_KillOrEnterBleedout( attacker, inflictor, attacker, take, mod );

			// if we freed ourselves in death function
			if ( !attacker->inuse ) {
				return;
			}

			// RF, entity scripting
			if ( attacker->s.number >= MAX_CLIENTS && attacker->health <= 0 ) { // might have revived itself in death function
				G_Script_ScriptEvent( attacker, "death", "" );
			}
		}

		G_ArmorDamage( targ );    //----(SA)	moved out to separate routine

		// Ridah, this needs to be done last, incase the health is altered in one of the event calls
		if ( targ->client ) {
			targ->client->ps.stats[STAT_HEALTH] = targ->health;
		}
		if ( attacker->client ) {
			attacker->client->ps.stats[STAT_HEALTH] = attacker->health;
		}
	}

}


/*
============
CanDamage

Returns qtrue if the inflictor can directly damage the target.  Used for
explosions and melee attacks.
============
*/
qboolean CanDamage( gentity_t *targ, vec3_t origin ) {
	vec3_t dest;
	trace_t tr;
	vec3_t midpoint;
	vec3_t	offsetmins = {-15, -15, -15};
	vec3_t	offsetmaxs = {15, 15, 15};

	// use the midpoint of the bounds instead of the origin, because
	// bmodels may have their origin is 0,0,0
	VectorAdd( targ->r.absmin, targ->r.absmax, midpoint );
	VectorScale( midpoint, 0.5, midpoint );

	VectorCopy(midpoint, dest);
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if ( tr.fraction == 1.0 ) {
		return qtrue;
	}



	if ( &g_entities[tr.entityNum] == targ ) {
		return qtrue;
	}

	// this should probably check in the plane of projection,
	// rather than in world coordinate
	VectorCopy(midpoint, dest);
	dest[0] += offsetmaxs[0];
	dest[1] += offsetmaxs[1];
	dest[2] += offsetmaxs[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if (tr.fraction == 1.0)
		return qtrue;

	VectorCopy(midpoint, dest);
	dest[0] += offsetmaxs[0];
	dest[1] += offsetmins[1];
	dest[2] += offsetmaxs[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if ( tr.fraction == 1.0 ) {
		return qtrue;
	}

	VectorCopy(midpoint, dest);
	dest[0] += offsetmins[0];
	dest[1] += offsetmaxs[1];
	dest[2] += offsetmaxs[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if ( tr.fraction == 1.0 ) {
		return qtrue;
	}

	VectorCopy(midpoint, dest);
	dest[0] += offsetmins[0];
	dest[1] += offsetmins[1];
	dest[2] += offsetmaxs[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if ( tr.fraction == 1.0 ) {
		return qtrue;
	}

	VectorCopy(midpoint, dest);
	dest[0] += offsetmaxs[0];
	dest[1] += offsetmaxs[1];
	dest[2] += offsetmins[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if ( tr.fraction == 1.0 ) {
		return qtrue;
	}

	VectorCopy(midpoint, dest);
	dest[0] += offsetmaxs[0];
	dest[1] += offsetmins[1];
	dest[2] += offsetmins[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if (tr.fraction == 1.0)
		return qtrue;

	VectorCopy(midpoint, dest);
	dest[0] += offsetmins[0];
	dest[1] += offsetmaxs[1];
	dest[2] += offsetmins[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if (tr.fraction == 1.0)
		return qtrue;

	VectorCopy(midpoint, dest);
	dest[0] += offsetmins[0];
	dest[1] += offsetmins[1];
	dest[2] += offsetmins[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if (tr.fraction == 1.0)
		return qtrue;

	return qfalse;
}


/*
============
G_RadiusDamage
============
*/
qboolean G_RadiusDamage( vec3_t origin, gentity_t *attacker, float damage, float radius,
						 gentity_t *ignore, int mod ) {
	float points, dist;
	gentity_t   *ent;
	int entityList[MAX_GENTITIES];
	int numListedEntities;
	vec3_t mins, maxs;
	vec3_t v;
	vec3_t dir;
	int i, e;
	qboolean hitClient = qfalse;
// JPW NERVE
	float boxradius;
// jpw


	if ( radius < 1 ) {
		radius = 1;
	}

	boxradius = 1.41421356 * radius; // radius * sqrt(2) for bounding box enlargement --
	// bounding box was checking against radius / sqrt(2) if collision is along box plane
	for ( i = 0 ; i < 3 ; i++ ) {
		mins[i] = origin[i] - boxradius; // JPW NERVE
		maxs[i] = origin[i] + boxradius; // JPW NERVE
	}

	numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( e = 0 ; e < numListedEntities ; e++ ) {
		ent = &g_entities[entityList[ e ]];

		if ( ent == ignore ) {
			continue;
		}
		if ( !ent->takedamage ) {
			continue;
		}

/* JPW NERVE -- we can put this back if we need to, but it kinna sucks for human-sized bboxes
		// find the distance from the edge of the bounding box
		for ( i = 0 ; i < 3 ; i++ ) {
			if ( origin[i] < ent->r.absmin[i] ) {
				v[i] = ent->r.absmin[i] - origin[i];
			} else if ( origin[i] > ent->r.absmax[i] ) {
				v[i] = origin[i] - ent->r.absmax[i];
			} else {
				v[i] = 0;
			}
		}
*/
// JPW NERVE
		if ( !ent->r.bmodel ) {
			VectorSubtract( ent->r.currentOrigin,origin,v ); // JPW NERVE simpler centroid check that doesn't have box alignment weirdness
		} else {
			for ( i = 0 ; i < 3 ; i++ ) {
				if ( origin[i] < ent->r.absmin[i] ) {
					v[i] = ent->r.absmin[i] - origin[i];
				} else if ( origin[i] > ent->r.absmax[i] ) {
					v[i] = origin[i] - ent->r.absmax[i];
				} else {
					v[i] = 0;
				}
			}
		}
// jpw
		dist = VectorLength( v );

		if ( dist >= radius ) {
			continue;
		}

		points = damage * ( 1.0 - dist / radius );

// JPW NERVE -- different radiusdmg behavior for MP -- big explosions should do less damage (over less distance) through failed traces
		if ( CanDamage( ent, origin ) ) {
			if ( LogAccuracyHit( ent, attacker ) ) {
				hitClient = qtrue;
			}
			VectorSubtract( ent->r.currentOrigin, origin, dir );
			// push the center of mass higher than the origin so players
			// get knocked into the air more
			dir[2] += 24;

			// Reduce self-damage in Survival mode only
			if (ent == attacker && attacker->client)
			{
				if (mod == MOD_ROCKET_SPLASH && g_gametype.integer == GT_COOP_SURVIVAL)
				{
					points *= 0.3f;
				}
			}

			G_Damage( ent, NULL, attacker, dir, origin, (int)points, DAMAGE_RADIUS, mod );
		}
	}
	return hitClient;
}

