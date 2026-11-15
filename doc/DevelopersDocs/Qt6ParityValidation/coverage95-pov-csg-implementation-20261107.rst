Static POV solid geometry
==========================

Original scope: FRR!E44/E46, spacecraft model formats and POV. The concrete gap
was that the object grammar accepted unions but rejected difference, intersection
and merge, despite their being ordinary geometry in the primary POV-Ray reference.

Implementation
----------------

The object/declaration parser now accepts those three operations and inverse on
supported solids. Existing boxes, spheres, cylinders and cones supply their
display tessellation. Closed mesh/mesh2 operands retain inside_vector and undergo
edge-incidence, orientability and inside/outside checks. Connected face winding
is resolved without replacing authored smooth normals. Meshes with unmatched edge
subdivisions or nonmanifold edges are rejected explicitly.

A private implementation fragment performs polygon splitting and solid Boolean
operations using explicit work stacks. It does not add an external library or
public GMAT API. Long clipping, topology and ray-membership sweeps check the
existing import cancellation predicate. Empty boundaries retain an outside-space
membership flag, so inversion and later operations distinguish empty from full
space. Object transforms are applied before clipping; reflected transforms retain
outward geometric winding and inverse-transpose shading normals.

Clipped vertices retain source-triangle barycentric material weights. The existing
corner-material shader therefore receives the original material/finish triplet,
rather than a replacement finish created by averaging parameters. Ordinary meshes
and ordinary unions retain their existing rendering path. Solid results are built
under their original model cache entry; temporary model pointers are not cached.

A completely subtracted model is valid and surface-free. The POV builder marks
that intentional result; common model preparation accepts it with unit framing
metadata and no added drawable. Both synchronous and background loading use that
preparation route. The ordinary missing-model fallback and invalid-bound rejection
remain for other inputs. Existing GMAT/POV axes conversion still occurs once.

Compilation and retained records
----------------------------------

Build v1 completed 5/5 steps in 8.24 seconds without warnings. Source inspection
then identified the empty-bound preparation conflict; OrbitRenderer.cpp was
captured before its integration edit. Build v2 completed 6/6 steps in 8.13 seconds,
with only the pre-existing QImage::mirrored deprecation warning, now at line 109
after an added include. Both observed compiler sessions are terminal. Final source
hashes match build v2; help XML parses and the four original reference hashes
remain unchanged.

Three existing source/help preimages, one absent-new-path record, the additional
pre-edit capture, patches and build receipts are retained in
build/nasa-coverage/coverage95-20261107-implementation-pov-csg.

Limits and continuation
-------------------------

No geometry, native/helper, GUI, mission or numerical runtime verification ran,
and no tests were added. This implementation is not whole-format acceptance:
infinite geometry, open cylinder/cone CSG operands, cutaway_textures,
image/procedural/layered textures, remaining primitive families and full
POV/profile scope remain explicit. The original worksheet does not define a
version/profile; the existing supported subset is not substituted for the full
original requirement. Clipping precision, topology, material interpolation,
transforms, empty/inverse cases, cancellation and viewer integration all require
the later verification phase. This increment changes no orbit calculation or
original assertion/tolerance.

All original rows, fingerprints, dispositions, fixtures and qualification gates
remain unchanged. No row credit is assigned. The recorded verified baseline is
1053/1158 (90.9%); 1093/1158 (94.39%) remains a carried engineering estimate.
The full implementation audit and campaign remain incomplete.

Return to the remaining original requirement clauses and inventory to separate
demonstrated missing behavior from implemented but unverified paths. Do not
automatically expand this increment into a ray tracer or restart the abandoned
GMAT include-language extension. Keep remaining POV capabilities visible.

Primary format references: `CSG grammar
<https://www.povray.org/documentation/view/3.60/533/>`_, `inside/outside rules
<https://www.povray.org/documentation/view/3.7.2/303/>`_, and `cutaway textures
<https://www.povray.org/documentation/view/3.6.1/362>`_. These define language
semantics; they do not recover an unspecified NASA model profile.
