#ifndef REVENGE_ORIGINAL_IO_DATA_H
#define REVENGE_ORIGINAL_IO_DATA_H

#include <stddef.h>

#include "revenge/types.h"

extern word16 SecondaryResourceDriveEnabled; /* DS:B662 */
extern word16 ResourceDriveSelectionDisabled; /* DS:B668 */
extern DosAddress CriticalErrorCallback; /* DS:B888 */

#endif
