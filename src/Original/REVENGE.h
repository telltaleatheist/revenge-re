#ifndef REVENGE_ORIGINAL_H
#define REVENGE_ORIGINAL_H

/*
 * Everything once declared here is declared in the header beside the code
 * that defines it (docs/SOURCE_LAYOUT.md). This header includes them all
 * until each file includes the ones it uses; then it goes.
 */

#include "revenge/dos.h"
#include "revenge/heap.h"
#include "revenge/types.h"
#include "revenge/unit.h"
#include <stddef.h>

#include "Audio/Audio.h"
#include "Campaign/Campaign.h"
#include "Data/AudioData.h"
#include "Graphics/Graphics.h"
#include "IO/IO.h"
#include "Input/Input.h"
#include "Resources/Resources.h"
#include "Runtime/Runtime.h"
#include "Tactical/Tactical.h"
#include "Text/Text.h"
#include "UI/UI.h"
#include "Units/Units.h"
#include "Utility/Utility.h"
#include "revenge/presentation_callback_bridge.h"

#endif
