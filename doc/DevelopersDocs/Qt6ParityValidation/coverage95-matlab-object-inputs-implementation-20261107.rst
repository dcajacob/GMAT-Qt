Scoped typed native MATLAB input export
=======================================

Whole-object MATLAB input previously evaluated native MATLAB_STRUCT text directly into the logical workspace binding. The common serializer could suppress declarations created outside the main script; string/list values could enter MATLAB source without consistent escaping; nested owned serializers could switch back to script mode; and fields absent on a later call could survive from a previous structure. This batch retains the native field/prefix decisions while staging supported values outside that source text and publishing a completed fresh structure.

Scoped native field capture
---------------------------

ObjectMatlabExportScope is a new exported nonvirtual stack type implemented in GmatBase.cpp. A thread-local stack restores the previous request on normal or exceptional exit. The request supplies native-property and computed-real callbacks; callbacks capture owned values without calling MATLAB during native serialization. GmatBase WriteParameterValue and WriteStringArrayValue can emit references to those captured values. The latter includes empty native lists instead of leaving a previous binding's field behind. Writable known defaults participate through native default comparison; unrelated hidden/read-only parameter policy remains in the native writers.

Explicit MATLAB/owned data modes bypass the common main-script creation filter and whole-object cloak. Ordinary script/Save behavior outside this request retains its source/default filtering. ObjectMatlabModeScope lets common, spacecraft, attitude and array serializers apply the enclosing MATLAB data mode to owned objects and restore their previous mode flags on return/error. Native data export suppresses common comments/scripted methods, plus the affected spacecraft/fmincon comment paths. Temporary numeric formatting uses the classic locale and round-trip precision where native class-specific formatting remains.

Spacecraft's normal GetGeneratingString performs a discarded parameter-writing prepass before its final base call. This explicit export skips that prepass, retaining the final native virtual writer and avoiding duplicate property capture. The six representation-state elements are intercepted from the already-computed repState values; no orbit conversion or state representation formula is replaced. Display-state/anomaly/model names use native property capture. Common and spacecraft string/reference lists route through the typed cell capture, and fmincon's custom lists participate while retaining its existing selected-option serialization.

Typed input construction
------------------------

MatlabInterface PutValue constructs owned dense real/logical arrays, character values, cells and scalar structures through the installed C Engine/Matrix API. Numeric shape/data capacity checks precede construction, column-major ordering is retained, and logical values validate zero/one. Strings decode the complete UTF-8 byte range to UTF-16, preserving quotes, control characters, non-ASCII text and embedded NUL without inserting the value into MATLAB source. Invalid/incomplete UTF-8 rejects explicitly. Character values are canonical rows; empty characters retain native empty-literal dimensions. Partial child allocations remain owned by the enclosing array or native RAII owner until transfer succeeds. Engine transfer failure remains a typed transport exception.

Supported native property captures include real/time, native integers, logicals, strings/enums/files/object names/equations/colors/On-Off, vectors/matrices, integer/Boolean/string/reference lists, precise MJD text and the six existing Generic subtypes. Matrix and vector values are copied from native getters. Native object/reference naming conventions remain; this is not a new object-reference reconstruction protocol. Unsupported property/class-specific serialization families retain their native route and remain part of the open scope.

CallMatlabFunction first captures its native object export into owned values and source references, transfers the value cell to a reserved workspace slot, constructs a fresh scalar structure in another slot and publishes it to the logical input only after construction succeeds. An empty native serialization reports an error instead of reusing a stale input. Object lifetime is checked across serialization. The existing spacecraft epoch-refresh action remains. String Variables and string-returning Parameters also use typed character input, preserving their existing value selection/evaluation behavior. No native string literal parsing/unescaping rule is changed.

Temporary names and publication boundaries
-----------------------------------------

The new CreateTemporaryVariable overload accepts excluded declarations before reserving a workspace name. Result slots, legacy protected output capture and the new input helpers exclude declared input/output expressions and their root bindings, plus the called function name. Existing workspace variables continue to be skipped. The old no-argument method remains and delegates to the same reservation path.

Temporary cleanup preserves the existing exception behavior. A failed export, typed transfer or scratch construction cannot publish a partially constructed logical object input. This does not promise atomic workspace rollback after successful publication or exactly-once external MATLAB side effects. Existing bounded transport retry and whole-call output staging remain intact. Output list replacement/resizing, reference-map rebinding, arbitrary component replacement, external object lifetime/reentry, complete custom serializer coverage and the original FRR-36 definition remain open.

Compilation and retained status
-------------------------------

Configured compilation v1: 75/75 steps, 8.16 seconds, return 0.
Configured compilation v2: 6/6 steps, 2.69 seconds, return 0.
Configured compilation v3: 71/71 steps, 7.8 seconds, return 0.
Configured compilation v4: 66/66 steps, 7.37 seconds, return 0.
Legacy Mac conditional syntax: 0.4 seconds, return 0, with the configured Linux compiler/R2026b SDK.

The conditional compilation does not qualify a macOS toolchain/runtime. The UTF conversion facility produces the existing standard-library wstring_convert deprecation diagnostics, now also at the input decoder. Eleven source/help paths match final configured v4 and the final conditional snapshot. Ten preimages and an absent new-header marker precede first edits. Before/build/final hashes, indexed deltas, source map, logs and authoritative terminal compiler results remain in the stage. Added-line whitespace is clean and the help XML parses. All compiler sessions are terminal.

The new scope/value helpers are nonvirtual. Existing instance layouts, public method signatures and virtual slots remain intact; overloads/new methods were added and matching configured consumers rebuilt. External binary/plugin compatibility and Windows/macOS runtime behavior remain unqualified.

No runtime/GUI/mission/numerical tests, acceptance audits/readers/seals, evidence admission, assertions/fixtures/tolerances/denominator edits, packages, installed/shared runtime changes, commits, pushes or external messages were performed. Main-script/function-formal restrictions retain their existing rules. No original all/any parent earns partial credit. Tracking-v76 remains 1093/1158 (94.39%), with 65 other rows. The recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full goal active and incomplete.
