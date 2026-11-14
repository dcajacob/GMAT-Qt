Bounded Linux GUI failure repairs — final handoff, 2026-10-31
============================================================

All 18 original failing registrations in this goal now have passing repair
receipts, with no waived, skipped or unexecuted case counted as resolved.
All five associated findings have bounded passing repair evidence: VNB
acceleration delivery, Yukon original-tolerance false convergence, Yukon wide
infeasibility, VF13ad report truth and NotAKnot cold-cache initialization.
The preceding Paths and two Debugger repairs are separate: together the repair
receipts cover all 21 effective failures retained in the sealed handoff.

This is failure closure against the stated bounded plan. It is not a fresh
all-green full-suite result, full current-build qualification, full core
technical approval or full NASA acceptance. Earlier valid passes retain their
original source/binary versions. Broader NASA qualification/expansion remains
deferred. The sealed earlier package still truthfully contains 1,722 passes
and 21 failures; it was preserved, not relabeled or regenerated.

Original failure disposition
----------------------------

``progress.json`` names the authoritative per-case result.json for every row.
All original result assertions and numerical tolerances remain intact. Fixture
preconditions/oracles were repaired where the original expectations contradicted
native lifecycle, legal insertion contexts or actual documented input rules.

* CommandGeometry: insert Achieve/Vary in legal Target contexts.
* TreeActivation: accept the reverted no-op draft before Build, respecting the
  sticky dirty-input contract. Required offscreen fixture passed.
* Compatibility: recreate standalone editors after successful Apply retires
  them; all later source, lifecycle and calculation checks now execute.
* Mission and MissionInsertion: initialize the native engine, declare function
  resources, and select the Else append boundary owned by the chosen If.
* Spacecraft attitude/visual clone policy and ResourceClone: compare against
  independently captured original palette colors; copying implicit colors
  requires explicit copied assignments. Exact source checks and native
  original/copy properties remain checked; clone production code unchanged.
* Inspection: capture the typed form's visible invalid-plot diagnostic as well
  as backend callback errors; retain rejection/source/pending assertions.
* BulirschControls: preserve a changed finite REAL input's spelling only when
  it exactly equals the native and serialized double. Boundary/full-precision
  behavior remains guarded. Visible checkpoints establish actual exposure.
* ScriptEditing: restore the expected capitalized mission-failure diagnostic.
* KalmanFilter: the old Unicode filename was valid. Reject an actual control
  byte instead, retaining the original rejection assertions and positive
  Unicode behavior. The positive CSV matches the independent reference exactly.
* Polyhedron: accept a reverted no-op draft before building the independent
  reference; original numerical/source assertions remain.
* GlobalScopes: typed unknown resources reject; raw syntactically valid Global
  follows native deferred execution. Explicit raw-route fixture matches its
  documented intent; typed negative, rename and malformed-input guards pass.
* NasaOptimizerRuleUnique: open the chooser in a legal objective-free context;
  verify duplicate exclusion there and backend rejection through raw script.
* EpochPlaybackRendering: remove footer subpixel text fringes from the measured
  scene mask via the fixture's font rendering. Every original pixel tolerance
  remains. Required offscreen fixture passed; native playback companion passed.
* NativeNasaVF13adResourceReportTruth: refresh only physical reporting cache
  before internal optimizer completion; original fixture and bounds unchanged.
* NativeNasaAttitudeFixedFrameOrbitVelocity: deliver the actual active forces'
  acceleration at the accepted identity/epoch/origin to object-frame axes;
  original fixture and bounds unchanged.

Verification and visibility
---------------------------

Sixteen of these 18 registrations pass on native xcb in a minimized, resizable,
non-fullscreen Xephyr window with recorded native exposure and zero automatic
screenshots. TreeActivation and EpochPlaybackRendering require offscreen Qt;
they executed and passed, but do not claim native visibility. NativeEpochPlayback
supplies separate visible playback evidence. Exposure proves that the process
rendered; it does not prove every intermediate control was readable onscreen.
``display-closeout-final.json`` verifies all 18 test-owned display processes
have exited in the host process namespace. No unrelated host application was
closed. No full-screen desktop takeover was used.

The exact source audit preserves all 1,190 original require calls, including
multiplicities, across 16 audited fixtures. The VNB case JSON and VF13ad shared
header also match their preserved originals byte-for-byte. The final audit
checks that traceability scopes, facets, expected results and anchors were not
weakened; changed prior source references resolve to exact preserved versions.
``audit_assertions.py`` and ``assertion-audit-current.json`` make this reviewable.

The final acceleration increment passes four native checks and two GUI cases
(VNB and radial), both with 26 native exposure events. VNB maximum rate error
is 2.43e-16 deg/s versus the unchanged 5e-9 bound. Generic-force, noncircular,
state/mass, epoch, axes/origin, failure/recovery, lifetime and actual repeated
Earth-to-Moon-to-Earth command handoff gates pass. See ``vnb/report.rst``.

The VF13ad original GUI plus affected Yukon optimizer checks pass visibly.
Two new Forward/Central contracts check scaled and unscaled independent
quadratic solutions and final reports. Trial/final numeric CSVs are byte-for-
byte identical before/after the reporting repair. The proprietary VF13ad
binary is unchanged. See ``vf13ad/report.rst`` and its raw receipts.

NotAKnot has independent cubic cold-copy/assignment checks in normal and
unoptimized actual-source builds; the poisoned-cache diagnostic fails before
and passes after initialization. Yukon uses the original requested feasibility
tolerance and preserves the previous valid Hessian when a new update loses
positive definiteness. Tight and wide contradictions reject, feasible recovery
passes, 12 unchanged analytic companions and four affected GUI modes pass.
The unchanged 32-case RunnerContract first failed under sandbox X11 isolation;
its proper host execution passes. Both receipts remain, and the audit explicitly
selects the host result rather than treating that failed attempt as passing.
The earlier reports and raw failing attempts remain intact.

No unrelated passing scenarios were replayed to manufacture an all-green count.
Affected shared code was retested when it changed. The final metadata-only gap
correction passes the 41-test traceability validator. There are now 1,753 CTest
registrations, ten added native support contracts; the GUI catalogue remains
1,732. Counts are not a coverage metric or a claim that every registration was
replayed on the final binaries.

Core scrutiny
-------------

``core-audit.json`` compares 93 tracked core files with this goal's starting
versions: the prior 88 plus five newly included files. Ten changed during this
goal; the other 83 retain their goal-start hashes. The prior Paths/Debugger
NLPFunctionGenerator empty-user-constraint repair is already part of this
goal's baseline and remains separately reviewable.

* NotAKnotInterpolator.cpp: initialize cold cached h/A arrays.
* Yukon.cpp: enforce original bounds and retain a valid Hessian on numerically
  invalid updates.
* Optimize.cpp and Vary.cpp: refresh reporting-only physical values before
  internal solver completion; no extra objective/mission evaluation.
* ODEModel.cpp/.hpp, ObjectReferencedAxes.cpp, Spacecraft.cpp and
  Propagate.cpp/.hpp: guarded active-force object-frame acceleration delivery,
  actual accepted identity, lifecycle and command handoff.

``core-incremental.patch`` and ``frontend-incremental.patch`` collect the exact
goal-only production changes for review; ``incremental-patches.json`` binds them.
Exact goal-start sources and incremental patches are retained under
``notaknot``, ``yukon``, ``vf13ad`` and ``vnb``, with permanent before-source
archives under ``doc/DevelopersDocs/Qt6ParityValidation/remaining-failures-20261031``.
TrackCoreChanges.py now describes this increment. All 93 files still have
technical approval=false. These runtime repairs require scientific/code review;
selected test passes are not whole-file, ABI, provider or physics approval.
The Qt frontend changes are separately listed and hashed in ``progress.json``.

Coverage gaps and retained failures outside this goal
----------------------------------------------------

``coverage-gaps-final.json`` retains all 28 selected-receipt gap categories,
updates VF13ad's historical failure to its bounded repaired disposition, and
names remaining acceleration domains. In particular:

* A prior manual 0.5x XY raster check still fails its strict full-color marker
  oracle. It remains an open subunit-scale gap; this goal did not repair it.
* Distant-origin relative-input precision and held scientific/tutorial regimes
  remain unqualified; their earlier failure evidence is retained.
* Legacy global acceleration getters/published acceleration, unsupported force
  families, implicit force/attitude coupling and general ephemeris derivatives
  remain unqualified. Unsupported/recursive queries now fail explicitly.
* Per-control assertions, all field input domains, dynamic/unnamed controls,
  full intermediate-step readability, native OS input/portals/accessibility,
  hardware GPU/compositor lifecycle, stress and Windows/macOS remain incomplete.
* Complete authored tutorial/corpus replay, current private NASA corpus,
  historical/current source conflicts and requirements retirement need separate
  qualification. The available workbook is the shipped 2012 baseline.
* Full current-build/release provenance and full regression remain deferred.
  Software X11 and selected local builds do not establish them.

No NASA ledger merge or new broad coverage credit was performed. The previously
reported cumulative mixed-version estimates remain 925/1,158 requirement scopes
(79.9%) and 1,833/2,367 historical operation scopes (77.4%); these are separate
source denominators, not percentages of GUI mechanisms or a current-build
qualification. Historical evidence maps can name prior live paths: their sealed
execution snapshots, not current bytes at those names, remain authoritative.
A two-case repair receipt can correctly report zero newly
verified NASA scopes without erasing earlier versioned evidence.

Reusing and running the suite
----------------------------

From the repository root, watch just the final repaired frame cases::

   python3 src/qtgui/tests/RunAcceptance.py --build-dir build/linux-gui \
     --visible --start-minimized --max-screenshots 0 --pause-ms 100 \
     --max-checkpoints 5000 \
     --filter '^QtGui\.NativeNasaAttitudeFixedFrameOrbit(Velocity|Radial)$' \
     --output build/gui-acceptance/frame-review-NEW-ID

The window starts minimized. Restore it when you want to watch, resize it
normally, and close it after completion. Scaling is host-aware. The output
path must be new. Remove the filter for a deliberate whole GUI-catalogue run;
omit ``--visible`` for unattended execution. A whole catalogue run will consume
substantially more time and evidence space and is not needed to repeat the
repair handoff. Run ``--help`` for pacing/selection options.

The added native support checks are reusable through CTest::

   ctest --test-dir build/linux-gui --output-on-failure \
     -R '^QtAcceptance\.(NotAKnotCold(Copy|Initialization)|YukonElasticTolerance-.*|SolverReportState(Forward|Central)|AccelerationContext(Force|Lifetime)|AccelerationHandoff)$'

For a deliberately comprehensive automated execution, build the configured
acceptance targets, run the GUI runner without a filter and run CTest support
registrations with ``-L qt-acceptance-support``. That label includes some checks
also present in the catalogue; use the registered names for a deduplicated CI
plan. Do not treat a partial filter or this receipt audit as a full execution.

Inspect retained evidence without replaying tests::

   python3 build/stabilization/20261031-remaining-failures/audit_completion.py
   python3 build/nasa-coverage/closeout-20261031/verify_handoff_evidence.py

Current identities, receipt hashes, per-run source/binary versions and exact
completion requirements are in ``current-repair-identities.json`` and
``completion-audit.json``. No new full binary snapshot was made. Passing
screenshots are disabled; only three small earlier failure-mask images remain.
Temporary native debugger objects/executables were removed after preserving
commands, sources and diagnostics. The earlier sealed handoff was verified
read-only (36,174 retained files), executing zero tests.
