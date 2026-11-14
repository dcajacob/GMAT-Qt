Conic Parameter owners and RADN dispatch — 2026-11-03
====================================================

Coverage remains **1,019/1,158 (88.0%)**, 139 original requirements open and
**82 more needed** for 1,101. Historical coverage remains 1,816/2,367. Original
fingerprints, assertions, denominators, whole-row conjunctions and all 59 holds
remain. This checkpoint adds no whole-requirement credit; scientific, technical,
full-current-build and full NASA qualification gates stay false.

58 types, with independent conic values and owner graphs
-------------------------------------------------------

All selected Keplerian/modified Keplerian, spherical, equinoctial/modified/
alternate equinoctial, Delaunay, angular-momentum and elementary orbital types
now bind actual Spacecraft owners in main, private and explicit Global scopes.
The 58 exact factory spellings remain in NasaFunctionConicParameterCases.json,
along with all 321 original system Parameter entries and their original metadata.
The seven six-vector types retain Array bridges for Report. Apoapsis/Periapsis
retain false reportable/plottable/settable flags: Write and numerical expressions
evaluate them, while scalar Variables carry their values to Report. Direct
Parameter Report is not claimed for these nonreportable types. Their stop-function
values are normalized position/velocity dot products, not apsis distances.

Every actual Parameter-to-owner reference, private/root isolation and Global
flag is checked synchronously inside a live callback. Actual Earth origin,
EarthMJ2000Eq frame/MJ2000Eq axes and live SolarSystem references are checked,
including the origin reference on origin-dependent Parameters. The actual Earth
Mu must equal the pinned 398600.4415. No private pointer is accessed after return.

The new oracle reuses the retained independent scalar conic geometry and nine-
representation equations, without calling GMAT conversion APIs. Root/private/
Global elliptic elements are respectively [10000,.2,30,40,50,60],
[11000,.25,40,50,60,70] and [12000,.3,50,60,70,80]. Private leaf calls add
100 km to semimajor axis and 5 degrees to true anomaly per call; Global calls add
200 km and 10 degrees. HA/RLA/DLA instead use hyperbolas with semimajor axes
-14000/-15000/-16000 and eccentricities 1.4/1.5/1.6, with analogous call changes.
The outgoing asymptote direction follows the limiting perifocal velocity basis.
Scalar right ascensions/azimuths use their signed principal ranges.

Independent full Cartesian states are assigned through actual script fields on
each leaf call. Native selected values, raw Write, Report and both epoch stores
are checked. Targets stay at 21545 while separate pulse spacecraft provide the
callback boundaries. Value bounds are 1e-11 for the selected dimensionless/rate
scalars and 1e-8 for other selected values; state bounds are 2e-7 km position and
5e-10 km/s velocity. This is nonsingular, Earth-inertial conic evidence, without
arbitrary-origin/frame/singularity/provider/propagation science qualification.

All 35 advertised writable scalars also run strict script and native idempotent
setter probes. These retain original writable flags and check the full owner
state; they do not establish general non-idempotent setter physics. Legacy unit
strings are compared with the actual unmanaged factory prototype and retained,
including placeholder and mixed-vector labels. This does not qualify those labels
as physical unit contracts. Original anomaly/read-only and other source conflicts
remain open.

RADN repair and retained failures
---------------------------------

The original RADNWrite pilot failed at its initial native setter with unknown
parameter ID35, despite the unchanged writable advertisement. RADN's getter is
RAAN+180 degrees, but its destination ID lacked setter dispatch. OrbitData::SetReal
now maps that item to the existing RAAN destination and wraps input-180 degrees
before using the existing direct/converted setter paths. No new conic conversion,
member, virtual slot or writable flag is introduced. Existing RMAG/VMAG and
detached sample-scope changes remain intact. All 245 pending core paths remain:
244 prior hashes unchanged and OrbitData.cpp is the sole existing-path increment.

RADNWrite additionally changes the selected Parameter by 17 degrees, checks its
independently rotated full Cartesian state and restores the original value/state
and owner/frame references. All 48 current main/private/Global roundtrips pass.
Other frames, wrapping/domain boundaries and broad setter physics remain open.

All failures remain exact source-versioned records, not passing negative controls:

* Two initial pilot failures: the RADN production defect and a new RLA oracle
  range error. The shipped guide requires -180 to180 degrees; the original new
  fixture incorrectly wrapped the same asymptote direction to0–360. The guide,
  failed oracle/catalog and native result remain; original NASA assertions did
  not change.
* Two unattended Build failures: the fixture attempted direct Report on the
  nonreportable Apoapsis/Periapsis types. Their native rejection is retained;
  supported scalar-variable bridges preserve their original flags.
* Eleven first-visible failures: seven vectors were incorrectly included in the
  scalar bridge condition; four runs completed the value checks but failed the
  active-window ReportViewer assumption. The correction retains the Array bridge
  and finds the unique actual ReportViewer by class and exact checked-file path,
  then activates it and compares its read-only bytes.
* Eleven targeted-visible failures at earlier editor/source/arm boundaries.
  The driver now explicitly focuses the public modal/widget window before QTest
  keyboard delivery and records failure diagnostics. Draft/file/source/history/
  prototype assertions remain strict. All eleven then pass visibly.

One initial test compile failure retains its exact source/log: CelestialBody's
integer getter hides the inherited string overload. Resolving Mu through its
existing parameter ID fixes the test. Five later builds pass. Two offline contract
audit executions retain the same schema error: nonreportable vectors were wrongly
treated as scalar bridges and newer raw fields required from older passing proofs.
The corrected auditor follows scalar/vector metadata and actual recorded fields.
Exact failed auditor sources and correction patches remain.

One launch was issued before its build handle was polled terminal. The retained
timing/image audit shows the executable finished8.338 seconds before runner start,
the new image hash matches and the emitted proofs exercise the new bridge. Both
handles are terminal; no actual build overlap or native interruption occurred and
that receipt is reused. All other native/build handles are terminal as well.

Evidence and continuation
-------------------------

**186 current GUI passes** comprise93 unattended and93 visible cases:58 read
routes and35 strict write variants in each mode. Five passing pilots are reused;
86 more unattended cases and two targeted scalar bridges complete that mode.
82 first-visible passes are reused and the11 affected cases finish in visible-v3.
The26 failed GUI cases above remain in their original invocations and capsules.

The current proofs contain558 missions,2,232 calls/callbacks,4,464 owner/frame/
origin/value observations,1,680 native idempotent probes,1,260 strict script
assignments,20,088 function Cartesian component assignments and2,790 markers.
Each Write/Report channel has6,144 expanded values. Script totals use actual
ReadShared leaf-body executions, excluding NestedShared wrapper calls. There
are25 corrupt-proof controls, including RADN changed/restored state and original
nonreportable metadata boundaries.

93 current visible process maps and1,581 exposures retain exact loaded-image
hashes. All three visible invocations retain115 maps, including failed cases.
Source/image inputs stay stable within each invocation. Windows start minimized
and remain resizable/nonfullscreen with software rendering. Screenshots, runtime
binary copies and kernel copies are zero. No native OS-input/GPU/platform claim.
File-editor keyboard Cancel/Save, Unicode reopen, Build/no-Build repeat,
source/history/prototype/caller isolation and actual report viewing remain.

The matrix preserves all566 rows,321 types and every earlier field, adding only
new witnesses/failure links. Native owner evidence covers260 types;61 Spacecraft
types remain queued,26 with older bounded contexts. Any-context counts become
286/35:50 newly joined names and eight with older weak contexts. All321 whole
obligations remain open; there is no factory or readable-role exclusion.

Authoritative v1 audit, conic matrix, remaining worklist, evidence manifest,
source identities/archive, stage receipt and seal verification are under
``build/nasa-coverage/coverage95-20261103-function-conic-parameter-bindings/``.
Three unattended and three visible invocations retain their exact sources,
proofs, raw outputs, JUnit, traces and failures. The previous time receipt and
canonical ledger remain hash-bound and unchanged. Native receipts bind237 core
paths, eight more are statically bound, and visible sources bind all245.

Replay new output directories without overwriting historical evidence::

   cmake --build build/linux-gui --target GmatQtNasaFunctionConicParameterBindingTests -j 6
   python3 src/qtgui/tests/RunFunctionConicParameterBindings.py --output build/gui-acceptance/conic-replay-headless
   python3 src/qtgui/tests/RunFunctionConicParameterBindings.py --visible --output build/gui-acceptance/conic-replay-visible
   python3 src/qtgui/tests/AuditFunctionConicParameterBindings.py --stage build/nasa-coverage/coverage95-20261103-function-conic-parameter-bindings --output /tmp/conic-evidence-audit.json
   python3 src/qtgui/tests/TrackCoreChanges.py --verify build/nasa-coverage/core-change-review/20261103-function-conic-parameter-bindings

All93 CTest registrations retain qt-gui,180-second timeout and engine lock;
both acceptance build targets depend on the driver. The wrapper defaults to the
complete selected matrix, zero screenshots and initially minimized visible runs.
Continue Planetodetic/asymptote/Brouwer, Sun/body, force/rate/covariance and
prerequisite domains. Preserve E491/E477/E406 and earlier science/source conflicts.
MATLAB is unavailable per the user:37 MATLAB holds remain without substitute or
waiver, and all59 prerequisite holds remain. The95% goal stays active/incomplete.
