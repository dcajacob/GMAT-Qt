Recommended acceptance campaign: VF13ad progress and report contracts
=====================================================================

The audited union is **887/1,158 requirements (76.6%)** and
**1,812/2,367 historical scopes (76.6%)**. Six newly verified local GUI contracts
are FRR!E294-E299: optional VF13ad progress with current controls, desired and
achieved constraints, cost and iteration number, plus a corresponding
per-iteration file report. Local contracts are now33/37 verified. Four units/
documentation contracts and57 evidence audits remain;39 audited gaps remain
open. Original scopes and denominators are unchanged.

The reporting fix
------------------

The retained output already contained controls, costs and iteration numbers,
but its constraint text exposed only residuals. New complete VF13ad iteration
blocks explicitly name the physical controls, cost, constraint operator,
desired value, achieved value and residual. Verbose style additionally includes
scaled controls. The native completed-iteration counter is used directly:
zero is the initial nominal evaluation. The same complete block reaches the
message window and the open report file when ShowProgress is true.

Two core files change in this increment:

* ``src/base/solver/InternalOptimizer.cpp`` builds and emits the complete block,
  gated by VF13ad, initialization and ShowProgress. Constraint reconstruction
  uses the existing solver-window conversion from nominal residuals to physical
  achieved values. File writes are checked.
* ``src/base/command/Optimize.cpp`` requests that block immediately after its
  existing physical reporting-cache refresh. The first new runtime check caught
  a stale last-probe physical Y value mixed with nominal cost/constraint values;
  moving emission after the refresh fixes the cause without loosening truth.

There are no header, object-layout or vtable changes. These are core changes
and require scrutiny: ``vf13ad-report-core.patch`` is the exact delta against
the immediately preceding state, with both before files retained.
``vf13ad-report-core-review.json`` records source identities and limitations.
Optimize was already in the previous97-file core inventory; InternalOptimizer
adds one new file, bringing that inventory to98. The other96 prior core files
remain unchanged. Technical approval and general scientific qualification
remain explicitly ungranted.

Actual GUI and independent result checks
----------------------------------------

``QtGui.NativeNasaVF13adProgressForward`` and ``Central`` both passed visibly.
Each edits ShowProgress, ReportStyle and a Unicode ReportFile path through the
actual resource editor, preserves pending/Apply/Undo/Redo/save/reopen behavior,
and runs reporting enabled and disabled. Both use nonidentity additive and
multiplicative scales for two controls. The constrained problem is::

   minimize (X-3)^2 + (Y+2)^2
   Upper = X+5 <= 7
   Lower = 2X-1 >= 1
   EqValue = Y+3 = 2

Its independently known minimum is X2, Y-1, cost2. Every raw trial retains
the algebraic constraint/cost identities and sentinel41. Every complete
reported iteration is checked against independently observed live nominal
native decisions, including physical/scaled values, all three distinct
operators, desired/achieved/residual values and cost. Iteration numbers must
be consecutive from zero and agree with the native completion count.
Every message block must equal the corresponding file block.

Debugger boundaries let the production queued message delivery run during the
mission and independently observe nominal decisions. No solver callback is
replaced. Each message frame must arrive while the mission is running.
Disabled reporting must publish no iteration blocks; its legitimate absence
of a report file is recorded explicitly. Both paths retain the configured
variable graph and source/file bytes.

The visible run used the host-aware scale3 configuration and a3966-by2506
resizable nested desktop, initially Iconic/minimized with no fullscreen state.
Forward and Central passed in approximately33 and41 seconds, with354 recorded
native exposures and zero screenshots. No runtime/binary snapshot was copied.

The existing affected ``SolverReportStateForward`` and ``Central`` regressions
also pass, retaining their original unscaled/nonidentity-scaled optimum and
every-final-report-block assertions. Existing unrelated GUI cases were not
replayed. Eleven actual qualification corruptions reject changed sources,
logs, rows, tests, exposure/window policy and unsupported full-NASA claims.

Reusable execution and evidence
--------------------------------

The new executable is an ordinary CMake/CTest target and a dependency of
``check-qt``. To run just the new automated cases::

   ctest --test-dir build/linux-gui -C Release \
     -R '^QtGui[.]NativeNasaVF13adProgress(Forward|Central)$' --output-on-failure

For visible execution, use a new output directory::

   python3 src/qtgui/tests/RunAcceptance.py --build-dir build/linux-gui \
     --filter '^QtGui[.]NativeNasaVF13adProgress(Forward|Central)$' \
     --nasa-review src/qtgui/tests/NasaVf13adProgressTraceability.json \
     --visible --start-minimized --max-screenshots 0 --output /tmp/vf13ad-report-check

The explicit review option leaves the original historical review unchanged.
That older manifest correctly rejects newer time-converter hashes; its old
time claims were not rehashed to the current implementation. The new review
qualifies only these six rows from the actual two-case receipt.

The campaign directory retains the visible receipt and raw small text/CSV
results, exact log/JUnit identities, scoped before/after source and test/base/
provider binary identities, and about649KB of matching text-source inputs.
No runtime binaries are copied. The campaign builder rechecks all six declared
native proofs, both exposed passing cases, archived input/authority identities,
window policy and absence of source/runtime drift. Existing harness and
historical-recovery qualifications keep their separate validation paths.
Superseded development summaries are losslessly compressed, saving about20MB;
failure receipts, logs, JUnit and numeric artifacts remain intact. The retention
manifest records original and compressed identities. The complete campaign
directory, including earlier increments, is about39MiB.

``vf13ad-progress-qualification.json`` records this targeted source-versioned
qualification; it does not claim an entire current-binary regression or prove
source-to-binary provenance for every runtime dependency.

Limits and remaining work
--------------------------

These are selected, complete capability witnesses using Verbose style and
Forward/Central differences. RunInitialGuess, nested VF13ad reporting, every
other style, nonconvergence and report I/O error paths were not newly qualified.
Native OS input, hardware GPU, other platforms, arbitrary optimization/science
domains, the current private NASA corpus and formal NASA approval remain open.

Development found and corrected one C++ API-call build error, the stale-control
report-cache problem and an overly strict new disabled-file fixture expectation.
The sandbox initially blocked private X11; approved execution outside it worked.
The original GUI tests/assertions are intact. The only existing runner change
adds explicit repository-relative review selection; its default behavior and
original source/proof checks remain intact.

The production-Qt preservation check accounts for the earlier PlotWidget marker
repair. MainWindow also has a pre-existing difference from the older sealed
candidate: the current visible receipt binds its actual bytes, without claiming
that the older candidate used them. Neither file was edited in this increment.

The four units/documentation contracts and57 remaining recovery candidates
still require work. The active217+37 goal remains unfinished, with every earlier
gap and deferred project retained.
