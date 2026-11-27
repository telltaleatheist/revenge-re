#include "REVENGE.h"
#include "revenge/original_data.h"
#include "revenge/video.h"

#include <assert.h>

static word16 callOrder;
static word16 loadOrder;
static word16 copyOrder;
static const char *loadedPath;
static byte *loadedPackedBuffer;
static byte *copiedPackedBuffer;
static byte *copiedDestination;

void LoadPackedIconResource(
	const char *resourcePath,
	byte *packedDisplayBuffer)
{
	loadOrder = ++callOrder;
	loadedPath = resourcePath;
	loadedPackedBuffer = packedDisplayBuffer;
}

void CopyDecodedIconPage(
	const byte *packedDisplayBuffer,
	byte *iconPageDestination)
{
	copyOrder = ++callOrder;
	copiedPackedBuffer = (byte *)packedDisplayBuffer;
	copiedDestination = iconPageDestination;
}

static void ResetObservations(void)
{
	callOrder = 0x0000;
	loadOrder = 0x0000;
	copyOrder = 0x0000;
	loadedPath = 0;
	loadedPackedBuffer = 0;
	copiedPackedBuffer = 0;
	copiedDestination = 0;
}

int main(void)
{
	static const char path[] = "ICONS.PAK";
	byte packedBuffer[0x10];
	byte pageBuffer[0x10];

	ResetObservations();
	VideoAdapterType = VIDEO_ADAPTER_VGA;
	LoadPackedIconPageDirect(path, packedBuffer, pageBuffer);
	assert(loadOrder == 0x0001);
	assert(copyOrder == 0x0002);
	assert(callOrder == 0x0002);
	assert(loadedPath == path);
	assert(loadedPackedBuffer == packedBuffer);
	assert(copiedPackedBuffer == packedBuffer);
	assert(copiedDestination == pageBuffer);
	assert(VideoAdapterType == VIDEO_ADAPTER_VGA);
	return 0;
}
