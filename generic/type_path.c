#include "reuriInt.h"
#include "tip445.h"

/* Intrep:
 *	ptrAndLongRep.ptr:		Tcl list of path elements.
 *	ptrAndLongRep.value:	unused
 */

#define PATH(irPtr)			((Tcl_Obj*)((irPtr)->ptrAndLongRep.ptr))
#define PATH_PTR(irPtr)		((Tcl_Obj**)(&((irPtr)->ptrAndLongRep.ptr)))

static void free_internal_rep(Tcl_Obj* obj);
static void dup_internal_rep(Tcl_Obj* src, Tcl_Obj* dup);
static void update_string_rep(Tcl_Obj* obj);

Tcl_ObjType path_objtype = {
	"ReuriPath",
	free_internal_rep,
	dup_internal_rep,
	update_string_rep,
	NULL
};

static void free_path(Tcl_ObjIntRep* ir) //<<<
{
	if (ir)
		replace_tclobj(PATH_PTR(ir), NULL);
}

//>>>
static void free_internal_rep(Tcl_Obj* obj) //<<<
{
	Tcl_ObjIntRep*		ir = Tcl_FetchIntRep(obj, &path_objtype);

	free_path(ir);
}

///>>>
static void dup_internal_rep(Tcl_Obj* src, Tcl_Obj* dup) //<<<
{
	Tcl_ObjIntRep*		ir = Tcl_FetchIntRep(src, &path_objtype);
	Tcl_ObjIntRep		newir;

	newir.ptrAndLongRep.ptr = NULL;
	replace_tclobj(PATH_PTR(&newir), PATH(ir));

	Tcl_StoreIntRep(dup, &path_objtype, &newir);
}

//>>>
static void update_string_rep(Tcl_Obj* obj) //<<<
{
	Tcl_ObjIntRep*		ir = Tcl_FetchIntRep(obj, &path_objtype);
	Tcl_DString			ds;

	Tcl_DStringInit(&ds);
	if (TCL_OK != Reuri_CompilePath(NULL, &ds, PATH(ir)))
		Tcl_Panic("Error compiling path: \"%s\"", Tcl_GetString(PATH(ir)));
	Tcl_InitStringRep(obj, Tcl_DStringValue(&ds), Tcl_DStringLength(&ds));
	Tcl_DStringFree(&ds);
}

//>>>

// Internal API <<<
// Internal API >>>
// Stubs API <<<
int Reuri_GetPathFromObj(Tcl_Interp* interp, Tcl_Obj* pathPtr, Tcl_Obj** pathlistPtrPtr) //<<<
{
	int				code = TCL_OK;
	Tcl_ObjIntRep*	ir = Tcl_FetchIntRep(pathPtr, &path_objtype);

	if (ir == NULL) {
		Tcl_ObjIntRep	newir;

		*PATH_PTR(&newir) = NULL;
		code = parse_path(interp, Tcl_GetString(pathPtr), PATH_PTR(&newir));
		if (code != TCL_OK) {
			free_path(&newir);
			goto finally;
		}

		Tcl_FreeIntRep(pathPtr);
		Tcl_StoreIntRep(pathPtr, &path_objtype, &newir);
		ir = Tcl_FetchIntRep(pathPtr, &path_objtype);
	}

	if (pathlistPtrPtr) replace_tclobj(pathlistPtrPtr, PATH(ir));

finally:
	return code;
}

//>>>
int Reuri_CompilePath(Tcl_Interp* interp, Tcl_DString* ds, Tcl_Obj* pathListPtr) //<<<
{
	int			code = TCL_OK;
	Tcl_Obj**	ov = NULL;
	int			oc;
	int			i;

	TEST_OK_LABEL(finally, code, Tcl_ListObjGetElements(interp, pathListPtr, &oc, &ov));

	if (oc == 0)
		return code;

	if (strcmp(Tcl_GetString(ov[1]), "/") != 0)
		percent_encode_ds(REURI_ENCODE_PATH, ds, Tcl_GetString(ov[0]));

	for (i=1; i<oc; i++) {
		Tcl_DStringAppend(ds, "/", 1);
		percent_encode_ds(REURI_ENCODE_PATH, ds, Tcl_GetString(ov[i]));
	}

finally:
	return code;
}

//>>>
// Stubs API >>>

// vim: foldmethod=marker foldmarker=<<<,>>> ts=4 shiftwidth=4

