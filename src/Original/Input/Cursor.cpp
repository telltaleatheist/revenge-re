// 0800: cursor
/* Software-cursor procedures from original segment 0800. */

#include "REVENGE.h"
#include "revenge/original_data.h"
#include "revenge/platform_backend.h"
#include "revenge/video.h"

/* 0800:3772-37FD: the handler the game gives the mouse driver (INT 33h
 * AX=0Ch, every event: 1Fh), called with the buttons in BX and the position
 * in CX and DX. Host: SDL's mouse events arrive here as the driver called it
 * (src/SDL/platform.cpp), from InitializeMouseSupport until
 * ShutdownMouseSupport. */
void MouseEventCallback(uint16_t buttons, uint16_t x, uint16_t y)
{
    if (MouseInputDisabled != FALSE)
        return;
    /* 0800:3781-3791 reports the middle button as the word at DS:7FA2
	 * when that is set; nothing sets it, so the middle button keeps
	 * bit 2. */
    if (MouseButtonState != buttons)
        PendingMouseButtonEvent |= MouseButtonState ^ buttons;
    MouseButtonState = buttons;
    /* 0800:37A9-37B3 sets the word at DS:7FA6 while a button is down;
	 * nothing reads it. */

    if (MouseCursorUpdateLock != 0x0000)
        return;
    MouseCursorUpdateLock = 0x0001;
    if (MouseDriverDoubleX == 0x0001)
        x >>= 0x01;
    MouseCursorRequestedX = (int16_t) x;
    MouseY = y;
    if (MouseCursorDisplayedX != MouseCursorRequestedX ||
        MouseCursorDisplayedY != (int16_t) MouseY)
    {
        HideMouseCursorOverlayUnlocked();
        ShowMouseCursorOverlayUnlocked();
    }
    MouseCursorUpdateLock = 0x0000;
}

/* 0800:3665-36A5 */
void ConfigureMouseCursorOverlay(
    int16_t hotspotX,
    int16_t hotspotY,
    const uint8_t *cursorBitmap)
{
    while (MouseCursorUpdateLock != 0x0000);
    ++MouseCursorUpdateLock;
    HideMouseCursorOverlayUnlocked();
    MouseCursorHotspotX = hotspotX;
    MouseCursorHotspotY = hotspotY;
    MouseCursorBitmap = cursorBitmap;
    ShowMouseCursorOverlayUnlocked();
    --MouseCursorUpdateLock;
}

/* 0800:3825-387C */
uint16_t IsMouseCursorInsideRectangle(
    int16_t left,
    int16_t top,
    int16_t right,
    int16_t bottom)
{
    uint16_t isInside;

    while (MouseCursorUpdateLock != 0x0000);
    ++MouseCursorUpdateLock;
    MouseCursorTestX = MouseCursorDisplayedX;
    MouseCursorTestY = MouseCursorDisplayedY;
    isInside = MouseCursorTestX >= left &&
        MouseCursorTestX <= right &&
        MouseCursorTestY >= top &&
        MouseCursorTestY <= bottom;
    --MouseCursorUpdateLock;
    return isInside;
}

/* 0800:387D-3967 */
void HideMouseCursorOverlayUnlocked(void)
{
    if (MouseInputDisabled != 0x0000)
        return;
    if (MouseCursorHideDepth == 0x0000)
    {
        int16_t cursorLeft;
        int16_t cursorTop;
        int16_t tileLeft;
        int16_t tileTop;

        MouseCursorErasedX = MouseCursorDisplayedX;
        MouseCursorErasedY = MouseCursorDisplayedY;
        cursorLeft = MouseCursorDisplayedX - MouseCursorHotspotX;
        cursorTop = MouseCursorDisplayedY - MouseCursorHotspotY;
        tileLeft = cursorLeft >> 0x03;
        if (tileLeft > -0x03 && tileLeft < VIDEO_SCREEN_8PX_COLUMN_COUNT)
        {
            MouseCursorRestoreTileLeft = tileLeft;
            MouseCursorRestoreTileRight = tileLeft + 0x03;
            if (MouseCursorRestoreTileLeft < 0x0000)
                MouseCursorRestoreTileLeft = 0x0000;
            if (MouseCursorRestoreTileRight > VIDEO_SCREEN_8PX_COLUMN_COUNT)
                MouseCursorRestoreTileRight = VIDEO_SCREEN_8PX_COLUMN_COUNT;
            tileTop = cursorTop >> 0x03;
            if (tileTop > -0x03 && tileTop < VIDEO_SCREEN_HEIGHT / 0x0008)
            {
                MouseCursorRestoreTileTop = tileTop;
                MouseCursorRestoreTileBottom = tileTop + 0x03;
                if (MouseCursorRestoreTileTop < 0x0000)
                    MouseCursorRestoreTileTop = 0x0000;
                if (MouseCursorRestoreTileBottom > VIDEO_SCREEN_HEIGHT / 0x0008)
                    MouseCursorRestoreTileBottom = VIDEO_SCREEN_HEIGHT / 0x0008;
                CopyBufferToVideoRegion(
                    MouseCursorRestoreTileLeft,
                    (uint16_t) MouseCursorRestoreTileTop,
                    (uint16_t) (MouseCursorRestoreTileRight -
                        MouseCursorRestoreTileLeft),
                    (uint16_t) (MouseCursorRestoreTileBottom -
                        MouseCursorRestoreTileTop),
                    MouseCursorBackground);
            }
        }
    }
    ++MouseCursorHideDepth;
    if (MouseCursorHideDepth == 0x0000)
        --MouseCursorHideDepth;
}

/* 0800:3968-398B */
void HideMouseCursorOverlay(void)
{
    while (MouseCursorUpdateLock != 0x0000);
    ++MouseCursorUpdateLock;
    HideMouseCursorOverlayUnlocked();
    --MouseCursorUpdateLock;
}

/* 0800:398C-39FA */
void SetMouseCursorPositionUnlocked(int16_t requestedX, int16_t requestedY)
{
    if (MouseInputDisabled != 0x0000)
        return;

    if (requestedX < MouseCursorMinX)
        requestedX = MouseCursorMinX;
    else if (requestedX > MouseCursorMaxX)
        requestedX = MouseCursorMaxX;
    MouseCursorRequestedX = requestedX;

    if (requestedY < MouseCursorMinY)
        requestedY = MouseCursorMinY;
    else if (requestedY > MouseCursorMaxY)
        requestedY = MouseCursorMaxY;
    MouseY = (uint16_t) requestedY;

    if (MouseDriverInstalled != 0x0000)
        SDLBackend_SetMousePosition(requestedX, requestedY);

    if (requestedX != MouseCursorDisplayedX ||
        requestedY != MouseCursorDisplayedY)
    {
        HideMouseCursorOverlayUnlocked();
        ShowMouseCursorOverlayUnlocked();
    }
}

/* 0800:39FB-3A22 */
void SetMouseCursorPosition(int16_t x, int16_t y)
{
    while (MouseCursorUpdateLock != 0x0000);
    ++MouseCursorUpdateLock;
    SetMouseCursorPositionUnlocked(x, y);
    --MouseCursorUpdateLock;
}

/* 0800:3A23-3A52 */
void MoveMouseCursorRelative(int16_t deltaX, int16_t deltaY)
{
    while (MouseCursorUpdateLock != 0x0000);
    ++MouseCursorUpdateLock;
    SetMouseCursorPositionUnlocked(
        (int16_t) (MouseCursorRequestedX + deltaX),
        (int16_t) ((int16_t) MouseY + deltaY));
    --MouseCursorUpdateLock;
}

/* 0800:3A53-3B79 */
void ShowMouseCursorOverlayUnlocked(void)
{
    int16_t drawLeft;
    int16_t drawTop;
    int16_t tileLeft;
    int16_t tileTop;

    if (MouseInputDisabled != 0x0000 ||
        MouseCursorHideDepth == 0x0000)
        return;
    --MouseCursorHideDepth;
    if (MouseCursorHideDepth != 0x0000)
        return;
    MouseCursorDisplayedX = MouseCursorRequestedX;
    MouseCursorDrawnX = MouseCursorRequestedX;
    MouseCursorDisplayedY = (int16_t) MouseY;
    MouseCursorDrawnY = (int16_t) MouseY;
    drawLeft = MouseCursorDisplayedX - MouseCursorHotspotX;
    drawTop = MouseCursorDisplayedY - MouseCursorHotspotY;
    MouseCursorDrawLeft = drawLeft;
    MouseCursorDrawTop = drawTop;
    tileLeft = drawLeft >> 0x03;
    if (tileLeft <= -0x03 || tileLeft >= VIDEO_SCREEN_8PX_COLUMN_COUNT)
        return;
    MouseCursorCaptureTileLeft = tileLeft;
    MouseCursorCaptureTileRight = tileLeft + 0x03;
    if (MouseCursorCaptureTileLeft < 0x0000)
        MouseCursorCaptureTileLeft = 0x0000;
    if (MouseCursorCaptureTileRight > VIDEO_SCREEN_8PX_COLUMN_COUNT)
        MouseCursorCaptureTileRight = VIDEO_SCREEN_8PX_COLUMN_COUNT;
    tileTop = drawTop >> 0x03;
    if (tileTop <= -0x03 || tileTop >= VIDEO_SCREEN_HEIGHT / 0x0008)
        return;
    MouseCursorCaptureTileTop = tileTop;
    MouseCursorCaptureTileBottom = tileTop + 0x03;
    if (MouseCursorCaptureTileTop < 0x0000)
        MouseCursorCaptureTileTop = 0x0000;
    if (MouseCursorCaptureTileBottom > VIDEO_SCREEN_HEIGHT / 0x0008)
        MouseCursorCaptureTileBottom = VIDEO_SCREEN_HEIGHT / 0x0008;
    CopyVideoRegionToBuffer(
        MouseCursorCaptureTileLeft,
        (uint16_t) MouseCursorCaptureTileTop,
        (uint16_t) (MouseCursorCaptureTileRight -
            MouseCursorCaptureTileLeft),
        (uint16_t) (MouseCursorCaptureTileBottom -
            MouseCursorCaptureTileTop),
        MouseCursorBackground);
    DrawClippedSprite(
        Video_GetIndexedFramebuffer(), MouseCursorBitmap,
        drawLeft, drawTop, 0x0000);
}

/* 0800:3B7A-3B9D */
void ShowMouseCursorOverlay(void)
{
    while (MouseCursorUpdateLock != 0x0000);
    ++MouseCursorUpdateLock;
    ShowMouseCursorOverlayUnlocked();
    --MouseCursorUpdateLock;
}
