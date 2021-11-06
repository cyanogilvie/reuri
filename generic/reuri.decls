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

# vim: ft=tcl foldmethod=marker foldmarker=<<<,>>>
