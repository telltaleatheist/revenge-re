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
typedef uint16_t (*MediaConfigurationHandler)(void);

#pragma pack(push, 1)
typedef struct MediaConfigurationChoice
{
    char name[0x0C];
    uint8_t value;
} MediaConfigurationChoice;

typedef struct SoundConfigurationChoice
{
    char name[0x0C];
    uint8_t driverIndex;
    uint8_t hasNumericParameter;
} SoundConfigurationChoice;
#pragma pack(pop)

static_assert(sizeof(MediaConfigurationChoice) == 0x0D,
    "MediaConfigurationChoice must retain its DOS layout");
static_assert(sizeof(SoundConfigurationChoice) == 0x0E,
    "SoundConfigurationChoice must retain its DOS layout");

#pragma pack(push, 1)
typedef struct PITCalibrationState
{
    uint16_t Channel0Divisor;
    DosAddress SavedIRQ0Vector;
    uint16_t SavedInterruptFlags;
    uint16_t LatchedChannel0Count;
} PITCalibrationState;
#pragma pack(pop)

static_assert(sizeof(PITCalibrationState) == 0x0A,
    "PITCalibrationState must retain DS:6FA4-6FAD layout");
static_assert(offsetof(PITCalibrationState, SavedIRQ0Vector) == 0x02,
    "saved IRQ0 vector must retain its original offset");

/* Original primary data segment (3A6A) fields, named by DS offset. */
extern uint16_t MapWidth;
extern uint16_t MapHeight;
extern uint8_t VideoPathUnavailable;
extern uint16_t SavedBIOSVideoMode;
extern const uint16_t BIOSVideoModeByAdapter[0x07];
extern uint16_t GameInitialized;
extern uint16_t StartupResourceState;
extern uint8_t *InvalidSaveDialogBacking;
extern uint16_t NumericKeypadNavigationEnabled;
extern uint16_t StartupInputMode;
extern uint16_t SoundEffectsEnabled;
extern uint16_t DigitalSoundBusy;
extern uint16_t DigitalPlaybackRateHz;
extern uint16_t AppliedMusicEnabled;
extern uint16_t SoundDriverNumericParameter;
extern uint16_t InputDeviceType;
extern int8_t SoundDriverIndex;
extern uint8_t *Font6Resource;
extern uint8_t *Font8Resource;
extern uint8_t *SecondaryScreenBuffer;
extern uint8_t *PlanarScreenBuffer;
extern uint16_t MouseCursorBackingSourceTileX;
extern uint16_t MouseCursorBackingSourcePixelY;
extern uint16_t MouseCursorBackingWidthTiles;
extern uint16_t MouseCursorBackingHeightPixels;
extern uint8_t *SharedGraphicsBackingBuffers[0x26];
extern uint8_t *VideoFramebuffer;
extern uint16_t ResourceRuntimeInitialized;
extern uint8_t VideoPaletteData[0x0310];
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
extern uint8_t CompressedTextFirstByteTable[0x10];
extern uint8_t CompressedTextSecondByteTable[0x80];
extern uint16_t CgaNarrowStrideMode;
extern uint16_t UiSelectionResetFlagA;
extern uint16_t UiSelectionResetFlagB;
extern uint16_t ActiveTextWindowLeftTile;
extern uint16_t ActiveTextWindowTopTile;
extern TextWindowContext TextWindowContexts[ORIGINAL_TEXT_WINDOW_COUNT];
extern uint16_t ActiveTextWindowIndex;
extern uint16_t ActiveTextWindowWidthTiles;
extern uint16_t ActiveTextWindowHeightTiles;
extern uint16_t ActiveTextForegroundColor;
extern uint16_t ActiveTextBackgroundColor;
extern uint16_t ActiveTextCursorColumn;
extern uint16_t ActiveTextCursorRow;
extern uint16_t TextWindowStateInitialized;
extern uint16_t UiNavigationUpKey;
extern uint16_t UiNavigationDownKey;
extern uint16_t UiPreviousRegionKey;
extern uint16_t UiNextRegionKey;
extern PITCalibrationState PITCalibration;
extern uint16_t VerticalRetraceEdgeMode;
extern uint8_t *DamagedTerrainSpriteLookup;
extern uint8_t TacticalEffectOverlayMap[0x00E4];
extern uint32_t GameTimerTicks;
extern uint32_t GameAnimationTicks;
extern const char CampaignClockSeparatorText[];
extern const char FormattedLineBreak[];
extern const char PathSeparatorText[];
extern const char PercentCloseText[];
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
extern uint16_t NarrationFocusRegion;
/* DS:1530 and DS:1566 are cached-file descriptors 40h's and 43h's name
 * pointers, which the code reads as the unit template and tactical resource
 * paths: not second objects. */
#define UnitTemplatePath     (CachedFileDescriptors[0x40].fileName)
#define TacticalResourcePath (CachedFileDescriptors[0x43].fileName)
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
#define SceneInstructionBase SceneInstructionBaseOffset
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
extern uint8_t MovementRatingByWalkMP[0x09];  /* the rules' */
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

extern uint8_t WeaponAIHeatLimitByExperience[0x05];  /* the rules' */
extern const uint8_t SpriteFacingByDirection[0x08];
extern uint8_t EngagementModeByComparisonMask[0x100];  /* the rules' */
extern RevengeWeaponPriorityData WeaponPriorityData;  /* the rules' */
extern uint16_t WeaponPriorityCountTable[WEAPON_PRIORITY_PROFILE_COUNT];  /* the rules' */
extern DosAddress WeaponPriorityListPointers[WEAPON_PRIORITY_PROFILE_COUNT];  /* the rules' */
extern uint8_t WeaponPriorityProfileByRangeHeat[0x80];  /* the rules' */
extern uint16_t MovementTimingMaskTable[0x09];  /* the rules' */
/* Forty formations of three x and three y offsets: a facing band (0, 0Ah,
 * 14h, 1Eh) plus one of the ten formation modes. SaveDirectory follows. */
extern int8_t FormationOffsetTable[0xF0];  /* the rules' */
extern uint8_t FormationSelectionAggressive[0x0C];  /* the rules' */
extern uint8_t FormationSelectionDefensive[0x0C];  /* the rules' */
extern uint8_t FormationSelectionBalanced[0x0C];  /* the rules' */
extern int16_t FormationMarkerOriginX[0x0A];
extern int16_t FormationMarkerOriginY[0x0A];
extern int16_t FacingVectorStartX[0x10];
extern int16_t FacingVectorStartY[0x10];
extern int16_t FacingVectorEndX[0x10];
extern int16_t FacingVectorEndY[0x10];
extern int16_t MovementVectorX[0x20];
extern int16_t MovementVectorY[0x20];
extern const int16_t DirectionDeltaXTable[0x08];
extern const int16_t DirectionDeltaYTable[0x08];
extern int16_t NeighborMapOffsetTable[0x08];
extern uint16_t DirectionSearchMirror;
extern int16_t DirectionSearchCountdown;
extern const int16_t DirectionSearchDeltaTable[0x10];
/* DS:3D1D: the Improved Missiles damage bonus of each missile weapon, from
 * WEAPON_LRM_5 on. Its first byte is HitLocationDirectionSelectorTable's
 * last. */
extern uint8_t ImprovedMissileDamageBonusTable[0x07];  /* the rules' */
extern const uint8_t TerrainDamageQueueOccupiedMask[0x04];
extern const uint8_t TerrainDamageQueueIncrementMask[0x04];
extern uint16_t HitLocationDirectionSelectorTable[0x10];  /* the rules' */
extern uint16_t HitLocationTable[HIT_LOCATION_TABLE_COUNT]
                                [HIT_LOCATION_TABLE_WORD_COUNT];  /* the rules' */
extern uint16_t UnitActionMaskByExperience[0x04];  /* the rules' */
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
/* How long a narration stays up, in ms of battle time (the game clock's
 * ticks while the battle loop runs). The original counted it in battle-loop
 * passes: 25Dh (605) of them, so its length, and whether its prompt still
 * opened, followed the machine's speed. On the reference machine (DOSBox-X,
 * 3000 cycles) a pass takes 33.06 ms on average: 70,580 passes over nine
 * battle stretches of stages 3 to 7, 31 to 40 ms by stretch. 605 passes are
 * 20,000 ms. */
#define NARRATION_DELAY_MS 20000
/* The narration's sound came on the pass at which five had gone by since
 * the set (the test for 258h): five passes, 165 ms. */
#define NARRATION_SOUND_AFTER_MS 165
/* One tick of the game clock (GameTimerTicks, 18.2 a second), in whole ms. */
#define GAME_CLOCK_TICK_MS 55
extern uint16_t PendingNarrationDelay;
extern uint16_t ActiveNarrationSubject;
extern uint16_t NarrationEmphasis;
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
extern const char *const RestartConfirmationText;
extern const char *const MissionMapHeaderText;
extern char TextEntryBuffer[0x16];
extern uint16_t TextEntryCursorIndex;
extern uint8_t TextEntryActive;
extern const char TextEntryPunctuation[];
extern const char TextCursorBlinkGlyph[];
extern const uint8_t CharacterClassTable[0x100];
extern const uint16_t VideoPixelAlignmentMasks[0x08];
extern const uint16_t PackedPixelShiftCounts[0x03];
extern uint16_t TextWindowFrameDefinition[0x0C];
extern const uint16_t *const TextWindowFrameDefinitionTable[0x01];
extern const uint8_t MonthLengthTable[0x0C];
/* DS:716E: the days before each month of a common year, and the year's. */
extern const uint16_t CalendarBoundaryTable[0x0D];
extern char StandardTimezoneName[0x04];
extern char DaylightTimezoneName[0x04];
extern uint16_t TimezoneOffsetLowWord;
extern uint16_t TimezoneOffsetHighWord;
extern uint16_t DaylightSavingEnabled;
extern const char TimezoneEnvironmentName[0x03];
extern const char DefaultStandardTimezoneName[0x04];
extern const char DefaultDaylightTimezoneName[0x04];
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
#define TacticalEffectSpriteTable \
    ((const uint8_t *const *) SharedGraphicsBackingBuffers)
extern uint8_t *ActiveFontResource;
extern uint8_t MusicDriverIndex;
extern int8_t ActiveFontGlyphAdvanceX;
extern int8_t ActiveFontLineHeight;
extern uint8_t *TacticalTileStrip;
extern int16_t BusyWaitLoopCalibration;
extern uint16_t ActiveUiHighlightXorColor;
extern uint16_t ActiveUiHighlightHeight;
extern uint16_t ActiveUiHighlightWidth;
extern uint16_t ActiveUiHighlightLeft;
extern uint16_t ActiveUiHighlightTop;
extern uint8_t *PrimaryScreenBuffer;
extern uint8_t SavedUiRegionSelectionRows[ORIGINAL_UI_REGION_COUNT];
extern int16_t ActiveUiRegionIndex;
extern uint16_t ExpandFirstUiRowHighlight;
extern uint16_t VideoAdapterType;
extern char TacticalMapFileName[0x09];
extern char TacticalIconSetFileName[0x0D];
extern char TacticalDestroyIconFileName[0x0D];
extern char TacticalIconListFileName[0x0D];
extern char TacticalMechSetFileName[0x0D];
extern uint16_t ResourceRetryCount;
extern uint16_t ReservedConventionalMemoryKB;
extern uint16_t ExitHandlerCount;
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
extern uint16_t ActiveVideoAdapterType;
extern uint16_t EncodedSpriteClipLeftTile;
extern uint16_t SpriteClipRightTile;
extern uint16_t SpriteClipTopPixel;
extern uint16_t EncodedSpriteClipBottomPixel;
extern uint16_t SpritePreserveMode;
extern uint8_t WeaponCombatRatingByType[0x17];         /* the rules' */
extern const uint8_t RelativeAttackDirectionByMask[0x10];
extern const int16_t LinePrimaryYStep[0x08];
extern const int16_t LinePrimaryXStep[0x08];
extern int16_t LinePrimaryRowOffset[0x08];
extern const int16_t LineSecondaryYStep[0x08];
extern const int16_t LineSecondaryXStep[0x08];
extern int16_t LineSecondaryRowOffset[0x08];
extern uint16_t EncodedSpriteVisibleWidth;
extern uint16_t EncodedSpriteSourceWidth;
extern uint8_t EncodedSpriteHorizontalPhase;
extern uint16_t EncodedSpriteClipWidthTiles;

#endif
