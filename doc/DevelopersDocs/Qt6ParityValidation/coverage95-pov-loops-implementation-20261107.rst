POV loop replay, scoped counters and break handling
==================================================

The private reader now executes for and while loops using source bookmarks. This extends the conditional foundation to generated static geometry/data while preserving actual source order and declaration scopes. No loop or model was run during this implementation phase.

Source and scope
----------------

A bookmark retains file frames, cursors, line positions and pending tokens. Scope pointers remain live, so declarations made during a body pass persist under their normal file/global scope rules. Replay restores read position without rolling back symbol values or changing the include evaluator's caller-owned bounded mode. Includes encountered during a new body pass follow the normal source path; skipped bodies remain raw tokens with no include or declaration effects.

Conditional and loop frames now share one private control stack, role and non-reused identity scheme. Matching end markers close the actual top role. Loop headers retain the existing enclosing-frame validation across expression evaluation. The combined stack retains the 200-level limit. Public reader signatures are unchanged.

Native loop contracts
---------------------

For introduces or replaces a scalar in the current scope, shadowing an outer name. The native zero-valued local binding exists before the start expression is read; the start is then stored before reading bound and step. Bound and authored/default step are sampled once. Iteration-end lookup reads the current same-name local scalar, allowing body changes as the native parser does. The counter is advanced before the next range test and its resulting binding remains after normal loop exit. The step defaults to one and follows the native minimum-magnitude rule.

While replays its header and evaluates it after each body pass. The inspected owning source uses slightly different strict/inclusive default-epsilon comparisons on initial entry and repetition for both loop families. Those native comparisons are retained explicitly. A zero-iteration or completed loop scans the body raw to the matching end. A running loop must end in its defining file. Missing/nonscalar/nonfinite counters and a step that cannot advance a continuing for loop fail with source locations.

Break targets the nearest running loop, retains its current counter, drops intervening conditional frames and raw-skips their remaining endings before closing the loop. Break outside a loop fails. Macro and switch break targets remain unimplemented and are not silently treated as loop targets.

Remaining runtime and language scope
------------------------------------

General while-loop termination and caller interruption/cancellation remain unqualified; no arbitrary iteration limit is substituted for the language. Macros, switch/case/range control, remaining native value/function/array variants, richer geometry/CSG and advanced textures remain open. These source loops do not establish full POV-Ray, Blender/Rhino or NASA model compatibility.

Primary references: https://www.povray.org/documentation/view/3.7.0/241 and https://wiki.povray.org/content/Reference%3AConditional_Directives. Actual for-counter, bound/step, replay/file and epsilon rules were inspected in upstream parser_tokenizer.cpp; its URL and blob identity remain in research-note.json. The inspected source identifies itself as 3.8, so complete version/platform conformance is not inferred.

Array/type/copy/symbol structures, raw decoding, typed array readers, numeric/string/logical expression code, geometry/material/transform methods, material/GLSL/Builder output and legacy loader are byte-identical to preimages. Mission dynamics are unaffected.

Compilation and retained evidence
---------------------------------

Configured build v1 returned 0 in 10.28 seconds; last printed Ninja progress 6/6. Session 48372 terminated with runner/compiler return 0. One Qt QImage::mirrored deprecation warning from OrbitRenderer.cpp is retained; no compiler errors.

Three existing source/help preimages, one unchanged legacy reference and four planning snapshots preceded edits. Indexed deltas, before/build/final hashes, source map, source/research notes, compiler log and terminal receipt remain in the stage directory. Final sources match the successful build capture, added-line whitespace is clean and visualization help XML parses. Other-platform/toolchain/runtime behavior remains unqualified.

No model load, directive/expression/loop evaluation, shader compilation, rendering, runtime/GUI/MATLAB/helper/mission/numerical tests, new tests, acceptance auditors/readers/seals or evidence admission occurred. Static comparisons and compilation are implementation feedback only. Original requirements, fingerprints, source text, assertions, fixtures, tolerances, denominator, gates and dispositions remain unchanged. FRR!E44/E46 remain partial. Tracking-v113 remains 1093/1158 (94.39%), 65 unresolved engineering rows; recorded source-versioned verified baseline 1053/1158 (90.9%), all 105 original acceptance rows open. The full implementation audit and goal are incomplete.

Next implementation
-------------------

Implement remaining original POV macros and switch/case/range control with their proper break targets, and complete caller interruption/cancellation without silently narrowing loop semantics. Continue directive-generated array headers/initializers, remaining function/string/color/array/dictionary variants, richer geometry/CSG/advanced textures and representative exporter research. Continue the full original function/solver/resource/persistence/body/frame/kernel/ephemeris worklist and explicit specification/external/historical dependencies. Preserve all original requirements; runtime verification follows the complete implementation pass.
