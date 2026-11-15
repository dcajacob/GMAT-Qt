MATLAB return-value capture and native argument storage
=======================================================

The prior MATLAB call assigned results directly to workspace destinations and read them back one at a time. Repeated destinations could erase distinct return values, a later malformed output could leave earlier GMAT destinations changed, output indices could be re-evaluated after another output changed their inputs, and some system Parameters still depended on parsing display output. This batch separates return slots, typed capture and native assignment.

Return slots and frozen targets
------------------------------

A shared invocation formatter preserves zero-input/zero-output syntax and bracketed positive outputs. The public FormEvalString still describes the configured logical call. Execution resolves input and output array indices once before input transfer, then directs MATLAB returns to separate reserved variables. Each name includes the native process ID and an atomic serial; an existing base-workspace variable is skipped. Reservation uses the actual C Engine and an owned empty mxArray. No user variable is deliberately overwritten to obtain scratch space.

Independent return slots preserve distinct return positions even when GMAT destinations repeat or overlap. Each array-element result is read as its scalar return rather than fetching a partly populated MATLAB matrix and inferring the selected element. Whole-array results require exact configured dimensions; only the selected element is changed for an element destination.

ReadOutParams captures all supported Parameter outputs into independent C++ values and assignment closures before invoking setters. Real/string system Parameters use their actual setters and typed values. Vectors accept the configured element count in row or column form; matrices retain configured dimensions and column-major-to-row-major conversion. Native user Variables, Strings and Arrays keep their writable setters even though their system-Parameter IsSettable flag defaults to false; other Parameters must be settable. Unsupported values, wrong dimensions, and read failures propagate before GMAT assignment.

After capture/validation, execution publishes results back to the original MATLAB workspace destinations to preserve that existing behavior, using the frozen indices. Temporary cleanup completes before native assignment. During exceptional unwinding cleanup is best effort and does not replace the primary exception. The old protected GetOutParams entry point remains and captures its current workspace expressions into separate slots before reading/assigning them.

This is a staged read, not an atomic multi-object transaction. A native setter can still reject a domain value after earlier setters have run, and workspace publication can have MATLAB side effects before an engine failure. The existing single transport retry remains bounded at the command boundary; exactly-once external side effects are not promised. Broader transaction, callback/reentry and lifetime scopes remain open.

Input values and numeric transfer
--------------------------------

Real-valued inputs now evaluate once and serialize with the classic locale and max_digits10, including explicit NaN/Inf spellings. The former generic EvaluateReal-plus-ToString path could evaluate a Parameter twice. Native numeric vector and matrix inputs are emitted from one evaluated value using the same scalar encoding. String-valued inputs retain the prior common character-literal encoder. Logical declaration names, rather than incidental instance names, identify the MATLAB input bindings.

PutRealArray directly rearranges native row-major data into the C Engine's column-major allocation. It checks dimensions, null input data, allocation/data access and engPutVariable results. Transport failure is a typed connection exception rather than unconditional success. GetRealArray owns every returned mxArray, checks rows/columns/count before narrowing to native Integer, copies only the caller capacity, preserves the full received count for validation, and retains existing dense-real-double/logical-scalar acceptance. GetString also rejects a missing engine before calling the C API. No orbit, force, covariance, propagation or solver formula changes.

Actual argument types and compatibility boundaries
--------------------------------------------------

CallFunction stores GmatBase pointers in its two argument lists. Whole objects no longer masquerade as Parameter objects in common formatting/reference enumeration. Parameter-typed SetRefObject still validates actual Parameters as before. The MATLAB sender keeps its existing Parameter overload and adds a GmatBase overload, retaining actual native whole-object input serialization with the logical argument name. Existing spacecraft epoch refresh behavior remains.

The Python interface retains its Parameter-only accepted inputs/outputs and now checks the actual native type at use. Its list population stores the actual base pointer after the existing input/output validation. No Python data family or wire format changes.

The pre-existing native whole-object output route remains separate: it captures a bounded formatted scalar structure into owned storage and defers the legacy importer until assignment. A non-structure result now fails before that importer. This does not implement arbitrary nested typed structures, full member/reference ownership, large object output, or lossless generic property transfer. Legacy importer field-warning behavior and limitations remain visible. Original formal whole-object output restrictions and main-script whole-object/literal parsing restrictions were not relaxed. No new FRR-36 definition is invented.

Compilation and remaining scope
-------------------------------
Configured compilation v1: 11/11 steps, 3.57 seconds, return 0.
Configured compilation v2: 80/80 steps, 293.99 seconds, return 0.
Legacy Mac conditional syntax compilation: 0.71 seconds, return 0, using the Linux compiler/R2026b SDK. This is not macOS platform qualification.

Seven paths match the final successful configured compilation and conditional-syntax source snapshots. Four plugin preimages preceded v1; the three common/Python paths were captured before their first edits and v2. Before/build/final hashes, indexed deltas, source map, logs and compiler terminal results remain in the stage. Added-line whitespace is clean. The existing standard-library wstring_convert deprecation warning remains; it is not a runtime verification result.

No virtual slots or new instance fields were added. The common argument-list element type changed, and nonvirtual helpers/overloads were added. Matching configured consumers rebuilt; external prebuilt binary compatibility and Windows/macOS toolchain/runtime behavior remain unqualified.

No runtime/GUI tests, missions, numerical matrices, acceptance audits/readers, evidence admission, assertion/fixture/tolerance changes, denominator changes, packages, installed/shared runtime changes, commits, pushes or external messages were performed. Full original function/resource/command/scientific scope, MATLAB whole-object/literal/property and reverse-call contracts, unresolved specification references and external/historical platforms remain open. Partial fixes earn no all/any parent credit.

Tracking-v70 remains 1093/1158 (94.39%), with 65 other rows. The recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full goal active and incomplete.
