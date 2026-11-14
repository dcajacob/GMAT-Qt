Returned ephemeris private frame ownership — implementation, 2026-11-05
========================================================================

Detached function results now retain private coordinate systems independently of
the callee's object store. Output, incoming-data and internal-frame roles share
one identity-preserving snapshot. The snapshot includes owned axes, referenced
coordinate systems and calculated points. Native clones are rebound and
initialized in their retained context before the function store is restored.

Caller input/output and true-global identity behavior remains: exact formal
resource identities determine caller bindings, with whole-resource outputs
preferred. Frame axes use their actual typed reference accessors, including
formal/caller aliases. Repeated references share one snapshot; circular private
frame dependencies and unavailable private bindings report errors. An unrelated
equal-name caller object is never selected as a lifetime substitute.

CoordinateBase copies intentionally reset origin, J2000-body and SolarSystem
pointers, so merely owning a CoordinateSystem Clone is insufficient. The new
builder restores that context and the cloned axes' point/constraint-frame
references. CalculatedPoint now exposes named body references through the
existing GetRefObject virtual slot; indexed access remains. Rebinding uses the
existing field name before translating a caller alias so the body-list entry is
replaced rather than appended under a different name.

EphemerisFile copies share the private graph lifetime, without sharing a writer.
Trial-entry snapshots retain the accepted writer's original graph while trial
configuration references another graph. Detached copies retain both applicable
owners. Assignment retains old and incoming owners until all references have
been copied, including a partially failed configuration copy. Frames retire
before newly discovered dependencies; inherited ownership stays live throughout
that retirement. A new shared member changes EphemerisFile layout and requires
rebuilding its native clients/subclasses; no GmatBase field or virtual slot was
added.

Scope still open
---------------

This is the frame/point portion of returned dependency ownership. Direct private
spacecraft and acceleration-model references without a caller counterpart still
need transfer. Spacecraft hardware/attitude/process-noise/estimated-parameter
references, ODE force/transient graphs, true-global roots pointing to locals,
other resources and full function scoping remain open. Other private resource
types encountered inside a retained frame require a caller input/output binding
and fail clearly when it is absent. This does not establish universal Clone,
third-party axis support, numerical correctness or full function I/O coverage.

Compilation and campaign status
-------------------------------

The private integrated build passes 148 steps / 162.10 seconds (v1), then 67
steps / 4.57 seconds (v2) after alias and ownership-classification refinements.
Core, Qt, console, native plugins and Python bindings compile/link. Existing
OrbitRenderer QImage::mirrored deprecation warnings remain. Source matches the
v2 input hashes. No tests, GUI launches, mission/function/model execution,
numerical comparisons, acceptance readers or seals ran.

The engineering tally stays 1092/1158 (94.30%), with 66 other original rows.
Recorded source-versioned verification remains 1053/1158 (90.9%); all 105 open
acceptance rows and original assertions, fixtures, tolerances, texts,
fingerprints, denominator, ledger and gates remain unchanged. Full implementation
continues before verification.

Artifacts
---------

``build/nasa-coverage/coverage95-20261105-implementation-function-ephemeris-frame-ownership``
contains six exact preimages, source hashes before/at each build/final, a focused
patch, source notes, both build logs/receipts and a summary. The private runtime
was rebuilt; no installed/shared runtime, package, commit or push was updated.
