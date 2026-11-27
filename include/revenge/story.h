#ifndef REVENGE_STORY_H
#define REVENGE_STORY_H

/*
 * The story: every text the game shows, taken from the player's REVENGE.EXE
 * by the importer (src/Import) and held in StoryTextBuffer, each at its own
 * place (revenge/story_text.h). The game names a single text with
 * STORY_TEXT(ID); a table of texts, as TitleTextTable, is filled with its
 * entries' places once every text is in.
 */

#include "revenge/story_text.h"

#include <stddef.h>
#include <stdint.h>


/* What a text fills besides its own place. */
typedef enum StoryTarget
{
    STORY_TARGET_NONE,          /* read in place, by STORY_TEXT */
    STORY_TARGET_TEXT,          /* an entry of a const char * table (or one such pointer) */
    STORY_TARGET_MUTABLE_TEXT,  /* an entry of a char * table: the game writes in its text */
    STORY_TARGET_ARRAY          /* a char array the text is copied into, its rest zeroed */
} StoryTarget;

typedef struct StoryTextSlot
{
    const char *id;           /* as the story file names it: "title.3" */
    uint32_t at;              /* its place in StoryTextBuffer */
    uint16_t capacity;        /* the text's length and its NUL, the copy's */
    uint8_t target;           /* a StoryTarget */
    void *destination;        /* the entry, pointer or array it fills; NULL for none */
    size_t destinationSize;   /* an array's size */
} StoryTextSlot;

extern const StoryTextSlot StoryTextSlots[STORY_TEXT_COUNT];

/* The id's index, or -1 when no text has it. */
int StoryText_Find(const char *id);

/* Puts a text of `length` bytes in its place, NUL-padded. A text longer than
 * the place, or one with a NUL in it, is a fault naming the id. */
void StoryText_Place(StoryTextId id, const char *text, size_t length);

/* Once every text is placed: the tables and arrays get theirs. */
void StoryText_FillTargets(void);

/* Puts every text and stored piece in place (src/Import/StoryHost.cpp):
 * from the story file in writeDirectory (our write folder) when it is current
 * for the edition its import.json names, without opening the copy at all;
 * otherwise from executablePath, which OriginalImport_Load must accept, the
 * story file being written first. With no write folder (NULL), from the copy
 * OriginalImport_Load already accepted. Returns 0 when the copy is needed and
 * isn't accepted (OriginalImport_Load has said why), 1 otherwise. A text or
 * piece that isn't the edition table's by its length or keyed hash, and a
 * story file that can't be read, stop the game with a fault naming what. */
int StoryHost_Prepare(const char *executablePath, const char *writeDirectory);


#endif
