// 2C7F: Runtime declarations
#ifndef REVENGE_RUNTIME_RUNTIME_H
#define REVENGE_RUNTIME_RUNTIME_H

#include "revenge/types.h"

uint16_t GetCurrentBIOSVideoMode(void);
void SetVideoModeForAdapter(uint16_t adapterOrRestoreSlot);
int16_t ReadTranslatedKeypress(void);

void InitializeGame(void);
void ShutdownGame(void);

typedef uint16_t (*MediaConfigurationHandler)(void);

#endif
