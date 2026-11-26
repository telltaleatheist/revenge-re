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
 * - keys come from the script, each fed at the game's own keyboard polls: a
 *   key is pending on the 4th poll since the last item was taken (more after
 *   idle:N), so the run depends on what the game does, never on the host's
 *   speed. The mouse is where the script last put it (0,0 and no buttons at
 *   first). No SDL event reaches the game.
 * - @snap:NAME writes, at the next wait, NAME.indices (the 320x200 indexed
 *   frame), NAME.palette (its 256 RGB colours), NAME.save (the block a save
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
/* The due key, as SDLBackend_ReadRawKeypress returns keys; polls until one is
 * due. */
uint16 Harness_ReadKey(void);
void Harness_Mouse(uint16 *x, uint16 *y, byte *buttons);

/* From the platform layer (platform.c), for the harness. */
uint16 Platform_RawKeypressForScancode(int scancode, byte modifierFlags);
void Platform_PaletteRgb(byte out[0x300]);

#endif
