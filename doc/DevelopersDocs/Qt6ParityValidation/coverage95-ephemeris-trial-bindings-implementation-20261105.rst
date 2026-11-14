Ephemeris trial reference and step setters, 2026-11-05
=====================================================

This is an implementation-only continuation of FRR!E406. The row stays partial.
Tracking remains 1086/1158 (93.78%); recorded verification remains 1053/1158
(90.9%). All 105 original acceptance rows remain open. No verification ran.

Concrete bypasses and repairs
-----------------------------

ObjectInitializer::SetRefFromName calls virtual SetRefObject, including for
function resources. EphemerisFile previously copied spacecraft, output-frame
and acceleration-model bindings into its existing writer immediately. That
bypassed the live trial-copy protection: restoring the resource configuration
did not restore those separately mutated writer pointers. The writer could
retain a binding into a temporary function scope. Numeric StepSize similarly
updated the live writer even though trial configuration was later restored.

These setters now retain command-visible trial values while suppressing the
writer update during effective SOLVING, including an enclosing solver's trial.
Ordinary accepted/nontrial setters retain their native update path. Reference
setters check the actual C++ type before assigning a typed pointer, retain
bindings on a type mismatch, and handle null safely in debug diagnostics. The
reference-name query now recognizes ODE_MODEL as well as the existing legacy
PHYSICAL_MODEL spelling. Numeric steps must be finite and positive.

The existing entry configuration snapshot restores resource settings before
accepted/terminal handling. This repair keeps the accepted writer's pointers
and sampling step intact during trials; it does not copy a callee's complete
dependency graph into the caller. Public signatures and class layout do not
change in this batch.

Caller lifetime inventory and remaining implementation
------------------------------------------------------

FunctionManager::AssignResult writes through caller wrappers using
ElementWrapper::SetValue; an ObjectWrapper invokes virtual Copy. Nested return
also clones output values, restores the caller frame, then invokes Copy on
named destinations. The following GmatFunction::Initialize call need not
initialize all resource objects when its objectsInitialized flag is already
set. Its Execute refreshes several resource families, but does not supply a
general return-time subscriber dependency transfer. Therefore neither route
establishes that copied spacecraft/frame/model pointers belong to the caller.

Caller outputs referring to private function resources still need an explicit
dependency-lifetime implementation. Translation between formal and caller names,
multiple related outputs, true globals, repeated calls and recursive frames
must be handled together. Do not infer ownership from matching names or fix
this by silently deleting dependencies or accepting dangling pointers.

EphemManager's internal SPK/CK recorders use SetBackgroundGeneration during
creation, after Initialize/ToggleOn. That flag controls short-segment writer
behavior and is not a public script parameter. Its initialization path is
preserved here. No change to internal event-recording or native final-pass
policy is inferred from this setter repair. FRR!E406's broader policy review
and the other remaining implementation scopes are still required.

Compilation and retained receipts
----------------------------------

Stage: ``build/nasa-coverage/coverage95-20261105-implementation-ephemeris-trial-bindings``.
The private integrated build passes in 4.60 seconds (session 71838 completed).
Base, Qt, console, plugins and bindings remain buildable; test/probe clients
were linked only. Preimages, exact at-build patch/hashes, build log/receipt and
final patch/hash/summary artifacts are retained. Only compilation was executed.

No test, GUI, script/function/model/solver execution, numerical comparison,
acceptance reader/seal or qualification run. No original fixture/assertion/
tolerance/fingerprint/denominator/ledger/gate changed. No installed/shared
runtime, package, commit or push update. Windows/macOS behavior remains
unqualified. Later verification must exercise type/null failures, all three
reference families, numeric steps, final replay and nested/recursive scopes.
