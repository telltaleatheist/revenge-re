// 0800: graphics runtime
// GraphicsRuntime.cpp
// Runtime, codec, tactical-map, and video procedures reconstructed from
// REVENGE.EXE segment 0800.

// Video-buffer, adapter, timer, and arithmetic procedures from segment 0800.
#include "Data/TacticalData.h"
#include "Data/TacticalGraphicsData.h"
#include "Graphics/Graphics.h"
#include "Utility/Utility.h"
#include "revenge/assets.h"
#include "revenge/heap.h"
#include "revenge/platform_backend.h"
#include "revenge/random.h"
#include "revenge/video.h"

#include <string.h>

/* INT 33h's start: the cursor's backing store, the screen's centre, and the
 * events the handler takes (motion and both buttons' presses and releases). */
constexpr uint32_t MOUSE_CURSOR_BACKGROUND_SIZE = 0x0240;
constexpr uint16_t MOUSE_START_X = VIDEO_SCREEN_WIDTH / 0x02;
constexpr uint16_t MOUSE_START_Y = VIDEO_SCREEN_HEIGHT / 0x02;
constexpr uint16_t MOUSE_EVENT_MASK_ALL = 0x001F;


// 0800:2B00: copy an adapter-formatted rectangle between far video buffers.
// Called from:
//      CopyClippedVideoRegion
void CopyVideoRegion(
    int16_t sourceX,
    int16_t sourceY,
    int16_t destinationX,
    int16_t destinationY,
    uint16_t width,
    uint16_t height,
    const uint8_t *sourceBase,
    uint8_t *destinationBase)
{
    // V: Copy a rectangular region between two FAR video buffers. The first
    // coordinate pair is the source origin, followed by width and height, then
    // source offset/segment and destination offset/segment.
    uint8_t adapterType = (uint8_t) ActiveVideoAdapterType;
    uint8_t hostIndexedCopy = FALSE;
    uint16_t sourceXByte;
    uint16_t destinationXByte;
    uint16_t rowByteCount;

    /* The VGA's page is A000:0000; at the hardware boundary it is the SDL
	 * indexed framebuffer. The EGA's A200 shadow page and the CGA's, the
	 * Tandy's and the EGA's layouts (in 0800:2B00-2D36) went with those
	 * adapters; the rebuild is VGA only. */
    if (adapterType == VIDEO_ADAPTER_VGA)
    {
        if ((uintptr_t) sourceBase == 0xA0000000UL)
        {
            sourceBase = SDLBackend_GetIndexedFramebuffer();
            hostIndexedCopy = TRUE;
        }
        if ((uintptr_t) destinationBase == 0xA0000000UL)
        {
            destinationBase = SDLBackend_GetIndexedFramebuffer();
            hostIndexedCopy = TRUE;
        }
    }

    if (hostIndexedCopy != FALSE)
    {
        uint16_t row;
        uint16_t pixelWidth = width << 0x03;
        uint16_t sourcePixelX = (uint16_t) sourceX << 0x03;
        uint16_t destinationPixelX = (uint16_t) destinationX << 0x03;

        /* 0800:2C1C-2C2A: each row by rep movsb, SI and DI 16-bit, so an
		 * offset past FFFFh carries on at the start of the segment, and a
		 * row past row 200 of A000 is video memory off the screen. */
        for (row = 0x0000; row < height; ++row)
        {
            uint16_t destinationOffset = (uint16_t) ((uint16_t) (destinationY + row) * VIDEO_SCREEN_WIDTH + destinationPixelX);
            uint16_t sourceOffset = (uint16_t) ((uint16_t) (sourceY + row) * VIDEO_SCREEN_WIDTH + sourcePixelX);
            uint16_t count;

            for (count = 0x0000; count < pixelWidth; ++count)
                destinationBase[destinationOffset++] = sourceBase[sourceOffset++];
        }
        return;
    }

    sourceXByte = sourceX << 0x03;
    destinationXByte = destinationX << 0x03;
    rowByteCount = width << 0x03;

    for (uint16_t row = 0x00; row < height; ++row)
    {
        uint16_t sourcePixelY = sourceY + row;
        uint16_t destinationPixelY = destinationY + row;
        uint16_t sourceScanlineOffset;
        uint16_t destinationScanlineOffset;

        sourceScanlineOffset = sourcePixelY * VGA_BYTES_PER_SCANLINE + sourceXByte;
        destinationScanlineOffset = destinationPixelY * VGA_BYTES_PER_SCANLINE + destinationXByte;

        const uint8_t *sourceRow = sourceBase + sourceScanlineOffset;
        uint8_t *destinationRow = destinationBase + destinationScanlineOffset;
        for (uint16_t columnByte = 0x00;
            columnByte < rowByteCount;
            ++columnByte)
        {
            destinationRow[columnByte] = sourceRow[columnByte];
        }
    }
    return;
}

void FillVideoTileRectangle(
    int16_t x,
    uint16_t y,
    uint16_t width,
    uint16_t height,
    uint8_t color)
{
    // Verified against 0800:2DB0-2EC7.
    // Fill the rectangle (X, Y, width, height) with the supplied logical
    // color. The CGA's, the Tandy's and the EGA's colour packing and layouts
    // (in 0800:2DB0-2EC7) went with them; the rebuild is VGA only.
    uint16_t xByte;
    uint16_t rowByteCount;
    uint16_t firstPixelY = y << 0x03;
    uint16_t scanlineCount = height << 0x03;
    uint8_t packedColor = (uint8_t) color;
    uint8_t *videoBase = Video_ResolveIndexedRenderTarget(ActiveRenderTarget);
    uint16_t pixelX;
    uint16_t pixelWidth;

    /* The original procedure below addresses packed or planar adapter bytes.
	 * SDL exposes the equivalent visible page as one indexed byte per pixel,
	 * so terminate the hardware representation at this lowest video boundary.
	 * X/width and Y/height remain the original 8-pixel tile units. */
    if (videoBase != NULL)
    {
        pixelX = (uint16_t) x << 0x03;
        pixelWidth = width << 0x03;

        for (uint16_t row = 0x00; row < scanlineCount; ++row)
        {
            if ((uint16_t) (firstPixelY + row) >= VIDEO_SCREEN_HEIGHT)
                break;
            if (pixelX < VIDEO_SCREEN_WIDTH)
            {
                uint16_t visibleWidth = pixelWidth;
                if ((uint32_t) pixelX + visibleWidth > VIDEO_SCREEN_WIDTH)
                    visibleWidth = VIDEO_SCREEN_WIDTH - pixelX;
                memset(videoBase +
                        (uint32_t) (firstPixelY + row) * VIDEO_SCREEN_WIDTH + pixelX,
                    packedColor, visibleWidth);
            }
        }
        return;
    }

    xByte = x << 0x03;
    rowByteCount = width << 0x03;

    for (uint16_t row = 0x00; row < scanlineCount; ++row)
    {
        uint16_t pixelY = firstPixelY + row;
        uint16_t scanlineOffset;

        scanlineOffset = pixelY * VGA_BYTES_PER_SCANLINE + xByte;

        uint8_t *videoRow = videoBase + scanlineOffset;
        for (uint16_t columnByte = 0x00;
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
    const uint8_t *tilePixels,
    int16_t x,
    uint16_t y)
{
    // V: Draw one adapter-formatted 8x8 tile from the FAR source at (X,Y).
    // The CGA's, the Tandy's and the EGA's tile layouts and the EGA's plane
    // writes (in 0800:2F05-3037) went with them; the rebuild is VGA only.
    uint16_t xByte;
    uint16_t firstPixelY = y << 0x03;
    uint16_t bytesPerSourceRow;
    uint8_t *videoBase = SDLBackend_GetIndexedFramebuffer();

    xByte = x << 0x03;
    bytesPerSourceRow = TILE_PIXEL_SIZE;

    for (uint16_t row = 0x00; row < TILE_PIXEL_SIZE; ++row)
    {
        uint16_t pixelY = firstPixelY + row;
        uint16_t scanlineOffset;

        scanlineOffset = pixelY * VGA_BYTES_PER_SCANLINE + xByte;

        uint8_t *videoRow = videoBase + scanlineOffset;
        for (uint16_t columnByte = 0x00;
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
    const uint8_t *compressedData,
    uint8_t *destination)
{
    uint16_t remainingBytes = REVENGE_PACKED_SCREEN_BYTE_COUNT;

    while (remainingBytes != 0x00)
    {
        uint8_t control = *compressedData++;
        uint16_t runLength;
        uint16_t runIndex;
        uint16_t repeatRun = control == 0x00 || control >= 0x80;
        uint8_t repeatedValue;

        if (control == 0x00)
        {
            // The stream's word, little-endian at any byte.
            runLength = (uint16_t) (compressedData[0x00] | compressedData[0x01] << 0x08);
            compressedData += 0x02;
        }
        else if (control >= 0x80)
            runLength = (uint8_t) -control;
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
    const uint8_t *compressedData,
    uint8_t *destination)
{
    uint16_t outputIndex = 0x00;

    while (outputIndex < REVENGE_PACKED_SCREEN_BYTE_COUNT)
    {
        uint8_t control = *compressedData++;
        uint16_t runLength;
        uint16_t runIndex;
        uint16_t repeatRun = control == 0x00 || control >= 0x80;
        uint8_t repeatedValue;

        if (control == 0x00)
        {
            // The stream's word, little-endian at any byte.
            runLength = (uint16_t) (compressedData[0x00] | compressedData[0x01] << 0x08);
            compressedData += 0x02;
        }
        else if (control >= 0x80)
            runLength = (uint8_t) -control;
        else
            runLength = control;
        repeatedValue = repeatRun ? *compressedData : 0x00;

        for (runIndex = 0x00; runIndex < runLength && outputIndex < REVENGE_PACKED_SCREEN_BYTE_COUNT;
            ++runIndex, ++outputIndex)
        {
            uint16_t column = outputIndex / VIDEO_SCREEN_HEIGHT;
            uint16_t row = outputIndex % VIDEO_SCREEN_HEIGHT;
            destination[row * PACKED_BYTES_PER_SCANLINE + column] =
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
uint32_t CalculateSelectedWeaponRating(
    uint32_t weaponMask,
    const uint16_t *weaponCounts)
{
    uint32_t rating;
    uint16_t weaponIndex;
    uint16_t maskLow = LOW_WORD(weaponMask);
    uint16_t maskHigh = HIGH_WORD(weaponMask);
    uint16_t selected;

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
        maskLow = (uint16_t) ((maskLow >> 0x01) | ((maskHigh & 0x0001) << 0x0F));
        maskHigh >>= 0x01;
        if (selected != 0x0000)
        {
            rating += (uint32_t) WeaponCombatRatingByType[weaponIndex] *
                (uint8_t) weaponCounts[weaponIndex];
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
static uint8_t TerrainDamageTables[0x0400];

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
uint8_t PseudoRandomState[0x03] = { 0x34, 0x12, 0x76 };

uint16_t GeneratePseudoRandomByte(void)
{
    /* 0800:4D46 -- three-byte Borland-era shift-register state. */
    uint8_t *state = PseudoRandomState;
    uint8_t accumulator;
    uint8_t carry;
    uint8_t nextCarry;

    accumulator = state[0x00];
    accumulator >>= 0x01;
    carry = accumulator & 0x01;
    accumulator >>= 0x01;
    nextCarry = state[0x02] >> 0x07;
    state[0x02] = (uint8_t) (state[0x02] << 0x01) | carry;
    carry = nextCarry;
    nextCarry = state[0x01] >> 0x07;
    state[0x01] = (uint8_t) (state[0x01] << 0x01) | carry;
    carry = nextCarry == 0x00;
    accumulator = (uint8_t) (accumulator - state[0x00] - carry);
    carry = accumulator & 0x01;
    state[0x00] = (uint8_t) (state[0x00] >> 0x01) | (uint8_t) (carry << 0x07);
    return (uint16_t) (state[0x00] ^ state[0x01]);
}

/* 0800:3B9E (NormalizeFarPointer) has no caller left, and 0800:2EC8
 * (CopyFarBytes32), the long multiply (542C) and the DX:AX shifts (54E8,
 * 5507) are Replaced: memcpy and the host's 32-bit arithmetic do them.
 * Every copy was between two different objects, so the original's forward
 * copy and memcpy never differ. */

uint16_t EncodeDisplayBackingRows(
    const uint8_t *source,
    uint8_t *destination,
    uint16_t groupsPerRow,
    uint8_t rowCount,
    int16_t sourceRowSkip)
{
    uint8_t *destinationStart = destination;
    uint8_t row;

    /* The CGA's single bytes (0800:11BD-11C2, 11DA-11ED) and the EGA's
	 * sparse colour masks (0800:11B2-11B7, 11EE-129D, and 129E) went with
	 * those adapters; the rebuild is VGA only. */
    for (row = 0x00; row < rowCount; ++row)
    {
        uint16_t group;

        for (group = 0x0000; group < (uint8_t) groupsPerRow; ++group)
        {
            *destination++ = *source++;
            *destination++ = *source++;
        }
        source += sourceRowSkip;
    }
    return (uint16_t) (destination - destinationStart);
}

/* 0800:5445 (DivideUnsignedLong) is Replaced: its one caller divides by
 * 400h with the host's unsigned 32-bit division. */
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
    MouseCursorBackground = (uint8_t *) Heap_Allocate(MOUSE_CURSOR_BACKGROUND_SIZE, "0800:36C0");
    if (MouseCursorBackground == NULL)
    {
        MouseDriverInstalled = FALSE;
        return;
    }
    MouseCursorRequestedX = MOUSE_START_X;
    MouseY = MOUSE_START_Y;
    MouseDriverDoubleX = FALSE;
    MouseCursorHideDepth = 0x01;
    MouseInputDisabled = FALSE;
    MouseCursorMinX = 0x0000;
    MouseCursorMaxX = VIDEO_SCREEN_MAX_X;
    MouseCursorMinY = 0x0000;
    MouseCursorMaxY = VIDEO_SCREEN_MAX_Y;
    MouseDriverInstalled = TRUE;
    SDLBackend_SetMousePosition(MOUSE_START_X, MOUSE_START_Y);
    SDLBackend_SetMouseVisible(FALSE);
    SDLBackend_SetMouseEventHandler(MOUSE_EVENT_MASK_ALL);
}

void CopyBufferToVideoRegion(
    int16_t xColumn,
    uint16_t yCharacterRow,
    uint16_t widthColumns,
    uint16_t heightCharacterRows,
    const uint8_t *source)
{
    uint16_t row;
    uint16_t pixelWidth;
    uint16_t pixelHeight;
    uint16_t leftX;
    uint16_t topY;
    uint8_t *framebuffer;

    if (xColumn < 0x0000 || xColumn >= VIDEO_SCREEN_8PX_COLUMN_COUNT ||
        yCharacterRow >= VIDEO_SCREEN_HEIGHT / TILE_PIXEL_SIZE ||
        widthColumns == 0x0000 || heightCharacterRows == 0x0000)
        return;
    pixelWidth = (uint16_t) (widthColumns << 0x03);
    pixelHeight = (uint16_t) (heightCharacterRows << 0x03);
    leftX = (uint16_t) xColumn << 0x03;
    topY = yCharacterRow << 0x03;
    if (leftX + pixelWidth > VIDEO_SCREEN_WIDTH)
        pixelWidth = VIDEO_SCREEN_WIDTH - leftX;
    if (topY + pixelHeight > VIDEO_SCREEN_HEIGHT)
        pixelHeight = VIDEO_SCREEN_HEIGHT - topY;
    framebuffer = SDLBackend_GetIndexedFramebuffer();
    for (row = 0x0000; row < pixelHeight; ++row)
    {
        memcpy(framebuffer + (uint32_t) (topY + row) * VIDEO_SCREEN_WIDTH + leftX,
            source, pixelWidth);
        source += pixelWidth;
    }
}

void CopyVideoRegionToBuffer(
    int16_t xColumn,
    uint16_t yCharacterRow,
    uint16_t widthColumns,
    uint16_t heightCharacterRows,
    uint8_t *destination)
{
    uint16_t row;
    uint16_t pixelWidth;
    uint16_t pixelHeight;
    uint16_t leftX;
    uint16_t topY;
    uint8_t *framebuffer;

    if (xColumn < 0x0000 || xColumn >= VIDEO_SCREEN_8PX_COLUMN_COUNT ||
        yCharacterRow >= VIDEO_SCREEN_HEIGHT / TILE_PIXEL_SIZE ||
        widthColumns == 0x0000 || heightCharacterRows == 0x0000)
        return;
    pixelWidth = (uint16_t) (widthColumns << 0x03);
    pixelHeight = (uint16_t) (heightCharacterRows << 0x03);
    leftX = (uint16_t) xColumn << 0x03;
    topY = yCharacterRow << 0x03;
    if (leftX + pixelWidth > VIDEO_SCREEN_WIDTH)
        pixelWidth = VIDEO_SCREEN_WIDTH - leftX;
    if (topY + pixelHeight > VIDEO_SCREEN_HEIGHT)
        pixelHeight = VIDEO_SCREEN_HEIGHT - topY;
    framebuffer = SDLBackend_GetIndexedFramebuffer();
    for (row = 0x0000; row < pixelHeight; ++row)
    {
        memcpy(destination,
            framebuffer + (uint32_t) (topY + row) * VIDEO_SCREEN_WIDTH + leftX,
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

uint16_t DOS_PrintCharacter(char character)
{
    /* 0800:3BBF -- DOS INT 21h/AH=06h. */
    return SDLBackend_DirectConsoleIO((uint8_t) character);
}

void VGA_LoadPalette(const uint8_t *paletteRgbTriplets)
{
    /* 0800:0974 -- BIOS INT 10h/AX=1012h. */
    SDLBackend_LoadVgaPalette(paletteRgbTriplets);
}

void SetVideoAdapterType(uint16_t adapterType)
{
    /* 0800:4DE7 -- original assignment to 4184:0E4C. */
    ActiveVideoAdapterType = adapterType;
}

/* 0800:4EAA, IsVerticalRetraceActive (the input-status port 03DAh's bit 3),
 * is Replaced: its one caller, CalibrateVideoTiming (34B8:00B9), no longer
 * samples the retrace. */

void WaitForVerticalRetraceEdge(uint8_t waitForRetraceStart)
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
    uint16_t x,
    uint16_t yStart,
    uint16_t yEnd,
    uint16_t color)
{
    uint16_t y;
    uint8_t pixelColor = (uint8_t) color;

    /* The CGA's, the Tandy's and the EGA's colour masks (in 0800:4EF3)
	 * went with them; the rebuild is VGA only. */
    for (y = yStart; y <= yEnd; ++y)
        Video_DrawIndexedPixelToActiveTarget(x, y, pixelColor);
}

void DrawHorizontalVideoRun(
    int16_t x,
    uint16_t y,
    uint16_t nativeRunLength,
    uint16_t color)
{
    uint16_t pixelsPerNativeByte;
    uint16_t pixelCount;
    uint16_t pixel;
    uint8_t pixelColor = (uint8_t) color;

    /* The VGA's run is one pixel a byte. The CGA's, the Tandy's and the
	 * EGA's packing (in 0800:29D5-2AFF) went with them. */
    pixelsPerNativeByte = 0x01;

    pixelCount = nativeRunLength * pixelsPerNativeByte;
    for (pixel = 0x00; pixel < pixelCount; ++pixel)
        Video_DrawIndexedPixelToActiveTarget(
            (uint16_t) (x + pixel), y, pixelColor);
}

void XorVideoRectangle(
    int16_t leftX,
    int16_t topY,
    int16_t rightX,
    int16_t bottomY,
    uint8_t color)
{
    int16_t x;
    int16_t y;
    uint8_t pixelColor = color;

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
        int16_t swap = leftX;
        leftX = rightX;
        rightX = swap;
    }
    if (topY > bottomY)
    {
        int16_t swap = topY;
        topY = bottomY;
        bottomY = swap;
    }

    /* The CGA's, the Tandy's and the EGA's colour masks (in 0800:484E)
	 * went with them; the rebuild is VGA only. */
    for (y = topY; y <= bottomY; ++y)
    {
        for (x = leftX; x <= rightX; ++x)
        {
            uint8_t *framebuffer = Video_ResolveIndexedRenderTarget(
                ActiveRenderTarget);
            uint32_t offset = (uint32_t) y * VIDEO_SCREEN_WIDTH + x;

            if (framebuffer == NULL)
                framebuffer = SDLBackend_GetIndexedFramebuffer();
            Video_DrawIndexedPixelToActiveTarget(
                (uint16_t) x, (uint16_t) y,
                (uint8_t) (framebuffer[offset] ^ pixelColor));
        }
    }
}

/* 0800:33A8 (GetLargestFreeConventionalMemoryKB) is Replaced: it asked DOS
 * for A000h paragraphs and answered 280h KiB when they were free. The host
 * has no conventional-memory arena, so its one caller (3390:0A22) reads that
 * answer as HostConventionalMemoryKB. */
