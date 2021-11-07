
/* !BEGIN!: Do not edit below this line. */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Exported function declarations:
 */

/* 0 */
EXTERN int		Reuri_URIObjGetPart(Tcl_Interp*interp,
				Tcl_Obj*uriPtr, enum reuri_part part,
				Tcl_Obj*defaultPtr, Tcl_Obj**valuePtrPtr);
/* 1 */
EXTERN int		Reuri_URIObjGetAll(Tcl_Interp*interp, Tcl_Obj*uriPtr,
				Tcl_Obj**res);
/* 2 */
EXTERN int		Reuri_NewQueryObj(Tcl_Interp*interp, int objc,
				Tcl_Obj* const objv[], Tcl_Obj**res);
/* 3 */
EXTERN int		Reuri_CompileQuery(Tcl_Interp*interp, Tcl_DString*ds,
				Tcl_Obj*params);
/* 4 */
EXTERN int		Reuri_CompilePath(Tcl_Interp*interp, Tcl_DString*ds,
				Tcl_Obj*pathListPtr, unsigned long absolute);

typedef struct ReuriStubs {
    int magic;
    void *hooks;

    int (*reuri_URIObjGetPart) (Tcl_Interp*interp, Tcl_Obj*uriPtr, enum reuri_part part, Tcl_Obj*defaultPtr, Tcl_Obj**valuePtrPtr); /* 0 */
    int (*reuri_URIObjGetAll) (Tcl_Interp*interp, Tcl_Obj*uriPtr, Tcl_Obj**res); /* 1 */
    int (*reuri_NewQueryObj) (Tcl_Interp*interp, int objc, Tcl_Obj* const objv[], Tcl_Obj**res); /* 2 */
    int (*reuri_CompileQuery) (Tcl_Interp*interp, Tcl_DString*ds, Tcl_Obj*params); /* 3 */
    int (*reuri_CompilePath) (Tcl_Interp*interp, Tcl_DString*ds, Tcl_Obj*pathListPtr, unsigned long absolute); /* 4 */
} ReuriStubs;

extern const ReuriStubs *reuriStubsPtr;

#ifdef __cplusplus
}
#endif

#if defined(USE_REURI_STUBS)

/*
 * Inline function declarations:
 */

#define Reuri_URIObjGetPart \
	(reuriStubsPtr->reuri_URIObjGetPart) /* 0 */
#define Reuri_URIObjGetAll \
	(reuriStubsPtr->reuri_URIObjGetAll) /* 1 */
#define Reuri_NewQueryObj \
	(reuriStubsPtr->reuri_NewQueryObj) /* 2 */
#define Reuri_CompileQuery \
	(reuriStubsPtr->reuri_CompileQuery) /* 3 */
#define Reuri_CompilePath \
	(reuriStubsPtr->reuri_CompilePath) /* 4 */

#endif /* defined(USE_REURI_STUBS) */

/* !END!: Do not edit above this line. */
