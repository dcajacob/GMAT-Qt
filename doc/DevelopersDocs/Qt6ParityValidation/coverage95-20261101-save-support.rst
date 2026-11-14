95 percent campaign: additional Save resource families
=====================================================

Result and original scope
-------------------------

Sixteen additional cases pass unattended and visibly, including estimation
support, filters, CustomFOV/Imager, display/file resources and three disabled
locator types. One earlier GroundTrack case is repaired and replaced in both
modes; the other 44 earlier cases are reused at their recorded source/runtime
versions. The combined bounded evidence has **61 cases, 122 contexts and 2,776
writable-property comparisons**, plus explicit literal/reference checks before
and after export/reload.

Coverage remains **998/1,158 requirements (86.2%)** and **1,816/2,367 historical
scopes (76.7%)**. No new NASA row is admitted. FRC!E132's complete "any Resource"
obligation still needs other families, modes, globals and reference graphs.
Original requirements, denominators, fingerprints and qualification gates stay
unchanged. The 95% goal remains active, with 103 complete requirements needed.

A stronger check found a gap in the earlier passing GroundTrack evidence:
ShowName was captured before Save but lost after reload. Native metadata marks
that setting read-only, so the earlier writable-only comparison missed it.
The original receipt and seal remain immutable; the new conjunction replaces
only that case. The old pass must not be treated as proof of flag persistence.

Core patches requiring scrutiny
-------------------------------

Five additional core paths changed. Together with the preceding seven, twelve
paths now have explicitly tracked repair diffs. Exact before/after identities,
source backups and narrow patches are in ``save-support-final-audit.json`` and
``save-support-core-diffs/`` under ``build/nasa-coverage/coverage95-20261101/``.

* ``src/base/parameter/RealVar.cpp`` and ``RealVar.hpp`` add the missing Value
  label for the existing parameter ID. Parameter enumeration previously threw
  even though the ID, type and numeric getter existed. The new display fixture
  captures its Variable and checks its literal after rebuilding.
* ``src/base/subscriber/GroundTrack.cpp`` writes nondefault ShowName flags.
  Hidden metadata previously suppressed them on export. The affected case
  preserves ``[false true]`` in both modes. Default all-true/empty arrays remain
  implicit; other hidden GroundTrack settings still need explicit review.
* ``src/base/subscriber/DynamicDataDisplay.cpp`` preserves automatic color mode.
  An UpdateDynamicData command produced a warning color; serialization froze
  that computed color as a user selection. The writer now records the default
  automatic setting for automatic cells. Separate automatic and explicit-RGB
  cases check the native mode and bounds; the existing DynamicData GUI
  regression also passes.
* ``src/base/subscriber/TextEphemFile.cpp`` restores the complete parameter
  count in construction and runtime copying, and returns the coordinate-system
  name from its getter. The omitted fields and copy truncation lost Interval,
  EpochFormat and CoordinateSystem. All three and Precision now have explicit
  reload witnesses. A first incomplete count repair is retained as a failure.

These are metadata and serialization repairs. No force equation, numerical
integrator or filtering/smoothing calculation was changed. The patches are not
scientific or blanket current-build approval. Locator cases use Disabled mode;
TextEphemFile output numerics and general event/estimation results are outside
this increment. The external VF13ad binary is identified but its unavailable
implementation is not audited or modified.

Stronger reusable checks and evidence reuse
------------------------------------------

All original native assertions are retained. One additional assertion checks
every listed literal/reference after reload, even when metadata labels it
read-only. The independent audit also checks those literals in both captured
states and compares the entire baseline script with exact archived fixture
inputs. This exposed both the GroundTrack gap and the TextEphemFile omission.

DynamicDataDisplay's row/cell settings use its dedicated native
GetDynamicDataStruct API because its generic StringArray getter is a stub.
Configured manual/automatic mode is compared separately from computed display
colors. No serializer text or GUI property mapping supplies the state oracle.

The evidence reader validates each passing visible member of a mixed receipt
without changing that receipt or discarding its failures. Failed members remain
ineligible until replaced. This reuses the 16 passing members of the first
17-case visible run; only TextEphemFile was rerun visibly.

An optional Qt JSON lookup had inserted ``before_save: null`` in case metadata
without changing inputs or execution. The lookup is fixed. For older receipts,
the checker derives exactly that metadata side effect from the archived driver
version, still requires the exact archived input case, and independently
compares the full baseline script. Nonempty commands and other fields are not
normalized. No GUI replay was needed for this bookkeeping correction or the
clarified proof-scope wording; executed versions remain archived.

All **30 checker contracts** pass, including read-only literals, missing
parameter descriptors, color-mode changes, mixed-receipt handling and optional
metadata. All **11 corrupted conjunction capsules** are rejected, including an
attempt to reuse the old GroundTrack pass and one to select the failed visible
TextEphemFile result. Five failed native attempts are retained with passing
replacements; the earlier failing stronger GroundTrack audit is also retained.

Storage and reproducible commands
---------------------------------

New visible cases used automatic scaling in initially minimized, resizable,
non-fullscreen nested windows. No passing screenshots or runtime binary copies
are retained. Compression of 124 duplicate reports/traces saved 58,228,736
allocated bytes (55.5 MiB); essential raw proofs, failures, source archives,
logs and JUnit remain protected.

From the repository root, validate the complete retained conjunction without
launching native cases::

   python src/qtgui/tests/AuditSaveSupportEvidence.py \
     --evidence build/nasa-coverage/coverage95-20261101/save-support-evidence.json
   ctest --test-dir build/linux-gui \
     -R '^QtAcceptance.SaveResourceEvidenceContract$' --output-on-failure

For a watchable replay, use a fresh output directory::

   python src/qtgui/tests/RunAcceptance.py \
     --build-dir build/linux-gui --filter '^QtGui.NasaSaveSupport' \
     --visible --display-route nested --display-scale auto --start-minimized \
     --pause-ms 350 --max-checkpoints 200 --max-screenshots 0 --report-every 16 \
     --output build/gui-acceptance/save-support-watch

The registrations also run unattended. Earlier 45-case commands and their
original narrower evidence remain in the preceding Save-resource report.

Remaining work
--------------

Forty-six native root type names now appear in these bounded Save fixtures.
Thirty-seven of the retained catalogue's 83 offered names do not directly match
those names. This is an inventory difference, not a percentage: aliases and
built-in functions require separate classification, and creation labels are
not a complete Save-resource universe. Older chemical/parameter evidence keeps
its original scope and is not discarded or silently promoted.

Remaining families include Formation, EphemerisFile, FileInterface, thrust-file
resources, PlanetographicRegion, function resources, user-body categories and
default-body/SolarSystem globals. String/Array runtime snapshots, other hidden
persistent fields, multiple-propagator mappings and additional reference/mode
combinations also need complete contracts. Read-only metadata cannot alone
define which state must survive Save. Full NASA, private-corpus provenance,
platform-wide and complete current-build/science gates remain open.

FRC!E135's default-filename conflict remains separate and unchanged. All 59
original prerequisite holds remain open: closing every other row reaches at
most 1,099/1,158 (94.9%). At least two holds require substantive resolution for
the 1,101 target; no unanswered prerequisite-path decision is assumed.
