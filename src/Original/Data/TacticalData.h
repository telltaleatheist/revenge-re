// 3A6A: tactical data declarations
#ifndef REVENGE_DATA_TACTICAL_DATA_H
#define REVENGE_DATA_TACTICAL_DATA_H

#include "revenge/battletech.h"
#include "revenge/combat_data.h"
#include "revenge/original/scene_data.h"
#include "revenge/roster.h"
#include "revenge/tactical_data.h"
#include "revenge/types.h"
#include "revenge/unit.h"

extern uint8_t *DamagedTerrainSpriteLookup;
extern uint8_t TacticalEffectOverlayMap[0x00E4];
extern uint32_t GameTimerTicks;
extern uint32_t GameAnimationTicks;
extern int8_t TacticalSlotSpriteState[0x18];
extern uint8_t UnitTypeResourceCode[0x07];
extern uint8_t UnitTacticalOrderMode[0x18];
extern int8_t UnitAttackTarget[0x19];
extern uint16_t UnitSpriteBaseId[0x18];
extern uint16_t UnitSpriteId[0x18];
extern uint16_t TacticalSpriteAnimationPhase;
extern uint16_t TacticalSelectedUnitIndex;
extern uint16_t TacticalViewportMapX;
extern uint16_t TacticalViewportMapY;
extern uint16_t TacticalCursorViewportX;
extern uint16_t TacticalCursorViewportY;
extern uint8_t UnitTargetMapX[0x19];
extern uint8_t UnitTargetMapY[0x19];
extern uint8_t UnitRouteTargetMapX[0x19];
extern uint8_t UnitRouteTargetMapY[0x19];
extern SceneUnitRoute SavedTacticalRoute;
extern uint8_t TacticalInteractionLocked;
extern uint16_t TacticalMenuInputState;
extern const char TacticalPaddingSpace[];
extern const char TacticalStatusSeparator[];
extern const char TacticalStatusPadding[];
extern const char *const UnitStatusArmorLabel;
extern const char *const UnitStatusHeatLabel;
extern const char *const UnitStatusExitLabel;
extern const char *const UnitStatusHeatDetailLabel;
extern const char *const UnitStatusDamageLabel;
extern const char *const UnitStatusRatingLabels;
extern const char *const IntelligenceHeading;
extern const char *const ScannedByActiveProbeText;
extern const char *const DetectedByText;
extern const char *const RadarSensorText;
extern const char *const EnemyAndFriendlyLancesText;
extern const char *const UnknownUnitText;
extern const char *const NoSatelliteUplinkText;
extern const char *const UplinkJammedText;
extern const char *const OutOfScanningText;
extern const char *const RangeText;
extern const char *const ArrowTagEquipmentText;
extern const char *const BapEquipmentText;
extern const char *const AccuracyEquipmentText;
extern const char *const DoubleHeatSinkEquipmentText;
extern const char *const JumpEquipmentSuffix;
extern const char *const CaseEquipmentText;
extern const char *const MoveDestinationPromptSuffix;
extern const char *const FormationHeading;
extern const char *const SatelliteZoomText;
extern const char *const ContinueText;
extern const char *const DoneText;
extern const char NoTacticalTargetText[];
extern const char *const ViewSatelliteMapText;
extern const char *const SelectStrafingAreaPrompt;
extern const char *const SelectArtilleryTargetPrompt;
extern const char *const SelectBarrageStartPrompt;
extern const char *const SelectBarrageEndPrompt;
extern const char *const SelectFireTargetPrompt;
extern const char *TacticalUnitCommandTextTable[0x0E];
extern const char *TacticalSupportCommandTextTable[0x08];
extern const char *TacticalOverburnTextTable[0x02];
extern const char *LanceStatusTextTable[0x08];
extern const char *const DeadText;
extern RevengeMapEdgeEffectRecord
    MapEdgeEffectQueue[MAP_EDGE_EFFECT_RECORD_COUNT];
extern uint16_t MapEdgeEffectCount;
extern uint16_t PrimaryStatusRow;
extern uint16_t SecondaryStatusRow;
extern uint16_t TacticalUiSuppressed;
extern uint16_t PrimaryStatusHighlightColor;
extern uint16_t CachedPrimaryStatusHighlightColor;
extern uint16_t SecondaryStatusHighlightColor;
extern uint16_t CachedSecondaryStatusHighlightColor;
/* DS:B574: 0Dh bytes; the status slots the code and the scenes use are 0-8. */
extern uint8_t TacticalStatusRowInvalidation[0x0D];
extern uint8_t TacticalOverviewMarkerX[0x18];
extern uint16_t TacticalOverviewViewportX;
extern uint8_t TacticalOverviewMarkerY[0x18];
extern uint16_t TacticalOverviewViewportY;
extern uint16_t TacticalOverviewScreenOriginX;

#define TacticalContactMarkerPending \
    TacticalStatusRowInvalidation[0x07]

extern uint16_t DisplayedTacticalStatusRow;
extern uint16_t TacticalStatusBlinkCountdown;
extern uint8_t TacticalStatusBlinkVisible;
extern const int8_t WeaponTraceMajorStepX[0x08];
extern const int8_t WeaponTraceMajorStepY[0x08];
extern const int8_t WeaponTraceMinorStepX[0x08];
extern const int8_t WeaponTraceMinorStepY[0x08];
extern const char TacticalNoUnitStatusText[];
extern const char *const FriendlyUnitStatusHeading;
extern const char EnemyUnitStatusHeading[];
extern const char UnitStatusSeparatorText[];
extern const char UnitHeatStatusPrefix[];
extern const char TacticalNarrationBlankText[];
extern const char TacticalClockBlankText[];
extern const char *const TacticalNarrationLabel;
extern const char *const CombatRatingLightText;
extern const char *const CombatRatingModerateText;
extern const char *const CombatRatingHeavyText;
extern const char *const CombatRatingBrutalText;
extern const char *const CombatRatingDevastatingText;
extern const char *const CombatRatingIncredibleText;
extern const char *const CombatRatingNoneText;
extern const char *CombatRatingDescriptionTable[0x07];
extern RevengeUnitRecord PersistentUnitRoster[0x18];
extern RevengeUnitRecord LiveScenarioUnits[0x18];
extern RevengeUnitRecord BlankUnitRecord;
extern uint8_t ScenarioUnitMapX[0x18];
extern uint8_t ScenarioUnitMapY[0x18];
extern uint8_t ScenarioUnitFacing[0x18];
/* DS:8337-8339: the hour, the minute and the battle's subtick, the byte the
 * battle loop steps; the scenario's saved clock (DS:855F) is the same three. */
extern uint8_t CampaignClockState[0x03];

#define TacticalClockSubtick (CampaignClockState[0x02])

extern uint8_t SavedCampaignClockState[0x03];
extern RevengePilotRosterRecord PilotRoster[REVENGE_PILOT_ROSTER_COUNT];
/* DS:2148, the roster entry past its end that slot FF reads (1A1F:339F,
 * 30F1:0AFC): in the image, four bytes of a table of file names. From the
 * player's copy (src/Support/original_import_pieces.txt). */
extern RevengePilotRosterRecord PilotRosterRecordPastEnd;
extern RevengeUnitLanceAssignment
    LanceAssignments[REVENGE_LANCE_ASSIGNMENT_COUNT];
/* DS:5928: seven rows; the scenarios use rows 0-4 and 6. */
extern uint8_t ScenarioLanceAllegianceTable[0x07][0x03];  /* the rules' */
extern uint16_t AmmoCapacityMultiplierTable[0x0E];     /* the rules' */
extern uint16_t CampaignFlags;
/* CampaignFlags' bits, and SavedCampaignFlags' (the flags the stage began
 * with). The battle bits are set as a battle ends (BattleRuntime.cpp); the
 * milestone bits by the scene's CAMPAIGN_MILESTONE selectors
 * (SceneInterpreter.cpp). 0010h (scenario 37h, unit 1 out) and 8000h stay
 * numbers: see the unknowns. */
/* The battle ended with result 2, which the battle sets when every enemy
 * unit is out or immobile and a scene sets to end the battle; every stage
 * takes it as the way forward. */
inline constexpr uint16_t CAMPAIGN_FLAG_BATTLE_WON = 0x0001;
/* Unit 0 was out at the battle's end, the in-battle menu left the battle, or
 * stage 20's battle wasn't won; every stage takes it as the replay. */
inline constexpr uint16_t CAMPAIGN_FLAG_BATTLE_LOST = 0x0002;
/* Unit 0 out (in scenario 35h, immobile) and the last attacking unit a
 * BattleMech. */
inline constexpr uint16_t CAMPAIGN_FLAG_LOST_TO_BATTLEMECH = 0x0004;
/* Unit 1 out at the battle's end. */
inline constexpr uint16_t CAMPAIGN_FLAG_UNIT_1_LOST = 0x0008;
/* The ejected pilot's unit still deployed and mobile (milestone 3). */
inline constexpr uint16_t CAMPAIGN_FLAG_EJECTED_PILOT_ALIVE = 0x0020;
/* The battle ended at hour 12 or later, or at 07:30 or later in scenarios 36h
 * and 37h. */
inline constexpr uint16_t CAMPAIGN_FLAG_CLOCK_PAST_LIMIT = 0x0040;
/* Milestone 0; stages 4-6, 11 and 12 also set it when unit 8 is out. */
inline constexpr uint16_t CAMPAIGN_FLAG_APC_BRIDGE = 0x0080;
inline constexpr uint16_t CAMPAIGN_FLAG_MILESTONE_UNKNOWN_04 = 0x0100;
/* Scenario 53h's battle clears it when it sets COMMANDER_CRIPPLED. */
inline constexpr uint16_t CAMPAIGN_FLAG_MILESTONE_UNKNOWN_05 = 0x0200;
inline constexpr uint16_t CAMPAIGN_FLAG_COMMANDER_CRIPPLED = 0x0400;
inline constexpr uint16_t CAMPAIGN_FLAG_SRM_SHIPMENT = 0x0800;
inline constexpr uint16_t CAMPAIGN_FLAG_LRM_SHIPMENT = 0x1000;
/* The stage began with UNIT_1_LOST in SavedCampaignFlags. */
inline constexpr uint16_t CAMPAIGN_FLAG_UNIT_1_LOST_BEFORE = 0x2000;
inline constexpr uint16_t CAMPAIGN_FLAG_MECH_SHOP_REACHED = 0x4000;
extern uint16_t CampaignStageIndex;
extern uint16_t ScenarioUnitZeroAvailable;
extern int16_t ScenarioUnitFifteenState;
extern uint16_t ScenarioUnitFourteenEnabled;
extern uint16_t EjectedPilotRosterState;
extern uint16_t ScenarioUnitThirteenEnabled;
extern uint8_t StoryUnitDeployedFlag;
extern uint8_t SelectedSaveSlotIndex;
extern uint8_t SaveLoadMenuDefaultSelection;
extern uint8_t ScenarioRosterPrepared;
extern int8_t PendingCampaignStageDelta;
extern uint8_t PendingCampaignPhaseDelta;
extern int16_t ScenarioVariantIndex;
extern uint8_t CampaignPhase;
extern uint8_t ReplayCampaignStateRequested;
extern uint8_t TrainingSequenceFlag;
extern uint16_t DeferredStage16Story;
extern uint16_t SavedCampaignFlags;
extern uint8_t AlternateLanceSelection;
extern uint8_t SuppressPostStageMenu;
extern uint8_t CampaignRestartState;
extern uint16_t ModalSavedTextWindow;
extern uint16_t ModalBackingX;
extern uint16_t ModalBackingY;
extern uint16_t ModalBackingWidth;
extern uint16_t ModalBackingHeight;
extern uint8_t ActivePilotListWindowHeight;
extern uint8_t CommanderListWindowHeight;
extern uint8_t FallenPilotListWindowHeight;

/* DS:1B8C-1B8E: the random pilot pool's count, in the campaign's three
 * copies of its state, as the pool has them (InitialExtendedCampaignState,
 * ForwardExtendedCampaignState and ReplayExtendedCampaignState). The
 * replay copy is the live count. */
typedef struct RandomPilotPoolCountCopies
{
    uint8_t initial; /* DS:1B8C */
    uint8_t forward; /* DS:1B8D */
    uint8_t replay; /* DS:1B8E */
} RandomPilotPoolCountCopies;
static_assert(sizeof(RandomPilotPoolCountCopies) == 0x03,
    "DS:1B8C-1B8E holds the three copies of the count.");

extern RandomPilotPoolCountCopies RandomPilotPoolCounts;
extern uint16_t SaveTransferState;
extern uint16_t SavedSaveTransferState;
extern uint16_t CampaignScenarioState;
extern uint8_t ReplayScenarioRequested;
extern uint8_t CampaignMenuRequested;
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
extern uint8_t UnitEditorAllegiance;
extern uint16_t UnitEditorScenarioGroupCount;
extern uint16_t UnitEditorScenarioCardCount;
extern uint16_t UnitEditorPilotCardCount;
extern uint16_t UnitEditorLanceGroupCount;

/* DS:1D42 is LanceAssignments[0x0C], not a second object. */
#define UnitEditorExitButtonLeft \
    (LanceAssignments[0x0C].left)
#define UnitEditorExitButtonTop \
    (LanceAssignments[0x0C].top)
#define UnitEditorExitButtonState \
    (LanceAssignments[0x0C].style)

extern uint16_t TextOutputMode;
extern uint16_t SelectedUnitEquipmentLocation;
extern RevengeUnitRecord EstimatedUnitRecord;
/* DS:B011 and DS:B01D are the two scratch records immediately after the
 * 18 tactical-unit rating records. */
extern RevengeTacticalCombatRatings UnitCombatRatings[0x1A];

#define DisplayedUnitCombatRatings UnitCombatRatings[0x18]
#define ComparedUnitCombatRatings  UnitCombatRatings[0x19]

extern uint8_t UnitLanceFormationState[0x18];
extern uint8_t TacticalScenarioLanceEnabled[0x03];
extern uint8_t SensorDamageProximityRangeTable[0x03];  /* the rules' */
extern uint8_t LanceFormationProfile[0x06];
extern uint8_t UnitEnemyEngagementActive[0x18];
extern uint8_t UnitMovementHeatExponent[0x18];
extern uint8_t UnitOverburnOrCloseAssaultEnabled[0x18];
extern uint8_t UnitFormationMode[0x18];
extern uint16_t UnitConsecutiveAttackTicks[0x18];
extern uint16_t FirstLanceOrdersEnabled;
extern uint8_t UnitWasAttackedThisTick[0x18];
extern uint8_t GroundTargetWasAttacked;
extern uint8_t UnitJumpingState[0x18];
extern uint8_t UnitJumpMovementRemaining[0x18];
extern uint8_t UnitDestructionMarkers[0x18][0x03];
extern uint16_t ScenarioFileSuffix;
extern uint8_t CampaignUpgradeAwards[0x04];
extern uint8_t UnitEnemyInProximityRange[0x18];
extern uint8_t UnitShutdownState[0x18];
extern uint8_t UnitEnemyVisible[0x18];
extern RevengeUnitSlot UnitTacticalTarget[0x19];
extern uint16_t EnemyCycleUnitIndex;
extern uint8_t UnitEnemyContactEstablished[0x18];
extern uint8_t UnitWithdrawalState[0x18];

/* DS:884B, DS:884E and DS:8853: the withdrawal states of the roster's units
 * 0Ch, 0Fh and 14h, which a later scenario's rosters test for withdrawal. */
#define CampaignUnitThirteenAvailabilityFlag (UnitWithdrawalState[0x0C])
#define CampaignUnitFifteenOverrideFlag      (UnitWithdrawalState[0x0F])
#define CampaignUnitFourteenAvailabilityFlag (UnitWithdrawalState[0x14])

extern RevengeUnitSlot UnitLastNarratedTarget[0x18];
extern uint8_t UnitPreferredEngagementMode[0x18];
extern uint8_t UnitTerrainType[0x18];
/* RecalculateUnitCombatRatings uses slot 18h as the status-panel scratch
 * record in addition to the 18h live tactical unit slots. */
extern uint8_t UnitBelowHalfDurability[TACTICAL_UNIT_COUNT + 0x01];
extern uint8_t UnitCachedWeaponCounts[(TACTICAL_UNIT_COUNT + 0x01) * WEAPON_DEFINITION_COUNT];
extern uint8_t UnitWeaponRecoveryCounters[(TACTICAL_UNIT_COUNT + 0x01) * WEAPON_DEFINITION_COUNT];
extern uint8_t UnitHeatSinkCount[TACTICAL_UNIT_COUNT + 0x01];
extern uint32_t UnitWeaponAvailabilityMask[TACTICAL_UNIT_COUNT + 0x01];
extern uint16_t WeaponCountScratch[0x17];
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
extern uint8_t CampaignUpgradeMarkerX[0x04];
extern uint8_t CampaignUpgradeMarkerY[0x04];
extern uint16_t FriendlyUnitDestroyed;
extern uint8_t ScenarioDestructionCompletionState;
extern uint8_t ScenarioDestructionCompletionMarker;
extern uint16_t ProtectUnitZero;
extern uint16_t ImpassableTerrainTypeMinimum;
extern int8_t TerrainToHitModifier[0x100];
/* What an experience past those four reads: the original's words after the
 * table, through DS:3EF5 (src/Support/original_import_pieces.txt). */
extern uint16_t UnitActionMaskByExperiencePastEnd[0xFC];
/* What a heat-limit index past WeaponAIHeatLimitByExperience's five reads:
 * the original's bytes after the table, through DS:04E0
 * (src/Support/original_import_pieces.txt). */
extern uint8_t WeaponAIHeatLimitByExperiencePastEnd[0xFC];
extern RevengeTerrainImpactAnimationRecord
    TerrainImpactAnimationRecords[TERRAIN_IMPACT_ANIMATION_RECORD_COUNT];
extern uint16_t NarrationMessageCategory;
extern uint16_t PendingNarrationDelay;
extern uint8_t DamagedWeaponRangeFlags[0x03];
extern int16_t CurrentUnitDurabilityTotal;
extern int16_t MaximumUnitDurabilityTotal;
extern char SharedNumericTextBuffer[0x20];
extern const char PercentageSuffixText[];
extern uint16_t TacticalUpdateDelayTicks;
extern uint16_t TacticalKeyRepeatDelayTicks;
extern uint16_t TacticalUpdateDelayReloadTicks;
extern uint16_t TacticalGameSpeedIndex;
extern uint16_t TacticalBattleResult;
extern uint16_t TacticalClockDivider;
extern uint16_t TacticalImpactQueueCount;
extern RevengeTacticalImpactRecord
    TacticalImpactQueue[TACTICAL_IMPACT_RECORD_COUNT];
extern const int16_t TacticalCursorSearchOffsetX[0x09];
extern const int16_t TacticalCursorSearchOffsetY[0x09];
extern const int8_t TacticalMouseEdgeInputByMask[0x10];
extern uint8_t SceneNarrationEventFlags[0x06];
/* DS:176A: 30h words, one for each heat-sink count a unit's 2Fh equipment
 * slots allow. */
extern uint16_t HeatDissipationBySinkCountQ8_8[0x30];  /* the rules' */
extern uint16_t TerrainDamageQueueWordCount;
extern uint16_t TacticalProjectileCount;
extern RevengeTacticalProjectileRecord
    TacticalProjectileRecords[TACTICAL_PROJECTILE_RECORD_COUNT];
extern uint16_t TacticalOverviewDisplayMode;
extern uint16_t LrmShipmentLoaded;
extern uint16_t SrmShipmentLoaded;
extern uint16_t TacticalOverviewMapMode;
extern uint16_t NarrationSourceUnit;
extern uint16_t LastAttackingUnitIndex;
extern uint16_t PendingMouseButtonEvent;
extern int16_t MouseCursorTestX;
extern int16_t MouseCursorTestY;
extern uint16_t MouseCursorUpdateLock;
extern int16_t MouseCursorRequestedX;
extern uint16_t MouseCursorHideDepth;
extern uint16_t MouseDriverDoubleX;
extern uint16_t MouseDriverInstalled;
extern uint16_t MouseButtonState;
extern uint16_t MouseY;
extern uint16_t MouseInputDisabled;
extern int16_t MouseCursorMinX;
extern int16_t MouseCursorMaxX;
extern int16_t MouseCursorMinY;
extern int16_t MouseCursorMaxY;
extern int16_t MouseCursorHotspotX;
extern int16_t MouseCursorHotspotY;
extern int16_t MouseCursorDisplayedX;
extern int16_t MouseCursorDisplayedY;
extern uint8_t *MouseCursorBackground;
extern const uint8_t *MouseCursorBitmap;
extern int16_t MouseCursorErasedX;
extern int16_t MouseCursorErasedY;
extern int16_t MouseCursorRestoreTileLeft;
extern int16_t MouseCursorRestoreTileTop;
extern int16_t MouseCursorRestoreTileRight;
extern int16_t MouseCursorRestoreTileBottom;
extern int16_t MouseCursorDrawnX;
extern int16_t MouseCursorDrawnY;
extern int16_t MouseCursorCaptureTileLeft;
extern int16_t MouseCursorCaptureTileTop;
extern int16_t MouseCursorCaptureTileRight;
extern int16_t MouseCursorCaptureTileBottom;
extern int16_t MouseCursorDrawLeft;
extern int16_t MouseCursorDrawTop;
extern uint8_t *ActiveRenderTarget;
extern uint16_t SpriteClipLeftTileMirror;
extern uint16_t SpriteClipRightTileMirror;
extern uint16_t SpriteClipTopPixelMirror;
extern uint16_t SpriteClipBottomPixelMirror;
extern uint16_t TacticalSpriteClipLeftTile;
extern uint16_t TacticalSpriteClipRightTile;
extern uint16_t TacticalSpriteClipTopPixel;
extern uint16_t TacticalSpriteClipBottomPixel;
extern uint16_t RequestedMusicEnabled;
extern uint16_t CachedMusicResourceIndex;
extern uint16_t SpriteDrawBusy;
extern uint16_t PlanarBackingBufferExtentLow;
extern uint16_t PlanarBackingBufferExtentHigh;
extern uint8_t SceneTransitionAbortFlag;
extern uint16_t PresentationDismissTimeout;
extern uint16_t PresentationCallbackOffset;
extern uint16_t PresentationCallbackSegment;
/* The far callback's values: the offsets in segment 0D73 of the procedures it
 * names (src/Support/presentation_callback_bridge.cpp calls them). */
inline constexpr uint16_t PRESENTATION_CALLBACK_SEGMENT = 0x0D73;
inline constexpr uint16_t PRESENTATION_CALLBACK_ACTIVE_PILOT_LIST = 0x214B;
inline constexpr uint16_t PRESENTATION_CALLBACK_SURVIVING_COMMANDER_LIST = 0x223D;
inline constexpr uint16_t PRESENTATION_CALLBACK_FALLEN_PILOT_LIST = 0x2361;
inline constexpr uint16_t PRESENTATION_CALLBACK_CAMPAIGN_PATH_SELECTION = 0x249E;
inline constexpr uint16_t PRESENTATION_CALLBACK_SCENARIO_VARIANT_SELECTION = 0x24B3;
inline constexpr uint16_t PRESENTATION_CALLBACK_ALTERNATE_SCENARIO_VARIANT_SELECTION = 0x24C9;
extern uint16_t TacticalOverviewActive;
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
extern uint16_t TacticalOverviewViewportDirty;
extern uint16_t *TacticalCellTerrainMap;
extern uint16_t *TacticalCellSpriteMap;
extern uint8_t ScenarioUnitNeedsUpdate[0x18];
extern uint8_t TacticalStateInitialized;
extern uint16_t SuppressCampaignClock;
extern uint16_t TacticalCursorOutlineEnabled;
extern uint16_t TacticalCursorOutlineState;

#endif
