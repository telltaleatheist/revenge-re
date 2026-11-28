#include "fake_backend.h"
#include "rules_for_tests.h"
#include "Data/RosterData.h"
#include "Data/TacticalControlData.h"
#include "Data/TacticalGraphicsData.h"
#include "Data/StartupData.h"
#include "Data/TacticalMapData.h"
#include "Data/TacticalTables.h"
#include "Data/TacticalUnitData.h"
#include "Tactical/CombatInternal.h"
#include "Tactical/Tactical.h"
#include "revenge/random.h"
#include "revenge/battletech.h"
#include "revenge/combat_data.h"
#include "revenge/unit.h"
#include "revenge/upgrades.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The UPGRADES switches for the player's lance run by the computer:
 * upgrade_lance_weapons (Tactical/Weapons.cpp, in 2D1B:14A4) and
 * upgrade_lance_tactics (Tactical/UnitState.cpp, at the end of 2D1B:0005 and
 * in 2D1B:08FA). Each must change a unit of the player's side (below 0Ch)
 * and leave an enemy's choice byte for byte as the original makes it. Each
 * mode is its own run; values are synthetic but for the rules' tables.
 *
 *   weapons_entries  every entry of every priority list: the switch's
 *                    choice is the original's but where the entry fires a
 *                    weapon it does not wait for, five entries, all list 14
 *   weapons_enemy    an enemy firing from list 14: the same state after
 *                    2D1B:14A4 with the switch on as off
 *   weapons_player   a unit of the player's side, the same: the state
 *                    differs (it fires the weapon the entry waits for)
 *   tactics_ratings  2D1B:0005's own ratings are the same on as off
 *   tactics_enemy    an enemy choosing how close to fight (2D1B:08FA), over
 *                    loadouts whose firepower the original misjudges: the
 *                    same choice and state on as off, every time
 *   tactics_player   a unit of the player's side, the same loadouts: some
 *                    choice differs
 */

constexpr uint16_t PLAYER_UNIT = 0x01;
constexpr uint16_t ENEMY_UNIT = 0x0D;
constexpr uint16_t SHIFTED_LIST = 0x0E;

static int failures;

static void Check(int condition, const char *what)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL: %s\n", what);
        ++failures;
    }
}

/* Everything 2D1B:14A4, 2D1B:0005 and 2D1B:08FA read or write, as one
 * block to set and compare. */
typedef struct Snapshot
{
    RevengeUnitRecord roster[TACTICAL_UNIT_COUNT];
    uint32_t availability[TACTICAL_UNIT_COUNT];
    uint8_t recovery[sizeof(UnitWeaponRecoveryCounters)];
    uint8_t counts[sizeof(UnitCachedWeaponCounts)];
    uint8_t random[sizeof(PseudoRandomState)];
    int16_t target[TACTICAL_UNIT_COUNT];
    uint8_t engagement[sizeof(UnitPreferredEngagementMode)];
    uint8_t ratings[sizeof(UnitCombatRatings)];
} Snapshot;

static void Take(Snapshot *snapshot)
{
    uint16_t unit;

    memset(snapshot, 0x00, sizeof(*snapshot));
    memcpy(snapshot->roster, PersistentUnitRoster, sizeof(snapshot->roster));
    memcpy(snapshot->availability, UnitWeaponAvailabilityMask, sizeof(snapshot->availability));
    memcpy(snapshot->recovery, UnitWeaponRecoveryCounters, sizeof(snapshot->recovery));
    memcpy(snapshot->counts, UnitCachedWeaponCounts, sizeof(snapshot->counts));
    memcpy(snapshot->random, PseudoRandomState, sizeof(snapshot->random));
    for (unit = 0x00; unit < TACTICAL_UNIT_COUNT; ++unit)
        snapshot->target[unit] = UnitTacticalTarget[unit];
    memcpy(snapshot->engagement, UnitPreferredEngagementMode, sizeof(snapshot->engagement));
    memcpy(snapshot->ratings, UnitCombatRatings, sizeof(snapshot->ratings));
}

/* Two units face to face, everything else empty: the acting unit and its
 * target, both deployed, in sight, seeking and destroying. */
static void Scene(uint16_t acting, uint16_t target, uint16_t distance)
{
    static const uint8_t RandomStart[sizeof(PseudoRandomState)] = { 0x34, 0x12, 0x76 };
    uint16_t unit;

    FakeBackend_Reset();
    memset(PersistentUnitRoster, 0x00, sizeof(PersistentUnitRoster));
    memset(UnitWeaponAvailabilityMask, 0x00, sizeof(UnitWeaponAvailabilityMask));
    memset(UnitWeaponRecoveryCounters, 0x00, sizeof(UnitWeaponRecoveryCounters));
    memset(UnitCachedWeaponCounts, 0x00, sizeof(UnitCachedWeaponCounts));
    memset(UnitCombatRatings, 0x00, sizeof(UnitCombatRatings));
    memset(UnitPreferredEngagementMode, 0x00, sizeof(UnitPreferredEngagementMode));
    memset(UnitEnemyVisible, 0x00, sizeof(UnitEnemyVisible));
    memset(UnitEnemyContactEstablished, 0x00, sizeof(UnitEnemyContactEstablished));
    memset(UnitEnemyInProximityRange, 0x00, sizeof(UnitEnemyInProximityRange));
    memcpy(PseudoRandomState, RandomStart, sizeof(PseudoRandomState));
    for (unit = 0x00; unit < TACTICAL_UNIT_COUNT; ++unit)
    {
        UnitTacticalTarget[unit] = TACTICAL_TARGET_NONE;
        UnitAttackTarget[unit] = TACTICAL_TARGET_NONE;
        UnitShutdownState[unit] = FALSE;
        UnitOverburnOrCloseAssaultEnabled[unit] = 0x00;
        UnitTacticalOrderMode[unit] = TACTICAL_ORDER_HOLD_POSITION;
    }
    TacticalClockDivider = 0xFFFF;
    for (unit = 0x00; unit < 0x02; ++unit)
    {
        const uint16_t index = unit == 0x00 ? acting : target;
        uint16_t family;

        PersistentUnitRoster[index].deploymentState = 0x01;
        PersistentUnitRoster[index].walkMovement = 0x04;
        /* Armour and structure everywhere, so a hit has a place to land. */
        memset(PersistentUnitRoster[index].currentDurability, 0x30, sizeof(PersistentUnitRoster[index].currentDurability));
        memset(PersistentUnitRoster[index].maximumDurability, 0x30, sizeof(PersistentUnitRoster[index].maximumDurability));
        for (family = 0x00; family < sizeof(PersistentUnitRoster[index].currentAmmunition) / sizeof(uint16_t); ++family)
            PersistentUnitRoster[index].currentAmmunition[family] = 0x14;
        UnitTacticalOrderMode[index] = TACTICAL_ORDER_SEEK_AND_DESTROY;
        UnitEnemyVisible[index] = TRUE;
        ScenarioUnitMapY[index] = 0x20;
    }
    ScenarioUnitMapX[acting] = 0x20;
    ScenarioUnitMapX[target] = (uint8_t) (0x20 + distance);
    ScenarioUnitFacing[acting] = (uint8_t) DetermineRelativeAttackDirection(
        ScenarioUnitMapX[acting], ScenarioUnitMapY[acting], ScenarioUnitMapX[target], ScenarioUnitMapY[target]);
    UnitTacticalTarget[acting] = (RevengeUnitSlot) target;
}

/* The selection mask (range and heat bits, 2D1B:14A4) of a unit at a
 * distance with this heat, and the heat and shortest distance whose mask
 * picks list 14 (distance -1 for none). */
static uint16_t SelectionMaskAt(uint16_t distance, uint16_t heat)
{
    uint16_t mask = heat == 0x00 ? 0x01 : heat < WEAPON_AI_LOW_HEAT_LIMIT ? 0x02
                                        : heat < WEAPON_AI_MEDIUM_HEAT_LIMIT ? 0x04
                                                                             : 0x08;

    if (distance < WEAPON_AI_LONG_RANGE_DISTANCE)
        mask |= 0x40;
    if (distance < WEAPON_AI_MEDIUM_RANGE_DISTANCE)
        mask |= 0x20;
    if (distance < WEAPON_AI_CLOSE_RANGE_DISTANCE)
        mask |= 0x10;
    return mask;
}

static int ShiftedListDistance(uint16_t *heat)
{
    uint16_t distance;

    for (*heat = 0x00; *heat <= WEAPON_AI_MEDIUM_HEAT_LIMIT; ++*heat)
        for (distance = 0x01; distance <= WEAPON_AI_MAXIMUM_ATTACK_DISTANCE; ++distance)
            if (WeaponPriorityProfileByRangeHeat[SelectionMaskAt(distance, *heat)] == SHIFTED_LIST)
                return distance;
    return -1;
}

/* An experience (0 to 3) whose heat limit (2D1B:14A4) lets a unit at this
 * heat fire, or -1. */
static int ExperienceFiringAt(uint16_t heat)
{
    int experience;

    for (experience = 0x00; experience < 0x04; ++experience)
        if (heat <= WeaponAIHeatLimitByExperience[experience + 0x01])
            return experience;
    return -1;
}

static const uint32_t *PriorityList(uint16_t profile)
{
    return &WeaponPriorityData.entries[(WeaponPriorityListPointers[profile].offset - 0x09C6) / sizeof(uint32_t)];
}

static void WeaponsEntries(void)
{
    uint16_t profile;
    int changed = 0;

    for (profile = 0x00; profile < WEAPON_PRIORITY_PROFILE_COUNT; ++profile)
    {
        const uint32_t *list = PriorityList(profile);
        const uint16_t count = WeaponPriorityCountTable[profile];
        uint16_t entry;

        for (entry = 0x00; entry < count; ++entry)
        {
            const uint32_t mask = list[entry];
            const uint16_t fired = (uint16_t) list[count + entry];
            const uint16_t chosen = LanceWeaponsUpgradeChoice(mask, fired);

            if ((mask & ((uint32_t) 0x01 << (fired - 0x01))) != 0x00)
                Check(chosen == fired, "an entry firing a weapon it waits for is kept");
            else
            {
                ++changed;
                Check(profile == SHIFTED_LIST, "only list 14 changes");
                Check(mask == ((uint32_t) 0x01 << (chosen - 0x01)), "the weapon the entry waits for is fired");
            }
        }
    }
    Check(changed == 5, "five entries change");
}

/* One call of 2D1B:14A4 for the acting unit, firing on list 14's first entry
 * (its weapon ready alone), with the switch as given. */
static void FireFromShiftedList(uint16_t acting, uint16_t target, int on, Snapshot *after)
{
    uint16_t heat = 0x00;
    const int distance = ShiftedListDistance(&heat);
    const int experience = ExperienceFiringAt(heat);
    const uint32_t *list = PriorityList(SHIFTED_LIST);
    const uint32_t ready = list[0];
    uint16_t weaponIndex;

    Check(distance > 0 && experience >= 0, "a distance and heat pick list 14, and a unit fires at that heat");
    if (distance <= 0 || experience < 0)
        exit(EXIT_FAILURE);
    Scene(acting, target, (uint16_t) distance);
    PersistentUnitRoster[acting].currentHeat = (uint8_t) heat;
    PersistentUnitRoster[acting].pilotExperience = (uint8_t) experience;
    for (weaponIndex = 0x00; weaponIndex < WEAPON_DEFINITION_COUNT; ++weaponIndex)
        if ((ready & ((uint32_t) 0x01 << weaponIndex)) != 0x00 ||
            weaponIndex + 0x01 == (uint16_t) list[WeaponPriorityCountTable[SHIFTED_LIST]])
            UnitCachedWeaponCounts[acting * WEAPON_DEFINITION_COUNT + weaponIndex] = 0x01;
    UnitWeaponAvailabilityMask[acting] = ready;
    Upgrades_Set(UPGRADE_LANCE_WEAPONS, on);
    (void) ExecuteUnitWeaponAI(acting);
    Take(after);
}

static int WeaponsSide(uint16_t acting, uint16_t target)
{
    Snapshot off;
    Snapshot on;

    FireFromShiftedList(acting, target, 0, &off);
    FireFromShiftedList(acting, target, 1, &on);
    return memcmp(&off, &on, sizeof(off)) != 0;
}

/* Both units' ratings from their weapons (2D1B:0005), then one pass of
 * 2D1B:08FA, with the switch as given. */
static void Judge(uint16_t acting, uint16_t target, const uint8_t *actingWeapons, const uint8_t *targetWeapons, int on,
    Snapshot *after)
{
    Upgrades_Set(UPGRADE_LANCE_TACTICS, on);
    Scene(acting, target, 0x04);
    /* The target holds: only the acting unit judges. */
    UnitTacticalOrderMode[target] = TACTICAL_ORDER_HOLD_POSITION;
    memcpy(&UnitCachedWeaponCounts[acting * WEAPON_DEFINITION_COUNT], actingWeapons, WEAPON_DEFINITION_COUNT);
    memcpy(&UnitCachedWeaponCounts[target * WEAPON_DEFINITION_COUNT], targetWeapons, WEAPON_DEFINITION_COUNT);
    RecalculateUnitCombatRatings(&PersistentUnitRoster[acting], acting, FALSE, FALSE);
    RecalculateUnitCombatRatings(&PersistentUnitRoster[target], target, FALSE, FALSE);
    UnitTacticalTarget[acting] = TACTICAL_TARGET_NONE;
    SelectUnitTacticalTargets();
    Take(after);
}

/* Over loadouts with a weapon h of the table's high half (10h-16h) and the
 * low-half weapon h - 10h, which 0800:072C's leaking mask also counts where
 * the range does not use it, against targets of one weapon each, one to
 * three of it: how many choices differ on and off for the acting unit. */
static int TacticsSide(uint16_t acting, uint16_t target)
{
    int differ = 0;
    uint16_t high;
    uint16_t opposing;
    uint8_t count;
    uint8_t opposingCount;

    for (high = 0x10; high < WEAPON_DEFINITION_COUNT; ++high)
        for (count = 0x01; count <= 0x03; ++count)
            for (opposing = 0x00; opposing < WEAPON_DEFINITION_COUNT; ++opposing)
                for (opposingCount = 0x01; opposingCount <= 0x03; ++opposingCount)
            {
                uint8_t actingWeapons[WEAPON_DEFINITION_COUNT] = {};
                uint8_t targetWeapons[WEAPON_DEFINITION_COUNT] = {};
                Snapshot off;
                Snapshot on;

                actingWeapons[high] = count;
                actingWeapons[high - 0x10] = 0x01;
                targetWeapons[opposing] = opposingCount;
                Judge(acting, target, actingWeapons, targetWeapons, 0, &off);
                Judge(acting, target, actingWeapons, targetWeapons, 1, &on);
                if (memcmp(&off, &on, sizeof(off)) != 0)
                    ++differ;
            }
    return differ;
}

int main(int argc, char **argv)
{
    const char *mode = argc > 1 ? argv[1] : "weapons_entries";

    static uint8_t terrain[0x8000];

    RequireRules();
    /* An open map, every cell clear, the first battle's terrain bounds and
     * no step limit (as tests/test_line_of_sight_map_width.cpp traces). */
    TerrainTypeMap = terrain;
    MapWidth = 0x80;
    MapHeight = 0x80;
    TerrainImpactAnimationTypeMinimum = 0xA0;
    TerrainImpactAnimationTypeMaximum = 0xC6;
    TerrainLineOfSightBlockingThreshold = 0xFF;
    TerrainLineOfSightMaximumSteps = 0x00;
    if (strcmp(mode, "weapons_entries") == 0)
        WeaponsEntries();
    else if (strcmp(mode, "weapons_enemy") == 0)
        Check(!WeaponsSide(ENEMY_UNIT, PLAYER_UNIT), "an enemy's fire is the same on as off");
    else if (strcmp(mode, "weapons_player") == 0)
        Check(WeaponsSide(PLAYER_UNIT, ENEMY_UNIT), "a player-side unit's fire changes");
    else if (strcmp(mode, "tactics_ratings") == 0)
    {
        uint8_t weapons[WEAPON_DEFINITION_COUNT] = {};
        uint8_t ratingsOff[sizeof(UnitCombatRatings)];

        weapons[0x02] = 0x02;
        weapons[0x11] = 0x01;
        Upgrades_Set(UPGRADE_LANCE_TACTICS, 0);
        Scene(PLAYER_UNIT, ENEMY_UNIT, 0x04);
        memcpy(&UnitCachedWeaponCounts[PLAYER_UNIT * WEAPON_DEFINITION_COUNT], weapons, sizeof(weapons));
        RecalculateUnitCombatRatings(&PersistentUnitRoster[PLAYER_UNIT], PLAYER_UNIT, FALSE, FALSE);
        memcpy(ratingsOff, UnitCombatRatings, sizeof(ratingsOff));
        Upgrades_Set(UPGRADE_LANCE_TACTICS, 1);
        Scene(PLAYER_UNIT, ENEMY_UNIT, 0x04);
        memcpy(&UnitCachedWeaponCounts[PLAYER_UNIT * WEAPON_DEFINITION_COUNT], weapons, sizeof(weapons));
        RecalculateUnitCombatRatings(&PersistentUnitRoster[PLAYER_UNIT], PLAYER_UNIT, FALSE, FALSE);
        Check(memcmp(ratingsOff, UnitCombatRatings, sizeof(ratingsOff)) == 0, "the original's ratings are the same on as off");
    }
    else if (strcmp(mode, "tactics_enemy") == 0)
        Check(TacticsSide(ENEMY_UNIT, PLAYER_UNIT) == 0, "an enemy's choice is the same on as off");
    else if (strcmp(mode, "tactics_player") == 0)
    {
        const int differ = TacticsSide(PLAYER_UNIT, ENEMY_UNIT);

        printf("tactics_player: %d loadouts judged differently\n", differ);
        Check(differ > 0, "a player-side unit's choice changes");
    }
    else
        return 2;
    if (failures == 0)
        printf("%s: ok\n", mode);
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
