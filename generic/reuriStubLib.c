#undef USE_TCL_STUBS
#undef USE_REURI_STUBS
#define USE_TCL_STUBS 1
#define USE_REURI_STUBS 1

#include "reuri.h"

MODULE_SCOPE const ReuriStubs*	reuriStubsPtr;
const ReuriStubs*				reuriStubsPtr = NULL;

#undef ReuriInitializeStubs
MODULE_SCOPE const char* ReuriInitializeStubs(Tcl_Interp* interp)
{
	const char*	got = NULL;
	got = Tcl_PkgRequireEx(interp, PACKAGE_NAME, PACKAGE_VERSION, 0, &reuriStubsPtr);
	return got;
}
