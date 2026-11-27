// 3A6A: tactical control data declarations
#ifndef REVENGE_DATA_TACTICAL_CONTROL_DATA_H
#define REVENGE_DATA_TACTICAL_CONTROL_DATA_H

#include "revenge/types.h"

extern uint32_t GameTimerTicks;
extern uint32_t GameAnimationTicks;
extern uint16_t TacticalSelectedUnitIndex;
extern uint16_t TacticalViewportMapX;
extern uint16_t TacticalViewportMapY;
extern uint16_t TacticalCursorViewportX;
extern uint16_t TacticalCursorViewportY;
extern uint8_t TacticalInteractionLocked;
extern uint16_t TacticalMenuInputState;
extern uint8_t TacticalOverviewMarkerX[0x18];
extern uint16_t TacticalOverviewViewportX;
extern uint8_t TacticalOverviewMarkerY[0x18];
extern uint16_t TacticalOverviewViewportY;
extern uint16_t TacticalOverviewScreenOriginX;
extern uint16_t NarrationFocusRegion;
extern uint16_t TacticalOverviewClockNoticeActive;
extern uint16_t TacticalRuntimeState;
extern uint16_t PreviousMouseCellX;
extern uint16_t PreviousMouseCellY;
extern uint8_t TacticalOverviewNoticeChance;
extern uint8_t EjectedPilotMapX;
extern uint8_t EjectedPilotMapY;
extern uint8_t EjectedPilotState;
extern uint16_t TacticalCursorRuntimeState;
extern uint16_t TacticalCursorMapX;
extern uint16_t TacticalCursorMapY;
extern uint16_t LastOverviewNoticeHour;
extern uint16_t TacticalOverviewRuntimeState;
extern uint8_t PrimaryLanceResourceCode;
/* TacticalEffects' and TacticalStatus' names for these two in their other
 * use. */
#define TacticalOverlayTerrainSpriteId TacticalRuntimeState
#define TacticalStatusHeaderSpriteId   PrimaryLanceResourceCode
extern uint16_t EnemyCycleUnitIndex;
extern uint16_t NarrationMessageCategory;
extern uint16_t PendingNarrationDelay;
extern uint16_t TacticalUpdateDelayTicks;
extern uint16_t TacticalKeyRepeatDelayTicks;
extern uint16_t TacticalUpdateDelayReloadTicks;
extern uint16_t TacticalGameSpeedIndex;
extern uint16_t TacticalBattleResult;
/* TacticalBattleResult: the battle runs; every enemy unit is out or
 * immobile, or a scene ended the battle so; unit 0 is out, the player's
 * force is gone, or a scene ended it so; the player left for DOS. */
inline constexpr uint16_t TACTICAL_BATTLE_RUNNING = 0x0000;
inline constexpr uint16_t TACTICAL_BATTLE_WON = 0x0002;
inline constexpr uint16_t TACTICAL_BATTLE_ENDED = 0x0003;
inline constexpr uint16_t TACTICAL_BATTLE_EXIT_TO_DOS = 0x0004;
extern uint16_t TacticalClockDivider;
extern const int16_t TacticalCursorSearchOffsetX[0x09];
extern const int16_t TacticalCursorSearchOffsetY[0x09];
extern const int8_t TacticalMouseEdgeInputByMask[0x10];
extern uint8_t SceneNarrationEventFlags[0x06];
extern uint16_t TacticalOverviewDisplayMode;
extern uint16_t TacticalOverviewMapMode;
extern uint16_t NarrationSourceUnit;
extern uint16_t TacticalOverviewActive;
extern uint16_t TacticalOverviewViewportDirty;
extern uint8_t TacticalStateInitialized;
extern uint16_t SuppressCampaignClock;
extern uint16_t TacticalCursorOutlineEnabled;
extern uint16_t TacticalCursorOutlineState;

#endif
