#include "REVENGE.h"

#include "revenge/original_data.h"
#include "revenge/video.h"

#include <assert.h>
#include <string.h>

static byte framebuffer[VIDEO_SCREEN_WIDTH * VIDEO_SCREEN_HEIGHT];
static byte encodingBuffer[0x0400];
static byte allocation[0x0400];
static int32 requestedAllocationSize;

byte *AllocateFarMemory(int32 byteCount)
{
	requestedAllocationSize = byteCount;
	memset(allocation, 0xCC, sizeof(allocation));
	return allocation;
}

static void ResetState(void)
{
	memset(framebuffer, 0x00, sizeof(framebuffer));
	memset(encodingBuffer, 0xCC, sizeof(encodingBuffer));
	requestedAllocationSize = -0x01;
	PrimaryScreenBuffer = framebuffer;
	PlanarScreenBuffer = encodingBuffer;
}

static void TestVgaZeroRunEncoding(void)
{
	static const byte expected[] = {
		0x00, 0x01, 0x01, 0x01,
		0x01, 0x00, 0x03, 0x02, 0x00, 0x01, 0x03, 0x04,
		0x00, 0x08
	};
	byte *firstRow;
	byte *result;

	ResetState();
	firstRow = framebuffer + 0x0002 * VIDEO_SCREEN_WIDTH + 0x0008;
	firstRow[0x00] = 0x01;
	firstRow[0x04] = 0x02;
	firstRow[0x06] = 0x03;
	firstRow[0x07] = 0x04;
	result = BuildVgaDisplayBackingBuffer(
		0x0001, 0x0002, 0x0001, 0x0002);

	assert(result == allocation);
	assert(requestedAllocationSize == (int32) sizeof(expected));
	assert(memcmp(allocation, expected, sizeof(expected)) == 0x00);
}

static void TestAdapterDispatcher(void)
{
	ResetState();
	VideoAdapterType = VIDEO_ADAPTER_TANDY;
	ActiveVideoAdapterType = VIDEO_ADAPTER_TANDY;
	assert(BuildDisplayBackingBuffer(
		0x0001, 0x0002, 0x0001, 0x0001) == allocation);
	assert(requestedAllocationSize == 0x00000008L);

	VideoAdapterType = VIDEO_ADAPTER_VGA;
	assert(BuildDisplayBackingBuffer(
		0x0000, 0x0000, 0x0000, 0x0000) == allocation);
	assert(requestedAllocationSize == 0x00000004L);
}

static void TestPlanarBackingFormat(void)
{
	static const byte expected[] = {
		0xCC, 0x00, 0x01, 0x00, 0x01, 0x02, 0x03, 0x04
	};

	ResetState();
	framebuffer[0x00] = 0x01;
	framebuffer[0x01] = 0x02;
	framebuffer[0x02] = 0x03;
	framebuffer[0x03] = 0x04;
	VideoAdapterType = VIDEO_ADAPTER_VGA;
	ActiveVideoAdapterType = VIDEO_ADAPTER_VGA;
	BuildPlanarDisplayBackingBuffer(0x0000, 0x0000, 0x0001, 0x0001);
	assert(memcmp(allocation, expected, sizeof(expected)) == 0x00);
}

int main(void)
{
	TestVgaZeroRunEncoding();
	TestAdapterDispatcher();
	TestPlanarBackingFormat();
	return 0;
}
