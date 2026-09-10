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
 * name:		ai_cast_survival.c
 *
 * desc:		Wolfenstein AI Character Survival
 *
*/

#include <stdlib.h> // For rand()
#include <stdio.h>  // For snprintf()

#include "g_local.h"
#include "../qcommon/q_shared.h"
#include "../botlib/botlib.h"      //bot lib interface
#include "../botlib/be_aas.h"
#include "../botlib/be_ea.h"
#include "../botlib/be_ai_gen.h"
#include "../botlib/be_ai_goal.h"
#include "../botlib/be_ai_move.h"
#include "../botlib/botai.h"          //bot ai interface

#include "ai_cast.h"
#include "g_survival.h"

#include "../steam/steam.h"

int BotPointAreaNum( vec3_t origin );  // ai_dmq3.c - handles floor-edge cases raw trap_AAS_PointAreaNum misses
void reinforce( gentity_t *ent );      // g_client.c - brings a limboed client back into the game

// Balance values below all come from survCfg (see g_survival.h / g_survival_config.c) - no more compile-time defines here.

/*
============
AICast_InitSurvival
============
*/
void AICast_InitSurvival(void) {
	Survival_LoadConfig();

	svParams.killCountRequirement = survCfg.initialKillCountReq;
	svParams.spawnedThisWave = 0;
	svParams.spawnedThisWaveFriendly = 0;
	svParams.waveCount = 0;
	svParams.waveInProgress = qfalse;
	svParams.wavePending = qtrue;
	svParams.waveChangeTime = level.time + survCfg.prepareTime * 1000;
	svParams.waveGameOver = qfalse;

	// reset explicitly - the DLL can stay loaded across map changes, so BSS zero-init alone isn't enough
	svParams.gameOverPhase = GAMEOVER_PHASE_NONE;
	svParams.gameOverPhaseTime = 0;
	svParams.gameOverCountdownShown = -1;
	VectorClear( svParams.gameOverCamOrigin );
	VectorClear( svParams.gameOverCamAngles );
	svParams.gameOverCamValid = qfalse;
	svParams.gameOverFadeInSent = qfalse;
	svParams.gameOverFadeOutSent = qfalse;
	svParams.gameOverMsgRefreshTime = 0;

	svParams.endingType = ENDING_NONE;
	svParams.exfilActive = qfalse;
	svParams.exfilRequireAll = qfalse;
	svParams.exfilStartTime = 0;
	svParams.exfilDuration = 0;
	svParams.exfilCountdownShown = -1;
	svParams.exfilWaitShown = -1;
	svParams.exfilPlayersInZone = 0;
	svParams.exfilBanner[0] = 0;

    // Special wave defaults
    svParams.specialWaveActive    = qfalse;
    svParams.lastSpecialWave      = 0;
    svParams.numSpecialWaveTypes  = 0;
    svParams.currentSpecialWaveType = AICHAR_LOPER_SPECIAL;

	svParams.maxActiveAI[AICHAR_SOLDIER] = survCfg.initialSoldiers;
	svParams.maxActiveAI[AICHAR_TRENCH] = survCfg.initialTrench;
	svParams.maxActiveAI[AICHAR_ZOMBIE_SURV] = survCfg.initialZombies;
	svParams.maxActiveAI[AICHAR_ZOMBIE_FLAME] = survCfg.initialFlamers;
	svParams.maxActiveAI[AICHAR_WARZOMBIE] = survCfg.initialWarriors;
	svParams.maxActiveAI[AICHAR_PROTOSOLDIER] = survCfg.initialProtos;
	svParams.maxActiveAI[AICHAR_PARTISAN] = 0; // auto-counted per placed friendly, see AICast_CreateCharacter_Survival
	svParams.maxActiveAI[AICHAR_ELITEGUARD] = survCfg.initialElites;
	svParams.maxActiveAI[AICHAR_BLACKGUARD] = survCfg.initialBlackguards;
	svParams.maxActiveAI[AICHAR_VENOM] = survCfg.initialVenoms;
	svParams.maxActiveAI[AICHAR_LOPER] = survCfg.initialLopers;
	svParams.maxActiveAI[AICHAR_HELGA] = survCfg.initialHelgas;
	svParams.maxActiveAI[AICHAR_HEINRICH] = survCfg.initialHeinrichs;
	svParams.maxActiveAI[AICHAR_PRIEST] = survCfg.initialPriests;
	svParams.maxActiveAI[AICHAR_XSHEPHERD] = survCfg.initialXshepherds;
}


/*
============
AICast_CreateCharacter_Survival

Applies Survival mode overrides after character creation
============
*/
void AICast_CreateCharacter_Survival(gentity_t *newent, cast_state_t *cs) {
    // If the character is friendly AI (aiTeam == 1), set respawnsleft to 0
    if (newent->aiTeam == 1) {
        cs->respawnsleft = 0;
        // Raise this character type's admission cap by one for each friendly actually
        // placed in the map, so the count needs no separate .surv config to match it.
        svParams.maxActiveAI[newent->aiCharacter]++;
    } else {
        // Unlimited respawn for other AI
        cs->respawnsleft = -1;
    }
	cs->registeredSurvivalKill = qfalse;
}


/*
============
AIChar_AIScript_AlertEntity_Survival

  triggered spawning, called from AI scripting
============
*/
void AIChar_AIScript_AlertEntity_Survival( gentity_t *ent ) {
	
	vec3_t mins, maxs;
	int numTouch, touch[10], i;
	cast_state_t    *cs;
	vec3_t spawn_origin, spawn_angles;

	gentity_t *player = AICast_FindEntityForName( "player" );

	if ( !ent->aiInactive ) {
		return;
	}

	cs = AICast_GetCastState( ent->s.number );

	if (svParams.specialWaveActive && ent->aiTeam != 1 && ent->aiCharacter != svParams.currentSpecialWaveType)
	{
		cs->aiFlags |= AIFL_WAITINGTOSPAWN;
		return;
	}

	if (!svParams.specialWaveActive && ent->aiTeam != 1 && AICast_IsSpecialWaveChar(ent->aiCharacter))
	{
		cs->aiFlags |= AIFL_WAITINGTOSPAWN;
		return;
	}

	// if the current bounding box is invalid, then wait
	VectorAdd( ent->r.currentOrigin, ent->r.mins, mins );
	VectorAdd( ent->r.currentOrigin, ent->r.maxs, maxs );
	trap_UnlinkEntity( ent );

	numTouch = trap_EntitiesInBox( mins, maxs, touch, 10 );

	// check that another client isn't inside us
	if ( numTouch ) {
		for ( i = 0; i < numTouch; i++ ) {
			// RF, note we should only check against clients since zombies need to spawn inside func_explosive (so they dont clip into view after it explodes)
			if ( g_entities[touch[i]].client && g_entities[touch[i]].r.contents == CONTENTS_BODY ) {
				//if (g_entities[touch[i]].r.contents & MASK_PLAYERSOLID)
				break;
			}
		}
		if ( i == numTouch ) {
			numTouch = 0;
		}
	}

	if ( numTouch ) {
		// invalid location
		cs->aiFlags |= AIFL_WAITINGTOSPAWN;
		return;
	}

    
	   if ( svParams.activeAI[ent->aiCharacter] >= (int)( svParams.maxActiveAI[ent->aiCharacter] * svParams.playerCountScale ) || !svParams.waveInProgress)  {
		cs->aiFlags |= AIFL_WAITINGTOSPAWN;
		return;
	   }

	    // Prevent friendly AI from spawning if respawnsleft is 0
		if (ent->aiTeam == 1 && cs->respawnsleft == 0) {
			cs->aiFlags |= AIFL_WAITINGTOSPAWN; // Mark as waiting to spawn
			return;
		}

	// Selecting the spawn point for the AI
				SelectSpawnPoint_AI( player, ent, spawn_origin, spawn_angles );
				G_SetOrigin( ent, spawn_origin );
				VectorCopy( spawn_origin, ent->client->ps.origin );
				SetClientViewAngle( ent, spawn_angles );
				// Increment the counter for active AI characters
                svParams.activeAI[ent->aiCharacter]++;

	// RF, has to disable this so I could test some maps which have erroneously placed alertentity calls
	//ent->AIScript_AlertEntity = NULL;
	cs->aiFlags &= ~AIFL_WAITINGTOSPAWN;
	ent->aiInactive = qfalse;

	ent->r.contents = CONTENTS_BODY;
	ent->clipmask = MASK_PLAYERSOLID | CONTENTS_MONSTERCLIP;
	if ( ent->aiTeam == AITEAM_ALLIES ) {
		ent->clipmask |= CONTENTS_FRIENDLYCLIP;
	}

	trap_LinkEntity( ent );

	// trigger a spawn script event
	AICast_ScriptEvent( AICast_GetCastState( ent->s.number ), "respawn", "" );

	// Increment spawned counters based on aiTeam
	if (ent->aiTeam == 1) { 
		svParams.spawnedThisWaveFriendly++;
	} else { 
		svParams.spawnedThisWave++;
	}

	// make it think so we update animations/angles
	AICast_Think( ent->s.number, (float)FRAMETIME / 1000 );
	cs->lastThink = level.time;
	AICast_UpdateInput( cs, FRAMETIME );
	trap_BotUserCommand( cs->bs->client, &( cs->lastucmd ) );
}


/*
===============
AICast_RegisterSurvivalKill

Handles wave and survival kill counters + progression trigger.
Call this from AICast_Die_Survival.
===============
*/
void AICast_RegisterSurvivalKill(gentity_t *self, gentity_t *attacker, int meansOfDeath) {
	
	if (!self)
	{
		Com_Printf("^1[AI_SURVIVE] ERROR: AICast_RegisterSurvivalKill called with NULL self\n");
		return;
	}

	// ===== GUARD AGAINST DOUBLE REGISTRATION =====
	cast_state_t* cs = AICast_GetCastState(self->s.number);
	if (cs->registeredSurvivalKill) {
		Com_Printf("^1[AI_SURVIVE] WARNING: Duplicate kill registration for %s (ent %d)\n",
			self->aiName ? self->aiName : "UNKNOWN", self->s.number);
		return;
	}
	cs->registeredSurvivalKill = qtrue;

	 // Skip counting if the dying entity is a friendly AI (aiTeam == 1)
    if (self->aiCharacter && self->aiTeam == 1) {
        //Com_Printf("^3[AI_SURVIVE] INFO: Friendly AI death ignored. aiCharacter=%d, aiTeam=%d\n", self->aiCharacter, self->aiTeam);
        return;
    }

	if (!svParams.waveInProgress){
		Com_Printf("^3[AI_SURVIVE] INFO: Wave not in progress, kill not counted. aiCharacter=%d, aiTeam=%d\n", self->aiCharacter, self->aiTeam);
		return;
	}

	qboolean killerPlayer   = attacker->client && !attacker->aiCharacter;
	qboolean killerFriendly = attacker->aiCharacter && attacker->aiTeam == 1;

if (!attacker) {
        Com_Printf("^1[AI_SURVIVE] WARNING: AI %s died with no attacker (meansOfDeath=%d)\n",
        self->aiName ? self->aiName : "UNKNOWN", meansOfDeath);
		svParams.waveKillCount++;
        AICast_CheckSurvivalProgression(&g_entities[0]);
        return;
}

if (!killerPlayer && !killerFriendly) {
   /* Com_Printf(
        "^3[AI_SURVIVE] WARNING: uncounted kill. attacker->client=%d, aiCharacter=%d, aiTeam=%d, meansOfDeath=%d\n",
        attacker->client != NULL,
        attacker->aiCharacter,
        attacker->aiTeam,
        meansOfDeath
    );*/

	svParams.waveKillCount++;
	AICast_CheckSurvivalProgression(&g_entities[0]);
	return;
}

	svParams.survivalKillCount++;
	svParams.waveKillCount++;

	if (killerPlayer) {
		Survival_AddKillScore(attacker, self, meansOfDeath);
	}

	// Always use attacker to trigger progression check
	AICast_CheckSurvivalProgression(attacker);

}


/*
====================
AICast_SetRebirthTimeSurvival

Centralized function for calculating AI rebirth time in Survival mode.
Handles friendly AI, and special cases per AI character if needed.
====================
*/
void AICast_SetRebirthTimeSurvival(gentity_t *ent, cast_state_t *cs) {
	if (!ent || !cs)
		return;

	// Skip characters that should never respawn
	if (ent->aiCharacter == AICHAR_ZOMBIE ||
		cs->norespawn)
		return;

	if (ent->aiTeam == AITEAM_ALLIES) { // Friendly AI
		if (cs->respawnsleft > 0) {
			cs->respawnsleft--;
		}
		svParams.spawnedThisWaveFriendly--;
		cs->rebirthTime = level.time + (survCfg.friendlySpawnTime * 1000) + rand() % 2000;
		return;
	}

	// Non-friendly AI
	if (cs->respawnsleft != 0) {
		if (cs->respawnsleft > 0) {
			cs->respawnsleft--;
		}

		int baseTime;

		switch (ent->aiCharacter) {
			case AICHAR_ELITEGUARD:
				baseTime = 5 * 1000;
				break;
			case AICHAR_HELGA:
			    baseTime = 350 * 1000;
				break;
			case AICHAR_HEINRICH:
			    baseTime = 500 * 1000;
				break;
			case AICHAR_BLACKGUARD:
				baseTime = 5 * 1000;
				break;
			case AICHAR_VENOM:
				baseTime = 60 * 1000;
				break;
			case AICHAR_PROTOSOLDIER:
				baseTime = 120 * 1000;
				break;
			case AICHAR_WARZOMBIE:
				baseTime = 5 * 1000;
				break;
			case AICHAR_LOPER:
				baseTime = 5 * 1000;
				break;
			case AICHAR_PRIEST:
				baseTime = 20 * 1000;
				break;
			case AICHAR_XSHEPHERD:
				baseTime = 8 * 1000;
				break;
			default: // Regular soldiers and zombies
				baseTime = 5 * 1000;
				break;
		}

		baseTime += rand() % 2000; // slight randomness

		cs->rebirthTime = level.time + baseTime;
	}
}

/*
============
AICast_Die_Survival
============
*/
void AICast_Die_Survival( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath ) {
	int contents;
	int killer = 0;
	cast_state_t    *cs;
	qboolean nogib = qtrue;

	qboolean killerPlayer	 = attacker && attacker->client && !( attacker->aiCharacter );
	
	
	  if (killerPlayer && attacker->client->ps.powerups[PW_VAMPIRE]) {

			trap_SendServerCommand( -1, "mu_play sound/Zombie/firstsight/firstsight3.wav 0\n" );
		    attacker->health += 25;
		
			if ( attacker->health > 300 ) 
			{
			attacker->health = 300;
		    }

	  }

	// print debugging message
	if ( aicast_debug.integer == 2 && attacker->s.number == 0 ) {
		G_Printf( "killed %s\n", self->aiName );
	}

    AICast_RegisterSurvivalKill(self, attacker, meansOfDeath);

	cs = AICast_GetCastState( self->s.number );

	if ( attacker ) {
		killer = attacker->s.number;
	} else {
		killer = ENTITYNUM_WORLD;
	}

	// record the sighting (FIXME: silent weapons shouldn't do this, but the AI should react in some way)
	if ( attacker && attacker->client ) {
		AICast_UpdateVisibility( self, attacker, qtrue, qtrue );
	}

	if ( self->aiCharacter == AICHAR_HEINRICH || self->aiCharacter == AICHAR_HELGA || self->aiCharacter == AICHAR_SUPERSOLDIER || self->aiCharacter == AICHAR_PROTOSOLDIER ) {
		if ( self->health <= GIB_HEALTH ) {
			self->health = -1;
		}
	}

	// process the event
	if ( self->client->ps.pm_type == PM_DEAD ) {
		// already dead
		if ( self->health < GIB_HEALTH ) {
			if ( self->aiCharacter == AICHAR_ZOMBIE || self->aiCharacter == AICHAR_ZOMBIE_SURV || self->aiCharacter == AICHAR_ZOMBIE_FLAME || self->aiCharacter == AICHAR_ZOMBIE_GHOST  ) {
				// RF, changed this so Zombies always gib now
				GibEntity( self, killer );
				nogib = qfalse;
				self->takedamage = qfalse;
				self->r.contents = 0;
				cs->secondDeadTime = 2;
				cs->rebirthTime = 0;
				cs->revivingTime = 0;
			} else {
				body_die( self, inflictor, attacker, damage, meansOfDeath );
				return;
			}
		}

	} else {    // this is our first death, so set everything up

		if ( level.intermissiontime ) {
			return;
		}

		self->client->ps.pm_type = PM_DEAD;

		self->enemy = attacker;

		// drop a weapon?
		// if client is in a nodrop area, don't drop anything
		contents = trap_PointContents( self->r.currentOrigin, -1 );
		if ( !( contents & CONTENTS_NODROP ) ) {
			TossClientWeapons( self );
			TossClientItems_Survival( self, attacker );
			TossClientPowerups( self, attacker );
		}

		// make sure the client doesn't forget about this entity until it's set to "dead" frame
		// otherwise it might replay it's death animation if it goes out and into client view
		self->r.svFlags |= SVF_BROADCAST;

		self->takedamage = qtrue;   // can still be gibbed

		self->s.weapon = WP_NONE;
		if ( cs->bs ) {
			cs->weaponNum = WP_NONE;
		}
		self->client->ps.weapon = WP_NONE;

		self->s.powerups = 0;
		self->r.contents = CONTENTS_CORPSE;

		self->s.angles[0] = 0;
		self->s.angles[1] = self->client->ps.viewangles[1];
		self->s.angles[2] = 0;

		VectorCopy( self->s.angles, self->client->ps.viewangles );

		self->s.loopSound = 0;

		self->r.maxs[2] = -8;
		self->client->ps.maxs[2] = self->r.maxs[2];

		// remove powerups
		memset( self->client->ps.powerups, 0, sizeof( self->client->ps.powerups ) );

		// never gib in a nodrop
		if ( self->health <= GIB_HEALTH ) {
			if ( self->aiCharacter == AICHAR_ZOMBIE || self->aiCharacter == AICHAR_ZOMBIE_SURV || self->aiCharacter == AICHAR_ZOMBIE_FLAME || self->aiCharacter == AICHAR_ZOMBIE_GHOST  ) {
				// RF, changed this so Zombies always gib now
				GibEntity( self, killer );
				nogib = qfalse;
			} else if ( !( contents & CONTENTS_NODROP ) ) {
				body_die( self, inflictor, attacker, damage, meansOfDeath );
				nogib = qfalse;
			}
		}

		// set enemy weapon
		BG_UpdateConditionValue(self->s.number, ANIM_COND_ENEMY_WEAPON, 0, qfalse);
		if (attacker && attacker->client)
		{
			BG_UpdateConditionValue(self->s.number, ANIM_COND_ENEMY_WEAPON, inflictor->s.weapon, qtrue);
		}
		else if (meansOfDeath == MOD_FLAMETHROWER || meansOfDeath == MOD_FLAMETRAP)
		{
			// non-client fire sources (props_flamethrower, flamechunks) still get the burn death anim
			BG_UpdateConditionValue(self->s.number, ANIM_COND_ENEMY_WEAPON, WP_FLAMETHROWER, qtrue);
		}
		else
		{
			BG_UpdateConditionValue(self->s.number, ANIM_COND_ENEMY_WEAPON, 0, qfalse);
		}

		// set enemy location
		BG_UpdateConditionValue(self->s.number, ANIM_COND_ENEMY_POSITION, 0, qfalse);
		if (infront(self, inflictor))
		{
			BG_UpdateConditionValue(self->s.number, ANIM_COND_ENEMY_POSITION, POSITION_INFRONT, qtrue);
		}
		else
		{
			BG_UpdateConditionValue(self->s.number, ANIM_COND_ENEMY_POSITION, POSITION_BEHIND, qtrue);
		}

		if (self->takedamage)
		{ // only play the anim if we haven't gibbed
			// play the animation
			BG_AnimScriptEvent(&self->client->ps, ANIM_ET_DEATH, qfalse, qtrue);
		}

		// set gib delay
		if (cs->aiCharacter == AICHAR_HEINRICH || cs->aiCharacter == AICHAR_HELGA)
		{
			cs->lastLoadTime = level.time + self->client->ps.torsoTimer - 200;
		}

		// set this flag so no other anims override us
		self->client->ps.eFlags |= EF_DEAD;
		self->s.eFlags |= EF_DEAD;

		cs->deadSinkStartTime = 0;
	}

	if ( nogib ) {
		// the body can still be gibbed
		self->die = body_die;
	}

	nogib = qtrue;

    AICast_SetRebirthTimeSurvival(self, cs);

	trap_LinkEntity( self );

	// kill, instanly, any streaming sound the character had going
	G_AddEvent( &g_entities[self->s.number], EV_STOPSTREAMINGSOUND, 0 );

	// mark the time of death
	cs->deathTime = level.time;

	// really dead now, so call the script
	if (self->aiCharacter != AICHAR_ZOMBIE && self->aiCharacter != AICHAR_HELGA && self->aiCharacter != AICHAR_HEINRICH && nogib && !cs->norespawn)
	{

		if (!cs->died)
		{
			G_UseTargets(self, self); // testing
			AICast_ScriptEvent(cs, "death", "");
			cs->died = qtrue;
		}
	}
	else
	{
		AICast_ScriptEvent(cs, "fakedeath", "");
	}
	// call the deathfunc for this cast, so we can play associated sounds, or do any character-specific things
	if (!(cs->aiFlags & AIFL_DENYACTION) && cs->deathfunc)
	{
		cs->deathfunc(self, attacker, damage, meansOfDeath);
	}
}

/*
============
AICast_UpdateMaxActiveAI
  Updates the maximum number of active AI characters based on the current wave count.
  This function is called at the start of each wave to adjust the limits for each character type.
  The limits are defined in svParams and are adjusted according to the wave count.
  The function ensures that the number of active AI characters does not exceed the defined maximums.
============
*/
void AICast_UpdateMaxActiveAI(void)
{
    // Normal soldiers
    svParams.maxActiveAI[AICHAR_SOLDIER] += survCfg.soldiersIncrease;
    if (svParams.maxActiveAI[AICHAR_SOLDIER] > survCfg.maxSoldiers) {
        svParams.maxActiveAI[AICHAR_SOLDIER] = survCfg.maxSoldiers;
    }

    // Trench
    svParams.maxActiveAI[AICHAR_TRENCH] += survCfg.trenchIncrease;
    if (svParams.maxActiveAI[AICHAR_TRENCH] > survCfg.maxTrench) {
        svParams.maxActiveAI[AICHAR_TRENCH] = survCfg.maxTrench;
    }

    // Elite Guards
    if (svParams.waveCount >= survCfg.waveEliteGuard) {
        svParams.maxActiveAI[AICHAR_ELITEGUARD] += survCfg.elitesIncrease;
        if (svParams.maxActiveAI[AICHAR_ELITEGUARD] > survCfg.maxElites) {
            svParams.maxActiveAI[AICHAR_ELITEGUARD] = survCfg.maxElites;
        }

	}

	if (svParams.waveCount >= survCfg.waveHelga)
	{
		svParams.maxActiveAI[AICHAR_HELGA] += survCfg.helgasIncrease;
		if (svParams.maxActiveAI[AICHAR_HELGA] > survCfg.maxHelgas)
		{
			svParams.maxActiveAI[AICHAR_HELGA] = survCfg.maxHelgas;
		}
	}

	if (svParams.waveCount >= survCfg.waveHeinrich)
	{
		svParams.maxActiveAI[AICHAR_HEINRICH] += survCfg.heinrichsIncrease;
		if (svParams.maxActiveAI[AICHAR_HEINRICH] > survCfg.maxHeinrichs)
		{
			svParams.maxActiveAI[AICHAR_HEINRICH] = survCfg.maxHeinrichs;
		}
	}

	// Black Guards
    if (svParams.waveCount >= survCfg.waveBlackguard) {
        svParams.maxActiveAI[AICHAR_BLACKGUARD] += survCfg.blackguardsIncrease;
        if (svParams.maxActiveAI[AICHAR_BLACKGUARD] > survCfg.maxBlackguards) {
            svParams.maxActiveAI[AICHAR_BLACKGUARD] = survCfg.maxBlackguards;
        }
    }

    // Venoms
    if (svParams.waveCount >= survCfg.waveVenom) {
        svParams.maxActiveAI[AICHAR_VENOM] += survCfg.venomsIncrease;
        if (svParams.maxActiveAI[AICHAR_VENOM] > survCfg.maxVenoms) {
            svParams.maxActiveAI[AICHAR_VENOM] = survCfg.maxVenoms;
        }
    }

    // Default Zombies
    svParams.maxActiveAI[AICHAR_ZOMBIE_SURV] += survCfg.zombiesIncrease;
    if (svParams.maxActiveAI[AICHAR_ZOMBIE_SURV] > survCfg.maxZombies) {
        svParams.maxActiveAI[AICHAR_ZOMBIE_SURV] = survCfg.maxZombies;
    }

    // Flamer Zombies
    if (svParams.waveCount >= survCfg.waveFlamers) {
        svParams.maxActiveAI[AICHAR_ZOMBIE_FLAME] += survCfg.flamersIncrease;
        if (svParams.maxActiveAI[AICHAR_ZOMBIE_FLAME] > survCfg.maxFlamers) {
            svParams.maxActiveAI[AICHAR_ZOMBIE_FLAME] = survCfg.maxFlamers;
        }
    }


    // Warriors
    if (svParams.waveCount >= survCfg.waveWarriors) {
        svParams.maxActiveAI[AICHAR_WARZOMBIE] += survCfg.warriorsIncrease;
        if (svParams.maxActiveAI[AICHAR_WARZOMBIE] > survCfg.maxWarriors) {
            svParams.maxActiveAI[AICHAR_WARZOMBIE] = survCfg.maxWarriors;
        }
    }

    // Protos
    if (svParams.waveCount >= survCfg.waveProto) {
        svParams.maxActiveAI[AICHAR_PROTOSOLDIER] += survCfg.protosIncrease;
        if (svParams.maxActiveAI[AICHAR_PROTOSOLDIER] > survCfg.maxProtos) {
            svParams.maxActiveAI[AICHAR_PROTOSOLDIER] = survCfg.maxProtos;
        }
    }

	if (svParams.waveCount >= survCfg.waveLopers) {
        svParams.maxActiveAI[AICHAR_LOPER] += survCfg.lopersIncrease;
        if (svParams.maxActiveAI[AICHAR_LOPER] > survCfg.maxLopers) {
            svParams.maxActiveAI[AICHAR_LOPER] = survCfg.maxLopers;
        }
    }

    // Priests
    if (svParams.waveCount >= survCfg.wavePriests) {
        svParams.maxActiveAI[AICHAR_PRIEST] += survCfg.priestsIncrease;
        if (svParams.maxActiveAI[AICHAR_PRIEST] > survCfg.maxPriests) {
            svParams.maxActiveAI[AICHAR_PRIEST] = survCfg.maxPriests;
        }
    }

    // X-Shepherds
    if (svParams.waveCount >= survCfg.waveXshepherds) {
        svParams.maxActiveAI[AICHAR_XSHEPHERD] += survCfg.xshepherdsIncrease;
        if (svParams.maxActiveAI[AICHAR_XSHEPHERD] > survCfg.maxXshepherds) {
            svParams.maxActiveAI[AICHAR_XSHEPHERD] = survCfg.maxXshepherds;
        }
    }
}

/*
============
AICast_ApplySurvivalAttributes
  Applies survival mode attributes to the AI character based on the current wave count.
  The function adjusts health, speed scales, and other attributes based on the character type and wave count.
  This function is called when a new wave starts or when an AI character is spawned.
  The adjustments are made to ensure that the AI characters become progressively stronger as the game progresses.
============
*/
void AICast_ApplySurvivalAttributes(gentity_t *ent, cast_state_t *cs)
{
	int waveAppeared = 0;
	switch (cs->aiCharacter)
	{
	case AICHAR_ELITEGUARD:
		waveAppeared = survCfg.waveEliteGuard;
		break;
	case AICHAR_HEINRICH:
		waveAppeared = survCfg.waveHeinrich;
		break;
	case AICHAR_HELGA:
		waveAppeared = survCfg.waveHelga;
		break;
	case AICHAR_BLACKGUARD:
		waveAppeared = survCfg.waveBlackguard;
		break;
	case AICHAR_VENOM:
		waveAppeared = survCfg.waveVenom;
		break;
	case AICHAR_WARZOMBIE:
		waveAppeared = survCfg.waveWarriors;
		break;
	case AICHAR_ZOMBIE_FLAME:
		waveAppeared = survCfg.waveFlamers;
		break;
	case AICHAR_PROTOSOLDIER:
		waveAppeared = survCfg.waveProto;
		break;
	case AICHAR_LOPER:
		waveAppeared = survCfg.waveLopers;
		break;
	case AICHAR_PRIEST:
		waveAppeared = survCfg.wavePriests;
		break;
	case AICHAR_XSHEPHERD:
		waveAppeared = survCfg.waveXshepherds;
		break;
	case AICHAR_TRENCH:
		waveAppeared = survCfg.waveTrench;
		break;
	case AICHAR_SOLDIER:
	case AICHAR_ZOMBIE_SURV:
	    waveAppeared = 1;
	default:
		waveAppeared = 0;
		break;
	}

	int rawSteps = (svParams.waveCount > 1) ? (svParams.waveCount - waveAppeared) : 0;
	if (rawSteps < 0)
		rawSteps = 0;

	if (g_survivalDifficulty.integer == 1)
	{
		if (svParams.waveCount < survCfg.growthCurveWaveThreshold)
		{
			rawSteps += survCfg.hardStepsBonusBaseEarly + rawSteps / survCfg.hardStepsBonusDivisorEarly;
		}
		else
		{
			rawSteps += survCfg.hardStepsBonusBaseLate + rawSteps / survCfg.hardStepsBonusDivisorLate;
		}

	}


	int newHealth = 0;
	float runSpeedScale = 1.0f;
	float sprintSpeedScale = 1.0f;
	float crouchSpeedScale = 1.0f;

	// cc holds this character's tunable curve; the case labels below just decide who gets health/speed scaling at all, matching the original switch 1:1.
	const survCharCurve_t *cc = &survCfg.charCurve[cs->aiCharacter];
	int growthOffset = survCfg.growthCurveWaveThreshold - 1;

	switch (cs->aiCharacter) {
		case AICHAR_SOLDIER:
		case AICHAR_TRENCH:
			if (svParams.waveCount < survCfg.growthCurveWaveThreshold)
			{
				newHealth = cc->healthBaseEarly + rawSteps * cc->healthPerStep;
			}
			else
			{
				float growth = powf(cc->healthGrowthRate, (float)(rawSteps - growthOffset));
				newHealth = (int)((cc->healthGrowthBase + growthOffset * cc->healthPerStep) * growth);
			}
			if (g_survivalAiHealthCap.integer == 1)
			{
				if (newHealth > cc->healthCap) newHealth = cc->healthCap;
			}
			break;

		case AICHAR_ELITEGUARD:
			if (svParams.waveCount < survCfg.growthCurveWaveThreshold)
			{
				newHealth = cc->healthBaseEarly + rawSteps * cc->healthPerStep;
			}
			else
			{
				float growth = powf(cc->healthGrowthRate, (float)(rawSteps - growthOffset));
				newHealth = (int)((cc->healthGrowthBase + growthOffset * cc->healthPerStep) * growth);
			}
			if (g_survivalAiHealthCap.integer == 1)
			{
			if (newHealth > cc->healthCap) newHealth = cc->healthCap;
			}
			break;

		case AICHAR_HELGA:
			if (svParams.waveCount < survCfg.growthCurveWaveThreshold)
			{
				newHealth = cc->healthBaseEarly + rawSteps * cc->healthPerStep;
			}
			else
			{
				float growth = powf(cc->healthGrowthRate, (float)(rawSteps - growthOffset));
				newHealth = (int)((cc->healthGrowthBase + growthOffset * cc->healthPerStep) * growth);
			}
			if (g_survivalAiHealthCap.integer == 1)
			{
			if (newHealth > cc->healthCap) newHealth = cc->healthCap;
			}
			break;


		case AICHAR_HEINRICH:
			if (svParams.waveCount < survCfg.growthCurveWaveThreshold)
			{
				newHealth = cc->healthBaseEarly + rawSteps * cc->healthPerStep;
			}
			else
			{
				float growth = powf(cc->healthGrowthRate, (float)(rawSteps - growthOffset));
				newHealth = (int)((cc->healthGrowthBase + growthOffset * cc->healthPerStep) * growth);
			}
			if (g_survivalAiHealthCap.integer == 1)
			{
			if (newHealth > cc->healthCap) newHealth = cc->healthCap;
			}
			break;

		case AICHAR_BLACKGUARD:
			if (svParams.waveCount < survCfg.growthCurveWaveThreshold)
			{
				newHealth = cc->healthBaseEarly + rawSteps * cc->healthPerStep;
			}
			else
			{
				float growth = powf(cc->healthGrowthRate, (float)(rawSteps - growthOffset));
				newHealth = (int)((cc->healthGrowthBase + growthOffset * cc->healthPerStep) * growth);
			}
			if (g_survivalAiHealthCap.integer == 1)
			{
			if (newHealth > cc->healthCap) newHealth = cc->healthCap;
			}
			break;

		case AICHAR_VENOM:
			if (svParams.waveCount < survCfg.growthCurveWaveThreshold)
			{
				newHealth = cc->healthBaseEarly + rawSteps * cc->healthPerStep;
			}
			else
			{
				float growth = powf(cc->healthGrowthRate, (float)(rawSteps - growthOffset));
				newHealth = (int)((cc->healthGrowthBase + growthOffset * cc->healthPerStep) * growth);
			}
			if (g_survivalAiHealthCap.integer == 1)
			{
			if (newHealth > cc->healthCap) newHealth = cc->healthCap;
			}
			break;

		case AICHAR_PRIEST:
		case AICHAR_XSHEPHERD:
			if (svParams.waveCount < survCfg.growthCurveWaveThreshold)
			{
				newHealth = cc->healthBaseEarly + rawSteps * cc->healthPerStep;
			}
			else
			{
				float growth = powf(cc->healthGrowthRate, (float)(rawSteps - growthOffset));
				newHealth = (int)((cc->healthGrowthBase + growthOffset * cc->healthPerStep) * growth);
			}
			if (g_survivalAiHealthCap.integer == 1)
			{
			if (newHealth > cc->healthCap) newHealth = cc->healthCap;
			}
			break;

		case AICHAR_ZOMBIE_SURV:
		case AICHAR_ZOMBIE_FLAME:
			if (svParams.waveCount < survCfg.growthCurveWaveThreshold)
			{
				newHealth = cc->healthBaseEarly + rawSteps * cc->healthPerStep;
			}
			else
			{
				float growth = powf(cc->healthGrowthRate, (float)(rawSteps - growthOffset));
				newHealth = (int)((cc->healthGrowthBase + growthOffset * cc->healthPerStep) * growth);
			}
			if (g_survivalAiHealthCap.integer == 1)
			{
			if (newHealth > cc->healthCap) newHealth = cc->healthCap;
			}
			runSpeedScale    = fminf(cc->runSpeedBase + rawSteps * cc->runSpeedPerStep, cc->runSpeedCap);
			sprintSpeedScale = fminf(cc->sprintSpeedBase + rawSteps * cc->sprintSpeedPerStep, cc->sprintSpeedCap);
			crouchSpeedScale = fminf(cc->crouchSpeedBase + rawSteps * cc->crouchSpeedPerStep, cc->crouchSpeedCap);
			break;
		case AICHAR_ZOMBIE_GHOST:
			if (svParams.waveCount < survCfg.growthCurveWaveThreshold)
			{
				newHealth = cc->healthBaseEarly + rawSteps * cc->healthPerStep;
			}
			else
			{
				float growth = powf(cc->healthGrowthRate, (float)(rawSteps - growthOffset));
				newHealth = (int)((cc->healthGrowthBase + growthOffset * cc->healthPerStep) * growth);
			}
			if (g_survivalAiHealthCap.integer == 1)
			{
			if (newHealth > cc->healthCap) newHealth = cc->healthCap;
			}
			runSpeedScale    = fminf(cc->runSpeedBase + rawSteps * cc->runSpeedPerStep, cc->runSpeedCap);
			sprintSpeedScale = fminf(cc->sprintSpeedBase + rawSteps * cc->sprintSpeedPerStep, cc->sprintSpeedCap);
			crouchSpeedScale = fminf(cc->crouchSpeedBase + rawSteps * cc->crouchSpeedPerStep, cc->crouchSpeedCap);
			break;

		case AICHAR_WARZOMBIE:
			if (svParams.waveCount < survCfg.growthCurveWaveThreshold)
			{
				newHealth = cc->healthBaseEarly + rawSteps * cc->healthPerStep;
			}
			else
			{
				float growth = powf(cc->healthGrowthRate, (float)(rawSteps - growthOffset));
				newHealth = (int)((cc->healthGrowthBase + growthOffset * cc->healthPerStep) * growth);
			}
			if (g_survivalAiHealthCap.integer == 1)
			{
			if (newHealth > cc->healthCap) newHealth = cc->healthCap;
			}
			runSpeedScale    = fminf(cc->runSpeedBase + rawSteps * cc->runSpeedPerStep, cc->runSpeedCap);
			sprintSpeedScale = fminf(cc->sprintSpeedBase + rawSteps * cc->sprintSpeedPerStep, cc->sprintSpeedCap);
			crouchSpeedScale = fminf(cc->crouchSpeedBase + rawSteps * cc->crouchSpeedPerStep, cc->crouchSpeedCap);
			break;

		case AICHAR_PROTOSOLDIER:
			if (svParams.waveCount < survCfg.growthCurveWaveThreshold)
			{
				newHealth = cc->healthBaseEarly + rawSteps * cc->healthPerStep;
			}
			else
			{
				float growth = powf(cc->healthGrowthRate, (float)(rawSteps - growthOffset));
				newHealth = (int)((cc->healthGrowthBase + growthOffset * cc->healthPerStep) * growth);
			}
			if (g_survivalAiHealthCap.integer == 1)
			{
			if (newHealth > cc->healthCap) newHealth = cc->healthCap;
			}
			runSpeedScale    = fminf(cc->runSpeedBase + rawSteps * cc->runSpeedPerStep, cc->runSpeedCap);
			sprintSpeedScale = fminf(cc->sprintSpeedBase + rawSteps * cc->sprintSpeedPerStep, cc->sprintSpeedCap);
			crouchSpeedScale = fminf(cc->crouchSpeedBase + rawSteps * cc->crouchSpeedPerStep, cc->crouchSpeedCap);
			break;

		case AICHAR_PARTISAN:
			if (svParams.waveCount < survCfg.growthCurveWaveThreshold)
			{
				newHealth = cc->healthBaseEarly + rawSteps * cc->healthPerStep;
			}
			else
			{
				float growth = powf(cc->healthGrowthRate, (float)(rawSteps - growthOffset));
				newHealth = (int)((cc->healthGrowthBase + growthOffset * cc->healthPerStep) * growth);
			}
			if (g_survivalAiHealthCap.integer == 1)
			{
			if (newHealth > cc->healthCap) newHealth = cc->healthCap;
			}
			break;

		case AICHAR_LOPER:
			if (svParams.waveCount < survCfg.growthCurveWaveThreshold)
			{
				newHealth = cc->healthBaseEarly + rawSteps * cc->healthPerStep;
			}
			else
			{
				float growth = powf(cc->healthGrowthRate, (float)(rawSteps - growthOffset));
				newHealth = (int)((cc->healthGrowthBase + growthOffset * cc->healthPerStep) * growth);
			}
			if (g_survivalAiHealthCap.integer == 1)
			{
			if (newHealth > cc->healthCap) newHealth = cc->healthCap;
			}
			break;

		case AICHAR_LOPER_SPECIAL:
			if (svParams.waveCount < survCfg.growthCurveWaveThreshold)
			{
				newHealth = cc->healthBaseEarly + rawSteps * cc->healthPerStep;
			}
			else
			{
				float growth = powf(cc->healthGrowthRate, (float)(rawSteps - growthOffset));
				newHealth = (int)((cc->healthGrowthBase + growthOffset * cc->healthPerStep) * growth);
			}
			if (g_survivalAiHealthCap.integer == 1)
			{
			if (newHealth > cc->healthCap) newHealth = cc->healthCap;
			}
			break;

		default:
			break;
	}

	// Apply to entity
	ent->health = ent->client->ps.stats[STAT_HEALTH] = ent->client->ps.stats[STAT_MAX_HEALTH] = cs->attributes[STARTING_HEALTH] = newHealth;
	ent->client->ps.runSpeedScale    = runSpeedScale;
	ent->client->ps.sprintSpeedScale = sprintSpeedScale;
	ent->client->ps.crouchSpeedScale = crouchSpeedScale;
}

/*
============
BG_SetBehaviorForSurvival
  Sets the behavior attributes for AI characters in survival mode.
  The function adjusts the aim skill, accuracy, attack skill, aggression, and reaction time
  based on the current wave count. The values are capped to ensure they do not exceed defined limits.
  This function is called at the start of each wave to adjust the AI behavior dynamically.
============
*/
void BG_SetBehaviorForSurvival(AICharacters_t characterNum) {
	int waveAppeared = 1;
	switch (characterNum) {
		case AICHAR_ELITEGUARD:   waveAppeared = survCfg.waveEliteGuard; break;
		case AICHAR_BLACKGUARD:   waveAppeared = survCfg.waveBlackguard; break;
		case AICHAR_VENOM:        waveAppeared = survCfg.waveVenom; break;
		case AICHAR_PROTOSOLDIER: waveAppeared = survCfg.waveProto; break;
		case AICHAR_SOLDIER: waveAppeared = 1; break;
		case AICHAR_TRENCH: waveAppeared = survCfg.waveTrench; break;
		default:  waveAppeared = 0; break;
	}

	int rawSteps = (svParams.waveCount > 1) ? (svParams.waveCount - waveAppeared) : 0;
	if (rawSteps < 0) rawSteps = 0;

	float delta = rawSteps * 0.05f;

	float aimSkill     = 0.0f;
	float aimAccuracy  = 0.0f;
	float attackSkill  = 0.0f;
	float aggression   = 0.0f;
	float reactionTime = 1.0f;

	switch (characterNum) {
		case AICHAR_SOLDIER:
		case AICHAR_TRENCH:
		case AICHAR_ELITEGUARD:
		case AICHAR_BLACKGUARD:
		case AICHAR_VENOM:
		case AICHAR_PROTOSOLDIER:
			{
				const survCharSkillTier_t *st = (g_survivalDifficulty.integer == 1)
					? &survCfg.charSkill[characterNum].hard
					: &survCfg.charSkill[characterNum].easy;

				aimSkill     = fminf(st->aimSkillBase + delta, st->aimSkillCap);
				aimAccuracy  = fminf(st->aimAccuracyBase + delta, st->aimAccuracyCap);
				attackSkill  = fminf(st->attackSkillBase + delta, st->attackSkillCap);
				aggression   = fminf(st->aggressionBase + delta, st->aggressionCap);
				reactionTime = fmaxf(st->reactionTimeBase - delta, st->reactionTimeFloor);
			}
			break;
		case AICHAR_PARTISAN:
			aimSkill     = 0.9f;
			aimAccuracy  = 0.9f;
			attackSkill  = 0.9f;
			aggression   = 0.9f;
			reactionTime = 0.1f;
			break;
		case AICHAR_ZOMBIE_SURV:
		case AICHAR_ZOMBIE_FLAME:
		case AICHAR_ZOMBIE_GHOST:
		case AICHAR_WARZOMBIE:
		case AICHAR_LOPER:
		case AICHAR_LOPER_SPECIAL:
		case AICHAR_HELGA:
		case AICHAR_HEINRICH:
		case AICHAR_PRIEST:
		case AICHAR_XSHEPHERD:
			aimSkill     = 1.0f;
			aimAccuracy  = 1.0f;
			attackSkill  = 1.0f;
			aggression   = 1.0f;
			reactionTime = 0.5f;
			break;
		default:
			return;
	}

	aiDefaults[characterNum].attributes[AIM_SKILL]     = aimSkill;
	aiDefaults[characterNum].attributes[AIM_ACCURACY]  = aimAccuracy;
	aiDefaults[characterNum].attributes[ATTACK_SKILL]  = attackSkill;
	aiDefaults[characterNum].attributes[AGGRESSION]    = aggression;
	aiDefaults[characterNum].attributes[REACTION_TIME] = reactionTime;
}

static void Survival_GameManagerEvent( const char *event ) {
	gentity_t *gm;
	char eventBuffer[MAX_QPATH];
	char emptyParams[] = "";

	gm = G_Find( NULL, FOFS( scriptName ), "game_manager" );
	if ( !gm ) {
		G_Printf( "Survival: ERROR no game_manager found for event '%s'\n", event );
		return;
	}

	Q_strncpyz( eventBuffer, event, sizeof( eventBuffer ) );

	G_Printf( "Survival: game_manager event -> %s\n", eventBuffer );

	G_Script_ScriptEvent( gm, eventBuffer, emptyParams );
}


static qboolean AICast_ShouldStartSpecialWave(void) {
    // dev/test override via "forcespecialwave" client command, bypasses all
    // normal gating below (cooldown, min-start, g_specialWaves toggle). One-shot.
    if (svParams.forceSpecialWaveNext) {
        svParams.forceSpecialWaveNext = qfalse;
        return qtrue;
    }

    // 0 = disabled → never start special waves
    if (g_specialWaves.integer == 0 || survCfg.specialWaveChance <= 0)
        return qfalse;

    int wave = svParams.waveCount; // wave we’re starting now

    // Too early
    if (wave < survCfg.specialWaveMinStart)
        return qfalse;

    // Enforce minimal gap after a special (cooldown)
    if (svParams.lastSpecialWave > 0) {
        int delta = wave - svParams.lastSpecialWave; // distance to previous special
        if (delta <= survCfg.specialWaveMinGap)
            return qfalse; // still cooling down
    }

    // How many full NON-special waves since last special?
    // If none yet, count since first eligible (specialWaveMinStart).
    int gapSince = (svParams.lastSpecialWave > 0)
        ? (wave - svParams.lastSpecialWave - 1)
        : (wave - survCfg.specialWaveMinStart);

    // Hard guarantee: if we waited long enough, force a special
    if (gapSince >= survCfg.specialWaveMaxGap)
        return qtrue;

    // Otherwise randomized
    int roll = rand() % 100; // 0..99
    return (roll < survCfg.specialWaveChance) ? qtrue : qfalse;
}

/*
============
AICast_IsSpecialWaveChar

  Is this AICharacters_t inherently a special-wave-only enemy (regardless of
  whether the current map has it selected in its specialWaveTypes pool)?
============
*/
qboolean AICast_IsSpecialWaveChar( int aiChar ) {
	switch ( aiChar ) {
	case AICHAR_LOPER_SPECIAL:
	case AICHAR_ZOMBIE_GHOST:
		return qtrue;
	default:
		return qfalse;
	}
}

typedef struct {
	const char *name;
	AICharacters_t type;
} specialWaveTypeName_t;

static const specialWaveTypeName_t specialWaveTypeNames[] = {
	{ "loper_special", AICHAR_LOPER_SPECIAL },
	{ "zombie_ghost",  AICHAR_ZOMBIE_GHOST  },
};
#define NUM_SPECIAL_WAVE_TYPE_NAMES ( sizeof( specialWaveTypeNames ) / sizeof( specialWaveTypeNames[0] ) )

/*
============
Survival_ParseSpecialWaveTypes

  Parses a space-separated list of special-wave enemy names (e.g. "loper_special
  zombie_ghost") from the game_manager entity's "specialWaveType" key into
  svParams.specialWaveTypes[]. Called once at map spawn time; if str is empty or
  no recognized names are found, numSpecialWaveTypes stays 0 and
  AICast_PickSpecialWaveType() falls back to AICHAR_LOPER_SPECIAL.
============
*/
void Survival_ParseSpecialWaveTypes( const char *str ) {
	char buf[256];
	char *p, *token;
	int i;

	svParams.numSpecialWaveTypes = 0;

	if ( !str || !str[0] ) {
		return;
	}

	Q_strncpyz( buf, str, sizeof( buf ) );
	p = buf;

	while ( svParams.numSpecialWaveTypes < MAX_SPECIAL_WAVE_TYPES ) {
		token = COM_ParseExt( &p, qfalse );
		if ( !token[0] ) {
			break;
		}

		for ( i = 0; i < NUM_SPECIAL_WAVE_TYPE_NAMES; i++ ) {
			if ( !Q_stricmp( token, specialWaveTypeNames[i].name ) ) {
				svParams.specialWaveTypes[svParams.numSpecialWaveTypes++] = specialWaveTypeNames[i].type;
				break;
			}
		}
		if ( i == NUM_SPECIAL_WAVE_TYPE_NAMES ) {
			G_Printf( "Survival: unknown specialWaveType '%s' on game_manager, ignoring\n", token );
		}
	}
}

/*
============
AICast_PickSpecialWaveType

  Picks which AICharacters_t is featured for a newly-starting special wave, from
  the mapper-configured pool. Falls back to AICHAR_LOPER_SPECIAL if the map's
  game_manager didn't set a "specialWaveType" key (preserves old behavior).
============
*/
static AICharacters_t AICast_PickSpecialWaveType( void ) {
	if ( svParams.numSpecialWaveTypes <= 0 ) {
		return AICHAR_LOPER_SPECIAL;
	}
	return (AICharacters_t)svParams.specialWaveTypes[rand() % svParams.numSpecialWaveTypes];
}

/*
============
Survival_CountActivePlayers
  Connected, non-spectator, non-bot clients currently in the match.
============
*/
int Survival_CountActivePlayers( void ) {
	int i;
	int count = 0;

	for ( i = 0; i < level.maxclients; i++ ) {
		gentity_t *cl = &g_entities[i];

		if ( !cl->inuse || !cl->client ) {
			continue;
		}
		if ( cl->client->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( cl->client->sess.sessionTeam == TEAM_SPECTATOR ) {
			continue;
		}
		if ( cl->r.svFlags & SVF_BOT ) {
			continue;
		}

		count++;
	}

	return count;
}

/*
============
Survival_CheckWipe
============
*/
void Survival_CheckWipe( void ) {
	int i;
	int playing = 0;

	if ( svParams.waveGameOver ) {
		return;
	}

	for ( i = 0; i < level.maxclients; i++ ) {
		gentity_t *cl = &g_entities[i];

		if ( !cl->inuse || !cl->client ) {
			continue;
		}
		if ( cl->client->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( cl->client->sess.sessionTeam == TEAM_SPECTATOR ) {
			continue;
		}
		if ( cl->r.svFlags & SVF_BOT ) {
			continue;
		}

		playing++;

		// health <= 0 alone doesn't mean fallen -- a downed player keeps that value during bleed-out
		if ( cl->health > 0 || cl->client->ps.stats[STAT_REVIVE_TIME] > 0 ) {
			return; // someone's still standing, or still has a chance to get back up
		}
	}

	if ( playing == 0 ) {
		return;
	}

	svParams.waveGameOver = qtrue;
	svParams.endingType = ENDING_WIPE;
	svParams.exfilActive = qfalse;
	svParams.gameOverPhase = GAMEOVER_PHASE_LINGER;
	svParams.gameOverPhaseTime = level.time;
	svParams.gameOverCountdownShown = -1;
	svParams.gameOverMsgRefreshTime = level.time;
	svParams.gameOverFadeOutSent = qfalse;

	// the outro force-draws the scoreboard; make sure every client has fresh score data for it
	CalculateRanks();
	SendScoreboardMessageToAllClients();

	trap_SendServerCommand( -1, "cp \"^1GAME OVER\n^7All players have fallen\n\"" );

	if ( survCfg.gameoverMusic[0] ) {
		trap_SendServerCommand( -1, va( "mu_play %s 0", survCfg.gameoverMusic ) );
	}
}

#define EXFIL_STAMP_GRACE 300   // ms since the last trigger_exfil touch to still count as "in the zone"

/*
============
Survival_ExfilPlayerHoldingZone

  Live, connected, non-spectator human standing in a trigger_exfil brush right now.
============
*/
static qboolean Survival_ExfilPlayerHoldingZone( gentity_t *cl ) {
	if ( !cl->inuse || !cl->client || cl->client->pers.connected != CON_CONNECTED ) {
		return qfalse;
	}
	if ( cl->client->sess.sessionTeam == TEAM_SPECTATOR || ( cl->r.svFlags & SVF_BOT ) ) {
		return qfalse;
	}
	if ( cl->health <= 0 ) {
		return qfalse;
	}
	if ( !cl->client->exfilZoneTime || level.time - cl->client->exfilZoneTime > EXFIL_STAMP_GRACE ) {
		return qfalse;
	}
	return qtrue;
}

/*
============
Survival_BuildExfilBanner

  Builds svParams.exfilBanner (the "Successful Exfil!" text) from whoever is in a zone.
============
*/
static void Survival_BuildExfilBanner( void ) {
	int i;
	int survivors = 0;
	int active = Survival_CountActivePlayers();
	char names[512];

	names[0] = 0;

	for ( i = 0; i < level.maxclients; i++ ) {
		gentity_t *cl = &g_entities[i];

		if ( !Survival_ExfilPlayerHoldingZone( cl ) ) {
			continue;
		}

		survivors++;
		Q_strcat( names, sizeof( names ), va( "^7%s\n", cl->client->pers.netname ) );
	}

	if ( survivors > 0 && survivors >= active ) {
		Q_strncpyz( svParams.exfilBanner, "^2Successful Exfil!\n^7All players survived!", sizeof( svParams.exfilBanner ) );
	} else {
		Com_sprintf( svParams.exfilBanner, sizeof( svParams.exfilBanner ),
			"^2Successful Exfil!\n^7Survived:\n%s", names );
	}
}

/*
============
Survival_BeginExfilEnding

  Countdown hit zero - hand off to the shared game-over sequence with ENDING_EXFIL text/music.
============
*/
static void Survival_BeginExfilEnding( void ) {
	Survival_BuildExfilBanner();

	svParams.endingType = ENDING_EXFIL;
	svParams.exfilActive = qfalse;
	svParams.waveGameOver = qtrue;

	svParams.gameOverPhase = GAMEOVER_PHASE_LINGER;
	svParams.gameOverPhaseTime = level.time;
	svParams.gameOverCountdownShown = -1;
	svParams.gameOverMsgRefreshTime = level.time;
	svParams.gameOverFadeInSent = qfalse;
	svParams.gameOverFadeOutSent = qfalse;

	Survival_GameManagerEvent( "exfil_complete" );

	// the outro force-draws the scoreboard; make sure every client has fresh score data for it
	CalculateRanks();
	SendScoreboardMessageToAllClients();

	trap_SendServerCommand( -1, va( "cp \"%s\n\"", svParams.exfilBanner ) );

	if ( survCfg.exfilMusic[0] ) {
		trap_SendServerCommand( -1, va( "mu_play %s 0", survCfg.exfilMusic ) );
	}
}

/*
============
Survival_TickExfil

  Per-frame extraction countdown: runs while the trigger_exfil zone is held (>= 1 player,
  or all active players with ALLPLAYERS), resets when the hold is lost, hands off to the
  shared game-over sequence on completion. GT_COOP_SURVIVAL only.
============
*/
void Survival_TickExfil( void ) {
	int i;
	int inZone = 0;
	int needed;
	int elapsed, secLeft, secTotal;

	if ( g_gametype.integer != GT_COOP_SURVIVAL ) {
		return;
	}
	if ( svParams.endingType != ENDING_NONE || svParams.gameOverPhase != GAMEOVER_PHASE_NONE ) {
		return;
	}

	for ( i = 0; i < level.maxclients; i++ ) {
		if ( Survival_ExfilPlayerHoldingZone( &g_entities[i] ) ) {
			inZone++;
		}
	}

	svParams.exfilPlayersInZone = inZone;

	needed = svParams.exfilRequireAll ? Survival_CountActivePlayers() : 1;
	if ( needed < 1 ) {
		needed = 1;
	}

	if ( inZone == 0 ) {
		if ( svParams.exfilActive ) {
			svParams.exfilActive = qfalse;
			svParams.exfilCountdownShown = -1;
			trap_SendServerCommand( -1, "cp \"^3EXTRACTION ABORTED\n^7Zone abandoned\n\"" );
		}
		svParams.exfilWaitShown = -1;
		return;
	}

	// ALLPLAYERS: some but not all present - stall (and drop a running countdown)
	if ( inZone < needed ) {
		if ( svParams.exfilActive ) {
			svParams.exfilActive = qfalse;
			svParams.exfilCountdownShown = -1;
			trap_SendServerCommand( -1, "cp \"^3EXTRACTION HALTED\n^7Waiting for all players\n\"" );
		} else if ( inZone != svParams.exfilWaitShown ) {
			trap_SendServerCommand( -1, va( "cp \"^3EXTRACTION\n^7Waiting for all players: ^3%i^7/^3%i\n\"", inZone, needed ) );
		}
		svParams.exfilWaitShown = inZone;
		return;
	}

	svParams.exfilWaitShown = -1;

	if ( !svParams.exfilActive ) {
		svParams.exfilActive = qtrue;
		svParams.exfilStartTime = level.time;
		svParams.exfilCountdownShown = -1;
		if ( svParams.exfilDuration < 1000 ) {
			svParams.exfilDuration = 20000;
		}
		Survival_GameManagerEvent( "exfil_start" );
	}

	secTotal = svParams.exfilDuration / 1000;
	elapsed = level.time - svParams.exfilStartTime;
	secLeft = secTotal - ( elapsed / 1000 );
	if ( secLeft < 0 ) {
		secLeft = 0;
	}

	if ( secLeft != svParams.exfilCountdownShown ) {
		svParams.exfilCountdownShown = secLeft;
		if ( secLeft > 0 ) {
			trap_SendServerCommand( -1, va( "cp \"^2EXTRACTION\n^7Hold the zone: ^3%i\n\"", secLeft ) );
		}
	}

	if ( elapsed >= svParams.exfilDuration ) {
		Survival_BeginExfilEnding();
	}
}

/*
============
Survival_FindGameOverCam

  Looks up a mapper-placed info_notnull with the given targetname (e.g.
  "gameover_cam1"). Returns NULL if the map doesn't have one - callers must
  degrade gracefully (see Survival_TickGameOver).
============
*/
static gentity_t *Survival_FindGameOverCam( const char *targetname ) {
	return G_Find( NULL, FOFS( targetname ), targetname );
}

/*
============
Survival_EnterGameOverPhase

  Common phase-transition work: latch the new phase/time, and for the camera
  phases, snap svParams.gameOverCamOrigin/Angles to the matching gameover_camN
  entity (if the map has one) plus a screen fade over the cut.
============
*/
static void Survival_EnterGameOverPhase( gameOverPhase_t phase, const char *camTargetname ) {
	svParams.gameOverPhase = phase;
	svParams.gameOverPhaseTime = level.time;
	svParams.gameOverFadeInSent = qfalse;
	svParams.gameOverFadeOutSent = qfalse;

	if ( !camTargetname ) {
		svParams.gameOverCamValid = qfalse;
		return;
	}

	{
		gentity_t *cam = Survival_FindGameOverCam( camTargetname );

		if ( cam ) {
			VectorCopy( cam->s.origin, svParams.gameOverCamOrigin );
			VectorCopy( cam->s.angles, svParams.gameOverCamAngles );
			svParams.gameOverCamValid = qtrue;

			// instant black hides the camera cut; Survival_TickGameOver fades back up gradually
			trap_SetConfigstring( CS_SCREENFADE, va( "1 %i 1", level.time ) );
		} else {
			svParams.gameOverCamValid = qfalse;
		}
	}
}

/*
============
Survival_TickGameOver

  GT_COOP_SURVIVAL only, called once per server frame from G_RunFrame (after
  ClientEndFrame, so nothing else overwrites our pm_type/origin override for
  this frame). Runs the post-wipe sequence: let the death-fall linger, cycle
  the view through up to 3 mapper-placed cameras (info_notnull entities
  targetnamed gameover_cam1/2/3), then hold on the 3rd and count down to a
  map_restart. Maps without the camera entities still get the GAME OVER text,
  music and countdown - they just don't get the camera cuts.
============
*/
void Survival_TickGameOver( void ) {
	int elapsed;
	int i;

	if ( svParams.gameOverPhase == GAMEOVER_PHASE_NONE ) {
		return;
	}

	// flag connected players so cgame can force the scoreboard + chat on; STAT_HEALTH/REVIVE_TIME aren't reliable here
	for ( i = 0; i < level.maxclients; i++ ) {
		gentity_t *cl = &g_entities[i];

		if ( !cl->inuse || !cl->client || cl->client->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( cl->client->sess.sessionTeam == TEAM_SPECTATOR || ( cl->r.svFlags & SVF_BOT ) ) {
			continue;
		}

		cl->client->ps.stats[STAT_GAMEOVER] = 1;
	}

	elapsed = level.time - svParams.gameOverPhaseTime;

	// fade back up from the cut-to-black once it's had time to land, one-shot per phase
	if ( svParams.gameOverCamValid && !svParams.gameOverFadeInSent &&
		 svParams.gameOverPhase >= GAMEOVER_PHASE_CAM1 && svParams.gameOverPhase <= GAMEOVER_PHASE_CAM3 &&
		 elapsed >= survCfg.gameoverFadeTime ) {
		svParams.gameOverFadeInSent = qtrue;
		trap_SetConfigstring( CS_SCREENFADE, va( "0 %i %i", level.time, survCfg.gameoverFadeTime ) );
	}

	// CAM1/CAM2 only: fade back to black on the current camera before cutting to the next; CAM3 has nowhere else to cut to
	if ( svParams.gameOverCamValid && !svParams.gameOverFadeOutSent &&
		 ( svParams.gameOverPhase == GAMEOVER_PHASE_CAM1 || svParams.gameOverPhase == GAMEOVER_PHASE_CAM2 ) &&
		 elapsed >= survCfg.gameoverFadeTime + survCfg.gameoverCamHoldTime ) {
		svParams.gameOverFadeOutSent = qtrue;
		trap_SetConfigstring( CS_SCREENFADE, va( "1 %i %i", level.time, survCfg.gameoverFadeTime ) );
	}

	// LINGER only: fade out of the death-fall view before cutting to CAM1, same as the CAM1/CAM2 transitions above
	if ( svParams.gameOverPhase == GAMEOVER_PHASE_LINGER && !svParams.gameOverFadeOutSent &&
		 elapsed >= survCfg.gameoverLingerTime ) {
		svParams.gameOverFadeOutSent = qtrue;
		trap_SetConfigstring( CS_SCREENFADE, va( "1 %i %i", level.time, survCfg.gameoverFadeTime ) );
	}

	// re-send periodically so cg_centertime doesn't fade this out before the countdown case below takes over
	if ( svParams.gameOverPhase == GAMEOVER_PHASE_LINGER ||
		 svParams.gameOverPhase == GAMEOVER_PHASE_CAM1 ||
		 svParams.gameOverPhase == GAMEOVER_PHASE_CAM2 ||
		 ( svParams.gameOverPhase == GAMEOVER_PHASE_CAM3 && elapsed < survCfg.gameoverCam3HoldTime ) ) {
		if ( level.time - svParams.gameOverMsgRefreshTime >= 2000 ) {
			svParams.gameOverMsgRefreshTime = level.time;
			if ( svParams.endingType == ENDING_EXFIL ) {
				trap_SendServerCommand( -1, va( "cp \"%s\n\"", svParams.exfilBanner ) );
			} else {
				trap_SendServerCommand( -1, "cp \"^1GAME OVER\n^7All players have fallen\n\"" );
			}
			SendScoreboardMessageToAllClients();   // keep the force-drawn scoreboard fed
		}
	}

	switch ( svParams.gameOverPhase ) {
	case GAMEOVER_PHASE_LINGER:
		// the fade-out above must finish before we cut to CAM1
		if ( elapsed >= survCfg.gameoverLingerTime + survCfg.gameoverFadeTime ) {
			Survival_EnterGameOverPhase( GAMEOVER_PHASE_CAM1, "gameover_cam1" );
		}
		break;

	case GAMEOVER_PHASE_CAM1:
		// fade in + hold + fade out (all three timed above) must finish before the cut
		if ( elapsed >= survCfg.gameoverFadeTime + survCfg.gameoverCamHoldTime + survCfg.gameoverFadeTime ) {
			Survival_EnterGameOverPhase( GAMEOVER_PHASE_CAM2, "gameover_cam2" );
		}
		break;

	case GAMEOVER_PHASE_CAM2:
		if ( elapsed >= survCfg.gameoverFadeTime + survCfg.gameoverCamHoldTime + survCfg.gameoverFadeTime ) {
			Survival_EnterGameOverPhase( GAMEOVER_PHASE_CAM3, "gameover_cam3" );
		}
		break;

	case GAMEOVER_PHASE_CAM3:
		if ( elapsed < survCfg.gameoverCam3HoldTime ) {
			break;
		}

		{
			int countdownElapsed = elapsed - survCfg.gameoverCam3HoldTime;
			int secLeft = survCfg.gameoverCountdown - ( countdownElapsed / 1000 );

			if ( secLeft < 0 ) {
				secLeft = 0;
			}

			if ( secLeft != svParams.gameOverCountdownShown ) {
				svParams.gameOverCountdownShown = secLeft;

				if ( secLeft > 0 ) {
					if ( svParams.endingType == ENDING_EXFIL ) {
						trap_SendServerCommand( -1, va( "cp \"%s\n^7Restarting in ^3%i\n\"", svParams.exfilBanner, secLeft ) );
					} else {
						trap_SendServerCommand( -1, va( "cp \"^1GAME OVER\n^7Restarting in ^3%i\n\"", secLeft ) );
					}
				}
			}

			if ( secLeft <= 0 ) {
				svParams.gameOverPhase = GAMEOVER_PHASE_RESTART;
				trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
			}
		}
		break;

	case GAMEOVER_PHASE_RESTART:
	default:
		break;
	}

	// hold every connected player's view on the active camera while cycling/holding
	if ( svParams.gameOverCamValid &&
		 svParams.gameOverPhase >= GAMEOVER_PHASE_CAM1 && svParams.gameOverPhase <= GAMEOVER_PHASE_CAM3 ) {

		// hides dead players/AI and CopyToBodyQue() corpses (g_client.c) from camera view; eType is what CG_Player() checks, not modelindex
		for ( i = 0; i < level.num_entities; i++ ) {
			gentity_t *cl = &g_entities[i];
			qboolean deadClient = cl->client && cl->health <= 0;
			qboolean bodyQueue = cl->classname && !Q_stricmp( cl->classname, "bodyque" );

			if ( !cl->inuse || !( deadClient || bodyQueue ) ) {
				continue;
			}

			if ( cl->client ) {
				cl->client->ps.eFlags = 0;
			}
			cl->s.eFlags = 0;
			cl->s.eType = ET_GENERAL;
			cl->s.modelindex = 0;
			cl->s.loopSound = 0;
			cl->s.event = 0;
		}

		for ( i = 0; i < level.maxclients; i++ ) {
			gentity_t *cl = &g_entities[i];

			if ( !cl->inuse || !cl->client ) {
				continue;
			}

			if ( cl->client->pers.connected != CON_CONNECTED ) {
				continue;
			}
			if ( cl->client->sess.sessionTeam == TEAM_SPECTATOR ) {
				continue;
			}
			if ( cl->r.svFlags & SVF_BOT ) {
				continue; // AI casts occupy client slots too - only lock human players' views
			}

			VectorCopy( svParams.gameOverCamOrigin, cl->client->ps.origin );
			VectorCopy( svParams.gameOverCamAngles, cl->client->ps.viewangles );
			VectorClear( cl->client->ps.velocity );
			// PM_FREEZE not PM_INTERMISSION - the latter forces the scoreboard on; cg_view.c/cg_weapons.c/bg_misc.c give PM_FREEZE the same view/weapon/body treatment instead
			cl->client->ps.pm_type = PM_FREEZE;
			// clear stale PMF_FOLLOW/PMF_LIMBO so CG_DrawFollow's old "Waiting for next wave" text doesn't leak through
			cl->client->ps.pm_flags &= ~( PMF_FOLLOW | PMF_LIMBO );
		}
	}
}

void AICast_CheckSurvivalProgression( gentity_t *attacker ) {
	int enemiesAlive;
	int i;

	if ( g_gametype.integer != GT_COOP_SURVIVAL ) {
		return;
	}

	if ( !svParams.waveInProgress ) {
		return;
	}

	if ( svParams.wavePending ) {
		return;
	}

	if ( svParams.waveKillCount < svParams.killCountRequirement ) {
		enemiesAlive = 0;

		for ( i = 0; i < level.num_entities; i++ ) {
			gentity_t *ent = &g_entities[i];

			if ( !ent->inuse || !ent->client ) {
				continue;
			}

			if ( ent->aiTeam == AITEAM_ALLIES ) {
				continue;
			}

			if ( ent->client->ps.eFlags & EF_DEAD ) {
				continue;
			}

			// Optional, but safer if some non-survival AI exists on map
			if ( !( ent->r.svFlags & SVF_CASTAI ) ) {
				continue;
			}

			enemiesAlive++;
		}

		if ( enemiesAlive == 0 ) {
			G_Printf( "^1[AI_SURVIVE] ERROR: No enemies alive but wave not complete (%d/%d kills)\n",
				svParams.waveKillCount,
				svParams.killCountRequirement );

			svParams.waveKillCount = svParams.killCountRequirement;
		}
	}

	if ( svParams.waveKillCount >= svParams.killCountRequirement ) {
		const char *endEvent;

		G_Printf( "^2[AI_SURVIVE] Wave %d complete (%d/%d)^7\n",
			svParams.waveCount,
			svParams.waveKillCount,
			svParams.killCountRequirement );

		svParams.wavePending = qtrue;
		svParams.waveInProgress = qfalse;
		svParams.waveChangeTime = level.time + survCfg.intermissionTime * 1000;

		endEvent = svParams.specialWaveActive ? "specialwave_end" : "wave_end";

		Survival_GameManagerEvent( endEvent );

		// revive everyone who died this wave, once, at the start of intermission
		for ( i = 0; i < level.maxclients; i++ ) {
			gentity_t *cl = &g_entities[i];

			if ( !cl->inuse || !cl->client ) {
				continue;
			}
			if ( cl->client->pers.connected != CON_CONNECTED ) {
				continue;
			}
			if ( cl->client->ps.pm_flags & PMF_LIMBO ) {
				reinforce( cl );
			}
		}
	}
}

void AICast_TickSurvivalWave( void ) {
	int i;
	int wave;
	int killReq;

	if ( !svParams.wavePending ) {
		return;
	}

	if ( level.time < svParams.waveChangeTime ) {
		return;
	}

	G_Printf( "Survival: starting wave %i\n", svParams.waveCount + 1 );

	svParams.wavePending = qfalse;
	svParams.waveInProgress = qtrue;
	svParams.waveCount++;
	svParams.waveKillCount = 0;
	svParams.spawnedThisWave = 0;

	// Some map-placed AI never go through AIChar_AIScript_AlertEntity_Survival
	for ( i = 0; i < level.num_entities; i++ ) {
		gentity_t *ent = &g_entities[i];

		if ( !ent->inuse || !ent->client || !( ent->r.svFlags & SVF_CASTAI ) ) {
			continue;
		}
		if ( ent->aiTeam == AITEAM_ALLIES ) {
			continue;
		}
		if ( ent->aiInactive ) {
			continue;
		}
		if ( ent->client->ps.eFlags & EF_DEAD ) {
			continue;
		}

		svParams.spawnedThisWave++;
	}

	wave = svParams.waveCount;
	killReq = 0;

	svParams.playerCount = Survival_CountActivePlayers();
	if ( svParams.playerCount < 1 ) {
		svParams.playerCount = 1;
	}
	svParams.playerCountScale = 1.0f + ( svParams.playerCount - 1 ) * ( survCfg.playerScalePerExtraPct / 100.0f );

if ( wave == 1 ) {
	G_Printf( "Survival: wave 1 before start_survival script\n" );
	Survival_GameManagerEvent( "start_survival" );
	G_Printf( "Survival: wave 1 after start_survival script\n" );

	killReq = survCfg.initialKillCountReq;
} else {
		killReq = (int)( 0.15f * wave * wave + 3.0f * wave + 10.0f );
	}

	killReq = (int)( killReq * svParams.playerCountScale );

	svParams.specialWaveActive = qfalse;

	if ( AICast_ShouldStartSpecialWave() ) {
		int count;

		svParams.specialWaveActive = qtrue;
		svParams.lastSpecialWave = wave;
		svParams.currentSpecialWaveType = AICast_PickSpecialWaveType();

		count = survCfg.specialWaveCountInitial +
			survCfg.specialWaveCountIncrease * ( wave - survCfg.specialWaveMinStart );

		if ( count < survCfg.specialWaveCountInitial ) {
			count = survCfg.specialWaveCountInitial;
		}

		// maxActiveAI stays unscaled - the spawn gate already multiplies every type's cap by playerCountScale, so scaling here too would double it.
		svParams.killCountRequirement = (int)( count * svParams.playerCountScale );

		svParams.maxActiveAI[svParams.currentSpecialWaveType] =
			( count < survCfg.specialWaveCountMax ) ? count : survCfg.specialWaveCountMax;
	} else {
		svParams.killCountRequirement = killReq;

		if ( wave > 1 ) {
			AICast_UpdateMaxActiveAI();
		}
	}

	for ( i = 0; i < g_maxclients.integer; i++ ) {
		gentity_t *cl = &g_entities[i];

		if ( !cl->inuse || !cl->client ) {
			continue;
		}

		if ( cl->client->pers.connected != CON_CONNECTED ) {
			continue;
		}

		cl->client->ps.persistant[PERS_WAVES]++;
	}

	if ( svParams.specialWaveActive ) {
		Survival_GameManagerEvent( "specialwave_start" );
	} else {
		Survival_GameManagerEvent( "wave_start" );
	}
}


/*
============
AICast_ReinforceAvailable

True if at least one allied AI cast is currently down (never deployed, or
dead and waiting) and would be brought back in by AICast_CallReinforcements.
============
*/
qboolean AICast_ReinforceAvailable( void ) {
	int i;
	gentity_t *ent;
	cast_state_t *cs;

	for ( i = 0; i < level.num_entities; i++ ) {
		ent = &g_entities[i];
		if ( !ent->inuse || !ent->aiCharacter || ent->aiTeam != AITEAM_ALLIES || ent->oneshot ) {
			continue;
		}
		cs = AICast_GetCastState( ent->s.number );
		if ( cs->norespawn || cs->respawnsleft != 0 ) {
			continue;
		}
		return qtrue;
	}
	return qfalse;
}


/*
============
AICast_CallReinforcements

Grants one queued life to every allied AI cast that is currently down.
The existing per-frame survival respawn/alert-retry logic
(AICast_SurvivalRespawn / AIChar_AIScript_AlertEntity_Survival) picks them
up and brings them back in on its own - nothing else to do here.
============
*/
void AICast_CallReinforcements( void ) {
	int i;
	gentity_t *ent;
	cast_state_t *cs;

	for ( i = 0; i < level.num_entities; i++ ) {
		ent = &g_entities[i];
		if ( !ent->inuse || !ent->aiCharacter || ent->aiTeam != AITEAM_ALLIES || ent->oneshot ) {
			continue;
		}
		cs = AICast_GetCastState( ent->s.number );
		if ( cs->norespawn || cs->respawnsleft != 0 ) {
			continue;
		}
		cs->respawnsleft = 1;
	}
}


/*
============
AICast_SurvivalRespawn
============
*/
void AICast_SurvivalRespawn(gentity_t *ent, cast_state_t *cs) {

   vec3_t mins, maxs;
   int touch[10], numTouch;
   float oldmaxZ;
   int i;
   gentity_t *player;
   gentity_t *spawnSpot;
   vec3_t spawn_origin, spawn_angles;

   if (ent->aiTeam != 1 && (svParams.spawnedThisWave >= svParams.killCountRequirement || !svParams.waveInProgress))
   {
	   return;
   }

	// Prevent friendly AI from respawning if respawnsleft is 0
    if (ent->aiTeam == 1 && cs->respawnsleft == 0) {
        return;
    }

	// Same block for respawns
	if (svParams.specialWaveActive && ent->aiTeam != 1 && ent->aiCharacter != svParams.currentSpecialWaveType)
	{
		return;
	}

	if (!svParams.specialWaveActive && ent->aiTeam != 1 && AICast_IsSpecialWaveChar(ent->aiCharacter))
	{
		return;
	}

			if ( ent->aiCharacter != AICHAR_ZOMBIE && ent->aiCharacter != AICHAR_HELGA
				 && ent->aiCharacter != AICHAR_HEINRICH ) {

				for ( i = 0 ; i < g_maxclients.integer ; i++ ) {
					player = &g_entities[i];

					if ( !player || !player->inuse ) {
						continue;
					}

					if ( player->r.svFlags & SVF_CASTAI ) {
						continue;
					}
				}
			}

			oldmaxZ = ent->r.maxs[2];

			// make sure the area is clear
			AIChar_SetBBox( ent, cs, qfalse );

			VectorAdd( ent->r.currentOrigin, ent->r.mins, mins );
			VectorAdd( ent->r.currentOrigin, ent->r.maxs, maxs );
			trap_UnlinkEntity( ent );

			numTouch = trap_EntitiesInBox( mins, maxs, touch, 10 );

			if ( numTouch ) {
				for ( i = 0; i < numTouch; i++ ) {
					if ( g_entities[touch[i]].r.contents & MASK_PLAYERSOLID ) {
						break;
					}
				}
				if ( i == numTouch ) {
					numTouch = 0;
				}
			}

			if ( numTouch == 0 ) {    // ok to spawn

				BG_SetBehaviorForSurvival(ent->aiCharacter);
				AICast_ApplySurvivalAttributes(ent, cs);
				ent->r.contents = CONTENTS_BODY;
				ent->clipmask = MASK_PLAYERSOLID | CONTENTS_MONSTERCLIP;
				if ( ent->aiTeam == AITEAM_ALLIES ) {
					ent->clipmask |= CONTENTS_FRIENDLYCLIP;
				}
				ent->takedamage = qtrue;
				ent->waterlevel = 0;
				ent->watertype = 0;
				ent->flags = 0;
				ent->s.dmgFlags = 0;
				ent->die = AICast_Die;
				ent->client->ps.eFlags &= ~EF_DEAD;
				ent->s.eFlags &= ~EF_DEAD;
				player = AICast_FindEntityForName( "player" );

                // Selecting the spawn point for the AI
				spawnSpot = SelectSpawnPoint_AI( player, ent, spawn_origin, spawn_angles );
				G_SetOrigin( ent, spawn_origin );
				VectorCopy( spawn_origin, ent->client->ps.origin );
				SetClientViewAngle( ent, spawn_angles );

		        // Refresh now so script actions that read our position this frame see where we actually are
				VectorCopy( ent->client->ps.origin, cs->bs->origin );
				VectorCopy( ent->client->ps.origin, cs->bs->eye );
				cs->bs->eye[2] += ent->client->ps.viewheight;
				// make sure we're using the right AAS world - this can run outside cs's own AICast_Think
				trap_AAS_SetCurrentWorld( cs->aasWorldIndex );
				cs->bs->areanum = BotPointAreaNum( cs->bs->origin );
				if ( cs->bs->areanum ) {
					cs->lastValidAreaNum[cs->aasWorldIndex] = cs->bs->areanum;
					cs->lastValidAreaTime[cs->aasWorldIndex] = level.time;
				}

				// clear stale goto target BEFORE the respawn script runs, not after (it may gotomarker itself)
				cs->castScriptStatus.scriptGotoId = -1;
				cs->castScriptStatus.scriptGotoEnt = -1;

				// Activate respawn scripts for AI
				AICast_ScriptEvent(cs, "respawn", "");

				// Clear leftover vanilla/script movement state after respawn script.
				cs->followEntity = -1;
				cs->enemyNum = -1;
				cs->lastEnemy = -1;
				cs->combatGoalTime = 0;
				cs->battleHuntPauseTime = 0;
				cs->battleHuntViewTime = 0;
				cs->takeCoverTime = 0;
				cs->obstructingTime = 0;
				cs->grenadeFlushFiring = qfalse;
				cs->grenadeFlushEndTime = -1;
				cs->lockViewAnglesTime = -1;

				// Turn off Headshot flag and reattach hat
				ent->client->ps.eFlags &= ~EF_HEADSHOT;
				G_AddEvent( ent, EV_REATTACH_HAT, 0 );

				cs->rebirthTime = 0;
				cs->deathTime = 0;
				cs->died = qfalse;
				cs->registeredSurvivalKill = qfalse;

				ent->client->ps.eFlags &= ~EF_DEATH_FRAME;
				ent->client->ps.eFlags &= ~EF_FORCE_END_FRAME;
				ent->client->ps.eFlags |= EF_NO_TURN_ANIM;

				// play the revive animation
				cs->revivingTime = level.time + BG_AnimScriptEvent( &ent->client->ps, ANIM_ET_REVIVE, qfalse, qtrue );

				if (ent->aiTeam == 1)
				{
					AICast_StateChange(cs, AISTATE_RELAXED);
					AIFunc_IdleStart(cs);

					svParams.spawnedThisWaveFriendly++;
				}
				else
				{
					AICast_StateChange(cs, AISTATE_RELAXED);
					AIFunc_IdleStart(cs);

					svParams.spawnedThisWave++;
				}
			} else {
				// can't spawn yet, so set bbox back, and wait
				ent->r.maxs[2] = oldmaxZ;
				ent->client->ps.maxs[2] = ent->r.maxs[2];
			}
			trap_LinkEntity( ent );


}
