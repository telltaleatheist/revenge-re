// The rules (include/revenge/rules.h): numbers we own, the original's to
// begin with, read from <data dir>/rules/*.json, <data dir>/ui/*.json and
// <data dir>/start/*.json at start into the tables the game reads. Never the
// game's text: a rule that names a text uses its story id, and a name in a
// file is the name the code gives the thing.
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
//     "selection_cards"  18 {"left", "top", "width"}: where each unit or
//         pilot card is drawn (UnitSelectionCard)
//     "lance_slots"  13 {"left", "top", "width"}: where each lance slot is
//         drawn (RevengeUnitLanceAssignment); the thirteenth is the editor's
//         exit button
//
//   screens.json:
//     "palette_file_by_screen"  37, the resource file (0 to 89) whose palette
//         each screen loads, by screen number less one
//     "text_window_frame"  {"corners": {"top_left", "top_right",
//         "bottom_left", "bottom_right"}, "edges": {"top", "left", "right",
//         "bottom"}}: the tiles a text window's frame is drawn from, each
//         edge a run of tiles repeated along it; 12 tiles and ends in all
//     "campaign_message_window_heights"  178, the height in tiles (0 to
//         127) of each campaign message's window, by message number; the
//         reader writes each message's text pointer (campaign_message.N)
//
//   tactical_panel.json, the unit status glyph, lines as [x0, y0, x1, y1] in
//   pixels from its origin:
//     "facing_lines"  8 facings of 2 lines, the facing arrow
//     "movement_lines"  2 bands (UnitMovementHeatExponent less one) of 8
//         facings, the line that shows the unit moving
//     "formation_marker_origins"  10 {"x", "y"}, by formation
//
//   windows.json:
//     "text_windows"  23 {"left_tile", "top_tile", "width_tiles",
//         "height_tiles", "foreground_color", "background_color"}
//         (TextWindowContext); each window's cursor starts at 0, 0
//     "ui_regions"  16 {"left_column", "top_row", "width_columns",
//         "row_count", "unknown000a", "xor_color", "unknown000e"}
//         (OriginalUiRegionRecord); each region's selected row starts at 0
//     "navigation_keys"  {"up", "down", "previous_region", "next_region"}:
//         the key codes the menus move by
//
// data/start holds the state a new campaign begins with, ours too:
//
//   new_campaign.json:
//     "initial", "forward", "replay"  the three copies of the campaign's
//         random pools, as the original starts them: each
//         {"random_unit_pool" (7 unit types or null),
//          "random_pilot_pool" (15 pilots, 0 to 58, or null),
//          "random_pilot_count" (0 to 15)}.
//         The game draws from the replay copy. A new campaign round copies
//         replay to forward (or forward back to replay, to play a stage
//         again), and Restart copies initial to replay. The initial copy's
//         pilot pool is 15 zeros while the replay copy's is pilots 18 to 32,
//         as the original's are; the forward copy is overwritten from the
//         replay copy before it is read.
//
//   lance_editor.json, the lance editor's starting state:
//     "selection_card_states"  18, each card's state
//     "lance_slot_styles"  13, each lance slot's style
//         (a card's and a slot's units, pilots and ids start empty, FFh, and
//         a card's sprite null; the reader writes them)
//     "lance_slot_counts"  3 words, the slots of each lance
//     "section_enabled"  29, 0 or 1: 7 families of 4 sections, then one
//         byte past them, as the original has it
//     "reload_graphics"  0 or 1
//
//   game.json, words:
//     "tactical_game_speed"  0 to 7, the battle speed's index (keys 1 to 8)
//     "tactical_update_delay_ticks"  the delay that speed's key sets
//         (SetTacticalGameSpeed's table: 35 for speed 2)
//     "presentation_dismiss_timeout"  how long a story screen waits for a
//         key before it goes on
//     "tactical_status_blink_countdown", "displayed_tactical_status_row"
//         the battle status panel's (65535: no row shown)
//
//   save_directory.json, the save directory before a save file is read:
//     "prefix"  4 bytes, as the original has them
//     "slots"  6 {"occupied" (0 or 1), "name" (up to 20 printable ASCII
//         characters)}: the slots the save and load menus list, named 1 to
//         6 until a save file names them
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

// Reads every file under data_dir/rules, /ui and /start into its tables, or
// throws RulesError naming the file and the key.
void load(const std::filesystem::path& data_dir);

}  // namespace revenge::rules
