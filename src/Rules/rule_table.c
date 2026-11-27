/* Written by tools/import/make_import_table.py from src/Rules/rule_pieces.txt
 * and a copy of the reference edition: where each rule's table is in the
 * original's memory, its length and the keyed hash of its bytes there.
 * Don't edit by hand. */
#include "revenge/rules.h"
#include "revenge/combat_data.h"
#include "revenge/original_data.h"

const RulePiece RulePieces[] =
{
	{ "weapons.definitions", &WeaponDefinitions, sizeof(WeaponDefinitions), 0x3A6A, 0x0435, 0x159, "9df29601c1a9b0ee" },
	{ "weapons.ammunition_capacity_multipliers", &AmmoCapacityMultiplierTable, sizeof(AmmoCapacityMultiplierTable), 0x3A6A, 0x058E, 0x1C, "d11d781441c3a5ae" },
	{ "weapons.improved_missile_damage_bonus", &ImprovedMissileDamageBonusTable, sizeof(ImprovedMissileDamageBonusTable), 0x3A6A, 0x3D1D, 0x7, "7f7c4eecf4db2dcb" },
	{ "weapons.combat_ratings", &WeaponCombatRatingByType, sizeof(WeaponCombatRatingByType), 0x4184, 0x08BC, 0x17, "f887554e762155fb" },
	{ "combat.heat_dissipation", &HeatDissipationBySinkCountQ8_8, sizeof(HeatDissipationBySinkCountQ8_8), 0x3A6A, 0x176A, 0x60, "b8c0e4bf9fb494e2" },
	{ "combat.hit_location_tables", &HitLocationTable, sizeof(HitLocationTable), 0x3A6A, 0x1038, 0x78, "ce85518345173f6e" },
	{ "combat.hit_table_by_direction", &HitLocationDirectionSelectorTable, sizeof(HitLocationDirectionSelectorTable), 0x3A6A, 0x3CFE, 0x20, "3d08ae8cf2af30c2" },
	{ "combat.sensor_damage_proximity_range", &SensorDamageProximityRangeTable, sizeof(SensorDamageProximityRangeTable), 0x3A6A, 0x03E7, 0x3, "a5111cb17f4f1961" },
	{ "combat.action_mask_by_experience", &UnitActionMaskByExperience, sizeof(UnitActionMaskByExperience), 0x3A6A, 0x3CF6, 0x8, "148a7e5565fe6600" },
	{ "combat.movement_timing_masks", &MovementTimingMaskTable, sizeof(MovementTimingMaskTable), 0x3A6A, 0x1026, 0x12, "5397e3a5c20ad67d" },
	{ "combat.movement_rating_by_walk_mp", &MovementRatingByWalkMP, sizeof(MovementRatingByWalkMP), 0x3A6A, 0x0097, 0x9, "3f96755d2e19a279" },
};
const size_t RulePieceCount = sizeof(RulePieces) / sizeof(RulePieces[0]);

_Static_assert(sizeof(WeaponDefinitions) == 0x159, "weapons.definitions is the original's size");
_Static_assert(sizeof(AmmoCapacityMultiplierTable) == 0x1C, "weapons.ammunition_capacity_multipliers is the original's size");
_Static_assert(sizeof(ImprovedMissileDamageBonusTable) == 0x7, "weapons.improved_missile_damage_bonus is the original's size");
_Static_assert(sizeof(WeaponCombatRatingByType) == 0x17, "weapons.combat_ratings is the original's size");
_Static_assert(sizeof(HeatDissipationBySinkCountQ8_8) == 0x60, "combat.heat_dissipation is the original's size");
_Static_assert(sizeof(HitLocationTable) == 0x78, "combat.hit_location_tables is the original's size");
_Static_assert(sizeof(HitLocationDirectionSelectorTable) == 0x20, "combat.hit_table_by_direction is the original's size");
_Static_assert(sizeof(SensorDamageProximityRangeTable) == 0x3, "combat.sensor_damage_proximity_range is the original's size");
_Static_assert(sizeof(UnitActionMaskByExperience) == 0x8, "combat.action_mask_by_experience is the original's size");
_Static_assert(sizeof(MovementTimingMaskTable) == 0x12, "combat.movement_timing_masks is the original's size");
_Static_assert(sizeof(MovementRatingByWalkMP) == 0x9, "combat.movement_rating_by_walk_mp is the original's size");
