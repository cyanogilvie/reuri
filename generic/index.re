#include "reuriInt.h"

/*!types:re2c*/
/*!i nclude:re2c "parse.h"*/
/*!rules:re2c:common
    re2c:api:style             = free-form;
    re2c:define:YYCTYPE        = "unsigned char";
    re2c:define:YYCURSOR       = s;
	re2c:yyfill:enable         = 0;
	re2c:flags:tags            = 1;
	re2c:define:YYGETCONDITION = "c";
	re2c:define:YYSETCONDITION = "c = @@;";


    alpha       = [a-zA-Z];
    digit       = [0-9];
    hexdigit    = [0-9a-fA-F];
    pct_encoded = "%" hexdigit{2};
	end			= "\x00";
	cesu8null   = "\xc0" "\x80";
*/


#if 0
// From https://re2c.org/examples/c/submatch/example_http_rfc7230.html <<<
typedef struct mtag_t {
    struct mtag_t *pred;
    long dist;
} mtag_t;

typedef struct mtagpool_t {
    mtag_t *head;
    mtag_t *next;
    mtag_t *last;
} mtagpool_t;

static void mtagpool_init(mtagpool_t *mtp)
{
    static const unsigned size = 256;
    mtp->head = (mtag_t*)malloc(size * sizeof(mtag_t));
    mtp->next = mtp->head;
    mtp->last = mtp->head + size;
}

static void mtagpool_free(mtagpool_t *mtp)
{
    free(mtp->head);
    mtp->head = mtp->next = mtp->last = NULL;
}

static mtag_t *mtagpool_next(mtagpool_t *mtp)
{
    unsigned size;
    mtag_t *head;

    if (mtp->next < mtp->last) return mtp->next++;

    size = mtp->last - mtp->head;
    head = (mtag_t*)malloc(2 * size * sizeof(mtag_t));
    memcpy(head, mtp->head, size * sizeof(mtag_t));
    free(mtp->head);
    mtp->head = head;
    mtp->next = head + size;
    mtp->last = head + size * 2;
    return mtp->next++;
}

static void mtag(mtag_t **pmt, const unsigned char *b, const unsigned char *t, mtagpool_t *mtp)
{
    mtag_t *mt = mtagpool_next(mtp);
    mt->pred = *pmt;
    mt->dist = t - b;
    *pmt = mt;
}
// From https://re2c.org/examples/c/submatch/example_http_rfc7230.html >>>
#endif

static inline long int _strntol(const char* nptr, int limit, const char** endptr, int base) // Like strtol but convert at most limit bytes <<<
{
	char	buf[limit+1];
	char*	end;
	long	conv;

	memcpy(buf, nptr, limit);
	buf[limit] = 0;
	conv = strtol(buf, &end, base);
	if (endptr) *endptr = nptr + (end-buf);
	return conv;
}

//>>>
int parse_index(Tcl_Interp* interp, const char* str, struct parse_idx_cx** indexPtrPtr) //<<<
{
	int							code = TCL_OK;
	const unsigned char*const	base = (const unsigned char*const)str;
	const unsigned char*		s = base;
	const unsigned char			*n, *hex_s, *decimal_s, *octal_s, *binary_s;
	/*!stags:re2c:index format = "const unsigned char *@@;\n"; */
	const unsigned char*		YYMARKER;
	long						c = yycindex;
	struct idx_range			range = {0};
	struct parse_idx_cx*		cx = new_parse_idx_cx(str);

#define SET_ATOM(a, t) \
	do { \
		if      (decimal_s)	a.val = _strntol((const char*)decimal_s,	(int)(s - decimal_s),	NULL, 10); \
		else if (hex_s)		a.val = _strntol((const char*)hex_s,		(int)(s - hex_s),		NULL, 16); \
		else if (octal_s)	a.val = _strntol((const char*)octal_s,		(int)(s - octal_s),		NULL, 8); \
		else if (binary_s)	a.val = _strntol((const char*)binary_s,		(int)(s - binary_s),	NULL, 2); \
		else                a.val = 0; \
		if (n) a.val = -a.val; \
		a.type = t; \
		goto top; \
	} while(0)

#define COMPLETE_RANGE(a, t) \
	do { \
		if      (decimal_s)	a.val = _strntol((const char*)decimal_s,	(int)(s - decimal_s),	NULL, 10); \
		else if (hex_s)		a.val = _strntol((const char*)hex_s,		(int)(s - hex_s),		NULL, 16); \
		else if (octal_s)	a.val = _strntol((const char*)octal_s,		(int)(s - octal_s),		NULL, 8); \
		else if (binary_s)	a.val = _strntol((const char*)binary_s,		(int)(s - binary_s),	NULL, 2); \
		else                a.val = 0; \
		if (n) a.val = -a.val; \
		a.type = t; \
		push_range(cx, &range); \
		goto top; \
	} while(0);

top:
	/*!local:re2c:index

	!use:common;

	octdigit		= [0-7];
	bit				= [01];
	decimal			= @decimal_s digit+;
	hex				= "0x" @hex_s hexdigit+;
	binary			= "0b" @binary_s bit+;
	octal			= "0" @octal_s octdigit+;
	number			= hex | binary | octal | decimal;
	relatom			= "end" (("+" | @n "-") number)?;
	absatom			= ("+"? | @n "-") number;

	<index> relatom				=> postindex { SET_ATOM(range.from, IDX_ENDREL);	}
	<index> absatom				=> postindex { SET_ATOM(range.from, IDX_ABS);		}

	<postindex> ".." relatom	=> postrange { COMPLETE_RANGE(range.to, IDX_ENDREL);	}
	<postindex> ".." absatom	=> postrange { COMPLETE_RANGE(range.to, IDX_ABS);		}

	<postindex> ","				=> index	{ push_range(cx, &range); range=(struct idx_range){0}; goto top; }
	<postrange> ","				=> index	{                         range=(struct idx_range){0}; goto top; }

	<postindex> end							{ push_range(cx, &range); goto finally; }
	<postrange> end							{                         goto finally; }

	<*> * {
	  	char	buf[3*sizeof(ptrdiff_t)+2];

		sprintf(buf, "%ld", s-1-base);
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("Index syntax error at %d: 0x%02x", (int)(s-1-base), yych));
		Tcl_SetErrorCode(interp, "REURI", "INDEX", "PARSE", str, buf, NULL);
		code = TCL_ERROR;
		goto finally;
	}
	*/

finally:
	if (code == TCL_OK) {
		*indexPtrPtr = cx;
	} else {
		free_parse_idx_cx(&cx);
	}

	return code;
}

//>>>

// vim: ft=c foldmethod=marker foldmarker=<<<,>>>
