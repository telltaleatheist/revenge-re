// 2D1B: weapons
// Weapons.cpp
// Procedures preserved from REVENGE_2D1B_TACTICAL_COMBAT.c.

#include "CombatInternal.h"
#include "Data/TacticalTables.h"
#include "revenge/debug.h"

// The heat limit at `index` of the table at DS:03E0 (2D1B:14A4 reads it with
// the experience byte unchecked). Past the table's five bytes it is the
// original's bytes on to DS:04E0, the most an experience byte plus one
// reaches, but for the two variables among them, read live: the overview
// map's mode (DS:03E5-03E6) and the battle's lances (DS:03EA-03EC).
static uint8_t WeaponAIHeatLimit(uint16_t index)
{
    constexpr uint16_t TABLE_COUNT = 0x05;
    constexpr uint16_t OVERVIEW_MAP_MODE_AT = 0x05;  // DS:03E5
    constexpr uint16_t SCENARIO_LANCES_AT = 0x0A;    // DS:03EA

    if (index < TABLE_COUNT)
        return WeaponAIHeatLimitByExperience[index];
    if (index >= OVERVIEW_MAP_MODE_AT &&
        index < OVERVIEW_MAP_MODE_AT + sizeof(TacticalOverviewMapMode))
        return (uint8_t) (TacticalOverviewMapMode >> (0x08 * (index - OVERVIEW_MAP_MODE_AT)));
    if (index >= SCENARIO_LANCES_AT &&
        index < SCENARIO_LANCES_AT + sizeof(TacticalScenarioLanceEnabled))
        return TacticalScenarioLanceEnabled[index - SCENARIO_LANCES_AT];
    return WeaponAIHeatLimitByExperiencePastEnd[REVENGE_INDEX(WeaponAIHeatLimitByExperiencePastEnd, index - TABLE_COUNT)];
}

// 2D1B:14A4: word16 ExecuteUnitWeaponAI(word16 unitIndex)
// Classify range, heat, and attack direction, select the first ready weapon
// from the appropriate priority profile, and return whether an attack occurred.
// Verified against 2D1B:14A4-1803.
uint16_t ExecuteUnitWeaponAI(uint16_t unitIndex)
{
    RevengeUnitRecord *unitRecord = &PersistentUnitRoster[unitIndex];
    uint16_t mayAttack = FALSE;
    uint16_t attackOccurred = FALSE;

    if (UnitTacticalOrderMode[unitIndex] ==
            TACTICAL_ORDER_SEEK_AND_DESTROY ||
        UnitAttackTarget[unitIndex] != TACTICAL_TARGET_NONE ||
        UnitTacticalOrderMode[unitIndex] ==
            TACTICAL_ORDER_HOLD_POSITION)
    {
        mayAttack = TRUE;
    }

    if (mayAttack != FALSE)
    {
        if (UnitShutdownState[unitIndex] != FALSE)
        {
            mayAttack = FALSE;
        }
        else
        {
            uint16_t heatLimitIndex =
                unitRecord->pilotExperience + 0x01;
            if (UnitOverburnOrCloseAssaultEnabled[unitIndex] != 0x00)
                --heatLimitIndex;
            if (unitRecord->currentHeat >
                WeaponAIHeatLimit(heatLimitIndex))
            {
                mayAttack = FALSE;
            }
        }
    }

    if (mayAttack == FALSE)
        return FALSE;

    int16_t targetIndex = UnitTacticalTarget[unitIndex];
    if (targetIndex != TACTICAL_TARGET_MAP_CELL &&
        (targetIndex < 0x00 || targetIndex >= TACTICAL_TARGET_MAP_CELL ||
            PersistentUnitRoster[targetIndex].deploymentState == 0x00))
    {
        return FALSE;
    }

    uint16_t targetX;
    uint16_t targetY;
    if (targetIndex == TACTICAL_TARGET_MAP_CELL)
    {
        targetX = UnitTargetMapX[unitIndex];
        targetY = UnitTargetMapY[unitIndex];
    }
    else
    {
        targetX = ScenarioUnitMapX[targetIndex];
        targetY = ScenarioUnitMapY[targetIndex];
    }

    uint16_t distance = CalculateApproximateMapDistance(
        ScenarioUnitMapX[unitIndex],
        ScenarioUnitMapY[unitIndex],
        targetX,
        targetY);
    if (distance > WEAPON_AI_MAXIMUM_ATTACK_DISTANCE)
        return FALSE;

    uint16_t selectionMask = 0x00;
    if (distance < WEAPON_AI_LONG_RANGE_DISTANCE)
        selectionMask |= 0x40;
    if (distance < WEAPON_AI_MEDIUM_RANGE_DISTANCE)
        selectionMask |= 0x20;
    if (distance < WEAPON_AI_CLOSE_RANGE_DISTANCE)
        selectionMask |= 0x10;

    uint16_t currentHeat = unitRecord->currentHeat;
    if (currentHeat == 0x00)
        selectionMask |= 0x01;
    else if (currentHeat < WEAPON_AI_LOW_HEAT_LIMIT)
        selectionMask |= 0x02;
    else if (currentHeat < WEAPON_AI_MEDIUM_HEAT_LIMIT)
        selectionMask |= 0x04;
    else
        selectionMask |= 0x08;

    // Preserve the original ground-target quirk: direction classification uses
    // pseudo-slot 18h at the byte immediately after each 24-entry coordinate
    // array rather than UnitTargetMapX/Y, although distance used the latter.
    uint16_t directionTargetX = targetIndex < TACTICAL_UNIT_COUNT ? ScenarioUnitMapX[targetIndex] : DropShipMapX;
    uint16_t directionTargetY = targetIndex < TACTICAL_UNIT_COUNT ? ScenarioUnitMapY[targetIndex] : DropShipMapY;
    int16_t attackDirection = DetermineRelativeAttackDirection(
        ScenarioUnitMapX[unitIndex],
        ScenarioUnitMapY[unitIndex],
        directionTargetX,
        directionTargetY);
    if (attackDirection < 0x00 || attackDirection >= BATTLETECH_DIRECTION_COUNT)
        attackDirection = 0x00;

    uint16_t arcDirection = ScenarioUnitFacing[unitIndex] + 0x03;
    uint16_t arcStep;
    uint16_t inRestrictedArc = FALSE;
    for (arcStep = 0x00; arcStep < 0x03 && inRestrictedArc == FALSE; ++arcStep)
    {
        if ((arcDirection++ & (BATTLETECH_DIRECTION_COUNT - 0x01)) ==
            (uint16_t) attackDirection)
            inRestrictedArc = TRUE;
    }
    if (inRestrictedArc != FALSE)
        BuildArmWeaponAvailabilityMasks(unitIndex);

    uint16_t priorityProfile =
        WeaponPriorityProfileByRangeHeat[selectionMask];
    uint16_t priorityCount = WeaponPriorityCountTable[priorityProfile];
    DosAddress priorityListPointer =
        WeaponPriorityListPointers[priorityProfile];
    const uint32_t *priorityTable =
        &WeaponPriorityData.entries[(priorityListPointer.offset - 0x09C6) / sizeof(uint32_t)];
    uint32_t availability = UnitWeaponAvailabilityMask[unitIndex];
    uint16_t priorityIndex;
    uint16_t selectedWeaponId = 0x00;

    for (priorityIndex = 0x00;
        priorityIndex < priorityCount && selectedWeaponId == 0x00;
        ++priorityIndex)
    {
        uint32_t priorityMask = priorityTable[priorityIndex];
        if ((priorityMask & availability) != 0x00)
        {
            selectedWeaponId =
                (uint16_t) priorityTable[priorityCount + priorityIndex];
        }
    }

    if (selectedWeaponId == 0x00)
        return FALSE;

    if (unitRecord->damageModel == UNIT_DAMAGE_MODEL_INFANTRY &&
        UnitOverburnOrCloseAssaultEnabled[unitIndex] != 0x00)
    {
        // Close assault temporarily supplies one round for weapon index 0Ah and,
        // if accepted, lets a unit target resolve the reciprocal assault. The
        // command UI labels state zero and one with its two assault modes for
        // infantry; non-infantry uses the same byte for overburn.
        ++unitRecord->autocannon5Ammunition;
        attackOccurred = ResolveWeaponAttack(
            unitIndex, WEAPON_AUTOCANNON_5, targetIndex);
        if (attackOccurred != FALSE)
        {
            // The executable performs the reciprocal close-assault access even
            // for pseudo-target 18h; retain that original boundary behavior.
            RevengeUnitRecord *targetRecord = &PersistentUnitRoster[targetIndex];
            ++targetRecord->autocannon5Ammunition;
            if (ResolveWeaponAttack(
                    targetIndex, WEAPON_AUTOCANNON_5, unitIndex) == FALSE)
            {
                --targetRecord->autocannon5Ammunition;
            }
        }
        else if (attackOccurred == FALSE)
        {
            --unitRecord->autocannon5Ammunition;
        }
    }
    else
    {
        attackOccurred = ResolveWeaponAttack(
            unitIndex, selectedWeaponId - 0x01, targetIndex);
    }

    if (attackOccurred != FALSE)
    {
        if (targetIndex < TACTICAL_UNIT_COUNT)
            UnitWasAttackedThisTick[targetIndex] = TRUE;
        else
            GroundTargetWasAttacked = TRUE;
    }
    return attackOccurred;
}
