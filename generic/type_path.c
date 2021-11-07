#include "reuriInt.h"
#include "tip445.h"

/* Intrep:
 *	ptrAndLongRep.ptr:		Tcl list of path elements.
 *	ptrAndLongRep.value:	true if path is absolute.
 */

#define PATH(irPtr)			((Tcl_Obj*)((irPtr)->ptrAndLongRep.ptr))
#define PATH_PTR(irPtr)		((Tcl_Obj**)(&((irPtr)->ptrAndLongRep.ptr)))
#define ABSOLUTE(irPtr)		((irPtr)->ptrAndLongRep.value)
#define ABSOLUTE_PTR(irPtr)	(&((irPtr)->ptrAndLongRep.value))

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
	newir.ptrAndLongRep.value = ABSOLUTE(ir);
	replace_tclobj(PATH_PTR(&newir), PATH(ir));

	Tcl_StoreIntRep(dup, &path_objtype, &newir);
}

//>>>
static void update_string_rep(Tcl_Obj* obj) //<<<
{
	Tcl_ObjIntRep*		ir = Tcl_FetchIntRep(obj, &path_objtype);
	Tcl_DString			ds;

	Tcl_DStringInit(&ds);
	if (TCL_OK != Reuri_CompilePath(NULL, &ds, PATH(ir), ABSOLUTE(ir)))
		Tcl_Panic("Error compiling path: \"%s\"", Tcl_GetString(PATH(ir)));
	Tcl_InitStringRep(obj, Tcl_DStringValue(&ds), Tcl_DStringLength(&ds));
	Tcl_DStringFree(&ds);
}

//>>>

// Internal API <<<
int ReuriGetPathFromObj(Tcl_Interp* interp, Tcl_Obj* path, Tcl_Obj** pathlist) //<<<
{
	int				code = TCL_OK;
	Tcl_ObjIntRep*	ir = Tcl_FetchIntRep(path, &path_objtype);

	if (ir == NULL) {
		Tcl_ObjIntRep	newir;

		*PATH_PTR(&newir) = NULL;
		ABSOLUTE(&newir) = 0;
		code = parse_path(interp, Tcl_GetString(path), PATH_PTR(&newir), ABSOLUTE_PTR(&newir));
		if (code != TCL_OK) {
			free_path(&newir);
			goto finally;
		}

		Tcl_FreeIntRep(path);
		Tcl_StoreIntRep(path, &path_objtype, &newir);
		ir = Tcl_FetchIntRep(path, &path_objtype);
	}

	replace_tclobj(pathlist, PATH(ir));

finally:
	return code;
}

//>>>
// Internal API >>>
// Stubs API <<<
int Reuri_CompilePath(Tcl_Interp* interp, Tcl_DString* ds, Tcl_Obj* pathListPtr, unsigned long absolute) //<<<
{
	int			code = TCL_OK;
	Tcl_Obj**	ov = NULL;
	int			oc;
	int			i;

	TEST_OK_LABEL(finally, code, Tcl_ListObjGetElements(interp, pathListPtr, &oc, &ov));

	if (absolute)
		Tcl_DStringAppend(ds, "/", 1);

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

