#include "fake_backend.h"
#include "Data/TacticalUnitData.h"
#include "Tactical/Tactical.h"

#include <assert.h>
#include <string.h>

/* 2D1B:0379 totals a unit's armor (or, for infantry, 0Bh bytes from its
 * internal structure) with mov al,...; cbw; add: each byte is signed. */
int main(void)
{
    RevengeUnitRecord unit;

    memset(&unit, 0, sizeof(unit));
    unit.deploymentState = 0x01;
    unit.currentArmor[0x00] = 0x80;
    unit.currentArmor[0x0A] = 0x05;
    unit.maximumArmor[0x00] = 0xFF;
    unit.maximumArmor[0x01] = 0x10;
    CalculateUnitDurabilityTotals(&unit);
    assert(CurrentUnitDurabilityTotal == -0x80 + 0x05);
    assert(MaximumUnitDurabilityTotal == -0x01 + 0x10);

    memset(&unit, 0, sizeof(unit));
    unit.deploymentState = 0x01;
    unit.damageModel = UNIT_DAMAGE_MODEL_INFANTRY;
    unit.currentInternal[0x00] = 0x90;
    unit.currentArmor[0x02] = 0x03;    /* the eleventh byte from +04h */
    unit.currentArmor[0x03] = 0x40;    /* past it: not counted */
    unit.maximumInternal[0x07] = 0x7F;
    CalculateUnitDurabilityTotals(&unit);
    assert(CurrentUnitDurabilityTotal == -0x70 + 0x03);
    assert(MaximumUnitDurabilityTotal == 0x7F);
    return 0;
}
