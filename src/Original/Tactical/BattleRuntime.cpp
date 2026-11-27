// 1A1F: battle runtime
// ScenarioRuntime.cpp
// Procedures preserved from REVENGE_1A1F_SCENARIO.c.

#include "BattleInternal.h"
#include "Audio/Audio.h"
#include "Data/AudioData.h"
#include "Data/ResourceData.h"
#include "Data/RuntimeData.h"
#include "Data/TacticalGraphicsData.h"
#include "Data/TacticalTables.h"
#include "Data/TextIOData.h"
#include "Graphics/Graphics.h"
#include "Input/Input.h"
#include "Resources/Resources.h"
#include "Runtime/Runtime.h"
#include "Tactical/Tactical.h"
#include "Utility/Utility.h"
#include "revenge/heap.h"
#include "revenge/debug.h"
#include "revenge/platform_backend.h"

#include <string.h>

// 1A1F:0000: void RunTacticalBattle(word16 scenarioCode)
// Called from:
//      InitializeTacticalScenario
void RunTacticalBattle(uint16_t scenarioCode)
{
    // Verified against ASM 1A1F:0000-0350. The original cleanup deliberately
    // frees only the odd-numbered late backing buffers before freeing buffers
    // 0Dh..01h and the tactical runtime allocation.
    ShowMouseCursorOverlay();
    ScenarioFileSuffix = scenarioCode;

    uint8_t *runtimeMemory =
        (uint8_t *) Heap_Allocate(TACTICAL_RUNTIME_ALLOCATION_SIZE, "1A1F:0016");
    TerrainTypeMap = runtimeMemory;
    TacticalOverviewMapBuffer = runtimeMemory + TACTICAL_OVERVIEW_MAP_OFFSET;
    PackedTerrainDamageMap = runtimeMemory + PACKED_TERRAIN_DAMAGE_MAP_OFFSET;
    TacticalCellSpriteMap = (uint16_t *) (runtimeMemory + TACTICAL_CELL_SPRITE_MAP_OFFSET);
    TacticalCellTerrainMap = (uint16_t *) (runtimeMemory + TACTICAL_CELL_TERRAIN_MAP_OFFSET);
    memset(runtimeMemory, 0x00, TACTICAL_RUNTIME_ALLOCATION_SIZE);
    InstallDamagedTerrainSpriteLookup();

    LoadPackedIconPageDirect(
        TacticalResourcePath,
        PlanarScreenBuffer,
        PrimaryScreenBuffer);

    uint16_t bufferIndex;
    for (bufferIndex = 0x01; bufferIndex <= 0x05; ++bufferIndex)
    {
        SET_TACTICAL_BACKING_BUFFER(
            bufferIndex,
            BuildPlanarDisplayBackingBuffer(
                (bufferIndex - 0x01) * 0x05, 0x00, 0x05, 0x28));
    }

    static const uint8_t panelX[0x08] = { 0x00, 0x06, 0x09, 0x0F, 0x00, 0x09, 0x0F, 0x18 };
    static const uint8_t panelY[0x08] = { 0x28, 0x28, 0x28, 0x28, 0x40, 0x40, 0x40, 0x40 };
    for (bufferIndex = 0x00; bufferIndex < 0x08; ++bufferIndex)
    {
        SET_TACTICAL_BACKING_BUFFER(
            bufferIndex + 0x06,
            BuildPlanarDisplayBackingBuffer(
                panelX[bufferIndex], panelY[bufferIndex], 0x03, 0x18));
    }

    for (bufferIndex = 0x00; bufferIndex < 0x08; ++bufferIndex)
    {
        SET_TACTICAL_BACKING_BUFFER(
            0x0E + bufferIndex * 0x02,
            BuildPlanarDisplayBackingBuffer(
                bufferIndex << 0x01, 0x60, 0x02, 0x10));
        SET_TACTICAL_BACKING_BUFFER(
            0x0F + bufferIndex * 0x02,
            BuildPlanarDisplayBackingBuffer(
                bufferIndex << 0x01, 0x70, 0x02, 0x10));
    }

    SuppressCampaignClock = TRUE;
    /* ASM 1A1F:0233-0244 restores the saved scenario clock into the live
	 * tactical clock.  PrepareScenarioUnitRosters performs the opposite
	 * copy when leaving a scenario. */
    memcpy(CampaignClockState, SavedCampaignClockState, 0x03);
    LoadTacticalScenarioResources((uint8_t) scenarioCode);
    REVENGE_PRESERVATION_TRACE("tactical: scenario loaded unit0=%u,%u deployed=%u unit15=%u,%u deployed=%u\n",
        ScenarioUnitMapX[0x00], ScenarioUnitMapY[0x00],
        PersistentUnitRoster[0x00].deploymentState,
        ScenarioUnitMapX[0x0F], ScenarioUnitMapY[0x0F],
        PersistentUnitRoster[0x0F].deploymentState);
    REVENGE_PRESERVATION_TRACE("tactical: map=%ux%u unit0 terrain=%u speed=%u delay=%u\n",
        MapWidth, MapHeight,
        TerrainTypeMap[ScenarioUnitMapY[0x00] * MapWidth +
            ScenarioUnitMapX[0x00]],
        TacticalGameSpeedIndex,
        TacticalUpdateDelayReloadTicks);
    LoadTacticalIconResource(
        TacticalMechSetFileName, 0x01);
    HideMouseCursorOverlay();
    ClearActiveVideoBuffer();
    InstallVgaPaletteBankAnchors();
    ShowMouseCursorOverlay();
    InitializeTacticalUnitRuntime();
    REVENGE_PRESERVATION_TRACE("tactical: unit runtime initialized\n");
    RedrawTacticalViewport();
    REVENGE_PRESERVATION_TRACE("tactical: viewport redrawn\n");

    uint16_t unitIndex;
    for (unitIndex = 0x00; unitIndex < TACTICAL_UNIT_COUNT; ++unitIndex)
    {
        RevengeUnitRecord *unitRecord =
            &PersistentUnitRoster[unitIndex];
        if (unitRecord->deploymentState != 0x00)
            ScenarioUnitNeedsUpdate[unitIndex] = TRUE;
    }

    UpdateTacticalFrame(0x00, TRUE);
    REVENGE_PRESERVATION_TRACE("tactical: first frame complete viewport=%u,%u cursor=%u,%u\n",
        TacticalViewportMapX, TacticalViewportMapY,
        TacticalCursorViewportX, TacticalCursorViewportY);
    REVENGE_PRESERVATION_TRACE("tactical: selected terrain sprite=%u source=%u\n",
        TacticalCellTerrainMap[TacticalCursorViewportY * 0x0013 +
            TacticalCursorViewportX],
        TerrainTypeMap[ScenarioUnitMapY[0x00] * MapWidth +
            ScenarioUnitMapX[0x00]]);
    TacticalStateInitialized = TRUE;
    if (MouseDriverInstalled != FALSE)
    {
        SetMouseCursorPosition(
            (TacticalCursorViewportX << 0x04) + 0x18,
            (TacticalCursorViewportY << 0x04) + 0x08);
    }

    RunTacticalInteractionLoop();

    for (bufferIndex = 0x1D; bufferIndex >= 0x0F; bufferIndex -= 0x02)
        Heap_Free(TACTICAL_BACKING_BUFFER(bufferIndex));
    for (bufferIndex = 0x0D; bufferIndex >= 0x01; --bufferIndex)
        Heap_Free(TACTICAL_BACKING_BUFFER(bufferIndex));
    Heap_Free(runtimeMemory);
}

// 1A1F:0351: void UpdateTacticalFrame(
//     word16 inputAction, word16 advanceUnits)
// Verified against 1A1F:0351-03D3.
void UpdateTacticalFrame(uint16_t inputAction, uint16_t advanceUnits)
{
    uint16_t movementRepeatCount = 0x01;
    uint16_t repeatIndex;
    uint16_t unitIndex;

    if (TacticalOverviewActive != 0x00 &&
        TacticalMenuInputState != 0x00 &&
        TacticalOverviewZoomed == 0x00)
    {
        movementRepeatCount = 0x0A;
    }

    for (repeatIndex = 0x00;
        repeatIndex < movementRepeatCount;
        ++repeatIndex)
    {
        MoveTacticalCursorByInput((int16_t) inputAction);
    }

    if (advanceUnits != 0x00)
    {
        for (unitIndex = 0x00; unitIndex < TACTICAL_UNIT_COUNT; ++unitIndex)
        {
            RevengeUnitRecord *unitRecord =
                &PersistentUnitRoster[unitIndex];
            if (unitRecord->deploymentState != 0x00)
            {
                AdvanceUnitTacticalMovement(unitIndex);
                ScenarioUnitNeedsUpdate[unitIndex] = 0x01;
            }
        }
    }

    KeepTacticalCursorOnSelectedUnit();
    ScrollTacticalViewportToCursor();
    HandleTacticalInput(inputAction);
    RefreshTacticalDisplay(advanceUnits);
}

// 1A1F:0468: void LoadTacticalIconResource(
//     const char far *resourcePath, word16 destinationPage)
// [V] destinationPage selects a 32 KiB page in the secondary graphics buffer.
// Page 1 takes the direct three-buffer loader; the other pages load through the
// packed display buffer and copy into the selected page.
void LoadTacticalIconResource(
    const char *resourcePath,
    uint16_t destinationPage)
{
    uint8_t *packedDisplayBuffer = PrimaryScreenBuffer;
    uint8_t *iconPageDestination = PlanarScreenBuffer;
    uint16_t pageIndex;

    for (pageIndex = 0x00; pageIndex < destinationPage; ++pageIndex)
        iconPageDestination = AdvanceFarPointerBy32K(iconPageDestination);

    CgaNarrowStrideMode = TRUE;
    if (destinationPage == 0x01)
    {
        LoadPackedIconPageDirect(
            resourcePath,
            packedDisplayBuffer,
            iconPageDestination);
    }
    else
    {
        LoadPackedIconResource(resourcePath, packedDisplayBuffer);
        CopyDecodedIconPage(packedDisplayBuffer, iconPageDestination);
    }

    CgaNarrowStrideMode = FALSE;
    /* The EGA's planar copy of the page at A400 (1A1F:04F6-0514,
	 * 0800:3038) went with the EGA. */
}

// 1A1F:03D4: void LoadTacticalIconSetResources(word16 iconSetNumber)
// [V] The assembly adds ASCII '0' and writes the resulting selector into the
// same character position of ICONSETx.ICN and DESTROYx.ICN. It then loads the
// DESTROY sheet (resource 3Eh), ICONSET sheet (3Dh), and ICONLIST.DAT (3Fh).
// Verified against 1A1F:03D4-0467.
void LoadTacticalIconSetResources(uint16_t iconSetNumber)
{
    uint8_t assetSuffix = (uint8_t) iconSetNumber + '0';
    TacticalIconSetFileName[0x07] = assetSuffix;
    TacticalDestroyIconFileName[0x07] = assetSuffix;

    LoadTacticalIconResource(
        TacticalDestroyIconFileName,
        0x02);
    LoadTacticalIconResource(
        TacticalIconSetFileName,
        0x00);

    int16_t iconListHandle = OpenTrackedFile(
        TacticalIconListFileName,
        0x01);
    SeekTrackedFile(iconListHandle, (int32_t) iconSetNumber * 0x0300, 0x00);
    ReadTrackedFile(
        iconListHandle,
        TerrainDamageCapabilityTable,
        0x0300);
    CloseTrackedFile(iconListHandle);
}

// 1A1F:051B: void LoadTacticalMap(word16 mapNumber)
// [V] MAP1..MAP9 use '1'..'9'; values 0Ah and above use 'A' onward. The
// remainder of the routine opens resource 3Ch, reads its 0Ch-byte header, and
// derives MapWidth, MapHeight, and PackedTerrainDamageRowBytes.
// Verified against 1A1F:051B-0612.
void LoadTacticalMap(uint16_t mapNumber)
{
    uint8_t *mapBuffer = TerrainTypeMap;
    int16_t mapFileHandle;
    uint16_t mapWidth;
    uint16_t mapHeight;
    uint16_t negativeMapWidth;
    uint16_t mapCellByteCount;

    TacticalMapFileName[0x03] =
        (uint8_t) mapNumber + ((int16_t) mapNumber < 0x0A ? '0' : '7');

    mapFileHandle = OpenTrackedFile(
        TacticalMapFileName,
        0x01);
    ReadTrackedFile(mapFileHandle, mapBuffer, 0x0C);

    // The header stores the map-cell dimensions at bytes 00h and 02h. The
    // payload immediately following it is one terrain-type byte per cell.
    mapWidth = mapBuffer[0x00];
    mapHeight = mapBuffer[0x02];
    MapWidth = mapWidth;
    MapHeight = mapHeight;
    /* 1A1F:058F-05AC: DS:7A9B-7ACB are the line tracer's row steps
	 * (4184:08FB-092B); entry 3 of the primary table (4184:0901) is also the
	 * row width the trace and the terrain builder read. */
    negativeMapWidth = (uint16_t) (0x00 - mapWidth);
    LineSecondaryRowOffset[0x06] = (int16_t) negativeMapWidth;  /* DS:7ACB */
    LineSecondaryRowOffset[0x01] = (int16_t) negativeMapWidth;  /* DS:7AC1 */
    LinePrimaryRowOffset[0x07] = (int16_t) negativeMapWidth;    /* DS:7AA9 */
    LinePrimaryRowOffset[0x00] = (int16_t) negativeMapWidth;    /* DS:7A9B */
    LineSecondaryRowOffset[0x05] = (int16_t) mapWidth;          /* DS:7AC9 */
    LineSecondaryRowOffset[0x02] = (int16_t) mapWidth;          /* DS:7AC3 */
    LinePrimaryRowOffset[0x04] = (int16_t) mapWidth;            /* DS:7AA3 */
    LinePrimaryRowOffset[0x03] = (int16_t) mapWidth;            /* DS:7AA1 */

    // Eight signed linear neighbor deltas. The static +/-1 entries at indices
    // two and six are not rewritten by this procedure.
    NeighborMapOffsetTable[0x00] = (int16_t) negativeMapWidth;
    NeighborMapOffsetTable[0x01] = (int16_t) (negativeMapWidth + 0x01);
    NeighborMapOffsetTable[0x03] = (int16_t) (mapWidth + 0x01);
    NeighborMapOffsetTable[0x04] = (int16_t) mapWidth;
    NeighborMapOffsetTable[0x05] = (int16_t) (mapWidth - 0x01);
    NeighborMapOffsetTable[0x07] = (int16_t) (negativeMapWidth - 0x01);

    PackedTerrainDamageRowBytes =
        ((mapWidth + 0x03) & 0x01FC) >> 0x02;
    mapCellByteCount = mapWidth * mapHeight;

    ReadTrackedFile(
        mapFileHandle,
        mapBuffer,
        (int32_t) (int16_t) mapCellByteCount);
    CloseTrackedFile(mapFileHandle);
}

// 1A1F:0613: void RunTacticalInteractionLoop(void)
// Verified against ASM 1A1F:0613-0C32. This deliberately remains one
// original procedure; the former Rejected session/tick/finalization split is
// evidence only. The uninitialized enemyForceDefeated stack word is retained
// pending an execution trace of the game-options exit path.
void RunTacticalInteractionLoop(void)
{
    uint16_t scanUnitIndex;
    uint16_t previousInput;
    uint16_t inputAction;
    uint16_t savedFrameCountdown;
    uint16_t advanceUnits;
    uint16_t completionRequested;
    uint16_t enemyForceDefeated = FALSE;
    uint16_t changedEnemyRecord;
    uint16_t unitIndex;

    REVENGE_PRESERVATION_TRACE("tactical: interaction refresh combat begin\n");
    RefreshAllUnitCombatState(TRUE);
    REVENGE_PRESERVATION_TRACE("tactical: interaction refresh combat complete\n");
    TacticalClockDivider = 0x0080;
    scanUnitIndex = 0x0000;
    for (unitIndex = 0x0000; unitIndex < TACTICAL_UNIT_COUNT; ++unitIndex)
    {
        UpdateUnitEnemyVisibility(unitIndex);
        UpdateUnitEnemyProximityState(unitIndex);
    }
    REVENGE_PRESERVATION_TRACE("tactical: interaction visibility complete\n");
    DrawCampaignClock();
    previousInput = 0x0000;
    completionRequested = 0x0000;
    TacticalUpdateDelayTicks = TacticalUpdateDelayReloadTicks;

    while (completionRequested == 0x0000 &&
        TacticalBattleResult == TACTICAL_BATTLE_RUNNING)
    {
        inputAction = 0x0000;
        savedFrameCountdown = TacticalUpdateDelayTicks;
        if (SoundEffectsEnabled == 0x0000)
            RequestPCSpeakerEffectStop();

        if (ServiceMusicAndCheckKeyboardInput() != 0x0000)
        {
            inputAction = (uint16_t) ReadTranslatedKeypress();
            TacticalKeyRepeatDelayTicks = 0x0005;
            if (inputAction == previousInput)
                TacticalMenuInputState = 0x0001;
            else
            {
                TacticalMenuInputState = 0x0000;
                previousInput = inputAction;
            }
            while (ServiceMusicAndCheckKeyboardInput() != 0x0000)
                ReadTranslatedKeypress();
        }
        if (TacticalKeyRepeatDelayTicks == 0x0000)
        {
            previousInput = 0x0000;
            TacticalMenuInputState = 0x0000;
        }

        inputAction = (uint16_t) TranslateTacticalMouseInput(
            (int16_t) inputAction);
        if (inputAction == KEY_ESCAPE)
        {
            if (TacticalOverviewActive != 0x0000)
            {
                if (TacticalOverviewViewportDirty != 0x0000)
                {
                    TacticalOverviewViewportDirty = 0x0000;
                    TacticalOverviewMapMode = TacticalOverviewDisplayMode;
                    DrawTacticalOverviewMap();
                }
                else
                    ExitTacticalOverview();
            }
            else
            {
                TacticalCursorOutlineEnabled = 0x0000;
                RefreshTacticalDisplay(FALSE);
                if (RunGameOptionsMenu(TRUE) == 0x0002)
                    completionRequested = 0x0001;
                TacticalCursorOutlineEnabled = 0x0001;
            }
        }

        advanceUnits = savedFrameCountdown == 0x0000;
        if (TacticalOverviewViewportDirty != 0x0000)
        {
            advanceUnits = FALSE;
            savedFrameCountdown = 0x0001;
        }
        UpdateTacticalFrame(
            (uint16_t) NormalizeNavigationKeyCode((int16_t) inputAction),
            advanceUnits);
        RefreshTacticalStatusSlotStates();

        if (savedFrameCountdown == 0x0000)
        {
            REVENGE_PRESERVATION_TRACE("tactical: tick scan=%u\n", scanUnitIndex);
            for (unitIndex = 0x0000; unitIndex < 0x0006; ++unitIndex)
            {
                UpdateUnitEnemyVisibility(scanUnitIndex);
                UpdateUnitEnemyProximityState(scanUnitIndex);
                if (++scanUnitIndex >= TACTICAL_UNIT_COUNT)
                    scanUnitIndex = 0x0000;
            }
            for (unitIndex = 0x0000; unitIndex < TACTICAL_UNIT_COUNT; unitIndex += LANCE_UNIT_COUNT)
                RebuildLanceFormationAfterLeaderLoss(unitIndex);
            RefreshAllUnitCombatState(FALSE);
            for (unitIndex = 0x0000; unitIndex < TACTICAL_UNIT_COUNT; ++unitIndex)
            {
                if (PersistentUnitRoster[unitIndex].deploymentState != 0x00)
                {
                    ExecuteUnitWeaponAI(unitIndex);
                }
            }
            AdvanceScenarioAreaEffects();
            ExecuteSceneInstructions();
            ++TacticalClockSubtick;
            TacticalClockDivider =
                (uint16_t) ((int16_t) TacticalClockDivider >> 0x01);
            if (TacticalClockDivider == 0x0000)
            {
                TacticalClockDivider = 0x0080;
                AdvanceTacticalCampaignClock();
                for (unitIndex = 0x0000; unitIndex < TACTICAL_UNIT_COUNT; ++unitIndex)
                {
                    if (UnitWasAttackedThisTick[unitIndex] != 0x00)
                        ++UnitConsecutiveAttackTicks[unitIndex];
                    else
                        UnitConsecutiveAttackTicks[unitIndex] = 0x0000;
                    UnitJumpingState[unitIndex] = 0x00;
                    UnitWasAttackedThisTick[unitIndex] = 0x00;
                    UnitJumpMovementRemaining[unitIndex] =
                        PersistentUnitRoster[unitIndex].jumpMovement;
                    UnitShutdownState[unitIndex] =
                        UpdateUnitHeatShutdown(unitIndex);
                }
            }
            UpdateUnitHeatAccumulators();
        }

        if (savedFrameCountdown == 0x0000)
            TacticalUpdateDelayTicks = TacticalUpdateDelayReloadTicks;
        if (completionRequested == 0x0000)
        {
            completionRequested = 0x0001;
            for (unitIndex = 0x0000; unitIndex < TACTICAL_FORCE_UNIT_COUNT; ++unitIndex)
            {
                if (PersistentUnitRoster[unitIndex].deploymentState != 0x00)
                    completionRequested = 0x0000;
            }
            enemyForceDefeated = 0x0001;
            for (unitIndex = ENEMY_FORCE_FIRST_UNIT; unitIndex < TACTICAL_UNIT_COUNT; ++unitIndex)
            {
                if (PersistentUnitRoster[unitIndex].deploymentState != 0x00 &&
                    PersistentUnitRoster[unitIndex].walkMovement != 0x00)
                {
                    enemyForceDefeated = 0x0000;
                }
            }
            if (enemyForceDefeated != 0x0000)
                TacticalBattleResult = TACTICAL_BATTLE_WON;
        }
        if (PersistentUnitRoster[0x00].deploymentState == 0x00)
            completionRequested = 0x0001;
    }

    if (PersistentUnitRoster[0x00].deploymentState == 0x00)
        TacticalBattleResult = TACTICAL_BATTLE_ENDED;
    if (ScenarioFileSuffix == 0x0053 &&
        PersistentUnitRoster[0x14].deploymentState != 0x00 &&
        PersistentUnitRoster[0x14].walkMovement == 0x00)
    {
        CampaignFlags &= (uint16_t) ~CAMPAIGN_FLAG_MILESTONE_UNKNOWN_05;
        CampaignFlags |= CAMPAIGN_FLAG_COMMANDER_CRIPPLED;
    }
    if (PendingNarrationDelay != 0x0000 &&
        (int16_t) NarrationMessageCategory >= 0x000B &&
        (int16_t) NarrationMessageCategory <= 0x0010)
    {
        unitIndex = ActiveNarrationSubject;
        if (PersistentUnitRoster[unitIndex].deploymentState == 0x00)
        {
            ActiveNarrationSubject = 0x0000;
        }
        ShowPendingNarrationPrompt();
        enemyForceDefeated = 0x0000;
    }
    if (TacticalBattleResult == TACTICAL_BATTLE_WON &&
        enemyForceDefeated != 0x0000)
    {
        changedEnemyRecord = 0x0000;
        for (unitIndex = ENEMY_FORCE_FIRST_UNIT; unitIndex < TACTICAL_UNIT_COUNT; ++unitIndex)
        {
            if (PersistentUnitRoster[unitIndex].deploymentState != 0x00 &&
                PersistentUnitRoster[unitIndex].walkMovement == 0x00)
            {
                changedEnemyRecord = 0x0001;
                NarrationSourceUnit = unitIndex;
                PersistentUnitRoster[unitIndex].deploymentState = 0x00;
                UnitDestructionMarkers[unitIndex][0x00] =
                    ScenarioUnitMapX[unitIndex];
                UnitDestructionMarkers[unitIndex][0x01] =
                    ScenarioUnitMapY[unitIndex];
                UnitDestructionMarkers[unitIndex][0x02] = 0xDD;
            }
        }
        if (changedEnemyRecord != 0x0000)
        {
            TacticalSelectedUnitIndex = TACTICAL_UNIT_NONE;
            RefreshTacticalDisplay(FALSE);
            PendingNarrationDelay = NARRATION_DELAY_MS;
            ActiveNarrationSubject = 0x0000;
            NarrationMessageCategory = 0x0009;
            NarrationEmphasis = 0x0000;
        }
    }
    if (PendingNarrationDelay != 0x0000 &&
        (NarrationMessageCategory == 0x0009 ||
            NarrationMessageCategory == 0x0012))
    {
        ShowPendingNarrationPrompt();
    }

    for (unitIndex = 0x0000;
        (int16_t) unitIndex < (int16_t) TacticalProjectileCount;
        ++unitIndex)
    {
        RevengeTacticalProjectileRecord *projectile =
            &TacticalProjectileRecords[unitIndex];
        if (projectile->shotHit != 0x00)
        {
            int16_t sourceIndex = projectile->targetUnitIndex;
            projectile->destinationMapX =
                ScenarioUnitMapX[sourceIndex];
            projectile->destinationMapY =
                ScenarioUnitMapY[sourceIndex];
        }
        projectile->shotHit = 0x00;
    }
    if (TacticalOverviewActive == 0x0000)
    {
        while (TacticalImpactQueueCount != 0x0000 ||
            TacticalProjectileCount != 0x0000)
        {
            UpdateTacticalFrame(0x0000, 0x0000);
        }
    }
    if (PersistentUnitRoster[0x00].deploymentState == 0x00)
        TacticalBattleResult = TACTICAL_BATTLE_ENDED;
    if (TacticalOverviewActive != 0x0000)
    {
        for (unitIndex = 0x0000; unitIndex < 0x0009; ++unitIndex)
            TacticalStatusRowInvalidation[unitIndex] = 0xFF;
        RedrawTacticalStatusPanel();
        ShowMouseCursorOverlay();
    }

    TacticalOverviewActive = 0x0000;
    TacticalProjectileCount = 0x0000;
    TacticalImpactQueueCount = 0x0000;
    MapEdgeEffectCount = 0x0000;
    TerrainDamageQueueWordCount = 0x0000;
    if (PersistentUnitRoster[0x00].deploymentState == 0x00)
        CampaignFlags |= CAMPAIGN_FLAG_BATTLE_LOST;
    if (PersistentUnitRoster[0x01].deploymentState == 0x00)
        CampaignFlags |= CAMPAIGN_FLAG_UNIT_1_LOST;
    if (ScenarioFileSuffix == 0x0037 &&
        PersistentUnitRoster[0x01].deploymentState == 0x00)
    {
        CampaignFlags |= 0x0010;
    }
    if (CampaignClockState[0x00] >= 0x0C)
        CampaignFlags |= CAMPAIGN_FLAG_CLOCK_PAST_LIMIT;
    if ((ScenarioFileSuffix == 0x0036 ||
            ScenarioFileSuffix == 0x0037) &&
        (CampaignClockState[0x00] > 0x07 ||
            (CampaignClockState[0x00] == 0x07 &&
                CampaignClockState[0x01] >= 0x1E)))
    {
        CampaignFlags |= CAMPAIGN_FLAG_CLOCK_PAST_LIMIT;
    }
    TacticalClockSubtick = 0x00;
    if (completionRequested != 0x0000)
        TacticalBattleResult = TACTICAL_BATTLE_ENDED;
    switch (TacticalBattleResult)
    {
    case TACTICAL_BATTLE_WON:
        CampaignFlags |= CAMPAIGN_FLAG_BATTLE_WON;
        break;
    case TACTICAL_BATTLE_EXIT_TO_DOS:
        VideoPathUnavailable = 0x01;
        break;
    default:
        break;
    }
    if ((PersistentUnitRoster[0x00].deploymentState == 0x00 ||
            (ScenarioFileSuffix == 0x0035 &&
                PersistentUnitRoster[0x00].walkMovement == 0x00)) &&
        PersistentUnitRoster[LastAttackingUnitIndex].damageModel ==
            UNIT_DAMAGE_MODEL_BATTLEMECH)
    {
        CampaignFlags |= CAMPAIGN_FLAG_LOST_TO_BATTLEMECH;
    }
    /* DOS finished the effect from IRQ0 while this loop spun; the port
	 * services its timers here, as the other waits on the effect do. */
    while (DigitalSoundBusy != 0x00)
        SDLBackend_ServiceTimers();
}

// 1A1F:10CD: void LoadUnitTemplateRecord(short unitType)
// VERIFIED from assembly: unitType is the sole stack argument at BP+06. SI is
// assigned the opened file handle inside this routine and is not an argument.
// The routine reads exactly one 0x96-byte template into DS:84C7.
void LoadUnitTemplateRecord(int16_t unitType)
{
    char *templatePath = UnitTemplatePath;
    int16_t fileHandle = OpenTrackedFile(templatePath, TRACKED_FILE_OPEN_READ);
    int32_t fileOffset = (int32_t) unitType * UNIT_RECORD_SIZE;

    SeekTrackedFile(fileHandle, fileOffset, 0x00);
    ReadTrackedFile(
        fileHandle,
        &EstimatedUnitRecord,
        UNIT_RECORD_SIZE);
    CloseTrackedFile(fileHandle);
}

// 1A1F:1132: void InitializeTacticalUnitRuntime(void)
// Verified against 1A1F:1132-11F6.
void InitializeTacticalUnitRuntime(void)
{
    uint16_t unitIndex;
    uint16_t deployedUnitCount;

    UnitTypeResourceCode[0x04] = 0xED;
    TacticalSlotSpriteState[0x04] = 0x00;
    for (unitIndex = 0x00; unitIndex < 0x04; ++unitIndex)
    {
        RevengeUnitRecord *unitRecord =
            &PersistentUnitRoster[unitIndex];
        TacticalSlotSpriteState[unitIndex] = 0x00;
        if (unitRecord->deploymentState == 0x00)
        {
            UnitTypeResourceCode[unitIndex] = 0x00;
            TacticalSlotSpriteState[unitIndex] = 0x06;
        }
    }

    if (TacticalScenarioLanceEnabled[0x01] != 0x00)
    {
        ScenarioDestructionCompletionState = 0x00;
        deployedUnitCount = 0x00;
        for (unitIndex = 0x04; unitIndex < 0x08; ++unitIndex)
        {
            RevengeUnitRecord *unitRecord =
                &PersistentUnitRoster[unitIndex];
            if (unitRecord->deploymentState != 0x00)
                ++deployedUnitCount;
        }
        if (deployedUnitCount == 0x00)
        {
            UnitTypeResourceCode[0x05] = 0x00;
            ScenarioDestructionCompletionState = 0x06;
        }
    }

    if (TacticalScenarioLanceEnabled[0x02] != 0x00)
    {
        TacticalSlotSpriteState[0x06] = 0x00;
        deployedUnitCount = 0x00;
        for (unitIndex = 0x08; unitIndex < ENEMY_FORCE_FIRST_UNIT; ++unitIndex)
        {
            RevengeUnitRecord *unitRecord =
                &PersistentUnitRoster[unitIndex];
            if (unitRecord->deploymentState != 0x00)
                ++deployedUnitCount;
        }
        if (deployedUnitCount == 0x00)
        {
            UnitTypeResourceCode[0x06] = 0x00;
            TacticalSlotSpriteState[0x06] = 0x06;
        }
    }

    if (TacticalScenarioLanceEnabled[0x00] != 0x00)
        PrimaryLanceResourceCode = 0xEB;
}

// 1A1F:18C9: void OffsetWeaponTraceEndpoint(
//     int16 *sourceX, int16 *sourceY,
//     int16 *targetX, int16 *targetY)
// Verified against 1A1F:18C9-19FC.
void OffsetWeaponTraceEndpoint(
    int16_t *sourceX,
    int16_t *sourceY,
    int16_t *targetX,
    int16_t *targetY)
{
    int16_t deltaY = (int16_t) (uint16_t) ((uint16_t) *targetY - (uint16_t) *sourceY);
    int16_t deltaX = (int16_t) (uint16_t) ((uint16_t) *sourceX - (uint16_t) *targetX);
    uint16_t octant = DetermineOctant(deltaY, deltaX);

    switch (octant)
    {
    case 0x00:
    case 0x07:
        *targetX = InterpolateLineCoordinate(
            (int16_t) (uint16_t) ((uint16_t) *targetY - 0x0008),
            *targetY, *sourceX, *targetX, *sourceY);
        *targetY = (int16_t) (uint16_t) ((uint16_t) *targetY - 0x0008);
        break;

    case 0x01:
    case 0x02:
        *targetY = InterpolateLineCoordinate(
            (int16_t) (uint16_t) ((uint16_t) *targetX + 0x0007),
            *targetX, *sourceY, *targetY, *sourceX);
        *targetX = (int16_t) (uint16_t) ((uint16_t) *targetX + 0x0007);
        break;

    case 0x03:
    case 0x04:
        *targetX = InterpolateLineCoordinate(
            (int16_t) (uint16_t) ((uint16_t) *targetY + 0x0007),
            *targetY, *sourceX, *targetX, *sourceY);
        *targetY = (int16_t) (uint16_t) ((uint16_t) *targetY + 0x0007);
        break;

    case 0x05:
    case 0x06:
        *targetY = InterpolateLineCoordinate(
            (int16_t) (uint16_t) ((uint16_t) *targetX - 0x0008),
            *targetX, *sourceY, *targetY, *sourceX);
        *targetX = (int16_t) (uint16_t) ((uint16_t) *targetX - 0x0008);
        break;
    }
}

// 1A1F:19FD: void CreateRandomAreaImpact(
//     short centerX, short centerY, word16 chanceDenominator)
// Verified against 1A1F:19FD-1A82.
void CreateRandomAreaImpact(
    int16_t centerX,
    int16_t centerY,
    uint16_t chanceDenominator)
{
    if (RandomInRangeInclusive(0x01, chanceDenominator) !=
        chanceDenominator)
    {
        return;
    }

    int16_t mapX = centerX + RandomInRangeInclusive(0x00, 0x03) - 0x02;
    int16_t mapY = centerY + RandomInRangeInclusive(0x00, 0x03) - 0x02;
    if (mapX < 0x00)
        mapX = 0x00;
    if (mapY < 0x00)
        mapY = 0x00;
    if (mapX >= MapWidth)
        mapX = MapWidth - 0x01;
    if (mapY >= MapHeight)
        mapY = MapHeight - 0x01;

    ApplyAreaTerrainAndUnitDamage(
        mapX,
        mapY,
        0x01,
        (GeneratePseudoRandomByte() & 0x07) + 0x03);
}

// 1A1F:1A83: void AdvanceScenarioAreaEffects(void)
// Verified against 1A1F:1A83-1C26.
void AdvanceScenarioAreaEffects(void)
{
    int16_t direction;
    int16_t mapX;
    int16_t mapY;

    if (ArtilleryShellingActive != 0x00)
    {
        CreateRandomAreaImpact(
            ArtilleryTargetMapX,
            ArtilleryTargetMapY,
            ArtillerySupportAvailable);
    }

    if (RollingBarrageActive != 0x00)
    {
        CreateRandomAreaImpact(
            RollingBarrageStartMapX,
            RollingBarrageStartMapY,
            ArtillerySupportAvailable);
        if (RollingBarrageStartMapX == RollingBarrageEndMapX &&
            RollingBarrageStartMapY == RollingBarrageEndMapY)
        {
            RollingBarrageStartMapX = ArtilleryTargetMapX;
            RollingBarrageStartMapY = ArtilleryTargetMapY;
        }
        else
        {
            direction = (int16_t) DetermineRelativeAttackDirection(
                RollingBarrageStartMapX,
                RollingBarrageStartMapY,
                RollingBarrageEndMapX,
                RollingBarrageEndMapY);
            if (direction >= 0x00 && direction <= 0x07)
            {
                if (RollingBarrageStartMapX != RollingBarrageEndMapX)
                {
                    RollingBarrageStartMapX +=
                        (uint8_t) DirectionDeltaXTable[direction];
                }
                if (RollingBarrageStartMapY != RollingBarrageEndMapY)
                {
                    RollingBarrageStartMapY +=
                        (uint8_t) DirectionDeltaYTable[direction];
                }
            }
        }
    }

    if (StrafingRunActive == 0x00 ||
        MapEdgeEffectCount != 0x00 ||
        RandomInRangeInclusive(0x00, StrafingSupportAvailable) == 0x00)
    {
        return;
    }

    direction = (int16_t) (GeneratePseudoRandomByte() & 0x03);
    switch (direction)
    {
    case 0x00:
        mapX = (int16_t) (uint16_t) (RollingBarrageEndMapX +
            (GeneratePseudoRandomByte() & 0x03) - 0x02);
        mapY = (int16_t) (uint16_t) (MapHeight + 0x01);
        break;

    case 0x01:
        mapX = -0x01;
        mapY = (int16_t) (uint16_t) (RollingBarrageEndMapY +
            (GeneratePseudoRandomByte() & 0x03) - 0x02);
        break;

    case 0x02:
        mapX = (int16_t) (uint16_t) (RollingBarrageEndMapX +
            (GeneratePseudoRandomByte() & 0x03) - 0x02);
        mapY = -0x01;
        break;

    default:
        mapX = (int16_t) (uint16_t) (MapWidth + 0x01);
        mapY = (int16_t) (uint16_t) (RollingBarrageEndMapY +
            (GeneratePseudoRandomByte() & 0x03) - 0x02);
        break;
    }
    QueueMapEdgeEffect(mapX, mapY, 0x01C0, (uint16_t) direction);
}
