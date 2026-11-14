STM read failure preserves reference history
===========================================

The five public orbit-STM Parameters previously captured a new initial reference
before validating the requested frame. A nonfinite frame was rejected, but its
failed read left reference history behind. The conversion now prepares that
reference privately and commits it only after the full matrix calculation succeeds.
No public API or normal successful-read behavior changes.

Targeted evidence
-----------------

* 80 existing passes are reused at their executed source version.
* All five original reference-history mutation failures are retained.
* Five repaired rejections and five new successful-capture controls pass.
* Three replay negative controls reject altered state, rejection and matrix values.
* 1,333 protected inputs remain unchanged; the existing authorized ASCII title
  correction is the only changed protected helper. Original assertions and bounds
  are unchanged. No full regression was run.

The joined 90 observations cover the explicitly listed nonfinite, shape, frame,
history and augmented-storage cases. They are not a full current-build or
whole-row qualification. FRR!E525 remains open; the separate remaining-obligation
record preserves the broader frame/body/force and reference-source work.

Evidence is in
``build/nasa-coverage/complete18-20261112-stm-domain-atomicity-v1``.
The sealed coverage remains 1,112/1,158 and 11/18 agreed rows.
