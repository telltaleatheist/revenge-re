#include "fake_backend.h"
#include "Data/AudioData.h"
#include "Data/MouseCursorData.h"
#include "Data/TacticalControlData.h"
#include "Data/TacticalStatusData.h"
#include "Data/TacticalUnitData.h"
#include "Tactical/Tactical.h"

#include <assert.h>
#include <string.h>

/* 2007:0E19 (src/Original/Tactical/TacticalStatus.cpp). */
void DrawSelectedUnitStatusPanel(void);

/* The narration countdown (PendingNarrationDelay) is battle time in ms by
 * the game clock, not draws of the status panel (Tactical/Tactical.h,
 * NARRATION_DELAY_MS): the same on a fast machine as on the reference one. */

static void Setup(void)
{
    FakeBackend_Reset();
    MouseInputDisabled = 0x0001;         /* no cursor to hide and restore */
    TacticalUiSuppressed = FALSE;
    TacticalSelectedUnitIndex = 0xFFFF;  /* no unit: the panel's blank line */
    ActiveNarrationSubject = 0x0014;     /* an enemy: status row 7 */
    NarrationMessageCategory = 0x0000;
    FriendlyUnitDestroyed = 0x0001;      /* no narration sound */
    memset(TacticalStatusRowInvalidation, 0x00, sizeof(TacticalStatusRowInvalidation));
    GameTimerTicks = 0x00000100UL;
    DrawSelectedUnitStatusPanel();        /* the clock as the battle left it */
    PendingNarrationDelay = NARRATION_DELAY_MS;
}

/* Draws without the clock moving take nothing: on the original's frame count
 * 605 of them ended it. */
static void CheckDrawsAlone(void)
{
    int draw;

    Setup();
    for (draw = 0; draw < 2000; ++draw)
        DrawSelectedUnitStatusPanel();
    assert(PendingNarrationDelay == NARRATION_DELAY_MS);
}

/* Each tick of battle time takes 55 ms, however many draws share it; it ends
 * on the 364th tick (20,000 ms) and marks the subject's row for redraw. */
static void CheckTicks(void)
{
    int tick;

    Setup();
    for (tick = 1; tick <= 363; ++tick)
    {
        ++GameTimerTicks;
        DrawSelectedUnitStatusPanel();
        DrawSelectedUnitStatusPanel();
        DrawSelectedUnitStatusPanel();
        assert(PendingNarrationDelay == NARRATION_DELAY_MS - tick * GAME_CLOCK_TICK_MS);
    }
    assert(TacticalStatusRowInvalidation[0x07] == 0x00);
    ++GameTimerTicks;
    DrawSelectedUnitStatusPanel();
    assert(PendingNarrationDelay == 0x0000);
    assert(TacticalStatusRowInvalidation[0x07] == 0xFF);
}

/* A wait outside the battle loop (a message, a menu) draws nothing, so it
 * counts one tick at most, as the original's frame count lost one draw. */
static void CheckWait(void)
{
    Setup();
    GameTimerTicks += 0x0200;
    DrawSelectedUnitStatusPanel();
    assert(PendingNarrationDelay == NARRATION_DELAY_MS - GAME_CLOCK_TICK_MS);
}

int main(void)
{
    CheckDrawsAlone();
    CheckTicks();
    CheckWait();
    return 0;
}
