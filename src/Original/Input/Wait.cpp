// 0D73: wait
/* Input-wait procedure from original segment 0D73. */

#include "REVENGE.h"

#include "revenge/input.h"

// 0D73:3C7C: word16 FlushInputAndWait(word16 timeoutRetraces)
// Called from:
//      PresentScreenAndMessage
uint16_t FlushInputAndWait(uint16_t timeoutRetraces)
{
    FlushPendingInput();
    return WaitForDismissInputOrTimeout(timeoutRetraces);
}
