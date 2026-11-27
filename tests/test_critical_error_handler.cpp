#include "fake_backend.h"
#include "IO/IO.h"
#include "revenge/original/io_data.h"

#include <assert.h>
#include <stdint.h>

int main(void)
{
    void *callback;

    FakeBackend_Reset();
    callback = (void *) (uintptr_t) 0x12345678U;
    InstallCriticalErrorHandler(callback);
    assert(CriticalErrorCallback.offset == 0x5678);
    assert(CriticalErrorCallback.segment == 0x1234);

    InstallDefaultCriticalErrorHandler();
    assert(CriticalErrorCallback.offset == 0x0006);
    assert(CriticalErrorCallback.segment == 0x34A7);
    return 0;
}
