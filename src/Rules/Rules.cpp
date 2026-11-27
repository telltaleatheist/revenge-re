// The rules files' readers (Rules.h has the shapes).
#include "Rules.h"

#include <array>
#include <cmath>
#include <cstring>
#include <map>
#include <fstream>
#include <iterator>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "../Support/Json.h"

extern "C" {
#include "revenge/original_import.h"
#include "revenge/story_text.h"
}

extern "C" {
#include "revenge/rules.h"
}

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
}

}  // namespace revenge::rules
