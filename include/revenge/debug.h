#ifndef REVENGE_DEBUG_H
#define REVENGE_DEBUG_H

#include <stddef.h>

void RevengeTrace(const char *format, ...);
void RevengeInstallCrashDiagnostics(void);

/* Something the port's own rules say can't happen, or a game file the play
 * can't go on without: prints "fault: " and the message, and stops the
 * program (exit 70). */
_Noreturn void RevengeFault(const char *format, ...);

/* A table the port sizes as the original's space: an index the game's data
 * and code can't produce means the port is wrong, so it stops the program
 * with a fault naming the table, the index and the table's count (exit 70)
 * rather than reading or writing past the table. */
size_t RevengeCheckIndex(const char *object, size_t index, size_t count);
#define REVENGE_INDEX(array, index) \
	RevengeCheckIndex(#array, (size_t) (index), sizeof(array) / sizeof((array)[0]))

/* Diagnostics injected while reconstructing a procedure are not part of the
 * original executable.  Keep them visibly distinct and absent from normal
 * preservation builds. */
#ifdef REVENGE_ENABLE_PRESERVATION_INSTRUMENTATION
#define REVENGE_PRESERVATION_TRACE(...) RevengeTrace(__VA_ARGS__)
#else
#define REVENGE_PRESERVATION_TRACE(...) ((void) 0)
#endif

#endif
