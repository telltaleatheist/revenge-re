// 3A6A: text IO data declarations
#ifndef REVENGE_DATA_TEXT_IO_DATA_H
#define REVENGE_DATA_TEXT_IO_DATA_H

#include "revenge/original/input_data.h"
#include "revenge/text.h"
#include "revenge/types.h"

#include <stddef.h>

enum
{
    ORIGINAL_TEXT_WINDOW_COUNT = 0x17
};

#pragma pack(push, 1)
typedef struct PITCalibrationState
{
    uint16_t Channel0Divisor;
    DosAddress SavedIRQ0Vector;
    uint16_t SavedInterruptFlags;
    uint16_t LatchedChannel0Count;
} PITCalibrationState;
#pragma pack(pop)

static_assert(sizeof(PITCalibrationState) == 0x0A,
    "PITCalibrationState must retain DS:6FA4-6FAD layout");
static_assert(offsetof(PITCalibrationState, SavedIRQ0Vector) == 0x02,
    "saved IRQ0 vector must retain its original offset");

extern uint8_t CompressedTextFirstByteTable[0x10];
extern uint8_t CompressedTextSecondByteTable[0x80];
extern uint16_t CgaNarrowStrideMode;
extern uint16_t UiSelectionResetFlagA;
extern uint16_t UiSelectionResetFlagB;
extern uint16_t ActiveTextWindowLeftTile;
extern uint16_t ActiveTextWindowTopTile;
extern TextWindowContext TextWindowContexts[ORIGINAL_TEXT_WINDOW_COUNT];
extern uint16_t ActiveTextWindowIndex;
extern uint16_t ActiveTextWindowWidthTiles;
extern uint16_t ActiveTextWindowHeightTiles;
extern uint16_t ActiveTextForegroundColor;
extern uint16_t ActiveTextBackgroundColor;
extern uint16_t ActiveTextCursorColumn;
extern uint16_t ActiveTextCursorRow;
extern uint16_t TextWindowStateInitialized;
extern uint16_t UiNavigationUpKey;
extern uint16_t UiNavigationDownKey;
extern uint16_t UiPreviousRegionKey;
extern uint16_t UiNextRegionKey;
extern PITCalibrationState PITCalibration;
extern uint16_t VerticalRetraceEdgeMode;

#define TacticalMenuFirstRegion \
    (TextWindowContexts[0x04].topTile)
#define TacticalMenuLastRegion \
    (TextWindowContexts[0x04].heightTiles)
#define TacticalMenuOptionCount \
    (UiRegionTable[0x01].rowCount)
#define TacticalMenuSelectedOption \
    (UiRegionTable[0x01].selectedRow)
/* These addresses are fields inside the original DS tables, not independent
 * globals.  RunGameOptionsMenu changes both records for its six-row
 * standalone and seven-row embedded layouts. */
#define GameOptionsMenuHeight \
    (TextWindowContexts[0x0B].heightTiles)
/* These addresses are fields inside the original DS tables, not independent
 * globals.  RunGameOptionsMenu changes both records for its six-row
 * standalone and seven-row embedded layouts. */
#define GameOptionsMenuEntryCount \
    (UiRegionTable[0x05].rowCount)

#endif
