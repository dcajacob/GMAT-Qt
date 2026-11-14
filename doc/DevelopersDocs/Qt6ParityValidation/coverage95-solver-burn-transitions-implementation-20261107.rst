Solver finite-burn execution checkpoints
=======================================

Solver checkpoints now capture the actual borrowed transient-force selection and lifetime identities. Reset stages that selection before native probes/copies, restores spacecraft maneuvering and thruster firing flags, and swaps the original force pointers back after value restoration. Branch transient context forwarding now sets the branch itself as well as its children. Propagation preparation compares list context, identity, order and lifetime, including equal-count changes, before reusing model/state bindings; successful preparation captures the selection and buffer retirement clears it. Publisher and subscriber cached maneuvering flags are staged with the checkpoint and restored silently, without emitting another maneuver event or changing output histories. Surviving subscriber identities retain captured flags; newly subscribed objects use the restored publisher flag and retired objects are not accessed. Ordinary spacecraft copying still preserves the destination maneuvering flag. Existing role/type/origin/state/covariance guards and all numerical routines remain. Compilation only; no row or verification credit.

Source findings
---------------

Native Thruster assignment previously rejected a changed firing flag under restoration. Spacecraft restored its pre-copy maneuvering flag even under the native restoration guard. Solver checkpoints stored spacecraft/formation values but not the active transient selection. BranchCommand forwarded the selection to children without setting its inherited context. Propagate used only a force-count comparison; replacing one active force by another could retain the same count. These owning paths motivated the changes.

Implementation
--------------

Solver checkpoints now capture the actual borrowed transient-force selection and lifetime identities. Reset stages that selection before native probes/copies, restores spacecraft maneuvering and thruster firing flags, and swaps the original force pointers back after value restoration. Branch transient context forwarding now sets the branch itself as well as its children. Propagation preparation compares list context, identity, order and lifetime, including equal-count changes, before reusing model/state bindings; successful preparation captures the selection and buffer retirement clears it. Publisher and subscriber cached maneuvering flags are staged with the checkpoint and restored silently, without emitting another maneuver event or changing output histories. Surviving subscriber identities retain captured flags; newly subscribed objects use the restored publisher flag and retired objects are not accessed. Ordinary spacecraft copying still preserves the destination maneuvering flag. Existing role/type/origin/state/covariance guards and all numerical routines remain. Compilation only; no row or verification credit.

The new internal TransientForceSelection owns only a pointer/weak-identity record and selected list address. Capture stages allocation before replacing its prior record. Preparation validates the selected list address and every captured object lifetime before live checkpoint mutation; it creates no force clone or name-based fallback. Native restore applies a staged vector swap after the existing probes and native copies. Weak identities prevent address reuse from matching an earlier force.

Publisher capture records its own flag and each actual subscriber identity/flag. Preparation allocates the retained snapshot before live restoration. Apply updates only the current subscription list, matching captured flags by live identity and using the captured scene flag for new subscribers. It does not invoke HandleManeuvering, SetManeuvering, Publish or any output/history callback.

Limits and compatibility
------------------------

This checkpoint captures force membership and cached execution flags, not arbitrary force internal definitions, dependencies or owned topology. Original force/list and publisher context must survive. Replacing an expired force by name is forbidden. Native copy failures still mark providers unready; this is not a general transaction rollback for every provider. Subscriber flags are restored silently, while existing history/stream/output policies remain; event-history and final/trial/nested-output behavior require later verification. PropagationEnabledCommand gains selection storage in its instance layout; matching rebuilt native clients/plugins are required. Publisher methods are nonvirtual; no Subscriber instance fields or virtual contracts change. No new resource parameter IDs, equations, numeric tolerances or file/service initialization policies.

Compilation and retained evidence
---------------------------------

Build v1 returned 0 in 30.86s, progress 87/87, 0 compiler deprecation warnings; no compiler errors. Build v2 returned 0 in 106.04s, progress 294/294, 1 compiler deprecation warnings; no compiler errors. 

Both sessions are terminal. Final source hashes match build v2. Nine existing source/help files and one new header were captured/planned before edits, alongside thirteen reference files and four planning snapshots. The publisher follow-up promoted its previously captured .cpp reference to source and captured two untouched header preimages after terminal v1, before changing them. The final packet contains thirteen source/help paths, one new header and twelve unchanged references. Indexed/combined patches, hashes and compiler receipts are retained. Both help XML files parse and added/changed lines have clean trailing whitespace. Source comparison proves the Spacecraft and Thruster assignment changes are precisely the restoration-policy lines; their remaining algorithms are identical to their preimages.

No new tests or runtime cache/burn/loop/function/subscriber/helper/MATLAB/GUI/model/mission/numerical runs, acceptance auditors/readers/seals or evidence admission occurred. Original requirements/text/fingerprints/fixtures/assertions/tolerances/denominator/gates and dispositions remain unchanged. Tracking-v125 retains 1093/1158 (94.39%), 65 unresolved engineering rows; recorded source-versioned verified baseline remains 1053/1158 (90.9%). All 105 original acceptance rows remain open. Full implementation audit and goal remain incomplete.

Next implementation
-------------------

Continue changed owned spacecraft/hardware/force/measurement graphs, declared inputs and public/borrowed interpolation/Hermite extents. Finish force internal-value/dependency and shared-owner lifecycles, nested function/global/solver/MATLAB contexts, and complete subscriber/output-history semantics. Then complete all remaining original worklist scope before runtime verification. Preserve the original assertions, tolerances, denominator and acceptance dispositions. Deferred checks include begin-only/end-only/balanced burns, initially active burns, nested loops/functions, same-count force switches, list/force retirement, subscriber churn, allocation failure and numerical/output equivalence.
