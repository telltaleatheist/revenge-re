#include "revenge/game_file.h"
#include "game_paths.h"

#include <SDL3/SDL.h>

enum
{
    FileHandleCapacity = 0x14,
    FirstGameFileHandle = 0x05,
    HostPathCapacity = 1024
};

static SDL_IOStream *files[FileHandleCapacity];
static GameFileAccess fileAccess[FileHandleCapacity];

static SDL_IOStream *GetFile(int16_t handle)
{
    if (handle < FirstGameFileHandle || handle >= FileHandleCapacity)
        return NULL;
    return files[handle];
}

static int16_t StoreFile(SDL_IOStream *file, GameFileAccess access)
{
    int16_t handle;

    if (file == NULL)
        return -0x01;
    for (handle = FirstGameFileHandle; handle < FileHandleCapacity; ++handle)
    {
        if (files[handle] == NULL)
        {
            files[handle] = file;
            fileAccess[handle] = access;
            return handle;
        }
    }
    SDL_CloseIO(file);
    return -0x01;
}

static int16_t OpenAt(const char *name, GamePathUse use, const char *mode,
    GameFileAccess access)
{
    char hostPath[HostPathCapacity];

    if (GamePaths_Resolve(name, use, hostPath, sizeof(hostPath)) != 0)
        return -0x01;
    return StoreFile(SDL_IOFromFile(hostPath, mode), access);
}

int16_t GameFile_Open(const char *name, GameFileAccess access)
{
    if (name == NULL)
        return -0x01;
    switch (access)
    {
    case GAME_FILE_READ:
        return OpenAt(name, GAME_PATH_READ, "rb", access);
    case GAME_FILE_WRITE:
    case GAME_FILE_READ_WRITE:
        return OpenAt(name, GAME_PATH_WRITE, "r+b", access);
    case GAME_FILE_CREATE:
    {
        /* 3915:0037 with 8102h: a missing file is made, an existing one
		 * opened for writing as it is. */
        char hostPath[HostPathCapacity];

        if (GamePaths_Resolve(name, GAME_PATH_READ, hostPath, sizeof(hostPath)) == 0)
            return OpenAt(name, GAME_PATH_WRITE, "r+b", access);
        return OpenAt(name, GAME_PATH_CREATE, "w+b", access);
    }
    }
    return -0x01;
}

int16_t GameFile_Close(int16_t handle)
{
    SDL_IOStream *file = GetFile(handle);

    if (file == NULL)
        return -0x01;
    files[handle] = NULL;
    return SDL_CloseIO(file) ? 0x0000 : -0x01;
}

int16_t GameFile_Read(int16_t handle, void *destination, uint16_t byteCount)
{
    SDL_IOStream *file = GetFile(handle);
    size_t bytesRead;

    if (file == NULL || fileAccess[handle] == GAME_FILE_WRITE ||
        fileAccess[handle] == GAME_FILE_CREATE)
        return -0x01;
    if (byteCount == 0x0000)
        return 0x0000;
    bytesRead = SDL_ReadIO(file, destination, byteCount);
    if (SDL_GetIOStatus(file) == SDL_IO_STATUS_ERROR)
        return -0x01;
    return (int16_t) (uint16_t) bytesRead;
}

int16_t GameFile_Write(int16_t handle, const void *source, uint16_t byteCount)
{
    SDL_IOStream *file = GetFile(handle);
    size_t bytesWritten;

    if (file == NULL || fileAccess[handle] == GAME_FILE_READ)
        return -0x01;
    if (byteCount == 0x0000)
        return 0x0000;
    bytesWritten = SDL_WriteIO(file, source, byteCount);
    if (SDL_GetIOStatus(file) == SDL_IO_STATUS_ERROR)
        return -0x01;
    return (int16_t) (uint16_t) bytesWritten;
}

int32_t GameFile_Seek(int16_t handle, int32_t offset, GameFileOrigin origin)
{
    SDL_IOStream *file = GetFile(handle);
    SDL_IOWhence whence;
    Sint64 position;

    if (file == NULL)
        return -0x01;
    switch (origin)
    {
    case GAME_FILE_FROM_START: whence = SDL_IO_SEEK_SET; break;
    case GAME_FILE_FROM_CURRENT: whence = SDL_IO_SEEK_CUR; break;
    case GAME_FILE_FROM_END: whence = SDL_IO_SEEK_END; break;
    default: return -0x01;
    }
    position = SDL_SeekIO(file, (Sint64) offset, whence);
    if (position < 0 || position > INT32_MAX)
        return -0x01;
    return (int32_t) position;
}

int32_t GameFile_Length(int16_t handle)
{
    int32_t position = GameFile_Seek(handle, 0x00000000L, GAME_FILE_FROM_CURRENT);
    int32_t length;

    if (position < 0x00000000L)
        return -0x01;
    length = GameFile_Seek(handle, 0x00000000L, GAME_FILE_FROM_END);
    if (length < 0x00000000L ||
        GameFile_Seek(handle, position, GAME_FILE_FROM_START) < 0x00000000L)
        return -0x01;
    return length;
}

int16_t GameFile_Delete(const char *name)
{
    if (name == NULL || GamePaths_Delete(name) != 0)
        return -0x01;
    return 0x0000;
}
