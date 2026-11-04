Rotating spacecraft-dependent plot frames
=========================================

Nine new GUI cases pass unattended and visibly. ObjectReferenced frames use
radial or velocity-aligned axes, Earth or spacecraft origins, forward and
backward propagation, and two spacecraft as frame references. Their numerical
providers have unequal ten- and three-second primitive steps. Both viewers'
sample membership and connections are checked. OrbitView positions and
velocities agree with independent prescribed circular trajectories and rotating
bases. Velocity includes the derivative of the coordinate transformation.

The first RotatingOriginForward mission aborted at a three-second endpoint:
``Object-frame acceleration requires the accepted spacecraft epoch.`` The
provider had already advanced farther. The failure and source-v1 are retained.

Correction and limits
---------------------

PlotOriginScope now indexes the spacecraft origin and ObjectReferenced primary
and secondary spacecraft by exact epoch and runtime identity. Conversion reads
their coeval Cartesian snapshots without modifying mission spacecraft. Missing
or conflicting reference states remain unavailable. Intentionally skipped
samples consume cadence without introducing a missing-data break.

PointMassForce exposes its existing Cartesian acceleration calculation for an
explicit state and epoch. Its ordinary spacecraft entrypoint passes the same
original inputs to the extracted calculation. ODEModel retains its provider
identity, ownership, initialization, origin, recursion and finite-output guards.
For scoped plot samples it evaluates point-mass forces at the sampled endpoint.
Other force types may use the live spacecraft only when its precise epoch and
all six Cartesian components match the snapshot exactly. Otherwise they raise
an explicit unavailable-input diagnostic; mass, attitude or hardware inputs are
not silently borrowed from a later state.

The final context probe verifies that a coeval generic force retains its native
spacecraft route, whereas stale-epoch and same-epoch/different-state snapshots
reject that force without calling it, changing the result argument, or modifying
the live spacecraft state or epoch. This is bounded point-mass support, not a
general sampled acceleration or attitude implementation. Additional force laws,
ephemeris-derived frame accelerations and other spacecraft-dependent axes remain
open. No original assertion or qualification gate was weakened.

Evidence
--------

* Nine new cases pass at source-v2 in both modes. They are RotatingOriginForward,
  RotatingOriginBackward, RotatingEarthForward, RotatingEarthBackward,
  RotatingVelocityForward, RotatingVelocityBackward, RotatingPairForward,
  RotatingPairBackward and RotatingPairThinned.
* Six affected original origin cases pass both modes at source-v2. Their saved
  outputs match the earlier origin stage exactly, including velocities, sampled
  epochs and connections. The original scope-isolation assertions also pass.
* RotatingOriginForward passes both modes at source-v3 with the final coeval
  generic-force correction and its three new context assertions. The other
  source-v2 cases are not represented as full-current-build replays.
* ObjectAccelerationDerivativeContract, AccelerationContextForce,
  AccelerationContextLifetime and AccelerationHandoff pass on their registered
  support routes at source-v3, with their original test sources unchanged.

The independent offline audit checks 496 epoch/connection pairs, 1,416 Cartesian
components, 122 exactly zero self-relative states, eight missing-reference
warnings, twelve matched regression outputs and six force-scope checks across
the two final modes. It independently differentiates geometric plotted positions
to check velocity. Six altered-proof controls reject changed positions,
velocities, epochs, invented samples, missing-reference bridges and nonzero
self-relative states. Visible receipts record four native xcb exposures on
initially iconic, resizable, non-fullscreen nested desktops; no hardware GPU or
human-observation claim follows.

Failures and environment
------------------------

The first build failed because CMake still referenced a removed migration
checkout. The cache preimage is retained. Reconfiguration uses the available
``/home/dan/GIT/gmat-build/gmat-git/depends/cspice/linux/cspice64`` and system
OpenSceneGraph. Dependency hashes and the resulting rebuild log are retained;
no equality to the earlier runtime dependencies is claimed.

One unattended six-case origin regression invocation segfaulted after its third
completed case. Its parent receipt remains failed. The same six cases completed
under gdb, in the visible matrix and in one subsequent unattended retry. The
cause remains unresolved; no crash fix or general stability qualification is
claimed. The failed mission, failed regression invocation and initial failed
build all remain in ``failures.json`` with their logs and source versions.

Canonical stage: ``build/nasa-coverage/coverage95-20261102-plot-step-rotating``.
It binds 437 artifacts, production preimages and exact patches, three source
versions, support-test source/binary identities, native logs/proofs/JUnit and
the four original row fingerprints. About 102.6 MiB of bound regular files is
retained, including referenced prior evidence; there are zero PNG or runtime
ELF copies. No earlier sealed artifacts were changed.

Replay only new or affected work
-------------------------------

.. code-block:: sh

   cmake --build build/linux-gui --target GmatQtPlotIntegrationStepTests -j4
   cases=RotatingOriginForward,RotatingOriginBackward,RotatingEarthForward,RotatingEarthBackward,RotatingVelocityForward,RotatingVelocityBackward,RotatingPairForward,RotatingPairBackward,RotatingPairThinned
   python3 src/qtgui/tests/RunPlotIntegrationStepAcceptance.py --cases "$cases" --pause-ms 0
   python3 src/qtgui/tests/RunPlotIntegrationStepAcceptance.py --cases "$cases" --visible --pause-ms 150

Use fresh output directories. The cases belong to the existing
``QtGui.PlotIntegrationSteps`` registration and acceptance build targets.
Verify retained evidence without replay:

.. code-block:: sh

   stage=build/nasa-coverage/coverage95-20261102-plot-step-rotating
   python3 "$stage"/AuditPlotStepRotatingEvidence.py --manifest "$stage"/evidence-manifest.json --output /tmp/plot-step-rotating-audit.json

Coverage remains **1,009/1,158 (87.1%)**, 149 open, **92 more needed**.
FRR!E329/E330/E633/E634 receive zero whole-row credit. General sampled
force/attitude inputs and remaining solver lifecycle/render conjunctions remain
open, along with all inherited clauses and the 59 prerequisite holds.
Historical operations remain 1,816/2,367. Scientific review, core approval,
full-current-build and full-NASA qualification remain open. The 95% objective
is active and incomplete.
