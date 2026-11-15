POV static numeric functions and mesh index expressions
======================================================

The Qt parser supported arithmetic on static scalars/vectors but rejected numeric function calls and component selection. Static mesh/primitive locations, dimensions, normals and transforms can therefore fail even though their values are determined while parsing. The mesh2 parser also guessed whether a comma introduced a texture index or a following face from only its first token, which does not identify the result type of a function or parenthesized expression.

Numeric implementation
----------------------

The parser now implements 30 scalar-returning functions covering ordinary/inverse trigonometric and hyperbolic math, absolute value, rounding, logs/exponentials/powers, variadic minimum/maximum, integer division/remainder, unit conversion and vector dot product/length. Four vector-returning functions implement cross product, normalization, Euler rotation and axis rotation. Function parsing retains normal expression precedence and nesting accounting. Argument count and scalar/three-component-vector contracts are checked. Zero divisors/directions and nonfinite/domain results fail at the function's source location.

Vector arguments follow the established scalar broadcast and shorter-vector zero-extension convention. Normalization scales the vector before computing its length. Rotations reuse the existing X/Y/Z axis order and matrix convention before the existing final Y/Z display exchange. The standard t/u/v and yes/no constants join the existing built-ins. Postfix vector/UV/color component aliases are available on literals, identifiers, parenthesized expressions and function results. Unsupported component names still fail explicitly.

Mesh2 expression classification
--------------------------------

The face-index path now parses an ambiguous following expression once, classifies a scalar as a texture selector or a three-component tuple as a pending next face, then retains that pending face for the next iteration. This supports functions, arithmetic and parenthesized expressions and handles a vector function followed by a scalar component selector. It retains zero/one/three texture-index behavior, integer/range/list checks and whole-import failure. It rejects an extra pending face rather than silently ignoring it.

Boundaries and primary references
----------------------------------

Lexer/include/scoped-token handling, material-block parsing, object transforms, material batching, GLSL and Builder handedness/winding sections are byte-identical to their preimages. The retained legacy LoadPOV source is unchanged. This packet does not parse or execute arrays, user-defined functions, conditionals/macros/loops, CSG or advanced textures, and does not qualify complete Blender/Rhino/NASA model compatibility. The new math is for model geometry; mission dynamics and scientific calculations are unchanged.

Primary POV-Ray references used during implementation:

* https://www.povray.org/documentation/view/3.7.0/228/ — float functions and units.
* https://www.povray.org/documentation/view/3.7.0/229/ — vector operations, promotion, components and constants.
* https://www.povray.org/documentation/view/3.7.1/231/ — float-function grammar including unary atan.
* https://www.povray.org/documentation/view/3.7.0/230/ — color component selectors.

The legacy 3.7.0 URLs display a 3.6 heading. Array reference https://www.povray.org/documentation/view/3.70/233/ was also reviewed to identify remaining work. This is not a claim of full current-version POV-Ray conformance. Remaining conditions/functions, grayscale extraction, scene variables, arrays/control flow, richer geometry/CSG and advanced texture semantics remain explicit.

Compilation and retained evidence
---------------------------------

Configured build v1 returned 0 in 9.64 seconds; last printed Ninja progress 6/6. Session 22986 terminated with runner/compiler return 0. Diagnostic lines are retained in summary.json.

Three existing source/help preimages, one unchanged legacy reference preimage and four planning snapshots preceded edits. Indexed deltas, before/build/final hashes, source map, function inventory, owning-source/research notes, compiler log and terminal receipt remain in the stage directory. Final sources match the successful build capture, added-line whitespace is clean and the visualization help XML parses. Public reader signatures/layouts are unchanged. Matching configured consumers compile; other-platform/toolchain behavior remains unqualified.

No model load, expression evaluation, shader compilation, rendering, runtime/GUI/MATLAB/helper/mission/numerical tests, new tests, acceptance auditors/readers/seals or evidence admission occurred. Compilation and static comparisons are implementation feedback only. Original requirements, fingerprints, source text, assertions, fixtures, tolerances, denominator, gates and dispositions remain unchanged. FRR!E44/E46 remain partial. Tracking-v109 remains 1093/1158 (94.39%), 65 unresolved engineering rows; recorded source-versioned verified baseline 1053/1158 (90.9%), all 105 original acceptance rows open. The full implementation audit and goal remain incomplete.

Next implementation
-------------------

Implement the remaining original POV grammar and geometry/material scope using primary references and representative public exporter sources. Classic array documentation specifies up to five dimensions, nested initializers, homogeneous initialized element types, whole-array scope and indexed assignments; these are research findings, not implemented features. Continue remaining numeric/condition/control functions, macros, arrays, richer geometry/CSG and advanced textures without silently omitting unsupported constructs. Preserve the complete original worklist, other function/solver/resource/persistence/body/frame/ephemeris implementation obligations and explicit specification/external/historical dependencies. Runtime verification follows the full implementation pass.
