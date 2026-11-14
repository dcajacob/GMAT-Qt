Typed native MATLAB object results
==================================

Native whole-object output previously requested the MATLAB display of a scalar structure, copied an 8192-byte command-window capture and parsed colon-delimited lines. Large structures, truncated vectors/matrices, display precision, multiline strings and MATLAB display formatting could therefore change the imported data. Known-field setter errors were also swallowed as unknown-field warnings. The normal result path now consumes typed engine values and stages native property assignments.

Owned engine snapshot
---------------------

MatlabValue is a separate owned value type. MatlabInterface GetValue owns the engGetVariable mxArray until conversion completes, and no child mxArray pointer escapes that ownership. Native structure field order, cell order and two-dimensional shapes are preserved. Dense real numeric classes, logicals and character arrays are copied into native storage. Signed/unsigned 64-bit integers that cannot be represented exactly as native Real remain unsupported rather than silently rounding. Dimension/count narrowing checks precede allocation of the native representation. Sparse, complex, higher-dimensional, non-scalar structures and unsupported MATLAB classes retain an unsupported representation and reject when a writable destination requires them.

Character conversion shares GetString's untruncated UTF-16-to-UTF-8 decoder, preserves embedded NUL/control characters and rejects incomplete or invalid UTF-16. The existing public string getter retains its shape acceptance; typed property strings require character vectors. A character vector used as a string-list value is one literal element; braces and quotes in that element are data. Actual cell vectors supply separate string elements. There is no new native literal unescaping rule.

In the Linux Qt engine configuration the complete snapshot runs on the existing library-owner worker. GMAT property lookup and assignment remain on the caller after the owned result is returned. The engine transport and bounded reconnect design remain intact. The command-window output buffer is not used by the normal whole-object readback, and the old protected UpdateObject helper remains available to legacy native callers.

Native property staging
-----------------------

ReadObjectOutput appends typed assignments to the same invocation-wide staging list as Parameter outputs. It captures the whole scalar structure before native setters, publishes the existing workspace bindings and clears reserved result slots before applying GMAT assignments. Return positions remain independent when destinations repeat. Real/time, signed/unsigned integer, Boolean, character/enum/file/equation/color/object-name, On/Off, vector, matrix, precise-time, Boolean-array, indexed integer/string/reference lists and the six existing Generic subtypes use their native setters.

Integer values validate finite integral ranges before casting. Boolean numeric values must be zero or one. Vectors/matrices and indexed integer/string/reference lists retain configured native sizes because those base indexed interfaces do not supply a universal replacement/resizing contract. Boolean arrays and Generic values use existing whole-value setters. Existing Generic subtype selection is retained. Legacy character encodings of numeric/Boolean vectors use the native parsers after bracket validation; typed numeric vectors/matrices do not round-trip through text. Precise MJD character values validate signed decimal syntax and day range before the native fractional parser, avoiding its permissive atoi conversion and incorrect negative fractional interpretation; numeric MJD values validate native day capacity before construction.

Only unknown fields are caught as unknown fields. Unknown/read-only fields warn and skip; known malformed writable fields propagate errors instead of silently disappearing. Field values/dimensions are read before any assignment, while native domain validation remains in the setters. This is not an atomic multi-object transaction: a rejected native setter can leave earlier assignments applied.

Nested owned components and identity
------------------------------------

A nested scalar structure requires an actual native owned-object property. Resolution uses the property's object type and reference getter and verifies the returned pointer occurs in the owner's actual owned-object list. It does not select an unrelated object through a matching public label. Spacecraft GetPropertyObjectType now reports ATTITUDE as Gmat::ATTITUDE, matching its existing reference getter and owned-object metadata. Missing native property metadata or unbound/non-owned references reject explicitly.

Root lifetime tokens guard assignment after engine/workspace operations. Nested guards also check the child's token and current actual owned binding; retaining an old component allocation cannot make it a current owned component. Parameter identity/type/read-only metadata is checked again before each typed setter. Arbitrary component replacement, reference-map rebinding, external callback atomicity and full private object graph ownership remain outside this completed batch.

Compilation and retained scope
------------------------------

Configured compilation v1: 11/11 steps, 4.0 seconds, return 0.
Configured compilation v2: 72/72 steps, 12.03 seconds, return 0.
Configured compilation v3: 6/6 steps, 3.19 seconds, return 0.
Legacy Mac conditional syntax: 0.54 seconds, return 0, using the configured Linux compiler/R2026b SDK.

The conditional compilation does not qualify a macOS toolchain or runtime. Configured builds retain the existing wstring_convert deprecation warning. All six final source/help paths match configured v3 and conditional source snapshots. Five preimages precede v1; the Spacecraft preimage precedes its first edit/v2. Before/build/final hashes, indexed deltas, source map, logs and authoritative terminal compiler results remain in the stage. Added-line whitespace is clean and help XML parses. All compiler sessions are terminal.

The new value type and helpers are nonvirtual; existing instance layouts and virtual slots are unchanged. Matching configured consumers rebuilt. External prebuilt binary compatibility and Windows/macOS toolchain/runtime behavior remain unqualified.

Original main-script and function-formal restrictions remain unchanged. Whole-object input still uses native MATLAB_STRUCT serialization, and complete literal/property transfer, reference/component rebinding, list resizing, unsupported classes/property families, FRR-36 definition and the other original function/resource/scientific clauses remain open. No original all/any requirement earns partial credit.

No runtime/GUI/mission/numerical tests, acceptance audits/readers/seals, evidence admission, assertions/fixtures/tolerances/denominator changes, packages, installed/shared runtime changes, commits, pushes or external messages were performed. Tracking-v75 remains 1093/1158 (94.39%), with 65 other rows. The recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full goal active and incomplete.
