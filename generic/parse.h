
/*!rules:re2c:common

    alpha       = [a-zA-Z];
    digit       = [0-9];
    hexdigit    = [0-9a-fA-F];
    pct_encoded = "%" hexdigit{2};
	end			= "\x00";
	cesu8null   = "\xc0" "\x80";

*/

