
/*!rules:re2c:common
	alpha		= [a-zA-Z];
	digit		= [0-9];
	hexdigit	= [0-9a-fA-F];
	pct_encoded	= "%" hexdigit{2};
	end			= "\x00";
	cesu8null	= "\xc0" "\x80";
*/

/*!rules:re2c:uri
	!use:common;

	unreserved  = alpha | digit | [-._~];
	sub_delims  = [!$&'()*+,;=];
	pchar       = unreserved | pct_encoded | sub_delims | [:@];
	allowed     = unreserved | sub_delims \ [=&];	// Remove =& from the universally allowed sub_delims, since the query rules don't allow them
	reserved    = ([^] \ end) \ allowed;

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
	uri = (scheme ":")? hier_part ("?" query)? ("#" fragment)?;
*/
