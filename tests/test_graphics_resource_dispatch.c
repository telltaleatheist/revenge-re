#include "REVENGE.h"

#include "revenge/original_data.h"
#include "revenge/video.h"

#include <assert.h>

static char resourceName[] = "TEST.CPS";
static byte decodeBuffer[0x10];
static byte displayBuffer[0x10];
static const char *capturedName;
static void *capturedCompressed;
static void *capturedDestination;

void LoadAndDecodePackedResource(
	const char *name,
	void *compressedBuffer,
	void *destination)
{
	capturedName = name;
	capturedCompressed = compressedBuffer;
	capturedDestination = destination;
}

static void ResetCapture(void)
{
	capturedName = NULL;
	capturedCompressed = NULL;
	capturedDestination = NULL;
}

int main(void)
{
	HostFarPointer_Reset();
	CachedFileDescriptors[0x03].fileName =
		DosAddress_FromValue(
			HostFarPointer_Register(resourceName, sizeof(resourceName)));

	ResetCapture();
	VideoAdapterType = VIDEO_ADAPTER_VGA;
	LoadGraphicsResourceToBuffers(
		0x0003, decodeBuffer, displayBuffer);
	assert(capturedName == resourceName);
	assert(capturedCompressed == decodeBuffer);
	assert(capturedDestination == displayBuffer);

	HostFarPointer_Reset();
	return 0;
}
