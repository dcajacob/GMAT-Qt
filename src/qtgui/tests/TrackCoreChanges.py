#!/usr/bin/env python3
"""Preserve reviewable engine diffs; inventory evidence, never certify physics."""
import argparse
import datetime
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[3]
SCOPES = ['src/base', 'src/gmatutil', 'plugins']
DESCRIPTIONS = {
    'SolarRadiationPressure.cpp': ('Physical plate-torque direction', 'Remove the extra torque force-direction negation: signed projected area already supplies the illuminated-face sign,as in local TorqueModeling PlateTorque/ReflectanceVector equations. Nonzero absorbing-plate moments and summed torques require independent checks;general optics,shadowing,orientation/provider science remains unqualified. No member,API,slot,translational force or eligibility change.'),
    'SpaceObject.cpp': ('Scoped samples and consistent epoch property writes', 'Retain prior PlotOriginScope sample-state reads. Synchronize the existing GmatTime epoch when the integer epoch property setter writes its Real counterpart,matching the existing high-precision and Spacecraft string-property APIs. No new member,virtual slot,time conversion,owner role or writable-eligibility change. Strict current owner/private/Global and internal Formation/member clock roundtrips are bounded;broader time-scale/leap/provider/propagation science and non-Spacecraft domains remain open.'),
    'AttitudeData.cpp': ('Native MRP Parameter setter dispatch', 'Route existing MRP1/2/3 SetReal identifiers to the actual attached Attitude scalar setters,matching other orientation fields. Original advertised writable flags,roles,conversions,owner storage and virtual slots remain unchanged. Strict live native non-idempotent mutation,matrix and restore controls cover main/private/Global owners;broader attitude/frame/propagation science remains open.'),
    'ElementWrapper.cpp': ('Rvector assignment dispatch', 'Forward an evaluated Rvector RHS to the existing Rvector LHS wrapper setter. Preserve existing string/matrix paths and downstream object dimension validation;no wrapper/member/virtual-slot or factory/eligibility changes. Original Quaternion self-assignment failure and dynamic ThrustAngle1/2 lengths1/3/4 source/target controls are retained;arbitrary vector/domain contracts remain open. Also transfer bracketed numeric Rmatrix literal commands through the existing sized matrix setter,matching resource initializers; reject wrong counts,non-numeric/non-finite entries and unsupported RHS before changing the destination. No metadata/member/slot change;rectangular transfer and failure atomicity need explicit checks.'),
    'HardwareParameters.cpp': ('Derived thruster unit labels', 'Replace three empty constructor Unit strings with N,s,kg/s for ThrustMagnitude,Isp,MassFlowRate,matching native quantities and the calculation guide dimensions. No formula,conversion,setter,IsSettable,owner,member,slot or factory/role change. Active/stopped main/private/Global owner and separate/cumulative fuel,duty/scale controls are bounded;general propulsion/frame/time/provider science and writable-contract discrepancies remain open.'),
    'Assignment.cpp': ('Function equation destination registration', 'Include one-dot equation destinations in the wrapper-name inventory,matching literal assignments,so the interpreter creates left-only time Parameters before function scope cloning. No equation formula,wrapper setter,ownership member,virtual slot or eligibility change. Original absolute epoch failures and related self-referenced/time/scalar GUI controls remain source-versioned and subject to technical review. Validate fixed bracketed numeric matrix literals against the actual destination dimensions; preserve the existing owner field vs read-only calculation Parameter distinction. No readonly eligibility change.'),
    'ThrustSegmentRvector.cpp': ('Thrust-angle owner references and widths', 'Bind supported ThrustSegment type rather than its primary Interface category on AddRef/SetRef,invalidate the cached owner and synchronize existing vector size/cache from the actual owner. No new data/member/slot or factory/classification change.'),
    'ThrustSegmentParameters.cpp': ('Thrust-angle vector lifetime and dispatch', 'Own by-value coefficient vectors during evaluation,resize existing caches,retain the third-component sentinel when present,override existing const setter slots while preserving mutable overloads,and delegate assignment to the parent instead of recursive self-calls. Lengths1/3/4 and repeated/nested/private/Global protocols are bounded;thrust-history force and estimation science remain open.'),
    'ThrustSegmentParameters.hpp': ('Thrust-angle const setter overrides', 'Declare const-reference SetRvector overrides for the two advertised angle types,preserving both original mutable virtual overloads and all data members. No added virtual slot or class-layout member.'),
    'BurnParameters.cpp': ('Finite-burn unit and readonly metadata', 'Correct the seven total-flow/acceleration/thrust parameters to kg/s,km/s^2,N and IsSettable=false,matching the calculation reference and existing unsupported setter implementations. Preserve all321 factory names,readable/reportable/plottable owner routes and numerical values;actual destination omission/typed rejection,Report/RHS readability,atomic native setter refusal and unchanged active/function/writable-impulse regressions remain subject to technical scrutiny. No member,virtual slot,unit conversion or formula change.'),
    'ManageObject.cpp': ('Global owner parameter promotion', 'Insert each automatic parameter under its own full name when its owner becomes global. The owner key is already occupied;using it silently discarded the parameter insertion while erasing its local name. Preserve the existing local-shadow/duplicate guards,parameter identity and ownership flags. No new member,virtual slot,formula or parameter eligibility change;bounded live-map/ref-owner evidence and original Global regressions remain subject to technical review.'),
    'Function.cpp': ('Read-only function solar-model context', 'Return the existing solarSys pointer through a const nonvirtual GetSolarSystem accessor so native scope evidence can compare the actual function model with the live spacecraft/sandbox model. No new member,virtual slot,ownership or execution change;diagnostic API remains subject to technical review.'),
    'Function.hpp': ('Read-only function solar-model API', 'Declare the const nonvirtual GetSolarSystem accessor. Existing object layout,virtual interface,ownership,provider initialization and numerical behavior are unchanged.'),
    'GetLastState.cpp': ('Built-in object binding and formal output', 'Read the actual Spacecraft reference bound by the input element wrapper instead of a distinct formal placeholder. Update the existing formal Array output when present so function-frame wrappers see the returned six components; retain the legacy detached-output path when no formal output exists. Typed null/input/output guards; no state formula, object member or virtual slot changes. Literal native input/output identity controls, main/direct/nested function calls and the unchanged main-menu regression give bounded evidence; general ownership,frames,propagation and science remain unqualified.'),
    'ObjectManagedFunction.cpp': ('Read-only function diagnostics', 'Expose the existing global object-store pointer through a const nonvirtual accessor for identity inspection. No data member or virtual slot; new public diagnostic API remains subject to technical review.'),
    'ObjectManagedFunction.hpp': ('Read-only function diagnostics API', 'Declare GetGlobalObjectStore returning const ObjectMap*. Existing object layout, ownership and virtual interface are unchanged.'),
    'UserDefinedFunction.cpp': ('Borrowed function-object cleanup', 'Skip identities recorded as sandbox-borrowed before dereferencing function template entries during cleanup. The sandbox can already have destroyed an automatic parameter also present in the template map; do not delete or inspect that borrowed object a second time. No member, formula or ownership transfer.'),
    'ObjectPropertyWrapper.cpp': ('Reference-list output', 'Serialize an object-array property as its ordered reference-name list when it does not select an attached object. Preserve the existing attached-object serialization branch; no reference binding, mutation or numerical-model change.'),
    'OrbitRvec6.cpp': ('Vector parameter reference initialization', 'Construct OrbitData with the actual name,owner type and dependency;bind the current Parameter during initialization and forward the existing reference-name-array virtual interface, matching OrbitReal. ObjectInitializer can bind the actual owner/frame/solar references. No new member or conversion formula.'),
    'OrbitRvec6.hpp': ('Vector parameter reference initialization', 'Declare the inherited reference-name-array override;no new member or virtual slot. Broader owner/frame and scientific qualification remains open.'),
    'ParameterFactory.cpp': ('Advertised parameter construction', 'Connect the existing advertised ModEquinoctial,IncomingAsymptote,OutgoingAsymptote,BrouwerMeanShort and BrouwerMeanLong names to their existing vector parameter classes. Preserve the factory list and all existing scalar routes;no conversion formula change.'),
    'Rvec6Var.cpp': ('Fixed-size vector assignment contract', 'Bridge the inherited generic vector evaluator to the existing six-component evaluator and expose read-only integer VectorSize=6 metadata used by Assignment shape validation. No new data member or numerical formula;new metadata remains subject to technical review.'),
    'Rvec6Var.hpp': ('Fixed-size vector assignment contract', 'Declare overrides of inherited vector/metadata methods and a parameter-ID enum for read-only VectorSize;no new object member or virtual slot. Complete parameter science and full-build qualification remain open.'),
    'FirstOrderGaussMarkov.cpp': ('Polymorphic resource copying', 'Override the existing virtual Copy contract using the existing typed assignment operator and atomic null/incompatible-type rejection. This enables whole-object assignment/function outputs while preserving destination identity and all model/base state. No process-noise formula or object-layout change;focused copy-response and resource tests are recorded separately.'),
    'FirstOrderGaussMarkov.hpp': ('Polymorphic resource copying', 'Declare the Copy override inherited from GmatBase;no new member or virtual slot. Other ownership/scientific/full-build qualification remains open.'),
    'Sandbox.cpp': ('Function execution context', 'Bind every GMAT function control sequence to this sandbox\'s trigger/event-manager clones, including previously built sequences on reuse. Existing branch setters forward the same context to children. No ownership, object layout, virtual API or numerical formula changes. Focused function-local simulation/filter/smoother, nested/expression and existing function/navigation checks are recorded separately; broader function/resource and full-build qualification remain open.'),
    'EphemerisFile.hpp': ('Nested final-solution output API', 'Override inherited SetRunState to apply the shared nested subscriber acceptance filter; no new data members. Selected OEM contexts are verified, broader formats and output regimes remain open.'),
    'SolverBranchCommand.cpp': ('Nested subscriber acceptance', 'Expose the existing active solver-state scan through GetSubscriberRunState and retain GetPlotRunState as a delegating entry point. EphemerisFile opts in; publisher and numerical solver state remain unchanged.'),
    'SolverBranchCommand.hpp': ('Nested subscriber acceptance API', 'Declare a static shared classifier while preserving the existing plot API; no object layout or virtual slot change in this class.'),
    'CoordinateConverter.cpp': ('Covariance orientation', 'Add explicit rotation-only composition for covariance components, including FK5/ICRF alignment. ObjectReferenced geometry avoids unused force/rate queries; ordinary state conversion and strict rate guards remain unchanged. General frame/platform/scientific qualification remains pending.'),
    'CoordinateConverter.hpp': ('Covariance orientation API', 'Declare nonvirtual A1Mjd/GmatTime/Real rotation-only helpers; no object layout or vtable changes.'),
    'ObjectReferencedAxes.hpp': ('Covariance orientation API', 'Declare nonvirtual geometry-only orientation and common frame calculation; no new members or virtual slots.'),
    'ProcessNoiseBase.cpp': ('Covariance orientation', 'Request orientation directly in both process-noise time overloads instead of an unused six-state/rate conversion. Retain covariance block rotation convention and all noise equations. Broader covariance scientific qualification remains pending.'),
    'Spacecraft.cpp': ('Origin-relative state preservation', 'Retain small Cartesian input/integrated states alongside unchanged ordinary high coordinates; bind to backing six-state, precise epoch, origin and fixed internal-frame identities, preserve copies, retain precision toggles and allow celestial clone rebinding only with exact name/ephemeris equality;reject stale/nonfinite data. Forget object-frame force-provider bindings on destruction to prevent stale identity reuse. Broader frame/body mutations and physical domains remain unqualified.'),
    'Spacecraft.hpp': ('Origin-relative state API and storage', 'Add guarded origin-relative Cartesian state metadata and nonvirtual methods. Spacecraft layout changes require rebuilding source plugins; proprietary binary layout compatibility is not blanket qualified.'),
    'ODEModel.cpp': ('Origin-relative propagation handoff', 'Preserve the actual local integrated state when mapping to objects and reuse only matching spacecraft/backing/epoch/origin states on command handoff. Existing absolute state translation and integration equations remain unchanged. Add a guarded object-frame acceleration bridge to the active configured forces at the accepted spacecraft identity, epoch and MJ2000Eq origin; reject unsupported/erroring/recursive/nonfinite providers. Preserve model/spacecraft lifetime, initial-report and command-handoff identity; legacy global acceleration getters remain unchanged.'),
    'ODEModel.hpp': ('Object-frame acceleration API', 'Declare nonvirtual/static force-context binding and query methods without changing ODEModel layout or virtual slots; general force/attitude coupling remains unqualified.'),
    'Propagate.cpp': ('Active object-frame acceleration handoff', 'Select the current numerical force provider after actual command initialization/clone replacement, preserving virtual Initialize dispatch. Suspend contexts on non-ODE propagation; initial sandbox setup does not override an earlier command provider.'),
    'Propagate.hpp': ('Propagation initialization helper', 'Declare a protected nonvirtual scoped initialization helper; class layout and virtual slots remain unchanged.'),
    'TimeData.cpp': ('Report sample epoch', 'Use the detached canonical epoch only inside report parameter sample scope; native spacecraft and all other time reads remain unchanged.'),
    'DragForce.cpp': ('SPAD drag acceleration query', 'Use speed squared times the SPAD effective-area vector in GetDerivativesForSpacecraft, matching propagation and the vector drag law. The prior componentwise velocity multiplication changed direction and magnitude. Spherical and integration branches remain unchanged; arbitrary attitude, geometry and atmospheric domains remain unqualified.'),
    'Publisher.cpp': ('Precise publication context', 'Add synchronous Real-packet GmatTime metadata with pointer/count/MJD identity,thread isolation,nested legacy/precise scope restoration andunchanged existing virtual Publish dispatch. Data arrays andsubscriber ABI stay unchanged;asynchronous consumers must copy metadata inside their callback. Add immutable, packet/object-identity-bound Cartesian sidecars with nested/thread/exception isolation and explicit report-only parameter scope; ordinary packets and live objects remain unchanged.'),
    'Publisher.hpp': ('Precise publication API', 'Add nonvirtual PublishWithEpoch andGetPublishedEpoch methods while preserving existing virtual Publish overloads andobject layout. Declare nonvirtual Cartesian sidecar and scoped report parameter APIs without changing Publisher layout or existing virtual slots.'),
    'EphemerisFile.cpp': ('SPK packet epoch handoff', 'Copy the current matching publication epoch intoSPK buffering. Other ephemeris formats retain existing handling;general coherent state sampling is not implemented. SPK alone retrieves the shared detached Cartesian sample and matching precise epoch; other formats and original packet/covariance/attitude remain unchanged. Opt into shared nested subscriber acceptance classification so an inner accepted pass is suppressed while an enclosing solver is still iterating; publisher state and physical calculations are unchanged. Two nested OEM cases and affected plot/flat-output regressions qualify selected paths only.'),
    'EphemWriterSPK.cpp': ('Precise SPK buffering', 'Retain precise epochs/legacy flags beside unchanged states throughsegment overflow,shared boundaries,reset andfinalization. No state orintegrator trajectory adjustment.'),
    'EphemWriterSPK.hpp': ('Precise SPK buffer metadata', 'Add transient current publication epoch andparallel epoch/validity vectors forSPK state records.'),
    'SpiceInterface.cpp': ('Precise SPICE clock conversion', 'Convert GmatTime A1 to TDB, subtract exact J2000 origin before one double-seconds conversion; retain old entrypoints unchanged. A new overload locally inverts the existing nonlinear mapping at the rounded ET cell to expose the output sampling epoch. Production state sampling and independent time-scale truth remain open.'),
    'SpiceInterface.hpp': ('Precise SPICE clock API', 'Add nonvirtual GmatTime conversion overloads without changing the existing virtual Real entrypoint. The canonical-epoch overload returns the precise A1 instant represented by the encoded ET; it does not sample states or retag live data.'),
    'SpiceOrbitKernelWriter.cpp': ('Precise SPK record clock', 'Use precise conversion only forflagged matching records;legacy records retain old conversion. Descriptor endpoints share the corresponding record encoding;states andinterpolation degree remain unchanged. Validate structural/finite/time-order metadata before opening output;use scoped arrays forfailure cleanup andclose empty DAF handles after rejected first segments.'),
    'SpiceOrbitKernelWriter.hpp': ('Precise SPK writer API', 'Add a precision-metadata overload andshared implementation while retaining the existing WriteSegment signature.'),

    'SolarFluxReader.cpp': ('Atmosphere weather history', 'For CSSI-predicted Ap inputs, load the current calendar day and each of the previous three days before constructing the NRLMSISE seven-element lag-window array. Fixes four repeated previous-day records. Synthetic all-slot forecast/history and weather-choice GUI witnesses are bounded input-delivery evidence; empirical density/drag and weather provenance remain unqualified.'),
    'Burn.cpp': ('Maneuver frame basis', 'After rotation from a named coordinate system to its own base, rotate ICRF-based free delta-V vectors into the internal FK5/MJ2000Eq axes. No origin translation or change to existing FK5/local/body branches. Selected asymmetric impulses at two epochs are acceptance witnesses; broad maneuver/ephemeris/frame qualification and technical approval remain pending.'),
    'Thruster.cpp': ('Finite-thrust frame basis', 'After rotation from a named coordinate system to its own base, rotate ICRF-based free thrust directions into the FK5/MJ2000Eq derivative basis. Existing FK5/local/body branches and thrust/mass-flow models are unchanged. Selected constant-mass directional integrations at two epochs are witnesses; broader force/frame regimes and technical approval remain pending.'),
    'BranchCommand.cpp': ('Nested command trigger managers', 'Forward the sandbox trigger-manager vector recursively to branch children using the stored next link. Reentrant estimator witness exercises a RunEstimator inside a For loop; arbitrary branch/function/event combinations remain unqualified.'),
    'BranchCommand.hpp': ('Nested command dependency API', 'Override the existing virtual SetTriggerManagers method to forward event-manager dependencies to branch children. No data member or new virtual slot is added.'),
    'BatchEstimatorBase.cpp': ('Batch final solution output', 'After convergence only, propagate the final fitted initial conditions to the last observation epoch without reprocessing observations or correcting the fit. Preserve and restore participant objects and the fitted vector on completion/failure/finalization; retain iterations and fit statistics. General estimator/force/multiple-participant/reentry/stop regimes require qualification.'),
    'BatchEstimatorBase.hpp': ('Batch final pass state', 'Own the output-pass flag, terminal epoch, fitted state and participant snapshot buffer; changes estimator layout and requires dependent rebuilds.'),
    'BatchEstimator.cpp': ('Batch covariance validation', 'Before Internal inversion, validate a detached reduced normal matrix using the existing Cholesky positive-definiteness check already used by Schur. Retain Internal inverse values and all observation, editing and update equations. A colocated two-epoch rank-deficient input previously completed with invalid covariance; the unchanged ordinary rejection/recovery case now passes. Broader conditioning and estimator science remain unqualified.'),
    'RunSimulator.cpp': ('Simulator command reentry', 'On entry after FINISHED, rebuild the command-local Simulator and PropSetup clones through the existing Initialize path, then Reset the fresh state machine. Resetting only the old state machine duplicates stale measurement adapters and crashes; doing neither silently reuses the previous observation file. Three nested batch/outer solver cases, independent each-final-trajectory checks, no-Build replay and affected regressions verify selected paths. Event/light-time, arbitrary force/noise and general simulator science remain unqualified.'),
    'RunEstimator.cpp': ('Batch final pass publication and command reentry', 'Publish fitted initial conditions with synchronized propagation clocks before output propagation and suppress the mistimed reset-state publication after every batch termination, including iteration limits without a final pass. Reject numerical spacecraft STMs with fewer than six Cartesian rows before force evaluation; the existing state-solve-for prerequisite remains. Reset each fresh command-local solver clone to INITIALIZING, and clear propagation reuse/new-pass flags before replacing owned PropSetup clones on every invocation. This prevents a retained FINISHED state from using an uninitialized new ODE model. Broader estimator/force/stop/error regimes remain unqualified. Forward the existing precise final-pass epoch in packet-scoped publication metadata;Cartesian arrays andlive spacecraft epochs/states are unchanged. SPK tag/state consistency remains a separate open gate. For batch final passes with active SPK targets, create canonical-ET Cartesian samples from detached spacecraft force evaluations; preserve live epoch/state, publish samples as sidecars, reject unsupported/nonfinite derivative paths. General discontinuity/covariance/attitude transport remains unqualified.'),
    'Estimator.cpp': ('Derived Keplerian report availability', 'Request rotation-only VNB covariance orientation instead of calculating an unused acceleration-dependent rate; retain covariance block convention and smoother state selection. Preserve Cartesian estimation and covariance reporting when the classical Keplerian report conversion is singular. Catch only Rmatrix::IsSingular at the derived text-report inverse: emit explicit unavailable/N/A uncertainties and keep Cartesian results. Fit/force/normal-equation math and other exceptions are unchanged; singular MAT/JSON outputs and arbitrary regimes remain unqualified.'),
    'UpdateDynamicData.cpp': ('Command parsing', 'Preserve array-index commas while separating selected display parameters; normalize index whitespace and reject unbalanced parentheses. Native built-in output and live display witness covers one selected array element, not general command/science qualification.'),
    'Yukon.cpp': ('Solver constraint identity and recovery', 'Retain original nonlinear rows across QP duplicate reduction; normalize QP copies, restore original-row duals and check every original bound. Route contradictory parallel linearizations through elastic mode, resize expanded storage, preserve safe QP ownership and check unrelaxed original functions at the requested feasibility tolerance before accepting convergence. Retain the previous valid SelfScaledBFGS Hessian when finite precision destroys positive definiteness in an update. Registered tight/wide infeasibility, same-process feasible recovery, twelve analytic companions and four optimizer GUI contracts supply bounded evidence; arbitrary nonlinear problems remain unqualified.'),
    'Optimize.cpp': ('Solver report-state handoff', 'Refresh owned Vary physical reporting caches from the restored nominal decision vector before an internal solver checks convergence and writes final output. Traverse ordinary child branches and leave nested solver ownership separate. No mission-variable assignment or extra solver evaluation; original VF13ad report truth, byte-identical trial/final CSVs, forward/central two-variable scaling contracts and affected Yukon GUI checks provide bounded evidence. General solver/provider/platform qualification remains open.'),
    'Vary.cpp': ('Solver report-state transformation', 'Add a reporting-only action using the existing physical transformation scaled/multiplicative-additive. Update only the unscaled solver report cache, without assigning varied objects, notifying subscribers, rewriting source or altering decisions. No object layout or virtual-interface change; technical approval remains pending.'),
    'NLPFunctionGenerator.cpp': ('Solver elastic feasibility', 'Include the final elastic slack component in the maximum absolute slack check; independently assert first, last and magnitude boundaries. Return zero for the empty user-constraint and elastic-variable sets rather than indexing missing elements; original Debugger solver assertions and bounded/unbounded empty-set probes cover this repair. Broader Yukon domains remain unqualified.'),
    'Interpolator.cpp': ('Interpolation input validation and copying', 'Reject a direction reversal before buffer mutation; preserve direction across Clear/copies, initialize copied owned pointers and restore buffered counters after allocation. Copy previousX during assignment. Increasing/decreasing inputs remain valid; duplicate/nonfinite output policies are unchanged.'),
    'Interpolator.hpp': ('Interpolation validation state', 'Declare the independent-input direction used by the shared atomic monotonicity guard; this changes interpolator object layout and requires rebuilding dependent libraries/plugins.'),
    'LinearInterpolator.cpp': ('Interpolation direction', 'Select the bounding interval symmetrically for decreasing data; preserve increasing-data interpolation and extrapolation policy.'),
    'LagrangeInterpolator.cpp': ('Interpolation copying', 'Copy populated native and cached arrays instead of erasing the copied ring; copy base configuration/direction during assignment; preserve empty-copy initialization.'),
    'CubicSplineInterpolator.cpp': ('Interpolation copying', 'Initialize scratch ownership before copying populated buffers/coefficients, and preserve base configuration/direction during assignment.'),
    'NotAKnotInterpolator.cpp': ('Interpolation copying', 'Initialize scratch ownership before copying populated buffers/coefficients, and preserve base configuration/direction during assignment. Value-initialize scalar h/A caches so cold copies and assignments do not read indeterminate doubles before the first spline build; release and unoptimized cold-copy diagnostics plus independent cubic continuation qualify the selected paths. Broader interpolation domains remain open.'),
    'FileUtil.cpp': ('Function output metadata', 'Stop first-function output declaration/type/shape/global scanning at the next function definition, matching the native warning-and-ignore rule; ignored declarations cannot reject or rescue the first function.'),
    'Save.cpp': ('Object export', 'Include attached runtime tank/thruster graphs before spacecraft, deduplicate identical named references and reject divergent same-name states before opening files; export current strings through detached literal snapshots; preserve naming and existing file-failure policy.'),
    'ConditionalBranch.cpp': ('Conditional validation', 'Reject nonfinite numeric literal wrappers when binding conditional operands; runtime parameter comparisons and operator precedence are unchanged.'),
    'For.cpp': ('Control flow', 'Check zero/wrong-direction loop steps before dividing and converting the iteration count; preserve warning, skip and valid-loop behavior.'),
    'EclipseLocator.cpp': ('Event report output', 'Append per-eclipse-type individual interval counts, maximum duration and maximum-event participants; retain existing event table and union totals; no event search/dynamics formula changes.'),
    'EclipseEvent.hpp': ('Event report API', 'Expose a const readonly eclipse type accessor for per-type report aggregation; no object layout or virtual interface change.'),
    'OrbitData.cpp': ('Parameter setters', 'Normalize the advertised RADN destination through its existing RAAN inverse (+180 getter), before reusing the existing direct/frame/origin setter paths; no new conic formula or writable flag. Delegate RMAG/VMAG to actual Spacecraft RMagnitude/VMagnitude fields in both direct spherical branches; conversion formulas unchanged. During explicit report sample scope, compute Cartesian-derived parameters from the detached sampled epoch/state; all other evaluation retains live state.'),
    'Report.cpp': ('File output', 'Honor ReportFile delimiter/fixed-width settings and minimum name widths for explicit Report command headers.'),
    'StateConversionUtil.cpp': ('Input validation and Planetodetic inverse geometry', 'Use uppercase position/velocity magnitude labels after uppercasing the input so existing magnitude validation executes. For exterior spheroids, intersect the geodetic normal with the requested radius exactly, replacing the second-order flattening approximation and near-pole shortcut. Preserve the interior/invalid-shape legacy branch and every other conversion function; interior/evolute and general-domain qualification remain open.'),
    'EphemerisPropagator.cpp': ('Propagation', 'Require finite positive user StepSize; internal/backward steps remain signed.'),
    'BulirschStoer.cpp': ('Propagation', 'Extrapolation depth, signed remaining intervals, temporary final-step minimum and warning-policy acceptance.'),
    'PredictorCorrector.cpp': ('Propagation', 'Direction, temporary final-step/starter bounds and permitted minimum-step error acceptance.'),
    'RungeKutta.cpp': ('Propagation', 'Temporarily lower minimum-step floor for a final event interval, restoring it on exit.'),
    'RungeKuttaNystrom.cpp': ('Propagation', 'Use the shared RungeKutta interval driver while retaining virtual Nystrom state steps.'),
    'BodyFixedAxes.cpp': ('Coordinates', 'Include user-body pole-angle rates in the frame rotation derivative.'),
    'GeocentricSolarMagneticAxes.cpp': ('Coordinates', 'Retain converted epoch precision through sidereal helpers.'),
    'MJ2000EcAxes.cpp': ('Coordinates', 'Construct the ecliptic rotation from84381.448 arcsec mean obliquity.'),
    'ObjectReferencedAxes.cpp': ('Coordinates', 'Add geometry-only orientation on a temporary frame copy, preserving live frame/rate caches and making no force query. Normal rate calculations still use guarded actual dynamics. Reject nonorthogonal rotations instead of constructing an unused warning. Honor the supplied relative acceleration in velocity/orbit-normal derivatives instead of overwriting it with zero;reject nonfinite acceleration before updating the last valid rotation. Use the guarded active-force acceleration bridge for propagated spacecraft; retain legacy fallback only for objects without a bound provider. Native VNB and targeted generic-force/lifetime/handoff checks pass; global acceleration APIs, unsupported force families and implicit force/attitude coupling remain unqualified.'),
    'TopocentricAxes.cpp': ('Coordinates', 'Use zero eccentricity for a Sphere horizon reference.'),
    'CoordinateSystem.cpp': ('Metadata', 'Expose Origin reference object type as SPACE_POINT.'),
    'CelestialBody.cpp': ('Coordinates', 'Compute user-relative orientation epoch differences before adding large Julian offsets.'),
    'ChemicalThruster.cpp': ('Force model', 'Reject nonfinite computed thrust/Isp before force and mass derivatives.'),
    'GravityField.cpp': ('Force model', 'Allow TideModel metadata choices for all bodies; data/setter validation still applies.'),
    'HarmonicGravity.cpp': ('Input and force model', 'Validate tide extension/readability/coefficients and reject empty/incomplete/nonfinite tide data.'),
    'PolyhedronBody.cpp': ('Portability', 'Protect std::min/max calls against platform macros.'),
    'GmatFunction.cpp': ('Function execution', 'Service pause/stop control polling every50 function commands during mission execution. Rebuild references on function-local ODEModels after prototype reset and before Parameter reinitialization on repeated calls. Restore SolarSystem/frames using existing ObjectInitializer APIs; leave propagation-size initialization demand-driven. After resetting function-local SpacePoints from fresh prototypes, rebind their borrowed SolarSystem/J2000 body before command initialization; repeated SPK Propagate can query that state before Execute. Missing body references remain errors; no new member or virtual slot.'),
    'FunctionManager.cpp': ('Function execution', 'Resolve nested callee outputs, caller destinations and indexed Array owners through local-first/global-second lookup. Borrow actual global outputs across local-frame restoration without cloning/deleting global identities; avoid same-object Copy. Save/restore recursive local value frames, capture callee outputs before restore and clean exceptional frames. Resolve indexed caller output owners and reapply scalar array-element returns after frame restoration. Allocate builtin formal output storage for unnamed math-expression results as well as named output calls. Preserve caller non-Global Solver name-cache statuses around user-defined calls, including scope unwinding, so private same-name solver results do not leak into the caller; explicit Global solver results stay shared. Selected scalar/indexed/nested builtins and bounded solver-cache normal/error/recovery paths are witnessed; arbitrary wrappers, recursion and exception combinations remain open.'),
    'FunctionManager.hpp': ('Function execution', 'Declare owned recursive local-value snapshot stack.'),
    'ScriptInterpreter.cpp': ('Interpretation', 'Clear function context/mode and restore continue-on-error after function parse exceptions; reset file-local definition/ignore-rest flags before interpreting a new main script.'),
    'Moderator.cpp': ('Interpretation', 'Clear currentFunction when function interpretation throws.'),
    'Interpreter.cpp': ('Interpretation', 'Validate direct-field command-mode restrictions before eager automatic parameter creation. Dispatch named resolved BuiltinGmatFunction resources by object kind across bracketed, ordinary-output and no-output calls while preserving registered-name fallback and user/Matlab/Python routing. General name-shadow conflicts remain unqualified. Honor anexplicit empty PrimaryBodies list byclearing harmonic gravity before thecreation loop;otherwise thedefault Earth field survives {}. Nonempty-primary andpoint-mass list behavior unchanged.'),
    'MathParser.cpp': ('Math callable discovery', 'Recognize declared Function objects in local or sandbox-global stores before retaining the existing same-name GMF-file fallback for compound expressions. Native factories and arithmetic formulas are unchanged; undeclared builtin expressions and arbitrary collisions remain unqualified.'),
    'Validator.cpp': ('Interpretation', 'When an isolated parser has no cached parameter owner, use an unmanaged factory prototype to obtain declared owner metadata before resolving same-name direct fields. Preserve known read-only parameter semantics; resolve valid direct resource fields when a same-name system parameter requires a different owner type, including ImpulsiveBurn.Isp. Permit only the direct writable Spacecraft.OrbitErrorCovariance mission field to use its native matrix setter; keep qualified calculation parameters and other read-only names on their existing path.'),
    'CallFunction.cpp': ('Interpretation', 'Reject known read-only system parameters as caller output destinations.'),
    'SpiceOrbitKernelReader.cpp': ('SPICE interface', 'Resolve numeric frame IDs to frame names for rotation/state transforms.'),
    'DifferentialCorrector.cpp': ('Solver', 'Do not report maximum-iteration failure after convergence on the last permitted iteration.'),
    'ReportFile.cpp': ('File output', 'Use native UTF-8 Windows paths; preserve inactive unused report files and truncate stale bytes after final solver row replacement. Windows and broader file/solver qualification remain separate. Opt into detached Cartesian/time parameters only while evaluating subscriber report wrappers; exception-safe scope leaves other consumers on their original packet.'),
    'XyPlot.cpp': ('Subscriber', 'Notify receiver on ToggleOff/ToggleOn so unpublished gaps do not become joined segments.'),
    'GroundTrack.cpp': ('Portability', 'Protect numeric_limits::max against platform macros.'),
    'CustomFOV.hpp': ('API visibility', 'Export CustomFOV with GMAT_API.'),
    'FieldOfView.hpp': ('API visibility', 'Export FieldOfView with GMAT_API.'),
    'StringUtil.cpp': ('File input', 'Accept non-ASCII filename bytes while rejecting reserved/control bytes; use unsigned char for isalpha.'),
}


# Preserve earlier descriptions while documenting this explicit Copy increment.
for _copy_class in ['AcceptFilter', 'CustomFOV', 'DragForce', 'ExponentialAtmosphere', 'ExternalModel', 'FileInterface', 'FiniteThrust', 'GravityField', 'Imager', 'JacchiaRobertsAtmosphere', 'MessageWindow', 'Msise86Atmosphere', 'Msise90Atmosphere', 'PointMassForce', 'RejectFilter', 'RelativisticCorrection', 'SNCProcessNoise', 'SolarRadiationPressure', 'StopCondition', 'ThrustHistoryFile', 'ThrustSegment', 'TrackingFileSet']:
    for _copy_suffix in ('.cpp', '.hpp'):
        _copy_file = _copy_class + _copy_suffix
        _category, _purpose = DESCRIPTIONS.get(_copy_file, ('Function-private resource reset', ''))
        _extra = (' Add an inherited Copy override with exact-class/null/incompatible guards and self-copy handling;delegate to the existing typed assignment.' if _copy_suffix == '.cpp' else ' Declare the inherited Copy override;no new data member or virtual slot.')
        if _copy_class == 'MessageWindow' and _copy_suffix == '.cpp':
            _extra = ' Copy Subscriber state and precision with an empty transient stream configured like the copy constructor;guard incompatible/null/derived sources and self-copy.'
        if _copy_class == 'Imager' and _copy_suffix == '.cpp':
            _extra += ' Typed assignment now copies Hardware base state before preserving its existing borrowed FOV reference/name.'
        if _copy_class == 'RelativisticCorrection' and _copy_suffix == '.cpp':
            _extra += ' Typed assignment clones nullable owned frames through scoped temporaries and releases previous destination frames;no force formula changes.'
        DESCRIPTIONS[_copy_file] = (_category, (_purpose + _extra).strip())
DESCRIPTIONS['Validator.cpp'] = (DESCRIPTIONS['Validator.cpp'][0], DESCRIPTIONS['Validator.cpp'][1] + ' Resolve a declared function-local owner before the caller validation map for direct-field/system-parameter collisions;matching-owner read-only semantics remain.')

for _attitude_class in ['CSFixed','Spinner','PrecessingSpinner','NadirPointing','CCSDSAttitude','SpiceAttitude','ThreeAxisKinematic','CommandableNadirPointing']:
    for _attitude_suffix in ('.cpp','.hpp'):
        _name = _attitude_class + _attitude_suffix
        _category, _purpose = DESCRIPTIONS.get(_name,('Named attitude scope',''))
        _extra = ' Add exact-class guarded inherited Copy dispatch to existing typed assignment;no formula or layout change.' if _attitude_suffix=='.cpp' else ' Declare the inherited Copy override;no new data member or virtual slot.'
        DESCRIPTIONS[_name] = (_category,(_purpose+_extra).strip())
for _name,_extra in {
    'AttitudeFactory.cpp':' Pass the requested name to SpiceAttitude construction;unnamed owner construction remains available.',
    'ObjectInitializer.cpp':' Bind declared frame/body references for named standalone attitudes in the active local/global maps without eager provider initialization.',
    'Moderator.cpp':' Bind a named main-script attitude prototype to its declared frame so numeric-state serialization has context;runtime/function clones are rebound by ObjectInitializer.',
    'Attitude.cpp':' Reset MATLAB-format state for all six script serialization modes,matching GmatBase;preserve quoted MATLAB/ephemeris-header output.'
}.items():
    _category,_purpose=DESCRIPTIONS.get(_name,('Named attitude scope',''))
    DESCRIPTIONS[_name]=(_category,(_purpose+_extra).strip())

DESCRIPTIONS['MathNode.cpp'] = ('Math node active-state copying', 'Clone default/scalar nodes without copying an unsized inactive matrix cache;preserve the actual function-input flag and stored string. Base assignment copies active scalar/string/matrix state, resizes sized matrix caches, and leaves unused scalar cache storage unspecified. SetMatrixValue resizes for a new valid shape. All 44 factory spellings have literal/flag/identity/independence controls;no derived polymorphic Copy,expression graph ownership,numerical formula or scientific qualification is inferred.')
DESCRIPTIONS['MathNode.cpp'] = (DESCRIPTIONS['MathNode.cpp'][0], DESCRIPTIONS['MathNode.cpp'][1] + ' Add inherited polymorphic Copy dispatch for the 42 exact concrete MathFactory classes,calling typed assignments with null/incompatible/unsupported-derived guards and self-copy. Default bare main/private/Global routes and selected scalar/matrix state are witnessed;configured operand/wrapper graph ownership remains open.')
DESCRIPTIONS['MathNode.hpp'] = ('Math node Copy interface','Override the inherited GmatBase Copy slot;no new virtual slot or object member.')
for _name,_parent in [('MathFunction.cpp','MathNode'),('BuiltinFunctionNode.cpp','MathNode'),('NumericFunctionNode.cpp','BuiltinFunctionNode'),('StringFunctionNode.cpp','BuiltinFunctionNode')]:
    DESCRIPTIONS[_name] = ('Math node active-state assignment','Delegate typed assignment through '+_parent+' so inherited active values and flags survive. Existing own fields/graph conventions are retained;no mathematical formula change or graph-ownership qualification.')
DESCRIPTIONS['FunctionManager.cpp'] = (DESCRIPTIONS['FunctionManager.cpp'][0],DESCRIPTIONS['FunctionManager.cpp'][1] + ' Initialize the transient-force pointer to null and assign cached matrix results only when sized,resizing the destination for valid result shapes. This supports default FunctionRunner resource Copy without changing matrix/table error guards;initialized manager/graph lifecycle remains unqualified.')

DESCRIPTIONS['MatlabInterface.cpp'] = ('MATLAB evaluation completion',
    'Use a per-evaluation completion scalar to separate a current error from stale errormsg state;'
    'initialize separately so an unparsed syntax error cannot reuse prior success. Avoid scratch names'
    'mentioned by the expression. Actual C Engine error/recovery and selected scratch-named expressions'
    'are witnessed;arbitrary base-workspace collisions, general I/O and process modes remain unqualified.'
    'No API, class layout, parameter flag or numerical formula change.')

DESCRIPTIONS['Sandbox.cpp'] = (DESCRIPTIONS['Sandbox.cpp'][0], DESCRIPTIONS['Sandbox.cpp'][1] +
    ' Before deleting sandbox objects, finalize each distinct live UserDefinedFunction graph that'
    ' is not already finalized. This prevents failed-initialization nested calls from retaining'
    ' deleted function dependencies. Do not access the borrowed main sequence during Clear;'
    ' Moderator may already have destroyed it. Normal completed graphs retain their finalized guard.'
    ' No member, API, formula or eligibility change; broader lifecycle qualification remains open.')

DESCRIPTIONS['CallMatlabFunction.cpp'] = ('MATLAB string transfer and formal output aliases',
    'Double actual apostrophes when constructing MATLAB character literals for String variables and'
    ' string-valued Parameters, preserving exact quoted Unicode input bytes. At initialization, allow'
    ' an existing formal input to also receive a MATLAB result only when the actual bound object is'
    ' an Array, real Parameter or string Parameter; reject unsupported actual types before execution.'
    ' The lexical currentFunction supplies the declared input list. No API, layout, parameter flag'
    ' or numerical formula change. Selected main/direct/nested calls and rejection/recovery controls'
    ' are witnessed; general I/O, object/literal contracts and process modes remain unqualified.')
DESCRIPTIONS['Interpreter.cpp'] = (DESCRIPTIONS['Interpreter.cpp'][0], DESCRIPTIONS['Interpreter.cpp'][1] +
    ' For a CallMatlabFunction inside a function, defer a missing output-object lookup only when the'
    ' exact destination name is a declared formal input of the lexical function. Runtime initialization'
    ' validates the actual bound output type. Other undeclared outputs retain strict rejection;'
    ' no general name exemption, API, layout or eligibility change.')

DESCRIPTIONS['GmatCommand.cpp'] = ('Command execution and MATLAB call labels',
    'Earlier accumulated command-base hunks remain pending separate technical review. The current'
    ' increment includes keyword-free CallMatlabFunction in the existing leading-label insertion'
    ' branch, preserving the authored name and safe Mission Tree source mapping. Assignment handling'
    ' retains its previous body; other command paths are unchanged by this increment. No API, class'
    ' layout, numerical formula or parameter-eligibility change. Exact before/after sources and'
    ' actual main/branch/grouped MATLAB GUI cases provide bounded source-versioned evidence.')


# Linux Qt fmincon integration; these descriptions confer no technical approval.
DESCRIPTIONS['MatlabInterface.cpp'] = (DESCRIPTIONS['MatlabInterface.cpp'][0], DESCRIPTIONS['MatlabInterface.cpp'][1] + ' In the Linux Qt build, serialize C Engine/Matrix operations on one dedicated MATLAB-library worker while the owning UI services real optimizer callbacks. Reject reentrant/off-owner requests and guard mode/output metadata. No MatlabInterface member or virtual-slot change; other platform/application branches retain synchronous behavior. General ownership,protocol,lifetime and complete MATLAB qualification remain pending.')
DESCRIPTIONS['Optimize.cpp'] = (DESCRIPTIONS['Optimize.cpp'][0], DESCRIPTIONS['Optimize.cpp'][1] + ' Validate the complete finite registered MATLAB callback input vector before use; reset the existing executing flag on exception; unregister this callback identity on RunComplete/destruction. Existing external solver state transitions and constraint/cost evaluation remain in use;no new member or numerical formula.')
DESCRIPTIONS['GmatInterface.cpp'] = ('Optimizer callback result lifetime','Add an untruncated std::string callback-result path and identity-checked unregister. Preserve the legacy char API but explicitly reject results exceeding its1024-byte buffer before copying. These are two new nonvirtual public methods,not blanket API or reverse-interface qualification.')
DESCRIPTIONS['GmatInterface.hpp'] = ('Optimizer callback API','Declare GetCallbackResultString and UnregisterCallbackServer as nonvirtual public methods. No new member or virtual slot;new API and rebuilt dependent source libraries remain pending technical scrutiny.')
DESCRIPTIONS['FminconOptimizer.cpp'] = ('Linux Qt real fmincon callback dispatch','Publish the per-solve Qt loopback endpoint to MATLAB,exercise the real existing fmincon driver/state machine and clear the transient session on success/error. Existing active-set algorithm and parameter mappings remain;actual MATLAB iteration/progress/report acceptance is separate and open.')
DESCRIPTIONS['FminconOptimizer.cpp'] = (DESCRIPTIONS['FminconOptimizer.cpp'][0],DESCRIPTIONS['FminconOptimizer.cpp'][1]+' Correct read-only metadata for the six existing writable named option aliases (IDs1000..1005), whose public setters already accept values. No new setter,member,virtual slot or solver formula. Actual GUI option input and native/MATLAB readback are separately audited.')
for _writer in ['MatWriter.cpp','RealMatData.cpp','StringMatData.cpp']:
    DESCRIPTIONS[_writer] = ('MATLAB library thread ownership','Dispatch MAT-file/Matrix API entry points through the same Linux Qt MATLAB-library worker as Engine calls. Reentrant calls during a pending evaluation are explicitly rejected. Preserve original data formats,field layouts and numerical values;ordinary MAT output and callback-error lifetime regressions remain separate checks.')
for _executor in ['MatlabLibraryExecutor.cpp','MatlabLibraryExecutor.hpp']:
    DESCRIPTIONS[_executor] = ('Single MATLAB-library worker','New private plugin implementation for serialized tasks,one owner thread,synchronous exception transfer,Qt UI servicing through the existing interpreter hook and reentrant-access rejection. No claim of asynchronous or general multithreaded GMAT execution;resource/cleanup and platform qualification remain pending.')
PATH_DESCRIPTIONS = {
    'plugins/MatlabInterfacePlugin/src/base/CMakeLists.txt': ('MATLAB C API Qt dispatch build','Retain the prior Linux Qt Engine-only/no-wx dependency repair and private C Engine/Matrix/MAT linkage. Compile the private single-worker implementation only for Linux Qt and link Threads;other platform/application conditions remain. This is integration evidence,not current-build or core approval.'),
    'plugins/FminconOptimizerPlugin/src/base/CMakeLists.txt': ('Linux Qt optimizer build route','Allow Linux Qt in addition to the prior Windows route only with actual MatlabInterface/SDK availability. Define the Qt callback path and limit Linux Qt linkage to the required C APIs;preserve the earlier all-library Windows linkage. Enabling the option alone is not an optimizer acceptance pass.')}

DESCRIPTIONS['GmatInterface.cpp'] = (DESCRIPTIONS['GmatInterface.cpp'][0],DESCRIPTIONS['GmatInterface.cpp'][1]+' Add one nonvirtual fixed iteration-notification bridge with a validation-only phase;it dispatches only the two prescribed Optimize actions and no external command string.')
DESCRIPTIONS['GmatInterface.hpp'] = (DESCRIPTIONS['GmatInterface.hpp'][0],DESCRIPTIONS['GmatInterface.hpp'][1]+' Declare PublishCallbackIteration(data,validateOnly) for fixed metadata;no new member or virtual slot.')
DESCRIPTIONS['Optimize.cpp'] = (DESCRIPTIONS['Optimize.cpp'][0],DESCRIPTIONS['Optimize.cpp'][1]+' Override inherited TakeAction for fixed MATLAB iteration validation/publication,guarding the active fmincon clone,interruption and callback reentry;delegate other actions unchanged.')
DESCRIPTIONS['Optimize.hpp'] = ('Fixed optimizer iteration dispatch','Declare the inherited TakeAction override;no new command member or virtual slot.')
DESCRIPTIONS['FminconOptimizer.cpp'] = (DESCRIPTIONS['FminconOptimizer.cpp'][0],DESCRIPTIONS['FminconOptimizer.cpp'][1]+' On Linux Qt,validate real OutputFcn phase/iteration/function counters before evaluation and validate native cost afterward;publish physical/scaled controls,achieved/desired constraints and cost once per actual notification. Separate actual iterations from branch evaluations;reset telemetry per run,flush optional reports and preserve the legacy non-Qt report path. No optimization formula or active-set algorithm change;telemetry does evaluate the real branch at each notified X. One inherited TakeAction override and two new plugin data members require technical ABI/layout review.')
DESCRIPTIONS['FminconOptimizer.hpp'] = ('Actual MATLAB iteration state','Override the existing TakeAction slot and add one Integer function counter plus one std::string phase in the Fmincon plugin class. This changes plugin class layout on all platforms;no base Solver/Optimizer member or new virtual slot. Dependent plugin source must be rebuilt;cross-platform ABI/technical qualification remains open.')

DESCRIPTIONS['findgmatrootpath.m'] = ('MATLAB C-interface startup path', 'Recognize absolute paths by platform syntax instead of comparing WHAT-normalized spellings. Preserve valid trailing separators and resolve relative paths against the located startup file. The existing installation check remains. Actual high-level MATLAB initial/repeat/error/recovery/unload/reopen execution and private invalid/relative-root controls are required;Windows path behavior and wider science/core/current-build gates remain unqualified. No C API,object layout or numerical formula change.')

for _file in ['GravityField.cpp','GravityField.hpp']:
    _prior = DESCRIPTIONS.get(_file, ('Sampled harmonic gravity',''))
    DESCRIPTIONS[_file] = (_prior[0], _prior[1] + ' Add a nonvirtual explicit Cartesian/GmatTime acceleration entry for sampled plot frames. Reuse the existing harmonic calculation and ordinary spacecraft entry semantics. Sampled evaluation uses the absolute precision epoch with zero elapsed offset and restores both base epochs,elapsed time and precision flag on return/exception. No new member or virtual slot;new C++ symbols and sampled-force behavior remain pending technical/scientific review. General mass/attitude/hardware-dependent forces are not admitted by this adapter.')
DESCRIPTIONS['ODEModel.cpp'] = (DESCRIPTIONS['ODEModel.cpp'][0], DESCRIPTIONS['ODEModel.cpp'][1] + ' Route exact GravityField sampled endpoints through the explicit-state helper as well as PointMassForce. Preserve all identity/lifetime/frame/epoch guards and the strict coeval-live fallback for other force types. Do not read later live spacecraft inputs for unsupported stale samples.')
DESCRIPTIONS['GravityField.cpp'] = (DESCRIPTIONS['GravityField.cpp'][0], DESCRIPTIONS['GravityField.cpp'][1] + ' Set and restore the separate Real/GmatTime cached tide epochs as well as base force clocks. GetTideData reads those caches for Sun/Moon ephemerides;retained Solid-tide controls measured a9.598031223967407e-11km/s2 stale-epoch discrepancy. Solid/SolidAndPole clock coherence is separate from independent tidal-model scientific qualification.')

DESCRIPTIONS['CInterfacePluginFunctions.cpp'] = (DESCRIPTIONS.get('CInterfacePluginFunctions.cpp', ('C-interface diagnostics',''))[0], DESCRIPTIONS.get('CInterfacePluginFunctions.cpp', ('',''))[1] + ' Append the existing ScriptInterpreter error list to the existing failed LoadScript status prefix. Preserve its return code and success behavior;no C API,layout or numerical formula change. Actual MATLAB script rejection and existing affected C clients require verification.')


for _file in ['Moderator.cpp','Moderator.hpp','ScriptInterpreter.cpp','ScriptInterpreter.hpp']:
    _prior = DESCRIPTIONS.get(_file, ('UTF-8 file client entry',''))
    DESCRIPTIONS[_file] = (_prior[0], _prior[1] + ' Add explicit nonvirtual UTF-8 file-reading overloads for C-API source;the existing virtual interpreter and default Moderator file entry retain ASCII rejection. Reuse the full filename/context pipeline. Validate scalar UTF-8 encodings before parsing;reject malformed/overlong/surrogate/out-of-range/truncated input and preserve diagnostics. No new object member,virtual slot or numerical formula;new C++ entry signatures require technical review. Console/include/function policy and broader qualification remain separate.')


for _file in ['TimeData.cpp','TimeData.hpp']:
    _prior = DESCRIPTIONS.get(_file, ('Elapsed time precision',''))
    DESCRIPTIONS[_file] = (_prior[0], _prior[1] + ' Retain a GmatTime reference alongside the existing Real initial epoch;copy/assignment/explicit reference/validation/reset paths maintain both. Compute elapsed days/seconds using high-precision owner or detached report-sample epochs when available;retain the old arithmetic for nonprecision owners. No new virtual slot,time-scale conversion or parameter eligibility. One GmatTime member increases TimeData/ElapsedSecs/ElapsedDays by32bytes on this Linux ABI;all dependent modules must rebuild and technical/scientific approval remains open. Strict10ns,copy/Global/private/explicit/clear and unchanged function-time regressions are required.')

DESCRIPTIONS['TimeData.cpp'] = (DESCRIPTIONS['TimeData.cpp'][0], DESCRIPTIONS['TimeData.cpp'][1] + ' Convert a precise elapsed interval to days from its small seconds difference,avoiding cancellation in a normalized negative GmatTime day representation;the unchanged negative0.25s control exposed the difference.')
DESCRIPTIONS['TimeData.cpp'] = (DESCRIPTIONS['TimeData.cpp'][0], DESCRIPTIONS['TimeData.cpp'][1] + ' Capture the actual SpaceObject GmatTime reference independently of the precision-evaluation flag,or the detached sample when that supplies the initial reference. Preserve the existing Real-only fallback for other SpacePoints that do not advertise precision time. Enabling precision during propagation must not introduce a rounded-Real reference discontinuity. The retained false-to-true mode control exposed a156ns jump;nonprecision elapsed arithmetic and explicit Real InitialEpoch behavior remain unchanged.')

_prior = DESCRIPTIONS.get('Propagate.cpp', ('Propagation stopping',''))
DESCRIPTIONS['Propagate.cpp'] = (_prior[0], _prior[1] + ' Cap the final-step rounding quantum by the explicitly configured time-based stop tolerance converted to seconds. Preserve the existing default microsecond quantum and all non-time stop rounding. The strict60s circular C-interface witness retained its2e-7km bound while exposing microsecond rounding despite a1e-10s StopTolerance. No new data member,API or force formula;precise final-step behavior requires affected propagation/time/C-client regressions and technical/scientific review.')
DESCRIPTIONS['Propagate.cpp'] = (DESCRIPTIONS['Propagate.cpp'][0], DESCRIPTIONS['Propagate.cpp'][1] + ' After stopping-condition initialization,enable existing precision-time clocks for the propagation setup,state,spacecraft and Formation members when an actual time-stop tolerance converts to less than1microsecond. The derived Propagate initializer otherwise retains ordinary clocks despite its base initializer enabling precision. Preserve default and non-time clock selection;retain before traces proving model,state,owner flags false and a1.9557774e-7s miss against1e-10s. No forced precision for unrelated default stops;whole-engine science remains open.')

def git(*args):
    return subprocess.check_output(['git', *args], cwd=ROOT)

def sha(data):
    return hashlib.sha256(data).hexdigest()

def digest(path):
    return sha(Path(path).read_bytes())

def core(path, expanded=True):
    p = Path(path)
    suffixes = {'.cpp', '.hpp', '.h', '.c'}
    if expanded:
        suffixes |= {'.for', '.f', '.f90', '.py', '.m', '.cmake', '.dat'}
    return '/gui/' not in path and (p.suffix.lower() in suffixes or
        expanded and p.name in {'CMakeLists.txt', 'AAREADME.TXT'})

def changed_paths(expanded=True, include_untracked=True):
    tracked = {p for p in git('diff', '--name-only', 'HEAD', '--', *SCOPES).decode().splitlines() if core(p, expanded)}
    untracked = {p for p in git('ls-files', '--others', '--exclude-standard', '--', *SCOPES).decode().splitlines() if core(p, expanded)} if include_untracked else set()
    return tracked, untracked

def recorded_paths(value, found):
    if isinstance(value, dict):
        for key, item in value.items():
            if key.startswith(('src/base/', 'src/gmatutil/', 'plugins/')):
                found.add(key)
            recorded_paths(item, found)
    elif isinstance(value, list):
        for item in value:
            recorded_paths(item, found)

def verify(folder):
    folder = Path(folder).resolve()
    data = json.loads((folder / 'inventory.json').read_text())
    assert data['head'] == git('rev-parse', 'HEAD').decode().strip(), 'HEAD changed'
    for item in data['files']:
        assert digest(ROOT / item['path']) == item['working_sha256'], 'Working file changed: ' + item['path']
        assert digest(folder / item['preserved_working']) == item['working_sha256']
        assert digest(folder / item['preserved_head']) == item['head_sha256']
        assert digest(folder / item['patch']) == item['patch_sha256']
    assert digest(folder / 'core.patch') == data['patch_sha256']
    tracked, untracked = changed_paths(data.get('schema_version', 1) >= 2,
                                      data.get('include_untracked', False))
    current = tracked | untracked
    assert current == {item['path'] for item in data['files']}, 'Core file set changed'
    print(json.dumps({'status': 'matching-current-core-worktree', 'files': len(current), 'inventory': str(folder / 'inventory.json'), 'technical_approval': False}))

DESCRIPTIONS['Validator.cpp'] = (DESCRIPTIONS['Validator.cpp'][0], DESCRIPTIONS['Validator.cpp'][1] + ' Distinguish dotted destinations inventoried for function registration from actual RHS expression inputs. Preserve a separate Parameter wrapper for genuine self-reference,while allowing writable covariance field destinations with readonly reporting Parameters. No eligibility change;original filter equations and absolute-time function regressions are required.')

DESCRIPTIONS['Code500Propagator.cpp'] = ('Code500 interpolation clocks', 'Decode file calendar/seconds with GmatTime and interpolate seconds relative to the first encoded epoch in both Real and precise paths. Preserve representable Real endpoint identity;use endian-aware field reads,correct the unsigned one-record end-stencil guard,and preserve precise initialization/UpdateState queries. Input time system is explicitly read from the header. No member or virtual-slot change;finite GUI and independent spline matrix remain bounded,pending technical/scientific review.')
DESCRIPTIONS['Code500Propagator.hpp'] = ('Code500 epoch helpers', 'Declare two protected nonvirtual helpers for decoded record epochs and relative interpolation time. No data member or virtual-slot change.')
DESCRIPTIONS['Code500EphemerisFile.cpp'] = ('Code500 input time metadata', 'Expose the already decoded input-header time-system indicator separately from output configuration. Preserve GetTimeSystem output behavior;Code500 propagator reads the explicit input accessor. No reader/writer layout or virtual change.')
DESCRIPTIONS['Code500EphemerisFile.hpp'] = ('Code500 input metadata API', 'Declare const nonvirtual GetInputTimeSystem accessor for existing decoded metadata. No member or virtual-slot change.')

# Exact text-ephemeris precision increment; broader format qualification stays open.
for _name,_purpose in {
    'Ephemeris.hpp':'Append precise epoch/validity fields to the public EphemPoint record and a nonvirtual GmatTime interpolation overload. Legacy Real-only records and changed public MJD fields fall back to their Real clock. Record layout changes require dependent rebuilds; existing virtual slots and Ephemeris object layout remain unchanged.',
    'Ephemeris.cpp':'Interpolate in local seconds at preserved file/query epochs; retain Real endpoint identity and strict precise boundaries. Bracket odd-order stencils, return exact nodes, rebuild on method change and throw on interpolation failure instead of returning zero state. Keep existing order-reduction warning and method formulas.',
    'STKEphemerisFile.cpp':'Construct precise scenario clock from midnight and original clock fields; retain input offset seconds per node, use documented Lagrange/Hermite-header fallback, compare precise duplicate times, ignore identical interior repeats and expose consistently rounded endpoint bounds. Header interpolation-degree handling and broader STK qualification remain separate.',
    'CCSDSEMSegment.hpp':'Add nonvirtual precise epoch parse/read APIs and append precise clock/validity fields to protected EpochAndData records. Outer class layout and virtual slots stay unchanged; record layout requires dependent rebuilds.',
    'CCSDSEMSegment.cpp':'Reuse existing calendar/leap validation for precise UTC/A1 node clocks, expose them without Real-MJD intermediates, and retain new epoch fields in existing deep-copy loops. Legacy parse API stays unchanged.',
    'CCSDSOEMSegment.hpp':'Add nonvirtual GmatTime AddData overload; no new virtual slot or outer class member.',
    'CCSDSOEMSegment.cpp':'Preserve exact input clocks, distinguish sub-MJD-ulp nodes, ignore identical duplicates before allocation and reject conflicting duplicates. Retain six Cartesian components of six/nine-value records and size-only validation. Existing Real callers delegate through their representable clock; OEM Lagrange-only policy stays unchanged.',
    'CCSDSOEMReader.cpp':'Pass precise parsed data epochs to the existing concrete OEM segment in both reader paths; covariance/metadata legacy clocks stay unchanged.',
    'CCSDSEphemerisFile.cpp':'Transfer precise segment node clocks to interpolation records in both load paths; return bounds from the same node clocks.',
    'StkEPropagator.cpp':'Keep precise query clocks through initialization, stepping and UpdateState; retain Real dispatch for default nonprecision operation.',
    'CcsdsEphPropagator.cpp':'Keep precise query clocks through initialization, stepping and UpdateState; retain Real dispatch for default nonprecision operation.',
}.items():
    _previous=DESCRIPTIONS.get(_name,('Text ephemeris input clocks',''))
    DESCRIPTIONS[_name]=(_previous[0],(_previous[1]+' '+_purpose).strip())

# Preserve the exact cached-start-clock increment for independent review.
for _name,_purpose in {
    'EphemerisPropagator.hpp':'Add one protected GmatTime file-start cache and nonvirtual precise start/conversion helpers. This changes the ephemeris-propagator base object layout;dependent plugin classes must rebuild. No new virtual slot or eligibility change.',
    'EphemerisPropagator.cpp':'Construct/copy/assign the precise file-start cache,fall back for Real-only providers and invalidate stale companions when their rounded boundary changes. Keep GT clocks through selected file/spacecraft/script starts and continuation;do not replace an explicit parsed start when file loading discovers its span. Preserve Real conversion for nonprecision operation.',
    'StkEPropagator.cpp':'Bind the retained start-node clock into the common precise boundary cache,with an empty-data guard;existing whole-file changes remain pending review.',
    'CcsdsEphPropagator.cpp':'Bind the retained start-node clock into the common precise boundary cache,with an empty-data guard;existing whole-file changes remain pending review.',
    'Code500Propagator.cpp':'Bind the decoded start-record clock into the common boundary cache and preserve it in the derived FROM_EPHEM initialization branch,with an empty-record guard. Existing precise interpolation changes remain pending review.',
}.items():
    _old=DESCRIPTIONS.get(_name,('Ephemeris start clocks',''))
    DESCRIPTIONS[_name]=(_old[0],(_old[1]+' '+_purpose).strip())

# Topocentric exact fresh-Build rotation/serialization increment.
for _name,_purpose in {
    'TopocentricAxes.cpp':'Forward the requested force-computation flag to the origin body-fixed frame so explicit input-epoch conversions bypass its warm nutation cache.',
    'Spacecraft.cpp':'Force rotation refresh for explicit state assignment and serialization at the spacecraft epoch. Preserve the existing two-argument representation getter with normal runtime cache behavior;use a protected nonvirtual overload for forced reads. No authored-value cache,rounding,member layout or virtual slot change.',
    'Spacecraft.hpp':'Declare one protected nonvirtual representation-read overload while preserving existing signatures and member layout.'
}.items():
    _old=DESCRIPTIONS.get(_name,('Coordinates',''))
    DESCRIPTIONS[_name]=(_old[0],(_old[1]+' '+_purpose).strip())

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    selection = parser.add_mutually_exclusive_group(required=True)
    selection.add_argument('--output', type=Path, help='Fresh snapshot directory; existing directories are never overwritten')
    selection.add_argument('--verify', type=Path, help='Check that preserved diff/file identities still match this worktree')
    args = parser.parse_args()
    if args.verify:
        verify(args.verify)
        return
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    head = git('rev-parse', 'HEAD').decode().strip()
    tracked, untracked = changed_paths()
    names = sorted(tracked | untracked)
    patches = {p: git('diff', '--binary', 'HEAD', '--', p) for p in tracked}
    for path in untracked:
        result = subprocess.run(['git', 'diff', '--no-index', '--binary', '--', '/dev/null', path], cwd=ROOT, capture_output=True)
        assert result.returncode == 1 and not result.stderr, 'Cannot preserve untracked source: '+path
        patches[path] = result.stdout
    diff = b''.join(patches[p] for p in names)
    (output / 'core.patch').write_bytes(diff)
    versions = []
    ledger_path = ROOT / 'build/nasa-coverage/coverage-ledger.json'
    ledger = json.loads(ledger_path.read_text())
    for version in ledger['versions']:
        receipt = json.loads(Path(version['receipt']).read_text())
        manifest = json.loads(Path(receipt['snapshot_manifest']).read_text())
        audit_path = Path(version['audit'])
        assert digest(audit_path) == version['audit_sha256'], 'Prior audit drift: ' + str(audit_path)
        audit = json.loads(audit_path.read_text())
        transitions = set()
        for key, value in audit.items():
            if 'transition' in key:
                recorded_paths(value, transitions)
        versions.append((version, manifest, transitions))
    files = []
    for number, path in enumerate(names, 1):
        current = (ROOT / path).read_bytes()
        baseline = b'' if path in untracked else git('show', 'HEAD:' + path)
        assert not current.startswith((b'\x7fELF', b'DAF/SPK', b'DAF/PCK')), 'Runtime binary/kernel is not a core-source artifact: '+path
        patch = patches[path]
        item_dir = output / 'files' / ('%02d' % number)
        item_dir.mkdir(parents=True)
        (item_dir / 'HEAD.source').write_bytes(baseline)
        (item_dir / 'working.source').write_bytes(current)
        (item_dir / 'change.patch').write_bytes(patch)
        category, purpose = PATH_DESCRIPTIONS.get(path, DESCRIPTIONS.get(Path(path).name, ('Unclassified', 'Pending technical description; inspect the preserved diff.')))
        candidates = [dict(label=v['label'], receipt=v['receipt'], receipt_sha256=v['receipt_sha256'], audit=v['audit'], audit_sha256=v['audit_sha256'], transition_mentions_file=path in transitions) for v, m, transitions in versions if m['original_hashes'].get(path) == sha(current)]
        transitions = [c['label'] for c in candidates if c['transition_mentions_file']]
        files.append(dict(path=path, category=category, purpose=purpose, head_present=path not in untracked, status='untracked' if path in untracked else 'tracked-difference', head_sha256=sha(baseline), working_sha256=sha(current), patch_sha256=sha(patch), preserved_head=str((item_dir/'HEAD.source').relative_to(output)), preserved_working=str((item_dir/'working.source').relative_to(output)), patch=str((item_dir/'change.patch').relative_to(output)), diff_hunks=re.findall(r'^@@.*?@@.*$', patch.decode(), re.M), same_file_hash_evidence_candidates=candidates, transition_provenance='File mentioned in an audited transition: '+', '.join(transitions) if transitions else 'Current worktree difference observed; start-of-task/exclusive authorship provenance unresolved.', scrutiny_status='pending technical review; no whole-file or whole-engine qualification inferred'))
    data = dict(schema_version=2, include_untracked=True, created_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(), head=head, branch=git('branch', '--show-current').decode().strip(), comparison='HEAD versus current index/worktree, including tracked differences and untracked core C/C++,Fortran,preparation scripts,build files and model inputs; not an upstream NASA release comparison', core_scopes=SCOPES, file_count=len(files), tracked_difference_count=len(tracked), untracked_count=len(untracked), patch_sha256=sha(diff), coverage_ledger=str(ledger_path), coverage_ledger_sha256=digest(ledger_path), files=files, limits=['Descriptions summarize observed diffs, not their correctness or exclusive authorship.', 'Schema v1 inventories covered tracked C/C++ differences only. Schema v2 also preserves untracked code and selected Fortran/build/preparation/model inputs; count increases from that scope correction are not new task edits.', 'An untracked file has no HEAD version;its preserved HEAD.source is empty and its full contents are preserved as an addition. Start-of-task preimages and exclusive authorship remain separate.', 'Same-file hash receipts are review candidates; they do not prove that every changed hunk was exercised.', 'Old branch commits and changes already present in HEAD are outside this diff. No local upstream tracking ref is available; a release-baseline audit remains separate.', 'Linux focused tests do not qualify all physics, platforms, Windows Unicode behavior or a full current-build regression.', 'No commit, revert, push or source modification is performed by this tool.'])
    (output / 'inventory.json').write_text(json.dumps(data, indent=2)+'\n')
    rows = ['# Core-engine changes preserved for scrutiny', '', 'Baseline HEAD: `'+head+'`. **Pending technical review.**', '', data['comparison']+'.', '', '| File | Area and observed purpose | Audited transition provenance |', '|---|---|---|']
    for item in files:
        rows.append('| ['+item['path']+']('+str(ROOT/item['path'])+') | '+item['category']+': '+item['purpose']+' | '+item['transition_provenance']+' |')
    rows += ['', '## Limits', ''] + ['- '+line for line in data['limits']]
    (output / 'review.md').write_text('\n'.join(rows)+'\n')
    verify(output)

if __name__ == '__main__':
    main()
