C API Propagate configuration and scalar stops, 2026-11-05
==========================================================

All Others!E252 requires configuration of one or more Propagate commands by
loading a script, referencing FRC-10. The interface uses the native parser and
command class. The source review also found and repaired missing measured-side
Variable/Array-element stopping routes in that class. This adds one row to the
provisional implementation tally, now 1088/1158 (93.96%); it adds no acceptance
credit. Recorded verification remains 1053/1158 (90.9%), all 105 acceptance rows
remain open, and 70 original rows remain partial/unclassified/dependent.

Interface and referenced command scope
---------------------------------------

LoadScript delegates to Moderator::InterpretScript, which invokes the ordinary
ScriptInterpreter. CommandFactory creates the same Propagate class used by the
GUI/native engine; the C wrapper has no separate command or field whitelist.
RunScript delegates to RunMission. BeginMissionSequence executes the initialized
sequence; PrepareMissionSequence retains preparation-only behavior. The latter
does not discard Propagate commands from the configured mission.

The FRC-10 source inventory is:

* E111: native direction/options parsing and propagation direction handling.
* E112: spacecraft groups configured by ConfigurePropSetup/AssemblePropagators
  and mapped through PropSetup/PropagationStateManager.
* E113--E114: vector-backed numerical groups and the existing Native/SingleSetup/
  SingleGroup ephemeris compatibility policies. The earlier policy batch retains
  the exact worksheet/native distinction; no fixed numerical group limit is added.
* E115--E117: native synchronized/independent group modes and their step paths.
* E118--E121: real-valued stop/goal wrappers, including Variables, Array elements
  and scalar Parameters. This batch completes the previously rejected user-scalar
  measured operands described below. Native spacecraft Parameter preconditions
  and parameter domains still apply; arbitrary expressions are not inferred.
* E122--E123: configured stopping lists, CheckStopConditions/TakeFinalStep and
  native StopTolerance configuration/refinement. No tolerances are changed.
* E124: first-step repeated-stop protection, now evaluating the typed scalar
  operand through the same path as ordinary sampling and refinement.

This is implementation presence for the configuration interface and command
routes. It is not independent scientific/runtime proof of every referenced
parameter, force model, epoch, group count or stop. Those original qualification
obligations remain intact for the later verification phase.

Measured Variable and Array-element stops
------------------------------------------

Propagate's wrapper setter already accepted real-valued operands, but
StopCondition::Validate rejected every non-system Parameter on the measured
side. SetSpacecrafts also required a spacecraft owner parsed from that operand.
Ordinary evaluation, base-epoch setup, stop differences, buffer refresh and
first-step checks read mStopParam directly. An Array-element wrapper instead
needs to evaluate its indexed scalar, not its owning Array object. AddToBuffer
had an inconsistent exception fallback to the wrapper.

StopCondition now explicitly admits Variable and indexed Array-element operands.
Whole Arrays and non-real operands remain rejected. EvaluateStopValue supplies
one common scalar evaluation path for sampling, buffers, base values, differences,
first-step protection and refinement diagnostics. It checks finite measured
values and propagates evaluation errors directly rather than retrying them
through an exception fallback. C++ callers without wrappers retain the native
scalar Parameter path. Reinitialization resets left-side cyclic metadata before
applying the current Parameter's time/angle traits, so a rebound non-angle
operand cannot inherit an old angle's wrapping.

Propagate binds Parameter metadata with a checked cast. Stop-condition reference
owners are parsed from StopVar rather than the condition's display name. Ownerless
user operands no longer generate bogus spacecraft references. Each condition
gets one epoch clock: its native spacecraft for spacecraft Parameters, or the
first propagated spacecraft for user scalars (including a Formation's first
member). Goal-side reference owners remain configuration dependencies and no
longer shift a later condition's epoch-clock index.

The selected Variable/Array value is sampled as it exists. Propagation does not
invent updates, dependencies, units or a root for a constant value. A value that
never reaches its goal cannot cause a stop. The Qt selector/help and Propagate
reference now describe these measured operands and their clock convention.
Native valid spacecraft stops, time/angle handling, interpolation formulas,
first-step tolerances and publication/refinement policies are retained.

Dynamics/parent dependency and model-ID contract
------------------------------------------------

All Others!E249/E251 remain dependent on the complete FRR-13 dynamics child scope,
including the unresolved Mars-GRAM/provider work. The C loader's presence is
not a substitute for the unavailable provider implementation. Their worklist
disposition is now depends_on_open_children; neither row receives credit.

FindOdeModel's empty-name return of zero is explicitly a native success status,
not a canonical model ID. Named queries return model IDs. The internal helper
and shipped MATLAB clients reflect that contract. No model-ID/selection change
is made by this batch, and zero is not newly made an alias for SetModel.

Build and retained artifacts
------------------------------

Stage: ``build/nasa-coverage/coverage95-20261105-implementation-capi-propagate-scope``.
Exact preimages, at-build/final patches and hashes, build logs/receipts and final
summary are retained. Build v1 passes 146 steps/220.89 seconds (session 78635 completed); v2 passes
66 steps/4.14 seconds (session 26199 completed). No live sessions remain. The
final executable changes match v2; an exact comment-only setter-description
correction after that build is retained separately, with no new compile needed. No C ABI entry point, existing signature,
instance layout or script grammar changes. Two nonvirtual StopCondition methods
are added; all dependents are rebuilt together.

Only compilation and essential source debugging were performed. No tests, GUI,
script/function/model/solver execution, numerical comparison, acceptance reader/
seal or qualification run. Test/probe clients were linked only. No original
fixture/assertion/tolerance/text/fingerprint/denominator/ledger/gate changed. No
installed/shared runtime, package, commit or push update. Cross-platform execution
and numerical behavior remain unqualified.

Later verification must exercise both sides of scalar stopping conditions,
single/multiple conditions with different goal owners, Variables/Array indices,
constant/nonfinite values, formation/group clocks, independent/backward modes,
cyclic-to-noncyclic rebinding, initial/repeated stops, root/refinement behavior,
C API load/prepare/run and existing model-selection semantics. Finish the rest
of the implementation before those checks.
