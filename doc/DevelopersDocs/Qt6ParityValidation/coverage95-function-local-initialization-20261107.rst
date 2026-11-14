Function-local spacecraft initialization repair, 2026-11-07
=========================================================

The bounded repair for the demonstrated local-hardware association failure is
complete. Seventy-four distinct original cases pass on the current runtime:
52 thruster/finite-burn/torque cases, 13 clone/function/caller-isolation
regressions and nine explicitly configured system-mass-property cases. No
original assertions, fixtures, tolerances or acceptance requirements changed.
All 105 original open acceptance rows and qualification gates remain open.

The saved campaign Goal was observed active after the user resumed it.
Continue the finite campaign without asking the user to restart each batch.
The later human authorization to verify implemented features controls the phase;
the historical implementation-first Goal text does not revoke that authorization.

Failure and repair
------------------

The original DutyCycle case failed when a repeated or nested function invocation
refreshed ``PrivateOnly`` from its fresh spacecraft prototype. The captured
diagnostic showed source readiness false, destination readiness true and ordinary
copy context. The destination stayed ready after Copy. Its declared LocalEngine
and LocalEngineFuel names survived, but incremental tank binding rejected the
still-unbound thruster because the spacecraft falsely claimed initialization.
ObjectInitializer suppressed that initialization exception inside the function;
the later parameter assignment reported LocalEngine absent from PrivateOnly.

``Spacecraft::operator=`` now clears readiness for an ordinary assignment from an
uninitialized source. The existing NativeRestoration branch still preserves the
destination readiness state. This changes no object layout, reference ownership,
function language or parameter contract. The nine earlier production repairs
remain byte-identical to their preceding publication.

Temporary diagnostics in ObjectInitializer and GmatFunction were removed exactly
to their captured preimages. The production patch contains only the readiness
invalidation in Spacecraft. The pre-fix diagnostic failures remain available.

Verification and limits
-----------------------

The 52 affected original cases all pass under private Xvfb. This changes 15
previously failed test outcomes to pass and executes 37 previously deferred
cases successfully. The cases include repeated calls, nested calls, live
private/global owner and hardware checks, caller/prototype isolation, reopened
scripts and a repeat run without Build. Their original assertions remain intact.

Thirteen original regression cases pass: spacecraft Mass/Tank/Thruster cloning,
function object/properties/nested I/O, Transmitter/Transponder/Receiver returns,
both named-call propagation integrations, FuelMass and FuelCenterOfMassX binding.
The three clone cases initially could not launch because their test executable
had not been built. Those launch errors are retained; all three passed after
building the original target. There were 75 passing executions of 74 unique
cases, with DutyCycle repeated deliberately, and three retained setup errors.

All nine original Initialized system-CM/MOI cases also pass. They explicitly
enable the analytic extended mass model. The separate default-model fixtures
expect nonzero modeled CM/MOI while leaving the model set to None; their original
failures remain failures. Shipped Resource_SpacecraftBallisticMass help describes
None as computing TotalMass only. No production physics or original fixture was
changed to reconcile that contract conflict.

The combined latest function-family status, using the previous completed run
plus this bounded overlay, is 588 passed, 20 failed, 167 not run and 79 earlier
failures held out, totaling the original 854-case plan. These are mixed-time
results, not a fresh full-family replay on this runtime. Forty-six formerly
unexecuted cases now have passing results. No whole-row NASA credit follows.

All 1,334 original test inputs, the original requirements, worklist and tracker
are unchanged. Runtime/source hashes match the completed runs. Private X11
execution does not establish user-visible native-input or platform acceptance.
The carried verified baseline stays 1053/1158 (90.9%); the carried engineering
estimate stays 1093/1158 (94.39%) and is not a fresh implementation audit.

Evidence and next work
----------------------

The stage is
``build/nasa-coverage/coverage95-20261107-function-local-initialization``.
Its authoritative files are ``validation-summary-v1.json``,
``function-family-overlay-v3.json``, ``retained-failures-v3.json``,
``readiness-repair-v1.patch`` and ``publication-v1.json``. Original registrations,
effective commands, receipts, before/after hashes and per-case outputs accompany
the results. Diagnostic v1/v2 and initial clone launch errors remain historical
evidence. All stage build/test handles are terminal at publication.

Next execute the already queued finite command/Mission Tree, Achieve and
ephemeris-boundary plan:
``../coverage95-20261107-implemented-feature-verification/command-and-boundaries-plan-v1.json``.
It contains 233 existing cases across 23 targets for All Others!E160/E196,
FRR!E404 and FRC!E93. Preserve original tests and stop repeated common failures
within a binary for diagnosis. Do not re-run the entire function family.

Keep the remaining function failures, 64-degree BrouwerMeanLong independent
reconstruction failure, undefined requirements, external components and
historical-platform claims open. This bounded repair does not complete the
campaign or authorize a new release.
