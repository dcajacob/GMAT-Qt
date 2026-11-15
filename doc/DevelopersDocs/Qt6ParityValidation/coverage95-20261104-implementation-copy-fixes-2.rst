Implementation first: parser, stop and imported-resource copies
==============================================================

This increment repairs seven additional production CPP files. All seven compile
cleanly using the configured project's flags, into private object files. New
tests and runtime/GUI qualification remain deferred under the user's direction.
The previous seven-file increment is unchanged. No original assertion, numeric
tolerance, requirement fingerprint, denominator or qualification gate changes.
Verified coverage remains 1053/1158, with 105 open and 48 needed for 1101; the
59 original holds and historical 1816/2367 coverage are unchanged.

MathParser now uses ordinary memberwise copy and assignment. Previously a copy
only rebuilt the function lists: its object-map pointer and operator characters
were uninitialized, and its operator strings were empty. Assignment appended
duplicate built-in functions while resetting the function count inconsistently.
The parser owns no raw resource; its borrowed object-map pointer, equations,
operator tokens and managed function lists can all be copied directly.

StopCondition now copies its starting value and right-hand cyclic flag. A copied
condition retains a borrowed interpolator or clones an internal one, following
the existing destructor's ownership policy. Assignment clones the replacement
before releasing an owned internal interpolator, preserves externally supplied
ones, and installs null or borrowed replacements explicitly. Previously it could
delete an externally owned object, leave a dangling pointer, or leave a copy's
interpolator pointer uninitialized. BranchCommand's copy constructor also now
initializes its nested-serialization depth from the source.

PolyhedronBody copies its shape filename and every managed geometry cache,
including normals, incenters, edges and face-attachment maps. Previously it
copied only vertices, faces and the loaded flag. A loaded copy consequently
skipped file loading while lacking the derived geometry needed by gravity
calculations. No geometry calculation or numeric constant changes.

Both TcopsVHFData and TcopsVHFAscii assignment operators had reversed
self-assignment guards. They now assign different objects and leave self
assignment alone. The common reader copies the origin, coordinate-system suffix
and time system; these were also absent from its copy constructor. DataReader
assignment uses its existing ClearData operation so old data-loaded flags cannot
survive when the destination is reset for a fresh read.

The exact increment is retained under
``build/nasa-coverage/coverage95-20261104-implementation-copy-fixes-2`` in
``preimages``, ``preimages-sha256.json``, ``after-sha256.json`` and
``incremental-core.patch``. The patch SHA256 is
``c18bf2e371c9cef5b9f1b5745beca4467d341b1969d8f025bd28340381478fd7``.
It passes a reverse-application check against the live sources and adds no
trailing whitespace. ``compile-results-v2.json`` records all seven clean
compilations; the initial five-file result and patch remain retained. Source
hashes still match the compiled inputs. No shared runtime binary, public branch
or preview package is changed, and no new test or evidence seal was created.

After implementation, backfill focused parser-copy/operator/function-map tests;
internal and external interpolator copy/assignment/lifetime tests; nested branch
serialization; loaded and unloaded polyhedron copies with independent gravity
checks; and TCOPS reader metadata, assignment and stale-loaded-state checks.
Reuse existing affected registrations and original assertions when possible,
including unattended and initially minimized/resizable visible GUI routes.
Compilation does not establish linked runtime behavior or whole-row acceptance.

Further source inspection found shallow ownership in TdmReadWriter's parser and
error-handler copies, a leaked reader in TdmObType assignment, and a broken
Finalize/Initialize lifecycle. Those files are unchanged in this increment and
remain concrete implementation work. Cursor/document ownership must be handled
explicitly; replacing an active copied stream with an empty reader must not be
presented as preserving its cursor. The NarrowAtLast scientific failure, original
scope conflicts and all other remaining requirements stay open. The overall
95 percent goal remains active/incomplete.
