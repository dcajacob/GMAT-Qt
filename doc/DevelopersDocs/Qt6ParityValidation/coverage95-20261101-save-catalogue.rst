95 percent campaign: remaining Save catalogue and aliases
==========================================================

Twenty-two new Save cases pass unattended and visibly: eighteen offered builtin
function labels representing fourteen implementation classes, and four resource
aliases. The existing GroundTrackPlot creation-label case is reused. All prior
87 native-state/source receipts remain at their frozen versions; no broad GUI
replay occurred. Combined bounded Save evidence has **109 cases, 218 contexts
and 4,888 writable-property comparisons**, plus reloaded literal/interface checks.

All 83 labels in the retained offered catalogue now have bounded type/persistence
evidence: 79 match observed native type names and four are aliases. There are
81 observed root type names because existing Sun/Star and SolarSystem globals
are also tested. This is an inventory accounting result, not NASA qualification.
Requirement coverage remains **998/1,158 (86.2%)**, with 103 complete requirements
needed for 95%. Historical coverage remains **1,816/2,367 (76.7%)**. The complete
FRC!E132 any-Resource obligation stays open; no new NASA credit is awarded.

Core fix requiring scrutiny
---------------------------

One additional core review path changes: ``src/base/interpreter/Interpreter.cpp``.
A recognized command containing a builtin-function resource operand was wrongly
reclassified as a function call. For example, after Create Sign Subject, applying
Save Subject produced a CallBuiltinGmatFunction and then a string-position error
in command-name serialization. The debugger captured this exact stack.

Function inference now preserves recognized commands, while retaining inference
for shorthand calls and assignment RHS expressions. The two existing named
Sign/sign function-call regressions pass in both modes: four contexts, two
phases each, and 80 native values independently checked against [-1,0,1,-1,1].
The captured raw CSV rows match as well. No mathematical function, force equation
or integration formula changes. Exact before/after identities and the narrow
patch are in ``save-catalogue-final-audit.json`` and ``save-catalogue-core-diffs/``
under ``build/nasa-coverage/coverage95-20261101/``.

The preceding 16 Save core repair paths stay unchanged; the explicit Save review
list now has 17 paths. Interpreter.cpp was already a modified core path in the
larger campaign, and this incremental patch is recorded separately. No new Qt
production path changes. Technical/scientific and complete current-build
approval remain false.

Test harness fix and older evidence limitation
-----------------------------------------------

The first repaired Save run exposed a harness SIGSEGV. Apply intentionally
replaces its command editor, but the generic test kept a pointer to the retired
panel. Debugger evidence located the invalid findChildren access in the added
failure diagnostics. The original clean-panel assertion also used that old
pointer. The driver now rebinds to the actual subwindow widget, verifies it is
a CommandEditor, and then runs the original source/hasChanges assertion.
Text-only Apply diagnostics and the actual applied source are retained.

Every earlier native require expression remains in its original order. The
current-editor guard is additional. Four initially passing alias pilots are
superseded because their clean-panel observation used the retired pointer;
only those affected new cases were replayed. The other 18 new cases use the
fixed harness, and all 22 now pass in both modes.

**The older 87 receipts are not promoted to proof of the current editor's clean
state.** Their exact accepted source, captured typed native properties, raw
exports and rebuilt-state comparisons remain independently audited evidence.
The retired-pointer clean check is explicitly recorded as a gap; original
capsules and seals remain immutable. This increment does not claim a full
current-build replay or silently repair the historical widget observation.

Independent evidence and failures
---------------------------------

Builtin Input/Output witnesses come from their native constructor declarations,
separately from serialization and GUI property mappings. Native descriptors,
empty FunctionPath and exact interface arrays survive export/rebuild for every
new label. These persistence fixtures do not execute or qualify all builtin
calculations, input domains or private/global function behavior.

Three failed acceptance attempts remain retained: two interpreter misclassification
failures and one harness SIGSEGV. All have a passing Sign replacement. Four old
alias passes are explicitly superseded. The increment has 55 acceptance attempts
including four affected-regression contexts, plus two debugger-only failing-case
reproductions. No passing screenshots, core dumps or runtime copies are retained.

The independent checker audits both modes, all native fields, source identities,
constructor interfaces, current-editor diagnostics, affected regression values
and their raw CSVs. The function regression JSON was recorded in the before/after
source snapshots but omitted from the runner's source archive. Its unchanged
bytes are now retained separately and must match the recorded execution hash
and actual registered fixture argument; no native replay was needed for that
source-binding repair.

All 41 shared checker contracts pass. Sixteen corrupted capsules are rejected
after a valid baseline, including false whole-row/goal credit, a hidden old
editor gap, a retired-editor alias pass, a failed Sign run, missing current
Apply diagnostics, altered constructor interfaces and missing/changed regression
inputs. The original authorities, denominators, assertions and gates remain.

Storage, replay and next gate
-----------------------------

Visible execution used automatic scaling with initially minimized, resizable,
non-fullscreen nested windows. Lossless compaction of 140 duplicate reports and
traces saved 49,135,616 allocated bytes (46.9 MiB), preserving admission inputs.

Audit without native replay::

   python src/qtgui/tests/AuditSaveCatalogueEvidence.py \
     --evidence build/nasa-coverage/coverage95-20261101/save-catalogue-evidence.json
   ctest --test-dir build/linux-gui \
     -R '^QtAcceptance.SaveResourceEvidenceContract$' --output-on-failure

To watch the new cases and affected regressions, choose a fresh output path::

   python src/qtgui/tests/RunAcceptance.py \
     --build-dir build/linux-gui \
     --filter '^QtGui.NasaSaveCatalogue|^QtGui.NasaFunctionBuiltin(Sign|SignAlias)$' \
     --visible --display-route nested --display-scale auto --start-minimized \
     --pause-ms 350 --max-checkpoints 200 --max-screenshots 0 --report-every 24 \
     --output build/gui-acceptance/save-catalogue-watch

Omit visible/display flags for unattended operation. Reuse sealed evidence at
its recorded source/runtime versions instead of replaying known-good cases.

Next, inspect the complete persistence obligation: hidden editable/read-only
fields, representation/input modes, valid reference graphs and inherited Save
clauses. The offered catalogue is not the full state universe. Earlier Formation
STM, file dependency portability, disabled ephemeris output, unfired thrust,
alternate region modes and the separate FRC!E135 filename conflict remain open.
All 59 prerequisite holds remain: the optimistic ceiling is still 1,099/1,158
(94.9%) until at least two holds are substantively resolved. No unanswered
prerequisite decision is assumed. Private NASA provenance, platform-wide,
scientific and full current-build qualification remain open.
