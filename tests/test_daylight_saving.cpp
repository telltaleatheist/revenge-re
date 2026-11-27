#include "REVENGE.h"

#include <stdio.h>

/* 3A3C:01ED, the C runtime's daylight-saving test (Timezone.cpp),
 * over every input the clock can give it: each year since 1970 to 2097, a day
 * of the year that year has (month 0) or a month and day, and the hours either
 * side of the 2 a.m. change. (A day of the year past the year's end would
 * read past the table, in the original as here.) The digest is of the port's answers before its calendar
 * table became the original's (DS:716E), which moved none of them. */
uint16_t IsDaylightSavingTime(
    uint16_t yearsSince1970,
    uint16_t month,
    uint16_t dayOrDayOfYear,
    uint8_t hour);

#define DAYLIGHT_DIGEST 0xBFE2FEF5UL

static unsigned long Mix(unsigned long digest, uint16_t answer)
{
    return ((digest ^ (answer != 0x00 ? 0x01UL : 0x00UL)) * 0x01000193UL) & 0xFFFFFFFFUL;
}

int main(void)
{
    unsigned long digest = 0x811C9DC5UL;
    uint16_t years;
    uint16_t month;
    uint16_t day;
    uint8_t hour;
    int failures = 0;

    for (years = 0x00; years < 0x80; ++years)
    {
        uint16_t lastDay = ((years + 0x46) & 0x03) == 0x00 ? 0x016D : 0x016C;

        for (day = 0x00; day <= lastDay; ++day)
            for (hour = 0x00; hour < 0x04; ++hour)
                digest = Mix(digest, IsDaylightSavingTime(years, 0x00, day, hour));
        for (month = 0x01; month <= 0x0C; ++month)
            for (day = 0x01; day <= 0x1F; ++day)
                for (hour = 0x00; hour < 0x04; ++hour)
                    digest = Mix(digest, IsDaylightSavingTime(years, month, day, hour));
    }
    /* 1990: July in daylight time, January not. */
    if (IsDaylightSavingTime(0x14, 0x07, 0x04, 0x0C) == 0x00)
    {
        puts("1990-07-04 isn't daylight time");
        ++failures;
    }
    if (IsDaylightSavingTime(0x14, 0x01, 0x01, 0x0C) != 0x00)
    {
        puts("1990-01-01 is daylight time");
        ++failures;
    }
    if (digest != DAYLIGHT_DIGEST)
    {
        printf("daylight digest %08lX, expected %08lX\n", digest, DAYLIGHT_DIGEST);
        ++failures;
    }
    return failures == 0 ? 0 : 1;
}
