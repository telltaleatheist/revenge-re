// 0D73: resource
/* Graphics-resource procedure from original segment 0D73. */

#include "REVENGE.h"

#include "revenge/original_data.h"
#include "revenge/video.h"

/* 0D73:4099-4125 */
void LoadGraphicsResourceToBuffers(
    word16 resourceIndex,
    byte *decodeBuffer,
    byte *displayBuffer)
{
    const char *resourceName = CachedFileDescriptors[resourceIndex].fileName;

    if (VideoAdapterType == VIDEO_ADAPTER_VGA)
    {
        LoadAndDecodePackedResource(
            resourceName, decodeBuffer, displayBuffer);
    }
    /* The other adapters' 4-bit packing (0D73:40CF-410B, 0800:0F74) and
	 * the CGA's conversion (0D73:410C-4123, 0800:35D8) went with them;
	 * the rebuild is VGA only. */
}
