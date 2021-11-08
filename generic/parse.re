// Based on https://re2c.org/examples/c/submatch/example_uri_rfc3986.html

#include "reuriInt.h"

/*!include:re2c "parse.h"*/

void parse_uri(struct parse_context* pc, const char* str, int len) //<<<
{
	struct interp_cx*	l = Tcl_GetAssocData(pc->interp, "reuri", NULL);
    const char
        *s1, *u1, *h1, *h3, *h5, *r1, *p1, *p3, *q1, *f1,
        *s2, *u2, *h2, *h4, *h6, *h7, *h8, *r2, *p2, *p4,
		*q2, *f2;
	const char*			s = str;
	const char*			YYMARKER;
	Tcl_DString			val;
	/*!stags:re2c:parse_uri format = "const char *@@{tag}; "; */

	Tcl_DStringInit(&val);

	/*!local:re2c:parse_uri
    re2c:api:style             = free-form;
    re2c:define:YYCTYPE        = "char";
    re2c:define:YYCURSOR       = s;
	re2c:yyfill:enable         = 0;
	re2c:flags:tags            = 1;

	!use:common;

    unreserved  = alpha | digit | [-._~];
    sub_delims  = [!$&'()*+,;=];
    pchar       = unreserved | pct_encoded | sub_delims | [:@];

    scheme = @s1 alpha (alpha | digit | [-+.])* @s2;
    userinfo = @u1 (unreserved | pct_encoded | sub_delims | ":")* @u2;
    dec_octet
        = digit
        | [\x31-\x39] digit
        | "1" digit{2}
        | "2" [\x30-\x34] digit
        | "25" [\x30-\x35];
    ipv4address = dec_octet "." dec_octet "." dec_octet "." dec_octet;
    h16         = hexdigit{1,4};
    ls32        = h16 ":" h16 | ipv4address;
    ipv6address
        =                            (h16 ":"){6} ls32
        |                       "::" (h16 ":"){5} ls32
        | (               h16)? "::" (h16 ":"){4} ls32
        | ((h16 ":"){0,1} h16)? "::" (h16 ":"){3} ls32
        | ((h16 ":"){0,2} h16)? "::" (h16 ":"){2} ls32
        | ((h16 ":"){0,3} h16)? "::"  h16 ":"     ls32
        | ((h16 ":"){0,4} h16)? "::"              ls32
        | ((h16 ":"){0,5} h16)? "::"              h16
        | ((h16 ":"){0,6} h16)? "::";
    ipvfuture   = "v" hexdigit+ "." (unreserved | sub_delims | ":" )+;
    ip_literal  = "[" ( ipv6address | ipvfuture ) "]";
    reg_name    = (unreserved | pct_encoded | sub_delims)*;
	unix_socket = ("[" | "[v0.local:") "/" pchar+ ("/" pchar+)* "]";
    host
        = @h1 ip_literal  @h2
        | @h3 ipv4address @h4
        | @h5 reg_name    @h6
		| @h7 unix_socket @h8;
    port      = @r1 digit* @r2;
    authority = (userinfo "@")? host (":" port)?;
    path_abempty  = ("/" pchar*)*;
    path_absolute = "/" (pchar+ ("/" pchar*)*)?;
    path_rootless = pchar+ ("/" pchar*)*;
    path_empty    = "";
    hier_part
        = "//" authority @p1 path_abempty @p2
        | @p3 (path_absolute | path_rootless | path_empty) @p4;
    query    = @q1 (pchar | [/?])* @q2;
    fragment = @f1 (pchar | [/?])* @f2;
    uri = scheme ":" hier_part ("?" query)? ("#" fragment)?;

    *   {
		Tcl_Obj*	ofsObj = NULL;

		pc->fail_ofs = (int)(s-1-str);
		replace_tclobj(&ofsObj, Tcl_NewIntObj(pc->fail_ofs));
		Tcl_SetErrorCode(pc->interp, "REURI", "PARSE", str, Tcl_GetString(ofsObj), NULL);
		replace_tclobj(&ofsObj, NULL);
		Tcl_SetObjResult(pc->interp, Tcl_ObjPrintf("URI parse error at offset %d", pc->fail_ofs));
		pc->rc = TCL_ERROR;
		goto finally;
	}
    end { goto finally; }
    uri {
		replace_tclobj(&pc->uri->scheme, Dedup_NewStringObj(l->dedup_pool, s1, (int)(s2-s1)));
		if (u1) replace_tclobj(&pc->uri->userinfo,	Dedup_NewStringObj(l->dedup_pool, u1, (int)(u2-u1)));

		if (h1) {
			replace_tclobj(&pc->uri->host, Dedup_NewStringObj(l->dedup_pool, h1, (int)(h2 - h1)));
			pc->uri->hosttype = REURI_HOST_IPV6;
		}
		if (h3) {
			replace_tclobj(&pc->uri->host, Dedup_NewStringObj(l->dedup_pool, h3, (int)(h4 - h3)));
			pc->uri->hosttype = REURI_HOST_IPV4;
		}
		if (h5 && h6 > h5) {
			replace_tclobj(&pc->uri->host, Dedup_NewStringObj(l->dedup_pool, h5, (int)(h6 - h5)));
			pc->uri->hosttype = REURI_HOST_HOSTNAME;
		}
		if (h7) {
			replace_tclobj(&pc->uri->host, Dedup_NewStringObj(l->dedup_pool, h7, (int)(h8 - h7)));
			pc->uri->hosttype = REURI_HOST_UNIX;
		}
		// TODO: Add unix domain sockets extension

		if (r1) replace_tclobj(&pc->uri->port,     Dedup_NewStringObj(l->dedup_pool, r1, (int)(r2 - r1)));
		if (p1 && p2>p1) replace_tclobj(&pc->uri->path,     Dedup_NewStringObj(l->dedup_pool, p1, (int)(p2 - p1)));
		if (p3 && p4>p3) replace_tclobj(&pc->uri->path,     Dedup_NewStringObj(l->dedup_pool, p3, (int)(p4 - p3)));
		if (q1) replace_tclobj(&pc->uri->query,    Dedup_NewStringObj(l->dedup_pool, q1, (int)(q2 - q1)));
		if (f1) replace_tclobj(&pc->uri->fragment, Dedup_NewStringObj(l->dedup_pool, f1, (int)(f2 - f1)));
		if ((int)(s-str) < len) {
			Tcl_SetErrorCode(pc->interp, "REURI", "PARSE", NULL);
			Tcl_SetObjResult(pc->interp, Tcl_ObjPrintf("Extra characters remain after URI at offset %d", (int)(s-str)));
			pc->fail_ofs = (int)(s-str);
			pc->rc = TCL_ERROR;
		}
		goto finally;
    }
	*/

finally:
	Tcl_DStringFree(&val);
}

//>>>
Tcl_Obj* percent_encode_query(Tcl_Interp* interp, Tcl_Obj* objPtr, enum reuri_encode_mode mode) //<<<
{
	struct interp_cx*		l = Tcl_GetAssocData(interp, "reuri", NULL);
	Tcl_Obj*				res = NULL;
	const unsigned char*	u;
	const unsigned char*	str = NULL;	// CESU-8
	const unsigned char*	s;
	int						strlen;
	int						c = yycstart;
	Tcl_DString				val;
	/*!stags:re2c:percent_encode format = "const unsigned char *@@{tag}; "; */

	u = s = str = (const unsigned char*)Tcl_GetStringFromObj(objPtr, &strlen);

	Tcl_DStringInit(&val);

	/*!local:re2c:percent_encode
    re2c:api:style             = free-form;
    re2c:define:YYCTYPE        = "unsigned char";
    re2c:define:YYCURSOR       = s;
	re2c:yyfill:enable         = 0;
	re2c:flags:tags            = 1;
	re2c:define:YYGETCONDITION = "c";
	re2c:define:YYSETCONDITION = "c = @@;";

	!use:common;

	mark        = [-_.!~*'()];
	unreserved  = alpha | digit | mark;
	reserved    = ([^] \ end) \ unreserved;

	<start> unreserved* / end {
		res = objPtr;
		goto finally;
	}
	<start> unreserved* / reserved {
		if (yych == '/' && mode == REURI_ENCODE_QUERY)
			goto yyc_start;

		if (s>str)
			Tcl_DStringAppend(&val, (const char*)u, (int)(s-u));

		u = s;
		c = yycmixed;
		goto yyc_mixed;
	}
	<start> unreserved* / cesu8null {
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
		if (yych == '/' && mode == REURI_ENCODE_QUERY) {
			Tcl_DStringAppend(&val, "/", 1);
		} else {
			char buf[4];

			sprintf(buf, "%%%02X", yych);
			Tcl_DStringAppend(&val, buf, 3);
		}
		u = s;
		goto yyc_mixed;
	}
	<mixed> unreserved+ {
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
		Tcl_Panic("Invalid character in CESU-8 bytes returned from Tcl at ofs %d: 0x%02x", (int)(s-str), *s);
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

	!use:common;

	mark        = [-_.!~*'()];
	unreserved  = alpha | digit | mark;
	reserved    = ([^] \ end) \ unreserved;

	end {
		return;
	}
	@u unreserved+ {
		Tcl_DStringAppend(ds, (const char*)u, (int)(s-u));
		goto top;
	}
	cesu8null {
		Tcl_DStringAppend(ds, "%00", 3);
		goto top;
	}
	reserved {
		if (yych == '/' && mode == REURI_ENCODE_QUERY) {
			Tcl_DStringAppend(ds, "/", 1);
		} else {
			sprintf(buf, "%%%02X", yych);
			Tcl_DStringAppend(ds, buf, 3);
		}
		goto top;
	}
	* {
		Tcl_Panic("Invalid character in CESU-8 bytes returned from Tcl at ofs %d: 0x%02x", (int)(s-base), *s);
	}
	*/
}

//>>>
static int _add_name(Tcl_Interp* interp, struct interp_cx* l, Tcl_DString* ds, Tcl_Obj* res_params, Tcl_Obj* res_index, const int pnum) //<<<
{
	int				code = TCL_OK;
	Tcl_Obj*		nameObj = NULL;

	replace_tclobj(&nameObj, Dedup_NewStringObj(l->dedup_pool, Tcl_DStringValue(ds), Tcl_DStringLength(ds)));

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

	replace_tclobj(&valueObj, Dedup_NewStringObj(l->dedup_pool, Tcl_DStringValue(ds), Tcl_DStringLength(ds)));

	TEST_OK_LABEL(finally, code, Tcl_ListObjAppendElement(interp, res_params, valueObj));

finally:
	Tcl_DStringSetLength(ds, 0);
	replace_tclobj(&valueObj, NULL);

	return code;
}

//>>>
int parse_query(Tcl_Interp* interp, const char* str, Tcl_Obj** params, Tcl_Obj** index) //<<<
{
	int						code = TCL_OK;
	struct interp_cx*		l = Tcl_GetAssocData(interp, "reuri", NULL);
	const unsigned char*	base = (const unsigned char*)str;
	const unsigned char*	s = base;
	int						pnum = 0;
	int						c = yycname;
	const unsigned char*	YYMARKER;
	const unsigned char*	start = NULL;
	Tcl_Obj*				res_params = NULL;
	Tcl_Obj*				res_index = NULL;
	Tcl_DString				acc;
	/*!stags:re2c:percent_encode_ds format = "const unsigned char *@@{tag}; "; */

	Tcl_DStringInit(&acc);

	replace_tclobj(&res_params, Tcl_NewListObj(0, NULL));
	replace_tclobj(&res_index,  Tcl_NewDictObj());

	if (*s == '?') s++;				/* Accept optional leading ? */
	if (s[0] == 0) goto finally;	/* Do nothing gracefully */

top:
	/*!local:re2c:parse_query
    re2c:api:style             = free-form;
    re2c:define:YYCTYPE        = "unsigned char";
    re2c:define:YYCURSOR       = s;
	re2c:yyfill:enable         = 0;
	re2c:flags:tags            = 1;
	re2c:define:YYGETCONDITION = "c";
	re2c:define:YYSETCONDITION = "c = @@;";

	!use:common;

	mark        = [-_.!~*'()];
	unreserved  = alpha | digit | mark | "/";
	reserved    = ([^] \ end) \ unreserved;

	<*> @start unreserved+ {
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
	
	<name> "=" {
		TEST_OK_LABEL(finally, code, _add_name(interp, l, &acc, res_params, res_index, pnum++));
		c = yycvalue;
		goto yyc_value;
	}
	<name> end {
		if (Tcl_DStringLength(&acc)) {
			TEST_OK_LABEL(finally, code, _add_name(interp, l, &acc, res_params, res_index, pnum++));
			TEST_OK_LABEL(finally, code, Tcl_ListObjAppendElement(interp, res_params, Dedup_NewStringObj(l->dedup_pool, "", 0)));
		}
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

	<*> * {
		Tcl_Panic("Invalid character in CESU-8 bytes returned from Tcl at ofs %d: 0x%02x", (int)(s-base), *s);
	}
	*/

finally:
	if (code == TCL_OK) {
		replace_tclobj(params, res_params);
		replace_tclobj(index,  res_index);
	}

	replace_tclobj(&res_params,	NULL);
	replace_tclobj(&res_index,	NULL);
	Tcl_DStringFree(&acc);
	return code;
}

//>>>
static int _add_path(Tcl_Interp* interp, struct interp_cx* l, Tcl_DString* ds, Tcl_Obj* res_pathlist) //<<<
{
	int				code = TCL_OK;
	Tcl_Obj*		valueObj = NULL;

	replace_tclobj(&valueObj, Dedup_NewStringObj(l->dedup_pool, Tcl_DStringValue(ds), Tcl_DStringLength(ds)));

	TEST_OK_LABEL(finally, code, Tcl_ListObjAppendElement(interp, res_pathlist, valueObj));

finally:
	Tcl_DStringSetLength(ds, 0);
	replace_tclobj(&valueObj, NULL);

	return code;
}

//>>>
int parse_path(Tcl_Interp* interp, const char* str, Tcl_Obj** pathlist) //<<<
{
	int						code = TCL_OK;
	struct interp_cx*		l = Tcl_GetAssocData(interp, "reuri", NULL);
	const unsigned char*	base = (const unsigned char*)str;
	const unsigned char*	s = base;
	const unsigned char*	YYMARKER;
	const unsigned char*	start = NULL;
	Tcl_Obj*				res_pathlist = NULL;
	Tcl_DString				acc;
	/*!stags:re2c:percent_encode_ds format = "const unsigned char *@@{tag}; "; */

	Tcl_DStringInit(&acc);

	replace_tclobj(&res_pathlist, Tcl_NewListObj(0, NULL));

	if (s[0] == 0) goto finally;	/* Do nothing gracefully */

top:
	/*!local:re2c:parse_path
    re2c:api:style             = free-form;
    re2c:define:YYCTYPE        = "unsigned char";
    re2c:define:YYCURSOR       = s;
	re2c:yyfill:enable         = 0;
	re2c:flags:tags            = 1;

	!use:common;

	mark        = [-_.!~*'()];
	unreserved  = alpha | digit | mark | [=&];
	reserved    = ([^] \ end) \ unreserved;

	@start unreserved+ {
		Tcl_DStringAppend(&acc, (const char*)start, (int)(s-start));
		goto top;
	}
	"+" {
		// Not sure about this
		Tcl_DStringAppend(&acc, " ", 1);
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
	
	"/" {
		if (s == base+1) Tcl_DStringAppend(&acc, "/", 1);
		TEST_OK_LABEL(finally, code, _add_path(interp, l, &acc, res_pathlist));
		goto top;
	}
	end {
		if (s > base+2) {
			TEST_OK_LABEL(finally, code, _add_path(interp, l, &acc, res_pathlist));
		}
		goto finally;
	}

	* {
		Tcl_Panic("Invalid character in CESU-8 bytes returned from Tcl at ofs %d: 0x%02x", (int)(s-base), *s);
	}
	*/

finally:
	if (code == TCL_OK) {
		replace_tclobj(pathlist, res_pathlist);
	}

	replace_tclobj(&res_pathlist, NULL);
	Tcl_DStringFree(&acc);
	return code;
}

//>>>

// vim: ft=c foldmethod=marker foldmarker=<<<,>>>
