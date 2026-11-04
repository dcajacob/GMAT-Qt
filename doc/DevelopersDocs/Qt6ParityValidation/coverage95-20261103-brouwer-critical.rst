Critical Brouwer longitude repair
=================================

Coverage remains **1,019/1,158 (88.0%)**, with 139 original requirements open
and **82 more needed** for 95 percent. Historical operations remain
**1,816/2,367 (76.7%)**. This is a demonstrated numerical repair with bounded
evidence, not additional whole-row credit. FRR!E13, all 59 prerequisite holds
and scientific/core/current-build/full-NASA gates remain open.

Defect and exact change
-----------------------

The previous direct-conversion matrix had one failing native context:
SphericalAZFPA to BrouwerMeanLong at osculating inclination 64 degrees. The
current StateConversionUtil source matched the retained failing version exactly;
that existing failure was reused rather than needlessly replayed before diagnosis.

A new independent symmetry contract exposed the mechanism. A zonal model must
rotate its Cartesian output by the same angle when its mean ascending node is
rotated about the pole. Eighteen checks use six inclinations and three node
rotations. Six fail at 63.2 and 64 degrees, with position errors up to about
13,250 km and velocity errors up to 7.54 km/s. The four other inclinations retain
rotation covariance. Expected values use an elementary Cartesian rotation,
without a second copy of the Brouwer formulas.

The critical branch in ``BrouwerMeanLongToOsculatingElements`` set ``blghp`` to
zero. Subsequent reconstruction uses it as total mean longitude, so this erased
the base longitude as well as suppressing the singular correction. It now uses
``raandp + aopdp + meanAnom``. The **0.001 branch threshold**, all remaining
primitive formulas, inverse algorithm, warnings, validity bounds and earlier
pending fixes are unchanged. This is the only production change in this
increment. ``StateConversionUtil.before.cpp`` and ``core-v2.patch`` preserve
its exact preimage and delta.

All **18 unchanged symmetry checks pass** after the correction. The original
native direct test now passes **238/238 directed contexts**, its 22 same-type
identities and unknown-type rejections, without changing any fixture, tolerance
or assertion. All 237 previously passing native contexts still pass.

An unresolved independent failure remains
-----------------------------------------

The original ``QtAcceptance.NasaDirectOrbitConversion`` wrapper still **fails**:
its separate NASA prototype reconstruction disagrees for that same critical
context. The MATLAB prototype uses a 0.1 threshold, whereas the native model
uses 0.001. Its critical branch also erases base longitude. The new native inverse
now reconstructs the original Cartesian target, but the historical prototype
maps its returned mean values to a different state (about 135 km maximum position
difference). The original wrapper, prototype, assertions and failure status are
preserved; no expected-failure inversion or relaxed bound is introduced.

The retained source-parity corpus has no valid long-mean samples in the changed
native branch. Its exact branch review is retained, so those established tests
were not replayed. This repair establishes axis symmetry and native inverse
consistency; it does not resolve historical model identity, independent physical
averaging, the full critical domain, or all-representation/all-frame conversion
qualification. Broader FRR!E13 credit remains unavailable.

New GUI evidence
----------------

Two new cases start with independently calculated Cartesian states for literal
Keplerian inputs at 63.2 and 64 degrees. The actual spacecraft editor selects
BrouwerMeanLong, exposes all six mean fields, and edits the node by 90 degrees.
Pending source isolation, parent Apply, Undo/Redo, Unicode save/reopen, mission
continuation and the actual read-only report viewer are checked. The expected
Cartesian result is an independent 90-degree rotation of the original state;
no native inverse result supplies the expected Cartesian values.

Both cases pass unattended and visibly at source-v5, with two persistence phases
per execution: **four GUI passes and eight raw Cartesian states**. Maximum
errors are **1.274e-11 km** and **6.218e-15 km/s**, within the original direct
matrix limits of 1e-6 km and 1e-9 km/s. Visible windows start minimized and remain
resizable/non-fullscreen; zero screenshots and no runtime copies are retained.

The first GUI source-v3 runs both failed before reaching the orbit editor because
the new registration used the offscreen runner while the fixture requires a
native widget platform. Only the registration was repaired. A source-v4 draft
with incorrect native-runner arguments was caught before execution; source-v5
uses the established Xvfb/native command structure. All versions and failures
remain retained. The fixture's assertions never changed.

Audit and replay
----------------

``AuditBrouwerCriticalRepair.py`` binds regular artifacts, preimage/delta,
unchanged before/after assertions, direct native and prototype results, raw GUI
reports, independent Cartesian geometry, stdout/JUnit, visible exposures, exact
executed sources and process-mapped runtime images. Three corrupt-proof controls
reject a wrong state, missing reopen and wrong node. Its explicit result is
``passed-bounded-repair`` with an empty qualified-key list. It does not enter the
cumulative requirement union.

Evidence lives in ``build/nasa-coverage/coverage95-20261103-brouwer-critical``.
The canonical coverage ledger remains
``build/nasa-coverage/coverage95-20261103-default-orientation/coverage-snapshot-v1.json``.
Reuse the source-v5 GUI passes. New/affected replay commands are::

   cmake --build build/linux-gui --target GmatBrouwerRotationCovarianceTests \
     GmatNasaDirectOrbitConversionTests GmatQtNasaBrouwerCriticalRotationTests -j4
   ctest --test-dir build/linux-gui -R '^QtAcceptance.BrouwerRotationCovariance$' \
     --output-on-failure --test-output-size-passed 100000
   python3 src/qtgui/tests/RunBrouwerCriticalAcceptance.py
   python3 src/qtgui/tests/RunBrouwerCriticalAcceptance.py --visible
   python3 src/qtgui/tests/AuditBrouwerCriticalRepair.py \
     --stage build/nasa-coverage/coverage95-20261103-brouwer-critical \
     --output /tmp/brouwer-critical-audit.json

The new math and GUI targets are dependencies of acceptance build/check targets.
The original direct wrapper's prototype mismatch remains an ordinary enabled
failing test. No unrelated pending work is removed. Two existing trailing-space
lines in the broader StateConversionUtil working-tree diff are unchanged from
the retained preimage; this increment's exact core patch has no such whitespace.
