// 0D73, 34A7, 38C6, 39DC: IO declarations
#ifndef REVENGE_IO_IO_H
#define REVENGE_IO_IO_H

#include "Data/ResourceData.h"
#include "revenge/types.h"

int16_t EnsureCompatibleSaveFile(
    int16_t fileHandle,
    char *savePath,
    uint16_t openMode);
void RuntimeExitStepStub(void);
void RunExitHandlersAndTerminate(uint16_t exitCode);
char *GetEnvironmentVariable(const char *name);
void InstallDefaultCriticalErrorHandler(void);
void InstallCriticalErrorHandler(void *criticalErrorCallback);
extern OriginalExitHandler RuntimeExitHook1;
extern OriginalExitHandler RuntimeExitHook2;
extern OriginalExitHandler RuntimeExitHook3;

#endif
