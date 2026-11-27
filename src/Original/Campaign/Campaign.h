// 0D73, 39CC, 3A07, 3A3C: Campaign declarations
#ifndef REVENGE_CAMPAIGN_CAMPAIGN_H
#define REVENGE_CAMPAIGN_CAMPAIGN_H

#include "revenge/dos.h"
#include "revenge/types.h"

#pragma pack(push, 1)
typedef struct LocalClockSnapshot
{
    uint32_t epochSeconds;
    uint16_t milliseconds;
    int16_t timezoneOffsetMinutes;
    uint16_t daylightSavingActive;
} LocalClockSnapshot;
#pragma pack(pop)

static_assert(sizeof(LocalClockSnapshot) == 0x0A,
    "LocalClockSnapshot must retain its DOS layout");

uint16_t ShowConfirmationDialog(const char *message);
void LoadAndPresentScreen(uint16_t resourceIndex);
uint16_t PresentScreenAndMessage(
    uint16_t screenResourceIndex,
    uint16_t messageIndex);
void CaptureLocalClockSnapshot(LocalClockSnapshot *snapshot);
void DrawCenteredClippedText(
    const char *sourceText,
    uint16_t splitIndex,
    uint16_t leftX,
    uint16_t y,
    uint16_t maximumWidth,
    uint8_t foregroundColor,
    uint8_t backgroundColor);
uint16_t WaitForDismissInputOrTimeout(uint16_t timeoutRetraces);
void LoadScreenAssetToWorkBuffer(uint16_t resourceIndex);
void ShowScenarioCompletionStatus(void);
void InitializeTimezoneFromEnvironment(void);
uint16_t IsDaylightSavingTime(
    uint16_t yearsSince1970,
    uint16_t month,
    uint16_t dayOrDayOfYear,
    uint8_t hour);
uint32_t ConvertLocalDateTimeToEpochSeconds(
    const DOS_Date *date,
    const DOS_Time *time);
void RunCampaignLoop(void);

#endif
