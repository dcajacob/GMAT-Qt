Function run controls during solver execution
==============================================

Original FRR!E485 requires GUI Pause/Stop actions to be processed while a function
executes. FRR!E477 and FRC!E7/E55 require supported commands inside functions and
solver sequences. The concrete gap was in GmatFunction::Execute: it enabled its
existing interrupt poll only when Publisher reported RUNNING. Optimize publishes
SOLVING for trials and SOLVEDPASS for accepted passes, so a function entered from
those contexts disabled its poll. A loop of assignments could therefore execute
without this function dispatcher servicing GUI controls.

The gate now includes RUNNING, TARGETING, OPTIMIZING, ESTIMATING, SOLVING and
SOLVEDPASS. The existing 50-command cadence, Moderator interrupt handler,
pause loop and stop exception are unchanged. Idle initialization-time calls
remain outside this polling service. This changes no solver calculation,
function argument policy, scope, owned resource graph or command grammar.

Function clause reconciliation
--------------------------------

The source review also found existing routes for the original I/O/scoping work.
These are source observations, not whole-row acceptance claims:

* FRR!E451-E458: FunctionManager::CreateObject materializes numeric/string
  properties, variables, arrays and literals. CreatePassingArgWrappers clones
  object inputs, renames the formal and marks it local rather than global.
* FRR!E459-E463: FunctionManager stages scalar, string, array and supported
  whole-resource outputs before caller assignment. The retained resource
  provider paths already exist; they must be verified across the required
  resource inventory rather than replaced with an unlimited graph redesign.
* FRR!E464-E469: ValidateFunctionArguments allows matching zero inputs and zero
  requested outputs. Inputs populate the function store; GmatFunction::Execute
  looks up declared outputs and errors when an output is absent. Global objects
  use the existing global store. Full isolation/alias behavior remains unverified.
* FRR!E477/E491: GmatFunction installs local/global stores, SolarSystem and
  coordinate/force context on native commands. Local Create, the Global command
  and automatic global resource handling already have owning source paths.
  This establishes where to check the clauses, not that every command and
  resource combination is correct.
* FRR!E479: GmatInterface guards callbacks and script-control operations during
  GMAT-called MATLAB activity and rejects Function callback registration. The
  MATLAB modes have actual engine opening calls, with SingleUse explicitly
  restricted to Windows. Cross-process engine sharing and complete MATLAB I/O
  remain subjects for later verification, not missing-mode assumptions.

Compilation and limits
------------------------

The integrated build returned 0 in 1.84 seconds, completing 4/4 steps without
warnings. Its observed compiler session is terminal, and the edited source hash
matches that build. The source preimage, patch, six reference hashes and compiler
receipt are in
build/nasa-coverage/coverage95-20261107-implementation-function-run-controls.

No tests or runtime verification ran. No requirement credit, original disposition,
fixture, assertion, tolerance, denominator or acceptance gate changed. The
verified baseline remains 1053/1158 (90.9%); 1093/1158 (94.39%) remains a carried
engineering estimate. Full command/resource coverage and the implementation
audit remain incomplete.

The next bounded source review is the command inventory for FRC!E7/E55, including
the function contexts above. Identify a specific unsupported command or clause
before making another implementation change.
