// 0D73: entry blink
/* Preserved text-entry cursor blink from REVENGE_0D73. */

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "REVENGE.h"

#include "revenge/original_data.h"

#include <string.h>

/* 0D73:3DA9-3E1D */
void UpdateTextEntryCursorBlink(void)
{
    LocalClockSnapshot clockSnapshot;
    uint16_t savedCursorColumn;
    uint16_t savedCursorRow;
    char cursorText[0x02];

    CaptureLocalClockSnapshot(&clockSnapshot);
    savedCursorColumn = ActiveTextCursorColumn;
    savedCursorRow = ActiveTextCursorRow;
    cursorText[0x00] = TextEntryBuffer[TextEntryCursorIndex];
    cursorText[0x01] = '\0';
    if ((clockSnapshot.milliseconds & 0x0200) != 0x0000)
        strcpy(cursorText, TextCursorBlinkGlyph);
    ActiveTextCursorColumn += TextEntryCursorIndex;
    HideMouseCursorOverlay();
    RenderFormattedText(cursorText);
    ShowMouseCursorOverlay();
    ActiveTextCursorColumn = savedCursorColumn;
    ActiveTextCursorRow = savedCursorRow;
}
