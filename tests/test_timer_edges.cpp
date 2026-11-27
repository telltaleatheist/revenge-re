#include "fake_backend.h"
#include "Audio/Audio.h"
#include "Data/RuntimeData.h"
#include "Data/TextIOData.h"
#include "Graphics/Graphics.h"
#include "revenge/platform_backend.h"
#include "revenge/video.h"

#include <assert.h>

static void TestPITDivisorExchange(void)
{
    uint16_t oldDivisor;

    PITCalibration.Channel0Divisor = 0x1234;
    oldDivisor = SetPITChannel0Divisor(0x5678);
    assert(oldDivisor == 0x1234);
    assert(PITCalibration.Channel0Divisor == 0x5678);
}

static void TestPITMath(void)
{
    assert(PITFrequencyToDivisor(0x0012) == 0x0000);
    assert(PITFrequencyToDivisor(0x0013) ==
        (uint16_t) (0x001234DEUL / 0x0013));
    assert(PITFrequencyToDivisor(0x4440) ==
        (uint16_t) (0x001234DEUL / 0x4440));
}

static void TestVideoTimingCalibration(void)
{
    /* Only the edge is left: always the retrace's end; DS:B650 untouched. */
    VerticalRetraceEdgeMode = VERTICAL_RETRACE_WAIT_FOR_START;
    BusyWaitLoopCalibration = 0x0000;

    CalibrateVideoTiming();

    assert(BusyWaitLoopCalibration == 0x0000);
    assert(VerticalRetraceEdgeMode ==
        VERTICAL_RETRACE_WAIT_FOR_END);
}

int main(void)
{
    FakeBackend_Reset();
    TestPITDivisorExchange();
    TestPITMath();
    TestVideoTimingCalibration();
    return 0;
}
