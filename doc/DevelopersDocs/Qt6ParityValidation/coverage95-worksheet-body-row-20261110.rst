Body-relative readonly orbit parameter leaves — 2026-11-10
========================================================

The new exact-row admission covers FRR!E519 (magnitude of position vector,
read only) and FRR!E520 (orbit semi-latus rectum, read only). The source-versioned
union is 1,080/1,158 (93.3%): 78 original rows remain open, and 21 additional rows
are needed to reach the unchanged 1,101-row target. This is not full-current-build,
scientific, platform or full NASA qualification.

Scope and source argument
-------------------------

The original FRR!E501 body-relative contract and FRR-16 default/user-body context
are retained. All eleven defaults are represented, with native Luna corresponding
to worksheet Moon, as are the actual Moon, Asteroid, Comet and Planet user-body
factory types. FRR!E525 begins a separate coordinate-system contract. The broader
FRR!E501/E502/E525/E526 parents receive no new credit.

Existing explicit Spacecraft OrbitParameterAccess=Worksheet provides the readonly
capability. Native remains the default writable compatibility mode. The original
requirement says the system shall allow the operations and does not prescribe a
default mode; this admission does not turn Native setter failures into passes or
claim a global ban on Native writes. No product code changes were needed here.

Parameter::IsSettable uses the actual spacecraft owner policy without a body-name
condition. OrbitReal and OrbitData reject Worksheet writes before mutation;
Assignment, ParameterWrapper, typed function outputs and destination selectors
propagate the policy. RMAG uses the body-relative position norm. Semi-latus rectum
uses the squared angular momentum divided by the actual body's gravitational
parameter. The generic origin interfaces and the covered Earth/non-Earth branch
support the valid-body leaf contract; the proof does not depend on a body-name
allowlist. Physical ephemeris/provider accuracy, arbitrary frames, invalid-state
and full nested-function/clone families remain separate qualification scopes.

Evidence without a broad rerun
------------------------------

The unchanged Earth witnesses from the November 8 review are reused at their
recorded source versions: two properties in two GUI profiles. A new permanent
worker fills fourteen missing origins, two properties and both profiles: 56 new
passing scenarios. It retains actual resource-editor policy Apply/Cancel,
Undo/Redo, Unicode save/reopen, configured and fresh selector rejection, typed
assignment/function-output rejection with recovery, report reads and viewers.

All 360 raw scalar results from the 60 source-versioned witnesses were independently
recomputed from the literal Cartesian states and synthetic mu=400000. Admission
enforces the original 1e-9 km bound; maximum error is 6.366462912410498e-12 km.
The new worker's predeclared execution bound was 1e-5 km. No original bound was
changed. Synthetic mu establishes arithmetic/routing, not physical body accuracy.
The original Earth worker still contains its Native setter compatibility checks.

CMake now registers 28 body/property cases and includes the worker in check-qt.
Generated compiler flags match the executed worker. One registered CTest command
passed using that binary and a private startup; the 56-case matrix was not rerun.
The initial configure and registered launch were blocked by sandbox namespace and
X11 restrictions before assertions; those receipts remain alongside successful
identical-scope executions outside those restrictions. No full regression was run.
The body scenarios actually loaded the previously verified private DE-boundary
backend, whose source/binary identities are retained rather than substituting an
installed runtime or claiming a fresh full build.

Admission and preservation
--------------------------

Component evidence:
build/nasa-coverage/coverage95-20261110-worksheet-body-closure-v1.
Admission and sealed prefix:
build/nasa-coverage/coverage95-20261110-worksheet-body-row-v1.
The frozen reader audits exact row text/fingerprints, original body/parent context,
raw values, policy persistence, rejections, profile traces and source/binary
provenance. Eleven damaged credit/guard/numerical/policy controls are rejected
offline. A frozen-reader replay reproduces the exact ledger without native runs.

The 1,334 original preimages remain in the source capsule. Current authorized
inputs retain only the two earlier approved observer-lifetime/log-isolation
amendments; this component changes none. Every prior admission, original failure
receipt and the 59 formal hold identities/statuses remain. The other 1,156 row
records and 137 tracked closure records are unchanged. Historical operations
remain 1,816/2,367. The frozen prior 1,078-row prefix is not edited in place.

At the user's request, FRR!E80 and its dependent FRR!E77 remain deferred. Existing
impulsive burns already work; neither deferred row receives credit or further
implementation in this stage. The saved decision is under
coverage95-20261110-thruster-ownership-decision-v1/decision-v1.json.

The prior full-regression result belongs to its frozen earlier source. This
admission adds the bounded body evidence to that source-versioned union; it does
not rerun or relabel the full regression as passing on the latest whole tree.
