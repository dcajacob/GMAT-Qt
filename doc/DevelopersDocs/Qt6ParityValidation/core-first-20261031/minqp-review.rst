MinQP: source-bound author review
=================================

The two preexisting changed hunks in MinQP.cpp were reviewed against the preserved
HEAD/current source and Yukon::ComputeSearchDirection. No additional production
change or demonstrated mis-solve is claimed. The file remains byte-identical to
the goal-start 97-file inventory. This is author analysis, not independent
scientific approval or whole-file solver qualification.

``build/release-check/20261031-core-first/review.json`` binds the HEAD, goal-start
and final hashes, the exact two original hunk headers, caller hashes, assertions
and remaining domains. The original patch is preserved as
``build/release-check/20261031-reviewed-candidate/core-inventory/files/20/change.patch``.

QP-WORKING-SET: original hunk at 330
-----------------------------------

Phase I expands each finite inequality side into a separate row, followed by its
nonnegative-slack row. Its indices therefore differ from the original QP's
indices. The changed recursive MinQP construction passes an empty inequality
working set instead of transferring original indices into that expanded space.
This is consistent with a cold phase-I start. The decision vector is extracted
without the slack, and the subsequent existing code rebuilds active original
inequality rows from the original bound errors.

Yukon constructs this QP from normalized current Jacobians and original bound
residuals, with an empty outer working set. It subsequently checks and restores
original unmerged constraint identities before accepting a successful direction.
The elastic fallback uses the same QP entry point. These callers support the
reviewed cold-start use; they do not establish arbitrary hot-start correctness.

QP-BOUND-ENCODING: original hunk at 1679
---------------------------------------

For an original finite lower bound ``a*x >= l``, the new Phase-I row is
``a*x + s >= l``. A finite upper bound ``a*x <= u`` becomes
``-a*x + s >= -u``. Both are lower-bound rows with unbounded upper sides.
A two-sided original row supplies both. Equality rows retain their exact lower
and upper value with a zero slack coefficient. The last row requires ``s >= 0``.
The linear objective minimizes s and its Hessian remains zero.

For each equality-feasible x, the minimum admissible s is therefore
``max(0, l-a*x, a*x-u)`` across the finite inequality sides. This avoids the old
upper-bound formulation, in which increasing positive slack tightened an upper
bound. Row count, explicit zero initialization, signs and finite-side conditions
match that construction. This algebra does not establish numerical robustness of
the surrounding active-set algorithm, equality projection or failure handling.

Existing proof reused
---------------------

The triage's twelve YukonParallel passes were inspected without replay. Eleven
solve analytic one-variable problems and check original constraints independently;
ElasticSlackEndpoints checks NLPFunctionGenerator directly, not MinQP.

Additional existing evidence was found: ``QtAcceptance.YukonQP`` in the preserved
support.xml. Its seven direct convex cold-start cases check known optima and all
original bounds for lower, upper, two-sided-below/above, scaled linearization,
large-slack/narrow-interval and equality-plus-bounds problems. Its source compiles
MinQP.cpp directly, so it adds useful evidence beyond the Yukon wrapper. Its two
empty-elastic helper checks concern NLPFunctionGenerator and add no QP optimum.
All sources, bounds and original passing receipts are unchanged.

Named gaps
-----------

Retained cases do not qualify arbitrary infeasible problems or Phase-I
failure/flag/multiplier output; hot/warm working sets; interleaved equality and
inequality identities; inherited historical slack-index cleanup; rank-deficient
or overdetermined equalities; zero/nonfinite rows; ill-conditioning; indefinite
Hessians; all problem dimensions or broad nonlinear dynamics. These remain direct
review/test domains, not new proven defects or implicit scientific approval.
