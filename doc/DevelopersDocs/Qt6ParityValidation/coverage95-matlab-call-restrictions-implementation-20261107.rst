MATLAB function reentry and legacy script controls
=================================================

The fixed Qt optimizer protocol has no general function-call command, and direct callback registration already rejects Function resources. The separate legacy interfaces nevertheless expose script construction, updates, whole-mission execution and optimizer callbacks through GmatInterface. Those operations could be reentered while a GMAT-called MATLAB function was executing. The wx socket service could also queue a command for execution after the MATLAB function returned. This packet adds activity checks at evaluation, receipt and execution boundaries.

Execution boundary
------------------

A noncopyable, nonvirtual MatlabFunctionScope increments a checked process-visible atomic activity count. CallMatlabFunction uses it only around MATLAB evaluation operations: startup/Open, checked/unchecked expressions, temporary-name reservation and explicit/destructor workspace cleanup. The count spans the synchronous worker wait and UI servicing, and unwinds on exceptions. Native argument evaluation, output setters and native GMAT FunctionManager/command execution retain their own normal path. The fmincon driver is not marked as a user MATLAB function, so its legitimate objective callbacks remain available.

GmatInterface rejects callback execution, input replacement and iteration publication while that function scope is active. ExecuteCallback checks again after interrupt handling. Script controls also reject active MATLAB functions, active callback scopes and non-idle mission state. A separate atomic operation scope prevents reentrant script build/update/run/edit operations while UI callbacks are serviced. Callback activity is counted for the actual scope lifetime, even if its registration is removed in the meantime. These counters do not introduce support for concurrent native missions or replace existing native object lifetime/thread contracts.

Legacy receipt and dispatch
---------------------------

The standalone and wx socket receivers classify script Pokes against process-visible activity before acknowledgment and queuing. A denied command is discarded; it cannot become an allowed command merely because a later UI event runs after the function returns. The existing socket echo remains a receipt acknowledgment, not an execution-result protocol. Close is treated as transport cleanup and does not append a Close statement to a script. Core dispatch checks remain authoritative for requests accepted earlier but no longer allowed when dispatched.

Standalone SocketServer::OnPoke and wx GmatConnection::RunPoke catch BaseException and report rejection rather than letting it escape the IPC handler. Read requests retain the existing cached object/parameter/status/result paths; this packet does not claim arbitrary plugin getters are side-effect-free. The old generic script buffer remains separate from the fixed optimizer-evaluation protocol.

Script validity
---------------

Appending or clearing text invalidates prior interpretation success. Build/update allocate an input stream if necessary, reset stream state and rewind the staged input before interpretation. Update records its actual interpreter result instead of retaining an earlier build's result. A failed build/update cannot permit RunScript using a preceding success flag. Run requires successful current interpretation; UI refresh is conditional on an available interpreter. Clear explicitly requires a UI interpreter before changing project state. Native GUI project replacement and every external edit source still require broader current-project provenance review.

Scope of the original requirement
---------------------------------

FRR!E479 says MATLAB functions shall not call GMAT functions. This packet implements rejection during GMAT-called MATLAB evaluation and preserves native function calls in optimizer/target branches. Independent MATLAB mission control while GMAT is idle is still a separate supported interface. The unversioned legacy protocol does not carry enough caller-origin information to classify every independent/background MATLAB function. That full requirement is not declared closed or counted from these checks. Original source text, fingerprint and disposition are unchanged.

Compilation and compatibility
-----------------------------

Configured C++ build v1: return 0, 21.1 seconds, final logged progress 75/75.
Configured C++ build v2: return 0, 4.71 seconds, final logged progress 6/6.
Additional standalone SocketServer.cpp syntax compilation: return 0, 0.48 seconds.

The terminal exit codes establish build completion; printed Ninja counters are retained verbatim. Main compiler diagnostics are preserved in the stage summary. The standalone syntax check reports the pre-existing LINUX_MAC macro redefinition. GmatConnection.cpp and GmatSocketServer.cpp are excluded from the configured Qt build, and no wx-config/development headers were found in the checked host/dependency roots. Their edits are source-reviewed only; no compilation or platform qualification is claimed for them. No fake/replacement wx headers were used.

Nine source/help preimages and four planning snapshots preceded edits. Final source hashes match configured v2; the standalone syntax receipt matches its final source. Delta whitespace is clean and three help XML files parse. Indexed patches, source map, compiler logs/terminal receipts and the explicit wx build limitation are retained. New scopes and static command checks are nonvirtual. Existing GmatInterface/command instance layouts and existing virtual slots are unchanged; matching configured consumers rebuilt. External binary and other-platform qualification remain undone.

Remaining implementation
------------------------

Finish the original MATLAB-to-GMAT restriction with explicit caller/provenance semantics across every supported interface; do not equate an external mission-control script with a direct function-call facility or prohibit native GMAT commands inside optimizer/target sequences. Complete legacy partial-frame/disconnect/request-delivery and result/status handling, asynchronous request ownership, full function I/O/scoping/resource/property/output staging, shared globals and dependency retention. Complete transport interruption/acknowledgment cleanup, file/session lifecycle and supported-platform semantics. Finish coherent changed-component/membership/topology/storage preparation, state/result/posterior and estimator/smoother semantics, public restoration-buffer contracts, shared external values/lifetimes and source/stream/participant/error/noise/hardware providers. Continue private renderer, any-Resource Save/persistence, body/kernel/NAIF/CSPICE-pool, ephemeris B-1/format/final-iterative/scientific and all specification/external/historical dependencies. Continue independent POV/rotation research. Defer runtime verification until the full implementation pass is complete.

No runtime/GUI/MATLAB/helper/model/mission/numerical tests, new tests, acceptance auditors/readers/seals or evidence admission occurred. Original assertions, fixtures, tolerances, denominator, gates, fingerprints, source text and dispositions are unchanged. No commits, pushes, packages, installed/shared runtime updates or external messages occurred.

Tracking-v103 remains 1093/1158 (94.39%), with 65 unresolved rows. Recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. No row or verification credit is claimed. Full implementation audit incomplete; goal active.
