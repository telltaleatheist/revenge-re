// 3A07: epoch time
// Reconstructed original local-time to epoch converter (segment 3A07).

#include "Campaign/Campaign.h"
#include "Data/CampaignData.h"
#include "Utility/Utility.h"

uint16_t IsDaylightSavingTime(
    uint16_t yearsSince1970, uint16_t month, uint16_t dayOrDayOfYear, uint8_t hour);

constexpr uint32_t SECONDS_PER_MINUTE = 0x0000003C;
constexpr uint32_t SECONDS_PER_HOUR = 0x00000E10;
constexpr uint32_t SECONDS_PER_DAY = 0x00015180;
constexpr uint32_t SECONDS_PER_COMMON_YEAR = 0x01E13380;
constexpr uint32_t SECONDS_PER_FOUR_YEARS = 0x07861F80;
constexpr uint32_t EPOCH_SECONDS_AT = 0x12CEA600;

// 3A07:0001: convert the DOS local date/time fields to the runtime's signed-
// timezone-adjusted Unix epoch value. The original returns this in DX:AX.
uint32_t ConvertLocalDateTimeToEpochSeconds(
    const DOS_Date *date,
    const DOS_Time *time)
{
    int16_t yearsSince1980;
    uint16_t remainingYears;
    uint16_t monthIndex;
    uint16_t dayOfYear;
    uint16_t hourOfYear;
    int32_t timezoneOffsetSeconds;
    uint32_t epochSeconds;

    InitializeTimezoneFromEnvironment();
    timezoneOffsetSeconds = (int32_t) MAKE_DWORD(
        TimezoneOffsetHighWord,
        TimezoneOffsetLowWord);
    epochSeconds = (uint32_t) (timezoneOffsetSeconds + EPOCH_SECONDS_AT);

    yearsSince1980 = (int16_t) date->year - DOS_EPOCH_YEAR;
    epochSeconds += (uint32_t) (yearsSince1980 >> 0x02) * SECONDS_PER_FOUR_YEARS;
    remainingYears = yearsSince1980 & 0x03;
    epochSeconds += (uint32_t) remainingYears * SECONDS_PER_COMMON_YEAR;
    if (remainingYears != 0x00)
        epochSeconds += SECONDS_PER_DAY;

    dayOfYear = 0x00;
    for (monthIndex = 0x00; monthIndex + 0x01 < date->month; ++monthIndex)
        dayOfYear += MonthLengthTable[monthIndex];
    dayOfYear += date->day - 0x01;
    if (date->month > 0x02 && (date->year & 0x03) == 0x00)
        ++dayOfYear;

    hourOfYear = dayOfYear * 0x18 + time->hour;
    if (DaylightSavingEnabled != FALSE &&
        IsDaylightSavingTime(
            date->year - UNIX_EPOCH_YEAR, 0x00, dayOfYear, time->hour) != FALSE)
        --hourOfYear;

    epochSeconds += (uint32_t) hourOfYear * SECONDS_PER_HOUR;
    epochSeconds += (uint32_t) time->minute * SECONDS_PER_MINUTE;
    epochSeconds += time->second;
    return epochSeconds;
}
