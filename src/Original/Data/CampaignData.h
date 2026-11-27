// 3A6A: campaign data declarations
#ifndef REVENGE_DATA_CAMPAIGN_DATA_H
#define REVENGE_DATA_CAMPAIGN_DATA_H

#include "revenge/types.h"

extern const char *const RestartConfirmationText;
extern const char *const MissionMapHeaderText;
extern char TextEntryBuffer[0x16];
extern uint16_t TextEntryCursorIndex;
extern uint8_t TextEntryActive;
extern const char TextEntryPunctuation[];
extern const char TextCursorBlinkGlyph[];
extern const uint8_t MonthLengthTable[0x0C];
/* DS:716E: the days before each month of a common year, and the year's. */
extern const uint16_t CalendarBoundaryTable[0x0D];
extern char StandardTimezoneName[0x04];
extern char DaylightTimezoneName[0x04];
extern uint16_t TimezoneOffsetLowWord;
extern uint16_t TimezoneOffsetHighWord;
extern uint16_t DaylightSavingEnabled;
extern const char TimezoneEnvironmentName[0x03];
extern const char DefaultStandardTimezoneName[0x04];
extern const char DefaultDaylightTimezoneName[0x04];

#endif
