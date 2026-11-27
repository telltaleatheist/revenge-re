/* Reads the player's REVENGE.EXE once and puts the game's own pieces in place
 * (include/revenge/original_import.h). */
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "revenge/original_import.h"
#include "revenge/sha256.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum
{
	MZ_HEADER_BYTES = 0x1C,
	PIECE_SCRATCH_BYTES = 0x20000,
	KEYED_HASH_DIGITS = 16
};

/* The digram dictionaries RenderFormattedText expands high-bit text codes with. */
#define DICTIONARY_FIRST_OFFSET 0x05AAu
#define DICTIONARY_SECOND_OFFSET 0x05BAu

char OriginalImage[ORIGINAL_IMAGE_CAPACITY] = { 0 };   /* defined here, not a common block */
static size_t originalImageBytes;
static uint8_t *originalHeader;     /* the MZ header with its relocation table */
static size_t originalHeaderBytes;
static size_t originalFileBytes;
static int originalImageAccepted;  /* by the last OriginalImport_Identify */
static int originalImportLoaded;
static const OriginalEdition *loadedEdition;
static const OriginalVariant *loadedVariant;

static uint16_t ImageWord(size_t offset)
{
	return (uint16_t) ((uint8_t) OriginalImage[offset] | (uint16_t) (uint8_t) OriginalImage[offset + 1] << 8);
}

/* The image offset a table entry's far pointer names (its segment as the file
 * stores it, before DOS relocates the load), or (size_t) -1 for a null pointer. */
static size_t EntryOffset(const OriginalPiece *piece, uint16_t index)
{
	size_t pointer = ORIGINAL_DATA_SEGMENT_IMAGE_OFFSET + piece->dataOffset + (size_t) index * piece->stride;
	uint16_t offset;
	uint16_t segment;

	if (pointer + 4 > originalImageBytes)
		return (size_t) -2;
	offset = ImageWord(pointer);
	segment = ImageWord(pointer + 2);
	if (offset == 0 && segment == 0)
		return (size_t) -1;
	return (size_t) segment * 16 + offset;
}

static size_t TextLength(size_t offset)
{
	size_t length = 0;

	while (offset + length < originalImageBytes && OriginalImage[offset + length] != '\0')
		++length;
	return offset + length < originalImageBytes ? length : (size_t) -1;
}

/* Expands one string the way RenderFormattedText (35CE) does: a code with its
 * high bit set stands for a first byte and a following code. */
static size_t ExpandText(size_t offset, size_t length, uint8_t *out, size_t capacity)
{
	size_t first = ORIGINAL_DATA_SEGMENT_IMAGE_OFFSET + DICTIONARY_FIRST_OFFSET;
	size_t second = ORIGINAL_DATA_SEGMENT_IMAGE_OFFSET + DICTIONARY_SECOND_OFFSET;
	size_t written = 0;
	size_t index;

	for (index = 0; index < length; ++index)
	{
		uint8_t value = (uint8_t) OriginalImage[offset + index];

		while ((value & 0x80) != 0)
		{
			unsigned dictionaryIndex = (value & 0x7F) >> 3;

			if (written < capacity)
				out[written] = (uint8_t) OriginalImage[first + dictionaryIndex];
			++written;
			value = (uint8_t) OriginalImage[second + dictionaryIndex * 8 + (value & 0x07)];
		}
		if (written < capacity)
			out[written] = value;
		++written;
	}
	return written;
}

static size_t Append(uint8_t *out, size_t capacity, size_t at, const void *bytes, size_t length)
{
	if (at < capacity)
		memcpy(out + at, bytes, at + length <= capacity ? length : capacity - at);
	return at + length;
}

size_t OriginalImport_PieceBytes(const OriginalPiece *piece, uint8_t *out, size_t capacity)
{
	static const uint8_t terminator = 0;
	size_t at = 0;
	uint16_t index;

	switch (piece->kind)
	{
	case ORIGINAL_PIECE_TEXT:
	{
		size_t offset = ORIGINAL_DATA_SEGMENT_IMAGE_OFFSET + piece->dataOffset;
		size_t length = TextLength(offset);

		if (length == (size_t) -1)
			return (size_t) -1;
		at = Append(out, capacity, at, &OriginalImage[offset], length);
		return Append(out, capacity, at, &terminator, 1);
	}
	case ORIGINAL_PIECE_BYTES:
	case ORIGINAL_PIECE_MEDIA:
	{
		size_t offset = ORIGINAL_DATA_SEGMENT_IMAGE_OFFSET + piece->dataOffset;

		if (offset + piece->length > originalImageBytes)
			return (size_t) -1;
		return Append(out, capacity, at, &OriginalImage[offset], piece->length);
	}
	case ORIGINAL_PIECE_TEXT_TABLE:
	case ORIGINAL_PIECE_EXPANDED_TEXT_TABLE:
	case ORIGINAL_PIECE_MUTABLE_TEXT_TABLE:
	case ORIGINAL_PIECE_POINTED_TEXT:
	case ORIGINAL_PIECE_MUTABLE_POINTED_TEXT:
		for (index = 0; index < (piece->count != 0 ? piece->count : 1); ++index)
		{
			size_t offset = EntryOffset(piece, index);
			size_t length;

			if (offset == (size_t) -2)
				return (size_t) -1;
			if (offset == (size_t) -1)
			{
				/* A null string is an empty one. */
				at = Append(out, capacity, at, &terminator, 1);
				continue;
			}
			length = TextLength(offset);
			if (length == (size_t) -1)
				return (size_t) -1;
			if (piece->kind == ORIGINAL_PIECE_EXPANDED_TEXT_TABLE)
				at += ExpandText(offset, length, at < capacity ? out + at : out, at < capacity ? capacity - at : 0);
			else
				at = Append(out, capacity, at, &OriginalImage[offset], length);
			at = Append(out, capacity, at, &terminator, 1);
		}
		return at;
	}
	return (size_t) -1;
}

void OriginalImport_KeyedHash(const uint8_t *bytes, size_t length, char hex[KEYED_HASH_DIGITS + 1])
{
	static const char digits[] = "0123456789abcdef";
	uint8_t digest[SHA256_DIGEST_BYTES];
	HmacSha256 mac;
	unsigned index;

	HmacSha256_Begin(&mac, OriginalImportKey, strlen(OriginalImportKey));
	HmacSha256_Add(&mac, bytes, length);
	HmacSha256_End(&mac, digest);
	for (index = 0; index < KEYED_HASH_DIGITS / 2; ++index)
	{
		hex[index * 2] = digits[digest[index] >> 4];
		hex[index * 2 + 1] = digits[digest[index] & 0x0F];
	}
	hex[KEYED_HASH_DIGITS] = '\0';
}

static uint16_t HeaderWord(size_t offset)
{
	return (uint16_t) (originalHeader[offset] | originalHeader[offset + 1] << 8);
}

/* Reads the file: its header into originalHeader, its load image into
 * OriginalImage. */
static int ReadImage(const char *executablePath)
{
	FILE *file = fopen(executablePath, "rb");
	uint8_t start[MZ_HEADER_BYTES];
	long fileBytes;
	size_t headerBytes;
	size_t imageBytes;
	size_t relocationCount;
	size_t relocationTable;

	free(originalHeader);
	originalHeader = NULL;
	originalHeaderBytes = originalFileBytes = originalImageBytes = 0;
	memset(OriginalImage, 0, sizeof(OriginalImage));
	if (file == NULL)
	{
		fprintf(stderr, "%s: can't be opened\n", executablePath);
		return 0;
	}
	if (fread(start, sizeof(start), 1, file) != 1 || start[0] != 'M' || start[1] != 'Z' ||
		fseek(file, 0, SEEK_END) != 0 || (fileBytes = ftell(file)) < 0)
	{
		fprintf(stderr, "%s: not an MZ program\n", executablePath);
		fclose(file);
		return 0;
	}
	headerBytes = (size_t) (start[8] | start[9] << 8) * 16;
	relocationCount = (size_t) (start[6] | start[7] << 8);
	relocationTable = (size_t) (start[0x18] | start[0x19] << 8);
	if (headerBytes < sizeof(start) || relocationTable + relocationCount * 4 > headerBytes)
	{
		fprintf(stderr, "%s: its relocation table is outside its header\n", executablePath);
		fclose(file);
		return 0;
	}
	if ((size_t) fileBytes <= headerBytes || (size_t) fileBytes - headerBytes > sizeof(OriginalImage))
	{
		fprintf(stderr, "%s: its load image is %ld bytes, outside the supported size\n", executablePath,
			fileBytes - (long) headerBytes);
		fclose(file);
		return 0;
	}
	imageBytes = (size_t) fileBytes - headerBytes;
	originalHeader = (uint8_t *) malloc(headerBytes);
	if (originalHeader == NULL || fseek(file, 0, SEEK_SET) != 0 ||
		fread(originalHeader, 1, headerBytes, file) != headerBytes ||
		fread(OriginalImage, 1, imageBytes, file) != imageBytes)
	{
		fprintf(stderr, "%s: can't be read\n", executablePath);
		fclose(file);
		free(originalHeader);
		originalHeader = NULL;
		memset(OriginalImage, 0, sizeof(OriginalImage));
		return 0;
	}
	fclose(file);
	originalFileBytes = (size_t) fileBytes;
	originalHeaderBytes = headerBytes;
	originalImageBytes = imageBytes;
	return 1;
}

size_t OriginalImport_TextBytes(const StoryTextSource *source, uint8_t *out, size_t capacity)
{
	size_t length;

	if (source->imageOffset == STORY_NO_SOURCE)
		return 0;
	if (source->imageOffset >= originalImageBytes)
		return (size_t) -1;
	length = TextLength(source->imageOffset);
	if (length == (size_t) -1)
		return (size_t) -1;
	if (source->expanded)
		return ExpandText(source->imageOffset, length, out, capacity);
	memcpy(out, &OriginalImage[source->imageOffset], length < capacity ? length : capacity);
	return length;
}

/* Names on one line each of the edition's pieces that doesn't match the
 * image. Returns how many. */
static size_t ReportPieceMismatches(const char *executablePath, const OriginalEdition *edition, uint8_t *scratch)
{
	size_t index;
	size_t mismatches = 0;

	for (index = 0; index < edition->pieceCount; ++index)
	{
		const OriginalPiece *piece = &edition->pieces[index];
		size_t length = OriginalImport_PieceBytes(piece, scratch, PIECE_SCRATCH_BYTES);
		char hash[KEYED_HASH_DIGITS + 1];
		int matches = length != (size_t) -1 && length <= PIECE_SCRATCH_BYTES;

		if (matches && piece->kind == ORIGINAL_PIECE_TEXT)
			matches = length == (size_t) piece->length + 1;
		if (matches)
		{
			OriginalImport_KeyedHash(scratch, length, hash);
			matches = strcmp(hash, piece->keyedHash) == 0;
		}
		if (!matches)
		{
			if (mismatches == 0)
				fprintf(stderr, "%s: pieces that don't match %s:", executablePath, edition->name);
			if (piece->kind == ORIGINAL_PIECE_TEXT)
				fprintf(stderr, " text DS:%04X", piece->dataOffset);
			else
				fprintf(stderr, " %s", piece->name);
			++mismatches;
		}
	}
	if (mismatches != 0)
		fprintf(stderr, " (%zu of %zu)\n", mismatches, edition->pieceCount);
	return mismatches;
}

static const OriginalVariant *identifiedVariant;

const OriginalEdition *OriginalImport_Identify(const char *executablePath, const OriginalEdition *editions,
	size_t editionCount)
{
	const OriginalEdition *edition = NULL;
	char hash[KEYED_HASH_DIGITS + 1];
	uint8_t *scratch;
	size_t index;
	size_t mismatches;

	identifiedVariant = NULL;
	originalImageAccepted = 0;
	if (!ReadImage(executablePath))
		return NULL;
	OriginalImport_KeyedHash(originalHeader, originalHeaderBytes, hash);
	for (index = 0; index < editionCount && edition == NULL; ++index)
	{
		if (editions[index].fileBytes == originalFileBytes &&
			(size_t) editions[index].headerParagraphs * 16 == originalHeaderBytes &&
			strcmp(editions[index].headerKeyedHash, hash) == 0)
			edition = &editions[index];
	}
	if (edition == NULL)
	{
		fprintf(stderr, "%s: not an edition the rebuild knows (it knows", executablePath);
		for (index = 0; index < editionCount; ++index)
			fprintf(stderr, "%s %s", index == 0 ? "" : ",", editions[index].name);
		fprintf(stderr, ")\n");
		return NULL;
	}
	OriginalImport_KeyedHash((const uint8_t *) OriginalImage, originalImageBytes, hash);
	for (index = 0; index < edition->variantCount && identifiedVariant == NULL; ++index)
	{
		if (strcmp(edition->variants[index].imageKeyedHash, hash) == 0)
			identifiedVariant = &edition->variants[index];
	}
	scratch = (uint8_t *) malloc(PIECE_SCRATCH_BYTES);
	if (scratch == NULL)
	{
		fprintf(stderr, "%s: no memory to check its pieces\n", executablePath);
		identifiedVariant = NULL;
		return NULL;
	}
	mismatches = ReportPieceMismatches(executablePath, edition, scratch);
	free(scratch);
	if (identifiedVariant == NULL && mismatches == 0)
		fprintf(stderr, "%s: its program differs from %s's outside the pieces, and the whole program is loaded\n",
			executablePath, edition->name);
	if (identifiedVariant == NULL || mismatches != 0)
	{
		identifiedVariant = NULL;
		return NULL;
	}
	originalImageAccepted = 1;
	return edition;
}

int OriginalImport_Load(const char *executablePath)
{
	const OriginalEdition *edition;

	if (originalImportLoaded)
		return 1;
	edition = OriginalImport_Identify(executablePath, OriginalEditions, OriginalEditionCount);
	if (edition == NULL)
		return 0;
	loadedEdition = edition;
	loadedVariant = identifiedVariant;
	originalImportLoaded = 1;
	return 1;
}

int OriginalImport_IsLoaded(void)
{
	return originalImportLoaded;
}

void OriginalImport_Release(void)
{
	memset(OriginalImage, 0, sizeof(OriginalImage));
	free(originalHeader);
	originalHeader = NULL;
	originalHeaderBytes = originalFileBytes = originalImageBytes = 0;
	originalImageAccepted = 0;
}

const OriginalEdition *OriginalImport_Edition(void)
{
	return loadedEdition;
}

const OriginalVariant *OriginalImport_Variant(void)
{
	return loadedVariant;
}

size_t OriginalImport_ImageBytes(void)
{
	return originalImageAccepted ? originalImageBytes : 0;
}

size_t OriginalImport_RelocationCount(void)
{
	return originalImageAccepted ? (size_t) HeaderWord(6) : 0;
}

uint32_t OriginalImport_Relocation(size_t index)
{
	size_t at = (size_t) HeaderWord(0x18) + index * 4;

	return (uint32_t) HeaderWord(at + 2) * 16 + HeaderWord(at);
}
