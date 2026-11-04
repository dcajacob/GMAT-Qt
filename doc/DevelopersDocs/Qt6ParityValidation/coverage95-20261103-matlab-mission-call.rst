MATLAB call creation and configuration in the Mission Tree
=========================================================

The independent admission and rebuilt ledger qualify exactly ``All Others!E180``
(FRAC-7.1.20). Coverage is **1,022/1,158 (88.3%)**, with 136 original requirements
open and **79 more needed** for the unchanged 1,101-row target. Historical operation
coverage remains 1,816/2,367. The full 95% campaign remains incomplete.

Exact requirement and retained scope
-----------------------------------

The original workbook leaf says "20)  Matlab function call". Its parent E160
requires Mission Tree creation/configuration at any point in the mission sequence.
The exact unchanged text, fingerprint, parent, sibling E179 and copied historical
bundle gate are retained in ``row-scope-review-v1.json``. The independent reader
checks the complete reviewed 14-case conjunction in both modes. General MATLAB I/O,
process sharing/isolation, reverse interface and optimizer behavior belong to
separate original sibling rows and remain open. The parent E160 remains open.
The copied MATLAB planning bundle gate remains recorded and false.

``exact-row-closure-overlay-v1.json`` retains all139 prior per-row records, original
texts/fingerprints/gaps/closure histories and all59 original hold identities. Three
holds are now resolved (E152, FRR E478 and E180), 56 remain unresolved, including34
MATLAB rows. The previous overlay is hash-bound. Every other row is unchanged.
All321 Parameter whole obligations, E477/E491/E406, scientific/core approvals,
full-current-build/full-NASA gates and source conflicts remain open.

Actual GUI and native conjunction
--------------------------------

The 14 cases cover Beginning, Middle, End, After, AfterBranch, If, Else, For,
While, Target, Optimize, Nested, ScriptEvent and NestedScriptEvent. They use the
actual tree context menus, MATLAB template and function resource chooser; ordered
input/output pickers with Up/Down and three Cancels; name and isolated draft;
Apply and idempotent Apply; exact Undo/Redo; Unicode save/reopen; native named
CallMatlabFunction source mapping and hierarchy. Middle also edits an existing
named call and verifies pending-rename Cancel isolation. Script Events use actual
wrapper/body editors and expanded native BeginScript/EndScript boundaries.

Each case executes initial, no-explicit-Build repeat and reopened missions. Run
automatically builds; the repeat is not a claim of retained sandbox reuse. Real
MatBeta receives ordered inputs (-3,7), returns (-31,11) in the selected output
order, and writes an independent call log. The auditor checks native values, exact
raw report contents, the actual readonly ReportViewer, source/history and .m-byte
preservation, JUnit, terminal observers and actual mapped project/SDK image hashes.
For/While/Nested make two calls per mission. Target/Optimize RunInitialGuess have
both nominal and publication passes, hence two calls and Keep=43. Beginning and
End/After have intentionally different report timing, checked exactly.

The admitted evidence contains **28 positive GUI contexts, 84 missions, 114 real
MATLAB calls and 448 visible native exposures**. Visible windows start minimized,
remain resizable and nonfullscreen. There are zero screenshots, runtime-binary or
kernel copies, and no terminated licensing services. Ten corrupt-proof controls
are rejected. Actual run/source/PID/batch-return bindings are retained in
``row-admission-audit-v2.json``; mixed-batch failures are not promoted to passes.

Source-versioned reuse
---------------------

Headless Beginning/If/Else/For/While/Nested reuse remaining-headless-v1 (source6);
End reuses solver-oracle-before-v1 (source7); After/AfterBranch/Target/Optimize use
affected-headless-v1 (source8); NestedScriptEvent uses events-headless-v1 (source10);
Middle/ScriptEvent use picker-headless-v1 (source11). Visible Middle/ScriptEvent
use picker-visible-v1 (source11); the remaining12 use remaining-visible-v2 (source12).
The eight relevant production C++ paths have identical hashes across these sources.
All source capsules v1..v12 remain; v9 was build-only and its failed compile is
retained. Final offline bookkeeping does not claim a new native execution.

Production changes and exact failure history
-------------------------------------------

The original tree had no MATLAB call template. MainWindow.cpp conditionally adds
one only when CallMatlabFunction is available. The keyword-free command serializer
then lost the configured call label, making the native command uneditable.
GmatCommand.cpp extends its existing assignment-label branch to CallMatlabFunction.
The exact pre/post patches, original lost-label/native-node failure and identical
test drivers across both repairs remain. No new API/layout/formula/parameter flag
is introduced. Current core inventory contains248 paths:247 prior hashes unchanged,
one existing GmatCommand.cpp increment. Global startup and Fmincon's platform
guard remain unchanged.

All55 pre-existing full-catalogue require expressions remain text-identical.
The original49 new-call require expressions remain in order with one precise
End-span correction: the exact source includes its authored preceding tail comment.
The other48 are text-identical; current totals are57 call and56 catalogue checks.
The original incorrect Target/Optimize one-call/Keep=42 oracle remains with native
raw evidence; the corrected exact two-call/Keep=43 oracle follows the existing
nominal/publication contract. No numerical bound was loosened.

Earlier fixture reveal, template, label, oracle, private-startup, picker and
missing-QDialog build failures remain. The first catalogue wrapper mutated only
its already-private startup through a symlink; its before hash and after bytes,
but no separately archived before bytes, survive. It is unqualified. The corrected
wrapper makes a regular private copy and guards original/selected startup identities.

One visible selection was gracefully interrupted on a Replace-command-text modal.
Its partial trace/process map, terminal130, read-only X11 dialog diagnostic and
exact process-group check survive. No SDK executable was in that group. The older
capture does not contain the fresh command-draft text or conclusively establish its cause.
New fixture mouse selection and strict empty/unmodified draft guards pass. No
unobserved cases from the interrupted selection receive credit.

The independent admission reader's first attempt failed an exposure census:
headless raw Qt exposures were incorrectly compared with visible-only native
exposures. The failed reader/log/qualification are immutable. Version2 checks raw
Qt exposures in both modes and native exposures only in visible mode; no GUI case
or assertion changed and no native rerun was needed.

Broader full catalogue remains failed
------------------------------------

The three existing full catalogue scenarios run in both modes: six failed contexts,
60 exact menu queries. Every query has the new MATLAB entry once and its correct
preview. The loaded C-API registry also exposes PrepareMissionSequence, for which
the Qt template is missing. Each full catalogue still fails for precisely that
omission; no hidden-command exemption or weakened assertion was added. These
auxiliary failures remain unqualified and confer no parent/current-build credit.
The earlier picker-specific catalogue failure is retained separately.

Replay and remaining work
-------------------------

Use a working licensed MATLAB C Engine installation, csh, the SDK plugin and a
visible display route. Use a fresh output directory for every replay::

   cmake --build build/linux-gui --target GmatQtNasaMatlabMissionCallTests GmatQtNasaMatlabScriptEventCallTests -j2
   python3 src/qtgui/tests/RunMatlabMissionCallAcceptance.py --filter '^QtGui.NasaMatlabMissionCall' --output build/gui-acceptance/matlab-mission-fresh-headless
   python3 src/qtgui/tests/RunMatlabMissionCallAcceptance.py --visible --filter '^QtGui.NasaMatlabMissionCall' --output build/gui-acceptance/matlab-mission-fresh-visible
   python3 src/qtgui/tests/AuditMatlabMissionCallAcceptance.py --qualification build/nasa-coverage/coverage95-20261103-matlab-mission-call/qualification-v2.json --output /tmp/matlab-mission-fresh-readback.json
   python3 src/qtgui/tests/TrackCoreChanges.py --verify build/nasa-coverage/coverage95-20261103-matlab-mission-call/core-inventory-v1

The default runner includes the three known-failed auxiliary catalogues. No prior
Object/Literals I/O probes or full-catalogue assertions are disabled. The frozen
run-with-maps scripts require their exact recorded source/images; fresh sources
need fresh custody. Fresh native success alone does not update the ledger.

Next priority is the actual fmincon Qt callback integration, followed by complete
original-row configuration/Vary/progress/report acceptance. Qt currently inherits
an unsupported external-server hook; the engine evaluation blocks its thread, and
the MATLAB scripts use legacy Unix MEX/Windows DDE callbacks. Removing the Linux
platform guard alone does not implement callbacks. General I/O/property outputs,
missing original FRR-36 reference, Unix single-use and reverse-interface contracts
also remain open. Keep all qualification gates and the full1,101-row target.
