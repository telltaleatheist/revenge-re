// 3A6A: text IO data
// TextIOData.cpp
// Typed objects recovered from REVENGE.EXE DS segment 3A6A.

#include "Data/TextIOData.h"
#include "revenge/audio.h"
#include "revenge/video.h"

/* DS:05AA and DS:05BA, the story's media, placed at start
 * (original_import_pieces.txt). */
uint8_t CompressedTextFirstByteTable[0x10];
uint8_t CompressedTextSecondByteTable[0x80];
uint16_t CgaNarrowStrideMode;
uint16_t UiSelectionResetFlagA;
uint16_t UiSelectionResetFlagB;
uint16_t ActiveTextWindowLeftTile;
uint16_t ActiveTextWindowTopTile;
/* Filled at start from data/ui/windows.json (revenge/rules.h). */
TextWindowContext TextWindowContexts[ORIGINAL_TEXT_WINDOW_COUNT];
uint16_t ActiveTextWindowIndex;
uint16_t ActiveTextWindowWidthTiles;
uint16_t ActiveTextWindowHeightTiles;
uint16_t ActiveTextForegroundColor;
uint16_t ActiveTextBackgroundColor;
uint16_t ActiveTextCursorColumn;
uint16_t ActiveTextCursorRow;
uint16_t TextWindowStateInitialized;
/* Filled at start from data/ui/windows.json (revenge/rules.h). */
uint16_t UiNavigationUpKey;
uint16_t UiNavigationDownKey;
uint16_t UiPreviousRegionKey;
uint16_t UiNextRegionKey;
OriginalUiRegionRecord UiRegionTable[ORIGINAL_UI_REGION_COUNT];
PITCalibrationState PITCalibration;
uint16_t VerticalRetraceEdgeMode = 0x0001;
