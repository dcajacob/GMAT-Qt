Declared-frame attitude acceptance evidence
==========================================

All **17 new fixed-attitude frame cases pass unattended and visibly**.
The independent audit checks 102 raw report rows: 918 orientation components,
306 angular-rate components and 612 inertial-state components, plus epochs
and sentinels. Twenty-three incomplete or corrupted capsules are rejected.

Requirement coverage remains **998/1,158 (86.2%)**; **103 complete original
requirements** are still needed for 95 percent. Historical coverage remains
**1,816/2,367 (76.7%)**. FRR!E52 remains unqualified for the source question
below. New test counts are not substituted for requirement credit.

What was added
--------------

``NasaAttitudeDeclaredFrameTests.cpp`` and its JSON fixture exercise the actual
Spacecraft attitude editor, model/reference/sequence popups, inactive inputs,
pending isolation, discard, Apply, exact Undo/Redo and complete native-graph
identity. Unicode save/reopen, unchanged mission and unrelated resource source,
native attitude/frame/origin binding, three-epoch raw results and the complete
read-only output viewer are checked.

The new cells cover MOEEq/MOEEc, TOEEq/TOEEc, MODEq/MODEc, TODEq/TODEc,
BodyInertial Earth/Mars/user-body branches, GSE/GSM, Topocentric and spacecraft,
barycenter and libration-point origins. The original seven attitude tests and
their fixture are unchanged. Their existing inertial/ecliptic/body/orbital
evidence is retained; none was replayed in this increment.

Expected orientations reuse the independently checked CSPICE and geometric
frame-oracle records from the named-frame campaign. Their original source,
registration, raw input and receipt bindings are rechecked. Angular rates are
derived independently from the basis derivatives using the skew product.
The NASA coordinate-system math specification explicitly neglects the small
of-date rotation derivatives; those cases preserve that approximation.
The original 5e-9 deg/s rate bound is unchanged. Each new orientation bound
matches its retained independent frame-oracle bound. The older seven attitude
tests keep their original 2e-10 orientation bound. Shared native time/ephemeris
inputs do not become independently qualified astronomical truth.

Ten passing unattended cases were reused after the initial runs. Only seven
failed cases were rerun following the fixture repair; all 17 new cases then
received their first visible execution. Windows were initially minimized,
resizable and non-fullscreen. No automatic passing screenshots were saved.

Failures and core scrutiny
--------------------------

Seven initial new cells failed the exact Undo/native-graph assertion. One
additional diagnostic rerun retained the complete before/after graphs and
unchanged source. The captured difference is solely the automatically assigned
NAIF ID and reference-frame ID of the fixture's secondary ``Vehicle`` spacecraft.
Rebuilding creates fresh default IDs. Assigning explicit IDs to the fixture
spacecraft repairs all seven cases while preserving every assertion.
All eight failed contexts and all three successful build logs remain retained.

There are **no new product or core changes**. All 104 tracked pending core
paths retain their sixth-increment identities; the prior interpreter and Qt
repairs remain unchanged. Every original ``require`` call in the new test's
first executed version is preserved, and the original seven attitude test and
fixture bytes are unchanged. Exact source versions for passing pilots and the
later diagnostic/ID changes are retained separately.

Lossless compression of 109 duplicate reports/traces recovered 84,209,664 bytes
(about 80.3 MiB). Raw numeric output, scripts, native proofs, logs/JUnit and
source capsules remain. No runtime binary copies were created. A normal new
test executable was built, as required to execute the new registrations.

Why the original row stays open
-------------------------------

FRR!E52 says ``Fixed offset with respect to any coordinate system defined in
FRR-35.`` The repository contains differing NASA definitions:

* ``Attitude.tex`` describes a separately supplied constant body-to-reference
  rotation, with ``R_BI = R_Bi * R_iI``.
* ``AttitudeD5.tex`` and the shipped ``Resource_SpacecraftAttitude.xml`` guide
  describe alignment with the selected frame, with ``R_BI = R_iI``; the guide
  explicitly says initial orientation and rate settings have no effect.

The new checks establish the current guide model's additional frame mechanisms.
They do not demonstrate a configurable arbitrary nonzero offset relative to an
arbitrary selected frame. Neither draft is silently chosen to narrow the
historical obligation. Authoritative reconciliation or an explicitly scoped
broader implementation is still required for whole-row credit. The source
question has been presented to the user; pending clarification, this row stays
open. No original denominator, fingerprint or qualification gate changed.

Reusable automation and evidence
--------------------------------

Build the new registered tests::

   cmake --build build/linux-gui --target GmatQtNasaAttitudeDeclaredFrameTests -j 4

Watch the new cases from a terminal in the logged-in desktop::

   python src/qtgui/tests/RunAcceptance.py --build-dir build/linux-gui \
     --filter '^QtGui.NativeNasaAttitudeDeclaredFrame' \
     --visible --display-route nested --display-scale auto --start-minimized \
     --pause-ms 350 --max-checkpoints 200 --max-screenshots 0 --report-every 30 \
     --nasa-review build/nasa-coverage/coverage95-20261101/attitude-declared-runtime-review.json \
     --output build/gui-acceptance/attitude-declared-watch-NEW

Use a fresh output directory, then restore the minimized test desktop to watch.
For unattended execution, omit the visible/display/pause options. The exact
17 CTest registrations are also part of the existing acceptance build targets.

Recheck the retained evidence without running a GUI mission::

   python src/qtgui/tests/AuditAttitudeDeclaredFrameEvidence.py \
     --evidence build/nasa-coverage/coverage95-20261101/attitude-declared-evidence.json

The campaign directory retains the independent/negative audits, complete
source/runtime input identities, source-only deltas, failed attempts, exact
CMake delta, ``attitude-declared-change-and-failure-review.json`` and
``attitude-declared-original-scope-review.json``. No new qualification is added
to the canonical requirement union. Scientific/core approval, full current-build
and full NASA qualification remain open.
