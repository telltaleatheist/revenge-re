#ifndef REVENGE_SDL_HARNESS_H
#define REVENGE_SDL_HARNESS_H

#include "revenge/types.h"

#include <stdint.h>
#include <time.h>

/*
 * The port's test harness (tools/trace/README.md). It is off unless
 * REVENGE_HARNESS_SCRIPT names a key script and REVENGE_HARNESS_OUT an
 * existing folder. When it is on:
 *
 * - every timer reads a test clock instead of SDL's: it starts at zero and
 *   moves only when the host would wait (a delay or a retrace wait moves it by
 *   the wait) or passes through a timer service or retrace poll (one step,
 *   REVENGE_HARNESS_STEP_US microseconds, 1000 unless set). Nothing sleeps.
 *   The date and time the game asks DOS for are 1991-01-01 12:00:00 plus the
 *   clock.
 * - keys come from the script, each fed through the game's own keyboard
 *   handler at the game's own keyboard polls: a
 *   key is pending on the 4th poll since the last item was taken (more after
 *   idle:N, not before steps:N battle steps since then, and not before
 *   still:N polls without a step), so the run depends on what the game does,
 *   never on the host's speed. A battle step is one move of the battle's
 *   clock subtick: steps:N lands a key at the same point of a battle on any
 *   machine, as polls don't, and still:N waits for the battle to stop for a
 *   message or menu. The mouse is where the script or the game last put it (0,0
 *   and no buttons at first). No SDL event reaches the game.
 * - @snap:NAME writes, at the next wait, NAME.indices (the 320x200 indexed
 *   frame), NAME.palette (the RGB colours it can show: 16 in the 16-colour
 *   mode, else 256), NAME.save (the block a save
 *   would write now) and NAME.state (the named state, one fact a line) to the
 *   output folder, and appends NAME to snaps.txt there.
 * - when the script is done, the next wait writes done.txt and ends the run.
 */

int Harness_Active(void);

/* The clock every host timer reads, in nanoseconds; SDL's when the harness is
 * off. */
uint64_t HostClock_NowNS(void);
uint64_t HostClock_NowMS(void);
/* Waits: sleeps when the harness is off; otherwise moves the test clock. */
void HostClock_DelayNS(uint64_t nanoseconds);
/* A host pass that waits on time without sleeping (a timer service, a retrace
 * poll): one step of the test clock, nothing when the harness is off. */
void HostClock_Step(void);
/* The calendar time the game is told: local time when the harness is off. */
int HostClock_CalendarTime(struct tm *out, uint32_t *hundredths);

/* One keyboard poll: 1 when the script's next key is due now. */
int Harness_KeyPending(void);
/* The due key: SDL's scancode and the modifiers (1 Shift, 2 Ctrl, 4 Alt) the
 * script gives it; polls until one is due. The platform sends it to the
 * game's keyboard handler as REVSNAP does (src/SDL/platform.c). */
void Harness_ReadKey(int *scancode, byte *modifiers);
/* A script's key name (Shift+, Ctrl+, Alt+ allowed): 1 when it names a key
 * the keyboard has. */
int Harness_ParseKey(const char *token, int *scancode, byte *modifiers);
void Harness_Mouse(uint16 *x, uint16 *y, byte *buttons);
/* The game placing the mouse (int 33h AX=4): it stays there until the script
 * or the game moves it, as a mouse nobody touches does. */
void Harness_PlaceMouse(int16 x, int16 y);

/* From the platform layer (platform.c), for the harness. */
/* The set-1 scan code the keyboard gives SDL's scancode, without E0h (as
 * tools/capture/keys.py has it); zero for a key with none. */
uint16 Platform_ScanCodeForKey(int scancode);
/* The colours the screen can show (16 in the 16-colour mode, else 256) as RGB
 * into out; returns how many. */
uint16 Platform_PaletteRgb(byte out[0x300]);

#endif
