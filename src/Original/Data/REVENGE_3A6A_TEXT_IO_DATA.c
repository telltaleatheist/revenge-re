// REVENGE_3A6A_TEXT_IO_DATA.c
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
int16 RuntimeErrno;
int16 RuntimeDOSError;

const int8_t DOSErrorToErrnoTable[0x59] =
{
	0x00, 0x13, 0x02, 0x02, 0x04, 0x05, 0x06, 0x08,
	0x08, 0x08, 0x14, 0x15, 0x05, 0x13, -0x01, 0x16,
	0x05, 0x11, 0x02, -0x01, -0x01, -0x01, -0x01, -0x01,
	-0x01, -0x01, -0x01, -0x01, -0x01, -0x01, -0x01, -0x01,
	0x05, 0x05, -0x01, -0x01, -0x01, -0x01, -0x01, -0x01,
	-0x01, -0x01, -0x01, -0x01, -0x01, -0x01, -0x01, -0x01,
	-0x01, -0x01, 0x0F, -0x01, 0x23, 0x02, -0x01, 0x0F,
	-0x01, -0x01, -0x01, -0x01, 0x13, -0x01, -0x01, 0x02,
	0x02, 0x05, 0x0F, 0x02, -0x01, -0x01, -0x01, 0x13,
	-0x01, -0x01, -0x01, -0x01, -0x01, -0x01, -0x01, -0x01,
	0x23, -0x01, -0x01, -0x01, -0x01, 0x23, -0x01, 0x13,
	-0x01
};

/* DS:712A: the runtime's handle states start with the five handles DOS opens
 * for a program (input, output, error, auxiliary, printer) and the rest
 * unused. */
OriginalFileRuntime FileRuntime =
{
	{
		CRT_FILE_STATE_DEVICE | CRT_FILE_STATE_READ,
		CRT_FILE_STATE_DEVICE | CRT_FILE_STATE_WRITE,
		CRT_FILE_STATE_DEVICE | CRT_FILE_STATE_WRITE,
		CRT_FILE_STATE_BINARY | CRT_FILE_STATE_DEVICE | CRT_FILE_STATE_READ_WRITE,
		CRT_FILE_STATE_BINARY | CRT_FILE_STATE_DEVICE | CRT_FILE_STATE_WRITE,
		CRT_FILE_STATE_UNUSED, CRT_FILE_STATE_UNUSED, CRT_FILE_STATE_UNUSED,
		CRT_FILE_STATE_UNUSED, CRT_FILE_STATE_UNUSED, CRT_FILE_STATE_UNUSED,
		CRT_FILE_STATE_UNUSED, CRT_FILE_STATE_UNUSED, CRT_FILE_STATE_UNUSED,
		CRT_FILE_STATE_UNUSED, CRT_FILE_STATE_UNUSED, CRT_FILE_STATE_UNUSED,
		CRT_FILE_STATE_UNUSED, CRT_FILE_STATE_UNUSED, CRT_FILE_STATE_UNUSED
	},
	CRT_FILE_STATE_TEXT,
	0xFFFF
};
PITCalibrationState PITCalibration;
word16 VerticalRetraceEdgeMode = 0x0001;
