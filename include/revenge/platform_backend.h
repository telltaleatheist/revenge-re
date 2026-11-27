#ifndef REVENGE_PLATFORM_BACKEND_H
#define REVENGE_PLATFORM_BACKEND_H

#include "revenge/dos.h"
#include "revenge/input.h"

/*
 * Portable declarations for the DOS, BIOS, and direct-hardware boundary.
 * Implementations may use SDL3, but SDL types and game-level services do not
 * cross this interface into the preserved Original source.
 */
uint16 SDLBackend_DirectConsoleIO(byte character);

void SDLBackend_GetSystemDate(DOS_Date *date);
void SDLBackend_GetSystemTime(DOS_Time *time);
const char *SDLBackend_GetEnvironmentVariable(const char *name);
_Noreturn void SDLBackend_TerminateProcess(byte exitCode);

int SDLBackend_Open(void);
void SDLBackend_Close(void);
/* The SDL renderer drawing the screen, NULL before SDLBackend_Open. */
const char *SDLBackend_RendererName(void);
int SDLBackend_PollQuit(void);
uint64_t SDLBackend_Milliseconds(void);
void SDLBackend_Delay(uint32_t milliseconds);

byte SDLBackend_GetVideoMode(void);
void SDLBackend_SetVideoMode(byte videoMode);
void SDLBackend_LoadVgaPalette(const byte *paletteRgbTriplets);
void SDLBackend_WaitVerticalRetrace(byte waitForRetraceStart);
byte *SDLBackend_GetIndexedFramebuffer(void);
void SDLBackend_ClearIndexedFramebuffer(byte color);
void SDLBackend_DrawIndexedPixel(uint16 x, uint16 y, byte color);
int SDLBackend_PresentIndexedFramebuffer(void);

/* The keyboard and the mouse as their interrupts brought them: the host's
 * events since the last pass go to KeyboardInterruptHandler (3037:0510) a
 * byte at a time once InstallKeyboardHandlers has run, and timers are
 * serviced. PollInput doesn't wait; WaitInput first waits briefly for the
 * host's next event. The harness's keys arrive here instead (harness.h). */
void SDLBackend_PollInput(void);
void SDLBackend_WaitInput(void);
/* The BIOS's keyboard byte at 0040:offset (17h, 18h, 96h): shift, lock and
 * prefix flags as SDL has them; zero under the harness. */
byte SDLBackend_ReadBiosKeyboardFlags(word16 offset);
/* From InstallKeyboardHandlers until RestoreKeyboardHandlers, key events go
 * to the game's handler. */
void SDLBackend_InstallKeyboardHandlers(void);
void SDLBackend_RestoreKeyboardHandlers(void);
void SDLBackend_SetMouseVisible(uint16 visible);
void SDLBackend_SetMousePosition(int16 logicalX, int16 logicalY);
/* INT 33h AX=0Ch: with a nonzero event mask, SDL's mouse events go to
 * MouseEventCallback (0800:3772) at the input passes; zero stops them. */
void SDLBackend_SetMouseEventHandler(word16 eventMask);

int SDLBackend_OpenAudio(void);
void SDLBackend_CloseAudio(void);
/* Renders the sound due by the host clock (src/Audio) into the stream. */
void SDLBackend_AdvanceAudio(void);
byte SDLBackend_EnablePCSpeaker(void);
void SDLBackend_SilencePCSpeaker(void);
void SDLBackend_WritePCSpeakerPITDivisor(uint16 divisor);
/* Speech when the sound is the speaker, played clean: unsigned 8-bit samples
 * at their rate. The game waits until they end, its timers held as long, as
 * the original's speaker speech ran with interrupts off (373D:0022). */
void SDLBackend_PlaySpeech(const byte *samples, word16 sampleCount, word16 sampleRateHz);
void SDLBackend_WriteMIDIByte(byte midiByte);
/*
 * The game's clock. What its timer interrupts did, the host calls between the
 * game's own calls: at the waits and polls, never inside a procedure, so
 * there is no interrupt flag and no vector table.
 * - The game's INT 1Ch handler (InstallGameClockTimer until
 *   RestoreGameClockTimer) at the BIOS tick's 1193182 / 10000h Hz.
 * - The music timer's services (InstallMusicTimer until RestoreSystemTimer)
 *   at the rates its PIT divisor gave them (30B2:0010).
 * - The digital sample tick (InstallDigitalPlaybackTimer until
 *   RestoreDigitalPlaybackTimer) at 1193182 / divisor Hz.
 */
void SDLBackend_SetGameClockHandler(void (*handler)(void));
void SDLBackend_StartMusicClock(word16 pitDivisor);
void SDLBackend_StopMusicClock(void);
void SDLBackend_StartDigitalPlaybackClock(word16 pitDivisor);
void SDLBackend_StopDigitalPlaybackClock(void);
/* Runs everything the game's clock has due by now. */
void SDLBackend_ServiceTimers(void);
/* Every timer's next tick put off by `nanoseconds`: what fell due in that
 * time comes that much later. */
void SDLBackend_HoldTimers(uint64_t nanoseconds);
void SDLBackend_SetDigitalOutputRate(word16 sampleRateHz);
void SDLBackend_WriteUnsigned8Sample(byte sample);

#endif
