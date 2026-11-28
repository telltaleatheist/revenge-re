#include "fake_backend.h"
#include "Data/CampaignStateData.h"
#include "Data/RosterData.h"
#include "Data/TacticalControlData.h"
#include "Data/TacticalUnitData.h"
#include "Tactical/CombatInternal.h"
#include "revenge/battletech.h"
#include "revenge/unit.h"
#include "revenge/upgrades.h"

#include <assert.h>
#include <string.h>

/* The UPGRADES switch upgrade_lance_pursuit (Tactical/UnitState.cpp,
 * ApplyLancePursuitUpgrade, after 2D1B:08FA SelectUnitTacticalTargets): the
 * eighth battle's stall, built by hand. The player's 'Mech (unit 0) has lost
 * a leg, units 1 and 2 keep formation, no unit has a target, and the one
 * enemy left (unit 19) stands far off, unseen. Each mode is its own run, as
 * the quiet's start is kept between passes.
 *
 *   off       with the switch off nothing changes, however long the quiet
 *   minute    on: nothing for 59 seconds of the battle clock, then the
 *             acting leader (unit 1) is sent at unit 19's cell, its own
 *             formation state 01h, and unit 2 is left to follow it
 *   walking   on: the player's 'Mech can walk: never
 *   seen      on: the enemy is in sight: never
 *   target    on: a unit of the player's side has a target: never
 *   restart   on: the clock going back (the battle replayed) starts the
 *             minute again
 */

static void Stall(void)
{
    uint16_t unit;

    FakeBackend_Reset();
    memset(PersistentUnitRoster, 0x00, sizeof(PersistentUnitRoster));
    memset(UnitLanceFormationState, 0x00, sizeof(UnitLanceFormationState));
    memset(UnitEnemyVisible, 0x00, sizeof(UnitEnemyVisible));
    memset(UnitEnemyContactEstablished, 0x00, sizeof(UnitEnemyContactEstablished));
    memset(UnitEnemyInProximityRange, 0x00, sizeof(UnitEnemyInProximityRange));
    for (unit = 0x00; unit < TACTICAL_UNIT_COUNT; ++unit)
    {
        UnitTacticalTarget[unit] = TACTICAL_TARGET_NONE;
        UnitAttackTarget[unit] = TACTICAL_TARGET_NONE;
    }
    /* Every unit's turn skipped (its experience's mask meets no tick), so
     * the original's pass leaves the targets as set here. */
    TacticalClockDivider = 0x0000;
    CampaignStageIndex = 0x0007;
    for (unit = 0x00; unit < 0x03; ++unit)
    {
        PersistentUnitRoster[unit].deploymentState = 0x01;
        PersistentUnitRoster[unit].walkMovement = 0x04;
        UnitLanceFormationState[unit] = 0x02;
        UnitMovementHeatExponent[unit] = 0x02;
        ScenarioUnitMapX[unit] = (uint8_t) (78 + unit);
        ScenarioUnitMapY[unit] = 30;
        UnitRouteTargetMapX[unit] = ScenarioUnitMapX[unit];
        UnitRouteTargetMapY[unit] = 30;
    }
    PersistentUnitRoster[0x00].walkMovement = 0x00;
    PersistentUnitRoster[19].deploymentState = 0x01;
    PersistentUnitRoster[19].walkMovement = 0x08;
    ScenarioUnitMapX[19] = 5;
    ScenarioUnitMapY[19] = 65;
}

/* One pass at 07:mm:ss of the battle clock. */
static void PassAt(uint8_t minute, uint8_t second)
{
    CampaignClockState[0] = 7;
    CampaignClockState[1] = minute;
    CampaignClockState[2] = second;
    SelectUnitTacticalTargets();
}

static int Sent(void)
{
    return UnitLanceFormationState[1] == 0x01 && UnitRouteTargetMapX[1] == 5 && UnitRouteTargetMapY[1] == 65;
}

static void Untouched(void)
{
    assert(UnitLanceFormationState[1] == 0x02 && UnitRouteTargetMapX[1] == 79 && UnitRouteTargetMapY[1] == 30);
    assert(UnitLanceFormationState[2] == 0x02 && UnitRouteTargetMapX[2] == 80);
}

/* Passes through a quiet of two minutes; the switch must not act. */
static void NeverActs(void)
{
    uint8_t second;

    for (second = 0; second < 60; second += 5)
    {
        PassAt(38, second);
        PassAt(39, second);
    }
    PassAt(40, 0);
    Untouched();
}

int main(int argc, char **argv)
{
    const char *mode = argc > 1 ? argv[1] : "minute";

    Stall();
    Upgrades_Set(UPGRADE_LANCE_PURSUIT, strcmp(mode, "off") != 0);
    if (strcmp(mode, "off") == 0)
        NeverActs();
    else if (strcmp(mode, "minute") == 0)
    {
        PassAt(38, 10);
        PassAt(38, 40);
        PassAt(39, 9);
        Untouched();
        PassAt(39, 10);
        assert(Sent());
        assert(UnitLanceFormationState[2] == 0x02 && UnitRouteTargetMapX[2] == 80);
        assert(UnitMovementHeatExponent[1] == 0x02);
        assert(UnitTacticalTarget[1] == TACTICAL_TARGET_NONE && UnitAttackTarget[1] == TACTICAL_TARGET_NONE);
        /* Once sent, the acting leader leads; the next quiet doesn't send it
         * again (its formation state is already 01h). */
        UnitRouteTargetMapX[1] = 40;
        PassAt(41, 0);
        PassAt(42, 30);
        assert(UnitRouteTargetMapX[1] == 40);
    }
    else if (strcmp(mode, "walking") == 0)
    {
        PersistentUnitRoster[0x00].walkMovement = 0x04;
        NeverActs();
    }
    else if (strcmp(mode, "seen") == 0)
    {
        UnitEnemyVisible[19] = TRUE;
        NeverActs();
    }
    else if (strcmp(mode, "target") == 0)
    {
        UnitTacticalTarget[2] = 19;
        NeverActs();
    }
    else if (strcmp(mode, "restart") == 0)
    {
        PassAt(38, 10);
        PassAt(38, 50);
        /* The battle replayed: its clock back at 07:34. */
        PassAt(34, 0);
        PassAt(34, 59);
        Untouched();
        PassAt(35, 0);
        assert(Sent());
    }
    else
        return 2;
    return 0;
}
