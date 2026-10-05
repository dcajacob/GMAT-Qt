# Linux Qt replacement qualification: workflows, viewers, plugins and files

Status: in progress. This is the acceptance checklist for requested areas 3, 4,
and 5. Existing milestone tests do not establish full replacement qualification.
Windows/macOS deployment is deferred. MATLAB is outside the selected Linux
runtime; supported plugins below come from its actual startup configuration.

## Current acceptance map — 2026-10-02

[The current nine-gate reconciliation](Qt6ParityValidation/current-acceptance-20261002.md)
maps these requirements to bounded workflow/plugin, full-example and Help
evidence, with remaining ordinary operations and host/external limits explicit.
Historical appendices retain the state at their recorded checkpoint; their old
pending phrases are not the current test queue. The original gates below stay
visible and are not blanket-marked complete by tutorial or private-desktop passes.

## Acceptance gates

- [ ] Audit each wx workflow below against Qt; implement missing user operations
      and record executable evidence. Shared generic controls can serve a workflow
      only when all its operations and validation are available.
- [ ] Array resizing and expression-valued cell editing with cancel, rollback,
      undo and calculation-preserving save/reopen.
- [ ] Resource reference selection, dependent settings, specialized spacecraft,
      force-model, propagation, coordinate-system, hardware and output workflows.
- [ ] Mission commands and branch editing through controls, preserving labels,
      comments, nested commands and unsupported syntax.
- [ ] Plotting option/callback audit, native and fallback correctness, replay,
      camera tracking and output lifecycle checks.
- [ ] OF conversion: perspective/FOV, multiple views, trajectory-relative and
      body-relative camera modes; preserve settings or explicitly diagnose an
      unresolved case. A warning alone does not count as implemented parity.
- [ ] Each selected plugin: configuration, execution, expected numerical/report
      result and error recovery through Qt, with fixture requirements recorded.
- [ ] Script round trips preserve calculations, expressions, unknown settings,
      comments and references; save/reopen, Save As, undo and failed-edit recovery.
- [ ] Rebuild the actual application/bin/GmatQt and pass relevant regression tests.

## wx workflow inventory

Every row begins unaudited; a panel's existence is not proof of Qt equivalence.

| wx source | Qt mapping / missing operations / evidence |
| --- | --- |
| `src/gui/hardware/ThrusterCoefficientDialog.hpp` | Grouped chemical/electric coefficient controls implemented; chemical and electric polynomial finite-burn execution, analytic fuel consumption and configuration round trips covered; broader coefficient regimes pending |
| `src/gui/hardware/BurnThrusterPanel.hpp` | Active thruster and impulsive-burn controls audited. Dependent Local frame/axes/origin, direction, scale, duty cycle and gravitational acceleration; ordered typed thruster tanks/mixtures and single impulsive tank; mass-decrement off, clear/restore and failed-burn recovery covered. Impulsive MJ2000Eq/VNB/LVLH/SpacecraftBody/EarthFixed execution, analytic fuel/frame checks and backward restoration covered. Broader frame/operating combinations pending. |
| `src/gui/hardware/ThrusterConfigPanel.hpp` | Active chemical/electric controls audited. Grouped direction/performance setup, electric model dependencies, all three electric models, minimum-power cutoff, maximum clipping, GUI-attached nuclear power, chemical/electric mixtures, finite burn/coast, numerical report equivalence and round trips covered. Shared power, solar/eclipses, broader polynomial/frame regimes pending. |
| `src/gui/hardware/PowerSystemConfigPanel.hpp` | wx field inventory audited; grouped general/bus/solar/shadow controls and shadow-body picker covered. List reconstruction, invalid-body rollback and Undo tested. Epoch-format conversion, failed conversion recovery and paired Apply/Undo covered; GUI-configured nuclear and unshadowed solar report execution covered; eclipse attenuation and decay cases pending. |
| `src/gui/hardware/TankAndMixDialog.hpp` | Combined tank/ratio editor, first-tank addition, type filtering, reorder with paired ratios, paired Apply, round trips and two-tank chemical/electric burns covered. Mass-decrement off, clear-all save/reopen and failed-burn restore covered. Broader tank combinations remain pending. |
| `src/gui/event/EventLocatorPanel.hpp` | Common and Contact/Eclipse/Intrusion-specific controls audited. Grouped Qt editor provides typed targets/bodies/observers/sensors/shadow types, paired epoch conversion, interval/light-time/report dependencies and input/output pickers. Pending Apply/Cancel, validation/rollback, Undo/Redo/save/reopen, bounded contacts, Transmit/Receive corrections, selected detailed reports, eclipse intervals, shipped Mercury transit and failed-output-directory recovery covered. One actual-input positive FixedGrid configuration/Apply/save-reopen/report passes with a valid owned frame, explicit 2015 noon/300-s coverage and 31 entries/one Sun event; first insufficient-SPK fixture is retained. See [FixedGrid](Qt6ParityValidation/fixed-grid-20261002.md). One actual-input Earth Region/one-spacecraft/no-imager contact passes Regions category paired-table creation, Apply/save-reopen and Output Enter; one 300 s run gives one event lasting 81.477 s bracketed by 31 state samples. See [Region](Qt6ParityValidation/region-contact-20261002.md). Additional formats/coverage boundaries and disk-write failures remain pending. |
| `src/gui/command/TogglePanel.hpp` | Subscriber checklist and On/Off dropdown; empty selection, Cancel, filtering and dual-report suppression/resumption after save/reopen tested. TogglePlotTests now covers actual MDI selection of Orbit/Ground/XY subscribers, exact Undo/Redo/Unicode save/reopen, independent state reports, suppressed samples with separated resumed arcs, live colors changed while disabled and close/reopen retention. CompletedDisabledPlots adds final ToggleOff with recent-segment display, retained full Latest history and unchanged source/state reports. SolverToggle covers actual Current-mode differential-corrector loop toggles. OptimizerToggle/NestedSolverToggle now add Yukon and a complete inner targeter inside the disabled interval: exact source/Undo/Redo/Unicode reopen, independent full reports/known goals, suppression/separated resumption, Ground/XY accepted paths matching None, full camera/replay and native close/reopen. EphemerisToggle now covers actual ordered EphemerisFile/Orbit/Ground/XY selection, initially disabled writer activation, two enabled arcs, terminal Off, OEM/STK complete state rows, analytic circle and script-reference reports, correct resumed STK segment starts, Output access, invalid-subscriber correction and repeat-run cleanup. New command/resource/report placement stays within the workspace; native Wayland controls and scenes inspected. SolverEphemerisToggle adds first-command DC/OEM and Yukon/STK scopes, accepted-only ephemeris with complete trial reports, Current/None accepted-history agreement and native recovery. Ground Track now names empty curves before the first solver breakpoint, removing accumulated trials without changing calculations. BinaryEphemerisToggle adds actual SPK two-arc output and Code-500 initial activation/terminal Off in both byte orders, binary Output details/Copy path, complete decoded/script-reference states, native viewer recovery and plugin readback. Code-500 internal Toggle gaps corrupt the independent script output and remain unqualified; nested solver ephemeris and other mixed/deeper combinations remain pending. |
| `src/gui/command/GmatCommandPanel.hpp` | Generic editable command text, interpretation/object validation, failure rollback and shared inspection buttons audited. Qt full-mission transactional Apply retains the text fallback; InspectionTests exercises ClearPlot to MarkPoint correction, missing-reference rollback, Unicode save/reopen and report invariance. Other generic command types remain partial. |
| `src/gui/command/ManeuverPanel.hpp` | Typed impulsive-burn and spacecraft selectors; Cancel, label/comment preservation, save/reopen and inertial delta-V execution tested. Backprop checkbox and reverse inertial delta-V tested. BurnTests extends execution to GUI-configured MJ2000Eq/VNB/LVLH/SpacecraftBody/EarthFixed, fuel depletion and backward state/fuel restoration. Broader spacecraft/frame/error combinations pending. |
| `src/gui/command/ScriptEventPanel.hpp` | wx comment/body separation, fixed Begin/End labels, resizable editor areas and pending Save/validation audited. Qt Script event dialog provides separate plain comments and a highlighted, numbered script body with a splitter; preserves named/inline outer boundaries and nested content. MissionTests covers opening without changes, comment-only preservation, Cancel, invalid-command rollback and correction, nested branches/events and quoted marker literals, single pending Undo/Redo, exact mission Undo/Redo, Unicode save/reopen and empty-body execution. Native Wayland layout/execution inspected. Common editor/menu workflows remain under their separate inventory audits. |
| `src/gui/command/NonlinearConstraintPanel.hpp` | Active optimizer/left/right/relation controls audited; inactive wx tolerance control excluded. Constraints adds array-element source mapping, read-only <=/>=/= choices, numeric single operand browser and literal-index pre-Apply bounds checks. Actual MDI Cancel/pending/retained Apply, exact labels/comments/source/Undo/Redo/Unicode Save/Save As/reopen, invalid type/reference/index rollback and correction/rerun are covered. Five fixed-bound Yukon cases match analytic optima and independent complete iteration reports; native Wayland controls inspected. A separate literal-left/varying-right script produces NaN before any GUI edit; this numerical regime remains unqualified with raw evidence, without an engine rewrite. Dynamic-index and broader operand/property combinations remain pending. |
| `src/gui/command/AchievePanel.hpp` | Boundary-value solver selector plus single-parameter goal/value browser. Selected target variable, Cancel, save/reopen and solved result tested; Omitted tolerance can be added from the engine default and edited; reopened solve covered. SolverOperands now adds actual MDI array goal/value/tolerance selectors (including the missing tolerance browser), numeric caller filtering, literal-index rejection, exact source/Undo/Redo/Unicode Save/Save As/reopen, independently known goal 4 and byte-identical full iteration reports after correction/rerun; native Wayland panel/picker inspected. Broader tolerance/property combinations pending. |
| `src/gui/command/ManageObjectPanel.hpp` | Active Global/Save checklist contract audited; automatic-global filtering and Save export/recovery evidence retained. GlobalScopes adds actual MDI ordered selection, Cancel/pending/retained Apply, missing Global insertion template, exact source/Undo/Redo/Unicode Save/Save As/reopen, shared Variable/Array/String/Spacecraft updates across two function calls, local shadow isolation, repeated Global idempotence and deferred-reference runtime failure/correction/rerun with independent complete reports. Clean command Apply now skips a no-op rebuild/Undo transaction. Native Wayland panel/checklist inspected. Clear is not registered by the selected runtime and has no active wx mission caller; the old synthetic Clear selector did not establish an executable workflow. Broader dependency/dynamic-scope cases remain under qualification. |
| `src/gui/command/BeginFiniteBurnPanel.hpp` | Typed finite-burn/spacecraft selectors; selected ten-second constant-thrust burn, analytic fuel consumption and save/reopen tested. Other thruster/tank models pending. |
| `src/gui/command/OptimizePanel.hpp` | Optimizer selector, SolveMode/ExitMode dropdowns and progress checkbox provided. Partial-option insertion preserves pending solver/name/options. Active Apply Corrections was missing from the earlier inventory; it now updates numeric initial guesses with source retention, pending/stale guards and Undo/Redo. Actual Yukon controls, known optimum and Unicode reopen tested; native Wayland panel captured and inspected. SolverModes covers all Solve/RunInitialGuess × SaveAndContinue/DiscardAndContinue/Stop combinations in a repeated spacecraft propagation loop, documented next-invocation guesses, exact independent reports and retained source. Disabled stopped/failed viewer histories and output-path recovery now pass offscreen and native Wayland. Other optimizer/constraint and nested combinations remain pending. |
| `src/gui/command/TargetPanel.hpp` | Boundary-value solver selector, SolveMode/ExitMode dropdowns, progress checkbox and omitted-default insertion tested. Initial-guess execution, Undo and later solve verified. Apply Corrections now covers scaled numeric guesses, retained references, nested ownership, script-event Vary commands, unexecuted guards and corrections/recovery after ExitMode Stop or nonconvergence. Known goals and Unicode reopen pass. SolverModes now covers every Solve/RunInitialGuess × ExitMode combination in a repeated spacecraft propagation loop, the documented saved/discarded next-invocation guess, known goals/epochs, exact independent state/geodetic reports and source round trips. Intentional Stop classification and disabled partial viewer completion/recovery pass. More complex nested/mixed solver and plot-mode combinations remain pending. |
| `src/gui/command/VaryPanel.hpp` | Solver selector offers boundary-value solvers and optimizers; writable numeric variable picker added. Selected Vary variable and Achieve target survive save/reopen and solve correctly. Missing option controls can be inserted from engine defaults without losing pending edits; edited bounds/step and reopened solve covered. Solver capability flags now control field enabling; DC/Yukon switching, Cancel and unknown-solver recovery preserve pending values. SolverOperands adds array variable/initial/all-option source spans, selected array Vary for DC/Yukon, mixed reference/literal bounds, scale factors, exact source/Undo/Redo/Unicode Save/Save As/reopen and independent full iteration reports for goal 4/optimum 3. Invalid literal indices, edited inverted bounds and out-of-range literal initial guesses stay pending with inline explanations; correction/rerun matches reference. Other plugin-specific variable cases pending. |
| `src/gui/command/FindEventsPanel.hpp` | Event-locator selector and Append controls covered; manual EclipseLocator replace/append execution and round trips tested. Other locator types and failure modes remain pending. |
| `src/gui/command/PropagatePanel.hpp` | Single parameter/value stop selection and source-preserving controls covered; periapsis/apoapsis selectors and execution covered; direction/tolerance and multiple-stop editing covered; multi-propagator assignment editing and synchronized two-spacecraft execution covered; STM/A-matrix controls and two-spacecraft execution covered. Segment color override now has pending controls, named/RGB picker, invalid correction, Cancel, exact Undo/Redo/save/reopen and independent calculation and Orbit/Ground color-history agreement; see the segment-color appendix. Covariance controls now include automatic STM, retained panel/source mapping, Cancel, exact Undo/Redo/Unicode save/reopen and independent shortened shipped Moon/SNC covariance and state reports; see the covariance-propagation appendix. Broader formation/mode and covariance configurations remain pending. |
| `src/gui/command/AssignmentPanel.hpp` | CommandForm destination/expression controls plus writable destination picker (including user strings/arrays); source-preservation tests. Picker filtering tested. Actual MDI spacecraft runtime OrbitColor/TargetColor string/RGB expression edits, rejected-syntax correction, retained Apply, exact Undo/Redo/Unicode save/reopen, independent reports and Orbit/Ground color histories are now covered; see the runtime-color appendix. Other destination-specific execution and complex syntax audit remain pending. |
| `src/gui/command/CallFunctionPanel.hpp` | Function resource selector plus ordered input/output argument browsers provided. Cancel, quoted/nested comma preservation, invalid numeric outputs, reordering and multi-output execution after save/reopen covered. Python module/function and ordered arguments, labeled calls, empty/bare inputs and unbracketed scalar outputs are covered in the Python appendices. GmatCallSyntax adds GMAT empty/bare calls, scalar output spelling, labeled serialization/source mapping, retained MDI Apply/Undo/reopen and independent zero-input/no-output execution. Retained whole-Array inputs/output cross-product execution is covered. One actual-input local whole-Spacecraft/String call adds offered/ordered picker selection, Variable/String returns, exact known mass/text report and save/reopen; see [local arguments](Qt6ParityValidation/local-function-arguments-20261002.md). Other resource/signature and whole-object output cases remain outside this bounded evidence. |
| `src/gui/command/EndFiniteBurnPanel.hpp` | Typed finite-burn/spacecraft selectors; fuel remains constant during coast after selected EndFiniteBurn. Other thruster/tank models pending. |
| `src/gui/command/MinimizePanel.hpp` | Active optimizer/objective browser and numeric-reference restrictions audited. Solver selector previously tested. SolverOperands adds array objective source mapping, actual MDI array choice/Cancel/pending/retained Apply, exact labels/comments/source/Undo/Redo/Unicode Save/Save As/reopen, independently known Yukon optimum 3 and byte-identical complete iteration report, out-of-range index rejection/correction/rerun; native Wayland panel inspected. Broader objective/property combinations pending. |
| `src/gui/command/ReportPanel.hpp` | Configured report-file picker and shared ordered parameter dialog: add/remove/reorder, numeric array indices, Cancel, labels/comments and numerical output tested. Object/property and coordinate/central-body browsing implemented; owned attitude and attached tank/thruster browsing tested; broader hardware/plugin types pending. |
| `src/gui/function/MatlabFunctionSetupPanel.hpp` | FunctionPath load/save and browse controls audited. It requires MATLAB function execution outside the selected startup/runtime (PLUGIN_MATLABINTERFACE=OFF); no MATLAB execution qualification is claimed. Supported GMAT function path/editing/creation workflows remain under FunctionSetupPanel. |
| `src/gui/function/FunctionSetupPanel.hpp` | In-app GMAT function-file editing, Save/Cancel and find/replace provided. BOM/CRLF preservation, external-change protection and edited-function execution after mission save/reopen covered. Save As, pending path Apply and execution from the copy covered. New template-based file creation, Cancel, path Apply and reopened execution covered; broader multi-output/function-signature cases remain pending. |
| `src/gui/solver/SolverVariablesPanel.hpp` | Inactive legacy panel audited: no creation call in the wx GUI, fixed 20-row prototype, SaveData only disables Apply. Active variable edits are VaryPanel; Qt solver/variable selectors, capability-dependent bounds and pending values are covered by WorkflowTests and CompatibilityTests. No separate inactive grid is required; broader active Vary cases remain under that row. |
| `src/gui/solver/SolverSetupPanel.hpp` | Legacy reflection-based settings audited: bool, real, integer and string load/save for writable fields, with no creation call in current GmatMainFrame. Active generic solvers/optimizers use GmatBaseSetupPanel. Qt ResourceEditor provides typed fields, units/options, report paths, clone validation and transactional Apply; selected DC/Yukon workflows are covered by WorkflowTests/CompatibilityTests. Broader selected plugin settings remain under the runtime inventory. |
| `src/gui/solver/SolverGoalsPanel.hpp` | Inactive legacy panel audited: no creation call, SaveData returns without writing and its grid update is commented after a known crash. Active goals/objectives/constraints are Achieve/Minimize/NonlinearConstraint commands; Qt mappings and remaining execution cases are recorded on those rows. No separate inactive goal grid is required. |
| `src/gui/solver/DCSetupPanel.hpp` | Six wx fields audited; algorithm/derivative dropdowns and report output picker corrected. Pending Apply, chooser Cancel, Broyden/CentralDifference save/reopen solve and report writing covered. Other algorithm/derivative combinations remain to qualify. |
| `src/gui/solver/SolverCreatePanel.hpp` | Unused skeleton audited: Setup and button logic are commented, Initialize/GetData/SetData are empty, and no GUI caller instantiates it. Active wx solver creation uses ResourceTree factories; Qt New resource uses runtime factories. ResourceRoundTripTests creates/deletes DifferentialCorrector and Yukon with original source and calculation reports unchanged. |
| `src/gui/solver/SQPSetupPanel.hpp` | Legacy MATLAB fmincon controls audited: TolFun/TolCon/TolX, MaxFunEvals, MaximumIterations, finite-difference limits and progress/report settings. The wx creation menu is Windows/MATLAB-gated; the selected Qt startup comments libFminconOptimizer and this build has PLUGIN_MATLABINTERFACE=OFF. Outside the selected Linux runtime; no execution or Qt MATLAB qualification is claimed. |
| `src/gui/controllogic/ForPanel.hpp` | CommandForm index/start/step/end with variable-only index selector and shared bound parameter browsers; Cancel, filtering, selected numeric bounds, execution and Undo tested. Broader parameter-valued loop execution pending. |
| `src/gui/controllogic/ConditionPanel.hpp` | Comparison-row builder with numeric/parameter/array operands, six relations, AND/OR, add/remove and shared operand browser. Cancel, incomplete rows, header-only replacement, real If execution and Undo tested; grouped/expression syntax stays in text. Broader parameter selection and While execution cases pending. |
| `src/gui/spacecraft/OrbitPanel.hpp` | Epoch-format conversion, invalid-date recovery and pending state edits covered. Typed frame/anomaly selectors, dependent representation restrictions, physical-state caching and grouped Apply implemented. All fourteen representation display round trips, Earth-fixed/Moon/barycenter frames, circular/missing-frame recovery, paired epoch/state conversion, exact Undo/Redo and Planetodetic/MA/EA/HA save/reopen/report execution covered. Unrelated Apply/deletion preserve these states. Broader representation editing, singularities and specialized frames remain to qualify. Orbit Designer/Summary are legacy hidden workflows; see the source audit below. |
| `src/gui/spacecraft/PowerSystemPanel.hpp` | Single typed selection plus empty selection audited. Direct dropdown with No power system, nuclear/solar candidates and pending Apply implemented. Returning to None, wrong-type rollback, attachment, save/reopen report execution and detachment covered. Electric propulsion consumes GUI-attached nuclear power in all three thrust models; missing-power failure and attachment recovery covered. Solar/shared-power propulsion cases pending. |
| `src/gui/spacecraft/BallisticsMassPanel.hpp` | All eleven wx controls audited. Focused Spherical/SPAD editor, input file choosers and engine interpolation choices implemented. Cancel, invalid-input recovery, pending edits, paired Apply, exact-source Undo/Redo, save/reopen and GUI-configured SPAD SRP execution/report values covered. SPAD drag force execution with Bilinear/Bicubic interpolation, scale 1.5 and missing-file recovery covered. Broader assets and interpolation/scale combinations remain to qualify. |
| `src/gui/spacecraft/TankPanel.hpp` | wx add/remove/add-all/remove-all attachment operations audited. Typed checklist, ordering and bulk controls implemented. Cancel, pending selection, paired Apply, Undo/Redo, invalid references, save/reopen, chemical two-tank burn/report and complete detachment covered. Electric tank attachment/execution combinations remain to qualify. |
| `src/gui/spacecraft/OrbitDesignerDialog.hpp` | Source audited: launcher explicitly hidden by OrbitPanel (GMT-3383). Retained legacy design tools are not exposed workflows in the base wx GUI; see the audit below. |
| `src/gui/spacecraft/VisualModelPanel.hpp` | File, rotation/translation/scale sliders and numeric fields, recenter/autoscale, new-file pose reset, colors and Earth-reference preview audited. Qt visual editor implemented with pending Apply, engine normalization and model-read diagnostics. Cancel, file/color pickers, invalid input recovery, Undo/Redo and save/reopen propagation covered. Textured OBJ preview/rotation/Earth reference tested at normal, 150% and 200% scaling and directly on Wayland. Broader 3DS asset/material cases remain to qualify. |
| `src/gui/spacecraft/AttitudePanel.hpp` | Model-dependent controls, frame restrictions, Euler/quaternion/MRP/DCM orientation and Euler-rate/angular-velocity selection audited. Focused Qt dialog with pending conversion, per-model edit retention, typed frame/body selectors and AEM input chooser implemented. Cancel, invalid state recovery, exact paired Apply/Undo/Redo, save/reopen and zero-rate Spinner report execution covered. Nonzero-rate propagation, specialized-model execution, other Euler sequences, frame dependencies and AEM/SPICE file cases remain to qualify. |
| `src/gui/spacecraft/SpaceObjectSelectDialog.hpp` | ResourceEditor engine-typed reference picker. Ordered tank selection, Cancel, pending state and mixture-preserving Apply tested; all object-specific uses still need audit. |
| `src/gui/spacecraft/OrbitSummaryDialog.hpp` | Source audited: read-only output of the hidden Orbit Designer, rather than a standalone spacecraft summary action; see the audit below. |
| `src/gui/spacecraft/SpacecraftPanel.hpp` | Wrapper Create/LoadData/SaveData/page-change behavior audited: eight active orbit, attitude, ballistics/mass, tank, power, SPICE, thruster and visual pages share a pending spacecraft clone; disabled sensor page excluded. Qt ResourceEditor and focused dialogs share pending values and one Apply/rebuild with rollback. Individual page evidence is recorded above/below. Broad mixed-page transactions and focus/navigation behavior remain to qualify. |
| `src/gui/spacecraft/ThrusterPanel.hpp` | wx attachment operations audited. Typed checklist with bulk selection/removal covered by Cancel, pending Apply, replacement of a decoy engine, exact-source Undo/Redo, missing-reference rollback and reopened finite-burn execution. Full detachment covered; electric execution with script-attached thrusters and GUI-attached power covered, electric GUI attachment combinations remain to qualify. |
| `src/gui/spacecraft/SpicePanel.hpp` | Ordered SPK/CK/SCLK/FK file-list controls added. SPK add/duplicate/order/Cancel/Apply, Undo/Redo, save/reopen, clear and missing-file recovery tested with bundled kernel copies. Mars Express SPK/CK/SCLK execution, Qt trajectory/attitude capture and clock-file recovery tested. FK runtime use and other NAIF combinations pending. |
| `src/gui/spacecraft/FormationSetupPanel.hpp` | ResourceEditor Add list and spacecraft picker; invalid member rejection, reordering, save/reopen and two-member propagation tested. Remaining wx-specific operations under audit. |
| `src/gui/foundation/GmatBaseSetupPanel.hpp` | Generic writable/visible field generation, cloned validation and INI-based layout/units/help audited. Qt ResourceEditor supplies engine-typed controls, owned properties and atomic script rebuild/rollback; object-specific execution evidence is recorded in individual rows. INI metadata parity, omitted plugin types and all dynamic refresh cases remain pending. |
| `src/gui/foundation/GmatDialog.hpp` | Shared OK/Cancel/reset, validation-before-close and Help contract audited. Qt focused dialogs keep pending values until acceptance; field/dialog Cancel and invalid-input recovery are covered by the corresponding suites. Close/Escape/focus behavior across all dialogs and context Help remain pending. |
| `src/gui/foundation/ParameterCreateDialog.hpp` | Active wx numeric Variable, literal String, Array creation (dimensions 1–1000), name validation and existing-user-parameter operations audited. Qt New resource supplies typed initial values, retains values on type changes, rejects duplicate/reserved names and invalid dimensions, and opens the created resource in the tree. Parameters tests cover actual dialog Cancel, correction, creation Undo/Redo, Unicode String and maximum Array creation, then deletion without changing original report results. Existing parameters open from Resources; wx list/Clear layout and shared Help remain unqualified. |
| `src/gui/foundation/ParameterSelectDialog.hpp` | Active report/XY/dynamic-data, Vary/Achieve/Minimize/constraints, loop/condition, propagation-stop, burn/group and function callers audited. wx supports single/multiple object/property selection, attached hardware, coordinate/body/ODE dependencies, writable/plottable/whole-object restrictions, array indices, Add/Remove/All/reorder and Cancel. Qt ReportParameterDialog and typed resource/group controls cover ordered scalar/array, dependency, hardware and caller-specific selection with existing execution/round-trip tests. Bulk multi-object/property selection and Add All/Remove All are now implemented. ParameterSelectionTests covers typed shared frames, caller filtering, conflict/recovery for different attached tanks, ordered Report Apply with independent numeric output, source/Undo/Redo/save/reopen, bulk deduplication and positional function duplicates; native Wayland controls were captured and inspected. Broader caller/dependency combinations and desktop input remain under qualification. |
| `src/gui/foundation/SinglePathSetupPanel.hpp` | wx pending directory text and directory chooser audited. Qt Set paths Output tab provides pending text/Browse, existing/writable validation and Apply. PathTests covers directory chooser acceptance/Cancel, invalid correction, Unicode output, relocated default reports/log and unchanged explicit report destination. Native Wayland layout checked; portal chooser and wider permission/storage failures remain unqualified. |
| `src/gui/foundation/GmatPanel.hpp` | Shared Apply/OK/Cancel, dirty-state, resource refresh, Help, Script and Summary contract audited. Qt resource/command panels validate and rebuild atomically, retain and refresh accepted panels after Apply, reject stale edits and protect pending changes. Read-only applied-script previews and command/mission summaries are covered by InspectionTests. Desktop/Mission tests cover retained resource and command panels, clean companion refresh and protected pending companions. Offline context Help and inherited modal Help are implemented and exercised. New command/resource/report windows now clamp their cascade geometry to the workspace; EphemerisToggle reproduces off-viewport controls, verifies visible Apply and resource/report bounds without moving existing windows, and passes native Wayland. Broader mixed Apply/focus/desktop cases remain under qualification. |
| `src/gui/foundation/MultiPathSetupPanel.hpp` | wx ordered path list, text/Browse, Add at top, Replace, Remove, Up/Down and directory validation audited. Qt Set paths GMAT Function and Python Module tabs provide these operations and duplicate protection, normalizing equivalent directories while keeping first search priority. PathTests covers actual function controls/choosers, Cancel/Apply and same-named function execution. PythonPaths adds ordered pending import/export, invalid rollback, unrelated startup retention and fresh-process known 13.25/23.5 imports; private actual input confirms reordered export/import and module-origin readback after restart. Python path changes require export and restart; see [ordered Python paths](Qt6ParityValidation/python-module-paths-20261002.md). Broader keyboard/focus and optional MATLAB paths remain unqualified. |
| `src/gui/foundation/GmatColorPanel.hpp` | COLOR_TYPE resource fields and visual picker/swatch added. Spacecraft orbit/target Cancel, pending Apply, Undo/Redo, invalid RGB rollback, save/reopen and published trajectory color tested. Propagate's segment override has matching controls and independent trajectory/color-history checks, including Ground Track display metadata and default restoration; see the segment-color appendix. Legacy per-view orbit/target color controls are compiled out in this selected wx build and their engine parameters are removed; active colors come from SpacePoint or Propagate controls. Other resource color types remain pending. |
| `src/gui/foundation/ArraySetupDialog.hpp` | wx numeric grid, direct row/column selection, Value/Update, finite-value validation and clone/commit audited. Qt numeric grid adds direct Row/Column/Value/Set cell controls and Enter support; selection scrolls to the cell and synchronizes its value. Parameters tests cover actual 1000×1000 creation, last-cell navigation, invalid Set, Cancel, pending acceptance/Apply, adjustable columns and save/reopen. Broader keyboard/focus and shared Help remain unqualified. |
| `src/gui/foundation/ShowScriptDialog.hpp` | Read-only object-generated script, monospaced/unwrapped display and Close audited. Qt resource and command Show script dialogs capture applied configuration; actual MDI controls preserve pending edits/source/undo state. Local Find and Copy are available. Singleton formatting, font zoom and broader object families remain unqualified. |
| `src/gui/foundation/GmatSavePanel.hpp` | Shared Save/Save As, save-build-run, active/dirty status, reload and close contract audited. ScriptDocumentTests qualifies independent active/inactive MDI scripts, per-document Save/search/Undo, explicit activation, pending Apply/Discard/Cancel and rejected-Apply recovery, selected save-build-run, active/inactive reload, encoding/write/collision failure protection and all-document runtime/close guards with independent relative-include reports. FileTests, WorkflowTests and ScriptEditingTests retain existing mission/file evidence. Widget choosers and synthetic shortcuts are covered; native portal input remains unqualified. |
| `src/gui/foundation/ParameterSetupPanel.hpp` | Active wx disabled Name and numeric Value/String Expression controls and Apply audited. Qt focused Initial value controls replace the ineffective generic fields; source edits preserve grouped declarations, comments, optional semicolons and subsequent mission assignments. Interpreted-value postconditions reject silent String truncation. Parameters tests cover pending values, applied script previews, invalid correction/rollback, exact Undo/Redo, Unicode save/reopen, native Wayland and independently scripted numeric/text reports. Resource windows now stay open and refresh after Apply, with repeated Variable Apply/source/report evidence in DesktopTests. Shared Help and wider resource/keyboard cases remain pending. |
| `src/gui/foundation/ArraySetupPanel.hpp` | ResourceEditor resizeable numeric grid plus separate mission-start expression grid; retained/new cells, dependent formulas, Cancel, rollback, Undo/Redo and save/reopen tested. Combined numeric/expression Apply, pending resize dimensions, shrink cleanup, atomic Undo and rollback covered; arbitrary existing assignments remain outside the grid workflow. Numeric dimensions now cover wx 1–1000 per axis; direct cell controls and maximum-array Cancel/Apply/reopen are covered by Parameters tests. |
| `src/gui/foundation/ShowSummaryDialog.hpp` | Captured command state, entire mission/all or physics selection, non-spacecraft-dependent coordinate systems, frame-change error rollback and text export audited. Qt inspection suite compares command states to separate reports in four frames, handles BeginScript via EndScript, skips unexecuted states, rejects stale results and covers Unicode export/source protection, native Wayland, failed/stopped recovery. Broader solver loops, spacecraft hardware fields and font zoom remain unqualified. |
| `src/gui/propagator/PropagationConfigPanel.hpp` | Owned propagator settings exposed; numerical/TLE step edits and serialization tested. Atmosphere/drag controls and selected Earth execution cases covered. ForceSelectorTests qualifies actual SRP/relativistic creation/removal, mixed-field failure/correction and source-preserving independent Earth state reports. GravityBodyTests adds mixed primary/point-mass selection and dependent drag, list-only removal, source/legacy retention and independent Earth reports; other specialized layout and remaining settings pending. |
| `src/gui/propagator/PropagatorSelectDialog.hpp` | Active PropagatePanel caller audited: configured PropSetup single selection, OK updates a pending grid row and Cancel leaves it unchanged. Qt PropagationForm/PropagationGroupsDialog provide configured propagator dropdowns and paired spacecraft groups. CompatibilityTests covers Cancel, a selected second propagator, empty/duplicate-spacecraft rejection, pending Apply, synchronized execution and save/reopen; WorkflowTests covers source/modifier/formation and variational flags. Broader propagation cases remain under PropagatePanel. |
| `src/gui/asset/GroundStationPanel.hpp` | Active wx ID/elevation/body/state/horizon/location controls and colors audited. Grouped Qt station editor, dependent conversion/labels/units, color and horizon-mask pickers implemented. Cancel, pending Apply, paired state/location ordering, Earth/Mars geometry, compact scrolling, exact Undo/Redo/save/reopen, contact intervals, mask execution/clear and missing-mask recovery covered. Station hardware/media/error models and broader bodies/contact cases remain unqualified. |
| `src/gui/debugger/InspectorPanel.hpp` | Active wx-only DebuggerCommandFactory registration in GmatApp and transient, non-serialized Breakpoint caller audited. Qt now supplies mission-tree breakpoint markers, Run/Debug, read-only live DEBUG_INSPECT/current Parameter values, spacecraft/all filters, command stepping, resume on Close/Escape and End/Stop. DebuggerTests exercises real context menus, For/If/propagation, Target/Optimize iteration values/reports, script events and function step-over, independent byte-exact reports, source/pending protection, Undo/Redo/save/reopen, Pause, safe main-window close and build/initialization/execution recovery. Offscreen/native X11 keyboard routing, Help and exposed native Wayland inspection were verified. Fresh desktop input and larger nested combinations remain under shared qualification; function-local stepping is outside this main-mission inspector. |
| `src/gui/forcemodel/DragInputsDialog.hpp` | Nine wx weather controls audited. Grouped Qt atmosphere/body/shape selection, dependent weather/Schatten controls and input pickers implemented. Earth MSISE90/JacchiaRoberts/NRLMSISE00 and Exponential configuration, validation/Cancel, paired Apply, Undo/Redo, save/reopen, density/trajectory reports and file-error recovery covered. ForceSourceTests adds narrow creator switching, first creation, disable/re-enable and mixed ErrorControl Apply without regenerating unrelated force settings. CSSI historic/predicted and selected Schatten prediction covered; broader file contents, coverage boundaries, Schatten modes and non-Earth cases remain to qualify. |
| `src/gui/coordsystem/CoordSysCreateDialog.hpp` | Basic creation plus dedicated Axes dialog tested; MOEEq epoch and constrained-frame edits checked. Remaining origin and specialized-mode cases pending. |
| `src/gui/coordsystem/CoordSystemConfigPanel.hpp` | Axis replacement, dependent field exposure, protected built-ins, failed-edit rollback, Undo and save/reopen tested. Broader modes pending. |
| `src/gui/coordsystem/CoordPanel.hpp` | ObjectReferenced radial frame, MOEEq epoch edits and Sun-aligned LocalAlignedConstrained transforms checked, including save/reopen. Other modes and dependency cases pending. |
| `src/gui/output/ReportFilePanel.hpp` | Read-only, unwrapped report text, full path in title, text selection, close and unavailable-file handling tested. Standard Qt copy controls provided; large reports now have bounded paging, navigation, page-local search and reload recovery. Complete-file Match case/Whole words now cover Unicode and stream/page boundaries, Next beyond 18 MiB, cancellation/close and missing-file recovery, with unchanged complete input/source/calculation bytes and inspected native Wayland controls. Broader encoding and in-scan replacement remain unqualified. |
| `src/gui/output/EventFilePanel.hpp` | Active read-only unwrapped text/Close, output-path resolution, FileWasWritten guard and disabled Help audited. Qt Output uses ReportViewer for generated reports and an explanatory read-only view for unwritten locators. EventLocatorTests covers generated contents, pending source, rebuild, Disabled, WriteReport-off and Manual-without-FindEvents stale-file rejection, Manual FindEvents recovery, close/reopen and unchanged source. Output windows explicitly activate; native Wayland generated/unwritten views inspected. Broader lifecycle/storage/keyboard cases remain pending. |
| `src/gui/output/CompareReportPanel.hpp` | wx read-only, unwrapped comparison output and Close audited. Qt comparison workspace uses the paged ReportViewer with complete-file search and Close. ComparisonTests covers complete results/export beyond 16 MiB, error summaries, Stop/close and generated-report agreement; native Wayland layout inspected. Shared ReportViewer full-file case/whole-word controls have ReportSearchOptions evidence; broader comparison menu and very large directory cases remain under their separate audits. |
| `src/gui/mission/UndockedMissionPanel.hpp` | Active MissionTree/GmatNotebook undock/restore caller audited. wx creates a separate mission tree and vertical MissionTreeToolBar, restoring the notebook on destruction. Qt MissionNavigation now moves the same tree and its toolbar into a mission-only floating dock, retaining Resources/Output tabs, selection and editing callbacks. Dock/Close restores the Mission tab; saved detached placement restores. MissionNavigationTests and native Wayland captures cover repeated lifecycle, filters, real command editing, exact source/Undo and independent reopened reports. Broader compositor minimize/input and mixed desktop layouts remain under shared window qualification. |
| `src/gui/mission/TreeViewOptionDialog.hpp` | Active MissionTreeToolBar caller audited. Sorted command checklist, Check/Uncheck All and Include/Exclude Apply update the visible MissionTree without editing the mission; Equation and ScriptEvent map to GMAT and BeginScript. Qt MissionNavigation provides sorted engine/current-command checklists, Check/Uncheck All, Include/Exclude Apply, Show all and collapsed/level 1–3/all expansion. Equation/ScriptEvent aliases and branch context/boundaries are retained without editing source. MissionNavigationTests covers actual dialog operations, unapplied Close, nested branches, exact snapshot/source/Undo and native Wayland layout; see the mission-navigation appendix. |
| `src/gui/view/ViewTextDialog.hpp` | Read-only multiline/Close and optional single-line OK/Cancel modes audited. Active callers are About license text, folder-run diagnostics and comparison results; editable rename caller is commented. Qt comparison/read-only report text is covered by ComparisonTests; About/license and folder-run delivery remain required under their separate unaudited rows. Shared keyboard/font/menu cases remain pending. |
| `src/gui/view/FindReplaceDialog.hpp` | Nonmodal Find/Replace with next/previous, wrap, session histories, selected replacement and Replace All. Case/whole-word controls, no-match feedback, read-only protection and single-operation Undo tested. |
| `src/gui/solarsys/LibrationPointPanel.hpp` | Active primary/secondary, L1–L5 and orbit/target color controls audited. Typed celestial-body/barycenter choices exclude spacecraft, libration points and SSB; paired Apply rejects equal bodies. Pending choices/colors, invalid edit rollback, exact Undo/Redo, Unicode save/reopen, coordinate reports and orbit publications covered. Earth/Luna all-five geometry and Sun/custom-barycenter execution covered; broader body/epoch regimes pending. |
| `src/gui/view/EditorPanel.hpp` | Active save/sync/run, empty-input protection and shared SavePanel actions audited. Qt Mission menu provides Save/build and Save/build/run for the selected document, with explicit activation. ScriptEditingTests covers save-before-Build/Run, Unicode paths, independent outputs, failures and empty-script protection. ScriptDocumentTests covers multiple inactive scripts, independent source/history/Save/search, failed activation restoration, pending-panel decisions, reload and close. FileTests/WorkflowTests retain encoding and editing evidence; native portal and wider desktop-input gates remain outstanding. |
| `src/gui/app/CompareFilesDialog.hpp` | Active wx modes, absolute tolerance, skip blanks, baseline/candidate prefixes, up to three directories, file limit and result export audited. Qt File > Compare files workspace and report Compare action provide these controls plus two-file comparison. ComparisonTests covers UTF-8/BOM/CRLF, tolerance boundaries, UTC columns, maxima, trailing rows, invalid input, three-directory matching/.truth fallback/exact limits, editable widths, picker Cancel/row removal, full export/input protection, Stop/close and real-engine report/save/reopen invariance. Native Wayland inspected. Non-UTF-8 files, arbitrary initial header/data ambiguity, mid-read replacement and extreme directory/record regimes remain unqualified. |
| `src/gui/app/ScriptPanel.hpp` | Legacy plain editor save/sync/run, line-number navigation and failed-save identity behavior audited. Qt numbered/highlighted script windows provide per-document Find/Replace and bounded Go to line; ScriptEditingTests and ScriptDocumentTests qualify selected save-build-run, independent active/inactive document editing/history/status, duplicate identity, reload, close, Unicode/relative includes and recovery with independent reports. Synthetic Save/Undo/Redo shortcuts are covered on offscreen/native X11; native Wayland capture uses widget actions and does not qualify fresh desktop input. |
| `src/gui/solarsys/CelestialBodyOrientationPanel.hpp` | wx read-only rotation-source/built-in pole rules, Earth nutation interval, custom pole values, frame ID and ordered FK files audited. Ceres pole edits and imported SPICE rotation change body-fixed reports and match separate scripts; FK ordering selects the final frame definition. Startup Luna FK removal survives save/reopen. Wider epochs, bodies, frame/pole conventions and source switching remain unqualified. |
| `src/gui/solarsys/UniversePanel.hpp` | Source/file/timing controls audited. SolarSystem resource and grouped Qt panel expose runtime sources, paired DE file, SPK/PCK browsing, UseTT and interval. DE405/421/424 and SPICE missions, copied Unicode files, retained SPICE DE fallback, independent script reports and body/frame checks, pending/Discard/Cancel, invalid/truncated-file rollback, correction, exact Undo/Redo, save/reopen, comment/implicit boundary preservation and later resource/mission edits covered. Native Wayland panel workflow passed. Wider epochs, caching regimes, malformed full DE contents, keyboard/portal chooser and shared Help remain unqualified. |
| `src/gui/solarsys/CelesBodySelectDialog.hpp` | Both active wx callers audited: solar-shadow lists hide Sun; primary/point-mass selection excludes the opposite pending gravity list. Qt checked lists provide add/remove, Select all/Clear, reorder, Cancel and Apply. Existing and user Asteroid choices, typed invalid/overlap rejection, exact Undo/Redo, explicit empty shadows, Unicode save/reopen and exact power/propagation agreement with separately written scripts covered. Body-only edits preserve surrounding raw configuration; Wayland dialogs inspected. Optional calculated-point mode, broader pending transitions, keyboard/focus/portal and shared Help remain unqualified. |
| `src/gui/solarsys/CelestialBodyPanel.hpp` | Four wx pages audited and exposed through a dedicated Qt MDI editor. Pending controls, applied-only preview, Close Cancel/Discard, invalid correction/rollback, exact Undo/Redo, Run/Stop guards, body creation and Unicode save/reopen covered. Native Wayland pages inspected. Shared Help, wider keyboard/focus, portal choosers and multi-panel lifecycle cases remain unqualified. |
| `src/gui/solarsys/CelestialBodyVisualizationPanel.hpp` | Texture chooser/preview, supported 3DS/OBJ model chooser, offset/rotation/scale bounds and orbit/target colors audited. Applied assets reach PlotCurve; native Wayland rendered checker texture and posed body model were inspected and exceed pixel-change gates while calculation reports stay identical. Invalid image/model/path rollback, default texture/model clearing and Unicode save/reopen covered. Wider formats, materials and relative paths remain unqualified. |
| `src/gui/subscriber/GroundTrackPlotPanel.hpp` | Active body/object, sampling/update/retention/redraw, visibility, solver and texture controls audited against current GroundTrack runtime and legacy GL behavior. Grouped Qt setup, typed selections, per-body maps, decoded-image validation and engine texture-path resolution implemented. Cancel/pending Apply, compact scrolling, Undo/Redo/save/reopen, rendered custom-map pixels, station-only plots, Mars frame/report agreement and one-point retention covered. LivePlotFlush additionally checks intermediate propagation-block refreshes, recent-segment limits during actual command pauses, live close/reopen and true completion with unchanged state reports. SolverPlots and OptimizerPlots now cover DC/Yukon All/Current/None histories, report invariance, camera/replay and native close/reopen; SolverToggle, OptimizerToggle and NestedSolverToggle cover Current-mode targeter/Yukon/nested toggles, suppressed samples and separated resumption; Ground/XY accepted paths match a None-mode reference, with exact reports and native replay/close-reopen. NestedSolverPlots/Cleanup now add two-level targeter history, scoped initialization/trial filtering, failure/Stop recovery and bounded parent-anchor retention. SolverEphemerisToggle additionally covers a solver as the first mission branch: Ground Current retains only 26 accepted points for DC/OEM and Yukon/STK, rather than the original 104/260 accumulated trial points; named empty curves now receive the initial breakpoint before propagation. Native captures and independent None histories agree, with full numerical reports unchanged. Optimizer/mixed/deeper nested cases, broader body/station and runtime asset-loss combinations remain to qualify. InvalidPlotData now omits finite SPK unavailable markers before map conversion and breaks all-absent intervals; native Orbit/Ground/XY retain 20 valid points with separated recovery arcs and unchanged complete engine reports. |
| `src/gui/subscriber/XyPlotSetupPanel.hpp` | Active wx ShowPlot/ShowGrid/SolverIterations, single X and ordered Y selection audited. Focused Qt setup and numeric property/frame/array browsers implemented. Cancel, pending Apply/reopen, invalid-reference rollback, exact Undo/Redo/save/reopen, grid/visibility and curve/report agreement covered. SolverPlots and OptimizerPlots cover DC/Yukon All/Current/None histories and report invariance; SolverToggle, OptimizerToggle and NestedSolverToggle cover Current-mode targeter/Yukon/nested toggles, suppressed samples and separated resumption; Ground/XY accepted paths match a None-mode reference, with exact reports and native replay/close-reopen. NestedSolverPlots/Cleanup add two-level targeter history, scoped initialization/trial filtering and failure/Stop/retention recovery. Optimizer/mixed/deeper nested cases and broader burn/hardware parameter execution remain to qualify. InvalidPlotData now excludes finite SPK unavailable markers from XY history and separates recovery arcs, with 20 retained valid points and unchanged independent complete reports on native Wayland. |
| `src/gui/solarsys/CelestialBodyPropertiesPanel.hpp` | Mu/radius/flattening validation and ordered PCK lists audited. Earth physical edits and Ceres configuration match separate raw-script reports. PCK Add/Replace/Remove/reorder/Cancel and wrong-type rollback covered; Luna startup PCK replacement/removal survives save/reopen with explicit kernel-list clear. Wider bodies/epochs/physical extremes and SPICE error-file diagnostics remain unqualified. |
| `src/gui/solarsys/BarycenterPanel.hpp` | Active body add/remove/clear and colors audited. Qt membership checklist, retained order, nonempty/unique/celestial-body validation, pending/Cancel/rollback, exact Undo/Redo/save/reopen, mass-weighted positions and dependent frame/libration execution covered. Built-in membership is protected while colors remain editable and persist without creating a new definition. Broader membership/epoch regimes pending. |
| `src/gui/solarsys/CelestialBodyOrbitPanel.hpp` | Runtime source choices, protected built-in source/file/central-body fields, NAIF ID and SPK lists audited. New Asteroid Ceres from the resource dialog, copied Unicode SPK, ephemeris-relative reports and separate scripts agree; unknown-ID and missing-SPK execution failures recover after correction. Dormant wx TwoBody/source-file controls are not enabled. Wider bodies, coverage and relative kernel paths remain unqualified. |
| `src/gui/app/CompareTextDialog.hpp` | Source audited: this compiled legacy class has no caller in the current wx GUI. GmatMainFrame::CompareFiles uses CompareFilesDialog for text/numeric comparison, mapped to the qualified Qt comparison workspace above. No separate exposed workflow was found. |
| `src/gui/subscriber/EphemerisFilePanel.hpp` | Active wx output, sampling, interval and dependent-format controls audited. Grouped Qt editor, typed spacecraft/frame selection, editable sampling/endpoints, paired epoch conversion, format-specific byte order/units/events and filename chooser implemented. OEM (custom extension), STK meters, Code-500 both byte orders and SPK exports/readback covered by pending/Cancel, invalid-edit rollback, exact Undo/Redo, Unicode script save/reopen, independent report/state checks, Output access, directory preservation, coverage and missing-file recovery. EphemerisToggle adds initially disabled activation, OEM/STK two-arc suppression/resumption and terminal Off, exact GUI source transactions, complete independent state rows/reports and analytic circular states, Output access and repeat-run cleanup. STK resumed segment metadata now uses its first actual data epoch, with pre/post state rows unchanged. Native Wayland mixed viewer/workspace checks pass. SolverEphemerisToggle adds bounded DC/OEM and Yukon/STK output within first-command solver scopes: 26 accepted ephemeris rows, complete 16/40-row trial/accepted reports, known goals and independent Current/None history agreement, with native recovery evidence. BinaryEphemerisToggle adds SPK two coverage arcs/all 26 states and both Code-500 byte orders with initial activation/terminal Off/all 37 continuous states, exact source/file recovery, binary Output details/Copy path and native viewer/plugin readback. Code-500 internal gaps and SPK reader gap traversal remain unqualified; CK quaternion, covariance/acceleration, nested/broader solver ephemeris, broader frames/bodies/event boundaries and disk-write cases remain unqualified. |
| `src/gui/burn/FiniteBurnSetupPanel.hpp` | Active wx individual/bulk thruster add/remove operations audited; Qt typed checklist and ordered selection serve the workflow. Cancel/pending Apply, paired-engine execution, analytic fuel/coast, report equivalence, Undo/Redo, Unicode save/reopen, wrong/missing/duplicate references, unattached-thruster recovery and clear-all covered. Empty active burns produce the same explicit engine diagnosis as scripts; GUI reselection recovers. Broader electric/shared-power combinations pending. |
| `src/gui/burn/ImpulsiveBurnSetupPanel.hpp` | Active wx fields audited. Grouped delta-V/frame/optional mass-depletion editor, single typed fuel tank, Isp/gravity dependency and corrective validation implemented. Inertial and all four Local axes, EarthFixed and zero delta-V covered by pending/Cancel, Undo/Redo, Unicode save/reopen, script-reference state, analytic fuel and VNB/LVLH transforms, backward restoration, invalid edit rollback, unattached-tank recovery and mass-off tank clear. Broader bodies, attitudes, epochs and fuel limits pending. |
| `src/gui/app/FileUpdateDialog.hpp` | Source and GmatMainFrame Help caller audited. The menu exists only in TESTING mode. FileUpdaterSVN::CheckForUpdates explicitly returns a non-Windows-not-implemented error before performing updates; the later selected-file/restart batch workflow is Windows-only. No active Linux update workflow is omitted. Windows deployment remains deferred; no Qt Windows update qualification is claimed. |
| `src/gui/app/TextEphemFileDialog.hpp` | Source and Generate Text Ephemeris caller audited: prototype spacecraft/epoch/frame/interval/output selection creates a TextEphemFile subscriber and runs the mission. The menu is TESTING-only and additionally guarded by the disabled __SHOW_EPHEM_FILE__ macro in GmatMenuBar. No current menu route exists in this Linux build. The engine still registers TextEphemFile; its generic/script behavior is not qualified by the modern EphemerisFile export suite. |
| `src/gui/subscriber/TsPlotOptionsDialog.hpp` | PlotWidget Style dialog: per-curve visibility, lines/markers, widths, marker sizes/shapes, line styles, colors and error bars; plot grid/legend. Cancel, existing-point styling, curve isolation and rendered differences tested. Active plot/axis labels, independent min/max ranges, tick counts and precision are now implemented and covered by XYAxes, including actual MDI Cancel/invalid correction/close-reopen and source/report retention, independent rendered positions/clipping/grid/labels and native Wayland tabs. XYExport now covers the active wx data export action, full-precision retained samples, source/output protection, atomic write failure/correction and native widget chooser. wx logarithmic and minor-tick controls are disabled and not applied; see the XY axis/export appendices. |
| `src/gui/subscriber/OrbitViewPanel.hpp` | Active object/draw, camera, frame/up-axis/scale, drawing/star, solver and data controls audited. Grouped Qt setup, ordered/paired visibility Apply, pending/Cancel, validation/rollback, exact Undo/Redo/save/reopen, report invariance, object/vector camera histories and UseInitialView rerun/close/reopen behavior covered. Drawing-only edits retain imported primary-camera metadata. SolverPlots and OptimizerPlots cover DC/Yukon All/Current/None histories, accepted endpoints/camera, replay and native close/reopen. SolverToggle, OptimizerToggle and NestedSolverToggle cover Current-mode targeter/Yukon/nested toggles, suppressed samples and separated resumption; Ground/XY accepted paths match a None-mode reference, with exact reports and native replay/close-reopen. NestedSolverPlots/Cleanup add two-level targeter histories, initialization/trial filtering, accepted camera/replay and failure/Stop recovery. Optimizer/mixed/deeper nested cases and broader camera/frame combinations remain pending. InvalidPlotData now excludes finite unavailable/unrenderable positions from Orbit and primary/alternate camera history; 20 retained valid points, final camera recovery, native replay/close-reopen/rerun and unchanged complete reports are covered. SegmentCameras adds named OF propagation-segment conversion, stored/automatic body/inertial cameras, captured nonzero-attitude endpoint clamp, late data, LookAt, replay, primary override/Undo and invalid-reference/Unicode file recovery with independent complete reports. Native reopened-viewer cascade clipping was reproduced and fixed for new windows; final geometry and rendered controls pass. Repeated/solver/backward/evicted segment regimes remain pending. ConvertedVisibility now preserves independent OF per-object label/trajectory flags alongside standard model/body flags, named reorder/removal metadata, source/file recovery and complete three-spacecraft reports, with native/fallback label-only rendering and latest-pose clipping evidence. ObjectDrawing adds persistent per-object Default/On/Off controls, Cancel/pending/retained Apply, mixed invalid rollback, paired removal, exact Undo/Redo/Unicode recovery, ordinary implicit-default retention and byte-identical reused complete reports. Native controls/scene and corrected adjustable initial columns were inspected; the final isolated layout probe repeats no mission. ConvertedMarkers adds independent center/end markers and pixel sizes, source-ordered conversion defaults, persistent Markers controls, runtime/report/file recovery and native/fallback ring/rose replay/depth checks; final cached shader/lifetime and indexed-size guards have isolated evidence. ConvertedWidths adds fractional named line widths, OF defaults/prefixes, persistent width controls and source/report recovery; native stroke clamping was reproduced and fixed with depth-tested screen-space geometry, with isolated native/fallback near-plane checks. ConvertedFonts adds stored per-object font sizes/placements and persistent Label style controls, source/report/file recovery and native/fallback pixel sizing/four placements. The inspected OF renderer does not consume its stored font fields, so this qualifies Qt behavior rather than identical OF font pixels. Final ignored-extra-prefix metadata guards have isolated evidence. Other OF object decorations remain pending. |
| `src/gui/app/RunScriptFolderDialog.hpp` | Active ResourceTree folder caller and result/error aggregation audited. wx supports starting index/count, repeats, two include/exclude filename filters, output and per-run directories, saved-script copies/re-run, comparison directory/name replacement/tolerance and optional saved comparison results, plus interrupted/build/init/run failure reporting and path/log restoration. Qt Mission / Run scripts from folder implements those operations, with isolated batch viewer/solver windows, preserved document/Undo/normal viewer history and restored engine/path/log state. FolderRunTests covers repeated output/comparison, exact copies and relative includes, failure categories, active/between-run Stop and retry; native Wayland rendered scenes and automatic OF conversion were inspected. Native portal Browse and exceptionally large result display remain unqualified; see the folder-run appendix. |
| `src/gui/subscriber/OpenGlOptionDialog.hpp` | Source modeless option controls and MdiChildTrajFrame caller audited: animation interval/increment, initial view, alternate coordinate system, drawing/colors, object visibility and orbit normals. The only creator is the unused MdiChildTrajFrame; neither that frame nor this dialog is in the current GUI CMake source list, and no caller constructs the frame. Active wx 3D viewers use MdiChild3DViewFrame/OrbitViewCanvas. Qt current camera/display/replay controls have separate evidence; this inactive helper does not qualify remaining active viewer capabilities. |
| `src/gui/subscriber/SubscriberSetupPanel.hpp` | Generic writable subscriber fields, boolean choices, load/save and validation audited. Qt ResourceEditor exposes engine-typed subscriber properties and specialized report/plot/file controls; selected execution, round trips and recovery are covered by plot, dynamic-data, ephemeris and report suites. Remaining subscriber types and generic field combinations remain pending. |
| `src/gui/subscriber/DynamicDataDisplaySetupPanel.hpp` | Active grid resize/retained cells, cell editing/clearing and condition colors audited; grouped Qt setup with pending Apply/Cancel, Undo/Redo, Unicode save/reopen, live reports, adjustable widths and immediate close/reopen covered. Extreme dimensions and unnamed-cell styling remain pending. |
| `src/gui/subscriber/ReportFileSetupPanel.hpp` | Report parameter lists accept numeric array elements; readable delimiter selector, precision/width rejection, exact save/reopen and explicit/automatic report values tested. Append across repeat runs, fixed-width headers, left/right alignment and zero fill tested. Shared ordered parameter selector added; owned attitude and attached tank/thruster browsing tested; broader hardware/plugin types and solver-iteration combinations pending. |
| `src/gui/subscriber/DynamicDataSettingsDialog.hpp` | Active parameter selection, text/background colors and warning/critical bounds audited. Real/string/array-element references, duplicate/whole-array/nonnumeric/reversed-bound rejection, Cancel, alarm/custom colors and calculation-preserving round trips covered. Broader parameter contexts and unnamed-cell styling remain pending. |
| `src/gui/app/WelcomePanel.hpp` | Active GmatMainFrame startup/menu caller audited. Recent scripts, sample navigation, local help/tutorial links and persisted ShowWelcomeOnStart preference exist in wx. Qt Welcome and Recent missions now provide history, sample Browse, in-app local guides/tutorials, configured project/video links, New mission and persisted startup preference. WelcomeTests covers dirty Cancel/Discard, missing recent files, chooser acceptance/Cancel, exact identity/source/Undo, Save As history and independent reports; native Wayland UI and actual launcher preference routing passed. Widget choosers were used; portal and external-browser interaction remain under shared desktop qualification. |
| `src/gui/app/AboutDialog.hpp` | Active Help/About caller audited. Qt AboutDialog delivers actual engine version/bitness/build details, Qt/OSG versions, credits/contact/project links and the exact offline License.txt through read-only InspectionDialog. DesktopTests covers menu opening, Close/Escape, compact layout, pending source/Undo retention and missing/malformed license correction; native Wayland captures inspected. External website/mail launch has not been exercised. |
| `src/gui/app/InteractiveMatlabDialog.hpp` | Uninstantiated legacy interactive dialog audited: sends selected inputs/outputs to CallMatlabFunction, displays results and clears/closes. No current GUI caller creates it. MATLAB is outside the selected Linux runtime; no Qt MATLAB workflow or execution qualification is claimed. |
| `src/gui/app/SetPathDialog.hpp` | wx startup read/write, ordered GMAT/MATLAB function paths, output/log Apply and directory errors audited. Qt Set paths validates full startup imports as pending state, provides full read-only startup preview and atomic export, and uses exact file-manager/global/log rollback. Existing PathTests covers startup/alias/mode/log/source protections, Unicode read/Apply/export, fresh-process reports and execution/pending guards. New PythonPaths distinguishes editable ordered pending Python rows from unchanged live paths: import/export/Cancel/invalid rollback, unrelated startup source retention, other-path Apply and fresh imports with independent results. Private actual input adds reorder/export/import, own source Save/Ctrl+O, 23.5 report and exact module origin after an exported-startup restart; see [ordered Python paths](Qt6ParityValidation/python-module-paths-20261002.md). Optional MATLAB editing, plugin/cached-data hot replacement, this workflow's host/portal choosers and broader startup/storage formats remain unqualified; existing separate private portal evidence is unchanged. |

## Selected runtime plugin inventory

Every row requires real-engine evidence, not just registration.

| Plugin | Configuration / execution / reports / recovery evidence |
| --- | --- |
| `../plugins/libDataInterface` | DataInterfaceTests: GUI input-file selection and format, typed Set target/source and all/seven field subsets, independent epoch/state/Cr and propagated reports, exact Undo/Redo/Unicode save/reopen, Output access, missing/malformed/invalid-epoch/missing-field/unknown-field recovery, Task-9 input and converted shortened shipped OF example covered. Broader bodies/frames, multiple records, repeated imports within one mission and filesystem permission failures remain pending. |
| `../plugins/libEphemPropagator` | Mars Express SPK configured through Qt kernel lists, converted viewer, exact round trips, report/view agreement and missing-clock recovery tested. EphemerisTests adds generated OEM, STK, Code-500 both byte orders and SPK readback, GUI-selected first spacecraft input files and propagator steps, Unicode script round trips, independent circular-orbit states, FromSpacecraft start clamping, after-coverage rejection and missing-file restore/reopen. BinaryEphemerisToggle now checks generated SPK coverage spans for two arcs and every fixed-step state, plugin readback within both arcs, after-coverage failure/reopen recovery, and Code500 continuous readback in both byte orders. Internal SPK reader gap traversal completes under the legacy skip policy and remains unqualified; Code-500 internal Toggle gaps produce mislabeled states in the independent writer reference. Broader frames/bodies, backward/boundary stepping and multiple-kernel coverage cases remain pending. InvalidPlotData qualifies Qt viewer containment of the legacy finite unavailable marker during internal SPK gaps, with separated valid arcs and unchanged reports; numerical gap traversal remains unqualified. |
| `../plugins/libEKF` | KalmanTests: one-hour, noise-free GPS version of the shipped filter/smoother example with SNC process noise and Gauss-Markov drag. Typed run/reference/solve-for controls, owned model settings, warm-start input/output browsing and paired epoch conversion, both continuation boundaries, exact state/covariance CSV equivalence with independent script configuration, report access, pending/Cancel/Undo/Redo/Unicode mission reopen, invalid-edit rollback and missing/malformed/late-seed recovery covered. Native Wayland panels/execution passed. CovarianceTests now qualifies the actual spacecraft initial covariance grid, pending/Cancel and invalid-value recovery, retained Apply, exact Undo/Redo/Unicode save/reopen and independently configured cold-start state/covariance CSV agreement; the native preview confirms layout and full stored precision. PluginCreation now covers actual creation/removal menus for EKF, Smoother, ProcessNoiseModel and EstimatedParameter with exact Undo/Redo and Unicode save/reopen. Full-day/noisy or real data, other measurement/model regimes, residual graphics, prediction, warm-start smoothing and broader malformed/disk cases remain pending. |
| `../plugins/libGmatEstimation` | EstimationTests: Qt tracking path/type table, typed simulator/estimator and station/solve-for lists, observation output selection, typed run commands, noise-free shortened shipped range-skin simulation/batch fit, independent state/observation equivalence, exact Undo/Redo/Unicode script save/reopen, report access, invalid-edit rollback and missing-observation recovery covered. Paired simulator/filter epochs, exact numeric observation boundaries and GUI-configured batch accept/reject frequency thinning and record rejection match independent state and residual edit-flag reports. KalmanTests also covers GPS simulation and concrete RunSmoother serialization, including labels/comments and command edits. Broader measurements, noisy/real data, level-one and other filter regimes, estimator epochs, multiple propagator mappings, pass biases and covariance settings remain pending. |
| `../plugins/libEventLocator` | CompatibilityTests: edited eclipse lists, exact save/Save As/reopen, invalid-type build recovery, eclipse intervals and Output report access. StationTests: GUI-edited station Cartesian/elevation/mask settings, save/reopen, automatic contact intervals and missing-mask recovery covered. EventLocatorTests: grouped configuration, paired epochs, bounded contacts, Transmit/Receive corrections, ISOYD max-elevation and azimuth/elevation/range reports, eclipse intervals, shipped Mercury intrusion and failed-output-directory restore/reopen covered. One actual-input positive FixedGrid configuration/Apply/save-reopen/report passes with a valid owned frame, explicit 2015 noon/300-s coverage and 31 entries/one Sun event; first insufficient-SPK fixture is retained. See [FixedGrid](Qt6ParityValidation/fixed-grid-20261002.md). One actual-input Earth Region/one-spacecraft/no-imager contact passes Regions category paired-table creation, Apply/save-reopen and Output Enter; one 300 s run gives one event lasting 81.477 s bracketed by 31 state samples. See [Region](Qt6ParityValidation/region-contact-20261002.md). Broader hardware/FOV, remaining formats/coverage boundaries and disk-write failures remain pending. |
| `../plugins/libExternalForceModel_py314` | ExternalForceTests: existing force-model module selection from configured Python search paths, Cancel and pending function/exclusion Apply, shortened shipped no-API example with independent internal two-body state agreement, exact Undo/Redo/Unicode save/reopen, missing module/function run failure and recovery, invalid-setting rollback, independently script-configured combined forces and unrelated report edits covered. Owned force serialization now retains module/function/exclusion settings so GUI reconstruction does not drop the contributor. First contributor creation/removal is now covered through the actual retained force-model panel, atomic mixed Apply, invalid input and Cancel, exact source Undo/Redo/Unicode save/reopen and independent script-configured reports; see the external-force creation appendix. Shared ordered Python path editing/export/restart is qualified by a Python call's module-origin readback; see [ordered paths](Qt6ParityValidation/python-module-paths-20261002.md). One actual custom-path ExternalForce after GUI-exported startup restart now passes module/function/exclusion Apply/save/CtrlO/readback, exact module-origin proof and two finite state rows matching a known 60 s constant-acceleration result; see [custom-path force](Qt6ParityValidation/custom-external-force-20261002.md). Full-day/API-dependent examples, packages/environment precedence, modified-module caching, multiple-spacecraft/variational and malformed-callback cases remain pending. |
| `../plugins/libExtraPropagators` | BulirschStoer: step edit, exact save/Save As/reopen, invalid-build recovery, report creation and analytic circular-orbit endpoint. Configured MinimumReduction/MaximumReduction now survive GUI Apply, nested/direct/runtime clones, Undo/Redo and Unicode save/reopen; one actual-input short run/report/summary passes after correcting the two-field copy reset. Deprecated read-only MinimumTolerance stays hidden. See [custom controls](Qt6ParityValidation/bulirsch-controls-20261002.md). Broader settings/regimes remain pending. |
| `../plugins/libFormation` | CompatibilityTests: Add editing/reordering, non-spacecraft rejection, exact save/Save As/reopen and failed-build recovery, both members propagate 60 seconds. PluginCreation adds actual Formation New/Delete menu operations, Cancel, exact unrelated source/Undo/Redo and Unicode save/reopen. Remaining settings/output coverage pending. |
| `../plugins/libGmatFunction` | CompatibilityTests: edited cross-product arguments, exact save/Save As/reopen, invalid-type build recovery, expected numerical cross product. PluginCreation now adds file-backed New resource, new function file chooser/editor Cancel and Save, deletion/Undo/Redo/Unicode reopen and independently expected result 19. FunctionImport covers existing file Browse, missing-path correction, resource/file name aliases and expected result 25 without modifying the imported file. FunctionFileNames adds creator/existing-resource templates for differing file basenames, invalid filename recovery, retained aliases/Apply/Undo/reopen and independently expected pass-through execution. GmatCallSyntax adds configured-function-only empty/bare/scalar source alignment, leading-label retention and independent zero-input/no-output result checks. GlobalScopes now adds Variable/Array/String/Spacecraft shared objects across repeated scalar-input/output calls, local shadow isolation, repeated Global and failed-reference correction/rerun with known outputs and exact source/report/file round trips. One actual-input local whole-Spacecraft/String positional call adds offered/ordered Inputs and Variable/String Outputs, unchanged declared caller mass/text, known 123.5/exact String report and save/reopen; see [local arguments](Qt6ParityValidation/local-function-arguments-20261002.md). Other resource/signature and report combinations remain outside the retained cases. |
| `../plugins/libMsise00` | AtmosphereTests: GUI selection/configuration, constant-flux density response, CSSI observed/predicted and selected Schatten prediction, source-preserving Undo/Redo/save/reopen, 600-second density/trajectory reports and missing-weather-file recovery covered. Broader operating regimes, file contents/coverage boundaries and remaining Schatten modes pending. |
| `../plugins/libNewParameters` | AtmosphereTests: AtmosDensity output from GUI-configured atmosphere models and SPAD drag, density/trajectory agreement with independently script-configured missions and save/reopen covered. Density unit metadata corrected to kg/km^3 without changing values. Other parameters and contexts pending qualification. |
| `../plugins/libPolyhedronGravity` | PolyhedronTests: existing contributor body/input-shape selection, density units, chooser Cancel and pending Apply, independent closed-cube far-field mass check and script state agreement, paired body/path configuration, exact Undo/Redo/Unicode save/reopen, SurfaceHeight parameter browser and report access, invalid density/body/missing/malformed shape rollback and recovery, CRLF/tabs/no final newline/decorative labels, relative paths and unrelated resource editing covered. Duplicate force serialization fixed; checked loader preserves valid record ordering and rejects malformed connectivity/geometry. First and multiple contributor creation/removal through the actual retained force panel, pending/Cancel/Help, typed body/shape/density validation, malformed-shape rollback/correction, legacy creator/alias conversion with implicit density retained, exact Undo/Redo/Unicode save/reopen and independent script-reference Earth/Mars reports are now covered; see the creation appendix. Real asteroid meshes, custom bodies, multiple spacecraft, variational/precision propagation, geometric self-intersections and broader SurfaceHeight numerical semantics remain pending. |
| `../plugins/libProductionPropagators` | PrinceDormand853: step edit, exact save/Save As/reopen, invalid-build recovery, report creation and analytic circular-orbit endpoint. Remaining cases pending. |
| `../plugins/libPythonInterface_py314` | Shipped Python example: exact save/Save As/reopen and failed-build recovery, independently computed cross-product result and report. PythonCalls adds actual MDI module/function and ordered arguments, labeled scalar-from-array and no-output string execution, lookup failure/correction/reopen, and context Help; native Wayland controls inspected. Empty/bare-input and unbracketed single-output calls now retain their source and runtime results; see the Python syntax appendix. Shared Python module path Add/Replace/Remove/Up/Down/Browse, pending import/export/rollback and fresh known-priority imports are now covered; private actual input confirms the selected module origin after restart. See [ordered Python paths](Qt6ParityValidation/python-module-paths-20261002.md). Package syntax, other return/error types, arbitrary environment precedence and changed-module cache recovery remain unqualified. |
| `../plugins/libSaveCommand` | CompatibilityTests: edited object list, exact save/Save As/reopen, runtime spacecraft/variable export and reload, repeat-run replacement, loop snapshots, bad-path and disk-write recovery. Remaining resource types and multi-snapshot reimport pending. |
| `../plugins/libScriptTools` | ScriptEditingTests: the sole registered CommandEcho command has typed On/Off editing, pending Apply, label/comment preservation, exact Undo/Redo/Unicode save/reopen, bounded execution tracing, independently checked 2/5 reports, invalid-edit rollback and initially disabled/enabled RunComplete state restoration. Its generator now retains the terminating semicolon so the GUI can locate/edit it. One stopped While/If case, post-echo file-write failure, initially on/off settings, unexecuted/repeated cleanup and configuration-preserving clones are covered; cleanup/copy defects corrected. Broader completed nested/loop and argument-validation cases remain pending. |
| `../plugins/libStation` | StationTests: GUI location/elevation/ID/colors/mask configuration, physical position and source-preserving Undo/Redo/save/reopen; script-reference contact intervals for baseline, elevation 25 degrees and bundled mask, mask clear and missing-file restore/reopen recovery covered. PluginCreation adds GroundStation New/Delete menu operations, Cancel, exact unrelated source/Undo/Redo and Unicode save/reopen. Hardware, measurement/media/error-model settings and broader bodies remain pending. |
| `../plugins/libThrustFile` | ThrustFileTests: history creation and input selection, typed segment/tank/solve-for lists and clear/restore, pending/Cancel angle and sigma vector resizing, Begin/EndFileThrust selectors, independent state reports, analytic scaled fuel depletion and post-End coast, exact Undo/Redo/Unicode Save/Save As/reopen, wrong-reference rollback, missing/malformed-file recovery and Output access covered. All four data formats with None/Linear interpolation, relative-file Build/Apply/reopen and the full-day bundled example covered. Multiple spacecraft/segments, cubic interpolation, time-varying angles, estimator solve-fors and file-boundary regimes remain pending. |
| `../plugins/thinksys/libTLEPropagator` | Shipped example: step edit, exact save/Save As/reopen, report epoch/state, sampling invariance, invalid-build and missing-file recovery. Broader settings/reference ephemeris comparison pending. |
| `../plugins/libYukonOptimizer` | CompatibilityTests: shipped algebraic optimization, exact save/Save As/reopen, invalid-type build recovery, analytic optimum and report. OptimizerPlots adds All/Current/None Orbit/Ground/XY modes, pending Apply/Undo/reopen, independently known quadratic optimum, exact display-independent state/geodetic reports, accepted camera/replay and native viewer close/reopen. Constraints adds the actual command controls, five fixed-bound scalar/array/property cases with analytic optima and byte-identical script-reference iteration reports, source/Undo/Redo/Unicode Save/Save As/reopen, operand rollback/recovery and native Wayland captures. SolverOperands adds selected array Vary/initial/options and Minimize with scale factors/mixed bounds, optimum 3, complete independent iteration reports, invalid-index/bound correction/rerun and exact source/file round trips. A varying-right bound with literal left produces NaN in the independent script before any GUI edit; that regime and additional settings/error modes remain unqualified. |

## Implementation checkpoint 1

Added command controls for For, If/While, assignments, Toggle and Global/Clear.
Header edits preserve nested source. Actual engine-normalized If headers omit
semicolons; both forms are covered. Tests execute changed loop/condition values
and verify Undo restores the original numerical result.

Array grids now resize: retained cells survive, new cells are zero, shrinking
removes cropped cells only from the dialog until accepted. Apply reconstructs
and validates the mission; Undo restores dimensions and values. Non-array
vector/matrix dimensions remain fixed. Expression cells are still pending.

All 11 Qt tests passed (18.70 seconds), including the new workflow assertions;
see `Qt6ParityValidation/check-qualification-1.txt`. This checkpoint does not
close any of the three requested qualification areas.

## Plugin/file checkpoint

The function, optimizer and eclipse fixtures now save, Save As to a Unicode
filename, deliberately fail interpretation, reopen and rebuild the saved script,
and only then run their numerical/report checks. Both saved files must match
the edited script exactly. The targeted compatibility test passed; see
`Qt6ParityValidation/plugin-roundtrip.txt`. This establishes these workflows,
not qualification of the remaining plugin inventory.

## Reference-picker and plugin checkpoint

Resource properties now offer file browsing and engine-typed scalar/list
reference selection. Ordered lists retain current members first (including
existing names absent from the offered set), support drag reordering and do not
apply until Apply. Tests cover tank order/mixture ratios, Cancel, pending state,
EarthFixed selection and choosing a new output filename without creating it.
Free text remains available for parameter expressions and plugin-defined types.

Qualification exposed the missing Formation.Add editor. It now accepts only
existing spacecraft and supports reconstruction through the normal undoable
Apply path. The compatibility test verifies member reordering, rejection of a
propagator as a member, exact file round trips and actual 60-second propagation
of both spacecraft. The shipped Python interface example also runs after file
round trips/recovery, with an independently calculated cross product and report.
These results expand evidence; the remaining plugin and viewer gates stay open.

The rebuilt Linux application passed all 11 tests in 19.07 seconds; evidence:
`Qt6ParityValidation/check-qualification-2.txt`.

## XY viewer control checkpoint

Qt now exposes its implemented XY styles through a Style dialog. Each curve has
its own controls, and changing the selected curve retains other pending edits.
OK applies; Cancel changes neither model nor pixels. Color/marker changes update
existing samples as well as the curve defaults. Styles are session-local and
are reset by rebuilding the mission; the dialog states that scope explicitly.

PlotTests verifies canceled edits preserve the captured image, accepted edits
change the image and selected curve, and another curve stays unchanged. The
same fixture runs under software-native and HiDPI configurations. Source audit
also confirmed wx ignores the index argument for ChangeWidth/ChangeStyle
(`GuiPlotReceiver.cpp`); Qt's whole-curve behavior matches that baseline.
Camera projection, multiple views and OF-relative camera conversion remain open.

All 11 tests passed in 19.53 seconds and GmatQt was rebuilt; evidence:
`Qt6ParityValidation/check-xy-style.txt`.

## Perspective camera checkpoint

Added interactive perspective projection and vertical FOV controls while keeping
orthographic as the default. Native rendering uses a perspective frustum;
fallback projection applies depth scaling and rejects objects behind the eye.
The fallback pan offset uses the camera plane, and native labels use clip-space
bounds rather than drawing labels behind the camera. Perspective sky geometry
uses the camera FOV/zoom and remains translation invariant.

Native tests cover FOV framing, depth-dependent body size, behind-camera
clipping, exact orthographic restoration and star translation/zoom behavior.
PlotTests exercises the actual projection/FOV controls and rendered changes in
both fallback and native/HiDPI configurations. OF FOV import, persistent camera
settings, multiple views and relative camera modes are still open; interactive
projection alone does not satisfy those conversion requirements.

All 11 tests passed in 20.90 seconds; GmatQt was rebuilt. Evidence:
`Qt6ParityValidation/check-perspective.txt`.

## OpenFrames projection import checkpoint

The converter now imports the first selected OF view's perspective projection
and FOVy (default 45 degrees), preserving fractional angles. Settings are stored
in a `% GMAT-Qt-Camera` JSON comment, so the base engine can still interpret the
converted calculations. Qt validates plot names, booleans, finite FOV from 1 to
150 degrees and duplicate directives before using them. Invalid or unsupported
FOV is rejected rather than clamped during conversion.

Receiver-created orbit models get these settings before their widgets are
created. GUI reconstruction retains directives when engine serialization omits
them. Tests cover defaults, fractional FOV, invalid values, duplicate directives,
the real Hohmann renderer model, resource edit/Undo, and save/reopen/run. This
closes basic OF perspective/FOV import, but not multiple-view selection, relative
orientation, or saving camera changes made with the interactive controls.

Rebuilt GmatQt and passed all 11 tests in 20.59 seconds; evidence:
`Qt6ParityValidation/check-fov-import.txt`.

## Interactive projection persistence checkpoint

OrbitView now has Keep projection. It adds/replaces that plot's validated camera
comment as one undoable editor change and updates the built-source snapshot,
without interpreting the mission or deleting the viewer during its callback.
The file is not written until the normal Save action. Running missions, unbuilt
script edits, pending panel edits and missing/non-OrbitView resources are rejected.

WorkflowTests exercises the actual toolbar action, fractional FOV, switching to
orthographic, repeated clicks without duplicate directives, unchanged mission
sequence, unchanged engine object identity, Undo/Redo, protection of unbuilt
edits, and save/reopen/run restoration. This closes projection/FOV persistence;
manual orbit angles/pan/zoom, multiple views and relative camera modes are still
outstanding, alongside the other workflow and plugin qualification gates.

GmatQt rebuilt; all 11 tests passed in 20.17 seconds. Evidence:
`Qt6ParityValidation/check-keep-projection.txt`.

## Expression-cell checkpoint

Added an Expressions grid to Array resources. General formulas are GMAT mission
commands, so new formula blocks initialize cells at mission start in row order;
the dialog states this timing explicitly. Numeric initial values stay separate.
Formula blocks preserve text across Apply, Undo/Redo and save/reopen. Existing
mission assignments remain untouched. Unrecognized statements inside a managed
block are refused instead of erased; invalid cells/formulas restore the previous
mission. Resource editing now preserves the original mission section verbatim.

WorkflowTests opens the grid, exercises Cancel and Apply, evaluates dependent
cells to an independently expected result, verifies Undo/Redo, unrelated resource
editing, save/reopen and reopened grid text, clears formulas and undoes that,
and verifies extra commands/unknown references are rejected without changing
source or runtime results. Numeric and expression edits currently require
separate Apply operations; arbitrary existing assignments use Mission editing.
The remaining specialized-editor, multi-camera and plugin gates stay open.

GmatQt rebuilt; all 11 tests passed in 21.53 seconds. Evidence:
`Qt6ParityValidation/check-array-expressions.txt`.


## Propagator plugin checkpoint

Resource panels now enumerate writable fields of a PropSetup's owned propagator,
including plugin settings hidden by the parent forwarding table. Edits target the
owned object. Resource serialization excludes the separately written force model
when matching propagator blocks; previously numerical-propagator Apply could fail
with the specialized-editor message despite having a supported property.

CompatibilityTests runs the shipped TLE mission, checks a one-day report span,
finite low-Earth-orbit state and matching spacecraft epoch, edits sampling from
300 to 120 seconds, verifies endpoint invariance, and recovers from a missing TLE
file without changing the result. BulirschStoer and PrinceDormand853 each undergo
step editing, exact save/Save As/reopen and invalid-build recovery, then propagate
a circular point-mass Earth orbit against an analytic endpoint within 0.1 metre.
Both produce reports. These checks qualify these scenarios, not every plugin
option or propagation regime. Specialized workflows and other inventory gaps
remain open.

The user's GmatQt executable was rebuilt. All 11 Qt tests passed in 21.91 seconds.
Evidence: `Qt6ParityValidation/check-plugin-propagators.txt`.


## Camera roll checkpoint

OF conversion now preserves the selected view's arbitrary up vector in validated
Qt camera metadata. The receiver applies it in ViewUpCoordinateSystem to every
camera-history sample, so playback uses the same roll as propagation. A nearest
standard axis remains in the script as a base-viewer fallback. Invalid and zero
vectors are rejected instead of approximated. Keep projection preserves the
vector; an explicit resource-panel ViewUpAxis edit removes that override.

WorkflowTests covers exact non-axis conversion, invalid metadata, real Hohmann
propagation with an arbitrary up vector at the first and last camera frames,
resource-edit retention, save/reopen, Keep projection retention, explicit axis
override, and Undo restoring the imported vector. Multiple-view switching and
body-/trajectory-relative orientation remain pending; this closes the arbitrary
up-vector approximation only.

GmatQt rebuilt; all 11 Qt tests passed in 31.38 seconds. Evidence:
`Qt6ParityValidation/check-camera-up.txt`.


## Multiple-camera checkpoint

Conversion now retains the full ordered OF View list as a first standard
OrbitView camera plus named Qt camera definitions. Each additional view has
projection/FOV, eye/center/up vectors, reference and target objects. Qt validates
all views and references before execution. The receiver records bounded histories
for each view and clears them on mission/solver reset. The camera selector uses
those histories for replay and does not reinterpret or rerun the mission.
Keep projection edits the selected view only, preserving the others.

WorkflowTests covers stored Current camera vectors, malformed secondary views,
the shipped Hohmann three-view list, per-frame Earth/spacecraft tracking,
replay basis changes, unchanged mission object identity/frame count on selection,
independent projection settings, save/reopen, invalid-reference build rejection
and recovery. PlotTests verifies visible switching and exact restoration in both
fallback and native/HiDPI renderers, plus history cleanup. Existing resource edits
and Undo retain the full camera metadata.

This implements named view switching and translation tracking. Body-relative
rotation, trajectory/segment views and original automatic framing remain open.
The current unstored-location fallback is explicitly reported as 30000 km; it is
not counted as OF automatic-framing parity. Base viewer compatibility retains
the first camera; additional definitions are Qt comments.

The user executable was rebuilt; all 11 Qt tests passed in 22.99 seconds.
Evidence: `Qt6ParityValidation/check-multiple-cameras.txt`.


## Body-relative camera checkpoint

Primary and additional OF views now retain the InertialFrame mode. With that
setting Off and ViewTrajectory Off, their eye offsets, center offsets and up
vectors rotate with the reference object's attitude into the plot coordinate
system. Celestial and spacecraft attitude conventions are handled separately.
InertialFrame On retains position tracking with plot-frame axes. Primary center
offsets are retained, and an explicit ViewDirection edit clears that override.
Validation rejects incompatible primary reference/eye types before execution.

WorkflowTests checks the primary Earth camera against independent EarthFixed
coordinate conversions, including nonzero center and non-axis up vectors at the
first and last samples, both in inertial and rotating plot frames. Additional
Earth and spinning-spacecraft views match the recorded body orientation and
position histories. Save/reopen retains the poses; incompatible reference edits
are rejected and the previous mission reruns unchanged. Conversion also verifies
InertialFrame On and rejects malformed primary stored vectors.

Trajectory/segment views, two-frame look-at ShortestAngle/AZEL orientation and
original automatic framing remain pending. These cases retain explicit conversion
notes and are not counted as completed parity.

The user executable was rebuilt; all 11 Qt tests passed in 24.63 seconds.
Evidence: `Qt6ParityValidation/check-body-cameras.txt`.


## Two-frame look-at checkpoint

LookAtFrame now imports OF's full camera-pose alignment instead of setting only
the target position. ShortestAngle On maps to direct shortest rotation; Off maps
to azimuth then elevation, with OF's singular-case threshold. Both primary and
additional views combine this alignment with their body-relative/absolute frame.
The stored center remains local to the reference frame. Explicit ViewDirection
editing clears the primary alignment; Undo restores it. Mode values and metadata
booleans are validated rather than silently interpreted as Off.

The optional OpenFramesCameraReference probe compares the shared Qt alignment
helper to actual OpenFrames FollowingTrackball transforms: 48 frame/mode/direction
cases, four points each, including coincident origins, near-pole cases and reverse
directions. Maximum point error was 2.06801e-12 scene units. The reference library
is `/home/dan/GIT/OpenFramesInterface/dep/OpenFrames-git/installed/lib/libOpenFrames.so`;
the source checkout HEAD inspected was `037643090d42c3ff804584f69b13b3b6511a8b66`.
This probe adds no OpenFrames dependency to the Qt application.

WorkflowTests exercises primary and additional Earth/spacecraft look-at views
in both modes, checks complete eye/center orientation and perpendicular up vectors
at the first and last samples, confirms the modes produce different roll, and
verifies save/reopen, explicit-direction override and Undo. Trajectory/segment
views and original automatic framing remain pending, along with the other
workflow/plugin qualification gates.

The user executable was rebuilt; all 11 Qt tests passed in 25.27 seconds.
Evidence: `Qt6ParityValidation/check-look-at.txt` and
`Qt6ParityValidation/look-at-reference.txt`.


## Save-command workflow/plugin checkpoint

Qt exposes Save in the command templates when its plugin is registered and edits
its object list through CommandForm. Actual export qualification found two plugin
defects: stream failures were ignored, and copied Save commands left fileArray
uninitialized. Save now initializes copied stream ownership, disposes prior
streams before reinitialization/assignment, checks open/flush/close results, and
closes streams on exceptions. Export restores original object comment flags even
when serialization fails. These fixes apply to the shared Save plugin, including
its use outside Qt.

CompatibilityTests edits a Save command to export a spacecraft and variable,
saves/reopens the mission exactly, executes it, then opens the exported resource
file through Qt and checks current numeric values. Repeated runs replace previous
output; a loop retains both snapshots. A nonexistent output path must fail, then
recover to identical output. On Linux, a temporary symlink to /dev/full verifies
post-open write failure and recovery. The fixture only modifies temporary outputs.
Broader resource-type export and multi-snapshot reimport remain pending.

GmatQt and the Save plugin were rebuilt; all 11 Qt tests passed in 26.55 seconds.
Evidence: `Qt6ParityValidation/check-save-command.txt`.


## Coordinate-system workflow checkpoint

The audit found that CoordinateSystem::SetStringParameter("Axes", ...) accepts
text without replacing the owned axes, and the generic Qt editor omitted owned
axis properties. Qt now creates/replaces the owned AxisSystem on a proposal clone,
exposes its writable fields and retains built-in protection. A dedicated Axes
dialog lets type and dependent fields be edited together before applying; a type
change rebuilds the pending form, and Cancel does not touch the configured model.
Primary/secondary/reference objects get pickers and R/V/N directions get choices.

As in the wx panel, ObjectReferenced changes require distinct primary/secondary
objects and exactly two different directions. This is checked before accepting
the edit because engine interpretation alone can defer geometry errors until an
actual coordinate conversion. WorkflowTests covers creation, dialog Cancel,
changing to ObjectReferenced with dependent fields, type-only MJ2000Ec edits,
owned field exposure, invalid directions, protected built-ins and Undo. The edited
R/N frame transforms the propagated spacecraft position to [radius,0,0], and that
numerical result survives save/reopen and another mission run.

The three wx coordinate-panel inventory rows are now partially audited. Additional
origin/dependency combinations and specialized axis modes remain pending and are
not counted as fully qualified.

GmatQt was rebuilt; all 11 Qt tests passed in 25.49 seconds. Evidence:
`Qt6ParityValidation/check-coordinate-axes.txt`.

### Epoch and constrained-frame follow-up

WorkflowTests edits MOEEq's owned Epoch in A1ModJulian, verifies that its orientation
is fixed across evaluation times, preserves vector length, and changes when the
configured epoch changes. Save/reopen preserves the calculated transform.

A LocalAlignedConstrained frame uses Sun as its alignment reference and
EarthMJ2000Eq as its constraint frame. Its transformed X direction matches the
independently calculated Earth-to-Sun unit vector; Z matches inertial Z projected
perpendicular to that direction (component tolerance 1e-10). The same checks pass
after save/reopen and another run. Reference choices are checked as well.

Qt now rejects static degenerate geometry before applying an edit: zero or
nonfinite vectors, parallel alignment/constraint vectors, reference equal to
origin, and self-referencing constraint frames. Rejected edits preserve the script.
The vector thresholds match the engine's 1e-9 geometry tolerance. Other epoch
models, indirect dependency cycles, non-Earth origins and time-varying constraint
singularities remain unqualified.

GmatQt was rebuilt; all 11 Qt tests passed in 26.08 seconds. Evidence:
`Qt6ParityValidation/check-constrained-axes.txt`.

## Report configuration checkpoint

The wx ReportFileSetupPanel audit found a Qt list parsing gap: array elements
were split at their internal comma and rejected. ReportFile Add now accepts
positive numeric array indices and the selection list preserves those entries.
The engine still resolves parameters and checks array bounds during candidate
interpretation. Delimiters now have readable labels, map back to literal
characters, and reject values the engine would silently truncate.

CompatibilityTests verifies array-element list edits and order, no initial dirty
state in the delimiter control, Tab selection, invalid setting rollback, exact
save/Save As/reopen with invalid-build recovery, and numerical output with
precision 6 and comma/tab delimiters. Automatic reporting during propagation
checks that edited Add order and array values reach the file; explicit Report
commands retain their own parameter order. Full parameter selection, append,
fixed-width formatting and solver-iteration combinations remain pending.

GmatQt was rebuilt; all 11 Qt tests passed in 49.81 seconds. Evidence:
`Qt6ParityValidation/check-report-settings.txt`.


### Report formatting and output lifecycle follow-up

CompatibilityTests now exercises fixed-width reports after settings edits and
save/reopen: width 18 plus GMAT's three separator spaces, headers, left/right
alignment, precision 6 and zero-filled significant digits. Checks compare complete
lines, including padding, against expected values. An ordinary second run replaces
the file exactly; enabling AppendToExistingFile preserves the prior file through
configuration/reopen and adds one complete report (including headers) per run.
A missing file is recreated correctly in append mode.

The Output report viewer now includes the full output path in its window title,
as wx does. Tests open the generated report through Output, compare all displayed
text with the file, verify read-only/no-wrap behavior and text selection, close it,
and verify that a missing report produces a diagnostic without an empty viewer.
At this checkpoint, reports still had a 16 MiB preview limit; the later paging
milestone below removes that access limit. Clipboard contents are not changed by tests.

GmatQt was rebuilt; all 11 Qt tests passed in 28.06 seconds. Evidence:
`Qt6ParityValidation/check-report-lifecycle.txt`.


### Shared report parameter selector

The Report command now offers a configured ReportFile picker and an ordered
parameter dialog. ReportFile Add uses the same dialog. It lists configured
reportable parameters, accepts typed references, adds whole arrays or individual
cells with bounds taken from the array, and supports removal, Up/Down and drag
reordering. The dialog edits a local selection; Cancel leaves its caller unchanged.
Command Apply continues to validate references through the engine.

CompatibilityTests exercises the actual command picker and dialog, array bounds,
remove/add/reorder, Cancel, exact label/comment preservation, application to the
mission and numerical results after save/reopen. It also verifies the resource
panel opens the shared dialog without splitting indexed parameters and receives
the reordered list. Property references not already configured can be entered,
but this checkpoint did not yet include object/property/dependency browsing (added
in the follow-up below) or qualify the full ParameterSelectDialog workflow. Drag ordering uses Qt's standard
InternalMove implementation; the executable test uses explicit Up/Down controls.

All 11 Qt checks passed in 27.49 seconds. After wrapping the selector's help text
for a reasonable dialog width, GmatQt was rebuilt and Compatibility passed again
in 2.39 seconds. Evidence: `Qt6ParityValidation/check-report-picker.txt` and
`Qt6ParityValidation/report-picker-final.txt`.


### Report object/property browser

The shared selector now builds object/property choices from ParameterInfo's
reportable types and registered owner objects. Independent properties generate
Owner.Property references. Coordinate-system, central-body and force-model
properties expose the corresponding dependency selection. Coordinate choices
honor metadata requirements for BodyFixed axes and celestial-body origins. Use
reference fills the editable parameter entry; Add parameter commits it only to
the dialog's local list. Browsing does not reconstruct or mutate the mission.

CompatibilityTests selects BrowserSat.EarthMJ2000Eq.X, BrowserSat.Earth.RMAG and
BrowserSat.ElapsedSecs using the browser controls, applies the resulting Report
command and verifies [7000,7000,0] after exact save/Save As/reopen and invalid-build
recovery. PlanetodeticLAT choices include a user-created Earth BodyFixed frame and
exclude EarthMJ2000Eq. Force-model dependency choices are implemented but their
numerical scenarios remain pending, as do owned/attached hardware parameters and
broader plugin-property coverage.

GmatQt was rebuilt; all 11 Qt checks passed in 28.72 seconds. Evidence:
`Qt6ParityValidation/check-report-property-browser.txt`.


### Attached hardware and owned attitude follow-up

The property browser now includes attached hardware and owned attitude parameter
types. Attached dependencies are filtered by both engine type and the selected
owner's direct object-reference fields. Transitive references from a thruster do
not establish spacecraft attachment. Missing hardware (or unknown required type)
leaves Use reference disabled. Owned attitude properties generate Owner.Property
without an unnecessary attitude object name.

CompatibilityTests selects BrowserSat.Q4, BrowserSat.BrowserTank.FuelMass and
BrowserSat.BrowserThruster.C1 alongside position/time parameters. It checks the
expected unit scalar quaternion, fuel mass 123.5 and coefficient 12.5 in the
actual report after save/Save As/reopen and invalid-build recovery. Spare tank
and thruster resources are excluded; switching to an unattached spacecraft
clears the dependency list and disables reference generation. Broader attitude
models, power systems, plates, electric thrusters and plugin hardware scenarios
remain unqualified.

All 11 Qt checks passed in 58.32 seconds. After the unknown-hardware-type guard,
GmatQt was rebuilt and Compatibility passed again in 2.25 seconds. Evidence:
`Qt6ParityValidation/check-report-hardware-browser.txt` and
`Qt6ParityValidation/report-hardware-browser-final.txt`.


### Force-model rate browser and engine corrections

Force-model parameter testing exposed an internal/script name mismatch: the
SMADot constructor registered EquinoctialSMADot, while the factory and dependency
metadata use SMADot. The constructor now uses the public factory name, allowing
the browser to offer a valid property and populate force-model choices.

The next numerical check failed because OrbitData returned zero TLONGDot when
perturbing acceleration was exactly zero. The shared engine now initializes that
rate to |r cross v|/r^2 in degrees/second; the existing perturbed calculation remains
in place. This corrects an engine result, not merely Qt display behavior.

CompatibilityTests selects SMADot and TLONGDot with RateForces through the browser,
applies the Report command, saves/reopens with invalid-build recovery and runs an
Earth point-mass mission. At radius 7000 km it checks zero semimajor-axis rate and
true-longitude rate v/r for circular motion, then edits tangential speed to 1.1
times circular speed and repeats the round trip and numerical checks. Rate
component tolerances are 1e-10 for SMADot and 1e-12 degrees/second for TLONGDot.
Perturbed-rate scenarios and other force-model rate components remain pending.

The original zero-rate failure is recorded in
`Qt6ParityValidation/force-rates-before.txt`. GmatQt and the shared engine were
rebuilt; all 11 Qt checks passed in 27.23 seconds. Evidence:
`Qt6ParityValidation/check-force-rates.txt`. This is targeted numerical and Qt
regression evidence, not a complete requalification of the shared engine.


## Typed command resource selectors

CommandForm now provides Select controls for impulsive/finite burns, Maneuver's
spacecraft, solvers, event locators and functions. Choices are filtered by engine
type: Target/Achieve use BoundaryValueSolver, Optimize/Minimize/constraints use
Optimizer, and Vary accepts both. Finite-burn spacecraft fields initially remained
text-only; the follow-up below verifies the engine single-spacecraft restriction
and adds their picker.

MissionTests uses the actual modal pickers to change an impulsive burn and its
spacecraft, checks Cancel plus label/comment preservation, applies the command,
saves/reopens it and verifies a 0.01 km/s inertial X velocity increment on the
selected spacecraft. It checks inclusion/exclusion for finite burns, target and
optimization solvers, Vary, constraints and eclipse locators. These selector
checks do not by themselves qualify every command's options or execution path.

GmatQt was rebuilt; all 11 Qt checks passed in 41.87 seconds. Evidence:
`Qt6ParityValidation/check-command-pickers.txt`.


### Finite-burn spacecraft and Backprop controls

BeginFiniteBurn::SetRefObjectName explicitly rejects multiple spacecraft. Qt now
provides a single-spacecraft selector in both BeginFiniteBurn and EndFiniteBurn,
consistent with that engine restriction. CompatibilityTests selects BurnSat for
both commands through the dialogs, saves/reopens, and runs ten seconds of constant
100 N thrust at Isp 300 seconds followed by ten seconds of coast. Tank mass matches
150 - thrust*time/(Isp*g0) within 1e-7 kg, and changes by less than 1e-10 kg after
EndFiniteBurn. The configured thruster supplies g0 for the independent mass-flow
calculation. Other thruster/tank models and coupled finite-burn workflows remain
pending.

Maneuver now exposes wx's Backprop control. It inserts/removes the keyword in its
own source span, retaining labels, resource fields and surrounding text. Equal
source offsets apply field replacement before keyword insertion. MissionTests
checks forward and backward 0.01 km/s inertial X maneuvers through save/reopen,
recognition of an existing BackProp keyword and disabling it without damaging the
command. Non-inertial frames and mass-decrement/backward-mass behavior remain
unqualified.

GmatQt was rebuilt; all 11 Qt checks passed in 50.28 seconds. Evidence:
`Qt6ParityValidation/check-burn-controls.txt`.


## Toggle output selection and execution

Toggle now provides a subscriber checklist and an On/Off dropdown. Existing
selection/order is retained; additional configured subscribers can be checked and
rows dragged. Non-subscriber resources are excluded, empty selection disables OK,
and Cancel discards pending checklist changes. The dropdown updates only the state
source span, retaining the command's other text.

CompatibilityTests edits both Toggle commands through the controls, selects two
ReportFiles, checks exclusion of spacecraft, empty-selection rejection and Cancel,
then saves/reopens with invalid-build recovery. Both reports remain silent for
the first ten-second propagation and record the same interval after reactivation,
ending at twenty seconds total (epoch tolerance 1e-6 seconds). ElapsedSecs stop
conditions are relative to each Propagate command; the second command requests
ten additional seconds. The finite-burn coast fixture was aligned to the same
semantics so its documented ten-second coast is now exactly what it requests.
Plot/ephemeris subscribers and Toggle inside solver loops remain pending.

GmatQt was rebuilt; all 11 Qt checks passed in 27.66 seconds. Evidence:
`Qt6ParityValidation/check-toggle-controls.txt`.


## Object-management selection

Global, Clear and Save now use the shared command checklist, with configured
objects and user Variable/Array/String resources. Computed system parameters are
not offered as objects. Save/Clear can offer automatic globals; Global excludes
them from new choices and explains why, matching ManageObjectPanel's distinction.
Existing source selections remain available so opening a dialog cannot silently
remove a name. Empty selection disables OK and Cancel preserves prior fields.

CompatibilityTests selects SavedSat/SavedNumber through the Save dialog and then
runs the existing numerical export, exact save/reopen, export reimport, repeat-run,
loop snapshot, invalid-output-path and disk-write recovery checks. It checks that
Global excludes Earth/EarthMJ2000Eq while allowing a user variable, and that Clear
Cancel preserves its object list. Global/Clear runtime scope/dependency behavior
still requires separate qualification; checklist coverage alone is not counted as
full command parity.

GmatQt was rebuilt; all 11 Qt checks passed in 27.96 seconds. Evidence:
`Qt6ParityValidation/check-object-pickers.txt`.

## Plot playback controls and narrow-window layout

OrbitView and GroundTrack now expose Start, Play/Pause, Latest, a position label,
and 0.25x–4x replay speed. Resuming keeps the paused position; playing at the end
restarts retained history. Scrubbing pauses playback and Latest restores following
incoming data. The timeline has its own full-width row rather than disappearing
into toolbar overflow when plots are tiled. Replay speed is relative to a nominal
three-second sweep of retained history, not simulation-clock playback.

PlotTests verifies pause/resume, rewind/latest, end-of-history stopping, scrubbing,
4x and fractional 0.25x advancement, retained-history eviction/reset, and visible,
in-bounds timeline/speed controls in a 330-pixel-wide plot. These checks run in
native X11 and offscreen/HiDPI configurations. This does not qualify remaining
OpenFrames trajectory/segment cameras or actual Intel/Wayland stability.

GmatQt was rebuilt; all 11 Qt checks passed in 50.01 seconds. Evidence:
`Qt6ParityValidation/check-playback-controls.txt`. A separate native tiled-window
capture confirms the timeline and speed selector remain visible beside the plots.

## Chart text in tiled windows

The XY/ground-track painter now measures legend entries and wraps them into rows,
reserving chart space below. Long titles and horizontal axis labels elide rather
than clipping through window edges. Legend rows are bounded to retain plot area;
when entries exceed that budget, a remaining-entry count replaces the last cell.
Hover text preserves full titles, axis labels and visible curve names.

The native six-window PlotTests capture was inspected at 1280x850: the narrow XY
window shows both complete legend entries on separate rows and a cleanly shortened
title, while orbit and ground-track playback controls remain visible. This is a
chart-layout improvement, not qualification of all plot interaction workflows.
GmatQt was rebuilt and all 11 Qt checks passed in 25.89 seconds. Evidence:
`Qt6ParityValidation/check-chart-layout.txt`.

## Script Find/Replace workflow

Audited wx FindReplaceDialog's next/previous, replacement, Replace All and history
controls. Qt now provides these through the Edit menu and standard shortcuts,
with optional case/whole-word matching. Searching brings the Script window forward
and retains the dialog for subsequent searches. No-match feedback is nonblocking;
replacement checks the script's read-only state before modifying text.

WorkflowTests opens the integrated menu action, verifies forward/backward wrap,
case and word boundaries, selected replacement, history retention, no-match
feedback and read-only protection. Replacing Sat with SatSat proves Replace All
does not repeatedly replace its own insertions; one Undo restores all original
text. Search itself does not normalize or reconstruct script source.

GmatQt was rebuilt and all 11 Qt checks passed in 28.73 seconds. Evidence:
`Qt6ParityValidation/check-find-replace.txt`. The broader editor audit (including
other editing/navigation preferences) remains separate from this completed search
workflow.

## Resource input/output file browsing

The generic filename picker previously used AnyFile for every field. Resource
metadata now distinguishes known input files from output destinations: inputs use
ExistingFile/Open and outputs use AnyFile/Save. Output browsing itself does not
write a file and does not present an overwrite confirmation for an operation it
is not performing. Unclassified plugin fields retain the prior general picker.

WorkflowTests checks the ReportFile output mode and a new path with spaces without
creating it. Spacecraft ModelFile browsing checks existing-input mode, Cancel,
selection of a path with spaces, and deferred Apply (the live spacecraft remains
unchanged). These checks qualify file selection only; function-source editing,
SPICE kernel lists and broader spacecraft model configuration remain pending.

GmatQt was rebuilt; all 11 Qt checks passed in 27.66 seconds. Evidence:
`Qt6ParityValidation/check-file-pickers.txt`.

## Structured mission conditions

Audited wx ConditionPanel's comparison columns and logical joins. Qt now provides
a local condition dialog with left/right operands, relation and AND/OR controls,
add/remove rows and a shared parameter browser. Numeric values (including signed
scientific notation), parameter references and positive numeric array indices are
recognized. Unsupported grouped/expression syntax stays available verbatim in the
text field rather than being silently transformed. Table columns are adjustable.

MissionTests exercises Cancel, removal/addition, incomplete-row rejection, relation
and operand edits, branch/label preservation and non-conversion of grouped syntax.
The real-engine If test now edits through the dialog, executes the changed branch
and restores the original result with Undo. Dedicated While execution, compound
join outcomes and all parameter-picker combinations still need broader coverage.

GmatQt was rebuilt; all 11 Qt checks passed in 28.35 seconds. Evidence:
`Qt6ParityValidation/check-condition-builder.txt`.

## Single-parameter command selection

The shared parameter browser now supports a scalar selection mode for condition
operands, Achieve goals/values, Minimize objectives and nonlinear constraints. It
uses the current entry directly, hides report-list management, and offers array
element construction. Empty text and bare arrays cannot be accepted. The ordered
report mode and its existing tests are retained unchanged.

MissionTests checks initial selection, hidden report-list controls, empty/bare-array
rejection and Choice(2,3) selection. It chooses a configured goalValue variable in
an Achieve command, verifies Cancel, saves/reopens and executes the target to x=8
(tolerance 1e-6). Existing repeated-run and solver-window lifecycle checks also run
with this selected parameter. Minimize/constraint execution combinations and more
property types remain pending; a shared widget is not counted as all-context proof.

The array-element check exposed that Moderator resolves indexed names to the base
array. The browser now distinguishes exact array names from indexed references;
Use element cannot append another index to an already indexed entry. GmatQt was
rebuilt and all 11 Qt checks passed in 38.47 seconds. Evidence:
`Qt6ParityValidation/check-single-parameter.txt`.

## Writable mission parameter selection

Vary and assignment destinations now use writable modes of the shared browser.
Configured user Variable/Array/String resources are handled separately from
system-parameter IsSettable metadata. Vary additionally restricts configured
parameters to numeric results (and arrays for element construction), and property
browsing to settable/plottable types, following wx's Vary selector. Assignment
allows user strings and whole arrays. Known read-only typed references and numeric
literals cannot be accepted as writable destinations; direct text editing remains
available for unsupported assignment forms.

MissionTests verifies Vary offers x/Choice but not a String, offers spacecraft X
but not ElapsedSecs, and rejects typed read-only references/numeric literals. It
checks assignment string/whole-array availability. The targeting fixture initially
varies the wrong variable; the actual Vary picker selects x, then the existing
Achieve picker selects goalValue. Save/reopen and subsequent solver runs must still
achieve x=8 within 1e-6. Broader hardware/plugin parameter and destination execution
cases remain pending.

GmatQt was rebuilt; all 11 Qt checks passed in 26.98 seconds. Evidence:
`Qt6ParityValidation/check-writable-parameters.txt`.

## Spacecraft SPICE file-list editor

SpacePoint's four spacecraft kernel arrays were excluded by the generic list
allowlist. They are now exposed as ordered file lists rather than resource-name
lists. The dialog supports adding existing files, removing selected entries,
drag reordering and Cancel. Serialization quotes paths, preserves commas/spaces,
replaces the entire list and allows clearing. Apostrophes, semicolons and embedded
line breaks are explicitly rejected in this GUI path; unrestricted filename
round trips remain an outstanding edge case.

WorkflowTests uses two copies of the bundled GEOSat.bsp, including filenames with
spaces and a comma. It adds through the real picker, checks duplicate prevention,
reorders, cancels removal, applies and verifies engine order. It then checks
Undo/Redo, exact save/reopen, failed-edit rollback for a nonexistent kernel and
clearing. All four kernel-list controls are present, but this does not qualify
CK/SCLK/FK loading or SPICE propagation/attitude execution.

GmatQt was rebuilt; all 11 Qt checks passed in 27.83 seconds. Evidence:
`Qt6ParityValidation/check-kernel-lists.txt`.

## SPICE orbit/attitude execution and file recovery

CompatibilityTests adapts Ex_SPICEOrbitAndAttitudePropagation.script to a 60-second
run, converts its two OF views through MainWindow's Qt converter, and configures
the bundled SPK, CK and SCLK files through the kernel-list dialogs. The sample's
referenced MarsExpress_MEX_V10.TF is absent from this checkout; the fixture omits
that FK and does not claim unmodified-sample or FK qualification. SpiceAttitude
works with the available files and explicit spacecraft/frame NAIF IDs in this case.

After exact save/Save As/reopen and invalid-build recovery, the mission must produce
at least 13 report/view samples and reach 60 seconds. Plotted Mars-frame positions
match report values within 1e-5 km. Recorded body-to-view orientation matches the
engine's transposed inertial-to-body matrix within 1e-10 and is orthonormal within
1e-8. This short fixture's attitude is effectively stationary (change about 1.6e-15),
so it qualifies orientation capture, not visibly changing SPICE attitude playback.

Removing a copied clock file after a successful run reproduced a recovery defect:
CSPICE may reread remaining text kernels while unloading another kernel. A missing
file left its pool and GMAT's loaded-file map inconsistent, so restoring the file
was insufficient. Shared SpiceInterface cleanup now clears the entire pool using
[KEEPER reset](https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/cspice/kclear_c.html)
for UnloadAllKernels. A failed individual unload rebuilds the available remaining
kernels and their cache, leaving unavailable files eligible for later reload.
This is shared engine cleanup and also affects non-Qt callers; propagation math is
unchanged. Restoring the clock must recover and produce an identical report, as
must a further repeat run. Full FK, coverage-boundary and changing-attitude cases
remain pending.

GmatQt was rebuilt; all 11 Qt checks passed in 27.54 seconds. Evidence:
`Qt6ParityValidation/check-spice-runtime.txt`; the reproduced failure is retained
in `Qt6ParityValidation/spice-recovery-before.txt`.

## Spacecraft visualization colors

The resource-property adapter previously skipped COLOR_TYPE fields. It now reads
and applies GMAT color strings, preserving names when unchanged and accepting RGB
triples via engine validation. ResourceEditor provides a color picker and preview
swatch; spacecraft OrbitColor/TargetColor are placed in Visualization. SPICE
classification also now includes every field containing Spice, so the four kernel
lists appear together before attitude-specific grouping.

PlotTests selects orbit/target colors, checks Cancel and deferred Apply, exercises
Undo/Redo, rejects an out-of-range RGB component without source mutation, then
saves/reopens/runs. The selected orbit color must reach both the viewer's curve
and final trajectory sample, and the target color must remain in engine state.
This qualifies spacecraft resource colors; solver-iteration target rendering,
per-view overrides and other resource color types remain pending.

GmatQt was rebuilt; all 11 Qt checks passed in 30.89 seconds. Evidence:
`Qt6ParityValidation/check-resource-colors.txt`.

## Solver option controls

Audited TargetPanel/OptimizePanel mode dropdowns and progress checkbox, plus
FindEventsPanel's Append checkbox. Qt uses engine-provided solver mode choices
and typed controls for existing options. An explicit Add default options action
adds only missing solver options using fresh command defaults; opening the form
does not rewrite the script. Existing values (including unfamiliar values) and
branch text remain preserved.

MissionTests checks full and partial insertion, pending solver-name edits, branch
labels/comments/bodies, dropdown/checkbox source updates and Append preservation.
The real Target workflow selects RunInitialGuess, verifies x remains at 1, undoes
the edit and continues through the existing saved/reopened Solve case reaching 8.
This does not qualify every Optimize mode or Stop/Discard combination, FindEvents omission was subsequently closed below.

Switching modes exposed stale progress windows: engine listener keys include the
command's generated source, so Initial Guess and Solve could leave separate
windows and the earlier unconverged result remained visible. Qt now tracks which
listeners were used by the current run and removes inactive windows when that run
ends. Listener objects remain alive for engine-held pointers. The mode-switch test
requires exactly one current progress window and successful convergence; existing
repeat-run, close/recreate and user column-width checks remain in place.

GmatQt was rebuilt; all 11 Qt checks passed in 30.94 seconds. Evidence:
`Qt6ParityValidation/check-solver-options.txt`.

## Thruster coefficient editor

Audited wx ThrusterCoefficientDialog: two coefficient tables with read-only names
and units. Qt now supplies matching chemical C1–C16 / K1–K16 and electric
ThrustCoeff1–5 / MassFlowCoeff1–5 tables, deriving units and row counts from engine
metadata. Columns remain adjustable. Values come from the resource panel's pending
state; Cancel is local, all entries must be finite before OK, and engine changes
wait for the parent Apply.

CompatibilityTests checks pending-value initialization, Cancel, unit protection,
resizing, overflow rejection without partial copying and deferred engine updates.
The chemical fixture sets C1/K1 through the dialog before save/Save As, failed-build
recovery, reopen and the existing finite-burn fuel-use/coasting assertion. Electric
fifth coefficients are changed through both tabs, applied and checked after the
same round trip. This electric case qualifies configuration persistence, not
execution of every thrust model or power-dependent operating mode.

GmatQt was rebuilt; all 11 Qt checks passed in 29.68 seconds. Evidence:
`Qt6ParityValidation/check-thruster-coefficients.txt`.

## FindEvents omitted options and report behavior

wx FindEventsPanel always exposes Append. Qt previously only exposed it when
already written in source. Add default options now also handles event searches,
reading the default from FindEvents itself. The resulting checkbox retains the
existing source-preserving edit behavior. MissionTests checks omitted insertion,
pending locator edits, label/comment preservation and no duplicate insertion.

CompatibilityTests extends the EclipseLocator plugin case with Manual mode and a
FindEvents command. It selects replacement through the form, saves/Save As,
recovers from failed interpretation, reopens and runs to a report containing
shadow intervals. Selecting Append through the form and repeating that lifecycle
must preserve the previous report as a prefix and add new output. Clearing Append
and rerunning must restore the original report exactly. This qualifies these
manual EclipseLocator report modes, not all ContactLocator/IntrusionLocator
options or file-error recovery cases.

GmatQt was rebuilt; all 11 Qt checks passed in 33.68 seconds. Evidence:
`Qt6ParityValidation/check-event-options.txt`.

## Camera controls in narrow viewers

Moved the embedded camera selector/projection/FOV widgets from the crowded plot
toolbar into a nonmodal Camera panel. This avoids relying on toolbar widget
availability at tiled widths. Keep projection is in the same panel and delegates
to the existing undoable script edit; changes still preview live. Close explicitly
dismisses the panel without reverting preview settings.

PlotTests checks access from a 330-pixel viewer, panel bounds, reopen persistence,
disabled script persistence for standalone plots, and the existing projection,
FOV and named-camera rendered-image differences. WorkflowTests now invokes the
panel's Keep projection button for unchanged-source preservation, selected-view
metadata, undo and unbuilt-script rejection. This improves camera-control access;
trajectory/segment-relative conversion and broader framing behavior remain pending.

GmatQt was rebuilt; all 11 Qt checks passed in 39.09 seconds. Evidence:
`Qt6ParityValidation/check-camera-panel.txt`. An isolated Xvfb/native render run
also passed; `Qt6ParityValidation/camera-panel.png` was visually inspected for
label/control fit. This is not a hardware-driver qualification claim.

## Live orbit display controls

Compared wx OrbitViewPanel drawing options with Qt model/renderer fields. The Qt
viewer now exposes axes, grid, labels, legend, XY/ecliptic planes, wireframe and
origin–Sun line in a nonmodal Display panel. These are live viewer overrides;
persistent resource changes still use the resource editor. The panel explicitly
states this distinction and resynchronizes its checkboxes on reopening.

PlotTests checks each control's initial state and model binding, unchanged replay
frame/camera history, a combined rendered-image change and exact restoration of
the original image after clearing the options. It also checks reopening after a
model value changes. This does not individually qualify every overlay's geometry
or replace the pending full OrbitView resource audit.

GmatQt was rebuilt; all 11 Qt checks passed in 38.90 seconds. Evidence:
`Qt6ParityValidation/check-display-controls.txt`. An isolated native/Xvfb run
also passed, and `Qt6ParityValidation/display-panel.png` was visually inspected.
Hardware-specific viewer qualification remains separate.

## Paired tank and mixture Apply

Auditing wx TankAndMixDialog exposed a Qt Apply ordering defect: changing Tank and
MixRatio together applied ratios against the old list first, then remapped those
new values by old tank name. A reorder could silently assign different ratios
than the user entered; a resized list could reject otherwise valid new ratios.
The Qt adapter now treats explicit simultaneous ratios as positional in the new
ordered list, validating one finite positive value per tank before interpretation.
A Tank-only edit continues to preserve existing ratios by tank name.

WorkflowTests covers simultaneous reorder/new ratios, count mismatch, zero,
negative and overflow rejection with unchanged source, removal/addition with ratio
resizing, and save/reopen preserving the resulting tank/value pairs. The existing
Tank-only GUI picker test still checks association preservation. This fixes the
Apply layer; a dedicated combined tank/mixture selector and multi-tank fuel-use
execution remain pending, so the wx dialog row is not marked complete.

GmatQt was rebuilt; all 11 Qt checks passed in 34.30 seconds. Evidence:
`Qt6ParityValidation/check-paired-mixtures.txt`.

## Combined tank and mixture editor

Qt now offers Tanks and mixtures with named, read-only tank rows, editable positive
ratios, available-tank selection, Add/Remove and Up/Down controls. Moving a row
moves its ratio with it; duplicate additions are disabled and columns remain
resizable. Cancel is local and OK defers engine changes until Apply. Explicit
paired edits submit both fields even if the numeric vector happens to equal the
original vector while tank order has changed.

WorkflowTests covers Cancel, candidate filtering, duplicate prevention, remove/
re-add, default ratio, paired movement, zero rejection, pending engine state,
column resizing and the unchanged-vector/reordered-tanks save/reopen case.
CompatibilityTests adds a second tank through this dialog and sets ratios 3:2.
After save/Save As, failed-build recovery and reopen, the finite-burn fixture must
consume fuel in the corresponding 60/40 split, with both masses unchanged after
EndFiniteBurn during coasting. This qualifies the chemical two-tank GUI path;
broader electric operating modes remain pending.

GmatQt was rebuilt; all 11 Qt checks passed in 41.78 seconds. Evidence:
`Qt6ParityValidation/check-tank-mixture-dialog.txt`.

## Perspective Fit in narrow windows

The OpenFrames View::resetView audit found that automatic framing uses the
minimum horizontal/vertical half-angle. Qt's Fit distance used only the vertical
angle, allowing horizontal clipping in portrait windows. OrbitCamera now accepts
the viewport aspect ratio and uses the smaller half-angle for perspective Fit;
both native and QPainter paths pass their actual drawing-area aspect. Explicit
scripted distances with Fit disabled remain unchanged.

NativeOrbit tests render a green sphere in 240×600, 600×240 and 400×400 viewports,
require visible body pixels and a margin on all four sides, at normal and high DPI.
This qualifies perspective Fit containment, not equivalence of all OF automatic
bounding-sphere centers or trajectory/segment framing.

Further trajectory audit: OFScene::GetTrajectoryFrameByName returns
OFSpaceObject::WholeTrajectory (mDrawTraj), while moving-object views use
FrameOnWholeTrajectory (mViewRefFrame). Thus simply disabling body orientation
while continuing to track object position is not a full trajectory-view mapping.
Stored whole-trajectory conversion is addressed in the following milestone;
automatic trajectory and segment framing remain open requirements.

GmatQt was rebuilt; all 11 Qt checks passed in 37.66 seconds. Evidence:
`Qt6ParityValidation/check-perspective-fit.txt`.

## Stored whole-trajectory conversion

The OFSpaceObject source confirms mDrawTraj is attached directly to the primary
plot frame, while mViewRefFrame has the position/attitude follower. Conversion now
maps stored whole-trajectory views to the plot-frame origin rather than tracking
the named spacecraft. Stored eye, center, up, FOV and existing two-frame look-at
handling remain in place. Secondary named trajectory presets likewise use an
empty object reference. Unknown trajectory names are rejected unless represented
in the plot Add list; CoordinateSystem is accepted directly.

WorkflowTests checks primary/secondary mapping, scientific command preservation,
unknown-object rejection, automatic-framing and segment-specific diagnostics.
A converted example is saved/reopened/run; its spacecraft must move more than
1 km while every camera eye/target sample remains fixed, with the stored eye
coordinate retained. The earlier generic trajectory warning is replaced by a
specific preservation note for supported stored views. Unsupported automatic
trajectory and segment cases now stop conversion with actionable errors rather
than substituting moving-object tracking. Their implementation remains pending;
this milestone does not claim complete trajectory camera parity.

GmatQt was rebuilt; all 11 Qt checks passed in 36.87 seconds. Evidence:
`Qt6ParityValidation/check-trajectory-camera.txt`.

## Paged report viewer

Replaced the first-16-MiB-only preview with ReportViewer: approximately 1 MiB per
page, First/Previous/Next/Last, page-number jump, reload and existing read-only
Find dialog scoped explicitly to the current page. All file regions are reachable
without accumulating previously viewed text. UTF-8 codepoints and CRLF pairs are
kept on the same side of a boundary. Long rows may span pages, which the UI states.
Read errors retain the prior display. Reload reopens the path so replaced/shrunk
files are handled, and page navigation clamps to the current file size.

FileTests reconstructs a four-page report containing three- and four-byte UTF-8
characters and a CRLF at boundaries, exercises backward/jump navigation, retains
content after a missing-file error, reaches a marker beyond 17 MiB, then reloads
a shorter replacement. Existing Compatibility/Plot report checks retain exact
small-report text, path-in-title, read-only/no-wrap, selection and close behavior.
Case-sensitive full-file search follows below; wx report-comparison workflows remain pending.

GmatQt was rebuilt; all 11 Qt checks passed in 31.05 seconds. Evidence:
`Qt6ParityValidation/check-report-paging.txt`.

## Complete report search

ReportViewer now supports a separate case-sensitive literal search across the
complete file, next non-overlapping match and cancellation. A zero-interval Qt
timer scans one MiB per callback with bounded overlap for cross-chunk matches;
closing the viewer destroys the timer/file objects. Navigation/reload cancels an
active scan. Results navigate to the matching page, highlight its visible portion
and explicitly identify a result continuing onto the next page. Search file
restarts from the beginning; exhaustion does not wrap silently.

FileTests covers a match spanning a page boundary, a Unicode character deferred
to the next page, advancing matches, cancellation, no-match feedback and finding
a marker beyond 17 MiB. Page-local Find retains its existing richer options.
Case-insensitive/whole-word full-file modes and report comparison are not claimed
by this milestone.

GmatQt was rebuilt; all 11 Qt checks passed in 33.34 seconds. Evidence:
`Qt6ParityValidation/check-report-search.txt`.

## Propagate parameter stop controls

The wx audit found a parameter/value stop grid and parameter browser beyond elapsed
seconds/days. Qt PropagationForm now represents one scalar parameter/value stop,
adds single-parameter pickers for both operands, and preserves source spans when
editing. Labels, comments and existing BackProp are retained. Seconds/Days remain
quick selections and follow spacecraft changes. Broader Propagate structures are
left in the source editor without being simplified.

WorkflowTests checks exact labeled/commented BackProp source preservation and
fallback for event-only conditions. The real command editor selects QtSat.A1ModJulian
through the browser, sets an epoch 0.01 days after the initial state, verifies Cancel
on the goal picker, applies, saves/reopens and runs. The resulting epoch must be
864 seconds after the initial epoch. Multi-propagator/multi-stop structures,
event-only controls, StopTolerance and other advanced modes remain pending.

GmatQt was rebuilt; all 11 Qt checks passed in 33.11 seconds. Evidence:
`Qt6ParityValidation/check-propagation-stops.txt`.

## Periapsis and apoapsis stop controls

PropagationForm now handles scalar stops and the goal-free Periapsis/Apoapsis
syntax. A dedicated parameter-browser mode includes these event parameters even
when they are not reportable; report and writable-parameter modes remain unchanged.
Selecting an event disables the numeric goal and its picker. Source edits retain
labels, comments and modifiers, and switching back to a scalar restores editable
goal controls. Other unrepresented propagation structures remain in source.

WorkflowTests checks event syntax without a spurious equality/goal.
CompatibilityTests selects both event types through the object/property/central-
body browser, applies, saves/Save As, recovers from failed interpretation, reopens
and executes. A two-body ellipse with SMA 10000 km and eccentricity 0.1 must stop
at radius 9000 km for periapsis and 11000 km for apoapsis, within 0.01 km. These
checks validate GUI choices against the unchanged propagation engine. Multiple
stops, synchronized propagators and StopTolerance controls remain pending.

GmatQt was rebuilt; all 11 Qt checks passed in 32.56 seconds. Evidence:
`Qt6ParityValidation/check-apsis-controls.txt`.

## Propagate direction and tolerance

Added a backward-propagation checkbox and optional StopTolerance field to the
single-stop form. The placeholder reads the default from a fresh engine Propagate
command. Empty input preserves omission or removes an explicit option; existing
tolerance values are replaced by span so formatting is retained. BackProp edits
also preserve labels/comments and existing spacing when unchanged.

WorkflowTests covers inserting/removing tolerance, preserving existing compact
option formatting and toggling an existing BackProp modifier. CompatibilityTests
extends the selected apsis workflow: zero tolerance must fail without changing
source, a positive tolerance must apply, and saved/reopened periapsis propagation
must move backward in epoch while apoapsis moves forward. Both retain the expected
orbital radius checks. These changes do not implement multiple stopping conditions,
synchronized propagators or other advanced Propagate options.

GmatQt was rebuilt; all 11 Qt checks passed in 34.38 seconds. Evidence:
`Qt6ParityValidation/check-propagation-options.txt`.

## Multiple stopping-condition table

CommandEditor now offers Stopping conditions for representable Propagate stop
blocks, independently of the single-stop quick form. The two-column dialog adds,
removes and reorders paired parameter/goal rows; shared parameter browsers support
both scalar and apsis conditions. Apsis goals are blank/read-only, blank rows cannot
be accepted, and source-only expressions remain protected by parsing checks.
StopTolerance/OrbitColor options, the propagation header, labels and comments are
retained. OK makes one source undo step, with engine validation deferred to Apply.

WorkflowTests covers the multiple-stop button, Cancel, editable column widths,
blank-row rejection, paired movement/removal, apsis goal protection and source Undo.
CompatibilityTests adds an elapsed-days stop through the new table to a 600-second
command with a tolerance and comment. After save/Save As, failed-build recovery and
reopen, execution must end at 86.4 seconds (0.001 days), the earlier condition.
This qualifies multi-stop editing; synchronized/multiple-propagator configuration
and broader stopping-expression combinations remain pending.

GmatQt was rebuilt; all 11 Qt checks passed in 54.61 seconds. Evidence:
`Qt6ParityValidation/check-multiple-stops.txt`.

## Propagator assignment editor

CommandEditor now offers Propagators and spacecraft for representable Propagate
headers. The dialog provides propagator selectors, editable object lists, an
ordered spacecraft/formation picker, add/remove rows, engine-provided propagation
modes and backward propagation. Empty or duplicate object assignments cannot be
accepted. Formations are supplied separately from the simple spacecraft form so
they cannot generate an invalid formation elapsed-time parameter there. Stop
blocks, options, labels and comments are preserved when changing assignments.

WorkflowTests covers modifier changes, multiple objects, a formation assignment,
exact preservation of the command suffix, and source fallback for STM flags.
CompatibilityTests exercises Cancel, invalid empty/duplicate rows, the nested
object picker, and two distinct propagators in Synchronized mode. After Apply,
save/Save As, failed-build recovery and reopen, both spacecraft advance to the
same epoch, stopping at the earlier of the configured conditions (86.4 seconds).
The second spacecraft also has a changed position. This is not full formation
execution qualification or coverage of all mode/propagator combinations.
Variational propagation flags still require the source editor.

Validation: rebuilt the user's GmatQt and passed all 11 Qt tests in 41.68 seconds
under the isolated test environment. Evidence:
`Qt6ParityValidation/check-propagation-groups.txt`.

## STM and A-matrix propagation controls

The propagator assignment dialog now includes Propagate STM and Compute A-matrix,
matching the command-wide checkboxes in wx PropagatePanel. Quoted and unquoted
flags are parsed separately from spacecraft names. Selecting or clearing a flag
updates the command while retaining the stop block, label and comment. The engine
applies these flags to all groups; serialization places selected flags in the
first group. Covariance flags continue to use source editing without a lossy
structured rewrite.

WorkflowTests loads both flags and independently clears STM while retaining
A-matrix. CompatibilityTests enables both through the dialog before save/Save As,
failed-build recovery and reopen, then runs the synchronized two-propagator
fixture. Both spacecraft reach the expected stop epoch and have nontrivial
FullSTM and FullAMatrix values. This verifies GUI-to-engine option delivery, not
an independent mathematical validation of GMAT's matrix implementation.

Validation: rebuilt the user's GmatQt and passed all 11 Qt tests in 34.45 seconds.
Native rendering tests ran under isolated Xvfb, not the user's live display.
Evidence: `Qt6ParityValidation/check-propagation-variational.txt`.

## For-loop selection controls

Audited wx ForPanel's four selectors and input restrictions. Qt now provides a
Variable-only Index selector and the shared parameter browser for Start, Step
and End, preserving free-text numeric entry. MissionTests checks index filtering
and Cancel, opens each bound browser, confirms a configured variable is available,
selects numeric bounds, applies the header edit, executes the changed loop and
undoes it to recover the original result. The branch body is preserved by the
existing source-span editing. This does not yet qualify every parameter-valued
loop-bound combination.

Validation: rebuilt the user's GmatQt; all 11 Qt tests passed in 34.57 seconds,
with native rendering under isolated Xvfb. Evidence:
`Qt6ParityValidation/check-for-pickers.txt`.

## Combined array values and formulas

Resource Apply now assembles numeric array settings and managed expression cells
into one candidate script and validates it once. Formula metadata bypasses the
ordinary engine property serializer. Expression indices are checked against the
proposed dimensions, including numeric-only resizes, so a shrink cannot silently
discard a managed formula cell. Existing mission commands remain preserved.

WorkflowTests changes a numeric value and a dependent formula in the same resource
panel, submits both with its Apply button, and checks the resulting calculation.
One Undo restores the original script and result. Invalid combined formulas and
shrinks past a formula cell leave source and runtime behavior unchanged. The
expression grid still uses dimensions from panel opening; entering formulas into
newly added cells before applying a resize remains a follow-up workflow gap.

Validation: rebuilt the user's GmatQt; all 11 Qt tests passed in 35.34 seconds.
Native display checks ran under isolated Xvfb. Evidence:
`Qt6ParityValidation/check-array-combined.txt`.

## Expression editing with pending array dimensions

The expression editor now derives its dimensions from pending numeric values.
New rows and columns are available without an intermediate Apply. If a pending
shrink excludes an existing formula, the grid keeps that cell visible, highlights
it and disables OK until the formula is cleared. Cancel preserves all formulas;
users can enlarge the numeric grid instead. Engine validation still handles
invalid numeric rows and formula references at Apply.

WorkflowTests grows a 1x2 array to 2x3, adds a dependent formula in the new bottom
right cell, and verifies the result is 21 after the combined Apply. It also opens
the expression editor after a pending shrink, checks that removed cells remain
visible and OK is disabled, clears them to enable OK, then cancels. Restoring the
larger numeric grid and applying proves the cancelled cleanup preserved formulas.
The existing single-Undo and invalid-formula rollback checks remain in place.

Validation: rebuilt the user's GmatQt and passed all 11 Qt tests in 34.47 seconds.
After adding explicit shrink-cleanup/Cancel assertions, the updated Workflow test
passed in 8.20 seconds. Native checks used isolated Xvfb. Evidence:
`Qt6ParityValidation/check-array-pending-size.txt` and
`Qt6ParityValidation/check-array-shrink-ui.txt`.

## Power-system field audit and shadow-body selector

Audited wx PowerSystemConfigPanel's general settings (EpochFormat, InitialEpoch,
InitialMaxPower, AnnualDecayRate, Margin), three bus coefficients, five solar
coefficients, ShadowModel and ShadowBodies. Qt now groups these settings in
General, Bus coefficients, Solar coefficients and Shadow tabs as applicable.
The audit found ShadowBodies was omitted because STRINGARRAY_TYPE fields require
explicit list support. It now uses the ordered resource selector populated with
celestial bodies, and list Apply rejects non-body references.

WorkflowTests checks that each wx field appears in its matching section, unrelated
fields are hidden, and nuclear systems have no solar-only tabs. It exercises the
shadow picker, pending selection, two-body reconstruction, invalid-body rollback
and Undo. The wx behavior that converts InitialEpoch when EpochFormat changes is
not yet implemented in Qt; power output and eclipse/shadow runtime qualification
also remain pending. These tests establish configuration workflow coverage only.

Validation: rebuilt the user's GmatQt; all 11 Qt tests passed in 34.53 seconds.
Native viewer checks used isolated Xvfb. Evidence:
`Qt6ParityValidation/check-power-controls.txt`.

## Power-system epoch-format conversion

The power resource panel now offers the engine's valid time representations in an
EpochFormat dropdown. Changing it converts the current pending InitialEpoch via
TimeSystemConverter, preserving the represented instant. If conversion fails, the
format selection is restored and the date text is retained for correction. The
configured resource is unchanged until Apply.

WorkflowTests exercises solar and nuclear panels, converting a known UTC date to
TAI modified Julian time (including the 32-second offset at that date), converting
back, and rejecting an invalid pending date without changing either field. Paired
Apply reconstructs both settings through the script and Undo restores the source.
Runtime power and shadow calculations remain a separate qualification gap.

Validation: rebuilt the user's GmatQt; all 11 Qt tests passed in 35.41 seconds.
The subsequently extended Workflow test, including paired Apply/Undo, passed in
18.51 seconds. Native viewer checks used isolated Xvfb. Evidence:
`Qt6ParityValidation/check-power-epoch.txt` and
`Qt6ParityValidation/check-power-epoch-apply.txt`.

## Power-model selection and report execution

Solar ShadowModel now offers None and DualCone, matching the engine's accepted
values and the wx selector. CompatibilityTests edits nuclear and solar resources
through ResourceEditor, including maximum power, decay, margin, bus coefficients,
and solar coefficients/model selection. It saves, saves under a second name,
recovers from a failed interpretation, reopens and runs each mission.

The generated report must show the selected constant bus demand, configured
margin applied to the generated power, and exactly 10 kW nuclear output. The
unshadowed solar case uses inverse-square generation coefficients and checks a
bounded near-Earth output; this is not an exact independent solar-model check.
Eclipse attenuation, time-dependent decay, and electric-thruster coupling remain
separate qualification cases.

Validation: rebuilt the user's GmatQt; all 11 Qt tests passed in 35.43 seconds.
Native viewer checks used isolated Xvfb. Evidence:
`Qt6ParityValidation/check-power-runtime.txt`.

## Spacecraft epoch display conversion

The spacecraft DateFormat selector now shares the pending-epoch conversion
behavior with power systems. A successful change updates Epoch through GMAT's
time converter; failed conversion restores the prior selector value and retains
the invalid text for correction. This addresses a wx OrbitPanel interaction gap
without changing the propagation engine.

WorkflowTests converts a spacecraft epoch through UTC Gregorian and TAI modified
Julian formats, checks invalid-date recovery, applies the pair, and compares the
configured epoch with the original instant. After save/reopen, the mission must
finish at the original epoch plus the requested 600 seconds. Other OrbitPanel
state-representation and coordinate-system interactions remain under audit.

Validation: rebuilt the user's GmatQt; all 11 Qt tests passed in 58.11 seconds.
Native viewer checks used isolated Xvfb. Evidence:
`Qt6ParityValidation/check-spacecraft-epoch.txt`.

## Pending spacecraft state representation

DisplayStateType choices now come from StateConversionUtil, filtered using wx's
celestial-body-origin and fixed-coordinate requirements. The selector converts a
cloned spacecraft with the six pending element values, then updates all six
labels, values and units only after successful conversion. Failures restore the
previous selector and keep the inputs intact. Pending coordinate, epoch or anomaly
changes must be applied first so conversion uses the intended reference settings.

WorkflowTests switches Cartesian to Keplerian, checks SMA against the configured
state, rejects invalid input without losing it, edits SMA, switches back to
Cartesian and then Keplerian, and applies the result. The configured spacecraft
remains unchanged until Apply; the reconstructed state retains the SMA edit and
Undo restores the original script. Other representations and combined coordinate
changes still need dedicated qualification.

The audit also exposed a generic state-editing failure: spacecraft element aliases
resolve to IDs outside the normal property range, where read-only validation
throws. Resource assignment now resolves current display labels to their six
writable Element IDs. Apply establishes DisplayStateType before assigning
elements, avoiding alphabetical ordering that could apply AOP before the new
representation. Both fixes are exercised by the pending-state conversion test.

Validation: rebuilt the user's GmatQt; all 11 Qt tests passed in 50.20 seconds.
Native viewer checks used isolated Xvfb. Evidence:
`Qt6ParityValidation/check-state-representation.txt`.

## Enumeration versus color controls

The shared resource-property reader incorrectly fell through from ENUMERATION_TYPE
into COLOR_TYPE, marking enumerations as colors. Fields whose engine enum list
was empty could consequently receive a color button, including spacecraft
AnomalyType and extra controls beside date/state settings. Enumeration handling
now reads its string value and finishes without setting the color flag.

WorkflowTests verifies that DateFormat, DisplayStateType and AnomalyType do not
offer color buttons or color metadata, while OrbitColor and TargetColor retain
their pickers. Existing PlotTests still exercise actual color selection, Cancel,
Apply, rollback and rendered trajectory color.

Validation: rebuilt the user's GmatQt; all 11 Qt tests passed in 35.00 seconds.
Native viewer checks used isolated Xvfb. Evidence:
`Qt6ParityValidation/check-enum-controls.txt`.

## GMAT function-file editor

Audited wx FunctionSetupPanel and added an Edit function file button to Qt
GmatFunction resources. It opens the selected FunctionPath in ScriptEditor with
find/replace, Save and Cancel. Saving uses QSaveFile and checks the current file
against the loaded bytes before writing, preventing silent overwrite of external
edits. Failed reads and invalid UTF-8 disable saving; BOM and uniform CRLF style
are retained. Run already rebuilds the mission before executing functions.

FileTests exercises content changes, BOM/CRLF retention, external-change conflict
and invalid UTF-8. CompatibilityTests runs a temporary GMAT function, edits its
multiplier through the resource button and dialog Save, then saves/reopens the
mission with failed-build recovery and verifies the changed output. wx's separate
function-file Save As workflow remains pending; MATLAB editing is outside the
current Linux/no-MATLAB scope.

Validation: rebuilt the user's GmatQt; all 11 Qt tests passed in 40.50 seconds.
Native viewer checks used isolated Xvfb. Evidence:
`Qt6ParityValidation/check-function-editor.txt`.

## Function-file Save As

FunctionFileDialog now provides Save As with a native destination chooser and
overwrite confirmation. Success writes the current editor contents to the new
file, preserving encoding/line-ending behavior, and returns its path to the
resource panel. FunctionPath remains pending until resource Apply rebuilds the
mission. Failed saves do not change the current path; saving to the original path
retains the external-change check.

FileTests covers Unicode/spaced destinations, preservation of unsaved contents,
existing-destination refusal without confirmation, failed destination writes,
and Cancel leaving the original untouched. CompatibilityTests opens the actual
Save As chooser, creates a copy in another directory, checks the old file remains,
checks the engine path is unchanged before Apply, applies the new path, then
saves/reopens the mission and executes the edited function. Creation of a new
empty function file is still outside this existing-file editor workflow.

Validation: rebuilt the user's GmatQt. The full run passed the other 10 Qt checks,
including Compatibility's actual Save As/execution workflow. Files initially
failed because its new assertion expected a trailing newline absent from the
editor contents. After correcting that expectation and adding Cancel coverage,
Files passed in 3.40 seconds. Thus every check has a passing result for the final
implementation, across the full and targeted runs. Native viewers used Xvfb.
Evidence: `Qt6ParityValidation/check-function-save-as.txt` and
`Qt6ParityValidation/check-function-save-as-files.txt`.

## New GMAT function-file creation

ResourceEditor now offers New function file for a GmatFunction resource. It asks
for an unused destination and opens a basic positional input/output template with
the resource's function name. Saving creates the file and leaves FunctionPath
pending until Apply. Cancel creates nothing. If a file appears at the chosen
destination while the template is open, Save refuses to overwrite it.

FileTests covers creation, Cancel and an externally created destination.
CompatibilityTests drives the destination chooser, edits the generated template,
saves, applies the path, saves/reopens the mission with failed-build recovery and
checks the function's resulting output. The resource in this fixture already
exists; combined resource creation and broader function signatures still require
additional qualification.

Validation: rebuilt the user's GmatQt; all 11 Qt tests passed in 41.98 seconds.
Native viewer checks used isolated Xvfb. Evidence:
`Qt6ParityValidation/check-new-function.txt`.

## Function-call input and output selection

CommandForm now offers ordered argument selectors for function inputs and outputs,
using shared parameter browsing plus whole space-point/impulsive-burn objects and
strings. Output browsing filters writable parameters and rejects numeric output
destinations. Inputs retain free text for literals and other function syntax.
Argument splitting respects quoted strings and nested parentheses/brackets;
command source spans preserve the function name and trailing comment.

CompatibilityTests checks Cancel, ordering, invalid output recovery, and a real
two-input/two-output function after Apply, save/Save As, failed-build recovery and
reopen. Swapping both lists must produce the expected sum/difference in the
selected output variables. A separate source-preservation check covers quoted
commas and array-index commas; it does not claim execution coverage for those
string/array signatures. Broader object arguments and zero-output calls remain
qualification cases.

Validation: rebuilt the user's GmatQt. The full suite passed ten checks; the
compatibility check initially expected a change notification when accepting an
unchanged argument list. The corrected test retains the original source as its
initial value and checks any emitted replacement against it. The targeted
compatibility rerun passed in 9.54 seconds. No production code changed between
the full run and that rerun. Native viewer checks used isolated Xvfb. Evidence:
`Qt6ParityValidation/check-function-arguments.txt` and
`Qt6ParityValidation/check-function-arguments-compat.txt`.

## Omitted solver-command option controls

The wx Vary and Achieve panels expose settings even when the script omits them.
Qt now offers Add default options for these commands, using Vary/Achieve engine
prototypes for defaults. Existing option order and values remain intact; missing
values are inserted inside the command argument list. Labels, comments and
pending field edits survive insertion. Added controls remain editable.

MissionTests checks partially specified Vary options, completely omitted options
for both commands, preservation of pending solver/value edits, and prevention of
duplicate insertion. The targeting workflow applies newly exposed bounds, maximum
step and tolerance, saves/reopens, and converges to the selected goal. This does
not yet qualify solver-dependent enable/disable behavior or every plugin option.

Validation: rebuilt the user's GmatQt; all 11 Qt tests passed in 63.63 seconds.
Native viewer checks used isolated Xvfb. Evidence:
`Qt6ParityValidation/check-solver-option-controls.txt`.

## Solver-dependent Vary controls

Audited wx VaryPanel::SetControlEnabling against the engine capability flags.
Qt now enables perturbation, bounds, maximum step and scale-factor fields only
when the selected configured solver supports them. Unsupported fields retain
their values and explain why they are disabled. An unknown solver disables these
fields until a valid solver is selected; source validation remains with Apply.

MissionTests switches from DifferentialCorrector to Yukon and back, verifies
Yukon's disabled bounds and enabled supported settings, and checks that pending
values, labels and comments survive. Unknown-solver recovery and Cancel are
covered. The existing targeting test still verifies Apply, save/reopen and solve.
The scale/step/perturbation-disabled combinations of other plugins remain outside
this runtime qualification; MATLAB remains deferred.

The form also handles construction before engine initialization by leaving
capability-dependent fields disabled without querying the configuration manager.
The initial run exposed this case; after guarding the lookup, all 11 Qt tests
passed in 70.23 seconds and the user's GmatQt was rebuilt. Native viewer checks
used isolated Xvfb. Evidence: `Qt6ParityValidation/check-solver-capabilities.txt`.

## DifferentialCorrector resource editor

Audited DCSetupPanel's six fields: MaximumIterations, ReportStyle, ReportFile,
ShowProgress, Algorithm and DerivativeMethod. Qt already exposes the iteration,
report-style and progress controls in Convergence/Output sections. Algorithm and
derivative method were free text because the engine marks them as enumerations
without supplying their choice lists. Qt now supplies the same three choices for
each as wx and the engine setters. Solver ReportFile now uses an output chooser.

MissionTests configures Broyden, CentralDifference, Verbose report output and an
iteration limit through ResourceEditor. It checks chooser Cancel and that pending
settings do not alter the configured engine, applies them, saves/reopens, solves
the existing target, and verifies a nonempty report at the selected path. This
qualifies that combination; it does not establish all algorithm/derivative pairs.

Validation: rebuilt the user's GmatQt; all 11 Qt tests passed in 64.67 seconds.
Native viewer checks used isolated Xvfb. Evidence:
`Qt6ParityValidation/check-dc-controls.txt`.

## Centered scene Fit

Qt Fit previously used radius from the coordinate origin, making translated
scenes unnecessarily small and leaving the camera aimed away from their center.
Fit now centers the camera on the bounding box of visible curve history and uses
a sphere enclosing that box. Native bounds include loaded model extents, scale
and offsets, or body radii; hidden objects do not add body radius, and invisible
curves do not contribute. The software renderer uses the same camera calculation
with its displayed body radii. Script view retains the stored camera target and
distance until the user selects Fit. Framing uses all retained history for stable
replay, rather than changing with the playback frame.

NativeOrbit tests render an origin-centered body and a body translated by ten
million units, with an invisible distant curve, in perspective and orthographic
projections and portrait, landscape and square windows. They require a visible,
centered body with margins. A separate shared-camera check protects stored
scripted target/distance. This improves Qt Fit; automatic OF trajectory/segment
conversion remains a separate open requirement.

Validation: rebuilt the user's GmatQt; all 11 Qt tests passed in 62.63 seconds.
Native checks ran under isolated Xvfb with software OpenGL, not the user's live
Intel/Wayland session. Evidence: `Qt6ParityValidation/check-centered-fit.txt`.

## Automatic named whole-trajectory cameras

Audited OFScene::ProcessOpenFramesView/UpdateSegmentViews and OpenFrames
View::resetView. Automatic trajectory views center a bounding sphere and place
the eye on negative Y with positive Z up; perspective distance uses the smaller
horizontal/vertical half-angle. Qt conversion now retains a named object's
automatic whole-trajectory camera in primary and secondary camera metadata.
Stored camera locations continue to take precedence. Unknown trajectory names
or names absent from OrbitView.Add fail Build with a diagnostic.

The shared camera calculation uses a sphere enclosing the selected curve's
retained path bounds, independently of unrelated bodies and trajectories. Qt
refreshes these bounds as samples arrive and uses complete retained history
during replay. This preserves Qt's live viewer behavior; OF refreshes automatic
trajectory views after completed segments. Fit still frames the whole visible
scene and Script view restores automatic trajectory framing. CoordinateSystem
automatic framing, automatic LookAt combinations and segment-specific cameras
remain unsupported by this conversion and receive explicit errors. Bounding
spheres of OF scene decorations are not claimed to match Qt path bounds.

WorkflowTests covers primary/secondary conversion metadata, unchanged mission
commands, a real converted sample after save/reopen, plot-model delivery, invalid
trajectory recovery and stored-camera precedence. OrbitRendererTests checks
translated path centers and aspect-aware camera distance against explicit
geometric expectations, unrelated-object isolation and named-camera selection.

Validation: rebuilt the user's GmatQt; all 11 Qt tests passed in 39.98 seconds.
After improving the unsupported-LookAt fixture to use valid syntax, the targeted
Workflow rerun passed in 21.85 seconds. Native checks used isolated Xvfb/software
OpenGL. Evidence: `Qt6ParityValidation/check-auto-trajectory.txt` and
`Qt6ParityValidation/check-auto-trajectory-workflow.txt`.

## Automatic trajectory LookAt orientation

Automatic named whole-trajectory conversion now supports LookAtFrame with both
ShortestAngle modes. The shared camera calculation transforms the trajectory
bounding center as well as eye/up orientation, using the published camera frame.
User rotation remains an offset around that transformed center. Primary and
secondary named views retain the mode through save/reopen. CoordinateSystem
automatic framing and segment-relative views remain open.

The optional OpenFramesCameraReference probe now compares complete automatic
world-to-view transforms against the installed OpenFrames library: a translated
bounding sphere, 12 target directions, both DIRECT/AZEL modes and three aspect
ratios (72 cases). Maximum transformed-point difference is 8.92633e-06 scene
units, within the 1e-4 tolerance for the reference's float bounding sphere. The
original 48 stored-frame alignment cases still pass. This adds no OpenFrames
dependency to the Qt application.

WorkflowTests converts primary and named automatic LookAt cameras, saves/reopens
and runs them, switches cameras, verifies target direction and orthogonal up,
and checks that ShortestAngle changes roll.

Validation: rebuilt the user's GmatQt; all 11 Qt tests passed in 54.93 seconds.
Native checks used isolated Xvfb/software OpenGL. Evidence:
`Qt6ParityValidation/check-auto-look-at.txt` and
`Qt6ParityValidation/auto-look-at-reference.txt`.

## Explicit camera edits after automatic conversion

Explicit OrbitView resource edits now take precedence over imported automatic
trajectory framing. Editing ViewPointReference, ViewPointVector, ViewScaleFactor,
ViewDirection, ViewUpAxis or ViewUpCoordinateSystem clears the primary automatic
mode and its imported LookAt/center override. Explicit up-axis or up-coordinate
edits also remove the imported arbitrary up vector. Named camera presets retain
their independent settings.

WorkflowTests applies a manual primary position/direction, verifies independent
named camera metadata, saves/reopens, runs and checks the resulting camera pose.
It also checks all six individual pose controls and exact-source Undo, then runs
the restored automatic camera. This closes a GUI conflict in which successful
Apply could leave the imported automatic camera overriding explicit user values.

Validation: rebuilt the user's GmatQt; all 11 Qt tests passed in 38.11 seconds.
Native checks used isolated Xvfb/software OpenGL. Evidence:
`Qt6ParityValidation/check-camera-overrides.txt`.

## Automatic CoordinateSystem cameras

Audited OFScene root creation (hidden root axes, position zero),
ProcessOpenFramesView and ReferenceFrame::getBound/View::resetView. Root bounds
do not include child scene objects. Without LookAt, OFScene supplies twelve
Earth equatorial radii as the automatic bounding radius; with LookAt, the empty
root bound falls back to one unit. Qt now converts both primary and named
CoordinateSystem views without a stored location, regardless of ViewTrajectory.
It retains these radius rules and viewport/FOV scaling. Stored locations retain
precedence; Fit and explicit camera overrides retain their Qt behavior.

The optional OpenFrames reference probe adds 12 root-camera cases: both LookAt
states, both rotation modes and three aspect ratios. All pass with maximum
point difference 0.0103549 scene units. OF constructs its automatic home eye
with a float Vec3; the distant-camera comparison therefore uses a 1e-7 relative
tolerance (minimum 1e-4 absolute), rather than claiming double-precision identity.
The original 48 stored-frame and 72 trajectory LookAt comparisons still pass.

WorkflowTests checks unchanged mission text during conversion, primary/named
metadata, save/reopen/run, origin center and the expected radius/FOV distance
with and without LookAt. Segment-relative cameras and automatic body-bound
framing remain open requirements.

Validation: rebuilt the user's GmatQt; all 11 Qt tests passed in 39.31 seconds.
Native checks used isolated Xvfb/software OpenGL. Evidence:
`Qt6ParityValidation/check-auto-origin.txt` and
`Qt6ParityValidation/auto-origin-reference.txt`.

## Automatic body and model framing

Audited OFSpaceObject's frame selection and Sphere::getBound: planet views use
the sphere radius; model views use geometry bounds. Converted primary and named
body views without stored locations now retain automaticBody metadata instead
of using a fixed camera distance. The existing body-relative/inertial/LookAt
pose supplies orientation and origin. Native rendering supplies the loaded
model's transformed bounding sphere, including offset, scale and rotation, or
the displayed body's radius. This retains Qt/wx model sizing rather than
changing model size to OF's convention. The software renderer uses its displayed
body radius or unit fallback for a marker; it does not load native mesh bounds.

Camera references must agree with automaticBody and belong to the plot's Add
list. Explicit primary camera pose edits remove automatic framing and imported
body-relative orientation; named views remain independent. Source Undo restores
the imported settings.

NativeOrbit tests frame a translated/rotated textured mesh in a portrait window
and require substantial rendered coverage at normal and high DPI. WorkflowTests
checks converted primary planet/named spacecraft modes, save/reopen execution,
planet radius and center, invalid-reference recovery, manual overrides and Undo.
The preexisting body orientation and LookAt execution checks remain active.
This qualifies Qt-rendered geometry framing, not identical OF model scale or
optional decoration bounds. Segment-relative camera conversion remains open.

Validation: rebuilt the user's GmatQt; all 11 Qt tests passed in 50.58 seconds.
Native checks used isolated Xvfb/software OpenGL. Evidence:
`Qt6ParityValidation/check-auto-body.txt`.

## Named propagation publication metadata

Orbit and ground-track publication now send the publishing command label with
sample data. Qt retains it per plotted point. The engine's default `Unnamed`
summary falls back to the script command name; explicit summary names retain
precedence. Solver-current buffering records labels at collection time and
replays them through the base, OrbitView and GroundTrackPlot implementations.
This metadata does not change propagated values or introduce an OpenFrames
runtime dependency. Existing wx canvases ignore the additional plot action.

CompatibilityTests saves/reopens and executes two named propagation commands,
checks both labels in the orbit samples and checks the total 180-second duration
(60 seconds followed by 120 seconds). It repeats the label check inside a
DifferentialCorrector with SolverIterations=Current and verifies the solved
variable. This is a prerequisite for segment cameras, not completed segment
camera support: anonymous command identity, segment boundaries, selection and
conversion remain open.

Validation: user's GmatQt rebuilt; all 11 Qt tests passed in 44.99 seconds.
Native rendering/window checks used isolated Xvfb/software OpenGL and do not
qualify the user's Intel/Wayland desktop. Evidence:
`Qt6ParityValidation/check-provider-metadata.txt`.


## Focused spacecraft ballistics and mass editor

Audited all controls in wx BallisticsMassPanel: DryMass, Cd, Cr, DragArea,
SRPArea, both SPAD files, both scale factors and both interpolation methods.
The spacecraft property panel now offers **Ballistics and mass…**, with the wx
Spherical and SPAD files groups, numeric units, existing-input file choosers and
engine-provided Bilinear/Bicubic choices. Nonnegative mass/area/scale fields and
finite coefficients are validated together before OK. Cancel leaves the pending
property table unchanged; OK updates that table, and Apply remains the single
mission-edit operation. Other spacecraft properties stay pending alongside it.

WorkflowTests edits all eleven controls, cancels a dialog and file chooser,
recovers from a negative mass, selects the supplied SphericalModel.spo file,
checks pending/configured-state isolation, applies, performs exact-source
Undo/Redo, saves/reopens, and executes a 60-second SPAD SRP mission. Its report
contains the edited mass, coefficients and areas; the reopened SPAD paths,
Bicubic settings and scales are also checked. SPAD drag dynamics and the full
matrix of interpolation/scale settings remain open qualification cases.

Validation: rebuilt application/bin/GmatQt; all 11 Qt suites passed in 39.07
seconds. Evidence: `Qt6ParityValidation/check-ballistics.txt`.

## Wayland default-viewer interaction check

Ran the existing GmatQtWindowTests harness directly on the current Wayland
session with QT_QPA_PLATFORM=wayland and temporary QSettings. No forced software
OpenGL variable was set. This is a separate process with isolated output and
settings; it does not close or change another running GMAT instance. It uses the
same frontend library and startup configuration as the rebuilt GmatQt.

The default mission completed, the ground view minimized, the OrbitView opened
from Output, the ground view restored and minimized again. The test captured
29,380 blue ocean pixels and 953 red trajectory pixels and reported a responsive
event loop after 3.016 seconds. The captured scene was visually inspected and
shows textured Earth, trajectory, starfield and constellation lines. This proves
that sequence on the current desktop in this run, not extended-session or all
viewer lifecycle qualification. Evidence: `Qt6ParityValidation/wayland-windows.txt`
and `Qt6ParityValidation/wayland-orbit.png`.


## Spacecraft hardware attachment and removal

Audited wx TankPanel and ThrusterPanel: individual add/remove and add-all/remove-all
operations, configured type filtering, selected-list order and replacement Apply.
Qt's ordered attachment checklists now offer **Select all** and **Clear selection**.
The shared controls also serve other typed resource-list pickers. Audited the wx
PowerSystemPanel's single readonly selector and empty choice; Qt now offers a
direct typed **PowerSystem** dropdown with **No power system**, retaining pending
Apply semantics.

Accepting complete tank/thruster removal exposed a serialization bug: explicit
`Tanks = {};`/`Thrusters = {};` was interpreted as a hardware name `{}`. Empty
spacecraft hardware lists now omit their assignments, matching fresh-object
engine serialization. ReportFile empty lists use the same omission rule even
when no prior assignment exists. Explicit empty assignments are not introduced
for spacecraft Tanks, Thrusters, AddHardware or AddPlates.

WorkflowTests checks Select all/Clear selection and Cancel on the ordered tank
picker. CompatibilityTests replaces a spacecraft's decoy thruster through its
GUI, selects all tanks, changes their order and applies both lists together.
It checks typed candidates, Cancel, pending/configured-state separation,
exact-source Undo/Redo and invalid tank/thruster rollback. The existing reopened
two-tank finite burn now uses those GUI-selected attachments; its two report rows
verify the 3:2 fuel split, analytic total consumption and coast shutdown.

A separate fixture attaches a nuclear power system through the new dropdown,
checks both nuclear/solar candidates and the None choice, rejects a spacecraft
as a power reference and saves/reopens. A report verifies generated power 10,
bus demand 2 and available thrust power 7.2. After removing that dependent report
command, GUI edits detach power and clear all tanks/thrusters together. Save/reopen
and execution confirm all three attachment fields are empty. Electric tank,
thruster and power-consuming propulsion combinations remain open qualification.

Validation: rebuilt the user's GmatQt; all 11 Qt suites passed in 38.79 seconds.
Native viewer tests used isolated Xvfb/software OpenGL. Evidence:
`Qt6ParityValidation/check-hardware-attachments.txt`.


## Spacecraft visual-model editor and fractional-DPI viewport

Audited wx VisualModelPanel's filename/browse, three rotation and translation
slider/text pairs, logarithmic scale slider/text, Recenter Model, Autoscale Model,
new-file pose reset, Show Earth and the shared orbit/target color controls.
Qt now offers **Visual model…** from the spacecraft properties. Its splitter
pairs grouped controls with the existing Qt orbit renderer; controls scroll on
smaller displays. Wheel/drag/Fit retain Qt navigation. Choosing a new model
resets its offsets/rotations and scale, matching wx; Recenter sets offsets to
zero and Autoscale restores scale one. Orbit and target colors stay pending.

The dialog normalizes numeric edits through setters on a cloned spacecraft,
including offsets limited to [-3.5,3.5], wrapped rotations and scale limited to
[0.001,1000]. Numeric fields show the engine's normalized values on editing
completion. Nonfinite input and unreadable model geometry leave the last valid
preview and report the error. Model paths resolve through the engine's model
path rules and use the same reader as the native orbit viewer. OK updates only
the pending property rows; Apply remains the mission edit. Offscreen/minimal
platforms retain controls/validation but have no native mesh preview.

The Earth size reference uses the standard equatorial radius and a per-object
wireframe flag; this leaves the spacecraft mesh filled/textured. The shared
painter also respects body wireframe settings. WorkflowTests covers Cancel,
sliders, recenter/autoscale, new-file reset, numeric normalization, malformed
model recovery, file/color chooser acceptance and cancellation, pending/model
separation, Apply, exact-source Undo/Redo and save/reopen. It verifies all orbit
state elements are unchanged and reopened propagation still lasts 600 seconds.
Native tests render a textured OBJ in the dialog, change its rotation and verify
changed pixels, then show the wireframe Earth reference. Broader 3DS assets and
material combinations remain open qualification cases.

Direct Wayland execution exposed stale OSG viewport dimensions during widget
resize at fractional display scaling. Initialization now uses physical pixels;
each paint refreshes OSG's context size and viewport from the current widget
size/ratio. The native harness also waits for initial window exposure/configure
before subsequent resize requests, since an outstanding Wayland initial
configure can overwrite an immediate client resize. Centered Fit assertions and
framebuffer-size checks remain in place. Added QtGui.NativeOrbitFractionalDPI
at QT_SCALE_FACTOR=1.5 alongside normal and 2x native checks.

Validation: rebuilt the user's GmatQt; all 12 Qt suites passed in 47.61 seconds.
Native suite rendering uses Xvfb/software OpenGL. The preview capture mode also
passed directly on the current Wayland desktop without forced software rendering,
including the preceding centered-Fit/resize checks, textured preview, rotation,
Earth reference and cleanup. The 2250x1620 dialog capture was visually inspected.
Evidence: `Qt6ParityValidation/check-visual-model.txt`,
`Qt6ParityValidation/visual-model-wayland.txt` and
`Qt6ParityValidation/visual-model-wayland.png`.

After the viewport correction, GmatQtWindowTests was also rerun directly on
Wayland with temporary settings. The default mission, ground-view minimize,
Output OrbitView opening, ground restoration and repeated minimize passed in
2.891 seconds. Its capture contained 21,679 ocean and 685 trajectory pixels.
This is evidence for that sequence on the current desktop, not all viewer
lifecycle or extended-session qualification. Evidence:
`Qt6ParityValidation/visual-default-wayland.txt` and
`Qt6ParityValidation/visual-default-wayland.png`.


## Focused attitude model, orientation and rate editing

Audited wx AttitudePanel's model-dependent field display, coordinate-system
restrictions, orientation and rate representation choices, Euler sequences,
precessing-spinner and nadir-pointing settings, and AEM file chooser. Qt now has
an **Attitude…** dialog with grouped controls, explicit angle/rate units, typed
coordinate/body choices and an existing-file AEM chooser. Initial-state controls
are hidden when the model does not allow them; locked reference frames are not
editable. SPICE models direct users to the spacecraft kernel controls.

The dialog uses spacecraft clones, retaining each model's pending settings while
switching models within the dialog. Representation changes first validate and
submit current pending values, then display engine-converted values. Rejected
numbers, zero quaternions, invalid cosine matrices and failed frame changes leave
inputs available for correction and restore the previous selector. Required
orientation/rate fields must be readable before committing a conversion; a
failed getter cannot silently leave an empty state editor.

Attitude state/rate components are submitted together through GMAT's existing
vector/matrix setters. This also corrects ordinary resource Apply: initialized
attitude clones reject scalar quaternion setters, and ordering must establish
model/representation/sequence before dependent values. No engine mathematics
were changed. OK retains the selected model's complete settings until spacecraft
Apply, including other pending spacecraft changes. The generic table displays
available accepted values and marks original-model fields that are no longer
used; subsequent attitude edits go through the dialog.

WorkflowTests covers dialog Cancel, Euler/quaternion/MRP/DCM round trips, both
rate representations, invalid values and recovery, model-specific controls and
pending-data retention, locked and invalid frames, AEM chooser Cancel, reopening
pending settings, paired DryMass/attitude Apply, exact-source Undo/Redo and
save/reopen. A zero-rate Spinner mission propagates for 600 seconds and reports
quaternion samples that match the reference computed directly from the input
Euler angles through GMAT's conversion utility.
The mission-command tail remains unchanged.

Rebuilt the actual `application/bin/GmatQt` executable; all 12 suites passed in
60.54 seconds. Native suite checks used isolated Xvfb/software OpenGL. Evidence:
`Qt6ParityValidation/check-attitude.txt`. A separate Wayland process with temporary
settings and report files passed the attitude workflow and captured the final
dialog, including unit labels; the capture was inspected for control/button fit.
Evidence: `Qt6ParityValidation/attitude-wayland.txt` and `attitude-wayland.png`.
The direct Wayland command used WorkflowTests' `--attitude-capture` mode, which
runs through this attitude workflow and exits before later workflow scenarios.
This is not a full desktop or long-session qualification claim.

Nonzero-rate dynamics, other Euler sequences/singularities, specialized
PrecessingSpinner/NadirPointing/CCSDS-AEM execution, changing reference-frame
dependencies and broader attitude file/error cases remain qualification work.
The existing SPICE execution coverage does not qualify every operation in this
new dialog. The larger workflow, plugin and viewer acceptance gates remain open.


## Spacecraft orbit frame, representation and anomaly workflow

Audited wx OrbitPanel's frame binding, pending-state conversion, internal
Cartesian cache, epoch ordering, representation restrictions and anomaly labels.
Qt now offers a direct coordinate-system selector. A frame-name string change
alone left the old coordinate-system pointer attached; preview and Apply now
bind the chosen configured frame through the engine's public reference API.
Frame changes refresh available representations and fall back to Cartesian
when a body-fixed or celestial-body-origin requirement is no longer met.

Display changes retain the physical internal Cartesian state between selections,
as wx does. Only edited numbers are interpreted again; all six elements are
submitted together through GMAT's StateConversionUtil and CoordinateConverter.
Conversions use the pending epoch, including precision time when available.
Invalid numbers, dates, missing frames and spacecraft-dependent circular frames
restore the previous selector and leave correction inputs intact. Apply retains
that cached state after setting the epoch: the engine epoch setter can otherwise
reinterpret an already-converted state in the original frame. Preview changes
remain separate from the configured model and mission source.

Orbit values are read as complete converted state vectors. The engine's scalar
MA/EA getters are unsuitable for these displayed values, and its default labels
can still say TA after selecting another anomaly. Qt supplies the selected
sixth-element label and exposes the grouped editing path even when that element
is marked read-only for scalar assignment. Anomaly choices follow eccentricity:
TA/MA/EA for elliptic orbits, TA/MA/HA for hyperbolic orbits. Frame/representation
changes fall back to TA when the previous anomaly becomes unavailable. The
anomaly selector is disabled for representations that do not use it.

The engine's Planetodetic forward/inverse conversions are approximate; merely
re-parsing unchanged display values produced a 1.4 mm X drift in the fixture.
Its standard script writer also omits MA/EA/HA sixth elements through a read-only
filter. Qt resource reconstruction therefore writes Cartesian inputs in the
selected frame for Planetodetic and non-TA anomaly cases, retaining the selected
DisplayStateType and explicit AnomalyType. This applies to other spacecraft
when an unrelated resource is edited or deleted as well. No numerical engine
source or conversion formulas were changed. The saved script's state input
labels can be Cartesian while the GUI continues to display the selected type.

WorkflowTests covers all fourteen representation display round trips, including
Brouwer mean and hyperbolic incoming/outgoing asymptote displays. Frame cases
include Earth BodyFixed, a Moon origin, a barycenter origin, an unavailable frame
and a circular spacecraft reference. Cases include invalid numbers/dates,
pending epoch plus Cartesian edits, configured-model isolation, paired Apply,
unchanged mission-command source, exact Undo/Redo, save/reopen, unrelated Apply
and resource deletion, and actual 600-second report execution. Planetodetic,
mean/eccentric anomaly and hyperbolic anomaly round trips retain their display
settings, correct table labels/values and propagated states. The Orbit tab uses
one frame selector; the deprecated StateType alias and unused action column are
hidden. Column resize behavior remains interactive.

The numerical fixture specifies its initial epoch explicitly, uses 1e-13
integrator accuracy and requests a 1e-10 elapsed-time stop tolerance. It verifies
initial precision epochs within 1 ns. Actual stopping epochs can still differ
by fractions of a microsecond in the existing engine; comparisons bound that
offset below 1 microsecond and align only the endpoint offset using reference
velocity/acceleration. Position and velocity residual bounds remain 1e-6 km and
1e-9 km/s. Frame previews and display round trips use tighter 1e-7 component
bounds. This does not claim bit-identical trajectories or qualification of the
engine default constructor's rounded precision epoch.

Broader element editing/Apply across every representation, coupled elliptic to
hyperbolic changes, singular/parabolic cases, other specialized axes and epoch
formats remain qualification cases. In particular, display round-trip coverage
alone does not qualify all Brouwer mean input conversions. Orbit Designer and
Orbit Summary remain separate unaudited workflows. The overall workflow,
viewer and plugin acceptance gates remain open.


Rebuilt the user's actual `application/bin/GmatQt` executable; all 12 Qt suites
passed in 48.63 seconds. Native viewer/window/plot checks used isolated
Xvfb/software OpenGL and include normal, 150% and 200% native orbit scaling.
Evidence: `Qt6ParityValidation/check-orbit-frames.txt`. A separate process with
isolated settings ran the orbit workflow directly on the current Wayland
desktop, including the preceding epoch/representation tests, all frame/anomaly
cases, round trips and engine report execution. Its `--orbit-capture` mode exits
after this workflow and does not claim full desktop or long-session validation.
The final 2250x1620 capture was inspected for readable columns, selector fit,
unit labels and Apply/Close controls. Evidence:
`Qt6ParityValidation/orbit-frames-wayland.txt` and `orbit-frames-wayland.png`.


## Grouped atmosphere and drag configuration

Audited wx DragInputsDialog's nine controls: F107, F107A, MagneticIndex,
historic/predicted sources, CSSI/Schatten filenames, Schatten error and timing.
The force-model resource editor now offers **Atmosphere and drag…**, including
primary-body, atmosphere and Spherical/SPAD shape selection. Weather sources
and Schatten settings use typed choices; CSSI, Schatten and Exponential input
files have existing-file pickers. The grouped controls retain pending inputs
across model/source changes, reject invalid numbers and files, and keep edits
separate from the configured mission until force-model Apply. A scrolling form
keeps acceptance buttons available at 600 by 440 logical pixels.

Apply adds, replaces or removes the engine-owned DragForce and its atmosphere
model, consuming the dependent properties as one group. It also handles
generic drag property edits. Merely setting the old atmosphere-name property
did not replace the owned atmosphere object. Pending filenames and numeric
settings are applied before attaching a replacement atmosphere. The current
Earth factory qualifier still hides Exponential for an old R2013a limitation;
Qt offers it because the current engine registers it and ships an Earth
Exponential example. Selecting None removes drag through the same atomic
script-edit/rebuild operation.

The engine checks both weather filenames and formats during initialization,
even when sources are ConstantFluxAndGeoMag. Qt now explains this requirement,
keeps the weather pickers available, and validates both retained files before
acceptance. CSSI prediction additionally requires its daily/monthly prediction
sections. A prediction checker accepts either format, so Qt also rejects CSSI
files selected as Schatten and Schatten files selected as CSSI. Directories and
unreadable/missing files are rejected before entering the core format checker.
These checks verify headers/section markers, not all numerical file contents
or temporal coverage.

AtmosphereTests creates independent script-configured reference missions and
compares them with missions configured through ResourceEditor and the dialog.
Constant-source Earth MSISE90, JacchiaRoberts and NRLMSISE00 cases compare
initial/final density and state for 600 seconds of propagation. Changing F107,
F107A and Kp increases the NRLMSISE00 density in the test. CSSI observed data
is exercised at 1 January 2024, and CSSI and Schatten predictions at 1 January
2027; the Schatten case selects PlusTwoSigma/LateCycle. Exponential input-file
configuration and switching from/to NRLMSISE00 are exercised. Comparisons allow
1e-8 relative density differences, 1e-6 km position and 1e-9 km/s velocity
differences. These are calculation-preserving GUI comparisons, not independent
validation of the atmosphere equations.

The SPAD cases configure the spacecraft through **Ballistics and mass…** and
select SPADFile through the force-model dialog. Bilinear and Bicubic
interpolation with scale 1.5 use a temporary copy of SphericalModel.spo.
Reports match independently script-configured cases, and the resulting
trajectory differs from Spherical drag. Missing CSSI and SPAD files cause
runtime failure; restoring the copied files and reopening the unchanged saved
script recovers successful execution. Validation, chooser Cancel/acceptance,
pending-model reopening, a paired ErrorControl edit, exact-source Undo/Redo,
mission-command/comment preservation, save/reopen and complete drag removal
are also exercised.

The NewParameters AtmosDensity implementation already returns kg/km^3 by
multiplying the drag model's kg/m^3 density by 1e9. Its parameter metadata
incorrectly said Kg/m^3; the unit label is now kg/km^3. Numerical output and
dynamics are unchanged. This qualifies AtmosDensity in these scenarios, not
the complete NewParameters plugin. Other Schatten modes, malformed numeric
weather data, coverage boundaries, additional SPAD assets/scales, non-Earth
atmospheres and other force-model operations remain qualification work.

Rebuilt the actual `application/bin/GmatQt`; all 13 Qt suites passed in
79.53 seconds, including the new Atmosphere suite and native orbit/window/plot
checks. The NewParameters plugin was also rebuilt for the unit metadata fix.
A separate process with isolated settings exercised the complete atmosphere
and SPAD workflow directly on the current Wayland desktop. The captured dialog
was visually inspected for readable fields, file controls and visible buttons;
compact-window scrolling is checked in the test. Evidence:
`Qt6ParityValidation/check-atmosphere.txt`,
`Qt6ParityValidation/atmosphere-wayland.txt` and
`Qt6ParityValidation/atmosphere-wayland.png`. The broader workflow, viewer and
plugin qualification gates remain open.


## Legacy Orbit Designer and Summary source audit

The retained Orbit Designer source offers SunSync, RepeatSunSync,
RepeatGroundTrack, Geostationary, Molniya and Frozen design tools. However,
`src/gui/spacecraft/OrbitPanel.cpp` explicitly hides its launcher with
`orbitDesignerButton->Show(false)` and the comment “Remove At-risk Orbit
Designer feature, GMT-3383.” `OrbitSummaryDialog` is read-only text created
by `OrbitDesignerDialog::OnSummary`; it is not a separate action displaying
an existing spacecraft state. These files therefore do not establish a
missing exposed workflow in the current base wx GUI. No Qt design prototype
or new orbital mathematics was added. Active orbit editing and its remaining
representation/frame qualification cases retain their existing scope.

## Ground-station location, appearance and contact qualification

Audited the active wx GroundStationPanel ID, minimum elevation, central body,
state type, horizon reference, three dependent location fields/units and
orbit/target colors. Its station hardware controls are commented out. The Qt
resource editor now has a **Ground station…** action grouping these controls
and an optional horizon-mask input picker with Clear. Coordinate selectors
convert the pending numeric location through the engine, with the selected
body's radius and flattening. Changing the body retains the current numeric
location, matching wx. Cartesian coordinates disable the horizon selector;
spherical fields show latitude/longitude in degrees and altitude in km.
Rejected conversions restore selectors and retain the invalid inputs.
Content scrolls in compact windows while OK/Cancel stay available.

Dialog OK retains pending edits in ResourceEditor; only parent Apply updates
the configured resource and mission script. Apply sets body and representation
before location components. Previously Qt's generic alphabetical property
order could interpret edited location components in the old representation.
Engine latitude/elevation bounds, finite inputs, colors and mask existence are
validated together on a clone. Named colors initialize the picker correctly.
The engine's BodyFixedPoint parameter-label array was missing a comma between
LOCATION_LABEL_3 and LOCATION_UNITS_1, shifting subsequent label lookups; this
metadata defect is corrected. GroundStation's mask setter now accepts an
empty filename so an existing mask can be removed.

StationTests exercises picker acceptance/Cancel, grouped validation, pending
reopening, exact-source Undo/Redo, preserved mission comments, save/reopen,
Earth Cartesian/spherical and Sphere/Ellipsoid conversions, and Mars preview
geometry using its own radius/flattening. Coordinate round trips retain the
physical Earth position within 1e-8 km. A one-day spacecraft/contact scenario
compares GUI-configured station results with independently script-configured
references: default elevation (five events), elevation 25 degrees, and the
bundled Ex_Contact_Location_Station_Mask.txt. Contact endpoints match the
report's millisecond precision; durations allow 1e-5 seconds because coordinate
roundoff can alter the event root's last microsecond. Elevated and masked
cases must differ from baseline. A missing mask fails execution; restoring
it and reopening the saved script recovers matching contacts. GUI Clear
removes the configured mask and restores baseline contacts after save/reopen.
These comparisons qualify calculation-preserving UI behavior, not independent
validation of the contact algorithms.

Station hardware, antenna/custom FOV, media corrections, error models,
non-Earth contact execution, light-time/aberration modes and full EventLocator
configuration remain unqualified. This checkpoint does not close the broader
workflow, viewer or plugin gates.

Rebuilt the actual `application/bin/GmatQt` launcher target and Station plugin.
All 14 Qt suites passed in 53.17 seconds, including the new Stations suite and
native viewer/window/plot checks. A separate isolated-settings process also
passed the entire station workflow on the current Wayland desktop. The final
dialog capture was visually inspected for field readability, grouping, units,
file/color controls and visible action buttons. Evidence:
`Qt6ParityValidation/check-stations.txt`,
`Qt6ParityValidation/stations-wayland.txt` and
`Qt6ParityValidation/stations-wayland.png`.


## Grouped event-locator controls and report failure propagation

Audited the wx EventLocatorPanel common target/body selections, report path and
run mode, write-report toggle, entire-interval toggle, paired epoch-format and
endpoint conversion, search step, light-time/aberration dependencies, and
Contact/Eclipse/Intrusion-specific observer/direction, shadow-type, sensor,
central-body, phase and report-coordinate/grid-file controls. Qt now provides
an **Event locator…** grouped editor with these operations. It also exposes
current ContactLocator report format/time format/precision/alignment and
interval-step settings. Typed target choices include spacecraft and, for
ContactLocator, registered planetographic regions. Observer choices include
ground stations and spacecraft supported by the current engine, excluding the
selected target itself. Changing the contact target refreshes that exclusion. Intrusion
sensor choices include imagers, and changing the target refreshes the
intruding-body list to exclude the target, matching wx.

Entire-interval selection disables the epoch controls. Changing input format
converts both pending endpoints before updating either field; conversion
failure restores the format and retains both original inputs. Opening the
dialog with only a pending format change displays the engine-converted dates
in that format. Apply sets the input format before either endpoint. Turning
off light-time clears/disables stellar aberration and disables the contact
direction selector. FixedGrid enables the grid-frame file and validates a
readable regular file; SensorFrame retains the unused path. Azimuth/elevation
report formats enable their interval step and require a positive value.

Dialog OK validates a clone and retains pending settings; parent Apply updates
the resource and mission source together. List replacement clears/adds typed
engine lists on the clone, instead of passing individual entries through the
generic scalar setter. Wrong target/observer types and reversed explicit
intervals fail without changing the script. Input/output file chooser modes,
Cancel, reopening pending values, compact scrolling and visible action buttons
are tested. A pending edit does not alter the configured step or interval mode.

EventLocatorTests compares Qt-configured missions with independently
script-configured references. A one-day station-contact scenario uses a
19:00-to-midnight explicit interval (two contacts), 30-second search steps,
Transmit and Receive light-time with stellar aberration, and selected ISOYD
SiteViewMaxElevationReport and AzimuthElevationRangeReport output with
precision 8 and a 60-second report interval. The one-day eclipse case compares
Umbra/Penumbra event rows, counts and summaries. The shipped
Ex_IntrusionLocator_Mercury_Sun_Transit example is configured through Qt with
its imager, Mercury intruder and a 120-second step; its report matches the
script reference. Tests also reject phase values outside [0,1] and a missing
FixedGrid file, and cover target exclusions, source-preserving Undo/Redo,
mission comments, exact save/reopen and format-only Apply. Contact endpoints
match the millisecond report precision; numeric report values allow absolute
1e-5 differences (seconds, degrees or km according to the column), with all
text tokens matching. These checks establish calculation-preserving GUI
behavior, not independent scientific validation of locator algorithms.

The failed-output-directory case uncovered two engine reporting defects:
ContactLocator discarded ReportEventDataLegacy's failure result, and the base
EventLocator ignored a false ReportEventData result. The legacy result is now
returned and the base locator throws a named report failure, so Qt reports a
failed run instead of successful completion without the requested file.
Creating the directory and reopening the unchanged saved script recovers
matching contacts. Successful reporting and numerical algorithms are unchanged.

FixedGrid execution with a valid kernel, region/spacecraft-observer contacts,
additional report formats, append combinations, coverage boundaries and
write failures after successfully opening the report remain unqualified.
The broader workflow/viewer/plugin gates remain open.

Rebuilt the actual `application/bin/GmatQt` launcher target and EventLocator
plugin. All 15 Qt suites passed in 56.42 seconds, including EventLocators and
native viewer/window/plot checks. A separate isolated-settings Wayland process
passed the entire contact/eclipse/intrusion workflow. The captured contact
dialog was visually inspected for readable grouped controls, units, scroll
access and visible OK/Cancel buttons. Evidence:
`Qt6ParityValidation/check-events.txt`,
`Qt6ParityValidation/events-wayland.txt` and
`Qt6ParityValidation/events-wayland.png`.


## XY plot setup and plottable parameter selection

Audited the active controls and save path in wx `XyPlotSetupPanel.cpp`.
The Qt resource editor now has **XY plot setup…** with Show plot, Show grid,
engine solver-iteration choices, one X parameter and ordered Y parameters.
The Y list supports selection, remove, drag reordering and Up/Down. The generic
X/Y Select buttons also use the same specialized parameter browser.

The shared parameter browser now distinguishes plottable parameters from
reportable parameters. It offers numeric variables and object properties with
frame/body/hardware dependencies, plus indexed array elements; strings and bare
arrays cannot be selected as plotted values. Array indices must be positive and
within configured dimensions. Duplicate Y selections are prevented. The
existing report, function-argument and command modes retain their filters.

Qualification exposed a Qt serialization gap: XY Y lists rejected array-element
syntax even though the engine supports it. Y-list replacement now accepts array
elements and validates their plottability. Clearing a disabled Y list removes its
assignment rather than emitting an empty parameter reference. Enabling a plot
without X/Y selections is rejected with corrective feedback; wx instead warns
and deactivates that plot. Qt keeps the requested visibility pending while the
user corrects the selections. Turning a plot off retains its existing X
parameter; clearing an existing X is not offered as a supported operation.

`PlotSetupTests` selects `A(1,1)` for X and, in order, a variable, `A(1,2)` and
`Sat.EarthMJ2000Eq.X` for Y through the GUI. It exercises filtering, array bounds,
duplicate prevention, property/frame browsing, picker and dialog Cancel, pending
Apply/reopen, list movement, invalid-input recovery and exact-source Undo/Redo.
After a Unicode-path save/reopen, every X sample is 2, the variable/array curves
remain 3/7, and the spacecraft curve endpoint agrees with an independently
script-configured report within 1e-8 km. The reference elapsed-time check allows
one microsecond for epoch precision. Disabling and clearing the plot survives
save/reopen and suppresses its viewer; re-enabling an empty plot is rejected.

All 16 Qt suites pass, including the new `QtGui.PlotSetup` suite, and the actual
`application/bin/GmatQt` is rebuilt. The full new workflow also passes directly
on Wayland, with the dialog capture visually inspected. Evidence:
`Qt6ParityValidation/check-xy-setup.txt`, `xy-setup-wayland.txt` and
`xy-setup-wayland.png`. Ground-track setup, remaining orbit redraw/camera modes,
full solver-iteration combinations and broader parameter contexts remain open.


## Ground-track setup, texture defaults and redraw limits

Audited wx `GroundTrackPlotPanel.cpp`, its active main-frame route, and the actual
subscriber factory. In this branch the `GroundTrackPlot` script alias creates
`GroundTrack`, whose native callbacks publish planetodetic spacecraft coordinates
and separate fixed station markers. The legacy concrete `GroundTrackPlot` remains
in the source. Qt's new **Ground-track setup…** serves both types, with Drawing,
Data and Other options groups: typed bodies, spacecraft/station checklists,
bulk selection/clear, collection/update frequencies, retention/redraw counts,
Show plot, solver iteration modes and a texture picker/Default action.
The color controls behind `__USE_COLOR_FROM_SUBSCRIBER__` are not enabled by the
current wx source/build; object colors remain editable through their resources.

Changing the body chooses its default map and retains custom choices per body
when returning. GroundTrack marks an empty/default TextureMap readonly to omit
it from serialization; this is not a restriction on the active wx texture
control. Qt now exposes and permits that field while retaining the engine's
serialization behavior. Map validation resolves the engine's texture path and
actually decodes the image, rejecting missing or truncated images before Apply.
The receiver also resolves bare texture filenames through the engine's texture
path, so a body-default filename selected by the GUI renders successfully.

Object-list reconstruction now avoids legacy `DrawObject` assignments for
current GroundTrack, where that property does not exist, and for the legacy
concrete subscriber, where it is readonly. Station selection and deletion reach
the current runtime's separate marker list without leaving stale spacecraft
curves or station markers. Show plot with no selected objects is rejected with
corrective feedback; station-only plots remain supported.

Qt previously discarded the GL `NumPointsToRedraw` argument and ignored the
current GroundTrack field. It now honors the requested recent segments during
runs and replay, including the preceding endpoint needed to connect each segment.
This follows legacy wx `ViewCanvas::ComputeActualIndex` semantics. Current
GroundTrack stores the same setting but its wx native window does not consume
it; Qt gives that existing GUI setting an effect. Retained history and stable
camera bounds are unchanged. `SetGlEndOfRun` and GroundTrack's `RunComplete`
restore the full retained trajectory, and clearing for a rerun resets that state.
`MaxPlotPoints = 1` is now respected by the receiver and model, matching the
engine's positive-integer range.

`PlotSetupTests` qualifies typed selection and map defaults, file-picker
accept/Cancel, a PNG with a recognizable header but no image data, pending
Apply/reopen, compact scrolling/action buttons, exact Undo/Redo and Unicode-path
save/reopen. A GUI-configured Mars plot renders the selected custom image,
verified from actual canvas pixels. Its longitude agrees with an independently
configured body-fixed report within 1e-9 degrees; planetodetic latitude agrees
with the engine's separate state converter within 1e-5 degrees, allowing the
existing ground-track iteration tolerance. The tests verify collection frequency,
retained sample count, one-point retention, update/redraw settings, a real
OrbitView GL redraw callback, default-map recovery, deselected station removal,
station-only execution and disabled-view suppression.

`PlotTests` captures both native OpenGL and fallback orbit drawing, plus 2D
ground tracks. It verifies recent-segment images differ from complete retained
tracks, completion/zero redraw restore full tracks, replay honors the limit,
history remains intact and clearing resets completion state. All 16 Qt suites
pass and the actual `application/bin/GmatQt` is rebuilt. The final setup and
execution workflow also passes directly on Wayland. Compact scrolling and buttons were tested, and dialog/map
captures were visually inspected. Evidence: `Qt6ParityValidation/check-ground-setup.txt`,
`ground-setup-wayland.txt`, `ground-setup-wayland.png` and `ground-view-wayland.png`.
Broader solver-loop display modes, body/station combinations, runtime asset loss
and remaining orbit camera/option workflows stay open; this does not close the
full viewer or replacement qualification gate.


## Grouped OrbitView setup and camera lifecycle

Audited active wx OrbitViewPanel controls and the native initialization path.
Qt now provides **Orbit-view setup…** with ordered space-point Add/Remove/Clear,
Up/Down and drag movement, per-object Draw flags, three object-or-vector camera
selectors with retained numeric fields while switching modes, view/up coordinate
systems, six up axes, positive view scale, drawing/star controls, solver iteration
choices and collection/update/retention/redraw counts. Content scrolls while
OK/Cancel remain outside the scroll area. Perspective and FOV behind the disabled
`__ENABLE_GL_PERSPECTIVE__`/FOV macros are not active wx resource requirements;
Qt's existing live Camera dialog and saved projection comments continue serving
those options. Overlap and subscriber color overrides are likewise inactive in
this base build.

OK validates a clone and returns pending resource values. Apply reconstructs Add
and DrawObject together, preserving flags by name when only Add changes. Visibility
metadata uses the engine's named show-object map rather than including implicit
camera/Sun entries from its runtime boolean array. Empty lists on a disabled plot
are omitted from serialization; explicit empty object/boolean assignments fail
the base interpreter. Shown empty plots, duplicate/wrong-type references, invalid
vector syntax/nonfinite values, wrong frames and nonpositive scales are rejected
without changing the mission. A zero vector ViewDirection is a valid target at the
coordinate-system origin, and remains supported.

The audit found a missing return in OrbitView's full-string ViewPointReference
setter. It fell through into ViewPointRefType, replacing the intended Vector/Object
classification with the supplied string. The return is corrected; indexed vector
interpretation and propagation equations are unchanged. The new workflow checks
complete-vector Apply, its serialized type, and resulting camera history.

Qt previously ignored the UseInitialView callback and reset each new canvas.
It now captures session camera state in the plot model, retaining it after viewer
closure, and restores it on rebuild/rerun when UseInitialView is Off. The retained
state includes zoom, rotation, pan, Fit, projection/FOV and selected camera name;
On restores the scripted camera. New/Open clears this session state. The model
still obtains new tracking history from the engine, and replay begins at Latest
for a new run. Unchanged vector formatting in the grouped editor does not count
as an explicit camera edit, so drawing-only changes preserve imported primary
camera metadata.

The show-object callback now controls body/model visibility independently of
trajectory lines, matching wx Draw Object behavior. Fully undrawn implicit objects
are excluded from Fit bounds and legends; otherwise the implicit Sun could expand
Fit to astronomical distances. A hidden body radius no longer enlarges its
trajectory bounds. Native and fallback captures exercise this distinction.

`OrbitSetupTests` covers object add/remove/duplicate prevention/reorder/clear,
paired visibility, object/vector mode switching, all three camera roles, typed
frames, scale and up axis, drawing/stars/sampling callbacks, compact scrolling,
Cancel/pending reopen, corrective validation, exact source Undo/Redo and Unicode
save/reopen. Reports of spacecraft Earth-fixed XYZ are identical before and after
the GUI configuration. Constant vectors yield the expected eye/target/up history;
object targets and positions track published spacecraft/Luna points. Multiple
reruns and viewer close/reopen retain the adjusted zoom, rotation, pan, Fit and
projection with UseInitialView Off, and On resets them. A drawing-only grouped
edit preserves an imported body/LookAt camera and executes it successfully.

Additional solver-loop modes, named-camera retention during camera-list changes,
more view/up-frame combinations, geometric degeneracies, scene/asset loss and
broader OF conversion qualification remain open. This checkpoint does not close
the full replacement, viewer or plugin gates.


Rebuilt the actual `application/bin/GmatQt` launcher target. All 17 Qt suites
passed in 60.73 seconds, including OrbitSetup, native/fallback plots, native window
lifecycle, conversion workflows and normal/fractional/200% renderer checks. A
separate isolated-settings process passed the entire new workflow on the current
Wayland desktop. Dialog and rendered-orbit captures were visually inspected: the
object/camera controls and action buttons are readable, lower drawing/data options
are accessible by scrolling, and the orbit capture shows the textured Earth,
trajectory, wireframe and configured guides/star field. Evidence:
`Qt6ParityValidation/check-orbit-setup.txt`, `orbit-setup-wayland.txt`,
`orbit-setup-wayland.png` and `orbit-view-wayland.png`.


## Thruster direction, electric models and tank recovery

Audited the active wx ThrusterConfigPanel controls and the thruster branch of
BurnThrusterPanel. Qt now offers a grouped Thruster setup dialog with Local or
configured coordinate systems, dependent axes/origin, direction components,
duty cycle, scale and gravitational acceleration. Electric controls include all
three thrust models, minimum/maximum usable power, efficiency, Isp and constant
thrust. Model changes retain inactive values and enable applicable controls;
electric polynomial coefficients remain in the existing coefficient dialog.
The compact dialog scrolls while its action buttons remain accessible.

Conditionally readonly Origin/Axes are available when switching a foreign frame
to Local; Apply sets the coordinate system before dependent fields. The original
generic application order incorrectly attempted readonly Axes first. Coupled
power limits and zero direction receive corrective feedback before Apply.
Conditionally readonly MixRatio is exposed for first-tank selection, with paired
Tank/MixRatio serialization. Tank pickers and Apply enforce ChemicalTank versus
ElectricTank. Clear-all omits the default empty Tank/MixRatio assignments because
the interpreter cannot read an explicit empty ratio vector. An active burn still
requires a tank; the GUI can retain a tankless hardware configuration and recover
after restoring its tanks. These are GUI configuration/serialization changes;
propulsion mathematics is unchanged.

ThrusterTests configures a chemical thruster and all three electric models through
Qt, plus a below-minimum-power case. The cases exercise direction/model dependency
controls, Cancel, pending reopen, first-tank addition, paired reordered mixture
ratios, coefficient editing, compact layout, Apply, exact Undo/Redo and Unicode
save/reopen. Electric cases deliberately fail without a power system, then attach
a nuclear system through the spacecraft GUI and execute successfully. Ten-second
burn reports match an independently script-configured mission within 1e-8 in
fuel, inertial position, thrust, Isp and mass flow. With power clipped from 10 kW
to 4 kW, analytic fuel losses are 0.0025 kg chemical, 0.0001 kg constant-thrust,
0.0000256 kg fixed-efficiency, 0.00003 kg polynomial and zero below minimum power.
The 3:1 ordered mixture consumes 75% from FuelB and 25% from FuelA. Fuel remains
unchanged during coast and with DecrementMass off. Clearing all tanks survives
save/reopen/build, fails an active burn, and restoring tanks recovers the original
report results. Invalid frame/body/direction/duty/scale/gravity/tank/power edits
leave source unchanged and permit another successful run.

Rebuilt application/bin/GmatQt. All 18 Qt suites passed in 64.27 seconds. The full
new workflow also passed in an isolated-settings process on native Wayland;
its captured dialog was visually inspected for labels, model dependencies and
accessible action buttons. Evidence: Qt6ParityValidation/check-thrusters.txt,
thruster-wayland.txt and thruster-wayland.png.

The impulsive-burn branch, other local/reference frames, shared electric power,
solar/eclipsed propulsion and broader polynomial/tank operating cases remain
open, along with the larger workflow, viewer and plugin qualification gates.


## Impulsive and finite burn resource workflows

Audited the active wx ImpulsiveBurnSetupPanel, its BurnThrusterPanel branch, and
FiniteBurnSetupPanel. Qt lacked the impulsive Tank field entirely because it
was excluded from supported object-array metadata. Foreign-frame Origin/Axes
were also hidden rather than made available when selecting Local. The new grouped
Impulsive burn setup dialog exposes delta-V elements in km/s, Local/named frames,
dependent axes/origin, optional mass depletion, one fuel tank, Isp and gravity.
Mass toggling retains values and disables inactive Isp/gravity. Zero delta-V is
valid. The dialog scrolls in smaller windows with fixed action buttons and keeps
changes pending until resource Apply.

Burn and thruster frame application now share the ordering rule: CoordinateSystem
before Local Origin/Axes. Impulsive tank replacement clears and rebuilds the
cloned list before mass-depletion validation, allowing the first tank and
DecrementMass to be applied together. Empty impulsive tank lists serialize by
omitting the default assignment. Multiple tanks with mass depletion receive the
engine's supported single-tank restriction as corrective GUI feedback. Existing
multi-tank input can remain represented while mass depletion is off. No burn or
propagation mathematics was changed.

FiniteBurnSetupPanel's individual and bulk selection operations are served by the
existing typed Qt checklist, including drag ordering. Apply now rejects wrong-type
or missing thruster names directly; empty lists serialize by omission. Clearing a
finite burn is a valid configuration operation and survives save/reopen/build.
Executing it produces the base engine's explicit diagnosis that the FiniteBurn
identifies no Thrusters. BurnTests confirms the same failure from an independently
script-configured empty burn and successful GUI reselection/recovery; the GUI does
not substitute a silent coast for the engine's rejected active burn.

BurnTests covers seven impulsive cases: inertial, Local MJ2000Eq/VNB/LVLH/
SpacecraftBody, named EarthFixed and zero delta-V. It exercises frame/depletion
control dependencies, Cancel, pending reopen, first-tank paired Apply, compact
scrolling, Isp/no-tank validation, exact source Undo/Redo, Unicode save/reopen and
label/comment preservation. Nine reported state/fuel fields match independent
script-configured missions within 1e-8. Fuel depletion matches the rocket equation
within 1e-9 for a 1200 kg initial spacecraft, Isp 400 s, g0 10 m/s² and the chosen
delta-V magnitude. Inertial components and VNB/LVLH basis transforms receive
independent numerical checks. BackProp restores state and fuel within 1e-9.
Invalid values/references roll back; a selected tank absent from the spacecraft
fails execution, and reselection restores the original report. Turning mass
depletion off and clearing the tank survives round trips and burns without fuel
loss. An imported two-tank mass-off configuration also retains its ordered tank
list through a grouped vector edit, Apply, Unicode save/reopen and execution.

The finite-burn workflow exercises typed individual/bulk selection, clear-all,
ordering, Cancel/pending Apply, exact Undo/Redo and labeled mission preservation.
A reordered pair of GUI-selected chemical engines produces the same position/fuel
reports as the script reference. The ten-second constant-rate burn consumes
0.002 kg from FuelA and 0.003 kg from FuelB, and coast fuel remains unchanged.
Wrong/missing/duplicate selection rolls back; unattached-thruster and empty-burn
failures recover through corrected selection and save/reopen.

Rebuilt the actual application/bin/GmatQt target. All 19 Qt suites passed in
70.19 seconds, including the new Burns suite and existing Thrusters/mission/viewer
regressions. The entire new workflow also passed on native Wayland in an
isolated-settings process. Its dialog capture was visually inspected for readable
labels, selectors, units, group layout and action buttons. Evidence:
Qt6ParityValidation/check-burns.txt, burn-wayland.txt and burn-wayland.png.

Broader burn bodies/attitudes/epochs, fuel limits and finite shared electric-power
combinations remain unqualified. The larger workflow, viewer and plugin gates
remain open; 52 wx workflow inventory entries are still marked Pending audit.


## Ephemeris output setup and generated-file readback

Audited the active wx EphemerisFilePanel controls: spacecraft, output frame, write
enable, format/filename, interpolator/order, editable sampling step, Code-500 byte
order, STK distance/event settings, epoch format and editable interval endpoints.
The commented-out wx StateType control is not an active workflow. Qt's grouped
Ephemeris output dialog supplies these controls with scrollable groups and fixed
OK/Cancel actions. It retains format-specific pending sampling/frame settings,
filters compatible frames, normalizes extensions when changing formats and
converts both interval dates atomically while preserving spacecraft-epoch
sentinels. Failed conversion restores the format and both dates. Cancel does not
modify the resource; OK remains pending until Apply.

Generic property controls now permit typed sampling/endpoints, convert paired
dates and update format dependencies. STK settings remain available when switching
from another format despite their conditional engine readonly metadata. Apply
sets format/epoch format before their dependents, rebinds the cloned spacecraft
and output-frame references, then uses engine validation. A Cartesian CK selection
receives a corrective message; quaternion CK output is not qualified here.

Spacecraft EphemerisName now offers an input-file chooser even when initially
empty. The engine's readonly flag in this state controls serialization visibility,
rather than preventing first assignment. The current OEM/STK/Code500 propagators
read this spacecraft field; their retired EphemFile setters perform no assignment.
Tests configure first input selection through the GUI rather than relying on the
retired field. SPK uses the existing spacecraft kernel list and matching NAIF ID.

Output now lists ephemeris files. OEM and STK text open in the paged report viewer;
binary formats show format/path/size with Copy path and Open folder actions. The
listed path retains custom extensions for non-SPK formats, matching engine
writing rather than the wx new-filename extension suggestion.

Qualification exposed two file-handling defects. An unresolved absolute output
path could yield an empty resolved filename without the setter rejecting it.
The setter now rejects that case. On Linux, the existing stream-based file check
also accepted directories; startup remove() could remove an empty directory and
replace it with ephemeris output. Startup now diagnoses a directory target before
removal. Qt rejects directory selection before Apply, and the runtime check
protects a target changed to a directory after Apply. These are file-handling
changes; propagation, interpolation and time-conversion mathematics are unchanged.

EphemerisTests executes OEM with a custom extension, STK in meters without event
boundaries, Code-500 little/big endian and SPK. GUI exports match independently
script-configured propagation reports within 1e-9 per reported component. The
600-second reference propagation agrees with a circular point-mass Earth orbit
within 1e-6 km position and 1e-9 km/s velocity. Generated files are read through
their actual plugins after GUI step edits, file selection and Unicode script
save/reopen; the state at 300 seconds is checked independently within 2e-4 km
position and 2e-7 km/s velocity. Tests cover group/generic dependencies, chooser
Cancel, pending reopen, failed date conversion, compact scrolling, exact
Undo/Redo, labeled mission/comment preservation, invalid reference/step/order/
interval rejection, Output access, directory preservation and recovery.

FromSpacecraft intentionally clamps a start epoch before coverage to the first
ephemeris epoch. Tests preserve that engine behavior and verify the same resulting
state, reject starts beyond coverage, then recover through the saved script.
Missing generated input files likewise fail and recover after restoration.

CK quaternion, covariance/acceleration output, broader frames/bodies, event
boundaries, segment gaps, backward propagation, exact boundary stepping, multiple
kernels and disk-full/permission cases remain unqualified. The larger workflow,
viewer and plugin gates remain open; 51 wx inventory entries remain Pending audit.

The full new workflow passed on native Wayland with isolated settings. The
compact-layout test waits for window exposure and resize acknowledgement before
checking scrolling and fixed buttons, so initial compositor sizing cannot
overwrite the tested resize. The native dialog capture was inspected: groups,
labels, enabled/disabled fields, long dates, filename and action buttons remain
readable. Observed readback maximum component errors were about 1.71e-6 km
position and 5.20e-8 km/s velocity across these fixtures; SPK was smaller. Native
evidence: Qt6ParityValidation/ephemeris-wayland.txt and ephemeris-wayland.png.

Rebuilt the actual application/bin/GmatQt target. All 20 Qt suites passed in
71.39 seconds with the final test implementation, including native viewer/window
checks, mission/plugin/file regressions and the new Ephemeris suite. Evidence:
Qt6ParityValidation/check-ephemeris.txt. Full replacement acceptance remains open.


## Dynamic-data setup, selective updates and retained viewer data

Audited the active wx DynamicDataDisplaySetupPanel and DynamicDataSettingsDialog
controls. Qt now has grouped Dynamic data setup with row/column resizing,
retained cells and default new cells, parameter selection, text/background
colors, warning/critical bounds, condition colors, double-click editing and
Delete/Clear selected. Both dialogs keep changes pending until resource Apply;
Cancel leaves the preceding state intact. Duplicate parameter references are
rejected because the engine otherwise silently drops them and shifts the layout.
Whole arrays require an indexed element; real and string parameters are accepted.
Numeric bounds must be finite and ordered. Black text retains the wx convention
of automatic condition colors; nonblack text overrides those colors.

UpdateDynamicData command settings provide a typed display selector and a
checklist of that display's actual parameters. An empty selection updates all
cells; an explicit selection retains the engine's cached values for other cells.
Edits preserve command labels/comments and validate the complete mission.

Testing exposed a shared MDI lifecycle defect: close removes a child from the
workspace before its deferred destruction, so immediate reopen could reuse a
still-live but detached window. Reopen now creates a fresh child from retained
plot/table data. Dynamic callbacks also clear stale padding when later updates
contain shorter rows, retain user-adjusted widths on live updates, and update
cached data while the window is closed.

DynamicDataTests configures a two-row grid through the GUI and compares numeric
reports with an independently script-configured mission. It covers real/string/
array values, blank cells, selective/all updates, inclusive normal boundaries,
warning/critical/custom colors, parameter browsing, resizing/clearing, pending
reopen, Cancel, invalid-input rollback, exact Undo/Redo, Unicode save/reopen,
command Apply and saved execution, immediate close/reopen, ragged and closed-table
callbacks, adjustable widths and clearing the entire table. A missing reference
fails full-candidate validation and recovers without changing the source script.
No propagation or numerical algorithms changed.

The focused suite and native Wayland workflow passed with isolated preferences.
The captured setup dialog was inspected for readable fields, grid columns and
accessible actions; compact resizing retains its buttons. Evidence:
Qt6ParityValidation/dynamic-data-wayland.txt and dynamic-data-wayland.png.

Extreme grid dimensions, styling unnamed cells and broader parameter/mission
contexts remain qualification cases. There are now 49 wx inventory entries marked
Pending audit; the broader workflow, camera/OF and plugin acceptance gates stay
open.

The actual application/bin/GmatQt executable was rebuilt. All 21 Qt suites passed
in 102.19 seconds, including native orbit/window checks, plotting/HiDPI, mission
and plugin/file regressions and the new DynamicData suite. Evidence:
Qt6ParityValidation/check-dynamic-data.txt. Full replacement acceptance remains
open.


## Calculated-point resource controls and viewer execution

Audited active wx BarycenterPanel and LibrationPointPanel controls, including
GmatColorPanel as instantiated by these panels (its segment override checkbox is
hidden here). Barycenter membership was absent from Qt's supported list fields.
It is now editable through the existing ordered membership checklist, with typed
celestial-body choices. Add/remove/clear, drag ordering, pending changes and
Cancel use the shared resource editor. Apply replaces the complete BodyNames
list and rejects empty, duplicate, missing and non-body entries. Although the
engine's diagnostic text mentions barycenter members, its actual Barycenter
SetRefObject rejects non-celestial bodies; Qt follows that executable behavior.

Libration points now offer all five L1–L5 choices and primary/secondary selectors
containing celestial bodies and custom barycenters. Spacecraft, other libration
points and SolarSystemBarycenter are excluded, matching the engine's supported
references. Apply validates the final pair so swapping bodies can be atomic,
while identical bodies are rejected. Both resources retain orbit/target color
entry and color pickers, with edits pending until Apply.

Testing exposed a built-in appearance serialization gap: a default built-in
barycenter has no generated script block to replace. Applying a color therefore
returned the specialized-editor error. Qt now writes/replaces only the validated
appearance assignments and reinterprets the whole candidate. It never emits a
Create or changes membership. Built-in editors explain their fixed definition
and expose only orbit/target colors. First color changes, updates to existing
appearance, preserving the other color, exact Undo/Redo, save/reopen and numerical
report invariance are tested.

CalculatedPointTests configures Earth/Luna membership and a Sun/custom-barycenter
libration point through actual GUI controls, then saves/reopens a Unicode script
and runs its dependent coordinate systems and OrbitView. Nine coordinate report
components match an independently script-configured mission exactly as written.
Mass-weighted body positions agree with published barycenter positions within
1e-7 km. GUI-applied Earth/Luna L1–L5 selections are individually saved/reopened
and executed; collinear position/order and equilibrium residuals and triangular
geometry provide independent checks on the selected definition reaching the
viewer. Two-body reorder and single-body membership, empty GUI selection recovery,
wrong references/types, duplicate membership and built-in protection are covered.
No propagation or calculated-point algorithms changed.

The native Wayland workflow passed with isolated preferences. The inspected
resource-editor capture shows readable properties, selection/color buttons,
L1–L5 dropdown and Apply/Close actions. The orbit capture and pixel check confirm
the configured L4 marker appears in its chosen color; nearby Earth/Sat/Center
labels can overlap at this scene scale, so this is not a label-placement parity
claim. Evidence: Qt6ParityValidation/calculated-points-wayland.txt,
calculated-points-wayland.setup.png and calculated-points-wayland.view.png.

Broader bodies, epochs, mass ratios and ephemeris-error boundaries remain
qualification cases. There are now 47 wx inventory entries marked Pending audit.
The full workflow, viewer/OF and plugin acceptance gates remain open.

Rebuilt the actual application/bin/GmatQt target. All 22 Qt suites passed in
84.12 seconds with the final implementation, including native orbit/window
checks, mission/plugin/file regressions, plotting/HiDPI and CalculatedPoints.
Evidence: Qt6ParityValidation/check-calculated-points.txt. Full replacement
acceptance remains open.


## File-interface configuration, import selection and execution

FileInterface already used the generic resource editor's format dropdown and
input-file chooser. Apply now checks for an existing, readable regular input
file and rejects blank names, directories and missing files before changing the
script. The Set command now has typed spacecraft/data-interface reference
selectors and a field checklist with an Import all fields option. A mission
command template inserts Set using current resources when available. Labels,
comments and unrepresented command syntax retain the shared editor behavior.

The field list comes from an initialized clone, leaving the configured interface
and target unchanged while browsing. TVHF_ASCII exposes Epoch, CartesianState
and Cr as assignable fields. Its additional SupportedFields entries are internal
vector components and coordinate/body metadata: selecting individual components
does not assign them, while explicitly selecting frame/body metadata throws.
They are therefore excluded from the picker. Previously entered unknown names
remain visible and checked so Cancel preserves them; OK requires a nonempty
supported subset or All. Returning to All removes the Data clause.

DataInterfaceTests exercises the real input chooser's Cancel and selection,
format dropdown, pending resource edits, Apply validation, exact Undo/Redo and
Unicode save/reopen. It compares all seven nonempty field subsets plus default
All against independently initialized missions, including initial epoch,
Cartesian state and Cr and the state after 600 seconds of propagation. The
reference epoch and state come from the bundled TVHF fixture's literal values;
propagation/coordinate/time-conversion algorithms are unchanged. It checks typed
target/source selection, existing subset and explicit All handling, unknown-field
preservation/rejection, report output and the read-only Output report viewer.

Missing-file, malformed-header, invalid-month, missing-epoch and unknown-field
failures recover after restoring the input or valid script. File repair is tested
with an immediate rerun as well as reopen. Task-9 input also reproduces the
expected report. The shipped Ex_FileInterface example is exercised after Qt OF
conversion, adding reports and shortening its two-day propagation to 600 seconds;
its converted viewer exists and a Unicode save/reopen repeats its report. This
does not qualify the unmodified two-day example or broader OF camera cases.

The native Wayland test passed using isolated preferences. The inspected field
picker capture has readable controls and accessible Cancel/OK actions. Evidence:
Qt6ParityValidation/data-interface-wayland.txt and
Qt6ParityValidation/data-interface-wayland.fields.png.

Broader bodies/frames, multiple records, repeated imports within one mission and
filesystem permission failures remain qualification cases. The 108-entry wx
inventory still has 47 entries marked Pending audit; qualifying Set alone does
not close the generic wx command panel audit. Of the 20 selected runtime plugins,
6 still have no qualification evidence and 14 have partial evidence. Full
workflow, viewer/OF and plugin acceptance gates remain open.

Rebuilt the actual application/bin/GmatQt target. All 23 Qt suites passed in
101.59 seconds, including native viewer/window checks, mission/plugin/file
regressions, plotting/HiDPI and the new DataInterface suite. Evidence:
Qt6ParityValidation/check-data-interface.txt. Full replacement acceptance remains
open.


## Tracking configuration and range-skin simulation/batch estimation

Audited estimation plugin parameter/setter/serializer behavior and the shipped
Ex_Estimate_RangeSkin mission. Compound AddTrackingConfig arrays and measurement,
station and solve-for lists were absent from Qt's supported properties. Added a
dedicated tracking table with add/remove/reorder controls, ordered spacecraft/
station/attached-hardware signal paths, repeated participants, multiple type
selection, pending edits and Cancel. Apply replaces all definitions and validates
the entire candidate script; invalid participants, hardware, types, duplicate
rows/types and malformed settings are rejected. Column widths use the shared
content sizing and remain user adjustable.

Simulator AddData, estimator Measurements, spacecraft/ErrorModel SolveFors and
station ErrorModels/AddHardware now use supported list editing. Simulators and
estimators expose their default propagator without replacing their additional
propagator mappings. FileName/RampTable ordered file pickers support new
simulation outputs and existing ramp inputs respectively. ErrorModel Type has
measurement choices; NoiseSigma uses the measurement's bias unit metadata and
noise/bias units update when the pending type changes. RunSimulator/RunEstimator
have typed selectors and insertion templates. Algorithms and numerical values
are not changed by these controls.

The audit found missing Resources/creation categories for tracking sets, error
models and interfaces, and added those along with calculated points, celestial
bodies, data filters and FOV navigation. Tests open tracking controls through
the actual Resources tree and create/delete unused tracking sets, error models
and file interfaces through the normal resource transactions.

Two script reconstruction gaps surfaced. Empty ground-station hardware/error
lists must be omitted because the engine rejects their explicit {} assignments.
Imager serializes an unset optional FOV as UndefinedFieldOfView, making antennas
fail unrelated resource/mission edits when that nonexistent object is referenced.
Qt reconstruction now omits this diagnostic placeholder only when the actual
reference name is empty. Explicit FOVs remain, including a real FOV named
UndefinedFieldOfView; another unset antenna does not acquire that object.

EstimationTests uses the shipped range-skin example with explicit bounded test
adjustments: simulation from 10 Jun 2012 00:00 to 08:00 UTC, 60-second sampling,
noise off, OLSEAdditiveConstant=1, temporary files and supplementary state reports.
The additive filter margin avoids rejecting the nearly noise-free fit; 60-second
sampling supplies enough observations to solve six state components. The full
two-day noisy example is not claimed qualified by this bounded test. The
selected fixture converges in four iterations.
Estimated initial positions recover the literal input truth within 1e-5 km and
velocities within 1e-8 km/s. GUI configuration reproduces the independently
script-configured mission's fourteen state/epoch report values and serialized
observations. The shared propagation, signal/time conversion and estimation
algorithms are unchanged.

Changing the default propagator previously caused the engine serializer to
omit the old default's explicit spacecraft mapping. Qt now retains the original
compound mapping assignments while applying the new scalar default. The test
selects an alternate default through both simulator/estimator pickers, checks
that their original spacecraft mappings survive, saves/reopens and reproduces
the reference observations and fit. Editing the mappings themselves and broader
multiple-propagator execution remain pending.

GUI coverage includes path/type picking, repeated participants, row reorder,
add/remove, adjustable widths, pending reopen/Cancel, new output selection
without file creation, station error-model clear/restore, solve-for and tracking
lists, type-dependent units, exact Undo/Redo, Unicode script save/reopen, run
command labels/comments and Output report access. Invalid references/types,
negative noise, incompatible Bias/PassBiases and malformed tracking edits roll
back. Estimation-only missing-observation failure recovers after file restoration
without the simulator recreating it. Existing engine validation rejects Unicode
estimator report paths; this is tested as a source-preserving rejection rather
than claimed support. Unicode script paths and observation paths with spaces are
covered. An unset antenna displays an empty FOV value, and selecting a real FOV
named UndefinedFieldOfView through its picker retains that newly applied
reference rather than removing it as an old diagnostic placeholder. Replacing
an existing FOV serializes the new reference even if the cloned hardware still
holds its old object pointer; clearing removes the assignment. Wrong-type FOV
selection rolls back without changing source.

The native Wayland workflow passed using isolated preferences. The inspected
tracking capture shows readable signal/type columns and accessible row/path/type
and Cancel/OK actions. Evidence: Qt6ParityValidation/estimation-wayland.txt and
Qt6ParityValidation/estimation-wayland.tracking.png.

Remaining cases include other measurement/data formats, noisy and real inputs,
DSN ramp/media/relativistic corrections, filters, paired epoch conversion,
multiple propagator mappings, pass biases, covariance/solve-for regimes and
broader RF hardware operation. EKF remains a separate pending plugin. The wx
inventory still has 47 of 108 entries marked Pending audit; this plugin work
does not close generic-panel audits. Of 20 selected runtime plugins, 5 have no
qualification evidence and 15 have partial evidence. Full replacement acceptance
remains open.

Rebuilt the actual application/bin/GmatQt target. All 24 Qt suites passed in
106.28 seconds, including the estimation suite, native viewer/window checks,
plotting/HiDPI, mission/plugin/file regressions and launcher validation. Evidence:
Qt6ParityValidation/check-estimation.txt. This checkpoint leaves the broader
replacement acceptance gates open.

## Paired simulation/filter epochs and batch data editing

Simulator and data-filter EpochFormat/InitialEpoch/FinalEpoch now have a shared
Time interval dialog. Format changes convert both endpoints atomically, in the
dialog and the property table. Cancel retains the earlier pending properties;
failed conversion retains the earlier format and both edited date strings.
Apply uses a clone, sets the format before either endpoint and rejects invalid
or reversed intervals. A format-only resource transaction converts both existing
endpoints automatically, instead of interpreting old text in the new format.

Numeric conversion initially used the converter's Real-based display string.
That rounding moved an endpoint beyond the simulator's nanosecond comparison
tolerance and changed its final sample by 60 seconds. Qt now serializes numeric
epochs using GmatTime::ToString. The bounded fixture's numeric interval survives
Unicode save/reopen and produces exactly the original observations and the same
fourteen state/epoch report values within 1e-8. Conversion back to UTCGregorian
also reproduces those outputs. All engine-advertised representations are cycled
through the dialog. GregorianDate's existing parser accepts milliseconds; the
UI retains that supported representation and explains its precision. This does
not qualify arbitrary fractional-calendar, UT1 or leap-second boundary cases.
The time conversion and simulation algorithms remain unchanged.

Estimator and TrackingFileSet DataFilters now provide typed checklists.
AcceptFilter/RejectFilter expose observed-object, tracker and measurement-type
lists, ordered input files, record numbers/ranges and available thinning modes.
File selection can restore All or an accept filter's From_AddTrackingConfig.
Positive records and ascending ranges are validated before candidate script
interpretation; wrong filter/reference/types, unknown measurements, zero
thinning frequency and invalid/reversed ranges roll back without source changes.

EstimationTests extends the bounded noise-free range-skin fixture with an
independently script-configured accept filter for the observation file,
EstSat and Range_Skin, every-second-record frequency thinning, and a reject
filter for records 1-3. GUI configuration changes the lists, frequency and
record range, converts filter epochs through the property table and selects
both estimator filters. Exact Undo/Redo and Unicode save/reopen retain the
settings. The resulting state/epoch values and measurement residual rows,
including USER edit flags, match the independent reference. Run metadata is
excluded from residual comparison. Clearing the estimator filter list restores
the original unfiltered state/epoch report; the simulator observations remain
unchanged throughout. File sentinel controls and Cancel are exercised as well.

Native Wayland execution passed with isolated settings; the inspected interval
capture shows readable complete numeric epochs and accessible Cancel/OK.
Evidence: Qt6ParityValidation/estimation-intervals-wayland.txt and
Qt6ParityValidation/estimation-intervals-wayland.interval.png. The actual
application/bin/GmatQt was rebuilt and all 24 Qt suites passed in 106.79 seconds.
Evidence: Qt6ParityValidation/check-estimation-intervals.txt.

Level-one filtering execution, time-based thinning, filtered tracker/hardware
and GPS identities, overlapping/multiple filters and interval boundaries remain
to qualify, along with estimator epoch conversion and the earlier broader
estimation cases. This expands partial evidence for GmatEstimation; 47 of 108 wx
inventory entries still await audit, and 5 of 20 runtime plugins still have no
qualification evidence. Full replacement acceptance remains open.

## Script-event comment/body workflow

Audited wx ScriptEventPanel's separate comments/body controls, fixed outer
BeginScript/EndScript labels, resizable sash layout and pending Save/validation.
Qt CommandEditor now offers Script event controls for complete event blocks.
The dialog separates plain comments from a numbered, highlighted script body
with a vertical splitter. Outer labels and inline comments remain fixed;
nested script events, branches and quoted marker text stay in the body. Both
the engine's semicolon-free canonical BeginScript and script-file wrappers are
recognized. Incomplete/non-event text retains the ordinary command source editor.

Opening without changes preserves the exact command string. Comment-only
updates retain the entire body and boundary text. OK updates only pending
command text in one Undo step; Cancel preserves it. Apply uses the existing
whole-mission transaction, so invalid commands restore the earlier model/source
and can be corrected without reopening the panel. Empty bodies are supported.
No mission execution or numerical algorithm was changed.

MissionTests executes a named outer event containing an If/Else branch, a named
nested event, a string containing EndScript/% and a comment containing BeginScript.
Its literal reference result is 10. An invalid Propagate edit fails without
changing that result or source. A corrected GUI body edit and two comment lines
retain both event names and all inline/nested comments and produce 12. Exact
mission Undo restores 10; Redo and Unicode save/reopen restore 12. Clearing the
body removes its commands and produces the expected pre-event result 5.

Native Wayland mission execution and dialog capture passed with isolated settings.
The inspected image shows readable comments, numbered/highlighted nested script,
fixed labelled boundaries, a splitter and accessible Cancel/OK. Evidence:
Qt6ParityValidation/script-event-wayland.txt and
Qt6ParityValidation/script-event-wayland.png. Rebuilt the actual
application/bin/GmatQt target; all 24 Qt suites passed in 106.34 seconds.
Evidence: Qt6ParityValidation/check-script-event.txt.

The ScriptEventPanel row now has executable audit evidence; 46 of 108 wx entries
remain marked Pending audit. Common editor/menu workflows remain under their
separate audits, and broader viewer, plugin and file gates stay open. Full
replacement acceptance is not established by this checkpoint.

## File/report comparison workflow

Audited wx CompareFilesDialog and GmatMainFrame::CompareFiles: three comparison
modes, absolute tolerance, text blank-line selection, baseline/candidate prefixes,
up to three directories, file count and result export. CompareReportPanel is
read-only, unwrapped output with Close; the active default wx comparison path
instead opens ViewTextDialog. CompareTextDialog remains compiled but has no
caller in the current wx sources. Qt now provides File > Compare files and a
Compare action in report viewers, with two-file or directory comparison,
adjustable directory columns, scrollable setup and a resizable result area.

The Qt worker streams UTF-8 input and complete difference output to temporary
storage. Text handles BOM/CRLF and optional blank-line skipping. Numeric-line
comparison reuses GmatFileUtil::CompareLines; numeric columns reuse
GetRealColumns for the existing numeric/UTC representations and report maximum
absolute differences. Engine simulation, estimation and numerical algorithms
are unchanged. Unlike the wx file loop, Qt counts trailing rows, diagnoses
malformed data after the first record and differing column counts, and does not
hang on lines beyond the wx fixed buffer. Lines larger than 16 MiB, non-finite
numbers and non-UTF-8/binary text receive explicit errors. Initial column headers
are skipped; arbitrary initial header/data ambiguity remains unqualified.

ComparisonTests independently specifies text differences, blank-line behavior,
numeric tolerance boundaries, Gregorian/ISO UTC equivalence, column maxima,
extra rows, unequal columns, malformed/non-finite data and missing files.
Three-directory GUI execution checks prefix matching, .truth fallback, backup/log
exclusion, an exact one-file limit, error counts, manually adjusted widths and
picker row identity after removal. Real picker Cancel and Save controls are
exercised using Qt dialogs; native desktop-portal chooser integration remains
unqualified. Invalid settings and Stop retain previous completed results.
Closing an active comparison cancels its worker without accessing deleted UI.

A result larger than 16 MiB remains inspectable through ReportViewer paging;
event processing continues during comparison, and export matches the entire
stored result. Unicode export, explicit replacement, missing-directory failure
and protection of inputs including a symbolic-link alias are covered. A real
engine report produces literal values 2 and 5; opening its Compare action
prefills the baseline and compares equal to the reference. Mission source and
dirty state stay unchanged, and Unicode save/reopen/run reproduces the report.

The actual application/bin/GmatQt executable was rebuilt. All 25 Qt suites
passed in 103.93 seconds; evidence: Qt6ParityValidation/check-comparison.txt.
Native Wayland execution passed and the inspected capture shows readable table
columns, mode/tolerance controls, scrollable setup, paged output and accessible
Compare/Stop/Save/Close controls. Evidence: Qt6ParityValidation/comparison-wayland.txt
and Qt6ParityValidation/comparison-wayland.png. The log includes Qt Wayland
text-input focus warnings during picker handling; this run passed.

Mid-read file replacement, non-UTF-8 formats, broader numeric/header/record
regimes, extreme directory counts and common editor/menu behavior remain
unqualified. The wx inventory has 43 of 108 entries still marked Pending audit.
The selected plugin evidence remains 5 without qualification and 15 partial.
Full replacement acceptance remains open.

## Thrust-file inputs, script actions and repeated viewer lifecycle

ThrustFilePlugin's history and segment fields and Begin/EndFileThrust commands
were audited against their engine implementations and the wx generic resource
editor. Qt now provides history creation, an existing-file chooser, typed
segment/tank/solve-for lists, clear/restore operations and resizable angle/sigma
coefficient grids. Coefficient dimensions remain pending until Apply; Cancel
keeps the prior values. File-thrust command controls select the history and an
ordered spacecraft list while retaining labels and comments.

ThrustFileTests compares GUI-configured missions with independently configured
scripts and verifies a five-kg baseline burn, a 1.875-kg scaled burn and constant
fuel after EndFileThrust. It covers all four acceleration/thrust data formats
with None/Linear interpolation, list clearing, exact Undo/Redo, Unicode
Save/Save As/reopen, input-picker Cancel, invalid-edit rollback, missing and
malformed input recovery and generated report access. The full-day bundled
example depletes 180 kg to a final total mass of 1170 kg and reproduces its
report after save/reopen. Multiple spacecraft/segments, cubic interpolation,
time-varying angles, estimator solve-fors and coverage boundaries remain open.

This execution exposed a GUI file-context bug: stream interpretation did not
set the engine's script directory. Build, Apply and restoration now set it to
the current document's folder; an untitled/default mission uses the startup
folder. Thrust-file Apply uses the engine's matching resolver. Process CWD is
unchanged, preserving startup asset lookup. Relative inputs beside their
script survive Build/Apply/save/reopen. Moving a document with raw Save As does
not rebase its relative asset references; the bundled test explicitly selects
an absolute input before moving the script. General asset rebasing and broader
relative-path consumers remain open.

The wx EditorPanel/ScriptPanel and spacecraft wrapper source were audited.
Qt adds Mission > Save and build script (Ctrl+Shift+F7), Save, build and run
mission (Ctrl+Shift+F5), and Edit > Go to line (Ctrl+L). Saving must succeed
before either combined action builds or runs; empty scripts are rejected.
ScriptEditingTests covers Cancel, exact saved content, Unicode paths, bounded
line navigation without modification, literal 2/5 execution outputs,
save-failure source/identity protection, invalid-script and empty-input recovery.
Multiple inactive documents and broader shared editor/menu operations remain
pending. The spacecraft wrapper maps to the existing focused Qt dialogs and
shared pending resource editor; broad mixed-page transactions remain pending.

ScriptToolsPlugin registers only CommandEcho. Qt adds its On/Off control and
creation template. Its generated script now includes the missing terminating
semicolon, allowing GUI mission lookup/editing without changing execution.
Tests verify pending Apply, retained names/comments, exact Undo/Redo and
save/reopen, trace boundaries, calculation-preserving output, invalid-edit
rollback and restoration of initially disabled/enabled echo settings.
Nested branches and stopped/failed execution remain pending.

WindowTests now repeats the default mission viewer lifecycle three times:
title-bar ground minimize/restore, Output double-click opening, small and large
resize, orbit maximize, immediate close/reopen of both views, and rerun with
ground minimized and orbit closed. Every checkpoint requires retained spacecraft
samples plus rendered blue texture/map and red trajectory pixels; the native
top-level surface and mission source must remain unchanged. X11's default test
also requires top-level main-window minimize/restore.

Native Wayland passed all three viewer cycles in 19.88 seconds with no forced
software-OpenGL setting. The inspected capture shows textured Earth and the
trajectory. Top-level minimize/restore is explicitly unqualified on this
desktop: both a plain Qt window and an isolated Qt MDI/OpenGL-anchor window
report WindowMinimized immediately and WindowNoState one second later.
The strict Wayland test fails that assertion; subsequent programmatic
restoration also fails synthetic Output activation. The successful viewer run
uses the explicit --skip-main-minimize flag and logs that limitation on every
cycle. This is not evidence for main-window compositor minimize/restore.
Evidence: Qt6ParityValidation/viewer-lifecycle-wayland.txt/.png,
qt-minimize-probe.cpp/.txt and qt-minimize-mdi-probe.txt. The probe source can be
compiled with pkg-config's Qt6Widgets/Qt6OpenGLWidgets flags; run without
arguments for a plain window, or with `mdi anchor` for the MDI case.

Native Wayland ThrustFileTests and ScriptEditingTests passed with isolated
preferences and temporary input/output files. Inspected captures show readable
thrust fields, coefficient buttons and script/navigation layout. Evidence:
Qt6ParityValidation/thrust-wayland.txt/.png and script-editing-wayland.txt/.png.
Native script-dialog focus handling emitted Qt Wayland text-input warnings.

The actual application/bin/GmatQt executable was rebuilt; all 27 Qt suites
passed in 122.38 seconds, including strict X11 main-window minimize/restore.
Evidence: Qt6ParityValidation/check-thrust-script-lifecycle.txt. The exact user
launcher was also run from /tmp with its default startup and default mission,
isolated settings, native Wayland and --run/--screenshot. It exited successfully;
the inspected capture shows the textured ground map and propagated track.
Evidence: Qt6ParityValidation/checkpoint-launch-wayland.txt/.png. This CLI
capture proves launch/default execution; native interactive lifecycle evidence
comes from WindowTests using the same frontend and startup.

The wx inventory now has 40 of 108 entries marked Pending audit. Selected
plugin evidence has three without qualification and 17 partial. Full workflow,
viewer/OF and plugin acceptance remains open.

## Existing external Python force-model editing and serialization

The plugin's three configuration fields, its Python import wrapper, owned-force
serialization and the wx propagation panel were audited. No dedicated wx
external-force controls were found. Although ScriptFileName is declared as a
filename, the runtime imports it as a Python module name. Qt now supplies a
module selector from configured Python search directories instead of a file
chooser that would store an unusable absolute filename. Dotted/custom module
names can still be typed. A tooltip explains the module-name and code-cache
contract. The function and ExcludeOtherForces setting are editable alongside
the module on an existing external contributor, with pending Apply and Cancel.

The plugin marked all fields read-only and omitted them during normal owned
serialization. Reconstructing a mission for any GUI resource edit could
therefore lose the external force. The owned serializer now writes the module
declaration before the dependent function and exclusion settings, using the
interpreter's supported syntax. Qt explicitly exposes only the three supported
configuration fields; inherited runtime properties remain protected. No force
evaluation or propagation algorithm was changed.

ExternalForceTests uses the shipped no-API Python model and example, shortened
to 600 seconds. Its six external state components agree with the independent
internal two-body propagation within 1e-5 km / 1e-8 km/s. GUI selection, chooser
Cancel, pending edits and Apply reproduce the independently configured script
report. Exact Undo/Redo, Unicode save/reopen, missing module/function runtime
failure and recovery, invalid-setting rollback and both exclusion settings are
covered. The combined-force result differs from the external-only result and
matches its separate script reference. An unrelated report precision edit also
retains that combined-force result and serialized external configuration.

Native Wayland execution passed with isolated settings and temporary reports;
the inspected capture shows readable module/function fields, selection and
boolean controls. Evidence: Qt6ParityValidation/external-force-wayland.txt/.png. The rebuilt
user executable and all 28 Qt suites passed (118.73 seconds); full log:
Qt6ParityValidation/check-external-force.txt.
New-contributor creation/removal, full-day/API-dependent examples,
package/custom-path persistence, modified-module caching, multiple spacecraft,
variational propagation and malformed callback results remain open. Property-comment
fidelity is also open: the interpreter can attach dotted external-field comments
to the resource declaration, overwriting earlier comments. The serializer retains
attribute comments when present on the owned model, but this does not repair
that import behavior. This is initial qualification of the existing-contributor workflow, not full plugin
qualification. The selected plugin inventory now has two without qualification
and 18 partial; 40 of 108 wx workflows remain Pending audit.

## Shared wx panel and dialog contracts

The five shared GmatBaseSetupPanel, GmatDialog, GmatPanel, GmatSavePanel and
SubscriberSetupPanel sources were reviewed alongside Qt ResourceEditor,
CommandEditor and MainWindow. These are shared contracts, not five independent
mission features. The inventory now records the existing typed controls, pending
edits, cloned validation, script reconstruction, save-before-build/run and
rollback evidence together with specific remaining differences. In particular,
context Help, object-script previews, command summaries, staying open after
Apply, INI layout metadata, complete dialog navigation and multiple inactive
script documents are not qualified.

This is a source audit, not additional execution qualification. Individual
resource and command rows retain their own unverified combinations. There are
now 35 of 108 entries marked Pending audit; audited partial rows still carry
open requirements. The selected plugin inventory remains two without initial
qualification and 18 partial.

## Existing polyhedron gravity controls and checked shape inputs

The three plugin fields, owned serialization, body-shape reader and SurfaceHeight
parameter dependency were audited. Qt now provides a celestial-body selector and
an existing-file shape picker, shows density in kg/m^3, and explains that mesh
coordinates use kilometres. Changes remain pending until Apply. Density must be
finite and positive; body selection must refer to a celestial body. Paired body
and shape edits retain the original field ownership while the canonical body
prefix changes.

The plugin reported itself as a user force although PolyhedralBodies already
declares its contributor. Engine serialization therefore emitted both
PolyhedralBodies and UserDefined, causing any GUI reconstruction to fail with a
duplicate-force error. Its classification now emits only the supported
PolyhedralBodies declaration. All uses of IsUserForce in this engine are
serialization/list construction; gravity and propagation evaluation are unchanged.

The old file loader ignored numeric-conversion failures and did not validate
vertex references before indexing them. The loader now parses into temporary
storage and commits a mesh only after counts, finite coordinates, connectivity,
nondegenerate triangles, paired/oppositely directed edges and positive enclosed
volume pass. This rejects open or inconsistently oriented meshes and globally inward
face winding before gravity evaluation. Relative inputs use the engine's script-directory
search context. Valid vertex/face ordering and the legacy decorative numeric
record labels remain intact. Tabs, CRLF and a missing final newline are accepted.
Self-intersecting surfaces are not detected or qualified.

PolyhedronTests uses a synthetic closed cube centered on Earth with 20-km edges
and density 2000 kg/m^3. This is a controlled input fixture, not an Earth physical
model or asteroid qualification. Its 60-second velocity agrees with an
independent far-field mass approximation within 0.2 percent. GUI selection and
density Apply match the independently configured script's state and legacy
SurfaceHeight report. The parameter browser selects Sat.FM.SurfaceHeight with
the correct force-model dependency; Output opens the generated report. Exact
Undo/Redo and Unicode save/reopen preserve execution. A Mars body/path change
and Earth restoration qualify paired configuration, not Mars propagation.

Ten malformed meshes cover invalid counts, truncated data, out-of-range and
repeated indices, open surfaces, nonfinite/collinear vertices, reversed face
orientation and trailing data. GUI Apply rejects each without changing source;
raw-script failure followed by load/build/run of the valid mission recovers the
reference report. Missing input and invalid density/body recover similarly.
Relative-path save/reopen and an unrelated report precision edit retain the
contributor and calculation. Native Wayland execution passed with isolated
settings; the inspected capture shows readable fields, density units, body
selection and shape browsing. Evidence: Qt6ParityValidation/polyhedron-wayland.txt/.png.
After correcting the separately discovered CommandEcho lifecycle failure, the
rebuilt user executable passed all 29 Qt suites in 126.18 seconds. Combined
validation log: Qt6ParityValidation/check-polyhedron-echo.txt.

New-contributor creation/removal, real asteroid meshes/custom bodies, multiple
bodies/spacecraft, variational and precision propagation, disk-access errors,
geometric self-intersection and broader SurfaceHeight numerical semantics remain
open. The selected plugin inventory now has one without initial qualification
(EKF) and 19 partial. The wx source-audit inventory remains 35 of 108 Pending
audit; this is not full replacement qualification.

## CommandEcho restoration across rebuild, failure and stop

The first 29-suite polyhedron checkpoint exposed a repeatable ScriptEditing
failure when echo was enabled before a subsequent Run. Run reconstructs the
mission, and cleanup of the previous configured commands unconditionally restored
a stale captured setting; newly constructed commands also had an uninitialized
capture field. This could overwrite the current user setting before the next
mission began. The failure is preserved in
Qt6ParityValidation/check-echo-before-fix.txt.

CommandEcho now initializes its runtime state and restores only a setting that
it changed during execution, once per run. Cleanup of an unexecuted command and
repeated cleanup do not overwrite later user settings. Base command cleanup is
retained. Clones and assignment preserve command configuration/name and start
with no pending runtime restoration; the previous Clone returned a default Off
command and the assignment operator's self-assignment condition was inverted.

ScriptEditingTests covers the originally failing repeated Run with echo initially
on, unchanged 2/5 report values, clone script/name fidelity, cleanup before
execution and double cleanup after a later setting change. A Save command's
missing-output-directory error occurs after CommandEcho executes; the run fails
and preserves the initial setting. A stopped While/If mission with a nested Off
command restores the initial setting. Both cases run with echo initially off and
on, then the saved valid script reopens/builds/runs successfully.

Native Wayland execution passed with isolated settings and temporary output:
Qt6ParityValidation/echo-lifecycle-wayland.txt/.png. All 29 Qt suites passed in
126.18 seconds after rebuilding the user's GmatQt executable; combined log:
Qt6ParityValidation/check-polyhedron-echo.txt. More extensive completed nested
branches/loop combinations and malformed argument syntax remain unqualified.
This closes the observed lifecycle defect without claiming full plugin or
replacement qualification.

## GPS Kalman filter, smoother and warm-start qualification

KalmanTests runs a one-hour, noise-free version of the shipped
Ex_FilterSmoother_GpsPosVec.script, retaining its GPS receiver/antenna/error
model, eighth-order Earth gravity, drag/SRP, SNC process noise and estimated
Gauss-Markov Cd coefficient. Seven generated observations feed the EKF and
Fraser-Potter smoother. The test reports the epoch, six Cartesian components and
Cd after each stage. Independently configured script results agree with the GUI
configuration within 1e-8 per reported column; observations and the complete
warm-start state/covariance CSV match exactly. This compares configuration
routes through the existing engine, not an independent filter implementation.

Resources now exposes Process Noise Models and Estimated Parameters. Their
owned models provide writable settings, typed coordinate-system and supported
solve-for controls, acceleration-noise vector cell editing and half-life units.
Spacecraft solve-for lists include named EstimatedParameter resources and its
noise-model picker lists configured ProcessNoiseModels. The smoother's Filter
picker explicitly selects sequential estimators because its dynamic engine
reference-type metadata does not populate the generic picker. RunSmoother has
a typed form/template; RunEstimator choices exclude smoothers. Pending edits,
picker Cancel, exact Undo/Redo, Unicode mission save/reopen and filter/smoother
Output report viewers are exercised. Production choices exclude the testing-only
LinearTime process-noise model; that model is not qualified here.

Warm-start input uses an existing-file chooser and output uses a save chooser;
empty input means cold start and empty output disables CSV writing. The
format/epoch pair applies in format-first order. Format changes convert explicit
dates with GmatTime precision, preserve boundary sentinels and retain the old
format/date after a failed conversion. The suite cycles every supported time
representation and compares converted, format-only, saved/reopened and
independently configured warm starts. It also compares FirstMeasurement and
LastWarmStartRecord continuation results using observations after the selected
seed. A missing prior seed and a seed already at the end of the data fail with
the engine's existing diagnostic, then valid input recovers. Missing GPS/warm
files and a malformed CSV header recover similarly. The simulator is omitted
from input-failure cases so it cannot silently recreate a missing file.

The work exposed two configuration/serialization defects. SNC's vector setter
assigned an accepted vector and then fell through to an unsupported base setter;
it now returns the accepted vector. RunSmoother inherited a formatter hard-coded
to RunEstimator, corrupting named commands into text such as
`RunEstimato 'Smooth GPS'r FPS`. The formatter now uses the concrete command
type. The rejected command-edit evidence is preserved in
Qt6ParityValidation/kalman-command-before-fix.txt. Neither change alters the
filter, smoother, process-noise or propagation calculations.

Native Wayland execution passed with isolated settings and temporary reports.
Exposed-panel captures were inspected for the warm-start, SNC and Gauss-Markov
controls: Qt6ParityValidation/kalman-wayland.txt and
kalman-wayland.{ekf,warm,snc,fogm}.png. The rebuilt user executable passed all
30 Qt suites in 153.67 seconds; combined log:
Qt6ParityValidation/check-kalman.txt. This does not qualify the desktop portal
file chooser or Wayland top-level main-window minimize/restore.

All 20 selected plugins now have initial execution evidence, and all remain
partial qualifications. Creation/removal, full-day/noisy or real observations,
additional measurements and estimation models, covariance controls, residual
graphics, prediction, warm-start smoothing, malformed numeric/covariance CSVs,
relative warm-start files and disk-write failures remain open. Engine solver and
warm-start filenames still reject non-ASCII characters; this suite rejects an
unsupported Unicode output edit without changing source while qualifying Unicode
mission filenames. The wx inventory remains 35 of 108 Pending audit and the
broader GUI/camera/OF acceptance gates remain unfinished.


## Applied-script previews and command/mission summaries

The shared wx inspection workflows were audited in GmatPanel::OnScript and
OnSummary, ShowScriptDialog::Create, ShowSummaryDialog's coordinate-system and
Save As handlers, and MissionTree's command/mission summary actions. wx supplies
summary display names while populating its tree; Qt now assigns temporary names
while reading summaries and restores all original names afterward. BeginScript
uses its matching EndScript's captured state with the script-event name, then
restores the EndScript name. Summary construction and coordinate conversion use
the existing engine; no propagation or numerical algorithms changed.

Resource and command editors have Show script buttons displaying the applied
configuration in read-only, monospaced, unwrapped text. The resource preview is
captured when its panel opens, so an engine rebuild cannot leave a dangling
object pointer. Pending resource/command text is retained and excluded from the
preview. Command editors also offer Summary; Mission tree context menus and
Run > Mission summary provide command/entire-mission access. A local Find dialog
supports searching and Copy; read-only viewers no longer show ineffective
replacement controls.

Summary dialogs provide coordinate-system selection and a Physics commands only
checkbox for entire missions. Spacecraft-origin and spacecraft-dependent axes
are excluded, including configured axes whose reference pointers have not yet
been initialized. Changing frame or filtering refreshes the engine text; an
error restores the previous selection and text. A model-generation guard
prevents an open summary or command panel from resolving an old index after a
rebuild. Run is required, rebuilding/loading invalidates results, and source
edits reject stale summaries. Failed/stopped runs can expose the states that
executed; unexecuted commands retain the engine's explicit no-data explanation.

InspectionTests reports the first and second coast states separately and compares
both six-component command summaries against those reports in EarthMJ2000Eq,
EarthFixed, Moon-centered MJ2000Eq and Earth-Moon barycenter MJ2000Eq frames.
BeginScript's end state matches the first coast, its name is restored, a skipped
If branch produces no fabricated state, and all/physics mission views differ
while retaining the executed propagations. This compares two output routes
through the same engine, not independent propagation mathematics. Actual MDI
resource/command previews preserve pending edits and source, read-only typing is
rejected, local Find/Copy works, and inspection adds no script undo entry.

Save as exports UTF-8 atomically. The Qt widget chooser's accepted and canceled
paths, Unicode output, unwritable-directory retry, and original/current mission
filename protection are covered; symlinks to the mission are rejected as well.
Tests use isolated settings, temporary reports/exports and a temporary engine
output directory. A generic ClearPlot command without a specialized form retains
editable command text: a missing-reference edit rolls back, correction to
MarkPoint applies, and Unicode save/reopen preserves the independent state
report. Runtime negative-mass rejection and a stopped While loop retain prior
command summaries, then a corrected run reproduces the full report.

Native Wayland passed; the saved summary layout was visually inspected:
Qt6ParityValidation/inspection-wayland.png and inspection-wayland.txt.
All 31 registered Qt suites passed across the headless/native runs. The
initial check-qt invocation passed all 26 headless suites; five X11 checks could
not connect to their temporary display inside the sandbox. All five passed
with display access in 37.75 seconds. Both results are retained in
Qt6ParityValidation/check-inspections.txt and check-inspections-native.txt.
This does not qualify the desktop portal chooser, top-level Wayland
minimize/restore, singleton previews, font zoom, broader command/plugin summary
fields or solver-loop summary semantics. Resource Variable/String setup still
needs its own wx audit; the generic property table hides their engine-read-only
expression metadata. Context Help and keeping applied panels open also remain
unfinished. The wx inventory now has 32 of 108 Pending audit; audited rows can
still be partial, and the broad replacement goal remains active.


## Variable/String initialization and direct Array cell controls

Audited the active wx ParameterSetupPanel, ParameterCreateDialog,
ArraySetupPanel and ArraySetupDialog and their GmatMainFrame routing. The active
Variable/String editor shows a disabled name and edits numeric Value or String
Expression. Production creation uses numeric literals and literal strings;
conditional experimental expression/object-selection paths are not enabled.
Arrays accept 1–1000 rows and columns and have a direct row/column Value/Update
workflow as well as the numeric grid.

Qt Variable and String resources now open focused Initial value panels with
applied-script previews. The generic table previously hid the read-only numeric
metadata; setting String Value changed the runtime string without updating its
serialized initializer. The focused Apply path updates the effective source
initializer, interprets the candidate, and checks the numeric value or both
String Expression and Value before committing the source. It preserves grouped
Create declarations, earlier initializers, inline comments and subsequent
mission assignments. Omitted initializers are inserted after the declaration;
optional semicolons and implicit mission boundaries are supported. If the
engine's first mission statement cannot be located safely, Apply reports an
error instead of rewriting executable assignments. No numerical algorithms or
shared engine parser code changed.

GMAT preserves inner apostrophes literally and does not decode doubled quotes.
Its existing percent/comment parser cannot represent every combination of quotes
and percent signs; some inputs build but truncate. Interpreted-value checks
reject those candidates and restore the prior model/source. When a representable
new literal contains percent and the initializer has a trailing comment, the
comment is preserved on its own preceding line to avoid it becoming string
data. Control characters and multiline literals are rejected. This is an
explicit engine-format limitation, not general string-format qualification.

New resource includes typed initial values for Variable/String, preserves each
pending value when changing type, validates duplicate/reserved names and opens
the created resource. Array creation and resizing now match the wx 1–1000 range
on both axes. Numeric grids add synchronized Row/Column selectors, scrolling to
the selected cell, a Value field and Set cell/Enter action. Values remain local
until grid OK and panel Apply; finite-value errors do not change the cell.
Column widths remain adjustable.

Parameters tests exercise actual MDI panels and New resource dialogs, numeric
and Unicode String Apply, pending previews, invalid correction, source/model
rollback, exact Undo/Redo, grouped and omitted initializers, scientific notation,
optional semicolons and implicit mission sequences. GUI-configured and separately
scripted missions produce identical numeric/text reports (including later
mission assignments), and Unicode mission save/reopen preserves both. This
compares configuration routes through the same engine, not independent numerical
implementations. Empty strings, spaces, apostrophes, doubled apostrophes,
semicolons, percent, quotes/backslashes and statement-looking literal data are
covered, together with rejection of known truncating combinations.

Creation Cancel, invalid-value correction without closing, retained values across
type changes, atomic creation Undo/Redo, Unicode String creation and reserved or
duplicate names are covered. Actual New resource creates a 1000×1000 Array; its
last cell is navigated directly, invalid Set is rejected, grid Cancel leaves the
model intact, and accepted pending data applies and survives save/reopen.
Deleting the added resources reproduces the original reports. Settings and
reports are isolated in temporary directories.

All 32 registered Qt suites passed in 172.91 seconds with display access,
including native X11 normal/HiDPI/fractional rendering, viewer lifecycles and
launch. The user executable application/bin/GmatQt was rebuilt. Combined log:
Qt6ParityValidation/check-parameters.txt.

Native Wayland passed and the Variable, String, creation and last-cell layouts
were visually inspected: Qt6ParityValidation/parameters-wayland.txt and
parameters-wayland.{variable,string,create,array}.png. The maximum-grid evidence
covers this host; it is not a scalability guarantee for every system. Desktop
portal chooser behavior, shared Help, keeping panels open after Apply, broader
keyboard/focus operations, and all imported source/parser combinations remain
unqualified. The wx inventory now has 30 of 108 entries Pending audit; audited
rows and all 20 selected plugins still contain unfinished qualifications. The
broader replacement goal remains active.


## Startup paths, import/export and fresh-process reload

Audited wx SetPathDialog, SinglePathSetupPanel and MultiPathSetupPanel: ordered
function-directory lists, text and directory browsing, Add at top, Replace,
Remove, Up/Down, directory errors, output/log updates, and startup read/write.
The optional MATLAB tab is outside the current Linux/no-MATLAB target. wx reads
another startup directly into the live FileManager, even before dialog
acceptance; Qt instead validates and previews the complete imported settings
while retaining the original session until Apply.

Edit > Set paths provides GMAT Function, Output and a read-only monospaced
Startup settings preview. Folder edits, import, and order changes remain local.
Equivalent directories are shown once while retaining their first search
position, because engine callers can add multiple slash spellings. Apply
validates existing directories, probes output writability and checks the
interpreted output/function paths and order before committing. It invalidates
the built mission and closes its applied configuration snapshots; mission text,
modified state and undo history are not changed. Pending configuration panels
block opening Set paths; the action is disabled during execution and re-enabled
after Stop. The mission directory retains the engine's first-search precedence
when a mission is built. Cached data and plugins require a new process after
startup changes; this does not implement hot replacement of those objects.

Startup preview/import uses an opaque value snapshot of FileManager's exact
path/file aliases, ordered include/GMAT/MATLAB/Python lists and metadata,
together with the global modes changed by startup parsing and the receiver's
log destination/enabling. Successful previews, malformed input and directory
validation errors restore all of that state. Preview logging is suspended so
parsing cannot create/truncate candidate logs; resuming the original log uses
append. Qt's wx-only startup compatibility check runs before interpretation.
The snapshot contains no FileInfo pointers. Existing file-entry replacement and
RefreshFiles cleanup now release their owned entries; the old refresh loop
compared begin against begin and never visited them. No propagation, numerical
or estimation algorithms changed.

Output Apply moves the receiver's log name as well as its path; the former
absolute startup filename previously defeated relocation. Startup export uses
QSaveFile and the engine's canonical startup serialization, with pending
OUTPUT_PATH and ordered GMAT_FUNCTION_PATH entries applied. Original startup
comment formatting is not preserved except the engine's saved ## comments.
The mission path and symlinks to it are protected; save Cancel and missing-parent
write failure retain the previous files and permit retry. Save and Apply are
separate operations. Reading a valid file updates pending controls/preview;
Close discards unapplied settings.

The production Qt startup omits the optional wx HELP_DIRECTORY_FILE. The engine
startup writer previously called GetFilename unconditionally and threw before
Set paths could open; missing optional help entries now serialize as comments.
The earlier message-window diagnostic is retained in
Qt6ParityValidation/paths-export-before-fix.txt. A fresh launch from a startup
saved elsewhere also failed because main changed the process directory to the
startup file's directory and FileManager still tried the Windows GMAT.exe name.
Qt now supplies its runtime executable directory to FileManager, and launch uses
the executable's directory for relative startup/plugin entries. The startup-file
getter also retains its stored directory instead of replacing it with the
current process directory. The pre-fix fresh-process failure is retained in
Qt6ParityValidation/paths-relaunch-before-fix.txt.

PathTests drives the actual menu/dialog controls and widget choosers. Dotted and
spaced function directories contain identically named functions multiplying 5
by 2 or 3: GUI ordering selects the expected 10 or 15. Independently scripted
explicit FunctionPath configuration produces the same numeric reports. Default
report/log destinations move to a Unicode output directory, while an explicit
mission report filename retains its destination. The Output tree and report
viewer show the relocated file. Unicode mission save/reopen retains execution.
These compare configuration routes through the existing engine, not independent
calculation implementations.

Malformed startup import changes run/echo modes before failing; validation
checks prove restoration of function/output paths, startup identity, custom
Unicode file alias, Python path order, testing/batch/echo modes and existing log
contents. A missing-root import also rolls back, and an enabled wx OpenFrames
plugin is rejected before changing session settings. Unicode startup
export/read/Cancel/Apply, mission/symlink protection, write-error retry and Save
Cancel are covered. A separately launched application/bin/GmatQt using the
saved startup, isolated settings and reopened mission reproduces the complete
report. A stopped loop re-enables path editing and recovers the same function
execution afterward. Fixtures, settings, reports and the child process's
screenshot are temporary.

All 33 registered Qt suites passed in 135.35 seconds with display access,
including native normal/HiDPI/fractional viewer checks, existing mission/plugin
suites and launch. The user executable application/bin/GmatQt was rebuilt.
Combined result: Qt6ParityValidation/check-paths.txt.

Native Wayland passed; the functions, output and startup preview captures were
inspected: Qt6ParityValidation/paths-wayland.txt and
paths-wayland.{functions,output,startup}.png. This exercises Qt widget choosers;
the desktop portal chooser, broader keyboard/focus, all startup option/file
formats, cyclic aliases, cached-data/plugin replacement and wider permission,
coverage or disk-failure cases remain unqualified. Shared Help and top-level
Wayland minimize/restore remain open. The wx inventory now has 27 of 108 Pending
audit entries; audited workflows and all 20 selected plugins retain unfinished
qualification cases, so the broader replacement goal remains active.

## Solar-system source, files and timing

The wx UniversePanel field and enabling rules were compared with SolarSystem's
actual runtime metadata and setters. Qt now exposes this intrinsic resource in
the tree, with a dedicated panel and no live object mutation before Apply.
Engine reconstruction plus field/file postconditions commits the complete
configuration or restores the previous model while retaining pending controls.
A binary-file preflight rejects incomplete DE records before the engine's
unchecked short header reads; numerical algorithms are unchanged.

The script updater removes only the controlled solar-system configuration
assignments, preserves their comments and untouched configuration, inserts an
ordered replacement before the explicit or identified implicit mission boundary,
and retains the complete command suffix. A SPICE configuration first selects
its DE fallback and filename, then SPICE and its kernels. The Qt canonical
serialization route preserves this pairing for subsequent resource edits,
resource deletion and mission edits; the engine's normal parameter order can
otherwise attempt a DEFilename assignment while SPICE is selected.

SolarSystemTests drives the actual MDI controls, all three installed DE sources
and SPICE, copied Unicode ephemeris/kernel paths, source switching and dependent
controls, Browse accept/Cancel, pending-close Discard/Cancel, correction, stale
panel guards, exact Undo/Redo and Unicode save/reopen. GUI reports match
independently written scripts through the same engine. Earth/Sun and Earth/Luna
frame offsets match the selected body's ephemeris at the reported mission epoch.
Source changes produce distinct DE outputs. Unrelated no-drag spacecraft
changes, create/delete and mission round trips preserve the SPICE reports.
Missing DE/SPK files, truncated DE headers, wrong SPK/PCK types and invalid
interval/source values leave the previous model and source unchanged. The panel
is disabled while running and re-enabled after Stop; the next mission recovers.

[Native Wayland evidence](Qt6ParityValidation/solar-wayland.txt) and inspected
[DE controls](Qt6ParityValidation/solar-wayland.de.png) and
[SPICE controls](Qt6ParityValidation/solar-wayland.spice.png) accompany the
[full regression log](Qt6ParityValidation/check-solar.txt). The tests use Qt
choosers; desktop portal choosers, wider epochs/cache timings, malformed complete
DE content and shared Help remain unqualified. These cases establish GUI
calculation preservation, not independent ephemeris/science certification.

All 34 Qt suites passed in 143.78 seconds after rebuilding the user's
application/bin/GmatQt. The wx inventory now has 26 of 108 Pending audit entries. Audited rows still
contain remaining acceptance cases; celestial-body pages and other workflow,
viewer and plugin/file gates remain open. The overall goal remains active.

## Celestial-body pages, kernels and appearance

The wx body container and its Properties, Orbit, Orientation and Visualization
pages were compared with runtime metadata and setters. Qt now opens a dedicated
four-page MDI editor, with the built-in source, central-body and pole protections
used by wx. User bodies expose their available ephemeris choices and pole values;
Earth has its nutation interval. The New resource dialog includes the engine's
celestial-body types. Dormant wx TwoBody and SourceFilename controls are not
activated; central body and rotation source remain read-only. Imported SPICE
rotation is displayed and can use edited frame ID and kernels.

The editor snapshots values without cloning a CelestialBody, whose destructor
can unload kernels used by other live engine objects. Apply reconstructs the
model from a narrowly replaced configuration block and checks the selected
values/files afterward. Errors restore the previous model and retain pending
controls. Comments, untouched configuration and the mission suffix are retained;
digit-prefixed 3DModel properties are recognized by the configuration updater.
Repeated identical updates do not duplicate assignments or grow the source.

Kernel lists support browsing, replacement, removal and ordered Up/Down moves.
Since script kernel setters append entries, complete membership uses an explicit
empty clear before nonempty lists. The interpreter previously discarded an
empty string array. A narrow serialization fix now forwards an empty braced
SpacePoint kernel list to its existing setter; other string arrays and nonempty
append behavior are unchanged. This lets Luna's startup PCK/FK defaults actually
be replaced or removed through Apply and save/reopen. No numerical algorithm
was changed.

CelestialBodyTests drives the actual controls and file/color dialogs. Earth
physical-property changes affect propagation/altitude/frame reports and the
plotted radius; these outputs match separately written scripts. New Asteroid
Ceres uses a copied Unicode SPK and explicit NAIF ID. Its ephemeris-relative and
body-fixed reports match independent raw configuration scripts. Pole changes
alter body-fixed reports; imported SPICE rotation plus edited frame ID/FK files
changes those reports, and reordering two definitions selects the final loaded
frame. PCK/FK Add/Replace/Remove/order/Cancel, startup replacement/clear and
separate raw empty-list assignments are covered. Unknown-ID and missing-SPK
execution failures recover after GUI correction with the prior reports restored.

Texture selection has a pending preview beside its field. Applied texture,
model pose/scale and color reach the orbit scene while reports remain identical.
Native rendered captures change by more than 2,000 pixels between the original
body, checker texture and posed OBJ body. Invalid real values, read-only edits,
pose bounds, missing/wrong kernel types, duplicate kernels and invalid textures
or models retain the previous source and body settings. Exact Undo/Redo,
Unicode save/reopen, default texture/model clearing, Close Cancel and Run/Stop
editing guards are exercised.

[Wayland run](Qt6ParityValidation/bodies-wayland.txt), inspected
[Properties](Qt6ParityValidation/bodies-wayland.properties.png),
[Orbit](Qt6ParityValidation/bodies-wayland.orbit.png),
[Orientation](Qt6ParityValidation/bodies-wayland.orientation.png),
[Visualization](Qt6ParityValidation/bodies-wayland.visualization.png),
[texture rendering](Qt6ParityValidation/bodies-wayland.texture.png) and
[body-model rendering](Qt6ParityValidation/bodies-wayland.model.png) are saved.
Long form rows wrap at the desktop scaling used for these captures. The
[regression log](Qt6ParityValidation/check-bodies.txt) covers all Qt suites.

The negative SPICE execution cases also emit FILEOPENFAILED/IOSTAT 128 while
writing the existing GMATSpiceKernelError.txt. Correction and rerun succeed,
but error-file diagnostics remain unqualified; the raw evidence retains these
messages. Other open cases include broader bodies/epochs/coverage and pole/frame
conventions, relative kernel paths, texture/model formats/materials, multiple
pending panels, portal choosers, keyboard/focus and shared Help. These checks
prove selected GUI/script calculation preservation through the existing engine,
not independent scientific qualification.

The inventory now has 21 of 108 Pending audit entries; the body-selection dialog
remains pending and audited rows still carry incomplete cases. Viewer lifecycle,
plugin/file acceptance and top-level Wayland main-window minimize/restore gates
remain open. The overall replacement objective is not complete.

The rebuilt application/bin/GmatQt passed all 35 Qt suites in 144.97 seconds.
The separate native Wayland body workflow also passed; it uses Qt widget
choosers rather than the desktop portal.

## Shared celestial-body selection callers

The active CelesBodySelectDialog callers are solar-power shadow selection and
propagation point-mass selection. wx hides Sun from shadows and the current
primary gravity body from point masses; its primary-body choices also reflect
selected point masses. Qt now supplies celestial-body references for both force
lists, including user bodies. The shared checked-list dialog filters hidden
names out of both available and existing selections. Gravity filtering reads
the opposite pending property row at click time, so it follows unapplied edits.
Typed shadow-Sun, non-body gravity names and overlapping primary/point-mass
lists are rejected atomically. The generic mixed-property Apply route also
checks gravity-list overlap against both pending lists.

An exact-report test exposed a pre-existing serialization drift: the power
system constructor's implicit numeric epoch differs slightly from conversion of
its default displayed Gregorian epoch. Rewriting the entire mission during a
body-list edit made that epoch explicit, changing a tested power output from
1.235996012692897 to 1.235996012761969. Body-list-only changes now replace only
those configuration assignments and retain surrounding source/defaults. This
applies to ShadowBodies and pure PrimaryBodies/PointMasses edits. Combined
scalar/list edits still use the broader serializer and require further default
and epoch qualification; no numerical calculation was changed.

Explicit empty solar-shadow lists were also ignored by the interpreter, allowing
Initialize to restore Earth. A narrow empty-list fix calls the existing solar
power clear action and indexed setter's no-bodies flag. It clears prior explicit
membership as well as suppressing defaults. Other empty string-array semantics
are unchanged except the already-qualified SpacePoint kernel clear.

CelestialBodyTests drives both actual MDI callers and shared dialogs. The Sun,
spacecraft, non-body hardware and SolarSystemBarycenter are excluded where
required; configured Asteroid Ceres is offered. Select all, Clear, Cancel,
pending acceptance, Apply, exact Undo/Redo and Unicode save/reopen are exercised.
A pending Earth primary selection hides Earth from point masses, and selected
Sun/Luna/Ceres point masses hide those names from primary choices. The solar
power and propagation reports match separately written scripts exactly. Clearing
shadows stays empty after save/reopen/run and matches a raw script that clears a
previous Earth assignment. Clearing point masses also executes successfully.

Native dialogs are captured in
[shadow selection](Qt6ParityValidation/body-selection-wayland.shadow-selection.png)
and [point-mass selection](Qt6ParityValidation/body-selection-wayland.point-selection.png),
with [native output](Qt6ParityValidation/body-selection-wayland.txt). The prior
SPICE diagnostic-file warnings remain in negative cases. Optional wx calculated-
point mode has no active selected caller and remains unqualified, along with
broader gravity/owned-component transitions, multiple pending panels, combined
power edits/defaults, keyboard/focus/portal behavior and shared Help.

There are now 20 of 108 Pending audit rows. Audited partial rows and viewer,
plugin/file and Wayland top-level minimize/restore gates remain open. The
replacement objective is unfinished.

After rebuilding application/bin/GmatQt, all 35 Qt suites passed in 146.68
seconds: [regression output](Qt6ParityValidation/check-body-selection.txt).

## Source-preserving resource edits and mixed implicit defaults

`ResourceRoundTripTests` exercises actual MDI resource panels rather than only
calling serializers. A spacecraft Cd edit preserves Keplerian input values,
comments, a power system's implicit initial epoch and an unused Asteroid's
implicit physical defaults. Its complete numerical report stays exactly equal
to the original run. Mixed solar-power scalar/shadow-list edits, explicit empty
shadows after prior membership, invalid-edit correction and save/reopen agree
exactly with independently written scripts. The same mixed edit preserves an
implicit mission boundary and its original command suffix.

The new patcher compares matching old/new resource snapshots and rewrites only
changed configuration assignments. It handles dotted owned settings, repeated
left-hand sides, continuations, grouped Array declarations and indexed numeric
cells. Resizing one of two Arrays preserves the other declaration, values and
comments, with reports equal to a separately resized script. Exact Undo/Redo
and Unicode save/reopen are covered. Removed owned coordinate-axis settings
are checked separately, and resource type replacement is rejected.

Force-model selectors create their dependent forces. Patching just a selector
can leave an unchanged subfield before its creator; legacy unqualified aliases
can also survive under the wrong body/model. Owned force edits therefore replace
that edited model's configuration as an ordered unit, including raw aliases,
while retaining other resources and the complete mission source. Atmosphere,
external-force and polyhedron suites cover model/body changes and numerical
report agreement. Optional FOV placeholders are omitted from both snapshots,
so selecting a real FOV named `UndefinedFieldOfView` is retained correctly;
the estimation suite covers selection, replacement and clearing.

The native Wayland resource workflow passed; its power panel was visually
inspected in `round-trip-wayland.power.png`, with raw output in
`round-trip-wayland.txt`. This extends mixed scalar/list/default qualification;
wider alias/dependency combinations, resource deletion, filesystem/portal
failures and remaining inventory gates still require qualification. No numerical
algorithm was changed.

The rebuilt GmatQt passed all 36 suites across the full 31-suite non-native run
and the five native suites rerun with desktop access. The initial sandbox run
could not reach its temporary X display; that failure and the successful native
rerun are preserved in `check-resource-round-trips.txt` and
`check-resource-round-trips-native.txt`. The separate Wayland workflow also
passed. The source inventory remains 20 of 108 Pending audit entries; this
checkpoint does not close the broader replacement goal.

## Source-preserving resource deletion

Resource deletion now removes only the selected declaration and configuration
assignments from the original source. Grouped Create declarations retain their
other Variables/Arrays, and inline/continued comments remain in place. The
mission suffix and implicit power epoch remain unchanged. A missing source
declaration is rejected with an instruction to edit its defining file; the
engine still rejects resources referenced by other objects or mission commands.

`ResourceRoundTripTests` covers exact create/delete/Undo/Redo of a numeric
Variable, creation/deletion of DifferentialCorrector and Yukon resources,
grouped Variable/Array deletion with retained other initializers/comments,
Unicode save/reopen and unchanged independent power/state/array reports.
WorkflowTests continues to exercise pending-panel/stale/reference guards and
the actual confirmation/context-menu deletion path; estimation and parameter
suites cover plugin resources and initialized numbers/strings.

The source audit of wx ResourceTree confirms that bodies/SolarSystem have Open
and Close without a Delete action. They are SolarSystem-owned and cannot be
removed through ConfigManager's generic deletion API. Qt disables that action
and returns an accurate explanation for direct calls, including a configured
user Asteroid; source/model/report retention is tested. Script deletion of a
user-defined body remains a separate text workflow. The first-character
resource declaration case exposed Qt's negative-index lastIndexOf behavior;
configuration removal now uses an explicit beginning-of-file boundary.

The dedicated Wayland create/delete/round-trip run passed; raw output is
`round-trip-deletion-wayland.txt`. Wider include-file, alias and storage-failure
cases, shared Help/keyboard/portal interactions and the remaining qualification
gates remain open. No numerical algorithm was changed.

The rebuilt user's GmatQt passed all 36 suites in 152.07 seconds in one run;
full output is `check-source-deletion.txt`. The separate native Wayland workflow
also passed. This checkpoint preserves the larger goal and its open gates.

## Event report lifecycle and remaining source audit

The wx EventFilePanel reads an existing file only after the running locator's
FileWasWritten flag is true. Qt previously opened any existing path, allowing
an old event report to appear after a rebuild, disabled/manual run, or pending
source edit. Qt now checks the current model/run and running locator before
reading. Unwritten output displays a read-only explanation and the Manual
FindEvents requirement. Generated output retains ReportViewer paging/search/
comparison. Report and binary-ephemeris windows explicitly activate, matching
the existing resource editor behavior; reopening event output after several
build/run/close operations had otherwise left the Script window active.

EventLocatorTests opens the actual Output rows and checks current generated
contents, read-only/no-wrap behavior, stale file retention without display for
rebuilt/edited/Disabled/WriteReport-off/Manual cases, Manual FindEvents recovery, close/reopen
and unchanged source. Existing GUI parameter/epoch/interval/report execution
and save/reopen comparisons continue to pass. Native Wayland generated and
unwritten windows were captured for inspection, alongside the grouped event
configuration: `event-output-wayland.png.generated.png`,
`event-output-wayland.png.unwritten.png` and `event-output-wayland.png`.
The raw native run is `event-output-wayland.txt`.

Ten additional source rows were audited. SolverVariables/Goals/Create are
unused non-writing prototypes; SolverSetup is an uninstantiated reflection
panel whose active generic caller is GmatBaseSetupPanel. Their active Qt
Vary/Achieve/solver factory/settings counterparts already have execution and
round-trip evidence. SQP, MatlabFunction and InteractiveMatlab are outside the
selected no-MATLAB runtime (the interactive dialog also has no current caller).
The active PropagatorSelectDialog maps to the tested configured-propagator
controls. ViewText's comparison/read-only behavior maps to Qt; its About/license
and folder-run callers remain explicit delivery gaps on their own rows. This
separates source audit from implementation and qualification: no missing active
operation is counted as completed merely because its helper was read.

The inventory now has 10 of 108 Pending audit entries. Audited partial workflows,
selected plugin/file limits, shared Help/portal/keyboard behavior and top-level
Wayland minimize/restore remain open. No numerical algorithm was changed; the
broader replacement objective remains unfinished.

The rebuilt GmatQt passed all 36 suites in 155.72 seconds after the report guard
and activation changes (`check-event-output.txt`). The final WriteReport-off
execution case and more accurate explanatory text were then verified by the
focused event suite in 3.84 seconds (`check-event-output-report-disabled.txt`)
and the updated native Wayland run/captures. The actual application was rebuilt
again with that final explanation. The replacement goal remains unfinished.


## 2026-09-30 solver views, desktop Apply and diagnostic recovery

This checkpoint continues the complete Linux replacement objective. Every broad
acceptance gate above remains open until its remaining cases have affirmative
evidence. Completing the source audit is not completing its missing operations.
Windows/macOS remain deferred and MATLAB remains disabled.

The initial native launcher capture showed both default viewers occupying the
same fallback rectangle: Ground Track covered Orbit View. Unspecified (zero
position and size) subscriber windows now receive distinct initial grid cells.
Explicit/scripted geometry is retained, and existing windows moved or resized
by the user leave automatic placement. The rebuilt application/bin/GmatQt,
using its selected startup and isolated settings, now visibly exposes both
textured Earth/orbit/starfield and the ground map/trajectory. The inspected
capture is `launcher-wayland-20260930.png`; its process log is
`launcher-wayland-20260930.txt` (empty, successful process exit).

`window-wayland-cycle.txt` records three native cycles of title-bar Ground Track
minimize/restore, focus/output activation, resize/maximize, immediate close and
reopen and rerun with a minimized Ground Track and closed Orbit View. Rendered
texture/trajectory pixels and mission source retention pass. This run explicitly
skips top-level compositor minimize/restore; that gate remains open. The strict
run `window-wayland-strict-20260930.txt` retains the failure: Qt reports a
minimized state then restores/maximizes before the delayed assertion. Earlier
plain Qt/MDI probes also fail that assertion; this is not proof of compositor
behavior or a completed gate. A read-only GNOME window-state query was rejected
by the compositor's AccessDenied policy; desktop settings were not changed.

Ground Track previously retained trial solver samples even with None selected
and did not receive the Current iteration breakpoints. Its subscriber now
filters None before sampling, sends display-only solver state, and joins the
solver's active display subscribers. Qt stores trial/accepted classification,
uses target/orbit colors and removes old Current trial arcs at breakpoints.
SolverPlotTests selects All/Current/None/All through real property controls,
preserves exact source Undo/Redo and Unicode save/reopen, and compares complete
numeric reports byte-for-byte with an independent no-display solve. The target
objective is checked to 1e-6; Ground/XY accepted histories agree within 1e-8
(display coordinates) and 1e-10 days (epoch). Native/fallback frames, camera
tracking, replay/latest restoration and immediate close/reopen are checked.
`solver-wayland.txt`, the tiled `.Current/.None/.All.png` captures and their
individual `.Orb/.Ground/.XY.png` scenes retain native evidence. Broader
optimization/nested/toggle/solver-summary combinations remain open. The optional
new ground actions are ignored by the wx receiver; wx Current visual parity is
not claimed. No numerical solver algorithm was changed.

About/license now provides the configured offline license verbatim, current
engine/build/runtime details and credits. `desktop-wayland.txt` and its
`.about.png`/`.license.png` captures record menu opening, read-only/search,
Close/Escape, compact layout, file error/correction and pending mission retention.
Earlier failed captures used relative paths after changing to the startup
folder; those harness failures are preserved in the two
`*-wayland-capture-path-failure.txt` logs. Capture destinations are now resolved
before changing working directory.

Successful resource Apply now refreshes the editor in the same MDI window from
the accepted engine model, so a second edit uses a new validated snapshot.
Clean companion resource panels also refresh, retaining page/filter/columns.
Other pending panels are retained and remain guarded against stale Apply.
DesktopTests checks repeated actual workspace Apply, pending companion retention,
stale rejection, exact Undo/Redo, Unicode save/reopen and independent numeric
reports. Native Wayland passes. Command/insertion windows still close accepted
snapshots; context Help, wider body/SolarSystem/dependent refresh, keyboard/focus
and portal interactions remain open. A refresh error is reported with close/
reopen recovery rather than escaping the Qt action callback.

The SPICE FILEOPENFAILED/IOSTAT-128 problem was reproduced independently: the
bundled WRLINE tries to create a new diagnostic file and fails if the previous
file exists. Initialization now preserves that file and exclusively probes an
unused sibling (GMATSpiceKernelError.1.txt, .2.txt, etc.) before selecting it.
RETURN is configured before diagnostic setup. Unwritable or over-255-byte paths
fall back visibly to SCREEN while GMAT still receives the kernel error. The
filename bound follows NAIF's [errdev_c contract](https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/cspice/errdev_c.html).
SpiceDiagnosticTests uses independent processes to verify flushed raw contents,
byte-preserved previous files, repeated launches, fresh output, Unicode missing
kernel errors, permission/long-path fallback and corrected load/unload.
`check-spice-diagnostics.txt` retains verbose execution. Historical negative-run
FILEOPENFAILED evidence remains in earlier logs. Concurrent diagnostics races,
post-probe permission/storage changes and wider kernel/segment cases remain
unqualified; no numerical SPICE calculation was changed.

The ten remaining inventory entries have now received source/caller audits.
FileUpdate is explicitly unimplemented on non-Windows in the wx utility;
TextEphem's prototype menu is disabled; OpenGlOption's only frame creator is
unused/uncompiled. Active debugger stepping/inspection, mission-only undocking
and filters, welcome/recent/sample navigation, bulk parameter selection and
folder-run reporting remain concrete delivery gaps on their audited rows.
The inventory has zero original Pending audit rows, not zero incomplete rows.

Validation before the final default-placement change: all 40 suites passed in
187.98 seconds (`check-solver-about-spice-apply.txt`). The final rebuilt
application passed all 40 suites in 172.30 seconds
(`check-linux-checkpoint-20260930.txt`), including explicit/manual viewer
position retention. The focused Desktop test also passed (`check-desktop-final.txt`).
The Linux replacement goal remains active and unfinished.


The final strict Wayland repeat still fails the top-level minimized-state
assertion (`window-wayland-strict-final.txt`); its texture/trajectory capture is
`window-wayland-strict-final.png`. The complete non-skipped viewer portion then
passed three cycles again, including explicit/manual geometry retention
(`window-wayland-cycle.txt`, 22.44 seconds), and the final solver capture run
passed. The native Desktop capture was refreshed after the small final About
mailto-link addition; its focused regression passed in 0.35 seconds. The full
40-suite run predates only that link/test addition and final whitespace cleanup.
The user's executable was rebuilt with all final source changes. Commits
19be5d1 (SPICE), 5c01a5e (solver display) and 423ebea (desktop/viewer behavior)
keep the changes reviewable on codex/qt6-gui. No acceptance gate is silently
closed by this checkpoint.


## 2026-09-30 context Help, command Apply and Wayland protocol evidence

The earlier strict Wayland failure at `isMinimized()` was a harness assertion
that this platform cannot support. The installed xdg-shell protocol explicitly
does not report minimized state. The exact [Qt 6.10.2 platform implementation](https://raw.githubusercontent.com/qt/qtbase/v6.10.2/src/plugins/platforms/wayland/plugins/shellintegration/xdg-shell/qwaylandxdgshell.cpp)
sends `set_minimized()` and immediately removes WindowMinimized from its client
state (requestWindowStates). WindowTests now retains the X11 assertion and checks
exposure after restoration on Wayland instead of asserting an unavailable flag.
This corrects the test; it does not force application window state.

`window-wayland-protocol-20260930.txt` records the minimize request, continued
event-loop progress and the surface becoming unexposed. Programmatic showNormal/
raise/activate did not re-expose it without a desktop input gesture, so the
updated check fails **restoration exposure**, not the unsupported minimized flag.
The trace alone is not independent evidence of the compositor's unreported
minimized state. Actual desktop input restoration remains unqualified. The
compositor's read-only GetWindows API denied introspection; no desktop security
or access setting was changed. The raw historical assertion failures are retained.

Offline context Help is now delivered through the Help menu, resource and command
Help buttons and F1, including ownership across modal-window boundaries. Engine
type aliases resolve to the existing documentation topics. Pending command text
updates its Help topic; modal ballistics/attitude/visual-model editors receive
specific spacecraft topics. The built-in read-only viewer provides page search,
Back/Forward, Topic, Contents, Retry and Close/Escape, plus explicit Open in browser.
Local missing/read/malformed UTF-8 errors remain visible and corrected files can
be retried. QSettings Help/type overrides are supported. Standard file/color/font
choosers retain their own button arrangements.

The source R2026a DocBook build succeeded using the repository's existing Makefile
and bundled Java tools. New explicit `build-qt-help` copies the generated user
guide to the selected runtime's default HELP_PATH; normal GUI compilation does
not acquire a Java requirement. Existing package install rules include those
files. HelpTests requires this prerequisite, checks the actual generated pages,
button/F1/modal routing, search/navigation/Escape, missing/malformed/corrected
files, overrides, aliases and exact preservation of pending resource values,
mission text and Undo. `help-wayland-20260930.txt` and its `.spacecraft.png` and
`.ballistics.png` captures passed on native Wayland and were visually inspected.
External browser/web invocation is implemented but not executed by this test.

Command Apply now reconstructs the accepted command controls in the same MDI
window with a current snapshot. Before/after/append insertion switches to editing
the first accepted command, so another Apply cannot repeat the insertion. After
a branch, the accepted index skips the anchor's descendants. Failure retains
pending text and accepted source. DesktopTests exercises actual tree/context-menu
routes, repeated Apply, failure recovery, before/after-branch/append insertion,
exact Undo/Redo, Unicode save/reopen and byte-equal numerical reports against an
independent script. `desktop-command-apply-wayland.txt` passed natively. Clean
resource companions refresh as before; wider command companion/body/SolarSystem
refresh and shared keyboard/portal/document cases remain open.

All 41 suites passed in 178.71 seconds (`check-help-command-apply.txt`). The
application was rebuilt after the small final Help history-status and standard
chooser exclusions, and focused Desktop/Help tests passed
(`check-help-command-final.txt`). That full suite predates only those small Help
changes; no claim of a later full rerun is made. Windows/macOS and MATLAB remain
deferred. The full replacement goal remains active: folder-run, debugger, mission
filters/undocking, welcome/navigation, bulk parameter operations and the remaining
viewer/plugin/file/shared-desktop acceptance evidence still require completion.


## Folder-run delivery and Linux qualification

Mission / Run scripts from folder now opens Options, Results and Plots in a
separate modal workspace. The immediate folder list is sorted by name and includes
.script/.m missions; function definitions and backup suffixes are excluded. The
starting number and count select that list before two independent include/exclude
filters. Repeats reuse the built engine objects. Output can use Run_1, Run_2 and
so on, with ReportFile/EphemerisFile filenames re-resolved for each repeat;
explicit destinations retain their configured paths.

Saved copies retain exact source bytes, including UTF-8 comments. They are run
from the selected copy folder, while relative includes/assets resolve from the
original source folder. This provides the wx save-copy/run operation, not a
self-contained asset archive. OpenFrames missions automatically offer conversion
for the Qt viewer; acceptance converts the in-memory build, retaining both source
and saved copy. Unsupported conversion reports the actual reason as a build
failure, and declining skips that script.

Numeric ReportFile comparison uses the existing absolute-tolerance line comparison
on a worker while the GUI continues delivering Stop. Reference basenames replace
GMAT with the requested text. Results include each repeat, loaded path, output
path, engine status, failure details and comparison totals. A summary is saved in
the output folder; comparison export is optional. Summary/report collisions select
an unused numbered summary. Export rejects missions, saved copies, baselines,
active reports and the summary as destinations, including resolved aliases.

Build, initialization, unknown initialization, runtime, unknown runtime,
interrupted, read, copy and output errors have distinct categories. Following
missions continue after ordinary failures. Stop interrupts active engine execution
or prevents subsequent runs; Close/Escape while busy requests Stop. Options and
Run recover afterward. Each script's diagnostics are captured independently of the
10,000-line Message Window cap. Logs use the batch output directory and the prior
log destination/enabled state is restored.

Pending configuration panels block batch execution without losing their values.
Clean panels close before their referenced engine objects are replaced. The open
script text, identity and Undo/Redo are retained, including unapplied text edits.
Original output/ephemeris/source-folder/log/batch settings and the accepted engine
model are restored after success and failures. Normal viewer histories remain
available. The final batch OrbitView/GroundTrack and solver progress windows stay
in the dialog's Plots workspace after restoration; Tile and Cascade are available.

FolderRunTests covers the delivered menu/dialog, sort/range/filters/functions/
backups, two repeats, output directories, source-exact copies/relative includes,
independent numeric reports, comparison exports, build/initialization/runtime
failure recovery, malformed UTF-8 repeated reads, active and between-run Stop,
pending-resource protection, failed output path recovery, exact document Undo/Redo,
normal viewer history, baseline/report overwrite protection, retained textured
OrbitView/GroundTrack scenes, OF conversion and repeated converged solver windows
without leaking into the ordinary workspace. The native Wayland test passed with
captured Results and Plots images (folder-run-wayland-20260930.txt and associated
PNGs); those images were inspected for readable output, textured Earth, starfield,
map and spacecraft trajectory. These routes use actual Qt widgets with synthetic
input; desktop portal chooser interaction is still unqualified.

The first complete regression attempt retained in
check-folder-run-prompt-timing-failed.txt timed out in the new native folder test.
Its single-shot acceptance callback ran before the OF prompt existed after queued
messages were drained. Polling for the actual prompt corrected the harness; the
focused final folder test passed in 2.98 seconds. The raw failed log is retained.
Large comparison files are streamed by the comparison engine, but exceptionally
large combined result text and disk exhaustion/race failures are not qualified by
these bounded cases. Full replacement remains in progress under the acceptance
gates and the remaining active-workflow/desktop requirements.

Final folder-run regression: check-folder-run.txt records all 43 Qt suites passing
in 167.72 seconds, including NativeFolderRun and the existing native X11
viewer/window suites. application/bin/GmatQt was rebuilt with these final changes.
The separate Wayland folder test/captures passed; this does not close the outstanding
GNOME top-level minimize/restore acceptance gate.


## Mission-tree filters and mission-only window qualification

The Mission tab now has Show all, Filter, expansion-level and Undock controls.
View also exposes Undock. Include/Exclude Apply filters the existing tree without
changing engine configuration or source. The sorted checklist uses runtime factory
commands and current mission types; Equation maps to GMAT and ScriptEvent to
BeginScript. Check all/Uncheck all change pending check states. Close/Escape before
Apply retains the existing view; successful Apply remains available for adjustment.
Ancestors of matching nested commands and branch boundaries stay visible. Script
Events remain complete source blocks represented by their BeginScript node, as in
the existing mission editor. An empty Include hides commands; All recovers them.

Collapsed, levels 1–3 and Expand all set expansion. The applied filter and level
survive normal tree rebuilds in the current session. Undock moves the same tree
and toolbar into a mission-only floating dock, leaving Resources and Output in
the original navigation. Selection and all editing/context callbacks stay on that
same tree. Dock, closing the floating window, or docking it into the main window
restores the Mission tab. The detached placement preference is persisted, and
restored after the main window's saved layout, avoiding an empty saved floating
window. Filter/depth choices are session state.

MissionNavigationTests covers Include/Exclude, Equation/ScriptEvent aliases,
Check/Uncheck All, closing unapplied changes, nested If/For boundaries, empty
Include/All recovery, expansion, pending source/snapshot and exact Undo/Redo,
three undock/close/restore cycles, selection/other tabs, saved placement and docking,
actual editing from the floating tree, Unicode save/reopen and byte-exact reports
against independently configured source. The existing Mission suite verifies
keyboard focus and Edit-menu targeting. A regression caught the reparented tree's
explicit hidden state; tree visibility is now restored when adding the controls.

mission-navigation-wayland-20260930.txt records a passing native Wayland run with
exposed filter and floating windows; filter/undocked/docked PNGs were captured and
inspected for readable controls and the familiar Resources/Mission/Output,
workspace/messages arrangement. The native Qt Wayland log includes text-input
surface warnings during synthetic focus changes. These widget routes establish
covered operations; fresh compositor keyboard/pointer interaction and top-level
minimize/restore remain under the shared desktop gate.

check-mission-navigation.txt records all 44 Qt suites passing in 155.31 seconds
after the final mission-tree changes. The user's application/bin/GmatQt was
rebuilt; the separate native Wayland navigation test/captures also passed.


## Welcome, recent missions and sample browsing

Help / Welcome opens one reusable nonmodal Welcome window. Normal interactive
startup without a requested script, run or screenshot shows it according to the
saved Welcome/showOnStartup preference, initially enabled. Suppressing automated
run/screenshot modes keeps those requested outputs unobstructed. The manual menu
remains available when startup Welcome is disabled.

Welcome provides New mission, recent missions with full-path tooltips, Open
selected/double-click, Clear recent list, configured sample folders/Browse and
in-app Using GMAT, Reference guide and Tutorials. Project/support/video links
come from the runtime bin/GMAT.ini Welcome/Links and GettingStarted/Tutorials
entries. Additional local HTML links validate the file before asking the desktop
to open it. Sample folders come from Welcome/Samples, falling back to the runtime
samples directory. The scrolling content preserves visible Close and the startup
preference in compact windows.

File / Recent missions shares the same history and active-document protection.
Successful UTF-8 opens and Save/Save As move their absolute path to the front of
at most ten entries; equivalent absolute/canonical paths are deduplicated. A
script opened successfully remains recent even if its subsequent build fails,
matching file history rather than a list of successful missions. Clearing history
removes settings only, retaining every source file. Unavailable recent files stay
visible with their path and a recoverable error. Pending panel/script protection
uses the same confirmed Discard/Save/Cancel flow as File Open and New. Welcome
closes after a successful open/build; failed/canceled operations retain it.

WelcomeTests exercises the menu/single-window behavior, enabled/disabled startup
preference, load/Save As/equivalent-path ordering and path identity, recent menu,
all three offline topics, pending source/identity/Undo retention, dirty Cancel and
Discard, unavailable-file handling, sample widget Browse Cancel/accept, New mission
Cancel/accept, compact footer and Clear recent list retaining files. Selected
numeric reports match independent script-configured values. Native Wayland
welcome-wayland-20260930.txt and its exposed-window PNG passed and were inspected.
The test explicitly uses widget file choosers; portal interaction is unqualified.

welcome-launch-wayland-20260930.txt records separate self-launched instances of
actual application/bin/GmatQt with isolated INI settings and no mission argument.
The enabled preference requests a native Welcome to GMAT surface; the disabled
preference requests only the main application window. Raw Wayland traces are in
/tmp/welcome-launch-true.txt and /tmp/welcome-launch-false.txt; the retained evidence
contains title requests and provenance. Only those two test-owned processes were
terminated. WelcomeTests separately establishes exposed rendering and operations;
protocol title requests alone do not prove physical compositor focus. External
browser/project/video links are implemented and source-audited but were not opened
by these tests. Full replacement still requires the outstanding shared desktop,
parameter-selection/debugger/document and remaining viewer/plugin/file gates.

check-welcome.txt records all 45 Qt suites passing in 162.67 seconds after the
final Welcome/recent/document changes. application/bin/GmatQt was rebuilt; the
separate final Wayland Welcome test/capture and actual launcher preference checks
also passed. These results retain the outstanding native desktop/portal limits.


## Original command source and bulk parameter qualification

ParameterSelectionTests exposed an unrelated source-retention defect: command
Apply previously edited the engine-generated complete script, rewriting resource
definitions and making implicit defaults explicit. MissionModel now maps the
accepted command graph back to original source spans and edits that source. It
shares the existing quoted-literal/comment/continuation statement scanner used by
resource patches. Sequential alignment plus branch ranges retains duplicate-command
identity. Numeric formatting, optional engine-filled dictionaries (including
Write), and For's generated implicit unit step are recognized for alignment.
The editor retains original options, labels, comments, semicolon-free syntax and
implicit configuration rather than writing the generated equivalents.

CommandForm now accepts leading comments and omitted terminators in simple
commands and For/solver headers, while changing only recognized header fields.
Nested bodies remain in the original statement. A source sequence that cannot
be aligned safely, including a mission-expanded include without a local span,
remains a text edit; no canonical whole-script fallback is used. Such cross-file
command editing is still unqualified. Configuration includes before the mission
are outside command alignment and retain their original text.

ResourceRoundTripTests verifies command label editing preserves the exact original
configuration, implicit SolarPowerSystem epoch and unused body defaults, with
byte-exact propagation/power/array reports, exact Undo/Redo and Unicode save/reopen.
It also verifies command editing without an explicit BeginMissionSequence leaves
that boundary implicit and calculations unchanged. MissionTests covers a labeled
start:end For loop without semicolons, its implicit unit step, leading/inline/body
comments, exact header edit/Undo/Redo/save/reopen and the independent sums 3 and 6.
Existing nested branch, duplicate identity, insertion/deletion, script-event,
solver and invalid-edit tests remain applicable.

The compatibility Save repeat fixture now sets its spacecraft NAIF identifiers
explicitly. Auto-assigned identifiers change on fresh builds; the old whole-script
regeneration had accidentally made them explicit. The fixture retains byte-exact
repeat and failure-recovery assertions with stable intended identifiers. This
changes the test fixture only, retaining engine ID allocation and preserving
implicit defaults in actual missions.

ReportParameterDialog now supplies Add all configured and Remove all for ordered
multiple selections. Bulk additions deduplicate configured names and compatible
references; manual positional function arguments may repeat a reference. Single
selectors hide bulk operations. The new multi-object property table shows common
caller-filtered properties and shares dependency rules with the single browser:
coordinate/body restrictions, ODE models and directly attached hardware. Reference
choices are intersected across selected owners. Check all skips unavailable shared
references; manually selecting a conflicting property blocks OK with a recoverable
explanation. Selecting fewer owners recovers without altering the engine.

ParameterSelectionTests uses the real Report command editor and nested selector
widgets. It covers Cancel, two spacecraft/two properties, stable output ordering,
repeated bulk deduplication, pending Apply, original resource/mission source, labels
and comments, exact Undo/Redo/Unicode save/reopen and a byte-exact independently
configured report. It covers configured Add all/Remove all, writable/reportable/
plottable and single-value constraints, positional function duplicates, adjustable
columns, Check/Uncheck all, empty owner/property sets, separate attached tanks and
body-fixed restrictions. Broader caller/dependency combinations remain part of
shared qualification; this does not requalify every parameter's numerical regime.

parameter-selection-wayland-20260930.txt records a passing native Wayland run and
its exposed bulk-property PNG was inspected. The capture shows both owners, checked
properties, shared frame controls and usable acceptance/cancel controls. Qt Wayland
text-input surface warnings during synthetic focus changes are retained. This is
widget/native rendering evidence; fresh desktop input, portal choosers and GNOME
top-level minimize/restore remain outstanding.

check-parameter-selection-source-mapping-failed.txt retains the first complete
regression: 45 of 46 passed, with KalmanFilter blocked by Write's generated default
dictionary. Recognizing that dictionary fixed the source alignment; focused
KalmanFilter/ParameterSelection passed. check-parameter-selection-before-implicit-step.txt
records all 46 passing in 211.49 seconds before the final implicit-step/header
addition; the focused Mission suite passed that addition in 4.31 seconds.

Final regression: check-parameter-selection.txt records all 46 Qt suites passing
in 210.49 seconds after the final original-source, implicit-step and bulk-selection
changes. The build rebuilt the actual application/bin/GmatQt launcher target.
The separate final native Wayland parameter-selection run/capture also passed.
The acceptance checklist remains in progress: debugger stepping/live inspection,
remaining document/plugin/viewer operations, portal input and top-level GNOME
minimize/restore still need their required affirmative evidence.

## Main-mission debugger delivery and shared execution qualification

The active wx Inspector/Breakpoint workflow now has a Qt implementation. A
Mission-tree context-menu checkbox adds a transient visible breakpoint before
that command. Normal Run honors selected markers. Debug mission (Ctrl+F5)
stops before the first meaningful command when no markers are selected. Build
retains placements for identical source; accepted source changes clear stale
placements. Clear breakpoints removes all markers. These operations never insert
serialized Breakpoint commands or change mission source.

The inspector pauses before execution and displays read-only live sandbox
objects, with Spacecraft/All objects filtering, DEBUG_INSPECT configuration and
Parameter current values. Step (F10) executes through normal engine dispatch;
Resume (F5) continues, End (Shift+F5) stops, and Close/Escape resumes. Pause
during an active debug run waits for the next command boundary. Function calls
are a single step; function-local inspection/stepping is outside this main-mission
inspector. Help opens the existing offline Breakpoint topic without resuming.
Runtime inspection does not enable resource/source editing.

The shared engine has a nullable, synchronous command-execution observer at the
Sandbox and BranchCommand dispatch boundaries. The Qt run owns its installation
and restores the previous observer on exit. No command is executed directly by
the inspector, no command graph/configuration is modified, and no numerical
algorithm changes. An inactive observer adds no frontend behavior. Solver branch
maintenance ticks are skipped using the existing GetNext self-return contract,
while entry and child commands remain observable.

DebuggerTests uses actual mission-context controls and normal Run, stepping a
labeled report, nested For/If commands and propagation. Its stepped report is
byte-exact against ordinary execution, with live variable/array/spacecraft values
and unchanged source. It covers marker retention/invalidation, protected pending
edits, exact Undo/Redo/Unicode save/reopen, Close/Escape, End/Stop/rerun, Pause while
dispatching, recursive-run rejection, safe main-window close while paused and
build/initialization/execution failure recovery. A function call steps over local
commands and retains its independent output. Target and Optimize stop at branch
entry and child commands; repeated iteration reports match ordinary execution
byte-exactly, inspector values satisfy the independently specified cost/target
expressions, and final solutions match 3/2 respectively. Script-event stepping
also preserves the independent report and original boundaries/source.

The additional solver checks initially failed because IsExecuting reports only
child-branch activity, allowing the inspector to pause on Target maintenance.
check-debugger-solver-boundary-failed.txt retains that failed check. The GetNext
filter corrected this; focused offscreen/native X11 debugger and Help passed,
followed by the stronger iteration-value checks. Native X11 and offscreen exercise
synthetic F10/F5/Shift+F5 routing. The separate Wayland run uses widget/action
controls for those shortcuts; it does not claim fresh compositor keyboard input.

debugger-wayland-20260930.txt records the final native Wayland run. Its exposed
inspector capture was visually inspected: readable command/object values, usable
splitter/scroll area and all Help/Step/Resume/End/Close controls. Wayland synthetic
popup-grab and text-input warnings are retained. No GNOME security/input settings
were changed. Main-window top-level minimize/restore and portal chooser input
remain outstanding under their separate acceptance requirements.

The shared frontends were rebuilt. debugger-shared-frontends.txt and its per-app
logs/reports record separate actual GmatConsole, wx GMAT and GmatQt processes,
with isolated temporary startup/output/preferences, running the same For/If and
60-second propagation fixture. All three produce identical three-row reports
with the observer inactive. The original 48-suite regression before the final
solver/Help checks passed in 199.70 seconds and is retained as
check-debugger-before-solver-final.txt. The final complete regression passed all
48 suites in 210.60 seconds (check-debugger.txt), including the final solver,
Help, current-iteration and safe-close assertions. The actual
application/bin/GmatQt target is rebuilt.

This delivers the main-mission inspector operations, not full replacement
qualification. Remaining document/plugin/viewer operations, native desktop
input and the wider acceptance gates above still require affirmative evidence.


## External-force creation, removal and mixed Apply

The force-model editor now provides Python external force controls when the
selected runtime exposes ExternalModel. They enable the first contributor, choose
a configured module or a typed Python module name, edit the derivative function
and Exclude other forces, or disable the contributor. Module filenames/paths and
invalid function identifiers are rejected with a recoverable explanation. The
dialog inherits Help from its real resource panel. Accepted dialog settings stay
pending until the parent's Apply and combine with ordinary force-model fields in
one validated source transaction. Successful Apply refreshes the retained MDI
panel; Cancel preserves both engine and pending parent fields.

Creation writes only the external creator and its supported settings; removal
removes that contributor's configuration while preserving other source/comments.
Ordinary force-model scalar and owned-leaf edits no longer regenerate unrelated
owned forces or make their implicit defaults explicit. Actual creator changes
still retain the existing dependency-order reconstruction, including atmosphere
model/body changes. Wider selector/source and configuration-include cases remain
subject to the full source-preservation gate; this bounded improvement does not
qualify every creator transition.

ExternalForceTests opens the actual Resources/MDI force editor and verifies
Cancel, first enable, invalid module/function recovery, inherited Help, mixed
Error Control plus contributor Apply, retained-panel refresh, pending removal
and removal Cancel. Error Control now offers the engine's five supported methods
rather than a free-text value. The tests compare created and removed missions
with independently configured script references, preserve unrelated implicit
force defaults and Unicode/comments, and check one exact Undo/Redo transaction,
Unicode save/reopen and rerun. Existing missing-import/function runtime failures
and recovery, combined forces and internal two-body agreement remain covered.

The four affected suites (Atmosphere, ResourceRoundTrips, ExternalForces and
Polyhedron) passed after retaining atmosphere creator ordering. The first native
capture failed because its relative output path was resolved after the test
changed directories; external-force-create-capture-path-failed.txt retains that
failure. The rerun used an absolute output path and passed in
external-force-create-wayland-20260930.txt. The .png.create.png capture explicitly
asserts native window exposure and was visually inspected: module/function,
exclusion and Help/Cancel/OK controls are readable. The older base .png is a
standalone panel grab and supplies no additional desktop-input evidence. Native
portal and GNOME minimize/restore acceptance remain outstanding.

Final regression: check-external-force-creation.txt records all 48 Qt suites
passing in 208.32 seconds after the final external-force controls, mixed Apply,
retained-panel assertions and narrowed force-source changes. The actual
application/bin/GmatQt target was rebuilt. This closes the first external
contributor's creation/removal workflow; the full acceptance checklist remains
in progress, including remaining polyhedron/document/plugin/viewer operations
and native desktop-input gates.


## Polyhedron contributor creation/removal and body-qualified dispatch

The force-model editor now supplies Polyhedron gravity controls when the selected
runtime exposes the plugin. A table manages one contributor per celestial body,
with typed body selection, input-shape Browse and density in kg/m³. Initial columns
are readable and remain adjustable. Add/Remove support first, additional, partial
and complete contributor changes. OK leaves the table pending; the parent Apply
combines these settings with its ordinary fields in one validated rebuild and
refreshes the retained MDI panel. Cancel retains existing parent/source settings.
Help inherits the actual resource panel's offline topic.

Source patching is confined to the polyhedron family. It preserves unrelated
forces, comments, existing expressions/unknown family assignments for retained
bodies and implicit defaults. Legacy CreateForceBody/ShapeFileName/BodyDensity
aliases receive body ownership before another body is added. Legacy UserDefined
polyhedron creation is removed from that list before the PolyhedralBodies creator
is written, avoiding duplicates while retaining other user forces. Each new force
requires an explicit CreateForceBody binding as well as the body-list entry.
The first missing-binding failure is retained in
check-polyhedron-create-body-binding-failed.txt; the final writer supplies it.

The independent Earth/Mars reference also exposed shared interpretation behavior:
qualified CreateForceBody, ShapeFileName and BodyDensity went through legacy
ODEModel aliases, updating the first contributor. The observed configured objects
were one Mars force with the last shape/density and another force with empty
binding/shape. Interpreter now routes body-qualified polyhedron assignments to the
owned force whose type and body match the qualifier, and rejects a qualifier with
no contributor. Unqualified legacy syntax retains its existing behavior. This is
a configuration-dispatch fix; no force evaluation or propagation algorithms change.

PolyhedronTests exercises the actual Resources/MDI editor, first creation Cancel,
nonfinite density and duplicate-body rejection/recovery, widget Browse Cancel,
Help, pending creation and Apply, retained-panel refresh, malformed-mesh rollback
and pending correction back to the unchanged configuration. It compares first and
Earth/Mars contributor reports against independent scripts, checks exact source
Undo/Redo and Unicode save/reopen, then partial and complete removal with separate
reference states. Removal selects the named body, independently of engine-owned
force ordering. Legacy UserDefined conversion retains an omitted density assignment
at its default, with report equivalence and round trips. Wrong-body qualified
source failure/recovery and multi-row removal are also covered. The existing cube
far-field mass check, SurfaceHeight browser, malformed geometry and relative-asset
checks remain applicable. This qualifies representative GUI configurations, not
new scientific propagation regimes or every possible mesh.

polyhedron-create-wayland-20260930.txt records a passing native Wayland run. Its
.png.create.png explicitly asserts exposure and was visually inspected: body,
shape and density columns, Add/Remove/Browse and Help/Cancel/OK are readable. The
base .png is an older standalone panel grab and adds no desktop-input evidence.
Widget choosers are used; native portal and GNOME minimize/restore gates remain
outstanding. The focused final Polyhedron suite passed in 4.02 seconds before
native capture. Full regression and shared frontend results are recorded below.


PolyhedronFrontendTests.py supplies a repeatable Linux check of the shared
interpretation fix after rebuilding GmatQt/GmatGUI/GmatConsole. Separate actual
console, wx and Qt processes run the same body-qualified Earth/Mars fixture with
isolated startup, output and preferences. polyhedron-shared-frontends.txt and
its per-application logs/reports record byte-identical six-column state reports.
The initial shared fixture used a Unicode shape filename; the legacy console
file reader rejects non-ASCII script contents before interpretation. That failure
is retained in polyhedron-shared-ascii-fixture-failed.txt and its console log.
The comparison rerun uses an ASCII path. Qt's Unicode configuration and
save/reopen coverage remains in PolyhedronTests; the shared comparison does not
claim Unicode support for the legacy console/wx readers.

Final regression: check-polyhedron-creation.txt records all 48 Qt suites passing
in 195.81 seconds after the final controls, body-qualified dispatch, recovery and
legacy/multiple/removal assertions. GmatQt, GmatGUI and GmatConsole were rebuilt;
the actual application/bin/GmatQt launcher points to the rebuilt executable.
This closes representative first/multiple polyhedron contributor creation and
removal. The full acceptance checklist remains in progress, including broader
document/plugin/viewer operations and native desktop-input gates.


## Independent active/inactive script documents

The Qt workspace now supports additional script windows while keeping Resources,
Mission and Output tied to an explicitly active mission. File actions open/new
inactive scripts; ordinary Save/Save As and search/line navigation use the selected
script. Duplicate opens focus the same buffer; duplicate filenames display full
paths. Mission / Make selected script active builds its current buffer; selected
Save/build and Save/build/run save before promotion. Separate buffers retain their
source, Undo/Redo, filenames, saved snapshots and modified states. Relative
includes/assets follow the promoted script's folder.

Pending active-mission panels offer Apply/Discard/Cancel before promotion. Failed
Apply preserves pending settings and aborts switching; failed target builds retain
the target buffer and restore the previous built mission. Successful promotion
closes obsolete configuration panels and clears old mission viewer histories and
breakpoints. It does not carry a previous mission's output forward as current.
Active-editor close retains its loaded mission and hidden buffer; View/Active
script or duplicate open restores it. Inactive close and application close check
unsaved scripts. Reload asks before replacing source, checks encoding/reads first,
and invalidates the old model when applied to the active script. Runtime protects
all editors and document switching/close operations.

The first regression exposed an MDI activation signal during QWidget teardown,
after the C++ document list had been destroyed. check-documents-teardown-failed.txt
and documents-teardown-backtrace.txt retain the failure and trace. MainWindow now
disconnects document/workspace/application callbacks and clears close callbacks
before destroying its model/document state; the existing Files/ScriptEditing
checks passed after that correction.

Configuration includes exposed two serializer effects: repeating a directive in
the mission tail prevented safe command alignment, and attaching one before a
resource declaration prevented assignment comparison. The initial failures are
retained in check-documents-include-mapping-failed.txt and
check-documents-resource-include-failed.txt. Canonical comparison now ignores
nonexecuting serialized include directives while retaining the original source's
directives. Cross-file mission commands remain protected when their expanded
statements cannot be aligned to the actual editor source. No canonical fallback
or rewrite of included files is introduced.

ScriptDocumentTests uses the actual File/Mission actions and Resources MDI panel.
Separate Unicode directories with equal filenames and relative configuration
includes produce independently specified 7/10/15/16 reports after editing,
promotion and GUI command changes. It checks duplicate identity and dirty siblings,
per-document Save/Save As/Find/Replace/line navigation and exact Undo/Redo,
Save As Cancel, independently open-file collision and missing-directory write
failure, Apply/Discard/Cancel and rejected pending Apply, invalid target builds
and correction, active/inactive Reload and malformed UTF-8 protection, selected
Save/build/run, Stop with every editor protected, hidden active reopen and dirty
inactive/main-window close choices. An actual mission include also produces the
independent 18 result while its cross-file commands reject GUI source patching.
The shortcut cases inject Qt events for Save/Undo/Redo on offscreen/native X11;
the separate Wayland capture uses actions and adds no fresh keyboard or portal
input qualification. Scientific engine algorithms are unchanged.


The final native Wayland run passed in documents-wayland-20260930.txt. The
.png capture asserts window exposure and was visually inspected: the familiar
navigation/MDI/messages layout, both script editors, line numbers, syntax colors,
Unicode text and active/inactive/dirty titles are readable. Equal-filename full
path titles are asserted earlier in the same test before Save As changes one
filename. Widget choosers were used; fresh compositor keyboard input, native
portal chooser operation and top-level GNOME minimize/restore remain outside
this evidence and retain their separate outstanding gates.

The initial complete regression passed 50 suites in 217.93 seconds
(check-documents-initial.txt). After the final write-failure, pending-rejection,
active Reload, cross-file command and shortcut assertions and full-path title
change, check-documents.txt records all 50 suites passing in 200.65 seconds.
This includes NativeDocuments on X11 plus the existing native orbit/window,
solver-display, folder-run, DPI and fallback suites. The actual GmatQt target
was rebuilt; the final launcher verification is retained in
build-documents-application.txt. Full Linux replacement qualification remains
in progress under the acceptance gates above, including wider source/plugin
operations and the outstanding native desktop-input gates.


## Initial spacecraft covariance editing

The existing numeric matrix editor now identifies the six spacecraft covariance
components and their units. Labels follow pending SolveFors: Cartesian X/Y/Z and
VX/VY/VZ, or Keplerian SMA/ECC/INC/RAAN/AOP/MA. DisplayStateType does not change the
covariance basis. The dialog states the documented EarthMJ2000Eq estimation input,
MJ2000Eq propagation axes, warm-start covariance precedence and batch
UseInitialCovariance condition. Changing solve-fors does not convert the numeric
values. Help resolves to the offline SpacecraftNavigation topic.

Copy upper triangle to lower triangle is explicit; opening or accepting the grid
never silently symmetrizes or changes its covariance basis. Both dialog OK and
the resource Apply setter reject nonfinite, asymmetric, singular or indefinite
matrices before modifying source/configuration. Positive definiteness uses the
existing CholeskyFactorization called by EKF, with finite-result checking; no
estimation, propagation or matrix factorization algorithm changes. These checks
apply to GUI edits, without rewriting how arbitrary script inputs are interpreted.

CovarianceTests runs the shipped GPS filter fixture shortened to ten minutes with
noise disabled, plus independently specified correlated position/velocity input.
The shipped runtime diag expression must become a literal configuration matrix
for this GUI-owned cold-start input; the first invalid fixture is retained in
check-covariance-initial-fixture-failed.txt. It opens the actual Resources/MDI
spacecraft editor, checks fixed dimensions, Cartesian/Keplerian-MA labels,
Cancel, explicit symmetry, invalid correction, pending acceptance and retained
Apply, exact Undo/Redo, comments and Unicode save/reopen. GUI execution and
reopened execution produce byte-identical state and covariance CSV output to the
independently configured script. Direct Apply cannot bypass symmetry checks;
singular input is also rejected. Keplerian labeling is qualified here without
claiming a new Keplerian estimation regime or batch covariance execution.

check-covariance.txt records that one focused suite passing in 1.08 seconds.
The subsequent native Wayland preview deliberately omits numerical execution and
uses only the new editor. Its first capture revealed long floating-point text
clipping the last column (covariance-wayland-before-width.png/.txt). The display
now abbreviates cell text through a delegate while EditRole/tooltips retain all
stored digits, and starts with six balanced, adjustable columns. The final
covariance-wayland-20260930.txt/.png records an exposed preview with viewport-fit
and full stored precision assertions; the capture was visually inspected.
Widget actions are used, without claiming fresh compositor input or portal
chooser qualification.

The functional focused check predates only those display/column corrections;
the final native preview checks the corrected layout/data presentation. The
previous complete 50-suite document regression is reused as baseline and was not
repeated for this change. The new focused test is included in check-qt for a later
broader run. build-covariance.txt and build-covariance-display.txt record the target
builds, which also rebuild the actual application/bin/GmatQt dependency. The full
Linux acceptance checklist remains in progress with its remaining plugin/source
operations and native desktop-input gates intact.


## Drag creator transitions retain unrelated force source

Changing the drag creator or its atmosphere body previously selected the generic
owned-force fallback, reconstructing the full edited force model from canonical
output. That could replace unrelated source syntax and make unprinted defaults
explicit. Drag-only creator transitions now replace the creator/body RHS at its
original position, retaining qualified or legacy creator spelling and comments.
Missing creators are inserted ahead of their dependent fields. Disabling removes
only drag-family assignments, including legacy aliases, and writes Drag = None.
Other scalar changes in the same Apply, such as ErrorControl, remain atomic.
Changed legacy leaf aliases are removed beside their canonical replacement.
The explicit or engine-located implicit mission boundary protects runtime code;
unlocatable boundaries fail before changing the source. Other root-force selector
transitions still use the existing ordered reconstruction fallback and retain
that source-fidelity limitation for further work.

ForceSourceTests covers mapped continuations/comments, unqualified aliases,
body-only and missing creator/body assignments, both mission boundaries and
refusal when the implicit boundary cannot be located. Its execution cases use
actual Resources/MDI atmosphere controls for first NRLMSISE00 creation, switching
to Exponential through a shipped-style creator, disabling and re-enabling drag.
Cancel and invalid-flux correction, pending acceptance, retained mixed Apply,
unchanged primary/gravity/point-mass source, unprinted SRP/relativistic/polyhedron
defaults, exact Undo/Redo and Unicode save/reopen are checked. Six reported states
agree with independently configured 600-second scripts for the three resulting
configurations. Body-only and continued-alias checks here concern source mapping;
no new non-Earth numerical regime is claimed. Existing atmosphere weather/SPAD
and native dialog evidence is reused without rerunning it.

The first execution fixture omitted the explicit Earth primary selection needed
by the existing engine to support drag. check-force-source-initial.txt and
check-force-source-fixture.txt retain those failures; no engine change was made.
After fixing the fixture, check-force-source-transition.txt records the focused
suite passing in 0.84 seconds. build-force-source.txt records its build and the
actual application dependency rebuild. No full-suite or native-layout rerun was
needed for this source-only change. The prior complete document regression and
focused covariance evidence remain the baseline; the new suite is available in
check-qt for later broader verification. Full replacement qualification and its
outstanding source/plugin and native desktop-input gates remain in progress.


## SRP and relativistic On/Off controls change the physical model

The generic ODEModel SetOnOffParameter implementation returns success for SRP
and RelativisticCorrection without changing its owned forces. The script
interpreter creates those forces separately. The Qt resource setter now does
that creation/removal on the pending clone through the existing physical-model
factory, using the force model's central body and ownership convention. Existing
On selections retain their owned settings; Off removes the matching force. No
force calculation or interpreter algorithm is changed. Apply rebuilds the source
and refreshes the retained panel, exposing new SRP settings when enabled.

Source patching now treats these two selectors independently of the whole-force
fallback. Existing selector RHS values change in place, preserving spelling,
continuations and comments. A missing creator precedes its owned dependent
settings; disabling removes only that family and legacy leaf aliases. Remaining
changed fields use the usual source delta. The operation composes with drag
creator transitions in one Apply. Primary/point-mass and other root-selector
mixed transactions retain the existing fallback and its source-fidelity limit;
this checkpoint does not claim that remaining gap closed.

ForceSelectorTests compares six-state reports against independent 1800-second
Earth scripts for SRP alone, SRP plus relativistic correction and changed flux,
relativistic correction alone, and both disabled. The references produce distinct
results, so a silently ignored selector cannot pass. Actual Resources/MDI
controls exercise pending On/Off, mixed ErrorControl and flux changes, rejected
flux correction with old configuration/pending panel retained, removal,
re-enabling and refreshed owned fields. Original primary/gravity/point-mass text,
comments and mission code stay exact; unrelated drag/polyhedron/user-force
defaults remain unprinted. Exact Undo/Redo and Unicode save/reopen precede the
state comparisons. Direct invalid On/Off is rejected. A further transaction
removes drag while enabling both other families and setting flux/ErrorControl;
its exact Undo/Redo and reopened states match the independent combined reference.
Parser cases cover continued selectors, legacy leaf removal and an implicit
mission boundary. These are bounded GUI/engine wiring checks, not new numerical
algorithm qualification or full SRP shape/shadow regime coverage.

The initial check crashed in the new test helper because dropdown-backed cells
do not contain a text item in their value column. force-selectors-backtrace.txt
identifies the helper; it was corrected without a product-lifetime change.
The sandbox-denied first tracing attempt remains in
force-selectors-initial-backtrace.txt. The build log also retains correction of
an incomplete test include. check-force-selectors.txt records the initial
successful focused run (0.83 seconds); check-force-selectors-mixed.txt records
the final focused run with the combined-family assertion (0.88 seconds).
build-force-selectors.txt and build-force-selectors-final.txt record the actual
application dependency rebuilds. Existing drag/weather/native panel and complete
regression evidence is reused; no full-suite or native-layout rerun was needed.
Full qualification remains active under all acceptance gates above.


## Mixed gravity body-list transactions retain source and dependent settings

PrimaryBodies and PointMasses changes now update the pending clone's owned
physical models before dependent settings, rather than only replacing a canonical
list string on a clone whose force inventory still held the old bodies. Removed
contributors disappear before replacements are added, retained bodies keep their
settings, and new gravity contributors use the same Earth/Luna/Venus/Mars named
file defaults as the existing interpreter. Unknown bodies and primary/point-mass
overlap reject the edit. Body lists use the same path for isolated and mixed
changes. Post-build checks confirm the selected body sets and restore the prior
mission on mismatch, including transactions with the specialized external or
polyhedron controls. No force calculation or scientific algorithm is changed.

The original selector RHS changes in place, including the legacy Gravity alias.
Missing primary selectors precede dependent gravity/drag source. Changed owned
assignments are patched by delta; removed primary settings disappear while their
comments survive. Unqualified gravity leaves have an unambiguous owner when the
old model has one primary, and are removed with that primary. Unrelated SRP,
relativistic, drag and other source remain unchanged, with unprinted defaults
retained. Direct root PolyhedralBodies/UserDefined/External changes still retain
the older whole-model fallback; their remaining active caller/setting scope must
be reconciled separately. The dedicated polyhedron/external controls retain their
existing narrow source writers. Multi-primary legacy alias ambiguity and wider
body/force combinations are not established by this checkpoint.

The atmosphere dialog initially rejected drag for a newly pending primary
because it only validated against the old configured gravity list. The force
panel now supplies a private preview with the pending body selections. The
failed checks are retained in check-gravity-bodies-initial.txt and
check-gravity-bodies-dialog.txt; the latter records the engine's explicit missing
primary explanation. The preview fix passed in check-gravity-bodies-preview.txt.
Cancel/acceptance still acts on the private preview, with changes pending until
the parent force-model Apply.

GravityBodyTests uses actual Resources/MDI panels to change a Luna point mass to
Mars with ErrorControl, exchange an Earth primary for an Earth point mass,
reject removal while its drag is still active and correct the pending dialog to
None, recreate the initially implicit Earth primary and drag together, and remove
the last point mass through a list-only Apply. It checks unchanged SRP/flux and
relativistic source, owned/legacy gravity removal, unknown/overlapping body
rejection, retained refreshed panels, one exact Undo/Redo, comments, mission
suffix and Unicode save/reopen. Independent 600-second Earth scripts provide the
six-state reports, including degree/order 4 defaults on the recreated primary.
Source cases also cover legacy Gravity, continued point lists, removed leaves
and an implicit mission boundary. Mars is a point-mass contributor here; no new
Mars/Luna/Venus primary numerical regime is claimed.

check-gravity-bodies.txt records the expanded focused check passing in 0.92
seconds. check-gravity-bodies-list-only.txt records the explicit isolated-list
case; the final check-gravity-bodies-legacy.txt passes in 1.01 seconds with the
real legacy Degree source case. build-gravity-bodies.txt and
build-gravity-bodies-final.txt record the actual application dependency rebuilds.
The established window/viewer, body chooser and broader regression evidence is
reused. No full-suite or native-layout rerun was needed for these transaction and
source changes. Full replacement qualification remains in progress under the
original acceptance gates and outstanding plugin/source/native-input limits.


## Polyhedron body moves and external module edits preserve force source

The remaining active owned fields that changed a root force creator could still
trigger whole-model regeneration: PolyhedronGravityModel.CreateForceBody and
External.ScriptFileName. Body moves now use the existing narrow polyhedron
writer, retaining settings under the new owner and leaving unchanged density
implicit. Known mesh/density edits for all contributors are included in the same
pending transaction. Invalid bodies or duplicate destinations reject before
source changes; the existing rebuilt-model settings check and rollback remain
in force. Unknown owned source assignments are mapped to the new body rather
than dropped. Comments, unrelated force text and the mission suffix survive.
The original body is also carried through the contributor dialog, including
reopening pending settings and edits first made in the per-field controls. An
explicit empty origin distinguishes a new pending row from a configured force.
Restoring an unchanged original row produces the original settings again.

Changing an existing external module now replaces the creator's RHS in place
before dependent function/scalar deltas are applied. Root and module-field
creator aliases are recognized. This also serves the existing contributor
dialog's nonstructural module edit; adding/removing the force retains the
specialized external writer. Unrelated force defaults are not printed. The
generic raw PolyhedralBodies/UserDefined/External structural fallback still
exists: ResourceProperties exposes neither of the first two compound arrays,
and ODEModel marks root External read-only. The active specialized creation,
removal and owned-body/module paths are handled separately. This bounds the
earlier fallback warning; it does not qualify arbitrary raw root transactions
or every possible multi-force combination. No numerical algorithm is changed.

Two dedicated modes in the existing executables avoid replaying their older
plugin suites. QtGui.PolyhedronBodySource exercises the actual Resources/MDI
per-field body/path edit with mixed ErrorControl, rejection/correction of a
non-celestial body with the old panel and pending values retained, accepted panel
refresh, legacy creator conversion, unchanged explicit point mass/central-body
source, implicit density and unrelated defaults. It restores the body and then
moves it through the contributor dialog, reopens pending settings and applies.
Both paths check exact Undo/Redo and Unicode save/reopen against independent
Earth/Mars six-state script reports. Pure writer checks cover qualified and
unknown owned settings; an isolated dialog check confirms a reopened new row
retains its empty origin. It reuses the cube fixture and the already established
loader/far-field evidence, rather than repeating mesh or numerical qualification.

QtGui.ExternalModuleSource uses the actual MDI module picker, pending function
and mixed ErrorControl, retained refreshed panel, legacy function alias, creator
spelling/comments and unchanged point mass/mission. Unrelated implicit force
defaults stay absent. Exact Undo/Redo and Unicode save/reopen precede agreement
with an independently configured shortened no-API example's twelve-state report.
Unknown owned assignments and commented continuations are pure source-mapping
checks; commented continuation execution is not claimed. Existing missing-module,
missing-function and creation/removal recovery evidence is reused.

The first polyhedron build missed a QJsonObject include. A separately launched
check then ran the stale executable's older suite, recorded in
check-polyhedron-body-source.txt; it is not evidence for the new mode. Its
generated capture was removed. Subsequent checks are conditional on successful
builds. check-polyhedron-body-source-final.txt records the new fixture failing;
check-polyhedron-body-source-debug.txt includes GMAT's explanation that the
legacy creator conflicts with default Earth gravity. The fixture now explicitly
retains a Sun point mass, matching the established legacy setup.
check-polyhedron-body-source-fixture.txt passed the per-field case in 1.32 seconds.
The dialog build also records a test-only findChild type correction in
build-polyhedron-body-dialog.txt. The final expanded check,
check-polyhedron-body-dialog.txt, passed in 0.66 seconds.

check-external-module-source.txt and its debug companion record the new fixture
failing because GMAT rejects its commented continuation. Execution now uses
valid single-line source; check-external-module-source-fixture.txt passed in 0.35
seconds. Build logs preserve the corrections and actual application rebuilds;
build-polyhedron-body-dialog.txt records the final GmatQt-R2026a relink and
GmatQt launcher recreation. Existing complete regression and native
window/viewer evidence is reused, with no full-suite or native-layout rerun.
Full Linux replacement qualification remains active under the original gates,
including native Wayland main-window restore/input, desktop portal and remaining
plugin/workflow limits. Windows/macOS remain deferred.


## Propagation segment color controls and Ground Track display metadata

The active wx PropagatePanel constructs a GmatColorPanel and writes
SetOverrideSegmentColor/SetSegmentOrbitColor on Apply. Qt retained an existing
OrbitColor option but lacked that editing operation. CommandEditor now supplies
Segment color controls for a representable propagation statement: an override
checkbox, GMAT named/RGB input and color picker. Turning it off removes only the
color option; other stops/options, labels, flags, comments and command text are
retained. One-step commands can acquire a color-only option block or return to
their original step form. RGB validation uses the engine's color conversion.
Cancel leaves pending source alone; OK changes it in one editor Undo; the normal
command Apply still validates/rebuilds the complete mission and retains its MDI
window. Duplicate color options and unrelated/incomplete statements do not offer
a lossy editor. Source mapping retains comment line breaks when replacing an RGB
literal, including comments inside the original literal.

The focused check exposed that the current GroundTrack subscriber inherited a
no-op SetSegmentOrbitColor callback, while Qt's SolverData action read only the
spacecraft default. Orbit View already received the publisher's segment palette.
GroundTrack now forwards the publisher's object list and override as optional
display metadata through the existing action channel. Qt records overrides per
object, clears only the named objects on Off, and selects the target palette for
solver trial samples and the segment/default palette for accepted samples.
Reinitialize clears the override map. The legacy wx receiver passes actions to
its widget; the additional optional action can be ignored. No state publication,
propagation force calculation, geodetic conversion or numerical algorithm changes.

PropagationColorTests exercises actual Mission/MDI controls for adding an RGB
override, replacing an existing named color and disabling another segment.
It covers parent/picker Cancel, invalid RGB correction through the picker,
pending exact Undo/Redo, each applied mission's exact Undo/Redo, retained panel
refresh, original force/report source and stop tolerance/comments, and Unicode
save/reopen. An independently configured three-segment mission supplies the
six-state report and Orbit/Ground per-sample paths/colors; reopened GUI edits
produce the same report bytes and paths within 1e-8 display-coordinate tolerance.
Both histories contain the chosen blue/orange/default red segments. Display-only
action checks cover a subset reset, solver target/accepted palette transitions,
invalid metadata and unchanged history on default restoration. Those are callback
checks, not a new solver numerical run or a full multi-spacecraft propagation
qualification. Pure source cases cover first/middle/last/sole color removal,
one-step insertion, labels, BackProp/Synchronized/variational flags, other options
and RGB comments. Existing broader solver, replay and window evidence is reused.

check-propagation-colors.txt records the initial mapping check's missing space
before a retained comment; the mapper now retains that separation and line break.
check-propagation-colors-mapped.txt records the first viewer-color assertion
failure. check-propagation-colors-palettes.txt identifies Ground as all red despite
colored Orbit segments. The display callback fix passed in
check-propagation-colors-ground.txt (0.64 seconds). A later display-only assertion
used the wrong hard-coded RGB integer, retained in check-propagation-colors-final.txt;
it now derives the integer from QColor. The final focused check is
check-propagation-colors-metadata.txt (0.62 seconds).

The native Wayland preview runs no numerical workflow. Its initial exposed image,
propagation-color-wayland-20260930-initial.png, showed clipped explanatory text
at a small height. Minimum layout sizing and a minimum vertical label policy
correct it; the final propagation-color-wayland-20260930.txt/.png records and
shows the complete explanation, checkbox, RGB input, picker and action buttons.
The preview explicitly asserts the label meets heightForWidth before saving.
This is widget exposure/layout evidence, not fresh compositor input or portal
qualification. build-propagation-colors-layout.txt records the final actual
GmatQt-R2026a relink and GmatQt launcher recreation. No full-suite or repeated
numerical run followed this layout-only adjustment. Original acceptance gates,
native Wayland main-window restore/input, portals and remaining plugin/workflow
requirements remain open; Windows/macOS remain deferred.


## Ground Track runtime spacecraft palettes and assignment controls

The segment-color control checkpoint did not establish mission-time default
color changes. OrbitPlot maintains runtime orbit/target palettes when Assignment
notifies the publisher. The current GroundTrack subscriber's inherited color
callbacks did nothing, and Qt's SolverData action fell back to the configuration
copy of each spacecraft. A mission that assigned Green or an RGB color therefore
showed those colors in Orbit View but restored the original red in Ground Track.
check-runtime-colors-initial.txt confirms the discrepancy: Ground retained blue
segment overrides and the peer's yellow but had zero green/RGB default samples.

GroundTrack now includes the live sandbox spacecraft's orbit/target palettes as
optional display metadata immediately before its existing solver-state/sample
callbacks. Qt validates the entire palette packet before replacing the stored
maps. Accepted samples select the segment override or live default; trial samples
select the live target palette. Reinitialize clears all color maps. No color
metadata is inferred from script text or from stale configured objects, and
existing history keeps its captured per-sample colors. The sampled palettes also
support views that start receiving data after a runtime color change, although a
specific late-Toggle execution is not qualified here. This changes display
metadata only: state publication, coordinate conversion, forces and numerical
algorithms stay unchanged. Receivers may ignore the optional action, using the
same existing Ground Track callback channel as segment and solver metadata.

RuntimeColorTests is a separate focused executable; it does not repeat the older
color-dialog or plugin suites. Its two-spacecraft mission contains an initial
default arc, runtime named orbit/target assignments, a blue segment override for
both spacecraft, a second RGB default assignment and an arc with the override
off. Both plots must contain the resulting named/RGB/blue/default histories,
including the peer's unchanged yellow default. Actual Mission/MDI assignment
expression controls change the named/RGB and target values. A rejected unclosed
string preserves the original mission and pending panel; correction applies in
the retained MDI window. Each successful change has exact source Undo/Redo;
the force/spacecraft/report configuration remains exact, followed by Unicode
save/reopen and execution.

The independent uncolored mission and the independently colored reference
produce identical twelve-state report bytes. Reopened GUI edits reproduce those
reports and both spacecraft's Orbit/Ground histories, with a 1e-8 bound on display
coordinate differences. Direct display-state transitions after the actual target
assignment confirm Cyan/Magenta trial colors and restored live default colors.
That last check exercises palette selection without another numerical solve;
the existing solver execution/replay evidence is reused. A rerun is specifically
included to verify previous runtime defaults do not leak into the initial
red/yellow arc. Other assignment destinations, broader runtime/Toggled-view and
solver combinations remain under their original qualification limits.

check-runtime-colors-palettes.txt records the corrected focused check passing in
1.23 seconds. build-runtime-colors-palettes.txt records the shared subscriber,
Qt receiver and actual GmatQt-R2026a rebuild and GmatQt launcher recreation.
No widget layout or rendering algorithm changed, so the existing native dialog,
viewer/rendering and complete regression evidence is reused. No full-suite or
native-layout rerun was needed. Full Linux replacement qualification remains
active, including the original outstanding Wayland restore/input, portal and
plugin/workflow gates. Windows/macOS remain deferred.


## Covariance propagation flags and retained command source

The active wx PropagatePanel has covariance/STM/A-matrix controls, while Qt's
assignment dialog previously rejected any command containing Covariance. It now
loads quoted or bare Covariance flags, offers the same command-wide option and
shows the STM that the engine includes automatically. Disabling covariance
restores the explicitly selected STM state; it does not leave a newly implicit
STM behind or discard an original explicit STM. A-matrix remains independent.
Unchanged settings return the original source exactly, including flags on later
propagator groups, whitespace, labels, stop options and comments. Unknown quoted
flags remain a source-editor workflow rather than entering lossy controls.
The tooltip and GUI guide state the existing Cartesian/MJ2000Eq/fixed-step
requirements. This does not change engine validation or numerical algorithms.

The first transaction test exposed another source-mapping defect: Apply accepted
the change, but the engine generated bare flags where the original source used
quotes. Every mission entry then lost safe source alignment, and the retained
command panel could not refresh. Propagate also repeats command-wide flags in
earlier groups and includes STM with Covariance. MissionModel now compares known
variational flags by their command-wide meaning while retaining the ordered
propagator/object groups, label and stopping/options text. This comparison is
restricted to recognized Propagate syntax. It retains original source instead
of regenerating it; different flags, propagators, labels and stop values still
fail alignment. Build-only multi-group checks cover the engine's added earlier
flags and a label containing (STM), plus those unsafe mismatches.

PropagationCovarianceTests is a separate focused executable. It uses the shipped
Ex_Propagate_Covariance Moon/SNC example shortened to 600 seconds, preserving the
Moon gravity/point masses/SRP, fixed steps, VNB process noise and reporting. Three
independently scripted runs supply six-state and covariance reports for disabled,
Covariance-only and combined STM/A-matrix/Covariance commands. The covariance
report changes when enabled. Actual Mission/MDI controls then reproduce each
reference's report bytes after Cancel, pending and applied exact Undo/Redo,
retained clean-panel refresh, Unicode save/reopen and run. The stop tolerance and
segment-color option, command label, comments and unrelated configuration remain
intact. Existing initial covariance-grid and broader solver/viewer evidence is
reused; this is GUI equivalence for the selected fixture, not a new scientific
qualification or full multi-group covariance execution claim.

check-propagation-covariance.txt and check-propagation-covariance-debug.txt retain
the original refresh failure. check-propagation-covariance-debug2.txt identifies
the successful Apply with an unmapped source entry. The mapping correction passed
in check-propagation-covariance-mapping.txt (1.04 seconds). The final focused check,
including the extra build-only mapping cases, passed in
check-propagation-covariance-final.txt (2.42 seconds). Raw failure logs also retain
the engine's time-stop warning at tolerance 1e-8; its achieved difference is about
7e-8 seconds. The independent reference and GUI runs agree; no tolerance or engine
behavior was changed to suppress that warning.

propagation-covariance-wayland-20260930.txt/.png records a preview-only native
Wayland exposure and capture. The inspected image shows complete explanatory text,
all three variational checkboxes, the automatic disabled STM, adjustable group
table and action buttons. It repeats no numerical workflow and makes no fresh
compositor-input or portal claim. build-propagation-covariance-mapping.txt records
the final product-code rebuild, GmatQt-R2026a relink and GmatQt launcher recreation;
build-propagation-covariance-final.txt rebuilds only the later test additions.
No full-suite rerun was needed; check-documents.txt remains the latest complete
regression checkpoint. The full Linux qualification goal remains active, including
Wayland main-window restore/input, portal and remaining plugin/workflow gates.
Windows/macOS remain deferred.


## Toggle plot discontinuities and selected color-control audit

The Toggle checklist already supported all subscribers, but report-only execution
evidence did not establish plot behavior. Orbit View processes data-state changes
at propagation-block flushes; GroundTrack returned before forwarding them, and XY
never notified its receiver. Disabled subscribers stop receiving real samples, so
Qt could not infer the missing interval. The initial focused fixture confirms
Ground and XY connected the resumed trajectory to the old endpoint across an
interval with no samples. Orbit View already separated its arcs correctly.

GroundTrack now forwards the existing ToggleOn/ToggleOff command actions through
its optional display channel. Qt changes its display activity and marks each
curve's next sample as a new arc. This preserves manual pen settings and all
previous samples. XY forwards those same command notifications using the existing
PlotInterface ActivateXyPlot/DeactivateXyPlot APIs; the Qt implementation already
marks breaks when deactivated. The wx receiver has implementations of both XY
APIs. The wx Ground receiver may ignore the optional actions, as it does other
Qt display metadata. This is no claim of a new wx Ground behavior qualification.
No publication ordering, sample collection, state conversion, propagation force,
solver or numerical algorithm changes. The native renderer and fallback both
honor each point's existing connect flag; rendering code did not change.

TogglePlotTests is a new separate focused executable. A two-spacecraft mission
propagates three thirty-second arcs, suppressing Orbit/Ground/XY during the middle
arc and changing one spacecraft's color before reactivation. Independently scripted
uninterrupted and toggled variants supply identical twelve-state report bytes.
The test checks the displayed spacecraft has no disabled-interval samples and
exactly one disconnected resume in all three plots. Ground and Orbit use the live
Green palette after the hidden assignment. Orbit retains camera frames within the
trajectory history. Actual Mission/MDI subscriber checklists then replace two
ReportFile-only Toggle selections with the three plots, preserving pending source,
retained clean command panels, exact Undo/Redo, labels/comments and unrelated
configuration. Unicode save/reopen execution reproduces the independent report
and inspected spacecraft's trajectory, color and connect history. Immediate plot
close/reopen retains the same model and samples. Existing checklist Cancel/filter
and native window/replay/rendering evidence is reused; no native numerical rerun
or full-suite repeat was needed. Solver-loop/ephemeris Toggle remains open.

build-toggle-plots-initial.txt retains a test-only missing QDialog include, fixed
before the first check. check-toggle-plots-initial.txt records the two actual
joined-arc failures. It also contains an overly broad harness assertion that XY
must set the 3D endOfRun flag: XY's existing display contract always retains all
points and ignores the 3D recent-segment finalization flag. That assertion is now
limited to Orbit/Ground; no unrelated XY finalization change was made. The fixed
focused test passes in check-toggle-plots-notifications.txt (0.79 seconds).
build-toggle-plots-notifications.txt records the shared subscriber library/native
plugins and actual GmatQt-R2026a rebuild and GmatQt launcher recreation.

The source review also resolves the earlier per-view orbit/target color pending
item. The wx OrbitViewPanel and GroundTrackPlotPanel creation, load and save code
for these colors is guarded by __USE_COLOR_FROM_SUBSCRIBER__. The selected build
and source headers do not define it. OrbitPlot::GetParameterID explicitly returns
PARAMETER_REMOVED for OrbitColor and TargetColor. These are disabled legacy
subscriber controls, not missing active selected-runtime operations. The active
SpacePoint color pickers, runtime Assignment colors and Propagate segment controls
have separate implementation/execution evidence in the preceding appendices.
audit-view-color-controls.txt records the relevant source and current wx compile
defines. This disposition does not qualify other resource color families.

The full Linux qualification goal remains active, with the original outstanding
Wayland main-window restore/input, portals and remaining workflow/plugin gates.
Windows/macOS remain deferred. check-documents.txt remains the latest complete
regression checkpoint; this targeted display fix adds only its focused evidence.


## Completed retained plots after a final ToggleOff

The resumed-arc checkpoint did not cover a mission ending with a plot disabled.
OrbitView returns before its end-of-run notification when inactive. Qt's retained
history consequently kept endOfRun false: with NumPointsToRedraw = 1, Latest
showed only the most recent segment after a successful mission. This is visible
output behavior, not a loss of propagation data or a numerical discrepancy.
check-disabled-completion-initial.txt records that Orbit View failure. Ground
already had its completion marker in this fixture, and XY always displays its
retained history independently of the 3D recent-segment setting.

The Qt mission coordinator now finalizes retained Orbit/Ground display histories
when RunMission reports Completed. It refreshes existing viewers and updates
closed viewers' retained models, without changing plot activity, points, colors,
cameras, script or sampling. Latest shows the whole retained trajectory; earlier
replay positions keep the existing recent-segment behavior. Failure and Stop do
not enter this new completion path. The folder coordinator uses the same display
method for Completed items; its final-ToggleOff execution is not separately
qualified by this main-mission fixture. New plot creation/builds retain their
existing history/completion reset. No engine or renderer algorithm changed.

The new QtGui.CompletedDisabledPlots check uses TogglePlotTests' separate
--finish-disabled mode. It does not repeat the preceding TogglePlots workflow.
An independent uninterrupted display run supplies twelve-state report bytes and
retained spacecraft paths. Actual Mission/MDI checklist editing changes a final
ReportFile-only ToggleOff to Orbit/Ground/XY, with pending source protection,
retained clean-panel Apply, exact Undo/Redo, original comments and Unicode
save/reopen. The disabled completed mission must reproduce the independent report
and retained spacecraft coordinates/colors/counts. Orbit/Ground must expose all
retained points at Latest despite the recent-segment limit; Ground/XY remain
inactive. All three plots close/reopen into the same retained model/history.
This verifies the new main-mission completion behavior; previous native
window/rendering, replay and Stop/error evidence is reused rather than rerun.

check-disabled-completion-final.txt records the focused check passing in 0.61
seconds. build-disabled-completion-final.txt records the final MainWindow/receiver
and actual GmatQt-R2026a rebuild plus GmatQt launcher recreation. No full-suite,
previous TogglePlots numerical workflow or native-layout rerun was needed.
The full Linux qualification goal remains active, including Wayland main-window
restore/input, portals and remaining selected workflow/plugin gates. Windows/macOS
remain deferred; check-documents.txt remains the latest complete regression.


## Live Ground Track propagation-block refresh

The preceding completed-disabled checkpoint covers terminal display state. A
separate live-state review found GroundTrack sent RunComplete for every
isEndOfReceive flush. Subscriber::FlushData sets that flag after individual
propagation blocks as well as solver blocks; Subscriber::SetEndOfRun separately
sets isEndOfRun. After the first Propagate, Qt therefore marked Ground Track
complete and ignored NumPointsToRedraw at the live latest frame, even though
additional mission commands were still waiting. Orbit View uses isEndOfRun and
remained live at that boundary. check-live-flush-initial.txt records the actual
first-boundary Ground failure while paused in the Qt command debugger.

GroundTrack now sends Refresh for an intermediate flush and RunComplete only
when isEndOfRun is set. Existing publication, collection-frequency countdown,
coordinate conversion, solver filtering, sample/history storage and numerical
algorithms are unchanged. The receiver already supports both actions; the fix
changes no rendering algorithm or widget layout. The mission coordinator's final
completed-history update from the preceding checkpoint still covers disabled
subscribers whose engine callback is skipped. Intermediate refreshes no longer
claim terminal completion.

QtGui.LivePlotFlush runs TogglePlotTests' separate --live-flush mode. It does not
repeat either previous Toggle mode or the broad debugger suite. The two-spacecraft,
three-block fixture sets recent redraw to one segment. An independent ordinary
execution supplies twelve-state report bytes. The actual Qt debugger then pauses
before two Toggle commands and the final Report, after each block. Orbit/Ground
must still be live with a nonzero recent-segment start at every pause. Ground
close/reopen at the second boundary must retain the same model, samples and live
state. Resuming produces the same report bytes and exact original script; true
completion must finally show the entire retained trajectory at Latest. This is
application-driven offscreen execution/lifecycle evidence; no new compositor input,
portal, native-layout, broader solver or scientific qualification is claimed.

build-live-flush-initial.txt retains a test-only Debugger lookup that incorrectly
used QObject::findChild for a class without Q_OBJECT; it was corrected to the
existing dynamic child lookup before the first executable check. The final focused
check passes in check-live-flush-final.txt (0.61 seconds).
build-live-flush-final.txt records the shared GroundTrack runtime/native plugin
relink and actual GmatQt-R2026a rebuild with GmatQt launcher recreation. No complete
suite, old Toggle modes or native-rendering repeat was needed. Previous native
rendering/window/replay and complete check-documents.txt evidence is reused.
The full Linux qualification remains active with the original outstanding desktop
and workflow/plugin gates. Windows/macOS remain deferred.


## Python call controls and labeled serialization checkpoint (2026-09-30)

The mission editor now distinguishes Python calls from configured GMAT Function
resources. Simple module/function identifiers receive an editable module selector
and a function field; ordered input/output argument browsers use the existing
parameter selector. The module list enumerates configured readable Python files
without importing them and accepts runtime modules such as builtins. The Python
command template appears only when its factory is loaded. Source-span edits retain
labels, comments, argument spelling and unrelated mission/configuration text.
Context Help now routes both labeled and no-output calls to CallPythonFunction.

The actual MDI fixture exposed a plugin serializer defect: the inherited command
label insertion produced `[Magnitude] = Python 'Compute norm'.ArrayFunctions.mag`
instead of a leading label. This prevented safe source alignment even though the
original script built and ran. CallPythonFunction now overrides label insertion
and places the label before the optional output list. The fix is restricted to
Python serialization; engine argument conversion, module caching and numerical
functions are unchanged. check-python-calls-diagnostic.txt retains the malformed
canonical source, and check-python-calls-magtimes.txt retains the initial mapping
failure. Existing unlabeled vector-result evidence is reused from Compatibility.

QtGui.PythonCalls independently configures the shipped ArrayFunctions.magtimes
with scalar/array inputs and checks the expected scalar result 15. The GUI then
changes mag to magtimes and selects ordered inputs through the actual MDI panel,
compares report bytes with that independent configuration, and verifies Cancel,
pending edits, retained clean Apply, exact Undo/Redo and Unicode save/reopen.
The same mission exercises a no-output builtins.print call with a String input.
Missing module and missing function names are applied through the GUI; execution
fails without losing source, then GUI correction/save/reopen recovers the same
report. The actual Help button opens the installed Python command page. Pure
source-span checks retain nested and quoted commas, labels and trailing comments;
they do not establish runtime support for literal arguments or arbitrary Python
expressions. The final focused check passes in check-python-calls-label-help.txt,
0.64 seconds. No complete regression, previous Python vector or viewer cases were
repeated. The earlier initial check incorrectly named the shipped function
scaleMag; that fixture error and test-only compile corrections remain in the raw
evidence and are not treated as product failures.

python-calls-wayland-20260930.txt and .png record one native Wayland actual MDI
preview. The module/function fields and ordered argument buttons were captured
and inspected; controls fit without clipping. This preview performs no Python
calls and does not claim top-level minimization, portal interaction or fresh
compositor input. build-python-calls-label-help.txt records the rebuilt native
Python/ExternalForce plugins and actual GmatQt-R2026a application. The final
application rebuild after documentation/comment cleanup is recorded separately.

This closes the missing call-control and representative lookup recovery cases,
not every Python function shape. Zero-input calls, package/dotted function syntax,
other return/error types and modified-module cache recovery remain unqualified;
unsupported syntax stays available in the source editor. Shared Wayland desktop
and other selected workflow/plugin acceptance gates remain active. Windows/macOS
remain deferred, MATLAB remains off, and full Linux replacement is not yet claimed.


## Python empty/bare-input and single-output source mapping (2026-09-30)

The previous checkpoint implemented call controls but left zero-input calls
unqualified. A new focused mode confirmed a concrete problem: Python builtins.float
and print run with empty inputs, while CallFunction serialized them without `()`.
The controls required parentheses and mission alignment compared their presence
literally, leaving the actual command read-only. check-python-zero-initial.txt
retains that failure. CallFunction now writes an empty argument list specifically
for CallPythonFunction; other function types retain their prior serialization.
The existing Python label override is reused. No argument conversion or numerical
algorithm changes were made.

The shipped CallPythonFunction reference also permits empty calls without
parentheses. Qt MissionModel now normalizes only the documented empty/bare-input
alternatives and brackets around a single simple output name. Exact labels,
ordered output names, module/function identifiers and argument text are retained.
The mapper still rejects different module, function, output, argument or label
source against the built command. Those negative alignment cases are exercised
without executing more missions. Bare calls receive the same controls and Help.
Adding arguments inserts parentheses at the original function end; clearing them
restores the original bare spelling. Function-only edits keep bare calls bare.

QtGui.PythonZeroInputs is a separate --zero-inputs mode. Its independent int()
report is zero; the actual MDI editor changes float() to int(), retains Apply,
performs exact Undo/Redo/Unicode save/reopen and reproduces that report. A labeled
print() with no inputs or outputs runs in the same mission and retains its source
mapping/form shape. check-python-zero-final.txt passes in 0.37 seconds after the
serializer fix. Later source-normalization changes leave the already equal
parenthetical case on the existing exact-comparison path; that numerical case
was not repeated.

QtGui.PythonScalarOutput and QtGui.PythonBareCalls use separate fixture modes.
The first executes an unbracketed single-output abs(Result) call, edits it to
float(Result) in the actual MDI editor and retains its unbracketed source spelling
through exact Undo/Redo/Unicode save/reopen and independent report agreement.
The second covers labeled float/int and print calls with no parentheses, the
same retained MDI/source/result workflow, adding/clearing arguments in a bare
source-span form, and the bare-call Help topic. Their final checks pass in
check-python-syntax-final.txt (0.31 and 0.34 seconds, total 0.65).

The initial scalar fixture included a label before an unbracketed output. The
engine treated that label as extra output names, producing a runtime failure;
check-python-scalar-initial.txt records this unsupported fixture shape. It was
changed to an unlabeled scalar output, which built/ran but exposed the bracket
normalization gap in check-python-scalar-unlabeled.txt. This checkpoint does not
claim engine support for labels before unbracketed outputs. Labeled bracketed
outputs remain covered by the other modes. This syntax work is limited to the
selected Python GUI workflow and does not broaden engine parsing semantics.

build-python-syntax-final.txt records the final shared engine/native plugin and
actual GmatQt-R2026a rebuild, with GmatQt launcher recreation. The earlier Python
runtime recovery, argument browser, vector result and native Wayland layout
checks are reused. No layout changed, so no native preview or broad suite was
repeated. Other Python return/error types, packages and changed-code cache
recovery remain under the inventory limits. Native Wayland main-window input,
portal and other selected workflow/plugin gates remain open; full Linux
replacement is still in progress and Windows/macOS remain deferred.


## Active XY plot axis and label controls (2026-09-30)

TsPlotCanvas::SetOptions is the active TsPlotOptionsDialog caller. It loads and
applies plot title, X/Y labels, line width, independent minimum/maximum overrides,
major tick counts and label precision. Qt's existing Style action supplied curve
styling/grid/legend but omitted those active axis/label operations. The audit also
finds that both logarithmic checkboxes and minor-tick controls are disabled and
are not read/applied by SetOptions. The line-style text field has no applied
GetStyle call; Qt's already implemented curve-style selector is more capable.
audit-xy-active-options.txt records the caller/settings and disabled controls.
No extra logarithmic or minor-tick behavior is inferred from inactive widgets.

The Qt Style dialog now has Plot, X axis, Y axis and Curves tabs. Plot provides
title and axis labels with the existing grid/legend choices. Each axis provides
independent fixed minimum/maximum checkboxes and text values, major tick intervals
(X 1–20, Y 1–25) and significant-digit label precision (2–16), matching the active
wx ranges. Unchecked limits follow data. Invalid/nonfinite/out-of-range values or
minimum greater than/equal to maximum disable OK with an explanation; correction
re-enables it. Values remain private until OK, and Cancel changes nothing.
Changing limits resets the viewer's pan/zoom to show the chosen range. A single
fixed edge beyond current data keeps a valid automatic opposite edge as data
arrives. Curves clip to the selected bounds. Numeric gutters grow for longer
precision labels; extremely narrow views can still constrain label space.

These are display settings, matching the wx plot options workflow. They do not
change the mission resource or script and reset on rebuild. Accepted settings
and collected points remain in the receiver model when a viewer closes/reopens.
Orbit/Ground drawing/camera behavior and their default tick labels are retained;
new limits/ticks/precision are applied only in the XY paint branch. No subscriber
publication or numerical algorithm changed.

QtGui.XYAxes runs a small actual mission/MDI workflow. Ordinary execution provides
independent report bytes. Through the real Style action the test edits labels,
limits, ticks and precision, checks Cancel, equal/nonfinite/nonnumeric correction,
accepted rendering, unchanged sample count/source/report, close/reopen values,
independent one-sided limits beyond data, removal of all overrides and exact
Unicode save/reopen/rerun results. Rebuilding must reset transient options. A
separate deterministic chart checks a point at independent expected screen
coordinates under two ranges and its clipping outside the selected range. It
also checks X/Y grid-line positions after tick changes and rendered numeric-label
regions after precision changes. This qualifies those GUI operations, not the
engine's numerical methods or every possible data range/precision combination.

The initial check passed in check-xy-axes-initial.txt (0.45 seconds). Rendered
X/Y tick/precision checks were added and passed in check-xy-axes-ticks.txt. The
final product change reserves wider numeric gutters; check-xy-axes-final.txt
passes in 0.52 seconds. No broad PlotTests, previous solver/Toggle/native-rendering
or full regression rerun was performed. Earlier curve-style/callback and viewer
history evidence is reused. build-xy-axes-final.txt records the actual
GmatQt-R2026a relink and GmatQt launcher recreation.

xy-axes-wayland-20260930.txt and .0/.1/.2/.3.png capture all four native tabs in
an actual MDI plot inside the initialized MainWindow. The tabs were inspected;
editable fields fit and the prior curve controls remain accessible. This preview
uses synthetic display points and performs no numerical mission execution. The
final numeric-gutter change does not alter the inspected dialog layout, so its
preview was not repeated. Top-level compositor input/minimize and portal gates
remain unqualified; this does not close those gates or claim full Linux
replacement completion. Windows/macOS remain deferred.


## XY retained-data export and populated display evidence (2026-09-30)

TsPlotCanvas's active right-click menu exposes Export Data and its SaveData caller
uses DumpData. Qt previously provided Save image without this data operation.
audit-xy-export-format.txt records the wx writer: title, X/Y labels, then one
curve-name section containing ordered `x, y` rows and a blank separator. Qt's new
XY Export data action preserves that text structure with UTF-8 names and 17-digit
C-locale doubles, rather than wx's 15-digit values. The extra precision permits
stored doubles to round-trip independently of axis-label precision. Export includes
all retained curves, hidden curves and out-of-range points; evicted MaxPlotPoints
history is not reconstructed, and plot-error metadata is not added to the legacy
X/Y format. No collection, subscriber publication or numerical behavior changes.

The writer uses QSaveFile and bounded streaming rather than a whole-export text
buffer. Cancel writes nothing. Open/write/commit errors return a diagnosis and
preserve the previous destination through atomic replacement. The live receiver
supplies current open-document, startup, report/solver/event and ephemeris paths;
export refuses to overwrite those files, including canonical symlink aliases.
The actual menu displays a write diagnosis. Regular user-chosen export files can
be replaced through the existing save chooser confirmation. This protection is
specific to the new data action and does not claim a broader image-export audit.

The first algebraic fixture produced zero XY samples: assignments/Report alone
were not publishing chart data. Its 20-byte headers fit below the induced file
limit, so the supposed write-failure assertion failed. check-xy-export-initial.txt
and check-xy-export-diagnostic.txt preserve the failure and its nonempty-error/
zero-sample diagnosis. The fixture was replaced with a short spacecraft
propagation and now explicitly requires more than three actual engine-published
samples. This was a fixture weakness, not a failed atomic writer.

That observation also bounds the preceding XYAxes algebraic MDI fixture: its
sample-count assertions covered an empty model. Its independent synthetic chart
still proves rendered limits, clipping, ticks and precision; its MDI controls,
Cancel/correction, exact source/report and close/reopen assertions remain useful.
XYExport now adds populated actual MDI display evidence: hide the collected curve,
set a minimum beyond the data and low axis precision through the Style dialog,
then export every retained sample exactly. These GUI settings retain the collected
samples and source/state reports and survive viewer close/reopen; rebuilding the
saved source recreates the same independent state report.

QtGui.XYExport invokes the actual export action and widget chooser for Cancel
and acceptance to a Unicode file. It checks headers, ordered samples against the
retained engine model, unchanged display/source/report, source/output/startup and
symlink rejection, missing-directory failure, close/reopen and repeat export,
and save/reopen/rerun state-report invariance. A process-local 128-byte file-size
limit forces an actual post-write failure on a populated export; the prior file
must remain byte exact, and removing the limit must recover the same complete
export. A synthetic sparse-ID, hidden/empty-curve case separately checks Unicode,
C-locale syntax under German defaults and exact double values despite view
range/precision changes. check-xy-export-populated.txt passes in 0.37 seconds.
No prior XYAxes, broader PlotTests, native-rendering or full-suite repeat was made.

build-xy-export-initial.txt records the real GmatQt-R2026a rebuild and launcher
recreation with the final product changes; subsequent builds change only the
fixture. xy-export-wayland-20260930.txt and its .picker/.plot.png captures record
the new action and exposed Wayland widget chooser, with Cancel/no file creation.
Both were inspected. The native preview uses synthetic display points and no
mission execution; it does not qualify desktop portals. The full Linux acceptance
goal remains active with original compositor-input, portal and other selected
workflow/plugin gates; Windows/macOS remain deferred.


## Plot image export protects mission files and recovers atomically (2026-09-30)

The existing Save image action used QSaveFile but did not reject open mission,
startup or configured output paths. It could replace source or a report with PNG
bytes. The action now shares the data export's live protected-path validation
before capturing/writing. Canonical symlink aliases are rejected. Failed image
encoding cancels the temporary writer; open/encode/commit failures return a
corrective diagnosis through the existing warning. Ordinary chosen image files
retain save-chooser replacement confirmation and atomic replacement.

QtGui.PlotImageExport uses the initialized MainWindow and actual MDI Save image
actions for XY, Ground Track and Orbit receiver entries. Widget chooser Cancel
writes nothing; acceptance writes a Unicode PNG whose decoded pixels equal the
current canvas. Current mission, startup, configured ReportFile/EphemerisFile
paths and a mission symlink are rejected and remain byte exact. Empty/missing
destinations return errors. A process-local 128-byte file limit induces a real
post-write failure for each viewer kind, preserving the prior destination;
removing it recovers the complete PNG. Retained model/frame/generation/display
and close/reopen export checks pass. check-plot-image-export.txt passes in 0.90
seconds. Synthetic receiver entries and sentinel outputs isolate the file-writing
operation; no numerical execution or native renderer qualification is claimed.

The previous native widget chooser/action layout and rendering evidence is reused:
this change adds validation and a diagnostic path, without changing layout or
rendering. No native preview, XY-export, broad viewer or full-suite repeat was
needed. build-plot-image-export.txt records the real GmatQt-R2026a relink and
GmatQt launcher recreation. This closes the image-export file protection gap,
not the full Linux replacement goal or the remaining compositor-input/portal
gates. Windows/macOS remain deferred.


## Selected plugin creation categories and file-backed GMAT functions (2026-09-30)

audit-plugin-creation-categories.txt identifies actual New resource gaps:
Formation and GroundStation factories use FORMATION and SPACE_POINT categories,
which the Qt creator omitted; EKF Smoother, ProcessNoiseModel and EstimatedParameter
factories use dynamically registered categories, also omitted. The creator now
queries these viewable factory categories, retaining its sorted/deduplicated list.
Existing Solvers/Process Noise Models/Estimated Parameters resource folders already
display these objects; no duplicate Smoother folder was added. Normal Linux does
not register the TESTING-only DataFile creation type. Owned propagator, atmosphere,
physical-force and measurement subtypes retain their established owner controls.

The first focused creation check accepted thirteen types then exposed a separate
GmatFunction defect: the complete candidate interpretation rejected a new function
whose `.gmf` did not yet exist. The New function file editor was only accessible
from an existing resource. check-plugin-creation-initial.txt preserves that failure.
check-plugin-creation-remaining-initial.txt isolates it without repeating the
thirteen successful creations; the engine log at diagnosis reported missing
`Added0.gmf`. Qt now offers a Function file field, Browse and New file in the creator.
An empty/missing/unreadable path gives a correctable message before interpreting.
New file uses the existing atomic function editor and an editable pass-through
template, then supplies its saved path to one source-preserving resource insertion.
Saving that external file is explicit and separate from the mission edit; creator
Cancel/mission Undo retain saved files, as the dialog explains. Existing function
files remain unchanged when imported. The numerical engine was not changed.

QtGui.PluginCreation covers eighteen actual menu-created resource types: Formation,
GroundStation, Smoother, ProcessNoiseModel, EstimatedParameter, ExtendedKalmanFilter,
Simulator, BatchEstimator, TrackingFileSet, ErrorModel, AcceptFilter, RejectFilter,
FileInterface, GmatFunction, Yukon, ThrustHistoryFile, ThrustSegment and EclipseLocator.
Each checks creator Cancel/accept, the opened resource editor, tree/model identity,
exact unrelated source and per-type Undo/Redo. A combined Unicode save/reopen
retains every object. Actual context-menu deletion and per-type Undo/Redo remove
only each selected declaration/settings and finally restore the byte-exact baseline.
The GmatFunction case checks missing-path correction, new-file chooser/editor
Cancel without writes, explicit Save and the resulting path/source insertion.
Its saved function changes input 9 into independently expected 19 in one small
mission. check-plugin-creation-functions.txt passes in 1.09 seconds.

QtGui.FunctionImport separately exercises Browse Cancel/accept, missing-file
correction, an Imported resource backed by Existing.gmf, exact source and original
file bytes, Unicode save/reopen and independently expected aliased-call result 25.
check-function-import.txt passes in 0.25 seconds. Existing plugin numerical/report
fixtures were reused, rather than rerun for these menu transactions. These checks
qualify creation/removal and file association, not initialized execution of all
eighteen default objects or broader scientific regimes. No previous compatibility,
viewer or full-suite repeat was made.

The native Wayland creator preview initially showed clipped explanatory text;
plugin-creation-wayland-20260930.txt and .creator.png retain that observation.
The note was shortened. plugin-creation-wayland-final-20260930.txt and .creator.png
show the final exposed file controls and readable note; both previews were inspected.
Native creator Cancel retains source. No native mission or portal run was performed.
The final preview/text-only correction reused the successful execution assertions.
build-plugin-creation.txt records the actual GmatQt-R2026a relinks and GmatQt
launcher recreation, including the final note. Full replacement qualification
remains active; compositor input/minimize, portal and other selected workflow
requirements retain their documented limits. Windows/macOS remain deferred.


## Current-mode solver-loop plot toggles (2026-10-01)

The prior TogglePlot and SolverPlot suites separately covered ordinary mission
toggles and solver display modes. QtGui.SolverToggle adds their interaction in
an actual differential-corrector branch with Current-mode Orbit/Ground/XY plots.
A separate source-configured baseline toggles only an idle report subscriber;
actual Mission/MDI controls then select the three plots for Off/On inside the
branch, retaining labels/comments, pending Apply, exact Undo/Redo and Unicode
save/reopen. All trial/accepted state and geodetic report bytes must agree with
the baseline. Multiple iteration reports are required, not merely a completed
run. The accepted objective is independently checked at X=7100 within 1e-6,
with branch/post-branch epochs at elapsed 140/160 seconds.

The first fixture incorrectly treated elapsed-time stop conditions as absolute
endpoints; they are durations. Its intended 50–80 second boundary was absent,
and check-solver-toggle-initial.txt preserves the failed assertion. Durations
were corrected to pre-branch 20, branch 30/30/60 and post-branch 20 seconds.
check-solver-toggle-timed.txt then passed in 1.22 seconds. Final assertions also
require multiple trial/accepted reports, the independently expected objective/
epochs, accepted final Orbit/Ground/XY coordinates, and the camera's final frame
and target agreeing with the collected spacecraft. check-solver-toggle-final.txt
passes in 0.59 seconds.

Every plot rejects samples strictly inside the disabled 50–80 interval and
retains at least one resumed point with an unconnected arc. The Current-mode
Orbit/Ground buffered publication did not bridge that interval. Actual MDI
activation/capture and close/reopen preserve the retained model and point count.
No product display fix was needed for this covered case. Existing native
rendering, ordinary Toggle and other solver-mode evidence is reused; none of
those suites or the full regression was repeated. build-solver-toggle.txt
records the new focused target build against the current real application.
The product binary did not change after the prior actual rebuild. Optimizer/
nested/ephemeris toggle combinations and original compositor/portal gates remain
open; this bounded check does not establish full Linux replacement completion.
Windows/macOS remain deferred.


## Newly generated function templates follow chosen filenames (2026-10-01)

The recent file-backed creator/import checks established that a configured
resource may alias a differently named `.gmf`. Source inspection then identified
a gap in both New resource and the existing resource's New function file action:
the template declaration always used the resource name, regardless of the
chosen filename. Interpreter function validation requires the declaration to
match the file basename. audit-function-template-names.txt records those source
locations and the already qualified alias association.

Both actions now share a small template helper that uses the chosen `.gmf`
basename for its function declaration. The resource alias remains unchanged.
An invalid new filename (non-identifier basename or non-`.gmf` extension) produces
a correctable diagnosis before opening/writing a new template. Existing-file
Browse/import is unchanged. The action callbacks catch template validation
errors; no parser or numerical algorithm was modified.

QtGui.FunctionFileNames exercises the actual creator and the resulting MDI
resource editor. Resource Alias is created with FirstFile.gmf, then its New
function file action saves SecondFile.gmf into a Unicode directory. Each template
header is checked against the independently chosen filename. Invalid `bad name.gmf`
choices in both actions create no file or pending/source edit. The second valid
path stays pending until Apply. Unrelated source remains exact, the configured
path changes, exact Undo/Redo restore the source snapshots, and both explicitly
saved external files remain byte exact. Unicode mission save/reopen retains the
Alias call, whose generated pass-through template returns independently expected
9 without modifying mission/file bytes.

The first assertion expected the changed path assignment to retain its original
location/prefix. check-function-file-names.txt and
check-function-file-names-diagnostic.txt preserve that failure and its source
comparison. The established resource patcher intentionally moves changed
assignments before the mission boundary while retaining unrelated source.
The check now excludes only that changed assignment when comparing unrelated
source, verifies its effective configured path, and retains exact complete
Undo/Redo and saved-source checks. No broad source-patcher change was needed.
check-function-file-names-final.txt passes in 0.30 seconds.

build-function-file-names.txt records the real GmatQt-R2026a relink and GmatQt
launcher recreation. Existing creator/file-editor native layout and broader
creation/import/compatibility/numerical evidence is reused; no native preview
or previous suite was repeated. Remaining workflow/plugin and compositor/portal
requirements remain open. Windows/macOS are deferred.


## GMAT empty/bare call editing and leading labels (2026-10-01)

CallFunction serializes zero inputs without parentheses and single outputs with
brackets. Qt source alignment previously treated those accepted alternate
spellings as different commands, disabling safe mission edits. CommandForm also
omitted input/function controls for bare calls, and scalar-output calls routed
context Help to Assignment. audit-gmat-call-syntax.txt records the scoped source
findings. Qt now normalizes only configured GmatFunction call spellings during
alignment, preserving labels, function identity and ordered outputs/arguments.
Bare function forms require a configured Function, so Stop and variable
assignments retain their original controls. Adding inputs to a bare call inserts
parentheses; clearing that field restores the original bare spelling.

The initial unlabeled cases passed in check-gmat-call-syntax-initial.txt (0.76
seconds). Adding an accepted leading label reproduced a separate serializer
bug: GmatCommand's keyword-based InsertCommandName omitted labels when the
GMAT keyword was disabled. check-gmat-call-labels-initial.txt retains that failure
and the canonical source missing the label. CallGmatFunction now overrides only
label insertion, placing it before the optional output list, after an optional
GMAT prefix. No parser or numerical algorithm changed; MATLAB is still deferred.

check-gmat-call-syntax-final.txt passes in 0.98 seconds. QtGui.GmatCallSyntax
executes actual MDI edits for bracketed empty inputs, bracketed bare calls,
unbracketed scalar output and labeled empty/bare calls. Each function change
stays pending until Apply, retains the clean open panel, exact unrelated source,
comments and original call spelling, then survives exact Undo/Redo and Unicode
save/reopen. Separately source-configured execution must produce output 13 and
global side-effect count 2. The latter requires both labeled Touch() and bare
Touch to execute; merely registering/building the function is insufficient.
Changed function/output/arguments/labels cannot align as equivalent. Both
no-output forms expose empty input controls. Bare-input insertion/restoration
and non-function exclusions are checked directly. A small Python span check
covers the shared input wrapper without repeating prior Python execution suites.

build-gmat-call-syntax.txt records the actual GmatQt-R2026a relink, GmatQt launcher
recreation and selected GmatFunction plugin rebuild. Existing ordered-argument,
creation/import and native editor/layout evidence is reused. No native preview,
viewer suite or full regression was repeated. Broader function signatures and
remaining desktop/plugin requirements stay open; this bounded case does not
establish full Linux replacement completion. Windows/macOS remain deferred.


## Yukon optimizer viewer histories and native replay (2026-10-01)

The earlier SolverPlots evidence covered differential-corrector histories only.
QtGui.OptimizerPlots adds a separate --optimizer mode to that existing harness;
CTest dispatches it independently, so the completed targeter fixture was not
repeated. The Yukon fixture minimizes Cost=(Alpha-2)^2, starts Alpha=1, and sets
a spacecraft's X from Alpha before a 60-second propagation. Pre/post propagation
last 20 seconds each. A separately source-configured no-display baseline must
produce multiple iteration reports, final elapsed time 100 seconds, Alpha within
1e-5 of the independently known optimum 2 and Cost below 1e-10. Viewer controls
then select Current, None and All on Orbit/Ground/XY resources. Pending Apply,
exact Undo/Redo and Unicode save/reopen precede each displayed execution. Every
complete state/geodetic report must remain byte exact with the baseline; each
mode also retains the known optimum.

check-optimizer-plots-initial.txt passes in 1.38 seconds; the corresponding
optimizer-plots-initial-details.txt records covered histories. All retains 76
samples per plot. Current retains Orbit 20 and Ground/XY 13; None retains 13 in
all three. Current Ground/XY paths must agree with the independently filtered
None paths within 1e-8 coordinates and 1e-10 days. Orbit endpoints and the final
camera target agree with the state report; Ground endpoints agree with reported
longitude/latitude, trial points retain target colors, and XY endpoints agree
with elapsed time/X. These assertions preserve the established Orbit Current
latest-trial history behavior rather than equating it with None.

optimizer-wayland-20261001.txt records the initial native pass and all three
mode histories. Its retained .Current.png and .Current.Orb.png showed that this
fixture's 20000 km camera distance clipped Earth in the short tiled viewport.
Only the optimizer fixture's distance was increased to 40000 km. The product
camera and targeter fixture were unchanged. The affected native run was repeated;
optimizer-wayland-final-20261001.txt and the final .Current/.None/.All tiled and
individual plot captures retain its evidence. The final Current Orbit and All
tiled images were inspected: the full textured Earth, spacecraft/short trajectory,
starfield, Ground map and distinct XY iteration history are visible. The same
native run verifies earlier-frame replay differs, Latest restores the original
frame, retained sample counts survive replay, and immediate viewer close/reopen
retains the data. It does not qualify top-level compositor minimize/restore.

No product rendering fix was needed for this covered optimizer case. No full
suite, prior differential-corrector, compatibility or creation suite was repeated.
build-optimizer-plots.txt records the focused harness build against the current
real GmatQt and selected plugin runtime; application/bin/GmatQt remains the
rebuilt launcher/binary from 4ba98b6 because product source did not change here.
Optimizer toggles, nested solver cases, broader plugin/error settings and the
original native desktop/portal gates remain open. Windows/macOS are deferred;
no optimizer algorithm or mathematical engine was rewritten.


## Nested targeter plot scope, initialization and recovery (2026-10-01)

NestedSolverPlots adds a separate --nested fixture to the solver viewer harness,
with two correctly paired differential-corrector Target sequences. Alpha starts
at 1 and targets 2; inner Beta starts at 1 and targets 3. Both controls affect
spacecraft state before propagation. Pre/three branch/post propagations are 20
seconds each, so the accepted final elapsed time is 100 seconds. The complete
iteration report and both independently known goals are checked against a
separately source-configured no-display baseline. Plot-mode edits retain pending
Apply, exact Undo/Redo, labels/comments and Unicode save/reopen.

The original check-nested-solver-plots-initial.txt failed: Current Ground/XY
retained 27 points while None retained 45, and both disagreed with the intended
accepted history. audit-nested-solver-display.txt traces two causes. The inner
solver's globally published accepted state previously reclassified plot data
while its parent was still solving. Current-mode clearing also used the latest
inner breakpoint when the outer solver needed its own anchor. The fixes are
plot metadata: Target/Optimize scope their complete dispatch with live solver
states; Orbit/Ground/XY alone consult the effective enclosing plot state.
Publisher state, report subscriber filtering and all mathematical execution
remain unchanged. This separation matters because Propagate consults publisher
state when setting up stop conditions. Per-solver/per-curve frame anchors now
identify the matching Current-mode clear, including an ancestor pruned from the
ordinary breakpoint list by point retention.

The intermediate branch-only change passed check-nested-solver-plots-scoped.txt
(2.75 seconds), but native inspection of nested-solver-wayland-20261001.Current.png
revealed initialization samples still present through elapsed 40 seconds. That
passing result is insufficient for complete nested scope. An inner Target can
execute during its parent's initialization, outside ExecuteBranch. A complete
Target/Optimize dispatch scope and the parent Target breakpoint placed before
inner initialization cover that path. check-nested-solver-initialization.txt
retains the resulting trial-classification failure while this was corrected.
The final None XY assertion rejects all post-20-second samples below X=7150,
which excludes the independently identified initialization/outer trial paths;
endpoints still must match the numerical report exactly within display tolerances.

check-nested-solver-final.txt passes all five affected suites in 5.73 seconds:
ordinary SolverPlots, OptimizerPlots, SolverToggle, NestedSolverPlots and
NestedSolverCleanup. Final All retains 72 samples per plot. Current retains
Orbit 18 and Ground/XY 15; None retains 15 in each. Current Ground/XY paths agree
with None within 1e-8 coordinates and 1e-10 days; they contain no trial samples.
Orbit Current keeps its established latest trial in addition to accepted history.
Camera target/final Orbit state, Ground geodetic endpoints and XY elapsed/state
endpoints agree with reports. The final native Wayland run and all three modes'
scene captures are nested-solver-wayland-final-20261001.txt/.Current/.None/.All.png
and their individual plot images. Current and All tiled captures were inspected:
full textured Earth/starfield, Ground map, distinct XY trials/accepted history,
replay/Latest restoration and immediate viewer close/reopen pass. This does not
qualify top-level compositor minimize/restore or fresh native input.

The pre-fix nested-before-fix.state.txt and final native .state.txt reports contain
19 complete explicit state/geodetic rows and compare byte identical. Both SHA-256
values are 82c7ee3f13eee1bccf40ff66cb989f993d666fe1c787fa12bfa5abdcdef723e2;
nested-report-invariance.txt records the comparison and known goals. Numerical
or report behavior was not rewritten to make the display checks pass.

NestedSolverCleanup executes Save Alpha inside the inner solver with an invalid
output directory, retains the failed source, restores the output path and
corrects/reopens the Unicode mission. An actual debugger breakpoint in a command
owned by the inner Target triggers Stop. Both failures remove the nested trial
context and release the debugger/mission. A corrected rerun reproduces the full
report, goals and All histories. Its two-point synthetic retention case discards
all inner trial samples from the outer anchor even after trimming that anchor,
then requires a disconnected resumed sample. The initial harness called the
clear helper without its required break index; build-nested-solver-plots.txt
retains the compile error. check-nested-solver-retention-stale.txt ran the prior
binary and is not evidence for that added assertion. The rebuilt assertion
passes in check-nested-solver-retention-built.txt (2.52 seconds), and the final
five-suite regression includes it. Stop breakpoint ownership was tightened to
the inner node label before the final checks. Raw failed and intermediate
observations are retained; earlier broad suites were repeated only where these
material shared display changes affected their behavior.

build-nested-solver-plots.txt records the final real GmatQt-R2026a relink, GmatQt
launcher recreation and selected plugin relinks. No full-suite repeat was made.
The covered two-level Target case closes that bounded viewer gap. Optimizer
nested/toggle and mixed/deeper solver cases, other plugin/workflow requirements
and the original compositor/portal gates remain open. Windows/macOS are deferred.


## Apply solver corrections qualification — 2026-10-01

A fresh audit of active TargetPanel.cpp and OptimizePanel.cpp identified an
operation omitted from the earlier rows: both wx panels have Apply Corrections,
while Qt had none. This is now an actual button in retained Target/Optimize
command panels. It uses the engine's existing Vary::SetInitialValue, including
inverse additive/multiplicative scaling, and patches only mapped numeric initial
guesses in the original script. Reference-based guesses remain unchanged and
are explained in the Message Window. No solver algorithm or numerical execution
was rewritten. A changed correction is one undoable source transaction, followed
by validation/rebuild and refresh of clean companion command/resource panels.
Pending panel changes and pending/stale source are protected; rebuilds invalidate
old results. A solver block that did not execute cannot apply another block's
result. The existing wx rule does not require convergence.

Command pointers and an additional source snapshot exposing script-event
commands are captured before execution. Ordinary Mission navigation still treats
BeginScript as one editable event. This matters after ExitMode=Stop or an
incomplete solver, because GetNext can throw or return the same executing
command. Corrections use pre-run ownership instead of traversing that state.
The numerical unary plus omitted by serialization is now accepted in source
alignment; binary addition and quoted labels remain significant. The fixture's
+1.000e0 is retained until that particular guess is intentionally corrected.

check-solver-corrections-final.txt passes QtGui.SolverCorrections in 3.58 seconds.
Actual MDI buttons target the independently known variable goals Alpha=4,
Beta=5, nested Gamma=7 and Yukon's quadratic Delta=2 (1e-5 optimizer tolerance).
It verifies scaling, exact unrelated source/comments/labels/options, retained
Seed reference, pending branch and Vary guards, refreshed clean companions,
exact Undo/Redo and Unicode save/reopen execution. Additional cases cover a Vary
inside BeginScript, an unexecuted If branch, corrections after ExitMode=Stop,
and an iteration-limited DC's actual last value followed by correction/recovery
to the known goal 4. ExitMode=Stop currently reports the engine interruption as
MainWindow::RunResult::Failed; this test does not claim broader stop-status UI
parity. The source fixture uses default DiscardAndContinue, with Variables; it
does not qualify spacecraft restoration under every solver mode.

check-solver-corrections-regression.txt passes the three affected existing suites
Mission, Inspection and Debugger in 4.98 seconds. This precedes the final small
reference-retention message and nonconvergence fixture addition; those are
covered by the final focused suite. No full regression suite or earlier viewer
mode matrix was unnecessarily repeated. build-solver-corrections.txt records the
actual GmatQt-R2026a relink and GmatQt launcher recreation as well as test builds.

solver-corrections-wayland-20261001.txt/.png records a native Wayland run and
inspected optimizer panel: corrected numeric source, readable retained command
form, visible Apply Corrections/Apply/Close, and the engine change message. That
run includes the script-event/unexecuted/ExitMode=Stop cases and predates only
the added reference-retention explanation/nonconvergence case. It establishes
native panel layout and programmatic control operation, not fresh desktop input,
top-level compositor minimize/restore, portal choosers or native viewer rendering.

Raw initial observations are retained. build-solver-corrections-initial.txt is
the compile-time parameter-overload error; no test was run against that failed
build. check-solver-corrections-initial/mapping/mapping-diagnostic.txt preserve
the unavailable panel and serialized source before the unary-plus fix.
check-solver-corrections-positive-guess/goals-diagnostic.txt records the test's
incorrectly tight optimizer assertion with default perturbation 0.001 (Delta
1.9995). Explicit perturbation 0.000001 and tighter configured tolerances correct
the fixture, without changing the numerical engine. The scaled and branches
checkpoints pass before the final additions. The nonconverged-initial checkpoint
used MaximumIterations=1 and stopped before any step; the final limit of 2
produces a new, nonconverged last value and covers the intended operation.

This closes the missing active solver correction action for these mapped source
cases. Full replacement qualification remains open: other plugin/workflow
requirements, broader solver combinations, and the compositor/portal acceptance
gates still need affirmative evidence. Windows/macOS remain deferred.


## Solver modes and terminated viewer history — 2026-10-01

The documented ExitMode distinction concerns the next execution of the same
solver block in control flow: SaveAndContinue uses the previous solution as its
next guess; DiscardAndContinue uses the original Vary guess. It does not mean
that the completed spacecraft propagation is discarded. The authoritative local
references are doc/help/src/Command_Target.xml and Command_Optimize.xml, and
Solver::CompleteInitialization/ResetVariables. SolverModeTests exercises both
blocks twice inside a labeled For with real spacecraft propagation, state and
geodetic reports, and Orbit/Ground/XY All histories. Known solved Alpha is 2;
RunInitialGuess keeps 1. Continuing modes reach elapsed 80 seconds; stopping
modes reach 40 seconds and do not execute the second invocation. The first
second-invocation report must start at the documented saved/discarded guess.

The first executable check, check-solver-modes-initial.txt, passes Target Solve
Save/Discard but finds intentional ExitMode=Stop classified as Failed. Existing
partial summaries and enabled viewer histories pass that check; no speculative
summary or engine traversal rewrite was made. Moderator::RunMission returns -4
for intentional Stop, ExitMode=Stop and user interruptions. Qt previously
required its own stopRequested flag as well. It now trusts the engine status and
shows Mission stopped with stopped-run summary context. Actual runtime errors
still report Failed. SolverCorrectionTests was updated to the correct stopped
result after its ExitMode=Stop case; its correction/recovery still passes.

check-solver-modes-stopped-status.txt passes all 12 advertised Target/Yukon
SolveMode/ExitMode combinations in 9.40 seconds. Actual retained MDI dropdowns
apply both choices; unrelated labels/comments/options and the complete script
remain exact. Nontrivial changes have exact single Undo/Redo, then Unicode
save/reopen execution. Each displayed GUI run matches a separately constructed
script with plotting disabled byte for byte in its complete explicit
state/geodetic report, and meets the independently known objective/epoch.
Partial mission summaries include the named solver and spacecraft. Retained
viewer endpoints match reports; replay/Latest and close/reopen preserve history.
This matrix predates only the disabled-terminal display fallback and subsequent
fixture-only explicit solver report destinations/recovery assertions; it was
not unnecessarily repeated after those changes.

That same checkpoint fails DisabledSolverStop: a completed solver followed by
Toggle Off and an explicit Stop leaves disabled Orbit/Ground Latest limited to
one live segment. The previous missionCompleted fallback ran only on success.
QtPlotReceiver::missionFinished now finalizes retained (possibly partial) Orbit
and Ground histories after every terminal outcome. The main mission and each
started folder-run item call it. It sets endOfRun and forces a viewer refresh;
it neither claims success nor creates missing samples. Historical replay before
Latest retains the configured live segment behavior. Core numerical execution,
Publisher state and subscriber report filtering were not changed.

check-solver-modes-termination.txt passes SolverCorrections, DisabledSolverStop
and DisabledSolverFailure in 2.75 seconds. The two new variants complete a DC or
Yukon solution, disable all three plots, then execute Stop or Save Alpha with a
missing output directory. Source and independent partial reports remain exact;
known final goal/epoch, viewer endpoints, complete retained Latest, partial
summary, released mission/editor locks, replay and close/reopen are asserted.
The Save failure stays Failed, distinct from intentional Stop. Report paths are
explicit valid fixture paths, so the missing output directory fails at Save
after data have been collected, not during solver initialization.

check-solver-modes-recovery-regression.txt passes the five affected existing
Inspection, CompletedDisabledPlots, LivePlotFlush, FolderRun and Debugger suites.
Its new recovery assertion fails only because the harness expected Alpha.gmat;
the selected Save plugin writes Alpha.Variable.data. That raw failed assertion
is retained. The corrected DisabledSolverFailure with no application change
passes in check-solver-modes-recovery-final.txt (1.15 seconds). Restoring output
to an isolated writable fixture directory, without changing mission source,
allows a successful repeat/save and the known Alpha=2/final elapsed=80 report;
disabled viewer histories still finalize. The source was not silently changed
to remove the failing operation. No full-suite repeat was made.

solver-disabled-stop-wayland-20261001.txt and
solver-disabled-failure-wayland-20261001.txt pass the two new variants on native
Wayland, for both DC and Yukon. Each has .Target.png/.Optimize.png captures.
The stopped Target and failed Optimize captures were inspected: full textured
Earth/starfield/constellations, Ground map, retained XY trial/accepted histories,
Latest controls and correct stopped/failed status are visible. Native replay,
close/reopen and output recovery pass. These are programmatic actual-widget
checks; fresh desktop input, top-level compositor minimize/restore and portal
choosers remain unqualified. They do not repeat or replace the earlier ordinary
All/Current/None or default-example renderer evidence.

build-solver-modes.txt records the real GmatQt-R2026a relink and GmatQt launcher
recreation. build-solver-modes-initial.txt retains the initial harness compile
error from an unavailable PlotCanvas::frame getter; no stale-binary test ran
after it. The harness now verifies replay by capturing and restoring Latest
pixels. audit-solver-modes.txt identifies source contracts and the two verified
application gaps. Every change is on codex/qt6-gui; Windows/macOS remain deferred.

The selected DC/Yukon mode controls and these disabled terminal histories have
bounded affirmative evidence. Full replacement still requires the remaining
plugin/workflow and desktop gates; more complex nested/mixed mode cases and
other plotting combinations remain under qualification.


## Optimizer and nested solver plot toggles — 2026-10-01

QtGui.OptimizerToggle and QtGui.NestedSolverToggle extend the existing actual
MDI Toggle workflow to two previously unqualified interactions. Both use Current
Orbit/Ground/XY plots, a plotted spacecraft, a 20-second pre-segment, 120 seconds
inside the solver and a 20-second post-segment. Pause/Resume Toggle initially
select only an idle ReportFile; the actual subscriber checklist selects exactly
Ground, Orb and XY. The resulting source differs only in those operands, with
labels, comments, expressions, solver settings and all other configuration exact.
Each Apply is pending until accepted and has exact single Undo/Redo, followed by
Unicode save/reopen and execution.

The Yukon fixture varies Alpha with the independently known quadratic optimum
2 and cost below 1e-10. It assigns spacecraft X=7000+100*Alpha before propagation.
The nested fixture uses the known outer Alpha=2 and inner Beta=3 goals, assigns
X and Y from them, and executes the complete inner solve in the disabled
50–80-second interval. All plots must omit samples strictly inside that interval
and retain a disconnected resumption. Accepted branch epoch is 140 seconds and
post-segment epoch is 160. The complete explicit iteration state/geodetic report
must remain byte identical to the independently configured ReportFile-only
Toggle source, despite the GUI change in subscribers. Known goals are checked
before and after the GUI transaction and accepted-history reference.

check-optimizer-toggle-initial.txt passes the first Yukon case in 0.68 seconds;
check-nested-solver-toggle-initial.txt passes the initial nested case in 0.66.
Those initial endpoints/suppression checks alone are not the final history
qualification. check-solver-toggle-accepted.txt passes both enhanced cases in
5.61 seconds. Current Ground and XY must have no trial-classified samples and
must match a separately run None-mode version of the exact plot-toggle source
sample for sample (1e-8 coordinates, 1e-10 days). That reference filters solver
trials instead of relying on Current-mode clearing, while keeping the toggles
and all numerical/report commands intact. Reports stay byte identical. Orbit
retains its established Current semantics; no invented trial-count requirement
was added. Its final camera target must track all three retained position
components. Replay must render earlier pixels and restore identical Latest
pixels; immediate close/reopen retains the same model and sample count.

check-solver-toggle-shared-harness.txt runs only the existing ordinary targeter
case after adding exact unrelated-source and replay assertions to its shared
harness, passing in 0.60 seconds. Earlier All/Current/None, solver mode matrices,
terminal-history suites and the full regression set were not repeated. No
implementation or numerical-engine change was needed for the new cases.
build-optimizer-toggle.txt records the focused builds and requests the actual
GmatQt target; the already current GmatQt-R2026a/launcher require no relink for
these test/documentation-only additions.

optimizer-toggle-wayland-20261001.txt and nested-toggle-wayland-20261001.txt pass
both enhanced cases on native Wayland, including the independent None reference.
Their full workspace .png and individual .Orb/.Ground/.XY.png captures preserve
Current results before that reference run. Both workspace captures were
inspected: full textured Earth/starfield/constellations, Ground maps, accepted
XY arcs with the disabled gap, retained On/Off command panels and completed
status are visible. Native replay/Latest, full camera target tracking and
immediate close/reopen pass. These programmatic actual-widget checks do not
qualify fresh desktop input, top-level compositor minimize/restore or portals.

The preserved optimizer .state.txt has 11 complete iteration/report rows and
SHA-256 778211eb61f44de0b219bbb9f8d60a7a0ac43c594a899e3cb1bbc528bc569b04.
Current retains Orbit 21 and Ground/XY 17 samples, each with one checked resumed
boundary. The nested .state.txt has 19 rows and SHA-256
4eef34b3b0d0cd58e343f6e62231600aee2e18c40085b53ec9c2855a9ee29163; all three
plots retain 17 samples with one resumed boundary. These are each fixture's
own independent reports, not equality between two different missions. The
native log asserts their equality to GUI-edited and None-reference runs.

The two bounded missing toggle cases now have affirmative evidence. Ephemeris
subscriber toggles, other mixed/deeper solver cases, unrelated plugin/workflow
requirements and original compositor/portal gates remain open. Windows/macOS
are deferred; full replacement qualification remains in progress.


## NonlinearConstraint controls and fixed-bound execution — 2026-10-01

The active wx NonlinearConstraintPanel has optimizer selection, left/right
operand text and single-parameter browsers, and read-only <=, >= and = choices.
Its SaveData accepts Real Number, Variable, Array Element and plottable
Parameter, then validates the command. The displayed tolerance control is
commented out; no active tolerance workflow was invented. Existing MissionTests
already checks the optimizer-only solver selector, so that test was not
reimplemented. audit-constraints.txt records these source contracts.

The initial QtGui.Constraints check executes an independent array-element
reference successfully but fails opening its actual MDI controls: the Qt form
regex excluded commas, including valid array index separators. The constraint
pattern now accepts array commas on either side and preserves source spans.
The previously free-text relation now has the three wx choices. Constraint
operand browsing uses NumericSingle: numeric properties/Variables, array
indices and finite real literals; String/UTCGregorian, whole arrays, out-of-range
literal elements and nan are rejected in the picker. Other callers retain their
existing modes. Free text still reaches ordinary transactional engine validation.

check-constraints-controls.txt finds a failed invalid-input assertion, refined
in check-constraints-validation.txt: engine interpretation accepts Grid(2,1)
for the declared Grid[1,2] and defers bounds to runtime. CommandForm now checks
both operands' literal indices before command Apply, matching wx. It returns an
inline range explanation and retains the pending panel and applied source.
Dynamic indices are left to the existing engine; no source regeneration or
numerical algorithm change was made. String/whole-array/missing-reference
rejections retain the complete previous script and model through the existing
rollback path. Correcting the first fixture then rerunning reproduces its
independent full report exactly. Invalid-input combinations run once in this
fixture, rather than for every relation case.

check-constraints-final.txt passes the first three numerical cases, then finds
that the independent 3 <= Value reference fails its known optimum, before any
GUI edit. The separate --dynamic-bound diagnostic in
check-constraints-dynamic-bound.txt confirms expected Value=3, actual=nan.
This is an unqualified engine regime, not a passing GUI round trip. The harness
retains that reproducible diagnostic; no regression gate accepts NaN and no
solver implementation was rewritten to make the fixture pass. Literal-left
GUI execution is instead qualified for a fixed array bound, 2 <= Grid(1,2).
Broader varying-right bounds and dynamic-index numerical semantics remain open.

The accepted five cases use Cost=(Value-2)^2 with one Yukon decision variable:
Grid(1,1) >= Grid(1,2) (bound 3, optimum 3); Value <= Limit (bound 1, optimum 1);
Sat.EarthMJ2000Eq.X = +1.5 (optimum 1.5); 2 <= Grid(1,2) (fixed bound 3,
optimum 2); and Value >= -1e0 (inactive bound, optimum 2). Mission assignments
keep Grid(1,1) and Sat.X equal to Value. Reports include Value, Cost, array and
frame-dependent property values at every trial and the accepted endpoint.
The independently known optimum/cost and four-column consistency are checked;
the complete GUI-edited report is byte identical to its separate script
reference. No numerical-engine equivalence is inferred for other regimes.

Actual retained MDI browsers exercise array indices and object/property/frame
selection, typed literals, Cancel, pending values, relation changes and Apply.
Only the three operand/relation spans may differ from the source; labels,
comments, resource settings, branch contents, reporting and implicit defaults
stay exact. Each case has one exact Undo/Redo and Unicode Save/Save As/reopen,
then executes the independently checked result. No full-suite repeat was made.
check-constraints-accepted.txt passes Constraints (3.26 s), Mission (5.43 s) and
ParameterSelection (5.18 s), 13.88 s total, after rebuilding those affected
shared-control targets. Earlier viewers, solver-mode matrices and plugin suites
were not repeated.

constraints-wayland-20261001.txt passes the same five cases and rollback/recovery
on native Wayland. Its .png and .picker.png were inspected: both operand controls,
relation dropdown, exact labeled command, retained Apply panel, numeric browser,
array selection, Help and completed status are visible. The captured first
case full report .state.txt has SHA-256
becae8cee1e6c225df0310340648709273bafc9eb46082898abcfc2ee54fd849.
These actual-widget checks do not qualify fresh desktop input, top-level
compositor minimize/restore or portal choosers.

build-constraints.txt and build-constraints-bounds.txt record actual
GmatQt-R2026a relinks and GmatQt launcher recreation after the two implementation
steps. build-constraints-accepted.txt rebuilds the final focused targets and
requests the already current actual application. Initial build/failure records
are preserved as diagnostic evidence, not final passes. All changes remain on
codex/qt6-gui. Windows/macOS are deferred and the full replacement goal remains
in progress against the original gates.


## Vary/Achieve/Minimize array operand workflows — 2026-10-01

The active wx Vary panel accepts array elements for the variable and initial,
perturbation, lower/upper, maximum step and scale fields. It checks edited literal
initial/bound ranges before saving; reference values remain engine expressions.
Achieve has active single-parameter browsers for goal, value and tolerance.
Minimize's objective browser accepts numeric parameter/Variable/array elements,
excluding literal numbers. audit-solver-operands.txt records the source contracts.

QtGui.SolverOperands first runs the independent array targeter reference but
fails the actual Vary panel: comma exclusion hides its Variable form field.
The Vary/Achieve/Minimize scalar capture now retains parenthesized array commas;
recognized options use the same capture, so commas in option array indices do
not split fields. Option order, surrounding spaces, labels, comments and all
unrelated source stay exact. Add-default-options behavior retains the existing
scalar Mission regression. Unsupported syntax remains in the text fallback.
No numerical engine or solver algorithm was changed.

Achieve now exposes its missing tolerance browser. Achieve goal/Minimize
objective use NumericReference, which selects numeric scalar references without
literal numbers or whole arrays. Achieve value/tolerance use NumericSingle,
which additionally permits finite real literals. Vary retains WritableReal.
Both new and existing caller restrictions are asserted in actual modal widgets.
The existing constraint literal-index validation now also checks Vary operands
and recognized option fields, Achieve goal/value/tolerance, and Minimize objective.
It rejects literal indices outside the current dimensions before Apply, retaining
pending input and the applied source/model; dynamic indices remain engine-owned.

check-solver-operands-controls.txt passes the targeter controls/round trip/report
and invalid-index recovery, then finds inverted literal Vary bounds accepted in
the Yukon panel. The new range preflight matches wx when initial/lower/upper fields
change: lower <= upper and any finite literal initial value within available
literal bounds. It does not evaluate reference-valued bounds or reject unchanged
legacy settings merely when another field changes. Errors appear inline without
committing source. The final fixture tests inverted bounds, an initial guess
above the upper bound, index correction and reference-result rerun.

The DC mission changes State(1,1) to State(1,2) through the Vary browser, initial
Params(1,1) to Params(1,5), Achieve goal to State(1,2), desired value to Goals(1,2)
and tolerance to Goals(1,3). Perturbation, lower/upper, maximum step and both
scale options are array references with retained nondefault option order.
The independently known final State(1,2) is 4. The Yukon mission selects the
same array decision variable/reference initial value, edits lower/upper to -5/8,
and selects CostGrid(1,2) instead of CostGrid(1,1) through Minimize. Its selected
cost is (State(1,2)-3)^2, with known optimum 3 and cost below 1e-10. Both retain
AdditiveScaleFactor=Params(1,6) (2) and MultiplicativeScaleFactor=Params(1,7) (3).
State(1,1) must remain zero; it is not silently varied instead.

Each actual MDI transaction has picker Cancel/pending/Apply, a retained clean
panel, one exact source Undo/Redo, Unicode Save and Save As/reopen, and execution.
Every complete trial/accepted four-column report must be byte identical to an
independently constructed script reference and meet the known goal/optimum.
Failed index/range edits preserve the complete applied source; correcting them
and rerunning reproduces the full report. No prior viewer or solver-mode matrix
was repeated. check-solver-operands-accepted.txt passes SolverOperands (0.84 s),
Mission (2.11 s) and Constraints (1.28 s), 4.23 s total. These are the affected
shared scalar/option/picker/preflight regressions, not a full-suite rerun.

solver-operands-wayland-20261001.txt passes both workflows and recovery on native
Wayland. The .Target.png, .Optimize.png and .tolerance.png were inspected:
Achieve array goal/value/tolerance and its browser, Minimize array objective,
retained exact command text, Help/Apply and completed status are visible. Vary's
actual native widgets are exercised by the harness but are behind the captured
active objective panel; these images do not claim a separate Vary layout capture.
This is programmatic actual-widget evidence; fresh input, top-level compositor
minimize/restore and portal chooser gates remain unqualified.

The preserved full .Target.state.txt report has SHA-256
27d1d10a003dd63eeaad25d8109f536e6a12d16ac2b82e2694cf5e4b68db6d07;
.Optimize.state.txt has
0c4897308aae6fe5c3b45f69d1baf4024683eef07f8fa7065bdd659bf328de5b.
build-solver-operands-controls.txt and build-solver-operands-accepted.txt record
the real GmatQt-R2026a relinks and GmatQt launcher recreation. Earlier failed
checks/builds are preserved as diagnostics. All changes remain on codex/qt6-gui;
Windows/macOS are deferred and original full qualification gates remain open.


## Global scope execution, insertion and clean command Apply — 2026-10-01

ManageObjectPanel's active callers in GmatMainFrame/MissionTree are Save and
Global. Its checklist includes configured objects, with automatic globals
omitted from new Global choices and retained for Save; empty selection is
rejected and accepted selections replace ObjectNames. The selected factory
GmatFunctionCommandFactory registers CallGmatFunction and Global. Neither the
core CommandFactory nor selected plugins register Clear, and MissionTree has
no active Clear caller. GlobalScopes verifies the live runtime contains Global
and ClearPlot but not Clear. The earlier synthetic Clear form/Cancel evidence
was not runtime qualification. Clear's absence is the selected-runtime
workflow disposition, not a reason to implement a new engine command. The
legacy text/form fallback remains; it is not offered as an insertion template.
audit-global-scopes.txt records these source/runtime distinctions.

QtGui.GlobalScopes uses an isolated ScopeTouch.gmf with a function-local Keep=123
and shared Counter/Grid/Text/Sat resources. Starting Counter=10, Grid(1,1)=1,
Grid(1,2)=7 and Sat.X=7000, calls with inputs 2 then 3 must produce Counter=12/15,
Grid(1,1)=5/11, unchanged Grid(1,2)=7, Text=shared and Sat.X=7002/7005. Main Keep
must stay 99 despite the function's same-named local variable. Returned values
are independently known 135/138; selected internal run objects must be global.
The complete two-row eight-column report from GUI-edited source is byte
identical to an independently constructed mission. The function file stays exact.

The actual MDI Global checklist selects/reorders Grid, Counter, Text and Sat,
replacing Decoy, with Cancel/pending/retained Apply, exact labels/comments and
unrelated source, one Undo/Redo and Unicode Save/Save As/reopen. The fixture then
uses the actual Mission context menu's Insert before action. Qt previously
omitted Global from its insertion templates despite the registered command.
check-global-scopes-fixture.txt reproduces that omission after the independent
reference and existing Global editor/report work pass. MainWindow now offers
Global only when the runtime registers it, with the existing object selector.
Insertion of a labeled/commented duplicate Global preserves all other source
and the same complete report, verifying repeated declaration idempotence.

check-global-scopes-activation.txt finds that another Apply on the retained clean
inserted panel adds a no-op transaction, so one Undo does not restore the prior
script. CommandEditor now returns before the backend when hasChanges is false.
This avoids that rebuild/transaction and its run-result invalidation. Dirty and
inserting panels retain validation/Apply behavior. The new fixture verifies
one exact insertion Undo/Redo after a second clean Apply, without duplicate
commands. Mission/Desktop exercise affected shared command/editor behavior.
No numerical-engine or resource-editor Apply behavior was changed.

Global deliberately resolves object names at execution; its Initialize only
marks found objects, and Execute reports missing objects. The initial assertion
in check-global-scopes-accepted.txt incorrectly expected MissingResource to fail
Apply. The corrected fixture retains that engine contract for the text fallback:
Apply accepts the structurally valid command, the run is Failed with a visible
MissingResource diagnostic and released controls, then selecting the correct
resources through the same panel and rerunning reproduces the full independent
report. It does not silently reject all dynamically provided names. The original
error expectation was a harness assumption, not a new Qt validation defect.

check-global-scopes-accepted.txt passes Mission (2.17 s) and Desktop (0.78 s) after
the final two product changes. It fails only the old missing-reference Apply
expectation. check-global-scopes-recovery.txt passes the corrected complete
GlobalScopes fixture (0.61 s). Only that changed harness was rebuilt/repeated;
the already passing shared regressions, existing function suites, viewer/mode
matrices and full suite were not rerun. build-global-scopes-accepted.txt records
the actual GmatQt-R2026a relink/launcher recreation; the recovery build also
requests the already current application.

Earlier diagnostic records are retained explicitly: the first harness compile
lacked QDialog's header (build-global-scopes-compile-error.txt), with no stale
binary test run. The initial function placed Create after Global, which enters
command mode; check-global-scopes-function-syntax.txt preserves the actual engine
log. Declarations were moved before BeginMissionSequence/Global, without engine
changes. The empty initial/diagnostic test errors also preceded flushing queued
message-window callbacks; the harness now drains them before reading errors.
check-global-scopes-controls.txt's unavailable checklist was a harness activation
error: setCurrentText does not emit QComboBox::textActivated; the check now drives
the actual template activation signal. Those records are not passing evidence.

The final global-scopes-wayland-20261001.txt passes the same workflow and recovery
on native Wayland. Its .png and .picker.png were inspected: ordered four-resource
checklist, retained exact Global command, Help/Apply and completed status are
visible. The .state.txt full reference report has SHA-256
5f5ae16aad970855ce3aacf2f8f4fa1041aa1ef703f8727d618bd48e4074271f.
The native log also retains a compositor popup-grab warning without a fresh
input serial. Menu selection here is programmatic Qt widget evidence; it does
not qualify fresh desktop popup input, top-level minimize/restore or portals.
No compositor policy was bypassed or repeated. Original Linux replacement gates
remain active; Windows/macOS are deferred.


## Ephemeris/viewer Toggle arcs and new-window placement — 2026-10-01

This increment follows the remaining Toggle/EphemerisFile output and native
workspace requirements. wx TogglePanel uses the full subscriber checklist for
Toggle (its XY-only and PenUp/PenDown variants are distinct callers). The selected
runtime EphemerisFile is a subscriber: Toggle activates it and invokes ToggleOn/
ToggleOff; an initially disabled writer is created on ToggleOn, and ToggleOff
closes an enabled output segment. These operations are already exposed by Qt's
ordered subscriber checklist and On/Off control.

GmatQtEphemerisToggleTests / QtGui.EphemerisToggle now exercises actual Mission/
MDI controls for Export, Orb, Ground and XY together. Every checklist rejects
empty acceptance, omits spacecraft Sat, accepts the ordered four outputs and
preserves Cancel/pending state. Each Apply retains the clean panel and changes
only the selected names, preserving the named command/comment and every unrelated
source byte. Exact Undo/Redo, Unicode Save/Save As/reopen and missing-subscriber
Apply rollback/checklist correction are covered. A repeat run replaces old output
without accumulating segments.

The bounded mission uses Earth point-mass gravity, a 7000 km equatorial circular
orbit, fixed 10-second integration, EarthMJ2000Eq output and explicit report
precision 17. It starts the initially disabled ephemeris, writes 0–120 s,
suppresses 120–240 s, resumes 240–360 s, then finishes Off through 420 s. The
independent script selects the same four outputs without GUI edits. Both
CCSDS-OEM and STK-TimePosVel (kilometers/event boundaries enabled) have 26 complete
epoch/six-state rows, no interior suppressed samples and exactly two enabled arcs.
Every output state agrees with the analytic circle (position tolerance 2e-5 km,
velocity 2e-8 km/s); all GUI output rows equal the independent script rows. The
four complete ReportFile states at 120/240/360/420 s remain byte identical. Text
outputs open through Output with their complete file contents and exact path.

Each Orbit/Ground/XY model retains 26 points with a separated resume boundary and
no samples in either disabled interior. Disabled Orbit/Ground are finalized at
completion; Orbit's complete final camera target matches its retained trajectory.
Earlier/Latest replay changes/restores pixels, all three viewers render and
close/reopen preserves their model/history. These checks add the mixed ephemeris
subscriber case; previous solver/optimizer/nested mode matrices were not repeated.

Initial results exposed two additional defects; the raw initial records remain:

- check-ephemeris-toggle-initial.txt passed the initial workflow (4.10 s), and
  ephemeris-toggle-wayland-20261001.* captured its native data/UI. The initial
  assertions did not qualify placement or actual STK boundary values. Capture
  inspection showed command controls outside the viewport. Adding the geometry
  assertion reproduced a 700×500 command panel at (545,163) in a 1004×601
  workspace; check-ephemeris-toggle-placement.txt fails that condition (0.38 s).
- check-ephemeris-toggle-stk-metadata-initial.txt extracts the original native
  STK header: its second SegmentBoundaryTimes value was about 120 s while the
  resumed data started at about 240 s. Matching an independently configured
  script did not establish that metadata's correctness.

MainWindow now fits only each newly opened command/resource/report window to the
workspace viewport after Qt chooses its cascade position. It preserves existing
window geometry; the new fixture checks resource opening does not move existing
viewers/reports, and command Apply plus resource/report bounds are accessible.
This does not assert arbitrary tiny-window layouts or alter manually moved
windows. The initial script-window and plot auto-placement policies remain.

STKEphemerisFile::WriteDataSegment now replaces a pending segment-boundary
placeholder with the first actual epoch when new segment data arrives. A gap
therefore records 240 s, rather than retaining the prior arc's 120 s end. The
assertion now checks actual 0/240 s STK boundaries and both OEM metadata blocks.
This is output metadata serialization; propagation, numerical algorithms and
state rows are unchanged. check-ephemeris-toggle-calculation-preservation.txt
compares initial/final captured data: all 26 serialized epoch/state rows in each
format and both four-row reports are byte identical across the fixes (OEM's
creation-date header is excluded from the state-row comparison). Both native
report SHA-256 values are
1e11c446e1337836e40593aee92fb1057e2ab4920f55eadc81fc5e86c9c7c431.

build-ephemeris-toggle-fixed.txt records the utility/frontend rebuild and actual
GmatQt-R2026a relink with GmatQt launcher recreation. The four affected suites
pass in check-ephemeris-toggle-fixed.txt: Mission 2.24 s, existing Ephemeris exports/
readback 3.94 s, new EphemerisToggle 1.76 s and Desktop 0.90 s (8.85 s total). The
existing Ephemeris checks are relevant to the shared STK writer and new output
window placement; Mission/Desktop cover shared panel behavior. No full-suite,
old solver/viewer matrix or unrelated plugin rerun was performed.

ephemeris-toggle-wayland-20261001-fixed.txt passes both final cases with the
placement/boundary assertions on native Wayland. Initial and final scene/control
captures were inspected: textured Earth, starfield, Ground map and separated XY
arcs render; the final command panels show all Help/Show script/Summary/Apply/Close
controls within the workspace. The final .oem/.e files and .state.txt reports are
preserved alongside the captures. This is programmatic actual-widget evidence;
it does not qualify fresh desktop input, top-level Wayland minimize/restore or
portal choosers. No compositor/input policy was bypassed. Binary/Code-500 Toggle
and solver-contained ephemeris cases remain under qualification, as do the
original Linux gates. Windows/macOS remain deferred.


## Solver-contained ephemeris and first-branch Ground Track initialization — 2026-10-01

This increment adds two bounded solver-contained cases to the actual MDI Toggle
workflow: DifferentialCorrector Target with CCSDS-OEM, and Yukon Optimize with
STK-TimePosVel. The solver is the first mission branch, with no preceding
Propagate. It selects Current Orbit/Ground/XY and All ReportFile iterations.
Alpha starts at 1 and reaches the known target/optimum 2; Yukon's quadratic Cost
is below 1e-10. Alpha deliberately does not alter spacecraft state. The existing
analytic circular-orbit checks therefore cover every state without expanding
this GUI task into algorithm qualification or a solver-by-format matrix.

GmatQtEphemerisToggleTests --solver / QtGui.SolverEphemerisToggle reuses the actual
ordered Export/Orb/Ground/XY checklist, empty-selection filtering, Cancel/pending/
retained Apply, exact comments/labels/Undo/Redo/Unicode Save/Save As/reopen,
missing-subscriber rollback/correction and repeat-run cleanup. Initially disabled
Export activates inside the solver, writes 0–120 s, suppresses 120–240 s,
resumes 240–360 s and ends disabled through 420 s. Each accepted-only ephemeris
contains 26 epoch/six-state rows and exactly two enabled arcs, with the two OEM
metadata blocks and actual 0/240 s STK event boundaries. Every state agrees with
the analytic circle and an independently configured script. The complete All
iteration reports contain 16 states for Target and 40 for Yukon; none are dropped
because display iterations are filtered. Output text and viewport placement,
all three rendered viewers, camera XYZ/frame, Earlier/Latest replay and retained
history through close/reopen are also checked.

The initial camera assertions in check-solver-ephemeris-toggle-initial.txt and
check-solver-ephemeris-toggle-camera.txt incorrectly compared the final camera
with the last filtered accepted Orbit point. OrbitView's existing Current
callback republishes the buffered solved pass with solving=true; the complete
retained history has 52 points, and camera/frame correctly follows its last
retained point. The harness now checks that endpoint and preserves all 52 points
through close/reopen. Those early failures do not establish a camera defect.

check-solver-ephemeris-toggle-filter.txt passes the corrected initial harness
(2.27 s), but only compares filtered accepted histories. The initial native
solver-ephemeris-toggle-wayland-20261001.txt exposes 104 Ground retained points
for Target and 260 for Yukon, despite 26 accepted points in each. The explicit
raw-count check fails in check-solver-ephemeris-toggle-retention-initial.txt.
That earlier passing subset check is not proof of Current trial cleanup.

GroundTrack::SetDataLabels previously named its curves only when propagation
started. SolverBranchCommand marks active subscriber breakpoints before the
first Propagate; the Qt Ground model then had no curves to anchor. Subsequent
ClearFromBreak could not discard those trials. GroundTrack::Initialize now sends
its existing Satellites metadata immediately after Reinitialize/ClearData,
creating named empty curves before the initial solver breakpoint. It publishes
no trajectory samples and changes no geodetic calculation, propagation,
solver algorithm or numerical Publisher/ReportFile filtering. The later
SetDataLabels action still supplies ordinary data labels/colors. The wx receiver
already handles this metadata and retains its existing color initialization.

The stronger fixture now requires Ground and XY Current retained counts to equal
all 26 accepted points. After correction/repeat-run, one independently configured
None-display run must match every accepted epoch, XYZ and connection boundary
for Orbit/Ground/XY, while the complete solver report and ephemeris remain
unchanged. This verifies cleanup of an initially empty history, rather than only
a filtered subset. Orbit's existing Current buffered-pass convention remains.

build-solver-ephemeris-toggle-fixed.txt records the core/subscriber and relevant
plugin rebuild, actual GmatQt-R2026a relink and GmatQt launcher recreation.
Four affected suites pass in check-solver-ephemeris-toggle-fixed.txt:
SolverToggle 0.71 s, SolverEphemerisToggle 2.37 s, NestedSolverPlots 1.47 s and
NestedSolverCleanup 1.03 s (5.58 s test total, 5.59 s wall time). These address
solver breakpoints and repeated/scoped initialization. No full suite, old complete
viewer/optimizer matrices or unrelated plugin/folder/document checks were rerun.

The final solver-ephemeris-toggle-wayland-20261001-fixed.txt passes both cases
with Ground/XY 26 retained accepted points and Orbit 26 accepted/52 retained.
Both Ground captures, Target Orbit, Yukon XY, both workspace captures and the
Target checklist were inspected. They show the accepted map arc, textured Earth/
starfield, separated XY arcs, ordered four-output selection and all command
Help/Show script/Summary/Apply/Close controls within the workspace. The native
.oem/.e files and complete .state.txt reports are retained. Calculation preservation
compares initial/final captured output in
check-solver-ephemeris-toggle-calculation-preservation.txt: all 26 serialized
accepted epoch/state rows in each format and the complete 16/40-row ReportFiles
are byte identical across the Ground initialization fix. OEM creation-date
headers are excluded from state-row comparison. Native report SHA-256 values:

- Target/OEM: ad5f5b2ed5d930089175d6b29b23fa50a077d3642d96bc3b53bcb26831804444.
- Yukon/STK: 284b6ecdb6f2ed9030811cc89a8341e68661e120dae2a371bee2fb3cf19276cd.

This is actual-widget/native rendering evidence. Fresh desktop input, top-level
Wayland minimize/restore and portal choosers remain unqualified; no compositor
policy was bypassed or repeated. Binary/Code-500 Toggle, nested solver ephemeris
and broader state-dependent solver output remain under qualification. The
original Linux acceptance gates remain active; Windows/macOS are deferred.


## Binary Toggle output, continuous Code-500 and reader limits — 2026-10-01

GmatQtEphemerisToggleTests --binary / QtGui.BinaryEphemerisToggle adds three
bounded actual-MDI cases: SPK, Code-500 LittleEndian and Code-500 BigEndian. The
Earth point-mass, 7000 km circular state and fixed 10-second propagation fixture
is unchanged. Initial Export.WriteEphemeris=false is activated by the actual
ordered Toggle subscriber selector; terminal Off at 360 s suppresses output
through 420 s. Every case covers empty-selection filtering, Cancel/pending/
retained Apply, named commands/comments and unrelated bytes, exact Undo/Redo,
Unicode Save/Save As/reopen, missing-subscriber rollback/correction and repeat-run
replacement without accumulated data. Command/resource/report windows remain
inside the workspace and opening a resource preserves existing window geometry.

SPK selects Export/Orb/Ground/XY for all four Toggle commands. Its coverage spans
are exactly 0–120 and 240–360 s. SpiceOrbitKernelReader queries all 26 fixed-step
states, including all four coverage endpoints; there is no coverage in the
disabled interior. The complete queried epoch/six-state rows match an independently
configured script and an analytic circular orbit. The kernel is unloaded before
repeat writing, avoiding a stale loaded file. libEphemPropagator then reads valid
states in each arc at absolute 60 and 300 s in separate configured missions.
Starting at 420 s is Failed, releases controls and has an ephemeris diagnostic;
Unicode saved-script reopen/rerun restores the second-arc report byte for byte.
The retained .reader.txt concatenates the two one-row reports, whose ElapsedSecs
is 60 relative to each mission's own start (0 and 240 s).

Code-500 is different. Its documented format supports one continuous fixed-step
block; EphemerisFile::Initialize explicitly sets allowMultipleSegments=false.
The passing cases select all four outputs for initial On/terminal Off, but only
Orb/Ground/XY for Pause/Resume. Export therefore remains enabled from 0 to 360 s.
Code500EphemerisFile decodes all 37 ten-second states, checking UTC, Earth and
coordinate-system indicator 4, the first/last epochs, complete six-state data
and the analytic circle. Both byte orders agree on every decoded state. The
Code500 propagator reads the complete three-state 60/180/300 s report correctly,
including the period when only viewers are disabled. This is qualification of
Code-500 activation/finalization with continuous output, not internal Toggle-gap
qualification.

All three viewers retain exactly 26 points over their two enabled arcs, with no
disabled interior samples or joined boundary. Orbit/Ground completion and full
camera XYZ/frame, rendered scenes, Earlier/Latest replay and close/reopen history
are covered. Binary Output opens its details window with the actual format, full
Unicode path and byte size; Copy path matches that path. Open folder is visible,
but was not invoked (no external desktop/file-manager routing claim).

Raw unsuccessful checks are retained and establish explicit limits:

- check-binary-ephemeris-toggle-initial.txt fails an additional SPK 5-second
  interpolation velocity check (about 1.8e-7 km/s error versus a 2e-8 threshold).
  check-binary-ephemeris-toggle-readback.txt also fails the existing binary
  readback threshold at 15 s (about 2.1e-7 versus 2e-7 km/s). The final fixture
  checks all 26 serialized ten-second nodes with the established binary readback
  tolerances of 2e-4 km and 2e-7 km/s. Extra between-node interpolation precision
  remains unqualified; no interpolation implementation or state generation was
  changed to make those assertions pass.
- check-binary-ephemeris-toggle-nodes.txt passes SPK's complete GUI/output case,
  then fails the independently configured Code-500 internal-gap reference before
  any GUI edits. Decoding its uniform-step slots finds a 240-second position
  (X=6767.0244744138172 km) at the slot labeled 110 s (analytic X at 110 s is
  6950.8426898279995 km). The utility accumulates states in one fixed-step data
  record while the resumed writer resets its requested epochs. A passing
  independent-script comparison alone would not make those times correct.
  That gap case remains unresolved; it was not rewritten as passing continuous
  output evidence or silently filled with invented disabled samples.
- build-binary-ephemeris-toggle-continuous.txt records a harness compile error
  from declaring QByteArray and row-list results in one auto declaration. It was
  corrected in the harness; no stale test executable was run after that build.
- check-binary-ephemeris-toggle-plugin.txt fails an incorrect expectation that
  traversing an internal SPK gap must fail. SPKPropagator explicitly uses a legacy
  skipEphemerisProp policy, while SpiceOrbitKernelReader returns sentinel states
  for failures inside the overall span and throws outside that span. The mission
  can complete through the hole. Gap-traversal states are not qualified by the
  positive within-arc readback cases; no numerical/reader policy rewrite was
  made. The final recovery test uses a genuine after-coverage failure.

The final build-binary-ephemeris-toggle-accepted.txt rebuilds the affected harness
and requests the already current GmatQt application. QtGui.BinaryEphemerisToggle
passes in check-binary-ephemeris-toggle-accepted.txt (3.10 s, 3.11 s wall time).
This increment changes no product code; the actual application already contains
the preceding rebuilt Ground initialization fix. No full suite or old complete
text/solver/viewer/plugin matrix was rerun.

binary-ephemeris-toggle-wayland-20261001.txt passes the three final cases once on
native Wayland using application/bin/gmat_startup_qt.txt. All three workspace
captures, SPK Orbit/Ground scenes, BigEndian XY/checklist, and SPK/BigEndian binary
details captures were inspected. They show accessible Help/Show script/Summary/
Apply/Close controls, the textured Earth/starfield, Ground map, separated XY arcs,
ordered four-output selector and binary paths/sizes/Copy path controls. The raw
.bsp/.eph, complete .decoded.txt, .state.txt, .reader.txt and after-coverage
.outside.txt evidence are retained. check-binary-ephemeris-toggle-report-preservation.txt
compares these artifacts without rerunning missions: all four-row ReportFiles
are byte identical to the previously qualified direct OEM report (SHA-256
1e11c446e1337836e40593aee92fb1057e2ab4920f55eadc81fc5e86c9c7c431), and all 37
decoded plus three reader rows agree byte for byte across Code-500 byte orders.

Native programmatic widgets/rendering do not qualify fresh desktop input,
top-level Wayland minimize/restore or portal choosers. No compositor policy was
bypassed or repeated. Code-500 internal gaps, SPK reader gap traversal, additional
interpolation precision, binary/nested solver output and broader numerical regimes
remain unqualified. The original Linux replacement goal stays active;
Windows/macOS remain deferred and MATLAB remains outside the selected runtime.


## Unavailable SPK states and retained viewer gaps — 2026-10-01

The preceding binary Toggle investigation exposed a viewer reliability defect:
SpiceOrbitKernelReader returns six finite -GmatRealConstants::REAL_MAX components
for an unavailable state inside its overall coverage span. The legacy
SPKPropagator skip policy can complete through an internal hole. A finite-only
plot check admitted these markers to XY/Orbit history and scripted primary or
alternate camera targets. GroundTrack could convert those raw markers into
misleading latitude/longitude; its all-absent return also failed to notify the
receiver to break the next segment. This increment contains unavailable display
data without changing the reader, propagator, numerical Publisher or ReportFile.

PlotModel rejects nonfinite and +/-maximum-double markers and marks the next
valid sample disconnected. Orbit positions also must fit the float vertices used
by OSG. QtPlotReceiver checks camera references before camera transforms and
checks final primary/alternate eye/target positions before retaining them; valid
available camera history remains usable. GroundTrack rejects marker components
before frame/geodetic conversion and sends absent NaN slots even when every
spacecraft is unavailable. The existing wx GroundTrackArea::AddData already
checks finite latitude/longitude per slot, so absent callbacks do not add wx
points. This is display containment, not corrected scientific gap propagation.

GmatQtInvalidPlotDataTests / QtGui.InvalidPlotData reuses the committed two-arc
SPK fixture from BinaryEphemerisToggle rather than regenerating binary output.
An independently configured mission with no viewers reports elapsed 60, 180 and
300 seconds. Its middle row contains all six -1.7976931348623157e308 markers;
that raw numerical failure evidence is retained. Before attempting a native
scene, the initial check-invalid-plot-data-initial.txt fails the direct model
assertion: unavailable finite reader data enters retained history. This is a
reproduced retention defect; no initial unsafe native scene or hang was claimed.

The same actual mission then configures Orbit, Ground and XY with a tracked
primary and named alternate camera. Exact Unicode Save/reopen retains source.
All three viewers omit the unavailable interval, recover their valid arcs with
no joining line and retain 20 points in the final native case. Both camera
histories contain usable coordinates; the final primary frame/target agrees with
the last valid spacecraft point. Rendered Earlier/Latest replay restores the
valid scene, switching named cameras changes pixels, close/reopen preserves
history, and a clean rerun neither accumulates points nor changes source/report.

build-invalid-plot-data-fixed.txt records the subscriber/frontend rebuild,
actual GmatQt-R2026a relink and GmatQt launcher recreation. Three affected checks
pass in check-invalid-plot-data-fixed.txt: InvalidPlotData 0.70 s, OrbitSetup
1.61 s and Plots 2.87 s, total 5.18 s. The latter two cover shared camera and
append behavior. No full suite or earlier binary/solver/plugin matrices were
rerun. audit-invalid-plot-data.txt records the source contracts and scope.

invalid-plot-data-wayland-20261001.txt passes once on native Wayland using the
actual application startup. All four captures (Orb, Ground, XY and workspace)
were inspected: textured Earth/starfield, accepted map arc, separated XY arcs,
and usable viewer controls/Completed status are visible. Its complete reference
and viewer reports are byte identical; static artifact comparison in
check-invalid-plot-data-report-preservation.txt verifies all three rows,
including the bad middle engine row. Both reports have SHA-256
c0333c4b173a56621bdd75b76ba8231dcafb444c5e23213def5a2ea850ce80e9.
The raw messages preserve the SPICE warning and existing deduplicated invalid/
unresolved camera fallback diagnostics. No numerical state or Completed status
was rewritten, and those reports do not qualify numerical SPK gap traversal.

This bounded viewer repair does not qualify all native stalls, arbitrary extreme
scientific ranges, fresh desktop input, top-level Wayland minimize/restore or
portal choosers. No compositor policy was bypassed or repeated. Code-500 internal
gaps, SPK numerical gap traversal and broader reader regimes retain their prior
limits. The full Linux replacement goal remains active; Windows/macOS remain
deferred and MATLAB remains outside the selected runtime.


## Complete-file report case and whole-word search — 2026-10-01

The shared report gate still lacked complete-file case/whole-word options;
page-local Find offered those options only within the displayed page. ReportViewer
now adds Match case (enabled by default) and Whole words to its complete-file
literal search. Changing an option cancels the scan and clears stale Next match;
Search file starts again at the beginning. Page-local Find remains independent.
The same ReportViewer component serves numerical reports and comparison output.
The active wx ReportFilePanel read-only/unwrapped Copy/Select All/Close contract
is retained; wx's Help button is disabled.

The scanner still reads one MiB per zero-interval Qt timer callback. It decodes
UTF-8-aligned bounded windows and compares Unicode text with Qt case semantics.
The original matched substring supplies the actual byte length and visible
selection, so a query such as k selecting the Kelvin sign does not lose byte
positions. Word context recognizes Unicode letters/numbers, connector
punctuation and combining marks, including supplementary characters. Tail
candidates wait for complete right context; Next starts with preceding context
and skips earlier byte offsets. Carry is bounded by four bytes per query UTF-16
unit plus 20 context bytes and up to three UTF-8 alignment bytes (the existing
query limit is 32768 units). There is no
whole-file buffer or numerical-engine change.

GmatQtReportSearchOptionTests / QtGui.ReportSearchOptions opens the actual Output
ReportViewer after a known two-row 2/5 engine calculation and Unicode mission
Save/reopen. check-report-search-options-initial.txt fails the missing controls.
The focused fixture then covers:

- Existing exact-case literal defaults; case-insensitive accented Unicode across
  a page/chunk boundary, original visible selection and the next match beyond
  18 MiB; distinct UTF-8 byte lengths (k/Kelvin sign), supplementary case pairs
  and CRLF-spanning literal selection.
- Whole-word exclusion of Orbiting at a chunk edge, underscores, adjacent Greek/
  supplementary letters and combining marks, with exact first/next valid word
  offsets and exhaustion feedback.
- Options/Stop/navigation cancellation, failed search when the file disappears,
  restoration/recovery and closing the viewer during an active scan. All complete
  large-file, engine report, editor source and saved-script bytes stay unchanged.

After the first implementation passed, a stronger overlap case reproduced a
false whole-word match: xedge's retained tail began with edge after the preceding
x had been discarded. check-report-search-options-overlap.txt preserves that
failure. The corrected window rejects a contextless first retained character
already evaluated in the preceding window and retains enough context for short
Unicode queries. Final checks also include embedded names at the old and final
overlap boundaries; this is an actual behavioral correction, not a relaxed
assertion.

check-report-search-options-fixed.txt passes Files (2.51 s) and the initial
ReportSearchOptions (1.87 s), total 4.39 s. Files covers existing UTF-8/CRLF page
partitioning, literal cross-page search/Next, paging beyond 16 MiB, Stop and
replacement/missing-file reload. After the final whole-word overlap correction,
only ReportSearchOptions is rerun and passes in
check-report-search-options-final.txt, 2.00 s. The broader Files check predates
that last small correction; its case-sensitive literal defaults remain outside
the added whole-word condition. No complete Qt/viewer/plugin/numerical matrix
was repeated. build-report-search-options-final.txt records actual
GmatQt-R2026a relink and GmatQt launcher recreation.

report-search-options-wayland-20261001.txt passes the final focused case once on
native Wayland. Both .report.png and workspace .png captures were inspected:
Match case/Whole words, complete-file controls, selected original Unicode result
on page 19, navigation and Completed status fit and remain visible. The full
.calculation.txt report is exactly two rows (2 and 5), byte identical before and
after all search operations; SHA-256 is b4c2d9b5e354b00d5f5840deec4dd744f79b8851f8ea1d97c3be47e932953ac5.
The harness also compares every byte of the large generated input and saved
source. audit-report-search-options.txt records the implementation/source
contracts and regression selection.

This closes the documented complete-file case/whole-word control gap. Regular
expressions, normalized/expanded case folds, arbitrary encodings, file replacement
during a scan and extreme record-count regimes are not claimed by these cases.
Native widget/rendering evidence does not qualify fresh compositor input,
top-level Wayland minimize/restore or portal choosers; those blocked experiments
were not repeated. The original full Linux replacement goal and other active
workflow/plugin gates remain open. Windows/macOS are deferred and MATLAB is off.


## Named OF segment cameras and reopened viewer placement — 2026-10-01

The converter previously rejected every dotted ViewFrame. The new
GmatQtSegmentCameraTests / QtGui.SegmentCameras first reproduced that failure in
check-segment-cameras-initial.txt, before any mission or scene assertions. The
initial harness compile failure (attempted mutation of a const shared model) is
retained in build-segment-cameras-initial.txt; it was corrected with a local model
copy before the reproduction build, without running a stale binary.

The local OpenFrames implementation resolves Object.NamedPropagate to the same
moving OFSegment reference frame for either ViewTrajectory mode. Its position
and attitude followers clamp to the segment endpoints; named lookup selects the
first matching arc. A default segment has no visible child geometry and therefore
uses View's one-unit bounding-radius fallback, rather than the spacecraft model
or whole trajectory bounds. Dotted LookAtFrame targets do not resolve through
OF's ordinary object lookup and remain explicitly rejected. The audit artifact
records the authoritative source paths and implementation contracts.

Qt conversion now preserves segmentFrame metadata for primary and named cameras,
using the base spacecraft in engine OrbitView fields. Build validates actual
Propagate provider identity and spacecraft membership (including formation
members). The receiver samples segment cameras after capturing provider,
position and body-to-view attitude. Ordinary cameras retain their previous
sampling order. The first contiguous retained matching arc supplies its latest
pose and then retains that endpoint through later arcs; cameras for a later
segment acquire no history before data arrives. Stored Current/Default pose,
body/inertial orientation, FOV and ordinary LookAt targets are retained.
Automatic segment framing uses the empty-frame unit bound; Fit still frames the
actual scene. Explicit primary resource camera edits clear only its segment
tracking and retain the named secondaries. No numerical-engine change is made.

The new bounded mission has a spinning spacecraft and two separately named
point-mass propagation arcs. Four converted views cover stored body-relative
FirstArc, automatic body-relative FirstArc, stored inertial SecondArc with
Earth LookAt, and automatic inertial SecondArc. The harness uses the actual
Build conversion offer, exact conversion Undo/Redo, Unicode Save/reopen,
selector/replay pixel changes, close/reopen history retention, primary override
and exact Undo, missing/wrong/unplotted references, and corrected-file rerun.
An independent mission without viewers supplies both complete seven-column
state rows at approximately 60 and 180 elapsed seconds. Every report byte is
identical before/after display and recovery. Nonzero attitude distinguishes the
first arc's orientation from the live later attitude. Formation membership is
Build-only evidence, not a formation runtime/numerical qualification.

check-segment-cameras-fixed.txt passes SegmentCameras (0.48 s), InvalidPlotData
(0.64 s) and OrbitSetup (1.53 s), total 2.65 s. These shared camera-reference and
unavailable-data checks were selected for the receiver changes. After stronger
primary-override and formation validation, check-segment-cameras-final.txt
passes the new case alone (1.24 s); that log predates the actual automatic-offer
and placement assertions. WorkflowTests' former unsupported-segment assertion
was updated to expect preserved metadata and its target compiled; the large
Workflow suite was not rerun.

A separate probe links the locally installed OpenFrames library only for
comparison; the application gains no OpenFrames dependency. It compares Qt's
world-to-view transform to actual OF View/trackball behavior for 24 new empty
segment-frame cases: stored/automatic, body/inertial, ordinary/LookAt and aspect
ratios 0.4/1/2.5. It deliberately supplies a misleading large spacecraft model
bound to check isolation. segment-camera-reference.txt passes with maximum point
error 2.23115e-07, below 1e-4 (OF's home-eye uses float storage). No previous
whole-trajectory/origin reference matrices were repeated.

The initial native Wayland case passed its camera/source/report assertions and
both scene captures rendered textured Earth, spacecraft, stars and
constellations. Inspection of the workspace capture nevertheless reproduced a
reopened viewer cascading off the right/bottom edge. Those initial artifacts
are retained under segment-cameras-wayland-20261001-initial; they are not evidence
that all workspace controls fit. QtPlotReceiver::show now constrains newly
created plot/table windows to the viewport after show; activating an existing
window does not move it. New harness geometry assertions cover reopened
containment and repeated activation preserving geometry.

build-segment-viewer-placement.txt records actual GmatQt-R2026a relink and GmatQt
launcher recreation. After this shared placement change,
check-segment-viewer-placement.txt passes SegmentCameras (1.80 s) and Plots
(6.68 s), total 8.49 s. The final focused native case was run once after that
change and passes in segment-cameras-wayland-20261001-fixed.txt. All three final
captures (.Stored.png, .AlignedLate.png and workspace .png) were inspected:
distinct textured scenes render and the reopened window, toolbar/replay controls
and Completed status are visible inside the workspace. This is native Qt
widget/rendering evidence; it does not imply fresh compositor input.

segment-camera-report-preservation.txt compares all four complete initial/final
independent/viewer reports. Both rows and every byte agree, SHA-256
6a09ace393866b3db884d1cd05adb21e8de3d1924783a4ba2f9a00bc196b0f12.
Raw numerical report spacing is preserved. The new source and evidence are
limited to this segment-camera gap and the reproduced reopen placement defect;
no complete Qt/plugin/numerical suite was repeated.

Repeated same-provider arcs in loops, solver/backward segments, retained-history
eviction replacing the first arc, propagation label decorations and tiny
viewports below widget minimum sizes remain unqualified. Fresh desktop input,
top-level Wayland minimize/restore and portal chooser gates retain their prior
limits; blocked experiments were not repeated. The full Linux replacement goal
remains active. Windows/macOS are deferred and MATLAB remains off.


## Independent OF object labels and trajectories — 2026-10-01

The next required OF display audit found two concrete gaps: DrawLabel arrays were
combined into one ShowLabels flag, and DrawTrajectory arrays were only retained
as comments. They could not preserve a trajectory-only spacecraft, a shown model
without a trajectory, or a label whose model is hidden. The local OF
SpacePointOptions defaults (DrawObject/DrawTrajectory/DrawLabel true),
OpenFramesInterface boolean setters and OFSpaceObject/OFSegment drawing code
were inspected; audit-converted-visibility.txt records the paths and semantics.
check-converted-visibility-initial.txt reproduces missing independent converted
metadata in 0.04 s before mission execution.

Conversion now records objectLabels/objectTrajectories in the existing Qt comment
metadata, keyed by plotted object name. It evaluates Add and boolean arrays in
source order: Add resets the unique ordered objects/defaults, available array
prefixes change only those objects, and empty/omitted suffixes retain defaults.
Extra valid flags beyond the object count are ignored as in the OF setter.
Bracketed true/false input is validated before conversion; malformed source stays
intact. No selected View is required to retain these flags, although that case
still carries the preexisting default-camera warning. JSON flag values and names
are checked; runtime validation requires an OrbitView and actual Add membership,
without treating CoordinateSystem as a pseudo-object for display flags.

DrawObject remains the standard engine body/model callback, independently of
trajectory flags. Receiver initialization assigns imported labels by name and
trajectory metadata overrides only the corresponding callback choice. Native
and CPU fallback label overlays honor the per-object choice even with a hidden
model. Ordinary curves without imported metadata keep their existing hidden-
object label behavior. Live visibility and the global Object labels master can
hide labels; label-only objects participate in Fit/legend. Source-preserving
Orbit setup edits retain choices on reordering and prune removed names during
Apply; exact Undo restores them. Primary camera edits and projection changes
retain the per-object maps. The numerical engine is unchanged.

GmatQtConvertedVisibilityTests / QtGui.ConvertedVisibility now exercises the
actual automatic Build conversion offer and Orbit setup Cancel, pending edit,
reorder and Apply controls, plus object removal, exact Undo, Unicode Save/reopen,
invalid array/metadata recovery and corrected rerun. Four explicit objects have
these independent settings:

| Object | Model/body | Trajectory | Label |
| --- | --- | --- | --- |
| A | Off | On | Off |
| B | On | Off | On |
| C | Off | Off | On |
| Earth | On | Off | Off |

The engine additionally supplies a hidden Sun for viewer bookkeeping. The
intermediate harness incorrectly required exactly four curves; its assertion
was corrected to verify the four explicit names and their unchanged distinct
flags. Another intermediate harness passed bracketed script syntax to the
resource-edit API; the final test instead operates actual Orbit setup controls.
Both failures are retained in check-converted-visibility-final.txt and
check-converted-visibility-object-names.txt. The first implementation build also
retains a harness-only inconsistent auto-declaration compile failure; its guard
prevented stale tests. These corrections did not relax the per-object drawing,
source or complete-report requirements.

The independent no-viewer mission propagates all three spacecraft for 120 s and
writes both complete 19-column rows (elapsed time plus all six Cartesian state
components per spacecraft). Converted execution and file/failure recovery retain
every report byte and all hidden-object samples. The final panel case passes
in check-converted-visibility-panel.txt, 0.70 s, using offscreen CPU rendering.
SegmentCameras passed its shared conversion/receiver check in
check-converted-visibility-final.txt, 0.49 s, before later harness corrections
and the final metadata-validation diagnostic/root-name tightening. It was not
repeated. Existing broad Workflow/viewer/plugin/numerical matrices were not run.

converted-visibility-wayland-20261001.txt passes the complete new case once on
native Wayland. All three .scene.png, .label-only.png and workspace .png captures
were inspected: textured Earth and B's model render, A's path remains, B/C labels
appear independently, Earth/A labels are absent, and the isolated hidden-model
label renders without a trajectory/model. The viewer toolbar/replay and Completed
status remain visible. Reference/viewer reports are byte identical, two rows of
19 columns, SHA-256 1273493f914d401d5510ac2f3421bbc0f6b4d8c54f7aa8a93d25ab3d6c8d13ab;
converted-visibility-report-preservation.txt preserves the static comparison.

After those complete cases, a stronger isolated fallback rendering assertion
reproduced an old visible label when the latest sample was behind the camera.
check-converted-visibility-clip-initial.txt retains that failure. Fallback now
selects the latest recorded sample first and then checks visibility, matching
the native overlay's existing order. The isolated fallback probe passes in
check-converted-visibility-clip-fixed.txt. The final native render-only probe
passes the same latest-pose clipping assertion in
check-converted-visibility-native-clip.txt. Neither probe reruns an engine mission.
The full native case/captures predate this small fallback-only correction and
final display-name validation tightening; their demonstrated native drawing
behavior is unchanged. build-converted-visibility-final-render.txt records actual
GmatQt-R2026a relink and GmatQt launcher recreation with the final product source.

A final source audit identified a related deletion path: resource deletion kept
its Qt directive, and automatic metadata retention could reinsert it. Display
membership validation would then reject the deleted viewer. Delete now removes
only that resource's directive and explicitly excludes it from metadata retention
for that transaction. Other directives remain unchanged, and normal Apply paths
retain their existing behavior. The same new focused case adds actual viewer
Delete, an independent no-viewer run with unchanged complete reports, and exact
source/metadata Undo followed by Build. check-converted-visibility-deletion.txt
passes the final whole new case in 0.76 s (0.77 s total), including fallback
clipping and final membership checks. build-converted-visibility-deletion.txt
records the final application relink and launcher recreation. Earlier native
captures/probes predate this deletion addition; native rendering code is unchanged
by it. No broad old suite or native mission was repeated after deletion.

This closes the independent label/trajectory conversion gap. Other OF object
axes/planes/grids, endpoint/center markers, velocity vectors, font/model overrides,
VR and time synchronization remain unqualified; persistent per-object label/
trajectory resource controls remain a script-metadata workflow. Native evidence
is Qt widget/rendering evidence; fresh desktop input, top-level Wayland minimize/
restore and portals retain their prior limits. Blocked experiments were not
repeated. The full Linux replacement goal remains active, Windows/macOS deferred,
and MATLAB off.


## Persistent per-object drawing controls — 2026-10-01

The previous appendix retained independent imported flags but left persistent
editing to script metadata. OrbitView resources now expose Object drawing… with
separate Trajectory and Label dropdowns for every pending Add object. Default
omits the named override: trajectories inherit the callback, labels inherit
shown-object behavior. On/Off retain an explicit choice. Imported choices load
by name; ordinary viewers keep implicit defaults on untouched OK. Standard
Orbit-view setup still controls body/model visibility, and the viewer's Object
labels control remains the master switch. The shared dialog Help route applies.

Dialog OK keeps edits pending; Cancel leaves prior pending choices intact. Apply
refreshes the existing MDI panel, retains camera metadata and changes only the
Qt directive for drawing-only edits. Drawing edits accepted before a later
pending object removal are pruned against the final Add list in the same Apply.
Typed API maps require named boolean values, actual Add membership and an
OrbitView target. Malformed maps, unknown objects and mixed invalid camera edits
roll back without losing source/pending choices. No numerical engine code changed.
audit-object-drawing-controls.txt records implementation and test scope.

The new --controls mode reuses GmatQtConvertedVisibilityTests without executing
the old conversion/viewer matrix. It reuses the complete committed independent
19-column reference from the previous appendix; no independent mission is rerun.
check-object-drawing-initial.txt reproduces the missing control before mission
execution in 0.17 s (0.18 s total). An intermediate harness passed shared_ptr
directly to require(bool), causing a compile failure retained in
build-object-drawing-fixed.txt; its guard prevented stale test execution. Explicit
bool conversion fixes the harness. check-object-drawing-controls.txt passes the
new ObjectDrawing case in 0.49 s and affected OrbitSetup in 1.55 s, 2.04 s total,
using offscreen CPU rendering. These checks predate the later ordinary-viewer
Build-only assertions and the final dialog column-width correction.

object-drawing-wayland-20261001.txt passes the complete controls case once on
native Wayland, including the later non-OrbitView rejection and ordinary-viewer
Build-only assertions. Actual MDI controls cover Cancel, unchanged OK, pending
OK/reopen, Default removal, mixed invalid Apply/correction, retained clean Apply,
paired object removal, exact Undo/Redo/rebuild, invalid typed map recovery and
Unicode Save/reopen/rerun. The ordinary viewer initializes Default choices,
untouched OK leaves source implicit, one label override adds only that named
map and Undo restores the exact source; this branch runs no mission.

The edited imported fixture displays A with its model/trajectory hidden and
label On; B with its model shown, trajectory Off and label Default; C with its
model/label hidden and trajectory On; Earth retains its shown body with labels
and trajectory Off. The native .scene.png and workspace .png were inspected:
textured Earth, B's model, A/B labels and C's path render, with accessible viewer
controls and Completed status. .controls.png preserves the initial native dialog
capture, which revealed the right column clipped after manual resizing. That
image is initial layout evidence, not proof of the final column layout.

Initial minimum widths were reduced to sensible content-sized defaults; header
handles remain adjustable. QtGui.ObjectDrawingLayout / --drawing-layout now
checks all three initial/restored columns fit, compact OK remains visible and a
manual width survives resize. check-object-drawing-layout.txt passes in 0.17 s.
object-drawing-layout-wayland-20261001.txt passes the same isolated native probe;
its .png was inspected with all headers, choices and dropdown arrows visible.
This final layout check initializes no engine and repeats no numerical mission.
The earlier full native workflow/captures predate only this column-width change;
source transactions and rendering are unchanged. The isolated dialog capture has
no parent Help manager; the actual earlier MDI dialog capture includes Help.

Both complete 19-column rows from the edited viewer and Unicode reopened run
match the reused independent reference byte for byte. The captured .report.txt
is 990 bytes, SHA-256
1273493f914d401d5510ac2f3421bbc0f6b4d8c54f7aa8a93d25ab3d6c8d13ab.
object-drawing-report-preservation.txt records the static comparison; it repeats
no mission. build-object-drawing-layout.txt records the final GmatQt-R2026a
relink and GmatQt launcher recreation. No broad old suite/plugin/numerical matrix
was repeated.

This closes persistent label/trajectory resource editing for imported and
ordinary OrbitViews. Other OF object decorations and broader solver/segment
regimes remain under the existing capability gates. Native evidence is Qt
widget/rendering evidence, not fresh desktop input. Top-level Wayland minimize/
restore and portal chooser gates retain their prior limits; blocked experiments
were not repeated. The full Linux replacement goal remains active. Windows/macOS
remain deferred and MATLAB off.


## OF center and endpoint markers — 2026-10-01

The next drawing audit found DrawCenterPoint, DrawEndPoints and DrawMarkerSize
still commented out during conversion. OF options default to independent center
and endpoint markers On and size 10; markers do not depend on model, path or
label visibility. OFSpaceObject owns a center MarkerArtist and OFSegment owns
START/END artists. Their supplied shaders use a hollow ring and a four-petal
rose. audit-converted-markers.txt records the inspected authoritative sources,
including GMAT's indexed unsigned-array parsing and OF's bounds check.

Conversion now retains named objectCenters/objectEndpoints/objectMarkerSizes in
the existing Qt directive, with source-ordered Add resets, supplied prefixes and
omitted defaults. Boolean overflow follows the existing prefix behavior; size
arrays reject excess indices as OF does. Negative, fractional and overflowing
sizes, malformed flags and invalid metadata retain original source/candidate
on rejection. JSON round trips preserve every unsigned 32-bit size. The primary
camera copy retains the maps together with labels/trajectories; camera edits,
source transactions, reordering/removal and Undo retain or prune all named maps.
No numerical engine code changed.

The persistent Object drawing dialog adds a Markers tab with independent
Center/Endpoints Default/On/Off and an unsigned pixel-size field. Blank size
inherits 10; ordinary Qt marker Default is Off, while imported OF defaults are
explicit choices. Invalid sizes keep the dialog open for correction. Cancel,
untouched OK, pending/reopen, retained clean Apply, named map validation, camera
retention, exact Undo/Redo/build and Unicode Save/reopen are exercised through
actual MDI controls. Body/model, paths, labels and global label master remain
independent. Removed objects lose marker and size metadata in the same Apply.

PlotModel selects endpoints for each retained disconnected or changing-provider
arc and the latest center through the replay frame. Markers do not create lines
across gaps or discard numerical history. Native OSG uses depth-tested pixel
point sprites with the ring/rose formulas; shaders are cached per scene. Empty
marker groups use reference-owned arrays, closing a source-review allocation
leak before delivery. CPU fallback paints matching glyphs and tests sphere-ray
occlusion so a center inside a shown body stays hidden. Both exclude latest
positions behind the camera instead of reusing older visible centers. Replay,
visibility, size and retained-history trimming have isolated rendering evidence.

The new --markers mode in GmatQtConvertedVisibilityTests / QtGui.ConvertedMarkers
uses the existing three-spacecraft 120 s mission and complete committed
independent 19-column reference. It repeats no old conversion matrix or reference
mission. Its initial 0.06 s guard caught the implementation's missing marker map
copy into the primary camera, before execution. check-converted-markers-initial.txt
also records affected ObjectDrawing passing 0.56 s and ObjectDrawingLayout passing
0.18 s; these were not repeated after later marker-only corrections. The corrected
case then reached removal with a harness input formatted as space-separated Add
names; the resource API requires commas. check-converted-markers-metadata.txt
retains that failure. The corrected full new case passes 0.73 s (0.74 total) in
check-converted-markers-removal.txt, including isolated offscreen fallback probes.

converted-markers-wayland-20261001.txt passes the new native case once. The scene
shows A's center ring with its body/path/label hidden, B's endpoint glyphs with
its body/path/label hidden, and textured Earth with neither marker choice. The
Markers control capture shows pending C center/end On, size 18 and B blank
Default size, with all four columns, Help/Cancel/OK visible. All six controls,
scene, workspace, isolated markers/replay/occlusion captures were inspected.
Isolated images show distinct arc endpoints, the center ring moving to the
replayed latest position, and a shown sphere occluding its own center glyph.
The native test also verifies close/reopen retains the model/history and edited
C marker choices reconstruct after file reopening.

Both complete report rows (elapsed plus all six Cartesian state components for
A/B/C) remain byte identical to the reused independent reference, before edits
and after Unicode Save/reopen/rerun. The captured report is 990 bytes, SHA-256
1273493f914d401d5510ac2f3421bbc0f6b4d8c54f7aa8a93d25ab3d6c8d13ab.
converted-markers-report-preservation.txt records a static full-byte comparison;
no mission is repeated for that audit.

The complete native case/captures predate the final empty-group lifetime and
shader-cache correction. check-converted-markers-lifetime-wayland.txt passes the
same isolated rendering assertions afterward, with no engine initialization or
mission. A final source audit distinguished indexed unsigned-size overflow from
boolean prefix behavior; conversion now rejects excess size indices. The pure
QtGui.MarkerMetadata / --marker-metadata probe passes in 0.06 s in
check-converted-markers-indexed.txt, covering empty/prefix/no-view defaults,
unsigned JSON maximum and excess/no-Add/overflow rejection without engine or
renderer initialization. The earlier native probes predate only this parser
restriction; rendering is unchanged. build-converted-markers-final.txt records
final actual GmatQt-R2026a relink and GmatQt launcher recreation. No broad old
suite/plugin/numerical matrix was repeated.

This covers center/end markers and normal pixel sizes, including imported and
persistent controls. Sizes 16/18/24/32 have rendered evidence; unsigned maximum
is metadata-only. Qt zero size disables glyphs; OF zero, very large sizes and
other GPU/device limits remain unqualified. Synthetic retained/provider arcs,
gaps and trimming do not qualify every repeated/solver/backward segment regime.
Object axes/planes/grids, velocity vectors, propagation labels, font/model
overrides, VR/time synchronization and broader Fit regimes retain their existing
limits. Native checks remain Qt widget/rendering evidence, not fresh desktop
input. Top-level Wayland minimize/restore and portal gates remain unchanged;
blocked experiments were not repeated. Full Linux qualification remains active,
Windows/macOS deferred, and MATLAB off.


## OF per-object line widths and native wide-stroke reliability — 2026-10-01

DrawLineWidth arrays were still comments during conversion, losing independent
fractional widths and OF's 2-pixel default. The authoritative OF real-vector
setter validates selected values >= 1.0, applies the supplied prefix, retains
omitted defaults and ignores excess entries. Add resets those defaults. The
segment passes the real width to its curve artist. audit-converted-widths.txt
records these sources and implementation locations.

Qt now stores objectLineWidths by name in the existing directive, with strict
finite real values >= 1 within the renderer's scalar range. Source-ordered
conversion retains empty/short/default/extra-prefix semantics and Add resets.
Malformed or unrenderable values retain the original source. Primary camera
metadata copies retain the map; receiver validation requires actual OrbitView
Add membership. Optional imported width overrides the existing Orbit width
without converting it to an integer; ordinary/XY width fields remain unchanged.
Removed overrides reset during receiver initialization.

Object drawing / Paths and labels adds a Width (px) field; blank means ordinary
Default 1 pixel. Imported OF defaults are explicit 2-pixel choices. Invalid
values leave the dialog open to correct. Cancel/untouched OK, pending/reopen,
retained clean Apply, exact Undo/Redo/build, Unicode Save/reopen, invalid map and
mixed camera rollback, other drawing/camera retention and named reorder/removal
are checked through actual MDI controls. The ordinary-viewer branch is Build-only:
untouched widths keep implicit source, one override changes only the directive
and Undo restores exact original text. No numerical engine code changed.

QtGui.ConvertedWidths / --widths reuses the previous complete independent
three-spacecraft state reference, with no reference mission or old conversion
matrix rerun. check-converted-widths-controls.txt passes the new case in 0.69 s
and the affected four-column ObjectDrawingLayout in 0.18 s, 0.87 s total, using
offscreen CPU rendering. These checks predate the later native-wide geometry
and fallback near-plane correction.

The once-run native controls/engine case then exposed actual stroke clamping in
its final isolated renderWidths assertion. converted-widths-wayland-20261001.txt
records that failed command; it is not an all-green native suite. The failure
occurs only after all preceding conversion, source, MDI, default/invalid Apply,
Undo/Redo, Unicode/reopened-report and object removal assertions completed.
Its controls/scene/workspace captures were inspected: all four columns and
Help/Cancel/OK fit, pending A=9.5/B=Default/C=3.25 are visible, and textured Earth
plus separate A/B/C paths render with accessible viewer controls. Those scene
captures predate the rendering correction and cannot prove final wide strokes.

converted-widths-render-wayland-initial.txt and .widths.png reproduce the clamp
without engine initialization: thin red 1.5 logical pixels covers 1,956 pixels,
while green 6.25 covers only 4,564 (seven physical rows at captured 2.25 DPR).
The native pixel count ratio requirement was retained. Imported strokes above
the conservative aliased/smooth GL width limit now use cached GLSL screen-space
quads with physical viewport sizing, per-end colors and original projected
depth. Homogeneous near/far clipping precedes division by w, avoiding inverted
screen directions through the camera. GPU clipping handles expanded viewport
edges rather than culling by the world-space centerline bound. Small/native
strokes retain their existing strips, avoiding blanket expanded-history cost.

The stronger isolated probe adds a blue sphere behind/in front of the stroke,
near-plane crossing and entirely behind-camera rejection. Two intermediate
harness assertions were corrected: fallback margins shift the plot center away
from the window center, and the correctly shaded blue sphere has green=82,
contradicting a raw green<80 threshold. The final test locates the actual crossing
and checks blue dominance. check-converted-widths-wide-final.txt,
check-converted-widths-depth.txt and converted-widths-depth-offscreen-initial
preserve those failures and the inspected correctly occluded blue-sphere image.
The fallback renderer also now clips crossing trajectory segments to the near
plane, retaining their visible part instead of dropping the connecting pair;
clipped-point ownership is stable and stored numerical histories are untouched.

Final QtGui.WidthRendering / --width-render passes offscreen in 0.21 s in
check-converted-widths-final.txt. converted-widths-render-wayland-final.txt passes
the isolated native case: thin remains 1,956 pixels and corrected thick becomes
9,324. Per-curve edits retain the other stroke, visibility/defaults work, a
foreground path covers the sphere, a background path is occluded, crossing paths
clip and wholly behind paths disappear. All final .widths/.depth/.front.png
captures were inspected. No engine/control/reference mission was repeated after
the renderer fix. The earlier native control assertions remain evidence for
unchanged source/control behavior; final isolated checks establish new rendering.

Both full 19-column rows before editing and after Unicode Save/reopen/rerun match
the reused independent reference byte for byte. The captured report is 990 bytes,
SHA-256 1273493f914d401d5510ac2f3421bbc0f6b4d8c54f7aa8a93d25ab3d6c8d13ab;
converted-widths-report-preservation.txt records a static full-byte comparison.
build-converted-widths-depth.txt records the final product GmatQt-R2026a relink
and launcher recreation. build-converted-widths-final.txt compiles the final
harness while the requested actual application is current. Broad old
viewer/marker/solver/plugin/numerical matrices were not repeated.

Normal 1.5/6.25 widths have native/fallback rendering evidence; metadata/control
values 1.25/3.25/3.75/4.75/9.5 and 1/2 defaults are covered. Very large widths,
extreme joins, zero-length projected segments and other GPUs remain unqualified.
Other OF object decorations, broader camera/solver regimes and Fit cases retain
their prior limits. Native evidence remains Qt widget/rendering evidence, not
fresh desktop input. Top-level Wayland minimize/restore and portal gates are
unchanged; blocked experiments were not repeated. Full Linux qualification
remains active, Windows/macOS deferred and MATLAB off.

## Stored OF label fonts and persistent Qt placement — 2026-10-01

DrawFontSize and DrawFontPosition were retained only as comments during Qt
conversion. The OF SpacePointOptions constructor stores defaults 14 and
Top-Right. The unsigned indexed setter validates the selected size index; empty
or short arrays leave omitted defaults. The string-array setter applies the
selected prefix and ignores excess entries. Scalar position assignments advance
a wrapping object counter; Add resets the stored per-object defaults, while the
counter's source order is retained. The wx editor offers sizes 1–100 and four
positions. audit-converted-fonts.txt records the authoritative sources.

The inspected OFSpaceObject/OFSegment and wx/Qt OFScene paths consume DrawLabel,
but do not consume DrawFontSize/DrawFontPosition or call label-size/placement
setters. These fields are stored editor intent, not demonstrated OF rendered
behavior. Qt now preserves and renders that intent; this work does not claim
pixel-for-pixel OF font equivalence.

Conversion retains named objectFontSizes/objectFontPositions alongside existing
camera/drawing metadata, including defaults, empty/prefix arrays, source-ordered
Add and scalar assignments. Unsupported positions, malformed arrays, indexed
size overflow and Qt-unrenderable sizes retain the exact original source with an
explanation. Strict typed JSON requires actual named integer/string values and
actual OrbitView Add membership. The current Qt size range is 0–10000 logical
pixels; 0 suppresses the label. Larger engine unsigned values remain unresolved,
not silently clamped. Primary camera copies, no-explicit-view conversion and
named pruning retain the settings. Receiver callbacks explicitly clear removed
imports, preventing reuse of an old size/placement.

Object drawing gains a separate Label style table, with size Default and four
position choices. Blank/Default keeps ordinary viewer behavior implicit. Edits
remain pending until parent Apply; invalid fractional sizes stay correctable in
the dialog. Cancel, untouched OK, pending reopening, retained clean Apply,
other camera/marker/path settings, exact Undo/Redo/build, paired invalid camera
rollback, unknown/typed-map rejection, named reorder/removal and Unicode
Save/reopen/rerun are checked through actual MDI controls. An ordinary Build-only
branch verifies untouched controls keep exact source and one explicit font
choice adds only the directive; Undo restores the original text.

Native and fallback overlays share OrbitLabels.hpp. Imported sizes use logical
pixel fonts, with QPainter applying DPR once. Four corner placements use measured
text bounds around the recorded object position. Painter save/restore keeps
caption/legend and other curves' fonts unchanged. Labels retain latest/replay
position, visibility and the live master switch; behind-camera labels do not
reuse an older pose. Ordinary fonts and their existing baseline placement remain
unchanged when imports are absent. No numerical engine code changed.

check-converted-fonts-initial.txt passes the new FontRendering in 0.19 s and
ConvertedFonts in 0.76 s (0.95 s total) offscreen. The new workflow reuses the
previous independent complete three-spacecraft report; no reference mission or
old conversion/marker/width matrix is repeated. ConvertedFonts executes the
initial translated mission and its edited Unicode recovery run, comparing both
full 19-column rows with the reused reference each time.

converted-fonts-wayland-20261001.txt passes the native --fonts case using the
actual application/bin/gmat_startup_qt.txt from application/bin. An initial
incorrect relative launcher path exited 127 before the executable ran; its raw
message is preserved in converted-fonts-native-launch-path.txt. The corrected
native workflow was executed once, including its isolated font probe. Native
12/32-pixel label-only glyph differences cover 557/3257 pixels; all four
placements, size-zero suppression, visibility/master, retained replay and
behind-camera rejection pass. The eight .controls/.scene/.png/.font12/.font32/
.top-left/.bottom-right/.bottom-left captures were inspected: all table columns
and Help/Cancel/OK fit, pending A=32/Top-Left and B=Default are visible, textured
Earth/paths and independently sized A/B labels render, and isolated glyphs move
to the requested corners. The workspace capture establishes viewer placement
and accessible controls; the scene/isolation captures establish label rendering.
These are programmatic Qt widget/rendering checks, not fresh desktop input.

A final audit corrected ignored excess position strings to match the OF prefix
setter. FontMetadata passes in 0.07 s in check-converted-fonts-metadata.txt,
without initializing an engine or renderer: zero/10000 metadata, no-view
conversion, typed JSON, ignored excess prefix, scalar counter wrap, Add default
reset and unsupported/no-Add rejection. Earlier control/native evidence predates
only this prefix parser correction; source/control/rendering behavior exercised
there is unchanged. No mission/control/native case was repeated afterward.
build-converted-fonts-final.txt records the final actual GmatQt-R2026a relink,
launcher recreation and rebuilt targeted harness.

converted-fonts-report-preservation.txt records a static full-byte comparison:
990 bytes, two 19-column rows, SHA-256
1273493f914d401d5510ac2f3421bbc0f6b4d8c54f7aa8a93d25ab3d6c8d13ab.
No broad old suite/plugin/numerical matrix was repeated. Metadata supports the
stated range; very large fonts, substitution/other platforms, edge clipping and
exact OF rendered pixels are unqualified. OF axes/planes/grids, velocity vectors,
propagation labels, model overrides and wider camera/solver/Fit regimes retain
their prior limits. Top-level Wayland minimize/restore and portal gates remain
unchanged; blocked experiments were not repeated. Full Linux qualification
remains active, Windows/macOS deferred and MATLAB off.


## Category-aware resource menus and desktop crash — 2026-10-01

The user requested a direct Add GroundStation operation when right-clicking the
Ground Stations category. The previous context menu always offered the shared
New resource action and opened the full creator with Spacecraft selected.

Category items now store their actual engine group independently of labels and
resource identifiers. Their menus expose Add <type> actions from the selected
runtime's viewable/creatable intersection, with explicit GroundStation and user
parameter mapping and Smoother's separately registered factory. Children inherit
their category; unavailable types are not offered. Root/empty-space and Edit
retain the generic creator. Category actions open a titled, single-type creator;
name validation, pending-panel/stale guards, transaction, source preservation and
editor activation use the existing creation path. The context popup event loop
finishes before the modal creator opens. Delete availability is retained.

The new QtGui.ResourceCategoryMenus / PluginCreationTests --context branch checks
eight categories (Ground Stations, Spacecraft, Burns, Variables/Arrays/Strings,
Solvers, Functions, Process Noise Models and Estimated Parameters), parameter
child inheritance and root generic behavior, matching labels and Delete state,
GroundStation/Array creators and Cancel, invalid/duplicate-name correction,
actual GroundStation creation/opened editor, exact unrelated source, Undo/Redo,
Unicode Save/reopen and unchanged global creator. No mission, old 18-type plugin
matrix or numerical report suite is repeated. check-resource-category-menu.txt
records the initial offscreen pass in 0.68 s.

A native --context attempt at 11:34 MDT coincided with a desktop logout requiring
the user to log in again. resource-category-menu-wayland-20261001.txt contains
Qt failed-grabbing-popup warnings, attached EGL proxy destruction and EGL surface
failure, followed by the harness's PASS text. That PASS is insufficient and is
not native desktop qualification. The collected menu/creator images are retained
as artifacts of the interrupted session; no successful native acceptance claim
is made from them.

Read-only host journal/coredump inspection confirms GNOME Shell PID15518 dumped
core with SIGSEGV at 2026-10-01 11:34:45 MDT. The session service reports
code=dumped/status=11/SEGV. The top main-thread trace reaches
wl_resource_post_event in libwayland-server through libmutter-18. The test log
mtime is 11:34:45.805. resource-category-menu-compositor-crash.txt retains the
main-thread backtrace and verified event metadata. This establishes a compositor
crash during the check, but does not prove the particular popup operation or
exact root cause. No compositor/driver/package configuration was changed.

All live-desktop testing stopped after the report. The --context harness now
refuses Wayland before creating a MainWindow or popup, so future automatic runs
cannot repeat this unsafe probe on that platform. The final application closes
the context popup loop before opening its creator, avoiding nested popup/modal
handling; this is not claimed as a verified compositor fix. The final focused
check-resource-category-menu-final.txt passes offscreen in 0.28 s (0.29 s total).
The earlier focused check was repeated only because the popup sequencing changed.
build-resource-category-menu-final.txt records final GmatQt-R2026a relink and
launcher recreation. The later harness comment clarification changes no behavior.

The requested category menu behavior is implemented and has focused interaction/
source evidence. Native popup safety/fresh desktop input and the compositor crash
remain unresolved; no further native rerun was attempted. Existing top-level
Wayland and portal gates remain open. Full Linux qualification remains active,
Windows/macOS deferred and MATLAB off.

## Debug icon and shared animation toolbar — 2026-10-01

The user requested a Debug icon distinct from Play and animation controls in the
same panel that control all displays together. Debug now uses a drawn bug icon
in the Standard toolbar and Run menu; its Ctrl+F5 shortcut and command remain.
The adjacent Animation group has Start, Play/Pause, Latest, a history slider,
position label and 0.25×–4× speed. Its single 30 ms timer drives Orbit, Ground
Track and XY retained histories. Position means the same percentage within each
plot's retained interval, not the same simulation epoch. Dynamic data tables
have current values without retained replay histories and are excluded.

The receiver retains shared position for closed MDI plots; reopening joins that
position. Global controls stop local timers to avoid competing playback clocks.
Individual plot actions or scrubbing release shared ownership, stop the master
timer and change its label to Local. Global Start/Latest also reclaim ownership
when their slider value is unchanged. Incoming data and history generation
resets resolve the retained interval at the shared position. Receiver clearing
stops animation and resets the toolbar; MainWindow shutdown stops the timer and
disconnects receiver callbacks before engine finalization. The controls remain
available independently of source-edit actions during a mission.

New QtGui.SharedPlayback / SharedPlaybackTests initializes the selected runtime
and creates synthetic histories through production receiver callbacks: Orbit
20 samples, Ground Track 40, XY 10 then 11, plus a live table. It verifies the
actual MainWindow toolbar and distinct Debug/Run icon pixels and shortcut,
empty/table-only disablement, start/scrub frame selection for unequal histories,
XY rendered history change, one shared timer and stopped local timers, speed
including fractional steps, pause/resume/end/restart, close/reopen at a new
shared position, local override and same-value Latest reclaim, incoming data,
cleared history, retained sample counts, clear/rerun state and default toolbar
fit. It refuses all non-offscreen platforms before constructing MainWindow.

check-shared-animation.txt records the initial focused pass through RunTest.py,
the same offscreen wrapper used by CTest. check-shared-animation-final.txt
records a second pass only after compacting toolbar layout; the final capture
shared-animation-toolbar-offscreen.png was inspected for icon distinction and
layout. This is offscreen widget/CPU evidence, not native OSG or desktop input
qualification. No mission, numerical report or old broad suite was repeated.
build-shared-animation.txt records the initial test-only compile error (Qt
requires Q_OBJECT for a typed findChild); the corrected harness locates the named
QWidget then dynamic_casts it. build-shared-animation-final.txt and final
build-shared-animation-layout.txt record successful frontend/test builds,
actual application/bin/GmatQt-R2026a relinks and GmatQt launcher recreation.

All live desktop testing remains stopped following the earlier GNOME Shell
crash. Native animation smoothness, multi-monitor/HiDPI toolbar behavior, theme
changes, common-epoch playback and table replay are not qualified by this work.
Top-level Wayland/portal and broader replacement gates remain open. The full
Linux goal remains active, Windows/macOS deferred and MATLAB off.

## Stars and constellations display switches — 2026-10-01

The user requested the ability to turn off stars and constellations in displays.
OrbitView's existing Display panel now exposes separate Stars and Constellations
checkboxes. They update the renderer's existing visibility flags and refresh the
viewer immediately. They retain live per-view choices across panel Close and
retained MDI close/reopen; other viewers, StarCount, cached catalogs, camera and
source settings are unchanged. Rebuilding/rerunning uses the script settings,
consistent with the panel's other live controls. Ground Track and XY plots have
no celestial sky and are unchanged.

The existing receiver startup catalog loading is shared with the new enable
path: a sky initially disabled can be enabled from the panel and resolve its
STAR_FILE or CONSTELLATION_FILE without rebuilding the mission. Disabling does
not discard the catalog; re-enabling uses the existing cache. Existing catalog
diagnostics and failure fallback are retained.

New QtGui.SkyControls / SkyControlsTests checks the actual MainWindow and display
panel with enabled/disabled initial states, independent toggles, cached re-enable,
lazy loading from the selected runtime paths, per-view isolation, MDI close/reopen,
StarCount/unrelated option preservation and exact editor text/modified-state
preservation. check-sky-controls.txt records the focused offscreen pass through
RunTest.py. The harness refuses non-offscreen platforms. The inspected
sky-controls-offscreen.png confirms both controls fit the panel. This checks UI,
receiver and model behavior; native celestial rendering was not retested.

build-sky-controls.txt records successful frontend/harness compilation, actual
application/bin/GmatQt-R2026a relink and GmatQt launcher recreation. No mission,
numerical report, shared-animation or old broad suite was repeated. All live
desktop testing remains stopped following the GNOME Shell crash. Native safety,
Wayland/portal and broader replacement gates remain open; full Linux qualification
remains active, Windows/macOS deferred and MATLAB off.

## Required references during resource creation — 2026-10-01

The user reported EphemerisFile creation failing with “The Spacecraft was not
set” and suspected similar failures in other types. The Qt creator prepended a
bare Create statement and rebuilt the entire candidate before opening the new
resource editor. Interpreter::FinalPassSubscribers requires EphemerisFile's
Spacecraft reference during that build, so the editor could never supply it.
The legacy wx subscriber creation path sets a default spacecraft first.

New resource now includes an EphemerisFile-only Spacecraft selector populated
from the currently built mission. The selected reference and Create declaration
are one undoable source edit; selection remains pending across type switches.
Direct creation callers use an existing spacecraft by default. No spacecraft or
a wrong-type/stale reference is rejected before applyModelScript, without model
reconstruction/source mutation. The user receives a clear instruction to create
a spacecraft first. Existing validation, rollback, tree synchronization and
opening the editor remain in the normal transaction path.

The new ResourceCreationDefaultsTests --probe checked the creator's 61 types
not covered by the prior 18-type plugin/user-parameter cases. Each was attempted
against the same built baseline with an explicitly named Vehicle spacecraft;
successful creation was undone/rebuilt and failure rollback was checked. No
missions ran. resource-creation-probe.txt records 59 successful builds and two
failures: EphemerisFile and Star. This is a creation/build check, not proof that
all default resources can execute without further configuration. The prior
plugin/user-parameter suites were not repeated.

An isolated --star probe and resource-creation-star-diagnostic.txt establish
that Create Star fails in Moderator::CreateCelestialBody/CelestialBody::SetUpBody
because its default central-body name is empty. This occurs before subsequent
script settings could repair it. Star is now excluded from generic and category
creation menus, preventing an operation the selected runtime cannot complete;
the existing Sun remains editable. No celestial/numerical engine change was made.
The initial probe's queued UI messages omitted that error; the retained diagnostic
was captured from the engine's application/output/GmatLog.txt after the isolated
probe. It is not a successful Star qualification claim.

The final QtGui.ResourceCreationDefaults check is focused on the changed cases:
EphemerisFile creation using an existing spacecraft, no-spacecraft/wrong-type
prevalidation, actual dialog error correction, pending type switches and Cancel,
explicit choice between two spacecraft, exact unrelated source, tree/editor
activation, Undo/Redo and Unicode Save/reopen. It also checks Star is no longer
offered and a direct unsupported request leaves the mission intact.
check-resource-creation-defaults.txt records the offscreen pass. The harness
refuses non-offscreen platforms. The successful 59-type diagnostic cases were
not repeated after the focused implementation, nor were old plugin, animation,
sky, numerical report or other broad matrices.

build-resource-creation-defaults.txt records successful final frontend/test
compilation, actual application/bin/GmatQt-R2026a relink and GmatQt launcher
recreation. All live desktop testing remains stopped following the GNOME Shell
crash. Native interaction/Wayland/portal and broader replacement gates remain
open; full Linux qualification remains active, Windows/macOS deferred and MATLAB
off.

## Unified resource forms and optional names — 2026-10-01

The user requested a primary configuration form instead of a property grid plus
separate setup dialog, and combining naming with configuration during creation.
They also requested automatic names when the Name field is left blank.

New resource now opens one larger dialog with optional Name, Type and the same
forms used for existing resources. The field displays an available type-based
suggestion; blank or whitespace input uses Moderator::GetNewName at Create.
Existing names are preserved, including when changing types. Ephemeris/report
default filenames follow the entered name until replaced with a custom filename.
The creator caches each type's pending form while switching types. Drafts come
from the runtime factories without registering objects in the mission. Cancel
leaves source and configured resources unchanged.

ResourceEditor embeds existing specialized forms for ephemeris output, thrusters,
impulsive burns, ground stations, OrbitView/GroundTrack/XYPlot, event locators,
dynamic data and tracking configurations. Spacecraft retain orbit/hardware as
the first page and embed mass, attitude and visual model forms under Setup.
Other supported properties use typed form fields. Advanced holds remaining
properties and auxiliary selectors; it hides fields already owned by the main
form. Focused cell/file/compound-selection dialogs remain available.

Create and Apply share clone validation and source-preserving transactions.
Creation includes changed pending values and required references in one edit;
it does not write untouched template defaults into the source. Array dimensions
and pending cell values stay synchronized, preserving retained cells and zeroing
new cells. Errors remain correctable in the same window. Existing primary forms
collect changed dependency groups; unrelated groups retain implicit defaults.
Successful Apply refreshes the snapshot, and the existing close/discard guard
handles pending inline edits.

New QtGui.UnifiedResources / UnifiedResourceTests checks fourteen unregistered
form drafts, main-form/Advanced separation, pending type switches and Cancel,
invalid creation and inline Apply rollback, configured ephemeris/tank/spacecraft/
string/array/planet creation, array resizing, name suggestions/collision avoidance and
explicit-name preservation, direct Apply/Close-discard, one-step Undo/Redo and
Unicode Save/reopen. check-unified-resources.txt records the final offscreen pass.
The harness refuses non-offscreen platforms. The inspected unified-resource-
create-offscreen.png and unified-resource-editor-offscreen.png show the actual
creator and inline panel. No numerical mission ran.

check-unified-resource-references.txt records the focused required-spacecraft
check adapted to the embedded selector: missing/wrong-type references, selection
between two spacecraft, exact unrelated source, tree/editor activation and
Undo/Redo/Unicode Save/reopen pass. The broad 59-type creation probe, plugin and
numerical matrices were not repeated. Parameter/Workflow fixtures were adapted
to the primary controls and compiled (build-unified-resource-fixtures.txt), but
their broader suites were not rerun; this is not a claim that the full historical
suite was executed against the changed forms.

build-unified-resources.txt records the final successful frontend/harness build,
actual application/bin/GmatQt-R2026a relink and GmatQt launcher recreation. All
live desktop testing remains stopped following the GNOME Shell crash. Native
interaction/Wayland/portal and broader replacement gates remain open; full Linux
qualification remains active, Windows/macOS deferred and MATLAB off.

## Subscriber window metadata visibility — 2026-10-01

The user identified UpperLeft, Size, RelativeZOrder and Maximized in the OrbitView
creator's Advanced table as inappropriate resource controls. Subscriber already
marks these fields, along with Minimized and TargetStatus, invisible through
IsParameterVisible. Qt resourceProperties was checking only read-only status.
It now honors the subscriber visibility metadata, removing these internal
layout/state entries from new-resource and existing-resource forms. Engine
setters, serialization and startup layout handling remain intact. Empty Advanced
property grids/filters are hidden, and wholly empty Advanced tabs are omitted;
legitimate Object drawing remains available for OrbitView.

QtGui.ResourceVisibility / ResourceVisibilityTests checks six output editors
(OrbitView, GroundTrack, XYPlot, ReportFile, EphemerisFile, DynamicDataDisplay),
three plot creator drafts, preserved Object drawing, and absence of empty grid
controls. A fixture with explicit layout values and Unicode comments retains
those exact assignments through a normal ShowPlot Apply, Undo/Redo and Unicode
Save/reopen. check-resource-visibility.txt records the focused offscreen pass;
the harness refuses non-offscreen platforms. The inspected resource-visibility-
create-offscreen.png and resource-visibility-editor-offscreen.png show the
cleaned-up Advanced page. No mission or previous broad suite was repeated.

build-resource-visibility.txt records successful frontend/test compilation and
actual application/bin/GmatQt-R2026a relink/launcher recreation. Live desktop
testing remains stopped following the GNOME Shell crash; native desktop and
broader Linux replacement gates remain open. Windows/macOS remain deferred and
MATLAB remains off.

## Advanced property audit and separate Qt checkout — 2026-10-01

The user's follow-up requested an audit of the remaining Advanced parameters.
An offscreen inspection enumerated 81 available resource factory types using the
selected Qt runtime, constructed unregistered drafts and inspected their actual
ResourceEditor property ownership. No types were skipped. There were 132
Advanced row instances representing 82 distinct parameter names. Every distinct
name has an exact guilabel match in the bundled Resource*.xml documentation.
resource-property-audit.tsv records the exposed primary/Advanced fields and
engine visibility/read-only flags; resource-property-audit.txt records the run
counts; resource-property-audit-documentation.tsv maps the Advanced names to the
bundled documentation. Matching a label was supplemented by reviewing the
hardware visibility overrides and documented uses of the suspicious fields.

No additional internal bookkeeping fields were identified in this scope.
Remaining Advanced controls include physical hardware origins, thruster
coefficients/mixtures, covariance, SPICE identifiers/kernels, station atmosphere
and tracking corrections, and ephemeris format settings. Hardware origin inputs
HWOriginInBCSX/Y/Z account for nine Advanced rows whose legacy visibility flag is
false. They specify the hardware frame origin in spacecraft body coordinates,
with meters as the documented unit, rather than GUI window position. Other
false-visibility fields in the primary forms include documented hardware
orientation and FieldOfView settings. A global visibility filter would remove
useful mission configuration, so the subscriber-only fix is retained. Engine
read-only exceptions were also reviewed against the existing specialized controls.

This audit covers factory-default forms for the available selected runtime;
it does not establish every conditional/configured form or execution validity.
It reused the existing compiled frontend archive whose Qt source matches the
new checkout, linked an inspection driver, and ran with the relocated new
checkout's runtime/startup. There was no resource registration, Apply, numerical
mission, live desktop session or repeated qualification matrix. No further
product code change or rebuild was necessary for this inspection.

The branch was pushed to the user's dcajacob/GMAT fork and cloned into
/home/dan/GIT/GMAT-Qt on codex/qt6-gui. The original checkout is retained. The
just-rebuilt application and selected runtime were copied without user settings
or mission outputs; runtime search paths in 22 ELF files were relocated. A fresh
Qt-only Linux CMake preset configured successfully, and the relocated application
completed isolated offscreen startup with its core libraries resolving inside
the new checkout. The fresh build directory has no copied objects and no full
engine rebuild was repeated. Qt6Project.md documents launch and future builds;
the ignored machine-local qt-runtime-snapshot.json records the copied source
commit and 20 selected native plugins. Creating this checkout does not itself
register a Codex sidebar project.

Native desktop qualification remains open after the GNOME Shell crash. All live
desktop testing remains stopped. Windows/macOS remain deferred and MATLAB off.

## Broad usability priority, retained attitude input and fresh Qt build — 2026-10-01

The user clarified that a broadly usable Qt GMAT GUI takes priority over
exhaustive display details at this stage. The handoff records that priority;
the full Linux scope is retained. The new checkout now has its own completed
Qt-only build of the engine, frontend and 20 selected native plugins. The first
894-step build succeeded (build-qt-project-first.txt); the later attitude fix
relinked the actual application/bin/GmatQt (build-pending-attitude.txt). This
supersedes the initial copied-runtime bootstrap, without modifying the original
checkout or enabling MATLAB/wx OF providers.

The first affected workflow run failed at ordinary created-resource execution
and stale-panel refresh/loading transitions; failure location varied under
diagnostics. The embedded ResourceForm was calling AttitudeDialog::settings
merely to inspect pending values. That method submits and clones the attitude
candidate and reads engine-backed fields. A retained form can outlive the model
references from its construction. The embedded reader now uses pendingValues,
which reads only its controls without normalization, cloning or engine reads.
Shared resource Apply still validates against a fresh current-model clone. The
standalone attitude dialog retains its conversion/validation behavior.

The broad QtGui.Workflow passes after the fix in 12.14 seconds, covering the
existing resource/mission editing, validation, pending/stale guards, creation/
deletion, propagation, execution interruption/recovery, source/Undo and reopened
report/viewer cases in that harness. QtGui.Parameters had already passed in
8.92 seconds and was not repeated. check-usability-initial.txt preserves the
initial failures and parameter pass; check-workflow-after-attitude.txt records
the later workflow pass alongside an intermediate new-fixture failure.

The expanded UnifiedResources regression keeps a clean Spinner-attitude form
across two interpreter rebuilds, detects/restores pending attitude input,
rejects stale Apply without changing source, and checks a fresh embedded Euler
angle Apply against the configured engine value with exact Undo/Redo/build.
Its existing fourteen draft types, configured creation, cancel/rollback and
Unicode round trips pass in the final check-retained-attitude-final.txt (0.78 s).
The first new fixture used the default attitude, which lacks writable angle
inputs; a later successful standalone Apply used MainWindow as its owner and
opened the normal unsaved-script close prompt, producing the retained offscreen
fixture timeout. Both fixture assumptions were corrected to use a writable
Spinner attitude and a separate owner widget. No native desktop hang or success
is inferred from that timeout. No numerical mission was added to this regression.

The in-progress display addition is also finished: OF DrawAxes prefix arrays
and Add/default resets now retain per-object flags rather than comments. Named
flags serialize, validate OrbitView Add membership and prune removed objects.
Object drawing / Body guides provides pending Default/On/Off controls. Native
geometry and the fallback use the retained body pose/radius; arrows follow
attitude and replay independently of models/paths, and the label master controls
captions. QtGui.ObjectAxes passes offscreen in 0.23 s, covering conversion,
typed maps, radius/rotation, hidden-model visibility, replay/behind-camera
handling and actual Cancel/pending/retained Apply/Undo/Redo/Unicode/pruning.
The initial driver used an older compiled frame fixture and was rebuilt after
its sequence correction. The axes implementation was unchanged by the later
attitude-reader fix. Native axes pixels, wider guide/camera cases and OF planes/
grids/velocity/other prior display limits remain open; no old display matrix or
reference mission was repeated.

qt-usability-results.txt summarizes these separate runs, not a full-suite pass.
The actual rebuilt executable completed isolated offscreen startup; the inspected
qt-project-built-startup-offscreen.png shows the shell, resources, script and
messages, with the debug icon and animation toolbar. Core libraries resolve
inside the new checkout. Live desktop testing remains stopped after the GNOME
Shell crash; native desktop/portal and full replacement gates remain open.
Windows/macOS remain deferred and the numerical engine was not rewritten.

## Complete shipped mission inventory and broad capability fixes — 2026-10-02

Every standalone mission candidate in the selected distribution now has a build
attempt: 138 primary samples, 19 help tutorials and 7 API/TLE samples. The current
ledger has 127 build successes and 120 completed execution successes. It retains
37 failed builds, five missing-catalog TLE runtime failures, the L2 tutorial's
explicit failed-bracket Stop and the first Yukon timeout. A longer Yukon attempt
is actively progressing in its separate ledger; it is not counted as passed.
See Qt6ParityValidation/shipped-examples-20261002.md and its complete manifest,
177-row source/helper table, current diagnostics and tutorial repair map.

The failures drove actual fixes: script-directory separators for ephemeris assets
and saved-copy folder runs; OF declaration/list semantics and unplotted body
cameras; retained Relative Position/Body-Fixed vectors in native and fallback
renderers; the public Python API/development initializer/portable callback;
and the Qt-branch interpreter string-array metadata regression exposed by SRP
ExtraShadowBodies. The actual application and affected core/plugins are rebuilt.
All three regular vector examples now build and execute. Native vector pixels
are not established by the offscreen arrow/report/replay check.

Coordinate drafts and nested Axes now share one pending outer Create/Apply
transaction. Mission menus append inside five branch types and offer Toggle
creation. Separate focused checks pass for MissionInsertion, ScriptAssets,
UnplottedBodyCameras, OpenFramesSyntax, CoordinateCreation, OpenFramesVectors and
StringArrayDispatch. They cover source/Undo/Unicode/retained Apply, independent
state reports, replay after engine replacement and empty-list semantics. The
coordinate test's first failure was a source-layout fixture assumption; the
comparison now strips only exact newly introduced coordinate statements and
compares every other original byte. This is not a full-suite or native pass.

Three repaired tutorial missions complete; the fourth builds and reaches its
documented bracket guard with unchanged numerical guesses. The formation
attachment is a duplicate lunar mission and still requires correct formation
content. Dependencies, missing public TLE inputs, unparented helpers and early
provenance limits remain explicit. Full raw completed outputs/logs/screenshots
are retained in the new checkout's ignored build/example-qualification/20261002.
Passing example stages and unrelated qualification matrices were not repeated.

The complete Linux replacement goal remains active. Native window/OpenGL/portal
and outstanding wx workflow/display acceptance gates remain open after the GNOME
Shell crash; live desktop testing stays stopped. Windows/macOS and MATLAB remain
deferred. No numerical algorithm was rewritten.


## External optimizer, tutorial repairs and resource Clone — 2026-10-02

The latest shipped corpus remains 164 standalone missions/tutorials, plus four
include fragments and nine GMF helpers. Actual current stages are 147 successful
builds, 17 failed builds, 136 completed executions, six unexpected runtime failures,
two expected tutorial Stop checkpoints and three timed-out runs. All 311 current
child records retain unchanged main-source hashes. Raw runtime statuses remain
136 passed/eight failed/three timed out: the two Step 1 Stops are explicit assessed
interruptions, not completed missions. See shipped-examples-20261002.md and its
manifest/177-row table for every file, history and exact artifact.

The free external R2026a VF13ad binary now loads against this engine and all 19
previously blocked sample/tutorial variants build. Fifteen complete, two reach
the documented initial-guess Stop, and two long Mars analyses hit 300-second
bounds. Original primary/Help variants differ in source hashes and each was
executed. The locally installed binary/README is ignored and not committed;
GMAT_QT_EXTERNAL_PLUGINS retains optional extensionless startup paths across
rebuilds, with no external plugin target added to the build. New runner records
snapshot all configured Linux plugin files; presence alone is not ABI proof.

FormationRendezvous's wrong lunar attachment is replaced by a clearly marked
four-spacecraft reconstruction preserving documented forces, burn bounds,
timings and separate position/velocity target goals. Missing historic Sat2–4
states are disclosed. Actual targets converge in three/two iterations; every
constrained residual meets 1e-4 km or km/s per component. The final residual file
is independent of trial Cartesian report rows. This is an executable teaching
demonstration, not restored historic trajectories or simultaneous docking.

L2 now has a measured sign-changing 10000/16000 km bracket with unchanged energy,
guard and calculations. Its full repaired run still fails 79.261 seconds later:
a 15109.375 km intermediate trial misses the geometric L2-entry box, propagates
for decades and exceeds DE405 coverage. One bounded diagnostic identifies an
Earth-return trial; simply adding a cap would alter the transfer objective.
Final targeting never starts. This remains a numerical example failure, not a
proprietary-dependency waiver. No exhaustive branch search or new retry follows.

The seventeen failed builds now comprise deferred MATLAB (4), SNOPT (3),
SNOPT-dependent CSALT (9) and absent legacy MarsGRAM plugin/data (1). Five TLE
runs still need exact matching absent public catalogs. Full dependency/OF
execution and data gates are retained. Native desktop/OpenGL/portal testing is
still stopped after the GNOME Shell crash; these offscreen results do not close
that gate. The existing long Yukon process reached its 7200-second bound after 11
of 20 windows. Its terminal timeout/history was merged by matching source hash;
no unchanged restart is scheduled. SPAD's earlier 209.91-second pass was not repeated.

Configured resources now offer Clone in Edit/Resources with optional naming and
one shared editor. Source-based cloning preserves grouped settings/comments,
references, implicit defaults, managed Array formulas and Qt viewer metadata;
Cancel/correction and commit/Undo/Redo/save/reopen remain atomic. Independent
review caught/fixed ambiguous first-command boundaries and omitted managed Array
blocks. Protected built-ins/bodies/PropSetup match wx exclusions. Resource Rename
is still a missing active workflow, not closed by Clone.

GmatQt and affected check executables were incrementally rebuilt. Final Clone
passes 0.47 seconds, OpenFramesSyntax 0.14 and OpenFramesVectors 0.46; duplicate
trailing OF terminators now convert without changing original lines or mission
text, including static IntegratedFlyby. Its CSALT execution remains unavailable.
Initial Clone compile/fixture failures were harness defects and their raw logs
are preserved alongside the corrected test. No successful conversion check or
old complete suite was repeated after fixture-only fixes. The full replacement
acceptance checklist remains in progress; no native popup or scientific-wide
qualification is claimed. Windows/macOS and MATLAB remain deferred.


## Resource Rename and terminal Yukon checkpoint — 2026-10-02

Rename now provides the active local resource workflow through Edit/Resources
and F2. `resource-rename-20261002.md` records source/reference coverage, pending
and rollback guards, exact Undo/Redo/Unicode reopening, wx PropSetup/FM behavior,
managed Array/Qt metadata preservation and the independent 7/3.5 report check.
The actual application was rebuilt; the focused offscreen check passes in 0.44
seconds. Includes/external GMFs and unsupported tracking row syntax remain
explicit script-editor boundaries. Tracking initialization is not measurement
estimation qualification. Initial helper and fixture failures are preserved.
No earlier passing example/UI matrices were repeated.

The resource context menu is destroyed before modal dispatch using copied value
selection. Its offscreen lifetime assertion passes; the read-only Qt source and
crash review establishes no demonstrated prior surviving grab or GMAT root
cause. This is not native compositor acceptance. Live desktop testing remains
stopped, and native windows/OpenGL/portal and other explicit acceptance limits
remain open. Windows/macOS and MATLAB remain deferred.

Yukon's one existing extended attempt ended at 7200.026 seconds after 11 of 20
launch windows; the next window was still optimizing. Only its terminal run and
history were merged after matching source SHA256. Completed raw results are
preserved under `build/example-qualification/20261002/yukon`. No unchanged restart
is scheduled. Corpus totals remain 147 successful builds/17 failures and 136
completed runs/six unexpected runtime failures/two intended Stops/three timeouts.
No full mission or numerical scientific success is inferred from partial windows.
The complete replacement goal remains active and unfinished.

## Added acceptance phase: build the Help tutorials through the GUI — 2026-10-02

The user requested a subsequent phase after the current qualification work:
follow the Help instructions as a person using the Qt application would and
independently construct, run, inspect and save/reopen each scenario. Shipped
tutorial mission scripts are reserved for a final comparison after preserving
the authored mission and evidence. They must not seed its construction.

[Qt6TutorialWalkthrough.md](Qt6TutorialWalkthrough.md) records the ordered twelve
published chapters and twelve additional/legacy source chapters, prerequisite
chains, stage handling, dependencies and the per-chapter acceptance procedure.
All walkthroughs are pending; reading the introductions and the earlier shipped
script corpus runs establish no interactive tutorial pass. Record missing or
confusing controls and fix reasonable GUI gaps, distinguishing prescribed
script/function-editor work from resource/mission editor coverage. Preserve
existing successful execution evidence instead of repeating unchanged tests.

Live desktop testing remains stopped after the compositor crash. Establish a
safe interactive display route for the walkthroughs; synthetic model fixtures
cannot replace human-style GUI construction or native window qualification.
Windows/macOS and MATLAB remain deferred. The full goal remains unfinished.


## Actual isolated X11 input, per-object guides and focus recovery — 2026-10-02

[Isolated input evidence](Qt6ParityValidation/isolated-x11-input-20261002.md)
records actual XTest input to the rebuilt application on authenticated private
Xvfb/Openbox/software GL. It establishes new-mission textured display execution,
one main minimize/restore and viewer close/reopen cycle, typed GroundStation
creation/Cancel/automatic naming/Save, and a deliberately long fixture's pause/
stop. The failed Ctrl+Z and still-running rerun are retained as failures.
Host GNOME Shell retained its original PID/start time after all sessions;
no root cause or fix for its earlier crash is established. Host GNOME/Wayland,
portal/hardware and wider repeated lifecycle gates remain open.

MainWindow now routes non-text Undo/Redo to the selected script, while focused
fields and selected inactive documents retain their own history. Modal controls
cannot undo source behind them; an explicit ownership chain recognizes fields
in owned dialog windows. The final QtGui.FocusUndo passes 0.28 seconds after
rebuilding the actual application. First Redo binding and two actual modal
boundary failures remain preserved with corrected dispositions. A separate
actual Mission-tree Ctrl+Z restores the complete GUI-authored saved script byte
for byte, then completes its mission in 0.563 seconds and saves the same source.
Native plot/modal input remains distinct from the focused offscreen cases.

[Object-guide evidence](Qt6ParityValidation/object-guides-20261002.md) adds
independent per-object DrawGrid/DrawXYPlane conversion and pending Body guides
controls, named source retention/rename/pruning and recorded pose geometry shared
by native/fallback/replay. QtGui.ObjectGuides passes 0.31 seconds. Actual native
X11 pixels show Earth's spherical grid and translucent 15R radial disk after
GUI Apply, camera drag and Fit. The saved global XYPlane stays On while Grid is
Off; this is not a native global-plane-Off isolation claim. Mesh/solver/camera
regimes and unsupported velocity/other decoration limits remain open.

Only newly affected focused checks were run. GmatQt is rebuilt, with final
SHA256 9bc8115f4200bc4c933b72de2383cfe7722d79c463d148a64c029238b460ea8d.
Durable raw evidence is in build/example-qualification/20261002/isolated-x11.
All interactive Help tutorial walkthroughs remain pending. This bounded route
supports future independent construction; it does not close full replacement
or native desktop acceptance. Windows/macOS and MATLAB remain deferred.


## Preview lifetime, velocity and command naming checkpoint — 2026-10-02

[Viewer/shared evidence](Qt6ParityValidation/viewer-preview-shared-20261002.md)
records a demonstrated runtime reliability defect: destruction of same-name
OrbitView settings/validation clones cleared the live display. Frontend preview
ownership now suppresses only that synchronous ClearObjects; genuine engine
cleanup remains active. QtGui.PreviewLifetime passes 0.89 seconds, including
exact retained histories/cameras and ordinary cleanup. Actual private X11 input
then establishes clean settings close plus two actual viewer deletion/Output
reopens at shared 47.6%, rotated camera retention and three main minimize/restores.
The original failure remains preserved. This is stronger bounded X11 lifecycle
evidence, not host GNOME/Wayland or hardware acceptance.

Actual GUI creation adds XYPlot to Orbit/Ground: all three scrub together,
Start/Play/Pause stops at27% with identical captures after 500 ms, per-Orbit Local
Start and global Latest work. A retained XY settings activation clips controls;
the focused workspace correction and retry are recorded separately. Capture
labels containing intended actions are not assumed outcomes: the early LocalXY
attempts moved global controls, several SaveAs captures still contain the dialog,
and the final velocity-replay attempt shows Latest. Their precise limitations
are recorded alongside successful cases.

[Velocity evidence](Qt6ParityValidation/object-velocity-20261002.md) adds retained
DrawVelocity conversion and pending per-object controls, real view-frame state
capture, independent fixed1000-second line geometry, native/fallback rendering,
replay/trim/Fit/source/rename preservation. QtGui.ObjectVelocity passes 0.85 seconds;
the independently affected ObjectGuides check passes 0.34 seconds. Actual private
X11 controls Apply/run and Output reopen show native velocity at Latest. The
rotating-frame comparison fixture sets NutationUpdateInterval=0 to align plot and
Report calculation paths; no production numerics or tolerances changed. Relative
Velocity/ThrustVector remain explicit unsupported converted modes. Wider solver,
mesh and camera regimes remain open.

[Name/Write evidence](Qt6ParityValidation/mission-label-write-20261002.md) adds
pending optional command names with exact prefix-only branch/source edits and
runtime-gated Write insertion/ordered parameter selection. Full option syntax
and SaveMission's filename remain explicit source boundaries. The first check
exposed a real existing named-assignment serialization defect with the GMAT
keyword off; a narrow11-line core fix retains that label without changing
numerical execution. QtGui.MissionLabelsWrite passes 0.89 seconds, including direct
OFF/ON/OFF/repeated serialization and an actual report 9/2.5. Initial compile,
fixture and product failures are retained with their final dispositions.

No earlier successful full UI/example matrix was repeated. Core identity is
recorded separately from the application: a shared-library-only fix does not
change the executable hash. Durable full sessions/check logs remain in
build/example-qualification/20261002. Read-only focused peer review found no
additional substantive preview/velocity/command defects. All Help walkthroughs
remain Pending: these qualification fixtures are not tutorial constructions.
Full Linux replacement acceptance remains unfinished; Windows/macOS/MATLAB
remain deferred.


[Workspace/native retry](Qt6ParityValidation/workspace-reachability-20261002.md)
resolves the clipped retained XY settings case. It also corrects an inherited
script-maximized state restoring a new form's unclamped original geometry.
New settings normalize before placement; explicit activation clamps the selected
normal retained panel with settled layouts and unchanged pending/source/report/
other-window state. QtGui.WorkspaceReachability passes 0.34 seconds. Actual input
opens settings before a 0.553-second mission, deliberately displaces its pending
panel, and restores full controls/edit through Window and Resources. Actual Close
reaches the discard guard. Fresh post-Discard pixels then establish master Start,
47.6% seek and native velocity retained after actual Output reopen. Wrong-button
captures still containing the dialog remain explicitly excluded.

The final rebuilt executable SHA256 is
c15cb5c18930019978bb8c335ec374258c67a2e24857ca7d455975808fb432a3;
core 5eb86098fbc52890a95d3226c56a4edd8e090308a940bb097749e23237532ab9;
selected startup 5f80be1f2dbe46faeb6d226328cace194d7770d086d5447c8b44928fb858559a.
Host GNOME Shell's PID/start remains unchanged. This closes the named private
X11 window/replay cases; host GNOME/Wayland, portal/hardware, wider solver/segment
camera regimes, outstanding example failures and full replacement remain open.
All independent Help tutorial walkthroughs are still Pending.


## Mixed optimizer/targeter scope correction — 2026-10-02

A new Yukon→DifferentialCorrector hierarchy exposed accepted inner samples leaking
unaccepted outer trials into None displays. Optimize now keeps its currentRunState
in sync with the five existing direct publisher transitions, without changing
child command states or numerical execution. MixedSolverScopes passes 0.99 s; the
same new check with installed VF13ad passes 1.03 s, including analytic goals, full
Reports, accepted paths/endpoints/camera, Unicode source reopening, inner
breakpoint Stop and exact complete recovery. The full 35-row Yukon report is
byte-identical before/after the correction. Original failure and the subsequent
Stop-file fixture failure are preserved. See
[bounded mixed solver evidence](Qt6ParityValidation/mixed-solver-scopes-20261002.md).
No old solver matrices or shipped corpus repeated. Actual GmatQt/core rebuilt;
app hash c15cb5c... is unchanged but linked core is now 68a79c56... . Named-arc and
full desktop gates remain open, as do all independent Help tutorials.

Final read-only review strengthens the new Stop proof: both tests observe the
intended index9/inner propagation text. VF13ad derives InternalOptimizer; these
two runs qualify two internal optimizers. The three RunExternalSolver assignments
are source-reviewed only. Boundary-verified preserves the final0.99/1.03 s checks;
the prior0.99/0.97 s pass remains an intermediate with weaker boundary proof.


## Bounded private Wayland input and lifecycle — 2026-10-02

[private-wayland-20261002.md](Qt6ParityValidation/private-wayland-20261002.md)
adds actual compositor-delivered keyboard/pointer evidence for the plotting/
output-lifecycle and script/pending-edit gates in an owned private software
Wayland configuration. The authored fixture completed once in 1.404 s; shared
master/Orbit/Ground replay controls reached 48.6% while XY remained rendered.
Three Super+h/Alt+Tab main-window minimize/restore cycles and the confirmed
second Output activation retained the native Orbit scene at 48.6%. Failed
Alt+F9 and earlier non-reopening Output/Enter attempts remain explicit.

Actual pending XY Close→Cancel retains the form/value; subsequent Close without
Saving removes it. Final Save As writes exactly the original 5371 bytes and
SHA256 2d30468533a50b6495871e381803045d687fc0429b504b4d6739f2b8cd37e8a4.
Screenshot024 still shows the chooser; later fixture hashing, main-window title
and post-session byte comparison establish Save. A later saved-file reopen/run
is untested. Open/Save use the native in-process GTK chooser; portal service
activation and registration logs do not establish FileChooser/Response parity.

This recorded runtime is app c15cb5c18930019978bb8c335ec374258c67a2e24857ca7d455975808fb432a3,
core 68a79c56f22c22407ba93f462e73fad4c4050d0cc9f54b0203b79f0d471085c3,
and helper ee8368a88d1b79be22bb127e6e04afe7f2a562443323374e896e6a6c0c2d944d,
before subsequent arc/tree and optional portal changes. Full launch-file
identities and all nine closed sessions are preserved under ignored
build/example-qualification/20261002/wayland; earlier preparation failures
and core-identity uncertainty remain distinct from qualification. Selected
unchanged PNGs are linked in the record.

No host runtime/socket, physical GPU/input or system bus was exposed; cleanup
left the host GNOME PID/start identity unchanged. This does not fix or qualify
the user's host crash, hardware/host GNOME, portals or wider viewer/solver
regimes. No Help tutorial construction was performed. Full Linux qualification
and all tutorial walkthroughs remain open; Windows/macOS and MATLAB remain
deferred. This documentation/copy step launched no additional GUI or test.



## Reconciled public TLE input checkpoint — 2026-10-02

The two source-matched runtime retries after exact June 2020 input recovery are
now in the corpus ledger: Falconsat7Jupe completed in 1.172 s; FalconSats resolved
its catalogs but failed in 0.622 s with SPICE(BADMECCENTRICITY) while using
historical elements at SystemTime(now). FalconSats remains an unexpected runtime
failure, not a scientific pass or proprietary/expected-Stop exception. No
other mission, build or passing test was repeated for this targeted TLE work.

Current corpus totals are 147 passed builds / 17 failed builds, 137 completed
executions, five unexpected runtime failures, two expected tutorial Stops and
three timeouts (raw 137 passed / seven failed / three timed out). Prior attempts
remain in history. The earlier five-absent-TLE-catalog statement is superseded:
Jupe's exact input is recovered and its historical run completes; FalconSats has
a current-time/stale-element domain failure; Contacts uses the now-decayed
FalconSat-7; GSFCSats/Starlink still need public December 2019 data through the
recorded human/account access route. Matching legacy MarsGRAM integration/data
remains unestablished without a proprietary-only waiver. See
[tle-dependency-inputs-20261002.md](Qt6ParityValidation/tle-dependency-inputs-20261002.md)
and the source-matched shipped corpus summary/table/failure records. These are
offscreen execution/input-disposition results; independent scientific accuracy,
native viewer acceptance and human Help tutorial construction remain separate.


## Named regular arcs and keyboard navigation — 2026-10-02

[Focused arc/navigation record](Qt6ParityValidation/named-arc-navigation-20261002.md)
adds actual publisher identity/effective trial state/flush metadata through the
existing TakeGlAction path. SegmentArc passes 0.61 s: same-command state reset
creates separate regular arcs; connected same-direction A→B→A retains the initial
name/provider union; Current accepted endpoints/attitude survive decimation of 7,
MaxPlotPoints=3, trial cleanup and Output deletion/reopen. All three full numerical
reports match independent camera-free missions exactly. Requested spacecraft
poses alone incur extra conversion. Compact arc records survive trajectory trim
and grow per disconnected arc until model/data reset; no separate metadata cap
or unbounded-run qualification. wx empty-context behavior remains unchanged.
The newly introduced accepted-Current duplicate replay was found and fixed during
pre-release review, with publication-level regression coverage.

TreeActivation passes 0.59 s through focused Return/Enter on Resources, Mission,
Report and retained Output plots, preserving pending/source/report state and
mouse/category/held-key/running/unbuilt guards. The first timer fixture failed
to observe its short running mission; a real synchronous publication now proves
the guard. Actual private Wayland station Return opens its settings in unchanged
screenshot013 on the final runtime. Native Output Enter remains unverified.

SegmentCameras passes 0.54 s with all original camera tolerances and behavioral
checks. The old 60-second stop landed on a 10-second step and skipped the engine's
conditional final FlushBuffers, so OF correctly retained one FirstArc through
both command names. The fixture is now 61+119 seconds (same total of 180 seconds), requires
two actual finalized/provider-distinct arcs, and verifies endpoint/A1 epoch/DCM
against a complete independent report. No production correction was warranted
for that assumption. Original compile, timer and camera failures/diagnostic source
remain preserved alongside final evidence.

Application/core rebuilt: app 7b4a0ae720bb074c71ac05e73d351e987beae5c7fcda524e105d7fb1d55fdae3,
core cf147e234b57818fecf475e0516da86d9187e10066d981e6c893ae7fd761e016.
Only the new or failed affected checks ran; no previous successful matrix/corpus
was repeated. Tutorial1 is in progress under separate evidence, with no Help
walkthrough pass claimed here. Host GNOME/hardware, wider viewer/solver gates and
full Linux replacement remain open; Windows/macOS/MATLAB remain deferred.


## Private portal FileChooser checkpoint — 2026-10-02

[Private portal evidence](Qt6ParityValidation/private-portal-20261002.md) adds
actual owned-bus Qt FileChooser traces for Open Cancel, accepted Save, directory
Cancel/path retention and directory Select. Exact Qt handle tokens correlate
Response 1/0/1/0 and owned URIs; the accepted 5371-byte Save equals the authored
fixture byte for byte. This narrows those four cases in the matching temporary
Qt6.10.2 plugin/private software Wayland configuration. Open acceptance/reopen
and Save cancellation remain unverified by this session: later chooser labels
are misleading, later Open requests return Cancel, and no second SaveFile occurs.

The recorded app/core are 7b4a0ae7…/cf147e23…; the record retains full launch
identities, three unchanged chooser PNGs and official extraction/license details.
All raw traces/actions/screenshots and first outer parsing failure are preserved
in ignored build/example-qualification/20261002/wayland/portal-first-attempt.
The helper transport repair has nine synthetic pipe-only passes and no repaired
GUI rerun; the script/result remain ignored evidence rather than a new source
test. Bounded cleanup leaves host GNOME PID/start identity unchanged, without
host portal, physical-driver or crash-fix qualification. No Help tutorial or
mission-execution claim is added here. Full replacement/wider gates remain open;
Windows/macOS/MATLAB remain deferred. This promotion ran no GUI/test/build.


## First independently constructed Help tutorial passed — 2026-10-02

[Help tutorial 1 evidence](Qt6ParityValidation/help-tutorial-01-20261002.md) replaces
the earlier all-walkthroughs-Pending disposition for Simulating an Orbit only.
Actual private XTest GUI input starts from New, follows available Help, renames/
configures Sat and LowEarthProp/forces/view, selects Earth Periapsis through the
parameter picker, runs, inspects command summaries including EarthFixed, animates
all master/Orbit/Ground labels at 17%, and saves/reopens its own mission. The first
600-second harness bound resumes the independently authored milestone. Source
was frozen before consulting the reference; prior corpus exposure is disclosed.

Actual authored/reference GUI runs complete in 0.361/0.174 s. Both show UTC
23 Jul 2014 20:48:34.393, radius 8600.5686739410 km and TA 360°. Differences remain
5.569 mm maximum position component (6.366 mm norm) and 3.178 micrometres/s maximum
velocity component. The first locally chosen 1 mm/1 micrometre-per-second assertion
failed; Help prescribes no threshold, and neither mission was changed/rerun to
force equality. Required physical/settings/logic match; force-order causation
is plausible but unproved. This is bounded GUI tutorial acceptance, not bitwise
numerical or independent scientific qualification.

The record preserves supported deprecated Drag warning, separate Qt ForceModel
placement, mistaken summary Save filename and first-reference BadWindow helper
failure/owned cleanup/repair. Five unchanged PNGs and the 5672-byte authored script
(SHA256 f0becc897f413aa8eb211813125e063ebd57e4e0d5fcf4136dd17142411dada4)
are tracked. App 7b4a0ae7.../core cf147e23... and raw/frozen/reference artifacts are
linked. Row 1 now Passed; other published/additional walkthroughs retain Pending,
with Tutorial 2 construction in progress separately. No old full matrix/corpus
repeated. Full Linux replacement, host/hardware/portal and broader viewer gates
remain open; Windows/macOS/MATLAB remain deferred. No runtime/source change was
made to prepare this evidence record.


## Simple Orbit Transfer Help walkthrough passed — 2026-10-02

[Help tutorial 2 evidence](Qt6ParityValidation/help-tutorial-02-20261002.md) records
independent Welcome/Help/New construction, TOI/GOI/DC1 and ordered targeting through
actual controls. Seven iterations/0.760 s satisfy the 0.1 km radius/0.0001 ECC
limits; the actual Target editor Apply Corrections button changes only two Vary
guesses, then one iteration/0.647 s repeats the achieved goals. Actual own Ctrl+O
reopen retains corrected TOI/GOI fields and byte-exact 5978-byte source (SHA256
652c63e2559d6cda7e489f458c870ae8908fd8aa780adf8b518afd5cc4c4d20c).
Two initial Achieve entries put later input in Name and were corrected before F5;
no unproved focus cause or numerical failure is claimed.

After the 17:57:42.093446 UTC freeze, one actual reference GUI run (1.881 s) has the
same complete 4991-byte solver report, SHA256
3edcbc50a784dcf587a7fb68c1d5b9a031e9de09b97beaddaa081f58a8a20d21.
Bounds/Target ExitMode and omitted resource/display defaults remain explicit;
report equality does not establish every-state or independent scientific parity.
Reference view translation was not saved to its original source. Five unchanged
PNGs/final authored source accompany the record; detailed raw evidence and the
post-freeze source comparison are indexed under ignored 02-simple-orbit-transfer.
Tutorials 1–2 now Passed, 3 In progress after actual Help/resources/commands and
13-iteration first authored convergence without reference access. Other tutorial
and full Linux replacement/host/hardware gates remain open. No old matrix/corpus
was repeated; Windows/macOS/MATLAB remain deferred.


## Finite-burn Help walkthrough passed — 2026-10-02

[Help tutorial 3 evidence](Qt6ParityValidation/help-tutorial-03-20261002.md) records
independent Welcome/Help/New hardware/resource/mission construction, finite-burn
target convergence (13 iterations/0.749 s), four exported summaries and actual
save/reopen. Cutoff MA is 25.131809686270°; fuel decreases 756→343.76990738327 kg;
perigee/apogee MA is 0/180° and radius 12000.000012291 km satisfies 0.1 km tolerance.
An unintended pending coordinate wheel change was discarded before Apply.

To inspect Help's trial view, explicit SolverIterations Current→All is the only
semantic model adjustment; the necessary 0.736 s run has the same full report.
Mouse face-on viewing and temporary stars/constellations/XY plane overrides leave
saved drawing defaults retained. Final 6190-byte source SHA256
8a4a6914110f27b306ad256c6c0e4dc7a7f943d2c0100d7810f1b2a3d34e84fa
was frozen at 18:26:43.612951 UTC. A later actual reference Ctrl+O/Convert views/
F5 run completes in 0.541 s without saving the original source. First authored,
All-history authored and reference whole reports are byte-identical: 5583 bytes,
SHA256 809ed5bba4fe90001b83612fc2e1790935e09f129a2a13f0689bcabb97853f14.

Source comparison retains lower bound −10 versus 0, All versus Current and
omitted hardware/solver/modern defaults. Complete-report equality does not prove
every propagated sample, rendered pixel or independent scientific accuracy.
Six unchanged PNGs/final authored source accompany the record; 114 raw files,
69 PNGs/19,806,000 bytes remain in the ignored artifact index. Runtime app
7b4a0ae7…/controlled core cf147e23… and owned 0/0/-15 cleanup are bounded private
X11 evidence, not host/hardware/crash qualification. Ledger 1–3 Passed, 4–5
In progress and 6–12 Pending. Tutorial 4 resource construction encountered a tank
creation-order issue/failed milestone save; reconstruction is underway. Tutorial 5
has initial Help blocks typed through the blank actual private GUI editor,
resources still entering and no reference consulted. Neither is a completed
walkthrough. Other tutorial/full replacement gates remain open. No old matrix/
corpus was repeated; Windows/macOS/MATLAB remain deferred.


## Help-discovered resource fixes and rebuilt runtime — 2026-10-02

[tutorial-resource-fixes-20261002.md](Qt6ParityValidation/tutorial-resource-fixes-20261002.md) records valid coupled ChemicalTank
settings, Local celestial-body Origin choices, owned propagator Type/FM retention
and scoped wx-style default force models for new numerical propagators. Explicit
existing model sharing and imported implicit models remain preserved. Actual creator
Cancel/name changes/Create, rollback, source/Undo/Redo/Unicode save-reopen pass four
new focused checks. Creation's initial exception was a test SRP getter mismatch;
both failure attempts remain retained, and only Creation was rerun after correction.
The rebuilt app is SHA256 7fb5bd6cdf8c2630599a2f8c4d20c5f1ce680c41f297ab8fc1ac158b4664f6d4,
with controlled core cf147e23… and selected startup 5f80be1f… unchanged. No old
passing matrix/corpus was repeated and no numerical-engine algorithm changed.
Tutorial 4's affected actual GUI steps resume separately. Tutorial 5's five own
stages now execute successfully, including a 5000.202164764 km lunar periapsis
exercise (11.291 s/four nominal passes); its GUI-authored final source has been
saved/reopened and frozen before reference comparison. Neither walkthrough is
marked Passed by this resource-fix checkpoint. Full Linux/host/hardware and other
tutorial gates remain open; Windows/macOS/MATLAB remain deferred.


## Case-sensitive Mars gravity default repaired — 2026-10-02

[mars-gravity-default-20261002.md](Qt6ParityValidation/mars-gravity-default-20261002.md)
records the actual Tutorial 4 NearMars Apply failure and Qt's corrected startup
resolution of MARS50C to official Mars50c.cof. Retained custom files and unrelated
forces remain intact. New MarsGravity passes 0.21 s; directly affected GravityBodies
passes 0.97 s. No extra Mars numerical mission or other passing suite/corpus was
repeated. The rebuilt app is 3ba673f1b2060dc6957d34f94ca11e9f48db3273374afb7d58da7c6654f52ef1,
with base cf147e23…/selected startup 5f80be1f… unchanged. Actual Help 4 resumes
from its own saved resource/view milestone; Help 6 begins independently from its
Help instructions. Full replacement/host/hardware and remaining tutorial gates
stay open; Windows/macOS/MATLAB remain deferred.


## Private portal completion and Output Return — 2026-10-02

[private-wayland-followup-20261002.md](Qt6ParityValidation/private-wayland-followup-20261002.md)
records current app3ba673f1…/basecf147e23… actual private portal Open acceptance
(Response0/owned fixture URI) and Save As cancellation (Response1/no new file,
unchanged fixture), using the repaired request-ID helper. One GUI F5 completes
0.595s; pending Orbit Earth close Cancel/Discard and readback retain the resource.
A single Output leaf Return reopens the retained Earth/trajectory/camera at shared
48.6%. Earlier successful portal cases, main minimize/restore and numerical
matrices were not repeated. Seven unchanged PNGs and 209 indexed raw files retain
unsuccessful intermediate actions. Helper quit was acknowledged; parent sandbox
teardown was -15, with a transient owned compositor in its immediate inventory.
Later owned-PID absence and unchanged user GNOME PID/start ticks are recorded;
no graceful GUI Quit, host/physical-GPU/crash or full replacement acceptance is
claimed. Remaining Help walkthroughs continue separately. Windows/macOS/MATLAB
remain deferred.


## Optimal lunar flyby Help walkthrough passed — 2026-10-02

[Tutorial 5](Qt6ParityValidation/help-tutorial-05-20261002.md) is Passed for bounded
private actual-input construction through its expressly taught code editor. All
five stages are covered: deliberate configuration Stop, patch-only/full/alternate
solves, and the 5000-km exercise. Actual own reopen/build precedes the
19:20:39.894341 UTC freeze of the 18701-byte source, SHA256
db65522abd06f9f4be8a52fb7c1b835db9e937751ab06887871595f9aa28f86c.
The own exercise converges in 11.291 s/four nominal passes; radius is
5000.202164763518 km and all eighteen equalities are retained.

Post-freeze original sample Step 5 fails in 162.538 s/44 passes/43 report iterations;
radius 4860.44617331 violates 5000. Its whole failure report equals the own first
failure only after 44 inequality-owner labels normalize. The reviewed repair
appends 22 prior own nominal-stage4 controls in all three Step 5 copies, preserving
physics/constraints/solver and archived originals. One actual updated-sample
CtrlO/Convert views/F5/readback passes 10.977 s/four passes/three report iterations.
Its whole 9854-byte report SHA256 72531ef34b947aa49bb04ceb826a626aa7143d8bb4eef36a6ccd969b5238a588
equals the frozen own 9850-byte report only after four printed backward-to-forward
Luna.RadPer labels normalize. No reference conversion Save or Steps 1–4 rerun.

Help copies are synchronized but not separately executed; original sample Step 3
Earth gravity differs, Help epoch-qualification equivalence is untested, and final
MOI perturbations/pass counts remain explicit. This is complete-report equality,
not every-state/visual/scientific parity. Ten unchanged PNGs and the frozen own
source accompany the record; full raw/source/result histories remain indexed in
ignored Tutorial 5 evidence. Runtime updated app 3ba673f1…/controlled base cf147e23…
is bounded private X11/softwareGL evidence; host/hardware/full replacement gates
remain open. Ledger 1–3 and 5 Passed, 4 and 6 In progress, 7–12 Pending.
Windows/macOS/MATLAB remain deferred; no old matrix/corpus was repeated.


## Mars B-plane Help walkthrough passed — 2026-10-02

[Help tutorial 4 evidence](Qt6ParityValidation/help-tutorial-04-20261002.md) records
independent resource/command-form construction, separate owned force models,
B-plane and capture solves, four exported summaries, three views and actual own
save/reopen before freezing the authored source. Both B-plane constraints meet
1e-5 km; Mars capture reaches 12000.01965272428 km within 0.1 km. Apply Corrections
retains all four updated Vary guesses and makes both Targets converge in one
iteration. The frozen 8628-byte source (SHA e889a806…) precedes the single reference
GUI run, whose targets also pass. The original sample is unchanged.

All intermediate failures and eight raw sessions are retained in the ignored
04-mars-b-plane evidence tree; seven unchanged PNGs and the authored source
accompany the record. Default omissions and OpenFrames/native differences remain
explicit; complete-report, every-state, pixel and scientific equality are not
claimed. App 3ba673f1…, controlled core cf147e23… and startup 5f80be1f… are unchanged.
Other chapters and full Linux, host, hardware and crash gates remain open;
no old matrix/corpus was repeated. Windows/macOS/MATLAB remain deferred.


## Electric propulsion Help walkthrough passed — 2026-10-02

[Tutorial 8](Qt6ParityValidation/help-tutorial-08-20261002.md) passed independent
Help-driven electric hardware and finite-burn form construction, one two-day run,
spiral/fuel inspection and actual own save/reopen before freezing the authored
5750-byte source at 20:08:14.975711 UTC (SHA 9e80e251…). The own run completes in
0.850 s, reaches SMA 8362.8537296235 km and consumes 20.96985583229 kg of fuel.

Post-freeze comparison found the original sample used a polynomial thruster and
Earth shadowing instead of the Help's constant 5 N model and empty shadow list.
Only those two settings are repaired. One changed-reference GUI run completes in
0.900 s and its full 3283-byte propagation summary matches the frozen own summary
byte for byte. Original results, all three private sessions and hashes are
retained; nine unchanged PNGs and the authored source accompany the record.
Execution uses app 3ba673f1…; a later function-output fix rebuilds ef933221… without
repeating this unchanged chapter. Full Linux, host, hardware and crash gates and
other tutorials remain open; Windows/macOS/MATLAB remain deferred.

## Eclipse and station contact Help walkthrough passed — 2026-10-02

[Tutorial 7](Qt6ParityValidation/help-tutorial-07-20261002.md) passed bounded
private actual-input construction of the Help chapter's core scenario from the
independently authored Tutorial 2 prerequisite. Earth shape, EclipseLocator,
Hyderabad station and ContactLocator were entered through resource forms; actual
Output reports show one 2105.5299296-s eclipse with three portions and two station
contacts. The combined run completes in 1.645 s. Own save/CtrlO reopen/Build and
resource readback precede the 20:12:02.032395 UTC freeze of the 6498-byte source,
SHA256 c7964540d1e47ff970989e9b8bfbe880ab308fb5c01554a4d69c27a8b1d709d5.

The blank-target initial attempt is retained as an operator correction. Help
names only the Simple Orbit Transfer prerequisite; no final shipped Tutorial7
script exists. Post-freeze additions match Help settings and qualitative report
expectations; prior compatible Tutorial2 reference evidence is reused without a
passing rerun. Default/Target/view differences remain explicit, as do optional
station-network/burn-coverage exercises and the missing GUI LSK field. Nine
unchanged PNGs and frozen own source accompany all raw actions/reports/identities
in the ignored indexed Tutorial7 evidence. App 3ba673f1..., controlled base
cf147e23... and startup 5f80be1f... are unchanged across both private X11/softwareGL
sessions. Bounded/acknowledged-helper cleanup is 0/0/-15, not graceful GUI Quit or
host/Wayland/portal/hardware/crash/full replacement acceptance. Windows/macOS/MATLAB
remain deferred; no old matrix, corpus or prerequisite-reference run was repeated.


## DSN simulation Help walkthrough passed — 2026-10-02

[Help tutorial9 evidence](Qt6ParityValidation/help-tutorial-09-20261002.md) records
independent staged script-editor construction, the short four-observation run
(0.081 s) and realistic three-station/ramp/noisy 21-day run (22.963 s). All 1348
finite observations have paired range/TCP station/epoch identities; all 674
range frequencies match the Help ramp metadata. Full own observations are
retained for independently authored Tutorial10. Save/reopen/Build and both
normal 0/0/0 private GUI exits precede the 20:21:58.590173 UTC freeze.

Explicit post-freeze numerical settings match the samples apart from hardware
attachment order and owned ramp path. Earlier source-matched corpus results
are reused, preserving their earlier-app/offscreen limits; no noisy reference
rerun or scientific equality claimed. Seven unchanged PNGs, authored sources,
ramp and full observations accompany the indexed complete raw evidence. App
identities3ba673f1.../ef933221... retain base cf147e23.../startup5f80be1f... .
Full Linux host/Wayland/portal/hardware/crash and remaining tutorial acceptance
remain separate; Windows/macOS/MATLAB deferred; no old matrix/corpus repeated.


## Event-locator creation defaults repaired — 2026-10-02

[Focused creation evidence](Qt6ParityValidation/event-locator-creation-20261002.md)
records the empty-target issue found in actual Help tutorial 7 construction.
New drafts select the first existing spacecraft and creation persists it even
when untouched. Explicit spacecraft/Contact regions are retained; invalid
choices reject before mutation; merely opening/Cancel creates no spacecraft.
One new offscreen widget regression passes in 0.37 s; independent source review
found no substantive regression. The first test compile error and identity
recorder correction remain recorded; no old suite or corpus was repeated.

The application is rebuilt as e5337acd... with controlled core/startup unchanged.
Subsequent independent Help 10–12 runs exposed a separate shared estimator
residual-plot issue: legends appear but samples are dropped, although numerical
reports complete. That plotting fix and the remaining tutorial/host/hardware
acceptance continue; no full replacement pass is claimed. Windows/macOS/MATLAB
remain deferred.


## GMAT functions Help walkthrough passed — 2026-10-02

[Help tutorial 6 evidence](Qt6ParityValidation/help-tutorial-06-20261002.md)
records independent resource/command controls, the own literal Help function,
Global sharing and repaired named no-output call. Inside/outside targets converge
in 6+11 iterations; explicit B-plane 1e-5 km goals and actual Achieve summary
RMAG 12000.017391244 km within 12000±0.1 km pass. MOI mass depletion, Call state,
shared three-view 0→47.6→88.6% animation and actual MarsView close/Output Enter
reopen are retained. Own source/function readback precedes 20:43:38.795064 UTC
freeze and compatible post-freeze comparison.

Full mission 4.748 s and justified required-observation run 4.915 s retain
private-X11 limits; earlier 5.588 s sample on 4522 app is reused without rerun.
Nine unchanged PNGs, exact sources/summaries/report/solver and 269 indexed raw
files preserve all failures and provenance. All nine sessions close through
controlled app SIGTERM (-15), Xvfb/WM exit0; graceful application exit is unclaimed.
Offered SolverIterations Current leaves full trial history unqualified.
No scientific/bitwise/source-default equality is inferred. Host/Wayland/portal/
hardware and remaining tutorial gates stay separate; Windows/macOS/MATLAB
remain deferred. No old matrix/corpus repeated.


## Estimator-owned residual plot data loss repaired — 2026-10-02

[Focused residual-plot evidence](Qt6ParityValidation/owned-residual-plot-20261002.md)
records the independent Help10–12 failure: bulk Deactivate discarded samples.
Separate Qt redraw suspension now retains data and forces Activate refresh;
ordinary Toggle admission remains independent. New actual OwnedPlot callback
regression passes 0.08 s after a disclosed test-only temporary-reference fix.
Rebuilt app b8074b02 retains unchanged base cf147/startup 5f80. One affected
actual GPS filter retry shows populated curves/export and identical CSV, with
report differences limited to Run Date. Chapter completion and host/physical
GPU gates remain separate; no old corpus or successful matrix repeated.


## DSN/GN orbit estimation Help walkthrough passed — 2026-10-02

[Help tutorial10 evidence](Qt6ParityValidation/help-tutorial-10-20261002.md)
records independent Help script-editor construction using own frozen9 inputs,
DSN two-iteration convergence, the actual shared residual-loss finding and
one changed-path34.357-s verification with allsixcurves/style/text/PNG export.
CAN range209 points reconcile report filtering and known A1 epoch offset.
Mandatory GN simulation24.271 s generates1348 paired observations; one
justified retry after preserved/tmp exhaustion completes53.322 s with full
three-iteration report,1342 used,WRMS.959887 and six rendered residuals. GN
CAN RangeRate209 exportpoints reconcile report/epochs. Rounded GN position
truth error27.585935 km exceedsprior7.549834 km; statistical convergence is
not scientific truth-accuracy acceptance.

Own Save/Ctrl+O reopen/Build and all0/0/0 cleanup precede21:17:24.535580 UTC
freeze. Post-freeze source/settings comparison plus source-matched earlier
55.659-s corpus sample avoids a needless noisy reference rerun. Known
hardware/input/data-output differences and all omitted defaults remain
explicit. Full raw failures/actions/results,three authored sources,own GN
observations,text/PNG exports and11 unchangedPNGs accompany the record.
Appb8074b02... retains basecf147e23.../startup5f80be1f... . MATLAB-dependent
analysis,full scientific/host/Wayland/hardware/crash and remainingtutorial
gates stay separate; Windows/macOS deferred; no old matrix/corpus repeated.


## GPS filter/smoother Help walkthrough passed — 2026-10-02

[Tutorial 11](Qt6ParityValidation/help-tutorial-11-20261002.md) completes bounded
private actual-input construction of simulation, cold filter/smoother, warm
start, residual/covariance inspection, two ephemerides, one-day prediction and
Linux console Appendix B. Own source/readback and 201-file freeze at
21:20:41.811444 UTC precede any related sample access. The affected plot retry
retains the original CSV byte for byte. All 49 warm a priori covariance values
match report rounding; 145 cold and 36 warm observations are accepted.

The prediction writes 217 filter and 937 smoother state/covariance rows; the
one Linux GmatConsole run exits0 in1.343 s and reproduces both ephemerides byte
for byte. Reports match after Run Date and three equivalent path prefixes
normalize. Original storage-exhaustion zero-byte files, harness mistakes,
all stages/actions and controlled final app−15 cleanup are retained. Related
2010 GPS examples differ deliberately; source-matched earlier corpus results
are reused without noisy reference reruns. Appb807/corecf147/startup5f80 remain
bounded private evidence; new consolef80e and unused corrected Bulirsch0472
identities are recorded. MATLAB analysis/conditioning, host/hardware/crash and
full replacement gates remain open. No old suite/corpus was repeated.


## Bulirsch configured reductions retained — 2026-10-02

[Positive custom controls](Qt6ParityValidation/bulirsch-controls-20261002.md) passes
one new0.27-s focused test and one private actual-input retry. The retained first
failure shows GUI Apply writing default reduction values; two copy-constructor
initializers now retain configured0.6/.0001 through serialization and actual
runtime clones. Algorithms are unchanged. Apply, exact source, save/CtrlO/form
reopen, one60-s F5 Completed0.336s, actual Output report and saved command summary
pass; maximum printed Cartesian discrepancy2.2055e-11km. Deprecated read-only
MinimumTolerance remains absent. Original failure/source and all raw/check
artifacts remain linked. Actual File Exit application0/WM0/Xvfb0 is distinguished
from the first helper-terminated app and the final helper's expected already-exited
application diagnostic. Appb8074b02... is unchanged; rebuilt ExtraPropagators
SHA04726589... supplies the correction. No old matrix/corpus rerun or full Linux,
host compositor, hardware, portal or scientific completion is claimed.

## Inter-spacecraft tracking Help walkthrough passed — 2026-10-02

[Tutorial 12](Qt6ParityValidation/help-tutorial-12-20261002.md) passed bounded
private actual-input construction through the Help-taught script editor. Ten
literal original Help blocks were entered into an empty new mission, saved,
run and reopened with simulator/estimator/command readback. Initial 12.392-s
execution converged in three iterations but both residual plots were blank;
that first failure and zero-sample export remain preserved. After the shared
OwnedPlot redraw/data-admission repair, one affected unchanged own 4.425-s retry
renders both plots and exports 806 Range/804 RangeRate points, reconciled to
final accepted report rows. Actual CtrlO/Build and final freeze 21:01:07.402213 UTC
precede reference access. All 126 operational statements match the original
sample; no original-reference GUI run or seeded authoring was needed.

The literal original tracking state is scientifically unsuitable: inertial
perigee 161.081 km lies inside Earth. A reviewed source-backed frame inference
changes only SimTrackSat/EstTrackSat CoordinateSystem to EarthFixed, retaining
state numbers and physics/solver settings. One changed-sample actual GUI run
completes in 3.406 s / two iterations, 1646/1652 observations, WRMS 0.990936275466, and
824/822 exported points reconciled to final report rows. GUI frame/Keplerian
readback agrees with independent polar-motion/LOD invariants, SMA 42166.24166491408 km,
ECC 4.033172844e-7. Preview changes were Discarded; no reference Save. The orbit
is near-geosynchronous, not an asserted historical recovery or general
scientific validation. Help illustrative GMD/report-frame excerpts were
refreshed only from that verified run. Original own 5478-byte source SHA 43e4aa2e...
stays unchanged; corrected sample SHA dcb5b405... and final Help 07777b2d... are
retained with indexed raw evidence and ten unchanged PNGs.

Construction app e5337acd...; affected/corrected sessions app b8074b02...;
base cf147e23... and startup 5f80be1f... are unchanged. Actual AltF4 closes all
three with 0/0/0; helper CLI exit 1 is its owned-exit handling, not a mission timeout.
Initialization-only EOF and transient /tmp exhaustion/recovery remain explicit.
No host GNOME/Wayland/portal/hardware/crash or full replacement acceptance is
claimed. Windows/macOS/MATLAB remain deferred. No old passing matrix or resource
construction was repeated.

The single build-qt-help build exits 0 at 21:31:25.812008 UTC; installed-content
checks at 21:32:17.193642 UTC confirm EarthFixed inputs and refreshed verified
GMD/report-frame excerpts. Supplemental build/check metadata retains the initial
checker path correction; no repeated build or mission.


## Positive FixedGrid report with valid owned frame — 2026-10-02

[FixedGrid evidence](Qt6ParityValidation/fixed-grid-20261002.md) closes one
selected-plugin gate through actual private GUI input. New/Script editor builds
an independently declared fixture; GridEvents selector/Browse/Apply, Save As/CtrlO,
Output Enter and final settings reopening pass. The first 60-s fixture completes
with insufficient-ephemeris warning and a degenerate 2000 report; it remains
unqualified and preserved. Only GUI MaxStep 10 plus duration 300 correct coverage.
One 0.052-s retry retains explicit UTC 2015 Jan 1 noon/UseEntireInterval and produces
31 finite 10-s entries/one Sun event exactly 12:00–12:05. No backend code or
numerical algorithm changed. Actual File Exit app/WM/Xvfb 0 is distinguished from
helper's already-exited-app handling 1. Appb8074b02.../core/startup remain unchanged.
No old matrix repeated; broader locator, host-native and scientific gates stay open.


## Ordered Python module paths and fresh restart readback — 2026-10-02

[The bounded path record](Qt6ParityValidation/python-module-paths-20261002.md)
qualifies Python Module Add/Replace/Remove/Up/Down/Browse in Set paths, pending
startup import/export that retains other source, Cancel/invalid rollback and
other-path Apply that preserves the live Python list. Imported pending rows remain
independent of the engine's stale list. Export and restarting with that file
are required for Python changes; no live reload/cache reset is promised.

One affected Paths check passes in 1.20 s; the new PythonPaths check passes in 1.17 s
after its preserved raw-absolute getter fixture failure is corrected, with no
production change or diagnostic rerun. Two fresh children select independent
13.25/23.5 constants. One private actual-input workflow adds/reorders/exports/
imports and reopens its GUI-authored source. A fresh exported-startup GUI run
completes in 0.031 s, opens report 23.5 and records the exact modules-second origin.
Both owned sessions close app/WM/Xvfb 0/0/0; helpers' already exited app diagnostic 1 is
separate. Full original 47 files / 5,064,944 bytes, startup/source/module hashes,
typing-protocol mistakes and five unchanged PNGs are retained and indexed.

Source a922195f... and actual app 787cd09b... / 5,876,904 B retain base cf147...,
util 1e4e... and production startup 5f80... . First-failure fixtures/logs and passing
stdout/source identities remain in the owned controls tree. Custom-path
ExternalForce execution, package/environment/cache behavior, host/Wayland/
portal/GPU/crash and full replacement gates remain separate. No old successful
matrix or corpus stage was repeated; Windows/macOS/MATLAB stay deferred.


## Earth Region/spacecraft overflight workflow passed — 2026-10-02

[Region evidence](Qt6ParityValidation/region-contact-20261002.md) closes one
ordinary Earth rectangle/one-spacecraft/no-imager workflow. New Regions Add and
paired inline vertices, Contact pending target/observer/output, Save As/CtrlO,
form readback and actual Output Enter pass. One F5 Completed 0.052 s produces one
2015 Jan 1 noon event lasting 81.477054418 s. Thirty-one finite 10 s EarthFixed state
rows bracket the +5° longitude exit at 80–90 s; interpolated 81.477227955 s is an
approximate check, not exact SPICE/event identity or a scientific threshold.
Source 1186 B SHA 2825e9fd... remains exact. Actual File Exit app/WM/Xvfb 0/0/0;
helper already-exited-app diagnostic 1 is separate. Initial Save/Enter timing
never committed/ran; all raw actions/screens remain. Source 2f9810c0/new Region 0.36 s
and affected EventLocators 4.65 s once retain creator-route/metadata failures,
detached-context fixture correction and interrupted build provenance. Current
app 244dc41a... / 5,926,536 B keeps base/util/startup controlled. Broader locator/FOV,
host/GPU/scientific and full Linux gates stay open; no old matrix repeated.


## Local Spacecraft/String function arguments passed — 2026-10-02

[The bounded local-arguments record](Qt6ParityValidation/local-function-arguments-20261002.md)
qualifies one independently authored whole-Spacecraft/String positional call.
Actual Inputs offered/selected ProbeSC and LabelIn; Outputs selected ReturnedMass
and Echoed. One Apply, Save/CtrlO/Build and sole F5 Completed 0.021 s produce an
exact known 123.5/text return and unchanged declared caller mass/text. Output
readback passes; 604 B mission and 207 B function hashes remain unchanged.
All 180 actions/30 screenshots, incomplete initial chooser Save/Enter timing,
one 105 B report and freeze/cleanup identities are retained. App/WM/Xvfb normal
exits 0/0/0 differ from the helper's already-exited diagnostic 1 before its 600 s
deadline. App 244dc41a... / 5,926,536 B and controlled core/util/startup are unchanged.
Existing whole-Array/global/zero-input evidence is reused without reruns. Other
resource/signature/object-output cases and physical desktop/GPU/crash/full Linux
replacement gates remain separate; no source/build/test changes were required.


## Two independent thrust histories through the GUI — 2026-10-02

[The bounded thrust record](Qt6ParityValidation/multi-thrust-live-20261002.md)
qualifies distinct two-segment histories, segment/tank readback, retained History
selection, separate one-spacecraft Begin/End controls, cardinality rejection,
Save/CtrlO/Build and actual Output Enter. Independent setup was typed in the
actual Script editor; only owned documented thrust data was supplied, no sample
mission seed. Original Synchronized F5 failed 0.351 s; its exact failure epoch
is unknown and the source-backed step-boundary explanation remains an inference.
The first sequential revision Completed 0.349 s but failed intended epochs/fuels
because ElapsedSecs goals are per-command durations. Only eight later literals
changed to 5; first 10 values and all physical settings/inputs were retained.

One corrected saved-source F5 Completed 0.330 s gives six finite boundary rows at
0/10/15/20/25/30 s. Max time/fuel residuals 2.165325e-7 s / 4.121148e-13 kg pass
independent 1e-6 bounds; final own source 5205 B SHA 317e151e... and 2502 B report
SHA c2a06348... remain exact. Actual AltF4 exits app/WM/Xvfb 0/0/0, distinct from
helper already-exited diagnostic 1. Original sources/reports, unsaved attempt,
600 s controlled -15 expiry after corrected Save, chooser/operator corrections,
full runtime identities/actions and eight unchanged PNGs remain retained/indexed.
App 244dc41a... / 5926536 B, core cf147..., util 1e4e..., startup 5f80..., plugin
e978... stayed fixed; prior cardinality 0.25 s was reused, no old tests repeated.

Synchronized boundary operation and combined asynchronous OrbitView remain
unqualified: completed final scene/log confirms absent SatA receives zero
placeholders in the shared callback and latest display; source appends earlier
history rather than clears it, actual point counts were not exported. No
trajectory truth/backward/time-varying-profile/solve-for, host/Wayland/portal/GPU/
crash or full replacement claim; Windows/macOS/MATLAB remain deferred.


## Custom-path ExternalForce after exported-startup restart — 2026-10-02

[The bounded custom-force record](Qt6ParityValidation/custom-external-force-20261002.md)
closes the missing consumption of GUI-exported Python paths by an actual force
callback. One Add/export process exits normally; a fresh process keyboard-authors
its own scaffold, Applies module/function/exclusion, saves/CtrlO/builds/readbacks,
and completes exactly one 60 s F5 in 0.038 s. Engine and callback origin both name
the owned directory outside bundled paths. Two finite report rows satisfy the
predeclared closed-form component bounds: max 8.891523749e−7 km / 1.187436260e−10
km/s at reported elapsed time 59.99999988125637 s; nominal-60 results also retained.
Own 1344 B source SHA 8318112e... and callback/export/report bytes remain exact.
Live-dialog startup export normalizes the FileManager snapshot, preserving the
21 ordered plugins; no original-comment byte or live-cache claim is made. The
terminal-slash verifier correction is retained. Both File Exits app/WM/Xvfb
0/0/0 differ from helpers' already-exited-app diagnostic 1. App 244dc41a... /
5,926,536 B/core/util/startup stayed fixed. No engine/build/test/matrix/corpus
repeat; package/environment/cache and host/hardware/full replacement gates remain.


## Missing-spacecraft viewer checkpoint — 2026-10-02

The independently authored two-history case exposed a completed OrbitView that
placed SatA at Earth's origin when a subsequent SatB publication omitted SatA.
Commit `08cdda3c` carries explicit presentation availability metadata through
ordinary callbacks, Current replay and named-camera preparation. Qt retains the
last real pose and breaks the resumed trajectory; valid origin coordinates and
existing numerical arrays/commands remain unchanged. Legacy absent-data warnings
remain visible. The wx capability guard is source-reviewed, with no wx build claim.

[The repair/build record](Qt6ParityValidation/viewer-data-availability-20261002.md)
binds the rebuilt 5,935,320-byte application, SHA `989d3499...`, and core SHA
`2082a341...` to the recorded source. The new availability regression passes
0.31 s after its preserved test-only structural-notification assertion failure;
only the failed test target was rebuilt/retried. Affected InvalidPlotData and
SolverPlots pass once in 1.49/4.07 s and were reused.

[One actual-input affected retry](Qt6ParityValidation/multi-thrust-viewer-retry-20261002.md)
opens the frozen authored source, completes one effective F5 in 0.335 s, displays
both spacecraft off Earth's origin and opens the Output report. The entire
2,502-byte report is byte-identical to the frozen pre-fix report. Normal owned
application/WM/Xvfb exits are 0/0/0. No passing tutorial, corpus or old matrix was
repeated. This private X11/software GL evidence does not qualify the host desktop,
physical GPU or compositor-crash cause; synchronized thrust and other explicitly
recorded external/domain limits remain separate.


## Saved compositor fault mechanism identified — 2026-10-02

[Read-only saved-core evidence](Qt6ParityValidation/compositor-popup-null-resource-20261002.md)
resolves the prior PID 15518 fault to Mutter popup setup sending an event through
a null Wayland resource. Exact Ubuntu 50.1-0ubuntu2.4 symbols/source match the
recorded module build ID. Register/instruction evidence establishes the immediate
fault; optimized-out popup fields leave its owner and precise interleaving
unproven. The inspected current upstream branch retains the unguarded send;
earlier related repairs already exist in 50.1 and do not cover this branch.

No live reproduction, application test, host change or speculative GMAT patch
was made. The original crash record remains unchanged, and runtime SHA 989d3499…
/base SHA 2082a341… remains the previously rebuilt binary. Host/GPU acceptance stays
open pending an established safe host condition and its outstanding operations.
Completed tutorial/corpus/regression work was not repeated.


## Resource creation Help and ordinary controls — 2026-10-02

[The bounded control record](Qt6ParityValidation/remaining-gui-controls-20261002.md)
adds visible selected-resource Help/F1 to creation and Clone, repairs active report
search routing and stored OF camera aliases, and implements paired CustomFOV
creation/editing. The four focused checks pass; directly affected shared checks
are recorded without repeating completed audits/tutorials/corpus. One private
actual-input X11 case passes Help/F1/Cancel, paired FOV creation, Save/reopen,
Build and four-row readback. App/WM/Xvfb exits are 0/0/0.

Current GmatQt is 6,008,024 bytes / SHA167efe0f…, with base/util/startup unchanged;
compiled source snapshots and raw failures/corrections remain bound in the record.
The unchanged CustomFOV engine rejects the diagnostic's exact boresight; known
interior/exterior checks pass, without a numerical repair or broad scientific
acceptance claim. Live host testing stays stopped. Mutter's immediate NULL
Wayland-resource fault is established, while GMAT client attribution, repaired
host condition and physical GPU acceptance remain unproven. All nine gates,
external/domain limits and deferred platforms remain in the current map. The
full replacement goal stays active and full qualification is not claimed.


## CustomFOV fallback path lifecycle — 2026-10-02

[The focused path record](Qt6ParityValidation/custom-fov-paths-20261002.md)
repairs pending/detached CustomFOV construction and all five Qt interpretation
routes so configured FOV fallback expressions and aliases survive Help/Cancel,
relative-file creation/Apply, rejection, failed Build/rebuild and Unicode reopen.
The new path check passes 0.31 s; directly affected controls and folder checks
pass 0.35/2.60 s. The 15-failure baseline and corrected compile signature remain
recorded. Current GmatQt is 6,027,456 bytes / SHA02d7711e…; base/startup are
byte-identical and util changes for an additive read-only path-definition getter.
No numerical algorithm, new native scene or host test changed. Exact boresight,
other recorded domain/external limits and host/GPU/client-attribution gates remain
unqualified. Live host testing stays stopped; full qualification is not claimed.


## Separate Default camera reset and current evidence — 2026-10-02

[The camera reset record](Qt6ParityValidation/camera-reset-20261002.md)
implements the actual missing legacy Reset-to-Default operation; existing Script
view already restores Current. Separate primary/named Default metadata, history
and retained reset state survive translation, replay and source transactions.
Toolbar Reset and scoped Space pass the new 0.23 s check and one actual-input
private X11 case. Three affected passing camera checks are reused. Its full
420-byte report remains identical across camera actions, the saved calculation
section is byte-identical and normal owned app/WM/Xvfb exits are 0/0/0.
Build/accessor and test fixture corrections remain recorded.

Current app is 6,075,296 bytes / SHA8d791a3d…; base/util/startup are unchanged from
the FOV path repair. [Native relative-file FOV](Qt6ParityValidation/custom-fov-file-native-20261002.md)
adds actual creation/Imager/Save/reopen/run/report evidence at its recorded
SHA02d7711e… runtime; its helper-induced SIGTERM is explicit. The earlier shared
resource creation/Clone Help and F1 evidence stays accepted without a repeat.
[Rebuilt event Help](Qt6ParityValidation/event-help-startup-kernels-20261002.md)
corrects the obsolete panel kernel instruction and validates the installed page
and startup link without rerunning a completed tutorial.

All nine acceptance gates remain in the current map. Live host testing stays
stopped. Mutter's NULL Wayland event-resource fault is established; GMAT client
attribution, safe/repaired host condition and physical GPU acceptance remain
unproven. Broader recorded scientific/external limits and deferred platforms
remain visible. Full qualification is not claimed.


## Safe local Rename follow-up — 2026-10-03

The blanket include/GMF restriction in the earlier Rename checkpoint is superseded
by [safe local Rename](Qt6ParityValidation/safe-local-rename-20261003.md). Exactly
one main-source declaration and conservative read-only dependency inspection now
allow unrelated literal dependencies. Affected/unresolved external references
still refuse before mutation. The actual app was rebuilt, the directly affected
Rename test passes in 0.62 s and an actual-input F2/Undo/Redo/Save/reopen/run case
reports the independent value10 with external files unchanged and exits0/0/0.
The nine acceptance gates remain in progress; no host/GPU or scientific-wide
qualification is claimed.


## Actual walkthrough usability repairs — 2026-10-03

[Single-step propagation](Qt6ParityValidation/single-step-20261003.md) adds the
ordinary no-stopping-condition operation, preserves source mapping for classified
options-only commands, and keeps typed controls reachable with scrolling. Its
focused known ±600-second checks and affected color check pass on their bound
SHA2c15ffe7… application.

[Coordinate Rename and initial window size](Qt6ParityValidation/axis-command-usability-20261003.md)
resolve native ACE/Lunar/older-Mars failures on GUI-created forwarded axis fields
and LEO's compressed new command child. At this checkpoint the installed app SHA483ed090… was rebuilt; AxisRename0.25s, CommandGeometry0.25s, ResourceRename0.63s and affected
WorkspaceReachability0.36s pass. Actual ACE F2 Rename also succeeds. Original
fixture failures and their corrections are explicit; no numerical engine changed.
The nine acceptance gates, host/GPU stop, external inputs/domain limits and
Windows/macOS/MATLAB deferrals remain in force. These changes do not establish
full replacement qualification or repeat the completed audit/tutorial corpus.


## Six additional Help chapters published; Lunar held — 2026-10-03

[Publication verification](Qt6ParityValidation/help-publication-20261003.md) records a successful build-qt-help build and installed18-chapter Tutorials book, preserving the original12 order/passes and adding only the six independently completed walks. All24 new chapter/subsection pages,327 local references,93 illustration references and four script attachments resolve. Single-page/chunk attachment URLs are separately correct; actual private GUI navigation supplements the filesystem checks. No completed mission or unaffected passing test was rerun for publication.

[Lunar Transfer](Qt6ParityValidation/help-lunar-transfer-20261003.md) remains HOLD and unpublished. Original and one changed LOI finite-difference perturbation case both miss ECC0±1e-5 at the25-iteration bound; final Moon-inertial inclination is about88.07°, short of an exact90° guarantee. Original targets, force models, attempts and results remain preserved. Its incidental early ECC sample-line access is disclosed; the later deliberate comparison followed the original run/reopen freeze. The full seven-additional-chapter requirement therefore remains unmet.

L2 remains held, two MATLAB chapters deferred and two legacy optimizer chapters unresolved. The nine original acceptance gates remain authoritative in the [current map](Qt6ParityValidation/current-acceptance-20261002.md). Live host testing stays stopped; Mutter's NULL Wayland-resource fault is established, while GMAT attribution, safe host condition and physical GPU acceptance remain unproven. External-input/domain limits and Windows/macOS/MATLAB deferrals remain. Full Linux qualification is not claimed.


## Readable Help attachments and Return search — 2026-10-03

[Help usability verification](Qt6ParityValidation/help-attachments-20261003.md) closes two actual new-publication blockers: linked script line/markup loss and Return also activating default Back. Local script/function text retains literal contents, URLs and history; Find stays on its document. Actual GmatQt is rebuilt at6,146,632 bytes / SHA12a9dddf…; new HelpAttachments0.06s and affected Help0.46s pass. A fresh private actual-input display/Return/Back/Forward retry closes normally0/0/0. All failed baselines and intermediate runtime identities remain. No numerical engine/mission or live-host test changed; the nine original gates and held/external/deferred requirements remain.


## Animation segment pauses repaired — 2026-10-03

[The retained-frame replay record](Qt6ParityValidation/playback-segment-gaps-20261003.md)
repairs the actual multi-segment pause reported with Mars B Plane: Current solver
mode discards trial samples but leaves gaps in cumulative callback IDs. Both local
and shared replay now select retained frames, preserving original IDs for cameras,
vectors and curve breaks. The new reproduced failure and five directly affected
passing checks cover gap/tail removal, live invalidation, trimming and reopen.
One affected private offscreen Mars B Plane replay probe passes shared/local
selection and all 100 master advances, with a source-matched byte-identical
solver report. No tutorial construction, corpus or numerical reference rerun.
GmatQt is rebuilt at 6,151,088 bytes / SHAaf0c42cf…; engine/util/startup are unchanged.
Host/GPU/client-attribution and all other nine-gate limits remain explicit. Live
host testing remains stopped; full qualification is not claimed.


## Two playback modes and speed buttons — 2026-10-03

[Interpolated playback evidence](Qt6ParityValidation/interpolated-playback-20261003.md)
records the user-requested Propagation steps / Interpolated steps selection and
shared/local Slower/Faster buttons. Default interpolation advances uniformly
through connected forward simulation time and preserves raw samples/exports;
disconnected/reset boundaries jump, and epoch-less XY/no-forward-time histories
use recorded samples. Seven relevant checks pass, including a freshly owned
Xvfb software-renderer case. One affected Mars B Plane run verifies connected
cadence, both modes and unchanged raw camera/vector/numerical histories, with
a byte-identical source-matched solver report. Initial export-fixture, narrow
layout and native child-lookup corrections remain in the logs. GmatQt is rebuilt
at 6,187,872 bytes / SHAe126f7a1…; base/util/Qt startup remain unchanged.
No completed corpus/tutorial or unaffected passing suite was repeated. The
original nine acceptance gates remain authoritative; live host testing is
stopped, GMAT crash attribution and host/physical GPU acceptance are unproven,
and held/external/domain plus Windows/macOS/MATLAB limits remain explicit.
Full qualification is not claimed.


## Active OF segment-label operation — 2026-10-04

[Segment-label evidence](Qt6ParityValidation/segment-labels-20261004.md) closes
DrawUsePropLabel conversion and the Label Segments resource operation, with
native/fallback replay labels for accepted arcs and merged body trajectories.
Eight relevant checks pass, including a bounded standalone initialization repair;
a new short two-segment fixture has a byte-identical complete report. GmatQt is
rebuilt at 6,230,920 bytes / SHA f2a52efa…; base/util/Qt startup are unchanged.
Retained geometry remains a display approximation after decimation/trimming.
No completed corpus/tutorial/audit was repeated. All nine gates and dependency
limits remain authoritative. Live host testing stays stopped; GMAT attribution,
host repair and physical-GPU acceptance are unproven. Windows/macOS/MATLAB remain
deferred, and full qualification is not claimed.


## Transmitter resource solve-for control — 2026-10-04

[Transmitter SolveFors evidence](Qt6ParityValidation/transmitter-solvefors-20261004.md)
records the reproduced missing-control failure and 0.32 s passing actual-form
check for FrequencyBias/empty-list creation, editing, rollback and source/file
recovery. GmatQt is rebuilt at 6,230,920 bytes / SHA ca4f6f60…; base/util/Qt startup
are unchanged. No estimation mission or completed audit was repeated. This
exact-type operation retains all nine gates, dependency dispositions and the
live-host stop. Host/GPU acceptance and GMAT crash attribution remain unproven;
Windows/macOS/MATLAB remain deferred and full qualification is not claimed.


## Receiver error-model control and remaining OF operations — 2026-10-04

[Receiver error-model evidence](Qt6ParityValidation/receiver-error-models-20261004.md)
records the reproduced omission and passing0.41s actual-form check for ordered
selection, clear/restore, validation, source/file recovery and Rename/Undo. GmatQt
is rebuilt at 6,235,016 bytes / SHA 2ef7d511…; base/util/Qt startup are unchanged.
No completed estimation/GPS mission or unaffected suite was repeated.
The separate [OF source review](Qt6ParityValidation/of-remaining-controls-20261004.md)
identifies active LastN retention and axes/frame labels as remaining operations.
All nine gates and dependency dispositions remain; live-host testing stays
stopped and full qualification is not claimed.


## Independent coordinate-label operations — 2026-10-04

[Coordinate-label evidence](Qt6ParityValidation/coordinate-labels-20261004.md)
closes active AxesLabels/FrameLabel conversion, pending controls and native/
fallback rendering. Four directly affected checks pass, including a corrected
Apply path, exact source/file recovery and a byte-identical new short report.
GmatQt is rebuilt at 6,260,520 bytes / SHA e823c522…; base/util/Qt startup remain
unchanged. No completed audit/tutorial/corpus or unaffected suite was repeated.
LastN remains an active implementation requirement. All nine gates and external/
held scientific dispositions remain; live host testing stays stopped, GMAT
attribution and host/physical-GPU acceptance are unproven, Windows/macOS/MATLAB
remain deferred, and full qualification is not claimed.


## LastN solver display operation — 2026-10-04

[LastN evidence](Qt6ParityValidation/lastn-20261004.md) records source-order
conversion, primary/setup count controls, explicit trial lifecycle and per-object
2×N finalized trajectory retention, with consistent camera/vector/replay cleanup.
New LastNSolver 0.51 s and the affected All/Current/None SolverPlots 1.37 s checks pass.
A new short targeter retains the identical complete 15-row report and 12 accepted
spacecraft points while reducing 78 trial points to 6. Test-only fixture/include
corrections are retained in the logs. Decorative-listener cleanup distribution
and visible in-progress Qt samples are explicit presentation/parity limits.

GmatQt is rebuilt at 6,306,584 bytes / SHA e48df911…; base SHA 5d455c6b… changes only
for the opt-in display lifecycle callback. Util/Qt startup remain unchanged.
All nine gates and held/dependency dispositions remain authoritative; no broad
completed audit/tutorial/corpus was repeated. Live host testing stays stopped,
GMAT attribution and host/physical-GPU acceptance remain unproven, Windows/macOS/
MATLAB remain deferred, and full qualification is not claimed.


## Native vector rendering evidence — 2026-10-04

[Native-vector evidence](Qt6ParityValidation/native-vectors-20261004.md) now verifies
Relative Position and Body-Fixed shafts/heads, Auto/Manual length, color, labels,
replay and invalid/removal behavior using independent projected pixel positions.
The new private Xvfb/llvmpipe check passes1.50s. App/base/util/startup remain unchanged;
no production change, mission or old passing suite was repeated. The prior native
vector pixel pending disposition is superseded at this bounded scope. Active
sensor-mask conversion, editing and display remain a newly identified operation.
Host/GPU acceptance and GMAT client attribution remain unproven; live-host testing
is stopped, all nine gates stay visible and full qualification is not claimed.


## Sensor-mask operations — 2026-10-04

[Sensor-mask evidence](Qt6ParityValidation/sensor-masks-20261004.md) closes the active
OF declaration/conversion, ordered pending editor and FOV display operation for
ConicalFOV, RectangularFOV and CustomFOV. Four new and three directly affected
checks pass, including independent native/fallback pixels, interpolated replay,
reference Rename/recovery and a byte-identical complete new 131-byte report.
Compile and new-test fixture corrections remain preserved. Source removal requires
first removing or reassigning dependent masks; numerical visibility is unchanged.

GmatQt is rebuilt at 6,432,232 bytes / SHA d27a4103…; base/util/Qt startup stay
unchanged. No completed audit/tutorial/example corpus reran. The nine original
gates remain authoritative, live-host testing remains stopped, and GMAT attribution
plus host/physical-GPU acceptance remain unproven. Held scientific/external-data
and deferred Windows/macOS/MATLAB dispositions remain explicit. Full qualification
is not claimed.


## Star display options — 2026-10-04

[Star-option evidence](Qt6ParityValidation/star-options-20261004.md) implements the active
preset/custom star controls, source-order translation, catalog recovery and
native/fallback display. Four new and three directly affected checks pass over
the recorded builds; original accessor/fixture failures, the pending form/count
repair and retained LastN count correction remain explicit. GmatQt is rebuilt at
6,507,840 bytes / SHA 43f55410…; base/util/startup/original catalog are unchanged.
No completed audit/tutorial/corpus is repeated. Remaining active OF translation
preferences are PlaybackTimeScale, TimeSyncParent, ShowToolbar and DrawModelFile,
with source evidence in the linked continuation queue. Keep all nine gates,
held scientific/external cases, deferred platforms/MATLAB and the stopped live-host
testing boundary. GMAT client attribution and host/physical-GPU acceptance remain
unproven. Full qualification is not claimed.


## Saved simulation-clock and toolbar options — 2026-10-04

[Playback-option evidence](Qt6ParityValidation/playback-options-20261004.md) now covers
PlaybackTimeScale, TimeSyncParent and ShowToolbar, superseding those three pending
preferences in the preceding checkpoint. Pending source controls, parent validation,
Rename, shared absolute epochs, signed/zero rates, both sampling modes and reachable
toolbar restoration have eight new/five directly affected passing checks across
the recorded builds. Private native/fallback pixels and a new backward/forward
mission validate the presentation path; the complete 315-byte report and retained
sample serialization remain identical through playback. The finite historical
camera query regression and test-fixture corrections are recorded with first failures.

GmatQt is rebuilt at 6,718,016 bytes / SHA 6cfca843…; base/util/startup
are unchanged. Qt retains progressive history and compact raw endpoints, without
claiming a reconstruction of every discarded trajectory sample. Per-plot DrawModelFile
remains active unmet scope. All nine gates, held scientific/external dependencies,
deferred Windows/macOS/MATLAB and stopped live-host testing remain in force.
GMAT client attribution and host/physical-GPU acceptance remain unproven.
Full replacement qualification is not claimed.
