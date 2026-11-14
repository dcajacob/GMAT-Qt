Achieve value and wrapper lifetime, 2026-11-05
=============================================

This is implementation work on the FRC!E93 source scope. It adds no original
requirement or verification credit. The broad scalar-parameter requirement
remains open; tracking stays 1086/1158 (93.78%), recorded verification stays
1053/1158 (90.9%) and all 105 original acceptance rows remain open.

Original child scope and owning paths
-------------------------------------

The original worksheet's FRC!E94--E97 children name scalar parameters defined
in FRR-38, user Variables, user Array elements, and real numbers. The native
Achieve command accepts real-valued wrappers, excludes whole Arrays and retains
reference-based goals by default. The already implemented AllowConstantGoal
option supports the worksheet literal child. The Qt command form selects
NumericReference by default and NumericSingle when that option is enabled;
desired values and tolerances use NumericSingle. Validator and wrapper
binding/evaluation remain the actual type-specific owning routes. This route
inventory does not by itself complete every FRR-38 family/body/frame domain.

Concrete repairs
-----------------

SetElementWrapper previously failed to retire a replaced goal wrapper and
could delete a replaced desired-value/tolerance wrapper that remained in use
by another field. Reinstalling the same pointer could also delete the active
wrapper. Replacement now deduplicates previous field pointers and deletes
only those no longer referenced by goal, desired value or tolerance. The
existing ClearWrappers ownership/deletion route remains in use.

Execute previously checked tolerance positivity only while registering the
goal. A mutable tolerance could later become zero, negative or non-finite and
be forwarded to the solver. Registration now requires finite measured value
and finite positive tolerance. Subsequent passes evaluate each current wrapper
once, check finite measured/desired values and finite positive tolerance, then
update the solver. An invalid later field cannot leave a partial Achieve
solver-result update. Valid solver update order and numerical formulas are
unchanged; initial registration keeps the native measured-value behavior.

The API tolerance field now uses the same range setter; that setter rejects
non-finite values. API desired-value writes reject non-finite values as well.
The literal-goal option, defaults, grammar, units, public signatures, class
layout and original acceptance tolerances/assertions are unchanged.

Compilation and remaining work
-------------------------------

Stage: ``build/nasa-coverage/coverage95-20261105-implementation-achieve-value-lifetime``.
The configured private integrated build passes in 9.11 seconds (session 3785
completed). Base, Qt, console, plugins and bindings built; test/probe clients
were linked only. Preimages, exact source-at-build patch/hashes, build log/
receipt and final patch/hash/summary artifacts are retained.

No test, GUI, script/function/model/solver execution, numerical comparison,
acceptance reader/seal or qualification run. No original fixture/assertion/
tolerance/fingerprint/denominator/ledger/gate changed. No installed/shared
runtime, package, commit or push update. Cross-platform execution is unqualified.

Complete the FRR-38 scalar family/body/frame source inventory and broader
implementation scopes before verification. Later verification must cover shared
and repeated wrapper replacement, different fields using the same expression,
Variables/Array elements/Parameters, native/constant goals, changing valid and
invalid tolerances, non-finite results and API writes. Compilation alone gives
no behavioral or numerical credit.
