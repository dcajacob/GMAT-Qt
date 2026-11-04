95 percent campaign: finite-burn plot sampling
==============================================

Coverage remains **1,009/1,158 (87.1%)**, with 149 original requirements open
and **92 more** needed for 95%. Historical-operation coverage remains
1,816/2,367 (76.7%). This continuation awards **zero whole-row credit**. The
full objective, original denominator, fingerprints, assertions and holds remain
unchanged; the goal is active.

Seven new cases pass unattended and visibly
-------------------------------------------

Both native consumers now have bounded accepted-step evidence for chemical
constant thrust and electric ConstantThrustAndIsp, with mass depletion on/off,
burn shutdown/coast, chemical restart, zero skip and a nonlinear position stop
during mass depletion. Each case uses the real primary plot forms to type the
step setting, apply it, verify exact Undo/Redo and save/reopen Unicode script
paths. Burn hardware is configured in the fixture script; this stage does not
claim new thruster-editor GUI coverage.

The fixture prescribes a spherical body of radius 1,000 km, fixed identity
orientation and mu=1e-20 km^3/s^2. Its existing shared Ceres SPK supplies only
the translated origin; no independent physical Ceres properties or ephemeris
are claimed. Residual central gravity contributes less than 3e-25 km over the
fixture interval. The initial state is (7000,300,500) km and (0,1,0.2) km/s.
Chemical thrust is 100 N with Isp=300 s; electric thrust is 0.5 N with Isp=2500 s,
a constant 10 kW power system and adequate usable-power settings. Both prescribe
g0=10 m/s^2, unit duty/scale and initial dry/fuel masses of 900/100 kg.

Expected motion comes from constant-force acceleration or the closed-form
variable-mass rocket equations, followed by straight-line coast. Native checks
use long-double calculations; the offline audit recomputes with 60-digit Decimal
arithmetic. Both plot consumers' exact point counts and epochs are checked.
OrbitView positions and velocities, GroundTrack longitude/latitude and four
reported Cartesian/fuel/total-mass states per case have independent numerical
oracles. Finite-burn setup is not substituted for measured endpoint behavior.

Ordinary split burn/coast cases select 0,20,37,50 seconds for one skipped step;
zero skip selects 0,10,20,30,37,47,50. Restart burns during 0–10 and 27–50 seconds,
with coast in between, preserving the selected 0,20,37,50 grid. Thus ignition,
shutdown and repeated initial packets do not invent extra collected advances.
The nonlinear case requests a position corresponding analytically to 37.123 s,
then coasts 13 s. Its state oracle uses measured epochs bounded by the original
literal-grid tolerance; its requested position-stop tolerance remains 1e-10 km.

Demonstrated stop-rounding defect and exact correction
-----------------------------------------------------

The first nonlinear fixture incorrectly evaluated every state at the nominal
requested time even though its allowed epoch window was 1e-4 s. The actual stop
was about eight microseconds earlier, causing a new-oracle failure. Its native
messages also exposed a separate requested-stop-tolerance miss. That failure
and source-v2 are preserved.

Source-v3 corrects only the new physical oracle to evaluate at measured epochs,
and adds an explicit assertion of the unchanged 1e-10 km position request.
Physical states, mass flow and plot membership pass, while this assertion fails
with residual -1.609805622138083e-10 km. The second failed receipt remains FAILED.
The two failures are not reclassified or used as passing family evidence.

Propagate::TakeFinalStep rounds a refined duration to the existing 1e-6 s
TIME_ROUNDOFF grid. In this case that rounding undoes sufficient refinement.
The narrow production correction preserves the rounding when it meets the
requested tolerance. Otherwise it restores the saved start state and retries
the unrounded refined duration. It clears the failed rounded candidate's
integration-step journal before committing the retry, so rejected endpoints
cannot leak into plotting. It leaves the existing warning in place if the retry
still misses tolerance. TIME_ROUNDOFF itself, force models and integrator
algorithms were not changed.

The unchanged position request then passes with residual
1.8189894035458565e-12 km and no stop warning. Both existing nonlinear ABM
consumer cases also pass unattended and visibly as directly affected checks.
Their original epoch/count/circular/velocity assertions remain intact. This is
not all-integrator, all-stop-type or scientific/core approval of the change.
Exact Propagate.cpp preimage and patch are retained for that review.

Source-version and evidence boundaries
--------------------------------------

The first six cases ran at source-v1 before the core correction. The nonlinear
case and two affected ABM checks ran at source-v4 after it. Earlier passes were
reused at recorded versions, not replayed for context or claimed as one complete
latest-build suite. Seven new cases plus two affected cases have both delivery
modes; the stage does not claim whole-row qualification from their count.

Evidence root::

   build/nasa-coverage/coverage95-20261102-plot-step-burn

The canonical evidence-manifest.json binds 291 retained artifacts and references
the preceding sealed retention-stage audit. evidence-audit.json verifies source
archives, native receipts/JUnit, proof/log/raw relationships, visible policy and
the unchanged canonical ledger. It checks 284 epoch, 426 Cartesian-position,
186 velocity, 124 geographic and 504 report-value comparisons. Position counts
include the two affected ABM cases; saved velocity counts are burn cases only.

Seven coherent altered-evidence controls are rejected after updating the changed
artifact hashes and matching proofs/logs: position, velocity, longitude, fuel
mass, epoch, window policy and a nonlinear residual outside 1e-10 km. The last
control remains within the looser general position tolerance, so it specifically
checks the nonlinear stop gate. Original artifacts are untouched.

Thirty-five JSON records were losslessly compressed with byte-for-byte roundtrip
checks, saving 68,694,725 bytes (65.5 MiB). About 24.4 MiB of regular evidence
remains. There are no PNG or ELF copies. The 189 launcher symlinks are inventoried
as mutable replay locations, not immutable executed-image artifacts.

Visible runs passed in 22.531 s (six initial cases) and 9.688 s (nonlinear plus
two affected ABM cases). They record four native xcb exposure events in initially
Iconic, resizable, non-fullscreen windows. This is not per-widget native exposure,
human observation, physical compositor/hardware GPU, all-loaded-images,
external-plugin ABI, complete current-build or scientific/core qualification.

Replay and remaining work
-------------------------

The existing QtGui.PlotIntegrationSteps registration and acceptance build targets
include the new cases. Select new or affected cases only, for example::

   cmake --build build/linux-gui --target GmatQtPlotIntegrationStepTests -j4
   python3 src/qtgui/tests/RunPlotIntegrationStepAcceptance.py --visible \
     --cases BurnChemicalFixed,BurnChemicalMass,BurnElectricFixed,BurnElectricMass
   python3 src/qtgui/tests/RunPlotIntegrationStepAcceptance.py --visible \
     --cases BurnChemicalRestart,BurnChemicalZero,BurnChemicalNonlinear

Omit --visible for private-X11 execution and choose a fresh --output directory
when specifying one. Do not edit sources during native acceptance execution.
Audit the saved evidence without running GMAT::

   python3 src/qtgui/tests/AuditPlotStepBurnEvidence.py \
     --manifest build/nasa-coverage/coverage95-20261102-plot-step-burn/evidence-manifest.json \
     --output /tmp/plot-step-burn-audit.json

FRR!E329/E330/E633/E634 remain open. Required conjunctions still include the
unchanged legacy boundary gate, unjournaled ephemeris-provider behavior,
general spacecraft-dependent frames and sampled acceleration/attitude with
coherent provider epochs, other burn/force/solver/integrator combinations,
toggle/absence/same-epoch/function/repeated-run lifetimes, and exact solver
redraw timing plus NumPointsToRedraw rendering. The new finite-burn witnesses
do not imply these unrelated modes have been verified.

All 59 prerequisite holds remain. With them unresolved the conditional optimistic
ceiling is still 1,099/1,158 (94.9%). MATLAB, OEM expectation authorization,
historical OS/hardware and scientific/core approval constraints are intact.
No commits, pushes or external publication occurred. All stage build/test
processes are terminal. The full goal remains active.
