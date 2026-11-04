Recommended campaign: physical input units and source preservation
===================================================================

Three additional requirement contracts are qualified: All Others!E14/E15/E16,
for kilometer, kilometer-per-second and degree entry. The original
``Suggestion to Remove`` annotations remain in their reviewed source text;
no requirement is retired. The source-versioned campaign union is now
**890/1,158 requirements (76.9%)** and **1,812/2,367 historical scopes (76.6%)**.
This is targeted evidence, not whole-current-build or NASA qualification.

The 37 proposed local contracts have 36 verified dispositions and one explicit
missing obligation, All Others!E12 (documentation of every input field).
Retained-evidence recovery remains 121 accepted, 39 held and 57 pending.
The original full denominators, assertions and deferred scopes remain intact.

New checks and actual repair
-----------------------------

``QtGui.NativeNasaUnitInputs`` enters 15 numeric fields through real resource
editors and observes their visible units. Four native missions check:

* Literal Cartesian position in km and velocity in km/s.
* Six Keplerian entries with degree angles against an independently implemented
  perifocal-to-inertial rotation, using literal mu=398600.4415 km^3/s^2.
* Three inertial impulsive-burn components in km/s against literal vector addition.
* A representation-selector-only change against the same independent physical
  state, ensuring all six unchanged components survive serialization.

Each phase checks pending-edit isolation, Apply, exact Undo/Redo, Unicode
save/reopen, a literal sentinel, exact source/history preservation on Run and
unchanged configured spacecraft serialization. The final visible case passes
in 16.115 seconds with six native exposure events, zero screenshots and no
runtime binary snapshots. Its window is resizable, starts minimized and is
not fullscreen; the user can restore it to watch. These are Qt event-driven
native widget checks, not physical OS-input or graphics-driver certification.

The degree contract exposed a real Qt GUI defect: Apply serialized Cartesian
components without retiring the original Keplerian input assignments. GMAT
correctly rejected the mixed input representations. MainWindow.cpp now includes
actual superseded input assignments in the patch's old-key inventory for orbit
commits. Retiring any such aliases also writes all six accepted Cartesian
components, including equal and zero values. Unrelated resource edits retain
the existing path. The patch changes no engine equations, headers or ABI.

The final affected existing Keplerian conversion, ModifiedKeplerian conversion
and resource/source round-trip cases all pass with their original assertions.
Only these three affected existing cases were replayed. New-case development
failures and superseded passes are retained; the fixture first needed explicit
NAIF IDs for exact rebuild comparison and a single initial input representation.
The failed Apply assertion was preserved and fixed in the application.

Core changes in this increment: zero. All 98 previously changed core files
remain unchanged and still require the separate scrutiny already recorded.
The one Qt production change has an exact incremental review patch and before
source, independent of the older pre-existing MainWindow fixture difference.

Documentation inventory and remaining gap
------------------------------------------

The reproducible ``AuditInputUnits.py`` scans all 128 shipped resource, command
and system DocBook sources without parse errors. It records 1,368 field-table
entries, including 768 GUI-tagged entries. These include duplicates, legacy
pages and templates; they are not a distinct-runtime-input denominator.

Three substantiated missing entries were filled: AZI degrees in the older
spacecraft-properties page, the DateFormat selector and format-dependent Epoch
units. The current orbit-state and epoch descriptions provide the references.
There are now 699 GUI-tagged entries with explicit Units metadata and 69 without.
The latter mostly concern references, switches, expressions and dependent solver
quantities. They are a review queue, not 69 confirmed product defects.

The new Qt6InputUnits.md guide explains common entries and links their shipped
field references. The prior hardware/force unit witnesses remain retained;
they were not replayed or expanded into an all-fields claim. All Others!E12
stays unqualified: a complete active-input-to-document crosswalk, including
conditional modes, commands, expressions, narrative-only and plugin inputs,
does not yet exist. The separate ambiguous non-dimensional-label row E13
remains outside this increment and unqualified.

Evidence and reuse
-------------------

In ``build/nasa-coverage/recommended-campaign-20261101/``:

* ``unit-input-qualification.json`` binds the exact review, receipt, terminal
  log, JUnit, archived text sources and scoped before/after source/runtime
  identities. The verifier also binds the executed test binary to its receipt.
  Sixteen real qualification corruptions are rejected without GUI replay.
* ``unit-input-visible/`` retains the final receipt, trace, proof and raw numeric
  results. ``unit-input-affected-regressions.xml`` records the three affected
  existing passes. Existing VF13ad qualification still validates from its own
  frozen inputs; it is not retroactively attributed to the new Qt source.
* ``unit-input-qt.patch`` and ``unit-input-qt-review.json`` isolate the Qt fix.
  ``input-units-inventory.json`` preserves every scanned entry and the gap queue.
* ``seventh-increment-audit.json`` records preservation and this bounded result.
  Redundant development summary JSON is losslessly compressed with raw and
  compressed hashes; no passing images or runtime copies are retained.

To watch only this case on the current build, use a fresh output directory::

   cmake --build build/linux-gui --target GmatQtNasaUnitInputTests -j4
   python3 src/qtgui/tests/RunAcceptance.py --build-dir build/linux-gui \
     --filter '^QtGui[.]NativeNasaUnitInputs$' \
     --nasa-review src/qtgui/tests/NasaUnitInputTraceability.json \
     --visible --start-minimized --pause-ms 600 --max-checkpoints 5000 \
     --max-screenshots 0 --output build/gui-acceptance/units-watch

Broader NASA qualification, scientific domain matrices, unavailable historical
procedures and authority-dependent projects remain deferred with their rows
visible. The active 217-recovery/37-local-contract goal is not complete.
