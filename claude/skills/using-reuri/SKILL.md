---
name: using-reuri
description: Parse, manipulate, and build URIs in Tcl using the reuri package. Use when working with URLs/URIs in Tcl, building or rewriting query strings, percent-encoding, splitting paths, or any time the user mentions reuri, URI manipulation, or http URL handling in Tcl.
---

## Package overview

reuri is a fast URI parser/manipulator. URIs are stored as ordinary Tcl
string values with a cached parsed internal representation; repeated
operations on the same value reuse the parse, so accessing parts is
~3-10x faster than the first parse and ~1000x faster than `uri::split`
from tcllib.

```tcl
package require reuri
```

## Two command namespaces — read carefully

- **`reuri ...`** operates on a *whole URI* (any of: full URL, relative ref,
  query-only `?foo=bar`, fragment-only `#frag`, scheme-only `http:`, or even
  empty).
- **`reuri::query ...`** and **`reuri::path ...`** operate on a *raw query
  or path string* (no scheme/host/etc.). Use these when you have an
  already-extracted query/path or are building one from scratch.

`reuri query <op> $uri ...` is shorthand for `reuri::query <op>
[query-portion-of-$uri] ...` — same op names, same args after the URI.

## Empty vs absent: three states per component (0.15+)

reuri follows RFC 3986 §6.2.3 strictly: an *empty* component (with its
delimiter present) is **not** equivalent to an *absent* component.
Each optional part has three observable states:

| state | query example | host example | port example |
|------|------|------|------|
| absent | `/a/b` | `http:foo` | `http://host` |
| present-empty | `/a/b?` | `http://` | `http://host:` |
| present-nonempty | `/a/b?x=1` | `http://host` | `http://host:80` |

All three round-trip through parse + serialize, and all three are
distinguished by `reuri exists`:

```tcl
reuri exists /a/b query          ;# => 0
reuri exists /a/b? query         ;# => 1
reuri exists /a/b?x=1 query      ;# => 1

reuri normalize /a/b?            ;# => /a/b?   (the "?" is preserved)
reuri normalize http://          ;# => http:// (empty authority preserved)
reuri normalize http://host:     ;# => http://host:  (empty port preserved)
```

Port `0` is distinct from an empty port and both distinct from absent:

```tcl
reuri normalize http://host      ;# => http://host     (no port)
reuri normalize http://host:     ;# => http://host:    (empty port)
reuri normalize http://host:0    ;# => http://host:0   (literal port 0, e.g. for OS-assigned-port listener configs)
```

**Path is the exception: always present** per RFC 3986 §3.3
(`path-empty = 0<pchar>`). `reuri exists $uri path` always returns 1;
`reuri get $uri path` / `reuri extract $uri path` always return a string
(possibly empty).

## `reuri set` vs `reuri unset` (0.15+)

- `reuri set u part value` — sets the part. An empty string sets the
  part to an explicit empty component (**not** removed):
  ```tcl
  set u /x?a=b
  reuri set u query {}         ;# u is now /x? — empty-but-present query
  ```
- `reuri unset u part` — removes the part entirely:
  ```tcl
  set u /x?a=b
  reuri unset u query          ;# u is now /x — no query at all
  ```

For **path**, which can't be absent, `reuri unset u path` resets it to
empty (same effect as `reuri set u path {}`).

### Passing a default for path raises (0.15+)

Because path is always present, `reuri get $u path <default>` and
`reuri extract $u path <default>` raise `REURI PART_ALWAYS_PRESENT`
rather than silently ignore the default. Drop the default argument,
and treat an empty string as the "no significant path" case if your
code needs that.

## The most important distinction: `get` vs `extract`

Both take an optional *part* and an optional default:

- **`reuri get $uri ?part? ?default?`** — returns the *decoded structured*
  value. For `path` it's a Tcl list of segments; for `query` it's a flat
  list of name/value pairs (like a dict but with possible duplicates).
- **`reuri extract $uri ?part? ?default?`** — returns the *raw encoded
  string* exactly as it appears in the URI.

```tcl
set u {http://h/a/b%20c?x=1&y=hello%20world}
reuri get     $u path     ;# => {/ a {b c}}        (list, decoded)
reuri extract $u path     ;# => /a/b%20c           (string, encoded)
reuri get     $u query    ;# => {x 1 y {hello world}}
reuri extract $u query    ;# => x=1&y=hello%20world
```

Without a *part*, both return a dict of all set parts (using the same
decoded-vs-encoded rule). Absent parts are omitted from the dict;
present-empty parts appear with empty-string values.

If a *part* is absent and no *default* is given, both throw `REURI
PART_NOT_SET <part>`. With a *default*, the default is returned instead.
(Exception: `path`, for which passing a default raises `REURI
PART_ALWAYS_PRESENT` — see above.)

### Valid parts

`scheme`, `userinfo`, `host`, `hosttype`, `port`, `path`, `query`,
`fragment`. `hosttype` is read-only and is one of: `none`, `hostname`,
`ipv4`, `ipv6`, `local` (the last is for unix-socket-style hosts in
brackets, e.g. `http://[/tmp/sock]/`).

### `host` is a list of segments when hosttype is `local`

For unix-socket hosts, `reuri get $u host` returns a Tcl list of decoded
path segments — same convention as `path`, with the leading `/` element
marking absoluteness:

```tcl
reuri get     {http://[/var/run/myapp.sock]/api} host  ;# => {/ var run myapp.sock}
reuri extract {http://[/var/run/myapp.sock]/api} host  ;# => [/var/run/myapp.sock]
```

Neither form is the bare `/var/run/myapp.sock` string. To get that for
comparison or for passing to `unix_sockets::connect` from the `unix_sockets` package:

```tcl
file join {*}[reuri get $u host]   ;# => /var/run/myapp.sock
```

(file join since the string generated will be interpreted by the OS, not
using URL quoting rules.)

(For `hostname`/`ipv4`/`ipv6` hosts, `reuri get` returns a plain string
and no list-stitching is needed.)

### Setting a local host

When *assigning* to `host`, include the brackets in the value:

```tcl
reuri set u host {[/var/run/myapp.sock]}     ;# correct, hosttype becomes "local"
reuri set u host /var/run/myapp.sock         ;# WRONG — throws REURI PARSE
```

`[/sock]:port` is also legal (unix socket *and* a port), e.g.
`http://[/tmp/myserv.80]:9000/foo`, but since local sockets do not
have the concept of a port, probably shouldn't be used anywhere.

## Mutating commands take a *variable name*, not a value

This trips people up. `set`, `unset`, `add`, `new` all take a variable
name (like `lappend`, `dict set`):

```tcl
set u http://example.com/
reuri set u path /api/v1            ;# u is now http://example.com/api/v1
reuri query set u limit 10          ;# u is now http://example.com/api/v1?limit=10
reuri query add u tag foo           ;#   ?limit=10&tag=foo
reuri query add u tag bar           ;#   ?limit=10&tag=foo&tag=bar (duplicates ok)
reuri query unset u tag             ;#   ?limit=10
reuri unset u query                 ;# removes the query entirely
```

`reuri set $u part value` (with `$` — passing the value, not the var
name) is **wrong** and will not do what you want.

The variable need not exist; mutating an unset variable starts from `{}`.

## Building URIs

Build piecewise — each `reuri set`/`reuri query add` updates the cached
parsed rep and defers serialisation until the string value is later fetched:

```tcl
set u {}
reuri set u scheme https
reuri set u host example.com
reuri set u port 8443
reuri set u path /api/v1/users
reuri query set u limit 10
reuri query set u q hello
# u => https://example.com:8443/api/v1/users?limit=10&q=hello
```

Or build a query string standalone with one of:

- `reuri::query new param value ?param value ...?` — returns
  `param=value&...` with **no leading `?`**. Also accepts a single list
  arg: `reuri::query new {param value param value}`.
- `reuri::query encode ...` — same args, but **prepends `?`** (or returns
  empty for an empty input). Useful when concatenating onto a base URL
  yourself.

Both encode names and values; trailing `=` is omitted when the value is
empty (`reuri::query new foo {}` => `foo`).

### Conflicts when assembling

reuri rejects part combinations that can't be serialised back as a valid
URI, throwing `REURI CONFLICT`:

- Setting a `host` on a URI with a *rootless* path (e.g. `http:foo` or
  bare `foo/bar`) — the path would become ambiguous. Make the path
  absolute first (`reuri set u path /foo`) or start from `{}`.
- Setting a rootless path (`reuri set u path bar`) on a URI that already
  has a host. Use `/bar` instead.

### Removing a part vs setting present-empty (0.15+)

Two distinct operations:

| action | command | effect on `u = /x?a=b` |
|---|---|---|
| remove the query | `reuri unset u query` | `/x` (no `?`) |
| set an empty query | `reuri set u query {}` | `/x?` (empty but present) |

Same distinction for `host`, `port`, `userinfo`, `fragment`. For
`scheme` there's no empty-vs-absent (scheme must have at least one
character if present). For `path`, `unset` resets to empty since path
can't be absent.

## Percent-encoding — pick the right mode

`reuri encode <mode> $value` percent-encodes `$value` for use in a
specific URI component. Pick the mode matching the part you're inserting
into; each mode follows the RFC 3986 rules for that component.

| mode | use for |
|------|---------|
| `query` | query parameter *name* |
| `queryval` | query parameter *value* (allows raw `=` in the value) |
| `path` | a single path segment (first segment of a rootless path) |
| `path2` | a single path segment (any segment except a rootless first) |
| `host` | host component |
| `userinfo` | userinfo |
| `fragment` | fragment |
| `awssig` | building canonical strings for AWS Sig v4 (strictest) |

`path` vs `path2`: `path2` allows raw `:` (legal mid-path); `path`
encodes `:` because the *first* segment of a rootless path can't contain
`:` without being mistaken for a `scheme:rest` URI.

`reuri decode $value` is the inverse: percent-decodes, replaces `+` with
space, and passes invalid `%XY` sequences through unchanged.

`reuri::query decode $query` decodes a whole query string into a flat
`{name value name value ...}` list. A leading unencoded `?` is stripped.
This is the inverse of `reuri::query new` (and of `reuri::query encode`
modulo the `?`).

You usually do **not** need to call `reuri encode` manually when using
`reuri set`/`reuri query set`/`reuri query add` — those encode their
arguments for you (param names with `query` rules, param values with
`queryval` rules — so a raw `=` in a value passes through unencoded).
Encode manually when you're building a part-string yourself, then pass
the encoded string to `reuri set`.

## Paths

```tcl
reuri::path get /one/two        ;# => {/ one two}    leading "/" element marks absolute
reuri::path get one/two         ;# => {one two}      no leading "/" => rootless
reuri::path get /               ;# => {/}
reuri::path get /one/two/       ;# => {/ one two {}} trailing slash => empty final segment
reuri::path get {}              ;# => {}             empty path

reuri path get $uri             ;# same, but takes any URI
reuri path get $uri 0           ;# pick element by index
reuri path get $uri end         ;# last
reuri path get $uri 1..end      ;# range

reuri::path join                       ;# => {}    no args
reuri::path join /                     ;# => /
reuri::path join / a b                 ;# => /a/b   leading "/" arg => absolute
reuri::path join a b                   ;# => a/b    rootless
reuri::path join / api {v 1} {x/y}     ;# => /api/v%201/x%2Fy
```

`reuri::path join` percent-encodes each argument as a single segment,
**including any `/` inside it**. This is the canonical way to put a
literal `/` in one segment: pass it as one arg and it becomes `%2F`.

`reuri::path split` is a deprecated alias for `reuri::path get`.

`reuri::path set` and `reuri::path resolve` are **not yet implemented**
(documented but stubs only). To rewrite a path, use the
get-list / lset / join / set pattern:

```tcl
# Replace the 3rd path element (the "c" in /a/b/c/d) with a literal "X Y"
set u https://h/a/b/c/d
set segs [reuri get $u path]              ;# => {/ a b c d}
                                          ;# WATCH OUT: index 0 is "/" not "a"
lset segs 3 {X Y}                         ;# segs => {/ a b {X Y} d}
reuri set u path [reuri::path join {*}$segs]
# u => https://h/a/b/X%20Y/d
```

For an absolute path, element 0 of the get-list is the literal `/` marker
— off-by-one bait. Use `reuri::path join {*}$segs` to round-trip the
list back to a path string (the leading `/` element makes the result
absolute again).

## Queries

```tcl
# Operate on raw query strings:
reuri::query get    foo=bar&x=y                  ;# => {foo bar x y}    flat list
reuri::query get    foo=bar&foo=baz foo          ;# => baz   (last wins by default)
reuri::query get    foo=bar foo -default missing ;# => bar
reuri::query get    foo=bar baz -default missing ;# => missing
reuri::query values foo=a&x=y&foo=b foo          ;# => {a b}   all values
reuri::query exists foo=bar foo                  ;# => 1
reuri::query names  a=1&b=2&a=3                  ;# => {a b a}

reuri::query set    qvar foo bar baz quux        ;# qvar set to foo=bar&baz=quux (replaces all instances of the set params)
reuri::query add    qvar foo bar                 ;# appends, allowing duplicates
reuri::query unset  qvar foo other               ;# removes all instances of named params

# Same operations but operating on a whole URI:
reuri query set     uvar foo bar
reuri query add     uvar tag baz
reuri query get     $uri foo -default {}
reuri query values  $uri foo
reuri query names   $uri
```

`reuri query edit $uri name value -name value ...` is a single-shot
batch editor: positional pairs are name/value to *replace*, a `-name`
arg removes that param, a trailing lone name (no value) sets it
empty-valued. Returns the new URI string.

```tcl
reuri query edit ?foo=bar&x=y&foo=baz a first -foo b second
# => ?x=y&a=first&b=second
```

## Indices (for `path get`/`path exists`)

Superset of Tcl `lindex`:

- Integers: `0`, `-1`, `0x10`, `0o7`, `0b101`
- `end`, `end-1`, `end+2`
- Ranges: `0..end`, `1..3`, `end..0` (reversed)
- Comma-separated lists: `1,3,end`, `0..2,end`
- Out-of-range elements yield empty strings (for `get`) or `0` (for `exists`)

> **Gotcha**: a range `a..b` iterates in whichever direction the *resolved*
> endpoints land — the written direction is not consulted. `1..end` on a
> length-1 list resolves to `from=1, to=0`, so it silently descends and yields
> `{{} /}` rather than an empty list. Same for `5..end` on a short list —
> descends through OOB indices. If you want `lrange`-style "empty when
> out-of-bounds" semantics, guard on `llength` first.

## Validation

- `reuri valid $uri` — strict RFC 3986 check on the *literal source
  string*: returns 0 if the string itself contains raw non-ASCII
  codepoints, raw control chars (incl. NUL), or malformed `%XY`. A
  well-formed `%00` is fine — validity is about the source bytes, not
  the decoded content. This is **stricter than the parser**: most other
  commands accept raw Unicode `>=0x80` and auto-encode it on output.
- `reuri normalize $uri` — canonical form: lowercases scheme + host,
  resolves redundant percent-encodings, encodes Unicode, etc. **Preserves
  empty components** per RFC 3986 §6.2.3 — it does *not* strip a trailing
  `?`, `#`, `:`, or `//` with empty authority. It also does **not** strip
  scheme-default ports (e.g. `:80` for `http`); if you want either kind
  of collapsing for a particular application, do it explicitly (e.g.
  `reuri unset u query` / `reuri unset u port`).
- `reuri exists $uri part` — returns 1 if `part` is present (empty or
  not), 0 if absent. `path` always returns 1.

## Migrating from 0.14.x to 0.15+

0.15 made reuri strictly RFC-3986-compliant: empty components are now
preserved (were previously collapsed to absent), and path is always
present (was sometimes reported absent). If you have code targeting
0.14.x or earlier, audit for these patterns:

### 1. `reuri set u part ""` no longer clears

In 0.14.x, passing an empty string to `reuri set` cleared the part. In
0.15+, it sets the part to a present-empty component. To clear, use the
new `reuri unset`.

```tcl
# 0.14.x — clears the query
reuri set u query ""

# 0.15+ — one of these, depending on intent:
reuri unset u query        ;# clear (most old code wants this)
reuri set u query {}       ;# set an explicit empty query (new, rarely wanted)
```

Same replacement for `host`, `port`, `userinfo`, `fragment`. For `path`,
there is no "absent" state: use `reuri unset u path` (same effect as
`reuri set u path ""`) to reset to empty.

### 2. Defaults for `path` now raise

Because path is always present, `reuri get $u path <default>` and
`reuri extract $u path <default>` raise `REURI PART_ALWAYS_PRESENT`.
Old code that relied on a default when parsing authority-only URIs
like `http://host` needs to be updated:

```tcl
# 0.14.x — default of "/" kicked in for http://host
set path [reuri extract $url path /]

# 0.15+ — get an empty string and substitute manually
set path [reuri extract $url path]
if {$path eq ""} {set path /}
```

### 3. `reuri exists $u path` is always 1

Code that used `reuri exists $u path` to detect "has a path after the
authority" won't work. Test the value instead:

```tcl
# 0.14.x — false for http://host
reuri exists $u path

# 0.15+ — test emptiness of the extracted value
expr {[reuri extract $u path] ne ""}
```

### 4. `reuri exists $u host` is now 1 for authority-only URIs

`http://` / `http://:81` / `http://?x=1` all have a present-empty host
now (hosttype `hostname`). If your code checked `reuri exists $u host`
to mean "has a usable hostname", test non-emptiness instead:

```tcl
# 0.14.x — false for http://:81
reuri exists $u host

# 0.15+ — same intent, explicit
expr {[reuri extract $u host] ne ""}
```

### 5. `reuri query unset` on the last param leaves `?`

In 0.14.x, unsetting the only remaining query param collapsed the URI
to no query. In 0.15+ it leaves `foo?` (an explicit empty query). If
you want the collapsed form, follow with an unset:

```tcl
# 0.14.x — /x (no ?) after unsetting the last param
set u /x?a=1
reuri query unset u a            ;# => /x in 0.14.x, /x? in 0.15+

# 0.15+ — add an unset to collapse when empty
reuri query unset u a
if {[reuri extract $u query] eq ""} { reuri unset u query }
```

### 6. `file:` scheme normalization behaviour

0.14.x would silently drop an empty authority from `file://` URIs
(yielding `file:`). 0.15+ preserves `//` per the RFC. If you were
relying on `file:///foo` normalizing to `file:/foo`, do the rewrite
yourself.

## The `encode_query_val_eq` quirk (AWS CloudFront workaround)

CloudFront mis-parses `=` in query *values* as a parameter separator.
Enable the quirk to force `=` to be percent-encoded inside values:

```tcl
reuri::quirk encode_query_val_eq 1
# Now reuri query set u param value=with=equals
# yields ...?param=value%3Dwith%3Dequals  (instead of param=value=with=equals)
```

The quirk is process-global and affects values *generated after* it's
set; cached string reps generated before the toggle don't change. Set
the quirk once early. Not available in safe interps.

## Performance — keep the parsed internal rep alive

reuri caches parsed parts on the Tcl_Obj. If you'll touch a URI more
than once, **store the URI in a variable and reuse it**. Don't
reconstruct the same URI string in a loop — that throws away the cached
parse.

```tcl
# GOOD — single parse, all subsequent ops are ~0.1µs
foreach uri $uri_list {
    set scheme   [reuri get $uri scheme]
    set host     [reuri get $uri host]
    set query    [reuri query get $uri]
}

# BAD — re-parses every iteration
foreach uri $uri_list {
    # each string concat returns a fresh pure-string value, is re-parsed each time:
    set scheme   [reuri get $uri#foo scheme]
    set host     [reuri get $uri#foo host]
    set query    [reuri query get $uri#foo]
}
```

`reuri set $varname ...` mutates the variable in place via the cached
parsed rep — no full re-parse, no reserialize until the string rep
is later requested. So a sequence of `reuri set u ...` / `reuri query set u ...`
calls on the same variable is efficient.

Bench numbers (microseconds per op, illustrative):

| op | reuri (cached) | reuri (fresh parse) | tcllib |
|---|---:|---:|---:|
| Parse + extract one part | 0.10 | 0.34 | 89 |
| Decode whole query → list | 0.07 | 1.1 | 43 |
| Encode a 5-param query | — | 1.3 | 15 |
| Validate a URI | 0.04 | 0.09 | 45+ |

## Exceptions

errorCode patterns to expect (and to catch when needed):

- `REURI PARSE_ERROR <str> <offset>` — input couldn't be parsed
- `REURI INVALID_PART <part>` — `<part>` is not a valid part name
- `REURI PART_NOT_SET <part>` — part isn't present and no default given
- `REURI PART_ALWAYS_PRESENT path` — 0.15+, default passed for `path`
- `REURI PARAM_NOT_SET <name>` — query param missing, no `-default`
- `REURI CONFLICT` — incompatible part change (rootless+host, etc.)
- `REURI UNBALANCED_PARAMS` — odd number of args to query new/encode/set

## Common pitfalls

1. **`reuri get $uri path` is not a string.** It's a Tcl list of decoded
   segments. To get the encoded string form, use `reuri extract $uri
   path`. Same for `query`.
2. **Don't double-`?`-prefix.** `reuri::query encode` already prepends
   `?`; `reuri::query new` does not. If concatenating onto a base
   ending in `?`, use `new`.
3. **Mutating commands take variable names, not values.** `reuri set u
   path /x` — `u` is the *name*, no `$`.
4. **Setting a host on a rootless URI raises CONFLICT.** Set/normalize
   the path to absolute first.
5. **`reuri::query` operates on a query string, `reuri query` on a
   URI.** Don't pass a full URI to `reuri::query get`; don't pass a
   bare query to `reuri query get` expecting it to be parsed (it'll be
   parsed as a relative URI with that as the path).
6. **`reuri valid` is strict.** A URI containing raw `é` will parse fine
   in `reuri get` but `reuri valid` returns 0. Use `reuri normalize`
   first if you need a strictly-valid form.
7. **For path segments after the first in a rootless path, use
   `reuri encode path2`**, not `path` — `path` encodes `:` which is
   fine elsewhere but unnecessary mid-path.
8. **To put a literal `/` inside a single path segment, pass it as one
   arg to `reuri::path join`** — it'll become `%2F`. e.g.
   `reuri::path join / files a/b/c info` → `/files/a%2Fb%2Fc/info`.
9. **`reuri get $u host` is also a list when hosttype is `local`.** Use
   `reuri::path join {*}[reuri get $u host]` to flatten to a bare path
   string like `/tmp/sock`.
10. **0.15+: `reuri set u part ""` sets a present-empty part.** Use
   `reuri unset u part` to remove it entirely. See the migration
   section above.
11. **0.15+: `reuri get $u path <default>` raises.** Path is always
    present; drop the default and treat `""` as the "no significant
    path" case.
12. RFC 3986's encoding rules are subtle and easy to misinterpret. This
   package strives for perfect compliance with the letter of the spec,
   as specified in the grammar (modulo the unfortunate treatment of `+`
   in HTTP URL query sections forced by universal practice but which was
   never in any RFC). Many other URI implementations are not so careful
   and reject valid constructions. When integrating with a buggy URI
   implementation, workarounds like building a path as a string with
   segments encoded using `reuri encode awssig $part` may be required,
   but should be avoided if possible.
