#ifndef REVENGE_TACTICAL_DATA_H
#define REVENGE_TACTICAL_DATA_H

#include <stddef.h>

#include "revenge/types.h"

enum
{
    TACTICAL_PROJECTILE_RECORD_COUNT = 0x50,
    TACTICAL_IMPACT_RECORD_COUNT = 0x50,
    TERRAIN_IMPACT_ANIMATION_RECORD_COUNT = 0x5A,
    MAP_EDGE_EFFECT_RECORD_COUNT = 0x03,
    TERRAIN_DAMAGE_QUEUE_ENTRY_COUNT = 0x19
};

#pragma pack(push, 1)
typedef struct RevengeTacticalProjectileRecord
{
    uint8_t weaponIndex;
    RevengeUnitSlot targetUnitIndex;
    uint8_t shotHit;
    RevengeUnitSlot attackerUnitIndex;
    uint16_t spriteId;
    int16_t currentMapX;
    int16_t currentMapY;
    int16_t destinationMapX;
    int16_t destinationMapY;
} RevengeTacticalProjectileRecord;

typedef struct RevengeTacticalImpactRecord
{
    int16_t mapX;
    int16_t mapY;
    uint16_t spriteId;
    uint16_t terminalSpriteId;
} RevengeTacticalImpactRecord;

typedef struct RevengeTerrainImpactAnimationRecord
{
    int16_t mapX;
    int16_t mapY;
    uint16_t animationVariant;
} RevengeTerrainImpactAnimationRecord;

typedef struct RevengeMapEdgeEffectRecord
{
    int16_t mapX;
    int16_t mapY;
    uint16_t spriteId;
    uint16_t edgeDirection;
} RevengeMapEdgeEffectRecord;

typedef struct RevengeTerrainDamageQueueEntry
{
    uint16_t packedByteIndex;
    uint16_t subcellIndex;
} RevengeTerrainDamageQueueEntry;
#pragma pack(pop)

static_assert(sizeof(RevengeTacticalProjectileRecord) == 0x0E,
    "Tactical projectile records must retain their DOS layout");
static_assert(offsetof(RevengeTacticalProjectileRecord, spriteId) == 0x04,
    "Projectile sprite ID must remain at record offset 04h");
static_assert(sizeof(RevengeTacticalImpactRecord) == 0x08,
    "Tactical impact records must retain their four-word DOS layout");
static_assert(offsetof(RevengeTacticalProjectileRecord, destinationMapY) == 0x0C,
    "Projectile destination Y must remain at record offset 0Ch");
static_assert(sizeof(RevengeTerrainImpactAnimationRecord) == 0x06,
    "Terrain-impact records must retain their DOS layout");
static_assert(sizeof(RevengeMapEdgeEffectRecord) == 0x08,
    "Map-edge effect records must retain their DOS layout");
static_assert(sizeof(RevengeTerrainDamageQueueEntry) == 0x04,
    "Terrain-damage queue entries must retain their DOS layout");

#endif
