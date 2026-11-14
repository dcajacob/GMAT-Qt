Live caller resumption for nested function calls
================================================

A private child call previously captured every eligible caller local, restored those values with Copy and reinitialized its caller function. That could replace owned components, require unrelated locals to support value capture, reset live loop/solver state and repeat explicit output writes.

The new internal live frame preserves the actual caller object graph and command sequence. Native nested entry chooses it when the child uses an independent store or will allocate a fresh one; scoped recursive function entry already starts with a cold private store. The frame records the current dictionary and its exact managed registration generation. Return verifies the same generation and consumes only the matching map/value frame. It performs no caller Copy, command Initialize, service rebinding or second output assignment. The existing matching failure guard consumes the same live frame after child cleanup/function restoration. Map/value stack publication remains atomic.

Input argument capture and formal materialization still use independent typed values; genuine Global effects and explicit output writes keep their ordinary native behavior. Explicit outputs are already assigned through their captured caller wrappers before stack return. Calls with no explicit outputs also preserve every unrelated caller local and execution handle. No frozen models, alternate solvers or substitute numerical algorithms were added. Caller-chain traversal uses actual manager/store identities and rejects cycles. Private argument initialization refuses an unexpectedly shared active caller dictionary before writing argument values.

Remaining scope
---------------

Explicit legacy/shared-map API calls retain the existing full value-snapshot path; their complete ownership and live resumption are still required. Managed generation checks do not qualify unregistered dictionaries, direct destruction, engine reset or concurrent/later callbacks. Global subscriber and service bindings previously refreshed during caller Initialize still need a dedicated scoped restoration path. Explicit output Copy can replace owned resource components, so untouched caller resources and command caches may still need passive reference rebinding; this batch does not claim those routes complete. Output assignment failure retains native explicit-output side effects rather than promising a new transaction guarantee. Complete input/resource providers, command/plugin copy/binding protocols and every original function I/O/core/scoping and any-command clause remain open.

Complete scoped caller subscriber/global-service binding restoration without command reinitialization, and passive caller resource/command-cache rebinding when explicit outputs replace owned components, preserving live solver/loop state. Finish legacy/shared/external-map ownership and all direct/finalization/custom-manager reentry; complete every command/plugin cold-copy, ownership, reference and passive interpreted function value-provider route. Continue all remaining original worklist requirements, including solver/estimator/simulator/subscriber/interface/measurement providers, body/kernel/NAIF/scientific clauses, output-name/cache transactions, inout aliases, custom vectors and arbitrary scope/interior/service/engine-reset/later lifetimes. Preserve full original scopes and verification deferral; continue independent POV and unresolved rotation research without inventing specifications.

Compilation
-----------

v1: 89/89 steps, 230.64 seconds, return 0.

The final configured core/Qt/console/enabled-plugin/Python-consumer build passed. Private stack-frame data and nonvirtual helpers changed; matching configured consumers rebuilt. Source preimages, before/build/final hashes, indexed deltas, terminal receipts and four planning preimages are retained in ``build/nasa-coverage/coverage95-20261106-implementation-live-caller-resumption``. Added-line whitespace is clean.

No runtime tests, GUI, missions, function/model/solver execution, numerical comparison, acceptance readers or seals ran. No packages, installed/shared runtime, commits or pushes. Original assertions, fixtures, tolerances, source texts, fingerprints, denominator, ledger and gates remain unchanged.

Tracking-v59 remains 1093/1158 (94.39%) implementation present and 65 other rows. Recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full implementation goal remains active and incomplete.
