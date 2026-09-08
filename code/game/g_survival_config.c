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

// g_survival_config.c - loads survCfg from maps/survival_default.surv + maps/<mapname>.surv, server-only, never networked to clients.

#include "g_local.h"
#include "g_survival.h"

survConfig_t survCfg;

// Manual-offset trick (like the codebase's FOFS()/CSFOFS()) instead of <stddef.h>'s offsetof, for QVM toolchain compatibility.
#define SCFOFS( x ) ( (size_t)&( ( (survCharCurve_t *)0 )->x ) )

typedef struct
{
	const char *key;
	int *field;
} survConfigKey_t;

// "<charname>.<field>" keys, e.g. "zombie_ghost.health_cap 300"
typedef struct
{
	const char *name;
	AICharacters_t character;
} survCharName_t;

static const survCharName_t survCharNames[] = {
	{ "soldier",       AICHAR_SOLDIER },
	{ "trench",        AICHAR_TRENCH },
	{ "eliteguard",    AICHAR_ELITEGUARD },
	{ "helga",         AICHAR_HELGA },
	{ "heinrich",      AICHAR_HEINRICH },
	{ "blackguard",    AICHAR_BLACKGUARD },
	{ "venom",         AICHAR_VENOM },
	{ "zombie",        AICHAR_ZOMBIE_SURV },
	{ "zombie_flame",  AICHAR_ZOMBIE_FLAME },
	{ "zombie_ghost",  AICHAR_ZOMBIE_GHOST },
	{ "warzombie",     AICHAR_WARZOMBIE },
	{ "protosoldier",  AICHAR_PROTOSOLDIER },
	{ "partisan",      AICHAR_PARTISAN },
	{ "loper",         AICHAR_LOPER },
	{ "loper_special", AICHAR_LOPER_SPECIAL },
	{ "priest",        AICHAR_PRIEST },
};
#define NUM_SURV_CHAR_NAMES ( sizeof( survCharNames ) / sizeof( survCharNames[0] ) )

typedef struct
{
	const char *suffix;
	size_t offset;
	qboolean isFloat;
} survCharFieldDef_t;

static const survCharFieldDef_t survCharFields[] = {
	{ "health_base_early",     SCFOFS( healthBaseEarly ),     qfalse },
	{ "health_per_step",       SCFOFS( healthPerStep ),       qfalse },
	{ "health_growth_base",    SCFOFS( healthGrowthBase ),    qfalse },
	{ "health_growth_rate",    SCFOFS( healthGrowthRate ),    qtrue },
	{ "health_cap",            SCFOFS( healthCap ),           qfalse },

	{ "run_speed_base",        SCFOFS( runSpeedBase ),        qtrue },
	{ "run_speed_per_step",    SCFOFS( runSpeedPerStep ),     qtrue },
	{ "run_speed_cap",         SCFOFS( runSpeedCap ),         qtrue },
	{ "sprint_speed_base",     SCFOFS( sprintSpeedBase ),     qtrue },
	{ "sprint_speed_per_step", SCFOFS( sprintSpeedPerStep ),  qtrue },
	{ "sprint_speed_cap",      SCFOFS( sprintSpeedCap ),      qtrue },
	{ "crouch_speed_base",     SCFOFS( crouchSpeedBase ),     qtrue },
	{ "crouch_speed_per_step", SCFOFS( crouchSpeedPerStep ),  qtrue },
	{ "crouch_speed_cap",      SCFOFS( crouchSpeedCap ),      qtrue },
};
#define NUM_SURV_CHAR_FIELDS ( sizeof( survCharFields ) / sizeof( survCharFields[0] ) )

// See SCFOFS above.
#define SKFOFS( x ) ( (size_t)&( ( (survCharSkillTier_t *)0 )->x ) )

// "<charname>.easy_<field>" / "<charname>.hard_<field>" keys, e.g. "eliteguard.hard_aggression_cap 1.0".
typedef struct
{
	const char *suffix;   // without the easy_/hard_ prefix
	size_t offset;        // within survCharSkillTier_t
} survCharSkillFieldDef_t;

static const survCharSkillFieldDef_t survCharSkillFields[] = {
	{ "aim_skill_base",      SKFOFS( aimSkillBase ) },
	{ "aim_skill_cap",       SKFOFS( aimSkillCap ) },
	{ "aim_accuracy_base",   SKFOFS( aimAccuracyBase ) },
	{ "aim_accuracy_cap",    SKFOFS( aimAccuracyCap ) },
	{ "attack_skill_base",   SKFOFS( attackSkillBase ) },
	{ "attack_skill_cap",    SKFOFS( attackSkillCap ) },
	{ "aggression_base",     SKFOFS( aggressionBase ) },
	{ "aggression_cap",      SKFOFS( aggressionCap ) },
	{ "reaction_time_base",  SKFOFS( reactionTimeBase ) },
	{ "reaction_time_floor", SKFOFS( reactionTimeFloor ) },
};
#define NUM_SURV_CHAR_SKILL_FIELDS ( sizeof( survCharSkillFields ) / sizeof( survCharSkillFields[0] ) )

// "weapon_price.<name> <price>" keys - short .surv-only labels, independent of item classnames.
typedef struct
{
	const char *name;
	int id;
} survNamedInt_t;

static const survNamedInt_t survWeaponNames[] = {
	{ "knife",             WP_KNIFE },
	{ "luger",             WP_LUGER },
	{ "silencer",          WP_SILENCER },
	{ "colt",              WP_COLT },
	{ "akimbo",            WP_AKIMBO },
	{ "tt33",              WP_TT33 },
	{ "dual_tt33",         WP_DUAL_TT33 },
	{ "revolver",          WP_REVOLVER },
	{ "hdm",               WP_HDM },
	{ "delisle",           WP_DELISLE },
	{ "delislescope",      WP_DELISLESCOPE },
	{ "sten",              WP_STEN },
	{ "mp40",              WP_MP40 },
	{ "mp34",              WP_MP34 },
	{ "thompson",          WP_THOMPSON },
	{ "ppsh",              WP_PPSH },
	{ "mauser",            WP_MAUSER },
	{ "mosin",             WP_MOSIN },
	{ "sniperrifle",       WP_SNIPERRIFLE },
	{ "snooperscope",      WP_SNOOPERSCOPE },
	{ "m1garand",          WP_M1GARAND },
	{ "g43",               WP_G43 },
	{ "m1941",             WP_M1941 },
	{ "m1941scope",        WP_M1941SCOPE },
	{ "fg42",              WP_FG42 },
	{ "mp44",              WP_MP44 },
	{ "bar",               WP_BAR },
	{ "m97",               WP_M97 },
	{ "auto5",             WP_AUTO5 },
	{ "panzerfaust",       WP_PANZERFAUST },
	{ "flamethrower",      WP_FLAMETHROWER },
	{ "venom",             WP_VENOM },
	{ "tesla",             WP_TESLA },
	{ "mg42m",             WP_MG42M },
	{ "browning",          WP_BROWNING },
	{ "grenade_launcher",  WP_GRENADE_LAUNCHER },
	{ "grenade_pineapple", WP_GRENADE_PINEAPPLE },
};
#define NUM_SURV_WEAPON_NAMES ( sizeof( survWeaponNames ) / sizeof( survWeaponNames[0] ) )

static const survNamedInt_t survPerkNames[] = {
	{ "secondchance",   PERK_SECONDCHANCE },
	{ "runner",         PERK_RUNNER },
	{ "scavenger",      PERK_SCAVENGER },
	{ "weaponhandling", PERK_WEAPONHANDLING },
	{ "rifling",        PERK_RIFLING },
	{ "resilience",     PERK_RESILIENCE },
};
#define NUM_SURV_PERK_NAMES ( sizeof( survPerkNames ) / sizeof( survPerkNames[0] ) )

static const survConfigKey_t survConfigKeys[] = {
	{ "prepare_time",              &survCfg.prepareTime },
	{ "intermission_time",         &survCfg.intermissionTime },
	{ "initial_killcount_req",     &survCfg.initialKillCountReq },
	{ "friendly_spawn_time",       &survCfg.friendlySpawnTime },

	{ "wave_eliteguard",           &survCfg.waveEliteGuard },
	{ "wave_trench",               &survCfg.waveTrench },
	{ "wave_blackguard",           &survCfg.waveBlackguard },
	{ "wave_venom",                &survCfg.waveVenom },
	{ "wave_proto",                &survCfg.waveProto },
	{ "wave_warriors",             &survCfg.waveWarriors },
	{ "wave_helga",                &survCfg.waveHelga },
	{ "wave_heinrich",             &survCfg.waveHeinrich },
	{ "wave_lopers",               &survCfg.waveLopers },
	{ "wave_flamers",              &survCfg.waveFlamers },
	{ "wave_priests",              &survCfg.wavePriests },

	{ "initial_soldiers",          &survCfg.initialSoldiers },
	{ "initial_trench",            &survCfg.initialTrench },
	{ "initial_zombies",           &survCfg.initialZombies },
	{ "initial_flamers",           &survCfg.initialFlamers },
	{ "initial_warriors",          &survCfg.initialWarriors },
	{ "initial_protos",            &survCfg.initialProtos },
	{ "initial_elites",            &survCfg.initialElites },
	{ "initial_blackguards",       &survCfg.initialBlackguards },
	{ "initial_venoms",            &survCfg.initialVenoms },
	{ "initial_lopers",            &survCfg.initialLopers },
	{ "initial_helgas",            &survCfg.initialHelgas },
	{ "initial_heinrichs",         &survCfg.initialHeinrichs },
	{ "initial_priests",           &survCfg.initialPriests },

	{ "max_soldiers",              &survCfg.maxSoldiers },
	{ "soldiers_increase",         &survCfg.soldiersIncrease },
	{ "max_zombies",               &survCfg.maxZombies },
	{ "zombies_increase",          &survCfg.zombiesIncrease },
	{ "max_flamers",               &survCfg.maxFlamers },
	{ "flamers_increase",          &survCfg.flamersIncrease },
	{ "max_warriors",              &survCfg.maxWarriors },
	{ "warriors_increase",         &survCfg.warriorsIncrease },
	{ "max_elites",                &survCfg.maxElites },
	{ "elites_increase",           &survCfg.elitesIncrease },
	{ "max_trench",                &survCfg.maxTrench },
	{ "trench_increase",           &survCfg.trenchIncrease },
	{ "max_blackguards",           &survCfg.maxBlackguards },
	{ "blackguards_increase",      &survCfg.blackguardsIncrease },
	{ "max_helgas",                &survCfg.maxHelgas },
	{ "helgas_increase",           &survCfg.helgasIncrease },
	{ "max_heinrichs",             &survCfg.maxHeinrichs },
	{ "heinrichs_increase",        &survCfg.heinrichsIncrease },
	{ "max_lopers",                &survCfg.maxLopers },
	{ "lopers_increase",           &survCfg.lopersIncrease },
	{ "max_venoms",                &survCfg.maxVenoms },
	{ "venoms_increase",           &survCfg.venomsIncrease },
	{ "max_protos",                &survCfg.maxProtos },
	{ "protos_increase",           &survCfg.protosIncrease },
	{ "max_priests",               &survCfg.maxPriests },
	{ "priests_increase",          &survCfg.priestsIncrease },

	{ "special_wave_chance",         &survCfg.specialWaveChance },
	{ "special_wave_min_start",      &survCfg.specialWaveMinStart },
	{ "special_wave_min_gap",        &survCfg.specialWaveMinGap },
	{ "special_wave_max_gap",        &survCfg.specialWaveMaxGap },
	{ "special_wave_count_initial",  &survCfg.specialWaveCountInitial },
	{ "special_wave_count_increase", &survCfg.specialWaveCountIncrease },
	{ "special_wave_count_max",      &survCfg.specialWaveCountMax },

	{ "ghost_teleport_min_range",  &survCfg.ghostTeleportMinRange },
	{ "ghost_teleport_max_range",  &survCfg.ghostTeleportMaxRange },
	{ "ghost_teleport_delay",      &survCfg.ghostTeleportDelay },

	{ "perks_limit",               &survCfg.perksLimit },
	{ "perks_limit_engineer",      &survCfg.perksLimitEngineer },
	{ "price_armor",               &survCfg.priceArmor },
	{ "price_weapon_upgrade",      &survCfg.priceWeaponUpgrade },
	{ "price_ammo_upgraded",       &survCfg.priceAmmoUpgraded },
	{ "price_random_weapon",       &survCfg.priceRandomWeapon },
	{ "price_random_perk",         &survCfg.priceRandomPerk },

	{ "rwb_spin_duration",         &survCfg.rwbSpinDuration },
	{ "rwb_spin_interval_min",     &survCfg.rwbSpinIntervalMin },
	{ "rwb_spin_interval_max",     &survCfg.rwbSpinIntervalMax },
	{ "rwb_decision_time",         &survCfg.rwbDecisionTime },
	{ "rwb_blink_start",           &survCfg.rwbBlinkStart },
	{ "rwb_blink_interval",        &survCfg.rwbBlinkInterval },
	{ "rwb_float_height",          &survCfg.rwbFloatHeight },
	{ "rwb_bob_height",            &survCfg.rwbBobHeight },
	{ "rwb_bob_speed",             &survCfg.rwbBobSpeed },

	{ "growth_curve_wave_threshold", &survCfg.growthCurveWaveThreshold },

	{ "hard_steps_bonus_base_early",    &survCfg.hardStepsBonusBaseEarly },
	{ "hard_steps_bonus_divisor_early", &survCfg.hardStepsBonusDivisorEarly },
	{ "hard_steps_bonus_base_late",     &survCfg.hardStepsBonusBaseLate },
	{ "hard_steps_bonus_divisor_late",  &survCfg.hardStepsBonusDivisorLate },

	{ "gameover_linger_time",      &survCfg.gameoverLingerTime },
	{ "gameover_cam_hold_time",    &survCfg.gameoverCamHoldTime },
	{ "gameover_cam3_hold_time",   &survCfg.gameoverCam3HoldTime },
	{ "gameover_fade_time",        &survCfg.gameoverFadeTime },
	{ "gameover_countdown",        &survCfg.gameoverCountdown },
};

#define NUM_SURV_CONFIG_KEYS ( sizeof( survConfigKeys ) / sizeof( survConfigKeys[0] ) )

/*
============
Surv_SetCharCurve

  Fills one survCfg.charCurve[] entry. Characters that don't get run/sprint/crouch
  speed scaling just pass 0 for all nine speed args (runSpeedCap of 0 means
  AICast_ApplySurvivalAttributes leaves that character's speed scales at 1.0).
============
*/
static void Surv_SetCharCurve( AICharacters_t ch,
	int healthBaseEarly, int healthPerStep, int healthGrowthBase, float healthGrowthRate, int healthCap,
	float runBase, float runPerStep, float runCap,
	float sprintBase, float sprintPerStep, float sprintCap,
	float crouchBase, float crouchPerStep, float crouchCap ) {

	survCharCurve_t *cc = &survCfg.charCurve[ch];

	cc->healthBaseEarly = healthBaseEarly;
	cc->healthPerStep = healthPerStep;
	cc->healthGrowthBase = healthGrowthBase;
	cc->healthGrowthRate = healthGrowthRate;
	cc->healthCap = healthCap;

	cc->runSpeedBase = runBase;
	cc->runSpeedPerStep = runPerStep;
	cc->runSpeedCap = runCap;
	cc->sprintSpeedBase = sprintBase;
	cc->sprintSpeedPerStep = sprintPerStep;
	cc->sprintSpeedCap = sprintCap;
	cc->crouchSpeedBase = crouchBase;
	cc->crouchSpeedPerStep = crouchPerStep;
	cc->crouchSpeedCap = crouchCap;
}

/*
============
Surv_SetCharSkillTier

  Fills one difficulty tier of survCfg.charSkill[]. aimSkill/aimAccuracy/attackSkill
  are always driven by the same base/cap pair in the compiled-in defaults (they
  always were - BG_SetBehaviorForSurvival never actually varies them from one
  another), so this only takes one "skill" pair and fans it out to all three;
  the three fields remain independently overridable per-field from a .surv file.
============
*/
static void Surv_SetCharSkillTier( survCharSkillTier_t *tier,
	float skillBase, float skillCap,
	float aggressionBase, float aggressionCap,
	float reactionBase, float reactionFloor ) {

	tier->aimSkillBase = tier->aimAccuracyBase = tier->attackSkillBase = skillBase;
	tier->aimSkillCap = tier->aimAccuracyCap = tier->attackSkillCap = skillCap;
	tier->aggressionBase = aggressionBase;
	tier->aggressionCap = aggressionCap;
	tier->reactionTimeBase = reactionBase;
	tier->reactionTimeFloor = reactionFloor;
}

/*
============
Surv_SetDefaults

  Compiled-in fallback values - what survival mode used before .surv files
  existed. These stay in effect for any key that neither
  survival_default.surv nor the map's own .surv file mentions.
============
*/
static void Surv_SetDefaults( void ) {
	survCfg.prepareTime = 15;
	survCfg.intermissionTime = 15;
	survCfg.initialKillCountReq = 5;
	survCfg.friendlySpawnTime = 15;

	survCfg.waveEliteGuard = 3;
	survCfg.waveTrench = 4;
	survCfg.waveBlackguard = 5;
	survCfg.waveVenom = 7;
	survCfg.waveProto = 9;
	survCfg.waveWarriors = 3;
	survCfg.waveHelga = 10;
	survCfg.waveHeinrich = 15;
	survCfg.waveLopers = 5;
	survCfg.waveFlamers = 8;
	survCfg.wavePriests = 6;

	survCfg.initialSoldiers = 5;
	survCfg.initialTrench = 0;
	survCfg.initialZombies = 5;
	survCfg.initialFlamers = 0;
	survCfg.initialWarriors = 0;
	survCfg.initialProtos = 0;
	survCfg.initialElites = 0;
	survCfg.initialBlackguards = 0;
	survCfg.initialVenoms = 0;
	survCfg.initialLopers = 0;
	survCfg.initialHelgas = 0;
	survCfg.initialHeinrichs = 0;
	survCfg.initialPriests = 0;

	survCfg.maxSoldiers = 10;      survCfg.soldiersIncrease = 1;
	survCfg.maxZombies = 20;       survCfg.zombiesIncrease = 1;
	survCfg.maxFlamers = 3;        survCfg.flamersIncrease = 1;
	survCfg.maxWarriors = 5;       survCfg.warriorsIncrease = 1;
	survCfg.maxElites = 4;         survCfg.elitesIncrease = 1;
	survCfg.maxTrench = 4;         survCfg.trenchIncrease = 1;
	survCfg.maxBlackguards = 4;    survCfg.blackguardsIncrease = 1;
	survCfg.maxHelgas = 1;         survCfg.helgasIncrease = 1;
	survCfg.maxHeinrichs = 1;      survCfg.heinrichsIncrease = 1;
	survCfg.maxLopers = 3;         survCfg.lopersIncrease = 1;
	survCfg.maxVenoms = 4;         survCfg.venomsIncrease = 1;
	survCfg.maxProtos = 2;         survCfg.protosIncrease = 1;
	survCfg.maxPriests = 3;        survCfg.priestsIncrease = 1;

	survCfg.specialWaveChance = 30;
	survCfg.specialWaveMinStart = 4;
	survCfg.specialWaveMinGap = 2;
	survCfg.specialWaveMaxGap = 5;
	survCfg.specialWaveCountInitial = 3;
	survCfg.specialWaveCountIncrease = 1;
	survCfg.specialWaveCountMax = 8;

	survCfg.ghostTeleportMinRange = 200;
	survCfg.ghostTeleportMaxRange = 900;
	survCfg.ghostTeleportDelay = 7000;

	survCfg.perksLimit = 3;
	survCfg.perksLimitEngineer = 4;
	survCfg.priceArmor = 150;
	survCfg.priceWeaponUpgrade = 1000;
	survCfg.priceAmmoUpgraded = 500;
	survCfg.priceRandomWeapon = 150;
	survCfg.priceRandomPerk = 250;

	survCfg.rwbSpinDuration = 5000;
	survCfg.rwbSpinIntervalMin = 70;
	survCfg.rwbSpinIntervalMax = 380;
	survCfg.rwbDecisionTime = 15000;
	survCfg.rwbBlinkStart = 10000;
	survCfg.rwbBlinkInterval = 150;
	survCfg.rwbFloatHeight = 28;
	survCfg.rwbBobHeight = 3;
	survCfg.rwbBobSpeed = 2200;

	survCfg.growthCurveWaveThreshold = 10;

	survCfg.hardStepsBonusBaseEarly = 2;
	survCfg.hardStepsBonusDivisorEarly = 2;
	survCfg.hardStepsBonusBaseLate = 4;
	survCfg.hardStepsBonusDivisorLate = 3;

	survCfg.gameoverLingerTime = 4000;
	survCfg.gameoverCamHoldTime = 4500;
	survCfg.gameoverCam3HoldTime = 2200;
	survCfg.gameoverFadeTime = 600;
	survCfg.gameoverCountdown = 5;
	Q_strncpyz( survCfg.gameoverMusic, "sound/music/s_longout", sizeof( survCfg.gameoverMusic ) );

	Com_Memset( survCfg.charCurve, 0, sizeof( survCfg.charCurve ) );

	Surv_SetCharCurve( AICHAR_SOLDIER,    20, 10,  20, 1.12f, 100,    0,0,0,  0,0,0,  0,0,0 );
	survCfg.charCurve[AICHAR_TRENCH] = survCfg.charCurve[AICHAR_SOLDIER];

	Surv_SetCharCurve( AICHAR_ELITEGUARD, 30, 12,  30, 1.13f, 150,    0,0,0,  0,0,0,  0,0,0 );

	// NOTE: Helga/Heinrich/Ghost's healthGrowthBase intentionally doesn't match healthBaseEarly - preserved from the original formula, not "fixed" here.
	Surv_SetCharCurve( AICHAR_HELGA,    4000, 14,  50, 1.13f, 20000,  0,0,0,  0,0,0,  0,0,0 );
	Surv_SetCharCurve( AICHAR_HEINRICH, 8000, 14,  50, 1.13f, 40000,  0,0,0,  0,0,0,  0,0,0 );

	Surv_SetCharCurve( AICHAR_BLACKGUARD, 80, 16,  80, 1.14f, 200,    0,0,0,  0,0,0,  0,0,0 );
	Surv_SetCharCurve( AICHAR_VENOM,     100, 18, 100, 1.15f, 500,    0,0,0,  0,0,0,  0,0,0 );
	Surv_SetCharCurve( AICHAR_PRIEST,    120, 16, 120, 1.15f, 500,    0,0,0,  0,0,0,  0,0,0 );

	Surv_SetCharCurve( AICHAR_ZOMBIE_SURV, 20, 10, 20, 1.12f, 200,
		0.8f, 0.03f, 1.2f,   1.2f, 0.04f, 1.6f,   0.25f, 0.02f, 0.5f );
	survCfg.charCurve[AICHAR_ZOMBIE_FLAME] = survCfg.charCurve[AICHAR_ZOMBIE_SURV];

	// see NOTE above - healthGrowthBase of 20 (not 80) matches the formula being replaced
	Surv_SetCharCurve( AICHAR_ZOMBIE_GHOST, 80, 10, 20, 1.12f, 300,
		0.8f, 0.03f, 1.2f,   1.2f, 0.04f, 1.6f,   0.25f, 0.02f, 0.5f );

	Surv_SetCharCurve( AICHAR_WARZOMBIE, 50, 14, 50, 1.14f, 500,
		0.8f, 0.04f, 1.6f,   1.2f, 0.05f, 2.0f,   0.25f, 0.03f, 0.75f );

	Surv_SetCharCurve( AICHAR_PROTOSOLDIER, 1000, 25, 1000, 1.18f, 2000,
		0.8f, 0.04f, 1.6f,   1.2f, 0.03f, 1.5f,   0.25f, 0.03f, 0.75f );

	Surv_SetCharCurve( AICHAR_PARTISAN,     500, 20, 500, 1.16f, 1000,  0,0,0,  0,0,0,  0,0,0 );
	Surv_SetCharCurve( AICHAR_LOPER,        250, 18, 250, 1.16f, 500,   0,0,0,  0,0,0,  0,0,0 );
	Surv_SetCharCurve( AICHAR_LOPER_SPECIAL, 25, 12,  25, 1.13f, 250,   0,0,0,  0,0,0,  0,0,0 );

	// AI skill tables (BG_SetBehaviorForSurvival); everyone else is fixed at max skill in code and never reads this table.
	Com_Memset( survCfg.charSkill, 0, sizeof( survCfg.charSkill ) );

	Surv_SetCharSkillTier( &survCfg.charSkill[AICHAR_SOLDIER].easy, 0.1f, 0.7f,  0.1f, 1.0f,  1.0f, 0.4f );
	Surv_SetCharSkillTier( &survCfg.charSkill[AICHAR_SOLDIER].hard, 0.4f, 0.8f,  0.4f, 1.0f,  0.8f, 0.4f );
	survCfg.charSkill[AICHAR_TRENCH] = survCfg.charSkill[AICHAR_SOLDIER];

	Surv_SetCharSkillTier( &survCfg.charSkill[AICHAR_ELITEGUARD].easy, 0.4f, 0.8f,  0.4f, 1.0f,  1.0f, 0.3f );
	Surv_SetCharSkillTier( &survCfg.charSkill[AICHAR_ELITEGUARD].hard, 0.5f, 0.9f,  0.5f, 1.0f,  0.6f, 0.2f );

	Surv_SetCharSkillTier( &survCfg.charSkill[AICHAR_BLACKGUARD].easy, 0.4f, 0.9f,  0.5f, 1.0f,  1.0f, 0.3f );
	Surv_SetCharSkillTier( &survCfg.charSkill[AICHAR_BLACKGUARD].hard, 0.55f, 0.9f, 0.55f, 1.0f, 0.7f, 0.3f );

	Surv_SetCharSkillTier( &survCfg.charSkill[AICHAR_VENOM].easy, 0.4f, 0.8f,  0.5f, 1.0f,  1.0f, 0.3f );
	Surv_SetCharSkillTier( &survCfg.charSkill[AICHAR_VENOM].hard, 0.5f, 0.8f,  0.6f, 1.0f,  0.7f, 0.3f );

	Surv_SetCharSkillTier( &survCfg.charSkill[AICHAR_PROTOSOLDIER].easy, 0.4f, 0.9f,  0.5f, 1.0f,  1.0f, 0.2f );
	Surv_SetCharSkillTier( &survCfg.charSkill[AICHAR_PROTOSOLDIER].hard, 0.6f, 0.9f,  0.6f, 1.0f,  0.8f, 0.2f );

	{
		int i;
		for ( i = 0; i < WP_NUM_WEAPONS; i++ ) {
			survCfg.weaponPrice[i] = -1;
		}
		for ( i = 0; i < NUM_PERKS; i++ ) {
			survCfg.perkPrice[i] = -1;
		}
	}
}

/*
============
Surv_ApplyDottedKey

  Handles "prefix.suffix value" keys:
    weapon_price.<name>        -> survCfg.weaponPrice[WP_*]
    perk_price.<name>          -> survCfg.perkPrice[PERK_*]
    <charname>.<field>         -> survCfg.charCurve[AICHAR_*].<field>
    <charname>.easy_<field>    -> survCfg.charSkill[AICHAR_*].easy.<field>
    <charname>.hard_<field>    -> survCfg.charSkill[AICHAR_*].hard.<field>

  Returns qtrue if the key was recognized (whether or not the value applied
  cleanly - unknown suffixes are still "handled" in the sense that the dotted
  form was understood, just logged as unknown).
============
*/
static qboolean Surv_ApplyDottedKey( const char *keybuf, const char *dot, const char *valueStr, const char *sourceForLog ) {
	char prefix[MAX_QPATH];
	const char *suffix = dot + 1;
	size_t prefixLen = (size_t)( dot - keybuf );
	int i;

	if ( prefixLen >= sizeof( prefix ) ) {
		prefixLen = sizeof( prefix ) - 1;
	}
	memcpy( prefix, keybuf, prefixLen );
	prefix[prefixLen] = 0;

	if ( !Q_stricmp( prefix, "weapon_price" ) ) {
		for ( i = 0; i < NUM_SURV_WEAPON_NAMES; i++ ) {
			if ( !Q_stricmp( suffix, survWeaponNames[i].name ) ) {
				survCfg.weaponPrice[survWeaponNames[i].id] = atoi( valueStr );
				return qtrue;
			}
		}
		G_Printf( "Survival: %s: unknown weapon_price name '%s', ignoring\n", sourceForLog, suffix );
		return qtrue;
	}

	if ( !Q_stricmp( prefix, "perk_price" ) ) {
		for ( i = 0; i < NUM_SURV_PERK_NAMES; i++ ) {
			if ( !Q_stricmp( suffix, survPerkNames[i].name ) ) {
				survCfg.perkPrice[survPerkNames[i].id] = atoi( valueStr );
				return qtrue;
			}
		}
		G_Printf( "Survival: %s: unknown perk_price name '%s', ignoring\n", sourceForLog, suffix );
		return qtrue;
	}

	for ( i = 0; i < NUM_SURV_CHAR_NAMES; i++ ) {
		if ( !Q_stricmp( prefix, survCharNames[i].name ) ) {
			int j;
			char *base = (char *)&survCfg.charCurve[survCharNames[i].character];

			for ( j = 0; j < NUM_SURV_CHAR_FIELDS; j++ ) {
				if ( !Q_stricmp( suffix, survCharFields[j].suffix ) ) {
					if ( survCharFields[j].isFloat ) {
						*(float *)( base + survCharFields[j].offset ) = (float)atof( valueStr );
					} else {
						*(int *)( base + survCharFields[j].offset ) = atoi( valueStr );
					}
					return qtrue;
				}
			}

			// not a curve field - maybe "easy_<field>" / "hard_<field>" (skill tier)
			{
				survCharSkillTier_t *tier = NULL;
				const char *skillSuffix = NULL;

				if ( !Q_stricmpn( suffix, "easy_", 5 ) ) {
					tier = &survCfg.charSkill[survCharNames[i].character].easy;
					skillSuffix = suffix + 5;
				} else if ( !Q_stricmpn( suffix, "hard_", 5 ) ) {
					tier = &survCfg.charSkill[survCharNames[i].character].hard;
					skillSuffix = suffix + 5;
				}

				if ( tier ) {
					char *skillBase = (char *)tier;

					for ( j = 0; j < NUM_SURV_CHAR_SKILL_FIELDS; j++ ) {
						if ( !Q_stricmp( skillSuffix, survCharSkillFields[j].suffix ) ) {
							*(float *)( skillBase + survCharSkillFields[j].offset ) = (float)atof( valueStr );
							return qtrue;
						}
					}
				}
			}

			G_Printf( "Survival: %s: unknown field '%s' for character '%s', ignoring\n", sourceForLog, suffix, prefix );
			return qtrue;
		}
	}

	return qfalse;
}

/*
============
Surv_ParseBuffer

  key value
  key value // trailing comment
  // full-line comment

  Two key shapes:
    flat_key value                    -> survConfigKeys[] table
    prefix.suffix value                -> Surv_ApplyDottedKey (per-character
                                           curves, per-weapon/per-perk prices)

  One key/value pair per line. Unknown keys and malformed lines are logged
  and skipped rather than treated as fatal, since a mapper's typo shouldn't
  take a whole file's worth of other overrides down with it.
============
*/
static void Surv_ParseBuffer( char *data, const char *sourceForLog ) {
	char keybuf[MAX_QPATH];
	const char *tok;
	const char *dot;
	int i;

	while ( 1 ) {
		tok = COM_ParseExt( &data, qtrue );
		if ( !tok[0] ) {
			break;
		}
		Q_strncpyz( keybuf, tok, sizeof( keybuf ) );

		tok = COM_ParseExt( &data, qfalse );
		if ( !tok[0] ) {
			G_Printf( "Survival: %s: key '%s' has no value on its line, ignoring\n", sourceForLog, keybuf );
			continue;
		}

		dot = strchr( keybuf, '.' );
		if ( dot ) {
			if ( !Surv_ApplyDottedKey( keybuf, dot, tok, sourceForLog ) ) {
				G_Printf( "Survival: %s: unknown key '%s', ignoring\n", sourceForLog, keybuf );
			}
			continue;
		}

		if ( !Q_stricmp( keybuf, "gameover_music" ) ) {
			Q_strncpyz( survCfg.gameoverMusic, tok, sizeof( survCfg.gameoverMusic ) );
			continue;
		}

		for ( i = 0; i < NUM_SURV_CONFIG_KEYS; i++ ) {
			if ( !Q_stricmp( keybuf, survConfigKeys[i].key ) ) {
				*survConfigKeys[i].field = atoi( tok );
				break;
			}
		}
		if ( i == NUM_SURV_CONFIG_KEYS ) {
			G_Printf( "Survival: %s: unknown key '%s', ignoring\n", sourceForLog, keybuf );
		}
	}
}

static qboolean Surv_LoadFile( const char *filename ) {
	fileHandle_t f;
	int len;
	char *buf;

	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( len < 0 ) {
		return qfalse;
	}

	G_Printf( "Survival: loading %s\n", filename );

	buf = G_Alloc( len + 1 );
	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );

	Surv_ParseBuffer( buf, filename );
	return qtrue;
}

/*
============
Survival_LoadConfig

  Server-side only. Seeds survCfg with the compiled-in defaults, then loads
  maps/survival_default.surv (applies to every map) and finally
  maps/<mapname>.surv (only if present - a map only needs to list the keys
  it wants to differ from the default file).
============
*/
void Survival_LoadConfig( void ) {
	vmCvar_t mapname;
	char filename[MAX_QPATH];

	Surv_SetDefaults();

	Surv_LoadFile( "maps/survival_default.surv" );

	trap_Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );
	Q_strncpyz( filename, "maps/", sizeof( filename ) );
	Q_strcat( filename, sizeof( filename ), mapname.string );
	Q_strcat( filename, sizeof( filename ), ".surv" );

	Surv_LoadFile( filename );
}
