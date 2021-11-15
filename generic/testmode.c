#ifdef TESTMODE
#include "reuriInt.h"

static int TestObjCmd(ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj* const objv[]) //<<<
{
	int			code = TCL_OK;
	static const char*	methods[] = {
		"parse_index_re2c",
		"parse_index_packcc",
		NULL
	};
	enum {
		M_PARSE_INDEX_RE2C,
		M_PARSE_INDEX_PACKCC
	};
	int	methodidx;

	if (objc < 2) {
		Tcl_WrongNumArgs(interp, 1, objv, "method ?arg ...?");
		code = TCL_ERROR;
		goto finally;
	}

	TEST_OK_LABEL(finally, code, Tcl_GetIndexFromObj(interp, objv[1], methods, "method", TCL_EXACT, &methodidx));

	switch (methodidx) {
		case M_PARSE_INDEX_RE2C: //<<<
			{
				enum {
					A_cmd=1,
					A_INDEX,
					A_objc
				};
				struct parse_idx_cx*	cx = NULL;

				CHECK_ARGS_LABEL(finally, code, "index");

				code = parse_index(interp, Tcl_GetString(objv[A_INDEX]), &cx);
				free_parse_idx_cx(&cx);
				if (code != TCL_OK)
					goto finally;
			}
			break;
			//>>>
		case M_PARSE_INDEX_PACKCC: //<<<
			{
				enum {
					A_cmd=1,
					A_INDEX,
					A_objc
				};
				enum parse_status		rc;
				struct parse_idx_cx*	cx = NULL;

				CHECK_ARGS_LABEL(finally, code, "index");

				cx = new_parse_idx_cx(Tcl_GetString(objv[A_INDEX]));
				rc = parse_idx_packcc(cx);
				if (rc != IDX_PARSE_OK) {
					char	buf[3*sizeof(cx->failofs)+2];

					sprintf(buf, "%ld", cx->failofs);
					code = TCL_ERROR;
					Tcl_SetErrorCode(interp, "REURI", "INDEX", "PARSE", cx->failtype, buf, NULL);
					Tcl_SetObjResult(interp, Tcl_ObjPrintf("Error parsing index: %s at %ld", cx->failtype, cx->failofs));
				}
				free_parse_idx_cx(&cx);
				if (code != TCL_OK)
					goto finally;
			}
			break;
			//>>>
		default: Tcl_Panic("Invalid method index: %d", methodidx);
	}

finally:
	return code;
}

//>>>

#define NS	"::reuri"
static struct cmd {
	char*			name;
	Tcl_ObjCmdProc*	proc;
} cmds[] = {
	{NS "::_test",	TestObjCmd},
	{NULL,			NULL}
};

int testmode_init(Tcl_Interp* interp, struct interp_cx* l) //<<<
{
	int			code = TCL_OK;
	struct cmd*	c = cmds;

	if (getenv("REURI_TESTMODE") == NULL)
		return code;

	while (c->name) {
		if (NULL == Tcl_CreateObjCommand(interp, c->name, c->proc, l, NULL)) {
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("Could not create command %s", c->name));
			code = TCL_ERROR;
			goto finally;
		}
		c++;
	}

finally:
	return code;
}

//>>>

#endif
// vim: ts=4 shiftwidth=4 foldmethod=marker foldmarker=<<<,>>>
