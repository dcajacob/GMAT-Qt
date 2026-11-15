Command and ephemeris boundary verification, 2026-11-07
=====================================================

The queued 233-case original plan finished with 203 passes, 13 failures and
17 cases not executed after three repeated common failures in their binary.
All 23 original test targets built successfully. This was a finite verification
batch on the runtime containing the completed local-spacecraft readiness repair;
no production source or protected test input changed during this batch.

The plan supplies evidence for All Others!E160/E196, FRR!E404 and FRC!E93.
It does not establish complete qualification of any of those original rows.
All 105 original open acceptance rows and gates remain open. The recorded
verified baseline and engineering estimate are unchanged. The saved Goal is
active; continue without asking the user to restart each batch.

Passing evidence
----------------

All six original OEM record-support cases pass, including ShortArc with its
previously authorized expected-degree correction. All nine original ephemeris
sampling cases pass, covering fixed aligned/remainder sampling, raw steps,
user interval, assignment/maneuver/dynamics boundaries, toggle gaps and short
arcs. Both SPK writer storage/recovery cases pass. These do not by themselves
qualify every requested interpolation order or strict/compatibility behavior.

All five original many-solver cases pass. The broader passing set includes
command summaries, standard command editors, tree/control operations,
navigation command operations, backend operations and GMAT/Python/builtin
function command operations. These supply selected command/owner routes;
they do not prove every Achieve parameter family or dependent requirement.

Retained failures and next repair
---------------------------------

Thirteen original failures remain:

* Save: extended summary Build, editor literal insertion, three tree insertion
  cases and root/branch output-operation Build.
* PrepareMissionSequence: original and protected operation cases.
* BeginMissionSequence: structural summary lacks spacecraft/frame.
* Achieve, Vary Target and Vary Optimize nested within If: interpreter rejects
  solver commands inside control logic.

The 17 unexecuted cases remain pending. Nine share the tree binary that reached
three matching Save failures; eight share the control-operation binary that
reached three matching nested-operation failures. Their names and failure
signatures are retained in the manifest; they are not passes.

The next bounded repair is Save command-label parsing. The original extended
summary log reports that ``'Probe'`` is an invalid object name in
``Save 'Probe' Sat``. Source review shows that ParseAndSetCommandName treats
the newly added optional Filename parameter as a leading filename operand,
requiring four quotes before it will recognize a command label. Save filenames
now live inside an optional output-options block, so the leading quoted token
is still its command label. Restore that existing label behavior with and
without options, keeping other command grammars unchanged. Preserve all original
tests; distinguish this parsing defect from old expectations of .data filenames
or source spelling. The user prefers worksheet defaults and asked to support
both filename styles. Do not revert that decision to satisfy old fixtures.

The other failures need separate clause-specific diagnosis. Do not remove solver
validation or redesign preparation/mission ownership simply to obtain passes.

Evidence and boundaries
-----------------------

The stage is
``build/nasa-coverage/coverage95-20261107-command-boundary-verification``.
Read ``validation-summary-v1.json``, ``retained-failures-v1.json``,
``native-v1-results.json`` and ``publication-v1.json``. The runner retains
original registrations/watchdogs, effective commands, per-case receipts and
logs, and 1,585 bound source/runtime/fixture hashes. All bound hashes still match.
All 1,334 original test inputs and the original requirements/worklist/tracker
are unchanged. No new assertions or tolerances were introduced.

Execution used private Xvfb, with original native wrappers where registered.
It does not establish user-visible native-input, cross-platform or scientific
acceptance. All build/test handles are terminal. Keep the remaining function
and independent BrouwerMeanLong failures, definitions, external dependencies
and historical-platform holds open. The campaign remains incomplete.
