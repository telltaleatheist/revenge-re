// 0D73: scenario messages
/* Indexed scenario-message procedures from original segment 0D73. */

#include "Data/CampaignStateData.h"
#include "Data/TextIOData.h"
#include "Input/Input.h"
#include "Text/Text.h"
#include "revenge/presentation_callback_bridge.h"

#include "revenge/input.h"
#include "revenge/debug.h"
#include "revenge/original/campaign_data.h"

void OpenModalTextWindow(uint16_t windowIndex, uint16_t clearAndFrame);
void RestoreModalTextWindow(void);

/* 0D73:2E7C-2F15 */
void ShowScenarioMessage(uint16_t messageIndex)
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
        (uint16_t) (int16_t) message->windowHeight;
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
void ShowMissionFollowupMessage(uint16_t messageIndex)
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
        (uint16_t) (int16_t) message->windowHeight;
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
