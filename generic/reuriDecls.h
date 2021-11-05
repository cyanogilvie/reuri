
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

typedef struct ReuriStubs {
    int magic;
    void *hooks;

    int (*reuri_URIObjGetPart) (Tcl_Interp*interp, Tcl_Obj*uriPtr, enum reuri_part part, Tcl_Obj*defaultPtr, Tcl_Obj**valuePtrPtr); /* 0 */
    int (*reuri_URIObjGetAll) (Tcl_Interp*interp, Tcl_Obj*uriPtr, Tcl_Obj**res); /* 1 */
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

#endif /* defined(USE_REURI_STUBS) */

/* !END!: Do not edit above this line. */
