#include "fake_backend.h"
#include "revenge/original_data.h"
#include "revenge/video.h"

#include <assert.h>
#include <string.h>

enum
{
    SPRITE_PAGE_BYTES = 0x10000,
    TILE_BYTES = 0x0080
};

static uint8_t spritePage[SPRITE_PAGE_BYTES];

void DrawTacticalViewportCell(
    int16_t viewportColumn,
    int16_t viewportRow,
    uint16_t viewportCellIndex);

static void ClearState(void)
{
    uint32_t index;

    FakeBackend_Reset();
    for (index = 0x0000; index < SPRITE_PAGE_BYTES; ++index)
        spritePage[index] = 0x00;
}

static void TestPackedTile(uint16_t adapterType)
{
    uint8_t *tile;

    ClearState();
    ActiveVideoAdapterType = adapterType;
    tile = spritePage + TILE_BYTES;
    tile[0x00] = 0xA3;
    tile[0x07] = 0x4F;
    tile[0x78] = 0xC5;

    DrawTacticalTileSprite(spritePage, 0x0001, 0x0002, 0x0003);

    assert(fakeBackend.indexedFramebuffer[0x0003 * 0x0140 + 0x0010] == 0x0A);
    assert(fakeBackend.indexedFramebuffer[0x0003 * 0x0140 + 0x0011] == 0x03);
    assert(fakeBackend.indexedFramebuffer[0x0003 * 0x0140 + 0x001E] == 0x04);
    assert(fakeBackend.indexedFramebuffer[0x0003 * 0x0140 + 0x001F] == 0x0F);
    assert(fakeBackend.indexedFramebuffer[0x0012 * 0x0140 + 0x0010] == 0x0C);
}

static void TestPackedComposition(uint16_t adapterType)
{
    uint8_t *foreground;
    uint8_t *background;

    ClearState();
    ActiveVideoAdapterType = adapterType;
    foreground = spritePage + TILE_BYTES;
    background = spritePage + TILE_BYTES * 0x02;
    foreground[0x00] = 0xA0;
    background[0x00] = 0x37;

    DrawCompositeTacticalTile(
        0x0001, 0x0002, spritePage, 0x0000, 0x0000);

    assert(fakeBackend.indexedFramebuffer[0x0000] == 0x0A);
    assert(fakeBackend.indexedFramebuffer[0x0001] == 0x07);
}

static void TestTerrainEffectOverlayFeedsViewportCell(void)
{
    static uint16_t terrainMap[0x00E4];
    static uint16_t spriteMap[0x00E4];
    uint8_t *effectTile;
    uint8_t *terrainTile;

    ClearState();
    ActiveVideoAdapterType = VIDEO_ADAPTER_VGA;
    PlanarScreenBuffer = spritePage;
    TacticalCellTerrainMap = terrainMap;
    TacticalCellSpriteMap = spriteMap;
    memset(terrainMap, 0x00, sizeof(terrainMap));
    memset(spriteMap, 0x00, sizeof(spriteMap));
    memset(TacticalEffectOverlayMap, 0x00,
        sizeof(TacticalEffectOverlayMap));

    terrainMap[0x00] = 0x0002;
    TacticalEffectOverlayMap[0x00] = 0x01;
    terrainTile = spritePage + 0x0002 * TILE_BYTES;
    effectTile = spritePage + 0x01F3 * TILE_BYTES;
    terrainTile[0x00] = 0x77;
    effectTile[0x00] = 0xA0;

    DrawTacticalViewportCell(0x00, 0x00, 0x00);

    assert(fakeBackend.indexedFramebuffer[0x0010] == 0x0A);
    assert(fakeBackend.indexedFramebuffer[0x0011] == 0x07);
}

int main(void)
{
    TestPackedTile(VIDEO_ADAPTER_VGA);
    TestPackedComposition(VIDEO_ADAPTER_VGA);
    TestTerrainEffectOverlayFeedsViewportCell();
    return 0;
}
