95 percent campaign: hidden and conditional Save state
=======================================================

Six new cases pass unattended and visibly: GroundTrack grid enabled/disabled,
a nondefault texture, a file-backed PlanetographicRegion, and both STK distance
units with opposite event-boundary settings. The affected QtGui.Region editor
workflow also passes in both modes. All 109 preceding native state/source cases
are reused at their recorded versions. Combined bounded Save evidence now has
**115 cases, 230 contexts and 5,324 writable-property comparisons**, plus explicit
reloaded literals and reinitialized file-derived coordinates.

Requirement coverage remains **998/1,158 (86.2%)**, with 103 complete requirements
needed for 95%. Historical coverage remains **1,816/2,367 (76.7%)**. No additional
NASA row is admitted: FRC!E132 still requires the complete persistence obligation.
Original authorities, denominators, fingerprints and qualification gates remain.

Two core repairs for scrutiny
------------------------------

``src/base/subscriber/GroundTrack.cpp`` previously marked UseGrid,
LatitudeLineCount and LongitudeLineCount read-only unconditionally, although
native setters accept them and plot initialization consumes them. Save therefore
omitted all three. Both grid fixtures lost their prescribed configuration after
reload, while their original literal assertions caught the loss. Nondefault
values are now serialized; the existing true/6/12 defaults remain implicit.
Grid rendering equations and plot initialization are unchanged. The tests check
configuration persistence, with ShowPlot disabled, plus a nondefault texture
reference. Independent cartography/rendered grid appearance is outside them.

``src/base/solarsys/PlanetographicRegion.cpp`` previously exposed file-derived
Latitude/Longitude lists as writable input after file initialization. Its generic
writer exported those lists together with AreaFileName; rebuilding failed because
the native setters explicitly forbid simultaneous file and explicit-list input.
Those coordinate fields now report read-only in file mode, matching the existing
setter contract. The label and integer metadata APIs use the same classification.
Vertex-input mode remains writable. No geometry, file parser, coordinate or
intersection equation changes.

This classification is checked beyond the writable-field comparison. The audit
compares native runtime vertices with the retained input file's two columns,
requires a file-only export, and then executes the rebuilt export to initialize
its coordinates. Both modes recover exactly [10,10,20,20] latitude and
[30,40,40,30] longitude. Immediately after a plain rebuild, the native coordinate
cache is empty; it is derived state and is checked after actual initialization.
The original comparison and literal assertions remain intact.

GroundTrack was already on the Save core review list; Region is one additional
path, bringing the list to **18**. All other 16 preceding paths remain unchanged.
Exact before/after identities, backups and narrow patches are in
``build/nasa-coverage/coverage95-20261101/save-modes-final-audit.json`` and
``save-modes-core-diffs/``. No Qt production path or native test-driver source
changes in this increment. Technical/scientific and complete current-build
approval remain false.

Failures and reusable evidence
--------------------------------

Three failed native attempts remain retained: two grid resets and the invalid
file-plus-vertices region export. Each has a passing replacement. The texture
and two STK pilot passes are reused without unattended replay. There are 17 new
acceptance attempts total: 15 new-case attempts, including the failures, and two
affected region-editor regression contexts. One initial build invocation used an
incorrect test target name; its log is retained, followed by the successful build.

The region text input is retained and hashed, including its Unicode filename.
The existing Moon texture is a separately hash-bound installation input, with no
image copy. External input references remain dependencies; this is not a portable
embedded Save package. STK ephemeris writing is disabled: these cases qualify
selected settings persistence, leaving output-format/physical conformance open.

The independent audit checks every selected source/runtime version, both GUI
modes, typed native descriptors and values, exact source Apply, retained asset
bytes, file-only region serialization and post-reload initialization. Both
original region-editor regression receipts retain their successful JUnit and
original source assertions for file/vertex switching and rollback. Ten corrupted
capsules are rejected after a valid baseline, including failed old grid/region
receipts, missing file/probe data, wrong physical-input identity, altered region
corners and a missing affected regression mode.

The 41 passing shared checker contracts are reused after verifying that their
source and retained log hashes are unchanged. No redundant checker replay was
needed. Older retired-editor clean observations remain explicitly unqualified;
their native source/state evidence is preserved, and the new cases use the
already repaired current-editor binding.

Descriptor review and storage
------------------------------

The new retained inventory covers all 115 state cases in both modes: 322 root
and 112 owned-object observations, 81 root types, 98 total observed types, and
2,009 distinct native type/field/kind rows. Of these, 1,086 have a read-only
observation, 1,053 were never writable in the selected configurations, and 47
have an explicit literal witness in a read-only observation. Many are computed
outputs or constant metadata. The next review groups them by shared source and
checks genuine configuration exceptions and missing valid modes. Inventory
counts award no NASA credit.

The inventory JSON/CSV and derivation script are retained as
``save-persistence-descriptor-inventory.*`` and
``review_save_persistence_inventory.py`` in the campaign directory. They use
native captures and source identities; GUI field mappings identify obligations
and never supply state truth. No full classification is inferred from a flag.

Visible runs used automatic scaling in initially minimized, resizable,
non-fullscreen nested windows. No passing PNGs or runtime binary copies are
retained. Lossless compaction of 49 duplicate reports/traces saved 23,433,216
allocated bytes (22.3 MiB), preserving all admission artifacts.

Commands and remaining scope
-----------------------------

Audit the retained conjunction without GUI replay::

   python src/qtgui/tests/AuditSaveModeEvidence.py \
     --evidence build/nasa-coverage/coverage95-20261101/save-modes-evidence.json

For a watchable replay, choose a fresh output directory::

   python src/qtgui/tests/RunAcceptance.py \
     --build-dir build/linux-gui --filter '^QtGui.NasaSaveMode|^QtGui.Region$' \
     --visible --display-route nested --display-scale auto --start-minimized \
     --pause-ms 350 --max-checkpoints 200 --max-screenshots 0 --report-every 7 \
     --output build/gui-acceptance/save-modes-watch

Omit visible/display flags for unattended operation. Reuse frozen passing
receipts for their exact scope and versions.

Remaining Save work includes owned force/attitude configuration modes, spacecraft
representations and file inputs, active local/named reference graphs, further
hidden persistent fields and inherited clauses. Earlier Formation STM,
external-dependency portability, scientific output conformance, the retired
editor observation gap and separate FRC!E135 filename conflict remain open.
All 59 prerequisite holds remain; the optimistic ceiling is 1,099/1,158 (94.9%)
until at least two holds are substantively resolved. No unanswered decision is
assumed. Full private NASA, platform, scientific and current-build qualification
remain open, and the 95% goal remains active.
