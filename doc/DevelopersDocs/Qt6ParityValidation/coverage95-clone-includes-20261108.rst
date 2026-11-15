Cloning resources defined in included scripts
============================================

The Clone dialog can now copy the successful Build's included statements into
the main script when their dependency or mission order prevents an ordinary
clone. An unchecked choice explains that the included files remain unchanged,
that later edits to those files will no longer update the copied script, and
that Undo restores the include links. Copying and cloning commit together.
The existing include-preserving behavior remains the default API behavior.

MainWindow.cpp/.hpp add ResourceCloneSourcePolicy::CopyIncludedStatements and
retain the existing overloads. Reconstruction uses the exact successful Qt
Build capture, comments the captured root/nested include directives, preserves
statement order and root text, and then uses the existing clone transaction.
Incomplete, ambiguous and runtime include captures remain rejected. No files
are reread to construct the copy. Existing parser, ASCII payload, failed-Build,
dirty-panel and resource eligibility guards remain in force.

Seven new native GUI cases cover configuration-only includes, separate mission
includes, mixed and nested includes, included Array expressions, a file changed
after Build, and rejection/recovery for a missing mission boundary. They use
the Resource Tree menu and actual Clone dialog, explicit consent, Cancel,
configured and executed Array values, one mission execution, atomic Undo/Redo,
Unicode main-script reopen and unchanged include-file bytes.

All seven pass headlessly and visibly. The unchanged original ResourceClone
regression also passes in both profiles: 16 selected passes. Independent retained
CSV checks cover 14 files and 84 literal scalar values. The visible nested,
resizable desktop requested an initially minimized window and recorded 106 native
window exposures with no screenshots. Qt-generated input is not native OS input
qualification. No broader platform, scientific or full-build approval follows.

The profiles have identical 1,178 source and 109 binary identity entries. The
source capsule contains 1,177 non-ELF entries; the compiled observer is recorded
by identity only. The earlier original headless pass keeps its exact two older
new-test/CMake versions. Its executed original witness and all production/runtime
identities agree with the final profiles; only the new test binary changed.

Seven failed predecessor cases remain retained. New fixture payloads initially
violated the existing ASCII include rule; new menu/checkbox drivers targeted the
wrong mechanism or hit position; the proposed implicit mission fixture was
rejected by the native parser. These corrections changed only new witnesses.
The final boundary case requires that rejection and then repairs its own input.
One new artifact reader initially assumed both test families used the same runner;
its correction required no native rerun. No original assertion, fixture or
numerical tolerance changed. All 1,334 protected original input hashes match.

Evidence and the read-only publication verifier are in
``build/nasa-coverage/coverage95-20261108-clone-include-reconstruction``.
``component-audit-v1.json`` checks raw files, values, capture revisions, parser
recovery, executed source versions and runtime identities. ``publication-v1.json``
binds the component artifacts. Reuse these completed executions unless a relevant
owner changes or a new defect requires a targeted run.

Canonical acceptance remains 1,076/1,158 (92.9%), with 82 original requirements
open and 25 more required for the unchanged 95 percent target. This component
adds no whole-row credit to All Others!E120/E156/E157: their original any-object
scope remains open. Undefined requirements and all wider acceptance gates remain
open. The active campaign continues without a restart or a new permission request.
