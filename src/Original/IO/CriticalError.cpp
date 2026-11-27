// 34A7: critical error
// CriticalError.cpp
// DOS critical-error handler installation.

#include "IO/IO.h"
#include "Utility/Utility.h"
#include "revenge/original/io_data.h"

constexpr uint16_t CRITICAL_ERROR_DEFAULT_CALLBACK_OFFSET = 0x0006;
constexpr uint16_t CRITICAL_ERROR_CODE_SEGMENT = 0x34A7;

// 34A7:0059: retain the application callback and install the resident INT 24h
// assembly dispatcher at 34A7:0021, which preserves all registers, switches
// to the game DS and invokes the retained callback. The install (34A7:0067-
// 0074, a call of DOS_SetInterruptVector, 39E3:001E) goes with the vector
// table: the host raises no critical error, a game file that fails returns
// -1 instead (revenge/game_file.h).
void InstallCriticalErrorHandler(void *criticalErrorCallback)
{
    uintptr_t callbackToken;

    callbackToken = (uintptr_t) criticalErrorCallback;
    CriticalErrorCallback.offset = LOW_WORD(callbackToken);
    CriticalErrorCallback.segment = HIGH_WORD(callbackToken);
}

// 34A7:0014: install the executable's default INT 24h callback at CS:0006.
// Called from InitializeGame.
void InstallDefaultCriticalErrorHandler(void)
{
    InstallCriticalErrorHandler(
        (void *) (uintptr_t) MAKE_FAR_POINTER(
            CRITICAL_ERROR_CODE_SEGMENT,
            CRITICAL_ERROR_DEFAULT_CALLBACK_OFFSET));
}
