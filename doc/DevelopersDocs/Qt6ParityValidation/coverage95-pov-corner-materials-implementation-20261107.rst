POV mesh2 corner material interpolation
======================================

The Qt reader rejected mesh2 triangles whose three corner texture indices differ. Public exporter source supplied a concrete grammar omission without requiring the user to identify a POV profile. This batch implements that omission; the original worksheet POV scope is not narrowed or declared complete.

Primary source research
----------------------

* Blender's official ``render_povray/model_poly_topology.py`` writes three texture-list indices per mesh2 triangle, including a vertex-color path. Its inspected file blob is ``808facc1c9e8dac0f52921383d4cbd5ccfbc7c56``. Source: https://github.com/blender/blender-addons/blob/main/render_povray/model_poly_topology.py . This establishes the exported face grammar, not compatibility with all other Blender material or scene constructs.
* The official mesh2 tutorial describes one material per face or three interpolated corner textures, mixed with unindexed faces in one mesh: https://www.povray.org/documentation/view/3.70/68/ .
* The official triangle interpolation reference also documents corner texture selection for mesh triangle forms: https://www.povray.org/documentation/view/3.60/360/ . Those separate triangle texture-list forms remain additional implementation scope.

The research receipt also retains the earlier Rhino and JPL source findings. Neither defines a complete historical NASA spacecraft POV profile. Exporter/documentation sources were inspected; no third-party model or exporter was executed and no model corpus was qualified. The user's lack of a requested profile is not treated as a blocker.

Importer and renderer
---------------------

Face texture selection now records zero, one or three indices. One index expands to the same selection for all three corners; every explicit index is checked against the texture list. Different corner selections resolve independent supported solid material values. Unsupported procedural/image/layered material statements still fail with an attributed diagnostic. Existing unindexed material inheritance is retained.

The builder resolves effective material values before batching. Triangles whose effective corner materials are equal keep the existing fixed-function path. Other triangles carry barycentric weights and all three color/finish tuples, retaining authored corner identities through the existing 0,2,1 winding reversal and Y/Z exchange. Each fragment evaluates the three supported solid finishes with the mission viewer's single light, then blends the resulting colors and opacity. Separately interpolating pigment and finish would create unwanted cross terms. The viewer lighting remains an approximation to POV-Ray rendering.

One shared program per reader build and up to two mixed-material batches per model avoid a shader program, uniform state and draw call for each triangle. Opaque and translucent faces use separate batches; translucent geometry uses the existing alpha blend/depth-write policy. General depth-sorting limitations remain unqualified. Two corner weights share varying lanes with transformed normal and eye position; six other vec4 varyings carry the material triples. This uses eight vec4 varyings and seven fixed texture-coordinate inputs in GLSL 1.20. No additional public API, virtual slot or exported instance layout is introduced; parser/builder data remains local to this translation unit.

Compilation and phase boundary
------------------------------

Configured C++ compiler v1: return 0, 7.96 seconds, last logged step 6/6. The Qt library and application linked. Existing OrbitRenderer QImage deprecation diagnostics are retained in the log.

Three source/help preimages and four planning preimages preceded source edits. Before/build/final hashes, indexed patches, source map, research receipt, compiler log and terminal receipt are retained. Session 53235 is terminal (71b45b). Final source hashes match the successful build, added-line whitespace is clean, and the help XML parses. C++ compilation accepts the embedded shader strings; it does not compile them with a graphics driver. No standalone GLSL compiler was found on PATH. Shader link, hardware limits, model import, performance, transparency and appearance remain for verification.

Continue independent public exporter/model and original historical POV scope research without asking the user for a dialect. Implement remaining concrete grammar and geometry/material omissions, including mesh triangle texture lists, richer declarations/expressions/control flow, unsupported geometry/CSG and image/procedural/layered finishes as required by the owning scope. Do not infer whole Blender/Rhino or NASA model compatibility from one mesh2 grammar repair; retain the complete original POV and parent requirements open. Continue the full original implementation worklist, external/historical/specification dependencies and rotation-drift research. Verification follows completion of implementation.

No runtime/GUI/model/mission/numerical tests, new tests, acceptance auditors/readers/seals, evidence admission, fixture/assertion/tolerance/denominator/gate changes, packages, installed/shared runtime changes, commits, pushes or external messages were performed in this batch.

Tracking-v96 remains 1093/1158 (94.39%), with 65 unresolved rows. The recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full implementation audit incomplete; goal remains active.
