Ephemeris filename rollover and local cleanup, 2026-11-05
========================================================

This batch continues FRR!E406 implementation. It does not complete that broad
row or add requirement/acceptance credit. The engineering tally stays
1086/1158 (93.78%); recorded verification stays 1053/1158 (90.9%). All 105
original acceptance rows remain open and all original gates remain unchanged.

Filename lifecycle
------------------

Filename assignment resolves the requested name into temporaries and validates
both its raw path and actual format-normalized destination before touching the
current configuration or writer. Directory destinations and missing parents
fail first. Embedded null characters reject. Existing invalid-name diagnostics
and extension warnings are retained. Native requested spelling remains deferred
until writer creation; format correction changes only the basename suffix,
never a directory containing .eph/.oem or another dot.

A real destination change flushes accepted samples, closes the concrete output,
deletes the old writer and resets per-file epoch/interpolation/maneuver/gap
state. The new output opens lazily on the next accepted sample or ordinary
ToggleOn. A file boundary is not a mission end: retirement uses nonterminal
flush flags and explicit closure, avoiding a false warning about a future
configured FinalEpoch. A finalization failure retains writer ownership and the
original destination. This is not an atomic filesystem transaction: a later
open/write error can fail after the old output has been successfully finalized.
Already written old output is retained.

Repeated assignments to the same output leave its live writer and buffers
untouched. Existing aliases are compared using POSIX device/inode or Windows
volume/file identifiers. If an existing destination's identity cannot be
established, the assignment fails before retirement. A missing destination is
allowed. Filesystem changes racing the subsequent open are not made atomic.
The Windows branch is source-only in this batch and has not been compiled or
qualified on Windows; Mac behavior is likewise not qualified here.

Trial assignments update command-visible configuration without changing the
active writer. The prior trial snapshot now includes Filename, full path,
previous name and default-name status. Those names are restored with the write
flags before final replay or terminal cleanup. Initialization inside an effective
SOLVING context also skips deletion of a pre-existing output. Ordinary
initialization outside trials retains its old stale-output behavior.

Fresh writer ownership
----------------------

Writer construction/configuration/opening use temporary unique ownership. The
member pointer is committed only after successful initialization and open, so
an exception cannot leave a partially configured member that causes the next
call to skip setup. False Initialize results fail explicitly. The configured
format options, frame, spacecraft, time bounds, interpolation policy and local/
global/background roles are supplied to the fresh writer. Background generation
is now retained in the parent as well as applied to an existing writer.

Three strings and a default-name flag extend the preceding private trial
snapshot. Copies do not acquire live snapshots; assignment clears them. Existing
parameter IDs, public signatures, formats, interpolation formulas and tolerances
are unchanged. All dependent images were rebuilt together.

Function-local finalization and cleanup
--------------------------------------

EphemerisFile's Finalize action now takes the terminal path even if collection
has been toggled off, so accepted data can be finalized and trial output names/
flags restored. It does not create a writer for an empty local subscriber.

The active FunctionManager cleanup routes use a common finalizer that attempts
unsubscribe on success or failure. Ephemeris finalization errors are forwarded;
legacy nonfatal plot diagnostics retain their prior policy. Single-object
replacement retains a failed object for its owner's cleanup, already
unsubscribed. End-of-call unsubscribe deduplicates aliases, attempts every owned
subscriber and then forwards its first failure. True globals are excluded.

Cleanup retains the first function-finalization/subscriber error and continues
retiring owned resources. Subscribers are finalized/deleted before spacecraft,
frames and other resources needed for final metadata. Once ownership containers
are cleared, completed cleanup is recorded before rethrow, preventing a retry
from revisiting freed objects. Existing borrowed GroundStation ownership is
preserved. The disabled legacy EmptyObjectMap deletion route was not reenabled;
DeleteObjectMap still deletes containers only. No universal recursive-output or
resource-binding contract is inferred from this repair.

Build receipts and remaining implementation
------------------------------------------

Stage: ``build/nasa-coverage/coverage95-20261105-implementation-ephemeris-filename-rollover``.
All configured integrated builds pass:

* v1: 77 steps / 158.65 seconds (rollover, snapshot layout and writer ownership).
* v2: 67 steps / 4.14 seconds (local finalization/cleanup).
* v3: 66 steps / 3.38 seconds (diagnostic compatibility).
* v4: 66 steps / 3.43 seconds (file-boundary flush semantics).
* v5: 66 steps / 4.67 seconds (same-file identity failure handling).
* v6: 66 steps / 3.76 seconds (trial initialization deletion guard).

Sessions 30479/11108/70625/28225/70076 completed; the interrupted v6 session
42905 is no longer live and its authoritative build-v6.json records exit 0.
Use the per-build JSON/logs and exact source-at-build patches for details.
Base, Qt, console, plugins and Python bindings built; test/probe clients were
linked only. No tests, GUI, function/model/solver execution, numerical
comparisons, acceptance readers/seals or qualification runs were launched.
No original fixture, assertion, tolerance, fingerprint, denominator, acceptance
ledger or gate changed. No installed/shared runtime, package, commit or push
update.

FRR!E406 remains partial. Next review recursive subscriber instances, caller
bindings and whole-object subscriber copies during iterative function execution.
The existing EphemerisFile assignment operator retires the destination writer;
do not assume that this is harmless when a live output is copied in a trial.
Preserve native RunInitialGuess and terminal-last-iterate replay; SOLVEDPASS
alone is not a convergence certificate. Complete any evidenced missing source
routes before returning to verification or admitting the broad row.

Later qualification must cover all output formats, new/existing/alias targets,
invalid and unwritable destinations, failure during flush/open, missing or
dotted filename suffixes, trial initialization and renames, initially disabled
outputs, nested/final replays, local/global subscribers, aliased maps, error
cleanup, repeated/recursive function calls and Windows/Mac identity paths.
