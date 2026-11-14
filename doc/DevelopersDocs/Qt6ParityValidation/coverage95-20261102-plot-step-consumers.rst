95 percent campaign: explicit plot integration-step consumers
============================================================

Coverage remains **1,009/1,158 (87.1%)**. All149 original open requirement
instances remain open; **92 more** are required for95%. Historical coverage
remains1,816/2,367 (76.7%), with its separate denominator. This stage admits
**zero** NASA requirements. The full goal remains active.

Both native consumers now have a distinct, opt-in ``IntegrationStepsToSkip``
integer property, exposed in their primary Qt resource forms. Its default
``-1`` preserves publication sampling through ``DataCollectFrequency``.
``0`` collects every committed nonzero integration advance; a positive value
skips that many advances between collected points. The initial trajectory point
is retained. For example, ``IntegrationStepsToSkip = 1`` collects the initial
point and every second subsequent accepted advance, including a shortened final
advance when its ordinal is due. The property does not change integration steps,
forces, tolerances, ordinary publisher packets or the legacy collection setting.

This is an implemented candidate with bounded evidence, not a completed
all-frame plot feature or a completed four-row qualification.

Implementation and demonstrated fixes
-------------------------------------

``GroundTrack.cpp/.hpp`` and ``OrbitPlot.cpp/.hpp`` consume the existing immutable
endpoint metadata synchronously. Each spacecraft has a separate cadence cursor,
keyed by its actual runtime identity; unrelated providers do not consume its
count. Temporary label/value batches carry only selected spacecraft, preserving
slots through the existing availability handling. Consumer storage copies values;
it does not retain pointers into the publisher's endpoint vector. New runs and
clones reset sampler state. A bounded decrement avoids ``skip + 1`` overflow at
``INT_MAX``. The existing publication collection path remains available.

Two new-consumer defects were exposed and repaired without changing expected
result assertions:

* A command-initial epoch can be rounded through Real MJD while the preceding
  committed endpoint retains GmatTime precision. The first implementation
  incorrectly restarted cadence at that boundary. Initial-point identity now
  compares their common published epoch precision and still requires identical
  Cartesian state. Accepted advances retain full-precision comparisons.
* OrbitPlot did not receive the solver rollback notifications used by GroundTrack.
  The new witness observed10/30-second points instead of20/37 after rollback.
  ``SolverBranchCommand.cpp`` now sends sampler checkpoint/restore notifications
  to explicitly enabled OrbitPlot consumers. It does not add OrbitPlot to the
  XY/ground-track pen/color list or change numerical solver state transitions.

The added ``IntegrationStepSampler.hpp`` owns cadence and per-solver-scope
checkpoints. Existing renderer solver buffering remains in place. Five existing
core files, one new core header, two Qt dialogs and CMake/test tooling changed
in this stage. Exact preimages and diffs are retained. Class layouts and OrbitPlot
parameter IDs changed: external proprietary-plugin ABI compatibility is **not**
qualified. No propagation algorithm, force equation or error estimator changed
in this stage.

Verified evidence, at recorded versions
--------------------------------------

Twenty-two new cases passed unattended and visibly:

* Forward/backward command boundaries; zero-skip single stepping; ``INT_MAX``;
  independent providers; Formation member ordering; synchronized providers with
  unequal native step sizes.
* DifferentialCorrector, Yukon, nested DifferentialCorrector and mixed
  Yukon/DifferentialCorrector contexts, each with None/Current/All. The test
  observes actual numerical solver clone states to distinguish trials from the
  accepted pass. These witnesses verify final retained trajectory membership and
  restored cadence; they do not alone prove every transient display state.
* An actual non-Earth translated origin, using the existing Ceres SPK as Probe's
  position source, prescribed static orientation, spherical shape and an
  independent inclined circular orbit. OrbitView Cartesian positions/velocities
  and GroundTrack longitude/latitude are checked analytically. This is stronger
  than the earlier synthetic origin-conversion check, but does not generalize
  to spacecraft-dependent frames.
* Nonlinear ABM stops at37.123 and37.25seconds. Both consumers retain exactly the
  independently expected accepted-step grid, including the due shortened final
  advance. Prior producer proof about discarded stop trials is reused at its
  sealed source version, not replayed or claimed as new credit.

Primary-form keyboard editing, draft isolation, Apply, exact Undo/Redo and
Unicode save/reload are exercised for the new setting. Seven basic, three DC,
three physical and nine additional solver cases were executed in separate
source-versioned invocations. Earlier passing cases were reused rather than
replayed after unrelated test additions. This is **not** a claim that all22
were executed together on the final build.

Five directly affected original tests also passed visibly: native plot cadence
and solver publication DC/Yukon/Nested/Mixed. Their assertion sources exactly
match the pre-consumer step-publication evidence inventory. The four original
solver tests retain their live membership assertions. Their legacy-mode proofs
are not substituted for new-mode transient membership proof.

``AuditPlotIntegrationStepEvidence.py`` independently checks1,338 saved position
values,892 epochs and12 geographic values across both delivery modes. Native
velocity assertions also pass; the offline count is explicitly position-only.
It binds raw/proof/log/JUnit relationships, exact selected cases, executed Qt
source identities, source archives, original assertion identities and the
unchanged canonical ledger. It records732 native xcb exposure events across
the new and affected visible runs. Windows were resizable, non-fullscreen and
initially Iconic. No screenshots were retained, and no physical compositor,
hardware GPU or human observation is claimed.

Four negative audit controls coherently alter copied proof/log/data and update
their hashes. Incorrect position, longitude, epoch and nonresizable-window
claims are still rejected. Original evidence is untouched; no native replay
was used for these controls.

All failures and storage
------------------------

``build/nasa-coverage/coverage95-20261102-plot-step-consumers/failures.json``
records the two consumer failures, the initial edit-installation assertion,
two fixture/runtime problems, two compiler errors and two launch/preflight
problems. These include the sandbox X11 rejection, startup-before-Spacecraft
ordering, a fixture traversal stuck on the executing branch's self-returning
``GetNext()``, and the stale broad review's Command_Report.xml reference.
The stuck process was terminated only after its exact executable identity was
checked; its parent invocation retains FAILED status. No failed invocation is
reclassified as a pass or used as successful whole-family evidence.

The execution-only regression review contains no new qualification reviews and
preserves original assertion hashes. It does not update or waive the older
broad NASA review. Failed source versions, build logs, raw receipts and concrete
counterexamples remain available.

Lossless compression of105 duplicate/summary JSON files saved113,023,777bytes
(107.8MiB), with original/compressed hashes and byte-for-byte roundtrip checks.
Approximately107.3MiB of regular evidence files remain, including raw native
traces. There are zero retained PNG or ELF copies. Launcher symlinks are live
replay locations, not immutable runtime evidence.

The initial manifest's enumeration followed those live symlinks. The separately
preserved V2 manifest corrects that binding error and records their targets as
an inventory, without changing native receipts, source versions or assertions.
V2 binds660 regular artifacts; V1 is retained as superseded. File hashes identify
recorded build/runtime files, not all actually loaded images. No full-current-
build or scientific/core qualification is inferred.

Exact open rows and next work
-----------------------------

The canonical cumulative ledger remains::

   build/nasa-coverage/coverage95-20261101/multi-target-coverage-snapshot.json

FRR!E329/E330 (OrbitView dynamic updating/steps skipped) and FRR!E633/E634
(GroundTrack data options/steps skipped) remain unqualified. Their exact source
texts, fingerprints and original statuses are unchanged. Required next work:

* Conjoin new-mode live transient solver membership and new-mode redraw/retention
  behavior with the already verified child requirements. Final curve membership
  alone does not establish every Current-mode intermediate state.
* Resolve the original enabled ``QtGui.NasaGroundIntegrationSkipBoundary`` gate.
  Its unchanged fixture targets legacy ``DataCollectFrequency``; its earlier
  failure remains open. It has not been disabled, rerun merely for context or
  relabeled. Adding an opt-in capability does not silently retire that gate.
* Verify finite-burn endpoints and ephemeris propagation, which has no native
  integration-step journal. Define unsupported-provider behavior explicitly;
  the current explicit mode has no endpoint samples for an unjournaled provider.
* Supply correct sampled-state semantics for spacecraft-origin/dependent frames,
  accelerations and attitude getters. The existing ParameterStateScope reaches
  OrbitData wrappers, not arbitrary live-object getters. Different provider
  endpoint epochs also need coherent reference-object states. The translated
  celestial-body witness does not prove those cases.
* Audit same-epoch state changes, toggled intervals, partial/absent-slot metadata,
  repeated-run/function lifetimes and all inherited clauses before whole-row
  admission. The focused sampler assertions are bounded evidence, not general
  qualification of these contexts.

All149 original gaps and the previous59 prerequisite holds remain in the
canonical ledger. With all59 unresolved, the conditional optimistic ceiling
remains1,099/1,158 (94.9%); at least two need substantive resolution. MATLAB,
OEM expectation authorization, historical OS/hardware, scientific/core review
and all other handoff constraints remain unchanged.

Replay and offline audit
------------------------

Build the registered new family::

   cmake --build build/linux-gui --target GmatQtPlotIntegrationStepTests -j4

An explicitly requested full watch can run::

   python3 src/qtgui/tests/RunPlotIntegrationStepAcceptance.py --visible

For only new/affected cases, use ``--cases`` (comma-separated), for example::

   python3 src/qtgui/tests/RunPlotIntegrationStepAcceptance.py --visible \
     --cases TranslatedOrigin,NonlinearABM,NonlinearABMQuarter

Omit ``--visible`` for private-X11 unattended execution. CTest registration is
``QtGui.PlotIntegrationSteps``; both acceptance build targets include it.
Choose a fresh output directory when supplying ``--output``. No source edits
may occur during a native acceptance invocation.

Verify the sealed evidence without executing GMAT::

   python3 src/qtgui/tests/AuditPlotIntegrationStepEvidence.py \
     --manifest build/nasa-coverage/coverage95-20261102-plot-step-consumers/evidence-manifest-v2.json \
     --output /tmp/plot-step-consumer-audit.json

The authoritative bounded audit is ``evidence-audit-v2.json`` in that stage
directory. ``audit-rejection-controls.json`` records the four negative controls.
No commits, pushes or external publications were requested or performed.
