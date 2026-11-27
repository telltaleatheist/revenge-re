// 3A6A: resource data declarations
#ifndef REVENGE_DATA_RESOURCE_DATA_H
#define REVENGE_DATA_RESOURCE_DATA_H

#include "revenge/dos.h"
#include "revenge/original/resource_data.h"
#include "revenge/types.h"

typedef void (*OriginalExitHandler)(void);

/* DS:1530 and DS:1566 are cached-file descriptors 40h's and 43h's name
 * pointers, which the code reads as the unit template and tactical resource
 * paths: not second objects. */
#define UnitTemplatePath     (CachedFileDescriptors[0x40].fileName)
#define TacticalResourcePath (CachedFileDescriptors[0x43].fileName)

extern uint16_t ResourceRetryCount;
extern uint16_t ReservedConventionalMemoryKB;
extern uint16_t ExitHandlerCount;
extern OriginalExitHandler ExitHandlerTable[0x20];
extern DOS_InterruptHandler SavedDivideErrorHandler;
extern DOS_InterruptHandler SavedOverflowHandler;
extern DOS_InterruptHandler SavedBoundsHandler;
extern DOS_InterruptHandler SavedInvalidOpcodeHandler;
extern const char *const ResourceCreateFileErrorText;
extern const char *const ResourceIllegalOpenModeErrorText;
extern const char *const ResourceInsertDiskErrorText;
extern const char *const ResourceTooManyOpenFilesErrorText;
extern const char *const ResourceCloseUnopenedErrorText;
extern const char *const ResourceReadUnopenedErrorText;
extern const char *const ResourceWriteUnopenedErrorText;
extern const char *const ResourceSeekUnopenedErrorText;
extern const char *const ResourceIllegalSeekOriginErrorText;
extern const char *const ResourceWriteCachedErrorText;
extern const char *const ResourceUnknownDriveErrorText;
extern const char *const ResourceUnknownCacheIndexErrorText;
extern const char *const ResourceOpenFilesAtExitErrorText;
extern const char *const ResourceFatalDiskErrorText;

#endif
