#ifndef REVENGE_SDL_MAP_DUMP_H
#define REVENGE_SDL_MAP_DUMP_H

/* When REVENGE_MAP_DUMP_DIR names a folder, writes every data object the
 * headers declare to SNAP.objects there, one line each: its name, its size
 * and its bytes in hex (tools/map/README.md). Otherwise does nothing. */
void MapDump_Write(const char *snap);

#endif
