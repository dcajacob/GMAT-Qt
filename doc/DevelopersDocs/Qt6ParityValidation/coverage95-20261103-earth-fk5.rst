Earth FK5 orientation and daily EOP boundary acceptance
======================================================

The source-versioned union advances to **1,018/1,158 (87.9%)**: 140 original
requirements remain open, with 83 more needed for 1,101. Historical operations
remain **1,816/2,367 (76.7%)**. The authoritative ledger is
``build/nasa-coverage/coverage95-20261103-earth-fk5/coverage-snapshot-v2.json``.
The full goal remains active. All 59 prerequisite holds and scientific/core,
full-current-build and full-NASA qualification gates remain open.

Exact original obligation
-------------------------

**FRR!E209**, “Earth uses FK5 orientation model”, is admitted. The actual Earth
panel displays read-only ``FK5IAU1980``; actual coordinate controls select
Earth-origin BodyFixed axes. The nutation update interval is edited from 60 to
0 seconds through the Earth panel, with pending/native isolation, Apply,
Undo/Redo and Unicode save/reopen. The numerical witness uses zero interval to
compute the reduction at each epoch. The actual report viewer shows the checked
raw results, and following commands and unrelated source are preserved.

This is the complete Earth-model exception, not the all-body parent **E208** or
all-dynamics **E144**. The unresolved ICRF table-generation/convention question
from the earlier FK5 investigation is unchanged: this new reference compares
MJ2000Eq directly with EarthFixed and does not consume that table. Exact source
text/fingerprint and the admission decision are in ``requirement-review-v1.json``.

Independent reference
---------------------

The oracle uses pinned PyERFA 2.0.1.5, ERFA 2.0.1 and SOFA 20231011 heritage.
Primary `ERFA v2.0.1 routines and validation constants
<https://github.com/liberfa/erfa/tree/v2.0.1>`_ are retained with their license,
source URLs and hashes. ``pnm80`` supplies the IAU 1976/1980 precession-nutation
matrix; ``gmst82`` supplies UT1 sidereal time. All **106 installed nutation terms**
match the independent official coefficient table exactly.

NASA's shipped ``MathematicalSpecification/CoordinateSystems.tex`` supplies the
explicit conventions: omit the two complementary equinox terms on/before
JD 2450449.5, use its stated polar-motion rotation order without an IAU2000 TIO
locator, and retain only the sidereal contribution to the state derivative,
with the stated LOD correction. The reference implements these documented
conventions; it does not substitute a modern terrestrial model or a derivative
of all slowly changing rotations. Daily IERS EOP values are shared data inputs,
with independent interpolation of polar motion and continuous UT1-minus-TAI
across leap intervals; LOD is the left daily value.

Fifteen epochs include pre/post-1997, two leap-second boundaries, fractional
ordinary days, daily EOP boundaries, out-of-order dates and repetition. Each has
an asymmetric mixed state and all six position/velocity basis states. Two phases
in each of unattended and visible execution give **420 state vectors**. Maximum
errors are **2.072e-8 km** (0.02072 mm) and **2.072e-11 km/s**
(0.02072 micrometres/s). The predeclared limits remain **1e-6 km**, **2e-10 km/s**
and **1e-11 days**; no fitted corrections or relaxed numerical assertions.

Twelve independent reference contracts cover published matrices/time constants,
leap continuity, EOP interpolation/domain, rotation/rate algebra, distinguishable
wrong clock/polar motion, historical equinox policy and sub-microsecond midnight
precision. A large second Julian-date part initially rounded a just-before-
midnight input onto the next day. Splitting at midnight and retaining calendar
day/fraction fixes the reference representation. A Decimal check establishes
the actual side of the original floating-point input without consulting GMAT.
The failed native v3 run stays failed; later cases use the corrected reference.

The model fidelity evidence is not independent physical astrometry, forecasting
accuracy, full frame/dynamics qualification or scientific/core approval.

Demonstrated core defect and repair
----------------------------------

A separate direct native test exposed a real EOP cache defect, independent of
the reference precision issue. At an exact daily knot, the cached branch used
the previous day's LOD, while a cold lookup used the new day. Five of eighteen
checks failed across forward/reverse/out-of-order queries. The native test uses
a prescribed three-day EOP file and exact integer UTC MJD inputs.

``src/gmatutil/util/EopFile.cpp`` now makes the cache interval half-open at the
right boundary. Exact next-day queries reach the ordinary lookup and return the
same daily LOD regardless of history. Existing linear x/y interpolation, daily
LOD policy and before/after-table clamping remain intact. All **18 unchanged
checks pass**. ``EopFile.before.cpp``, ``EopFile-v5.patch``, before/after image
identities and JUnit receipts retain the exact evidence. This is the sole
production source change in this increment; scientific/core approval stays open.

Native runs, audit and retention
-------------------------------

The new ``QtGui.NasaEarthFk5Orientation`` case passes unattended and visibly at
source-v5. Three affected regressions pass in both modes:
``QtGui.NasaHarmonicGravityEarth``, ``QtGui.NasaStationCartesianEllipsoid`` and
``QtGui.NasaMsise86Baseline``. This gives eight passing GUI executions and 100 visible native exposures. The
visible desktop starts minimized, remains resizable/non-fullscreen and retains
zero screenshots. Before/after hashes and four process maps bind the executed
source and runtime images; no human-observation or physical-GPU claim is made.

``AuditEarthFk5Acceptance.py`` regenerates the independent reference from frozen
sources/data, verifies package identity, compares raw CSV/proof/stdout/JUnit,
checks visible exposures and exact core before/after evidence, and rejects six
corrupt proofs: position, velocity, time, inertial input, missing basis and
missing reopen phase. The cumulative audit revalidates the 17 inherited
qualifications plus this exact obligation without replaying their GUI missions.

Retained failures include the first equinox negative-control date, the fixture's
shortened FK5 label, the reference's midnight precision loss and the five native
EOP cache failures. Qualification v1 also remains as a failed audit: CTest
truncated successful support output at 1,024 bytes. One unchanged support-test
replay with a larger output retention limit supplies the complete eighteen-check
receipt used by qualification v2. No GUI replay or assertion change was needed.

Regular artifacts exclude live runtime aliases. No copied runtime binaries or
large ephemeris files are introduced. ``stage-receipt-v2.json`` records storage
and the exact 59 prerequisite holds, all still present among the remaining rows.

Replay and reference prerequisites
---------------------------------

Use the existing configured Linux build::

   cmake --build build/linux-gui --target GmatQtNasaEarthFk5Tests GmatEopDailyKnotTests -j4
   ctest --test-dir build/linux-gui -R '^QtAcceptance.EopDailyKnot$' --output-on-failure --test-output-size-passed 100000
   python3 src/qtgui/tests/RunEarthFk5Acceptance.py
   python3 src/qtgui/tests/RunEarthFk5Acceptance.py --visible

``--with-regressions`` adds only the three affected cases when justified by new
changes. Build ``GmatQtNasaHarmonicGravityTests``,
``GmatQtNasaStationCoordinateTests`` and ``GmatQtNasaMsise86Tests`` first; the last
requires the optional MSISE86 plugin. The native fixture consumes the frozen
JSON reference and does not require PyERFA to run.

To regenerate/audit reference evidence, restore the pinned reference environment
if a temporary-directory cleanup removes it::

   python3 -m venv --system-site-packages /tmp/gmat-earth-fk5-reference-env
   /tmp/gmat-earth-fk5-reference-env/bin/python -m pip install -r src/qtgui/tests/Fk5StandardsOracle.requirements.txt
   /tmp/gmat-earth-fk5-reference-env/bin/python src/qtgui/tests/EarthFk5OracleTests.py -v
   python3 src/qtgui/tests/AuditEarthFk5Acceptance.py \
     --qualification build/nasa-coverage/coverage95-20261103-earth-fk5/qualification-v2.json \
     --output /tmp/earth-fk5-audit.json

The qualification checks the retained reference package file hashes; a changed
package requires explicit reconciliation rather than silent substitution. The
recorded environment uses Python 3.14 and NumPy 2.3.5. No GMAT runtime is copied.
``rebuild-ledger-inputs-v2.json`` lists the exact sealed baseline/qualifications;
all 140 remaining source obligations and gaps are in the cumulative snapshot.
