POV dynamic, mixed and optional array forms
==========================================

The Qt POV reader now supports dimensionless dynamic one-dimensional arrays, explicitly mixed element types and optional sparse initializer positions. Dynamic access grows the nominal size to include its index even for defined/ifdef/undef; fixed bounds and homogeneous type checks retain their prior defaults. Mixed arrays permit per-element type changes, including through macro argument aliases. Copies retain independent dimensions, dynamic/mixed modes, initialized values and holes. Optional initializer blocks accept empty or uninitialized plain-identifier positions while fixed arrays retain complete positional structure; optional flags do not implicitly propagate to nested leaf rows. Dynamic trailing commas add no phantom final element. Completed rows/values accept native optional commas, and final active controls are prepared before the initializer closing brace. Declaration terminal tokens remain held through publication. Three source/help paths compile; visualization help XML parses. Lexer/macros/cancellation, expressions, control formulas, geometry/material/transform/GLSL/Builder, worker/viewer paths and legacy loader remain unchanged. Native 3.8 source disables ordinary directives/macros during array declaration recognition, so arbitrary generated header tokens are not established as a missing native requirement. Complete SDL/exporter/profile and broader original campaign scope remain open. No array/model/runtime verification or row credit.

Array size and type ownership
-----------------------------

A declaration without dimension brackets creates a resizable one-dimensional array with nominal length zero. Any indexed access beyond its current length grows it to index plus one, including presence queries and undef. Missing entries remain uninitialized; ordinary reads still fail. Growth rejects an index whose required size cannot fit the existing signed dimension representation. Fixed arrays retain the one-through-five-rank and positive-dimension checks. Sparse storage does not allocate a value for every intervening missing position.

The explicit mixed flag removes the shared homogeneous tag constraint; repeated assignments can change a slot type. Macro formal aliases retain their actual owning array, so the same rule applies through a reference parameter. Ordinary arrays preserve their shared type and width/material-kind constraints, including after undef. Array value copies retain mode flags, dimensions, type metadata and recursively copied independent value cells; immutable geometry prototypes retain their existing sharing behavior. Uninitialized holes do not become initialized by a copy.

Initializer grammar
-------------------

An optional prefix applies to the single initializer block that follows it. In a multidimensional initializer, the leaf row blocks require their own optional prefix to omit leaf values. Outer optional flags are not inherited. Empty positions and uninitialized plain identifiers can leave holes; an uninitialized indexed value retains its normal read error. Fixed arrays still require the complete row/position structure, including separators needed to distinguish omitted positions. Premature closure cannot silently fill the remaining fixed positions.

Dynamic initializers derive their length from occupied or explicitly omitted positions. Their closing probe and trailing comma do not add an element. Native optional separators are accepted between complete values and rows. After a final value/comma, already active directives are prepared before checking the closing brace. Declaration terminal tokens stay held until the completed value is published; raw header lookahead does not open a following include.

Native scope finding
--------------------

The inspected upstream 3.8 array declaration parser disables ordinary directive and macro expansion while it recognizes dimensions and initializers. The tokenizer returns a hash token when ordinary directives are disallowed. Arbitrary directive-generated array headers therefore are not established as an additional missing native requirement by the original worksheet. Existing permissive expression/control behavior is retained, not promoted to a claim of complete native SDL grammar. Mixed and dimensionless array forms are documented 3.8 features. URLs and blob identities remain in research-note.json. No upstream code was copied.

The original NASA worksheet names the POV format without an accepted language/profile. Earlier historical research identifies a specialized exported smooth-mesh/material use case; it explicitly cautions against inferring an entire ray-tracer obligation. Representative exporter grammar remains unresolved and FRR!E44/E46 remain partial. Complete dictionary/pseudo-dictionary, reserved-word/SDL-version, macro filename, native function, geometry and texture compatibility are not implied by this packet. Independent original function/solver/persistence and other implementation work must continue while that model scope is researched.

Compilation and retained evidence
---------------------------------

Configured build v1 returned 0 in 10.58 seconds; last printed Ninja progress 6/6. Session 13783 is terminal. One existing Qt QImage::mirrored deprecation warning from OrbitRenderer.cpp remains; no compiler errors.

Three existing source/help preimages, four unchanged references and four planning snapshots preceded edits. Indexed patches, before/build/final hashes, source/research notes, build log and terminal receipt remain in the stage. Final sources match the compiler capture, changed-line whitespace is clean and visualization help XML parses. Lexer, macro/cancellation, all subsequent expressions/control/geometry/material/transform, material/GLSL/Builder and public entry-point sections compare byte-identical where recorded. Viewer/dialog/job and legacy references compare unchanged.

No runtime/GUI/MATLAB/helper/array/model/mission/numerical tests, new tests, parser/initializer evaluation, shader compilation, rendering, acceptance auditors/readers/seals or evidence admission occurred. Original requirements, fingerprints, source text, fixtures, assertions, tolerances, denominator, gates and dispositions remain unchanged. Tracking-v117 retains 1093/1158 (94.39%), 65 unresolved engineering rows; source-versioned verified baseline 1053/1158 (90.9%), all 105 original acceptance rows open. The full implementation audit and goal remain incomplete.

Next implementation
-------------------

Return to the complete original function/solver and any-Resource Save/persistence worklist, together with body/frame/kernel/ephemeris, resource-tree and explicit specification/external/historical requirements. For POV, inspect actual representative exported model sources and documented exporter grammar against the historical smooth-mesh/material use case; implement concrete omissions without inferring an entire ray-tracer obligation or silently qualifying the subset. Native dictionary/pseudo-dictionary and remaining function/string/color/geometry/texture compatibility and macro filename lifecycle remain open within that unresolved profile. Preserve all original rows and defer runtime verification until the implementation pass is complete.
