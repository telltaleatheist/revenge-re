#ifndef REVENGE_RULES_H
#define REVENGE_RULES_H

/* The rules, the layout and the starting state: numbers of our own, the
 * original's to begin with, read from the .json files of <data
 * directory>/rules, /ui and /start at start (src/Rules/Rules.h has the
 * files' shapes) into the tables the game reads, each table's bytes laid out
 * as the original's. The rule table (src/Rules/rule_table.cpp, written by
 * tools/import/make_import_table.py from src/Rules/rule_pieces.txt) says
 * where each table is in the original's memory and the keyed hash of its
 * bytes there, so the tests can show that data/rules, data/ui and data/start
 * rebuild each byte for byte (rules.rebuild) and that a player's copy holds
 * them (rules.exe). The game never reads them from the copy. */

#include <stddef.h>
#include <stdint.h>


typedef struct RulePiece
{
    const char *id;           /* file.table, as weapons.definitions */
    void *object;             /* the table the rules fill */
    size_t size;              /* its size, the original's */
    uint16_t segment;         /* its place in the original's memory, */
    uint16_t offset;          /* the image loaded at 0800h */
    uint16_t length;
    const char *keyedHash;    /* 16 hex digits; empty for a tuned table */
} RulePiece;

extern const RulePiece RulePieces[];
extern const size_t RulePieceCount;

/* Reads every file under dataDirectory/rules, /ui and /start into its
 * tables. A file that is missing or can't be read, a key that is missing,
 * unknown or twice, a value of the wrong kind or out of its range, or an
 * array of the wrong length stops the program with a fault naming the file
 * and the key. */
void Rules_Load(const char *dataDirectory);


#endif
