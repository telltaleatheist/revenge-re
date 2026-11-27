#ifndef REVENGE_SDL_MAP_DUMP_H
#define REVENGE_SDL_MAP_DUMP_H

/* When REVENGE_MAP_DUMP_DIR names a folder, writes every data object the
 * headers declare to SNAP.objects there, one line each: its name, its size
 * and its bytes in hex (tools/map/README.md). Otherwise does nothing. */
void MapDump_Write(const char *snap);

#include <stdio.h>

/* Every data object by name, one line each: its name and the first 16 hex
 * digits of the SHA-256 of its bytes, a pointer in them written as what it
 * points into: the object's name and the offset there, or the DOS memory
 * block's linear address (map_dump.cpp). The same on every run and system,
 * wherever the host put the objects. */
void MapDump_WriteNames(FILE *file);

#endif
