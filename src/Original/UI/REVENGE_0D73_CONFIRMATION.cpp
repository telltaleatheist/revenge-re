/* Restart-confirmation procedure from original segment 0D73. */

#include "REVENGE.h"

#include "revenge/original_data.h"

/* 0D73:242E-2447 */
word16 ConfirmRestartGame(void)
{
    /* DS:0822: the confirmation's region (7) starts on its second row. */
    UiRegionTable[0x07].selectedRow = 0x0001;
    return ShowConfirmationDialog(RestartConfirmationText) == 0x0000;
}
