#include "fake_backend.h"
#include "Data/TacticalData.h"
#include "Tactical/Tactical.h"

#include <assert.h>
#include <string.h>

/* 2007:14DF and 2007:1544 mark the previous status row for redraw with one
 * byte, mov byte ptr [bx+0B574h],0FFh, and only when the row changed. */

static void ResetRows(void)
{
    memset(TacticalStatusRowInvalidation, 0x00, sizeof(TacticalStatusRowInvalidation));
    TacticalUiSuppressed = FALSE;
    MouseInputDisabled = 0x0001;       /* no cursor to hide and restore */
}

static void CheckPrimary(void)
{
    ResetRows();
    PrimaryStatusRow = 0x02;
    PrimaryStatusHighlightColor = 0x0F;
    CachedPrimaryStatusHighlightColor = 0x0F;
    UpdatePrimaryUnitStatusRowHighlight(0x03);
    assert(TacticalStatusRowInvalidation[0x02] == 0xFF);
    assert(TacticalStatusRowInvalidation[0x04] == 0x00);
    assert(TacticalStatusRowInvalidation[0x05] == 0x00);
    assert(PrimaryStatusRow == 0x03);

    /* Only the blink color changed: the row is redrawn, nothing is invalidated. */
    ResetRows();
    PrimaryStatusHighlightColor = 0x07;
    UpdatePrimaryUnitStatusRowHighlight(0x03);
    assert(memchr(TacticalStatusRowInvalidation, 0xFF, sizeof(TacticalStatusRowInvalidation)) == NULL);
    assert(CachedPrimaryStatusHighlightColor == 0x07);
}

static void CheckSecondary(void)
{
    ResetRows();
    SecondaryStatusRow = 0x01;
    SecondaryStatusHighlightColor = 0x0F;
    CachedSecondaryStatusHighlightColor = 0x0F;
    UpdateSecondaryUnitStatusRowHighlight(0x04);
    assert(TacticalStatusRowInvalidation[0x01] == 0xFF);
    assert(TacticalStatusRowInvalidation[0x02] == 0x00);
    assert(TacticalStatusRowInvalidation[0x03] == 0x00);
    assert(SecondaryStatusRow == 0x04);

    ResetRows();
    SecondaryStatusHighlightColor = 0x07;
    UpdateSecondaryUnitStatusRowHighlight(0x04);
    assert(memchr(TacticalStatusRowInvalidation, 0xFF, sizeof(TacticalStatusRowInvalidation)) == NULL);
}

int main(void)
{
    FakeBackend_Reset();
    CheckPrimary();
    CheckSecondary();
    return 0;
}
