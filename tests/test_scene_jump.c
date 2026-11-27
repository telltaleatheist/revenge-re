// The scene interpreter's plain jump (opcode F7) read from the start of a
// battle's scene. A battle starts the cursor at the instruction block through
// the buffer's own far address (1A1F scenario runtime), whose segment is not
// DS; the jump must read its operand there, then move the cursor to DS.
// Synthetic scene bytes only.

#include "revenge/far_memory.h"
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
	JUMP_TARGET = 0x0010,
	SCENE_BUFFER_OFFSET = 0x71A8
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
	uint32 start;

	SDLBackend_InitializeDOSMemoryArena();
	memset(&LoadedSceneFile, 0x00, sizeof(LoadedSceneFile));
	LoadedSceneFile.header.instructionBlockOffset = INSTRUCTION_BLOCK_OFFSET;
	SceneInstructionBaseOffset = INSTRUCTION_BASE;
	block = LoadedSceneFile.raw + INSTRUCTION_BASE;
	block[0x00] = SCENE_JUMP;
	block[0x01] = (byte) JUMP_TARGET;
	block[0x02] = (byte) (JUMP_TARGET >> 0x08);
	block[JUMP_TARGET] = SCENE_YIELD;

	// As the battle's start sets it: the block's own far address.
	start = HostFarPointer_Encode(block);
	SceneInstructionCursor.segment = HIGH_WORD(start);
	SceneInstructionCursor.offset = LOW_WORD(start);
	Check(SceneInstructionCursor.segment != SCENE_DATA_SEGMENT,
		"the battle's start names the block through a segment other than DS");

	ExecuteSceneInstructions();

	Check(SceneInstructionCursor.segment == SCENE_DATA_SEGMENT,
		"the jump moves the cursor to DS");
	Check(SceneInstructionCursor.offset ==
			SCENE_BUFFER_OFFSET + INSTRUCTION_BASE + JUMP_TARGET + 0x01,
		"the jump lands on its target, the yield after it read");
	if (failures == 0)
		printf("scene jump: ok\n");
	return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
