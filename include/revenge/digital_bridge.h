#ifndef REVENGE_DIGITAL_BRIDGE_H
#define REVENGE_DIGITAL_BRIDGE_H

#include "revenge/types.h"

struct DigitalPlaybackDescriptor;

/* The two sampled-output descriptors the rebuild keeps (NULL for the
 * original's others, whose cards it has no driver for). */
struct DigitalPlaybackDescriptor *
DigitalBridge_ResolvePlaybackDescriptor(uint16_t descriptorOffset);

#endif
