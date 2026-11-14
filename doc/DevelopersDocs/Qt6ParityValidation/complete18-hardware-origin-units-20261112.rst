Hardware-origin and tank unit display
====================================

The generic resource editor omitted metre units from HWOriginInBCSX/Y/Z.
A three-field fallback now supplies ``m`` for Hardware objects when native
metadata has no unit. Existing native units take priority; other quantities
and all numerical code are unchanged.

The original three blank-label failures are retained. Hardware origins pass
headlessly and visibly across eleven supported hardware types, including the
FuelTank and Thruster aliases. Tank mass, centre of mass, inertia, pressure,
temperature, density and volume contribute forty additional field observations.
Their passing headless evidence is reused from before the separate origin-label
repair. The completed batch has 146 passing observations, with native/source/
history/file isolation checked. No mission or full regression was run.

This batch completes 73 entries in the saved numeric-unit worklist, leaving 204
for review. The remaining count is not a defect count and does not enumerate all
current conditional input configurations. All Others E13 remains open; canonical
coverage stays at 1,111/1,158, ten of eighteen agreed requirements.

Evidence is sealed in
``build/nasa-coverage/complete18-20261112-hardware-origin-units-v1``.
The reader checks frozen source and definition documents, build and execution
identities, raw unit observations, the exact narrow patch and rejection of an
incorrect scale. Original numerical assertions and failure receipts remain
unchanged. No full-current-build or scientific approval is claimed.
