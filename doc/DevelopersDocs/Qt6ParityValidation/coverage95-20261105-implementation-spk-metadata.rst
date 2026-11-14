SPK copy ownership and metadata cleanup
======================================

Implementation-first change in SpiceOrbitKernelWriter.cpp only. No new tests,
assertions, fixtures, registrations, readers or seals. No header, object layout,
virtual slot, interpolation, clock, state or tolerance change.

The copy constructor previously copied the live native DAF handle and open-file
flag. Both writers could then finalize the same file. Copy construction now
rejects an open source; assignment rejects an open source or destination before
mutating either. Closed-writer copies retain their previous configuration and
finalized/data-written/append state, copy the previously omitted temporary
metadata filename, and have an invalid native handle and null stdio pointer.
Closed streams no longer leave a dangling tmpTxtFile pointer in construction.
This is a lifecycle restriction for an internal native writer, not a change to
the GUI's resource-cloning eligibility or a new independent-output clone API.

WriteMetaData now checks fopen, every write, flush and close before handing the
file to SPICE. A lost/unwritable metadata path reports UtilityException instead
of dereferencing a null stream. Scoped cleanup closes a remaining stdio stream
and removes the owned temporary file on success or failure. The dynamically
allocated filename array is replaced by scoped storage with the identical
Fortran length/terminator convention. Native text-open, comment-write and close
failures are checked separately. An acquired SPICE input unit is closed during
unwinding after comment failure, preserving the captured original diagnostic.
Cleanup remains best-effort if the native close itself fails.

The constructor's existing unavailable-metadata probe fallback is unchanged;
this repair reports later failures after metadata writing was enabled. All
metadata text, temporary-path naming and serialized state/epoch formats remain
unchanged. No whole-format or arbitrary-path conformance is claimed.

Build and existing evidence
---------------------------

Stage: build/nasa-coverage/coverage95-20261105-implementation-spk-metadata.
Exact preimage and incremental-v1.patch retained. Patch SHA256:
``83baec0ae06f4fc2201151aa86ff30df0c1a1acd3111231d1fa5331e669bf8f2``.
The matched private build passes in 2.65 seconds, with no warnings/errors.
Seven monitored installed images remain unchanged; public code/releases are
untouched. Current private Base SHA256:
``a040d12366d5e904155f7ac40ce81fc807fc0addcb2b3357f8a6abc848dfa3a1``.
Use summary.json for this runtime version; prior increment receipts stay intact.

The prior turn's unchanged SPK boundary/output executables are reused against
the updated private core; headers and ABI are unchanged. Storage, Recovery and
Degree7 pass. Their original 19 invalid-input classes, 256 native rejections,
four abandoned-writer checks, zero tracked array/native-handle leakage, decoded
components and 34-knot GUI export assertions remain intact. These tests cover
affected native recovery and successful metadata/export paths. They do not
directly test copying an open writer or inject metadata stream/unit failures.

Native support uses QCoreApplication; the GUI export uses xcb in a resizable
nested desktop that starts minimized. Zero screenshots. Maps/loader logs
identify the new private core and exclude the old installed core. All test and
display handles are terminal. Replay uses saved run-*/command.json files and
the existing startup/display helpers with new output directories.

Direct closed-copy/open-copy/assignment and metadata fault-injection backfill
remains deferred. No original row admitted: verified 1053/1158, 105 open and
48 needed for 1101. All 59 prerequisite identities and separate historical
1816/2367 remain unchanged. The campaign stays active/incomplete.
