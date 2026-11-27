// 3A6A: tactical map data declarations
#ifndef REVENGE_DATA_TACTICAL_MAP_DATA_H
#define REVENGE_DATA_TACTICAL_MAP_DATA_H

#include "revenge/tactical_data.h"
#include "revenge/types.h"

extern uint8_t *DamagedTerrainSpriteLookup;
extern uint8_t TacticalEffectOverlayMap[0x00E4];
extern RevengeMapEdgeEffectRecord
    MapEdgeEffectQueue[MAP_EDGE_EFFECT_RECORD_COUNT];
extern uint16_t MapEdgeEffectCount;
extern const int8_t WeaponTraceMajorStepX[0x08];
extern const int8_t WeaponTraceMajorStepY[0x08];
extern const int8_t WeaponTraceMinorStepX[0x08];
extern const int8_t WeaponTraceMinorStepY[0x08];
extern uint8_t *TerrainTypeMap;
extern uint8_t *TacticalOverviewMapBuffer;
extern uint8_t *PackedTerrainDamageMap;
extern uint8_t *ActiveTerrainDamageByte;
extern uint8_t *TerrainDamageCapabilityTable;
extern const uint8_t TerrainDamageCellIncrementMask[0x04];
extern const uint8_t TerrainDamageExtractMask[0x04];
extern const uint8_t TerrainDamageShift[0x04];
extern const uint8_t TerrainDamageCellOccupiedMask[0x04];
extern const uint8_t TerrainDamageStageIncrement[0x04];
extern uint16_t PackedTerrainDamageRowBytes;
extern uint8_t DestructibleTerrainTypeBase;
extern uint8_t TerrainImpactAnimationTypeMinimum;
extern uint8_t TerrainImpactAnimationTypeMaximum;
extern uint16_t TerrainLineOfSightBlockingThreshold;
extern uint8_t TerrainLineOfSightMaximumSteps;
extern uint8_t TacticalTerrainEffectMode;
extern RevengeTerrainDamageQueueEntry
    TerrainDamageQueue[TERRAIN_DAMAGE_QUEUE_ENTRY_COUNT];
extern uint16_t ImpassableTerrainTypeMinimum;
extern int8_t TerrainToHitModifier[0x100];
extern RevengeTerrainImpactAnimationRecord
    TerrainImpactAnimationRecords[TERRAIN_IMPACT_ANIMATION_RECORD_COUNT];
extern uint16_t TacticalImpactQueueCount;
extern RevengeTacticalImpactRecord
    TacticalImpactQueue[TACTICAL_IMPACT_RECORD_COUNT];
extern uint16_t TerrainDamageQueueWordCount;
extern uint16_t TacticalProjectileCount;
extern RevengeTacticalProjectileRecord
    TacticalProjectileRecords[TACTICAL_PROJECTILE_RECORD_COUNT];
extern uint8_t ArtilleryShellingActive;
extern uint8_t RollingBarrageActive;
extern uint8_t StrafingRunActive;
extern uint8_t ArtilleryTargetMapX;
extern uint8_t ArtilleryTargetMapY;
extern uint8_t RollingBarrageEndMapX;
extern uint8_t RollingBarrageEndMapY;
extern uint8_t RollingBarrageStartMapX;
extern uint8_t RollingBarrageStartMapY;
extern uint8_t DropShipMapX;
extern uint8_t DropShipMapY;
extern uint8_t ArtillerySupportAvailable;
extern uint8_t StrafingSupportAvailable;
extern uint16_t *TacticalCellTerrainMap;
extern uint16_t *TacticalCellSpriteMap;

#endif
