Spacecraft-origin MJ2000Eq plot frames
===================================

The first native test plotted a spacecraft relative to itself at its own
three-second endpoint. OrbitView showed a position error of 46.584902 km and
a velocity error of 0.034447 km/s. The frame conversion had read the spacecraft
at the end of its provider's longer synchronization interval. The corrected
forward and backward tests measure exactly zero for every component of the
origin spacecraft's state relative to itself.

For explicit step sampling in a spacecraft-origin MJ2000Eq view, OrbitPlot
indexes the origin's committed snapshots by exact epoch once per publication.
A synchronous read scope supplies the corresponding Cartesian state during
frame conversion. If the spacecraft is outside the packet, its live state may
be captured only when its own clock matches the requested epoch. Conflicting
states at one epoch are unavailable. The scope checks runtime object identity,
query epoch and the active publication. Nested raw publication cannot inherit
it, and unwinding restores the outer scope. Mission spacecraft are never
modified to draw a point.

When a selected endpoint lacks a coherent origin state, OrbitView omits it,
reports the gap, and disconnects the next valid point on that trajectory. An
endpoint deliberately skipped by cadence does not create a gap. A new arc
reports its first missing reference even if its first point already required a
line break. This rule leaves interpolation between independent providers open.

The existing per-spacecraft selection metadata now also distinguishes objects
represented elsewhere in a committed packet. The native Qt receivers use the
individual arc breaks in that metadata when independent provider records arrive
out of chronological order. This keeps continuous forward and backward curves
connected in both viewers and preserves a real missing-data break.

Verified cases
--------------

* FrameFollowerForward and FrameFollowerBackward use a ten-second leader and a
  three-second follower, stopping at ±23 seconds. Both plots retain every
  committed point. OrbitView's six components match the difference of two
  independently computed circular states at the same time.
* FrameLeaderMissing uses the leader as the origin. The follower can be drawn
  at 0, 10, 20 and 23 seconds. It breaks across the unavailable interior epochs
  and connects 20 to 23 seconds. GroundTrack retains the full follower grid.
* FrameLeaderThinned selects only coeval follower points at 0, 10 and 20 seconds.
  They stay connected, with no missing-origin warning. The leader need not be
  plotted at an epoch for its committed state to serve as the origin.
* FrameLiveOrigin uses an unpropagated reference spacecraft. Its live clock
  matches the initial epoch only. OrbitView shows just that valid point while
  GroundTrack and the mission continue through 23 seconds.
* FrameNewArcMissing advances the reference independently, then reassigns the
  other spacecraft at its old epoch. The newly started arc has no matching
  origin and reports the missing sample. The earlier valid points remain.

All six pass unattended and visibly at source-v8. The affected ResetPeer and
TranslatedOrigin regressions also pass both ways and their saved outputs match
their respective sealed earlier versions. The unchanged OrbitDataAvailability
regression passes on its registered headless route.

Evidence and replay
-------------------

``build/nasa-coverage/coverage95-20261102-plot-step-origin`` contains the
471-artifact manifest, exact production preimages/diffs, source archives and
the failure log. The two original failures demonstrate the wrong position and
invented interior samples. A subsequent case exposed a spurious line break.
The failure log also records the missing-include build error, intermediate
edit/build state, two fixture errors involving cumulative messages and test
family selection, and the missing first-warning defect. Every failed native
invocation retains its failed status.

The independent audit checks 222 epoch/connection pairs, 600 relative Cartesian
components, 64 self-relative zero states, eight gap diagnostics and four
matched regression outputs. Visible runs recorded two native X11 exposure
events on the initially iconic, resizable, non-fullscreen nested desktop.
Six altered proof copies were rejected. Lossless compression saved 88,231,288
bytes. About 39.1 MiB of regular stage and invocation evidence is retained with
zero PNG or runtime ELF copies. Use a fresh directory to replay.

.. code-block:: bash

   python3 src/qtgui/tests/RunPlotIntegrationStepAcceptance.py --cases FrameFollowerForward,FrameFollowerBackward,FrameLeaderMissing,FrameLeaderThinned,FrameLiveOrigin,FrameNewArcMissing --visible --pause-ms 150 --output build/gui-acceptance/plot-step-origin-local-replay
   python3 src/qtgui/tests/AuditPlotStepOriginEvidence.py --manifest build/nasa-coverage/coverage95-20261102-plot-step-origin/evidence-manifest.json --output /tmp/plot-step-origin-audit.json

FRR!E330/E634 gain evidence for actual spacecraft-origin translation and
coherent or unavailable provider epochs. FRR!E329/E633 still inherit open
sampling children and solver redraw/retention/rendering clauses. Rotating axes,
spacecraft-dependent acceleration and attitude, additional provider domains and
the 59 prerequisite holds remain open. The current scope uses Linux xcb and
MJ2000Eq axes. Scientific/core and full-current-build qualification remain open.
Coverage is **1,009/1,158 (87.1%)**, 149 open, **92 more needed**. Historical
coverage is 1,816/2,367. Zero whole rows were admitted.
