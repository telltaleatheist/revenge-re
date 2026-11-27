#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "revenge/platform_backend.h"
#include "revenge/dos_memory.h"
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
			DOSMemory_SetBlock(NULL, 0x00000000UL, 0x0000);
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
		DOSMemory_SetBlock(dosMemoryBlock, dosMemoryBlockCapacity,
			dosMemoryBlockSegment);
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

