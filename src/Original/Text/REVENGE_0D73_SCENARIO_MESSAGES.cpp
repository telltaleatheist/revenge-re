/* Indexed scenario-message procedures from original segment 0D73. */

#include "REVENGE.h"

#include "revenge/input.h"
#include "revenge/debug.h"
#include "revenge/original_data.h"
#include "revenge/original/campaign_data.h"

void OpenModalTextWindow(word16 windowIndex, word16 clearAndFrame);
void RestoreModalTextWindow(void);

/* 0D73:2E7C-2F15 */
void ShowScenarioMessage(word16 messageIndex)
{
    const CampaignMessageEntry *message;

    if ((PresentationCallbackOffset |
            PresentationCallbackSegment) != 0x0000)
    {
        SceneTransitionAbortFlag = FALSE;
    }
    if (SceneTransitionAbortFlag != FALSE || messageIndex == 0x0000)
        return;

    message = &CampaignMessageTable[messageIndex];
    REVENGE_PRESERVATION_TRACE(
        "text: scenario message=%u height=%d text=%p\n",
        messageIndex, message->windowHeight,
        (void *) CampaignMessageTextTable[messageIndex]);
    TextWindowContexts[0x0F].heightTiles =
        (word16) (int16) message->windowHeight;
    TextWindowContexts[0x0F].topTile = 0x0001;
    OpenModalTextWindow(0x000F, TRUE);
    HideMouseCursorOverlay();
    RenderFormattedText(CampaignMessageTextTable[messageIndex]);
    ShowMouseCursorOverlay();
    FlushPendingInput();
    if ((PresentationCallbackOffset |
            PresentationCallbackSegment) != 0x0000)
    {
        InvokePresentationCallback();
        SceneTransitionAbortFlag = FALSE;
    }
    else
    {
        WaitForDismissInput();
    }
    RestoreModalTextWindow();
}

/* 0D73:3AE4-3B7D */
void ShowMissionFollowupMessage(word16 messageIndex)
{
    const CampaignMessageEntry *message;

    if ((PresentationCallbackOffset |
            PresentationCallbackSegment) != 0x0000)
    {
        SceneTransitionAbortFlag = FALSE;
    }
    if (SceneTransitionAbortFlag != FALSE || messageIndex == 0x0000)
        return;

    message = &CampaignMessageTable[messageIndex];
    TextWindowContexts[0x0F].heightTiles =
        (word16) (int16) message->windowHeight;
    TextWindowContexts[0x0F].topTile = 0x0006;
    OpenModalTextWindow(0x000F, TRUE);
    HideMouseCursorOverlay();
    RenderFormattedText(CampaignMessageTextTable[messageIndex]);
    ShowMouseCursorOverlay();
    FlushPendingInput();
    if ((PresentationCallbackOffset |
            PresentationCallbackSegment) != 0x0000)
    {
        InvokePresentationCallback();
        SceneTransitionAbortFlag = FALSE;
    }
    else
    {
        WaitForDismissInput();
    }
    RestoreModalTextWindow();
}
