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
 * sets the pointer of every card it will draw before drawing it. A card saves
 * the pointer it was loaded with, set by the editor or not.
 *
 *   (no argument)  after the editor's preparation, every card it draws points
 *                  at the editor's buffer for its unit or pilot
 *   stale          a card drawn as loaded is a fault (ctest matches it)
 *   unset          the image's start cards hold no unit slot (FFh) until the
 *                  campaign's first selection sets them (mode 1, stage 0);
 *                  prepared as they stand, the roster index FFFFh is a fault
 *                  (ctest matches it), not a read past the roster
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
    BuildEditorBuffers();
    LoadOriginalSaveCards();

    if (argc > 1 && strcmp(argv[1], "unset") == 0)
    {
        UnitSelectionCards_LoadStart();
        PrepareLanceSelectionRecords(0x0000, 0x0000, FALSE);
        return 0;
    }

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
    /* A card the editor has set still saves the pointer it was loaded with. */
    {
        RevengeSavedSelectionCard saved;

        assert(UnitSelectionCards[0x00].spriteBuffer != NULL);
        UnitSelectionCard_Store(&saved, &UnitSelectionCards[0x00]);
        assert(saved.spriteBuffer.segment == 0x7929 && saved.spriteBuffer.offset == 0x0008);
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

    /* A new campaign's cards, from the image's start cards, given their unit
	 * slots as the campaign's first selection gives them (stage 0, mode 1,
	 * which also loads each unit's template from the game's files), then set
	 * by the editor: each saves 0000:0000, as the image holds them. */
    UnitSelectionCards_LoadStart();
    for (slot = 0x0000; slot < 0x0006; ++slot)
    {
        UnitSelectionCards[slot].persistentUnitSlot = (RevengeUnitSlot) slot;
        UnitSelectionCards[slot].assignmentSlot = REVENGE_ASSIGNMENT_SLOT_NONE;
    }
    PrepareLanceSelectionRecords(0x0000, 0x0000, FALSE);
    for (slot = 0x0000; slot < UNIT_SELECTION_CARD_COUNT; ++slot)
    {
        RevengeSavedSelectionCard saved;

        UnitSelectionCard_Store(&saved, &UnitSelectionCards[slot]);
        assert(saved.spriteBuffer.segment == 0x0000 && saved.spriteBuffer.offset == 0x0000);
    }
    assert(UnitSelectionCards[0x00].spriteBuffer != NULL);
    return 0;
}
