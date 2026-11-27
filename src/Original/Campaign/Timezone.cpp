// 3A3C: timezone
// Reconstructed original timezone and daylight-saving runtime (segment 3A3C).

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "Campaign/Campaign.h"
#include "Data/CampaignData.h"
#include "Data/RuntimeData.h"
#include "IO/IO.h"
#include "Utility/Utility.h"

#include <string.h>

/* TZ's zone names are three letters; with no TZ the zone is five hours west
 * of UTC (the runtime's default names, DS's, say which). */
constexpr int TIMEZONE_NAME_LENGTH = 0x03;
constexpr uint16_t DEFAULT_TIMEZONE_OFFSET_SECONDS = 0x4650;

// 3A3C:0008
// Borland tzset path. Parse TZ as a three-character standard-zone name,
// signed decimal UTC offset in hours, and an optional three-character DST name.
void InitializeTimezoneFromEnvironment(void)
{
    const char *timezone = GetEnvironmentVariable(TimezoneEnvironmentName);
    uint16_t validTimezone = timezone != 0x00 && (uint16_t) strlen(timezone) >= 0x04;

    if (validTimezone)
    {
        uint16_t firstThreeAreLetters =
            (CharacterClassTable[(unsigned char) timezone[0x00]] & CHARACTER_CLASS_LETTER) != 0x00 &&
            (CharacterClassTable[(unsigned char) timezone[0x01]] & CHARACTER_CLASS_LETTER) != 0x00 &&
            (CharacterClassTable[(unsigned char) timezone[0x02]] & CHARACTER_CLASS_LETTER) != 0x00;
        uint16_t offsetStartsWithDigit =
            (CharacterClassTable[(unsigned char) timezone[0x03]] & CHARACTER_CLASS_DIGIT) != 0x00;
        uint16_t offsetStartsWithSign = timezone[0x03] == '-' || timezone[0x03] == '+';
        uint16_t signedOffsetHasDigit =
            (CharacterClassTable[(unsigned char) timezone[0x04]] & CHARACTER_CLASS_DIGIT) != 0x00;

        validTimezone = firstThreeAreLetters &&
            (offsetStartsWithDigit || offsetStartsWithSign) &&
            (offsetStartsWithDigit || signedOffsetHasDigit);
    }

    if (!validTimezone)
    {
        DaylightSavingEnabled = 0x01;
        TimezoneOffsetHighWord = 0x00;
        TimezoneOffsetLowWord = DEFAULT_TIMEZONE_OFFSET_SECONDS;
        strcpy(StandardTimezoneName, DefaultStandardTimezoneName);
        strcpy(DaylightTimezoneName, DefaultDaylightTimezoneName);
        return;
    }

    memset(DaylightTimezoneName, 0x00, 0x04);
    strncpy(StandardTimezoneName, timezone, TIMEZONE_NAME_LENGTH);
    StandardTimezoneName[TIMEZONE_NAME_LENGTH] = 0x00;

    {
        long offsetSeconds = ParseSignedDecimalLong(timezone + TIMEZONE_NAME_LENGTH) * 0x0E10L;
        TimezoneOffsetLowWord = (uint16_t) offsetSeconds;
        TimezoneOffsetHighWord = HIGH_WORD(offsetSeconds);
    }

    DaylightSavingEnabled = 0x00;
    {
        unsigned short index = 0x03;
        while (timezone[index] != '\0')
        {
            if ((CharacterClassTable[(unsigned char) timezone[index]] & CHARACTER_CLASS_LETTER) != 0x00)
            {
                if ((uint16_t) strlen(timezone + index) >= 0x03 &&
                    (CharacterClassTable[(unsigned char) timezone[index + 0x01]] & CHARACTER_CLASS_LETTER) != 0x00 &&
                    (CharacterClassTable[(unsigned char) timezone[index + 0x02]] & CHARACTER_CLASS_LETTER) != 0x00)
                {
                    strncpy(DaylightTimezoneName, timezone + index, TIMEZONE_NAME_LENGTH);
                    DaylightTimezoneName[TIMEZONE_NAME_LENGTH] = 0x00;
                    DaylightSavingEnabled = 0x01;
                }
                return;
            }
            ++index;
        }
    }
}

// 3A3C:01ED: word16 IsDaylightSavingTime(
//     word16 yearsSince1970, word16 month,
//     word16 dayOrDayOfYear, byte hour)
// Called from:
//      CaptureLocalClockSnapshot
//      ConvertLocalDateTimeToEpochSeconds
uint16_t IsDaylightSavingTime(
    uint16_t yearsSince1970,
    uint16_t month,
    uint16_t dayOrDayOfYear,
    uint8_t hour)
{
    uint16_t resolvedMonth = month;
    if (month == 0x00)
    {
        uint16_t normalizedDay = dayOrDayOfYear;
        if (dayOrDayOfYear >= 0x3B && ((yearsSince1970 + 0x46) & 0x03) == 0x00)
            normalizedDay = dayOrDayOfYear - 0x01;
        resolvedMonth = 0x00;
        while (CalendarBoundaryTable[resolvedMonth] <= normalizedDay)
            ++resolvedMonth;
    }
    else
    {
        if (month < 0x03 || ((yearsSince1970 + 0x46) & 0x03) != 0x00)
            --dayOrDayOfYear;
        dayOrDayOfYear += CalendarBoundaryTable[month - 0x01];
    }
    if (resolvedMonth < 0x04)
    {
        return FALSE;
    }
    if (resolvedMonth != 0x04)
    {
        if (resolvedMonth > 0x0A)
            return FALSE;
        if (resolvedMonth != 0x0A)
            return TRUE;
    }
    uint16_t transitionDay;
    if (yearsSince1970 > 0x10 && resolvedMonth == 0x04)
        transitionDay = CalendarBoundaryTable[resolvedMonth - 0x01] + 0x07;
    else
        transitionDay = CalendarBoundaryTable[resolvedMonth];
    if ((((uint8_t) yearsSince1970 + 0xB2) & 0x03) != 0x00)
        --transitionDay;
    transitionDay -= (uint16_t) ((uint32_t) (yearsSince1970 * 0x016D +
                                     transitionDay + ((yearsSince1970 + 0x01) >> 0x02) + 0x04) %
        0x07);
    if (resolvedMonth == 0x04)
    {
        if (dayOrDayOfYear <= transitionDay &&
            (dayOrDayOfYear != transitionDay || hour < 0x02))
            return FALSE;
    }
    else if (dayOrDayOfYear >= transitionDay &&
        (dayOrDayOfYear != transitionDay || hour > 0x01))
        return FALSE;
    return TRUE;
}
