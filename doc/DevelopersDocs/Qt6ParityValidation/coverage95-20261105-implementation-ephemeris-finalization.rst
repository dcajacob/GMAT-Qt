Ephemeris terminal errors and SPK cleanup
========================================

Implementation-first repair in Publisher.cpp, Subscriber.cpp,
EphemerisWriter.cpp, EphemWriterSPK.cpp and SpiceOrbitKernelWriter.cpp.
No new tests, assertions, fixtures, registrations, matrices, readers or seals.
No header, object layout, virtual slot, interpolation algorithm, order, state,
clock or scientific tolerance changed.

Production behavior
-------------------

EphemerisWriter::HandleEndOfRun previously caught a final write exception,
printed it and returned normally. EphemWriterSPK::FinishUpWritingSPK also caught
and discarded segment-write exceptions. Its failed-writer flag then prevented
a later close from reporting that lost segment. Both explicit completion paths
now propagate the original error, so an incomplete ephemeris cannot become a
successful mission merely because the failure happened during the last flush.

Publisher::NotifyEndOfRun retains the first exception, continues terminal
notification for the remaining subscribers, and rethrows that exception after
the loop. False callback results accumulate instead of stopping notification
early. Subscriber's end-of-receive/block/run flags clear on exceptional exits as
well as normal ones, allowing subsequent initialization/recovery to proceed.

Explicit SPK native close failures now raise UtilityException instead of being
silently cleared or only printed. The native handle is not marked closed when
close fails. SPK wrapper and utility destructors contain finalization exceptions
and still attempt native resource release, rather than replacing a mission
error by throwing from destruction. The utility falls back to closing a
remaining DAF handle after failed finalization. This is best-effort teardown,
not a guarantee that every operating-system/native-close failure is recoverable.

SPK finalization previously allocated saved endpoint copies even for done=true,
then never inserted or freed them. It also leaked those copies when a write
threw. They now have scoped ownership and are allocated only for continuation;
ownership transfers back to the retained arrays when continuation succeeds.
Precise endpoint metadata must align before an append endpoint is retained.

Build and existing checks
-------------------------

Artifacts are under
``build/nasa-coverage/coverage95-20261105-implementation-ephemeris-finalization``.
Exact preimages and incremental-v1.patch are retained. Patch SHA256:
``d2ec7a18f820d0534899975825a0f44025a519a3fa32a2adc46e90cd1a9ef6e4``.
Reverse application checks pass; the increment adds no trailing whitespace.

The matched private integrated-current build passes all 60 affected steps in
3.17 seconds, including the five changed translation units and affected plugin,
Qt and console links. No warnings or errors were produced. Seven monitored
installed images remain byte-identical; public code/releases are untouched.
Final Base SHA256:
``92b14fc5fb91b3329b134b3ab70f459837f4a7a62eced56938273df35c3c0f38``.
``summary.json`` records the current private runtime hashes. Earlier build and
increment receipts remain historical records of their own source/image versions.

Existing NasaSpkWriterBoundaryTests, NasaSpkOutputTests,
NasaInterruptedEphemerisTests and AcceptanceSupport compile unchanged against
the private current core/frontend. Direct compiler/linker commands and outputs
are retained under existing-*/commands.json; no shared-output build is invoked.
Five existing checks pass:

* Storage: 19 original invalid-input classes rejected, 90 decoded components,
  zero outstanding tracked C++ arrays.
* Recovery: 256 original native rejections, four abandoned-writer cleanup
  checks, 90 decoded components, zero outstanding arrays/native-handle leakage.
* Degree7: the actual GUI export finishes with one SPK segment, 34 raw knots,
  original clock/state/reader assertions and continuation sentinel 42.
* NestedDCRuntimeError: Failed with zero OEM rows, then two Completed recoveries
  with exactly 25 final-trajectory rows each.
* NestedYukonStopCommand: Stopped with zero OEM rows, then two Completed
  recoveries with exactly 25 final-trajectory rows each.

Boundary checks are native support using QCoreApplication. The other cases
exercise their original actual GUI routes through native xcb. Nested desktops
are resizable and start minimized; zero screenshots were created. Process maps
identify the new private Base and exclude the older installed Base. Loader
initialization logs also identify Base for the very short native support runs.
All test/display handles are terminal. Original assertions, tolerances, negative
expectations and recovery phases are unchanged.

Replay uses saved per-case command.json files and the existing isolated startup
and VisibleTestDisplay helpers with new output directories. Keep original
receipts and artifacts intact.

Deferred qualification
----------------------

Direct terminal-write/metadata/close fault injection, multiple simultaneous
subscriber errors, throwing non-BaseException callbacks, continuation endpoint
allocation accounting and same-session recovery after those terminal failures
remain deferred. The existing checks cover affected normal export and native/
solver error recovery; they do not directly prove every new exception guard.
No atomic output-file rollback, all provider/science/platform behavior or whole
FRR!E406 final-solution conjunction is qualified.

NarrowAtLast remains its original accuracy failure. No algorithm, input fixture,
expected degree, numerical bound or error condition was changed to clear it.
All original source identities and held contradictions remain intact.

No original rows admitted: verified 1053/1158, 105 open, 48 needed for 1101.
All 59 prerequisite identities, 31 resolved/28 unresolved including six MATLAB,
and separate historical 1816/2367 coverage remain unchanged. Goal active and
incomplete; continue concrete supported implementation gaps before new test
expansion.
