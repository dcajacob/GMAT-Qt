Interpreted invocation command and prototype frames
==================================================

Nested entry previously retained one interpreted command graph and one set of prototype/validator tables. A recursive invocation could therefore reset the command objects still executing in its enclosing call.

The new UserDefinedFunction suspension override stages a cold structural clone through the existing native definition-copy path. An internal storage owner is never registered or executed as a function; actual command self references and self declaration aliases identify the real callee. Staging precedes live field detachment. The frame swaps the complete interpreted field set without allocation, preserving the original command objects and their execution cursors. Common function metadata, argument interfaces and environment views continue using the existing layered invocation protocol.

Restoration attempts concrete inner function completion while its prototype tables are installed, then restores the enclosing body even if completion throws. The layered protocol reports restoration failure and continues common-field restoration. Inner graph storage retires before enclosing environment ownership is released. GmatFunction retains its actual unused-global list in a private owner; the inner list stays installed through command completion, and the outer leaf list/flag restore afterward. Rebuilding that list stages lookup results before replacing its existing owner. Copies and ordinary root calls retain their existing cold initialization paths.

Limits and next work
--------------------

This is implementation and compilation only. The existing definition clone still relies on each concrete command/resource native copy and initialization contracts; it does not prove complete plugin ownership, passive cross-resource rebinding, resource value-provider admission or runtime behavior. In particular, caller PopFromStack still copies saved resource values and reinitializes callers; complete resumption of enclosing command caches and nested/iterative/recursive solver/loop state remains required. Do not credit the complete function I/O/core/scoping parents or the any-command requirement.

Complete caller resumption without resetting live enclosing loop/solver execution, including cross-resource bindings after saved values and outputs are copied. Complete every command/plugin cold-copy, ownership and passive rebinding route and passive UserDefinedFunction/GmatFunction value providers; complete direct/finalization/custom-manager reentry and all shared interpreter/service lifetimes. Continue remaining solver/estimator/simulator/subscriber/interface/measurement and body/kernel/NAIF/scientific/resource clauses, output-name/cache transactions, legacy discovery, inout aliases, external maps, custom vector arithmetic and arbitrary scope/interior/engine-reset/later lifetimes. Keep original full scopes and verification deferral; research POV and unresolved rotation semantics independently without inventing specifications.

Compilation
-----------

v1: 11/76 steps, 9.17 seconds, return 1.
v2: 67/67 steps, 4.62 seconds, return 0.
v3: 4/4 steps, 1.94 seconds, return 0.
v4: 66/66 steps, 5.18 seconds, return 0.
v5: 66/66 steps, 4.32 seconds, return 0.

The first compile exposed the abstract storage base; a private non-executable concrete storage owner resolves it. The final configured core/Qt/console/enabled-plugin/Python-consumer build passed. These overrides add no instance fields or virtual slots; matching configured consumers rebuilt. Source preimages, before/build/final hashes, indexed deltas, terminal receipts and four planning preimages are retained in ``build/nasa-coverage/coverage95-20261106-implementation-interpreted-invocation-frames``. New-line whitespace is clean.

No runtime tests, GUI, missions, function/model/solver execution, numerical comparison, acceptance readers or seals ran. No packages, installed/shared runtime, commits or pushes. Original assertions, fixtures, tolerances, source texts, fingerprints, denominator, ledger and gates remain unchanged.

Tracking-v58 remains 1093/1158 (94.39%) implementation present and 65 other rows. Recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full implementation goal remains active and incomplete.
