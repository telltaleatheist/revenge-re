// 3349: text window reset
// Reconstructed original select-and-clear text-window wrapper (segment 3349).

#include "Text/Text.h"
#include "revenge/text.h"

// 3349:0009: void SelectAndClearTextWindow(word16 windowIndex)
// Called from:
//      RunGameOptionsMenu
void SelectAndClearTextWindow(uint16_t windowIndex)
{
    SelectTextWindow(windowIndex);
    ClearActiveTextWindow();
}
