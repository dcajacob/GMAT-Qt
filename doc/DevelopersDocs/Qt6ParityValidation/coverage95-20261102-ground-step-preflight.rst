GroundTrack skip preflight: real accepted-step granularity
========================================================

Coverage remains **1,003/1,158 (86.6%)**, with **98** whole requirements needed
for 95%. Historical coverage remains **1,816/2,367 (76.7%)**. No production
code changed and no NASA requirement or GUI-mechanism credit was awarded.

The existing enabled GroundTrack boundary assertion remains unchanged and
unresolved. Its two Propagate commands demonstrate that repeated initial-state
publications advance the legacy collection counter. A simple packet-tagging
repair is insufficient for the four original rows FRR!E329/E330/E633/E634.

New native evidence
-------------------

``QtGui.AcceptedStepGranularity`` passes unattended and visibly. A test-only
RungeKutta89 subclass calls the unchanged native Step implementation exactly
once per primitive invocation and records its actual time and six-state vector.
The real runtime clone is first initialized through a ten-second native mission.
A single subsequent ``Step(100)`` call with MaxStep=10 contains ten accepted
primitive steps at elapsed times 20 through 110 seconds. All sixty state values
match an independent literal-mu circular-orbit calculation. An ordinary native
publication observer receives zero packets during that fixed-interval call.

This establishes a concrete gap between propagation-output packets and accepted
numerical integration steps. It does not claim every integrator has the same
step pattern. The visible run records two native window exposures; no new GUI
editing mechanism, physical host input or compositor behavior is qualified.

``AuditAcceptedStepGranularity.py`` checks both modes, the independent grid and
state references, raw/proof/log/JUnit identity, 139 frozen compiler dependency
files, executed source capsules and unchanged runtime maps. It verifies 29
consumed identities and 297 unchanged production source files. No existing
passing scenario was rerun.

Two launch failures remain explicit: the first registration used an offscreen
wrapper rejected by the intact native-widget guard; the corrected native wrapper
then encountered sandbox Xvfb socket restrictions before calculations started.
The authorized private-display run passed. No numerical assertion was weakened.
The visible run used the usual resizable, initially minimized, automatically
scaled nested desktop, without fullscreen or screenshots.

Why the four-row repair needs more than a counter change
------------------------------------------------------

Independent preflight review identifies these requirements for a correct design:

* Capture committed accepted-step endpoints, including multiple accepted steps
  inside one fixed-interval call. Rejected stop-crossing attempts and refinement
  probes must not become accepted samples.
* Count the committed nonzero final/refined advance once. It is a real step,
  not a bookkeeping duplicate.
* Deliver single-step endpoints to opted-in plots; the current routing sends
  them only to ephemeris writers.
* Preserve cadence for each tracked trajectory across commands, without unrelated
  providers, partial spacecraft packets or metadata updates consuming its count.
* Preserve packet context through nesting/exceptions, and restore/reset cadence
  through solver rollback, iteration boundaries and new runs.
* Implement both native plot classes. GroundTrack derives directly from
  Subscriber, so an OrbitPlot-only change cannot repair both plots.

Existing publication-based cadence and raw solver-publication assertions must
remain intact. A direct skipped-step count needs safe zero and INT_MAX handling.
Independent boundary, shortened-stop, single-step, backward, multiple-provider
and solver None/Current/All checks are required before the four rows can qualify.
The parent rows also require their existing redraw/retention children conjoined.
No incomplete sampling feature has been installed.

95 percent prerequisite limit
-----------------------------

The current ledger has 155 remaining requirements, including all 59 previously
reviewed prerequisite holds: 37 MATLAB-dependent rows, 11 removed two-body-model
rows, eight historical OS rows and three undefined hardware-target rows.
There are 96 other remaining rows. Even proving every one of those would yield
**1,099/1,158 (94.9%)**. This is an optimistic conditional ceiling, not a forecast.
At least two held obligations need substantive resolution to reach 95%.

No MATLAB command or installation was found in the usual local locations during
this preflight. A question requesting any available licensed installation path
is pending. No license availability, answer or historical-policy change is
assumed. The original denominator and source obligations are preserved.

Evidence and replay
-------------------

The decision record, exact original four rows, source-versioned audit, launch
failures and prerequisite arithmetic are under
``build/nasa-coverage/coverage95-20261101/ground-step-preflight-*``.
Twenty redundant generated summaries were losslessly compressed, saving about
34.4 MiB. Stage allocation before the final source seal was 13.0 MiB, with no
passing screenshots or copied runtime/kernel binaries.

Build and watch only this diagnostic::

   cmake --build build/linux-gui --target GmatQtAcceptedStepGranularityTests -j 4
   python3 src/qtgui/tests/RunAcceptance.py --build-dir build/linux-gui \
     --filter '^QtGui.AcceptedStepGranularity$' --visible \
     --display-route nested --display-scale auto --start-minimized \
     --pause-ms 350 --max-screenshots 0 --max-checkpoints 200 \
     --output build/gui-acceptance/accepted-step-granularity-watch

Choose a fresh output directory and restore the minimized desktop to watch.
Omit the visibility/display options for the private-Xvfb unattended run.
Verify retained evidence without running GMAT::

   python3 src/qtgui/tests/AuditScriptEventCommandAcceptance.py \
     --verify-receipt build/nasa-coverage/coverage95-20261101/ground-step-preflight-evidence-audit-v1.json

The next implementation needs a committed-step endpoint/rollback design before
either plot can supply literal integration-step skipping. The four rows and
the 95% goal remain open.
