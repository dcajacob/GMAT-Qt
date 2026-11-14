Bounded Linux solver closeout
==============================

The supported Linux behavior is verified and a newly exposed GUI rollback bug
is repaired. Twelve new native rejection contracts pass unattended; all twelve
also pass visibly, together with four existing solver-editor transaction tests.
Original assertions are preserved. No numerical core or interpreter semantics
were changed.

The original twelve positive nesting hypotheses remain disabled, unexecuted and
uncredited. They are unresolved NASA source conflicts, not passing tests and
not newly implemented solver capabilities. This closes the bounded supported-
behavior investigation and repair; it does not resolve the missing historical
keyword intent or qualify full NASA GUI coverage.

Source decision
----------------

The shipped Interpreter::ValidateSolverCmdLevels explicitly rejects solver
sequence commands inside ordinary control-logic branches. The shipped wx
MissionTree menus offer the solver leaves inside the enclosing Target/Optimize
sequence, and Using_MissionTree.xml gives broader containment rules. The 2012
RTTM labels the twelve operations but does not supply their private keyword
procedures or expected outcomes. In particular, H2012:R1279 records Passed,
H2012:R1317 records NotRunnable, and the other ten rows have no recorded result.
Those records cannot establish the intended outcome of the new reconstructed
positive missions on this engine.

The bounded conformance decision is to verify the interpreter's existing
rejection and the GUI's preservation of the user's work. Supporting positive
nesting would be a separately specified engine feature with scientific review,
not an appropriate test waiver or inferred qualification repair.

Every case has a source-linked disposition in
``solver-disposition-20261031/cases.json``. The twelve original CTest names,
disabled flags, fixture bytes, case fingerprints and positive assertions are
unchanged. Twelve distinct ``NativeNasaSolverNestingRejection`` registrations
exercise the same reconstructed positions through separate negative contracts.
Their passes never substitute for the original hypotheses.

Production repair
------------------

The new unchanged-model assertion failed after Apply rejected a nested Vary.
The actual configured spacecraft IDs were restored correctly, but the cached
canonical mission text still contained temporary reconstruction IDs. The
retained diagnostic shows ``Sat.NAIFId`` changing from -10001001 to -10003001
and its reference-frame ID from -9001001 to -9003001 in that cache alone.
An ensuing edit could therefore start from resource text inconsistent with
the live model.

MainWindow::applyModelScript now refreshes the canonical resource text after
restoring implicit IDs on its failed-edit path. It retains the original editor
and draft. No solver placement restriction, equation, state propagation,
covariance calculation or numerical tolerance changes. The native interpreter
and seven production shared libraries remain byte-identical to the previous
reviewed candidate.

Verification
--------------

* Twelve new contracts verify the exact native nesting diagnostic for Apply
  and OK, retained typed draft/editor owner, pending and rejected Cancel paths,
  unchanged source/undo/redo/modified state/disk/configured resource text and
  values/native command hierarchy/canonical model, and a successful rebuild
  of the previously accepted baseline. These are rejection/rollback contracts;
  they do not claim solver convergence or positive nested-command execution.
* The repaired VaryTarget canary passes; its pass is reused for unattended
  accounting while the other eleven new cases run once.
* Sixteen affected cases pass visibly: those twelve plus the existing
  SolverContext Root, Target, OptimizeAddObjective and OptimizeCapacity cases.
  The nested desktop starts minimized and remains resizable. No screenshots
  or binary/runtime snapshots are retained.
* All 41 traceability support assertions pass. The original positive fixtures,
  existing test sources, interpreter/core sources and prior disabled flags are
  checked unchanged by the closeout audit.
* The prior sealed handoff verifies read-only: 36,111 files and 44,757 symbolic
  links. The integrity verifier executes zero product tests.

Evidence root: ``build/release-check/20261031-solver-disposition/``.
``canonical-diagnostic/`` retains the failing new assertion and before/after
cache bytes. ``automated-summary.json``, ``visible/``, ``completion-audit.json``,
``increment.patch`` and ``effective-junit.xml`` contain the final evidence.
The initial sandbox attempt could not connect to private X11 before assertions
ran; the permitted native route succeeded. That setup abort left one approximately
1 MB system-managed crash dump outside the workspace. Later native and visible
runs explicitly disabled core dumps. The container prevents privileged cleanup.

Reproduce only the new automated contracts::

   cmake --build build/linux-gui --target GmatQtNasaSolverNestingRejectionTests -j4
   python3 src/qtgui/tests/RunAcceptance.py --build-dir build/linux-gui \
      --max-screenshots 0 --filter '^QtGui\.NativeNasaSolverNestingRejection' \
      --output build/gui-acceptance/NEW-NESTING-AUTOMATED

Watch the complete affected scope, starting minimized::

   cmake --build build/linux-gui --target GmatQtNasaSolverNestingRejectionTests GmatQtNasaSolverContextTests -j4
   python3 src/qtgui/tests/RunAcceptance.py --build-dir build/linux-gui \
      --visible --start-minimized --scale 1 --max-screenshots 0 \
      --filter '^QtGui\.NativeNasa(SolverNestingRejection[A-Za-z]+|SolverContext(Root|Target|OptimizeAddObjective|OptimizeCapacity))$' \
      --output build/gui-acceptance/NEW-NESTING-VISIBLE

Final accounting and remaining gaps
------------------------------------

The current 1,770 registrations have source-versioned effective outcomes of
1,758 passed and twelve skipped positive source conflicts, with zero known
active failures. This composes the prior full baseline and affected repairs,
four new core API contracts, the half-scale XY repair and this solver closeout.
Reused passes retain their original source/runtime/binary identities. It is
not an uninterrupted all-green full replay of the final binaries.

No new requirement-scope or historical-scope credit is awarded for rejecting
unsupported positive hypotheses. The retained handoff remains 742/1,158 (64.1%)
requirement scopes and 1,803/2,367 (76.2%) historical scopes. The 2012 source
conflicts remain open. Broader arbitrary DPI/marker/platform coverage, distant-
origin precision, general scientific/frame/force domains, physical GPU/native
OS paths and full current NASA qualification remain unqualified. Independent
technical/scientific approval of the accumulated 97-file core inventory is
still pending. See reviewed-candidate-20261031.rst, core-followup-review-20261031.rst
and xy-half-scale-20261031.rst for their preserved evidence and limits.

The normal build-qt-acceptance target includes the new executable. A future
deliberate full replay should first rebuild that whole target and use a new
output directory. No broad passing-suite replay was needed for this repair.
The strict-coverage gate continues to fail while qualification gaps remain.
