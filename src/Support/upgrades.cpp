#include "revenge/upgrades.h"

#include "Json.h"
#include "revenge/debug.h"

#include <ctype.h>
#include <exception>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

/* Ours (include/revenge/upgrades.h): the switches' state, all off to start. */

static const char *const UpgradeKeys[UPGRADE_COUNT] = { "lance_pursuit" };
static int upgradeOn[UPGRADE_COUNT];
static int testMode;
static int lineShown = 1;
static void (*settingsSaver)(void);
static std::string texts[UPGRADES_TEXT_COUNT];
static std::string labels[UPGRADE_COUNT];
static std::string helps[UPGRADE_COUNT];

const char *Upgrades_Key(int upgrade)
{
    return upgrade >= 0 && upgrade < UPGRADE_COUNT ? UpgradeKeys[upgrade] : "";
}

int Upgrades_Index(const char *key)
{
    int upgrade;

    for (upgrade = 0; upgrade < UPGRADE_COUNT; ++upgrade)
    {
        const char *known = UpgradeKeys[upgrade];
        const char *word = key;

        while (*known != '\0' && tolower((unsigned char) *word) == *known)
        {
            ++known;
            ++word;
        }
        if (*known == '\0' && *word == '\0')
            return upgrade;
    }
    return -1;
}

int Upgrades_IsOn(int upgrade)
{
    return upgrade >= 0 && upgrade < UPGRADE_COUNT && upgradeOn[upgrade];
}

void Upgrades_Set(int upgrade, int on)
{
    if (upgrade >= 0 && upgrade < UPGRADE_COUNT)
        upgradeOn[upgrade] = on != 0;
}

void Upgrades_SetTestMode(int on)
{
    testMode = on != 0;
}

int Upgrades_TestMode(void)
{
    return testMode;
}

void Upgrades_SetLineShown(int shown)
{
    lineShown = shown != 0;
}

int Upgrades_LineShown(void)
{
    return lineShown;
}

void Upgrades_SetSaver(void (*saver)(void))
{
    settingsSaver = saver;
}

void Upgrades_Save(void)
{
    if (settingsSaver != NULL)
        settingsSaver();
}

/* A member that must be a string of printable ASCII, as the game's font
 * draws it; where names it for the fault. */
static std::string TakeWords(const revenge::support::Json &object, const char *key, const std::string &where)
{
    const revenge::support::Json *value = object.find(key);

    if (value == NULL || !value->is_string() || value->string().empty())
        throw revenge::support::JsonError(where + key + " is not a string");
    for (const char c : value->string())
        if (static_cast<unsigned char>(c) < 0x20 || static_cast<unsigned char>(c) > 0x7E)
            throw revenge::support::JsonError(where + key + " holds a character outside printable ASCII");
    return value->string();
}

void Upgrades_LoadText(const char *dataDirectory)
{
    static const char *const TextKeys[UPGRADES_TEXT_COUNT] = { "options_line", "on", "off", "done" };
    const std::filesystem::path path = std::filesystem::path(dataDirectory) / "ui" / "upgrades.json";

    try
    {
        std::ifstream file(path, std::ios::binary);
        std::ostringstream text;
        int index;

        if (!file)
            throw revenge::support::JsonError("can't be read");
        text << file.rdbuf();
        const revenge::support::Json root = revenge::support::parse_json(text.str());
        if (!root.is_object())
            throw revenge::support::JsonError("is not an object");
        for (index = 0; index < UPGRADES_TEXT_COUNT; ++index)
            texts[index] = TakeWords(root, TextKeys[index], "");
        const revenge::support::Json *switches = root.find("switches");
        if (switches == NULL || !switches->is_object())
            throw revenge::support::JsonError("switches is not an object");
        for (index = 0; index < UPGRADE_COUNT; ++index)
        {
            const revenge::support::Json *entry = switches->find(UpgradeKeys[index]);
            const std::string where = std::string("switches.") + UpgradeKeys[index] + ".";

            if (entry == NULL || !entry->is_object())
                throw revenge::support::JsonError(where.substr(0, where.size() - 1) + " is not there");
            labels[index] = TakeWords(*entry, "label", where);
            helps[index] = TakeWords(*entry, "help", where);
        }
    }
    catch (const std::exception &error)
    {
        RevengeFault("%s: %s", path.string().c_str(), error.what());
    }
}

const char *Upgrades_Text(int which)
{
    return which >= 0 && which < UPGRADES_TEXT_COUNT ? texts[which].c_str() : "";
}

const char *Upgrades_Label(int upgrade)
{
    return upgrade >= 0 && upgrade < UPGRADE_COUNT ? labels[upgrade].c_str() : "";
}

const char *Upgrades_Help(int upgrade)
{
    return upgrade >= 0 && upgrade < UPGRADE_COUNT ? helps[upgrade].c_str() : "";
}
