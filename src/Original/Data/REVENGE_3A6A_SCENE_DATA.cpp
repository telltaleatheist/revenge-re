// REVENGE_3A6A_SCENE_DATA.cpp
// Canonical C objects for the SCENE interpreter's original DS:3A6A state.

#include "revenge/original/scene_data.h"

SceneFileBuffer LoadedSceneFile;
SceneUnitRoute SceneUnitRoutes[SCENE_ROUTE_COUNT];
word16 SceneInstructionBaseOffset;
byte *SceneInstructionCursor;
