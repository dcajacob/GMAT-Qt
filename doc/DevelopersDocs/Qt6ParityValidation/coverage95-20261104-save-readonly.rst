95 percent campaign: read-only reconstruction and subscriber repairs
==================================================================

The complete new initialization component passes 18 affected resource cases in
both headless and initially minimized, resizable, nonfullscreen visible profiles.
It checks 130 new read-only field observations, reuses 132 captured post-reload
observations, and retains 1922 writable-property comparisons. Direct native
subscriber regressions pass 166 assertions. Two demonstrated core defects are
repaired without changing original assertions, tolerances, public declarations
or class layouts.

Original FRC!E132 remains open: "The system shall have a command named Save that
allows users to save any Resource object to a file." Its fingerprint remains
``5546e993341e14217244951300a7744969787f96fb0af50bf78a3aa60037dfe3``. This
component does not substitute a smaller requirement. Canonical coverage stays
1053/1158 (90.9%), with 105 open and 48 needed for 1101. The original 59 hold
identities and existing dispositions, 37 MATLAB identities and separate
historical 1816/2367 coverage remain unchanged. FRC!E135's filename-format
conflict, scientific/core/external ABI/current-build/platform/full NASA gates
and the overall goal remain unqualified.

Read-only inventory
-------------------

``build/nasa-coverage/coverage95-20261104-save-readonly`` is the stage.
``AuditSaveReadonlyInventory.py`` replays the 152 retained selected snapshots
without native reruns. It uses native descriptor IDs in addition to labels,
preserving the distinct inherited Array.InitialValue descriptors. It inventories
8509 observations: 7163 equal values, 153 changed values and 1193 unavailable
values. There are 1429 read-only descriptor identities, 76 with changed values
and 238 with unavailable observations. The earlier label-only inventory had
merged one duplicated label.

The 153 changes are partitioned without discarding any:

* Existing reload probes already reconstruct 66 observations: 62 fixed-offset
  attitude aliases, two GMAT function signature fields and two file-backed
  region coordinate arrays. Both profiles supply 132 reusable checks.
* Eighteen cases require 65 new post-reload initialization/consumer checks,
  totaling 130 observations across both profiles.
* Twenty-two initializer-history, session-ID and body-number observations
  remain pending owning-source/consumer review.

Neither equal default values nor read-only/cloaked metadata prove a field is
transient. No field is excluded as transient by this audit. In particular,
GmatBase's generic Covariance getter can be unavailable while the native
GetCovariance object contains state. That native-API/alias obligation remains
open, along with other missing getters and the complete Resource/reference,
attitude, anomaly/frame/body, file initialization and dependency scopes.

New native consumers
--------------------

The generator preserves every original resource setup, selection, definition
and literal witness, adding only a support variable and explicit read-only
requests. A real reload mission increments ReloadAlive from 41 to 42.
DynamicDataDisplay also executes its actual UpdateDynamicData consumer. These
missions reconstruct hardware rotation/FOV masks, formation and thrust-file
epochs, plot initialization values, tracking adapter configuration, automatic
display colors and output-file path state.

``NasaSaveReadonlyInitializationTests.cpp`` extends the generic Save driver by
four tagged insertions. Removing those insertions recovers the original generic
driver byte-for-byte. No original expression or assertion is rewritten. The
original editor, picker, empty guard, Apply, source/history, following statement,
native writable fields, standalone rebuild and probe assertions all remain.
Additional checks compare actual original and reloaded runtime typed getters,
descriptor identities and metadata, using the existing 1e-12 relative bound.
The ordinary proof parser accepts both original and additional terminal records.

Two actual core defects
-----------------------

The initial headless probes retain three failures; none was relabeled a pass.
Both OrbitView cases returned unstable runtime MaxFOV/InitialFOV integers. The
copy constructor and assignment operator omitted all three existing FOV members.
The repair copies MinFOV, MaxFOV and InitialFOV in both operations. Script lookup
for these deprecated labels still returns PARAMETER_REMOVED; no removed user
feature is restored. Native descriptor enumeration remains deterministic, and
copy/Clone/assignment/self-assignment/base-Copy preserve defaults and nondefault
native values.

TextEphemFile's constructor replaced its inherited filename with ``fname.data``
after ReportFile had cached a different default path. The public Filename was
``.data``, while the actual FullPathFileName still ended in Subject.txt. Export
and reload consequently changed its effective output path. The repair resolves
the already selected filename through the existing ReportFile setter and retains
the original default-name flag. The public filename and Save export convention
are unchanged; the effective path now agrees with the declared filename.

Only ``src/base/subscriber/OrbitView.cpp`` and
``src/base/subscriber/TextEphemFile.cpp`` change in this increment. Exact
preimages, hashes and ``incremental-core-v1.patch`` are retained. Both preimages
match the actually inherited frozen source. No numeric method, constant,
tolerance, declaration or class layout changes. The isolated old/new cores have
the same 19024 defined dynamic symbols, including the same 104 symbols for the
two classes. This is a mechanical check, not broad external ABI approval.

Source ownership and native results
----------------------------------

``build-v1`` owns the new driver/fixture and source-preservation receipt. Fifteen
cases pass both profiles on the existing source4 core/source6 frontend. Their
headless passes are reused from the initial3 and successful12 in the retained
15-attempt run. That run's three failing cases and nonzero aggregate status stay
explicit. The corresponding visible run passes15.

``core-build-v1`` freezes 1122 base/util source files, then overlays exactly the
two repaired CPP files. It builds GmatBase into its own ``build/images`` folder,
using the original base CMake source list and unchanged Util4. It does not
overwrite the shared application library or copy a runtime image. The original
shared Base4 SHA remains
``2d580483d9d46d1fdbfd5b9d0d36960e2e17dcc4b9fff617115596c340877c97``;
the new isolated image SHA is
``97306c99ff06f7ac59e2d3430708ef2409e95ecc49f504e5ac305d94b82d2ee1``.

Only the three failed cases run against this repaired image, in
``readonly-repaired-headless-v2`` and ``readonly-repaired-visible-v2``. Actual
process maps prove the new core is loaded and the old shared Base is absent.
The unchanged linked driver/frontend and Util4 remain hash-bound. Existing
source-versioned campaign passes keep their original core image and evidence.
The working application and public preview packages are not rebuilt or updated.

Native copy/path support regressions
-----------------------------------

``SaveSubscriberNativeTests.cpp`` tests constructor defaults, native custom FOV
values, copy construction, Clone, assignment, self-assignment and base Copy;
it also tests default/provided/explicit filenames and copied/assigned effective
paths. Its 166 assertions run without GUI contexts and map exactly the support
executable, isolated core and original Util4.

Retained support failures include a missing CSPICE include path in the first
standalone build and an incorrect new test's use of removed FOV string lookup.
The corrected regression enumerates actual native IDs and additionally asserts
that script lookup stays removed. Core code and original tests are not changed
to accommodate that test error. The final producer/execution are under
``native-support-v3``. All previous logs and recipes remain present.

Independent audit and replay
----------------------------

``AuditSaveReadonlyInitialization.py`` replays every new and reused witness,
original native assertions, producer, source archive, actual library map,
visible route and failure. A private instance of the unchanged generic reader
binds its source-path parameter to the actual derived CPP. Its original checks
are intact. The first new reader attempt did not model the optional archived
JSON null before_save value; the corrected adapter models exactly the compiled
Qt null-to-empty-string conversion for baseline expansion while comparing input
and metadata bytes unchanged. That failed reader/log remain retained.

The successful audit is ``evidence-replay-v2.json``. Use fresh output paths from
the repository root and replay evidence instead of repeating native passes::

   python3 build/nasa-coverage/coverage95-20261104-save-readonly/verify-component-v2.py \
     --output build/nasa-coverage/coverage95-20261104-save-readonly/component-replay-next.json
   python3 src/qtgui/tests/AuditSaveReadonlyInitialization.py \
     --output build/nasa-coverage/coverage95-20261104-save-readonly/evidence-replay-next.json

Main CMake contains 18 reusable ``QtGui.NasaSaveReadonlyInit*`` registrations and
the native ``QtGui.SaveSubscriberNativeCopyPath`` regression, with ordinary
observer/check/build dependencies. New native execution needs fresh guards and
the intended core version; old execution scripts deliberately bind their prior
registration/helper inputs. ``original-row-scope-v1.json`` retains the complete
FRC!E132 wording, fingerprint and remaining obligations. No original whole-row
credit or 95 percent completion is inferred from these component totals.

The final seal is ``component-replay-v2.json``: 420 frozen sources and 1040
direct artifact bindings, with manifest SHA
``84e90f938b4cdebe5267f6b0a329e4c5a5a55f8789de946857cc5abf4eefe373``.
The failed first seal is retained. Its sole mismatch was the frozen helper's
temporary extraction path. The corrected verifier validates that helper's exact
source digest against both capsule and prior evidence, relocates that one
artifact key, and requires byte-identical serialization of the complete ledger.
No native execution, original assertion, evidence value or tolerance changes.
