MAT-file output during optimizer callbacks
==========================================

The prior nested MATLAB bridge covered engine/workspace calls, but MatWriter and its leaf containers still sent C library work to the already-busy engine worker. Keeping a native MATFile open across calls also prevented safely switching to the waiting MATLAB process for output. This packet changes the Linux Qt writer to assemble independent values and complete each native file operation before returning.

Owned data and record assembly
------------------------------

Numeric matrices are captured in column-major order. Rectangular string matrices remain cell matrices; jagged real and string inputs remain outer row cells of column values. Three-dimensional inputs remain row cells containing rectangular numeric/string matrices. Shapes and native capacities are checked before allocation; undeclared/null containers and malformed rectangular inputs fail before file modification. Record templates retain declared field order and initialize missing fields with empty numeric values. All records are assembled before a write, including described records with no supplied values.

MatData copies now retain numDim, and its assignment condition is corrected. RealMatData/StringMatData copies retain the actual 2-D and 3-D buffers rather than only base metadata. Their new nonvirtual value accessors call no vendor APIs. The Qt writer stages independent container copies, preserves aliases within its own copied block list and does not copy a live file-open state. ClearData/destruction release transferred containers once by identity. This follows the existing AddData ownership contract; it does not establish new concurrent leases for arbitrary external container borrowers.

File lifecycle and dispatch
---------------------------

OpenFile anchors the selected filename as an absolute path, creates/truncates the requested file and closes its native handle. WriteData opens that existing file for update, creates/fills a temporary native structure, checks matPutVariable and matClose and releases all arrays/handles on the library worker. CloseFile ends the logical output session; no native handle remains between operations. Reentrancy guards protect active writer operations, and a write stages its values before worker dispatch. The new CreateMatlabArray entrypoint reuses existing typed conversion and refuses native array creation outside the Qt library worker.

Inside a current protocol-v2 callback, OPEN_MAT_FILE and WRITE_MAT_FILE jobs execute through the waiting MATLAB client. The helper uses local variables, converts record cells into a shaped structure array and saves that variable without leaking data into the base workspace. Explicit MAT format keeps filenames without an automatic suffix; appends retain earlier output variables, with the required version-6 option. Version 4 rejects structured output explicitly. Save warnings on data writes become errors, and the prior lastwarn state is restored. Ordinary writes still use the vendor MAT-file API without requiring a MATLAB engine to be open.

The same logical writer can therefore choose the native worker or waiting client at each operation. This is implementation, not demonstrated round-trip or format behavior. Short-lived handles change the former file-handle lifetime: external rename/replacement/concurrent writers and filesystem failures still require lifecycle verification. Partial external writes are not rolled back on transport failure or a save warning. Existing raw RealMatData/StringMatData WriteData entrypoints still require serialized native handles and retain the busy-worker guard; they are not rerouted across processes.

Scope and compatibility
-----------------------

The new writer implementation is selected by the existing Linux Qt MATLAB-engine build definition. Other platforms retain their existing I/O route; leaf buffered-copy corrections and nonvirtual value accessors are common. MatWriter adds private state with an unconditional declaration so every consumer sees the same layout. Matching plugin consumers must rebuild; external binary/plugin and other-platform qualification remain undone. No existing virtual slots are added. General MATLAB function-call restrictions, all-command function/solver semantics and broader original worksheet clauses remain open.

Fourteen existing source/help preimages, three new source paths and four planning snapshots preceded edits. Before/build/final hashes, indexed patches, source map, logs, terminal compiler receipts and helper asset identities remain in the stage directory. All final source hashes match configured build v2. Three help XML files parse and added-line whitespace is clean.

Configured build v1: return 0, 6.47 seconds, final printed progress 78/79.
Configured build v2: return 0, 0.19 seconds, final printed progress 3/3.

Terminal exit codes establish build completion; printed Ninja counters are retained verbatim. Build v1 compiles the C++ implementation and has existing wstring_convert deprecation diagnostics. Build v2 relinks/copies the changed MATLAB helper; it has no warning/error lines. All three build-tree helper/driver copies match current sources. Compilation does not parse or execute MATLAB code or prove MAT-file content, format/encoding equivalence, cancellation behavior or nested solver results.

Primary documentation
---------------------

* https://www.mathworks.com/help/matlab/apiref/matopen.html — Native file open/update modes and version choices; handles must close before another route writes the file.
* https://www.mathworks.com/help/matlab/ref/save.html — Local scalar-structure fields save as named variables; append preserves other variables, requires -v6 for version 6; version 4 cannot store structures. Explicit -mat keeps the supplied filename.
* https://www.mathworks.com/help/matlab/ref/lastwarn.html — Read warnings even when display is disabled; preserve/restore the enclosing last-warning message and identifier.

The nested w mapping uses uncompressed version-7 output; this choice follows the documented compression/Unicode capabilities, but byte-level equivalence to native matOpen(w) is unproven. Empty-file creation, ASCII/version-6 encodings and every supported version require later verification.

Remaining work
--------------

Complete original MATLAB-to-GMAT function restrictions against GmatInterface and the registered legacy script-control/mission paths, distinguishing callback objective evaluation from a general function-call facility. Preserve native GMAT function and command execution inside optimizer and target sequences. Finish complete function I/O/scoping/resource/property/output staging, shared globals and dependency retention. Complete transport interruption/acknowledgment cleanup and file/session lifecycle semantics across supported platforms, including raw vendor-handle leaf clients. Finish coherent changed-component/membership/topology/storage preparation, state/result/posterior and estimator/smoother semantics, public restoration-buffer contracts, shared external values/lifetimes and source/stream/participant/error/noise/hardware providers. Continue private renderer, any-Resource Save/persistence, body/kernel/NAIF/CSPICE-pool, ephemeris B-1/format/final-iterative/scientific and all specification/external/historical dependencies. Continue independent POV/rotation research. Defer runtime verification until the full implementation pass is complete.

No runtime/GUI/MATLAB/helper/model/mission/numerical tests, new tests, acceptance auditors/readers/seals or evidence admission occurred. Original assertions, fixtures, tolerances, denominator, gates, fingerprints, source text and dispositions are unchanged. No commits, pushes, packages, installed/shared runtime updates or external messages occurred.

Tracking-v102 remains 1093/1158 (94.39%), with 65 unresolved rows. The recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. No row or verification credit is claimed. Full implementation audit incomplete; goal active.
