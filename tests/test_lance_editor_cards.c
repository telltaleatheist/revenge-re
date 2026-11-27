#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "REVENGE.h"
#include "revenge/original_data.h"
#include "revenge/original/lance_editor_data.h"
#include "revenge/platform_backend.h"
#include "revenge/save_data.h"
#include "revenge/video.h"
#include "Units/REVENGE_2507_LANCE_EDITOR_INTERNAL.h"
#include "original_import_for_tests.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* The selection cards of a save the original wrote (REVENGE_ORIGINAL_SAVE,
 * slot 1, kept outside the repo) through the lance editor's card
 * preparation. A load leaves each card's sprite pointer as the original wrote
 * it, a far pointer into the original's memory, never one of ours; the editor
 * sets the pointer of every card it will draw before drawing it.
 *
 *   (no argument)  after the editor's preparation, every card it draws points
 *                  at the editor's buffer for its unit or pilot
 *   stale          a card drawn as loaded is a fault (ctest matches it)
 *
 * The editor's graphics buffers are built first, all of them: entries 01h-10h
 * for the units, 17h-25h for the pilots' portraits (2507:5016). */
static byte framebuffer[VIDEO_SCREEN_WIDTH * VIDEO_SCREEN_HEIGHT];
static byte encodingBuffer[0x2000];

static void LoadOriginalSaveCards(void)
{
	const char *path = getenv("REVENGE_ORIGINAL_SAVE");
	static byte file[REVENGE_SAVE_FILE_SIZE];
	word16 slotStart;
	RevengeCampaignSnapshot snapshot;
	FILE *save;
	size_t index;

	if (path == NULL || path[0] == '\0')
	{
		printf("skipped: set REVENGE_ORIGINAL_SAVE to a save the original wrote\n");
		exit(77);
	}
	save = fopen(path, "rb");
	assert(save != NULL);
	assert(fread(file, 1, sizeof(file), save) == sizeof(file));
	fclose(save);
	/* The directory's word for slot 1 is where its campaign snapshot starts. */
	slotStart = (word16) (file[0x04] | (file[0x05] << 0x08));
	memcpy(&snapshot, &file[slotStart], sizeof(snapshot));
	for (index = 0; index < REVENGE_SAVE_SCENARIO_RECORD_COUNT; ++index)
		UnitSelectionCard_Load(&UnitSelectionCards[index], &snapshot.scenarioRecords[index]);
}

static void BuildEditorBuffers(void)
{
	memset(framebuffer, 0x00, sizeof(framebuffer));
	HostFarPointer_Register(framebuffer, sizeof(framebuffer));
	HostFarPointer_Register(encodingBuffer, sizeof(encodingBuffer));
	PrimaryScreenBuffer = framebuffer;
	PlanarScreenBuffer = encodingBuffer;
	VideoAdapterType = 0x03; /* VGA */
	LoadGraphicsBufferRange(0x0001, 0x0008);
	LoadGraphicsBufferRange(0x0009, 0x0010);
	LoadGraphicsBufferRange(0x0017, 0x001E);
	LoadGraphicsBufferRange(0x001F, 0x0025);
}

int main(int argc, char **argv)
{
	word16 slot;

	RequireOriginalImport();
	SDLBackend_InitializeDOSMemoryArena();
	BuildEditorBuffers();
	LoadOriginalSaveCards();

	if (argc > 1 && strcmp(argv[1], "stale") == 0)
	{
		(void) LanceEditorCardSprite(&UnitSelectionCards[0x00]);
		return 0;
	}

	/* A card as loaded saves the far pointer it was loaded with. */
	{
		RevengeSavedSelectionCard saved;

		UnitSelectionCard_Store(&saved, &UnitSelectionCards[0x00]);
		assert(saved.spriteBuffer.segment == 0x7929 && saved.spriteBuffer.offset == 0x0008);
	}

	/* The units: the first lance's six cards, as the campaign's start has them. */
	PrepareLanceSelectionRecords(0x0000, 0x0000, FALSE);
	/* A built card saves its sprite's far address in the DOS memory block. */
	{
		RevengeSavedSelectionCard saved;
		uint32 capacity;
		word16 segment;
		const byte *block = SDLBackend_DOSMemoryBlock(&capacity, &segment);
		uint32 linear;

		UnitSelectionCard_Store(&saved, &UnitSelectionCards[0x00]);
		linear = ((uint32) segment << 0x04) +
			(uint32) (UnitSelectionCards[0x00].spriteBuffer - block);
		assert(saved.spriteBuffer.segment == (word16) (linear >> 0x04) &&
			saved.spriteBuffer.offset == (word16) (linear & 0x0F));
	}
	for (slot = 0x0000; slot < UnitEditorScenarioCardCount; ++slot)
	{
		const UnitSelectionCard *card = &UnitSelectionCards[slot];
		word16 unitType = PersistentUnitRoster[card->persistentUnitSlot].unitTypeId;

		assert(LanceEditorCardSprite(card) ==
			UnitEditorGraphicsBuffer(UnitEditorUnitSpriteIndex[unitType]));
	}

	/* The pilots: the same cards as pilot cards, one pilot each, their
	 * pointers still the original's. */
	LoadOriginalSaveCards();
	UnitEditorPilotCardCount = UnitEditorPilotCardCountByFamily[0x00];
	for (slot = 0x0000; slot < UnitEditorPilotCardCount; ++slot)
		UnitSelectionCards[slot].selectionId = (RevengePilotId) slot;
	PreparePilotSelectionRecords(0x0000, 0x0000, FALSE);
	for (slot = 0x0000; slot < UnitEditorPilotCardCount; ++slot)
	{
		const UnitSelectionCard *card = &UnitSelectionCards[slot];

		assert(LanceEditorCardSprite(card) ==
			UnitEditorGraphicsBuffer(PilotPortraitIndexById[card->selectionId]));
	}
	return 0;
}
