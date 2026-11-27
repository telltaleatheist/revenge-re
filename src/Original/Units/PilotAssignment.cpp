// 2507: pilot assignment
// PilotAssignment.cpp
// Reconstructed from the original procedures in segment 2507.

#include "LanceEditorInternal.h"
#include "Campaign/Campaign.h"
#include "Data/RuntimeData.h"
#include "Data/StartupData.h"
#include "Data/TacticalData.h"
#include "Graphics/Graphics.h"
#include "Input/Input.h"
#include "Runtime/Runtime.h"
#include "Utility/Utility.h"
#include "revenge/text.h"

// 2507:5016: prepare pilot-selection records and portrait buffers.
// VERIFIED against ASM 2507:5016-5116.
void PreparePilotSelectionRecords(
    uint16_t selectionFamily,
    int16_t selectionMode,
    uint16_t loadPresentationResources)
{
    uint16_t pilotRecordIndex;
    UnitSelectionCard *pilotRecord;
    RevengePilotId pilotId;
    uint16_t portraitBufferIndex;

    if (loadPresentationResources != FALSE)
    {
        LoadGraphicsResourceToBuffers(
            0x0025,
            SecondaryScreenBuffer,
            PrimaryScreenBuffer);
        LoadGraphicsBufferRange(0x0017, 0x001E);
        LoadGraphicsResourceToBuffers(
            0x0026,
            SecondaryScreenBuffer,
            PrimaryScreenBuffer);
        LoadGraphicsBufferRange(0x001F, 0x0025);
    }

    if (selectionMode != 0x0000)
    {
        UnitEditorPilotCardCount =
            UnitEditorPilotCardCountByFamily[selectionFamily];
    }

    for (pilotRecordIndex = 0x0000;
        (int16_t) pilotRecordIndex < (int16_t) UnitEditorPilotCardCount;
        ++pilotRecordIndex)
    {
        pilotRecord = &UnitSelectionCards[pilotRecordIndex];
        if (selectionMode != 0x0000)
        {
            pilotId = (RevengePilotId) PopRandomPilotId();
            pilotRecord->selectionId = pilotId;
            pilotRecord->alternateId = REVENGE_PILOT_ID_NONE;
        }

        pilotRecord->state = 0x01;
        pilotId = pilotRecord->selectionId;
        portraitBufferIndex = PilotPortraitIndexById[pilotId];
        pilotRecord->spriteBuffer =
            UnitEditorGraphicsBuffer(portraitBufferIndex);
    }

    if (selectionMode != 0x0000)
        (void) PopRandomPilotId();
    InitializeLanceAssignments(
        selectionFamily + 0x0003,
        selectionMode);
}

// 2507:5117: prepare, display, and edit one pilot-selection family.
// VERIFIED against ASM 2507:5117-5151.
void RunPilotSelection(
    uint16_t selectionFamily,
    int16_t selectionMode)
{
    SceneTransitionAbortFlag = FALSE;
    PreparePilotSelectionRecords(
        selectionFamily,
        selectionMode,
        TRUE);
    DrawPilotSelectionScreen(selectionFamily);
    RunPilotAssignmentEditor(selectionFamily);
    FreeGraphicsBufferRange(0x0017, 0x0025);
}

// 2507:5152: validate pilot assignments for one selection family.
// VERIFIED against ASM 2507:5152-51EB.
uint16_t ValidatePilotConfiguration(uint16_t selectionFamily)
{
    uint16_t configurationValid;
    uint16_t lanceGroup;
    uint16_t unassignedPilotCount;
    uint16_t pilotRecordIndex;

    configurationValid = TRUE;
    lanceGroup = 0x0000;
    if (UnitEditorPilotCardCount != 0x0000)
    {
        do
        {
            if (UnitEditorSectionEnabled[selectionFamily * LANCE_EDITOR_SECTION_COUNT] != FALSE)
            {
                unassignedPilotCount = UnitEditorPilotCardCount;
                for (pilotRecordIndex = 0x0000;
                    (int16_t) pilotRecordIndex <
                    (int16_t) UnitEditorPilotCardCount;
                    ++pilotRecordIndex)
                {
                    if ((UnitSelectionCards[pilotRecordIndex].state & 0x01) == 0x00)
                    {
                        --unassignedPilotCount;
                    }
                }

                if (unassignedPilotCount != 0x0000 &&
                    (int16_t) UnitEditorLanceSlotCount[lanceGroup] <
                        0x0004)
                {
                    configurationValid = FALSE;
                }
            }
            ++lanceGroup;
        } while (lanceGroup < 0x0003 && configurationValid != FALSE);
    }

    UnitEditorSectionEnabled[selectionFamily * LANCE_EDITOR_SECTION_COUNT + 0x03] =
        (uint8_t) configurationValid;
    return configurationValid;
}

// 2507:51EC: select a lance section and enter its pilot-assignment grid.
// VERIFIED against ASM 2507:51EC-55C5.
void RunPilotAssignmentEditor(uint16_t selectionFamily)
{
    uint16_t configurationIndex;
    uint16_t selectedSection;
    uint16_t previousSection;
    uint16_t sectionBeforeInput;
    uint16_t sectionIndex;
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

    configurationIndex = selectionFamily + 0x0001;
    selectedSection = 0x0000;
    while (UnitEditorSectionEnabled[configurationIndex * LANCE_EDITOR_SECTION_COUNT + selectedSection] == 0x00 &&
        selectedSection < 0x0003)
    {
        ++selectedSection;
    }
    previousSection = selectedSection;
    textLeftColumn = 0x002B;
    fontWidth = ActiveFontGlyphAdvanceX;
    fontHeight = ActiveFontLineHeight;

    sectionRow = (int16_t) LanceEditorLayout.PilotEditorSectionRows[selectedSection];
    HideMouseCursorOverlay();
    XorVideoRectangle(
        (int16_t) (textLeftColumn * fontWidth),
        (int16_t) (sectionRow * fontHeight),
        (int16_t) ((textLeftColumn + 0x0007) * fontWidth - 0x0001),
        (int16_t) ((sectionRow + 0x0001) * fontHeight - 0x0001),
        0x01);
    ShowMouseCursorOverlay();
    FlushPendingInput();
    finished = FALSE;

    while (finished == FALSE)
    {
        activateSection = FALSE;
        sectionBeforeInput = selectedSection;
        inputCode = 0x0000;
        if (ServiceMusicAndCheckKeyboardInput() != 0x0000)
        {
            inputCode = NormalizeNavigationKeyCode(
                ReadTranslatedKeypress());
            if (inputCode == KEY_ESCAPE)
            {
                if (UnitEditorSectionEnabled[configurationIndex * LANCE_EDITOR_SECTION_COUNT + 0x03] != 0x00)
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
                UnitEditorSectionEnabled[configurationIndex * LANCE_EDITOR_SECTION_COUNT + 0x03] != 0x00)
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
                    sectionRow = (int16_t) LanceEditorLayout.PilotEditorSectionRows[sectionIndex];
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
            sectionRow = (int16_t) LanceEditorLayout.PilotEditorSectionRows[sectionBeforeInput];
            XorVideoRectangle(
                (int16_t) (textLeftColumn * fontWidth),
                (int16_t) (sectionRow * fontHeight),
                (int16_t) ((textLeftColumn + 0x0007) * fontWidth - 0x0001),
                (int16_t) ((sectionRow + 0x0001) * fontHeight - 0x0001),
                0x01);
            WaitVerticalRetraces(0x0002);
            sectionRow = (int16_t) LanceEditorLayout.PilotEditorSectionRows[selectedSection];
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
            if (previousSection != 0x0003)
                RefreshPilotAssignmentSlots(previousSection, FALSE);
            if (selectedSection != 0x0003)
                RefreshPilotAssignmentSlots(selectedSection, TRUE);
            previousSection = selectedSection;
            FlushPendingInput();
        }

        if (activateSection != FALSE && UnitEditorPilotCardCount != 0x0000)
        {
            RunPilotAssignmentGrid(selectedSection);
            (void) ValidatePilotConfiguration(configurationIndex);
            (void) DrawUnitEditorSectionMenu(configurationIndex);
            HideMouseCursorOverlay();
            sectionRow = (int16_t) LanceEditorLayout.PilotEditorSectionRows[selectedSection];
            XorVideoRectangle(
                (int16_t) (textLeftColumn * fontWidth),
                (int16_t) (sectionRow * fontHeight),
                (int16_t) ((textLeftColumn + 0x0007) * fontWidth - 0x0001),
                (int16_t) ((sectionRow + 0x0001) * fontHeight - 0x0001),
                0x01);
            ShowMouseCursorOverlay();
            FlushPendingInput();
        }
    }
}

// 2507:55C6: assign the remaining pilot cards to one lance's unit slots.
// VERIFIED against ASM 2507:55C6-5C25.
void RunPilotAssignmentGrid(uint16_t lanceGroupIndex)
{
    uint16_t selectedLanceSlot;
    uint16_t previousLanceSlot;
    uint16_t selectedAssignmentIndex;
    uint16_t previousAssignmentIndex;
    uint16_t assignmentGroupBase;
    uint16_t selectedPilotRecord;
    uint16_t previousPilotRecord;
    uint16_t sourcePilotRecord;
    uint16_t destinationPilotRecord;
    uint16_t actionRequested;
    uint16_t finished;
    uint16_t mouseButtonState;
    uint16_t mouseButtonEvent;
    RevengeUnitLanceAssignment *assignment;
    UnitSelectionCard *pilotCard;
    UnitSelectionCard *sourcePilotCard;
    UnitSelectionCard *destinationPilotCard;
    int16_t inputCode;
    int16_t mouseX;
    int16_t mouseY;

    selectedLanceSlot = 0x0000;
    previousLanceSlot = selectedLanceSlot;
    assignmentGroupBase = lanceGroupIndex * 0x0004;
    selectedAssignmentIndex = assignmentGroupBase + selectedLanceSlot;
    assignment = &LanceAssignments[selectedAssignmentIndex];
    assignment->style |= 0x01;
    DrawLanceAssignmentSlot(selectedAssignmentIndex, TRUE, FALSE);

    selectedPilotRecord = 0x0000;
    previousPilotRecord = selectedPilotRecord;
    pilotCard = &UnitSelectionCards[selectedPilotRecord];
    pilotCard->state |= 0x02;
    DrawPilotSelectionSlot(selectedPilotRecord, TRUE, 0x0001);
    FlushPendingInput();
    finished = FALSE;

    while (finished == FALSE)
    {
        actionRequested = FALSE;
        inputCode = 0x0000;
        if (ServiceMusicAndCheckKeyboardInput() != 0x0000)
        {
            inputCode = NormalizeNavigationKeyCode(
                ReadTranslatedKeypress());
            if (inputCode == KEY_ESCAPE)
            {
                finished = TRUE;
                actionRequested = TRUE;
            }
            else if (inputCode == TranslatedExtendedKey(KEY_UP))
            {
                --selectedLanceSlot;
            }
            else if (inputCode == TranslatedExtendedKey(KEY_DOWN))
            {
                ++selectedLanceSlot;
            }
            else if (inputCode == TranslatedExtendedKey(KEY_LEFT))
            {
                --selectedPilotRecord;
            }
            else if (inputCode == TranslatedExtendedKey(KEY_RIGHT))
            {
                ++selectedPilotRecord;
            }
            else if (inputCode == KEY_CARRIAGE_RETURN || inputCode == KEY_SPACE)
            {
                if (selectedLanceSlot == 0x0004)
                    finished = TRUE;
                actionRequested = TRUE;
            }
            if (selectedLanceSlot > 0x0004)
                selectedLanceSlot = 0x0000;
            if (selectedLanceSlot > UnitEditorLanceSlotCount[lanceGroupIndex])
                selectedLanceSlot = 0x0004;
            if ((int16_t) selectedLanceSlot < 0x0000)
                selectedLanceSlot = 0x0004;
            if (selectedPilotRecord >= UnitEditorPilotCardCount)
                selectedPilotRecord = 0x0000;
            if ((int16_t) selectedPilotRecord < 0x0000)
                selectedPilotRecord = UnitEditorPilotCardCount - 0x0001;
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
                finished = TRUE;
                actionRequested = TRUE;
            }
            else
            {
                for (sourcePilotRecord = 0x0000;
                    sourcePilotRecord < UnitEditorPilotCardCount;
                    ++sourcePilotRecord)
                {
                    sourcePilotCard = &UnitSelectionCards[sourcePilotRecord];
                    if (mouseX >= (int16_t) (sourcePilotCard->left + 0x0001) &&
                        mouseX < (int16_t) (sourcePilotCard->left + 0x003F) &&
                        mouseY >= (int16_t) (sourcePilotCard->top + 0x0001) &&
                        mouseY < (int16_t) (sourcePilotCard->top + 0x0037))
                    {
                        selectedPilotRecord = sourcePilotRecord;
                        if (mouseButtonEvent != 0x0000 &&
                            (mouseButtonState & MOUSE_BUTTON_LEFT) != 0x0000)
                        {
                            if (selectedLanceSlot == 0x0004)
                                finished = TRUE;
                            actionRequested = TRUE;
                        }
                    }
                }

                for (sourcePilotRecord = 0x0000;
                    sourcePilotRecord <= UnitEditorLanceSlotCount[lanceGroupIndex] &&
                    sourcePilotRecord < 0x0004;
                    ++sourcePilotRecord)
                {
                    assignment = &LanceAssignments[assignmentGroupBase + sourcePilotRecord];
                    if (mouseX >= (int16_t) assignment->left &&
                        mouseX < (int16_t) (assignment->left + 0x0054) &&
                        mouseY >= (int16_t) assignment->top &&
                        mouseY < (int16_t) (assignment->top + 0x0018))
                    {
                        if (mouseButtonEvent != 0x0000 &&
                            (mouseButtonState & MOUSE_BUTTON_LEFT) != 0x0000 &&
                            selectedLanceSlot == sourcePilotRecord)
                            actionRequested = TRUE;
                        selectedLanceSlot = sourcePilotRecord;
                    }
                }

                if (mouseX >= (int16_t) UnitEditorExitButtonLeft &&
                    mouseX < (int16_t) (UnitEditorExitButtonLeft + 0x0054) &&
                    mouseY >= (int16_t) UnitEditorExitButtonTop &&
                    mouseY < (int16_t) (UnitEditorExitButtonTop + 0x0018))
                {
                    if (mouseButtonEvent != 0x0000 &&
                        (mouseButtonState & MOUSE_BUTTON_LEFT) != 0x0000)
                    {
                        if (selectedLanceSlot == 0x0004)
                            finished = TRUE;
                        actionRequested = TRUE;
                    }
                    selectedLanceSlot = 0x0004;
                }
            }
        }

        selectedAssignmentIndex = assignmentGroupBase + selectedLanceSlot;
        previousAssignmentIndex = assignmentGroupBase + previousLanceSlot;
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
                DrawLanceAssignmentSlot(
                    previousAssignmentIndex, TRUE, FALSE);
            }
            if (selectedLanceSlot == 0x0004)
            {
                UnitEditorExitButtonState |= 0x01;
                DrawLanceAssignmentSlot(0x0000, TRUE, TRUE);
            }
            else
            {
                LanceAssignments[selectedAssignmentIndex].style |= 0x01;
                DrawLanceAssignmentSlot(
                    selectedAssignmentIndex, TRUE, FALSE);
            }
            previousLanceSlot = selectedLanceSlot;
        }

        if (selectedPilotRecord != previousPilotRecord)
        {
            UnitSelectionCards[previousPilotRecord].state &= 0xFD;
            DrawPilotSelectionSlot(previousPilotRecord, TRUE, 0x0001);
            UnitSelectionCards[selectedPilotRecord].state |= 0x02;
            DrawPilotSelectionSlot(selectedPilotRecord, TRUE, 0x0001);
            previousPilotRecord = selectedPilotRecord;
        }

        if (actionRequested == FALSE || finished != FALSE ||
            UnitEditorLanceSlotCount[lanceGroupIndex] >= 0x0004)
            continue;
        assignment = &LanceAssignments[selectedAssignmentIndex];
        if ((assignment->style & 0x02) != 0x00)
            continue;

        pilotCard = &UnitSelectionCards[selectedPilotRecord];
        assignment->pilotId = pilotCard->selectionId;
        assignment->style |= 0x02;
        DrawLanceAssignmentSlot(selectedAssignmentIndex, TRUE, FALSE);
        pilotCard->state = 0x00;
        DrawPilotSelectionSlot(selectedPilotRecord, TRUE, -0x0001);
        pilotCard->selectionId = REVENGE_PILOT_ID_NONE;
        pilotCard->alternateId = REVENGE_PILOT_ID_NONE;
        ++UnitEditorLanceSlotCount[lanceGroupIndex];
        --UnitEditorPilotCardCount;
        if ((int16_t) UnitEditorPilotCardCount <= 0x0000)
        {
            UnitEditorPilotCardCount = 0x0000;
            finished = TRUE;
            continue;
        }
        if (selectedPilotRecord >= UnitEditorPilotCardCount)
            selectedPilotRecord = UnitEditorPilotCardCount - 0x0001;

        destinationPilotRecord = 0x0000;
        for (sourcePilotRecord = 0x0000;
            sourcePilotRecord <= UnitEditorPilotCardCount;
            ++sourcePilotRecord)
        {
            sourcePilotCard = &UnitSelectionCards[sourcePilotRecord];
            if (sourcePilotCard->state == 0x00)
                continue;
            destinationPilotCard = &UnitSelectionCards[destinationPilotRecord];
            if (destinationPilotRecord != sourcePilotRecord)
            {
                destinationPilotCard->state = sourcePilotCard->state;
                destinationPilotCard->selectionId = sourcePilotCard->selectionId;
                destinationPilotCard->alternateId = sourcePilotCard->alternateId;
                destinationPilotCard->spriteBuffer = sourcePilotCard->spriteBuffer;
                DrawPilotSelectionSlot(
                    destinationPilotRecord, TRUE, 0x0001);
                sourcePilotCard->state = 0x00;
                DrawPilotSelectionSlot(
                    sourcePilotRecord, TRUE, -0x0001);
                sourcePilotCard->selectionId = REVENGE_PILOT_ID_NONE;
                sourcePilotCard->alternateId = REVENGE_PILOT_ID_NONE;
            }
            if (destinationPilotRecord == selectedPilotRecord)
            {
                destinationPilotCard->state |= 0x02;
                DrawPilotSelectionSlot(
                    destinationPilotRecord, TRUE, 0x0001);
                previousPilotRecord = selectedPilotRecord;
            }
            ++destinationPilotRecord;
        }
        if (UnitEditorLanceSlotCount[lanceGroupIndex] == 0x0004)
            finished = TRUE;
    }

    if (selectedLanceSlot != 0x0004)
    {
        previousAssignmentIndex = assignmentGroupBase + previousLanceSlot;
        LanceAssignments[previousAssignmentIndex].style &= 0xFE;
        DrawLanceAssignmentSlot(previousAssignmentIndex, TRUE, FALSE);
        UnitEditorExitButtonState |= 0x01;
        DrawLanceAssignmentSlot(0x0000, TRUE, TRUE);
    }
    UnitEditorExitButtonState &= 0xFE;
    DrawLanceAssignmentSlot(0x0000, TRUE, TRUE);
    if (UnitEditorPilotCardCount != 0x0000)
    {
        UnitSelectionCards[selectedPilotRecord].state &= 0xFD;
        DrawPilotSelectionSlot(selectedPilotRecord, TRUE, 0x0001);
    }
}

// 2507:5C26: draw the complete pilot-selection screen and active family.
// VERIFIED against ASM 2507:5C26-5CE6.
void DrawPilotSelectionScreen(uint16_t selectionFamily)
{
    int16_t selectedLanceGroup;
    uint16_t pilotSlot;

    HideMouseCursorOverlay();
    SetActiveRenderTarget(PrimaryScreenBuffer);
    DrawSelectionScreenBackground();
    (void) ValidatePilotConfiguration(selectionFamily + 0x0001);
    selectedLanceGroup = DrawUnitEditorSectionMenu(
        selectionFamily + 0x0001);

    for (pilotSlot = 0x0000; pilotSlot < 0x0006; ++pilotSlot)
    {
        if ((int16_t) pilotSlot >= (int16_t) UnitEditorPilotCardCount)
        {
            DrawPilotSelectionSlot(
                pilotSlot,
                FALSE,
                -0x0001);
        }
    }
    DrawExperienceAndGunneryHeaders(0x0047);
    DrawExperienceAndGunneryHeaders(0x00AC);

    SetActiveRenderTarget(VideoFramebuffer);
    LoadScreenPalette(0x0025);
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
    RefreshPilotAssignmentSlots(
        (uint16_t) selectedLanceGroup,
        TRUE);
}

// 2507:5CE7: redraw pilot cards and the assignment slots for one family.
// VERIFIED against ASM 2507:5CE7-5DBD.
void RefreshPilotAssignmentSlots(
    uint16_t selectionFamily,
    uint16_t showPilotCards)
{
    uint16_t pilotRecordIndex;
    uint16_t assignmentIndex;
    uint16_t lastAssignmentIndex;

    for (pilotRecordIndex = 0x0000;
        (int16_t) pilotRecordIndex < (int16_t) UnitEditorPilotCardCount;
        ++pilotRecordIndex)
    {
        UnitSelectionCards[pilotRecordIndex].state &= 0xFD;
        DrawPilotSelectionSlot(
            pilotRecordIndex,
            TRUE,
            showPilotCards != FALSE ? 0x0000 : -0x0001);
    }

    assignmentIndex = selectionFamily * 0x0004;
    lastAssignmentIndex = assignmentIndex +
        UnitEditorLanceSlotCount[selectionFamily];
    while ((int16_t) assignmentIndex < (int16_t) lastAssignmentIndex)
    {
        LanceAssignments[assignmentIndex].style &= 0xFE;
        DrawLanceAssignmentSlot(
            assignmentIndex,
            showPilotCards != FALSE ? TRUE : FALSE,
            FALSE);
        ++assignmentIndex;
    }

    DrawLanceAssignmentSlot(0x0000, TRUE, TRUE);
}

// 2507:5DBE: draw one pilot portrait card and its identifying text.
// VERIFIED against ASM 2507:5DBE-6092.
void DrawPilotSelectionSlot(
    uint16_t pilotRecordIndex,
    uint16_t redrawBackground,
    int16_t displayMode)
{
    uint8_t *savedRenderTarget;
    UnitSelectionCard *pilotRecord;
    RevengePilotRosterRecord *pilot;
    int16_t leftX;
    int16_t topY;
    uint16_t width;
    uint8_t flags;
    uint16_t textColor;
    int16_t pilotId;
    int16_t identityIndex;
    int16_t labelIndex;
    const char *text;

    HideMouseCursorOverlay();
    savedRenderTarget = ActiveRenderTarget;
    SetActiveRenderTarget(PrimaryScreenBuffer);

    pilotRecord = &UnitSelectionCards[pilotRecordIndex];
    leftX = (int16_t) pilotRecord->left;
    topY = (int16_t) pilotRecord->top;
    width = pilotRecord->width;
    flags = pilotRecord->state;
    if (displayMode == -0x0001)
    {
        FillVideoRectangle(
            (int16_t) (leftX + 0x0002),
            (int16_t) (topY + 0x0002),
            (int16_t) (leftX + 0x003D),
            (int16_t) (topY + 0x0035),
            0x000C);
    }
    else
    {
        DrawClippedSprite(
            ActiveRenderTarget,
            LanceEditorCardSprite(pilotRecord),
            leftX,
            topY,
            0x00);
    }

    DrawClippedAxisAlignedLine(
        leftX, topY, (int16_t) (leftX + 0x003F), topY, 0x000E);
    if ((flags & 0xFE) != 0x00)
    {
        DrawClippedRectangleOutline(
            (int16_t) (leftX + 0x0001),
            (int16_t) (topY + 0x0001),
            (int16_t) (leftX + 0x003E),
            (int16_t) (topY + 0x0036),
            0x000F);
    }
    else
    {
        DrawClippedAxisAlignedLine(
            (int16_t) (leftX + 0x0001),
            (int16_t) (topY + 0x0001),
            (int16_t) (leftX + 0x0001),
            (int16_t) (topY + 0x0036),
            0x000C);
        DrawClippedAxisAlignedLine(
            (int16_t) (leftX + 0x0001),
            (int16_t) (topY + 0x0001),
            (int16_t) (leftX + 0x003E),
            (int16_t) (topY + 0x0001),
            0x000C);
        DrawClippedAxisAlignedLine(
            (int16_t) (leftX + 0x003E),
            (int16_t) (topY + 0x0001),
            (int16_t) (leftX + 0x003E),
            (int16_t) (topY + 0x0036),
            0x000E);
        DrawClippedAxisAlignedLine(
            (int16_t) (leftX + 0x0001),
            (int16_t) (topY + 0x0036),
            (int16_t) (leftX + 0x003E),
            (int16_t) (topY + 0x0036),
            0x000E);
    }

    if (redrawBackground != FALSE)
    {
        CopyClippedVideoRegion(
            (int16_t) (leftX >> 0x03),
            topY,
            (int16_t) (leftX >> 0x03),
            topY,
            0x0008,
            0x0038,
            PrimaryScreenBuffer,
            VideoFramebuffer);
    }
    SetActiveRenderTarget(savedRenderTarget);
    ShowMouseCursorOverlay();

    if (displayMode == -0x0001)
        textColor = 0x000D;
    else
        textColor = LanceEditorLayout.PilotCardTextColor[flags];

    HideMouseCursorOverlay();
    pilotId = pilotRecord->selectionId;
    pilot = &PilotRoster[pilotId];
    identityIndex = pilot->identityId;
    text = PilotGivenNameTextTable[identityIndex];
    DrawCenteredClippedText(
        text,
        0x0000,
        (uint16_t) (leftX - 0x0001),
        (uint16_t) (topY + 0x0038),
        width,
        (uint8_t) textColor,
        0x00);

    topY = (int16_t) (topY + ActiveFontLineHeight);
    text = PilotSurnameTextTable[identityIndex];
    DrawCenteredClippedText(
        text,
        0x0000,
        (uint16_t) (leftX - 0x0001),
        (uint16_t) (topY + 0x0038),
        width,
        (uint8_t) textColor,
        0x00);

    topY = (int16_t) (topY + 0x0047 -
        ActiveFontLineHeight);
    labelIndex = pilot->experienceIndex;
    text = PilotExperienceLabelTextTable[labelIndex];
    DrawLeftAlignedClippedText(
        text,
        (uint16_t) leftX,
        (uint16_t) topY,
        width,
        (uint8_t) textColor,
        0x00);

    topY = (int16_t) (topY + 0x0009);
    labelIndex = pilot->gunneryIndex - 0x0002;
    text = PilotGunneryLabelTextTable[labelIndex];
    DrawLeftAlignedClippedText(
        text,
        (uint16_t) leftX,
        (uint16_t) topY,
        width,
        (uint8_t) textColor,
        0x00);
    ShowMouseCursorOverlay();
}

// 2507:6093: remove and return one random pilot ID from DS:1BAD.
// VERIFIED against ASM 2507:6093-60D1.
int16_t PopRandomPilotId(void)
{
    uint16_t selectedIndex;
    int16_t pilotId;
    uint16_t randomPilotCount;

    randomPilotCount = RandomSelectionPoolCount;
    selectedIndex = (uint16_t) RandomInRangeInclusive(
        0x0000,
        (uint16_t) (randomPilotCount - 0x0001));
    pilotId = RandomPilotPool[selectedIndex];
    RandomPilotPool[selectedIndex] =
        RandomPilotPool[randomPilotCount - 0x0001];
    RandomPilotPool[randomPilotCount - 0x0001] =
        (uint8_t) REVENGE_PILOT_ID_NONE;
    --RandomSelectionPoolCount;
    return pilotId;
}
