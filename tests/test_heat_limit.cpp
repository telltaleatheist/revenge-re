// A unit's heat limit with its experience past the limit table's five bytes.
// ExecuteUnitWeaponAI (2D1B:14A4) holds a unit's fire while its heat is above
// the byte at DS:03E0 + experience + 1 (one less with overburn), the
// experience byte unchecked: an enemy deployed with experience 41h reads
// DS:0422, past the table. Past the table the port reads the original's bytes
// (WeaponAIHeatLimitByExperiencePastEnd), but DS:03E5-03E6 and DS:03EA-03EC
// are the overview map's mode and the battle's lances, read live.
//
// Whether a unit passed the heat check shows in its weapon availability: with
// its target in its rear arc, a unit past the check rebuilds the mask (to zero,
// with nothing in its arms); one held at the check leaves it as it was.
// Synthetic values only.

#include "revenge/original_data.h"
#include "REVENGE.h"
#include "Tactical/CombatInternal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

constexpr int SEEK_AND_DESTROY_ORDER = 0x02;
constexpr int ACTING_UNIT = 0x11;
constexpr int TARGET_UNIT = 0x01;
constexpr int WINDOW_START = 0x05;  // DS:03E5 is the table's index 5
constexpr uint32 UNTOUCHED = 0xA5A5A5A5UL;

static int failures;

static void Check(int condition, const char *what)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL: %s\n", what);
        ++failures;
    }
}

// Whether the acting unit, with this experience, overburn and heat, gets past
// the heat check.
static int PassesHeatCheck(byte experience, byte overburn, byte heat)
{
    word16 unitIndex;

    memset(PersistentUnitRoster, 0x00, sizeof(PersistentUnitRoster));
    for (unitIndex = 0x00; unitIndex < 0x18; ++unitIndex)
    {
        UnitShutdownState[unitIndex] = FALSE;
        UnitOverburnOrCloseAssaultEnabled[unitIndex] = 0x00;
        UnitAttackTarget[unitIndex] = TACTICAL_TARGET_NONE;
        UnitTacticalTarget[unitIndex] = TACTICAL_TARGET_NONE;
    }

    PersistentUnitRoster[ACTING_UNIT].deploymentState = 0x01;
    PersistentUnitRoster[ACTING_UNIT].pilotExperience = experience;
    PersistentUnitRoster[ACTING_UNIT].currentHeat = heat;
    UnitOverburnOrCloseAssaultEnabled[ACTING_UNIT] = overburn;
    UnitTacticalOrderMode[ACTING_UNIT] = SEEK_AND_DESTROY_ORDER;
    UnitTacticalTarget[ACTING_UNIT] = TARGET_UNIT;
    ScenarioUnitMapX[ACTING_UNIT] = 0x10;
    ScenarioUnitMapY[ACTING_UNIT] = 0x10;

    PersistentUnitRoster[TARGET_UNIT].deploymentState = 0x01;
    ScenarioUnitMapX[TARGET_UNIT] = 0x12;
    ScenarioUnitMapY[TARGET_UNIT] = 0x10;

    // The target in the rear arc: the arc's first direction is facing + 3.
    ScenarioUnitFacing[ACTING_UNIT] = (word16) ((DetermineRelativeAttackDirection(
                                                     ScenarioUnitMapX[ACTING_UNIT], ScenarioUnitMapY[ACTING_UNIT],
                                                     ScenarioUnitMapX[TARGET_UNIT], ScenarioUnitMapY[TARGET_UNIT]) -
                                                    0x03) &
        0x07);

    UnitWeaponAvailabilityMask[ACTING_UNIT] = UNTOUCHED;
    (void) ExecuteUnitWeaponAI(ACTING_UNIT);
    return UnitWeaponAvailabilityMask[ACTING_UNIT] != UNTOUCHED;
}

int main(void)
{
    memset(WeaponAIHeatLimitByExperiencePastEnd, 0xFF, sizeof(WeaponAIHeatLimitByExperiencePastEnd));

    // Experience 41h reads DS:0422, with overburn DS:0421.
    WeaponAIHeatLimitByExperiencePastEnd[0x42 - WINDOW_START] = 0x00;
    WeaponAIHeatLimitByExperiencePastEnd[0x41 - WINDOW_START] = 0x02;
    Check(PassesHeatCheck(0x41, 0x00, 0x00), "experience 41h, heat 0: past the check (limit 0)");
    Check(!PassesHeatCheck(0x41, 0x00, 0x01), "experience 41h, heat 1: held (limit 0)");
    Check(PassesHeatCheck(0x41, 0x01, 0x02), "experience 41h with overburn, heat 2: past (limit 2)");
    Check(!PassesHeatCheck(0x41, 0x01, 0x03), "experience 41h with overburn, heat 3: held (limit 2)");

    // DS:03E5: the overview map's mode, live (experience 4 reads index 5).
    TacticalOverviewMapMode = 0x0C07;
    Check(PassesHeatCheck(0x04, 0x00, 0x07), "experience 4, heat 7: past (the mode's low byte, 7)");
    Check(!PassesHeatCheck(0x04, 0x00, 0x08), "experience 4, heat 8: held (the mode's low byte, 7)");
    Check(!PassesHeatCheck(0x05, 0x00, 0x0D), "experience 5, heat 13: held (the mode's high byte, 12)");

    // DS:03EA-03EC: the battle's lances, live.
    TacticalScenarioLanceEnabled[0x00] = 0x03;
    TacticalScenarioLanceEnabled[0x02] = 0x09;
    Check(PassesHeatCheck(0x09, 0x00, 0x03), "experience 9, heat 3: past (lance 1's byte, 3)");
    Check(!PassesHeatCheck(0x09, 0x00, 0x04), "experience 9, heat 4: held (lance 1's byte, 3)");
    Check(!PassesHeatCheck(0x0B, 0x00, 0x0A), "experience 11, heat 10: held (lance 3's byte, 9)");

    // Within the table, the rules' limits.
    Check(PassesHeatCheck(0x00, 0x00, WeaponAIHeatLimitByExperience[0x01]),
        "experience 0 at its limit: past");
    Check(!PassesHeatCheck(0x00, 0x00, (byte) (WeaponAIHeatLimitByExperience[0x01] + 0x01)),
        "experience 0 over its limit: held");

    if (failures == 0)
        printf("heat limit: ok\n");
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
