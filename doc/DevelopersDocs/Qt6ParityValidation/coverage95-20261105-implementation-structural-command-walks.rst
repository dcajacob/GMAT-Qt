Structural command walks preserve execution control
====================================================

Implementation-first repair following exact open C-interface rows E249/E251/E252.
Their full external-client configuration/force/provider/ABI obligations remain
open. Source review found that model enumeration used virtual execution GetNext,
whose solver override changes Publisher run state, can return self during an
active branch and can throw for solver ExitMode=Stop. The existing structural
GetNextWhileExecuting API is designed to inspect those links without navigation.

Five production files change. CInterfacePluginFunctions uses structural links
for RuntimePropagators and its legacy indexed helper. GmatCommand and
BranchCommand terminal walks use structural links after RunComplete. Function
caller-binding preparation, GmatFunction initialization and function finalization
do likewise. The actual GmatFunction execution loop retains GetNext; solver
execution navigation remains intentional there. C-interface summary traversal
already used the structural API and needs no edit. No new API, layout, member,
slot, formula or model-ID convention change. Native lazy preparation can still
initialize publisher/model data; this is not a universal side-effect-free claim.

Stage: build/nasa-coverage/coverage95-20261105-implementation-structural-command-walks.
Five exact preimages and incremental-v1.patch retained. Patch SHA256:
706c64b709a866320796d3a6a05c0b37b472dfedfb960d542811bd65a006d2c6.
Reverse patch check passes. The private integrated-current build explicitly
includes GmatBase,GmatFunction,CInterface,GmatQt,GmatConsole and passes4.92s
without diagnostics. summary.json records final source/native image hashes.
Seven monitored installed images unchanged; public code/packages unchanged here.

The unchanged pure-C acceptance client and existing fixture builder pass basic
133 and stm481 assertions,614 total, against the private CInterface/Base. They
retain two independent models,6/42state dimensions,orbital/force/STM results,
selection and external state assignments. No GUI substitution for the C ABI.

Five unchanged GUI checks pass: Recursion,Stop,NestedDCRuntimeError,
NestedYukonStopCommand and SolverStateScoped. The solver-binding executable is
compiled unchanged against the private frontend/core. SolverStateScoped is the
existing separately registered cache/store-owner contract; the retained strict
external-execution-clone probe remains registered and unqualified. No original
strict assertion is changed or promoted. Error,interruption,private/Global status
and same-session recovery observations retain their original expectations.

Replay uses build-existing.py,run-existing-capi.py,run-existing-gui.py and saved
case commands with fresh isolated output/startup directories. C client image
records identify private CInterface/Base; GUI maps identify the private Base and
exclude installed images. Native xcb GUI runs in a resizable initially minimized
desktop with zero screenshots. C support runs without display variables. All
compiler/native/display handles terminal. No new tests,fixture definitions,
registrations,matrices,readers or seals.

Direct active-branch/stopped-solver lookup run-state instrumentation and complete
branch/setup/provider coverage remain deferred. Existing checks are regressions,
not whole-row qualification or proof that every structural walker is repaired.
No original row admitted. Verified1053/1158,105open,48neededfor1101;59holds,
31resolved/28unresolved including6MATLAB,37MATLAB identities,historical1816/2367
unchanged. Goal active/incomplete.
