// 2B2D, 2B34: unit status
// UnitStatus.cpp
// Reconstructed from REVENGE.EXE and its annotated instruction listing.
// using Reko decompiler version 0.11.4.0.

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "REVENGE.h"
#include "revenge/original_data.h"
#include "revenge/text.h"

#include <string.h>

enum
{
    WEAPON_STATUS_HAS_AMMUNITION_WEAPON = 0x80,
    WEAPON_STATUS_HAS_USABLE_AMMUNITION = 0x01
};

void CountUnitWeaponsByType(const RevengeUnitRecord *unitRecord);
uint16_t CountOrRemoveUnitHeatSinks(RevengeUnitRecord *unitRecord, uint16_t removeInstalledHeatSinks);
uint16_t CalculateUnitWeaponHeatScale(RevengeUnitRecord *unitRecord);
void RecalculateUnitCombatRatings(
    RevengeUnitRecord *unitRecord,
    uint16_t unitSlot,
    uint16_t rebuildWeaponCounts,
    uint16_t advanceRecovery);
uint16_t HasUnitLineOfSight(
    int16_t sourceUnitIndex,
    int16_t targetUnitIndex);
uint16_t IsCurrentLanceLeader(uint16_t unitIndex);
void DrawTacticalUnitControlPanel(uint16_t unitIndex);
uint16_t PrepareDisplayedUnitCombatRatings(uint16_t unitIndex);
void BuildDamagedWeaponRangeFlags(uint16_t unitIndex);
void DrawCombatRatingDescription(
    int16_t combatRating,
    uint16_t x,
    uint16_t y,
    uint8_t foregroundColor,
    uint8_t backgroundColor);
void HideMouseCursorOverlay(void);
void ShowMouseCursorOverlay(void);
void SelectAndClearTextWindow(uint16_t windowIndex);
void SelectTextWindow(uint16_t windowIndex);
void DrawActiveTextWindowBorder(uint16_t color);
void SelectTacticalTextForegroundColor(void);
void DrawTileAlignedText(
    const char *text,
    uint16_t tileX,
    uint16_t tileY,
    uint8_t foregroundColor,
    uint8_t backgroundColor);
int16_t UpdateUnitEnemyProximityState(uint16_t unitIndex);
int16_t UpdateUnitEnemyVisibility(uint16_t unitIndex);
uint16_t HasTacticalReferencePointLineOfSightToUnit(
    uint16_t referencePointIndex, uint16_t unitIndex);
char *IntegerToString(
    int16_t value, char *destination, uint16_t radix);

// 2B34:04B5: render the shared formatted-text line-break command at DS:38BE.
// Called from the tactical command and game-options panels in REVENGE_2208_TACTICAL_INTERACTION.c.
void RenderFormattedLineBreak(void)
{
    RenderFormattedText(FormattedLineBreak);
}

// 2B34:04C2: draw the complete tactical detail/control panel for one unit.
// Verified against ASM 2B34:04C2-0FDA. It deliberately retains the
// original DS tables, visibility tests, integer arithmetic and render order.
void DrawTacticalUnitControlPanel(uint16_t unitIndex)
{
    uint16_t lateCampaignVariant;
    uint16_t enemyUnit;
    uint16_t firstDetailRow;
    uint16_t friendlyHasLineOfSight;
    uint16_t revealEnemyDamage;
    uint16_t concealEnemyDamage;
    uint16_t panelSuppressed;
    uint16_t candidateIndex;
    RevengeUnitRecord *unitRecord;
    uint16_t unitType;
    uint16_t damageModel;
    uint16_t currentDurability;
    uint16_t maximumDurability;
    uint16_t durabilityBand;
    uint16_t conditionPercent;
    uint16_t movementDescription;
    uint16_t textColor;

    lateCampaignVariant =
        ScenarioFileSuffix >= 0x46 &&
        ScenarioFileSuffix < 0x54;
    enemyUnit = unitIndex >= ENEMY_FORCE_FIRST_UNIT;
    firstDetailRow = enemyUnit != FALSE ? 0x00 : 0x02;
    unitRecord = &PersistentUnitRoster[unitIndex];

    HideMouseCursorOverlay();
    friendlyHasLineOfSight = FALSE;
    for (candidateIndex = 0x00;
        candidateIndex < TACTICAL_FORCE_UNIT_COUNT;
        ++candidateIndex)
    {
        if (PersistentUnitRoster[candidateIndex].deploymentState != 0x00 &&
            PersistentUnitRoster[candidateIndex].beagleActiveProbe != 0x00 &&
            HasUnitLineOfSight(candidateIndex, unitIndex) != FALSE)
        {
            friendlyHasLineOfSight = TRUE;
        }
    }

    revealEnemyDamage = TRUE;
    if (enemyUnit != FALSE && friendlyHasLineOfSight == FALSE &&
        UnitEnemyInProximityRange[unitIndex] == 0x00)
    {
        revealEnemyDamage = FALSE;
    }
    concealEnemyDamage = PrepareDisplayedUnitCombatRatings(unitIndex);
    BuildDamagedWeaponRangeFlags(unitIndex);
    SelectAndClearTextWindow(0x03);
    DrawActiveTextWindowBorder(0x0E);
    ActiveTextForegroundColor = 0x0F;
    panelSuppressed = FALSE;

    if (enemyUnit != FALSE &&
        UpdateUnitEnemyProximityState(unitIndex) == FALSE &&
        UpdateUnitEnemyVisibility(unitIndex) == FALSE)
    {
        for (candidateIndex = 0x00; candidateIndex < 0x04;
            ++candidateIndex)
        {
            if (CampaignUpgradeAwards[candidateIndex] != 0x00 &&
                HasTacticalReferencePointLineOfSightToUnit(
                    candidateIndex, unitIndex) != FALSE)
            {
                DrawCenteredTextInActiveWindow(
                    DetectedByText,
                    0x05, 0x0F, 0x00);
                DrawCenteredTextInActiveWindow(
                    RadarSensorText,
                    0x06, 0x0F, 0x00);
                panelSuppressed = TRUE;
                break;
            }
        }
    }

    if (panelSuppressed == FALSE)
    {
        unitType = unitRecord->unitTypeId;
        DrawCenteredTextInActiveWindow(
            UnitNameTable[unitType],
            0x00, 0x0F, 0x00);
        if (enemyUnit == FALSE)
        {
            strcpy(
                (char *) SharedNumericTextBuffer,
                PilotSurnameTable[unitRecord->pilotId]);
            strcat(
                (char *) SharedNumericTextBuffer,
                PathSeparatorText);
            strcat(
                (char *) SharedNumericTextBuffer,
                PilotExperienceNameTable[unitRecord->pilotExperience]);
            DrawCenteredTextInActiveWindow(
                (const char *) SharedNumericTextBuffer,
                0x01, 0x0F, 0x00);
            DrawCenteredTextInActiveWindow(
                UnitAffiliationNameTable[unitRecord->allegiance],
                0x02, 0x0F, 0x00);
        }

        SelectTacticalTextForegroundColor();
        DrawTileAlignedText(
            (char *) UnitStatusRatingLabels,
            ActiveTextWindowLeftTile,
            ActiveTextWindowTopTile + firstDetailRow + 0x02,
            ((uint8_t) ActiveTextForegroundColor), 0x00);
        if (concealEnemyDamage != FALSE)
        {
            if (UpdateUnitEnemyProximityState(unitIndex) != FALSE)
            {
                DrawTileAlignedText(
                    (char *) UnitStatusDamageLabel,
                    ActiveTextWindowLeftTile,
                    ActiveTextWindowTopTile + 0x08,
                    ((uint8_t) ActiveTextForegroundColor), 0x00);
            }
        }
        else
        {
            uint16_t heatScale;
            DrawTileAlignedText(
                (char *) UnitStatusHeatDetailLabel,
                ActiveTextWindowLeftTile,
                ActiveTextWindowTopTile + 0x09,
                ((uint8_t) ActiveTextForegroundColor), 0x00);
            heatScale = unitRecord->currentHeat / BATTLEMECH_HEAT_SCALE;
            if (heatScale > 0x05)
                heatScale = 0x05;
            if (UnitShutdownState[unitIndex] != 0x00)
                heatScale = 0x06;
            DrawTileAlignedText(
                (char *) HeatScaleDescriptionTable[heatScale],
                ActiveTextWindowLeftTile + 0x05,
                ActiveTextWindowTopTile + 0x09,
                0x0F, 0x00);
        }

        textColor = DamagedWeaponRangeFlags[0x00] == 0x00 ? 0x0F : 0x0E;
        if (revealEnemyDamage == FALSE)
            textColor = 0x0F;
        DrawCombatRatingDescription(
            UnitCombatRatings[unitIndex].longFirepower,
            (ActiveTextWindowLeftTile + 0x04) << 0x03,
            (ActiveTextWindowTopTile + firstDetailRow + 0x02) << 0x03,
            (uint8_t) textColor, 0x00);
        textColor = DamagedWeaponRangeFlags[0x01] == 0x00 ? 0x0F : 0x0E;
        if (revealEnemyDamage == FALSE)
            textColor = 0x0F;
        DrawCombatRatingDescription(
            UnitCombatRatings[unitIndex].mediumFirepower,
            (ActiveTextWindowLeftTile + 0x04) << 0x03,
            (ActiveTextWindowTopTile + firstDetailRow + 0x03) << 0x03,
            (uint8_t) textColor, 0x00);
        textColor = DamagedWeaponRangeFlags[0x02] == 0x00 ? 0x0F : 0x0E;
        if (revealEnemyDamage == FALSE)
            textColor = 0x0F;
        DrawCombatRatingDescription(
            UnitCombatRatings[unitIndex].closeFirepower,
            (ActiveTextWindowLeftTile + 0x04) << 0x03,
            (ActiveTextWindowTopTile + firstDetailRow + 0x04) << 0x03,
            (uint8_t) textColor, 0x00);

        currentDurability = 0x00;
        maximumDurability = 0x00;
        damageModel = unitRecord->damageModel;
        for (candidateIndex = 0x00; candidateIndex < 0x0B; ++candidateIndex)
        {
            if (damageModel == UNIT_DAMAGE_MODEL_INFANTRY)
            {
                currentDurability += unitRecord->currentDurability[candidateIndex];
                maximumDurability += unitRecord->maximumDurability[candidateIndex];
            }
            else
            {
                currentDurability += unitRecord->currentArmor[candidateIndex];
                maximumDurability += unitRecord->maximumArmor[candidateIndex];
            }
        }
        durabilityBand = (uint16_t) ((int16_t) maximumDurability / 0x46);
        if ((int16_t) durabilityBand > 0x03)
            durabilityBand = 0x03;
        DrawTileAlignedText(
            (char *) DurabilityDescriptionTable[durabilityBand],
            ActiveTextWindowLeftTile + 0x04,
            ActiveTextWindowTopTile + firstDetailRow + 0x05,
            0x0F, 0x00);
        if (concealEnemyDamage != FALSE)
        {
            conditionPercent = (uint16_t) (((int32_t) (currentDurability * 0x0A) /
                                               (int16_t) maximumDurability) *
                0x0A);
        }
        else
        {
            conditionPercent = (uint16_t) ((int32_t) (currentDurability * 0x64) /
                (int16_t) maximumDurability);
        }
        SharedNumericTextBuffer[0x00] = 0x28;
        IntegerToString(
            (int16_t) conditionPercent,
            (char *) &SharedNumericTextBuffer[0x01], 0x0A);
        strcat(
            (char *) SharedNumericTextBuffer,
            PercentCloseText);
        if (concealEnemyDamage == FALSE ||
            UpdateUnitEnemyProximityState(unitIndex) != FALSE)
        {
            DrawTileAlignedText(
                (char *) SharedNumericTextBuffer,
                ActiveTextWindowLeftTile + 0x05 +
                    (uint16_t) strlen(DurabilityDescriptionTable[durabilityBand]),
                ActiveTextWindowTopTile + firstDetailRow + 0x05,
                0x0F, 0x00);
        }
        if (concealEnemyDamage != FALSE &&
            UpdateUnitEnemyProximityState(unitIndex) != FALSE)
        {
            uint16_t conditionBand = conditionPercent / 0x14;
            DrawTileAlignedText(
                (char *) ConditionDescriptionTable[conditionBand],
                ActiveTextWindowLeftTile + 0x04,
                ActiveTextWindowTopTile + 0x08,
                0x0F, 0x00);
        }

        textColor = unitRecord->walkMovement == 0x00 ? 0x0E : 0x0F;
        movementDescription = MovementRatingByWalkMP[unitRecord->walkMovement];
        DrawTileAlignedText(
            (char *) MovementDescriptionTable[movementDescription],
            ActiveTextWindowLeftTile + 0x04,
            ActiveTextWindowTopTile + firstDetailRow + 0x06,
            (uint8_t) textColor, 0x00);
        if (unitRecord->jumpMovement != 0x00)
        {
            DrawTileAlignedText(
                (char *) JumpEquipmentSuffix,
                ActiveTextWindowLeftTile + 0x04 +
                    (uint16_t) strlen(MovementDescriptionTable[movementDescription]),
                ActiveTextWindowTopTile + firstDetailRow + 0x06,
                (uint8_t) textColor, 0x00);
        }

        if (enemyUnit != FALSE)
        {
            if (friendlyHasLineOfSight != FALSE)
            {
                DrawTileAlignedText(
                    (char *) ScannedByActiveProbeText,
                    ActiveTextWindowLeftTile,
                    ActiveTextWindowTopTile + 0x0B,
                    0x0F, 0x00);
            }
            else if (concealEnemyDamage != FALSE &&
                UpdateUnitEnemyProximityState(unitIndex) == FALSE)
            {
                DrawCenteredTextInActiveWindow(
                    OutOfScanningText,
                    0x09, 0x0F, 0x00);
                DrawCenteredTextInActiveWindow(
                    RangeText,
                    0x0A, 0x0F, 0x00);
            }
        }
        else
        {
            ActiveTextForegroundColor = 0x0F;
            ActiveTextCursorColumn = 0x00;
            ActiveTextCursorRow = 0x0A;
            if (unitRecord->missileUpgradeState != 0x00)
                RenderFormattedText(
                    FriendlyEquipmentDescriptionTable[lateCampaignVariant]);
            if (unitRecord->beagleActiveProbe != 0x00)
                RenderFormattedText(BapEquipmentText);
            if (unitRecord->indirectFireSpotter != 0x00)
                RenderFormattedText(ArrowTagEquipmentText);
            if (unitRecord->unknown8F != 0x00)
                RenderFormattedText(AccuracyEquipmentText);
            if (unitRecord->doubleHeatSinks != 0x00)
                RenderFormattedText(DoubleHeatSinkEquipmentText);
            if (unitRecord->caseProtection != 0x00)
                RenderFormattedText(CaseEquipmentText);
        }

        if (enemyUnit != FALSE && concealEnemyDamage != FALSE &&
            UpdateUnitEnemyProximityState(unitIndex) != FALSE)
        {
            int16_t assessmentIndex = 0x06;
            int16_t renderedAssessment;
            ActiveTextCursorColumn = 0x00;
            ActiveTextCursorRow = 0x0A;
            if (conditionPercent != 0x64)
            {
                assessmentIndex = 0x07;
                if (unitRecord->walkMovement == 0x00)
                {
                    assessmentIndex = 0x0B;
                }
                else if (DamagedWeaponRangeFlags[0x00] != 0x00)
                {
                    assessmentIndex =
                        UnitCombatRatings[unitIndex].longFirepower == 0x00 ? 0x01 : 0x00;
                }
                else if (DamagedWeaponRangeFlags[0x01] != 0x00)
                {
                    assessmentIndex =
                        UnitCombatRatings[unitIndex].mediumFirepower == 0x00 ? 0x03 : 0x02;
                }
                else if (DamagedWeaponRangeFlags[0x02] != 0x00)
                {
                    assessmentIndex =
                        UnitCombatRatings[unitIndex].closeFirepower == 0x00 ? 0x05 : 0x04;
                }

                if (assessmentIndex == 0x07 &&
                    damageModel == UNIT_DAMAGE_MODEL_INFANTRY)
                    assessmentIndex = 0x0D;
                if (assessmentIndex == 0x07)
                {
                    uint16_t currentArmor =
                        unitRecord->currentArmor[BATTLEMECH_ARMOR_LEFT_LEG] +
                        unitRecord->currentArmor[BATTLEMECH_ARMOR_RIGHT_LEG];
                    uint16_t maximumArmor =
                        unitRecord->maximumArmor[BATTLEMECH_ARMOR_LEFT_LEG] +
                        unitRecord->maximumArmor[BATTLEMECH_ARMOR_RIGHT_LEG];
                    if (currentArmor != maximumArmor)
                    {
                        assessmentIndex = 0x08;
                        if ((uint16_t) (currentArmor << 0x01) < maximumArmor)
                            assessmentIndex = 0x09;
                    }
                    if (assessmentIndex == 0x07)
                    {
                        if (UnitShutdownState[unitIndex] != 0x00)
                        {
                            assessmentIndex = 0x0E;
                        }
                        else if ((unitRecord->currentInternal[BATTLEMECH_LOCATION_LEFT_ARM] != 0x00 &&
                                     unitRecord->currentInternal[BATTLEMECH_LOCATION_RIGHT_ARM] != 0x00) ||
                            damageModel != UNIT_DAMAGE_MODEL_BATTLEMECH)
                        {
                            uint16_t weaponDamageFlags = 0x00;
                            for (candidateIndex = 0x00;
                                candidateIndex < AMMUNITION_FAMILY_COUNT;
                                ++candidateIndex)
                            {
                                if (unitRecord->weaponFamilyCounts[candidateIndex] !=
                                    0x00)
                                {
                                    weaponDamageFlags |=
                                        WEAPON_STATUS_HAS_AMMUNITION_WEAPON;
                                    if (unitRecord->currentAmmunition[candidateIndex] !=
                                        0x00)
                                    {
                                        weaponDamageFlags |=
                                            WEAPON_STATUS_HAS_USABLE_AMMUNITION;
                                    }
                                }
                            }
                            if (weaponDamageFlags ==
                                WEAPON_STATUS_HAS_AMMUNITION_WEAPON)
                                assessmentIndex = 0x0C;
                        }
                        else
                        {
                            assessmentIndex = 0x0A;
                        }
                    }
                }
            }

            if (assessmentIndex >= 0x00 && assessmentIndex <= 0x0E)
            {
                renderedAssessment = assessmentIndex;
                if ((assessmentIndex == 0x08 || assessmentIndex == 0x09 ||
                        assessmentIndex == 0x0B) &&
                    damageModel != UNIT_DAMAGE_MODEL_BATTLEMECH)
                {
                    if (renderedAssessment == 0x0B)
                        renderedAssessment = 0x11;
                    if (renderedAssessment == 0x08)
                        renderedAssessment = 0x0F;
                    if (renderedAssessment == 0x09)
                        renderedAssessment = 0x10;
                }
                SelectTextWindow(0x15);
                ActiveTextCursorRow = 0x00;
                ActiveTextCursorColumn = 0x02;
                RenderFormattedText(
                    EnemyAssessmentTextTable[renderedAssessment]);
            }
        }
    }

    RecalculateUnitCombatRatings(
        unitRecord,
        unitIndex, TRUE, FALSE);
    ShowMouseCursorOverlay();
}

// 2B34:0FDB: center a string horizontally in the active tile window and draw
// it at the requested row offset. The two color arguments occupy words on the
// Borland stack even though the renderer consumes their low bytes.
void DrawCenteredTextInActiveWindow(
    const char *text,
    uint16_t rowOffset,
    uint16_t foregroundColor,
    uint16_t backgroundColor)
{
    uint16_t textLength = (uint16_t) strlen(text);
    uint16_t tileX = ActiveTextWindowLeftTile +
        ((ActiveTextWindowWidthTiles - textLength) >> 0x01);
    uint16_t tileY = ActiveTextWindowTopTile + rowOffset;

    DrawTileAlignedText(
        (char *) text,
        tileX,
        tileY,
        (uint8_t) foregroundColor,
        (uint8_t) backgroundColor);
}

// 2B2D:0077: word16 CountOrRemoveUnitHeatSinks(
//     RevengeUnitRecord *unitRecord, word16 removeInstalledHeatSinks)
// Called from:
//      CalculateUnitWeaponHeatScale
//      RecalculateUnitCombatRatings
// Verified against the aliased entry 2B34:0007-0120
// (logical address 2B2D:0077).
uint16_t CountOrRemoveUnitHeatSinks(
    RevengeUnitRecord *unitRecord,
    uint16_t removeInstalledHeatSinks)
{
    uint16_t heatSinkCount = unitRecord->engineHeatSinkCapacity;
    uint16_t equipmentSlot;

    for (equipmentSlot = 0x00;
        equipmentSlot < sizeof(unitRecord->equipmentSlots);
        ++equipmentSlot)
    {
        if (unitRecord->equipmentSlots[equipmentSlot] != EQUIPMENT_ID_HEAT_SINK)
            continue;

        ++heatSinkCount;
        if (removeInstalledHeatSinks != 0x00)
            unitRecord->equipmentSlots[equipmentSlot] = EQUIPMENT_SLOT_EMPTY;
    }

    return heatSinkCount;
}

// 2B2D:0191: word16 CalculateUnitWeaponHeatScale(RevengeUnitRecord *unitRecord)
// Called from:
//      DrawUnitStatusScreen
// Verified against 2B2D:0191-021A.
uint16_t CalculateUnitWeaponHeatScale(RevengeUnitRecord *unitRecord)
{
    int16_t calculatedHeat;
    uint16_t weaponIndex;
    uint16_t heatScaleIndex;

    CountUnitWeaponsByType(unitRecord);

    calculatedHeat = unitRecord->jumpMovement == 0x00 ? 0x02 : 0x05;

    for (weaponIndex = 0x00; weaponIndex < WEAPON_DEFINITION_COUNT; ++weaponIndex)
    {
        int16_t weaponHeat = WeaponDefinitions[weaponIndex].heat;
        uint16_t equippedCount = WeaponCountScratch[weaponIndex];
        calculatedHeat += weaponHeat * equippedCount;
    }

    calculatedHeat -= CountOrRemoveUnitHeatSinks(unitRecord, 0x00);
    if (calculatedHeat < 0x00)
        calculatedHeat = 0x00;
    else if (calculatedHeat > BATTLEMECH_MAXIMUM_HEAT)
        calculatedHeat = BATTLEMECH_MAXIMUM_HEAT;

    heatScaleIndex = (uint16_t) (calculatedHeat / 0x04);
    if (heatScaleIndex > 0x05)
        heatScaleIndex = 0x05;

    return heatScaleIndex;
}

// 2B34:01AB: int16 FindUnitWeaponByOrdinal(
//     Stack RevengeUnitRecord *unitRecord, Stack word16 weaponOccurrence,
//     Stack word16 removeWeapon)
// Called from the original assembly implementation of:
//      CountUnitWeaponsByType
int16_t FindUnitWeaponByOrdinal(
    RevengeUnitRecord *unitRecord,
    uint16_t weaponOccurrence,
    uint16_t removeWeapon)
{
    // V: scan one-based weapon equipment IDs 01h..17h in the eight stored
    // location groups. Vehicle records reuse the groups, but their exact
    // front/side/rear/turret projection still requires UI/damage evidence.
    uint8_t *locationEquipment[BATTLEMECH_LOCATION_COUNT] = {
        &unitRecord->headEquipment,
        unitRecord->leftArmEquipment,
        unitRecord->leftTorsoEquipment,
        unitRecord->centerTorsoEquipment,
        unitRecord->rightTorsoEquipment,
        unitRecord->rightArmEquipment,
        unitRecord->leftLegEquipment,
        unitRecord->rightLegEquipment
    };
    static const uint8_t locationLengths[BATTLEMECH_LOCATION_COUNT] = {
        BATTLEMECH_HEAD_EQUIPMENT_SLOT_COUNT,
        BATTLEMECH_ARM_EQUIPMENT_SLOT_COUNT,
        BATTLEMECH_TORSO_EQUIPMENT_SLOT_COUNT,
        BATTLEMECH_CENTER_TORSO_EQUIPMENT_SLOT_COUNT,
        BATTLEMECH_TORSO_EQUIPMENT_SLOT_COUNT,
        BATTLEMECH_ARM_EQUIPMENT_SLOT_COUNT,
        BATTLEMECH_LEG_EQUIPMENT_SLOT_COUNT,
        BATTLEMECH_LEG_EQUIPMENT_SLOT_COUNT
    };
    uint16_t locationIndex;
    uint16_t matchingWeaponIndex = 0x00;

    // Verified against 2B34:01AB-0374.
    for (locationIndex = 0x00;
        locationIndex < BATTLEMECH_LOCATION_COUNT;
        ++locationIndex)
    {
        uint16_t slotIndex;
        for (slotIndex = 0x00;
            slotIndex < locationLengths[locationIndex];
            ++slotIndex)
        {
            uint8_t equipmentId = locationEquipment[locationIndex][slotIndex];
            uint16_t weaponIndex;
            uint8_t ammunitionFamily;

            if (equipmentId < WEAPON_EQUIPMENT_ID(WEAPON_SMALL_LASER) ||
                equipmentId > WEAPON_EQUIPMENT_ID(WEAPON_SRM_6))
            {
                continue;
            }
            if (matchingWeaponIndex++ != weaponOccurrence)
                continue;

            SelectedUnitEquipmentLocation = locationIndex;
            if (removeWeapon == 0x00)
                return equipmentId;

            locationEquipment[locationIndex][slotIndex] = EQUIPMENT_SLOT_EMPTY;
            weaponIndex = WEAPON_INDEX_FROM_EQUIPMENT_ID(equipmentId);
            ammunitionFamily =
                WeaponDefinitions[weaponIndex].ammunitionFamilyId;
            if (ammunitionFamily == AMMUNITION_NONE)
                return equipmentId;

            CountUnitWeaponsByType(unitRecord);
            if (WeaponCountScratch[weaponIndex] == 0x00)
            {
                unitRecord->weaponFamilyCounts[AMMUNITION_INDEX(ammunitionFamily)] = 0x00;
                unitRecord->currentAmmunition[AMMUNITION_INDEX(ammunitionFamily)] = 0x00;
            }
            return equipmentId;
        }
    }

    return -0x01;
}

// 2B34:0422: void DrawCombatRatingDescription(uint16 combatRating,
//     uint16 x, uint16 y, byte foregroundColor, byte backgroundColor)
// Called from:
//      DrawUnitStatusScreen
void DrawCombatRatingDescription(
    int16_t combatRating,
    uint16_t x,
    uint16_t y,
    uint8_t foregroundColor,
    uint8_t backgroundColor)
{
    // V: DS:40CF is a seven-entry far-pointer table of combat rating
    // descriptions: six from the lowest rating up, then the one for no rating.
    uint16_t descriptionIndex;
    if (combatRating == 0x00)
        descriptionIndex = 0x06;
    else
    {
        descriptionIndex = combatRating / 0x09;
        if (descriptionIndex > 0x05)
            descriptionIndex = 0x05;
    }

    const char *description =
        CombatRatingDescriptionTable[descriptionIndex];
    DrawTextString(
        description,
        x,
        y,
        foregroundColor,
        backgroundColor);
}

// 2B34:0469: void CountUnitWeaponsByType(const RevengeUnitRecord *unitRecord)
// Called from:
//      DrawUnitStatusScreen
//      CalculateUnitWeaponHeatScale
//      FindUnitWeaponByOrdinal
//      RecalculateUnitCombatRatings
void CountUnitWeaponsByType(const RevengeUnitRecord *unitRecord)
{
    uint16_t weaponTypeIndex;
    uint16_t weaponOccurrence = 0x00;
    int16_t equipmentId;

    for (weaponTypeIndex = 0x00;
        weaponTypeIndex < WEAPON_DEFINITION_COUNT;
        ++weaponTypeIndex)
    {
        WeaponCountScratch[weaponTypeIndex] = 0x00;
    }

    for (;;)
    {
        equipmentId = FindUnitWeaponByOrdinal(
            (RevengeUnitRecord *) unitRecord,
            weaponOccurrence++, FALSE);
        if (equipmentId == -0x01)
            break;
        ++WeaponCountScratch[WEAPON_INDEX_FROM_EQUIPMENT_ID((uint16_t) equipmentId)];
    }
}

// 2B34:140F: void FlagDamagedWeaponRanges(
//     const byte *equipmentSlots, word16 slotCount)
// Called from:
//      BuildDamagedWeaponRangeFlags
void FlagDamagedWeaponRanges(
    const uint8_t *equipmentSlots,
    uint16_t slotCount)
{
    // V: DS:99C2..99C4 are long-, medium-, and close-range flags. Only
    // equipment entries carrying the damage bit participate in the status-panel
    // summary; the remaining seven bits contain the one-based weapon ID.
    uint8_t *damagedRangeFlags =
        (uint8_t *) DamagedWeaponRangeFlags;
    uint16_t slotIndex;

    for (slotIndex = 0x00; slotIndex < slotCount; ++slotIndex)
    {
        uint8_t equipmentId = equipmentSlots[slotIndex];
        uint16_t rangeClass;

        if ((equipmentId & EQUIPMENT_DAMAGED_FLAG) == 0x00)
            continue;

        equipmentId &= EQUIPMENT_ID_MASK;
        if (equipmentId < WEAPON_EQUIPMENT_ID(WEAPON_SMALL_LASER) ||
            equipmentId > WEAPON_EQUIPMENT_ID(WEAPON_SRM_6))
            continue;

        switch (WEAPON_INDEX_FROM_EQUIPMENT_ID(equipmentId))
        {
        case WEAPON_LARGE_LASER:
        case WEAPON_LARGE_PULSE_LASER:
        case WEAPON_AUTOCANNON_10:
            rangeClass = WEAPON_RANGE_MEDIUM;
            break;

        case WEAPON_ER_LARGE_LASER:
        case WEAPON_PPC:
        case WEAPON_ER_PPC:
        case WEAPON_AUTOCANNON_2:
        case WEAPON_AUTOCANNON_5:
        case WEAPON_LB_10X_AUTOCANNON:
        case WEAPON_GAUSS_RIFLE:
        case WEAPON_LRM_5:
        case WEAPON_LRM_10:
        case WEAPON_LRM_15:
        case WEAPON_LRM_20:
            rangeClass = WEAPON_RANGE_LONG;
            break;

        default:
            rangeClass = WEAPON_RANGE_CLOSE;
            break;
        }

        damagedRangeFlags[rangeClass] = TRUE;
    }
}

// 2B34:130C: void BuildDamagedWeaponRangeFlags(word16 unitIndex)
// Called from:
//      DrawTacticalUnitControlPanel
void BuildDamagedWeaponRangeFlags(uint16_t unitIndex)
{
    static const uint8_t equipmentGroupLengths[BATTLEMECH_LOCATION_COUNT] = {
        BATTLEMECH_ARM_EQUIPMENT_SLOT_COUNT,
        BATTLEMECH_ARM_EQUIPMENT_SLOT_COUNT,
        BATTLEMECH_TORSO_EQUIPMENT_SLOT_COUNT,
        BATTLEMECH_TORSO_EQUIPMENT_SLOT_COUNT,
        BATTLEMECH_CENTER_TORSO_EQUIPMENT_SLOT_COUNT,
        BATTLEMECH_LEG_EQUIPMENT_SLOT_COUNT,
        BATTLEMECH_LEG_EQUIPMENT_SLOT_COUNT,
        BATTLEMECH_HEAD_EQUIPMENT_SLOT_COUNT
    };
    RevengeUnitRecord *unitRecord = &PersistentUnitRoster[unitIndex];
    uint8_t *damagedRangeFlags =
        (uint8_t *) DamagedWeaponRangeFlags;
    uint8_t *equipmentGroups[BATTLEMECH_LOCATION_COUNT] = {
        unitRecord->leftArmEquipment,
        unitRecord->rightArmEquipment,
        unitRecord->leftTorsoEquipment,
        unitRecord->rightTorsoEquipment,
        unitRecord->centerTorsoEquipment,
        unitRecord->leftLegEquipment,
        unitRecord->rightLegEquipment,
        &unitRecord->headEquipment
    };
    uint16_t groupIndex;

    // The assembly uses DI=0..2. Reko's generated 6641h upper bound was a
    // malformed reconstruction of that three-byte clear.
    for (groupIndex = 0x00;
        groupIndex < WEAPON_RANGE_CLASS_COUNT;
        ++groupIndex)
        damagedRangeFlags[groupIndex] = FALSE;

    for (groupIndex = 0x00;
        groupIndex < BATTLEMECH_LOCATION_COUNT;
        ++groupIndex)
    {
        FlagDamagedWeaponRanges(
            equipmentGroups[groupIndex],
            equipmentGroupLengths[groupIndex]);
    }
}

// 2B34:11D2: void BuildUndamagedUnitCombatEstimate(word16 unitIndex)
// Called from:
//      PrepareDisplayedUnitCombatRatings
//      CalculateLanceAverageCombatRatings
void BuildUndamagedUnitCombatEstimate(uint16_t unitIndex)
{
    RevengeUnitRecord *sourceUnit = &PersistentUnitRoster[unitIndex];
    RevengeUnitRecord *estimatedUnit =
        (RevengeUnitRecord *) &EstimatedUnitRecord;
    uint16_t fieldIndex;

    // V: construct the information-safe enemy estimate consumed by the combat
    // rating calculator. Maximum armor becomes displayed current armor, weapon
    // family counts become widened ammunition values, and every equipment
    // damage bit is stripped. Fields irrelevant to rating calculation are not
    // copied into this shared scratch record.
    for (fieldIndex = 0x00;
        fieldIndex < BATTLEMECH_TORSO_EQUIPMENT_SLOT_COUNT;
        ++fieldIndex)
    {
        if (fieldIndex < BATTLEMECH_ARMOR_LOCATION_COUNT)
        {
            estimatedUnit->currentArmor[fieldIndex] =
                sourceUnit->maximumArmor[fieldIndex];
        }

        estimatedUnit->currentAmmunition[fieldIndex] =
            sourceUnit->weaponFamilyCounts[fieldIndex];
        estimatedUnit->leftTorsoEquipment[fieldIndex] =
            sourceUnit->leftTorsoEquipment[fieldIndex] & EQUIPMENT_ID_MASK;
        estimatedUnit->rightTorsoEquipment[fieldIndex] =
            sourceUnit->rightTorsoEquipment[fieldIndex] & EQUIPMENT_ID_MASK;

        if (fieldIndex < BATTLEMECH_ARM_EQUIPMENT_SLOT_COUNT)
        {
            estimatedUnit->leftArmEquipment[fieldIndex] =
                sourceUnit->leftArmEquipment[fieldIndex] & EQUIPMENT_ID_MASK;
            estimatedUnit->rightArmEquipment[fieldIndex] =
                sourceUnit->rightArmEquipment[fieldIndex] & EQUIPMENT_ID_MASK;
        }

        if (fieldIndex < BATTLEMECH_CENTER_TORSO_EQUIPMENT_SLOT_COUNT)
        {
            estimatedUnit->centerTorsoEquipment[fieldIndex] =
                sourceUnit->centerTorsoEquipment[fieldIndex] & EQUIPMENT_ID_MASK;
            estimatedUnit->leftLegEquipment[fieldIndex] =
                sourceUnit->leftLegEquipment[fieldIndex] & EQUIPMENT_ID_MASK;
            estimatedUnit->rightLegEquipment[fieldIndex] =
                sourceUnit->rightLegEquipment[fieldIndex] & EQUIPMENT_ID_MASK;
        }
    }

    estimatedUnit->headEquipment =
        sourceUnit->headEquipment & EQUIPMENT_ID_MASK;
}

// 2B34:10F4: void CalculateLanceAverageCombatRatings(
//     word16 leaderUnitIndex, word16 useActualUnitData)
// Called from:
//      PrepareDisplayedUnitCombatRatings
void CalculateLanceAverageCombatRatings(
    uint16_t leaderUnitIndex,
    uint16_t useActualUnitData)
{
    RevengeTacticalCombatRatings *averageRatings =
        &DisplayedUnitCombatRatings;
    uint16_t lanceEndUnitIndex = (leaderUnitIndex + 0x04) & 0x00FC;
    uint16_t includedUnitCount = 0x00;
    uint16_t ratingIndex;
    uint16_t unitIndex;

    for (ratingIndex = 0x00; ratingIndex < 0x06; ++ratingIndex)
        ((int16_t *) averageRatings)[ratingIndex] = 0x00;

    for (unitIndex = leaderUnitIndex;
        unitIndex < lanceEndUnitIndex;
        ++unitIndex)
    {
        RevengeUnitRecord *unitRecord = &PersistentUnitRoster[unitIndex];
        RevengeTacticalCombatRatings *unitRatings;

        if (unitRecord->deploymentState == 0x00)
            continue;

        if (useActualUnitData != FALSE)
        {
            RecalculateUnitCombatRatings(
                unitRecord, unitIndex, TRUE, FALSE);
        }
        else
        {
            BuildUndamagedUnitCombatEstimate(unitIndex);
            RecalculateUnitCombatRatings(
                (RevengeUnitRecord *) &EstimatedUnitRecord,
                unitIndex,
                TRUE,
                FALSE);
        }

        unitRatings = &UnitCombatRatings[unitIndex];
        for (ratingIndex = 0x00; ratingIndex < 0x06; ++ratingIndex)
            ((int16_t *) averageRatings)[ratingIndex] +=
                ((int16_t *) unitRatings)[ratingIndex];
        ++includedUnitCount;
    }

    // The caller supplies a deployed lance leader, so the original performs an
    // unconditional division here. Preserve that contract while making the
    // invariant explicit for readers of the reconstructed C.
    if (includedUnitCount != 0x00)
    {
        for (ratingIndex = 0x00; ratingIndex < 0x06; ++ratingIndex)
            ((int16_t *) averageRatings)[ratingIndex] /= (int16_t) includedUnitCount;
    }
}

// 2B34:1014: word16 PrepareDisplayedUnitCombatRatings(word16 unitIndex)
// Called from:
//      DrawTacticalUnitControlPanel
uint16_t PrepareDisplayedUnitCombatRatings(uint16_t unitIndex)
{
    uint16_t concealEnemyDamage = unitIndex >= ENEMY_FORCE_FIRST_UNIT;
    uint16_t unitIsLanceLeader;

    if (concealEnemyDamage != FALSE)
    {
        uint16_t friendlyUnitIndex;
        for (friendlyUnitIndex = 0x00;
            friendlyUnitIndex < TACTICAL_FORCE_UNIT_COUNT;
            ++friendlyUnitIndex)
        {
            RevengeUnitRecord *friendlyUnit =
                &PersistentUnitRoster[friendlyUnitIndex];
            if (friendlyUnit->deploymentState != 0x00 &&
                friendlyUnit->beagleActiveProbe != 0x00 &&
                HasUnitLineOfSight(
                    friendlyUnitIndex, unitIndex) != FALSE)
            {
                concealEnemyDamage = FALSE;
            }
        }
    }

    unitIsLanceLeader = IsCurrentLanceLeader(unitIndex);
    if (unitIsLanceLeader != FALSE)
    {
        CalculateLanceAverageCombatRatings(
            unitIndex, concealEnemyDamage == FALSE);
    }
    else if (concealEnemyDamage != FALSE)
    {
        BuildUndamagedUnitCombatEstimate(unitIndex);
        RecalculateUnitCombatRatings(
            (RevengeUnitRecord *) &EstimatedUnitRecord,
            unitIndex,
            TRUE,
            FALSE);
    }
    else
    {
        RecalculateUnitCombatRatings(
            &PersistentUnitRoster[unitIndex],
            unitIndex,
            TRUE,
            FALSE);
    }

    return concealEnemyDamage;
}
