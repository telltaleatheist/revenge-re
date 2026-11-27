// The scene interpreter's plain jump (opcode F7) read from the start of a
// battle's scene. A battle starts the cursor at the instruction block (1A1F
// scenario runtime); the jump reads its operand there, then moves the cursor
// to the target, counted from the instruction base. Synthetic scene bytes
// only.

#include "revenge/original/scene_data.h"
#include "revenge/platform_backend.h"
#include "../src/Original/REVENGE.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void ExecuteSceneInstructions(void);

enum
{
	INSTRUCTION_BLOCK_OFFSET = 0x0126,
	INSTRUCTION_BASE = INSTRUCTION_BLOCK_OFFSET + 0x0002,
	JUMP_TARGET = 0x0010
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
	byte *block;

	memset(&LoadedSceneFile, 0x00, sizeof(LoadedSceneFile));
	LoadedSceneFile.header.instructionBlockOffset = INSTRUCTION_BLOCK_OFFSET;
	SceneInstructionBaseOffset = INSTRUCTION_BASE;
	block = LoadedSceneFile.raw + INSTRUCTION_BASE;
	block[0x00] = SCENE_JUMP;
	block[0x01] = (byte) JUMP_TARGET;
	block[0x02] = (byte) (JUMP_TARGET >> 0x08);
	block[JUMP_TARGET] = SCENE_YIELD;

	// As the battle's start sets it: the instruction block.
	SceneInstructionCursor = block;

	ExecuteSceneInstructions();

	Check(SceneInstructionCursor == block + JUMP_TARGET + 0x01,
		"the jump lands on its target, the yield after it read");
	if (failures == 0)
		printf("scene jump: ok\n");
	return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
