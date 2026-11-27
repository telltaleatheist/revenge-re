#include "revenge/combat_data.h"
#include "Data/TacticalData.h"
#include "revenge/unit.h"

#include "rules_for_tests.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

void ApplyWeaponDamage(
    int16_t attackerUnitIndex, int16_t weaponIndex, int16_t targetUnitIndex);
static uint16_t SumArmor(const RevengeUnitRecord *unitRecord)
{
    uint16_t total = 0x00;
    uint16_t locationIndex;

    for (locationIndex = 0x00;
        locationIndex < BATTLEMECH_ARMOR_LOCATION_COUNT;
        ++locationIndex)
    {
        total += unitRecord->currentArmor[locationIndex];
    }
    return total;
}

static void PrepareCombatants(void)
{
    uint16_t locationIndex;
    RevengeUnitRecord *attacker = &PersistentUnitRoster[0x00];
    RevengeUnitRecord *target =
        &PersistentUnitRoster[ENEMY_FORCE_FIRST_UNIT];

    memset(PersistentUnitRoster, 0x00,
        sizeof(PersistentUnitRoster));
    attacker->deploymentState = 0x01;
    attacker->walkMovement = 0x04;
    target->deploymentState = 0x01;
    target->walkMovement = 0x04;
    for (locationIndex = 0x00;
        locationIndex < BATTLEMECH_LOCATION_COUNT;
        ++locationIndex)
    {
        target->currentInternal[locationIndex] = 0x14;
        target->maximumInternal[locationIndex] = 0x14;
    }
    for (locationIndex = 0x00;
        locationIndex < BATTLEMECH_ARMOR_LOCATION_COUNT;
        ++locationIndex)
    {
        target->currentArmor[locationIndex] = 0x14;
        target->maximumArmor[locationIndex] = 0x14;
    }
    ScenarioUnitMapX[0x00] = 0x02;
    ScenarioUnitMapY[0x00] = 0x02;
    ScenarioUnitMapX[ENEMY_FORCE_FIRST_UNIT] = 0x03;
    ScenarioUnitMapY[ENEMY_FORCE_FIRST_UNIT] = 0x02;
    ScenarioUnitFacing[ENEMY_FORCE_FIRST_UNIT] = 0x00;
}

static void VerifyHitLocationAndArmorDamage(void)
{
    RevengeUnitRecord *target;
    uint16_t armorBefore;
    uint16_t internalBefore;
    uint16_t locationIndex;

    PrepareCombatants();
    target = &PersistentUnitRoster[ENEMY_FORCE_FIRST_UNIT];
    armorBefore = SumArmor(target);
    internalBefore = 0x00;
    for (locationIndex = 0x00;
        locationIndex < BATTLEMECH_LOCATION_COUNT;
        ++locationIndex)
    {
        internalBefore += target->currentInternal[locationIndex];
    }

    /* The initial deterministic 2D6 roll selects a valid directional hit
	 * location. Twenty points of armor prevents penetration, so the machine
	 * gun's two damage must remain entirely in that selected armor location. */
    ApplyWeaponDamage(
        0x00, WEAPON_MACHINE_GUN, ENEMY_FORCE_FIRST_UNIT);
    assert(SumArmor(target) ==
        armorBefore - WeaponDefinitions[WEAPON_MACHINE_GUN].damage);
    for (locationIndex = 0x00;
        locationIndex < BATTLEMECH_LOCATION_COUNT;
        ++locationIndex)
    {
        internalBefore -= target->currentInternal[locationIndex];
    }
    assert(internalBefore == 0x00);
}

/* With Improved Missiles each missile weapon adds its bonus to its damage:
 * the bonuses the original reads from DS:3D0D at weapon indexes 10h-16h. */
static void VerifyImprovedMissileBonus(void)
{
    static const uint8_t expectedBonus[0x07] = { 0x00, 0x02, 0x03, 0x04, 0x05, 0x01, 0x02 };
    RevengeUnitRecord *target;
    uint16_t armorBefore;
    uint16_t locationIndex;
    int16_t weaponIndex;

    for (weaponIndex = WEAPON_LRM_5; weaponIndex <= WEAPON_SRM_6; ++weaponIndex)
    {
        PrepareCombatants();
        target = &PersistentUnitRoster[ENEMY_FORCE_FIRST_UNIT];
        for (locationIndex = 0x00;
            locationIndex < BATTLEMECH_ARMOR_LOCATION_COUNT;
            ++locationIndex)
        {
            target->currentArmor[locationIndex] = 0x30;
            target->maximumArmor[locationIndex] = 0x30;
        }
        PersistentUnitRoster[0x00].missileUpgradeState = 0x01;
        armorBefore = SumArmor(target);
        ApplyWeaponDamage(0x00, weaponIndex, ENEMY_FORCE_FIRST_UNIT);
        assert(SumArmor(target) == armorBefore - WeaponDefinitions[weaponIndex].damage - expectedBonus[weaponIndex - WEAPON_LRM_5]);
    }
}

int main(void)
{
    RequireRules();
    VerifyHitLocationAndArmorDamage();
    VerifyImprovedMissileBonus();
    return EXIT_SUCCESS;
}
