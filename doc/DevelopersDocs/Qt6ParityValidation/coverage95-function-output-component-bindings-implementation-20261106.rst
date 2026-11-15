Function output component bindings — implementation, 2026-11-06
===============================================================

Covered whole-resource function results now reconnect to final caller components
after resource values have been copied. Previously a root mapping could leave a
result pointing to components replaced by another output Copy. Captured private
graphs now remain independent until the caller destinations are ready. This
improves the supported return paths; general function I/O remains incomplete.

Output phases and identities
----------------------------

Each captured invocation has a shared epoch marker. Origin identities propagate
through retained context Clone/Copy within that invocation. An older retained
context cannot match a later reused frame address merely because it is equal.
Input aliases and explicit whole-resource output destinations are captured, with
output precedence. True-global shared identity policy remains.

Ordinary return and internal PopFromStack first copy non-ephemeris values, then
reconnect covered retained roots against final caller resource/component
identities, then copy detached ephemeris configuration last. Rebinding uses an
independent reader and changes references without replacing the destination's
already-copied owned components. Static forces, axes, spacecraft-owned native
components and their embedded FOV/error/noise/estimated-model children have
correspondence paths. Shape, type and force-body mismatches fail explicitly.

The first ephemeris assignment uses a temporary rebound copy. The original
captured ephemeris graph stays private for PopFromStack; restoration can replace
components used by that first assignment. Borrowed live global ephemerides stay
identity operations. Graph binding does not initialize ephemeris output writers;
native Copy retains its accepted/trial writer policy and live Sandbox burn
context. The original public three-argument PopFromStack entry remains, with an
internal overload carrying binding metadata.

Value and scope preservation
----------------------------

Real, Array/Rmatrix and String outputs are evaluated into value proxies before
any caller assignment. Typed wrappers preserve values through aliased returns.
After caller-frame restoration, typed target wrappers apply them to Variables,
Arrays, Array elements and parameter/property setters. This avoids copying a
Variable proxy into an incompatible system Parameter class. Caller vector-shape
policy is carried through restoration. Unnamed retained object results reconnect
after caller-frame handling.

GmatBase existing-object assignment now preserves the destination's IsLocal
scope flag, as its IsGlobal policy already does. Returning a local value must
not relabel the owning scope of a caller/global destination. Fresh construction
still copies source flags; explicit scope commands retain their responsibilities.
ObjectInitializer also recognizes retained ephemeris spacecraft/model references.

Remaining scope
----------------

Input/caller scene capture before formal refresh and general recursive frame
graphs still need work. PushToStack remains root-only for objects without a
prior retained context. Components borrowed from an external caller parent
outside the captured private graph are not universally protected against earlier
formal refresh or caller model replacement. This increment does not close that
lifetime case.

Other resource families, global roots with private dependencies, complete input
isolation/scoping, other primitive/complex-selector cases, direct native copy
paths, post-return owner replacement, cross-Sandbox lifetimes and moving-frame/
PSM scientific domains remain. No universal output rollback guarantee is added.
Entire-object E452/E460 and input-preservation E466 remain unchanged. No broad
parent engineering or acceptance row is awarded.

Compilation and retained work
-----------------------------

The private integrated build passes 91 steps / 143.27 s (v1), 66 / 5.48 s (v2),
66 / 5.49 s (v3) and 66 / 5.44 s (v4), with no warnings. Core, Qt, console,
plugins and Python bindings compile/link. Final sources match v4. Sessions 54463,
24367, 14930 and 7983 are closed. Five exact preimages, hashes, focused patch,
notes, logs/receipts and summary are under
``build/nasa-coverage/coverage95-20261106-implementation-function-output-component-bindings``.

No existing native fields or virtual slots are added. The new internal context
interface is extended; external binary and cross-platform behavior is unqualified.
An edit-script guard assertion stopped before the scope assignment was modified;
its selector was corrected. All native builds passed.

No tests, GUI, mission/function/model/solver execution, numerical comparisons,
acceptance readers or seals ran. Original assertions, fixtures, tolerances,
requirement texts/fingerprints, denominator, ledger and gates are unchanged.
Tracking stays 1093/1158 (94.39%), with 65 other original rows; recorded verified
coverage stays 1053/1158 (90.9%) and all 105 acceptance rows remain open. No
installed/shared runtime, package, commit or push changed. Continue the complete
implementation pass before verification.
