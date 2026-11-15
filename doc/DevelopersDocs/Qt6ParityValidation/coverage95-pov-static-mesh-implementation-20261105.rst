Static POV mesh implementation, 2026-11-05
=========================================

The Qt visual-model path now has a native static POV reader. The installed
osgdb_pov module exposes writeNode, not a readNode implementation. The older
wx ModelObject also explicitly rejects POV, and its abandoned LoadPOV source
references fields absent from the current ModelObject. Neither is a working
import path to reuse unchanged.

Supported import contract
-------------------------

* triangle, smooth_triangle, mesh and mesh2 static geometry.
* union and object instances; named object/material/transform declarations.
* Finite scalar/vector literals and declarations, + - * / expressions, unit
  axis vectors and pi; translate, scale, XYZ rotation and 12-value matrices.
* Solid rgb/rgbf/rgbt/rgbft pigments, ambient/diffuse/specular/phong finishes,
  authored normals, mesh2 normal/texture indices and uniform per-face materials.
* Nested comments and UTF-8 paths/source. Nesting beyond128 is rejected.

Declared objects are instantiated only when used. Their transforms are retained;
declarations are not blindly concatenated into visible geometry. Geometry is
batched by material and reused for identical instances. The legacy GMAT Y/Z
exchange converts POV Y-up into GMAT Z-up, with compensating triangle winding
and transformed normals. The existing spacecraft normalization, offset, rotation,
scale, source persistence and file preflight consume the resulting OSG model.
Lighting uses the viewport's diffuse/specular approximation, not POV-Ray ray
tracing; pigment filter/transmit channels are displayed as opacity. Untextured
surfaces use a neutral gray viewport material. Direct pigment/finish modifiers
edit an instance's root texture; explicit textures on union children take
precedence over the union's texture. A second texture layer is rejected.

Mesh list counts, duplicate fields, integer indices, normal lengths, finite
expressions, nondegenerate triangles and nonsingular transforms are checked.
UV coordinate lists can accompany a solid-color mesh but image/procedural
textures are not imported. Multiple per-vertex texture IDs are accepted only
when identical. Unsupported constructs fail the import with a line diagnostic;
they are not silently dropped from a displayed spacecraft.

Includes, macros/loops, CSG/procedural primitives, layered/image textures and
per-vertex texture blending are outside this implementation. Scene camera,
light, background and global-settings blocks are ignored as render-only data;
preprocessor directives inside such blocks are rejected. The reader never
executes external scene code or opens includes. It implements a declared static
mesh subset, not the entire POV-Ray language.

Primary syntax references
-------------------------

The independently implemented grammar follows POV-Ray's own
`finite patch object reference
<https://www.povray.org/documentation/view/3.6.1/529>`_ and
`mesh2 reference
<https://www.povray.org/documentation/view/3.6.0/293/>`_. The exported-data mesh
subset is a bounded implementation choice. The historical worksheet does not
define a complete POV feature profile, so FRR!E46 and parent E44 receive no
automatic qualification credit.

Verification remains deferred
-----------------------------

No model fixture, GUI check, screenshot comparison or acceptance run was added
or launched in this implementation phase. Geometry, winding, transform order,
normal/finish inheritance, instance reuse, Unicode/file errors, negative scales,
malformed-data rejection and viewport interaction require the later verification
pass. The original denominator, assertions, failure receipts and gates remain.
