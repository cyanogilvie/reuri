
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
EXTERN Tcl_Obj*		Reuri_PercentEncodeObj(Tcl_Interp*interp,
				enum reuri_encode_mode mode, Tcl_Obj*objPtr);
/* 3 */
EXTERN int		Reuri_NewQueryObj(Tcl_Interp*interp, int objc,
				Tcl_Obj* const objv[], Tcl_Obj**res);
/* 4 */
EXTERN int		Reuri_CompileQuery(Tcl_Interp*interp, Tcl_DString*ds,
				Tcl_Obj*params);
/* 5 */
EXTERN int		Reuri_CompilePath(Tcl_Interp*interp, Tcl_DString*ds,
				Tcl_Obj*pathListPtr);
/* 6 */
EXTERN int		Reuri_URIObjPartExists(Tcl_Interp*interp,
				Tcl_Obj*uriPtr, enum reuri_part part,
				int*existsPtr);
/* Slot 7 is reserved */
/* 8 */
EXTERN int		Reuri_GetPathFromObj(Tcl_Interp*interp,
				Tcl_Obj*pathPtr, Tcl_Obj**pathlistPtrPtr);
/* Slot 9 is reserved */
/* 10 */
EXTERN Tcl_Obj*		Reuri_PercentDecodeObj(Tcl_Obj*in);
/* 11 */
EXTERN int		Reuri_GetPartFromObj(Tcl_Interp*interp,
				Tcl_Obj*partObj, enum reuri_part*part);
/* 12 */
EXTERN int		Reuri_URIObjExtractPart(Tcl_Interp*interp,
				Tcl_Obj*uriPtr, enum reuri_part part,
				Tcl_Obj*defaultPtr, Tcl_Obj**valuePtrPtr);
/* 13 */
EXTERN int		Reuri_URIObjExtractAll(Tcl_Interp*interp,
				Tcl_Obj*uriPtr, Tcl_Obj**res);
/* 14 */
EXTERN int		Reuri_URIObjSet(Tcl_Interp*interp, Tcl_Obj*uriPtr,
				enum reuri_part part, Tcl_Obj*valuePtr,
				Tcl_Obj**resPtrPtr);

typedef struct ReuriStubs {
    int magic;
    void *hooks;

    int (*reuri_URIObjGetPart) (Tcl_Interp*interp, Tcl_Obj*uriPtr, enum reuri_part part, Tcl_Obj*defaultPtr, Tcl_Obj**valuePtrPtr); /* 0 */
    int (*reuri_URIObjGetAll) (Tcl_Interp*interp, Tcl_Obj*uriPtr, Tcl_Obj**res); /* 1 */
    Tcl_Obj* (*reuri_PercentEncodeObj) (Tcl_Interp*interp, enum reuri_encode_mode mode, Tcl_Obj*objPtr); /* 2 */
    int (*reuri_NewQueryObj) (Tcl_Interp*interp, int objc, Tcl_Obj* const objv[], Tcl_Obj**res); /* 3 */
    int (*reuri_CompileQuery) (Tcl_Interp*interp, Tcl_DString*ds, Tcl_Obj*params); /* 4 */
    int (*reuri_CompilePath) (Tcl_Interp*interp, Tcl_DString*ds, Tcl_Obj*pathListPtr); /* 5 */
    int (*reuri_URIObjPartExists) (Tcl_Interp*interp, Tcl_Obj*uriPtr, enum reuri_part part, int*existsPtr); /* 6 */
    void (*reserved7)(void);
    int (*reuri_GetPathFromObj) (Tcl_Interp*interp, Tcl_Obj*pathPtr, Tcl_Obj**pathlistPtrPtr); /* 8 */
    void (*reserved9)(void);
    Tcl_Obj* (*reuri_PercentDecodeObj) (Tcl_Obj*in); /* 10 */
    int (*reuri_GetPartFromObj) (Tcl_Interp*interp, Tcl_Obj*partObj, enum reuri_part*part); /* 11 */
    int (*reuri_URIObjExtractPart) (Tcl_Interp*interp, Tcl_Obj*uriPtr, enum reuri_part part, Tcl_Obj*defaultPtr, Tcl_Obj**valuePtrPtr); /* 12 */
    int (*reuri_URIObjExtractAll) (Tcl_Interp*interp, Tcl_Obj*uriPtr, Tcl_Obj**res); /* 13 */
    int (*reuri_URIObjSet) (Tcl_Interp*interp, Tcl_Obj*uriPtr, enum reuri_part part, Tcl_Obj*valuePtr, Tcl_Obj**resPtrPtr); /* 14 */
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
#define Reuri_PercentEncodeObj \
	(reuriStubsPtr->reuri_PercentEncodeObj) /* 2 */
#define Reuri_NewQueryObj \
	(reuriStubsPtr->reuri_NewQueryObj) /* 3 */
#define Reuri_CompileQuery \
	(reuriStubsPtr->reuri_CompileQuery) /* 4 */
#define Reuri_CompilePath \
	(reuriStubsPtr->reuri_CompilePath) /* 5 */
#define Reuri_URIObjPartExists \
	(reuriStubsPtr->reuri_URIObjPartExists) /* 6 */
/* Slot 7 is reserved */
#define Reuri_GetPathFromObj \
	(reuriStubsPtr->reuri_GetPathFromObj) /* 8 */
/* Slot 9 is reserved */
#define Reuri_PercentDecodeObj \
	(reuriStubsPtr->reuri_PercentDecodeObj) /* 10 */
#define Reuri_GetPartFromObj \
	(reuriStubsPtr->reuri_GetPartFromObj) /* 11 */
#define Reuri_URIObjExtractPart \
	(reuriStubsPtr->reuri_URIObjExtractPart) /* 12 */
#define Reuri_URIObjExtractAll \
	(reuriStubsPtr->reuri_URIObjExtractAll) /* 13 */
#define Reuri_URIObjSet \
	(reuriStubsPtr->reuri_URIObjSet) /* 14 */

#endif /* defined(USE_REURI_STUBS) */

/* !END!: Do not edit above this line. */
