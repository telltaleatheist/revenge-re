// 2208: targeting
// Targeting.cpp
// Procedures preserved from REVENGE_2208_TACTICAL_INTERACTION.c.

#include "CommandsInternal.h"
#include "Data/StartupData.h"
#include "Data/RosterData.h"
#include "Data/TacticalControlData.h"
#include "Data/TacticalMapData.h"
#include "Data/TacticalStatusData.h"
#include "Data/TacticalUnitData.h"
#include "Data/TacticalTables.h"
#include "Data/TextIOData.h"
#include "Graphics/Graphics.h"
#include "Input/Input.h"
#include "Runtime/Runtime.h"
#include "Tactical/Tactical.h"
#include "Text/Text.h"
#include "revenge/tactical_panel_data.h"
#include "revenge/debug.h"

// 2208:060D
void SelectTacticalMapCoordinate(uint16_t unitIndex)
{
    uint16_t savedSelectedUnit;
    uint16_t savedCursorColumn;
    uint16_t savedViewportX;
    uint16_t savedViewportY;
    uint16_t savedCursorX;
    uint16_t savedCursorY;
    uint16_t cursorMapX;
    uint16_t cursorMapY;
    uint8_t savedAttackTarget;
    uint8_t savedTargetMapX;
    uint8_t savedTargetMapY;
    int16_t inputAction;
    int16_t maximumViewportX;
    int16_t maximumViewportY;

    HideMouseCursorOverlay();
    savedViewportX = TacticalViewportMapX;
    savedViewportY = TacticalViewportMapY;
    savedCursorX = TacticalCursorViewportX;
    savedCursorY = TacticalCursorViewportY;
    cursorMapX = savedViewportX + savedCursorX;
    cursorMapY = savedViewportY + savedCursorY;

    TacticalViewportMapX += savedCursorX - 0x0A;
    if ((int16_t) savedCursorX < 0x0A &&
        (int16_t) TacticalViewportMapX >= 0x00)
    {
        TacticalCursorViewportX = 0x0A;
    }
    if ((int16_t) savedCursorX > 0x0A &&
        (int16_t) (MapWidth - TACTICAL_VIEWPORT_WIDTH_CELLS) >=
            (int16_t) TacticalViewportMapX)
    {
        TacticalCursorViewportX = 0x0A;
    }
    maximumViewportX = (int16_t) MapWidth - TACTICAL_VIEWPORT_WIDTH_CELLS;
    if ((int16_t) TacticalViewportMapX < 0x00)
        TacticalViewportMapX = 0x00;
    if (maximumViewportX <= (int16_t) TacticalViewportMapX)
        TacticalViewportMapX = (uint16_t) maximumViewportX;

    TacticalViewportMapY += savedCursorY - 0x06;
    if ((int16_t) savedCursorY < 0x06 &&
        (int16_t) TacticalViewportMapY >= 0x00)
    {
        TacticalCursorViewportY = 0x06;
    }
    if ((int16_t) savedCursorY > 0x06 &&
        (int16_t) (MapHeight - TACTICAL_VIEWPORT_HEIGHT_CELLS) >=
            (int16_t) TacticalViewportMapY)
    {
        TacticalCursorViewportY = 0x06;
    }
    maximumViewportY = (int16_t) MapHeight - TACTICAL_VIEWPORT_HEIGHT_CELLS;
    if ((int16_t) TacticalViewportMapY < 0x00)
        TacticalViewportMapY = 0x00;
    if (maximumViewportY <= (int16_t) TacticalViewportMapY)
        TacticalViewportMapY = (uint16_t) maximumViewportY;

    TacticalCursorViewportX =
        cursorMapX - TacticalViewportMapX;
    TacticalCursorViewportY =
        cursorMapY - TacticalViewportMapY;
    if ((int16_t) TacticalCursorViewportX < 0x00)
        TacticalCursorViewportX = 0x00;
    if ((int16_t) TacticalCursorViewportX >= TACTICAL_VIEWPORT_WIDTH_CELLS)
        TacticalCursorViewportX = 0x12;
    if ((int16_t) TacticalCursorViewportY < 0x00)
        TacticalCursorViewportY = 0x00;
    if ((int16_t) TacticalCursorViewportY >= TACTICAL_VIEWPORT_HEIGHT_CELLS)
        TacticalCursorViewportY = 0x0B;

    savedSelectedUnit = TacticalSelectedUnitIndex;
    TacticalSelectedUnitIndex = TACTICAL_UNIT_NONE;
    savedCursorColumn = ActiveTextCursorColumn;
    savedAttackTarget = UnitAttackTarget[unitIndex];
    savedTargetMapX = UnitTargetMapX[unitIndex];
    savedTargetMapY = UnitTargetMapY[unitIndex];

    do
    {
        RefreshTacticalCoordinateSelection(
            unitIndex, 0x00, savedCursorColumn);
        inputAction = 0x00;
        if (ServiceMusicAndCheckKeyboardInput() != FALSE)
        {
            inputAction = ReadTranslatedKeypress();
            while (ServiceMusicAndCheckKeyboardInput() != FALSE)
                (void) ReadTranslatedKeypress();
        }
        inputAction = NormalizeNavigationKeyCode(
            TranslateTacticalMouseInput(inputAction));

        switch (inputAction)
        {
        case (int16_t) (KEY_PAGE_DOWN - 0x0100):
        case (int16_t) (KEY_DOWN - 0x0100):
        case (int16_t) (KEY_END - 0x0100):
        case (int16_t) (KEY_RIGHT - 0x0100):
        case (int16_t) (KEY_LEFT - 0x0100):
        case (int16_t) (KEY_PAGE_UP - 0x0100):
        case (int16_t) (KEY_UP - 0x0100):
        case (int16_t) (KEY_HOME - 0x0100):
            MoveTacticalCursorByInput(inputAction);
            break;

        case (int16_t) (KEY_F9 - 0x0100):
        case '+':
            if (unitIndex < ENEMY_FORCE_FIRST_UNIT)
                CycleTacticalEnemyTarget();
            break;
        }
    } while (inputAction != KEY_ESCAPE &&
        inputAction != KEY_SPACE &&
        inputAction != KEY_CARRIAGE_RETURN);

    if (inputAction == KEY_ESCAPE)
    {
        UnitAttackTarget[unitIndex] = savedAttackTarget;
        UnitTargetMapX[unitIndex] = savedTargetMapX;
        UnitTargetMapY[unitIndex] = savedTargetMapY;
    }
    TacticalSelectedUnitIndex = savedSelectedUnit;
    TacticalViewportMapX = savedViewportX;
    TacticalViewportMapY = savedViewportY;
    TacticalCursorViewportX = savedCursorX;
    TacticalCursorViewportY = savedCursorY;
    ShowMouseCursorOverlay();
}

// 2208:0857: rebuild the tactical viewport while the player selects an attack
// coordinate or movement route, draw the appropriate cursor/route overlays,
// and refresh the coordinate or contact text at the saved text column.
void RefreshTacticalCoordinateSelection(
    uint16_t unitIndex,
    uint16_t selectionMode,
    uint16_t savedCursorColumn)
{
    uint16_t savedCursorMapX;
    uint16_t savedCursorMapY;
    uint16_t savedUnitMapX;
    uint16_t savedUnitMapY;
    uint16_t previousRouteX;
    uint16_t previousRouteY;
    uint16_t routeX;
    uint16_t routeY;
    int16_t routeIndex;
    uint16_t targetFound;
    uint16_t enemyUnitIndex;
    uint16_t cursorColorPhase;
    int16_t maximumViewportX;
    int16_t maximumViewportY;
    SceneUnitRoute *route;
    RevengeUnitRecord *unitRecord;

    if (TacticalCursorAutoScroll != FALSE)
    {
        ScrollTacticalViewportToCursor();
    }
    else
    {
        TacticalViewportMapX +=
            TacticalCursorViewportX - 0x0A;
        if ((int16_t) TacticalCursorViewportX < 0x0A &&
            (int16_t) TacticalViewportMapX >= 0x00)
            TacticalCursorViewportX = 0x0A;
        if ((int16_t) TacticalCursorViewportX > 0x0A &&
            (int16_t) (MapWidth - TACTICAL_VIEWPORT_WIDTH_CELLS) >=
                (int16_t) TacticalViewportMapX)
            TacticalCursorViewportX = 0x0A;
        maximumViewportX = (int16_t) MapWidth - TACTICAL_VIEWPORT_WIDTH_CELLS;
        if ((int16_t) TacticalViewportMapX < 0x00)
            TacticalViewportMapX = 0x00;
        if (maximumViewportX <= (int16_t) TacticalViewportMapX)
            TacticalViewportMapX = (uint16_t) maximumViewportX;

        TacticalViewportMapY +=
            TacticalCursorViewportY - 0x06;
        if ((int16_t) TacticalCursorViewportY < 0x06 &&
            (int16_t) TacticalViewportMapY >= 0x00)
            TacticalCursorViewportY = 0x06;
        if ((int16_t) TacticalCursorViewportY > 0x06 &&
            (int16_t) (MapHeight - TACTICAL_VIEWPORT_HEIGHT_CELLS) >=
                (int16_t) TacticalViewportMapY)
            TacticalCursorViewportY = 0x06;
        maximumViewportY = (int16_t) MapHeight - TACTICAL_VIEWPORT_HEIGHT_CELLS;
        if ((int16_t) TacticalViewportMapY < 0x00)
            TacticalViewportMapY = 0x00;
        if (maximumViewportY <= (int16_t) TacticalViewportMapY)
            TacticalViewportMapY = (uint16_t) maximumViewportY;

        if ((int16_t) TacticalCursorViewportX < 0x00)
            TacticalCursorViewportX = 0x00;
        if ((int16_t) TacticalCursorViewportX >= TACTICAL_VIEWPORT_WIDTH_CELLS)
            TacticalCursorViewportX = 0x12;
        if ((int16_t) TacticalCursorViewportY < 0x00)
            TacticalCursorViewportY = 0x00;
        if ((int16_t) TacticalCursorViewportY >= TACTICAL_VIEWPORT_HEIGHT_CELLS)
            TacticalCursorViewportY = 0x0B;
    }

    RebuildTacticalViewport();
    cursorColorPhase = GameAnimationTicks & 0x0008;
    route = &SceneUnitRoutes[unitIndex];

    if (selectionMode != FALSE)
    {
        savedCursorMapX = TacticalCursorViewportX +
            TacticalViewportMapX;
        savedCursorMapY = TacticalCursorViewportY +
            TacticalViewportMapY;
        UnitRouteTargetMapX[unitIndex] = (uint8_t) savedCursorMapX;
        UnitRouteTargetMapY[unitIndex] = (uint8_t) savedCursorMapY;

        TacticalCursorViewportX =
            ScenarioUnitMapX[unitIndex] -
            TacticalViewportMapX;
        TacticalCursorViewportY =
            ScenarioUnitMapY[unitIndex] -
            TacticalViewportMapY;
        if ((int16_t) TacticalCursorViewportX >= 0x00 &&
            (int16_t) TacticalCursorViewportX < TACTICAL_VIEWPORT_WIDTH_CELLS &&
            (int16_t) TacticalCursorViewportY >= 0x00 &&
            (int16_t) TacticalCursorViewportY < TACTICAL_VIEWPORT_HEIGHT_CELLS)
        {
            DrawTacticalCursorOutline(
                cursorColorPhase != FALSE ? 0x0F : 0x0C);
        }

        for (routeIndex = 0x0000;
            routeIndex <= route->waypointIndex;
            ++routeIndex)
        {
            TacticalCursorViewportX =
                route->waypointMapX[routeIndex] -
                TacticalViewportMapX;
            TacticalCursorViewportY =
                route->waypointMapY[routeIndex] -
                TacticalViewportMapY;
            if ((int16_t) TacticalCursorViewportX >= 0x00 &&
                (int16_t) TacticalCursorViewportX < TACTICAL_VIEWPORT_WIDTH_CELLS &&
                (int16_t) TacticalCursorViewportY >= 0x00 &&
                (int16_t) TacticalCursorViewportY < TACTICAL_VIEWPORT_HEIGHT_CELLS)
            {
                DrawTacticalCursorOutline(
                    cursorColorPhase != FALSE ? 0x0E : 0x0D);
            }
        }
        TacticalCursorViewportX =
            savedCursorMapX - TacticalViewportMapX;
        TacticalCursorViewportY =
            savedCursorMapY - TacticalViewportMapY;
    }
    else
    {
        UnitTargetMapX[unitIndex] = (uint8_t) (TacticalCursorViewportX +
            TacticalViewportMapX);
        UnitTargetMapY[unitIndex] = (uint8_t) (TacticalCursorViewportY +
            TacticalViewportMapY);
        if ((int16_t) TacticalCursorViewportX >= 0x00 &&
            (int16_t) TacticalCursorViewportX < TACTICAL_VIEWPORT_WIDTH_CELLS &&
            (int16_t) TacticalCursorViewportY >= 0x00 &&
            (int16_t) TacticalCursorViewportY < TACTICAL_VIEWPORT_HEIGHT_CELLS)
        {
            DrawTacticalCursorOutline(
                cursorColorPhase != FALSE ? 0x08 : 0x05);
        }
    }

    HideMouseCursorOverlay();
    if (selectionMode == FALSE)
    {
        targetFound = TRUE;
        UnitTacticalTarget[unitIndex] = TACTICAL_TARGET_NONE;
        if (TacticalCellSpriteMap[TacticalCursorViewportY * 0x13 +
                TacticalCursorViewportX] != 0x0000)
        {
            for (enemyUnitIndex = ENEMY_FORCE_FIRST_UNIT;
                enemyUnitIndex < TACTICAL_UNIT_COUNT;
                ++enemyUnitIndex)
            {
                unitRecord = &PersistentUnitRoster[enemyUnitIndex];
                if (unitRecord->deploymentState != FALSE &&
                    ScenarioUnitMapX[enemyUnitIndex] ==
                        TacticalViewportMapX +
                            TacticalCursorViewportX &&
                    ScenarioUnitMapY[enemyUnitIndex] ==
                        TacticalViewportMapY +
                            TacticalCursorViewportY)
                {
                    targetFound = FALSE;
                    UnitAttackTarget[unitIndex] =
                        (uint8_t) enemyUnitIndex;
                    UnitTacticalTarget[unitIndex] =
                        (uint8_t) enemyUnitIndex;
                    enemyUnitIndex = TACTICAL_UNIT_COUNT;
                }
            }
        }
        if (unitIndex == TACTICAL_UNIT_COUNT)
            targetFound = TRUE;

        ActiveTextCursorColumn = savedCursorColumn;
        ActiveTextCursorRow = 0x0000;
        if (targetFound != FALSE)
        {
            FormatTacticalMapCoordinates(
                UnitTargetMapX[unitIndex],
                UnitTargetMapY[unitIndex]);
            RenderFormattedText(SharedNumericTextBuffer);
            ActiveTextBackgroundColor = 0x000C;
            ActiveTextForegroundColor = 0x000C;
            while (ActiveTextCursorColumn < 0x0026)
                RenderFormattedText(TacticalPaddingSpace);
            ActiveTextForegroundColor = 0x000F;
            UnitTacticalTarget[unitIndex] = TACTICAL_TARGET_MAP_CELL;
            UnitAttackTarget[unitIndex] = TACTICAL_TARGET_MAP_CELL;
        }
        else
        {
            enemyUnitIndex = UnitTacticalTarget[unitIndex];
            ActiveTextForegroundColor = 0x0008;
            unitRecord = &PersistentUnitRoster[enemyUnitIndex];
            RenderFormattedText(UnitNameTable[unitRecord->unitTypeId]);
            RenderFormattedText(TacticalStatusSeparator);
            FormatUnitDurabilityPercentage(enemyUnitIndex);
            RenderFormattedText(SharedNumericTextBuffer);
            ActiveTextBackgroundColor = 0x000C;
            ActiveTextForegroundColor = 0x000C;
            while (ActiveTextCursorColumn < 0x0026)
                RenderFormattedText(TacticalStatusPadding);
            ActiveTextForegroundColor = 0x000F;
        }
    }
    else
    {
        ActiveTextCursorColumn = savedCursorColumn;
        ActiveTextCursorRow = 0x0000;
        FormatTacticalMapCoordinates(
            TacticalCursorViewportX +
                TacticalViewportMapX,
            TacticalCursorViewportY +
                TacticalViewportMapY);
        RenderFormattedText(SharedNumericTextBuffer);

        savedCursorMapX = TacticalCursorViewportX;
        savedCursorMapY = TacticalCursorViewportY;
        savedUnitMapX = ScenarioUnitMapX[unitIndex];
        savedUnitMapY = ScenarioUnitMapY[unitIndex];
        previousRouteX = savedUnitMapX;
        previousRouteY = savedUnitMapY;
        for (routeIndex = 0x0000;
            routeIndex <= route->waypointIndex;
            ++routeIndex)
        {
            ScenarioUnitMapX[unitIndex] = (uint8_t) previousRouteX;
            ScenarioUnitMapY[unitIndex] = (uint8_t) previousRouteY;
            routeX = route->waypointMapX[routeIndex];
            routeY = route->waypointMapY[routeIndex];
            if (ScenarioUnitMapX[unitIndex] != routeX ||
                ScenarioUnitMapY[unitIndex] != routeY)
            {
                DrawWeaponAttackTrace(
                    unitIndex, routeX, routeY, 0x0001, 0x0001, 0x0000);
            }
            previousRouteX = routeX;
            previousRouteY = routeY;
        }
        TacticalCursorViewportX = savedCursorMapX;
        TacticalCursorViewportY = savedCursorMapY;
        ScenarioUnitMapX[unitIndex] = (uint8_t) savedUnitMapX;
        ScenarioUnitMapY[unitIndex] = (uint8_t) savedUnitMapY;
    }
    ShowMouseCursorOverlay();
}

// 2208:0DA1: cycle F9/'+' targeting through the twelve opposing tactical
// slots. Only deployed, currently visible or proximate contacts qualify; the
// first qualifying slot in cyclic unit order is focused.
void CycleTacticalEnemyTarget(void)
{
    int16_t selectedTarget;
    uint16_t firstCandidate;
    uint16_t candidateOffset;
    uint16_t candidateUnit;
    uint16_t contactKnown;
    uint16_t followsCursor;
    uint16_t cursorMapX;
    uint16_t cursorMapY;
    RevengeUnitRecord *unitRecord;

    selectedTarget = -0x01;
    firstCandidate = TacticalSelectedUnitIndex + 0x0001;
    if ((int16_t) firstCandidate < 0x0C)
        firstCandidate = 0x0C;
    if ((int16_t) firstCandidate > 0x17)
        firstCandidate = 0x18;

    // The executable uses the cursor-Y word in both comparisons here.
    cursorMapX = TacticalViewportMapX +
        TacticalCursorViewportY;
    cursorMapY = TacticalViewportMapY +
        TacticalCursorViewportY;

    for (candidateOffset = 0x0000;
        candidateOffset < 0x000C;
        ++candidateOffset)
    {
        candidateUnit = firstCandidate + candidateOffset;
        while ((int16_t) candidateUnit >= TACTICAL_UNIT_COUNT)
            candidateUnit -= 0x0C;

        unitRecord = &PersistentUnitRoster[candidateUnit];
        if (unitRecord->deploymentState == FALSE)
            continue;

        (void) UpdateUnitEnemyProximityState(candidateUnit);
        (void) UpdateUnitEnemyVisibility(candidateUnit);
        contactKnown = UnitEnemyInProximityRange[candidateUnit] |
            UnitEnemyVisible[candidateUnit];
        if (contactKnown == FALSE)
            continue;

        if (selectedTarget == -0x01)
            selectedTarget = (int16_t) candidateUnit;

        followsCursor = FALSE;
        if (ScenarioUnitMapY[candidateUnit] > cursorMapY)
            followsCursor = TRUE;
        else if (ScenarioUnitMapY[candidateUnit] == cursorMapY &&
            ScenarioUnitMapX[candidateUnit] > cursorMapX)
        {
            followsCursor = TRUE;
        }

        if (followsCursor != FALSE && selectedTarget != -0x01)
        {
            if (ScenarioUnitMapY[candidateUnit] <
                ScenarioUnitMapY[selectedTarget])
            {
                selectedTarget = (int16_t) candidateUnit;
            }
            else if (ScenarioUnitMapY[candidateUnit] ==
                    ScenarioUnitMapY[selectedTarget] &&
                ScenarioUnitMapX[candidateUnit] <
                    ScenarioUnitMapX[selectedTarget])
            {
                selectedTarget = (int16_t) candidateUnit;
            }
        }

        if (selectedTarget != -0x01)
        {
            FocusTacticalViewOnUnit((uint16_t) selectedTarget);
            break;
        }
    }
}

// 2208:0EBC
uint16_t SelectTacticalMoveDestinationInput(
    uint16_t unitIndex,
    uint16_t mode)
{
    uint16_t selectionAccepted;
    uint16_t savedSelectedUnit;
    uint16_t savedCursorColumn;
    uint16_t savedViewportX;
    uint16_t savedViewportY;
    uint16_t savedCursorX;
    uint16_t savedCursorY;
    uint16_t cursorMapX;
    uint16_t cursorMapY;
    uint16_t routeIndex;
    uint16_t remainingPoints;
    uint8_t savedUnitMapX;
    uint8_t savedUnitMapY;
    SceneUnitRoute *route;
    int16_t inputAction;
    int16_t maximumViewportX;
    int16_t maximumViewportY;

    HideMouseCursorOverlay();
    savedViewportX = TacticalViewportMapX;
    savedViewportY = TacticalViewportMapY;
    savedCursorX = TacticalCursorViewportX;
    savedCursorY = TacticalCursorViewportY;
    cursorMapX = savedViewportX + savedCursorX;
    cursorMapY = savedViewportY + savedCursorY;

    TacticalViewportMapX += savedCursorX - 0x0A;
    if ((int16_t) savedCursorX < 0x0A &&
        (int16_t) TacticalViewportMapX >= 0x00)
    {
        TacticalCursorViewportX = 0x0A;
    }
    if ((int16_t) savedCursorX > 0x0A &&
        (int16_t) (MapWidth - TACTICAL_VIEWPORT_WIDTH_CELLS) >=
            (int16_t) TacticalViewportMapX)
    {
        TacticalCursorViewportX = 0x0A;
    }
    maximumViewportX = (int16_t) MapWidth - TACTICAL_VIEWPORT_WIDTH_CELLS;
    if ((int16_t) TacticalViewportMapX < 0x00)
        TacticalViewportMapX = 0x00;
    if (maximumViewportX <= (int16_t) TacticalViewportMapX)
        TacticalViewportMapX = (uint16_t) maximumViewportX;

    TacticalViewportMapY += savedCursorY - 0x06;
    if ((int16_t) savedCursorY < 0x06 &&
        (int16_t) TacticalViewportMapY >= 0x00)
    {
        TacticalCursorViewportY = 0x06;
    }
    if ((int16_t) savedCursorY > 0x06 &&
        (int16_t) (MapHeight - TACTICAL_VIEWPORT_HEIGHT_CELLS) >=
            (int16_t) TacticalViewportMapY)
    {
        TacticalCursorViewportY = 0x06;
    }
    maximumViewportY = (int16_t) MapHeight - TACTICAL_VIEWPORT_HEIGHT_CELLS;
    if ((int16_t) TacticalViewportMapY < 0x00)
        TacticalViewportMapY = 0x00;
    if (maximumViewportY <= (int16_t) TacticalViewportMapY)
        TacticalViewportMapY = (uint16_t) maximumViewportY;

    TacticalCursorViewportX =
        cursorMapX - TacticalViewportMapX;
    TacticalCursorViewportY =
        cursorMapY - TacticalViewportMapY;
    if ((int16_t) TacticalCursorViewportX < 0x00)
        TacticalCursorViewportX = 0x00;
    if ((int16_t) TacticalCursorViewportX >= TACTICAL_VIEWPORT_WIDTH_CELLS)
        TacticalCursorViewportX = 0x12;
    if ((int16_t) TacticalCursorViewportY < 0x00)
        TacticalCursorViewportY = 0x00;
    if ((int16_t) TacticalCursorViewportY >= TACTICAL_VIEWPORT_HEIGHT_CELLS)
        TacticalCursorViewportY = 0x0B;

    selectionAccepted = TRUE;
    savedSelectedUnit = TacticalSelectedUnitIndex;
    TacticalSelectedUnitIndex = TACTICAL_UNIT_NONE;
    remainingPoints = unitIndex == 0x18 ? 0x01 : 0x04;
    route = &SceneUnitRoutes[unitIndex];
    SavedTacticalRoute = *route;
    savedUnitMapX = UnitRouteTargetMapX[unitIndex];
    savedUnitMapY = UnitRouteTargetMapY[unitIndex];
    route->waypointIndex = 0x00;
    savedCursorColumn = ActiveTextCursorColumn;
    inputAction = (int16_t) mode;

    while (remainingPoints-- != 0x00)
    {
        while (inputAction != KEY_SPACE &&
            inputAction != KEY_CARRIAGE_RETURN &&
            inputAction != KEY_ESCAPE)
        {
            switch (inputAction)
            {
            case (int16_t) (KEY_PAGE_DOWN - 0x0100):
            case (int16_t) (KEY_DOWN - 0x0100):
            case (int16_t) (KEY_END - 0x0100):
            case (int16_t) (KEY_RIGHT - 0x0100):
            case (int16_t) (KEY_LEFT - 0x0100):
            case (int16_t) (KEY_PAGE_UP - 0x0100):
            case (int16_t) (KEY_UP - 0x0100):
            case (int16_t) (KEY_HOME - 0x0100):
                MoveTacticalCursorByInput(inputAction);
                break;
            }

            /* The original word-sized additions wrap before the signed and
			 * unsigned bounds checks.  Retain those 16-bit intermediates:
			 * ordinary C integer promotion would interpret cursor -1 as 65535
			 * and incorrectly snap it to the opposite map edge. */
            cursorMapX = (uint16_t) (TacticalCursorViewportX +
                TacticalViewportMapX);
            cursorMapY = (uint16_t) (TacticalCursorViewportY +
                TacticalViewportMapY);
            if ((int16_t) cursorMapX < 0x00)
            {
                TacticalViewportMapX = 0x00;
                TacticalCursorViewportX = 0x00;
            }
            if ((int16_t) cursorMapY < 0x00)
            {
                TacticalViewportMapY = 0x00;
                TacticalCursorViewportY = 0x00;
            }
            cursorMapX = (uint16_t) (TacticalCursorViewportX +
                TacticalViewportMapX);
            cursorMapY = (uint16_t) (TacticalCursorViewportY +
                TacticalViewportMapY);
            if (cursorMapX >= MapWidth)
            {
                TacticalCursorViewportX =
                    MapWidth - 0x01 -
                    TacticalViewportMapX;
            }
            if (cursorMapY >= MapHeight)
            {
                TacticalCursorViewportY =
                    MapHeight - 0x01 -
                    TacticalViewportMapY;
            }

            route->waypointMapX[route->waypointIndex] = (uint8_t) (TacticalCursorViewportX +
                TacticalViewportMapX);
            route->waypointMapY[route->waypointIndex] = (uint8_t) (TacticalCursorViewportY +
                TacticalViewportMapY);
            UnitRouteTargetMapX[unitIndex] =
                route->waypointMapX[route->waypointIndex];
            UnitRouteTargetMapY[unitIndex] =
                route->waypointMapY[route->waypointIndex];
            RefreshTacticalCoordinateSelection(
                unitIndex, 0x01, savedCursorColumn);

            inputAction = 0x00;
            if (ServiceMusicAndCheckKeyboardInput() != FALSE)
            {
                inputAction = ReadTranslatedKeypress();
                while (ServiceMusicAndCheckKeyboardInput() != FALSE)
                    (void) ReadTranslatedKeypress();
            }
            inputAction = NormalizeNavigationKeyCode(
                TranslateTacticalMouseInput(inputAction));
            if ((inputAction == KEY_SPACE ||
                    inputAction == KEY_CARRIAGE_RETURN) &&
                route->waypointIndex == 0x00 &&
                UnitRouteTargetMapX[unitIndex] ==
                    ScenarioUnitMapX[unitIndex] &&
                UnitRouteTargetMapY[unitIndex] ==
                    ScenarioUnitMapY[unitIndex])
            {
                inputAction = 'A';
            }
        }

        if (inputAction != KEY_ESCAPE &&
            route->waypointIndex > 0x00 &&
            route->waypointIndex < 0x03)
        {
            routeIndex = route->waypointIndex;
            if (route->waypointMapX[routeIndex - 0x01] ==
                    route->waypointMapX[routeIndex] &&
                route->waypointMapY[routeIndex - 0x01] ==
                    route->waypointMapY[routeIndex])
            {
                while (++routeIndex < 0x04)
                {
                    route->waypointMapX[routeIndex] =
                        route->waypointMapX[routeIndex - 0x01];
                    route->waypointMapY[routeIndex] =
                        route->waypointMapY[routeIndex - 0x01];
                }
                remainingPoints = 0x00;
            }
        }
        if (inputAction != KEY_ESCAPE)
        {
            ++route->waypointIndex;
            inputAction = 0x00;
        }
    }

    if (inputAction == KEY_ESCAPE)
    {
        *route = SavedTacticalRoute;
        UnitRouteTargetMapX[unitIndex] = savedUnitMapX;
        UnitRouteTargetMapY[unitIndex] = savedUnitMapY;
        selectionAccepted = FALSE;
    }
    else
    {
        route->waypointIndex = 0x00;
        route->activeMapX = ScenarioUnitMapX[unitIndex];
        route->activeMapY = ScenarioUnitMapY[unitIndex];
        UnitRouteTargetMapX[unitIndex] = route->waypointMapX[0x00];
        UnitRouteTargetMapY[unitIndex] = route->waypointMapY[0x00];
        if (unitIndex == 0x18)
        {
            route->waypointMapX[0x03] = UnitRouteTargetMapX[unitIndex];
            route->waypointMapY[0x03] = UnitRouteTargetMapY[unitIndex];
            route->waypointIndex = 0x03;
        }
    }

    TacticalSelectedUnitIndex = savedSelectedUnit;
    TacticalViewportMapX = savedViewportX;
    TacticalViewportMapY = savedViewportY;
    TacticalCursorViewportX = savedCursorX;
    TacticalCursorViewportY = savedCursorY;
    ShowMouseCursorOverlay();
    return selectionAccepted;
}

// 2208:1F0C
void DisplayDropShipIntelligence(void)
{
    uint16_t unitIndex;
    uint16_t left;
    uint16_t right;
    uint16_t firstLineY;
    uint16_t secondLineY;

    HideMouseCursorOverlay();
    SelectAndClearTextWindow(0x08);
    DrawActiveTextWindowBorder(0x0E);

    left = ActiveTextWindowLeftTile << 0x03;
    right = ((ActiveTextWindowLeftTile +
                 ActiveTextWindowWidthTiles)
                << 0x03) -
        0x01;
    firstLineY = (ActiveTextWindowTopTile << 0x03) + 0x07;
    secondLineY = ((ActiveTextWindowTopTile + 0x07) << 0x03) +
        0x04;
    DrawClippedAxisAlignedLine(left, firstLineY, right, firstLineY, 0x0D);
    DrawClippedAxisAlignedLine(left, secondLineY, right, secondLineY, 0x0D);
    DrawCenteredTextInActiveWindow(
        IntelligenceHeading,
        0x00,
        0x0F,
        0x00);

    for (unitIndex = ENEMY_FORCE_FIRST_UNIT; unitIndex < TACTICAL_UNIT_COUNT; ++unitIndex)
    {
        (void) UpdateUnitEnemyVisibility(unitIndex);
        (void) UpdateUnitEnemyProximityState(unitIndex);
    }

    ActiveTextCursorColumn = 0x00;
    ActiveTextCursorRow = 0x02;
    RenderFormattedText(EnemyAndFriendlyLancesText);

    ActiveTextCursorColumn = 0x00;
    ActiveTextCursorRow = 0x03;
    for (unitIndex = ENEMY_FORCE_FIRST_UNIT; unitIndex < TACTICAL_UNIT_COUNT; unitIndex += LANCE_UNIT_COUNT)
        RenderLanceIntelligenceSummary(unitIndex);

    ActiveTextCursorColumn = 0x00;
    ActiveTextCursorRow = 0x09;
    for (unitIndex = 0x00; unitIndex < TACTICAL_FORCE_UNIT_COUNT; unitIndex += LANCE_UNIT_COUNT)
        RenderLanceIntelligenceSummary(unitIndex);

    ShowMouseCursorOverlay();
    (void) WaitForDismissInput();
}

// 2208:203B: render one four-unit lance row in the DropShip intelligence
// window, including its readiness label, first known member position,
// formation marker, facing, and movement vector.
void RenderLanceIntelligenceSummary(uint16_t firstUnitIndex)
{
    uint16_t fallbackLanceState;
    uint16_t hasContact;
    uint16_t hasEngagement;
    uint16_t lanceLabelIndex;
    uint16_t totalRating;
    uint16_t hasDeployedUnit;
    uint16_t memberOffset;
    uint16_t firstDeployedMemberOffset;
    uint16_t selectedUnitIndex;
    RevengeUnitRecord *unitRecord;

    fallbackLanceState = FALSE;
    hasEngagement = FALSE;
    hasContact = FALSE;
    for (memberOffset = 0x0000; memberOffset < 0x0004; ++memberOffset)
    {
        selectedUnitIndex = firstUnitIndex + memberOffset;
        if (UnitEnemyContactEstablished[selectedUnitIndex] !=
            FALSE)
        {
            hasContact = TRUE;
        }
        if (UnitEnemyEngagementActive[selectedUnitIndex] !=
            FALSE)
        {
            hasEngagement = TRUE;
        }
        if (firstUnitIndex < ENEMY_FORCE_FIRST_UNIT &&
            ScenarioUnitNeedsUpdate[selectedUnitIndex] != FALSE)
        {
            hasContact = TRUE;
            hasEngagement = TRUE;
        }
    }

    if (hasContact == FALSE && hasEngagement == FALSE)
    {
        if (firstUnitIndex == 0x0004)
            fallbackLanceState = TacticalScenarioLanceEnabled[0x01];
        else if (firstUnitIndex == 0x0008)
            fallbackLanceState = TacticalScenarioLanceEnabled[0x02];
    }
    if (fallbackLanceState != FALSE)
    {
        hasContact = TRUE;
        hasEngagement = TRUE;
    }
    if (hasContact == FALSE)
        return;

    if (hasEngagement != FALSE)
    {
        lanceLabelIndex =
            LanceFormationProfile[firstUnitIndex / 0x0004];
    }
    else
    {
        totalRating = 0x0000;
        for (memberOffset = 0x0000; memberOffset < 0x0004; ++memberOffset)
        {
            unitRecord = &PersistentUnitRoster[firstUnitIndex + memberOffset];
            totalRating += unitRecord->tonnage;
        }
        lanceLabelIndex = (uint16_t) ((((int16_t) totalRating >> 0x02) - 0x14) / 0x14 + 0x04);
    }
    RenderFormattedText(LanceStatusTextTable[lanceLabelIndex]);

    hasDeployedUnit = FALSE;
    memberOffset = 0x0000;
    firstDeployedMemberOffset = 0x0000;
    while (memberOffset < 0x0004)
    {
        unitRecord = &PersistentUnitRoster[firstUnitIndex + memberOffset];
        if (unitRecord->deploymentState != FALSE)
        {
            hasDeployedUnit = TRUE;
            firstDeployedMemberOffset = memberOffset;
            break;
        }
        ++memberOffset;
    }

    if (hasDeployedUnit == FALSE)
    {
        ActiveTextCursorColumn = 0x000C;
        RenderFormattedText(DeadText);
        return;
    }

    if (firstUnitIndex == ENEMY_FORCE_FIRST_UNIT)
    {
        hasDeployedUnit = FALSE;
        memberOffset = firstDeployedMemberOffset;
        while (memberOffset < 0x0004)
        {
            if (UpdateUnitEnemyVisibility(
                    firstUnitIndex + memberOffset) != FALSE)
            {
                hasDeployedUnit = TRUE;
            }
            ++memberOffset;
        }
    }

    if (hasDeployedUnit != FALSE)
    {
        selectedUnitIndex = firstUnitIndex + firstDeployedMemberOffset;
        DrawLanceFormationMarker(
            UnitFormationMode[selectedUnitIndex], 0x0009);
        ActiveTextCursorColumn = 0x000C;
        FormatTacticalMapCoordinates(
            ScenarioUnitMapX[selectedUnitIndex],
            ScenarioUnitMapY[selectedUnitIndex]);
        RenderFormattedText(SharedNumericTextBuffer);
        DrawUnitFacingAndMovementVectors(selectedUnitIndex);
    }
    else
    {
        ActiveTextCursorColumn = 0x000C;
        RenderFormattedText(UnknownUnitText);
    }
}

// 2208:2323: draw the selected unit's facing vector, secondary facing arm,
// and optional movement-heat vector beside its intelligence-row position.
void DrawUnitFacingAndMovementVectors(uint16_t unitIndex)
{
    uint16_t originX;
    uint16_t originY;
    uint16_t facing;
    uint16_t movementBand;
    uint16_t tableOffset;

    originX = (ActiveTextWindowLeftTile + 0x000F) << 0x03;
    originY = (ActiveTextWindowTopTile +
                  ActiveTextCursorRow)
        << 0x03;
    facing = ScenarioUnitFacing[unitIndex];

    DrawClippedWeaponTraceLine(
        (int16_t) (originX + FacingVectorStartX[facing]),
        (int16_t) (originY + FacingVectorStartY[facing]),
        (int16_t) (originX + FacingVectorStartX[facing + 0x08]),
        (int16_t) (originY + FacingVectorStartY[facing + 0x08]),
        0x0005,
        0x0000);
    DrawClippedWeaponTraceLine(
        (int16_t) (originX + FacingVectorEndX[facing]),
        (int16_t) (originY + FacingVectorEndY[facing]),
        (int16_t) (originX + FacingVectorEndX[facing + 0x08]),
        (int16_t) (originY + FacingVectorEndY[facing + 0x08]),
        0x0005,
        0x0000);

    if (UnitMovementHeatExponent[unitIndex] != 0x00)
    {
        movementBand =
            UnitMovementHeatExponent[unitIndex] - 0x01;
        tableOffset = movementBand << 0x04;
        DrawClippedWeaponTraceLine(
            (int16_t) (originX + MovementVectorX[tableOffset + facing]),
            (int16_t) (originY + MovementVectorY[tableOffset + facing]),
            (int16_t) (originX +
                MovementVectorX[tableOffset + facing + 0x08]),
            (int16_t) (originY +
                MovementVectorY[tableOffset + facing + 0x08]),
            0x0005,
            0x0000);
    }
}

// 2208:2224: draw the six-pixel marker for one of the ten lance formations.
// Verified against ASM 2208:2224-2322.
void DrawLanceFormationMarker(
    int16_t formationCode,
    uint16_t tileColumnOffset)
{
    uint16_t formationIndex;
    int16_t originX;
    int16_t originY;
    uint16_t memberIndex;
    uint16_t tableOffset;
    int16_t memberX;
    int16_t memberY;

    formationIndex = (uint16_t) formationCode & 0x7F;
    originX = (int16_t) (((ActiveTextWindowLeftTile +
                              tileColumnOffset)
                             << 0x03) +
        FormationMarkerOriginX[formationIndex]);
    originY = (int16_t) (((ActiveTextWindowTopTile +
                              ActiveTextCursorRow)
                             << 0x03) +
        FormationMarkerOriginY[formationIndex]);

    DrawClippedPixel(originX, originY, 0x0F);
    DrawClippedPixel((int16_t) (originX + 0x01), originY, 0x0F);
    for (memberIndex = 0x00; memberIndex < 0x03; ++memberIndex)
    {
        tableOffset = formationIndex * 0x06 + memberIndex;
        memberX = (int16_t) (originX +
            (FormationOffsetTable[REVENGE_INDEX(
                 FormationOffsetTable, tableOffset)]
                << 0x02));
        memberY = (int16_t) (originY +
            (FormationOffsetTable[REVENGE_INDEX(
                 FormationOffsetTable, tableOffset + 0x03)]
                << 0x01));
        DrawClippedPixel(memberX, memberY, 0x0F);
        DrawClippedPixel((int16_t) (memberX + 0x01), memberY, 0x0F);
    }
}

// 2208:25F0: format a tactical map position as sector letter plus two decimal
// sub-cell digits in the shared four-byte text buffer at DS:B67E. Verified
// against ASM 2208:25F0-26B0.
void FormatTacticalMapCoordinates(uint16_t mapX, uint16_t mapY)
{
    int16_t sectorWidth;
    int16_t sectorHeight;
    int16_t sectorX;
    int16_t sectorY;
    int16_t subCellX;
    int16_t subCellY;
    uint8_t sectorLetter;

    sectorWidth = (int16_t) MapWidth / 0x05;
    sectorHeight = (int16_t) MapHeight / 0x05;
    sectorX = (int16_t) mapX / sectorWidth;
    if (sectorX > 0x04)
        sectorX = 0x04;
    sectorY = (int16_t) mapY / sectorHeight;
    if (sectorY > 0x04)
        sectorY = 0x04;

    subCellX = (int16_t) ((((uint16_t) mapX -
                               (uint16_t) (sectorX * sectorWidth)) *
                              0x0A) /
        sectorWidth);
    subCellY = (int16_t) ((((uint16_t) mapY -
                               (uint16_t) (sectorY * sectorHeight)) *
                              0x0A) /
        sectorHeight);
    if (subCellX > 0x09)
        subCellX = 0x09;
    if (subCellY > 0x09)
        subCellY = 0x09;

    sectorLetter = (uint8_t) ('A' + sectorX + sectorY * 0x05);
    if (sectorLetter > 'Y')
        sectorLetter = 'Y';
    SharedNumericTextBuffer[0x00] = sectorLetter;
    SharedNumericTextBuffer[0x01] = (uint8_t) ('0' + subCellX);
    SharedNumericTextBuffer[0x02] = (uint8_t) ('0' + subCellY);
    SharedNumericTextBuffer[0x03] = 0x00;
}

// 2208:146C: select the center of the strafing area and preserve it in the
// tactical support-command state. Verified against 2208:146C-1489.
void SelectStrafingArea(void)
{
    SelectTacticalMapCoordinate(0x0018);
    ArtilleryShellingActive = 0x00;
    RollingBarrageActive = 0x00;
    RollingBarrageEndMapX = UnitTargetMapX[TACTICAL_TARGET_MAP_CELL];
    RollingBarrageEndMapY = UnitTargetMapY[TACTICAL_TARGET_MAP_CELL];
}

// 2208:25C4: replace the command-window text while preserving the tactical
// display beneath the mouse cursor. Verified against 2208:25C4-25EF.
void ShowTacticalCommandPrompt(const char *prompt)
{
    HideMouseCursorOverlay();
    SelectAndClearTextWindow(0x0006);
    ActiveTextForegroundColor = 0x000F;
    RenderFormattedText(prompt);
    ShowMouseCursorOverlay();
}

// 2208:2A8A: draw the selection outline over each deployed follower in the
// selected leader's four-unit lance, restoring the caller's cursor afterward.
// Verified against ASM 2208:2A8A-2B0D.
void HighlightLanceFollowers(uint16_t leaderUnitIndex)
{
    int16_t savedCursorX;
    int16_t savedCursorY;
    uint16_t unitIndex;

    savedCursorX = TacticalCursorViewportX;
    savedCursorY = TacticalCursorViewportY;
    unitIndex = leaderUnitIndex + 0x01;
    while ((unitIndex & 0x03) != 0x00)
    {
        if (PersistentUnitRoster[unitIndex].deploymentState != FALSE)
        {
            TacticalCursorViewportX =
                (int16_t) ((uint16_t) ScenarioUnitMapX[unitIndex] -
                    TacticalViewportMapX);
            TacticalCursorViewportY =
                (int16_t) ((uint16_t) ScenarioUnitMapY[unitIndex] -
                    TacticalViewportMapY);
            if ((int16_t) TacticalCursorViewportX >= 0x00 &&
                (int16_t) TacticalCursorViewportX < TACTICAL_VIEWPORT_WIDTH_CELLS &&
                (int16_t) TacticalCursorViewportY >= 0x00 &&
                (int16_t) TacticalCursorViewportY < TACTICAL_VIEWPORT_HEIGHT_CELLS)
            {
                DrawTacticalCursorOutline(0x05);
            }
        }
        ++unitIndex;
    }

    TacticalCursorViewportX = savedCursorX;
    TacticalCursorViewportY = savedCursorY;
}
