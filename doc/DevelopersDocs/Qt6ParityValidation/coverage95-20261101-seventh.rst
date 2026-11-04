Inertial celestial-frame conic definitions
==========================================

The seventh 95-percent increment closes **FRR!E5**. Source-versioned requirement
coverage is **998/1,158 (86.2%)**, with **103 more complete requirements** needed
for the 1,101 target. Historical coverage stays **1,816/2,367 (76.7%)**.
The original **160 requirement** and **551 historical** gaps remain explicit in
``build/nasa-coverage/coverage95-20261101/coverage.json``. The goal remains active.

Complete original conjunction
-----------------------------

E5 explicitly lists Classical Keplerian, Modified Keplerian and Equinoctial
through children E6/E7/E8, in inertial celestial-origin frames. It is separate
from E13's broader all-supported-representation conversion scope.

**66 new registered cases pass unattended and visibly.** Twenty-two new
frame/body contexts each exercise all three representations, with three
states/epochs. The conjunction covers the seven declared inertial axes,
eleven default celestial bodies and four user-defined body types. Additional
BodyInertial Mars/user-body cases exercise those branches. The original
EarthMJ2000Eq definition children, body/frame prerequisites and the complete
E9 transaction family remain mandatory at their recorded source/runtime
versions. Five passing pilots were reused; only the failed pilot and sixty
missing cases ran next. No known passing unattended case was replayed.

New checks exercise the actual frame round trip and representation popups,
six typed conic fields and units, native inertial/axis/body/type/user/GM/TA
bindings, Apply and its source rebuild, complete unrelated-resource isolation,
raw numerical mission output, epoch and sentinel, and unchanged source/history.
Classical/Modified states include inclined elliptic, hyperbolic and
circular/equatorial inputs; Equinoctial uses its supported elliptic domain.
At least two samples per cell change the physical state from the baseline.
Three dedicated cells also check parabolic/coupled invalid input rejection and
atomic recovery. Existing generic transaction assertions are retained rather
than duplicated across every new cell.

The independent oracle uses scalar conic geometry, periapsis/apoapsis algebra,
and an independently solved Kepler equation for Equinoctial inputs. No GMAT
conversion API supplies expected values. Original bounds remain **1e-6 km**
and **1e-9 km/s**. The complete declared-category witnesses are combined with
the reviewed common conic/GM and invertible inertial-frame argument. This
factors the unbounded valid parameter cross-product; it is not an exhaustive
parameter sweep or independent celestial ephemeris/orientation qualification.
Gravitational constants are prescribed test inputs, not new claims of physical
truth. All 396 qualified report rows and 2,376 independent Cartesian report
fields are checked. Twenty-one corrupt/incomplete capsules are rejected.

Failures, changes and retained limits
-------------------------------------

There are **no new product or core changes**. All 104 previously pending core
paths match the sixth increment snapshot, including its interpreter repair.
The Qt selector repair and four earlier path repairs also remain unchanged.
Existing assertion sources, independent fixture data and tolerances remain
unchanged. The prior affected regression evidence is reused.

One failed new pilot is retained. Its fixed-axis setup supplied a Gregorian
string to ``CoordinateSystem.Epoch``, whose script property requires a real
number. The harness now uses the retained numeric25300.25 convention and checks
the actual native epoch. This correction changes no numerical assertion.
All three new builds passed. ``seventh-change-and-failure-review.json`` contains
exact attempt logs and input/runtime identities. No failing case was disabled
or reclassified as passing without execution.

``seventh-native-conversion-review-note.json`` separately records a source-level
concern in the Equinoctial-to-Keplerian utility branch. It is not a reproduced
GUI failure, and no core fix or additional requirement credit is inferred.
The representation-to-Cartesian route tested here is distinct. Independent
core/science approval, complete current-build and full NASA qualification stay
open. E13, attitude/source conflicts, historical OS and MATLAB/interface
obligations remain gaps. The original59 prerequisite holds still imply an
optimistic94.9-percent ceiling if all persist; at least two must be resolved
for95 percent. No original obligation is retired to improve arithmetic.

Reusable commands and evidence
------------------------------

Build and run the new family unattended from the repository root::

  cmake --build build/linux-gui --target GmatQtNasaInertialDefinitionTests -j2
  ctest --test-dir build/linux-gui -R '^QtGui.NativeNasaInertialDefinition' --output-on-failure

Watch it in the existing resizable/scaled desktop. Restore its initially
minimized window; it does not enter fullscreen::

  python src/qtgui/tests/RunAcceptance.py --build-dir build/linux-gui \
    --filter '^QtGui.NativeNasaInertialDefinition' --visible \
    --display-route nested --display-scale auto --start-minimized \
    --pause-ms 0 --max-screenshots 0 --max-checkpoints 200 --report-every 70

Narrow the filter to a single case for inspection. The runner prints its fresh
temporary evidence directory. For this campaign, ``run_inertial_definitions.py``
and ``record_inertial_definition_inputs.py`` freeze the actual inputs/runtime
identities. No runtime binaries are copied and passing screenshots are disabled.
Unneeded completed traces/reports are losslessly compressed, with hashes and
savings recorded in ``seventh-retention-audit.json``.

Audit the complete frozen conjunction without running a GUI::

  python src/qtgui/tests/AuditInertialDefinitionAcceptance.py \
    --qualification build/nasa-coverage/coverage95-20261101/inertial-definition-qualification.json

``next-conversion-gap-review.json`` maps98 distinct directed edges in existing
fixtures and84 missing fixture edges within a possible182-edge planning graph.
These are fixture counts, not passing-evidence or requirement counts. Review
original source-bound receipts, eligible frame/domain restrictions and retained
FreeFlyer truth provenance before adding only genuinely missing witnesses.
``next-after-inertial-definitions.json`` preserves that next gate and every
remaining original gap without assigning further credit.
