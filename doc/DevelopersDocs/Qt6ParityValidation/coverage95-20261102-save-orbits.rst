95 percent campaign: orbit definition persistence
=================================================

Eleven new Save cases pass unattended and visibly: Cartesian, Keplerian,
ModifiedKeplerian, Equinoctial, SphericalAZFPA, SphericalRADEC,
ModifiedEquinoctial, AlternateEquinoctial, Delaunay, IncomingAsymptote and
OutgoingAsymptote. All 131 preceding native state/source cases are reused at
their recorded versions. Combined evidence has **142 cases, 284 contexts and
9,122 writable-property comparisons**.

Coverage remains **998/1,158 requirements (86.2%)**, with 103 complete requirements
needed for 95%. Historical coverage remains **1,816/2,367 (76.7%)**. This increment
awards no complete requirement. FRC!E132 still requires complete persistent
state, mode and reference-graph obligations. Authorities, denominators,
fingerprints and qualification gates remain unchanged.

Independent physical state across Save/rebuild
-----------------------------------------------

These cases reuse the original positive orbit-definition inputs and independent
Cartesian truth. Each explicitly saves the selected Earth/Spacecraft graph,
including Earth.Mu=398600.4418. Saving the configured body matters: matching
orbital elements alone would not prove the same Cartesian velocity if the
reference body's gravitational parameter changed after reload.

An optional native helper reads Spacecraft.GetState("Cartesian") independently
of active display descriptors, GUI property mappings and serialization. It
checks six finite components against the existing position/velocity bounds
(1e-6 km and 1e-9 km/s). Every case has both runtime and reloaded captures in
each mode: **44 captures and 264 component checks** across 22 execution contexts.
The Python audit separately derives conic position/velocity from the original
semi-major axis, eccentricity, inclination, node, periapsis and true anomaly.
It binds the original fixture inputs, frame, Mu and tolerances, then checks the
raw native captures. No GMAT conversion routine supplies its expected truth.

Existing native writable-property and literal checks remain active. The Save
editor/picker, ordered selection, empty-selection rejection, exact source Apply,
following mission statement, independent export/rebuild and owned configuration
checks retain their original assertions. These tests run no propagation and do
not establish general orbital conversion or scientific-domain qualification.

Retained failures and repairs to the new fixtures
--------------------------------------------------

There are 35 attempts: 13 retained failures and 22 selected passing contexts.
The first eleven attempts completed their missions and native state checks but
failed to find the fixture's incorrectly named export. The existing multi-object
Save rule writes Earth_Subject.data; the fixture had used Earth's single-object
filename. Correcting that expectation changed no product behavior and does not
resolve the separate historical FRC!E135 default-filename conflict.

Nine corrected unattended cases passed immediately. The two spherical cases
then exposed a readback convention in the new expected literals: their native
RA getter returns equivalent signed angles. The original positive-angle input
scripts and all independent Cartesian values remain unchanged. Only the two
expected readback literals were corrected using the native signed-RA convention,
as the earlier orbit tests already account for angular equivalence. Strict
literal comparisons and all numerical bounds remain intact. Only those two
failed cases were rerun unattended; all eleven first visible checks passed.
The nine unattended passes were reused. No previous 131-case native replay
occurred, and all failed registered cases have passing replacements.

No new core or Qt production changes
-------------------------------------

All 21 preceding Save core review paths remain unchanged, as do Qt production
sources. The shared native test driver adds only the optional Cartesian helper
and its two captures. All original lines and all **47 balanced require
expressions**, including the helper declaration, remain in order. Two new require
expressions check object/shape/bounds and finite Cartesian truth. No existing
assertion, expected bound or comparison scope is weakened. The exact test-driver
patch and original source are retained with save-orbits-final-audit.json.

The 53 shared checker contract passes are reused with unchanged checker/test
sources. Eleven additional Cartesian admission contracts pass, including changed
position/velocity, omitted capture, wrong object/frame, altered tolerance,
nonfinite/bool values, missing components and inconsistent independent inputs.
Their tested functions and loaded Cartesian case remain exactly unchanged after
later audit-only RA binding and summary-counter additions; those identities are
recorded rather than replaying the passing contracts. Ten corrupted conjunction
capsules are rejected after a valid baseline, including failed receipts, missing
visible/native captures, altered Mu and missing graph membership.

The early pre-retention summary counted only six components per execution
context. The final audit explicitly counts both native capture phases, 44
captures/264 components. This arithmetic correction changes no assertion, raw
result, fixture input or requirement credit.

Storage, audit and visible replay
----------------------------------

Visible cases use automatic scaling in initially minimized, resizable,
non-fullscreen nested windows. No passing screenshots, runtime binary copies or
existing kernel copies are retained. Lossless compaction of 90 duplicate reports
and traces saved 32,288,768 allocated bytes (30.8 MiB), preserving every admission
artifact. The expanded descriptor inventory is saved separately as
save-orbits-descriptor-inventory.json/csv; earlier seals remain unchanged.

Audit the retained conjunction without native replay::

   python src/qtgui/tests/AuditSaveOrbitEvidence.py \
     --evidence build/nasa-coverage/coverage95-20261101/save-orbits-evidence.json

The new checker contract is reusable through CTest::

   ctest --test-dir build/linux-gui \
     -R '^QtAcceptance.SaveOrbitCartesianEvidenceContract$' --output-on-failure

To watch only these eleven new cases, choose a fresh output directory and restore
the initially minimized test window::

   python src/qtgui/tests/RunAcceptance.py \
     --build-dir build/linux-gui --filter '^QtGui.NasaSaveOrbit' \
     --visible --display-route nested --display-scale auto --start-minimized \
     --pause-ms 350 --max-checkpoints 200 --max-screenshots 0 --report-every 11 \
     --output build/gui-acceptance/save-orbits-watch

Omit visible/display flags for unattended operation. Frozen receipts qualify
only their recorded source/runtime versions, not the complete latest build.

Remaining gaps and target feasibility
--------------------------------------

Other representations (including Planetodetic and Brouwer modes), anomaly
variants, body/frame/reference graphs, singular/domain boundaries and hidden
persistent state remain open. Parent attitude/rate/sequence, optional FK,
file-backed post-rebuild initialization, dependency portability, computed getter
and earlier retired-editor-clean-observation limitations remain inherited.
These eleven selected definitions do not establish complete any-Resource Save.

All 59 original prerequisite holds remain unresolved. Even if every other row
closed, the scope would reach only 1,099/1,158 (94.9%); at least two held
obligations need substantive resolution for 1,101. Further source conflicts may
lower achievable coverage. No unanswered prerequisite decision is assumed.
Private NASA, historical platforms, scientific and complete current-build gates
remain open; the 95% goal is not complete.
