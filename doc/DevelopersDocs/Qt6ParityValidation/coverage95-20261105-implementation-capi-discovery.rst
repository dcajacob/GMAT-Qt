C-interface command-graph discovery
==================================

The prior C-interface repair fixed stale runtime pointers and derivative-buffer
sizes. This increment repairs discovery itself, continuing the remaining native
client configuration scope in All Others!E249/E251/E252. It changes one C++ file
and adds a comment to the C header. No numerical formula, signature, layout,
virtual slot, original assertion, tolerance or fixture changes.

Runtime discovery enumerates child branches and following commands in script
order, collecting every PropSetup associated with each Propagate command.
Visited-command tracking handles branch-end back-links without looping or
duplicating entries. The walker uses an explicit stack. Native branch
initialization establishes children even when mission execution did not choose
that branch. First-match name semantics and run-local model IDs remain.

Model lookup filters propagators through UsesODEModel, so an ephemeris setup
does not hide the next numerical model behind an unused/default ODE pointer.
Matching models are prepared only if a numerical state manager is empty, using
the existing PrepareToPropagate action. Already populated state is retained,
including external C-client assignments. The C-interface source contains no
command Execute call. Looking up a model therefore cannot run or resume a
mission command. Native preparation can still initialize model/publisher data;
this is not a universal absence-of-side-effects claim.

The first-propagator internal helper shares branch enumeration. The legacy
indexed internal helper retains its interface but now calls preparation rather
than Execute. The public FindOdeModel route uses complete enumeration directly.
Function-private invocation state is not made persistent or newly exposed, and
no disambiguating grammar for same-name runtime clones is added.

Build and reused smoke
----------------------

Artifacts reside under
``build/nasa-coverage/coverage95-20261105-implementation-capi-discovery``.
The affected translation unit compiles and the private candidate links without
diagnostics. Exact preimages, file hashes and the incremental patch isolate the
change. Reverse-application and added-whitespace checks pass. Existing support
objects and base libraries are reused; this is not a full build of all current
worktree changes. No installed library or public package was replaced.

Unchanged ``GmatNasaCapiAcceptanceTests`` basic/stm checks pass with 133 and 481
assertions. Each contains two independent models; state dimensions are six and
42. Original orbital/force expectations, state assignment, selection and
derivative comparisons remain. Loaded-image output identifies the candidate
and excludes the older installed CInterface. These are native C support checks,
with zero GUI executions and no original-row admission. All processes finished.

Replay uses each smoke directory's command.json from its session directory,
with the stage's plugins directory prepended to LD_LIBRARY_PATH. The saved
startup uses an extensionless private PLUGIN path. The saved scripts come from
the unchanged existing fixture builder. Preserve earlier results on replay.
No new matrix, source capsule, reader or acceptance seal was produced.

Deferred validation
-------------------

After implementation, backfill nested/empty/multiple branches and loops,
multiple setups per command, ephemeris-before-numerical discovery, lazy
Prepare-only initialization, stopped-run queries and query-order changes after
client state assignment. The completed-run two-circle smoke does not prove
these new paths. Full force/provider, naming, function, platform/ABI, scientific,
current-build and GUI conjunctions remain open.

Verified coverage remains 1053/1158, with 48 further qualified rows needed for
1101. Original hold identities and separate historical coverage remain intact.
All Others!E249/E251/E252 remain open; the campaign is incomplete.
