// The rules (include/revenge/rules.h): numbers we own, the original's to
// begin with, read from <data dir>/rules/*.json and <data dir>/ui/*.json at
// start into the tables the game reads. Never the game's text: a rule that names a text uses its story
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
//   ai.json:
//     "heat_limit_by_experience"  5, the heat above which a computer unit
//         holds its fire, by pilot experience plus one (one less overburning)
//     "engagement_by_comparison_mask"  256 TACTICAL_ENGAGEMENT_ modes (none,
//         close, medium, long, withdraw, position), by the mask comparing a
//         unit's combat ratings with its target's
//     "weapon_priorities"  16 lists of {"if_ready": [weapons], "fire":
//         weapon}: a unit fires the first entry any of whose weapons is
//         ready. The lists fill the bank one after another (176 entries in
//         all), and the counts and pointers to them are rebuilt from them.
//         In every entry but five the weapon fired is the one that must be
//         ready. In the fifteenth list (number 14) the five weapons fired
//         are shifted by one against their readiness: each entry fires the
//         weapon the next one waits for, and the last fires the first's.
//         The original plays so, and the file keeps it.
//     "priority_list_by_range_heat"  128 list numbers (0 to 15)
//     "formation_offsets"  4 facing bands of 10 formations, each
//         {"x": [3], "y": [3]}, the three followers' offsets from the leader
//     "formation_choices"  {"aggressive", "defensive", "balanced"}: 12
//         formations each (0 to 9), by pilot experience times three plus a
//         random 0 to 2
//
//   units.json, unit types as numbers (0 to 58h, the code names none but
//   REVENGE_UNIT_TYPE_NONE, which is null):
//     "predefined_lances"  5 rows of 6 unit types, by roster plus lance
//     "random_unit_count_by_roster"  10, how many of a roster's pool to draw
//         from (0 to 7)
//     "random_unit_pools"  7 pools of 7 unit types, rosters 3 on
//     "fixed_unit_types"  2
//     "lance_allegiances_by_scenario"  7 scenarios of 3 allegiances, one for
//         each four-unit group
//     "unit_class_types"  the training's 7 lists of unit types by class (or
//         null); laid out one after another, each ended by FFh, in 44h bytes
//
//   pilots.json, pilots by roster number (0 to 58):
//     "pilots"  59 {"identity" (the pilot's name and portrait number),
//         "experience" (0 to 3), "gunnery"}: the roster's starting records.
//         Each pilot's availability, which the campaign changes, starts at 0.
//     "default_pilots_by_scenario"  5 rows of 4 pilots, by scenario row and
//         lance slot
//     "fixed_pilots"  2
//
//   campaign.json:
//     "scenario_ordinal_base_by_stage"  22, the scenario number each
//         campaign stage starts from (plus its variant)
//     "mission_dates"  12 {"month" (0 to 11), "year", "advance_months"}: the
//         date each mission screen shows and how many months it counts on
//     "scenario_unit_cards_by_family", "pilot_cards_by_family"  7 each, how
//         many unit and pilot cards the lance editor deals (0 to 18, the
//         cards there are)
//
// data/ui holds the layout, ours as the rules are, read the same way:
//
//   mission.json:
//     "fade_colors"  4 palette colours the mission texts fade through
//     "blink_rects"  7 {"x", "y", "extent"}: the squares the mission map
//         blinks, in pixels
//     "sprites"  12 {"tile_x", "pixel_y", "width_tiles", "height_pixels"}:
//         where each mission sprite is cut from its picture
//
//   lance_editor.json:
//     "layout"  LanceEditorLayoutData, a key for each field (its name in the
//         code, lower case): positions and sizes in pixels or tiles, palette
//         colours, rows. The fields named Left/Top/Width/Height (or
//         SourceTileX/SourcePixelY/WidthTiles/HeightPixels) together are
//         one array of records, as "unit_editor_sections": [{"left", "top",
//         "width", "height"}, ...]
//     "unit_sprite_by_type"  89, the graphics buffer (0 to 37) each unit
//         type's card shows
//     "pilot_portrait_by_pilot"  33, the graphics buffer each pilot's card
//         shows
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

// Reads every file under data_dir/rules and data_dir/ui into its tables, or throws
// RulesError naming the file and the key.
void load(const std::filesystem::path& data_dir);

}  // namespace revenge::rules
