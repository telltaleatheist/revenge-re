// A unit's firepower ratings (2D1B:0005) through the weapon-rating sum
// (0800:072C). The sum's loop tests for an empty mask with `or bx,bp`, which
// leaves the mask's high word ORed into its low word on every pass: a weapon in
// the high half (10h-16h) is never counted itself, and its bit reaches the low
// half, which counts weapons the mask did not select. Synthetic ratings and
// counts only.

#include "revenge/combat_data.h"
#include "Data/TacticalData.h"
#include "Data/TacticalGraphicsData.h"
#include "Tactical/Tactical.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint32_t CalculateSelectedWeaponRating(uint32_t weaponMask, const uint16_t *weaponCounts);

constexpr int SLOT = 0x14;
constexpr int NOT_READY = 0x05;

static int failures;

static void Check(int condition, const char *what)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL: %s\n", what);
        ++failures;
    }
}

/* Weapons 00h, 0Eh and 10h, one of each, as the cached counts; those in
 * ready are recovered, the others still recycling. */
static void Rate(uint32_t ready, int16_t closeFirepower, int16_t mediumFirepower,
    int16_t longFirepower, const char *what)
{
    static const uint16_t carried[] = { 0x00, 0x0E, 0x10 };
    RevengeUnitRecord unit;
    const RevengeTacticalCombatRatings *ratings = &UnitCombatRatings[SLOT];
    uint16_t index;
    char line[160];

    memset(&unit, 0x00, sizeof(unit));
    memset(&UnitCachedWeaponCounts[SLOT * WEAPON_DEFINITION_COUNT], 0x00, WEAPON_DEFINITION_COUNT);
    memset(&UnitWeaponRecoveryCounters[SLOT * WEAPON_DEFINITION_COUNT], 0x00, WEAPON_DEFINITION_COUNT);
    for (index = 0x00; index < sizeof(carried) / sizeof(carried[0]); ++index)
    {
        uint16_t weapon = carried[index];
        UnitCachedWeaponCounts[SLOT * WEAPON_DEFINITION_COUNT + weapon] = 0x01;
        if ((ready >> weapon & 0x01) == 0x00)
            UnitWeaponRecoveryCounters[SLOT * WEAPON_DEFINITION_COUNT + weapon] = NOT_READY;
    }

    RecalculateUnitCombatRatings(&unit, SLOT, FALSE, TRUE);

    snprintf(line, sizeof(line), "%s: availability %08lX", what, (unsigned long) UnitWeaponAvailabilityMask[SLOT]);
    Check(UnitWeaponAvailabilityMask[SLOT] == ready, line);
    snprintf(line, sizeof(line), "%s: close %d, medium %d, long %d (want %d, %d, %d)", what,
        ratings->closeFirepower, ratings->mediumFirepower, ratings->longFirepower,
        closeFirepower, mediumFirepower, longFirepower);
    Check(ratings->closeFirepower == closeFirepower && ratings->mediumFirepower == mediumFirepower && ratings->longFirepower == longFirepower, line);
}

int main(void)
{
    uint16_t counts[WEAPON_DEFINITION_COUNT];

    memset(WeaponDefinitions, 0x00, sizeof(WeaponDefinitions));
    memset(WeaponCombatRatingByType, 0x00, sizeof(WeaponCombatRatingByType));
    WeaponCombatRatingByType[0x00] = 0x02;
    WeaponCombatRatingByType[0x01] = 0x03;
    WeaponCombatRatingByType[0x0E] = 0x09;
    WeaponCombatRatingByType[0x0F] = 0x05;
    WeaponCombatRatingByType[0x10] = 0x07;
    WeaponCombatRatingByType[0x11] = 0x04;

    /* Low-half weapons only: the sum is the plain one. */
    Rate(0x00000001UL, 2, 0, 0, "weapon 00h ready");
    Rate(0x00004001UL, 11, 4, 0, "weapons 00h and 0Eh ready");
    /* Weapon 10h is in the medium and long sums alone, rated 7. Its bit
	 * leaks to the low half: weapon 00h (2) and 0Fh (carried none) are
	 * counted in its place, and it is not. */
    Rate(0x00010001UL, 2, 9, 9, "weapons 00h and 10h ready");
    Rate(0x00010000UL, 0, 9, 9, "weapon 10h ready");

    memset(counts, 0x00, sizeof(counts));
    counts[0x01] = 0x01;
    counts[0x10] = 0x02;
    counts[0x11] = 0x03;
    Check(CalculateSelectedWeaponRating(0x00000003UL, counts) == 0x03,
        "a low-half mask sums the weapons it selects");
    Check(CalculateSelectedWeaponRating(0x00020000UL, counts) == 0x0F,
        "weapon 11h's bit counts weapon 01h, then weapon 11h: 3 + 4 * 3");
    if (failures == 0)
        printf("weapon rating: ok\n");
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
