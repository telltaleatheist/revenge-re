// 2007: tactical effects
// TacticalEffects.cpp - tactical display, projectile, and effect procedures.
// Reconstructed from REVENGE.EXE and its verified annotation project.

#include "Data/StartupData.h"
#include "Data/CampaignStateData.h"
#include "Data/RosterData.h"
#include "Data/TacticalControlData.h"
#include "Data/TacticalMapData.h"
#include "Data/TacticalStatusData.h"
#include "Data/TacticalUnitData.h"
#include "Data/TacticalTables.h"
#include "Data/TextIOData.h"
#include "Graphics/Graphics.h"
#include "Input/Input.h"
#include "Tactical/Tactical.h"
#include "Text/Text.h"
#include "revenge/video.h"


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

// 2007:0AA4: queue one 0Eh-byte animated missile/projectile record.
// Verified against ASM 2007:0AA4-0C6E.
void QueueTacticalProjectile(
    uint16_t attackerUnitIndex,
    uint8_t weaponIndex,
    uint16_t targetUnitIndex,
    uint16_t shotHit)
{
    int16_t queueCount;
    RevengeTacticalProjectileRecord *record;
    uint16_t direction;
    uint16_t destinationX;
    uint16_t destinationY;
    uint16_t hasDestination;

    queueCount = (int16_t) TacticalProjectileCount;
    if (queueCount >= 0x0050)
        return;

    record = &TacticalProjectileRecords[queueCount];
    record->weaponIndex = weaponIndex;
    record->targetUnitIndex = (RevengeUnitSlot) targetUnitIndex;
    record->attackerUnitIndex = (RevengeUnitSlot) attackerUnitIndex;
    destinationX = (uint16_t) record->destinationMapX;
    destinationY = (uint16_t) record->destinationMapY;

    if ((int16_t) targetUnitIndex < TACTICAL_UNIT_COUNT)
    {
        direction = DetermineOctant(
            (int16_t) (uint16_t) (ScenarioUnitMapY[attackerUnitIndex] -
                ScenarioUnitMapY[targetUnitIndex]),
            (int16_t) (uint16_t) (ScenarioUnitMapX[targetUnitIndex] -
                ScenarioUnitMapX[attackerUnitIndex]));
    }
    else
    {
        direction = DetermineOctant(
            (int16_t) (uint16_t) (ScenarioUnitMapY[attackerUnitIndex] -
                UnitTargetMapY[attackerUnitIndex]),
            (int16_t) (uint16_t) (UnitTargetMapX[attackerUnitIndex] -
                ScenarioUnitMapX[attackerUnitIndex]));
    }

    hasDestination = FALSE;
    if (targetUnitIndex == TACTICAL_TARGET_MAP_CELL)
    {
        destinationX = UnitTargetMapX[attackerUnitIndex];
        destinationY = UnitTargetMapY[attackerUnitIndex];
        hasDestination = TRUE;
    }
    else if (shotHit == FALSE)
    {
        destinationX = (uint16_t) (ScenarioUnitMapX[targetUnitIndex] +
            (DirectionDeltaXTable[direction] << 0x01));
        destinationY = (uint16_t) (ScenarioUnitMapY[targetUnitIndex] +
            (DirectionDeltaYTable[direction] << 0x01));
        hasDestination = TRUE;
    }

    // On a hit against a live unit the ASM stores the incoming DI register at
    // 0Ah. The sole caller, ResolveWeaponAttack at 2D1B:1D17-1D21, keeps
    // the weapon index in DI and passes that same value as weaponIndex. The word
    // at 0Ch comes from an uninitialized stack local. AdvanceTacticalProjectiles
    // ignores both destination words for a live hit, so retaining the old 0Ch
    // value preserves the executable's observable behavior without introducing
    // undefined C behavior.
    if (hasDestination == FALSE)
    {
        destinationX = (uint16_t) weaponIndex;
    }
    record->destinationMapX = (int16_t) destinationX;
    record->destinationMapY = (int16_t) destinationY;

    record->shotHit = (uint8_t) shotHit;
    record->spriteId = (direction << 0x01) + 0x0E;
    record->currentMapX = ScenarioUnitMapX[attackerUnitIndex];
    record->currentMapY = ScenarioUnitMapY[attackerUnitIndex];
    TacticalProjectileCount = (uint16_t) (queueCount + 0x0001);
}

// 2007:07C9: draw the four sides of one 16x16 tactical-cell outline.
void DrawTacticalCellOutline(
    int16_t leftX,
    int16_t topY,
    uint16_t color)
{
    HideMouseCursorOverlay();
    DrawClippedAxisAlignedLine(
        leftX, topY, leftX + 0x0F, topY, color);
    DrawClippedAxisAlignedLine(
        leftX, topY, leftX, topY + 0x0F, color);
    DrawClippedAxisAlignedLine(
        leftX + 0x0F, topY, leftX + 0x0F, topY + 0x0F, color);
    DrawClippedAxisAlignedLine(
        leftX, topY + 0x0F, leftX + 0x0F, topY + 0x0F, color);
    ShowMouseCursorOverlay();
}

// 2007:079A: draw the current tactical cursor and alternate its palette color.
void DrawTacticalCursorOutline(uint16_t color)
{
    DrawTacticalCellOutline(
        TacticalCursorViewportX * TACTICAL_CELL_PIXEL_SIZE +
            TACTICAL_CURSOR_PIXEL_LEFT,
        TacticalCursorViewportY * TACTICAL_CELL_PIXEL_SIZE,
        color);
    TacticalCursorOutlineState ^= 0x000F;
}

// 2007:0926: draw a clipped Bresenham-style weapon trace. When dashedTrace
// is nonzero, plot one pixel every six steps with a randomized starting phase.
// Verified against 2007:0926-0A4C.
void DrawClippedWeaponTraceLine(
    int16_t startX,
    int16_t startY,
    int16_t endX,
    int16_t endY,
    uint16_t color,
    uint16_t dashedTrace)
{
    uint16_t plotInterval = dashedTrace != FALSE ? TACTICAL_TRACE_DASH_INTERVAL : 0x01;
    uint16_t stepsUntilPlot = dashedTrace != FALSE ? (GeneratePseudoRandomByte() & 0x03) + 0x01 : 0x01;
    int16_t deltaY = (int16_t) (uint16_t) ((uint16_t) startY - (uint16_t) endY);
    int16_t deltaX = (int16_t) (uint16_t) ((uint16_t) endX - (uint16_t) startX);
    int16_t absoluteY = deltaY < 0x00 ? (int16_t) (uint16_t) (0x00 - (uint16_t) deltaY) : deltaY;
    int16_t absoluteX = deltaX < 0x00 ? (int16_t) (uint16_t) (0x00 - (uint16_t) deltaX) : deltaX;
    int16_t majorDistance = absoluteY > absoluteX ? absoluteY : absoluteX;
    int16_t minorDistance = absoluteY > absoluteX ? absoluteX : absoluteY;
    uint16_t octant = DetermineOctant(deltaY, deltaX);
    int16_t majorStepX = WeaponTraceMajorStepX[octant];
    int16_t majorStepY = WeaponTraceMajorStepY[octant];
    int16_t minorStepX = WeaponTraceMinorStepX[octant];
    int16_t minorStepY = WeaponTraceMinorStepY[octant];
    int16_t error = majorDistance >> 0x01;

    HideMouseCursorOverlay();
    while (startX != endX || startY != endY)
    {
        stepsUntilPlot = (uint16_t) (stepsUntilPlot - 0x01);
        if ((int16_t) stepsUntilPlot <= 0x00)
        {
            DrawClippedPixel(startX, startY, color);
            stepsUntilPlot = plotInterval;
        }
        if (startX != endX)
            startX = (int16_t) (uint16_t) ((uint16_t) startX + (uint16_t) majorStepX);
        if (startY != endY)
            startY = (int16_t) (uint16_t) ((uint16_t) startY + (uint16_t) majorStepY);
        error = (int16_t) (uint16_t) ((uint16_t) error - (uint16_t) minorDistance);
        if (error < 0x00)
        {
            if (startX != endX)
                startX = (int16_t) (uint16_t) ((uint16_t) startX + (uint16_t) minorStepX);
            if (startY != endY)
                startY = (int16_t) (uint16_t) ((uint16_t) startY + (uint16_t) minorStepY);
            error = (int16_t) (uint16_t) ((uint16_t) error + (uint16_t) majorDistance);
        }
    }
    DrawClippedPixel(startX, startY, color);
    ShowMouseCursorOverlay();
}

/* 2007:0DDB is reconstructed in Tactical/Display.cpp. */

// 2007:16E8: void AdvanceImpactSpriteAnimations(void)
// Verified against 2007:16E8-17DD.
void AdvanceImpactSpriteAnimations(void)
{
    uint16_t recordIndex = 0x00;

    HideMouseCursorOverlay();
    while (recordIndex < TacticalImpactQueueCount)
    {
        RevengeTacticalImpactRecord *record =
            &TacticalImpactQueue[recordIndex];
        int16_t relativeX = (int16_t) (uint16_t) ((record->mapX - TacticalViewportMapX) << 0x04);
        int16_t relativeY = (int16_t) (uint16_t) ((record->mapY - TacticalViewportMapY) << 0x04);
        uint16_t spriteId = record->spriteId;

        if (spriteId >= 0x01 && spriteId < 0x06)
        {
            relativeX = (int16_t) (uint16_t) ((uint16_t) relativeX - 0x0C);
            relativeY = (int16_t) (uint16_t) ((uint16_t) relativeY - 0x12);
        }
        if (spriteId >= 0x06 && spriteId < 0x0E)
        {
            relativeX = (int16_t) (uint16_t) ((uint16_t) relativeX - 0x04);
            relativeY = (int16_t) (uint16_t) ((uint16_t) relativeY - 0x06);
        }

        DrawTacticalEffectSprite(spriteId, relativeX, relativeY);
        ++spriteId;
        record->spriteId = spriteId;
        if (record->terminalSpriteId == spriteId)
        {
            uint16_t sourceIndex;
            for (sourceIndex = recordIndex + 0x01;
                sourceIndex < TacticalImpactQueueCount;
                ++sourceIndex)
            {
                TacticalImpactQueue[sourceIndex - 0x01] =
                    TacticalImpactQueue[sourceIndex];
            }
            --TacticalImpactQueueCount;
        }
        else
        {
            ++recordIndex;
        }
    }
    ShowMouseCursorOverlay();
}

// 2007:17DE: advance the 0Eh-byte tactical projectile queue.
// Verified against 2007:17DE-1AE2.
void AdvanceTacticalProjectiles(void)
{
    RevengeTacticalProjectileRecord *records;
    int16_t projectileIndex;

    records = TacticalProjectileRecords;
    HideMouseCursorOverlay();
    projectileIndex = 0x00;
    while (projectileIndex < (int16_t) TacticalProjectileCount)
    {
        RevengeTacticalProjectileRecord *projectile;
        int16_t destinationMapX;
        int16_t destinationMapY;
        int16_t viewportPixelX;
        int16_t viewportPixelY;

        projectile = &records[projectileIndex];
        viewportPixelX = (int16_t) (uint16_t) ((uint16_t) (projectile->currentMapX -
                                                   (int16_t) TacticalViewportMapX)
            << 0x04);
        viewportPixelY = (int16_t) (uint16_t) ((uint16_t) (projectile->currentMapY -
                                                   (int16_t) TacticalViewportMapY)
            << 0x04);
        DrawTacticalEffectSprite(
            projectile->spriteId, viewportPixelX, viewportPixelY);

        if (projectile->targetUnitIndex < TACTICAL_TARGET_MAP_CELL &&
            projectile->shotHit != FALSE)
        {
            int16_t targetUnitIndex = projectile->targetUnitIndex;
            destinationMapX = ScenarioUnitMapX[targetUnitIndex];
            destinationMapY = ScenarioUnitMapY[targetUnitIndex];
        }
        else
        {
            destinationMapX = projectile->destinationMapX;
            destinationMapY = projectile->destinationMapY;
        }

        if (projectile->currentMapX == destinationMapX &&
            projectile->currentMapY == destinationMapY)
        {
            uint16_t impactClass;
            int16_t sourceIndex;

            if (projectile->shotHit == FALSE ||
                projectile->targetUnitIndex == TACTICAL_TARGET_MAP_CELL)
            {
                AdvanceTerrainDamageState(
                    destinationMapX, destinationMapY, 0x03);
            }

            impactClass = 0x0A;
            if (projectile->shotHit != FALSE)
            {
                int16_t weaponIndex = projectile->weaponIndex;

                ApplyWeaponDamage(
                    projectile->attackerUnitIndex,
                    weaponIndex,
                    projectile->targetUnitIndex);
                if (WeaponDefinitions[weaponIndex].damage >
                    HEAVY_WEAPON_DAMAGE_THRESHOLD)
                {
                    impactClass = 0x06;
                }
            }
            QueueTacticalImpactVisual(
                destinationMapX, destinationMapY, impactClass);

            --projectileIndex;
            sourceIndex = projectileIndex + 0x01;
            while (sourceIndex < (int16_t) TacticalProjectileCount)
            {
                records[sourceIndex] = records[sourceIndex + 0x01];
                ++sourceIndex;
            }
            --TacticalProjectileCount;
        }
        else
        {
            uint16_t direction;

            direction = DetermineRelativeAttackDirection(
                projectile->currentMapX,
                projectile->currentMapY,
                destinationMapX,
                destinationMapY);
            if (projectile->currentMapX != destinationMapX)
            {
                projectile->currentMapX = (int16_t) (uint16_t) ((uint16_t) projectile->currentMapX +
                    DirectionDeltaXTable[direction]);
            }
            if (projectile->currentMapY != destinationMapY)
            {
                projectile->currentMapY = (int16_t) (uint16_t) ((uint16_t) projectile->currentMapY +
                    DirectionDeltaYTable[direction]);
            }
            projectile->spriteId = (uint16_t) ((projectile->spriteId & 0x00FE) |
                ((projectile->spriteId + 0x01) & 0x0001));
        }
        ++projectileIndex;
    }
    ShowMouseCursorOverlay();
}

// 2007:1219: rebuild the byte overlay map used for the current 19x12 tactical
// viewport. Each active terrain-impact record advances through three variants
// unless animation is paused, then contributes variant+1 to its visible cell.
void BuildTacticalTerrainEffectOverlay(void)
{
    RevengeTerrainImpactAnimationRecord *records;
    RevengeTerrainImpactAnimationRecord *record;
    uint16_t recordIndex;
    int16_t viewportX;
    int16_t viewportY;

    // Verified against 2007:1219-132B.
    records = TerrainImpactAnimationRecords;
    ClearTacticalEffectOverlayMap();
    for (recordIndex = 0x00; recordIndex < 0x5A; ++recordIndex)
    {
        record = &records[recordIndex];

        if (TacticalInteractionLocked == 0x00)
        {
            record->animationVariant =
                (uint16_t) (record->animationVariant + 0x01);
            if ((int16_t) record->animationVariant > 0x02)
                record->animationVariant = 0x00;
        }

        viewportX = record->mapX - (int16_t) TacticalViewportMapX;
        viewportY = record->mapY - (int16_t) TacticalViewportMapY;
        if (viewportX >= 0x00 && viewportX < TACTICAL_VIEWPORT_WIDTH_CELLS &&
            viewportY >= 0x00 && viewportY < TACTICAL_VIEWPORT_HEIGHT_CELLS)
        {
            TacticalEffectOverlayMap[viewportY * TACTICAL_VIEWPORT_WIDTH_CELLS + viewportX] =
                (uint8_t) (record->animationVariant + 0x01);
        }
    }
}

// 2007:1AE3: void AdvanceMapEdgeEffects(void)
// Verified against 2007:1AE3-1CBC.
void AdvanceMapEdgeEffects(void)
{
    RevengeMapEdgeEffectRecord *effects = MapEdgeEffectQueue;
    uint16_t effectIndex = 0x00;

    while (effectIndex < MapEdgeEffectCount)
    {
        RevengeMapEdgeEffectRecord *effect = &effects[effectIndex];
        int16_t mapX = (int16_t) effect->mapX;
        int16_t mapY = (int16_t) effect->mapY;
        int16_t impactX = mapX;
        int16_t impactY = mapY;
        uint16_t removeEffect = FALSE;

        switch (effect->edgeDirection)
        {
        case 0x00:
            impactX = mapX + (int16_t) (GeneratePseudoRandomByte() & 0x01);
            impactY = mapY + (int16_t) (GeneratePseudoRandomByte() & 0x03) - 0x06;
            --mapY;
            if (mapY < -0x01)
                removeEffect = TRUE;
            break;

        case 0x01:
            impactY = mapY - (int16_t) (GeneratePseudoRandomByte() & 0x01);
            impactX = mapX + (int16_t) (GeneratePseudoRandomByte() & 0x03) + 0x04;
            ++mapX;
            if (mapX >= (int16_t) MapWidth)
                removeEffect = TRUE;
            break;

        case 0x02:
            impactX = mapX + (int16_t) (GeneratePseudoRandomByte() & 0x01);
            impactY = mapY + (int16_t) (GeneratePseudoRandomByte() & 0x03) + 0x06;
            ++mapY;
            if (mapY >= (int16_t) MapHeight)
                removeEffect = TRUE;
            break;

        case 0x03:
            impactY = mapY - (int16_t) (GeneratePseudoRandomByte() & 0x01);
            impactX = mapX - (int16_t) (GeneratePseudoRandomByte() & 0x03) - 0x04;
            --mapX;
            if (mapX < -0x01)
                removeEffect = TRUE;
            break;

        default:
            break;
        }

        if ((GeneratePseudoRandomByte() & 0x01) != 0x00)
        {
            uint16_t weaponIndex = (GeneratePseudoRandomByte() & 0x07) +
                WEAPON_SMALL_PULSE_LASER;
            uint16_t visualEffectType = 0x0A;

            if (weaponIndex > WEAPON_SMALL_PULSE_LASER)
                visualEffectType = 0x06;
            if (weaponIndex > WEAPON_MEDIUM_PULSE_LASER)
                visualEffectType = 0x01;
            ApplyAreaTerrainAndUnitDamage(
                impactX, impactY, visualEffectType, (int16_t) weaponIndex);
        }

        effect->mapX = (uint16_t) mapX;
        effect->mapY = (uint16_t) mapY;
        if (removeEffect != FALSE)
        {
            uint16_t sourceIndex;

            for (sourceIndex = effectIndex + 0x01;
                sourceIndex < MapEdgeEffectCount; ++sourceIndex)
                effects[sourceIndex - 0x01] = effects[sourceIndex];
            --MapEdgeEffectCount;
        }
        else
            ++effectIndex;
    }
}

// 2007:021A: clip a map-cell sprite rectangle to the 19x12 viewport and
// stamp sequential sprite IDs into the display maps.
void StampTacticalSpriteRectangle(
    int16_t mapX,
    int16_t mapY,
    uint16_t widthCells,
    uint16_t heightCells,
    uint16_t firstSpriteId)
{
    uint16_t sourceRowStride;
    uint16_t viewportCellIndex;
    uint16_t column;
    uint16_t spriteId;
    int16_t viewportMapX;
    int16_t viewportMapY;
    int16_t viewportX;
    int16_t viewportY;
    int16_t rectangleRight;
    int16_t rectangleBottom;

    // Verified against 2007:021A-0327.
    sourceRowStride = widthCells;
    viewportMapX = (int16_t) TacticalViewportMapX;
    viewportMapY = (int16_t) TacticalViewportMapY;
    rectangleRight = (int16_t) (uint16_t) ((uint16_t) mapX + widthCells);
    rectangleBottom = (int16_t) (uint16_t) ((uint16_t) mapY + heightCells);

    if (mapX >= (int16_t) (uint16_t) ((uint16_t) viewportMapX +
                    TACTICAL_VIEWPORT_WIDTH_CELLS) ||
        mapY >= (int16_t) (uint16_t) ((uint16_t) viewportMapY +
                    TACTICAL_VIEWPORT_HEIGHT_CELLS) ||
        rectangleRight <= viewportMapX ||
        rectangleBottom <= viewportMapY)
    {
        // Off the viewport: nothing stamped, the overlay cleared all the same.
        TacticalOverlayTerrainSpriteId = NO_TACTICAL_OVERLAY_SPRITE_ID;
        return;
    }

    viewportX = (int16_t) (uint16_t) ((uint16_t) mapX - (uint16_t) viewportMapX);
    viewportY = (int16_t) (uint16_t) ((uint16_t) mapY - (uint16_t) viewportMapY);
    if (viewportY < 0x00)
    {
        heightCells = (uint16_t) (heightCells + (uint16_t) viewportY);
        firstSpriteId = (uint16_t) (firstSpriteId +
            (uint16_t) ((uint16_t) -viewportY * sourceRowStride));
        viewportY = 0x00;
    }
    if (viewportX < 0x00)
    {
        widthCells = (uint16_t) (widthCells + (uint16_t) viewportX);
        firstSpriteId = (uint16_t) (firstSpriteId - (uint16_t) viewportX);
        viewportX = 0x00;
    }
    if ((int16_t) (uint16_t) ((uint16_t) viewportX + widthCells) >
        TACTICAL_VIEWPORT_WIDTH_CELLS)
        widthCells = TACTICAL_VIEWPORT_WIDTH_CELLS - viewportX;
    if ((int16_t) (uint16_t) ((uint16_t) viewportY + heightCells) >
        TACTICAL_VIEWPORT_HEIGHT_CELLS)
        heightCells = TACTICAL_VIEWPORT_HEIGHT_CELLS - viewportY;

    viewportCellIndex = (uint16_t) ((uint16_t) viewportY *
            TACTICAL_VIEWPORT_WIDTH_CELLS +
        (uint16_t) viewportX);
    while (heightCells-- != 0x00)
    {
        spriteId = firstSpriteId;
        if (TacticalOverlayTerrainSpriteId !=
            NO_TACTICAL_OVERLAY_SPRITE_ID)
        {
            TacticalCellTerrainMap[viewportCellIndex] =
                TacticalOverlayTerrainSpriteId;
        }
        for (column = 0x00; column < widthCells; ++column)
        {
            TacticalCellSpriteMap[viewportCellIndex + column] =
                spriteId++;
        }
        firstSpriteId += sourceRowStride;
        viewportCellIndex += TACTICAL_VIEWPORT_WIDTH_CELLS;
    }

    TacticalOverlayTerrainSpriteId =
        NO_TACTICAL_OVERLAY_SPRITE_ID;
}

// 2007:0007: compose destruction, upgrade, unit, and edge-effect sprites into
// the tactical viewport's two 19x12 cell maps.
// VERIFIED against 2007:0007-01ED.
void ComposeTacticalSpriteMaps(void)
{
    RevengeMapEdgeEffectRecord *effects;
    uint8_t *marker;
    RevengeUnitRecord *unitRecord;
    int16_t effectCount;
    int16_t markerIndex;
    int16_t unitIndex;
    uint16_t drawUnit;
    uint16_t drewContactMarker;

    for (unitIndex = 0x0000; unitIndex < TACTICAL_UNIT_COUNT; ++unitIndex)
    {
        marker = UnitDestructionMarkers[(uint16_t) unitIndex];
        if (marker[0x02] != 0x00)
            StampTacticalSpriteRectangle(
                marker[0x00], marker[0x01],
                0x0001, 0x0001, 0x0100 + marker[0x02]);
    }
    for (markerIndex = 0x0000; markerIndex < 0x0004; ++markerIndex)
    {
        if (CampaignUpgradeAwards[markerIndex] != 0x00)
            StampTacticalSpriteRectangle(
                CampaignUpgradeMarkerX[markerIndex],
                CampaignUpgradeMarkerY[markerIndex],
                0x0001, 0x0001, 0x01DA);
    }

    for (unitIndex = 0x0000; unitIndex < TACTICAL_UNIT_COUNT; ++unitIndex)
    {
        unitRecord = &PersistentUnitRoster[unitIndex];
        if (unitRecord->deploymentState != 0x01)
            continue;

        drewContactMarker = FALSE;
        if (unitIndex < ENEMY_FORCE_FIRST_UNIT)
            drawUnit = TRUE;
        else if (TacticalOverviewMapMode == FALSE)
            drawUnit = TRUE;
        else
        {
            drawUnit = UnitEnemyVisible[unitIndex];
            if (drawUnit == FALSE &&
                UnitEnemyInProximityRange[unitIndex] != FALSE)
            {
                StampTacticalSpriteRectangle(
                    ScenarioUnitMapX[unitIndex],
                    ScenarioUnitMapY[unitIndex],
                    0x0001, 0x0001, 0x01EF);
                drewContactMarker = TRUE;
            }
        }
        if (drawUnit != FALSE)
        {
            if (UnitJumpingState[unitIndex] != 0x00 &&
                UnitJumpMovementRemaining[unitIndex] != 0x00)
                TacticalOverlayTerrainSpriteId =
                    ((ScenarioUnitMapX[unitIndex] ^
                         ScenarioUnitMapY[unitIndex]) &
                        0x01) +
                    0x01F1;
            StampTacticalSpriteRectangle(
                ScenarioUnitMapX[unitIndex],
                ScenarioUnitMapY[unitIndex],
                0x0001, 0x0001,
                UnitSpriteId[unitIndex]);
        }
        else if (unitIndex >= ENEMY_FORCE_FIRST_UNIT &&
            unitIndex == TacticalSelectedUnitIndex &&
            drewContactMarker == FALSE)
            TacticalSelectedUnitIndex = TACTICAL_UNIT_NONE;
    }

    effectCount = (int16_t) MapEdgeEffectCount;
    effects = MapEdgeEffectQueue;
    for (markerIndex = 0x0000; markerIndex < effectCount; ++markerIndex)
        StampTacticalSpriteRectangle(
            effects[markerIndex].mapX,
            effects[markerIndex].mapY,
            0x0002, 0x0002,
            effects[markerIndex].spriteId);
}

// 2007:0650: void RebuildTacticalViewport(void)
// Verified against 2007:0650-0799.
void RebuildTacticalViewport(void)
{
    const uint8_t *spritePage = PlanarScreenBuffer;
    uint16_t viewportCellIndex = 0x00;
    uint16_t row;
    uint16_t column;
    uint16_t statusSlot;
    uint16_t destinationY;

    BuildTacticalTerrainCellMaps(
        TerrainTypeMap,
        TacticalCellSpriteMap,
        TacticalViewportMapX,
        TacticalViewportMapY);
    BuildTacticalTerrainEffectOverlay();
    ComposeTacticalSpriteMaps();

    for (row = 0x00; row < TACTICAL_VIEWPORT_HEIGHT_CELLS; ++row)
    {
        for (column = 0x00; column < TACTICAL_VIEWPORT_WIDTH_CELLS; ++column)
        {
            DrawTacticalViewportCell(column, row, viewportCellIndex);
            ++viewportCellIndex;
        }
    }

    if (TacticalOverviewActive != 0x00)
        return;

    destinationY = 0x00;
    for (statusSlot = 0x00; statusSlot < 0x07; ++statusSlot)
    {
        uint8_t runtimeValue = UnitTypeResourceCode[statusSlot];
        int8_t runtimeState = TacticalSlotSpriteState[statusSlot];
        if (runtimeValue != 0x00 &&
            runtimeState != TacticalStatusRowInvalidation[statusSlot])
        {
            uint16_t firstSpriteId = 0x0100 + runtimeValue;
            uint16_t secondSpriteId = 0x01F6 + runtimeState;
            /* The CGA's compositor (2007:06DF, 0800:09BB) went with the CGA. */
            DrawCompositeTacticalTile(
                firstSpriteId, secondSpriteId,
                spritePage, 0x00, destinationY);
            TacticalStatusRowInvalidation[statusSlot] = runtimeState;
        }
        destinationY += 0x14;
        if (statusSlot == 0x04)
            destinationY += 0x04;
    }

    if (TacticalContactMarkerPending == 0xFF)
    {
        DrawTacticalTileSpriteForAdapter(
            spritePage, 0x01EF, 0x00, 0x0098);
        TacticalContactMarkerPending = 0x00;
    }
    if (TacticalStatusHeaderSpriteId != 0x00)
    {
        DrawTacticalTileSpriteForAdapter(
            spritePage,
            0x0100 + TacticalStatusHeaderSpriteId,
            0x00,
            0x00B0);
    }
}

// 2007:01EE: void DrawTileAlignedText(
//     char far *text, word16 tileX, word16 tileY,
//     byte foregroundColor, byte backgroundColor)
void DrawTileAlignedText(
    const char *text,
    uint16_t tileX,
    uint16_t tileY,
    uint8_t foregroundColor,
    uint8_t backgroundColor)
{
    DrawTextString(
        text,
        tileX << 0x03,
        tileY << 0x03,
        foregroundColor,
        backgroundColor);
}

// 2007:0C6F: draw a one-pixel border immediately outside the active text
// window's tile rectangle.
// Verified against 2007:0C6F-0CFA.
void DrawActiveTextWindowBorder(uint16_t color)
{
    uint16_t leftX = (uint16_t) ((uint16_t) (ActiveTextWindowLeftTile << 0x03) - 0x01);
    uint16_t topY = (uint16_t) ((uint16_t) (ActiveTextWindowTopTile << 0x03) - 0x01);
    uint16_t rightX = (uint16_t) (leftX + (uint16_t) (ActiveTextWindowWidthTiles << 0x03));
    uint16_t bottomY = (uint16_t) (topY + (uint16_t) (ActiveTextWindowHeightTiles << 0x03));

    DrawClippedAxisAlignedLine(leftX, topY, rightX, topY, color);
    DrawClippedAxisAlignedLine(leftX, topY, leftX, bottomY, color);
    DrawClippedAxisAlignedLine(rightX, topY, rightX, bottomY, color);
    DrawClippedAxisAlignedLine(leftX, bottomY, rightX, bottomY, color);
}

/* 2007:1CBD is reconstructed in Tactical/Display.cpp. */

// 2007:14DF: update the outline around one primary unit-status row only when
// its row or blink color changed. A row change marks the previous row for
// redraw with one byte (2007:152D-1531: mov byte ptr [bx+0B574h],0FFh).
void UpdatePrimaryUnitStatusRowHighlight(uint16_t rowIndex)
{
    uint16_t previousRow = PrimaryStatusRow;
    uint16_t color = PrimaryStatusHighlightColor;
    int16_t topY;

    if (TacticalUiSuppressed != FALSE ||
        (previousRow == rowIndex && CachedPrimaryStatusHighlightColor == color))
        return;
    topY = rowIndex * 0x14;
    if (rowIndex >= 0x05)
        topY += 0x04;
    if (rowIndex == 0x07)
        topY += 0x08;
    DrawTacticalCellOutline(0x00, topY, color);
    if (previousRow != rowIndex)
        TacticalStatusRowInvalidation[previousRow] = 0xFF;
    PrimaryStatusRow = rowIndex;
    CachedPrimaryStatusHighlightColor = color;
}

// 2007:1544: equivalent cached outline update for the secondary command row;
// the previous row is marked the same way (2007:1588-1590).
void UpdateSecondaryUnitStatusRowHighlight(uint16_t rowIndex)
{
    uint16_t previousRow = SecondaryStatusRow;
    uint16_t color = SecondaryStatusHighlightColor;
    int16_t topY;

    if (previousRow == rowIndex && CachedSecondaryStatusHighlightColor == color)
        return;
    topY = rowIndex * 0x14;
    if (rowIndex >= 0x05)
        topY += 0x04;
    if (rowIndex >= 0x07)
        return;
    DrawTacticalCellOutline(0x00, topY, color);
    if (previousRow != rowIndex)
        TacticalStatusRowInvalidation[previousRow] = 0xFF;
    SecondaryStatusRow = rowIndex;
    CachedSecondaryStatusHighlightColor = color;
}
