/* Mechanical dispatch for the original DS:24E6:24E8 far callback. */

#include "Data/CampaignStateData.h"
#include "revenge/presentation_callback_bridge.h"

void RenderActivePilotList(void);
void RenderSurvivingCommanderList(void);
void RenderFallenPilotList(void);
void RunCampaignPathSelectionCallback(void);
void RunScenarioVariantSelectionCallback(void);
void RunAlternateScenarioVariantSelectionCallback(void);

void InvokePresentationCallback(void)
{
    if (PresentationCallbackSegment != PRESENTATION_CALLBACK_SEGMENT)
        return;

    switch (PresentationCallbackOffset)
    {
    case PRESENTATION_CALLBACK_ACTIVE_PILOT_LIST: RenderActivePilotList(); break;
    case PRESENTATION_CALLBACK_SURVIVING_COMMANDER_LIST: RenderSurvivingCommanderList(); break;
    case PRESENTATION_CALLBACK_FALLEN_PILOT_LIST: RenderFallenPilotList(); break;
    case PRESENTATION_CALLBACK_CAMPAIGN_PATH_SELECTION: RunCampaignPathSelectionCallback(); break;
    case PRESENTATION_CALLBACK_SCENARIO_VARIANT_SELECTION: RunScenarioVariantSelectionCallback(); break;
    case PRESENTATION_CALLBACK_ALTERNATE_SCENARIO_VARIANT_SELECTION: RunAlternateScenarioVariantSelectionCallback(); break;
    default: break;
    }
}
