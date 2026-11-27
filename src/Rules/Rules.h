// The rules (include/revenge/rules.h): numbers we own, the original's to
// begin with, read from <data dir>/rules/*.json at start into the tables the
// game reads. Never the game's text: a rule that names a text uses its story
// id, and a name in a file is the name the code gives the thing.
//
//   weapons.json
//     {"weapon_table": {"small_laser": {...}, ..., "srm_6": {...}},
//      "ammunition_capacity_multipliers": [14 numbers]}
//     The weapons are keyed by RevengeWeaponIndex (revenge/combat_data.h),
//     each once. A weapon:
//       {"heat", "damage_points", "minimum_range", "maximum_range_units",
//        "weight_half_tons", "missile_rack_size",
//        "ammunition" (RevengeAmmunitionFamily's name, or null for none),
//        "critical_slots", "recovery_ticks", "effect", "combat_rating"}
//     and, for the missile weapons (lrm_5 on), "improved_missile_damage_bonus".
//     The lrm_5 bonus is one byte with HitLocationDirectionSelectorTable's
//     last in the original, so the two must agree.
//     A weapon's record also points to its name, weapon_name.N in the story:
//     that pointer is the reference edition's address of the text, not a rule.
//     The capacity multipliers are in the original's order: the code indexes
//     them by a scene's ammunition index and by a unit record's field index.
//
// Every value is a whole number in its field's range; arrays keep the
// original's length. A file that breaks any of this is refused, naming the
// file and the key.
#pragma once

#include <filesystem>
#include <stdexcept>

namespace revenge::rules {

struct RulesError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

// Reads every rules file under data_dir/rules into its tables, or throws
// RulesError naming the file and the key.
void load(const std::filesystem::path& data_dir);

}  // namespace revenge::rules
