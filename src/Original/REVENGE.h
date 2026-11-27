#ifndef REVENGE_ORIGINAL_H
#define REVENGE_ORIGINAL_H

/*
 * Shared declarations for the preservation transcription of REVENGE.EXE.
 *
 * Only contracts supported by a Verified METHOD_TABLE entry belong here.
 * Reko's generated Eq_* type graph is intentionally not imported: those
 * names describe inference state, not types established by the executable.
 */

#include "revenge/dos.h"
#include "revenge/heap.h"
#include "revenge/types.h"
#include "revenge/unit.h"

#include <stddef.h>

#pragma pack(push, 1)
typedef struct LocalClockSnapshot
{
    uint32_t epochSeconds;
    uint16_t milliseconds;
    int16_t timezoneOffsetMinutes;
    uint16_t daylightSavingActive;
} LocalClockSnapshot;
#pragma pack(pop)

static_assert(sizeof(LocalClockSnapshot) == 0x0A,
    "LocalClockSnapshot must retain its DOS layout");

typedef struct DigitalSampleDescriptor DigitalSampleDescriptor;
typedef struct DigitalDriverQueue DigitalDriverQueue;

/* A queue's list (375F:0868, 08C9): each descriptor starts with its link,
 * and the queue's end is a link of its own. The original kept each as two far
 * pointers: the next descriptor, and the link pointing here (the queue's
 * first, or the previous descriptor's next). */
typedef struct DigitalSampleLink
{
    struct DigitalSampleLink *Next;
    struct DigitalSampleLink **PreviousLink;
} DigitalSampleLink;

/* A descriptor's completion (its +2C far pointer): 31FB:06E9 for a weapon
 * effect, else the driver's own 375F:0AE5, which does nothing (NULL). */
typedef void (*DigitalSampleCompletion)(DigitalSampleDescriptor *descriptor);

/* The original's descriptor is 34h bytes; each far pointer here is a
 * reference, at the offset the comment gives. */
struct DigitalSampleDescriptor
{
    DigitalSampleLink Link;                 /* +00 next, +04 previous link */
    DigitalDriverQueue *OwnerQueue;         /* +08 */
    const uint8_t *SampleData;                 /* +0C */
    uint16_t SampleLength;
    uint16_t LoopStart;
    uint16_t LoopEnd;
    uint16_t LoopLength;
    uint16_t SourceRateHz;
    uint16_t PlaybackScale;
    uint16_t SourceStepFraction;
    uint16_t SourceStepWhole;
    uint16_t PlaybackHandlerOffset;
    uint16_t SourceCursorFraction;
    /* +24/+26: the cursor's offset and segment. Here, bytes from SampleData:
	 * the original's offset began at the sample's own (normalized, under
	 * 10h), so the two wrap alike for any sample under FFF0h bytes. */
    uint16_t SourceCursor;
    uint16_t RemainingFraction;
    uint16_t RemainingWhole;
    DigitalSampleCompletion Completion;     /* +2C */
    DigitalSampleDescriptor *CompletionArgument; /* +30 */
};

/* The original's queue is 12h bytes: its first link at +00, the end link at
 * +04 (the first points there while the queue is empty). */
struct DigitalDriverQueue
{
    DigitalSampleLink *First;
    DigitalSampleLink End;
    uint16_t ActiveDescriptorCount;
    uint16_t MixBias;
    uint8_t OutputSample;
    uint8_t ReservedOutputByte;
};

/* The CS:04BC-05D5 digital playback timer/mixer state. CS:04BC, the IRQ0
 * vector 38AE:0010 saved, has no field: the rebuild never takes the vector
 * (375F:0A1D is Replaced). */
typedef struct DigitalPlaybackRuntimeState
{
    uint16_t SystemTimerDivisor;
    uint16_t SystemTimerAccumulator;
    uint16_t InterruptNestingGuard;
    uint16_t DigitalTimerReloadMinusOne;
    uint16_t PlaybackRateHz;
    uint16_t WriteSampleCallbackOffset;
    DigitalDriverQueue *ActiveDriverQueue;  /* CS:04CE */
    uint16_t RequestedMixScale;
    uint16_t DriverMixScale;
    uint16_t ActiveMixScale;
    uint8_t MixLookup[0x0100];
} DigitalPlaybackRuntimeState;

/* The DS:6F4C-6FA3 queues and calibration descriptor (the second queue at
 * DS:6F5E, the descriptor at DS:6F70). The rebuild has no calibration
 * (375F:05D6): its queue and descriptor stay as places in the original's
 * data, unused. */
typedef struct DigitalPlaybackWorkingSet
{
    DigitalDriverQueue PlaybackTimerQueue;
    DigitalDriverQueue CalibrationQueue;
    DigitalSampleDescriptor CalibrationSample;
} DigitalPlaybackWorkingSet;

#pragma pack(push, 1)

typedef struct DigitalPlaybackDescriptor
{
    uint16_t HardwareDescriptorOffset;
    uint16_t AcquireCallbackOffset;
    uint16_t ReleaseCallbackOffset;
    uint16_t WriteSampleCallbackOffset;
    uint16_t MixScale;
    uint16_t ReferenceCount;
} DigitalPlaybackDescriptor;

typedef struct DigitalWeaponEffectRecord
{
    uint16_t ResourceIndex;
    uint16_t PlaybackScale;
    uint16_t PackedBufferOffset;
    uint16_t SampleLength;
    uint16_t LoadState;
} DigitalWeaponEffectRecord;

typedef struct DigitalSoundEntryRecord
{
    uint8_t SoundCode;
    uint16_t ResourceIndex;
    uint32_t SourceFileOffset;
    uint16_t SampleLength;
    uint16_t PackedBufferOffset;
} DigitalSoundEntryRecord;

typedef struct DigitalSoundDataLayout
{
    uint16_t DriverDescriptorOffsets[0x09];
    /* DS:5950: the original's 0Fh sample descriptors, 34h bytes each. The
	 * port's are DigitalSamplePlaybackDescriptorTable, typed; these bytes
	 * stay as the story's media gives them. */
    uint8_t SampleDescriptorPlaces[0x0F][0x34];
    DigitalWeaponEffectRecord WeaponEffects[0x10];
    DigitalSoundEntryRecord SoundEntries[0x16];
    uint8_t ScenarioCodeStrings[0x017D];
} DigitalSoundDataLayout;
#pragma pack(pop)

static_assert(sizeof(DigitalPlaybackDescriptor) == 0x0C,
    "DigitalPlaybackDescriptor must retain its DOS layout");
static_assert(sizeof(DigitalWeaponEffectRecord) == 0x0A,
    "DigitalWeaponEffectRecord must retain its DOS layout");
static_assert(sizeof(DigitalSoundEntryRecord) == 0x0B,
    "DigitalSoundEntryRecord must retain its DOS layout");
static_assert(offsetof(DigitalSoundDataLayout, SampleDescriptorPlaces) == 0x0012,
    "digital sample descriptors must begin at DS:5950");
static_assert(offsetof(DigitalSoundDataLayout, WeaponEffects) == 0x031E,
    "weapon effects must begin at DS:5C5C");
static_assert(offsetof(DigitalSoundDataLayout, SoundEntries) == 0x03BE,
    "digital sound entries must begin at DS:5CFC");
static_assert(offsetof(DigitalSoundDataLayout, ScenarioCodeStrings) == 0x04B0,
    "scenario sound strings must begin at DS:5DEE");
static_assert(sizeof(DigitalSoundDataLayout) == 0x062D,
    "digital sound data must retain DS:593E-5F6A layout");

typedef union DigitalSoundDataImage
{
    DigitalSoundDataLayout Layout;
    uint8_t Bytes[0x062D];
} DigitalSoundDataImage;

extern DigitalPlaybackDescriptor NullDigitalPlayback;
extern DigitalPlaybackDescriptor SoundBlasterPlayback;
extern DigitalPlaybackRuntimeState DigitalPlaybackRuntime;
extern DigitalPlaybackWorkingSet DigitalPlaybackBuffers;

enum
{
    DIGITAL_SOUND_DATA_FIRST_OFFSET = 0x593E,
    DIGITAL_SOUND_DATA_END_OFFSET = 0x5F6B,
    DIGITAL_SOUND_DATA_BYTE_COUNT =
        DIGITAL_SOUND_DATA_END_OFFSET - DIGITAL_SOUND_DATA_FIRST_OFFSET
};

extern DigitalSoundDataImage DigitalSoundData;
extern DigitalSampleDescriptor DigitalSamplePlaybackDescriptorTable[0x0F];
extern DigitalSampleDescriptor *ActiveDigitalSampleDescriptor;
extern DigitalDriverQueue ActiveDigitalDriverQueue;
extern uint16_t DigitalPlaybackRateHz;
extern uint8_t DigitalWeaponEffectRepeatCount;
extern uint8_t *DigitalWeaponSampleBuffer;
extern uint8_t *DigitalSoundBuffer;
extern uint8_t DigitalSoundDriverMutationGuard;
extern uint16_t NarrationMode;
extern uint16_t ActiveNarrationSubject;
extern uint16_t NarrationEmphasis;

#define LOW_BYTE(value)  ((uint8_t) ((uint32_t) (value) & 0x00FFU))
#define HIGH_BYTE(value) ((uint8_t) (((uint32_t) (value) >> 0x08) & 0x00FFU))
#define LOW_WORD(value)  ((uint16_t) ((uint32_t) (value) & 0xFFFFU))
#define HIGH_WORD(value) ((uint16_t) (((uint32_t) (value) >> 0x10) & 0xFFFFU))
#define MAKE_WORD(highByte, lowByte) \
    ((uint16_t) (((uint16_t) (uint8_t) (highByte) << 0x08) | \
        (uint8_t) (lowByte)))
#define MAKE_DWORD(highWord, lowWord) \
    ((uint32_t) (((uint32_t) (uint16_t) (highWord) << 0x10) | \
        (uint16_t) (lowWord)))
#define MAKE_FAR_POINTER(segment, offset) MAKE_DWORD((segment), (offset))

enum
{
    DIGITAL_SAMPLE_HANDLER_IDLE = 0x0A7C,
    DIGITAL_SAMPLE_HANDLER_ONESHOT = 0x0A82,
    DIGITAL_SAMPLE_HANDLER_LOOP = 0x0AAB,
    DIGITAL_DESCRIPTOR_NULL = 0x6EB2,
    DIGITAL_DESCRIPTOR_SOUND_BLASTER = 0x6EE2
};

/* Verified DOS and BIOS boundary contracts. */
void SetVideoAdapterType(uint16_t adapterType);
uint16_t GetCurrentBIOSVideoMode(void);
void SetVideoModeForAdapter(uint16_t adapterOrRestoreSlot);
void RebuildTacticalVideoResources(
    uint16_t adapterType,
    uint16_t showScenarioCompletion);
uint16_t ConfirmRestartGame(void);
uint16_t ShowConfirmationDialog(const char *message);
void ShowScenarioMessage(uint16_t messageIndex);
void ShowMissionFollowupMessage(uint16_t messageIndex);
void LoadAndPresentScreen(uint16_t resourceIndex);
uint16_t PresentScreenAndMessage(
    uint16_t screenResourceIndex,
    uint16_t messageIndex);
void DrawMissionHeaderFrame(void);
void UpdateTextEntryCursorBlink(void);
uint16_t EditTextField(
    uint16_t maximumLength,
    const char *initialText,
    uint8_t allowedCharacterClasses);
void CaptureLocalClockSnapshot(LocalClockSnapshot *snapshot);
void InvokePresentationCallback(void);
int16_t EnsureCompatibleSaveFile(
    int16_t fileHandle,
    char *savePath,
    uint16_t openMode);
void ShowInvalidSaveVersionDialog(void);
void DrawCenteredClippedText(
    const char *sourceText,
    uint16_t splitIndex,
    uint16_t leftX,
    uint16_t y,
    uint16_t maximumWidth,
    uint8_t foregroundColor,
    uint8_t backgroundColor);
void WaitVerticalRetraces(uint16_t retraceCount);
void WaitForVerticalRetraceEdge(uint8_t waitForRetraceStart);
void CalibrateVideoTiming(void);

/* Programmable-timer contracts (segment 38AE). */
uint16_t SetPITChannel0Divisor(uint16_t divisor);
uint16_t PITFrequencyToDivisor(uint16_t frequencyHz);

/* Verified input and utility contracts used across original segment files. */
uint16_t GeneratePseudoRandomByte(void);
uint16_t KeyboardInputPending(void);
uint16_t ReadRawKeypress(void);
int8_t TranslateKeyCode(void);
int16_t ReadTranslatedKeypress(void);
int16_t NormalizeNavigationKeyCode(int16_t keyCode);
int16_t RandomInRangeInclusive(uint16_t minimum, uint16_t maximum);
uint8_t *AdvanceFarPointerBy32K(uint8_t *pointer);
uint16_t EncodeDisplayBackingRows(
    const uint8_t *source,
    uint8_t *destination,
    uint16_t groupsPerRow,
    uint8_t rowCount,
    int16_t sourceRowSkip);
void BuildTacticalOverviewMapBuffer(
    const uint8_t *source,
    uint8_t *destination,
    uint16_t width,
    uint16_t height);
void InstallDamagedTerrainSpriteLookup(void);
void ClearTacticalEffectOverlayMap(void);
void ClearActiveVideoBuffer(void);
void DrawVerticalSpan(
    uint16_t x,
    uint16_t yStart,
    uint16_t yEnd,
    uint16_t color);
void DrawHorizontalVideoRun(
    int16_t x,
    uint16_t y,
    uint16_t packedWidth,
    uint16_t color);
void Draw8x8VideoTile(
    const uint8_t *tilePixels,
    int16_t tileX,
    uint16_t tileY);
void ConfigureMouseCursorOverlay(
    int16_t hotspotX,
    int16_t hotspotY,
    const uint8_t *cursorBitmap);
uint16_t IsMouseCursorInsideRectangle(
    int16_t left,
    int16_t top,
    int16_t right,
    int16_t bottom);
void HideMouseCursorOverlay(void);
void SetMouseCursorPositionUnlocked(int16_t x, int16_t y);
void SetMouseCursorPosition(int16_t x, int16_t y);
void MoveMouseCursorRelative(int16_t deltaX, int16_t deltaY);
void ShowMouseCursorOverlay(void);
void HideMouseCursorOverlayUnlocked(void);
void ShowMouseCursorOverlayUnlocked(void);
void CopyBufferToVideoRegion(
    int16_t xColumn,
    uint16_t yCharacterRow,
    uint16_t widthColumns,
    uint16_t heightCharacterRows,
    const uint8_t *source);
void CopyVideoRegionToBuffer(
    int16_t xColumn,
    uint16_t yCharacterRow,
    uint16_t widthColumns,
    uint16_t heightCharacterRows,
    uint8_t *destination);
void DrawClippedSprite(
    uint8_t *destination,
    const uint8_t *encodedSprite,
    int16_t x,
    int16_t y,
    uint16_t preserveExistingPixels);
char *UppercaseStringInPlace(char *text);
char *FormatLongInteger(
    uint8_t alphabeticDigitBase,
    uint8_t signedConversion,
    uint16_t radix,
    char *destination,
    uint32_t value);
char *IntegerToString(
    int16_t value,
    char *destination,
    uint16_t radix);
char *LongToString(
    int32_t value,
    char *destination,
    uint16_t radix);
void LoadVideoPalette(const uint8_t *paletteData);
void InstallVgaPaletteBankAnchors(void);
uint16_t FlushInputAndWait(uint16_t timeoutRetraces);
uint16_t WaitForDismissInputOrTimeout(uint16_t timeoutRetraces);
uint8_t *BuildDisplayBackingBuffer(
    uint16_t sourceTileX,
    uint16_t sourcePixelY,
    uint16_t widthTiles,
    uint16_t heightPixels);
uint8_t *BuildVgaDisplayBackingBuffer(
    uint16_t sourceTileX,
    uint16_t sourcePixelY,
    uint16_t widthTiles,
    uint16_t heightPixels);
uint8_t *BuildPlanarDisplayBackingBuffer(
    uint16_t sourceTileX,
    uint16_t sourcePixelY,
    uint16_t widthTiles,
    uint16_t heightPixels);
void RebuildMouseCursorBackingBuffer(void);
void LoadGraphicsResourceToBuffers(
    uint16_t resourceIndex,
    uint8_t *decodeBuffer,
    uint8_t *displayBuffer);
void LoadScreenAssetToWorkBuffer(uint16_t resourceIndex);
void ShowScenarioCompletionStatus(void);
void RequestPCSpeakerEffectStop(void);
void AllocateMusicStagingBuffer(void);
void ReleaseMusicDriverBuffer(void);
void InitializeMusicDriver(void);
void ShutdownMusicDriver(void);
void StartLoadedMusicTrack(void);
void LoadMusicResource(uint16_t resourceIndex);
void PlayMusicResource(uint16_t resourceIndex);
void InstallMusicTimer(void);
void RestoreSystemTimer(void);
void PlayPCSpeakerEffectByIndex(uint16_t effectId);
void AcquireDigitalPlaybackDriver(
    uint16_t driverDescriptorOffset,
    uint16_t playbackRateHz);
void ReleaseDigitalPlaybackDriver(uint16_t driverDescriptorOffset);
void InstallDigitalPlaybackTimer(uint16_t playbackRateHz);
void RestoreDigitalPlaybackTimer(void);
void InitializeDigitalDriverQueue(DigitalDriverQueue *queue);
void InstallDigitalDriverQueueCallback(
    DigitalPlaybackDescriptor *playbackDescriptor,
    DigitalDriverQueue *queue);
void AttachDigitalDriverQueue(
    uint16_t driverDescriptorOffset,
    DigitalDriverQueue *queue);
void InitializeDigitalSampleDescriptor(
    DigitalSampleDescriptor *descriptor,
    const uint8_t *sampleData,
    uint16_t sampleLength,
    uint16_t playbackRateHz,
    uint16_t playbackScale);
void ConfigureDigitalSampleTiming(
    DigitalSampleDescriptor *descriptor,
    uint16_t playbackScale);
void ConfigureDigitalSampleCompletion(
    DigitalSampleDescriptor *descriptor,
    DigitalSampleCompletion completion,
    DigitalSampleDescriptor *argument);
void EnqueueDigitalSampleDescriptor(
    DigitalSampleDescriptor *descriptor,
    DigitalDriverQueue *queue);
void RemoveDigitalSampleDescriptor(
    DigitalSampleDescriptor *descriptor);
void PrepareDigitalSamplePlayback(
    DigitalSampleDescriptor *descriptor);
uint16_t GetDigitalSamplePlaybackState(
    DigitalSampleDescriptor *descriptor);
uint32_t SetDigitalPlaybackTimingScale(uint16_t timingScale);
void ServiceDigitalAudioTick(void);
void InstallGameClockTimer(void);
void RestoreGameClockTimer(void);
void GameClockInt1CHandler(void);
void CopyUncompressedFarBlock(
    const void *source,
    void *destination,
    uint32_t byteCount);
uint16_t ReadWholeResourceFile(
    const char *resourceName,
    void *destination,
    uint16_t byteCount);
void ReadResourceFileSpan(
    const char *resourceName,
    int32_t fileOffset,
    void *destination,
    uint16_t byteCount);
void LoadAndDecodePackedResource(
    const char *resourceName,
    void *compressedBuffer,
    void *destination);
void DecodeWestwoodPackedResource(
    const void *compressedBlock,
    void *destination);
void RuntimeExitStepStub(void);
void RunExitHandlersAndTerminate(uint16_t exitCode);
void RuntimeTerminationStub(void);
void DOS_TerminateProcess(uint16_t exitCode);

/* Verified resource-stream contracts from segment 3390. */
void AbortRuntimeWithMessage(const char *errorText);
void ShutdownAndExit(uint16_t exitCode);
void ShowResourceErrorMessage(void);
void ShowFatalDiskErrorMessage(void);
int16_t OpenTrackedFile(char *path, uint16_t openMode);
void CloseTrackedFile(int16_t handleSlot);
uint32_t ReadTrackedFile(
    int16_t handleSlot,
    void *destination,
    uint32_t byteCount);
uint32_t WriteTrackedFile(
    int16_t handleSlot,
    const void *source,
    uint32_t byteCount);
void SeekTrackedFile(int16_t handleSlot, int32_t distance, uint16_t origin);
uint16_t CanOpenFileForRead(const char *path);
void DeleteFileIgnoringErrors(char *filePath);
uint16_t CanOpenFileForWrite(const char *path);
uint16_t GetResourceDriveMask(char *path);
void ReleaseCachedResources(void);
void InitializeResourceCache(void);
uint16_t IsResourceCached(char *path);
uint16_t FindResourceDescriptorIndex(char *path);
uint16_t ReadCachedResource(
    uint16_t resourceIndex,
    void *destination,
    uint32_t requestedBytes);
void AdjustActiveTextWindowMargin(uint16_t marginTiles);

/* Verified text and primitive drawing contracts. */
void DrawTextString(
    const char *text,
    uint16_t x,
    uint16_t y,
    uint8_t foregroundColor,
    uint8_t backgroundColor);
uint16_t MeasureTextWidth(const char *text);
void DrawClippedPixel(int16_t x, int16_t y, uint16_t color);
void DrawHorizontalSpan(
    int16_t y, int16_t startX, int16_t endX, uint16_t color);
void DrawClippedAxisAlignedLine(
    int16_t x1,
    int16_t y1,
    int16_t x2,
    int16_t y2,
    uint16_t color);
uint16_t DrawRepeatingFrameTileSequence(
    const uint16_t *frameDefinition,
    uint16_t sequenceIndex,
    int16_t x,
    int16_t y,
    uint16_t tileCount,
    uint16_t horizontal);
void DrawActiveTextWindowFrame(uint16_t frameStyleIndex);
long ParseSignedDecimalLong(const char *text);
char *GetEnvironmentVariable(const char *name);
void InitializeTimezoneFromEnvironment(void);
uint16_t IsDaylightSavingTime(
    uint16_t yearsSince1970,
    uint16_t month,
    uint16_t dayOrDayOfYear,
    uint8_t hour);
uint32_t ConvertLocalDateTimeToEpochSeconds(
    const DOS_Date *date,
    const DOS_Time *time);
void DrawCombatRatingDescription(
    int16_t combatRating,
    uint16_t x,
    uint16_t y,
    uint8_t foregroundColor,
    uint8_t backgroundColor);
void DrawTacticalUnitControlPanel(uint16_t unitIndex);
void PrepareScenarioUnitRosters(void);
uint8_t AssignScenarioLanceAllegiances(uint16_t scenarioAllegianceIndex);
void InitializeScenarioUnitFromTemplate(
    uint16_t unitType,
    uint16_t unitIndex);
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
void FlagDamagedWeaponRanges(
    const uint8_t *equipmentSlots,
    uint16_t slotCount);
void BuildDamagedWeaponRangeFlags(uint16_t unitIndex);
void DrawTacticalClippedSprite(
    uint8_t *renderTarget,
    const uint8_t *encodedSprite,
    int16_t relativeX,
    int16_t relativeY,
    uint16_t textWindowIndex);
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
void BuildUndamagedUnitCombatEstimate(uint16_t unitIndex);
void CalculateLanceAverageCombatRatings(
    uint16_t leaderUnitIndex,
    uint16_t useActualUnitData);
uint16_t PrepareDisplayedUnitCombatRatings(uint16_t unitIndex);
void DrawClippedRectangleOutline(
    int16_t left,
    int16_t top,
    int16_t right,
    int16_t bottom,
    uint16_t color);
void FillVideoRectangle(
    int16_t leftX,
    int16_t topY,
    int16_t rightX,
    int16_t bottomY,
    uint16_t color);
void Capture8x8VideoTile(
    uint8_t *framebuffer,
    uint8_t *destination,
    int16_t tileX,
    uint16_t tileY);
void LoadPackedIconResource(
    const char *resourcePath,
    uint8_t *packedDisplayBuffer);
void LoadPackedIconPageDirect(
    const char *resourcePath,
    uint8_t *packedDisplayBuffer,
    uint8_t *iconPageDestination);
void InstallDefaultCriticalErrorHandler(void);
void InstallCriticalErrorHandler(void *criticalErrorCallback);
uint8_t *CaptureVideoTileStrip(
    uint8_t *source,
    int16_t startTileX,
    uint16_t startTileY,
    int16_t tileCount);
void CopyDecodedIconPage(
    const uint8_t *packedDisplayBuffer,
    uint8_t *iconPageDestination);
int16_t RunSingleUiRegionSelection(int16_t regionIndex);
void HighlightUiRegionSelection(int16_t regionIndex);
void BeginUiRegionSelection(
    int16_t firstRegionIndex, uint16_t regionCount);
void ToggleActiveUiRegionHighlight(void);
int16_t RunUiRegionSelectionLoop(
    int16_t firstRegionIndex, uint16_t regionCount);
int16_t PollUiRegionSelection(
    int16_t firstRegionIndex, uint16_t regionCount);
void SelectTextWindow(uint16_t windowIndex);
void SelectAndClearTextWindow(uint16_t windowIndex);
void ClearActiveTextWindow(void);
uint8_t *LoadFontResource(const char *fileName);
void SetActiveFontResource(uint8_t *fontResource);
uint16_t MeasureTextWidth(const char *text);
void RenderTextToActiveWindow(
    char *text, uint16_t advanceToNextLine);
void SetActiveRenderTarget(uint8_t *renderTarget);

/* Verified original game lifetime. The preserved source owns main(). */
void InitializeGame(void);
void ShutdownGame(void);
void RunCampaignLoop(void);
void RunTacticalBattle(uint16_t scenarioCode);
void SelectTacticalTextForegroundColor(void);
void SetTacticalSlotSpriteState(
    uint16_t slotIndex,
    uint16_t requestedState);
void DrawCampaignClockSeparator(void);
void RenderFormattedLineBreak(void);
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

#endif
