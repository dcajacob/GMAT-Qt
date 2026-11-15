FRC!E54 optimization termination admitted
=======================================

The source-versioned acceptance union is now **1,077/1,158 (93.0%)**.
There are **81 original requirements open** and **24 more needed** for the
unchanged 1,101-requirement / 95% target. The separate historical operation
union remains 1,816/2,367.

Only FRC!E54 is newly admitted. Its original text remains: "The system shall
allow a user to define the end of an optimization sequence by issuing an
EndTarget command." FRC!E6's EndOptimize spelling remains supported. The
Target wording in adjacent rows is preserved; it does not replace this
optimization obligation. The complete original context is retained in the
scope review, including FRC!E5/E6/E7 and FRC!E53/E54/E55.

Whole-row evidence
------------------

The shared interpreter accepts EndTarget for an innermost Optimize. Append
logic preserves ownership when Target and Optimize nest in either direction;
EndTarget validates either solver owner. Other block types still require their
matching marker. Its execution and serialization retain the end-command
contract. This structural path is independent of the selected optimizer's
numerical algorithm. Qt recognizes both endings for branch operations.

The component in
``build/nasa-coverage/coverage95-20261108-optimizer-endtarget-validation``
passes eleven native cases and eight GUI executions. Eight actual optimizer
runs cover four canonical/worksheet pairs, including both mixed solver nesting
directions. Literal quadratic and linear solutions, sentinels and successor
execution agree bit for bit between spellings. Three wrong-marker cases reject
for their specific structural errors. The unchanged canonical structural tests
and derived alias fixture pass original/If-contained insertion, ownership,
Undo/Redo, nonempty summary and Unicode source Save/reopen checks in both
profiles. There are 48 native, 192 raw GUI report and 96 summary scalar checks;
the visible profile records 102 native exposures and zero screenshots.

See ``coverage95-optimizer-endtarget-verification-20261108.rst`` for the
component's exact source/runtime and qualification limits. Production behavior
was already implemented; this increment adds verification and row admission.

Admission and preservation
-------------------------

Admission stage:
``build/nasa-coverage/coverage95-20261108-optimizer-endtarget-row-v1``.
Its 71 frozen readers reconstruct the prior 1,076-row union and admit exactly
this original row. Twelve deliberately incomplete or altered admission records
are rejected. An independent frozen-reader replay produces a byte-identical
ledger. No native mission is replayed to rebuild the ledger.

The final source capsule contains 2,731 paths, including all 1,334 protected
original inputs and assertions. All original fingerprints, numerical bounds,
1,158/2,367 denominators and the 1,101 target are preserved. The full audit
changes only FRC!E54; the other 1,157 row records and other 138 closure records
are unchanged. All 59 separate formal hold identities and statuses remain:
42 resolved and 17 unresolved, including the same six unresolved MATLAB holds.

The sealed prefix, receipt, manifest, row audit, exact delta and remaining-row
list are in the admission stage. Run its ``verify-stage-v1.py`` for a read-only
replay using the archived readers and source versions.

FRC!E7 and FRC!E55 still require their full all-command solver scopes. This
admission does not qualify every optimizer's convergence, scientific/physical
frame or ephemeris truth, external ABI, platforms, native OS input, the complete
current build or full NASA acceptance. The campaign goal remains active.
