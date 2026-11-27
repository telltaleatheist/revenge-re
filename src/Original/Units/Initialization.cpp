// 2208: scenario unit initialization
/* Scenario-unit initialization procedure from original segment 2208. */

#include "Data/TacticalData.h"
#include "Tactical/Tactical.h"
#include "Units/Units.h"

void LoadUnitTemplateRecord(int16_t unitType);
void UpdateUnitSpriteForFacing(uint16_t unitIndex, uint16_t facing);
void RecalculateUnitCombatRatings(
    RevengeUnitRecord *unitRecord,
    uint16_t unitIndex,
    uint16_t rebuildWeaponCounts,
    uint16_t advanceRecovery);

/* 2208:2472-2587 */
void InitializeScenarioUnitFromTemplate(
    uint16_t unitType,
    uint16_t unitIndex)
{
    RevengeUnitRecord *unitRecord;
    uint8_t preservedAllegiance;
    uint16_t tacticalSpriteBase;

    unitRecord = &PersistentUnitRoster[unitIndex];
    preservedAllegiance = unitRecord->allegiance;
    if (unitType != UNIT_TEMPLATE_NONE)
    {
        LoadUnitTemplateRecord((int16_t) unitType);
        *unitRecord = EstimatedUnitRecord;
        unitRecord->deploymentState = 0x01;
        unitRecord->allegiance = preservedAllegiance;
    }

    tacticalSpriteBase = unitRecord->tacticalSpriteBase;
    UnitSpriteBaseId[unitIndex] = tacticalSpriteBase + 0x0100;
    if (unitIndex >= 0x000C)
    {
        if (unitRecord->damageModel == UNIT_DAMAGE_MODEL_INFANTRY)
            UnitSpriteBaseId[unitIndex] += 0x0002;
        else
            UnitSpriteBaseId[unitIndex] += 0x0008;
    }

    UnitTacticalOrderMode[unitIndex] = TACTICAL_ORDER_SEEK_AND_DESTROY;
    UnitAttackTarget[unitIndex] = TACTICAL_TARGET_NONE;
    unitRecord->heatAccumulatorQ8_8 = 0x0000;
    UpdateUnitSpriteForFacing(unitIndex, 0x0000);
    RecalculateUnitCombatRatings(
        unitRecord, unitIndex, 0x0001, 0x0000);
}
