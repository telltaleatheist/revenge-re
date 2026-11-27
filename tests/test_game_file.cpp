/* The game's files (revenge/game_file.h), in the save folder the test names:
 * a written file reads back; a seek moves and the length leaves the position;
 * a read-only handle refuses a write and a write-only one a read; 0 bytes
 * move nothing; the create mode makes a missing file empty and leaves an
 * existing one as it is; handles count from 5 and at most 15 are open; a
 * closed or unknown handle, an absent file and a second delete are -1.
 */
#include "revenge/game_file.h"

#include <stdio.h>
#include <string.h>

static int failures;

static void Check(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL: %s\n", message);
        ++failures;
    }
}

int main(void)
{
    static const char name[] = "GAMEFILE.TMP";
    char buffer[0x10];
    int16 handles[0x10];
    int16 handle;
    int count;

    (void) GameFile_Delete(name);
    Check(GameFile_Open(name, GAME_FILE_READ) == -0x01, "an absent file doesn't open");

    handle = GameFile_Open(name, GAME_FILE_CREATE);
    Check(handle == 0x0005, "the first handle is 5, as DOS gave it");
    Check(GameFile_Write(handle, "LANTERN!", 0x0008) == 0x0008, "a write moves its bytes");
    Check(GameFile_Write(handle, "x", 0x0000) == 0x0000, "a write of 0 bytes moves none");
    Check(GameFile_Read(handle, buffer, 0x0001) == -0x01, "a write-only handle refuses a read");
    Check(GameFile_Close(handle) == 0x0000, "a close");
    Check(GameFile_Close(handle) == -0x01, "a second close");

    handle = GameFile_Open(name, GAME_FILE_CREATE);
    Check(GameFile_Length(handle) == 0x0008, "the create mode keeps an existing file");
    Check(GameFile_Close(handle) == 0x0000, "a close");

    handle = GameFile_Open(name, GAME_FILE_READ);
    Check(GameFile_Seek(handle, 0x0004L, GAME_FILE_FROM_START) == 0x0004L, "a seek from the start");
    Check(GameFile_Length(handle) == 0x0008L, "the length");
    Check(GameFile_Seek(handle, 0x0000L, GAME_FILE_FROM_CURRENT) == 0x0004L,
        "the length leaves the position");
    memset(buffer, 0, sizeof(buffer));
    Check(GameFile_Read(handle, buffer, (word16) sizeof(buffer)) == 0x0004 &&
            memcmp(buffer, "ERN!", 0x0004) == 0,
        "a read stops at the end");
    Check(GameFile_Read(handle, buffer, 0x0000) == 0x0000, "a read of 0 bytes");
    Check(GameFile_Seek(handle, -0x0002L, GAME_FILE_FROM_END) == 0x0006L, "a seek from the end");
    Check(GameFile_Write(handle, "x", 0x0001) == -0x01, "a read-only handle refuses a write");
    Check(GameFile_Close(handle) == 0x0000, "a close");

    handle = GameFile_Open(name, GAME_FILE_READ_WRITE);
    Check(GameFile_Write(handle, "l", 0x0001) == 0x0001, "a read-write handle writes");
    Check(GameFile_Seek(handle, 0x0000L, GAME_FILE_FROM_START) == 0x0000L &&
            GameFile_Read(handle, buffer, 0x0002) == 0x0002 &&
            memcmp(buffer, "lA", 0x0002) == 0,
        "and reads what it wrote");
    Check(GameFile_Close(handle) == 0x0000, "a close");

    for (count = 0; count < 0x10; ++count)
        handles[count] = GameFile_Open(name, GAME_FILE_READ);
    Check(handles[0x0E] == 0x0013 && handles[0x0F] == -0x01, "15 handles, 5 to 19");
    for (count = 0; count < 0x0F; ++count)
        (void) GameFile_Close(handles[count]);
    Check(GameFile_Read(0x0004, buffer, 0x0001) == -0x01, "handle 4 is no game file");

    Check(GameFile_Delete(name) == 0x0000, "a delete");
    Check(GameFile_Delete(name) == -0x01, "a second delete");
    return failures != 0;
}
