// REVENGE_3A6A_TACTICAL_DATA.c
// Typed objects recovered from REVENGE.EXE DS segment 3A6A.

#include "revenge/original_data.h"
#include "revenge/audio.h"
#include "revenge/far_memory.h"
#include "revenge/video.h"
#include "REVENGE.h"
#include "revenge/story_text.h"

byte *DamagedTerrainSpriteLookup;
byte TacticalEffectOverlayMap[0x00E4];
uint32 GameTimerTicks;
uint32 GameAnimationTicks;
int8 TacticalSlotSpriteState[0x18];
byte UnitTypeResourceCode[0x07];
byte UnitTacticalOrderMode[0x18];
/* DS:88F3. 2208:060D and 0857 also index entry 18h, the map-cell selection's,
 * which is the free byte DS:890B before the next variable. */
int8 UnitAttackTarget[0x19];
word16 UnitSpriteBaseId[0x18];
word16 UnitSpriteId[0x18];
word16 TacticalSpriteAnimationPhase;
/* DS:03DC starts at FFFFh: no unit selected. */
word16 TacticalSelectedUnitIndex = 0xFFFF;
word16 TacticalViewportMapX;
word16 TacticalViewportMapY;
word16 TacticalCursorViewportX;
word16 TacticalCursorViewportY;
byte UnitTargetMapX[0x19];
byte UnitTargetMapY[0x19];
byte UnitRouteTargetMapX[0x19];
byte UnitRouteTargetMapY[0x19];
SceneUnitRoute SavedTacticalRoute;
byte TacticalInteractionLocked;
word16 TacticalMenuInputState;
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
const char *TacticalUnitCommandTextTable[0x0E] =
{
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
const char *TacticalSupportCommandTextTable[0x08] =
{
	STORY_TEXT(TACTICAL_SUPPORT_COMMAND_0), /* ShellCoordinateText */
	STORY_TEXT(TACTICAL_SUPPORT_COMMAND_1), /* CallOffShellingText */
	STORY_TEXT(TACTICAL_SUPPORT_COMMAND_2), /* RollingBarrageText */
	STORY_TEXT(TACTICAL_SUPPORT_COMMAND_3), /* CallOffBarrageText */
	STORY_TEXT(TACTICAL_SUPPORT_COMMAND_4), /* CallStrafingText */
	STORY_TEXT(TACTICAL_SUPPORT_COMMAND_5), /* CallOffStrafingText */
	STORY_TEXT(TACTICAL_SUPPORT_COMMAND_6), /* CloseAssaultText */
	STORY_TEXT(TACTICAL_SUPPORT_COMMAND_7) /* CeaseAssaultText */
};
const char *TacticalOverburnTextTable[0x02] =
	{ STORY_TEXT(TACTICAL_OVERBURN_0) /* OverburnOffText */, STORY_TEXT(TACTICAL_OVERBURN_1) /* OverburnOnText */ };
const char *LanceStatusTextTable[0x08] =
{
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
word16 MapEdgeEffectCount;
word16 PrimaryStatusRow;
word16 SecondaryStatusRow;
word16 TacticalUiSuppressed;
word16 PrimaryStatusHighlightColor;
word16 CachedPrimaryStatusHighlightColor;
word16 SecondaryStatusHighlightColor;
word16 CachedSecondaryStatusHighlightColor;
byte TacticalStatusRowInvalidation[0x0D];
byte TacticalOverviewMarkerX[0x18];
word16 TacticalOverviewViewportX;
byte TacticalOverviewMarkerY[0x18];
word16 TacticalOverviewViewportY;
word16 TacticalOverviewScreenOriginX;
/* Filled at start from data/start/game.json (revenge/rules.h). */
word16 DisplayedTacticalStatusRow;
word16 TacticalStatusBlinkCountdown;
byte TacticalStatusBlinkVisible;
const int8_t WeaponTraceMajorStepX[0x08] =
	{ 0x00, 0x01, 0x01, 0x00, 0x00, -0x01, -0x01, 0x00 };
const int8_t WeaponTraceMajorStepY[0x08] =
	{ -0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, -0x01 };
const int8_t WeaponTraceMinorStepX[0x08] =
	{ 0x01, 0x00, 0x00, 0x01, -0x01, 0x00, 0x00, -0x01 };
const int8_t WeaponTraceMinorStepY[0x08] =
	{ 0x00, -0x01, 0x01, 0x00, 0x00, 0x01, -0x01, 0x00 };
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
const char *CombatRatingDescriptionTable[0x07] =
{
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
byte ScenarioUnitMapX[0x18];
byte ScenarioUnitMapY[0x18];
byte ScenarioUnitFacing[0x18];
byte CampaignClockState[0x03];
byte SavedCampaignClockState[0x03];
/* DS:1D4C, the pilots as the game starts them: from the player's copy
 * (src/Support/original_import_pieces.txt). */
/* Its starting records, from data/rules/pilots.json (revenge/rules.h); the
 * campaign changes each pilot's availability. */
RevengePilotRosterRecord PilotRoster[REVENGE_PILOT_ROSTER_COUNT];
/* Filled at start from data/ui/lance_editor.json (the places) and
 * data/start/lance_editor.json (the starting state); revenge/rules.h. */
RevengeUnitLanceAssignment
	LanceAssignments[REVENGE_LANCE_ASSIGNMENT_COUNT];
/* Filled at start from data/rules/units.json (revenge/rules.h). */
byte ScenarioLanceAllegianceTable[0x07][0x03];
/* Filled at start from data/rules/weapons.json (revenge/rules.h). */
word16 AmmoCapacityMultiplierTable[0x0E];
word16 CampaignFlags;
word16 CampaignStageIndex;
word16 ScenarioUnitZeroAvailable;
int16 ScenarioUnitFifteenState;
word16 ScenarioUnitFourteenEnabled;
word16 EjectedPilotRosterState;
word16 ScenarioUnitThirteenEnabled;
byte StoryUnitDeployedFlag;
byte SelectedSaveSlotIndex;
byte SaveLoadMenuDefaultSelection;
byte ScenarioRosterPrepared;
int8 PendingCampaignStageDelta;
byte PendingCampaignPhaseDelta;
int16 ScenarioVariantIndex;
byte CampaignPhase;
byte ReplayCampaignStateRequested;
byte TrainingSequenceFlag;
word16 DeferredStage16Story;
word16 SavedCampaignFlags;
byte AlternateLanceSelection;
byte SuppressPostStageMenu;
byte CampaignRestartState;
word16 ModalSavedTextWindow;
word16 ModalBackingX;
word16 ModalBackingY;
word16 ModalBackingWidth;
word16 ModalBackingHeight;
byte ActivePilotListWindowHeight;
byte CommanderListWindowHeight;
byte FallenPilotListWindowHeight;
/* DS:1B8C-1B8E: filled at start from data/start/new_campaign.json
 * (revenge/rules.h). */
RandomPilotPoolCountCopies RandomPilotPoolCounts;
word16 SaveTransferState;
word16 SavedSaveTransferState;
word16 CampaignScenarioState;
byte ReplayScenarioRequested;
byte CampaignMenuRequested;
word16 NarrationFocusRegion;
word16 TacticalOverviewClockNoticeActive;
word16 TacticalRuntimeState;
word16 PreviousMouseCellX;
word16 PreviousMouseCellY;
byte TacticalOverviewNoticeChance;
byte EjectedPilotMapX;
byte EjectedPilotMapY;
byte EjectedPilotState;
word16 TacticalCursorRuntimeState;
word16 TacticalCursorMapX;
word16 TacticalCursorMapY;
word16 LastOverviewNoticeHour;
word16 TacticalOverviewRuntimeState;
byte PrimaryLanceResourceCode;
byte UnitEditorAllegiance;
word16 UnitEditorScenarioGroupCount;
word16 UnitEditorScenarioCardCount;
word16 UnitEditorPilotCardCount;
word16 UnitEditorLanceGroupCount;
word16 TextOutputMode;
word16 SelectedUnitEquipmentLocation;
RevengeUnitRecord EstimatedUnitRecord;
RevengeTacticalCombatRatings UnitCombatRatings[0x1A];
byte UnitLanceFormationState[0x18];
byte TacticalScenarioLanceEnabled[0x03];
/* Filled at start from data/rules/combat.json (revenge/rules.h). */
byte SensorDamageProximityRangeTable[0x03];
byte LanceFormationProfile[0x06];
byte UnitEnemyEngagementActive[0x18];
byte UnitMovementHeatExponent[0x18];
byte UnitOverburnOrCloseAssaultEnabled[0x18];
byte UnitFormationMode[0x18];
word16 UnitConsecutiveAttackTicks[0x18];
word16 FirstLanceOrdersEnabled;
byte UnitWasAttackedThisTick[0x18];
byte GroundTargetWasAttacked;
byte UnitJumpingState[0x18];
byte UnitJumpMovementRemaining[0x18];
byte UnitDestructionMarkers[0x18][0x03];
word16 ScenarioFileSuffix;
byte CampaignUpgradeAwards[0x04];
byte UnitEnemyInProximityRange[0x18];
byte UnitShutdownState[0x18];
byte UnitEnemyVisible[0x18];
/* DS:88A4. Entry 18h is the map-cell selection's (2208:0857), the free byte
 * DS:88BC. */
RevengeUnitSlot UnitTacticalTarget[0x19];
word16 EnemyCycleUnitIndex;
byte UnitEnemyContactEstablished[0x18];
byte UnitWithdrawalState[0x18];
RevengeUnitSlot UnitLastNarratedTarget[0x18];
byte UnitPreferredEngagementMode[0x18];
byte UnitTerrainType[0x18];
byte UnitBelowHalfDurability[TACTICAL_UNIT_COUNT + 0x01];
byte UnitCachedWeaponCounts[
	(TACTICAL_UNIT_COUNT + 0x01) * WEAPON_DEFINITION_COUNT];
byte UnitWeaponRecoveryCounters[
	(TACTICAL_UNIT_COUNT + 0x01) * WEAPON_DEFINITION_COUNT];
byte UnitHeatSinkCount[TACTICAL_UNIT_COUNT + 0x01];
uint32 UnitWeaponAvailabilityMask[TACTICAL_UNIT_COUNT + 0x01];
word16 WeaponCountScratch[0x17];
byte *TerrainTypeMap;
byte *TacticalOverviewMapBuffer;
byte *PackedTerrainDamageMap;
byte *ActiveTerrainDamageByte;
byte *TerrainDamageCapabilityTable;
const byte TerrainDamageCellIncrementMask[0x04] =
	{ 0x01, 0x04, 0x10, 0x40 };
const byte TerrainDamageExtractMask[0x04] =
	{ 0x03, 0x0C, 0x30, 0xC0 };
const byte TerrainDamageShift[0x04] =
	{ 0x00, 0x02, 0x04, 0x06 };
const byte TerrainDamageCellOccupiedMask[0x04] =
	{ 0xC0, 0x30, 0x0C, 0x03 };
const byte TerrainDamageStageIncrement[0x04] =
	{ 0x40, 0x10, 0x04, 0x01 };
word16 PackedTerrainDamageRowBytes;
byte DestructibleTerrainTypeBase;
byte TerrainImpactAnimationTypeMinimum;
byte TerrainImpactAnimationTypeMaximum;
word16 TerrainLineOfSightBlockingThreshold;
byte TerrainLineOfSightMaximumSteps;
byte TacticalTerrainEffectMode;
RevengeTerrainDamageQueueEntry TerrainDamageQueue[TERRAIN_DAMAGE_QUEUE_ENTRY_COUNT];
byte CampaignUpgradeMarkerX[0x04];
byte CampaignUpgradeMarkerY[0x04];
word16 FriendlyUnitDestroyed;
byte ScenarioDestructionCompletionState;
byte ScenarioDestructionCompletionMarker;
word16 ProtectUnitZero;
word16 ImpassableTerrainTypeMinimum;
int8_t TerrainToHitModifier[0x100];
RevengeTerrainImpactAnimationRecord
	TerrainImpactAnimationRecords[TERRAIN_IMPACT_ANIMATION_RECORD_COUNT];
word16 NarrationMessageCategory;
word16 PendingNarrationDelay;
byte DamagedWeaponRangeFlags[0x03];
int16 CurrentUnitDurabilityTotal;
int16 MaximumUnitDurabilityTotal;
char SharedNumericTextBuffer[0x20];
const char PercentageSuffixText[] = "%";
word16 TacticalUpdateDelayTicks;
word16 TacticalKeyRepeatDelayTicks;
/* Filled at start from data/start/game.json (revenge/rules.h). */
word16 TacticalUpdateDelayReloadTicks;
word16 TacticalGameSpeedIndex;
word16 TacticalBattleResult;
word16 TacticalClockDivider;
word16 TacticalImpactQueueCount;
RevengeTacticalImpactRecord
	TacticalImpactQueue[TACTICAL_IMPACT_RECORD_COUNT];
const int16 TacticalCursorSearchOffsetX[0x09] =
	{ 0x00, 0x00, 0x01, 0x00, -0x01, -0x01, 0x01, 0x01, -0x01 };
const int16 TacticalCursorSearchOffsetY[0x09] =
	{ 0x00, -0x01, 0x00, 0x01, 0x00, -0x01, -0x01, 0x01, 0x01 };
/* 1A1F:58A0 sign-extends these (cbw), so the extended key codes B0h-B9h
 * arrive negative. */
const int8 TacticalMouseEdgeInputByMask[0x10] =
	{ 0x00, -0x4B, -0x50, -0x4F, -0x4D, 0x00, -0x51, 0x00,
	  -0x48, -0x47, 0x00, 0x00, -0x49, 0x00, 0x00, 0x00 };
byte SceneNarrationEventFlags[0x06];
word16 HeatDissipationBySinkCountQ8_8[0x30];
word16 TerrainDamageQueueWordCount;
word16 TacticalProjectileCount;
RevengeTacticalProjectileRecord
	TacticalProjectileRecords[TACTICAL_PROJECTILE_RECORD_COUNT];
word16 TacticalOverviewDisplayMode;
word16 LrmShipmentLoaded;
word16 SrmShipmentLoaded;
word16 TacticalOverviewMapMode;
word16 NarrationSourceUnit;
word16 LastAttackingUnitIndex;
word16 PendingMouseButtonEvent;
int16 MouseCursorTestX;
int16 MouseCursorTestY;
word16 MouseCursorUpdateLock;
int16 MouseCursorRequestedX;
word16 MouseCursorHideDepth;
word16 MouseDriverDoubleX;
word16 MouseDriverInstalled;
word16 MouseButtonState;
word16 MouseY;
word16 MouseInputDisabled;
int16 MouseCursorMinX;
int16 MouseCursorMaxX;
int16 MouseCursorMinY;
int16 MouseCursorMaxY;
int16 MouseCursorHotspotX;
int16 MouseCursorHotspotY;
int16 MouseCursorDisplayedX;
int16 MouseCursorDisplayedY;
byte *MouseCursorBackground;
const byte *MouseCursorBitmap;
int16 MouseCursorErasedX;
int16 MouseCursorErasedY;
int16 MouseCursorRestoreTileLeft;
int16 MouseCursorRestoreTileTop;
int16 MouseCursorRestoreTileRight;
int16 MouseCursorRestoreTileBottom;
int16 MouseCursorDrawnX;
int16 MouseCursorDrawnY;
int16 MouseCursorCaptureTileLeft;
int16 MouseCursorCaptureTileTop;
int16 MouseCursorCaptureTileRight;
int16 MouseCursorCaptureTileBottom;
int16 MouseCursorDrawLeft;
int16 MouseCursorDrawTop;
byte *ActiveRenderTarget;
word16 SpriteClipLeftTileMirror;
word16 SpriteClipRightTileMirror;
word16 SpriteClipTopPixelMirror;
word16 SpriteClipBottomPixelMirror;
word16 TacticalSpriteClipLeftTile;
word16 TacticalSpriteClipRightTile;
word16 TacticalSpriteClipTopPixel;
word16 TacticalSpriteClipBottomPixel;
/* REVENGE.EXE DS:800C is initialized to 0001h. */
word16 RequestedMusicEnabled = TRUE;
word16 CachedMusicResourceIndex;
word16 SpriteDrawBusy;
byte *MusicStagingBuffer;
word16 PlanarBackingBufferExtentLow;
word16 PlanarBackingBufferExtentHigh;
byte SceneTransitionAbortFlag;
/* DS:240F: filled at start from data/start/game.json (revenge/rules.h). */
word16 PresentationDismissTimeout;
word16 PresentationCallbackOffset;
word16 PresentationCallbackSegment;

word16 TacticalOverviewActive;
byte ArtilleryShellingActive;
byte RollingBarrageActive;
byte StrafingRunActive;
byte ArtilleryTargetMapX;
byte ArtilleryTargetMapY;
byte RollingBarrageEndMapX;
byte RollingBarrageEndMapY;
byte RollingBarrageStartMapX;
byte RollingBarrageStartMapY;
byte DropShipMapX;
byte DropShipMapY;
byte ArtillerySupportAvailable;
byte StrafingSupportAvailable;
word16 TacticalOverviewViewportDirty;
static word16 TacticalCellTerrainStorage[0xE4];
static word16 TacticalCellSpriteStorage[0xE4];
word16 *TacticalCellTerrainMap = TacticalCellTerrainStorage;
word16 *TacticalCellSpriteMap = TacticalCellSpriteStorage;
byte ScenarioUnitNeedsUpdate[0x18];
byte TacticalStateInitialized;
word16 SuppressCampaignClock;
word16 TacticalCursorOutlineEnabled;
word16 TacticalCursorOutlineState;
