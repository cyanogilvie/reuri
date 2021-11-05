// Based on https://re2c.org/examples/c/submatch/example_uri_rfc3986.html

#include "reuriInt.h"

void parse_uri(struct parse_context* pc, const char* str, int len)
{
	struct interp_cx*	l = Tcl_GetAssocData(pc->interp, "reuri", NULL);
    const char
        *s1, *u1, *h1, *h3, *h5, *r1, *p1, *p3, *q1, *f1,
        *s2, *u2, *h2, *h4, *h6, *r2, *p2, *p4, *q2, *f2;
	const char*			s = str;
	const char*			YYMARKER;
	Tcl_DString			val;
	/*!stags:re2c format = "const char *@@{tag}; "; */

	Tcl_DStringInit(&val);

	/*!re2c
    re2c:api:style             = free-form;
    re2c:define:YYCTYPE        = "char";
    re2c:define:YYCURSOR       = s;
	re2c:yyfill:enable         = 0;
	re2c:flags:tags            = 1;

    end = "\x00";

    alpha       = [a-zA-Z];
    digit       = [0-9];
    hexdigit    = [0-9a-fA-F];
    unreserved  = alpha | digit | [-._~];
    pct_encoded = "%" hexdigit{2};
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
    host
        = @h1 ip_literal  @h2
        | @h3 ipv4address @h4
        | @h5 reg_name    @h6;
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
		pc->fail_ofs = (int)(s-str);
		pc->rc = TCL_ERROR;
		goto finally;
	}
    end { goto finally; }
    uri {
		replace_tclobj(&pc->uri->scheme, Dedup_NewStringObj(l->dedup_pool, s1, (int)(s2-s1)));
        //if (u1) fprintf(stderr, "  userinfo: %.*s\n", (int)(u2 - u1), u1);

		if (h1) {
			replace_tclobj(&pc->uri->host, Dedup_NewStringObj(l->dedup_pool, h1, (int)(h2 - h1)));
			pc->uri->hosttype = REURI_HOST_IPV6;
		}
		if (h3) {
			replace_tclobj(&pc->uri->host, Dedup_NewStringObj(l->dedup_pool, h3, (int)(h4 - h3)));
			pc->uri->hosttype = REURI_HOST_IPV4;
		}
		if (h5) {
			replace_tclobj(&pc->uri->host, Dedup_NewStringObj(l->dedup_pool, h5, (int)(h6 - h5)));
			pc->uri->hosttype = REURI_HOST_HOSTNAME;
		}
		// TODO: Add unix domain sockets extension, record host type in struct uri

		if (r1) replace_tclobj(&pc->uri->port,     Dedup_NewStringObj(l->dedup_pool, r1, (int)(r2 - r1)));
		if (p1) replace_tclobj(&pc->uri->path,     Dedup_NewStringObj(l->dedup_pool, p1, (int)(p2 - p1)));
		if (p3) replace_tclobj(&pc->uri->path,     Dedup_NewStringObj(l->dedup_pool, p3, (int)(p4 - p3)));
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


// vim: ft=c foldmethod=marker foldmarker=<<<,>>>
