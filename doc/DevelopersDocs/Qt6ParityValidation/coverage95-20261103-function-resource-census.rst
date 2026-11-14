Native function-resource census and model Copy repair
=====================================================

The loaded native factory census contains **566 distinct entries in 36
categories**. Two additional resource workflows, Plate and FirstOrderGaussMarkov,
pass in both GUI modes after repairing FirstOrderGaussMarkov's missing
polymorphic Copy implementation. The stage has **eight fully bound GUI passes**:
six new passes (including the two census executions) and two affected
EstimatedParameter regression passes. A separate native support test passes
392 exact covariance-component copy comparisons and two invalid-source guards.

Coverage remains **1,019/1,158 (88.0%)**, 139 original requirements open and
**82 more needed**. Historical coverage remains **1,816/2,367 (76.7%)**.
No exact requirement is newly qualified. All original fingerprints, assertions,
whole-row conjunctions, the denominator and 59 prerequisite holds remain.
Scientific, technical-core, full-current-build and full-NASA gates remain false;
the full goal is active and incomplete.

Complete inventory, incomplete eligibility matrix
--------------------------------------------------

``NasaFunctionResourceCensusTests.cpp`` captures both
``Moderator::GetAllObjectTypeArrayMap()`` and ``GetListOfAllFactoryItems()``
from a built native MainWindow. It records category IDs and registry labels,
all creatable names, per-category viewable/unviewable flags, first-category
name resolution and duplicates. The flattened map and all-factory list agree:
566 entries, 566 unique names, zero duplicates, including the empty DataFile
category. Headless and visible census bytes agree. Source, history and configured
object names remain unchanged; the census constructs or executes no resource.

The older 83-entry ResourceTree catalog omitted both new tested classes. GUI
listing therefore cannot bound the main-resource/function requirement. Registry
category 166 is labeled UnknownObject but retains FileInterface,
ThrustHistoryFile and ThrustSegment. These entries are neither dropped nor
declared unsupported because their category label is missing.

``factory-scope-matrix-v1.json`` retains every factory entry with
``qualified=false``. Commands have separate scope; the fourteen Propagator
implementations and twenty-two AxisSystem implementations have source-backed
unnamed/owned construction routes. Their owner/function obligations remain
open. The script spelling Propagator maps to PropSetup in ParseDefinitionBlock;
existing automatic-global propagator proofs are reusable without replay.
Factory membership alone establishes neither named-resource eligibility nor
global semantics. Named resources, owned components, system parameters and
mathematical nodes still need source-backed classification and their applicable
existing or new execution evidence.

FRR!E493 requires creation and use corresponding to main-resource capability.
Whole-object output is an additional witness here, not a newly imposed
mandatory operation for every factory class. Broad E491/E493/E497 completeness
is not inferred from two new resource types or from a census.

Native defect, bounded repair and support check
------------------------------------------------

The unchanged ``NasaFunctionResourceTests.cpp`` driver first passed Plate but
failed FirstOrderGaussMarkov with ``Cannot copy objects of type
FirstOrderGaussMarkov``. The model already had Clone, a copy constructor and a
typed assignment operator. Function outputs dispatch through GmatBase::Copy,
which the model had not overridden.

``FirstOrderGaussMarkov.cpp/.hpp`` add an override of that inherited virtual
method. It rejects null/incompatible sources and delegates to the existing
typed assignment operator, preserving the destination identity. No original
model line, numerical formula, data member or virtual slot changes. Exact
preimages and ``core-v2.patch`` are retained. The new native-reference hash in
the fixture is an explicit post-repair source transition; type, field, values,
assertions and bounds are unchanged. Old source capsules and receipts retain
their original identities.

``QtAcceptance.FirstOrderGaussMarkovCopy`` checks polymorphic assignment,
two independent copies, self-copy, destination names, inherited solve-for and
conversion behavior, source-mutation independence and atomic null/wrong-type
rejection. It compares all components of the existing 7-by-7 process-noise
responses at 0.1 and 600 seconds, exercising both existing time branches:
392 exact comparisons in total. This is a metamorphic copy-response contract,
not independent covariance-physics qualification.

The full core inventory now records **153 pending paths**. All previous 151
working-source hashes are unchanged; only these two model paths are added.
The inventory records review obligations, not technical approval or exclusive
authorship of earlier changes.

GUI evidence and custody corrections
-------------------------------------

The two new resource cases reuse the original driver byte-for-byte. Each
window executes two missions, before and after Unicode save/reopen, with four
function calls. Distinct caller outputs, same-name caller shadows, local
non-leakage and unchanged configured prototypes are checked. Actual function
file keyboard editing, Cancel/Save, source/history, raw continuation 41/42 and
output-tree report viewing remain required. Plate uses Area 2.5/7.25;
FirstOrderGaussMarkov and its EstimatedParameter regression use HalfLife
300/600. The six accepted resource GUI contexts execute twelve missions and
twenty-four function calls; the two census contexts execute no mission.

An initial CMake ``TARGET EKF`` guard was evaluated before plugin targets were
configured. It omitted the new support target and FOGM registration. The
unknown-target build failure, zero-test support invocation and partially
selected unattended invocation remain retained and are not counted as the
missing test passes. The guard now uses the explicit PLUGIN_EKF configuration;
the actual support test passes with ``--no-tests=error``.

The first headless and visible EstimatedParameter regression passes omitted
their fixture JSON from the source-hash list. Both remain source-limited and
are excluded from the final eight. Adding that fixture and the corresponding
EstimatedParameter sources to RunFunctionFactoryResources.py was followed by
focused regression-only reruns in both modes. The existing new-case passes
were reused. The original failing FOGM execution remains retained separately.

``AuditFunctionFactoryScope.py`` checks source-versioned native proofs, raw
files, logs, JUnit, unchanged source/image snapshots, all 566 matrix entries,
the exact two-file repair, original-driver equality and three corrupt-proof
rejections (missing type, wrong output and leaked private object). Five visible
processes are mapped to exact executable/core/EKF image hashes. Their windows
start minimized, resizable and non-fullscreen. Zero screenshots, runtime-image
copies or kernel copies are retained. All build/native handles are terminal.

Authoritative evidence audit, regular-file manifest, stage receipt, source
capsules, exact patches, failure logs and seal verification are v1 under
``build/nasa-coverage/coverage95-20261103-function-resource-census/``.
Earlier source-limited receipts remain historical custody evidence.

Replay and next scope
-----------------------

Build/run only when new or affected work warrants replay::

   cmake --build build/linux-gui --target GmatQtNasaFunctionResourceCensusTests GmatQtNasaFunctionResourceTests GmatFirstOrderGaussMarkovCopyTests -j4
   ctest --test-dir build/linux-gui -R '^QtAcceptance.FirstOrderGaussMarkovCopy$' --no-tests=error -V
   python3 src/qtgui/tests/RunFunctionResourceCensus.py
   python3 src/qtgui/tests/RunFunctionResourceCensus.py --visible
   python3 src/qtgui/tests/RunFunctionFactoryResources.py --with-regression
   python3 src/qtgui/tests/RunFunctionFactoryResources.py --with-regression --visible

``--filter '^QtGui.NasaFunctionResourceExtendedEstimatedParameter$'`` with
``--with-regression`` selects only the affected regression. The registered
case names are NasaFunctionFactoryPlate and
NasaFunctionFactoryFirstOrderGaussMarkov, each prefixed with QtGui.
Read-only audit::

   python3 src/qtgui/tests/AuditFunctionFactoryScope.py \
     --stage build/nasa-coverage/coverage95-20261103-function-resource-census \
     --output /tmp/function-factory-audit.json

Next finish source-backed eligibility/ownership/global classification for the
exact E491 conjunction and conjoin retained proofs at their recorded versions.
Reuse the older resource witnesses, built-ins, automatic-global proofs, private
navigation graph and these two cases. Add only missing mechanisms. E477's
SaveMission restriction and E406's terminal non-solution output conflict remain
unresolved; no preserved expectation changes here.
