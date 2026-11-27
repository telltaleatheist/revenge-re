// 356D: text window clear
// Reconstructed original active text-window clear procedure (segment 356D).

#include "Data/TextIOData.h"
#include "Text/Text.h"
#include "revenge/video.h"

// 356D:0006: void ClearActiveTextWindow(void)
// Called from:
//      OpenModalTextWindow
//      ShowInvalidSaveVersionDialog
//      ShowSaveLoadMenu
//      RunGameOptionsMenu
//      SelectAndClearTextWindow
void ClearActiveTextWindow(void)
{
    FillVideoTileRectangle(
        ActiveTextWindowLeftTile,
        ActiveTextWindowTopTile,
        ActiveTextWindowWidthTiles,
        ActiveTextWindowHeightTiles,
        (uint8_t) ActiveTextBackgroundColor);
    ActiveTextCursorRow = 0x00;
    ActiveTextCursorColumn = 0x00;
}
