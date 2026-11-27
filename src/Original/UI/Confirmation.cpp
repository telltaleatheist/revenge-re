// 0D73: confirmation
/* Restart-confirmation procedure from original segment 0D73. */

#include "Campaign/Campaign.h"
#include "Data/CampaignData.h"
#include "UI/UI.h"
#include "revenge/original/input_data.h"

/* 0D73:242E-2447 */
uint16_t ConfirmRestartGame(void)
{
    /* DS:0822: the confirmation's region (7) starts on its second row. */
    UiRegionTable[0x07].selectedRow = 0x0001;
    return ShowConfirmationDialog(RestartConfirmationText) == 0x0000;
}
