RHS expression ownership — implementation, 2026-11-06
====================================================

RHSEquation native copies now own independent MathTree definitions and wrapper maps.
Identical wrappers under multiple lexical names copy once; cloned owned values are
registered before borrowed wrapper aliases reconnect. This includes native value
clones whose source wrapper borrowed its object. RAII holds new copies through
construction, type/source/peer checks and map association. Native definition copies
still need owner initialization; resource/callee/scope lifetime closure is unfinished.

Assignment native copies no longer share the original RHS container. Assignment
copies base command state through its native operator and holds the old context while
replacing state. Cleanup identifies shared lhs/rhs operands by actual pointer while
the equation map still exists; each top-level wrapper deletes once. Replacement keeps
wrappers still used by another role. Reference-name inventory retains populated map
slots. These changes repair container aliasing, orphaned wrapper and deletion paths.

Changed expression strings rebuild instead of returning the old tree. Parsing stages
new owners and cleans a successfully parsed graph if later quote checks fail. Same
expression initialization keeps its tree. Reinterpretation stages a new RHS and drops
the old one, including transitions to string assignment. Lookup guards null local/
global maps, uses find without inserting/caching into caller scope maps, reports
missing/binding errors and associates each shared top-level wrapper once. A null
wrapper setter returns false. Optional debug string-result tracking is corrected.

Compilation and retained scope
--------------------------------

Private integrated build v1 passed82 steps/139.95s. V2 failed during source debugging
because NULL was forwarded as a long integer to map.emplace; v3 corrects it to nullptr
and passes66 steps/3.09s. V4 handles the string transition and passes66 steps/3.01s.
Final four source hashes match v4;57363/19876/31385/19074 closed. RHSEquation syntax-
only compilation with debug memory/run/wrapper/rename/assignment branches passes0.49s;
Assignment debug memory/wrapper/rename passes0.46s. No whitespace errors. The RHS
header adds a nonvirtual payload-transfer method; no new fields or virtual slots.
Consistent source rebuild completed; other platforms/external providers unqualified.

EquationWrapper, passive actual object/callee/scope/runtime binding, native result
ownership and general nested/interior aliases remain to implement. MathElement's
copied refObject and FunctionRunner environment remain borrowed pending owner binding.
SetElementWrapper retirement, GetMathTree ownership transfer, unknown providers and
exceptional allocation/Swap rollback are not qualified. Parser-internal failures are
not covered by the post-parse RAII owner. No expression form is excluded or replaced
with a frozen constant. Every original broad resource/command/private/global/body/
kernel/NAIF/owner/later/recursive/exceptional/scientific obligation remains.

Exact preimages/build/final hashes/indexed patches/source map/notes/logs/receipts/
summary/planning preimages are under
``build/nasa-coverage/coverage95-20261106-implementation-rhs-expression-ownership``.
No tests/GUI/mission/function/model/solver/numerical/acceptance reader/seal execution.
No installed/shared runtime/package/commit/push changes. Tracking-v38 stays1093/1158
(94.39%); recorded source-versioned verified baseline1053/1158(90.9%),all105 original
acceptance rows open. Assertions, fixtures, texts, tolerances, fingerprints,
denominator, ledger and gates retained. No broad row credit. Goal active.
