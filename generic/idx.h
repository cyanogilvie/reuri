#ifndef _IDX_H
#define _IDX_H

#ifdef __cplusplus
extern "C" {
#endif

enum idx_type {
    IDX_SINGLE,
    IDX_RANGE
};

#ifdef __cplusplus
}
#endif

int Idx_PickFromList(Tcl_Interp* interp, Tcl_Obj* listObj, Tcl_Obj* indexObj, Tcl_Obj** picked);
int Idx_Exists(Tcl_Interp* interp, size_t length, Tcl_Obj* indexObj, Tcl_Obj** exists);
int Idx_Resolve(Tcl_Interp* interp, Tcl_Obj* indexObj, size_t length, Tcl_Obj** elementsPtrPtr, enum idx_type* type);
int Idx_IndexType(Tcl_Interp* interp, Tcl_Obj* indexObj, enum idx_type* type);
#endif
