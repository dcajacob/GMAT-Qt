Mission Tree backend operations and FindEvents summary repair
==============================================================

Coverage remains **1,019/1,158 (88.0%)**, with 139 original requirements open
and **82 more needed**. Historical operations remain **1,816/2,367 (76.7%)**.
All Others!E196 remains open: all six operations must hold for the complete
command census. All 59 prerequisite holds and scientific/core/current-build/
full-NASA qualification gates remain open.

Eight contexts and independent evidence
--------------------------------------

Set, BeginFileThrust, EndFileThrust and FindEvents each pass at root and inside
an If branch. Tests exercise actual context Open/reuse, double-click Open,
tree Close, before/after insertion with exact source preservation and parent
identity, Delete Cancel/confirmation, Undo/Redo, actual context Summary and
Unicode save/reopen. Each restored mission executes twice.

Six unattended source-v1 cases passed and were reused. Two FindEvents summary
cases failed. After the repair, those two and the existing Script Event
FindEvents regression passed unattended at source-v2. All nine passed visibly:
**16 new passes**, two regression passes, **32 raw twelve-field result rows**
and **192 independent summary components**. Visible windows start minimized,
remain resizable and non-fullscreen; nine process maps identify loaded images.
No screenshots, runtime binary copies or large-kernel copies are retained.

The independent orbit uses literal mu=398600.4415, radius=7000 km, duration
20 seconds for Set/file-thrust and 6500 seconds at 45-degree inclination for
FindEvents. Summary epochs are 0, 0, 10 and 6500 seconds respectively. The
original bounds remain 1e-4 seconds, 2e-5 km, 2e-8 km/s and 1e-12 for scalars;
summary display bounds remain 2e-5.

Selective Set imports Cr=1.4 while retaining the epoch and Cartesian state,
although the input file deliberately supplies different values. The zero-force
file-thrust history consumes fuel at 0.5 kg/s for ten seconds: 100 kg becomes
95 kg, then remains 95 kg at twenty seconds. Forty extra rows contain 128
independently checked numeric fields across modes, phases and positions.
FindEvents retains one East contact from 2773.493088543415 to
3226.2079913020134 seconds after 2000-01-01 12:00 UTC, duration
452.7149027585983 seconds, with a 0.002-second bound. Eight new contact
intervals and both affected-regression intervals pass the retained oracle.

Exact demonstrated repair
-------------------------

FindEvents::Execute successfully located events but omitted state capture;
the actual summary said it was unsupported or the command did not execute.
The sole production change enables BuildCommandSummary(true) after successful
LocateEvents and before return. Exception behavior and event-search algorithms
are unchanged. Original failures, unchanged test assertions, exact preimage,
one-line patch and before/after runtime identities are retained.

Audit and continuation
----------------------

AuditCommandBackendOperations.py verifies source versions, exact core delta,
raw reports and contact intervals, independent geometry, stdout/JUnit, actual
native exposures and loaded images. Four corrupt controls reject wrong state,
missing Close, missing reopen and wrong position. evidence-audit-v2.json reports
passed-bounded-backend-operations with no qualified keys.

Audit v1 failed because it incorrectly compared the intentionally retained
pre-fix core source hash against the post-fix source for every run. Its source,
manifest and failure remain archived. Audit v2 explicitly binds source-v1 to
the exact core preimage and all later runs to the fixed source. No GUI replay
or assertion change was needed for this audit correction.

The bounded stage is build/nasa-coverage/coverage95-20261103-command-backends.
Its evidence-inputs-v2.json binds regular files only; runtime symlinks are
excluded. stage-receipt-v2.json retains storage, source hashes and all 59 holds.
The authoritative ledger remains the default-orientation coverage-snapshot-v1.
The prior command-operations whole-parent-worklist.json retains all 53 factory
entries and the exact original parent fingerprint. Remaining work includes
navigation commands, SaveMission, structural/unique-starter applicability and
missing tree Close/context Summary routes in older lifecycle cases. Reuse the
eight completed function/dynamic/backend families at their recorded versions.

Replay
------

.. code-block:: sh

   cmake --build build/linux-gui --target GmatQtNasaCommandBackendOperationTests -j4
   python3 src/qtgui/tests/RunCommandBackendOperations.py
   python3 src/qtgui/tests/RunCommandBackendOperations.py --visible
   python3 src/qtgui/tests/AuditCommandBackendOperations.py \
     --stage build/nasa-coverage/coverage95-20261103-command-backends \
     --output /tmp/command-backend-audit.json

The eight individual registrations are
QtGui.NasaCommandBackendOperations<Set|BeginFileThrust|EndFileThrust|FindEvents><Root|Branch>.
The existing regression retains its original registration and assertions.
The full goal is active and incomplete.
