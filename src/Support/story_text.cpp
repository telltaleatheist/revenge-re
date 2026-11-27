#include "revenge/story.h"

#include "revenge/debug.h"

#include <string.h>

int StoryText_Find(const char *id)
{
    int index;

    for (index = 0; index < STORY_TEXT_COUNT; ++index)
        if (strcmp(StoryTextSlots[index].id, id) == 0)
            return index;
    return -1;
}

void StoryText_Place(StoryTextId id, const char *text, size_t length)
{
    const StoryTextSlot *slot = &StoryTextSlots[REVENGE_INDEX(StoryTextSlots, id)];

    if (memchr(text, '\0', length) != NULL)
        RevengeFault("the story text %s holds a NUL", slot->id);
    if (length + 1 > slot->capacity)
        RevengeFault("the story text %s is %llu bytes, and its place holds %u", slot->id,
            (unsigned long long) length, (unsigned) slot->capacity - 1u);
    memset(&StoryTextBuffer[slot->at], 0, slot->capacity);
    memcpy(&StoryTextBuffer[slot->at], text, length);
}

void StoryText_FillTargets(void)
{
    int index;

    for (index = 0; index < STORY_TEXT_COUNT; ++index)
    {
        const StoryTextSlot *slot = &StoryTextSlots[index];
        char *text = &StoryTextBuffer[slot->at];

        switch (slot->target)
        {
        case STORY_TARGET_TEXT:
            *(const char **) slot->destination = text;
            break;
        case STORY_TARGET_MUTABLE_TEXT:
            *(char **) slot->destination = text;
            break;
        case STORY_TARGET_ARRAY:
        {
            size_t length = strlen(text);

            if (length + 1 > slot->destinationSize)
                RevengeFault("the story text %s doesn't fit its array", slot->id);
            memset(slot->destination, 0, slot->destinationSize);
            memcpy(slot->destination, text, length);
            break;
        }
        default:
            break;
        }
    }
}
