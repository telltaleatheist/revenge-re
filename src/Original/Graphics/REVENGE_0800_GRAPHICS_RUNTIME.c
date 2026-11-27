// REVENGE_0800_GRAPHICS_RUNTIME.c
// Runtime, codec, tactical-map, and video procedures reconstructed from
// REVENGE.EXE segment 0800.

// Video-buffer, adapter, timer, and arithmetic procedures from segment 0800.
#include "REVENGE.h"
#include "revenge/original_data.h"
#include "revenge/platform_backend.h"
#include "revenge/random.h"
#include "revenge/video.h"

#include <string.h>

#define VGA_BYTES_PER_SCANLINE         0x0140

// 0800:2B00: copy an adapter-formatted rectangle between far video buffers.
// Called from:
//      CopyClippedVideoRegion
void CopyVideoRegion(
	int16 sourceX,
	int16 sourceY,
	int16 destinationX,
	int16 destinationY,
	word16 width,
	word16 height,
	const byte *sourceBase,
	byte *destinationBase)
{
	// V: Copy a rectangular region between two FAR video buffers. The first
	// coordinate pair is the source origin, followed by width and height, then
	// source offset/segment and destination offset/segment.
	byte adapterType = (byte) ActiveVideoAdapterType;
	byte hostIndexedCopy = FALSE;
	word16 sourceXByte;
	word16 destinationXByte;
	word16 rowByteCount;

	/* The VGA's page is A000:0000; at the hardware boundary it is the SDL
	 * indexed framebuffer. The EGA's A200 shadow page and the CGA's, the
	 * Tandy's and the EGA's layouts (in 0800:2B00-2D36) went with those
	 * adapters; the rebuild is VGA only. */
	if (adapterType == VIDEO_ADAPTER_VGA)
	{
		if ((uintptr_t)sourceBase == 0xA0000000UL)
		{
			sourceBase = SDLBackend_GetIndexedFramebuffer();
			hostIndexedCopy = TRUE;
		}
		if ((uintptr_t)destinationBase == 0xA0000000UL)
		{
			destinationBase = SDLBackend_GetIndexedFramebuffer();
			hostIndexedCopy = TRUE;
		}
	}

	if (hostIndexedCopy != FALSE)
	{
		word16 row;
		word16 pixelWidth = width << 0x03;
		word16 sourcePixelX = (word16)sourceX << 0x03;
		word16 destinationPixelX = (word16)destinationX << 0x03;

		/* 0800:2C1C-2C2A: each row by rep movsb, SI and DI 16-bit, so an
		 * offset past FFFFh carries on at the start of the segment, and a
		 * row past row 200 of A000 is video memory off the screen. */
		for (row = 0x0000; row < height; ++row)
		{
			word16 destinationOffset = (word16)
				((word16) (destinationY + row) * VIDEO_SCREEN_WIDTH + destinationPixelX);
			word16 sourceOffset = (word16)
				((word16) (sourceY + row) * VIDEO_SCREEN_WIDTH + sourcePixelX);
			word16 count;

			for (count = 0x0000; count < pixelWidth; ++count)
				destinationBase[destinationOffset++] = sourceBase[sourceOffset++];
		}
		return;
	}

	sourceXByte = sourceX << 0x03;
	destinationXByte = destinationX << 0x03;
	rowByteCount = width << 0x03;

	for (word16 row = 0x00; row < height; ++row)
	{
		word16 sourcePixelY = sourceY + row;
		word16 destinationPixelY = destinationY + row;
		word16 sourceScanlineOffset;
		word16 destinationScanlineOffset;

		sourceScanlineOffset = sourcePixelY * VGA_BYTES_PER_SCANLINE + sourceXByte;
		destinationScanlineOffset = destinationPixelY * VGA_BYTES_PER_SCANLINE + destinationXByte;

		const byte *sourceRow = sourceBase + sourceScanlineOffset;
		byte *destinationRow = destinationBase + destinationScanlineOffset;
		for (word16 columnByte = 0x00;
			columnByte < rowByteCount;
			++columnByte)
		{
			destinationRow[columnByte] = sourceRow[columnByte];
		}
	}
	return;

}


void FillVideoTileRectangle(
	int16 x,
	word16 y,
	word16 width,
	word16 height,
	byte color)
{
	// Verified against 0800:2DB0-2EC7.
	// Fill the rectangle (X, Y, width, height) with the supplied logical
	// color. The CGA's, the Tandy's and the EGA's colour packing and layouts
	// (in 0800:2DB0-2EC7) went with them; the rebuild is VGA only.
	word16 xByte;
	word16 rowByteCount;
	word16 firstPixelY = y << 0x03;
	word16 scanlineCount = height << 0x03;
	byte packedColor = (byte)color;
	byte *videoBase = Video_ResolveIndexedRenderTarget(ActiveRenderTarget);
	word16 pixelX;
	word16 pixelWidth;

	/* The original procedure below addresses packed or planar adapter bytes.
	 * SDL exposes the equivalent visible page as one indexed byte per pixel,
	 * so terminate the hardware representation at this lowest video boundary.
	 * X/width and Y/height remain the original 8-pixel tile units. */
	if (videoBase != NULL)
	{
		pixelX = (word16)x << 0x03;
		pixelWidth = width << 0x03;

		for (word16 row = 0x00; row < scanlineCount; ++row)
		{
			if ((word16)(firstPixelY + row) >= VIDEO_SCREEN_HEIGHT)
				break;
			if (pixelX < VIDEO_SCREEN_WIDTH)
			{
				word16 visibleWidth = pixelWidth;
				if ((uint32)pixelX + visibleWidth > VIDEO_SCREEN_WIDTH)
					visibleWidth = VIDEO_SCREEN_WIDTH - pixelX;
				memset(videoBase +
					(uint32)(firstPixelY + row) * VIDEO_SCREEN_WIDTH + pixelX,
					packedColor, visibleWidth);
			}
		}
		return;
	}

	xByte = x << 0x03;
	rowByteCount = width << 0x03;

	for (word16 row = 0x00; row < scanlineCount; ++row)
	{
		word16 pixelY = firstPixelY + row;
		word16 scanlineOffset;

		scanlineOffset = pixelY * VGA_BYTES_PER_SCANLINE + xByte;

		byte *videoRow = videoBase + scanlineOffset;
		for (word16 columnByte = 0x00;
			columnByte < rowByteCount;
			++columnByte)
		{
			videoRow[columnByte] = packedColor;
		}
	}
	return;

}


// 0800:2F05: draw one adapter-formatted 8x8 tile at tile coordinates X,Y.
// Called from:
//      DrawActiveTextWindowFrame
//      DrawRepeatingFrameTileSequence
void Draw8x8VideoTile(
	const byte *tilePixels,
	int16 x,
	word16 y)
{
	// V: Draw one adapter-formatted 8x8 tile from the FAR source at (X,Y).
	// The CGA's, the Tandy's and the EGA's tile layouts and the EGA's plane
	// writes (in 0800:2F05-3037) went with them; the rebuild is VGA only.
	word16 xByte;
	word16 firstPixelY = y << 0x03;
	word16 bytesPerSourceRow;
	byte *videoBase = SDLBackend_GetIndexedFramebuffer();

	xByte = x << 0x03;
	bytesPerSourceRow = 0x08;

	for (word16 row = 0x00; row < 0x08; ++row)
	{
		word16 pixelY = firstPixelY + row;
		word16 scanlineOffset;

		scanlineOffset = pixelY * VGA_BYTES_PER_SCANLINE + xByte;

		byte *videoRow = videoBase + scanlineOffset;
		for (word16 columnByte = 0x00;
			columnByte < bytesPerSourceRow;
			++columnByte)
		{
			videoRow[columnByte] =
				tilePixels[row * bytesPerSourceRow + columnByte];
		}
	}
	return;

}


// 0800:33D6: decode the signed-run stream into 7D00h linear bytes.
void DecodeLinearRleScreen(
	const byte *compressedData,
	byte *destination)
{
	word16 remainingBytes = 0x7D00;

	while (remainingBytes != 0x00)
	{
		byte control = *compressedData++;
		word16 runLength;
		word16 runIndex;
		word16 repeatRun = control == 0x00 || control >= 0x80;
		byte repeatedValue;

		if (control == 0x00)
		{
			runLength = *(const word16 *)compressedData;
			compressedData += 0x02;
		}
		else if (control >= 0x80)
			runLength = (byte)-control;
		else
			runLength = control;
		repeatedValue = repeatRun ? *compressedData : 0x00;

		for (runIndex = 0x00; runIndex < runLength && remainingBytes != 0x00;
			++runIndex, --remainingBytes)
			*destination++ = repeatRun ? repeatedValue : *compressedData++;

		if (repeatRun)
			++compressedData;
	}
}

// 0800:342D: decode the same stream down 200-pixel columns in a
// 160-byte-stride 320x200 packed page.
void DecodeColumnMajorRleScreen(
	const byte *compressedData,
	byte *destination)
{
	word16 outputIndex = 0x00;

	while (outputIndex < 0x7D00)
	{
		byte control = *compressedData++;
		word16 runLength;
		word16 runIndex;
		word16 repeatRun = control == 0x00 || control >= 0x80;
		byte repeatedValue;

		if (control == 0x00)
		{
			runLength = *(const word16 *)compressedData;
			compressedData += 0x02;
		}
		else if (control >= 0x80)
			runLength = (byte)-control;
		else
			runLength = control;
		repeatedValue = repeatRun ? *compressedData : 0x00;

		for (runIndex = 0x00; runIndex < runLength && outputIndex < 0x7D00;
			++runIndex, ++outputIndex)
		{
			word16 column = outputIndex / 0x00C8;
			word16 row = outputIndex % 0x00C8;
			destination[row * 0x00A0 + column] =
				repeatRun ? repeatedValue : *compressedData++;
		}

		if (repeatRun)
			++compressedData;
	}
}


// 0800:072C: Register uint32 CalculateSelectedWeaponRating(
//     Stack uint32 weaponMask, Stack const byte *weaponCounts)
// Called from:
//      RecalculateUnitCombatRatings
uint32 CalculateSelectedWeaponRating(
	uint32 weaponMask,
	const uint16 *weaponCounts)
{
	uint32 rating;
	word16 weaponIndex;
	word16 maskLow = LOW_WORD(weaponMask);
	word16 maskHigh = HIGH_WORD(weaponMask);
	word16 selected;

	rating = 0x00000000UL;
	for (weaponIndex = 0x0000;
		weaponIndex < WEAPON_DEFINITION_COUNT;
		++weaponIndex)
	{
		/* 0800:075A tests the mask for zero with OR BX,BP, which leaves the
		 * high word ORed into the low word on every pass. */
		maskLow |= maskHigh;
		if (maskLow == 0x0000)
			break;

		/* SHR BP,1 then RCR BX,1: the 32-bit shift, its low bit the weapon's. */
		selected = maskLow & 0x0001;
		maskLow = (word16) ((maskLow >> 0x01) | ((maskHigh & 0x0001) << 0x0F));
		maskHigh >>= 0x01;
		if (selected != 0x0000)
		{
			rating += (uint32) WeaponCombatRatingByType[weaponIndex]
				* (byte) weaponCounts[weaponIndex];
		}
	}

	return rating;
}

/* CS:01C9-05C8 in the original, space in its code segment: the damaged-
 * sprite lookup's 300h bytes, which ICONLIST.DAT fills for the active icon
 * set, then the scenario classification map's 100h, which
 * BuildScenarioTerrainDamageCapabilityMap writes whole for each scenario.
 * Nothing reads either before it is written, so the port's own storage
 * stands for it, not the program image. */
static byte TerrainDamageTables[0x0400];

void InstallDamagedTerrainSpriteLookup(void)
{
	/* 0800:0959 publishes CS:01C9 through DS:7A73. BuildTacticalTerrainCellMaps
	 * indexes the lookup directly at 0800:0694 using (damageState - 1) in AH
	 * and terrainType in AL. The classification map at CS:04C9 follows it; it
	 * is not the damaged-sprite lookup. */
	TerrainDamageCapabilityTable = TerrainDamageTables;
	DamagedTerrainSpriteLookup = TerrainDamageCapabilityTable;
}

/* 4184:0E48 -- the generator's three-byte shift-register state, which
 * nothing seeds: every run draws the same sequence. */
byte PseudoRandomState[0x03] = { 0x34, 0x12, 0x76 };

word16 GeneratePseudoRandomByte(void)
{
	/* 0800:4D46 -- three-byte Borland-era shift-register state. */
	byte *state = PseudoRandomState;
	byte accumulator;
	byte carry;
	byte nextCarry;

	accumulator = state[0x00];
	accumulator >>= 0x01;
	carry = accumulator & 0x01;
	accumulator >>= 0x01;
	nextCarry = state[0x02] >> 0x07;
	state[0x02] = (byte) (state[0x02] << 0x01) | carry;
	carry = nextCarry;
	nextCarry = state[0x01] >> 0x07;
	state[0x01] = (byte) (state[0x01] << 0x01) | carry;
	carry = nextCarry == 0x00;
	accumulator = (byte) (accumulator - state[0x00] - carry);
	carry = accumulator & 0x01;
	state[0x00] = (byte) (state[0x00] >> 0x01) | (byte) (carry << 0x07);
	return (word16) (state[0x00] ^ state[0x01]);
}

uint32 NormalizeFarPointer(word16 offset, word16 segment)
{
	/* 0800:3B9E -- normalize to an offset in 0000h..000Fh. */
	return ((uint32) (segment + (offset >> 0x04)) << 0x10) |
		(offset & 0x000F);
}

void CopyFarBytes32(
	const void *source,
	void *destination,
	uint32 byteCount)
{
	const byte *sourceByte = (const byte *) source;
	byte *destinationByte = (byte *) destination;

	/* 0800:2EC8 is a forward REP MOVSW/MOVSB copy, not memmove. */
	while (byteCount-- != 0x00000000UL)
		*destinationByte++ = *sourceByte++;
}

uint32 MultiplyUnsignedLong(
	word16 leftLow,
	word16 rightHigh,
	word16 leftHigh,
	word16 rightLow)
{
	/* 0800:542C returns the low 32 bits of unsigned DX:AX * CX:BX. */
	return (uint32) (((uint32) leftHigh << 0x10) | leftLow) *
		(uint32) (((uint32) rightHigh << 0x10) | rightLow);
}

uint32 ShiftLeft32_DXAX(
	word16 valueLow,
	byte shiftCount,
	word16 valueHigh)
{
	uint32 value = ((uint32) valueHigh << 0x10) | valueLow;

	/* All callers supply counts in the original helper's defined 0..31
	 * domain. The two assembly branches split that domain at 10h. */
	return value << shiftCount;
}

uint32 ShiftRight32_DXAX(
	word16 valueLow,
	byte shiftCount,
	word16 valueHigh)
{
	uint32 value = ((uint32) valueHigh << 0x10) | valueLow;

	return value >> shiftCount;
}

word16 EncodeDisplayBackingRows(
	const byte *source,
	byte *destination,
	word16 groupsPerRow,
	byte rowCount,
	int16 sourceRowSkip)
{
	byte *destinationStart = destination;
	byte row;

	/* The CGA's single bytes (0800:11BD-11C2, 11DA-11ED) and the EGA's
	 * sparse colour masks (0800:11B2-11B7, 11EE-129D, and 129E) went with
	 * those adapters; the rebuild is VGA only. */
	for (row = 0x00; row < rowCount; ++row)
	{
		word16 group;

		for (group = 0x0000; group < (byte) groupsPerRow; ++group)
		{
			*destination++ = *source++;
			*destination++ = *source++;
		}
		source += sourceRowSkip;
	}
	return (word16) (destination - destinationStart);
}

uint32 DivideUnsignedLong(uint32 dividend, uint32 divisor)
{
	return dividend / divisor;
}
/* 0800:2440: the game's INT 1Ch handler, 18.2 times a second. Host: the
 * game's clock calls it (SDLBackend_SetGameClockHandler) at the BIOS tick's
 * rate. Its far jump to the saved vector (the BIOS's own INT 1Ch, an IRET)
 * goes with the vector table. */
void GameClockInt1CHandler(void)
{
	if (TacticalUpdateDelayTicks != 0x00)
		--TacticalUpdateDelayTicks;
	if (TacticalKeyRepeatDelayTicks != 0x00)
		--TacticalKeyRepeatDelayTicks;
	++GameTimerTicks;
	++GameAnimationTicks;
}

/* 0800:23F0: saves the INT 1Ch vector and installs 0800:2440 (INT 21h
 * AH=35h and AH=25h). Host: the game's clock calls the handler from now. */
void InstallGameClockTimer(void)
{
	SDLBackend_SetGameClockHandler(GameClockInt1CHandler);
}

/* 0800:241E: puts the saved INT 1Ch vector back (INT 21h AH=25h). Host: the
 * game's clock stops calling the handler. */
void RestoreGameClockTimer(void)
{
	SDLBackend_SetGameClockHandler(NULL);
}

/* 0800:34F4 (STI) and 0800:34F6 (CLI) are Replaced: the host's timer and
 * input services run between the game's calls, never inside one, so there
 * is no interrupt flag to set or clear. */

void InitializeMouseSupport(void)
{
	/* 0800:36A6 reserves the 0240h-byte cursor backing store, initializes
	 * the 320x200 bounds, centers the cursor, and installs the INT 33h
	 * handler. SDL is the driver: always present (the probe at 36E5-3728),
	 * one pixel a unit (the doubling test at 372A-373F), hidden by the
	 * reset; the handler's installation (AX=0Ch, 375B-3767) routes SDL's
	 * mouse events to MouseEventCallback (0800:3772). */
	MouseCursorBackground = Heap_Allocate(0x0240, "0800:36C0");
	if (MouseCursorBackground == NULL)
	{
		MouseDriverInstalled = FALSE;
		return;
	}
	MouseCursorRequestedX = 0x00A0;
	MouseY = 0x0064;
	MouseDriverDoubleX = FALSE;
	MouseCursorHideDepth = 0x01;
	MouseInputDisabled = FALSE;
	MouseCursorMinX = 0x0000;
	MouseCursorMaxX = 0x013F;
	MouseCursorMinY = 0x0000;
	MouseCursorMaxY = 0x00C7;
	MouseDriverInstalled = TRUE;
	SDLBackend_SetMousePosition(0x00A0, 0x0064);
	SDLBackend_SetMouseVisible(FALSE);
	SDLBackend_SetMouseEventHandler(0x001F);
}

void CopyBufferToVideoRegion(
	int16 xColumn,
	word16 yCharacterRow,
	word16 widthColumns,
	word16 heightCharacterRows,
	const byte *source)
{
	word16 row;
	word16 pixelWidth;
	word16 pixelHeight;
	word16 leftX;
	word16 topY;
	byte *framebuffer;

	if (xColumn < 0x0000 || xColumn >= VIDEO_SCREEN_8PX_COLUMN_COUNT ||
		yCharacterRow >= VIDEO_SCREEN_HEIGHT / 0x0008 ||
		widthColumns == 0x0000 || heightCharacterRows == 0x0000)
		return;
	pixelWidth = (word16) (widthColumns << 0x03);
	pixelHeight = (word16) (heightCharacterRows << 0x03);
	leftX = (word16) xColumn << 0x03;
	topY = yCharacterRow << 0x03;
	if (leftX + pixelWidth > VIDEO_SCREEN_WIDTH)
		pixelWidth = VIDEO_SCREEN_WIDTH - leftX;
	if (topY + pixelHeight > VIDEO_SCREEN_HEIGHT)
		pixelHeight = VIDEO_SCREEN_HEIGHT - topY;
	framebuffer = SDLBackend_GetIndexedFramebuffer();
	for (row = 0x0000; row < pixelHeight; ++row)
	{
		memcpy(framebuffer + (uint32) (topY + row) * VIDEO_SCREEN_WIDTH + leftX,
			source, pixelWidth);
		source += pixelWidth;
	}
}

void CopyVideoRegionToBuffer(
	int16 xColumn,
	word16 yCharacterRow,
	word16 widthColumns,
	word16 heightCharacterRows,
	byte *destination)
{
	word16 row;
	word16 pixelWidth;
	word16 pixelHeight;
	word16 leftX;
	word16 topY;
	byte *framebuffer;

	if (xColumn < 0x0000 || xColumn >= VIDEO_SCREEN_8PX_COLUMN_COUNT ||
		yCharacterRow >= VIDEO_SCREEN_HEIGHT / 0x0008 ||
		widthColumns == 0x0000 || heightCharacterRows == 0x0000)
		return;
	pixelWidth = (word16) (widthColumns << 0x03);
	pixelHeight = (word16) (heightCharacterRows << 0x03);
	leftX = (word16) xColumn << 0x03;
	topY = yCharacterRow << 0x03;
	if (leftX + pixelWidth > VIDEO_SCREEN_WIDTH)
		pixelWidth = VIDEO_SCREEN_WIDTH - leftX;
	if (topY + pixelHeight > VIDEO_SCREEN_HEIGHT)
		pixelHeight = VIDEO_SCREEN_HEIGHT - topY;
	framebuffer = SDLBackend_GetIndexedFramebuffer();
	for (row = 0x0000; row < pixelHeight; ++row)
	{
		memcpy(destination,
			framebuffer + (uint32) (topY + row) * VIDEO_SCREEN_WIDTH + leftX,
			pixelWidth);
		destination += pixelWidth;
	}
}

/* 0800:37FE-3824: the handler removed (INT 33h AX=0Ch, CX=0) when it was
 * installed. */
void ShutdownMouseSupport(void)
{
	if (MouseDriverInstalled != FALSE)
		SDLBackend_SetMouseEventHandler(0x0000);
}

uint16 DOS_PrintCharacter(char character)
{
	/* 0800:3BBF -- DOS INT 21h/AH=06h. */
	return SDLBackend_DirectConsoleIO((byte) character);
}

void DOS_SelectDefaultDrive(byte zeroBasedDrive)
{
	/* 0800:4D81 -- DOS INT 21h/AH=0Eh. */
	SDLBackend_SelectDefaultDrive(zeroBasedDrive);
}

void VGA_LoadPalette(const byte *paletteRgbTriplets)
{
	/* 0800:0974 -- BIOS INT 10h/AX=1012h. */
	SDLBackend_LoadVgaPalette(paletteRgbTriplets);
}

void SetVideoAdapterType(uint16 adapterType)
{
	/* 0800:4DE7 -- original assignment to 4184:0E4C. */
	ActiveVideoAdapterType = adapterType;
}

/* 0800:4EAA, IsVerticalRetraceActive (the input-status port 03DAh's bit 3),
 * is Replaced: its one caller, CalibrateVideoTiming (34B8:00B9), no longer
 * samples the retrace. */

void WaitForVerticalRetraceEdge(byte waitForRetraceStart)
{
	/* 0800:4EC2 -- input-status port 03DAh polling loops. */
	SDLBackend_WaitVerticalRetrace(waitForRetraceStart);
}

void ClearActiveVideoBuffer(void)
{
	/* 0800:2D59 clears the entire adapter framebuffer: the VGA's 8000h
	 * words, the host's 320x200 indexed image. The CLI and STI around it
	 * (2D64, 2DAA) go: nothing interrupts the host's clear. */
	SDLBackend_ClearIndexedFramebuffer(0x00);
}

void DrawVerticalSpan(
	word16 x,
	word16 yStart,
	word16 yEnd,
	word16 color)
{
	word16 y;
	byte pixelColor = (byte) color;

	/* The CGA's, the Tandy's and the EGA's colour masks (in 0800:4EF3)
	 * went with them; the rebuild is VGA only. */
	for (y = yStart; y <= yEnd; ++y)
		Video_DrawIndexedPixelToActiveTarget(x, y, pixelColor);
}

void DrawHorizontalVideoRun(
	int16 x,
	word16 y,
	word16 nativeRunLength,
	word16 color)
{
	word16 pixelsPerNativeByte;
	word16 pixelCount;
	word16 pixel;
	byte pixelColor = (byte) color;

	/* The VGA's run is one pixel a byte. The CGA's, the Tandy's and the
	 * EGA's packing (in 0800:29D5-2AFF) went with them. */
	pixelsPerNativeByte = 0x01;

	pixelCount = nativeRunLength * pixelsPerNativeByte;
	for (pixel = 0x00; pixel < pixelCount; ++pixel)
		Video_DrawIndexedPixelToActiveTarget(
			(word16) (x + pixel), y, pixelColor);
}

void XorVideoRectangle(
	int16 leftX,
	int16 topY,
	int16 rightX,
	int16 bottomY,
	byte color)
{
	int16 x;
	int16 y;
	byte pixelColor = color;

	if (leftX < VIDEO_SCREEN_MIN_X)
		leftX = VIDEO_SCREEN_MIN_X;
	else if (leftX > VIDEO_SCREEN_MAX_X)
		leftX = VIDEO_SCREEN_MAX_X;
	if (rightX < VIDEO_SCREEN_MIN_X)
		rightX = VIDEO_SCREEN_MIN_X;
	else if (rightX > VIDEO_SCREEN_MAX_X)
		rightX = VIDEO_SCREEN_MAX_X;
	if (topY < VIDEO_SCREEN_MIN_Y)
		topY = VIDEO_SCREEN_MIN_Y;
	else if (topY > VIDEO_SCREEN_MAX_Y)
		topY = VIDEO_SCREEN_MAX_Y;
	if (bottomY < VIDEO_SCREEN_MIN_Y)
		bottomY = VIDEO_SCREEN_MIN_Y;
	else if (bottomY > VIDEO_SCREEN_MAX_Y)
		bottomY = VIDEO_SCREEN_MAX_Y;

	if (leftX > rightX)
	{
		int16 swap = leftX;
		leftX = rightX;
		rightX = swap;
	}
	if (topY > bottomY)
	{
		int16 swap = topY;
		topY = bottomY;
		bottomY = swap;
	}

	/* The CGA's, the Tandy's and the EGA's colour masks (in 0800:484E)
	 * went with them; the rebuild is VGA only. */
	for (y = topY; y <= bottomY; ++y)
	{
		for (x = leftX; x <= rightX; ++x)
		{
			byte *framebuffer = Video_ResolveIndexedRenderTarget(
				ActiveRenderTarget);
			uint32 offset = (uint32) y * VIDEO_SCREEN_WIDTH + x;

			if (framebuffer == NULL)
				framebuffer = SDLBackend_GetIndexedFramebuffer();
			Video_DrawIndexedPixelToActiveTarget(
				(word16) x, (word16) y,
				(byte) (framebuffer[offset] ^ pixelColor));
		}
	}
}

word16 GetLargestFreeConventionalMemoryKB(void)
{
	word16 paragraphs =
		SDLBackend_GetLargestFreeConventionalMemoryParagraphs(0xA000);

	return paragraphs >= 0xA000 ? 0x0280 : paragraphs >> 0x06;
}


