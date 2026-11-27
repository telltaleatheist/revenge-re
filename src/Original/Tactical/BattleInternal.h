// 1A1F: battle, scene and campaign clock declarations
#ifndef REVENGE_TACTICAL_BATTLE_INTERNAL_H
#define REVENGE_TACTICAL_BATTLE_INTERNAL_H

// REVENGE_1A1F_SCENARIO.c
// Tactical battle orchestration, SCENE interpretation, input, and campaign
// clock procedures reconstructed from REVENGE.EXE segment 1A1F.

#include "Data/StartupData.h"
#include "Data/TacticalData.h"
#include "revenge/input.h"
#include "revenge/original/scene_data.h"
#include "revenge/debug.h"
#include "revenge/video.h"

uint16_t ReadTerrainDamageState(int16_t mapX, int16_t mapY);
void BeginTerrainDamageAtCell(
    uint16_t mapX, uint16_t mapY, uint16_t terrainType);

int16_t ResolveUnitAtTacticalCursor(void);
void MoveTacticalCursorByInput(int16_t inputCode);
uint16_t DetermineOctant(int16_t deltaY, int16_t deltaX);
int16_t InterpolateLineCoordinate(
    int16_t clipCoordinate,
    int16_t targetCoordinate,
    int16_t sourceValue,
    int16_t targetValue,
    int16_t sourceCoordinate);
uint16_t DetermineRelativeAttackDirection(
    int16_t sourceX,
    int16_t sourceY,
    int16_t targetX,
    int16_t targetY);
void OffsetWeaponTraceEndpoint(
    int16_t *sourceX, int16_t *sourceY,
    int16_t *targetX, int16_t *targetY);
void AdvanceScenarioAreaEffects(void);
int16_t MapUnitToTacticalStatusSlot(uint16_t unitIndex);
void KeepTacticalCursorOnSelectedUnit(void);
void RefreshTacticalStatusSlotStates(void);
void AdvanceTacticalCampaignClock(void);
void BuildScenarioTerrainDamageCapabilityMap(uint16_t terrainEffectMode);
void ApplyScenarioMissileUpgradeAvailability(void);
int16_t TranslateTacticalMouseInput(int16_t inputAction);
void DrawClippedWeaponTraceLine(
    int16_t startX, int16_t startY,
    int16_t endX, int16_t endY,
    uint16_t color, uint16_t dashedTrace);
void DrawTileAlignedText(
    const char *text,
    uint16_t tileX,
    uint16_t tileY,
    uint8_t foregroundColor,
    uint8_t backgroundColor);
void LoadTacticalMap(uint16_t mapNumber);
void LoadTacticalIconSetResources(uint16_t iconSetNumber);
void ExecuteSceneInstructions(void);
uint16_t AdvanceTerrainDamageState(
    int16_t mapX, int16_t mapY, uint16_t maximumDamageState);
void UpdateUnitSpriteForFacing(uint16_t unitIndex, uint16_t facing);
void DrawTacticalOverviewMap(void);
uint16_t RunGameOptionsMenu(uint16_t embeddedMode);
void UpdateUnitHeatAccumulators(void);
void ApplyAreaTerrainAndUnitDamage(
    int16_t mapX, int16_t mapY, uint16_t visualEffectType, int16_t weaponIndex);
void OpenModalTextWindow(uint16_t windowIndex, uint16_t clearAndFrame);
void RestoreModalTextWindow(void);
uint16_t ConfirmExitToDos(void);
void DisableMusicPlayback(void);
void EnableMusicPlayback(void);

#define TacticalResourceSlotState SceneNarrationEventFlags
#define TacticalOverviewZoomed    TacticalOverviewViewportDirty
#define TacticalOverviewSavedMode TacticalOverviewDisplayMode
#define NarrationReady            FriendlyUnitDestroyed

#define TACTICAL_RUNTIME_ALLOCATION_SIZE 0xCB90
#define TACTICAL_VIEWPORT_WIDTH_CELLS    0x0013
#define TACTICAL_VIEWPORT_HEIGHT_CELLS   0x000C
#define CAMPAIGN_MINUTES_PER_HOUR        0x003C
#define CAMPAIGN_HOURS_PER_DAY           0x0018
#define TACTICAL_OVERVIEW_MAP_OFFSET     0x4E20
#define PACKED_TERRAIN_DAMAGE_MAP_OFFSET 0xA000
#define TACTICAL_CELL_SPRITE_MAP_OFFSET  0xC800
#define TACTICAL_CELL_TERRAIN_MAP_OFFSET 0xC9C8
#define TACTICAL_BACKING_BUFFER(index) \
    SharedGraphicsBackingBuffers[(index)]
#define SET_TACTICAL_BACKING_BUFFER(index, buffer) \
    (SharedGraphicsBackingBuffers[(index)] = (buffer))

// Proposed behavioral names from the complete tactical-AI consumer pass.
// 00 follows only the scripted route, 01 attacks without autonomous pursuit,
// and 02 selects and pursues targets autonomously.
#define TACTICAL_ORDER_SCRIPTED_ROUTE   0x00
#define TACTICAL_ORDER_HOLD_POSITION    0x01
#define TACTICAL_ORDER_SEEK_AND_DESTROY 0x02

#define CAMPAIGN_MILESTONE_APC_BRIDGE         0x00
#define CAMPAIGN_MILESTONE_SRM_SHIPMENT       0x01
#define CAMPAIGN_MILESTONE_LRM_SHIPMENT       0x02
#define CAMPAIGN_MILESTONE_PILOT_EJECTED      0x03
#define CAMPAIGN_MILESTONE_UNKNOWN_04         0x04
#define CAMPAIGN_MILESTONE_UNKNOWN_05         0x05
#define CAMPAIGN_MILESTONE_COMMANDER_CRIPPLED 0x06
#define CAMPAIGN_MILESTONE_MECH_SHOP_REACHED  0x07

#define UNIT_UPGRADE_MISSILE_SYSTEM        0x00
#define UNIT_UPGRADE_BEAGLE_ACTIVE_PROBE   0x01
#define UNIT_UPGRADE_INDIRECT_FIRE_SPOTTER 0x02
#define UNIT_UPGRADE_CASE                  0x03
#define UNIT_UPGRADE_CAMPAIGN_AWARD        0x04

void AssignPilotRosterEntryToUnit(
    uint16_t unitIndex,
    uint16_t rosterSlot);
void AdvanceUnitSceneRoute(uint16_t unitIndex);
void SwapUnitRouteWaypoint(uint16_t unitIndex, uint16_t waypointIndex);
void InstallUnitEquipmentInFirstFreeSlot(
    uint16_t unitIndex,
    uint8_t equipmentId);
int16_t FindUnitWeaponByOrdinal(
    RevengeUnitRecord *unitRecord,
    uint16_t weaponOccurrence,
    uint16_t removeWeapon);
uint16_t FindOperationalLanceLeader(uint16_t unitIndex);
void RenderFormattedText(const char *formattedText);
uint16_t WaitForDismissInput(void);
uint16_t ServiceMusicAndCheckKeyboardInput(void);
void DrawCampaignClock(void);
void RefreshAllUnitCombatState(uint16_t rebuildWeaponCounts);
int16_t UpdateUnitEnemyVisibility(uint16_t unitIndex);
int16_t UpdateUnitEnemyProximityState(uint16_t unitIndex);
void RebuildLanceFormationAfterLeaderLoss(uint16_t lanceBaseUnitIndex);
uint16_t ExecuteUnitWeaponAI(uint16_t unitIndex);
uint8_t UpdateUnitHeatShutdown(uint16_t unitIndex);
void RedrawTacticalStatusPanel(void);

// Assembly-verified tactical icon resource helpers.
void RebuildTacticalViewport(void);
void UpdateTacticalFrame(uint16_t inputAction, uint16_t advanceUnits);
void InitializeTacticalUnitRuntime(void);
void LoadTacticalScenarioResources(uint8_t scenarioCode);
void RedrawTacticalViewport(void);
void DrawTacticalCursorOutline(uint16_t color);
void DrawSelectedUnitStatusPanel(void);
void AdvanceUnitTacticalMovement(uint16_t unitIndex);
void ScrollTacticalViewportToCursor(void);
void HandleTacticalInput(uint16_t inputAction);
uint16_t HasActiveLanceFollowers(uint16_t unitIndex);
void HighlightLanceFollowers(uint16_t leaderUnitIndex);

void ShowPendingNarrationPrompt(void);
void ExitTacticalOverview(void);
void FocusTacticalViewOnUnit(uint16_t unitIndex);
uint16_t SelectNextUnitAtCursor(void);
uint16_t SelectPreviousUnitAtCursor(void);
uint16_t FindUnitAtTacticalCursor(void);
void OpenTacticalUnitControlPanel(uint16_t unitIndex);
uint16_t IsTacticalFunctionKey(int16_t inputAction);
void RenderPendingNarration(void);
void CreateRandomAreaImpact(
    int16_t centerX,
    int16_t centerY,
    uint16_t chanceDenominator);

#define TACTICAL_UNIT_NONE                   0xFFFF
#define TACTICAL_ENEMY_FIRST_UNIT            0x000C
#define TACTICAL_ENEMY_SPRITE_LIMIT          0x00DC
#define TACTICAL_ENEMY_SPRITE_EXCLUDED_FIRST 0x00C0
#define TACTICAL_ENEMY_SPRITE_EXCLUDED_END   0x00D0
#define NARRATION_READY_OFFSET               0xB5D2
#define TACTICAL_OVERVIEW_ZOOMED_OFFSET      0x8335
#define TACTICAL_OVERVIEW_SAVED_UNIT_OFFSET  0x8102

#define PERSISTENT_UNIT_RECORD(unitIndex) \
    (&PersistentUnitRoster[(unitIndex)])
/* The next byte of the loaded scene's instructions. The cursor stays inside
 * the scene buffer (DS:71A8, SCENE_FILE_BUFFER_SIZE bytes); past it is a
 * fault. */
static inline uint8_t NextSceneByte(void)
{
    uint8_t *cursor = SceneInstructionCursor++;

    return LoadedSceneFile.raw[REVENGE_INDEX(LoadedSceneFile.raw,
        (size_t) (cursor - LoadedSceneFile.raw))];
}
#define NEXT_SCENE_BYTE() NextSceneByte()
RevengeUnitRecord *GetPersistentUnitRecord(uint16_t unitIndex);
#define GetUnitRouteRecord(unitIndex) (&SceneUnitRoutes[(unitIndex)])
#define ReadSceneByte()               NEXT_SCENE_BYTE()
/* Low byte first, as the original reads them. Two increments of the cursor
 * in one expression would leave their order to the compiler. */
static inline uint16_t ReadSceneWordLowFirst(void)
{
    uint16_t low = NEXT_SCENE_BYTE();
    uint16_t high = NEXT_SCENE_BYTE();

    return (uint16_t) (low | (uint16_t) (high << 0x08));
}
#define ReadSceneWord() ReadSceneWordLowFirst()
/* The target is taken before the cursor moves: an operand read through the
 * cursor (SCENE_JUMP's) comes from where the cursor is (1A1F:1C48). The
 * original sets DS:(71A8h + base + target); a target past the buffer faults
 * at the next read. */
#define JumpToSceneOffset(sceneOffset) \
    do \
    { \
        uint16_t jumpTarget = (uint16_t) (sceneOffset); \
        SceneInstructionCursor = LoadedSceneFile.raw + SceneInstructionBaseOffset + jumpTarget; \
    } while (0)
#define BranchOrSkipSceneWord(takeBranch) \
    do \
    { \
        uint16_t branchOffset = ReadSceneWord(); \
        if ((takeBranch) != 0x00) \
            JumpToSceneOffset(branchOffset); \
    } while (0)

#endif
