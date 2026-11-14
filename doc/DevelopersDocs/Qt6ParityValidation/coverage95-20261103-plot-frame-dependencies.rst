Coeval LocalAlignedConstrained plot dependencies
================================================

The new native witness reproduced a **46.584746 km** sampled-position error.
OrbitPlot now follows actual coordinate-system dependencies, including an LAC
ReferenceObject and nested constraint frames, before creating its read-only
packet scope. Six new cases and four affected regressions pass unattended and
visibly. Coverage remains **1,019/1,158 (88.0%)**, with 139 open requirements
and 82 more needed. Historical operations remain **1,816/2,367 (76.7%)**.
No complete original row is admitted by this increment.

Repair and exact scope
-----------------------

Previously only the view origin and ObjectReferenced primary/secondary
spacecraft were indexed. An LAC ReferenceObject could therefore read the
spacecraft's later live state while converting an earlier accepted endpoint.
The original native failure, full raw viewer histories, actual final owner
states, source capsule, image identities, log and JUnit remain failed evidence.
The original 4e-5 km position and 5e-8 km/s velocity bounds remain unchanged.

The sole production increment is in ``src/base/subscriber/OrbitPlot.cpp``.
A synchronous traversal collects origin, primary, secondary and reference
spacecraft, then follows coordinate-system references exposed by the axes.
It visits actual frame identities once and copies each reference-name list
before recursion, because native implementations reuse static list storage.
The existing packet scope still resolves exact runtime identities and epochs,
rejects ambiguous/unavailable states, and never changes live spacecraft to
render a point. No state, axis, force or integration formula changes; no class
layout, virtual interface, parameter ID or writable flag changes.

The pending core inventory remains 245 paths: the previous 244 hashes are
unchanged and the existing OrbitPlot path has this additional exact patch.
The stage's ``OrbitPlot-increment-v2.patch`` isolates it from earlier dirty
workspace changes. The full schema-2 core inventory and patches remain under
``core-inventory-v1``; technical/scientific approval remains open.

New witnesses and retained regressions
---------------------------------------

``LACReferenceForward`` and ``LACReferenceBackward`` use Earth origin, a
spacecraft reference on a prescribed yz circular orbit, and a fixed inertial
constraint. Numerical providers have ten- and three-second primitive steps.
Both viewers retain every accepted endpoint; OrbitView's position and velocity
match independently constructed LAC bases and differentiated positions.

``LACConstraintForward`` and ``LACConstraintBackward`` add an ObjectReferenced
constraint frame using the other spacecraft. The finer trajectory has valid
frame inputs only at 0, 10, 20 and 23 seconds (or their negatives). Missing
interior inputs are omitted with explicit diagnostics and line breaks; the
20-to-23 segment stays connected. ``LACConstraintThinned`` selects only coeval
0/10/20-second points and retains their connections. ``LACLiveReference`` uses
an unpropagated reference: OrbitView retains only the valid initial point,
while GroundTrack and the mission continue.

The native oracle uses prescribed geometric bases. The separate Python audit
constructs the constraint projection and independently differentiates the
position with a different finite-difference interval. It checks **226
epoch/connection pairs, 588 Cartesian components, 144 final owner-state
components and ten missing-reference diagnostics** across both modes.
GroundTrack evidence covers membership and finite coordinates, not independent
Earth geographic or orientation truth. Earth/Sun auxiliary render curves remain
in the raw captures without acquiring new independent numerical qualification.

Four affected existing cases retain their original assertions and match their
sealed earlier trajectory outputs exactly in both modes:
``FrameFollowerForward``, ``FrameLeaderMissing``, ``RotatingOriginForward`` and
``RotatingPairForward``. The rotating generic-force guards remain intact.
There are **12 new-case and eight regression passing contexts**. Five actual
native process maps bind 26 project images each. The two visible processes
record four native exposures in initially minimized, resizable, non-fullscreen
nested desktops. No per-widget exposure, hardware GPU or human observation
claim is inferred. There are no retained screenshots or copied runtime/kernel
binaries. The first passing pilot is reused; no successful case is replayed
after unrelated source changes.

Failures and audit
-------------------

Two failed invocations remain: the sandbox could not start the private X display,
and the first real native execution exposed the geometric error. The authorized
display retry reached the unchanged numerical assertions. Both builds and the
new offline audit pass. No native interruption or assertion relaxation occurred.
Four coherently damaged proof controls reject wrong position, velocity, line
connection and runtime-reference identity.

``AuditPlotLacEvidence.py`` checks raw/proof/log/JUnit agreement, exact source
capsules, actual image-map custody, visible policy, all twelve new deliveries,
eight unchanged regression outputs and the original failure. The executed-source
supplement retains 27 unchanged input paths omitted from the initial broad
capsule; their actual execution hashes matched before any production edit.
The larger capsule's extra files are source/input retention, not extra tests.

The authoritative stage is
``build/nasa-coverage/coverage95-20261103-plot-frame-dependencies``.
FRR!E329/E330/E633/E634 retain their original fingerprints and whole-row gates:
general sampled force/attitude inputs, further provider/reference lifetimes,
remaining solver/render boundaries and inherited conjunctions. All 59 original
hold keys remain open. The newly verified MATLAB installation changes its
availability prerequisite, not the acceptance status of 37 dependent rows.
See the separate MATLAB prerequisite report. Full current-build and full NASA
qualification remain false; the full 95 percent goal remains active.

Replay only new or affected cases
----------------------------------

Use fresh output destinations::

   cmake --build build/linux-gui --target GmatQtPlotIntegrationStepTests -j4
   cases=LACReferenceForward,LACReferenceBackward,LACConstraintForward,LACConstraintBackward,LACConstraintThinned,LACLiveReference
   python3 src/qtgui/tests/RunPlotIntegrationStepAcceptance.py --cases "$cases" --pause-ms 0
   python3 src/qtgui/tests/RunPlotIntegrationStepAcceptance.py --cases "$cases" --visible --pause-ms 150
   python3 src/qtgui/tests/AuditPlotLacEvidence.py \
     --stage build/nasa-coverage/coverage95-20261103-plot-frame-dependencies \
     --output /tmp/plot-lac-audit.json

The case selector uses the existing ``QtGui.PlotIntegrationSteps`` registration
and its existing acceptance build dependencies. The retained stage launcher also
records actual loaded images for chosen case sets. Reuse sealed passes unless
a relevant change or failure requires replay.
