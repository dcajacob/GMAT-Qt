Public C-interface spacecraft configuration acceptance
======================================================

The source-versioned union advances to **1,007/1,158 (87.0%)**, adding exactly
``All Others!E250``: configuring one or more spacecraft by loading a GMAT
script through the public C interface. Historical coverage remains
1,816/2,367 (76.7%). There are 151 unqualified original requirements and
94 more are needed for the 95-percent target.

This batch changes tests and test-build integration only. **No production or
core source changed.** No earlier native or GUI scenario was replayed.

Complete configuration conjunction
----------------------------------

``capi-config-clause-map.json`` maps all 38 inherited configuration clauses
from the original FRR1 through FRR4 sections to their exact accepted proofs:
31 baseline mappings, five recovered mappings and the two complete inertial
body/frame-state definition qualifications. The audit checks 153 original
checkpoint/assertion-source links across 32 prior passing GUI cases, including
their actual logs and JUnit. Full frame/body definitions retain their recorded
qualified domains and source versions. They are not retested or broadened.

FRR!E13's arbitrary pairwise state-conversion behavior remains unqualified.
It is not an additional configuration mechanism. This E250 claim concerns
loading configured spacecraft, not complete spacecraft physics qualification.
The dynamics-model and Propagate configuration rows E251/E252, their E249
parent and the MATLAB-interface rows remain open.

C ``LoadScript`` forwards valid script files to the Moderator filename
overload. Qt uses the stream overload. The filename path checks ASCII encoding
before joining the same ``ScriptInterpreter::Interpret()`` parser. The shared
parser identity and unchanged public forwarding body are checked at recorded
versions. This does not claim byte-equivalent acceptance of Unicode comments
from GUI fixtures. Existing 133/481/190-assertion C loading, state/derivative/STM,
epoch and lifetime evidence is reused without executing those clients again.

New direct C witness
--------------------

A genuine C11 client loads and runs one new mixed configuration containing
31 distinct spacecraft and their chemical tanks:

* Six original orbital input representations.
* All ten epoch format/system combinations.
* Fifteen attitude definitions, including all twelve Euler sequences,
  quaternion/DCM/MRP input and both angular-rate forms.
* Nondefault dry mass, Cd, Cr, drag/SRP areas and tank-inclusive total mass.

The public object inventory and named epoch getters are checked by 1,006 C
assertions. A complete raw report supplies 25 fields per spacecraft: epoch,
six Cartesian components, nine DCM entries, three angular velocities, five
ballistic settings and total mass. All **775 values** match the preserved
independent fixture inputs. These are new C-boundary combinations; the GUI
workflows supplying the underlying configuration proofs are not replayed.

The audit regenerates the exact script, epoch input and expected matrix from
the frozen executed generator and original oracle assets. Both client and
numerical-result JUnit records, source snapshots, compiled client identity and
all 25 actually loaded project images are bound. Four executing admission
reader modules are frozen and checked. Independent review passes. Corrupted
mass, attitude, epoch and incomplete-clause controls are rejected.

The native run passed on its first execution. The first audit attempt used
the wrong level of the older recovered-scope JSON schema; indexing its
``requirements`` member resolved that metadata error without replaying tests.
All original assertions and expected fixture values remain intact.

Automation and retained evidence
--------------------------------

With ``PLUGIN_CINTERFACE=ON`` on Linux::

  cmake --build build/linux-gui --target GmatNasaCapiSpacecraftConfigurationTests -j4
  ctest --test-dir build/linux-gui \
    -R '^QtAcceptance.NasaCInterfaceSpacecraftConfiguration$' --output-on-failure

For a chosen evidence destination::

  python3 src/qtgui/tests/RunCapiSpacecraftConfigurationAcceptance.py \
    --executable build/linux-gui/src/qtgui/GmatNasaCapiSpacecraftConfigurationTests \
    --output-root build/capi-acceptance

The new CTest is an acceptance-support check. ``build-qt-acceptance`` now
depends on all seven C-interface test/probe targets, so opt-in acceptance runs
build these support executables before checking them. The dependency graph was
verified without executing old tests. No GUI windows, screenshots, runtime
copies or kernel copies were created by this C-only run.

Artifacts under ``build/nasa-coverage/coverage95-20261101/``:

* ``capi-config-coverage-snapshot.json``: cumulative original-row union and all
  remaining requirements/historical gaps.
* ``capi-config-qualification.json``, ``capi-config-clause-map.json`` and
  ``capi-config-route-review.json``: exact admission and inherited scope.
* ``capi-config-native-v1``: the single native execution, raw CSV, oracle
  snapshots, source archive, actual library identities and both JUnit results.
* ``capi-config-negative-controls.json`` and
  ``capi-config-independent-review.json``: verifier and independent review.
* ``capi-config-final-audit.json``: final artifact/source seal.

Complete current-build, arbitrary physics/input/platform and full NASA
qualification remain unclaimed. OEM E374/E375 remain open: the preserved
ShortArc expectation conflicts with B-1 interpolation support. A specific
request to correct that expectation is pending; no dependent change was made.
