#include "reuriInt.h"
#include "tip445.h"

/* Intrep:
 *	twoPtrValue.ptr1:	struct uri*
 *	twoPtrValue.ptr2:	Not used
 */

static void free_internal_rep(Tcl_Obj* obj);
static void dup_internal_rep(Tcl_Obj* src, Tcl_Obj* dup);
static void update_string_rep(Tcl_Obj* obj);

Tcl_ObjType uri_objtype = {
	"Reuri",
	free_internal_rep,
	dup_internal_rep,
	update_string_rep,
	NULL
};

void free_uri(struct uri** uriPtrPtr) //<<<
{
	struct uri*			uri = *uriPtrPtr;

	replace_tclobj(&uri->scheme,	NULL);
	replace_tclobj(&uri->userinfo,	NULL);
	replace_tclobj(&uri->host,		NULL);
	replace_tclobj(&uri->port,		NULL);
	replace_tclobj(&uri->path,		NULL);
	replace_tclobj(&uri->query,		NULL);
	replace_tclobj(&uri->fragment,	NULL);

	ckfree(uri);
	*uriPtrPtr = NULL;
}

//>>>
static void free_internal_rep(Tcl_Obj* obj) //<<<
{
	Tcl_ObjInternalRep*	ir = Tcl_FetchInternalRep(obj, &uri_objtype);
	struct uri*			uri = (struct uri*)ir->twoPtrValue.ptr1;

	free_uri(&uri);
}

///>>>
static void dup_internal_rep(Tcl_Obj* src, Tcl_Obj* dup) //<<<
{
	Tcl_ObjInternalRep*	ir = Tcl_FetchInternalRep(src, &uri_objtype);
	Tcl_ObjInternalRep	newir;
	struct uri*			uri = (struct uri*)ir->twoPtrValue.ptr1;
	struct uri*			dup_uri = ckalloc(sizeof *dup_uri);

	memset(dup_uri, 0, sizeof *dup_uri);

	replace_tclobj(&dup_uri->scheme,	uri->scheme);
	replace_tclobj(&dup_uri->userinfo,	uri->userinfo);
	replace_tclobj(&dup_uri->host,		uri->host);
	replace_tclobj(&dup_uri->port,		uri->port);
	replace_tclobj(&dup_uri->path,		uri->path);
	replace_tclobj(&dup_uri->query,		uri->query);

	newir.twoPtrValue.ptr1 = dup_uri;
	Tcl_StoreInternalRep(dup, &uri_objtype, &newir);
}

//>>>
static void update_string_rep(Tcl_Obj* obj) //<<<
{
	Tcl_ObjInternalRep*	ir = Tcl_FetchInternalRep(obj, &uri_objtype);
	struct uri*			uri = (struct uri*)ir->twoPtrValue.ptr1;
	Tcl_DString			ds;

	Tcl_DStringInit(&ds);
	ReuriCompile(&ds, uri);
	Tcl_InitStringRep(obj, Tcl_DStringValue(&ds), Tcl_DStringLength(&ds));
	Tcl_DStringFree(&ds);
}

//>>>

// Internal API <<<
void ReuriCompile(Tcl_DString* ds, struct uri* uri) //<<<
{
#define APPEND_PART(part) \
	do { \
		int len; \
		const char*	str = Tcl_GetStringFromObj(part, &len); \
		Tcl_DStringAppend(ds, str, len); \
	} while(0)

	if (uri->scheme) {
		APPEND_PART(uri->scheme);
		Tcl_DStringAppend(ds, "://", 3);
	}

	if (uri->userinfo) {
		APPEND_PART(uri->userinfo);
		Tcl_DStringAppend(ds, "@", 1);
	}

	// TODO: complete
}

//>>>
int ReuriGetURIFromObj(Tcl_Interp* interp, Tcl_Obj* uriPtr, struct uri** uri) //<<<
{
	int					code = TCL_OK;
	Tcl_ObjInternalRep*	ir = Tcl_FetchInternalRep(uriPtr, &uri_objtype);
	struct uri*			newuri = NULL;

	if (ir == NULL) {
		Tcl_ObjInternalRep	newir;
		const char*			uristr;
		int					uristr_len;
		struct parse_context	pc = {
			.interp			= interp,
			.rc				= TCL_OK
		};

		uristr = Tcl_GetStringFromObj(uriPtr, &uristr_len);
		pc.uri = newuri = (struct uri*)ckalloc(sizeof *newuri);
		memset(newuri, 0, sizeof *newuri);

		parse_uri(&pc, uristr);
		code = pc.rc;
		if (pc.rc != TCL_OK) goto finally;

		memset(&newir, 0, sizeof newir);
		newir.twoPtrValue.ptr1 = newuri;
		newuri = NULL;
		pc.uri = NULL;

		Tcl_StoreInternalRep(uriPtr, &uri_objtype, &newir);
		ir = Tcl_FetchInternalRep(uriPtr, &uri_objtype);
	}

	*uri = (struct uri*)ir->twoPtrValue.ptr1;

finally:
	if (newuri) free_uri(&newuri);

	return code;
}

//>>>
// Internal API >>>

// vim: foldmethod=marker foldmarker=<<<,>>> ts=4 shiftwidth=4
