POV macro source execution and argument ownership
================================================

The Qt POV reader now captures macro source without executing its body, creates global replaceable definitions, and invokes source fragments with caller-parented local scopes. Arguments are evaluated before formal bindings exist. Stable value cells retain references to caller identifiers and array elements; expressions and values emitted from shorter-lived nested macro locals are copied. Array-element aliases retain homogeneous-type checks, named undef removes a binding, and indexed undef clears its actual slot. Optional omitted/uninitialized arguments add no local entry, preserving outer lookup. Natural completion and nearest-macro break restore caller source positions; call-header boundaries protect final delimiters. Typed array tokens retain reference provenance, and include filenames can use completed macro/string/array expressions while retaining their actual source context. Three source/help paths compile; visualization help XML parses. Geometry/math/material/transform/GLSL/Builder and legacy loader remain unchanged. Cancellation, remaining language/array/dictionary/function/geometry/texture and full exporter/version compatibility remain open. No runtime/model/rendering verification or row credit.

Scope and value lifetime
-----------------------

Macro definitions retain their source text, physical file and initial line, plus parameter names. They capture no symbol table, so global definitions created by another call do not retain its locals. Calls evaluate positional arguments in the caller before creating a caller-parented local table. Definitions can be replaced or undefined; active source fragments retain their own text. Parentheses remain required even with no parameters. Nested control blocks must balance within a definition, and inactive definitions/includes are scanned without execution.

Symbol and array entries now own shared value cells. Bare initialized caller identifiers and slots alias those cells; expression arguments create owned copies. Array copies recursively create independent cells while immutable geometry prototypes remain shared. Aliases to array elements retain the array solely in the formal binding for homogeneous-type checks; actual array entries have no back-link. Named undef removes the selected name; indexed undef clears its retained slot. A later same-name declaration creates a distinct binding. A nested macro local emitted as an argument is copied when its table is outside the caller context, matching the inspected native context comparison. Typed array reads retain value snapshots and separate lvalue provenance.

Optional parameters with missing or uninitialized actuals add no local binding, allowing outer same-name lookup. Required missing values and excess/premature positional lists report source errors. The local pseudo-dictionary remains part of the open dictionary scope. Call-header boundary guards retain a final closing delimiter in its caller until the new invocation has entered. Completed source fragments restore the caller before following delimiters and filename context checks. More general interleaved macro/directive filename lifecycles remain unqualified.

Natural completion and break
----------------------------

A private macro-source boundary distinguishes return from included-file EOF. Natural completion requires that nested controls have closed. Break chooses the nearest actual loop, switch or invoking macro; a macro return discards intervening conditional frames and restores the caller bookmark. Loop/switch breaks retain their own existing behavior. Public reader signatures are unchanged; new state is private to each parse. Caller interruption/cancellation remains open, including genuinely nonterminating source loops. No arbitrary iteration cutoff was added.

Primary semantics were inspected in the POV-Ray macro documentation and upstream tokenizer/parser/symbol-table sources; URLs and blob identities remain in research-note.json. No upstream code was copied. The inspected source is 3.8 and this packet does not establish a complete POV dialect/profile or exporter contract.

Compilation and retained evidence
---------------------------------

Configured build v1 returned 0 in 10.3 seconds; last printed progress 6/6. One existing Qt QImage::mirrored deprecation warning from OrbitRenderer.cpp; no compiler errors.
Configured build v2 returned 0 in 9.31 seconds; last printed progress 5/5. No compiler diagnostics.
Configured build v3 returned 0 in 8.99 seconds; last printed progress 5/5. No compiler diagnostics.

Three source/help preimages, one unchanged legacy reference and four planning snapshots preceded edits. Indexed patches, before/build/final hashes, source/research notes, compiler logs and terminal session records remain in the stage. Final source hashes match v3, changed-line whitespace is clean, and visualization help XML parses. Math/geometry/material/transform and material/GLSL/Builder sections compare byte-identical with preimages. Other-platform/toolchain/runtime behavior remains unqualified.

No model import, macro/argument/control evaluation, shader compilation, rendering, runtime/GUI/MATLAB/helper/mission/numerical tests, new tests, acceptance auditors/readers/seals or evidence admission occurred. Original requirements, fingerprints, source text, assertions, fixtures, tolerances, denominator, gates and dispositions remain unchanged. FRR!E44/E46 remain partial. Tracking-v115 retains 1093/1158 (94.39%), 65 unresolved engineering rows; recorded source-versioned verified baseline 1053/1158 (90.9%), all 105 original acceptance rows open. The full implementation audit and goal remain incomplete.

Next implementation
-------------------

Complete POV caller interruption/cancellation, remaining directive-generated array headers/initializers, native function/string/color/array/dictionary variants, richer geometry/CSG and advanced textures, and research representative exporters. Macro-produced filename fragments must finish in the including source context; more general interleaved macro/directive filename lifecycles remain unqualified. Continue the complete original function/solver/resource/persistence/body/frame/kernel/ephemeris worklist and explicit specification/external/historical dependencies. Preserve original clauses and defer runtime verification until the implementation pass is complete.
