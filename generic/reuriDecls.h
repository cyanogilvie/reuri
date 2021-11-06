
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

typedef struct ReuriStubs {
    int magic;
    void *hooks;

    int (*reuri_URIObjGetPart) (Tcl_Interp*interp, Tcl_Obj*uriPtr, enum reuri_part part, Tcl_Obj*defaultPtr, Tcl_Obj**valuePtrPtr); /* 0 */
    int (*reuri_URIObjGetAll) (Tcl_Interp*interp, Tcl_Obj*uriPtr, Tcl_Obj**res); /* 1 */
    Tcl_Obj* (*reuri_PercentEncodeObj) (Tcl_Interp*interp, enum reuri_encode_mode mode, Tcl_Obj*objPtr); /* 2 */
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

#endif /* defined(USE_REURI_STUBS) */

/* !END!: Do not edit above this line. */
