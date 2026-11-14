Current POV implementation review, 2026-11-08
============================================

Original rows FRR!E44/E46 remain unchanged. The current owning-source review
confirms that the old recorded chooser/CSG omissions are stale. The Qt chooser
accepts .pov, and preview and mission rendering use the current reader. Its
implemented static subset includes meshes, supported solid finishes, transforms,
finite primitives, solid Boolean operations, torus forms, scoped includes,
expressions, arrays, conditionals, loops, switch clauses and macros. The loader
has cancellation and detached CPU-model completion paths. Presence does not
establish full language, exporter or historical NASA model compatibility.

The retained historical LoadPOV.cpp is a specialized object-marker/material and
smooth-triangle reader with incomplete articulation. It is not a complete POV
language specification; ModelObject.cpp currently rejects .pov. The current Qt
implementation must not be replaced with that obsolete loader.

Primary research and scope
---------------------------

The `JPL model FAQ <https://maps.jpl.nasa.gov/faq.html>`_ describes CAD/blueprint
model data in available formats, principally Inventor. It supplies no complete
GMAT POV profile. The `spacecraft model author's page
<https://bjj.mmedia.is/povstuff/spacecraft/>`_ describes generated Galileo triangle
geometry and an approximately 8,000-line Cassini scene based partly on JPL OBJ
data. The inspected page supplies no model-source download link. These sources
were inspected as references; no third-party model or exporter was executed or
added to the product. They do not establish a new concrete importer omission.

Current source and test evidence
---------------------------------

Stage: ``build/nasa-coverage/coverage95-20261108-pov-current-review``. Current
owner preimages include the private solid-geometry fragment, parser, loader,
preview, renderer, help and legacy loader. The existing unchanged
``QtGui.ImplementationPovGeometry`` passes thirteen observations: three box
Boolean volumes, a transformed difference, ring and horn torus volumes, four
spindle-mode volumes and three intentionally empty results. The witness uses
signed mesh volume and independent scalar/cross-section expectations within
its existing tolerances. It was built against the current private runtime.

The check inspects generated geometry. It does not draw the model, compile
the corner-material shader through a graphics driver, or establish GUI file
selection, appearance, transparency, arbitrary clipping precision, complete
parser semantics, cancellation performance or an exporter corpus.

No production source, test, fixture, assertion or tolerance changed. Source and
runtime guards match; a verified source archive and all 1,334 protected original
hashes are recorded in results-summary-v1.json. No speculative parser extension
is warranted by this review. The broader model/profile and original parent
requirements remain open while the finite implementation checklist continues.

No requirement credit is claimed. Canonical acceptance remains 1,078/1,158
(93.1%), with 23 more requirements needed for the unchanged 95% target.
