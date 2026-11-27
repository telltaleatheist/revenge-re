/* Written by tools/map/make_map_dump.py from the headers' externs; don't edit. */

#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "REVENGE.h"
#include "revenge/audio.h"
#include "revenge/combat_data.h"
#include "revenge/media_settings.h"
#include "revenge/original/audio_data.h"
#include "revenge/original/campaign_data.h"
#include "revenge/original/input_data.h"
#include "revenge/original/io_data.h"
#include "revenge/original/lance_editor_data.h"
#include "revenge/original/resource_data.h"
#include "revenge/original/scene_data.h"
#include "revenge/original_data.h"
#include "revenge/random.h"
#include "revenge/story.h"
#include "revenge/story_text.h"
#include "Audio/REVENGE_3621_MIDI_INTERNAL.h"
#include "map_dump.h"
#include "revenge/platform_backend.h"
#include "revenge/sha256.h"

#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stddef.h>

typedef struct MapDumpObject
{
	const char *name;
	const void *object;
	size_t size;          /* 0 for a string of unstated length */
	int pointers;         /* 1: a pointer or an array of them; -1: not by name */
} MapDumpObject;

static const MapDumpObject objects[] =
{
	{ "NullMIDIDriverDescriptor", &NullMIDIDriverDescriptor, sizeof(NullMIDIDriverDescriptor), 0 },
	{ "SpeakerMIDIDriverDescriptor", &SpeakerMIDIDriverDescriptor, sizeof(SpeakerMIDIDriverDescriptor), 0 },
	{ "RolandMIDIDriverDescriptor", &RolandMIDIDriverDescriptor, sizeof(RolandMIDIDriverDescriptor), 0 },
	{ "WeaponDefinitions", &WeaponDefinitions, sizeof(WeaponDefinitions), 0 },
	{ "WeaponNameTable", &WeaponNameTable, sizeof(WeaponNameTable), 1 },
	{ "ConfigOptionNames", &ConfigOptionNames, sizeof(ConfigOptionNames), 1 },
	{ "Seg426B", &Seg426B, sizeof(Seg426B), 0 },
	{ "PCSpeakerEffectStreamData", &PCSpeakerEffectStreamData, sizeof(PCSpeakerEffectStreamData), 0 },
	{ "PCSpeakerEffectStreamPointerTable", &PCSpeakerEffectStreamPointerTable, sizeof(PCSpeakerEffectStreamPointerTable), 1 },
	{ "MusicStagingBuffer", &MusicStagingBuffer, sizeof(MusicStagingBuffer), 1 },
	{ "MIDISequence", &MIDISequence, sizeof(MIDISequence), 0 },
	{ "MIDIParser", &MIDIParser, sizeof(MIDIParser), 0 },
	{ "MIDIChannelMap", &MIDIChannelMap, sizeof(MIDIChannelMap), 0 },
	{ "MIDISystemExclusive", &MIDISystemExclusive, sizeof(MIDISystemExclusive), 0 },
	{ "SpeakerMIDIRuntime", &SpeakerMIDIRuntime, sizeof(SpeakerMIDIRuntime), 0 },
	{ "SpeakerMIDIFrequencyDivisors", &SpeakerMIDIFrequencyDivisors, sizeof(SpeakerMIDIFrequencyDivisors), 0 },
	{ "MIDIInstrumentPatches", &MIDIInstrumentPatches, sizeof(MIDIInstrumentPatches), 0 },
	{ "RolandMPU401", &RolandMPU401, sizeof(RolandMPU401), 0 },
	{ "TrainingScenarioCompletionFlags", &TrainingScenarioCompletionFlags, sizeof(TrainingScenarioCompletionFlags), 0 },
	{ "ScreenPaletteResourceByScreen", &ScreenPaletteResourceByScreen, sizeof(ScreenPaletteResourceByScreen), 0 },
	{ "SaveDirectory", &SaveDirectory, sizeof(SaveDirectory), 0 },
	{ "SaveCampaignSnapshot", &SaveCampaignSnapshot, sizeof(SaveCampaignSnapshot), 0 },
	{ "InitialCampaignState", &InitialCampaignState, sizeof(InitialCampaignState), 0 },
	{ "ForwardCampaignState", &ForwardCampaignState, sizeof(ForwardCampaignState), 0 },
	{ "ReplayCampaignState", &ReplayCampaignState, sizeof(ReplayCampaignState), 0 },
	{ "InitialExtendedCampaignState", &InitialExtendedCampaignState, sizeof(InitialExtendedCampaignState), 0 },
	{ "ForwardExtendedCampaignState", &ForwardExtendedCampaignState, sizeof(ForwardExtendedCampaignState), 0 },
	{ "ReplayExtendedCampaignState", &ReplayExtendedCampaignState, sizeof(ReplayExtendedCampaignState), 0 },
	{ "TitleTextTable", &TitleTextTable, sizeof(TitleTextTable), 1 },
	{ "MonthNameTextTable", &MonthNameTextTable, sizeof(MonthNameTextTable), 1 },
	{ "MissionDateLabelTextTable", &MissionDateLabelTextTable, sizeof(MissionDateLabelTextTable), 1 },
	{ "ScenarioOrdinalBaseByCampaignStage", &ScenarioOrdinalBaseByCampaignStage, sizeof(ScenarioOrdinalBaseByCampaignStage), 0 },
	{ "ExitToDosPrompt", &ExitToDosPrompt, sizeof(ExitToDosPrompt), 1 },
	{ "CampaignPathMenuTextTable", &CampaignPathMenuTextTable, sizeof(CampaignPathMenuTextTable), 1 },
	{ "ScenarioVariantMenuTextTable", &ScenarioVariantMenuTextTable, sizeof(ScenarioVariantMenuTextTable), 1 },
	{ "AlternateScenarioMenuTextTable", &AlternateScenarioMenuTextTable, sizeof(AlternateScenarioMenuTextTable), 1 },
	{ "MainMenuTextTable", &MainMenuTextTable, sizeof(MainMenuTextTable), 1 },
	{ "InGameMenuTextTable", &InGameMenuTextTable, sizeof(InGameMenuTextTable), 1 },
	{ "SaveGameSeparatorText", &SaveGameSeparatorText, sizeof(SaveGameSeparatorText), 1 },
	{ "MissionFadeColorTable", &MissionFadeColorTable, sizeof(MissionFadeColorTable), 0 },
	{ "MissionBlinkRectX", &MissionBlinkRectX, sizeof(MissionBlinkRectX), 0 },
	{ "MissionBlinkRectY", &MissionBlinkRectY, sizeof(MissionBlinkRectY), 0 },
	{ "MissionBlinkRectExtent", &MissionBlinkRectExtent, sizeof(MissionBlinkRectExtent), 0 },
	{ "MissionDateMonthIndex", &MissionDateMonthIndex, sizeof(MissionDateMonthIndex), 0 },
	{ "MissionDateYear", &MissionDateYear, sizeof(MissionDateYear), 0 },
	{ "MissionDateAdvanceMonths", &MissionDateAdvanceMonths, sizeof(MissionDateAdvanceMonths), 0 },
	{ "MissionSpriteSourceTileX", &MissionSpriteSourceTileX, sizeof(MissionSpriteSourceTileX), 0 },
	{ "MissionSpriteSourcePixelY", &MissionSpriteSourcePixelY, sizeof(MissionSpriteSourcePixelY), 0 },
	{ "MissionSpriteWidthTiles", &MissionSpriteWidthTiles, sizeof(MissionSpriteWidthTiles), 0 },
	{ "MissionSpriteHeightPixels", &MissionSpriteHeightPixels, sizeof(MissionSpriteHeightPixels), 0 },
	{ "PilotIdentityPrefixText", &PilotIdentityPrefixText, sizeof(PilotIdentityPrefixText), 1 },
	{ "PilotNamePrefixText", &PilotNamePrefixText, sizeof(PilotNamePrefixText), 1 },
	{ "DateCommaSpaceText", &DateCommaSpaceText, sizeof(DateCommaSpaceText), 1 },
	{ "DateSpaceText", &DateSpaceText, sizeof(DateSpaceText), 1 },
	{ "ScenarioCompleteHeadingText", &ScenarioCompleteHeadingText, sizeof(ScenarioCompleteHeadingText), 1 },
	{ "ScenarioFailedHeadingText", &ScenarioFailedHeadingText, sizeof(ScenarioFailedHeadingText), 1 },
	{ "CampaignMessageTable", &CampaignMessageTable, sizeof(CampaignMessageTable), 0 },
	{ "CampaignMessageTextTable", &CampaignMessageTextTable, sizeof(CampaignMessageTextTable), 1 },
	{ "PilotRosterHeadingText", &PilotRosterHeadingText, sizeof(PilotRosterHeadingText), 1 },
	{ "UiRegionTable", &UiRegionTable, sizeof(UiRegionTable), 0 },
	{ "SecondaryResourceDriveEnabled", &SecondaryResourceDriveEnabled, sizeof(SecondaryResourceDriveEnabled), 0 },
	{ "ResourceDriveSelectionDisabled", &ResourceDriveSelectionDisabled, sizeof(ResourceDriveSelectionDisabled), 0 },
	{ "CriticalErrorCallback", &CriticalErrorCallback, sizeof(CriticalErrorCallback), 0 },
	{ "SharedGraphicsBackingBuffers", &SharedGraphicsBackingBuffers, sizeof(SharedGraphicsBackingBuffers), 1 },
	{ "UnitSelectionCards", &UnitSelectionCards, sizeof(UnitSelectionCards), 0 },
	{ "UnitSelectionCardStart", &UnitSelectionCardStart, sizeof(UnitSelectionCardStart), 0 },
	{ "LanceEditorFirstBriefingPending", &LanceEditorFirstBriefingPending, sizeof(LanceEditorFirstBriefingPending), 0 },
	{ "LanceEditorBriefingPresented", &LanceEditorBriefingPresented, sizeof(LanceEditorBriefingPresented), 0 },
	{ "LanceEditorReloadGraphics", &LanceEditorReloadGraphics, sizeof(LanceEditorReloadGraphics), 0 },
	{ "UnitAllegianceLabelTextTable", &UnitAllegianceLabelTextTable, sizeof(UnitAllegianceLabelTextTable), 1 },
	{ "UnknownUnitLabel", &UnknownUnitLabel, sizeof(UnknownUnitLabel), 1 },
	{ "UnitStatusSpace", &UnitStatusSpace, sizeof(UnitStatusSpace), 1 },
	{ "UnitRosterSeparator", &UnitRosterSeparator, sizeof(UnitRosterSeparator), 1 },
	{ "UnitStatusSeparator", &UnitStatusSeparator, sizeof(UnitStatusSeparator), 1 },
	{ "UnitStatHeaderTextTable", &UnitStatHeaderTextTable, sizeof(UnitStatHeaderTextTable), 1 },
	{ "UnitEditorLanceSlotCount", &UnitEditorLanceSlotCount, sizeof(UnitEditorLanceSlotCount), 0 },
	{ "RandomUnitTypeCount", &RandomUnitTypeCount, sizeof(RandomUnitTypeCount), 0 },
	{ "UnitEditorUnitSpriteIndex", &UnitEditorUnitSpriteIndex, sizeof(UnitEditorUnitSpriteIndex), 0 },
	{ "UnitNameSplitIndex", &UnitNameSplitIndex, sizeof(UnitNameSplitIndex), 0 },
	{ "PredefinedUnitTypeTable", &PredefinedUnitTypeTable, sizeof(PredefinedUnitTypeTable), 0 },
	{ "UnitEditorSectionEnabled", &UnitEditorSectionEnabled, sizeof(UnitEditorSectionEnabled), 0 },
	{ "UnitEditorScenarioCardCountByFamily", &UnitEditorScenarioCardCountByFamily, sizeof(UnitEditorScenarioCardCountByFamily), 0 },
	{ "RandomUnitTypeCountByFamily", &RandomUnitTypeCountByFamily, sizeof(RandomUnitTypeCountByFamily), 0 },
	{ "RandomUnitTypePoolByFamily", &RandomUnitTypePoolByFamily, sizeof(RandomUnitTypePoolByFamily), 0 },
	{ "PilotPortraitIndexById", &PilotPortraitIndexById, sizeof(PilotPortraitIndexById), 0 },
	{ "DefaultPilotIdsByScenario", &DefaultPilotIdsByScenario, sizeof(DefaultPilotIdsByScenario), 0 },
	{ "UnitEditorPilotCardCountByFamily", &UnitEditorPilotCardCountByFamily, sizeof(UnitEditorPilotCardCountByFamily), 0 },
	{ "UnitClassTypeLists", &UnitClassTypeLists, sizeof(UnitClassTypeLists), 1 },
	{ "UnitClassLabelTextTable", &UnitClassLabelTextTable, sizeof(UnitClassLabelTextTable), 1 },
	{ "TrainingDeploymentLabelTextTable", &TrainingDeploymentLabelTextTable, sizeof(TrainingDeploymentLabelTextTable), 1 },
	{ "TrainingControlLabelTextTable", &TrainingControlLabelTextTable, sizeof(TrainingControlLabelTextTable), 1 },
	{ "ScenarioVariantDescriptionTextTable", &ScenarioVariantDescriptionTextTable, sizeof(ScenarioVariantDescriptionTextTable), 1 },
	{ "UnitEditorSectionLabelTextTable", &UnitEditorSectionLabelTextTable, sizeof(UnitEditorSectionLabelTextTable), 1 },
	{ "FixedPilotIds", &FixedPilotIds, sizeof(FixedPilotIds), 0 },
	{ "FixedUnitTypeIds", &FixedUnitTypeIds, sizeof(FixedUnitTypeIds), 0 },
	{ "UnitClassTypeListBytes", &UnitClassTypeListBytes, sizeof(UnitClassTypeListBytes), 0 },
	{ "ScenarioRouteLabelTextTable", &ScenarioRouteLabelTextTable, sizeof(ScenarioRouteLabelTextTable), 1 },
	{ "UnitEditorSectionButtonLabelTextTable", &UnitEditorSectionButtonLabelTextTable, sizeof(UnitEditorSectionButtonLabelTextTable), 1 },
	{ "TrainingDeploymentRosterLabelTextTable", &TrainingDeploymentRosterLabelTextTable, sizeof(TrainingDeploymentRosterLabelTextTable), 1 },
	{ "UnitRosterColumnLabelTextTable", &UnitRosterColumnLabelTextTable, sizeof(UnitRosterColumnLabelTextTable), 1 },
	{ "FriendlyEnemySuffixText", &FriendlyEnemySuffixText, sizeof(FriendlyEnemySuffixText), 1 },
	{ "TrainingCategoryHeadingText", &TrainingCategoryHeadingText, sizeof(TrainingCategoryHeadingText), 1 },
	{ "TrainingCategoryLabelTextTable", &TrainingCategoryLabelTextTable, sizeof(TrainingCategoryLabelTextTable), 1 },
	{ "TrainingScenarioHeadingText", &TrainingScenarioHeadingText, sizeof(TrainingScenarioHeadingText), 1 },
	{ "TrainingScenarioLabelTextTable", &TrainingScenarioLabelTextTable, sizeof(TrainingScenarioLabelTextTable), 1 },
	{ "NoSelectionOptionsText", &NoSelectionOptionsText, sizeof(NoSelectionOptionsText), 1 },
	{ "TrainingUnitSelectionPrompt", &TrainingUnitSelectionPrompt, sizeof(TrainingUnitSelectionPrompt), 1 },
	{ "LanceEditorLayout", &LanceEditorLayout, sizeof(LanceEditorLayout), 0 },
	{ "PilotSurnameTextTable", &PilotSurnameTextTable, sizeof(PilotSurnameTextTable), 1 },
	{ "PilotGivenNameTextTable", &PilotGivenNameTextTable, sizeof(PilotGivenNameTextTable), 1 },
	{ "PilotExperienceLabelTextTable", &PilotExperienceLabelTextTable, sizeof(PilotExperienceLabelTextTable), 1 },
	{ "PilotGunneryLabelTextTable", &PilotGunneryLabelTextTable, sizeof(PilotGunneryLabelTextTable), 1 },
	{ "UnitMovementClassLabelTextTable", &UnitMovementClassLabelTextTable, sizeof(UnitMovementClassLabelTextTable), 1 },
	{ "UnitArmorConditionLabelTextTable", &UnitArmorConditionLabelTextTable, sizeof(UnitArmorConditionLabelTextTable), 1 },
	{ "UnitStatusEquipmentSeparator", &UnitStatusEquipmentSeparator, sizeof(UnitStatusEquipmentSeparator), 1 },
	{ "ResourceFiles", &ResourceFiles, sizeof(ResourceFiles), 0 },
	{ "CachedFileDescriptors", &CachedFileDescriptors, sizeof(CachedFileDescriptors), 0 },
	{ "ResourceFileNames", &ResourceFileNames, sizeof(ResourceFileNames), 0 },
	{ "ResourceFileLengths", &ResourceFileLengths, sizeof(ResourceFileLengths), 0 },
	{ "LoadedSceneFile", &LoadedSceneFile, sizeof(LoadedSceneFile), 0 },
	{ "SceneUnitRoutes", &SceneUnitRoutes, sizeof(SceneUnitRoutes), 0 },
	{ "SceneInstructionBaseOffset", &SceneInstructionBaseOffset, sizeof(SceneInstructionBaseOffset), 0 },
	{ "SceneInstructionCursor", &SceneInstructionCursor, sizeof(SceneInstructionCursor), 1 },
	{ "ScenarioResourcePathText", &ScenarioResourcePathText, sizeof(ScenarioResourcePathText), 0 },
	{ "SceneFileTooLargeText", &SceneFileTooLargeText, sizeof(SceneFileTooLargeText), 1 },
	{ "GameOptionLabelTextTable", &GameOptionLabelTextTable, sizeof(GameOptionLabelTextTable), 1 },
	{ "GameOptionToggleTextTable", &GameOptionToggleTextTable, sizeof(GameOptionToggleTextTable), 1 },
	{ "NarrationModeLabelTextTable", &NarrationModeLabelTextTable, sizeof(NarrationModeLabelTextTable), 1 },
	{ "InputDeviceLabelTextTable", &InputDeviceLabelTextTable, sizeof(InputDeviceLabelTextTable), 1 },
	{ "FocusUnitText", &FocusUnitText, sizeof(FocusUnitText), 1 },
	{ "FocusLanceText", &FocusLanceText, sizeof(FocusLanceText), 1 },
	{ "DisabledUnitNarrationTextTable", &DisabledUnitNarrationTextTable, sizeof(DisabledUnitNarrationTextTable), 1 },
	{ "DestroyedUnitNarrationTextTable", &DestroyedUnitNarrationTextTable, sizeof(DestroyedUnitNarrationTextTable), 1 },
	{ "ScenarioEventNarrationTextTable", &ScenarioEventNarrationTextTable, sizeof(ScenarioEventNarrationTextTable), 1 },
	{ "SceneNarrationSeparatorText", &SceneNarrationSeparatorText, sizeof(SceneNarrationSeparatorText), 1 },
	{ "MapWidth", &MapWidth, sizeof(MapWidth), 0 },
	{ "MapHeight", &MapHeight, sizeof(MapHeight), 0 },
	{ "VideoPathUnavailable", &VideoPathUnavailable, sizeof(VideoPathUnavailable), 0 },
	{ "SavedBIOSVideoMode", &SavedBIOSVideoMode, sizeof(SavedBIOSVideoMode), 0 },
	{ "BIOSVideoModeByAdapter", &BIOSVideoModeByAdapter, sizeof(BIOSVideoModeByAdapter), 0 },
	{ "GameInitialized", &GameInitialized, sizeof(GameInitialized), 0 },
	{ "StartupResourceState", &StartupResourceState, sizeof(StartupResourceState), 0 },
	{ "InvalidSaveDialogBacking", &InvalidSaveDialogBacking, sizeof(InvalidSaveDialogBacking), 1 },
	{ "NumericKeypadNavigationEnabled", &NumericKeypadNavigationEnabled, sizeof(NumericKeypadNavigationEnabled), 0 },
	{ "StartupInputMode", &StartupInputMode, sizeof(StartupInputMode), 0 },
	{ "SoundEffectsEnabled", &SoundEffectsEnabled, sizeof(SoundEffectsEnabled), 0 },
	{ "DigitalSoundBusy", &DigitalSoundBusy, sizeof(DigitalSoundBusy), 0 },
	{ "DigitalPlaybackRateHz", &DigitalPlaybackRateHz, sizeof(DigitalPlaybackRateHz), 0 },
	{ "AppliedMusicEnabled", &AppliedMusicEnabled, sizeof(AppliedMusicEnabled), 0 },
	{ "SoundDriverNumericParameter", &SoundDriverNumericParameter, sizeof(SoundDriverNumericParameter), 0 },
	{ "InputDeviceType", &InputDeviceType, sizeof(InputDeviceType), 0 },
	{ "SoundDriverIndex", &SoundDriverIndex, sizeof(SoundDriverIndex), 0 },
	{ "Font6Resource", &Font6Resource, sizeof(Font6Resource), 1 },
	{ "Font8Resource", &Font8Resource, sizeof(Font8Resource), 1 },
	{ "SecondaryScreenBuffer", &SecondaryScreenBuffer, sizeof(SecondaryScreenBuffer), 1 },
	{ "PlanarScreenBuffer", &PlanarScreenBuffer, sizeof(PlanarScreenBuffer), 1 },
	{ "MouseCursorBackingSourceTileX", &MouseCursorBackingSourceTileX, sizeof(MouseCursorBackingSourceTileX), 0 },
	{ "MouseCursorBackingSourcePixelY", &MouseCursorBackingSourcePixelY, sizeof(MouseCursorBackingSourcePixelY), 0 },
	{ "MouseCursorBackingWidthTiles", &MouseCursorBackingWidthTiles, sizeof(MouseCursorBackingWidthTiles), 0 },
	{ "MouseCursorBackingHeightPixels", &MouseCursorBackingHeightPixels, sizeof(MouseCursorBackingHeightPixels), 0 },
	{ "VideoFramebuffer", &VideoFramebuffer, sizeof(VideoFramebuffer), 1 },
	{ "ResourceRuntimeInitialized", &ResourceRuntimeInitialized, sizeof(ResourceRuntimeInitialized), 0 },
	{ "VideoPaletteData", &VideoPaletteData, sizeof(VideoPaletteData), 0 },
	{ "MediaConfigurationFileName", MediaConfigurationFileName, 0, 0 },
	{ "InvalidSaveVersionText", &InvalidSaveVersionText, sizeof(InvalidSaveVersionText), 1 },
	{ "VersionDecimalSeparator", VersionDecimalSeparator, 0, 0 },
	{ "VersionTranslationSource", VersionTranslationSource, 0, 0 },
	{ "VersionTranslationReplacement", VersionTranslationReplacement, 0, 0 },
	{ "MediaConfigAssignmentDelimiter", MediaConfigAssignmentDelimiter, 0, 0 },
	{ "VideoConfigTokenDelimiters", VideoConfigTokenDelimiters, 0, 0 },
	{ "MusicConfigTokenDelimiters", MusicConfigTokenDelimiters, 0, 0 },
	{ "SoundConfigTokenDelimiters", SoundConfigTokenDelimiters, 0, 0 },
	{ "SoundParameterTokenDelimiters", SoundParameterTokenDelimiters, 0, 0 },
	{ "InputConfigTokenDelimiters", InputConfigTokenDelimiters, 0, 0 },
	{ "VideoConfigurationRecords", &VideoConfigurationRecords, sizeof(VideoConfigurationRecords), 0 },
	{ "MusicConfigurationRecords", &MusicConfigurationRecords, sizeof(MusicConfigurationRecords), 0 },
	{ "InputConfigurationRecords", &InputConfigurationRecords, sizeof(InputConfigurationRecords), 0 },
	{ "SoundConfigurationRecords", &SoundConfigurationRecords, sizeof(SoundConfigurationRecords), 0 },
	{ "CompressedTextFirstByteTable", &CompressedTextFirstByteTable, sizeof(CompressedTextFirstByteTable), 0 },
	{ "CompressedTextSecondByteTable", &CompressedTextSecondByteTable, sizeof(CompressedTextSecondByteTable), 0 },
	{ "CgaNarrowStrideMode", &CgaNarrowStrideMode, sizeof(CgaNarrowStrideMode), 0 },
	{ "UiSelectionResetFlagA", &UiSelectionResetFlagA, sizeof(UiSelectionResetFlagA), 0 },
	{ "UiSelectionResetFlagB", &UiSelectionResetFlagB, sizeof(UiSelectionResetFlagB), 0 },
	{ "ActiveTextWindowLeftTile", &ActiveTextWindowLeftTile, sizeof(ActiveTextWindowLeftTile), 0 },
	{ "ActiveTextWindowTopTile", &ActiveTextWindowTopTile, sizeof(ActiveTextWindowTopTile), 0 },
	{ "TextWindowContexts", &TextWindowContexts, sizeof(TextWindowContexts), 0 },
	{ "ActiveTextWindowIndex", &ActiveTextWindowIndex, sizeof(ActiveTextWindowIndex), 0 },
	{ "ActiveTextWindowWidthTiles", &ActiveTextWindowWidthTiles, sizeof(ActiveTextWindowWidthTiles), 0 },
	{ "ActiveTextWindowHeightTiles", &ActiveTextWindowHeightTiles, sizeof(ActiveTextWindowHeightTiles), 0 },
	{ "ActiveTextForegroundColor", &ActiveTextForegroundColor, sizeof(ActiveTextForegroundColor), 0 },
	{ "ActiveTextBackgroundColor", &ActiveTextBackgroundColor, sizeof(ActiveTextBackgroundColor), 0 },
	{ "ActiveTextCursorColumn", &ActiveTextCursorColumn, sizeof(ActiveTextCursorColumn), 0 },
	{ "ActiveTextCursorRow", &ActiveTextCursorRow, sizeof(ActiveTextCursorRow), 0 },
	{ "TextWindowStateInitialized", &TextWindowStateInitialized, sizeof(TextWindowStateInitialized), 0 },
	{ "UiNavigationUpKey", &UiNavigationUpKey, sizeof(UiNavigationUpKey), 0 },
	{ "UiNavigationDownKey", &UiNavigationDownKey, sizeof(UiNavigationDownKey), 0 },
	{ "UiPreviousRegionKey", &UiPreviousRegionKey, sizeof(UiPreviousRegionKey), 0 },
	{ "UiNextRegionKey", &UiNextRegionKey, sizeof(UiNextRegionKey), 0 },
	{ "PITCalibration", &PITCalibration, sizeof(PITCalibration), 0 },
	{ "VerticalRetraceEdgeMode", &VerticalRetraceEdgeMode, sizeof(VerticalRetraceEdgeMode), 0 },
	{ "DamagedTerrainSpriteLookup", &DamagedTerrainSpriteLookup, sizeof(DamagedTerrainSpriteLookup), 1 },
	{ "TacticalEffectOverlayMap", &TacticalEffectOverlayMap, sizeof(TacticalEffectOverlayMap), 0 },
	{ "GameTimerTicks", &GameTimerTicks, sizeof(GameTimerTicks), 0 },
	{ "GameAnimationTicks", &GameAnimationTicks, sizeof(GameAnimationTicks), 0 },
	{ "CampaignClockSeparatorText", CampaignClockSeparatorText, 0, 0 },
	{ "FormattedLineBreak", FormattedLineBreak, 0, 0 },
	{ "PathSeparatorText", PathSeparatorText, 0, 0 },
	{ "PercentCloseText", PercentCloseText, 0, 0 },
	{ "TacticalSlotSpriteState", &TacticalSlotSpriteState, sizeof(TacticalSlotSpriteState), 0 },
	{ "UnitTypeResourceCode", &UnitTypeResourceCode, sizeof(UnitTypeResourceCode), 0 },
	{ "UnitTacticalOrderMode", &UnitTacticalOrderMode, sizeof(UnitTacticalOrderMode), 0 },
	{ "UnitAttackTarget", &UnitAttackTarget, sizeof(UnitAttackTarget), 0 },
	{ "UnitSpriteBaseId", &UnitSpriteBaseId, sizeof(UnitSpriteBaseId), 0 },
	{ "UnitSpriteId", &UnitSpriteId, sizeof(UnitSpriteId), 0 },
	{ "TacticalSpriteAnimationPhase", &TacticalSpriteAnimationPhase, sizeof(TacticalSpriteAnimationPhase), 0 },
	{ "TacticalSelectedUnitIndex", &TacticalSelectedUnitIndex, sizeof(TacticalSelectedUnitIndex), 0 },
	{ "TacticalViewportMapX", &TacticalViewportMapX, sizeof(TacticalViewportMapX), 0 },
	{ "TacticalViewportMapY", &TacticalViewportMapY, sizeof(TacticalViewportMapY), 0 },
	{ "TacticalCursorViewportX", &TacticalCursorViewportX, sizeof(TacticalCursorViewportX), 0 },
	{ "TacticalCursorViewportY", &TacticalCursorViewportY, sizeof(TacticalCursorViewportY), 0 },
	{ "UnitTargetMapX", &UnitTargetMapX, sizeof(UnitTargetMapX), 0 },
	{ "UnitTargetMapY", &UnitTargetMapY, sizeof(UnitTargetMapY), 0 },
	{ "UnitRouteTargetMapX", &UnitRouteTargetMapX, sizeof(UnitRouteTargetMapX), 0 },
	{ "UnitRouteTargetMapY", &UnitRouteTargetMapY, sizeof(UnitRouteTargetMapY), 0 },
	{ "SavedTacticalRoute", &SavedTacticalRoute, sizeof(SavedTacticalRoute), 0 },
	{ "TacticalInteractionLocked", &TacticalInteractionLocked, sizeof(TacticalInteractionLocked), 0 },
	{ "TacticalMenuInputState", &TacticalMenuInputState, sizeof(TacticalMenuInputState), 0 },
	{ "TacticalPaddingSpace", TacticalPaddingSpace, 0, 0 },
	{ "TacticalStatusSeparator", TacticalStatusSeparator, 0, 0 },
	{ "TacticalStatusPadding", TacticalStatusPadding, 0, 0 },
	{ "UnitStatusArmorLabel", &UnitStatusArmorLabel, sizeof(UnitStatusArmorLabel), 1 },
	{ "UnitStatusHeatLabel", &UnitStatusHeatLabel, sizeof(UnitStatusHeatLabel), 1 },
	{ "UnitStatusExitLabel", &UnitStatusExitLabel, sizeof(UnitStatusExitLabel), 1 },
	{ "UnitStatusHeatDetailLabel", &UnitStatusHeatDetailLabel, sizeof(UnitStatusHeatDetailLabel), 1 },
	{ "UnitStatusDamageLabel", &UnitStatusDamageLabel, sizeof(UnitStatusDamageLabel), 1 },
	{ "UnitStatusRatingLabels", &UnitStatusRatingLabels, sizeof(UnitStatusRatingLabels), 1 },
	{ "IntelligenceHeading", &IntelligenceHeading, sizeof(IntelligenceHeading), 1 },
	{ "ScannedByActiveProbeText", &ScannedByActiveProbeText, sizeof(ScannedByActiveProbeText), 1 },
	{ "DetectedByText", &DetectedByText, sizeof(DetectedByText), 1 },
	{ "RadarSensorText", &RadarSensorText, sizeof(RadarSensorText), 1 },
	{ "EnemyAndFriendlyLancesText", &EnemyAndFriendlyLancesText, sizeof(EnemyAndFriendlyLancesText), 1 },
	{ "UnknownUnitText", &UnknownUnitText, sizeof(UnknownUnitText), 1 },
	{ "NoSatelliteUplinkText", &NoSatelliteUplinkText, sizeof(NoSatelliteUplinkText), 1 },
	{ "UplinkJammedText", &UplinkJammedText, sizeof(UplinkJammedText), 1 },
	{ "OutOfScanningText", &OutOfScanningText, sizeof(OutOfScanningText), 1 },
	{ "RangeText", &RangeText, sizeof(RangeText), 1 },
	{ "ArrowTagEquipmentText", &ArrowTagEquipmentText, sizeof(ArrowTagEquipmentText), 1 },
	{ "BapEquipmentText", &BapEquipmentText, sizeof(BapEquipmentText), 1 },
	{ "AccuracyEquipmentText", &AccuracyEquipmentText, sizeof(AccuracyEquipmentText), 1 },
	{ "DoubleHeatSinkEquipmentText", &DoubleHeatSinkEquipmentText, sizeof(DoubleHeatSinkEquipmentText), 1 },
	{ "JumpEquipmentSuffix", &JumpEquipmentSuffix, sizeof(JumpEquipmentSuffix), 1 },
	{ "CaseEquipmentText", &CaseEquipmentText, sizeof(CaseEquipmentText), 1 },
	{ "MoveDestinationPromptSuffix", &MoveDestinationPromptSuffix, sizeof(MoveDestinationPromptSuffix), 1 },
	{ "FormationHeading", &FormationHeading, sizeof(FormationHeading), 1 },
	{ "SatelliteZoomText", &SatelliteZoomText, sizeof(SatelliteZoomText), 1 },
	{ "ContinueText", &ContinueText, sizeof(ContinueText), 1 },
	{ "DoneText", &DoneText, sizeof(DoneText), 1 },
	{ "NoTacticalTargetText", NoTacticalTargetText, 0, 0 },
	{ "ViewSatelliteMapText", &ViewSatelliteMapText, sizeof(ViewSatelliteMapText), 1 },
	{ "SelectStrafingAreaPrompt", &SelectStrafingAreaPrompt, sizeof(SelectStrafingAreaPrompt), 1 },
	{ "SelectArtilleryTargetPrompt", &SelectArtilleryTargetPrompt, sizeof(SelectArtilleryTargetPrompt), 1 },
	{ "SelectBarrageStartPrompt", &SelectBarrageStartPrompt, sizeof(SelectBarrageStartPrompt), 1 },
	{ "SelectBarrageEndPrompt", &SelectBarrageEndPrompt, sizeof(SelectBarrageEndPrompt), 1 },
	{ "SelectFireTargetPrompt", &SelectFireTargetPrompt, sizeof(SelectFireTargetPrompt), 1 },
	{ "TacticalUnitCommandTextTable", &TacticalUnitCommandTextTable, sizeof(TacticalUnitCommandTextTable), 1 },
	{ "TacticalSupportCommandTextTable", &TacticalSupportCommandTextTable, sizeof(TacticalSupportCommandTextTable), 1 },
	{ "TacticalOverburnTextTable", &TacticalOverburnTextTable, sizeof(TacticalOverburnTextTable), 1 },
	{ "LanceStatusTextTable", &LanceStatusTextTable, sizeof(LanceStatusTextTable), 1 },
	{ "DeadText", &DeadText, sizeof(DeadText), 1 },
	{ "MapEdgeEffectQueue", &MapEdgeEffectQueue, sizeof(MapEdgeEffectQueue), 0 },
	{ "MapEdgeEffectCount", &MapEdgeEffectCount, sizeof(MapEdgeEffectCount), 0 },
	{ "PrimaryStatusRow", &PrimaryStatusRow, sizeof(PrimaryStatusRow), 0 },
	{ "SecondaryStatusRow", &SecondaryStatusRow, sizeof(SecondaryStatusRow), 0 },
	{ "TacticalUiSuppressed", &TacticalUiSuppressed, sizeof(TacticalUiSuppressed), 0 },
	{ "PrimaryStatusHighlightColor", &PrimaryStatusHighlightColor, sizeof(PrimaryStatusHighlightColor), 0 },
	{ "CachedPrimaryStatusHighlightColor", &CachedPrimaryStatusHighlightColor, sizeof(CachedPrimaryStatusHighlightColor), 0 },
	{ "SecondaryStatusHighlightColor", &SecondaryStatusHighlightColor, sizeof(SecondaryStatusHighlightColor), 0 },
	{ "CachedSecondaryStatusHighlightColor", &CachedSecondaryStatusHighlightColor, sizeof(CachedSecondaryStatusHighlightColor), 0 },
	{ "TacticalStatusRowInvalidation", &TacticalStatusRowInvalidation, sizeof(TacticalStatusRowInvalidation), 0 },
	{ "TacticalOverviewMarkerX", &TacticalOverviewMarkerX, sizeof(TacticalOverviewMarkerX), 0 },
	{ "TacticalOverviewViewportX", &TacticalOverviewViewportX, sizeof(TacticalOverviewViewportX), 0 },
	{ "TacticalOverviewMarkerY", &TacticalOverviewMarkerY, sizeof(TacticalOverviewMarkerY), 0 },
	{ "TacticalOverviewViewportY", &TacticalOverviewViewportY, sizeof(TacticalOverviewViewportY), 0 },
	{ "TacticalOverviewScreenOriginX", &TacticalOverviewScreenOriginX, sizeof(TacticalOverviewScreenOriginX), 0 },
	{ "DisplayedTacticalStatusRow", &DisplayedTacticalStatusRow, sizeof(DisplayedTacticalStatusRow), 0 },
	{ "TacticalStatusBlinkCountdown", &TacticalStatusBlinkCountdown, sizeof(TacticalStatusBlinkCountdown), 0 },
	{ "TacticalStatusBlinkVisible", &TacticalStatusBlinkVisible, sizeof(TacticalStatusBlinkVisible), 0 },
	{ "WeaponTraceMajorStepX", &WeaponTraceMajorStepX, sizeof(WeaponTraceMajorStepX), 0 },
	{ "WeaponTraceMajorStepY", &WeaponTraceMajorStepY, sizeof(WeaponTraceMajorStepY), 0 },
	{ "WeaponTraceMinorStepX", &WeaponTraceMinorStepX, sizeof(WeaponTraceMinorStepX), 0 },
	{ "WeaponTraceMinorStepY", &WeaponTraceMinorStepY, sizeof(WeaponTraceMinorStepY), 0 },
	{ "TacticalNoUnitStatusText", TacticalNoUnitStatusText, 0, 0 },
	{ "FriendlyUnitStatusHeading", &FriendlyUnitStatusHeading, sizeof(FriendlyUnitStatusHeading), 1 },
	{ "EnemyUnitStatusHeading", EnemyUnitStatusHeading, 0, 0 },
	{ "UnitStatusSeparatorText", UnitStatusSeparatorText, 0, 0 },
	{ "UnitHeatStatusPrefix", UnitHeatStatusPrefix, 0, 0 },
	{ "TacticalNarrationBlankText", TacticalNarrationBlankText, 0, 0 },
	{ "TacticalClockBlankText", TacticalClockBlankText, 0, 0 },
	{ "TacticalNarrationLabel", &TacticalNarrationLabel, sizeof(TacticalNarrationLabel), 1 },
	{ "CombatRatingLightText", &CombatRatingLightText, sizeof(CombatRatingLightText), 1 },
	{ "CombatRatingModerateText", &CombatRatingModerateText, sizeof(CombatRatingModerateText), 1 },
	{ "CombatRatingHeavyText", &CombatRatingHeavyText, sizeof(CombatRatingHeavyText), 1 },
	{ "CombatRatingBrutalText", &CombatRatingBrutalText, sizeof(CombatRatingBrutalText), 1 },
	{ "CombatRatingDevastatingText", &CombatRatingDevastatingText, sizeof(CombatRatingDevastatingText), 1 },
	{ "CombatRatingIncredibleText", &CombatRatingIncredibleText, sizeof(CombatRatingIncredibleText), 1 },
	{ "CombatRatingNoneText", &CombatRatingNoneText, sizeof(CombatRatingNoneText), 1 },
	{ "CombatRatingDescriptionTable", &CombatRatingDescriptionTable, sizeof(CombatRatingDescriptionTable), 1 },
	{ "PersistentUnitRoster", &PersistentUnitRoster, sizeof(PersistentUnitRoster), 0 },
	{ "LiveScenarioUnits", &LiveScenarioUnits, sizeof(LiveScenarioUnits), 0 },
	{ "BlankUnitRecord", &BlankUnitRecord, sizeof(BlankUnitRecord), 0 },
	{ "ScenarioUnitMapX", &ScenarioUnitMapX, sizeof(ScenarioUnitMapX), 0 },
	{ "ScenarioUnitMapY", &ScenarioUnitMapY, sizeof(ScenarioUnitMapY), 0 },
	{ "ScenarioUnitFacing", &ScenarioUnitFacing, sizeof(ScenarioUnitFacing), 0 },
	{ "CampaignClockState", &CampaignClockState, sizeof(CampaignClockState), 0 },
	{ "SavedCampaignClockState", &SavedCampaignClockState, sizeof(SavedCampaignClockState), 0 },
	{ "PilotRoster", &PilotRoster, sizeof(PilotRoster), 0 },
	{ "PilotRosterRecordPastEnd", &PilotRosterRecordPastEnd, sizeof(PilotRosterRecordPastEnd), 0 },
	{ "LanceAssignments", &LanceAssignments, sizeof(LanceAssignments), 0 },
	{ "ScenarioLanceAllegianceTable", &ScenarioLanceAllegianceTable, sizeof(ScenarioLanceAllegianceTable), 0 },
	{ "AmmoCapacityMultiplierTable", &AmmoCapacityMultiplierTable, sizeof(AmmoCapacityMultiplierTable), 0 },
	{ "CampaignFlags", &CampaignFlags, sizeof(CampaignFlags), 0 },
	{ "CampaignStageIndex", &CampaignStageIndex, sizeof(CampaignStageIndex), 0 },
	{ "ScenarioUnitZeroAvailable", &ScenarioUnitZeroAvailable, sizeof(ScenarioUnitZeroAvailable), 0 },
	{ "ScenarioUnitFifteenState", &ScenarioUnitFifteenState, sizeof(ScenarioUnitFifteenState), 0 },
	{ "ScenarioUnitFourteenEnabled", &ScenarioUnitFourteenEnabled, sizeof(ScenarioUnitFourteenEnabled), 0 },
	{ "EjectedPilotRosterState", &EjectedPilotRosterState, sizeof(EjectedPilotRosterState), 0 },
	{ "ScenarioUnitThirteenEnabled", &ScenarioUnitThirteenEnabled, sizeof(ScenarioUnitThirteenEnabled), 0 },
	{ "StoryUnitDeployedFlag", &StoryUnitDeployedFlag, sizeof(StoryUnitDeployedFlag), 0 },
	{ "SelectedSaveSlotIndex", &SelectedSaveSlotIndex, sizeof(SelectedSaveSlotIndex), 0 },
	{ "SaveLoadMenuDefaultSelection", &SaveLoadMenuDefaultSelection, sizeof(SaveLoadMenuDefaultSelection), 0 },
	{ "ScenarioRosterPrepared", &ScenarioRosterPrepared, sizeof(ScenarioRosterPrepared), 0 },
	{ "PendingCampaignStageDelta", &PendingCampaignStageDelta, sizeof(PendingCampaignStageDelta), 0 },
	{ "PendingCampaignPhaseDelta", &PendingCampaignPhaseDelta, sizeof(PendingCampaignPhaseDelta), 0 },
	{ "ScenarioVariantIndex", &ScenarioVariantIndex, sizeof(ScenarioVariantIndex), 0 },
	{ "CampaignPhase", &CampaignPhase, sizeof(CampaignPhase), 0 },
	{ "ReplayCampaignStateRequested", &ReplayCampaignStateRequested, sizeof(ReplayCampaignStateRequested), 0 },
	{ "TrainingSequenceFlag", &TrainingSequenceFlag, sizeof(TrainingSequenceFlag), 0 },
	{ "DeferredStage16Story", &DeferredStage16Story, sizeof(DeferredStage16Story), 0 },
	{ "SavedCampaignFlags", &SavedCampaignFlags, sizeof(SavedCampaignFlags), 0 },
	{ "AlternateLanceSelection", &AlternateLanceSelection, sizeof(AlternateLanceSelection), 0 },
	{ "SuppressPostStageMenu", &SuppressPostStageMenu, sizeof(SuppressPostStageMenu), 0 },
	{ "CampaignRestartState", &CampaignRestartState, sizeof(CampaignRestartState), 0 },
	{ "ModalSavedTextWindow", &ModalSavedTextWindow, sizeof(ModalSavedTextWindow), 0 },
	{ "ModalBackingX", &ModalBackingX, sizeof(ModalBackingX), 0 },
	{ "ModalBackingY", &ModalBackingY, sizeof(ModalBackingY), 0 },
	{ "ModalBackingWidth", &ModalBackingWidth, sizeof(ModalBackingWidth), 0 },
	{ "ModalBackingHeight", &ModalBackingHeight, sizeof(ModalBackingHeight), 0 },
	{ "ActivePilotListWindowHeight", &ActivePilotListWindowHeight, sizeof(ActivePilotListWindowHeight), 0 },
	{ "CommanderListWindowHeight", &CommanderListWindowHeight, sizeof(CommanderListWindowHeight), 0 },
	{ "FallenPilotListWindowHeight", &FallenPilotListWindowHeight, sizeof(FallenPilotListWindowHeight), 0 },
	{ "RandomPilotPoolCounts", &RandomPilotPoolCounts, sizeof(RandomPilotPoolCounts), 0 },
	{ "SaveTransferState", &SaveTransferState, sizeof(SaveTransferState), 0 },
	{ "SavedSaveTransferState", &SavedSaveTransferState, sizeof(SavedSaveTransferState), 0 },
	{ "CampaignScenarioState", &CampaignScenarioState, sizeof(CampaignScenarioState), 0 },
	{ "ReplayScenarioRequested", &ReplayScenarioRequested, sizeof(ReplayScenarioRequested), 0 },
	{ "CampaignMenuRequested", &CampaignMenuRequested, sizeof(CampaignMenuRequested), 0 },
	{ "NarrationFocusRegion", &NarrationFocusRegion, sizeof(NarrationFocusRegion), 0 },
	{ "TacticalOverviewClockNoticeActive", &TacticalOverviewClockNoticeActive, sizeof(TacticalOverviewClockNoticeActive), 0 },
	{ "TacticalRuntimeState", &TacticalRuntimeState, sizeof(TacticalRuntimeState), 0 },
	{ "PreviousMouseCellX", &PreviousMouseCellX, sizeof(PreviousMouseCellX), 0 },
	{ "PreviousMouseCellY", &PreviousMouseCellY, sizeof(PreviousMouseCellY), 0 },
	{ "TacticalOverviewNoticeChance", &TacticalOverviewNoticeChance, sizeof(TacticalOverviewNoticeChance), 0 },
	{ "EjectedPilotMapX", &EjectedPilotMapX, sizeof(EjectedPilotMapX), 0 },
	{ "EjectedPilotMapY", &EjectedPilotMapY, sizeof(EjectedPilotMapY), 0 },
	{ "EjectedPilotState", &EjectedPilotState, sizeof(EjectedPilotState), 0 },
	{ "TacticalCursorRuntimeState", &TacticalCursorRuntimeState, sizeof(TacticalCursorRuntimeState), 0 },
	{ "TacticalCursorMapX", &TacticalCursorMapX, sizeof(TacticalCursorMapX), 0 },
	{ "TacticalCursorMapY", &TacticalCursorMapY, sizeof(TacticalCursorMapY), 0 },
	{ "LastOverviewNoticeHour", &LastOverviewNoticeHour, sizeof(LastOverviewNoticeHour), 0 },
	{ "TacticalOverviewRuntimeState", &TacticalOverviewRuntimeState, sizeof(TacticalOverviewRuntimeState), 0 },
	{ "PrimaryLanceResourceCode", &PrimaryLanceResourceCode, sizeof(PrimaryLanceResourceCode), 0 },
	{ "UnitEditorAllegiance", &UnitEditorAllegiance, sizeof(UnitEditorAllegiance), 0 },
	{ "UnitEditorScenarioGroupCount", &UnitEditorScenarioGroupCount, sizeof(UnitEditorScenarioGroupCount), 0 },
	{ "UnitEditorScenarioCardCount", &UnitEditorScenarioCardCount, sizeof(UnitEditorScenarioCardCount), 0 },
	{ "UnitEditorPilotCardCount", &UnitEditorPilotCardCount, sizeof(UnitEditorPilotCardCount), 0 },
	{ "UnitEditorLanceGroupCount", &UnitEditorLanceGroupCount, sizeof(UnitEditorLanceGroupCount), 0 },
	{ "TextOutputMode", &TextOutputMode, sizeof(TextOutputMode), 0 },
	{ "SelectedUnitEquipmentLocation", &SelectedUnitEquipmentLocation, sizeof(SelectedUnitEquipmentLocation), 0 },
	{ "EstimatedUnitRecord", &EstimatedUnitRecord, sizeof(EstimatedUnitRecord), 0 },
	{ "UnitCombatRatings", &UnitCombatRatings, sizeof(UnitCombatRatings), 0 },
	{ "UnitLanceFormationState", &UnitLanceFormationState, sizeof(UnitLanceFormationState), 0 },
	{ "TacticalScenarioLanceEnabled", &TacticalScenarioLanceEnabled, sizeof(TacticalScenarioLanceEnabled), 0 },
	{ "SensorDamageProximityRangeTable", &SensorDamageProximityRangeTable, sizeof(SensorDamageProximityRangeTable), 0 },
	{ "LanceFormationProfile", &LanceFormationProfile, sizeof(LanceFormationProfile), 0 },
	{ "UnitEnemyEngagementActive", &UnitEnemyEngagementActive, sizeof(UnitEnemyEngagementActive), 0 },
	{ "UnitMovementHeatExponent", &UnitMovementHeatExponent, sizeof(UnitMovementHeatExponent), 0 },
	{ "UnitOverburnOrCloseAssaultEnabled", &UnitOverburnOrCloseAssaultEnabled, sizeof(UnitOverburnOrCloseAssaultEnabled), 0 },
	{ "UnitFormationMode", &UnitFormationMode, sizeof(UnitFormationMode), 0 },
	{ "UnitConsecutiveAttackTicks", &UnitConsecutiveAttackTicks, sizeof(UnitConsecutiveAttackTicks), 0 },
	{ "FirstLanceOrdersEnabled", &FirstLanceOrdersEnabled, sizeof(FirstLanceOrdersEnabled), 0 },
	{ "UnitWasAttackedThisTick", &UnitWasAttackedThisTick, sizeof(UnitWasAttackedThisTick), 0 },
	{ "GroundTargetWasAttacked", &GroundTargetWasAttacked, sizeof(GroundTargetWasAttacked), 0 },
	{ "UnitJumpingState", &UnitJumpingState, sizeof(UnitJumpingState), 0 },
	{ "UnitJumpMovementRemaining", &UnitJumpMovementRemaining, sizeof(UnitJumpMovementRemaining), 0 },
	{ "UnitDestructionMarkers", &UnitDestructionMarkers, sizeof(UnitDestructionMarkers), 0 },
	{ "ScenarioFileSuffix", &ScenarioFileSuffix, sizeof(ScenarioFileSuffix), 0 },
	{ "CampaignUpgradeAwards", &CampaignUpgradeAwards, sizeof(CampaignUpgradeAwards), 0 },
	{ "UnitEnemyInProximityRange", &UnitEnemyInProximityRange, sizeof(UnitEnemyInProximityRange), 0 },
	{ "UnitShutdownState", &UnitShutdownState, sizeof(UnitShutdownState), 0 },
	{ "MovementRatingByWalkMP", &MovementRatingByWalkMP, sizeof(MovementRatingByWalkMP), 0 },
	{ "UnitNameTable", &UnitNameTable, sizeof(UnitNameTable), 1 },
	{ "PilotSurnameTable", &PilotSurnameTable, sizeof(PilotSurnameTable), 1 },
	{ "UnitAffiliationNameTable", &UnitAffiliationNameTable, sizeof(UnitAffiliationNameTable), 1 },
	{ "PilotExperienceNameTable", &PilotExperienceNameTable, sizeof(PilotExperienceNameTable), 1 },
	{ "HeatScaleDescriptionTable", &HeatScaleDescriptionTable, sizeof(HeatScaleDescriptionTable), 1 },
	{ "MovementDescriptionTable", &MovementDescriptionTable, sizeof(MovementDescriptionTable), 1 },
	{ "DurabilityDescriptionTable", &DurabilityDescriptionTable, sizeof(DurabilityDescriptionTable), 1 },
	{ "ConditionDescriptionTable", &ConditionDescriptionTable, sizeof(ConditionDescriptionTable), 1 },
	{ "FriendlyEquipmentDescriptionTable", &FriendlyEquipmentDescriptionTable, sizeof(FriendlyEquipmentDescriptionTable), 1 },
	{ "EnemyAssessmentTextTable", &EnemyAssessmentTextTable, sizeof(EnemyAssessmentTextTable), 1 },
	{ "UnitEnemyVisible", &UnitEnemyVisible, sizeof(UnitEnemyVisible), 0 },
	{ "UnitTacticalTarget", &UnitTacticalTarget, sizeof(UnitTacticalTarget), 0 },
	{ "EnemyCycleUnitIndex", &EnemyCycleUnitIndex, sizeof(EnemyCycleUnitIndex), 0 },
	{ "UnitEnemyContactEstablished", &UnitEnemyContactEstablished, sizeof(UnitEnemyContactEstablished), 0 },
	{ "UnitWithdrawalState", &UnitWithdrawalState, sizeof(UnitWithdrawalState), 0 },
	{ "UnitLastNarratedTarget", &UnitLastNarratedTarget, sizeof(UnitLastNarratedTarget), 0 },
	{ "UnitPreferredEngagementMode", &UnitPreferredEngagementMode, sizeof(UnitPreferredEngagementMode), 0 },
	{ "UnitTerrainType", &UnitTerrainType, sizeof(UnitTerrainType), 0 },
	{ "UnitBelowHalfDurability", &UnitBelowHalfDurability, sizeof(UnitBelowHalfDurability), 0 },
	{ "UnitCachedWeaponCounts", &UnitCachedWeaponCounts, sizeof(UnitCachedWeaponCounts), 0 },
	{ "UnitWeaponRecoveryCounters", &UnitWeaponRecoveryCounters, sizeof(UnitWeaponRecoveryCounters), 0 },
	{ "UnitHeatSinkCount", &UnitHeatSinkCount, sizeof(UnitHeatSinkCount), 0 },
	{ "UnitWeaponAvailabilityMask", &UnitWeaponAvailabilityMask, sizeof(UnitWeaponAvailabilityMask), 0 },
	{ "WeaponCountScratch", &WeaponCountScratch, sizeof(WeaponCountScratch), 0 },
	{ "TerrainTypeMap", &TerrainTypeMap, sizeof(TerrainTypeMap), 1 },
	{ "TacticalOverviewMapBuffer", &TacticalOverviewMapBuffer, sizeof(TacticalOverviewMapBuffer), 1 },
	{ "PackedTerrainDamageMap", &PackedTerrainDamageMap, sizeof(PackedTerrainDamageMap), 1 },
	{ "ActiveTerrainDamageByte", &ActiveTerrainDamageByte, sizeof(ActiveTerrainDamageByte), 1 },
	{ "TerrainDamageCapabilityTable", &TerrainDamageCapabilityTable, sizeof(TerrainDamageCapabilityTable), 1 },
	{ "TerrainDamageCellIncrementMask", &TerrainDamageCellIncrementMask, sizeof(TerrainDamageCellIncrementMask), 0 },
	{ "TerrainDamageExtractMask", &TerrainDamageExtractMask, sizeof(TerrainDamageExtractMask), 0 },
	{ "TerrainDamageShift", &TerrainDamageShift, sizeof(TerrainDamageShift), 0 },
	{ "TerrainDamageCellOccupiedMask", &TerrainDamageCellOccupiedMask, sizeof(TerrainDamageCellOccupiedMask), 0 },
	{ "TerrainDamageStageIncrement", &TerrainDamageStageIncrement, sizeof(TerrainDamageStageIncrement), 0 },
	{ "PackedTerrainDamageRowBytes", &PackedTerrainDamageRowBytes, sizeof(PackedTerrainDamageRowBytes), 0 },
	{ "DestructibleTerrainTypeBase", &DestructibleTerrainTypeBase, sizeof(DestructibleTerrainTypeBase), 0 },
	{ "TerrainImpactAnimationTypeMinimum", &TerrainImpactAnimationTypeMinimum, sizeof(TerrainImpactAnimationTypeMinimum), 0 },
	{ "TerrainImpactAnimationTypeMaximum", &TerrainImpactAnimationTypeMaximum, sizeof(TerrainImpactAnimationTypeMaximum), 0 },
	{ "TerrainLineOfSightBlockingThreshold", &TerrainLineOfSightBlockingThreshold, sizeof(TerrainLineOfSightBlockingThreshold), 0 },
	{ "TerrainLineOfSightMaximumSteps", &TerrainLineOfSightMaximumSteps, sizeof(TerrainLineOfSightMaximumSteps), 0 },
	{ "TacticalTerrainEffectMode", &TacticalTerrainEffectMode, sizeof(TacticalTerrainEffectMode), 0 },
	{ "TerrainDamageQueue", &TerrainDamageQueue, sizeof(TerrainDamageQueue), 0 },
	{ "CampaignUpgradeMarkerX", &CampaignUpgradeMarkerX, sizeof(CampaignUpgradeMarkerX), 0 },
	{ "CampaignUpgradeMarkerY", &CampaignUpgradeMarkerY, sizeof(CampaignUpgradeMarkerY), 0 },
	{ "FriendlyUnitDestroyed", &FriendlyUnitDestroyed, sizeof(FriendlyUnitDestroyed), 0 },
	{ "ScenarioDestructionCompletionState", &ScenarioDestructionCompletionState, sizeof(ScenarioDestructionCompletionState), 0 },
	{ "ScenarioDestructionCompletionMarker", &ScenarioDestructionCompletionMarker, sizeof(ScenarioDestructionCompletionMarker), 0 },
	{ "ProtectUnitZero", &ProtectUnitZero, sizeof(ProtectUnitZero), 0 },
	{ "ImpassableTerrainTypeMinimum", &ImpassableTerrainTypeMinimum, sizeof(ImpassableTerrainTypeMinimum), 0 },
	{ "TerrainToHitModifier", &TerrainToHitModifier, sizeof(TerrainToHitModifier), 0 },
	{ "WeaponAIHeatLimitByExperience", &WeaponAIHeatLimitByExperience, sizeof(WeaponAIHeatLimitByExperience), 0 },
	{ "SpriteFacingByDirection", &SpriteFacingByDirection, sizeof(SpriteFacingByDirection), 0 },
	{ "EngagementModeByComparisonMask", &EngagementModeByComparisonMask, sizeof(EngagementModeByComparisonMask), 0 },
	{ "WeaponPriorityData", &WeaponPriorityData, sizeof(WeaponPriorityData), 0 },
	{ "WeaponPriorityCountTable", &WeaponPriorityCountTable, sizeof(WeaponPriorityCountTable), 0 },
	{ "WeaponPriorityListPointers", &WeaponPriorityListPointers, sizeof(WeaponPriorityListPointers), 0 },
	{ "WeaponPriorityProfileByRangeHeat", &WeaponPriorityProfileByRangeHeat, sizeof(WeaponPriorityProfileByRangeHeat), 0 },
	{ "MovementTimingMaskTable", &MovementTimingMaskTable, sizeof(MovementTimingMaskTable), 0 },
	{ "FormationOffsetTable", &FormationOffsetTable, sizeof(FormationOffsetTable), 0 },
	{ "FormationSelectionAggressive", &FormationSelectionAggressive, sizeof(FormationSelectionAggressive), 0 },
	{ "FormationSelectionDefensive", &FormationSelectionDefensive, sizeof(FormationSelectionDefensive), 0 },
	{ "FormationSelectionBalanced", &FormationSelectionBalanced, sizeof(FormationSelectionBalanced), 0 },
	{ "FormationMarkerOriginX", &FormationMarkerOriginX, sizeof(FormationMarkerOriginX), 0 },
	{ "FormationMarkerOriginY", &FormationMarkerOriginY, sizeof(FormationMarkerOriginY), 0 },
	{ "FacingVectorStartX", &FacingVectorStartX, sizeof(FacingVectorStartX), 0 },
	{ "FacingVectorStartY", &FacingVectorStartY, sizeof(FacingVectorStartY), 0 },
	{ "FacingVectorEndX", &FacingVectorEndX, sizeof(FacingVectorEndX), 0 },
	{ "FacingVectorEndY", &FacingVectorEndY, sizeof(FacingVectorEndY), 0 },
	{ "MovementVectorX", &MovementVectorX, sizeof(MovementVectorX), 0 },
	{ "MovementVectorY", &MovementVectorY, sizeof(MovementVectorY), 0 },
	{ "DirectionDeltaXTable", &DirectionDeltaXTable, sizeof(DirectionDeltaXTable), 0 },
	{ "DirectionDeltaYTable", &DirectionDeltaYTable, sizeof(DirectionDeltaYTable), 0 },
	{ "NeighborMapOffsetTable", &NeighborMapOffsetTable, sizeof(NeighborMapOffsetTable), 0 },
	{ "DirectionSearchMirror", &DirectionSearchMirror, sizeof(DirectionSearchMirror), 0 },
	{ "DirectionSearchCountdown", &DirectionSearchCountdown, sizeof(DirectionSearchCountdown), 0 },
	{ "DirectionSearchDeltaTable", &DirectionSearchDeltaTable, sizeof(DirectionSearchDeltaTable), 0 },
	{ "ImprovedMissileDamageBonusTable", &ImprovedMissileDamageBonusTable, sizeof(ImprovedMissileDamageBonusTable), 0 },
	{ "TerrainDamageQueueOccupiedMask", &TerrainDamageQueueOccupiedMask, sizeof(TerrainDamageQueueOccupiedMask), 0 },
	{ "TerrainDamageQueueIncrementMask", &TerrainDamageQueueIncrementMask, sizeof(TerrainDamageQueueIncrementMask), 0 },
	{ "HitLocationDirectionSelectorTable", &HitLocationDirectionSelectorTable, sizeof(HitLocationDirectionSelectorTable), 0 },
	{ "HitLocationTable", &HitLocationTable, sizeof(HitLocationTable), 0 },
	{ "UnitActionMaskByExperience", &UnitActionMaskByExperience, sizeof(UnitActionMaskByExperience), 0 },
	{ "UnitActionMaskByExperiencePastEnd", &UnitActionMaskByExperiencePastEnd, sizeof(UnitActionMaskByExperiencePastEnd), 0 },
	{ "WeaponAIHeatLimitByExperiencePastEnd", &WeaponAIHeatLimitByExperiencePastEnd, sizeof(WeaponAIHeatLimitByExperiencePastEnd), 0 },
	{ "TerrainImpactAnimationRecords", &TerrainImpactAnimationRecords, sizeof(TerrainImpactAnimationRecords), 0 },
	{ "NarrationMessageCategory", &NarrationMessageCategory, sizeof(NarrationMessageCategory), 0 },
	{ "PendingNarrationDelay", &PendingNarrationDelay, sizeof(PendingNarrationDelay), 0 },
	{ "ActiveNarrationSubject", &ActiveNarrationSubject, sizeof(ActiveNarrationSubject), 0 },
	{ "NarrationEmphasis", &NarrationEmphasis, sizeof(NarrationEmphasis), 0 },
	{ "DamagedWeaponRangeFlags", &DamagedWeaponRangeFlags, sizeof(DamagedWeaponRangeFlags), 0 },
	{ "CurrentUnitDurabilityTotal", &CurrentUnitDurabilityTotal, sizeof(CurrentUnitDurabilityTotal), 0 },
	{ "MaximumUnitDurabilityTotal", &MaximumUnitDurabilityTotal, sizeof(MaximumUnitDurabilityTotal), 0 },
	{ "SharedNumericTextBuffer", &SharedNumericTextBuffer, sizeof(SharedNumericTextBuffer), 0 },
	{ "PercentageSuffixText", PercentageSuffixText, 0, 0 },
	{ "TacticalUpdateDelayTicks", &TacticalUpdateDelayTicks, sizeof(TacticalUpdateDelayTicks), 0 },
	{ "TacticalKeyRepeatDelayTicks", &TacticalKeyRepeatDelayTicks, sizeof(TacticalKeyRepeatDelayTicks), 0 },
	{ "TacticalUpdateDelayReloadTicks", &TacticalUpdateDelayReloadTicks, sizeof(TacticalUpdateDelayReloadTicks), 0 },
	{ "TacticalGameSpeedIndex", &TacticalGameSpeedIndex, sizeof(TacticalGameSpeedIndex), 0 },
	{ "TacticalBattleResult", &TacticalBattleResult, sizeof(TacticalBattleResult), 0 },
	{ "TacticalClockDivider", &TacticalClockDivider, sizeof(TacticalClockDivider), 0 },
	{ "TacticalImpactQueueCount", &TacticalImpactQueueCount, sizeof(TacticalImpactQueueCount), 0 },
	{ "TacticalImpactQueue", &TacticalImpactQueue, sizeof(TacticalImpactQueue), 0 },
	{ "TacticalCursorSearchOffsetX", &TacticalCursorSearchOffsetX, sizeof(TacticalCursorSearchOffsetX), 0 },
	{ "TacticalCursorSearchOffsetY", &TacticalCursorSearchOffsetY, sizeof(TacticalCursorSearchOffsetY), 0 },
	{ "TacticalMouseEdgeInputByMask", &TacticalMouseEdgeInputByMask, sizeof(TacticalMouseEdgeInputByMask), 0 },
	{ "SceneNarrationEventFlags", &SceneNarrationEventFlags, sizeof(SceneNarrationEventFlags), 0 },
	{ "HeatDissipationBySinkCountQ8_8", &HeatDissipationBySinkCountQ8_8, sizeof(HeatDissipationBySinkCountQ8_8), 0 },
	{ "TerrainDamageQueueWordCount", &TerrainDamageQueueWordCount, sizeof(TerrainDamageQueueWordCount), 0 },
	{ "TacticalProjectileCount", &TacticalProjectileCount, sizeof(TacticalProjectileCount), 0 },
	{ "TacticalProjectileRecords", &TacticalProjectileRecords, sizeof(TacticalProjectileRecords), 0 },
	{ "TacticalOverviewDisplayMode", &TacticalOverviewDisplayMode, sizeof(TacticalOverviewDisplayMode), 0 },
	{ "LrmShipmentLoaded", &LrmShipmentLoaded, sizeof(LrmShipmentLoaded), 0 },
	{ "SrmShipmentLoaded", &SrmShipmentLoaded, sizeof(SrmShipmentLoaded), 0 },
	{ "TacticalOverviewMapMode", &TacticalOverviewMapMode, sizeof(TacticalOverviewMapMode), 0 },
	{ "NarrationSourceUnit", &NarrationSourceUnit, sizeof(NarrationSourceUnit), 0 },
	{ "LastAttackingUnitIndex", &LastAttackingUnitIndex, sizeof(LastAttackingUnitIndex), 0 },
	{ "PendingMouseButtonEvent", &PendingMouseButtonEvent, sizeof(PendingMouseButtonEvent), 0 },
	{ "MouseCursorTestX", &MouseCursorTestX, sizeof(MouseCursorTestX), 0 },
	{ "MouseCursorTestY", &MouseCursorTestY, sizeof(MouseCursorTestY), 0 },
	{ "MouseCursorUpdateLock", &MouseCursorUpdateLock, sizeof(MouseCursorUpdateLock), 0 },
	{ "MouseCursorRequestedX", &MouseCursorRequestedX, sizeof(MouseCursorRequestedX), 0 },
	{ "MouseCursorHideDepth", &MouseCursorHideDepth, sizeof(MouseCursorHideDepth), 0 },
	{ "MouseDriverDoubleX", &MouseDriverDoubleX, sizeof(MouseDriverDoubleX), 0 },
	{ "MouseDriverInstalled", &MouseDriverInstalled, sizeof(MouseDriverInstalled), 0 },
	{ "MouseButtonState", &MouseButtonState, sizeof(MouseButtonState), 0 },
	{ "MouseY", &MouseY, sizeof(MouseY), 0 },
	{ "MouseInputDisabled", &MouseInputDisabled, sizeof(MouseInputDisabled), 0 },
	{ "MouseCursorMinX", &MouseCursorMinX, sizeof(MouseCursorMinX), 0 },
	{ "MouseCursorMaxX", &MouseCursorMaxX, sizeof(MouseCursorMaxX), 0 },
	{ "MouseCursorMinY", &MouseCursorMinY, sizeof(MouseCursorMinY), 0 },
	{ "MouseCursorMaxY", &MouseCursorMaxY, sizeof(MouseCursorMaxY), 0 },
	{ "MouseCursorHotspotX", &MouseCursorHotspotX, sizeof(MouseCursorHotspotX), 0 },
	{ "MouseCursorHotspotY", &MouseCursorHotspotY, sizeof(MouseCursorHotspotY), 0 },
	{ "MouseCursorDisplayedX", &MouseCursorDisplayedX, sizeof(MouseCursorDisplayedX), 0 },
	{ "MouseCursorDisplayedY", &MouseCursorDisplayedY, sizeof(MouseCursorDisplayedY), 0 },
	{ "MouseCursorBackground", &MouseCursorBackground, sizeof(MouseCursorBackground), 1 },
	{ "MouseCursorBitmap", &MouseCursorBitmap, sizeof(MouseCursorBitmap), 1 },
	{ "MouseCursorErasedX", &MouseCursorErasedX, sizeof(MouseCursorErasedX), 0 },
	{ "MouseCursorErasedY", &MouseCursorErasedY, sizeof(MouseCursorErasedY), 0 },
	{ "MouseCursorRestoreTileLeft", &MouseCursorRestoreTileLeft, sizeof(MouseCursorRestoreTileLeft), 0 },
	{ "MouseCursorRestoreTileTop", &MouseCursorRestoreTileTop, sizeof(MouseCursorRestoreTileTop), 0 },
	{ "MouseCursorRestoreTileRight", &MouseCursorRestoreTileRight, sizeof(MouseCursorRestoreTileRight), 0 },
	{ "MouseCursorRestoreTileBottom", &MouseCursorRestoreTileBottom, sizeof(MouseCursorRestoreTileBottom), 0 },
	{ "MouseCursorDrawnX", &MouseCursorDrawnX, sizeof(MouseCursorDrawnX), 0 },
	{ "MouseCursorDrawnY", &MouseCursorDrawnY, sizeof(MouseCursorDrawnY), 0 },
	{ "MouseCursorCaptureTileLeft", &MouseCursorCaptureTileLeft, sizeof(MouseCursorCaptureTileLeft), 0 },
	{ "MouseCursorCaptureTileTop", &MouseCursorCaptureTileTop, sizeof(MouseCursorCaptureTileTop), 0 },
	{ "MouseCursorCaptureTileRight", &MouseCursorCaptureTileRight, sizeof(MouseCursorCaptureTileRight), 0 },
	{ "MouseCursorCaptureTileBottom", &MouseCursorCaptureTileBottom, sizeof(MouseCursorCaptureTileBottom), 0 },
	{ "MouseCursorDrawLeft", &MouseCursorDrawLeft, sizeof(MouseCursorDrawLeft), 0 },
	{ "MouseCursorDrawTop", &MouseCursorDrawTop, sizeof(MouseCursorDrawTop), 0 },
	{ "ActiveRenderTarget", &ActiveRenderTarget, sizeof(ActiveRenderTarget), 1 },
	{ "SpriteClipLeftTileMirror", &SpriteClipLeftTileMirror, sizeof(SpriteClipLeftTileMirror), 0 },
	{ "SpriteClipRightTileMirror", &SpriteClipRightTileMirror, sizeof(SpriteClipRightTileMirror), 0 },
	{ "SpriteClipTopPixelMirror", &SpriteClipTopPixelMirror, sizeof(SpriteClipTopPixelMirror), 0 },
	{ "SpriteClipBottomPixelMirror", &SpriteClipBottomPixelMirror, sizeof(SpriteClipBottomPixelMirror), 0 },
	{ "TacticalSpriteClipLeftTile", &TacticalSpriteClipLeftTile, sizeof(TacticalSpriteClipLeftTile), 0 },
	{ "TacticalSpriteClipRightTile", &TacticalSpriteClipRightTile, sizeof(TacticalSpriteClipRightTile), 0 },
	{ "TacticalSpriteClipTopPixel", &TacticalSpriteClipTopPixel, sizeof(TacticalSpriteClipTopPixel), 0 },
	{ "TacticalSpriteClipBottomPixel", &TacticalSpriteClipBottomPixel, sizeof(TacticalSpriteClipBottomPixel), 0 },
	{ "RequestedMusicEnabled", &RequestedMusicEnabled, sizeof(RequestedMusicEnabled), 0 },
	{ "CachedMusicResourceIndex", &CachedMusicResourceIndex, sizeof(CachedMusicResourceIndex), 0 },
	{ "SpriteDrawBusy", &SpriteDrawBusy, sizeof(SpriteDrawBusy), 0 },
	{ "PlanarBackingBufferExtentLow", &PlanarBackingBufferExtentLow, sizeof(PlanarBackingBufferExtentLow), 0 },
	{ "PlanarBackingBufferExtentHigh", &PlanarBackingBufferExtentHigh, sizeof(PlanarBackingBufferExtentHigh), 0 },
	{ "SceneTransitionAbortFlag", &SceneTransitionAbortFlag, sizeof(SceneTransitionAbortFlag), 0 },
	{ "PresentationDismissTimeout", &PresentationDismissTimeout, sizeof(PresentationDismissTimeout), 0 },
	{ "PresentationCallbackOffset", &PresentationCallbackOffset, sizeof(PresentationCallbackOffset), 0 },
	{ "PresentationCallbackSegment", &PresentationCallbackSegment, sizeof(PresentationCallbackSegment), 0 },
	{ "RestartConfirmationText", &RestartConfirmationText, sizeof(RestartConfirmationText), 1 },
	{ "MissionMapHeaderText", &MissionMapHeaderText, sizeof(MissionMapHeaderText), 1 },
	{ "TextEntryBuffer", &TextEntryBuffer, sizeof(TextEntryBuffer), 0 },
	{ "TextEntryCursorIndex", &TextEntryCursorIndex, sizeof(TextEntryCursorIndex), 0 },
	{ "TextEntryActive", &TextEntryActive, sizeof(TextEntryActive), 0 },
	{ "TextEntryPunctuation", TextEntryPunctuation, 0, 0 },
	{ "TextCursorBlinkGlyph", TextCursorBlinkGlyph, 0, 0 },
	{ "CharacterClassTable", &CharacterClassTable, sizeof(CharacterClassTable), 0 },
	{ "VideoPixelAlignmentMasks", &VideoPixelAlignmentMasks, sizeof(VideoPixelAlignmentMasks), 0 },
	{ "PackedPixelShiftCounts", &PackedPixelShiftCounts, sizeof(PackedPixelShiftCounts), 0 },
	{ "TextWindowFrameDefinition", &TextWindowFrameDefinition, sizeof(TextWindowFrameDefinition), 0 },
	{ "TextWindowFrameDefinitionTable", &TextWindowFrameDefinitionTable, sizeof(TextWindowFrameDefinitionTable), 1 },
	{ "MonthLengthTable", &MonthLengthTable, sizeof(MonthLengthTable), 0 },
	{ "CalendarBoundaryTable", &CalendarBoundaryTable, sizeof(CalendarBoundaryTable), 0 },
	{ "StandardTimezoneName", &StandardTimezoneName, sizeof(StandardTimezoneName), 0 },
	{ "DaylightTimezoneName", &DaylightTimezoneName, sizeof(DaylightTimezoneName), 0 },
	{ "TimezoneOffsetLowWord", &TimezoneOffsetLowWord, sizeof(TimezoneOffsetLowWord), 0 },
	{ "TimezoneOffsetHighWord", &TimezoneOffsetHighWord, sizeof(TimezoneOffsetHighWord), 0 },
	{ "DaylightSavingEnabled", &DaylightSavingEnabled, sizeof(DaylightSavingEnabled), 0 },
	{ "TimezoneEnvironmentName", &TimezoneEnvironmentName, sizeof(TimezoneEnvironmentName), 0 },
	{ "DefaultStandardTimezoneName", &DefaultStandardTimezoneName, sizeof(DefaultStandardTimezoneName), 0 },
	{ "DefaultDaylightTimezoneName", &DefaultDaylightTimezoneName, sizeof(DefaultDaylightTimezoneName), 0 },
	{ "TacticalOverviewActive", &TacticalOverviewActive, sizeof(TacticalOverviewActive), 0 },
	{ "ArtilleryShellingActive", &ArtilleryShellingActive, sizeof(ArtilleryShellingActive), 0 },
	{ "RollingBarrageActive", &RollingBarrageActive, sizeof(RollingBarrageActive), 0 },
	{ "StrafingRunActive", &StrafingRunActive, sizeof(StrafingRunActive), 0 },
	{ "ArtilleryTargetMapX", &ArtilleryTargetMapX, sizeof(ArtilleryTargetMapX), 0 },
	{ "ArtilleryTargetMapY", &ArtilleryTargetMapY, sizeof(ArtilleryTargetMapY), 0 },
	{ "RollingBarrageEndMapX", &RollingBarrageEndMapX, sizeof(RollingBarrageEndMapX), 0 },
	{ "RollingBarrageEndMapY", &RollingBarrageEndMapY, sizeof(RollingBarrageEndMapY), 0 },
	{ "RollingBarrageStartMapX", &RollingBarrageStartMapX, sizeof(RollingBarrageStartMapX), 0 },
	{ "RollingBarrageStartMapY", &RollingBarrageStartMapY, sizeof(RollingBarrageStartMapY), 0 },
	{ "DropShipMapX", &DropShipMapX, sizeof(DropShipMapX), 0 },
	{ "DropShipMapY", &DropShipMapY, sizeof(DropShipMapY), 0 },
	{ "ArtillerySupportAvailable", &ArtillerySupportAvailable, sizeof(ArtillerySupportAvailable), 0 },
	{ "StrafingSupportAvailable", &StrafingSupportAvailable, sizeof(StrafingSupportAvailable), 0 },
	{ "TacticalOverviewViewportDirty", &TacticalOverviewViewportDirty, sizeof(TacticalOverviewViewportDirty), 0 },
	{ "TacticalCellTerrainMap", &TacticalCellTerrainMap, sizeof(TacticalCellTerrainMap), 1 },
	{ "TacticalCellSpriteMap", &TacticalCellSpriteMap, sizeof(TacticalCellSpriteMap), 1 },
	{ "ScenarioUnitNeedsUpdate", &ScenarioUnitNeedsUpdate, sizeof(ScenarioUnitNeedsUpdate), 0 },
	{ "TacticalStateInitialized", &TacticalStateInitialized, sizeof(TacticalStateInitialized), 0 },
	{ "SuppressCampaignClock", &SuppressCampaignClock, sizeof(SuppressCampaignClock), 0 },
	{ "TacticalCursorOutlineEnabled", &TacticalCursorOutlineEnabled, sizeof(TacticalCursorOutlineEnabled), 0 },
	{ "TacticalCursorOutlineState", &TacticalCursorOutlineState, sizeof(TacticalCursorOutlineState), 0 },
	{ "ActiveFontResource", &ActiveFontResource, sizeof(ActiveFontResource), 1 },
	{ "MusicDriverIndex", &MusicDriverIndex, sizeof(MusicDriverIndex), 0 },
	{ "ActiveFontGlyphAdvanceX", &ActiveFontGlyphAdvanceX, sizeof(ActiveFontGlyphAdvanceX), 0 },
	{ "ActiveFontLineHeight", &ActiveFontLineHeight, sizeof(ActiveFontLineHeight), 0 },
	{ "TacticalTileStrip", &TacticalTileStrip, sizeof(TacticalTileStrip), 1 },
	{ "BusyWaitLoopCalibration", &BusyWaitLoopCalibration, sizeof(BusyWaitLoopCalibration), 0 },
	{ "ActiveUiHighlightXorColor", &ActiveUiHighlightXorColor, sizeof(ActiveUiHighlightXorColor), 0 },
	{ "ActiveUiHighlightHeight", &ActiveUiHighlightHeight, sizeof(ActiveUiHighlightHeight), 0 },
	{ "ActiveUiHighlightWidth", &ActiveUiHighlightWidth, sizeof(ActiveUiHighlightWidth), 0 },
	{ "ActiveUiHighlightLeft", &ActiveUiHighlightLeft, sizeof(ActiveUiHighlightLeft), 0 },
	{ "ActiveUiHighlightTop", &ActiveUiHighlightTop, sizeof(ActiveUiHighlightTop), 0 },
	{ "PrimaryScreenBuffer", &PrimaryScreenBuffer, sizeof(PrimaryScreenBuffer), 1 },
	{ "SavedUiRegionSelectionRows", &SavedUiRegionSelectionRows, sizeof(SavedUiRegionSelectionRows), 0 },
	{ "ActiveUiRegionIndex", &ActiveUiRegionIndex, sizeof(ActiveUiRegionIndex), 0 },
	{ "ExpandFirstUiRowHighlight", &ExpandFirstUiRowHighlight, sizeof(ExpandFirstUiRowHighlight), 0 },
	{ "VideoAdapterType", &VideoAdapterType, sizeof(VideoAdapterType), 0 },
	{ "TacticalMapFileName", &TacticalMapFileName, sizeof(TacticalMapFileName), 0 },
	{ "TacticalIconSetFileName", &TacticalIconSetFileName, sizeof(TacticalIconSetFileName), 0 },
	{ "TacticalDestroyIconFileName", &TacticalDestroyIconFileName, sizeof(TacticalDestroyIconFileName), 0 },
	{ "TacticalIconListFileName", &TacticalIconListFileName, sizeof(TacticalIconListFileName), 0 },
	{ "TacticalMechSetFileName", &TacticalMechSetFileName, sizeof(TacticalMechSetFileName), 0 },
	{ "ResourceRetryCount", &ResourceRetryCount, sizeof(ResourceRetryCount), 0 },
	{ "ReservedConventionalMemoryKB", &ReservedConventionalMemoryKB, sizeof(ReservedConventionalMemoryKB), 0 },
	{ "ExitHandlerCount", &ExitHandlerCount, sizeof(ExitHandlerCount), 0 },
	{ "ExitHandlerTable", &ExitHandlerTable, sizeof(ExitHandlerTable), 1 },
	{ "RuntimeExitHook1", &RuntimeExitHook1, sizeof(RuntimeExitHook1), 1 },
	{ "RuntimeExitHook2", &RuntimeExitHook2, sizeof(RuntimeExitHook2), 1 },
	{ "RuntimeExitHook3", &RuntimeExitHook3, sizeof(RuntimeExitHook3), 1 },
	{ "RuntimeTerminationHook", &RuntimeTerminationHook, sizeof(RuntimeTerminationHook), 1 },
	{ "SavedDivideErrorHandler", &SavedDivideErrorHandler, sizeof(SavedDivideErrorHandler), 1 },
	{ "SavedOverflowHandler", &SavedOverflowHandler, sizeof(SavedOverflowHandler), 1 },
	{ "SavedBoundsHandler", &SavedBoundsHandler, sizeof(SavedBoundsHandler), 1 },
	{ "SavedInvalidOpcodeHandler", &SavedInvalidOpcodeHandler, sizeof(SavedInvalidOpcodeHandler), 1 },
	{ "ResourceErrorDisplayCallback", &ResourceErrorDisplayCallback, sizeof(ResourceErrorDisplayCallback), 1 },
	{ "ResourceErrorDismissCallback", &ResourceErrorDismissCallback, sizeof(ResourceErrorDismissCallback), 1 },
	{ "ResourceCreateFileErrorText", &ResourceCreateFileErrorText, sizeof(ResourceCreateFileErrorText), 1 },
	{ "ResourceIllegalOpenModeErrorText", &ResourceIllegalOpenModeErrorText, sizeof(ResourceIllegalOpenModeErrorText), 1 },
	{ "ResourceInsertDiskErrorText", &ResourceInsertDiskErrorText, sizeof(ResourceInsertDiskErrorText), 1 },
	{ "ResourceTooManyOpenFilesErrorText", &ResourceTooManyOpenFilesErrorText, sizeof(ResourceTooManyOpenFilesErrorText), 1 },
	{ "ResourceCloseUnopenedErrorText", &ResourceCloseUnopenedErrorText, sizeof(ResourceCloseUnopenedErrorText), 1 },
	{ "ResourceReadUnopenedErrorText", &ResourceReadUnopenedErrorText, sizeof(ResourceReadUnopenedErrorText), 1 },
	{ "ResourceWriteUnopenedErrorText", &ResourceWriteUnopenedErrorText, sizeof(ResourceWriteUnopenedErrorText), 1 },
	{ "ResourceSeekUnopenedErrorText", &ResourceSeekUnopenedErrorText, sizeof(ResourceSeekUnopenedErrorText), 1 },
	{ "ResourceIllegalSeekOriginErrorText", &ResourceIllegalSeekOriginErrorText, sizeof(ResourceIllegalSeekOriginErrorText), 1 },
	{ "ResourceWriteCachedErrorText", &ResourceWriteCachedErrorText, sizeof(ResourceWriteCachedErrorText), 1 },
	{ "ResourceUnknownDriveErrorText", &ResourceUnknownDriveErrorText, sizeof(ResourceUnknownDriveErrorText), 1 },
	{ "ResourceUnknownCacheIndexErrorText", &ResourceUnknownCacheIndexErrorText, sizeof(ResourceUnknownCacheIndexErrorText), 1 },
	{ "ResourceOpenFilesAtExitErrorText", &ResourceOpenFilesAtExitErrorText, sizeof(ResourceOpenFilesAtExitErrorText), 1 },
	{ "ResourceFatalDiskErrorText", &ResourceFatalDiskErrorText, sizeof(ResourceFatalDiskErrorText), 1 },
	{ "ActiveVideoAdapterType", &ActiveVideoAdapterType, sizeof(ActiveVideoAdapterType), 0 },
	{ "EncodedSpriteClipLeftTile", &EncodedSpriteClipLeftTile, sizeof(EncodedSpriteClipLeftTile), 0 },
	{ "SpriteClipRightTile", &SpriteClipRightTile, sizeof(SpriteClipRightTile), 0 },
	{ "SpriteClipTopPixel", &SpriteClipTopPixel, sizeof(SpriteClipTopPixel), 0 },
	{ "EncodedSpriteClipBottomPixel", &EncodedSpriteClipBottomPixel, sizeof(EncodedSpriteClipBottomPixel), 0 },
	{ "SpritePreserveMode", &SpritePreserveMode, sizeof(SpritePreserveMode), 0 },
	{ "WeaponCombatRatingByType", &WeaponCombatRatingByType, sizeof(WeaponCombatRatingByType), 0 },
	{ "RelativeAttackDirectionByMask", &RelativeAttackDirectionByMask, sizeof(RelativeAttackDirectionByMask), 0 },
	{ "LinePrimaryYStep", &LinePrimaryYStep, sizeof(LinePrimaryYStep), 0 },
	{ "LinePrimaryXStep", &LinePrimaryXStep, sizeof(LinePrimaryXStep), 0 },
	{ "LinePrimaryRowOffset", &LinePrimaryRowOffset, sizeof(LinePrimaryRowOffset), 0 },
	{ "LineSecondaryYStep", &LineSecondaryYStep, sizeof(LineSecondaryYStep), 0 },
	{ "LineSecondaryXStep", &LineSecondaryXStep, sizeof(LineSecondaryXStep), 0 },
	{ "LineSecondaryRowOffset", &LineSecondaryRowOffset, sizeof(LineSecondaryRowOffset), 0 },
	{ "EncodedSpriteVisibleWidth", &EncodedSpriteVisibleWidth, sizeof(EncodedSpriteVisibleWidth), 0 },
	{ "EncodedSpriteSourceWidth", &EncodedSpriteSourceWidth, sizeof(EncodedSpriteSourceWidth), 0 },
	{ "EncodedSpriteHorizontalPhase", &EncodedSpriteHorizontalPhase, sizeof(EncodedSpriteHorizontalPhase), 0 },
	{ "EncodedSpriteClipWidthTiles", &EncodedSpriteClipWidthTiles, sizeof(EncodedSpriteClipWidthTiles), 0 },
	{ "PseudoRandomState", &PseudoRandomState, sizeof(PseudoRandomState), 0 },
	{ "StoryTextSlots", &StoryTextSlots, sizeof(StoryTextSlots), -1 },
	{ "StoryTextBuffer", &StoryTextBuffer, sizeof(StoryTextBuffer), 0 },
	{ "MIDIDriverDescriptorIdentities", &MIDIDriverDescriptorIdentities, sizeof(MIDIDriverDescriptorIdentities), 1 },
	{ "MIDIDriverHostDispatches", &MIDIDriverHostDispatches, sizeof(MIDIDriverHostDispatches), 1 },
	{ "NullDigitalPlayback", &NullDigitalPlayback, sizeof(NullDigitalPlayback), 0 },
	{ "SoundBlasterPlayback", &SoundBlasterPlayback, sizeof(SoundBlasterPlayback), 0 },
	{ "DigitalPlaybackRuntime", &DigitalPlaybackRuntime, sizeof(DigitalPlaybackRuntime), 0 },
	{ "DigitalPlaybackBuffers", &DigitalPlaybackBuffers, sizeof(DigitalPlaybackBuffers), 0 },
	{ "DigitalSoundData", &DigitalSoundData, sizeof(DigitalSoundData), 0 },
	{ "DigitalSamplePlaybackDescriptorTable", &DigitalSamplePlaybackDescriptorTable, sizeof(DigitalSamplePlaybackDescriptorTable), 0 },
	{ "ActiveDigitalSampleDescriptor", &ActiveDigitalSampleDescriptor, sizeof(ActiveDigitalSampleDescriptor), 1 },
	{ "ActiveDigitalDriverQueue", &ActiveDigitalDriverQueue, sizeof(ActiveDigitalDriverQueue), 0 },
	{ "DigitalWeaponEffectRepeatCount", &DigitalWeaponEffectRepeatCount, sizeof(DigitalWeaponEffectRepeatCount), 0 },
	{ "DigitalWeaponSampleBuffer", &DigitalWeaponSampleBuffer, sizeof(DigitalWeaponSampleBuffer), 1 },
	{ "DigitalSoundBuffer", &DigitalSoundBuffer, sizeof(DigitalSoundBuffer), 1 },
	{ "DigitalSoundDriverMutationGuard", &DigitalSoundDriverMutationGuard, sizeof(DigitalSoundDriverMutationGuard), 0 },
	{ "NarrationMode", &NarrationMode, sizeof(NarrationMode), 0 },
};

/* Each pointer inside an object that isn't itself a pointer or an array
 * of them: its offset, repeated count1 times stride1 apart and each of
 * those count2 times stride2 apart (the arrays it sits in). */
typedef struct MapDumpSlot
{
	size_t object;
	size_t offset;
	size_t count1;
	size_t stride1;
	size_t count2;
	size_t stride2;
} MapDumpSlot;

static const MapDumpSlot pointerSlots[] =
{
	{ 6, offsetof(RevengeMusicTimerState, ActiveSpeakerEffectStream), 1, 0, 1, 0 },
	{ 6, offsetof(RevengeMusicTimerState, PendingSpeakerEffectStream), 1, 0, 1, 0 },
	{ 10, offsetof(MIDISequenceState, MIDIStreamCursor), 1, 0, 1, 0 },
	{ 64, offsetof(UnitSelectionCard, spriteBuffer), sizeof(UnitSelectionCards) / sizeof(UnitSelectionCards[0]), sizeof(UnitSelectionCards[0]), 1, 0 },
	{ 115, offsetof(ResourceFileRuntime, errorText), 1, 0, 1, 0 },
	{ 116, offsetof(CachedFileDescriptor, fileName), sizeof(CachedFileDescriptors) / sizeof(CachedFileDescriptors[0]), sizeof(CachedFileDescriptors[0]), 1, 0 },
	{ 116, offsetof(CachedFileDescriptor, cachedData), sizeof(CachedFileDescriptors) / sizeof(CachedFileDescriptors[0]), sizeof(CachedFileDescriptors[0]), 1, 0 },
	{ 687, offsetof(DigitalPlaybackRuntimeState, ActiveDriverQueue), 1, 0, 1, 0 },
	{ 688, offsetof(DigitalPlaybackWorkingSet, PlaybackTimerQueue.First), 1, 0, 1, 0 },
	{ 688, offsetof(DigitalPlaybackWorkingSet, PlaybackTimerQueue.End.Next), 1, 0, 1, 0 },
	{ 688, offsetof(DigitalPlaybackWorkingSet, PlaybackTimerQueue.End.PreviousLink), 1, 0, 1, 0 },
	{ 688, offsetof(DigitalPlaybackWorkingSet, CalibrationQueue.First), 1, 0, 1, 0 },
	{ 688, offsetof(DigitalPlaybackWorkingSet, CalibrationQueue.End.Next), 1, 0, 1, 0 },
	{ 688, offsetof(DigitalPlaybackWorkingSet, CalibrationQueue.End.PreviousLink), 1, 0, 1, 0 },
	{ 688, offsetof(DigitalPlaybackWorkingSet, CalibrationSample.Link.Next), 1, 0, 1, 0 },
	{ 688, offsetof(DigitalPlaybackWorkingSet, CalibrationSample.Link.PreviousLink), 1, 0, 1, 0 },
	{ 688, offsetof(DigitalPlaybackWorkingSet, CalibrationSample.OwnerQueue), 1, 0, 1, 0 },
	{ 688, offsetof(DigitalPlaybackWorkingSet, CalibrationSample.SampleData), 1, 0, 1, 0 },
	{ 688, offsetof(DigitalPlaybackWorkingSet, CalibrationSample.Completion), 1, 0, 1, 0 },
	{ 688, offsetof(DigitalPlaybackWorkingSet, CalibrationSample.CompletionArgument), 1, 0, 1, 0 },
	{ 690, offsetof(DigitalSampleDescriptor, Link.Next), sizeof(DigitalSamplePlaybackDescriptorTable) / sizeof(DigitalSamplePlaybackDescriptorTable[0]), sizeof(DigitalSamplePlaybackDescriptorTable[0]), 1, 0 },
	{ 690, offsetof(DigitalSampleDescriptor, Link.PreviousLink), sizeof(DigitalSamplePlaybackDescriptorTable) / sizeof(DigitalSamplePlaybackDescriptorTable[0]), sizeof(DigitalSamplePlaybackDescriptorTable[0]), 1, 0 },
	{ 690, offsetof(DigitalSampleDescriptor, OwnerQueue), sizeof(DigitalSamplePlaybackDescriptorTable) / sizeof(DigitalSamplePlaybackDescriptorTable[0]), sizeof(DigitalSamplePlaybackDescriptorTable[0]), 1, 0 },
	{ 690, offsetof(DigitalSampleDescriptor, SampleData), sizeof(DigitalSamplePlaybackDescriptorTable) / sizeof(DigitalSamplePlaybackDescriptorTable[0]), sizeof(DigitalSamplePlaybackDescriptorTable[0]), 1, 0 },
	{ 690, offsetof(DigitalSampleDescriptor, Completion), sizeof(DigitalSamplePlaybackDescriptorTable) / sizeof(DigitalSamplePlaybackDescriptorTable[0]), sizeof(DigitalSamplePlaybackDescriptorTable[0]), 1, 0 },
	{ 690, offsetof(DigitalSampleDescriptor, CompletionArgument), sizeof(DigitalSamplePlaybackDescriptorTable) / sizeof(DigitalSamplePlaybackDescriptorTable[0]), sizeof(DigitalSamplePlaybackDescriptorTable[0]), 1, 0 },
	{ 692, offsetof(DigitalDriverQueue, First), 1, 0, 1, 0 },
	{ 692, offsetof(DigitalDriverQueue, End.Next), 1, 0, 1, 0 },
	{ 692, offsetof(DigitalDriverQueue, End.PreviousLink), 1, 0, 1, 0 },
};

typedef void (*MapDumpFunction)(void);

typedef struct MapDumpFunctionName
{
	const char *name;
	MapDumpFunction function;
} MapDumpFunctionName;

static const MapDumpFunctionName functions[] =
{
	{ "empty", (MapDumpFunction) RuntimeExitStepStub },
	{ "empty", (MapDumpFunction) RuntimeTerminationStub },
	{ "ShowResourceErrorMessage", (MapDumpFunction) ShowResourceErrorMessage },
	{ "ShowFatalDiskErrorMessage", (MapDumpFunction) ShowFatalDiskErrorMessage },
	{ "HandleDigitalWeaponEffectCompletion", (MapDumpFunction) HandleDigitalWeaponEffectCompletion },
};

void MapDump_Write(const char *snap)
{
	const char *folder = SDL_getenv("REVENGE_MAP_DUMP_DIR");
	char path[4096];
	FILE *file;
	size_t index;

	if (folder == NULL || folder[0] == '\0')
		return;
	snprintf(path, sizeof(path), "%s/%s.objects", folder, snap);
	file = fopen(path, "wb");
	if (file == NULL)
		return;
	for (index = 0; index < sizeof(objects) / sizeof(objects[0]); ++index)
	{
		const unsigned char *bytes = (const unsigned char *) objects[index].object;
		size_t size = objects[index].size != 0 ? objects[index].size
			: strlen((const char *) objects[index].object) + 1;
		size_t at;

		fprintf(file, "%s %lu ", objects[index].name, (unsigned long) size);
		for (at = 0; at < size; ++at)
			fprintf(file, "%02x", bytes[at]);
		fputc('\n', file);
	}
	fclose(file);
}


/* The snaps by name (MapDump_WriteNames). A pointer is a host address, so
 * its bytes differ from run to run and system to system: each pointer-sized,
 * pointer-aligned word of an object that points into a data object, or into
 * a block of the heap, is hashed as that place's name and the offset in it
 * (a block's name is its number in allocation order, revenge/heap.h). Every
 * other byte is hashed as it is. */

typedef struct MapDumpRegion
{
	uintptr_t start;
	size_t size;
	const char *name;
} MapDumpRegion;

static MapDumpRegion *regions;
static size_t regionCount;

static size_t ObjectSize(size_t index)
{
	return objects[index].size != 0 ? objects[index].size
		: strlen((const char *) objects[index].object) + 1;
}

static int CompareRegions(const void *left, const void *right)
{
	uintptr_t a = ((const MapDumpRegion *) left)->start;
	uintptr_t b = ((const MapDumpRegion *) right)->start;

	return a < b ? -1 : a > b ? 1 : 0;
}

static void FindRegions(void)
{
	size_t count = sizeof(objects) / sizeof(objects[0]);
	size_t index;

	free(regions);
	regions = (MapDumpRegion *) calloc(count, sizeof(MapDumpRegion));
	if (regions == NULL)
	{
		regionCount = 0;
		return;
	}
	regionCount = 0;
	for (index = 0; index < count; ++index)
	{
		regions[regionCount].start = (uintptr_t) objects[index].object;
		regions[regionCount].size = ObjectSize(index);
		regions[regionCount].name = objects[index].name;
		++regionCount;
	}
	qsort(regions, regionCount, sizeof(MapDumpRegion), CompareRegions);
}

/* The region an address falls in, or NULL. An address just past an object's
 * end is not taken as that object's: what the linker puts there differs from
 * system to system (MSVC sets string literals beside named data). */
static const MapDumpRegion *Locate(uintptr_t address)
{
	size_t low = 0;
	size_t high = regionCount;

	if (address == 0)
		return NULL;
	while (low < high)
	{
		size_t middle = low + (high - low) / 2;

		if (regions[middle].start <= address)
			low = middle + 1;
		else
			high = middle;
	}
	if (low == 0)
		return NULL;
	if (address - regions[low - 1].start >= regions[low - 1].size)
		return NULL;
	return &regions[low - 1];
}

/* A pointer slot's place, written to the hash: the object or heap block it
 * is in, else the function it is, else "elsewhere" (a static the headers don't
 * name, a literal, a buffer on the stack). NULL is its zero bytes. */
static void HashPlace(Sha256 *hash, uintptr_t word, int *taken)
{
	const MapDumpRegion *region = Locate(word);
	char place[96];
	int length = 0;
	size_t index;
	uint32 number;
	uint32 offset;

	*taken = 0;
	if (region != NULL)
	{
		length = snprintf(place, sizeof(place), "\x01%s+%zx\x01", region->name,
			(size_t) (word - region->start));
	}
	else if (word != 0 && Heap_Find(word, &number, &offset))
	{
		length = snprintf(place, sizeof(place), "\x01heap:%lu+%lx\x01",
			(unsigned long) number, (unsigned long) offset);
	}
	else if (word != 0)
	{
		for (index = 0; index < sizeof(functions) / sizeof(functions[0]); ++index)
		{
			uintptr_t function = 0;

			memcpy(&function, &functions[index].function,
				sizeof(function) < sizeof(functions[index].function) ? sizeof(function)
					: sizeof(functions[index].function));
			if (function == word)
			{
				length = snprintf(place, sizeof(place), "\x01%s()\x01", functions[index].name);
				break;
			}
		}
		if (length == 0)
			length = snprintf(place, sizeof(place), "\x01elsewhere\x01");
	}
	if (length > 0)
	{
		Sha256_Add(hash, place, (size_t) length);
		*taken = 1;
	}
}

static int CompareOffsets(const void *left, const void *right)
{
	size_t a = *(const size_t *) left;
	size_t b = *(const size_t *) right;

	return a < b ? -1 : a > b ? 1 : 0;
}

/* The offsets of the object's pointers, ascending: every aligned slot of an
 * object declared as pointers, else its struct members that are (the slot
 * table). *count is how many; the caller frees the list. */
static size_t *PointerOffsets(size_t index, size_t size, size_t *count)
{
	size_t capacity = 0;
	size_t *offsets = NULL;
	size_t slot;

	*count = 0;
	if (objects[index].pointers > 0)
	{
		const unsigned char *bytes = (const unsigned char *) objects[index].object;
		size_t at;

		offsets = (size_t *) malloc((size / sizeof(void *) + 1) * sizeof(size_t));
		if (offsets == NULL)
			return NULL;
		for (at = 0; at + sizeof(void *) <= size; ++at)
		{
			if (((uintptr_t) (bytes + at)) % sizeof(void *) == 0)
				offsets[(*count)++] = at;
		}
		return offsets;
	}
	for (slot = 0; slot < sizeof(pointerSlots) / sizeof(pointerSlots[0]); ++slot)
	{
		const MapDumpSlot *entry = &pointerSlots[slot];
		size_t first;
		size_t second;

		if (entry->object != index)
			continue;
		for (first = 0; first < entry->count1; ++first)
		{
			for (second = 0; second < entry->count2; ++second)
			{
				size_t at = entry->offset + first * entry->stride1 + second * entry->stride2;

				if (at + sizeof(void *) > size)
					continue;
				if (*count == capacity)
				{
					size_t *grown;

					capacity = capacity == 0 ? 16 : capacity * 2;
					grown = (size_t *) realloc(offsets, capacity * sizeof(size_t));
					if (grown == NULL)
					{
						free(offsets);
						*count = 0;
						return NULL;
					}
					offsets = grown;
				}
				offsets[(*count)++] = at;
			}
		}
	}
	if (*count > 1)
		qsort(offsets, *count, sizeof(size_t), CompareOffsets);
	return offsets;
}

/* The object's hash: its bytes, with each pointer in it hashed as its place
 * (PointerOffsets). Other bytes hash as they are: a number that happens to
 * equal an address is not a pointer. */
static void HashObject(size_t index, char hex[17])
{
	const unsigned char *bytes = (const unsigned char *) objects[index].object;
	size_t size = ObjectSize(index);
	size_t count;
	size_t *offsets = PointerOffsets(index, size, &count);
	size_t run = 0;
	size_t slot;
	uint8_t digest[SHA256_DIGEST_BYTES];
	Sha256 hash;
	int digit;

	Sha256_Begin(&hash);
	for (slot = 0; slot < count; ++slot)
	{
		size_t at = offsets[slot];
		uintptr_t word;
		int taken = 0;

		if (at < run)
			continue;
		memcpy(&word, bytes + at, sizeof(word));
		Sha256_Add(&hash, bytes + run, at - run);
		run = at;
		HashPlace(&hash, word, &taken);
		if (taken)
			run = at + sizeof(void *);
	}
	free(offsets);
	Sha256_Add(&hash, bytes + run, size - run);
	Sha256_End(&hash, digest);
	for (digit = 0; digit < 8; ++digit)
		snprintf(hex + digit * 2, 3, "%02x", digest[digit]);
}

void MapDump_WriteNames(FILE *file)
{
	size_t index;
	char hex[17];

	FindRegions();
	for (index = 0; index < sizeof(objects) / sizeof(objects[0]); ++index)
	{
		if (objects[index].pointers < 0)
			continue;
		HashObject(index, hex);
		fprintf(file, "%s %s\n", objects[index].name, hex);
	}
}
