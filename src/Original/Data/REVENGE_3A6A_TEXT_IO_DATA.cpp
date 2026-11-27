// REVENGE_3A6A_TEXT_IO_DATA.cpp
// Typed objects recovered from REVENGE.EXE DS segment 3A6A.

#include "revenge/original_data.h"
#include "revenge/audio.h"
#include "revenge/video.h"
#include "REVENGE.h"

/* DS:05AA and DS:05BA, the story's media, placed at start
 * (original_import_pieces.txt). */
byte CompressedTextFirstByteTable[0x10];
byte CompressedTextSecondByteTable[0x80];
word16 CgaNarrowStrideMode;
word16 UiSelectionResetFlagA;
word16 UiSelectionResetFlagB;
word16 ActiveTextWindowLeftTile;
word16 ActiveTextWindowTopTile;
/* Filled at start from data/ui/windows.json (revenge/rules.h). */
TextWindowContext TextWindowContexts[ORIGINAL_TEXT_WINDOW_COUNT];
word16 ActiveTextWindowIndex;
word16 ActiveTextWindowWidthTiles;
word16 ActiveTextWindowHeightTiles;
word16 ActiveTextForegroundColor;
word16 ActiveTextBackgroundColor;
word16 ActiveTextCursorColumn;
word16 ActiveTextCursorRow;
word16 TextWindowStateInitialized;
/* Filled at start from data/ui/windows.json (revenge/rules.h). */
word16 UiNavigationUpKey;
word16 UiNavigationDownKey;
word16 UiPreviousRegionKey;
word16 UiNextRegionKey;
OriginalUiRegionRecord UiRegionTable[ORIGINAL_UI_REGION_COUNT];
PITCalibrationState PITCalibration;
word16 VerticalRetraceEdgeMode = 0x0001;
