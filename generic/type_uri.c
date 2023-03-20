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
	dup_uri->hosttype = uri->hosttype;
	replace_tclobj(&dup_uri->port,		uri->port);
	replace_tclobj(&dup_uri->path,		uri->path);
	replace_tclobj(&dup_uri->query,		uri->query);
	replace_tclobj(&dup_uri->fragment,	uri->fragment);

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
		// Must conform to:
		//	scheme      = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
		APPEND_PART(uri->scheme);
		Tcl_DStringAppend(ds, ":", 1);
	}

	if (uri->host && uri->hosttype != REURI_HOST_NONE) { // Only emit authority part if we have a host
		Tcl_DStringAppend(ds, "//", 2);

		if (uri->userinfo) {
			percent_encode_ds(REURI_ENCODE_USERINFO, ds, Tcl_GetString(uri->userinfo));
			Tcl_DStringAppend(ds, "@", 1);
		}

		switch (uri->hosttype) {
			case REURI_HOST_IPV6:
				// Must conform to: IP-literal = "[" ( IPv6address / IPvFuture  ) "]"
				Tcl_DStringAppend(ds, "[", 1);
				APPEND_PART(uri->host);
				Tcl_DStringAppend(ds, "]", 1);
				break;
			case REURI_HOST_IPV4:
				// Must conform to:	IPv4address = dec-octet "." dec-octet "." dec-octet "." dec-octet
				APPEND_PART(uri->host);
				break;
			case REURI_HOST_HOSTNAME:
				percent_encode_ds(REURI_ENCODE_HOST, ds, Tcl_GetString(uri->host));
				break;
			case REURI_HOST_UNIX:
				// Must confirm to: "/" pchar+ ("/" pchar+)*
				Tcl_DStringAppend(ds, "[", 1);
				APPEND_PART(uri->host);
				Tcl_DStringAppend(ds, "]", 1);
				break;
			case REURI_HOST_NONE:
				break;
			case REURI_HOST_SIZE:
				// Dummy value
				break;
		}

		if (uri->port) {
			// Must conform to:
			//	port        = *DIGIT
			Tcl_DStringAppend(ds, ":", 1);
			APPEND_PART(uri->port);
		}
	}

	if (uri->path) {
		// Must be empty or start with a /
		// First segment must be non-zero length
		// TODO: what to do if path is relative?
		APPEND_PART(uri->path);
	}

	if (uri->query) {
		Tcl_DStringAppend(ds, "?", 1);
		const int end = Tcl_DStringLength(ds);
		APPEND_PART(uri->query);
		// If the query was empty, walk back the ?
		if (Tcl_DStringLength(ds) == end) Tcl_DStringTrunc(ds, end-1);
	}

	if (uri->fragment) {
		Tcl_DStringAppend(ds, "#", 1);
		percent_encode_ds(REURI_ENCODE_FRAGMENT, ds, Tcl_GetString(uri->fragment));
	}
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
void ReuriSetURI(Tcl_Obj* uriPtr, struct uri* uri) //<<<
{
	struct uri*		newuri = NULL;
	Tcl_ObjInternalRep*	ir = Tcl_FetchInternalRep(uriPtr, &uri_objtype);

	if (ir) {
		newuri = ir->twoPtrValue.ptr1;
	} else {
		newuri = ckalloc(sizeof(struct uri));
		memset(newuri, 0, sizeof(struct uri));
	}

	replace_tclobj(&newuri->scheme,		uri->scheme);
	replace_tclobj(&newuri->userinfo,	uri->userinfo);
	replace_tclobj(&newuri->host,		uri->host);
	newuri->hosttype = uri->hosttype;
	replace_tclobj(&newuri->port,		uri->port);
	replace_tclobj(&newuri->path,		uri->path);
	replace_tclobj(&newuri->query,		uri->query);
	replace_tclobj(&newuri->fragment,	uri->fragment);

	Tcl_ObjInternalRep	newir = {.twoPtrValue.ptr1 = newuri};
	Tcl_StoreInternalRep(uriPtr, &uri_objtype, &newir);
	Tcl_InvalidateStringRep(uriPtr);
}

//>>>
// Internal API >>>

// vim: foldmethod=marker foldmarker=<<<,>>> ts=4 shiftwidth=4
