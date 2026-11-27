// The harness's destroy:N item (src/SDL/harness.h) is for making test saves
// only: with the harness off, as in play, Harness_DestroyUnit refuses and the
// unit stays as it was.

#include "harness.h"

#include "revenge/original_data.h"

#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>

enum
{
	UNIT = 0x14
};

static int failures;

static void Check(int condition, const char *what)
{
	if (!condition)
	{
		fprintf(stderr, "FAIL: %s\n", what);
		++failures;
	}
}

int main(void)
{
	RevengeUnitRecord *unit = &PersistentUnitRoster[UNIT];

	SDL_unsetenv_unsafe("REVENGE_HARNESS_SCRIPT");
	SDL_unsetenv_unsafe("REVENGE_HARNESS_OUT");
	unit->deploymentState = 0x01;
	unit->walkMovement = 0x04;
	TacticalBattleResult = 0x0000;

	Check(Harness_Active() == 0, "the harness is off without its script");
	Check(Harness_DestroyUnit(UNIT) == 0, "destroy is refused with the harness off");
	Check(unit->deploymentState == 0x01, "the unit is still deployed");
	Check(unit->walkMovement == 0x04, "the unit can still move");
	if (failures == 0)
		printf("harness destroy: refused outside the harness\n");
	return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
