Function-local navigation and sandbox event context
===================================================

Three new function workflows and three affected regressions pass unattended
and visibly: **12 GUI passes**, comprising six new and six regression passes.
The new workflows execute **12 function-local navigation pipelines**, including
no-Build repeat runs, and preserve independent caller spacecraft and sentinels.

Coverage remains **1,019/1,158 (88.0%)**, 139 original requirements open and
**82 more needed** for 95 percent. Historical coverage remains
**1,816/2,367 (76.7%)**. These are bounded resource/command constituents, not
whole FRR!E491/E493/E477 qualification. Original fingerprints, the denominator,
all 59 prerequisite holds and separate scientific/core/current-build/full-NASA
gates remain unchanged. The full goal remains active and incomplete.

New executable obligation
--------------------------

The older function resource tests established selected creation, copying and
reference behavior. They did not execute a complete private navigation graph.
``NasaFunctionNavigationScopeTests.cpp`` now creates and uses a simulator,
ExtendedKalmanFilter, smoother, tracking sets, receiver/antenna/error model,
spacecraft, process-noise configuration, forces, propagation and report objects
inside a real GMAT function. It reuses the unchanged navigation fixture and
its noiseless 7,000 km point-mass circle. Unused declarations, such as FogmCd,
do not become provider-execution evidence merely because they are present.

The three contexts are a direct function call, a nested call with the pipeline
inside an If branch, and a function call used as an assignment expression.
The actual function resource/file editor receives keyboard edits, Cancel and
Save; drafts preserve the external file, caller source/history and native
prototypes. Unicode save/reopen preserves the full caller graph. Each window
runs twice, with no Build before its second execution. Actual output-tree
report viewing matches the checked raw bytes.

Same-name caller spacecraft remain at X=8,100 km while the local results follow
the independently specified circle. Local Keep=41 and caller Keep=103 remain
distinct. A returned 3-by-8 array carries full states, epochs and sentinels;
the configured caller prototypes remain byte-identical. PropSetup and coordinate
systems retain their documented automatic-global semantics; this test does not
assert that they are private or prove every automatic-global lifetime case.

Core defect and exact repair
----------------------------

The original native function run failed during initialization: RunSimulator's
Event Manager pointer was unset. Sandbox initialized main-sequence commands
with its trigger-manager clones, but its GMAT-function preparation path omitted
that binding.

``src/base/executive/Sandbox.cpp`` adds six lines after function preparation.
Every function control sequence receives this sandbox's trigger managers,
including a previously built sequence on reuse. The existing BranchCommand
setter forwards that same borrowed context to nested children. Existing nested
function discovery handles deeper calls. Ownership stays with the sandbox;
there is no singleton fallback, new manager allocation, class-layout, virtual
API or numerical-formula change.

``Sandbox.before.cpp`` and ``core-v6.patch`` retain the exact preimage and
increment. The focused core inventory records **151 pending core paths** for
technical review; it does not certify them or establish exclusive authorship
of earlier changes. The three affected regressions are mutual function recursion,
automatic-global propagator use and the existing main/Script Event navigation
workflow. All pass in both modes. Broader function/resource qualification and
technical/scientific approval remain open.

Independent audit and retained failures
---------------------------------------

``AuditFunctionNavigationScope.py`` checks all raw continuation/returned/state
reports: **732 report scalars**, **132 GPS observation rows** and **792 filter/
smoother component rows** across twelve pipelines. The state checks retain the
original independent circle, epoch and sentinel bounds. GPS data retain exact
record/type/sensor/epoch checks and the independent circle-norm bound; filter
and smoother reports retain all observed/computed/residual components and
their original residual and consistency bounds. This is not independent
qualification of every measurement-frame direction or noisy-estimation model.
Five damaged/incomplete proof controls are rejected.

All four failed native attempts remain source-versioned:

* v2: the new driver did not reveal the scrolled file button;
* v3: complete caller identity changed after reopen;
* v4: a diagnostic repeat established that only automatic NAIF/frame IDs changed;
* v5: the actual missing sandbox Event Manager binding failed the mission.

The fixture now assigns explicit IDs while retaining the complete identity
assertion. No numeric oracle or bound was relaxed. The first compiled source's
observation-line parser was corrected before any native execution. The optional
family argument and diagnostic booleans reexpress two protocol predicates;
all other original new-driver predicates remain verbatim. Existing original
function/backend tests are unchanged. Exact source comparisons are retained.

The first direct unattended pass (v6) is reused. V7 runs only the two additional
contexts and three affected regressions unattended; the visible invocation runs
all six cases for their first visible checks. Source archives recover one old
TrackCoreChanges metadata version by exact hash from the preceding capsule;
no historical receipt or reference identity is rewritten.

All six visible processes are mapped to exact test/core/provider image hashes,
with unchanged before/after inputs. Windows start minimized and remain resizable
and non-fullscreen. Zero screenshots or runtime/kernel copies are retained.
All build/native handles are terminal. The evidence audit, regular-file manifest,
stage receipt, exact source versions and replay logs live under
``build/nasa-coverage/coverage95-20261103-function-navigation/``.

Replay and next scope
----------------------

Build and run the new registrations only when new/affected work warrants it::

   cmake --build build/linux-gui --target GmatQtNasaFunctionNavigationScopeTests -j4
   python3 src/qtgui/tests/RunFunctionNavigationScope.py
   python3 src/qtgui/tests/RunFunctionNavigationScope.py --visible

``--with-regressions`` adds the three named affected cases. Both existing
acceptance build targets include the new executable. Read-only audit::

   python3 src/qtgui/tests/AuditFunctionNavigationScope.py \
     --stage build/nasa-coverage/coverage95-20261103-function-navigation \
     --output /tmp/function-navigation-audit.json

Next complete the resource/type/ownership/global-eligibility census for the
exact E491 conjunction. Reuse these navigation pipelines, the older 28 resource
class output/reference witnesses, built-in functions and automatic-global
proofs. Add only uncovered classes and ownership mechanisms. The old 83-entry
GUI catalog is a recorded candidate inventory, not proof of every scriptable
or hidden native resource. E477's separate SaveMission restriction and E406's
terminal non-solution output conflict remain unresolved; this repair changes
neither preserved expectation.
