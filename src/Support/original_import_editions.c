/* Written by tools/import/make_import_table.py from original_editions.txt.
 * Layouts, piece counts and keyed hashes only. Don't edit by hand. */
#include "revenge/original_import.h"

extern const OriginalPiece OriginalPieces_dos100[0x189];

static const OriginalVariant Variants_dos100[] =
{
	{ "", "29a96f24f3a0ca3d" },
};

const OriginalEdition OriginalEditions[] =
{
	{ "DOS 1.00", 256411, 0x420, "dce27be0dd72d8d1", OriginalPieces_dos100, 0x189,
		Variants_dos100, sizeof(Variants_dos100) / sizeof(Variants_dos100[0]) },
};

const size_t OriginalEditionCount = sizeof(OriginalEditions) / sizeof(OriginalEditions[0]);
