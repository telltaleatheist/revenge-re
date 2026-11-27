#define _CRT_SECURE_NO_WARNINGS   /* original_import_for_tests.h uses fopen */
#include "fake_backend.h"
#include "revenge/original_data.h"
#include "revenge/original/lance_editor_data.h"
#include "../src/Original/REVENGE.h"
#include "../src/Original/Units/REVENGE_2507_LANCE_EDITOR_INTERNAL.h"

#include "original_import_for_tests.h"
#include <assert.h>

/* 2507:0647-065E: SelectUnitTypeFromList treats the list at DS:32D7, the
 * Omni list (UnitClassTypeLists[5]), as the one whose last unit is offered
 * only while an Enemy unit is chosen. */
int main(void)
{
    word16 listIndex;

    RequireOriginalImport();
    assert(IsOmniUnitTypeList(UnitClassTypeLists[0x05]));
    for (listIndex = 0x00; listIndex < 0x05; ++listIndex)
        assert(!IsOmniUnitTypeList(UnitClassTypeLists[listIndex]));
    return 0;
}
