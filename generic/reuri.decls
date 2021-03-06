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
declare 9 generic {
	int Reuri_ResolveIndex(Tcl_Interp* interp, Tcl_Obj* indexObj, size_t length, Tcl_Obj** elementsPtrPtr)
}

# vim: ft=tcl foldmethod=marker foldmarker=<<<,>>>
