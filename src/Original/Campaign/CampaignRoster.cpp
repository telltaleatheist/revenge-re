// 0D73: campaign roster
// CampaignRoster.cpp
// Procedures reconstructed from REVENGE.EXE segment 0D73.

#include "CampaignInternal.h"
#include "Data/RuntimeData.h"
#include "Data/TacticalData.h"
#include "Input/Input.h"
#include "revenge/text.h"

// 0D73:2101: byte PrepareActivePilotList(void)
// VERIFIED against 0D73:2101-214A.
uint8_t PrepareActivePilotList(void)
{
    uint16_t pilotSlot;
    uint16_t lineCount;
    uint16_t windowHeight;

    lineCount = 0x0003;
    for (pilotSlot = 0x0000; pilotSlot < 0x0025; ++pilotSlot)
    {
        if (PilotRoster[pilotSlot].availability == 0x01)
        {
            ++lineCount;
        }
    }
    windowHeight = (uint16_t) ((ActiveFontLineHeight * lineCount +
                                   0x0007) >>
        0x03);
    ActivePilotListWindowHeight = (uint8_t) windowHeight;
    return (uint8_t) windowHeight;
}

// 0D73:214B: void RenderActivePilotList(void)
// VERIFIED against 0D73:214B-21A4.
void RenderActivePilotList(void)
{
    uint16_t pilotSlot;

    HideMouseCursorOverlay();
    RenderFormattedText(PilotRosterHeadingText);
    ShowMouseCursorOverlay();
    for (pilotSlot = 0x0000; pilotSlot < 0x0025; ++pilotSlot)
    {
        if (PilotRoster[pilotSlot].availability == 0x01)
        {
            RenderPilotName(
                PilotRoster[pilotSlot].identityId);
        }
    }
    WaitForDismissInput();
}

// 0D73:21A5: byte PrepareSurvivingCommanderList(void)
// VERIFIED against 0D73:21A5-223C.
uint8_t PrepareSurvivingCommanderList(void)
{
    uint16_t firstUnitIndex;
    uint16_t unitSlot;
    uint16_t lineCount;
    uint16_t windowHeight;

    lineCount = 0x0003;
    for (firstUnitIndex = 0x0000;
        firstUnitIndex < 0x000C;
        firstUnitIndex += 0x0004)
    {
        for (unitSlot = 0x0000; unitSlot < 0x0004; ++unitSlot)
        {
            RevengeUnitRecord *unitRecord = firstUnitIndex < 0x0008 ? &PersistentUnitRoster[firstUnitIndex + unitSlot] : &LiveScenarioUnits[firstUnitIndex + unitSlot];
            if (unitRecord->tonnage != 0x00 &&
                unitRecord->deploymentState != 0x00)
            {
                ++lineCount;
                break;
            }
        }
    }
    windowHeight = (uint16_t) ((ActiveFontLineHeight * lineCount +
                                   0x0007) >>
        0x03);
    CommanderListWindowHeight = (uint8_t) windowHeight;
    return (uint8_t) windowHeight;
}

// 0D73:223D: void RenderSurvivingCommanderList(void)
// VERIFIED against 0D73:223D-22E7.
void RenderSurvivingCommanderList(void)
{
    uint16_t firstUnitIndex;
    uint16_t unitSlot;

    HideMouseCursorOverlay();
    RenderFormattedText(PilotRosterHeadingText);
    ShowMouseCursorOverlay();
    for (firstUnitIndex = 0x0000;
        firstUnitIndex < 0x000C;
        firstUnitIndex += 0x0004)
    {
        for (unitSlot = 0x0000; unitSlot < 0x0004; ++unitSlot)
        {
            RevengeUnitRecord *unitRecord = firstUnitIndex < 0x0008 ? &PersistentUnitRoster[firstUnitIndex + unitSlot] : &LiveScenarioUnits[firstUnitIndex + unitSlot];
            if (unitRecord->tonnage != 0x00 &&
                unitRecord->deploymentState != 0x00)
            {
                RenderPilotName(unitRecord->pilotId);
                break;
            }
        }
    }
    WaitForDismissInput();
}

// 0D73:22E8: word16 HasFallenPilots(void)
// VERIFIED against 0D73:22E8-2316.
uint16_t HasFallenPilots(void)
{
    uint16_t pilotSlot;

    for (pilotSlot = 0x0000; pilotSlot < 0x0025; ++pilotSlot)
    {
        if (PilotRoster[pilotSlot].availability == 0xFF)
        {
            return TRUE;
        }
    }
    return FALSE;
}

// 0D73:2317: byte PrepareFallenPilotList(void)
// VERIFIED against 0D73:2317-2360.
uint8_t PrepareFallenPilotList(void)
{
    uint16_t pilotSlot;
    uint16_t lineCount;
    uint16_t windowHeight;

    lineCount = 0x0002;
    for (pilotSlot = 0x0000; pilotSlot < 0x0025; ++pilotSlot)
    {
        if (PilotRoster[pilotSlot].availability == 0xFF)
        {
            ++lineCount;
        }
    }
    windowHeight = (uint16_t) ((ActiveFontLineHeight * lineCount +
                                   0x0007) >>
        0x03);
    FallenPilotListWindowHeight = (uint8_t) windowHeight;
    return (uint8_t) windowHeight;
}

// 0D73:2361: void RenderFallenPilotList(void)
// VERIFIED against 0D73:2361-23BA.
void RenderFallenPilotList(void)
{
    uint16_t pilotSlot;

    HideMouseCursorOverlay();
    RenderFormattedText(PilotRosterHeadingText);
    ShowMouseCursorOverlay();
    for (pilotSlot = 0x0000; pilotSlot < 0x0025; ++pilotSlot)
    {
        if (PilotRoster[pilotSlot].availability == 0xFF)
        {
            RenderPilotName(
                PilotRoster[pilotSlot].identityId);
        }
    }
    WaitForDismissInput();
}

// 0D73:23BB: void RenderPilotName(RevengePilotId pilotId)
// VERIFIED against 0D73:23BB-2413.
void RenderPilotName(RevengePilotId pilotId)
{
    HideMouseCursorOverlay();
    RenderFormattedText(PilotIdentityPrefixText);
    RenderFormattedText(PilotGivenNameTextTable[pilotId]);
    RenderFormattedText(PilotNamePrefixText);
    RenderFormattedText(PilotSurnameTextTable[pilotId]);
    ShowMouseCursorOverlay();
}
