#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "revenge/platform_backend.h"
#include "revenge/far_memory.h"
#include "revenge/original_data.h"
#include "revenge/original_import.h"
#include "harness.h"
#include "revenge/original/campaign_data.h"
#include "revenge/original/lance_editor_data.h"
#include "revenge/audio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static byte *dosMemoryBlock;
static uint32 dosMemoryBlockBytes;
static uint32 dosMemoryBlockCapacity;
static word16 dosMemoryBlockSegment;
static byte originalDataSegment[0x10000];


#define ORIGINAL_DATA_SEGMENT_MAPPED_BYTES 0xB8A0

void SDLBackend_InitializeDOSMemoryArena(void)
{
	/* Borland establishes these CRT break values before calling main. The host
	 * entry shim supplies the equivalent conventional-memory arena so the
	 * preserved sbrk/far-heap procedures can remain unchanged. */
	/* REVENGE.EXE loads at 0800h with its PSP at 07F0h.  The 3A7Ah-paragraph
	 * load image plus the MZ header's 038Ah minimum allocation reaches 4604h,
	 * which is where Borland establishes the first legal program break. */
	MemoryBlockSegment = 0x07F0;
	MinimumProgramBreakSegment = 0x4604;
	MinimumProgramBreakOffset = 0x0000;
	CurrentProgramBreakSegment = 0x4604;
	CurrentProgramBreakOffset = 0x0000;
	MaximumProgramBreakSegment = 0xA000;
	MaximumProgramBreakOffset = 0x0000;
	ProgramBreakKilobytes = 0x0000;

	/* DS:offset expressions the port doesn't name address
	 * originalDataSegment, which starts at zero: every byte of the original's
	 * data segment that play reads is a named object the port fills (its
	 * initializer, the rules, the story), registered below over this
	 * backing. Neither the data segment nor the program image is taken from
	 * the player's copy. */
	HostFarPointer_RegisterAt(&PresentationDismissTimeout,
		sizeof(PresentationDismissTimeout), 0x3A6A240FUL);
	HostFarPointer_RegisterAt(&SceneTransitionAbortFlag,
		sizeof(SceneTransitionAbortFlag), 0x3A6AA864UL);
	HostFarPointer_RegisterAt(TextWindowContexts,
		sizeof(TextWindowContexts), 0x3A6A063AUL);
	HostFarPointer_RegisterAt(UiRegionTable,
		sizeof(UiRegionTable), 0x3A6A07AAUL);
	HostFarPointer_RegisterAt(LiveScenarioUnits,
		sizeof(LiveScenarioUnits), 0x3A6A99E0UL);
	HostFarPointer_RegisterAt(HeatDissipationBySinkCountQ8_8,
		sizeof(HeatDissipationBySinkCountQ8_8), 0x3A6A176AUL);
	HostFarPointer_RegisterAt(&TacticalStatusBlinkCountdown,
		sizeof(TacticalStatusBlinkCountdown), 0x3A6A2F36UL);
	HostFarPointer_RegisterAt(&DisplayedTacticalStatusRow,
		sizeof(DisplayedTacticalStatusRow), 0x3A6A2F38UL);
	HostFarPointer_RegisterAt(TacticalStatusRowInvalidation,
		sizeof(TacticalStatusRowInvalidation), 0x3A6AB574UL);
	HostFarPointer_RegisterAt(TacticalImpactQueue,
		sizeof(TacticalImpactQueue), 0x3A6AB2F4UL);
	HostFarPointer_RegisterAt(&TacticalStatusBlinkVisible,
		sizeof(TacticalStatusBlinkVisible), 0x3A6A86BDUL);
	HostFarPointer_RegisterAt(SceneNarrationEventFlags,
		sizeof(SceneNarrationEventFlags), 0x3A6AAE63UL);
#define REGISTER_DATA_OBJECT(object, offset) \
	HostFarPointer_RegisterAt(&(object), sizeof(object), \
		0x3A6A0000UL + (offset))
	REGISTER_DATA_OBJECT(TrainingScenarioCompletionFlags, 0x00A0);
	REGISTER_DATA_OBJECT(ScreenPaletteResourceByScreen, 0x1704);
	REGISTER_DATA_OBJECT(SaveDirectory, 0x18CE);
	REGISTER_DATA_OBJECT(SaveCampaignSnapshot, 0x1956);
	REGISTER_DATA_OBJECT(InitialCampaignState, 0x1B77);
	REGISTER_DATA_OBJECT(ForwardCampaignState, 0x1B7E);
	REGISTER_DATA_OBJECT(ReplayCampaignState, 0x1B85);
	REGISTER_DATA_OBJECT(InitialExtendedCampaignState, 0x1B8F);
	REGISTER_DATA_OBJECT(ForwardExtendedCampaignState, 0x1B9E);
	REGISTER_DATA_OBJECT(ReplayExtendedCampaignState, 0x1BAD);
	REGISTER_DATA_OBJECT(ScenarioOrdinalBaseByCampaignStage, 0x254C);
	REGISTER_DATA_OBJECT(MissionFadeColorTable, 0x2718);
	REGISTER_DATA_OBJECT(MissionBlinkRectX, 0x271C);
	REGISTER_DATA_OBJECT(MissionBlinkRectY, 0x272A);
	REGISTER_DATA_OBJECT(MissionBlinkRectExtent, 0x2738);
	REGISTER_DATA_OBJECT(MissionDateMonthIndex, 0x273F);
	REGISTER_DATA_OBJECT(MissionDateYear, 0x274B);
	REGISTER_DATA_OBJECT(MissionDateAdvanceMonths, 0x2763);
	REGISTER_DATA_OBJECT(MissionSpriteSourceTileX, 0x2783);
	REGISTER_DATA_OBJECT(MissionSpriteSourcePixelY, 0x279B);
	REGISTER_DATA_OBJECT(MissionSpriteWidthTiles, 0x27B3);
	REGISTER_DATA_OBJECT(MissionSpriteHeightPixels, 0x27CB);
	REGISTER_DATA_OBJECT(CampaignMessageTable, 0x2B54);
	REGISTER_DATA_OBJECT(LoadedSceneFile, 0x71A8);
	REGISTER_DATA_OBJECT(SceneUnitRoutes, 0x86C1);
	REGISTER_DATA_OBJECT(SceneInstructionBaseOffset, 0xA862);
	REGISTER_DATA_OBJECT(SceneInstructionCursor, 0xA9D9);
	REGISTER_DATA_OBJECT(RandomPilotPoolCounts, 0x1B8C);
	REGISTER_DATA_OBJECT(UnitSelectionCards, 0x1BBC);
	REGISTER_DATA_OBJECT(LanceAssignments, 0x1CCA);
	REGISTER_DATA_OBJECT(TacticalUpdateDelayReloadTicks, 0x17DA);
	REGISTER_DATA_OBJECT(TacticalGameSpeedIndex, 0x17DC);
	REGISTER_DATA_OBJECT(LanceEditorFirstBriefingPending, 0x30C4);
	REGISTER_DATA_OBJECT(LanceEditorBriefingPresented, 0x30C6);
	REGISTER_DATA_OBJECT(LanceEditorReloadGraphics, 0x30C8);
	REGISTER_DATA_OBJECT(UnitEditorLanceSlotCount, 0x3112);
	REGISTER_DATA_OBJECT(RandomUnitTypeCount, 0x3118);
	REGISTER_DATA_OBJECT(UnitEditorUnitSpriteIndex, 0x3119);
	REGISTER_DATA_OBJECT(UnitNameSplitIndex, 0x3172);
	REGISTER_DATA_OBJECT(PredefinedUnitTypeTable, 0x31CB);
	REGISTER_DATA_OBJECT(UnitEditorSectionEnabled, 0x31E9);
	REGISTER_DATA_OBJECT(UnitEditorScenarioCardCountByFamily, 0x3206);
	REGISTER_DATA_OBJECT(RandomUnitTypeCountByFamily, 0x320D);
	REGISTER_DATA_OBJECT(RandomUnitTypePoolByFamily, 0x3217);
	REGISTER_DATA_OBJECT(PilotPortraitIndexById, 0x3248);
	REGISTER_DATA_OBJECT(DefaultPilotIdsByScenario, 0x3269);
	REGISTER_DATA_OBJECT(UnitEditorPilotCardCountByFamily, 0x3291);
	REGISTER_DATA_OBJECT(FixedPilotIds, 0x3354);
	REGISTER_DATA_OBJECT(FixedUnitTypeIds, 0x3356);
	REGISTER_DATA_OBJECT(LanceEditorLayout, 0x3470);
#undef REGISTER_DATA_OBJECT
	HostFarPointer_RegisterAt(originalDataSegment,
		ORIGINAL_DATA_SEGMENT_MAPPED_BYTES, 0x3A6A0000UL);
}

_Noreturn void SDLBackend_TerminateProcess(byte exitCode)
{
	SDLBackend_Close();
	exit((int) exitCode);
}

word16 SDLBackend_ResizeMemoryBlock(
    word16 memorySegment,
    word16 paragraphCount)
{
	uint32 requestedBytes = (uint32) paragraphCount << 0x04;
	uint32 maximumBytes;

	if (requestedBytes == 0x00000000UL)
	{
		if (dosMemoryBlock != NULL)
		{
			HostFarPointer_Unregister(dosMemoryBlock);
			free(dosMemoryBlock);
		}
		dosMemoryBlock = NULL;
		dosMemoryBlockBytes = 0x00000000UL;
		dosMemoryBlockCapacity = 0x00000000UL;
		dosMemoryBlockSegment = 0x0000;
		return 0xFFFF;
	}

	/* DOS AH=4Ah changes the size of an existing conventional-memory block;
	 * its segment and all pointers into it remain stable.  Reserve the complete
	 * segment-to-A000 arena once, then change only the logical byte count. */
	maximumBytes = memorySegment < 0xA000
		? (uint32) (0xA000 - memorySegment) << 0x04
		: 0x00000000UL;
	if (dosMemoryBlock == NULL)
	{
		dosMemoryBlock = (byte *) calloc(0x01, maximumBytes);
		if (dosMemoryBlock == NULL)
			return (word16) (maximumBytes >> 0x04);
		dosMemoryBlockCapacity = maximumBytes;
		dosMemoryBlockSegment = memorySegment;
		HostFarPointer_RegisterAt(dosMemoryBlock, dosMemoryBlockCapacity,
			(uint32) dosMemoryBlockSegment << 0x10);
	}
	if (memorySegment != dosMemoryBlockSegment ||
		requestedBytes > dosMemoryBlockCapacity)
		return (word16) (dosMemoryBlockCapacity >> 0x04);
	dosMemoryBlockBytes = requestedBytes;
	return 0xFFFF;
}

const byte *SDLBackend_DOSMemoryBlock(uint32 *capacity, word16 *segment)
{
	*capacity = dosMemoryBlockCapacity;
	*segment = dosMemoryBlockSegment;
	return dosMemoryBlock;
}

word16 SDLBackend_GetLargestFreeConventionalMemoryParagraphs(
	word16 requestedParagraphs)
{
	/* The host has no DOS conventional-memory arena. Report the requested
	 * capacity at this lowest platform boundary; the preserved caller retains
	 * the original paragraph-to-KiB conversion and 640-KiB cap. */
	return requestedParagraphs;
}

void SDLBackend_GetSystemDate(DOS_Date *date)
{
    struct tm localTime;

    if (date == 0 || !HostClock_CalendarTime(&localTime, 0))
        return;

    date->year = (uint16)(localTime.tm_year + 0x076C);
    date->month = (byte)(localTime.tm_mon + 0x01);
    date->day = (byte)localTime.tm_mday;
}

void SDLBackend_GetSystemTime(DOS_Time *timeValue)
{
    struct tm localTime;
    uint32_t hundredths;

    if (timeValue == 0 || !HostClock_CalendarTime(&localTime, &hundredths))
        return;

    timeValue->hour = (byte)localTime.tm_hour;
    timeValue->minute = (byte)localTime.tm_min;
    timeValue->second = (byte)localTime.tm_sec;
    timeValue->hundredth = (byte)hundredths;
}

const char *SDLBackend_GetEnvironmentVariable(const char *name)
{
	return getenv(name);
}

