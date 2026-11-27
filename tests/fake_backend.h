#ifndef REVENGE_TEST_FAKE_BACKEND_H
#define REVENGE_TEST_FAKE_BACKEND_H

#include "revenge/dos.h"
#include "revenge/input.h"
#include "revenge/video.h"

typedef struct FakeBackendState
{
    uint8_t consoleCharacters[0x40];
    uint16_t consoleCharacterCount;
    uint16_t firstWord;
    uint16_t secondWord;
    uint8_t firstByte;
    void *buffer;
    void (*gameClockHandler)(void);
    uint16_t musicClockDivisor;
    uint16_t digitalPlaybackClockDivisor;
    uint8_t biosKeyboardFlags[0x03];  /* 0040:0017, 0018, 0096 */
    uint16_t keyboardHandlerInstalled;
    uint16_t inputPolls;
    int16_t mouseX;                  /* where the game last put it (AX=4) */
    int16_t mouseY;
    uint16_t mouseEventMask;         /* the handler's events (AX=0Ch) */
    uint8_t videoMode;
    uint8_t vgaPalette[0x0300];
    uint16_t retraceCount;
    uint8_t indexedFramebuffer[VIDEO_SEGMENT_SIZE];
    uint8_t midiBytes[0x0100];
    uint16_t midiByteCount;     /* stops at the buffer's end */
    void (*midiSink)(uint8_t);   /* when set, every byte goes to it instead */
    uint8_t exitCode;            /* SDLBackend_TerminateProcess's, set before exit */
} FakeBackendState;

extern FakeBackendState fakeBackend;

void FakeBackend_Reset(void);

#endif
