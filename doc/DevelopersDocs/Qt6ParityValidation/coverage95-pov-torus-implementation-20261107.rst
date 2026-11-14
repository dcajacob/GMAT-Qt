POV torus geometry
====================

Original scope: FRR!E44/E46, spacecraft model formats and POV. The importer
previously rejected torus as an unsupported object type. It now parses the
major/minor radius form through the existing finite-primitive path.

The independent surface-of-revolution tessellation covers ring, horn and spindle
forms. Axis endpoints are explicit, periodic seams reuse coordinates, and collapsed
pole triangles are omitted. Smooth normals follow the circular section. Existing
transforms, instances, materials, cancellation and GMAT/POV axis conversion remain
shared with other primitives. No external geometry library or ray tracer is added.

The spindle modes implement the documented surface/membership distinction:

* union retains both displayed sheets and includes the spindle in the solid;
* merge displays only the outer sheet;
* difference excludes the spindle volume, retaining its boundary surface;
* intersection retains the spindle only and is surface-free on a non-spindle.

Each negative radius toggles inversion while its magnitude supplies dimensions.
Zero major radius has a spherical boundary; zero minor radius has no surface.
The optional sturm scalar is parsed as a ray-solver hint and does not alter the
display tessellation. Remaining unsupported constructs still fail explicitly.

Visible internal sheets are stored separately from the boundary needed for solid
membership. The CSG evaluator uses that boundary when clipping a solid operand;
ordinary display and standalone inverse retain the visible sheets. Inverted unions
likewise preserve their displayed component surfaces. An explicit empty-solid kind
uses the existing intentional-empty model path, including inside object wrappers.
Its volume can still participate in subsequent inversion/Boolean operations.

Compilation and limits
------------------------

The integrated build completed 5/5 steps in 9.68 seconds without warnings. The
observed compiler session is terminal. Final source/help hashes match compilation,
the five reference hashes are unchanged and help XML parses. Three preimages,
the patch, primary-reference notes and compiler receipt are retained in
build/nasa-coverage/coverage95-20261107-implementation-pov-torus.

No geometry/helper/native/GUI/mission runtime verification ran, and no tests were
added. Tessellation accuracy, singular/degenerate cases, shading, transforms,
transparency and nested CSG require later verification. This is not whole-format
acceptance or a definition of the unspecified NASA POV profile. Other primitive,
texture and geometry scope remains open.

Original rows, fingerprints, fixtures, assertions, tolerances, denominator,
dispositions and gates are unchanged. No requirement credit is assigned. The
verified baseline remains 1053/1158 (90.9%); 1093/1158 (94.39%) remains a carried
engineering estimate. The full implementation audit and campaign are incomplete.
The user phase-order question is still pending; no reply means no authorization
to start runtime verification. No further renderer expansion is selected by this
packet. Use the original remaining-clause/decision ledger for the next action.

Primary references: the `torus reference
<https://wiki.povray.org/content/Reference:Torus>`_, vendor parser semantics at
`Parse_Torus
<https://raw.githubusercontent.com/POV-Ray/povray/master/source/parser/parser.cpp>`_,
and `torus surface/membership semantics
<https://raw.githubusercontent.com/POV-Ray/povray/master/source/core/shape/torus.cpp>`_.
Implementation uses independent circular-section tessellation rather than vendor
ray-intersection implementation code.
