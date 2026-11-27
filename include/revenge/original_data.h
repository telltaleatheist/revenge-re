#ifndef REVENGE_ORIGINAL_DATA_H
#define REVENGE_ORIGINAL_DATA_H

#include "revenge/types.h"
#include "revenge/original/io_data.h"
#include "revenge/original/input_data.h"
#include "revenge/original/resource_data.h"
#include "revenge/original/scene_data.h"
#include "revenge/text.h"
#include "revenge/dos.h"
#include "revenge/unit.h"
#include "revenge/roster.h"
#include "revenge/combat_data.h"
#include "revenge/tactical_data.h"

enum
{
	ORIGINAL_TEXT_WINDOW_COUNT = 0x17
};


typedef void (*ResourceErrorCallback)(void);
typedef void (*OriginalExitHandler)(void);
typedef word16 (*MediaConfigurationHandler)(void);

#pragma pack(push, 1)
typedef struct MediaConfigurationChoice
{
	char name[0x0C];
	byte value;
} MediaConfigurationChoice;

typedef struct SoundConfigurationChoice
{
	char name[0x0C];
	byte driverIndex;
	byte hasNumericParameter;
} SoundConfigurationChoice;
#pragma pack(pop)

_Static_assert(sizeof(MediaConfigurationChoice) == 0x0D,
	"MediaConfigurationChoice must retain its DOS layout");
_Static_assert(sizeof(SoundConfigurationChoice) == 0x0E,
	"SoundConfigurationChoice must retain its DOS layout");

#pragma pack(push, 1)
typedef struct PITCalibrationState
{
	word16 Channel0Divisor;
	DosAddress SavedIRQ0Vector;
	word16 SavedInterruptFlags;
	word16 LatchedChannel0Count;
} PITCalibrationState;
#pragma pack(pop)

_Static_assert(sizeof(PITCalibrationState) == 0x0A,
	"PITCalibrationState must retain DS:6FA4-6FAD layout");
_Static_assert(offsetof(PITCalibrationState, SavedIRQ0Vector) == 0x02,
	"saved IRQ0 vector must retain its original offset");

/* Original primary data segment (3A6A) fields, named by DS offset. */
extern word16 MapWidth;
extern word16 MapHeight;
extern byte VideoPathUnavailable;
extern word16 SavedBIOSVideoMode;
extern const word16 BIOSVideoModeByAdapter[0x07];
extern word16 GameInitialized;
extern word16 StartupResourceState;
extern byte *InvalidSaveDialogBacking;
extern word16 NumericKeypadNavigationEnabled;
extern word16 StartupInputMode;
extern word16 SoundEffectsEnabled;
extern word16 DigitalSoundBusy;
extern word16 DigitalPlaybackRateHz;
extern word16 AppliedMusicEnabled;
extern word16 SoundDriverNumericParameter;
extern word16 InputDeviceType;
extern int8 SoundDriverIndex;
extern byte *Font6Resource;
extern byte *Font8Resource;
extern byte *SecondaryScreenBuffer;
extern byte *PlanarScreenBuffer;
extern word16 MouseCursorBackingSourceTileX;
extern word16 MouseCursorBackingSourcePixelY;
extern word16 MouseCursorBackingWidthTiles;
extern word16 MouseCursorBackingHeightPixels;
extern byte *SharedGraphicsBackingBuffers[0x26];
extern byte *VideoFramebuffer;
extern word16 ResourceRuntimeInitialized;
extern byte VideoPaletteData[0x0310];
extern const char MediaConfigurationFileName[];
extern const char *const InvalidSaveVersionText;
extern const char VersionDecimalSeparator[];
extern const char VersionTranslationSource[];
extern const char VersionTranslationReplacement[];
extern const char MediaConfigAssignmentDelimiter[];
extern const char VideoConfigTokenDelimiters[];
extern const char MusicConfigTokenDelimiters[];
extern const char SoundConfigTokenDelimiters[];
extern const char SoundParameterTokenDelimiters[];
extern const char InputConfigTokenDelimiters[];
extern MediaConfigurationChoice VideoConfigurationRecords[0x05];
extern MediaConfigurationChoice MusicConfigurationRecords[0x07];
extern MediaConfigurationChoice InputConfigurationRecords[0x02];
extern SoundConfigurationChoice SoundConfigurationRecords[0x09];
extern byte CompressedTextFirstByteTable[0x10];
extern byte CompressedTextSecondByteTable[0x80];
extern word16 CgaNarrowStrideMode;
extern word16 UiSelectionResetFlagA;
extern word16 UiSelectionResetFlagB;
extern word16 ActiveTextWindowLeftTile;
extern word16 ActiveTextWindowTopTile;
extern TextWindowContext TextWindowContexts[ORIGINAL_TEXT_WINDOW_COUNT];
extern word16 ActiveTextWindowIndex;
extern word16 ActiveTextWindowWidthTiles;
extern word16 ActiveTextWindowHeightTiles;
extern word16 ActiveTextForegroundColor;
extern word16 ActiveTextBackgroundColor;
extern word16 ActiveTextCursorColumn;
extern word16 ActiveTextCursorRow;
extern word16 TextWindowStateInitialized;
extern word16 UiNavigationUpKey;
extern word16 UiNavigationDownKey;
extern word16 UiPreviousRegionKey;
extern word16 UiNextRegionKey;
extern int16 RuntimeErrno;
extern int16 RuntimeDOSError;
extern const int8_t DOSErrorToErrnoTable[0x59];
extern PITCalibrationState PITCalibration;
extern word16 VerticalRetraceEdgeMode;
extern byte *DamagedTerrainSpriteLookup;
extern byte TacticalEffectOverlayMap[0x00E4];
extern uint32 GameTimerTicks;
extern uint32 GameAnimationTicks;
extern const char CampaignClockSeparatorText[];
extern const char FormattedLineBreak[];
extern const char PathSeparatorText[];
extern const char PercentCloseText[];
extern int8 TacticalSlotSpriteState[0x18];
extern byte UnitTypeResourceCode[0x07];
extern byte UnitTacticalOrderMode[0x18];
extern int8 UnitAttackTarget[0x19];
extern word16 UnitSpriteBaseId[0x18];
extern word16 UnitSpriteId[0x18];
extern word16 TacticalSpriteAnimationPhase;
extern word16 TacticalSelectedUnitIndex;
extern word16 TacticalViewportMapX;
extern word16 TacticalViewportMapY;
extern word16 TacticalCursorViewportX;
extern word16 TacticalCursorViewportY;
extern byte UnitTargetMapX[0x19];
extern byte UnitTargetMapY[0x19];
extern byte UnitRouteTargetMapX[0x19];
extern byte UnitRouteTargetMapY[0x19];
extern SceneUnitRoute SavedTacticalRoute;
/* DS:88BD is both the configured input-device selector and the tactical
 * mouse-edge scrolling flag. */
#define TacticalCursorAutoScroll InputDeviceType
/* These addresses are fields inside the original packed DS tables. */
#define TacticalMenuFirstRegion \
	(TextWindowContexts[0x04].topTile)
#define TacticalMenuLastRegion \
	(TextWindowContexts[0x04].heightTiles)
#define TacticalMenuOptionCount \
	(UiRegionTable[0x01].rowCount)
#define TacticalMenuSelectedOption \
	(UiRegionTable[0x01].selectedRow)
extern byte TacticalInteractionLocked;
extern word16 TacticalMenuInputState;
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
extern word16 MapEdgeEffectCount;
extern word16 PrimaryStatusRow;
extern word16 SecondaryStatusRow;
extern word16 TacticalUiSuppressed;
extern word16 PrimaryStatusHighlightColor;
extern word16 CachedPrimaryStatusHighlightColor;
extern word16 SecondaryStatusHighlightColor;
extern word16 CachedSecondaryStatusHighlightColor;
/* DS:B574: 0Dh bytes; the status slots the code and the scenes use are 0-8. */
extern byte TacticalStatusRowInvalidation[0x0D];
extern byte TacticalOverviewMarkerX[0x18];
extern word16 TacticalOverviewViewportX;
extern byte TacticalOverviewMarkerY[0x18];
extern word16 TacticalOverviewViewportY;
extern word16 TacticalOverviewScreenOriginX;
#define TacticalContactMarkerPending \
	TacticalStatusRowInvalidation[0x07]
extern word16 DisplayedTacticalStatusRow;
extern word16 TacticalStatusBlinkCountdown;
extern byte TacticalStatusBlinkVisible;
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
extern byte ScenarioUnitMapX[0x18];
extern byte ScenarioUnitMapY[0x18];
extern byte ScenarioUnitFacing[0x18];
/* DS:8337-8339: the hour, the minute and the battle's subtick, the byte the
 * battle loop steps; the scenario's saved clock (DS:855F) is the same three. */
extern byte CampaignClockState[0x03];
#define TacticalClockSubtick (CampaignClockState[0x02])
extern byte SavedCampaignClockState[0x03];
extern RevengePilotRosterRecord PilotRoster[REVENGE_PILOT_ROSTER_COUNT];
/* DS:2148, the roster entry past its end that slot FF reads (1A1F:339F,
 * 30F1:0AFC): in the image, four bytes of a table of file names. From the
 * player's copy (src/Support/original_import_pieces.txt). */
extern RevengePilotRosterRecord PilotRosterRecordPastEnd;
extern RevengeUnitLanceAssignment
	LanceAssignments[REVENGE_LANCE_ASSIGNMENT_COUNT];
/* DS:5928: seven rows; the scenarios use rows 0-4 and 6. */
extern byte ScenarioLanceAllegianceTable[0x07][0x03];  /* the rules' */
extern word16 AmmoCapacityMultiplierTable[0x0E];     /* the rules' */
extern word16 CampaignFlags;
extern word16 CampaignStageIndex;
extern word16 ScenarioUnitZeroAvailable;
extern int16 ScenarioUnitFifteenState;
extern word16 ScenarioUnitFourteenEnabled;
extern word16 EjectedPilotRosterState;
extern word16 ScenarioUnitThirteenEnabled;
extern byte StoryUnitDeployedFlag;
extern byte SelectedSaveSlotIndex;
extern byte SaveLoadMenuDefaultSelection;
extern byte ScenarioRosterPrepared;
extern int8 PendingCampaignStageDelta;
extern byte PendingCampaignPhaseDelta;
extern int16 ScenarioVariantIndex;
extern byte CampaignPhase;
extern byte ReplayCampaignStateRequested;
extern byte TrainingSequenceFlag;
extern word16 DeferredStage16Story;
extern word16 SavedCampaignFlags;
extern byte AlternateLanceSelection;
extern byte SuppressPostStageMenu;
extern byte CampaignRestartState;
extern word16 ModalSavedTextWindow;
extern word16 ModalBackingX;
extern word16 ModalBackingY;
extern word16 ModalBackingWidth;
extern word16 ModalBackingHeight;
extern byte ActivePilotListWindowHeight;
extern byte CommanderListWindowHeight;
extern byte FallenPilotListWindowHeight;
/* DS:1B8C-1B8E: the random pilot pool's count, in the campaign's three
 * copies of its state, as the pool has them (InitialExtendedCampaignState,
 * ForwardExtendedCampaignState and ReplayExtendedCampaignState). The
 * replay copy is the live count. */
typedef struct RandomPilotPoolCountCopies
{
	byte initial; /* DS:1B8C */
	byte forward; /* DS:1B8D */
	byte replay; /* DS:1B8E */
} RandomPilotPoolCountCopies;
_Static_assert(sizeof(RandomPilotPoolCountCopies) == 0x03,
	"DS:1B8C-1B8E holds the three copies of the count.");
extern RandomPilotPoolCountCopies RandomPilotPoolCounts;
extern word16 SaveTransferState;
extern word16 SavedSaveTransferState;
extern word16 CampaignScenarioState;
extern byte ReplayScenarioRequested;
extern byte CampaignMenuRequested;
/* DS:8432 and DS:8436 are entries 11h and 12h of the far-pointer table
 * beginning at DS:83EE.  Keep that original storage alias on the host: the
 * resource allocator and range-free procedure address the table entries,
 * while campaign presentation code uses their individual names. */
#define MissionSpriteBacking (SharedGraphicsBackingBuffers[0x11])
#define SavedMissionBacking  (SharedGraphicsBackingBuffers[0x12])
/* These addresses are fields inside the original DS tables, not independent
 * globals.  RunGameOptionsMenu changes both records for its six-row
 * standalone and seven-row embedded layouts. */
#define GameOptionsMenuHeight \
	(TextWindowContexts[0x0B].heightTiles)
#define GameOptionsMenuEntryCount \
	(UiRegionTable[0x05].rowCount)
extern word16 NarrationFocusRegion;
/* DS:1530 and DS:1566 are cached-file descriptors 40h's and 43h's name
 * pointers, which the code reads as the unit template and tactical resource
 * paths: not second objects. */
#define UnitTemplatePath (CachedFileDescriptors[0x40].fileName)
#define TacticalResourcePath (CachedFileDescriptors[0x43].fileName)
extern word16 TacticalOverviewClockNoticeActive;
extern word16 TacticalRuntimeState;
extern word16 PreviousMouseCellX;
extern word16 PreviousMouseCellY;
extern byte TacticalOverviewNoticeChance;
extern byte EjectedPilotMapX;
extern byte EjectedPilotMapY;
extern byte EjectedPilotState;
extern word16 TacticalCursorRuntimeState;
extern word16 TacticalCursorMapX;
extern word16 TacticalCursorMapY;
extern word16 LastOverviewNoticeHour;
extern word16 TacticalOverviewRuntimeState;
extern byte PrimaryLanceResourceCode;
#define SceneInstructionBase SceneInstructionBaseOffset
extern byte UnitEditorAllegiance;
extern word16 UnitEditorScenarioGroupCount;
extern word16 UnitEditorScenarioCardCount;
extern word16 UnitEditorPilotCardCount;
extern word16 UnitEditorLanceGroupCount;
/* DS:1D42 is LanceAssignments[0x0C], not a second object. */
#define UnitEditorExitButtonLeft \
	(LanceAssignments[0x0C].left)
#define UnitEditorExitButtonTop \
	(LanceAssignments[0x0C].top)
#define UnitEditorExitButtonState \
	(LanceAssignments[0x0C].style)
extern word16 TextOutputMode;
extern word16 SelectedUnitEquipmentLocation;
extern RevengeUnitRecord EstimatedUnitRecord;
/* DS:B011 and DS:B01D are the two scratch records immediately after the
 * 18 tactical-unit rating records. */
extern RevengeTacticalCombatRatings UnitCombatRatings[0x1A];
#define DisplayedUnitCombatRatings UnitCombatRatings[0x18]
#define ComparedUnitCombatRatings UnitCombatRatings[0x19]
extern byte UnitLanceFormationState[0x18];
extern byte TacticalScenarioLanceEnabled[0x03];
extern byte SensorDamageProximityRangeTable[0x03];  /* the rules' */
extern byte LanceFormationProfile[0x06];
extern byte UnitEnemyEngagementActive[0x18];
extern byte UnitMovementHeatExponent[0x18];
extern byte UnitOverburnOrCloseAssaultEnabled[0x18];
extern byte UnitFormationMode[0x18];
extern word16 UnitConsecutiveAttackTicks[0x18];
extern word16 FirstLanceOrdersEnabled;
extern byte UnitWasAttackedThisTick[0x18];
extern byte GroundTargetWasAttacked;
extern byte UnitJumpingState[0x18];
extern byte UnitJumpMovementRemaining[0x18];
extern byte UnitDestructionMarkers[0x18][0x03];
extern word16 ScenarioFileSuffix;
extern byte CampaignUpgradeAwards[0x04];
extern byte UnitEnemyInProximityRange[0x18];
extern byte UnitShutdownState[0x18];
extern byte MovementRatingByWalkMP[0x09];  /* the rules' */
extern const char *UnitNameTable[0x59];
extern const char *PilotSurnameTable[0x3B];
extern const char *UnitAffiliationNameTable[0x04];
extern const char *PilotExperienceNameTable[0x04];
extern const char *HeatScaleDescriptionTable[0x07];
extern const char *MovementDescriptionTable[0x05];
extern const char *DurabilityDescriptionTable[0x04];
extern const char *ConditionDescriptionTable[0x05];
extern const char *FriendlyEquipmentDescriptionTable[0x02];
extern const char *EnemyAssessmentTextTable[0x12];
#define NarrationMessageData (LoadedSceneFile.raw)
extern byte UnitEnemyVisible[0x18];
extern RevengeUnitSlot UnitTacticalTarget[0x19];
extern word16 EnemyCycleUnitIndex;
extern byte UnitEnemyContactEstablished[0x18];
extern byte UnitWithdrawalState[0x18];
/* DS:884B, DS:884E and DS:8853: the withdrawal states of the roster's units
 * 0Ch, 0Fh and 14h, which a later scenario's rosters test for withdrawal. */
#define CampaignUnitThirteenAvailabilityFlag (UnitWithdrawalState[0x0C])
#define CampaignUnitFifteenOverrideFlag (UnitWithdrawalState[0x0F])
#define CampaignUnitFourteenAvailabilityFlag (UnitWithdrawalState[0x14])
extern RevengeUnitSlot UnitLastNarratedTarget[0x18];
extern byte UnitPreferredEngagementMode[0x18];
extern byte UnitTerrainType[0x18];
/* RecalculateUnitCombatRatings uses slot 18h as the status-panel scratch
 * record in addition to the 18h live tactical unit slots. */
extern byte UnitBelowHalfDurability[TACTICAL_UNIT_COUNT + 0x01];
extern byte UnitCachedWeaponCounts[
	(TACTICAL_UNIT_COUNT + 0x01) * WEAPON_DEFINITION_COUNT];
extern byte UnitWeaponRecoveryCounters[
	(TACTICAL_UNIT_COUNT + 0x01) * WEAPON_DEFINITION_COUNT];
extern byte UnitHeatSinkCount[TACTICAL_UNIT_COUNT + 0x01];
extern uint32 UnitWeaponAvailabilityMask[TACTICAL_UNIT_COUNT + 0x01];
extern word16 WeaponCountScratch[0x17];
extern byte *TerrainTypeMap;
extern byte *TacticalOverviewMapBuffer;
extern byte *PackedTerrainDamageMap;
extern byte *ActiveTerrainDamageByte;
extern byte *TerrainDamageCapabilityTable;
extern const byte TerrainDamageCellIncrementMask[0x04];
extern const byte TerrainDamageExtractMask[0x04];
extern const byte TerrainDamageShift[0x04];
extern const byte TerrainDamageCellOccupiedMask[0x04];
extern const byte TerrainDamageStageIncrement[0x04];
extern word16 PackedTerrainDamageRowBytes;
extern byte DestructibleTerrainTypeBase;
extern byte TerrainImpactAnimationTypeMinimum;
extern byte TerrainImpactAnimationTypeMaximum;
extern word16 TerrainLineOfSightBlockingThreshold;
extern byte TerrainLineOfSightMaximumSteps;
extern byte TacticalTerrainEffectMode;
extern RevengeTerrainDamageQueueEntry
	TerrainDamageQueue[TERRAIN_DAMAGE_QUEUE_ENTRY_COUNT];
extern byte CampaignUpgradeMarkerX[0x04];
extern byte CampaignUpgradeMarkerY[0x04];
extern word16 FriendlyUnitDestroyed;
extern byte ScenarioDestructionCompletionState;
extern byte ScenarioDestructionCompletionMarker;
extern word16 ProtectUnitZero;
extern word16 ImpassableTerrainTypeMinimum;
extern int8_t TerrainToHitModifier[0x100];

extern byte WeaponAIHeatLimitByExperience[0x05];  /* the rules' */
extern const byte SpriteFacingByDirection[0x08];
extern byte EngagementModeByComparisonMask[0x100];  /* the rules' */
extern RevengeWeaponPriorityData WeaponPriorityData;  /* the rules' */
extern word16 WeaponPriorityCountTable[WEAPON_PRIORITY_PROFILE_COUNT];  /* the rules' */
extern DosAddress WeaponPriorityListPointers[WEAPON_PRIORITY_PROFILE_COUNT];  /* the rules' */
extern byte WeaponPriorityProfileByRangeHeat[0x80];  /* the rules' */
extern word16 MovementTimingMaskTable[0x09];  /* the rules' */
/* Forty formations of three x and three y offsets: a facing band (0, 0Ah,
 * 14h, 1Eh) plus one of the ten formation modes. SaveDirectory follows. */
extern int8 FormationOffsetTable[0xF0];  /* the rules' */
extern byte FormationSelectionAggressive[0x0C];  /* the rules' */
extern byte FormationSelectionDefensive[0x0C];  /* the rules' */
extern byte FormationSelectionBalanced[0x0C];  /* the rules' */
extern int16 FormationMarkerOriginX[0x0A];
extern int16 FormationMarkerOriginY[0x0A];
extern int16 FacingVectorStartX[0x10];
extern int16 FacingVectorStartY[0x10];
extern int16 FacingVectorEndX[0x10];
extern int16 FacingVectorEndY[0x10];
extern int16 MovementVectorX[0x20];
extern int16 MovementVectorY[0x20];
extern const int16 DirectionDeltaXTable[0x08];
extern const int16 DirectionDeltaYTable[0x08];
extern int16 NeighborMapOffsetTable[0x08];
extern word16 DirectionSearchMirror;
extern int16 DirectionSearchCountdown;
extern const int16 DirectionSearchDeltaTable[0x10];
/* DS:3D1D: the Improved Missiles damage bonus of each missile weapon, from
 * WEAPON_LRM_5 on. Its first byte is HitLocationDirectionSelectorTable's
 * last. */
extern byte ImprovedMissileDamageBonusTable[0x07];  /* the rules' */
extern const byte TerrainDamageQueueOccupiedMask[0x04];
extern const byte TerrainDamageQueueIncrementMask[0x04];
extern word16 HitLocationDirectionSelectorTable[0x10];  /* the rules' */
extern word16 HitLocationTable[HIT_LOCATION_TABLE_COUNT]
	[HIT_LOCATION_TABLE_WORD_COUNT];  /* the rules' */
extern word16 UnitActionMaskByExperience[0x04];  /* the rules' */
extern RevengeTerrainImpactAnimationRecord
	TerrainImpactAnimationRecords[TERRAIN_IMPACT_ANIMATION_RECORD_COUNT];
extern word16 NarrationMessageCategory;
extern word16 PendingNarrationDelay;
extern word16 ActiveNarrationSubject;
extern word16 NarrationEmphasis;
extern byte DamagedWeaponRangeFlags[0x03];
extern int16 CurrentUnitDurabilityTotal;
extern int16 MaximumUnitDurabilityTotal;
extern char SharedNumericTextBuffer[0x20];
extern const char PercentageSuffixText[];
extern word16 TacticalUpdateDelayTicks;
extern word16 TacticalKeyRepeatDelayTicks;
extern word16 TacticalUpdateDelayReloadTicks;
extern word16 TacticalGameSpeedIndex;
extern word16 TacticalBattleResult;
extern word16 TacticalClockDivider;
extern word16 TacticalImpactQueueCount;
extern RevengeTacticalImpactRecord
	TacticalImpactQueue[TACTICAL_IMPACT_RECORD_COUNT];
extern const int16 TacticalCursorSearchOffsetX[0x09];
extern const int16 TacticalCursorSearchOffsetY[0x09];
extern const int8 TacticalMouseEdgeInputByMask[0x10];
extern byte SceneNarrationEventFlags[0x06];
/* DS:176A: 30h words, one for each heat-sink count a unit's 2Fh equipment
 * slots allow. */
extern word16 HeatDissipationBySinkCountQ8_8[0x30];  /* the rules' */
extern word16 TerrainDamageQueueWordCount;
extern word16 TacticalProjectileCount;
extern RevengeTacticalProjectileRecord
	TacticalProjectileRecords[TACTICAL_PROJECTILE_RECORD_COUNT];
extern word16 TacticalOverviewDisplayMode;
extern word16 LrmShipmentLoaded;
extern word16 SrmShipmentLoaded;
extern word16 TacticalOverviewMapMode;
extern word16 NarrationSourceUnit;
extern word16 LastAttackingUnitIndex;
extern word16 PendingMouseButtonEvent;
extern int16 MouseCursorTestX;
extern int16 MouseCursorTestY;
extern word16 MouseCursorUpdateLock;
extern int16 MouseCursorRequestedX;
extern word16 MouseCursorHideDepth;
extern word16 MouseDriverDoubleX;
extern word16 MouseDriverInstalled;
extern word16 MouseButtonState;
extern word16 MouseY;
extern word16 MouseInputDisabled;
extern int16 MouseCursorMinX;
extern int16 MouseCursorMaxX;
extern int16 MouseCursorMinY;
extern int16 MouseCursorMaxY;
extern int16 MouseCursorHotspotX;
extern int16 MouseCursorHotspotY;
extern int16 MouseCursorDisplayedX;
extern int16 MouseCursorDisplayedY;
extern byte *MouseCursorBackground;
extern const byte *MouseCursorBitmap;
extern int16 MouseCursorErasedX;
extern int16 MouseCursorErasedY;
extern int16 MouseCursorRestoreTileLeft;
extern int16 MouseCursorRestoreTileTop;
extern int16 MouseCursorRestoreTileRight;
extern int16 MouseCursorRestoreTileBottom;
extern int16 MouseCursorDrawnX;
extern int16 MouseCursorDrawnY;
extern int16 MouseCursorCaptureTileLeft;
extern int16 MouseCursorCaptureTileTop;
extern int16 MouseCursorCaptureTileRight;
extern int16 MouseCursorCaptureTileBottom;
extern int16 MouseCursorDrawLeft;
extern int16 MouseCursorDrawTop;
extern byte *ActiveRenderTarget;
extern word16 SpriteClipLeftTileMirror;
extern word16 SpriteClipRightTileMirror;
extern word16 SpriteClipTopPixelMirror;
extern word16 SpriteClipBottomPixelMirror;
extern word16 TacticalSpriteClipLeftTile;
extern word16 TacticalSpriteClipRightTile;
extern word16 TacticalSpriteClipTopPixel;
extern word16 TacticalSpriteClipBottomPixel;
extern word16 RequestedMusicEnabled;
extern word16 CachedMusicResourceIndex;
extern word16 SpriteDrawBusy;
extern word16 PlanarBackingBufferExtentLow;
extern word16 PlanarBackingBufferExtentHigh;
extern byte SceneTransitionAbortFlag;
extern word16 PresentationDismissTimeout;
extern word16 PresentationCallbackOffset;
extern word16 PresentationCallbackSegment;
extern const char *const RestartConfirmationText;
extern const char *const MissionMapHeaderText;
extern char TextEntryBuffer[0x16];
extern word16 TextEntryCursorIndex;
extern byte TextEntryActive;
extern const char TextEntryPunctuation[];
extern const char TextCursorBlinkGlyph[];
extern const byte CharacterClassTable[0x100];
extern const word16 VideoPixelAlignmentMasks[0x08];
extern const word16 PackedPixelShiftCounts[0x03];
extern word16 TextWindowFrameDefinition[0x0C];
extern const word16 *const TextWindowFrameDefinitionTable[0x01];
extern const byte MonthLengthTable[0x0C];
/* DS:716E: the days before each month of a common year, and the year's. */
extern const word16 CalendarBoundaryTable[0x0D];
extern char StandardTimezoneName[0x04];
extern char DaylightTimezoneName[0x04];
extern word16 TimezoneOffsetLowWord;
extern word16 TimezoneOffsetHighWord;
extern word16 DaylightSavingEnabled;
extern const char TimezoneEnvironmentName[0x03];
extern const char DefaultStandardTimezoneName[0x04];
extern const char DefaultDaylightTimezoneName[0x04];
extern word16 TacticalOverviewActive;
extern byte ArtilleryShellingActive;
extern byte RollingBarrageActive;
extern byte StrafingRunActive;
extern byte ArtilleryTargetMapX;
extern byte ArtilleryTargetMapY;
extern byte RollingBarrageEndMapX;
extern byte RollingBarrageEndMapY;
extern byte RollingBarrageStartMapX;
extern byte RollingBarrageStartMapY;
extern byte DropShipMapX;
extern byte DropShipMapY;
extern byte ArtillerySupportAvailable;
extern byte StrafingSupportAvailable;
extern word16 TacticalOverviewViewportDirty;
extern word16 *TacticalCellTerrainMap;
extern word16 *TacticalCellSpriteMap;
extern byte ScenarioUnitNeedsUpdate[0x18];
extern byte TacticalStateInitialized;
extern word16 SuppressCampaignClock;
extern word16 TacticalCursorOutlineEnabled;
extern word16 TacticalCursorOutlineState;
#define TacticalEffectSpriteTable \
	((const byte * const *)SharedGraphicsBackingBuffers)
extern byte *ActiveFontResource;
extern byte MusicDriverIndex;
extern int8 ActiveFontGlyphAdvanceX;
extern int8 ActiveFontLineHeight;
extern byte *TacticalTileStrip;
extern int16 BusyWaitLoopCalibration;
extern word16 ActiveUiHighlightXorColor;
extern word16 ActiveUiHighlightHeight;
extern word16 ActiveUiHighlightWidth;
extern word16 ActiveUiHighlightLeft;
extern word16 ActiveUiHighlightTop;
extern byte *PrimaryScreenBuffer;
extern byte SavedUiRegionSelectionRows[ORIGINAL_UI_REGION_COUNT];
extern int16 ActiveUiRegionIndex;
extern word16 ExpandFirstUiRowHighlight;
extern word16 VideoAdapterType;
extern char TacticalMapFileName[0x09];
extern char TacticalIconSetFileName[0x0D];
extern char TacticalDestroyIconFileName[0x0D];
extern char TacticalIconListFileName[0x0D];
extern char TacticalMechSetFileName[0x0D];
extern word16 ResourceRetryCount;
extern word16 ReservedConventionalMemoryKB;
extern word16 ExitHandlerCount;
extern OriginalExitHandler ExitHandlerTable[0x20];
extern OriginalExitHandler RuntimeExitHook1;
extern OriginalExitHandler RuntimeExitHook2;
extern OriginalExitHandler RuntimeExitHook3;
extern OriginalExitHandler RuntimeTerminationHook;
extern DOS_InterruptHandler SavedDivideErrorHandler;
extern DOS_InterruptHandler SavedOverflowHandler;
extern DOS_InterruptHandler SavedBoundsHandler;
extern DOS_InterruptHandler SavedInvalidOpcodeHandler;
extern ResourceErrorCallback ResourceErrorDisplayCallback;
extern ResourceErrorCallback ResourceErrorDismissCallback;
extern const char *const ResourceCreateFileErrorText;
extern const char *const ResourceIllegalOpenModeErrorText;
extern const char *const ResourceInsertDiskErrorText;
extern const char *const ResourceTooManyOpenFilesErrorText;
extern const char *const ResourceCloseUnopenedErrorText;
extern const char *const ResourceReadUnopenedErrorText;
extern const char *const ResourceWriteUnopenedErrorText;
extern const char *const ResourceSeekUnopenedErrorText;
extern const char *const ResourceIllegalSeekOriginErrorText;
extern const char *const ResourceWriteCachedErrorText;
extern const char *const ResourceUnknownDriveErrorText;
extern const char *const ResourceUnknownCacheIndexErrorText;
extern const char *const ResourceOpenFilesAtExitErrorText;
extern const char *const ResourceFatalDiskErrorText;


/* Original shared graphics data segment (4184) fields. */
extern word16 ActiveVideoAdapterType;
extern word16 EncodedSpriteClipLeftTile;
extern word16 SpriteClipRightTile;
extern word16 SpriteClipTopPixel;
extern word16 EncodedSpriteClipBottomPixel;
extern word16 SpritePreserveMode;
extern byte WeaponCombatRatingByType[0x17];         /* the rules' */
extern const byte RelativeAttackDirectionByMask[0x10];
extern const int16 LinePrimaryYStep[0x08];
extern const int16 LinePrimaryXStep[0x08];
extern int16 LinePrimaryRowOffset[0x08];
extern const int16 LineSecondaryYStep[0x08];
extern const int16 LineSecondaryXStep[0x08];
extern int16 LineSecondaryRowOffset[0x08];
extern word16 EncodedSpriteVisibleWidth;
extern word16 EncodedSpriteSourceWidth;
extern byte EncodedSpriteHorizontalPhase;
extern word16 EncodedSpriteClipWidthTiles;

#endif
