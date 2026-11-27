// The rules' C door (include/revenge/rules.h).
#include "Rules.h"

#include <exception>

#include "revenge/debug.h"
#include "revenge/rules.h"

void Rules_Load(const char* dataDirectory) {
    if (dataDirectory == nullptr || dataDirectory[0] == '\0')
        RevengeFault("rules: there is no data folder to read them from");
    try {
        revenge::rules::load(dataDirectory);
    } catch (const std::exception& error) {
        RevengeFault("rules: %s", error.what());
    }
}
