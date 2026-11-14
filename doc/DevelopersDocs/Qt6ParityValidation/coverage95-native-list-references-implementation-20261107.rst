Native list replacement and MATLAB reference binding
===================================================

GmatBase's existing SetField(Integer, StringArray) virtual method previously printed a malformed diagnostic and returned without setting data. The MATLAB object importer also restricted every list to its current size, changed reference names without binding the corresponding native objects, and could reselect a public same-name resource in place of a private binding. This batch implements the native typed route and concrete replacement providers, and adds staged actual-reference plans for properties with native type metadata.

Native typed field updates
--------------------------

GmatBase SetField now checks the declared property type, delegates Generic string-array values to the existing Generic setter, and uses indexed setters for supported fixed-size string lists. Identical lists are no-ops. It verifies the resulting list matches the requested values and propagates rejected setters. Size changes require a complete native provider. Changed object-reference lists also require such a provider: merely rewriting names can leave obsolete owned objects, including spacecraft hardware, attached. The default method reports that missing implementation instead of silently succeeding. The existing label overload still dispatches through the virtual integer-ID method.

SpacePoint overrides the existing virtual entry point for orbit, attitude/planetary, clock and frame kernel lists. It copies and validates the complete candidate list with the native file/type validator before swapping the descriptor values and invalidating readiness. Typed filenames remain literal data. The legacy braced script setter parses native quote syntax first, then dispatches to the same provider; invalid later entries no longer clear/publish an earlier portion of that list. CelestialBody's override preserves its native ck/pck distinction through SpacePoint, invalidates ephemeris dependencies for successful kernel edits and supplies complete DSN directory replacement with the native Earth/path/duplicate checks. Empty lists clear configuration. This does not add global CSPICE pool unload/precedence transactions or reload cached DSN media data; those original scientific/provider obligations remain open.

Formation member definitions
----------------------------

Formation overrides the typed field method for Add, validates all candidate member names and duplicates, and prepares the native empty six-element placeholder before changing a definition. Unchanged name lists preserve existing bindings and authoritative state. Changed names clear the previous member bindings/dimension/count and mark parameters changed, matching native Clear semantics until actual members are rebound. A previous context lease remains alive through that change. Formation also reports Add as a spacecraft-reference property and retains the indexed metadata overload.

The MATLAB bridge binds all resolved members and invokes the existing native BuildState only when the names or bindings changed and the list is nonempty. Native ordering, member-state size and epoch consistency checks remain in that builder; no packed-state or scientific formula is replaced. This is not an atomic multi-object transaction. A later rejected binding/domain value can leave earlier native assignments applied.

Actual reference plans
----------------------

Known non-owned object/reference-array properties use the native property object type. For unchanged names the plan first preserves the actual current reference, including private same-name bodies/frames/members that differ from a public map entry. Changed or unbound names resolve through the command's active FindObject scope, with actual type/lifetime checks before assignment. Objects selected from that map are checked again against the map before native setters run. Existing bindings are not replaced merely because a public object has the same label.

Assignments retain the previous target context and captured reference contexts through the native setter interval. Unknown type metadata and owned/factory selectors retain native string semantics; no reference type is guessed. An unchanged null reference is preserved, while clearing a live known reference requires its provider's explicit clear operation. Native reference setters are called after names are accepted, and already-bound actual references avoid unnecessary replacement or hardware cloning.

Lists now reach their native typed setter instead of the importer applying every indexed edit itself. Concrete providers can grow, shrink or clear their lists. Other native list families retain their available sizes/operations and fail explicitly when a required complete provider is absent. List/provider/domain validation can occur during native assignment after the typed result read; this does not promise an all-output rollback. Captured leases protect private dependencies during assignment, not a universal ownership guarantee for public referenced roots after new links cross function scopes. That lifetime/component/shared-global work remains open.

Compilation and retained scope
-------------------------------

Configured compilation v1: 616/616 steps, 173.31 seconds, return 0.
Configured compilation v2: 10/10 steps, 5.78 seconds, return 0.
Conditional syntax without SPICE, src/base/foundation/GmatBase.cpp: 1.19 seconds, return 0.
Conditional syntax without SPICE, src/base/foundation/SpacePoint.cpp: 0.61 seconds, return 0.
Conditional syntax without SPICE, src/base/solarsys/CelestialBody.cpp: 0.71 seconds, return 0.
Conditional syntax without SPICE, plugins/FormationPlugin/src/base/formation/Formation.cpp: 0.55 seconds, return 0.
Conditional syntax without SPICE, plugins/MatlabInterfacePlugin/src/base/command/CallMatlabFunction.cpp: 0.95 seconds, return 0.

Conditional syntax checks use the configured Linux compiler and omit __USE_SPICE__. They do not link or qualify a complete non-SPICE application or other platform/runtime behavior. Configured v1 reports an existing QImage::mirrored deprecation warning in OrbitRenderer.cpp, a file outside this batch.

Nine source/help paths have preimages before first edits, before/build/final hashes, indexed deltas, source map, logs and authoritative terminal compiler results. Final sources match configured v2 and the conditional snapshot. All compiler sessions are terminal; added-line whitespace is clean and help XML parses. Existing instance layouts, public signatures and virtual slots are unchanged; concrete classes override existing methods and matching configured consumers rebuilt. External binary/plugin and platform/runtime compatibility remain unqualified.

No runtime/GUI/mission/numerical tests, acceptance audits/readers/seals, evidence admission, assertions/fixtures/tolerances/denominator changes, packages, installed/shared runtime changes, commits, pushes or external messages were performed. Full original requirements, including remaining list/reference/component/ownership families, FRR-36, function/resource/command and scientific clauses, remain open. No original all/any parent earns partial credit.

Tracking-v77 remains 1093/1158 (94.39%), with 65 other rows. The recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full goal active and incomplete.
