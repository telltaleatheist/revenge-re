#include "fake_backend.h"
#include "revenge/game_file.h"
#include "revenge/media_settings.h"
#include "revenge/platform_backend.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FakeBackendState fakeBackend;

REVENGE_NORETURN void SDLBackend_TerminateProcess(uint8_t exitCode)
{
    fakeBackend.exitCode = exitCode;
    exit((int) exitCode);
}

void FakeBackend_Reset(void)
{
    memset(&fakeBackend, 0, sizeof(fakeBackend));
}

uint16_t SDLBackend_DirectConsoleIO(uint8_t character)
{
    fakeBackend.consoleCharacters[fakeBackend.consoleCharacterCount++] = character;
    return (uint16_t) (0x0600 | character);
}

/* Game files as the DOS fakes had them: every open succeeds, a read or a
 * write reports all its bytes moved and leaves the buffer as it was, and a
 * seek lands at 12345678h. */
int16_t GameFile_Open(const char *name, GameFileAccess access)
{
    (void) name;
    (void) access;
    return 0x0006;
}

int16_t GameFile_Close(int16_t handle)
{
    (void) handle;
    return 0x0000;
}

int16_t GameFile_Read(int16_t handle, void *destination, uint16_t byteCount)
{
    (void) handle;
    (void) destination;
    return (int16_t) byteCount;
}

int16_t GameFile_Write(int16_t handle, const void *source, uint16_t byteCount)
{
    (void) handle;
    (void) source;
    return (int16_t) byteCount;
}

int32_t GameFile_Seek(int16_t handle, int32_t offset, GameFileOrigin origin)
{
    (void) handle;
    (void) offset;
    (void) origin;
    return 0x12345678L;
}

int32_t GameFile_Length(int16_t handle)
{
    (void) handle;
    return 0x12345678L;
}

int16_t GameFile_Delete(const char *name)
{
    (void) name;
    return 0x0000;
}

void SDLBackend_GetSystemDate(DOS_Date *date)
{
    date->year = 0x07EA;
    date->month = 0x09;
    date->day = 0x1A;
}

void SDLBackend_GetSystemTime(DOS_Time *timeValue)
{
    timeValue->hour = 0x0C;
    timeValue->minute = 0x22;
    timeValue->second = 0x38;
    timeValue->hundredth = 0x4E;
}

const char *SDLBackend_GetEnvironmentVariable(const char *name)
{
    (void) name;
    return 0;
}

void SDLBackend_SetGameClockHandler(void (*handler)(void))
{
    fakeBackend.gameClockHandler = handler;
}

void SDLBackend_StartMusicClock(uint16_t pitDivisor)
{
    fakeBackend.musicClockDivisor = pitDivisor;
}

void SDLBackend_StopMusicClock(void)
{
    fakeBackend.musicClockDivisor = 0x0000;
}

void SDLBackend_StartDigitalPlaybackClock(uint16_t pitDivisor)
{
    fakeBackend.digitalPlaybackClockDivisor = pitDivisor;
}

void SDLBackend_StopDigitalPlaybackClock(void)
{
    fakeBackend.digitalPlaybackClockDivisor = 0x0000;
}

void SDLBackend_PollInput(void)
{
    ++fakeBackend.inputPolls;
}

/* The game waits here only when its keyboard ring is empty (3037:01B5), and
 * nothing fills the ring on this backend but the test itself: the wait would
 * never end. A test that reaches it is waiting for a key it didn't queue (a
 * message box, an error it didn't expect), so it fails here, naming that. */
void SDLBackend_WaitInput(void)
{
    ++fakeBackend.inputPolls;
    fputs("fake backend: the game waits for a key, and none is queued\n", stderr);
    abort();
}

uint8_t SDLBackend_ReadBiosKeyboardFlags(uint16_t offset)
{
    switch (offset)
    {
    case 0x0017: return fakeBackend.biosKeyboardFlags[0x00];
    case 0x0018: return fakeBackend.biosKeyboardFlags[0x01];
    case 0x0096: return fakeBackend.biosKeyboardFlags[0x02];
    default: return 0x00;
    }
}

int SDLBackend_PollQuit(void)
{
    return FALSE;
}

uint64_t SDLBackend_Milliseconds(void)
{
    return 0x00000000U;
}

void SDLBackend_Delay(uint32_t milliseconds)
{
    (void) milliseconds;
}

void SDLBackend_InstallKeyboardHandlers(void)
{
    fakeBackend.keyboardHandlerInstalled = TRUE;
}

void SDLBackend_RestoreKeyboardHandlers(void)
{
    fakeBackend.keyboardHandlerInstalled = FALSE;
}

void SDLBackend_SetMouseEventHandler(uint16_t eventMask)
{
    fakeBackend.mouseEventMask = eventMask;
}

uint8_t SDLBackend_GetVideoMode(void)
{
    return fakeBackend.videoMode;
}

void SDLBackend_SetVideoMode(uint8_t videoMode)
{
    fakeBackend.videoMode = videoMode;
}

void SDLBackend_LoadVgaPalette(const uint8_t *paletteRgbTriplets)
{
    memcpy(fakeBackend.vgaPalette, paletteRgbTriplets,
        sizeof(fakeBackend.vgaPalette));
}

void SDLBackend_WaitVerticalRetrace(uint8_t waitForRetraceStart)
{
    fakeBackend.firstByte = waitForRetraceStart;
    ++fakeBackend.retraceCount;
}

void SDLBackend_SetMouseVisible(uint16_t visible)
{
    (void) visible;
}

void SDLBackend_SetMousePosition(int16_t logicalX, int16_t logicalY)
{
    fakeBackend.mouseX = logicalX;
    fakeBackend.mouseY = logicalY;
}

uint8_t *SDLBackend_GetIndexedFramebuffer(void)
{
    return fakeBackend.indexedFramebuffer;
}

void SDLBackend_ClearIndexedFramebuffer(uint8_t color)
{
    memset(fakeBackend.indexedFramebuffer, color, 0x0140 * 0x00C8);
}

void SDLBackend_DrawIndexedPixel(uint16_t x, uint16_t y, uint8_t color)
{
    if (x < 0x0140 && y < 0x00C8)
        fakeBackend.indexedFramebuffer[(uint32_t) y * 0x0140 + x] = color;
}

int SDLBackend_PresentIndexedFramebuffer(void)
{
    return TRUE;
}

void Video_PresentTransientFrame(void)
{
}

int SDLBackend_OpenAudio(void)
{
    return TRUE;
}

void SDLBackend_CloseAudio(void)
{
}

uint8_t SDLBackend_EnablePCSpeaker(void)
{
    return 0x03;
}

void SDLBackend_SilencePCSpeaker(void)
{
}

void SDLBackend_WritePCSpeakerPITDivisor(uint16_t divisor)
{
    fakeBackend.firstWord = divisor;
}

void SDLBackend_AdvanceAudio(void)
{
}

void SDLBackend_PlaySpeech(const uint8_t *samples, uint16_t sampleCount, uint16_t sampleRateHz)
{
    fakeBackend.buffer = (void *) samples;
    fakeBackend.secondWord = sampleCount;
    fakeBackend.firstWord = sampleRateHz;
}

void SDLBackend_HoldTimers(uint64_t nanoseconds)
{
    (void) nanoseconds;
}

void SDLBackend_WriteMIDIByte(uint8_t midiByte)
{
    if (fakeBackend.midiSink != NULL)
        fakeBackend.midiSink(midiByte);
    else if (fakeBackend.midiByteCount < sizeof(fakeBackend.midiBytes))
        fakeBackend.midiBytes[fakeBackend.midiByteCount++] = midiByte;
}

void SDLBackend_SetDigitalOutputRate(uint16_t sampleRateHz)
{
    fakeBackend.firstWord = sampleRateHz;
}

void SDLBackend_WriteUnsigned8Sample(uint8_t sample)
{
    fakeBackend.firstByte = sample;
}

void SDLBackend_ServiceTimers(void)
{
}

/* The host's settings (src/SDL/settings.cpp): the install's MEDIA.CFG choices
 * the trace copy has, VGA and the speaker. */
const RevengeMediaChoices *HostSettings_MediaChoices(void)
{
    static const RevengeMediaChoices speaker = { { 0x0000, 0x0006, 0x0008, 0x0001 }, "", MEDIA_MUSIC_SPEAKER, MEDIA_SOUND_SPEAKER };

    return &speaker;
}
