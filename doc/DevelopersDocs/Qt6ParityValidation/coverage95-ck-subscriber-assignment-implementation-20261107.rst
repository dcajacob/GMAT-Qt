CK subscriber assignment and clock isolation
============================================

CK subscriber copies now retain configuration in a detached, uninitialized session. Assignment stages all incoming values before strictly retiring the destination writer and its generated clock, then publishes prepared values without transferring an open stream. New clocks use atomically reserved private directories. CoordinateConverter copies now retain the complete matrices, derivatives and matching epoch cache, and CK finalization no longer leaks unnecessary carry-over samples. These source repairs strengthen partial dependencies and claim no complete original requirement or verification credit.

Owning-source findings and implementation
------------------------------------------

The old AttitudeWriterCK assignment overwrote its owned native pointer with NULL after base assignment and copied the source clock filename. It could leak the destination session and later retire a clock it did not own. Copy construction also inherited readiness and segment counters without a writer or samples. Copies now reset session epochs, readiness, counters, maneuver history and sample arrays while retaining configured times, stepping, formats, scopes and referenced provider bindings. The current sample arrays are initialized in the base constructor and copied as values in its copy constructor; invalid detached epochs keep zeroed sample storage from being published as data.

Assignment constructs a detached prepared value before any retirement. RetireWriter marks the old session unready, finalizes its pending samples or explicitly aborts a known failed writer, checks debug text-stream closure and strictly retires the owned clock and empty directory. Only after successful retirement does SwapState exchange all 78 declared base value/reference/buffer fields; the closed dstream is deliberately excluded. Fixed sample arrays and complete converter cache values are exchanged without allocations. Native pointers and private clock paths are not borrowed from the source. A prepared temporary owns and destroys old pointer buffers once. Failure to retire stops publication and retains the destination native owner and pending paths for retry; actual file finalization is not rolled back.

Clock filenames formerly used spacecraft name plus a microsecond/performance-counter timestamp, without reserving a filesystem namespace. ReserveCKClock now prepares path strings before atomically creating a private gmat_ck directory, retries existing names and publishes paths by swaps only after acquisition. Each directory contains the generated clock.tsc. Cleanup checks native unload/file retirement before removing only the empty owned directory; unexpected contents cause an error and are never recursively deleted. Constructor retry retires a pending prior namespace before replacing its ownership record. A failed creation no longer publishes isEphemFileOpened; a missing writer is not reported open.

The subscriber destructor uses the same strict retirement route, guards retirement diagnostics, attempts an explicit native abort after a retirement error, retains uncertain clock paths/files and frees remaining sample pointers. FinalizeCKFile now allocates its last-sample carry-over only when continuing a segment and keeps those allocations in unique_ptr until successful segment writing. The existing ClearOrbitData path retains vector capacity for the continuing sample publication. The general BufferOrbitData allocation route remains a separate follow-up.

CoordinateConverter formerly copied specifyRotMatrixDeriv and lastIcrfToFk5Epoch while defaulting or retaining unrelated matrices and derivative storage. A same-epoch query could therefore reuse the wrong ICRF/FK5 cache. The copy constructor now captures every cache value, assignment stages a full copy, and a new nonvirtual Swap exchanges fixed 3x3 elements, derivative-vector ownership and the matching epoch. No frame equations, units, clock coefficients, numerical interpolation, NAIF IDs or native SPICE APIs are changed. CoordinateConverter and EphemerisWriter gain nonvirtual methods only; AttitudeWriterCK gains one owned directory string and therefore requires a matching native rebuild.

Limits
------

Compilation and static source consistency establish no runtime acceptance result. Copies preserve provider references rather than duplicating externally owned spacecraft, frames or models. Configured CK output filenames remain shared external names; assignment does not duplicate files or undo prior finalization. Complete native kernel-pool leases, concurrent outputs, CK temporary metadata filenames, generic base/other-format writer assignment, raw CK BufferOrbitData allocation-failure staging, constructor failures outside these paths, callback/function/provider graph closure and all other original requirements remain open. This is not a complete implementation audit. No runtime/numerical/GUI/function/mission/native-helper verification was performed.

Compilation and retained evidence
---------------------------------

Build v1 returned 0 in 117.58s, progress 427/427, 1 existing compiler deprecation warnings. All compiler sessions are terminal. Final hashes match build v1. 7 existing source/help preimages, 0 absent-new-path records, 14 unchanged reference files and four planning snapshots preceded edits. Indexed/combined patches, hashes, source findings and compiler receipts are retained. New text passes whitespace checks (0 inherited whitespace lines recognized by the diff). Participating help XML files parse.

No new tests, native/model/helper/parser/GUI/MATLAB/mission/numerical/runtime execution, acceptance auditors/readers/seals or evidence admission occurred. Original requirements/text/fingerprints/fixtures/assertions/tolerances/denominator/gates/dispositions remain unchanged. No row or verification credit. Tracking-v145 retains 1093/1158 (94.39%), 65 unresolved engineering rows. Recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full implementation audit and goal remain incomplete.

Next implementation
-------------------

Return to the original-row implementation worklist, prioritizing remaining public resource clone/body/point and function/provider graph behavior. Retain CK sample-buffer allocation failures, shared output and metadata-file ownership as explicit follow-ups where required by those original rows. Do not run verification or change original requirements, fixtures, assertions, tolerances or dispositions.
