Plot-step verification and function cleanup repair, 2026-11-08
=============================================================

The original ground-track integration-skip boundary case passes. Accepted-step
publication passes all eight existing scenarios, and transactional capture passes
16 integrator/direction cases with 960 independent state comparisons. The plot
consumer matrix now has passing case results for all 125 existing scenarios on
the repaired runtime, across two runs. Its unfiltered registered test still
fails an assumption about retaining all Message Window history; that failure is
preserved. No original assertion, fixture or tolerance changed.

The batch exposed and repaired a real function cleanup crash while rebuilding
the next mission. All 105 original open acceptance rows and qualification gates
remain open. No new acceptance credit or percentage change is claimed. The
campaign Goal remains active and incomplete.

Demonstrated crash and repair
----------------------------

The first full consumer run completed 93 scenarios, then crashed during the
build following FunctionGlobalTwice. A two-case FunctionGlobalTwice/FunctionNested
reproducer captured the crash in UserDefinedFunction::ClearFunctionObjects,
called by Sandbox::Clear. Flushed diagnostics identified a cached declaration
entry for One.A1ModJulian, outside the function's owned definition pool.

Sandbox clears its owning object maps before the later function destructor
inspects every declaration-map entry. A borrowed system Parameter could therefore
already be deleted when ClearFunctionObjects tried to classify it. The repair
retires aliases to actual sandbox-owned objects while those objects are still
alive, after function finalization and before either owning map starts deleting
values. It removes matching non-owned pointers from declaration, automatic,
validator and cleanup bookkeeping. It neither deletes the resources nor changes
function language, value-copy or plot-cadence semantics.

The helper is nonvirtual and adds no class fields or virtual slots. Production
changes are confined to UserDefinedFunction.cpp/.hpp and Sandbox.cpp. The exact
focused sequence passes after the repair, as do all four function consumer
scenarios and six unchanged function regressions: object I/O, nested I/O, missing
output recovery, hardware DutyCycle binding, named-call propagation and
Transmitter return/edit behavior. Earlier production repairs remain intact.
All temporary diagnostics were removed; their failed runs and patch are retained.

Producer and consumer evidence
-----------------------------

The eight publication scenarios cover forward/backward split commands, single
steps, synchronized propagation in both directions, formation propagation and
two nonlinear ABM stop epochs. They compare accepted endpoints with independent
circle states and preserve the legacy publication bytes. The capture transaction
matrix exercises eight numerical methods in both directions, including nested
commit/discard, exception and thread isolation. The original ground-track case
checks cadence across a Propagate boundary using the actual resource editor and
an independent circular-state observer. These all pass on the final runtime.

The original consumer matrix covers both OrbitView and GroundTrack, skip/count
boundaries, multiple providers, formations, solver modes and rollback, live
membership, retention/redraw, fixed chemical/electric burns, ephemeris providers,
toggle/reset lifetimes, functions, translated origins, rotating frames, gravity
contexts and LocalAlignedConstrained frames. Its existing numerical, state,
membership, warning, image and source-isolation assertions are preserved.
Individual case limits remain those stated by the original tests; these results
do not establish arbitrary-frame or general-force scientific qualification.

The repaired unfiltered run completed 95 cases, including all four function
scenarios. It then failed the FrameFollowerForward message-prefix assertion.
MainWindow limits the Message Window to 10,000 blocks; the saved history has
9,999 split lines. This evidence and a successful fresh-process replay of all
30 remaining frame scenarios identify a conflict between the test's unbounded
history assumption and the application's bounded display retention. Source,
runtime and original input hashes match across those runs. The 125 passing case
proofs combine 95 from the unfiltered run and 30 from the fresh filtered run.

The unfiltered PlotIntegrationSteps CTest is still recorded failed. Its assertion
was not bypassed or changed, and the application history limit was not changed.
The fresh run uses the scenario filter already provided by the original test.
Ten other original registered tests pass on the final runtime. The earlier
segmentation fault and the first launch's source-path setup error are retained
separately from those results; debugger return code zero is not treated as a
passing child test when SIGSEGV was captured.

Evidence and next work
----------------------

Stage: ``build/nasa-coverage/coverage95-20261108-plot-step-sampling``.
Read ``validation-summary-v1.json``, ``consumer-case-results-v1.json``,
``retained-failures-v1.json``, ``sandbox-function-cleanup-repair-v1.patch`` and
``publication-v1.json``. Preimages, diagnostic traces, registrations, effective
commands, explicit scenario filters, receipts, logs, case proofs and source/
runtime hashes accompany the runs. All current bound hashes match and all
build/test handles are terminal. The plot producer/consumer source files did
not change in this batch.

All 1,334 original protected inputs and the original requirements, worklist and
tracker remain byte-identical. Private Xvfb supplies no user-visible native-input,
cross-platform or whole-row acceptance. Preserve the independent BrouwerMeanLong
failure, remaining function/command contracts and specification/external/
historical holds. The carried verification and engineering tallies are unchanged.

Next verify strict versus compatibility interpolation sufficiency for the
requested Lagrange orders and odd Hermite degrees, using independent polynomial
and mission-close checks. The exact remaining routes and starting sources are
in next-interpolation-plan-v1.json. Existing OEM/sampling/SPK support passes do
not alone prove every requested type/order, discontinuity, EOF or scratch path.
