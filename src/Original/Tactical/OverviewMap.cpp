// 24C5: overview map
// OverviewMap.cpp
// Tactical overview-map rendering and transition helpers.

#include "Data/RuntimeData.h"
#include "Data/StartupData.h"
#include "Data/TacticalData.h"
#include "Graphics/Graphics.h"
#include "Input/Input.h"
#include "Tactical/Tactical.h"
#include "Text/Text.h"
#include "revenge/text.h"

constexpr int OVERVIEW_VIEWPORT_WIDTH_CELLS = 0x13;
constexpr int OVERVIEW_VIEWPORT_HEIGHT_CELLS = 0x0C;
constexpr uint8_t OVERVIEW_INVALID_COORDINATE = 0xFF;
constexpr uint8_t OVERVIEW_FRIENDLY_MARKER_COLOR = 0xDD;
constexpr uint8_t OVERVIEW_OPPOSING_MARKER_COLOR = 0x66;
constexpr uint8_t OVERVIEW_SPECIAL_OPPOSING_COLOR = 0xAA;
constexpr uint8_t OVERVIEW_VIEWPORT_FRAME_COLOR = 0x0F;
constexpr int OVERVIEW_RUNTIME_SLOT_COUNT = 0x09;
#define TACTICAL_OVERVIEW_MAP_BUFFER TacticalOverviewMapBuffer

// These callees are maintained in other segment files or will be promoted in
// the following tactical pass. Their stack contracts are already unambiguous.
void PlotPixel(uint16_t x, uint16_t y, uint8_t color);
void ScrollTacticalViewportToCursor(void);
void RedrawTacticalStatusPanel(void);
void DrawTacticalOverviewViewportFrame(void);
uint16_t GeneratePseudoRandomByte(void);
void BuildTacticalOverviewMapBuffer(
    const uint8_t *source,
    uint8_t *destination,
    uint16_t width,
    uint16_t height);
void ShowTacticalModalMessage(const char *formattedText);
void CopyVideoRegionToBuffer(
    int16_t xColumn,
    uint16_t yCharacterRow,
    uint16_t widthColumns,
    uint16_t heightCharacterRows,
    uint8_t *destination);
void CopyBufferToVideoRegion(
    int16_t xColumn,
    uint16_t yCharacterRow,
    uint16_t widthColumns,
    uint16_t heightCharacterRows,
    const uint8_t *source);
void SelectAndClearTextWindow(uint16_t windowIndex);
void DrawActiveTextWindowBorder(uint16_t color);
void RenderFormattedText(const char *formattedText);
uint16_t WaitForDismissInput(void);
void HideMouseCursorOverlay(void);
void ShowMouseCursorOverlay(void);
void ClearActiveVideoBuffer(void);
void RefreshTacticalDisplay(uint16_t advanceEffects);
void DrawTacticalOverviewMap(void);
void DrawClippedAxisAlignedLine(
    int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);

// 24C5:0002
void EnterTacticalOverview(void)
{
    uint16_t showClockNotice;
    uint8_t *screenBuffer;

    showClockNotice = FALSE;
    TacticalOverviewViewportDirty = FALSE;
    if (TacticalOverviewClockNoticeActive != FALSE &&
        CampaignClockState[0x01] ==
            (int16_t) LastOverviewNoticeHour)
    {
        showClockNotice = TRUE;
    }

    if (showClockNotice == FALSE)
    {
        TacticalOverviewClockNoticeActive = FALSE;
        if (CampaignClockState[0x01] !=
                (int16_t) LastOverviewNoticeHour &&
            GeneratePseudoRandomByte() >= TacticalOverviewNoticeChance)
        {
            showClockNotice = TRUE;
            TacticalOverviewClockNoticeActive = TRUE;
            LastOverviewNoticeHour = CampaignClockState[0x01];
        }
    }

    if (TacticalStateInitialized == FALSE)
    {
        ShowTacticalModalMessage(NoSatelliteUplinkText);
        return;
    }

    if (showClockNotice == FALSE)
    {
        TacticalOverviewActive = TRUE;
        TacticalSelectedUnitIndex = TACTICAL_UNIT_NONE;
        TacticalOverviewScreenOriginX =
            (uint16_t) (((int16_t) 0x00A0 - (int16_t) MapWidth) >> 0x01);
        BuildTacticalOverviewMapBuffer(
            TerrainTypeMap,
            TacticalOverviewMapBuffer,
            MapWidth,
            MapHeight);
        HideMouseCursorOverlay();
        DrawTacticalOverviewMap();
        return;
    }

    HideMouseCursorOverlay();
    screenBuffer = PrimaryScreenBuffer;
    CopyVideoRegionToBuffer(0x01, 0x00, 0x03, 0x0B, screenBuffer);
    SelectAndClearTextWindow(0x0D);
    DrawActiveTextWindowBorder(0x0E);
    RenderFormattedText(UplinkJammedText);
    ShowMouseCursorOverlay();
    (void) WaitForDismissInput();
    HideMouseCursorOverlay();
    CopyBufferToVideoRegion(0x01, 0x00, 0x03, 0x0B, screenBuffer);
    RefreshTacticalDisplay(FALSE);
    ShowMouseCursorOverlay();
}

// 24C5:012E
void RefreshTacticalOverview(void)
{
    uint16_t rowStride = (MapWidth + 0x07) & 0xFFF8;
    uint16_t unitIndex;
    int16_t column;
    int16_t row;
    uint8_t opposingMarkerColor = OVERVIEW_OPPOSING_MARKER_COLOR;
    uint8_t *viewportTopRow;

    // Restore the terrain pixels under the markers drawn by the previous pass.
    for (unitIndex = 0x00; unitIndex < TACTICAL_UNIT_COUNT; ++unitIndex)
    {
        uint8_t oldX = TacticalOverviewMarkerX[unitIndex];
        uint8_t oldY = TacticalOverviewMarkerY[unitIndex];

        if (oldX != OVERVIEW_INVALID_COORDINATE ||
            oldY != OVERVIEW_INVALID_COORDINATE)
        {
            uint8_t terrainColor = TACTICAL_OVERVIEW_MAP_BUFFER[(uint16_t) (oldY * rowStride) + oldX];
            PlotPixel(
                TacticalOverviewScreenOriginX + oldX,
                oldY,
                terrainColor);
            TacticalOverviewMarkerX[unitIndex] =
                OVERVIEW_INVALID_COORDINATE;
            TacticalOverviewMarkerY[unitIndex] =
                OVERVIEW_INVALID_COORDINATE;
        }
    }

    // Restore the terrain beneath the previous 19x12 viewport outline.
    viewportTopRow = TACTICAL_OVERVIEW_MAP_BUFFER +
        (uint16_t) (TacticalOverviewViewportY * rowStride) +
        TacticalOverviewViewportX;
    for (column = 0x00; column < OVERVIEW_VIEWPORT_WIDTH_CELLS; ++column)
    {
        PlotPixel(
            TacticalOverviewScreenOriginX +
                TacticalOverviewViewportX + column,
            TacticalOverviewViewportY,
            viewportTopRow[column]);
        PlotPixel(
            TacticalOverviewScreenOriginX +
                TacticalOverviewViewportX + column,
            TacticalOverviewViewportY +
                OVERVIEW_VIEWPORT_HEIGHT_CELLS - 0x01,
            viewportTopRow[(OVERVIEW_VIEWPORT_HEIGHT_CELLS - 0x01) * rowStride + column]);
    }
    for (row = 0x01; row < OVERVIEW_VIEWPORT_HEIGHT_CELLS - 0x01; ++row)
    {
        uint8_t *mapRow = viewportTopRow +
            (uint16_t) ((uint16_t) row * rowStride);
        PlotPixel(
            TacticalOverviewScreenOriginX +
                TacticalOverviewViewportX,
            TacticalOverviewViewportY + row,
            mapRow[0x00]);
        PlotPixel(
            TacticalOverviewScreenOriginX +
                TacticalOverviewViewportX +
                OVERVIEW_VIEWPORT_WIDTH_CELLS - 0x01,
            TacticalOverviewViewportY + row,
            mapRow[OVERVIEW_VIEWPORT_WIDTH_CELLS - 0x01]);
    }

    DrawTacticalOverviewViewportFrame();

    if (ScenarioFileSuffix >= 0x46 && ScenarioFileSuffix != 0x54)
        opposingMarkerColor = OVERVIEW_SPECIAL_OPPOSING_COLOR;

    for (unitIndex = 0x00; unitIndex < TACTICAL_UNIT_COUNT; ++unitIndex)
    {
        RevengeUnitRecord *unitRecord =
            &PersistentUnitRoster[unitIndex];
        uint8_t markerColor = OVERVIEW_FRIENDLY_MARKER_COLOR;
        if (unitIndex >= ENEMY_FORCE_FIRST_UNIT)
            markerColor = opposingMarkerColor;

        if (unitRecord->deploymentState == 0x00)
            continue;

        TacticalOverviewMarkerX[unitIndex] =
            ScenarioUnitMapX[unitIndex];
        TacticalOverviewMarkerY[unitIndex] =
            ScenarioUnitMapY[unitIndex];
        PlotPixel(
            TacticalOverviewScreenOriginX +
                TacticalOverviewMarkerX[unitIndex],
            TacticalOverviewMarkerY[unitIndex],
            markerColor);
    }
}

// 24C5:02F1
void DrawTacticalOverviewViewportFrame(void)
{
    uint16_t left;
    uint16_t top;
    uint16_t right;
    uint16_t bottom;

    // V: constrain the tactical camera origin before copying it into the
    // overview's previous-frame coordinates.
    ScrollTacticalViewportToCursor();
    TacticalOverviewViewportX = TacticalViewportMapX;
    TacticalOverviewViewportY = TacticalViewportMapY;

    left = (TacticalOverviewScreenOriginX +
               TacticalViewportMapX)
        << 0x01;
    top = TacticalViewportMapY << 0x01;
    right = left + ((OVERVIEW_VIEWPORT_WIDTH_CELLS << 0x01) - 0x01);
    bottom = top + ((OVERVIEW_VIEWPORT_HEIGHT_CELLS << 0x01) - 0x01);

    DrawClippedAxisAlignedLine(left, top, right, top,
        OVERVIEW_VIEWPORT_FRAME_COLOR);
    DrawClippedAxisAlignedLine(left, top, left, bottom,
        OVERVIEW_VIEWPORT_FRAME_COLOR);
    DrawClippedAxisAlignedLine(right, top, right, bottom,
        OVERVIEW_VIEWPORT_FRAME_COLOR);
    DrawClippedAxisAlignedLine(left, bottom, right, bottom,
        OVERVIEW_VIEWPORT_FRAME_COLOR);
}

// 24C5:037F
void DrawTacticalOverviewMap(void)
{
    uint16_t rowStride = (MapWidth + 0x07) & 0xFFF8;
    int16_t mapX;
    int16_t mapY;
    uint16_t unitIndex;

    ClearActiveVideoBuffer();
    for (mapY = 0x00; mapY < (int16_t) MapHeight; ++mapY)
    {
        uint8_t *mapRow = TACTICAL_OVERVIEW_MAP_BUFFER +
            (uint16_t) ((uint16_t) mapY * rowStride);
        for (mapX = 0x00; mapX < (int16_t) rowStride; ++mapX)
            PlotPixel(
                TacticalOverviewScreenOriginX + mapX,
                mapY,
                mapRow[mapX]);
    }

    for (unitIndex = 0x00; unitIndex < TACTICAL_UNIT_COUNT; ++unitIndex)
    {
        TacticalOverviewMarkerX[unitIndex] =
            OVERVIEW_INVALID_COORDINATE;
        TacticalOverviewMarkerY[unitIndex] =
            OVERVIEW_INVALID_COORDINATE;
    }
    DrawTacticalOverviewViewportFrame();
}

// 24C5:0401
void ExitTacticalOverview(void)
{
    uint16_t runtimeSlot;

    TacticalOverviewActive = FALSE;
    ClearActiveVideoBuffer();
    for (runtimeSlot = 0x00;
        runtimeSlot < OVERVIEW_RUNTIME_SLOT_COUNT;
        ++runtimeSlot)
    {
        TacticalStatusRowInvalidation[runtimeSlot] = 0xFF;
    }

    // V: restore the ordinary tactical interface after the overview buffer is
    // cleared.
    RedrawTacticalStatusPanel();
    ShowMouseCursorOverlay();
}
