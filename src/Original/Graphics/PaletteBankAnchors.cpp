// 0D73: VGA palette bank anchors
/* Palette procedure from original segment 0D73. */

#include "Data/GraphicsData.h"
#include "Data/RuntimeData.h"
#include "Graphics/Graphics.h"

#include "revenge/video.h"

// 0D73:329E: void InstallVgaPaletteBankAnchors(void)
// Called from:
//      InitializeTacticalBattle
void InstallVgaPaletteBankAnchors(void)
{
    // [V] Copy the first 16 RGB triplets into palette entries
    // 00h,10h,20h,...,F0h, then install the resulting VGA DAC palette.
    if (VideoAdapterType != VIDEO_ADAPTER_VGA)
        return;

    uint8_t *palette = VideoPaletteData;
    uint16_t baseColor;
    for (baseColor = 0x00; baseColor < EGA_LOGICAL_PALETTE_ENTRY_COUNT; ++baseColor)
    {
        uint16_t source = baseColor * 0x03;
        uint16_t destination = baseColor * 0x30;
        palette[destination] = palette[source];
        palette[destination + 0x01] = palette[source + 0x01];
        palette[destination + 0x02] = palette[source + 0x02];
    }
    LoadVideoPalette(palette);
}
