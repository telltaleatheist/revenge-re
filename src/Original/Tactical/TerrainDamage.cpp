// 2D1B: terrain damage
// Procedures preserved from REVENGE_2D1B_TACTICAL_COMBAT.c.

#include "CombatInternal.h"
#include "Data/TacticalTables.h"
#include "Data/TacticalMapData.h"

// 2D1B:23F8: void ApplyOrQueueTerrainDamage(
//     word16 packedByteIndex, word16 subcellIndex, word16 terrainType)
// Ordinary terrain enters damage state one immediately. The three special
// destructible terrain types are queued as packed-byte/subcell pairs for the
// staged updater at 2007:042A.
void ApplyOrQueueTerrainDamage(
    uint16_t packedByteIndex,
    uint16_t subcellIndex,
    uint16_t terrainType)
{
    if (terrainType < DestructibleTerrainTypeBase ||
        terrainType > DestructibleTerrainTypeBase + 0x02)
    {
        PackedTerrainDamageMap[packedByteIndex] |=
            TerrainDamageQueueIncrementMask[subcellIndex];
        return;
    }

    if (TacticalTerrainEffectMode == 0x06 ||
        TerrainDamageQueueWordCount >= 0x32)
    {
        return;
    }

    TerrainDamageQueue[TerrainDamageQueueWordCount >> 0x01].packedByteIndex =
        packedByteIndex;
    TerrainDamageQueue[TerrainDamageQueueWordCount >> 0x01].subcellIndex =
        subcellIndex;
    TerrainDamageQueueWordCount += 0x02;
}

// 2D1B:23AC: void BeginTerrainDamageAtCell(
//     word16 mapX, word16 mapY, word16 terrainType)
// Convert a map coordinate to its packed two-bit terrain-damage field and
// begin damage only when that field is still zero.
void BeginTerrainDamageAtCell(
    uint16_t mapX,
    uint16_t mapY,
    uint16_t terrainType)
{
    uint16_t packedByteIndex =
        mapY * PackedTerrainDamageRowBytes +
        (uint16_t) ((int16_t) mapX >> 0x02);
    uint16_t subcellIndex = mapX & 0x03;
    uint8_t occupiedMask = TerrainDamageQueueOccupiedMask[subcellIndex];

    if ((PackedTerrainDamageMap[packedByteIndex] &
            occupiedMask) == 0x00)
    {
        ApplyOrQueueTerrainDamage(
            packedByteIndex, subcellIndex, terrainType);
    }
}
