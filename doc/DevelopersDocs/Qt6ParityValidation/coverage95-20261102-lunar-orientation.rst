Moon DE405 and IAU2002 orientation acceptance
==========================================

The source-versioned union advances to **1,013/1,158 (87.5%)**: 145 original
requirements remain open, with 88 more needed for 1,101. Historical operations
remain **1,816/2,367 (76.7%)**. The authoritative ledger is
``build/nasa-coverage/coverage95-20261102-lunar-orientation/coverage-snapshot-v2.json``.
The goal remains active. All 59 prerequisite holds and scientific, core,
full-current-build and full-NASA qualification gates remain open.

Exact original obligations
--------------------------

* **FRR!E211**, DE405: the actual SolarSystem panel selects DE405 and its installed
  file; the actual Moon panel displays its DEFile source; the actual coordinate
  chooser selects BodyFixed. Independent coefficient decoding and Clenshaw
  evaluation predict the full Cartesian transformation and its velocity terms.
* **FRR!E212**, IAU2002: the actual lunar controls remove the default binary PCK,
  select NASA's historical ``pck00008.tpc``, choose SPICE and ``IAU_MOON``, and
  preserve that configuration through Apply, save/reopen and reset/reselection.
  A separately differentiated analytic periodic series predicts the results.
* **FRR!E210**, “Moon supports”: admitted only as the complete conjunction of
  those two children. The all-body parent **E208** and Earth/FK5 **E209** remain
  open. Original text and fingerprints are preserved in ``requirement-review.json``.

Model identity and independent results
--------------------------------------

NASA's `PCK release announcement
<https://naif.jpl.nasa.gov/pipermail/spice_announce/2004-September/000023.html>`_
identifies the IAU/IAG 2000 report published by Seidelmann et al. in **2002**.
This resolves the original IAU2002 wording by publication identity, while
preserving the original requirement. The `historical PCK
<https://naif.jpl.nasa.gov/pub/naif/generic_kernels/pck/a_old_versions/pck00008.tpc>`_
is used explicitly; the installed modern PCK is not credited as an equivalent.
The retained file SHA256 is
``ca07df56e9f1bca85a4c6748b618a46c0bf8e081f96f81e324896f2fdc54ea27``.

The DE405 binary header identifies version 405 and its libration layout.
All 360 lunar coefficients in three records match the official
`DE405 ASCII data <https://ssd.jpl.nasa.gov/ftp/eph/planets/ascii/de405/ascp2000.405>`_
and `header <https://ssd.jpl.nasa.gov/ftp/eph/planets/ascii/de405/header.405>`_
exactly. Only the small coefficient subset and download identity are retained;
no second ephemeris or runtime binary is copied.

The standard-library Python oracle calls no GMAT or SPICE orientation, clock,
or state helper. It also checks orthogonality and finite-difference derivatives.
Each model has 15 epoch witnesses, including DE record/subinterval boundaries,
out-of-order dates, broad dates and repetition. A mixed Cartesian state plus
all six basis states establishes the complete linear position/velocity mapping
at each epoch. Reports verify the specified TDB epoch and inertial inputs before
comparing the transformed state. DE405 uses its native MJ2000Eq base; the IAU
PCK uses ICRF. No unproved equivalence of those reference frames is assumed.

The installed ICRF conversion table limits that spacecraft route to roughly
1957–2100. TDB MJD 70000.5 is retained as an explicit out-of-range rejection and
recovery witness; the positive IAU broad-date case uses 57000.5. DE405 retains
70000.5. These tests establish model fidelity in the supported domain, not
modern physical orientation accuracy or unbounded date support.

Execution and retained failures
--------------------------------

Both registrations pass unattended and visibly at source-v5. The two DE405
phases and three IAU phases yield **1,050** checked state vectors across both
modes. Maximum DE405 errors are 1.237e-8 km and 1.456e-11 km/s; IAU errors are
1.680e-9 km and 2.002e-12 km/s. The fixed tolerances remain 5e-7 km and
5e-10 km/s. The actual report viewers display the checked raw files.

The visible desktop records 25 native xcb exposures, starts Iconic/minimized,
is resizable and non-fullscreen, and retains zero screenshots. Two process
image maps, source/image hashes before and after, and an executed-source archive
bind the run to its exact inputs. No human-observation or physical GPU claim.

Three failed IAU invocations remain failed and retained: v1 exceeded the ICRF
date domain; v2 incorrectly assumed the default PCK list was empty; v4 inspected
the error before queued GUI messages arrived. The final fixture tests the range
rejection, clears the default list explicitly, and delivers queued messages
before checking the same error. Source-v3 was a build-only intermediate.
No production source changed and no established passing regression was replayed.

``AuditLunarOrientationAcceptance.py`` regenerates the independent cases,
checks raw reports/stdout/proofs/JUnit, actual visible exposure and loaded images,
and rejects six corrupt proofs: position, velocity, epoch, missing basis,
missing reselection and missing range rejection. ``evidence-audit-v2.json`` and
``qualification-v2.json`` bind the exact artifacts. The cumulative builder then
revalidates the previous union plus these three original obligations.

Replay
------

From the repository root, with the existing configured Linux build::

   cmake --build build/linux-gui --target GmatQtNasaLunarOrientationTests -j4
   python3 src/qtgui/tests/RunLunarOrientationAcceptance.py
   python3 src/qtgui/tests/RunLunarOrientationAcceptance.py --visible

The runner chooses a fresh output directory and requests an initially minimized
visible desktop, zero screenshots and only the two new registrations. Their
CTest names are ``QtGui.NasaLunarOrientationDE405`` and
``QtGui.NasaLunarOrientationIAU2002``. Existing passing evidence remains reusable
at its recorded versions; replay alone does not award new scope.

Audit the retained evidence without rerunning missions::

   python3 src/qtgui/tests/AuditLunarOrientationAcceptance.py \
     --qualification build/nasa-coverage/coverage95-20261102-lunar-orientation/qualification-v2.json \
     --output /tmp/lunar-orientation-audit.json

``rebuild-ledger-inputs-v2.json`` records the exact prior qualifications and sealed
baseline. All 145 remaining row-specific gaps stay in ``coverage-snapshot-v2.json``.

Retention note: qualification v2 removes unnecessary live runtime-symlink
bindings from v1; exact executed image identities remain in the sealed receipts
and process maps. V1 is retained as superseded, with no native replay. There are
192 regular native evidence files totaling 65,638,914 bytes; the 150 generated
runtime aliases are symbolic links, not copied binaries.
