/* Written by tools/import/make_import_table.py from src/Rules/rule_pieces.txt
 * and a copy of the reference edition: where each rule's table is in the
 * original's memory, its length and the keyed hash of its bytes there.
 * Don't edit by hand. */
#include "revenge/rules.h"
#include "revenge/combat_data.h"
#include "revenge/original_data.h"
#include "revenge/original/lance_editor_data.h"
#include "revenge/original/campaign_data.h"

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
	{ "ai.heat_limit_by_experience", &WeaponAIHeatLimitByExperience, sizeof(WeaponAIHeatLimitByExperience), 0x3A6A, 0x03E0, 0x5, "705b4ccab5c04b2d" },
	{ "ai.engagement_by_comparison_mask", &EngagementModeByComparisonMask, sizeof(EngagementModeByComparisonMask), 0x3A6A, 0x08C6, 0x100, "e51db07782afd025" },
	{ "ai.weapon_priorities", &WeaponPriorityData, sizeof(WeaponPriorityData), 0x3A6A, 0x09C6, 0x580, "3bf24bcaa69324b2" },
	{ "ai.weapon_priority_counts", &WeaponPriorityCountTable, sizeof(WeaponPriorityCountTable), 0x3A6A, 0x0F46, 0x20, "8b7feecd63b90d44" },
	{ "ai.weapon_priority_lists", &WeaponPriorityListPointers, sizeof(WeaponPriorityListPointers), 0x3A6A, 0x0F66, 0x40, "2f90e757e0eb3b77" },
	{ "ai.priority_list_by_range_heat", &WeaponPriorityProfileByRangeHeat, sizeof(WeaponPriorityProfileByRangeHeat), 0x3A6A, 0x0FA6, 0x80, "136ff1108b18585e" },
	{ "ai.formation_offsets", &FormationOffsetTable, sizeof(FormationOffsetTable), 0x3A6A, 0x17DE, 0xF0, "2c359b9dcfd3ad17" },
	{ "ai.formation_choices_aggressive", &FormationSelectionAggressive, sizeof(FormationSelectionAggressive), 0x3A6A, 0x2F72, 0xC, "72f6c97993e592dd" },
	{ "ai.formation_choices_defensive", &FormationSelectionDefensive, sizeof(FormationSelectionDefensive), 0x3A6A, 0x2F7E, 0xC, "58d91cd8eeebf566" },
	{ "ai.formation_choices_balanced", &FormationSelectionBalanced, sizeof(FormationSelectionBalanced), 0x3A6A, 0x2F8A, 0xC, "97f471c7b807f850" },
	{ "units.predefined_lances", &PredefinedUnitTypeTable, sizeof(PredefinedUnitTypeTable), 0x3A6A, 0x31CB, 0x1E, "0f462f048b246b8d" },
	{ "units.random_unit_count_by_roster", &RandomUnitTypeCountByFamily, sizeof(RandomUnitTypeCountByFamily), 0x3A6A, 0x320D, 0xA, "21aaa4a42b85cbf7" },
	{ "units.random_unit_pools", &RandomUnitTypePoolByFamily, sizeof(RandomUnitTypePoolByFamily), 0x3A6A, 0x3217, 0x31, "e85ec1ac355b2369" },
	{ "units.unit_class_types", &UnitClassTypeListBytes, sizeof(UnitClassTypeListBytes), 0x3A6A, 0x3298, 0x44, "afa9e735f900a409" },
	{ "units.fixed_unit_types", &FixedUnitTypeIds, sizeof(FixedUnitTypeIds), 0x3A6A, 0x3356, 0x2, "a82eaa24a41fddaa" },
	{ "units.lance_allegiances", &ScenarioLanceAllegianceTable, sizeof(ScenarioLanceAllegianceTable), 0x3A6A, 0x5928, 0x15, "8a2a2d51ed8066f4" },
	{ "pilots.roster", &PilotRoster, sizeof(PilotRoster), 0x3A6A, 0x1D4C, 0xEC, "3b6a8afa53abb786" },
	{ "pilots.default_by_scenario", &DefaultPilotIdsByScenario, sizeof(DefaultPilotIdsByScenario), 0x3A6A, 0x3269, 0x28, "f4a1c9d3a839f49a" },
	{ "pilots.fixed", &FixedPilotIds, sizeof(FixedPilotIds), 0x3A6A, 0x3354, 0x2, "fcff83adf5bf3e97" },
	{ "campaign.scenario_ordinal_base", &ScenarioOrdinalBaseByCampaignStage, sizeof(ScenarioOrdinalBaseByCampaignStage), 0x3A6A, 0x254C, 0x16, "9aae4d188417c3a3" },
	{ "campaign.mission_date_months", &MissionDateMonthIndex, sizeof(MissionDateMonthIndex), 0x3A6A, 0x273F, 0xC, "23f90bd8c86f8a71" },
	{ "campaign.mission_date_years", &MissionDateYear, sizeof(MissionDateYear), 0x3A6A, 0x274B, 0x18, "534344de5aa882a1" },
	{ "campaign.mission_date_advance_months", &MissionDateAdvanceMonths, sizeof(MissionDateAdvanceMonths), 0x3A6A, 0x2763, 0x18, "ee893d85b5e03044" },
	{ "campaign.scenario_unit_cards", &UnitEditorScenarioCardCountByFamily, sizeof(UnitEditorScenarioCardCountByFamily), 0x3A6A, 0x3206, 0x7, "a1d6daaf6fee93e6" },
	{ "campaign.pilot_cards", &UnitEditorPilotCardCountByFamily, sizeof(UnitEditorPilotCardCountByFamily), 0x3A6A, 0x3291, 0x7, "e8d81d90ea6156e8" },
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
_Static_assert(sizeof(WeaponAIHeatLimitByExperience) == 0x5, "ai.heat_limit_by_experience is the original's size");
_Static_assert(sizeof(EngagementModeByComparisonMask) == 0x100, "ai.engagement_by_comparison_mask is the original's size");
_Static_assert(sizeof(WeaponPriorityData) == 0x580, "ai.weapon_priorities is the original's size");
_Static_assert(sizeof(WeaponPriorityCountTable) == 0x20, "ai.weapon_priority_counts is the original's size");
_Static_assert(sizeof(WeaponPriorityListPointers) == 0x40, "ai.weapon_priority_lists is the original's size");
_Static_assert(sizeof(WeaponPriorityProfileByRangeHeat) == 0x80, "ai.priority_list_by_range_heat is the original's size");
_Static_assert(sizeof(FormationOffsetTable) == 0xF0, "ai.formation_offsets is the original's size");
_Static_assert(sizeof(FormationSelectionAggressive) == 0xC, "ai.formation_choices_aggressive is the original's size");
_Static_assert(sizeof(FormationSelectionDefensive) == 0xC, "ai.formation_choices_defensive is the original's size");
_Static_assert(sizeof(FormationSelectionBalanced) == 0xC, "ai.formation_choices_balanced is the original's size");
_Static_assert(sizeof(PredefinedUnitTypeTable) == 0x1E, "units.predefined_lances is the original's size");
_Static_assert(sizeof(RandomUnitTypeCountByFamily) == 0xA, "units.random_unit_count_by_roster is the original's size");
_Static_assert(sizeof(RandomUnitTypePoolByFamily) == 0x31, "units.random_unit_pools is the original's size");
_Static_assert(sizeof(UnitClassTypeListBytes) == 0x44, "units.unit_class_types is the original's size");
_Static_assert(sizeof(FixedUnitTypeIds) == 0x2, "units.fixed_unit_types is the original's size");
_Static_assert(sizeof(ScenarioLanceAllegianceTable) == 0x15, "units.lance_allegiances is the original's size");
_Static_assert(sizeof(PilotRoster) == 0xEC, "pilots.roster is the original's size");
_Static_assert(sizeof(DefaultPilotIdsByScenario) == 0x28, "pilots.default_by_scenario is the original's size");
_Static_assert(sizeof(FixedPilotIds) == 0x2, "pilots.fixed is the original's size");
_Static_assert(sizeof(ScenarioOrdinalBaseByCampaignStage) == 0x16, "campaign.scenario_ordinal_base is the original's size");
_Static_assert(sizeof(MissionDateMonthIndex) == 0xC, "campaign.mission_date_months is the original's size");
_Static_assert(sizeof(MissionDateYear) == 0x18, "campaign.mission_date_years is the original's size");
_Static_assert(sizeof(MissionDateAdvanceMonths) == 0x18, "campaign.mission_date_advance_months is the original's size");
_Static_assert(sizeof(UnitEditorScenarioCardCountByFamily) == 0x7, "campaign.scenario_unit_cards is the original's size");
_Static_assert(sizeof(UnitEditorPilotCardCountByFamily) == 0x7, "campaign.pilot_cards is the original's size");
