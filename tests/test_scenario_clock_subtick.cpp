#include "Data/TacticalData.h"
#include "Units/Units.h"

#include <assert.h>

/* The campaign clock is three bytes at DS:8337: the hour, the minute and the
 * battle's subtick (DS:8339), the byte the battle loop steps. Leaving a
 * scenario (30F1:0006) saves the three to DS:855F and starting a battle
 * (1A1F:0233) restores them, so the subtick goes out and comes back with the
 * hour and the minute. */
void PrepareScenarioUnitRosters(void);
void AdvanceTacticalCampaignClock(void);

int main(void)
{
    /* No scenario's roster case (31h-54h): only the clock and the map
	 * positions are saved. The clock isn't drawn. */
    ScenarioFileSuffix = 0x0055;
    SuppressCampaignClock = TRUE;

    /* Out: 14:25 and the battle's clock stepped twice (2 subticks each). */
    CampaignClockState[0x00] = 0x0E;
    CampaignClockState[0x01] = 0x19;
    AdvanceTacticalCampaignClock();
    AdvanceTacticalCampaignClock();
    PrepareScenarioUnitRosters();
    assert(SavedCampaignClockState[0x00] == 0x0E);
    assert(SavedCampaignClockState[0x01] == 0x19);
    assert(SavedCampaignClockState[0x02] == 0x04);

    /* Back: the three bytes a battle restores are the ones it steps, so
	 * 14:25 with 58 subticks rolls to 14:26 at the next step. */
    CampaignClockState[0x02] = 0x3A;
    AdvanceTacticalCampaignClock();
    assert(CampaignClockState[0x01] == 0x1A);
    assert(CampaignClockState[0x02] == 0x00);
    return 0;
}
