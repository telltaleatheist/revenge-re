// 0D73: save compatibility
/* Preserved save-file compatibility loop from REVENGE_0D73. */

#include "IO/IO.h"
#include "Resources/Resources.h"
#include "UI/UI.h"

/* The save directory's word for slot 0, the save index, which begins with
 * the file's version; the oldest version the game reads. */
constexpr int16_t SAVE_INDEX_DIRECTORY_OFFSET = 0x0002;
constexpr int16_t MINIMUM_SAVE_VERSION = 0x0097;

/* 0D73:4572-4610 */
int16_t EnsureCompatibleSaveFile(
    int16_t fileHandle,
    char *savePath,
    uint16_t openMode)
{
    int16_t nextRecordOffset;
    int16_t saveVersion;
    uint16_t searching;

    searching = TRUE;
    while (searching != FALSE)
    {
        nextRecordOffset = SAVE_INDEX_DIRECTORY_OFFSET;
        SeekTrackedFile(fileHandle, nextRecordOffset, 0x0000);
        ReadTrackedFile(fileHandle, &nextRecordOffset, sizeof nextRecordOffset);
        SeekTrackedFile(fileHandle, nextRecordOffset, 0x0000);
        ReadTrackedFile(fileHandle, &saveVersion, sizeof saveVersion);
        if (saveVersion >= MINIMUM_SAVE_VERSION)
        {
            searching = FALSE;
        }
        else
        {
            CloseTrackedFile(fileHandle);
            ShowInvalidSaveVersionDialog();
            fileHandle = OpenTrackedFile(savePath, openMode);
        }
    }
    return fileHandle;
}
