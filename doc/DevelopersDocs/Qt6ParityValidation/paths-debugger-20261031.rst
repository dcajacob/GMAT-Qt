Paths and Debugger stabilization — 2026-10-31
===========================================

The three selected failed registrations now pass: ``QtGui.Paths``,
``QtGui.NativeDebugger`` and ``QtGui.Debugger``. All three ran visibly on an
initially minimized, resizable Xephyr desktop with software X11, host scaling
and zero automatic observer screenshots. The desktop was maximized within the
window manager, without fullscreen state. No unrelated baseline passes were
replayed. This is a bounded repair batch, not a full current-build qualification.

Repairs and evidence
--------------------

Paths crashed inside ``QTreeWidgetItem::takeChild`` while ``refreshTrees``
reparented Luna beneath Earth after rebuilding the resource tree. Live GDB
captured both the stack and source locals. The fixture retained a resource-tree
iterator across tree deletion and repopulation; another output-tree iterator
also outlived later rebuilds. Both traversals now end before the corresponding
tree can be rebuilt. All 74 original assertion calls remain textually exact.
The test completes ordered function-path changes, results 10 then 15, output
and log relocation, startup rollback/export/import, Unicode child launch and
Stop/recovery. No production Qt tree code changed. A tiny standalone iterator
probe did not reproduce the complete fixture crash; it is not claimed as proof.

The Debugger exception occurred in Yukon's ``MaxUserConViolation``, called by
``PrepareForNextIteration`` during the ordinary reference optimization. The
previous log's ``Double.gmf`` location did not identify the throwing code.
An optimization with no user constraints has neither user-violation entries
nor elastic variables. Two helpers nevertheless indexed their first element.
``NLPFunctionGenerator.cpp`` now returns zero for these two empty sets.
Nonempty branches, convergence tolerances and objective formulas remain exact.
The new empty-set regression failed before the repair and passes afterward,
covering unbounded and variable-bound-only cases. Both complete original
Debugger variants pass, including their unchanged iteration-report comparison,
analytic solution bounds, stepping, Pause/Stop and recovery assertions.
``DebuggerTests.cpp`` is unchanged, including all 80 assertion calls.

The Paths GUI route now uses the existing acceptance platform guard. Its
separate ``ScriptAssets`` branch retains its offscreen assertion and has an
explicit offscreen CTest label. Existing test names and selection remain intact.

Validation
----------

Nine unique registered checks pass:

* The three repaired GUI registrations, visibly.
* ``QtGui.ScriptAssets`` on its required offscreen route.
* ``QtGui.NasaOptimizerRuleFeasible`` and
  ``QtGui.NasaOptimizerRuleNonlinear``, visibly, as affected constrained controls.
* ``QtAcceptance.YukonQP``, including two new empty-set configurations and the
  original seven independent QP cases.
* ``QtAcceptance.RunnerContract`` and ``QtAcceptance.NasaTraceability``.

The first RunnerContract attempt could not connect its native X11 subprocess
from the filesystem sandbox. The identical contract passed on the host with
core dumps disabled. Its failed attempt is retained. The ordinary acceptance
runner initially stopped before execution when its source checks detected the
changed Yukon file; that history is also retained.

Thirteen earlier NASA claim references now point to the exact original source
at ``paths-debugger-20261031/NLPFunctionGenerator.before.cpp``. Their digests,
proof assertions, expected values, scopes and historical receipts remain exact.
The final source-binding check succeeds. Those claims have not been upgraded
to qualification of the repaired build. No NASA coverage credit is added.

Core scrutiny and remaining gaps
-------------------------------

Exactly one of the 88 tracked core files changed; all other 87 hashes match the
sealed handoff. The incremental core patch has two empty-set guards, with no
layout or ABI changes. Technical approval is still pending. The existing Yukon
false-convergence and wider-infeasibility findings remain open; these guards
do not repair them. VNB acceleration delivery, VF13ad report truth and cold-cache
NotAKnot qualification also remain open.

Eighteen of the original 21 failing registered cases remain unresolved. The
sealed handoff remains 1,722 passes and 21 failures; these new receipts supersede
only the three selected failures for this batch. Passing evidence for other
versions is retained, not a claim that this entire current build passes.
The earlier bounded NASA ledger remains 925/1158 requirement scopes (79.9%)
and 1833/2367 historical scopes (77.4%); broader NASA qualification is deferred.
Clean-build Linux qualification, independent broader scientific review,
physical host input/GPU, Windows/macOS and stress/input-domain gates remain open.

Review artifacts are in ``build/stabilization/20261031-paths-debugger``:
``summary.json``, ``increment.patch``, ``core-increment.patch``,
``core-change-review.json``, ``assertion-audit.json``, live diagnostic stacks,
``visible-fixed/results.json``, ``affected/results.json``, support JUnit files
and ``sealed-handoff-verification.json``. The sealed closeout package is intact.
No global core dumps, recurring automation, commit or push was enabled.

Targeted replay
---------------

From the repository root, build only the affected executables and run the
three repaired cases. Restore the minimized test desktop to watch:

.. code-block:: bash

   cmake --build build/linux-gui --target GmatQtPathTests GmatQtDebuggerTests -j 2
   python3 src/qtgui/tests/RunAcceptance.py --build-dir build/linux-gui \
     --visible --start-minimized --max-screenshots 0 --pause-ms 100 \
     --max-checkpoints 5000 --filter '^QtGui\.(Paths|NativeDebugger|Debugger)$'

The default result directory is a fresh temporary directory. The complete
watch/check acceptance targets remain available; this batch did not run them.
