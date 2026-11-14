Nested solver controls and command summaries
===========================================

The interpreter now accepts supported solver controls inside ordinary If,
Else, For and While branches while retaining the nearest enclosing solver's
ownership. It checks every conditional arm. Previously it rejected seven
legal placements and incorrectly accepted two invalid Else-arm placements
in the retained 20-case native baseline.

The GUI also retains a selected command's pre-execution identity when reading
its summary. A solver can change its generating strings during execution;
searching the original script for those changed strings lost Vary commands.
Summary naming uses structural command links and restores the original names.
Existing edit/build/run validity guards remain in force.

Evidence and results
--------------------

Evidence directory:
``build/nasa-coverage/coverage95-20261108-solver-nested-control-validation``.

* ``native-final-v2``: 20/20 cases pass in isolated native processes. Nine
  DifferentialCorrector cases actually solve, four optimizer branch cases
  check Build only, and seven invalid scope/name/type cases must be rejected.
  The independent linear results are U=4, Quantity=9 and Keep=41; nested
  ownership cases additionally require V=2 and OtherQuantity=3. Existing
  solver registration, problem dimensions, math and control semantics stay
  unchanged.
* ``gui-final-headless-v2`` and ``gui-final-visible-v2``: seven cases pass
  per profile. Five are unchanged original solver-control operation tests;
  two are supplemental Vary summary tests. Each includes command editing,
  insertion/deletion, Undo/Redo, Unicode reopen and two mission executions.
  Raw report and summary values are checked against the original circular
  orbit and literal scalar expectations. The supplemental witnesses require
  all eight actual summary dialogs across both profiles to be delivered.
* The visible profile records 279 native window exposures, with zero retained
  screenshots. Input uses Qt events; native operating-system input is not
  thereby qualified.

The native final run predates the GUI-only summary repair. Its executed
source and image identities are retained separately. Its native owner sources
and loaded images did not change afterward; CMake gained the supplemental GUI
test registration. No unnecessary native replay was performed for that change.
The final GUI profiles share identical source and binary inventories.

Preserved failures and test limitations
--------------------------------------

The first repaired native run passed 19/20. Its new While fixture initialized
its counter outside Target, so later solver evaluations skipped the controls.
Only that new fixture's loop setup was corrected to initialize each evaluation;
its result assertions and numerical bounds were preserved. The original 1,334
protected inputs remain unchanged.

The first five original GUI cases returned success in both profiles, but an
artifact audit found eight empty Vary summary files. Their existing timer helper
did not require callback delivery. A supplemental witness reproduced the actual
product error, ``This command no longer exists. Reopen its panel.``, before the
summary identity repair. Those original receipts and the two failing supplemental
cases remain preserved. Only the final GUI runs support the repaired summary
claim; an unconditional operation marker is insufficient evidence.

Build setup failures, the failed native While fixture, original empty summary
files, source preimages and exact executed source versions are retained.
``component-audit-v2.json`` records the independent artifact checks;
``publication-v1.json`` binds the component and source archive.

Campaign scope
--------------

The mixed-time 233-case command overlay now contains 226 passing and seven
failing records. Only these five original solver-control records were updated.
The remaining five legacy Save filename expectations and two Prepare source
contract conflicts remain open. Other records keep their prior source/runtime
versions; this overlay does not qualify the whole current build.

No whole ``FRC!E7`` or ``FRC!E55`` all-command requirement is admitted from this
selected matrix. Optimizer convergence, arbitrary solver sequences and complete
scientific correctness remain outside this component's evidence. The canonical
campaign remains 1,076/1,158 (92.9%), with 82 open requirements and 25 more needed
for the unchanged 95% target. Full NASA, scientific, platform and current-build
qualification remain open.
