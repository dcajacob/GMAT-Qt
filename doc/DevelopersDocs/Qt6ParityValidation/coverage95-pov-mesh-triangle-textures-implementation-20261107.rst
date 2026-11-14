POV mesh triangle corner textures
=================================

The preceding packet enabled mesh2 corner material interpolation. Ordinary mesh triangle and smooth_triangle forms still rejected their separate three-texture identifier syntax. This batch implements that documented geometry/material path using the same resolved material and rendering representation. It does not redefine or complete the worksheet POV requirement.

Primary sources
---------------

The official triangle interpolation reference describes three declared texture identifiers in a mesh triangle texture_list, for both flat and smooth triangles: https://www.povray.org/documentation/view/3.60/360/ . The official parser's Parse_Mesh1 and Parse_Mesh_Texture functions confirm mesh context, declared texture references and comma parsing between entries: https://github.com/POV-Ray/povray/blob/master/source/parser/parser.cpp . The inspected parser file blob is c13e02a5eaed8066777b8513dddabe205229f4a8. These sources were inspected, not executed; the receipt records source locations and design boundaries. No POV-Ray source was copied into the product.

Implementation
--------------

The mesh parser passes explicit mesh-triangle context when reading triangle or smooth_triangle faces. In that context a texture_list resolves exactly three declared texture names from their current lexical scopes. Optional commas support the documented whitespace-separated spelling and the parser's comma-separated spelling. Material values are captured while each name is in scope, so subsequent redefinition cannot rewrite earlier faces. Each selected material uses the reader's existing supported defaults and fields.

The face keeps the three values in authored corner order. Existing face copying into the mesh retains this immutable tuple; the preceding renderer applies its winding/normal association and shared mixed-material batching without modification. Equal effective materials retain the fixed-function path. Whole-object inheritance remains selected for untextured faces.

A triangle list has no count and accepts texture identifiers, unlike the separate counted mesh2 texture list of texture blocks. Missing, undefined or wrong-type names, extra entries, malformed braces and quoted keywords fail with attributed diagnostics. Corner textures combined with other local material statements fail under the existing unsupported layered-material policy. This policy remains a limitation; it does not establish support for every material override accepted by POV-Ray. Lists outside mesh triangles remain unsupported. Public readPovModel signatures and exported instance layouts are unchanged.

Compilation and remaining work
------------------------------

Configured C++ compiler v1: return 0, 7.64 seconds, last logged step 6/6.
Configured C++ compiler v2: return 0, 5.18 seconds, last logged step 5/5.

Three source/help and four planning preimages preceded edits. Before/build/final hashes, indexed patches, source map, research receipt, compiler logs and terminal receipts are retained. Both compiler sessions are terminal and final source hashes match v2. Added-line whitespace is clean and the help XML parses. Existing OrbitRenderer QImage deprecation diagnostics are retained. Embedded shader strings remain C++ data: no graphics driver compiled them in this phase. No model, rendering, inheritance, scope or numerical execution was performed. Those behaviors require subsequent verification.

Continue independent public exporter/model and original historical POV scope research without asking the user for a dialect. Implement remaining concrete grammar and geometry/material omissions, including richer declarations/expressions/control flow, arrays, unsupported geometry/CSG and image/procedural/layered finishes as required by the owning scope. Mixed corner materials and both mesh grammars do not establish whole Blender/Rhino or NASA model compatibility; keep the complete original POV and parent requirements open. Continue the full original implementation worklist, external/historical/specification dependencies and rotation-drift research. Verification follows completion of implementation.

No runtime/GUI/model/mission/numerical tests, new tests, acceptance auditors/readers/seals, evidence admission, fixture/assertion/tolerance/denominator/gate changes, packages, installed/shared runtime changes, commits, pushes or external messages were performed in this batch.

Tracking-v97 remains 1093/1158 (94.39%), with 65 unresolved rows. The recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full implementation audit incomplete; goal active.
