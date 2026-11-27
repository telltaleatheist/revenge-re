// 3A6A: tactical control data
// TacticalControlData.cpp
// Typed objects recovered from REVENGE.EXE DS segment 3A6A.

#include "Data/TacticalControlData.h"

uint32_t GameTimerTicks;
uint32_t GameAnimationTicks;
/* DS:03DC starts at FFFFh: no unit selected. */
uint16_t TacticalSelectedUnitIndex = 0xFFFF;
uint16_t TacticalViewportMapX;
uint16_t TacticalViewportMapY;
uint16_t TacticalCursorViewportX;
uint16_t TacticalCursorViewportY;
uint8_t TacticalInteractionLocked;
uint16_t TacticalMenuInputState;
uint8_t TacticalOverviewMarkerX[0x18];
uint16_t TacticalOverviewViewportX;
uint8_t TacticalOverviewMarkerY[0x18];
uint16_t TacticalOverviewViewportY;
uint16_t TacticalOverviewScreenOriginX;
uint16_t NarrationFocusRegion;
uint16_t TacticalOverviewClockNoticeActive;
uint16_t TacticalRuntimeState;
uint16_t PreviousMouseCellX;
uint16_t PreviousMouseCellY;
uint8_t TacticalOverviewNoticeChance;
uint8_t EjectedPilotMapX;
uint8_t EjectedPilotMapY;
uint8_t EjectedPilotState;
uint16_t TacticalCursorRuntimeState;
uint16_t TacticalCursorMapX;
uint16_t TacticalCursorMapY;
uint16_t LastOverviewNoticeHour;
uint16_t TacticalOverviewRuntimeState;
uint8_t PrimaryLanceResourceCode;
uint16_t EnemyCycleUnitIndex;
uint16_t NarrationMessageCategory;
uint16_t PendingNarrationDelay;
uint16_t TacticalUpdateDelayTicks;
uint16_t TacticalKeyRepeatDelayTicks;
/* Filled at start from data/start/game.json (revenge/rules.h). */
uint16_t TacticalUpdateDelayReloadTicks;
uint16_t TacticalGameSpeedIndex;
uint16_t TacticalBattleResult;
uint16_t TacticalClockDivider;
const int16_t TacticalCursorSearchOffsetX[0x09] = { 0x00, 0x00, 0x01, 0x00, -0x01, -0x01, 0x01, 0x01, -0x01 };
const int16_t TacticalCursorSearchOffsetY[0x09] = { 0x00, -0x01, 0x00, 0x01, 0x00, -0x01, -0x01, 0x01, 0x01 };
/* 1A1F:58A0 sign-extends these (cbw), so the extended key codes B0h-B9h
 * arrive negative. */
const int8_t TacticalMouseEdgeInputByMask[0x10] = { 0x00, -0x4B, -0x50, -0x4F, -0x4D, 0x00, -0x51, 0x00,
    -0x48, -0x47, 0x00, 0x00, -0x49, 0x00, 0x00, 0x00 };
uint8_t SceneNarrationEventFlags[0x06];
uint16_t TacticalOverviewDisplayMode;
/* DS:03E5 starts at 0001h, as the image has it, until a battle's scene
 * header sets it (1A1F:1C48). */
uint16_t TacticalOverviewMapMode = 0x0001;
uint16_t NarrationSourceUnit;

uint16_t TacticalOverviewActive;
uint16_t TacticalOverviewViewportDirty;
uint8_t TacticalStateInitialized;
uint16_t SuppressCampaignClock;
uint16_t TacticalCursorOutlineEnabled;
uint16_t TacticalCursorOutlineState;
