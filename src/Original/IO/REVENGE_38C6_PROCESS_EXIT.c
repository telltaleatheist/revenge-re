// Reconstructed original exit-handler dispatcher (segment 38C6).

#include "REVENGE.h"
#include "revenge/original_data.h"

void DOS_TerminateProcess(word16 exitCode);

// 38C6:000F: void RuntimeExitStepStub(void)
// The C library's stand-in for its three exit steps, the far pointers at
// DS:700A, 700E and 7012 that RunExitHandlersAndTerminate calls (38C6:002C-
// 0034): a lone retf, the byte before 38C6:0010 (38C3:003F in the listing).
// The image holds 38C6:000F in all three, and no instruction writes them:
// the library's buffered-stream and open-file closers that would replace
// them aren't in the program. The game's files are its own tracked handles
// (3390:0037), which DOS closed at the exit and the host closes here.
void RuntimeExitStepStub(void)
{
}

/* DS:700A, 700E, 7012: 38C6:000F in the image, never written. */
OriginalExitHandler RuntimeExitHook1 = RuntimeExitStepStub;
OriginalExitHandler RuntimeExitHook2 = RuntimeExitStepStub;
OriginalExitHandler RuntimeExitHook3 = RuntimeExitStepStub;

// 38C6:0010: void RunExitHandlersAndTerminate(word16 exitCode)
// Called from:
//      PrintAllocationErrorAndExit
//      ShowInvalidSaveVersionDialog
//      ShutdownAndExit
void RunExitHandlersAndTerminate(word16 exitCode)
{
	word16 previousHandlerCount;

	do
	{
		previousHandlerCount = ExitHandlerCount;
		--ExitHandlerCount;
		if (previousHandlerCount != 0x00)
			ExitHandlerTable[ExitHandlerCount]();
	}
	while (previousHandlerCount != 0x00);

	RuntimeExitHook1();
	RuntimeExitHook2();
	RuntimeExitHook3();
	DOS_TerminateProcess(exitCode);
}
