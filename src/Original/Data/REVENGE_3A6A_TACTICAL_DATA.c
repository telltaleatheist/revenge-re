// REVENGE_3A6A_TACTICAL_DATA.c
// Typed objects recovered from REVENGE.EXE DS segment 3A6A.

#include "revenge/original_data.h"
#include "revenge/audio.h"
#include "revenge/far_memory.h"
#include "revenge/video.h"
#include "REVENGE.h"
#include "revenge/original_import.h"

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
const char *const UnitStatusArmorLabel = ORIGINAL_TEXT(0x30E0);
const char *const UnitStatusHeatLabel = ORIGINAL_TEXT(0x30E7);
const char *const UnitStatusExitLabel = ORIGINAL_TEXT(0x30ED);
const char *const UnitStatusHeatDetailLabel = ORIGINAL_TEXT(0x4105);
const char *const UnitStatusDamageLabel = ORIGINAL_TEXT(0x410C);
const char *const UnitStatusRatingLabels =
	ORIGINAL_TEXT(0x40EB);
const char *const IntelligenceHeading = ORIGINAL_TEXT(0x4228);
const char *const ScannedByActiveProbeText = ORIGINAL_TEXT(0x4269);
const char *const DetectedByText = ORIGINAL_TEXT(0x4278);
const char *const RadarSensorText = ORIGINAL_TEXT(0x4284);
const char *const EnemyAndFriendlyLancesText =
	ORIGINAL_TEXT(0x4299);
const char *const UnknownUnitText = ORIGINAL_TEXT(0x42DB);
const char *const NoSatelliteUplinkText = ORIGINAL_TEXT(0x4534);
const char *const UplinkJammedText =
	ORIGINAL_TEXT(0x4553);
const char *const OutOfScanningText = ORIGINAL_TEXT(0x501D);
const char *const RangeText = ORIGINAL_TEXT(0x502D);
const char *const ArrowTagEquipmentText = ORIGINAL_TEXT(0x45FD);
const char BapEquipmentText[] = "BAP ";
const char *const AccuracyEquipmentText = ORIGINAL_TEXT(0x4608);
const char *const DoubleHeatSinkEquipmentText = ORIGINAL_TEXT(0x4612);
const char JumpEquipmentSuffix[] = " J";
const char *const CaseEquipmentText = ORIGINAL_TEXT(0x4628);
const char *const MoveDestinationPromptSuffix = ORIGINAL_TEXT(0x4698);
const char *const FormationHeading = ORIGINAL_TEXT(0x4790);
const char *const SatelliteZoomText = ORIGINAL_TEXT(0x479C);
const char *const ContinueText = ORIGINAL_TEXT(0x47AB);
const char *const DoneText = ORIGINAL_TEXT(0x5187);
const char NoTacticalTargetText[] = "?";
const char *const ViewSatelliteMapText = ORIGINAL_TEXT(0x4215);
const char *const SelectStrafingAreaPrompt = ORIGINAL_TEXT(0x462E);
const char *const SelectArtilleryTargetPrompt = ORIGINAL_TEXT(0x4645);
const char *const SelectBarrageStartPrompt =
	ORIGINAL_TEXT(0x465C);
const char *const SelectBarrageEndPrompt = ORIGINAL_TEXT(0x467B);
const char *const SelectFireTargetPrompt = ORIGINAL_TEXT(0x46A5);
const char *TacticalUnitCommandTextTable[0x0E] =
{
	ORIGINAL_TEXT(0x4EDF), /* TacticalCommandsText */
	ORIGINAL_TEXT(0x4EE8), /* TacticalLanceCommandsText */
	ORIGINAL_TEXT(0x4EF7), /* TacticalTargetText */
	ORIGINAL_TEXT(0x4F00), /* TacticalCeaseFireAtText */
	ORIGINAL_TEXT(0x4F0E), /* TacticalStopText */
	ORIGINAL_TEXT(0x4F13), /* TacticalWalkingSpeedText */
	ORIGINAL_TEXT(0x4F21), /* TacticalRunningSpeedText */
	ORIGINAL_TEXT(0x4F2F), /* TacticalMoveText */
	ORIGINAL_TEXT(0x4F35), /* TacticalStoppedText */
	ORIGINAL_TEXT(0x4F3A), /* TacticalCruisingSpeedText */
	ORIGINAL_TEXT(0x4F49), /* TacticalFlankSpeedText */
	ORIGINAL_TEXT(0x4F55), /* TacticalFireAtNoOneText */
	ORIGINAL_TEXT(0x4F64), /* TacticalFireDefensivelyText */
	ORIGINAL_TEXT(0x4F75) /* TacticalFireAtWillText */
};
const char *TacticalSupportCommandTextTable[0x08] =
{
	ORIGINAL_TEXT(0x4F82), /* ShellCoordinateText */
	ORIGINAL_TEXT(0x4F93), /* CallOffShellingText */
	ORIGINAL_TEXT(0x4FA5), /* RollingBarrageText */
	ORIGINAL_TEXT(0x4FB5), /* CallOffBarrageText */
	ORIGINAL_TEXT(0x4FC6), /* CallStrafingText */
	ORIGINAL_TEXT(0x4FD4), /* CallOffStrafingText */
	ORIGINAL_TEXT(0x4FE6), /* CloseAssaultText */
	ORIGINAL_TEXT(0x4FF4) /* CeaseAssaultText */
};
const char *TacticalOverburnTextTable[0x02] =
	{ ORIGINAL_TEXT(0x5002) /* OverburnOffText */, ORIGINAL_TEXT(0x5010) /* OverburnOnText */ };
const char *LanceStatusTextTable[0x08] =
{
	ORIGINAL_TEXT(0x5033), /* LanceCommandText */
	ORIGINAL_TEXT(0x503B), /* LanceFireText */
	ORIGINAL_TEXT(0x5040), /* LanceAssaultText */
	ORIGINAL_TEXT(0x5048), /* LanceReconText */
	ORIGINAL_TEXT(0x504E), /* LanceLightText */
	ORIGINAL_TEXT(0x5054), /* LanceMediumText */
	ORIGINAL_TEXT(0x505B), /* LanceHeavyText */
	ORIGINAL_TEXT(0x5061) /* LanceAssaultWeightText */
};
const char DeadText[] = "Dead";
RevengeMapEdgeEffectRecord MapEdgeEffectQueue[MAP_EDGE_EFFECT_RECORD_COUNT];
word16 MapEdgeEffectCount;
word16 PrimaryStatusRow;
word16 SecondaryStatusRow;
word16 TacticalUiSuppressed;
word16 PrimaryStatusHighlightColor;
word16 CachedPrimaryStatusHighlightColor;
word16 SecondaryStatusHighlightColor;
word16 CachedSecondaryStatusHighlightColor;
byte TacticalStatusRowInvalidation[0x18];
byte TacticalOverviewMarkerX[0x18];
word16 TacticalOverviewViewportX;
byte TacticalOverviewMarkerY[0x18];
word16 TacticalOverviewViewportY;
word16 TacticalOverviewScreenOriginX;
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
const char FriendlyUnitStatusHeading[] = "CU ";
const char EnemyUnitStatusHeading[] = "   ";
const char UnitStatusSeparatorText[] = " ";
const char UnitHeatStatusPrefix[] = "/";
const char TacticalNarrationBlankText[] = "   ";
const char TacticalClockBlankText[] = "   ";
const char TacticalNarrationLabel[] = "MSG";
const char *const CombatRatingLightText = ORIGINAL_TEXT(0x366D);
const char *const CombatRatingModerateText = ORIGINAL_TEXT(0x4E09);
const char *const CombatRatingHeavyText = ORIGINAL_TEXT(0x4E12);
const char *const CombatRatingBrutalText = ORIGINAL_TEXT(0x4E18);
const char *const CombatRatingDevastatingText = ORIGINAL_TEXT(0x4E1F);
const char *const CombatRatingIncredibleText = ORIGINAL_TEXT(0x4E2B);
const char *const CombatRatingNoneText = ORIGINAL_TEXT(0x4E36);
const char *CombatRatingDescriptionTable[0x07] =
{
	ORIGINAL_TEXT(0x366D), /* CombatRatingLightText */
	ORIGINAL_TEXT(0x4E09), /* CombatRatingModerateText */
	ORIGINAL_TEXT(0x4E12), /* CombatRatingHeavyText */
	ORIGINAL_TEXT(0x4E18), /* CombatRatingBrutalText */
	ORIGINAL_TEXT(0x4E1F), /* CombatRatingDevastatingText */
	ORIGINAL_TEXT(0x4E2B), /* CombatRatingIncredibleText */
	ORIGINAL_TEXT(0x4E36) /* CombatRatingNoneText */
};
RevengeUnitRecord PersistentUnitRoster[0x18];
RevengeUnitRecord LiveScenarioUnits[0x18];
RevengeUnitRecord BlankUnitRecord;
byte SavedScenarioUnitMapX[0x18];
byte SavedScenarioUnitMapY[0x18];
byte ScenarioUnitMapX[0x18];
byte ScenarioUnitMapY[0x18];
byte ScenarioUnitFacing[0x18];
byte CampaignClockState[0x03];
byte SavedCampaignClockState[0x03];
/* DS:1D4C, the pilots as the game starts them: from the player's copy
 * (src/Support/original_import_pieces.txt). */
RevengePilotRosterRecord PilotRoster[REVENGE_PILOT_ROSTER_COUNT];
RevengeUnitLanceAssignment
	LanceAssignments[REVENGE_LANCE_ASSIGNMENT_COUNT];
const byte ScenarioLanceAllegianceTable[0x08][0x03] = {
	{ 0x01, 0x01, 0x01 }, { 0x00, 0x00, 0x00 },
	{ 0x00, 0x01, 0x01 }, { 0x01, 0x00, 0x01 },
	{ 0x00, 0x02, 0x02 }, { 0x00, 0x03, 0x03 },
	{ 0x00, 0x00, 0x03 }, { 0x00, 0x00, 0x00 }
};
const word16 AmmoCapacityMultiplierTable[0x0E] = {
	0x002D, 0x0014, 0x000A, 0x0008, 0x0008, 0x0005, 0x00C8,
	0x0018, 0x000C, 0x0008, 0x0006, 0x0032, 0x0019, 0x000F
};
word16 CampaignFlags;
word16 CampaignStageIndex;
word16 ScenarioUnitZeroAvailable;
int16 ScenarioUnitFifteenState;
word16 ScenarioUnitFourteenEnabled;
word16 EjectedPilotRosterState;
word16 ScenarioUnitThirteenEnabled;
byte CampaignUnitThirteenAvailabilityFlag;
byte CampaignUnitFifteenOverrideFlag;
byte CampaignUnitFourteenAvailabilityFlag;
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
byte CampaignState1B8C;
byte CampaignState1B8D;
/* DS:1B8E: from the player's copy (original_import_pieces.txt). */
byte CampaignState1B8E;
word16 SaveTransferState;
word16 SavedSaveTransferState;
word16 CampaignScenarioState;
byte ReplayScenarioRequested;
byte CampaignMenuRequested;
word16 NarrationFocusRegion;
/* Relocated initial far pointers recovered from REVENGE.EXE DS:1530 and
 * DS:1566. */
word16 UnitTemplatePathOffset = 0x21B8;
word16 UnitTemplatePathSegment = 0x3A6A;
word16 TacticalResourcePathOffset = 0x21DD;
word16 TacticalResourcePathSegment = 0x3A6A;
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
const byte SensorDamageProximityRangeTable[0x03] =
	{ 0x14, 0x0A, 0x00 };
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
word16 HeatDissipationBySinkCountQ8_8[0x100];
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
byte MusicDriverPlaybackMode;
word16 CachedMusicResourceIndex;
word16 SpriteDrawBusy;
byte *MusicStagingBuffer;
word16 PlanarBackingBufferExtentLow;
word16 PlanarBackingBufferExtentHigh;
byte SceneTransitionAbortFlag;
/* DS:240F: from the player's copy (original_import_pieces.txt). */
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
