// Based on https://re2c.org/examples/c/submatch/example_uri_rfc3986.html

#include "reuriInt.h"

/*!types:re2c*/
/*!include:re2c "parse.h"*/

#define PARSE_ERROR_LABEL(label, var, msg) \
	do { \
		parse_fail(interp, fail, base, msg); \
		var = TCL_ERROR; \
		goto label; \
	} while(0)

static void parse_fail(Tcl_Interp* interp, const unsigned char* fail, const unsigned char* base, const char* msg) //<<<
{
	if (interp) {
		int			char_ofs;
		size_t		byte_fail_ofs = fail - base;
		const char*	p = (const char*)base;
		for (
			char_ofs = 0;
			p < (const char*)(base + byte_fail_ofs);
			p = Tcl_UtfNext(p), char_ofs++
		);

		Tcl_Obj*	ofsObj = NULL;
		replace_tclobj(&ofsObj, Tcl_NewIntObj(char_ofs));
		Tcl_SetErrorCode(interp, "REURI", "PARSE", (const char*)base, Tcl_GetString(ofsObj), NULL);
		replace_tclobj(&ofsObj, NULL);
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("Failed to parse %s at ofs %d", msg, (int)char_ofs));
	}
}

//>>>
static int cesu8_to_pct_encoding(Tcl_DString* ds, const unsigned char* str, Tcl_Obj** ofs_adjustments) //<<<
{
	const unsigned char	*s = str, *tok, *YYMARKER;
	int					adj = 0;
	Tcl_Obj*			adjs = NULL;

	replace_tclobj(&adjs, Tcl_NewListObj(1, NULL));

top:
	tok = s;
	/*!local:re2c:cesu8_to_pct_encoding
	re2c:encoding:utf8         = 1;
	re2c:define:YYCTYPE        = "unsigned char";
	re2c:define:YYCURSOR       = s;
	re2c:yyfill:enable         = 0;

	!use:common;

	anychar		= [^] \ end;

	anychar+ end {
		// Don't bother with the copy to ds if we're returning false
		if (adj) Tcl_DStringAppend(ds, (const char*)tok, s-tok);
		goto finally;
	}

	anychar+ {
		Tcl_DStringAppend(ds, (const char*)tok, s-tok);
		goto top;
	}

	end {
		goto finally;
	}

	* {
		if (s-str >= 2 && s[-1] == 0xC0 && s[0] == 0x80) {	// Replace CESU-8 null with %00
			Tcl_DStringAppend(ds, "%00", 3);
			s++;
			adj += 2;
			if (TCL_OK != Tcl_ListObjAppendElement(NULL, adjs, Tcl_NewIntObj(s-str)))
				Tcl_Panic("Append to ofs_adjustments list failed");
			if (TCL_OK != Tcl_ListObjAppendElement(NULL, adjs, Tcl_NewIntObj(adj)))
				Tcl_Panic("Append to ofs_adjustments list failed");
			goto top;
		}

		goto finally;
	}
	*/
finally:
	if (adj) replace_tclobj(ofs_adjustments, adjs);
	replace_tclobj(&adjs, NULL);
	return adj;
}

//>>>
void parse_uri(struct parse_context* pc, const char* str) //<<<
{
	const unsigned char*	base = (const unsigned char*)str;
	struct interp_cx*		l = Tcl_GetAssocData(pc->interp, "reuri", NULL);
	const unsigned char
		*s1, *u1, *h1, *h3, *h5, *r1, *p1, *p3, *q1, *f1,
		*s2, *u2, *h2, *h4, *h6, *h7, *h8, *r2, *p2, *p4,
		*q2, *f2;
	const unsigned char*	s = base;
	const unsigned char*	mar = s;
	const unsigned char*	fail = s;
	Tcl_DString				val;
	Tcl_DString				reparsed;
	Tcl_Obj*				ofs_adjustments = NULL;
	int						attempted_reparse = 0;
	/*!stags:re2c:parse_uri format = "const unsigned char *@@{tag}; "; */

	Tcl_DStringInit(&val);
	Tcl_DStringInit(&reparsed);

top:
	/*!local:re2c:parse_uri
	re2c:yyfill:enable         = 0;
	re2c:flags:tags            = 1;
	re2c:api                   = custom;
	re2c:api:style             = free-form;
	re2c:encoding:utf8         = 1;
	re2c:define:YYCTYPE        = "unsigned char";
	re2c:define:YYPEEK         = "*s";
	re2c:define:YYSKIP         = "++s;";
	re2c:define:YYBACKUP       = "fail = s>fail ? s : fail; mar = s;";
	re2c:define:YYRESTORE      = "fail = s>fail ? s : fail; s = mar;";
	re2c:define:YYSTAGP        = "@@{tag} = s;";
 	re2c:define:YYSTAGN        = "@@{tag} = NULL;";

	!use:uri;

	* {
		Tcl_Obj*	ofsObj = NULL;

		if (
			!attempted_reparse &&
			fail[0] == 0xC0 && fail[1] == 0x80 &&
			cesu8_to_pct_encoding(&reparsed, base, &ofs_adjustments)
		) {
			attempted_reparse = 1;
			s = base = (const unsigned char*)Tcl_DStringValue(&reparsed);
			goto top;
		}

		size_t		byte_fail_ofs = fail - base;
		const char*	p = (const char*)base;
		for (
			pc->fail_ofs = 0;
			p < (const char*)(base + byte_fail_ofs);
			p = Tcl_UtfNext(p), pc->fail_ofs++
		);

		int			ac;
		Tcl_Obj**	av = NULL;

		if (ofs_adjustments) {
			TEST_OK_LABEL(finally, pc->rc, Tcl_ListObjGetElements(pc->interp, ofs_adjustments, &ac, &av));
			for (int i=ac-2; i>=0; i-=2) {
				int		byte, adj;
				TEST_OK_LABEL(finally, pc->rc, Tcl_GetIntFromObj(pc->interp, av[i],   &byte));
				TEST_OK_LABEL(finally, pc->rc, Tcl_GetIntFromObj(pc->interp, av[i+1], &adj));
				if (byte <= byte_fail_ofs) {
					pc->fail_ofs -= adj;
					break;
				}
			}
		}

		replace_tclobj(&ofsObj, Tcl_NewIntObj(pc->fail_ofs));
		Tcl_SetErrorCode(pc->interp, "REURI", "PARSE", str, Tcl_GetString(ofsObj), NULL);
		replace_tclobj(&ofsObj, NULL);
		Tcl_SetObjResult(pc->interp, Tcl_ObjPrintf("URI parse error at offset %d", pc->fail_ofs));
		pc->rc = TCL_ERROR;
		goto finally;
	}

	uri end {
		if (s1) replace_tclobj(&pc->uri->scheme,	Dedup_NewStringObj(l->dedup_scheme,   (const char*)s1, (int)(s2-s1)));
		if (u1) replace_tclobj(&pc->uri->userinfo,	Dedup_NewStringObj(l->dedup_userinfo, (const char*)u1, (int)(u2-u1)));

		if (h1) {
			replace_tclobj(&pc->uri->host, Dedup_NewStringObj(l->dedup_host_ipv6,     (const char*)h1, (int)(h2 - h1)));
			pc->uri->hosttype = REURI_HOST_IPV6;
		}
		if (h3) {
			replace_tclobj(&pc->uri->host, Dedup_NewStringObj(l->dedup_host_ipv4,     (const char*)h3, (int)(h4 - h3)));
			pc->uri->hosttype = REURI_HOST_IPV4;
		}
		if (h5 && h6 > h5) {
			replace_tclobj(&pc->uri->host, Dedup_NewStringObj(l->dedup_host_reg_name, (const char*)h5, (int)(h6 - h5)));
			pc->uri->hosttype = REURI_HOST_HOSTNAME;
		}
		if (h7) {
			replace_tclobj(&pc->uri->host, Dedup_NewStringObj(l->dedup_host_local,    (const char*)h7, (int)(h8 - h7)));
			pc->uri->hosttype = REURI_HOST_UNIX;
		}

		if (r1 && r2>r1) replace_tclobj(&pc->uri->port, Dedup_NewStringObj(l->dedup_port, (const char*)r1, (int)(r2 - r1)));
		if (p1 && p2>p1) replace_tclobj(&pc->uri->path, Dedup_NewStringObj(l->dedup_path, (const char*)p1, (int)(p2 - p1)));
		if (p3 && p4>p3) replace_tclobj(&pc->uri->path, Dedup_NewStringObj(l->dedup_path, (const char*)p3, (int)(p4 - p3)));
		if (q1) replace_tclobj(&pc->uri->query,         Dedup_NewStringObj(l->dedup_query, (const char*)q1, (int)(q2 - q1)));
		if (f1) replace_tclobj(&pc->uri->fragment,      Dedup_NewStringObj(l->dedup_fragment, (const char*)f1, (int)(f2 - f1)));
		goto finally;
	}
	*/

finally:
	Tcl_DStringFree(&val);
	Tcl_DStringFree(&reparsed);
	replace_tclobj(&ofs_adjustments, NULL);
}

//>>>
int uri_valid(const char* str) //<<<
{
	_Pragma("GCC diagnostic push");
	_Pragma("GCC diagnostic ignored \"-Wunused-but-set-variable\"");
	// We need these tag variables so that we can reuse the parse rules
    const char
        *s1, *u1, *h1, *h3, *h5, *r1, *p1, *p3, *q1, *f1,
        *s2, *u2, *h2, *h4, *h6, *h7, *h8, *r2, *p2, *p4,
		*q2, *f2;
	_Pragma("GCC diagnostic pop");
	const char*			s = str;
	const char*			YYMARKER;
	/*!stags:re2c:uri_valid format = "const char *@@{tag}; "; */

	/*!local:re2c:uri_valid
    re2c:api:style             = free-form;
    re2c:define:YYCTYPE        = "char";
    re2c:define:YYCURSOR       = s;
	re2c:yyfill:enable         = 0;
	re2c:flags:tags            = 1;

	!use:uri_strict;

    uri end	{return 1;}
    *		{return 0;}
	*/
}

//>>>
static inline int conditionally_allowed(enum reuri_encode_mode mode, const char yych) //<<<
{
	switch (mode) {
		case REURI_ENCODE_QUERY:	return (yych=='/' || yych==':' || yych=='@' || yych=='?');
		case REURI_ENCODE_QUERYVAL:	return (yych=='/' || yych==':' || yych=='@' || yych=='?' || yych=='=');
		case REURI_ENCODE_PATH:		return (yych=='@' || yych=='=' || yych=='&' || yych=='+');
		case REURI_ENCODE_PATH2:	return (yych=='@' || yych=='=' || yych=='&' || yych==':' || yych=='+');
		case REURI_ENCODE_HOST:		return (yych=='=' || yych=='&' || yych=='+');
		case REURI_ENCODE_USERINFO:	return (yych=='=' || yych=='&' || yych==':' || yych=='+');
		case REURI_ENCODE_FRAGMENT:	return (yych=='@' || yych=='/' || yych=='?' || yych=='=' || yych=='&' || yych==':' || yych=='+');
		case REURI_ENCODE_AWSSIG:	return 0;		// Should not get here - AWSSIG rules exclude more than this function can
	}
	return 0;
}

//>>>
Tcl_Obj* percent_encode(Tcl_Interp* interp, Tcl_Obj* objPtr, enum reuri_encode_mode mode) //<<<
{
	struct interp_cx*		l = interp ? Tcl_GetAssocData(interp, "reuri", NULL) : NULL;
	Tcl_Obj*				res = NULL;
	const unsigned char*	u;
	const unsigned char*	str = NULL;	// CESU-8
	const unsigned char*	s;
	int						c = yycstart;
	Tcl_DString				val;
	/*!stags:re2c:percent_encode format = "const unsigned char *@@{tag}; "; */

	u = s = str = (const unsigned char*)Tcl_GetString(objPtr);

	Tcl_DStringInit(&val);

	/*!local:re2c:percent_encode
	re2c:api:style             = free-form;
	re2c:define:YYCTYPE        = "unsigned char";
	re2c:define:YYCURSOR       = s;
	re2c:yyfill:enable         = 0;
	re2c:flags:tags            = 1;
	re2c:define:YYGETCONDITION = "c";
	re2c:define:YYSETCONDITION = "c = @@;";

	!use:uri_strict;

	<start> allowed* / end {
		res = objPtr;
		goto finally;
	}
	<start> allowed* / reserved {
		if (conditionally_allowed(mode, yych)) {
			s++;
			goto yyc_start;
		}

		if (s>str)
			Tcl_DStringAppend(&val, (const char*)u, (int)(s-u));

		u = s;
		c = yycmixed;
		goto yyc_mixed;
	}
	<start> allowed* / cesu8null {
		if (s>str)
			Tcl_DStringAppend(&val, (const char*)u, (int)(s-u));

		u = s;
		c = yycmixed;
		goto yyc_mixed;
	}

	<mixed> cesu8null {
		Tcl_DStringAppend(&val, "%00", 3);
		u = s;
		goto yyc_mixed;
	}
	<mixed> reserved {
		if (conditionally_allowed(mode, yych)) {
			Tcl_DStringAppend(&val, (const char*)s-1, 1);
		} else {
			char buf[4];

			sprintf(buf, "%%%02X", yych);
			Tcl_DStringAppend(&val, buf, 3);
		}
		u = s;
		goto yyc_mixed;
	}
	<mixed> allowed+ {
		Tcl_DStringAppend(&val, (const char*)u, (int)(s-u));
		u = s;
		goto yyc_mixed;
	}
	<mixed> end {
		res = l ?
				Dedup_NewStringObj(l->dedup_pool, Tcl_DStringValue(&val), Tcl_DStringLength(&val)) :
				Tcl_DStringToObj(&val);
		goto finally;
	}

	<*> *	{
		Tcl_Panic("Unable to percent_encode byte at ofs %d: 0x%02x", (int)(s-str), *s);
	}
	*/

finally:
	Tcl_DStringFree(&val);
	return res;
}

//>>>
Tcl_Obj* percent_encode_awssig(Tcl_Interp* interp, Tcl_Obj* objPtr) //<<<
{
	struct interp_cx*		l = Tcl_GetAssocData(interp, "reuri", NULL);
	Tcl_Obj*				res = NULL;
	const unsigned char*	u;
	const unsigned char*	str = NULL;	// CESU-8
	const unsigned char*	s;
	int						c = yycstart;
	Tcl_DString				val;
	/*!stags:re2c:percent_encode_awssig format = "const unsigned char *@@{tag}; "; */

	u = s = str = (const unsigned char*)Tcl_GetString(objPtr);

	Tcl_DStringInit(&val);

	/*!local:re2c:percent_encode_awssig
    re2c:api:style             = free-form;
    re2c:define:YYCTYPE        = "unsigned char";
    re2c:define:YYCURSOR       = s;
	re2c:yyfill:enable         = 0;
	re2c:flags:tags            = 1;
	re2c:define:YYGETCONDITION = "c";
	re2c:define:YYSETCONDITION = "c = @@;";

	!use:common;

	// URI encode every byte except the unreserved characters: 'A'-'Z', 'a'-'z', '0'-'9', '-', '.', '_', and '~'.
	allowed		= [A-Za-z0-9._~-];
	reserved	= [^] \ end \ allowed;

	<start> allowed* / end {
		res = objPtr;
		goto finally;
	}
	<start> allowed* / reserved {
		if (s>str)
			Tcl_DStringAppend(&val, (const char*)u, (int)(s-u));

		u = s;
		c = yycmixed;
		goto yyc_mixed;
	}
	<start> allowed* / cesu8null {
		if (s>str)
			Tcl_DStringAppend(&val, (const char*)u, (int)(s-u));

		u = s;
		c = yycmixed;
		goto yyc_mixed;
	}

	<mixed> cesu8null {
		Tcl_DStringAppend(&val, "%00", 3);
		u = s;
		goto yyc_mixed;
	}
	<mixed> reserved {
		char buf[4];

		sprintf(buf, "%%%02X", yych);
		Tcl_DStringAppend(&val, buf, 3);
		u = s;
		goto yyc_mixed;
	}
	<mixed> allowed+ {
		Tcl_DStringAppend(&val, (const char*)u, (int)(s-u));
		u = s;
		goto yyc_mixed;
	}
	<mixed> end {
		res = Dedup_NewStringObj(l->dedup_pool, \
				Tcl_DStringValue(&val), Tcl_DStringLength(&val));
		goto finally;
	}

	<*> *	{
		Tcl_Panic("Unable to percent_encode byte at ofs %d: 0x%02x", (int)(s-str), *s);
	}
	*/

finally:
	Tcl_DStringFree(&val);
	return res;
}

//>>>
void percent_encode_ds(enum reuri_encode_mode mode, Tcl_DString* ds, const char* str) //<<<
{
	const unsigned char*	base = (const unsigned char*)str;
	const unsigned char*	s = base;
	const unsigned char*	u = NULL;
	char					buf[4];
	/*!stags:re2c:percent_encode_ds format = "const unsigned char *@@{tag}; "; */

top:
	/*!local:re2c:percent_encode_ds
	re2c:api:style             = free-form;
	re2c:define:YYCTYPE        = "unsigned char";
	re2c:define:YYCURSOR       = s;
	re2c:yyfill:enable         = 0;
	re2c:flags:tags            = 1;

	!use:uri_strict;

	end {
		return;
	}
	@u allowed+ {
		Tcl_DStringAppend(ds, (const char*)u, (int)(s-u));
		goto top;
	}
	cesu8null {
		Tcl_DStringAppend(ds, "%00", 3);
		goto top;
	}
	reserved {
		if (conditionally_allowed(mode, yych)) {
			Tcl_DStringAppend(ds, (const char*)s-1, 1);
		} else {
			sprintf(buf, "%%%02X", yych);
			Tcl_DStringAppend(ds, buf, 3);
		}
		goto top;
	}
	* {
		Tcl_Panic("Unable to percent_encode byte at ofs %d: 0x%02x", (int)(s-base), *s);
	}
	*/
}

//>>>
static int _add_name(Tcl_Interp* interp, struct interp_cx* l, Tcl_DString* ds, Tcl_Obj* res_params, Tcl_Obj* res_index, const int pnum) //<<<
{
	int				code = TCL_OK;
	Tcl_Obj*		nameObj = NULL;

	replace_tclobj(&nameObj, l ? Dedup_NewStringObj(l->dedup_pool, Tcl_DStringValue(ds), Tcl_DStringLength(ds)) : Tcl_DStringToObj(ds));

	// Append the name to params
	TEST_OK_LABEL(finally, code, Tcl_ListObjAppendElement(interp, res_params, nameObj));

	// Add this instance of name at pnum to the index
	TEST_OK_LABEL(finally, code, query_add_index(interp, res_index, nameObj, pnum));

finally:
	Tcl_DStringSetLength(ds, 0);
	replace_tclobj(&nameObj, NULL);

	return code;
}

//>>>
static int _add_value(Tcl_Interp* interp, struct interp_cx* l, Tcl_DString* ds, Tcl_Obj* res_params) //<<<
{
	int				code = TCL_OK;
	Tcl_Obj*		valueObj = NULL;

	replace_tclobj(&valueObj, l ? Dedup_NewStringObj(l->dedup_pool, Tcl_DStringValue(ds), Tcl_DStringLength(ds)) : Tcl_DStringToObj(ds));

	TEST_OK_LABEL(finally, code, Tcl_ListObjAppendElement(interp, res_params, valueObj));

finally:
	Tcl_DStringSetLength(ds, 0);
	replace_tclobj(&valueObj, NULL);

	return code;
}

//>>>
int percent_decode(Tcl_Obj* str, Tcl_Obj** res) //<<<
{
	const unsigned char*	base = (const unsigned char*)Tcl_GetString(str);
	const unsigned char*	s = base;
	int						c = yycstart;
	const unsigned char*	YYMARKER;
	const unsigned char*	start = NULL;
	Tcl_DString				acc;
	/*!stags:re2c:percent_decode format = "const unsigned char *@@{tag}; "; */

	if (*s == 0) {
		replace_tclobj(res, str);
		return TCL_OK;
	}

	/*!local:re2c:percent_decode
    re2c:api:style             = free-form;
    re2c:define:YYCTYPE        = "unsigned char";
    re2c:define:YYCURSOR       = s;
	re2c:yyfill:enable         = 0;
	re2c:flags:tags            = 1;
	re2c:define:YYGETCONDITION = "c";
	re2c:define:YYSETCONDITION = "c = @@;";

	!use:common;

	special    = [%+];
	unencoded  = ([^] \ special) \ end;

	<start> unencoded* {
		// No change to input
		replace_tclobj(res, str);
		return TCL_OK;
	}

	<start> @start unencoded* / special {
		Tcl_DStringInit(&acc);
		if (s>start) Tcl_DStringAppend(&acc, (const char*)start, (int)(s-start));
		c = yycdecoded;
		goto yyc_decoded;
	}

	<decoded> @start unencoded+ {
		Tcl_DStringAppend(&acc, (const char*)start, (int)(s-start));
		c = yycdecoded;
		goto yyc_decoded;
	}

	<decoded> pct_encoded {
		const unsigned char	buf[3] = {s[-2], s[-1], 0};
		unsigned char		byte = strtol((const char*)buf, NULL, 16);

		if (byte) {
			Tcl_DStringAppend(&acc, (const char*)&byte, 1);
		} else {
			Tcl_DStringAppend(&acc, "\xc0\x80", 2);
		}
		goto yyc_decoded;
	}

	<decoded> "+" {
		Tcl_DStringAppend(&acc, " ", 1);
		goto yyc_decoded;
	}

	<decoded> "%" {
		// Compat: just leave percentages that don't form valid pct_encoded sequences
		Tcl_DStringAppend(&acc, "%", 1);
		goto yyc_decoded;
	}
	
	<decoded> end {
		replace_tclobj(res, Tcl_NewStringObj(Tcl_DStringValue(&acc), Tcl_DStringLength(&acc)));
		Tcl_DStringFree(&acc);
		return TCL_OK;
	}

	<*> * {
		fprintf(stderr, "Unable to percent decode string at ofs %d: 0x%02x", (int)(s-base), *s);
		return TCL_ERROR;
	}
	*/
}

//>>>
void percent_decode_ds(const char* str, Tcl_DString* ds) //<<<
{
	const unsigned char*	s = (const unsigned char*)str;
	const unsigned char*	tok;
	const unsigned char*	YYMARKER;

top:
	tok = s;

	/*!local:re2c:percent_decode_ds
    re2c:define:YYCTYPE        = "unsigned char";
    re2c:define:YYCURSOR       = s;
	re2c:yyfill:enable         = 0;

	!use:common;

	special    = [%];
	unencoded  = [^] \ special \ end;

	unencoded+ {
		Tcl_DStringAppend(ds, (const char*)tok, s-tok);
		goto top;
	}

	pct_encoded {
		const unsigned char	buf[3] = {tok[1], tok[2], 0};
		unsigned char		byte = strtol((const char*)buf, NULL, 16);

		if (byte) {
			Tcl_DStringAppend(ds, (const char*)&byte, 1);
		} else {
			Tcl_DStringAppend(ds, "\xc0\x80", 2);
		}
		goto top;
	}

	"%" {
		Tcl_DStringAppend(ds, (const char*)tok, 1);
		goto top;
	}

	end {
		return;
	}
	*/
}

//>>>
int decode_query(Tcl_Interp* interp, const char* str, Tcl_Obj** params, Tcl_Obj** index) //<<<
{
	int						code = TCL_OK;
	struct interp_cx*		l = interp ? Tcl_GetAssocData(interp, "reuri", NULL) : NULL;
	const unsigned char*	base = (const unsigned char*)str;
	int						pnum = 0;
	int						c = yycname;
	const unsigned char*	start = NULL;
	Tcl_Obj*				res_params = NULL;
	Tcl_Obj*				res_index = NULL;
	Tcl_Obj*				empty = NULL;
	Tcl_DString				acc;
	/*!stags:re2c:decode_query format = "const unsigned char *@@{tag}; "; */

	replace_tclobj(&empty, l ? l->empty : Tcl_NewObj());

	Tcl_DStringInit(&acc);

	replace_tclobj(&res_params, Tcl_NewListObj(0, NULL));
	replace_tclobj(&res_index,  Tcl_NewDictObj());

	if (base[0] == 0) goto finally;	/* Do nothing gracefully */

	const unsigned char*	s = base;
	const unsigned char*	mar = s;
	const unsigned char*	fail = s;

top:
	/*!local:re2c:decode_query
	re2c:api                   = custom;
	re2c:api:style             = free-form;
	re2c:encoding:utf8         = 1;
	re2c:yyfill:enable         = 0;
	re2c:flags:tags            = 1;
	re2c:define:YYCTYPE        = "unsigned char";
	re2c:define:YYGETCONDITION = "c";
	re2c:define:YYSETCONDITION = "c = @@;";
	re2c:define:YYPEEK         = "*s";
	re2c:define:YYSKIP         = "++s;";
	re2c:define:YYBACKUP       = "fail = s>fail ? s : fail; mar = s;";
	re2c:define:YYRESTORE      = "fail = s>fail ? s : fail; s = mar;";
	re2c:define:YYSTAGP        = "@@{tag} = s;";
 	re2c:define:YYSTAGN        = "@@{tag} = NULL;";

	!use:common;

	special    = [%+&=];
	unencoded  = ([^] \ special) \ end;

	<*> @start unencoded+ {
		Tcl_DStringAppend(&acc, (const char*)start, (int)(s-start));
		goto top;
	}
	<*> "+" {
		Tcl_DStringAppend(&acc, " ", 1);
		goto top;
	}
	<*> pct_encoded {
		const unsigned char	buf[3] = {s[-2], s[-1], 0};
		unsigned char		byte = strtol((const char*)buf, NULL, 16);

		if (byte) {
			Tcl_DStringAppend(&acc, (const char*)&byte, 1);
		} else {
			Tcl_DStringAppend(&acc, "\xc0\x80", 2);
		}
		goto top;
	}
	<*> "%" {
		// Compat: just leave percentages that don't form valid pct_encoded sequences
		Tcl_DStringAppend(&acc, "%", 1);
		goto top;
	}
	
	<name> "=" {
		TEST_OK_LABEL(finally, code, _add_name(interp, l, &acc, res_params, res_index, pnum++));
		c = yycvalue;
		goto yyc_value;
	}
	<name> "&" {
        TEST_OK_LABEL(finally, code, _add_name(interp, l, &acc, res_params, res_index, pnum++));
        TEST_OK_LABEL(finally, code, Tcl_ListObjAppendElement(interp, res_params, empty));
		goto yyc_name;
	}
	<name> end {
        TEST_OK_LABEL(finally, code, _add_name(interp, l, &acc, res_params, res_index, pnum++));
        TEST_OK_LABEL(finally, code, Tcl_ListObjAppendElement(interp, res_params, empty));
		goto finally;
	}

	<value> "&" {
		TEST_OK_LABEL(finally, code, _add_value(interp, l, &acc, res_params));
		c = yycname;
		goto yyc_name;
	}
	<value> end {
		TEST_OK_LABEL(finally, code, _add_value(interp, l, &acc, res_params));
		goto finally;
	}
	<value> "=" {
		Tcl_DStringAppend(&acc, "=", 1);
		goto yyc_value;
	}

	<*> * {
		if (s>base && s[-1] == 0xc0 && s[0] == 0x80) { // CESU-8 encoding of codepoint 0
			Tcl_DStringAppend(&acc, "\xc0\x80", 2);
			s++;
			goto top;
		}

        PARSE_ERROR_LABEL(finally, code, "query");
	}
	*/

finally:
	if (code == TCL_OK) {
		replace_tclobj(params, res_params);
		replace_tclobj(index,  res_index);
	}

	replace_tclobj(&res_params,	NULL);
	replace_tclobj(&res_index,	NULL);
	replace_tclobj(&empty, NULL);
	Tcl_DStringFree(&acc);
	return code;
}

//>>>
#define _add_path _add_value
int decode_path(Tcl_Interp* interp, const char* str, Tcl_Obj** pathlist) //<<<
{
	int						code = TCL_OK;
	struct interp_cx*		l = interp ? Tcl_GetAssocData(interp, "reuri", NULL) : NULL;
	const unsigned char*	base = (const unsigned char*)str;
	const unsigned char*	s = base;
	const unsigned char*	mar = s;
	const unsigned char*	fail = s;
	const unsigned char*	start = NULL;
	Tcl_Obj*				res_pathlist = NULL;
	Tcl_DString				acc;
	/*!stags:re2c:decode_path format = "const unsigned char *@@{tag}; "; */

	Tcl_DStringInit(&acc);

	replace_tclobj(&res_pathlist, Tcl_NewListObj(0, NULL));

	if (s[0] == 0) goto finally;	/* Do nothing gracefully */

top:
	/*!local:re2c:decode_path
	re2c:api                   = custom;
	re2c:api:style             = free-form;
	re2c:encoding:utf8         = 1;
	re2c:yyfill:enable         = 0;
	re2c:flags:tags            = 1;
	re2c:define:YYCTYPE        = "unsigned char";
	re2c:define:YYPEEK         = "*s";
	re2c:define:YYSKIP         = "++s;";
	re2c:define:YYBACKUP       = "fail = s>fail ? s : fail; mar = s;";
	re2c:define:YYRESTORE      = "fail = s>fail ? s : fail; s = mar;";
	re2c:define:YYSTAGP        = "@@{tag} = s;";
 	re2c:define:YYSTAGN        = "@@{tag} = NULL;";

	!use:common;

	lenient		= [^] \ end \ [/%];

	@start lenient+ {
		Tcl_DStringAppend(&acc, (const char*)start, (int)(s-start));
		goto top;
	}
	pct_encoded {
		const unsigned char	buf[3] = {s[-2], s[-1], 0};
		unsigned char		byte = strtol((const char*)buf, NULL, 16);

		if (byte) {
			Tcl_DStringAppend(&acc, (const char*)&byte, 1);
		} else {
			Tcl_DStringAppend(&acc, "\xc0\x80", 2);
		}
		goto top;
	}
	"%" {
		// Invalid percent that isn't a valid percent encoded sequence, add it as a literal
		Tcl_DStringAppend(&acc, "%", 1);
		goto top;
	}
	"/" {
		if (s == base+1) Tcl_DStringAppend(&acc, "/", 1);
		TEST_OK_LABEL(finally, code, _add_path(interp, l, &acc, res_pathlist));
		goto top;
	}
	end {
		if (Tcl_DStringLength(&acc))
			TEST_OK_LABEL(finally, code, _add_path(interp, l, &acc, res_pathlist));
		goto finally;
	}

	* {
		if (s>base && s[-1] == 0xc0 && s[0] == 0x80) { // CESU-8 encoding of codepoint 0
			Tcl_DStringAppend(&acc, "\xc0\x80", 2);
			s++;
			goto top;
		}

		if (interp) {
			int			char_ofs;
			size_t		byte_fail_ofs = fail - base;
			const char*	p = (const char*)base;
			for (
				char_ofs = 0;
				p < (const char*)(base + byte_fail_ofs);
				p = Tcl_UtfNext(p), char_ofs++
			);

			Tcl_Obj*	ofsObj = NULL;
			replace_tclobj(&ofsObj, Tcl_NewIntObj(char_ofs));
			Tcl_SetErrorCode(interp, "REURI", "PARSE", str, Tcl_GetString(ofsObj), NULL);
			replace_tclobj(&ofsObj, NULL);
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("Failed to parse path at ofs %d", (int)char_ofs));
		}
		code = TCL_ERROR;
		goto finally;
	}
	*/

finally:
	if (code == TCL_OK)
		replace_tclobj(pathlist, res_pathlist);

	replace_tclobj(&res_pathlist, NULL);
	Tcl_DStringFree(&acc);
	return code;
}

//>>>
void ascii_lowercase_ds(Tcl_DString* ds, const char* str) //<<<
{
	const unsigned char*	s = (const unsigned char*)str;
	const unsigned char*	tok;

top:
	tok = s;

	/*!local:re2c:ascii_lowercase_ds
	re2c:define:YYCTYPE        = "unsigned char";
	re2c:define:YYCURSOR       = s;
	re2c:yyfill:enable         = 0;

	!use:common;

	upper		= [A-Z];
	pass		= [^] \ end \ upper;

	upper	{ unsigned char l=tok[0]|(1<<5); Tcl_DStringAppend(ds, (const char*)&l, 1);      goto top; }
	pass+	{                                Tcl_DStringAppend(ds, (const char*)tok, s-tok); goto top; }
	end		{ return; }
	*/
}

//>>>
int parse_host_local(Tcl_Interp* interp, const char* str, Tcl_Obj** pathlist) //<<<
{
	int						code = TCL_OK;
	struct interp_cx*		l = interp ? Tcl_GetAssocData(interp, "reuri", NULL) : NULL;
	const unsigned char*	base = (const unsigned char*)str;
	const unsigned char*	s = base;
	const unsigned char*	mar = s;
	const unsigned char*	fail = s;
	const unsigned char*	tok;
	Tcl_Obj*				res_pathlist = NULL;
	Tcl_DString				acc;

	Tcl_DStringInit(&acc);

	replace_tclobj(&res_pathlist, Tcl_NewListObj(0, NULL));

	/*!local:re2c:parse_host_local_prefix
	re2c:api                   = custom;
	re2c:api:style             = free-form;
	re2c:encoding:utf8         = 1;
	re2c:yyfill:enable         = 0;
	re2c:define:YYCTYPE        = "unsigned char";
	re2c:define:YYPEEK         = "*s";
	re2c:define:YYSKIP         = "++s;";
	re2c:define:YYBACKUP       = "fail = s>fail ? s : fail; mar = s;";
	re2c:define:YYRESTORE      = "fail = s>fail ? s : fail; s = mar;";

	"[v0.local:" | "["	{goto start;}
	*					{goto fail;}

	*/

start:
	if (s[0] == '/') {
		Tcl_DStringAppend(&acc, "/", 1);
		TEST_OK_LABEL(finally, code, _add_path(interp, l, &acc, res_pathlist));
		s++;
	}

top:
	tok = s;

	/*!local:re2c:parse_host_local
	re2c:api                   = custom;
	re2c:api:style             = free-form;
	re2c:encoding:utf8         = 1;
	re2c:yyfill:enable         = 0;
	re2c:define:YYCTYPE        = "unsigned char";
	re2c:define:YYPEEK         = "*s";
	re2c:define:YYSKIP         = "++s;";
	re2c:define:YYBACKUP       = "fail = s>fail ? s : fail; mar = s;";
	re2c:define:YYRESTORE      = "fail = s>fail ? s : fail; s = mar;";

	!use:common;

	lenient		= [^] \ end \ [/%] \ "]";

	lenient+ {
		Tcl_DStringAppend(&acc, (const char*)tok, (int)(s-tok));
		goto top;
	}
	pct_encoded {
		const unsigned char	buf[3] = {tok[1], tok[2], 0};
		unsigned char		byte = strtol((const char*)buf, NULL, 16);

		if (byte) {
			Tcl_DStringAppend(&acc, (const char*)&byte, 1);
		} else {
			Tcl_DStringAppend(&acc, "\xc0\x80", 2);
		}
		goto top;
	}
	"%" {
		// Invalid percent that isn't a valid percent encoded sequence, add it as a literal
		Tcl_DStringAppend(&acc, "%", 1);
		goto top;
	}
	"/" {
		TEST_OK_LABEL(finally, code, _add_path(interp, l, &acc, res_pathlist));
		goto top;
	}
	"]" end {
		if (Tcl_DStringLength(&acc))
			TEST_OK_LABEL(finally, code, _add_path(interp, l, &acc, res_pathlist));
		goto finally;
	}
	* {
		if (tok[0] == 0xc0 && tok[1] == 0x80) { // CESU-8 encoding of codepoint 0
			Tcl_DStringAppend(&acc, "\xc0\x80", 2);
			s++;
			goto top;
		}

		goto fail;
	}
	*/

fail:
	code = TCL_ERROR;
	if (interp) {
		int			char_ofs;
		size_t		byte_fail_ofs = fail - base;
		const char*	p = (const char*)base;
		for (
			char_ofs = 0;
			p < (const char*)(base + byte_fail_ofs);
			p = Tcl_UtfNext(p), char_ofs++
		);

		Tcl_Obj*	ofsObj = NULL;
		replace_tclobj(&ofsObj, Tcl_NewIntObj(char_ofs));
		Tcl_SetErrorCode(interp, "REURI", "PARSE", str, Tcl_GetString(ofsObj), NULL);
		replace_tclobj(&ofsObj, NULL);
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("Failed to parse local socket at ofs %d", (int)char_ofs));
	}

finally:
	if (code == TCL_OK)
		replace_tclobj(pathlist, res_pathlist);

	replace_tclobj(&res_pathlist, NULL);
	Tcl_DStringFree(&acc);
	return code;
}

//>>>
int parse_host_ipv6(Tcl_Interp* interp, const char* str, Tcl_Obj** addr) //<<<
{
	int						code = TCL_OK;
	struct interp_cx*		l = interp ? Tcl_GetAssocData(interp, "reuri", NULL) : NULL;
	const unsigned char*	base = (const unsigned char*)str;
	const unsigned char*	s = base;
	const unsigned char*	mar = s;
	const unsigned char*	fail = s;

	/*!local:re2c:parse_host_ipv6
	re2c:api                   = custom;
	re2c:api:style             = free-form;
	re2c:yyfill:enable         = 0;
	re2c:define:YYCTYPE        = "unsigned char";
	re2c:define:YYPEEK         = "*s";
	re2c:define:YYSKIP         = "++s;";
	re2c:define:YYBACKUP       = "fail = s>fail ? s : fail; mar = s;";
	re2c:define:YYRESTORE      = "fail = s>fail ? s : fail; s = mar;";

	!use:uri;

	* {PARSE_ERROR_LABEL(finally, code, "IPv6 host part");}
	ip_literal {
		replace_tclobj(addr, l ? Dedup_NewStringObj(l->dedup_pool, str+1, s-base-2) : Tcl_NewStringObj(str+1, s-base-2));
		goto finally;
	}
	*/

finally:
	return code;
}

//>>>
int decode_port(Tcl_Interp* interp, const char* str, int* portnum) //<<<
{
	const unsigned char*	s = (const unsigned char*)str;
	int						acc = 0;

top:
	/*!local:re2c:decode_port
	re2c:define:YYCTYPE        = "unsigned char";
	re2c:define:YYCURSOR       = s;
	re2c:yyfill:enable         = 0;

	!use:common;

	digit	{ acc = acc*10 + s[-1]-'0'; goto top; }
	end		{ *portnum = acc; return TCL_OK; }
	*		{ if (interp) Tcl_SetObjResult(interp, Tcl_ObjPrintf("%s", "Could not parse port")); return TCL_ERROR; }
	*/
}

//>>>
int parse_scheme(Tcl_Interp* interp, Tcl_Obj* in, Tcl_Obj** out) //<<<
{
	int						code = TCL_OK;
	const char*				str = Tcl_GetString(in);
	const unsigned char*	base = (const unsigned char*)str;
	const unsigned char*	s = base;
	const unsigned char*	mar = s;
	const unsigned char*	fail = s;
	_Pragma("GCC diagnostic push");
	_Pragma("GCC diagnostic ignored \"-Wunused-but-set-variable\"");
	const unsigned char		*s1, *s2;
	_Pragma("GCC diagnostic pop");
	/*!stags:re2c:parse_scheme format = "const unsigned char *@@{tag};"; */

	/*!local:re2c:parse_scheme
	!use:tags_fail;
	!use:uri;

	*			{PARSE_ERROR_LABEL(finally, code, "scheme");}
	end			{replace_tclobj(out, NULL); goto finally;}
	scheme end	{replace_tclobj(out, in);   goto finally;}
	*/

finally:
	return code;
}

//>>>
int parse_userinfo(Tcl_Interp* interp, Tcl_Obj* in, Tcl_Obj** out) //<<<
{
	int						code = TCL_OK;
	const char*				str = Tcl_GetString(in);
	const unsigned char*	base = (const unsigned char*)str;
	const unsigned char*	s = base;
	const unsigned char*	mar = s;
	const unsigned char*	fail = s;
	_Pragma("GCC diagnostic push");
	_Pragma("GCC diagnostic ignored \"-Wunused-but-set-variable\"");
	const unsigned char		*u1, *u2;
	_Pragma("GCC diagnostic pop");
	/*!stags:re2c:parse_userinfo format = "const unsigned char *@@{tag};"; */

	/*!local:re2c:parse_userinfo
	!use:tags_fail;
	!use:uri;

	*				{PARSE_ERROR_LABEL(finally, code, "userinfo");}
	end				{replace_tclobj(out, NULL); goto finally;}
	userinfo end	{replace_tclobj(out, in);   goto finally;}
	*/

finally:
	return code;
}

//>>>
int parse_host(Tcl_Interp* interp, Tcl_Obj* in, Tcl_Obj** out, enum reuri_hosttype* hosttype) //<<<
{
	int						code = TCL_OK;
	const char*				str = Tcl_GetString(in);
	const unsigned char*	base = (const unsigned char*)str;
	const unsigned char*	s = base;
	const unsigned char*	mar = s;
	const unsigned char*	fail = s;
	_Pragma("GCC diagnostic push");
	_Pragma("GCC diagnostic ignored \"-Wunused-but-set-variable\"");
	const unsigned char		*h1, *h2, *h3, *h4, *h5, *h6, *h7, *h8;
	_Pragma("GCC diagnostic pop");
	/*!stags:re2c:parse_host format = "const unsigned char *@@{tag};"; */

	/*!local:re2c:parse_host
	!use:tags_fail;
	!use:uri;

	*	{PARSE_ERROR_LABEL(finally, code, "host");}

	end {
		replace_tclobj(out, NULL);
		*hosttype = REURI_HOST_NONE;
		goto finally;
	}

	host end {
		replace_tclobj(out, in);
		if (h1) {
			*hosttype = REURI_HOST_IPV6;
		} else if (h3) {
			*hosttype = REURI_HOST_IPV4;
		} else if (h5 && h6 > h5) {
			*hosttype = REURI_HOST_HOSTNAME;
		} else if (h7) {
			*hosttype = REURI_HOST_UNIX;
		}
		goto finally;
	}
	*/

finally:
	return code;
}

//>>>
int parse_port(Tcl_Interp* interp, Tcl_Obj* in, Tcl_Obj** out) //<<<
{
	int						code = TCL_OK;
	const char*				str = Tcl_GetString(in);
	const unsigned char*	base = (const unsigned char*)str;
	const unsigned char*	s = base;
	const unsigned char*	mar = s;
	const unsigned char*	fail = s;
	_Pragma("GCC diagnostic push");
	_Pragma("GCC diagnostic ignored \"-Wunused-but-set-variable\"");
	const unsigned char		*r1, *r2;
	_Pragma("GCC diagnostic pop");
	/*!stags:re2c:parse_port format = "const unsigned char *@@{tag};"; */

	/*!local:re2c:parse_port
	!use:tags_fail;
	!use:uri;

	*			{PARSE_ERROR_LABEL(finally, code, "port");}
	end			{replace_tclobj(out, NULL); goto finally;}
	port end	{replace_tclobj(out, in);   goto finally;}
	*/

finally:
	return code;
}

//>>>
int parse_path(Tcl_Interp* interp, Tcl_Obj* in, Tcl_Obj** out) //<<<
{
	int						code = TCL_OK;
	const char*				str = Tcl_GetString(in);
	const unsigned char*	base = (const unsigned char*)str;
	const unsigned char*	s = base;
	const unsigned char*	mar = s;
	const unsigned char*	fail = s;

	/*!local:re2c:parse_path
	!use:notags_fail;
	!use:uri;

	path
		= path_absolute
		| path_rootless
		| path_empty;

	*			{PARSE_ERROR_LABEL(finally, code, "path");}
	end			{replace_tclobj(out, NULL); goto finally;}
	path end	{replace_tclobj(out, in);   goto finally;}
	*/

finally:
	return code;
}

//>>>
int parse_query(Tcl_Interp* interp, Tcl_Obj* in, Tcl_Obj** out) //<<<
{
	int						code = TCL_OK;
	const char*				str = Tcl_GetString(in);
	const unsigned char*	base = (const unsigned char*)str;
	const unsigned char*	s = base;
	const unsigned char*	mar = s;
	const unsigned char*	fail = s;
	_Pragma("GCC diagnostic push");
	_Pragma("GCC diagnostic ignored \"-Wunused-but-set-variable\"");
	const unsigned char		*q1, *q2;
	_Pragma("GCC diagnostic pop");
	/*!stags:re2c:parse_query format = "const unsigned char *@@{tag};"; */

	/*!local:re2c:parse_query
	!use:tags_fail;
	!use:uri;

	*			{PARSE_ERROR_LABEL(finally, code, "query");}
	end			{replace_tclobj(out, NULL); goto finally;}
	query end	{replace_tclobj(out, in);   goto finally;}
	*/

finally:
	return code;
}

//>>>
int parse_fragment(Tcl_Interp* interp, Tcl_Obj* in, Tcl_Obj** out) //<<<
{
	int						code = TCL_OK;
	const char*				str = Tcl_GetString(in);
	const unsigned char*	base = (const unsigned char*)str;
	const unsigned char*	s = base;
	_Pragma("GCC diagnostic push");
	_Pragma("GCC diagnostic ignored \"-Wunused-but-set-variable\"");
	const unsigned char		*f1, *f2;
	_Pragma("GCC diagnostic pop");
	/*!stags:re2c:parse_fragment format = "const unsigned char *@@{tag};"; */

	/*!local:re2c:parse_fragment
	!use:tags_fail;
	!use:uri;

	*				{PARSE_ERROR_LABEL(finally, code, "fragment");}
	end				{replace_tclobj(out, NULL); goto finally;}
	fragment end	{replace_tclobj(out, in);   goto finally;}
	*/

finally:
	return code;
}

//>>>

// vim: ft=c foldmethod=marker foldmarker=<<<,>>> ts=4 shiftwidth=4
