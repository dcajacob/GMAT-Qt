Function invocation ownership and cleanup
========================================

Implementation-first repair in FunctionManager.cpp, ObjectManagedFunction.cpp
and GmatFunction.cpp. No new test cases, fixtures, assertions, registrations,
matrices or qualification readers were added. Headers, object layouts, virtual
slots and numerical formulas are unchanged.

The copied FunctionManager previously retained output-wrapper pointers that
both instances later deleted. Copies now retain configuration and rebuild
their own invocation arguments. Assignment releases the destination's inactive
runtime and rejects assignment to an executing manager. ObjectManagedFunction
copies keep argument names with null wrappers, without borrowing another
invocation's live bindings or pending wrapper deletion list. A nonowning
manager no longer deletes another manager's object-store map in its destructor.

Cleanup retires a call before recursive FCS RunComplete can revisit it. It
clears call, local-store and caller stacks along with value snapshots, wrappers
and cached object results. The owning manager detaches a released object store
from the function. Initialize recreates a missing store even when the function
retains its initialized flag. Cleanup does not claim completion if finalization
throws. This is not an atomic rollback guarantee for every failing resource.

Argument initialization failures are checked. Output-wrapper creation and
binding failures produce FunctionException instead of dereferencing null or
using an unbound wrapper. Rebuilding arguments resets the expression-result
flag. GetOutputArgument rejects the one-past-end index through FunctionException
instead of allowing std::vector::at to throw std::out_of_range.

Build and existing checks
-------------------------

Artifacts are under
``build/nasa-coverage/coverage95-20261105-implementation-function-lifetime``.
``preimage`` and ``incremental.patch`` retain only these exact production edits.
Patch SHA256:
``0ec5491d47bd191cb9a1c864061c7c24337dc7b2f3f1a8c0e8235e3a1b483ea3``.
Reverse application checks pass; this increment adds no trailing whitespace.
Existing unrelated worktree whitespace is preserved.

Both private builds pass. The final command rebuilds GmatBase, GmatFunction,
GmatQt and GmatConsole with affected plugin links in the existing matched
``coverage95-20261105-integrated-current`` build. No installed binary was
replaced; seven monitored installed images remain byte-identical. This newer
private runtime supersedes the original integrated-build image hashes for
subsequent development checks. Original integrated-build receipts stay intact.
``summary.json`` records final image hashes and the bounded results.

The existing NasaFunctionControlTests and AcceptanceSupport sources were
compiled unchanged and linked against the private current frontend/core.
The saved ``existing-control/commands.json`` contains direct compiler/linker
commands with private outputs; it does not execute the normal shared-output
build. The three existing native GUI cases pass:

* Recursion: depths 0, 1 and 5, factorial outputs and shared call count.
* Mutual: depths 0, 1 and 6, original independent recurrence and global counts.
* Stop: stop while the function runs, preserve later mission state, edit input
  through the actual resource editor, then complete with report [37,37,37].

All three reports have zero error against their original expected values.
Final receipts are ``existing-*-v2/receipt.json`` and
``existing-control-results-v2.json``. The same checks also passed v1 before a
small final cleanup guard refinement; those results are retained separately.
Final Base SHA256 is
``f61718f91e733ff654a1d414f82d92be5251689caa9df291ad636309a5870141``.

Both runs use native xcb in a resizable nested desktop that starts minimized.
Zero screenshots were created. Actual mapped images identify the private core
and exclude the older installed core. All test and display handles are terminal.
Replay uses the saved compiler commands, case command.json files and the
existing AcceptanceRuntime.isolate_startup/VisibleTestDisplay helpers with new
output directories. Keep the recorded failed or successful receipts intact.

Deferred backfill and qualification
----------------------------------

Direct ownership checks for copying/assigning managers after outputs are bound,
destruction in both orders, empty/boundary argument requests, initialization and
result-assignment failures, and throwing cleanup remain deferred. These existing
recursion/stop checks are affected regression evidence, not a complete proof of
every new guard or universal function-resource/global eligibility.

No original acceptance row was admitted. Verified coverage remains 1053/1158,
105 open and 48 needed for 1101. All 59 prerequisite identities and separate
historical 1816/2367 coverage remain intact. The campaign is active/incomplete;
continue concrete supported implementation gaps before expanding new tests.
