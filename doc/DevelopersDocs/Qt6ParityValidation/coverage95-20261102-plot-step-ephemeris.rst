SPK ephemeris sampling for OrbitView and GroundTrack
===================================================

Coverage is **1,009 of 1,158 (87.1%)**. Of the 149 open requirements, 92 more
must pass to reach 95%. Historical operations have a separate count of 1,816 of
2,367. This stage adds evidence for four open requirements and admits zero rows.

The forward Type 9 SPK case originally retained just the starts of the two
Propagate commands, at 0 and 10 seconds. With one step skipped, its expected
points were 0, 20 and 37 seconds. The failed run and first fixture revision are
preserved. That fixture also initially plotted in EarthMJ2000Eq while the
synthetic SPK supplied J2000/ICRF states. The passing fixture requests EarthICRF
so its coordinates can be compared directly with the kernel polynomial.

Propagate now captures the ephemeris provider's J2000 state after a successful
advance and keeps it in the candidate vector until the stopping search commits
that step. The publication identifies ephemeris records separately from accepted
numerical integration steps. Both existing plot consumers use the recorded
spacecraft identity and sample epoch. The core preimages and exact patches for
Propagate.cpp and Publisher.hpp are in ``core-change-audit.json`` beside the
stage manifest. The SPK inputs are generated 4 KiB test kernels.

The Type 9 forward and Type 13 backward cases exercised a ten-second command,
a repeated start and a shortened final advance. Zero skip collected the full
0, 10, 20, 30, 37 second grid. These three cases passed unattended and visibly
at source-v2. The source-v3 mixed case used one synchronized propagation with a
Type 13 SPK spacecraft and a second spacecraft on a three-second RungeKutta89
integrator. The SPK provider delivered 0, 10, 20, 23 seconds. The numerical
provider delivered 0, 3, 6, 9, 10, 13, 16, 19, 20, 23 seconds. With skip one,
both plots showed 0 and 20 seconds for the first spacecraft and 0, 6, 10, 16,
20 seconds for the second. Direct observation of the publisher checked the
provider kind, method, identity, initial flags and ordering. A discarded advance
past the requested stop would fail these checks.

The independent audit recomputed the cubic positions and derivatives and the
second spacecraft's circular trajectory. It checked 64 provider records, 72
plot epochs and 216 Cartesian components across both execution modes. The two
visible runs recorded 16 native X11 exposure events on initially iconic,
resizable, non-fullscreen nested desktops. GroundTrack evidence here covers
sample membership and finite coordinates. Its Earth longitude and latitude
accuracy still needs an independent geographic comparison for these fixtures.

Requirement disposition
-----------------------

* FRR!E330 and E634 have bounded evidence for explicit sampling of these two
  SPK segment types, including the mixed provider run. Other ephemeris formats,
  general spacecraft-dependent frames, sampled acceleration and attitude,
  lifetime transitions, and the original legacy boundary gate remain open.
* FRR!E329 and E633 inherit their respective open sampling children. Their
  complete redraw, retained-history and rendering conjunctions remain open.
* Scientific and core review, full current-build qualification and all 59
  prerequisite holds retain their existing dispositions. The separate OEM
  ShortArc expectation and MATLAB availability questions remain open.

Evidence and replay
-------------------

The stage root is ``build/nasa-coverage/coverage95-20261102-plot-step-ephemeris``.
Its manifest binds 191 artifacts including source archives, exact core diffs,
the preceding burn manifest and audit, and the canonical cumulative ledger.
Five altered proof copies were rejected for wrong provider provenance, an extra
crossed stop, wrong position, wrong velocity and wrong collected epoch. These
controls exercise the per-case validator. Lossless compression saved 49,089,641
bytes. The retained evidence has zero PNGs or runtime ELF copies. The inventory
distinguishes 135 mutable launcher links from archived evidence files.

For visible replay, choose a fresh output directory and run the registered test
through its case selector. Omitting ``--visible`` runs the unattended mode.
Audit the original receipts from the repository root with the second command.

.. code-block:: bash

   python3 src/qtgui/tests/RunPlotIntegrationStepAcceptance.py --visible --pause-ms 150 --cases EphemType9Forward,EphemType13Backward,EphemType13Zero,EphemType13Mixed --output build/gui-acceptance/plot-step-ephemeris-local-replay
   python3 src/qtgui/tests/AuditPlotStepEphemerisEvidence.py --manifest build/nasa-coverage/coverage95-20261102-plot-step-ephemeris/evidence-manifest.json --output /tmp/plot-step-ephemeris-audit.json
