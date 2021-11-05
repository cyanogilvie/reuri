#ifndef _REURIINT_H
#define _REURIINT_H

#include "reuri.h"
#include "tclstuff.h"
#include <dedup.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern const char* reuri_part_str[];

enum reuri_hosttype {
	REURI_HOST_NONE,
	REURI_HOST_IPV6,			// IPv6 literal address: [::1]
	REURI_HOST_IPV4,			// IPv4 literal address: 127.0.0.1
	REURI_HOST_HOSTNAME,		// hostname: localhost
	REURI_HOST_UNIX				// unix domain socket path: /tmp/myserv.80
};

struct param {
	Tcl_Obj*		name;
	Tcl_Obj*		value;
	struct param*	next;
};

struct uri {
	Tcl_Obj*			scheme;
	Tcl_Obj*			user;
	Tcl_Obj*			password;
	Tcl_Obj*			host;
	enum reuri_hosttype	hosttype;
	Tcl_Obj*			port;
	Tcl_Obj*			path;
	Tcl_Obj*			pathlist;
	Tcl_Obj*			query;
	Tcl_Obj*			fragment;
	struct param*		first_param;
};

struct parse_context {
	struct uri*		uri;
	Tcl_Interp*		interp;
	int				rc;
	int				fail_ofs;
};

struct interp_cx {
	struct dedup_pool*	dedup_pool;
};

// reuri.c internal API
int ReuriGetPartFromObj(Tcl_Interp* interp, Tcl_Obj* partObj, enum reuri_part* part);

// type_uri.c internal API
void ReuriCompile(Tcl_DString* ds, struct uri* uri);
int ReuriGetURIFromObj(Tcl_Interp* interp, Tcl_Obj* uriPtr, struct uri** uri);

// parse.re internal API
void parse_uri(struct parse_context* pc, const char* str, int len);

#endif
