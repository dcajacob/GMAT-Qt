Function output wrapper completion
===================================

Implementation-first repair in five production files: ObjectManagedFunction.hpp/
.cpp, FunctionManager.cpp, FunctionRunner.cpp and GmatFunction.cpp. Previously,
named-result assignment deleted source wrappers without clearing function lookup
or queue entries. Expression callers deleted copied queues, leaving stale entries
in the function. Built-in expression outputs were absent from those queues and
were not released there. Unselected outputs and partial result failures could
also leave wrappers behind. ObjectManagedFunction assignment's local deletion
list is separate from its member queue; it was not itself a double deletion.

New nonvirtual ClearOutputWrappers() releases map and queued output wrappers once,
clears aliases before deletion, preserves formal names, and excludes input-wrapper
aliases still owned by the calling manager. It uses no temporary deletion
collection. ClearInOutArgMaps uses this drain when output deletion is requested;
its input collection has an explicit name instead of shadowing the member queue.

FunctionManager drains outputs after copying complete named/expression results,
including unwinding from partial assignment. The invocation failure guard also
drains outputs after function finalization and before publication/state cleanup.
FunctionRunner no longer deletes copied queues. GmatFunction's pending wrapper
has scoped ownership through creation/binding/queue insertion; after registration
the queue retains it even if later map insertion throws.

Caller ownership is preserved for direct/native clients: no new destructor or
assignment deletion policy, and the existing beginning-of-execution queue reset
remains. Public GetOutputArgument/GetWrappersToDelete signatures are unchanged.
One nonvirtual cleanup method is added; no member, object layout or virtual slot
changes. Referenced resource ownership stays with existing wrapper/object-store
implementations. External plugins/API compatibility remain unqualified.

Build and bounded checks
------------------------

Stage: build/nasa-coverage/coverage95-20261105-implementation-function-output-ownership.
Five exact preimages and incremental-v1/v2/v3.patch remain. V1 compiled only;
v2 corrected comments and passed nine existing checks. V3 preserves the original
direct-client caller contract while retaining the complete internal cleanup.
Final patch SHA256:
77535920fcdcda44f90d875563d45b64d0cdfeb8ee873fa09624954d22f0e587.
Reverse patch check passes. Private builds pass15.52/1.69/2.79s with no diagnostics;
summary.json records final source/image hashes. Seven installed images unchanged.

Final v3 passes the same nine existing GUI cases unchanged: I/O Scalar,Array,
String,Object; builtin QuaternionProduct,StringNumberInvalid; control Recursion,
Mutual,Stop. Original values,error/correction and same-session recovery assertions
remain. Native xcb, resizable initially minimized desktop, zero screenshots,
private core mapped and installed core absent; all handles terminal. Replay uses
run-existing-v2.py and saved case commands with fresh output directories. Original
v2 receipts remain separately under existing-*-v1; final v3 receipts use v2 names.

No new tests, fixtures, registrations, matrices, readers or seals. Direct wrapper
alias/unused-output/allocation-failure tests and allocation accounting are deferred;
these checks do not prove a universal leak-free or plugin ABI contract. No original
row admitted. Verified1053/1158,105open,48neededfor1101;59holds,31resolved/
28unresolved including6MATLAB,37MATLAB identities,historical1816/2367 unchanged.
Goal active/incomplete; public code/packages unchanged by this increment.
