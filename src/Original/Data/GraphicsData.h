// 3A6A: graphics data declarations
#ifndef REVENGE_DATA_GRAPHICS_DATA_H
#define REVENGE_DATA_GRAPHICS_DATA_H

#include "revenge/types.h"

extern uint8_t VideoPaletteData[0x0310];

extern uint8_t *ActiveRenderTarget;
extern uint16_t SpriteClipLeftTileMirror;
extern uint16_t SpriteClipRightTileMirror;
extern uint16_t SpriteClipTopPixelMirror;
extern uint16_t SpriteClipBottomPixelMirror;
extern uint16_t TacticalSpriteClipLeftTile;
extern uint16_t TacticalSpriteClipRightTile;
extern uint16_t TacticalSpriteClipTopPixel;
extern uint16_t TacticalSpriteClipBottomPixel;
extern uint16_t SpriteDrawBusy;
extern uint16_t PlanarBackingBufferExtentLow;
extern uint16_t PlanarBackingBufferExtentHigh;

#endif
