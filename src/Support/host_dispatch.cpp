#include "revenge/host_dispatch.h"

const void *HostDispatch_Select(
    const void *identity,
    const void *const *identities,
    const void *const *dispatches,
    uint16_t count,
    const void *fallback)
{
    uint16_t index;

    for (index = 0x0000; index < count; ++index)
    {
        if (identity == identities[index])
            return dispatches[index];
    }
    return fallback;
}
