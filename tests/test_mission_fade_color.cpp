// The title text's fade colours (0D73:2F5F). The fade-out starts one past the
// four colours, as the original does, and that step reads the byte after them
// in the data segment: the low byte of MissionBlinkRectX[0], as it stands then.
// Synthetic values only.

#include "revenge/original/campaign_data.h"
#include "../src/Original/REVENGE.h"

#include <stdio.h>
#include <stdlib.h>

uint8_t MissionFadeColor(uint16_t fadeIndex);

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
    uint16_t fadeIndex;

    for (fadeIndex = 0x00; fadeIndex < 0x04; ++fadeIndex)
        MissionFadeColorTable[fadeIndex] = (uint8_t) (0x11 * (fadeIndex + 0x01));
    MissionBlinkRectX[0x00] = 0x1234;

    for (fadeIndex = 0x00; fadeIndex < 0x04; ++fadeIndex)
        Check(MissionFadeColor(fadeIndex) == MissionFadeColorTable[fadeIndex],
            "steps 0-3 are the four fade colours");
    Check(MissionFadeColor(0x04) == 0x34,
        "step 4 is the byte after them: MissionBlinkRectX[0]'s low byte");
    MissionBlinkRectX[0x00] = 0x0007;
    Check(MissionFadeColor(0x04) == 0x07,
        "step 4 reads MissionBlinkRectX[0] as it stands at the fade");
    if (failures == 0)
        printf("mission fade colour: ok\n");
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
