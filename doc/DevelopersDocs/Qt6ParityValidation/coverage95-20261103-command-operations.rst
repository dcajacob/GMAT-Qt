Mission Tree operation extension and dynamic-data summary repair
================================================================

Coverage remains **1,019/1,158 (88.0%)**, with 139 original requirements open
and **82 more needed**. Historical operations remain **1,816/2,367 (76.7%)**.
The complete any-command parent **All Others!E196** remains open. All 59
prerequisite holds and scientific/core/current-build/full-NASA gates remain open.

Scope and complete-parent worklist
----------------------------------

The original parent requires Open, Close, Insert before, Insert after, Delete
and Command Summary for any command. Its child references are E197 through E202.
Existing lifecycle evidence covers thirty selected recipes; it does not establish
all command classes, tree Close routes, context summaries or allowed positions.

``whole-parent-worklist.json`` retains the exact parent fingerprint and all
**53 factory entries** from the previously audited Script Event inventory. It
lists candidate prior child evidence separately from missing operations. These
are discovery mappings, not new execution credit. Aliases, structural/end nodes,
unique starters and restricted contexts retain their explicit inherited
applicability questions. No command disappears from the parent census.

This increment completes eight new contexts: built-in GMAT function calls,
GmatFunction calls, Python function calls and UpdateDynamicData, each at the root
and inside an If branch. The fixtures reuse the exact previously checked command
scripts and independent scalar results; the old lifecycle assertions and original
fixtures are unchanged.

What the new tests establish
---------------------------

The actual Mission Tree context menu and editor are exercised for each context:

* Open, repeated Open with one owner, double-click Open and tree Close.
* Insert before and after, pending isolation, exact preservation of original
  source bytes, sibling order and parent identity, Undo/Redo and restoration.
* Delete Cancel and confirmation, exact source-interval deletion, Undo/Redo and
  restoration of the original mission.
* Actual context-menu Command Summary, independent spacecraft state, preserved
  source and Unicode save/reopen followed by a second complete execution.

Independent scalar results are 4 for the GMAT function's ``1*1+3``, 7 for
Python's ``abs(-7)``, and four 0.5 components for the prescribed quaternion
product. Dynamic-data execution retains Count=99 in the subsequent report.
The circular trajectory uses literal mu=398600.4415 and radius=7000 km over
20 seconds. Original bounds are unchanged: 1e-4 s, 2e-5 km, 2e-8 km/s and
1e-12 for the scalar fields. Summaries are checked independently against the
same circle with the declared 2e-5 display bound.

Six function-call cases passed in unattended source-v1 and were reused. Two
UpdateDynamicData cases failed in their actual summary dialogs. After the core
repair, only those two cases and one affected existing regression were run
unattended at source-v2. All eight new cases and that regression then passed
visibly. This gives **16 new GUI passes**, two affected-regression passes,
**32 independent operation report rows** and **192 summary components**.
Visible windows start minimized, remain resizable/non-fullscreen, and retain
zero screenshots. Nine process maps bind the visible runtime images.

Demonstrated defect and repair
-----------------------------

``UpdateDynamicData::Execute`` previously returned directly from the display
update and never called ``BuildCommandSummary``. The command executed correctly,
but its summary lacked the spacecraft state required by the new checks.
It now preserves the display update's boolean result, calls
``BuildCommandSummary(completed)``, and returns that same result.

This is the only production change in this increment. Exact preimage, patch and
before/after image identity are retained. All new assertions are unchanged from
the failing run. The existing ``NasaScriptEventExtraUpdateDynamicData`` regression
passes in both modes and verifies that the display retains Keep=41 and Count=7
even though the later mission assignment sets Count=99. The repair therefore
adds command-state capture without replacing the display's captured values.
No numerical propagator or function algorithm changes.

Evidence and remaining work
---------------------------

``AuditCommandOperationExtension.py`` checks exact source versions, unchanged
assertions, original failures, the precise core delta, raw reports, independent
geometry/scalars, summary text, stdout/JUnit, native exposures and loaded images.
Four corrupt-proof controls reject changed state, missing Close, missing reopen
and wrong branch position. Its result is ``passed-bounded-command-operations``
with an empty qualified-key list. No cumulative ledger rebuild or legacy GUI
replay is needed for this bounded increment.

Next, use the full worklist to conjoin the remaining operation families and
positions. In particular, Set, FindEvents, RunSmoother, file-thrust commands,
SaveMission and structural/unique-starter nodes need explicit operation evidence
or source-backed applicability decisions. Existing selected lifecycle tests do
not automatically supply tree Close or context Summary for every class. The
four newly completed families should be reused, not replayed to rediscover them.

The authoritative ledger remains
``build/nasa-coverage/coverage95-20261103-default-orientation/coverage-snapshot-v1.json``.
The bounded stage is ``build/nasa-coverage/coverage95-20261103-command-operations``.
Regular artifact hashes exclude live runtime symlinks; no runtime binaries or
large kernels are copied. The stage receipt retains storage and all 59 holds.

Replay only new or affected cases
---------------------------------

.. code-block:: sh

   cmake --build build/linux-gui --target GmatQtNasaCommandOperationExtensionTests -j4
   python3 src/qtgui/tests/RunCommandOperationExtension.py
   python3 src/qtgui/tests/RunCommandOperationExtension.py --visible
   python3 src/qtgui/tests/AuditCommandOperationExtension.py \
     --stage build/nasa-coverage/coverage95-20261103-command-operations \
     --output /tmp/command-operation-audit.json

The eight cases are registered individually as
``QtGui.NasaCommandOperations<Family><Root|Branch>`` and included in acceptance
build/check dependencies. The affected existing display regression remains its
unchanged registration. The full 95-percent goal remains active and incomplete.
