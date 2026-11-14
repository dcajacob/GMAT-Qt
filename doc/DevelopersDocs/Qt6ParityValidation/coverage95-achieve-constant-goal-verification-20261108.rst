Finite numeric measured Achieve goals
====================================

The implemented AllowConstantGoal option passes native and actual GUI checks.
The default remains false; explicit true allows a finite numeric literal as the
measured Goal, on the left of the equals sign. This verifies the implemented
worksheet compatibility behavior without substituting desired-value evidence.

The checks also found and repaired a separate accessor gap. Achieve advertises
AchievedValue as a read-only REAL_TYPE field and GetNumber returns its stored
value, but GetRealParameter previously threw. The repair adds the missing
achievedValueID dispatch. It returns the same stored value, with no change to
formulas, solver behavior, finite checks, public signatures or object layout.

Evidence
--------

Stage: ``build/nasa-coverage/coverage95-20261108-achieve-constant-goal-validation``.
Registered native entry: ``QtAcceptance.AchieveConstantGoal``.

Eight native cases pass:

* Default and explicit-false reference goals solve U=4, Quantity=9, with Keep=41
  and successor After=7. Omitted false stays omitted from serialization;
  explicit false remains explicit. Clone and assignment preserve the policy.
* Default and explicit-false numeric measured goals must be rejected.
* Opted-in fractional 1.25 and signed scientific -2.5e-1 measured goals, with
  equal desired values, converge while U=1 and Quantity=3 remain unchanged.
* An opted-in measured 1.25 versus desired 2.25 cannot converge. The actual
  mission returns -5, solver status is FAILED, and After remains zero. The
  measured value stays 1.25 while the solver perturbs U to 1.01.
* Overflow literal 1e309 must be rejected as nonfinite.

All five executed goal cases check both real accessor paths, read-only/type
metadata, policy copy and serialization. Twenty-five scalar results are
independently checked. Two pre-repair runs are retained: each has three passing
rejections and five accessor failures. The diagnostic run proves the solver
and literal-policy behavior already worked through GetNumber; only the missing
GetRealParameter dispatch was repaired. Mission inputs and numerical bounds
are byte-identical across those runs after normalizing private output paths.

QtGui.AchieveConstantGoal and the unchanged QtGui.NasaAchieveParameter pass
unattended and visibly, for four registered GUI executions. The new workflow
runs six missions per profile: default reference, two literal spellings before
and after Unicode reopen, and explicit-false reference compatibility. It checks
the actual option checkbox and measured-Goal selector, literal/nonfinite picker
rules, invalid Apply/OK recovery, draft Cancel, exact source and Undo/Redo.
Across both profiles it records 16 actual selector checks, 16 invalid Apply/OK
rejections and 60 independent report/measurement scalars. The original tests
add 16 report scalar checks, with their requested 1e-9 goal residual bound intact.
There are 58 native visible exposures and zero screenshots.

Identity and scope
------------------

``component-audit-v1.json`` independently validates the artifacts and literal
linear results. The before/after guards retain 1,158 source and 41 binary
identities. Both GUI profiles share 1,203 source and 115 binary identities.
The source archive retains 2,338 exact non-ELF versions; 139 native loaded ELF
images are identified without copying them. All 1,334 protected original test
inputs and assertions remain unchanged. One existing renamed-main return
warning in the protected included lifecycle helper is retained.

Exact row candidate FRC!E97 lists Real number under Achieve control variables.
The complete original context and whole-row admission must be evaluated
separately. The broader FRC!E93 type parent, literal Vary controls, all solver
science, functions, frames, platforms and native OS input are not qualified by
these cases. At component publication, canonical acceptance remains
1,077/1,158 (93.0%), with 24 more needed for the unchanged 95% target. The
component publication itself adds zero ledger credit.
