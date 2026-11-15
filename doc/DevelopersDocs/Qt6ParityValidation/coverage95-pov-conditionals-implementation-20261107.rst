POV conditional selection and inactive source regions
====================================================

Conditional parsing is distinct from the eager numeric conditional operator. An inactive directive branch must leave its includes unopened, declarations unchanged and expressions/messages unevaluated. The previous directive reader advanced beyond a keyword before deciding what it represented; that could expand an include immediately after a branch marker. The new control path decides before that read.

Selection and skipped tokens
-----------------------------

The private parser tracks if/ifdef/ifndef chains and selects elseif/else alternatives using scalar default-epsilon truth or actual identifier/array-element presence. The predicate closing token and branch keyword remain terminal until selection. Selected paths resume normal typed token/include handling. Inactive paths scan raw tokens and stop at the current chain's next alternative or end, preserving strings/comments and balancing nested conditional, loop, switch and macro block openers without executing them.

Conditional nesting is bounded at the documented 200 levels. Current-chain duplicate/after-else alternatives, unmatched markers and unterminated source report their original locations. Skipped nested contents are lexically balanced rather than fully interpreted. Non-reused control identities and captured frame flags also detect predicates that replace or mutate their own enclosing chain, while balanced nested predicate directives can return to the retained context.

Skipped branch keywords are queued for ordinary directive processing. Queued tokens precede bounded/raw reading so include-filename evaluation retains its existing EOF boundary. The directive reader now rejects unsupported active directives before reading their payload; inactive unsupported blocks can be skipped as source text when properly balanced. No active loop, switch or macro execution is implemented by this packet.

Messages and value consumers
---------------------------

Active error directives abort import with the authored string and source location. Warnings use the viewer's Qt warning log; debug/render/statistics messages use its information log. Supported string identifiers/functions and indexed string values use existing typed readers. Inactive messages produce no output. These are viewer diagnostic routes, not a native POV-Ray renderer or its configurable text-stream system.

Primary conditional/language references are https://wiki.povray.org/content/Reference%3AConditional_Directives and https://wiki.povray.org/content/Reference%3ALanguage_Directives. Historical official message grammar and actual upstream skipped-parser states were also inspected; URLs and source blob identity remain in research-note.json. The version boundary remains explicit and full current-version POV-Ray conformance is not claimed.

Array/type/copy/symbol structures, raw decoding, array read/initializer handlers, numeric/string/logical expression code, material blocks, transforms, geometry parsers, material/GLSL/Builder output and legacy loader source are byte-identical to their preimages. Public reader signatures are unchanged; control state is private to each parser. Mission dynamics are unaffected.

Compilation and retained evidence
---------------------------------

Configured build v1 returned 0 in 9.95 seconds; last printed Ninja progress 6/6. Session 50060 terminated with runner/compiler return 0. One Qt QImage::mirrored deprecation warning from OrbitRenderer.cpp is retained; no compiler errors.

Three existing source/help preimages, one unchanged legacy reference and four planning snapshots preceded edits. Indexed deltas, before/build/final hashes, source map, source/research notes, compiler log and terminal receipt remain in the stage directory. Final sources match the successful build capture, added-line whitespace is clean and visualization help XML parses. Other-platform/toolchain/runtime behavior remains unqualified.

No model load, directive/expression evaluation, shader compilation, rendering, runtime/GUI/MATLAB/helper/mission/numerical tests, new tests, acceptance auditors/readers/seals or evidence admission occurred. Static comparisons and compilation are implementation feedback only. Original requirements, fingerprints, source text, assertions, fixtures, tolerances, denominator, gates and dispositions remain unchanged. FRR!E44/E46 remain partial. Tracking-v112 remains 1093/1158 (94.39%), 65 unresolved engineering rows; recorded source-versioned verified baseline 1053/1158 (90.9%), all 105 original acceptance rows open. The full implementation audit and goal are incomplete.

Next implementation
-------------------

Implement remaining original POV loop/macro/switch execution and break semantics, then directive-generated array headers/initializers, remaining function/string/color/array/dictionary variants and richer geometry/CSG/advanced textures. Continue representative exporter research and the complete original function/solver/resource/persistence/body/frame/kernel/ephemeris worklist and explicit specification/external/historical dependencies. Preserve all original requirements and defer runtime verification until implementation is complete.
