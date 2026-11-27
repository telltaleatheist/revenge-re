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
//     A weapon's record also points to its name, weapon_name.N in the story:
//     that pointer is the reference edition's address of the text, not a rule.
//     The capacity multipliers are in the original's order: the code indexes
//     them by a scene's ammunition index and by a unit record's field index.
//     The lrm_5 bonus is one byte with the high byte of combat.json's
//     hit_table_by_direction[15] (a table number), so it is 0.
//
//   combat.json, arrays in the original's order and length:
//     "heat_dissipation_by_sink_count"  48 words, the heat each count of heat
//         sinks takes off a unit's heat accumulator, in 1/256ths
//     "hit_location_tables"  5 tables of 11 armour locations (RevengeBattle-
//         MechArmorLocation's names), one for each 2d6 roll from 2 to 12; the
//         fifth is the infantry table
//     "hit_table_by_direction"  16 table numbers (0 to 3), by the attack's
//         direction against the target's facing
//     "sensor_damage_proximity_range"  3 distances, by a unit's sensor hits:
//         an enemy nearer than this engages the unit
//     "action_mask_by_experience"  4 masks, by pilot experience, of the
//         tactical clock's ticks on which a unit may act
//     "movement_timing_masks"  9 masks, by movement rate, of the ticks on
//         which a unit moves
//     "movement_rating_by_walk_mp"  9, the movement class each walking MP
//         shows as
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
