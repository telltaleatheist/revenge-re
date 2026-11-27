// 3A6A: mouse cursor data declarations
#ifndef REVENGE_DATA_MOUSE_CURSOR_DATA_H
#define REVENGE_DATA_MOUSE_CURSOR_DATA_H

#include "revenge/types.h"

extern uint16_t PendingMouseButtonEvent;
extern int16_t MouseCursorTestX;
extern int16_t MouseCursorTestY;
extern uint16_t MouseCursorUpdateLock;
extern int16_t MouseCursorRequestedX;
extern uint16_t MouseCursorHideDepth;
extern uint16_t MouseDriverDoubleX;
extern uint16_t MouseDriverInstalled;
extern uint16_t MouseButtonState;
extern uint16_t MouseY;
extern uint16_t MouseInputDisabled;
extern int16_t MouseCursorMinX;
extern int16_t MouseCursorMaxX;
extern int16_t MouseCursorMinY;
extern int16_t MouseCursorMaxY;
extern int16_t MouseCursorHotspotX;
extern int16_t MouseCursorHotspotY;
extern int16_t MouseCursorDisplayedX;
extern int16_t MouseCursorDisplayedY;
extern uint8_t *MouseCursorBackground;
extern const uint8_t *MouseCursorBitmap;
extern int16_t MouseCursorErasedX;
extern int16_t MouseCursorErasedY;
extern int16_t MouseCursorRestoreTileLeft;
extern int16_t MouseCursorRestoreTileTop;
extern int16_t MouseCursorRestoreTileRight;
extern int16_t MouseCursorRestoreTileBottom;
extern int16_t MouseCursorDrawnX;
extern int16_t MouseCursorDrawnY;
extern int16_t MouseCursorCaptureTileLeft;
extern int16_t MouseCursorCaptureTileTop;
extern int16_t MouseCursorCaptureTileRight;
extern int16_t MouseCursorCaptureTileBottom;
extern int16_t MouseCursorDrawLeft;
extern int16_t MouseCursorDrawTop;

#endif
