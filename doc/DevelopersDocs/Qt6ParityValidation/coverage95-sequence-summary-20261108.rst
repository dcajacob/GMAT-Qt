Initial sequence-state summary repair, 2026-11-08
===============================================

Direct inspection of BeginMissionSequence now displays its captured initial
spacecraft state. The unchanged original structural-operation case passes.
Five focused checks pass, including existing command-summary, hidden-command
and preparation-restriction cases. The whole-mission report continues to omit
the sequence boundary. All 105 original open acceptance rows and qualification
gates remain open; the campaign is active and incomplete.

The original worksheet requires command summaries and optional spacecraft state
after each command (All Others!E196/E202/E208). BeginMissionSequence::Execute
already captures the initial state using BuildCommandSummary. Its constructor
sets includeInSummary false, and the common summary formatter applied that
aggregate-report exclusion to individual inspection as well. The formatter now
allows direct inspection for BeginMissionSequence. The aggregate exclusion and
the normal missing/unexecuted-state path remain in place. No state is computed
from the final mission state to manufacture an initial-state summary.

Verification
------------

The original BeginMissionSequence structural-operation case now passes with its
existing source/history, operation and summary assertions. The original
NasaCommandSummaryFields, NasaCommandHiddenDispositions and
NasaCommandPrepareRestrictionsOriginal cases also pass. The implementation
PropagatePolicy check now inspects a direct boundary summary containing all 16
spacecraft and checks that the whole-mission report omits the boundary, under
each Native/SingleSetup/SingleGroup policy. It passes. The original structural
case supplies the independent initial-state comparison; the 16-spacecraft check
covers summary inclusion and existing propagation behavior.

The combined latest status of the original 233-case command/boundary plan is
221 passed and 12 failed, with every case executed. This is mixed-time evidence,
not a complete new replay on this runtime. No original assertion, fixture,
tolerance, requirement text/fingerprint, worklist or tracker changed. All 1,334
original protected inputs remain byte-identical. The preceding Save-label and
spacecraft-readiness repairs remain intact.

Remaining operation contracts
-----------------------------

The remaining 12 original failures comprise five implicit-.data Save fixture
conflicts, five solver commands nested within If, and two Prepare starter-edit
cases. Preserve worksheet Save defaults and explicit LegacyData support as the
user requested; do not add duplicate outputs for old fixture expectations.
The existing solver placement checks remain unchanged.

The Prepare evidence does not demonstrate a wrong source interval being
committed. The protected case's deleted.script equals initial.script byte for
byte: deletion was rejected and rolled back. Removing its sole sequence starter
leaves a later Count multiplication before BeginMissionSequence, which the
interpreter rejects. The older operation case expects a named assignment before
the starter to commit; the protected variant explicitly requires the native
rejection at that position. Both originals remain unchanged. Preserve those
failed contracts for requirement review instead of changing parser phase rules
or committing an invalid edit to satisfy the source oracle.

Evidence and next work
----------------------

The stage is ``build/nasa-coverage/coverage95-20261108-sequence-summary``.
Read ``validation-summary-v1.json``, ``command-boundary-overlay-v3.json``,
``remaining-contracts-v1.json``, ``sequence-summary-v1.patch`` and
``publication-v1.json``. Preimages, original registrations, effective commands,
receipts, logs and bound source/runtime hashes accompany the results. Every
build/test handle is terminal. Private Xvfb does not establish user-visible
native-input, platform or whole-row acceptance.

Next verify the four integration-step plot-sampling rows FRR!E329/E330/E633/E634
using the original NasaGroundIntegrationSkipBoundary case and existing
StepPublication producer test. The exact registrations and two build targets
are retained in next-plot-plan-v1.json. Diagnose the actual producer/consumer
behavior before choosing a repair. Keep the remaining function, interpolation,
scientific, specification, external and historical-platform gates open.
