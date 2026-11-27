/* Written by tools/import/make_import_table.py from src/Rules/rule_pieces.txt
 * and a copy of the reference edition: where each rule's table is in the
 * original's memory, its length and the keyed hash of its bytes there.
 * Don't edit by hand. */
#include "revenge/rules.h"
#include "revenge/combat_data.h"
#include "revenge/original_data.h"
#include "revenge/original/lance_editor_data.h"
#include "revenge/original/campaign_data.h"
#include "revenge/original/audio_data.h"

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
	{ "mission.fade_colors", &MissionFadeColorTable, sizeof(MissionFadeColorTable), 0x3A6A, 0x2718, 0x4, "d3e8eeebdd9d39b7" },
	{ "mission.blink_x", &MissionBlinkRectX, sizeof(MissionBlinkRectX), 0x3A6A, 0x271C, 0xE, "e8711111f886ee17" },
	{ "mission.blink_y", &MissionBlinkRectY, sizeof(MissionBlinkRectY), 0x3A6A, 0x272A, 0xE, "36dfaa095982ab48" },
	{ "mission.blink_extent", &MissionBlinkRectExtent, sizeof(MissionBlinkRectExtent), 0x3A6A, 0x2738, 0x7, "988864439a626bbe" },
	{ "mission.sprite_tile_x", &MissionSpriteSourceTileX, sizeof(MissionSpriteSourceTileX), 0x3A6A, 0x2783, 0x18, "6a981c23a5e0ed2d" },
	{ "mission.sprite_pixel_y", &MissionSpriteSourcePixelY, sizeof(MissionSpriteSourcePixelY), 0x3A6A, 0x279B, 0x18, "9654b93a1db1da5c" },
	{ "mission.sprite_width_tiles", &MissionSpriteWidthTiles, sizeof(MissionSpriteWidthTiles), 0x3A6A, 0x27B3, 0x18, "0648481faaca095c" },
	{ "mission.sprite_height_pixels", &MissionSpriteHeightPixels, sizeof(MissionSpriteHeightPixels), 0x3A6A, 0x27CB, 0x18, "814bb2f50c9d771d" },
	{ "lance_editor.unit_sprites", &UnitEditorUnitSpriteIndex, sizeof(UnitEditorUnitSpriteIndex), 0x3A6A, 0x3119, 0x59, "32e2805258a97d7d" },
	{ "lance_editor.pilot_portraits", &PilotPortraitIndexById, sizeof(PilotPortraitIndexById), 0x3A6A, 0x3248, 0x21, "bc30e707e833bb08" },
	{ "lance_editor.layout", &LanceEditorLayout, sizeof(LanceEditorLayout), 0x3A6A, 0x3470, 0x1D4, "133bbdf581993fe9" },
	{ "lance_editor.selection_cards", &UnitSelectionCardStart, sizeof(UnitSelectionCardStart), 0x3A6A, 0x1BBC, 0x10E, "d51147e7cfebca65" },
	{ "lance_editor.lance_slots", &LanceAssignments, sizeof(LanceAssignments), 0x3A6A, 0x1CCA, 0x82, "ed9b671d2f37f653" },
	{ "screens.text_window_frame", &TextWindowFrameDefinition, sizeof(TextWindowFrameDefinition), 0x3A6A, 0x08AA, 0x18, "784fdaba56e12fbc" },
	{ "screens.campaign_messages", &CampaignMessageTable, sizeof(CampaignMessageTable), 0x3A6A, 0x2B54, 0x37A, "8a6e56333800019e" },
	{ "screens.palette_files", &ScreenPaletteResourceByScreen, sizeof(ScreenPaletteResourceByScreen), 0x3A6A, 0x1704, 0x25, "e78bb6d27f4e84d3" },
	{ "windows.text_windows", &TextWindowContexts, sizeof(TextWindowContexts), 0x3A6A, 0x063A, 0x170, "12875c4daab358d7" },
	{ "windows.ui_regions", &UiRegionTable, sizeof(UiRegionTable), 0x3A6A, 0x07AA, 0x100, "12112a8ba40acc81" },
	{ "windows.navigation_key_up", &UiNavigationUpKey, sizeof(UiNavigationUpKey), 0x3A6A, 0x6242, 0x2, "791a2cd414f7bcdb" },
	{ "windows.navigation_key_down", &UiNavigationDownKey, sizeof(UiNavigationDownKey), 0x3A6A, 0x6244, 0x2, "0d446e6a5bef00d9" },
	{ "windows.navigation_key_previous_region", &UiPreviousRegionKey, sizeof(UiPreviousRegionKey), 0x3A6A, 0x6246, 0x2, "161d0952f5ea12ae" },
	{ "windows.navigation_key_next_region", &UiNextRegionKey, sizeof(UiNextRegionKey), 0x3A6A, 0x6248, 0x2, "2d76dc1e579f017e" },
	{ "tactical_panel.formation_marker_x", &FormationMarkerOriginX, sizeof(FormationMarkerOriginX), 0x3A6A, 0x2F96, 0x14, "b096bff2a16441c6" },
	{ "tactical_panel.formation_marker_y", &FormationMarkerOriginY, sizeof(FormationMarkerOriginY), 0x3A6A, 0x2FAA, 0x14, "d4b7e94022312e3d" },
	{ "tactical_panel.facing_start_x", &FacingVectorStartX, sizeof(FacingVectorStartX), 0x3A6A, 0x2FBE, 0x20, "cdaf1ce7f55e11fe" },
	{ "tactical_panel.facing_start_y", &FacingVectorStartY, sizeof(FacingVectorStartY), 0x3A6A, 0x2FDE, 0x20, "fbafd066f1163192" },
	{ "tactical_panel.facing_end_x", &FacingVectorEndX, sizeof(FacingVectorEndX), 0x3A6A, 0x2FFE, 0x20, "7bcc3bfc01fd3f0f" },
	{ "tactical_panel.facing_end_y", &FacingVectorEndY, sizeof(FacingVectorEndY), 0x3A6A, 0x301E, 0x20, "276d404218862052" },
	{ "tactical_panel.movement_x", &MovementVectorX, sizeof(MovementVectorX), 0x3A6A, 0x303E, 0x40, "2e470f455aec0344" },
	{ "tactical_panel.movement_y", &MovementVectorY, sizeof(MovementVectorY), 0x3A6A, 0x307E, 0x40, "86d4436c3808c49b" },
	{ "new_campaign.initial_unit_pool", &InitialCampaignState, sizeof(InitialCampaignState), 0x3A6A, 0x1B77, 0x7, "3e02b73bac989cf9" },
	{ "new_campaign.forward_unit_pool", &ForwardCampaignState, sizeof(ForwardCampaignState), 0x3A6A, 0x1B7E, 0x7, "3e02b73bac989cf9" },
	{ "new_campaign.replay_unit_pool", &ReplayCampaignState, sizeof(ReplayCampaignState), 0x3A6A, 0x1B85, 0x7, "3e02b73bac989cf9" },
	{ "new_campaign.random_pilot_counts", &RandomPilotPoolCounts, sizeof(RandomPilotPoolCounts), 0x3A6A, 0x1B8C, 0x3, "9aa5762952aa130c" },
	{ "new_campaign.initial_pilot_pool", &InitialExtendedCampaignState, sizeof(InitialExtendedCampaignState), 0x3A6A, 0x1B8F, 0xF, "5ed3771e99a6af78" },
	{ "new_campaign.forward_pilot_pool", &ForwardExtendedCampaignState, sizeof(ForwardExtendedCampaignState), 0x3A6A, 0x1B9E, 0xF, "5ed3771e99a6af78" },
	{ "new_campaign.replay_pilot_pool", &ReplayExtendedCampaignState, sizeof(ReplayExtendedCampaignState), 0x3A6A, 0x1BAD, 0xF, "2111c357b439a9b8" },
	{ "lance_editor_start.reload_graphics", &LanceEditorReloadGraphics, sizeof(LanceEditorReloadGraphics), 0x3A6A, 0x30C8, 0x2, "58f87d0a96fd20cb" },
	{ "lance_editor_start.lance_slot_counts", &UnitEditorLanceSlotCount, sizeof(UnitEditorLanceSlotCount), 0x3A6A, 0x3112, 0x6, "0516c073ec9f9f7e" },
	{ "lance_editor_start.section_enabled", &UnitEditorSectionEnabled, sizeof(UnitEditorSectionEnabled), 0x3A6A, 0x31E9, 0x1D, "6324cca6a61409b0" },
	{ "game_start.tactical_update_delay_ticks", &TacticalUpdateDelayReloadTicks, sizeof(TacticalUpdateDelayReloadTicks), 0x3A6A, 0x17DA, 0x2, "1c6886649de1104e" },
	{ "game_start.tactical_game_speed", &TacticalGameSpeedIndex, sizeof(TacticalGameSpeedIndex), 0x3A6A, 0x17DC, 0x2, "c849093f52e6bb81" },
	{ "game_start.presentation_dismiss_timeout", &PresentationDismissTimeout, sizeof(PresentationDismissTimeout), 0x3A6A, 0x240F, 0x2, "c02198fcd61de6b3" },
	{ "game_start.tactical_status_blink_countdown", &TacticalStatusBlinkCountdown, sizeof(TacticalStatusBlinkCountdown), 0x3A6A, 0x2F36, 0x2, "58f87d0a96fd20cb" },
	{ "game_start.displayed_tactical_status_row", &DisplayedTacticalStatusRow, sizeof(DisplayedTacticalStatusRow), 0x3A6A, 0x2F38, 0x2, "ac6c9144ab715413" },
	{ "save_directory.directory", &SaveDirectory, sizeof(SaveDirectory), 0x3A6A, 0x18CE, 0x88, "f838daf88a97478a" },
	{ "sound.speaker_note_divisors", &Seg426B.MusicFrequencyDivisorTable, sizeof(Seg426B.MusicFrequencyDivisorTable), 0x426B, 0x0020, 0xC0, "46a4813a841aec0e" },
};
const size_t RulePieceCount = sizeof(RulePieces) / sizeof(RulePieces[0]);

static_assert(sizeof(WeaponDefinitions) == 0x159, "weapons.definitions is the original's size");
static_assert(sizeof(AmmoCapacityMultiplierTable) == 0x1C, "weapons.ammunition_capacity_multipliers is the original's size");
static_assert(sizeof(ImprovedMissileDamageBonusTable) == 0x7, "weapons.improved_missile_damage_bonus is the original's size");
static_assert(sizeof(WeaponCombatRatingByType) == 0x17, "weapons.combat_ratings is the original's size");
static_assert(sizeof(HeatDissipationBySinkCountQ8_8) == 0x60, "combat.heat_dissipation is the original's size");
static_assert(sizeof(HitLocationTable) == 0x78, "combat.hit_location_tables is the original's size");
static_assert(sizeof(HitLocationDirectionSelectorTable) == 0x20, "combat.hit_table_by_direction is the original's size");
static_assert(sizeof(SensorDamageProximityRangeTable) == 0x3, "combat.sensor_damage_proximity_range is the original's size");
static_assert(sizeof(UnitActionMaskByExperience) == 0x8, "combat.action_mask_by_experience is the original's size");
static_assert(sizeof(MovementTimingMaskTable) == 0x12, "combat.movement_timing_masks is the original's size");
static_assert(sizeof(MovementRatingByWalkMP) == 0x9, "combat.movement_rating_by_walk_mp is the original's size");
static_assert(sizeof(WeaponAIHeatLimitByExperience) == 0x5, "ai.heat_limit_by_experience is the original's size");
static_assert(sizeof(EngagementModeByComparisonMask) == 0x100, "ai.engagement_by_comparison_mask is the original's size");
static_assert(sizeof(WeaponPriorityData) == 0x580, "ai.weapon_priorities is the original's size");
static_assert(sizeof(WeaponPriorityCountTable) == 0x20, "ai.weapon_priority_counts is the original's size");
static_assert(sizeof(WeaponPriorityListPointers) == 0x40, "ai.weapon_priority_lists is the original's size");
static_assert(sizeof(WeaponPriorityProfileByRangeHeat) == 0x80, "ai.priority_list_by_range_heat is the original's size");
static_assert(sizeof(FormationOffsetTable) == 0xF0, "ai.formation_offsets is the original's size");
static_assert(sizeof(FormationSelectionAggressive) == 0xC, "ai.formation_choices_aggressive is the original's size");
static_assert(sizeof(FormationSelectionDefensive) == 0xC, "ai.formation_choices_defensive is the original's size");
static_assert(sizeof(FormationSelectionBalanced) == 0xC, "ai.formation_choices_balanced is the original's size");
static_assert(sizeof(PredefinedUnitTypeTable) == 0x1E, "units.predefined_lances is the original's size");
static_assert(sizeof(RandomUnitTypeCountByFamily) == 0xA, "units.random_unit_count_by_roster is the original's size");
static_assert(sizeof(RandomUnitTypePoolByFamily) == 0x31, "units.random_unit_pools is the original's size");
static_assert(sizeof(UnitClassTypeListBytes) == 0x44, "units.unit_class_types is the original's size");
static_assert(sizeof(FixedUnitTypeIds) == 0x2, "units.fixed_unit_types is the original's size");
static_assert(sizeof(ScenarioLanceAllegianceTable) == 0x15, "units.lance_allegiances is the original's size");
static_assert(sizeof(PilotRoster) == 0xEC, "pilots.roster is the original's size");
static_assert(sizeof(DefaultPilotIdsByScenario) == 0x28, "pilots.default_by_scenario is the original's size");
static_assert(sizeof(FixedPilotIds) == 0x2, "pilots.fixed is the original's size");
static_assert(sizeof(ScenarioOrdinalBaseByCampaignStage) == 0x16, "campaign.scenario_ordinal_base is the original's size");
static_assert(sizeof(MissionDateMonthIndex) == 0xC, "campaign.mission_date_months is the original's size");
static_assert(sizeof(MissionDateYear) == 0x18, "campaign.mission_date_years is the original's size");
static_assert(sizeof(MissionDateAdvanceMonths) == 0x18, "campaign.mission_date_advance_months is the original's size");
static_assert(sizeof(UnitEditorScenarioCardCountByFamily) == 0x7, "campaign.scenario_unit_cards is the original's size");
static_assert(sizeof(UnitEditorPilotCardCountByFamily) == 0x7, "campaign.pilot_cards is the original's size");
static_assert(sizeof(MissionFadeColorTable) == 0x4, "mission.fade_colors is the original's size");
static_assert(sizeof(MissionBlinkRectX) == 0xE, "mission.blink_x is the original's size");
static_assert(sizeof(MissionBlinkRectY) == 0xE, "mission.blink_y is the original's size");
static_assert(sizeof(MissionBlinkRectExtent) == 0x7, "mission.blink_extent is the original's size");
static_assert(sizeof(MissionSpriteSourceTileX) == 0x18, "mission.sprite_tile_x is the original's size");
static_assert(sizeof(MissionSpriteSourcePixelY) == 0x18, "mission.sprite_pixel_y is the original's size");
static_assert(sizeof(MissionSpriteWidthTiles) == 0x18, "mission.sprite_width_tiles is the original's size");
static_assert(sizeof(MissionSpriteHeightPixels) == 0x18, "mission.sprite_height_pixels is the original's size");
static_assert(sizeof(UnitEditorUnitSpriteIndex) == 0x59, "lance_editor.unit_sprites is the original's size");
static_assert(sizeof(PilotPortraitIndexById) == 0x21, "lance_editor.pilot_portraits is the original's size");
static_assert(sizeof(LanceEditorLayout) == 0x1D4, "lance_editor.layout is the original's size");
static_assert(sizeof(UnitSelectionCardStart) == 0x10E, "lance_editor.selection_cards is the original's size");
static_assert(sizeof(LanceAssignments) == 0x82, "lance_editor.lance_slots is the original's size");
static_assert(sizeof(TextWindowFrameDefinition) == 0x18, "screens.text_window_frame is the original's size");
static_assert(sizeof(CampaignMessageTable) == 0x37A, "screens.campaign_messages is the original's size");
static_assert(sizeof(ScreenPaletteResourceByScreen) == 0x25, "screens.palette_files is the original's size");
static_assert(sizeof(TextWindowContexts) == 0x170, "windows.text_windows is the original's size");
static_assert(sizeof(UiRegionTable) == 0x100, "windows.ui_regions is the original's size");
static_assert(sizeof(UiNavigationUpKey) == 0x2, "windows.navigation_key_up is the original's size");
static_assert(sizeof(UiNavigationDownKey) == 0x2, "windows.navigation_key_down is the original's size");
static_assert(sizeof(UiPreviousRegionKey) == 0x2, "windows.navigation_key_previous_region is the original's size");
static_assert(sizeof(UiNextRegionKey) == 0x2, "windows.navigation_key_next_region is the original's size");
static_assert(sizeof(FormationMarkerOriginX) == 0x14, "tactical_panel.formation_marker_x is the original's size");
static_assert(sizeof(FormationMarkerOriginY) == 0x14, "tactical_panel.formation_marker_y is the original's size");
static_assert(sizeof(FacingVectorStartX) == 0x20, "tactical_panel.facing_start_x is the original's size");
static_assert(sizeof(FacingVectorStartY) == 0x20, "tactical_panel.facing_start_y is the original's size");
static_assert(sizeof(FacingVectorEndX) == 0x20, "tactical_panel.facing_end_x is the original's size");
static_assert(sizeof(FacingVectorEndY) == 0x20, "tactical_panel.facing_end_y is the original's size");
static_assert(sizeof(MovementVectorX) == 0x40, "tactical_panel.movement_x is the original's size");
static_assert(sizeof(MovementVectorY) == 0x40, "tactical_panel.movement_y is the original's size");
static_assert(sizeof(InitialCampaignState) == 0x7, "new_campaign.initial_unit_pool is the original's size");
static_assert(sizeof(ForwardCampaignState) == 0x7, "new_campaign.forward_unit_pool is the original's size");
static_assert(sizeof(ReplayCampaignState) == 0x7, "new_campaign.replay_unit_pool is the original's size");
static_assert(sizeof(RandomPilotPoolCounts) == 0x3, "new_campaign.random_pilot_counts is the original's size");
static_assert(sizeof(InitialExtendedCampaignState) == 0xF, "new_campaign.initial_pilot_pool is the original's size");
static_assert(sizeof(ForwardExtendedCampaignState) == 0xF, "new_campaign.forward_pilot_pool is the original's size");
static_assert(sizeof(ReplayExtendedCampaignState) == 0xF, "new_campaign.replay_pilot_pool is the original's size");
static_assert(sizeof(LanceEditorReloadGraphics) == 0x2, "lance_editor_start.reload_graphics is the original's size");
static_assert(sizeof(UnitEditorLanceSlotCount) == 0x6, "lance_editor_start.lance_slot_counts is the original's size");
static_assert(sizeof(UnitEditorSectionEnabled) == 0x1D, "lance_editor_start.section_enabled is the original's size");
static_assert(sizeof(TacticalUpdateDelayReloadTicks) == 0x2, "game_start.tactical_update_delay_ticks is the original's size");
static_assert(sizeof(TacticalGameSpeedIndex) == 0x2, "game_start.tactical_game_speed is the original's size");
static_assert(sizeof(PresentationDismissTimeout) == 0x2, "game_start.presentation_dismiss_timeout is the original's size");
static_assert(sizeof(TacticalStatusBlinkCountdown) == 0x2, "game_start.tactical_status_blink_countdown is the original's size");
static_assert(sizeof(DisplayedTacticalStatusRow) == 0x2, "game_start.displayed_tactical_status_row is the original's size");
static_assert(sizeof(SaveDirectory) == 0x88, "save_directory.directory is the original's size");
static_assert(sizeof(Seg426B.MusicFrequencyDivisorTable) == 0xC0, "sound.speaker_note_divisors is the original's size");
