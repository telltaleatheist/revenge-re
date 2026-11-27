// 2007: tactical status
// TacticalEffects.cpp - tactical display, projectile, and effect procedures.
// Reconstructed from REVENGE.EXE and its verified annotation project.

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "Campaign/Campaign.h"
#include "Data/AudioData.h"
#include "Data/RuntimeData.h"
#include "Data/StartupData.h"
#include "Data/CampaignStateData.h"
#include "Data/RosterData.h"
#include "Data/TacticalControlData.h"
#include "Data/TacticalMapData.h"
#include "Data/TacticalStatusData.h"
#include "Data/TacticalUnitData.h"
#include "Data/TextIOData.h"
#include "Graphics/Graphics.h"
#include "Input/Input.h"
#include "Tactical/Tactical.h"
#include "Text/Text.h"
#include "Utility/Utility.h"
#include "revenge/tactical_panel_data.h"
#include "revenge/video.h"

#include <string.h>


void ApplyTerrainImpact(
    int16_t mapX, int16_t mapY,
    uint16_t advanceBeforeRecording, uint16_t forceAnimationRecord);
void ApplyAreaTerrainAndUnitDamage(
    int16_t mapX, int16_t mapY, uint16_t visualEffectType, int16_t weaponIndex);
void ApplyWeaponDamage(
    int16_t attackerUnitIndex, int16_t weaponIndex, int16_t targetUnitIndex);
uint16_t AdvanceTerrainDamageState(
    int16_t mapX, int16_t mapY, uint16_t requestedDamageState);
// 2007:1AE3 advances and removes the four-word map-edge effect records at
// DS:A800, occasionally emitting secondary impact visuals along their path.
void AdvanceMapEdgeEffects(void);
// 2007:16E8 advances the four-word impact-sprite records at DS:B2F4 and
// compacts the queue when the current sprite reaches its terminal sprite.
void AdvanceImpactSpriteAnimations(void);
void ClearTacticalEffectOverlayMap(void);
// 2007:17DE advances the 0Eh-byte projectile records at DS:A9DD toward their
// target cells, applies damage on arrival, queues an impact and compacts them.
void AdvanceTacticalProjectiles(void);
void QueueTacticalImpactVisual(
    int16_t mapX, int16_t mapY, uint16_t impactClass);
uint16_t DetermineRelativeAttackDirection(
    int16_t sourceX, int16_t sourceY, int16_t targetX, int16_t targetY);
uint16_t GeneratePseudoRandomByte(void);
uint16_t DetermineOctant(int16_t deltaY, int16_t deltaX);
void DrawClippedAxisAlignedLine(
    int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
void DrawClippedPixel(int16_t x, int16_t y, uint16_t color);
void DrawTacticalClippedSprite(
    uint8_t *renderTarget,
    const uint8_t *encodedSprite,
    int16_t relativeX,
    int16_t relativeY,
    uint16_t textWindowIndex);
void DrawTacticalTileSprite(
    const uint8_t *spritePage, uint16_t spriteId,
    uint16_t destinationX, uint16_t destinationY);
void DrawCompositeTacticalTile(
    uint16_t firstSpriteId, uint16_t secondSpriteId,
    const uint8_t *spritePage,
    uint16_t destinationX, uint16_t destinationY);
void BuildTacticalTerrainCellMaps(
    const uint8_t *terrainTypeMap,
    uint16_t *tacticalCellSpriteMap,
    uint16_t viewportX,
    uint16_t viewportY);
void DrawTacticalTileSpriteForAdapter(
    const uint8_t *spritePage,
    uint16_t spriteId,
    uint16_t destinationX,
    uint16_t destinationY);
void DrawSelectedUnitStatusPanel(void);
void RedrawTacticalViewport(void);
void DrawCampaignClock(void);
void FormatUnitDurabilityPercentage(int16_t unitIndex);
void DrawCampaignClockSeparator(void);
int16_t MapUnitToTacticalStatusSlot(uint16_t unitIndex);
uint16_t IsCurrentLanceLeader(uint16_t unitIndex);
void PlayScenarioDigitalSoundSequence(int8_t scenarioCode);
void DispatchWeaponEffect(int16_t weaponEffectId);
void RefreshTacticalOverview(void);

/* 2007:0328 is reconstructed in Tactical/Display.cpp. */

// 2007:042A: void AdvanceTerrainEffects(word16 advanceEffects)
//
// Advance queued two-bit terrain damage, redraw the terrain layer when the
// tactical display permits it, and let active DS:810C animation records spread
// to adjacent cells or expire. The queue stores word pairs: packed damage-map
// byte index followed by the two-bit subcell index.
// Verified against 2007:042A-064F.
void AdvanceTerrainEffects(uint16_t advanceEffects)
{
    if (advanceEffects != FALSE)
    {
        int16_t queueEntryIndex;
        for (queueEntryIndex = 0x00;
            queueEntryIndex < (int16_t) (TerrainDamageQueueWordCount >> 0x01);
            ++queueEntryIndex)
        {
            RevengeTerrainDamageQueueEntry *entry =
                &TerrainDamageQueue[queueEntryIndex];
            uint16_t packedByteIndex = entry->packedByteIndex;
            uint16_t subcellIndex = entry->subcellIndex;
            uint8_t occupiedMask = TerrainDamageCellOccupiedMask[subcellIndex];
            uint8_t increment = TerrainDamageStageIncrement[subcellIndex];
            uint8_t *packedDamageByte =
                &PackedTerrainDamageMap[packedByteIndex];

            if ((*packedDamageByte & occupiedMask) != occupiedMask)
            {
                *packedDamageByte += increment;
                continue;
            }

            // The queued cell reached its terminal staged value. Toggle the
            // occupied bits and remove this word pair without leaving a gap.
            *packedDamageByte ^= occupiedMask;
            int16_t sourceEntryIndex;
            for (sourceEntryIndex = queueEntryIndex + 0x01;
                sourceEntryIndex <
                (int16_t) (TerrainDamageQueueWordCount >> 0x01);
                ++sourceEntryIndex)
            {
                TerrainDamageQueue[sourceEntryIndex - 0x01] =
                    TerrainDamageQueue[sourceEntryIndex];
            }
            TerrainDamageQueueWordCount -= 0x02;
            --queueEntryIndex;
        }
    }

    if (TacticalOverviewViewportDirty == 0x00)
        AdvanceMapEdgeEffects();

    if (advanceEffects != FALSE)
    {
        RevengeTerrainImpactAnimationRecord *records =
            TerrainImpactAnimationRecords;
        uint16_t recordIndex;
        for (recordIndex = 0x00; recordIndex < 0x5A; ++recordIndex)
        {
            RevengeTerrainImpactAnimationRecord *record =
                &records[recordIndex];
            uint16_t randomDirection;
            if (record->mapX == -0x01)
                continue;

            randomDirection = GeneratePseudoRandomByte() & 0x7F;
            switch (randomDirection)
            {
            case 0x00:
            case 0x01:
                ApplyTerrainImpact(
                    record->mapX - 0x01, record->mapY, FALSE, FALSE);
                break;
            case 0x02:
            case 0x03:
                ApplyTerrainImpact(
                    record->mapX + 0x01, record->mapY, FALSE, FALSE);
                break;
            case 0x04:
            case 0x05:
                ApplyTerrainImpact(
                    record->mapX, record->mapY - 0x01, FALSE, FALSE);
                break;
            case 0x06:
            case 0x07:
                ApplyTerrainImpact(
                    record->mapX, record->mapY + 0x01, FALSE, FALSE);
                break;
            case 0x09:
                record->mapX = -0x01;
                break;
            }
        }
    }

    AdvanceImpactSpriteAnimations();
    AdvanceTacticalProjectiles();
}

// 2007:0846: void QueueTacticalImpactVisual(
//     int16 mapX, int16 mapY, word16 impactClass)
// Verified against 2007:0846-08D4 and all executable call sites.
void QueueTacticalImpactVisual(
    int16_t mapX,
    int16_t mapY,
    uint16_t impactClass)
{
    uint16_t impactSpriteId;
    uint16_t soundEffectId;
    uint16_t queueIndex = TacticalImpactQueueCount;
    RevengeTacticalImpactRecord *record;

    if (queueIndex >= 0x50)
        return;

    // V: these are the only classes passed by the four executable call sites.
    switch (impactClass)
    {
    case 0x01:
        impactSpriteId = 0x06;
        soundEffectId = 0x04;
        break;
    case 0x06:
        impactSpriteId = 0x0A;
        soundEffectId = 0x02;
        break;
    case 0x0A:
        impactSpriteId = 0x0E;
        soundEffectId = 0x0E;
        break;
    default:
        // The original leaves DI unchanged for any other value. No known caller
        // exercises that undefined path, so do not manufacture a sprite ID.
        return;
    }

    record = &TacticalImpactQueue[queueIndex];
    record->mapX = mapX;
    record->mapY = mapY;
    record->spriteId = impactClass;
    record->terminalSpriteId = impactSpriteId;
    DispatchWeaponEffect(soundEffectId);
    TacticalImpactQueueCount = queueIndex + 0x01;
}

// 2007:08D5: void QueueMapEdgeEffect(
//     short mapX, short mapY, word16 baseSpriteId, word16 edgeDirection)
void QueueMapEdgeEffect(
    int16_t mapX,
    int16_t mapY,
    uint16_t baseSpriteId,
    uint16_t edgeDirection)
{
    uint16_t queueCount = MapEdgeEffectCount;
    if (queueCount >= 0x03)
        return;

    RevengeMapEdgeEffectRecord *record =
        &MapEdgeEffectQueue[queueCount];
    record->mapX = mapX;
    record->mapY = mapY;
    record->spriteId = baseSpriteId + (edgeDirection << 0x02);
    record->edgeDirection = edgeDirection;
    MapEdgeEffectCount = queueCount + 0x01;
}

// 2007:0CFB: void ApplyAreaTerrainAndUnitDamage(
//     short mapX, short mapY, word16 visualEffectType, short weaponIndex)
// Verified against 2007:0CFB-0D8F.
void ApplyAreaTerrainAndUnitDamage(
    int16_t mapX,
    int16_t mapY,
    uint16_t visualEffectType,
    int16_t weaponIndex)
{
    uint16_t impactMapX = (uint16_t) mapX;
    uint16_t impactMapY = (uint16_t) mapY;
    uint16_t adjacentMapX = impactMapX + 0x01;
    uint16_t adjacentMapY = impactMapY + 0x01;

    QueueTacticalImpactVisual(mapX, mapY, visualEffectType);
    AdvanceTerrainDamageState(mapX, mapY, TERRAIN_DAMAGE_STATE_MAXIMUM);

    uint16_t unitIndex;
    for (unitIndex = 0x00; unitIndex < TACTICAL_UNIT_COUNT; ++unitIndex)
    {
        RevengeUnitRecord *unitRecord =
            &PersistentUnitRoster[unitIndex];
        if (unitRecord->deploymentState == 0x00)
            continue;

        uint16_t unitX = ScenarioUnitMapX[unitIndex];
        uint16_t unitY = ScenarioUnitMapY[unitIndex];
        if ((unitX == impactMapX || unitX == adjacentMapX) &&
            (unitY == impactMapY || unitY == adjacentMapY))
        {
            ApplyWeaponDamage(0x00, weaponIndex, unitIndex);
        }
    }
}

/* 2007:1D06 is reconstructed in Tactical/Display.cpp. */

// 2007:1D61: format the selected unit's remaining durability percentage in
// the shared text buffer. Opposing units deliberately expose only 10% steps.
void FormatUnitDurabilityPercentage(int16_t unitIndex)
{
    RevengeUnitRecord *unitRecord =
        &PersistentUnitRoster[unitIndex];
    uint16_t percentage;

    CalculateUnitDurabilityTotals(unitRecord);
    if (unitIndex >= ENEMY_FORCE_FIRST_UNIT)
    {
        percentage = (uint16_t) (((uint32_t) CurrentUnitDurabilityTotal * 0x0A) /
                         MaximumUnitDurabilityTotal) *
            0x0A;
    }
    else
    {
        percentage = (uint16_t) (((uint32_t) CurrentUnitDurabilityTotal * 0x64) /
            MaximumUnitDurabilityTotal);
    }

    IntegerToString(
        percentage,
        SharedNumericTextBuffer,
        0x0A);
    strcat(
        SharedNumericTextBuffer,
        PercentageSuffixText);
}

// 2007:1DCC: void DrawCampaignClockSeparator(void)
void DrawCampaignClockSeparator(void)
{
    uint8_t separatorColor = (LOW_WORD(GameAnimationTicks) & 0x10) != 0x00 ? 0x0F : 0x0C;
    HideMouseCursorOverlay();
    DrawTileAlignedText(
        CampaignClockSeparatorText,
        0x25,
        0x18,
        separatorColor,
        0x0C);
    ShowMouseCursorOverlay();
}

// 2007:15FF: void DrawCampaignClock(void)
// Verified against 2007:15FF-16B9.
void DrawCampaignClock(void)
{
    char *clockText;
    int16_t clockValue;
    uint16_t firstDigitOffset;

    if (SuppressCampaignClock != 0x00 ||
        TacticalOverviewActive != 0x00)
    {
        return;
    }

    clockText = SharedNumericTextBuffer;
    clockText[0x00] = '\0';
    clockValue = CampaignClockState[0x00];
    firstDigitOffset = 0x00;
    if (clockValue < 0x0A)
    {
        clockText[0x00] = '0';
        firstDigitOffset = 0x01;
    }
    IntegerToString(
        clockValue,
        clockText + firstDigitOffset,
        0x0A);
    HideMouseCursorOverlay();
    DrawTileAlignedText((char *) clockText, 0x23, 0x18, 0x0F, 0x0C);
    ShowMouseCursorOverlay();

    DrawCampaignClockSeparator();

    clockValue = CampaignClockState[0x01];
    firstDigitOffset = 0x00;
    if (clockValue < 0x0A)
    {
        clockText[0x00] = '0';
        firstDigitOffset = 0x01;
    }
    IntegerToString(
        clockValue,
        clockText + firstDigitOffset,
        0x0A);
    HideMouseCursorOverlay();
    DrawTileAlignedText((char *) clockText, 0x26, 0x18, 0x0F, 0x0C);
    ShowMouseCursorOverlay();
}

// 2007:132C: draw a raised, beveled menu button at a text-cell position.
// Called from:
//      RunMainMenuSelection
void DrawBeveledMenuButton(
    uint16_t textColumn,
    uint16_t textRow,
    const char *text,
    uint16_t widthPixels)
{
    uint16_t fontHeight = ActiveFontLineHeight;
    uint16_t glyphAdvance = ActiveFontGlyphAdvanceX;
    uint16_t leftX = (ActiveTextWindowLeftTile << 0x03) +
        textColumn * glyphAdvance;
    uint16_t topY = (ActiveTextWindowTopTile << 0x03) +
        textRow * fontHeight;

    // Verified against 2007:132C-14DE.
    HideMouseCursorOverlay();
    FillVideoRectangle(leftX - 0x02, topY - 0x02,
        leftX + widthPixels + 0x02, topY + fontHeight + 0x01, 0x0E);
    DrawCenteredClippedText(text, 0x00, leftX, topY,
        widthPixels, 0x0F, 0x00);
    DrawClippedAxisAlignedLine(leftX - 0x04, topY - 0x03,
        leftX + widthPixels + 0x02, topY - 0x03, 0x0F);
    DrawClippedAxisAlignedLine(leftX - 0x03, topY - 0x02,
        leftX + widthPixels + 0x01, topY - 0x02, 0x0F);
    DrawClippedAxisAlignedLine(leftX - 0x04, topY - 0x02,
        leftX - 0x04, topY + fontHeight + 0x01, 0x0F);
    DrawClippedAxisAlignedLine(leftX - 0x03, topY - 0x01,
        leftX - 0x03, topY + fontHeight, 0x0F);
    DrawClippedAxisAlignedLine(leftX + widthPixels + 0x02, topY - 0x02,
        leftX + widthPixels + 0x02, topY + fontHeight + 0x01, 0x0D);
    DrawClippedAxisAlignedLine(leftX + widthPixels + 0x01, topY - 0x01,
        leftX + widthPixels + 0x01, topY + fontHeight, 0x0D);
    DrawClippedAxisAlignedLine(leftX - 0x02, topY + fontHeight,
        leftX + widthPixels + 0x01, topY + fontHeight, 0x0D);
    DrawClippedAxisAlignedLine(leftX - 0x03, topY + fontHeight + 0x01,
        leftX + widthPixels + 0x01, topY + fontHeight + 0x01, 0x0D);
    ShowMouseCursorOverlay();
}

// 2007:0A4D: uint16 DetermineOctant(int16 deltaY, int16 deltaX)
uint16_t DetermineOctant(int16_t deltaY, int16_t deltaX)
{
    int16_t negativeX;
    int16_t negativeY;

    if (deltaY < 0x00)
    {
        if (deltaX > 0x00)
        {
            negativeY = (int16_t) (0x0000u - (uint16_t) deltaY);
            return negativeY > deltaX ? 0x03 : 0x02;
        }
        negativeX = (int16_t) (0x0000u - (uint16_t) deltaX);
        negativeY = (int16_t) (0x0000u - (uint16_t) deltaY);
        return negativeX > negativeY ? 0x05 : 0x04;
    }

    if (deltaX > 0x00)
        return deltaX > deltaY ? 0x01 : 0x00;

    negativeX = (int16_t) (0x0000u - (uint16_t) deltaX);
    return negativeX < deltaY ? 0x07 : 0x06;
}

// 2007:0D90: uint16 CalculateApproximateMapDistance(
//     Stack int16 sourceX, Stack int16 sourceY,
//     Stack int16 targetX, Stack int16 targetY)
// Called from:
//      ResolveWeaponAttack
uint16_t CalculateApproximateMapDistance(
    int16_t sourceX,
    int16_t sourceY,
    int16_t targetX,
    int16_t targetY)
{
    int16_t deltaX;
    int16_t deltaY;
    int16_t distance;

    deltaX = (int16_t) ((uint16_t) sourceX - (uint16_t) targetX);
    deltaY = (int16_t) ((uint16_t) sourceY - (uint16_t) targetY);
    if (deltaX < 0x00)
        deltaX = (int16_t) (0x0000u - (uint16_t) deltaX);
    if (deltaY < 0x00)
        deltaY = (int16_t) (0x0000u - (uint16_t) deltaY);

    if (deltaX > deltaY)
        distance = (int16_t) (deltaX + (deltaY >> 0x01));
    else
        distance = (int16_t) (deltaY + (deltaX >> 0x01));
    return (uint16_t) distance;
}

/* Host: the game clock's tick at the previous draw, and the ms the
 * narration countdown lost then (NARRATION_DELAY_MS in Tactical/Tactical.h). */
static uint32_t narrationClockTick;
static uint16_t narrationLastStepMs;

// 2007:0E19: resolve the selected or cursor-contact unit, populate tactical
// text window 6, maintain both status-row highlights, and service the pending
// narration countdown.
// Verified against 2007:0E19-1218.
// Host: the countdown is battle time in ms, not draws (Tactical/Tactical.h).
void DrawSelectedUnitStatusPanel(void)
{
    uint16_t narrationStatusRow;
    uint16_t displayedStatusRow;
    uint16_t selectedUnitIndex;
    uint16_t cursorSpriteId = 0x0000;
    uint16_t drawUnitDetails;
    uint16_t heatScaleIndex;
    uint16_t textLength;
    uint16_t narrationColor;
    uint16_t viewportCellIndex;
    uint16_t mapX;
    uint16_t mapY;
    uint16_t unitIndex;
    int16_t blinkCountdown;
    uint8_t unitTypeId;
    RevengeUnitRecord *unitRecord;
    char *statusText;
    uint16_t narrationStepMs;

    /* At most one tick a draw: a longer gap is a wait outside the battle
	 * loop (a message, a menu), when the original drew nothing. */
    narrationStepMs = GameTimerTicks != narrationClockTick ? GAME_CLOCK_TICK_MS : 0x0000;
    narrationClockTick = GameTimerTicks;

    narrationStatusRow = 0xFFFF;
    displayedStatusRow = 0xFFFF;
    if (PendingNarrationDelay != 0x0000)
        narrationStatusRow = MapUnitToTacticalStatusSlot(
            ActiveNarrationSubject);

    selectedUnitIndex = TacticalSelectedUnitIndex;
    if (selectedUnitIndex == TACTICAL_UNIT_NONE)
    {
        viewportCellIndex = (uint16_t) (TacticalCursorViewportY *
                TACTICAL_VIEWPORT_WIDTH_CELLS +
            TacticalCursorViewportX);
        cursorSpriteId =
            TacticalCellSpriteMap[viewportCellIndex];
        if (cursorSpriteId != 0x0000)
        {
            mapX = (uint16_t) (TacticalViewportMapX +
                TacticalCursorViewportX);
            mapY = (uint16_t) (TacticalViewportMapY +
                TacticalCursorViewportY);
            for (unitIndex = 0x0000;
                unitIndex < TACTICAL_UNIT_COUNT && selectedUnitIndex == TACTICAL_UNIT_NONE;
                ++unitIndex)
            {
                unitRecord = &PersistentUnitRoster[unitIndex];
                if (unitRecord->deploymentState != 0x00 &&
                    ScenarioUnitMapX[unitIndex] == mapX &&
                    ScenarioUnitMapY[unitIndex] == mapY)
                {
                    selectedUnitIndex = unitIndex;
                }
            }
        }
    }

    if (selectedUnitIndex == TACTICAL_UNIT_NONE)
    {
        HideMouseCursorOverlay();
        DrawTileAlignedText(
            TacticalNoUnitStatusText,
            0x0000, 0x0018, 0x0C, 0x0C);
        ShowMouseCursorOverlay();
        if (DisplayedTacticalStatusRow != 0xFFFF)
        {
            TacticalStatusRowInvalidation[DisplayedTacticalStatusRow] = 0xFF;
            DisplayedTacticalStatusRow = 0xFFFF;
        }
    }
    else
    {
        SelectTextWindow(0x0006);
        ActiveTextCursorRow = 0x0000;
        ActiveTextCursorColumn = 0x0000;
        ActiveTextForegroundColor = 0x000F;
        ActiveTextBackgroundColor = 0x000C;
        drawUnitDetails = TRUE;

        if (selectedUnitIndex >= ENEMY_FORCE_FIRST_UNIT)
        {
            HideMouseCursorOverlay();
            if (TacticalSelectedUnitIndex == TACTICAL_UNIT_NONE &&
                (cursorSpriteId & 0x00FF) == 0x00EF)
            {
                ClearActiveTextWindow();
                DrawCampaignClock();
                RedrawTacticalViewport();
                drawUnitDetails = FALSE;
            }
            else if (UnitEnemyVisible[selectedUnitIndex] ==
                    FALSE &&
                UnitEnemyInProximityRange[selectedUnitIndex] == FALSE &&
                TacticalOverviewMapMode != 0x0000)
            {
                drawUnitDetails = FALSE;
            }
            ShowMouseCursorOverlay();
        }

        if (drawUnitDetails != FALSE)
        {
            HideMouseCursorOverlay();
            if (selectedUnitIndex < 0x0004 ||
                IsCurrentLanceLeader(selectedUnitIndex) != FALSE)
            {
                RenderFormattedText(selectedUnitIndex < ENEMY_FORCE_FIRST_UNIT ? FriendlyUnitStatusHeading : EnemyUnitStatusHeading);
            }
            if (selectedUnitIndex >= ENEMY_FORCE_FIRST_UNIT)
            {
                ActiveTextForegroundColor = 0x0006;
                EnemyCycleUnitIndex = selectedUnitIndex;
            }

            unitRecord = &PersistentUnitRoster[selectedUnitIndex];
            unitTypeId = unitRecord->unitTypeId;
            RenderFormattedText(UnitNameTable[unitTypeId]);
            RenderFormattedText(
                UnitStatusSeparatorText);
            FormatUnitDurabilityPercentage(selectedUnitIndex);

            statusText = SharedNumericTextBuffer;
            strcat(
                statusText,
                UnitHeatStatusPrefix);
            heatScaleIndex =
                (uint16_t) (unitRecord->currentHeat / BATTLEMECH_HEAT_SCALE);
            if (heatScaleIndex > 0x0005)
                heatScaleIndex = 0x0005;
            if (UnitShutdownState[selectedUnitIndex] != 0x00)
                heatScaleIndex = 0x0006;
            strcat(
                statusText,
                (const char *) HeatScaleDescriptionTable[heatScaleIndex]);

            textLength = (uint16_t) strlen(statusText);
            while ((uint16_t) (textLength +
                       ActiveTextCursorColumn) < 0x001B)
            {
                statusText[textLength] = ' ';
                ++textLength;
            }
            statusText[(uint16_t) (0x001B -
                ActiveTextCursorColumn)] = '\0';
            RenderFormattedText(statusText);

            displayedStatusRow =
                MapUnitToTacticalStatusSlot(selectedUnitIndex);
            if (displayedStatusRow != 0xFFFF)
            {
                if (narrationStatusRow == 0xFFFF ||
                    displayedStatusRow != narrationStatusRow ||
                    TacticalStatusBlinkVisible == 0x00)
                {
                    UpdatePrimaryUnitStatusRowHighlight(
                        displayedStatusRow);
                }
                DisplayedTacticalStatusRow = displayedStatusRow;
            }
            ShowMouseCursorOverlay();
        }
    }

    blinkCountdown = (int16_t) (uint16_t) (TacticalStatusBlinkCountdown - 0x0001);
    TacticalStatusBlinkCountdown = (uint16_t) blinkCountdown;
    if (blinkCountdown <= 0x0000)
    {
        TacticalStatusBlinkCountdown = 0x000A;
        TacticalStatusBlinkVisible ^= 0x01;
        CachedPrimaryStatusHighlightColor = 0xFFFF;
        CachedSecondaryStatusHighlightColor = 0xFFFF;
    }

    if (PendingNarrationDelay == 0x0000)
    {
        HideMouseCursorOverlay();
        DrawTileAlignedText(
            TacticalClockBlankText,
            0x001B, 0x0018, 0x0C, 0x0C);
        ShowMouseCursorOverlay();
    }
    else
    {
        if (PendingNarrationDelay <=
                NARRATION_DELAY_MS - NARRATION_SOUND_AFTER_MS &&
            PendingNarrationDelay + narrationLastStepMs >
                NARRATION_DELAY_MS - NARRATION_SOUND_AFTER_MS)
        {
            if (NarrationMessageCategory == 0x0001)
                DispatchWeaponEffect(0x0000);
            else if (FriendlyUnitDestroyed == 0x0000 &&
                ActiveNarrationSubject != 0x0000)
            {
                PlayScenarioDigitalSoundSequence(
                    (int8_t) ScenarioFileSuffix);
            }
        }

        HideMouseCursorOverlay();
        if (TacticalStatusBlinkVisible != 0x00)
        {
            narrationColor = NarrationEmphasis != 0x0000 ? 0x0008 : 0x000A;
            DrawTileAlignedText(
                TacticalNarrationLabel,
                0x001B, 0x0018, (uint8_t) narrationColor, 0x0C);
            SecondaryStatusHighlightColor = narrationColor;
            if (narrationStatusRow != 0xFFFF)
                UpdateSecondaryUnitStatusRowHighlight(
                    narrationStatusRow);
        }
        else
        {
            DrawTileAlignedText(
                TacticalNarrationBlankText,
                0x001B, 0x0018, 0x0C, 0x0C);
            SecondaryStatusHighlightColor = 0x0000;
            if (narrationStatusRow != 0xFFFF &&
                narrationStatusRow != displayedStatusRow)
            {
                UpdateSecondaryUnitStatusRowHighlight(
                    narrationStatusRow);
            }
        }
        ShowMouseCursorOverlay();

        narrationLastStepMs = narrationStepMs < PendingNarrationDelay ? narrationStepMs : PendingNarrationDelay;
        PendingNarrationDelay -= narrationLastStepMs;
        if (PendingNarrationDelay == 0x0000)
        {
            NarrationEmphasis = 0x0000;
            TacticalStatusRowInvalidation[narrationStatusRow] = 0xFF;
        }
    }

    DrawCampaignClockSeparator();
}

// 2007:159F: draw the eight tactical game-speed bars when the overview is off.
void RedrawTacticalViewport(void)
{
    uint16_t speedIndex;

    if (TacticalOverviewActive != FALSE)
        return;

    HideMouseCursorOverlay();
    for (speedIndex = 0x00; speedIndex < 0x08; ++speedIndex)
    {
        uint16_t color = 0x03;
        /* The CGA's colour step (2007:15BD) went with the CGA. */
        if (speedIndex == TacticalGameSpeedIndex)
            color = 0x04;
        FillVideoRectangle(
            0x00F8 + speedIndex * 0x03,
            0x00C7 - speedIndex,
            0x00F9 + speedIndex * 0x03,
            0x00C7,
            color);
    }
    ShowMouseCursorOverlay();
}

// 2007:16BA: void RedrawTacticalStatusPanel(void)
void RedrawTacticalStatusPanel(void)
{
    uint16_t previousTextWindow = ActiveTextWindowIndex;

    HideMouseCursorOverlay();
    SelectAndClearTextWindow(0x06);
    DrawSelectedUnitStatusPanel();
    RedrawTacticalViewport();
    DrawCampaignClock();
    ShowMouseCursorOverlay();
    SelectTextWindow(previousTextWindow);
}
