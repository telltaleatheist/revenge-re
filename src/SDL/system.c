#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "revenge/platform_backend.h"
#include "revenge/interrupts.h"
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

static DOS_InterruptHandler interruptHandlers[0x100];
static uint32 legacyInterruptVectors[0x100];
static byte *dosMemoryBlock;
static uint32 dosMemoryBlockBytes;
static uint32 dosMemoryBlockCapacity;
static word16 dosMemoryBlockSegment;
static byte originalDataSegment[0x10000];

void ServiceDigitalPlaybackInterrupt(void);
void PITCalibrationIRQ0Handler(void);

#define ORIGINAL_DATA_SEGMENT_MAPPED_BYTES 0xB8A0

static word16 ReadLittleEndianWord(const byte *bytes)
{
	return (word16)(bytes[0x00] | ((word16)bytes[0x01] << 0x08));
}

static byte *ResolveOriginalDataPointer(word16 dataOffset)
{
	uint32 encodedAddress =
		((uint32)ReadLittleEndianWord(&originalDataSegment[dataOffset + 0x02])
			<< 0x10) |
		ReadLittleEndianWord(&originalDataSegment[dataOffset]);
	return HostFarPointer_Resolve(encodedAddress);
}

/* The program the import accepted (revenge/original_import.h), loaded as DOS
 * loaded it: the load image at segment 0800h after the 100h-byte PSP, each
 * relocated segment word moved by the load segment. These are the bytes the
 * import checked, whole, not the file again. */
static void LoadOriginalModuleImage(byte *programBlock, uint32 blockBytes)
{
	size_t moduleSize = OriginalImport_ImageBytes();
	size_t relocationCount = OriginalImport_RelocationCount();
	size_t relocationIndex;

	if ((uint32) moduleSize + 0x0100UL > blockBytes)
	{
		/* DOS would refuse to load a program larger than the block, and
		 * nothing the game does next works without it. */
		fprintf(stderr, "REVENGE.EXE: its load image (%lu bytes) doesn't fit the "
			"conventional-memory block (%lu bytes)\n", (unsigned long) moduleSize,
			(unsigned long) (blockBytes - 0x0100UL));
		exit(1);
	}
	memcpy(programBlock + 0x0100, OriginalImage, moduleSize);
	for (relocationIndex = 0x0000; relocationIndex < relocationCount;
		++relocationIndex)
	{
		uint32 targetOffset = 0x0100UL + OriginalImport_Relocation(relocationIndex);
		word16 relocatedSegment;

		if (targetOffset + 0x01 >= blockBytes)
			continue;
		relocatedSegment = (word16) (
			ReadLittleEndianWord(&programBlock[targetOffset]) + 0x0800);
		programBlock[targetOffset] = (byte) relocatedSegment;
		programBlock[targetOffset + 0x01] = (byte) (relocatedSegment >> 0x08);
	}
}

/* The initialised part of segment 3A6A from the same image, the rest zero, its
 * relocated segment words moved by the load segment. */
static void LoadOriginalDataSegment(void)
{
	size_t moduleSize = OriginalImport_ImageBytes();
	size_t relocationCount = OriginalImport_RelocationCount();
	size_t relocationIndex;

	memset(originalDataSegment, 0x00, sizeof(originalDataSegment));
	if (moduleSize > ORIGINAL_DATA_SEGMENT_IMAGE_OFFSET)
	{
		size_t initializedByteCount = moduleSize - ORIGINAL_DATA_SEGMENT_IMAGE_OFFSET;
		if (initializedByteCount > sizeof(originalDataSegment))
			initializedByteCount = sizeof(originalDataSegment);
		memcpy(originalDataSegment, &OriginalImage[ORIGINAL_DATA_SEGMENT_IMAGE_OFFSET],
			initializedByteCount);
	}

	for (relocationIndex = 0x0000; relocationIndex < relocationCount;
		++relocationIndex)
	{
		uint32 targetLinear = OriginalImport_Relocation(relocationIndex);
		word16 dataOffset;
		word16 relocatedSegment;

		if (targetLinear < ORIGINAL_DATA_SEGMENT_IMAGE_OFFSET ||
			targetLinear >= ORIGINAL_DATA_SEGMENT_IMAGE_OFFSET + 0xFFFFUL)
			continue;
		dataOffset = (word16)(targetLinear - ORIGINAL_DATA_SEGMENT_IMAGE_OFFSET);
		relocatedSegment = (word16)(
			ReadLittleEndianWord(&originalDataSegment[dataOffset]) + 0x0800);
		originalDataSegment[dataOffset] = (byte)relocatedSegment;
		originalDataSegment[dataOffset + 0x01] = (byte)(relocatedSegment >> 0x08);
	}
}

void SDLBackend_InitializeDOSMemoryArena(void)
{
	word16 graphicsBufferIndex;
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

	/* The DOS loader copied the initialized portion of segment 3A6A from the
	 * MZ image, zero-filled its BSS tail, and applied load-segment relocations.
	 * Recreate that data edge so preserved DS:offset expressions address the
	 * same bytes. Named globals are registered first as overlays where the C17
	 * transcription exposes an independently typed object. */
	LoadOriginalDataSegment();
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
	REGISTER_DATA_OBJECT(ScenarioResourcePathPointer, 0x1554);
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

	/* These initialized DS words are represented as typed C globals because
	 * they are used throughout the preserved UI code.  Hydrate them from the
	 * loaded image just as the DOS loader did for their original storage. */
	UiNavigationUpKey =
		ReadLittleEndianWord(&originalDataSegment[0x6242]);
	UiNavigationDownKey =
		ReadLittleEndianWord(&originalDataSegment[0x6244]);
	UiPreviousRegionKey =
		ReadLittleEndianWord(&originalDataSegment[0x6246]);
	UiNextRegionKey =
		ReadLittleEndianWord(&originalDataSegment[0x6248]);
	memcpy(TextWindowContexts, &originalDataSegment[0x063A],
		sizeof(TextWindowContexts));
	memcpy(UiRegionTable, &originalDataSegment[0x07AA],
		sizeof(UiRegionTable));
	SecondaryScreenBuffer = ResolveOriginalDataPointer(0x83DE);
	PlanarScreenBuffer = ResolveOriginalDataPointer(0x83E2);
	ActiveRenderTarget = ResolveOriginalDataPointer(0x7D3B);
	for (graphicsBufferIndex = 0x00; graphicsBufferIndex < 0x20;
		++graphicsBufferIndex)
	{
		SharedGraphicsBackingBuffers[graphicsBufferIndex] =
			ResolveOriginalDataPointer((word16)(0x83EE + graphicsBufferIndex * 0x04));
	}
	PrimaryScreenBuffer = ResolveOriginalDataPointer(0xB65E);
	VideoFramebuffer = ResolveOriginalDataPointer(0xB664);
	TacticalStatusBlinkCountdown =
		ReadLittleEndianWord(&originalDataSegment[0x2F36]);
	DisplayedTacticalStatusRow =
		ReadLittleEndianWord(&originalDataSegment[0x2F38]);
	memcpy(TacticalStatusRowInvalidation,
		&originalDataSegment[0xB574],
		sizeof(TacticalStatusRowInvalidation));
	memcpy(TacticalImpactQueue, &originalDataSegment[0xB2F4],
		sizeof(TacticalImpactQueue));
	TacticalStatusBlinkVisible = originalDataSegment[0x86BD];
	memcpy(SceneNarrationEventFlags,
		&originalDataSegment[0xAE63], sizeof(SceneNarrationEventFlags));

#define HYDRATE_DATA_OBJECT(object, offset) \
	memcpy(&(object), &originalDataSegment[(offset)], sizeof(object))
	HYDRATE_DATA_OBJECT(TrainingScenarioCompletionFlags, 0x00A0);
	HYDRATE_DATA_OBJECT(SaveDirectory, 0x18CE);
	HYDRATE_DATA_OBJECT(SaveCampaignSnapshot, 0x1956);
	HYDRATE_DATA_OBJECT(CampaignMessageTable, 0x2B54);
	HYDRATE_DATA_OBJECT(LoadedSceneFile, 0x71A8);
	HYDRATE_DATA_OBJECT(SceneUnitRoutes, 0x86C1);
	HYDRATE_DATA_OBJECT(SceneInstructionBaseOffset, 0xA862);
	HYDRATE_DATA_OBJECT(SceneInstructionCursor, 0xA9D9);
	HYDRATE_DATA_OBJECT(ScenarioResourcePathPointer, 0x1554);
	HYDRATE_DATA_OBJECT(TacticalUpdateDelayReloadTicks, 0x17DA);
	HYDRATE_DATA_OBJECT(TacticalGameSpeedIndex, 0x17DC);
	HYDRATE_DATA_OBJECT(LanceEditorFirstBriefingPending, 0x30C4);
	HYDRATE_DATA_OBJECT(LanceEditorBriefingPresented, 0x30C6);
	HYDRATE_DATA_OBJECT(RandomUnitTypeCount, 0x3118);
#undef HYDRATE_DATA_OBJECT
}

_Noreturn void SDLBackend_TerminateProcess(byte exitCode)
{
	SDLBackend_Close();
	exit((int) exitCode);
}

void RevengeHost_EnableInterruptDelivery(void)
{
    /* Timer services are dispatched cooperatively by the host main loop, so
       they cannot pre-empt an original procedure between these boundaries. */
}

void RevengeHost_DisableInterruptDelivery(void)
{
    /* See RevengeHost_EnableInterruptDelivery. */
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
		LoadOriginalModuleImage(dosMemoryBlock, dosMemoryBlockCapacity);
		HostFarPointer_RegisterAt(dosMemoryBlock, dosMemoryBlockCapacity,
			(uint32) dosMemoryBlockSegment << 0x10);
	}
	if (memorySegment != dosMemoryBlockSegment ||
		requestedBytes > dosMemoryBlockCapacity)
		return (word16) (dosMemoryBlockCapacity >> 0x04);
	dosMemoryBlockBytes = requestedBytes;
	return 0xFFFF;
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

void SDLBackend_InvokeInterrupt(
	byte interruptNumber,
	const InterruptRegisterBlock *inputRegisters,
	InterruptRegisterBlock *outputRegisters,
	InterruptSegmentState *segmentState)
{
	*outputRegisters = *inputRegisters;
	outputRegisters->carry = FALSE;
	outputRegisters->flags = 0x0000;

	if (interruptNumber == 0x10)
	{
		byte service = (byte) (inputRegisters->ax >> 0x08);
		if (service == 0x00)
			SDLBackend_SetVideoMode((byte) inputRegisters->ax);
		else if (service == 0x0F)
			outputRegisters->ax = SDLBackend_GetVideoMode();
	}
	(void) segmentState;
}

void SDLBackend_SetInterruptVector(
    byte interruptNumber,
    DOS_InterruptHandler handler)
{
    /* Host callbacks replace the DOS IVT. Registration is retained so timer,
     * keyboard, and critical-error seams can be connected incrementally. */
    interruptHandlers[interruptNumber] = handler;
}

void SDLBackend_DispatchInterrupt(byte interruptNumber)
{
	DOS_InterruptHandler handler = interruptHandlers[interruptNumber];

	if (handler != NULL)
		handler();
}

DOS_InterruptHandler SDLBackend_SwapInterruptHandler(
    byte interruptNumber,
    DOS_InterruptHandler handler)
{
    DOS_InterruptHandler previousHandler;

    previousHandler = interruptHandlers[interruptNumber];
    interruptHandlers[interruptNumber] = handler;
    return previousHandler;
}

uint32 SDLBackend_SwapInterruptVector(
    byte interruptNumber,
    uint32 newVector)
{
    uint32 oldVector;

    oldVector = legacyInterruptVectors[interruptNumber];
    legacyInterruptVectors[interruptNumber] = newVector;
    return oldVector;
}

void SDLBackend_ChainInterruptVector(uint32 interruptVector)
{
	DOS_InterruptHandler handler = NULL;

	switch (interruptVector)
	{
	case 0x375F0A1DUL:
		handler = ServiceDigitalPlaybackInterrupt;
		break;
	case 0x38AE00D7UL:
		handler = PITCalibrationIRQ0Handler;
		break;
	}
	if (handler != NULL)
		handler();
}

void SDLBackend_DispatchInstalledInterruptVector(byte interruptNumber)
{
	SDLBackend_ChainInterruptVector(legacyInterruptVectors[interruptNumber]);
}

void SDLBackend_ProgramPITChannel0(word16 divisor)
{
    /* The cooperative host timer has no programmable PIT. The preserved
       procedure still owns the divisor state exchanged around this call. */
    (void)divisor;
}

void SDLBackend_ProgramPITChannel0Mode(byte mode, word16 divisor)
{
    (void)mode;
    (void)divisor;
}

word16 SDLBackend_LatchPITChannel0Count(void)
{
    return 0x0000;
}

void SDLBackend_WaitPITChannel0Interrupt(void)
{
    SDLBackend_Delay(0x0037);
}

word16 SDLBackend_SaveAndDisableInterrupts(void)
{
    RevengeHost_DisableInterruptDelivery();
    return 0x0200;
}

void SDLBackend_RestoreInterruptFlags(word16 flags)
{
    if ((flags & 0x0200) != 0x0000)
        RevengeHost_EnableInterruptDelivery();
}

void SDLBackend_AcknowledgeInterrupt(byte interruptNumber)
{
    /* Host callbacks do not require a programmable-interrupt-controller EOI. */
    (void)interruptNumber;
}

uint32 SDLBackend_GetBIOSTickCount(void)
{
    /* DOS advances 0000:046C about 18.2 times per second. The game reads
     * it in spin loops (34B8:000A), so a read is a pass that waits on time. */
    HostClock_Step();
    return (uint32)((SDLBackend_Milliseconds() * UINT64_C(0x00B6)) /
        UINT64_C(0x2710));
}
