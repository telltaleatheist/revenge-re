// The rules' C door (include/revenge/rules.h).
#include "Rules.h"

#include <algorithm>
#include <exception>

#include "revenge/debug.h"
#include "revenge/rules.h"

void Rules_Load(const char *dataDirectory)
{
    if (dataDirectory == nullptr || dataDirectory[0] == '\0')
        RevengeFault("rules: there is no data folder to read them from");
    try
    {
        revenge::rules::load(dataDirectory);
    }
    catch (const std::exception &error)
    {
        RevengeFault("rules: %s", error.what());
    }
}

void Rules_StartingReplayPilots(uint8_t pool[15], uint8_t *count)
{
    std::copy(revenge::rules::g_starting_replay_pilots.begin(), revenge::rules::g_starting_replay_pilots.end(), pool);
    *count = revenge::rules::g_starting_replay_pilot_count;
}
