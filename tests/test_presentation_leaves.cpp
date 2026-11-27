#include "Data/GraphicsData.h"
#include "Data/RuntimeData.h"
#include "Data/TacticalData.h"
#include "Graphics/Graphics.h"
#include "Input/Input.h"

#include "fake_backend.h"
#include "revenge/video.h"

#include <assert.h>
#include <string.h>

static void TestVgaPaletteBankAnchors(void)
{
    uint8_t baseColors[0x30];
    uint16_t index;

    FakeBackend_Reset();
    for (index = 0x0000; index < 0x0030; ++index)
    {
        baseColors[index] = (uint8_t) (index + 0x01);
        VideoPaletteData[index] = baseColors[index];
    }
    memset(VideoPaletteData + 0x0030, 0x00, 0x02D0);

    VideoAdapterType = VIDEO_ADAPTER_TANDY;
    InstallVgaPaletteBankAnchors();
    assert(fakeBackend.vgaPalette[0x0000] == 0x00);

    VideoAdapterType = VIDEO_ADAPTER_VGA;
    InstallVgaPaletteBankAnchors();
    for (index = 0x0000; index < 0x0010; ++index)
    {
        uint16_t sourceOffset = index * 0x0003;
        uint16_t destinationOffset = index * 0x0030;

        assert(memcmp(
                   VideoPaletteData + destinationOffset,
                   baseColors + sourceOffset,
                   0x0003) == 0x00);
    }
    assert(memcmp(fakeBackend.vgaPalette, VideoPaletteData,
               0x0300) == 0x00);
}

static void TestFlushAndZeroDurationWait(void)
{
    FakeBackend_Reset();
    PendingMouseButtonEvent = TRUE;
    assert(FlushInputAndWait(0x0001) == 0x0000);
    assert(PendingMouseButtonEvent == FALSE);
}

int main(void)
{
    TestVgaPaletteBankAnchors();
    TestFlushAndZeroDurationWait();
    return 0;
}
