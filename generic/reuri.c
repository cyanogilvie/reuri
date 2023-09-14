#include "reuriInt.h"

// Internal API <<<

// Must be kept in sync with reuri_part
const char*	reuri_part_str[] = {
	"scheme",
	"userinfo",
	"host",
	"hosttype",
	"port",
	"path",
	"query",
	"fragment",
	NULL				// NULL here so we can point Tcl_GetIndexFromObj at this
};

const char* reuri_encode_mode_str[] = {
	"query",
	"queryval",
	"path",
	"path2",
	"host",
	"userinfo",
	"fragment",
	"awssig",
	NULL				// NULL here so we can point Tcl_GetIndexFromObj at this
};

// Must be kept in sync with reuri_hosttype
const char* reuri_hosttype_str[] = {
	"none",
	"ipv6",			// IPv6 literal address: [::1]
	"ipv4",			// IPv4 literal address: 127.0.0.1
	"hostname",		// hostname: localhost
	"local",		// unix domain socket path: /tmp/myserv.80
	NULL
};

#if 0
/*
 * Thread string dedup pool.  Incompatible with supporting unloading: when the
 * library is unloaded from its last interp the library will be detached from
 * the process, leaving the Tcl_ThreadExitHandler pointing to a now invalid
 * thread_dedup_pool_cleanup address.
 */
Tcl_ThreadDataKey	thread_string_pool;

static void thread_dedup_pool_cleanup(ClientData cdata) //<<<
{
	struct dedup_pool*	pool = (struct dedup_pool*)cdata;

	Dedup_FreePool(pool);
	pool = NULL;
}

//>>>
struct dedup_pool* thread_dedup_pool() //<<<
{
	struct dedup_pool*	pool = Tcl_GetThreadData(&thread_string_pool, sizeof(struct dedup_pool));

	if (pool == NULL) {
		pool = Dedup_NewPool(interp);
		Tcl_CreateThreadExitHandler(thread_dedup_pool_cleanup, pool);
	}

	return pool;
}

//>>>
Tcl_Obj* thread_string(const char* bytes, int length) //<<<
{
	return Dedup_NewStringObj(thread_dedup_pool, bytes, length);
}

//>>>
#endif

static void free_interp_cx(ClientData cdata, Tcl_Interp* interp) //<<<
{
	struct interp_cx*	l = (struct interp_cx*)cdata;
	int					i;

	if (l) {
		if (l->dedup_pool)			{ Dedup_FreePool(l->dedup_pool);			l->dedup_pool = NULL; }

		if (l->dedup_scheme)		{ Dedup_FreePool(l->dedup_scheme);			l->dedup_scheme = NULL; }
		if (l->dedup_userinfo)		{ Dedup_FreePool(l->dedup_userinfo);		l->dedup_userinfo = NULL; }
		if (l->dedup_host_reg_name) { Dedup_FreePool(l->dedup_host_reg_name);	l->dedup_host_reg_name = NULL; }
		if (l->dedup_host_ipv4)		{ Dedup_FreePool(l->dedup_host_ipv4);		l->dedup_host_ipv4 = NULL; }
		if (l->dedup_host_ipv6)		{ Dedup_FreePool(l->dedup_host_ipv6);		l->dedup_host_ipv6 = NULL; }
		if (l->dedup_host_local)	{ Dedup_FreePool(l->dedup_host_local);		l->dedup_host_local = NULL; }
		if (l->dedup_port)			{ Dedup_FreePool(l->dedup_port);			l->dedup_port = NULL; }
		if (l->dedup_path)			{ Dedup_FreePool(l->dedup_path);			l->dedup_path = NULL; }
		if (l->dedup_query)			{ Dedup_FreePool(l->dedup_query);			l->dedup_query = NULL; }
		if (l->dedup_fragment)		{ Dedup_FreePool(l->dedup_fragment);		l->dedup_fragment = NULL; }

		for (i=0; reuri_hosttype_str[i]; i++)
			replace_tclobj(&l->hosttype[i], NULL);

		replace_tclobj(&l->empty,		NULL);
		replace_tclobj(&l->empty_list,	NULL);
		replace_tclobj(&l->empty_query,	NULL);
		replace_tclobj(&l->t,			NULL);
		replace_tclobj(&l->f,			NULL);
		replace_tclobj(&l->apply,		NULL);
		replace_tclobj(&l->sort_unique,	NULL);

		ckfree(l);
		l = NULL;
	}
}

//>>>
int ReuriGetPartFromObj(Tcl_Interp* interp, Tcl_Obj* partObj, enum reuri_part* part) //<<<
{
	int			ipart, code;
	code = Tcl_GetIndexFromObj(interp, partObj, reuri_part_str, "part", TCL_EXACT, &ipart);
	*part = ipart;
	return code;
}

//>>>
static int query_get(Tcl_Interp* interp, int objc, Tcl_Obj*const objv[], int objc_used, Tcl_Obj* queryObj, const char* srcparam, Tcl_Obj** out) //<<<
{
	int				code = TCL_OK;
	Tcl_Obj*		res = NULL;
	Tcl_Obj*		query = NULL;
	Tcl_Obj*		index = NULL;
	Tcl_Obj*		idxlist = NULL;
	Tcl_Obj*		def = NULL;
	Tcl_Obj*		idxobj = NULL;
	Tcl_Obj*		allvals = NULL;
	Tcl_Obj**		idxv = NULL;
	int				idxc;
	Tcl_Obj**		qv = NULL;
	int				qc;

	const int		A_cmd	= objc_used;
	const int		A_PARAM	= A_cmd+1;
	const int		A_objc	= A_PARAM+1;

	if (objc < A_PARAM) {
		static char	usage[256];
		snprintf(usage, 256, "%s ?param ?-default defaultval??", srcparam);
		CHECK_ARGS_LABEL(finally, code, usage);
	}

	TEST_OK_LABEL(finally, code, ReuriGetQueryFromObj(interp, queryObj, &query, &index));

	if (objc > A_objc) {
		static const char* opts[] = {
			"-default",
			"-index",
			NULL
		};
		enum {
			OPT_DEFAULT,
			OPT_INDEX,
			OPT_END
		};
		int	opt_i = A_objc;
		int opt;

		while (opt_i < objc) {
			TEST_OK_LABEL(finally, code, Tcl_GetIndexFromObj(interp, objv[opt_i++], opts, "option", TCL_EXACT, &opt));
			#define use_next(to) \
				do { \
					if (opt_i >= objc) { \
						Tcl_SetErrorCode(interp, "TCL", "ARGUMENT", "MISSING", NULL); \
						THROW_ERROR_LABEL(finally, code, "missing argument to \"", Tcl_GetString(objv[opt_i-1]), "\""); \
					} \
					replace_tclobj(&(to), objv[opt_i++]); \
				} while(0);

			switch (opt) {
				case OPT_DEFAULT:	use_next(def);		break;
				case OPT_INDEX:		use_next(idxobj);	break;
				default: THROW_ERROR_LABEL(finally, code, "Unhandled query get option case");
			}
			#undef use_next
		}
	}

	if (objc <= A_PARAM) {
		// No param specified, return everything
		replace_tclobj(out, query);
		goto finally;
	}

	TEST_OK_LABEL(finally, code, Tcl_ListObjGetElements(interp, query, &qc, &qv));
	TEST_OK_LABEL(finally, code, Tcl_DictObjGet(interp, index, objv[A_PARAM], &idxlist));
	if (idxlist) {
		int idx;
		Tcl_IncrRefCount(idxlist);
		TEST_OK_LABEL(finally, code, Tcl_ListObjGetElements(interp, idxlist, &idxc, &idxv));
		TEST_OK_LABEL(finally, code, Tcl_GetIntFromObj(interp, idxv[idxc-1], &idx));

		if (idxobj) {
			// TODO: maybe optimize common cases like "0", "end" to just get that value without building allvals
			replace_tclobj(&allvals, Tcl_NewListObj(idxc, NULL));
			for (int i=0; i<idxc; i++) {
				TEST_OK_LABEL(finally, code, Tcl_GetIntFromObj(interp, idxv[i], &idx));
				TEST_OK_LABEL(finally, code, Tcl_ListObjAppendElement(interp, allvals, qv[idx*2+1]));
			}
			TEST_OK_LABEL(finally, code, Idx_PickFromList(interp, allvals, idxobj, &res));
		} else {
			replace_tclobj(&res, qv[idx*2+1]);
		}

		replace_tclobj(out, res);
	} else if (idxobj) {
		struct parse_idx_cx*	index;

		TEST_OK_LABEL(finally, code, IdxGetIndexFromObj(interp, idxobj, &index));

		replace_tclobj(&res, Tcl_NewListObj(0, NULL));

		// If index is a single, apply the default rules
		if (index->set.size == 1 && index->set.range[0].to.type != IDX_NONE) goto try_default;

		for (int i=0; i<index->set.size; i++) {
			if (index->set.range[i].to.type != IDX_NONE) continue;	// For ranges, emit nothing
			if (def) {
				TEST_OK_LABEL(finally, code, Tcl_ListObjAppendElement(interp, res, def));
			} else {
				Tcl_SetErrorCode(interp, "REURI", "PARAM_NOT_SET", Tcl_GetString(objv[A_PARAM]), NULL);
				THROW_ERROR_LABEL(finally, code, "param \"", Tcl_GetString(objv[A_PARAM]), "\" doesn't exist");
			}
		}
		replace_tclobj(out, res);
	} else {
		goto try_default;
	}

finally:
	replace_tclobj(&res, NULL);
	replace_tclobj(&query, NULL);
	replace_tclobj(&index, NULL);
	replace_tclobj(&idxlist, NULL);
	replace_tclobj(&def, NULL);
	replace_tclobj(&idxobj, NULL);
	replace_tclobj(&allvals, NULL);
	return code;

try_default:
	if (def) {
		replace_tclobj(out, def);
	} else {
		Tcl_SetErrorCode(interp, "REURI", "PARAM_NOT_SET", Tcl_GetString(objv[A_PARAM]), NULL);
		THROW_ERROR_LABEL(finally, code, "param \"", Tcl_GetString(objv[A_PARAM]), "\" doesn't exist");
	}
	goto finally;
}

//>>>
static int query_values(Tcl_Interp* interp, Tcl_Obj* queryObj, Tcl_Obj* param, Tcl_Obj** out) //<<<
{
	int					code = TCL_OK;
	struct interp_cx*	l = Tcl_GetAssocData(interp, "reuri", NULL);
	Tcl_Obj*			res = NULL;
	Tcl_Obj*			query = NULL;
	Tcl_Obj*			index = NULL;
	Tcl_Obj*			idxlist = NULL;
	Tcl_Obj**			idxv = NULL;
	int					idxc;
	Tcl_Obj**			qv = NULL;
	int					qc;

	TEST_OK_LABEL(finally, code, ReuriGetQueryFromObj(interp, queryObj, &query, &index));
	TEST_OK_LABEL(finally, code, Tcl_ListObjGetElements(interp, query, &qc, &qv));
	TEST_OK_LABEL(finally, code, Tcl_DictObjGet(interp, index, param, &idxlist));
	if (idxlist) {
		TEST_OK_LABEL(finally, code, Tcl_ListObjGetElements(interp, idxlist, &idxc, &idxv));

		replace_tclobj(&res, Tcl_NewListObj(idxc, NULL));
		for (int i=0; i<idxc; i++) {
			int idx;
			TEST_OK_LABEL(finally, code, Tcl_GetIntFromObj(interp, idxv[i], &idx));
			TEST_OK_LABEL(finally, code, Tcl_ListObjAppendElement(interp, res, qv[idx*2+1]));
		}
		replace_tclobj(out, res);
	} else {
		replace_tclobj(out, l->empty_list);
	}

finally:
	replace_tclobj(&res, NULL);
	replace_tclobj(&query, NULL);
	replace_tclobj(&index, NULL);
	// idxlist is on loan from the index dictionary
	return code;
}

//>>>
static int query_names(Tcl_Interp* interp, Tcl_Obj* queryObj, Tcl_Obj** out) //<<<
{
	int					code = TCL_OK;
	struct interp_cx*	l = Tcl_GetAssocData(interp, "reuri", NULL);
	Tcl_Obj*			res = NULL;
	Tcl_Obj*			query = NULL;
	Tcl_Obj*			names = NULL;
	Tcl_Obj**			qv = NULL;
	int					qc;

	TEST_OK_LABEL(finally, code, ReuriGetQueryFromObj(interp, queryObj, &query, NULL));
	TEST_OK_LABEL(finally, code, Tcl_ListObjGetElements(interp, query, &qc, &qv));
	if (qc > 0) {
		replace_tclobj(&res, Tcl_NewListObj(qc, NULL));
		for (int i=0; i<qc; i+=2)
			TEST_OK_LABEL(finally, code, Tcl_ListObjAppendElement(interp, res, qv[i]));
	} else {
		replace_tclobj(&res, l->empty_list);
	}
	replace_tclobj(out, res);

finally:
	replace_tclobj(&res, NULL);
	replace_tclobj(&query, NULL);
	replace_tclobj(&names, NULL);
	return code;
}

//>>>
static int query_add(Tcl_Interp* interp, Tcl_Obj* queryObj, Tcl_Obj* param, Tcl_Obj* value, Tcl_Obj** out) //<<<
{
	int					code = TCL_OK;
	Tcl_Obj*			res = NULL;
	Tcl_Obj*			params = NULL;
	Tcl_Obj*			index = NULL;
	Tcl_Obj*			idxlist = NULL;
	Tcl_Obj*			lidxlist = NULL;
	Tcl_Obj*			newidx = NULL;

	if (queryObj == NULL) {
		Tcl_Obj*	ov[2] = {param, value};
		TEST_OK_LABEL(finally, code, Reuri_NewQueryObj(interp, 2, ov, &res));
		replace_tclobj(out, res);
		goto finally;
	}

	replace_tclobj(&res, Tcl_IsShared(queryObj) ? Tcl_DuplicateObj(queryObj) : queryObj);

	TEST_OK_LABEL(finally, code, ReuriGetQueryFromObj(interp, res, &params, &index));
	replace_tclobj(&params, Tcl_DuplicateObj(params));
	replace_tclobj(&index,  Tcl_DuplicateObj(index));
	// params and index are unshared

	int last;
	TEST_OK_LABEL(finally, code, Tcl_ListObjLength(interp, params, &last));
	replace_tclobj(&newidx, Tcl_NewIntObj(last/2));

	// Append this instance of param
	TEST_OK_LABEL(finally, code, Tcl_ListObjAppendElement(interp, params, param));
	TEST_OK_LABEL(finally, code, Tcl_ListObjAppendElement(interp, params, value));

	// Record this instance of param in the index
	TEST_OK_LABEL(finally, code, Tcl_DictObjGet(interp, index, param, &idxlist));
	if (!idxlist || Tcl_IsShared(idxlist)) {
		replace_tclobj(&lidxlist, idxlist ? Tcl_DuplicateObj(idxlist) : Tcl_NewListObj(1, NULL));
		idxlist = lidxlist;
	}
	TEST_OK_LABEL(finally, code, Tcl_ListObjAppendElement(interp, idxlist, newidx));
	TEST_OK_LABEL(finally, code, Tcl_DictObjPut(interp, index, param, idxlist));

	ReuriSetQuery(res, params, index);

	replace_tclobj(out, res);

finally:
	replace_tclobj(&params, NULL);
	replace_tclobj(&index, NULL);
	// idxlist is on loan from the index dict
	replace_tclobj(&lidxlist, NULL);
	replace_tclobj(&newidx, NULL);
	replace_tclobj(&res, NULL);
	return code;
}

//>>>
static int query_unset(Tcl_Interp* interp, Tcl_Obj* queryObj, int paramc, Tcl_Obj*const paramv[], Tcl_Obj** out) //<<<
{
	int					code = TCL_OK;
	struct interp_cx*	l = Tcl_GetAssocData(interp, "reuri", NULL);
	Tcl_Obj*			res = NULL;
	Tcl_Obj*			params = NULL;
	Tcl_Obj*			index = NULL;
	Tcl_Obj*			hitlist = NULL;
	Tcl_Obj*			sortcmdv[3] = {l->apply, l->sort_unique};

	if (
			queryObj == NULL ||	// Removing anything from an empty query is an invariant
			paramc == 0			// Do Nothing Gracefully
	) {
		replace_tclobj(out, queryObj ? queryObj : l->empty_query);
		goto finally;
	}

	replace_tclobj(&res, Tcl_IsShared(queryObj) ? Tcl_DuplicateObj(queryObj) : queryObj);

	TEST_OK_LABEL(finally, code, ReuriGetQueryFromObj(interp, res, &params, &index));
	replace_tclobj(&params, Tcl_DuplicateObj(params));
	replace_tclobj(&index,  Tcl_DuplicateObj(index));
	// params and index are unshared

	// Gather the indices for all instances of all the params we're unsetting
	replace_tclobj(&hitlist, Tcl_NewListObj(paramc, NULL));	// Guess that we will have an average of one idx per param
	for (int p=0; p<paramc; p++) {
		int			len;
		Tcl_Obj*	idxlist = NULL;
		Tcl_Obj**	idxv = NULL;
		int			idxc;

		TEST_OK_LABEL(finally, code, Tcl_ListObjLength(interp, hitlist, &len));	// TODO: keep track of this directly instead?
		TEST_OK_LABEL(finally, code, Tcl_DictObjGet(interp, index, paramv[p], &idxlist));
		if (!idxlist) continue;
		TEST_OK_LABEL(finally, code, Tcl_ListObjGetElements(interp, idxlist, &idxc, &idxv));
		TEST_OK_LABEL(finally, code, Tcl_ListObjReplace(interp, hitlist, len, 0, idxc, idxv));
	}

	// Sort unique in descending order as integers: [lsort -integer -unique -decreasing]
	sortcmdv[2] = hitlist;
	TEST_OK_LABEL(finally, code, Tcl_EvalObjv(interp, 3, sortcmdv, TCL_EVAL_GLOBAL));
	replace_tclobj(&hitlist, Tcl_GetObjResult(interp));
	Tcl_ResetResult(interp);		// Necessary?

	// Remove the elements from the params list
	Tcl_Obj**	hitv = NULL;
	int			hitc;
	TEST_OK_LABEL(finally, code, Tcl_ListObjGetElements(interp, hitlist, &hitc, &hitv));
	for (int i=0; i<hitc; i++) {
		int		idx;
		TEST_OK_LABEL(finally, code, Tcl_GetIntFromObj(interp, hitv[i], &idx));
		TEST_OK_LABEL(finally, code, Tcl_ListObjReplace(interp, params, idx*2, 2, 0, NULL));
	}

	ReuriSetQuery(res, params, NULL);

	replace_tclobj(out, res);

finally:
	replace_tclobj(&params, NULL);
	replace_tclobj(&index, NULL);
	replace_tclobj(&hitlist, NULL);
	replace_tclobj(&res, NULL);
	return code;
}

//>>>
static int query_set(Tcl_Interp* interp, Tcl_Obj* queryObj, Tcl_Obj* param, Tcl_Obj* value, Tcl_Obj** out) //<<<
{
	int					code = TCL_OK;
	struct interp_cx*	l = Tcl_GetAssocData(interp, "reuri", NULL);
	Tcl_Obj*			res = NULL;
	Tcl_Obj*			params = NULL;
	Tcl_Obj*			index = NULL;
	Tcl_Obj*			lidxlist = NULL;
	Tcl_Obj*			newidx = NULL;
	Tcl_Obj*			sortcmdv[3] = {l->apply, l->sort_unique};

	if (queryObj == NULL) {
		Tcl_Obj*	ov[2] = {param, value};
		TEST_OK_LABEL(finally, code, Reuri_NewQueryObj(interp, 2, ov, &res));
		replace_tclobj(out, res);
		goto finally;
	}

	replace_tclobj(&res, Tcl_IsShared(queryObj) ? Tcl_DuplicateObj(queryObj) : queryObj);

	TEST_OK_LABEL(finally, code, ReuriGetQueryFromObj(interp, res, &params, &index));
	replace_tclobj(&params, Tcl_DuplicateObj(params));
	replace_tclobj(&index,  Tcl_DuplicateObj(index));
	// params and index are unshared

	// Gather the indices for all instances of all the params we're unsetting
	Tcl_Obj*	idxlist = NULL;
	Tcl_Obj**	idxv = NULL;
	int			idxc;

	TEST_OK_LABEL(finally, code, Tcl_DictObjGet(interp, index, param, &idxlist));
	if (!idxlist) {
		// param doesn't exist in query yet, append it
		int last;
add:
		TEST_OK_LABEL(finally, code, Tcl_ListObjLength(interp, params, &last));
		replace_tclobj(&newidx, Tcl_NewIntObj(last/2));

		// Append this instance of param
		TEST_OK_LABEL(finally, code, Tcl_ListObjAppendElement(interp, params, param));
		TEST_OK_LABEL(finally, code, Tcl_ListObjAppendElement(interp, params, value));

		// Record this instance of param in the index
		replace_tclobj(&lidxlist, Tcl_NewListObj(1, NULL));
		idxlist = lidxlist;
		TEST_OK_LABEL(finally, code, Tcl_ListObjAppendElement(interp, idxlist, newidx));
		TEST_OK_LABEL(finally, code, Tcl_DictObjPut(interp, index, param, idxlist));
	} else {
		TEST_OK_LABEL(finally, code, Tcl_ListObjGetElements(interp, idxlist, &idxc, &idxv));

		if (idxc == 1) { // Typical case: single existing entry, just replace its value
			int			idx;
			TEST_OK_LABEL(finally, code, Tcl_GetIntFromObj(interp, idxv[0], &idx));
			TEST_OK_LABEL(finally, code, Tcl_ListObjReplace(interp, params, idx*2+1, 1, 1, &value))
		} else if (idxc == 0) {
			goto add;
		} else { // Multiple existing instances, replace the first and remove the later instances
			// Sort unique in descending order as integers: [lsort -integer -unique -decreasing]
			sortcmdv[2] = idxlist;
			TEST_OK_LABEL(finally, code, Tcl_EvalObjv(interp, 3, sortcmdv, TCL_EVAL_GLOBAL));
			replace_tclobj(&lidxlist, Tcl_GetObjResult(interp));
			idxlist = lidxlist;
			Tcl_ResetResult(interp);		// Necessary?

			// Remove the elements from the params list
			int			idx;
			TEST_OK_LABEL(finally, code, Tcl_ListObjGetElements(interp, idxlist, &idxc, &idxv));

			// Remove second and subsequent instances
			for (int i=0; i<idxc-1; i++) {
				TEST_OK_LABEL(finally, code, Tcl_GetIntFromObj(interp, idxv[i], &idx));
				TEST_OK_LABEL(finally, code, Tcl_ListObjReplace(interp, params, idx*2, 2, 0, NULL));
			}

			// Replace the first instance's value
			TEST_OK_LABEL(finally, code, Tcl_GetIntFromObj(interp, idxv[idxc-1], &idx));
			TEST_OK_LABEL(finally, code, Tcl_ListObjReplace(interp, params, idx*2+1, 1, 1, &value));

			replace_tclobj(&index, NULL);
		}
	}

	ReuriSetQuery(res, params, index);

	replace_tclobj(out, res);

finally:
	replace_tclobj(&params, NULL);
	replace_tclobj(&index, NULL);
	replace_tclobj(&res, NULL);
	replace_tclobj(&lidxlist, NULL);
	replace_tclobj(&newidx, NULL);
	return code;
}

//>>>
// Internal API >>>
// Stubs API <<<
int Reuri_URIObjGetPart(Tcl_Interp* interp, Tcl_Obj* uriPtr, enum reuri_part part, Tcl_Obj* defaultPtr, Tcl_Obj** valuePtrPtr) //<<<
{
	struct interp_cx*	l = Tcl_GetAssocData(interp, "reuri", NULL);
	int					code = TCL_OK;
	struct uri*			uri = NULL;
	Tcl_Obj*			p = NULL;
	Tcl_Obj*			decoded = NULL;
	Reuri_ObjType*		objtype = NULL;

	TEST_OK_LABEL(finally, code, ReuriGetURIFromObj(interp, uriPtr, &uri));

	switch (part) {
		case REURI_SCHEME:		p = uri->scheme;	objtype = &scheme_objtype;				break;
		case REURI_USERINFO:	p = uri->userinfo;	objtype = &userinfo_objtype;			break;
		case REURI_HOST:		p = uri->host;		objtype = host_objtype(uri->hosttype);	break;
		case REURI_PORT:		p = uri->port;		objtype = &port_objtype;				break;
		case REURI_PATH:		p = uri->path;		objtype = &path_objtype;				break;
		case REURI_QUERY:		p = uri->query;		objtype = &query_objtype;				break;
		case REURI_FRAGMENT:	p = uri->fragment;	objtype = &fragment_objtype;			break;
		case REURI_HOSTTYPE:	p = l->hosttype[uri->hosttype];								break;
		default: THROW_ERROR_LABEL(finally, code, "Invalid part");
	}

	if (p == NULL) {
		if (defaultPtr) {
			replace_tclobj(valuePtrPtr, defaultPtr);
		} else {
			Tcl_SetErrorCode(interp, "REURI", "PART_NOT_SET", reuri_part_str[part], NULL);
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("%s part is not defined in %s",
						reuri_part_str[part], Tcl_GetString(uriPtr)));
			code = TCL_ERROR;
			goto finally;
		}
	} else {
		if (objtype)
			TEST_OK_LABEL(finally, code, Reuri_GetDecodedFromPart(interp, p, objtype, &decoded));

		replace_tclobj(valuePtrPtr, (part == REURI_HOSTTYPE) ? p : decoded);
		p = NULL;
	}

finally:
	replace_tclobj(&decoded, NULL);
	return code;
}

//>>>
int Reuri_URIObjExtractPart(Tcl_Interp* interp, Tcl_Obj* uriPtr, enum reuri_part part, Tcl_Obj* defaultPtr, Tcl_Obj** valuePtrPtr) //<<<
{
	struct interp_cx*	l = Tcl_GetAssocData(interp, "reuri", NULL);
	int					code = TCL_OK;
	struct uri*			uri = NULL;
	Tcl_Obj*			p = NULL;

	TEST_OK_LABEL(finally, code, ReuriGetURIFromObj(interp, uriPtr, &uri));

	switch (part) {
		case REURI_SCHEME:		p = uri->scheme;				break;
		case REURI_USERINFO:	p = uri->userinfo;				break;
		case REURI_HOST:		p = uri->host;					break;
		case REURI_PORT:		p = uri->port;					break;
		case REURI_PATH:		p = uri->path;					break;
		case REURI_QUERY:		p = uri->query;					break;
		case REURI_FRAGMENT:	p = uri->fragment;				break;
		case REURI_HOSTTYPE:	p = l->hosttype[uri->hosttype];	break;
		default: THROW_ERROR_LABEL(finally, code, "Invalid part");
	}

	if (p == NULL) {
		if (defaultPtr) {
			replace_tclobj(valuePtrPtr, defaultPtr);
		} else {
			Tcl_SetErrorCode(interp, "REURI", "PART_NOT_SET", reuri_part_str[part], NULL);
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("%s part is not defined in %s",
						reuri_part_str[part], Tcl_GetString(uriPtr)));
			code = TCL_ERROR;
			goto finally;
		}
	} else {
		replace_tclobj(valuePtrPtr, p);
		p = NULL;
	}

finally:
	return code;
}

//>>>
int Reuri_URIObjPartExists(Tcl_Interp* interp, Tcl_Obj* uriPtr, enum reuri_part part, int* existsPtr) //<<<
{
	struct interp_cx*	l = Tcl_GetAssocData(interp, "reuri", NULL);
	int					code = TCL_OK;
	struct uri*			uri = NULL;
	Tcl_Obj*			res = NULL;

	TEST_OK_LABEL(finally, code, ReuriGetURIFromObj(interp, uriPtr, &uri));

	switch (part) {
		case REURI_SCHEME:		res = uri->scheme;		break;
		case REURI_USERINFO:	res = uri->userinfo;	break;
		case REURI_HOST:		res = uri->host;		break;
		case REURI_PORT:		res = uri->port;		break;
		case REURI_PATH:		res = uri->path;		break;
		case REURI_QUERY:		res = uri->query;		break;
		case REURI_FRAGMENT:	res = uri->fragment;	break;
		case REURI_HOSTTYPE:	res = l->hosttype[uri->hosttype];	break;
		default: THROW_ERROR_LABEL(finally, code, "Invalid part");
	}

	*existsPtr = res != NULL;

	res = NULL;

finally:
	return code;
}

//>>>
int Reuri_URIObjGetAll(Tcl_Interp* interp, Tcl_Obj* uriPtr, Tcl_Obj** res) //<<<
{
	int					code = TCL_OK;
	struct uri*			uri = NULL;
	Tcl_Obj*			d = NULL;
	struct interp_cx*	l = (struct interp_cx*)Tcl_GetAssocData(interp, "reuri", NULL);
	Tcl_Obj*			val = NULL;

	TEST_OK_LABEL(finally, code, ReuriGetURIFromObj(interp, uriPtr, &uri));

	replace_tclobj(&d, Tcl_NewDictObj());

#define ADD_PART(k, v, ot) \
	do { \
		if (v) { \
			if (ot) { \
				Reuri_GetDecodedFromPart(interp, v, ot, &val); \
			} else { \
				replace_tclobj(&val, v); \
			} \
			TEST_OK_LABEL(finally, code, Tcl_DictObjPut(interp, d, \
						Dedup_NewStringObj(l->dedup_pool, k, sizeof(k)-1), val)); \
		} \
	} while(0);

	_Pragma("GCC diagnostic push");
	_Pragma("GCC diagnostic ignored \"-Waddress\"");
	ADD_PART("scheme",    uri->scheme,					&scheme_objtype);
	ADD_PART("userinfo",  uri->userinfo,				&userinfo_objtype);
	ADD_PART("host",      uri->host,					host_objtype(uri->hosttype));
	ADD_PART("hosttype",  l->hosttype[uri->hosttype],	NULL);
	ADD_PART("port",      uri->port,					&port_objtype);
	ADD_PART("path",      uri->path,					&path_objtype);
	ADD_PART("query",     uri->query,					&query_objtype);
	ADD_PART("fragment",  uri->fragment,				&fragment_objtype);
	_Pragma("GCC diagnostic pop");
#undef ADD_PART

	replace_tclobj(res, d);

finally:
	replace_tclobj(&d, NULL);
	replace_tclobj(&val, NULL);
	return code;
}

//>>>
int Reuri_URIObjExtractAll(Tcl_Interp* interp, Tcl_Obj* uriPtr, Tcl_Obj** res) //<<<
{
	int					code = TCL_OK;
	struct uri*			uri = NULL;
	Tcl_Obj*			d = NULL;
	struct interp_cx*	l = (struct interp_cx*)Tcl_GetAssocData(interp, "reuri", NULL);

	TEST_OK_LABEL(finally, code, ReuriGetURIFromObj(interp, uriPtr, &uri));

	replace_tclobj(&d, Tcl_NewDictObj());

#define ADD_PART(k, v) \
	do { \
		if (v) { \
			TEST_OK_LABEL(finally, code, Tcl_DictObjPut(interp, d, \
						Dedup_NewStringObj(l->dedup_pool, k, sizeof(k)-1), v)); \
		} \
	} while(0);

	ADD_PART("scheme",    uri->scheme);
	ADD_PART("userinfo",  uri->userinfo);
	ADD_PART("host",      uri->host);
	ADD_PART("hosttype",  l->hosttype[uri->hosttype]);
	ADD_PART("port",      uri->port);
	ADD_PART("path",      uri->path);
	ADD_PART("query",     uri->query);
	ADD_PART("fragment",  uri->fragment);
#undef ADD_PART

	replace_tclobj(res, d);

finally:
	replace_tclobj(&d, NULL);
	return code;
}

//>>>
int Reuri_URIObjSet(Tcl_Interp* interp, Tcl_Obj* uriPtr, enum reuri_part part, Tcl_Obj* valuePtr, Tcl_Obj** resPtrPtr) //<<<
{
	int					code = TCL_OK;
	struct uri*			uri = NULL;
	Tcl_Obj*			newval = NULL;
	Tcl_Obj*			res = NULL;
	Tcl_Obj**			partPtr = NULL;

	replace_tclobj(&res, uriPtr);

	if (Tcl_IsShared(res))
		replace_tclobj(&res, Tcl_DuplicateObj(res));

	TEST_OK_LABEL(finally, code, ReuriGetURIFromObj(interp, res, &uri));

	enum reuri_hosttype	old_hosttype = uri->hosttype;

	switch (part) {
		case REURI_SCHEME:   partPtr = &uri->scheme;	TEST_OK_LABEL(finally, code, parse_scheme  (interp, valuePtr, &newval)); break;
		case REURI_USERINFO: partPtr = &uri->userinfo;	TEST_OK_LABEL(finally, code, parse_userinfo(interp, valuePtr, &newval)); break;
		case REURI_HOST:     partPtr = &uri->host;		TEST_OK_LABEL(finally, code, parse_host    (interp, valuePtr, &newval, &uri->hosttype)); break;
		case REURI_PORT:     partPtr = &uri->port;		TEST_OK_LABEL(finally, code, parse_port    (interp, valuePtr, &newval)); break;
		case REURI_PATH:     partPtr = &uri->path;		TEST_OK_LABEL(finally, code, parse_path    (interp, valuePtr, &newval)); break;
		case REURI_QUERY:    partPtr = &uri->query;		TEST_OK_LABEL(finally, code, parse_query   (interp, valuePtr, &newval)); break;
		case REURI_FRAGMENT: partPtr = &uri->fragment;	TEST_OK_LABEL(finally, code, parse_fragment(interp, valuePtr, &newval)); break;
			THROW_ERROR_LABEL(finally, code, "Not implemented yet");
		default:
			THROW_ERROR_LABEL(finally, code, "Unhandled part");
	}

	// Check consistency constraints
	switch (part) {
		case REURI_HOST:
			if (uri->hosttype != REURI_HOST_NONE && Reuri_PathType(uri->path) == REURI_PATH_ROOTLESS) {
				uri->hosttype = old_hosttype;
				Tcl_SetErrorCode(interp, "REURI", "CONFLICT", NULL);
				THROW_ERROR_LABEL(finally, code, "Can't add a host: path is rootless");
			}
			break;
		case REURI_PATH:
			if (uri->hosttype != REURI_HOST_NONE && Reuri_PathType(newval) == REURI_PATH_ROOTLESS) {
				Tcl_SetErrorCode(interp, "REURI", "CONFLICT", NULL);
				THROW_ERROR_LABEL(finally, code, "Can't set a rootless path with a host");
			}
			break;
		default: break;
	}

	replace_tclobj(partPtr, newval);
	Tcl_InvalidateStringRep(res);
	replace_tclobj(resPtrPtr, res);

finally:
	replace_tclobj(&newval, NULL);
	replace_tclobj(&res, NULL);
	return code;
}

//>>>
Tcl_Obj* Reuri_PercentEncodeObj(Tcl_Interp* interp, enum reuri_encode_mode mode, Tcl_Obj* objPtr) //<<<
{
	switch (mode) {
		case REURI_ENCODE_QUERY:    return percent_encode(interp, objPtr, REURI_ENCODE_QUERY);
		case REURI_ENCODE_QUERYVAL: return percent_encode(interp, objPtr, REURI_ENCODE_QUERYVAL);
		case REURI_ENCODE_PATH:     return percent_encode(interp, objPtr, REURI_ENCODE_PATH);
		case REURI_ENCODE_PATH2:    return percent_encode(interp, objPtr, REURI_ENCODE_PATH2);
		case REURI_ENCODE_USERINFO: return percent_encode(interp, objPtr, REURI_ENCODE_USERINFO);
		case REURI_ENCODE_HOST:     return percent_encode(interp, objPtr, REURI_ENCODE_HOST);
		case REURI_ENCODE_FRAGMENT: return percent_encode(interp, objPtr, REURI_ENCODE_FRAGMENT);
		case REURI_ENCODE_AWSSIG:	return percent_encode_awssig(interp, objPtr);
		default:					Tcl_Panic("Invalid encode mode: %d", mode); return NULL;
	}
}

//>>>
Tcl_Obj* Reuri_PercentDecodeObj(Tcl_Obj* in) //<<<
{
	Tcl_Obj*	res = NULL;

	percent_decode(in, &res);
	return res;
}

//>>>
// Stubs API >>>
// Script API <<<
static int UriObjCmd(ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj* const objv[]) //<<<
{
	int					code = TCL_OK;
	struct interp_cx*	l = (struct interp_cx*)Tcl_GetAssocData(interp, "reuri", NULL);
	static const char*	methods[] = {
		"get",
		"extract",
		"exists",
		"set",
		"valid",
		"context",
		"resolve",
		"absolute",
		"encode",
		"decode",
		"query",
		"path",
		"normalize",
		NULL
	};
	enum {
		M_GET,
		M_EXTRACT,
		M_EXISTS,
		M_SET,
		M_VALID,
		M_CONTEXT,
		M_RESOLVE,
		M_ABSOLUTE,
		M_ENCODE,
		M_DECODE,
		M_QUERY,
		M_PATH,
		M_NORMALIZE
	};
	int	methodidx;

	enum {A_cmd=0, M_METHOD, A_args};
	CHECK_MIN_ARGS_LABEL(finally, code, "method ?arg ...?");

	TEST_OK_LABEL(finally, code, Tcl_GetIndexFromObj(interp, objv[1], methods, "method", TCL_EXACT, &methodidx));

	switch (methodidx) {
		case M_GET: //<<<
			{
				enum args {
					A_METHOD = 1,
					A_URI,
					A_PART,
					A_DEFAULT
				};
				Tcl_Obj*		def = NULL;
				Tcl_Obj*		res = NULL;
				enum reuri_part	part;

				switch (objc) {
					case 3:	// Return all parts in a dict
						TEST_OK_LABEL(get_finally, code, Reuri_URIObjGetAll(interp, objv[A_URI], &res));
						Tcl_SetObjResult(interp, res);
						replace_tclobj(&res, NULL);
						goto finally;

					case 4:							break;
					case 5:	def = objv[A_DEFAULT];	break;
					default:
						Tcl_WrongNumArgs(interp, 2, objv, "uri part ?default?");
						code = TCL_ERROR;
						goto finally;
				}

				TEST_OK_LABEL(get_finally, code, ReuriGetPartFromObj(interp, objv[A_PART], &part));
				TEST_OK_LABEL(get_finally, code, Reuri_URIObjGetPart(interp, objv[A_URI], part, def, &res));

				Tcl_SetObjResult(interp, res);

			get_finally:
				replace_tclobj(&res, NULL);
			}
			break;
			//>>>
		case M_EXTRACT: //<<<
			{
				enum args {
					A_METHOD = 1,
					A_URI,
					A_PART,
					A_DEFAULT
				};
				Tcl_Obj*		def = NULL;
				Tcl_Obj*		res = NULL;
				enum reuri_part	part;

				switch (objc) {
					case 3:	// Return all parts in a dict
						TEST_OK_LABEL(extract_finally, code, Reuri_URIObjExtractAll(interp, objv[A_URI], &res));
						Tcl_SetObjResult(interp, res);
						replace_tclobj(&res, NULL);
						goto finally;

					case 4:							break;
					case 5:	def = objv[A_DEFAULT];	break;
					default:
						Tcl_WrongNumArgs(interp, 2, objv, "uri part ?default?");
						code = TCL_ERROR;
						goto finally;
				}

				TEST_OK_LABEL(extract_finally, code, ReuriGetPartFromObj(interp, objv[A_PART], &part));
				TEST_OK_LABEL(extract_finally, code, Reuri_URIObjExtractPart(interp, objv[A_URI], part, def, &res));

				Tcl_SetObjResult(interp, res);

			extract_finally:
				replace_tclobj(&res, NULL);
			}
			break;
			//>>>
		case M_EXISTS: //<<<
			{
				int				exists;
				enum reuri_part	part;

				enum {A_cmd=1, A_URI, A_PART, A_objc};
				CHECK_ARGS_LABEL(finally, code, "uri part");

				TEST_OK_LABEL(finally, code, ReuriGetPartFromObj(interp, objv[A_PART], &part));
				TEST_OK_LABEL(finally, code, Reuri_URIObjPartExists(interp, objv[A_URI], part, &exists));
				Tcl_SetObjResult(interp, exists ? l->t : l->f);
			}
			break;
			//>>>
		case M_SET: //<<<
			{
				enum reuri_part	part;
                Tcl_Obj*        res = NULL;
				Tcl_Obj*		uri = NULL;

				enum {A_cmd=1, A_URI, A_PART, A_VALUE, A_objc};
				CHECK_ARGS_LABEL(finally, code, "uri part value");

				struct uri*	uri_ir = NULL;
				uri = Tcl_ObjGetVar2(interp, objv[A_URI], NULL, 0);
				if (!uri) replace_tclobj(&uri, Tcl_NewObj());
				TEST_OK_LABEL(query_finally, code, ReuriGetURIFromObj(interp, uri, &uri_ir));
				TEST_OK_LABEL(finally, code, ReuriGetPartFromObj(interp, objv[A_PART], &part));
				TEST_OK_LABEL(finally, code, Reuri_URIObjSet(interp, uri, part, objv[A_VALUE], &res));
				replace_tclobj(&res, Tcl_ObjSetVar2(interp, objv[A_URI], NULL, res, TCL_LEAVE_ERR_MSG));
				if (res == NULL) {
					code = TCL_ERROR;
				} else {
					Tcl_SetObjResult(interp, res);
				}
				replace_tclobj(&res, NULL);
			}
			break;
			//>>>
		case M_VALID: //<<<
			{
				enum {A_cmd=1, A_URI, A_objc};
				CHECK_ARGS_LABEL(finally, code, "uri");
				const char*	str = Tcl_GetString(objv[A_URI]);
				Tcl_SetObjResult(interp, uri_valid(str) ? l->t : l->f);
			}
			break;
			//>>>
		case M_CONTEXT: //<<<
			{
				// TODO: implement
				THROW_ERROR_LABEL(finally, code, "Not implemented yet");
			}
			break;
			//>>>
		case M_RESOLVE: //<<<
			{
				// TODO: implement
				THROW_ERROR_LABEL(finally, code, "Not implemented yet");
			}
			break;
			//>>>
		case M_ABSOLUTE: //<<<
			{
				// TODO: implement
				THROW_ERROR_LABEL(finally, code, "Not implemented yet");
			}
			break;
			//>>>
		case M_ENCODE: //<<<
			{
				int						imode;
				enum reuri_encode_mode	mode;
				Tcl_Obj*	res = NULL;

				enum args { A_cmd = 1, A_MODE, A_VAL, A_objc };
				CHECK_ARGS_LABEL(finally, code, "mode value");

				TEST_OK_LABEL(finally, code,
						Tcl_GetIndexFromObj(interp, objv[A_MODE], reuri_encode_mode_str,
							"mode", TCL_EXACT, &imode));
				mode = imode;

				replace_tclobj(&res, Reuri_PercentEncodeObj(interp, mode, objv[A_VAL]));
				Tcl_SetObjResult(interp, res);
				replace_tclobj(&res, NULL);
			}
			break;
			//>>>
		case M_DECODE: //<<<
			{
				Tcl_Obj*	res = NULL;

				enum args { A_cmd = 1, A_VAL, A_objc };
				CHECK_ARGS_LABEL(finally, code, "value");

				res = Reuri_PercentDecodeObj(objv[A_VAL]);

				if (res == NULL)
					THROW_ERROR_LABEL(finally, code, "Could not decode string");	// Should not be reachable

				Tcl_SetObjResult(interp, res);
				replace_tclobj(&res, NULL);
			}
			break;
			//>>>
		case M_QUERY: //<<<
			{
				static const char*	ops[] = {
					"get",
					"values",
					"add",
					"exists",
					"set",
					"unset",
					"names",
					"reorder",
					"new",
					"edit",
					NULL
				};
				enum {
					OP_GET,
					OP_VALUES,
					OP_ADD,
					OP_EXISTS,
					OP_SET,
					OP_UNSET,
					OP_NAMES,
					OP_REORDER,
					OP_NEW,
					OP_EDIT
				};
				int			opidx;
				Tcl_Obj*	query = NULL;
				Tcl_Obj*	res = NULL;
				Tcl_Obj*	uri = NULL;
				Tcl_Obj*	luri = NULL;
				Tcl_Obj*	tmp = NULL;
				int			setvar = 0;

				enum args {A_cmd=1, A_OP, A_URI, A_args};
				CHECK_MIN_ARGS_LABEL(query_finally, code, "op uri ?arg ...?");

				TEST_OK_LABEL(query_finally, code, Tcl_GetIndexFromObj(interp, objv[A_OP], ops, "op", TCL_EXACT, &opidx));
				switch (opidx) {
					case OP_ADD:
					case OP_UNSET:
					case OP_SET:
					case OP_NEW:
						{
							struct uri*	uri_ir = NULL;
							uri = Tcl_ObjGetVar2(interp, objv[A_URI], NULL, 0);
							if (uri) {
								//TEST_OK_LABEL(query_finally, code, Reuri_URIObjGetPart(interp, uri, REURI_QUERY, NULL, &query));
								TEST_OK_LABEL(query_finally, code, ReuriGetURIFromObj(interp, uri, &uri_ir));
								replace_tclobj(&query, uri_ir->query);
							}
							setvar = 1;
						}
						break;
					default:
						TEST_OK_LABEL(query_finally, code, Reuri_URIObjExtractPart(interp, objv[A_URI], REURI_QUERY, l->empty_query, &query));
				}

				switch (opidx) {
					case OP_GET: //<<<
						{
							enum {A_cmd=2, A_URI, A_args};
							CHECK_MIN_ARGS_LABEL(query_finally, code, "uri ?param ?-default default??");

							TEST_OK_LABEL(query_finally, code,
									query_get(interp, objc, objv, A_args-1, query, "uri", &res));

							Tcl_SetObjResult(interp, res);
						}
						break;
						//>>>
					case OP_VALUES: //<<<
						{
							enum {A_cmd=2, A_URI, A_PARAM, A_objc};
							CHECK_ARGS_LABEL(query_finally, code, "uri param");
							TEST_OK_LABEL(query_finally, code, query_values(interp, query, objv[A_PARAM], &res));
							Tcl_SetObjResult(interp, res);
						}
						break;
						//>>>
					case OP_ADD: //<<<
						{
							enum {A_cmd=2, A_URIVAR, A_PARAM, A_VALUE, A_objc};
							CHECK_ARGS_LABEL(query_finally, code, "uri param value");
							TEST_OK_LABEL(query_finally, code, query_add(interp, query, objv[A_PARAM], objv[A_VALUE], &query));
						}
						break;
						//>>>
					case OP_EXISTS: //<<<
						{
							Tcl_Obj*	index = NULL;
							Tcl_Obj*	idxlist = NULL;

							enum {A_cmd=2, A_URI, A_PARAM, A_objc};
							CHECK_ARGS_LABEL(exists_finally, code, "uri param");
							TEST_OK_LABEL(exists_finally, code, ReuriGetQueryFromObj(interp, query, NULL, &index));
							TEST_OK_LABEL(exists_finally, code, Tcl_DictObjGet(interp, index, objv[A_PARAM], &idxlist));
							Tcl_SetObjResult(interp, idxlist ? l->t : l->f);

						exists_finally:
							replace_tclobj(&index, NULL);
							// idxlist ref is on loan from the index dict
						}
						break;
						//>>>
					case OP_SET: //<<<
						{
							enum {A_cmd=2, A_URIVAR, A_args};
							CHECK_MIN_ARGS_LABEL(query_finally, code, "uri ?param value ...?");
							for (int i=A_args; i<objc; i+=2)
								TEST_OK_LABEL(query_finally, code, query_set(interp, query, objv[i], i+1<objc ? objv[i+1] : l->empty, &query));
						}
						break;
						//>>>
					case OP_UNSET: //<<<
						{
							enum {A_cmd=2, A_URIVAR, A_args};
							CHECK_MIN_ARGS_LABEL(query_finally, code, "uri ?param ...");
							TEST_OK_LABEL(query_finally, code, query_unset(interp, query, objc-A_args, objv+A_args, &query));
						}
						break;
						//>>>
					case OP_NAMES: //<<<
						{
							enum {A_cmd=2, A_URI, A_objc};
							CHECK_ARGS_LABEL(query_finally, code, "uri");
							TEST_OK_LABEL(query_finally, code, query_names(interp, query, &res));
							Tcl_SetObjResult(interp, res);
						}
						break;
						//>>>
					case OP_NEW: //<<<
						{
							Tcl_Obj* const*	ov = NULL;
							int				oc;

							switch (objc - A_args) {
								case 1:
									TEST_OK_LABEL(finally, code, Tcl_ListObjGetElements(interp, objv[A_args], &oc, (Tcl_Obj***)&ov));
									break;
								default:
									ov = objv + A_args;
									oc = objc - A_args;
									break;
							}

							TEST_OK_LABEL(finally, code, Reuri_NewQueryObj(interp, oc, ov, &res));
							replace_tclobj(&query, res);
						}
						break;
						//>>>
					case OP_EDIT: //<<<
						{
							enum {A_cmd=2, A_URI, A_args};
							for (int i=A_args; i<objc; i++) {
								if ((Tcl_GetString(objv[i]))[0] == '-') {
									replace_tclobj(&tmp, Tcl_NewStringObj(Tcl_GetString(objv[i])+1, -1));
									TEST_OK_LABEL(query_finally, code, query_unset(interp, query, 1, &tmp, &query));
								} else {
									i++;
									TEST_OK_LABEL(query_finally, code, query_set(interp, query, objv[i-1], i<objc ? objv[i] : l->empty, &query));
								}
							}

							struct uri* uri_ir = NULL;
							replace_tclobj(&luri, Tcl_IsShared(objv[A_URI]) ? Tcl_DuplicateObj(objv[A_URI]) : objv[A_URI]);
							TEST_OK_LABEL(query_finally, code, ReuriGetURIFromObj(interp, luri, &uri_ir));
							replace_tclobj(&uri_ir->query, query);
							Tcl_InvalidateStringRep(luri);

							replace_tclobj(&res, luri);
							Tcl_SetObjResult(interp, res);
						}
						break;
						//>>>
					default:
						THROW_ERROR_LABEL(query_finally, code, "Unhandled uri query case");
				}

				if (setvar) { // Write the uri back into the var <<<
					if (uri == NULL) {
						// Make a new URI
						struct uri uri_ir = {.query = query};
						replace_tclobj(&luri, Tcl_NewObj());
						uri = luri;
						ReuriSetURI(uri, &uri_ir);
					} else {
						// Update existing one
						struct uri* uri_ir = NULL;
						if (Tcl_IsShared(uri)) uri = Tcl_DuplicateObj(uri);
						TEST_OK_LABEL(query_finally, code, ReuriGetURIFromObj(interp, uri, &uri_ir));
						replace_tclobj(&uri_ir->query, query);
						Tcl_InvalidateStringRep(uri);
					}

					uri = Tcl_ObjSetVar2(interp, objv[A_URI], NULL, uri, TCL_LEAVE_ERR_MSG);
					if (uri == NULL) {
						code = TCL_ERROR;
						goto query_finally;
					}

					replace_tclobj(&res, uri);
					Tcl_SetObjResult(interp, res);
				}
				//>>>

			query_finally:
				replace_tclobj(&query, NULL);
				replace_tclobj(&res, NULL);
				replace_tclobj(&luri, NULL);
				replace_tclobj(&tmp, NULL);
			}
			break;
			//>>>
		case M_PATH: //<<<
			{
				static const char*	ops[] = {
					"get",
					"exists",
					"set",
					NULL
				};
				enum {
					OP_GET,
					OP_EXISTS,
					OP_SET
				};
				int			opidx;
				Tcl_Obj*	path = NULL;

				enum args {A_cmd=1, A_OP, A_URI, A_objc};
				if (objc < A_objc) CHECK_ARGS_LABEL(path_finally, code, "op uri ?arg ...?");

				TEST_OK_LABEL(path_finally, code, Tcl_GetIndexFromObj(interp, objv[A_OP], ops, "op", TCL_EXACT, &opidx));
				TEST_OK_LABEL(path_finally, code, Reuri_URIObjExtractPart(interp, objv[A_URI], REURI_PATH, l->empty_list, &path));

				switch (opidx) {
					case OP_GET: //<<<
						{
							Tcl_Obj*		pathlist = NULL;
							Tcl_Obj*		res = NULL;

							enum {A_cmd=2, A_URI, A_INDEX, A_objc};
							if (objc < A_INDEX || objc > A_objc)
								CHECK_ARGS_LABEL(path_get_finally, code, "uri ?index?");

							TEST_OK_LABEL(path_get_finally, code, Reuri_GetPathFromObj(interp, path, &pathlist));
							if (objc > A_INDEX) {
								TEST_OK_LABEL(path_get_finally, code, Idx_PickFromList(interp, pathlist, objv[A_INDEX], &res));
							} else {
								replace_tclobj(&res, pathlist);
							}
							Tcl_SetObjResult(interp, res);

						path_get_finally:
							replace_tclobj(&pathlist, NULL);
							replace_tclobj(&res, NULL);
						}
						break;
						//>>>
					case OP_EXISTS: //<<<
						{
							Tcl_Obj*		pathlist = NULL;
							Tcl_Obj*		res = NULL;

							enum {A_cmd=2, A_URI, A_INDEX, A_objc};
							if (objc < A_INDEX || objc > A_objc)
								CHECK_ARGS_LABEL(path_exists_finally, code, "uri ?index?");

							if (objc > A_INDEX) {
								int pathc;
								TEST_OK_LABEL(path_exists_finally, code, Reuri_GetPathFromObj(interp, path, &pathlist));
								TEST_OK_LABEL(path_exists_finally, code, Tcl_ListObjLength(interp, pathlist, &pathc));
								TEST_OK_LABEL(path_exists_finally, code, Idx_Exists(interp, pathc, objv[A_INDEX], &res));
							} else {
								int exists;
								TEST_OK_LABEL(path_exists_finally, code, Reuri_URIObjPartExists(interp, objv[A_URI], REURI_PATH, &exists));
								replace_tclobj(&res, exists ? l->t : l->f);
							}
							Tcl_SetObjResult(interp, res);

						path_exists_finally:
							replace_tclobj(&pathlist, NULL);
							replace_tclobj(&res, NULL);
						}
						break;
						//>>>
					default:
						THROW_ERROR_LABEL(path_finally, code, "Unhandled uri path case");
				}

			path_finally:
				replace_tclobj(&path, NULL);
			}
			break;
			//>>>
		case M_NORMALIZE: //<<<
			{
				struct uri*	uri;
				enum {A_cmd=1, A_URI, A_objc};
				CHECK_ARGS_LABEL(finally, code, "uri");
				TEST_OK_LABEL(finally, code, ReuriGetURIFromObj(interp, objv[A_URI], &uri));
				Tcl_InvalidateStringRep(objv[A_URI]);
				Tcl_SetObjResult(interp, objv[A_URI]);
			}
			break;
			//>>>
		default: THROW_PRINTF_LABEL(finally, code, "Invalid method index: %d", methodidx);
	}

finally:
	return code;
}

//>>>
static int QueryObjCmd(ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj* const objv[]) //<<<
{
	int					code = TCL_OK;
	struct interp_cx*	l = Tcl_GetAssocData(interp, "reuri", NULL);
	static const char*	methods[] = {
		"get",
		"values",
		"add",
		"exists",
		"set",
		"unset",
		"names",
		"reorder",
		"new",
		"encode",
		"decode",
		NULL
	};
	enum {
		M_GET,
		M_VALUES,
		M_ADD,
		M_EXISTS,
		M_SET,
		M_UNSET,
		M_NAMES,
		M_REORDER,
		M_NEW,
		M_ENCODE,
		M_DECODE
	};
	int	methodidx;
	Tcl_Obj*	res = NULL;

	if (objc < 2) {
		Tcl_WrongNumArgs(interp, 1, objv, "method ?arg ...?");
		code = TCL_ERROR;
		goto finally;
	}

	TEST_OK_LABEL(finally, code, Tcl_GetIndexFromObj(interp, objv[1], methods, "method", TCL_EXACT, &methodidx));

	switch (methodidx) {
		case M_GET: //<<<
			{
				enum {A_cmd=1, A_QUERY, A_args, A_objc};
				if (objc < A_args) CHECK_ARGS_LABEL(finally, code, "query ?param ?-default defaultval??");
				TEST_OK_LABEL(finally, code, query_get(interp, objc, objv, A_args-1, objv[A_QUERY], "query", &res));
				Tcl_SetObjResult(interp, res);
			}
			break;
			//>>>
		case M_VALUES: //<<<
			{
				enum {A_cmd=1, A_QUERY, A_PARAM, A_objc};
				CHECK_ARGS_LABEL(finally, code, "query param");
				TEST_OK_LABEL(finally, code, query_values(interp, objv[A_QUERY], objv[A_PARAM], &res));
				Tcl_SetObjResult(interp, res);
			}
			break;
			//>>>
		case M_ADD: //<<<
			{
				Tcl_Obj*	queryObj = NULL;
				enum {A_cmd=1, A_QUERYVAR, A_PARAM, A_VALUE, A_objc};
				CHECK_ARGS_LABEL(finally, code, "queryVarName param value");
				TEST_OK_LABEL(finally, code, query_add(interp, Tcl_ObjGetVar2(interp, objv[A_QUERYVAR], NULL, 0), objv[A_PARAM], objv[A_VALUE], &res));
				queryObj = Tcl_ObjSetVar2(interp, objv[A_QUERYVAR], NULL, res, TCL_LEAVE_ERR_MSG);
				if (queryObj == NULL) { code = TCL_ERROR; goto finally; }
				replace_tclobj(&res, queryObj);
				Tcl_SetObjResult(interp, res);
			}
			break;
			//>>>
		case M_EXISTS: //<<<
			{
				Tcl_Obj*	index = NULL;
				Tcl_Obj*	idxlist = NULL;

				enum {A_cmd=1, A_QUERY, A_PARAM, A_objc};
				CHECK_ARGS_LABEL(exists_finally, code, "query param");
				TEST_OK_LABEL(exists_finally, code, ReuriGetQueryFromObj(interp, objv[A_QUERY], NULL, &index));
				TEST_OK_LABEL(exists_finally, code, Tcl_DictObjGet(interp, index, objv[A_PARAM], &idxlist));
				Tcl_SetObjResult(interp, idxlist ? l->t : l->f);

			exists_finally:
				replace_tclobj(&index, NULL);
				// idxlist ref is on loan from the index dict
			}
			break;
			//>>>
		case M_SET: //<<<
			{
				Tcl_Obj*	queryObj = NULL;
				Tcl_Obj*	tmp = NULL;
				enum {A_cmd=1, A_QUERYVAR, A_args};
				CHECK_MIN_ARGS_LABEL(finally, code, "queryVarName ?param value ...?");
				tmp = Tcl_ObjGetVar2(interp, objv[A_QUERYVAR], NULL, 0);
				if (tmp == NULL) tmp = l->empty;
				for (int i=A_args; i<objc; i+=2) {
					TEST_OK_LABEL(finally, code, query_set(interp, tmp, objv[i], i+1<objc ? objv[i+1] : l->empty, &res));
					tmp = res;
				}
				queryObj = Tcl_ObjSetVar2(interp, objv[A_QUERYVAR], NULL, res ? res : tmp, TCL_LEAVE_ERR_MSG);
				if (queryObj == NULL) { code = TCL_ERROR; goto finally; }
				replace_tclobj(&res, queryObj);
				Tcl_SetObjResult(interp, res);
			}
			break;
			//>>>
		case M_UNSET: //<<<
			{
				Tcl_Obj*	queryObj = NULL;
				enum {A_cmd=1, A_QUERYVAR, A_args};
				CHECK_MIN_ARGS_LABEL(finally, code, "queryVarName ?param ...?");
				TEST_OK_LABEL(finally, code, query_unset(interp, Tcl_ObjGetVar2(interp, objv[A_QUERYVAR], NULL, 0), objc-A_args, objv+A_args, &res));
				queryObj = Tcl_ObjSetVar2(interp, objv[A_QUERYVAR], NULL, res, TCL_LEAVE_ERR_MSG);
				if (queryObj == NULL) { code = TCL_ERROR; goto finally; }
				replace_tclobj(&res, queryObj);
				Tcl_SetObjResult(interp, res);
			}
			break;
			//>>>
		case M_NAMES: //<<<
			{
				enum {A_cmd=1, A_QUERY, A_objc};
				CHECK_ARGS_LABEL(finally, code, "query");
				TEST_OK_LABEL(finally, code, query_names(interp, objv[A_QUERY], &res));
				Tcl_SetObjResult(interp, res);
			}
			break;
			//>>>
		case M_REORDER: //<<<
			{
				// TODO: implement
				THROW_ERROR_LABEL(finally, code, "Not implemented yet");
			}
			break;
			//>>>
		case M_NEW: //<<<
			{
				Tcl_Obj* const*	ov = NULL;
				int				oc;

				switch (objc) {
					case 3:
						TEST_OK_LABEL(finally, code, Tcl_ListObjGetElements(interp, objv[2], &oc, (Tcl_Obj***)&ov));
						break;
					default:
						ov = objv+2;
						oc = objc-2;
						break;
				}

				TEST_OK_LABEL(finally, code, Reuri_NewQueryObj(interp, oc, ov, &res));
				Tcl_SetObjResult(interp, res);
			}
			break;
			//>>>
		case M_ENCODE: //<<<
			{
				Tcl_Obj* const*	ov = NULL;
				int				oc;

				switch (objc) {
					case 3:
						TEST_OK_LABEL(finally, code, Tcl_ListObjGetElements(interp, objv[2], &oc, (Tcl_Obj***)&ov));
						break;
					default:
						ov = objv+2;
						oc = objc-2;
						break;
				}

				TEST_OK_LABEL(finally, code, Reuri_NewQueryObj(interp, oc, ov, &res));

				const char* str = Tcl_GetString(res);
				if (str[0]) {
					Tcl_SetObjResult(interp, Tcl_ObjPrintf("?%s", Tcl_GetString(res)));
				} else {
					Tcl_SetObjResult(interp, l->empty);
				}
			}
			break;
			//>>>
		case M_DECODE: //<<<
			{
				Tcl_Obj*	q = NULL;
				enum {A_cmd=1, A_QUERY, A_objc};
				CHECK_ARGS_LABEL(decode_finally, code, "query");
				replace_tclobj(&q, objv[A_QUERY]);

				int			len;
				const char*	str = Tcl_GetStringFromObj(q, &len);
				if (str[0] == '?') replace_tclobj(&q, Tcl_NewStringObj(str+1, len-1));

				TEST_OK_LABEL(decode_finally, code, ReuriGetQueryFromObj(interp, q, &res, NULL));
				Tcl_SetObjResult(interp, res);
			decode_finally:
				replace_tclobj(&q, NULL);
			}
			break;
			//>>>
		default: Tcl_Panic("Invalid method index: %d", methodidx);
	}

finally:
	replace_tclobj(&res, NULL);
	return code;
}

//>>>
static int PathObjCmd(ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj* const objv[]) //<<<
{
	int					code = TCL_OK;
	struct interp_cx*	l = (struct interp_cx*)Tcl_GetAssocData(interp, "reuri", NULL);
	static const char*	methods[] = {
		"split",
		"get",
		"exists",
		"set",
		"join",
		"resolve",
		NULL
	};
	enum {
		M_SPLIT,
		M_GET,
		M_EXISTS,
		M_SET,
		M_JOIN,
		M_RESOLVE
	};
	int	methodidx;

	if (objc < 2) {
		Tcl_WrongNumArgs(interp, 1, objv, "method ?arg ...?");
		code = TCL_ERROR;
		goto finally;
	}

	TEST_OK_LABEL(finally, code, Tcl_GetIndexFromObj(interp, objv[1], methods, "method", TCL_EXACT, &methodidx));

	switch (methodidx) {
		case M_SPLIT: // DEPRECATED <<<
			{
				Tcl_Obj*	res = NULL;

				enum {A_cmd=1, A_PATH, A_objc};
				CHECK_ARGS_LABEL(finally, code, "path");

				TEST_OK_LABEL(finally, code, Reuri_GetPathFromObj(interp, objv[A_PATH], &res));
				Tcl_SetObjResult(interp, res);
				replace_tclobj(&res, NULL);
			}
			break;
			//>>>
		case M_GET: //<<<
			{
				Tcl_Obj*		pathlist = NULL;
				Tcl_Obj*		res = NULL;

				enum {A_cmd=1, A_PATH, A_INDEX, A_objc};
				if (objc < A_INDEX || objc > A_objc)
					CHECK_ARGS_LABEL(get_finally, code, "path ?index?");

				TEST_OK_LABEL(get_finally, code, Reuri_GetPathFromObj(interp, objv[A_PATH], &pathlist));
				if (objc > A_INDEX) {
					TEST_OK_LABEL(get_finally, code, Idx_PickFromList(interp, pathlist, objv[A_INDEX], &res));
				} else {
					replace_tclobj(&res, pathlist);
				}
				Tcl_SetObjResult(interp, res);

			get_finally:
				replace_tclobj(&pathlist, NULL);
				replace_tclobj(&res, NULL);
			}
			break;
			//>>>
		case M_EXISTS: //<<<
			{
				Tcl_Obj*		pathlist = NULL;
				Tcl_Obj*		res = NULL;
				int pathc;

				enum {A_cmd=1, A_PATH, A_INDEX, A_objc};
				if (objc < A_INDEX || objc > A_objc)
					CHECK_ARGS_LABEL(exists_finally, code, "path ?index?");

				TEST_OK_LABEL(exists_finally, code, Reuri_GetPathFromObj(interp, objv[A_PATH], &pathlist));
				TEST_OK_LABEL(exists_finally, code, Tcl_ListObjLength(interp, pathlist, &pathc));
				if (objc > A_INDEX) {
					TEST_OK_LABEL(exists_finally, code, Idx_Exists(interp, pathc, objv[A_INDEX], &res));
				} else {
					replace_tclobj(&res, pathc > 0 ? l->t : l->f);
				}
				Tcl_SetObjResult(interp, res);

			exists_finally:
				replace_tclobj(&pathlist, NULL);
				replace_tclobj(&res, NULL);
			}
			break;
			//>>>
		case M_SET: //<<<
			{
				// TODO: implement
				THROW_ERROR_LABEL(finally, code, "Not implemented yet");
			}
			break;
			//>>>
		case M_JOIN: //<<<
			{
				Tcl_DString	ds;

				Tcl_DStringInit(&ds);

				enum {A_cmd=1, A_args};
				CHECK_MIN_ARGS_LABEL(join_finally, code, "?segment ...?");

				int segmentc	= objc - A_args;
				if (segmentc > 0) {
					int			len;
					const char* seg = Tcl_GetStringFromObj(objv[A_args], &len);
					if (len == 1 && seg[0] == '/') {
						// Root
						if (segmentc == 1) Tcl_DStringAppend(&ds, "/", 1);
					} else {
						percent_encode_ds(REURI_ENCODE_PATH, &ds, seg);
					}

					for (int i=A_args+1; i<segmentc+A_args; i++) {
						seg = Tcl_GetString(objv[i]);
						Tcl_DStringAppend(&ds, "/", 1);
						percent_encode_ds(REURI_ENCODE_PATH2, &ds, seg);
					}
				}

			join_finally:
				if (code == TCL_OK) {
					Tcl_DStringResult(interp, &ds);
				} else {
					Tcl_DStringFree(&ds);
				}
			}
			break;
			//>>>
		case M_RESOLVE: //<<<
			{
				// TODO: implement
				THROW_ERROR_LABEL(finally, code, "Not implemented yet");
			}
			break;
			//>>>
		default: Tcl_Panic("Invalid method index: %d", methodidx);
	}

finally:
	return code;
}

//>>>
#if TESTMODE
static int NopObjCmd(ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj* const objv[]) //<<<
{
	return TCL_OK;
}

//>>>
#endif

#define NS	"::reuri"
static struct cmd {
	char*			name;
	Tcl_ObjCmdProc*	proc;
} cmds[] = {
	{NS "::uri",	UriObjCmd},
	{NS "::query",	QueryObjCmd},
	{NS "::path",	PathObjCmd},
#if TESTMODE
	{NS "::nop",	NopObjCmd},
#endif
	{NULL,			NULL}
};
// Script API >>>

extern const ReuriStubs* const reuriConstStubsPtr;

#ifdef __cplusplus
extern "C" {
#endif
DLLEXPORT int Reuri_Init(Tcl_Interp* interp) //<<<
{
	int					code = TCL_OK;
	struct interp_cx*	l = NULL;
	Tcl_Namespace*		ns = NULL;
	struct cmd*			c = cmds;

#if USE_TCL_STUBS
	if (Tcl_InitStubs(interp, TCL_VERSION, 0) == NULL)
		return TCL_ERROR;
#endif
#if USE_DEDUP_STUBS
	if (Dedup_InitStubs(interp) == NULL)
		return TCL_ERROR;
#endif

	ns = Tcl_CreateNamespace(interp, NS, NULL, NULL);
	TEST_OK_LABEL(finally, code, Tcl_Export(interp, ns, "*", 0));

	// Set up interp_cx <<<
	l = (struct interp_cx*)ckalloc(sizeof *l);
	*l = (struct interp_cx){0};

	// General string dedup pool
	l->dedup_pool = Dedup_NewPool(interp);

	// Part-specific dedup pools
	l->dedup_scheme			= Dedup_NewPool(interp);
	l->dedup_userinfo		= Dedup_NewPool(interp);
	l->dedup_host_reg_name	= Dedup_NewPool(interp);
	l->dedup_host_ipv4		= Dedup_NewPool(interp);
	l->dedup_host_ipv6		= Dedup_NewPool(interp);
	l->dedup_host_local		= Dedup_NewPool(interp);
	l->dedup_port			= Dedup_NewPool(interp);
	l->dedup_path			= Dedup_NewPool(interp);
	l->dedup_query			= Dedup_NewPool(interp);
	l->dedup_fragment		= Dedup_NewPool(interp);

	l->typeInt = Tcl_GetObjType("int");
	{
		int i;
		for (i=0; reuri_hosttype_str[i]; i++)
			replace_tclobj(&l->hosttype[i], Dedup_NewStringObj(l->dedup_pool, reuri_hosttype_str[i], -1));
	}
	replace_tclobj(&l->empty,		Dedup_NewStringObj(l->dedup_pool, "", 0));
	replace_tclobj(&l->empty_list,	Tcl_NewListObj(0, NULL));
	replace_tclobj(&l->t,			Tcl_NewBooleanObj(1));
	replace_tclobj(&l->f,			Tcl_NewBooleanObj(0));
	replace_tclobj(&l->apply,		Tcl_NewStringObj("apply", -1));
	replace_tclobj(&l->sort_unique,	Tcl_NewStringObj("l {lsort -integer -unique -decreasing $l}", -1));

	TEST_OK_LABEL(finally, code, Reuri_NewQueryObj(interp, 0, NULL, &l->empty_query));

	Tcl_SetAssocData(interp, "reuri", free_interp_cx, l);
	// Set up interp_cx >>>

	while (c->name) {
		if (NULL == Tcl_CreateObjCommand(interp, c->name, c->proc, l, NULL)) {
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("Could not create command %s", c->name));
			code = TCL_ERROR;
			goto finally;
		}
		c++;
	}

#if TESTMODE
	TEST_OK_LABEL(finally, code, testmode_init(interp, l));
#endif

	TEST_OK_LABEL(finally, code, Tcl_PkgProvideEx(interp, PACKAGE_NAME, PACKAGE_VERSION, reuriConstStubsPtr));

finally:
	return code;
}

//>>>
DLLEXPORT int Reuri_SafeInit(Tcl_Interp* interp) //<<<
{
	// No unsafe features
	return Reuri_Init(interp);
}

//>>>

/*
 * No Reuri_Unload: Can't unload packages that use custom ObjTypes - the
 * objects could still be around in literal tables for instance and when
 * they're eventually freed it will segfault because the type handler points at
 * invalid memory.
 */

#ifdef __cplusplus
}
#endif

// vim: foldmethod=marker foldmarker=<<<,>>> ts=4 shiftwidth=4
