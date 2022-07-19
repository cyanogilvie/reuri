#ifndef _REURIINT_H
#define _REURIINT_H

#include "reuri.h"
#include "tclstuff.h"
#include <dedup.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <setjmp.h>

extern const char* reuri_part_str[];

enum reuri_hosttype {
	REURI_HOST_NONE,
	REURI_HOST_IPV6,			// IPv6 literal address: [::1]
	REURI_HOST_IPV4,			// IPv4 literal address: 127.0.0.1
	REURI_HOST_HOSTNAME,		// hostname: localhost
	REURI_HOST_UNIX,			// unix domain socket path: /tmp/myserv.80
	REURI_HOST_SIZE				// Marker for the size of the set
};
extern const char* reuri_hosttype_str[];

struct param {
	Tcl_Obj*		name;
	Tcl_Obj*		value;
	struct param*	next;
};

struct uri {
	Tcl_Obj*			scheme;
	Tcl_Obj*			userinfo;
	Tcl_Obj*			host;
	enum reuri_hosttype	hosttype;
	Tcl_Obj*			port;
	Tcl_Obj*			path;
	Tcl_Obj*			query;
	Tcl_Obj*			fragment;
};

struct parse_context {
	struct uri*		uri;
	Tcl_Interp*		interp;
	int				rc;
	int				fail_ofs;
};

struct interp_cx {
	struct dedup_pool*	dedup_pool;
	const Tcl_ObjType*	typeInt;
	Tcl_Obj*			hosttype[REURI_HOST_SIZE];
};

// reuri.c internal API <<<
int ReuriGetPartFromObj(Tcl_Interp* interp, Tcl_Obj* partObj, enum reuri_part* part);
// reuri.c internal API >>>
// type_uri.c internal API <<<
void ReuriCompile(Tcl_DString* ds, struct uri* uri);
int ReuriGetURIFromObj(Tcl_Interp* interp, Tcl_Obj* uriPtr, struct uri** uri);
// type_uri.c internal API >>>
// parse.re internal API <<<
void parse_uri(struct parse_context* pc, const char* str, int len);
Tcl_Obj* percent_encode(Tcl_Interp* interp, Tcl_Obj* objPtr, enum reuri_encode_mode mode);
void percent_encode_ds(enum reuri_encode_mode mode, Tcl_DString* ds, const char* str);
int parse_query(Tcl_Interp* interp, const char* str, Tcl_Obj** params, Tcl_Obj** index);
int parse_path(Tcl_Interp* interp, const char* str, Tcl_Obj** pathlist);
// parse.re internal API >>>
// type_query.c internal API <<<
int query_add_index(Tcl_Interp* interp, Tcl_Obj* index, Tcl_Obj* name, const int pnum);
int ReuriGetQueryFromObj(Tcl_Interp* interp, Tcl_Obj* query, Tcl_Obj** params, Tcl_Obj** index);
// type_query.c internal API >>>
// type_index.c internal API <<<
enum idx_type {
	IDX_NONE=0,
	IDX_ABS,
	IDX_ENDREL
};

enum parse_status {
	IDX_PARSE_OK = 0,
	IDX_PARSE_ERROR
};

struct idx_atom {
	enum idx_type	type;
	int				val;
};

struct idx_range {
	struct idx_atom from;
	struct idx_atom to;
};

struct idx_set {
	int size;
	int	top;
	struct idx_range*	range;
};

struct parse_idx_cx {
	int						rc;
	const char*				str;
	size_t					failofs;
	const char*				failtype;		/* Must not point at dynamically allocated memory */
	const unsigned char*	p;
	struct idx_set			set;
	jmp_buf					exception;
	struct idx_range		static_ranges[10];
};

struct parse_idx_cx* new_parse_idx_cx(const char* str);
void free_set(struct parse_idx_cx* cx);
void free_parse_idx_cx(struct parse_idx_cx** cx);
void throw(struct parse_idx_cx* cx, const char* failtype, size_t failofs);
void push_range(struct parse_idx_cx* cx, struct idx_range* r);
// type_index.c internal API >>>
// index.re internal API <<<
int parse_index(Tcl_Interp* interp, const char* str, struct parse_idx_cx** indexPtrPtr);
// index.re internal API >>>
// packcc_index.peg internal API <<<
enum parse_status parse_idx_packcc(struct parse_idx_cx* auxil);
// packcc_index.peg internal API >>>
// stack.c internal API <<<
struct stack_entry {
	struct stack_entry*	prev;
	void*				thing;
};

void* stack_push(struct stack_entry** stack_top, size_t len);
void* stack_pop(struct stack_entry** stack_top);
size_t stack_size(struct stack_entry* stack_top);
void stack_discard(struct stack_entry** stack_top);
// stack.c internal API >>>

#ifdef TESTMODE
int testmode_init(Tcl_Interp* interp, struct interp_cx* l);
#endif

#endif
// vim: ft=c foldmethod=marker foldmarker=<<<,>>>
