All default-body orientation acceptance
=======================================

The source-versioned union advances to **1,019/1,158 (88.0%)**: 139 original
requirements remain open, with 82 more needed for 1,101. Historical operations
remain **1,816/2,367 (76.7%)**. The authoritative ledger is
``build/nasa-coverage/coverage95-20261103-default-orientation/coverage-snapshot-v1.json``.
The full goal remains active. All 59 prerequisite holds and scientific/core,
full-current-build and full-NASA qualification gates remain open.

Exact original obligation
-------------------------

**FRR!E208**, the IAU2002 orientation parent for all default celestial bodies
with its stated exceptions, is admitted as a complete conjunction. Nine new
models cover Sun, Mercury, Venus, Mars, Jupiter, Saturn, Uranus, Neptune and
Pluto. The actual native default-body census is exactly those nine plus Earth
and Luna. The separate complete Earth FK5 proof E209 and Moon DE405/IAU2002
proofs E210/E211/E212 are re-audited at their recorded source versions.
Their GUI missions are not replayed or attributed to a newly qualified whole
binary. No other requirement gains credit from this increment.

The nine new cases inspect actual read-only model and six orientation controls,
compare native getters with independent historical constants, select BodyFixed
through the nested axes chooser and parent Apply, and check Undo/Redo, Unicode
save/reopen, unchanged unrelated source and following mission commands. Both
phases compare the actual report viewer with the complete raw report.

Independent reference and scope
-------------------------------

``DefaultBodyOrientationOracle.py`` parses the exact NASA historical
``pck00008.tpc`` retained in the lunar qualification. That qualification verifies
the IAU/IAG2000 report's 2002 publication identity. No modern model or current
engine-generated table supplies the expected values. All nine sets of constants
and Neptune's periodic terms come from the pinned historical PCK.

NASA's ``MathematicalSpecification/CoordinateSystems.tex`` specifies the default
MJ2000Eq orientation and its velocity approximation: hold the pole factors fixed
and differentiate the prime-meridian angle. The oracle preserves that convention;
it does not substitute full pole derivatives. The full analytic angle rates are
retained for diagnostic use, and independent finite differences verify Wdot.
These are model-fidelity checks, not physical astrometry, forecasting accuracy,
scientific approval or a general frame/dynamics qualification.

Each model has twelve epoch entries, including subsecond, fractional, distant,
out-of-order and repeated dates within the installed DE405 domain. Each entry
has an asymmetric mixed state and six position/velocity basis states. Two phases
in two modes yield **3,024 new Cartesian state vectors**. Maximum errors are
**2.274e-13 km** and **4.441e-16 km/s**. Original predeclared bounds remain
**5e-6 km**, **5e-10 km/s**, and **2e-10 TDB days**. Neither the native assertions
nor reference JSON changed between the failed source-v4 and passing source-v5.

Demonstrated defects and repairs
-------------------------------

The first complete unattended source-v4 run passed four bodies and failed Mars,
Jupiter, Saturn, Uranus and Neptune against the original velocity limit. Full
phase-zero raw matrices survive even for failed cases. A separate diagnostic
reconstructs all observed outputs to floating-point precision using the old
clock arithmetic and Neptune rate. That diagnostic is not expected truth and
is not substituted into the admission oracle.

Two production expressions change:

* ``CelestialBody::GetJulianDaysFromTDBEpoch`` subtracts the small J2000 offset
  directly instead of first adding a full Julian-date offset. This avoids losing
  roughly twenty microseconds at the subsecond witness.
* Neptune's periodic prime-meridian derivative uses
  ``Rad(52.316) / DAYS_PER_JULIAN_CENTURY`` for its sine-argument rate. The old
  0.0014323 degrees/day value omitted the radians conversion in differentiating
  ``sin(Rad(N))``. Wdot's output remains degrees/day.

The shared clock helper affects all nine default models, so all nine cases were
rerun. Earth FK5 and Moon DE405/SPICE reductions use their distinct previously
proved paths. BodyInertial consumes only the same RA/DEC outputs checked by this
complete basis matrix. This increment preserves the documented default-body
pole-rate approximation and earlier user-defined-body behavior.

``CelestialBody.before.cpp``, ``Planet.before.cpp`` and ``core-v5.patch`` retain
the exact two deltas against the preexisting dirty workspace, plus before/after
core-image identities. Earlier unrelated CelestialBody changes are preserved.
All five original native failures now pass with identical assertions. Technical
core and scientific review remain open.

Native runs, audit and retention
-------------------------------

All nine ``QtGui.NasaDefaultOrientation<Body>`` registrations pass unattended at
``default-orientation-headless-v5`` and visibly at
``default-orientation-visible-v1``. Visible windows start minimized, remain
resizable/non-fullscreen and retain zero screenshots. Nine process maps and
before/after hashes bind the executed sources and images. No human-observation
or physical-GPU claim follows.

``AuditDefaultBodyOrientationAcceptance.py`` regenerates the reference from
frozen Python/PCK inputs, checks the exact historical model identity through the
lunar qualifier, independently re-audits both exception qualifiers, checks the
11-body census and all 18 native results, and compares raw CSV/proof/stdout/JUnit.
It validates visible traces, loaded images, unchanged assertions and the exact
production deltas. Nine corrupt-proof controls reject wrong position, velocity,
epoch, inertial input, model, constants, missing basis, reopen phase and body.
The cumulative builder revalidates all 19 qualifications without legacy GUI
replays. The exact baseline and qualification list are in
``rebuild-ledger-inputs-v1.json``.

Retained development failures also include two C++ builds that used hidden
base-class getter overloads and the first oracle finite-difference check. The
latter suffered cancellation with a 10-second step at large unwrapped angles;
a 1,000-second step fixed numerical verification before native testing, while
retaining its original 3e-12 rad/s bound. No native result was fitted to that
change. All failed artifacts remain failed.

Artifact hashes bind regular files only, excluding every live runtime symlink.
No runtime binaries or large ephemerides are copied. ``stage-receipt-v1.json``
records storage, exposures, current source identities and all 59 unchanged
prerequisite holds. The all-dynamics E144 and ICRF table-reference discrepancy
remain open.

Replay
------

Use the configured Linux build::

   cmake --build build/linux-gui --target GmatQtNasaDefaultBodyOrientationTests -j4
   python3 src/qtgui/tests/RunDefaultBodyOrientationAcceptance.py
   python3 src/qtgui/tests/RunDefaultBodyOrientationAcceptance.py --visible
   python3 src/qtgui/tests/AuditDefaultBodyOrientationAcceptance.py \
     --qualification build/nasa-coverage/coverage95-20261103-default-orientation/qualification-v1.json \
     --output /tmp/default-orientation-audit.json

Native replay uses the frozen JSON and does not need a reference Python package.
The independent parent audit inherits the Earth qualifier's pinned
``/tmp/gmat-earth-fk5-reference-env/bin/python`` prerequisite. Restore its exact
package identity using the Earth FK5 report if temporary files are cleaned.
Reuse the recorded passes unless changed source or a new failure justifies
execution. All 139 open exact source obligations remain in the cumulative ledger.
