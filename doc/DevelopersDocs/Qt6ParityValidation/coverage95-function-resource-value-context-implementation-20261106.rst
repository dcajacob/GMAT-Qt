Function resource value context — implementation, 2026-11-06
============================================================

Supported whole-object function results now retain a dependency context beyond
the function's local store. Named output preparation covers CoordinateSystem,
Barycenter/LibrationPoint, Spacecraft and ODEModel roots, alongside the existing
EphemerisFile path. Unnamed results use the same dispatcher. Native Clone/Copy
hooks reconnect copied references to the destination and its private graph.
This is concrete progress on entire-object I/O; the broad rows remain open.

Lifetime and copying
--------------------

The new FunctionValueContext interface describes private dependencies and actual
retained references. A mutex-protected GmatBase side registry holds each live
root's shared lease. Base copy operations carry it, and base destruction removes
the entry after derived cleanup. Retired contexts release outside the mutex
because deleting their resources can reenter the registry. Registry storage is
process-lifetime, while entries follow resource lifetimes. No existing native
resource fields or virtual slots are added.

The graph owns private dependency clones and never owns its public root. Root
backreferences use the destination identity. Known owned components register
before following cross-links; private force/hardware/axis references can discover
their native parent. Axes now register their owned identity. Known source,
private, copied or caller peer Clone results fail without adopting them.

Five native Clone/Copy families bind only after the derived object is complete.
Clone holds a unique owner while binding can fail; Copy retains the old
destination context through native replacement and checks incompatible/self
copies. Dependency Clone callbacks are suppressed during the enclosing graph
construction. Successfully rebound private clones clear inherited root leases;
successful public-root copies discard temporary predecessor contexts.

Initialization
---------------

ObjectInitializer preserves actual retained bindings before caller-name lookup.
Retained reference identities include borrowed caller/global resources, kept
separate from owned private resources. A unique private reference can be bound;
ambiguous or failed binding is fatal. Private objects are not added as general
variables in the caller's script namespace.

CoordinateSystem initialization temporarily supplies its axes with the frame's
lookup context and preserves already-bound matching origin/J2000 objects.
Spacecraft association retains existing owned hardware; new requested names
still use native association. Retained spacecraft use the existing private-copy
initialization path, avoiding ConfigManager replacement of their bound hardware.
The existing dependency initialization phases remain.

Remaining scope
----------------

A caller-mapped resource can have its owned component addresses replaced when
another output is copied. That path now fails explicitly instead of borrowing
the source-owned component. General multi-output commit/component rebinding is
still required. Other resource families, global roots with private dependencies,
general input isolation, recursive/native/provider contexts and moving-frame/PSM
initialization also remain. Direct C++ copy-constructor/operator= paths outside
the implemented Clone/Copy hooks have not received universal graph rebinding.
Context SolarSystem and external references retain current Sandbox lifetimes;
cross-Sandbox/external value lifetime is unqualified.

E452/E460 remain entire-object I/O, and E466 still requires unchanged non-global
inputs unless returned. E450/E491 and resource Clone parents are not narrowed or
credited by this increment. Engineering tracking stays 1093/1158 (94.39%), with
65 other original rows. Verified recorded coverage stays 1053/1158 (90.9%); all
105 acceptance rows remain open. The full implementation goal remains active.

Compilation and artifacts
--------------------------

The private integrated build passes 747 steps / 222.33 s (v1), 68 / 7.35 s (v2),
and 68 / 4.96 s (v3), including core, Qt, console, plugins and Python bindings.
V1 has the existing OrbitRenderer QImage::mirrored deprecation; v2/v3 have no
warnings. Final sources match v3. Sessions 9873, 17108 and 70192 are closed.

Eleven source paths (nine original preimages, later ObjectInitializer preimage,
and one new header), before/at-build/final hashes, focused patch, source notes,
logs/receipts and summary are retained under
``build/nasa-coverage/coverage95-20261106-implementation-function-resource-value-context``.
No existing native layout/vtable changed this stage; previous layout changes,
external binary providers and Windows/macOS qualification remain separate.

No tests, GUI, mission/function/model/solver execution, numerical comparisons,
acceptance readers or seals ran. Existing test/probe clients were linked only.
Original assertions, fixtures, tolerances, requirement texts/fingerprints,
denominator, ledger and qualification gates are unchanged. No installed/shared
runtime, package, commit or push changed. Complete implementation continues
before verification.
