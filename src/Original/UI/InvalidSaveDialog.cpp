// 0D73: invalid save dialog
/* Preserved invalid-save-version dialog from REVENGE_0D73. */

#include "REVENGE.h"

#include "revenge/input.h"
#include "revenge/original_data.h"
#include "revenge/video.h"

/* 0D73:4611-4865 */
void ShowInvalidSaveVersionDialog(void)
{
    uint8_t *backingBuffer;
    uint32_t backingSize;
    uint16_t savedWindowIndex;
    uint16_t allocatedHere;
    uint16_t savedForegroundColor;
    uint16_t savedBackgroundColor;
    uint16_t dismissKey;
    int16_t leftX;
    int16_t topY;
    int16_t rightX;
    int16_t bottomY;

    savedWindowIndex = ActiveTextWindowIndex;
    SelectTextWindow(StartupResourceState);
    allocatedHere = FALSE;
    if (InvalidSaveDialogBacking == NULL)
    {
        backingSize = (uint32_t) ActiveTextWindowWidthTiles *
            ActiveTextWindowHeightTiles;
        backingSize <<= VideoAdapterType == VIDEO_ADAPTER_VGA ? 0x06 : 0x05;
        backingBuffer = (uint8_t *) Heap_Allocate(backingSize, "0D73:466E");
        InvalidSaveDialogBacking = backingBuffer;
        allocatedHere = TRUE;
    }
    backingBuffer = InvalidSaveDialogBacking;

    HideMouseCursorOverlay();
    CopyVideoRegionToBuffer(
        (int16_t) ActiveTextWindowLeftTile,
        ActiveTextWindowTopTile,
        ActiveTextWindowWidthTiles,
        ActiveTextWindowHeightTiles,
        backingBuffer);
    ClearActiveTextWindow();

    leftX = (int16_t) (ActiveTextWindowLeftTile << 0x03);
    topY = (int16_t) (ActiveTextWindowTopTile << 0x03);
    rightX = (int16_t) (((ActiveTextWindowLeftTile +
                             ActiveTextWindowWidthTiles)
                            << 0x03) -
        0x01);
    bottomY = (int16_t) (((ActiveTextWindowTopTile +
                              ActiveTextWindowHeightTiles)
                             << 0x03) -
        0x01);
    DrawClippedAxisAlignedLine(leftX, topY, rightX, topY, 0x000F);
    DrawClippedAxisAlignedLine(leftX, bottomY, rightX, bottomY, 0x000F);
    DrawClippedAxisAlignedLine(leftX, topY, leftX, bottomY, 0x000F);
    DrawClippedAxisAlignedLine(rightX, topY, rightX, bottomY, 0x000F);
    ShowMouseCursorOverlay();

    AdjustActiveTextWindowMargin(0xFFFF);
    savedForegroundColor = ActiveTextForegroundColor;
    savedBackgroundColor = ActiveTextBackgroundColor;
    ActiveTextForegroundColor = 0x000F;
    ActiveTextBackgroundColor = 0x000C;
    HideMouseCursorOverlay();
    RenderFormattedText(InvalidSaveVersionText);
    ShowMouseCursorOverlay();
    ActiveTextForegroundColor = savedForegroundColor;
    ActiveTextBackgroundColor = savedBackgroundColor;
    dismissKey = WaitForDismissInput();

    AdjustActiveTextWindowMargin(0x0001);
    HideMouseCursorOverlay();
    CopyBufferToVideoRegion(
        (int16_t) ActiveTextWindowLeftTile,
        ActiveTextWindowTopTile,
        ActiveTextWindowWidthTiles,
        ActiveTextWindowHeightTiles,
        backingBuffer);
    ShowMouseCursorOverlay();
    SelectTextWindow(savedWindowIndex);

    if (allocatedHere != FALSE)
    {
        Heap_Free(backingBuffer);
        InvalidSaveDialogBacking = NULL;
    }
    if (dismissKey == KEY_ESCAPE)
    {
        ShutdownGame();
        RunExitHandlersAndTerminate(0x0001);
    }
}
