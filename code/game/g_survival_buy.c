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

// g_survival_buy.c

#include "g_local.h"
#include "g_survival.h"


// Prices, perk limits, and mystery-box timing below all come from survCfg (see g_survival.h / g_survival_config.c).

static int Survival_GetCompiledInWeaponPrice(int weapon) {
	switch (weapon) {
		case WP_KNIFE:        return 10;
		// Pistols
		case WP_LUGER:        return 30;
		case WP_SILENCER:     return 30;
		case WP_COLT:         return 30;
		case WP_AKIMBO:       return 60;
		case WP_TT33:         return 35;
		case WP_DUAL_TT33:    return 70;
		case WP_REVOLVER:     return 80;
		case WP_HDM:          return 50;

		// SMGs
		case WP_STEN:         return 90;
		case WP_MP40:         return 100;
		case WP_MP34:         return 120;
		case WP_THOMPSON:     return 120;
		case WP_PPSH:         return 130;

		// Rifles
		case WP_MAUSER:       return 50;
		case WP_MOSIN:        return 60;
		case WP_SNIPERRIFLE:  return 100;
		case WP_SNOOPERSCOPE: return 150;
		case WP_M1GARAND:     return 150;
		case WP_G43:          return 150;
		case WP_M1941:        return 150;
		case WP_M1941SCOPE:   return 150;
		case WP_DELISLE:      return 350;
		case WP_DELISLESCOPE: return 350;

		// Auto Rifles

		// Assault Rifles
		case WP_FG42:         return 250;
		case WP_MP44:         return 250;
		case WP_BAR:          return 250;

		// Shotguns
		case WP_M97:          return 200;
		case WP_AUTO5:        return 225;

		// Heavy
		case WP_PANZERFAUST:  return 400;
		case WP_FLAMETHROWER: return 500;
		case WP_VENOM:        return 500;
		case WP_TESLA:        return 500;
		case WP_MG42M:        return 500;
		case WP_BROWNING:     return 500;

		// Grenades
		case WP_GRENADE_LAUNCHER:   return 100;
		case WP_GRENADE_PINEAPPLE:  return 100;

		default: return 100;
	}
}

/*
============
Survival_GetDefaultWeaponPrice

  survCfg.weaponPrice[] override (set via "weapon_price.<name>" in a .surv
  file) takes priority; otherwise falls back to the compiled-in table above.
============
*/
int Survival_GetDefaultWeaponPrice(int weapon) {
	if (weapon > WP_NONE && weapon < WP_NUM_WEAPONS && survCfg.weaponPrice[weapon] >= 0) {
		return survCfg.weaponPrice[weapon];
	}
	return Survival_GetCompiledInWeaponPrice(weapon);
}

/*
===========================================================================
Random weapon box (mystery box), survival mode

Flow: RWB_IDLE --(USE, pays price)--> RWB_SPINNING --(lands)--> RWB_LANDED
      RWB_LANDED --(USE, grants weapon)--> RWB_IDLE
      RWB_LANDED --(decision time expires)--> RWB_IDLE (forfeited)

State lives on the target_buy entity (see rwb* fields in g_local.h). The
floating carousel model is a separate, non-solid ET_ITEM entity spawned
above the target_buy's origin and despawned on pickup/reset.
===========================================================================
*/

#define RWB_IDLE     0
#define RWB_SPINNING 1
#define RWB_LANDED   2

// Timing below comes from survCfg.rwb* (ms/units, see g_survival_config.c)

static const weapon_t rwb_weapon_pool[] = {
	WP_LUGER, WP_SILENCER, WP_COLT,
	WP_AKIMBO, WP_TT33, WP_DUAL_TT33, WP_REVOLVER, WP_HDM, WP_MP40, WP_THOMPSON, WP_STEN, WP_MP34, WP_PPSH,
	WP_MAUSER, WP_MOSIN, WP_SNIPERRIFLE, WP_SNOOPERSCOPE, WP_FG42,
	WP_M1GARAND, WP_G43, WP_M1941, WP_MP44, WP_BAR, WP_M97, WP_AUTO5,
	WP_PANZERFAUST, WP_FLAMETHROWER, WP_VENOM, WP_TESLA,
	WP_MG42M, WP_BROWNING, WP_DELISLE
};
static const int rwb_weapon_pool_count = sizeof( rwb_weapon_pool ) / sizeof( rwb_weapon_pool[0] );

void Survival_RandomBox_Think( gentity_t *ent );

static int Survival_RandomBox_ItemIndexForWeapon( weapon_t weapon ) {
	for ( int i = 1; bg_itemlist[i].classname; i++ ) {
		if ( bg_itemlist[i].giType == IT_WEAPON && bg_itemlist[i].giWeapon == weapon ) {
			return i;
		}
	}
	return 0;
}

// Picks a weapon the activator doesn't already have
static weapon_t Survival_RandomBox_PickFinalWeapon( gentity_t *activator ) {
	weapon_t chosen;
	int tries = 20;

	do {
		chosen = rwb_weapon_pool[rand() % rwb_weapon_pool_count];
		tries--;
	} while ( G_FindWeaponSlot( activator, chosen ) >= 0 && tries > 0 );

	if ( tries <= 0 ) {
		return WP_NONE;
	}

	return chosen;
}

static void Survival_RandomBox_SetGlow( gentity_t *display, int r, int g, int b, int intensity ) {
	display->s.constantLight = r | ( g << 8 ) | ( b << 16 ) | ( intensity << 24 );
}

static void Survival_RandomBox_SetDisplayItem( gentity_t *display, int itemIndex ) {
	display->item = &bg_itemlist[itemIndex];
	display->s.modelindex = itemIndex;
}

static gentity_t *Survival_RandomBox_SpawnDisplay( gentity_t *box, int itemIndex ) {
	gentity_t *display = G_Spawn();

	display->classname = "random_weapon_display";
	display->s.eType = ET_ITEM;
	display->r.contents = 0;
	display->clipmask = 0;

	// side-on display, no stand model, no touch-pickup prediction
	display->s.eFlags = EF_ITEM_FACE_SIDE | EF_ITEM_NO_PREDICT | EF_ITEM_FORCE_NO_STAND;

	VectorSet( display->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, -ITEM_RADIUS );
	VectorSet( display->r.maxs, ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS );

	VectorCopy( box->s.origin, display->s.pos.trBase );
	display->s.pos.trBase[2] += survCfg.rwbFloatHeight;
	display->s.pos.trDelta[2] = survCfg.rwbBobHeight;
	display->s.pos.trDuration = survCfg.rwbBobSpeed;
	display->s.pos.trTime = level.time;
	display->s.pos.trType = TR_SINE;

	VectorCopy( display->s.pos.trBase, display->r.currentOrigin );

	Survival_RandomBox_SetDisplayItem( display, itemIndex );

	trap_LinkEntity( display );

	return display;
}

static void Survival_RandomBox_Reset( gentity_t *ent ) {
	if ( ent->rwbDisplay ) {
		G_FreeEntity( ent->rwbDisplay );
		ent->rwbDisplay = NULL;
	}

	ent->rwbState = RWB_IDLE;
	ent->rwbChosenWeapon = WP_NONE;
	ent->rwbChosenItemIndex = 0;
	ent->rwbBlinking = qfalse;
	ent->think = NULL;
	ent->nextthink = 0;
}

// clears the linked func_invisible_user's mapper-set reuse delay so landing doesn't leave it debounced
static void Survival_RandomBox_ClearActivatorDebounce( gentity_t *box ) {
	if ( !box->targetname ) {
		return;
	}

	for ( int i = 0; i < level.num_entities; i++ ) {
		gentity_t *e = &g_entities[i];

		if ( !e->inuse || Q_stricmp( e->classname, "func_invisible_user" ) != 0 ) {
			continue;
		}

		if ( e->target && Q_stricmp( e->target, box->targetname ) == 0 ) {
			e->wait = level.time - 1;
		}
	}
}

static void Survival_RandomBox_Land( gentity_t *ent ) {
	Survival_RandomBox_ClearActivatorDebounce( ent );

	if ( ent->rwbDisplay ) {
		Survival_RandomBox_SetDisplayItem( ent->rwbDisplay, ent->rwbChosenItemIndex );
		Survival_RandomBox_SetGlow( ent->rwbDisplay, 255, 220, 120, 70 );
		G_Sound( ent->rwbDisplay, G_SoundIndex( "sound/misc/randombox_create.wav" ) );
	}

	ent->rwbState = RWB_LANDED;
	ent->rwbPhaseStartTime = level.time;
	ent->rwbBlinking = qfalse;

	ent->think = Survival_RandomBox_Think;
	ent->nextthink = level.time + survCfg.rwbBlinkStart;
}

// warm-to-hot palette the spin light strobes through on every swap
static const int rwb_pulse_r[] = { 255, 255, 255, 255 };
static const int rwb_pulse_g[] = { 160, 210, 110, 60 };
static const int rwb_pulse_b[] = { 60,  90,  190, 40 };
static const int rwb_pulse_count = sizeof( rwb_pulse_r ) / sizeof( rwb_pulse_r[0] );

static void Survival_RandomBox_ThinkSpin( gentity_t *ent ) {
	int elapsed = level.time - ent->rwbPhaseStartTime;
	float frac;
	int interval;

	if ( elapsed >= survCfg.rwbSpinDuration || !ent->rwbDisplay ) {
		Survival_RandomBox_Land( ent );
		return;
	}

	frac = elapsed / (float)survCfg.rwbSpinDuration;

	// cosmetic swap: any pool weapon, just avoid an immediate repeat
	int guard = 6;
	int cosmeticIndex = 0;
	do {
		weapon_t cosmetic = rwb_weapon_pool[rand() % rwb_weapon_pool_count];
		cosmeticIndex = Survival_RandomBox_ItemIndexForWeapon( cosmetic );
		guard--;
	} while ( cosmeticIndex > 0 && cosmeticIndex == ent->rwbDisplay->s.modelindex && guard > 0 );

	if ( cosmeticIndex > 0 ) {
		Survival_RandomBox_SetDisplayItem( ent->rwbDisplay, cosmeticIndex );

		// dlight strobes a random hue each swap, brightening as landing nears
		{
			int hue = rand() % rwb_pulse_count;
			int intensity = 20 + (int)( 55 * frac * frac );

			Survival_RandomBox_SetGlow( ent->rwbDisplay, rwb_pulse_r[hue], rwb_pulse_g[hue], rwb_pulse_b[hue], intensity );
		}
	}

	// ease the swap interval out from fast to slow as we approach landing
	frac = frac * frac;
	interval = survCfg.rwbSpinIntervalMin + (int)( ( survCfg.rwbSpinIntervalMax - survCfg.rwbSpinIntervalMin ) * frac );

	ent->nextthink = level.time + interval;
}

static void Survival_RandomBox_ThinkLanded( gentity_t *ent ) {
	int elapsed = level.time - ent->rwbPhaseStartTime;

	if ( elapsed >= survCfg.rwbDecisionTime || !ent->rwbDisplay ) {
		Survival_RandomBox_Reset( ent );
		return;
	}

	ent->rwbBlinking = (qboolean)!ent->rwbBlinking;
	if ( ent->rwbBlinking ) {
		ent->rwbDisplay->s.eFlags |= EF_NODRAW;
	} else {
		ent->rwbDisplay->s.eFlags &= ~EF_NODRAW;
	}

	ent->nextthink = level.time + survCfg.rwbBlinkInterval;
}

void Survival_RandomBox_Think( gentity_t *ent ) {
	if ( ent->rwbState == RWB_SPINNING ) {
		Survival_RandomBox_ThinkSpin( ent );
	} else if ( ent->rwbState == RWB_LANDED ) {
		Survival_RandomBox_ThinkLanded( ent );
	}
}

/*
============
Survival_RandomBox_Start
Charges the price and kicks off the spin. Box must be RWB_IDLE.
============
*/
qboolean Survival_RandomBox_Start( gentity_t *ent, gentity_t *activator ) {
	int price;
	weapon_t chosen;
	int itemIndex;
	int startIndex;

	if ( !activator || !activator->client ) return qfalse;

	if ( ent->rwbState != RWB_IDLE ) {
		trap_SendServerCommand( -1, "mu_play sound/items/use_nothing.wav 0\n" );
		return qfalse;
	}

	price = ent->price > 0 ? ent->price : survCfg.priceRandomWeapon;

	if ( activator->client->ps.persistant[PERS_SCORE] < price ) {
		trap_SendServerCommand( -1, "mu_play sound/items/use_nothing.wav 0\n" );
		return qfalse;
	}

	chosen = Survival_RandomBox_PickFinalWeapon( activator );
	if ( chosen == WP_NONE ) {
		trap_SendServerCommand( -1, "mu_play sound/items/use_nothing.wav 0\n" );
		return qfalse;
	}

	itemIndex = Survival_RandomBox_ItemIndexForWeapon( chosen );
	if ( itemIndex <= 0 ) {
		return qfalse;
	}

	// Charged on activation - walking away forfeits it
	Survival_AwardScore( activator, -price );

	ent->rwbState = RWB_SPINNING;
	ent->rwbPhaseStartTime = level.time;
	ent->rwbChosenWeapon = chosen;
	ent->rwbChosenItemIndex = itemIndex;
	ent->rwbBlinking = qfalse;

	startIndex = Survival_RandomBox_ItemIndexForWeapon( rwb_weapon_pool[rand() % rwb_weapon_pool_count] );
	if ( startIndex <= 0 ) {
		startIndex = itemIndex;
	}

	ent->rwbDisplay = Survival_RandomBox_SpawnDisplay( ent, startIndex );
	Survival_RandomBox_SetGlow( ent->rwbDisplay, 255, 200, 80, 20 );

	// spin jingle, plays once for the full spin duration instead of a per-swap tick
	G_Sound( ent->rwbDisplay, G_SoundIndex( "sound/misc/randombox.wav" ) );

	ent->think = Survival_RandomBox_Think;
	ent->nextthink = level.time + survCfg.rwbSpinIntervalMin;

	return qtrue;
}

/*
============
Survival_RandomBox_Pickup
Grants the already-chosen weapon. Box must be RWB_LANDED. Any player can claim it.
============
*/
qboolean Survival_RandomBox_Pickup( gentity_t *ent, gentity_t *activator ) {
	gitem_t *item;
	weapon_t chosen;
	weapon_t selectWeapon;
	int maxAmmo;

	if ( !activator || !activator->client ) return qfalse;
	if ( ent->rwbState != RWB_LANDED ) return qfalse;

	if ( ent->rwbChosenItemIndex <= 0 || !bg_itemlist[ent->rwbChosenItemIndex].classname ) {
		Survival_RandomBox_Reset( ent );
		return qfalse;
	}

	chosen = ent->rwbChosenWeapon;
	item = &bg_itemlist[ent->rwbChosenItemIndex];

	Give_Weapon_New_Inventory( activator, chosen, qfalse );

	maxAmmo = BG_GetMaxAmmo( &activator->client->ps, chosen, LT_AMMO_BONUS_MULTIPLIER );
	Add_Ammo( activator, chosen, maxAmmo, qtrue );  // fill clip
	Add_Ammo( activator, chosen, maxAmmo, qfalse ); // top off reserve

	if ( chosen == WP_M1GARAND ) {
		Give_Weapon_New_Inventory( activator, WP_M7, qfalse );
		int m7MaxAmmo = BG_GetMaxAmmo( &activator->client->ps, WP_M7, LT_AMMO_BONUS_MULTIPLIER );
		Add_Ammo( activator, WP_M7, m7MaxAmmo, qfalse );
	}

	// never auto-select into the scoped alt mode - land on the base weapon instead
	selectWeapon = chosen;
	if ( GetWeaponTableData( chosen )->weaponClass & WEAPON_CLASS_SCOPED ) {
		selectWeapon = GetWeaponTableData( chosen )->weapAlts;
	}

	// force-switch to the new weapon, replicating PM_FinishWeaponChange's bookkeeping by hand
	activator->client->ps.weapon = selectWeapon;
	activator->client->ps.weaponstate = WEAPON_RAISING;
	activator->client->ps.weaponTime = 250;
	activator->client->ps.weapAnimTimer = 0;
	activator->client->ps.weapAnim = ( ( activator->client->ps.weapAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | WEAP_RAISE;
	BG_UpdateConditionValue( activator->client->ps.clientNum, ANIM_COND_WEAPON, selectWeapon, qtrue );
	BG_AnimScriptEvent( &activator->client->ps, ANIM_ET_RAISEWEAPON, qfalse, qfalse );

	// sync cgame's weapon selection or the next usercmd re-requests the old weapon
	trap_SendServerCommand( activator - g_entities, va( "selectweap %d\n", selectWeapon ) );

	G_AddPredictableEvent( activator, EV_ITEM_PICKUP, item - bg_itemlist );
	trap_SendServerCommand( -1, "mu_play sound/misc/buy.wav 0\n" );

	Survival_RandomBox_Reset( ent );

	return qtrue;
}

/*
============
Survival_HandleRandomPerkBox
============
*/
qboolean Survival_HandleRandomPerkBox(gentity_t *ent, gentity_t *activator, char **itemName, int *itemIndex) {
	if (!activator || !activator->client) return qfalse;

	static char *random_perks[] = {
		"perk_resilience", "perk_scavenger", "perk_runner",
		"perk_weaponhandling", "perk_rifling", "perk_secondchance"
	};

	int price = (ent->price > 0) ? ent->price : survCfg.priceRandomPerk;
	const int numPerks = sizeof(random_perks) / sizeof(random_perks[0]);

	// Perk count limit (only matters for NEW perks, upgrades do not consume a slot)
	int perkCount = 0;
	for (int i = 0; i < MAX_PERKS; i++) {
		if (activator->client->ps.perks[i] > 0)
			perkCount++;
	}
	int maxPerks = (activator->client->ps.stats[STAT_PLAYER_CLASS] == PC_ENGINEER) ? survCfg.perksLimitEngineer : survCfg.perksLimit;

	// Not enough score?
	if (activator->client->ps.persistant[PERS_SCORE] < price) {
		G_AddEvent(activator, EV_GENERAL_SOUND, G_SoundIndex("sound/items/use_nothing.wav"));
		return qfalse;
	}

	// Find a valid outcome (reroll a few times to avoid wasting money on PRO-owned perks)
	for (int tries = 0; tries < 16; tries++) {

		int randomIndex = rand() % numPerks;
		*itemName = random_perks[randomIndex];

		for (int i = 1; bg_itemlist[i].classname; i++) {
			if (!Q_strcasecmp(*itemName, bg_itemlist[i].classname)) {
				*itemIndex = i;
				gitem_t *perkItem = &bg_itemlist[i];

				int perk = perkItem->giTag;
				int level = activator->client->ps.perks[perk];

				// Already PRO? reroll
				if (level >= 2) {
					break;
				}

				// New perk but no free slots? reroll (upgrades are still allowed)
				if (level <= 0 && perkCount >= maxPerks) {
					break;
				}

				// Apply: base (0->1) or pro (1->2)
				if (level <= 0) {
					activator->client->ps.perks[perk] = 1;
				} else {
					activator->client->ps.perks[perk] = 2;
				}

				activator->client->ps.stats[STAT_PERK] |= (1 << perk);
				Survival_AwardScore(activator, -price);

				G_AddPredictableEvent(activator, EV_ITEM_PICKUP, perkItem - bg_itemlist);
				trap_SendServerCommand(-1, "mu_play sound/misc/buy_perk.wav 0\n");
				G_StartTimedAction(activator, ACTION_BUYPERK, PERKBUY_ACTION_MS);
				return qtrue;
			}
		}
	}

	G_AddEvent(activator, EV_GENERAL_SOUND, G_SoundIndex("sound/items/use_nothing.wav"));
	return qfalse;
}

/*
===========================================================================
Perk choice machine, survival mode

Flow: idle --(USE, free)--> open, sends the price/owned list to the
      activator only and shows an on-screen list; mouse wheel moves the
      highlight client-side and syncs it back via "perksel"
      open --(USE by the SAME activator)--> confirm: buys whichever perk
      is currently highlighted via the normal Survival_HandlePerkPurchase
      path, then closes
      open --(idle timeout, or a different activator tries to USE it)-->
      closes with no charge / is denied

State lives on the target_buy entity (ent->pcActivator/pcSelected), mirroring
the random weapon box's rwb* fields above.
===========================================================================
*/

/*
============
Survival_PerkChoice_Close
============
*/
static void Survival_PerkChoice_Close( gentity_t *ent ) {
	if ( ent->pcActivator >= 0 && ent->pcActivator < MAX_CLIENTS ) {
		trap_SendServerCommand( ent->pcActivator, "perkmenu close\n" );
	}

	ent->pcActivator = -1;
	ent->think = NULL;
	ent->nextthink = 0;
}

// idle timeout - closes the window if the player stops browsing without confirming
static void Survival_PerkChoice_Think( gentity_t *ent ) {
	Survival_PerkChoice_Close( ent );
}

static void Survival_PerkChoice_ClearActivatorDebounce( gentity_t *box ) {
	if ( !box->targetname ) {
		return;
	}

	for ( int i = 0; i < level.num_entities; i++ ) {
		gentity_t *e = &g_entities[i];

		if ( !e->inuse || Q_stricmp( e->classname, "func_invisible_user" ) != 0 ) {
			continue;
		}

		if ( e->target && Q_stricmp( e->target, box->targetname ) == 0 ) {
			e->wait = level.time - 1;
		}
	}
}

/*
============
Survival_HandlePerkChoiceOpen

Opens the choice window for 'activator' and sends the price/owned list for
all 6 perks (PRO-upgrade price already doubled, matching Survival_HandlePerkPurchase).
Browsing is free - money is only taken on confirm.
============
*/
static void Survival_HandlePerkChoiceOpen( gentity_t *ent, gentity_t *activator ) {
	char cmd[192];
	int perk;

	Com_sprintf( cmd, sizeof( cmd ), "perkmenu open" );

	for ( perk = PERK_RESILIENCE; perk < NUM_PERKS; perk++ ) {
		int owned = activator->client->ps.perks[perk];
		int price = Survival_GetDefaultPerkPrice( perk );

		if ( owned == 1 ) {
			price *= 2; // PRO upgrade costs double, matches Survival_HandlePerkPurchase
		}

		Q_strcat( cmd, sizeof( cmd ), va( " %d %d", owned, price ) );
	}

	ent->pcActivator = activator->s.number;
	ent->pcSelected = PERK_RESILIENCE;

	Survival_PerkChoice_ClearActivatorDebounce( ent );

	ent->think = Survival_PerkChoice_Think;
	ent->nextthink = level.time + PERK_CHOICE_TIMEOUT;

	trap_SendServerCommand( activator->s.number, cmd );
}

/*
============
Survival_HandlePerkChoiceConfirm

Buys whichever perk is currently highlighted via the normal single-perk
purchase path, then closes the window either way (success or denied).
============
*/
static qboolean Survival_HandlePerkChoiceConfirm( gentity_t *ent, gentity_t *activator ) {
	gitem_t *item = BG_FindItemForPerk( ent->pcSelected );
	qboolean success = item ? Survival_HandlePerkPurchase( activator, item, 0 ) : qfalse;

	Survival_PerkChoice_Close( ent );

	return success;
}

/*
============
Survival_HandleAmmoPurchase
============
*/
qboolean Survival_HandleAmmoPurchase(gentity_t *ent, gentity_t *activator, int price) {
	if (!activator || !activator->client)
		return qfalse;

	int heldWeap = activator->client->ps.weapon;
	int upgradeLevel;
	if (heldWeap <= WP_NONE || heldWeap >= WP_NUM_WEAPONS)
		return qfalse;

	// Skip Lieutenant call-in weapons: cooldown-gated class abilities, not ammo-limited buys
	if (heldWeap == WP_AIRSTRIKE || heldWeap == WP_ARTY)
		return qfalse;

	int ammoIndex = BG_FindAmmoForWeapon(heldWeap);
	if (ammoIndex < 0)
		return qfalse;

	// Use upgrade-aware max ammo
	int maxAmmo = BG_GetMaxAmmo(&activator->client->ps, heldWeap, LT_AMMO_BONUS_MULTIPLIER);

	// Check if already full
	if (activator->client->ps.ammo[ammoIndex] >= maxAmmo)
		return qfalse;

	// Base price fallback: half weapon price
	int basePrice = Survival_GetDefaultWeaponPrice(heldWeap);
	int ammoPrice = basePrice / 2;

	upgradeLevel = activator->client->ps.weaponUpgraded[heldWeap];

	// Upgrade modifier
	if (price <= 0) {
		if (upgradeLevel >= 1) {
			ammoPrice = survCfg.priceAmmoUpgraded * upgradeLevel;
		}
	}

	// Mapper override
	if (price > 0) {
		ammoPrice = price;
	}

	// Check score
	if (activator->client->ps.persistant[PERS_SCORE] < ammoPrice) {
		trap_SendServerCommand(-1, "mu_play sound/items/use_nothing.wav 0\n");
		return qfalse;
	}

	// Refill ammo and clip using upgrade-aware cap
	Add_Ammo(activator, heldWeap, maxAmmo, qtrue);
	Add_Ammo(activator, heldWeap, maxAmmo, qfalse);

	// Deduct score
	Survival_AwardScore(activator, -ammoPrice);

	trap_SendServerCommand(-1, "mu_play sound/misc/buy.wav 0\n");
	return qtrue;
}

/*
============
Survival_HandleWeaponUpgrade
============
*/
qboolean Survival_HandleWeaponUpgrade(gentity_t *ent, gentity_t *activator, int price)
{
	playerState_t *ps = &activator->client->ps;
	int weap = ps->weapon;
	int currentLevel;
	int maxUpgradeLevel = 3;
	int baseUpgradePrice;
	int upgradePrice;

	if (weap <= WP_NONE || weap >= WP_NUM_WEAPONS)
		return qfalse;

	// Weapons that cannot be upgraded
	if ( weap == WP_KNIFE || weap == WP_SNIPERRIFLE || weap == WP_FG42SCOPE || weap== WP_SNOOPERSCOPE || weap == WP_M1941SCOPE || weap == WP_DELISLESCOPE || weap == WP_DYNAMITE  || weap == WP_GRENADE_LAUNCHER || weap == WP_GRENADE_PINEAPPLE )
	{
		G_AddEvent(activator, EV_GENERAL_SOUND, G_SoundIndex("sound/items/use_nothing.wav"));
		return qfalse;
	}

	currentLevel = ps->weaponUpgraded[weap];

	// Already fully upgraded
	if (currentLevel >= maxUpgradeLevel)
	{
		G_AddEvent(activator, EV_GENERAL_SOUND, G_SoundIndex("sound/items/use_nothing.wav"));
		return qfalse;
	}

	// Use fallback base price
	baseUpgradePrice = survCfg.priceWeaponUpgrade;

	// Mapper override
	if (price > 0)
	{
		baseUpgradePrice = price;
	}

	// Level 1 = x1, Level 2 = x2, Level 3 = x3
	upgradePrice = baseUpgradePrice * (currentLevel + 1);

	if (activator->client->ps.persistant[PERS_SCORE] < upgradePrice)
	{
		G_AddEvent(activator, EV_GENERAL_SOUND, G_SoundIndex("sound/items/use_nothing.wav"));
		return qfalse;
	}

	ps->weaponUpgraded[weap]++;

	// If main weapon is upgraded upgrade alt too

	if (weap == WP_MAUSER)
		ps->weaponUpgraded[WP_SNIPERRIFLE] = ps->weaponUpgraded[weap];

	if (weap == WP_FG42)
		ps->weaponUpgraded[WP_FG42SCOPE] = ps->weaponUpgraded[weap];

	if (weap == WP_M1941)
		ps->weaponUpgraded[WP_M1941SCOPE] = ps->weaponUpgraded[weap];

	if (weap == WP_DELISLE)
		ps->weaponUpgraded[WP_DELISLESCOPE] = ps->weaponUpgraded[weap];

	Survival_AwardScore(activator, -upgradePrice);

	// Refill ammo
	Add_Ammo(activator, weap, BG_GetMaxAmmo(&activator->client->ps, weap, LT_AMMO_BONUS_MULTIPLIER), qtrue);
	Add_Ammo(activator, weap, BG_GetMaxAmmo(&activator->client->ps, weap, LT_AMMO_BONUS_MULTIPLIER), qfalse);

	trap_SendServerCommand(-1, "mu_play sound/misc/wpn_upgrade.wav 0\n");
	return qtrue;
}
/*
============
Survival_HandleWeaponOrGrenade
============
*/
qboolean Survival_HandleWeaponOrGrenade(gentity_t *ent, gentity_t *activator, gitem_t *item, int price) {
	if (!activator || !item) return qfalse;

	const int weapon = item->giTag;
	const int ammoIndex = BG_FindAmmoForWeapon(weapon);
	int maxAmmo;
	int upgradeLevel;

	if (weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS || ammoIndex < 0)
		return qfalse;

	// Use fallback price if undefined
	if (price <= 0) {
		price = Survival_GetDefaultWeaponPrice(weapon);
	}

	upgradeLevel = activator->client->ps.weaponUpgraded[weapon];

	// Special handling: grenades (no new weapon granted)
	if (item->giType == IT_AMMO && (
		weapon == WP_GRENADE_LAUNCHER ||
		weapon == WP_GRENADE_PINEAPPLE
	)) {
		maxAmmo = BG_GetMaxAmmo(&activator->client->ps, weapon, LT_AMMO_BONUS_MULTIPLIER);

		if (activator->client->ps.ammoclip[weapon] >= maxAmmo) {
			return qfalse; // Already full
		}

		if (COM_BitCheck(activator->client->ps.weapons, weapon)) {
			if (upgradeLevel >= 1) {
				price = survCfg.priceAmmoUpgraded * upgradeLevel;
			} else {
				price /= 2; // Discount if already owned
			}
		}

		if (activator->client->ps.persistant[PERS_SCORE] < price) {
			G_AddEvent(activator, EV_GENERAL_SOUND, G_SoundIndex("sound/items/use_nothing.wav"));
			return qfalse;
		}

		Survival_AwardScore(activator, -price);
		Add_Ammo(activator, weapon, maxAmmo, qtrue);
		Add_Ammo(activator, weapon, maxAmmo, qfalse);

		G_AddPredictableEvent(activator, EV_ITEM_PICKUP, item - bg_itemlist);
		trap_SendServerCommand(-1, "mu_play sound/misc/buy.wav 0\n");

		return qtrue;
	}

	// Already owns weapon — refill ammo only
	if (COM_BitCheck(activator->client->ps.weapons, weapon)) {
		// Adjust refill price
		if (upgradeLevel >= 1) {
			price = survCfg.priceAmmoUpgraded * upgradeLevel;
		} else {
			price /= 2;
		}

		if (activator->client->ps.persistant[PERS_SCORE] < price) {
			G_AddEvent(activator, EV_GENERAL_SOUND, G_SoundIndex("sound/items/use_nothing.wav"));
			return qfalse;
		}

		maxAmmo = BG_GetMaxAmmo(&activator->client->ps, weapon, LT_AMMO_BONUS_MULTIPLIER);
		if (activator->client->ps.ammo[weapon] >= maxAmmo) {
			G_AddEvent(activator, EV_GENERAL_SOUND, G_SoundIndex("sound/items/use_nothing.wav"));
			return qfalse; // Already full
		}

		Survival_AwardScore(activator, -price);

		Add_Ammo(activator, weapon, maxAmmo, qtrue);
		Add_Ammo(activator, weapon, maxAmmo, qfalse);

		G_AddPredictableEvent(activator, EV_ITEM_PICKUP, item - bg_itemlist);
		trap_SendServerCommand(-1, "mu_play sound/misc/buy.wav 0\n");

		return qtrue;
	}

	// Buying a new weapon
	if (activator->client->ps.persistant[PERS_SCORE] < price) {
		G_AddEvent(activator, EV_GENERAL_SOUND, G_SoundIndex("sound/items/use_nothing.wav"));
		return qfalse;
	}

	Survival_AwardScore(activator, -price);

	Give_Weapon_New_Inventory(activator, weapon, qfalse);

	maxAmmo = BG_GetMaxAmmo(&activator->client->ps, weapon, LT_AMMO_BONUS_MULTIPLIER);
	Add_Ammo(activator, weapon, maxAmmo, qtrue);
	Add_Ammo(activator, weapon, maxAmmo, qfalse);

	G_AddPredictableEvent(activator, EV_ITEM_PICKUP, item - bg_itemlist);
	trap_SendServerCommand(-1, "mu_play sound/misc/buy.wav 0\n");

	return qtrue;
}
/*
============
Survival_HandleArmorPurchase
============
*/
qboolean Survival_HandleArmorPurchase(gentity_t *activator, gitem_t *item, int price) {
	if (!activator || !item || !activator->client) return qfalse;

	if (activator->client->ps.stats[STAT_ARMOR] >= 200)
		return qfalse;

	// Fallback price if not set by mapper
	if (price <= 0)
		price = survCfg.priceArmor;

	// Check score
	if (activator->client->ps.persistant[PERS_SCORE] < price) {
		trap_SendServerCommand(-1, "mu_play sound/items/use_nothing.wav 0\n");
		return qfalse;
	}

	// Deduct, apply, and notify
	Survival_AwardScore(activator, -price);
	activator->client->ps.stats[STAT_ARMOR] = 200;

	G_AddPredictableEvent(activator, EV_ITEM_PICKUP, item - bg_itemlist);
	trap_SendServerCommand(-1, "mu_play sound/misc/buy.wav 0\n");

	return qtrue;
}

/*
============
Survival_GetDefaultPerkPrice
============
*/
static int Survival_GetCompiledInPerkPrice(int perk) {
	switch (perk) {
		case PERK_SECONDCHANCE:    return 150;
		case PERK_RUNNER:          return 200;
		case PERK_SCAVENGER:       return 250;
		case PERK_WEAPONHANDLING:  return 300;
		case PERK_RIFLING:         return 350;
		case PERK_RESILIENCE:      return 400;
		default:                   return 200;
	}
}

/*
============
Survival_GetDefaultPerkPrice

  survCfg.perkPrice[] override (set via "perk_price.<name>" in a .surv file)
  takes priority; otherwise falls back to the compiled-in table above.
============
*/
int Survival_GetDefaultPerkPrice(int perk) {
	if (perk > PERK_NONE && perk < NUM_PERKS && survCfg.perkPrice[perk] >= 0) {
		return survCfg.perkPrice[perk];
	}
	return Survival_GetCompiledInPerkPrice(perk);
}

#define PERK_LEVEL_NONE  0
#define PERK_LEVEL_BASE  1
#define PERK_LEVEL_PRO   2

/*
============
Survival_HandlePerkPurchase
============
*/
qboolean Survival_HandlePerkPurchase(gentity_t *activator, gitem_t *item, int price) {
    if (!activator || !item || item->giType != IT_PERK)
        return qfalse;

    int perk = item->giTag;
    int curLevel = activator->client->ps.perks[perk];

    // Determine what we're buying: base (0->1) or pro (1->2)
    int targetLevel = 0;
    if (curLevel <= 0) {
        targetLevel = 1;          // buy base
    } else if (curLevel == 1) {
        targetLevel = 2;          // upgrade to pro
    } else {
        return qfalse;            // already pro (or higher)
    }

    // Only enforce max perks when buying a NEW perk (0->1).
    if (targetLevel == 1) {
        int perkCount = 0;
        for (int i = 0; i < MAX_PERKS; i++) {
            if (activator->client->ps.perks[i] > 0)
                perkCount++;
        }

        int maxPerks = (activator->client->ps.stats[STAT_PLAYER_CLASS] == PC_ENGINEER) ?
            survCfg.perksLimitEngineer : survCfg.perksLimit;

        if (perkCount >= maxPerks)
            return qfalse;
    }

    // Base price: entity override if >0, otherwise from .surv (svParams.*)
    if (price <= 0) {
        price = Survival_GetDefaultPerkPrice(perk);
    }

    // PRO costs double base price
    if (targetLevel == 2) {
        price *= 2;
    }

    // Not enough score?
    if (activator->client->ps.persistant[PERS_SCORE] < price) {
        G_AddEvent(activator, EV_GENERAL_SOUND, G_SoundIndex("sound/items/use_nothing.wav"));
        return qfalse;
    }

    // Grant / upgrade perk
    activator->client->ps.perks[perk] = targetLevel;
    activator->client->ps.stats[STAT_PERK] |= (1 << perk);
    Survival_AwardScore(activator, -price);

    G_AddPredictableEvent(activator, EV_ITEM_PICKUP, item - bg_itemlist);
    trap_SendServerCommand(-1, "mu_play sound/misc/buy_perk.wav 0\n");
    G_StartTimedAction(activator, ACTION_BUYPERK, PERKBUY_ACTION_MS);

    return qtrue;
}


/*
============
Survival_RefreshMaxHealth

Mirrors the "--- SURVIVAL maxHealth rules ---" block in ClientUserinfoChanged
(g_client.c) - recomputes pers.maxHealth/STAT_MAX_HEALTH from class + Resilience
level. target_buy only ever needs this one side effect of ClientUserinfoChanged;
calling the full function on every purchase also forced a needless model/anim
reparse (G_CheckForExistingModelInfo, plus the conditional legsAnim/torsoAnim
reset) and wiped session stats via Coop_DeleteStats.
============
*/
static void Survival_RefreshMaxHealth( gentity_t *activator ) {
	gclient_t *client = activator->client;

	if ( client->ps.perks[ PERK_RESILIENCE ] ) {
		client->pers.maxHealth = ( client->ps.stats[ STAT_PLAYER_CLASS ] == PC_SOLDIER ) ? 300 : 200;
	} else {
		client->pers.maxHealth = ( client->ps.stats[ STAT_PLAYER_CLASS ] == PC_SOLDIER ) ? 200 : 100;
	}

	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;
}

/*QUAKED target_buy (1 0 0) (-8 -8 -8) (8 8 8)
Gives the activator all the items pointed to.
*/
void Use_Target_buy(gentity_t *ent, gentity_t *other, gentity_t *activator) {
	if (!activator || !activator->client || !ent->buy_item) return;

	int itemIndex = 0;
	char *itemName = ent->buy_item;
	int price = (ent->price > 0) ? ent->price : 0;
	gitem_t *item = NULL;
	qboolean success = qfalse;

	// Special case: ammo
	if (!Q_stricmp(itemName, "ammo")) {
		if (Survival_HandleAmmoPurchase(ent, activator, price)) {
			// Survival_HandleAmmoPurchase already deducts its own (upgrade-aware) price internally
			Survival_RefreshMaxHealth(activator);
		}
		return;
	}

	// Special case: random weapon (mystery box)
	if (!Q_stricmp(itemName, "random_weapon")) {
		if (ent->rwbState == RWB_IDLE) {
			success = Survival_RandomBox_Start(ent, activator);
		} else if (ent->rwbState == RWB_LANDED) {
			success = Survival_RandomBox_Pickup(ent, activator);
		} else {
			// spinning: locked until it lands or resets
			trap_SendServerCommand(-1, "mu_play sound/items/use_nothing.wav 0\n");
		}
		if (success) {
			Survival_RefreshMaxHealth(activator);
		}
		return; // Don't flow into generic weapon handling
	}

	// Special case: upgrade weapon
	if (!Q_stricmp(itemName, "upgrade_weapon"))
	{
		if (Survival_HandleWeaponUpgrade(ent, activator, price))
		{
			Survival_RefreshMaxHealth(activator);
		}
		return;
	}

	// Special case: random perk
	if (!Q_stricmp(itemName, "random_perk")) {
		success = Survival_HandleRandomPerkBox(ent, activator, &itemName, &itemIndex);
		if (success) {
			Survival_RefreshMaxHealth(activator);
		}
		return;
	}

	// Special case: perk choice (small on-screen list, player picks which perk to buy)
	if (!Q_stricmp(itemName, "perk_choice")) {
		if (ent->pcActivator == activator->s.number) {
			success = Survival_HandlePerkChoiceConfirm(ent, activator);
		} else if (ent->pcActivator < 0) {
			Survival_HandlePerkChoiceOpen(ent, activator);
		} else {
			// someone else is currently browsing this machine
			trap_SendServerCommand(-1, "mu_play sound/items/use_nothing.wav 0\n");
		}
		if (success) {
			Survival_RefreshMaxHealth(activator);
		}
		return;
	}

	// Fallback: find item by name
	if (itemIndex <= 0) {
		for (int i = 1; bg_itemlist[i].classname; i++) {
			if (!Q_strcasecmp(itemName, bg_itemlist[i].classname)) {
				itemIndex = i;
				break;
			}
		}
	}

	if (itemIndex <= 0) return;
	item = &bg_itemlist[itemIndex];

	// Not enough points?
	if (activator->client->ps.persistant[PERS_SCORE] < price) {
		trap_SendServerCommand(-1, "mu_play sound/items/use_nothing.wav 0\n");
		return;
	}

	switch (item->giType) {
		case IT_WEAPON:
		case IT_AMMO:
			success = Survival_HandleWeaponOrGrenade(ent, activator, item, price);
			break;
		case IT_ARMOR:
			success = Survival_HandleArmorPurchase(activator, item, price);
			break;
		case IT_PERK:
			success = Survival_HandlePerkPurchase(activator, item, price);
			break;
		default:
			return;
	}

	if (success) {
		Survival_RefreshMaxHealth(activator);
	}
}

#define AXIS_OBJECTIVE      1
#define ALLIED_OBJECTIVE    2

void Touch_objective_info(gentity_t *ent, gentity_t *other, trace_t *trace) {
	gentity_t *buyEnt = NULL;
	gentity_t *funcUser = NULL;
	gentity_t *constructible = NULL;
	int price = 0;
	int ammoPrice = 0;
	int isWeapon = ent->isWeapon;
	const char *weaponName = ent->translation;
	const char *techName = NULL;
	const gitem_t *item = NULL;
	int upgradeLevel = 0;

	if (other->aiCharacter)
	{
		return;
	}

	// Try to find the linked target_buy
	for (int i = 0; i < level.num_entities; i++)
	{
		buyEnt = &g_entities[i];
		if (!buyEnt->inuse)
			continue;
		if (Q_stricmp(buyEnt->classname, "target_buy") != 0)
			continue;
		if (ent->target && buyEnt->targetname && Q_stricmp(ent->target, buyEnt->targetname) == 0)
		{
			techName = buyEnt->buy_item;
			price = buyEnt->price; // This can be 0, and fallback will trigger
			break;
		}
	}

	// If no target_buy was found but a target exists, try to find linked func_invisible_user
	if (!techName && ent->target)
	{
		for (int i = 0; i < level.num_entities; i++)
		{
			gentity_t *candidate = &g_entities[i];
			if (!candidate->inuse)
				continue;
			if (Q_stricmp(candidate->classname, "func_invisible_user") != 0)
				continue;
			if (candidate->targetname && Q_stricmp(ent->target, candidate->targetname) == 0)
			{
				funcUser = candidate;
				price = funcUser->price;
				break;
			}
		}
	}

	// If neither of those was found but a target exists, try a linked func_constructible
	if (!techName && !funcUser && ent->target)
	{
		for (int i = 0; i < level.num_entities; i++)
		{
			gentity_t *candidate = &g_entities[i];
			if (!candidate->inuse)
				continue;
			if (Q_stricmp(candidate->classname, "func_constructible") != 0)
				continue;
			if (candidate->targetname && Q_stricmp(ent->target, candidate->targetname) == 0)
			{
				constructible = candidate;
				break;
			}
		}
	}

	// Unbuilt and priced: show a price tip instead of falling through to the generic objective line
	if (constructible && !constructible->active && constructible->price > 0)
	{
		price = constructible->price;
	}

	// Reinforcement callers show live squad-status text instead of a price prompt when nobody's down
	if (funcUser && (funcUser->spawnflags & 32) && !AICast_ReinforceAvailable())
	{
		trap_SendServerCommand(other - g_entities, "cpbuy \"Squad At Full Strength\"");
		return;
	}

	if (other->client->ps.weapon > WP_NONE && other->client->ps.weapon < WP_NUM_WEAPONS)
	{
		upgradeLevel = other->client->ps.weaponUpgraded[other->client->ps.weapon];
	}

	// Handle special cases BEFORE item lookup
	if (techName) {
		if (!Q_stricmp(techName, "ammo")) {

		// Do not show a price for Lieutenant call-in weapons: not purchasable
		if (other->client->ps.weapon == WP_AIRSTRIKE || other->client->ps.weapon == WP_ARTY) {
			return;
		}
		
			price = (price > 0) ? price : Survival_GetDefaultWeaponPrice(other->client->ps.weapon) / 2;

			if (upgradeLevel >= 1)
			{
				price = survCfg.priceAmmoUpgraded * upgradeLevel;
			}

			if (weaponName && price > 0) {
				trap_SendServerCommand(other - g_entities, va(
					"cpbuy \"%s\nprice: %d\"",
					weaponName, price));
				return;
			}
		} else if (!Q_stricmp(techName, "random_weapon")) {
			if (buyEnt && buyEnt->rwbState == RWB_LANDED) {
				const char *pickupName = ( buyEnt->rwbChosenItemIndex > 0 && bg_itemlist[buyEnt->rwbChosenItemIndex].classname ) ?
					bg_itemlist[buyEnt->rwbChosenItemIndex].pickup_name : "Weapon";

				trap_SendServerCommand(other - g_entities, va(
					"cpbuy \"Press USE to pickup\n%s\"",
					pickupName));
				return;
			}

			if (buyEnt && buyEnt->rwbState == RWB_SPINNING) {
				return; // rolling - no tooltip
			}

			price = (price > 0) ? price : survCfg.priceRandomWeapon;
			if (weaponName && price > 0) {
				trap_SendServerCommand(other - g_entities, va(
					"cpbuy \"%s\nprice: %d\"",
					weaponName, price));
				return;
			}
		}
		else if (!Q_stricmp(techName, "upgrade_weapon"))
		{
			if (upgradeLevel >= 3)
			{
				return;
			}

			if (price <= 0)
			{
				price = survCfg.priceWeaponUpgrade * (upgradeLevel + 1);
			}

			if (weaponName && price > 0)
			{
				trap_SendServerCommand(other - g_entities, va(
															   "cpbuy \"%s\nprice: %d\"",
															   weaponName, price));
				return;
			}
		}
		else if (!Q_stricmp(techName, "random_perk"))
		{
			price = (price > 0) ? price : survCfg.priceRandomPerk;
			if (weaponName && price > 0)
			{
				trap_SendServerCommand(other - g_entities, va(
															   "cpbuy \"%s\nprice: %d\"",
															   weaponName, price));
				return;
			}
		}
		else if (!Q_stricmp(techName, "perk_choice"))
		{
			if (buyEnt && buyEnt->pcActivator == other->s.number) {
				buyEnt->nextthink = level.time + PERK_CHOICE_PROXIMITY_GRACE;
				return;
			}

			trap_SendServerCommand(other - g_entities, va(
				"cpbuy \"%s\nUSE to choose a perk\"",
				weaponName ? weaponName : "Perk Machine"));
			return;
		}
	}

	// Try to find the item definition
	if (techName) {
		item = BG_FindItem(techName);
	}

	if (!item && techName) {
		for (int i = 1; bg_itemlist[i].classname; i++) {
			if (!Q_stricmp(bg_itemlist[i].classname, techName)) {
				item = &bg_itemlist[i];
				break;
			}
		}
	}

	// If price not defined explicitly, fall back based on item type
	if (price <= 0 && item) {
		if (item->giType == IT_WEAPON || item->giType == IT_AMMO) {
			price = Survival_GetDefaultWeaponPrice(item->giTag);
		} else if (item->giType == IT_PERK) {
			price = Survival_GetDefaultPerkPrice(item->giTag);
		} else if (item->giType == IT_ARMOR) {
			price = survCfg.priceArmor;
		}
	}

	// Ammo price only applies to weapons
	ammoPrice = isWeapon ? price / 2 : 0;

	if (upgradeLevel >= 1)
	{
		ammoPrice = survCfg.priceAmmoUpgraded * upgradeLevel;
	}

	// Perk PRO tip override (dynamic string + dynamic price)
	if (item && item->giType == IT_PERK && weaponName) {
		int perkLevel = other->client->ps.perks[item->giTag];

		if (perkLevel <= 0) {
			if (price > 0) {
				trap_SendServerCommand(other - g_entities, va(
					"cpbuy \"%s\nprice: %d\"",
					weaponName, price));
				return;
			}
		} else if (perkLevel == 1) {
			if (price > 0) {
				trap_SendServerCommand(other - g_entities, va(
					"cpbuy \"%s ^PRO\nprice: %d\"",
					weaponName, price * 2));
				return;
			}
		} else {
			return;
		}
	}

	// Display custom tip if price and name are known
	if (price > 0 && weaponName) {
		if (isWeapon) {
			trap_SendServerCommand(other - g_entities, va(
				"cpbuy \"%s\nprice: %d\nammo_price: %d\"",
				weaponName, price, ammoPrice));
		} else {
			trap_SendServerCommand(other - g_entities, va(
				"cpbuy \"%s\nprice: %d\"",
				weaponName, price));
		}
		return;
	}

	// Otherwise fallback to standard objective info
	if (other->timestamp <= level.time) {
		other->timestamp = level.time + 4500;

		const char *msg = ent->track ? ent->track : va("objective #%i", ent->count);
		int teamFlag = (ent->spawnflags & AXIS_OBJECTIVE) ? 0 :
		               (ent->spawnflags & ALLIED_OBJECTIVE) ? 1 : -1;

		trap_SendServerCommand(other - g_entities, va("oid %d \"You are near %s\n\"", teamFlag, msg));
	}
}