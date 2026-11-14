Sampled harmonic gravity and tidal clocks in plot frames
========================================================

Coverage remains **1,050/1,158 (90.7%)**:108 original requirements are open,
and51 more are needed for95percent. Historical operations remain1,816/2,367.
This increment repairs two real sampled-force failures but admits no whole row.
All59 original prerequisite identities survive:30 resolved,29 unresolved,
including seven unresolved MATLAB rows. The complete1050 sealed prefix is reused.

Observed failures and repair
-----------------------------

Unequal ten-/three-second propagators publish accepted endpoints that can precede
the live spacecraft. A rotating ObjectReferenced plot frame initially failed with
``Sampled object-frame acceleration is unavailable for force GravityField``.
Both unattended and initially minimized visible failures remain at execution
source1,with successful build receipts and actual loaded-project image maps.

The new nonvirtual ``GravityField::GetDerivativesAtState`` accepts a Cartesian
MJ2000 value and absolute ``GmatTime``. It reuses the existing harmonic calculation,
sets zero elapsed offset for this absolute query,and restores both base epochs,
elapsed time and precision flag on return or exception. The ordinary spacecraft
entry preserves its original Real/precision epoch selection and clock behavior.
ODEModel admits this exact force type through the existing sampled context;
all provider identity,lifetime,frame,coeval-state,recursion and finite-result
guards remain. Other sampled force types retain their strict coeval-live fallback.

Review then found that ``GetTideData`` reads separate cached ``now/nowGT`` epochs.
A targeted Solid-tide counterexample measured9.598031223967407e-11km/s2 of error
from a deliberately stale cache in both profiles. The final adapter also sets
and restores these two epochs. Twelve Solid/SolidAndPole model/epoch contracts
pass across both profiles,matching an explicitly coeval instance of the same
native algorithm within5e-15km/s2,with a nonzero tidal contribution above1e-12.
This proves clock coherence;independent tidal physics remains unqualified.

Evidence and its limits
------------------------

Nine new plot cases pass in both profiles:Earth-/spacecraft-origin radial frames,
velocity-aligned frames,two-spacecraft frames,forward/backward propagation and
thinned coeval samples. Actual private propagator clones contain exactly one
GravityField,degree/order0/0,with the shipped JGM2 coefficient identity and Mu
matching the independent circular oracle. OrbitView positions and frame-rate
velocities retain4e-5km and5e-8km/s bounds. GroundTrack checks membership and finite
coordinates;independent Earth geographic/orientation truth is not inferred.

Four affected plot regressions pass in both modes and match their sealed earlier
outputs exactly. Missing frame inputs retain diagnostics,omitted points and line
breaks. The original generic-force controls still preserve coeval calls and reject
stale epochs or changed states before calling an unsupported provider. One
affected rotating-velocity case is checked again in both profiles after the tide
repair;its complete proof equals the earlier pass.

The Earth/Ceres harmonic cases retain all original GUI,trajectory,force-grid,
complex-step/DP5,Undo/Redo,Unicode and ReportViewer assertions. Added sampled
checks span four absolute epochs,seven axis/pole/general positions,ten truncation/
field profiles per body,and a deliberately nonzero1234.5-second force clock.
They include an injected origin exception and recovery without live owner changes.
The final four GUI contexts contain1120 sampled-force queries. The previous four
contexts remain and their proofs and untided sampled observations match exactly.
The offline audit consequently checks2240 preserved query records,6720 acceleration
components,440 plot epoch/connection pairs and1248 plot Cartesian components.
Native rotations remain explicit shared inputs,not independent orientation truth.

The864-case frame-derivative contract and three command-line acceleration context/
lifetime/handoff regressions pass. The latter have actual process/image custody.
Passing visible traces provide252 native exposures across the audited source
versions,in initially minimized,resizable,non-fullscreen desktops. No screenshots,
runtime binaries or SPK kernels are copied. The unchanged harmonic harness retains
its synthetic and JGM2 coefficient inputs for the independent potential audit.

Eight native failures,three preflight failures and one failed build are retained.
Four native failures came from a new test probe used before rebinding/initializing
its deliberately cleared copy-constructor frames;the corrected fixture preserves
all assertions. The build failure was an inconsistent multi-declarator ``auto``
type in the added tide test. No failed build authorized native execution. The
preflights rejected stale review inputs or an absolute review path before starting
a native test;fresh repository-relative reviews retain exact current hashes.

Core changes,remaining rows and custody
----------------------------------------

The inventory remains263 pending core paths:260 prior hashes are unchanged.
Only existing ``GravityField.cpp``,``GravityField.hpp`` and ``ODEModel.cpp`` have
additional increments. Both initial and final patches and inventories remain.
No class member or virtual slot is added;the new C++ entry requires technical
review. All137 original ``require`` statements in the two extended native tests
survive. Of1044 older test/helper/case paths,1040 are byte-identical;the remaining
four are the two additive native tests,the plot runner's added references,and
the core-change descriptions.

Stage:``build/nasa-coverage/coverage95-20261103-plot-gravity-samples``.
Execution sources1-9,all build receipts,raw native maps,logs,JUnit,proofs and
failures remain. Source6 has a failed build and no native admission. Final gravity
and tide execution uses source9;the larger plot matrix reuses exact source2.
The final review archive preserves later offline auditor/replay/document changes
separately from execution capsules. Seven damaged proof controls are rejected.
The sealed audit can run from its preserved Python sources without treating
current mutable source or coefficient bytes as historical execution evidence.

``remaining-requirement-audit-v2.json`` retains all108 exact keys,fingerprints and
recorded gaps,plus the59 hold identities. FRR!E329/E330/E633/E634 remain open:
general mass/attitude/hardware-dependent sampled forces,further provider/reference
lifetimes,remaining solver/render boundaries and inherited conjunctions still need
evidence. The historical skip/period notes remain verbatim;the explicit skip-count
control and extensive prior cadence evidence already exist. They must not be
reimplemented or erased to resolve a stale note. No bounded matrix substitutes for
the original whole-row obligation. Scientific,technical-core,full-current-build
and full-NASA gates remain false. The full95percent goal remains active.

Replay only new or affected cases
----------------------------------

The existing ``QtGui.PlotIntegrationSteps`` and two
``QtGui.NasaHarmonicGravityEarth/Ceres`` registrations are reused. The new wrapper
creates a fresh bounded review for harmonic replay;its registration lists are
retained. Use fresh output directories and leave the sealed stage immutable::

   cmake --build build/linux-gui --target GmatQtPlotIntegrationStepTests GmatQtNasaHarmonicGravityTests --parallel 2
   python3 src/qtgui/tests/RunPlotGravityAcceptance.py --group plot --output /tmp/gravity-plot-headless-new
   python3 src/qtgui/tests/RunPlotGravityAcceptance.py --group plot --visible --output /tmp/gravity-plot-visible-new
   python3 src/qtgui/tests/RunPlotGravityAcceptance.py --group harmonic --output /tmp/gravity-harmonic-headless-new
   python3 src/qtgui/tests/RunPlotGravityAcceptance.py --group harmonic --visible --output /tmp/gravity-harmonic-visible-new
   python3 src/qtgui/tests/AuditPlotGravityEvidence.py --stage build/nasa-coverage/coverage95-20261103-plot-gravity-samples --manifest build/nasa-coverage/coverage95-20261103-plot-gravity-samples/evidence-manifest-v1.json --output /tmp/gravity-evidence-audit-new.json

If later work changes the auditor or its imports,extract the source-only
``final-reviewed-sources-v1.tar.gz`` to a fresh temporary directory and run that
preserved ``src/qtgui/tests/AuditPlotGravityEvidence.py`` with ``--repo`` pointing
to this checkout and the same sealed stage/manifest paths. The seal records a
successful independent replay using that preserved source tree.
