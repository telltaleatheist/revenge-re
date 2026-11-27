// 0D73: presentation
/* Campaign screen/message presentation procedure from original segment 0D73. */

#include "Campaign/Campaign.h"
#include "Data/CampaignStateData.h"
#include "Input/Input.h"
#include "Text/Text.h"

/* 0D73:341F-347C */
uint16_t PresentScreenAndMessage(
    uint16_t screenResourceIndex,
    uint16_t messageIndex)
{
    uint16_t dismissKey;
    uint16_t skipWait;

    dismissKey = 0x0000;
    skipWait = FALSE;
    if ((PresentationCallbackOffset |
            PresentationCallbackSegment) != 0x0000)
    {
        SceneTransitionAbortFlag = FALSE;
        skipWait = TRUE;
    }
    if (SceneTransitionAbortFlag == FALSE)
    {
        if (screenResourceIndex != 0x00FF)
            LoadAndPresentScreen(screenResourceIndex);
        if (skipWait == FALSE)
            dismissKey = FlushInputAndWait(
                PresentationDismissTimeout);
    }
    if (SceneTransitionAbortFlag == FALSE && messageIndex != 0x0000)
    {
        ShowScenarioMessage(messageIndex);
        dismissKey = 0x0000;
    }
    return dismissKey;
}
