// 3390: resource error message
// ResourceErrorMessage.cpp
// The resource layer's error messages (3390:071E-098B), the two callbacks
// AbortRuntimeWithMessage and OpenTrackedFile's errors call.

#include "Data/ResourceData.h"
#include "Data/RuntimeData.h"
#include "Data/StartupData.h"
#include "Data/TextIOData.h"
#include "Graphics/Graphics.h"
#include "Resources/Resources.h"
#include "Runtime/Runtime.h"
#include "Text/Text.h"
#include "UI/UI.h"
#include "revenge/heap.h"
#include "revenge/input.h"
#include "revenge/text.h"
#include "revenge/video.h"

// 3390:071E: the resource layer's error (the text at ResourceFiles.errorText)
// in a box over the start-up text window, until a key. DS:5FBE, the display
// callback of AbortRuntimeWithMessage and OpenTrackedFile's errors. The box
// is drawn as the invalid save's (0D73:4611), without the mouse cursor's
// hide and show.
void ShowResourceErrorMessage(void)
{
    uint8_t *backingBuffer;
    uint32_t backingSize;
    uint16_t savedWindowIndex;
    uint16_t allocatedHere;
    uint16_t savedForegroundColor;
    uint16_t savedBackgroundColor;
    int16_t leftX;
    int16_t topY;
    int16_t rightX;
    int16_t bottomY;

    savedWindowIndex = ActiveTextWindowIndex;
    SelectTextWindow(StartupResourceState);
    allocatedHere = FALSE;
    if (InvalidSaveDialogBacking == NULL)
    {
        /* 0746-074D: the 16-bit product, sign-extended (cwd). */
        backingSize = (uint32_t) (int32_t) (int16_t) (uint16_t) (ActiveTextWindowWidthTiles * ActiveTextWindowHeightTiles);
        backingSize <<= VideoAdapterType == VIDEO_ADAPTER_VGA ? VGA_TILE_BYTE_SHIFT : EGA_TILE_BYTE_SHIFT;
        InvalidSaveDialogBacking = (uint8_t *) Heap_Allocate(backingSize, "3390:077F");
        allocatedHere = TRUE;
    }
    backingBuffer = InvalidSaveDialogBacking;

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
    AdjustActiveTextWindowMargin(0xFFFF);

    /* 08B9-08EE wrote the drive's letters into the insert-disk text
	 * (DS:6012, at +0Ch and +17h) when that was the error. It never is:
	 * the disk prompt went with OpenTrackedFile's (3390:007C). */
    savedForegroundColor = ActiveTextForegroundColor;
    savedBackgroundColor = ActiveTextBackgroundColor;
    ActiveTextForegroundColor = 0x000F;
    ActiveTextBackgroundColor = 0x000C;
    RenderFormattedText(ResourceFiles.errorText);
    ActiveTextForegroundColor = savedForegroundColor;
    ActiveTextBackgroundColor = savedBackgroundColor;
    (void) ReadTranslatedKeypress();

    AdjustActiveTextWindowMargin(0x0001);
    CopyBufferToVideoRegion(
        (int16_t) ActiveTextWindowLeftTile,
        ActiveTextWindowTopTile,
        ActiveTextWindowWidthTiles,
        ActiveTextWindowHeightTiles,
        backingBuffer);
    SelectTextWindow(savedWindowIndex);

    if (allocatedHere != FALSE)
    {
        Heap_Free(InvalidSaveDialogBacking);
        InvalidSaveDialogBacking = NULL;
    }
}

// 3390:097B: the fatal disk error (the text at DS:5FBA) as the resource
// error, shown by ShowResourceErrorMessage. DS:5FC2, the dismiss callback:
// after an error's own text, the program says it returns to DOS.
void ShowFatalDiskErrorMessage(void)
{
    ResourceFiles.errorText = ResourceFatalDiskErrorText;
    ShowResourceErrorMessage();
}

/* DS:5FBE, 5FC2: 3390:071E and 3390:097B in the image, never written. */
ResourceErrorCallback ResourceErrorDisplayCallback = ShowResourceErrorMessage;
ResourceErrorCallback ResourceErrorDismissCallback = ShowFatalDiskErrorMessage;
