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
};
const size_t RulePieceCount = sizeof(RulePieces) / sizeof(RulePieces[0]);

_Static_assert(sizeof(WeaponDefinitions) == 0x159, "weapons.definitions is the original's size");
_Static_assert(sizeof(AmmoCapacityMultiplierTable) == 0x1C, "weapons.ammunition_capacity_multipliers is the original's size");
_Static_assert(sizeof(ImprovedMissileDamageBonusTable) == 0x7, "weapons.improved_missile_damage_bonus is the original's size");
_Static_assert(sizeof(WeaponCombatRatingByType) == 0x17, "weapons.combat_ratings is the original's size");
