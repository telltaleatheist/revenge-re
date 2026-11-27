// 0800: tactical geometry
// GraphicsRuntime.cpp
// Runtime, codec, tactical-map, and video procedures reconstructed from
// REVENGE.EXE segment 0800.

// Tactical geometry and terrain procedures from REVENGE.EXE segment 0800.
#include "Data/TacticalData.h"
#include "Data/TacticalGraphicsData.h"
#include "Tactical/Tactical.h"
#include "revenge/platform_backend.h"
#include "revenge/video.h"

#include <stdint.h>
#include <string.h>

// 0800:06D1: uint16 DetermineRelativeAttackDirection(
//     int16 sourceX, int16 sourceY, int16 targetX, int16 targetY)
uint16_t DetermineRelativeAttackDirection(
    int16_t sourceX,
    int16_t sourceY,
    int16_t targetX,
    int16_t targetY)
{
    int16_t deltaY;
    int16_t deltaX;
    int16_t absoluteY;
    int16_t absoluteX;
    int16_t doubledDelta;
    uint16_t comparisonMask;

    deltaY = (int16_t) ((uint16_t) sourceY - (uint16_t) targetY);
    deltaX = (int16_t) ((uint16_t) targetX - (uint16_t) sourceX);
    absoluteY = deltaY;
    if (absoluteY < 0x0000)
    {
        absoluteY = (int16_t) (0x0000 - (uint16_t) absoluteY);
    }
    absoluteX = deltaX;
    if (absoluteX < 0x0000)
    {
        absoluteX = (int16_t) (0x0000 - (uint16_t) absoluteX);
    }

    comparisonMask = 0x0000;
    doubledDelta = (int16_t) ((uint16_t) deltaY << 0x01);
    if (doubledDelta >= absoluteX)
    {
        comparisonMask |= 0x0008;
    }
    doubledDelta = (int16_t) (0x0000 - (uint16_t) doubledDelta);
    if (doubledDelta >= absoluteX)
    {
        comparisonMask |= 0x0004;
    }
    doubledDelta = (int16_t) ((uint16_t) deltaX << 0x01);
    if (doubledDelta >= absoluteY)
    {
        comparisonMask |= 0x0002;
    }
    doubledDelta = (int16_t) (0x0000 - (uint16_t) doubledDelta);
    if (doubledDelta >= absoluteY)
    {
        comparisonMask |= 0x0001;
    }

    return RelativeAttackDirectionByMask[comparisonMask];
}

// 0800:078C: V TraceMapLineOfSight.
// Walk a Bresenham-like grid line over the active terrain map. Return TRUE on
// reaching the target; return FALSE at the step limit, after three cells in
// the special-terrain range, or at the first terrain ID at/above the blocking
// threshold.
// Verified against 0800:078C-0929.
uint16_t TraceMapLineOfSight(
    int16_t sourceX,
    int16_t sourceY,
    int16_t targetX,
    int16_t targetY,
    const uint8_t *terrainTypeMap,
    uint8_t specialTerrainMinimum,
    uint8_t specialTerrainMaximum,
    uint8_t blockingTerrainThreshold,
    uint8_t maximumSteps)
{
    int16_t verticalDelta;
    int16_t horizontalDelta;
    int16_t absoluteVerticalDelta;
    int16_t absoluteHorizontalDelta;
    int16_t majorDelta;
    int16_t minorDelta;
    int16_t error;
    int16_t currentX;
    int16_t currentY;
    uint16_t terrainOffset;
    uint16_t direction;
    uint8_t specialTerrainRemaining;

    currentX = sourceX;
    currentY = sourceY;
    /* The original adds the map's far offset (8, the heap's payload offset)
	 * and keeps the sum in 16 bits, inside the map's segment. The walk stays
	 * between source and target, so every cell it reads is inside the map
	 * (at most 4E20h bytes, 1A1F's TACTICAL_OVERVIEW_MAP_OFFSET): the sum
	 * never wraps, and the offset here starts at the map's first byte. */
    terrainOffset = (uint16_t) ((uint16_t) sourceX +
        (uint16_t) ((uint8_t) sourceY *
            (uint8_t) LinePrimaryRowOffset[0x03]));
    verticalDelta = (int16_t) ((uint16_t) sourceY - (uint16_t) targetY);
    horizontalDelta = (int16_t) ((uint16_t) targetX - (uint16_t) sourceX);
    absoluteVerticalDelta = verticalDelta;
    if (absoluteVerticalDelta < 0x00)
    {
        absoluteVerticalDelta = (int16_t) (0x00 - (uint16_t) absoluteVerticalDelta);
    }
    absoluteHorizontalDelta = horizontalDelta;
    if (absoluteHorizontalDelta < 0x00)
    {
        absoluteHorizontalDelta = (int16_t) (0x00 - (uint16_t) absoluteHorizontalDelta);
    }
    majorDelta = absoluteVerticalDelta;
    minorDelta = absoluteHorizontalDelta;
    if (absoluteHorizontalDelta > absoluteVerticalDelta)
    {
        majorDelta = absoluteHorizontalDelta;
        minorDelta = absoluteVerticalDelta;
    }

    if (verticalDelta < 0x00)
    {
        if (horizontalDelta >= 0x01)
        {
            direction = 0x02;
            if ((int16_t) (0x00 - (uint16_t) verticalDelta) >
                horizontalDelta)
            {
                ++direction;
            }
        }
        else
        {
            direction = 0x04;
            if ((int16_t) (0x00 - (uint16_t) horizontalDelta) >
                (int16_t) (0x00 - (uint16_t) verticalDelta))
            {
                ++direction;
            }
        }
    }
    else if (horizontalDelta >= 0x01)
    {
        direction = 0x00;
        if (horizontalDelta > verticalDelta)
        {
            ++direction;
        }
    }
    else
    {
        direction = 0x06;
        if (verticalDelta >
            (int16_t) (0x00 - (uint16_t) horizontalDelta))
        {
            ++direction;
        }
    }

    error = (int16_t) ((uint16_t) majorDelta >> 0x01);
    specialTerrainRemaining = 0x03;
    for (;;)
    {
        uint8_t terrainType;
        const uint8_t *terrainCell;

        --maximumSteps;
        if (maximumSteps == 0x00)
        {
            return FALSE;
        }
        if (currentX == targetX && currentY == targetY)
        {
            return TRUE;
        }

        terrainCell = terrainTypeMap + terrainOffset;
        terrainType = *terrainCell;
        if (terrainType >= blockingTerrainThreshold)
        {
            return FALSE;
        }
        if (terrainType >= specialTerrainMinimum &&
            terrainType <= specialTerrainMaximum)
        {
            --specialTerrainRemaining;
            if (specialTerrainRemaining == 0x00)
            {
                return FALSE;
            }
        }

        if (currentX != targetX)
        {
            currentX = (int16_t) ((uint16_t) currentX +
                (uint16_t) LinePrimaryXStep[direction]);
            terrainOffset = (uint16_t) (terrainOffset +
                (uint16_t) LinePrimaryXStep[direction]);
        }
        if (currentY != targetY)
        {
            currentY = (int16_t) ((uint16_t) currentY +
                (uint16_t) LinePrimaryYStep[direction]);
            terrainOffset = (uint16_t) (terrainOffset +
                (uint16_t) LinePrimaryRowOffset[direction]);
        }

        error = (int16_t) ((uint16_t) error - (uint16_t) minorDelta);
        if (error >= 0x00)
        {
            continue;
        }
        error = (int16_t) ((uint16_t) error + (uint16_t) majorDelta);
        if (currentX != targetX)
        {
            currentX = (int16_t) ((uint16_t) currentX +
                (uint16_t) LineSecondaryXStep[direction]);
            terrainOffset = (uint16_t) (terrainOffset +
                (uint16_t) LineSecondaryXStep[direction]);
        }
        if (currentY != targetY)
        {
            currentY = (int16_t) ((uint16_t) currentY +
                (uint16_t) LineSecondaryYStep[direction]);
            terrainOffset = (uint16_t) (terrainOffset +
                (uint16_t) LineSecondaryRowOffset[direction]);
        }
    }
}

// 0800:092A: int16 InterpolateLineCoordinate(int16 clipCoordinate,
//     int16 lineStartCoordinate, int16 otherStartCoordinate,
//     int16 otherEndCoordinate, int16 lineEndCoordinate)
int16_t InterpolateLineCoordinate(
    int16_t clipCoordinate,
    int16_t lineStartCoordinate,
    int16_t otherStartCoordinate,
    int16_t otherEndCoordinate,
    int16_t lineEndCoordinate)
{
    int16_t boundaryDelta;
    int16_t otherDelta;
    int16_t lineDelta;
    int32_t numerator;
    int16_t interpolatedDelta;

    if (lineEndCoordinate == lineStartCoordinate)
    {
        return clipCoordinate;
    }

    boundaryDelta = (int16_t) ((uint16_t) clipCoordinate -
        (uint16_t) lineStartCoordinate);
    otherDelta = (int16_t) ((uint16_t) otherStartCoordinate -
        (uint16_t) otherEndCoordinate);
    lineDelta = (int16_t) ((uint16_t) lineEndCoordinate -
        (uint16_t) lineStartCoordinate);
    numerator = (int32_t) boundaryDelta * (int32_t) otherDelta;
    interpolatedDelta = (int16_t) (numerator / lineDelta);
    return (int16_t) ((uint16_t) interpolatedDelta +
        (uint16_t) otherEndCoordinate);
}

// 0800:0A89: translate the map-cell bytes used by the tactical overview into
// display colors and pad each destination row to an eight-byte boundary.
// The four color bands are the literal CS:04C9 translation table used by XLAT.
void BuildTacticalOverviewMapBuffer(
    const uint8_t *source,
    uint8_t *destination,
    uint16_t width,
    uint16_t height)
{
    static const uint8_t overviewPixelTranslation[0x0100] = {
        0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
        0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
        0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
        0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
        0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
        0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
        0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
        0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
        0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
        0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
        0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
        0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
        0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
        0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
        0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
        0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
        0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE,
        0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE,
        0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE,
        0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE,
        0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33,
        0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33,
        0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33,
        0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33,
        0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33,
        0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00
    };
    uint16_t paddedWidth;
    uint16_t row;
    uint16_t column;

    paddedWidth = (width + 0x0007) & 0xFFF8;
    for (row = 0x0000; row < height; ++row)
    {
        for (column = 0x0000; column < width; ++column)
        {
            uint8_t terrainType = *source++;

            if (terrainType < 0x80)
                *destination++ = 0x77;
            else if (terrainType < 0xA0)
                *destination++ = 0xEE;
            else if (terrainType < 0xC0)
                *destination++ = 0x33;
            else if (terrainType < 0xC6)
                *destination++ = 0xAA;
            else
                *destination++ = 0x00;
        }
        for (; column < paddedWidth; ++column)
            *destination++ = overviewPixelTranslation[0x00];
    }
}

constexpr uint16_t DAMAGED_TERRAIN_SPRITE_FLAG = 0x0200;


// 0800:0999: clear the 19x12 byte overlay map used while composing transient
// terrain-impact sprites. DS:7978 and 4184:07D8 are physical aliases.
void ClearTacticalEffectOverlayMap(void)
{
    uint16_t cellIndex;
    for (cellIndex = 0x00;
        cellIndex < TACTICAL_VIEWPORT_CELL_COUNT;
        ++cellIndex)
    {
        TacticalEffectOverlayMap[cellIndex] = 0x00;
    }
}

// 0800:05D3: build the two adjacent 19x12 tactical viewport cell maps.
// Called from:
//      RebuildTacticalViewport
void BuildTacticalTerrainCellMaps(
    const uint8_t *terrainTypeMap,
    uint16_t *tacticalCellSpriteMap,
    uint16_t viewportX,
    uint16_t viewportY)
{
    const uint8_t *damagedTerrainSpriteLookup;
    const uint8_t *packedDamageRow;
    const uint8_t *packedDamage;
    const uint8_t *terrainRow;
    uint16_t *terrainCellMap;
    uint16_t mapWidth;
    uint16_t packedRowStride;
    uint16_t terrainRowOffset;
    uint16_t packedDamageRowOffset;
    uint16_t cellIndex;
    uint16_t row;
    uint16_t column;
    uint8_t damageMask;
    uint8_t damageState;
    uint8_t shift;
    uint8_t terrainType;
    uint16_t terrainSprite;

    // Verified against 0800:05D3-06D0. The original byte MUL operations
    // deliberately use only the low bytes of viewportY and the row strides.
    mapWidth = (uint16_t) LinePrimaryRowOffset[0x03];
    packedRowStride = (uint16_t) (((mapWidth + 0x03) & 0x01FC) >> 0x02);
    // The original adds the map's far offset (8) in 16 bits too. Neither sum
    // can wrap: at most 8 + FFh + FFh * FFh = FF08h for the terrain row, and
    // 8 + A000h + 3Fh + FFh * 40h = E007h for the damage row.
    terrainRowOffset = (uint16_t) (viewportX +
        (uint8_t) viewportY * (uint8_t) mapWidth);
    packedDamageRowOffset = (uint16_t) (PACKED_TERRAIN_DAMAGE_MAP_OFFSET +
        (viewportX >> 0x02) +
        (uint8_t) viewportY * (uint8_t) packedRowStride);
    terrainRow = terrainTypeMap + terrainRowOffset;
    packedDamageRow = terrainTypeMap + packedDamageRowOffset;
    terrainCellMap = tacticalCellSpriteMap + TACTICAL_VIEWPORT_CELL_COUNT;
    damagedTerrainSpriteLookup = DamagedTerrainSpriteLookup;

    for (cellIndex = 0x00;
        cellIndex < TACTICAL_VIEWPORT_CELL_COUNT;
        ++cellIndex)
    {
        tacticalCellSpriteMap[cellIndex] = 0x0000;
    }

    for (row = 0x00; row < TACTICAL_VIEWPORT_HEIGHT_CELLS; ++row)
    {
        packedDamage = packedDamageRow;
        damageMask = (uint8_t) (0xC0 >> ((viewportX & 0x03) << 0x01));
        for (column = 0x00;
            column < TACTICAL_VIEWPORT_WIDTH_CELLS;
            ++column)
        {
            terrainType = *terrainRow++;
            damageState = *packedDamage & damageMask;
            terrainSprite = terrainType;

            if (damageState != 0x00)
            {
                shift = (uint8_t) (0x06 - (((viewportX + column) & 0x03) << 0x01));
                damageState = (uint8_t) (damageState >> shift);
                terrainSprite = DAMAGED_TERRAIN_SPRITE_FLAG |
                    damagedTerrainSpriteLookup[((uint16_t) (damageState - 0x01) << 0x08) +
                        terrainType];
            }
            terrainCellMap[row * TACTICAL_VIEWPORT_WIDTH_CELLS + column] =
                terrainSprite;

            damageMask >>= 0x02;
            if (damageMask == 0x00)
            {
                damageMask = 0xC0;
                ++packedDamage;
            }
        }
        terrainRow += mapWidth - TACTICAL_VIEWPORT_WIDTH_CELLS;
        packedDamageRow += packedRowStride;
    }
}
