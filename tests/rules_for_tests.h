#ifndef REVENGE_TESTS_RULES_FOR_TESTS_H
#define REVENGE_TESTS_RULES_FOR_TESTS_H

/* The rules (revenge/rules.h) from the source tree's data/, for a test whose
 * code reads their tables: CMake's chr_uses_rules names the folder. */
#include "revenge/rules.h"

#ifndef CHR_DATA_DIRECTORY
#error "a test reading the rules is built with chr_uses_rules (CMakeLists.txt)"
#endif

static void RequireRules(void)
{
	Rules_Load(CHR_DATA_DIRECTORY);
}

#endif
