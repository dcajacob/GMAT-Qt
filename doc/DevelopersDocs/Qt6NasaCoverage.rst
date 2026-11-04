NASA GUI requirement coverage
=============================

Fixed-reference attitude expansion — 2026-10-31
---------------------------------------------

Seven new CoordinateSystemFixed/reference-frame cases run in both modes.Six
pass independent orientation/rate/orbit checks;the velocity-aligned VNB case
retains a scientific rate failure(0 versus0.0617653degrees/second).Its original
assertion remains active,and both acceleration barriers are present in HEAD.
This increment changes tests only;all88 core and181 Qt production hashes remain
unchanged.1731checks and264audited collections retain79.9percent requirements /
77.4percent historical coverage.The VNB rate and VF13ad report remain unresolved.
No broader model/frame/private-corpus/science/native-input/hardware/platform/
stress/clean-checkout-CI/full current-build qualification is inferred.See
``Qt6ParityValidation/nasa-attitude-fixed-frames-20261031.md``.

Body-relative precision repair — 2026-10-31
-----------------------------------------

The original Uranus and Pluto cases now pass visibly and unattended with
unchanged assertions and bounds. A new native contract passes 96 checks;
three directly affected regressions are retained. The sole unresolved mission
failure is the VF13ad provider report. Coverage remains 79.9 percent requirements
and 77.4 percent historical operations across 263 audited collections; repairing
existing cases adds no source-row credit. The cumulative core inventory is 88
files awaiting scrutiny. Full NASA/private-corpus/science/native-input/hardware/
platform/stress/clean-checkout-CI and full current-build gates remain open.
See ``Qt6ParityValidation/nasa-body-local-state-20261031.md``.

NASA's GUI test criteria are published at:

* https://gmat.atlassian.net/wiki/spaces/GW/pages/380273283
* https://gmat.atlassian.net/wiki/spaces/GW/pages/380273297/GUI%2BTesting

The target includes requirement-based verification, every widget, input
validation and boundaries, mission-level results, repeatability and stress.
Passing the existing regression suite is insufficient to establish that target.

Current versioned demonstrated coverage is **925/1,158 requirement scopes
(79.9%)** and **1,833/2,367 historical operation scopes (77.4%)**.
The ledger at ``build/nasa-coverage/coverage-ledger.json`` retains independently
audited builds; it does not claim a full regression pass on the current build.
Requirement scopes describe specified behaviors; historical scopes describe
operations in the available2012GUI matrix. Several operations may exercise one
requirement. Neither percentage establishes all widget/input/science coverage.

The latest increment qualifies four hardware-unit cases in both modes: 43
resource/field pairs, 75 distinct unit surfaces and 190 label checks. All 32
advanced chemical coefficient units were blank; a GUI-only metadata fallback
now uses the existing NASA native unit arrays, matching the coefficient dialog.
Primary scalar labels were already correct. There are 144 exposed visible
boundaries, zero screenshots and two reused unattended canaries. All 85 core
hashes, the other 180 Qt production hashes and 218 credited assertion pairs stay
exact. Counts remain 79.9%/77.4%, now 1723 checks and 262 collections. Whole units
requirements and three documentation K-exponent discrepancies remain open. See
``Qt6ParityValidation/nasa-hardware-units-20261030.md``.

The preceding increment repairs both original batch-SPK failures, unattended and
visibly, with every original assertion and numerical bound unchanged. Shared
detached canonical epoch/Cartesian samples feed SPK and scoped report parameters;
live states and ordinary packets remain intact. Observations, simulator/fitted
initial reports and all ET tags are bit-identical. Maximum midpoint velocity
error is 4.304e-9 km/s under the unchanged 2e-8 limit. Two native checks (32
contracts) and three affected filter/smoother/OEM regressions pass. There are
46 exposed visible boundaries and zero screenshots. Three mission failures
remain; counts stay 79.9%/77.4%, now 1719 checks, 261 collections and 85 core
files awaiting scrutiny. See
``Qt6ParityValidation/nasa-spk-shared-sampling-20261030.md``.

The preceding increment qualifies a canonical SPK epoch clock primitive with one
native support contract: 49 boundary probes and 1435 retained batch epochs. A
read-only shared-sample control removes both sets of 143 midpoint failures, but
production state sampling is not connected and both actual tests remain failed.
File-only correction violates the unchanged report-membership limit. No GUI
scenario is replayed. Counts remain 79.9%/77.4%, with 1718 registered checks,
260 audited collections and 84 tracked core files. The other 82 core hashes,
181 Qt production hashes and 218 credited assertion pairs stay exact. See
``Qt6ParityValidation/nasa-spk-canonical-epoch-20261030.md``.

The preceding increment qualifies both actual atmosphere DragModel popup choices
in four contexts, with independent nonzero density, force and RK4 state checks.
A three-expression correction fixes the SPAD per-spacecraft acceleration query;
propagated report bytes remain identical. The passing Spherical unattended case
is reused. There are 56 exposed visible boundaries and zero screenshots; the
desktop starts minimized and stays resizable. Counts remain 79.9%/77.4%, with
1717 checks, 259 audited collections and 84 tracked core files. All prior 83 core
hashes, 181 Qt production hashes and 218 credited assertion pairs stay exact.
Whole dynamics rows and the five earlier mission failures remain open. See
``Qt6ParityValidation/nasa-drag-model-choices-20261030.md``.

The preceding increment qualifies all three actual SRP model popup values in six
GUI contexts: Spherical, SPADFile and NPlate, with nonzero independent force
checks and short-arc results. An Interpreter bug retained default Earth gravity
despite explicit empty PrimaryBodies; it is fixed without changing assertions.
There are 57 exposed visible boundaries, zero screenshots, one reused SPAD
canary and one directly affected legacy check. Counts remain79.9%/77.4%, now
1715checks and258collections. Only Interpreter.cpp changes in production;
other 82 core hashes, all 181 Qt production hashes and218credited pairs stay exact.
All 233requirement/534 historical gaps andfive older failures remain open. See
``Qt6ParityValidation/nasa-srp-model-choices-20261030.md``.

The preceding increment fixes SPK writer rejection and recovery in one core file.
Two new native contracts reproduce early output-file opening and 512 leaked
C++ arrays, then pass unchanged after the fix. They check 19 invalid input
classes, 256 repeated native rejections, four abandoned writers and 180 decoded
components. No existing GUI scenario is replayed and no whole-row credit is
added. There are now 1712 checks, 257 audited collections and 83 tracked core
files; the other 82 core hashes and all 218 credited assertion pairs stay exact.
The five retained mission failures and canonical state sampling remain open.
See ``Qt6ParityValidation/nasa-spk-writer-boundaries-20261030.md``.

The preceding core increment preserves precise estimator epochs through synchronous
publication and SPK encoding. Sixteen native contracts and two affected
filter/smoother SPK checks pass. Both original batch SPK cases still fail in
both modes: 143 midpoint failures per context, down from 1017, with maximum
velocity error 3.617e-8 km/s above the unchanged 2e-8 limit. Native report bytes
and raw Cartesian states remain identical. No whole source-row credit is added.
There are now 1710 checks, 256 audited collections and 83 tracked core files;
all 218 credited assertion pairs remain exact. Canonical state sampling, five
retained failures and the full current-build qualification remain unresolved.
See ``Qt6ParityValidation/nasa-spk-precise-publication-20261030.md``.

The preceding increment qualifies five missing force input routes in both modes:
three atmosphere file pickers,typed central-body rejection/correction and
ErrorControl=None. A real GUI bug accepted Spacecraft Sat as CentralBody and
committed source/native/history;ResourceProperties.cpp now checks the required
CelestialBody type before mutation. Ten contexts/missions pass,with121explicit
visible boundaries,31paired early Shows andzero screenshots. Four unaffected
unattended passes andthree existing checks/17proofs are reused;no existing GUI
scenario is replayed. All29inventoried fields are reviewed,28withsome bounded
witness andDensityModel lacking itsMarsGRAM provider. This is not full-field
qualification. All218credited assertion pairs and74core hashes stay exact.
Counts remain79.9%/77.4%,now1709checks and255collections,withall233requirement/
534historical gaps andfive old failures retained. See
``Qt6ParityValidation/nasa-force-routes-20261030.md``.

The preceding evidence review reuses the original GravityValidation receipts,
without a GUI replay or build. All 52 checkpoint types (104 proof records across
two modes) are explicitly traced to six degree/order/STM/potential/tide fields.
The current assertion source matches both preserved snapshots exactly; the
recorded visible setup is already resizable, scale3 and start-minimized. Input
is programmatic Qt/API delivery, not keyboard/native OS qualification. The
existing reversed Order help-table inequality remains a formal conflict.
Fifteen of29 inventoried force fields now have selected scalar/weather/gravity
validation witnesses; this is not full field qualification. Fourteen other
fields, all233requirement/534historical gaps and five old failures remain. All
218credited assertion pairs,74core hashes and Qt production sources remain
exact. Counts stay79.9%/77.4%,1704checks and254execution collections. See
``Qt6ParityValidation/nasa-gravity-boundary-reuse-20261030.md``.

The preceding increment qualifies 22 weather-source/Schatten configurations in both
modes: 44 contexts/missions, 298 actual popup selections, 704 explicit UTC
lookups and 40 independently checked cached forecast records. It reproduces and
repairs a core CSSI Ap history defect: the four records now use day offsets
0,-1,-2,-3 instead of repeating the previous day. All 73 earlier core hashes and
218 credited assertion-source pairs remain exact; ``SolarFluxReader.cpp`` is
tracked core file 74 awaiting scrutiny. One new unattended canary is reused;
the other 21 unattended and all 22 visible cases pass, plus the directly
changed existing Atmosphere integration check. There are 763 explicit exposed
visible boundaries and 237 paired early Shows, zero screenshots and no unrelated
replay. Whole counts remain 79.9%/77.4%, now 1704 checks and 254 collections.
All 233 requirement/534 historical gaps and five old failures remain; the old
failures are not rerun on this build. Empirical nonzero drag, interpolation,
within-day Ap patterns and broader gates remain open. See
``Qt6ParityValidation/nasa-weather-choices-20261030.md``.

The preceding increment qualifies five scalar force-input boundary/status cases
in both modes: 82 rejected inputs, 34 positive Apply commits, 82 no-op Apply
acknowledgements, 68 Undo/Redo actions and 10 runtime-delivery/null-area missions.
It exposes and repairs a generic GUI Apply serialization defect: 16 digits
could round a legal value onto an excluded endpoint. ``MainWindow.cpp`` now
uses at least ``max_digits10`` while serializing, then restores the prior setting.
The directly affected original ``PropSetupApply`` regression passes unchanged;
the new passing unattended SRP canary is reused. Visible traces have 737 explicit
exposed boundaries and 99 distinct later counterparts for early Show events,
with zero screenshots. All 73 core hashes and 218 credited assertion pairs stay
exact. This is bounded evidence for five of the 29 inventoried force fields;
whole-scope percentages remain 79.9 and 77.4 with 1682 checks and 253 collections.
Five old failures and all 233 requirement/534 historical gaps remain. See
``Qt6ParityValidation/nasa-force-input-boundary-20261030.md``.

The preceding increment inventories seven basic and 29 owned-force fields through
their actual form or Advanced-table routes. Four missing unit labels are fixed
in ``ResourceProperties.cpp``: SRP flux, nominal Sun distance, F107 and F107A,
with eight guide-pinned label observations across both modes. The two new
inventory cases retain the unaffected basic unattended pass; the affected owned
case and both visible cases use final metadata. No core or numeric value
changes; all 73 core hashes and 218 credited assertion pairs remain exact.
The field worklist names remaining input/status/Apply/result and conditional
force-configuration gaps. Whole-scope percentages remain 79.9 and 77.4, with
1677 registered checks and 252 audited collections. Five old failures and all
233 requirement/534 historical gaps remain. See
``Qt6ParityValidation/nasa-force-control-inventory-20261030.md``.

The preceding increment adds eight named FK5/ICRF impulse and integrated-thrust
GUI cases at two epochs. All eight pass visibly; six affected unattended
contexts use the final repair and two passing unattended impulse controls are
retained. The new cases exposed missing ICRF-to-FK5 conversion in ``Burn.cpp``
and ``Thruster.cpp``; both now rotate free vectors into the internal
FK5/MJ2000Eq basis. All 71 prior core hashes and 218 credited assertion pairs
remain exact; 73 core files now await scrutiny. The unchanged 2e-7 km position
and 2e-10 km/s velocity bounds pass, with 72 explicit exposed visible boundaries
and zero screenshots. FRR!E144 gains bounded partial evidence only: whole-scope
counts remain 79.9 and 77.4 percent. The suite has 1675 registered checks and
251 audited collections. Five old failures, 233 requirement/534 historical gaps
and all broader gates remain open. See
``Qt6ParityValidation/nasa-fk5-dynamics-20261030.md``.

The preceding suite increment qualifies four new bounded numerical-capacity cases
in both modes:16or128groups,32or256spacecraft,nine selected methods,independent
or synchronized single-step timing,and forward/backward directions. Eight
missions check1152spacecraft result observations and6912native state components.
All56new predicates and218prior credited assertion pairs remain exact;no
production/core change or unrelated replay. The suite has1667checks and250
collections. Whole-scope percentages remain79.9and77.4:the historical unlimited
capacity and ambiguous multiple-propagator recipe remain uncredited. Five old
failures and all233requirement/534historical gaps remain. Zero screenshots;
209853109bytesofderived metadata compacted. See
``Qt6ParityValidation/nasa-numerical-capacity-20261030.md``.

The preceding diagnostic localizes the retained Uranus/Pluto numerical failures
without a GUI/mission replay or code/assertion change. A read-only native probe
measures four origin translations;independent80digit adjacent-binary64 analysis
reconstructs24saved components exactly. At the saved Pluto finalY,the nearest
possible reported value through the same Earth-origin representation is about
0.459mmfrom truth,exceeding the retained0.2mmbound. Integrator accuracy alone
cannot repair that output path. Uranus requires separate handoff investigation.
Both failures and all coverage counts remain unchanged;no waiver or new credit.
See ``Qt6ParityValidation/nasa-body-origin-precision-20261030.md``.

The latest coverage increment explicitly qualifies11named-body Apply-status rows using
44retained passing control/appearance contexts and132missions. The audit binds
all exposed current controls,clean/pending/invalid/applied/reopened states and
atomic rejection/persistence to each literal Validation_ApplyButton_Status row.
No GUI/build/mission replay and no production/test/core changes were needed.
All218assertion pairs and71core hashes remain exact. Cumulative coverage is
925requirements and1833historical operations across249collections,with233
requirement/534historical gaps and95GUI entries. The failure inventory now
includes two previously separate Uranus/Pluto numerical holds alongside the
three prior VF13ad/SPK entries:five retained unresolved failures,no new failures
or relaxed assertions. Uranus/Pluto and ambiguous aliases remain uncredited.
See ``Qt6ParityValidation/nasa-body-apply-status-20261030.md``.

The preceding increment adds four complete active-field user-body clone workflows.
Eight visible/unattended contexts check all27variable fields of the selected
29-field SPICE-to-IAU configuration,complete pending Cancel and Apply,independent
copy/original edits and deletions,Undo/Redo,reopen,and48scalar missions.
Fifteen historical scopes qualify;Moon aliases and the private original
ChangeAll payload remain open. Seven passing contexts retain mouse popup
input;only failed unattended Comet was rerun with strengthened keyboard
selection. All production GUI/core sources and217prior assertion pairs remain
unchanged. Cumulative counts are925requirements and1822historical operations
across248collections,with233requirement/545historical gaps and95GUI entries.
No unrelated replay or screenshots;173882786bytes of derived metadata recovered.
See ``Qt6ParityValidation/nasa-user-body-all-clone-20261030.md``.

The preceding increment qualifies eight untouched-clone/selected-role Rename
workflows,including retained-editor edits after counterpart deletion and local
live Undo/Redo. Six historical scopes qualify with16versioned contexts and48
scalar missions. One older unattended canary is reused;seven unattended and all
eight visible contexts use the final source-change refresh fix. MainWindow.cpp
refreshes clean editors after changed-source Build while retaining identical-
source Run/Stop identity. The unchanged affected body regression passes.
Cumulative counts are925requirements and1807historical operations across247
collections,with233requirement/560historical gaps and95GUI entries. All216prior
assertion pairs and71core hashes are unchanged. No unrelated replay/screenshots.
See ``Qt6ParityValidation/nasa-user-body-clone-rename-20261030.md``.

The preceding increment implements unused user-body GUI Delete through a separate
SolarSystem source transaction,retaining the generic ConfigManager body guard.
Four new cases pass both modes:8contexts,56dependency refusals,8actual deletions
and72scalar recovery/continuation missions. Three historical Delete scopes
qualify;private clone-operation/ChangeAll/alias recipes and broader domains
remain open. Cumulative counts are925requirements and1801historical operations
across246collections,with233requirement/566historical gaps and95GUI entries.
All215prior assertion pairs and71core hashes are unchanged;four Qt production
files await scrutiny. One unchanged affected regression passes;no unrelated
replay or screenshots. See
``Qt6ParityValidation/nasa-user-body-delete-20261030.md``.

The preceding increment enables user-defined body Rename and repairs child-body
CentralBody string-reference rewriting in two Qt sources. Four new cases pass
both modes:8contexts,16missions,72name rejections and complete selected native
body/reference graph checks. Twelve historical scopes qualify;Delete and
clone-rename/private recipes remain held. Cumulative counts are925requirements
and1798historical operations across245collections,with233requirement/569historical
gaps and95GUI entries. All71core hashes and every prior assertion remain intact;
one affected legacy test helper now explicitly discards touched drafts before
closing. No unrelated replay or screenshots. See
``Qt6ParityValidation/nasa-user-body-rename-20261030.md``.

The preceding increment qualifies untouched initial user-body cloning through four
new cases in both modes:8contexts,24missions,26writable fields per body and
complete native serialization comparisons. Three separate CloneDefault rows
qualify;modified-body passes are reused. Cumulative counts are925requirements
and1786historical operations across244collections,with233requirement/581historical
gaps and95GUI entries. All213prior assertion pairs,71core hashes and production
Qt sources remain unchanged. No unrelated replay or screenshots. See
``Qt6ParityValidation/nasa-user-body-default-clone-20261030.md``.

The preceding increment implements documented user-defined body cloning in one Qt
source. Four classes pass8visible/unattended contexts and24missions,with copied
source,reference retention,independent edits,24default-protection checks and
same-input SPICE state equivalence. Three exact CloneModified rows qualify;
separate CloneDefault and private ChangeAll/rename/delete recipes remain held.
Current coverage is925requirements/1783historical operations across243collections;
233requirement/584historical gaps and95GUI entries remain. The83offered names now
have54bounded type witnesses of differing roles,not full type qualification.
All212prior assertion sources and71core hashes are unchanged;one Qt change awaits
scrutiny. No unrelated replay or screenshots. See
``Qt6ParityValidation/nasa-user-body-clone-20261030.md``.


The preceding increment adds4function-file byte/extension scenarios:8visible and
unattended contexts,20missions and37exposed viewing boundaries. UTF-8 literal
results,malformed comment bytes,a .gmf.txt suffix,and leading-byte rejection/GUI
recovery are directly checked. FRR!E480remains uncredited because these behaviors
conflict with a literal historical only-.gmf/ASCII restriction. Formal conflicts
increase to42;all previous41records,212credited assertion sources and71core hashes
are unchanged. No production GUI/core changes,no unrelated replay and no
screenshots. Source gaps remain233requirements/587historical operations and95GUI
entries,with the3older failures and broader gates open. See
``Qt6ParityValidation/nasa-function-file-policy-20261030.md``.


The preceding expansion qualifies the documented external-plugin mechanism in
All Others!E38using a test-only external resource and command. Three new cases
pass in6visible/unattended contexts,10missions and6custom command executions.
Missing/omitted providers reject unavailable types and recover;actual resource
creation,Cancel,Undo/Redo,property rejection,native results,raw report/viewer and
Unicode reopen are checked. Two negative unattended passes are reused. All211prior
assertion sources and71core hashes are unchanged;no production GUI change.
233requirement/587historical gaps and95GUI entries remain,plus the3older failures
and broader gates. The isolated provider does not add to the default83offerings.
See ``Qt6ParityValidation/nasa-external-plugin-20261030.md``.


The preceding expansion adds19built-in function scenarios covering15offered type
names,including4selected domain rejection/recovery cases. They pass visibly and
unattended across preserved versions. The final visible run also passes5directly
affected validation/call-syntax regressions. Seventeen unaffected new unattended
passes are reused. Three core files and one Qt validation file repair named
function resolution/dispatch,compound callable discovery,indexed result returns
and unnamed output storage;71corefiles await scrutiny,with68other previous hashes
unchanged. No previous credited assertion source changes;no new whole-row credit.
The catalog now has some bounded witness for50of83offered names,not full type
qualification;33offerings remain for reconciliation/new proof. All234/587source
gaps,3older enabled failures and broader gates remain. See
``Qt6ParityValidation/nasa-function-builtins-20261030.md``.


The preceding expansion adds11resource cases using the unchanged numeric driver,
plus1native Copy support contract. All11pass across22visible/unattended versioned
contexts:44missions,88function calls and121exposed visible boundaries. Five missing
polymorphic Copy methods are implemented;70corefiles await scrutiny and all
previous60hashes are unchanged. Six unaffected unattended passes and7prior
specialized type witnesses are reused. One unintended ExtendedKalmanFilter replay
is disclosed;no other old cases replayed. Three matrices cover28selectedtypes;
35of83GUI offerings have some bounded witness after reconciliation,with differing
roles and no whole-type qualification.48offerings await reconciliation/new proof.
No new whole-source credit,zero screenshots,and all234/587source gaps and3existing
enabled failures remain. See
``Qt6ParityValidation/nasa-function-resource-extended-20261030.md``.

The newest expansion adds6string/list/reference-dependent function-resource
cases plus a current GUI catalog check:14versioned new contexts,24missions and
48function calls. All7new cases and6directly affected regressions pass visibly
and unattended across versioned receipts. The GUI offers83type names;these two
function-resource matrices cover17selectedtypes,with66offerings requiring
reconciliation against specialized prior tests. No new whole-source credit.
Three native ownership/initialization causes are fixed;60corefiles await scrutiny,
with57previous hashes unchanged. Three unaffected unattended contexts retain
older-core receipts. Zero screenshots;all3existing enabled failures and234/587
source gaps remain. See
``Qt6ParityValidation/nasa-function-reference-resources-20261030.md``.

The latest partial-scope expansion adds11function-local resource/whole-object
output cases. All11selected classes pass both modes:22versioned contexts,
44missions and88scripted calls. Native type/literal values,distinct caller names
and pointers,private-name isolation,configured prototypes,actual external-file
Cancel/Save,Unicode reload and the report viewer are checked. Ten unaffected
unattended passes are reused after a Simulator-only valid-dependency extension.
All121explicit visible boundaries are exposed;22pre-exposure modal Show records
have distinct exposed counterparts. Zero screenshots and no core/product GUI
changes. No universal any-resource/global/type/property or provider-physics credit
is inferred:counts remain924/1,158and1,780/2,367,now237collections and1590checks.
All236prior collections and211credited assertion pairs remain unchanged;the two
new partial-source versions are separately bound.234requirement/587historical
and96GUI gaps,three unresolved failures and all broader gates remain. See
``Qt6ParityValidation/nasa-function-resource-matrix-20261030.md``.

The latest coverage-adding increment qualifies the finite non-spherical central
body heading,E104,with both named children backed by new independent force and
trajectory evidence. Two ten-scenario Earth/Ceres matrices pass both modes:
40qualified missions,280native force samples and240state components. EarthJGM2
is exercised through70/70;synthetic Ceres through8/8. Direct dual potential/RK4
and mandatory separate complex-step/DP5 checks pass. All140explicit visible
boundaries are exposed;40rawmodal Show records have distinct exposed counterparts.
No screenshots or production/core changes. All235prior collections and210prior
assertion pairs remain;there are236collections,1579checks and211pairs. Remaining
234requirement/587historical gaps,96GUI entries,three enabled failures,12disabled
positives,41conflicts and all broader gates remain. Individual high-degree
coefficient sensitivity,orientation/ephemeris truth and empirical models are not
inferred from selected whole-field sums. See
``Qt6ParityValidation/nasa-nonspherical-gravity-20261030.md``.

The latest diagnostic increment captures all1,435precise final-pass epochs in
one already-failing positive SPK case. All1,434intervals are exactly20seconds;
spacecraft,published and native report states agree. Ordinary MJD publication
loses up to156.25ns;precise clock tags alone still fail143midpoints. A coherent
canonical-time/state diagnostic model passes all1,434midpoints at unchanged
limits,but no production repair or acceptance credit is inferred. The original
strict assertion remains failed. Zero screenshots,no unrelated passing replay,
no production/core/assertion changes;the58-file scrutiny inventory is unchanged.
See ``Qt6ParityValidation/nasa-spk-precision-feasibility-20261030.md``. All235
requirement/587historical gaps,three enabled failures and broader gates remain.

The preceding coverage-adding increment executes39independent NAIF epoch literals and8start-keyword
paths per mode in four new GUI scenarios.94missions/282report rows/1692state
components pass;all286visible boundaries are exposed,with zero screenshots.
Both string-input time-conversion overloads now preserve valid UTC60th-second
labels and reject nonexistent/non-UTC labels.128native time checks,64roundtrips
and14rejections pass. One new core file joins58-file scrutiny inventory;all prior
57core hashes and209assertion pairs remain unchanged. Technical approval is false.
Three ordinary unattended GUI passes and a leap GUI pass are reused under exact
source-impact review;all four scenarios pass visibly on the final core.
Finite headings E130(cannonball-only list,reusing old receipts) and E306(explicit
and inherited epoch choices) receive bounded credit. Counts become923/1,158
(79.7percent) requirements and1,780/2,367(75.2percent) historical operations,
235collections,1577checks and210pairs.235requirement/587historical gaps and97GUI
entries remain. UTC MJD during a leap second remains explicitly ambiguous and
unqualified;all scientific/native/platform/provenance/full-build gates remain.
Lossless new derived-metadata compression recovers206,028,473bytes;raw failures
and evidence remain. See ``Qt6ParityValidation/nasa-epoch-format-execution-20261030.md``.

The preceding increment directly qualifies selected cannonball drag configuration
and results under FRR!E131. Ten new scenarios pass unattended and visibly with
independent SI force, converged RK4 states and a separate fifth-order Python
calculation. Across both modes:20missions,80resource Applies,40atmosphere dialog
decisions,200report columns,120state components and60acceleration components.
All181explicit action boundaries are exposed;20raw pre-exposure Show checkpoints
retain distinct exposed counterparts. No core/product GUI changes or unrelated
passing replays;all57core hashes and208prior assertion pairs remain unchanged.
The report clock differs from the live integrator by1.3737e-7seconds and remains
unqualified for exact timing. Counts become921/1,158(79.5percent) requirements,
1,780/2,367(75.2percent) historical scopes,234collections,1571checks and209pairs.
237requirement/587historical gaps,99GUI requirements and all broader gates remain.
Zero screenshots;205,988,803derived bytes recovered with raw evidence preserved.
See ``Qt6ParityValidation/nasa-cannonball-drag-20261030.md``.

The preceding increment qualifies selected EA/MA/HA callers without changing core
or production GUI code. All57core hashes and the original Assignment driver are
unchanged. A new support gate passes3420valid-domain anomaly fields and4140
independent anomaly-based state reconstructions over2070inputs;reported pairs
satisfy Kepler equations to8.88e-16radians.720near-circular EA/MA fields remain
unqualified,including356example differences,distinct from the earlier720AOP/TA
holds. Eight new readonly-calculation GUI cases pass both modes:160report
columns,64angular results,96Cartesian components,80invalid transactions including
32readonly rejections,and192exposed visible boundaries. Original assertions and
bounds remain. No historical setter conflict or whole NASA row is waived.
Counts stay920/1,158and1,780/2,367,now233collections and1570checks;all208credited
source pairs remain unchanged. Zero screenshots/unrelated replays;68,612,249derived
bytes recovered,with shared immutable baseline sources. See
``Qt6ParityValidation/nasa-anomaly-callers-20261030.md``. The next review returns
to uncredited GUI requirements while preserving all condition/science/platform gaps.

The preceding increment fixes stable AOP/RAAN/TA scalar geometry and equatorial
retrograde orientation in three core helper functions. The same new regression
records994checked-field and1173joint-state failures before the repair,and passes
5490checked fields/2070joint scalar-angle Cartesian states afterward at unchanged
bounds.720near-circular individual fields remain unqualified;357exceed the example
angle bound. Eight new Assignment/direct-Report GUI cases pass both modes through
the unchanged original driver:160report columns,64angular results,96Cartesian
components,48invalid transactions and160exposed visible boundaries. Eight initial
GUI build failures are retained;correct origin-qualified TA replaces a mistaken
frame-qualified fixture name without numerical/core/assertion changes,and the
passing native gate is reused. Only one of57tracked core files changes;all other
56hashes and production GUI sources are unchanged. Technical approval is false.
Counts stay920/1,158and1,780/2,367,now232collections and1561checks;all208credited
source pairs remain unchanged. Zero screenshots/unrelated replays;102,908,657derived
bytes recovered,and immutable baseline sharing avoids another source copy.
See ``Qt6ParityValidation/nasa-scalar-angles-20261030.md``. EA/MA/HA boundaries,
condition-limited fields and all broader NASA/science/platform gates remain open.

The preceding increment fixes the separate public scalar INC calculation with
stable atan2 momentum geometry. The new regression reproduces460failures before
the fix and passes2070degree/radian pairs afterward at the unchanged1e-8degree
bound;maximum error is2.84e-14degrees. All2070also match the preserved NAIF scalar
receipt,which is reused without state/conic replay. Four new Assignment/direct
Report parameter cases pass both modes through the unchanged original driver:
64report columns,16scalar results,48Cartesian components,24invalid transactions
and80exposed visible boundaries. Only CartesianToINC changes in one of57tracked
core files;all other56hashes and production GUI sources are unchanged. Technical
approval is false. Scalar AOP/RAAN/TA near-singular callers and all existing
mean/prototype/provider/physics/platform gaps remain open. The first new invalid
fixture preflight is retained;exactly represented guard inputs replace rounded
escape speed without changing the rejection assertion. Counts stay920/1,158and
1,780/2,367,now231collections and1552checks;all208credited source pairs remain
unchanged. Zero screenshots/unrelated replays;68,589,150derived bytes recovered
plus20,422,084bytes deduplicated from a new baseline source copy. See
``Qt6ParityValidation/nasa-scalar-inclination-20261030.md``.

The preceding increment makes the reusable support gate enforce56native Brouwer
physical inverse reconstructions and2070analytic Kepler angular-boundary states.
Two new near-equatorial/retrograde-apsis GUI definitions pass both modes through
the byte-unchanged original driver:24typed/reopened fields,48unit checks,
120native components,16invalid transactions and68exposed boundaries. Three core
functions use stable angles/inclination reconstruction and tighter short-period
inverse convergence. Only StateConversionUtil changes among57tracked core files;
technical approval is false. Independent prototype inverse reconstruction passes
54/56,and44/56individual inverse fields pass. The separate NAIF conic round-trip
passes2027/2070;43strict hyperbolic velocity differences remain. Public scalar INC
still differs in460cases and is an explicit next repair target. No bound is
relaxed,and no full mean-science/parent requirement claim is made. Counts stay
920/1,158and1,780/2,367,now230collections and1547checks;all208credited source pairs
are unchanged. Zero screenshots/unrelated replays;68,522,273derived bytes recovered.
See ``Qt6ParityValidation/nasa-brouwer-inverse-20261030.md``.

The preceding increment adds two actual six-field Brouwer definitions using the
byte-unchanged original driver. Both pass both modes:24typed/reopened fields,
48unit checks,120native components,16invalid transactions,four mission reports
and68exposed visible boundaries. A new support gate retains all30historical mean
inputs at two mu values:56valid NASA-prototype forward comparisons and4invalid
zero-reference rejections pass. Two long-period core functions preserve nonzero
eccentricity/plane geometry and tighten inverse convergence;only StateConversionUtil
changes among57tracked core files. Technical approval is false. Inverse remains
held:17strict field and14Cartesian round-trip gaps. Only6/56historical FreeFlyer
Cartesian comparisons pass. No independent averaging/full-parent science claim.
One missing-input support attempt is preserved;only that failed check was rerun,
and both unattended GUI passes were reused across the metadata-only correction.
Counts stay920/1,158and1,780/2,367,now229collections and1,545checks;all208prior
credited assertion pairs remain unchanged. Zero screenshots/unrelated replays;
102,893,466derived bytes recovered. See
``Qt6ParityValidation/nasa-brouwer-definitions-20261030.md``.

The preceding increment reconciles the existing Estimation filename fixture with
its unchanged UTF-8 validator:the original rejection predicate now uses a reserved
question mark,and a new positive primary-field Unicode report case verifies
Apply/history/save/reopen and actual native output. Every original assertion and
numeric bound is preserved. The same affected case passes visibly and unattended:
20successful missions,16state comparisons/210components,12literal truth checks,
and114explicit exposed viewing boundaries. No core/product GUI changes,unrelated
replays or screenshots. Known active failures fall from four to three:VF13ad raw
text truth and both strict SPK midpoint velocity cases remain unchanged. Coverage
stays920/1,158and1,780/2,367,now228collections and1,542checks;207assertion source/hash
pairs are unchanged and one is explicitly rebound.68,565,915derived bytes recovered.
See ``Qt6ParityValidation/nasa-estimation-filename-20261030.md``. All broader gaps
and full newest-build/core/frontend qualification remain open.

The preceding increment enforces the guide's Earth-only Brouwer origin policy in
the initial catalog and live frame previews. Two namespace cases pass both modes:
52frame contexts,48finite mean-field/unit checks,four Earth-like Mars-mu controls,
four pending-Discard checks and70exposed viewing boundaries. Only two frontend
filters change;all57core hashes and208prior assertion pairs are unchanged.
Both unattended results are reused across a visible-only timer-placement fix.
All30historical FreeFlyer truth files are retained:60comparisons at two mu values,
6strict Cartesian matches and4native exceptions for2invalid zero reference files.
A restricted NASA prototype adapter retains238assignments;46/60native comparisons
match its formulas. This is source parity,not independent averaging science.
Positive six-field input and numerical/model/precision questions remain open;
no tolerance or core formula changes. Counts stay920/1,158and1,780/2,367,now
227collections and1,542checks. No unrelated replay or screenshots;171,378,127
derived bytes recovered. See ``Qt6ParityValidation/nasa-brouwer-origin-20261030.md``.

The preceding increment makes the existing Workflow's formerly hidden operations
watchable:83explicit native viewing boundaries are exposed,and all65previously
hidden observations have distinct exposed counterparts in the same original
callback/assertion sequence.28fixture constructors and45modal-driver source
sites gain safe viewing waits. Every prior source byte is recoverable by
reversing only those view additions;no assertion/callback body is removed.
A Qt6.10focused-filename guard repairs the exposed file-chooser driver. The
passing unattended result is reused across that visible-only correction.
66raw pre-map Show observations remain in the new trace;they are not counted
as explicit exposure boundaries. No core/product GUI/shared observer changes.
The57core hashes remain current and unapproved. No unrelated/Planetodetic
replay or screenshot;102,875,832derived bytes recovered. Counts stay920/1,158
and1,780/2,367,now226collections and1,540checks. Four known failures and all
broader scientific/native-input/hardware/platform/stress/provenance/current-build
gates remain open. See ``Qt6ParityValidation/nasa-workflow-visibility-20261030.md``.

The preceding increment adds four exterior Planetodetic input cases, all passing
both modes: eight contexts,48 typed/reopened fields,96 unit checks,240 state
components,64 invalid transactions,eight reports and176 exposed boundaries.
Independent native geometry/NAIF checks pass74 states/444 components,including
exact and near poles. The native exterior inverse now uses exact spheroid
geometry;all other conversion functions and the legacy interior branch are
unchanged. Visible orbit selectors now synchronize their validated choices.
The directly affected Workflow passes both modes after an explicit no-op Apply
clears its reverted draft;all old assertions survive with three new guards.
Its legacy trace still has65unexposed automatic checkpoints,explicitly held.
Counts remain920/1,158and1,780/2,367,now225collections and1,540checks.207old
assertion pairs are unchanged;one Workflow pair is rebound with every prior
assertion preserved.57core files await scrutiny;only StateConversionUtil changes
from the previous inventory. Zero screenshots;342,881,203derived bytes recovered.
E13,general/interior/frame/science/current-build gates and four prior failures
remain open. See ``Qt6ParityValidation/nasa-planetodetic-20261030.md``.

The preceding GUI increment adds two negative-C3 elliptic apoapsis-extension cases.
Both pass both modes:four contexts,24typed fields,48unit checks,120state
components,four reports and116paced visible boundaries.64invalid Apply/OK
transactions cover nonfinite/nonnumeric C3,zero/plus-minus5e-8C3,circular C3 and
exact-pole DHA,with complete valid-draft recovery. The unchanged native inverse
probe passes16new negative-C3 samples/96components. The original driver remains
byte-unchanged;the derived driver preserves36original require calls and adds
two guards. No prior replay,GUI retry,screenshots or core/product GUI changes.
Exact threshold endpoints,conventions and broad domains remain open;E13 stays
uncredited. Counts stay920/1,158and1,780/2,367,now224collections and1,536checks.
208credited source pairs and57core hashes remain unchanged. See
``Qt6ParityValidation/nasa-asymptote-elliptic-20261030.md``.

The preceding GUI increment uses the unchanged original definition driver for five
new six-field input cases:Modified/AlternateEquinoctial,Delaunay,Incoming/Outgoing
Asymptote. All pass both modes:10contexts,60typed fields,60reopened components,
120unit checks,300state components,40invalid Apply/OK rejections,10reports and
170paced visible boundaries. This qualifies selected GUI input/native-inverse
examples;general domains and asymptote conventions remain open. Raw legacy
proof prose is preserved with fixture-specific audited scope. No previous case
replay,failed retry,screenshots or core/product GUI changes.208credited source
pairs and57core hashes remain unchanged. Counts stay920/1,158and1,780/2,367,
now223collections and1,534checks;parent E13 and all broader gates remain open.
See ``Qt6ParityValidation/nasa-orbit-extra-definitions-20261030.md``.

The preceding GUI increment adds56hyperbolic display-conversion pairs among eight
representations,including Incoming/OutgoingAsymptote. Eight new cases pass both
modes:16contexts,112pair executions,2,688displayed components/units,5,472native
components,672reported components and624paced boundaries. A separate native
probe checks16inverse samples/96components. Actual six-field GUI asymptote
input,general inverse domains,negative-C3 extension and incoming position-ray
versus velocity/B-plane convention reconciliation remain open. The72prior
elliptic scenarios are retained without replay. No GUI failures/retries,
screenshots or core/product GUI changes;208credited source pairs and57core
hashes remain unchanged. Counts stay920/1,158and1,780/2,367,now222collections
and1,529checks;parent E13 and all broader gates stay held. See
``Qt6ParityValidation/nasa-hyperbolic-conversions-20261030.md``.

The preceding GUI increment adds42directed pairs for Delaunay,ModifiedEquinoctial
andAlternateEquinoctial. Nine new cases pass both modes:18contexts,84pair
executions,2,016displayed components/units,4,140native components,504reported
components and471paced boundaries. Reusing the prior30pairs without replay
completes72directed pairs among nine explicitly named representations in the
bounded elliptic EarthMJ2000Eq matrix. A source-bound combined matrix retains
144pair/mode entries. Five other native representations,other frames and full
mathematical domains remain open;parent E13 stays uncredited. No failed retry,
screenshots or core/product GUI changes;208credited source pairs and57core
hashes remain unchanged. Counts stay920/1,158and1,780/2,367,now221collections
and1,521checks. All four prior failures and broader gates remain open. See
``Qt6ParityValidation/nasa-orbit-extended-conversions-20261030.md``.

The preceding GUI increment qualifies30directed pairs among Cartesian,Keplerian,
ModifiedKeplerian,Equinoctial,SphericalAZFPA andSphericalRADEC. Six new cases pass
both modes,12contexts/60pair executions,with1,440independently checked displayed
components/units,2,952native components,360reported components and336paced
visible boundaries. Cancel/Apply,exact history,Unicode reopen and mission
results are checked. Six distinct elliptic orbits andEarthMJ2000Eq only;other
representations,frames andsingular/hyperbolic domains keep parent E13 uncredited.
No existing standalone definition replay,failed retry,screenshots or core/product
GUI change. All208credited assertion pairs and57core hashes remain. Counts stay
920/1,158and1,780/2,367,now220collections and1,512checks. Four enabled failures
and all remaining gates persist;102,700,651derived bytes recovered losslessly.
See ``Qt6ParityValidation/nasa-orbit-conversions-20261030.md``.

The preceding diagnostic localizes batch-SPK epoch precision across native
publication, buffering and conversion. The current native converter reproduces
all2,870stored tags exactly. A conversion-only GmatTime substitution still
fails1,133midpoints per case;precise uniform input reduces this to143,but only
a recentered,non-SPK-representable control reaches zero. No clock/core fix or
tolerance change is adopted. Both strict SPK failures and all physical assertions
remain. No GUI mission replay or screenshots;57core hashes match. Counts stay
920/1,158and1,780/2,367,219collections and1,506registrations;this diagnostic
adds no scope credit. See ``Qt6ParityValidation/nasa-spk-clock-localization-20261030.md``.

The preceding increment adds an independent type13 SPK Hermite reader and nine
offline contracts. It agrees with all11,476retained reader queries/68,856state
components across four prior GUI contexts. The existing decoder now checks the
encoded curve before its unchanged physical assertions: three saved decoded
contexts pass the reader check and retain the original physical failure. All37
Python physical assertions/five functions,82strict C++predicates and208credited
assertion pairs remain unchanged. This narrows the precision investigation;
it does not qualify the physical trajectory or close E406. No GUI replay,
screenshots or core/product GUI edits;all57core hashes remain pending review.
Counts remain920/1,158and1,780/2,367,with219collections and1,506registrations;
the new registration is support only. Four enabled failures and all remaining
gaps remain open.68,372,878derived bytes were recovered losslessly. See
``Qt6ParityValidation/nasa-spk-hermite-20261030.md``.

The preceding increment adds eight normal-startup filter/smoother format cases.
All pass both modes, with an unchanged directly affected TotalMass regression.
A narrow Validator fix restores NASA's direct Spacecraft covariance assignment;
576matrix components match an independent diagonal reference. Twelve forbidden
format rejections and four supported STK contexts pass, with132points/792Cartesian
values,72event boundaries,6native clones and167visible boundaries. These are
format/assignment/structural/native controls,not independent full filter science.
All56original new predicates and208credited assertion pairs remain;3guards were
added. Only Validator.cpp changes in production;other56core hashes are unchanged,
57files remain pending scrutiny. No format-guard or product GUI patch. No
unrelated passing replay or screenshots;136,905,479derived bytes recovered.
Counts stay920/1,158requirements and1,780/2,367historical scopes,now218collections
and1,505checks. E406 stays uncredited;four prior failures remain open. See
``Qt6ParityValidation/nasa-filter-formats-20261030.md``.

The preceding increment adds strict opposite-bias batch SPK cases. Both remain
ordinary enabled failures, reproduced both modes: all raw final-pass native
states/clock/frame/circle/segment checks pass, but1,017midpoint queries per
context exceed the unchanged2e-8km/s velocity bound. Four contexts check5,744raw
knots/34,464Cartesian values and11,476reader queries;4,068midpoints fail and all
knots pass. This strict fixture bound is not a newly found numerical NASA
requirement. Clock-reencoding controls isolate representation/cadence sensitivity;
no production clock fix or tolerance adjustment is adopted. All82new predicates,
208credited assertion pairs, prior tests and core57 remain unchanged. No product
GUI/core edit, unrelated passing replay or screenshots. Counts stay920/1,158and
1,780/2,367, now217collections/1,497checks. E406 stays uncredited. See
``Qt6ParityValidation/nasa-batch-spk-20261030.md`` for failures, controls and limits.

The preceding increment adds four batch final-pass format cases: STK kilometers/
meters and Code500 little/big endian. All pass both modes with independent full
file decoding and circular truth:11,480points/68,880Cartesian values,60terminal
padding states,116paced visible boundaries,7offline contracts and16rejected
raw-file mutations. All208credited assertion pairs, prior OEM tests and core57
are unchanged; no production GUI/core edit or unrelated passing replay. The
application binary is unchanged. E406 remains partial/uncredited; counts stay
920/1,158requirements and1,780/2,367historical scopes, now216collections and
1,494checks. No screenshots;136,651,494derived bytes recovered losslessly.
See ``Qt6ParityValidation/nasa-batch-formats-20261030.md`` for source-bound
receipts, the guide-justified new STK boundary-expectation correction and gaps.

The preceding increment corrects the new one-iteration test's unsupported
convergence-to-truth expectation using an independent universal-variable and
scaled-SVD reference. Native output remains byte-identical. All35original
predicate expressions and bounds remain;9newguards check every computed range,
residual and the58/358accepted set, with tighter independent-state bounds.
The formerly failed case passes unattended and visibly;7offline mathematical
contracts also pass. No product GUI/core source or application binary changed.
Core57 remains pending scrutiny. Counts stay920/1,158requirements and1,780/2,367
historical scopes with215collections and1,489checks. E406 remains uncredited;
no ephemeris is generated here. No passing GUI replay or screenshots. See
``Qt6ParityValidation/nasa-batch-one-step-20261030.md``.

The preceding increment adds structured RunEstimator SolveMode controls and fixes
nested trigger-manager forwarding plus retained initialization on repeated
estimator invocation. Initial-guess/no-correction and two-pass same-command For
cases pass both modes with unchanged new test source. One directly affected
existing solver-mode regression passes unattended. The new one-iteration case
stops correctly but still fails its unsubstantiated accuracy bound; it remains
enabled/unwaived for independent review. No ephemeris is generated by these mode
cases and E406 receives no credit. Counts remain 920 / 1,780 scopes with 214
collections and 1,488 checks. The core scrutiny inventory is 57 files; other54
prior core hashes remain unchanged. No unrelated passing replay or screenshots.
See ``Qt6ParityValidation/nasa-batch-modes-20261030.md``.

The preceding increment implements the missing batch final trajectory replay. Both
strict opposite-bias cases pass unattended and visibly with their entire test
source unchanged: 5,740 OEM points / 34,440 Cartesian values across four contexts,
36 paced visible boundaries and restored fitted initial conditions. Observation
bytes and numerical fit progress are unchanged. Three core files are added to
the scrutiny inventory, now 55; prior 52 core hashes remain unchanged and no
technical approval is inferred. E406 remains partially demonstrated/uncredited;
counts stay 920 requirements / 1,780 historical scopes with 213 collections.
No unrelated passing cases or screenshots were replayed. See
``Qt6ParityValidation/nasa-batch-final-pass-20261030.md``.

The preceding increment added strict batch-estimation OEM cases for opposite
initial biases. Both fit and recover the independent circular truth under the
shipped NASA stopping settings, but both fail visibly and unattended: the OEM
contains one initial fitted state tagged at the final observation epoch.
``BatchEstimatorBase::IsFinalPass()`` is an unconditional false; final replay
remains missing. E406 receives no credit. A narrow singular-Keplerian text-report
fix adds one core file, bringing the tracked inventory to 52; technical approval
remains false. Eleven directly affected editor-lifetime regressions pass both
modes. The existing Estimation Unicode-report rejection assertion stays failed.
Counts remain 920 requirement / 1,780 historical scopes, with 212 ledger
collections and 1,485 checks. Zero screenshots or unrelated passing GUI replays.
See ``Qt6ParityValidation/nasa-batch-ephemeris-20261030.md`` and the core review.

The preceding increment repairs the existing sampling test's enabled-only clean
Apply assumption. All 54 original predicates and numerical bounds remain;
one added guard proves clean Apply is disabled and idempotent. All nine
directly affected cases pass both modes. The audit checks 672 OEM epochs,
4032 Cartesian values and 2688 physical invariants; all
405/405 visible boundaries are paced. No unaffected GUI replay, screenshots
or production GUI/core edit occurs. Counts stay at 920 requirement / 1,780
historical scopes, with 211 ledger collections, 208 mapped assertion pairs
and 1,483 checks. The sampling-helper compatibility gap is closed; broader
gates remain open. See
``Qt6ParityValidation/nasa-sampling-clean-apply-20261030.md``.

The preceding increment verifies final-solution-only OEM output during Target/DC
and Optimize/Yukon, visibly and unattended. All 100 exported epochs and 600
Cartesian values match the independently prescribed final circle; 46 raw trial
rows include 24 distinguishable nonfinal trajectories. All 54 inherited
predicates are retained, all 86/86 visible boundaries are paced, and no
screenshots, unrelated GUI replays or production GUI/core edits occur.
E406 remains uncredited: estimation and broader format/segment behavior are
still open. Counts stay at 920 requirement / 1,780 historical scopes, with 210
ledger collections and 208 mapped credited assertion pairs. The suite registers
1,483 checks. A reused sampling helper's enabled-only clean-Apply assumption
also needs attention before full current-build qualification. See
``Qt6ParityValidation/nasa-solver-ephemeris-20261030.md``.

The preceding increment qualifies E604 selected contact occulting bodies with a
new lunar circle and an independent finite-segment geometry oracle. None,
Luna, Mars and Luna+Mars all pass both modes. Sixteen raw intervals, 32 boundaries,
48 final-state values and 72 station coordinates are checked; maximum endpoint
error is 0.339 ms against the unchanged 2 ms limit. All 56 inherited predicates
and 207 prior mapped pairs remain intact. All 209/209 visible boundaries are
paced; zero screenshots or unrelated passing GUI replays. No production GUI or
core edit occurs. There are 210 collections and 1,481 checks; 238 requirement /
587 historical gaps and 100 narrow GUI entries remain open. See
``Qt6ParityValidation/nasa-contact-occultation-20261030.md``.

The preceding increment repairs E481 function-body validation using isolated native
parsing in the Qt frontend. All three strict cases pass both modes
without assertion edits; a new isolation witness also passes both modes. The
nine action/placement cells reject invalid bodies, including unused/nested
functions. Twelve directly affected regressions pass, with no unrelated replay.
All 48/48 visible boundaries are paced; no screenshots. Six frontend/build
files change; no engine-core file changes and technical approval remains false.
There are 209 collections and 1,477 checks; 239 requirement/587 historical gaps
remain. See ``Qt6ParityValidation/nasa-function-validation-fix-20261030.md``.

The preceding increment adds a strict nine-cell function-body validation matrix
for used, unused and nested definitions across Build, Run and Save/Build/Run.
Both modes reproduce five unmet cells: Build accepts all three invalid bodies,
and Run/SaveRun accept the unused body. All three new cases remain failed,
enabled and uncredited. Twelve valid recovery/control missions and 42/42 paced
visible boundaries are verified, with no screenshots or known passing replays.
No production/core code changed. Coverage stays 918/1,158 and 1,780/2,367; the
208 prior collections remain unchanged. There are 1,476 registered checks.
See ``Qt6ParityValidation/nasa-function-validation-20261030.md``.

The preceding increment qualifies implicit GMAT function declaration scope E486
through straight and nested cases in both modes. Actual path/call selectors,
pre-Run native resources and shared command references, GUI input edits,
explicit declaration controls and Unicode reloads accompany independent
signed/zero/fractional results. Twelve missions verify 108 raw values and
72 pre-Run reference bindings. All 24/24 visible boundaries are paced. Three
failed harness canaries are retained; every original predicate is preserved.
No passing mode was replayed, no screenshot or production/core change was made.
There are 208 collections and 1,473 checks; 240 requirement/587 historical gaps
remain. See ``Qt6ParityValidation/nasa-function-declaration-20261030.md``.

The preceding increment qualifies the six-name impulsive-burn parameter family
E575 by combining one new canonical Element1/2/3 GUI Assignment-to-Maneuver
case in both modes with two retained V/N/B alias contexts. Twelve maneuvers
and 408 raw values check independent VNB results, read/write references,
invalid Apply isolation, exact history and Unicode repeatability. All 50/50
visible boundaries are paced, including 28 new boundaries. No passing mode
was replayed, no screenshot or production/core change was made. There are
207 collections and 1,471 checks; 241 requirement/587 historical gaps remain.
See ``Qt6ParityValidation/nasa-burn-parameter-family-20261030.md``.

The preceding increment qualifies automatic-global propagator scope E496 through
two new existing/GUI-created cases in both modes. Eight missions and 24 nested
propagations verify independent circle/copy states, native global flags and
settings, source/history/external-file isolation and Unicode repeatability.
82/82 authored visible boundaries are paced; zero screenshots or known passing
case/mode replays. No production/core code changed. There are 206 collections
and 1,470 checks. The broader scoping parent E491 and 242 requirement/587
historical gaps remain open. See
``Qt6ParityValidation/nasa-function-propagator-global-20261030.md``.

The preceding increment qualifies the two finite function input/output type-category
scopes E451/E459 through six missing unattended modes and six retained visible
contexts. All named category witnesses, raw results and 22 case/mode bindings
are checked; no passing GUI mode or screenshot is repeated. There are 205
audited collections. This is finite category coverage, not all resource
subclasses/domains or the broader function rules. 243 requirement/587 historical
scopes and all broader gates remain open. See
``Qt6ParityValidation/nasa-function-types-20261029.md``.

The preceding increment adds 15 actual body model/color chooser input cases, all
passing unattended and visibly. There are 30 new contexts, 540 invalid Apply/OK
rejections, 90 commits and 150 no-propagation literal-state witnesses.
1,785/1,785 authored visible boundaries are paced; zero screenshots or known
passing case/mode replays. Eleven existing Controls scopes gain mandatory
composed appearance+retained Controls evidence; no distinct row credit is added.
No production/core edits occur. There are 204 audited collections and 1,468
registered checks. Uranus/Pluto propagated witnesses and 245 requirement/587
historical gaps remain open. See
``Qt6ParityValidation/nasa-body-appearance-20261029.md``.

The preceding increment closes the retained Toggle checkbox pacing gap:
688/688 authored visible boundaries are paced with a 5000-checkpoint budget.
Only the two previously incompletely paced visible modes replay; their
unattended passes are reused. No assertions, GUI/core code or distinct coverage
counts change. There are 203 audited collections; the older 40 unpaced boundaries
remain preserved. 245 requirement/587 historical gaps and all broader gates
remain open. See ``Qt6ParityValidation/nasa-toggle-pacing-20261029.md``.

The preceding increment qualifies Mercury Controls in both modes by evaluating
the independent circle at the reported actual epoch while preserving the
requested-time bound and all 77 existing predicate expressions. No production
or core files changed. 302/302 visible boundaries are paced; zero screenshots
and zero known passing case/mode replays. All 24 retained passing body contexts
also pass offline reanalysis. Uranus and Pluto remain strict numeric holds.
There are 202 audited collections and 245 requirement/587 historical gaps. See
``Qt6ParityValidation/nasa-body-clock-20261029.md``.

The preceding increment adds15new celestial-body control cases:12pass unattended
and visibly;Mercury,Uranus andPluto retain failing strict numeric witnesses.
Only10historical Controls scopes are credited.24qualified missions check5058
field-status records,602line probes,142picker visits,1616rejected readonly
mutation routes,346native configuration values,144native local-state components,
672raw values and4118/4118paced visible boundaries.894additional paced failed-
case prefix boundaries earn no credit. Apply-state and readonly global Edit
repairs touch3Qt files;an intermediate teardown callback fault is diagnosed and
fixed. All200prior assertion files/collections and51core hashes are preserved.
No tolerance is raised;one new-test native color read uses the supported getter
with identical RGB equality. No old passing case/mode pair is replayed. There
are201audited collections,201mapped pairs and1453registered checks.245requirement/
588historical scopes,107GUI entries,41conflicts,12disabled positives,40older
Toggle boundaries and all broader science/private-NASA/full-build/technical
approval gates remain open. Zero screenshots;see
``Qt6ParityValidation/nasa-body-controls-20261029.md``.

The preceding increment qualifies22new finite-context cases in both modes:20End
default cases across selected Begin pairs,root/For depth,Before/After,duplicate
spelling and non-Begin fallback;and two complete four-pair,both-direct-reference
rename suites with Begin/End typed focus.56missions,56insertions,56Cancel paths,
64Rename visits,32accepted renames,32stale-editor rejections,6384native values,
5376raw values,560firing bits and922/922exposed,paced boundaries are checked.
One MainWindow.cpp fix inherits selected Begin native references for End
insertion. All68original driver predicates/bytes,199prior assertion pairs,
199collections and51core hashes remain exact. No core changes;technical
approval is false. One failed canary is retained;no passing case/complete mission
replayed.64mandatory per-mode/version claims include retained End defaults.
Four new historical scopes and one strengthened Defaults scope yield200audited
collections,200mapped pairs and1438checks.245requirement/598historical scopes,
107GUI entries,41conflicts,12disabled positives,40older Toggle pacing boundaries
and broader/private-NASA/science/full-build/core-and-GUI gates remain open.
Native automatic paired-End creation is not applicable policy;the source finding
earns no execution credit. Zero screenshots;see
``Qt6ParityValidation/nasa-finite-end-context-20261029.md``.

The preceding increment completes six new default follow-up scenarios in both
modes:missing spacecraft only,pending template replacement Cancel/Yes without
unused resources,and actual new-default hardware configuration then mission
execution from either finite command. Eight missions,44command commits,
8hardware commits,4explicit ID commits,24picker visits,244native values,
208raw values,20firing bits and346/346exposed,paced boundaries are checked.
A MainWindow.cpp fix preserves omitted spacecraft NAIF IDs during unrelated Qt
source transactions while explicit assignments win. No core source changes;
technical approval remains false. All198prior assertion pairs,198collections
and51core hashes are preserved. All74original new-driver predicates stay exact;
12guards are added. Five failed new-case attempts and one pre-GUI source-binding
rejection are retained;no complete passing case or successful mission replayed.
Retained field/chooser/default evidence plus the new workflows require52claim/
mode bindings,adding4historical scopes and strengthening2Defaults scopes.
There are199collections,199mapped pairs and1416checks.245requirement/602historical
scopes,107narrow GUI entries,41formal conflicts,12disabled positives,40older Toggle
pacing boundaries and broader/paired-insertion/private-NASA/full-build/core-and-
GUI gates remain open. Zero screenshots;new derived-report compression recovers
267972489bytes. See ``Qt6ParityValidation/nasa-finite-default-workflows-20261029.md``.

The preceding increment repairs NASA-specified default finite-burn resource
creation on actual BeginFiniteBurn/EndFiniteBurn insertion. Ten new cases
qualify two bounded historical Defaults scopes in both modes:20contexts,
12atomic commits,16collision rejections,48picker visits,8automatic finite burns,
4automatic spacecraft and212/212exposed,paced visible boundaries. Native
first-declared references,Cancel/native-pointer/source/history/file isolation,
exact object/tree preservation,no invented hardware,single Undo/Redo and Unicode
roundtrips are checked. Two unchanged NoResources native passes retain original
source/binary bindings;only eight failed new expected-order cases are rerun.
Six Qt files across three classes change;technical approval remains false.
All197prior assertion pairs,197collections and51core hashes are preserved.
No unrelated passing cases or successful mission prefixes are replayed.
There are198collections,198mapped pairs and1410checks.245requirement/606historical
scopes,107narrow GUI entries,41formal conflicts,12disabled positives,40older
Toggle pacing boundaries and broader/default-hardware/private-NASA/full-build/
core-and-GUI gates remain open. Zero screenshots;new derived report compression
recovers133711747bytes. See
``Qt6ParityValidation/nasa-finite-command-defaults-20261029.md``.

The preceding increment qualifies six bounded BeginFiniteBurn/EndFiniteBurn
command control/field/invalid-input scopes with two new cases in both modes:
24missions,368rejections,40picker visits,280control censuses,2736native values,
2304raw report values,240firing bits and902/902exposed,paced visible boundaries.
An explicit near-zero-gravity,constant-mass,inertial-thrust then coast fixture
checks both selected and nonselected spacecraft. Two Qt files repair rejection
atomicity and native-supported duplicate-reference projection/form editing;
no core source changes occur. Technical approval remains false. All196prior
assertion pairs,196collections and51core hashes are preserved. Two directly
affected older regressions ran once at the intermediate validation-only
version;new duplicate-specific matrices qualify the final repairs. Diagnostic
retries repeated16successful new-case mission prefixes and552rejection prefixes;
no unrelated passing registered case was replayed. There are197collections,
197mapped pairs and1400checks.245requirement/608historical scopes,107narrow GUI
entries,41formal conflicts,12disabled positives,40older Toggle pacing boundaries
and all broader/private-NASA/full-build/core-and-GUI gates remain open. Zero
screenshots;lossless new derived-report compression recovers266560126bytes.
See ``Qt6ParityValidation/nasa-finite-command-fields-20261029.md``.

The preceding increment closes the concrete zero-eligible Pen and actual list-drag
boundaries on six existing field/control scopes, adding no unique percentage
credit. Six new focused cases pass both modes: 12missions,32native XTest drags
(24drops/8Escape cancels),40pickers,8rejections,8created XYPlots,256native state
values,128raw report values and254/254exposed,paced visible boundaries. Old field
matrices stay at their original bindings;no passing GUI case or mission prefix
is replayed. The native pointer is confined to owned Xvfb/nested Xephyr displays;
physical/host/all-platform input remains unqualified. Two harness receipt strings
now describe per-case input truthfully;execution/assertion logic and194prior
assertion pairs,195collections and51core hashes remain intact. There are no
product/core changes or screenshots.196collections,196mapped pairs and1398checks
remain inventories.245requirement/614historical scopes and all broader gates
remain open. See ``Qt6ParityValidation/nasa-plot-picker-boundaries-20261029.md``.

The preceding increment qualifies six bounded current ClearPlot/MarkPoint/PenUp/
PenDown Subscriber field/control scopes with four new cases in both modes.
Every nonempty three-XY/five-plot fixture subset plus typed ordering,duplicate
occurrences and whitespace executes against native output/state witnesses:
176missions,432rejections,160picker visits,3904native state values,2816raw report
values and1936/1936exposed,paced boundaries. Visible runs start minimized and
resizable at scale3,with5000checkpoint budget and0screenshots. No product/core
changes occur;192prior assertion pairs,194prior collections and51core hashes
are preserved. Failed new-driver corrections repeated30successful mission
prefixes;no previously passing registered GUI case is replayed unattended.
Literal native duplicate retention and OrbitView's later-publication drawing
path are checked from source-derived result oracles. There are195collections,
194mapped pairs and1392registered checks. The audit rejects1748corruptions.
245requirement/614historical scopes,107narrow GUI requirement entries,41formal
conflicts,12disabled positives,40older Toggle pacing boundaries and all broader
gates remain open. Original private field recipes,arbitrary input/resource
domains,zero-eligible Pen contexts and actual drag/drop are explicitly held.
See ``Qt6ParityValidation/nasa-plot-field-controls-20261029.md``.

The preceding increment qualifies nine concrete GroundStation/Barycenter/ReportFile
Create/Clone/Rename name matrices plus nine focused same-dialog error-to-success
cases in both modes. Three Name rows gain bounded current-operation credit,
requiring all six claims per row in each mode. The complete current52command/
10identifier-folder/three-keyword census and selected lexical/collision classes
are checked with native pointer/settings/source/history/file isolation,Cancel,
six valid-name classes,case-sensitive and math-name shadowing,automatic naming/
self-rename,exact owned source/Undo/Redo/Unicode and typed fields/reference IDs.
The broad passes are retained;focused recovery fills only the missing same-open-
dialog commit transition. No old passing GUI case is replayed.

Across36contexts:1428rejections,126accepted inputs/missions,18recoveries,
342native scalar values and342raw report values,108native strings,756spacecraft values and36
report-routing checks. All1020visible boundaries are exposed and paced at scale3
in minimized,resizable,non-fullscreen windows;no screenshots. The audit rejects
5330corruptions. Two unsupported-fixture RHS failures remain preserved;the legal
fixture retains the native station setting check and adds an Origin identity
assertion. The Create keyword hypothesis is disproved:Create/GMAT already appear
in the command factory and are rejected. No application/core changes occur.

There are194audited collections,192mapped assertion pairs and1388GUI checks;
all193prior collections,190pairs,the supporting driver and51core hashes remain.
Lossless new derived-report compression recovers231204163bytes.245requirement/
620historical scopes,107narrow GUI requirements,41formal conflicts,12disabled
positives,40older Toggle pacing boundaries,and all science/native-input/GPU/
hardware/platform/stress/provenance/current NASA/newest-build/core-and-GUI
scrutiny gates remain open. All name lengths/encodings/normalization and exact
private recipes are unqualified. See
``Qt6ParityValidation/nasa-typed-resource-names-20261029.md``.

The preceding increment qualifies the complete finite24combination matrix of
Target/Optimize parents,six child solver/control branches,and actual Append
inside/Insert before terminator actions. Each case solves signed outer goals+5
and-5 and checks independent nested solution3,loop sum6,If/IfElse branch results,
continuation42 and untouched spacecraft. All96missions,128converged clones,
1152native values and1152raw report values pass across both modes;all504visible
boundaries are exposed and paced at scale3,minimized,resizable,not fullscreen.
The audit rejects2660corruptions. Two failed fixture-isolation runs are preserved;
explicit NAIF identifiers correct automatic fresh-instance identity assumptions,
with every new and prior assertion intact. The first corrected pass is retained;
only23remaining new unattended cases run,then24first visible cases. No old
passing GUI case is replayed,no product/core changes,no screenshots.

Six consistent historical scopes are credited;18Operation/keyword parent-label
disagreements remain held despite supporting current positive evidence. There
are193audited collections,190assertion pairs and1370GUI registrations;all192
prior collections,189pairs,the supporting driver and51core hashes remain exact.
Lossless new derived-report compression recovers164431314bytes. All245requirement/
623historical scopes,107narrow GUI requirement entries,41formal conflicts,
12disabled positives,40older Toggle pacing boundaries,and science/native-input/
GPU/hardware/platform/stress/provenance/current NASA/newest-build/core-and-GUI
scrutiny gates remain open. FRC!E7/E55all-command obligations are not inferred
from six finite child families. See
``Qt6ParityValidation/nasa-solver-parent-insertion-20261029.md``.

The preceding increment qualifies17selected-parent body creation cases in both
modes and10bounded historical Add scopes. Qt now offers the native parent/type
menus,inherits and locks CentralBody,reuses the body form,commits owned source
atomically and places children beneath their native parent. Five Qt production
files change;no core file changes.13default-parent unattended passes are retained;
only four failed custom-parent fixtures rerun,then all17receive their first
visible replay. Two directly affected creation regressions pass;no unrelated old
passing case is replayed. All45new assertion expressions and188prior assertion
pairs remain exact. A descriptive fixture header moves below the custom-parent
block after a raw diagnostic identifies comment metadata,not physical mutation;
global-header attachment remains unqualified and all failures are preserved.

Across34contexts:34missions,68menus/creators/kernel pickers,34cancellations,
34duplicate rejections,306native child fields,238spacecraft values and34raw
report values. All204visible boundaries are exposed and paced at scale3 in a
minimized,resizable,non-fullscreen test window. The audit rejects977corruptions.
No screenshots;lossless new derived-report compression recovers262284116bytes.
There are192audited collections,189mapped assertion pairs and1346GUI checks.
All191prior collections,the prior supporting driver and51core hashes remain.
Three Add identities MercuryMoon/Sarturn/VenusMoon remain held.107GUI requirement
entries,245requirement/629historical scopes,41formal conflicts,12disabled
positives,40older Toggle pacing boundaries,and all science/native-input/GPU/
hardware/platform/stress/provenance/current NASA/full newest-build/core-and-GUI
scrutiny gates stay open. Settings/continuation42 do not establish orbital or
ephemeris science. See
``Qt6ParityValidation/nasa-body-parent-creation-20261029.md``.

The preceding increment finishes source applicability assessment of all2367rows
in the available2012historical matrix. This is not100percent acceptance or
private/current NASA keyword-plan review. Eighty remaining rows are individually
traced:13body Add,32unresolved body-identity mechanisms,24solver-parent structure,
10Unique solver-leaf placements and1Report item row. Seventy-nine remain held;
all carry exact source fields,fingerprints,authority hashes and next actions.

H2012:R1945 receives one bounded Report item scope through five unchanged old
cases in both modes:Variable,whole2x2Array,Array element,String and Spacecraft
property. Independent literal/array outputs,exact source,pending/persistence and
native exposure are reaudited:10retained missions,16typed values,90output bytes,
30setting rejections and60exposed paced boundaries. The audit rejects126
corruptions. No GUI case is replayed,no screenshots or source/binary snapshots
are added,and no application/core/test assertions change.

Source review is2367/2367(100percent),with0unreviewed metadata rows;demonstrated
coverage remains bounded. There are191audited collections,188assertion pairs
and1329GUI registrations. All190prior records,188assertion pairs,the prior
supporting driver and51core hashes are preserved. Explicit remaining-source
gaps list245requirement scopes and639historical scopes. The107tracked GUI
requirement entries,41formal conflicts,12disabled positives,40retained Toggle
pacing boundaries and all science/native-input/GPU/hardware/platform/stress/
provenance/current NASA/newest-build/core-and-GUI scrutiny gates stay open.
Native selected-parent body creation differs from Qt generic creation and is
identified for a focused fix/test;no creation coverage is inferred from generic
passes. See ``Qt6ParityValidation/nasa-historical-source-review-complete-20261029.md``.

The preceding increment qualifies Toggle checkbox input and native activation
for five explicit output types. Two new cases pass unattended and visibly;
all32checkbox combinations and all31nonempty subsets for both On/Off states
are checked with actual Qt mouse/Space events,empty-OK disabling,changed Cancel/
OK,pending isolation,exact Apply/history/Unicode/reopen and native references.
Across four contexts:124missions,620native activation bits,320native reference
names,868physical values,248scalar values and688native-checked viewing boundaries,
including648paced and40unpaced after the observer cap. The40boundary
pacing gap stays explicit. New5000maximum support passes a separate1100boundary
harness probe;the passing GMAT cases are retained without replay.
The independent audit rejects1756corruptions. No old passing case is replayed;
no production/core changes and no screenshots. Test-only runner/observer budget
support is extended with original assertions preserved;future replay can use2000
checkpoints after rebuilding. Retained checkbox runs are not fully paced.

H2012:R1986 receives bounded scope credit;14private LoadChangeAll bodies remain
held. Historical source review reaches2287/2367(96.6%),leaving80unreviewed rows.
There are1329GUI registrations,188mapped assertion pairs and190audited
collections;all187prior pairs,189collections,the prior supporting driver and
51core-file hashes are preserved. Lossless compression recovers64694348
derived-report bytes. The107GUI requirement gaps,41formal conflicts,12disabled
positives and all scientific/native-input/GPU/hardware/platform/stress/current
NASA/provenance/newest-build/technical scrutiny gates remain open. Native
activation does not claim output sample/file suppression or propagation science.
See ``Qt6ParityValidation/nasa-toggle-checkbox-catalogue-20261029.md``.

The preceding increment qualifies all3727finite positions of seven spacecraft
visualization sliders and reuses15unchanged attitude conversion cases in both
modes. The new slider case passes unattended and visibly;75exposed viewing
boundaries,exact forward/inverse mappings,selected gestures,Cancel/Apply/history,
Unicode persistence and native metadata/physical isolation are checked. The
independent audit rejects328corruptions. The retained attitude cases are not
replayed;their180transitions,2160pose values and840raw report values are
independently rechecked with explicit archived authority/source bindings.

H2012:R82 and R164 receive bounded historical scope credit. Visualization,tank
and impulsive-burn conversion recipes R163,R165,R286 remain held because their
conversion domains/private bodies are unspecified. Source review reaches
2272/2367(96.0%),leaving95unreviewed rows. There are1327GUI registrations,
187mapped assertion source/hash pairs and189audited collections;all186prior
pairs,the previous supporting driver,188collections and51core-file hashes are
preserved. The GUI availability patch disables transforms/resets without a
selected model,matching NASA's native panel;no core changes this increment.
Technical review remains pending. The first visible picker interruption and
test-only responder correction are preserved with all assertions intact;
the passing unattended catalogue retains its exact older source version.

Zero new screenshots;lossless compression recovers162027023derived-report bytes.
The107GUI requirement gaps,41formal conflicts,12disabled positives and all
scientific/native-input/hardware/platform/stress/current NASA/newest-build/
technical scrutiny gates remain open. See
``Qt6ParityValidation/nasa-attitude-visual-conversions-20261029.md``.

The preceding increment qualifies the selected Maneuver Burn/Spacecraft subdialogs.
Two new cases pass unattended and visibly, with 20 actual chooser visits,
16 genuinely changed-selection cancellations and four accepted changes.
Eight edited/reopened missions check 256 raw numeric values, selected impulse
and untouched-spacecraft states, masses, native command identities and configured
values. The audit rejects 672 corruptions and verifies 48 viewing boundaries.
No prior passing case is replayed; production/core code is unchanged. A new
harness callback-order interruption is preserved; all assertions remain intact.

Fourteen historical rows are reviewed: H2012:R1817 receives bounded credit;
FieldTests and twelve reference-rename/every-resource recipes remain held.
Historical source review reaches 2267/2367 (95.8%), leaving 100 unreviewed rows.
There are 1326 GUI registrations, 186 mapped assertion source/hash pairs and
188 audited collections. All 185 prior pairs, the previous supporting driver,
187 collection records and 51 core-file hashes remain unchanged. No screenshots;
lossless compression recovers 96781189 derived-report bytes. The 107 GUI
requirement gaps, 41 formal conflicts, 12 disabled positives and broader
scientific/native-input/hardware/platform/stress/current NASA/newest-build/
technical scrutiny gates remain open. See
``Qt6ParityValidation/nasa-maneuver-subdialogs-20261029.md``.

The preceding increment qualifies three bounded If/Else scopes: selected subdialogs
and the documented ten-row uniform AND/OR grids. Two new cases pass unattended
and visibly, without production/core changes or existing-pass replay. Each
executes all 1024 Boolean vectors in edited and reopened missions. Across both
modes the audit checks 8192 truth rows, 114688 numeric values, every row's
independent effect, native condition/join/operand identity, exact history and
Unicode persistence, empty/incomplete/malformed recovery and actual dialogs.
It rejects 276 corruptions and verifies 98 exposed viewing boundaries.

Historical rows H2012:R1540, R1592 and R1593 receive bounded credit. R1591
FieldTests remains held. Source review reaches 2253/2367 (95.2%), leaving
114 unreviewed rows. There are 1324 GUI registrations, 185 mapped assertion
source/hash pairs and 187 audited collections. All 184 previous pairs, the
previous supporting driver, 186 prior collection records and 51 core hashes
remain unchanged. No screenshots; lossless compression recovers 64340937 bytes.
Nested operand pickers reselect existing values; changed-selection transactions
and other field/type/operator/precedence domains remain unqualified. The 107 GUI
requirement gaps, 41 formal conflicts, 12 disabled positives and broader science,
native input, GPU/hardware/platform/stress/current NASA, newest-build integration
and technical scrutiny remain open. See
``Qt6ParityValidation/nasa-ifelse-full-grid-20261029.md``.

The preceding increment adds four native solver-menu catalogue and command cases,
all passing unattended and visibly. Per mode, it checks the entire loaded
native command catalogue in 30 positions and selects 106 templates. A new
result witness inserts GetLastState and UpdateDynamicData through their actual
forms, checks Cancel, history and Unicode reopen, and validates 48 raw report
values across four missions, native state identity and 12 live display cells.
The audit rejects 206 corruptions and verifies 254 exposed viewing boundaries.

This found and fixed two missing menu templates, missing typed controls for
unconfigured native built-ins, and a core UpdateDynamicData array-comma parser
bug. All 184 existing mapped assertion source/hash pairs and the previous 50
core-file hashes remain unchanged. The core inventory grows to 51 files. Two
directly affected regressions pass unattended; no unrelated known pass is
replayed. Technical approval remains false. The new parser's source attribution
is explicit: its immutable native library matches the built library, while
its exact source and patch are separately preserved after execution. Future
snapshots now freeze that source. Compiler provenance remains unqualified.

Six broader historical menu/unique rows are reviewed and held, not credited:
H2012:R1260, R1298, R1336, R1392, R1697 and R1698. Historical source review is
2249/2367 (95.0%), leaving 118 unreviewed rows. There are 1322 registered GUI
checks and 186 audited collections, including this supporting increment with
zero new row credits. The 107 GUI requirement gaps, 41 formal conflicts and
12 disabled solver positives remain. Full menu actions, distinct If/If-Else
paths, private recipes, other built-ins and their selector dialogs, scientific
truth, native input, GPU/hardware/platform/stress, current NASA provenance,
full newest-build integration and core/GUI scrutiny remain open. No screenshots
were created; lossless compression recovered 257703958 derived-report bytes.
See ``Qt6ParityValidation/nasa-solver-menu-catalogue-20261029.md``.

The preceding increment qualifies four literal solver SubDialogs rows. A new
Optimize/VF13ad Vary writable-property browser case passes unattended and
visibly; Achieve gains its missing unattended pass and retains its original
visible pass. Minimize and constraint browser evidence is reused exactly.
The new Vary case checks actual Cancel/OK, invalid/readonly filtering, exact
property-only Apply/history/Unicode reopen, physical initial/probe values,
active-constraint optimum 2/cost 1, native registered control identity and
unchanged unrelated/configured values. The audit checks 164 modern numeric
report values, 28 new native control values, ten retained legacy analytic
case records and rejects 118 corruptions. Only the new case receives visible
replay (8 exposed paced boundaries); no known pass is repeated. An initial
launcher-only failure is preserved and corrected to native X11 with the
assertion intact. A native name-only alias preserves its exact unattended
pass. No production/core changes or screenshots. All 183 previous assertion
pairs, 184 prior collections and 50 scientific core hashes are preserved.
Totals are 185 collections, 184 assertion pairs and 1318 GUI checks. Source
review reaches 2243/2367 (94.8%), leaving 124 unreviewed historical rows and
107 uncredited GUI requirements. All private-keyword/other-subdialog/input/
scientific/native-input/platform/stress/current-NASA/newest-build/core-and-GUI
scrutiny gates remain. Newly derived reports compress losslessly by 96362181
bytes. See ``Qt6ParityValidation/nasa-solver-subdialogs-20261029.md``.

The preceding increment closes the two unattended constituent gaps for the
Minimize operand-type and Assignment RHS finite-list headings. Only the
existing Minimize scalar-property and InlineUserFunction cases run in the
missing unattended mode; both pass unchanged assertions. Original visible
and all other child-category evidence is retained without replay. The audit
rechecks 60 earlier claim bindings and adds two, validates 20 scalar values
across two new and two retained case contexts, and rejects 132 corruptions.
Four existing requirement scopes and one historical subdialog scope gain
both-mode evidence. Distinct coverage stays 913 requirements / 1716 historical
scopes; all four finite lists from the preceding review now have both modes
across their original versions. All 183 assertion pairs, 183 prior collections,
50 scientific core hashes and the preceding GUI fix remain unchanged. No code
changes or screenshots. There are 184 collections and 1317 registered checks.
The full current-build suite, broader scientific/native-input/platform/
private-NASA/core-and-GUI scrutiny gates remain open, with 128 historical rows
unreviewed and 107 GUI requirement entries uncredited. See
``Qt6ParityValidation/nasa-unattended-type-gates-20261029.md``.

The preceding increment links retained evidence to four bounded finite-list
requirement headings: point-mass body categories, Minimize operand types,
NonlinearConstraint operand types and Assignment right-hand-side data types.
All enumerated child categories are mandatory. The Minimize property and
inline-function constituents remain visible-only; their two headings do not
claim unattended qualification. Two historical multi-spacecraft scopes reuse
exact Synchronized/Independent two-numerical-group passes; two ambiguous
multiple-parameter/propagator rows remain held. Zero execution, builds,
screenshots, snapshots or code changes. The audit checks 60 claim contexts
from 32 retained test contexts, rechecks 24 propagated states (144 Cartesian
components) and raw Assignment literals/arithmetic, and rejects 300 corrupted
evidence records. Legacy constraint temporary reports remain unavailable;
original terminal/source analytic assertions are retained with that limitation.
All 183 current assertion pairs, 182 prior collections, 50 scientific core
hashes and the preceding GUI Append-fix hash are unchanged. The ledger has
183 collections and 1,317 registered checks. Historical source review reaches
2,239/2,367 (94.6%), leaving 128 unreviewed rows and 107 uncredited GUI
requirement entries. Forty-one formal conflicts, twelve disabled solver
positives and all broader scientific/native-input/platform/current-NASA/
newest-build/core-and-GUI scrutiny gates remain open. See
``Qt6ParityValidation/nasa-finite-list-reuse-20261029.md``.

The preceding increment qualifies twenty literal IfElse-parent/Else-boundary
scopes. Six new four-position cases pass both modes, but NASA legacy source
review exposes a Qt Append mismatch: IfElse Append must insert before Else,
not before EndIf. MainWindow.cpp now selects the chosen branch's own Else;
other branches retain their terminator. The production GUI patch is tracked
separately;all 50 scientific core hashes remain unchanged. Only six affected
Append operations are rerun in each mode using new Append-only registrations;
the other three positions retain exact passes. Twelve initial wrong-Append
missions are preserved and excluded from coverage. The audit qualifies48selected
missions,1268child native values,420parent counter values,96true/false entries
and32converged clone checks,rejecting2504corruptions. The raw four-cohort traces
include610exposed paced boundaries,including excluded initial Append attempts.
No screenshots. Original assertions/defaults and181other assertion pairs remain;
4490production reference rebindings and both test-driver source versions are
explicit.182collections,183mapped assertion pairs and1317GUI checks are tracked.
Historical source review2235/2367(94.4%),132unreviewed;111GUI requirement gaps,
41formal conflicts,12disabled solver positives,six newly held ambiguous/unique
rows and all prior qualification/core scrutiny gaps remain. Lossless new report
compression recovers127941792bytes. See
``Qt6ParityValidation/nasa-else-parent-20261029.md``.

The preceding increment qualifies twelve literal While-parent Append/BeforeEndWhile
scopes across For, If, If/Else, While, Target and Optimize. Six new cases pass
unattended and visibly with 24 missions, 1,052 child native values, 348 parent
counter values, 48 parent entries, 24 converged clone checks and 268 exposed
visible boundaries. The independent audit rejects 1,832 corruptions. All prior
assertions and default If/Outer fixtures remain intact; optional While fixtures
extend two test drivers, with no production/core changes or old passing replay.
There are zero new screenshots. Historical source review reaches 2,209/2,367
(93.3%), leaving 158 unreviewed rows and 111 uncredited GUI requirements.
All 180 prior collections and 50 tracked core hashes are preserved; the new
ledger has 181 collections, 183 mapped assertion pairs and 1,305 GUI checks.
Twelve disabled solver positives and 41 source conflicts remain. Lossless
compression recovers 63,198,495 bytes of newly derived reports. Parent zero/many
iterations, other parents/positions and all prior held/current-build/science/
platform/native-input/core scrutiny gates remain open. See
``Qt6ParityValidation/nasa-while-parent-20261029.md``.

The preceding increment qualifies eight literal If-parent Append/Before-EndIf rows.
Six new current capability fixtures cover For,plain If,If-with-Else,While,Target
and Optimize;four copied-column IfElse/While rows remain held despite passes.
Twelve passing contexts execute24missions with24insertions,24cancelled drafts,
60control-body observations,32solver before/after samples,1052native values and
24converged clone checks.268new paced visible boundaries and1412rejected evidence
corruptions;zero screenshots. The existing drivers gain an explicit Inner parent
option restricted to two positions;Outer remains the default and every prior
assertion remains. Exact source patches and197/26live hash rebindings are recorded;
old passes are retained without replay.181other assertion pairs,179prior records
and50core hashes are unchanged. No production/core changes.180collections,
183current assertion pairs and1299registrations are tracked.62,777,653bytes
recovered by lossless new derived-report compression. Source review2197/2367
(92.8%),170unreviewed;111GUI requirements,41conflicts,12disabled positives,parent
false/Else/other positions,all prior held/scientific/native-input/OS/GPU/hardware/
platform/stress/current NASA/full newest-build/core scrutiny remain open. See
``Qt6ParityValidation/nasa-if-parent-20261029.md``.


The preceding increment qualifies twelve exact For-parent Append/Before-EndFor rows
for For,plain If,If-with-Else,While,Target and Optimize using retained control/
solver evidence. Zero execution,passing-case replay,GUI launch or new snapshot.
24retained contexts qualify24selected missions with60control-body observations,
32solver before/after samples,1052native state values and24converged clone checks.
The independent retained-artifact audit rejects1340corruptions.776retained visible
case-level boundaries include other original grouped missions;only BranchAppend
or EndBefore qualifies each new row. Source,fixture,log/JUnit,hierarchy,parent
terminator and raw results bind exact archived binaries/references. Post-execution
authority context is distinguished from archived execution inputs;no newest-build
or core/frontend equivalence is inferred. All183assertion pairs,178prior records
and50core hashes remain intact.179collections and1293registrations are tracked.
Twelve new source reviews bring2185/2367(92.3%),182unreviewed.111GUI requirements,
41conflicts,12disabled positives,other parent/Else/terminator/label scopes,all prior
held/scientific/native-input/OS/GPU/hardware/platform/stress/current NASA/full
newest-build/core scrutiny remain open. Compact new reports retain every source
key/status and full selected evidence without duplicating immutable inventories.
See ``Qt6ParityValidation/nasa-for-structure-reuse-20261029.md``.

The preceding increment qualifies five actual Barycenter checklist scopes:available
first Jupiter,interior Neptune,last Venus,Jupiter/Neptune pair,and Clear selection.
Legacy Add appends to selected bodies;available rank is not selected insertion.
Ten passing contexts supply22picker visits,six Clear actions,two rejected empty
GUI Applies,180body-state inputs,60native spacecraft values and70checked mission
values,with92paced exposed visible boundaries and784rejected evidence corruptions.
Independent mu-weighted aggregation checks the real Center-origin output using
shared native ephemerides;it does not qualify physical ephemeris truth. Exact
source/history/Unicode isolation remains checked. Original fixture failures and
Clear timeout are retained;canonical assignment placement and valid no-op Apply
recovery were corrected without production/core changes or assertion weakening.
All182prior assertion pairs,177prior collections and50core hashes remain intact.
178collections,183current assertion pairs and1293registrations are tracked.
Zero screenshots;124,661,769bytes recovered by lossless derived-report compression.
34complete body Apply/Controls-status rows are source-reviewed and held. Source
review2173/2367(91.8%),194unreviewed.111GUI requirements,eightliteral Barycenter
fixture rows,41conflicts,12disabled positives,all prior held scopes and actual
drag/scientific/native-input/OS/GPU/hardware/platform/stress/current NASA/full
newest-build/core scrutiny remain open. See
``Qt6ParityValidation/nasa-bary-membership-20261029.md``.

The preceding increment qualifies eleven Barycenter/ReportFile complete-active-field
clone scopes with two new fixtures using the existing clone driver. All16declared
editable fields change through actual controls;complete editable-property copies,
independent original/clone edits,both deletion histories,source/file isolation and
continuation9 are checked. Four passing contexts supply32committed field changes,
512native stage values,eight deletion outcomes,four continuation results and
122paced exposed visible boundaries. The audit rejects1356evidence corruptions.
A test-helper readback now accepts exact selected-index item data for delimiter
labels while retaining the original exact-text path;all prior assertions remain.
Forty live hashes are explicitly rebound;old receipts remain unchanged and the
eight previous clone families are not replayed. No production/core changes;
181other assertion pairs,176prior collections and50core hashes are intact.
177collections,182current assertion pairs and1288registrations are tracked.
Zero screenshots;61,972,358bytes recovered by lossless derived-report compression.
24historical rows are reviewed;13literal named-body/picker-specific rows remain
held. Source review2139/2367(90.4%),228unreviewed.111GUI requirements,41conflicts,
12disabled positives,all prior held scopes and scientific/native-input/OS/GPU/
hardware/platform/stress/current NASA/full newest-build/core scrutiny remain
open. Active ReportFile output/file independence and barycenter dynamical/frame
truth are not inferred. See
``Qt6ParityValidation/nasa-clone-reference-20261029.md``.

The preceding increment qualifies twelve finite command-panel operation scopes:
Save,BeginFiniteBurn and EndFiniteBurn each supply invalid Apply recovery,
Help,Maximize/Restore and Tab navigation. Three new fixtures reuse the unchanged
command-mechanism driver. Six passing contexts check24rejections,12Help routes,
24window actions,160focus steps and12independent scalar report values,with
93paced exposed visible boundaries and450rejected evidence corruptions.
The original two finite-burn passes are retained;only Save is retried after
fixing its omitted offline-guide include. That one-line documentation delivery
fix exposes the original under-construction Save topic;complete Save help is
still unqualified. No C++/core changes;all182prior assertion pairs,175prior
collections and50core hashes remain intact.176collections and1286registrations
are tracked. Zero screenshots;92,807,474bytes recovered by lossless new derived
report compression.37historical rows are reviewed;25remain held for literal
MinRestore identity,complete fields/defaults/child dialogs,reference chains or
all-field load scope. Source review2115/2367(89.4%),252unreviewed.111GUI
requirements,41conflicts,12disabled positives,all prior held rename/close/clone/
Vary rows and scientific/native-input/OS/GPU/hardware/platform/stress/current
NASA/full newest-build/core scrutiny remain open. See
``Qt6ParityValidation/nasa-command-panel-completion-20261029.md``.

The preceding increment qualifies three finite GUI/script/console delivery
requirements:All Others!E17,E18,E19. Two actual GUI edit/toolbar cases hand the
exact saved script to real console processes through explicit/positional startup
routes. Four passing contexts check40independent report values,40native GUI
values,40console values,12real processes and eight invalid-console rejections.
The first visible replays open the actual read-only report viewer and supply
28paced exposed boundaries. The audit rejects502evidence corruptions. Both
unattended passes and prior FRAC3/4 child evidence are retained without replay.
Only a visible-only report-viewer block separates the explicitly preserved
source versions. No production/core change;50tracked core hashes,181prior
assertion pairs and174prior collections are intact.175collections,182current
assertion pairs and1283registrations are tracked. Zero screenshots;153,694,308
bytes recovered by lossless new derived-report compression. DocumentedASCII
contents and Unicode filenames are distinct:non-ASCII console-content rejection
is separately checked,and full GUI/CUI encoding parity remains unresolved.
Twelve historical name/optional-backend/full-project summary rows are reviewed
and held. Source review2078/2367(87.8%),289unreviewed;111GUI requirement entries,
51rename rows,50exit rows,90clone rows,41conflicts,12disabled positives,two Vary
ambiguities and scientific/native-input/OS/GPU/hardware/platform/stress/current
NASA/full newest-build/core approval gates remain open. See
``Qt6ParityValidation/nasa-interface-delivery-20261029.md``.

The preceding increment qualifies four finite SPK output requirement scopes:
FRR!E387,E390,E395,E396. Five new native cases cover every supported odd order
1/3/5/7/9,nonuniform accepted integration knots,and TDB clocks at three UTC
starts including2015/2016leap transitions. Native CSPICE reads and a separate
direct-DAF/LSK Python decoder check218epochs and1308Cartesian components.
Shared hashed FK5/ICRF table samples use independent polynomial/Rodrigues
algebra;frame-definition truth is not inferred. The initial oracle frame
mismatch is retained;corrected expected values keep the original tolerances.
Ten passing contexts reject18invalid-order inputs and1668evidence corruptions,
with123paced visible boundaries and zero screenshots. One passing unattended
case is retained while the other four run;no old passing scenario is replayed.
No production/core changes;all50tracked core hashes and173prior collections/
180assertion pairs remain intact.174collections and1281GUI registrations are
tracked.51body-rename rows are source-reviewed without credit:30default-body
exclusions,9user-body policy conflicts and12literal-identity gaps. Source review
reaches2066/2367(87.3%),leaving301unreviewed rows.114GUI requirements,50exit rows,
90clone rows,41formal conflicts,12disabled positives,two Vary ambiguities and
broader scientific/native-input/OS/GPU/hardware/platform/stress/current NASA/
full newest-build/core approval gates remain open. See
``Qt6ParityValidation/nasa-spk-output-20261029.md``.

The preceding increment qualifies33panel-close scopes:30through retained exact
passes and three new clean/pending MDI title-bar cases using the unchanged
lifecycle driver. Native child-close source resolves panel No/Yes as veto/retain
versus discard/close;Qt Cancel/Discard supplies those outcomes.83source rows
are reviewed,while12literal-body rows and38program-exit rows remain held.
Program_ExitNo/Yes cannot be replaced by child-only checkpoint names:the main
frame has different child-loss,save-script and running-mission prompts.
36case contexts bind66claims,124recorded decisions,54independent report values,
28raw native/configured panel-field values,30actual title-bar hit rectangles and
390paced visible boundaries. The audit rejects844corruptions.
No old passing case was rerun. No assertion/production/core source changes;
all180assertion pairs,172old collections and50core hashes remain intact.
There are173audited collections and1276registered checks. Source review is
2015/2367(85.1%),leaving352unreviewed. Zero screenshots. The118GUI requirement
entries,90clone rows,41formal conflicts,12disabled solver positives,two Vary
ambiguities and broader scientific/native-input/OS/GPU/hardware/platform/stress/
current NASA/full newest-build/core approval gates remain open. See
``Qt6ParityValidation/nasa-exit-decisions-20261029.md``.

The preceding increment qualifies40 supported-family ChangeAll/Clone operation
scopes with eight new native cases in both modes. Their125declared active fields
are changed through actual controls;complete editable-property copies,independent
original/clone edits,both deletion outcomes,history/source/Unicode isolation and
continuation9 are checked.16contexts deliver250field edits and
9024native stage values,32deletion outcomes and
618paced exposed visible boundaries. The audit rejects
2460corruptions. Five existing unattended passes are retained
across later driver/frontend fixes rather than replayed. A narrow MainWindow
clone fix preserves implicit effective orbit/target colors;two directly affected
existing lifecycle regressions pass. No core file changes;all50tracked hashes
and171prior collections/179assertion pairs remain intact.172collections and
1273registrations are now tracked. Zero screenshots. Source review remains
1932/2367(81.6%);435unreviewed rows,90other clone rows,118GUI requirements,
41formal conflicts,12disabled positives,two Vary ambiguities and all broader
scientific/native-input/OS/GPU/hardware/platform/stress/current NASA/full
newest-build/core approval gates remain open. See
``Qt6ParityValidation/nasa-clone-all-20261029.md``.

The preceding increment qualifies five selected panel button/Apply scopes through
exact retained passing evidence,with no new test execution. ChemicalTank and
ChemicalThruster each supply Cancel and positive OK assertion proofs;Spacecraft
Visual supplies actual positive Apply/native readback/history evidence. Six
retained case contexts give10row claims,eight legacy button assertion records,
four visual Apply commits,10visual field readbacks and eight independent
continuation values. The audit rejects92corruptions. Retained visible Visual
has31paced boundaries;the two older legacy cases show312native exposure events,
with their original Qt click/pacing limits retained.24old screenshot records
remain in original receipts;their PNGs were already removed. Zero new screenshots.
All179assertion pairs,170old collections and50core hashes remain unchanged.
The same batch reviews130ChangeAll/Clone rows without credit:40supported-family
all-field/isolation proof gaps,30user-body guide/runtime conflicts,55current
default-body exclusions and five Propagator exclusions. No row is retired or
converted to a passing negative. Source review becomes1932/2367(81.6%),leaving
435unreviewed. Demonstrated coverage is902/1158(77.9%) and1561/2367(65.9%),with
171collections and1265registrations. The118GUI requirement entries,41formal
conflicts,12disabled nested positives,two Vary ambiguities and broader scientific/
native-input/OS/GPU/hardware/platform/stress/current NASA/full newest-build/core
approval gates remain open. See
``Qt6ParityValidation/nasa-resource-clone-batch-20261029.md``.

The preceding increment qualifies Toggle and Global changed-object loop scopes
through two new cases in both modes. Toggle changes ReportFile precision and
activation on each actual For pass;native flags/settings and nine enabled output
rows versus13untoggled control rows check the results. Global changes a Variable,
Array and String;three passes check local/global-store transfer,stable identities
and a real function reading/updating the same shared values. Across four bound
contexts,the audit checks48report rows,474report/native values,44configured values,
18string boundaries,72ownership/identity records and106automatic-output values.
All70visible boundaries expose and pace in minimized,resizable scale3 desktops;
1658corruptions are rejected. Unsupported new-fixture script forms were corrected
without removing numerical/native assertions. Both Global passes retain their
exact original source versions across Toggle-only corrections;no unrelated pass
replays. Zero new screenshots and no product/core changes. All178old mapped
assertion pairs,169old collections and50core hashes are preserved. Coverage
becomes902/1158(77.9%) and1556/2367(65.7%),with170collections,179assertion pairs
and1265registrations. Lossless new derived-report compression recovers176,120,855
bytes. Only the two internally ambiguous Vary rows remain held in this loop list;
118GUI requirement entries,570unreviewed historical rows,41formal conflicts,
12disabled nested positives and scientific/native-input/OS/GPU/hardware/platform/
stress/current NASA/full newest-build/core approval gates remain open. See
``Qt6ParityValidation/nasa-subscriber-global-loop-20261029.md``.

The preceding increment qualifies two changed-input Target/Optimize loop scopes by
reusing exact original visible/unattended solver-tree evidence. No new test run,
production change or screenshot occurs. Each same inserted branch executes twice
in an actual For loop with changed runtime Variable I;independent linear Target
or quadratic Optimize solutions shift by1. Four direct/nested positive placements
per family give16selected missions,32branch executions,64before/after samples,
1040native state values and48converged-clone checks across both modes. The audit
rejects1532corruptions. Four retained visible case routes contain260fully paced
boundaries;that case-level count also includes Boundary groups outside-loop
missions,which receive no new loop credit. All178old mapped assertion pairs,
168old collections and50core hashes remain intact. The original MainWindow
hash is retained explicitly;later solver-leaf menu/default/capacity/commit gates
are not inferred qualified by these old passes. Coverage becomes902/1158(77.9%)
and1554/2367(65.7%),with169collections and1263registrations. Toggle,Global and
two Vary column-ambiguity loop rows remain held. The118GUI requirement entries,
570unreviewed historical rows,41formal conflicts,12disabled nested positives,
scientific/native-input/OS/GPU/hardware/platform/stress/current NASA/full newest-build
and core technical-approval gates remain open. See
``Qt6ParityValidation/nasa-solver-object-loop-reuse-20261029.md``.

The preceding increment qualifies four changed-object burn/propagation-loop scopes
through three new cases in both modes. A real three-pass For loop revisits the
same Maneuver or ignition/powered-propagation/shutdown/coast command objects.
Delta-V/Isp or attached-thruster duty/scale changes each pass. Independent rocket
mass and constant-acceleration/coast oracles check72report rows,1512report values,
1032native values and72configured values. All112visible boundaries expose and pace
in minimized,resizable scale3 desktops. The audit rejects2944corruptions.
The initial impulse build exposed a core Validator name collision: writable
ImpulsiveBurn.Isp was mistaken for a spacecraft/thruster system parameter.
Owner-aware direct-field resolution repairs it;original assertions and fixtures
are unchanged. Four directly affected read-only/tank/thruster regressions pass.
The two finite cases retain their exact original passing builds without replay.
All177old mapped assertion pairs and167prior collections are preserved;only
Validator changes among the50tracked core files. Technical approval remains
false. The new inventory is
``build/nasa-coverage/core-change-review/20261029-burn-parameter-owner/inventory.json``.
Coverage becomes902/1158(77.9%) and1552/2367(65.6%),with168collections,
178assertion pairs and1263registrations. Zero new screenshots;lossless compression
of new derived metadata recovers146,265,504bytes. Six changed-object loop families,
two Vary source-column ambiguities,12disabled nested solver-leaf positives,
41formal conflicts,118GUI requirement entries,570unreviewed historical rows and
science/native-input/OS/GPU/hardware/platform/stress/current NASA/full newest-build
qualification remain open. See
``Qt6ParityValidation/nasa-burn-object-loop-20261029.md``.

The preceding increment qualifies three changed-object conditional-loop scopes.
Three new no-Else cases cover Variable,Array and same-frame Spacecraft operands
in both modes. First unattended While equality/inequality contexts complement
six retained If/Else and While contexts;no existing pass replays. Across14bound
contexts,the audit checks42report rows,330report values,30native values and
30configured values,and rejects420corruptions. All48new plus48retained explicit
visible boundaries expose and pace in minimized,resizable scale3 desktops.
No new screenshots or product/core changes occur. Original48historical screenshot
records remain in old receipts,but the images were already removed. All176old
mapped assertion pairs,the analytic Yukon source,50core hashes and166old
collections remain unchanged. Coverage becomes902/1158(77.9%) and1548/2367(65.4%),
with167collections,177assertion pairs and1260registrations. Exact older execution
versions remain explicit. Lossless new derived-report compression recovers
58,360,057bytes. Ten other loop families,including two Vary column ambiguities,
12nested positive solver-leaf conflicts,41formal source conflicts,118GUI entries,
570unreviewed historical rows and broader science/native-input/hardware/platform/
stress/current NASA/full newest-build/core approval gates remain open. See
``Qt6ParityValidation/nasa-conditional-object-loop-20261029.md``.

The preceding increment qualifies five bounded Add Many solver-leaf operations
through five new cases in both modes. Three accepted insertions check4control
Target/Optimize registration,4linear goals,3intersecting inequalities,and one
Minimize in each of3separate Optimize sequences with second-objective rejection.
Independent full-rank linear/separable convex-quadratic solutions agree with
actual reports,native values and registered solver clones. Ten missions execute
14solver branches,30commits,30positiveCancel paths,30cleanApply checks,96typed
edits,12invalidApply/OK actions,6capacity-menu checks,140report values,100final
native values,32control values and28goal identities. All286visible boundaries
expose and pace in initially minimized,resizable scale3 desktops. No screenshots,
unrelated passing replay or retry occurs. Audit320negative checks pass.
All175old mapped assertion pairs,the analytic Yukon source,50core hashes and
165old collections are preserved. No product/core behavior changes. The ledger
becomes902/1158(77.9%) and1545/2367(65.3%),with166collections,176assertion pairs and
1257registrations. Lossless new derived-report compression recovers58,098,451bytes.
The exact private historical recipes and arbitrary multiplicity remain unknown.
Twelve nested positive solver-leaf conflicts stay disabled and uncredited;
41formal source conflicts,13loop families,118GUI requirement entries,570unreviewed
historical rows and broader science/native-input/hardware/platform/stress/current
NASA/full newest-build/core approval gates remain open. See
``Qt6ParityValidation/nasa-solver-many-20261029.md``.

The preceding increment qualifies six actual Minimize/NonlinearConstraint insertion
operations at legal control-flow boundaries in both modes. Before/after the For
header and after EndFor remain distinct GUI actions. Two-pass loops make the
independent optimum depend on position:Minimize U3/U4 and constraint U3/U5.
Twelve missions check12commits,12Cancel drafts,24typed edits,144actual debugger
samples,168report/native values and168exposed/paced visible boundaries. No
unrelated passing replay,screenshots,product changes or core changes occur.
All174old mapped assertions,the native Yukon driver,50core hashes and164old
collections remain unchanged. Coverage becomes902/1158(77.9%) and1540/2367(65.1%),
with165collections,175mapped assertion pairs and1252registrations.

Twelve additional nested positive hypotheses fail at commit. NASA's wx menu
and Qt expose these leaves inside control logic,but the native interpreter
explicitly prohibits that nesting;NASA main corroborates the prohibition. The
original positive assertions and failures are preserved,with registrations
explicitly disabled under a source-conflict label. No failure is converted to a
passing negative and no new core semantics are enabled. This adds one formal
source conflict(41total),not12coverage credits. The earlier nested-context menu
availability proof did not establish positive insertion. Source review remains
1797/2367(75.9%),570unreviewed;17solver-leaf rows,13loop families,118GUI requirement
entries and broader science/native-input/hardware/platform/stress/current NASA/
full newest-build gates remain open. The first unattended mapping is explicitly
post-execution;exact source/binary/input proofs are retained. Audit322negative
checks pass. Lossless new derived-report compression recovers116,512,727bytes.
See ``Qt6ParityValidation/nasa-solver-leaf-boundaries-20261029.md``.

The preceding increment repairs Yukon constraint identity and elastic recovery.
The unchanged OptimizeCapacity case now gives U4/Cost4 and passes unattended
and visibly. Feasible/no-objective and nonlinear-curve/active-bound GUI
regressions pass unattended. All149 explicit Capacity viewing boundaries expose
and pace in a minimized,resizable scale3 desktop;zero screenshots are saved.
Its first visible receipt hit the default24-checkpoint pacing cap and remains
unqualified for full pacing;only that case repeats with max-checkpoints1000.
Twelve native support cases have exact versioned passing evidence:five retained
first-fix passes,one retained scaled pass and six final-recovery passes. The new
slack endpoint assertion also rejects the preserved original library. Existing
174 mapped assertion pairs and all48 prior core files are unchanged. Yukon.cpp
and NLPFunctionGenerator.cpp bring the tracked core inventory to50;technical
approval remains false. The 92 negative audit checks reject corrupt results,
contexts and proof bindings. FRC!E13's context extension is now conjoined from
explicitly versioned claims;no new numerator is granted. Coverage stays
902/1158(77.9%) and1534/2367(64.8%),with1234 runner registrations. Lossless new
inventory/plan compression recovers86,860,373 bytes. See
``Qt6ParityValidation/nasa-yukon-constraint-identity-20261029.md``. The23remaining
solver-leaf rows,13loop families,118GUI requirements,570unreviewed historical
rows and all broader science/native-input/hardware/platform/stress/current NASA/
full newest-build gates remain open.

The preceding partial increment qualified five negative empty-root solver-leaf
scopes. Three new GUI cases pass unattended/visibly: root restrictions, Target
contexts/two controls, and Optimize objective insertion. Enclosing-solver default
binding, template filtering, one-objective capacity, source-bypass/order guards
and after-terminator context are fixed in the frontend. 78 context queries,
36 invalid Apply/OK actions, six positive insertions, 72 report/native values
and 339 exposed/paced visible boundaries qualify. Root's unattended pass is
retained through a closing-node-only correction. No core source changed at that checkpoint;no
unrelated passing replays, screenshots or dataset copies. All 173 old assertions,
48 core hashes and 162 collections remain. Coverage becomes 902/1158 (77.9%)
and 1534/2367 (64.8%): 163 collections, 174 assertion pairs, 1228 current runner
registrations including six new analytic support checks. Source review remains
1797/2367 (75.9%), 570 rows unreviewed. Lossless new derived-report compression
recovers 115,831,873 bytes.

At that preceding checkpoint,the fourth case,OptimizeCapacity,failed: valid U>=3 and U>=4 for
minimizing (U-2)^2 produce NaN instead of U4/Cost4. Independent analytic Yukon
state-machine probes confirm false convergence at U3 for duplicate/offset
constraints and failure with initially matching nonlinear gradients. Three
native controls pass and are retained; only failed probes repeat for immutable
preservation. No fixture/assertion is weakened. Whole FRC!E13 extension and the
full context increment were then unqualified. The audited core repair above
supersedes that failure while preserving its receipts. 23
other solver-leaf rows, 13 loop families, 118 GUI requirements and all broader
provenance/science/native OS/GPU/hardware/platform/stress/core/full newest-build
gates remain open. See
``Qt6ParityValidation/nasa-solver-context-partial-20261029.md``.

The preceding increment qualifies 18 previously held Target/Optimize branch-tree
scopes through six new cases in both modes. Complete legal solver branches are
inserted at nine ordinary control-tree positions; one observation follows each
branch terminator. Independent linear target/quadratic optimum checks include
actual converged DifferentialCorrector/Yukon clones and one registered control.
36 missions produce 120 before/after samples, 2028 native state values and 96
converged clone/control observations. 44 complete branches and 36 Cancel paths
qualify. All 434 visible boundaries expose and pace in initially minimized,
resizable scale 3 desktops. Two unattended Root canaries are retained; only the
four other cases run afterward. No failures, production changes, unrelated
passing replay, screenshots or dataset copies. All 172 old assertion pairs,
48 core hashes and 161 collections remain unchanged. The ledger becomes
902/1158 (77.9%) and 1529/2367 (64.6%), with 162 collections, 1218 checks and
173 assertion pairs. Lossless derived-report compression recovers 86,670,928
bytes. Source review remains 1797/2367 (75.9%), 570 rows unreviewed. 28 solver-leaf
rows, 13 other changed-object-in-loop families, 118 GUI requirements, 40 formal
conflicts and current NASA/science/native OS/GPU/hardware/platform/stress/core48/
full newest-build qualification remain open. See
``Qt6ParityValidation/nasa-solver-tree-positions-20261029.md``.

The preceding increment qualifies one Report changed-object-in-loop scope through
three new cases in both modes. Runtime Precision,formatting and Unicode filename
changes check40 native boundaries,320 typed values,360 settings observations and
20 output rows. All161 explicit visible boundaries expose and pace in initially
minimized,resizable scale3 desktops. Persistent stream alignment and the GUI's
configured-versus-running report filename defects are fixed without changing
assertions. The Precision unattended pass is retained;only two actual failures
retry. No unrelated passing replay,screenshots or dataset copies. All171old
assertion pairs and160collections remain;the already tracked ReportFile.cpp core
file changes while47other core hashes remain unchanged. Core48 technical scrutiny
stays open with a new inventory. Derived-report compression recovers114,260,079
bytes. Source review reaches1797/2367(75.9%),570rows unreviewed.13other loop rows
remain held,including two Vary column ambiguities. Coverage becomes902/1158(77.9%)
and1511/2367(63.8%);161collections,1212checks,172assertion pairs.118GUI obligations,
40formal conflicts,current NASA/science/native OS/GPU/hardware/platform/stress/core48/
full newest-build qualification remain open. See
``Qt6ParityValidation/nasa-report-runtime-loop-20261029.md``.

The preceding increment qualifies72 historical tree-position scopes across eight
ordinary command families and nine positions.24 new cases pass in both modes;
21 ordinary unattended contexts are retained after a Global-only oracle fix.
Actual context menus/templates/source editor,Cancel,positive OK and retained
Apply,exact source/history/Unicode persistence,and native/visible parents qualify.
An explicitly instrumented non-mutating Count assignment follows each selected
command.144 missions produce480 native before/after observations and6864 native
state/scalar values,with independent circle/impulse/scalar and Report/Save/
Toggle/Global-store/thruster-activation checks.176 selected insertions and144
Cancel paths qualify. All1736 explicit visible boundaries expose and pace in
resizable,initially minimized scale3 desktops. Global initialization flags are
distinguished from execution-time local/global store movement. Initial fixture/
Global-oracle failures remain uncredited. No production/core changes,unaffected
replay,screenshots or data copies. All170 old assertions,48 core hashes and159
collections remain unchanged.160 collections,1209 checks,171 assertion pairs.
Lossless derived-report compression recovers171,196,851 bytes. Source review reaches
1783/2367(75.3%),584 historical rows unreviewed.46 solver-context rows are reviewed
and held. Requirements stay902/1158(77.9%);historical coverage1510/2367(63.8%).
118 GUI requirement obligations,40 formal conflicts,scientific/native OS/GPU/
hardware/platform/stress/current NASA/core48/full newest-build gaps remain open.
See ``Qt6ParityValidation/nasa-command-tree-positions-20261029.md``.

The preceding increment qualifies four historical Save/Global SelectAll/SelectNone
rows. Two new GUI cases exercise every eligible checkbox through mouse events:
24 Save objects and seven Global objects. All/none Cancel,parent Apply isolation,
empty OK rejection,one-object recovery,exact Undo/Redo/Unicode save/reopen,native
selected identities/order and mission results qualify in both modes. Twelve
qualified missions check72 raw values,70 selected identities,84 native global
flags and six Save aggregate exports. All42 explicit visible boundaries expose
and pace in the requested resizable,initially minimized scale3 desktop.
The original successful unattended contexts are retained. Save-only filename/
declaration oracle corrections and a visible-only callback ordering fix preserve
failed/interrupted receipts;no existing assertion is removed. No production/core
changes,unaffected replay,screenshots or data copies. All169 old assertions,48
core hashes and158 collections remain unchanged.159 collections,1185 checks,170
assertion pairs. Lossless derived-report compression recovers135,867,615 bytes.
Six rows reviewed;ValidateEveryResource and original AddAll/DeleteAll remain
held. Source review1665/2367(70.3%),702 historical rows unreviewed. Requirements
stay902/1158(77.9%);historical coverage becomes1438/2367(60.8%).118 GUI requirement
obligations,40 formal conflicts,core48 scrutiny and scientific/native OS/GPU/
hardware/platform/stress/current NASA/full newest-build gaps remain open.
See ``Qt6ParityValidation/nasa-object-selection-20261029.md``.

The preceding increment qualifies three built-in SolarSystemBarycenter requirements,
FRR!E258/E259/E260. Two new source cases pass unattended and visibly: actual
Origin picker Cancel/accept and parent Apply, immutable membership in the GUI
and both native setters, source switching, exact Undo/Redo, Unicode save/reopen,
and actual report viewing. Twelve missions check504 raw values and216 native
SSB components against isolated direct-CSPICE references at three epochs.
DE405 uses the native DE reader; SPK uses DE421 and the documented shared-table
ICRF-to-MJ2000Eq convention, independently interpolated and rotated. The initial
wrong-frame/path reference failures remain failed and uncredited; tolerances
stay0.002km/2e-8km/s. The maximum position discrepancy is0.000026159km.
All55 explicit visible boundaries expose and pace in a resizable, initially
minimized desktop at scale3. No production/core changes,unaffected replay,
screenshots or data copies. All168 old assertions,48 core hashes and157 old
collections remain preserved;158 collections,1183 checks,169 assertion pairs.
Lossless derived-report compression recovers81,260,909 bytes. Historical
coverage stays60.6%;source review stays70.1%,708 historical rows unreviewed.
The refreshed requirement gap index retains256 uncredited rows,including118
GUI obligations.40 formal conflicts,scientific/native OS/GPU/hardware/platform/
stress/current NASA/core48 scrutiny and full newest-build integration remain
open. See ``Qt6ParityValidation/nasa-builtin-ssb-20261029.md``.

The preceding increment strengthens the existing Delete obligation,All Others!E159,
without adding a unique requirement or historical row. Six new native cases cover
the four documented built-in frames and custom resource/command-reference refusal.
A preserved pre-fix EarthFixed menu exposes enabled Delete. Qt MainWindow now
omits Delete for built-in coordinates and explicitly refuses direct frontend
removal. All six cases and the affected coordinate lifecycle positive control pass
visibly/unattended. All29 original Delete claims are audited in each mode;the
current coordinate claim is preferred and28 others retain their old versions.
The resulting35-claim conjunction requires every component.24 new missions check
172 raw and168 native values,16 protected-menu states,eight direct refusals,eight
reference refusals and16 decision/error dialogs.50/50 explicit new visible
boundaries qualify. All167 prior assertions,48 core hashes and156 collections
remain unchanged.157 collections,1181 checks,168 current assertion source/hash
pairs. No core changes,unaffected replay,screenshots or dataset copies. Lossless
derived-report compression recovers81,117,387 bytes. Percentages stay77.6%/60.6%;
source review70.1%,708 historical rows unreviewed.40 formal conflicts,built-in SSB
DE405/SPK science and full newest-build/native OS/GPU/hardware/platform/stress/
current NASA/core48 scrutiny remain open. See
``Qt6ParityValidation/nasa-frame-delete-20261028.md``.

The preceding increment qualifies36 body-event/default-Delete historical scopes and
reviews104 rows,with68 explicit holds. Fourteen new cases exercise actual viewport
mouse/context events,named typed editors,current Mu63,pending Mu77,same-owner
reentry,source/history/native/file isolation and independent continuation42.
Current Saturn is tested separately without resolving historical Sarturn.
Qt MainWindow now hides Delete for default celestial bodies as the guide requires
and guards nullable action dismissal.44 clean/pending protected-menu states and
318/318 explicit visible boundaries qualify.28 new dual-mode missions check28 raw
and28 native results;184 editor witnesses,84 double-clicks and100 context events.
One affected Earth Clone menu regression passes in both modes;unchanged user scout
is retained and no unaffected passing scenario replays. All166 prior assertions,
48 core hashes and155 old collections remain unchanged.156 collections,1175 checks
and167 current assertion source/hash pairs. Source review1659/2367(70.1%),708
unreviewed. Forty formal conflicts include one newly explicit user-body Rename/
Delete gap;68 held rows receive no credit. No core changes,zero screenshots and
shared immutable datasets. Lossless derived-report compression recovers189,056,900
bytes. Built-in-frame Delete still needs direct qualification;full newest-build/
scientific/native OS/GPU/hardware/platform/stress/private NASA/core48 scrutiny
remain open. See ``Qt6ParityValidation/nasa-body-open-20261028.md``.

The preceding increment qualifies one nonmonotonic interpolation scope,FRR!E407,
through a separate native scientific companion and three actual GUI direction/
recovery cases in both modes. All five shipped native interpolators check
increasing/decreasing polynomial values,atomic reversal refusal,cloning,Clear
recovery,and direction across wrapped/repeated inputs. Code500 stops before the
terminal assignment/report and emits only its incomplete header;GUI format repair
to CCSDS-OEM recovers. Two OEM cases check both direction orders and two arcs.
Independent audits check132 OEM samples/792 state values,112 report values and114
native polynomial values.35/35 explicit new visible boundaries are exposed/paced.
Three affected scientific regressions pass in both modes;no unaffected replay.
Six new core files repair input validation,backward Linear bracketing and populated
copy ownership/buffer defects;the original42 core files and164 prior assertion
hashes remain unchanged.48 core files now await scrutiny;technical approval is
false.155 audited collections,1161 registered checks and166 current assertion
source/hash pairs. Native companion and two initial OEM passes are reused without
replay. Zero screenshots,shared immutable datasets;lossless compression of this
increment's repeated derived inventories recovers188,673,911 bytes. Derivative
Hermite science/assignment,duplicate/nonfinite policies,Code500 internal Toggle
holes,812 unreviewed historical rows and full newest-build/platform/scientific
qualification remain open. See ``Qt6ParityValidation/nasa-interpolator-order-20261028.md``.

The preceding closed-load increment qualifies14 ordinary Load scopes through14 new native cases
in both modes. Actual File/Open requires all configuration panels closed,checks
picker Cancel,distinct resource namespace/current native field and actual reopened
editor. Malformed files must leave an editable script,empty resource/mission base
and disabled creation;Run cannot execute stale data or overwrite the prior report.
Typed correction and actual Save-and-build restore the normal GUI and valid
mission.84 successful missions check168 report values/84 configured-field
witnesses;28 invalid Runs refused,56 safe-state snapshots and28 save/build
recoveries.252/252 explicit visible boundaries are paced in three disjoint,
initially minimized/resizable scale3 groups. Two affected existing file/valid
creation regressions also pass in both modes;no unrelated passing test replayed.
Qt MainWindow::refreshTrees now hides partial objects and gates creation after a
failed parse. No core file changes,observer screenshots or dataset duplication.
The60 held rows comprise52 unavailable ProgramExitNo/Yes recipes and eight
unresolved body-alias loads.153 prior collections,163 assertion sources and42
core hashes preserved;existing MainWindow source bindings explicitly re-reviewed
and rebound while archived bindings remain unchanged.154 collections,1157
registered checks,164 current assertion source/hash pairs;historical source
review1555/2367(65.7%),812 unreviewed. Full newest-build/scientific/native
OS/GPU/hardware/platform/stress/current private NASA and other explicit gaps
remain open. See ``Qt6ParityValidation/nasa-closed-load-20261028.md``.

The preceding clone-policy increment qualified25 clone scopes:24 through13 new native cases in
both modes,and one retained GroundStation clone. Default-body/propagator Clone
refusal before/after field Apply,and modified attitude/visual copies with
independent original/copy edits,exact source/history and Unicode persistence pass.
34 new missions check68 report values,46 native fields and144 Cartesian values;
124/124 explicit visible boundaries are paced in minimized/resizable scale3 groups.
A delayed visual preview cloned stale coordinate references after Apply rebuilt
the mission. The Qt-only ResourceForm fix uses a fresh isolated visual validator;
42 core files are unchanged. Only affected spacecraft/combined-transform contexts
were requalified; six independently decoded geometry images were then deleted,
with measurements/hashes retained. No unaffected passing scenario replayed or
observer screenshot retained. The39 held source rows comprise25 unknown Name
InputText recipes,six user-body clone conflicts and eight unresolved body-alias
clone rows.152 prior collections,162 assertion sources and42 core hashes remain
unchanged. The210 existing rows with ResourceForm references are re-reviewed and
rebound to the narrow GUI patch; archived collection bindings remain intact.
153 collections,1143 registered checks,163 current assertion source/hash pairs;
historical source review1481/2367(62.6%),886 unreviewed. Full newest-build,
broader science/native OS/GPU/hardware/platform/stress/current private NASA and
other explicit gaps remain open. See ``Qt6ParityValidation/nasa-clone-policy-20261028.md``.

The preceding command-reference increment qualified19 finite command-reference deletion scopes through
19 new native cases in both modes. Actual Resource Tree Delete refuses in-use
objects before/after Rename; Cancel/accept,pending/stale editor guards,exact
source/literal/prefix/history,Unicode reopen and Mission Tree Delete Cancel/Yes
pass with independent renamed/Undo-recovered/deleted native results.114 missions
include106 successes and eight specified missing-solver-input errors;1272 report
values,1272 native values and24 exported Save state values are checked.570/570
explicit visible boundaries are paced in four disjoint initially minimized,
resizable scale3 groups. Fifteen unattended passes are retained;only four failed
queued-diagnostic probes rerun after a driver-only timing correction. Nineteen
ambiguous historical RenameRef recipes remain reviewed but unverified. No old
passing scenarios replayed,observer screenshots or production/core changes.
151 prior collections,161 assertion sources and42 core hashes preserved.
152 collections,1130 registered checks,162 assertion source/hash pairs;historical
source review1417/2367(59.9%),950 unreviewed. Full newest-build integration and
private NASA/scientific/native input/hardware/platform/stress qualification remain
open. See ``Qt6ParityValidation/nasa-command-reference-20261028.md``.

The preceding panel-lifecycle increment qualified13 resource-panel lifecycle scopes:11 through
five new cases in both modes,and two Cancel scopes through audited saved passes.
Actual title-bar X clean/pending decisions,distinct two-Apply/positive OK,
original-owner closure after callback focus change,exact source/history/Unicode
files and actual panel-open File/Open pass.14 new missions/28 report values/14
native field witnesses;four retained Cancel missions supply four report values.
All104/104 new visible boundaries are paced in two initially minimized/resizable
scale3 batches. No old-case replay,production/core changes or screenshots.
160 prior assertion sources/42 core hashes/150 collections preserved.
151 collections,1111 checks,161 assertion pairs;source review1379/2367(58.3%),
988 unreviewed. Retained unattended source differs only in unexecuted empty-input
handling/diagnostics for these nonempty fixtures; actual archived hashes are
preserved. No original ExitNo/Yes/private-keyword or field-physics/full-build
qualification is inferred. See
``Qt6ParityValidation/nasa-panel-lifecycle-20261028.md``.

The preceding increment closes the recorded no-op-leaf runtime-order gap for eight
existing leaf-insertion scopes,adding zero unique row credit. Four new cases
pass both modes with Keep+=10,actual pre-leaf/body observations and paired
before/after values differing exactly10 while final states commute.16 missions,
16 insertions/16 cancelled drafts,60 body+32 leaf observations/756 native values
pass independent integer simulation. All194/194 visible boundaries are paced
in two initially minimized/resizable scale3 batches. No old-case replay,
production/core changes or screenshots;159 prior assertion sources/42 core
hashes/149 collections preserved.150 collections,1106 checks,160 assertion
pairs;source review remains1366/2367(57.7%),1001 unreviewed. Generic unused
Apply/clean-Apply capability labels are excluded from the curated extension;
all broader input/parent/scientific/provenance/full-build gates stay open. See
``Qt6ParityValidation/nasa-control-leaf-order-20261028.md``.

The preceding increment qualifies36 control-command insertion scopes with12 new
cases in both modes. Four distinct identities(For,plain If,If-with-Else,While)
exercise nine positions each in an outer-For/inner-If fixture. Actual context
menus/templates/drafts,positive insertion OK,Apply becoming replacement,
Cancel/clean Apply,exact source/Undo/Redo/Unicode files and native/visible tree
parents pass.72 missions/88 insertions/72 cancelled drafts/216 native body
observations/2016 scalar values match independent integer simulation. All
856/856 authored visible boundaries are paced in seven bounded,
initially minimized/resizable scale3 batches. No old scenario replay,production
or core changes or screenshots.158 old assertion sources/42 core hashes/148
collections preserved.149 collections,1102 checks,159 assertion pairs; source
review1366/2367(57.7%),1001 unreviewed. Arbitrary parent/filter/solver/input
contexts and original private keyword recipes remain unqualified alongside
all previous explicit scientific/platform/provenance/full-build gates. See
``Qt6ParityValidation/nasa-control-tree-insertion-20261028.md``.

The preceding increment qualifies all22 typed editor-commit gaps held below.
18 new cases pass first execution in both modes:36 positive OKs,eight Applies,
44 editor retirements/preserved sibling drafts,44 missions/80 rows/530 values.
Actual operand/condition/stopping browsers, exact source/history/Unicode files,
reopened headers and original-owner closure despite callback focus changes pass.
Independent truth tables/loop counts, polynomial optima/active bounds and complex
circular propagation oracles agree. All206/206 visible boundaries are
paced in seven bounded resizable/initially minimized scale3 batches. No old
scenario replay,production/core changes or screenshots;157 assertion sources,
42 core hashes and147 collections preserved.148 collections,1090 checks,158
assertion pairs. These are finite examples; source review remains1330/2367,
1037 unreviewed, and all explicit broader scientific/provenance/native-input
and complete latest-build gates stay open. See
``Qt6ParityValidation/nasa-typed-editor-commits-20261028.md``.

The preceding increment reviews27 typed command Apply/OK rows, qualifying five
positive Apply scopes and holding22. Seven passing contexts are retained, and
only three first-unattended contexts execute, all passing; no new visible replay,
registration, source/assertion/production changes or screenshots. If-with-Else
Array/Property/Variable and Minimize Property/Variable examples check144 raw
values in12 reports. All98 selected retained authored boundaries are paced.
Actual archived source versions are kept: reviewed source deltas affect an
unexecuted Boolean truth-table branch or opening/rejection/pacing/scope text;
positive Apply/result blocks and selected inputs remain identical. No stronger
current behavior or full latest-build pass is inferred.157 assertion pairs,
42 core hashes and146 prior collections unchanged;147 collections,1072 checks.
Historical source review1330/2367(56.2%),1037 unreviewed. Positive editor OK is
not inferred from picker OK or invalid editor rejection. See
``Qt6ParityValidation/nasa-typed-command-operations-20261028.md``.

The preceding increment qualifies52 bounded body Help/Maximize/Minimize/Tab scopes
through13 new cases passing first execution in both modes. Actual MDI menu
operations check viewport maximization, icon minimization and exact restoration;
Help button/F1/local readonly content/Escape and all four page focus cycles pass.
52 Help routes,104 page cycles,208 directional traversals,104 window operations,
1976 forward/1976 reverse focus visits and26 raw/native continuation values
are checked. All234/234 authored visible boundaries are paced in five
bounded initially minimized/resizable scale3 batches; no fullscreen/screenshots.
No old GUI cases replayed or production changes.156 old assertion sources,
42 core hashes and145 collections remain unchanged.146 collections,1072 checks,
157 assertion pairs. Historical review1303/2367(55.0%),1064 unreviewed. Full
latest-build/NASA-wide qualification and all explicit science/provenance/native
input gaps remain open. See ``Qt6ParityValidation/nasa-body-mechanisms-20261028.md``.

The preceding increment qualifies39 bounded historical body Close/DialogExit/
panel-open Load scopes through13 new cases in both modes. Actual panel Close,
MDI system-menu and resource context Close, pending Cancel/Discard preservation
or retirement, unrelated pending-sibling isolation and actual Unicode File/Open
with stale-owner retirement pass.26 native/report continuation witnesses and104
prompts are checked; all286/286 authored visible boundaries are paced.
Five bounded batches execute exactly13 new visible cases, retaining earlier
passes.52 existing Apply/Error/Cancel/OK scopes are excluded from replay.
No production/core changes,screenshots or old GUI replays.155 prior assertion
sources,42 core hashes and144 old collections remain unchanged.145 collections,
1059 checks,156 assertion pairs. Historical review1251/2367(52.9%),1116 unreviewed.
Ambiguous aliases/keywords/ExitNo/Yes and complete latest-build qualification
remain open. See ``Qt6ParityValidation/nasa-body-close-20261028.md``.

The preceding increment qualifies two bounded function-definition scopes,
FRR!E483/E484, through three new cases in both modes. Actual function-file
Cancel/Save, warning attribution, ignored invalid-tail and tail-only changes,
first-body reload, single-definition recovery and readonly ReportViewer checks
pass across30 missions/450 raw values. All72/72 authored visible boundaries are
paced in a resizable initially minimized scale3 desktop; no screenshots.
Two core causes are repaired: later function declarations contaminated first
output metadata, and ignore-rest state truncated the next main-script rebuild.
Nine direct output-scope regressions improve from3/9 to9/9; ten affected existing
function-I/O cases pass unattended.154 old assertion sources and143 prior
collections remain unchanged; no unrelated known-pass GUI replay. The core
inventory is42 files: new FileUtil plus an additional ScriptInterpreter delta,
with the other40 hashes preserved, pending technical scrutiny.144 collections,
1046 registered acceptance checks plus the separate core CTest,155 assertion
pairs. Arbitrary ignored-tail/unbalanced-control syntax, other multi-definition
I/O types, remaining function rules and complete latest-build qualification
remain open. See ``Qt6ParityValidation/nasa-function-definitions-20261028.md``.

The preceding increment repairs the two identified readonly harness editor-lifetime
bugs and requalifies their seven affected cases in both modes. QPointer tracks
retired editors, rebinds the actual current editor after Apply and verifies
rejected Apply retains the correction owner. All92 original assertions and all
scientific fixture/oracle/comparison code are preserved.28 retired-editor and56
rejected-owner checks plus1244 raw values pass, with231/231 boundaries paced.
No unique scope credit is added.152 other assertion sources,41 core hashes and
142 prior collections remain unchanged; no production/registration/contract
changes,screenshots or unrelated GUI replays.143 collections,1043 checks,154
current assertion pairs. The identified two-harness lifetime follow-up is
resolved; other harness lifetime/full newest-build qualification is not inferred.
See ``Qt6ParityValidation/nasa-readonly-owner-lifetime-20261028.md``.

The preceding increment qualifies two bounded readonly full/partition STM scopes,
FRR!E539/E540, in both directions and modes. Five actual report selectors/viewers,
readonly guards and STM/persistence routes check988 raw values across100 nominal/
perturbed trajectories. The unchanged analytic oracle, independent RK4 and DOP853
variational/flow integrations plus two-scale nonlinear Richardson derivatives
agree. All140/140 authored boundaries are paced. Three failed preliminary scouts
remain uncredited after fixing new-harness stale-editor and function-basename
fixture causes; all assertions stay intact. No production changes,screenshots or
old GUI replays;153 old assertion files,41 core hashes and141 collections remain
unchanged.142 collections,1043 checks,154 assertion pairs. Older BetaAngle/
geodetic harness successful-Apply lifetime review is explicitly open and next;
archived independent numerical evidence stays preserved. General scientific/
provenance/native-input and complete latest-build gates remain open. See
``Qt6ParityValidation/nasa-stm-parameter-20261028.md``.

The preceding increment qualifies one bounded read-only BetaAngle requirement,
FRR!E504, in Earth/Mars on first visible/unattended execution. Three epochs and
seven signed/boundary angles check924 raw values,168 beta values and16 picker/
16 typed write rejections. Direct CSPICE Sun/time inputs and independently
prescribed planes are audited using Python atan2/cross products, not native
BetaAngle. All66/66 authored boundaries are paced. Shared DE421/LSK inputs
do not establish independent ephemeris accuracy. No production changes,failed
cases,screenshots or old GUI replays;152 assertion files,41 core hashes and140
prior collections remain unchanged.141 collections,1041 checks,153 assertion
pairs. Parent any-body/science/native-input/current-private-NASA and complete
latest-build gates remain open. See
``Qt6ParityValidation/nasa-beta-angle-20261028.md``.

The preceding increment qualifies six new command-panel Load contexts on first
execution in both modes:For,If,While,VaryTarget,Propagate andReport. Actual
clean/pending File/Open Cancel/Discard and different-namespace loading check288
raw values across24 missions, with168/168 authored boundaries paced. All19
non-plot command Load with Panel Open rows now have bounded evidence. No
production changes,failed cases,screenshots or old-case replays;151 preceding
assertion files,41 core hashes and139 prior collections remain unchanged.
140 collections,1039 checks,152 assertion pairs. Historical review1212/2367
(51.2%),1155 rows unreviewed. Original keyword/ChangeAll/ExitNo/Yes,resource-panel,
scientific breadth and full latest-build qualification remain open. See
``Qt6ParityValidation/nasa-command-panel-load-six-20261028.md``.

The preceding increment traces13 separately inventoried Load with Panel Open rows
using existing clean/pending editor loading proofs in both modes. No GUI tests,
executions,screenshots,production changes or new raw captures are added. The
same52 missions/624 values and364/364 paced boundaries are reaudited; this adds
row traceability, not new exercised mechanisms.151 assertion bindings,41 core
hashes and138 prior collection records remain unchanged.139 collections,1033
checks. Historical review1206/2367(51.0%),1161 rows unreviewed. The other six
non-plot command panel contexts and original keyword/provenance/science/full
latest-build gates remain open. See
``Qt6ParityValidation/nasa-command-load-panel-reuse-20261028.md``.

The preceding increment qualifies13 additional historical Load contexts in both modes.
Actual File/Open clean/pending Cancel, direct pending-load refusal and Discard
load a different resource namespace and41-to73 sentinel. Stale owners/native/plot
identities retire; reopened references and624 raw values across52 missions pass.
All364/364 authored visible boundaries are paced. Two failed new scouts
remain uncredited after fixing a property-token rewrite in the new fixture
generator; assertions stay intact. No production changes,screenshots or old-case
replays;150 preceding assertion files,41 core hashes and137 prior collections
remain unchanged.138 collections,1033 checks,151 assertion pairs. Historical
review1193/2367(50.4%),1174 rows unreviewed. Original keyword/ChangeAll/ExitNo/Yes,
scientific breadth and full latest-build qualification remain open. See
``Qt6ParityValidation/nasa-mission-command-load-20261028.md``.

The preceding increment qualifies17 additional historical CommandSummary contexts on
first execution in both modes. Actual pre-run/pending/clean summary routes compare
544 cached fields against748 raw values and independent circle/impulse/literals;
late X/Cd/dry-mass mutations reject latest-state substitution.136/136 authored
visible boundaries are paced. All23 exact CommandSummary metadata rows now have
bounded evidence, with A1/full-field/frame/domain/science gaps retained. No
production changes,screenshots,failed contexts or old-case replays;149 preceding
assertion files and41 core hashes are unchanged.137 collections,1020 checks and
150 assertion pairs; historical review1180/2367(49.9%),1187 rows unreviewed. See
``Qt6ParityValidation/nasa-command-summary-extended-20261028.md``.

The preceding increment reuses exact-source visible Report/Propagate summary evidence
and adds the unchanged existing case's first unattended pass. Two historical
summary-operation scopes are added;13 already-covered field scopes gain mode
evidence, with no unique requirement increase.352 display comparisons,184 raw
values and12 independently reconstructed initial-position components are reaudited.
A1 GUI display requirementsE211/E212 remain conflicted and uncredited. No new tests,
visible replay,screenshots,application/core changes;149 assertion files,41 core
hashes and135 prior collections remain unchanged.136 collections,1003 checks,
149 assertion pairs; historical review1163/2367(49.1%),1204 rows unreviewed. See
``Qt6ParityValidation/nasa-command-summary-operation-reuse-20261028.md``.

The preceding increment qualifies95 historical naming-operation scopes across19
command contexts in both modes. Actual Rename/editor/cancel/error/duplicate/
empty/Unicode/pending guards preserve exact source/history/native identities;
independent circle,impulse,control/solver and command witnesses check456 values.
All893/893 authored visible boundaries are paced.14 unattended passes
and fully recorded visible passes are reused; one interrupted context with a
missing driver exit required requalification, with the partial recovery uncredited.
No application/core changes,screenshots or unaffected qualified-case replays;
147 prior assertion files and41 core hashes remain unchanged.135 collections,
1003 checks and149 assertion pairs. Historical source review1161/2367(49.0%)
leaves1206 unreviewed rows. See
``Qt6ParityValidation/nasa-mission-command-names-20261028.md``.

The preceding acceptance increment qualifies five read-only geodetic/hour-angle
quantities:Altitude,Latitude,Longitude,LST andMHA. All ten first-execution visible/
unattended contexts pass. Independent forward ellipsoid/static rotation and raw
inertial/body-fixed witnesses check320 values,40 picker rejections,40 typed Apply
rejections,30 cancellations and165/165 paced boundaries. No application/core
changes,screenshots or earlier GUI-case replays;146 preceding assertion files
and41 core hashes remain unchanged.134 collections,984 checks and147 assertion
source/hash pairs. See ``Qt6ParityValidation/nasa-readonly-geodetic-20261028.md``.

The preceding acceptance increment qualifies eight GroundTrack default/custom-body,
spacecraft/station icon and adjacent-label scopes in Dense/Sparse visible and
unattended contexts. Independent circular/static-frame results check112 values;
16 icon and16 label checks pass at two viewer sizes, with66/66 paced boundaries.
Existing skip-count conflictFRR!E634 and parentE633 remain uncredited despite
observed interval sampling. No application/core changes, no retained passing
images or unaffected-case replays;145 preceding assertion files and41 core hashes
are unchanged.133 collections,979 checks and146 assertion source/hash pairs. See
``Qt6ParityValidation/nasa-groundtrack-body-symbols-20261028.md``.

The preceding acceptance increment qualifies three V/N/B burn-component read/write
scopes. Actual writable property selection originally produced a wrong-frame
reference; the GUI now selects native burn components while readable frame
behavior is preserved. Both modes check six physical maneuvers,204 raw values,
exact alias/canonical readback, invalid Apply/history/Unicode recovery and
22/22 paced boundaries. One affected selector regression passes. No core
changes or screenshots;144 preceding assertion sources and41 core hashes are
unchanged.132 collections,977 checks and145 assertion source/hash pairs. See
``Qt6ParityValidation/nasa-burn-vnb-aliases-20261028.md``.

The preceding source-only review completes applicability/proof-obligation review
for all1,158 available historical requirement rows. Its93 new reviews add zero
acceptance credit:82 GUI obligations,six external Mars-GRAM dependencies and
five console obligations remain unverified. Parent/all/any clauses do not
inherit selected child-case credit; DEFERRED/Suggestion to Remove and nine
source questions remain explicit. No GUI reruns,production changes,snapshots
or screenshots. All144 assertion sources,41 core hashes,131 collection records
and the acceptance ledger are preserved. See
``Qt6ParityValidation/nasa-remaining-requirement-source-review-20261028.md``.

The latest scope increment qualifies20 plot-command Open/Name/Rename/Cancel/
Error/UsedName operations. The missing NASA Mission-tree Rename dialog is
implemented in the Qt GUI using existing label/mission transactions; duplicate
command names remain allowed. Eight contexts check56 Rename trials, Unicode/
punctuation/empty/duplicate labels, quote errors, no-op/history/pending guards,
and64 raw native-result values. Zero screenshots;143 preceding assertion files
and41 core hashes are unchanged. MainWindow.cpp and its Qt header are the only
production changes. The ledger retains131 collections,976 registered checks
and144 NASA-bound assertion source/hash pairs.22 reviewed plot-command scopes
remain held, including six ambiguous reference-name recipes. See
``Qt6ParityValidation/nasa-plot-command-names-20261028.md``.

The preceding scope increment qualifies six plot-command Load operations through
four new cases in both modes. Actual File/Open checks clean/pending editor
loads, Cancel protection, Discard and stale-owner/reference retirement;16
missions retain128 raw report values and independent circle/XY/action
witnesses.32 actual main-window close scenarios check current Save/Discard/
Cancel, while eight ambiguous historical ExitNo/ExitYes rows remain held.
All136/136 viewing boundaries are paced; zero screenshots or production
changes;142 preceding assertion files and41 core hashes are unchanged.
The ledger retains130 collections,972 registered checks and143 NASA-bound
assertion source/hash pairs.42 reviewed plot-command scopes remain held. See
``Qt6ParityValidation/nasa-plot-command-lifecycle-20261028.md``.

The preceding scope increment qualifies four ClearPlot/PenUp/PenDown/MarkPoint
command-summary operations. One new case passes visibly and unattended,
checking pre-run unavailability, applied identity despite pending name/plot
edits,128 cached numeric comparisons against80 raw report values, reverse-order
old-cache revisits and29/29 paced boundaries. No production changes or
screenshots;141 preceding assertion files and41 core hashes are unchanged.
The ledger retains129 collections,968 registered checks and142 NASA-bound
assertion source/hash pairs.48 reviewed plot-command scopes remain held. See
``Qt6ParityValidation/nasa-plot-command-summaries-20261028.md``.

The preceding supplementary qualification adds no NASA row credit. Eight new
PenUp/PenDown raw-script contexts prove runtime rejection of ReportFile targets,
no later assignment/report, exact source correction/history and successful
XYPlot recovery, in normal/debugger modes visibly and unattended. A GUI-only
failure diagnostic now reports the last command type reached. All141 existing
NASA-bound assertion programs and41 core hashes are unchanged. Zero new
screenshots;967 registered checks;128 NASA scope collections remain. Retained
unattended evidence precedes a diagnostic-string-only refinement exercised by
the visible build; no full latest-build qualification is claimed. See
``Qt6ParityValidation/nasa-pen-script-policy-20261028.md``.

The preceding NASA scope increment qualifies16 ClearPlot/MarkPoint/PenUp/PenDown Help,
MaxRestore,MinRestore and Tab scopes through four new fixture contexts using
the existing driver assertions. PenUp/PenDown Help now defaults to the installed
combined guide; GUI typed references now match documented compatible plot
classes, rejecting the reproduced ReportFile acceptance without changing core.
Eight contexts check16 Help routes,32 window actions,192 focus entries,40
typed rejections,16 scalar report values and132/132 paced boundaries. Zero
screenshots;127 preceding collection records, all141 assertion programs and41
core hashes are preserved. The driver changes only a descriptive caption;
the old16 fixture cases are exact.963 checks,128 collections and141 current
assertion pairs remain.52 reviewed plot-command rows stay held. A separate
native/script pen-reference permissiveness finding was recorded for scrutiny,
with runtime behavior unverified at that checkpoint. The supplementary
pen-script policy audit above resolves the tested runtime rejection. See
``Qt6ParityValidation/nasa-plot-command-help-window-tab-20261028.md``.

The preceding no-XY increment qualified four Add/AddOpen scopes, fixed a picker
offering nonexistent checked PlotName, retained the pending-panel creation
guard, and checked correction/GUI creation/reopen/native40s results. Four
contexts checked8 empty/8 valid picker states,8 missing-reference and4 creation
rejections,8 accepted XY creations,64 components,84 XY points and138/138 paced
boundaries. That checkpoint was981 historical scopes (41.4%),959 checks,
127 collections and141 assertion pairs. See
``Qt6ParityValidation/nasa-plot-no-resource-recovery-20261028.md``.

The preceding four plot-reference deletion scopes checked16 resource
rejections,32 native identities,128 components,144 XY points and104/104 paced
boundaries. That checkpoint was977 historical scopes (41.3%),957 checks,
126 collections and140 assertion pairs. See
``Qt6ParityValidation/nasa-plot-reference-deletion-20261028.md``.

The preceding36 plot-tree positions checked120 native executions,1,728
components and1,444/1,444 paced boundaries. That checkpoint was973 historical
scopes (41.1%),953 checks,125 collections and139 assertion pairs. See
``Qt6ParityValidation/nasa-plot-tree-positions-20261028.md``.

Source review is1,158/1,158 requirements (100%,0 unreviewed) and1,066/2,367
historical rows (45.0%,1,301 unreviewed).2012 Passed metadata is not current
proof. Legacy MinRestore operation labels, No/Yes close semantics and original
field/loop/name keyword payloads remain ambiguous or absent. PenUp/PenDown
standalone help pages are under construction; the substantive combined-guide
route has bounded Help-button/F1 qualification.39 formal conflicts and all science,
core/native/hardware/platform/stress/provenance/full-build gates remain open.

The preceding finite-option increment added eight ephemeris/origin scopes by
reusing50 contexts (27 visible,23 unattended), requiring every listed child.
It reaudited212 raw OEM records /1,272 state components. Three older OEM
sources differ only in ancestor-aware tab selection; all numerical/history/
report/control assertions and fixtures are unchanged, without qualifying the
newer navigation. Four selected OEM cases lack first unattended proof. That
checkpoint was877/1,158 scopes (75.7%),935/2,367 historical scopes (39.5%),917
checks,123 collections and138 assertion pairs. Its31 new held reviews cover
9 ephemeris,12 historical OS/resource and10 MATLAB-dependent rows. See
``Qt6ParityValidation/nasa-finite-options-reuse-and-gaps-20261028.md``.

The preceding contact increment added bounded scopesFRR!E599/603 through
three new first-pass dual-mode cases with independent rise/set geometry,
selected/no-contact/excluded observer membership and clipped intervals. It
checked10 intervals,20 endpoints,10 durations,4 clipped endpoints,14 sections,
4 no-contact sections,4 exclusions,36 final-state values and54 station
coordinates, with maximum boundary error0.000441214s and170/170 authored
visible boundaries paced. That checkpoint was869/1,158 scopes (75.0%),917
checks,122 collections and138 source/hash pairs, with no application/core
change or screenshots. Contact selector/list/occulting/root requirements and
configured-shape/PCK horizon consistency remained held. See
``Qt6ParityValidation/nasa-contact-independent-results-20261028.md``.

The preceding light-time increment added bounded scopesFRR!E620 and parent618
through three new dual-mode eclipse cases, retaining the audited None child.
It checked14 intervals,28 endpoints,28 duration fields,26 correction-sensitivity
boundaries,14 per-type rows and36 final-state values, with maximum boundary
error0.000404803s and176/176 authored visible boundaries paced. That checkpoint
was867/1,158 requirement scopes (74.9%), with914 checks,121 collections and137
source/hash pairs. See
``Qt6ParityValidation/nasa-eclipse-reception-lighttime-20261028.md``.

The preceding increment added two finite eclipse-option headings and qualified
the first unattended contexts of all nine independent eclipse-result cases.
Eighteen existing eclipse scopes obtained both-mode proof cohorts. One affected
visible Mixed case passed and eight earlier visible science results were reused.
It reaudited28 intervals,56 endpoints,56 durations,30 per-type summary rows and126
final-state values, with maximum boundary error0.000278900s and54/54 new authored
boundaries paced. The clean-Apply probe was strengthened without changing old
numerical/history assertions. That checkpoint was865/1,158 requirement scopes,
with911 registered checks,120 collections and136 source/hash pairs. See
``Qt6ParityValidation/nasa-eclipse-headings-unattended-20261028.md``.

The preceding increment qualified total-mass spherical SRP through actual inline
ballistics and two-tank edits, five short numerical missions in each of visible
and unattended contexts, an independent force equation, actual integrated
position/velocity deflections, circular Off control and equal-total-mass
allocation invariance. It retains180 raw values and checks24 native-force,
24 integrated-velocity,24 integrated-position and24 allocation components.
All46/46 authored viewing boundaries are paced; no screenshots or unaffected
passing-case replays. No application/core change was needed.135 prior assertion
source/hash pairs,41 tracked core hashes and118 prior collection records are
preserved. That checkpoint had911 registered checks,119 audited collections and136
source/hash pairs. Its source review was1,010/1,158 requirements
(87.2%,148 unreviewed) and952/2,367 historical rows (40.2%,1,415 unreviewed).
See ``Qt6ParityValidation/nasa-ballistic-total-mass-srp-20261028.md``.

The Code-500 internal-gap negative was already recorded in October and was
not replayed. The source trace retains its fixed-slot packing cause; a conforming
hole representation remains unresolved. Passing SRP results do not resolve it.

The preceding increment added the finite Toggle subscriber heading and EphemerisFile
scope, strengthening existing Off/On interval proofs. Six existing visible/
unattended outcomes are reused without GUI runs, missions, new screenshots or
runtime copies. At that checkpoint,134 prior assertion-source/hash pairs,41 tracked core hashes
and117 preceding collection records were preserved, with910 registered checks
and118 audited collections. See
``Qt6ParityValidation/nasa-toggle-ephemeris-reuse-20261028.md``.

Exact retained ephemeris completion records prove the original OEM/STK assertion
paths; temporary raw ephemeris payloads are unavailable for fresh reparse. Twelve
retained ReportFile values are independently reaudited. The verifier adds only
two complete exact format records, with insertion-only negative contracts;
all41 support tests pass. No application or core source changes. Code-500
internal Toggle gaps remain unqualified; its continuous-enabled binary case
cannot prove that behavior. Save default filename rowFRC!E135 is now a source
conflict: the historical ObjectName.ObjectType rule differs from the current
ObjectName.ObjectType.data implementation. No compatibility/core change or
waiver is made without a reconciled authoritative NASA rule.

Thirty-three TSR rows for comparison/query/metadata/reporting are reviewed as
unverified test-system capabilities, retained in the original denominator and
granted zero credit. That checkpoint source review was1,007/1,158 requirement rows
(87.0%,151 unreviewed) and952/2,367 historical rows (40.2%,1,415 unreviewed),
distinct from demonstrated coverage.39 formal source conflicts remain.

The preceding MarkPoint increment added mission/render acceptance in visible
and unattended contexts, repaired typed non-XY rejection in CommandForm.cpp,
and reused eight exact assignment outcomes. Its48 direct pixel-oracle images
total2,415,228 bytes; routine observer captures were disabled.86/89 explicit
boundaries were paced, leaving three final unselected-plot resize pacing gaps
alongside18 older gaps. The documented future replay uses300 checkpoints in
the resizable, initially minimized desktop with host scaling. See
``Qt6ParityValidation/nasa-markpoint-inline-data-20261028.md``.

The strict registered VF13ad ReportTruth case remains **failed and unwaived**:
text reports print2.01 (forward) or1.99 (central) while native/CSV optimum is2.
No skip/WILL_FAIL or output rewriting is applied; the unchanged failure was not
replayed. The external interface source is unavailable locally. Fourteen
fmincon rows remain external/unverified for the absent Windows/MATLAB backend.
Native OS input, host GPU/platform/stress/general science/current private NASA
provenance and full current-build visible/unattended qualification stay open.
Existing core changes still need technical scrutiny.

The checkpoint paragraphs below retain historical increment details; the
current ledger and the newest source-linked audit govern present counts.

The preceding native-integrator increment qualified common controls for all nine
current numerical methods and clean dialog exit through nine new native passes,
241 invalid rejections,27 paired bounds and421/421 paced boundaries. See
``Qt6ParityValidation/nasa-native-integrator-controls-20261028.md``.

The preceding enumeration increment qualified25 closed headings only after all97
children and403 claims were accounted for, reusing119 outcomes with zero replay.
The broader FRR!E154 all-method controls exclusion is now qualified by the new
native increment; fifteen other broad/incomplete exclusions remain. See
``Qt6ParityValidation/nasa-resource-enumeration-reuse-20261028.md``.

The preceding finite-burn increment qualified both current thruster families
and five invalid-list boundaries through three new passing cases and41/41 paced
boundaries. A reproduced mixed chemical/electric Apply defect was fixed in five
frontend validation lines. The old GUI single-selection description still
conflicts with existing Qt homogeneous multiple selection. See
``Qt6ParityValidation/nasa-finite-burn-selection-20261028.md``.

The preceding resource ShowScript increment qualified eleven historical rows
using17 first-execution passes,84 inspectors, all nine live numerical methods,
both tank/thruster families and five actual spacecraft contexts. Of320 visible
boundaries,316 were paced; four spacecraft viewing-pause gaps remain. Use600
checkpoints for the next intentional/final replay. See
``Qt6ParityValidation/nasa-resource-script-previews-20261028.md``.

The preceding body increment qualified twelve body ShowScript rows through14
first-execution cases,56 inspectors,168 scalar values and168/168 paced boundaries.
Actual Saturn and custom Moon passes remain uncredited companions for four
ambiguous historical identities. See
``Qt6ParityValidation/nasa-body-script-previews-20261028.md``.

The preceding preview increment qualifies Formation and Earth ShowScript
operations: four new Formation inspector states and retained Earth evidence,
without orbit replay. See
``Qt6ParityValidation/nasa-formation-earth-script-previews-20261028.md``.

The preceding Formation increment adds actual membership and Propagate group
GUI cases: two visible passes, twelve numerical routes,396 report values,
288 Cartesian checks and packed member state/shared epoch witnesses. See
``Qt6ParityValidation/nasa-formation-membership-and-propagation-20261028.md``.

The preceding command-heading increment closes12 listed enumerations using
all54 child rows and80 claims.61 retained outcomes are reused without execution;
it adds traceability rather than new behavior. See
``Qt6ParityValidation/nasa-command-enumeration-reuse-20261028.md``.

The preceding VF13ad increment adds actual bundled creation/configuration and
five named Vary controls: six new visible passes,33 numerical routes,1,755 raw
report values and24/24 paced boundaries. Seven fmincon scopes remain external
and unverified on this Linux build. See
``Qt6ParityValidation/nasa-vf13ad-controls-20261028.md``.

The preceding resource-name increment qualifies bounded allowed-resource
Rename/Delete scopes using27 lifecycle families and selected legacy guards.
A narrow GUI validation fix passes48 atomic rejections after reproducing
acceptance of a Resource Tree folder name. Universal operations and exact Clone
remain unverified. See
``Qt6ParityValidation/nasa-resource-names-and-operations-20261028.md``.

The preceding increment adds current-guide Report An Issue/Provide Feedback controls
and strengthens the existing Help/menu-bar scopes without adding distinct
source-row credit. One new native case passes first execution with six exact
intercepted requests, six cancellations, eight report values and25/25 paced
points.20 previous outcomes are reused without replay. Small immutable snapshot
inputs now share cached bytes: three new contracts and a real-input probe check
98.92% less warm private input allocation; original snapshots remain unchanged.
Core41 hashes remain unchanged and full-suite/client/network/science gates stay
open. See ``Qt6ParityValidation/nasa-help-contacts-and-storage-20261028.md`` for
exact evidence, destination choice, storage measurements and remaining gaps.

The preceding increment adds two direct Help links and qualifies wiki/forum/project
routes plus the historical Help and menu-bar headings. One new native case
passes first execution with nine exact URL dispatches, nine cancellations,
eight retained report values and39/39 paced boundaries. Nineteen old outcomes
are reused without replay; all95 previous collection artifact identities remain
unchanged. Source review is859/1,158 rows (74.2%). One GUI source and one help
source change; core41 hashes remain unchanged. Current-guide Report An Issue/
Provide Feedback, browser/network delivery and complete suite gates remain
open. See ``Qt6ParityValidation/nasa-help-links-20261028.md`` for exact evidence,
primary URL provenance, the bounded heading interpretation and storage costs.

The preceding increment qualifies four bounded toolbar/Edit/Window/About parent
headings by requiring every one of45 already checked child rows.25 preserved
visible outcomes are reused with zero new GUI runs, snapshots or screenshots.
All94 preceding collection identities remain unchanged. Source review is now
854/1,158 rows (73.7%). Whole-menubar/Help headings remain open: direct forum/
project controls and a separately checked Wiki menu route are missing. See
``Qt6ParityValidation/nasa-menu-heading-reuse-20261028.md`` for the exact
constituent/receipt audit, version bindings and unresolved coverage domains.

The preceding increment adds actual MDI tracking-file locations and native exposed
script-preview scopes. Two new visible passes retain150 report values and reuse
four old outcomes without replaying known passes. Four MATLAB-dependent source
rows are reviewed without GUI credit and remain in the denominator. Source
review is now850/1,158 rows (73.4%). All33 explicit viewing points are visible,
but14 later tracking points lack pauses after observer checkpoint exhaustion;
this replay gap needs a larger budget at the final visible gate. No production
GUI/core changes occur; all41 core hashes remain unchanged. See
``Qt6ParityValidation/nasa-tracking-and-script-previews-20261028.md`` for exact
source/JUnit/raw-result evidence, preserved failures, the missing-executable
preflight guard and remaining dependency/science/full-suite gaps.

The preceding increment adds potential-file and all-user-body SPK setting/result
scopes. Four new visible cases retain 576 report values and check 432 independent
nonzero polynomial position/velocity components, with eight invalid rejections
and 32/32 paced boundaries. Only the failed Planet report-precision fixture
retries; numeric predicates remain unchanged. Two old gravity outcomes are
reused. An additional 27 C API source rows are classified non-GUI, remain
unverified and stay in the original denominator. Source review is now
844/1,158 rows (72.9%), separately from verified-scope coverage. No production
GUI/core file changes occur; core 41 hashes remain unchanged. See
``Qt6ParityValidation/nasa-potential-and-user-spk-20261028.md`` for exact evidence,
source-bound applicability guards and remaining API/science/full-suite gaps.

The preceding increment adds five bounded scientific-file/solar-system location
scopes. Four real Set paths selectors export restart-required leap-second, EOP,
nutation and LSK files; session Apply preserves cached scientific data. Four new
visible cases pass first execution, including four fresh GUI processes, 96 raw
report values, 16 reference leap offsets and 16 invalid rejections. All 68 viewing
boundaries are paced. Only two directly affected old path regressions execute
once; one preserved DE/SPK outcome is reused. Four GUI files change, core 41
hashes remain unchanged and the LSK help/script-setter conflict stays open. See
``Qt6ParityValidation/nasa-scientific-file-locations-20261028.md`` for evidence,
exact GUI patch, supported restart semantics and remaining content/science/
provenance/full-suite gaps.

The preceding increment adds all-body texture and physical-property heading scopes
through 15 new visible cases: 11 default bodies and four concrete user types,
60 actual embedded OrbitView surface-color PNG oracles, 600 raw report values
and 45 invalid-file rejections. The expanded Earth pass is retained while only
failed additional-body fixtures retry; one old physical-controls outcome is
reused. No production GUI/core file changes or large ephemeris copies occur.
See ``Qt6ParityValidation/nasa-body-textures-20261027.md`` for exact evidence,
fixture corrections, reusable visible/private-X11 commands and remaining
codec/UV/science/OS/platform/full-suite gaps.

The preceding increment adds five bounded Asteroid/Comet/Moon/Array/GMAT-function
creation scopes by auditing six preserved visible/unattended passes. Actual
creation dialogs, typed objects, Array dimensions/cells and generated function
invocation are checked. Zero GUI executions, snapshots, screenshots or source
changes were needed; all 89 prior collection identities and 41 core file hashes
remain unchanged. See
``Qt6ParityValidation/nasa-resource-creation-reuse-20261027.md`` for exact
evidence and all-body texture/orientation/any-object/full-suite gaps.

The preceding increment adds four bounded DE405-file-location, Earth-nutation,
SPK-file and default-body ephemeris-model scopes. Two new native cases retain
196 report values and154 six-component body states, with six invalid-input
rejections and20/20 paced boundaries. The nutation pass is retained while only
failed DE405 retries after a test-oracle JSON lifetime fix; exact predicates
remain unchanged. Two preserved solar outcomes are reused, no large ephemeris
fixture is copied and no production source changes. See
``Qt6ParityValidation/nasa-ephemeris-locations-20261027.md`` for evidence, the
system-header rebuild context and independent-science/full-suite gaps.

The preceding increment adds nine bounded File-menu/report-log/function-location
scopes. A new File output-directory entry opens the existing guarded dialog on
its Output tab; the original Edit transaction body is preserved. One new native
case checks12 raw numeric values and17/17 paced boundaries. Eight preserved
passes add File/default/Save As/function evidence with zero old GUI replay and
11 retained numeric values. Two GUI files change; engine hashes stay unchanged.
See ``Qt6ParityValidation/nasa-file-output-20261027.md`` for exact patch, fixture
corrections, versioned evidence and remaining data-location/full-suite gates.

The preceding increment implements native selected-region XY zoom and qualifies
FRR!E353 through three new visible cases:39 rendered position probes,17 selected
boundary groups,99 report values and66 raw samples. All41 viewing boundaries
are paced. One directly affected axis regression passes once; no unrelated
known pass or passing new case is replayed. Two GUI files change; engine hashes
stay unchanged. See ``Qt6ParityValidation/nasa-xy-region-zoom-20261027.md`` for
exact GUI patch, preserved fixture failures, pixel evidence and remaining gates.

The preceding increment adds20 bounded XY setup/style/axis requirement scopes
from twelve preserved successful visible/unattended outcomes, with zero GUI
execution and no test/GUI/core source changes. Exact legacy predicates, terminal
PASS/JUnit identities, eight numeric log fields and registered-scale witnesses
are audited. Missing raw legacy pixels, incomplete per-control pacing and
selected-region zoom beyond the new bounded cases remain explicit gaps. See
``Qt6ParityValidation/nasa-xy-control-reuse-20261027.md`` for evidence and limits.

The preceding increment adds13 bounded command-summary field/value scopes through
one new visible case: two spacecraft, four summaries,176 displayed values,
40 fixed fixture checks and independent initial Cartesian-position geometry.
All11 viewing boundaries are paced. Only the failed new fixture retries; no
known pass is repeated and no production GUI/core file changes. The 2012 A1
display requirement remains unverified against current four-time-system output.
See ``Qt6ParityValidation/nasa-command-summary-fields-20261027.md`` for exact
source/version bindings, failure corrections, result values and remaining gates.

The preceding increment adds E57 and strengthens seven existing editor scopes with
actual function-file line-number, Go-to-line and retained Find/Replace controls.
Three initial GUI failures confirm missing controls/lost history. One GUI file
adds local controls/shortcuts and owned search lifecycle; engine hashes stay
unchanged. All three cases pass through preserved/resumed results and a corrected
modal observer. The completed Lines pass is reused; no known pass is repeated.
Twelve numerical values, four navigation boundaries and rendered gutter ink are
checked with 38/38 paced boundaries. See
``Qt6ParityValidation/nasa-function-editor-controls-20261027.md`` for preserved
failures/timeout, exact GUI/observer deltas, reuse lineage and remaining gates.

The preceding increment qualifies blue keyword rendering in the actual script
and function-file editors through two new visible cases. A missing loaded
function-editor vocabulary is repaired. Across two local palettes, 2,250 keyword
glyph crops and 20 lexical context crops are checked, with 80/80 paced boundaries
and unchanged 42/41 results. Grayscale antialiasing is pinned for mask stability.
The then-open E57 heading is addressed by the newer local-control audit above;
full font/compositor/platform/current-build gates remain open. See
``Qt6ParityValidation/nasa-syntax-color-20261027.md`` for precise limits.

The preceding increment adds six bounded script Find/Replace/navigation scopes:
one new integrated visible case plus two preserved Go-to-line passes. It checks
15 literal match selections/offsets, option/wrap/stale-selection/empty/read-only
boundaries, grouped replacement history and Unicode persistence, with independent
6/12 results and sentinel 41. All 31 viewing boundaries are paced. No old GUI
replay or product GUI/core change. See
``Qt6ParityValidation/nasa-editor-search-20261027.md`` for precise limits; the
then-open blue-color requirement is addressed by the newer bounded audit above.

The preceding increment adds seven plot-command requirement scopes and 15 historical
operations through three new visible cases. PenUp/PenDown suppress and resume
connected XY/Orbit/Ground output; ClearPlot empties two selected XY outputs and
permits new data. Independent checks cover 72 report values, nine live boundaries,
84 selected samples, ten rejections and four XY pixel-gap witnesses. All 114
viewing boundaries are paced. Two passes are retained while only failed GL retries.
No product GUI/core change. Native Orbit/Ground pixel-gap, marker/solver/domain and
full-build gates stay open. See
``Qt6ParityValidation/nasa-plot-command-results-20261027.md`` for precise limits.

The preceding increment adds six bounded Assignment/function/mode scopes through
five new visible cases and 17 reused preserved outcomes. Four first-run passes
are retained; only the failed inline-function case retries after diagnosis and
fixture filename correction. Numeric/object-to-String policy and inline alias
discovery remain explicit gaps; new cases are visible-only. The existing 41-file
engine inventory still matches and awaits technical scrutiny. See
``Qt6ParityValidation/nasa-assignment-validation-20261027.md`` for exact limits.

The preceding increment adds ten bounded requirement scopes: five reused command
results and five propagation-goal/first-step/Stop scopes from eight first-run
visible passes. Five historical stopping-goal Apply/subdialog/Unicode Load
scopes are explicitly mapped after execution. The audit independently checks150
numerical values and retained plots, with77/77 paced viewing boundaries and no
old GUI replay or production GUI/core changes. See
``Qt6ParityValidation/nasa-propagation-stop-20261027.md`` for precise limits.

The preceding increment adds selected Save property/attachment scopes and four
historical picker/field-validation scopes through six new cases, with shared
state ambiguity kept diagnostic-only. The audit checks608 property values,
17 standalone object definitions,33 independent numerical values and one string
value. It repairs empty Save Objects validation, missing attached runtime
hardware definitions and stale string exports. Four qualifying passes are
retained; a missing string Value readback is added without removing any old
assertion. All78 explicit boundaries are paced and passing observer frames
are pruned. One GUI file changes and Save.cpp joins the core inventory, now41
files with all prior40 hashes unchanged and technical approval pending. See
``Qt6ParityValidation/nasa-save-attachments-20261027.md`` for precise changes,
source-versioned evidence and remaining gates.

The preceding increment adds seven ScriptEvent grouping/branch-containment scopes
through six new visible cases and twelve malformed-boundary rejection checks.
Two Save export/reload scopes and one changed-in-loop historical scope reuse
preserved visible/unattended Compatibility assertions with zero replay.
All six failed new fixtures retry after reacquiring the editor replaced by Apply;
all assertions/oracles remain intact. No product GUI/core file changes and the
40-file core inventory still matches. Exact Save filename wording and nested-event
save normalization remain gaps, as does fully paced replay beyond the first
24 observer checkpoints per process. See
``Qt6ParityValidation/nasa-script-event-save-20261027.md`` for source-linked
receipts, reuse/provenance limits and remaining gates.

The preceding increment adds two bounded historical scopes for malformed If
validation and cached mutable For ranges, while strengthening existing While
and For facets. Eight new visible cases include four mixed-chain diagnostics;
those diagnostics receive no NASA-conformance credit because the bundled
left-to-right guide conflicts with runtime AND precedence. Five passes are
retained and only three failed cases retry with assertions/oracles unchanged.
The independent audit checks 372 values and 24 predicate rejections. Two engine
fixes guard zero-step arithmetic and reject nonfinite conditional literals;
the core inventory now contains40 files, with all previous38 hashes unchanged
and technical review pending. See
``Qt6ParityValidation/nasa-control-boundaries-20261027.md`` for exact deltas,
release-runtime A/B evidence, diagnostics and remaining gates.

The preceding increment adds ten bounded control-logic requirement scopes and
fourteen historical scopes through 21 new visible cases: exact relational
boundaries, AND/OR truth tables and literal/variable/array/property For ranges.
Twenty passes are retained; only a failed truth-matrix fixture retries, with
all assertions/oracle data unchanged. The versioned audit independently checks
624 values in 75 rows and requires every named case per source row. No product
GUI/core file changes; the core 38-file inventory matches. A For index-type
wording conflict and broader expression/input/platform/stress/provenance gates
remain open. See ``Qt6ParityValidation/nasa-control-logic-20261027.md``.

The preceding increment adds five reused Vary sequence/control-type scopes without
GUI replay and nine visibly checked DC option/property/solver-selection scopes.
Every selected nominal/perturbation/maximum-step/bound result and scaled native
variable is checked; unreachable goals clip and report nonconvergence. One GUI
selector validation gap is repaired. Two directly affected regressions pass;
core 38-file hashes are unchanged. Three bounded historical Vary load/subdialog
scopes are mapped explicitly after execution. Named fmincon/VF13ad settings and
literal-control wording remain uncredited. See
``Qt6ParityValidation/nasa-vary-parameter-20261027.md`` for evidence and gaps.

The preceding increment adds eight bounded Target branch/multiple-control/goal/
solve/exit-mode scopes by reusing four preserved visible/unattended results with
zero GUI replay. One new visible Achieve case adds selected scalar-variable and
spacecraft-property goal scopes, with independent Control 4/2 and actual 1e-9
residual checks. Historical EndTarget-sequence and Achieve literal-goal wording
conflicts remain uncredited. No production GUI/core file changes; the preserved
38-file core inventory matches. See
``Qt6ParityValidation/nasa-target-achieve-20261027.md`` for source-linked evidence,
stronger tolerance facets, retention policy and explicit remaining gaps.

The preceding increment adds four bounded Achieve/Minimize array scopes from
preserved visible/unattended evidence with zero GUI replay, plus one new visible
scalar-property Minimize scope. Changing the objective changes the independent
optimum from 2 to 3; raw results, typed-picker transactions and exact persistence
are checked. No production GUI or engine file changes. The core 38-file
inventory matches its preserved hashes and remains pending technical review.
Passing observer frames are pruned automatically. See
``Qt6ParityValidation/nasa-minimize-property-20261027.md`` and
``Qt6ParityValidation/nasa-solver-operand-reuse-20261027.md`` for source-linked
evidence and bounded claims.

The preceding increments add four function-path, seven reused optimizer and
three optimizer-cardinality/constraint scopes. The existing multiple-constraint
scope also gained selected nonlinear functions with reversed active bounds,
exposing and repairing MinQP Phase I upper-bound slack. That numerical change
is explicitly tracked for scrutiny; broad scientific/stress qualification and
the final complete current-build suite remain open. See
``Qt6ParityValidation/nasa-nonlinear-constraints-20261027.md``.

An earlier increment adds five bounded Mission Tree scopes through nine
visible cases: Close, Run, whole applied-script preview, detail text and context
summary. Repeated pending-owner Open, popup expansion after resize and hierarchy
are supplemental proofs. Eight passes are retained; only two failed focus cases
retry, and one quote-spacing boundary runs separately. Ten numeric reports and
exact source/history/label/preview contracts are audited. Three GUI production
files change; the core 37-file inventory is unchanged. Four unique affected
regressions and three support contracts pass. See
``Qt6ParityValidation/nasa-mission-tree-menu-20261027.md`` for evidence and gaps.

The preceding increment adds six bounded Mission Tree scopes from 60 preserved
lifecycle results across visible/unattended builds with zero GUI replay.
Exact source/snapshot/menu guards, 574 original structured checkpoints and
22 independent literal numeric log records are audited. Its then-missing
selected menu routes are addressed by the newest increment; broader domains
remain open. See ``Qt6ParityValidation/nasa-mission-tree-reuse-20261027.md``.

The preceding increment adds nine bounded Orbit/Ground/XY solver-mode scopes
through eight preserved DC/Yukon/nested/mixed results in visible and unattended
builds, with zero GUI replay. Source assertions cover configuration, history/
report coherence, known goals, endpoints, replay and close/reopen. Transient
iteration visibility and exhaustive trial/Orbit None membership remain gaps.
See ``Qt6ParityValidation/nasa-solver-plot-reuse-20261027.md``.

The preceding increment adds five bounded plot presentation scopes through six
visible cases: UTC Gregorian captions, ground cursor geography after resize/
pan/zoom, and native-renderer mouse zoom/rotation. Two epoch passes are retained;
only failed or visually insufficient cases retry. Independent UTC, pixel/map,
gesture, raw-sample and literal mission oracles pass. Four GUI production files
change; the core 37-file inventory is unchanged. Two affected old regressions
and three support contracts pass. Full current-build qualification remains open.
See ``Qt6ParityValidation/nasa-plot-presentation-20261027.md``.

The preceding increment adds twenty-three bounded OrbitView/GroundTrack scopes
by reusing eight preserved component results across visible and unattended
builds, with zero GUI replay and no production/core change. A shared audit
pipeline verifies 134 assertion references and immutable sources/results.
Two collection skip-versus-period wording conflicts remain uncredited.
See ``Qt6ParityValidation/nasa-plot-component-reuse-20261027.md``.

The preceding increment adds eighteen bounded toolbar scopes: fourteen from ten
new visible route cases and four from one exact-source preserved animation
result, with no animation replay for that credit. Missing blank-script,
clipboard, close and About/wiki routes now appear in a compact second toolbar;
shared animation retains its existing placement. Eight passing cases remain
retained; only Unicode picker and generated-default reference expectations
retry. Literal mission reports and exact document/clipboard/default contracts
are audited. The core37 inventory is unchanged. One directly affected original
animation regression and three support contracts pass; full current-build
qualification remains pending. See
``Qt6ParityValidation/nasa-toolbar-20261027.md`` for evidence and explicit gaps.

The preceding increment adds sixteen bounded Eclipse result scopes through nine
visible cases. Independent geometry checks every selected event time, duration,
type, body and group, plus global/per-type totals and participants. Eight
passing raw outputs are retained and re-audited after correcting the reference
FK5/ICRF input-frame distinction; only Antumbra retries. Assertions and
tolerances remain unchanged. Two plugin files add the historically requested
per-type report section; event search/dynamics formulas are unchanged. The
37-file core inventory remains subject to technical scrutiny. One directly
affected existing regression and three support contracts pass; broader suite
replay remains an integration gate. See
``Qt6ParityValidation/nasa-eclipse-results-20261027.md`` for evidence and gaps.

The preceding increment adds nine OEM sampling/discontinuity scopes through
nine actual resource-window cases, retaining every raw file and checking all
selected Cartesian states, epochs and metadata. Fixed/raw steps, bounded
user interval, assignment/large inertial impulse/propagator boundaries,
GUI-corrected Toggle gaps and exact non-grid propagation-terminal values
have independent circular-orbit oracles. Four passing cases are retained;
only five unresolved cases retry. All672 preceding checks are excluded.
Harness input errors and fixture corrections are documented: per-command
ElapsedSecs and upper-bound user FinalEpoch follow the NASA guide. The
historical insufficient-data error requirement remains conflicted with the
current guide's reduced-order behavior. No production/core file changed.
See ``Qt6ParityValidation/nasa-ephemeris-sampling-20261027.md``. Arbitrary
physics/frames, binary/covariance/standard conformance, exact off-grid user
endpoint policy, platform/stress and full current-build regression remain open.

The preceding increment adds eighteen bounded scopes: seven from nine new
Window-menu cases and eleven from reusing six Ephemeris/About results.
Close Current/All and separate vertical/horizontal tiling were missing GUI
operations and are now implemented. Native pointer-driven cases qualify
geometry before/after resize, cascade, complete navigation cycles and dirty
Save/Discard/Cancel with unapplied resource guards; all run a literal9 mission
witness. The expansion excluded all663 preceding checks. One directly
affected Window-entry regression was rebuilt and passed separately.
Ephemeris/About reuse retains exact source/terminal assertions and20 analytic
error observations, but temporary raw outputs and per-widget exposure were not
retained. It does not imply full CCSDS/SPK conformance, all interpolation or
arbitrary event science. MainWindow.cpp is the sole production change;
the35-file core inventory is unchanged. See
``Qt6ParityValidation/nasa-window-menu-20261027.md`` and
``Qt6ParityValidation/nasa-ephemeris-about-reuse-20261027.md``.
Full current-build regression, native OS input, platform/stress and current
private NASA provenance remain open.

The preceding increment adds six calculation scopes through twelve actual
Assignment GUI cases: four B-plane readonly quantities and two outgoing-angle
setters, each in equatorial/ecliptic inertial frames. Independent vector/conic
geometry checks92 values plus12 sentinels, including all Cartesian components,
and52 syntax/readonly rejections. The first visible pass is retained by resume;
all651 prior checks are excluded. This first visible qualification also runs the
automated assertions; duplicate unattended development execution is deferred
to the integration gate. No production/core change was made. See
``Qt6ParityValidation/nasa-orbit-calculations-20261027.md``. Test-only source-read caching also reduces a
measured current coverage evaluation from1.348 to0.036seconds, with identical
complete report JSON. Every call rehashes content; same-size/timestamp drift
is rejected. Two new contracts and all three support checks pass. This is a
reporting measurement, not a suite-wide speedup. See
``Qt6ParityValidation/nasa-traceability-cache-20261027.md``. Rotating frames,
singular/nonhyperbolic and runtime-error domains, current private corpus,
native OS input, hardware/platform/stress and final full regression remain open.

The preceding increment reuses five preserved native burn results for three
Element component definition/persistence scopes, without GUI execution.
The audit rechecks1,260 numbers and independently projects45 measured delta-V
components. Scripted mission inputs overwrite the GUI-created components;
the two proofs are explicitly separate. V/N/B deprecated aliases remain
uncovered. Unattended and full current-build qualification are not inferred.
No production/core change was made. See
``Qt6ParityValidation/nasa-burn-component-reuse-20261027.md``.

The preceding increment adds fifteen orbital parameter scopes from twenty actual
Mission Assignment cases, checking the reported parameter plus all six
Cartesian components with independent forward/inverse geometry. Both fast
and first native qualification check140 values,20 sentinels,60 syntax
rejections and6 readonly destination rejections. Fourteen passing development
cases were retained; only unresolved/new cases ran after corrections. The631
preceding checks were excluded from expansion, with two shared-driver impact
regressions recorded separately. The guide's spherical defaults and signed
angle ranges are explicit in the oracle. Five historical mutability rows
remain unresolved; prior E519 read-component evidence remains versioned.
A real native RMAG/VMAG delegation bug required four field-name string changes
in OrbitData; the35-file core inventory preserves the diff for scrutiny and
verifies that the previous34 core files are unchanged. See
``Qt6ParityValidation/nasa-orbital-parameters-20261027.md``. Broader origins,
frames, singular/invalid runtime domains, native OS input, platform/stress,
current corpus and final full current-build regression remain open.

The preceding increment adds twenty parameter scopes by re-auditing48 preserved
results, without running GMAT GUI scenarios. Attitude/rate, epoch and ballistic
GUI property definitions/readback/results retain their original independent
science/literal oracles. Assignment/function/parameter-picker setter routes
remain separate gaps. These are source-row component scopes, not all parameter
operations or a current-build pass. Test-only capture reuse also preserves all
live capture checkpoints while reusing an adjacent identical PNG. A controlled
scale3 native probe falls from3.02 to1.37 seconds with all15 decoded frames
and12 explicit boundaries identical; this is not a general suite speed claim.
The28 runner contracts pass, and the strengthened registry check exposed and
corrected nine mechanism-map omissions. The core34-file inventory is unchanged.
See ``Qt6ParityValidation/nasa-parameter-component-reuse-20261027.md`` and
``Qt6ParityValidation/capture-reuse-20261027.md``.

The preceding increment adds twenty-nine requirement scopes: twenty-seven from
thirty-two report-output cases and two from re-auditing existing Toggle plot
results without replay. Two historical Report operation scopes reuse the same
newly qualified results without further execution. Both unattended and first native qualification check
34 mission runs, 98 values and 96 invalid Apply/OK rejections per mode. The
26 passing development results were retained; retries ran only four failures,
plus two new fixed-header boundaries and two affected existing regressions.
The native desktop starts minimized, remains resizable and never uses
fullscreen. Report formatting, disabled-file preservation and stale final
solver-row bytes required two core file changes; the 34-file core inventory
and incremental patches are preserved for scrutiny. Guide conflicts, broad
input/solver/file domains, per-control legacy exposure, current corpus,
platform/stress and full current-build regression remain open. See
``Qt6ParityValidation/nasa-report-output-20261027.md`` and
``Qt6ParityValidation/nasa-toggle-output-reuse-20261027.md``.

The preceding work adds sixty requirement scopes: thirty-seven from forty-two
new Assignment expression cases and twenty-three from re-auditing eight
preserved control-flow/Script Editor results without replay. Assignment cases
pass unattended and first native qualification with independent scalar/matrix
oracles, 126 invalid rejections per mode, exact history and Unicode reopen.
All 557 preceding checks were excluded from the expansion. Passing development
receipts were retained; retries were limited to failures and checks directly affected by fixes. GUI fixes correct
math/function form classification, pre-Apply array dimensions and transpose
quote parsing. The core 33-file inventory is unchanged; two targeted existing
parser/function regressions are recorded separately. The current private NASA
baseline, broader domains, per-control legacy visibility, platform/stress and
full current-build regression remain open. See
``Qt6ParityValidation/nasa-assignment-expressions-20261027.md``,
``Qt6ParityValidation/nasa-control-flow-reuse-20261027.md`` and
``Qt6ParityValidation/nasa-script-editor-reuse-20261027.md``.

The preceding increment checks actual GUI-to-rendered model transforms and a
shipped3DS asset. All550 preceding checks were excluded. The independent
PNG audit verifies21 images per mode (18 geometric checks plus3 asset images),
21 invalid-input rejections per mode,56 exposed native boundaries and204
observer screenshots. It caught reversed multi-axis model rotation order;
OrbitRenderer now matches NASA's original GUI. This is a GUI-only fix; the
33-file core inventory is unchanged. Six passing fast cases and six passing
visible cases were retained while only unresolved cases were retried. E45 is
newly covered; E47/E48/E49 are strengthened. Arbitrary assets/cameras, body
transforms, host hardware, POV applicability and final full regression remain
open. See ``Qt6ParityValidation/nasa-visual-transforms-20261027.md``.
The preceding increment reuses six preserved results from three existing tests
without executing any GUI tests. Reviewed Workflow, ModelDrawingRuntime and
NativeModelDrawing assertions add bounded translation/rotation/scale scopes
(E47/E48/E49): GUI persistence, retained plot-model transforms and independent
private software-renderer scaling. Component proofs are distinguished from
complete GUI-to-pixel qualification. Actual3DS loading remains open. The
historical POV requirement (E46) conflicts with the current bundled NASA guide
and is recorded as unresolved, not covered or retired. No production/core
change was made. See ``Qt6ParityValidation/nasa-visual-reuse-20261027.md``.
The preceding increment verifies fifteen complete attitude/rate definitions:
all twelve Euler sequences and quaternion/DCM/MRP. Every case exercises all
four orientation and both rate conversion targets and independent60-second
constant-spin results. All535 preceding checks were excluded. The new cases
passed first in unattended mode and first native qualification; the audit
checks90 conversions,1,080 attitude/rate values and120 invalid rejections per
mode, plus330 exposed boundaries and660 PNGs. Eleven bounded scopes are
added (E25–E34 and E51). E51 explicitly reuses the same execution through a
post-execution mapping; no receipt or snapshot is rewritten. Existing Workflow
attitude assertions were audited as context without replay. No production/core
change was required; other frames/models, singular domains, long-duration
behavior and final full regression remain open. See
``Qt6ParityValidation/nasa-attitude-definitions-20261027.md``.
The preceding increment verifies all ten documented epoch input formats and100
directed format conversions at selected2000 dates. All525 preceding checks
were excluded; the ten new cases passed first in unattended mode and then
first native qualification. Both modes independently check100 conversions,
40 epoch checkpoints,50 report values and40 invalid Apply/OK rejections.
Native evidence retains260 exposed boundaries and430 PNGs. Seven bounded
requirement scopes are added (E14/E17/E18/E21/E22/E23/E24), and four are
strengthened. Input precision checks remain separate from50us TDB oracle
comparison and0.51ms Gregorian display rounding. No production/core change
was required. Arbitrary dates, leap boundaries, globally microsecond-accurate
TDB conversion and the final full regression remain open. See
``Qt6ParityValidation/nasa-epoch-definitions-20261027.md``.
The preceding increment verifies six positive orbit definitions through actual
primary controls, with independent Cartesian oracles, labels/units, invalid
Apply/OK recovery, exact history, Unicode reopen and mission readback. All
519 preceding checks were excluded; three fast passes were retained while only
three failures were retried. All six passed their first native qualification.
The audit checks 180 state components and 32 invalid rejections per mode,
220 exposed native viewing boundaries and 332 valid PNGs. Five new scopes
(E7/E8/E10/E11/E12) are added and E6 is strengthened. The units and pre-conversion
input checks were repaired. A two-line core fix activates existing spherical
magnitude validation; it is separately preserved for scrutiny in the new
33-file core inventory. One orbit/frame per representation is covered;
arbitrary frames/conversion pairs, singular/hyperbolic inputs and full current
build qualification remain open. See
``Qt6ParityValidation/nasa-orbit-definitions-20261027.md``.
The preceding increment reuses exact-source Workflow assertions from preserved
unattended/native receipts and the already audited positive SpacecraftEpoch
case. **No GUI scenario was executed or replayed.** Five previously uncredited
bounded requirement scopes are linked: Classical Keplerian, Gregorian,
Modified Julian Date, TAI and UTC. Per-panel visible exposure, arbitrary frames,
all input fields, independent absolute state/time accuracy and leap domains
are not inferred. Display-only ModifiedKeplerian/Equinoctial/spherical checks
remain uncredited as committed definitions. See
``Qt6ParityValidation/nasa-legacy-orbit-epoch-20261027.md``.
The preceding increment uses one shared driver for 16 command-panel contexts.
**All 503 preceding checks were excluded.** Fast development passes are retained;
first native qualification is separate, and no passing case was replayed within
its mode. The audit adds 108 distinct historical operation scopes, with no
FRR/FRAC credit. Each mode checks 64 invalid Apply/OK rejections, 32 Help routes,
64 MDI window actions, 492 focus visits and 38 independent report values.
All 496 explicit native viewing boundaries and 1,216 valid PNGs are retained.
The 16 retained fast passes total 11.806 seconds of test runtime, excluding
build/snapshot/debugging overhead. Qt fixes add command-source Tab navigation
and pre-Apply validation of existing Global object names. The 32-file core
inventory remains unchanged; its technical scrutiny is still pending.
Selected fields, initial-guess solver/scalar limits, native-input and full
regression gaps are explicit. See
``Qt6ParityValidation/nasa-command-mechanisms-20261027.md``.
The preceding increment adds four separate spacecraft epoch/attitude/mass/visual
page cases. **All 499 preceding checks were excluded; no passing case was
replayed.** Actual Help routes, focus cycles, MDI window restoration, invalid
Apply/OK atomicity, correction, exact history/Unicode and independent Keep=9
mission results are directly checked. The editor now scrolls the mass and visual
forms instead of exceeding the workspace; embedded model normalization matches
the existing modal editor. Exact window assertions remain, with stronger
workspace/field/action containment checks. The visual case also checks actual
slider Home/End endpoints and typed focus-out clamping to NASA's 0.001/1000
scale boundaries. The audit verifies 16 invalid rejections, eight Help routes,
16 window actions, 228 focus visits, four independent mission results,
112 explicit viewing boundaries and 264 valid PNGs. This adds 32 historical
operation scopes and no FRR/FRAC credit; per-field/page/input/science and
native input/platform/full-regression limits remain explicit. See
``Qt6ParityValidation/nasa-spacecraft-panels-20261027.md``.
The preceding increment adds nine panel mechanism cases across selected resource
families: actual Help button/F1/Escape, forward/reverse Tab, MDI system-menu
max/min/restore, invalid Apply/OK atomicity, valid recovery, exact history and
Unicode persistence, plus independent Keep=9 mission results.
**All 490 preceding checks were excluded; no passing scenario was replayed.**
Eight passes were retained while only unresolved XYPlot was retried. Its fixture
now saves normal geometry after layout minimum constraints settle; exact restore
assertions remain. No production source changed. The audit verifies 36 invalid
rejections, 18 Help routes, 36 window actions, 346 focus visits, 9 mission results,
243 explicit viewing boundaries and 585 valid PNGs. This adds 53 distinct
historical scopes and strengthens three existing scopes, with no new FRR/FRAC
credit. One selected field per family, Qt synthetic input and unavailable
original keyword-body limits remain explicit. See
``Qt6ParityValidation/nasa-panel-mechanisms-20261027.md``.
The preceding increment adds five impulsive body/named/user-origin frame cases.
**All 485 preceding checks were excluded; no passing scenario was replayed.**
All five passed first execution. Fifteen forward/reverse pairs check independent
velocity/DCM/basis/rocket results and restoration, plus actual Create/Cancel and
Maneuver/BackProp controls. The audit checks 1,260 values: 990 independent
literal/matrix/rocket values and 270 conditional shared ephemeris translations.
All 77 explicit viewing boundaries and 169 valid PNGs are preserved. This adds
FRR!E94 and strengthens ten existing scopes; complete/any-frame E88/E89 remain
unqualified. See ``Qt6ParityValidation/nasa-impulsive-frames-20261027.md``.
The preceding increment added six integrated chemical-thruster cases: regulated
and blow-down pressure, pressure-dependent thrust/Isp, optional decrement,
active duty/scale changes, shutdown/coasting/restart and independent trajectories.
**All 479 preceding checks were excluded; no passing scenario was replayed.**
Independent mass-time and acceleration-based calculations audit 1,800 reported
values, 126 explicit viewing boundaries and 240 valid PNGs. Seven already-counted
row scopes are strengthened; **zero new distinct rows** are added, so the
headline percentage stays unchanged. No production change was needed.
See ``Qt6ParityValidation/nasa-thruster-integration-20261027.md``.
The preceding increment added eight actual ChemicalThruster creation/frame cases.
**All 471 preceding checks were excluded; no passing scenario was replayed.**
It checks local inertial/Earth and active user-Ceres VNB/LVLH, a nonidentity
spacecraft-body DCM, named ecliptic and object-referenced force geometry,
actual tank/spacecraft/burn attachments, invalid creation and exact history.
The audit checks 1,560 reported values: 1,320 independent literal/matrix/force
values and 240 relative-state values conditional on shared SPICE input.
All 170 explicit viewing boundaries and 422 valid PNGs are retained.
This adds six bounded row scopes; complete/any-frame FRR!E68/E69 remain
unqualified. No production change was needed. NASA's attitude-model rules and
new-thruster defaults required fixture corrections before the first pass.
See ``Qt6ParityValidation/nasa-thruster-frames-20261027.md``.
The preceding seven chemical polynomial/duty/scale/domain cases added four
bounded row scopes and checked 805 numerical values. A non-real polynomial
previously produced NaN with successful mission completion; the core now
rejects non-finite thrust or Isp. Six passes were retained while Domain alone
was rerun. See ``Qt6ParityValidation/nasa-thruster-polynomials-20261027.md``.
The collection retains passes across fixes and does not qualify a single
current-build/full-regression run. General multi-engine/mixture/high-order integrated depletion/trajectory,
thermal models, arbitrary polynomial/frame/input domains, independent ephemeris
and attitude dynamics, broader impulse/big-maneuver/convergence domains, physical accuracy,
native input/platform/stress, the unavailable current NASA corpus and separate
unattended/final full regression remain open.
The earlier increment reused three existing Thrusters/Burns/ChemicalTankApply
tests for eighteen previously uncredited scopes with zero GUI executions or
replays. Their historical exact PASS/source/JUnit/hash gates remain preserved,
with explicit capped-screenshot and missing-raw-report limitations.
See ``Qt6ParityValidation/nasa-legacy-hardware-reuse-20261027.md``.
The preceding eleven tank-parameter scenarios/eighteen scopes are documented in
``Qt6ParityValidation/nasa-tank-parameters-20261027.md``.
Historical RMAG read-only wording conflicts with the current settable guide;
SemilatusRectum also has contradictory settable flags in the current guide.
These receive no inferred read-only enforcement credit. The latter row remains
unqualified. A static guide census records the remaining parameter domains.
Tank zero-state guide/setter discrepancies, equality at capacity and integrated
depletion/thermal models also remain explicit gaps. The new tank cases impose
mass changes to check pressure response; they do not qualify integrated flow.
The preceding ten read-only parameter scenarios/ten scopes are documented in
``Qt6ParityValidation/nasa-readonly-parameters-20261027.md``.
The preceding nine recursive-frame scenarios/eight scopes are documented in
``Qt6ParityValidation/nasa-function-frames-20261027.md``.
The preceding six function-control scenarios/ten scopes are documented in
``Qt6ParityValidation/nasa-function-control-20261027.md``.
The preceding ten function-I/O scenarios/twenty scopes are documented in
``Qt6ParityValidation/nasa-function-io-20261027.md``.

The last audited build rejects degree/order beyond the selected potential file's
limits and invalid tide-file inputs, including a directory that previously
hung the parser. Its audited unattended collection passes all 255 checks,
including the original 11 failure cases. A corrected file-picker test result
replaces only that one test; the other 254 passing results are retained on the
same runtime. All prior assertions remain intact.

The full visible replay of that preserved build was stopped after 50 passes
when the user prioritized coverage expansion over repeating regressions.
Its partial receipt is retained and does not qualify the remaining 205 cases.
The preceding build's full visible run also passed 255 checks, with native
exposure for all 202 visible routes and 2,282 valid snapshots. That older run
is historical evidence, not qualification of the current parser fixes.

Last audited unattended collection: **157/1,158 requirement-instance
scopes (13.6%)** and **636/2,367 historical-operation scopes (26.9%)**.
These are bounded proofs, not percentages for all widgets or all input domains.
The collection includes 77 accuracy-policy scenarios, 84 terminal-step
scenarios, five original error-control modes, 12 gravity-bound GUI cases,
one mixed-force transaction case, 16 file/STM domain cases, 23 tide-field
cases, three atmosphere model/runtime proofs and
visible body-selection proofs for Earth harmonic gravity, all eight major
planets as point masses, Pluto and user-defined Ceres.
Eleven source discrepancies remain explicit, including normalization-formula
and variational-notation discrepancies in the shipped mathematical specification
and reversed latitude/longitude ordering in a GroundStation example paragraph.
The historical user-body two-body requirements also conflict with the shipped
SPICE-only guide and explicit runtime deprecation guards.
The historical SPK Origin setting also conflicts with the current guide and
intentionally inactive CentralBody setter.

The last audited collection record is
``build/nasa-coverage/tide-review/validation-collection.json``.
It verifies the same production inputs and the one reviewed test-harness
correction. The full unattended baseline is
``build/gui-acceptance/20261027-tide-suite-headless/results.json``; the corrected
test has its own preserved unattended and visible receipts. The full visible
counterpart is ``build/gui-acceptance/20261027-tide-suite-visible/``.
The prior 255-check collection, including four earlier test enhancements, is
audited as historical in ``build/nasa-coverage/pre-tide-history/``.

The live frontend now also contains a Gravity and tides dialog, with dependent
None / Inherited / Tide File source controls, file-dependent model choices,
potential presets, visible file limits and per-body pending transactions.
Five separate tests qualify this addition: ``QtGui.NasaGravityDialog`` and
``QtGui.NasaTideEarthInherited``, ``QtGui.NasaTideEarthFile``,
``QtGui.NasaTideEarthPole`` and ``QtGui.NasaTideLunarFile``. This allows a single
unverified case to run without repeating the other scenarios. The existing
52 gravity-validation proofs are unchanged.

All five new tests pass unattended, producing 16 structured proofs. The three
Earth mission tests verify two previously unreviewed requirement instances,
``FRR!E138`` (Solid Earth Tides) and ``FRR!E139`` (Solid and Pole Tides), within
an explicit GUI-configuration scope. Four Earth/lunar missions check the running
owned gravity field and six state components after 600 seconds against an
equivalent authored GMAT script. Both routes share GMAT's force equations;
independent tide-physics accuracy is still unqualified. All five new cases also
pass visibly in a minimized, resizable desktop, with native exposure for each
case and 56 valid PNG snapshots. The source/receipt/JUnit/proof audit is
``build/nasa-coverage/gravity-dialog-review/validation-summary.json``. Receipts are in
``build/gui-acceptance/20261027-gravity-dialog-{headless,visible}/``.

These are two additional bounded requirement scopes, not a fresh full-build
qualification. The prior 157-scope collection and the newer two-scope receipt
have different frontend versions. Do not present their union as a current
260-test pass. The historical-operation count remains 636; no broad Dynamics
Model validation row is closed by these cases. Further full-suite runs are
deferred until the feature batch is ready.

User-defined non-spherical gravity
---------------------------------

``QtGui.NasaUserGravity`` adds a separately selectable case for ``FRR!E106``.
It selects user-defined Ceres through the primary-body picker, chooses a
synthetic degree-two COF through the file picker, checks Cancel and invalid
file-bound correction, and verifies pending Apply, exact Undo/Redo, Unicode
save/reopen and the running owned gravity field. The potential's constants
deliberately differ from those on the body, qualifying the documented file
precedence without altering the body definition.

The scenario exposed a valid GUI Apply rejected as ``Property is read-only``:
``GravityField::IsParameterReadOnly`` listed only five editable TideModel
bodies. Its metadata now follows the existing data-dependent model setter,
as described by the guide and used by the wx GUI. A detached custom-body probe
accepts Solid with supplied Love numbers and rejects both Solid without data
and Earth-only SolidAndPole. No numerical force equations changed.

The synthetic field has normalized C20 = -J2/sqrt(5), J2 = 0.01,
mu = 63 km^3/s^2 and reference radius 470 km. A circular equatorial orbit of
radius 1,000 km uses v = sqrt(mu/r * (1 + 1.5*J2*(R/r)^2)). Its exact Cartesian
state after 600 seconds is evaluated independently of GMAT. The GUI mission
agrees to 6.48e-8 km in position and 1.83e-11 km/s in velocity. Its monopole
control differs by 0.0377 km, establishing sensitivity to the non-spherical
term. This is one synthetic analytic configuration, not empirical Ceres gravity
or general harmonic accuracy.

The new case passes unattended and visibly, with 32 valid visible PNGs.
Its source/receipt/JUnit/proof audit is
``build/nasa-coverage/user-harmonic-review/validation-summary.json``.
None of the preceding 260 checks was selected. Their earlier receipts remain
historical after this shared-core metadata change; full regression qualification
is deferred to the completed feature batch.

The versioned ledger ``build/nasa-coverage/coverage-ledger.json`` records
**196/1,158 demonstrated requirement scopes (16.9%)** and
**637/2,367 historical-operation scopes (26.9%)**, across audited preserved
builds. Nine station, thirteen calculated-point and eleven differential-corrector
scopes have current reviewed production bindings;
the preceding scopes retain their own preserved-build evidence. These cumulative demonstrations
are not a current-build pass or percentages for all widgets/input domains.
Thirty-nine scopes have been added since the 157-scope baseline without selecting
the preceding regression cases in these focused qualification runs.

Station coordinates and independent geometry
-------------------------------------------

Ten independently selectable ``QtGui.NasaStation*`` cases qualify the bounded
coordinate scope of ``FRR!E174`` through ``FRR!E182``. They drive the actual
embedded New/resource forms, with Earth, Cartesian/Spherical state types and
Sphere/Ellipsoid horizon references. Nominal cases cover all four state/shape
combinations; spherical cases also cover both poles and the equator. Body
radius 6,400 km and flattening 0.01 deliberately differ from defaults.

Each case checks Create/Cancel, labels/units/dependent selectors, preview
conversion, pending Close/Cancel/Discard, invalid Apply/correction, exact
creation/edit Undo/Redo, Unicode save/reopen and a fresh sandbox object.
An independent geocentric/geodetic formula checks all three body-fixed
coordinates after Create and Apply to 1e-8 km. A second date must give the
same body-fixed position, and the fixture executes its known mission result.
Across the cases, 28 invalid-input rejections retain pending input and leave
source/model/history unchanged.

The new zero-height boundary cases found a production bug: creation filtered
out coordinates equal to scalar defaults even after their meaning changed.
Spherical height zero was omitted because the original Cartesian Z was zero;
changing the representation first left a converted negative height instead.
``ResourceEditor`` now retains the full coupled location tuple when any
location selector/component changes during creation. No geometry assertion
was weakened; the test source is byte-identical to its pre-fix backup.

All ten new cases pass unattended and visibly, with 296 valid visible PNGs.
The six repaired boundary passes were reused while completing the unattended
collection. None of the preceding 263 checks was selected. Receipts are in
``build/gui-acceptance/20261027-station-coordinates-fixed-{headless,visible}/``;
the source/JUnit/proof/geometry/snapshot audit and exact production diff are
in ``build/nasa-coverage/station-coordinates-review/``. Visible playback starts
minimized in a resizable desktop. To watch one new boundary case::

   python3 src/qtgui/tests/RunAcceptance.py --visible --start-minimized --filter '^QtGui.NasaStationNorthEllipsoid$'

The existing Add/Apply/Load historical operation mappings are retained and
receive no duplicate credit. Broad station valid/invalid validation rows
``H2012:R379`` and ``H2012:R380`` remain reviewed but unverified: coordinate
cases do not establish all ID, elevation, mask, hardware, color/model-reference
inputs or the missing original keyword sequences. Empirical geodesy,
time-dependent inertial orientation, contact/measurement accuracy, exhaustive
input domains and native/platform/stress qualification remain open.

Calculated-point states and creation defaults
--------------------------------------------

The existing ``QtGui.CalculatedPoints`` test and its assertions are unchanged.
It already checks several body-list operations and L1--L5 position geometry.
Nineteen new, separately selectable ``QtGui.NasaCalculated*`` cases add the
missing six-component results, default creation and reference combinations.
Fifteen cases cover all five points with celestial bodies, a barycenter
primary, or a barycenter secondary. Three barycenter cases cover mixed
default/user membership, a single user body and untouched defaults; a final
libration case creates the untouched default Sun/Earth/L1 configuration.

The tests use the actual New/resource editors and displayed selectors,
Cancel, invalid Create/Apply correction, exact source/model/history isolation,
Undo/Redo, Unicode save/reopen and fresh sandbox objects. They check four
epochs after Create and Apply: 152 state vectors / 912 components and 38
invalid-input rejections per mode. Known synthetic GM constants prevent a
test from merely accepting whatever default mass ratios happen to be present.
Ceres uses the shipped 1900--2100 SPK input; ephemeris truth is not independently
qualified by these combination tests.

Barycenter positions and velocities are independently weighted by GM rather
than calling the barycenter's mass/state implementation. L1--L3 positions
use a bracketed physical equilibrium equation instead of the engine's Newton
iterations on quintics; L4/L5 use triangular geometry. The velocity oracle
uses a five-point derivative of independently constructed positions under
locally linear relative motion. The audit also checks the exact local linear
velocity formula, agreeing within 2.14e-14 km/s. NASA's shipped mathematical
specification explicitly assumes zero relative acceleration for this model.
The general derivative of changing ephemerides/orbit planes remains the
``LIBRATION-VELOCITY-EPHEMERIS-PLANE-DERIVATIVE`` gap.

The creation tests found a production mismatch before reaching the state
checks: factory-only previews left calculated-point defaults empty. New
LibrationPoint displayed a blank sixth point choice. ``ResourceDraft`` now
matches ``Moderator::CreateCalculatedPoint`` and the source guide, using
Sun/Earth/L1 or Earth/Luna defaults without registering a pending resource.
The five-choice assertion is retained, and explicit default assertions were
added. Untouched Create is checked as well as edited creation.

All nineteen cases pass unattended and visibly in a minimized, resizable
desktop. The audit checks 746 valid PNGs and native exposure for each case;
two representative picker/creator captures were also inspected for readable
content. Snapshot caps remain explicit and do not prove every intermediate
step was captured. None of the preceding 273 checks was selected. The
source/receipt/JUnit/geometry audit and exact production diff are in
``build/nasa-coverage/calculated-state-review/``; receipts are in
``build/gui-acceptance/20261027-calculated-state-{headless,visible}/``.
For one new visible case::

   python3 src/qtgui/tests/RunAcceptance.py --visible --start-minimized --filter '^QtGui.NasaCalculatedL4BaryPrimary$'

The batch adds thirteen bounded requirement scopes, ``FRR!E245`` through
``FRR!E257``. The existing barycenter Add operation mapping is retained without
duplicate credit. Broad historical Libration Points tree/dialog/validation
rows remain reviewed but unverified. Full regression qualification of the
changed preview helper remains deferred to the completed feature batch.

Historical user-body orbit requirements
--------------------------------------

``FRR!E220``, ``E228``--``E236`` and ``E238`` retain requests for two-body
propagation or parent lists containing two-body orbital-element controls.
The shipped CelestialBody guide now specifies only SPICE for custom-body
ephemerides. ``CelestialBody.cpp`` explicitly throws deprecation exceptions
for the two-body element setters and marks those fields read-only. These
eleven source instances are reviewed but unverified: no authoritative
retirement disposition has been reconciled, and SPICE passes are not credited
as the named removed mode. The workbook remains unchanged. This is the tenth
recorded source conflict and the explicit
``HISTORICAL-USER-BODY-TWO-BODY`` acceptance gap.

The audit identified a source-retention gap: references attached only to
conflict findings were not included by the snapshot collector when no passing
test claim mentioned them. The calculated-state GUI snapshot omits
``CelestialBody.cpp``, ``SolarSystem.cpp`` and ``HarmonicField.cpp``; those
source-only findings therefore have a separate
preserved source-review collection, not an implied source file in that GUI
snapshot. The collection retains all hashed conflict references at
``build/nasa-coverage/calculated-state-review/source-review/manifest.json``.
Archived GUI inputs/receipts remain unchanged. Future snapshots now include
these references. One new end-to-end runner contract verifies that such a
file is copied and survives a later working-tree edit; no GUI case was rerun
for this collection fix, and it adds no requirement credit.

Differential corrector controls, iterations and output
----------------------------------------------------

Eighteen new, separately selectable ``QtGui.NasaDifferential*`` cases cover
``FRR!E261`` through ``FRR!E271``. The existing solver, correction, operand,
mode and lifecycle assertions remain unchanged. Three difference methods
are checked with progress enabled/disabled, maximum nominal passes one/two,
and an initially solved one-pass boundary. Additional Forward cases cover
Normal, Concise and Debug reports; the three method cases use Verbose.

The actual New/resource editors exercise typed controls, non-writing output
picker Cancel/selection, atomic invalid input correction, exact creation/edit
Undo/Redo and Unicode save/reopen. The target is independently specified as
F1 = X^2 + Y = 11 and F2 = 3X + 2Y = 13. Its unique positive root in the
selected X interval is X = 3, Y = 2. Perturbations 0.02 and 0.04 deliberately
make the one-sided and central updates distinguishable.

Every nominal and perturbation row, finite-difference Newton update,
reported Jacobian/inverse, desired/achieved goal, variance and iteration
number is checked. The report matrix uses variable rows, so its orientation
is checked rather than assumed. The audit independently rebuilds polynomial
finite differences at high precision, solves by elimination and checks the
matrix/inverse product. Each mode contains 73 nominal iterations, 223
trial/publish rows, 38 matrix pairs and 26 invalid-input rejections. The
single final publish pass is checked explicitly; it is not miscounted as a
second solver iteration.

The cap-one/two cases must retain the last nominal state and continue to
the known post-loop mission result. Initially solved cap-one cases must
converge without reporting an iteration-limit failure. The four-row live
solver table must match the actual controls, desired/achieved goals and
residuals. ShowProgress=false suppresses both message progress and the
configured solver report while retaining the separately requested solver
window. Normal and Concise reports are equal, and all progress/report
policies retain the same numerical trials for a given derivative method.
Raw source, trial/final reports, messages and solver output are retained
beside each test log.

Two underlying defects were fixed without changing numerical correction
equations. The shared filename validator rejected all non-ASCII bytes,
including a valid Unicode solver report chosen through the GUI. It now
retains those bytes while preserving reserved-character, ASCII control and
drive syntax checks. The new ``QtGui.UnicodeFileNames`` support case checks
five actual ASCII/UTF-8 create/reopen paths in a Unicode directory and 114
negative input scenarios. Its passing source/binary/library-matched result
was reused, rather than executed again in both GUI profiles. Native
Windows/macOS Unicode file I/O remains unqualified.

The differential corrector also printed an exceeded-iterations failure
when an initially solved target converged on its allowed final pass. The
message is now conditional on non-convergence. Both convergence and genuine
iteration-limit diagnostics are checked, with no numerical update changes.

Both eighteen-case runs pass; the first unattended ForwardVerbose result
was reused while the other seventeen ran. None of the preceding 292 checks
was selected. Receipts are in
``build/gui-acceptance/20261027-differential-{headless,visible}/``. The audit
checks 882 valid PNGs, exact source/receipt/JUnit bindings and all proof values
in ``build/nasa-coverage/differential-review/validation-summary.json``. The
separate utility receipt is not inserted as a fabricated GUI-run result.

``H2012:R1124`` gains a bounded Validate ShowScript proof. Before Apply, the
actual read-only inspector must show all six known applied DC settings and
preserve pending edits, source, model and history; after Apply it must refresh
to the new settings. This adds one historical operation scope. Existing
Add/Apply mappings are retained without duplicate credit. Broad Apply-error,
control, name and valid/invalid input operations remain reviewed-unverified.

An additional visual audit found that automatic window-show captures could
precede painting of the final MDI result. A larger budget alone still gave
only early frames. ``acceptanceView`` now emits an explicit viewing-boundary
event after event processing; the test observer captures it under the same
image/checkpoint budgets. It does not enter a nested event loop or change
ordinary unattended behavior. One new observer contract and one focused
ForwardLimit1 visible follow-up qualify this fix; the other seventeen solver
results were retained. The native late-stage captures were directly inspected:
X=2, Y=1, goals 5/8 versus desired 11/13, residuals -6/-5 and "Did not converge"
are readable and correct. The follow-up has 63 PNGs and fifteen explicit
boundaries, with its own source-bound audit at
``build/nasa-coverage/differential-review/capture-validation-summary.json``.
The eighteen-case cohort and later capture-only harness are versioned
separately; they are not one current full-suite pass.

To watch the new solver result case in a minimized, resizable desktop::

   python3 src/qtgui/tests/RunAcceptance.py --visible --start-minimized --max-checkpoints 200 --max-screenshots 200 --filter '^QtGui.NasaDifferentialForwardLimit1$'

General ill-conditioning, scaling, active bounds, Broyden variants and
propagation targeting remain open. The shipped ``Solvers.tex`` has empty
Differential Correction, Broyden and Newton sections; the bounded polynomial
oracles do not substitute for a complete authoritative numerical V&V corpus.
This is the explicit ``SOLVER-MATHEMATICAL-SPEC-INCOMPLETE`` gap. Native input,
physical/platform/stress qualification and final full regression of the
changed core and capture harness remain deferred.

Cartesian state and orbit STM
----------------------------

``QtGui.NasaStateTransitionForward`` and ``QtGui.NasaStateTransitionBackward``
are independently selectable new cases. They retain the existing covariance
and propagation tests and qualify ``FRR!E141`` (the two listed propagation
quantities), ``FRR!E142`` (Cartesian state) and ``FRR!E143`` (orbit STM) within
one signed circular Earth point-mass arc per direction. No broad Dynamics
Model validation operation is closed by these cases.

The actual Mission/command/assignment controls exercise an empty-group
diagnostic, STM Cancel, pending Apply, exact pending and applied Undo/Redo,
Unicode save/reopen, runtime flag/matrix readback, and STM disable without
stale values. All six Cartesian components are compared with a circular
Kepler solution, and all 36 STM entries with an independent circular-orbit
variational solution. The Hill-frame matrix is transformed to inertial
position/velocity at both endpoints; it is not compared directly as though
it were already inertial. The propagated matrix must also preserve the
canonical symplectic form.

The tests compare dynamics with the actual integration clock. The initial
nominal-time oracle exposed an integration duration of 600.000000167 seconds
and a reported elapsed time of 600.000000070 seconds for a requested 600-second
arc. The guide explicitly warns of epoch time quantization. The cases verify
the warning and its requested tolerance/achieved difference. They do not
qualify the requested 1e-9-second stopping precision; this remains the explicit
``SUBMICROSECOND-STOP-PRECISION`` gap in the reusable acceptance contract.
State and matrix numerical thresholds were retained when the oracle was
corrected to use the achieved integration time.

Both new cases pass unattended and visibly. Their visible results come from
separate preserved runs with identical inputs and viewing profiles; the
passing forward replay was retained while the backward case ran. The audit
verifies 52 valid PNGs, source/log/JUnit identities, the three bounded scopes
in both modes, and 2,179 unchanged production inputs relative to the preceding
user-body qualification. The preceding 261 cases were not selected.
The audit and derived visible collection are in
``build/nasa-coverage/state-transition-review/``.

Initial visible development failures were harness navigation errors: the
correct command editor existed, while active/current MDI queries selected
the script under a minimized outer desktop. The test now locates the sole
real command editor by commandIndex and type, selects it in the inner
workspace, verifies its source and visibility, and reacquires its replacement
after Apply. No production code changed in this batch. General perturbed
STMs, alternate bodies/frames, multi-object/group propagation, ephemeris
propagators and covariance accuracy remain unqualified by these two cases.

The preceding 254-check collection also passed in both modes and is retained
as historical evidence in
``build/nasa-coverage/gravity-validation/pre-fix-history/validation-collection.json``.
Its 251-case visible baseline had native exposure for all 198 visible routes
and retained 2,234 valid snapshots. Its unattended baseline resumed after
SIGTERM, reusing 186 verified same-snapshot passes and running the remaining
65 checks. This older collection does not qualify the newer gravity fix.

There remain 945 unreviewed requirement instances and seventeen reviewed but
unverified instances (LSK editability, Bulirsch minimum tolerance, four
MSISE-86 model/setting rows and eleven historical user-body orbit rows),
1,714 unreviewed historical operations and sixteen reviewed but unverified
operations (five Dynamics Model, two Ground Station, three Libration Points
and six Differential Corrector).
The 20 ForceModel lifecycle scopes exercise an unused resource and one known
field; dependent-propagator references and all-field validation are not inferred.

Two source inventories
-----------------------

``NasaGuiCoverage.py`` inventories both shipped NASA documents:

* ``doc/SystemDocs/Requirements/GMAT Requirements.xlsx`` has 1,158 requirement
  instances. Its document metadata records a 2012-05-06 modification. These
  entries are a historical baseline to reconcile with current help/runtime and
  NASA's current corpus, not proof of the current complete specification.
* The historical 2012 requirements-to-test CSV has 2,367 operation rows across
  62 groups. Every Requirement(s) cell is blank. Those rows cannot supply a
  requirement-ID coverage denominator or a direct requirements mapping.

Requirements use sheet/cell identity, such as ``FRR!E35``. Historical cases use
explicitly local source-row keys such as ``H2012:R2``. Exact ID spellings,
duplicate IDs, deferred entries and unnumbered entries survive extraction.
Fingerprints bind each review to its exact source row and both source files.
Old Passed/Skipped/Disabled fields remain historical data and grant no credit.
No applicability decision silently removes a row from either inventory.

Reviewed execution evidence
----------------------------

``src/qtgui/tests/NasaGuiTraceability.json`` holds reviewed applicability,
rationale, exact assertion-source hash/anchor, test/checkpoint, expected field,
proof facets and bounded claim scope. Referenced specifications/runtime sources
are also hashed. Source changes fail closed until the mapping is reviewed.

An implemented mapping is ``mapped-unverified`` until the selected test passes
and its log includes the matching structured NASA_CHECK proof with every
required facet. The assertion source must match both the reviewed source and
the run receipt. Failed, missing or skipped tests, malformed/missing markers,
wrong fields, incomplete facets and stale assertion sources receive no credit.
A proof covers its reviewed requirement instance only; it cannot cover another
row merely because they share a duplicate ID or belong to one mechanism group.

Run the current requirement tests visibly::

  python3 src/qtgui/tests/RunAcceptance.py --build --visible --start-minimized \
    --filter '^QtGui.NasaBallistics$' --pause-ms 500 --max-screenshots 24

Omit the filter for all registered regressions. Omit ``--visible`` for unattended
operation. Filtered cases remain unexecuted coverage gaps.

Every acceptance report now includes ``nasa-requirements.json`` and
``nasa-traceability.md`` alongside the historical matrix and runtime evidence.
Generate an inventory or analyze a saved receipt independently::

  python3 src/qtgui/tests/NasaGuiCoverage.py --output build/nasa-coverage/inventory
  python3 src/qtgui/tests/NasaGuiCoverage.py --output build/nasa-coverage/review \
    --results build/gui-acceptance/your-run/results.json

First executed requirement batch
--------------------------------

``QtGui.NasaBallistics`` checks FRR-4.1 and FRR-4.4.1 through FRR-4.4.4:
setting dry mass, Cd, Cr, drag area and SRP area. It uses the actual resource
editor and ballistics modal on DefaultSC and UserSat. Checks include nominal,
zero and small positive values; invalid text/non-finite rejection; negative
mass/area rejection; Cancel and pending Apply; repeated Apply; exact source
Undo/Redo and Unicode save/reopen; and a five-field execution report compared
with the known entered values. Proofs are emitted only after the entire
scenario succeeds. Input is currently in-process Qt; no OS-input credit is
claimed for this batch.

The first visible receipt is
``build/gui-acceptance/20261026-nasa-ballistics-visible/``. It passed, recorded
19 native exposure events and retained 24 snapshots. Existing C++ test sources
were left unchanged. The full 183-case predecessor remains separate; this
single-case run does not establish a new full-suite result.

Those five proofs establish the reviewed setting requirements. They do not
establish mass-dependent force-model accuracy, complete input-domain coverage,
every ballistic/SPAD widget, historical case equivalence, or scientific truth
for trajectory propagation. At that checkpoint, parent and other requirements
remained unreviewed.

Reusable resource lifecycle batch
----------------------------------

``NasaResourceLifecycleTests.cpp`` and ``NasaResourceCases.json`` add 26
independently registered resource-family scenarios. The same authored template
uses actual category menus, resource menus, resource editor controls, pending
change prompts and Qt file dialogs. It checks:

* Typed Create, invalid/duplicate/reserved-name rejection and Create Cancel.
* Context-menu Open and double-click Open, correct resource/panel naming,
  context-menu Close and the editor Close button.
* Pending Close Cancel versus Discard, Apply and repeated Apply, exact source
  Undo/Redo and GUI Save As/Load through a Unicode filename with/without an
  already-open panel.
* Default cloning, renaming a default copy and its original, and deleting the
  copy to restore the exact original source.
* Modified cloning, independent edits of original/copy, rename Cancel and
  invalid/duplicate/reserved-name recovery, and confirmed unused deletion.
* Exact unrelated source bytes, typed model/property values and tree/panel
  state. Propagator cloning is checked as disabled, matching the guide.

The families are Spacecraft, ChemicalTank, ChemicalThruster, Transmitter,
Transponder, Receiver, Antenna, GroundStation, Formation, ImpulsiveBurn,
FiniteBurn, Barycenter, LibrationPoint, PropSetup, DifferentialCorrector,
BatchEstimator, ExtendedKalmanFilter, Simulator, XYPlot, OrbitView, ReportFile,
EphemerisFile, Variable, String, CoordinateSystem and Yukon. Each recipe names
the currently exposed property and expected values. Inactive legacy engine
fields were not enabled just to make recipes work; their broader requirements
remain to review.

These scenarios have reviewed links to 25 additional FRAC-6.1 resource-tree
requirements and 200 historical operation rows. The shared parent Spacecraft
tree operations can serve its historical page groups; page-specific editing
is not credited from another page's test. ChangeAll and unspecified input-domain
cases are not closed by a single property edit. Historical rows remain a
separate denominator and their original Passed fields never supply current
evidence.

Run this batch visibly::

  python3 src/qtgui/tests/RunAcceptance.py --build --visible --start-minimized \
    --filter '^QtGui.NasaLifecycle' --pause-ms 500 --max-checkpoints 100

Two missing/blocking GUI behaviors were repaired while building the tests:
the resource menu now offers Open/Close, and display edits on a disabled XY
plot no longer fail merely because the form resubmits an unchanged unset X
reference. The XY scenario also checks that enabling an incomplete plot and
clearing an existing X reference still fail atomically.

The file chooser waits for its actual filename input and enabled submission
button before clicking Save/Open. It retains a bounded watchdog until the
dialog closes; the watchdog includes the explicitly requested viewing budget.
An earlier visible attempt that stalled at Save As was interrupted and retained
in ``20261026-nasa-expanded-visible``. The corrected focused case passed in
``20261026-nasa-picker-visible``. Neither attempt is presented as a passing
complete suite.

The corrected full visible run is
``build/gui-acceptance/20261026-nasa-expanded-final/``: **210/210 passed**,
including every resource-family scenario. It records passing scoped proofs
for 30 requirement instances and 200 historical operation rows. 1,128
requirement instances and 2,167 historical rows remain unreviewed. The saved
validation summary confirms that existing C++ test sources, all 141 selected
source files, executable hashes, recipes/specifications, startup and CTest
registrations did not change during the run. The owned test desktop stopped
normally.

Native exposure was recorded in 150 cases; 586 snapshots were retained. There
are still 47 offscreen-only cases, five support checks and eight visible-route
cases without native exposure. All 17 current coverage-gap categories remain
open. This checkpoint is progress toward the full target, not full NASA GUI
acceptance.

Mission-command lifecycle batch
--------------------------------

``QtGui.NasaCommand*`` adds 29 command scenarios to the same reusable runner.
It covers Assignment, For, If/Else, While, ScriptEvent, Report, Write, Toggle,
Stop, Save, Global, CommandEcho, PenUp, PenDown, ClearPlot, MarkPoint, Maneuver,
BeginFiniteBurn, EndFiniteBurn, Target, Optimize, Propagate, Vary in both Target
and Optimize, Achieve, Minimize, NonlinearConstraint, RunSimulator and
RunEstimator. These scenarios reuse the existing frontend and observer; older
regression assertions remain intact.

Each scenario uses the actual Mission context menu and named template, checks
pending insertion and repeated Apply, opens through the menu and double-click,
rejects malformed command text and invalid labels without losing correction
input, and checks Show Script against applied source while edits are pending.
Close Cancel/Discard, typed-field or source editing, exact whole-source
Undo/Redo, Unicode save/reopen, Delete Cancel/Yes and removal of the selected
branch/body are checked. Insert After must put a neutral assignment after the
selected whole command while retaining enclosing solver branches. Five branch
families also check collapse/expand and the Mission expansion control. The
four plot-command selectors check empty/multiple choices, filtering and Cancel.
PenUp/PenDown offer XYPlot, OrbitView and GroundTrack; ClearPlot/MarkPoint offer
only XYPlot. Fixtures include every supported type plus an excluded ReportFile;
PenUp is applied to GroundTrack and PenDown to OrbitView.
Save/reopen in this batch uses the frontend file methods; native file dialogs
and portals receive no new credit.

Ten scenarios run the edited mission and compare report rows with literal
recipe constants: Assignment evaluates 2+3=5; For sums four increments of two;
If selects its alternate body; While performs four increments; ScriptEvent
executes its body; and Report writes distinct V=0 and i=7 in both requested
orders. Write, Toggle, Global and CommandEcho retain the final report truth.
``NASA_RESULT`` log records preserve expected and observed rows. The remaining
command cases assert editing and persistence, not solver, orbit, finite-burn,
estimation or external-function numerical accuracy.

The initial tests exposed missing insertion templates for PenUp/PenDown.
The GUI now offers those plus ClearPlot/MarkPoint, with typed plot-selection
controls. They reuse the existing engine commands. All 29 scenarios pass
ordinary CTest. The reviewed manifest adds 32 requirement instances and 243
historical operation rows; broad input/array/position tests, archived projects,
Minimize's one-objective restriction, function calls and other unmatched rows
remain gaps. Requirement and historical denominators stay unchanged.

Run this batch visibly::

  python3 src/qtgui/tests/RunAcceptance.py --build --visible --start-minimized \
    --filter '^QtGui.NasaCommand' --pause-ms 500 --max-screenshots 24

Run the complete current catalog by omitting the filter. The rebuilt full visible receipt is
``build/gui-acceptance/20261026-nasa-commands-final/``; its final status must be
read from ``results.json``, ``report.md`` and ``validation-summary.json``. The
earlier ``20261026-nasa-commands-visible`` run stopped with 205 passing checks
and 35 without completed results; its shell reported 143. Concurrent source and
executable changes were observed, so that run is retained as provisional. Its
runner and owned display had already exited when a planned SIGINT was checked;
no signal was sent by that attempt.
Reference/recipe hashes must now match both the reviewed current files and the
run receipt. A current recipe cannot retroactively credit a run that used a
different recipe or did not record its hash. All 13 traceability contract tests
pass, including missing/different run-input fingerprints.

NASA corpus access
-------------------

The public test-plan and GUI-testing pages point to NASA-hosted script and
TestComplete project repositories, including:

* https://aetd-git.gsfc.nasa.gov/gmat/gmat-test
* https://aetd-svn.gsfc.nasa.gov/svn/GMAT/trunk/test/gui/Resources/All_Resources_Tests

The web retrieval tool could not access those URLs on 2026-10-26. This does
not establish whether an authenticated or other approved access path exists.
Their project/input workbooks were not imported or assumed available. The
new historical mappings are reviewed current-operation equivalents with
explicit test assertions; they are not executions of the archived TestComplete
projects. Complete corpus and current-baseline reconciliation remain open.

Additional primary-source review on 2026-10-26 confirmed two limits:

* NASA's 2014 `Verification and Validation paper
  <https://ntrs.nasa.gov/api/citations/20140017798/downloads/20140017798.pdf?attachment=true>`_
  describes about 3,400 GUI unit tests in 90 projects for R2013a, and separately
  describes complete widget/input testing, external benchmarks and end-to-end
  system cases. The shipped 2012 matrix therefore cannot establish even the
  entire later historical GUI corpus, much less a current NASA denominator.
  Those paper counts are historical, not a current-suite inventory.
* The `official public source repository <https://github.com/nasa/GMAT>`_ main
  directory listing has no top-level test corpus. NASA's public `Script Testing
  page <https://gmat.atlassian.net/wiki/spaces/GW/pages/380273291/Script+Testing>`_
  describes the test system as internal and requiring NDC credentials. This
  does not establish that every approved access route is unavailable. No
  authenticated NASA test repository was accessed or imported in this work.

Open source conflicts and assurance work
----------------------------------------

The guide says SRPArea must be positive; wx and the runtime accept zero. The
guide lists nonnegative Cd/Cr; wx accepts finite real values, and the runtime
warns for negative Cd or Cr outside [0,2]. These conflicts stay explicit in the
manifest and reports. Tests retain the existing wx/runtime behavior while the
complete NASA input-domain decision remains open.

The interpreter also forbids a spacecraft from belonging to two formations.
Thus an empty Formation can clone, while an exact copy of a populated Formation
conflicts with the guide's general exact-copy description. The clone dialog
now explains the shared-member restriction before rebuilding. Its scenario
checks atomic rejection and recovery by selecting disjoint members, then
independent edits of both formations. That corrected-copy proof is not counted
as an exact populated-clone proof; the source conflict remains open.

The Mission Tree guide says command Delete does not confirm; Qt uses a
Yes/Cancel dialog. The tests preserve this existing interaction and check both
choices. This remains an explicit source conflict rather than an assertion of
identical archived GUI behavior.

Current-baseline reconciliation, per-widget result oracles, generated/unnamed
controls, alternative user-input paths, full authored tutorial replay, scientific
holds and external truth data, native file portals/accessibility, physical GPU
and compositor behavior, platform/scaling combinations, long stress/soak and
controlled build provenance remain separate acceptance obligations. Five setting
proofs do not close them. See ``Qt6GuiAcceptance.rst`` and the current acceptance
ledger for those gates.

Validate the reporting contracts with::

  ctest --test-dir build/linux-gui -R '^QtAcceptance\.' --output-on-failure

Add tests and reviewed proofs by requirement family. Reuse existing tests only
after inspecting their actual assertions and adding precise proof linkage;
test names, widget observations, screenshots and whole-file source references
are not outcome oracles.

Combined mission checkpoint: 2026-10-26
---------------------------------------

``build/gui-acceptance/20261026-nasa-mission-checkpoint/report.md`` combines
238 passing checks from the interrupted full run, seven passing continuation
checks and four passing latest-source checks. Repeated cases count once: all
241 current registered names have passing evidence, and their direct test-source
fingerprints match their selected receipts. There is no claim of one frozen
build: concurrent source/executable changes and interrupted runners remain in
the raw receipts and combined validation summary.

All 29 new command scenarios passed ordinary CTest and visible replay, including
the stronger plot-type filters. Ten scenarios preserve expected and observed
literal report values. All original regression assertions remain intact: 135 of
136 original C++ files are byte-identical; concurrent work added one assertion
to FunctionFileNamesTests.cpp. Removing that addition reproduces the original
file exactly, and the newer test passed visibly. The focused continuations had
matching source/executable fingerprints at completion; their owned displays
stopped.

Current conservative traceability estimates are **62/1,158 = 5.4%** for reviewed
requirement-instance scopes and **443/2,367 = 18.7%** for historical-operation
scopes. These are distinct denominators; the workbook includes non-GUI
requirements and the historical operations are bounded current equivalents.
There is no established overall per-widget/full-input-domain percentage.
1,096 requirement instances and 1,924 historical rows remain unreviewed.
Coverage estimates will accompany progress updates as requested.

Next priority is executable/test-input snapshots and resumable execution so
concurrent rebuilds and runner interruptions cannot create a mixed-build full
receipt. Then continue per-control/boundary proof expansion, review new tutorial
mechanism mappings and reconcile the current NASA corpus. Full NASA acceptance
and the durable goal remain incomplete.

Existing parameter-test proof reuse
------------------------------------

``QtGui.Parameters`` now emits eight explicit NASA checkpoints after all its
existing assertions pass. No original assertion or control flow was removed:
removing the new JSON includes/helper and proof calls reproduces the prior file
exactly (recorded in ``build/nasa-coverage/parameter-proofs/assertion-integrity.json``).
The visible focused run is ``build/gui-acceptance/20261026-parameter-proofs/``.

These checkpoints cover named Variable creation, real-valued initialization,
named/dimensioned Array creation, real-valued cell editing, String creation and
quote handling, one runtime String assignment, and scalar multiplication. The
existing test checks 5.125/0/10.25/17.25 report values, exact source and Undo/Redo,
Unicode save/reopen, invalid/nonfinite inputs, and the existing GUI's 1000x1000
array boundary. It does not establish every array dimension/type/expression,
all string characters, or native OS input coverage.

The cumulative reviewed evidence is **70/1,158 = 6.0%** requirement scopes and
**443/2,367 = 18.7%** historical-operation scopes. There are now 1,088 unreviewed
requirement instances and 1,924 unreviewed historical rows. The cumulative
receipt in ``build/nasa-coverage/20261026-cumulative-parameters/`` combines the
clean earlier preserved batches with this separately executed parameter case;
it does not claim one complete frozen-build run. The continuing preserved run
uses the newer 70-scope manifest.

Preserved-run checkpoint
-------------------------

The runner now supports Linux project-artifact snapshots, matching-result
reuse, and bounded resumable batches; see ``Qt6GuiAcceptance.rst``. Actual
process-map inspection found all 25 loaded project libraries in the preserved
bundle and none in the changing live project tree. Contract tests verify
changed live inputs, snapshot corruption, missing/changed result evidence,
execution-profile mismatches, and CTest failure/skip/timeout semantics.

A first preserved attempt identified generated solver/segment PNGs and reports
being written into the input tree. That attempt remains marked as an integrity
failure. The corrected implementation puts those products under each case's
``outputs/`` directory without weakening its assertions or its CTest properties.
The three real affected cases passed and the input snapshot remained unchanged.

The corrected bundle is rooted at
``build/gui-acceptance/20261026-preserved-output-fix/snapshot/``. Its selected
executables match a subsequent completed build. An unrelated, unselected
TutorialGuiProbe.cpp change is recorded separately and is not qualified by this
suite. ``20261026-preserved-final-1`` then executed 50 more cases and reused the
three checked output cases: **53/241 passing**, **188 pending**, with all 2,542
preserved files intact. This is a resumable validation checkpoint, not a claim
of a completed full run. ``build/nasa-coverage/current-progress.json`` records
its latest continuation path and the cumulative scope estimates.

Script-editor acceptance expansion
----------------------------------

``MainWindow`` now provides Comment, Uncomment, Indent more, Indent less,
Select All and optional line numbers. Block operations follow the documented
percent-comment and three-space conventions, retain backwards selections and
form one Undo operation. A selection ending at column zero excludes the next
line. Line-number visibility belongs to each open document. Read-only editors
and non-script fields cannot be changed by the new script actions.

``QtGui.NasaScriptEditor`` drives the real menus and Qt keyboard events. It
checks current and selected lines, empty lines, whitespace and embedded percent
characters, grouped Undo/Redo, Unicode clipboard and save/reopen, independent
documents, line-number margins and literal 6/2/6 mission results. The ten new
requirement scopes are ``All Others!E87``, ``E90-E95``, ``E100`` and ``E102-E103``.
This is in-process Qt input, not OS keyboard/accessibility qualification or
exhaustive editor input coverage.

The final visible checkpoint is
``build/gui-acceptance/20261026-editor-final-complete/``. The editor scenario
and all eleven originally failing checks pass; the input snapshot remains
unchanged. Nine completed results were verified and reused after a runner
interruption, then the remaining three checks passed. The full expanded
242-check catalog still has 230 pending checks in this snapshot.

The cumulative 80/443 scope report is
``build/nasa-coverage/20261026-cumulative-editor/``. It combines source-bound
evidence from identified runs and explicitly does not claim one complete
current-build regression run. At this editor milestone, all 149 pre-existing
C++ test files were byte-identical to the pre-editor-change snapshot; the comparison is recorded
in ``build/nasa-coverage/script-editor-assertion-integrity.json``.

The new snapshot follows a completed full acceptance build. Its default
launcher output directory routes to a separate writable location, while the
link and all preserved input files are verified. Visible replay uses the
tutorial runner's host scaling policy and a normal resizable Xephyr window;
it does not request fullscreen. See ``Qt6GuiAcceptance.rst`` for display
controls, periodic estimates and safe resume commands.

Complete unattended checkpoint
------------------------------

``build/gui-acceptance/20261026-unattended-5/`` records **242/242 passing**,
with no failed or skipped checks. The final audit verifies every case identity,
log and JUnit hash, the combined JUnit, and all 2,545 preserved input files.
All eleven originally failing cases pass. The 80 requirement scopes and 443
historical-operation scopes above are verified by this same artifact set.

The five bounded batches use one snapshot; this is not a claim of one
uninterrupted process. The original inputs are byte-identical to the current
visible snapshot, as recorded in
``build/nasa-coverage/build-profile-comparison.json``. An actual process-map
inspection found all 25 loaded project libraries in the unattended snapshot.
System dependencies and compiler provenance remain outside that guarantee.

The corresponding resizable, host-scaled visible run completed with
**241 passing and one failing check**, retained at
``build/gui-acceptance/20261026-editor-suite-5/``. The failure was the XY raster
coordinate-unit mismatch described below; the unattended result did not hide
it. Input domains, all individual controls, native OS input,
physical GPU/compositor behavior, the complete scientific/tutorial corpus,
platform/stress combinations, and the current NASA baseline still have open
qualification gaps. See each generated report for the full gap inventory.

The initial editor candidate audit is retained in
``build/nasa-coverage/editor-next-candidates.json``. The subsequent seven-leaf
implementation and focused proof receipt are described below; the candidate
audit itself provides no coverage credit. The remaining historical operations are grouped in
``build/nasa-coverage/historical-operation-gaps.json`` to guide subsequent
per-control acceptance work.

XY raster coordinates and DPI regression
----------------------------------------

At the host's 3x scale, ``PlotCanvas::captureImage()`` correctly returns an
1800x1200 physical-pixel image for a 600x400 logical-pixel canvas. The XY fixture
was sampling the former with logical coordinates. A diagnostic located the red
marker around physical (597, 564), corresponding to the expected logical
(199, 188). Production rendering and its physical-resolution export contract
were already correct.

``XYAxisTests.cpp`` now checks the capture's DPR and physical dimensions, then
normalizes only its fixed-coordinate fixture to logical pixels. The original
expected coordinates, tolerances, clipping, grid and label assertions remain
unchanged. Removing this adapter and reversing the capture-call substitutions
reproduces the previous source byte for byte; evidence is in
``build/nasa-coverage/xy-dpi/assertion-integrity.json``.

The corrected test passes at 1x, 1.5x, 2x and 3x, including visible replay in
``build/gui-acceptance/20261026-xy-dpi-visible/``. A separate 4x check passes.
Dedicated 1.5x, 2x and 3x registrations expand the catalog to 245 checks, and
the fresh full unattended run passed **245/245** in
``build/gui-acceptance/20261026-xy-unattended-5/``. Its
``validation-summary.json`` independently verifies every case identity, log,
JUnit result and all 2,545 immutable inputs. The corresponding full visible
replay also passed **245/245** in
``build/gui-acceptance/20261026-xy-suite-3/``; its independent audit is saved
as ``validation-summary.json``. These added
regressions do not increase the NASA requirement-scope numerator.

A manual 0.5x check still fails the original full-color marker oracle after
normalization. That lower-resolution raster case is retained as
``SUBUNIT-DPI-RASTER`` in the gap inventory. It is not skipped into a pass and
the color assertion has not been relaxed. Full scale/input-domain
qualification therefore remains open.

Editor search and typing evidence
---------------------------------

The next seven reviewed leaves are Undo typing, Redo typing, Find text,
Find Next, Replace text, Find and Replace, and Go To Line. Existing search,
script-document and script-editing tests are reused. Additions preserve every
original line/assertion and add structured proof records. The typing case
uses Qt key events and checks literal source, caret and dirty-state restoration
through menu Undo and shortcut Redo. The Replace All case checks an independent
complete expected source and the existing literal execution result of 15.
Go To Line checks Cancel, the actual line-count bound and exact line 8.
Together with the previous ten leaves, all 17 listed Script Editor menu
operations in ``All Others!E87:E103`` now have bounded proofs; this does not
qualify every editor widget, input domain or native OS event.

The focused unattended receipt is
``build/gui-acceptance/20261026-editor-search-contained-headless/`` (4/4 passed).
The cumulative source-checked receipt is
``build/nasa-coverage/20261026-cumulative-visible-editor/`` (87/443 scopes).
This is bounded input-domain evidence: Replace-current, multiple-match,
case/whole-word/no-match variations and OS-native input are not claimed.
The frozen full visible replay remains a separate build and does not silently
inherit these new test assertions.

A separate visual review found a SearchRouting capture with report controls
extending beyond the workspace edge. ``WINDOW-CONTENT-VISIBILITY`` records
that native exposure and successful result assertions do not establish that
every control is currently on screen. The selected SearchRouting fixture now
moves each selected MDI document
inside the viewport and asserts containment; this passed the focused
unattended run. The focused visible revalidation also passed, including the
containment assertion. Other fixtures and supported
scales remain unqualified; resizable host-window support alone does not close
this presentation gap. See
``build/nasa-coverage/editor-search/visibility-review.json``.

Completed visible checkpoint
----------------------------

``build/gui-acceptance/20261026-xy-suite-3/`` completes all **245/245** checks
from the preserved project-artifact set, with no failures or skipped checks.
All eleven original failures pass. The independent audit checks every case
identity, log/JUnit hash, combined JUnit and immutable snapshot input.
This includes 193 visible-route, 47 explicitly headless-only and five support
checks. Native exposure was recorded for 183 checks; ten visible-route
checks produced no native exposure and remain listed in the report. This does
not qualify native OS input. The host test desktop remains a normal resizable window at automatic
host scaling, with no fullscreen state.

The four updated editor checks also passed visibly in
``build/gui-acceptance/20261026-editor-search-visible/`` and unattended in
``build/gui-acceptance/20261026-editor-search-contained-headless/``.
Their ``validation-summary.json`` files check individual results and source
identities. Production inputs are identical to the full-suite snapshot;
only four test sources/executables, traceability and gap metadata changed.
The cumulative 87/443 scope receipt explicitly identifies these separate
runs. The NASA goal remains incomplete and active.

Exposure audit and explicit display boundaries
---------------------------------------------

The completed 245-check visible baseline recorded native exposure in 183
checks; ten marked for visible execution had no exposure. Eight contained
actual widget workflows that completed before the display event loop could
expose a window. Those tests now call ``AcceptanceView.hpp`` at explicit safe
boundaries, before modal timers are armed. Visible execution requires a native
platform and waits for an exposed top-level window; ordinary unattended
execution retains its original scheduling. Existing result assertions are
unchanged. The camera reset capture now shows the actual viewer and controls.

The other two cases, ``QtTutorials.OrbitRoundTrip`` and
``QtTutorials.ForceOrder``, only exercise frontend transactions. They remain
registered and executed, explicitly labeled ``qt-acceptance-transaction`` as
support checks. They are not evidence of authored tutorial GUI replay.
The catalog therefore contains 191 visible candidates, 47 headless-only checks
and seven support checks; these route counts are not coverage percentages.

All 13 affected cases (including the other Python-call variants) passed in
``build/gui-acceptance/20261027-exposure-headless/`` and
``build/gui-acceptance/20261027-exposure-visible/``. Every one of the 11 selected
visible candidates recorded native exposure. Each run has a source/snapshot,
log/JUnit and PNG audit in ``validation-summary.json``. The historical full
245-check run remains a separate receipt. No new NASA requirement or operation
scope is credited: the reviewed totals remain **87/1,158 (7.5%)** and
**443/2,367 (18.7%)**. Exposure does not establish complete step visibility,
OS input, physical GPU qualification, or every control/input domain.

Workspace OK and Cancel actions
-------------------------------

The shared workspace panel now offers OK and Cancel alongside Apply and Close.
NASA's ``src/gui/foundation/GmatPanel.cpp`` saves valid pending data on OK and
closes without saving on Cancel. The Qt implementation follows those semantics:
invalid OK stays open, clean OK adds no Undo step, and Cancel discards only
pending edits while retaining an earlier Apply. A successful Apply may replace
the editor widget; the close target is captured before that refresh, so OK
closes its owning MDI window even if focus changes during the callback.
Embedded creation dialogs retain their own existing button workflows.

``QtGui.NasaPanelButtons`` checks scalar and ReportFile resource panels,
assignment and Report command panels, and clean SolarSystem/Earth close paths.
It preserves exact unrelated source, validates negative precision and undefined
Report parameters, checks sibling isolation and exact Undo/Redo, and requires
fresh numerical reports with known values 11, 12, 6, 5 and 17. The first visible
run exposed a separate resource-opening defect: newly created panels could stay
behind the maximized script. The opening path now explicitly activates, raises
and focuses its new panel; the failing assertion was retained.

The final focused runs passed in
``build/gui-acceptance/20261027-panel-final-headless/`` and
``build/gui-acceptance/20261027-panel-final-visible/``. Four historical rows
(``H2012:R1203``, ``R1204``, ``R1905``, ``R1913``) now have these bounded proof
records. No other OK/Cancel rows receive credit merely from the shared helper.
The complete 246-check revalidations are running in
``build/gui-acceptance/20261027-panel-suite-headless/`` and
``build/gui-acceptance/20261027-panel-suite-visible/``. Combined coverage remains
pending until their evidence is verified. The remaining button-row candidates
are preserved in ``build/nasa-coverage/panel-buttons/remaining-button-rows.json``;
parent-panel versus modal-subpanel context must be reviewed separately.

Focused command-button expansion
--------------------------------

The existing command lifecycle scenarios now also check explicit OK and
Cancel without removing any original assertion. They verify clean OK without
an Undo entry, discarding pending changes while retaining a prior Apply,
invalid OK with correction input retained, valid OK committing exact source,
and exact Undo/Redo restoration. A separate ``IfNoElse`` fixture distinguishes
the no-Else historical context from the existing If-with-Else scenario and
checks the independent numerical result ``0,7``.

All 30 affected command cases passed unattended in
``build/gui-acceptance/20261027-command-buttons-headless/``. This adds 44
source-bound historical OK/Cancel scopes. The cumulative receipt is
``build/nasa-coverage/20261027-cumulative-command-buttons-qualified/``.
The already completed 246-check unattended result is
``build/gui-acceptance/20261027-panel-suite-headless-native/``: 229 valid passes
were reused and only 17 native cases were retried after correcting local Xvfb
access. This test-only command extension does not trigger another full rerun;
its focused visible replay is queued after the current visible baseline.

At the user's request, the current outer test desktop is minimized. Future
queued visible replay uses ``--start-minimized``. Inner window states and
assertions are unchanged; restore the outer framed window to watch. The launch
mode was checked with an actual minimized host window and an exposed inner
Qt probe, in ``build/nasa-coverage/minimized-display-smoke/validation.json``.

Focused resource-button expansion
---------------------------------

All 26 existing resource lifecycle cases passed unattended with their original
assertions retained. Added checks cover clean OK without a new Undo entry,
Cancel discarding pending edits while retaining an earlier Apply, OK committing
a known property value, and exact source Undo/Redo restoration before the
existing clone, rename, delete and persistence checks continue.

Only 13 reviewed historical button rows receive new credit: Spacecraft
Ballistic Mass, ImpulsiveBurn, Numerical Integrators, Ground Station, Barycenter,
Differential Corrector and XYPlot. The claims are bounded to the exercised
property and parent-panel context. A ChemicalTank property edit does not prove
the spacecraft tank-attachment panel, and DryMass does not prove the Orbit,
Attitude or Visualization tabs. Per-family invalid-input domains remain open.

The focused receipt is
``build/gui-acceptance/20261027-resource-buttons-headless/`` and the combined
source-bound receipt is
``build/nasa-coverage/20261027-cumulative-resource-buttons/``. Its 87/504 scopes
combine the audited full unattended baseline with the two focused test-only
extensions. The resource visible replay is queued after the command replay,
using ``--start-minimized``. This does not justify another full-suite rerun.

Celestial-body physical input expansion
--------------------------------------

The existing ``QtGui.CelestialBodies`` test retains every original source line
and now checks Mu, EquatorialRadius and Flattening using key events in the
named GUI controls on all eleven default bodies and user-created Moon,
Asteroid, Comet and Planet objects. New bodies are first given valid physical
values: their intentionally incomplete zero defaults must not be mistaken for
a rejected edit. The test rejects empty, text, nonfinite, overflowing and
negative values, plus zero for the two strictly positive fields. It accepts
small positive values and zero flattening in accordance with
``doc/help/src/Resource_CelestialBody.xml``.

The same cases verify pending-state isolation, clean OK, changed OK, Cancel,
exact source/model Undo and Redo, repeated Apply without extra history, and
save/reopen. Claims cover GUI model availability and these physical settings;
they do not establish independently correct ephemerides or force models,
complete input domains, or the unexercised body tabs. The historical typo
``Sarturn`` is explicitly matched to Saturn, while ``EarthMoon`` uses a created
Moon with its verified default Earth central body. MercuryMoon and VenusMoon
are not inferred from it.

The new source-bound unattended proof and cumulative receipt are
``build/gui-acceptance/20261027-body-physical-headless/`` and
``build/nasa-coverage/20261027-cumulative-body-physical/``. A focused minimized
visible replay is queued after the existing command/resource replays. The
original native texture/model assertions remain and will execute on that route;
the earlier unattended body result did not execute those native-only branches.

Traceability claims can now carry their own hashed ``reference_sources``. A
new body-document dependency is required for the body proof without invalidating
unrelated historical receipts that did not use that document. Missing or
changed claim dependencies fail verification; dedicated negative tests cover
both cases.

Solar-system control expansion
------------------------------

The existing ``QtGui.SolarSystem`` test retains every original source line.
Its DE405, DE421, DE424 and SPICE scenarios now compare the six-component
state of all eleven default bodies against a separately authored script
configuration at the report epoch. Existing thirteen-column mission results,
frame checks, exact source Undo/Redo and file persistence assertions remain.
This demonstrates GUI/script route equivalence using the same runtime; it is
not independent scientific ephemeris validation.

The extension also checks both TT checkbox states with zero, 1e-12 and 2.5
second update intervals; invalid, empty, nonfinite and overflowing intervals
retain correction input and leave source, history and model unchanged. SPK
and PCK Browse buttons now receive explicit Cancel and accept actions using
Unicode fixture paths. The focused unattended receipt is
``build/gui-acceptance/20261027-solar-controls-headless/``. Its minimized visible
replay follows the queued celestial-body replay. The cumulative source-bound
receipt is ``build/nasa-coverage/20261027-cumulative-solar-controls/``.

``SOLAR-LSK-EDITABILITY`` records a concrete source conflict: FRR-16.9.3 and
the ``LSKFilename`` field in ``Resource_SolarSystem.xml`` call for GUI/script
editing, while ``SolarSystem.cpp`` marks the parameter deprecated/read-only
and ignores setter writes. The Qt panel omits an editable field. This row is
reviewed but unresolved and receives no coverage credit; a supported intended
behavior must be established before claiming this requirement is met.

Observed-action gap inventory
------------------------------

``build/nasa-coverage/20261027-action-census/action-census.json`` combines
named-object observations from the latest source-matching passing result for
each of 247 cases. It finds 624 named action/class groups, of which 260 have
no recorded activation. These are review candidates, not a widget-coverage
percentage: names may recur in different panels, event traces aggregate by
name, and unnamed or unconstructed widgets are missing. Activation alone
also does not prove a correct result.

This inventory identifies alternate resource-tree Add routes and specific
file-picker, playback, selection and appearance controls for subsequent tests.
The existing generic creation-dialog path does not establish that each
resource-tree Add action selects the correct type or cancels without mutation.

Resource-tree Add routes
------------------------

``QtGui.ResourceCreationDefaults`` now also selects every enabled Add action
from all current resource-tree categories. All 83 actions in this runtime
passed: menu label, internal name and target type agree; the popup is retired
before the creator opens; the creator preselects that exact type; Cancel
preserves configured object identity/content, script bytes, dirty state and
Undo history. Every original creation/default assertion is retained.

The frozen unattended result is
``build/gui-acceptance/20261027-resource-menu-routes-headless/``. Its focused
minimized visible replay is queued after the solar-system replay. Visible
selection and cancellation have a short viewing delay for each route. This
proves alternate routing and cancellation, not complete creation or all fields
of every offered resource type, and adds no NASA numerator by itself.

The refreshed diagnostic inventory is
``build/nasa-coverage/20261027-action-census-resource-menus/action-census.json``.
It contains 630 named action/class groups, with 210 having no recorded
activation. The six newly observed groups and remaining unvisited/unnamed
widgets reinforce why this must not be presented as a whole-GUI percentage.
Filtered runner progress now explicitly labels its count as evidence from
that run receipt, separate from cumulative project coverage.

Shared playback button companion
--------------------------------

``QtGui.SharedPlayback`` retains its original offscreen guard and assertions.
The separately registered ``QtGui.SharedPlaybackButtons`` clicks the five
actual toolbar widgets, checks unequal retained-history frame positions,
starts and pauses the real timer, compares paused/restored images, exercises
speed and mode controls, and checks preserved sample values and clear state.
Its native route is limited to software-rendered X11 acceptance. It does not
replace the original regression or qualify a physical GPU/compositor.

Both cases passed unattended in
``build/gui-acceptance/20261027-shared-playback-buttons-headless-2/``. Only the
new button case is queued for minimized visible replay. The earlier failed
companion receipt is retained: its Latest assertion incorrectly assumed a
numbered last frame; the documented runtime representation follows all data
using an unbounded frame sentinel. The corrected test also requires restoration
of the complete rendered history image, without relaxing any existing test.

The live catalog now contains 248 cases. Queued focused audit tools derive
unselected counts from their frozen catalogs, so adding cases does not make
a passing older snapshot look failed. The full visible baseline still runs
its original 246-case snapshot.

The latest action inventory is
``build/nasa-coverage/20261027-raw-action-census/action-census.json``. It reads
raw trace events to distinguish a QAction activation from a click of a
QToolButton sharing the same name; the older summarized-name inventories could
not attribute those events to the exact class. It has 630 observed named
class/action groups, 205 without recorded activation, and no malformed trace
records. These remain bounded diagnostic candidates, not a whole-GUI percent.

Completed baseline and frozen focused replay
-------------------------------------------

The final shared-production baseline completed visibly in
``build/gui-acceptance/20261027-panel-suite-visible/``: **246/246 passed**,
including all eleven original failing cases. Its immutable snapshot, individual
case identities/logs/JUnit, 2,152 unique PNGs and source-bound NASA proofs were
audited. All 192 cases assigned a visible route had native exposure. The
remaining cases retain their headless/support classification; these are not
claimed as native GUI tests.

The initial focused replay queue encountered a live-source snapshot/build race
and stopped before executing any case. Its partial output is retained. The
replacement is a single immutable replay in
``build/gui-acceptance/20261027-expanded-visible/`` containing the 62 changed
or newly added command, resource, body, solar-system, Add-menu, playback-button
and XY-style cases. All 62 have prior source/input/binary-matched unattended
passes, recorded in ``build/nasa-coverage/expanded-visible/preflight.json``.
The new desktop starts minimized and remains resizable. Its run is in progress;
these paragraphs do not claim its completion.

The XY appearance extension preserves every original plot-test assertion and
uses actual Visible/Lines/Markers/Error bars widgets and the QColorDialog,
including picker Cancel, picker acceptance and parent-dialog Cancel. It checks
selected-curve rendering and unchanged sibling/numerical samples. Its color
oracle inspects the graph interior so subpixel-colored title/tick glyphs are
not mistaken for residual curve pixels; no prior pixel assertion is weakened.
The final normal/HiDPI unattended pair passed in
``build/gui-acceptance/20261027-xy-style-controls-headless-final/``.

Source reconciliation and dynamics review
----------------------------------------

The public NASA repository's current ``main`` branch carries the same Git
blobs as this checkout for the requirements workbook, the May 2012 GUI matrix
and ``FRR-13DynamicsModelTestProceduresNew.doc``. The workbook metadata remains
May 2012; the newer dynamics procedure was last saved in June 2012. This
confirms public-source identity, not that the historical workbook is NASA's
complete current internal baseline. The guide's SharePoint endpoint returned
HTTP 401 here, and its older SVN hostname did not resolve. The current internal
corpus therefore remains unavailable; the user does not have a known local
copy. Public source references:

* https://github.com/nasa/GMAT/tree/main/doc/SystemDocs/Requirements
* https://github.com/nasa/GMAT/tree/main/doc/SystemDocs/Test
* https://gmat.atlassian.net/wiki/spaces/GW/pages/380273297/GUI%2BTesting

All 39 historical Dynamics Models operations are now explicitly reviewed as
GUI-related but unverified. ``source-audit/dynamics-review.json`` under
``build/nasa-coverage/`` records the required evidence for each row. This
review adds no passing scopes. It also preserves the procedure's 225
trajectory/model matrix cells and 42 detailed scenarios. Old initials, Done,
X and N/A values are retained as historical data, never current execution
results. Both shipped versions leave their Unique GUI Tests section empty.

The dynamics procedure calls for comparison of numerical state histories
against an independent tool, using a shared integrator where possible; it
assigns ballistic/mass, integrator and multiple-spacecraft stopping behavior
to other requirement groups. Our current GUI/property proofs and same-engine
script comparisons do not fill that independent-truth requirement. The newer
procedure also removes an earlier proposed empty-force-list error test and
changes a Jacchia-Roberts altitude scenario. These versions must be reconciled
before turning such wording into new assertions.

The official GUI Testing page also describes a 185-item system suite, linked
to a separate System RTTM and test projects. Its detailed inventory and
truth files were not retrieved here; overlap with the shipped historical
requirements/operations is not quantified. Neither the 248 registered Qt
cases nor the public-baseline percentages imply coverage of that entire
NASA system suite.

Integrator controls and dependent validation
--------------------------------------------

``QtGui.PropSetupApply`` now retains every original test assertion and adds
all seven numerical methods: RungeKutta89, RungeKutta68, RungeKutta56,
PrinceDormand 45, PrinceDormand78, AdamsBashforthMoulton and BulirschStoer.
The controls are tested for pending isolation, explicit invalid inputs, exact
Undo/Redo, save/reopen and owned/runtime method identity. Each method runs a
600-second Earth point-mass circular orbit against a closed-form six-component
state, with position tolerance 2e-5 km and velocity tolerance 2e-8 km/s.

The first diagnostic exposed accepted crossed step limits and invalid
predictor-corrector error bounds. The shared preview validation now requires
MinStep <= MaxStep and 0 < LowerError < TargetError < Accuracy. Paired step
limits apply atomically so valid downward changes are accepted. All seven
focused diagnostic cases pass. The full current-build unattended snapshot
passed and was audited, so the 17 new requirement scopes and four historical
operation scopes are included above. Full visible verification passed and
was audited against the same preserved project inputs.

The numerical evidence is one simple analytic problem. General trajectories,
long-duration stability, convergence order, all error-control modes, reduction
coefficient boundary domains and forced unattainable-accuracy behavior remain
unverified. MinStep zero follows the current engine and the guide's explicit
recommendation, despite the guide field table's stricter wording.

FRR-14.7.3 is a further explicit source conflict: the workbook requires a
separate minimum error tolerance, while current BulirschStoer marks
``MinimumTolerance`` deprecated/read-only and its setter has no effect. The
existing BulirschControls test requires that field to be absent. Neither that
test nor ordinary Accuracy is treated as satisfying or retiring the old
requirement. The current public baseline therefore has five recorded source
conflicts; reconciliation remains open.

Error-control modes
-------------------

``QtGui.NasaErrorControl`` is a test-only companion using the existing
acceptance runner. All five ForceModel modes are selected through the named
GUI control, checked for pending isolation, exact Undo/Redo, save/reopen and
mission runtime readback. Every reported state over a 600-second Earth
point-mass circle is compared with its closed-form solution. None additionally
requires 61 samples at exactly 10-second spacing. Unattended and minimized
visible receipts passed; the visible run recorded native exposure and 12
snapshots, including the visible ErrorControl selector.

Five bounded FRR-14.6 scopes are verified. The fixed maximum step makes this
a configuration and trajectory check, not verification that the adaptive
norm formulas differ correctly or meet general convergence requirements.
The application binary was unchanged. The two full 248-case snapshots passed and were audited; this new companion
is the 249th registered check. The collection audit verifies that every shared
runtime file and existing test registration matches across the full runs and
this additive companion. The only shared-file differences are its CMake
registration and the expanded traceability metadata. This is a collection of
verified runs, not one 249-case invocation.

Completed current-build collection
----------------------------------

``build/nasa-coverage/20261027-validation-collection.json`` records all 249
registered checks passing in unattended and visible configurations, with
196 native GUI cases. The application binary still matches the tested build.
The full visible snapshot contains 2,198 valid PNGs and the companion adds 12.
All original eleven failures pass. Test desktops started minimized and
resizable, with no fullscreen state.

This increment adds 22 bounded requirement scopes and four historical
operation scopes to the prior 115/598 estimate. Remaining work includes the
39 reviewed Dynamics Model operations, unreviewed source rows, adaptive
error-norm and unattainable-accuracy branches, broad scientific benchmarks,
0.5x raster qualification, native OS input/portal and hardware/platform/stress
qualification, the five recorded source conflicts and unavailable current
NASA internal test corpus. No full NASA acceptance claim is made.

Accuracy-policy and ForceModel expansion in progress
----------------------------------------------------

The new ForceModel fixture reuses the unchanged resource-lifecycle executable
with a separate JSON input, preserving the other 26 fixtures and their source
bindings. Twenty historical operation scopes are mapped for the known
ErrorControl field, creation, open/close, clone/rename, save/reload and deletion
paths. ChangeAll, all-field validity, focus/window actions and scientific
trajectory comparisons are not inferred from these lifecycle checks.

The error-control companion now adds 70 policy scenarios across all seven
integrators: stop versus warning-only continuation, fixed-step bypass,
loose tolerance, reduction to a minimum step, and forward/backward propagation.
Completed runs must reach the signed stopping epoch, execute the following
mission command and keep sampled step magnitudes within their configured
bounds. The original five analytic trajectory assertions remain unchanged.

These tests exposed actual engine defects: predictor-corrector warning-only
steps were retried until failure; Bulirsch-Stoer returned failure after its
warning; backward predictor-corrector adaptation could restore a forward step;
and a loose Bulirsch-Stoer tolerance could choose fewer extrapolation levels
than its convergence test permits. The fixes accept the explicitly permitted
minimum step, retain propagation direction, enforce the Bulirsch step floor
and ensure enough extrapolation levels to test convergence. All 70 focused
scenarios, the existing propagator/Bulirsch regressions and the full 250-check
unattended snapshot passed. Its receipt was audited; the corresponding full
visible execution was superseded by the terminal-step repair described below.

Terminal-step validation in progress
------------------------------------

The off-grid terminal probe detected a case outside the initial policy set:
1850 seconds was requested, but a minimum-step interaction returned 2100.
The revised suite adds ``QtGui.NasaTerminalSteps`` with 84 targeted scenarios
and retains ``QtGui.NasaErrorControl`` with 77 policy scenarios plus its
original five independent circular-orbit mode checks. The same executable
uses explicit CTest environment settings to select each test path.

One bounded FRC-10.1 scope is now mapped for single-spacecraft forward and
backward propagation. Coupled spacecraft, multiple groups and all scalar
stopping parameters are not inferred. Both focused test paths pass; the
251-case unattended run has passed its full receipt audit; the visible run
is still in progress. These newer bindings have unattended evidence, while
complete current-engine visible verification remains pending. The former 250-case receipts are marked with
``superseded.json`` outside their immutable snapshots.

Four additional ForceModel window scopes are now registered as
``QtGui.NasaForceModelWindow`` and passed in both modes. They check actual
child title-bar minimize/maximize, hidden versus workspace-filling content,
normal-state restoration, correct Help/F1 content and exact applied Show
Script while preserving a pending ErrorControl edit. The visible case recorded
12 valid snapshots. All 251 existing test registrations and runtime binaries
match the earlier frozen build; only test registration/review metadata and the
new test were added. The collection audit recomputes 138 requirement scopes
and 626 historical operation scopes from the retained proof logs. This is a
252-case verified unattended collection, not one 252-case invocation, and the
complete current visible collection remains pending.

ForceModel invalid Apply and application exit
--------------------------------------------

``QtGui.NasaForceModelTransactions`` passed unattended and visibly. It opens
the actual ForceModel table and stages an invalid SRP.Flux together with a valid
ErrorControl change. Rejected Apply must leave both configured values, exact
script/file bytes, pending inputs and Undo history unchanged. Corrected Apply
must commit both values, support exact Undo/Redo and remain unsaved until Save.

The actual File Exit action then exercises Cancel at the pending-panel prompt,
Cancel at the later script-save prompt, Discard without saving, and Save. The
first two preserve the pending panel; Discard leaves the previously saved file
unchanged; Save writes exactly committed settings and never the pending edit.
The mapping follows the separate wx child-close and main-frame save decisions,
not an assumed equivalence of all Yes/No buttons. Historical rows 289, 299 and
300 receive bounded transaction evidence; all-field validation, running-mission
exit and native OS input are not inferred. The visible run retained 12 snapshots.

The 253-check collection is the 251-check baseline plus two independently
executed test additions. All pre-existing test binaries and shared runtime
inputs match across these snapshots. Its current full visible baseline remains
in progress; the new tests have passed both routes.

Configured ForceModel cloning
------------------------------

``QtGui.NasaForceModelClone`` passed unattended and visibly. A configured model
contains Earth gravity, Luna/Sun point masses, NRLMSISE00 drag, SRP and
relativity. Four GUI properties are changed before cloning. The complete
engine-generated configuration and every enumerated property must match after
cloning except for the resource name. Owned force objects must be distinct.

The test changes multiple properties on each copy, verifies the other model
remains identical, deletes each copy in turn, and checks Undo/Redo and Unicode
save/reopen. Five bounded clone-operation scopes (historical rows 304–308)
are verified. These checks do not establish every input combination or the
physical accuracy of the configured forces; those remain explicit gaps.

The first draft used SRP flux 1500 as a nominal edit. NASA documents the range
1200 < Flux < 1450, and the engine correctly rejected it. The corrected nominal
value is 1420; no assertion was removed and no production change was needed.
The failed diagnostic is retained in
``build/nasa-coverage/force-model-clone/failure-review.json``.

The current 254-check unattended collection consists of the frozen 251-check
baseline and three test-only additions. The three additions passed visibly,
with 12 snapshots each. Complete baseline visible verification remains active.

Coupled gravity validation in progress
--------------------------------------

A new regression with 12 GUI cases and one mixed-force transaction demonstrated that Degree 3 / Order 4 was accepted
by Qt. The wx GUI explicitly rejects Order > Degree. The pending-model
validator now rejects that relationship before changing source/model/history,
and accepts valid pairs as a transaction regardless of setter ordering. The
focused regression passes after the fix; a fresh full-build validation remains
required. The older visible run continues against its immutable snapshot.

The ForceModel help table reverses the inequality, saying Degree <= Order;
wx validation and engine diagnostics instead say Order <= Degree. This is now
the sixth recorded source discrepancy (GRAVITY-ORDER-INEQUALITY). The source
help document is preserved unchanged; the test follows the wx GUI contract.
Field-on-file maxima, all other ForceModel fields and broad scientific cases
remain outside this regression's scope.

Historical visible validation completed
---------------------------------------

The pre-gravity-fix 251-case visible baseline finished with all checks passing.
Its audit verified 198 visible-route cases with native exposure, no missing
exposure records, 2,234 valid snapshots and all original 11 failure cases.
Together with the three additive ForceModel tests, the historical 254-case
collection is now verified in both modes. Its receipt explicitly records that
the live application has changed; it does not qualify the newer gravity fix.

The new 255-case unattended run is active. A process-identity-checked queue
will start its minimized, resizable visible counterpart only after both full
prerequisite runs pass and all live inputs match the new preserved snapshot.
The planned atmosphere readback/proof patch remains outside the live source
and adds no coverage credit yet.

Post-fix unattended audit
-------------------------

The complete new 255-case unattended run passed with an unchanged snapshot.
All six focused test logs contain their expected proof sets: error-control
82, terminal-step 84, gravity validation 13, ForceModel windows 4, transactions
5 and cloning 5. The fresh full visible run started only after both prerequisite
runs passed and the live inputs matched the preserved post-fix snapshot.
The atmosphere assertion/proof enhancement is prepared outside live sources;
its 12 reviewed requirement candidates have not received coverage credit.

Atmosphere requirement traceability
------------------------------------

The existing ``QtGui.Atmosphere`` scenarios remain intact. Twelve bounded
requirements now trace the JacchiaRoberts, MSISE90 and NRLMSISE00 selections
and their F107, F107A and geomagnetic-index settings. New assertions read the
actual runtime force owned by the mission propagator. Their density and state
results continue to be compared with independently scripted configurations
using the same engine; independent validation of atmosphere physics is not
inferred. MSISE-86 and the broad parent physics requirements remain unverified.

The changed test passed both routes and retained all 248 original source
lines. The collection audit replaces only its result; it does not repeat the
254 unchanged cases or claim one uninterrupted 255-case invocation.

MSISE-86 capability gap
-----------------------

The current configured runtime registers Exponential, MSISE90, JacchiaRoberts
and NRLMSISE00 atmosphere models. It does not register MSISE86; an actual
ForceModel script selecting MSISE86 fails with an interpreter error. The
capability probe is retained in
``build/nasa-coverage/atmosphere-review/runtime-capabilities.json``.
FRR!E116–E119 are therefore reviewed but unverified, not excluded from the
denominator. A working implementation/plugin and result fixtures are still
needed; current availability is not a retirement of the historical requirement.

The collection's review overlay adds only these four no-credit reviews. It
checks that all previously executed claims and reference hashes remain
identical to the saved run's review. This improves gap reporting without
manufacturing execution evidence or repeating passing tests.

SRP and relativity requirement proofs
--------------------------------------

The existing ``QtGui.ForceSelectors`` test now checks the actual mission-owned
force model after every trajectory comparison. All four SRP/relativity On/Off
combinations must be observed; enabled SRP must use the Spherical model. Its
130 original source lines remain, including mixed invalid Apply recovery,
source/history checks and six-state comparisons with scripted configurations.
The strengthened test passed both routes, retaining 12 visible snapshots.

Three bounded scopes (FRR!E132, FRR!E133 and FRR!E140) are now verified. They
cover the current GUI/runtime path for spherical SRP and relativistic
correction, not independent equation validation or every orbit/body domain.
Together with the atmosphere replacement, the current collection reuses 253
unchanged results. It remains pending complete current visible validation.

Named gravity bodies and runtime force selection
------------------------------------------------

``QtGui.GravityBodies`` retains all 122 original source lines and assertions.
The added scenarios open the visible PrimaryBodies/PointMasses list chooser,
check that Cancel preserves the previous selection, select the requested
bodies, and check pending isolation, Apply, exact source Undo/Redo and Unicode
save/reopen. They compare six propagated state components after 600 seconds
with a separately scripted configuration, then inspect the actual
propagator-owned ODEModel: one Earth GravityField at degree/order 4/4, or the
exact selected PointMassForce body set with no leftover harmonic model.

FRR!E105 has bounded default-Earth selection evidence. FRR!E135 requires all
eight major planets to pass, and FRR!E136 requires Pluto. Earth remains the
central attracting point mass in the planet cases. FRR!E137 adds a configured
user-defined Ceres asteroid with the shipped SPICE ephemeris; the chooser
selects it and the mission uses exactly Earth and Ceres point-mass forces.
Its definition must survive the complete edit/history/save workflow.
FRR!E106 (user-defined harmonic gravity), the broad parent requirements, all
coefficient files/degrees/orders and independent gravity/ephemeris accuracy
remain open. Other user-body types and central-body combinations are unverified.
Widget-level selection is not OS input qualification.

Both final focused routes passed; the visible route recorded 69 native exposure
events and 120 PNGs. Its trace contains all 22 body-selection dialogs and 39
captures of exposed body dialogs. Evidence is in
``build/gui-acceptance/20261027-gravity-user-body-{headless,visible}/`` and
``build/nasa-coverage/gravity-user-body-review/visual-dialog-audit.json``.
The same-runtime collection audit substitutes these three strengthened tests
and verifies the other 252 results against the preserved full unattended
baseline. Its visible collection remains pending the full visible baseline.

ForceModel dialog exit
----------------------

The existing ``QtGui.NasaForceModelWindow`` test retains all original source
lines and adds the current MDI exit behavior for H2012:R291. Actual title-bar
mouse events open the unapplied-change prompt. Cancel must retain the same
window, pending choice, applied model, source and undo history. Discard must
close it without committing. The actual OK control must apply and close, and
Undo/Redo must restore the exact source and model values.

Both final focused runs passed, with 15 native exposure events and 33 PNGs in
the visible run. The first preflight timed out because the new test callback
checked for the prompt during the title-button press delay, before release
opened it. The corrected test waits for the actual modal, preserves every
assertion, and passes. No production code changed. The failed initial log and
corrected receipts are retained in ``build/nasa-coverage/force-dialog-exit/``;
the snapshots are ``build/gui-acceptance/20261027-force-dialog-exit-{headless,visible}/``.

The dialog-exit-stage collection substitutes four strengthened tests and
reuses 251 unchanged passes. It contains 157 verified requirement scopes and
635 historical-operation scopes; its full visible collection remains pending
the active full visible baseline. Remaining reviewed Dynamics Model gaps are
Tab and the five broad input/validation operations; closing one editor with
one field does not qualify those domains or the archived TestComplete sequence.

ForceModel keyboard traversal
------------------------------

``QtGui.NasaForceModelWindow`` now sends Tab and Shift+Tab key events to the
currently focused widget. The default primary form must reach all named
body/force/error-control inputs and actions; reverse traversal must invert
its observed order. Advanced / All Properties must visit all 24 visible,
enabled grid cells in order and return in reverse without entering hidden
rows or columns. Source, model and pending values must remain unchanged.
Visible playback paces the key events so the user can follow focus movement.

The test checks 17 primary-form focus targets on this fixture. Native style
button ordering can differ from offscreen ordering; reversibility is verified
within each run, without imposing one platform's button order on the other.
This is a bounded H2012:R298 proof for the default ForceModel layout. Other
force-family layouts, the exact archived TestComplete sequence and OS-generated
keyboard input remain unqualified. The initial new test incorrectly assumed
CentralBody was a combo box; the actual named input is now located independent
of widget subclass. All original assertions and exit scenarios remain.

Both focused routes passed and the collection audit verified 157 requirement
scopes and 636 historical-operation scopes. The final visible route took
19.361 seconds, with 15 native exposure events and 33 PNGs. Snapshots are in
``build/gui-acceptance/20261027-force-tab-{headless,visible}/``; the collection
audit is in ``build/nasa-coverage/force-tab-traversal/validation-collection.json``.
The four strengthened tests replace four baseline results; 251 unchanged
passes are reused. The five remaining reviewed Dynamics Model gaps are the
broad Apply/control/valid/invalid/name-input validation rows.

Additional tide validation and remaining UI gap
------------------------------------------------

The tide-file diagnostic and editor cases are recorded in
``build/nasa-coverage/tide-review/``. Current source work adds rejection of
invalid file/content inputs and safe handling of a directory. The preceding
partial full-run receipt is retained at
``build/gui-acceptance/20261027-gravity-domain-suite-headless/`` and is not a
complete passing qualification of this newer parser.

A separate GUI gap remains: the guide describes a dependent tide-data-source
selector (None, Inherited, Tide File) and an enabled/disabled tide-model list.
The current editor exposes TideFile and a text TideModel field. The new tests
verify that field path; they do not credit the missing dependent-selector
mechanism, every body/model combination, coefficient precedence or independent
tidal-force accuracy. No new historical or requirement percentage is awarded
for these partial field-domain additions alone.

SPK settings and independent interpolation, 2026-10-27
----------------------------------------------------

``QtGui.NasaSpkEpochControls`` checks ten supported epoch representations,
explicit-date conversion at Gregorian millisecond precision, FromSpacecraft
and EphemStart keywords, seven atomic invalid/conversion rejections, correction,
and a format-only public GUI transaction. The SPK resource editor now offers
an editable start-source selector and typed epoch formats. Format changes
convert the date instead of reinterpreting it; invalid pairs leave source,
model and Undo history intact. The engine rejects nonpositive/nonfinite user
StepSize magnitudes while retaining internal signed propagation direction.

Seven separate runtime cases cover Type9/Type13 forward and backward inherited
arcs, EphemStart, and explicit numeric/calendar epochs chosen through the actual
resource editor. The explicit start differs from the spacecraft epoch. Each
case checks three interior epochs and all six state components against a
specified cubic and its exact derivative, using nonuniform synthetic kernel
knots written directly with CSPICE. The report is EarthICRF to avoid confusing
interpolation with GMAT's ICRF/FK5 rotation. Reported TDB time and the cumulative
spacecraft clock are checked separately; per-command elapsed stops are
relative intervals. The following command must execute too.

The fixture follows NAIF's primary documentation:
https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/cspice/spkw09_c.html and
https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/cspice/spkw13_c.html .
A separate Python audit parses the binary DAF segment summary and supplied
states directly, then checks raw reported positions/velocities using decimal
Horner evaluation. It verifies snapshot, receipt, JUnit, log and PNG integrity.
The original ``EphemerisTests.cpp`` assertions are unchanged and were not rerun.

Eight new cases passed unattended across preserved development versions.
Five passing cases were retained while the three unfinished/new start cases
were completed; no combined current-build unattended pass is fabricated.
All eight passed on their first visible route, with 109 valid PNGs, in a
resizable desktop starting minimized. One subsequent case qualifies only the
additional report-workspace fit and right-column scrolling assertions; its
12 PNGs retain both sides of the result. It adds no requirement credit.

The preserved visible batch qualifies bounded ``FRR!E301``, ``E302``, ``E304``,
``E307`` and ``E308`` scopes. Parent groupings ``E300``, ``E303``, ``E306`` and
Origin ``E305`` remain reviewed-unverified. No historical operation is newly
credited. The audit and receipts are linked from
``build/nasa-coverage/spk-review/validation-summary.json`` and
``build/gui-acceptance/20261027-spk-visible/report.md``.

Open SPK work includes pending numeric-to-SPK creation/type-switch controls,
actual spacecraft kernel picker/replacement/cache behavior, mixed propagation
groups, STM/A-matrix rejection and other segment families. CentralBody/Origin
is documented and implemented as inactive; this suite does not invent a
working origin setting. The guide's numeric StartEpoch default also differs
from the runtime EphemStart default and needs authoritative reconciliation.
General time/frame science, current private NASA corpus completeness, native
OS input, other platforms, hardware and stress remain unqualified.

Pending propagator type settings, 2026-10-27
-------------------------------------------

The next increment closes the pending numeric-to-SPK settings gap described
above. ``PropagatorSettings`` builds the active controls from a detached
registered factory prototype, so selecting SPK immediately exposes StepSize,
EpochFormat and editable StartEpoch. Numerical family changes similarly expose
predictor-corrector or extrapolator-specific controls. Per-type pending values
survive switching away and back, and compatible numeric settings carry across
new numerical families. Inactive fields do not enter Apply or dirty detection.
The original hidden table remains synchronized for existing consumers.

Three independently selectable cases pass on identical frozen inputs in both
modes: ``QtGui.NasaSpkDraftCreate``, ``QtGui.NasaSpkDraftEdit`` and
``QtGui.NasaSpkDraftFamilies``. They exercise actual creator/editor widgets,
per-type retention, compatible-field carryover, explicit epoch conversion,
Cancel without resource registration, invalid Create correction, inactive
numeric-field omission, both Apply directions, the actual force-model picker
Cancel/OK, exact existing-object Undo/Redo and Unicode new-object save/reopen.
The SPK CentralBody field is read-only with an explanatory tooltip, and the
public GUI transaction rejects attempts to change its unsupported Origin.
No working Origin transformation is inferred.

The three visible routes start minimized in a resizable desktop and retain
46 valid PNGs. The original ``NasaSpkTests.cpp``, ``PropSetupApplyTests.cpp`` and
``PropSetupCreationTests.cpp`` assertions are byte unchanged; none of the
preceding 319 checks was selected. Raw final sources, receipts, JUnit/log,
source bindings and exact prototype/model readback assertions are audited in
``build/nasa-coverage/spk-draft-review/validation-summary.json``. The audit also
checks that the serialized active settings match independently specified
values and omit inactive settings.

``FRR!E300`` receives bounded type-selection/configuration credit; it has only
SPK as a listed historical child type. No historical operation is newly
credited. This gives 202 distinct requirement scopes (17.4%) and 637 historical
operation scopes (26.9%) across preserved builds. This new frontend has not yet
received the final full regression/scientific replay. Next work remains actual
spacecraft SPK kernel selection/replacement/cache behavior, mixed groups,
STM/A-matrix rejection, other segment families, Origin reconciliation and the
previously stated science/native/platform/stress/provenance limits.

Spacecraft SPK kernel selection and cache, 2026-10-27
----------------------------------------------------

Four new cases expand the actual Advanced/SPICE kernel-list and existing-file
picker paths. ``QtGui.NasaSpkKernelSwap`` checks Unicode file selection,
duplicate suppression, kernel-list and spacecraft-panel Cancel isolation,
pending Apply, same-ID A/B/A file replacement, exact Undo/Redo and Unicode
save/reopen. ``QtGui.NasaSpkKernelPriority`` checks configured [A,B], [B,A] and
[A,B] order against the last-loaded overlapping-file precedence rule in NAIF's
SPK Required Reading:
https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/req/spk.html .
``QtGui.NasaSpkKernelInvalid`` checks malformed-file Apply atomicity, empty-list
and missing-NAIF runtime diagnostics, and corrected-ID recovery.
``QtGui.NasaSpkKernelPickerCancel`` selects both valid and malformed files and
presses Cancel in the inner file picker, then checks that outer-list OK still
leaves the source, model, Undo history and pending panel unchanged.

Two synthetic Type9 kernels share NAIF ID -100078 and Earth-relative ICRF
coverage, with deliberately distinct affine positions and velocities. Seven
successful mission phases in each mode check all six state components and
both clocks against independent analytic values; two additional runtime phases
must fail with the expected diagnostics. A separate decimal audit parses the
binary DAF metadata and supplied fixture states directly, checks every raw
report against the intended selected file, and verifies frozen inputs,
receipts, JUnit/log hashes and PNGs. The final report viewer must show exactly
the last checked data, with both sides of the horizontal viewport captured.

The first three unattended passes were retained. Only the subsequently added
inner-file-picker Cancel case ran next. All four new cases passed their first
visible route, recording 226 valid PNGs in a resizable desktop starting
minimized. The earlier unattended source inventories did not include the newly
cited SpiceInterface.cpp; its unchanged before-run copy and visible snapshot
retain the source, and all production binaries match across the three
snapshots. No archived receipt was modified to insert that file. Later test
edits add the inner Cancel branch and report-fidelity assertion; they retain
the original state/clock assertions. Exact report-view fidelity is directly
qualified in the visible snapshot. No production fix was required, and the
preceding 322 checks were not selected.

This strengthens ``FRR!E302`` with new file-selection/cache/error facets and
adds no requirement or historical-operation row credit. The estimate remains
202/1,158 requirement scopes (17.4%) and 637/2,367 historical operation scopes
(26.9%) across preserved builds. In particular, Solar System/user-body kernel
list ``FRR!E239`` is a different scope and is not credited from spacecraft
checks. The evidence is in
``build/nasa-coverage/spk-kernel-review/validation-summary.json``.

Order is changed through the actual QListWidget model, so native drag/drop
remains unqualified. Qt file-picker widget automation is not OS-input or native
portal qualification. Mixed SPK/numeric groups, STM/A-matrix rejection,
configurable Origin, other segment families, general frame/time science,
current NASA corpus completeness and platform/hardware/stress remain open.

Mixed propagation groups and configured boundaries, 2026-10-27
--------------------------------------------------------------

Fifteen new ``QtGui.NasaSpkGroups*`` cases cover synchronized/independent,
numeric/ephemeris leader, forward/backward single steps; repeated configured
SPK instances; two distinct coupled numeric groups; and unsupported
STM/A-matrix/covariance, SPK cardinality, formation, empty-formation and
formation/member overlap inputs. A new case reproduced the assignment dialog
accepting STM with an SPK group. Optional configured-mission validation now
checks the group dialog and raw pending-command Apply, leaving the core
propagation algorithms and standalone syntax-only editor defaults intact.
Invalid options retain their pending values for correction and disable OK.

Each numeric group contains two spacecraft in a single 12-state propagation
vector. The two-group cases use independent 31-second and 23-second numeric
steps; ephemeris groups use 17 and 29 seconds. Synchronized cases use the first
group's step for all groups. Numeric and ephemeris leaders and both directions
are checked. Numeric states use independently specified circular point-mass
orbits at different initial quarter turns; one Type9 fixture contains distinct
NAIF IDs and affine six-state trajectories for the two SPK groups. Separate
SPK clone/state-manager identities and six-state dimensions are checked even
when a configured SPK is reused.

The fifteen cases check 384 state components per mode, every spacecraft's
clock, 12 frontend rejection attempts, four unchanged core boundary failures,
and recovery. They also check actual Add/Remove, Cancel/pending Apply, exact
pending/applied Undo/Redo, Unicode save/reopen and report-view fidelity.
Independent high-precision Taylor circle and decimal affine calculations,
direct DAF metadata/state parsing, raw reports, receipts, JUnit/log hashes and
PNGs are audited. The fixed state tolerances remain 1e-6 km and 1e-9 km/s;
the independent maximum position discrepancy is below 1.3e-7 km.

Twelve unattended passes were retained, including two reused by the runner's
matching snapshot/profile resume. A failing test action that removed a row
directly was corrected to click the actual Remove button, preserving its
assertion. Only that case and the two subsequently added numeric-group cases
ran next. All fifteen passed their first visible route, producing 421 valid
PNGs in a resizable desktop starting minimized. The previous state-transition,
covariance and workflow assertions remain byte unchanged, and none of the
preceding 326 checks was selected. The new frontend has not received the final
full regression qualification.

Post-run mapping review qualifies ``FRC!E112``, ``E115``, ``E116`` and ``E117``
against exact assertion/production/document sources already frozen in the
visible snapshot. ``FRC!E111`` had earlier single-spacecraft credit and is
strengthened without duplicate credit. The original receipt, snapshot and
run-time coverage report are unchanged; the separately hashed reviewed mapping
and coverage report are in ``build/nasa-coverage/spk-groups-review/``. Two
additional background sources were collected only for documentary review and
are explicitly excluded from executable claim references. No GUI replay was
performed merely to update mappings.

``FRC!E113`` unlimited numeric-group capacity remains reviewed-unverified.
``FRC!E114`` historical one-and-only-one ephemeris wording also remains
unverified pending reconciliation of configured-resource versus per-command
clone interpretation against the current guide and implementation. These do
not receive credit from finite examples. The estimate is now 206/1,158 scopes
(17.8%), with historical operations still 637/2,367 (26.9%). The audit is
``build/nasa-coverage/spk-groups-review/validation-summary.json``.

Remaining work includes arbitrary coupled forces and multi-step/stop domains,
capacity/stress, ephemeris-count interpretation, inactive Origin, user-body
SPICE kernels, other segment/frame/time science, native OS input, platform and
hardware qualification, and current NASA corpus completeness.

The mixed-mode cases above exercise the modifier order generated by the group
editor. The alternate synchronization/backward keyword order shown elsewhere
in the Propagate guide still needs a dedicated parser/preservation/raw-Apply
case; its input domain is not inferred from the eight mode combinations.

Mission Assignment source types and array-cell shape validation (2026-10-28)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``NasaAssignmentCases.json`` adds five cases to the existing Assignment harness:
``ScalarCell``, ``CellVariable``, ``CellArray``, ``CellExpression`` and
``CellProperty``. They use the actual Mission-tree editor, check pending edits,
Cancel, invalid Apply/OK atomicity, exact source/Undo/Redo, Unicode reopen and a
single raw numerical report with an unchanged sentinel. Every array-cell report
includes all four cells. No existing case or C++ assertion was changed.

The historical requirements FRR!E411--E416 and FRR!E418--E424 distinguish scalar
and array-element sources. The primary Array/Variable guides place expression
assignments in the Mission Sequence. The Variable resource initializer remains
literal-only. Both five-type headings require all five constituent proofs;
equal-dimension whole-array assignment reuses the retained ``ArrayCopy`` result.

New coverage found a frontend validation defect: ``GetConfiguredObject`` resolves
an indexed destination to its parent Array, so the whole-array dimension check
incorrectly rejected scalar values for a cell. ``CommandForm.cpp`` now requires
a 1x1 expression result for indexed destinations, retaining the configured array
dimensions for an unindexed destination. The added matrix-to-cell negative case
also showed why skipping shape validation entirely is insufficient: interpretation
accepts the draft before mission initialization. The final repair retains that
rejection. This patch changes GUI validation only; no engine/math source changes
are included. The incremental patch is recorded in
``build/nasa-coverage/array-source-assignment-review/CommandForm.increment.patch``.

Only failed new cases are replayed. Successful new and retained historical
outcomes keep their original frozen source, recipe and production references;
they do not establish a single pass of the current build. Detailed claims,
independent raw-value audits, failed attempts and exact input rebindings are in
``build/nasa-coverage/array-source-assignment-review/``. Runs start minimized in a
resizable desktop, use no observer screenshots, and retain logs/JUnit/raw outputs.
Inputs use in-process widget edits/events and frontend calls, not proven OS input.
Arbitrary input domains, general science, platforms, GPU/stress, private current
NASA provenance and final full-build visible/unattended qualification remain open.
The separate strict VF13ad report-final failure and existing source conflicts,
viewing gaps and core-change scrutiny are unaffected.

The audited cumulative ledger now demonstrates 837/1158 requirement scopes
(72.3%) and 903/2367 historical operation scopes (38.1%). Source review is
945/1158 requirements (81.6%) and 920/2367 historical operations (38.9%). The
increment contains five distinct new passing scenarios, 17 new rejection checks,
22 new raw numerical values and 104/104 paced viewing boundaries; six retained
native outcomes contribute 18 independently rechecked values. Four strict failed
attempts were preserved while repairing the two affected cases. All 108 prior
collection records, 125 assertion source/hash pairs and 41 tracked core hashes
remain unchanged. The registered suite now has 884 checks; this is versioned
bounded coverage, not current-build suite qualification.

OrbitView optional displays and body-fixed Grid (2026-10-28)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The primary OrbitView guide defines ``Grid`` as longitude/latitude lines on the
celestial bodies in the plot. The previous Qt implementation instead drew a
screen-aligned Cartesian background grid. New ``NativeNasaOrbitGuideGrid`` input
and framebuffer checks reproduced that defect: 47,074 changed pixels were outside
the body and no surface pixels darkened. The native and raster rendering paths
now use the existing body-fixed spherical guide geometry for displayed celestial
bodies. Independent per-object OpenFrames guides retain their selection; XY-Plot
and GroundTrack grids keep their prior behavior. This is a frontend rendering
change, without engine/dynamics source changes. Review its three-file delta in
``build/nasa-coverage/orbit-display-guides-review/frontend-grid.increment.patch``.

Two new cases exercise the actual viewer Display dialog. ``Axes`` checks three
colored coordinate directions. ``Grid`` checks the original sphere pose, a
27-degree body rotation and a translated body, localizes changed pixels to the
body silhouette with a three-logical-pixel line-edge band, and rejects the
preserved Cartesian-background counterexample. Both cases verify unchanged raw
position/epoch/rotation samples and exact framebuffer restoration after Off.
The first harness run clicked noninteractive checkbox whitespace; the harness now
clicks the indicator. Its original bright-body mask was also corrected to include
the shaded silhouette and line edges. These changes preserve the original legacy
assertions and the new locality/result/restoration assertions. Failed attempts
and direct visual oracles remain separate from passing receipts.

Existing successful ``OrbitSetup`` and ``NativeOrbit`` results supply the grouped
configuration and XY-plane/wireframe/star/constellation pixel assertions without
replay. Every child configuration and rendering claim is required for the closed
six-option heading FRR!E321--E327. The exact existing native renderer completion
line is now supported by the source-bound legacy reviewer; negative checks reject
changed, quoted, duplicate or missing lines. Execution logs are never rewritten.

Run these new tests through ``RunAcceptance.py`` with
``--filter '^QtGui\.NativeNasaOrbitGuide(Axes|Grid)$'``. Add ``--visible
--start-minimized --max-screenshots 0`` to watch them in the resizable nested
desktop. Their CTest registrations use the existing native software-X11 launcher
for unattended runs. Direct comparison images are test oracles and are retained;
automatic observer frames are disabled. The increment records visible passes,
not a new complete unattended/current-build qualification.

The raster pixel path, multiple/custom/flattened bodies, general frame/science,
OS input and host GPU/compositor/platform behavior remain unqualified. The guide's
separate rule disabling the constellation field when stars are Off is explicitly
held as a source-only candidate in
``build/nasa-coverage/orbit-display-guides-review/remaining-direct-behavior-gaps.json``.
The existing VF13ad final-text failure, source conflicts, previous viewing gaps,
private current NASA corpus and core-change scrutiny remain open.

The audited cumulative ledger now demonstrates 844/1158 requirement scopes
(72.9%) and 903/2367 historical operation scopes (38.1%). Source review reaches
952/1158 requirements (82.2%) and 920/2367 historical operations (38.9%). The
increment independently recomputes 135,590 changed pixels across four passing
image phases, with 30/30 paced viewing boundaries. Two existing component
outcomes are reused. One rebuilt, directly affected ``QtGui.ObjectGuides``
unattended regression passes for shared geometry and per-object fallback
compatibility; it adds no new coverage credit. No unrelated known pass is
replayed. Eight passing direct-image oracles and preserved failure images occupy
5,594,985 bytes in total; no automatic observer frames are requested.

All 109 prior audit/report/receipt/mapping identities, 125 existing assertion
source/hash pairs and 41 tracked core hashes remain unchanged. One derived
``reviewed_production_reference_bindings_match_current`` flag for the historical
``native-selected-region-xy-zoom`` collection becomes false after the
``PlotWidget.cpp`` edit. Its historical pass is retained; current-build
qualification must be renewed in the final integration batch. The suite now has
886 registered checks. All figures are bounded, versioned scopes, not a claim
that the complete current-build suite or NASA's current private corpus passes.

Stars dependency, raster and unattended qualification (2026-10-28)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The OrbitView guide and original wx ``OrbitViewPanel.cpp`` agree: Stars Off
disables constellation editing and StarCount, while retaining the constellation
selection. New ``NativeNasaOrbitStarControlsResource`` and ``...Viewer`` checks
reproduced editable constellation controls with Stars Off in the embedded primary
resource form and viewer Display dialog. ``OrbitViewDialog.cpp`` and
``PlotWidget.cpp`` now update availability without clearing the value. Reopening
the viewer also resynchronizes availability after an external display-model edit.
The exact GUI-only patch is in
``build/nasa-coverage/orbit-star-raster-review/frontend-star-controls.increment.patch``.
No engine/subscriber or dynamics code changes are included.

The resource test uses actual Resource-tree activation and the current embedded
form, checking checked/unchecked preservation, disabled clicks, StarCount80/81,
pending edits, Cancel, exact committed source/native values, Undo/Redo, Unicode
reopen and raw numerical sentinel9. The first harness attempt encountered the
hidden legacy launcher; it was replaced with the embedded route. Its initial
source oracle also assumed in-place assignment ordering. The final full-source
oracle follows the existing serializer's canonical edited-assignment block and
retains every other character. Both harness failures remain failed receipts.
Original legacy assertions are untouched.

``NasaOrbitGuideRaster`` reuses the existing grid pixel/result assertions through
an explicitly offscreen raster route. It checks the same identity,27-degree
rotation and translated sphere, body-localized pixels, unchanged raw samples and
exact Off restoration. Its silhouette mask explicitly excludes the background
color. It does not claim a native window or hardware/GPU behavior. The shared
native guide harness previously accepted native platforms only with visible mode;
its explicit native platform guard now supports private software-X11 unattended
execution while retaining the valid-renderer and pixel assertions.

Both new stars cases pass visibly in a minimized/resizable test desktop and
unattended on private software X11. The existing native Axes/Grid cases receive
their first unattended qualification. Raster passes offscreen. Two retained
configuration/renderer outcomes supply the unchanged component assertions without
replay. The audit requires the additional stars and raster claims in their
existing requirement scopes and the six-child display heading. Unique coverage
stays844/1158 requirements (72.9%) and903/2367 historical operations (38.1%):
closing a behavior or execution-context gap cannot inflate source-row counts.

``build/nasa-coverage/orbit-star-raster-review/gap-resolution.json`` supersedes the
previous source-only candidate while preserving that old record. The batch has
seven passing new execution contexts across five test names,42 independently
checked state transitions,seven pixel phases and two raw sentinel missions.
Four failed attempts remain preserved. There are889 registered checks.
Unrelated passing contexts are not replayed; automatic observer screenshots stay
off. Direct comparison images remain test oracles. Full-current-build visible
and unattended integration, VF13ad report truth, prior source conflicts/viewing
gaps, core-change review, general science/OS input/GPU/platform/stress and current
private NASA corpus reconciliation remain open.

The independent audit rechecks42 control-state transitions and53,954 changed
pixels across seven image phases. Both raw sentinel reports contain9. All32
explicit viewing boundaries are paced;90 exposed checkpoints are recorded in the
visible cases. The14 retained direct pixel oracles total1,177,982 bytes; no
automatic observer screenshots are saved. All110 previous collection records
are unchanged. The shared guide harness is deliberately extended for raster and
unattended platform selection with its original rendering assertions preserved;
the other126 existing test-source hashes and all41 tracked core hashes remain
unchanged. Seven existing requirement scopes gain mandatory evidence across the
new contexts; the unique requirement/historical row sets do not grow.

Optimize modes, backend selection and two independent controls (2026-10-28)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The existing ``QtGui.SolverModes`` case already exercises all twelve actual
Target/Yukon combinations of Solve/RunInitialGuess and the three exit modes.
Its unchanged visible and unattended results are reused for the Optimize subset:
all six combinations per context, literal solved Alpha2 versus initial Alpha1,
Save/Discard next-invocation guess recurrence, stopped pass1/elapsed40 versus
subsequent elapsed80, typed dropdowns, pending edits, exact source/history and
Unicode reopen. The audit rechecks24 complete terminal records, including12
Optimize records. Temporary full state/geodetic reports were not retained;
scientific truth beyond the original literal control/epoch checks is not inferred.
The old GUI tests are not replayed.

One new ``NativeNasaOptimizeTwoControls`` case fills the missing single-branch
multiple-control scenario. Its actual Optimize picker offers configured VF13ad
and Yukon resources and excludes a DifferentialCorrector, spacecraft and scalar
variable. It checks cancelled selection, accepts ``OtherVF``, and explicitly
updates the matching child references in the existing branch source editor.
This tests selection and commit; it does not add automatic child rebinding.
The second Vary initial value is edited through its typed field, with eight
invalid Apply/OK rejections, Cancel, exact Apply/idempotence/Undo/Redo and Unicode
reopen before one solve per execution context.

The real VF13ad clone must be named OtherVF, register exactly X/Y, and converge
for ``Cost=(X-3)^2+(Y+2)^2`` subject to ``X<=2`` and ``Y>=-1``. Every raw trial
row is checked against the independent quadratic and unchanged sentinel41;
the final CSV and native variables must match the unique analytic constrained
minimum X2,Y-1,Cost2. Visible execution starts minimized in a resizable desktop;
the first unattended execution uses private software X11. Both pass. The first
fixture disabled ShowProgress while requesting the text report it controls;
the fixture now enables that documented output without changing assertions,
and the original failed attempt remains failed. Printed final-text truth is
still covered by the separate unwaived VF13ad report failure.

The increment demonstrates eleven new bounded scopes FRC!E5,E6,E11,E15,E17--E23.
FRC!E7,E8,E14,E52 remain reviewed/unverified: universal internal command support,
the complete referenced Vary contract, conditional backend enumeration and the
historically deferred unsupported-option warning are not proved by finite
examples. FRC!E16 remains external/unverified for the disabled Windows/MATLAB
fmincon backend; no Yukon substitution or denominator removal is used.
The new problem is separable and low-dimensional, not general coupled or
high-dimensional solver/science qualification. Runtime type observations, raw
CSV/native values, source bindings and remaining gaps are recorded in
``build/nasa-coverage/optimizer-command-review/``.

A source-only unattended-route candidate in the existing VF13ad/Formation rigs
is separately recorded: their shared predicate admits native platforms only
under visible mode. Those old cases are not replayed, relabeled or skipped in
this increment. The new case uses its own explicit native-platform guard.
Full-current-build qualification, the strict VF13ad final-text failure, source
conflicts, earlier viewing gaps, core-change scrutiny, native OS input, host
GPU/platform/stress and current private NASA corpus reconciliation remain open.

The cumulative audited ledger now demonstrates855/1158 requirement scopes
(73.8%) and903/2367 historical operation scopes (38.1%). Requirement source review
is968/1158 (83.6%), leaving190 unreviewed rows; historical source review remains
920/2367 (38.9%). The new case has two passing execution contexts,80 raw values
across18 trial rows and two final rows,16 rejected edits and54/54 paced visible
boundaries. Its one failed output-switch fixture attempt is preserved. All111
prior collection records,128 existing assertion-source hashes and41 tracked
core hashes are unchanged. This increment changes no application/core code and
saves no screenshots. The suite now registers890 checks. These are bounded,
versioned estimates; full-current-build acceptance remains unqualified.

Native unattended launch qualification (2026-10-28)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The shared widget-platform predicate previously accepted native X11 only when
the execution mode was visible. Two first unattended runs reproduced failures
before reaching the Formation and VF13ad assertions. ``RunNativeTest.py`` now
opts in its native child with ``GMAT_QT_NATIVE_TEST_LAUNCH=1`` while preserving
the actual execution mode. Ordinary widget tests still default to offscreen;
private software X11 remains unattended. Visible execution still requires its
desktop DISPLAY, and the separate visible exposure guard remains intact.

The change affects74 native registrations across14 assertion-source families.
All73 selected cases pass their first unattended contexts, using the existing
assertions for body textures, resource/body/Formation script previews, Formation
membership and propagation, VF13ad controls/creation, integrator controls,
finite-burn selection, resource naming and help routes. The remaining case is
the known strict ``NativeNasaVF13adResourceReportTruth`` failure: its final text
value disagrees with the native/CSV optimum. It is neither rerun unchanged nor
waived. The two pre-fix guard failures remain preserved as failed receipts.

The reusable runner contract now includes real private-X11 positive and
negative predicate/launcher checks. It rejects unmarked native execution and
incorrect marker values, retains the actual unattended mode, rejects unsupported
minimal platform execution and preserves the missing-DISPLAY guard. The
independent probe also rejects fake visible exposure on offscreen even with
the native marker. All31 runner contract tests pass. These support checks do
not count as NASA requirement scopes or visible GUI qualification.

The audit in ``build/nasa-coverage/native-unattended-route-review/`` binds all73
outcomes to unchanged logs/JUnit and frozen helpers/sources. All129 existing
assertion-source hash pairs,41 tracked core hashes,112 collection records and
the source mapping are unchanged. This increment changes only test support,
performs no visible replays, and saves no observer screenshots. Cumulative
coverage remains855/1158 requirements (73.8%) and903/2367 historical operations
(38.1%). Requirement review remains968/1158 and historical review920/2367.
The full current-build visible/unattended suites, strict report failure,
38 source conflicts, four body identities,18 prior viewing gaps, current NASA
corpus provenance, OS input, host GPU, platform, stress and broader scientific
qualification remain unresolved.

Historical spacecraft epoch lifecycle operations (2026-10-28)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Twenty previously unreviewed ``Spacecraft_OrbitEpoch`` operation rows are now
reviewed. Twelve additional bounded scopes are demonstrated. Apply, clean
dialog exit and conversion evidence reuse the ten unchanged epoch-definition
cases in their original visible and unattended contexts:200 directed
conversions,80 native epoch checkpoints and120 raw values are reaudited without
replaying those tests. These are preserved-runtime proofs, not a new-build pass.

Two new lifecycle registrations reuse the existing independent TAI ModJulian
and UTC Gregorian scientific expectations. They check repeated Apply, Cancel
before/after Apply, refusal/discard of a dirty panel close, valid/clean OK,
exact source and Undo/Redo, sibling/orbit isolation, actual File/Open selection
of a Unicode script with clean resource panels open, and refusal/discard of
program exit with an unapplied epoch draft. Five-system mission reports must
match the independent oracle and preserve sentinel9. NASA's wx close source
defines No as retaining the dirty panel and Yes as discarding its draft; the
Qt Cancel/Discard choices are explicitly mapped to those outcomes.

Both new cases pass visible and unattended execution, with76/76 paced native
viewing boundaries,24 new raw values and no observer screenshots. The visible
test desktop starts minimized, remains resizable and uses host-aware scaling.
The first TAI fixture expected excess input decimals to survive canonical
serialization. Its original exact-source failure is preserved; only its input
fixture is changed to canonical16-digit MJD text. All C++ and numerical
assertions and the original high-precision epoch cases remain unchanged. Only
the failed unattended TAI context is rerun; the passing UTC context is reused.

Eight rows remain reviewed/unverified: six epoch ChangeAll/Modify clone
workflows, Apply-button state fidelity and original Name InputText validation.
Clean Apply is observed enabled in Qt, while NASA's wx source toggles Apply
with the dirty flag. Repeated Apply proves idempotence, not historical button
state fidelity. The new cases do not qualify unsaved-document Save-on-exit,
all epoch/leap-second domains or native OS input. No application/core code
changes. All129 preceding assertion-source hash pairs and41 tracked core hashes
are unchanged; both runner/traceability support contracts pass.

The audit in ``build/nasa-coverage/epoch-lifecycle-review/`` preserves exact
frozen input bindings, raw logs/JUnit,15 negative verifier checks and all112
prior collection records. The113-collection ledger demonstrates855/1158
requirements (73.8%) and915/2367 historical operations (38.7%). Historical source
review is940/2367 (39.7%), leaving1427 unreviewed rows; requirement review remains
968/1158 (83.6%), leaving190. The suite registers892 checks. The strict VF13ad
report-value failure,38 source conflicts,four body identities,18 earlier viewing
gaps,current NASA corpus provenance,core technical scrutiny,broader science,
host GPU/platform/stress and complete current-build visible/unattended suites
remain unresolved.

Historical epoch clone branches (2026-10-28)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Six previously reviewed/unverified epoch clone scopes are now demonstrated:
``H2012:R19--R23`` and ``H2012:R27``. Two new registrations reuse the existing
independent epoch fixtures. Modify changes Epoch only before cloning;
ChangeAll changes both primary epoch-panel inputs DateFormat/Epoch. NASA's
wx OrbitPanel source establishes those two controls. The reference page also
lists other time parameters; they are not treated as additional primary inputs
or claimed fully qualified by these clone cases.

The actual context Clone dialog checks typed naming, Cancel/OK, distinct copy
identity, native format/time and exact clone Undo/Redo. Separate branches start
from the same copied state to edit the clone, edit the original, delete the
clone or delete the original. Independent five-system mission reports check
the unchanged peer or surviving object after each operation. Cartesian state,
mass913.25, frame, mission sentinel9, exact source/history and Unicode reopen
remain checked. Temporary reporting commands are removed with an exact Undo;
the resulting configured source/model is restored before the next branch.

Four qualifying contexts pass. The audit checks12 qualifying missions,120 raw
values,74 native epoch snapshots and42/42 paced viewing boundaries. The first
two ChangeAll outcomes passed a narrower linear sequence and are preserved
without historical credit; only that case is rerun after separating branches.
The Modify outcomes are reused: an exact source comparison proves all code
executed by Modify is unchanged and the changed ChangeAll region is skipped.
That retained evidence is not relabeled as execution of the new binary.
Including development qualification there are six case executions,22 missions
and216 raw values. No unaffected established GUI cases are replayed.

Visible desktops start minimized, are resizable and use host-aware scaling;
no observer screenshots are saved. No application/core code changes. All130
preceding assertion-source hash pairs and41 tracked core hashes remain intact.
The runner/traceability contracts pass, and12 negative verifier checks reject
failed results, wrong source hashes and missing checkpoints. Each credited row
links both qualifying receipts/logs/JUnit in the audited report.

``build/nasa-coverage/epoch-clone-review/`` records the input bindings, source
impact review, independent calendar/offset recheck, primary NASA panel review,
raw artifacts and remaining holds. The114-collection ledger preserves all113
prior collection records and demonstrates855/1158 requirement scopes (73.8%)
and921/2367 historical operation scopes (38.9%). Source review counts remain
968/1158 requirements and940/2367 historical rows, leaving190 and1427 unreviewed.
The suite registers894 checks. Epoch Apply-button state fidelity and original
Name InputText validation remain unverified. Full spacecraft-property cloning,
universal epoch/leap-second/precision domains, strict VF13ad final-text truth,
38 conflicts,four body identities,18 earlier viewing gaps,current NASA corpus,
core technical scrutiny,OS input,GPU/platform/stress/scientific breadth and
complete current-build visible/unattended suites remain unresolved.

Shared resource Apply-button state (2026-10-28)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

A new strict epoch assertion first reproduces the clean Apply mismatch:
the old Qt button remains enabled with no pending changes. NASA's
``GmatPanel::Show``, ``EnableUpdate`` and ``OnApply`` initialize clean/disabled,
retain dirty state until successful Apply and leave rejected drafts correctable.
The shared ``ResourceEditor`` now tracks existing control signals after form
synchronization and updates Apply accordingly, with no polling or model writes.
The dirty latch survives reverting a draft until Apply/no-op reset. Embedded
creation/clone forms retain acceptance of unchanged defaults. Only
``ResourceEditor.cpp/.hpp`` application files change; no core files change.

Thirteen new cases reuse the resource/panel fixtures. They check initial clean,
valid/invalid draft, rejected Apply, correction, clean successful Apply,
disabled-button no-op, Cancel after Apply and clean reopening, exact history,
Unicode persistence, native field readback and sentinel9. Epoch also checks the
reverted-draft latch and no-op reset. Both execution modes qualify26 contexts,
266 status/native readbacks,26 sentinel values and250/250 paced visible
boundaries. Visible desktops start minimized, remain resizable and use host
scaling. No observer screenshots are saved.

One original widget failure and three empty-input fixture failures are preserved.
The fixture helper originally sent an empty key string, leaving the selected
Barycenter text unchanged. It now sends Backspace; the equality assertion stays
intact. Twelve passing nonempty unattended contexts are reused after an exact
source-impact check proves their Qt calls/assertions unchanged. Only the failed
Barycenter unattended case is repeated; the13 visible qualifications are new.
Three directly affected old compatibility cases pass: ``NasaLifecycleSpacecraft``,
``ResourceCreationDefaults`` and ``NasaPanelButtons``. No unrelated established
GUI cases are replayed. All131 earlier assertion-source hash pairs and41 core
hashes remain intact; runner/traceability support checks pass.

Nine additional historical rows are credited. Existing numerical-integrator
row ``H2012:R360`` gains a separate state proof while retaining seven earlier
science claims; its aggregate is not requalified or counted as new credit.
The115-collection ledger preserves all114 previous collection records and
demonstrates855/1158 requirements (73.8%) and930/2367 historical scopes (39.3%).
Seven new historical rows are reviewed, bringing source review to947/2367
(40.0%), with1420 unreviewed; requirement review stays968/1158, leaving190.
The suite registers907 checks.

``build/nasa-coverage/apply-state-review/`` contains the GUI-only patch, NASA
contract anchors, exact input bindings, source-impact review, raw states,
negative verifier checks and compatibility receipts. Selected line-edit cases
do not qualify every checkbox/spin/slider/table/dialog signal path, resource,
invalid domain or physical mission effect. Reverted-draft close behavior and
original Name InputText provenance remain unqualified. The strict VF13ad
final-text failure,38 source conflicts,four body identities,18 older viewing
gaps,current NASA corpus,core technical scrutiny,OS input/GPU/platform/stress,
scientific breadth and complete current-build visible/unattended suites remain
unresolved. Other established executables still need the final shared-frontend
rebuild and integration qualification.

Restored-value resource input lifecycle (2026-10-28)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The prior reverted-draft close gap is reproduced before repair: restoring the
epoch leaves Apply enabled and values equal to the saved snapshot, but Close
silently retires the panel. NASA's wx close source uses the dirty-input flag.
``EditablePanel::hasUnappliedInput`` now separates that policy from the existing
value-difference predicate. ResourceEditor includes its dirty latch and honors
discard/success; other panel classes retain their existing value-based policy.
Sixteen MainWindow guards and two document-switch guards use the new query.
No-op OK closes its captured owner after Apply clears dirty input, with no
commit callback or extra history. Five GUI files change; no core files change.

Two new Epoch/Variable cases change a real value and restore it before each
branch. They check panel Close Cancel/Discard, no-op Apply/OK, blocked direct
load/build/run with no mission output, document activation Cancel/no-op Apply,
actual File/Open Cancel/Discard and program Close Cancel/Discard. The alternate
document must activate native V3, then return to the original V2 without changing
its source/history/file. The accepted mission independently checks A1 epoch,
V2 and sentinel9. Both visible and unattended modes pass:24 restored-value
sequences,12 new raw values and70/70 paced viewing boundaries. Visible desktops
start minimized, remain resizable and use host scaling; no observer screenshots.

Four retained ordinary epoch lifecycle outcomes and24 raw values are reaudited
at their exact original input/runtime bindings, without replay. Six historical
rows gain the restored-value domain in addition to their two existing format
proofs. They were already counted, so the numerator does not increase.
``NasaPanelButtons``, ``NasaLifecycleSpacecraft`` and the directly affected epoch
Apply-state case pass unattended with unchanged assertions. The pre-fix failure
is preserved. All132 preceding assertion-source hash pairs and41 core hashes
remain intact; runner/traceability support checks pass.

``build/nasa-coverage/reverted-draft-review/`` contains the GUI patch, source
bindings, raw states/results,15 negative verifier checks and per-row receipts.
The116-collection ledger preserves all115 preceding collection records and
still demonstrates855/1158 requirements (73.8%) and930/2367 historical scopes
(39.3%). Review counts remain968 requirements and947 historical rows, leaving
190 and1420 unreviewed. The suite registers909 checks. These selected resource
line edits do not qualify every changed mutation path, panel class/input kind,
unsaved-document Save decision or wrapper. Original Name InputText provenance,
the strict VF13ad report failure,38 conflicts,four body identities,18 prior
viewing gaps,current NASA corpus,core scrutiny,OS input/GPU/platform/stress,
scientific breadth and complete current-build visible/unattended suites remain
unresolved.
