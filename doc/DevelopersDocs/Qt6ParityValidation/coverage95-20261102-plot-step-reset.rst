Same-epoch state assignment in both plot consumers
=================================================

All three new cases pass visibly and unattended at source-v4. A spacecraft is
propagated around a static spherical body with mu 1,000 km³/s² and then assigned
six new Cartesian components at its current epoch. Those components describe a
second circular orbit with the same radius and a phase advanced by 2,000 seconds.
Expected Cartesian positions and velocities, spherical longitude and latitude,
point counts, times, and every line connection are checked independently.

* ResetSkipped assigns at 10 seconds, where skip one suppressed the old point.
  Both plots show 0, 10 and 30 seconds, with a line only from 10 to 30.
* ResetCollected assigns at 20 seconds. Both show 0, 20, 20 and 40 seconds.
  The two distinct states at 20 seconds belong to separate line segments.
* ResetPeer adds a second spacecraft to the same propagation. Both viewers
  preserve its uninterrupted 0, 20, 40 second trajectory while drawing the
  first spacecraft's two arcs. The peer's radius is 2,000 km and the assigned
  spacecraft's radius is 1,500 km. Both orbits have 30 degrees inclination.

The first failure at source-v1 showed the sampled trajectory drawn across the
assignment. The sampler recognized the discontinuity but the renderer received
only coordinates. Source-v2 supplies a per-spacecraft arc-start indication.
Both single-spacecraft cases passed unattended and visibly there. Source-v3's
new peer failed because the compact packet omitted its unchanged state. That
omission broke the peer's line as though the spacecraft had become unavailable.
Source-v4 distinguishes unavailable, selected, starts-new-arc and intentionally
skipped samples in synchronous plot metadata. OrbitView carries this metadata
with its existing buffered solver samples. GroundTrack consumes it once against
the packet's epoch and spacecraft names. Ordinary availability messages retain
their original binary presence values.

Evidence and scope
------------------

``build/nasa-coverage/coverage95-20261102-plot-step-reset`` contains both
failed receipts, the four source archives, six production preimages and exact
diffs, the manifest and audit. The two final invocations also ran the affected
LiveSolverCurrent regression. Its solver states, final curves, pass counts and
live-observation summary agree with the corresponding earlier receipt. The
unchanged QtGui.OrbitDataAvailability registration passed in its declared
headless route, including malformed/stale masks and buffered Current replay.

The audit binds 261 artifacts. Across the initial source-v2 pair and final
source-v4 matrix it checked 84 epoch/connection pairs, 252 Cartesian components
and 84 geographic components. Visible invocations recorded 22 native exposures
in resizable, initially iconic, non-fullscreen desktops. Six altered proof
copies were rejected for wrong connections, epoch, Cartesian position or
velocity, or geography. Lossless compression saved 68,609,023 bytes. Replay
commands use a fresh directory and keep passing screenshots disabled.

.. code-block:: bash

   python3 src/qtgui/tests/RunPlotIntegrationStepAcceptance.py --cases ResetSkipped,ResetCollected,ResetPeer --visible --pause-ms 150 --output build/gui-acceptance/plot-step-reset-local-replay
   python3 src/qtgui/tests/AuditPlotStepResetEvidence.py --manifest build/nasa-coverage/coverage95-20261102-plot-step-reset/evidence-manifest.json --output /tmp/plot-step-reset-audit.json

FRR!E330/E634 gained evidence for reassigned states and an unaffected peer.
FRR!E329/E633 still inherit those open sampling children and their remaining
redraw/rendering clauses. General frames, function/provider lifetime, distinct
solver trial resets, ephemeris formats and the original enabled legacy boundary
gate remain open. Core review and full current-build qualification remain open.
Coverage is **1,009/1,158 (87.1%)**, 149 open and **92 more needed**. The separate
historical-operation count is 1,816/2,367. Zero whole rows were added.
