Function output boundary evidence
=================================

The stage is stored in
``build/nasa-coverage/complete18-20261112-function-bracket-boundaries-v1``.

Eight targeted cases pass through MainWindow Build and Run. They cover
zero-output calls, bracketed multiple outputs, malformed unbracketed output
lists and array-element output shorthand in Native and strict modes. Rejected
calls are corrected and executed, with independent scalar and CSV comparisons.
Function definition files remain unchanged.

The new tests exposed a validation defect. An unbracketed multiple-output list
could pass Build and then fail during Run. The interpreter now rejects that
list at Build with a specific diagnostic. Existing comma-aware tokenization
preserves commas inside an array index, and valid single-output shorthand is
retained. The optional strict mode still requires brackets for array outputs.

The initial supplementary fixture needed a caller Global declaration. Another
early expectation incorrectly treated passing Build as proof that unbracketed
multiple outputs could run. Those failed attempts and their later corrections
are retained. The aggregate runner also stopped before execution on an unrelated
stale documentation fingerprint, so the targeted checks used the registered
CTest entry point directly.

The four saved single-output policy cases and twelve source-format cases are
reused at their executed source versions. No full regression was run. Cloning
and function parent admission still depends on the requested interpretation of
the old worksheet. The readiness index identifies five candidate rows and
their remaining interpretation gates. Canonical coverage remains 1,113/1,158,
with 12/18 agreed rows complete.
