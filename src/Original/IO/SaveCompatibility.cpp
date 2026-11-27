// 0D73: save compatibility
/* Preserved save-file compatibility loop from REVENGE_0D73. */

#include "IO/IO.h"
#include "Resources/Resources.h"
#include "UI/UI.h"

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
        nextRecordOffset = 0x0002;
        SeekTrackedFile(fileHandle, nextRecordOffset, 0x0000);
        ReadTrackedFile(fileHandle, &nextRecordOffset, 0x00000002UL);
        SeekTrackedFile(fileHandle, nextRecordOffset, 0x0000);
        ReadTrackedFile(fileHandle, &saveVersion, 0x00000002UL);
        if (saveVersion >= (int16_t) 0x0097)
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
