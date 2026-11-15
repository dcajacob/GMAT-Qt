Exact ephemeris counts and Propagate copy ownership, 2026-11-08
==============================================================

Propagate now offers ExactlyOneSetup and ExactlyOneGroup in script and the Qt
groups dialog. The former requires one distinct resolved ephemeris setup and
permits repeated groups to use it; the latter requires one ephemeris group.
Both reject zero ephemeris groups. Native, SingleSetup and SingleGroup retain
their existing behavior, including numerical-only commands. Canonical acceptance
remains 1078/1158 (93.1%); this component admits no complete requirement row.

The original single modes implement at-most-one, which did not enforce the
worksheet's nonzero one-and-only-one count. Two explicit choices now cover that
meaning without changing default missions. Setters validate the enum before
references are bound; initialization validates the actual resolved setup/group
counts before per-group runtime copies. Numeric groups do not enter the count.
Distinct setup identity uses the resolved object, including same-name independent
objects. Existing unknown-reference, provider and active-propagation restrictions
remain. The GUI parser, choices, validation, source matching and help recognize
both new values. Existing choices now say At most one to distinguish them.

The first eleven-case witness passed only its old-policy compatibility case.
After implementation, count/syntax/runtime checks exposed two separate issues.
The new affine SPK fixture unintentionally used the documented EphemStart
default while expecting the spacecraft's epoch. Setting FromSpacecraft makes
the intended epoch explicit; four native SPK execution cases then passed at
unchanged 2e-5 km and 2e-9 km/s bounds. The observed earliest-coverage failure
under EphemStart remains an open provider-boundary issue, not a repaired one.
Its original script, kernel and SPICE error are retained for the next stage.

Native copy destruction also crashed with the existing Native mode. GDB and
the owning source showed that Propagate's satellite-name arrays were shallow
copied even though each command destructor deletes them. Copies now stage and
own independent membership arrays; assignment stages replacements before
releasing its previous arrays. PropagationEnabledCommand assignment now invokes
GmatCommand assignment to copy command configuration, including comments.
Runtime initialization stays cold. No public data fields, virtual slots or
numerical equations change.

An initial new assertion incorrectly expected assignment to copy the source
instance name. GmatBase explicitly preserves the destination name. That new
witness assertion was corrected to the native contract: copy construction keeps
the source name; assignment keeps the destination name while copying comments,
policy and configuration. The failed phase and correction are retained explicitly.
No protected original assertion was changed. The original empty assigned name
observation alone is not evidence of a product defect.

The final native witness passes 12/12. It includes 35 count-matrix observations,
quoted/unquoted serialization, constructor/assignment configuration and safe
destruction after source-mission rebuilding, zero rejection with invalidated
readiness, and four actual SPK missions: forward, backward, repeated setup and
mixed numeric/SPK. Five affine state outputs have maximum observed position and
velocity errors 9.10e-13 km and 4.45e-16 km/s. Those results are for the explicit
FromSpacecraft fixture and do not qualify default EphemStart boundaries.

Six GUI cases pass in each profile: the new exact-policy workflow, unchanged
ImplementationPropagatePolicy, and all four unchanged 16/128-group capacity
cases. The new workflow checks both exact modes, zero rejection/recovery in the
actual dialog, Cancel, Apply, idempotence, quote/source preservation, Undo/Redo,
Unicode Save/Save As/reopen and actual SPK completion. The capacity cases check
two spacecraft per group, nine numerical methods, independent/synchronized and
forward/backward stepping, real table edits, unique native group clones,
independent circular-state/raw/native/viewer results and source/history isolation.
These bounded runs do not prove literal unlimited memory or resource capacity.

All 14 original FRC-10 context rows and all 71 positional FRR-13/14 rows remain
retained, including six nonstandard/deferred IDs omitted by a prefix-only lookup.
The source path still uses dynamic group storage with no new numerical group
cap. Complete provider/domain/science/copy/stress/platform/current-build and full
parent conjunctions remain open. All 1334 protected original inputs remain
unchanged. All final build/test/debugger handles are terminal.

Stage: build/nasa-coverage/coverage95-20261108-propagate-capacity-policy-current-review.
Continue autonomously with a current reproduction of the retained SPK
EphemStart coverage-boundary failure. Do not drop it because the count fixture
now specifies its intended FromSpacecraft epoch, and do not widen tolerances or
silently clamp genuine outside-coverage requests.
