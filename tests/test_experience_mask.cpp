// A unit's action mask with its experience past the mask table's four words.
// SelectUnitTacticalTargets (2D1B:08FA) lets a unit act on a subtick when its
// experience's word (DS:3CF6) has the clock divider's bit. The original
// indexes the table by the experience byte unchecked: an enemy deployed with
// experience 41h (the entry past the roster's end) reads the word at DS:3D78,
// past the table, and the port reads it from UnitActionMaskByExperiencePastEnd.
// Synthetic values only.

#include "revenge/original_data.h"
#include "../src/Original/REVENGE.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void SelectUnitTacticalTargets(void);

constexpr int SEEK_AND_DESTROY_ORDER = 0x02;
constexpr int ACTING_UNIT = 0x11;
constexpr int TARGET_UNIT = 0x01;
constexpr int PAST_END_EXPERIENCE = 0x41;
constexpr int MASK_BIT = 0x0008;
constexpr int OTHER_BIT = 0x0080;

static int failures;

static void Check(int condition, const char *what)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL: %s\n", what);
        ++failures;
    }
}

static void Prepare(word16 divider)
{
    word16 unitIndex;

    for (unitIndex = 0x00; unitIndex < 0x18; ++unitIndex)
    {
        PersistentUnitRoster[unitIndex].deploymentState = 0x00;
        UnitAttackTarget[unitIndex] = TACTICAL_TARGET_NONE;
        UnitTacticalTarget[unitIndex] = TACTICAL_TARGET_NONE;
        UnitEnemyVisible[unitIndex] = FALSE;
    }
    memset(UnitActionMaskByExperience, 0x00, sizeof(UnitActionMaskByExperience));
    memset(UnitActionMaskByExperiencePastEnd, 0x00, sizeof(UnitActionMaskByExperiencePastEnd));
    UnitActionMaskByExperiencePastEnd[PAST_END_EXPERIENCE - 0x04] = MASK_BIT;

    PersistentUnitRoster[ACTING_UNIT].deploymentState = 0x01;
    PersistentUnitRoster[ACTING_UNIT].pilotExperience = PAST_END_EXPERIENCE;
    UnitTacticalOrderMode[ACTING_UNIT] = SEEK_AND_DESTROY_ORDER;
    ScenarioUnitMapX[ACTING_UNIT] = 0x10;
    ScenarioUnitMapY[ACTING_UNIT] = 0x10;

    PersistentUnitRoster[TARGET_UNIT].deploymentState = 0x01;
    PersistentUnitRoster[TARGET_UNIT].pilotExperience = 0x00;
    UnitEnemyVisible[TARGET_UNIT] = TRUE;
    ScenarioUnitMapX[TARGET_UNIT] = 0x14;
    ScenarioUnitMapY[TARGET_UNIT] = 0x10;

    TacticalClockDivider = divider;
}

int main(void)
{
    Prepare(MASK_BIT);
    SelectUnitTacticalTargets();
    Check(UnitTacticalTarget[ACTING_UNIT] == TARGET_UNIT,
        "on a subtick its past-end word has, the unit acts and takes its target");

    Prepare(OTHER_BIT);
    SelectUnitTacticalTargets();
    Check(UnitTacticalTarget[ACTING_UNIT] == TACTICAL_TARGET_NONE,
        "on a subtick its past-end word lacks, the unit waits");

    if (failures == 0)
        printf("experience mask: ok\n");
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
