#include "reuriInt.h"
#include "tip445.h"

/* Intrep:
 *	twoPtrValue.ptr1:	Tcl list with param -> value pairs
 *	twoPtrValue.ptr2:	Tcl dict mapping parameter names to a list of
 *						positions that those params occur.  For a=x&b=y&a=z
 *						a -> {0 2}, b -> 1
 */

#define PARAMS(irPtr)		((Tcl_Obj*)((irPtr)->twoPtrValue.ptr1))
#define INDEX(irPtr)		((Tcl_Obj*)((irPtr)->twoPtrValue.ptr2))
#define PARAMS_PTR(irPtr)	((Tcl_Obj**)(&(irPtr)->twoPtrValue.ptr1))
#define INDEX_PTR(irPtr)	((Tcl_Obj**)(&(irPtr)->twoPtrValue.ptr2))

static void free_internal_rep(Tcl_Obj* obj);
static void dup_internal_rep(Tcl_Obj* src, Tcl_Obj* dup);
static void update_string_rep(Tcl_Obj* obj);

Tcl_ObjType query_objtype = {
	"ReuriQuery",
	free_internal_rep,
	dup_internal_rep,
	update_string_rep,
	NULL
};

static void free_query(Tcl_ObjInternalRep* ir) //<<<
{
	if (ir) {
		replace_tclobj(PARAMS_PTR(ir), NULL);
		replace_tclobj(INDEX_PTR(ir),  NULL);
	}
}

//>>>
static void free_internal_rep(Tcl_Obj* obj) //<<<
{
	Tcl_ObjInternalRep*	ir = Tcl_FetchInternalRep(obj, &query_objtype);

	free_query(ir);
}

///>>>
static void dup_internal_rep(Tcl_Obj* src, Tcl_Obj* dup) //<<<
{
	Tcl_ObjInternalRep*	ir = Tcl_FetchInternalRep(src, &query_objtype);
	Tcl_ObjInternalRep	newir;

	newir.twoPtrValue.ptr1 = NULL;
	newir.twoPtrValue.ptr2 = NULL;
	replace_tclobj(PARAMS_PTR(&newir), PARAMS(ir));
	replace_tclobj(INDEX_PTR(&newir),  INDEX(ir));
	Tcl_StoreInternalRep(dup, &query_objtype, &newir);
}

//>>>
static void update_string_rep(Tcl_Obj* obj) //<<<
{
	Tcl_ObjInternalRep*	ir = Tcl_FetchInternalRep(obj, &query_objtype);
	Tcl_DString			ds;

	Tcl_DStringInit(&ds);
	if (TCL_OK != Reuri_CompileQuery(NULL, &ds, PARAMS(ir)))
		Tcl_Panic("Error compiling query params: \"%s\"", Tcl_GetString(PARAMS(ir)));
	Tcl_InitStringRep(obj, Tcl_DStringValue(&ds), Tcl_DStringLength(&ds));
	Tcl_DStringFree(&ds);
}

//>>>

// Internal API <<<
int ReuriGetQueryFromObj(Tcl_Interp* interp, Tcl_Obj* query, Tcl_Obj** params, Tcl_Obj** index) //<<<
{
	int					code = TCL_OK;
	Tcl_ObjInternalRep*	ir = Tcl_FetchInternalRep(query, &query_objtype);

	if (ir == NULL) {
		Tcl_ObjInternalRep	newir;

		newir.twoPtrValue.ptr1 = NULL;
		newir.twoPtrValue.ptr2 = NULL;
		code = parse_query(interp, Tcl_GetString(query), PARAMS_PTR(&newir), INDEX_PTR(&newir));
		if (code != TCL_OK) {
			free_query(&newir);
			goto finally;
		}

		Tcl_StoreInternalRep(query, &query_objtype, &newir);
		ir = Tcl_FetchInternalRep(query, &query_objtype);
	}

	if (params) replace_tclobj(params, PARAMS(ir));
	if (index)  replace_tclobj(index,  INDEX(ir));

finally:
	return code;
}

//>>>
int query_add_index(Tcl_Interp* interp, Tcl_Obj* index, Tcl_Obj* name, const int pnum) //<<<
{
	int				code = TCL_OK;
	Tcl_Obj*		index_vals = NULL;

	// Get an unshared ref to the index values
	TEST_OK_LABEL(finally, code, Tcl_DictObjGet(interp, index, name, &index_vals));
	if (index_vals == NULL) {
		index_vals = Tcl_NewListObj(0, NULL);
		TEST_OK_LABEL(finally, code, Tcl_DictObjPut(interp, index, name, index_vals));
	} else if (Tcl_IsShared(index_vals)) {
		index_vals = Tcl_DuplicateObj(index_vals);
		TEST_OK_LABEL(finally, code, Tcl_DictObjPut(interp, index, name, index_vals));
	}
	// index_vals ref is on loan from the dict

	// Append this pnum to the index values
	TEST_OK_LABEL(finally, code, Tcl_ListObjAppendElement(interp, index_vals, Tcl_NewIntObj(pnum)));
	index_vals = NULL;

finally:
	return code;
}

//>>>
// Internal API >>>
// Stubs API <<<
int Reuri_NewQueryObj(Tcl_Interp* interp, int objc, Tcl_Obj* const objv[], Tcl_Obj** res) //<<<
{
	int					code = TCL_OK;
	Tcl_ObjInternalRep	newir;
	Tcl_Obj*			newobj = NULL;
	Tcl_Obj*			new_params = NULL;
	Tcl_Obj*			new_index = NULL;
	int					pnum;

	if (objc % 2 == 1) {
		struct interp_cx*	l = (struct interp_cx*)Tcl_GetAssocData(interp, "reuri", NULL);
		Tcl_SetErrorCode(interp, "REURI", "UNBALANCED_PARAMS", NULL);
		Tcl_SetObjResult(interp, Dedup_NewStringObj(l->dedup_pool, "Parameter list isn't even", -1));
		code = TCL_ERROR;
		goto finally;
	}

	replace_tclobj(&new_params, Tcl_NewListObj(objc, objv));
	replace_tclobj(&new_index,  Tcl_NewDictObj());

	for (pnum=0; pnum<objc; pnum++)
		TEST_OK_LABEL(finally, code, query_add_index(interp, new_index, objv[pnum], pnum));

	*PARAMS_PTR(&newir) = NULL;
	*INDEX_PTR(&newir) = NULL;

	replace_tclobj(PARAMS_PTR(&newir), new_params);
	replace_tclobj(INDEX_PTR(&newir),  new_index);

	replace_tclobj(&newobj, Tcl_NewObj());
	Tcl_StoreInternalRep(newobj, &query_objtype, &newir);
	Tcl_InvalidateStringRep(newobj);

	replace_tclobj(res, newobj);

finally:
	replace_tclobj(&new_params, NULL);
	replace_tclobj(&new_index,  NULL);
	replace_tclobj(&newobj,     NULL);
	return code;
}

//>>>
int Reuri_CompileQuery(Tcl_Interp* interp, Tcl_DString* ds, Tcl_Obj* params) //<<<
{
	int			code = TCL_OK;
	Tcl_Obj**	pv = NULL;
	int			pc;
	int			i = 0;

	TEST_OK_LABEL(finally, code, Tcl_ListObjGetElements(interp, params, &pc, &pv));
	if (pc % 2 == 1) {
		struct interp_cx*	l = (struct interp_cx*)Tcl_GetAssocData(interp, "reuri", NULL);
		if (interp) {
			Tcl_SetErrorCode(interp, "REURI", "UNBALANCED_PARAMS", NULL);
			Tcl_SetObjResult(interp, Dedup_NewStringObj(l->dedup_pool, "Parameter list isn't even", -1));
		}
		code = TCL_ERROR;
		goto finally;
	}
	if (pc == 0) goto finally;

	Tcl_DStringAppend(ds, "?", 1);
	percent_encode_ds(REURI_ENCODE_QUERY, ds, Tcl_GetString(pv[i++]));
	Tcl_DStringAppend(ds, "=", 1);
	percent_encode_ds(REURI_ENCODE_QUERY, ds, Tcl_GetString(pv[i++]));
	while (i<pc) {
		Tcl_DStringAppend(ds, "&", 1);
		percent_encode_ds(REURI_ENCODE_QUERY, ds, Tcl_GetString(pv[i++]));
		Tcl_DStringAppend(ds, "=", 1);
		percent_encode_ds(REURI_ENCODE_QUERY, ds, Tcl_GetString(pv[i++]));
	}

finally:
	return code;
}

//>>>
// Stubs API >>>

// vim: foldmethod=marker foldmarker=<<<,>>> ts=4 shiftwidth=4

