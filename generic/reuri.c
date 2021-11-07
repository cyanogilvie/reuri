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
	NULL				// NULL here so we can point Tcl_GetIndexFromObj at this
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

	if (l) {
		if (l->dedup_pool) {
			Dedup_FreePool(l->dedup_pool);
			l->dedup_pool = NULL;
		}

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
// Internal API >>>
// Stubs API <<<
int Reuri_URIObjGetPart(Tcl_Interp* interp, Tcl_Obj* uriPtr, enum reuri_part part, Tcl_Obj* defaultPtr, Tcl_Obj** valuePtrPtr) //<<<
{
	int			code = TCL_OK;
	struct uri*	uri = NULL;
	Tcl_Obj*	res = NULL;

	TEST_OK_LABEL(finally, code, ReuriGetURIFromObj(interp, uriPtr, &uri));

	switch (part) {
		case REURI_SCHEME:		res = uri->scheme;		break;
		case REURI_USERINFO:	res = uri->userinfo;	break;
		case REURI_HOST:		res = uri->host;		break;
		case REURI_PORT:		res = uri->port;		break;
		case REURI_PATH:		res = uri->path;		break;
		case REURI_QUERY:		res = uri->query;		break;
		case REURI_FRAGMENT:	res = uri->fragment;	break;
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

	*valuePtrPtr = res;
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
		case REURI_ENCODE_QUERY: return percent_encode_query(interp, objPtr, REURI_ENCODE_QUERY);
		case REURI_ENCODE_PATH:  return percent_encode_query(interp, objPtr, REURI_ENCODE_PATH);
		case REURI_ENCODE_HOST:  return percent_encode_query(interp, objPtr, REURI_ENCODE_HOST);
	}
	Tcl_Panic("Invalid encode mode: %d", mode);
	return NULL;	// Unreachable, pacify compiler
}

//>>>
// Stubs API >>>
// Script API <<<
static int UriObjCmd(ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj* const objv[]) //<<<
{
	int			code = TCL_OK;
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
		M_DECODE
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
						TEST_OK_LABEL(finally, code, Reuri_URIObjGetAll(interp, objv[A_URI], &res));
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

				TEST_OK_LABEL(finally, code, ReuriGetPartFromObj(interp, objv[A_PART], &part));
				TEST_OK_LABEL(finally, code, Reuri_URIObjGetPart(interp, objv[A_URI], part, def, &res));

				Tcl_SetObjResult(interp, res);
			}
			break;
			//>>>
		case M_EXISTS: //<<<
			{
				// TODO: implement
				THROW_ERROR_LABEL(finally, code, "Not implemented yet");
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
				// TODO: implement
				THROW_ERROR_LABEL(finally, code, "Not implemented yet");
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
static int QueryObjCmd(ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj* const objv[]) //<<<
{
	int			code = TCL_OK;
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

	if (objc < 2) {
		Tcl_WrongNumArgs(interp, 1, objv, "method ?arg ...?");
		code = TCL_ERROR;
		goto finally;
	}

	TEST_OK_LABEL(finally, code, Tcl_GetIndexFromObj(interp, objv[1], methods, "method", TCL_EXACT, &methodidx));

	switch (methodidx) {
		case M_GET: //<<<
			{
				// TODO: implement
				THROW_ERROR_LABEL(finally, code, "Not implemented yet");
			}
			break;
			//>>>
		case M_VALUES: //<<<
			{
				// TODO: implement
				THROW_ERROR_LABEL(finally, code, "Not implemented yet");
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
				// TODO: implement
				THROW_ERROR_LABEL(finally, code, "Not implemented yet");
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
				// TODO: implement
				THROW_ERROR_LABEL(finally, code, "Not implemented yet");
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
				Tcl_Obj*		res = NULL;

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
				replace_tclobj(&res, NULL);
			}
			break;
			//>>>
		case M_DECODE: //<<<
			{
				enum {
					A_METHOD=1,
					A_QUERY,
					A_objc
				};
				Tcl_Obj*	res = NULL;

				if (objc != A_objc) {
					Tcl_WrongNumArgs(interp, 2, objv, "query");
					code = TCL_ERROR;
					goto finally;
				}

				TEST_OK_LABEL(finally, code, ReuriGetQueryFromObj(interp, objv[A_QUERY], &res, NULL));
				Tcl_SetObjResult(interp, res);
				replace_tclobj(&res, NULL);
			}
			break;
			//>>>
		default: Tcl_Panic("Invalid method index: %d", methodidx);
	}

finally:
	return code;
}

//>>>
static int PathObjCmd(ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj* const objv[]) //<<<
{
	int			code = TCL_OK;
	static const char*	methods[] = {
		"split",
		"join",
		"resolve",
		NULL
	};
	enum {
		M_SPLIT,
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
		case M_SPLIT: //<<<
			{
				// TODO: implement
				THROW_ERROR_LABEL(finally, code, "Not implemented yet");
			}
			break;
			//>>>
		case M_JOIN: //<<<
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
		default: Tcl_Panic("Invalid method index: %d", methodidx);
	}

finally:
	return code;
}

//>>>

#define NS	"::reuri"
struct cmd {
	char*			name;
	Tcl_ObjCmdProc*	proc;
} cmds[] = {
	{NS "::uri",	UriObjCmd},
	{NS "::query",	QueryObjCmd},
	{NS "::path",	PathObjCmd},
	{NULL,			NULL}
};
// Script API >>>

extern const TclStubs* const reuriConstStubsPtr;

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
	memset(l, 0, sizeof *l);

	l->dedup_pool = Dedup_NewPool(interp);

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

	code = Tcl_PkgProvideEx(interp, PACKAGE_NAME, PACKAGE_VERSION, reuriConstStubsPtr);
	if (code != TCL_OK) goto finally;

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
DLLEXPORT int Reuri_Unload(Tcl_Interp* interp, int flags) //<<<
{
	int					code = TCL_OK;
	Tcl_Namespace*		ns = Tcl_FindNamespace(interp, NS, NULL, TCL_GLOBAL_ONLY);

	if (ns) {
		Tcl_DeleteNamespace(ns);
		ns = NULL;
	}

	Tcl_DeleteAssocData(interp, "reuri");

	return code;
}

//>>>
DLLEXPORT int Reuri_SafeUnload(Tcl_Interp* interp, int flags) //<<<
{
	// No unsafe features
	return Reuri_Unload(interp, flags);
}

//>>>
#ifdef __cplusplus
}
#endif

// vim: foldmethod=marker foldmarker=<<<,>>> ts=4 shiftwidth=4
