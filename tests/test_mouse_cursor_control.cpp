#include "Data/TacticalData.h"
#include "Graphics/Graphics.h"
#include "Input/Input.h"
#include "fake_backend.h"

#include <assert.h>

static uint16_t spriteDrawCalls;

void DrawClippedSprite(
    uint8_t *destination,
    const uint8_t *encodedSprite,
    int16_t x,
    int16_t y,
    uint16_t preserveExistingPixels)
{
    (void) encodedSprite;
    (void) preserveExistingPixels;
    ++spriteDrawCalls;
    if (x >= 0x0000 && y >= 0x0000)
        destination[(uint32_t) y * 0x0140 + x] = 0x0F;
}

int main(void)
{
    static const uint8_t cursorBitmap[0x20] = { 0x00 };
    static uint8_t cursorBackground[0x0240];

    FakeBackend_Reset();
    MouseCursorUpdateLock = 0x0000;
    MouseInputDisabled = 0x0000;
    MouseCursorMinX = 0x0000;
    MouseCursorMaxX = 0x013F;
    MouseCursorMinY = 0x0000;
    MouseCursorMaxY = 0x00C7;
    MouseDriverInstalled = FALSE;
    MouseCursorDisplayedX = 0x000A;
    MouseCursorDisplayedY = 0x0014;
    MouseCursorRequestedX = 0x000A;
    MouseY = 0x0014;
    MouseCursorHideDepth = 0x0000;
    MouseCursorBackground = cursorBackground;
    MouseCursorBitmap = cursorBitmap;
    MouseInputDisabled = TRUE;
    MouseEventCallback(0x0000, 0x0030, 0x0040);
    assert(MouseCursorRequestedX == 0x000A);
    assert(MouseY == 0x0014);
    MouseInputDisabled = FALSE;
    MouseDriverInstalled = TRUE;

    assert(IsMouseCursorInsideRectangle(
               0x000A, 0x0014, 0x000A, 0x0014) == 0x0001);
    assert(IsMouseCursorInsideRectangle(
               0x000B, 0x0014, 0x0020, 0x0020) == 0x0000);

    SetMouseCursorPosition(-0x0005, 0x00D0);
    assert(MouseCursorRequestedX == 0x0000);
    assert(MouseY == 0x00C7);
    assert(fakeBackend.mouseX == 0x0000);
    assert(fakeBackend.mouseY == 0x00C7);
    assert(spriteDrawCalls == 0x0001);
    assert(fakeBackend.indexedFramebuffer[0x00C7 * 0x0140] == 0x0F);

    MoveMouseCursorRelative(0x0005, -0x0007);
    assert(MouseCursorRequestedX == 0x0005);
    assert(MouseY == 0x00C0);
    assert(fakeBackend.indexedFramebuffer[0x00C7 * 0x0140] == 0x00);
    assert(fakeBackend.indexedFramebuffer[0x00C0 * 0x0140 + 0x0005] == 0x0F);

    ConfigureMouseCursorOverlay(0x0002, 0x0003, cursorBitmap);
    assert(MouseCursorHotspotX == 0x0002);
    assert(MouseCursorHotspotY == 0x0003);
    assert(MouseCursorBitmap == cursorBitmap);

    MouseButtonState = 0x0000;
    PendingMouseButtonEvent = 0x0000;
    MouseEventCallback(MOUSE_BUTTON_LEFT, 0x001E, 0x0028);
    assert(MouseCursorRequestedX == 0x001E);
    assert(MouseY == 0x0028);
    assert(MouseButtonState == MOUSE_BUTTON_LEFT);
    assert(PendingMouseButtonEvent == MOUSE_BUTTON_LEFT);

    /* The same buttons: nothing new pending. */
    PendingMouseButtonEvent = 0x0000;
    MouseEventCallback(MOUSE_BUTTON_LEFT, 0x001E, 0x0028);
    assert(PendingMouseButtonEvent == 0x0000);

    PendingMouseButtonEvent = 0x0000;
    MouseEventCallback(0x0000, 0x001E, 0x0028);
    assert(MouseButtonState == 0x0000);
    assert(PendingMouseButtonEvent == MOUSE_BUTTON_LEFT);

    /* The middle button keeps bit 2 (DS:7FA2 is never set). */
    PendingMouseButtonEvent = 0x0000;
    MouseEventCallback(MOUSE_BUTTON_MIDDLE, 0x001E, 0x0028);
    assert(MouseButtonState == MOUSE_BUTTON_MIDDLE);
    assert(PendingMouseButtonEvent == MOUSE_BUTTON_MIDDLE);
    MouseEventCallback(0x0000, 0x001E, 0x0028);

    /* While the cursor is being updated, the buttons still count but the
	 * position waits. */
    MouseCursorUpdateLock = 0x0001;
    MouseEventCallback(MOUSE_BUTTON_RIGHT, 0x0040, 0x0050);
    assert(MouseButtonState == MOUSE_BUTTON_RIGHT);
    assert(MouseCursorRequestedX == 0x001E);
    MouseCursorUpdateLock = 0x0000;
    MouseEventCallback(0x0000, 0x001E, 0x0028);

    /* A driver that doubles X (DS:7FAC). */
    MouseDriverDoubleX = 0x0001;
    MouseEventCallback(0x0000, 0x0050, 0x0028);
    assert(MouseCursorRequestedX == 0x0028);
    MouseDriverDoubleX = 0x0000;
    MouseEventCallback(0x0000, 0x001E, 0x0028);

    HideMouseCursorOverlay();
    ShowMouseCursorOverlay();
    assert(MouseCursorUpdateLock == 0x0000);
    assert(MouseCursorHideDepth == 0x0000);
    assert(spriteDrawCalls == 0x0007);
    return 0x00;
}
