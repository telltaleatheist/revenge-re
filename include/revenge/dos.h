#ifndef REVENGE_DOS_H
#define REVENGE_DOS_H

#include "revenge/types.h"

typedef void (*DOS_InterruptHandler)(void);

typedef struct DOS_Date
{
    uint16 year;
    byte month;
    byte day;
} DOS_Date;

typedef struct DOS_Time
{
    byte hour;
    byte minute;
    byte second;
    byte hundredth;
} DOS_Time;

uint16 DOS_PrintCharacter(char character);
void DOS_PrintString(const char *text);

void DOS_GetSystemDate(DOS_Date *date);
void DOS_GetSystemTime(DOS_Time *time);

#endif
