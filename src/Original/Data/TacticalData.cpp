// 3A6A: tactical data
// TacticalData.cpp
// Typed objects recovered from REVENGE.EXE DS segment 3A6A.

#include "Data/TacticalData.h"
#include "revenge/audio.h"
#include "revenge/video.h"
#include "revenge/story_text.h"

uint8_t *DamagedTerrainSpriteLookup;
uint8_t TacticalEffectOverlayMap[0x00E4];
uint32_t GameTimerTicks;
uint32_t GameAnimationTicks;
int8_t TacticalSlotSpriteState[0x18];
uint8_t UnitTypeResourceCode[0x07];
uint8_t UnitTacticalOrderMode[0x18];
/* DS:88F3. 2208:060D and 0857 also index entry 18h, the map-cell selection's,
 * which is the free byte DS:890B before the next variable. */
int8_t UnitAttackTarget[0x19];
uint16_t UnitSpriteBaseId[0x18];
uint16_t UnitSpriteId[0x18];
uint16_t TacticalSpriteAnimationPhase;
/* DS:03DC starts at FFFFh: no unit selected. */
uint16_t TacticalSelectedUnitIndex = 0xFFFF;
uint16_t TacticalViewportMapX;
uint16_t TacticalViewportMapY;
uint16_t TacticalCursorViewportX;
uint16_t TacticalCursorViewportY;
uint8_t UnitTargetMapX[0x19];
uint8_t UnitTargetMapY[0x19];
uint8_t UnitRouteTargetMapX[0x19];
uint8_t UnitRouteTargetMapY[0x19];
SceneUnitRoute SavedTacticalRoute;
uint8_t TacticalInteractionLocked;
uint16_t TacticalMenuInputState;
const char TacticalPaddingSpace[] = " ";
const char TacticalStatusSeparator[] = " ";
const char TacticalStatusPadding[] = " "; /* DS:30C2 */
const char *const UnitStatusArmorLabel = STORY_TEXT(UNIT_STATUS_ARMOR_LABEL);
const char *const UnitStatusHeatLabel = STORY_TEXT(UNIT_STATUS_HEAT_LABEL);
const char *const UnitStatusExitLabel = STORY_TEXT(UNIT_STATUS_EXIT_LABEL);
const char *const UnitStatusHeatDetailLabel = STORY_TEXT(UNIT_STATUS_HEAT_DETAIL_LABEL);
const char *const UnitStatusDamageLabel = STORY_TEXT(UNIT_STATUS_DAMAGE_LABEL);
const char *const UnitStatusRatingLabels =
    STORY_TEXT(UNIT_STATUS_RATING_LABELS);
const char *const IntelligenceHeading = STORY_TEXT(INTELLIGENCE_HEADING);
const char *const ScannedByActiveProbeText = STORY_TEXT(SCANNED_BY_ACTIVE_PROBE);
const char *const DetectedByText = STORY_TEXT(DETECTED_BY);
const char *const RadarSensorText = STORY_TEXT(RADAR_SENSOR);
const char *const EnemyAndFriendlyLancesText =
    STORY_TEXT(ENEMY_AND_FRIENDLY_LANCES);
const char *const UnknownUnitText = STORY_TEXT(UNKNOWN_UNIT);
const char *const NoSatelliteUplinkText = STORY_TEXT(NO_SATELLITE_UPLINK);
const char *const UplinkJammedText =
    STORY_TEXT(UPLINK_JAMMED);
const char *const OutOfScanningText = STORY_TEXT(OUT_OF_SCANNING);
const char *const RangeText = STORY_TEXT(RANGE_TEXT);
const char *const ArrowTagEquipmentText = STORY_TEXT(ARROW_TAG_EQUIPMENT);
/* DS:45F8, the string UnitStatusEquipmentSeparator names too. */
const char *const BapEquipmentText = STORY_TEXT(UNIT_STATUS_EQUIPMENT_SEPARATOR);
const char *const AccuracyEquipmentText = STORY_TEXT(ACCURACY_EQUIPMENT);
const char *const DoubleHeatSinkEquipmentText = STORY_TEXT(DOUBLE_HEAT_SINK_EQUIPMENT);
const char *const JumpEquipmentSuffix = STORY_TEXT(JUMP_EQUIPMENT_SUFFIX); /* DS:461D */
const char *const CaseEquipmentText = STORY_TEXT(CASE_EQUIPMENT);
const char *const MoveDestinationPromptSuffix = STORY_TEXT(MOVE_DESTINATION_PROMPT_SUFFIX);
const char *const FormationHeading = STORY_TEXT(FORMATION_HEADING);
const char *const SatelliteZoomText = STORY_TEXT(SATELLITE_ZOOM);
const char *const ContinueText = STORY_TEXT(CONTINUE_TEXT);
const char *const DoneText = STORY_TEXT(GAME_OPTION_LABEL_6);
const char NoTacticalTargetText[] = "?";
const char *const ViewSatelliteMapText = STORY_TEXT(VIEW_SATELLITE_MAP);
const char *const SelectStrafingAreaPrompt = STORY_TEXT(SELECT_STRAFING_AREA_PROMPT);
const char *const SelectArtilleryTargetPrompt = STORY_TEXT(SELECT_ARTILLERY_TARGET_PROMPT);
const char *const SelectBarrageStartPrompt =
    STORY_TEXT(SELECT_BARRAGE_START_PROMPT);
const char *const SelectBarrageEndPrompt = STORY_TEXT(SELECT_BARRAGE_END_PROMPT);
const char *const SelectFireTargetPrompt = STORY_TEXT(SELECT_FIRE_TARGET_PROMPT);
const char *TacticalUnitCommandTextTable[0x0E] = {
    STORY_TEXT(TACTICAL_UNIT_COMMAND_0), /* TacticalCommandsText */
    STORY_TEXT(TACTICAL_UNIT_COMMAND_1), /* TacticalLanceCommandsText */
    STORY_TEXT(TACTICAL_UNIT_COMMAND_2), /* TacticalTargetText */
    STORY_TEXT(TACTICAL_UNIT_COMMAND_3), /* TacticalCeaseFireAtText */
    STORY_TEXT(TACTICAL_UNIT_COMMAND_4), /* TacticalStopText */
    STORY_TEXT(TACTICAL_UNIT_COMMAND_5), /* TacticalWalkingSpeedText */
    STORY_TEXT(TACTICAL_UNIT_COMMAND_6), /* TacticalRunningSpeedText */
    STORY_TEXT(TACTICAL_UNIT_COMMAND_7), /* TacticalMoveText */
    STORY_TEXT(TACTICAL_UNIT_COMMAND_8), /* TacticalStoppedText */
    STORY_TEXT(TACTICAL_UNIT_COMMAND_9), /* TacticalCruisingSpeedText */
    STORY_TEXT(TACTICAL_UNIT_COMMAND_10), /* TacticalFlankSpeedText */
    STORY_TEXT(TACTICAL_UNIT_COMMAND_11), /* TacticalFireAtNoOneText */
    STORY_TEXT(TACTICAL_UNIT_COMMAND_12), /* TacticalFireDefensivelyText */
    STORY_TEXT(TACTICAL_UNIT_COMMAND_13) /* TacticalFireAtWillText */
};
const char *TacticalSupportCommandTextTable[0x08] = {
    STORY_TEXT(TACTICAL_SUPPORT_COMMAND_0), /* ShellCoordinateText */
    STORY_TEXT(TACTICAL_SUPPORT_COMMAND_1), /* CallOffShellingText */
    STORY_TEXT(TACTICAL_SUPPORT_COMMAND_2), /* RollingBarrageText */
    STORY_TEXT(TACTICAL_SUPPORT_COMMAND_3), /* CallOffBarrageText */
    STORY_TEXT(TACTICAL_SUPPORT_COMMAND_4), /* CallStrafingText */
    STORY_TEXT(TACTICAL_SUPPORT_COMMAND_5), /* CallOffStrafingText */
    STORY_TEXT(TACTICAL_SUPPORT_COMMAND_6), /* CloseAssaultText */
    STORY_TEXT(TACTICAL_SUPPORT_COMMAND_7) /* CeaseAssaultText */
};
const char *TacticalOverburnTextTable[0x02] = { STORY_TEXT(TACTICAL_OVERBURN_0) /* OverburnOffText */, STORY_TEXT(TACTICAL_OVERBURN_1) /* OverburnOnText */ };
const char *LanceStatusTextTable[0x08] = {
    STORY_TEXT(LANCE_STATUS_0), /* LanceCommandText */
    STORY_TEXT(LANCE_STATUS_1), /* LanceFireText */
    STORY_TEXT(LANCE_STATUS_2), /* LanceAssaultText */
    STORY_TEXT(LANCE_STATUS_3), /* LanceReconText */
    STORY_TEXT(LANCE_STATUS_4), /* LanceLightText */
    STORY_TEXT(LANCE_STATUS_5), /* LanceMediumText */
    STORY_TEXT(LANCE_STATUS_6), /* LanceHeavyText */
    STORY_TEXT(LANCE_STATUS_7) /* LanceAssaultWeightText */
};
/* DS:42D6 (2208:2181), the end of a longer string. */
const char *const DeadText = STORY_TEXT(DEAD_TEXT);
RevengeMapEdgeEffectRecord MapEdgeEffectQueue[MAP_EDGE_EFFECT_RECORD_COUNT];
uint16_t MapEdgeEffectCount;
uint16_t PrimaryStatusRow;
uint16_t SecondaryStatusRow;
uint16_t TacticalUiSuppressed;
uint16_t PrimaryStatusHighlightColor;
uint16_t CachedPrimaryStatusHighlightColor;
uint16_t SecondaryStatusHighlightColor;
uint16_t CachedSecondaryStatusHighlightColor;
uint8_t TacticalStatusRowInvalidation[0x0D];
uint8_t TacticalOverviewMarkerX[0x18];
uint16_t TacticalOverviewViewportX;
uint8_t TacticalOverviewMarkerY[0x18];
uint16_t TacticalOverviewViewportY;
uint16_t TacticalOverviewScreenOriginX;
/* Filled at start from data/start/game.json (revenge/rules.h). */
uint16_t DisplayedTacticalStatusRow;
uint16_t TacticalStatusBlinkCountdown;
uint8_t TacticalStatusBlinkVisible;
const int8_t WeaponTraceMajorStepX[0x08] = { 0x00, 0x01, 0x01, 0x00, 0x00, -0x01, -0x01, 0x00 };
const int8_t WeaponTraceMajorStepY[0x08] = { -0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, -0x01 };
const int8_t WeaponTraceMinorStepX[0x08] = { 0x01, 0x00, 0x00, 0x01, -0x01, 0x00, 0x00, -0x01 };
const int8_t WeaponTraceMinorStepY[0x08] = { 0x00, -0x01, 0x01, 0x00, 0x00, 0x01, -0x01, 0x00 };
const char TacticalNoUnitStatusText[] =
    "                           ";
const char *const FriendlyUnitStatusHeading = STORY_TEXT(FRIENDLY_UNIT_STATUS_HEADING); /* DS:4620 */
const char EnemyUnitStatusHeading[] = "   ";
const char UnitStatusSeparatorText[] = " ";
const char UnitHeatStatusPrefix[] = "/";
const char TacticalNarrationBlankText[] = "   ";
const char TacticalClockBlankText[] = "   ";
const char *const TacticalNarrationLabel = STORY_TEXT(TACTICAL_NARRATION_LABEL); /* DS:4624 */
const char *const CombatRatingLightText = STORY_TEXT(UNIT_CLASS_LABEL_0);
const char *const CombatRatingModerateText = STORY_TEXT(COMBAT_RATING_MODERATE);
const char *const CombatRatingHeavyText = STORY_TEXT(COMBAT_RATING_HEAVY);
const char *const CombatRatingBrutalText = STORY_TEXT(COMBAT_RATING_BRUTAL);
const char *const CombatRatingDevastatingText = STORY_TEXT(COMBAT_RATING_DEVASTATING);
const char *const CombatRatingIncredibleText = STORY_TEXT(COMBAT_RATING_INCREDIBLE);
const char *const CombatRatingNoneText = STORY_TEXT(COMBAT_RATING_NONE);
const char *CombatRatingDescriptionTable[0x07] = {
    STORY_TEXT(UNIT_CLASS_LABEL_0), /* CombatRatingLightText */
    STORY_TEXT(COMBAT_RATING_MODERATE), /* CombatRatingModerateText */
    STORY_TEXT(COMBAT_RATING_HEAVY), /* CombatRatingHeavyText */
    STORY_TEXT(COMBAT_RATING_BRUTAL), /* CombatRatingBrutalText */
    STORY_TEXT(COMBAT_RATING_DEVASTATING), /* CombatRatingDevastatingText */
    STORY_TEXT(COMBAT_RATING_INCREDIBLE), /* CombatRatingIncredibleText */
    STORY_TEXT(COMBAT_RATING_NONE) /* CombatRatingNoneText */
};
RevengeUnitRecord PersistentUnitRoster[0x18];
RevengeUnitRecord LiveScenarioUnits[0x18];
RevengeUnitRecord BlankUnitRecord;
uint8_t ScenarioUnitMapX[0x18];
uint8_t ScenarioUnitMapY[0x18];
uint8_t ScenarioUnitFacing[0x18];
uint8_t CampaignClockState[0x03];
uint8_t SavedCampaignClockState[0x03];
/* DS:1D4C, the pilots as the game starts them: from the player's copy
 * (src/Support/original_import_pieces.txt). */
/* Its starting records, from data/rules/pilots.json (revenge/rules.h); the
 * campaign changes each pilot's availability. */
RevengePilotRosterRecord PilotRoster[REVENGE_PILOT_ROSTER_COUNT];
/* DS:2148: what slot FF reads as a roster entry; from the player's copy. */
RevengePilotRosterRecord PilotRosterRecordPastEnd;
/* Filled at start from the story (src/Support/original_import_pieces.txt). */
uint16_t UnitActionMaskByExperiencePastEnd[0xFC];
/* Filled at start from the story (src/Support/original_import_pieces.txt). */
uint8_t WeaponAIHeatLimitByExperiencePastEnd[0xFC];
/* Filled at start from data/ui/lance_editor.json (the places) and
 * data/start/lance_editor.json (the starting state); revenge/rules.h. */
RevengeUnitLanceAssignment
    LanceAssignments[REVENGE_LANCE_ASSIGNMENT_COUNT];
/* Filled at start from data/rules/units.json (revenge/rules.h). */
uint8_t ScenarioLanceAllegianceTable[0x07][0x03];
/* Filled at start from data/rules/weapons.json (revenge/rules.h). */
uint16_t AmmoCapacityMultiplierTable[0x0E];
uint16_t CampaignFlags;
uint16_t CampaignStageIndex;
uint16_t ScenarioUnitZeroAvailable;
int16_t ScenarioUnitFifteenState;
uint16_t ScenarioUnitFourteenEnabled;
uint16_t EjectedPilotRosterState;
uint16_t ScenarioUnitThirteenEnabled;
uint8_t StoryUnitDeployedFlag;
uint8_t SelectedSaveSlotIndex;
uint8_t SaveLoadMenuDefaultSelection;
uint8_t ScenarioRosterPrepared;
int8_t PendingCampaignStageDelta;
uint8_t PendingCampaignPhaseDelta;
int16_t ScenarioVariantIndex;
uint8_t CampaignPhase;
uint8_t ReplayCampaignStateRequested;
uint8_t TrainingSequenceFlag;
uint16_t DeferredStage16Story;
uint16_t SavedCampaignFlags;
uint8_t AlternateLanceSelection;
uint8_t SuppressPostStageMenu;
uint8_t CampaignRestartState;
uint16_t ModalSavedTextWindow;
uint16_t ModalBackingX;
uint16_t ModalBackingY;
uint16_t ModalBackingWidth;
uint16_t ModalBackingHeight;
uint8_t ActivePilotListWindowHeight;
uint8_t CommanderListWindowHeight;
uint8_t FallenPilotListWindowHeight;
/* DS:1B8C-1B8E: filled at start from data/start/new_campaign.json
 * (revenge/rules.h). */
RandomPilotPoolCountCopies RandomPilotPoolCounts;
uint16_t SaveTransferState;
uint16_t SavedSaveTransferState;
uint16_t CampaignScenarioState;
uint8_t ReplayScenarioRequested;
uint8_t CampaignMenuRequested;
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
uint8_t UnitEditorAllegiance;
uint16_t UnitEditorScenarioGroupCount;
uint16_t UnitEditorScenarioCardCount;
uint16_t UnitEditorPilotCardCount;
uint16_t UnitEditorLanceGroupCount;
uint16_t TextOutputMode;
uint16_t SelectedUnitEquipmentLocation;
RevengeUnitRecord EstimatedUnitRecord;
RevengeTacticalCombatRatings UnitCombatRatings[0x1A];
uint8_t UnitLanceFormationState[0x18];
/* DS:03EA starts as the image has it, lances 1 and 2 on, until a battle's
 * scene header sets it (1A1F:1C48). */
uint8_t TacticalScenarioLanceEnabled[0x03] = { 0x01, 0x01, 0x00 };
/* Filled at start from data/rules/combat.json (revenge/rules.h). */
uint8_t SensorDamageProximityRangeTable[0x03];
uint8_t LanceFormationProfile[0x06];
uint8_t UnitEnemyEngagementActive[0x18];
uint8_t UnitMovementHeatExponent[0x18];
uint8_t UnitOverburnOrCloseAssaultEnabled[0x18];
uint8_t UnitFormationMode[0x18];
uint16_t UnitConsecutiveAttackTicks[0x18];
uint16_t FirstLanceOrdersEnabled;
uint8_t UnitWasAttackedThisTick[0x18];
uint8_t GroundTargetWasAttacked;
uint8_t UnitJumpingState[0x18];
uint8_t UnitJumpMovementRemaining[0x18];
uint8_t UnitDestructionMarkers[0x18][0x03];
uint16_t ScenarioFileSuffix;
uint8_t CampaignUpgradeAwards[0x04];
uint8_t UnitEnemyInProximityRange[0x18];
uint8_t UnitShutdownState[0x18];
uint8_t UnitEnemyVisible[0x18];
/* DS:88A4. Entry 18h is the map-cell selection's (2208:0857), the free byte
 * DS:88BC. */
RevengeUnitSlot UnitTacticalTarget[0x19];
uint16_t EnemyCycleUnitIndex;
uint8_t UnitEnemyContactEstablished[0x18];
uint8_t UnitWithdrawalState[0x18];
RevengeUnitSlot UnitLastNarratedTarget[0x18];
uint8_t UnitPreferredEngagementMode[0x18];
uint8_t UnitTerrainType[0x18];
uint8_t UnitBelowHalfDurability[TACTICAL_UNIT_COUNT + 0x01];
uint8_t UnitCachedWeaponCounts[(TACTICAL_UNIT_COUNT + 0x01) * WEAPON_DEFINITION_COUNT];
uint8_t UnitWeaponRecoveryCounters[(TACTICAL_UNIT_COUNT + 0x01) * WEAPON_DEFINITION_COUNT];
uint8_t UnitHeatSinkCount[TACTICAL_UNIT_COUNT + 0x01];
uint32_t UnitWeaponAvailabilityMask[TACTICAL_UNIT_COUNT + 0x01];
uint16_t WeaponCountScratch[0x17];
uint8_t *TerrainTypeMap;
uint8_t *TacticalOverviewMapBuffer;
uint8_t *PackedTerrainDamageMap;
uint8_t *ActiveTerrainDamageByte;
uint8_t *TerrainDamageCapabilityTable;
const uint8_t TerrainDamageCellIncrementMask[0x04] = { 0x01, 0x04, 0x10, 0x40 };
const uint8_t TerrainDamageExtractMask[0x04] = { 0x03, 0x0C, 0x30, 0xC0 };
const uint8_t TerrainDamageShift[0x04] = { 0x00, 0x02, 0x04, 0x06 };
const uint8_t TerrainDamageCellOccupiedMask[0x04] = { 0xC0, 0x30, 0x0C, 0x03 };
const uint8_t TerrainDamageStageIncrement[0x04] = { 0x40, 0x10, 0x04, 0x01 };
uint16_t PackedTerrainDamageRowBytes;
uint8_t DestructibleTerrainTypeBase;
uint8_t TerrainImpactAnimationTypeMinimum;
uint8_t TerrainImpactAnimationTypeMaximum;
uint16_t TerrainLineOfSightBlockingThreshold;
uint8_t TerrainLineOfSightMaximumSteps;
uint8_t TacticalTerrainEffectMode;
RevengeTerrainDamageQueueEntry TerrainDamageQueue[TERRAIN_DAMAGE_QUEUE_ENTRY_COUNT];
uint8_t CampaignUpgradeMarkerX[0x04];
uint8_t CampaignUpgradeMarkerY[0x04];
uint16_t FriendlyUnitDestroyed;
uint8_t ScenarioDestructionCompletionState;
uint8_t ScenarioDestructionCompletionMarker;
uint16_t ProtectUnitZero;
uint16_t ImpassableTerrainTypeMinimum;
int8_t TerrainToHitModifier[0x100];
RevengeTerrainImpactAnimationRecord
    TerrainImpactAnimationRecords[TERRAIN_IMPACT_ANIMATION_RECORD_COUNT];
uint16_t NarrationMessageCategory;
uint16_t PendingNarrationDelay;
uint8_t DamagedWeaponRangeFlags[0x03];
int16_t CurrentUnitDurabilityTotal;
int16_t MaximumUnitDurabilityTotal;
char SharedNumericTextBuffer[0x20];
const char PercentageSuffixText[] = "%";
uint16_t TacticalUpdateDelayTicks;
uint16_t TacticalKeyRepeatDelayTicks;
/* Filled at start from data/start/game.json (revenge/rules.h). */
uint16_t TacticalUpdateDelayReloadTicks;
uint16_t TacticalGameSpeedIndex;
uint16_t TacticalBattleResult;
uint16_t TacticalClockDivider;
uint16_t TacticalImpactQueueCount;
RevengeTacticalImpactRecord
    TacticalImpactQueue[TACTICAL_IMPACT_RECORD_COUNT];
const int16_t TacticalCursorSearchOffsetX[0x09] = { 0x00, 0x00, 0x01, 0x00, -0x01, -0x01, 0x01, 0x01, -0x01 };
const int16_t TacticalCursorSearchOffsetY[0x09] = { 0x00, -0x01, 0x00, 0x01, 0x00, -0x01, -0x01, 0x01, 0x01 };
/* 1A1F:58A0 sign-extends these (cbw), so the extended key codes B0h-B9h
 * arrive negative. */
const int8_t TacticalMouseEdgeInputByMask[0x10] = { 0x00, -0x4B, -0x50, -0x4F, -0x4D, 0x00, -0x51, 0x00,
    -0x48, -0x47, 0x00, 0x00, -0x49, 0x00, 0x00, 0x00 };
uint8_t SceneNarrationEventFlags[0x06];
uint16_t HeatDissipationBySinkCountQ8_8[0x30];
uint16_t TerrainDamageQueueWordCount;
uint16_t TacticalProjectileCount;
RevengeTacticalProjectileRecord
    TacticalProjectileRecords[TACTICAL_PROJECTILE_RECORD_COUNT];
uint16_t TacticalOverviewDisplayMode;
uint16_t LrmShipmentLoaded;
uint16_t SrmShipmentLoaded;
/* DS:03E5 starts at 0001h, as the image has it, until a battle's scene
 * header sets it (1A1F:1C48). */
uint16_t TacticalOverviewMapMode = 0x0001;
uint16_t NarrationSourceUnit;
uint16_t LastAttackingUnitIndex;
uint16_t PendingMouseButtonEvent;
int16_t MouseCursorTestX;
int16_t MouseCursorTestY;
uint16_t MouseCursorUpdateLock;
int16_t MouseCursorRequestedX;
uint16_t MouseCursorHideDepth;
uint16_t MouseDriverDoubleX;
uint16_t MouseDriverInstalled;
uint16_t MouseButtonState;
uint16_t MouseY;
uint16_t MouseInputDisabled;
int16_t MouseCursorMinX;
int16_t MouseCursorMaxX;
int16_t MouseCursorMinY;
int16_t MouseCursorMaxY;
int16_t MouseCursorHotspotX;
int16_t MouseCursorHotspotY;
int16_t MouseCursorDisplayedX;
int16_t MouseCursorDisplayedY;
uint8_t *MouseCursorBackground;
const uint8_t *MouseCursorBitmap;
int16_t MouseCursorErasedX;
int16_t MouseCursorErasedY;
int16_t MouseCursorRestoreTileLeft;
int16_t MouseCursorRestoreTileTop;
int16_t MouseCursorRestoreTileRight;
int16_t MouseCursorRestoreTileBottom;
int16_t MouseCursorDrawnX;
int16_t MouseCursorDrawnY;
int16_t MouseCursorCaptureTileLeft;
int16_t MouseCursorCaptureTileTop;
int16_t MouseCursorCaptureTileRight;
int16_t MouseCursorCaptureTileBottom;
int16_t MouseCursorDrawLeft;
int16_t MouseCursorDrawTop;
uint8_t *ActiveRenderTarget;
uint16_t SpriteClipLeftTileMirror;
uint16_t SpriteClipRightTileMirror;
uint16_t SpriteClipTopPixelMirror;
uint16_t SpriteClipBottomPixelMirror;
uint16_t TacticalSpriteClipLeftTile;
uint16_t TacticalSpriteClipRightTile;
uint16_t TacticalSpriteClipTopPixel;
uint16_t TacticalSpriteClipBottomPixel;
/* REVENGE.EXE DS:800C is initialized to 0001h. */
uint16_t RequestedMusicEnabled = TRUE;
uint16_t CachedMusicResourceIndex;
uint16_t SpriteDrawBusy;
uint8_t *MusicStagingBuffer;
uint16_t PlanarBackingBufferExtentLow;
uint16_t PlanarBackingBufferExtentHigh;
uint8_t SceneTransitionAbortFlag;
/* DS:240F: filled at start from data/start/game.json (revenge/rules.h). */
uint16_t PresentationDismissTimeout;
uint16_t PresentationCallbackOffset;
uint16_t PresentationCallbackSegment;

uint16_t TacticalOverviewActive;
uint8_t ArtilleryShellingActive;
uint8_t RollingBarrageActive;
uint8_t StrafingRunActive;
uint8_t ArtilleryTargetMapX;
uint8_t ArtilleryTargetMapY;
uint8_t RollingBarrageEndMapX;
uint8_t RollingBarrageEndMapY;
uint8_t RollingBarrageStartMapX;
uint8_t RollingBarrageStartMapY;
uint8_t DropShipMapX;
uint8_t DropShipMapY;
uint8_t ArtillerySupportAvailable;
uint8_t StrafingSupportAvailable;
uint16_t TacticalOverviewViewportDirty;
static uint16_t TacticalCellTerrainStorage[0xE4];
static uint16_t TacticalCellSpriteStorage[0xE4];
uint16_t *TacticalCellTerrainMap = TacticalCellTerrainStorage;
uint16_t *TacticalCellSpriteMap = TacticalCellSpriteStorage;
uint8_t ScenarioUnitNeedsUpdate[0x18];
uint8_t TacticalStateInitialized;
uint16_t SuppressCampaignClock;
uint16_t TacticalCursorOutlineEnabled;
uint16_t TacticalCursorOutlineState;
