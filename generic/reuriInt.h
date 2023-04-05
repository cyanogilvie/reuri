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
	struct dedup_pool*	dedup_scheme;
	struct dedup_pool*	dedup_userinfo;
	struct dedup_pool*	dedup_host_reg_name;
	struct dedup_pool*	dedup_host_ipv4;
	struct dedup_pool*	dedup_host_ipv6;
	struct dedup_pool*	dedup_host_local;
	struct dedup_pool*	dedup_port;
	struct dedup_pool*	dedup_path;
	struct dedup_pool*	dedup_query;
	struct dedup_pool*	dedup_fragment;
	const Tcl_ObjType*	typeInt;
	Tcl_Obj*			hosttype[REURI_HOST_SIZE];
	Tcl_Obj*			empty;
	Tcl_Obj*			empty_list;
	Tcl_Obj*			empty_query;
	Tcl_Obj*			t;
	Tcl_Obj*			f;
	Tcl_Obj*			apply;
	Tcl_Obj*			sort_unique;
};

typedef int (update_rep)(Tcl_Interp* interp, Tcl_Obj* obj);		// interp may be NULL

// Extend Tcl_ObjType with a callback for generating the normalized rep
struct Reuri_ObjType {
	Tcl_ObjType		base;
	update_rep*		Reuri_UpdateDecodedProc;		// string rep -> fully decoded
	update_rep*		Reuri_UpdateNormalizedProc;		// decoded -> normalized encoded
};
typedef struct Reuri_ObjType Reuri_ObjType;

// reuri.c internal API <<<
int ReuriGetPartFromObj(Tcl_Interp* interp, Tcl_Obj* partObj, enum reuri_part* part);
// reuri.c internal API >>>
// type_uri.c internal API <<<
void ReuriCompile(Tcl_DString* ds, struct uri* uri);
int ReuriGetURIFromObj(Tcl_Interp* interp, Tcl_Obj* uriPtr, struct uri** uri);
void ReuriSetURI(Tcl_Obj* uriPtr, struct uri* uri);
// type_uri.c internal API >>>
// parse.re internal API <<<
void parse_uri(struct parse_context* pc, const char* str);
int uri_valid(const char* str);
Tcl_Obj* percent_encode(Tcl_Interp* interp, Tcl_Obj* objPtr, enum reuri_encode_mode mode);
Tcl_Obj* percent_encode_awssig(Tcl_Interp* interp, Tcl_Obj* objPtr);
void percent_encode_ds(enum reuri_encode_mode mode, Tcl_DString* ds, const char* str);
int percent_decode(Tcl_Obj* str, Tcl_Obj** res);
void percent_decode_ds(const char* str, Tcl_DString* ds);
int decode_query(Tcl_Interp* interp, const char* str, Tcl_Obj** params, Tcl_Obj** index);
int decode_path(Tcl_Interp* interp, const char* str, Tcl_Obj** pathlist);
void ascii_lowercase_ds(Tcl_DString* ds, const char* str);
int parse_host_local(Tcl_Interp* interp, const char* str, Tcl_Obj** pathlist);
int parse_host_ipv6(Tcl_Interp* interp, const char* str, Tcl_Obj** addr);
int decode_port(Tcl_Interp* interp, const char* str, int* portnum);
int parse_scheme  (Tcl_Interp* interp, Tcl_Obj* in, Tcl_Obj** out);
int parse_userinfo(Tcl_Interp* interp, Tcl_Obj* in, Tcl_Obj** out);
int parse_host(Tcl_Interp* interp, Tcl_Obj* in, Tcl_Obj** out, enum reuri_hosttype* hosttype);
int parse_port(Tcl_Interp* interp, Tcl_Obj* in, Tcl_Obj** out);
int parse_path(Tcl_Interp* interp, Tcl_Obj* in, Tcl_Obj** out);
int parse_query(Tcl_Interp* interp, Tcl_Obj* in, Tcl_Obj** out);
int parse_fragment(Tcl_Interp* interp, Tcl_Obj* in, Tcl_Obj** out);
// parse.re internal API >>>
// type_index.c internal API <<<
enum idx_atom_type {
	IDX_NONE=0,
	IDX_ABS,
	IDX_ENDREL
};

enum parse_status {
	IDX_PARSE_OK = 0,
	IDX_PARSE_ERROR
};

struct idx_atom {
	enum idx_atom_type	type;
	int					val;
};

struct idx_range {
	struct idx_atom from;
	struct idx_atom to;
};

struct idx_set {
	int					size;
	int					top;
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
// type_uri_part.c internal API <<<
extern Reuri_ObjType	scheme_objtype;
extern Reuri_ObjType	userinfo_objtype;
extern Reuri_ObjType	host_reg_name_objtype;
extern Reuri_ObjType	host_ipv4_objtype;
extern Reuri_ObjType	host_ipv6_objtype;
extern Reuri_ObjType	host_local_objtype;
extern Reuri_ObjType	port_objtype;
extern Reuri_ObjType	path_objtype;
extern Reuri_ObjType	query_objtype;
extern Reuri_ObjType	fragment_objtype;

int Reuri_GetDecodedFromPart(Tcl_Interp* interp, Tcl_Obj* obj, Reuri_ObjType* objtype, Tcl_Obj** decoded);
int Reuri_GetNormalizedFromPart(Tcl_Interp* interp, Tcl_Obj* obj, Reuri_ObjType* objtype, Tcl_Obj** normalized);
Reuri_ObjType* host_objtype(enum reuri_hosttype hosttype);
Tcl_Obj* Reuri_NewPartFromString(Reuri_ObjType* objtype, const char* str, int len);
int Reuri_GetPathFromObj(Tcl_Interp* interp, Tcl_Obj* pathPtr, Tcl_Obj** pathlistPtrPtr);
int Reuri_CompilePath(Tcl_Interp* interp, Tcl_DString* ds, Tcl_Obj* pathListPtr);
int query_add_index(Tcl_Interp* interp, Tcl_Obj* index, Tcl_Obj* name, const int pnum);
int ReuriGetQueryFromObj(Tcl_Interp* interp, Tcl_Obj* query, Tcl_Obj** params, Tcl_Obj** index);
void ReuriSetQuery(Tcl_Obj* query, Tcl_Obj* params, Tcl_Obj* index);
enum reuri_pathtype Reuri_PathType(Tcl_Obj* pathPtr);
// type_uri_part.c internal API >>>
// index.re internal API <<<
int parse_index(Tcl_Interp* interp, const char* str, struct parse_idx_cx** indexPtrPtr);
int IdxGetIndexFromObj(Tcl_Interp* interp, Tcl_Obj* indexObj, struct parse_idx_cx** index);
// index.re internal API >>>
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
// vim: ft=c foldmethod=marker foldmarker=<<<,>>> ts=4 shiftwidth=4
