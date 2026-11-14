Formation clone and sharing policy
==================================

The existing Formation sharing policy is now exercised through the actual
Resource Tree context menu and Clone dialog. A populated Formation can be
copied with the same spacecraft references when both formations allow shared
members. Without that permission, Clone reports the conflicting member and
leaves source, history and objects unchanged. One-sided permission is rejected.
No production behavior was changed by this batch.

Four GUI cases pass across the headless and visible profiles. The shared cases
also execute two missions each: propagation through the exact copy, followed by
changing only the copy's member and propagating that member. Independent circular
orbits check all four spacecraft and their clocks, including untouched nonmembers.
Original configuration, exact clone source, undo/redo and Unicode save/reopen are
checked. The exclusive headless pass is reused at its recorded source version.

Three failed development receipts remain. The initial fixture compared automatic
per-instance spacecraft identifiers across a rebuild. A diagnostic version
recorded that exact change; explicit identifiers corrected the fixture. A later
check used script braces in the comma-separated member controller input; correcting
the input completed the independent-edit witness. Numerical expectations and
original protected assertions were not weakened. No product defect is claimed
from those fixture errors, and no full regression ran.

All Others E156/E157 remain unqualified while interpretation of the old universal
cloning wording is pending. The proposed rule preserves existing native resource
protections and explicit sharing consent; the tests do not themselves approve
that interpretation or establish every resource's complete clone behavior.
Canonical coverage remains 1,111/1,158, with ten of eighteen agreed items complete.
The source-versioned evidence stage is
``build/nasa-coverage/complete18-20261112-formation-clone-v1``.
