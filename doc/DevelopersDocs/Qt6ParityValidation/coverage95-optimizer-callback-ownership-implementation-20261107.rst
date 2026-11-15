Scoped optimizer callback ownership
===================================

The Qt callback service already accepted only fixed optimizer evaluation/iteration operations. Its shared GmatInterface nevertheless routed through one raw callback pointer. Initializing another optimizer could replace that passive selection, stale callback owners were unchecked, and ExecuteCallback reported success regardless of the service return value. This packet implements actual owner/lifetime routing at execution and request boundaries.

Source scope and worksheet restriction
-------------------------------------

The inspected Qt service has no general script, resource mutation or GMAT function-call operation. GmatInterface and legacy SocketServer separately expose script build/update/run paths. This packet does not infer a complete MATLAB-to-GMAT call prohibition from the restricted Qt protocol. Direct Function-resource callback registration is now rejected, while the original FRR!E479 source text, disposition and open state remain unchanged. Full legacy mission/script and cross-call semantics still require review. No general ban on GMAT functions inside Optimize was introduced.

Registration and routing
------------------------

Passive registration records the actual object's weak lifetime token. Every callback input/status/result/iteration/execute path resolves a live registration before dereferencing it. Expired pointers are cleared. Null and Function resources are rejected as callback services, including declarations of GMAT or MATLAB functions; normal command/service callback providers remain available.

A noncopyable, nonvirtual CallbackScope pushes an actual owner record with a unique registration ID. Scoped records take priority over passive registration. MATLAB-backed Optimize selects its own service during actual Execute; another command's initialization cannot redirect that active scope. Nested scopes return to the preceding live selection. Scope destruction removes only its own ID, and unregistration removes all records for that actual object. ID exhaustion fails before wrap/reuse.

The Qt endpoint pins the current selection for the complete request: optional iteration validation, input transfer, evaluation, result parsing and optional iteration publication. It checks that selection at every step. Core service operations also hold and validate their own selected records. An ended/unregistered/replaced selection fails the request instead of allowing subsequent stages to fall through to another callback. This validates routing; it does not provide atomic rollback of external effects already performed by a service.

Execution and reentrancy
------------------------

ExecuteCallback validates its selection after CheckUserInterrupt, which can process UI events, and after the service returns. It returns the actual boolean execution result. A callback already executing cannot be invoked again through that same selection, and its inputs cannot be replaced during execution. Distinct nested command scopes retain their own routing. Optimize's existing callback execution guard, state-machine behavior and native GMAT command branch remain in place.

The Qt endpoint's existing processing guard and MATLAB engine/transport restrictions remain. Scoped owner routing alone does not establish arbitrary nested MATLAB optimizers, concurrent dispatch or full native active-session checkpoints. Normal native execution-thread/lifetime contracts still apply; this is not a new concurrent ownership lease on command objects. Callback payload schema, numeric shapes, authentication, interpolation and optimization formulas are unchanged.

Compilation and compatibility
-----------------------------

Configured C++ compiler v1: return 0, 18.15 seconds, last logged step 74/74.
Configured C++ compiler v2: return 0, 18.73 seconds, last logged step 74/74.

Seven source/help and four planning preimages preceded edits. Before/build/final hashes, indexed patches, source map, compiler logs and terminal receipts are retained. All compiler sessions are terminal; final source hashes match the last successful build. Added-line whitespace is clean and three help XML files parse. Existing diagnostic lines are retained in the compiler record. GmatInterface adds weak registration and scope-stack storage, changing its private singleton instance layout. The new scope API is nonvirtual; no existing virtual slots change. Matching native/plugin/GUI consumers rebuilt against the header; independently built binary consumers require matching headers/libraries. External binary/plugin and other platform qualification remain undone.

Complete the original MATLAB-to-GMAT function restriction against the full registered interface and legacy script-control/mission paths without treating optimizer branch execution as a blanket command ban. Preserve native GMAT function calls inside optimizer sequences. Finish callback lifetime/reentrancy and nested engine/transport behavior, whole function I/O/scoping/resource/property/output staging, public edits and component/shared-global dependency retention. Continue coherent changed-component/membership/topology/storage preparation at iteration boundaries, state/result/posterior and estimator/smoother requirements, public restoration-buffer contracts, shared external values/lifetimes and source/stream/participant/error/noise/hardware providers. Finish private renderer, any-Resource Save/persistence, body/kernel/NAIF/CSPICE-pool, ephemeris B-1/format/final-iterative/scientific scopes and all specification/external/historical dependencies. Continue independent POV/rotation research. Defer runtime verification until the full implementation pass is complete.

No runtime/GUI/mission/numerical tests, new tests, acceptance auditors/readers/seals, evidence admission, fixture/assertion/tolerance/denominator/gate changes, packages, installed/shared runtime changes, commits, pushes or external messages were performed in this batch.

Tracking-v99 remains 1093/1158 (94.39%), with 65 unresolved rows. The recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full implementation audit incomplete; goal active.
