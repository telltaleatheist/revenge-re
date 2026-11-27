// 3A6A: scene data
// SceneData.cpp
// Canonical C objects for the SCENE interpreter's original DS:3A6A state.

#include "revenge/original/scene_data.h"

SceneFileBuffer LoadedSceneFile;
SceneUnitRoute SceneUnitRoutes[SCENE_ROUTE_COUNT];
uint16_t SceneInstructionBaseOffset;
uint8_t *SceneInstructionCursor;
