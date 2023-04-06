
/*!rules:re2c:common
	alpha		= [a-zA-Z];
	digit		= [0-9];
	hexdigit	= [0-9a-fA-F];
	pct_encoded	= "%" hexdigit{2};
	end			= "\x00";
	cesu8null	= "\xc0" "\x80";
*/

// uri <<<
/*!rules:re2c:uri
	!use:common;

	unreserved  = alpha | digit | [-._~];
	sub_delims  = [!$&'()*+,;=];
	highchar	= [\x80-\U0010FFFF];
	pchar       = unreserved | pct_encoded | sub_delims | [:@];
	uchar       = pchar | highchar;

	// These are the minimally restrictive classes that can unambiguously classify the parts of the uri in lenient input mode
	userinfo_uchar		= [^] \ end \ "[" \ [@/?#]  | pct_encoded;
	reg_name_uchar		= [^] \ end \ "[" \ [@:/?#] | pct_encoded;
	path_rootless_uchar	= [^] \ end \ [/?#:]        | pct_encoded;
	path_uchar			= [^] \ end \ [/?#]         | pct_encoded;
	unix_socket_uchar	= [^] \ end \ "]" \ "/"     | pct_encoded;
	query_uchar			= [^] \ end \ "#"           | pct_encoded;
	fragment_uchar		= [^] \ end                 | pct_encoded;
	
	allowed     = unreserved | sub_delims \ [=&+];	// Remove =&+ from the universally allowed sub_delims, since the query rules don't allow them
	reserved    = ([^] \ end) \ allowed;

	scheme = @s1 alpha (alpha | digit | [-+.])* @s2;
	userinfo = @u1 userinfo_uchar* @u2;
	dec_octet
		= digit
		| [1-9] digit
		| "1" digit{2}
		| "2" [0-4] digit
		| "25" [0-5];
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
	reg_name    = reg_name_uchar*;
	unix_socket = ("[" | "[v0.local:") "/" unix_socket_uchar+ ("/" unix_socket_uchar+)* "]";
	host
		= @h1 ip_literal  @h2
		| @h3 ipv4address @h4
		| @h5 reg_name    @h6
		| @h7 unix_socket @h8;
	port      = @r1 digit* @r2;
	authority = (userinfo "@")? host (":" port)?;
	path_abempty  = ("/" path_uchar*)*;
	path_absolute = "/" (path_uchar+ ("/" path_uchar*)*)?;
	path_rootless = path_rootless_uchar+ ("/" path_uchar*)*;
	path_empty    = "";
	hier_part
		= "//" authority @p1 path_abempty @p2
		| @p3 (path_absolute | path_rootless | path_empty) @p4;
	query    = @q1 query_uchar* @q2;
	fragment = @f1 fragment_uchar* @f2;
	uri      = (scheme ":")? hier_part ("?" query)? ("#" fragment)?;
*/
// uri >>>

// uri-strict <<<
/*!rules:re2c:uri_strict
	!use:common;

	unreserved  = alpha | digit | [-._~];
	sub_delims  = [!$&'()*+,;=];
	pchar       = unreserved | pct_encoded | sub_delims | [:@];
	allowed     = unreserved | sub_delims \ [=&+];	// Remove =&+ from the universally allowed sub_delims, since the query rules don't allow them
	reserved    = ([^] \ end) \ allowed;

	scheme = @s1 alpha (alpha | digit | [-+.])* @s2;
	userinfo = @u1 (unreserved | pct_encoded | sub_delims | ":")* @u2;
	dec_octet
		= digit
		| [1-9] digit
		| "1" digit{2}
		| "2" [0-4] digit
		| "25" [0-5];
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
	unix_socket = ("[" | "[v0.local:") @h7 "/" pchar+ ("/" pchar+)* @h8 "]";
	host
		= @h1 ip_literal  @h2
		| @h3 ipv4address @h4
		| @h5 reg_name    @h6
		| unix_socket;
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
	uri      = (scheme ":")? hier_part ("?" query)? ("#" fragment)?;
*/
// uri-strict >>>

/*!rules:re2c:tags_fail
	re2c:api                   = custom;
	re2c:api:style             = free-form;
	re2c:flags:tags            = 1;
	re2c:yyfill:enable         = 0;
	re2c:define:YYCTYPE        = "unsigned char";
	re2c:define:YYPEEK         = "*s";
	re2c:define:YYSKIP         = "++s;";
	re2c:define:YYBACKUP       = "fail = s>fail ? s : fail; mar = s;";
	re2c:define:YYRESTORE      = "fail = s>fail ? s : fail; s = mar;";
	re2c:define:YYSTAGP        = "@@{tag} = s;";
 	re2c:define:YYSTAGN        = "@@{tag} = NULL;";
	re2c:define:YYSHIFT        = "s += @@{shift};";
	re2c:define:YYSHIFTSTAG    = "@@{tag} += @@{shift};";
*/

/*!rules:re2c:notags_fail
	re2c:api                   = custom;
	re2c:api:style             = free-form;
	re2c:flags:tags            = 0;
	re2c:yyfill:enable         = 0;
	re2c:define:YYCTYPE        = "unsigned char";
	re2c:define:YYPEEK         = "*s";
	re2c:define:YYSKIP         = "++s;";
	re2c:define:YYBACKUP       = "fail = s>fail ? s : fail; mar = s;";
	re2c:define:YYRESTORE      = "fail = s>fail ? s : fail; s = mar;";
*/

/*!rules:re2c:tags_fail_utf8
	re2c:api                   = custom;
	re2c:api:style             = free-form;
	re2c:flags:tags            = 1;
	re2c:yyfill:enable         = 0;
	re2c:encoding:utf8         = 1;
	re2c:define:YYCTYPE        = "unsigned char";
	re2c:define:YYPEEK         = "*s";
	re2c:define:YYSKIP         = "++s;";
	re2c:define:YYBACKUP       = "fail = s>fail ? s : fail; mar = s;";
	re2c:define:YYRESTORE      = "fail = s>fail ? s : fail; s = mar;";
	re2c:define:YYSTAGP        = "@@{tag} = s;";
 	re2c:define:YYSTAGN        = "@@{tag} = NULL;";
	re2c:define:YYSHIFT        = "s += @@{shift};";
	re2c:define:YYSHIFTSTAG    = "@@{tag} += @@{shift};";
*/

// vim: foldmethod=marker foldmarker=<<<,>>> ts=4 shiftwidth=4
