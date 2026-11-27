// 2208: tactical command, formation and targeting declarations
#ifndef REVENGE_TACTICAL_COMMANDS_INTERNAL_H
#define REVENGE_TACTICAL_COMMANDS_INTERNAL_H

// Tactical input, command, formation, and intelligence procedures reconstructed
// from REVENGE.EXE and the Revenge annotation project.

#include "REVENGE.h"
#include "revenge/input.h"
#include "revenge/original_data.h"

#define TACTICAL_ORDER_SCRIPTED_ROUTE   0x00
#define TACTICAL_ORDER_HOLD_POSITION    0x01
#define TACTICAL_ORDER_SEEK_AND_DESTROY 0x02

#define TACTICAL_FORCE_FIRST_UNIT_OFFSET            FRIENDLY_FORCE_FIRST_UNIT
#define TACTICAL_ORDER_MODE_COUNT                   0x03
#define NARRATION_LEADER_SUBSTITUTION_FIRST_UNIT    0x0004
#define CONTACT_NARRATION_CATEGORY_SCRIPTED_ROUTE   0x0006
#define CONTACT_NARRATION_CATEGORY_HOLD_POSITION    0x0004
#define CONTACT_NARRATION_CATEGORY_SEEK_AND_DESTROY 0x0003
#define NO_UNIT_INDEX                               0xFFFF

void LoadUnitTemplateRecord(int16_t unitType);
void UpdateUnitSpriteForFacing(uint16_t unitIndex, uint16_t facing);
void RecalculateUnitCombatRatings(
    RevengeUnitRecord *unitRecord,
    uint16_t unitSlot,
    uint16_t rebuildWeaponCounts,
    uint16_t advanceRecovery);
uint16_t HasUnitLineOfSight(int16_t sourceUnitIndex, int16_t targetUnitIndex);
uint16_t CalculateApproximateMapDistance(
    int16_t sourceX, int16_t sourceY, int16_t targetX, int16_t targetY);
uint16_t DetermineRelativeAttackDirection(
    int16_t sourceX, int16_t sourceY, int16_t targetX, int16_t targetY);
uint16_t FindOperationalLanceLeader(uint16_t unitIndex);
uint16_t HasNarrationMessage(
    uint16_t narrationSubject, uint16_t messageCategory);
uint16_t GeneratePseudoRandomByte(void);
int16_t CompareLanceCombatRatings(
    uint16_t firstUnitIndex, int16_t secondUnitIndex);
void DrawLanceFormationMarker(
    int16_t formationCode, uint16_t tileColumnOffset);
void DrawTacticalUnitControlPanel(uint16_t unitIndex);
void FormatTacticalMapCoordinates(uint16_t mapX, uint16_t mapY);
void HighlightLanceFollowers(uint16_t leaderUnitIndex);
uint16_t HasActiveLanceFollowers(uint16_t unitIndex);
void SelectLanceFormationMode(
    uint16_t leaderUnitIndex, uint16_t randomizeFormation);
void RunTacticalUnitCommandMenu(uint16_t unitIndex);
void RunTacticalGameOptionsMenu(void);
uint16_t ValidatePrimaryLanceLeader(uint16_t unitIndex);
void RefreshTacticalDisplay(uint16_t advanceEffects);
void RedrawTacticalStatusPanel(void);
void RebuildTacticalViewport(void);
void HideMouseCursorOverlay(void);
void ShowMouseCursorOverlay(void);
void SelectAndClearTextWindow(uint16_t windowIndex);
void DrawActiveTextWindowBorder(uint16_t color);
void DrawTacticalCellOutline(int16_t leftX, int16_t topY, uint16_t color);
void DrawClippedPixel(int16_t x, int16_t y, uint16_t color);
void DrawTacticalCursorOutline(uint16_t color);
void DrawClippedWeaponTraceLine(
    int16_t startX,
    int16_t startY,
    int16_t endX,
    int16_t endY,
    uint16_t color,
    uint16_t dashedTrace);
void DrawWeaponAttackTrace(
    uint16_t attackerUnitIndex,
    int16_t targetMapX,
    int16_t targetMapY,
    uint16_t offsetEndpoints,
    uint16_t shotHit,
    uint16_t weaponIndex);
void DrawCenteredTextInActiveWindow(
    const char *text,
    uint16_t rowOffset,
    uint16_t foregroundColor,
    uint16_t backgroundColor);
void RenderFormattedLineBreak(void);
void RenderFormattedText(const char *formattedText);
void FormatUnitDurabilityPercentage(int16_t unitIndex);
void DrawTileAlignedText(
    const char *text,
    uint16_t tileX,
    uint16_t tileY,
    uint8_t foregroundColor,
    uint8_t backgroundColor);
int16_t RunOptionsMenuSelection(int16_t firstRegionIndex);
void EnterTacticalOverview(void);
void SelectTacticalMapCoordinate(uint16_t unitIndex);
uint16_t SelectTacticalMoveDestinationInput(uint16_t unitIndex, uint16_t mode);
void SelectStrafingArea(void);
void DisplayDropShipIntelligence(void);
void RenderLanceIntelligenceSummary(uint16_t firstUnitIndex);
void DrawUnitFacingAndMovementVectors(uint16_t unitIndex);
int16_t UpdateUnitEnemyVisibility(uint16_t unitIndex);
int16_t UpdateUnitEnemyProximityState(uint16_t unitIndex);
uint16_t WaitForDismissInput(void);
uint16_t ServiceMusicAndCheckKeyboardInput(void);
int16_t TranslateTacticalMouseInput(int16_t inputAction);
void MoveTacticalCursorByInput(int16_t inputCode);
void ScrollTacticalViewportToCursor(void);
void FocusTacticalViewOnUnit(uint16_t unitIndex);
void RefreshTacticalCoordinateSelection(
    uint16_t unitIndex,
    uint16_t selectionMode,
    uint16_t savedCursorColumn);
void CycleTacticalEnemyTarget(void);
void ShowTacticalCommandPrompt(const char *prompt);
void AdvanceLanceFormation(uint16_t unitIndex);
uint16_t SelectTacticalMoveDestination(
    uint16_t unitIndex, uint16_t selectionMode);


#endif
