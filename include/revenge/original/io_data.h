#ifndef REVENGE_ORIGINAL_IO_DATA_H
#define REVENGE_ORIGINAL_IO_DATA_H

#include <stddef.h>

#include "revenge/types.h"

extern uint16_t SecondaryResourceDriveEnabled; /* DS:B662 */
extern uint16_t ResourceDriveSelectionDisabled; /* DS:B668 */
extern DosAddress CriticalErrorCallback; /* DS:B888 */

#endif
