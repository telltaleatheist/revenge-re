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
#include "revenge/representation.h"
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
	word16 segment;
	word16 offset;
	/* A field of each record that holds a host pointer, left out: its place
	 * in a record of `stride` bytes and its size (0 for none). */
	size_t stride;
	size_t skipAt;
	size_t skipSize;
} ImageRow;

#define ROW(object, segment, offset) { #object, &(object), sizeof(object), (segment), (offset), 0, 0, 0 }
#define DS(object, offset) ROW(object, 0x3A6A, offset)

static const ImageRow rows[] =
{
	/* Registered at their DS addresses (src/SDL/system.c). */
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
	DS(ScreenPaletteResourceByScreen, 0x1703),
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
	DS(SceneInstructionCursor, 0xA9D9),
	DS(ScenarioResourcePathPointer, 0x1554),
	DS(UnitSelectionCards, 0x1BBC),
	DS(LanceAssignments, 0x1CCA),
	DS(TacticalUpdateDelayReloadTicks, 0x17DA),
	DS(TacticalGameSpeedIndex, 0x17DC),
	DS(LanceEditorFirstBriefingPending, 0x30C4),
	DS(LanceEditorBriefingPresented, 0x30C6),
	DS(LanceEditorReloadGraphics, 0x30C8),
	DS(UnitEditorLanceSlotCount, 0x3112),
	DS(RandomUnitTypeCount, 0x3118),
	DS(UnitEditorUnitSpriteIndex, 0x3119),
	DS(UnitMovementClassByType, 0x3172),
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
	DS(PCSpeakerSampleConversion, 0xB6D4),
	DS(MIDISequence, 0x624A),
	DS(MIDIParser, 0x62B0),
	DS(MIDIChannelMap, 0x6728),
	DS(FileRuntime, 0x712A),
	DS(ResourceFiles, 0xB632),
	DS(SecondaryResourceDriveEnabled, 0xB662),
	DS(ResourceDriveSelectionDisabled, 0xB668),
	DS(WeaponPriorityData, 0x09C6),
	DS(UnitTemplatePathOffset, 0x1530),
	DS(UnitTemplatePathSegment, 0x1532),
	DS(TacticalResourcePathOffset, 0x1566),
	DS(TacticalResourcePathSegment, 0x1568),
	{ "TacticalStatusPadding", TacticalStatusPadding, 2, 0x3A6A, 0x30C2, 0, 0, 0 },
	DS(MIDISystemExclusive, 0x6738),
	DS(SpeakerMIDIRuntime, 0x67BC),
	DS(AdLibMIDIRuntime, 0x6A6E),
	DS(RolandMPU401, 0x6C0E),
	DS(DigitalPlaybackBuffers, 0x6F4C),
	DS(PITCalibration, 0x6FA4),
	DS(InputDeviceType, 0x88BD),
	DS(UnitCombatRatings, 0xAEF1),
	DS(CriticalErrorCallback, 0xB888),
	/* Each file's name is a host pointer; the rest is compared, as the
	 * resource cache's start (3390:0A22) fills it. */
	{ "CachedFileDescriptors", CachedFileDescriptors, sizeof(CachedFileDescriptors), 0x3A6A, 0x10B0,
		sizeof(CachedFileDescriptors[0]), offsetof(CachedFileDescriptor, fileName),
		sizeof(CachedFileDescriptors[0].fileName) },
	DS(PCSpeakerEffectStreamPointerTable, 0x58DB),
	DS(ScenarioLanceAllegianceTable, 0x5928),

	/* The sampled-sound devices: each output descriptor (DS:6EB2, 0Ch apart)
	 * names its hardware descriptor; four of those name a probe list. The last
	 * three come after the carrier register list at DS:6EFA. */
	DS(NullDigitalHardware, 0x6D28),
	DS(Tandy3VoiceHardware, 0x6D40),
	DS(TandyDACHardware, 0x6D6A),
	DS(IBMPS1AudioHardware, 0x6DA2),
	DS(SoundBlasterHardware, 0x6DC2),
	DS(SoundBlasterProbePorts, 0x6DD2),
	DS(AdLibSoundHardware, 0x6DEC),
	DS(CMSSoundHardware, 0x6E12),
	DS(CMSProbePorts, 0x6E22),
	DS(InnovationSoundHardware, 0x6E52),
	DS(InnovationProbePorts, 0x6E62),
	DS(CovoxSoundHardware, 0x6E84),
	DS(CovoxProbePorts, 0x6E94),
	DS(NullDigitalPlayback, 0x6EB2),
	DS(Tandy3VoicePlayback, 0x6EBE),
	DS(TandyDACPlayback, 0x6ECA),
	DS(IBMPS1Playback, 0x6ED6),
	DS(SoundBlasterPlayback, 0x6EE2),
	DS(AdLibPlayback, 0x6EEE),
	DS(CMSPlayback, 0x6F28),
	DS(InnovationPlayback, 0x6F34),
	DS(CovoxPlayback, 0x6F40),

	/* Places the data map's checks settled (tools/map/data_map.txt). */
	DS(StartupResourceState, 0x5FCE),
	/* Where cached-file descriptor 45h's name points (DS:158A). */
	{ "MediaConfigurationFileName", MediaConfigurationFileName, 0x0A, 0x3A6A, 0x21F4, 0, 0, 0 },
};

static byte image[ORIGINAL_IMAGE_CAPACITY];

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
		word16 value;

		if (at + 1 >= sizeof(image))
			continue;
		value = (word16) (image[at] | image[at + 1] << 8);
		value = (word16) (value + 0x0800);
		image[at] = (byte) value;
		image[at + 1] = (byte) (value >> 8);
	}
}

int main(void)
{
	size_t index;
	unsigned differing = 0;

	RequireOriginalImport();
	/* The start the program makes before the game's own (src/SDL/entry.c). */
	SDLBackend_InitializeDOSMemoryArena();
	OriginalData_InitializeResourceDescriptorPointers();
	LoadImage();
	for (index = 0; index < sizeof(rows) / sizeof(rows[0]); ++index)
	{
		const ImageRow *row = &rows[index];
		size_t at = (size_t) (row->segment - 0x0800) * 16 + row->offset;
		const byte *port = (const byte *) row->object;
		size_t first = row->size;
		size_t count = 0;
		size_t byteIndex;

		for (byteIndex = 0; byteIndex < row->size; ++byteIndex)
		{
			byte original = at + byteIndex < sizeof(image) ? image[at + byteIndex] : 0;
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
