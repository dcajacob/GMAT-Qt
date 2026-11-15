Function input scene capture — implementation, 2026-11-06
=========================================================

Function inputs now capture their values before any formal input is changed.
Previously recursive argument permutations could read an already-replaced
formal, or a spacecraft could still reference a force deleted by an earlier
model Copy. Covered resources now capture dependencies from the caller scene,
including native model/component owners outside the callee store. This is
source implementation progress; the full function I/O/scoping rows remain open.

Input capture and binding
-------------------------

CaptureInputObjects evaluates/snapshots arguments before CreatePassingArgWrappers
or RefreshFormalInputObjects copies any formal. System Parameter inputs use
Real/Array/String value proxies through the existing typed evaluator, rather
than keeping a live caller-owner handle as the formal value. Each argument has
its own snapshot. A shared invocation marker identifies exact source roots.

Covered graphs use caller-local resources and existing global ownership policy.
After all formal root copies, retained references reconnect to final formal
roots and stable native components. Repeated actual resources produce distinct
formal values; references from another input retain their private snapshot when
there is no unique formal alias. No last-mapping guess is made. True-global
references retain the existing shared-identity policy.

Input type changes rebuild matching objects/wrappers using the already captured
values, without evaluating expressions again. PrepareObjectMap refreshes the
borrowed caller lookup each entry. Explicit evaluated arguments transfer
ownership by position, preserving repeated-expression argument slots. The
optional FunctionRunner math-node path stages those values before common
Execute entry, instead of initializing formal objects before caller saving.
Pending values retire on relevant setup changes, assignment, cleanup and
destruction. Rebuilding wrappers no longer leaks unused builtin output objects.

Saved caller frames
--------------------

PushToStack retains covered native dependency graphs while every original
caller address is live. Globals, functions and subscribers keep their existing
skip policy. Pop restores root values, replacing incompatible native types when
needed, and reconnects saved graph references after non-ephemeris output Copies.
This order avoids reconnecting a saved spacecraft to model components that an
immediately following output Copy would delete. Existing ephemeris-last output
policy and live transient-force context remain.

Remaining implementation
-------------------------

Graph support has not expanded to every root family. PropSetup, Formation,
GroundStation transitive dependencies, burns, events, other subscribers/providers,
true globals with private dependencies, private bodies/shared kernels and
unsupported primitive/selector outputs still need work. Native Parameter owners,
caches and opaque borrowers across type replacement, child-created local
cleanup and exceptional/universal frame rollback are not universally repaired.
Later command/parent replacement, direct native copy paths, cross-Sandbox
lifetimes and moving-frame/PSM scientific domains also remain.

Entire-object E452/E460 and input-preservation E466 remain unchanged. These
covered paths add no broad parent engineering or acceptance credit. Continue
all other original command/body/frame/OEM/provider/specification scopes before
verification; none is waived by this increment.

Compilation and retained work
-----------------------------

The private integrated build passes 89 steps / 141.29 s (v1) and 66 steps /
5.58 s (v2), without warnings. Core, Qt, console, plugins and Python bindings
compile/link. Final sources match v2; sessions 21800 and 58635 are closed.
Three exact preimages, before/at-build/final hashes, focused patch, source notes,
logs/receipts and summary are retained under
``build/nasa-coverage/coverage95-20261106-implementation-function-input-scene-capture``.
FunctionManager internal storage changes require a consistent rebuild; external
binary and cross-platform behavior remain unqualified.

No tests, GUI, mission/function/model/solver execution, numerical comparisons,
acceptance readers or seals ran. Original assertions, fixtures, tolerances,
requirement texts/fingerprints, denominator, ledger and gates are unchanged.
Tracking stays 1093/1158 (94.39%), with 65 other original rows; the recorded
source-versioned verified baseline stays 1053/1158 (90.9%). All 105 original
acceptance rows remain open. No installed/shared runtime, package, commit or
push changed. The goal remains active and the full implementation is incomplete.
