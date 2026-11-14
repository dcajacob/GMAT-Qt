Nested MATLAB work in Qt optimizer callbacks
============================================

An outer fmincon call occupies the serialized C Engine worker while waiting for GMAT to evaluate its optimizer branch. Routing another MATLAB operation from that branch back to the same worker cannot complete normally. This implementation gives protocol-v2 callbacks a reciprocal path through the MATLAB client that is already waiting for GMAT. It covers engine/workspace operations and nested optimizer global preservation; it does not qualify every vendor-library client or complete an original worksheet row.

Transport and value ownership
-----------------------------

MatlabValue keeps its existing name, representation and fields, but its definition/export ownership moves from the MATLAB plugin to GmatBase. The base-only request/reply and nonvirtual scoped bridge contain no vendor handles. A thread-local context stack provides an executor and per-callback output/name state. A disabled legacy scope masks an enclosing executor. Context IDs are checked after work returns; the executor is copied before invocation so nested context growth does not invalidate it. Worker threads retain their ordinary serialized C API path.

The Qt codec carries double/logical arrays, UTF-8 character values, scalar structures and cells in a fixed packet schema. Numeric values use little-endian binary64 bytes encoded as Base64, preserving column order and binary representation through transport. Dimensions, counts, nesting and logical values are checked. Unsupported MATLAB classes, complex/sparse and unsupported multidimensional values retain the native unsupported policy. A separate character-fetch operation preserves GetString flattening. None of these transport properties has been exercised against MATLAB in this phase.

Qt dispatch and lifetime
------------------------

The existing unversioned evaluate/iteration requests remain accepted. Version 2 permits authenticated, request-bound nested work notifications and completion acknowledgments. MATLAB payloads use private temporary exchange files rather than the bounded network frame. The waiting Qt event loop drains the parent socket explicitly, checks owner retention, disconnect and user interruption, and matches completion by socket, token, parent request and job identity. Scoped temporary storage is retired on exit.

The server permits a nested optimizer request only while waiting for a MATLAB job. Its start method reuses the active endpoint instead of rotating the parent's token. Parent callback retention and current callback selection are separate checks: the parent remains alive while a child optimizer is selected, while each original request stage still validates its actual selection. This is source-level lifetime/routing implementation, not proof of concurrent operation or transactional rollback after a transport failure.

Engine API and nested optimizer state
-------------------------------------

MatlabInterface checks the bridge before worker marshaling for evaluation, open/state/mode checks, values, arrays, strings and temporary-name reservation. The route uses the same base workspace and reserves names there. Per-callback output and calling-object state avoid sharing the busy engine's buffer. Closing or changing the engine mode from a nested callback is rejected; ordinary C Engine serialization and busy guards remain.

The MATLAB job helper evaluates in the base workspace, captures output and exchanges typed values. The fmincon client services job notifications before resuming its wait for the final optimizer result. The helper also saves/restores the parent callback endpoint, nonlinear constraint/Jacobian globals and iteration audit/history with session-scoped LIFO identities. FminconOptimizer owns that guard around a nested solve, preserves parent globals, marks failed initialization unready and avoids closing the parent engine for semantic callback errors. Error-destructor restoration is best effort; arbitrary disconnect/acknowledgment ambiguity remains an explicit limitation.

The optimization driver remains the existing script, byte-identical to its preimage. An exploratory edit treating it as a function was withdrawn after source review. This batch does not change that interface. All three helper/driver build copies match their current sources, and CMake tracks the helpers as link dependencies so changes trigger build synchronization.

Capture chronology and build limits
-----------------------------------

Thirteen participating existing files have preimages and four files are new. The unchanged legacy driver has an additional reference preimage. FminconOptimizer.cpp was captured after terminal v1 and before its first edit/v2. GmatInterface.hpp was captured after terminal v2 and before its first edit/v3. All source edits occurred with the preceding compiler session closed. Earlier build hash sets therefore contain the then-current participating paths, including the exploratory driver in v1; final source hashes match build v4. Four planning snapshots precede the planning updates.

Configured build v1: compiler return 0, 7.6 seconds, final logged progress 80/81.
Configured build v2: compiler return 0, 3.95 seconds, final logged progress 71/71.
Configured build v3: compiler return 0, 19.66 seconds, final logged progress 74/74.
Configured build v4: compiler return 0, 1.53 seconds, final logged progress 2/3.

The terminal compiler exit codes establish build completion; Ninja's final printed counters are retained verbatim and are not treated as independent completion evidence. Existing wstring_convert deprecation diagnostics occur in v1/v2; v3/v4 contain no warning/error lines. Delta whitespace is clean and three help XML files parse. Indexed patches, hashes, compiler process closures, helper asset identities and capture notes remain in the stage directory.

MatlabValue export ownership now resides in GmatBase and requires matching headers/libraries. No existing virtual slots were added. Existing MatlabInterface/GmatInterface instance layouts are unchanged in this packet; the Qt server's private layout changes. Matching configured consumers built successfully; external binary compatibility and other platforms remain unqualified. C++ compilation does not compile or execute the embedded MATLAB code or establish its behavior.

Primary documentation
---------------------

* https://www.mathworks.com/help/matlab/ref/matlab.net.base64encode.html — Binary Base64 and explicit UTF-8 conversion for text; helper dependency introduced R2016b.
* https://www.mathworks.com/help/matlab/ref/matlab.net.base64decode.html — Decode binary payload into bytes before numeric or UTF-8 reconstruction.
* https://www.mathworks.com/help/matlab/ref/evalin.html — Evaluate nested jobs in the existing base workspace.
* https://www.mathworks.com/help/matlab/ref/evalc.html — Capture MATLAB evaluation output in a character result.

Remaining implementation and status
-----------------------------------

Finish the full original function I/O/scoping/resource/property/output and MATLAB-to-GMAT restriction requirements without banning native GMAT functions inside optimizer branches. Review MAT-file writer operations: MatWriter, RealMatData and StringMatData still marshal vendor work to the engine worker and need an explicit policy for nested callbacks. Finish transport failure/cancellation cleanup, arbitrary nested/concurrent sessions and other-platform behavior, preserving original callback ownership. Complete coherent changed-component/membership/topology/storage preparation, state/result/posterior and estimator/smoother semantics, public restoration-buffer contracts, shared external values/lifetimes and source/stream/participant/error/noise/hardware providers. Continue private renderer, any-Resource Save/persistence, body/kernel/NAIF/CSPICE-pool and ephemeris B-1/format/final-iterative/scientific scopes. Preserve specification/external/historical dependencies and independent POV/rotation research. Defer runtime verification until the full implementation pass is complete.

No runtime/GUI/MATLAB/helper/model/mission/numerical tests, new tests, acceptance auditors/readers/seals or evidence admission were performed. Assertions, fixtures, tolerances, denominator, gates, fingerprints, source text and dispositions are unchanged. No commits, pushes, packages, installed/shared runtime changes or external messages were performed.

Tracking-v101 remains 1093/1158 (94.39%), with 65 unresolved rows. The recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. No additional row or verification credit is claimed. Full implementation audit is incomplete; the goal remains active.
