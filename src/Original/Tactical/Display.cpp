// 2007: display
/* Tactical sprite composition and display procedures from segment 2007. */

#include "Data/StartupData.h"
#include "Data/TacticalControlData.h"
#include "Data/TacticalMapData.h"
#include "Graphics/Graphics.h"
#include "Input/Input.h"
#include "Tactical/Tactical.h"

#include "revenge/video.h"

void AdvanceTerrainEffects(uint16_t advanceEffects);
void RebuildTacticalViewport(void);
void DrawTacticalCursorOutline(uint16_t outlineState);
void RefreshTacticalOverview(void);
void DrawSelectedUnitStatusPanel(void);

/* 2007:0DDB-0E18 */
void DrawTacticalEffectSprite(
    uint16_t spriteId,
    int16_t relativeX,
    int16_t relativeY)
{
    uint8_t *renderTarget;

    if (TacticalOverviewActive != FALSE)
        return;
    renderTarget = VideoFramebuffer;
    HideMouseCursorOverlay();
    DrawTacticalClippedSprite(
        renderTarget,
        TacticalEffectSpriteTable[spriteId],
        relativeX,
        relativeY,
        0x0002);
    ShowMouseCursorOverlay();
}

/* 2007:0328-0429 */
void DrawTacticalViewportCell(
    int16_t viewportColumn,
    int16_t viewportRow,
    uint16_t viewportCellIndex)
{
    const uint8_t *spritePage;
    uint16_t destinationX;
    uint16_t destinationY;
    uint16_t terrainSpriteId;
    uint16_t unitSpriteId;
    uint8_t terrainEffect;

    unitSpriteId = TacticalCellSpriteMap[viewportCellIndex];
    terrainSpriteId = TacticalCellTerrainMap[viewportCellIndex];
    /* DS:7978 is the same E4h-byte array cleared at 0800:0999 and populated
	 * at 2007:1219.  It is not a second display-side buffer. */
    terrainEffect = TacticalEffectOverlayMap[viewportCellIndex];
    destinationX = (uint16_t) (viewportColumn * 0x0002 + 0x0002);
    destinationY = (uint16_t) (viewportRow << 0x04);
    spritePage = PlanarScreenBuffer;

    if (unitSpriteId != 0x0000)
    {
        DrawCompositeTacticalTile(
            unitSpriteId,
            terrainEffect != 0x00 ? (uint16_t) terrainEffect + 0x01F2 : terrainSpriteId,
            spritePage,
            destinationX,
            destinationY);
    }
    else if (terrainEffect != 0x00)
    {
        DrawCompositeTacticalTile(
            (uint16_t) terrainEffect + 0x01F2,
            terrainSpriteId,
            spritePage,
            destinationX,
            destinationY);
    }
    else
    {
        DrawTacticalTileSprite(
            spritePage,
            terrainSpriteId,
            destinationX,
            destinationY);
    }
}

/* 2007:1CBD-1D05 */
void DrawTacticalTileSpriteForAdapter(
    const uint8_t *spritePage,
    uint16_t spriteId,
    uint16_t destinationX,
    uint16_t destinationY)
{
    /* The CGA's high sprite bank (2007:1CC4, 0800:0A2E) went with the CGA; the
	 * rebuild is VGA only. */
    DrawTacticalTileSprite(
        spritePage, spriteId, destinationX, destinationY);
}

/* 2007:1D06-1D60 */
void RefreshTacticalDisplay(uint16_t advanceEffects)
{
    if (TacticalOverviewActive != FALSE)
    {
        AdvanceTerrainEffects(advanceEffects);
        if (TacticalOverviewViewportDirty != FALSE)
        {
            RebuildTacticalViewport();
            DrawTacticalCursorOutline(TacticalCursorOutlineState);
        }
        else
        {
            RefreshTacticalOverview();
        }
    }
    else
    {
        AdvanceTerrainEffects(advanceEffects);
        /* The original draws every projectile and impact directly into
		 * visible A000h memory before rebuilding the viewport.  Publish the
		 * completed effect frame once here; presenting in the individual
		 * sprite primitive exposes only part of a multi-effect frame. */
        Video_PresentTransientFrame();
        HideMouseCursorOverlay();
        RebuildTacticalViewport();
        if (TacticalCursorOutlineEnabled != FALSE)
            DrawTacticalCursorOutline(TacticalCursorOutlineState);
        DrawSelectedUnitStatusPanel();
        ShowMouseCursorOverlay();
    }
}
