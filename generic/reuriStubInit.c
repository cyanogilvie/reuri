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
    Reuri_URIObjPartExists, /* 6 */
    0, /* 7 */
    Reuri_GetPathFromObj, /* 8 */
    Reuri_ResolveIndex, /* 9 */
    Reuri_PercentDecodeObj, /* 10 */
};

/* !END!: Do not edit above this line. */


const ReuriStubs* ReuriStubsPtr = NULL;
MODULE_SCOPE const ReuriStubs* const reuriConstStubsPtr;
const ReuriStubs* const reuriConstStubsPtr = &reuriStubs;
