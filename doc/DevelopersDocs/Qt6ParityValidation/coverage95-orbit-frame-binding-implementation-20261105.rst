Orbit conversion frame bindings, 2026-11-05
==========================================

The FRR!E13 implementation review covered the worksheet's six orbit-state
forms and coordinate-system conditions in FRR!E5 through E13, read directly
from ``doc/SystemDocs/Requirements/GMAT Requirements.xlsx``. The worksheet
requires inertial celestial-body frames for the three gravity-based forms and
general supported frames for Cartesian and the two spherical forms.

Owning-source routes
--------------------

StateConversionUtil has source dispatch for all fourteen advertised native
representations: Cartesian, Keplerian, ModifiedKeplerian, SphericalAZFPA,
SphericalRADEC, Equinoctial, ModifiedEquinoctial, AlternateEquinoctial, Delaunay,
Planetodetic, IncomingAsymptote, OutgoingAsymptote, BrouwerMeanShort and
BrouwerMeanLong. Each nonidentity destination has a direct or Cartesian-mediated
route. The preceding Delaunay anomaly repair fixes the angle convention at that
helper boundary. Existing representation domains and singularity errors remain;
no invalid-state or all-planet Brouwer capability is invented.

Spacecraft binds input frames, applies the celestial-body preconditions and
converts representations through Cartesian state at the spacecraft epoch.
OrbitData converts parameter-frame Cartesian state before selecting an orbit
representation and uses the inverse path for element writes. The Qt orbit
editor submits a complete six-element state, binds the selected frame, filters
body/fixed-frame representation choices and preserves Cartesian state across
display-only changes. Native binding rejects spacecraft-dependent circular
frames. This is an implementation inventory, not numerical proof of these paths.

Repairs completed in this batch
-------------------------------

* Spacecraft frame binding resolves the new origin before mutation, clears body
  gravity/shape constants for calculated-point origins, and restores the frame
  pointer plus all cached origin fields when applying the binding throws.
  Unresolved origins produce an error before mutation instead of a null access.
* SetStateFromRepresentation restores the prior Cartesian state and retained
  origin-relative cache if the later cache conversion or ephemeris evaluation
  throws. Conversion temporaries are local to the call.
* OrbitData reinitialization derives gravity and shape from the newly resolved
  origin for both origin-dependent and frame-dependent parameters. Calculated
  points no longer inherit constants from a previously bound body.
* Parameter frame rotations retain a publication sample's GmatTime stamp or a
  precise spacecraft's live GmatTime. The ordinary Real-epoch path remains.
* AlternateEquinoctial now advertises its required celestial-body origin, so
  native preconditions and both frontend choice filters agree with its use of
  the body's gravitational constant.

No exported signature, class layout, formula, numerical tolerance, fixture or
assertion changes. The binding rollback does not claim a transaction over
independent thruster/attitude reference updates in SetRefObject.

Build and campaign disposition
-------------------------------

Stage: ``build/nasa-coverage/coverage95-20261105-implementation-orbit-frame-binding``.
All four private configured builds passed: v1, 69 steps/7.29 seconds; v2,
66 steps/3.10 seconds; v3, 66 steps/7.21 seconds; v4, 66 steps/3.19 seconds.
Sessions 34642, 51023, 30912 and 51624 are terminal. Exact source hashes and
patches are retained there. Compilation is the only execution performed; test/probe clients
were only linked. No test, GUI, numerical matrix, model, acceptance reader or
seal was run. Installed/shared runtimes and packages were not updated.

FRR!E13 is now classified as implementation present in the engineering worklist.
This adds one implementation row, making the provisional tally 1083/1158
(93.52%): thirty additional original rows, twenty-five new/compatibility and
five existing implementations. It is not a completed implementation audit of
the remaining worklist. Recorded verification stays 1053/1158 (90.9%); all 105
original acceptance rows and qualification gates remain open and unchanged.

Later verification must cover the complete representation/frame scope, native
and Qt entry points, body/calculated-point rebinding, rejection rollback,
parameter reinitialization, precise sample/live epochs, Delaunay anomaly
selectors, singular/domain errors and repeated display changes. Compilation
does not establish numerical accuracy or GUI behavior.
