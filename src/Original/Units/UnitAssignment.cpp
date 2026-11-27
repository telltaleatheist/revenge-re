// 2507: unit assignment
// UnitAssignment.cpp
// Reconstructed from the original procedures in segment 2507.

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "LanceEditorInternal.h"
#include "Campaign/Campaign.h"
#include "Data/RuntimeData.h"
#include "Data/StartupData.h"
#include "Data/TacticalData.h"
#include "Graphics/Graphics.h"
#include "Input/Input.h"
#include "Runtime/Runtime.h"
#include "Tactical/Tactical.h"
#include "Text/Text.h"
#include "Units/Units.h"
#include "Utility/Utility.h"
#include "revenge/tactical_panel_data.h"
#include "revenge/text.h"
#include "revenge/debug.h"

#include <string.h>

// 2507:30FC: select a lance section and enter its unit-assignment grid.
// VERIFIED against ASM 2507:30FC-3575.
void RunUnitAssignmentEditor(uint16_t configurationIndex)
{
    uint16_t selectedSection;
    uint16_t previousSection;
    uint16_t sectionBeforeInput;
    uint16_t sectionIndex;
    uint16_t useSeparateScenarioGroups;
    uint16_t activateSection;
    uint16_t finished;
    uint16_t mouseButtonState;
    uint16_t mouseButtonEvent;
    int16_t inputCode;
    int16_t mouseX;
    int16_t mouseY;
    int16_t textLeftColumn;
    int16_t sectionRow;
    int16_t fontWidth;
    int16_t fontHeight;

    selectedSection = 0x0000;
    if (configurationIndex == 0x0000 || configurationIndex == 0x0001)
    {
        RunUnitAssignmentGrid(0x0000, 0x0000, TRUE);
        return;
    }

    useSeparateScenarioGroups = configurationIndex < 0x0003 ? 0x0003 : 0x0000;
    configurationIndex -= 0x0002;
    while (UnitEditorSectionEnabled[configurationIndex * LANCE_EDITOR_SECTION_COUNT + selectedSection] == 0x00 &&
        selectedSection < 0x0003)
    {
        ++selectedSection;
    }
    previousSection = selectedSection;
    textLeftColumn = 0x002B;
    fontWidth = ActiveFontGlyphAdvanceX;
    fontHeight = ActiveFontLineHeight;

    sectionRow = (int16_t) LanceEditorLayout.UnitEditorSectionRows[selectedSection];
    HideMouseCursorOverlay();
    XorVideoRectangle(
        (int16_t) (textLeftColumn * fontWidth),
        (int16_t) (sectionRow * fontHeight),
        (int16_t) ((textLeftColumn + 0x0007) * fontWidth - 0x0001),
        (int16_t) ((sectionRow + 0x0001) * fontHeight - 0x0001),
        0x01);
    ShowMouseCursorOverlay();
    FlushPendingInput();
    activateSection = FALSE;
    finished = FALSE;

    while (finished == FALSE)
    {
        sectionBeforeInput = selectedSection;
        inputCode = 0x0000;
        if (ServiceMusicAndCheckKeyboardInput() != 0x0000)
        {
            inputCode = NormalizeNavigationKeyCode(
                ReadTranslatedKeypress());
            if (inputCode == KEY_ESCAPE)
            {
                if (UnitEditorSectionEnabled[configurationIndex * LANCE_EDITOR_SECTION_COUNT + 0x0003] != 0x00)
                    finished = TRUE;
            }
            else if (inputCode == TranslatedExtendedKey(KEY_UP))
            {
                do
                {
                    if ((int16_t) --selectedSection < 0x0000)
                        selectedSection = 0x0003;
                } while (UnitEditorSectionEnabled[configurationIndex * LANCE_EDITOR_SECTION_COUNT + selectedSection] == 0x00 &&
                    selectedSection != sectionBeforeInput);
            }
            else if (inputCode == TranslatedExtendedKey(KEY_DOWN))
            {
                do
                {
                    if (++selectedSection > 0x0003)
                        selectedSection = 0x0000;
                } while (UnitEditorSectionEnabled[configurationIndex * LANCE_EDITOR_SECTION_COUNT + selectedSection] == 0x00 &&
                    selectedSection != sectionBeforeInput);
            }
            else if (inputCode == KEY_CARRIAGE_RETURN || inputCode == KEY_SPACE)
            {
                if (selectedSection == 0x0003)
                    finished = TRUE;
                else
                    activateSection = TRUE;
            }
        }
        else if (MouseInputDisabled == 0x0000)
        {
            mouseX = (int16_t) MouseCursorRequestedX;
            mouseY = (int16_t) MouseY;
            mouseButtonState = MouseButtonState;
            mouseButtonEvent = PendingMouseButtonEvent;
            PendingMouseButtonEvent = 0x0000;

            if (mouseButtonEvent != 0x0000 &&
                (mouseButtonState & MOUSE_BUTTON_RIGHT) != 0x0000 &&
                UnitEditorSectionEnabled[configurationIndex * LANCE_EDITOR_SECTION_COUNT + 0x0003] != 0x00)
            {
                finished = TRUE;
            }
            else
            {
                for (sectionIndex = 0x0000; sectionIndex < 0x0004;
                    ++sectionIndex)
                {
                    if (UnitEditorSectionEnabled[configurationIndex * LANCE_EDITOR_SECTION_COUNT + sectionIndex] == 0x00)
                        continue;
                    sectionRow = (int16_t) LanceEditorLayout.UnitEditorSectionRows[sectionIndex];
                    if (mouseX >= textLeftColumn * fontWidth &&
                        mouseX < (textLeftColumn + 0x0007) * fontWidth &&
                        mouseY >= sectionRow * fontHeight &&
                        mouseY < (sectionRow + 0x0001) * fontHeight)
                    {
                        selectedSection = sectionIndex;
                        if (mouseButtonEvent != 0x0000 &&
                            (mouseButtonState & MOUSE_BUTTON_LEFT) != 0x0000)
                        {
                            if (selectedSection == 0x0003)
                                finished = TRUE;
                            else
                                activateSection = TRUE;
                        }
                    }
                }
            }
        }

        if (selectedSection != sectionBeforeInput)
        {
            HideMouseCursorOverlay();
            sectionRow = (int16_t) LanceEditorLayout.UnitEditorSectionRows[sectionBeforeInput];
            XorVideoRectangle(
                (int16_t) (textLeftColumn * fontWidth),
                (int16_t) (sectionRow * fontHeight),
                (int16_t) ((textLeftColumn + 0x0007) * fontWidth - 0x0001),
                (int16_t) ((sectionRow + 0x0001) * fontHeight - 0x0001),
                0x01);
            WaitVerticalRetraces(0x0002);
            sectionRow = (int16_t) LanceEditorLayout.UnitEditorSectionRows[selectedSection];
            XorVideoRectangle(
                (int16_t) (textLeftColumn * fontWidth),
                (int16_t) (sectionRow * fontHeight),
                (int16_t) ((textLeftColumn + 0x0007) * fontWidth - 0x0001),
                (int16_t) ((sectionRow + 0x0001) * fontHeight - 0x0001),
                0x01);
            ShowMouseCursorOverlay();
        }

        if (selectedSection != previousSection)
        {
            if (useSeparateScenarioGroups != 0x0000)
            {
                if (previousSection != 0x0003)
                    RefreshLanceSelectionSlots(
                        previousSection, previousSection, FALSE);
                if (selectedSection != 0x0003)
                    RefreshLanceSelectionSlots(
                        selectedSection, selectedSection, TRUE);
            }
            else
            {
                if (previousSection != 0x0003)
                    RefreshLanceSelectionSlots(
                        0x0000, previousSection, FALSE);
                if (selectedSection != 0x0003)
                    RefreshLanceSelectionSlots(
                        0x0000, selectedSection, TRUE);
            }
            previousSection = selectedSection;
            FlushPendingInput();
        }

        if (activateSection != FALSE)
        {
            ClearUnitEditorSectionPanel();
            if (useSeparateScenarioGroups != 0x0000)
                RunUnitAssignmentGrid(
                    selectedSection, selectedSection, TRUE);
            else
                RunUnitAssignmentGrid(
                    0x0000, selectedSection, FALSE);
            (void) ValidateLanceConfiguration(configurationIndex);
            (void) DrawUnitEditorSectionMenu(configurationIndex);
            FlushPendingInput();
            HideMouseCursorOverlay();
            sectionRow = (int16_t) LanceEditorLayout.UnitEditorSectionRows[selectedSection];
            XorVideoRectangle(
                (int16_t) (textLeftColumn * fontWidth),
                (int16_t) (sectionRow * fontHeight),
                (int16_t) ((textLeftColumn + 0x0007) * fontWidth - 0x0001),
                (int16_t) ((sectionRow + 0x0001) * fontHeight - 0x0001),
                0x01);
            ShowMouseCursorOverlay();
            activateSection = FALSE;
        }
    }
}

// 2507:3576: constant true callback retained as its own executable boundary.
// VERIFIED against ASM 2507:3576-357F.
uint16_t ReturnTrue(void)
{
    return TRUE;
}

// 2507:3580: edit the unit-card to lance-slot assignment grid.
// VERIFIED against ASM 2507:3580-4296.
void RunUnitAssignmentGrid(
    uint16_t scenarioGroupIndex,
    uint16_t lanceGroupIndex,
    uint16_t useSeparateScenarioGroups)
{
    int16_t selectedLanceSlot;
    int16_t previousLanceSlot;
    int16_t selectedUnitCard;
    int16_t previousUnitCard;
    uint16_t lanceGroupBase;
    uint16_t unitCardGroupBase;
    uint16_t selectedAssignmentIndex;
    uint16_t previousAssignmentIndex;
    uint16_t selectedUnitCardIndex;
    uint16_t previousUnitCardIndex;
    uint16_t scanIndex;
    uint16_t actionRequested;
    uint16_t exitRequested;
    uint16_t finished;
    uint16_t mouseButtonState;
    uint16_t mouseButtonEvent;
    uint16_t oldAssignmentIndex;
    uint16_t graphicsBufferIndex;
    uint16_t unitType;
    uint16_t allowExitSelection;
    int16_t inputCode;
    int16_t mouseX;
    int16_t mouseY;
    uint8_t pilotId;
    RevengeUnitLanceAssignment *assignmentRecord;
    UnitSelectionCard *unitCardRecord;
    RevengeUnitLanceAssignment *previousRecord;
    RevengeUnitRecord *persistentUnit;
    RevengeUnitRecord *liveUnit;
    RevengeUnitRecord *scratchUnit;

#define DRAW_ASSIGNMENT(index) \
    DrawLanceAssignmentSlot((index), TRUE, FALSE)
#define DRAW_UNIT_CARD(index, mode) \
    DrawUnitStatusScreen((index), TRUE, (mode), TRUE)

    if (useSeparateScenarioGroups != FALSE &&
        UnitEditorScenarioCardCount != 0x0000)
    {
        allowExitSelection = ReturnTrue();
    }
    else
    {
        allowExitSelection = TRUE;
    }
    selectedLanceSlot = UnitEditorLanceSlotCount[scenarioGroupIndex] != 0x0000 ? 0x0000 : 0x0004;
    previousLanceSlot = selectedLanceSlot;
    lanceGroupBase = lanceGroupIndex * 0x0004;
    selectedAssignmentIndex = lanceGroupBase + selectedLanceSlot;
    previousAssignmentIndex = selectedAssignmentIndex;
    if (selectedLanceSlot == 0x0004)
    {
        UnitEditorExitButtonState |= 0x01;
        DrawLanceAssignmentSlot(0x0000, TRUE, TRUE);
    }
    else
    {
        LanceAssignments[selectedAssignmentIndex].style |= 0x01;
        DRAW_ASSIGNMENT(selectedAssignmentIndex);
    }

    selectedUnitCard = 0x0000;
    previousUnitCard = selectedUnitCard;
    unitCardGroupBase = scenarioGroupIndex * 0x0006;
    selectedUnitCardIndex = unitCardGroupBase + selectedUnitCard;
    previousUnitCardIndex = selectedUnitCardIndex;
    UnitSelectionCards[selectedUnitCardIndex].state |= 0x02;
    DRAW_UNIT_CARD(selectedUnitCardIndex, 0x0001);
    FlushPendingInput();
    finished = FALSE;
    REVENGE_PRESERVATION_TRACE(
        "lance: grid start scenarioGroup=%u lanceGroup=%u cards=%u slots=%u\n",
        scenarioGroupIndex, lanceGroupIndex,
        UnitEditorScenarioCardCount,
        UnitEditorLanceSlotCount[lanceGroupIndex]);

    while (finished == FALSE)
    {
        actionRequested = FALSE;
        exitRequested = FALSE;
        inputCode = 0x0000;
        if (ServiceMusicAndCheckKeyboardInput() != 0x0000)
        {
            inputCode = NormalizeNavigationKeyCode(
                ReadTranslatedKeypress());
            if (inputCode == KEY_ESCAPE)
            {
                exitRequested = TRUE;
                actionRequested = TRUE;
            }
            else if (inputCode == TranslatedExtendedKey(KEY_UP))
                --selectedLanceSlot;
            else if (inputCode == TranslatedExtendedKey(KEY_DOWN))
                ++selectedLanceSlot;
            else if (inputCode == TranslatedExtendedKey(KEY_LEFT))
                --selectedUnitCard;
            else if (inputCode == TranslatedExtendedKey(KEY_RIGHT))
                ++selectedUnitCard;
            else if (inputCode == KEY_CARRIAGE_RETURN || inputCode == KEY_SPACE)
            {
                if (selectedLanceSlot == 0x0004)
                    exitRequested = TRUE;
                actionRequested = TRUE;
            }
        }
        else if (MouseInputDisabled == 0x0000)
        {
            mouseX = (int16_t) MouseCursorRequestedX;
            mouseY = (int16_t) MouseY;
            mouseButtonState = MouseButtonState;
            mouseButtonEvent = PendingMouseButtonEvent;
            PendingMouseButtonEvent = 0x0000;
            if (mouseButtonEvent != 0x0000 &&
                (mouseButtonState & MOUSE_BUTTON_RIGHT) != 0x0000)
            {
                exitRequested = TRUE;
                actionRequested = TRUE;
            }
            else
            {
                for (scanIndex = 0x0000;
                    scanIndex < UnitEditorScenarioCardCount; ++scanIndex)
                {
                    unitCardRecord = &UnitSelectionCards[unitCardGroupBase + scanIndex];
                    if (mouseX >= (int16_t) (unitCardRecord->left + 0x0001) &&
                        mouseX < (int16_t) (unitCardRecord->left + 0x003F) &&
                        mouseY >= (int16_t) (unitCardRecord->top + 0x0001) &&
                        mouseY < (int16_t) (unitCardRecord->top + 0x0038))
                    {
                        selectedUnitCard = scanIndex;
                        if (mouseButtonEvent != 0x0000 &&
                            (mouseButtonState & MOUSE_BUTTON_LEFT) != 0x0000)
                        {
                            if (selectedLanceSlot == 0x0004)
                                exitRequested = TRUE;
                            actionRequested = TRUE;
                        }
                    }
                }
                for (scanIndex = 0x0000;
                    scanIndex < UnitEditorLanceSlotCount[lanceGroupIndex];
                    ++scanIndex)
                {
                    assignmentRecord = &LanceAssignments[lanceGroupBase + scanIndex];
                    if (mouseX >= (int16_t) assignmentRecord->left &&
                        mouseX < (int16_t) (assignmentRecord->left + 0x0054) &&
                        mouseY >= (int16_t) assignmentRecord->top &&
                        mouseY < (int16_t) (assignmentRecord->top + 0x0018))
                    {
                        if (mouseButtonEvent != 0x0000 &&
                            (mouseButtonState & MOUSE_BUTTON_LEFT) != 0x0000 &&
                            selectedLanceSlot == scanIndex)
                            actionRequested = TRUE;
                        selectedLanceSlot = scanIndex;
                    }
                }
                if (allowExitSelection != FALSE &&
                    mouseX >= (int16_t) UnitEditorExitButtonLeft &&
                    mouseX < (int16_t) (UnitEditorExitButtonLeft + 0x0054) &&
                    mouseY >= (int16_t) UnitEditorExitButtonTop &&
                    mouseY < (int16_t) (UnitEditorExitButtonTop + 0x0018))
                {
                    if (mouseButtonEvent != 0x0000 &&
                        (mouseButtonState & MOUSE_BUTTON_LEFT) != 0x0000)
                    {
                        exitRequested = TRUE;
                        actionRequested = TRUE;
                    }
                    selectedLanceSlot = 0x0004;
                }
            }
        }

        if (selectedLanceSlot > 0x0004)
            selectedLanceSlot = 0x0000;
        if (selectedLanceSlot >=
            (int16_t) UnitEditorLanceSlotCount[lanceGroupIndex])
        {
            selectedLanceSlot = allowExitSelection != FALSE ? 0x0004 : 0x0000;
        }
        if (selectedLanceSlot < 0x0000)
        {
            selectedLanceSlot = allowExitSelection != FALSE ? 0x0004 : UnitEditorLanceSlotCount[lanceGroupIndex] - 0x0001;
        }
        if (selectedUnitCard >= (int16_t) UnitEditorScenarioCardCount)
            selectedUnitCard = 0x0000;
        if (selectedUnitCard < 0x0000)
            selectedUnitCard = UnitEditorScenarioCardCount - 0x0001;

        selectedAssignmentIndex = lanceGroupBase + selectedLanceSlot;
        previousAssignmentIndex = lanceGroupBase + previousLanceSlot;
        selectedUnitCardIndex = unitCardGroupBase + selectedUnitCard;
        previousUnitCardIndex = unitCardGroupBase + previousUnitCard;
        if (selectedLanceSlot != previousLanceSlot)
        {
            if (previousLanceSlot == 0x0004)
            {
                UnitEditorExitButtonState &= 0xFE;
                DrawLanceAssignmentSlot(0x0000, TRUE, TRUE);
            }
            else
            {
                LanceAssignments[previousAssignmentIndex].style &= 0xFE;
                DRAW_ASSIGNMENT(previousAssignmentIndex);
            }
            if (selectedLanceSlot == 0x0004)
            {
                UnitEditorExitButtonState |= 0x01;
                DrawLanceAssignmentSlot(0x0000, TRUE, TRUE);
            }
            else
            {
                LanceAssignments[selectedAssignmentIndex].style |= 0x01;
                DRAW_ASSIGNMENT(selectedAssignmentIndex);
            }
            previousLanceSlot = selectedLanceSlot;
        }
        if (selectedUnitCard != previousUnitCard)
        {
            UnitSelectionCards[previousUnitCardIndex].state &= 0xFD;
            DRAW_UNIT_CARD(previousUnitCardIndex, 0x0001);
            UnitSelectionCards[selectedUnitCardIndex].state |= 0x02;
            DRAW_UNIT_CARD(selectedUnitCardIndex, 0x0001);
            previousUnitCard = selectedUnitCard;
        }
        if (actionRequested == FALSE)
            continue;
        REVENGE_PRESERVATION_TRACE(
            "lance: action key=%04X card=%u slot=%u exit=%u\n",
            (uint16_t) inputCode, selectedUnitCard, selectedLanceSlot,
            exitRequested);

        assignmentRecord = &LanceAssignments[selectedAssignmentIndex];
        unitCardRecord = &UnitSelectionCards[selectedUnitCardIndex];
        // The exit button, or a card another lance holds, skips the
        // assignment and goes on to the next free slot.
        uint16_t assignCard = TRUE;
        if (exitRequested != FALSE && selectedLanceSlot == 0x0004)
        {
            selectedLanceSlot = 0x0000;
            selectedAssignmentIndex = lanceGroupBase;
            assignmentRecord =
                &LanceAssignments[selectedAssignmentIndex];
            UnitEditorExitButtonState &= 0xFE;
            DrawLanceAssignmentSlot(0x0000, TRUE, TRUE);
            previousLanceSlot = selectedLanceSlot;
            assignCard = FALSE;
        }
        else if (unitCardRecord->assignmentSlot != REVENGE_ASSIGNMENT_SLOT_NONE &&
            (unitCardRecord->assignmentSlot >> 0x02) != (uint8_t) lanceGroupIndex)
            assignCard = FALSE;

        if (assignCard != FALSE)
        {
            /* 2507:3AB8-4080.  Bit 2 records whether an assignment slot is
			 * occupied.  The original toggles it before dispatching between the
			 * occupied-slot and empty-slot paths. */
            assignmentRecord->style ^= 0x04;
            DRAW_ASSIGNMENT(selectedAssignmentIndex);
            if (assignmentRecord->liveUnitSlot != REVENGE_UNIT_SLOT_NONE)
            {
                if (assignmentRecord->liveUnitSlot ==
                    unitCardRecord->assignmentSlot)
                {
                    assignmentRecord->persistentUnitSlot = REVENGE_UNIT_SLOT_NONE;
                    assignmentRecord->liveUnitSlot = REVENGE_UNIT_SLOT_NONE;
                    unitCardRecord->state ^= 0x01;
                    unitCardRecord->assignmentSlot = REVENGE_ASSIGNMENT_SLOT_NONE;
                    DRAW_UNIT_CARD(selectedUnitCardIndex, 0x0001);
                }
                else if (assignmentRecord->persistentUnitSlot !=
                    REVENGE_UNIT_SLOT_NONE)
                {
                    /* The occupied slot's +07 field is a unit-card index.  It is
					 * not an assignment index; confusing the two is what caused
					 * replacements to propagate through a screen column. */
                    oldAssignmentIndex = assignmentRecord->persistentUnitSlot;
                    UnitSelectionCards[oldAssignmentIndex].state ^= 0x01;
                    UnitSelectionCards[oldAssignmentIndex].assignmentSlot =
                        REVENGE_ASSIGNMENT_SLOT_NONE;
                    DRAW_UNIT_CARD(oldAssignmentIndex, 0x0000);

                    if ((unitCardRecord->state & 0x01) != 0x00)
                    {
                        unitCardRecord->state ^= 0x01;
                        unitCardRecord->assignmentSlot =
                            (uint8_t) selectedAssignmentIndex;
                        DRAW_UNIT_CARD(selectedUnitCardIndex, 0x0001);
                    }
                    else
                    {
                        oldAssignmentIndex = unitCardRecord->assignmentSlot;
                        previousRecord =
                            &LanceAssignments[oldAssignmentIndex];
                        previousRecord->style ^= 0x04;
                        DRAW_ASSIGNMENT(oldAssignmentIndex);
                        previousRecord->persistentUnitSlot =
                            REVENGE_UNIT_SLOT_NONE;
                        previousRecord->liveUnitSlot =
                            REVENGE_UNIT_SLOT_NONE;
                        unitCardRecord->assignmentSlot =
                            (uint8_t) selectedAssignmentIndex;
                    }

                    /* Both branches above converge at 2507:3C12.  Restore the
					 * occupied style after erasing the old label, install the
					 * selected card, and redraw the replacement assignment. */
                    assignmentRecord->style ^= 0x04;
                    assignmentRecord->persistentUnitSlot =
                        (uint8_t) selectedUnitCardIndex;
                    assignmentRecord->liveUnitSlot =
                        (uint8_t) selectedAssignmentIndex;
                    persistentUnit =
                        &PersistentUnitRoster[selectedUnitCardIndex];
                    liveUnit =
                        &LiveScenarioUnits[selectedAssignmentIndex];
                    *liveUnit = *persistentUnit;
                    liveUnit->deploymentState = 0x01;
                    pilotId = assignmentRecord->pilotId;
                    liveUnit->pilotId = pilotId;
                    liveUnit->gunneryTargetNumber =
                        PilotRoster[pilotId].gunneryIndex;
                    liveUnit->pilotExperience =
                        PilotRoster[pilotId].experienceIndex;
                    DRAW_ASSIGNMENT(selectedAssignmentIndex);
                }
                else
                {
                    /* 2507:3CFA-3EFA: this slot contains a non-roster unit.  The
					 * executable exchanges its live record with the selected
					 * persistent card through DS:84C7, then refreshes that card's
					 * sprite pointer. */
                    if ((unitCardRecord->state & 0x01) == 0x00)
                    {
                        oldAssignmentIndex = unitCardRecord->assignmentSlot;
                        previousRecord =
                            &LanceAssignments[oldAssignmentIndex];
                        previousRecord->style ^= 0x04;
                        DRAW_ASSIGNMENT(oldAssignmentIndex);
                        previousRecord->persistentUnitSlot =
                            REVENGE_UNIT_SLOT_NONE;
                        previousRecord->liveUnitSlot =
                            REVENGE_UNIT_SLOT_NONE;
                        DRAW_UNIT_CARD(selectedUnitCardIndex, 0xFFFF);
                    }

                    persistentUnit =
                        &PersistentUnitRoster[selectedUnitCardIndex];
                    liveUnit = &LiveScenarioUnits[selectedAssignmentIndex];
                    scratchUnit = &EstimatedUnitRecord;
                    *scratchUnit = *persistentUnit;
                    *persistentUnit = *liveUnit;
                    *liveUnit = *scratchUnit;
                    liveUnit->deploymentState = 0x01;
                    unitType = persistentUnit->unitTypeId;
                    graphicsBufferIndex = UnitEditorUnitSpriteIndex[unitType];
                    unitCardRecord->spriteBuffer =
                        UnitEditorGraphicsBuffer(graphicsBufferIndex);
                    unitCardRecord->state |= 0x01;
                    unitCardRecord->persistentUnitSlot =
                        (uint8_t) selectedUnitCardIndex;
                    unitCardRecord->assignmentSlot = REVENGE_ASSIGNMENT_SLOT_NONE;
                    DRAW_UNIT_CARD(selectedUnitCardIndex, 0x0001);

                    assignmentRecord->style ^= 0x04;
                    assignmentRecord->persistentUnitSlot =
                        (uint8_t) selectedUnitCardIndex;
                    assignmentRecord->liveUnitSlot =
                        (uint8_t) selectedAssignmentIndex;
                    pilotId = assignmentRecord->pilotId;
                    liveUnit->pilotId = pilotId;
                    liveUnit->gunneryTargetNumber =
                        PilotRoster[pilotId].gunneryIndex;
                    liveUnit->pilotExperience =
                        PilotRoster[pilotId].experienceIndex;
                    DRAW_ASSIGNMENT(selectedAssignmentIndex);
                }
            }
            else
            {
                assignmentRecord->persistentUnitSlot =
                    (uint8_t) selectedUnitCardIndex;
                assignmentRecord->liveUnitSlot =
                    (uint8_t) selectedAssignmentIndex;
                persistentUnit = &PersistentUnitRoster[selectedUnitCardIndex];
                liveUnit = &LiveScenarioUnits[selectedAssignmentIndex];
                *liveUnit = *persistentUnit;
                liveUnit->deploymentState = 0x01;
                pilotId = assignmentRecord->pilotId;
                liveUnit->pilotId = pilotId;
                liveUnit->gunneryTargetNumber =
                    PilotRoster[pilotId].gunneryIndex;
                liveUnit->pilotExperience =
                    PilotRoster[pilotId].experienceIndex;
                DRAW_ASSIGNMENT(selectedAssignmentIndex);

                if ((unitCardRecord->state & 0x01) != 0x00)
                {
                    unitCardRecord->state ^= 0x01;
                    unitCardRecord->assignmentSlot =
                        (uint8_t) selectedAssignmentIndex;
                    DRAW_UNIT_CARD(selectedUnitCardIndex, 0x0001);
                }
                else
                {
                    oldAssignmentIndex = unitCardRecord->assignmentSlot;
                    previousRecord = &LanceAssignments[oldAssignmentIndex];
                    previousRecord->style ^= 0x04;
                    DRAW_ASSIGNMENT(oldAssignmentIndex);
                    previousRecord->persistentUnitSlot = REVENGE_UNIT_SLOT_NONE;
                    previousRecord->liveUnitSlot = REVENGE_UNIT_SLOT_NONE;
                    unitCardRecord->assignmentSlot =
                        (uint8_t) selectedAssignmentIndex;
                }
            }
        }

        if (unitCardRecord->assignmentSlot != REVENGE_ASSIGNMENT_SLOT_NONE &&
            UnitEditorScenarioCardCount != 0x0000)
        {
            scanIndex = previousLanceSlot;
            do
            {
                oldAssignmentIndex =
                    lanceGroupBase + selectedLanceSlot;
                ++selectedLanceSlot;
                if (selectedLanceSlot >=
                    UnitEditorLanceSlotCount[lanceGroupIndex])
                    selectedLanceSlot = 0x0000;
                if (selectedLanceSlot == scanIndex)
                {
                    finished = TRUE;
                }

                previousRecord =
                    &LanceAssignments[oldAssignmentIndex];
                if ((previousRecord->style & 0x01) != 0x00)
                {
                    previousRecord->style &= 0xFE;
                    DRAW_ASSIGNMENT(oldAssignmentIndex);
                }

                selectedAssignmentIndex =
                    lanceGroupBase + selectedLanceSlot;
                assignmentRecord =
                    &LanceAssignments[selectedAssignmentIndex];
                if (assignmentRecord->liveUnitSlot == REVENGE_UNIT_SLOT_NONE)
                {
                    assignmentRecord->style |= 0x01;
                    DRAW_ASSIGNMENT(selectedAssignmentIndex);
                    break;
                }
            } while (finished == FALSE);

            if (finished != FALSE && exitRequested == FALSE)
            {
                assignmentRecord->style &= 0xFE;
                DRAW_ASSIGNMENT(selectedAssignmentIndex);
                selectedLanceSlot = 0x0004;
                UnitEditorExitButtonState |= 0x01;
                DrawLanceAssignmentSlot(0x0000, TRUE, TRUE);
                finished = FALSE;
            }
            if (exitRequested != FALSE && useSeparateScenarioGroups == FALSE)
                finished = TRUE;
            previousLanceSlot = selectedLanceSlot;
        }
        else if (UnitEditorScenarioCardCount == 0x0000)
        {
            if (exitRequested == FALSE)
            {
                assignmentRecord->style &= 0xFE;
                DRAW_ASSIGNMENT(selectedAssignmentIndex);
                selectedLanceSlot = 0x0004;
                UnitEditorExitButtonState |= 0x01;
                DrawLanceAssignmentSlot(0x0000, TRUE, TRUE);
            }
            else
            {
                finished = TRUE;
            }
        }
    }

    if (selectedLanceSlot == 0x0004)
    {
        UnitEditorExitButtonState &= 0xFE;
        DrawLanceAssignmentSlot(0x0000, TRUE, TRUE);
    }
    else
    {
        LanceAssignments[lanceGroupBase + selectedLanceSlot].style &= 0xFE;
        DRAW_ASSIGNMENT(lanceGroupBase + selectedLanceSlot);
    }
    if (UnitEditorScenarioCardCount != 0x0000)
    {
        UnitSelectionCards[unitCardGroupBase + selectedUnitCard].state &= 0xFD;
        DRAW_UNIT_CARD(unitCardGroupBase + selectedUnitCard, 0x0001);
    }
    REVENGE_PRESERVATION_TRACE("lance: grid complete\n");

#undef DRAW_UNIT_CARD
#undef DRAW_ASSIGNMENT
}

// 2507:4297: draw the fixed background frames shared by lance and pilot
// selection screens.
// VERIFIED against ASM 2507:4297-44F8.
void DrawSelectionScreenBackground(void)
{
    uint16_t recordIndex;
    RevengeUnitLanceAssignment *assignment;
    UnitSelectionCard *unitCard;
    int16_t leftX;
    int16_t topY;
    uint16_t width;

    SelectAndClearTextWindow(0x0000);
    leftX = 0x0000;
    topY = 0x0000;
    FillVideoRectangle(
        leftX, topY, (int16_t) (leftX + 0x0012),
        (int16_t) (topY + 0x0062), 0x000D);
    DrawClippedAxisAlignedLine(
        leftX, topY, (int16_t) (leftX + 0x0013), topY, 0x000E);
    DrawClippedAxisAlignedLine(
        leftX, topY, leftX, (int16_t) (topY + 0x0063), 0x000E);

    topY = 0x0065;
    FillVideoRectangle(
        leftX, topY, (int16_t) (leftX + 0x0012),
        (int16_t) (topY + 0x0062), 0x000D);
    DrawClippedAxisAlignedLine(
        leftX, topY, (int16_t) (leftX + 0x0013), topY, 0x000E);
    DrawClippedAxisAlignedLine(
        leftX, topY, leftX, (int16_t) (topY + 0x0062), 0x000E);

    for (recordIndex = 0x0000; recordIndex < 0x0006; ++recordIndex)
    {
        unitCard = &UnitSelectionCards[recordIndex];
        leftX = (int16_t) unitCard->left - 0x0003;
        topY = (int16_t) unitCard->top;
        if (recordIndex == 0x0002 || recordIndex == 0x0005)
        {
            FillVideoRectangle(
                leftX, topY, (int16_t) (leftX + 0x0044),
                (int16_t) (topY + 0x0062), 0x000D);
            DrawClippedAxisAlignedLine(
                leftX, topY, (int16_t) (leftX + 0x0044), topY, 0x000E);
        }
        else
        {
            FillVideoRectangle(
                leftX, topY, (int16_t) (leftX + 0x0045),
                (int16_t) (topY + 0x0062), 0x000D);
            DrawClippedAxisAlignedLine(
                leftX, topY, (int16_t) (leftX + 0x0046), topY, 0x000E);
        }
        DrawClippedAxisAlignedLine(
            leftX,
            topY,
            leftX,
            (int16_t) (topY + (recordIndex < 0x0003 ? 0x0063 : 0x0062)),
            0x000E);
    }

    for (recordIndex = 0x0000; recordIndex < 0x0004; ++recordIndex)
    {
        assignment = &LanceAssignments[recordIndex];
        leftX = (int16_t) assignment->left;
        topY = (int16_t) assignment->top;
        width = assignment->width;
        FillVideoRectangle(
            leftX, topY, (int16_t) (leftX + width),
            (int16_t) (topY + 0x0017), 0x000D);
        DrawClippedAxisAlignedLine(
            leftX, topY, (int16_t) (leftX + width), topY, 0x000E);
        DrawClippedAxisAlignedLine(
            leftX, topY, leftX, (int16_t) (topY + 0x0017), 0x000E);
    }

    assignment = &LanceAssignments[0x0C];
    leftX = (int16_t) assignment->left;
    topY = (int16_t) assignment->top;
    width = assignment->width;
    FillVideoRectangle(
        leftX, topY, (int16_t) (leftX + width),
        (int16_t) (topY + 0x0017), 0x000D);
    DrawClippedAxisAlignedLine(
        leftX, topY, (int16_t) (leftX + width), topY, 0x000E);
    DrawClippedAxisAlignedLine(
        leftX, topY, leftX, (int16_t) (topY + 0x0017), 0x000E);
}

// 2507:44F9: draw the complete lance-selection screen and its active group.
// VERIFIED against ASM 2507:44F9-45DD.
void DrawLanceSelectionScreen(uint16_t configurationIndex)
{
    int16_t selectedLanceGroup;
    uint16_t scenarioSlot;

    REVENGE_PRESERVATION_TRACE(
        "lance: draw configuration=%u card0=%u,%u,%u card1=%u,%u,%u "
        "assignment0=%u,%u,%u exit=%u,%u,%u\n",
        configurationIndex,
        UnitSelectionCards[0x00].left,
        UnitSelectionCards[0x00].top,
        UnitSelectionCards[0x00].width,
        UnitSelectionCards[0x01].left,
        UnitSelectionCards[0x01].top,
        UnitSelectionCards[0x01].width,
        LanceAssignments[0x00].left,
        LanceAssignments[0x00].top,
        LanceAssignments[0x00].width,
        LanceAssignments[0x0C].left,
        LanceAssignments[0x0C].top,
        LanceAssignments[0x0C].width);

    HideMouseCursorOverlay();
    SetActiveRenderTarget(PrimaryScreenBuffer);
    DrawSelectionScreenBackground();

    if ((int16_t) configurationIndex < 0x0002)
    {
        selectedLanceGroup = 0x0000;
        ClearUnitEditorSectionPanel();
    }
    else
    {
        (void) ValidateLanceConfiguration(configurationIndex - 0x0002);
        selectedLanceGroup = DrawUnitEditorSectionMenu(
            configurationIndex - 0x0002);
    }

    for (scenarioSlot = 0x0000; scenarioSlot < 0x0006; ++scenarioSlot)
    {
        if ((int16_t) scenarioSlot >= (int16_t) UnitEditorScenarioCardCount)
        {
            DrawUnitStatusScreen(
                (uint16_t) (selectedLanceGroup * 0x0006 + scenarioSlot),
                FALSE,
                0xFFFF,
                FALSE);
        }
    }
    DrawRangeAndSpeedStatHeaders(0x0047);
    DrawRangeAndSpeedStatHeaders(0x00AC);

    SetActiveRenderTarget(VideoFramebuffer);
    LoadScreenPalette(0x0023);
    CopyClippedVideoRegion(
        0x0000,
        0x0000,
        0x0000,
        0x0000,
        0x0028,
        0x00C8,
        PrimaryScreenBuffer,
        VideoFramebuffer);
    ShowMouseCursorOverlay();
    RefreshLanceSelectionSlots(
        0x0000,
        (uint16_t) selectedLanceGroup,
        TRUE);
}

// 2507:45DE: redraw scenario cards and assignment buttons for one group.
// VERIFIED against ASM 2507:45DE-46CA.
void RefreshLanceSelectionSlots(
    uint16_t scenarioGroupIndex,
    uint16_t lanceGroupIndex,
    uint16_t showSelectionContents)
{
    uint16_t scenarioRecordIndex;
    uint16_t lastScenarioRecordIndex;
    uint16_t assignmentIndex;
    uint16_t lastAssignmentIndex;
    uint16_t lastActiveAssignmentIndex;

    scenarioRecordIndex = scenarioGroupIndex * 0x0006;
    lastScenarioRecordIndex = scenarioRecordIndex + UnitEditorScenarioCardCount;
    while ((int16_t) scenarioRecordIndex < (int16_t) lastScenarioRecordIndex)
    {
        UnitSelectionCards[scenarioRecordIndex].state &= 0xFD;
        DrawUnitStatusScreen(
            scenarioRecordIndex,
            TRUE,
            showSelectionContents != FALSE ? 0x0000 : 0xFFFF,
            FALSE);
        ++scenarioRecordIndex;
    }

    assignmentIndex = lanceGroupIndex * 0x0004;
    lastAssignmentIndex = assignmentIndex + 0x0004;
    lastActiveAssignmentIndex = assignmentIndex +
        UnitEditorLanceSlotCount[lanceGroupIndex];
    while ((int16_t) assignmentIndex < (int16_t) lastAssignmentIndex)
    {
        if ((int16_t) assignmentIndex < (int16_t) lastActiveAssignmentIndex)
        {
            LanceAssignments[assignmentIndex].style &= 0xFE;
            DrawLanceAssignmentSlot(
                assignmentIndex,
                showSelectionContents != FALSE ? TRUE : FALSE,
                FALSE);
        }
        else
        {
            DrawLanceAssignmentSlot(
                assignmentIndex,
                FALSE,
                FALSE);
        }
        ++assignmentIndex;
    }
    DrawLanceAssignmentSlot(0x0000, TRUE, TRUE);
}

// 2507:46CB: remove and return one random unit type from DS:1B85.
// VERIFIED against ASM 2507:46CB-4709.
int16_t PopRandomUnitType(void)
{
    uint16_t selectedIndex;
    int16_t unitType;
    uint16_t randomUnitTypeCount;

    randomUnitTypeCount = RandomUnitTypeCount;
    selectedIndex = (uint16_t) RandomInRangeInclusive(
        0x0000,
        (uint16_t) (randomUnitTypeCount - 0x0001));
    unitType = RandomUnitTypePool[selectedIndex];
    RandomUnitTypePool[selectedIndex] =
        RandomUnitTypePool[randomUnitTypeCount - 0x0001];
    RandomUnitTypePool[randomUnitTypeCount - 0x0001] =
        (uint8_t) REVENGE_UNIT_TYPE_NONE;
    --RandomUnitTypeCount;
    return unitType;
}

// 2507:470A: draw the four unit-editor section buttons and return the first
// enabled section, or FFFFh when none is enabled.
// VERIFIED against ASM 2507:470A-47DE.
int16_t DrawUnitEditorSectionMenu(uint16_t configurationIndex)
{
    uint16_t buttonColumn;
    uint16_t fontWidth;
    uint16_t fontHeight;
    uint16_t sectionIndex;
    uint16_t buttonRow;
    int16_t firstEnabledSection;
    const char *label;

    HideMouseCursorOverlay();
    FillVideoRectangle(0x00EB, 0x0000, 0x013F, 0x0045, 0x000C);
    buttonColumn = 0x002B;
    fontWidth = ActiveFontGlyphAdvanceX;
    fontHeight = ActiveFontLineHeight;
    firstEnabledSection = -0x0001;
    for (sectionIndex = 0x0000; sectionIndex < 0x0004; ++sectionIndex)
    {
        buttonRow = LanceEditorLayout.UnitEditorSectionButtonRows[sectionIndex];
        label = UnitEditorSectionButtonLabelTextTable[sectionIndex];
        DrawBeveledMenuButton(
            buttonColumn,
            buttonRow,
            label,
            (uint16_t) (fontWidth * 0x0007));

        if (UnitEditorSectionEnabled[configurationIndex * LANCE_EDITOR_SECTION_COUNT + sectionIndex] != FALSE)
        {
            if (firstEnabledSection == -0x0001)
                firstEnabledSection = (int16_t) sectionIndex;
        }
        else
        {
            XorVideoRectangle(
                (int16_t) (buttonColumn * fontWidth),
                (int16_t) (buttonRow * fontHeight),
                (int16_t) ((buttonColumn + 0x0007) * fontWidth - 0x0001),
                (int16_t) ((buttonRow + 0x0001) * fontHeight - 0x0001),
                0x02);
        }
    }
    ShowMouseCursorOverlay();
    return firstEnabledSection;
}

// 2507:47DF: clear and bevel the unit-editor section panel.
// VERIFIED against ASM 2507:47DF-4839.
void ClearUnitEditorSectionPanel(void)
{
    HideMouseCursorOverlay();
    FillVideoRectangle(0x00EB, 0x0000, 0x013F, 0x0045, 0x000D);
    DrawClippedAxisAlignedLine(
        0x00EB, 0x0000, 0x00EB, 0x0045, 0x000E);
    DrawClippedAxisAlignedLine(
        0x00EB, 0x0000, 0x013F, 0x0000, 0x000E);
    ShowMouseCursorOverlay();
}

// 2507:483A: draw one unit card and, when requested, its detailed equipment
// panel. VERIFIED against ASM 2507:483A-4E44.
void DrawUnitStatusScreen(
    uint16_t statusPanelIndex,
    uint16_t redrawBackground,
    uint16_t displayMode,
    uint16_t showDetailedStats)
{
    char textBuffer[0x1C];
    uint8_t *savedRenderTarget;
    UnitSelectionCard *statusPanel;
    RevengeUnitRecord *unitRecord;
    const char *text;
    uint16_t leftX;
    uint16_t topY;
    uint16_t panelWidth;
    uint16_t panelTextColor;
    uint16_t detailTextColor;
    uint16_t weaponListX;
    uint16_t weaponListY;
    uint16_t equippedCount;
    uint16_t armorLabelWidth;
    uint16_t heatLabelWidth;
    uint16_t heatScaleIndex;
    uint16_t unitTypeId;
    uint16_t movementClassIndex;
    uint16_t weaponIndex;
    int16_t armorConditionIndex;
    uint8_t panelFlags;

    HideMouseCursorOverlay();
    savedRenderTarget = ActiveRenderTarget;
    SetActiveRenderTarget(PrimaryScreenBuffer);

    statusPanel = &UnitSelectionCards[statusPanelIndex];
    leftX = statusPanel->left;
    topY = statusPanel->top;
    panelWidth = statusPanel->width;
    panelFlags = statusPanel->state;

    if (displayMode == 0xFFFF)
    {
        FillVideoRectangle(
            leftX + 0x02, topY + 0x02,
            leftX + 0x3D, topY + 0x35, 0x0C);
    }
    else
    {
        FillVideoRectangle(
            leftX + 0x02, topY + 0x02,
            leftX + 0x3D, topY + 0x35,
            LanceEditorLayout.UnitCardBackgroundColorByState[panelFlags]);
        DrawClippedSprite(
            ActiveRenderTarget,
            LanceEditorCardSprite(statusPanel),
            leftX, topY, 0x00);
    }

    DrawClippedAxisAlignedLine(
        leftX, topY, leftX + 0x3F, topY, 0x0E);
    if ((panelFlags >> 0x01) != 0x00)
    {
        DrawClippedRectangleOutline(
            leftX + 0x01, topY + 0x01,
            leftX + 0x3E, topY + 0x36, 0x0F);
    }
    else
    {
        DrawClippedAxisAlignedLine(
            leftX + 0x01, topY + 0x01,
            leftX + 0x01, topY + 0x36, 0x0C);
        DrawClippedAxisAlignedLine(
            leftX + 0x01, topY + 0x01,
            leftX + 0x3E, topY + 0x01, 0x0C);
        DrawClippedAxisAlignedLine(
            leftX + 0x3E, topY + 0x01,
            leftX + 0x3E, topY + 0x36, 0x0E);
        DrawClippedAxisAlignedLine(
            leftX + 0x01, topY + 0x36,
            leftX + 0x3E, topY + 0x36, 0x0E);
    }

    if (redrawBackground != FALSE)
    {
        CopyClippedVideoRegion(
            (int16_t) leftX >> 0x03, topY,
            (int16_t) leftX >> 0x03, topY,
            0x08, 0x38,
            PrimaryScreenBuffer,
            VideoFramebuffer);
    }

    SetActiveRenderTarget(savedRenderTarget);
    ShowMouseCursorOverlay();
    panelTextColor = displayMode == 0xFFFF ? 0x0D : LanceEditorLayout.UnitCardTextColorByState[panelFlags];

    HideMouseCursorOverlay();
    unitRecord = &PersistentUnitRoster[REVENGE_INDEX(PersistentUnitRoster, statusPanel->persistentUnitSlot)];
    unitTypeId = unitRecord->unitTypeId;
    DrawCenteredClippedText(
        UnitNameTable[unitTypeId],
        UnitNameSplitIndex[unitTypeId],
        leftX - 0x01, topY + 0x38, panelWidth,
        (uint8_t) panelTextColor, 0x00);

    RecalculateUnitCombatRatings(
        unitRecord, 0x0018, TRUE, TRUE);
    DrawCombatRatingDescription(
        DisplayedUnitCombatRatings.longFirepower,
        leftX - 0x01, topY + 0x47, (uint8_t) panelTextColor, 0x00);
    DrawCombatRatingDescription(
        DisplayedUnitCombatRatings.mediumFirepower,
        leftX - 0x01, topY + 0x4E, (uint8_t) panelTextColor, 0x00);
    DrawCombatRatingDescription(
        DisplayedUnitCombatRatings.closeFirepower,
        leftX - 0x01, topY + 0x55, (uint8_t) panelTextColor, 0x00);

    movementClassIndex = MovementRatingByWalkMP[DisplayedUnitCombatRatings.movement];
    text = UnitMovementClassLabelTextTable[movementClassIndex];
    strcpy(textBuffer, text);
    if (unitRecord->jumpMovement != 0x00)
    {
        strcat(
            textBuffer,
            UnitStatusSeparator);
    }
    DrawTextString(
        textBuffer, leftX - 0x01, topY + 0x5C,
        (uint8_t) panelTextColor, 0x00);

    if (displayMode != 0x00 && showDetailedStats != FALSE)
    {
        if (displayMode == 0x01 && (panelFlags >> 0x01) != 0x00)
        {
            detailTextColor = LanceEditorLayout.UnitCardTextColorByState[panelFlags];
        }
        else
        {
            detailTextColor = 0x0D;
        }

        DrawCenteredClippedText(
            UnitNameTable[unitTypeId],
            UnitNameSplitIndex[unitTypeId],
            0xEC, 0x02, 0x54, (uint8_t) detailTextColor, 0x00);
        CountUnitWeaponsByType(unitRecord);
        weaponListX = 0xEC;
        weaponListY = 0x11;
        for (weaponIndex = 0x00;
            weaponIndex < WEAPON_DEFINITION_COUNT;
            ++weaponIndex)
        {
            equippedCount = WeaponCountScratch[weaponIndex];
            if (equippedCount == 0x00)
                continue;
            IntegerToString(equippedCount, textBuffer, 0x0A);
            strcat(
                textBuffer,
                UnitStatusSpace);
            text = WeaponNameTable[weaponIndex];
            strcat(textBuffer, text);
            DrawLeftAlignedClippedText(
                textBuffer, weaponListX, weaponListY, 0x54,
                (uint8_t) detailTextColor, 0x00);
            weaponListY = (uint16_t) (weaponListY +
                ActiveFontLineHeight);
        }

        if (unitRecord->beagleActiveProbe != 0x00)
        {
            DrawLeftAlignedClippedText(
                UnitStatusEquipmentSeparator,
                weaponListX, weaponListY, 0x54,
                (uint8_t) detailTextColor, 0x00);
            weaponListY = (uint16_t) (weaponListY +
                ActiveFontLineHeight);
        }
        if (unitRecord->caseProtection != 0x00)
        {
            DrawLeftAlignedClippedText(
                CaseEquipmentText,
                weaponListX, weaponListY, 0x54,
                (uint8_t) detailTextColor, 0x00);
            weaponListY = (uint16_t) (weaponListY +
                ActiveFontLineHeight);
        }
        if (unitRecord->doubleHeatSinks != 0x00)
        {
            DrawLeftAlignedClippedText(
                DoubleHeatSinkEquipmentText,
                weaponListX, weaponListY, 0x54,
                (uint8_t) detailTextColor, 0x00);
        }

        strcpy(
            textBuffer,
            UnitStatusArmorLabel);
        DrawTextString(
            textBuffer, weaponListX, 0x39,
            (uint8_t) detailTextColor, 0x00);
        unitRecord->deploymentState = 0x01;
        CalculateUnitDurabilityTotals(unitRecord);
        unitRecord->deploymentState = 0x00;
        armorConditionIndex = MaximumUnitDurabilityTotal / 0x46;
        if (armorConditionIndex > 0x03)
            armorConditionIndex = 0x03;
        armorLabelWidth = MeasureTextWidth(textBuffer);
        text = UnitArmorConditionLabelTextTable[armorConditionIndex];
        DrawRightAlignedClippedText(
            text, weaponListX + armorLabelWidth, 0x39,
            0x54 - armorLabelWidth, (uint8_t) detailTextColor, 0x00);

        strcpy(
            textBuffer,
            UnitStatusHeatLabel);
        DrawTextString(
            textBuffer, weaponListX,
            (uint16_t) (ActiveFontLineHeight + 0x39),
            (uint8_t) detailTextColor, 0x00);
        heatScaleIndex = CalculateUnitWeaponHeatScale(unitRecord);
        heatLabelWidth = MeasureTextWidth(textBuffer);
        DrawRightAlignedClippedText(
            HeatScaleDescriptionTable[heatScaleIndex],
            weaponListX + heatLabelWidth,
            (uint16_t) (ActiveFontLineHeight + 0x39),
            0x54 - heatLabelWidth,
            (uint8_t) detailTextColor,
            0x00);
    }
    ShowMouseCursorOverlay();
}

// 2507:4E45: draw one lance-assignment button or the EXIT button.
// VERIFIED against ASM 2507:4E45-5015.
void DrawLanceAssignmentSlot(
    uint16_t assignmentIndex,
    uint16_t selected,
    uint16_t drawExitSlot)
{
    RevengeUnitLanceAssignment *assignment;
    int16_t leftX;
    int16_t topY;
    uint16_t width;
    uint8_t flags;
    uint16_t textColor;
    int16_t pilotId;
    int16_t liveUnitIndex;
    int16_t unitTypeId;
    const char *text;

    if (drawExitSlot != FALSE)
        assignmentIndex = 0x000C;

    assignment = &LanceAssignments[assignmentIndex];
    leftX = (int16_t) assignment->left;
    topY = (int16_t) assignment->top;
    width = assignment->width;
    flags = assignment->style;
    if (selected != FALSE)
        textColor = LanceEditorLayout.LanceAssignmentTextColor[flags & 0x01];
    else
        textColor = 0x000D;

    HideMouseCursorOverlay();
    if (drawExitSlot != FALSE)
    {
        DrawCenteredClippedText(
            UnitStatusExitLabel,
            0x0000,
            (uint16_t) leftX,
            (uint16_t) (topY + 0x0002),
            width,
            (uint8_t) textColor,
            0x00);
    }
    else
    {
        pilotId = assignment->pilotId;
        if (pilotId != REVENGE_PILOT_ID_NONE)
        {
            text = PilotSurnameTextTable[pilotId];
            DrawCenteredClippedText(
                text,
                0x0000,
                (uint16_t) leftX,
                (uint16_t) (topY + 0x0002),
                width,
                (uint8_t) textColor,
                0x00);
        }
    }

    if ((flags & 0x01) != 0x00 && selected != FALSE)
    {
        DrawClippedRectangleOutline(
            leftX,
            topY,
            (int16_t) (leftX + width),
            (int16_t) (topY + 0x0017),
            0x000F);
    }
    else
    {
        DrawClippedAxisAlignedLine(
            leftX, topY, (int16_t) (leftX + width), topY, 0x000E);
        DrawClippedAxisAlignedLine(
            leftX, topY, leftX, (int16_t) (topY + 0x0017), 0x000E);
        DrawClippedAxisAlignedLine(
            (int16_t) (leftX + width),
            (int16_t) (topY + 0x0001),
            (int16_t) (leftX + width),
            (int16_t) (topY + 0x0017),
            0x000D);
        DrawClippedAxisAlignedLine(
            (int16_t) (leftX + 0x0001),
            (int16_t) (topY + 0x0017),
            (int16_t) (leftX + width),
            (int16_t) (topY + 0x0017),
            0x000D);
    }

    liveUnitIndex = assignment->liveUnitSlot;
    if (liveUnitIndex != REVENGE_UNIT_SLOT_NONE)
    {
        if ((flags & 0x04) == 0x00)
            textColor = 0x000D;
        unitTypeId = LiveScenarioUnits[liveUnitIndex].unitTypeId;
        text = UnitNameTable[unitTypeId];
        DrawCenteredClippedText(
            text,
            0x0000,
            (uint16_t) leftX,
            (uint16_t) (topY + ActiveFontLineHeight +
                0x0004),
            width,
            (uint8_t) textColor,
            0x00);
    }
    ShowMouseCursorOverlay();
}
