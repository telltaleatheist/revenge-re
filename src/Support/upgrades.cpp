#include "revenge/upgrades.h"

#include <ctype.h>

/* Ours (include/revenge/upgrades.h): the switches' state, all off to start. */

static const char *const UpgradeKeys[UPGRADE_COUNT] = { "lance_pursuit" };
static int upgradeOn[UPGRADE_COUNT];
static int testMode;

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
