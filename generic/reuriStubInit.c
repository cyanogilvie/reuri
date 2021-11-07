#include "reuri.h"

/* !BEGIN!: Do not edit below this line. */

const ReuriStubs reuriStubs = {
    TCL_STUB_MAGIC,
    0,
    Reuri_URIObjGetPart, /* 0 */
    Reuri_URIObjGetAll, /* 1 */
    Reuri_PercentEncodeObj, /* 2 */
    Reuri_NewQueryObj, /* 3 */
    Reuri_CompileQuery, /* 4 */
    Reuri_CompilePath, /* 5 */
};

/* !END!: Do not edit above this line. */


const ReuriStubs* ReuriStubsPtr = NULL;
MODULE_SCOPE const ReuriStubs* const reuriConstStubsPtr;
const ReuriStubs* const reuriConstStubsPtr = &reuriStubs;
