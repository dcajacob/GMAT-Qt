Function output wrapper ownership, 2026-11-05
==============================================

UserDefinedFunction declares wrappersToDelete in addition to the same-named
ObjectManagedFunction member. GmatFunction added pending outputs to the derived
queue, while ClearOutputWrappers and GetWrappersToDelete use the base queue.
Normal map-backed cleanup could still release an output, but a failure between
queue insertion and outputArgMap insertion left that pending wrapper invisible
to its intended cleanup owner.

GmatFunction now explicitly queues outputs in ObjectManagedFunction's queue.
Result construction releases prior outputs through ClearOutputWrappers instead
of discarding a deletion list. GmatFunction destruction releases outputs before
its command sequence, UserDefinedFunction before template resources, and
ObjectManagedFunction destruction/assignment also release owned outputs. Input
wrappers remain borrowed from FunctionManager. The unused legacy derived vector
is retained and documented; class layout and virtual signatures are unchanged.

The in-tree source search finds the GetWrappersToDelete accessors and no
additional consuming code. FunctionManager's caller input/output wrappers retain
their separate ownership. No runtime/memory/exception-injection behavior is
claimed from this source review.

The private integrated build passes 77 steps / 7.87 seconds, session 16937
completed. Final source hashes match at-build-v1. The stage retains four exact
preimages, before/at-build/final hashes, patches, source notes, log/receipt and
summary-v1:
build/nasa-coverage/coverage95-20261105-implementation-function-output-queue.

The source notes also record the definition-assignment follow-up. Ordinary whole
function mission assignment is blocked by the native IsCommandModeAssignable
check; argument/result/C++ copies are separate paths. GmatFunction assignment
retains compiled state while copying metadata, and UserDefinedFunction assignment
detaches its sequence without a full lifecycle transition. Missing FCS rebuilds
currently use private Sandbox setup code with nested-function/event binding.
Clearing fcs alone, or casually parsing a new definition at runtime without
context/cache handling, is not a complete replacement implementation. Active
definition graphs and general returned private dependencies remain open.

Tracking-v12 updates implementation notes without adding rows: 1092/1158
(94.30%), 66 other original rows. Recorded verified acceptance stays 1053/1158
(90.9%); all 105 acceptance rows remain open. No tests, GUI, mission/function/
model/solver execution, numerical comparisons, memory checks, acceptance readers
or seals were run. No original requirement/text/fingerprint, assertion, fixture,
tolerance, denominator, ledger or gate changed. Verification remains deferred
until the remaining implementation pass is complete.
