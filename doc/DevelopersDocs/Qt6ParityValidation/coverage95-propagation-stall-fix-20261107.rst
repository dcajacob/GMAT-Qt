Propagation stall and stale plot bindings: repaired
===================================================

Both unchanged named-call integration cases now pass within their original
30-second mission watchdogs, with every original numerical/publication assertion:

=============================================== ============ ===========
Case                                            Unattended   Native X11
=============================================== ============ ===========
NasaAssignmentNamedCallGmatFunction              2.12 s       2.73 s
NasaAssignmentNamedCallBuiltinGmatFunction       0.67 s       1.27 s
=============================================== ============ ===========

These are complete test-process times including GUI setup. Native runs used
private Xvfb displays, not user-visible acceptance. The uninstrumented final
build passes 27 executions covering 25 distinct registered cases: the two cases
above in both modes and 23 additional regressions. The additional cases are eight
propagation-stop cases, all ten function I/O cases, function/subscriber continuity,
native subscriber/global-loop Toggle and Global, subscriber native copy/path
behavior, and the OEM ShortArc ephemeris-output case.

Demonstrated causes and changes
--------------------------------

The elapsed-time stop wrapper read zero even while the propagated spacecraft and
the directly bound stopping Parameter advanced through 10, 20, 30, 40, 50 and
60 seconds. StopCondition::SetStopParameter replaced the Parameter pointer during
sandbox initialization without updating lhsWrapper. EvaluateStopValue reads that
wrapper, so the ten-second stop was never reached. The setter now rebinds the
wrapper to the selected Parameter, matching the existing RHS binding policy.
Array-element selection and Variable wrapper semantics remain in their wrappers.

After that repair, missions completed in about 0.03 seconds and their report and
spacecraft values satisfied the independent orbit/function checks. The original
plot assertion then exposed a second stale-binding path: ordinary subscriber
clones were being converted into retained function values, capturing configured
Parameters rather than binding to the mission sandbox. The plot wrappers read
zero while the final report correctly showed about 20 seconds. Diagnostics
confirmed that Trace, Control, Values and Extra had no retained context at entry.

Sandbox::AddObject now uses the existing FunctionValueCopyGuard while cloning
ordinary subscribers with no retained context. Normal initialization then binds
their references to that sandbox. Sources already carrying a retained function
value context keep their previous clone path. Standalone subscriber Clone and
function-value graph construction are unchanged. The retained/global regression
checks passed; this is not proof of every possible retained resource graph.

Only two production files retain changes: src/base/stopcond/StopCondition.cpp and
src/base/executive/Sandbox.cpp. No class layout, parameter ID or virtual slot was
added. Integrators, physical models, numerical thresholds and test watchdogs were
not changed. The earlier function-path, bracket and cost-label repairs remain.

Validation and retained evidence
----------------------------------

Records are under build/nasa-coverage/coverage95-20261107-propagation-stall:

* validation-summary-v1.json lists final outcomes and scope.
* original-replay-unattended-v3.json and original-replay-native-v3.json retain
  the four final executions of the original integration cases.
* regression-results-unattended-v1.json, regression-results-native-v1.json and
  ephemeris-results-unattended-v1.json retain the 23 additional passes.
* The bounded debugger stack, elapsed-time/owner samples, intermediate plot
  assertion failures and diagnostic source versions remain available.
* Preimages, final source/runtime hashes, exact patch and commands allow the
  executed versions to be inspected. No source changed during a final test run.

Temporary instrumentation in Propagate.cpp and XyPlot.cpp was removed; both files
are byte-identical to their preimages. The last sandbox diagnostic print was also
removed before final validation. The final repair build completed 63/63 steps;
the ephemeris test target subsequently built successfully. All build, debugger
and test handles for this work are terminal.

All 1,334 pre-existing test inputs remain byte-identical, including assertions,
fixtures and tolerances. Original requirements, fingerprints, worklist, tracker,
denominator and qualification gates are unchanged. Earlier failed executions
remain failed historical records; no receipt was rewritten as a pass.

This completes the bounded task of repairing the two named-call integration
failures. It assigns no new NASA requirement credit or percentage and does not
complete the broader verification campaign. Undefined/external requirements and
scientific/current-build/full-NASA gates remain open. The verified 1053/1158
baseline and 1093/1158 engineering estimate are carried values, not recalculated
results from these tests. No commits, pushes, packages or installed-runtime
changes were made.
