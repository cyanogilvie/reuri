% reuri(3) 0.1 | URI Manipulation for Tcl
% Cyan Ogilvie
% 0.1


# NAME

reuri - URI Manipulation for Tcl


# SYNOPSIS

**package require reuri** ?0.1?

**reuri::uri** **get** *uri* ?*part* ?*default*??\
**reuri::uri** **exists** *uri* *part*\
?? **reuri::uri** **set** *variable* *part* *value*\
?? **reuri::uri** **valid** *uri*\
?? **reuri::uri** **context** *uri* *script*\
?? **reuri::uri** **resolve** *uri*\
?? **reuri::uri** **absolute** *uri*\
**reuri::uri** **encode** **query**|**path**|**host** *value*\
?? **reuri::uri** **decode** *value*

?? **reuri::query** **get** *query* ?*param* ?*default*??\
?? **reuri::query** **values** *query* *param*\
?? **reuri::query** **add** *variable* *param* *value*\
?? **reuri::query** **exists** *query* *param*\
?? **reuri::query** **set** *variable* *param* *value* ?*offset*?\
?? **reuri::query** **unset** *variable* *param* ?*offset*?\
?? **reuri::query** **names** *query*\
?? **reuri::query** **reorder** *variable* *params*\
**reuri::query** **encode** *params*|?*param* *value* ...?\
**reuri::query** **decode** *query*

**reuri::path** **split** *path*\
?? **reuri::path** **join** *segments*\
?? **reuri::path** **get** *path* *index*\
?? **reuri::path** **set** *variable* *index* *value*\
?? **reuri::path** **unset** *variable* *index*\
?? **reuri::path** **resolve** *path*

### Note
Commands marked up as "?? **reuri::foo**" are not yet implemented, or only
partially implemented.


# DESCRIPTION

This package allows efficient manipulation of URI strings from Tcl.  A fast
parser is used to extract the parts from the URI string representation and
cache them in a custom Tcl\_ObjType.  Subsequent accesses read or update the
existing internal parsed data, and updating the string representation
recompiles the URI string representation from the parsed parts.

This package serves a similar purpose to the **uri** module of tcllib, but is
much faster.  Where splitting a typical http url using the tcllib module might
take half a millisecond typically, parsing and extracting a part with this
package is on the order of 100 times faster, which matters if hundreds of URLs
have to be manipulated in the time budget of serving a hit on a website.

While this package is in the 0 major version series this API is not stable and
will change in backwards-incompatible ways.  Starting with a 1 series release
the API will preserve backwards compatibility within a major version number
sequence.  <span color="red">**USING THE 0 SERIES VERSIONS IN PRODUCTION
CODE IS NOT RECOMMENDED.**</span>


# COMMANDS

**reuri::uri** **get** *uri* ?*part* ?*default*??
:	Return the URL *part* from *uri* or throw an exception if *uri* can't be parsed or
	doesn't have the specified *part* and no *default* was specified.  If *part* isn't
    defined in *uri* (but is a valid part name) and a *default* is given, that will be
    returned instead.  If *part* isn't specified, return a dictionary containing all
    parts.  See **PARTS** below for valid parts, and **EXCEPTIONS** for the
    exceptions that might be thrown.

**reuri::uri** **exists** *uri* *part*
:	Return true if *uri* contains *part*, false otherwise.

**reuri::uri** **set** *variable* *part* *value*
:	Replace the *part* of the URI value contained in *variable* with *value*, returning
    the updated uri value stored in *variable*.

**reuri::uri** **valid** *uri*
:   Return true if *uri* is a syntactically valid URI and can be parsed by this package.

**reuri::uri** **context** *uri* *script*
:	Evaluate *script* in the current call frame but treat *uri* as the context for
    resolving any relative references that occur while processing *script*.  The return
    state of *script* is transparently propagated to the return state of this command,
    including exceptions and return codes like break and continue.  This command can
    be nested, with each successive call creating an inner scope with a reference URI
    that is resolved relative to its context.

**reuri::uri** **resolve** *uri*
:   Return a URI by resolving the possibly-relative *uri* in the context of any
    **resolve::uri** **context** calls on the callstack.

**reuri::uri** **absolute** *uri*
:   Return true if *uri* is absolute (ie. not a relative URI).

**reuri::uri** **encode** **query**|**path**|**host** *value*
:   Percent-encode the UTF-8 representation of *value*, suitable for inclusion as
    component of the part given by **query**, **path** or **host**.

**reuri::uri** **decode** *value*
:   Percent decode *value*, the inverse of **reuri::uri** **encode**.

**reuri::query** **get** *query* ?*param* ?*default*??
:   Retrieve the value for the named *param* in the *query* part.  If *param*
    occurs multiple times in the query part, returns the value for the last instance.
    If *param* doesn't exist and *default* is supplied, return that in its place,
    otherwise throw the **REURI** **PARAM_NOT_SET** exception.  If *param* is not supplied,
    return all of the query parameters as a list of alternating parameter names and
    their values, in the order they appear in *query* (similar to a dictionary but
    multiple instances of the same param name can occur).

**reuri::query** **values** *query* *param*
:   Return a list of all of the values for *param* in the *query*, in
    the order they appear in the URI.

**reuri::query** **add** *variable* *param* *value*
:   Append the query *param* with *value* to the query part contained in
    the variable *variable*.  If *param* already exists in the query part, append
    the new instance, leaving existing instances in place.

**reuri::query** **exists** *query* *param*
:   Return true if *param* exists in *query*.

**reuri::query** **set** *variable* *param* *value* ?*offset*?
:   Replace any instances of *param* in the query part stored in *variable*
    with a single instance containing *value*.  If *offset* is specified, then
    replace only that instance of *param* in *variable*, or throw exception
    **REURI** **BAD_OFFSET** if the offset isn't valid.

**reuri::query** **unset** *variable* *param* ?*offset*?
:   Remove all instances of *param* in *variable*, unless *offset* is specified in which
    case only remove the specified instance, or throw **REURI** **BAD_OFFSET** if
    the offset isn't valid.

**reuri::query** **names** *query*
:   Return a list of all of the param names that appear in *query*, in the order they
    appear.  If multiple instances occur then the result contains multiple instances
    in the corresponding positions.

**reuri::query** **reorder** *variable* *names*
:   Reorder the query part so that the params occur in the order given by *names*,
    with any that weren't specified in *names* occuring after all those that were.
    If any duplicate param names exist on the URI, all their instances are placed
    in the position given in *names*, with the instances preserving their relative
    positions.

**reuri::query** **encode** *params*|?*param* *value* ...?
:   Percent-encode the supplied *params* (as a list with pairs of param and value),
    or in the *param* and *value* arguments and return a properly formatted query
    string.  If the supplied parameter set is empty then the result is a blank
    string, otherwise it will have a "?" character prefixed.  Parameters with an
    empty corresponding value are encoded without an "=".

**reuri::query** **decode** *query*
:   Decode *query* into a list of params and their values.

**reuri::path** **split** *path*
:   Return a fully decoded list of path segments in *path*.

**reuri::path** **set** *variable* *index* *value*
:   

**reuri::path** **unset** *variable* *index*
:   

**reuri::path** **join** *segments*
:   Produce a properly encoded URI path part given the list of *segments*.

**reuri::path** **resolve** *path*
:   Return *path* resolved in the context of all the URIs on the callstack from
    **reuri::uri** **context** calls.


# PARTS

The named parts of URIs that are accessible in this package are:

**scheme**
:   The part before the first `:` like `http` in `http://github.com`

**userinfo**
:   The userinfo part: `admin:secret` in `http://admin:secret@example.com`

**host**
:   The host part of the authority section: `google.com` in `https://google.com`,
    `127.0.0.1` in `http://127.0.0.1:8080`, `::1` in `http://[::1]:8080`, and
    `/tmp/myserv.80` in `http://[/tmp/myserv.80]/foo`.  This last example isn't
    valid by RFC 3986 but is one of the common ways to refer to the socket in
    HTTP-over-unix sockets.

**port**
:   The numeric port number portion of the authority section, `1234` in
    `http://localhost:1234/foo`.

**path**
:   `/foo/bar` in `http://localhost/foo/bar?quux=123`.

**query**
:   `thing=foo&other=bar` in `http://localhost/path?thing=foo&other=bar#frag`

**fragment**
:   `endbit` in `http://localhost/foo?x=y&a=b#endbit`



# EXCEPTIONS

Exceptions with errorcodes matching the following patterns will be thrown by these commands
if the supplied URIs aren't valid, the specified part isn't valid, or some other assumption
was violated:

**REURI** **PARSE_ERROR** *uri* *offset*
:   Parsing the supplied URI value *uri* failed at character offset *offset*.

**REURI** **INVALID_PART** *part*
:   The specified *part* isn't a valid part (possibly for this type of URI).

**REURI** **PART_NOT_SET** *part*
:   The requested *part* is not defined in the supplied URI value, and no default was provided.

**REURI** **PARAM_NOT_SET** *param*
:   The requested *param* is not defined in the query part of the supplied URI value,
    and no default was provided.

**REURI** **BAD_OFFSET** *param* *offset*
:   The specified *offset* wasn't valid for *param* in the supplied URI.

**REURI** **UNBALANCED_PARAMS**
:   The supplied set of parameters isn't even.  Each parameter name must have a matching value.

# C API

A C API is exposed via the stubs mechanism:

int **Reuri_URIObjGetPart**(*interp*, *uriPtr*, *part*, *defaultPtr*, *valuePtrPtr*)

int **Reuri_URIObjSetPart**(*interp*, *uriPtr*, *part*, *valuePtr*)

int **Reuri_URIObjSetParam**(*interp*, *uriPtr*, *paramPtr*, *valuePtr*, *paramMode*)

TODO: complete

## ARGUMENTS

Tcl\_Interp *\*interp*
:   A Tcl interpreter, which will have its result updated with and exception
    thrown, if supplied.  Can be NULL in which case the exception info is discarded.

Tcl\_Obj *\*uriPtr*
:   A pointer to the URI value.  Must be unshared for those calls that change its value.

enum reuri\_part *part*
:   One of **REURI_SCHEME**, **REURI_USERINFO**, **REURI_HOST**,
    **REURI_PORT**, **REURI_PATH**, **REURI_QUERY** or **REURI_FRAGMENT**.

Tcl\_Obj *\*defaultPtr*
:   A pointer to the default value to be returned if the requested element isn't
    present on the URI.  Can be NULL if no default value is desired.

Tcl\_Obj *\*\*valuePtrPtr*
:   Location where the requested element will be stored.

Tcl\_Obj *\*valuePtr*
:   The new value to store for the specified element.

Tcl\_Obj *\*paramPtr*
:   The name of the query parameter.

enum reuri\_param\_mode *paramMode*
:   One of **REURI_PARAM_ADD** or **REURI_PARAM_REPLACE**.


# EXAMPLES

Extract the host and port from a URI, with a scheme-defined default port:

~~~tcl
proc connect uri {
    switch [reuri::uri get $uri scheme] {
        http    {set default_port 80}
        https   {set default_port 443}
        default {error "Unsupported scheme \"[reuri::uri get $uri scheme]\""}
    }

    socket [reuri::uri get $uri host] [reuri::uri get $uri port $default_port]
}
~~~

Update query parameters from a dictionary:

~~~tcl
proc mergeparams {uri patch} {
    foreach {k v} $patch {
        reuri::uri query set uri $k $v
    }

    return $uri
}
~~~

Retrieve the host and store each resolved address as a query param in c using the stubs API:

~~~c
int ResolveAddrCmd(ClientData cdata, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    int code = TCL_OK, rc;
    Tcl_Obj *host = NULL;
    Tcl_Obj *uri = NULL;
    Tcl_Obj *paramname = NULL;
    Tcl_Obj *paramval = NULL;
    struct addrinfo *res = NULL;
    struct addrinfo *addr = NULL;

    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "uri")
        code = TCL_ERROR;
        goto finally;
    }

    /* We're going to modify the uri object's value,
     * so ensure it is unshared:
     */
    uri = objv[1];
    if (Tcl_IsShared(uri))
        uri = Tcl_DuplicateObj(uri);
    Tcl_IncrRefCount(uri);

    /* Retrieve the host portion from the URI: */
    code = Reuri_URIObjGetPart(interp, uri, REURI_HOST, &host);
    if (TCL_OK != code) goto finally;

    rc = getaddrinfo(Tcl_GetString(host), NULL, NULL, &res);
    if (rc) {
        Tcl_SetObjResult(interp, Tcl_ObjPrintf(interp,
                "Resolve error: %s", gai_strerror(rc)));
        code = TCL_ERROR;
        goto finally;
    }

    Tcl_IncrRefCount(paramname = Tcl_NewStringObj("addr", 4));

    for (addr=res; addr; addr=addr->ai_next) {
        const char *addrstr[NI_MAXHOST];

        /* Format the numeric address as a string, whatever its
         * address family
         */
        rc = getnameinfo(addr->ai_addr, addr->ai_addrlen,
                host, sizeof(host), NULL, 0, NI_NUMERICHOST);
        if (rc) {
            Tcl_SetObjResult(interp,
                    Tcl_NewStringObj("Could not format address", -1));
            goto finally;
        }

        if (paramval) Tcl_DecrRefCount(paramval);
        Tcl_IncrRefCount(paramval = Tcl_NewStringObj(addrstr, -1));

        /*
         * Append another &addr= param to the query part to store
         * this address.  REURI_PARAM_ADD signals that we want
         * to add another instance of &addr= to the URI, not replace
         * any that are there already.
         */
        code = Reuri_URIObjSetParam(interp, uri,
                paramname, paramval, REURI_PARAM_ADD);
        if (TCL_OK != code) goto finally;
    }

    /* Return the changed uri value: */
    Tcl_SetObjResult(interp, uri);

finally:
    if (res) {
        freeaddrinfo(res);
        res = NULL;
    }

    if (uri) {
        Tcl_DecrRefCount(uri);
        uri = NULL;
    }

    if (paramname) {
        Tcl_DecrRefCount(paramname);
        paramname = NULL;
    }

    if (paramval) {
        Tcl_DecrRefCount(paramval);
        paramval = NULL;
    }

    return code;
}
~~~

# CONFORMING TO

This package aims to conform to RFC 3986, with the optional addition of recognising
HTTP-over-unix sockets style URLs like `http://[/tmp/myserv.80]/foo?bar=baz`.


# BUGS

Please report any bugs to the github issue tracker: https://github.com/cyanogilvie/reuri/issues


# SEE ALSO

The uri module of tcllib.


# TODO
- [x] Implement http://[/tmp/mysock.80]/foo style unix domain sockets support
- [ ] Implement **reuri::uri set**
- [ ] Implement **reuri::uri valid**
- [ ] Implement **reuri::uri context**
- [ ] Implement **reuri::uri resolve**
- [ ] Implement **reuri::uri absolute**
- [ ] Implement **reuri::uri decode**
- [ ] Implement **reuri::query get**
- [ ] Implement **reuri::query values**
- [ ] Implement **reuri::query add**
- [ ] Implement **reuri::query exists**
- [ ] Implement **reuri::query set**
- [ ] Implement **reuri::query unset**
- [ ] Implement **reuri::query names**
- [ ] Implement **reuri::query reorder**
- [x] Implement **reuri::path split**
- [ ] Implement **reuri::path get**
- [ ] Implement **reuri::path set**
- [ ] Implement **reuri::path unset**
- [ ] Implement **reuri::path join**
- [ ] Implement **reuri::path resolve**

# LICENSE

This package is Copyright 2021 Cyan Ogilvie, and is made available under the
same license terms as the Tcl Core

