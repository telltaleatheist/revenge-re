#define _CRT_SECURE_NO_WARNINGS   /* fopen and freopen */
#include "revenge/original_import.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* A copy is accepted only as a known edition (its layout) and a known variant
 * of it (its whole load image), with every piece matching; otherwise it is
 * refused, naming what differs. A made-up program stands in for the game. */

enum
{
	HEADER_BYTES = 0x20,
	TEXT_OFFSET = 0x0010,
	IMAGE_BYTES = ORIGINAL_DATA_SEGMENT_IMAGE_OFFSET + 0x20
};

static const char programPath[] = "editions-test.exe";
static const char messagesPath[] = "editions-test-messages.txt";
static uint8_t file[HEADER_BYTES + IMAGE_BYTES];
static size_t fileBytes;

static void MakeProgram(void)
{
	memset(file, 0, sizeof(file));
	file[0x00] = 'M';
	file[0x01] = 'Z';
	file[0x06] = 0x01;                 /* one relocation */
	file[0x08] = HEADER_BYTES / 16;    /* header paragraphs */
	file[0x0A] = 0x10;                 /* minimum allocation */
	file[0x18] = 0x1C;                 /* the relocation table */
	file[0x1C] = 0x04;                 /* offset 0004 */
	file[0x1E] = 0x01;                 /* segment 0001 */
	memcpy(&file[HEADER_BYTES + ORIGINAL_DATA_SEGMENT_IMAGE_OFFSET + TEXT_OFFSET], "Kq7", 4);
	fileBytes = sizeof(file);
}

static void WriteProgram(void)
{
	FILE *out = fopen(programPath, "wb");

	assert(out != NULL);
	assert(fwrite(file, 1, fileBytes, out) == fileBytes);
	fclose(out);
}

static char headerHash[17];
static char imageHash[17];
static char changedImageHash[17];
static char textHash[17];
static OriginalPiece pieces[1];
static OriginalVariant variants[2];
static OriginalEdition edition;

static void Hash(char out[17], const uint8_t *bytes, size_t length)
{
	OriginalImport_KeyedHash(bytes, length, out);
}

/* Identifies the program on disk; the messages it wrote, in messages. */
static const OriginalEdition *Identify(char *messages, size_t capacity)
{
	const OriginalEdition *found;
	FILE *in;
	size_t length;

	WriteProgram();
	assert(freopen(messagesPath, "w", stderr) != NULL);
	found = OriginalImport_Identify(programPath, &edition, 1);
	fflush(stderr);
	in = fopen(messagesPath, "rb");
	assert(in != NULL);
	length = fread(messages, 1, capacity - 1, in);
	messages[length] = '\0';
	fclose(in);
	return found;
}

int main(void)
{
	char messages[512];
	uint8_t text[4] = { 'K', 'q', '7', 0 };

	MakeProgram();
	Hash(headerHash, file, HEADER_BYTES);
	Hash(imageHash, &file[HEADER_BYTES], IMAGE_BYTES);
	Hash(textHash, text, sizeof(text));
	pieces[0].name = "text";
	pieces[0].kind = ORIGINAL_PIECE_TEXT;
	pieces[0].dataOffset = TEXT_OFFSET;
	pieces[0].length = 3;
	pieces[0].keyedHash = textHash;
	variants[0].label = "";
	variants[0].imageKeyedHash = imageHash;
	edition.name = "Test 1.0";
	edition.fileBytes = (uint32_t) sizeof(file);
	edition.headerParagraphs = HEADER_BYTES / 16;
	edition.headerKeyedHash = headerHash;
	edition.pieces = pieces;
	edition.pieceCount = 1;
	edition.variants = variants;
	edition.variantCount = 1;

	/* The plain copy: accepted, its image and relocations as the file has them. */
	assert(Identify(messages, sizeof(messages)) == &edition);
	assert(messages[0] == '\0');
	assert(OriginalImport_ImageBytes() == IMAGE_BYTES);
	assert(OriginalImport_RelocationCount() == 1);
	assert(OriginalImport_Relocation(0) == 0x14);
	assert(memcmp(ORIGINAL_TEXT(TEXT_OFFSET), "Kq7", 4) == 0);

	/* A byte changed outside the pieces: refused, as the whole program is loaded. */
	file[HEADER_BYTES + 0x40] = 0x01;
	assert(Identify(messages, sizeof(messages)) == NULL);
	assert(strstr(messages, "differs from Test 1.0's outside the pieces") != NULL);
	assert(OriginalImport_ImageBytes() == 0);
	assert(OriginalImport_RelocationCount() == 0);

	/* The same copy, once listed as a variant: accepted. */
	Hash(changedImageHash, &file[HEADER_BYTES], IMAGE_BYTES);
	variants[1].label = "changed";
	variants[1].imageKeyedHash = changedImageHash;
	edition.variantCount = 2;
	assert(Identify(messages, sizeof(messages)) == &edition);
	assert(messages[0] == '\0');

	/* A piece changed: refused, naming it, even with its image listed. */
	file[HEADER_BYTES + ORIGINAL_DATA_SEGMENT_IMAGE_OFFSET + TEXT_OFFSET] = 'k';
	Hash(changedImageHash, &file[HEADER_BYTES], IMAGE_BYTES);
	assert(Identify(messages, sizeof(messages)) == NULL);
	assert(strstr(messages, "pieces that don't match Test 1.0: text DS:0010 (1 of 1)") != NULL);
	MakeProgram();

	/* Another layout: the header, or the file's size. */
	file[0x0A] = 0x11;
	assert(Identify(messages, sizeof(messages)) == NULL);
	assert(strstr(messages, "not an edition the rebuild knows (it knows Test 1.0)") != NULL);
	MakeProgram();
	fileBytes = sizeof(file) - 1;
	assert(Identify(messages, sizeof(messages)) == NULL);
	assert(strstr(messages, "not an edition the rebuild knows") != NULL);
	MakeProgram();

	/* A relocation table past the header. */
	file[0x18] = 0x1E;
	assert(Identify(messages, sizeof(messages)) == NULL);
	assert(strstr(messages, "its relocation table is outside its header") != NULL);

	remove(programPath);
	remove(messagesPath);
	return 0;
}
