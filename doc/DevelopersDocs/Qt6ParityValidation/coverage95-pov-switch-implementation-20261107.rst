POV switch selection, clause flow and break targets
==================================================

The private control parser now supports switch/case/range in addition to conditional and loop directives. The selector is read once. A case uses the native strict default-epsilon difference test; a range uses exact inclusive lower/upper comparisons. An inverted range naturally does not match.

Clause execution
----------------

The first ordinary switch clause must be a case or range at the actual switch control level. Intervening directives can be processed, but geometry before the first clause or a case/range under another current control reports an error. Every label expression on a selected chain is still parsed. Once selected, the chain remains active across later clauses until break/end. Default else activates an unmatched chain and retains an already active one, following the inspected owning source.

The lexer now names its three raw skip destinations: conditional alternatives, switch labels/default and control end. Unmatched clauses scan raw tokens without evaluating their includes, declarations, messages or expressions. Nested native blocks remain balanced under the existing combined 200-level control limit. Skipped nested contents are lexically balanced rather than executed.

Break finds the nearest actual switch or loop, drops only intervening conditional frames and skips to that target's end. A switch end closes its switch role rather than replaying a loop. Loop bodies and counters retain the previous implementation; their handlers only replace equivalent boolean skip arguments with the explicit End mode. Nested switches and loops therefore have distinct break targets. Macro return/break targets remain open.

Ownership and primary evidence
------------------------------

Switch header/label evaluation checks the retained control identity, role and relevant state before publication. The captured selector is private and remains fixed even when its source variable changes in the body. Public reader signatures are unchanged; all new control state is private to each parser.

Primary reference: https://www.povray.org/documentation/view/3.7.0/241. Actual captured-selector, first-label, case/default/fallthrough and break behavior was inspected in upstream parser_tokenizer.cpp; its URL and blob identity remain in research-note.json. The source identifies itself as 3.8. The implementation follows those owning states without claiming full historical/current-version or exporter compatibility.

Array/type/copy/symbol structures, raw decoding, typed array readers, numeric/string/logical expression code, geometry/material/transform methods, material/GLSL/Builder output and legacy loader are byte-identical to preimages. For/while handler comparison normalizes only the equivalent named skip mode. Mission dynamics are unaffected.

Macros, caller interruption/cancellation, remaining native value/function/array variants, richer geometry/CSG, advanced textures and full original POV model support remain open.

Compilation and retained evidence
---------------------------------

Configured build v1 returned 0 in 9.93 seconds; last printed Ninja progress 6/6. Session 99361 terminated with runner/compiler return 0. One Qt QImage::mirrored deprecation warning from OrbitRenderer.cpp is retained; no compiler errors.

Three existing source/help preimages, one unchanged legacy reference and four planning snapshots preceded edits. Indexed deltas, before/build/final hashes, source map, source/research notes, compiler log and terminal receipt remain in the stage directory. Final sources match the successful build capture, added-line whitespace is clean and visualization help XML parses. Other-platform/toolchain/runtime behavior remains unqualified.

No model load, directive/expression/loop/clause evaluation, shader compilation, rendering, runtime/GUI/MATLAB/helper/mission/numerical tests, new tests, acceptance auditors/readers/seals or evidence admission occurred. Static comparisons and compilation are implementation feedback only. Original requirements, fingerprints, source text, assertions, fixtures, tolerances, denominator, gates and dispositions remain unchanged. FRR!E44/E46 remain partial. Tracking-v114 remains 1093/1158 (94.39%), 65 unresolved engineering rows; recorded source-versioned verified baseline 1053/1158 (90.9%), all 105 original acceptance rows open. The full implementation audit and goal are incomplete.

Next implementation
-------------------

Implement remaining original POV macros with native argument, scope, value/lifetime and break-return behavior, and complete caller interruption/cancellation. Continue directive-generated array headers/initializers, remaining function/string/color/array/dictionary variants, richer geometry/CSG/advanced textures and representative exporter research. Continue the complete original function/solver/resource/persistence/body/frame/kernel/ephemeris worklist and explicit specification/external/historical dependencies. Preserve all original clauses; runtime verification follows the full implementation pass.
