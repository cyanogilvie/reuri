#include "reuri.h"

/* !BEGIN!: Do not edit below this line. */

const ReuriStubs reuriStubs = {
    TCL_STUB_MAGIC,
    0,
    Reuri_URIObjGetPart, /* 0 */
    Reuri_URIObjGetAll, /* 1 */
};

/* !END!: Do not edit above this line. */


const ReuriStubs* ReuriStubsPtr = NULL;
MODULE_SCOPE const ReuriStubs* const reuriConstStubsPtr;
const ReuriStubs* const reuriConstStubsPtr = &reuriStubs;
