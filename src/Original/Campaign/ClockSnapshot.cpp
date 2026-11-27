// 39CC: clock snapshot
// Reconstructed original local-clock snapshot procedure (segment 39CC).

#include "Campaign/Campaign.h"
#include "Data/CampaignData.h"
#include "Utility/Utility.h"

// 39CC:000F: void CaptureLocalClockSnapshot(LocalClockSnapshot far *snapshot)
// Called from:
//      UpdateTextEntryCursorBlink
void CaptureLocalClockSnapshot(LocalClockSnapshot *snapshot)
{
    DOS_Date dateBefore, dateAfter;
    DOS_Time time;
    int32_t timezoneOffsetSeconds;
    InitializeTimezoneFromEnvironment();
    do
    {
        DOS_GetSystemDate(&dateBefore);
        DOS_GetSystemTime(&time);
        DOS_GetSystemDate(&dateAfter);
    } while (dateBefore.year != dateAfter.year ||
        dateBefore.day != dateAfter.day || dateBefore.month != dateAfter.month);
    timezoneOffsetSeconds = (int32_t) MAKE_DWORD(
        TimezoneOffsetHighWord,
        TimezoneOffsetLowWord);
    snapshot->timezoneOffsetMinutes = (int16_t) (timezoneOffsetSeconds / 0x3C);
    if (DaylightSavingEnabled != 0x00 && IsDaylightSavingTime(dateBefore.year - 0x07B2, (int16_t) dateBefore.month, (int16_t) dateBefore.day, time.hour) != 0x00)
        snapshot->daylightSavingActive = TRUE;
    else
        snapshot->daylightSavingActive = FALSE;
    snapshot->epochSeconds = ConvertLocalDateTimeToEpochSeconds(&dateBefore, &time);
    snapshot->milliseconds = (uint16_t) time.hundredth * 0x0A;
}
