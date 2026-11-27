// 0800, 0D73, 3037, 34C8, 34E6, 3568, 358E, 3599, 35AB: Input declarations
#ifndef REVENGE_INPUT_INPUT_H
#define REVENGE_INPUT_INPUT_H

#include "revenge/types.h"

uint16_t KeyboardInputPending(void);
uint16_t ReadRawKeypress(void);
int8_t TranslateKeyCode(void);
int16_t NormalizeNavigationKeyCode(int16_t keyCode);
void ConfigureMouseCursorOverlay(
    int16_t hotspotX,
    int16_t hotspotY,
    const uint8_t *cursorBitmap);
uint16_t IsMouseCursorInsideRectangle(
    int16_t left,
    int16_t top,
    int16_t right,
    int16_t bottom);
void HideMouseCursorOverlay(void);
void SetMouseCursorPositionUnlocked(int16_t x, int16_t y);
void SetMouseCursorPosition(int16_t x, int16_t y);
void MoveMouseCursorRelative(int16_t deltaX, int16_t deltaY);
void ShowMouseCursorOverlay(void);
void HideMouseCursorOverlayUnlocked(void);
void ShowMouseCursorOverlayUnlocked(void);
uint16_t FlushInputAndWait(uint16_t timeoutRetraces);
int16_t RunSingleUiRegionSelection(int16_t regionIndex);
void HighlightUiRegionSelection(int16_t regionIndex);
void BeginUiRegionSelection(
    int16_t firstRegionIndex, uint16_t regionCount);
int16_t RunUiRegionSelectionLoop(
    int16_t firstRegionIndex, uint16_t regionCount);
int16_t PollUiRegionSelection(
    int16_t firstRegionIndex, uint16_t regionCount);

#endif
