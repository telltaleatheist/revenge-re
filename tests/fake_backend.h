#ifndef REVENGE_TEST_FAKE_BACKEND_H
#define REVENGE_TEST_FAKE_BACKEND_H

#include "revenge/dos.h"
#include "revenge/input.h"
#include "revenge/video.h"

typedef struct FakeBackendState
{
    byte consoleCharacters[0x40];
    uint16 consoleCharacterCount;
    byte drive;
    const char *path;
    uint16 fileHandle;
    uint16 firstWord;
    uint16 secondWord;
    byte firstByte;
    void *buffer;
    long fileOffset;
    DOS_InterruptHandler interruptHandler;
    uint32 interruptVectors[0x0100];
	uint32 chainedInterruptVector;
    uint32 biosTickCount;
    uint32 biosTickStep;
    word16 pitChannel0Divisor;
    byte pitChannel0Mode;
    byte acknowledgedInterrupt;
    uint16 rawKeypress;
    word16 inputPending;
    uint16 queuedRawKeypress;
    word16 queuedInputPending;
    MouseInputState mouse;
    byte videoMode;
    byte vgaPalette[0x0300];
    uint16 retraceCount;
    byte indexedFramebuffer[VIDEO_SEGMENT_SIZE];
    byte midiBytes[0x0100];
    uint16 midiByteCount;     /* stops at the buffer's end */
    void (*midiSink)(byte);   /* when set, every byte goes to it instead */
} FakeBackendState;

extern FakeBackendState fakeBackend;

void FakeBackend_Reset(void);

#endif
