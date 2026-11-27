#include "REVENGE.h"
#include "fake_backend.h"
#include "revenge/original_data.h"

#include <assert.h>

int main(void)
{
	FakeBackend_Reset();
	InstallGameClockTimer();
	assert(fakeBackend.gameClockHandler == GameClockInt1CHandler);

	TacticalUpdateDelayTicks = 0x0002;
	TacticalKeyRepeatDelayTicks = 0x0001;
	GameTimerTicks = 0x0000FFFFUL;
	GameAnimationTicks = 0x0001FFFFUL;
	fakeBackend.gameClockHandler();
	assert(TacticalUpdateDelayTicks == 0x0001);
	assert(TacticalKeyRepeatDelayTicks == 0x0000);
	assert(GameTimerTicks == 0x00010000UL);
	assert(GameAnimationTicks == 0x00020000UL);
	fakeBackend.gameClockHandler();
	assert(TacticalUpdateDelayTicks == 0x0000);
	assert(TacticalKeyRepeatDelayTicks == 0x0000);

	RestoreGameClockTimer();
	assert(fakeBackend.gameClockHandler == NULL);
	return 0x00;
}
