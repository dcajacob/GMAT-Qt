POV array values, scope and indexed consumers
============================================

The static reader previously rejected arrays. Exported/static scene declarations can use arrays for coordinates, textures, transforms, object instances and include filenames. The implementation adds typed array values to the private parser symbol model and resolves indexed reads through the existing typed consumers.

Container and value contracts
-----------------------------

Fixed arrays support one through five positive dimensions, overflow-checked shape products, dimension-matched nested initializers, individual element assignments and sparse uninitialized storage. Array integer conversion applies the upstream default small boundary allowance before truncation. Rank, range, negative and uninitialized accesses fail with source locations. Nested array values can be indexed through successive complete ranks.

The first initialized value establishes an array's homogeneous element type. Numeric tuple widths and texture/pigment/finish kinds remain distinct. Strings, transforms, static objects and nested arrays retain their existing typed value families. Undefining an element removes its value while retaining the container's type. Whole-array assignment recursively copies nested containers and initialized values, preserving holes and preventing a later element edit from changing the copied array. Static model prototypes remain immutable and can retain shared geometry as existing instances do.

Whole-array declarations follow the existing local/global identifier rules. A local element edit cannot silently create an independent local element of an outer array. Element undefinition and indexed assignment use the resolved actual container. Declaration queries distinguish missing elements from empty values; array queries return rank and one-based dimension sizes. Unsupported value families and syntax still fail the import.

Typed reads and source boundaries
---------------------------------

An indexed read becomes a private typed token view, so scalar/vector expressions, strings and include filenames, object instances, materials and transforms use their existing consumers. The complete closing subscript is retained as the terminal token until the consumer advances. Raw lookahead restores lexer file/cursor/scope state, does not expand a following include, preserves include-filename EOF bounds and distinguishes actual symbols from same-text string contents. New declarations and element values commit before reading a following include that may use them.

The reader still lacks general directive-generated array headers/initializers, dynamic/mixed/dictionary variants, macros/conditional/loop semantics, remaining native value families and advanced geometry/material features. These containers do not establish full current-version POV-Ray or exporter compatibility. No silently skipped geometry is introduced.

Primary reference and compatibility
-----------------------------------

Classic array semantics: https://www.povray.org/documentation/view/3.70/233/
and version distinctions: https://wiki.povray.org/content/Reference%3AArray.
The upstream parser, tokenizer, symbol-table and core-configuration sources were reviewed for copy/type/scope/index/undefinition behavior; URLs and inspected blob hashes are retained in research-note.json. The upstream source identifies itself as 3.8, so no complete 3.7/3.8 conformance claim follows. The reader uses the upstream default epsilon value; platform-specific overrides are not imported.

Raw lexical decoding, existing numeric/vector function implementations after metadata dispatch, material blocks, transforms, object modifiers, material batching/GLSL and Builder handedness/winding sections are byte-identical to their preimages. The retained legacy loader is unchanged. Public reader signatures are unchanged; new symbol/container state is private to the parser. Mission dynamics are unaffected.

Compilation and retained evidence
---------------------------------

Configured build v1 returned 0 in 10.32 seconds; last printed Ninja progress 6/6. Session 49965 terminated with runner/compiler return 0. One Qt QImage::mirrored deprecation warning from OrbitRenderer.cpp is retained; no compiler errors.

Three existing source/help preimages, one unchanged legacy reference and four planning snapshots preceded edits. Indexed deltas, before/build/final hashes, source map, source/research notes, compiler log and terminal receipt remain in the stage directory. Final sources match the successful build capture, added-line whitespace is clean and visualization help XML parses. Other-platform/toolchain/runtime behavior remains unqualified.

No model load, expression evaluation, shader compilation, rendering, runtime/GUI/MATLAB/helper/mission/numerical tests, new tests, acceptance auditors/readers/seals or evidence admission occurred. Static comparisons and compilation are implementation feedback only. Original requirements, fingerprints, source text, assertions, fixtures, tolerances, denominator, gates and dispositions remain unchanged. FRR!E44/E46 remain partial. Tracking-v110 remains 1093/1158 (94.39%), 65 unresolved engineering rows; recorded source-versioned verified baseline 1053/1158 (90.9%), all 105 original acceptance rows open. The full implementation audit and goal are incomplete.

Next implementation
-------------------

Continue remaining original POV control/macros/conditions, directive-generated array headers/initializers, dynamic/mixed/dictionary variants, richer geometry/CSG and advanced textures using primary references and representative exporter sources. Static array containers are not full POV-Ray scene compatibility. Continue the complete original function/solver/resource/persistence/body/frame/kernel/ephemeris implementation worklist and explicit specification/external/historical dependencies. Runtime verification follows the full implementation pass.
