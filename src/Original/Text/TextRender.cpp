// 356F: text render
// Reconstructed original active text-window string renderer (segment 356F).

#include "Data/RuntimeData.h"
#include "Data/GraphicsData.h"
#include "Data/TextIOData.h"
#include "Graphics/Graphics.h"
#include "Text/Text.h"
#include "revenge/debug.h"
#include "revenge/video.h"

#include <string.h>

// 356F:000B
// Draw a mutable string in the active text window. When its cursor is below the
// last usable line, scroll the window by one font line before drawing. The
// source buffer is consumed: its first byte is cleared before returning.
void RenderTextToActiveWindow(char *text, uint16_t advanceToNextLine)
{
    int16_t glyphWidth = ActiveFontGlyphAdvanceX;
    int16_t lineHeight = ActiveFontLineHeight;
    uint16_t leftX = ActiveTextWindowLeftTile << 0x03;
    uint16_t topY = ActiveTextWindowTopTile << 0x03;
    uint16_t widthPixels = ActiveTextWindowWidthTiles << 0x03;
    uint16_t heightPixels = ActiveTextWindowHeightTiles << 0x03;
    uint16_t lastLineY = (uint16_t) (lineHeight *
        ((int16_t) heightPixels / lineHeight - 0x01));
    uint16_t drawX;
    uint16_t drawY;
    uint16_t windowScrolled =
        (int16_t) (uint16_t) (heightPixels - lineHeight) <
        (int16_t) (uint16_t) (ActiveTextCursorRow * lineHeight);

    REVENGE_PRESERVATION_TRACE(
        "text: draw '%s' window=%u xy=%u,%u size=%u,%u cursor=%u,%u colors=%u,%u font=%p target=%p\n",
        text, ActiveTextWindowIndex,
        ActiveTextWindowLeftTile, ActiveTextWindowTopTile,
        ActiveTextWindowWidthTiles, ActiveTextWindowHeightTiles,
        ActiveTextCursorColumn, ActiveTextCursorRow,
        ActiveTextForegroundColor, ActiveTextBackgroundColor,
        (void *) ActiveFontResource, (void *) ActiveRenderTarget);

    // Verified against 356F:000B-01F2; the final clear/epilogue is confirmed
    // by the Reko low-level output where the segment listing ends.

    if (windowScrolled)
    {
        uint8_t *renderTarget = ActiveRenderTarget;
        CopyClippedVideoRegion(
            ActiveTextWindowLeftTile,
            topY + lineHeight,
            ActiveTextWindowLeftTile,
            topY,
            ActiveTextWindowWidthTiles,
            lastLineY,
            renderTarget,
            renderTarget);
        FillVideoRectangle(
            leftX,
            topY + lastLineY,
            leftX + widthPixels - 0x01,
            topY + lastLineY + lineHeight - 0x01,
            (uint8_t) ActiveTextBackgroundColor);
        drawX = leftX;
        drawY = topY + lastLineY;
    }
    else
    {
        drawX = leftX + ActiveTextCursorColumn * glyphWidth;
        drawY = topY + ActiveTextCursorRow * lineHeight;
    }

    DrawTextString(
        text,
        drawX,
        drawY,
        (uint8_t) ActiveTextForegroundColor,
        (uint8_t) ActiveTextBackgroundColor);

    if (advanceToNextLine != FALSE)
    {
        ActiveTextCursorColumn = 0x00;
        ++ActiveTextCursorRow;
    }
    else
    {
        if (windowScrolled)
            ActiveTextCursorRow =
                (uint16_t) ((int16_t) lastLineY / lineHeight);
        ActiveTextCursorColumn += (uint16_t) strlen(text);
        if ((int16_t) (uint16_t) (widthPixels - glyphWidth) <
            (int16_t) (uint16_t) (ActiveTextCursorColumn * glyphWidth))
        {
            ActiveTextCursorColumn = 0x00;
            ++ActiveTextCursorRow;
        }
    }

    text[0x00] = '\0';
}
