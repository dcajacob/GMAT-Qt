Solve-for declaration transactions
=================================

Estimator AddSolveFor now prepares its replacement selection before asking EstimationStateManager to validate/store the role, and publishes the public list only after that call succeeds. The state manager stages all five parallel solve-for name/object/ID arrays and swaps them only after all string/vector allocations complete. Qualified append and indexed setters restore prior parser scratch on failure. The already-bound object overload uses the same staged metadata storage. Native last-dot decomposition, object/name/ID values, index bounds/diagnostics and the three overloads' existing duplicate return/logging policies remain. The helper rejects inconsistent existing metadata arity instead of indexing mismatched arrays. No native state preparation, equations, domains, ready-state policies, parameter IDs, class instance fields or virtual contracts change. Compilation only; no row or verification credit.

Declaration publication
-----------------------

The estimator previously edited solveForStrings before calling esm.SetProperty. A missing separator or invalid native position could then reject the value after the public list had changed. The owner now stages the complete selected list and publishes it with swap after state-manager acceptance. Its native bounds diagnostic and successful duplicate behavior remain.

EstimationStateManager now uses one private nonvirtual StoreSolveForRole helper for qualified append, indexed qualified selection and already-bound object roles. It validates parallel-array arity and position, copies each role array, performs every append/replacement in the temporary arrays and then swaps all five into place. Raw object entries remain non-owning pointers; this adds no cloning, ownership transfer or provider initialization. Qualified parsing still calls the existing DecomposeParameter and retains its last-dot and diagnostic behavior; parser scratch is copied before parsing and swapped back if parsing/metadata staging fails.

Limits
------

This packet stages declarations, not a whole active estimation session or arbitrary-provider transaction. It preserves the existing native last-dot parser and duplicate behavior rather than claiming complete reconciliation between explicit selections and automatically discovered solve-for roles. Native state-layout/reference readiness, subsequent object/property/domain validation, covariance/posterior/session behavior and all original navigation/function/command/any-Resource scope remain open. No allocation-failure injection, bad-input or duplicate execution, native preparation, reload or scientific qualification occurred. The new storage helper is private and nonvirtual; there are no instance-field, virtual-contract or parameter-ID changes.

Compilation and retained evidence
---------------------------------

Build v1 returned 0 in 16.21s, progress 22/22, 0 existing compiler deprecation warnings; no compiler errors. Compiler session 90308 is terminal. Final hashes match build v1. Four source/help preimages, eleven unchanged reference files and four planning snapshots preceded changes. Indexed/combined patches, hashes, source findings and compiler receipts are retained. Delta whitespace is clean and the Save help XML parses. Native binding/preparation, measurement manager, state manager, state/covariance, RunEstimator and resource-help reference sources remain unchanged.

No new tests, declaration/allocator/parser/duplicate/native-preparation/reload/GUI/model/helper/MATLAB/mission/numerical/runtime runs, acceptance auditors/readers/seals or evidence admission occurred. Original requirements/text/fingerprints/fixtures/assertions/tolerances/denominator/gates/dispositions remain unchanged. Tracking-v129 retains 1093/1158 (94.39%), 65 unresolved engineering rows; recorded source-versioned verified baseline remains 1053/1158 (90.9%). All 105 original acceptance rows remain open. Full implementation audit and goal remain incomplete.

Next implementation
-------------------

Continue coherent live declaration/component/storage preparation and explicit-versus-automatically discovered solve-for/duplicate role reconciliation; the staged declaration update does not complete those contracts. Finish other navigation input transactions, full native provider/default/list/reference/session behavior, any-Resource serialization/portability, original function/scoping/shared-global/dependency lifetimes, all-command solver/estimation/MATLAB behavior and every remaining worklist dependency before runtime verification. Deferred checks include bad syntax/index, append/replace/object-bound roles, each duplicate policy, metadata allocation failures, parser scratch restoration, subsequent binding/native preparation and scientific estimation behavior.
