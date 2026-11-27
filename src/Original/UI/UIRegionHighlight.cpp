// 35CA: UI region highlight
// Reconstructed original active UI-region highlight toggle (segment 35CA).

#include "REVENGE.h"
#include "revenge/original_data.h"
#include "revenge/video.h"

void HideMouseCursorOverlay(void);
void ShowMouseCursorOverlay(void);

// 35CA:0009: void ToggleActiveUiRegionHighlight(void)
// Called from:
//      ShowConfirmationDialog
//      RunMainMenuSelection
//      RunInGameMenu
//      RunGameOptionsMenu
void ToggleActiveUiRegionHighlight(void)
{
    uint16_t cursorHideDepth;

    cursorHideDepth = MouseCursorHideDepth;
    if (cursorHideDepth == 0x00)
        HideMouseCursorOverlay();
    XorVideoRectangle(
        ActiveUiHighlightLeft,
        ActiveUiHighlightTop,
        (uint16_t) (ActiveUiHighlightLeft +
            ActiveUiHighlightWidth - 0x01),
        (uint16_t) (ActiveUiHighlightTop +
            ActiveUiHighlightHeight - 0x01),
        (uint8_t) ActiveUiHighlightXorColor);
    if (cursorHideDepth == 0x00)
        ShowMouseCursorOverlay();
}
