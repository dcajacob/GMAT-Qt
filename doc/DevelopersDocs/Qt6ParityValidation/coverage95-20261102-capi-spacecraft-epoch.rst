Public spacecraft epoch C API: two complete original obligations
==============================================================

The final audited union is **1,000/1,158 original requirements (86.4%)**,
up from998. Historical coverage stays **1,816/2,367 (76.7%)**. The95% target
requires1,101 requirements, leaving101 to qualify. Original source rows,
fingerprints, denominators and existing assertions are unchanged.

The two additions are ``All Others!E272`` (public spacecraft epoch) and
``All Others!E269`` (the finite parent of Cartesian state, STM and epoch
post-run access). E270/E271 were already qualified. E272's exact original
``DEFERRED:`` wording is preserved; actual implemented behavior supplies
the evidence, without claiming NASA changed its historical disposition.

Implementation and core review
------------------------------

``GetSpacecraftEpoch(const char *spacecraftName, double *epochA1Mjd)`` is
a real exported pure-C API. It copies the named run-sandbox spacecraft's
scalar A1 modified Julian epoch (JD minus2430000.0), returns a status,
preserves caller output on failure and describes the result through
``getLastMessage``. It does not select an ODE model or mutate spacecraft,
model state or configuration. Start/load invalidate access; a successful
initialization followed by completion, Stop or execution failure permits
access. Initialization failure does not expose a partially built sandbox.

Only these production source paths change in this increment:

* ``plugins/CInterfacePlugin/src/plugin/CInterfaceFunctions.h``:
  additive public declaration and documented units/lifetime/status contract.
* ``plugins/CInterfacePlugin/src/plugin/CInterfacePluginFunctions.cpp``:
  additive read-only getter and availability bookkeeping in Start/Load/Run.
  Existing statuses, return paths and state/model operations are preserved.

Exact pre-increment copies, per-file hashes and incremental diffs are retained
under ``build/nasa-coverage/coverage95-20261101/capi-spacecraft-epoch-core-before``
and ``capi-spacecraft-epoch-v4-core-changes.json``. The21 preceding Save core
paths remain unchanged. This is a C-interface plugin change; it adds no GUI
production code or propagation/force/time-conversion physics. Independent
implementation review is recorded, without blanket scientific/core approval.

Runtime verification and failures
---------------------------------

The final V4 external C11 client passes190 assertions,21 independent epoch
readings and nine RunScript calls. It exercises different fractional initial
epochs, forward/backward propagation, an unpropagated spacecraft, repeated
runs, PrepareMissionSequence, stopped and runtime-failed execution,
initialization failure, reload/restart and direct mission epoch assignments.
Null/empty/missing/case-mismatched/non-spacecraft inputs preserve caller output.
Reads preserve both selected42-element model states byte-for-byte and do not
change object count. Actual loaded project images, before/after inputs,
source archives, logs and JUnit are hash-bound; no private epoch probe is used.

Two failed attempts remain explicit:

* V1 used an initialization-error fixture rejected during loading. V2 changed
  the fixture to an empty force model, genuinely reaching RunScript's
  initialization failure.
* V3 added a direct mission-assignment assertion and exposed a defect in the
  new getter: unconditionally reading precision storage returned a stale
  epoch after a Real-valued assignment. V4 reads the documented scalar epoch
  used by the existing TimeData path. The assertion is preserved. The earlier
  V2 whole-row candidate and review are withdrawn and retained separately.

Only the affected existing basic/STM C clients were rerun, passing all133+481
unchanged assertions. These passes are reused after the getter-only correction:
the archived interface is byte-identical outside exactly the new getter body,
the public header is identical, legacy clients never call the getter and
loaded GmatBase/GmatUtil identities match. The auditor enforces that partition,
including a complete brace-delimited function and unchanged neighboring code.
This is explicitly a source-versioned conjunction, not a claim that every
test ran against one latest binary. Prior GUI suites were not replayed.

Eleven negative audit controls reject wrong/configured/stale/unit-shifted
epochs, missing cases or child/evidence seals, extra credit, blanket NASA
approval, old API status changes and hidden neighboring code. They execute
no additional missions. Independent review covers the corrected source and
final auditor, with exact hashes.

Reusable execution and evidence
------------------------------

Build and run the new acceptance case::

  cmake --build build/linux-gui --target GmatNasaCapiSpacecraftEpochTests -j4
  ctest --test-dir build/linux-gui --output-on-failure \
    -R '^QtAcceptance.NasaCInterfaceSpacecraftEpoch$'

This requirement concerns the native C ABI; it has no visible GUI interaction
and receives no GUI execution credit. Existing visible replay continues to use
resizable, non-fullscreen, initially minimized windows. No host windows are
raised by this case. No passing screenshots or runtime binary copies are kept.
The six actual C-client attempts retain about30MiB of compressed source/text
evidence, rather than engine installations.

The authoritative artifacts are under
``build/nasa-coverage/coverage95-20261101``:

* ``capi-spacecraft-epoch-qualification.json`` and
  ``capi-spacecraft-epoch-v4-coverage.json``: exact two-row admission and full
  union replay, using repaired immutable references for earlier qualifications.
* ``capi-spacecraft-epoch-v4-independent-review.json``,
  ``capi-spacecraft-epoch-v4-negative-controls.json`` and the frozen
  ``capi-spacecraft-epoch-v4-review`` auditor: bounded independent review,
  discriminating controls and reproducible admission logic.
* ``capi-spacecraft-epoch-final-audit.json``: terminal completion seal for this
  increment, not completion of the95% goal.

Remaining scope
---------------

All158 unqualified original requirements and551 historical cases remain in
the coverage report. Broader script-loading resource/command families, MATLAB,
historical platforms, unspecified hardware, missing optional models, complete
Save/resource scoping, integration-step sampling and source conflicts remain
open. The real GroundTrack boundary failure remains enabled and unchanged.

The previously recorded59 prerequisite holds are not administratively retired.
The optimistic all-non-held ceiling is1,099/1,158 (94.9%); reaching1,101 also
requires substantively resolving at least two holds. This increment provides
no GUI-wide, MATLAB, platform, scientific or full-current-build qualification.
The95% goal remains active.
