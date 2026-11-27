#ifndef REVENGE_TEST_FAKE_BACKEND_H
#define REVENGE_TEST_FAKE_BACKEND_H

#include "revenge/dos.h"
#include "revenge/input.h"
#include "revenge/video.h"

typedef struct FakeBackendState
{
    byte consoleCharacters[0x40];
    uint16 consoleCharacterCount;
    uint16 firstWord;
    uint16 secondWord;
    byte firstByte;
    void *buffer;
    void (*gameClockHandler)(void);
    word16 musicClockDivisor;
    word16 digitalPlaybackClockDivisor;
    byte biosKeyboardFlags[0x03];  /* 0040:0017, 0018, 0096 */
    word16 keyboardHandlerInstalled;
    word16 inputPolls;
    int16 mouseX;                  /* where the game last put it (AX=4) */
    int16 mouseY;
    word16 mouseEventMask;         /* the handler's events (AX=0Ch) */
    byte videoMode;
    byte vgaPalette[0x0300];
    uint16 retraceCount;
    byte indexedFramebuffer[VIDEO_SEGMENT_SIZE];
    byte midiBytes[0x0100];
    uint16 midiByteCount;     /* stops at the buffer's end */
    void (*midiSink)(byte);   /* when set, every byte goes to it instead */
    byte exitCode;            /* SDLBackend_TerminateProcess's, set before exit */
} FakeBackendState;

extern FakeBackendState fakeBackend;

void FakeBackend_Reset(void);

#endif
