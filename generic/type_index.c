#include "reuriInt.h"
#include "tip445.h"

/* Intrep:
 *	ptrAndLongRep.ptr:		struct parse_idx_cx*
 *	ptrAndLongRep.value:	unused
 */

#define INDEX(irPtr)			((struct parse_idx_cx*)((irPtr)->ptrAndLongRep.ptr))
#define INDEX_PTR(irPtr)		((struct parse_idx_cx**)(&((irPtr)->ptrAndLongRep.ptr)))

void Reuri_DStringAppendIndex(Tcl_DString* ds, struct parse_idx_cx* idx);

static void free_internal_rep(Tcl_Obj* obj);
static void update_string_rep(Tcl_Obj* obj);

Tcl_ObjType index_objtype = {
	"ReuriIndex",
	free_internal_rep,
	NULL,
	update_string_rep,
	NULL
};

static void free_internal_rep(Tcl_Obj* obj) //<<<
{
	Tcl_ObjIntRep*		ir = Tcl_FetchIntRep(obj, &index_objtype);

	if (ir)
		free_parse_idx_cx(INDEX_PTR(ir));
}

///>>>
void _dstring_append_index(Tcl_DString* ds, struct idx_atom* index) //<<<
{
	char		buf[3*sizeof(index->val)+2];
	const int	wrote = sprintf(buf, "%d", index->val);

	switch (index->type) {
		case IDX_ABS:											break;
		case IDX_ENDREL:	Tcl_DStringAppend(ds, "end", 3);	break;
		default:
			Tcl_Panic("Invalid index type: %d", index->type);
	}

	Tcl_DStringAppend(ds, buf, wrote);
}

//>>>
void _dstring_append_indexrange(Tcl_DString* ds, struct idx_range* range) //<<<
{
	_dstring_append_index(ds, &range->from);
	if (range->to.type != IDX_NONE) {
		Tcl_DStringAppend(ds, "..", 2);
		_dstring_append_index(ds, &range->to);
	}
}

//>>>
static void update_string_rep(Tcl_Obj* obj) //<<<
{
	Tcl_ObjIntRep*		ir = Tcl_FetchIntRep(obj, &index_objtype);
	struct parse_idx_cx*	idx = INDEX(ir);
	Tcl_DString			ds;

	Tcl_DStringInit(&ds);
	Reuri_DStringAppendIndex(&ds, idx);
	Tcl_InitStringRep(obj, Tcl_DStringValue(&ds), Tcl_DStringLength(&ds));
	Tcl_DStringFree(&ds);
}

//>>>

// Internal API <<<
void init_set(struct parse_idx_cx* cx) //<<<
{
	cx->set.size = sizeof(cx->static_ranges) / sizeof(struct idx_range);
	cx->set.top = 0;
	cx->set.range = (struct idx_range*)&cx->static_ranges;
}

//>>>
struct parse_idx_cx* new_parse_idx_cx(const char* str) //<<<
{
	struct parse_idx_cx*	cx = malloc(sizeof *cx);

	cx->str = str;
	cx->p = (const unsigned char*)cx->str;
	cx->rc = IDX_PARSE_OK;
	init_set(cx);

	return cx;
}

//>>>
void throw(struct parse_idx_cx* cx, const char* failtype, size_t failofs) //<<<
{
	cx->rc			= IDX_PARSE_ERROR;
	cx->failofs		= failofs;
	cx->failtype	= failtype;
	longjmp(cx->exception, cx->rc);
}

//>>>
void push_range(struct parse_idx_cx* cx, struct idx_range* r) //<<<
{
	struct idx_set*	set = &cx->set;

	if (set->size <= set->top) {
		int		newsize = set->size * 2;

		if (set->range == (struct idx_range*)&cx->static_ranges) {
			struct idx_range*	newrange = malloc(sizeof(struct idx_range) * newsize);
			memcpy(newrange, set->range, set->size * sizeof(struct idx_range));
			set->range = newrange;
		} else {
			struct idx_range*	newmem = NULL;

			newmem = realloc(set->range, sizeof(struct idx_range) * newsize);
			if (newmem == NULL) 
				throw(cx, "NOMEM", 0);
			set->range = newmem;
		}
		set->size = newsize;
	}

	set->range[set->top++] = *r;
}

//>>>
void free_set(struct parse_idx_cx* cx) //<<<
{
	if (cx->set.range && cx->set.range != (struct idx_range*)&cx->static_ranges) {
		free(cx->set.range);
	}
	init_set(cx);
}

//>>>
void free_parse_idx_cx(struct parse_idx_cx** cx) //<<<
{
	if (*cx) {
		(*cx)->str = NULL;
		(*cx)->p = NULL;
		free_set(*cx);
		free(*cx);
		*cx = NULL;
	}
}

//>>>
void Reuri_DStringAppendIndex(Tcl_DString* ds, struct parse_idx_cx* idx) //<<<
{
	int		i;

	if (idx->set.top == 0) return;

	_dstring_append_indexrange(ds, &idx->set.range[0]);
	for (i=1; i<idx->set.top; i++) {
		Tcl_DStringAppend(ds, ",", 1);
		_dstring_append_indexrange(ds, &idx->set.range[i]);
	}
}

//>>>
static int ReuriGetIndexFromObj(Tcl_Interp* interp, Tcl_Obj* indexObj, struct parse_idx_cx** index) //<<<
{
	int					code = TCL_OK;
	Tcl_ObjIntRep*		ir = Tcl_FetchIntRep(indexObj, &index_objtype);

	if (ir == NULL) {
		Tcl_ObjIntRep	newir;

		*INDEX_PTR(&newir) = NULL;
		code = parse_index(interp, Tcl_GetString(indexObj), INDEX_PTR(&newir));
		if (code != TCL_OK) {
			free_parse_idx_cx(INDEX_PTR(&newir));
			goto finally;
		}

		Tcl_FreeIntRep(indexObj);
		Tcl_StoreIntRep(indexObj, &index_objtype, &newir);
		ir = Tcl_FetchIntRep(indexObj, &index_objtype);
	}

	*index = INDEX(ir);

finally:
	return code;
}

//>>>
// Internal API >>>
// Stubs API <<<
static int64_t _resolve_index_val(enum idx_type type, int64_t length, int val) //<<<
{
	int64_t	resolved;

	switch (type) {
		case IDX_ABS:
			resolved = val;
			break;

		case IDX_ENDREL:
			resolved = val + length;
			break;

		default:
			Tcl_Panic("Invalid index type: %d", type);
			return 0;	// Unreachable, pacify compiler
	}

	return resolved;
}

//>>>
int Reuri_ResolveIndex(Tcl_Interp* interp, Tcl_Obj* indexObj, size_t length, Tcl_Obj** elementsPtrPtr) //<<<
{
	int						code = TCL_OK;
	//struct interp_cx*		l = Tcl_GetAssocData(interp, "reuri", NULL);
	struct interp_cx*		l = NULL;
	Tcl_Obj*				res = NULL;
	int						i;
	struct parse_idx_cx*	index = NULL;
	//Tcl_ObjIntRep*			int_ir = Tcl_FetchIntRep(indexObj, l->typeInt);
	Tcl_ObjIntRep*			int_ir = NULL;

	TIME("Retrieve interp_cx from assoc data",
	l = Tcl_GetAssocData(interp, "reuri", NULL);
	);

	replace_tclobj(&res, Tcl_NewListObj(0, NULL));

	// First check if the indexObj is a native integer, in which case don't shimmer it to our intrep
	TIME("Check for int intrep",
	int_ir = Tcl_FetchIntRep(indexObj, l->typeInt);
	);
	if (int_ir) {
		fprintf(stderr, "Using native int directly: %s\n", Tcl_GetString(indexObj));
		TEST_OK_LABEL(finally, code, Tcl_ListObjAppendElement(interp, res, indexObj));
		goto done;
	}

	TEST_OK_LABEL(finally, code, ReuriGetIndexFromObj(interp, indexObj, &index));

	for (i=0; i<index->set.top; i++) {
		int64_t		from = _resolve_index_val(index->set.range[i].from.type, length, index->set.range[i].from.val);
		int64_t		to   = from;
		int64_t		r;

		if (index->set.range[i].to.type != IDX_NONE)
			to = _resolve_index_val(index->set.range[i].to.type, length, index->set.range[i].to.val);

		if (to >= from) {
			for (r=from; r<=to; r++)
				TEST_OK_LABEL(finally, code, Tcl_ListObjAppendElement(interp, res, Tcl_NewIntObj(r)));
		} else {
			for (r=from; r>=to; r--)
				TEST_OK_LABEL(finally, code, Tcl_ListObjAppendElement(interp, res, Tcl_NewIntObj(r)));
		}
	}

done:
	replace_tclobj(elementsPtrPtr, res);

finally:
	replace_tclobj(&res, NULL);
	return code;
}

//>>>
// Stubs API >>>

// vim: foldmethod=marker foldmarker=<<<,>>> ts=4 shiftwidth=4

