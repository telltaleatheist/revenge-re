// The scene's deploy (opcode E9) with roster slot FF, "no pilot". The original
// (1A1F:339F) compares the slot byte, widened to a word, with FFFF, which it
// never is, so it always assigns a roster entry (30F1:0AFC): slot FF is the
// entry past the roster's end, the four bytes at DS:2148. The unit gets pilot
// FF and that entry's experience and gunnery. Synthetic values only.

#include "revenge/original_data.h"
#include "revenge/representation.h"
#include "revenge/original/scene_data.h"
#include "../src/Original/REVENGE.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void ExecuteSceneInstructions(void);
void InitializeResourceCache(void);

enum
{
	INSTRUCTION_BLOCK_OFFSET = 0x0126,
	INSTRUCTION_BASE = INSTRUCTION_BLOCK_OFFSET + 0x0002,
	UNIT = 0x10,
	UNIT_TYPE = 0x00,
	TEMPLATE_GUNNERY = 0x04,
	PAST_END_EXPERIENCE = 0x23,
	PAST_END_GUNNERY = 0x37
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
	static const byte deploy[] = { SCENE_DEPLOY_UNIT, UNIT, UNIT_TYPE, 0x04, 0x00, 0xFF, SCENE_YIELD };
	byte *block;
	const RevengeUnitRecord *unit = &PersistentUnitRoster[UNIT];

	OriginalData_InitializeResourceDescriptorPointers();
	InitializeResourceCache();

	/* The test backend's reads leave the buffer as it is: the template the
	 * deploy reads is this record. */
	memset(&EstimatedUnitRecord, 0x00, sizeof(EstimatedUnitRecord));
	EstimatedUnitRecord.gunneryTargetNumber = TEMPLATE_GUNNERY;
	PilotRosterRecordPastEnd.experienceIndex = PAST_END_EXPERIENCE;
	PilotRosterRecordPastEnd.gunneryIndex = PAST_END_GUNNERY;

	memset(&LoadedSceneFile, 0x00, sizeof(LoadedSceneFile));
	LoadedSceneFile.header.instructionBlockOffset = INSTRUCTION_BLOCK_OFFSET;
	SceneInstructionBaseOffset = INSTRUCTION_BASE;
	block = LoadedSceneFile.raw + INSTRUCTION_BASE;
	memcpy(block, deploy, sizeof(deploy));
	SceneInstructionCursor = block;

	ExecuteSceneInstructions();

	Check(unit->deploymentState == 0x01, "the unit is deployed");
	Check((byte) unit->pilotId == 0xFF, "slot FF is assigned: the unit's pilot is FF");
	Check(unit->pilotExperience == PAST_END_EXPERIENCE,
		"its experience is the entry past the roster's end");
	Check(unit->gunneryTargetNumber == PAST_END_GUNNERY,
		"its gunnery is the entry past the roster's end, not the template's");
	if (failures == 0)
		printf("deploy pilot: ok\n");
	return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
