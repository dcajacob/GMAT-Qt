Direct conversion repairs and six new GUI conversion edges
==========================================================

Original requirement coverage remains **1,002/1,158 (86.5%)**, with99 needed
for95%. Historical operation coverage stays **1,816/2,367 (76.7%)**. This
increment adds no whole-row credit: ``FRR!E13`` still has explicit model,
frame/origin, representation, anomaly and boundary gates.

Core repair: new direct paths, not replayed GUI passes
----------------------------------------------------

The existing GUI pair tests use Cartesian-backed state storage and preview.
They do not exercise every direct ``StateConversionUtil::Convert(from,to)``
path. A new native test executes182 directed elliptic contexts across all14
registered types, plus56 hyperbolic contexts across eight eligible types.
It also retains22 exact same-type identity checks and unknown-type rejection.

The first run reproduced78 failing contexts. Several helpers passed non-Cartesian
element arrays to Cartesian/Keplerian routines; others omitted supported target
branches. The repair changes only11 dispatch-helper bodies in
``src/gmatutil/util/StateConversionUtil.cpp``:

* Modified-Keplerian, spherical-AZFPA, equinoctial, modified-equinoctial,
  Delaunay, Planetodetic, incoming/outgoing asymptote and Brouwer paths now
  pass the correct converted intermediate.
* Explicit previously missing targets use the established Cartesian dispatch.
  Existing optimized routes, unknown-type handling and public signatures remain.

The new matrix now has **237/238 passing contexts**, with77 repaired and no
regression among the160 originally passing contexts. All original matrix
assertions and bounds are preserved. Every byte outside those11 helper bodies
matches the pre-increment file, including primitive formulas, critical thresholds
and inverse algorithms. The original three GUI conversion drivers, conversion
header and ``SpacecraftOrbit.cpp`` are unchanged.

This is a core numerical-dispatch change and changes results on defective paths.
Exact pre-increment copies, the incremental diff and function-by-function repair
plan are retained under ``build/nasa-coverage/coverage95-20261101`` as
``direct-conversion-core-before``, ``direct-conversion-dispatch-v1.diff`` and
``direct-conversion-repair-plan.json``. Independent review is recorded; it is not
blanket scientific or full-current-build approval.

Remaining real native failure
-----------------------------

The new complete native test remains **enabled and failing normally** for
``Elliptic_SphericalAZFPA -> BrouwerMeanLong`` at osculating inclination64degrees.
There is no ``WILL_FAIL``, disabled assertion or expected-failure inversion.
Its labels are ``native-math;qt-nasa-known-gap``. A scientific known gap is not
classified as a harness preflight check, so it does not prevent launching other
GUI checks. Ordinary complete CTest execution still reports the failure.

The native long-mean forward model uses critical modification at
``BISUBC >= 0.001``; the retained NASA ``BROLYD.m`` prototype uses
``BISUBC >= 0.1``. The failing fixture and a tentative prototype-inverse solution
take different branches in those models. A small prototype residual therefore
does not prove a native inverse solution. The guide also describes instability
near63/117degrees. No primitive threshold or inverse fallback is changed to
force agreement. The discrepancy and source identities are explicit in
``direct-conversion-critical-model-gap.json``.

Brouwer checks retain their source-prototype/model limits; they do not establish
independent physical averaging. Elliptic asymptotes retain their declared
apoapsis-ray extension rather than a physical hyperbolic-asymptote claim.

New visible GUI coverage
------------------------

Only six previously missing directed GUI edges are added:
Planetodetic to/from Cartesian, SphericalAZFPA and SphericalRADEC. Each runs on
four retained independent spheroid geometries: sphere, Earthlike, strongly
oblate and southern-latitude cases. All use an eligible body-fixed Earth frame.

Both unattended and visible modes pass all16 new registrations:24 directed
contexts,576 displayed-field comparisons and576 unit comparisons per mode.
Checks cover actual popup selection, pending Cancel, Apply/native state,
exact Undo/Redo, Unicode save/reopen and Cartesian report/sentinel results.
All48 raw reports are independently audited. The visible run records176 native
exposure events in the resizable, initially minimized, non-fullscreen nested
desktop, with no screenshots or runtime copies. No old passing GUI case is replayed.

``PlanetodeticConversionOracle.py`` independently reproduces the executed
fixture byte-for-byte. It uses the existing spheroid-normal position equations
and radial/north/east velocity projections. The regeneration script was added
after execution and is separately frozen; it is not claimed as an executed
input. The executed fixture and underlying geometry source are already archived.
Radial-HFPA and Earth-frame/orientation scientific limits remain explicit.

The previous98 distinct edges are a **fixture inventory**, not a verified
execution count. These six new verified GUI edges do not overlap that inventory.
The182-edge planning graph still has78 edges without fixtures; admissible
frame/domain restrictions must be considered before treating it as a test matrix.

Failures and retention
----------------------

The first independent prototype-inverse generator failed a strict residual
check and is retained. The final native inverse witness instead checks actual
mean outputs through independent prototype forward reconstruction, preserving
model limits. Native V1 records78 failures; V3 records one. V2 was an accidental
unchanged-baseline rerun after a repair-script parser failed before writing;
it is retained and supplies no additional evidence credit. The corrected repair
recipe validates all helper boundaries before writing. No GUI case failed.

No passing screenshots, runtime images or existing kernels are copied.
Duplicate reports/traces are losslessly compressed after evidence auditing.

Reusable execution and handoff
------------------------------

Build the new cases::

  cmake --build build/linux-gui --target GmatNasaDirectOrbitConversionTests \
    GmatQtNasaPlanetodeticConversionTests -j4

Run the direct native matrix (currently expected to report the real failure)::

  ctest --test-dir build/linux-gui --output-on-failure \
    -R '^QtAcceptance.NasaDirectOrbitConversion$'

Watch only the new GUI conversion cases::

  python3 src/qtgui/tests/RunPlanetodeticConversionAcceptance.py --visible

Restore the minimized test desktop to watch; ``--pause-ms`` controls pacing and
``--filter`` selects particular new cases. The default pause is350ms. Automated
GUI execution is also registered under ``check-qt`` and ``build-qt-acceptance``.

``direct-conversion-expansion-evidence.json`` audits the actual mixed native
result and both passing GUI modes. It deliberately has no qualified keys and
no NASA credit. ``direct-conversion-final-audit.json`` seals this bounded
increment, not completion of95%. All156 remaining original requirements and551
historical cases remain. The GroundTrack step-sampling gap and59 prerequisite
holds are preserved. The95% goal remains active.
