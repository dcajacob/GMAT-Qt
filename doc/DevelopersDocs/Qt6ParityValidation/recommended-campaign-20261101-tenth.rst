Recommended campaign: visual file transformations
==================================================

Three requirement scopes are recovered: FRR!E47 (XYZ translation), FRR!E48
(fixed model rotation), and FRR!E49 (model scale used by OrbitView).
Source-versioned campaign coverage is now **898/1,158 requirements (77.5%)**
and **1,816/2,367 historical scopes (76.7%)**. No original denominator or
held/deferred source row is changed. This is selected capability evidence,
not full current-build or NASA qualification.

The 217 recovery candidates now have 133 accepted, 39 held and 45 pending
dispositions. All pending recovery candidates are requirements. The local
contracts remain 36 verified and one unqualified all-input documentation row.
The active goal is not complete.

What the source review established
-----------------------------------

The saved transform mapping points to driver291ab378..., used by the separately
repaired visible Load3DS test. The six actual transform cases used af6c69b6...
in both modes. Those two whole-file hashes are not interchangeable.

The review proves exact byte identity of the applicable source for Translate,
RotateX, RotateY, RotateZ, Scale and Compose: the only functional difference is
the separate Load3DS filename-entry branch. Its include and picker-result
diagnostic are also identified explicitly. After substituting the unchanged
non-Load3DS picker branch and removing only those identified additions, the
whole earlier source must equal the normalized later source. Any other code or
assertion change rejects the comparison. The actual registered command's family
argument must match. Load3DS and unknown families are explicitly excluded.

Both mode-specific reviewed mappings retain every original legacy and native
clause, expected value, checkpoint, facet, scope, assertion source path and
reference path. Actual execution source/reference hashes are recorded per
component. Original receipts, mappings and snapshots remain untouched; no old
run is rebound to newer GUI code. Actual selected input-family records must
also equal the mapped later records. Every source/command/executable/snapshot,
terminal/JUnit and structured proof guard is rechecked.

The complete conjunction has 18 distinct retained contexts: twelve native
transform contexts, four legacy GUI/rendering contexts and two offscreen
runtime-support contexts. ModelDrawingRuntime explicitly rejects platforms
other than offscreen and has requested visibility=headless. It remains a required
runtime guard; it is not presented as a visible GUI witness. Every actual GUI
witness in the visible cohort still requires native exposure. Historical
Workflow subpanels were not all individually exposed; the later native transform
cases provide the explicit input/preview/mission witnesses.

NASA intended behavior and checked results
-------------------------------------------

The exact source requirements name XYZ offset, fixed rotation and OrbitView
scale. Shipped SpacecraftVisualizationProperties field definitions and the
original wx ModelObject implementation provide the referenced behavior and
transform conventions. This increment does not invent a new model transform.

The twelve native contexts cover the six selected OBJ transformation families
in both modes. A combined XYZ rotation and nonzero XYZ offset supplements the
individual axis rotations, translation and scale. Each requires pending preview,
reopened preview and final mission rendering. The original driver additionally
checks inherited file/transform values in the actual mission PlotModel, identity
attitude, sentinel9, invalid values, exact history and source/native isolation.

The independent original raster oracle recomputes Rx/Ry/Rz, normalized mesh
offset/scale and orthographic projection. All 36 retained original PNG captures
were read again, without copying or capturing an image. Their green-pixel counts,
bounds and centroids match the original published proof records exactly.
Independent projected bounds retain the original three-pixel size and two-pixel
centroid tolerances. All declared nonfinite/text rejections remain required.
Thirty-six mutations of actual pixel results, rejection lists and transforms
are rejected.

The hash-bound independent review links each image/measurement to its exact
original receipt, log and structured proof. Passing PNGs are not required for
later reuse of that sealed numerical review; if still present, their hashes
must match. No existing sealed image is deleted here. No screenshot, image
copy, runtime copy or GMAT execution is created by this increment.

These remain controlled OBJ probes, selected transforms, fixed cameras and
private software OpenGL. Arbitrary meshes/file formats, other transform ranges,
camera/material/occlusion domains, physical GPU/compositor/OS input, platform
coverage and current NASA's unavailable private corpus remain unqualified.
In particular, an earlier source is never said to execute the later Load3DS
repair. No application/core source changes are made; all 98 prior changed core
files retain their separate technical-scrutiny obligation.

Validation and artifacts
-------------------------

All 32 recovery/tooling unit contracts pass, including four reusable tests of
the source-equivalence guard. Fifteen corruptions of actual mappings, bindings,
semantic-review identity and source comparisons are rejected. The campaign
builder independently recomputes this narrowly scoped audit before adding
the three rows. Earlier recovery and local-contract qualifications remain
validated from their own inputs, with original assertions preserved.

In ``build/nasa-coverage/recommended-campaign-20261101/``:

* ``visual-unattended-reviewed-mapping.json`` and
  ``visual-visible-reviewed-mapping.json`` record exact per-component versions.
* ``visual-recovery-plan.json`` and ``visual-recovery-audit.json`` retain the
  complete original conjunction, source-equivalence checks and witness roles.
* ``visual-pixel-audit.json`` records 36 original raster/geometry rechecks,
  36 rejected corruptions and source/receipt/log/proof identities.
* ``visual-review-negative-checks.json`` records fifteen rejected actual review
  corruptions; ``recovery-contract-tenth.log`` records the 32 unit tests.
* ``tenth-increment-audit.json`` records preservation and this bounded result.

The remaining graphics options and all other 45 pending recovery rows still
need their complete individual reviews. The all-input documentation gap,
previously held candidates, scientific/optional/interface projects and
authority-dependent scope remain open. No whole-parent or broader input-domain
credit is inferred from these three rows.
