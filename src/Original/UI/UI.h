// 0D73, 3390, 35CA: UI declarations
#ifndef REVENGE_UI_UI_H
#define REVENGE_UI_UI_H

#include "revenge/types.h"

uint16_t ConfirmRestartGame(void);
void ShowInvalidSaveVersionDialog(void);
void ShowResourceErrorMessage(void);
void ShowFatalDiskErrorMessage(void);
void ToggleActiveUiRegionHighlight(void);

typedef void (*ResourceErrorCallback)(void);

extern ResourceErrorCallback ResourceErrorDisplayCallback;
extern ResourceErrorCallback ResourceErrorDismissCallback;

#endif
