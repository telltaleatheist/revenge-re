#ifndef REVENGE_ORIGINAL_IMPORT_H
#define REVENGE_ORIGINAL_IMPORT_H

/*
 * The game's own text and data come from the player's REVENGE.EXE, never from
 * this source tree. OriginalImport_Load reads it once at start and accepts it
 * only as an edition the rebuild knows (src/Support/original_editions.txt):
 * first by its layout (the file's size and its MZ header with the relocation
 * table), then by its whole load image, one of the edition's known variants.
 * The whole image is checked, not just the pieces, because the host loads all
 * of it as DOS did. Then every piece the game takes is checked against the
 * edition's import table (src/Support/original_import_table_<edition>.c,
 * written by tools/import/make_import_table.py): an offset, a length and a
 * keyed hash for each, never the bytes. A copy that isn't accepted is named
 * with what differs: no edition's layout, or the pieces that don't match.
 * The data pieces are put in place here; the texts go to the story
 * (revenge/story.h), which the importer takes from the same image.
 *
 * The working form is DOS 1.00's: the pieces' offsets and the data segment's
 * place in the image are its own. It is the only edition so
 * far; one laid out otherwise needs its pieces mapped to these offsets, which
 * the table and the loader gain with it.
 */

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum
{
	ORIGINAL_IMAGE_CAPACITY = 0x40000
};

/* 3A6A:0000 (the data segment) in the load image, which DOS loads at 0800h. */
#define ORIGINAL_DATA_SEGMENT_IMAGE_OFFSET 0x326A0UL

extern char OriginalImage[ORIGINAL_IMAGE_CAPACITY];

/* 1 when the copy is a known edition and variant, every piece matched and was
 * put in place; otherwise 0, after naming on stderr what didn't match. */
int OriginalImport_Load(const char *executablePath);
int OriginalImport_IsLoaded(void);

/* The import table, for the loader and the tests. */
typedef enum OriginalPieceKind
{
	ORIGINAL_PIECE_TEXT,                  /* a string at a DS offset */
	ORIGINAL_PIECE_TEXT_TABLE,            /* far pointers at a DS offset to strings: const char *[] */
	ORIGINAL_PIECE_EXPANDED_TEXT_TABLE,   /* the same, with the game's digram compression expanded */
	ORIGINAL_PIECE_MUTABLE_TEXT_TABLE,    /* the same as a text table, each string a writable copy: char *[] */
	ORIGINAL_PIECE_POINTED_TEXT,          /* the first far pointer of a table, as one const char * */
	ORIGINAL_PIECE_MUTABLE_POINTED_TEXT,  /* the same, copied into a char array of `length` bytes */
	ORIGINAL_PIECE_BYTES,                 /* `length` bytes at a DS offset for an object of that size, the
	                                         story's data: the story file keeps it and places it
	                                         (revenge/story.h), not the loader */
	ORIGINAL_PIECE_MEDIA                  /* the same, the story's media */
} OriginalPieceKind;

typedef struct OriginalPiece
{
	const char *name;
	OriginalPieceKind kind;
	uint16_t dataOffset;
	uint16_t count;         /* entries of a table; 0 otherwise */
	uint8_t stride;         /* bytes between a table's far pointers */
	uint16_t length;        /* a text's length without its NUL; a byte piece's or array's size */
	void *target;           /* NULL for ORIGINAL_PIECE_TEXT, which is read in place */
	const char *keyedHash;  /* the first 16 hex digits of the keyed HMAC-SHA-256 of the piece */
} OriginalPiece;

/* Where a story text (revenge/story.h) is in an edition's load image: the
 * image offset of its string, or STORY_NO_SOURCE for a null table entry (an
 * empty text); the segment the original names it by (its far pointer's, the
 * image loaded at 0800h); its length and keyed hash as the importer must find
 * them, the hash over the text and its NUL. An expanded text is hashed as
 * expanded. */
#define STORY_NO_SOURCE 0xFFFFFFFFUL

typedef struct StoryTextSource
{
	uint32_t imageOffset;
	uint16_t segment;
	uint16_t length;
	uint8_t expanded;       /* 1: the game's digram compression, expanded */
	const char *keyedHash;
} StoryTextSource;

/* One known copy of an edition, by the keyed hash of its whole load image. */
typedef struct OriginalVariant
{
	const char *label;            /* "" for the plain copy, else a mark of its own */
	const char *imageKeyedHash;
} OriginalVariant;

/* An edition: how its program is laid out, its pieces and its known variants
 * (src/Support/original_import_editions.c, from original_editions.txt). */
typedef struct OriginalEdition
{
	const char *name;             /* as messages name it: "DOS 1.00" */
	uint32_t fileBytes;
	uint16_t headerParagraphs;
	const char *headerKeyedHash;  /* the MZ header with its relocation table */
	const OriginalPiece *pieces;
	size_t pieceCount;
	const StoryTextSource *storyTexts;   /* STORY_TEXT_COUNT of them, by StoryTextId */
	const OriginalVariant *variants;
	size_t variantCount;
} OriginalEdition;

extern const OriginalEdition OriginalEditions[];
extern const size_t OriginalEditionCount;
extern const char OriginalImportKey[];

/* Reads executablePath into OriginalImage and finds it among editions: the
 * edition whose layout and one of whose variants it matches, with every piece
 * matching too. Otherwise NULL, after naming on stderr what differs. Places
 * nothing; OriginalImport_Load does, for the editions above. Reading replaces
 * the image, so the tests that call it don't load as well. */
const OriginalEdition *OriginalImport_Identify(const char *executablePath, const OriginalEdition *editions,
	size_t editionCount);

/* After OriginalImport_Load: the edition and variant accepted (NULL before).
 * After it or an OriginalImport_Identify that accepted the copy: the load
 * image's size and its relocations, each the image offset of a segment word
 * DOS adds the load segment to (0 and none otherwise). */
const OriginalEdition *OriginalImport_Edition(void);

/* Forgets the copy's image (zeroed) once the story is prepared, so nothing
 * in play can read it; the edition stays known. */
void OriginalImport_Release(void);
const OriginalVariant *OriginalImport_Variant(void);
size_t OriginalImport_ImageBytes(void);
size_t OriginalImport_RelocationCount(void);
uint32_t OriginalImport_Relocation(size_t index);

/* The first 16 hex digits of the keyed HMAC-SHA-256 the tables hold. */
void OriginalImport_KeyedHash(const uint8_t *bytes, size_t length, char hex[17]);

/* The bytes a piece is hashed over: each string (expanded where the kind says)
 * or list in order, each string followed by its NUL. Returns the byte count,
 * or (size_t) -1 when the piece doesn't fit the image. Writes at most
 * `capacity` bytes. */
size_t OriginalImport_PieceBytes(const OriginalPiece *piece, uint8_t *out, size_t capacity);

/* A story text's bytes as the image holds them (expanded where the source
 * says), without the NUL: the byte count, or (size_t) -1 when the source
 * doesn't fit the image. Writes at most `capacity` bytes. */
size_t OriginalImport_TextBytes(const StoryTextSource *source, uint8_t *out, size_t capacity);

#ifdef __cplusplus
}
#endif

#endif
