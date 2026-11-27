#ifndef REVENGE_PLATFORM_BACKEND_H
#define REVENGE_PLATFORM_BACKEND_H

#include "revenge/dos.h"
#include "revenge/input.h"
#include "revenge/interrupts.h"

/*
 * Portable declarations for the DOS, BIOS, and direct-hardware boundary.
 * Implementations may use SDL3, but SDL types and game-level services do not
 * cross this interface into the preserved Original source.
 */
uint16 SDLBackend_DirectConsoleIO(byte character);
void SDLBackend_SelectDefaultDrive(byte zeroBasedDrive);

int16 SDLBackend_CreateFile(uint16 attributes, const char *filePath);
int16 SDLBackend_OpenFile(const char *filePath, uint16 openFlags);
int16 SDLBackend_CloseFile(int16 fileHandle);
int16 SDLBackend_ReadFile(
	uint16 fileHandle, void *destinationBuffer, uint16 byteCount);
int16 SDLBackend_WriteFile(
	uint16 fileHandle, const void *sourceBuffer, uint16 byteCount);
long SDLBackend_SeekFile(uint16 fileHandle, long fileOffset, byte origin);
int16 SDLBackend_DeleteFile(const char *filePath);
int16 SDLBackend_GetSetFileAttributes(
	const char *filePath, byte operation, uint16 attributes);
int16 SDLBackend_DeviceIOControl(
	uint16 fileHandle, uint16 operation, void *data, uint16 count);

word16 SDLBackend_ResizeMemoryBlock(
	word16 memorySegment, word16 paragraphCount);
word16 SDLBackend_GetLargestFreeConventionalMemoryParagraphs(
	word16 requestedParagraphs);
void SDLBackend_InitializeDOSMemoryArena(void);
void SDLBackend_GetSystemDate(DOS_Date *date);
void SDLBackend_GetSystemTime(DOS_Time *time);
const char *SDLBackend_GetEnvironmentVariable(const char *name);
void SDLBackend_InvokeInterrupt(
	byte interruptNumber,
	const InterruptRegisterBlock *inputRegisters,
	InterruptRegisterBlock *outputRegisters,
	InterruptSegmentState *segmentState);
void SDLBackend_SetInterruptVector(
	byte interruptNumber, DOS_InterruptHandler handler);
DOS_InterruptHandler SDLBackend_SwapInterruptHandler(
	byte interruptNumber, DOS_InterruptHandler handler);
uint32 SDLBackend_SwapInterruptVector(
	byte interruptNumber, uint32 newVector);
void SDLBackend_ChainInterruptVector(uint32 interruptVector);
void SDLBackend_DispatchInstalledInterruptVector(byte interruptNumber);
void SDLBackend_ProgramPITChannel0(word16 divisor);
void SDLBackend_ProgramPITChannel0Mode(byte mode, word16 divisor);
void SDLBackend_AcknowledgeInterrupt(byte interruptNumber);
uint32 SDLBackend_GetBIOSTickCount(void);
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
uint16 SDLBackend_IsVerticalRetraceActive(void);
void SDLBackend_WaitVerticalRetrace(byte waitForRetraceStart);
byte *SDLBackend_GetIndexedFramebuffer(void);
void SDLBackend_ClearIndexedFramebuffer(byte color);
void SDLBackend_DrawIndexedPixel(uint16 x, uint16 y, byte color);
int SDLBackend_PresentIndexedFramebuffer(void);

word16 SDLBackend_InputPending(void);
uint16 SDLBackend_ReadRawKeypress(void);
void SDLBackend_InstallKeyboardHandlers(void);
void SDLBackend_RestoreKeyboardHandlers(void);
void SDLBackend_SetMouseVisible(uint16 visible);
void SDLBackend_SetMousePosition(int16 logicalX, int16 logicalY);

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
 * Deliver every PIT/BIOS interrupt whose host-time deadline has elapsed.
 * DOS hardware interrupted otherwise empty busy-waits asynchronously; the
 * single-threaded hosted build calls this pump cooperatively at those waits
 * and at existing platform boundaries.
 */
void SDLBackend_ServiceTimers(void);
/* Every timer's next tick put off by `nanoseconds`: what fell due in that
 * time comes that much later. */
void SDLBackend_HoldTimers(uint64_t nanoseconds);
void SDLBackend_DispatchInterrupt(byte interruptNumber);
void SDLBackend_SetDigitalOutputRate(word16 sampleRateHz);
void SDLBackend_WriteUnsigned8Sample(byte sample);

#endif
