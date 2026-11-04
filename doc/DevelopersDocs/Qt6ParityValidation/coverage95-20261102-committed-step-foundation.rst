Accepted-step journal and prerequisite engine repairs
=====================================================

Original requirements coverage remains **1,009/1,158 (87.1%)**. Historical
coverage remains1,816/2,367 (76.7%);92 requirements are still needed for95%.
This batch assigns **zero new NASA or GUI-mechanism credit**. It implements
and verifies the producer layer needed by the four remaining plotting rows
FRR!E329/E330/E633/E634; those rows remain unqualified.

Implemented behavior
----------------------

AcceptedStepHistory records immutable native state vectors at actual accepted
primitive endpoints. A fixed-interval call can therefore produce multiple
records. Capture is dormant until an explicit transaction is opened; ordinary
runs perform no journal copies, allocations or additional time getters.

A root transaction stages into private storage until Commit. Nested committed
trials append to their parent's staging; discarded trials and exceptions do
not leak entries. Thread-local contexts restore their parent on destruction.
This rolls back **journal entries only**, not the integrator itself.

Four accepted-step paths are covered: ordinary Runge-Kutta, the separate
Nyström path, predictor/corrector and Bulirsch–Stoer. ABM startup legitimately
records through its RK89 starter before genuine ABM corrector steps. Consumers
must group records by their native model/context, rather than assuming each
integrator pointer equals the top-level propagator.

Two demonstrated engine defects repaired
------------------------------------------

Bulirsch–Stoer's fixed-interval driver set the next trial to the full remaining
interval. Its primitive step did not first enforce MaxStep. The retained
50-second diagnostic records exactly one accepted50-second step despite
MaxStep5. The repair caps the signed primitive trial before extrapolation.
Both directions now produce ten5-second endpoints for that interval.

ABM stop cleanup restored physical time while retaining advanced startup and
derivative history. A controlled1-second comparison at unchanged Accuracy1e-10
passes after single-step warmup, but fails after stop-condition warmup: time
reverts2→1 while startup count stays2, then ABM enters its corrector too early.
Its estimated error is2.8447420776988333e-5. Capture was dormant during warmup,
so the journal did not cause this defect.

The previous inverse-history code also copied the RealArray object address,
rather than its element storage. BufferHistory now invokes the existing virtual
Reset after physical rollback. ABM invalidates its history and derivative pointer,
then reconstructs valid history through its existing starter. No coefficients,
error estimator or accuracy setting changed. Tests require count0/incomplete
after actual rollback and genuine ABM corrector work after startup.

Exact preimages and patches are under ``committed-step-production-before``.
The build guard reconstructs the original normalized source text after removing
only the endpoint hooks and these two explicit policies. No other original
step calculation changes are admitted. The new journal has no effect on the
layout or virtual interfaces of existing engine classes.

Verified evidence
-------------------

The complete matrix passes unattended and visibly: eight methods in both
directions,16 cases,160 primitive endpoints and960 independent Cartesian
component comparisons per mode. Other methods use5-second primitives; ABM
uses1-second primitives, all at unchanged Accuracy1e-10. Initial ABM5-second
failures remain explicit and are not credited as verified5-second behavior.
Numerical assertions remain3e-5km /4e-8km/s; independent rereading finds maximum
errors5.46e-12km /4.44e-15km/s for the bounded circle inputs.

Native assertions also check all six unchanged live spacecraft components and
epoch, zero new publication packets, nine invalid transaction operations,
nested commit/discard, exception unwinding and thread isolation. Raw retained
ABM journals demonstrate reset and actual startup/corrector execution.

Only two existing GUI/integrator-control cases were replayed: ABM and
Bulirsch–Stoer. Both pass in both modes with their unchanged property, invalid
input, Undo/Redo, Unicode reopen and600-second independent result assertions.
Their original report tolerances remain2e-5km /2e-8km/s. Other passing tests
and the previous accepted-step preflight were not replayed.

Visible runs use initially minimized, resizable, non-fullscreen windows and
automatic host scaling. The new native matrix records two xcb exposure events
and no PNGs. This is visible native evidence, not a new GUI editing mechanism
or physical host/GPU qualification. Actual project image maps bind the visible
matrix process to its executable, GmatBase and ExtraPropagators library hashes.

Six failed test attempts are retained: missing Nyström capture, the BS bound
violation, and ABM profile/rollback diagnostics. The single-step ABM control
passes at the same1-second bound/accuracy as the failing stop-mode control.
All failed receipts and raw journals remain available. Duplicate generated
summaries are losslessly compressed; no runtime/physical-kernel copies or
passing screenshots are retained.

Replay and audit
------------------

Build and watch only the new contract::

  cmake --build build/linux-gui --target GmatQtAcceptedStepTransactionTests -j4
  python3 src/qtgui/tests/RunAcceptedStepTransactionAcceptance.py --visible

Omit ``--visible`` for unattended execution. ``--list`` lists just this CTest.
``GMAT_ACCEPTED_STEP_METHOD=AdamsBashforthMoulton`` selects a diagnostic method;
``GMAT_ACCEPTED_STEP_WARMUP=single`` selects the single-step control. The
unfiltered/default-stop matrix is the recorded final acceptance configuration.
Both test-build targets include the new executable.

Evidence is under ``build/nasa-coverage/coverage95-20261101/``:

* ``committed-step-evidence-manifest.json`` and ``committed-step-evidence-audit.json``.
* ``committed-step-headless-v4`` and ``committed-step-visible-v1``: final matrix.
* ``committed-step-affected-headless-v1`` / ``committed-step-affected-visible-v1``:
  the two directly affected existing cases.
* ``committed-step-build-guard.json`` and compiled source archive.
* ``committed-step-visible-images.json``, failures, source patches and journals.

The manifest pins consumed receipts, logs, proofs, journals, image metadata and
compiled sources. Verification checks the five executing reader sources,
replays all four source partitions against their exact preimages, and compares
the original regression assertion source with the prior canonical catalogue.

Verify retained results without executing another native/GUI scenario::

  python3 src/qtgui/tests/AuditAcceptedStepTransactionEvidence.py --manifest build/nasa-coverage/coverage95-20261101/committed-step-evidence-manifest.json --output /tmp/committed-step-audit.json

Remaining four-row implementation
----------------------------------

Propagate must bind model-relative times and force-origin vectors to spacecraft,
epochs and J2000/origin context while the non-owning pointers remain live. It
must commit only the accepted trajectory after stop refinement, discard probes,
and preserve ordinary publication/ephemeris behavior.

Then both plot classes need explicit integration-step sampling, with per-trajectory
cadence across commands, single-step/backward/multiple-provider and solver
None/Current/All behavior. Preserve the existing publication-sample mode and
all existing assertions. The original enabled GroundTrack boundary case must
pass, and parent rows must conjoin their redraw/retention children before any
of these four original requirements earns credit.
