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
 * - the game's clock reads a test clock instead of SDL's, in whole
 *   milliseconds: it starts at zero and moves only when the host would wait
 *   (a delay or a retrace wait moves it by the wait) or passes through a timer
 *   service or a wait for the script's next key (one millisecond). Nothing
 *   sleeps.
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
 *   message or menu. A mouse:X/Y/BUTTONS item calls the game's mouse
 *   handler when it is due, as REVSNAP does. No SDL event reaches the game.
 * - @snap:NAME writes, at the next wait, NAME.indices (the 320x200 indexed
 *   frame), NAME.palette (the RGB colours it can show: 16 in the 16-colour
 *   mode, else 256), NAME.save (the block a save
 *   would write now) and NAME.state (the named state, one fact a line) to the
 *   output folder, and appends NAME to snaps.txt there.
 * - destroy:N, after a steps:N wait, ends battle unit N (Harness_DestroyUnit).
 * - when the script is done, the next wait writes done.txt and ends the run.
 */

int Harness_Active(void);

/* The game's clock, in milliseconds: SDL's when the harness is off. Every
 * timer runs from it (src/SDL/platform.cpp). */
uint64_t HostClock_NowMS(void);
/* Waits: sleeps when the harness is off; otherwise moves the test clock. */
void HostClock_DelayMS(uint64_t milliseconds);
/* A host pass that waits on time without sleeping (a timer service, a wait
 * for the next key): one millisecond of the test clock, nothing when the
 * harness is off. */
void HostClock_Step(void);
/* The calendar time the game is told: local time when the harness is off. */
int HostClock_CalendarTime(struct tm *out, uint32_t *hundredths);

/* A script's destroy:N item: unit N ended by the game's own path
 * (DestroyScenarioUnit, 2D1B:2453), as the hit that ends it would. It makes a
 * test save at a point no script plays to (tools/trace/README.md); a player
 * can't reach it. 1 when done; 0, nothing changed, unless the harness is on,
 * the battle has stepped since the script's previous item and has no result,
 * and the unit is deployed. */
int Harness_DestroyUnit(word16 unitIndex);

/* One keyboard poll: 1 when the script's next key is due now. */
int Harness_KeyPending(void);
/* The due key: SDL's scancode and the modifiers (1 Shift, 2 Ctrl, 4 Alt) the
 * script gives it; polls until one is due. The platform sends it to the
 * game's keyboard handler as REVSNAP does (src/SDL/platform.cpp). */
void Harness_ReadKey(int *scancode, byte *modifiers);
/* A script's key name (Shift+, Ctrl+, Alt+ allowed): 1 when it names a key
 * the keyboard has. */
int Harness_ParseKey(const char *token, int *scancode, byte *modifiers);

/* From the platform layer (platform.cpp), for the harness. */
/* A script's mouse item, to the game's mouse handler when it has one. */
void Platform_SendHarnessMouse(uint16 x, uint16 y, byte buttons);
/* A script's quit item: the host's quit, as the window's close button asks it. */
void Platform_RequestHarnessQuit(void);
/* A script's fail item: fail:allocation, the heap's next request failing,
 * or fail:abort, a fatal path the game has, forced (platform.cpp,
 * FailIfAsked). */
enum
{
	HARNESS_FAIL_NONE,
	HARNESS_FAIL_ALLOCATION,
	HARNESS_FAIL_ABORT
};
void Platform_RequestHarnessFailure(int failure);
/* The set-1 scan code the keyboard gives SDL's scancode, without E0h (as
 * tools/capture/keys.py has it); zero for a key with none. */
uint16 Platform_ScanCodeForKey(int scancode);
/* The colours the screen can show (16 in the 16-colour mode, else 256) as RGB
 * into out; returns how many. */
uint16 Platform_PaletteRgb(byte out[0x300]);
/* A row of the text screen (src/SDL/platform.cpp), 80 characters, no NUL. */
const char *Platform_TextRow(uint16 row);

#endif
