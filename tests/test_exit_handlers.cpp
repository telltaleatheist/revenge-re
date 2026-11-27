#include "REVENGE.h"
#include "fake_backend.h"
#include "revenge/original_data.h"

#include <stdio.h>
#include <stdlib.h>

/* 38C6:0010 and the hooks it calls. As the image has them, the three exit
 * hooks (DS:700A, 700E, 7012) are the C library's stub at 38C6:000F, the
 * termination hook (DS:71A4) is its stub at 0800:019F, and the resource
 * error callbacks (DS:5FBE, 5FC2) are 3390:071E and 3390:097B: none is
 * left empty, and each stub returns. Then the order: the registered handlers
 * last to first, the three hooks, the termination hook, and the exit code
 * to the host. The host's end comes through exit(), so the order is checked
 * in an atexit function. */

static byte callOrder[0x08];
static byte callCount;

static void RecordCall(byte callId)
{
	callOrder[callCount++] = callId;
}

static void ExitHandler0(void) { RecordCall(0x01); }
static void ExitHandler1(void) { RecordCall(0x02); }
static void ExitHandler2(void) { RecordCall(0x03); }
static void RuntimeHook1(void) { RecordCall(0x04); }
static void RuntimeHook2(void) { RecordCall(0x05); }
static void RuntimeHook3(void) { RecordCall(0x06); }
static void TerminationHook(void) { RecordCall(0x07); }

static void CheckAtExit(void)
{
	static const byte expectedOrder[0x07] =
	{
		0x03, 0x02, 0x01, 0x04, 0x05, 0x06, 0x07
	};
	word16 index;

	if (ExitHandlerCount != 0xFFFF || fakeBackend.exitCode != 0x34 ||
		callCount != 0x07)
	{
		fputs("exit-handler state mismatch\n", stderr);
		_Exit(0x01);
	}
	for (index = 0x0000; index < 0x0007; ++index)
	{
		if (callOrder[index] != expectedOrder[index])
		{
			fputs("exit-handler call order mismatch\n", stderr);
			_Exit(0x01);
		}
	}
	_Exit(0x00);
}

int main(void)
{
	if (RuntimeExitHook1 != RuntimeExitStepStub ||
		RuntimeExitHook2 != RuntimeExitStepStub ||
		RuntimeExitHook3 != RuntimeExitStepStub ||
		RuntimeTerminationHook != RuntimeTerminationStub ||
		ResourceErrorDisplayCallback != ShowResourceErrorMessage ||
		ResourceErrorDismissCallback != ShowFatalDiskErrorMessage)
	{
		fputs("a hook isn't the image's\n", stderr);
		return 0x01;
	}
	RuntimeExitHook1();
	RuntimeExitHook2();
	RuntimeExitHook3();
	RuntimeTerminationHook();

	FakeBackend_Reset();
	ExitHandlerCount = 0x0003;
	ExitHandlerTable[0x00] = ExitHandler0;
	ExitHandlerTable[0x01] = ExitHandler1;
	ExitHandlerTable[0x02] = ExitHandler2;
	RuntimeExitHook1 = RuntimeHook1;
	RuntimeExitHook2 = RuntimeHook2;
	RuntimeExitHook3 = RuntimeHook3;
	RuntimeTerminationHook = TerminationHook;
	if (atexit(CheckAtExit) != 0)
		return 0x01;

	/* The DOS exit takes the code's low byte (AL). */
	RunExitHandlersAndTerminate(0x1234);
	fputs("RunExitHandlersAndTerminate returned\n", stderr);
	return 0x01;
}
