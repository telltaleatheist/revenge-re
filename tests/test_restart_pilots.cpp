#include "fake_backend.h"
#include "rules_for_tests.h"
#include "Campaign/CampaignInternal.h"
#include "Data/CampaignData.h"
#include "Data/RosterData.h"
#include "Units/LanceEditorInternal.h"
#include "revenge/upgrades.h"

#include <assert.h>
#include <string.h>

/* The UPGRADES switch upgrade_restart_pilots (Campaign/CampaignMenus.cpp,
 * ApplyRestartPilotsUpgrade, at 0D73:274D RunInGameMenu's Restart). A
 * campaign played a while has drawn most of its replay pool; Restart then
 * copies the initial state over the replay state as the original does
 * (0D73:29AD-29D1), and the gate's action runs or not. Each mode is its own
 * run.
 *
 *   original  the initial copy alone: 15 pilots in the pool, every one 0,
 *             so every pilot drawn is 0 (DISCOVERIES.md)
 *   fixed     the switch's action after it: the pool and count a new
 *             campaign starts with, so the draws are of those pilots, each
 *             once, and the initial copy is left as it was
 *   kept      the pool kept at load is the data's, whatever the campaign
 *             did to the live one after
 */

/* The campaign's replay pool after play, then the original's Restart copy. */
static void PlayThenRestart(void)
{
    uint16_t index;

    /* Two pilots left in the live pool. */
    memset(ReplayExtendedCampaignState, 0x00, sizeof(ReplayExtendedCampaignState));
    ReplayExtendedCampaignState[0] = 40;
    ReplayExtendedCampaignState[1] = 41;
    RandomPilotPoolCounts.replay = 0x02;
    /* Restart: the initial copy over the replay copy. */
    RandomPilotPoolCounts.replay = RandomPilotPoolCounts.initial;
    for (index = 0x0000; index < CAMPAIGN_EXTENDED_STATE_COUNT; ++index)
        ReplayExtendedCampaignState[index] = InitialExtendedCampaignState[index];
}

int main(int argc, char **argv)
{
    const char *mode = argc > 1 ? argv[1] : "fixed";
    uint8_t seen[0x40];
    uint8_t initial[CAMPAIGN_EXTENDED_STATE_COUNT];
    int draw;

    FakeBackend_Reset();
    RequireRules();
    memcpy(initial, InitialExtendedCampaignState, sizeof(initial));
    memset(seen, 0x00, sizeof(seen));
    if (strcmp(mode, "original") == 0)
    {
        PlayThenRestart();
        assert(RandomPilotPoolCounts.replay == 15);
        for (draw = 0; draw < 15; ++draw)
            assert(PopRandomPilotId() == 0);
    }
    else if (strcmp(mode, "fixed") == 0)
    {
        PlayThenRestart();
        ApplyRestartPilotsUpgrade();
        assert(RandomPilotPoolCounts.replay == 15);
        for (draw = 0; draw < 15; ++draw)
        {
            const int16_t pilot = PopRandomPilotId();

            assert(pilot >= 18 && pilot <= 32 && seen[pilot] == 0);
            seen[pilot] = 1;
        }
        assert(RandomPilotPoolCounts.replay == 0);
        assert(memcmp(initial, InitialExtendedCampaignState, sizeof(initial)) == 0);
    }
    else if (strcmp(mode, "kept") == 0)
    {
        uint8_t pool[15];
        uint8_t count = 0;

        memset(ReplayExtendedCampaignState, 0x00, sizeof(ReplayExtendedCampaignState));
        RandomPilotPoolCounts.replay = 0x00;
        Rules_StartingReplayPilots(pool, &count);
        assert(count == 15);
        for (draw = 0; draw < 15; ++draw)
            assert(pool[draw] == 18 + draw);
    }
    else
        return 2;
    return 0;
}
