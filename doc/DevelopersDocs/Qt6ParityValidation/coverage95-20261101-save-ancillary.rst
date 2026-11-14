95 percent campaign: Save parameter and ancillary resources
===========================================================

Nine new cases pass unattended and visibly: runtime Variable/String/Array,
Formation, EphemerisFile, FileInterface, ThrustSegment, ThrustHistoryFile and
GmatFunction. All 61 previous cases are reused at their archived versions.
The combined bounded evidence has **70 cases, 140 contexts and 3,316 writable
property comparisons**, plus explicit literal/reference witnesses after reload.

Requirement coverage remains **998/1,158 (86.2%)**, with 103 complete original
requirements needed for 95%. Historical coverage remains **1,816/2,367 (76.7%)**.
No new NASA credit is awarded: FRC!E132's complete any-Resource obligation is
still open. Original assertions, denominators, authorities and gates remain.

Core repairs for review
-----------------------

Four additional core paths changed, bringing the explicit Save review list to
16 paths. Exact before/after hashes, source backups and patches are recorded in
``build/nasa-coverage/coverage95-20261101/save-ancillary-final-audit.json`` and
``save-ancillary-core-diffs/``. All preceding twelve repairs remain unchanged.

* ``src/base/parameter/Array.cpp`` removes a double parameter-ID offset from
  GetParameterTypeString. The descriptor now agrees with the native type API.
  The case checks the full runtime 2-by-3 matrix before and after rebuilding.
* ``src/base/parameter/StringVar.cpp`` and ``StringVar.hpp`` expose the existing
  Value ID in the parameter count, label and type APIs. Getter/setter and
  evaluation behavior are unchanged. The first narrow String pass lacked the
  Value descriptor and is replaced by the stronger passing evidence.
* ``plugins/FormationPlugin/src/base/formation/Formation.cpp`` marks FormationSTM
  read-only. That advertised propagation item has no implemented getter/setter
  and GetPropItem returns NULL. **Formation STM remains unimplemented and
  unqualified**; this metadata repair does not implement its dynamics. The
  saved member graph and spacecraft literal properties are checked separately.

These patches have no scientific or blanket current-build approval. No force
or integration equation changed. Every original native require expression
remains in its original order; new fixture and reload-probe checks are added.

Independent checks and failures
--------------------------------

Three text inputs total 4,327 declared UTF-8 bytes, repeated across six mode
contexts. Retained copies, filenames, paths, byte counts and SHA-256 digests
are independently bound to the exact fixture. Save preserves external file
references; it does not embed a portable dependency package. Ephemeris writing
is disabled, and thrust is not fired: output-format conformance and thrust
physics are outside these cases.

The GmatFunction test executes out=2*input+1 with input 6 before Save (13), then
rebuilds the actual export and executes with input 7 (15). Both modes capture
and independently check the reloaded execution; one function body and these
two calls do not qualify all argument/private/global semantics.

Two failed native attempts (Array metadata and Formation STM metadata) remain
retained with passing replacements. FileInterface Format and EphemerisFile
StepSize witness tags were corrected to the actual Enumeration type; expected
values and numeric bounds were preserved. Only affected cases were rerun.
All **41 checker contracts** pass and **12 corrupted capsules** are rejected,
including missing input bytes, file manifest, reloaded execution and an older
String pass lacking its descriptor. No previous 61-case native replay occurred.

Storage and reproducibility
----------------------------

Visible runs used automatic scaling in initially minimized, resizable,
non-fullscreen nested windows. No passing PNGs or runtime binary copies are
retained. Lossless compression of 75 duplicate reports/traces saved 35,815,424
allocated bytes (34.2 MiB); admission artifacts remain byte-for-byte unchanged.

Check the retained conjunction without launching native cases::

   python src/qtgui/tests/AuditSaveAncillaryEvidence.py \
     --evidence build/nasa-coverage/coverage95-20261101/save-ancillary-evidence.json
   ctest --test-dir build/linux-gui \
     -R '^QtAcceptance.SaveResourceEvidenceContract$' --output-on-failure

For a watchable replay, choose a fresh output directory::

   python src/qtgui/tests/RunAcceptance.py \
     --build-dir build/linux-gui --filter '^QtGui.NasaSaveAncillary' \
     --visible --display-route nested --display-scale auto --start-minimized \
     --pause-ms 350 --max-checkpoints 200 --max-screenshots 0 --report-every 9 \
     --output build/gui-acceptance/save-ancillary-watch

The same registrations run unattended by omitting the visible/display flags.
No replay is needed to reuse the sealed source-versioned passing evidence.

Remaining gaps
---------------

Fifty-five native root type names now occur in the combined Save fixtures;
28 offered catalogue labels do not directly match them. These are inventory
counts, not requirements percentages: aliases, globals and hidden persistent
state require substantive review. Remaining work includes SolarSystem/default
body globals, four user-body categories, PlanetographicRegion, builtin function
resources, hidden fields, input modes and reference graphs. Creation labels
alone do not define the complete Save-resource universe.

FRC!E135's default-filename conflict is unchanged. All 59 prerequisite holds
remain: even closing every other requirement reaches only 1,099/1,158 (94.9%).
At least two holds need substantive resolution to reach 1,101. No unanswered
prerequisite decision is assumed. Private NASA provenance, platform-wide,
complete current-build and scientific qualification gates remain open.
