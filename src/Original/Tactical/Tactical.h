// 0800, 1A1F, 2007, 2208, 2D1B: Tactical declarations
#ifndef REVENGE_TACTICAL_TACTICAL_H
#define REVENGE_TACTICAL_TACTICAL_H

#include "revenge/types.h"
#include "revenge/unit.h"

uint8_t *AdvanceFarPointerBy32K(uint8_t *pointer);
void BuildTacticalOverviewMapBuffer(
    const uint8_t *source,
    uint8_t *destination,
    uint16_t width,
    uint16_t height);
void ClearTacticalEffectOverlayMap(void);
void PrepareUnitTypeResources(uint16_t unitIndex);
void SelectTacticalMapCoordinate(uint16_t unitIndex);
void RefreshTacticalCoordinateSelection(
    uint16_t unitIndex,
    uint16_t selectionMode,
    uint16_t savedCursorColumn);
void CycleTacticalEnemyTarget(void);
uint16_t SelectTacticalMoveDestinationInput(
    uint16_t unitIndex,
    uint16_t mode);
uint16_t SelectTacticalMoveDestination(
    uint16_t unitIndex,
    uint16_t selectionMode);
int16_t RunOptionsMenuSelection(int16_t firstRegionIndex);
void RunTacticalUnitCommandMenu(uint16_t unitIndex);
void SelectStrafingArea(void);
void RunTacticalGameOptionsMenu(void);
/* Ours (include/revenge/upgrades.h): the UPGRADES page (TacticalControl.cpp). */
void OpenUpgradesPage(void);
void RunTacticalInteractionLoop(void);
void CalculateUnitDurabilityTotals(RevengeUnitRecord *unitRecord);
void FormatUnitDurabilityPercentage(int16_t unitIndex);
uint16_t IsCurrentLanceLeader(uint16_t unitIndex);
uint16_t ValidatePrimaryLanceLeader(uint16_t unitIndex);
void LoadTacticalIconResource(
    const char *resourcePath,
    uint16_t destinationPage);
uint16_t HasNarrationMessage(
    uint16_t narrationSubject,
    uint16_t messageCategory);
void QueueLinkedUnitStatusNarration(uint16_t unitIndex);
void DrawTacticalEffectSprite(
    uint16_t spriteId,
    int16_t relativeX,
    int16_t relativeY);
void DrawTacticalViewportCell(
    int16_t viewportColumn,
    int16_t viewportRow,
    uint16_t viewportCellIndex);
void DrawTacticalTileSpriteForAdapter(
    const uint8_t *spritePage,
    uint16_t spriteId,
    uint16_t destinationX,
    uint16_t destinationY);
void RefreshTacticalDisplay(uint16_t advanceEffects);
void RecalculateUnitCombatRatings(
    RevengeUnitRecord *unitRecord,
    uint16_t unitIndex,
    uint16_t rebuildWeaponCounts,
    uint16_t advanceRecovery);
uint16_t HasUnitLineOfSight(
    int16_t sourceUnitIndex,
    int16_t targetUnitIndex);
void RunTacticalBattle(uint16_t scenarioCode);
void SelectTacticalTextForegroundColor(void);
void SetTacticalSlotSpriteState(
    uint16_t slotIndex,
    uint16_t requestedState);
void DrawCampaignClockSeparator(void);
void ShowTacticalModalMessage(const char *formattedText);
void QueueRandomMapEdgeEffect(int16_t centerX, int16_t centerY);
void QueueMapEdgeEffect(
    int16_t mapX,
    int16_t mapY,
    uint16_t baseSpriteId,
    uint16_t edgeDirection);
void DrawTacticalCellOutline(
    int16_t leftX,
    int16_t topY,
    uint16_t color);
void UpdatePrimaryUnitStatusRowHighlight(uint16_t rowIndex);
void UpdateSecondaryUnitStatusRowHighlight(uint16_t rowIndex);

/* The battle view: 19 columns by 12 rows of 16-pixel cells. */
inline constexpr int TACTICAL_VIEWPORT_WIDTH_CELLS = 0x0013;
inline constexpr int TACTICAL_VIEWPORT_HEIGHT_CELLS = 0x000C;
inline constexpr int TACTICAL_VIEWPORT_CELL_COUNT = 0x00E4;
inline constexpr int TACTICAL_CELL_PIXEL_SIZE = 0x0010;
inline constexpr int TACTICAL_CURSOR_PIXEL_LEFT = 0x0010;
inline constexpr uint16_t NO_TACTICAL_OVERLAY_SPRITE_ID = 0x0000;
inline constexpr uint16_t TACTICAL_TRACE_DASH_INTERVAL = 0x0006;

/* Where the packed terrain-damage map starts in the battle's runtime
 * allocation. */
/* A map cell's two-bit damage state at its top: off the map, one of the
 * three special destructible terrain types, or damaged as far as it goes. */
inline constexpr uint16_t TERRAIN_DAMAGE_STATE_MAXIMUM = 0x03;
inline constexpr uint16_t PACKED_TERRAIN_DAMAGE_MAP_OFFSET = 0xA000;

/* A unit's order (UnitTacticalOrderMode). Proposed behavioural names from the
 * complete tactical-AI consumer pass: 00 follows only the scripted route, 01
 * attacks without autonomous pursuit, and 02 selects and pursues targets
 * autonomously. */
inline constexpr uint8_t TACTICAL_ORDER_SCRIPTED_ROUTE = 0x00;
inline constexpr uint8_t TACTICAL_ORDER_HOLD_POSITION = 0x01;
inline constexpr uint8_t TACTICAL_ORDER_SEEK_AND_DESTROY = 0x02;

/* No unit: TacticalSelectedUnitIndex, a lance leader, a narration subject. */
inline constexpr uint16_t TACTICAL_UNIT_NONE = 0xFFFF;

/* How long a narration stays up, in ms of battle time (the game clock's
 * ticks while the battle loop runs). The original counted it in battle-loop
 * passes: 25Dh (605) of them, so its length, and whether its prompt still
 * opened, followed the machine's speed. On the reference machine (DOSBox-X,
 * 3000 cycles) a pass takes 33.06 ms on average: 70,580 passes over nine
 * battle stretches of stages 3 to 7, 31 to 40 ms by stretch. 605 passes are
 * 20,000 ms. */
inline constexpr int NARRATION_DELAY_MS = 20000;
/* The narration's sound came on the pass at which five had gone by since
 * the set (the test for 258h): five passes, 165 ms. */
inline constexpr int NARRATION_SOUND_AFTER_MS = 165;
/* One tick of the game clock (GameTimerTicks, 18.2 a second), in whole ms. */
inline constexpr int GAME_CLOCK_TICK_MS = 55;

#endif
