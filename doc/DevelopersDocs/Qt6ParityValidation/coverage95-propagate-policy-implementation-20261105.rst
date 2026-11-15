Propagate grouping implementation, 2026-11-05
===========================================

The command now offers explicit ephemeris-count policies alongside the current
behavior. This implements both interpretations of the historical FRC!E114
wording without silently choosing whether a propagator means a configured setup
or a command group. The user previously requested support for both current and
worksheet behaviors. Original requirements, fixtures and acceptance decisions
are unchanged.

EphemerisPolicy
---------------

* Native is the default and retains the existing supported grouping behavior.
* SingleSetup permits zero or one distinct configured ephemeris PropSetup.
  Multiple groups can reference that same setup.
* SingleGroup permits zero or one ephemeris group, including when two groups
  would otherwise reference the same setup.

Pure numerical commands remain valid in every policy. Ephemeris providers are
identified through the native EphemerisPropagator type hierarchy. Their
configured setup pointers are counted before per-group runtime clones are
created; aliases of one resolved setup do not create distinct setup identities.
Independent local setup objects count separately. Existing provider rules, such
as one spacecraft in each SPK group, remain in force. Numerical integrator
groups do not enter either count.

The native enum metadata, setter/getter, copy/assignment, parser and serializer
are connected. The field can appear with stopping conditions or by itself for
a single step. Quoted and unquoted enum values are accepted; duplicate fields,
unknown values, incomplete quotes and whitespace inside quoted enum values are
rejected. Reinterpreting a command resets an omitted policy to Native. An
explicit Native choice is retained during native serialization.

Configuration changes invalidate initialization and are rejected by the setter
during active propagation. Strict-policy initialization first invalidates the
old readiness flag and checks resolved setups before runtime cloning. A failed
count check therefore cannot leave the command marked initialized from a
previous configuration. The shared policy error routine is used by native
initialization and the Qt configured-command check.

Qt integration
--------------

The Propagators and spacecraft dialog exposes Ephemeris limit choices for
current behavior, one distinct setup and one group. Its combo reads the native
enum values. Changing only the policy edits only its source value, preserving
quotes and the surrounding command. Adding a policy preserves existing stop
and color fields. The compact propagation form, stopping-condition dialog and
color editor recognize and retain the field, including policy-only single-step
commands. An existing policy can keep otherwise inactive StopTolerance text
inside valid option braces, as an orbit color already did.

Source/native command matching retains the policy when normalizing single-step
options and treats quoted/unquoted enum spellings alike for stopped commands.
This avoids dropping the new option from a command key or losing its association
with the native command after serialization. These are source implementations;
GUI interaction and source/history behavior remain unverified in this phase.

Numerical group capacity
------------------------

The owning-source review for FRC!E113 found existing dynamic storage throughout
the group path: ConfigurePropSetup/SetObject append to propName/satName,
Initialize iterates all names and creates a setup per group, propagation
preparation builds p/fm/psm/epoch collections from the complete setup list, and
stepping/publication traverse those collections. Publisher buffers use computed
sizes. The Qt group table likewise appends rows without a product-level count
limit. The fixed two-element stopping brackets and optional eleven-body debug
array are not propagator-capacity limits.

No arbitrary numerical group cap was found in this path and none is introduced
by EphemerisPolicy. This is the source implementation of dynamic capacity,
subject to available memory and native container/index ranges. It is not a
claim that finite execution proves mathematically unbounded capacity or that
all resource-exhaustion/stress behavior has been qualified. The original
FRC!E113 acceptance gap remains intact.

Implementation tracking and deferred verification
-------------------------------------------------

The provisional engineering tally adds the existing dynamic-capacity row
FRC!E113 and the new compatible ephemeris-count row FRC!E114. It becomes
1082/1158 (93.44%): 1053 retained qualified instances plus 29 additional rows
with implementation present. Of the additional rows, 24 are new/compatibility
implementations and five are existing implementations. This remains an estimate,
not a completed source audit or acceptance credit. The other 76 original open
rows are not automatically 76 absent features.

The recorded verified baseline remains 1053/1158. The original 105 acceptance
rows remain open, and qualification gates remain unchanged. The current tally
is in ``build/nasa-coverage/coverage95-20261105-implementation-progress/tracking-v2.json``.

Configured private builds pass: v1 82 steps/139.05 seconds; v2 five steps/3.81
seconds; v3 66 steps/5.30 seconds. Dependent base/Qt/console/plugin/Python images
were rebuilt together. Existing numeric parameter IDs, including the reserved
OrbitColor slot, are preserved; a new enum ID is appended and Propagate gains
configuration data members. No installed/shared runtime or public package was
updated. Compiled/linked test/probe targets were not executed.

Later verification must cover all policies with zero/one/multiple ephemeris
groups, repeated versus distinct setups and aliases, mixed/numeric-only groups,
provider-specific restrictions, stopped/single-step/backward/synchronized forms,
copy/assignment/reparse and initialization failure, native serialization and
quoted values, duplicate/malformed input, GUI Apply/Cancel/Undo/Redo and source
preservation. Capacity/stress and scientific/physical checks remain deferred.
No new test, model, GUI, renderer, numerical or qualification run was performed.

Stage:
``build/nasa-coverage/coverage95-20261105-implementation-propagate-ephemeris-policy``.
