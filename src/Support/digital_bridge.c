#include "../Original/REVENGE.h"
#include "revenge/audio.h"
#include "revenge/digital_bridge.h"

struct DigitalPlaybackDescriptor *
DigitalBridge_ResolvePlaybackDescriptor(word16 descriptorOffset)
{
	switch (descriptorOffset)
	{
	case 0x6EB2: return &NullDigitalPlayback;
	case 0x6EE2: return &SoundBlasterPlayback;
	default: return NULL;
	}
}
