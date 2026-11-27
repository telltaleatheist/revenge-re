// 2208, 2B34, 30F1: Units declarations
#ifndef REVENGE_UNITS_UNITS_H
#define REVENGE_UNITS_UNITS_H

#include "revenge/types.h"

void DrawCombatRatingDescription(
    int16_t combatRating,
    uint16_t x,
    uint16_t y,
    uint8_t foregroundColor,
    uint8_t backgroundColor);
void DrawTacticalUnitControlPanel(uint16_t unitIndex);
void PrepareScenarioUnitRosters(void);
uint8_t AssignScenarioLanceAllegiances(uint16_t scenarioAllegianceIndex);
void InitializeScenarioUnitFromTemplate(
    uint16_t unitType,
    uint16_t unitIndex);
void FlagDamagedWeaponRanges(
    const uint8_t *equipmentSlots,
    uint16_t slotCount);
void BuildDamagedWeaponRangeFlags(uint16_t unitIndex);
void BuildUndamagedUnitCombatEstimate(uint16_t unitIndex);
void CalculateLanceAverageCombatRatings(
    uint16_t leaderUnitIndex,
    uint16_t useActualUnitData);
uint16_t PrepareDisplayedUnitCombatRatings(uint16_t unitIndex);
void RenderFormattedLineBreak(void);

#endif
