Historical POV import scope research, 2026-11-05
================================================

The user could not identify a model dialect/profile. No further format choice
or representative model is required from the user. Research and owning-source
work must establish the import scope independently. FRR!E44/E46 remain partial;
the original worksheet names POV without describing its accepted language.

Historical owning sources
---------------------------

The retained src/gui/rendering/LoadPOV.cpp is a specialized mesh reader, created
in 2009 and derived from Odyssey code according to its header. It recognizes
object marker tokens, rgbf or rh_layercolor material sequences, ambient/diffuse/
phong values, and smooth_triangle vertex/normal groups. It swaps Y/Z components
and reverses triangle ordering. It also recognizes rh_phong, rh_phong_size and
rh_image_map token sequences. An articulation branch is explicitly incomplete.
Fixed-size legacy structures are not a general POV scene interpreter.

The retained ModelObject.cpp rejects .pov/.POV explicitly. Its current model
structure is not the old vertex/polygon/material layout expected by LoadPOV.
The shipped visualization help still described only .3ds support. Therefore
merely enabling that old function is not a usable restoration. The Qt reader
already independently implements a broader static geometry subset, with
separate include/scoped-string/finite-primitive batches. The user-facing help
now distinguishes the legacy GUI from Qt and describes the supported subset.

Inference: the historical loader supports an exported smooth-mesh use case,
including CAD-like rh_* material tokens. It does not establish an obligation
to implement the entire ray-tracing language, nor does it establish a complete
current import profile. No uninspected Rhino exporter compatibility is claimed.

External primary-source research
----------------------------------

The model author's `spacecraft model page
<https://bjj.mmedia.is/povstuff/spacecraft/>`_ describes Galileo and Cassini POV
models, including generated triangle geometry and a much larger authored
scene. This confirms that public spacecraft uses of POV can differ markedly;
it supplies neither a GMAT-required profile nor downloadable model source on
the inspected page. No geometry was executed or visually qualified.

POV-Ray's own `directive documentation
<https://www.povray.org/documentation/view/3.7.1/242/>`_ describes its conditional/
loop-capable language and user message/error directives. Thus a general .pov
scene is more than a fixed mesh container. The existing subset must remain
explicit. Attempts to inspect the corresponding NASA GitHub raw/blob pages
returned cache-miss errors; the local owning source, not those failed fetches,
supports the historical findings above.

Remaining implementation
--------------------------

Find actual representative exported model sources and their documented exporter
grammar. Review material/include/control constructs against the existing static
reader and implement concrete omissions without guessing at scene semantics.
Macros/loops/conditionals/arrays, further primitives, CSG and advanced textures
remain outside the implemented subset. Do not silently narrow the original
worksheet or count FRR!E44/E46 complete. This research adds no acceptance or
engineering row credit. Continue other independent implementation scopes while
researching these models; do not ask the user to define a POV dialect again.

The FK5 source-scope stage retains this report and the help correction. No test,
model load/render, GUI, solver run, numerical comparison or qualification run.
Original source assertions, fixtures, fingerprints, denominator and gates remain
unchanged. No imported third-party model source was added to the product.
