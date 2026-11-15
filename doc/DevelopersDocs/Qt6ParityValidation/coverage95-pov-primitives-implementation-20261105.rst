POV finite primitives, 2026-11-05
================================

The native Qt POV importer now accepts box, sphere, cylinder and cone objects,
including declared prototypes, object instances and union children. This extends
the earlier triangle/mesh reader. It does not claim the entire POV-Ray language
or close the original FRR!E44/E46 requirements.

Implemented geometry
--------------------

Boxes use either pair of opposite corners and produce six flat faces. Spheres
use their authored center and positive radius. Cylinders use distinct endpoint
centers and a positive radius. Cones use distinct endpoint centers and
nonnegative endpoint radii, at least one positive; either end can be a tip.
Cylinders and cones have flat closed caps unless the open keyword is present.
These forms follow the primary POV-Ray
`finite solid syntax <https://www.povray.org/documentation/view/3.8.0/528/>`_,
`box description <https://www.povray.org/documentation/view/3.8.2/22/>`_,
`cylinder description <https://www.povray.org/documentation/view/3.60/278/>`_
and `cone description <https://www.povray.org/documentation/view/3.60/277/>`_.

Curved surfaces are display tessellations with 64 angular subdivisions; spheres
also have 32 latitude divisions. Sphere poles and zero-radius cone tips do not
emit collapsed duplicate faces. Curved-side normals come from the analytic
surface, while caps and boxes use independent flat normals. Arbitrary cone and
cylinder axes use a local orthonormal basis. Cone-tip normals use the sector
midpoint where the limiting smooth normal depends on approach direction.
This is viewport geometry, not a ray-traced or CSG surface representation.

Primitives use the existing material inheritance, declared-instance reuse,
model transforms and Y-up/Z-up conversion. Their tessellation remains in local
model coordinates so that authored translations, rotations and nonuniform
scales follow the same path as mesh geometry. Invalid dimensions and values
outside display precision raise an import diagnostic rather than returning
partial geometry.

Vector and transform syntax
---------------------------

Scalar values can be used where vectors are expected, including sphere {0,1},
box {0,1}, vector lists, UV vectors and transforms. Scalars repeat into the
required components; shorter vector values gain zero components. Arithmetic
also distinguishes scalar repetition from shorter-vector zero extension. See
the primary `operator promotion reference
<https://www.povray.org/documentation/3.7.0/r3_3.html>`_.

A zero scale component is treated as one, following POV's no-scaling convention
for that axis. Explicit singular matrix transforms still fail the existing
inverse/finite checks. See the primary
`scale tutorial <https://www.povray.org/documentation/3.7.0/t2_2.html>`_.

Remaining implementation and verification
-----------------------------------------

Includes, macros/loops, other procedural primitives, CSG, layered/image textures
and per-vertex texture blending remain outside the implemented importer. The
complete historical POV requirement is retained as partial. The earlier static
mesh implementation note describes the previous increment; its statement that
all finite primitives are unsupported is superseded for these four shapes.

The configured private build passes: six steps in 7.14 seconds. No model input,
test executable, GUI, renderer check, screenshot comparison or acceptance audit
was run. Later verification must cover primitive topology/winding, normals and
caps, both cone-tip orientations, endpoint/corner ordering, transforms and
instances, scalar/vector promotion, model normalization and malformed inputs.
Original fixtures, assertions, tolerances, denominator and qualification gates
are unchanged. No installed/shared runtime or public package was updated.

Implementation stage:
``build/nasa-coverage/coverage95-20261105-implementation-pov-primitives``.
