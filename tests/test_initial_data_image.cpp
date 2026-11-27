#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "REVENGE.h"
#include "revenge/original_data.h"
#include "revenge/combat_data.h"
#include "revenge/original/audio_data.h"
#include "revenge/original/campaign_data.h"
#include "revenge/original/io_data.h"
#include "revenge/original/lance_editor_data.h"
#include "revenge/original/resource_data.h"
#include "revenge/original/scene_data.h"
#include "revenge/platform_backend.h"
#include "revenge/random.h"
#include "original_import_for_tests.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* Each object whose place in the original the source states, as the port
 * starts it (its initializer, the import from the player's copy, the host's
 * copy of the data segment's registered objects, then the resource cache's
 * descriptors) against the bytes DOS loads there: the image with its
 * relocated segment words moved by the load segment, 0800h as the port takes
 * it. A field holding a host pointer is left out (the port's pointer isn't the
 * original's far pointer); every object that differs is listed. Past DS:80FB
 * the image holds no data, so the objects there start at zero. */

typedef struct ImageRow
{
    const char *name;
    const void *object;
    size_t size;
    uint16_t segment;
    uint16_t offset;
    /* A field of each record that holds a host pointer, left out: its place
	 * in a record of `stride` bytes and its size (0 for none). */
    size_t stride;
    size_t skipAt;
    size_t skipSize;
    /* An object whose far pointers are references: it writes the object as
	 * the original lays it out (size bytes), each reference as the far
	 * pointer the original holds there. */
    void (*encode)(uint8_t *originalBytes);
} ImageRow;

#define ROW(object, segment, offset)           { #object, &(object), sizeof(object), (segment), (offset), 0, 0, 0, NULL }
#define DS(object, offset)                     ROW(object, 0x3A6A, offset)
#define ENCODED(object, offset, size, encoder) { #object, NULL, (size), 0x3A6A, (offset), 0, 0, 0, (encoder) }

/* A reference written as the original's far pointer. NULL is 0:0; a
 * reference to anything else gets FFFF:FFFF, which no image holds where the
 * original starts with a far pointer of its own. */
static void PutWord(uint8_t *bytes, size_t at, uint16_t value)
{
    bytes[at] = (uint8_t) value;
    bytes[at + 1] = (uint8_t) (value >> 8);
}

static void PutFar(uint8_t *bytes, size_t at, uint16_t segment, uint16_t offset)
{
    PutWord(bytes, at, offset);
    PutWord(bytes, at + 2, segment);
}

static void PutNull(uint8_t *bytes, size_t at, const void *reference)
{
    PutFar(bytes, at, reference == NULL ? 0x0000 : 0xFFFF, reference == NULL ? 0x0000 : 0xFFFF);
}

/* DS:58DB: each into DS:5642. */
static void EncodeSpeakerEffectStreams(uint8_t *bytes)
{
    size_t index;

    for (index = 0; index < PC_SPEAKER_EFFECT_COUNT; ++index)
        PutFar(bytes, index * 4, 0x3A6A, (uint16_t) (0x5642 + (PCSpeakerEffectStreamPointerTable[index] - PCSpeakerEffectStreamData)));
}

/* DS:624A: six words, the stream cursor, five words, two bytes. */
static void EncodeMIDISequence(uint8_t *bytes)
{
    const MIDISequenceState *state = &MIDISequence;

    PutWord(bytes, 0x00, state->MIDIBeatsPerMinute);
    PutWord(bytes, 0x02, state->MIDITimerCallsPerMinute);
    PutWord(bytes, 0x04, state->MIDITicksPerMinute);
    PutWord(bytes, 0x06, state->MIDITickRateRemainder);
    PutWord(bytes, 0x08, state->MIDITicksPerBeat);
    PutWord(bytes, 0x0A, state->MIDIBeatsPerMeasure);
    PutNull(bytes, 0x0C, state->MIDIStreamCursor);
    PutWord(bytes, 0x10, state->MIDIPlaybackFlags);
    PutWord(bytes, 0x12, state->MIDITicksUntilNextEvent);
    PutWord(bytes, 0x14, state->MIDIBeatTickRemainder);
    PutWord(bytes, 0x16, state->MIDIMeasureBeatRemainder);
    PutWord(bytes, 0x18, state->MIDIMeasureCount);
    bytes[0x1A] = state->MIDIPlaybackStateBusy;
    bytes[0x1B] = state->MIDIRunningStatus;
}

/* DS:6C0E: the IRQ 2 vector the rebuild never takes (0:0), then the queue. */
static void EncodeRolandMPU401(uint8_t *bytes)
{
    PutFar(bytes, 0x00, 0x0000, 0x0000);
    memcpy(bytes + 0x04, RolandMPU401.TransmitQueue, sizeof(RolandMPU401.TransmitQueue));
    PutWord(bytes, 0x104, RolandMPU401.WriteIndex);
    PutWord(bytes, 0x106, RolandMPU401.ReadIndex);
    PutWord(bytes, 0x108, RolandMPU401.ByteCount);
}

/* A queue's 12h bytes and a descriptor's 34h (REVENGE.h). Before the game
 * starts, none holds a reference. */
static void EncodeQueue(uint8_t *bytes, const DigitalDriverQueue *queue)
{
    PutNull(bytes, 0x00, queue->First);
    PutNull(bytes, 0x04, queue->End.Next);
    PutNull(bytes, 0x08, queue->End.PreviousLink);
    PutWord(bytes, 0x0C, queue->ActiveDescriptorCount);
    PutWord(bytes, 0x0E, queue->MixBias);
    bytes[0x10] = queue->OutputSample;
    bytes[0x11] = queue->ReservedOutputByte;
}

static void EncodeSceneCursor(uint8_t *bytes)
{
    PutNull(bytes, 0x00, SceneInstructionCursor);
}

/* As a save writes them (UnitSelectionCard_Store). */
static void EncodeSelectionCards(uint8_t *bytes)
{
    size_t index;

    for (index = 0; index < UNIT_SELECTION_CARD_COUNT; ++index)
    {
        RevengeSavedSelectionCard saved;

        UnitSelectionCard_Store(&saved, &UnitSelectionCards[index]);
        memcpy(bytes + index * sizeof(saved), &saved, sizeof(saved));
    }
}

static void EncodeResourceFiles(uint8_t *bytes)
{
    size_t index;

    PutNull(bytes, 0x00, ResourceFiles.errorText);
    PutWord(bytes, 0x04, ResourceFiles.pendingDriveMask);
    PutWord(bytes, 0x06, ResourceFiles.availableDriveMask);
    for (index = 0; index < ORIGINAL_TRACKED_FILE_COUNT; ++index)
        PutWord(bytes, 0x08 + index * 2, ResourceFiles.trackedHandles[index]);
    PutWord(bytes, 0x1C, ResourceFiles.preloadFiles);
}

static void EncodeCachedFiles(uint8_t *bytes)
{
    size_t index;

    for (index = 0; index < ORIGINAL_RESOURCE_DESCRIPTOR_COUNT; ++index)
    {
        const CachedFileDescriptor *file = &CachedFileDescriptors[index];
        uint8_t *record = bytes + index * 0x12;

        PutNull(record, 0x00, file->fileName);
        PutWord(record, 0x04, LOW_WORD(file->fileLength));
        PutWord(record, 0x06, HIGH_WORD(file->fileLength));
        PutNull(record, 0x08, file->cachedData);
        PutWord(record, 0x0C, file->resourceDriveMask);
        PutWord(record, 0x0E, LOW_WORD(file->currentPosition));
        PutWord(record, 0x10, HIGH_WORD(file->currentPosition));
    }
}

static void EncodeDescriptor(uint8_t *bytes, const DigitalSampleDescriptor *descriptor)
{
    PutNull(bytes, 0x00, descriptor->Link.Next);
    PutNull(bytes, 0x04, descriptor->Link.PreviousLink);
    PutNull(bytes, 0x08, descriptor->OwnerQueue);
    PutNull(bytes, 0x0C, descriptor->SampleData);
    PutWord(bytes, 0x10, descriptor->SampleLength);
    PutWord(bytes, 0x12, descriptor->LoopStart);
    PutWord(bytes, 0x14, descriptor->LoopEnd);
    PutWord(bytes, 0x16, descriptor->LoopLength);
    PutWord(bytes, 0x18, descriptor->SourceRateHz);
    PutWord(bytes, 0x1A, descriptor->PlaybackScale);
    PutWord(bytes, 0x1C, descriptor->SourceStepFraction);
    PutWord(bytes, 0x1E, descriptor->SourceStepWhole);
    PutWord(bytes, 0x20, descriptor->PlaybackHandlerOffset);
    PutWord(bytes, 0x22, descriptor->SourceCursorFraction);
    PutWord(bytes, 0x24, descriptor->SourceCursor);
    PutWord(bytes, 0x26, 0x0000);
    PutWord(bytes, 0x28, descriptor->RemainingFraction);
    PutWord(bytes, 0x2A, descriptor->RemainingWhole);
    PutNull(bytes, 0x2C, (const void *) (descriptor->Completion != NULL ? descriptor : NULL));
    PutNull(bytes, 0x30, descriptor->CompletionArgument);
}

/* DS:6F4C: two queues, then the calibration descriptor. */
static void EncodePlaybackBuffers(uint8_t *bytes)
{
    EncodeQueue(bytes + 0x00, &DigitalPlaybackBuffers.PlaybackTimerQueue);
    EncodeQueue(bytes + 0x12, &DigitalPlaybackBuffers.CalibrationQueue);
    EncodeDescriptor(bytes + 0x24, &DigitalPlaybackBuffers.CalibrationSample);
}

static const ImageRow rows[] = {
    /* Registered at their DS addresses (src/SDL/system.cpp). */
    DS(PresentationDismissTimeout, 0x240F),
    DS(SceneTransitionAbortFlag, 0xA864),
    DS(TextWindowContexts, 0x063A),
    DS(UiRegionTable, 0x07AA),
    DS(LiveScenarioUnits, 0x99E0),
    DS(HeatDissipationBySinkCountQ8_8, 0x176A),
    DS(TacticalStatusBlinkCountdown, 0x2F36),
    DS(DisplayedTacticalStatusRow, 0x2F38),
    DS(TacticalStatusRowInvalidation, 0xB574),
    DS(TacticalImpactQueue, 0xB2F4),
    DS(TacticalStatusBlinkVisible, 0x86BD),
    DS(SceneNarrationEventFlags, 0xAE63),
    DS(TrainingScenarioCompletionFlags, 0x00A0),
    DS(ScreenPaletteResourceByScreen, 0x1704),
    DS(SaveDirectory, 0x18CE),
    DS(SaveCampaignSnapshot, 0x1956),
    DS(InitialCampaignState, 0x1B77),
    DS(ForwardCampaignState, 0x1B7E),
    DS(ReplayCampaignState, 0x1B85),
    DS(InitialExtendedCampaignState, 0x1B8F),
    DS(ForwardExtendedCampaignState, 0x1B9E),
    DS(ReplayExtendedCampaignState, 0x1BAD),
    DS(ScenarioOrdinalBaseByCampaignStage, 0x254C),
    DS(MissionFadeColorTable, 0x2718),
    DS(MissionBlinkRectX, 0x271C),
    DS(MissionBlinkRectY, 0x272A),
    DS(MissionBlinkRectExtent, 0x2738),
    DS(MissionDateMonthIndex, 0x273F),
    DS(MissionDateYear, 0x274B),
    DS(MissionDateAdvanceMonths, 0x2763),
    DS(MissionSpriteSourceTileX, 0x2783),
    DS(MissionSpriteSourcePixelY, 0x279B),
    DS(MissionSpriteWidthTiles, 0x27B3),
    DS(MissionSpriteHeightPixels, 0x27CB),
    DS(CampaignMessageTable, 0x2B54),
    DS(LoadedSceneFile, 0x71A8),
    DS(SceneUnitRoutes, 0x86C1),
    DS(SceneInstructionBaseOffset, 0xA862),
    ENCODED(SceneInstructionCursor, 0xA9D9, 0x04, EncodeSceneCursor),
    ENCODED(UnitSelectionCards, 0x1BBC, 0x10E, EncodeSelectionCards),
    DS(UnitSelectionCardStart, 0x1BBC),
    DS(LanceAssignments, 0x1CCA),
    DS(TacticalUpdateDelayReloadTicks, 0x17DA),
    DS(TacticalGameSpeedIndex, 0x17DC),
    DS(LanceEditorFirstBriefingPending, 0x30C4),
    DS(LanceEditorBriefingPresented, 0x30C6),
    DS(LanceEditorReloadGraphics, 0x30C8),
    DS(UnitEditorLanceSlotCount, 0x3112),
    DS(RandomUnitTypeCount, 0x3118),
    DS(UnitEditorUnitSpriteIndex, 0x3119),
    DS(UnitNameSplitIndex, 0x3172),
    DS(PredefinedUnitTypeTable, 0x31CB),
    DS(UnitEditorSectionEnabled, 0x31E9),
    DS(UnitEditorScenarioCardCountByFamily, 0x3206),
    DS(RandomUnitTypeCountByFamily, 0x320D),
    DS(RandomUnitTypePoolByFamily, 0x3217),
    DS(PilotPortraitIndexById, 0x3248),
    DS(DefaultPilotIdsByScenario, 0x3269),
    DS(UnitEditorPilotCardCountByFamily, 0x3291),
    DS(FixedPilotIds, 0x3354),
    DS(FixedUnitTypeIds, 0x3356),
    DS(UnitClassTypeListBytes, 0x3298),
    DS(LanceEditorLayout, 0x3470),

    /* The save block's fields (SaveGame, 0D73:4866; tools/trace/state.py). */
    DS(CampaignStageIndex, 0x24D6),
    DS(ScenarioVariantIndex, 0x24D8),
    DS(CampaignPhase, 0x24DA),
    DS(TrainingSequenceFlag, 0x24DC),
    DS(CampaignFlags, 0x24DF),
    DS(SavedCampaignFlags, 0x24E1),
    DS(ScenarioUnitZeroAvailable, 0xA967),
    DS(EjectedPilotRosterState, 0x9991),
    DS(ScenarioUnitFifteenState, 0x88F1),
    DS(ScenarioUnitFourteenEnabled, 0x8925),
    DS(ScenarioUnitThirteenEnabled, 0x99C5),
    DS(LastAttackingUnitIndex, 0xA965),
    DS(CampaignScenarioState, 0x8594),
    DS(LrmShipmentLoaded, 0x8108),
    DS(SrmShipmentLoaded, 0x810A),
    DS(SavedCampaignClockState, 0x855F),
    DS(ScenarioUnitMapX, 0x8562),
    DS(ScenarioUnitMapY, 0x857B),
    DS(RandomPilotPoolCounts, 0x1B8C),
    DS(PilotRoster, 0x1D4C),

    /* The trace's state lines (tools/trace/state.py). */
    ROW(PseudoRandomState, 0x4184, 0x0E48),
    DS(GameTimerTicks, 0x591B),
    DS(GameAnimationTicks, 0x591F),
    DS(CampaignClockState, 0x8337),
    DS(TacticalBattleResult, 0x833C),
    DS(TacticalSelectedUnitIndex, 0x03DC),
    DS(TacticalOverviewMapMode, 0x03E5),
    DS(TacticalScenarioLanceEnabled, 0x03EA),
    DS(TacticalOverviewActive, 0x8104),
    DS(TacticalUpdateDelayTicks, 0x5923),
    DS(TacticalCursorMapX, 0xA96D),
    DS(TacticalCursorMapY, 0xA96F),
    DS(TacticalCursorViewportX, 0x8331),
    DS(TacticalCursorViewportY, 0x8333),
    DS(PersistentUnitRoster, 0x8B81),

    /* Cited beside their definitions. */
    ROW(LinePrimaryRowOffset, 0x4184, 0x08FB),
    ROW(LineSecondaryRowOffset, 0x4184, 0x091F),
    DS(RequestedMusicEnabled, 0x800C),
    DS(UnitAttackTarget, 0x88F3),
    DS(UnitTacticalTarget, 0x88A4),
    DS(HitLocationTable, 0x1038),
    DS(HitLocationDirectionSelectorTable, 0x3CFE),
    DS(VideoConfigurationRecords, 0x3AD1),
    ENCODED(MIDISequence, 0x624A, 0x1C, EncodeMIDISequence),
    DS(MIDIParser, 0x62B0),
    DS(MIDIChannelMap, 0x6728),
    ENCODED(ResourceFiles, 0xB632, 0x1E, EncodeResourceFiles),
    DS(SecondaryResourceDriveEnabled, 0xB662),
    DS(ResourceDriveSelectionDisabled, 0xB668),
    DS(WeaponPriorityData, 0x09C6),
    { "TacticalStatusPadding", TacticalStatusPadding, 2, 0x3A6A, 0x30C2, 0, 0, 0, NULL },
    DS(MIDISystemExclusive, 0x6738),
    DS(SpeakerMIDIRuntime, 0x67BC),
    ENCODED(RolandMPU401, 0x6C0E, 0x10A, EncodeRolandMPU401),
    ENCODED(DigitalPlaybackBuffers, 0x6F4C, 0x58, EncodePlaybackBuffers),
    DS(PITCalibration, 0x6FA4),
    DS(InputDeviceType, 0x88BD),
    DS(UnitCombatRatings, 0xAEF1),
    DS(CriticalErrorCallback, 0xB888),
    /* As the resource cache's start (3390:0A22) fills them. Each file's name
	 * points at its own text in the original and at ResourceFileNames here:
	 * it is left out. */
    { "CachedFileDescriptors", NULL, ORIGINAL_RESOURCE_DESCRIPTOR_COUNT * 0x12, 0x3A6A, 0x10B0,
        0x12, 0x00, 0x04, EncodeCachedFiles },
    ENCODED(PCSpeakerEffectStreamPointerTable, 0x58DB, 0x40, EncodeSpeakerEffectStreams),
    DS(ScenarioLanceAllegianceTable, 0x5928),

    /* The sampled-sound devices: each output descriptor (DS:6EB2, 0Ch apart)
	 * names its hardware descriptor; four of those name a probe list. The last
	 * three come after the carrier register list at DS:6EFA. */
    DS(NullDigitalPlayback, 0x6EB2),
    DS(SoundBlasterPlayback, 0x6EE2),

    /* Places the data map's checks settled (tools/map/data_map.txt). */
    /* Found by the port's bytes in the loaded image: the weapon records'
	 * name pointers are relocated words, so the file alone doesn't match. */
    DS(WeaponDefinitions, 0x0435),
    DS(FormationOffsetTable, 0x17DE),
    /* The asm's operands (3A3C:0217-0267); the port had it a word early. */
    DS(CalendarBoundaryTable, 0x716E),
    DS(ImprovedMissileDamageBonusTable, 0x3D1D),
    DS(StartupResourceState, 0x5FCE),
    /* Where cached-file descriptor 45h's name points (DS:158A). */
    { "MediaConfigurationFileName", MediaConfigurationFileName, 0x0A, 0x3A6A, 0x21F4, 0, 0, 0, NULL },
};

static uint8_t image[ORIGINAL_IMAGE_CAPACITY];

/* The image as DOS loads it at 0800h: each relocated segment word moved by
 * the load segment. */
static void LoadImage(void)
{
    size_t count = OriginalImport_RelocationCount();
    size_t index;

    memcpy(image, OriginalImage, OriginalImport_ImageBytes());
    for (index = 0; index < count; ++index)
    {
        size_t at = OriginalImport_Relocation(index);
        uint16_t value;

        if (at + 1 >= sizeof(image))
            continue;
        value = (uint16_t) (image[at] | image[at + 1] << 8);
        value = (uint16_t) (value + 0x0800);
        image[at] = (uint8_t) value;
        image[at + 1] = (uint8_t) (value >> 8);
    }
}

int main(void)
{
    size_t index;
    unsigned differing = 0;

    RequireOriginalImport();
    OriginalData_InitializeResourceDescriptorPointers();
    LoadImage();
    for (index = 0; index < sizeof(rows) / sizeof(rows[0]); ++index)
    {
        const ImageRow *row = &rows[index];
        size_t at = (size_t) (row->segment - 0x0800) * 16 + row->offset;
        static uint8_t encoded[0x1000];
        const uint8_t *port = (const uint8_t *) row->object;
        size_t first = row->size;
        size_t count = 0;
        size_t byteIndex;

        if (row->encode != NULL)
        {
            memset(encoded, 0, sizeof(encoded));
            row->encode(encoded);
            port = encoded;
        }
        for (byteIndex = 0; byteIndex < row->size; ++byteIndex)
        {
            uint8_t original = at + byteIndex < sizeof(image) ? image[at + byteIndex] : 0;
            if (row->skipSize != 0 && byteIndex % row->stride >= row->skipAt &&
                byteIndex % row->stride < row->skipAt + row->skipSize)
                continue;
            if (port[byteIndex] != original)
            {
                if (count == 0)
                    first = byteIndex;
                ++count;
            }
        }
        if (count != 0)
        {
            ++differing;
            printf("%s %04X:%04X (%u bytes): %u differ, first +%X port %02X image %02X\n",
                row->name, (unsigned) row->segment, (unsigned) row->offset,
                (unsigned) row->size, (unsigned) count, (unsigned) first,
                (unsigned) port[first], (unsigned) image[at + first]);
        }
    }
    printf("%u of %u objects differ from the image\n", differing,
        (unsigned) (sizeof(rows) / sizeof(rows[0])));
    return differing != 0;
}
