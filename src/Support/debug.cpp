#define _CRT_SECURE_NO_WARNINGS

#include "revenge/debug.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static LONG WINAPI RevengeUnhandledException(
    EXCEPTION_POINTERS *exceptionPointers)
{
    const void *moduleBase = GetModuleHandleA(NULL);
    const void *exceptionAddress =
        exceptionPointers->ExceptionRecord->ExceptionAddress;
    uintptr_t moduleOffset = (uintptr_t) exceptionAddress -
        (uintptr_t) moduleBase;

    fprintf(stderr,
        "crash: code=%08lX address=%p module=%p rva=%llX\n",
        exceptionPointers->ExceptionRecord->ExceptionCode,
        exceptionAddress, moduleBase,
        (unsigned long long) moduleOffset);
    fflush(stderr);
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif

void RevengeTrace(const char *format, ...)
{
    va_list arguments;

    if (getenv("REVENGE_TRACE") == NULL)
        return;
    va_start(arguments, format);
    vfprintf(stderr, format, arguments);
    va_end(arguments);
    fflush(stderr);
}

void RevengeFault(const char *format, ...)
{
    va_list arguments;

    fputs("fault: ", stderr);
    va_start(arguments, format);
    vfprintf(stderr, format, arguments);
    va_end(arguments);
    fputc('\n', stderr);
    fflush(stderr);
    exit(70);
}

size_t RevengeCheckIndex(const char *object, size_t index, size_t count)
{
    if (index >= count)
        RevengeFault("%s[%llu] is past its %llu entries", object,
            (unsigned long long) index, (unsigned long long) count);
    return index;
}

void RevengeInstallCrashDiagnostics(void)
{
#ifdef _WIN32
    SetUnhandledExceptionFilter(RevengeUnhandledException);
#endif
}
