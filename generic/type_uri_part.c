#include "reuriInt.h"
#include "tip445.h"

/* Intrep:
 *	twoPtrValue.ptr1:	Tcl_Obj storing decoded value
 *	twoPtrValue.ptr2:	Tcl_Obj storing the normalized encoded representation	(either, but not both of ptr1 or ptr2 may be null)
 * String rep:
 *	encoded form that is valid according to the rules of RFC 3986 for the URI part represented
 */

#define DECODED(irPtr)			((Tcl_Obj*)((irPtr)->twoPtrValue.ptr1))
#define DECODED_PTR(irPtr)		((Tcl_Obj**)(&((irPtr)->twoPtrValue.ptr1)))
#define NORMALIZED(irPtr)		((Tcl_Obj*)((irPtr)->twoPtrValue.ptr2))
#define NORMALIZED_PTR(irPtr)	((Tcl_Obj**)(&((irPtr)->twoPtrValue.ptr2)))

#if 0
/* Intrep:
 *	ptrAndLongRep.ptr:		Tcl_Obj storing decoded value
 *	ptrAndLongRep.value:	normalized || enum reuri_part.  normalized is true if stringrep is in normalized form
 * String rep:
 *	encoded form that is valid according to the rules of RFC 3986 for the URI part represented
 */

#define NORMALIZED		(1 << (sizeof(long)*8-1))
#define PART_MASK		(~NORMALIZED)

#define DECODED(irPtr)			((Tcl_Obj*)((irPtr)->ptrAndLongRep.ptr))
#define DECODED_PTR(irPtr)		((Tcl_Obj**)(&((irPtr)->ptrAndLongRep.ptr)))
static inline int				is_normalized(Tcl_ObjInternalRep* ir)					{ return (ir->ptrAndLongRep & NORMALIZED) != 0; }
static inline enum reuri_part	get_part(Tcl_ObjInternalRep* ir)						{ return  ir->ptrAndLongRep & PART_MASK;         }
static void						set_normalized(Tcl_ObjInternalRep* ir, int normalized)	{ ir->ptrAndLongRep = (ir->ptrAndLongRep &  PART_MASK) | ((normalized!=0) << NORMALIZED); }
static void						set_part(Tcl_ObjInternalRep* ir, enum reuri_part part)	{ ir->ptrAndLongRep = (ir->ptrAndLongRep & ~PART_MASK) | (part & PART_MASK);              }
#endif

static void free_internal_rep(Tcl_Obj* obj, Reuri_ObjType* objtype);
static void dup_internal_rep(Tcl_Obj* src, Tcl_Obj* dup, Reuri_ObjType* objtype);
static void update_string_rep(Tcl_Obj* obj, Reuri_ObjType* objtype);

static void free_internal_rep_scheme(Tcl_Obj* obj)				{ free_internal_rep(obj, &scheme_objtype);     }
static void dup_internal_rep_scheme(Tcl_Obj* src, Tcl_Obj* dup)	{ dup_internal_rep(src, dup, &scheme_objtype); }
static void update_string_rep_scheme(Tcl_Obj* obj)				{ update_string_rep(obj, &scheme_objtype);     }
static int update_decoded_rep_scheme(Tcl_Interp* interp, Tcl_Obj* obj);
static int update_normalized_rep_scheme(Tcl_Interp* interp, Tcl_Obj* obj);

static void free_internal_rep_userinfo(Tcl_Obj* obj)				{ free_internal_rep(obj, &userinfo_objtype);     }
static void dup_internal_rep_userinfo(Tcl_Obj* src, Tcl_Obj* dup)	{ dup_internal_rep(src, dup, &userinfo_objtype); }
static void update_string_rep_userinfo(Tcl_Obj* obj)				{ update_string_rep(obj, &userinfo_objtype);     }
static int update_decoded_rep_userinfo(Tcl_Interp* interp, Tcl_Obj* obj);
static int update_normalized_rep_userinfo(Tcl_Interp* interp, Tcl_Obj* obj);

static void free_internal_rep_host_reg_name(Tcl_Obj* obj)				{ free_internal_rep(obj, &host_reg_name_objtype);     }
static void dup_internal_rep_host_reg_name(Tcl_Obj* src, Tcl_Obj* dup)	{ dup_internal_rep(src, dup, &host_reg_name_objtype); }
static void update_string_rep_host_reg_name(Tcl_Obj* obj)				{ update_string_rep(obj, &host_reg_name_objtype);     }
static int update_decoded_rep_host_reg_name(Tcl_Interp* interp, Tcl_Obj* obj);
static int update_normalized_rep_host_reg_name(Tcl_Interp* interp, Tcl_Obj* obj);

static void free_internal_rep_host_ipv4(Tcl_Obj* obj)				{ free_internal_rep(obj, &host_ipv4_objtype);     }
static void dup_internal_rep_host_ipv4(Tcl_Obj* src, Tcl_Obj* dup)	{ dup_internal_rep(src, dup, &host_ipv4_objtype); }
static void update_string_rep_host_ipv4(Tcl_Obj* obj)				{ update_string_rep(obj, &host_ipv4_objtype);     }
static int update_decoded_rep_host_ipv4(Tcl_Interp* interp, Tcl_Obj* obj);
static int update_normalized_rep_host_ipv4(Tcl_Interp* interp, Tcl_Obj* obj);

static void free_internal_rep_host_ipv6(Tcl_Obj* obj)				{ free_internal_rep(obj, &host_ipv6_objtype);     }
static void dup_internal_rep_host_ipv6(Tcl_Obj* src, Tcl_Obj* dup)	{ dup_internal_rep(src, dup, &host_ipv6_objtype); }
static void update_string_rep_host_ipv6(Tcl_Obj* obj)				{ update_string_rep(obj, &host_ipv6_objtype);     }
static int update_decoded_rep_host_ipv6(Tcl_Interp* interp, Tcl_Obj* obj);
static int update_normalized_rep_host_ipv6(Tcl_Interp* interp, Tcl_Obj* obj);

static void free_internal_rep_host_local(Tcl_Obj* obj)				{ free_internal_rep(obj, &host_local_objtype);     }
static void dup_internal_rep_host_local(Tcl_Obj* src, Tcl_Obj* dup)	{ dup_internal_rep(src, dup, &host_local_objtype); }
static void update_string_rep_host_local(Tcl_Obj* obj)				{ update_string_rep(obj, &host_local_objtype);     }
static int update_decoded_rep_host_local(Tcl_Interp* interp, Tcl_Obj* obj);
static int update_normalized_rep_host_local(Tcl_Interp* interp, Tcl_Obj* obj);

static void free_internal_rep_port(Tcl_Obj* obj)				{ free_internal_rep(obj, &port_objtype);     }
static void dup_internal_rep_port(Tcl_Obj* src, Tcl_Obj* dup)	{ dup_internal_rep(src, dup, &port_objtype); }
static void update_string_rep_port(Tcl_Obj* obj)				{ update_string_rep(obj, &port_objtype);     }
static int update_decoded_rep_port(Tcl_Interp* interp, Tcl_Obj* obj);
static int update_normalized_rep_port(Tcl_Interp* interp, Tcl_Obj* obj);

static void free_internal_rep_path(Tcl_Obj* obj)				{ free_internal_rep(obj, &path_objtype);     }
static void dup_internal_rep_path(Tcl_Obj* src, Tcl_Obj* dup)	{ dup_internal_rep(src, dup, &path_objtype); }
static void update_string_rep_path(Tcl_Obj* obj)				{ update_string_rep(obj, &path_objtype);     }
static int update_decoded_rep_path(Tcl_Interp* interp, Tcl_Obj* obj);
static int update_normalized_rep_path(Tcl_Interp* interp, Tcl_Obj* obj);

static void free_internal_rep_query(Tcl_Obj* obj)				{ free_internal_rep(obj, &query_objtype);     }
static void dup_internal_rep_query(Tcl_Obj* src, Tcl_Obj* dup)	{ dup_internal_rep(src, dup, &query_objtype); }
static void update_string_rep_query(Tcl_Obj* obj)				{ update_string_rep(obj, &query_objtype);     }
static int update_decoded_rep_query(Tcl_Interp* interp, Tcl_Obj* obj);
static int update_normalized_rep_query(Tcl_Interp* interp, Tcl_Obj* obj);

static void free_internal_rep_fragment(Tcl_Obj* obj)				{ free_internal_rep(obj, &fragment_objtype);     }
static void dup_internal_rep_fragment(Tcl_Obj* src, Tcl_Obj* dup)	{ dup_internal_rep(src, dup, &fragment_objtype); }
static void update_string_rep_fragment(Tcl_Obj* obj)				{ update_string_rep(obj, &fragment_objtype);     }
static int update_decoded_rep_fragment(Tcl_Interp* interp, Tcl_Obj* obj);
static int update_normalized_rep_fragment(Tcl_Interp* interp, Tcl_Obj* obj);

Reuri_ObjType scheme_objtype = {
	.base = {
		"ReuriScheme",
		free_internal_rep_scheme,
		dup_internal_rep_scheme,
		update_string_rep_scheme,
		NULL
	},
	update_decoded_rep_scheme,
	update_normalized_rep_scheme
};
Reuri_ObjType userinfo_objtype = {
	.base = {
		"ReuriUserinfo",
		free_internal_rep_userinfo,
		dup_internal_rep_userinfo,
		update_string_rep_userinfo,
		NULL
	},
	update_decoded_rep_userinfo,
	update_normalized_rep_userinfo
};
Reuri_ObjType host_reg_name_objtype = {
	.base = {
		"ReuriHost",
		free_internal_rep_host_reg_name,
		dup_internal_rep_host_reg_name,
		update_string_rep_host_reg_name,
		NULL
	},
	update_decoded_rep_host_reg_name,
	update_normalized_rep_host_reg_name
};
Reuri_ObjType host_ipv4_objtype = {
	.base = {
		"ReuriHost",
		free_internal_rep_host_ipv4,
		dup_internal_rep_host_ipv4,
		update_string_rep_host_ipv4,
		NULL
	},
	update_decoded_rep_host_ipv4,
	update_normalized_rep_host_ipv4
};
Reuri_ObjType host_ipv6_objtype = {
	.base = {
		"ReuriHost",
		free_internal_rep_host_ipv6,
		dup_internal_rep_host_ipv6,
		update_string_rep_host_ipv6,
		NULL
	},
	update_decoded_rep_host_ipv6,
	update_normalized_rep_host_ipv6
};
Reuri_ObjType host_local_objtype = {
	.base = {
		"ReuriHost",
		free_internal_rep_host_local,
		dup_internal_rep_host_local,
		update_string_rep_host_local,
		NULL
	},
	update_decoded_rep_host_local,
	update_normalized_rep_host_local
};
Reuri_ObjType port_objtype = {
	.base = {
		"ReuriPort",
		free_internal_rep_port,
		dup_internal_rep_port,
		update_string_rep_port,
		NULL
	},
	update_decoded_rep_port,
	update_normalized_rep_port
};
Reuri_ObjType path_objtype = {
	.base = {
		"ReuriPath",
		free_internal_rep_path,
		dup_internal_rep_path,
		update_string_rep_path,
		NULL
	},
	update_decoded_rep_path,
	update_normalized_rep_path
};
Reuri_ObjType query_objtype = {
	.base = {
		"ReuriQuery",
		free_internal_rep_query,
		dup_internal_rep_query,
		update_string_rep_query,
		NULL
	},
	update_decoded_rep_query,
	update_normalized_rep_query
};
Reuri_ObjType fragment_objtype = {
	.base = {
		"ReuriFragment",
		free_internal_rep_fragment,
		dup_internal_rep_fragment,
		update_string_rep_fragment,
		NULL
	},
	update_decoded_rep_fragment,
	update_normalized_rep_fragment
};

static void free_internal_rep(Tcl_Obj* obj, Reuri_ObjType* objtype) //<<<
{
	Tcl_ObjInternalRep*		ir = Tcl_FetchInternalRep(obj, (Tcl_ObjType*)objtype);
	if (ir) {
		release_tclobj(DECODED_PTR(ir));
		release_tclobj(NORMALIZED_PTR(ir));
	}
}

///>>>
static void dup_internal_rep(Tcl_Obj* src, Tcl_Obj* dup, Reuri_ObjType* objtype) //<<<
{
	Tcl_ObjInternalRep*		ir = Tcl_FetchInternalRep(src, (Tcl_ObjType*)objtype);
	Tcl_ObjInternalRep		newir = {0};

	replace_tclobj(DECODED_PTR(&newir),		DECODED(ir));
	replace_tclobj(NORMALIZED_PTR(&newir),	NORMALIZED(ir));

	Tcl_StoreInternalRep(dup, (Tcl_ObjType*)objtype, &newir);
}

//>>>
static void update_string_rep(Tcl_Obj* obj, Reuri_ObjType* objtype) //<<<
{
	Tcl_Obj*	norm = NULL;
	int			len;
	const char*	str;

	if (TCL_OK != Reuri_GetNormalizedFromPart(NULL, obj, objtype, &norm))
		Tcl_Panic("Could not get normalized representation of URI part");

	str = Tcl_GetStringFromObj(norm, &len);
	Tcl_InitStringRep(obj, str, len);
	replace_tclobj(&norm, NULL);
}

//>>>

static int update_decoded_rep_scheme(Tcl_Interp* interp, Tcl_Obj* obj) //<<<
{
	Tcl_ObjInternalRep*		ir = Tcl_FetchInternalRep(obj, (Tcl_ObjType*)&scheme_objtype);
	Tcl_DString	ds;
	Tcl_DStringInit(&ds);
	ascii_lowercase_ds(&ds, Tcl_GetString(obj));
	replace_tclobj(DECODED_PTR(ir), Tcl_DStringToObj(&ds));

	return TCL_OK;
}

//>>>
static int update_normalized_rep_scheme(Tcl_Interp* interp, Tcl_Obj* obj) //<<<
{
	Tcl_ObjInternalRep*		ir = Tcl_FetchInternalRep(obj, (Tcl_ObjType*)&scheme_objtype);
	int						code = TCL_OK;
	Tcl_Obj*				norm = NULL;

	TEST_OK_LABEL(finally, code, Reuri_GetDecodedFromPart(interp, obj, &scheme_objtype, &norm));
	replace_tclobj(NORMALIZED_PTR(ir), norm);

finally:
	replace_tclobj(&norm, NULL);
	return code;
}

//>>>

static int update_decoded_rep_userinfo(Tcl_Interp* interp, Tcl_Obj* obj) //<<<
{
	Tcl_ObjInternalRep*		ir = Tcl_FetchInternalRep(obj, (Tcl_ObjType*)&userinfo_objtype);
	Tcl_DString				ds;

	Tcl_DStringInit(&ds);
	percent_decode_ds(Tcl_GetString(obj), &ds);
	replace_tclobj(DECODED_PTR(ir), Tcl_DStringToObj(&ds));

	return TCL_OK;
}

//>>>
static int update_normalized_rep_userinfo(Tcl_Interp* interp, Tcl_Obj* obj) //<<<
{
	Tcl_ObjInternalRep*		ir = Tcl_FetchInternalRep(obj, (Tcl_ObjType*)&userinfo_objtype);
	int						code = TCL_OK;
	Tcl_Obj*				norm = NULL;

	TEST_OK_LABEL(finally, code, Reuri_GetDecodedFromPart(interp, obj, &userinfo_objtype, &norm));
	replace_tclobj(NORMALIZED_PTR(ir), percent_encode(interp, norm, REURI_ENCODE_USERINFO));

finally:
	replace_tclobj(&norm, NULL);
	return code;
}

//>>>

static int update_decoded_rep_host_reg_name(Tcl_Interp* interp, Tcl_Obj* obj) //<<<
{
	Tcl_ObjInternalRep*		ir = Tcl_FetchInternalRep(obj, (Tcl_ObjType*)&host_reg_name_objtype);
	Tcl_DString				ds;

	Tcl_DStringInit(&ds);
	percent_decode_ds(Tcl_GetString(obj), &ds);
	replace_tclobj(DECODED_PTR(ir), Tcl_DStringToObj(&ds));

	return TCL_OK;
}

//>>>
static int update_normalized_rep_host_reg_name(Tcl_Interp* interp, Tcl_Obj* obj) //<<<
{
	Tcl_ObjInternalRep*		ir = Tcl_FetchInternalRep(obj, (Tcl_ObjType*)&host_reg_name_objtype);
	int						code = TCL_OK;
	Tcl_Obj*				norm = NULL;

	TEST_OK_LABEL(finally, code, Reuri_GetDecodedFromPart(interp, obj, &host_reg_name_objtype, &norm));
	replace_tclobj(NORMALIZED_PTR(ir), percent_encode(interp, norm, REURI_ENCODE_HOST));

finally:
	replace_tclobj(&norm, NULL);
	return code;
}

//>>>

static int update_decoded_rep_host_ipv4(Tcl_Interp* interp, Tcl_Obj* obj) //<<<
{
	Tcl_ObjInternalRep*		ir = Tcl_FetchInternalRep(obj, (Tcl_ObjType*)&host_ipv4_objtype);
	replace_tclobj(DECODED_PTR(ir), Tcl_DuplicateObj(obj));

	return TCL_OK;
}

//>>>
static int update_normalized_rep_host_ipv4(Tcl_Interp* interp, Tcl_Obj* obj) //<<<
{
	Tcl_ObjInternalRep*		ir = Tcl_FetchInternalRep(obj, (Tcl_ObjType*)&host_ipv4_objtype);
	int						code = TCL_OK;
	Tcl_Obj*				norm = NULL;

	TEST_OK_LABEL(finally, code, Reuri_GetDecodedFromPart(interp, obj, &host_ipv4_objtype, &norm));
	replace_tclobj(NORMALIZED_PTR(ir), norm);

finally:
	replace_tclobj(&norm, NULL);
	return code;
}

//>>>

static int update_decoded_rep_host_ipv6(Tcl_Interp* interp, Tcl_Obj* obj) //<<<
{
	Tcl_ObjInternalRep*		ir = Tcl_FetchInternalRep(obj, (Tcl_ObjType*)&host_ipv6_objtype);
	if (TCL_OK != parse_host_ipv6(interp, Tcl_GetString(obj), DECODED_PTR(ir)))
		Tcl_Panic("Cannot parse IPv6 host part representation: \"%s\"", Tcl_GetString(obj));

	return TCL_OK;
}

//>>>
static int update_normalized_rep_host_ipv6(Tcl_Interp* interp, Tcl_Obj* obj) //<<<
{
	Tcl_ObjInternalRep*		ir = Tcl_FetchInternalRep(obj, (Tcl_ObjType*)&host_ipv6_objtype);
	int						len;
	const char*				str = Tcl_GetStringFromObj(DECODED(ir), &len);
	Tcl_DString				ds;
	Tcl_DStringInit(&ds);
	Tcl_DStringAppend(&ds, "[", 1);
	// TODO: normalize IPv6 representation - zeros to ::, IPv4-in-IPv6, etc?
	Tcl_DStringAppend(&ds, str, len);
	Tcl_DStringAppend(&ds, "]", 1);
	replace_tclobj(NORMALIZED_PTR(ir), Tcl_DStringToObj(&ds));

	return TCL_OK;
}

//>>>

static int update_decoded_rep_host_local(Tcl_Interp* interp, Tcl_Obj* obj) //<<<
{
	int						code = TCL_OK;
	Tcl_ObjInternalRep*		ir = Tcl_FetchInternalRep(obj, (Tcl_ObjType*)&host_local_objtype);

	if (TCL_OK != (code = parse_host_local(interp, Tcl_GetString(obj), DECODED_PTR(ir)))) {
		if (interp) goto finally; else Tcl_Panic("Cannot parse local socket representation: \"%s\"", Tcl_GetString(obj));
	}

finally:
	return code;
}

//>>>
static int update_normalized_rep_host_local(Tcl_Interp* interp, Tcl_Obj* obj) //<<<
{
	int						code = TCL_OK;
	Tcl_ObjInternalRep*		ir = Tcl_FetchInternalRep(obj, (Tcl_ObjType*)&host_local_objtype);
	Tcl_DString				ds;
	Tcl_Obj**				pathv = NULL;
	int						pathc;
	int						i;

	if (TCL_OK != (code = Tcl_ListObjGetElements(interp, DECODED(ir), &pathc, &pathv))) {
		if (interp) goto finally; else Tcl_Panic("Couldn't interpret rep_host_local decoded value as a list");
	}

	Tcl_DStringInit(&ds);
	//Tcl_DStringAppend(&ds, "[v0.local:", 10);
	Tcl_DStringAppend(&ds, "[", 1);

	i = 0;
	if (pathc >= 1 && strcmp(Tcl_GetString(pathv[i++]), "/") != 0)
		percent_encode_ds(REURI_ENCODE_HOST, &ds, Tcl_GetString(pathv[i-1]));

	for (; i<pathc; i++) {
		Tcl_DStringAppend(&ds, "/", 1);
		percent_encode_ds(REURI_ENCODE_HOST, &ds, Tcl_GetString(pathv[i]));
	}

	Tcl_DStringAppend(&ds, "]", 1);

	replace_tclobj(NORMALIZED_PTR(ir), Tcl_DStringToObj(&ds));

finally:
	return code;
}

//>>>

static int update_decoded_rep_port(Tcl_Interp* interp, Tcl_Obj* obj) //<<<
{
	int						code = TCL_OK;
	int						portnum;
	Tcl_ObjInternalRep*		ir = Tcl_FetchInternalRep(obj, (Tcl_ObjType*)&port_objtype);

	if (TCL_OK != (code = decode_port(interp, Tcl_GetString(obj), &portnum))) {
		if (interp) goto finally; else Tcl_Panic("Couldn't parse port number from \"%s\"", Tcl_GetString(obj));
	}

	replace_tclobj(DECODED_PTR(ir), Tcl_NewIntObj(portnum));

finally:
	return code;
}

//>>>
static int update_normalized_rep_port(Tcl_Interp* interp, Tcl_Obj* obj) //<<<
{
	int						code = TCL_OK;
	int						portnum;
	Tcl_ObjInternalRep*		ir = Tcl_FetchInternalRep(obj, (Tcl_ObjType*)&port_objtype);

	if (TCL_OK != (code = Tcl_GetIntFromObj(interp, DECODED(ir), &portnum))) {
		if (interp) goto finally; else Tcl_Panic("Couldn't retrieve port number from \"%s\"", Tcl_GetString(DECODED(ir)));
	}

	replace_tclobj(NORMALIZED_PTR(ir), Tcl_ObjPrintf("%d", portnum));

finally:
	return code;
}

//>>>

static int update_decoded_rep_path(Tcl_Interp* interp, Tcl_Obj* obj) //<<<
{
	int						code = TCL_OK;
	Tcl_ObjInternalRep*		ir = Tcl_FetchInternalRep(obj, (Tcl_ObjType*)&path_objtype);

	if (TCL_OK != (code = decode_path(interp, Tcl_GetString(obj), DECODED_PTR(ir)))) {
		if (interp) goto finally; else Tcl_Panic("Cannot parse local socket representation: \"%s\"", Tcl_GetString(obj));
	}

finally:
	return code;
}

//>>>
static int update_normalized_rep_path(Tcl_Interp* interp, Tcl_Obj* obj) //<<<
{
	int						code = TCL_OK;
	Tcl_ObjInternalRep*		ir = Tcl_FetchInternalRep(obj, (Tcl_ObjType*)&path_objtype);
	Tcl_DString				ds;

	Tcl_DStringInit(&ds);

	if (TCL_OK != (code = Reuri_CompilePath(interp, &ds, DECODED(ir)))) {
		if (interp) goto finally; else Tcl_Panic("Couldn't interpret path decoded value as a list");
	}

	replace_tclobj(NORMALIZED_PTR(ir), Tcl_DStringToObj(&ds));

finally:
	return code;
}

//>>>

static int update_decoded_rep_query(Tcl_Interp* interp, Tcl_Obj* obj) //<<<
{
	int						code = TCL_OK;
	Tcl_ObjInternalRep*		ir = Tcl_FetchInternalRep(obj, (Tcl_ObjType*)&query_objtype);
	Tcl_Obj*				irv[2] = {0};

	/* Intrep:
	 *	irv[0]:	Tcl list with param -> value pairs
	 *	irv[1]:	Tcl dict mapping parameter names to a list of
	 *			positions that those params occur.  For a=x&b=y&a=z
	 *			a -> {0 2}, b -> 1
	 */

	if (TCL_OK != (code = decode_query(interp, Tcl_GetString(obj), &irv[0], &irv[1]))) {
		if (interp) goto finally; else Tcl_Panic("Cannot parse query: \"%s\"", Tcl_GetString(obj));
	}

	replace_tclobj(DECODED_PTR(ir), Tcl_NewListObj(irv[1] ? 2 : 1, irv));

	replace_tclobj(&irv[0], NULL);
	replace_tclobj(&irv[1], NULL);

finally:
	return code;
}

//>>>
static int update_normalized_rep_query(Tcl_Interp* interp, Tcl_Obj* obj) //<<<
{
	int						code = TCL_OK;
	Tcl_ObjInternalRep*		ir = Tcl_FetchInternalRep(obj, (Tcl_ObjType*)&query_objtype);
	Tcl_DString				ds;
	Tcl_Obj**				irv = NULL;
	int						irc;

	if (TCL_OK != (code = Tcl_ListObjGetElements(interp, DECODED(ir), &irc, &irv))) {
		if (interp) goto finally; else Tcl_Panic("Couldn't interpret query decoded value as a list");
	}
	if (irc < 1) {
		if (interp) {
			THROW_ERROR_LABEL(finally, code, "Corrupt query interp list, needs at least 1 element");
		} else {
			Tcl_Panic("Corrupt query intrep list, needs at least 1 element");
		}
	}

	Tcl_DStringInit(&ds);

	if (TCL_OK != (code = Reuri_CompileQuery(interp, &ds, irv[0]))) {
		if (interp) goto finally; else Tcl_Panic("Error compiling query params: \"%s\"", Tcl_GetString(irv[0]));
	}

	replace_tclobj(NORMALIZED_PTR(ir), Tcl_DStringToObj(&ds));

finally:
	return code;
}

//>>>

static int update_decoded_rep_fragment(Tcl_Interp* interp, Tcl_Obj* obj) //<<<
{
	Tcl_ObjInternalRep*		ir = Tcl_FetchInternalRep(obj, (Tcl_ObjType*)&fragment_objtype);
	Tcl_DString				ds;

	Tcl_DStringInit(&ds);
	percent_decode_ds(Tcl_GetString(obj), &ds);
	replace_tclobj(DECODED_PTR(ir), Tcl_DStringToObj(&ds));

	return TCL_OK;
}

//>>>
static int update_normalized_rep_fragment(Tcl_Interp* interp, Tcl_Obj* obj) //<<<
{
	Tcl_ObjInternalRep*		ir = Tcl_FetchInternalRep(obj, (Tcl_ObjType*)&fragment_objtype);
	int						code = TCL_OK;
	Tcl_Obj*				norm = NULL;

	TEST_OK_LABEL(finally, code, Reuri_GetDecodedFromPart(interp, obj, &fragment_objtype, &norm));
	replace_tclobj(NORMALIZED_PTR(ir), percent_encode(interp, norm, REURI_ENCODE_FRAGMENT));

finally:
	replace_tclobj(&norm, NULL);
	return code;
}

//>>>

// Internal API <<<
int Reuri_GetDecodedFromPart(Tcl_Interp* interp, Tcl_Obj* obj, Reuri_ObjType* objtype, Tcl_Obj** decoded) //<<<
{
	int						code = TCL_OK;
	Tcl_ObjInternalRep*		ir = Tcl_FetchInternalRep(obj, (Tcl_ObjType*)objtype);

	if (!ir) {
		Tcl_ObjInternalRep	newir = {0};
		Tcl_GetString(obj);	// Ensure the string rep is available
		Tcl_StoreInternalRep(obj, (Tcl_ObjType*)objtype, &newir);
		ir = Tcl_FetchInternalRep(obj, (Tcl_ObjType*)objtype);
	}

	if (DECODED(ir) == NULL)
		TEST_OK_LABEL(finally, code, objtype->Reuri_UpdateDecodedProc(interp, obj));

	if (objtype == &query_objtype) {
		Tcl_Obj**	irv = NULL;
		int			irc;
		TEST_OK_LABEL(finally, code, Tcl_ListObjGetElements(interp, DECODED(ir), &irc, &irv));
		if (irc < 1) THROW_ERROR_LABEL(finally, code, "Corrupt uri query decoded intrep, must have at least 1 element");
		replace_tclobj(decoded, irv[0]);
	} else {
		replace_tclobj(decoded, DECODED(ir));
	}

finally:
	return code;
}

//>>>
int Reuri_GetNormalizedFromPart(Tcl_Interp* interp, Tcl_Obj* obj, Reuri_ObjType* objtype, Tcl_Obj** normalized) //<<<
{
	int						code = TCL_OK;
	Tcl_ObjInternalRep*		ir = Tcl_FetchInternalRep(obj, (Tcl_ObjType*)objtype);

	if (!ir) {
		Tcl_ObjInternalRep	newir = {0};
		Tcl_GetString(obj);	// Ensure the string rep is available
		Tcl_StoreInternalRep(obj, (Tcl_ObjType*)objtype, &newir);
		ir = Tcl_FetchInternalRep(obj, (Tcl_ObjType*)objtype);
	}

	if (DECODED(ir) == NULL)
		TEST_OK_LABEL(finally, code, objtype->Reuri_UpdateDecodedProc(interp, obj));

	if (NORMALIZED(ir) == NULL)
		TEST_OK_LABEL(finally, code, objtype->Reuri_UpdateNormalizedProc(interp, obj));

	replace_tclobj(normalized, NORMALIZED(ir));

finally:
	return code;
}

//>>>
Reuri_ObjType* host_objtype(enum reuri_hosttype hosttype) //<<<
{
	switch (hosttype) {
		case REURI_HOST_IPV6:		return &host_ipv6_objtype;
		case REURI_HOST_IPV4:		return &host_ipv4_objtype;
		case REURI_HOST_HOSTNAME:	return &host_reg_name_objtype;
		case REURI_HOST_UNIX:		return &host_local_objtype;
		default:					return NULL;
	}
}

//>>>
Tcl_Obj* Reuri_NewPartFromString(Reuri_ObjType* objtype, const char* str, int len) //<<<
{
	Tcl_ObjInternalRep	newir = {0};

	Tcl_Obj*	res = Tcl_NewStringObj(str, len);
	Tcl_StoreInternalRep(res, (Tcl_ObjType*)objtype, &newir);
	return res;
}

//>>>
int Reuri_GetPathFromObj(Tcl_Interp* interp, Tcl_Obj* pathPtr, Tcl_Obj** pathlistPtrPtr) //<<<
{
	int					code = TCL_OK;
	Tcl_ObjInternalRep*	ir = Tcl_FetchInternalRep(pathPtr, (Tcl_ObjType*)&path_objtype);

	if (ir == NULL) {
		Tcl_ObjInternalRep	newir = {0};

		code = decode_path(interp, Tcl_GetString(pathPtr), DECODED_PTR(&newir));
		if (code != TCL_OK) {
			replace_tclobj(DECODED_PTR(&newir), NULL);
			replace_tclobj(NORMALIZED_PTR(&newir), NULL);
			goto finally;
		}

		Tcl_StoreInternalRep(pathPtr, (Tcl_ObjType*)&path_objtype, &newir);
		ir = Tcl_FetchInternalRep(pathPtr, (Tcl_ObjType*)&path_objtype);
	}

	if (pathlistPtrPtr) replace_tclobj(pathlistPtrPtr, DECODED(ir));

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

	if (strcmp(Tcl_GetString(ov[0]), "/") != 0) {
		percent_encode_ds(REURI_ENCODE_PATH, ds, Tcl_GetString(ov[0]));
	} else if (oc == 1) {
		Tcl_DStringAppend(ds, "/", 1);
	}

	for (i=1; i<oc; i++) {
		Tcl_DStringAppend(ds, "/", 1);
		percent_encode_ds(REURI_ENCODE_PATH2, ds, Tcl_GetString(ov[i]));
	}

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
int ReuriRebuildIndex(Tcl_Interp* interp, Tcl_Obj* params, Tcl_Obj** index) //<<<
{
	int					code = TCL_OK;
	Tcl_Obj*			res = NULL;
	Tcl_Obj**			pv = NULL;
	int					pc;
	
	replace_tclobj(&res, Tcl_NewDictObj());

	TEST_OK_LABEL(finally, code, Tcl_ListObjGetElements(interp, params, &pc, &pv));
	if (pc % 2 != 0) THROW_ERROR_LABEL(finally, code, "params list has an odd number of elements");

	for (int i=0; i<pc/2; i++)
		TEST_OK_LABEL(finally, code, query_add_index(interp, res, pv[i*2], i));

	replace_tclobj(index, res);

finally:
	replace_tclobj(&res, NULL);
	return code;
}

//>>>
int ReuriGetQueryFromObj(Tcl_Interp* interp, Tcl_Obj* query, Tcl_Obj** params, Tcl_Obj** index) //<<<
{
	int					code = TCL_OK;
	Tcl_ObjInternalRep*	ir = Tcl_FetchInternalRep(query, (Tcl_ObjType*)&query_objtype);
	Tcl_Obj*			newidx = NULL;

	if (ir == NULL) {
		Tcl_ObjInternalRep	newir = {0};
		const char*			str = Tcl_GetString(query);
		const char*			s = str[0] == '?' ? str+1 : str;

		Tcl_Obj*	irv[2] = {0};
		code = decode_query(interp, s, &irv[0], &irv[1]);
		if (code != TCL_OK) {
			replace_tclobj(&irv[0], NULL);
			replace_tclobj(&irv[1], NULL);
			goto finally;
		}
		replace_tclobj(DECODED_PTR(&newir), Tcl_NewListObj(irv[1]?2:1, irv));
		replace_tclobj(&irv[0], NULL);
		replace_tclobj(&irv[1], NULL);

		Tcl_StoreInternalRep(query, (Tcl_ObjType*)&query_objtype, &newir);
		if (s > str) Tcl_InvalidateStringRep(query);	// A leading ? was trimmed, have to discard the old stringrep
		ir = Tcl_FetchInternalRep(query, (Tcl_ObjType*)&query_objtype);
	}

	{
		int			irc;
		Tcl_Obj**	irv = NULL;
		TEST_OK_LABEL(finally, code, Tcl_ListObjGetElements(interp, DECODED(ir), &irc, &irv));
		if (irc < 1 || irc > 2)
			THROW_ERROR_LABEL(finally, code, "Corrupted query intrep: decoded list should have 1 or 2 elements");

		if (index) {
			if (irc < 2) {
				TEST_OK_LABEL(finally, code, ReuriRebuildIndex(interp, irv[0], &newidx));

				if (Tcl_IsShared(DECODED(ir)))
					replace_tclobj(DECODED_PTR(ir), Tcl_DuplicateObj(DECODED(ir)));

				TEST_OK_LABEL(finally, code, Tcl_ListObjAppendElement(interp, DECODED(ir), newidx));
				TEST_OK_LABEL(finally, code, Tcl_ListObjGetElements(interp, DECODED(ir), &irc, &irv));
			}
			replace_tclobj(index, irv[1]);
		}

		if (params) replace_tclobj(params, irv[0]);
	}

finally:
	replace_tclobj(&newidx, NULL);
	return code;
}

//>>>
void ReuriSetQuery(Tcl_Obj* query, Tcl_Obj* params, Tcl_Obj* index) //<<<
{
	Tcl_ObjInternalRep	newir = {0};
	Tcl_Obj*			irv[2] = {params, index};

	replace_tclobj(DECODED_PTR(&newir), Tcl_NewListObj(index?2:1, irv));

	Tcl_StoreInternalRep(query, (Tcl_ObjType*)&query_objtype, &newir);
	Tcl_InvalidateStringRep(query);
}

//>>>
// Internal API >>>
// Stubs API <<<
int Reuri_NewQueryObj(Tcl_Interp* interp, int objc, Tcl_Obj* const objv[], Tcl_Obj** res) //<<<
{
	int					code = TCL_OK;
	Tcl_ObjInternalRep	newir = {0};
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

	for (pnum=0; pnum<objc; pnum+=2)
		TEST_OK_LABEL(finally, code, query_add_index(interp, new_index, objv[pnum], pnum));

	Tcl_Obj*	irv[2] = {new_params, new_index};
	replace_tclobj(DECODED_PTR(&newir), Tcl_NewListObj(2, irv));

	replace_tclobj(&newobj, Tcl_NewObj());
	Tcl_StoreInternalRep(newobj, (Tcl_ObjType*)&query_objtype, &newir);
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
		if (interp) {
			struct interp_cx*	l = (struct interp_cx*)Tcl_GetAssocData(interp, "reuri", NULL);
			Tcl_SetErrorCode(interp, "REURI", "UNBALANCED_PARAMS", NULL);
			Tcl_SetObjResult(interp, Dedup_NewStringObj(l->dedup_pool, "Parameter list isn't even", -1));
		}
		code = TCL_ERROR;
		goto finally;
	}
	if (pc == 0) goto finally;

	const char*	str = NULL;

	percent_encode_ds(REURI_ENCODE_QUERY, ds, Tcl_GetString(pv[i++]));
	str = Tcl_GetString(pv[i++]);
	if (str[0]) {
		Tcl_DStringAppend(ds, "=", 1);
		percent_encode_ds(REURI_ENCODE_QUERYVAL, ds, str);
	}
	while (i<pc) {
		Tcl_DStringAppend(ds, "&", 1);
		percent_encode_ds(REURI_ENCODE_QUERY, ds, Tcl_GetString(pv[i++]));
		str = Tcl_GetString(pv[i++]);
		if (str[0]) {
			Tcl_DStringAppend(ds, "=", 1);
			percent_encode_ds(REURI_ENCODE_QUERYVAL, ds, str);
		}
	}

finally:
	return code;
}

//>>>
enum reuri_pathtype Reuri_PathType(Tcl_Obj* pathPtr) //<<<
{
	if (pathPtr == NULL) return REURI_PATH_EMPTY;
	const char*	str = Tcl_GetString(pathPtr);
	return (str[0] == '/') ? REURI_PATH_ABSOLUTE : REURI_PATH_ROOTLESS;
}

//>>>
// Stubs API >>>

// vim: foldmethod=marker foldmarker=<<<,>>> ts=4 shiftwidth=4
