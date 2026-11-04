95 percent campaign: Save bodies, globals and regions
=====================================================

Seventeen new cases pass unattended and visibly: all eleven shipped default
bodies, the four current user-body categories, SolarSystem and a coordinate-list
PlanetographicRegion. All previous 70 cases are reused at their frozen versions.
Combined bounded Save evidence now has **87 cases, 174 contexts and 4,566
writable-property comparisons**, plus explicit reloaded literal witnesses.

Coverage remains **998/1,158 requirements (86.2%)** and **1,816/2,367 historical
scopes (76.7%)**. This increment awards no new NASA row. The complete any-Resource
requirement FRC!E132 still needs remaining families, modes and hidden/reference
state contracts. The 95% goal remains active: 103 complete requirements needed.
Original authorities, assertions, denominators and qualification gates remain.

Product and test repairs
-------------------------

One Qt production path changes: ``src/qtgui/CommandForm.cpp``. The native Save
command explicitly supports SolarSystem, but the picker gathered configured
resources and celestial bodies only, omitting SolarSystem. Its name is now
added specifically for Save. The real editor/picker selection, exact Apply,
mission execution, export and independent rebuild pass in both modes.
Other command selectors keep their existing candidate logic.

No additional core path changes. All 16 preceding Save metadata/serialization
review paths remain unchanged and unapproved for broad scientific use. Exact
Qt before/after identities and the narrow diff are retained under
``build/nasa-coverage/coverage95-20261101/save-bodies-final-audit.json`` and
``save-bodies-qt-diffs/``. No equations or ephemeris implementation changed.

The test harness originally searched the sandbox ordinary object map for
bodies. Bodies are held in its separate SolarSystem clone. A fixture spacecraft
now exposes that clone through its public native GetSolarSystem API; the
harness asserts that it differs from the configured pre-run SolarSystem before
reading body/global state. The actual saved runtime state is compared with the
independently rebuilt export; configured state is never substituted for it.

The independent audit caught incorrect Object type tags on the four user-body
CentralBody witnesses: the native descriptor is String. The expected Sun value
is unchanged. Native descriptor checks are added before the original runtime
and reload literal assertions, so this mismatch fails immediately in future.
Only those four cases were rerun after the correction; the other thirteen
passes in each mode were reused. Every earlier native assertion remains in its
original order, with additional checks only.

Scope, evidence and failures
----------------------------

Six failed native attempts remain retained: five body lookup failures in the
harness and the missing SolarSystem picker choice. Every failing registered
case has a passing replacement. Eight superseded passes with the old user-body
type tags are explicitly excluded from the selected evidence. There are 48
new native attempts total, including the replacements; no previous 70-case
native replay occurred.

Prescribed synthetic Mu/radius/flattening and user orientation constants test
persistence only. User-body cases use the existing, hash-bound Ceres SPK as
positional input and run no propagation. They do not independently validate
Ceres ephemerides, physical constants, orientation models or science results.
SolarSystem uses a nondefault 37.5-second update interval and checks native
settings and owned-body descriptors. The region preserves its four latitude/
longitude vertices; file-based regions and geographic algorithms are outside
this fixture.

All 41 shared checker contracts pass. Fourteen deliberately corrupted evidence
capsules are rejected after a valid baseline audit, including missing/changed
physical-input identity, missing runtime/reloaded states, a failed old body
receipt, a changed global literal and false whole-row or goal credit. Essential
source, runtime identity, JUnit, visibility, raw state and export artifacts are
bound independently. The checker itself required explicit binding of the
external Ceres input identity; this bookkeeping repair required no GUI replay.

Visible execution used automatically scaled, initially minimized, resizable,
non-fullscreen nested windows. No passing PNGs or runtime binary copies are
retained. Lossless compaction of 126 duplicate reports/traces saved 47,087,616
allocated bytes (44.9 MiB), preserving every admission artifact.

Reusable commands
------------------

Audit the complete retained conjunction without native replay::

   python src/qtgui/tests/AuditSaveBodyEvidence.py \
     --evidence build/nasa-coverage/coverage95-20261101/save-bodies-evidence.json
   ctest --test-dir build/linux-gui \
     -R '^QtAcceptance.SaveResourceEvidenceContract$' --output-on-failure

To watch this increment, choose a fresh output directory::

   python src/qtgui/tests/RunAcceptance.py \
     --build-dir build/linux-gui --filter '^QtGui.NasaSaveBody' \
     --visible --display-route nested --display-scale auto --start-minimized \
     --pause-ms 350 --max-checkpoints 200 --max-screenshots 0 --report-every 17 \
     --output build/gui-acceptance/save-bodies-watch

Omit visible/display flags for unattended operation. Existing sealed passes
remain reusable at their archived source/runtime versions; this is not a full
current-build regression qualification.

Remaining scope
----------------

Sixty-two native root type names now appear in the combined Save fixtures.
Twenty-three offered catalogue labels do not directly match them. These are
inventory counts, not coverage percentages: builtin functions, aliases and the
full persistence universe need substantive source and behavior review. Remaining
work includes function resources, aliases, alternate input modes, file-based
regions, hidden persistent fields and reference graphs. Earlier Formation STM,
external dependency portability, disabled ephemeris output and unfired thrust
gaps remain open. FRC!E135's default-filename source conflict is unchanged.

All 59 prerequisite holds remain unresolved. Even completing every other row
reaches at most 1,099/1,158 (94.9%); at least two holds need substantive resolution
for the 1,101 target. No unanswered prerequisite decision is assumed. Private
NASA provenance, platform-wide and scientific qualification remain open.
