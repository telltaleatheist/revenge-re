// The rules files' readers (Rules.h has the shapes).
#include "Rules.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <map>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "../Support/Json.h"

#include "revenge/original/lance_editor_data.h"
#include "revenge/original_import.h"
#include "revenge/roster.h"
#include "revenge/rules.h"
#include "revenge/story_text.h"

namespace revenge::rules {

namespace {

using support::Json;

// RevengeWeaponIndex's names, in its order.
constexpr std::array<std::string_view, 0x17> kWeapons = {
    "small_laser", "small_pulse_laser", "medium_laser", "medium_pulse_laser", "large_laser",
    "large_pulse_laser", "er_large_laser", "ppc", "er_ppc", "autocannon_2", "autocannon_5",
    "autocannon_10", "lb_10x_autocannon", "gauss_rifle", "autocannon_20", "machine_gun",
    "lrm_5", "lrm_10", "lrm_15", "lrm_20", "srm_2", "srm_4", "srm_6"};
constexpr std::size_t kFirstMissileWeapon = 0x10;  // WEAPON_LRM_5

// RevengeAmmunitionFamily's names from 1 (0 is none).
constexpr std::array<std::string_view, 0x0F> kAmmunition = {
    "", "autocannon_2", "autocannon_5", "autocannon_10", "lb_10x_autocannon", "gauss_rifle",
    "autocannon_20", "machine_gun", "lrm_5", "lrm_10", "lrm_15", "lrm_20", "srm_2", "srm_4", "srm_6"};

// Each table's bytes, as the original lays it out, by its rule id.
using Tables = std::map<std::string, std::vector<std::uint8_t>, std::less<>>;

constexpr std::size_t kWeaponRecordBytes = 0x0F;  // WeaponDefinitions' record

// RevengeBattleMechArmorLocation's names (revenge/unit.h), in its order: the
// codes the hit-location tables hold.
constexpr std::array<std::string_view, 0x0B> kArmorLocations = {
    "head", "left_arm", "left_torso", "center_torso", "right_torso", "right_arm",
    "left_leg", "right_leg", "left_rear_torso", "center_rear_torso", "right_rear_torso"};
constexpr std::size_t kHitLocationTables = 0x05;  // HIT_LOCATION_TABLE_COUNT
constexpr std::size_t kHitLocationRolls = 0x0B;   // 2d6: 2 to 12
constexpr std::size_t kHitLocationWords = 0x0C;   // the rolls and a zero word
constexpr unsigned kFacingHitTables = 0x04;       // HIT_LOCATION_FACING_TABLE_COUNT

// The TACTICAL_ENGAGEMENT_ modes' names (src/Original/Tactical/REVENGE_2D1B_INTERNAL.h).
constexpr std::array<std::string_view, 0x06> kEngagementModes = {
    "none", "close", "medium", "long", "withdraw", "position"};
constexpr std::size_t kPriorityProfiles = 0x10;    // WEAPON_PRIORITY_PROFILE_COUNT
constexpr std::size_t kPriorityBankBytes = 0x580;  // WEAPON_PRIORITY_DATA_BYTE_COUNT
constexpr unsigned kPriorityBankOffset = 0x09C6;   // the bank's DS offset, which its pointers name
constexpr unsigned kDataSegment = 0x3A6A;          // DS, as the image loaded at 0800h names it
constexpr std::size_t kFacingBands = 0x04;         // FormationOffsetTable: bands of ten modes
constexpr std::size_t kFormationModes = 0x0A;
constexpr unsigned kUnitTypes = 0x59;               // LANCE_EDITOR_UNIT_TYPE_COUNT
constexpr std::uint8_t kNoUnitType = 0xFF;          // REVENGE_UNIT_TYPE_NONE as a byte
constexpr std::size_t kUnitClasses = 0x07;
constexpr std::size_t kUnitClassListBytes = 0x44;   // UnitClassTypeListBytes

// Where each class's list starts in UnitClassTypeListBytes, or -1 for none:
// units.json sets it, and load() points UnitClassTypeLists there.
std::array<long, kUnitClasses> g_class_list_starts{};

// "weapons.json: weapons.ppc.heat is missing".
[[noreturn]] void fail(const std::string& where, const std::string& why) {
    throw RulesError(where + " " + why);
}

// One JSON object's members, taken by name; done() refuses what is left.
class Fields {
public:
    Fields(const Json& value, std::string where) : where_(std::move(where)) {
        if (!value.is_object()) fail(where_, "is not an object");
        object_ = &value.object();
    }
    const std::string& where() const { return where_; }
    // A member's name: after the file's ("weapons.json:"), or the object's.
    std::string at(std::string_view key) const {
        return where_ + (where_.back() == ':' ? " " : ".") + std::string(key);
    }
    const Json& take(std::string_view key) {
        const auto found = object_->find(key);
        if (found == object_->end()) fail(at(key), "is missing");
        taken_.insert(std::string(key));
        return found->second;
    }
    void done() const {
        for (const auto& [key, value] : *object_)
            if (taken_.count(key) == 0) fail(at(key), "is no key of the rules here");
    }

private:
    const Json::Object* object_ = nullptr;
    std::string where_;
    std::set<std::string, std::less<>> taken_;
};

unsigned whole(const Json& value, const std::string& where, unsigned maximum) {
    if (!value.is_number()) fail(where, "is not a number");
    const double number = value.number();
    if (number != std::floor(number) || number < 0 || number > maximum)
        fail(where, "is not a whole number from 0 to " + std::to_string(maximum));
    return static_cast<unsigned>(number);
}

void put_word(std::uint8_t* at, unsigned value) {
    at[0] = static_cast<std::uint8_t>(value & 0xFF);
    at[1] = static_cast<std::uint8_t>(value >> 8);
}

// An array of `count` whole numbers up to `maximum`, laid out as bytes or as
// little-endian words (`width` 1 or 2).
std::vector<std::uint8_t> numbers(const Json& value, const std::string& where, std::size_t count,
                                  unsigned maximum, std::size_t width) {
    if (!value.is_array() || value.array().size() != count)
        fail(where, "is not an array of " + std::to_string(count) + " numbers");
    std::vector<std::uint8_t> out(count * width, 0);
    for (std::size_t index = 0; index < count; ++index) {
        const unsigned number = whole(value.array()[index], where + "[" + std::to_string(index) + "]", maximum);
        if (width == 2)
            put_word(&out[index * 2], number);
        else
            out[index] = std::uint8_t(number);
    }
    return out;
}

// An array of `count` whole numbers from `minimum` to `maximum`, as signed
// bytes.
std::vector<std::uint8_t> signed_bytes(const Json& value, const std::string& where, std::size_t count, int minimum,
                                       int maximum) {
    if (!value.is_array() || value.array().size() != count)
        fail(where, "is not an array of " + std::to_string(count) + " numbers");
    std::vector<std::uint8_t> out(count, 0);
    for (std::size_t index = 0; index < count; ++index) {
        const Json& item = value.array()[index];
        const std::string at = where + "[" + std::to_string(index) + "]";
        if (!item.is_number()) fail(at, "is not a number");
        const double number = item.number();
        if (number != std::floor(number) || number < minimum || number > maximum)
            fail(at, "is not a whole number from " + std::to_string(minimum) + " to " + std::to_string(maximum));
        out[index] = std::uint8_t(int(number) & 0xFF);
    }
    return out;
}

// One of `names`, by its index.
std::size_t named(const Json& value, const std::string& where, const std::string_view* names, std::size_t count,
                  const char* what) {
    if (!value.is_string()) fail(where, std::string("is not ") + what);
    for (std::size_t index = 0; index < count; ++index)
        if (names[index] == value.string()) return index;
    fail(where, std::string("is not ") + what);
}

// The reference edition's place of the story text `id`, as the original's
// far pointer names it.
void put_text_pointer(std::uint8_t* at, int id, const std::string& where) {
    const StoryTextSource* sources = OriginalEditions[0].storyTexts;
    if (sources == nullptr || sources[id].imageOffset == STORY_NO_SOURCE)
        fail(where, "names a text the reference edition doesn't place");
    const StoryTextSource& source = sources[id];
    put_word(at, unsigned(source.imageOffset - std::uint32_t(source.segment - 0x0800) * 16));
    put_word(at + 2, source.segment);
}

void read_weapons(const Json& root, Tables& tables) {
    std::vector<std::uint8_t>& records = tables["weapons.definitions"];
    std::vector<std::uint8_t>& ratings = tables["weapons.combat_ratings"];
    std::vector<std::uint8_t>& bonuses = tables["weapons.improved_missile_damage_bonus"];
    std::vector<std::uint8_t>& multipliers_out = tables["weapons.ammunition_capacity_multipliers"];
    records.assign(kWeapons.size() * kWeaponRecordBytes, 0);
    ratings.assign(kWeapons.size(), 0);
    bonuses.assign(kWeapons.size() - kFirstMissileWeapon, 0);
    multipliers_out.assign(0x0E * 2, 0);
    Fields file(root, "weapons.json:");
    Fields weapons(file.take("weapon_table"), file.at("weapon_table"));
    for (std::size_t index = 0; index < kWeapons.size(); ++index) {
        Fields weapon(weapons.take(kWeapons[index]), weapons.at(kWeapons[index]));
        std::uint8_t* record = &records[index * kWeaponRecordBytes];
        put_text_pointer(record, STORY_ID_WEAPON_NAME_0 + int(index), weapon.where());
        record[0x04] = std::uint8_t(whole(weapon.take("heat"), weapon.at("heat"), 0xFF));
        record[0x05] = std::uint8_t(whole(weapon.take("damage_points"), weapon.at("damage_points"), 0xFF));
        record[0x06] = std::uint8_t(whole(weapon.take("minimum_range"), weapon.at("minimum_range"), 0xFF));
        record[0x07] = std::uint8_t(
            whole(weapon.take("maximum_range_units"), weapon.at("maximum_range_units"), 0xFF));
        put_word(record + 0x08, whole(weapon.take("weight_half_tons"), weapon.at("weight_half_tons"), 0xFFFF));
        record[0x0A] = std::uint8_t(
            whole(weapon.take("missile_rack_size"), weapon.at("missile_rack_size"), 0xFF));
        const Json& ammunition = weapon.take("ammunition");
        std::size_t family = 0;
        if (!std::holds_alternative<std::nullptr_t>(ammunition.value))
            family = 1 + named(ammunition, weapon.at("ammunition"), kAmmunition.data() + 1, kAmmunition.size() - 1,
                               "null or an ammunition the code names");
        record[0x0B] = std::uint8_t(family);
        record[0x0C] = std::uint8_t(whole(weapon.take("critical_slots"), weapon.at("critical_slots"), 0xFF));
        record[0x0D] = std::uint8_t(whole(weapon.take("recovery_ticks"), weapon.at("recovery_ticks"), 0xFF));
        record[0x0E] = std::uint8_t(whole(weapon.take("effect"), weapon.at("effect"), 0xFF));
        ratings[index] = std::uint8_t(whole(weapon.take("combat_rating"), weapon.at("combat_rating"), 0xFF));
        if (index >= kFirstMissileWeapon)
            bonuses[index - kFirstMissileWeapon] = std::uint8_t(whole(
                weapon.take("improved_missile_damage_bonus"), weapon.at("improved_missile_damage_bonus"), 0xFF));
        weapon.done();
    }
    weapons.done();
    multipliers_out = numbers(file.take("ammunition_capacity_multipliers"),
                              file.at("ammunition_capacity_multipliers"), 0x0E, 0xFFFF, 2);
    file.done();
}

void read_combat(const Json& root, Tables& tables) {
    Fields file(root, "combat.json:");
    tables["combat.heat_dissipation"] = numbers(
        file.take("heat_dissipation_by_sink_count"), file.at("heat_dissipation_by_sink_count"), 0x30, 0xFFFF, 2);

    const Json& hit = file.take("hit_location_tables");
    const std::string hit_where = file.at("hit_location_tables");
    if (!hit.is_array() || hit.array().size() != kHitLocationTables)
        fail(hit_where, "is not an array of " + std::to_string(kHitLocationTables) + " tables");
    std::vector<std::uint8_t>& hit_out = tables["combat.hit_location_tables"];
    hit_out.assign(kHitLocationTables * kHitLocationWords * 2, 0);
    for (std::size_t table = 0; table < kHitLocationTables; ++table) {
        const Json& rolls = hit.array()[table];
        const std::string where = hit_where + "[" + std::to_string(table) + "]";
        if (!rolls.is_array() || rolls.array().size() != kHitLocationRolls)
            fail(where, "is not an array of " + std::to_string(kHitLocationRolls) + " locations, rolls 2 to 12");
        for (std::size_t roll = 0; roll < kHitLocationRolls; ++roll)
            put_word(&hit_out[(table * kHitLocationWords + roll) * 2],
                     unsigned(named(rolls.array()[roll], where + "[" + std::to_string(roll) + "]",
                                    kArmorLocations.data(), kArmorLocations.size(),
                                    "an armour location the code names")));
    }

    tables["combat.hit_table_by_direction"] = numbers(
        file.take("hit_table_by_direction"), file.at("hit_table_by_direction"), 0x10, kFacingHitTables - 1, 2);
    tables["combat.sensor_damage_proximity_range"] = numbers(
        file.take("sensor_damage_proximity_range"), file.at("sensor_damage_proximity_range"), 0x03, 0xFF, 1);
    tables["combat.action_mask_by_experience"] = numbers(
        file.take("action_mask_by_experience"), file.at("action_mask_by_experience"), 0x04, 0xFFFF, 2);
    tables["combat.movement_timing_masks"] = numbers(
        file.take("movement_timing_masks"), file.at("movement_timing_masks"), 0x09, 0xFFFF, 2);
    tables["combat.movement_rating_by_walk_mp"] = numbers(
        file.take("movement_rating_by_walk_mp"), file.at("movement_rating_by_walk_mp"), 0x09, 0xFF, 1);
    file.done();
}

// An array of `count` names, each one of `names`, as their indexes in bytes.
std::vector<std::uint8_t> names_of(const Json& value, const std::string& where, std::size_t count,
                                   const std::string_view* names, std::size_t name_count, const char* what) {
    if (!value.is_array() || value.array().size() != count)
        fail(where, "is not an array of " + std::to_string(count) + " names");
    std::vector<std::uint8_t> out(count, 0);
    for (std::size_t index = 0; index < count; ++index)
        out[index] = std::uint8_t(
            named(value.array()[index], where + "[" + std::to_string(index) + "]", names, name_count, what));
    return out;
}

void read_ai(const Json& root, Tables& tables) {
    Fields file(root, "ai.json:");
    tables["ai.heat_limit_by_experience"] = numbers(
        file.take("heat_limit_by_experience"), file.at("heat_limit_by_experience"), 0x05, 0xFF, 1);
    tables["ai.engagement_by_comparison_mask"] =
        names_of(file.take("engagement_by_comparison_mask"), file.at("engagement_by_comparison_mask"), 0x100,
                 kEngagementModes.data(), kEngagementModes.size(), "an engagement mode the code names");

    // The priority lists, one after another in the bank: each list's masks,
    // then its weapons (one-based), as longwords; the counts and the far
    // pointers to each list follow from them.
    const Json& lists = file.take("weapon_priorities");
    const std::string lists_where = file.at("weapon_priorities");
    if (!lists.is_array() || lists.array().size() != kPriorityProfiles)
        fail(lists_where, "is not an array of " + std::to_string(kPriorityProfiles) + " lists");
    std::vector<std::uint8_t>& bank = tables["ai.weapon_priorities"];
    std::vector<std::uint8_t>& counts = tables["ai.weapon_priority_counts"];
    std::vector<std::uint8_t>& pointers = tables["ai.weapon_priority_lists"];
    bank.assign(kPriorityBankBytes, 0);
    counts.assign(kPriorityProfiles * 2, 0);
    pointers.assign(kPriorityProfiles * 4, 0);
    std::size_t used = 0;
    for (std::size_t profile = 0; profile < kPriorityProfiles; ++profile) {
        const Json& list = lists.array()[profile];
        const std::string where = lists_where + "[" + std::to_string(profile) + "]";
        if (!list.is_array()) fail(where, "is not an array of entries");
        const std::size_t count = list.array().size();
        if (used + count * 8 > kPriorityBankBytes)
            fail(where, "doesn't fit: the bank holds " + std::to_string(kPriorityBankBytes / 8) + " entries in all");
        put_word(&counts[profile * 2], unsigned(count));
        put_word(&pointers[profile * 4], unsigned(kPriorityBankOffset + used));
        put_word(&pointers[profile * 4 + 2], kDataSegment);
        for (std::size_t entry = 0; entry < count; ++entry) {
            Fields fields(list.array()[entry], where + "[" + std::to_string(entry) + "]");
            const Json& ready = fields.take("if_ready");
            if (!ready.is_array() || ready.array().empty())
                fail(fields.at("if_ready"), "is not an array of weapons");
            std::uint32_t mask = 0;
            for (std::size_t index = 0; index < ready.array().size(); ++index)
                mask |= std::uint32_t(1) << named(ready.array()[index],
                                                  fields.at("if_ready") + "[" + std::to_string(index) + "]",
                                                  kWeapons.data(), kWeapons.size(), "a weapon the code names");
            const std::size_t fire =
                1 + named(fields.take("fire"), fields.at("fire"), kWeapons.data(), kWeapons.size(),
                          "a weapon the code names");
            fields.done();
            std::uint8_t* at_mask = &bank[used + entry * 4];
            std::uint8_t* at_fire = &bank[used + (count + entry) * 4];
            put_word(at_mask, mask & 0xFFFF);
            put_word(at_mask + 2, mask >> 16);
            put_word(at_fire, unsigned(fire));
        }
        used += count * 8;
    }

    tables["ai.priority_list_by_range_heat"] = numbers(
        file.take("priority_list_by_range_heat"), file.at("priority_list_by_range_heat"), 0x80,
        unsigned(kPriorityProfiles - 1), 1);

    const Json& bands = file.take("formation_offsets");
    const std::string bands_where = file.at("formation_offsets");
    if (!bands.is_array() || bands.array().size() != kFacingBands)
        fail(bands_where, "is not an array of " + std::to_string(kFacingBands) + " facing bands");
    std::vector<std::uint8_t>& offsets = tables["ai.formation_offsets"];
    offsets.clear();
    for (std::size_t band = 0; band < kFacingBands; ++band) {
        const Json& modes = bands.array()[band];
        const std::string where = bands_where + "[" + std::to_string(band) + "]";
        if (!modes.is_array() || modes.array().size() != kFormationModes)
            fail(where, "is not an array of " + std::to_string(kFormationModes) + " formations");
        for (std::size_t mode = 0; mode < kFormationModes; ++mode) {
            Fields formation(modes.array()[mode], where + "[" + std::to_string(mode) + "]");
            for (const char* axis : {"x", "y"}) {
                const std::vector<std::uint8_t> three =
                    signed_bytes(formation.take(axis), formation.at(axis), 0x03, -0x80, 0x7F);
                offsets.insert(offsets.end(), three.begin(), three.end());
            }
            formation.done();
        }
    }

    Fields choices(file.take("formation_choices"), file.at("formation_choices"));
    for (const char* stance : {"aggressive", "defensive", "balanced"})
        tables[std::string("ai.formation_choices_") + stance] =
            numbers(choices.take(stance), choices.at(stance), 0x0C, unsigned(kFormationModes - 1), 1);
    choices.done();
    file.done();
}

// A unit type number (0 to 58h), or null for none.
std::uint8_t unit_type(const Json& value, const std::string& where) {
    if (std::holds_alternative<std::nullptr_t>(value.value)) return kNoUnitType;
    return std::uint8_t(whole(value, where, kUnitTypes - 1));
}

// `rows` arrays of `columns` unit types (or nulls), one after another.
std::vector<std::uint8_t> unit_rows(const Json& value, const std::string& where, std::size_t rows,
                                    std::size_t columns) {
    if (!value.is_array() || value.array().size() != rows)
        fail(where, "is not an array of " + std::to_string(rows) + " rows");
    std::vector<std::uint8_t> out;
    for (std::size_t row = 0; row < rows; ++row) {
        const Json& types = value.array()[row];
        const std::string at = where + "[" + std::to_string(row) + "]";
        if (!types.is_array() || types.array().size() != columns)
            fail(at, "is not an array of " + std::to_string(columns) + " unit types or nulls");
        for (std::size_t column = 0; column < columns; ++column)
            out.push_back(unit_type(types.array()[column], at + "[" + std::to_string(column) + "]"));
    }
    return out;
}

void read_units(const Json& root, Tables& tables) {
    Fields file(root, "units.json:");
    tables["units.predefined_lances"] = unit_rows(file.take("predefined_lances"), file.at("predefined_lances"), 5, 6);
    tables["units.random_unit_count_by_roster"] = numbers(
        file.take("random_unit_count_by_roster"), file.at("random_unit_count_by_roster"), 0x0A, 0x07, 1);
    tables["units.random_unit_pools"] = unit_rows(file.take("random_unit_pools"), file.at("random_unit_pools"), 7, 7);
    tables["units.fixed_unit_types"] = numbers(
        file.take("fixed_unit_types"), file.at("fixed_unit_types"), 0x02, kUnitTypes - 1, 1);

    const Json& allegiances = file.take("lance_allegiances_by_scenario");
    const std::string allegiance_where = file.at("lance_allegiances_by_scenario");
    if (!allegiances.is_array() || allegiances.array().size() != 7)
        fail(allegiance_where, "is not an array of 7 scenarios");
    std::vector<std::uint8_t>& allegiance_out = tables["units.lance_allegiances"];
    allegiance_out.clear();
    for (std::size_t scenario = 0; scenario < 7; ++scenario) {
        const std::vector<std::uint8_t> three = numbers(
            allegiances.array()[scenario], allegiance_where + "[" + std::to_string(scenario) + "]", 3, 0xFF, 1);
        allegiance_out.insert(allegiance_out.end(), three.begin(), three.end());
    }

    // The training's lists of unit types by class, each ended by FFh, one
    // after another as the original's; a null list has no bytes.
    const Json& lists = file.take("unit_class_types");
    const std::string lists_where = file.at("unit_class_types");
    if (!lists.is_array() || lists.array().size() != kUnitClasses)
        fail(lists_where, "is not an array of " + std::to_string(kUnitClasses) + " lists or nulls");
    std::vector<std::uint8_t>& bytes = tables["units.unit_class_types"];
    bytes.clear();
    for (std::size_t unit_class = 0; unit_class < kUnitClasses; ++unit_class) {
        const Json& list = lists.array()[unit_class];
        const std::string where = lists_where + "[" + std::to_string(unit_class) + "]";
        if (std::holds_alternative<std::nullptr_t>(list.value)) {
            g_class_list_starts[unit_class] = -1;
            continue;
        }
        if (!list.is_array()) fail(where, "is not an array of unit types or null");
        g_class_list_starts[unit_class] = long(bytes.size());
        for (std::size_t index = 0; index < list.array().size(); ++index)
            bytes.push_back(std::uint8_t(
                whole(list.array()[index], where + "[" + std::to_string(index) + "]", kUnitTypes - 1)));
        bytes.push_back(kNoUnitType);
        if (bytes.size() > kUnitClassListBytes)
            fail(where, "doesn't fit: the lists hold " + std::to_string(kUnitClassListBytes) +
                            " bytes in all, one more than its types for each list");
    }
    bytes.resize(kUnitClassListBytes, 0);
    file.done();
}

constexpr std::size_t kPilots = 0x3B;  // REVENGE_PILOT_ROSTER_COUNT

void read_pilots(const Json& root, Tables& tables) {
    Fields file(root, "pilots.json:");
    const Json& pilots = file.take("pilots");
    const std::string pilots_where = file.at("pilots");
    if (!pilots.is_array() || pilots.array().size() != kPilots)
        fail(pilots_where, "is not an array of " + std::to_string(kPilots) + " pilots");
    // RevengePilotRosterRecord: identity, experience, gunnery, and the
    // availability the campaign changes, which starts at 0.
    std::vector<std::uint8_t>& roster = tables["pilots.roster"];
    roster.assign(kPilots * 4, 0);
    for (std::size_t index = 0; index < kPilots; ++index) {
        Fields pilot(pilots.array()[index], pilots_where + "[" + std::to_string(index) + "]");
        roster[index * 4] = std::uint8_t(whole(pilot.take("identity"), pilot.at("identity"), unsigned(kPilots - 1)));
        roster[index * 4 + 1] = std::uint8_t(whole(pilot.take("experience"), pilot.at("experience"), 0x03));
        roster[index * 4 + 2] = std::uint8_t(whole(pilot.take("gunnery"), pilot.at("gunnery"), 0x7F));
        pilot.done();
    }

    const Json& defaults = file.take("default_pilots_by_scenario");
    const std::string defaults_where = file.at("default_pilots_by_scenario");
    if (!defaults.is_array() || defaults.array().size() != 5)
        fail(defaults_where, "is not an array of 5 rows");
    std::vector<std::uint8_t>& defaults_out = tables["pilots.default_by_scenario"];
    defaults_out.clear();
    for (std::size_t row = 0; row < 5; ++row) {
        const std::vector<std::uint8_t> four = numbers(defaults.array()[row],
            defaults_where + "[" + std::to_string(row) + "]", 4, unsigned(kPilots - 1), 2);
        defaults_out.insert(defaults_out.end(), four.begin(), four.end());
    }
    tables["pilots.fixed"] = numbers(file.take("fixed_pilots"), file.at("fixed_pilots"), 2, unsigned(kPilots - 1), 1);
    file.done();
}

void read_campaign(const Json& root, Tables& tables) {
    Fields file(root, "campaign.json:");
    tables["campaign.scenario_ordinal_base"] = numbers(
        file.take("scenario_ordinal_base_by_stage"), file.at("scenario_ordinal_base_by_stage"), 0x16, 0xFF, 1);

    // CAMPAIGN_MISSION_DATE_COUNT dates, each a month (MonthNameTextTable's
    // index), a year and the months the date screen counts on by.
    const Json& dates = file.take("mission_dates");
    const std::string dates_where = file.at("mission_dates");
    if (!dates.is_array() || dates.array().size() != 0x0C)
        fail(dates_where, "is not an array of 12 dates");
    std::vector<std::uint8_t>& months = tables["campaign.mission_date_months"];
    std::vector<std::uint8_t>& years = tables["campaign.mission_date_years"];
    std::vector<std::uint8_t>& advances = tables["campaign.mission_date_advance_months"];
    months.assign(0x0C, 0);
    years.assign(0x0C * 2, 0);
    advances.assign(0x0C * 2, 0);
    for (std::size_t index = 0; index < 0x0C; ++index) {
        Fields date(dates.array()[index], dates_where + "[" + std::to_string(index) + "]");
        months[index] = std::uint8_t(whole(date.take("month"), date.at("month"), 0x0B));
        put_word(&years[index * 2], whole(date.take("year"), date.at("year"), 0xFFFF));
        put_word(&advances[index * 2], whole(date.take("advance_months"), date.at("advance_months"), 0xFFFF));
        date.done();
    }

    tables["campaign.scenario_unit_cards"] = numbers(
        file.take("scenario_unit_cards_by_family"), file.at("scenario_unit_cards_by_family"), 0x07, 0x12, 1);
    tables["campaign.pilot_cards"] = numbers(
        file.take("pilot_cards_by_family"), file.at("pilot_cards_by_family"), 0x07, 0x12, 1);
    file.done();
}

// Arrays of `count` objects with the given fields, each field one table of
// `width`-byte numbers up to `maximum` (field name, table id, width, maximum).
struct Column {
    const char* field;
    const char* table;
    std::size_t width;
    unsigned maximum;
};

void records(const Json& value, const std::string& where, std::size_t count, std::initializer_list<Column> columns,
             Tables& tables) {
    if (!value.is_array() || value.array().size() != count)
        fail(where, "is not an array of " + std::to_string(count) + " records");
    for (const Column& column : columns) tables[column.table].assign(count * column.width, 0);
    for (std::size_t index = 0; index < count; ++index) {
        Fields record(value.array()[index], where + "[" + std::to_string(index) + "]");
        for (const Column& column : columns) {
            const unsigned number = whole(record.take(column.field), record.at(column.field), column.maximum);
            std::vector<std::uint8_t>& out = tables[column.table];
            if (column.width == 2)
                put_word(&out[index * 2], number);
            else
                out[index] = std::uint8_t(number);
        }
        record.done();
    }
}

// data/ui: layout, ours as the rules are.
void read_mission(const Json& root, Tables& tables) {
    Fields file(root, "mission.json:");
    tables["mission.fade_colors"] = numbers(file.take("fade_colors"), file.at("fade_colors"), 0x04, 0xFF, 1);
    records(file.take("blink_rects"), file.at("blink_rects"), 0x07,
            {{"x", "mission.blink_x", 2, 0xFFFF}, {"y", "mission.blink_y", 2, 0xFFFF},
             {"extent", "mission.blink_extent", 1, 0xFF}},
            tables);
    records(file.take("sprites"), file.at("sprites"), 0x0C,
            {{"tile_x", "mission.sprite_tile_x", 2, 0xFFFF}, {"pixel_y", "mission.sprite_pixel_y", 2, 0xFFFF},
             {"width_tiles", "mission.sprite_width_tiles", 2, 0xFFFF},
             {"height_pixels", "mission.sprite_height_pixels", 2, 0xFFFF}},
            tables);
    file.done();
}

// A struct of the original's, field by field in its order: a field on its
// own is a key (one number, or an array of `count`); the fields of a record
// (`member` set) are consecutive in the struct and one array of `count`
// records in the file, each record holding one number of each.
struct LayoutField {
    const char* key;
    const char* member;
    std::size_t width;
    std::size_t count;
};

std::vector<std::uint8_t> struct_layout(const Json& value, const std::string& where,
                                        std::span<const LayoutField> fields) {
    Fields object(value, where);
    std::vector<std::uint8_t> out;
    const auto append = [&out](unsigned number, std::size_t width) {
        out.push_back(std::uint8_t(number & 0xFF));
        if (width == 2) out.push_back(std::uint8_t(number >> 8));
    };
    std::size_t first = 0;
    while (first < fields.size()) {
        const LayoutField& field = fields[first];
        const unsigned maximum = field.width == 2 ? 0xFFFF : 0xFF;
        if (field.member == nullptr) {
            const Json& item = object.take(field.key);
            if (field.count == 1) {
                append(whole(item, object.at(field.key), maximum), field.width);
            } else {
                const std::vector<std::uint8_t> bytes =
                    numbers(item, object.at(field.key), field.count, maximum, field.width);
                out.insert(out.end(), bytes.begin(), bytes.end());
            }
            ++first;
            continue;
        }
        std::size_t last = first;
        while (last < fields.size() && fields[last].member != nullptr &&
               std::string_view(fields[last].key) == field.key)
            ++last;
        const Json& list = object.take(field.key);
        const std::string list_where = object.at(field.key);
        if (!list.is_array() || list.array().size() != field.count)
            fail(list_where, "is not an array of " + std::to_string(field.count) + " records");
        std::vector<std::vector<unsigned>> columns(last - first, std::vector<unsigned>(field.count, 0));
        for (std::size_t index = 0; index < field.count; ++index) {
            Fields record(list.array()[index], list_where + "[" + std::to_string(index) + "]");
            for (std::size_t column = first; column < last; ++column)
                columns[column - first][index] =
                    whole(record.take(fields[column].member), record.at(fields[column].member),
                          fields[column].width == 2 ? 0xFFFF : 0xFF);
            record.done();
        }
        for (std::size_t column = first; column < last; ++column)
            for (const unsigned number : columns[column - first]) append(number, fields[column].width);
        first = last;
    }
    object.done();
    return out;
}

// LanceEditorLayoutData (revenge/original/lance_editor_data.h), field by field.
constexpr LayoutField kLanceEditorLayout[] = {
    {"training_unit_choices", "left", 2, 12},
    {"training_unit_choices", "top", 2, 12},
    {"training_control_choices", "left", 2, 4},
    {"training_control_choices", "top", 2, 4},
    {"training_unit_group_left", nullptr, 2, 3},
    {"training_unit_group_labels", "left", 2, 3},
    {"training_unit_group_labels", "width", 2, 3},
    {"allegiance_header_color", nullptr, 1, 2},
    {"training_deployment_slot_left", nullptr, 2, 3},
    {"training_control_choice_column", nullptr, 2, 2},
    {"training_category_regions", "left", 2, 2},
    {"training_category_regions", "top", 2, 2},
    {"training_category_regions", "width", 2, 2},
    {"training_category_regions", "height", 2, 2},
    {"training_scenario_regions", "left", 2, 6},
    {"training_scenario_regions", "top", 2, 6},
    {"training_scenario_regions", "width", 2, 6},
    {"training_scenario_regions", "height", 2, 6},
    {"training_deployment_frame_left", nullptr, 2, 3},
    {"training_deployment_text_color", nullptr, 2, 2},
    {"training_deployment_background_color", nullptr, 2, 2},
    {"training_roster_labels", "left", 2, 3},
    {"training_roster_labels", "width", 2, 3},
    {"training_category_text_color", nullptr, 2, 2},
    {"training_category_background_color", nullptr, 2, 2},
    {"training_scenario_text_color", nullptr, 2, 2},
    {"training_scenario_background_color", nullptr, 2, 2},
    {"unit_editor_sections", "left", 2, 8},
    {"unit_editor_sections", "top", 2, 8},
    {"unit_editor_sections", "width", 2, 8},
    {"unit_editor_sections", "height", 2, 8},
    {"scenario_variant_texts", "left", 2, 2},
    {"scenario_variant_texts", "top", 2, 2},
    {"scenario_variant_normal_color", nullptr, 2, 1},
    {"scenario_variant_selected_color", nullptr, 2, 1},
    {"scenario_variant_label_color", nullptr, 2, 4},
    {"scenario_variant_regions", "left", 2, 2},
    {"scenario_variant_regions", "top", 2, 2},
    {"scenario_variant_regions", "width", 2, 2},
    {"scenario_variant_regions", "height", 2, 2},
    {"unit_roster_left_column", nullptr, 2, 1},
    {"unit_roster_right_column", nullptr, 2, 1},
    {"unit_editor_section_rows", nullptr, 2, 4},
    {"unit_editor_section_button_rows", nullptr, 2, 4},
    {"unit_card_background_color_by_state", nullptr, 2, 4},
    {"unit_card_text_color_by_state", nullptr, 2, 4},
    {"lance_assignment_text_color", nullptr, 2, 2},
    {"pilot_editor_section_rows", nullptr, 2, 4},
    {"pilot_card_text_color", nullptr, 2, 4},
    {"lance_backings", "source_tile_x", 2, 4},
    {"lance_backings", "source_pixel_y", 2, 4},
    {"lance_backings", "width_tiles", 2, 4},
    {"lance_backings", "height_pixels", 2, 4},
    {"scenario_backings", "source_tile_x", 2, 2},
    {"scenario_backings", "source_pixel_y", 2, 2},
    {"scenario_backings", "width_tiles", 2, 2},
    {"scenario_backings", "height_pixels", 2, 2},
    {"editor_backings", "source_tile_x", 2, 8},
    {"editor_backings", "source_pixel_y", 2, 8},
    {"editor_backings", "width_tiles", 2, 8},
    {"editor_backings", "height_pixels", 2, 8},
};

constexpr std::size_t kSelectionCards = 0x12;  // UNIT_SELECTION_CARD_COUNT
constexpr std::size_t kLanceSlots = 0x0D;

// `count` records of `size` bytes whose first three words are a place
// ({"left", "top", "width"} in the file); the bytes at `empty` are FFh, the
// rest 0.
std::vector<std::uint8_t> placed_records(const Json& value, const std::string& where, std::size_t count,
                                         std::size_t size, std::initializer_list<std::size_t> empty) {
    if (!value.is_array() || value.array().size() != count)
        fail(where, "is not an array of " + std::to_string(count) + " places");
    std::vector<std::uint8_t> out(count * size, 0);
    for (std::size_t index = 0; index < count; ++index) {
        Fields place(value.array()[index], where + "[" + std::to_string(index) + "]");
        std::uint8_t* record = &out[index * size];
        put_word(record, whole(place.take("left"), place.at("left"), 0xFFFF));
        put_word(record + 2, whole(place.take("top"), place.at("top"), 0xFFFF));
        put_word(record + 4, whole(place.take("width"), place.at("width"), 0xFFFF));
        place.done();
        for (const std::size_t at : empty) record[at] = 0xFF;
    }
    return out;
}

// The shared graphics buffers the lance editor's sprites and portraits are
// drawn from (SharedGraphicsBackingBuffers).
constexpr unsigned kLastGraphicsBuffer = 0x25;

void read_lance_editor(const Json& root, Tables& tables) {
    Fields file(root, "lance_editor.json:");
    tables["lance_editor.layout"] = struct_layout(file.take("layout"), file.at("layout"), kLanceEditorLayout);
    tables["lance_editor.unit_sprites"] =
        numbers(file.take("unit_sprite_by_type"), file.at("unit_sprite_by_type"), 0x59, kLastGraphicsBuffer, 1);
    tables["lance_editor.pilot_portraits"] = numbers(file.take("pilot_portrait_by_pilot"),
                                                     file.at("pilot_portrait_by_pilot"), 0x21, kLastGraphicsBuffer, 1);

    // UnitSelectionCard (15 bytes) and RevengeUnitLanceAssignment (10): the
    // place of each, from here; the rest is the starting state
    // (data/start/lance_editor.json), the slots and ids empty (FFh) and the
    // card's sprite null.
    tables["lance_editor.selection_cards"] = placed_records(file.take("selection_cards"), file.at("selection_cards"),
                                                            kSelectionCards, 15, {0x07, 0x08, 0x09, 0x0A});
    tables["lance_editor.lance_slots"] =
        placed_records(file.take("lance_slots"), file.at("lance_slots"), kLanceSlots, 10, {0x07, 0x08, 0x09});
    file.done();
}

// data/start/lance_editor.json, after data/ui's: the records' starting state
// and the editor's.
void read_lance_editor_start(const Json& root, Tables& tables) {
    Fields file(root, "lance_editor.json (start):");
    const std::vector<std::uint8_t> states =
        numbers(file.take("selection_card_states"), file.at("selection_card_states"), kSelectionCards, 0xFF, 1);
    std::vector<std::uint8_t>& cards = tables.at("lance_editor.selection_cards");
    for (std::size_t card = 0; card < kSelectionCards; ++card) cards[card * 15 + 6] = states[card];
    const std::vector<std::uint8_t> styles =
        numbers(file.take("lance_slot_styles"), file.at("lance_slot_styles"), kLanceSlots, 0xFF, 1);
    std::vector<std::uint8_t>& slots = tables.at("lance_editor.lance_slots");
    for (std::size_t slot = 0; slot < kLanceSlots; ++slot) slots[slot * 10 + 6] = styles[slot];
    tables["lance_editor_start.lance_slot_counts"] =
        numbers(file.take("lance_slot_counts"), file.at("lance_slot_counts"), 3, 0xFFFF, 2);
    tables["lance_editor_start.section_enabled"] =
        numbers(file.take("section_enabled"), file.at("section_enabled"), 0x1D, 1, 1);
    std::vector<std::uint8_t>& reload = tables["lance_editor_start.reload_graphics"];
    reload.assign(2, 0);
    put_word(reload.data(), whole(file.take("reload_graphics"), file.at("reload_graphics"), 1));
    file.done();
}

constexpr unsigned kResourceFiles = 0x5A;  // ORIGINAL_RESOURCE_DESCRIPTOR_COUNT

constexpr std::size_t kCampaignMessages = 0xB2;  // CAMPAIGN_MESSAGE_COUNT

void read_screens(const Json& root, Tables& tables) {
    Fields file(root, "screens.json:");
    tables["screens.palette_files"] = numbers(file.take("palette_file_by_screen"), file.at("palette_file_by_screen"),
                                              0x25, kResourceFiles - 1, 1);

    // TextWindowFrameDefinition: the four corner tiles, then the top, left,
    // right and bottom edges' tile runs, each ended by FFh, in 12 words.
    Fields frame(file.take("text_window_frame"), file.at("text_window_frame"));
    std::vector<std::uint8_t>& out = tables["screens.text_window_frame"];
    out.assign(0x0C * 2, 0);
    std::size_t used = 0;
    const auto put = [&out, &used, &frame](unsigned tile) {
        if (used == 0x0C) fail(frame.where(), "doesn't fit: the frame holds 12 tiles and ends in all");
        put_word(&out[used * 2], tile);
        ++used;
    };
    Fields corners(frame.take("corners"), frame.at("corners"));
    for (const char* corner : {"top_left", "top_right", "bottom_left", "bottom_right"})
        put(whole(corners.take(corner), corners.at(corner), 0xFE));
    corners.done();
    Fields edges(frame.take("edges"), frame.at("edges"));
    for (const char* edge : {"top", "left", "right", "bottom"}) {
        const Json& tiles = edges.take(edge);
        const std::string where = edges.at(edge);
        if (!tiles.is_array()) fail(where, "is not an array of tiles");
        for (std::size_t index = 0; index < tiles.array().size(); ++index)
            put(whole(tiles.array()[index], where + "[" + std::to_string(index) + "]", 0xFE));
        put(0xFF);
    }
    edges.done();
    if (used != 0x0C) fail(frame.where(), "doesn't fill the frame's 12 tiles and ends");
    frame.done();

    // CampaignMessageTable: 178 records of the message's text, a far pointer
    // (the reference edition's place of campaign_message.N; the first has
    // none), and the height of its window in tiles.
    const std::vector<std::uint8_t> heights =
        numbers(file.take("campaign_message_window_heights"), file.at("campaign_message_window_heights"),
                kCampaignMessages, 0x7F, 1);
    std::vector<std::uint8_t>& messages = tables["screens.campaign_messages"];
    messages.assign(kCampaignMessages * 5, 0);
    for (std::size_t index = 1; index < kCampaignMessages; ++index)
        put_text_pointer(&messages[index * 5], STORY_ID_CAMPAIGN_MESSAGE_0 + int(index), "screens.json:");
    for (std::size_t index = 0; index < kCampaignMessages; ++index) messages[index * 5 + 4] = heights[index];
    file.done();
}

// `count` records of eight words: the file's `keys` in order, each a word up
// to FFFFh, at the record's words `at`; the other words are 0.
std::vector<std::uint8_t> word_records(const Json& value, const std::string& where, std::size_t count,
                                       std::initializer_list<const char*> keys,
                                       std::initializer_list<std::size_t> at) {
    if (!value.is_array() || value.array().size() != count)
        fail(where, "is not an array of " + std::to_string(count) + " records");
    std::vector<std::uint8_t> out(count * 0x10, 0);
    for (std::size_t index = 0; index < count; ++index) {
        Fields record(value.array()[index], where + "[" + std::to_string(index) + "]");
        auto word = at.begin();
        for (const char* key : keys)
            put_word(&out[index * 0x10 + *word++ * 2], whole(record.take(key), record.at(key), 0xFFFF));
        record.done();
    }
    return out;
}

// data/ui/windows.json.
void read_windows(const Json& root, Tables& tables) {
    Fields file(root, "windows.json:");
    // TextWindowContext: the six words from here, the cursor's column and row
    // (words 6 and 7) 0.
    tables["windows.text_windows"] =
        word_records(file.take("text_windows"), file.at("text_windows"), 0x17,
                     {"left_tile", "top_tile", "width_tiles", "height_tiles", "foreground_color", "background_color"},
                     {0, 1, 2, 3, 4, 5});
    // OriginalUiRegionRecord: all but the selected row (word 4), which is 0.
    tables["windows.ui_regions"] =
        word_records(file.take("ui_regions"), file.at("ui_regions"), 0x10,
                     {"left_column", "top_row", "width_columns", "row_count", "unknown000a", "xor_color",
                      "unknown000e"},
                     {0, 1, 2, 3, 5, 6, 7});
    Fields keys(file.take("navigation_keys"), file.at("navigation_keys"));
    for (const char* key : {"up", "down", "previous_region", "next_region"}) {
        std::vector<std::uint8_t>& out = tables[std::string("windows.navigation_key_") + key];
        out.assign(2, 0);
        put_word(out.data(), whole(keys.take(key), keys.at(key), 0xFFFF));
    }
    keys.done();
    file.done();
}

// A line of the unit status glyph: [x0, y0, x1, y1] in pixels from its
// origin, the start into entry `at` of the two tables and the end into entry
// `at + 8`.
void put_line(const Json& value, const std::string& where, std::vector<std::uint8_t>& x, std::vector<std::uint8_t>& y,
              std::size_t at) {
    const std::vector<std::uint8_t> line = numbers(value, where, 4, 0x7FFF, 2);
    std::copy_n(&line[0], 2, &x[at * 2]);
    std::copy_n(&line[2], 2, &y[at * 2]);
    std::copy_n(&line[4], 2, &x[(at + 8) * 2]);
    std::copy_n(&line[6], 2, &y[(at + 8) * 2]);
}

void read_tactical_panel(const Json& root, Tables& tables) {
    Fields file(root, "tactical_panel.json:");
    const Json& facing = file.take("facing_lines");
    const std::string facing_where = file.at("facing_lines");
    if (!facing.is_array() || facing.array().size() != 8) fail(facing_where, "is not an array of 8 facings");
    std::vector<std::uint8_t>* tables_by_line[2][2] = {
        {&tables["tactical_panel.facing_start_x"], &tables["tactical_panel.facing_start_y"]},
        {&tables["tactical_panel.facing_end_x"], &tables["tactical_panel.facing_end_y"]}};
    for (auto& pair : tables_by_line)
        for (std::vector<std::uint8_t>* table : pair) table->assign(0x10 * 2, 0);
    for (std::size_t direction = 0; direction < 8; ++direction) {
        const Json& lines = facing.array()[direction];
        const std::string where = facing_where + "[" + std::to_string(direction) + "]";
        if (!lines.is_array() || lines.array().size() != 2) fail(where, "is not an array of 2 lines");
        for (std::size_t line = 0; line < 2; ++line)
            put_line(lines.array()[line], where + "[" + std::to_string(line) + "]", *tables_by_line[line][0],
                     *tables_by_line[line][1], direction);
    }

    // By movement band (UnitMovementHeatExponent less one), then facing.
    const Json& movement = file.take("movement_lines");
    const std::string movement_where = file.at("movement_lines");
    if (!movement.is_array() || movement.array().size() != 2) fail(movement_where, "is not an array of 2 bands");
    std::vector<std::uint8_t>& movement_x = tables["tactical_panel.movement_x"];
    std::vector<std::uint8_t>& movement_y = tables["tactical_panel.movement_y"];
    movement_x.assign(0x20 * 2, 0);
    movement_y.assign(0x20 * 2, 0);
    for (std::size_t band = 0; band < 2; ++band) {
        const Json& lines = movement.array()[band];
        const std::string where = movement_where + "[" + std::to_string(band) + "]";
        if (!lines.is_array() || lines.array().size() != 8) fail(where, "is not an array of 8 facings");
        for (std::size_t direction = 0; direction < 8; ++direction)
            put_line(lines.array()[direction], where + "[" + std::to_string(direction) + "]", movement_x, movement_y,
                     band * 0x10 + direction);
    }

    records(file.take("formation_marker_origins"), file.at("formation_marker_origins"), 0x0A,
            {{"x", "tactical_panel.formation_marker_x", 2, 0x7FFF}, {"y", "tactical_panel.formation_marker_y", 2, 0x7FFF}},
            tables);
    file.done();
}

// data/start: the state a new campaign begins with, ours as the rules are.
// The three copies of the campaign's random pools (initial, forward,
// replay), each the unit pool (RandomUnitTypePool's 7 unit types), the pilot
// pool (RandomPilotPool's 15 pilots) and the pilot pool's count, which
// RandomPilotPoolCounts holds for all three.
void read_new_campaign(const Json& root, Tables& tables) {
    Fields file(root, "new_campaign.json:");
    std::vector<std::uint8_t>& counts = tables["new_campaign.random_pilot_counts"];
    counts.assign(3, 0);
    static const char* const kCopies[3] = {"initial", "forward", "replay"};
    for (std::size_t copy = 0; copy < 3; ++copy) {
        const std::string name = kCopies[copy];
        Fields state(file.take(name), file.at(name));
        const Json& units = state.take("random_unit_pool");
        const std::string units_where = state.at("random_unit_pool");
        if (!units.is_array() || units.array().size() != 7)
            fail(units_where, "is not an array of 7 unit types or nulls");
        std::vector<std::uint8_t>& unit_pool = tables["new_campaign." + name + "_unit_pool"];
        unit_pool.assign(7, 0);
        for (std::size_t index = 0; index < 7; ++index)
            unit_pool[index] = unit_type(units.array()[index], units_where + "[" + std::to_string(index) + "]");
        const Json& pilots = state.take("random_pilot_pool");
        const std::string pilots_where = state.at("random_pilot_pool");
        if (!pilots.is_array() || pilots.array().size() != 15) fail(pilots_where, "is not an array of 15 pilots");
        std::vector<std::uint8_t>& pool = tables["new_campaign." + name + "_pilot_pool"];
        pool.assign(15, 0);
        for (std::size_t index = 0; index < 15; ++index) {
            const Json& pilot = pilots.array()[index];
            pool[index] = std::holds_alternative<std::nullptr_t>(pilot.value)
                              ? std::uint8_t(0xFF)  // REVENGE_PILOT_ID_NONE
                              : std::uint8_t(whole(pilot, pilots_where + "[" + std::to_string(index) + "]",
                                                   unsigned(kPilots - 1)));
        }
        counts[copy] = std::uint8_t(whole(state.take("random_pilot_count"), state.at("random_pilot_count"), 15));
        state.done();
    }
    file.done();
}

// data/start/save_directory.json: CampaignSaveDirectory as it stands before a
// save file is read, the 4-byte prefix then 6 slots of 22 bytes (occupied,
// then the name, NUL-padded to 21).
void read_save_directory(const Json& root, Tables& tables) {
    constexpr std::size_t kSlots = 6, kSlotSize = 0x16, kNameSize = 0x15;
    Fields file(root, "save_directory.json:");
    std::vector<std::uint8_t> out = numbers(file.take("prefix"), file.at("prefix"), 4, 0xFF, 1);
    const Json& slots = file.take("slots");
    const std::string where = file.at("slots");
    if (!slots.is_array() || slots.array().size() != kSlots) fail(where, "is not an array of 6 slots");
    for (std::size_t index = 0; index < kSlots; ++index) {
        Fields slot(slots.array()[index], where + "[" + std::to_string(index) + "]");
        std::uint8_t record[kSlotSize] = {};
        record[0] = static_cast<std::uint8_t>(whole(slot.take("occupied"), slot.at("occupied"), 1));
        const Json& name = slot.take("name");
        if (!name.is_string() || name.string().size() >= kNameSize)
            fail(slot.at("name"), "is not a name of up to 20 characters");
        for (const char c : name.string())
            if (static_cast<unsigned char>(c) < 0x20 || static_cast<unsigned char>(c) > 0x7E)
                fail(slot.at("name"), "holds a character outside printable ASCII");
        std::copy(name.string().begin(), name.string().end(), record + 1);
        slot.done();
        out.insert(out.end(), record, record + kSlotSize);
    }
    file.done();
    tables["save_directory.directory"] = std::move(out);
}

// data/start/game.json: the play's starting settings, as words.
void read_game_start(const Json& root, Tables& tables) {
    Fields file(root, "game.json:");
    const auto word = [&](const char* key, const char* table, unsigned maximum) {
        std::vector<std::uint8_t>& out = tables[table];
        out.assign(2, 0);
        put_word(out.data(), whole(file.take(key), file.at(key), maximum));
    };
    word("tactical_game_speed", "game_start.tactical_game_speed", 0x07);
    word("tactical_update_delay_ticks", "game_start.tactical_update_delay_ticks", 0xFFFF);
    word("presentation_dismiss_timeout", "game_start.presentation_dismiss_timeout", 0xFFFF);
    word("tactical_status_blink_countdown", "game_start.tactical_status_blink_countdown", 0xFFFF);
    word("displayed_tactical_status_row", "game_start.displayed_tactical_status_row", 0xFFFF);
    file.done();
}

// data/sound/speaker.json: the speaker's notes, the timer divisor of each of
// the 96 notes an effect stream names (0 is a rest).
void read_speaker(const Json& root, Tables& tables) {
    Fields file(root, "speaker.json:");
    tables["sound.speaker_note_divisors"] = numbers(file.take("note_divisors"), file.at("note_divisors"), 0x60, 0xFFFF, 2);
    file.done();
}

// Bytes two tables share in the original's memory must agree.
void check_shared(const Tables& tables) {
    // The first missile weapon's bonus is the high byte of the last direction
    // entry, which is a table number from 0 to 3: so the bonus is 0.
    const std::uint8_t bonus = tables.at("weapons.improved_missile_damage_bonus")[0];
    const std::uint8_t shared = tables.at("combat.hit_table_by_direction")[0x1F];
    if (bonus != shared)
        fail("weapons.json: weapon_table." + std::string(kWeapons[kFirstMissileWeapon]) +
                 ".improved_missile_damage_bonus",
             "is one byte with the high byte of combat.json's hit_table_by_direction[15], which is " +
                 std::to_string(shared));
}

Json read_file(const std::filesystem::path& path, const std::string& name) {
    std::ifstream in(path, std::ios::binary);
    if (!in) fail(name, "can't be read from " + path.string());
    const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    try {
        return support::parse_json(text);
    } catch (const support::JsonError& error) {
        fail(name + ":", error.what());
    }
}

}  // namespace

void load(const std::filesystem::path& data_dir) {
    const std::filesystem::path rules = data_dir / "rules";
    Tables tables;
    read_weapons(read_file(rules / "weapons.json", "weapons.json"), tables);
    read_combat(read_file(rules / "combat.json", "combat.json"), tables);
    read_ai(read_file(rules / "ai.json", "ai.json"), tables);
    read_units(read_file(rules / "units.json", "units.json"), tables);
    read_pilots(read_file(rules / "pilots.json", "pilots.json"), tables);
    read_campaign(read_file(rules / "campaign.json", "campaign.json"), tables);
    const std::filesystem::path ui = data_dir / "ui";
    read_mission(read_file(ui / "mission.json", "mission.json"), tables);
    read_lance_editor(read_file(ui / "lance_editor.json", "lance_editor.json"), tables);
    read_screens(read_file(ui / "screens.json", "screens.json"), tables);
    read_tactical_panel(read_file(ui / "tactical_panel.json", "tactical_panel.json"), tables);
    read_windows(read_file(ui / "windows.json", "windows.json"), tables);
    const std::filesystem::path start = data_dir / "start";
    read_new_campaign(read_file(start / "new_campaign.json", "new_campaign.json"), tables);
    read_lance_editor_start(read_file(start / "lance_editor.json", "lance_editor.json (start)"), tables);
    read_game_start(read_file(start / "game.json", "game.json"), tables);
    read_save_directory(read_file(start / "save_directory.json", "save_directory.json"), tables);
    read_speaker(read_file(data_dir / "sound" / "speaker.json", "speaker.json"), tables);
    check_shared(tables);
    // Every row of the rule table is filled, each with its own size.
    for (std::size_t index = 0; index < RulePieceCount; ++index) {
        const RulePiece& piece = RulePieces[index];
        const auto found = tables.find(piece.id);
        if (found == tables.end() || found->second.size() != piece.size)
            throw RulesError(std::string(piece.id) + ": no rules file fills it, at its size");
        std::memcpy(piece.object, found->second.data(), piece.size);
        tables.erase(found);
    }
    if (!tables.empty())
        throw RulesError(tables.begin()->first + ": the rule table has no row for it");
    for (std::size_t unit_class = 0; unit_class < kUnitClasses; ++unit_class)
        UnitClassTypeLists[unit_class] =
            g_class_list_starts[unit_class] < 0 ? nullptr : &UnitClassTypeListBytes[g_class_list_starts[unit_class]];
    // The selection cards from their records, as a load reads them.
    UnitSelectionCards_LoadStart();
}

}  // namespace revenge::rules
