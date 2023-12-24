library reuri
interface reuri

declare 0 generic {
	int Reuri_URIObjGetPart(Tcl_Interp* interp, Tcl_Obj* uriPtr, enum reuri_part part, Tcl_Obj* defaultPtr, Tcl_Obj** valuePtrPtr)
}
declare 1 generic {
	int Reuri_URIObjGetAll(Tcl_Interp* interp, Tcl_Obj* uriPtr, Tcl_Obj** res)
}
declare 2 generic {
    Tcl_Obj* Reuri_PercentEncodeObj(Tcl_Interp* interp, enum reuri_encode_mode mode, Tcl_Obj* objPtr)
}
declare 3 generic {
	int Reuri_NewQueryObj(Tcl_Interp* interp, int objc, Tcl_Obj* const objv[], Tcl_Obj** res)
}
declare 4 generic {
	int Reuri_CompileQuery(Tcl_Interp* interp, Tcl_DString* ds, Tcl_Obj* params)
}
declare 5 generic {
	int Reuri_CompilePath(Tcl_Interp* interp, Tcl_DString* ds, Tcl_Obj* pathListPtr)
}
declare 6 generic {
	int Reuri_URIObjPartExists(Tcl_Interp* interp, Tcl_Obj* uriPtr, enum reuri_part part, int* existsPtr)
}
declare 8 generic {
	int Reuri_GetPathFromObj(Tcl_Interp* interp, Tcl_Obj* pathPtr, Tcl_Obj** pathlistPtrPtr)
}
declare 10 generic {
	Tcl_Obj* Reuri_PercentDecodeObj(Tcl_Obj* in)
}
declare 11 generic {
	int Reuri_GetPartFromObj(Tcl_Interp* interp, Tcl_Obj* partObj, enum reuri_part* part)
}
declare 12 generic {
	int Reuri_URIObjExtractPart(Tcl_Interp* interp, Tcl_Obj* uriPtr, enum reuri_part part, Tcl_Obj* defaultPtr, Tcl_Obj** valuePtrPtr)
}
declare 13 generic {
	int Reuri_URIObjExtractAll(Tcl_Interp* interp, Tcl_Obj* uriPtr, Tcl_Obj** res)
}
declare 14 generic {
	int Reuri_URIObjSet(Tcl_Interp* interp, Tcl_Obj* uriPtr, enum reuri_part part, Tcl_Obj* valuePtr, Tcl_Obj** resPtrPtr)
}

declare 15 generic {
	int Reuri_URIObjQueryGet(Tcl_Interp* interp, Tcl_Obj* uriPtr, Tcl_Obj* param, Tcl_Obj* def /* may be NULL */, Tcl_Obj* index /* may be NULL */, int flags, Tcl_Obj** out)
}
declare 16 generic {
	int Reuri_URIObjQueryValues(Tcl_Interp* interp, Tcl_Obj* uriPtr, Tcl_Obj* param, Tcl_Obj** out)
}
declare 17 generic {
	int Reuri_URIObjQueryAdd(Tcl_Interp* interp, Tcl_Obj* uriPtr, Tcl_Obj* param, Tcl_Obj* value, Tcl_Obj** out)
}
#declare 18 generic {
#	Use Get with flags without REURI_FLAG_REQUIRED
#	int Reuri_URIObjQueryExists(Tcl_Interp* interp, Tcl_Obj* uriPtr, Tcl_Obj* param, int* out)
#}
declare 19 generic {
	int Reuri_URIObjQuerySet(Tcl_Interp* interp, Tcl_Obj* uriPtr, Tcl_Obj* param, Tcl_Obj* value, Tcl_Obj** out)
}
declare 20 generic {
	int Reuri_URIObjQueryUnset(Tcl_Interp* interp, Tcl_Obj* uriPtr, Tcl_Obj* param, Tcl_Obj** out)
}
declare 21 generic {
	int Reuri_URIObjQueryNames(Tcl_Interp* interp, Tcl_Obj* uriPtr, Tcl_Obj** out)
}
declare 22 generic {
	int Reuri_URIObjQueryNew(Tcl_Interp* interp, Tcl_Obj* uriPtr, Tcl_Obj* params /* list */, Tcl_Obj** out)
}

# vim: ft=tcl foldmethod=marker foldmarker=<<<,>>>
