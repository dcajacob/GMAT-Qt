POV logical, relational and conditional expressions
==================================================

Static POV declarations could use arithmetic and functions but could not express comparisons, boolean combinations or conditional values. Native owning-source review established an important evaluation rule: the scene parser evaluates both logical operands, both ternary arms and every select argument before choosing a result. This implementation follows that rule.

Expression semantics
--------------------

Parentheses now invoke the full expression hierarchy while ordinary arithmetic/vector field parsing retains its existing boundary. Relational operators share precedence and associate left-to-right; logical AND/OR likewise share precedence. Conditional selection follows them and recurses for nested selections. Unary logical not works component-wise. Two-character comparison operators are lexed as symbols while quoted string contents retain their token kind.

Scalar/vector comparisons and logical operations preserve scalar broadcast and shorter-vector zero extension. Truth requires absolute value strictly greater than the shared default epsilon. Equality is the complement of that difference test; inclusive comparisons include near-equality. A ternary predicate must be scalar, both arms evaluate, and the selected value retains its own width. Three/four-argument select evaluates and validates every scalar argument and uses its native sign/zero selection rule.

String expressions support relational comparison plus strcmp and strlen. Comparisons use case-sensitive UTF-16 code-unit order and string termination consistent with the native UCS2 boundary; length counts those code units. Existing string construction/filename paths retain their prior behavior. Full Unicode/string-function compatibility remains unqualified.

Optional boolean model modifiers recognize parenthesized/unary/function expressions as well as existing scalar literals/identifiers. The source review also found that native pow rejects zero raised to zero, while std::pow can return one. The existing math route now reports that native domain error. The named epsilon constant replaces the identical local array conversion value without changing its numeric boundary.

Primary evidence and retained boundaries
---------------------------------------

POV-Ray references: https://www.povray.org/documentation/view/3.7.0/228/
and https://www.povray.org/documentation/view/3.7.0/229/.
Actual evaluation, boundary and domain rules were inspected in upstream parser_expressions.cpp; its URL and blob hash, plus the default-epsilon configuration source, are retained in research-note.json. Legacy documentation URLs display a 3.6 heading and the inspected source identifies itself as 3.8. No complete version/profile conformance claim follows.

Array container/type/copy/symbol structures, lexer include/lookahead handling, typed array read and initializer routes, material blocks, object transforms, geometry parsers, material batching/GLSL and Builder handedness/winding sections are byte-identical to their preimages. The retained legacy loader is unchanged. Public reader signatures are unchanged; all new expression machinery is private. Mission dynamics are unaffected.

Directive control/macros, remaining functions and string/color semantics, array variants, directive-generated headers, richer geometry/CSG and advanced textures remain open. Expression support is a foundation for control flow rather than a claim that control directives already work.

Compilation and retained evidence
---------------------------------

Configured build v1 returned 0 in 10.36 seconds; last printed Ninja progress 6/6. Session 24848 terminated with runner/compiler return 0. One Qt QImage::mirrored deprecation warning from OrbitRenderer.cpp is retained; no compiler errors.

Three existing source/help preimages, one unchanged legacy reference and four planning snapshots preceded edits. Indexed deltas, before/build/final hashes, source map, source/research notes, compiler log and terminal receipt remain in the stage directory. Final sources match the successful build capture, added-line whitespace is clean and visualization help XML parses. Other-platform/toolchain/runtime behavior remains unqualified.

No model load, expression evaluation, shader compilation, rendering, runtime/GUI/MATLAB/helper/mission/numerical tests, new tests, acceptance auditors/readers/seals or evidence admission occurred. Static comparisons and compilation are implementation feedback only. Original requirements, fingerprints, source text, assertions, fixtures, tolerances, denominator, gates and dispositions remain unchanged. FRR!E44/E46 remain partial. Tracking-v111 remains 1093/1158 (94.39%), 65 unresolved engineering rows; recorded source-versioned verified baseline 1053/1158 (90.9%), all 105 original acceptance rows open. The full implementation audit and goal remain incomplete.

Next implementation
-------------------

Implement remaining original POV directive control and macro semantics using the native scene-parser evaluation rules, then remaining functions/color/string/array variants, directive-generated headers/initializers, richer geometry/CSG and advanced textures. Continue representative exporter research and the full original function/solver/resource/persistence/body/frame/kernel/ephemeris implementation worklist and explicit specification/external/historical dependencies. Keep all original clauses and defer runtime verification until the implementation pass is complete.
