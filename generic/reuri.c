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
	"path",
	"host",
	"fragment",
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
		if (l->dedup_pool) {
			Dedup_FreePool(l->dedup_pool);
			l->dedup_pool = NULL;
		}

		for (i=0; reuri_hosttype_str[i]; i++)
			replace_tclobj(&l->hosttype[i], NULL);

		replace_tclobj(&l->empty,		NULL);
		replace_tclobj(&l->empty_list,	NULL);
		replace_tclobj(&l->t,			NULL);
		replace_tclobj(&l->f,			NULL);

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
// Internal API >>>
// Stubs API <<<
int Reuri_URIObjGetPart(Tcl_Interp* interp, Tcl_Obj* uriPtr, enum reuri_part part, Tcl_Obj* defaultPtr, Tcl_Obj** valuePtrPtr) //<<<
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

	if (res == NULL) {
		if (defaultPtr) {
			res = defaultPtr;
		} else {
			Tcl_SetErrorCode(interp, "REURI", "PART_NOT_SET", reuri_part_str[part], NULL);
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("%s part is not defined in %s",
						reuri_part_str[part], Tcl_GetString(uriPtr)));
			code = TCL_ERROR;
			goto finally;
		}
	}

	replace_tclobj(valuePtrPtr, res);
	res = NULL;

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

	TEST_OK_LABEL(finally, code, ReuriGetURIFromObj(interp, uriPtr, &uri));

	replace_tclobj(&d, Tcl_NewDictObj());

#define ADD_PART(k, v) \
	if (v) TEST_OK_LABEL(finally, code, Tcl_DictObjPut(interp, d, \
			Dedup_NewStringObj(l->dedup_pool, k, -1), v));

	ADD_PART("scheme",    uri->scheme);
	ADD_PART("userinfo",  uri->userinfo);
	ADD_PART("host",      uri->host);
	ADD_PART("hosttype",  l->hosttype[uri->hosttype]);
	ADD_PART("port",      uri->port);
	ADD_PART("path",      uri->path);
	ADD_PART("query",     uri->query);
	ADD_PART("fragment",  uri->fragment);

	replace_tclobj(res, d);

finally:
	replace_tclobj(&d, NULL);
	return code;
}

//>>>
Tcl_Obj* Reuri_PercentEncodeObj(Tcl_Interp* interp, enum reuri_encode_mode mode, Tcl_Obj* objPtr) //<<<
{
	switch (mode) {
		case REURI_ENCODE_QUERY:    return percent_encode(interp, objPtr, REURI_ENCODE_QUERY);
		case REURI_ENCODE_PATH:     return percent_encode(interp, objPtr, REURI_ENCODE_PATH);
		case REURI_ENCODE_HOST:     return percent_encode(interp, objPtr, REURI_ENCODE_HOST);
		case REURI_ENCODE_FRAGMENT: return percent_encode(interp, objPtr, REURI_ENCODE_FRAGMENT);
	}
	Tcl_Panic("Invalid encode mode: %d", mode);
	return NULL;	// Unreachable, pacify compiler
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
		NULL
	};
	enum {
		M_GET,
		M_EXISTS,
		M_SET,
		M_VALID,
		M_CONTEXT,
		M_RESOLVE,
		M_ABSOLUTE,
		M_ENCODE,
		M_DECODE,
		M_QUERY,
		M_PATH
	};
	int	methodidx;

	if (objc < 2) {
		Tcl_WrongNumArgs(interp, 1, objv, "method ?arg ...?");
		code = TCL_ERROR;
		goto finally;
	}

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
		case M_EXISTS: //<<<
			{
				enum {
					A_METHOD=1,
					A_URI,
					A_PART,
					A_objc
				};
				int				exists;
				enum reuri_part	part;

				if (objc != A_objc) {
					Tcl_WrongNumArgs(interp, 2, objv, "uri part");
					code = TCL_ERROR;
					goto finally;
				}

				TEST_OK_LABEL(finally, code, ReuriGetPartFromObj(interp, objv[A_PART], &part));
				TEST_OK_LABEL(finally, code, Reuri_URIObjPartExists(interp, objv[A_URI], part, &exists));
				Tcl_SetObjResult(interp, exists ? l->t : l->f);
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
				enum args {
					A_METHOD = 1,
					A_MODE,
					A_VAL,
					A_objc
				};
				int						imode;
				enum reuri_encode_mode	mode;
				Tcl_Obj*	res = NULL;

				if (objc != A_objc) {
					Tcl_WrongNumArgs(interp, 2, objv, "mode value");
					code = TCL_ERROR;
					goto finally;
				}

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
				enum args {
					A_METHOD = 1,
					A_VAL,
					A_objc
				};
				Tcl_Obj*	res = NULL;

				if (objc != A_objc) {
					Tcl_WrongNumArgs(interp, 2, objv, "value");
					code = TCL_ERROR;
					goto finally;
				}

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
					"exists",
					"set",
					"unset",
					"names",
					"reorder",
					NULL
				};
				enum {
					OP_GET,
					OP_VALUES,
					OP_EXISTS,
					OP_SET,
					OP_UNSET,
					OP_NAMES,
					OP_REORDER
				};
				int			opidx;
				Tcl_Obj*	query = NULL;
				Tcl_Obj*	res = NULL;

				enum args {A_cmd=1, A_OP, A_URI, A_objc};
				if (objc < A_objc) CHECK_ARGS_LABEL(query_finally, code, "op uri ?arg ...?");

				TEST_OK_LABEL(query_finally, code, Tcl_GetIndexFromObj(interp, objv[A_OP], ops, "op", TCL_EXACT, &opidx));
				TEST_OK_LABEL(query_finally, code, Reuri_URIObjGetPart(interp, objv[A_URI], REURI_QUERY, l->empty_query, &query));

				switch (opidx) {
					case OP_GET: //<<<
						{
							enum {A_cmd=2, A_URI, A_args, A_objc};
							if (objc < A_args)
								CHECK_ARGS_LABEL(query_finally, code, "uri ?param ?-default default??");

							TEST_OK_LABEL(query_finally, code,
									query_get(interp, objc, objv, A_args-1, query, "uri", &res));

							Tcl_SetObjResult(interp, res);
						}
						break;
						//>>>
					default:
						THROW_ERROR_LABEL(finally, code, "Unhandled uri query case");
				}
			query_finally:
				replace_tclobj(&query, NULL);
				replace_tclobj(&res, NULL);
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
				TEST_OK_LABEL(path_finally, code, Reuri_URIObjGetPart(interp, objv[A_URI], REURI_PATH, l->empty_list, &path));

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

							TEST_OK_LABEL(path_exists_finally, code, Reuri_GetPathFromObj(interp, path, &pathlist));
							if (objc > A_INDEX) {
								int pathc;
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
						THROW_ERROR_LABEL(finally, code, "Unhandled uri path case");
				}

			path_finally:
				replace_tclobj(&path, NULL);
			}
			break;
			//>>>
		default: Tcl_Panic("Invalid method index: %d", methodidx);
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
				Tcl_Obj*	query = NULL;
				Tcl_Obj*	index = NULL;
				Tcl_Obj*	idxlist = NULL;
				Tcl_Obj**	idxv = NULL;
				int			idxc;
				Tcl_Obj**	qv = NULL;
				int			qc;

				enum {A_cmd=1, A_QUERY, A_PARAM, A_objc};
				CHECK_ARGS_LABEL(values_finally, code, "query param");
				TEST_OK_LABEL(values_finally, code, ReuriGetQueryFromObj(interp, objv[A_QUERY], &query, &index));
				TEST_OK_LABEL(values_finally, code, Tcl_ListObjGetElements(interp, query, &qc, &qv));
				TEST_OK_LABEL(values_finally, code, Tcl_DictObjGet(interp, index, objv[A_PARAM], &idxlist));
				if (idxlist) {
					Tcl_IncrRefCount(idxlist);
					TEST_OK_LABEL(values_finally, code, Tcl_ListObjGetElements(interp, idxlist, &idxc, &idxv));

					replace_tclobj(&res, Tcl_NewListObj(idxc, NULL));
					for (int i=0; i<idxc; i++) {
						int idx;
						TEST_OK_LABEL(values_finally, code, Tcl_GetIntFromObj(interp, idxv[i], &idx));
						TEST_OK_LABEL(values_finally, code, Tcl_ListObjAppendElement(interp, res, qv[idx*2+1]));
					}
					Tcl_SetObjResult(interp, res);
				}

			values_finally:
				replace_tclobj(&query, NULL);
				replace_tclobj(&index, NULL);
				replace_tclobj(&idxlist, NULL);
			}
			break;
			//>>>
		case M_ADD: //<<<
			{
				// TODO: implement
				THROW_ERROR_LABEL(finally, code, "Not implemented yet");
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
				// TODO: implement
				THROW_ERROR_LABEL(finally, code, "Not implemented yet");
			}
			break;
			//>>>
		case M_UNSET: //<<<
			{
				// TODO: implement
				THROW_ERROR_LABEL(finally, code, "Not implemented yet");
			}
			break;
			//>>>
		case M_NAMES: //<<<
			{
				Tcl_Obj*	query = NULL;
				Tcl_Obj*	names = NULL;
				Tcl_Obj**	qv = NULL;
				int			qc;

				enum {A_cmd=1, A_QUERY, A_objc};
				CHECK_ARGS_LABEL(names_finally, code, "query");

				TEST_OK_LABEL(names_finally, code, ReuriGetQueryFromObj(interp, objv[A_QUERY], &query, NULL));
				TEST_OK_LABEL(names_finally, code, Tcl_ListObjGetElements(interp, query, &qc, &qv));
				if (qc > 0) {
					replace_tclobj(&res, Tcl_NewListObj(qc, NULL));
					for (int i=0; i<qc; i+=2)
						TEST_OK_LABEL(names_finally, code, Tcl_ListObjAppendElement(interp, res, qv[i]));
				} else {
					replace_tclobj(&res, l->empty_list);
				}
				Tcl_SetObjResult(interp, res);

			names_finally:
				replace_tclobj(&query, NULL);
				replace_tclobj(&names, NULL);
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

				Tcl_SetObjResult(interp, res);
			}
			break;
			//>>>
		case M_DECODE: //<<<
			{
				enum {A_cmd=1, A_QUERY, A_objc};
				CHECK_ARGS_LABEL(finally, code, "query");

				fprintf(stderr, "decode\n");
				TEST_OK_LABEL(finally, code, ReuriGetQueryFromObj(interp, objv[A_QUERY], &res, NULL));
				Tcl_SetObjResult(interp, res);
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
				enum {
					A_cmd=1,
					A_SEGMENTS,
					A_objc
				};
				Tcl_Obj**	segmentv = NULL;
				int			segmentc;
				Tcl_DString	ds;

				Tcl_DStringInit(&ds);

				CHECK_ARGS_LABEL(join_finally, code, "segments");

				TEST_OK_LABEL(join_finally, code, Tcl_ListObjGetElements(interp, objv[A_SEGMENTS], &segmentc, &segmentv));
				if (segmentc > 0) {
					int			len;
					const char* seg = Tcl_GetStringFromObj(segmentv[0], &len);
					if (len == 1 && seg[0] == '/') {
						// Root
						if (segmentc == 1) Tcl_DStringAppend(&ds, "/", 1);
					} else {
						percent_encode_ds(REURI_ENCODE_PATH, &ds, seg);
					}

					for (int i=1; i<segmentc; i++) {
						seg = Tcl_GetString(segmentv[i]);
						Tcl_DStringAppend(&ds, "/", 1);
						percent_encode_ds(REURI_ENCODE_PATH, &ds, seg);
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

#define NS	"::reuri"
static struct cmd {
	char*			name;
	Tcl_ObjCmdProc*	proc;
} cmds[] = {
	{NS "::uri",	UriObjCmd},
	{NS "::query",	QueryObjCmd},
	{NS "::path",	PathObjCmd},
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

	l->dedup_pool = Dedup_NewPool(interp);
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
