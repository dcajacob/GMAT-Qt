Committed integration-step publication
======================================

Original coverage remains **1,009/1,158 (87.1%)**;92 more complete requirements
are needed for95%. Historical coverage stays1,816/2,367 (76.7%). This layer earns
**zero new NASA credit**. FRR!E329/E330/E633/E634 remain unqualified until both
plot consumers and their inherited clauses pass. The95-percent goal stays active.

Implemented behavior
----------------------

Propagate captures actual native primitive endpoints for each candidate. It
attaches immutable endpoint metadata to its existing initial, normal, single-step
and final publications only when an explicitly registered subscriber requests it.
Ordinary packet values/counts and the normal ephemeris-only single-step filter
are preserved. An opted-in consumer also receives single-step endpoint payloads.
Without a consumer, the old virtual TakeAStep/Publish calls execute without
snapshot allocations or state copies.

Crossed-stop candidates and interpolation/refinement probes do not enter the
published trajectory. An ambient diagnostic journal retains native attempts;
the separate publication vector represents only the committed trajectory.
Metadata is synchronous, pointer/count-bound, frozen against caller mutation,
and cleared/restored across nested legacy calls and exceptions. Individual,
bulk and direct publisher resets remove consumer registrations; subscriber
destruction removes its registration without creating a Publisher singleton.

Snapshots bind actual runtime spacecraft through PSM Cartesian component maps.
Formation members use the implemented runtime-member array in its native order,
not a configuration-name lookup. Each force model supplies its own base epoch
and coordinate origin. ODEModel's new nonvirtual snapshot helper translates all
Cartesian blocks to the internal J2000-body MJ2000Eq context without updating
live spacecraft. Its synthetic oracle uses two distinct body rates, two Cartesian
blocks and untouched extra state elements, so wrong epoch/sign/block handling
cannot cancel out of the expected result.

Demonstrated core correction
------------------------------

A synchronized secondary RK propagator with MinStep=MaxStep3 could not finish a
10-second interval: it repeatedly clamped the final1-second remainder to3 and
failed after51 attempts. RungeKutta already temporarily lowered its accuracy
floor for short final-step intervals. The correction applies that same bounded
remainder policy to every fixed interval, including synchronized propagation.
The configured MinStep is restored on exit. Estimator equations, tolerance and
StopIfAccuracyIsViolated remain unchanged. Forward/backward cases and a test-only
injected excessive error verify exact remainder completion, strict rejection,
warning-policy acceptance and restoration after success/exception.

Nine preserved preimage paths include Publisher.cpp/.hpp, Subscriber.cpp,
ODEModel.cpp/.hpp, Propagate.cpp/.hpp, RungeKutta.cpp and Qt CMake registration.
Seven are engine implementation/API files plus the RK implementation and build
registration. Existing class layouts and virtual tables are unchanged by this
layer; this is not blanket proprietary-plugin ABI or core technical approval.
Exact before/after source and per-file patches are retained for scrutiny.

Verification and every failed attempt
---------------------------------------

All eight new cases pass visibly: forward/backward command boundaries, single
steps, synchronized forward/backward providers with different primitive sizes,
ordered Formation members, and nonlinear ABM stops at37.123 and37.25seconds.
All eight ordinary packet sequences match their legacy controls exactly.
Independent rereading verifies130 saved Cartesian states /780 scalar values;
maximum errors from the serialized epochs are2.073e-6km and2.235e-9km/s.
Each ABM raw journal has43 native advances but only38 published advances.
Published entries match raw endpoints; the overshoot/ring probes are excluded.
The executed source also compares each published state to its raw journal state.

The visible invocation records50xcb exposure events, initially Iconic state,
resizable/non-fullscreen desktop operation and zero screenshots. Actual process
maps bind25 loaded project images. This proves the nested visible route, not
physical host/GPU/compositor observation or that a person watched it.

Unattended evidence is reused at its recorded source/runtime versions:

* headless-v3 completed forward/backward/single-step subcases, then failed.
* headless-v4 completed synchronized forward, then failed.
* headless-v5 passes Formation and the37.123-second ABM case.
* headless-v6 passes synchronized backward/error policy and37.25-second ABM.

The earlier parent invocations remain failed. They are not relabeled as whole
passing runs. The complete final eight-case proof is visible-v1; no fresh full
unattended/current-build matrix is claimed from this conjunction.

The three directly affected existing PublicationEpochContract,
SharedSampleContract and OriginRelativeStateContract checks pass unattended.
All three assertion sources match the20261031 reviewed-candidate catalogue.
No unrelated passing suite was replayed.

Failures retained in full:

* build-v2: new synthetic body fixture lacked its required Clone override.
* build-v9: new policy fixture used a hidden string Boolean setter overload.
* headless-v1: new fixture incorrectly treated ElapsedSecs as mission-global;
  its second command was corrected to27seconds, retaining the37-second oracle.
* headless-v2: new continuity assertion was tighter than ordinary double-MJD
  command-boundary quantization; the new test now explicitly uses a1us clock
  bound. Existing assertions were untouched.
* headless-v3: the demonstrated RK synchronized remainder defect above.
* headless-v4: the new Formation binder incorrectly called unsupported single
  GetRefObject; it now uses the actual runtime-member array with unique identity.

Independent review rejected the first evidence capsule because reused older
source bytes were absent and some checker links were implicit. Version2 preserves
those exact recovered versions and enforces their receipt, native-JUnit/log/proof
and original-assertion links. The original capsule/audit is retained as superseded
evidence; its earlier pass is not used to claim the corrected durability gates.

Replay and offline audit
--------------------------

Build only the new family::

  cmake --build build/linux-gui --target GmatQtStepPublicationTests -j4
  python3 src/qtgui/tests/RunStepPublicationAcceptance.py --visible

Omit --visible for unattended use. GMAT_STEP_PUBLICATION_CASE accepts a case
name or comma-separated selection. Normal check-qt and build-qt-acceptance
include the executable; CTest name is QtGui.StepPublication.

All evidence is under build/nasa-coverage/coverage95-20261101/ with the
step-publication- prefix. Verify retained evidence without native replay::

  python3 src/qtgui/tests/AuditStepPublicationEvidence.py --manifest build/nasa-coverage/coverage95-20261101/step-publication-evidence-manifest-v2.json --output /tmp/step-publication-audit.json

The manifest binds393 artifacts,29 exact current text archives plus four older executed source versions, nine preimages
and patches, raw case/JUnit/log receipts, the loaded-image record and successful
incremental builds. Its SHA256 is
14e4e8c16ceb9f38113f69abd8703e38c3ba7e8a35568fd0ab5816b2c240ccca.
The auditor regenerates all780 independent values and verifies the exact legacy
packets, committed sequence, journal membership, visibility and unchanged ledger.
It also regenerates780 values from the reused unattended subcases, enforces
original parent statuses, exact native log/proof/JUnit conjunctions and unchanged
assertion/catalogue hashes. Recovered intermediate source bytes must match the
old receipt hashes; no old receipt was rewritten and no native replay was needed.
It does not infer a clean build or full source-to-binary qualification.
Lossless compression saved68.98MiB of duplicate summaries; raw receipts,
failures, proofs and source versions remain. No passing PNGs/runtime copies remain.

Remaining work before any plotting credit
-------------------------------------------

Both GroundTrack and OrbitPlot need integration-step consumers and actual
per-trajectory cadence across commands, including repeated initial points,
partial final advances, backwards/single-step and multiple providers. Preserve
the existing publication-sample behavior and every original assertion, including
the enabled GroundTrack boundary case. Conjoin redraw/retention parent clauses.

Solver None/Current/All and rollback, finite burns, ephemeris propagator sampling,
real translated-origin runtime endpoint publication and all-frame consumer
semantics remain unqualified. The synthetic origin check is not a substitute
for a real non-Earth propagation. No4-row pass, full NASA qualification, full
current-build qualification or core technical approval is assigned here.
