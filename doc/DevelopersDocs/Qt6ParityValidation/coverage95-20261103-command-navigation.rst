Navigation command operations: five contexts pass, nested filter failure retained
================================================================================

Coverage remains **1,019/1,158 (88.0%)**, with 139 original requirements open
and **82 more needed**. Historical operations remain **1,816/2,367 (76.7%)**.
No additional original key is admitted. The complete All Others!E196 conjunction,
all 59 prerequisite holds and scientific/core/current-build/full-NASA gates remain
open. This increment changes test/registration/audit sources only.

Exact new results
-----------------

RunSimulator Root and Branch, RunEstimator Root, and RunSmoother Root and Branch
pass unattended and visibly. Each uses the actual tree Open/reuse, double-click
Open, tree Close, before/after insertion, Delete Cancel/confirmation, Undo/Redo,
context Summary and Unicode save/reopen routes. Insertion/deletion preserve exact
unrelated source bytes and parent/order identity. Both restored mission phases
run independently.

RunEstimator Branch fails in both modes. The six ordinary registrations remain
unchanged, with **ten passes and two retained failures**. No expected-failure
annotation, fallback fixture, assertion relaxation or partial parent admission is
used. Visible windows start minimized and remain resizable/non-fullscreen; all
six processes are mapped. Zero screenshots and runtime/kernel copies are retained.

The five passing contexts supply **20 raw twelve-field result rows**, **360
summary state components**, **60 additional navigation state rows** and **1,320
filter/smoother GPS component rows**. Each summary is split into its three unique
spacecraft sections, and all states are independently checked at the command's
own epoch: Sat at twenty seconds, SimSat at six hundred seconds, and EstSat at
zero for RunSimulator or six hundred for RunEstimator/RunSmoother.

Independent references and preserved bounds
------------------------------------------

The retained navigation fixture is unchanged. Its force model has only Earth
point mass, mu=398600.4415, radius=7000 km and no measurement noise. Closed-form
circular states determine all inertial state expectations. The six-hundred-second
reference is [5586.094942828199, 4218.476418057735, 0, -4.547549691678059,
6.021852872331281, 0], with A1 epoch 25357.5073383609. Count progresses through
one, two and three after simulation, filtering and smoothing.

Original bounds remain 1e-4 s, 2e-5 km, 2e-8 km/s, 2e-10 days for navigation
report epochs, and 1e-12 for scalar fields. Summary display bounds remain 2e-5.
Each phase retains eleven sixty-second GPS observation records, filter and
smoother reports, and a warm-start file. Both reports must contain all three
components of all eleven records with the correct UTC identities, residuals
below 2e-5 and observed/computed vector norms within 2e-5 km of 7000 km.
These radius checks do not claim complete Earth-fixed attitude qualification.

Retained production defect and next repair target
------------------------------------------------

When RunEstimator EKF executes inside an If branch and RunSmoother FPS follows
the branch at root, the filter completes with the expected final state. The
smoother then reports:

  There is no forward filter information because no EKF estimation has been run
  before running RunSmoother

Both native engine logs retain this failure after Estimation Completed. The
failed case has no passing proof.json and supplies no admitted numerical rows.

RunSmoother::Initialize currently follows GetPrevious() siblings and never
descends into a preceding completed branch. The executed estimator is therefore
missed. RunEstimator::GetRefObject also returns its estimator without honoring
the supplied name, so a broader traversal must explicitly match the requested
filter. A repair must distinguish completed, skipped and future commands and
terminate safely across branch graph cycles; simply scanning every child may
select an unexecuted or stale filter. No speculative production patch was made.

The next bounded repair should preserve the five passing contexts, use the
unchanged failing RunEstimatorBranch registration, and add focused guards for
nested/Else branches, a skipped matching filter, a different named filter and
future commands before broadening the search. Retain exact production preimages
and deltas and rerun only the failed/new/affected cases. The previous source-
versioned passes remain useful even if a later plugin image changes.

Evidence audit and source versions
---------------------------------

The stage is build/nasa-coverage/coverage95-20261103-command-navigation.
Source-v1 preserves an initial test-driver compilation error caused by copying
an overly broad helper block. Source-v2 corrects the block selection; both native
runs execute source-v2. No assertion was changed after either native outcome.
The existing backend and lifecycle drivers are unchanged.

Audit v1 assumed a spacecraft presentation order absent from the native contract.
Its exact source, failed log and input manifest are preserved. Audit v2 checks the
complete unique named census without an order assumption and independently checks
every spacecraft's six values. It verifies both ordinary failures as failures,
all passing stdout/JUnit/raw outputs, source archives, runtime/process maps, and
four corrupt-proof controls. evidence-audit-v2.json reports
``audited-partial-navigation-operations-with-retained-failure`` with no qualified
keys. No GUI replay was needed for the offline audit correction.

The authoritative ledger remains
build/nasa-coverage/coverage95-20261103-default-orientation/coverage-snapshot-v1.json.
The 53-entry whole-parent worklist remains in the preceding command-operations
stage, bound by scope-continuation.json. SaveMission, structural/unique-starter
applicability and older per-class tree Close/context Summary gaps remain as well.
The full 95-percent goal is active and incomplete.

Replay
------

.. code-block:: sh

   cmake --build build/linux-gui --target GmatQtNasaCommandNavigationOperationTests -j4
   python3 src/qtgui/tests/RunCommandNavigationOperations.py
   python3 src/qtgui/tests/RunCommandNavigationOperations.py --visible
   python3 src/qtgui/tests/AuditCommandNavigationOperations.py \
     --stage build/nasa-coverage/coverage95-20261103-command-navigation \
     --output /tmp/navigation-operation-audit.json

The replay intentionally remains failing until the production defect is fixed.
Six individual QtGui.NasaCommandNavigationOperations<Family><Root|Branch>
registrations are included in the acceptance build/check dependencies.
