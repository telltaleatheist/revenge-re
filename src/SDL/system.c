#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "revenge/platform_backend.h"
#include "revenge/original_data.h"
#include "revenge/original_import.h"
#include "harness.h"
#include "revenge/original/campaign_data.h"
#include "revenge/original/lance_editor_data.h"
#include "revenge/audio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

_Noreturn void SDLBackend_TerminateProcess(byte exitCode)
{
	SDLBackend_Close();
	exit((int) exitCode);
}

void SDLBackend_GetSystemDate(DOS_Date *date)
{
    struct tm localTime;

    if (date == 0 || !HostClock_CalendarTime(&localTime, 0))
        return;

    date->year = (uint16)(localTime.tm_year + 0x076C);
    date->month = (byte)(localTime.tm_mon + 0x01);
    date->day = (byte)localTime.tm_mday;
}

void SDLBackend_GetSystemTime(DOS_Time *timeValue)
{
    struct tm localTime;
    uint32_t hundredths;

    if (timeValue == 0 || !HostClock_CalendarTime(&localTime, &hundredths))
        return;

    timeValue->hour = (byte)localTime.tm_hour;
    timeValue->minute = (byte)localTime.tm_min;
    timeValue->second = (byte)localTime.tm_sec;
    timeValue->hundredth = (byte)hundredths;
}

const char *SDLBackend_GetEnvironmentVariable(const char *name)
{
	return getenv(name);
}

