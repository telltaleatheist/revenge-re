// 3A6A: graphics data
// GraphicsData.cpp
// Typed objects recovered from REVENGE.EXE DS segment 3A6A.

#include "Data/GraphicsData.h"
#include "revenge/audio.h"
#include "revenge/video.h"

/* The story's media video_palette, placed at start (original_import_pieces.txt). */
uint8_t VideoPaletteData[0x0310];
uint8_t *ActiveRenderTarget;
uint16_t SpriteClipLeftTileMirror;
uint16_t SpriteClipRightTileMirror;
uint16_t SpriteClipTopPixelMirror;
uint16_t SpriteClipBottomPixelMirror;
uint16_t TacticalSpriteClipLeftTile;
uint16_t TacticalSpriteClipRightTile;
uint16_t TacticalSpriteClipTopPixel;
uint16_t TacticalSpriteClipBottomPixel;
uint16_t SpriteDrawBusy;
uint16_t PlanarBackingBufferExtentLow;
uint16_t PlanarBackingBufferExtentHigh;
