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
	KEYED_HASH_DIGITS = 16,
	TEXT_LIST_END = 0xFF
};

/* The digram dictionaries RenderFormattedText expands high-bit text codes with. */
#define DICTIONARY_FIRST_OFFSET 0x05AAu
#define DICTIONARY_SECOND_OFFSET 0x05BAu

char OriginalImage[ORIGINAL_IMAGE_CAPACITY] = { 0 };   /* defined here, not a common block */
static size_t originalImageBytes;
static int originalImportLoaded;
static char emptyText[1];

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

static size_t ListLength(size_t offset)
{
	size_t length = 0;

	while (offset + length < originalImageBytes && (uint8_t) OriginalImage[offset + length] != TEXT_LIST_END)
		++length;
	return offset + length < originalImageBytes ? length + 1 : (size_t) -1;
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
	case ORIGINAL_PIECE_BYTE_LISTS:
		for (index = 0; index < (piece->count != 0 ? piece->count : 1); ++index)
		{
			size_t offset = EntryOffset(piece, index);
			size_t length;

			if (offset == (size_t) -2)
				return (size_t) -1;
			if (offset == (size_t) -1)
			{
				/* A null list adds nothing; a null string is an empty one. */
				if (piece->kind != ORIGINAL_PIECE_BYTE_LISTS)
					at = Append(out, capacity, at, &terminator, 1);
				continue;
			}
			length = piece->kind == ORIGINAL_PIECE_BYTE_LISTS ? ListLength(offset) : TextLength(offset);
			if (length == (size_t) -1)
				return (size_t) -1;
			if (piece->kind == ORIGINAL_PIECE_EXPANDED_TEXT_TABLE)
				at += ExpandText(offset, length, at < capacity ? out + at : out, at < capacity ? capacity - at : 0);
			else
				at = Append(out, capacity, at, &OriginalImage[offset], length);
			if (piece->kind != ORIGINAL_PIECE_BYTE_LISTS)
				at = Append(out, capacity, at, &terminator, 1);
		}
		return at;
	}
	return (size_t) -1;
}

static void KeyedHash(const uint8_t *bytes, size_t length, char hex[KEYED_HASH_DIGITS + 1])
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

static int ReadImage(const char *executablePath)
{
	FILE *file = fopen(executablePath, "rb");
	uint8_t header[MZ_HEADER_BYTES];
	long fileBytes;
	long headerBytes;
	size_t imageBytes;

	if (file == NULL)
	{
		fprintf(stderr, "%s: can't be opened\n", executablePath);
		return 0;
	}
	if (fread(header, sizeof(header), 1, file) != 1 || header[0] != 'M' || header[1] != 'Z' ||
		fseek(file, 0, SEEK_END) != 0 || (fileBytes = ftell(file)) < 0)
	{
		fprintf(stderr, "%s: not an MZ program\n", executablePath);
		fclose(file);
		return 0;
	}
	headerBytes = (long) (header[8] | header[9] << 8) * 16;
	if (fileBytes <= headerBytes || (size_t) (fileBytes - headerBytes) > sizeof(OriginalImage))
	{
		fprintf(stderr, "%s: its load image is %ld bytes, outside the supported size\n", executablePath,
			fileBytes - headerBytes);
		fclose(file);
		return 0;
	}
	imageBytes = (size_t) (fileBytes - headerBytes);
	memset(OriginalImage, 0, sizeof(OriginalImage));
	if (fseek(file, headerBytes, SEEK_SET) != 0 || fread(OriginalImage, 1, imageBytes, file) != imageBytes)
	{
		fprintf(stderr, "%s: can't be read\n", executablePath);
		fclose(file);
		memset(OriginalImage, 0, sizeof(OriginalImage));
		return 0;
	}
	fclose(file);
	originalImageBytes = imageBytes;
	return 1;
}

static char *CopyText(const uint8_t *bytes, size_t length)
{
	char *copy = (char *) malloc(length + 1);

	if (copy != NULL)
	{
		memcpy(copy, bytes, length);
		copy[length] = '\0';
	}
	return copy;
}

/* Puts a matched piece in place. Returns 0 only when memory runs out. */
static int PlacePiece(const OriginalPiece *piece, uint8_t *scratch)
{
	uint16_t index;

	switch (piece->kind)
	{
	case ORIGINAL_PIECE_TEXT:
		return 1;
	case ORIGINAL_PIECE_BYTES:
		memcpy(piece->target, &OriginalImage[ORIGINAL_DATA_SEGMENT_IMAGE_OFFSET + piece->dataOffset], piece->length);
		return 1;
	case ORIGINAL_PIECE_POINTED_TEXT:
	{
		size_t offset = EntryOffset(piece, 0);

		*(const char **) piece->target = offset == (size_t) -1 ? emptyText : &OriginalImage[offset];
		return 1;
	}
	case ORIGINAL_PIECE_MUTABLE_POINTED_TEXT:
	{
		size_t offset = EntryOffset(piece, 0);
		size_t length = offset == (size_t) -1 ? 0 : TextLength(offset);

		memset(piece->target, 0, piece->length);
		if (length > 0)
			memcpy(piece->target, &OriginalImage[offset], length);
		return 1;
	}
	case ORIGINAL_PIECE_TEXT_TABLE:
		for (index = 0; index < piece->count; ++index)
		{
			size_t offset = EntryOffset(piece, index);

			((const char **) piece->target)[index] = offset == (size_t) -1 ? emptyText : &OriginalImage[offset];
		}
		return 1;
	case ORIGINAL_PIECE_BYTE_LISTS:
		for (index = 0; index < piece->count; ++index)
		{
			size_t offset = EntryOffset(piece, index);

			((const uint8_t **) piece->target)[index] =
				offset == (size_t) -1 ? NULL : (const uint8_t *) &OriginalImage[offset];
		}
		return 1;
	case ORIGINAL_PIECE_EXPANDED_TEXT_TABLE:
		for (index = 0; index < piece->count; ++index)
		{
			size_t offset = EntryOffset(piece, index);
			size_t length = offset == (size_t) -1 ? 0
				: ExpandText(offset, TextLength(offset), scratch, PIECE_SCRATCH_BYTES);
			char *copy = CopyText(scratch, length);

			if (copy == NULL)
				return 0;
			((const char **) piece->target)[index] = copy;
		}
		return 1;
	case ORIGINAL_PIECE_MUTABLE_TEXT_TABLE:
		for (index = 0; index < piece->count; ++index)
		{
			size_t offset = EntryOffset(piece, index);
			char *copy = offset == (size_t) -1 ? CopyText(scratch, 0)
				: CopyText((const uint8_t *) &OriginalImage[offset], TextLength(offset));

			if (copy == NULL)
				return 0;
			((char **) piece->target)[index] = copy;
		}
		return 1;
	}
	return 0;
}

int OriginalImport_Load(const char *executablePath)
{
	uint8_t *scratch;
	size_t index;
	size_t mismatches = 0;

	if (originalImportLoaded)
		return 1;
	if (!ReadImage(executablePath))
		return 0;
	scratch = (uint8_t *) malloc(PIECE_SCRATCH_BYTES);
	if (scratch == NULL)
	{
		fprintf(stderr, "%s: no memory to check its pieces\n", executablePath);
		return 0;
	}
	for (index = 0; index < OriginalPieceCount; ++index)
	{
		const OriginalPiece *piece = &OriginalPieces[index];
		size_t length = OriginalImport_PieceBytes(piece, scratch, PIECE_SCRATCH_BYTES);
		char hash[KEYED_HASH_DIGITS + 1];
		int matches = length != (size_t) -1 && length <= PIECE_SCRATCH_BYTES;

		if (matches && piece->kind == ORIGINAL_PIECE_TEXT)
			matches = length == (size_t) piece->length + 1;
		if (matches)
		{
			KeyedHash(scratch, length, hash);
			matches = strcmp(hash, piece->keyedHash) == 0;
		}
		if (!matches)
		{
			if (mismatches == 0)
				fprintf(stderr, "%s: pieces that don't match the supported copy:", executablePath);
			if (piece->kind == ORIGINAL_PIECE_TEXT)
				fprintf(stderr, " text DS:%04X", piece->dataOffset);
			else
				fprintf(stderr, " %s", piece->name);
			++mismatches;
		}
	}
	if (mismatches != 0)
	{
		fprintf(stderr, " (%zu of %zu)\n", mismatches, OriginalPieceCount);
		free(scratch);
		return 0;
	}
	for (index = 0; index < OriginalPieceCount; ++index)
	{
		if (!PlacePiece(&OriginalPieces[index], scratch))
		{
			fprintf(stderr, "%s: no memory for %s\n", executablePath, OriginalPieces[index].name);
			free(scratch);
			return 0;
		}
	}
	free(scratch);
	originalImportLoaded = 1;
	return 1;
}

int OriginalImport_IsLoaded(void)
{
	return originalImportLoaded;
}
