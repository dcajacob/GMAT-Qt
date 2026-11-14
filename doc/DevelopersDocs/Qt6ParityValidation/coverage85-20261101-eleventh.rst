85 percent requirements handoff: complete scalar constraint family
======================================================================

The bounded campaign reaches **985/1,158 (85.1%)** source-versioned requirement
instances. Historical operation coverage stays **1,816/2,367 (76.7%)**.
The original denominators, source fingerprints and every previous qualified
addition remain intact. **173 requirement instances and 551 historical operations
remain unqualified.** This completes the requested 85 percent milestone; it does
not establish full NASA qualification or a complete pass of today's whole binary.

What closes the final two rows
----------------------------------

``FRC!E105`` requires the scalar-parameter category of NonlinearConstraint
operands. The workbook literally says ``FRR-388``; that nonexistent identifier is
retained. The conservative interpretation inventories the entire FRR-38
Parameters and Calculations chapter: 80 leaf rows, 97 declared scalar types,
all vector component expansions, and all 12 Euler sequences. Native matrices
and Gregorian strings are explicitly classified as non-scalar; their independent
NASA obligations are preserved.

162 new cases cover 96 previously missing types in both operand roles. Each
uses the actual numeric property browser and constraint editor, pending isolation,
Apply, exact Undo/Redo, Unicode save/reopen, and an independently prescribed
active bound. Raw reports check the physical operand, constrained control,
quadratic objective and sentinel. Actual optimizer progress must show accepted
left/right values and convergence. Every case passes unattended and visible.
The original selected-X, user-variable and array-element assertions are retained
and re-audited in both original cohorts. With all three original categories
complete, parent ``FRC!E104`` also qualifies. The 80 parameter-definition rows do
not receive automatic additional credit from this operand capability suite.

Repairs and core scrutiny
-----------------------------

Four core files changed in this increment. There are now 104 pending core files;
the other 100 previously recorded paths remain unchanged.

* Yukon now uses signed constraint residuals, zero residual bounds and the
  corresponding signed Jacobians consistently. The former raw-value/bound
  mixture caused a derivative-dependent error for dynamic right operands.
* Yukon retains final accepted constraint values, and internal Optimize publishes
  those values before announcing completion. The former progress display could
  retain a finite-difference probe even when the final raw result was correct.
* Geodetic iteration now stops at 1e-12 radians, improving the precision of the
  same latitude/altitude formula on an oblate body.

These changes affect numerical optimization and calculation precision. They
have not received independent technical or scientific approval. Before/after
sources, narrow patches, hashes and impact notes are in
``build/nasa-coverage/coverage85-20261101/scalar-core-review.json``. The previous
100-file list remains in the recommended campaign's ``final-core-scrutiny.json``;
the additional CInterface path remains in ``capi-summary-core-review.json``.

Eight affected registrations pass visibly: Constraints, ConstraintDynamicBound,
OptimizerRuleFeasible, OptimizerRuleNonlinear, OptimizeTwoControls,
SolverPublicationYukon, ReadonlyGeodeticAltitude and ReadonlyGeodeticLatitude.
ConstraintDynamicBound reuses the existing test's previously unregistered mode.
Every original assertion remains in place. Established unrelated scenarios were
not replayed. All failed attempts and diagnostic failures remain recorded in
``scalar-failure-history.json``; no failed receipt was rewritten as a pass.

Retained limitations
------------------------

* Numeric-literal Vary serialization can lose a digit and prevent safe GUI
  command/source alignment. This remains an open limitation. These constraint
  fixtures use a named initial-value variable while preserving their assertions.
* A distant synthetic-body ephemeris introduced approximately 28 micrometres of
  coordinate subtraction error in the new strict altitude fixture. A prescribed
  nearby origin passes its unchanged 20-micrometre assertion. Distant-origin
  precision remains unqualified by this fixture.
* The available authority is the shipped historical workbook/corpus; current
  private NASA requirements and qualification provenance remain unavailable.
* The 97 declared scalar types are a finite NASA chapter inventory, not all 284
  real-valued types in the current factory, every physical domain, all frames,
  plugins, input mechanisms, operating systems, native portals or host GPUs.
* Every original remaining source row and inherited qualification gate is kept
  in ``coverage.json``. Existing evidence belongs to its actual recorded source
  and runtime versions, not an attribution to the latest complete binary.

Reproduce and watch
-----------------------

The 162 cases are ordinary ``check-qt``/CTest registrations. Build the configured
Qt frontend and Yukon plugin, then run the unattended family with::

   ctest --test-dir build/linux-gui -R '^QtGui\.NasaConstraintScalar' --output-on-failure

Watch the family in a resizable desktop, initially minimized, with::

   python src/qtgui/tests/RunAcceptance.py --build-dir build/linux-gui \
     --filter '^QtGui\.NasaConstraintScalar' --visible --display-route nested \
     --display-scale auto --start-minimized --pause-ms 0 --max-screenshots 0 \
     --nasa-review src/qtgui/tests/NasaConstraintScalarRuntimeReview.json \
     --output build/gui-acceptance/scalar-watch

Restore the minimized test desktop to watch it. The runtime-only review awards
no NASA credit by itself. Cumulative coverage comes from the independently
audited composition, rather than the percentage printed for one replay receipt.
No passing screenshots or runtime binary copies are retained.

The complete result, original gaps, source archives, failure dispositions,
core patches, independent proof mutations and reproducibility audit are under
``build/nasa-coverage/coverage85-20261101``. Recheck the final scalar admission
without executing a mission using::

   python src/qtgui/tests/AuditConstraintScalarAcceptance.py \
     --qualification build/nasa-coverage/coverage85-20261101/scalar-family-qualification.json
   python build/nasa-coverage/coverage85-20261101/negative_scalar_qualification.py
   python build/nasa-coverage/coverage85-20261101/audit_eleventh.py
