#ifndef REVENGE_DOS_H
#define REVENGE_DOS_H

#include "revenge/types.h"

typedef void (*DOS_InterruptHandler)(void);

typedef struct DOS_Date
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
} DOS_Date;

typedef struct DOS_Time
{
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t hundredth;
} DOS_Time;

uint16_t DOS_PrintCharacter(char character);
void DOS_PrintString(const char *text);

void DOS_GetSystemDate(DOS_Date *date);
void DOS_GetSystemTime(DOS_Time *time);

#endif
