Exact NASA MSIS-86 atmosphere acceptance
=======================================

The source-versioned union advances to **1,017/1,158 (87.8%)**: 141 original
requirements remain open, with 84 more needed for 1,101. Historical operations
remain **1,816/2,367 (76.7%)**. The authoritative ledger is
``build/nasa-coverage/coverage95-20261103-msise86/coverage-snapshot-v1.json``.
All 59 prerequisite holds and scientific/core/current-build/full-NASA gates
remain open. The 95% goal is active and incomplete.

Exact obligations and implementation
------------------------------------

* **FRR!E116**, MSISE-86: a new optional plugin implements the exact archived NASA
  model. Credit requires the complete conjunction with all three input children,
  actual native GUI configuration, persisted/runtime identity and independent
  density results. Other drag/dynamics parents remain open.
* **FRR!E117**, Constant solar: the F107 field is changed from 150 to 180.
* **FRR!E118**, Average flux: the F107A field is changed from 150 to 70.
* **FRR!E119**, Geomagnetic index: the Kp field is changed from 1 to 4.

Each changed input produces a distinct independently computed density. Exact
original text and fingerprints remain in ``requirement-review-v1.json`` and
``qualification-v1.json``; no denominator or assertion is relaxed.

``PLUGIN_MSISE86ATMOSPHERE`` defaults to OFF. The local acceptance build enables
it and registers ``MSISE86`` for Earth. GNU Fortran is required. Original model
and data files come from NASA's `CCMC ModelWeb archive
<https://git.smce.nasa.gov/ccmc-share/modelwebarchive/-/tree/21b2dfadc8a9c126d55f0e77d28aeb8f85b36b81/MSIS/MSIS86>`_,
pinned revision ``21b2dfadc8a9c126d55f0e77d28aeb8f85b36b81``. The original
README, model, coefficients, driver, reference output and documentation are
retained with download URLs, hashes and sizes. This new adapter does not claim
to reproduce or obtain the old separately distributed GMAT wrapper.

The generator embeds the original 1,464 coefficients, replaces working-directory
file initialization, namespaces external routines/COMMON blocks and expresses
two historical assumed-size arrays with ``(*)``. Scientific expressions remain
unchanged. ``-fno-automatic`` preserves cached local storage; omitting it fails
the original NASA examples. A mutex serializes model calls. Earth-only factory
selection and a 90 km lower geodetic-altitude guard make the supported domain
explicit. The exact production changes and CMake preimages are retained in
``exact-changes-v1.patch``; source archives retain all intermediate versions.
No existing GmatBase/GmatUtil scientific implementation was edited by this increment.

Independent evidence and GUI execution
--------------------------------------

NASA's ten published examples contain 100 species/density/temperature values.
The plugin matches them at their published precision across original, reverse
and interleaved order: **300 comparisons**. Thirty intervening MSISE90 calls
remain identical to their pre-plugin baseline. Original NASA Fortran compiled
without adapter changes also matches all 100 published values with the required
legacy storage flags. The audit regenerates the isolated source and checks
symbol names, source identities, raw reference values and the registered CTest.

Twelve GUI cases vary daily flux, average flux, magnetic index, altitude
(90.1/400/1,000 km), latitude (south/equator/north/near-pole), longitude, date
and ellipsoid flattening. Actual model popups and constant fields pass Cancel,
nested OK/parent Apply, typed native readback, Undo/Redo, Unicode save/reopen
and dialog readback. Two real drag missions per case verify cloned model identity,
continuation and the actual report viewer's displayed bytes.

All twelve cases and two affected regressions pass unattended and visibly at
source-v6. The regressions are ``QtGui.Atmosphere`` and
``QtGui.NativeNasaWeatherChoiceConstantHistoryCssiPredict``. A thirteenth new case
at source-v7 rejects 89 km with the precise message, then recovers at 90.001 km;
it also verifies Earth/Mars factory qualification. It passes in both modes.
The production plugin is byte-identical between the two source groups. There
are **30 passing GUI executions**, 15 mapped visible processes and 311 native
exposures, with initially minimized, resizable, non-fullscreen desktops and
zero screenshots. This does not claim human observation or physical GPU testing.

The independent oracle compiles unchanged NASA Fortran separately, without GMAT
libraries. Python civil-date and inverse-ellipsoid calculations derive its inputs
from retained EarthFixed report states. The `ISGI Kp-to-ap table
<https://isgi.unistra.fr/Documents/ConversionTables/Kp2ap.pdf>`_ independently
supplies Kp1/ap4 and Kp4/ap27. All **48 native density vectors** match exactly
(maximum relative error 0; fixed tolerance 5e-6). Native atmosphere output is
kg/m³, whereas the force-qualified report parameter is kg/km³; both conversion
and direct native/report consistency are checked explicitly.

This establishes fidelity to the required model and its adapter over the tested
domain. It does not independently validate the physical empirical model, full
Earth/FK5 astronomy, long-trajectory accuracy or untested altitude regimes.
Scientific/core approval remains open.

Audit and retained failures
--------------------------

``AuditMsise86Acceptance.py`` checks the two frozen source groups, before/after
input identities, loaded binary maps, raw CSV/script/proof/stdout/JUnit, visible
exposures, model identity and the independent calculations. Eight corrupt proofs
are rejected: wrong model, density, native units, missing repeat/popup, wrong case,
accepted below-domain altitude and missing recovery. ``evidence-audit-v1.json``
records the passing result. The cumulative audit revalidates all 16 inherited
qualifications plus this increment without rerunning passing GUI missions.

Six development failures are retained in ``failure-register-v1.json``:
original reference built without static local storage; generated LOWER data
incorrectly typed as integer; native fixture registered offscreen; oracle report
units mistaken for SI; support CTest initially unregistered; and native preflight
missing two regression executables. Fixes preserve scientific expressions and
original assertions. All failed logs/source versions remain evidence, not passes.

Native/runtime symbolic links are excluded from immutable artifact bindings.
Recorded image hashes and process maps retain binary identity; no copied runtime
binaries or large ephemeris files are introduced. ``stage-receipt-v1.json``
records regular-file storage and confirms all 59 original holds remain open.

Replay
------

In the existing configured Linux build::

   cmake -S . -B build/linux-gui -DPLUGIN_MSISE86ATMOSPHERE=ON
   cmake --build build/linux-gui --target GmatQtNasaMsise86Tests GmatQtNasaMsise86BoundaryTests -j4
   ctest --test-dir build/linux-gui -R '^QtAcceptance.NasaMsise86Reference$' --output-on-failure
   python3 src/qtgui/tests/RunMsise86Acceptance.py
   python3 src/qtgui/tests/RunMsise86Acceptance.py --visible

The runner selects only the thirteen new cases, uses a fresh output directory,
and requests zero screenshots and an initially minimized visible desktop.
``--with-regressions`` adds the two affected cases when new changes justify it;
build ``GmatQtAtmosphereTests`` and ``GmatQtNasaWeatherChoiceTests`` first.
Reuse existing passes at their recorded versions; replay alone awards no scope.

Audit retained evidence without rerunning missions::

   python3 src/qtgui/tests/AuditMsise86Acceptance.py \
     --qualification build/nasa-coverage/coverage95-20261103-msise86/qualification-v1.json \
     --output /tmp/msise86-audit.json

``rebuild-ledger-inputs-v1.json`` identifies the sealed baseline and exact
qualifications. All 141 remaining row-specific obligations and gaps are retained
in ``coverage-snapshot-v1.json``. In particular, Mars-GRAMM, all-model parents,
full frame/attitude/parameter conjunctions and the historical holds remain open.
