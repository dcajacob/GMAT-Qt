Output and plot command operations, SaveMission replay and CommandEcho summary
==============================================================================

Coverage remains **1,019/1,158 (88.0%)**, 139 open and **82 more needed**.
Historical operations remain **1,816/2,367 (76.7%)**. This increment admits no
original requirement key. All Others!E196 retains its complete any-command
conjunction, original fingerprint and 53-entry factory census. All 59 prerequisite
holds and scientific/core/current-build/full-NASA gates remain open.

New native scope
----------------

Twenty-four new contexts exercise root and nested If placement for ClearPlot,
PenUp, PenDown, MarkPoint, Write, Global, Report, Save, Toggle, SaveMission and
both On and Off CommandEcho instances. These are eleven distinct factory classes;
the two CommandEcho instances have separate tests. The original retained mission
fixtures and all older lifecycle/Script Event assertions are unchanged.

Each context exercises actual tree Open/reuse, double-click Open, tree Close,
Insert before and after, Delete Cancel/confirmation, Undo/Redo, context Command
Summary and Unicode save/reopen. Insertion and deletion preserve the exact
unrelated source bytes, selected sibling order and parent identity. Both restored
mission phases execute with independent numerical and command-specific checks.

Twenty cases passed unattended at source-v1 and were reused. Four CommandEcho
cases failed in their real summary dialogs. After the one-line repair, only
those four and the existing CommandEcho regression were run unattended at
source-v2. All twenty-four new contexts and the regression then passed visibly:
**48 new GUI passes** and **two affected-regression passes**. Twenty-five process
maps identify the visible runtime images. Windows start minimized, remain
resizable/non-fullscreen, and retain zero screenshots.

Independent outputs and effects
-------------------------------

The main orbit uses literal mu=398600.4415, radius=7000 km and twenty seconds of
point-mass circular motion. The unchanged bounds are 1e-4 s, 2e-5 km, 2e-8 km/s
and 1e-12 for scalars. All scalar tails are Count=0, U=1, Cost=0, Quantity=0,
with Keep=41. The actual summaries are checked at ten seconds for the four plot
commands and twenty seconds for the other commands, with the existing 2e-5
display bound. This yields **96 raw twelve-field rows** and **576 summary
components** across positions, phases and modes.

Command-specific checks retain the following obligations:

* ClearPlot removes pre-clear points; PenUp breaks later links; PenDown preserves
  the initial gap and resumes links; MarkPoint highlights exactly the ten-second
  point while leaving the unselected control unchanged. Raw plotted elapsed-time
  coordinates and link/mark flags are retained and audited.
* Global moves the same runtime Keep object from the local store to the global
  store while Count remains local. The two debugger-observed phases are checked
  during each mission execution, including nested placement.
* CommandEcho is On before the first selected Report and Off before the second.
  Both actual transitions are observed during every new mission and the existing
  regression. The regression's original assertions are unchanged.
* Write emits the exact selected Unicode string GUI_EVENT_WRITE_41_Ω. Report
  writes the selected value 41. Save exports the six independently calculated
  runtime Cartesian components. Toggle deactivates only the selected Trace.
* A terminal root SaveMission creates the native mission file. Its nested If
  placement creates no file, preserving the established terminal-only contract.
  Stale exports are removed before each phase. Both root exports are retained,
  and the final exported script is loaded, built and executed through the GUI in
  each mode. Those **two additional export-replay rows** independently reproduce
  the original twenty-second orbit and scalar results. This checks executable
  export content, not merely file existence.

Demonstrated CommandEcho defect and exact repair
-----------------------------------------------

CommandEcho::Execute changed the echo mode successfully but omitted
BuildCommandSummary. Four new context Summary cases therefore lacked spacecraft
state even though both On/Off runtime observations had passed. The sole production
change adds BuildCommandSummary(true) after SetCommandEchoMode and before return.
Echo-mode logic, cleanup and numerical algorithms are unchanged.

Exact preimage, one-line core-v2.patch, original failures and prior/new ScriptTools
library hashes are retained. The CommandEcho preimage and old library identity
were captured during the original batch, before any production edit; the separate
capture-status record is retained rather than represented as a launch-time source
review. Source-v2 explicitly includes CommandEcho.cpp, and visible process maps
bind the rebuilt ScriptTools plugin. All native assertions remain unchanged from
the failing run. No passing old output cases were needlessly rerun unattended.

Audit and full-parent continuation
---------------------------------

AuditCommandOutputOperations.py verifies source versions, unchanged old/new test
assertions, exact core repair, original failure set, raw numerical reports,
command-epoch summaries, saved states, export replay, plot traces, Global/Echo
runtime phases, stdout/JUnit, visible exposure and loaded images. Four corrupt
controls reject changed state, missing tree Close, missing reopen and wrong echo
state. evidence-audit-v1.json reports passed-bounded-output-operations with no
qualified keys.

The stage is build/nasa-coverage/coverage95-20261103-command-output. Its
parent-operation-status-v1.json preserves all 53 original factory entries and
inherited candidate evidence. It maps **22 entries** to bounded six-operation
root/If-branch evidence at retained source versions and leaves **31 entries**
explicitly requiring complete operation or applicability evidence. This is a
continuation worklist, not a whole-parent qualification or a claim that every
placement has been proved. Aliases, structural nodes and unique starters remain
in the census.

Next continue the remaining dynamics, solver and control-flow command operations,
and resolve structural/unique-starter applicability against the original scope.
Reuse completed function/dynamic/backend/navigation/output families; do not
restart their older lifecycle suites. The authoritative ledger remains
build/nasa-coverage/coverage95-20261103-default-orientation/coverage-snapshot-v1.json.
The stage receipt retains all 59 holds and storage totals. Regular-file hashes
exclude runtime symlinks; no runtime binaries or large kernels were copied.
The full 95-percent goal remains active and incomplete.

Replay
------

.. code-block:: sh

   cmake --build build/linux-gui --target GmatQtNasaCommandOutputOperationTests -j4
   python3 src/qtgui/tests/RunCommandOutputOperations.py
   python3 src/qtgui/tests/RunCommandOutputOperations.py --visible
   python3 src/qtgui/tests/AuditCommandOutputOperations.py \
     --stage build/nasa-coverage/coverage95-20261103-command-output \
     --output /tmp/command-output-audit.json

The twenty-four cases are individually registered as
QtGui.NasaCommandOutputOperations<Family><Root|Branch> and included in acceptance
build/check dependencies. The affected regression remains
QtGui.NasaScriptEventExtraCommandEcho with its original registration.
