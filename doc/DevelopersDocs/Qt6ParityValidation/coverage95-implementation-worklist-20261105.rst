Remaining implementation worklist, 2026-11-05
============================================

This is an implementation planning map of all 105 original open requirements.
It is not acceptance evidence or a completed current-source implementation audit.
The conservative implementation tally is now 1093/1158 (94.39%); the recorded
verified baseline remains 1053/1158. No tests, GUI runs or numerical checks were
performed to produce this map. Original requirements and qualification gates
remain unchanged. Source routes below identify the implementation owners; their
presence alone does not discharge broad all/any clauses.

The machine-readable map, exact original texts/fingerprints and source paths are
in ``build/nasa-coverage/coverage95-20261105-implementation-progress/worklist-v1.json``.
The current engineering tally is ``tracking-v148.json`` in the same directory.

Disposition counts
------------------

* ``depends_on_open_children``: 7
* ``external_dependency``: 9
* ``historical_platform_target``: 8
* ``implementation_present_already_tracked``: 27
* ``implementation_present_newly_tracked``: 13
* ``partial_implementation``: 9
* ``source_routes_need_scope_review``: 15
* ``specification_dependency``: 17

Next implementation actions
---------------------------

Continue the full owning-source review rather than treating every unqualified
row as absent code. Remaining work includes the limited POV reader and broader
source/scope obligations. Provider-dependent work requires the actual
Mars-GRAM and VF13ad sources/SDKs. Specification and historical-platform entries
must be resolved explicitly rather than silently redefined.

The C API combined load/run entry point was a concrete stub found during this
pass. It now calls the established LoadScript and RunScript operations in order,
only executing the latter after a successful load. Its configured build passes;
its runtime verification and the broad C API clauses remain open.

User-body TwoBody propagation and orbit settings
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``FRR!E220``, ``FRR!E228``, ``FRR!E229``, ``FRR!E230``, ``FRR!E231``, ``FRR!E232``, ``FRR!E233``, ``FRR!E234``, ``FRR!E235``, ``FRR!E236``, ``FRR!E238``

Disposition: ``implementation_present_already_tracked``.

Included in the existing conservative tracking estimate; no additional credit is assigned by this map.

Next: Retain the implementation. Behavioral qualification remains deferred.

Owning sources:

* ``src/base/solarsys/CelestialBody.cpp``
* ``src/base/solarsys/SolarSystem.cpp``

SPK propagator origin/settings
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``FRR!E303``, ``FRR!E305``

Disposition: ``implementation_present_already_tracked``.

Included in the existing conservative tracking estimate; no additional credit is assigned by this map.

Next: Retain the implementation. Behavioral qualification remains deferred.

Owning sources:

* ``plugins/EphemPropagatorPlugin/src/base/propagator/SPKPropagator.cpp``

Bulirsch minimum tolerance and settings
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``FRR!E170``, ``FRR!E173``

Disposition: ``implementation_present_already_tracked``.

Included in the existing conservative tracking estimate; no additional credit is assigned by this map.

Next: Retain the implementation. Behavioral qualification remains deferred.

Owning sources:

* ``plugins/ExtraPropagatorsPlugin/src/base/propagator/BulirschStoer.cpp``

EA/MA/HA writes
~~~~~~~~~~~~~~~

``FRR!E506``, ``FRR!E513``, ``FRR!E536``

Disposition: ``implementation_present_already_tracked``.

Included in the existing conservative tracking estimate; no additional credit is assigned by this map.

Next: Retain the implementation. Behavioral qualification remains deferred.

Owning sources:

* ``src/gmatutil/util/StateConversionUtil.cpp``
* ``src/base/spacecraft/Spacecraft.cpp``
* ``src/base/parameter/OrbitData.cpp``
* ``src/qtgui/SpacecraftOrbit.cpp``

Worksheet RMAG/SemilatusRectum read-only compatibility
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``FRR!E519``, ``FRR!E520``

Disposition: ``implementation_present_already_tracked``.

Included in the existing conservative tracking estimate; no additional credit is assigned by this map.

Next: Retain the implementation. Behavioral qualification remains deferred.

Owning sources:

* ``src/gmatutil/util/StateConversionUtil.cpp``
* ``src/base/spacecraft/Spacecraft.cpp``
* ``src/base/parameter/OrbitData.cpp``
* ``src/qtgui/SpacecraftOrbit.cpp``

Optimize EndTarget spelling
~~~~~~~~~~~~~~~~~~~~~~~~~~~

``FRC!E54``

Disposition: ``implementation_present_already_tracked``.

Included in the existing conservative tracking estimate; no additional credit is assigned by this map.

Next: Retain the implementation. Behavioral qualification remains deferred.

Owning sources:

* ``src/base/command/SolverBranchCommand.cpp``
* ``src/base/command/Optimize.cpp``
* ``src/base/command/Target.cpp``

Opt-in finite numeric Achieve goal
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``FRC!E97``

Disposition: ``implementation_present_already_tracked``.

Included in the existing conservative tracking estimate; no additional credit is assigned by this map.

Next: Retain the implementation. Behavioral qualification remains deferred.

Owning sources:

* ``src/base/command/Achieve.cpp``

Worksheet Save default filename
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``FRC!E135``

Disposition: ``implementation_present_already_tracked``.

Included in the existing conservative tracking estimate; no additional credit is assigned by this map.

Next: Retain the implementation. Behavioral qualification remains deferred.

Owning sources:

* ``plugins/SaveCommandPlugin/src/base/command/Save.cpp``

Existing integration-step plot sampling controls
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``FRR!E329``, ``FRR!E330``, ``FRR!E633``, ``FRR!E634``

Disposition: ``implementation_present_already_tracked``.

Included in the existing conservative tracking estimate; no additional credit is assigned by this map.

Next: Retain the implementation. Behavioral qualification remains deferred.

Owning sources:

* ``src/base/subscriber/OrbitPlot.cpp``
* ``src/base/subscriber/GroundTrackPlot.cpp``

State conversions
~~~~~~~~~~~~~~~~~

``FRR!E13``

Disposition: ``implementation_present_newly_tracked``.

The owning-source pass covered the six worksheet forms/frame preconditions,
fourteen advertised native dispatch routes and spacecraft/parameter/Qt
Cartesian conversion paths. It repaired Delaunay angle conventions, stale body
constants on frame/parameter rebinding, binding and state/cache failure rollback,
precise parameter-frame epochs and AlternateEquinoctial origin metadata.

This adds one implementation row to the engineering tally. No numerical or GUI
behavior is qualified. Original acceptance status and all gates remain unchanged.

Next: Retain the implementation; complete representation/frame and entry-path
verification after the implementation pass.

Owning sources:

* ``src/gmatutil/util/StateConversionUtil.cpp``
* ``src/base/spacecraft/Spacecraft.cpp``
* ``src/base/parameter/OrbitData.cpp``
* ``src/qtgui/SpacecraftOrbit.cpp``

* ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-delaunay-anomaly-implementation-20261105.rst``

* ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-orbit-frame-binding-implementation-20261105.rst``

POV model language
~~~~~~~~~~~~~~~~~~

``FRR!E44``, ``FRR!E46``

Disposition: ``partial_implementation``.

The importer now implements static meshes/four finite primitives, corner finishes, includes/scoped declarations and strings, numeric/vector/logical expressions, conditionals/loops/switches/macros, fixed/dynamic/mixed arrays and optional initializers. POV preview/scene jobs support cooperative cancellation. These source changes compile; runtime compatibility is deferred. The original worksheet names POV without a model feature profile. Representative exported-model grammar, remaining container/function/geometry/material compatibility and the complete original import scope remain unresolved; this subset is not qualified as the whole format.

Next: Research the historical GMAT loader, NASA references and representative public model examples to establish the required POV import scope. The user could not identify a model/profile. Do not infer an entire ray-tracer obligation or silently qualify the current subset; continue the other implementation work.

Owning sources:

* ``src/qtgui/PovModel.cpp``
* ``src/qtgui/PovModel.hpp``
* ``src/qtgui/VisualModelDialog.cpp``
* ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-pov-primitives-implementation-20261105.rst``
* ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-pov-includes-implementation-20261105.rst``
* ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-pov-scopes-strings-implementation-20261105.rst``

Kernel attitude wording
~~~~~~~~~~~~~~~~~~~~~~~

``FRR!E50``, ``FRR!E53``

Disposition: ``specification_dependency``.

The native provider reads CK/SCLK pointing, including ckgpav_c; the worksheet literally says SPK attitude kernel.

Next: Reconcile the literal kernel wording with the attitude specification; do not relabel orbit SPK data as pointing.

Owning sources:

* ``src/base/attitude/SpiceAttitude.cpp``
* ``src/base/spice/SpiceAttitudeKernelReader.cpp``

Thruster/impulsive mass decrement
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``FRR!E77``, ``FRR!E80``

Disposition: ``specification_dependency``.

Both classes expose GravitationalAccel. ImpulsiveBurn owns impulsive mass decrement; the row is under thruster properties.

Next: Resolve the intended object ownership; do not invent a thruster-to-impulsive-burn coupling.

Owning sources:

* ``src/base/hardware/Thruster.cpp``
* ``src/base/burn/ImpulsiveBurn.cpp``

Dynamics and drag parents
~~~~~~~~~~~~~~~~~~~~~~~~~

``FRR!E103``, ``FRR!E107``

Disposition: ``depends_on_open_children``.

ODE and drag dispatch exist; the original children include the absent Mars-GRAM provider.

Next: Complete provider children before declaring the all-model parents complete.

Owning sources:

* ``src/base/forcemodel/ODEModel.cpp``
* ``src/base/forcemodel/DragForce.cpp``

Mars-GRAM provider
~~~~~~~~~~~~~~~~~~

``FRR!E120``, ``FRR!E121``, ``FRR!E122``, ``FRR!E123``, ``FRR!E124``, ``FRR!E125``

Disposition: ``external_dependency``.

No checked-in Mars-GRAM provider source was located. A selector/input-file field cannot implement the missing atmosphere.

Next: Obtain the authorized provider/SDK and required data, then connect Low/Mean/High and input file. Provider availability remains unanswered.

Owning sources:

* ``src/base/forcemodel/ODEModel.cpp``
* ``src/base/forcemodel/DragForce.cpp``

FK5 integration frame
~~~~~~~~~~~~~~~~~~~~~

``FRR!E144``

Disposition: ``implementation_present_newly_tracked``.

Owning-source review establishes the existing internal FK5/MJ2000Eq integration
axes, raw state mapping, origin translations and separate force/display frames.
This adds one existing implementation row, with no scientific/acceptance credit.
See ``coverage95-fk5-source-scope-implementation-20261105.rst``.

Next: Retain the implementation; independent numerical verification is deferred.

Owning sources:

* ``src/base/executive/Moderator.cpp``
* ``src/base/coordsystem/AxisSystem.cpp``
* ``src/base/coordsystem/MJ2000EqAxes.cpp``
* ``src/base/spacecraft/Spacecraft.cpp``
* ``src/base/propagator/PropagationStateManager.cpp``
* ``src/base/command/Propagate.cpp``
* ``src/base/forcemodel/ODEModel.cpp``
* ``src/base/forcemodel/GravityField.cpp``
* ``src/base/forcemodel/DragForce.cpp``
* ``src/base/coordsystem/CoordinateConverter.cpp``
* ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-fk5-source-scope-implementation-20261105.rst``


Rotation drift definition
~~~~~~~~~~~~~~~~~~~~~~~~~

``FRR!E221``, ``FRR!E227``

Disposition: ``specification_dependency``.

Retained research identifies angle/rate pairs and existing Wdot, but no authority for a new spin-acceleration term.

Next: Resolve drift definition/units before adding physics; preserve existing orientation settings and research findings.

Owning sources:

* ``src/base/solarsys/CelestialBody.cpp``
* ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-rotation-drift-research-20261105.rst``

VF13ad provider
~~~~~~~~~~~~~~~

``FRR!E286``, ``FRR!E289``, ``FRR!E292``

Disposition: ``external_dependency``.

No checked-in VF13ad provider source was located. Shared solver code does not establish its derivative method or reverse-communication ABI.

Next: Obtain the authorized VF13ad/HSL provider source/SDK; do not substitute an optimizer or guess COMMON layout.

Owning sources:

* ``src/base/command/SolverBranchCommand.cpp``
* ``src/base/command/Optimize.cpp``
* ``src/base/command/Target.cpp``
* ``src/base/command/Vary.cpp``

Ephemeris formats and OEM B-1
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``FRR!E374``, ``FRR!E375``

Disposition: ``source_routes_need_scope_review``.

Format dispatch and CCSDS writer exist. Native parser readback does not establish full B-1 structure and metadata conformance.

Next: Review writer obligations against 502.0-B-1 and implement actual gaps; independent conformance execution remains verification.

Owning sources:

* ``src/base/subscriber/EphemerisFile.cpp``
* ``src/base/subscriber/EphemWriterWithInterpolator.cpp``
* ``src/base/subscriber/EphemWriterCCSDS.cpp``
* ``src/base/subscriber/EphemWriterSTK.cpp``
* ``src/base/subscriber/EphemWriterCode500.cpp``
* ``src/base/subscriber/EphemWriterSPK.cpp``

OEM coordinate-system placeholder
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``FRR!E377``, ``FRR!E381``

Disposition: ``specification_dependency``.

The original entry literally says FILL THIS OUT FROM CCSDS publication 502.0-B-1. The parent depends on that unfinished scope.

Next: Derive the concrete coordinate-system contract from the original standard before claiming completeness; retain current time/frame support.

Owning sources:

* ``src/base/subscriber/EphemerisFile.cpp``
* ``src/base/subscriber/EphemWriterWithInterpolator.cpp``
* ``src/base/subscriber/EphemWriterCCSDS.cpp``
* ``src/base/subscriber/EphemWriterSTK.cpp``
* ``src/base/subscriber/EphemWriterCode500.cpp``
* ``src/base/subscriber/EphemWriterSPK.cpp``

Insufficient interpolation data
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``FRR!E404``

Disposition: ``implementation_present_newly_tracked``.

Source scope covers the unchanged original OEM/Lagrange and SPK/Hermite methods. Error mode checks counts before common final-data processing, including canFinalize=false discontinuities. Repeated Lagrange epochs replace samples; Clear restores configured order. SPK uses the native Hermite count with a two-epoch positive-span floor. Startup collection, ReduceOrder compatibility and native fatal error propagation remain. Nineteen source routes and NAIF Type-13 authority are retained. Compiled without runtime/numerical/GUI/acceptance verification.

Next: Retain the implementation. After the full implementation pass, verify every requested Lagrange/odd Hermite order, strict versus compatibility behavior, boundary/EOF/explicit sampling, duplicate/backward epochs, restored base order and background scratch behavior. Do not weaken original assertions or infer full B-1/final-pass qualification.

Owning sources:

* ``src/base/subscriber/EphemerisFile.cpp``
* ``src/base/subscriber/EphemWriterWithInterpolator.cpp``
* ``src/base/subscriber/EphemWriterCCSDS.cpp``
* ``src/base/subscriber/EphemWriterSTK.cpp``
* ``src/base/subscriber/EphemWriterCode500.cpp``
* ``src/base/subscriber/EphemWriterSPK.cpp``
* ``src/base/spice/SpiceOrbitKernelWriter.cpp``
* ``src/gmatutil/util/interpolator/LagrangeInterpolator.cpp``
* ``src/gmatutil/util/interpolator/Interpolator.cpp``
* ``src/base/subscriber/EphemerisWriter.cpp``
* ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-ephemeris-interpolation-boundaries-implementation-20261106.rst``

Final iterative ephemeris
~~~~~~~~~~~~~~~~~~~~~~~~~

``FRR!E406``

Disposition: ``partial_implementation``.

Trial packet/notification, filename rollover and local cleanup repairs remain.
Reachable whole-resource trial copies now preserve a live output's writer,
publication bindings and epoch/segment bookkeeping while exposing copied values.
The full detached entry configuration is restored before accepted/terminal
processing. Detached/completed formal inputs use fresh-copy initialization,
so cached SOLVING state cannot restore stale settings over final argument
binding. Value copies preserve the destination's local ownership role. Builds
pass; no runtime behavior or acceptance credit is assigned from compilation.

Native terminal/initial-guess replay remains and does not prove convergence.
The complete row remains partial.

Next: Review caller/recursive lifetime of copied spacecraft/frame dependencies
and the broad final-pass policy contract. Continue other remaining scopes before
verification.

Owning sources:

* ``src/base/command/SolverBranchCommand.cpp``
* ``src/base/command/Optimize.cpp``
* ``src/base/command/Target.cpp``
* ``src/base/subscriber/EphemerisFile.cpp``
* ``src/base/subscriber/EphemWriterWithInterpolator.cpp``
* ``src/base/subscriber/EphemWriterCCSDS.cpp``
* ``src/base/subscriber/EphemWriterSTK.cpp``
* ``src/base/subscriber/EphemWriterCode500.cpp``
* ``src/base/subscriber/EphemWriterSPK.cpp``

* ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-ephemeris-trial-notifications-implementation-20261105.rst``

* ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-ephemeris-filename-rollover-implementation-20261105.rst``

* ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-ephemeris-trial-copy-implementation-20261105.rst``

Function I/O
~~~~~~~~~~~~~

``FRR!E450``

Disposition: ``partial_implementation``.

The full original E450-E471 source inventory is retained in the function bracket-provenance stage. Scalar, String, Array, property, literal, count and formal/global-resolution routes are present. GroundStation inputs now own cloned station values and station-owned hardware/error models, with recursive snapshots, targeted frame initialization and cleanup. Whole-resource root snapshots still do not own arbitrary private dependency graphs, and general private/transitive lifetimes remain open. Definition copies now honor the existing compiled-source rebuild boundary. The positive-output bracket rule is tracked separately. E465 explicitly permits no-output calls.

Next: Complete general resource dependency isolation/returned private graphs. Preserve the existing compiled-source rebuild boundary and native no-input/no-output syntax.

Owning sources:

* ``src/base/function/Function.cpp``
* ``src/base/function/UserDefinedFunction.cpp``
* ``plugins/GmatFunctionPlugin/src/base/function/GmatFunction.cpp``
* ``src/base/interpreter/ScriptInterpreter.cpp``

Function core parent
~~~~~~~~~~~~~~~~~~~~

``FRR!E472``

Disposition: ``depends_on_open_children``.

This parent includes any-command execution, MATLAB restrictions and .gmf ASCII contents.

Next: Resolve/complete every child rather than treating a heading as an independent extra feature.

Owning sources:

* ``src/base/function/Function.cpp``
* ``src/base/function/UserDefinedFunction.cpp``
* ``plugins/GmatFunctionPlugin/src/base/function/GmatFunction.cpp``
* ``src/base/interpreter/ScriptInterpreter.cpp``
* ``plugins/MatlabInterfacePlugin/src/base/interface/MatlabInterface.cpp``
* ``plugins/MatlabInterfacePlugin/src/base/command/CallMatlabFunction.cpp``

All commands inside functions
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``FRR!E477``

Disposition: ``source_routes_need_scope_review``.

Native validation/initialization traverses the function command graph. SaveMission Immediate addresses one specific missing path.

Next: Review command-specific function restrictions and implement concrete unsupported routes while retaining grammar preconditions.

Owning sources:

* ``src/base/function/Function.cpp``
* ``src/base/function/UserDefinedFunction.cpp``
* ``plugins/GmatFunctionPlugin/src/base/function/GmatFunction.cpp``
* ``src/base/interpreter/ScriptInterpreter.cpp``
* ``src/qtgui/MainWindow.cpp``
* ``src/qtgui/MissionModel.cpp``
* ``src/base/command/SaveMission.cpp``

MATLAB-to-GMAT restriction
~~~~~~~~~~~~~~~~~~~~~~~~~~

``FRR!E479``

Disposition: ``source_routes_need_scope_review``.

MATLAB calls and GMAT function dispatch use separate providers; that alone does not establish the universal callback restriction.

Next: Inspect registered callback/server paths and exact scope; do not infer arbitrary MATLAB/MEX behavior from normal calls.

Owning sources:

* ``plugins/MatlabInterfacePlugin/src/base/interface/MatlabInterface.cpp``
* ``plugins/MatlabInterfacePlugin/src/base/command/CallMatlabFunction.cpp``
* ``src/base/function/Function.cpp``
* ``src/base/function/UserDefinedFunction.cpp``
* ``plugins/GmatFunctionPlugin/src/base/function/GmatFunction.cpp``
* ``src/base/interpreter/ScriptInterpreter.cpp``

GMF ASCII policy
~~~~~~~~~~~~~~~~

``FRR!E480``

Disposition: ``implementation_present_newly_tracked``.

The main/configuration/full-definition, nested include and cached-sequence paths
have been reviewed. LegacyASCII now requires .gmf basenames and seven-bit ASCII
in main/include sources; Native retains general include filenames. Basename
suffix checks, null pointer rejection, relative file/directory resolution and
cached FunctionPath mutation are repaired. The row is tracked as implementation
present under the user's compatibility direction, with verification deferred.

Next: Retain the implementation; verify source modes, file/encoding errors,
nested includes, relative/dotted paths and cached policy/path rejection after
the implementation pass.

Owning sources:

* ``src/base/function/Function.cpp``
* ``src/base/function/UserDefinedFunction.cpp``
* ``plugins/GmatFunctionPlugin/src/base/function/GmatFunction.cpp``
* ``src/base/interpreter/ScriptInterpreter.cpp``

* ``src/base/interpreter/Interpreter.cpp``

* ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-gmf-source-paths-implementation-20261105.rst``

Complete function scoping
~~~~~~~~~~~~~~~~~~~~~~~~~

``FRR!E491``

Disposition: ``source_routes_need_scope_review``.

Local/global stores, binding, cloning and initialization exist. Formation/PropSetup fixes cover concrete paths rather than all resource classes.

Next: Review the original scoping children against the complete resource ownership/binding inventory and repair actual gaps.

Owning sources:

* ``src/base/function/Function.cpp``
* ``src/base/function/UserDefinedFunction.cpp``
* ``plugins/GmatFunctionPlugin/src/base/function/GmatFunction.cpp``
* ``src/base/interpreter/ScriptInterpreter.cpp``
* ``src/qtgui/MainWindow.cpp``
* ``src/qtgui/ResourceForm.cpp``
* ``doc/help/src/Using_ResourcesTree.xml``
* ``plugins/FormationPlugin/src/base/formation/Formation.cpp``
* ``src/base/propagator/PropSetup.cpp``

All-body/all-frame orbit parameters
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``FRR!E501``, ``FRR!E502``, ``FRR!E525``, ``FRR!E526``

Disposition: ``source_routes_need_scope_review``.

OrbitData resolves origins/frames and native reads/writes. Anomaly writes and read-only compatibility have separate tracked rows.

Next: Review supported body/frame/parameter domains and singular preconditions; do not treat missing numerical evidence as missing code.

Owning sources:

* ``src/gmatutil/util/StateConversionUtil.cpp``
* ``src/base/spacecraft/Spacecraft.cpp``
* ``src/base/parameter/OrbitData.cpp``
* ``src/qtgui/SpacecraftOrbit.cpp``

General event Brent refinement
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``FRR!E605``, ``FRR!E606``

Disposition: ``implementation_present_newly_tracked``.

Brent/RootTolerance now covers every registered locator route: station rise/set,
eclipse boundaries, spacecraft and planetographic-region ContactIntervals, and
intrusion FOV/phase/occultation/transit boundaries. Component windows are refined
before intersection/difference. Continuous FOV geometry retains concave simple
polygons and point/ellipsoid targets. Region transitions use signed finite-edge
distance, and each contact's peak search uses its own time window. Native
metadata, copies/serialization, command-mode policy, additional-target workers
and Qt controls are connected. Configured builds pass; numerical behavior is
unverified. Clipped/neighboring/precision/bracket and discovery limits are
explicit. These rows add implementation presence only, with no acceptance credit.

Next: Retain these routes for the later numerical/GUI verification pass. Continue
the other 72 partial/unclassified/dependent original open rows first.

Owning sources:

* ``src/base/subscriber/EphemManager.cpp``
* ``plugins/EventLocatorPlugin/src/base/locator/ContactLocator.cpp``
* ``plugins/EventLocatorPlugin/src/base/locator/EclipseLocator.cpp``
* ``plugins/EventLocatorPlugin/src/base/locator/IntrusionLocator.cpp``
* ``src/qtgui/EventLocatorDialog.cpp``

* ``src/base/subscriber/EventRootRefinement.cpp``

* ``src/base/subscriber/EventRootRefinement.hpp``

* ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-eclipse-brent-implementation-20261105.rst``

* ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-contact-intrusion-brent-implementation-20261105.rst``

All commands in solver sequences
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``FRC!E7``, ``FRC!E55``

Disposition: ``partial_implementation``.

Target/Optimize control binding, counts, Apply Corrections and last-Vary marking use structural solver scope through ordinary branches, excluding nested solver descendants. All six Execute metadata-registration sweeps now use RegisterSolverControlData in script order, preserving direct same-type nested solver completion and the native mode-specific Vary reset ordering. Ordinary branch statements/conditions/loops and branch-hidden solvers are not executed by this metadata sweep. Optimize reporting refresh uses the same structural scope. Internal event scratch recording is distinguished from user output. Full registered-command, nested-context, kernel-pool, function and output/lifecycle contracts remain incomplete.

Next: Continue complete native/plugin command context and lifecycle review: nested solver/function initialization and publisher transitions, prior-trial kernel selection/coverage, immediate SaveMission and command-specific output/cleanup. Fixed problem dimensions require metadata valid before control-flow execution and consistent controls on subsequent passes. In-tree currentRunState references are confined to GmatCommand, SolverBranchCommand and Optimize; ordinary commands use Publisher state, so no blanket recursive command-state update is justified by this source review.

Owning sources:

* ``src/base/command/SolverBranchCommand.hpp``
* ``src/base/command/SolverBranchCommand.cpp``
* ``src/base/command/Optimize.cpp``
* ``src/base/command/Target.cpp``
* ``src/qtgui/MainWindow.cpp``
* ``src/qtgui/MissionModel.cpp``
* ``src/base/command/SaveMission.cpp``

Source review: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-solver-control-registration-implementation-20261105.rst``.

Vary literal control identity
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``FRC!E8``, ``FRC!E47``, ``FRC!E51``

Disposition: ``specification_dependency``.

Real number is listed as a control variable without a mutable identity or coupling to later commands.

Next: Resolve identity/coupling before adding anonymous literal controls; preserve named Variable/Array/Parameter controls.

Owning sources:

* ``src/base/command/Vary.cpp``

Achieve type parent
~~~~~~~~~~~~~~~~~~~~

``FRC!E93``

Disposition: ``implementation_present_newly_tracked``.

All 88 original FRR-38 entries are mapped: 8 headings, 74 numeric scalar leaf rows, 2 matrix rows and 4 Gregorian string rows. The inventory resolves 106 distinct factory names, including 97 real type names / 92 real classes, to concrete inheritance/return-type and Evaluate routes. Validator/native wrappers, Qt owner/dependency selection and Achieve real-value binding/evaluation support the numeric families, Variables and Array elements; finite literals use AllowConstantGoal compatibility. ParameterWrapper now rejects null/wrong runtime types without changing valid parameter-name alias binding. This is numeric type-interface implementation presence, not universal body/frame/scientific or invalid-domain/error-policy qualification.

Next: Finish the other remaining implementation scopes before verification. Later qualification must preserve the exact FRC!E93-E97/FRR-38 originals and exercise actual family/owner/dependency/value routes. Review legacy finite undefined-value/sentinel propagation separately from type availability; do not invent physics, relax domains, or silently change report/error policies.

Owning sources:

* ``src/base/command/Achieve.cpp``
* ``src/qtgui/CommandForm.cpp``
* ``src/qtgui/ReportParameterDialog.cpp``
* ``src/base/interpreter/Validator.cpp``
* ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-achieve-value-lifetime-implementation-20261105.rst``
* ``src/base/factory/ParameterFactory.cpp``
* ``src/base/parameter/ParameterWrapper.cpp``
* ``src/base/parameter/ParameterWrapper.hpp``
* ``src/base/parameter/RealVar.cpp``
* ``src/qtgui/ParameterReferences.cpp``
* ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-achieve-family-scope-implementation-20261105.rst``

Constraint versus objective wording
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``FRC!E103``

Disposition: ``specification_dependency``.

The worksheet calls NonlinearConstraint a cost-function command. Current code separates constraint residuals and the Minimize objective.

Next: Resolve terminology or intended alternative syntax; do not invent penalty/objective semantics.

Owning sources:

* ``src/base/command/NonlinearConstraint.cpp``
* ``src/base/command/Minimize.cpp``

Numerical Propagate capacity
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``FRC!E113``

Disposition: ``implementation_present_newly_tracked``.

Existing parser/setup/runtime/step/publication collections and Qt group rows have dynamic capacity without an arbitrary numerical group-count cap. Natural memory/container/index ranges remain. This source implementation finding is separate from the retained unlimited stress qualification gap.

Next: Retain dynamic capacity; defer resource-exhaustion/stress and numerical verification. Do not add an arbitrary capacity setting or claim a finite run proves unlimited capacity.

Owning sources:

* ``src/base/command/Propagate.cpp``
* ``src/base/propagator/PropSetup.cpp``
* ``src/base/command/PropagationEnabledCommand.cpp``
* ``src/qtgui/PropagationGroupsDialog.cpp``
* ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-propagate-policy-implementation-20261105.rst``

One ephemeris propagator
~~~~~~~~~~~~~~~~~~~~~~~~

``FRC!E114``

Disposition: ``implementation_present_newly_tracked``.

EphemerisPolicy now supports Native, SingleSetup and SingleGroup in core metadata, parsing, copying, initialization, serialization and Qt editors. Both configured-setup and command-group count interpretations are explicit, using resolved identities before runtime clones. Numerical-only commands remain valid and numerical groups are not capped.

Next: Retain all three supported policies and both explicit count meanings. Defer native/GUI/provider/count/alias/serialization/reinitialization verification; no original acceptance credit is assigned by source implementation.

Owning sources:

* ``src/base/command/Propagate.cpp``
* ``src/base/propagator/PropSetup.cpp``
* ``plugins/EphemPropagatorPlugin/src/base/propagator/SPKPropagator.cpp``
* ``src/base/command/PropagationEnabledCommand.cpp``
* ``src/qtgui/PropagationGroupsDialog.cpp``
* ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-propagate-policy-implementation-20261105.rst``

Save any resource
~~~~~~~~~~~~~~~~~

``FRC!E132``

Disposition: ``source_routes_need_scope_review``.

Save uses native serializers and dependency collection; filename and hardware/string/array snapshot implementations exist.

Next: Review all original resource classes and reconstructible dependency/runtime-serialization contracts; generic serialization is not universal proof.

Owning sources:

* ``plugins/SaveCommandPlugin/src/base/command/Save.cpp``
* ``src/qtgui/MainWindow.cpp``
* ``src/qtgui/ResourceForm.cpp``
* ``doc/help/src/Using_ResourcesTree.xml``
* ``plugins/FormationPlugin/src/base/formation/Formation.cpp``
* ``src/base/propagator/PropSetup.cpp``

Function-call cross references
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``FRC!E160``, ``FRC!E161``

Disposition: ``depends_on_open_children``.

The rows refer to complete function requirement sets rather than separate command behavior.

Next: Retain exact cross-reference scope and complete or resolve the referred children.

Owning sources:

* ``plugins/MatlabInterfacePlugin/src/base/interface/MatlabInterface.cpp``
* ``plugins/MatlabInterfacePlugin/src/base/command/CallMatlabFunction.cpp``
* ``src/base/function/Function.cpp``
* ``src/base/function/UserDefinedFunction.cpp``
* ``plugins/GmatFunctionPlugin/src/base/function/GmatFunction.cpp``
* ``src/base/interpreter/ScriptInterpreter.cpp``

Units wording
~~~~~~~~~~~~~

``All Others!E13``

Disposition: ``specification_dependency``.

The row literally asks for units on non-dimensional fields, which is ambiguous relative to physical-unit labels.

Next: Resolve dimensional/non-dimensional wording; retain defined native units without inventing units for dimensionless values.

Owning sources:

* ``src/qtgui/MainWindow.cpp``
* ``src/qtgui/ResourceForm.cpp``
* ``doc/help/src/Using_ResourcesTree.xml``
* ``plugins/FormationPlugin/src/base/formation/Formation.cpp``
* ``src/base/propagator/PropSetup.cpp``

Console parity
~~~~~~~~~~~~~~

``All Others!E20``

Disposition: ``source_routes_need_scope_review``.

The original exceptions are XY plots, Orbit View, GUI interface and Ground Track Plot. Native mission dispatch is shared.

Next: Review all remaining nonvisual entry points against those exceptions; no requirement for console rendering is inferred.

Owning sources:

* ``src/console/driver.cpp``
* ``src/base/executive/Moderator.cpp``
* ``src/qtgui/MainWindow.cpp``
* ``src/qtgui/MissionModel.cpp``
* ``src/base/command/SaveMission.cpp``

Resource Tree inventory
~~~~~~~~~~~~~~~~~~~~~~~

``All Others!E120``

Disposition: ``source_routes_need_scope_review``.

Factory-backed creation and typed forms exist. Original optimizer entries have explicit library-availability conditions.

Next: Review every listed class/configuration entry point, preserving conditional provider clauses and outstanding provider requirements.

Owning sources:

* ``src/qtgui/MainWindow.cpp``
* ``src/qtgui/ResourceForm.cpp``
* ``doc/help/src/Using_ResourcesTree.xml``
* ``plugins/FormationPlugin/src/base/formation/Formation.cpp``
* ``src/base/propagator/PropSetup.cpp``

Resource cloning and operations
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``All Others!E156``, ``All Others!E157``

Disposition: ``source_routes_need_scope_review``.

Native Clone restrictions coexist with explicit Clone propagator. Formation shared-member compatibility and ownership fixes address specific gaps. FRAC-4.5 is an inconsistent original cross reference.

Next: Reconcile the referenced set and inspect all listed copy/dependency policies; do not claim universal Clone from a few repaired types.

Owning sources:

* ``src/qtgui/MainWindow.cpp``
* ``src/qtgui/ResourceForm.cpp``
* ``doc/help/src/Using_ResourcesTree.xml``
* ``plugins/FormationPlugin/src/base/formation/Formation.cpp``
* ``src/base/propagator/PropSetup.cpp``

Mission Tree commands and operations
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``All Others!E160``, ``All Others!E196``

Disposition: ``implementation_present_newly_tracked``.

The complete original 21 command families and six operations have owning-source routes through templates, native factories, structured/source editors and the common reconstruction/rollback transaction. Insertion now adds a missing line boundary after a final semicolon-free command/comment. Branch ranges, insertion contexts, native structural rules and summary freshness checks are documented in the source inventory; GUI/runtime/serializer behavior remains unverified.

Next: Retain implementation; defer GUI/runtime qualification until the full implementation pass is complete.

Owning sources:

* ``src/qtgui/MainWindow.cpp``
* ``src/qtgui/MissionModel.cpp``
* ``src/base/command/SaveMission.cpp``

MATLAB function interface
~~~~~~~~~~~~~~~~~~~~~~~~~

``All Others!E241``

Disposition: ``source_routes_need_scope_review``.

Engine launch, typed marshalling, error handling and mode controls exist; complete I/O scope is broader.

Next: Review all original I/O types and error/ownership paths; defer licensed runtime checks.

Owning sources:

* ``plugins/MatlabInterfacePlugin/src/base/interface/MatlabInterface.cpp``
* ``plugins/MatlabInterfacePlugin/src/base/command/CallMatlabFunction.cpp``

MATLAB instance modes
~~~~~~~~~~~~~~~~~~~~~

``All Others!E242``, ``All Others!E243``, ``All Others!E244``

Disposition: ``partial_implementation``.

Mode controls include Windows engOpenSingleUse and engOpen. API names do not establish same-instance sharing across every platform/GMAT instance.

Next: Complete the platform-specific contract using actual supported engine APIs; do not label Linux engOpen as Windows shared-session behavior.

Owning sources:

* ``plugins/MatlabInterfacePlugin/src/base/interface/MatlabInterface.cpp``
* ``plugins/MatlabInterfacePlugin/src/base/command/CallMatlabFunction.cpp``

C API script configuration/dynamics parents
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``All Others!E249``, ``All Others!E251``

Disposition: ``depends_on_open_children``.

The loader/run interface delegates to native parsing/preparation/execution. Its
presence does not complete FRR-13's unavailable provider children, including
Mars-GRAM. Complete those original model/SDK obligations before parent credit.

C API Propagate configuration and scalar stops
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``All Others!E252``

Disposition: ``implementation_present_newly_tracked``.

Native C API loading exposes Propagate's FRC-10 configuration routes. This batch
also implements measured Variable/Array-element stops throughout typed sampling,
buffers, stop differences, first-step protection and refinement. Each condition
has its own epoch clock; ownerless user scalars use the first propagated
spacecraft. RHS owners no longer shift clock indices. Native valid spacecraft
stops and numerical tolerances are retained; cyclic metadata resets on rebind.

See ``coverage95-capi-propagate-scope-implementation-20261105.rst`` for owning
sources and the exact FRC-10 source inventory. This is implementation presence,
not C API/scientific/runtime qualification. Whole-row acceptance remains open.

Historical operating systems
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``All Others!E279``, ``All Others!E280``, ``All Others!E281``, ``All Others!E282``, ``All Others!E283``, ``All Others!E284``, ``All Others!E285``, ``All Others!E287``

Disposition: ``historical_platform_target``.

The original entries name XP/Vista/7, Leopard and Fedora Core and their family headings. Modern builds do not establish these historical targets.

Next: Resolve retained historical platform/toolchain obligations; do not replace them with modern OS names or remove their denominator entries.

Owning sources:

* ``CMakeLists.txt``
* ``src/qtgui/CMakeLists.txt``
* ``src/console/driver.cpp``
* ``src/base/executive/Moderator.cpp``

Undefined hardware minima
~~~~~~~~~~~~~~~~~~~~~~~~~

``All Others!E294``, ``All Others!E295``, ``All Others!E296``

Disposition: ``specification_dependency``.

The source uses Xxx MHz and Xxx GB placeholders without numerical minima.

Next: Obtain concrete requirements before implementation or measurement; do not choose convenient values for row closure.

Owning sources:

* ``CMakeLists.txt``
* ``src/qtgui/CMakeLists.txt``

Undefined Validation type
~~~~~~~~~~~~~~~~~~~~~~~~~

``All Others!E310``

Disposition: ``specification_dependency``.

The complete source text is only Validation, without algorithm, inputs or pass/fail rule.

Next: Resolve the intended comparison contract before inventing a validator.

Owning sources:



Ephemeris trial reference and numeric-step setters
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The direct SetRefObject/SetStepSize writer bypasses now preserve accepted writer
bindings/step during effective SOLVING. Actual reference types and finite numeric
steps are checked. FRR!E406 remains partial: output Copy routes do not establish
caller ownership of private callee dependencies. Implement that lifetime contract
and finish policy/internal-recording review before qualification. See
``coverage95-ephemeris-trial-bindings-implementation-20261105.rst``.

Achieve wrapper lifetime and dynamic values
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

FRC!E93 children E94-E97 name FRR-38 scalar Parameters, Variables, Array elements
and real numbers. The real-wrapper/Qt-picker routes exist, with explicit
AllowConstantGoal compatibility. Wrapper replacement now preserves active aliases
and retires unused goal wrappers. Every solver-result update checks finite
measured/desired values and finite positive tolerance; API writes use matching
guards. At that batch, the full family inventory was still pending. The later
family-scope batch completes numeric type-interface presence for FRC!E93 in
tracking-v8; universal body/frame/scientific obligations remain open. See
``coverage95-achieve-value-lifetime-implementation-20261105.rst``.

POV historical scope research
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The old LoadPOV source identifies exported smooth-mesh/material data, but current
wx code rejects POV and its abandoned structures cannot simply be reenabled.
The Qt help now documents its supported static subset. Further representative
exporter/source research belongs to the implementation work; no user dialect
choice is needed and FRR!E44/E46 remain partial. See
``coverage95-pov-historical-scope-research-20261105.rst``.

Function-expression result snapshots and failed calls
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Expression evaluators now require successful Execute and invalidate previous
typed results. Function-owned unnamed object results receive owned root-value
snapshots before recursive caller restoration; true globals remain borrowed.
GroundStation input/value borrowing was removed in the later owned-station pass. Detached function-definition copies no longer share the source-owned
prototype map. Caller-name/dependency graph transfer and definition assignment
remain concrete implementation work. No broad FRR!E406/E450/E491 credit is added.
See ``coverage95-function-result-snapshots-implementation-20261105.rst``.

Returned ephemeris direct caller bindings
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Direct formal spacecraft/frame/ODE-model references now bind to known whole-
resource caller outputs (preferred) or inputs after pointer-identity checks.
Prepared named values survive nested return restoration; unnamed local snapshots
use the same helper. True global dependencies keep their identity. The existing
ODE-model rename branch is now reachable. Private/transitive dependency graph
ownership and definition replacement remain open; no broad row credit is added.
See ``coverage95-function-return-bindings-implementation-20261105.rst``.


Internal event scratch recordings during solver trials
-------------------------------------------------------

Internal EphemManager SPK/CK scratch recorders now declare their background role before initialization; a shared predicate applies solver-trial suppression only to user EphemerisFile output. Scratch packets, notifications, toggles, file rollovers, copies, reference/step updates and lifecycle proceed during trials without user-output snapshots. User output retains existing accepted-pass behavior. Prior-trial kernel selection/coverage, recursive ownership and full contextual command behavior remain unqualified.

At the scratch-recording batch, Execute still used direct-list registration
sweeps. The later control-registration batch below repairs that additional gap;
the broader command/context obligations remain open. These concrete runtime-registration and kernel-context scopes stay
open for source implementation work before later verification. No tests or
acceptance credit accompany this repair. Tally and disposition counts unchanged.
Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-event-scratch-trials-implementation-20261105.rst``.


Solver-owned last-Vary markers
-------------------------------

Last-Vary notification markers now use the structural solver scope, clear stale markers and are assigned by each solver Initialize after successful child initialization, covering nested/function solvers independently of Sandbox top-level marking. The native static last-in-script-order convention is preserved; Execute registration and kernel-pool isolation remain open.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-solver-vary-markers-implementation-20261105.rst``. The configured integrated build passes 66 steps in
4.05 seconds. No new implementation-row/acceptance credit or verification runs;
tally and disposition counts unchanged. The later control-registration batch
repairs the additional Execute sweeps; complete command/context contracts remain
open. Structural binding/marking alone was not counted as completed execution
support.


Solver control metadata registration
-------------------------------------

Target/Optimize control binding, counts, Apply Corrections and last-Vary marking use structural solver scope through ordinary branches, excluding nested solver descendants. All six Execute metadata-registration sweeps now use RegisterSolverControlData in script order, preserving direct same-type nested solver completion and the native mode-specific Vary reset ordering. Ordinary branch statements/conditions/loops and branch-hidden solvers are not executed by this metadata sweep. Optimize reporting refresh uses the same structural scope. Internal event scratch recording is distinguished from user output. Full registered-command, nested-context, kernel-pool, function and output/lifecycle contracts remain incomplete.

Continue complete native/plugin command context and lifecycle review: nested solver/function initialization and publisher transitions, prior-trial kernel selection/coverage, immediate SaveMission and command-specific output/cleanup. Fixed problem dimensions require metadata valid before control-flow execution and consistent controls on subsequent passes. In-tree currentRunState references are confined to GmatCommand, SolverBranchCommand and Optimize; ordinary commands use Publisher state, so no blanket recursive command-state update is justified by this source review.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-solver-control-registration-implementation-20261105.rst``. No requirement-row or acceptance credit is added.


Achieve numeric family inventory and typed binding
----------------------------------------------------

The completed source type/family inventory maps all 88 FRR-38 entries, including
74 real scalar leaf rows / 97 distinct numeric names / 92 real classes. Numeric
wrappers now reject null/wrong resource types while preserving parameter aliases.
FRC!E93 is implementation-present in tracking-v8: 1089/1158 (94.04%), 36 added
rows (30 new/compatibility, 6 existing), 69 other original rows. Recorded
verification remains 1053/1158 and all 105 acceptance rows remain open. This is
numeric-interface presence, not universal body/frame/scientific/error-policy
qualification. Matrix/string rows remain in the original denominator.
Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-achieve-family-scope-implementation-20261105.rst``.


Checked numerical evaluation and stopping data
-----------------------------------------------

Checked numerical evaluation now honors native system RealVar Evaluate success before reading its cache and rejects non-finite results. MathElement operands, actual Achieve/Minimize/NonlinearConstraint updates and active stopping value/goal/clock/refinement reads use it. Missing objective/constraint operands raise errors instead of submitting fake runtime numbers; optional constraint tolerance is read once and retains native finite nonpositive fallback. Direct reporting and native initial metadata lookup paths remain. Vary scaling/value and other numeric consumers still need source work; no broader row closure or acceptance credit.

The integrated builds pass 323 steps / 217.91 s and 66 steps / 2.94 s. No tests,
GUI, mission/function/model/solver execution, numerical comparison, evidence
reader or seal. No new row credit; tally 1089/1158 (94.04%), 69 other original
rows, recorded verified baseline 1053/1158, all 105 acceptance rows open. Report:
``doc/DevelopersDocs/Qt6ParityValidation/coverage95-checked-numerical-values-implementation-20261105.rst``.


Vary values, scaling and six-field refresh
--------------------------------------------

Vary now supplies six values to both Solver registration/refresh, fixing the five-value refresh stack payload. Mutable option evaluation, physical unscaling/report refresh/corrections use checked native parameter status. Range inputs preserve infinities, defaults, clamping and retained-solution behavior. Solver snapshots and validates all six fields and every destination before committing, with provider-specific nonzero perturbation requirements. Native formula/reset/capability/notification behavior and correction after RunComplete remain. Numerical overflow fallbacks and runtime/provider cases are unverified; broad command contexts remain incomplete.

Builds pass 323 steps / 355.43 s and 66 steps / 6.94 s. No runtime/numerical or
acceptance verification. No new row credit; tracking remains 1089/1158 (94.04%),
69 other original rows; recorded verified baseline 1053/1158, all 105 acceptance
rows open. Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-vary-numerical-data-implementation-20261105.rst``.


Mission Tree source scope and insertion separators
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The 21-command/six-operation inventory and separator repair add two engineering
implementation-presence rows in tracking-v9: 1091/1158 (94.21%), 67 other rows.
The private integrated build passes five steps / 5.04 s. No verification runs or
acceptance credit. The report is
``coverage95-mission-tree-scope-implementation-20261105.rst``; the stage is
``build/nasa-coverage/coverage95-20261105-implementation-mission-tree-scope``.


GMAT function output brackets
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``FRR!E470``

Disposition: ``implementation_present_newly_tracked``.

CreateAssignmentCommand now preserves authored output syntax for declared GMAT
functions, allowing the strict bracket setting and cached syntax checks to see
single-output shorthand. Other providers retain their existing canonicalization.
The zero-output editorial question adds no rule beyond E465's explicit support
for calls without outputs. The full I/O parent remains partial.

Build v1 passes 66 steps / 10.97 s; v2 passes 66 steps / 10.40 s. No verification
runs. See ``coverage95-function-bracket-provenance-implementation-20261105.rst``.
Tracking-v10: 1093/1158 (94.39%), 66 other original rows; verified baseline and all
105 open acceptance rows remain unchanged.


Owned GroundStation function values
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

GroundStation copies now clone owned hardware, configured/path error models and
antenna links before swapping collections. Function inputs use owned formals;
recursive saved values, local result snapshots and cleanup include stations.
Targeted initialization restores copied station frames on refresh/recursive
return without batch-initializing unrelated global objects. Copied output/direct
assignments initialize their station frames. Owner names follow formal aliases.

The private integrated build passes 184 steps / 174.31 s. No verification runs.
General private/transitive dependencies, other resources, kernel context and
function-definition lifetime remain open; no parent row is added. Tracking-v11
stays 1093/1158 (94.39%), 66 other original rows. See
``coverage95-groundstation-function-values-implementation-20261105.rst``.


Function output wrapper queue ownership
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

GmatFunction pending results now enter the same ObjectManagedFunction queue used
by ClearOutputWrappers/GetWrappersToDelete. Prior results, copied destinations
and destructors release outputs before dependent resources. Borrowed inputs and
class layout remain. This repairs the shadow-queue gap without claiming full
definition replacement/private-graph closure. Build: 77 steps / 7.87 s; no
verification runs. See ``coverage95-function-output-queue-implementation-20261105.rst``.
Tracking-v12 stays 1093/1158 (94.39%), 66 other original rows.


Function definition copies and compiled-source policy
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Copies can no longer pair a changed definition with an old compiled graph.
Identical definitions retain current cached interfaces/invocations; changed
interpreted definitions require the existing rebuild lifecycle. Uncompiled copies
release old templates and reset execution context. Initial/same-pointer sequence
installation is supported; replacing an interpreted sequence requires rebuilding.
Normal RunMission still clears its sandbox and adds fresh clones. No runtime
hot-swap or complete function I/O/scoping closure is claimed.

Builds pass 76 steps / 12.86 s and 76 steps / 12.34 s; no verification runs.
Tracking-v13 remains 1093/1158 (94.39%), 66 other original rows. See
``coverage95-function-definition-copy-policy-implementation-20261105.rst``.


Returned ephemeris private frame ownership
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Detached returned ephemerides now own private output/data/internal frames,
including native axes, referenced coordinate systems and calculated points.
Exact formal caller mappings and true globals retain their identity. Trial-entry
owners keep accepted-writer bindings alive; copies retain applicable graphs.
Direct private spacecraft/ODE roots and general transitive graphs remain open.

Builds pass 148 steps / 162.10 s and 67 steps / 4.57 s; no verification runs.
Tracking-v14 stays 1093/1158 (94.39%), 66 other original rows. See
``coverage95-function-ephemeris-frame-ownership-implementation-20261105.rst``.


Returned ephemeris private spacecraft ownership, 2026-11-06
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Private spacecraft within returned ephemeris graphs now retain their native
owned-component identities and borrowed frame/estimation references without
initial-state conversion or duplicate hardware/model ownership. Native phased
initialization uses a copied-resource path that preserves already bound hardware
associations. Body-shape/interpolator/model copy defects and attached covariance
owner references are repaired. Private ODE/transient roots, standalone returned
resource owners, input graph isolation and complete scoping remain open.

Builds pass 392 steps / 353.21 s, 68 / 7.17 s, 66 / 6.18 s and 66 / 7.83 s.
No verification runs. Tracking-v15 stays 1093/1158 (94.39%), 66 other original
rows. See ``coverage95-function-ephemeris-spacecraft-ownership-implementation-20261106.rst``.


ODE copied-force owners and ephemeris temporary attachments, 2026-11-06
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ODE value copies now stage/own independent force components, including transients,
and release them through every native force removal path. Borrowed Sandbox
transients retain existing ownership. Ephemeris acceleration sampling scopes
new attachments, preserves already present identities and rejects destructive
unique replacement before mutation. Returned private force/burn/frame/transient
graphs, standalone resource roots and input isolation still require implementation.
The original entire-object and non-global-input preservation clauses remain.

Builds pass 225 steps / 163.31 s and 66 / 5.05 s; no verification runs.
Tracking-v16 stays 1093/1158 (94.39%), 66 other original rows. See
``coverage95-ode-copied-force-ownership-implementation-20261106.rst``.

Returned ephemeris dynamics context
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Returned ephemerides retain private static force-model dependencies and bind caller live burn context, compiled but unverified. Prior frozen-transient plan is superseded. General root lifetimes/input isolation, private-body/provider references, moving-frame contexts and full function scoping remain open; no additional rows.

See ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-function-ephemeris-dynamics-context-implementation-20261106.rst``. The private integrated build passes 327, 66 and 76
steps across the three source versions; no verification ran.

Function resource value contexts
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Retained function resource contexts now cover named/unnamed native frame, calculated-point, spacecraft and ODE roots, with deterministic side-registry leases, Clone/Copy destination rebinding and initialization that preserves retained references/owned hardware. Compiled but unverified. Multi-output component commit/rebinding, other root families, direct native copy paths, cross-Sandbox lifetimes, global/private dependency graphs and full input isolation/scoping remain open; no added rows.

See ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-function-resource-value-context-implementation-20261106.rst``. The integrated build passes 747, 68 and 68 steps;
no verification ran.

Function output component bindings
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Covered function-private output graphs now use invocation-bound identities and resource-copy/component-rebind/ephemeris-last phases for ordinary and nested returns. Real/Array/String outputs freeze before caller mutation and use typed setters after restoration; destination IsLocal scope is preserved. Compiled but unverified. Pre-refresh caller/input scene capture, external caller-owned component lifetimes, full recursive frames/input isolation, other roots/selector/primitive cases and global/provider scopes remain open; no added rows.

See ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-function-output-component-bindings-implementation-20261106.rst``. Four integrated compilation versions pass; no
verification ran. Finish caller/input graph capture and all remaining scope.


Caller/input scene capture, 2026-11-06
--------------------------------------

Inputs now capture caller-scene roots and evaluated Parameter values before formal mutation, with exact invocation/component binding after all formal Copies. Saved supported caller graphs reconnect after non-ephemeris output Copies. Explicit evaluated arguments stage by position before caller saving; caller lookups refresh each entry. Compiled but unverified; no added rows. Other root families, Parameter/opaque borrower and cache handling across type replacement, child-created scope cleanup, global/provider/private-body contexts, later parent replacement and full isolation/scoping remain.

Source report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-function-input-scene-capture-implementation-20261106.rst``. Private integrated v1/v2
builds pass; no runtime/acceptance verification ran. No original row is closed.

Next: Expand native graph support to remaining root families and complete Parameter/opaque borrower/cache rebinding, recursive local cleanup, global/private dependency ownership and exceptional scoping. Preserve entire-object inputs/outputs and native definition rebuild rules; keep all other original scopes open before verification.


Formation retained values, 2026-11-06
-------------------------------------

Formation now participates in retained function root/dependency graphs through a core/plugin native member binder. The binder preserves copied packed state and flags while staging member/name permutations and J2000 refs; actual bound-member lookup preserves initialization identity. Native dormant Clone differs from explicit full function-value snapshots. FormationInterface native assignment retains contexts owning destination members, with flattened/pruned constituent leases. Compiled but unverified; active propagation name lookup, buffer-source identity, spacecraft native buffer rebinding, deferred unbound members and complete command/scoping/provider graphs remain. No broad row credit.

Source report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-formation-function-graphs-implementation-20261106.rst``. Private integrated v1/v2/v3 builds pass;
no runtime/numerical/acceptance verification ran. No original row is closed.

Next: Complete active propagation bound-member and buffer-source identities, spacecraft buffer graph rebinding and PSM/force/stop/transient consumers; retain default/shared/dormant Formation policies. Materialize deferred configured members, expand remaining native roots and complete general function isolation/scoping before verification. Keep the full original object/body/frame/command/provider scope.


Propagation buffer identities, 2026-11-06
-----------------------------------------

Propagation buffers now use exact captured sources and bound Formation members, retain source contexts, refresh changed root/member/layout/context bindings before cached models, and clear on native lifecycle boundaries. Spacecraft restore uses typed Copy/context rebind; Formation stays a dormant native buffer. Propagate duplicate checks use identity and scalar stop clocks use bound members. Dynamic Formation updates now carry the complete next spacecraft slot, and UpdateInitialData no longer repeats whole all-force setup/update loops. Compiled but unverified; complete force/PSM/owned-component caches, transient/name/history consumers, other roots and full function isolation/scoping remain open. No parent credit.

Source report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-propagation-buffer-identities-implementation-20261106.rst``. Private integrated v1/v2 builds pass;
no runtime/numerical/acceptance verification ran. No original row is closed.

Next: Complete owned spacecraft hardware/plate/frame force-cache refresh after native Copy, PSM/name/history/transient consumer ownership, and remaining PropSetup/GroundStation/burn/event/provider graphs. Preserve all original object/body/frame/command and scoping obligations; finish the implementation pass before runtime verification.


Finite-burn owned bindings, 2026-11-06
--------------------------------------

FiniteBurn now refreshes current owned thruster/type bindings before firing, resolves requested reporting references passively, clears old definition caches and limits depletion selection to its selected thrusters. Tank association and chemical sampling use validated current owner pointers without reading old caches or changing names/mixture ratios; disabled chemical zero results remain. Removed spacecraft plates are deleted. Compiled but unverified. Complete live burn target restoration/identity, root/prototype lifetimes, other provider/frame and function graph/scoping obligations remain; no parent credit.

Source report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-finite-burn-owned-bindings-implementation-20261106.rst``. Private integrated v1/v2/v3 builds pass;
no runtime/numerical/acceptance verification ran. No original row is closed.

Next: Complete live finite-thrust target identity/passive restoration and remaining prototype/root/provider/frame lifetimes, then expand PropSetup/PSM, GroundStation and other resource graphs. Preserve complete original object, command, body/frame and scoping obligations; finish implementation before runtime verification.


Finite-thrust sampling isolation, 2026-11-06
--------------------------------------------

FiniteThrust per-spacecraft sampling now uses guarded private spacecraft and burn copies. The new const nonvirtual helper binds its scratch target without the maneuver-start setter and calls Fire once, leaving live burn target/output/last-fire/override data intact while scratch hardware receives evaluation caches. Native start/integration and legacy name selection remain. Compiled but unverified; complete configured-target identity/alias/lifetime, shared provider/frame caches and other root/scoping obligations remain. No parent credit.

Source report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-finite-thrust-sampling-isolation-implementation-20261106.rst``. Private integrated build passes;
no runtime/numerical/acceptance verification ran. No original row is closed.

Next: Complete configured transient target identity/alias/lifetime semantics without conflating propagated-object lists with target registries; preserve live ephemeris transient policy. Continue PropSetup/PSM, GroundStation and every other original resource/body/frame/command/scoping obligation before verification.


Configured finite-thrust identities, 2026-11-06
-----------------------------------------------

Begin records actual target roots separately from propagated objects. FiniteThrust selects live configured identities, guards burn references and compares selected-thruster overlap; legacy name-only providers remain supported. End matches actual burn/spacecraft objects and removes the exact matched force. Propagation/Formation diagnostics/ephemeris selection and ODE transient admission use the target contract. Non-owning GmatBase lifetime tokens reject expired/address-reused references. Compiled but unverified; explicit sampling-copy provenance, force-cache borrower lifetime and global maneuver persistence remain. No parent credit.

Source report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-finite-thrust-target-identities-implementation-20261106.rst``. Private integrated v3 passes; prior compilation errors and correction retained. Tracking-v26 stays 1093/1158 (94.39%); no runtime/numerical/acceptance verification or original row closure.

Next: Complete explicit sampling aliases from the owning retained graph and actual invocation identities; audit Begin-owned force removal/reinitialization and ODE borrowers. Native commands already initialize on function entry/frame restoration. Preserve live transient policy and explicit global maneuvers; do not blanket-remove all forces on return. Continue every original remaining resource/body/frame/command/scoping/provider obligation before verification.


Force borrower retirement, 2026-11-06
--------------------------------------

PhysicalModel destruction now removes exact force references from live subscribed ODE models. AddForce registers weak receiving identities with insertion rollback; normal ReleaseForce unregisters before clearing/replacement/deletion. The callback avoids the dying derived force and invalidates cached setup. Propagation cleanup deletes its exact transient pointer, and UpdateSpacecraftParameters runs its all-force update once. Compiled but unverified; active-vector/global policy, still-live force dependencies/aliases and remaining resource graphs remain. No parent credit.

Source report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-force-borrower-retirement-implementation-20261106.rst``. Both private integrated builds pass; no runtime/numerical/acceptance verification ran. Tracking-v27 remains 1093/1158 (94.39%).

Next: Fix Burn assignment's discarded owned local frame and implement passive retained Burn frame/body/spacecraft references, preserving dormant unbound definitions. Finish explicit sampling-copy provenance and global/function activity lifetime without blanket global cleanup; keep every original whole-object/input/body/frame/command/provider scope.


Retained Burn graphs, 2026-11-06
----------------------------------

Finite/impulsive burns now participate in retained function roots/dependencies with passive frame/origin/J2000/spacecraft/prototype binding and dormant unbound initialization. Concrete Clone/Copy hooks reconnect completed values; private initialization follows frames/spacecraft. Owned local frames retire on assignment/rebinding/lifecycle changes, shortcut flags reset, and selected tank/thruster lookup/index caches refresh safely. Compiled but unverified; no parent credit.

Source report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-burn-function-graphs-implementation-20261106.rst``. All three integrated builds pass; no runtime/numerical/acceptance verification ran. Tracking-v28 stays 1093/1158 (94.39%).

Next: Add retained FiniteThrust burn/target graph rebinding; its native copies currently clear burns and retain evaluator/propagated lists. Preserve separate ephemeris caller-live transient policy and explicit-global identity; finish aliases/persistence and all remaining original resource/body/frame/command/provider/scoping obligations before verification.


Finite-thrust value graphs, 2026-11-06
---------------------------------------

FiniteThrust now rebinds retained burns/configured targets with live identities, declaration-role aliases and evaluator/PSM cache retirement. Full ODE value graphs retain force components; only ephemeris scenes use caller-live reporting policy. Incoming overlapping forces supersede owned copies after insertion. PhysicalModel assignment retires raw/model aliases safely for null source state. Both builds and the new DEBUG_MEMORY syntax branches compile; no runtime/acceptance qualification or parent credit.

Source report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-finite-thrust-value-graphs-implementation-20261106.rst``. Tracking-v29 stays 1093/1158 (94.39%). A factory source inventory supplies a bounded next scope map; literal registrations include commands/Parameters/internal/conditional types and do not prove resource support.

Next: Complete named-resource family ownership/Clone/Copy/context mapping and PropSetup/owned propagator/model/PSM retained binding, preserving destination name and dormant policies. Continue GroundStation, standalone hardware, subscribers/events/providers and every original live-alias/global/private-body/deferred/recursive/exceptional/scientific obligation before verification.


PropSetup ownership and retained graphs — 2026-11-06
----------------------------------------------------

Configured PropSetup function values now map their native engine/model and PSM
declarations; ordinary command clones preserve empty registrations. StateManager
copies own independent ListItems; PSM/estimation map replacement retires old
entries. Native preparation rebuilds runtime mappings. Final integrated build
v3 passes; no behavioral verification or broad parent credit. See
``coverage95-propsetup-function-graphs-implementation-20261106.rst``.
Continue remaining resource families, aliases/globals/providers and every
original entire-object/scoping/body/frame/scientific obligation.


GroundStation retained graphs — 2026-11-06
------------------------------------------

Stations now enter retained input/saved-caller/output/native Clone/Copy scenes,
with exact owned hardware/configured/path-model and central/J2000-body mappings.
Geometry initializes before dependent frames; retained initialization preserves
registered mask Imager/FOV identities. Both integrated builds pass; no behavioral
verification or broad parent credit. See
``coverage95-groundstation-transitive-graphs-implementation-20261106.rst``.
Standalone resource families, live aliases/globals/private providers, unresolved
configuration and every original whole-object/scoping/scientific clause remain.


Hardware/FOV retained graphs — 2026-11-06
-----------------------------------------

Named hardware/FOV input/saved-caller/output/native copy scenes now map actual
dependencies, with passive thruster/power binding and deferred child hooks during
owner construction. Owned FOV/model and constructor-signal copies are staged.
All private integrated builds pass; no behavioral verification or parent credit.
See ``coverage95-hardware-function-graphs-implementation-20261106.rst`` and its
source creation/ownership map. Interior owned-root aliases, other named families,
providers/globals/later owners and all original broad clauses remain open.


Attitude/Plate retained graphs — 2026-11-06
-------------------------------------------

Eight attitudes and Plate now enter retained root/dependency/native copy scenes.
Plate history frames bind in the actual source scope with lifetime identities;
owned readers are staged, numeric definitions and prepared attitude state retained.
Integrated final compilation passes; no behavioral verification or parent credit.
See ``coverage95-attitude-plate-function-graphs-implementation-20261106.rst``.
Original entire-object/input preservation and all remaining families, aliases,
dynamic/private/provider/global/exceptional/scientific scope remain unchanged.


Filter/error/noise/estimated-parameter graphs, 2026-11-06
----------------------------------------------------------

Filters/error/noise/estimated-parameter retained roots now compile with passive provider binding, actual participant/frame refs, owned epoch snapshots, thinning history and guarded native component replacement. All other families, dynamic Parameters/wrappers, owned aliases and original broad/scientific scope remain open; no parent credit.

Source report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-filter-error-noise-function-graphs-implementation-20261106.rst``. Both private integrated builds pass; no runtime/numerical/acceptance verification ran. Tracking-v34 remains 1093/1158 (94.39%).

Next: Continue solver/subscriber/event/interface/data/measurement/stop-condition/function/body/solar-system/custom families. Preserve dynamic expressions and dependencies; complete owned aliases, global/private/kernel/NAIF, later replacement and all original broad/scientific scope before verification.


Parameter dependency/native copy graphs, 2026-11-06
----------------------------------------------------

Parameter dependencies now map actual declared/cached references through twelve located data families; 299 located native Clone bodies and 299 typed Copy bodies complete contexts/self-metadata after native construction/assignment. Compiled only. Dynamic expressions retain dependencies; StopCondition/wrapper ownership and other owner/provider/global/scientific scope remain open. Method counts are not runtime universe evidence; no parent credit.

Source report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-parameter-dependency-graphs-implementation-20261106.rst``. Four private integrated builds pass, with no runtime/numerical/acceptance verification. Tracking-v35 stays 1093/1158 (94.39%).

Next: Implement StopCondition retained wrapper/parameter ownership and exact nested array-index dependencies. Continue every remaining solver/subscriber/event/interface/data/measurement/function/body/solar-system/custom resource, aliases, global/private/kernel/NAIF/later and original recursive/exceptional/scientific scope. Finish implementation before verification.


Stopping-condition/wrapper graphs, 2026-11-06
----------------------------------------------

StopCondition retained roots snapshot wrappers/interpolation tables, register owned Parameters and preserve actual references/prepared history. Eleven typed wrapper binders and native variable/string/index ownership repairs compile. EquationWrapper and general alias/late/global/private/provider/scientific scope remain open; no broad parent credit.

Source report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-stop-wrapper-function-graphs-implementation-20261106.rst``. Three integrated builds and legacy-cloning DEBUG_MEMORY syntax branch compile; no runtime/numerical/acceptance verification. Tracking-v36 stays1093/1158(94.39%).

Next: Implement EquationWrapper/RHSEquation/MathTree/FunctionRunner expression/map ownership and live dependencies. Continue all remaining resource/command/body/frame/provider, configured-transient/owner/global lifetime/alias, optional parser, private/kernel/NAIF/later/recursive/exceptional/scientific obligations before verification.


Expression node ownership, 2026-11-06
-------------------------------------

MathTree deep-owns unique native node graphs, preserves shared structural aliases and includes FunctionRunner argument subtrees in cloning, cleanup and thirteen setup/rename/name-inventory walkers. Definition metadata copies; wrapper and scope maps detach pending owner binding. No expression evaluation or new row credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-math-tree-node-graphs-implementation-20261106.rst``. Four private integrated builds and two syntax-only optional/debug branches pass; no runtime/acceptance verification. Tracking-v37 unchanged.

Next: Implement RHSEquation/EquationWrapper owned wrapper maps and passive actual object/callee/context binding, preserving live expressions. Continue all other original resource/command/private/global/body/kernel/NAIF/alias/recursive/exceptional/scientific obligations before verification.


RHS expression ownership, 2026-11-06
------------------------------------

RHSEquation native definitions now own copied trees and deduplicated wrapper maps, preserve top-level aliases and canonical cloned owned values, and stage before replacement. Assignment copies own independent equation containers, clears shared operands by actual identity before container deletion, and preserves aliases during replacement. Changed expressions rebuild; null-map lookup no longer inserts/mutates scope maps. Compiled only; passive retained expression/callee/context closure remains open.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-rhs-expression-ownership-implementation-20261106.rst``. Final private integrated build and two debug syntax-only branches pass; no runtime/acceptance verification. Tracking-v38 unchanged.

Next: Implement EquationWrapper native definition/result ownership and RHSEquation/MathTree/MathElement/FunctionRunner passive actual object/scope/callee context binding, preserving live expressions and all original forms. Continue every remaining original resource/command/private/global/body/kernel/NAIF/owner-alias/later/recursive/exceptional/scientific obligation before verification.


Equation definitions/results, 2026-11-06
----------------------------------------

EquationWrapper copies full cold definitions/base metadata/type/bindings, invalidates old results, and reevaluates live dependencies through scalar/matrix/vector-view/string/object getters. Generated Array/Object values now belong to result wrappers; Assignment uses RAII and rethrows function errors. RHS readiness invalidates on native definition/wrapper changes, matrix shape/type metadata remains strict, and structural wrapper cycles/owned-value aliases are inventoried. Compiled only; actual retained node/scope/callee binding remains open.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-equation-wrapper-result-ownership-implementation-20261106.rst``. Four integrated builds and six optional debug syntax-only branches pass. No runtime/acceptance verification. Tracking-v39 unchanged.

Next: Implement EquationWrapper/RHSEquation/MathTree/MathElement/FunctionRunner passive actual object/scope/callee binding and retained ownership. Native direct RVECTOR math-node returns, private/caller/global/container aliases and every remaining original resource/command/body/kernel/NAIF/later/recursive/exceptional/scientific obligation remain before verification.


Expression scope binding, 2026-11-06
-------------------------------------

Passive expression binding now maps actual MathElement refs, builtin wrappers, function callees/environments, local/configuration scopes and owned MathTree/node graphs. Scope aliases share scene-owned storage; engine globals/runtime force services retain native sharing, while value-owned tables copy. Native copies stay cold, explicit values preserve source readiness. Definition changes retain referenced older owners, with ordered retirement. Compiled only; full Function/private resource admission and other original scope remain open.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-expression-scope-binding-implementation-20261106.rst``. Five integrated builds and ten optional debug/argument syntax compilations pass. No runtime/acceptance verification. Tracking-v40 unchanged.

Next: Implement remaining Function/GmatFunction/native callable resource graphs and ordinary function-environment preparation, then direct RVECTOR math-node results. Continue all original resource/command/solver/subscriber/interface/event/data/body/solar-system/provider/global/caller/interior-alias/later/kernel/NAIF/recursive/exceptional/scientific obligations before verification.


Native function environments, 2026-11-06
-----------------------------------------

Native expression initialization now resolves its callee in the incoming local/global scopes, preserving null shadowing and clearing missing/wrong-type bindings. Call-site names no longer rename shared function definitions. Callee replacement retires old invocation state through native definition assignment; same-callee reentry keeps its stack. GetEphemStates owns an independent typed clone of its file frame, retires the destination frame on assignment, remaps direct owned-frame aliases and rejects incompatible Copy inputs. Compilation only; full Function resource/body/prototype graphs and their private-resource gate remain open.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-function-environment-implementation-20261106.rst``. Corrected private integrated compilation passes; runtime/acceptance verification remains deferred. Tracking-v41 unchanged.

Next: Implement retained Function/GmatFunction/native callable resource body/prototype graphs and passive reference providers without dropping compiled definitions. Continue direct RVECTOR nodes and every original resource/command/provider/global/caller/interior-alias/later/kernel/NAIF/recursive/exceptional/scientific obligation. POV scope still requires independent exporter/model research; do not ask the user to identify a profile. Finish implementation before verification.


Compiled function definitions, 2026-11-06
------------------------------------------

Cold UserDefinedFunction/GmatFunction copies now retain compiled command graphs and independently owned local/automatic prototypes. Structural next/branch roles, native deletion ownership, parent backlinks and previous links copy through guarded staging; invocation cursors/stores stay cold. Prototype aliases share one owner, older owners survive dependent definition changes, automatic runtime instances cannot replace the persistent parser prototype, and cleanup deduplicates actual identities. Call commands bind typed callees/scopes without renaming shared definitions, clear copied call caches, and receive current environment/frame services before execution. Compiled only; full passive Function resource providers/admission and broad original clauses remain open.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-function-definition-graphs-implementation-20261106.rst``. Four private integrated builds pass; no runtime/acceptance verification. Tracking-v42 unchanged.

Next: Implement passive Function/ObjectManagedFunction/UserDefinedFunction/GmatFunction and native callable resource ownership/reference providers, register compiled commands/prototypes and all embedded expression/wrapper owners before binding, and finish each command/private-resource family. The retained private Function gate remains closed until that full closure exists. Continue direct RVECTOR nodes and all original global/caller/interior/later/kernel/NAIF/recursive/exceptional/scientific/resource/command requirements. Finish implementation before verification; independently research POV models/exporters without asking the user for a dialect.


Callable function values, 2026-11-06
-------------------------------------

Fifteen located native callable classes (fourteen core built-ins and MatlabFunction) now provide explicit passive environment/reference binding, typed completed Clone/Copy, cold invocation interfaces and owned ephemeris-frame registration. Function providers advertise support explicitly; UDF prototypes/commands disclose ownership but interpreted function binding remains gated. Current FOS objects are private dependencies for actual cached refs, native global/force sharing remains, cleanup retires callable caches before deleting stores, and managers track actual callee lifetime identities. Native scalar/string/cache copies were completed without invoking clocks, RNGs, sleeps or functions. Compiled only; broader function/command/scope/alias/lifetime clauses remain open.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-callable-function-values-implementation-20261106.rst``. Four private integrated builds pass; no runtime/acceptance verification. Tracking-v43 unchanged.

Next: First retire command/RHSEquation/MathTree wrapper and scope bindings when an invocation FOS ends: Finalize currently leaves borrowed maps/wrappers, and v42 deep UDF definition copying must not inspect retired FOS data. Then implement passive interpreted Function/UDF/GmatFunction compiled-body/prototype/reference providers and full command-family ownership before enabling that provider. Finish owned-frame borrow-of-destination retirement, global/scope-map/service/caller/interior/later lifetime cases, direct RVECTOR math results, and every original resource/command/body/kernel/NAIF/recursive/exceptional/scientific requirement. Preserve full scope; verification stays deferred until implementation is finished. Research POV independently; do not ask the user for a profile.


Function binding retirement, 2026-11-06
---------------------------------------

FunctionManager cleanup now retires actual matching compiled-command scope bindings before deleting owned invocation stores, including owned recursive stores and the native-finalization error route. Assignment/RHSEquation/MathTree detach borrowed references, deduplicate owned operand aliases and retire wrappers while the FOS lives. Native arithmetic/element/builtin/runner nodes have passive retirement contracts; call commands detach cached argument/callee/environment views. Lexical definitions and topology survive, and next native function initialization revalidates wrappers. Compiled only; interpreted providers and broad resource/command/lifetime clauses remain open.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-function-binding-retirement-implementation-20261106.rst``. Integrated compilation passed; runtime/acceptance verification remains deferred. Tracking-v44 unchanged.

Next: Implement the remaining command-specific ownership/reference and retirement providers, then complete passive UserDefinedFunction/GmatFunction compiled-body/prototype providers before enabling interpreted function value admission. Finish retained/interior/global/caller/scope-table/service/engine-reset/later/exceptional lifetimes, GetEphemStates borrow-of-destination-owned-frame Copy, direct RVECTOR math results, all original body/kernel/NAIF/scientific and resource requirements. Research POV independently; the user has no dialect or representative models and must not be asked again. Finish implementation before verification without narrowing original requirements.


Command runtime retirement, 2026-11-06
---------------------------------------

Fourteen core command classes now retire their specific runtime views before a matching function scope is deleted. Native command ownership edges order children before parent-owned snapshots. Loop/solver wrappers, While-owned elapsed Parameters, reporter/writable/subscriber views, callback registrations and initialization flags retire. Native conditional/report/write declaration copies preserve sized empty operand slots, complete base/configuration data and cold binding; wrapper replacement preserves live aliases and cleanup clears duplicate roles before unique deletion. Compiled only; full command/plugin/solver/prototype providers and all broad original lifetimes remain open.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-command-runtime-retirement-implementation-20261106.rst``. Integrated compilation passed; runtime/acceptance verification remains deferred. Tracking-v45 unchanged.

Next: Resolve managed automatic prototype entries that can outlive their invocation FOS, with actual identity and persistent declaration ownership preserved. Complete solver ownership/retained references and all remaining command/plugin ownership/reference/retirement providers, then passive UserDefinedFunction/GmatFunction body/prototype admission. Finish retained/interior/global/caller/scope-table/service/engine-reset/later/exceptional lifetimes, GetEphemStates destination-owned-frame alias Copy, direct RVECTOR results and every original body/kernel/NAIF/scientific/resource requirement. Keep full original scope and verification deferred until implementation is finished; research POV independently without asking the user for a dialect.


Automatic definition lifetimes, 2026-11-06
---------------------------------------

Automatic declarations distinguish owned persistent prototypes from borrowed current-scope lookup hits. Actual per-slot lifetime identities and borrowed variants protect native reuse/cold copies from retired invocation objects. Owned automatic Parameters passively retire declaration/cache references while preserving lexical roles and pure value/history data; null declaration slots rebuild through ordinary validation. Global restoration views contain only live non-invocation borrows. Compiled only; full interpreted function/command/resource/alias/lifetime clauses remain open.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-automatic-definition-lifetimes-implementation-20261106.rst``. Integrated compilation passed; runtime/acceptance verification remains deferred. Tracking-v46 unchanged.

Next: Complete full solver ownership and all remaining command/plugin ownership/reference/retirement providers, then passive UserDefinedFunction/GmatFunction body/prototype admission. Finish retained/interior/global/caller/scope-table/service/engine-reset/later/exceptional lifetimes, GetEphemStates destination-owned-frame alias Copy, direct RVECTOR results and every original body/kernel/NAIF/scientific/resource requirement. Automatic custom non-Parameter prototype providers, declaration-map mutations and arbitrary ownership aliases remain within original scope. Keep verification deferred until implementation is finished and research POV independently without asking the user for a dialect.


Solver command ownership, 2026-11-06
---------------------------------------

Target/Optimize local solver instances now have explicit command owners and stable native-class identities; clone updates copy into the local instance instead of borrowing/deleting the configured master. Owned lifetimes extend through branch and following command destruction. Loop snapshots stage full native clones, preserve actual aliases and restore exact live source identities instead of same-name lookups. DifferentialCorrector cleanup tracks allocated row dimensions, cold solver copies retire prior buffers/streams/plots, and error cleanup attempts child retirement while preserving the first error. Compiled only; full solver/resource/command providers, reference scopes and scientific cases remain open.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-solver-command-ownership-implementation-20261106.rst``. Integrated compilation passed; runtime/acceptance verification remains deferred. Tracking-v47 unchanged.

Next: Complete full native solver value/reference providers, remaining command/plugin ownership/reference/retirement providers and passive UserDefinedFunction/GmatFunction body/prototype admission. Finish snapshot cross-resource/private-formation/caller/global/recursive/exceptional bindings, all interior/scope-table/service/engine-reset/later lifetimes, GetEphemStates destination-owned-frame alias Copy, direct RVECTOR results and every original body/kernel/NAIF/scientific/resource requirement. Preserve original scope and keep verification deferred until implementation is finished. Research POV independently without asking the user for a dialect.


Native vector boundaries, 2026-11-06
------------------------------------

Native vectors now materialize independently at function input and output boundaries. Direct RHS and EquationWrapper vector results retain their native type through an owning RvectorWrapper; function results preserve vector type and shape, while Array formals use column snapshots. Fixed three/six-component generic bridges dispatch native typed evaluation/setters with dimension checks, and matrix-valued property assignment receives an oriented matrix rather than an unsized buffer. Compilation only; full native solver, command/plugin, interpreted function, scope and scientific obligations remain open.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-function-vector-boundaries-implementation-20261106.rst``. Integrated compilation passed; runtime/acceptance verification remains deferred. Tracking-v48 unchanged.

Next: Complete full native solver value/reference providers, remaining command/plugin ownership/reference/retirement providers and passive UserDefinedFunction/GmatFunction body/prototype admission. Finish snapshot cross-resource/private-formation/caller/global/recursive/exceptional bindings, all interior/scope-table/service/engine-reset/later lifetimes, GetEphemStates destination-owned-frame alias Copy and every original body/kernel/NAIF/scientific/resource requirement. Finish native VECTOR_WT builtin formal carriers and discovery, custom vector shape providers and vector arithmetic/cross-scope/recursive/error cases without reducing original scope. Preserve verification deferral until the full implementation pass is complete. Research POV independently without asking the user for a dialect.


Passive Yukon state copies, 2026-11-06
--------------------------------------

Yukon native copies now preserve all declared numerical/options/history state without re-querying the user problem or reconstructing the initial guess. Function-generator and native problem data copy independently, including unsized arrays; assignments stage allocations before replacing data. Explicit callback rebinding validates actual original pointers, and the GMAT problem bridge checks a weak identity for its actual optimizer owner before access. Compilation only; native Solver/Yukonad/plot value admission, full command/plugin/interpreted and original scientific/scope obligations remain open.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-yukon-passive-value-state-implementation-20261106.rst``. Integrated compilation passed; runtime/acceptance verification remains deferred. Tracking-v49 unchanged.

Next: Join the completed passive Yukon backend copies to full native Solver/Optimizer/Yukonad/DifferentialCorrector/fmincon value and reference providers, including owned plot/subscriber resources, native derivative caches, owner/service lifetimes and explicit callback rebinding. Complete all remaining command/plugin ownership/reference/retirement providers and passive UserDefinedFunction/GmatFunction body/prototype admission. Finish snapshot cross-resource/private-formation/caller/global/recursive/exceptional bindings, all interior/scope-table/service/engine-reset/later lifetimes, GetEphemStates destination-owned-frame alias Copy, native VECTOR_WT builtin carriers/discovery/custom shape/arithmetic cases and every original body/kernel/NAIF/scientific/resource requirement. Keep the complete original scope and verification deferral. Research POV independently without asking the user for a dialect.


Native solver function values, 2026-11-06
-----------------------------------------

DC, Yukon and fmincon now have explicit native retained-value providers. Numerical data, actual DC allocation dimensions, derivative caches and independent Yukon callback/runtime state copy passively; file/MATLAB services remain dormant. Completed typed Clone/Copy rebuild value contexts after native cold definition copies, while function preparation preserves captured data. Copying no longer publishes status under the source name; cleanup clears unscaled storage. Compilation only. Other solver subclasses, owned plots, full command/plugin/interpreted, alias/scope/service and scientific obligations remain open.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-native-solver-function-values-implementation-20261106.rst``. Integrated compilation passed; runtime/acceptance verification remains deferred. Tracking-v50 unchanged.

Next: Complete native providers for every remaining solver/estimator/simulator and other resource family, owned plot/subscriber/interface/measurement resources, all command/plugin ownership/reference/retirement paths and passive UserDefinedFunction/GmatFunction body/prototype admission. Finish snapshot cross-resource/private-formation/caller/global/recursive/exceptional bindings, arbitrary ownership/interior/scope-table/service/engine-reset/later lifetimes, GetEphemStates destination-owned-frame alias Copy, native VECTOR_WT builtin carriers/discovery/custom shape/arithmetic and every original body/kernel/NAIF/scientific/resource requirement. Keep complete original scope and verification deferral. Research POV independently without asking the user for a dialect.


Native built-in vector carriers, 2026-11-06
-------------------------------------------

Built-in VECTOR_WT outputs now allocate native RvectorVar formal objects. The generic vector Parameter has completed typed Clone/Copy, staged size-changing assignment and complete VectorSize metadata. Vector output signatures accept row or column declarations and expose the native one-dimensional value. Formal-output metadata lengths validate before indexing, and scalar/Array/vector/String allocation remains owned through sizing and logging. Compiled only. Repeated changing output types/shapes, borrowed-scope ownership, legacy output discovery, custom vector metadata/arithmetic and full original function/solver/resource/scope/scientific obligations remain open.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-builtin-vector-carriers-implementation-20261106.rst``. Integrated compilation passed; runtime/acceptance verification remains deferred. Tracking-v51 unchanged.

Next: Complete repeated builtin output metadata type/shape refresh and formal ownership/alias lifetimes, legacy output discovery and custom vector metadata/arithmetic. Complete every remaining solver/estimator/simulator and other resource provider, owned plot/subscriber/interface/measurement resources, all command/plugin ownership/reference/retirement paths and passive UserDefinedFunction/GmatFunction body/prototype admission. Finish snapshot cross-resource/private-formation/caller/global/recursive/exceptional bindings, arbitrary ownership/interior/scope-table/service/engine-reset/later lifetimes, GetEphemStates destination-owned-frame alias Copy and every original body/kernel/NAIF/scientific/resource requirement. Keep complete original scope and verification deferral. Research POV independently without asking the user for a dialect.


Builtin output refresh and store lifetimes, 2026-11-06
------------------------------------------------------

Builtin formal outputs now refresh for changed native types/dimensions before and after native initialization. Complete replacement dictionaries and objects stage before publication, compatible outputs retain identity, and displaced managed outputs stay with the actual store owner across borrowing managers. Owner identity detects retired managed stores and avoids stale-map reuse. Previous owned stores remain tracked across rebinding. Argument interfaces retire by actual wrapper identity, wrapper aliases delete once and wrappers retire before FOS objects. Compiled only; external/unmanaged ownership, custom initialization/declaration protocols and all original function/resource/scope/scientific obligations remain open.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-builtin-output-generations-implementation-20261106.rst``. Integrated compilation passed; runtime/acceptance verification remains deferred. Tracking-v52 unchanged.

Next: Complete custom builtin output-declaration/cache rebinding protocols, legacy output discovery and every custom vector metadata/arithmetic case, including all inout aliases and externally supplied map lifetimes. Complete every remaining solver/estimator/simulator and resource provider, owned plot/subscriber/interface/measurement resources, all command/plugin ownership/reference/retirement paths and passive UserDefinedFunction/GmatFunction body/prototype admission. Finish snapshot cross-resource/private-formation/caller/global/recursive/exceptional bindings, arbitrary ownership/interior/scope-table/service/engine-reset/later lifetimes, GetEphemStates destination-owned-frame alias Copy and every original body/kernel/NAIF/scientific/resource requirement. Preserve complete original scope and verification deferral. Research POV independently without asking the user for a dialect.


Function output declaration notifications, 2026-11-06
----------------------------------------------------

Native function output type/shape setters now stage all three metadata arrays and synchronously notify the active manager before returning, so native initialization can bind the prepared typed objects. Fourteen native leaf overrides use this base protocol. Recursive identical declarations are idempotent; conflicting recursive declarations fail, and preparation exceptions restore metadata. An actual-lifetime-checked invocation guard suspends the outer handler during argument setup, binds the current manager, and restores the prior handler after success or failure cleanup. Definition copies do not copy active callbacks. Compiled only; complete nested invocation state, output-name/cache contracts, external maps and every original function/resource/scope/scientific clause remain open.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-output-declaration-notifications-implementation-20261106.rst``. Two integrated compilations passed. Tracking-v53 unchanged; verification deferred.

Next: Complete the whole nested invocation state and native output-name/cache contracts, legacy discovery, inout aliases, external-map lifetimes and custom vector arithmetic/type inference. Complete all remaining solver/estimator/simulator and resource providers, owned plot/subscriber/interface/measurement resources, command/plugin ownership/reference/retirement paths and passive UserDefinedFunction/GmatFunction body/prototype admission. Finish snapshot cross-resource/private-formation/caller/global/recursive/exceptional bindings, arbitrary ownership/interior/scope-table/service/engine-reset/later lifetimes, GetEphemStates destination-owned-frame alias Copy and all original body/kernel/NAIF/scientific/resource requirements. Retain full original scope and verification deferral; research POV and rotation drift independently without inventing specifications or asking again for a POV dialect.


GetEphemStates frame alias Copy, 2026-11-06
------------------------------------------

GetEphemStates now supports Copy/assignment when the source borrows the destination-owned file coordinate frame. Actual frame owners are shared only for lifetime, while each copied primary frame and axes remain independent. Source direct output/internal-frame borrows and retained-context dependencies acquire leases before replacing target values; shared-primary aliases deduplicate actual ownership. Cold copy remaps owned output/internal aliases. All four native reader frame replacements stage before publication and retain direct aliases. Cleanup releases leases, clears internal aliases and uses actual lifetime tokens to avoid retiring a reused/dead component. Compiled only; arbitrary scope-table/interior/concurrent/engine-reset ownership and every broad function/resource/scientific requirement remain open.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-getephem-frame-alias-copy-implementation-20261106.rst``. Two integrated compilations passed. Tracking-v54 unchanged; verification deferred.

Next: Complete whole nested function invocation state and native output-name/cache contracts, legacy discovery, inout aliases, external-map lifetimes and custom vector arithmetic/type inference. Complete every remaining solver/estimator/simulator/resource provider, owned plot/subscriber/interface/measurement resources, command/plugin ownership/reference/retirement and passive UserDefinedFunction/GmatFunction body/prototype admission. Finish snapshot cross-resource/private-formation/caller/global/recursive/exceptional bindings and all arbitrary ownership/interior/scope-table/service/engine-reset/later lifetime cases, including GetEphemStates aliases outside its direct cached frame fields and declared retained contexts. Finish all original body/kernel/NAIF/scientific/resource requirements. Preserve full original scope and verification deferral; research POV and unresolved rotation semantics independently without inventing specifications or asking again for a POV dialect.


Function invocation field state, 2026-11-06
-------------------------------------------

Nested function entries now save and restore complete Function declaration metadata and common environment services through an actual-lifetime-checked layered state protocol. Entry depth distinguishes reentry during argument preparation as well as execution, is cold in copies and unwinds through cleanup. ObjectManagedFunction restores registered store views only while their exact owner remains registered; expired managed views stay detached and uninitialized. Seven stateful native leaves preserve their cache/value fields, including independently retained GetEphemStates frame owners and fixed-vector state. Restoration stages allocations, swaps without model evaluation, retires inner storage before enclosing environments, preserves failure status and reports normal-path restoration failure. Every managed map has an independent registration token; manager views validate both owner and generation. Deleted dictionaries retire bindings and preserve their owned entries in the owner retirement pool. Compilation only; complete wrapper/input/output/result/manager stacks, interpreted FCS/prototype/command state and all original alias/lifetime/scientific requirements remain open.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-function-invocation-state-implementation-20261106.rst``. Final integrated compilation passed. Tracking-v55 unchanged; verification deferred.

Next: Complete function argument/output wrapper ownership and each manager input/result/call-stack frame across same-manager and shared-function reentry, including exceptional restoration, output-name/cache transactions, legacy discovery, inout aliases, external maps and custom vector type/arithmetic. Complete interpreted FCS/prototype/command runtime frames and passive UserDefinedFunction/GmatFunction providers, every remaining solver/estimator/simulator/resource provider and owned plot/subscriber/interface/measurement resources. Finish snapshot cross-resource/private-formation/caller/global/recursive/exceptional binding and arbitrary ownership/interior/scope-table/service/engine-reset/later lifetimes, GetEphemStates aliases outside direct fields/retained contexts, all original body/kernel/NAIF/scientific/resource requirements and independent POV/rotation research. Preserve the full worklist and verification deferral.


Function wrapper invocation bindings, 2026-11-06
------------------------------------------------

Function argument/result wrapper bindings now suspend into their own formal maps and output-deletion queue for nested calls. Actual wrapper leases preserve the original objects and aliases, defer native owner deletion while a call still needs them, and invalidate captured identities on direct destruction. Input leases retain manager ownership until release; function-owned suspended outputs transfer into leases and restored bindings retain their owners. Cold interfaces and all allocations stage before detachment, with unarmed restoration during failed staging. Inner interfaces retire before native cache/environment restoration; retired saved bindings remain null, mark readiness false and report failure. Function copies keep invocation leases empty. Compilation only; complete manager configuration/input/result/stack frames, interpreted FCS/prototype/command frames, external ownership graphs and all original function/resource/scientific clauses remain open.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-function-wrapper-invocations-implementation-20261106.rst``. Final integrated compilation passed. Tracking-v56 unchanged; verification deferred.

Next: Finish each FunctionManager configuration/input/output/result/call-stack frame across same-manager and shared-function reentry, including pending arguments and exceptional restoration. Complete interpreted FCS/prototype/command runtime frames and passive UserDefinedFunction/GmatFunction providers; complete all remaining solver/estimator/simulator/resource providers and owned plot/subscriber/interface/measurement resources. Complete native output-name/cache transactions, legacy discovery, inout aliases, external-map lifetimes, custom vector type/arithmetic, snapshot cross-resource/private-formation/caller/global/recursive/exceptional bindings, arbitrary ownership/interior/scope-table/service/engine-reset/later lifetimes and every original body/kernel/NAIF/scientific/resource requirement. Preserve the full worklist and verification deferral; continue independent POV and unresolved rotation research without invented specifications.


Manager invocation frames, 2026-11-06
-------------------------------------

Reentrant FunctionManager call setup and typed execution now use a separate cold manager with independent configuration, arguments, FOS ownership/generation, wrappers, pending values, results, initializer and stacks. Running/pending/completed frame owners stay distinct, matching failed caller frames unwind atomically, and the full structural command graph rebinds its actual caller before initialization. Setup getters follow prepared calls, active callee services/names do not mutate before scoped entry, and nested function stores start cold. Completed frame runtimes retire before reuse while their returned value storage stays alive for argument capture. Reentrant children retire before parent resources; active finalization/context retirement defer safely, stale child cleanup cannot finalize another installed call, and nested failure does not clear caller publication data. Validator scratch/diagnostic/navigation and Moderator lookup contexts restore through scoped snapshots; root diagnostics remain visible and actual resource/global side effects persist. Activation ownership is cold in copies; explicit naming policy follows the call definition. Compiled only; interpreted FCS/prototype/command frames, full direct/finalization/custom-manager lifecycle protocols, arbitrary ownership/scope/service and every original scientific/resource requirement remain open.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-manager-call-frames-implementation-20261106.rst``. Final integrated compilation passed. Tracking-v57 unchanged; verification deferred.

Next: Complete interpreted FCS/prototype/command invocation runtime frames and passive UserDefinedFunction/GmatFunction resource providers, then every remaining solver/estimator/simulator/resource provider and owned plot/subscriber/interface/measurement resource. Complete reentry during direct/finalization/custom-manager lifecycle and all shared interpreter/service ownership, native output-name/cache transactions, legacy discovery, inout aliases, external-map lifetimes and custom vector type/arithmetic. Finish snapshot cross-resource/private-formation/caller/global/recursive/exceptional binding, arbitrary ownership/interior/scope-table/service/engine-reset/later lifetimes and all original body/kernel/NAIF/scientific/resource requirements. Preserve the full worklist and verification deferral; continue independent POV and unresolved rotation research without invented specifications.


Interpreted invocation frames, 2026-11-06
----------------------------------------

Nested interpreted function entry now stages an independent cold command sequence and local/automatic prototype tables before detaching live state. Command self bindings target the actual callee rather than the temporary storage owner. The enclosing sequence, tables, validator lookup, ownership pool, legacy non-owning wrapper list and initialization/finalization flags swap into a scoped owner and restore intact after inner concrete completion, including completion exceptions. Cold inner commands retire before enclosing common environments restore. GmatFunction also owns a separate unused-global list and new-function flag for each invocation; list reconstruction publishes only after successful lookup/allocation, and leaf restoration follows inner command completion. Four production paths compile. Full command/plugin copy and binding protocols, caller stack/resource rebinding, passive interpreted function value providers, shared services and every original resource/scientific requirement remain open. No row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-interpreted-invocation-frames-implementation-20261106.rst``. Tracking-v58 unchanged; final integrated compilation passed; verification deferred.

Next: Complete caller resumption without resetting live enclosing loop/solver execution, including cross-resource bindings after saved values and outputs are copied. Complete every command/plugin cold-copy, ownership and passive rebinding route and passive UserDefinedFunction/GmatFunction value providers; complete direct/finalization/custom-manager reentry and all shared interpreter/service lifetimes. Continue remaining solver/estimator/simulator/subscriber/interface/measurement and body/kernel/NAIF/scientific/resource clauses, output-name/cache transactions, legacy discovery, inout aliases, external maps, custom vector arithmetic and arbitrary scope/interior/engine-reset/later lifetimes. Keep original full scopes and verification deferral; research POV and unresolved rotation semantics independently without inventing specifications.


Live caller resumption, 2026-11-06
----------------------------------------

Ordinary nested calls with a private child object store now preserve the caller's actual resources and command execution state instead of cloning every unrelated local, copying those values back and reinitializing the enclosing FCS. A live frame records the exact managed dictionary generation and is consumed once on success or matching failure unwind. Explicit outputs are assigned once through the existing typed value/binding path, with no second evaluation during stack return. Argument values still use independent typed copies, and nested function state restores through its existing scoped frame. Caller-chain store checks detect shared ancestors and validate private argument initialization; explicit legacy shared-map frames retain their value-snapshot path and remain part of the full unfinished scope. Two production paths compile. Complete component-changing output bindings, legacy/shared/external-map ownership, full command/plugin/resource providers and every original function/scientific requirement remain open. No row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-live-caller-resumption-implementation-20261106.rst``. Tracking-v59 unchanged; final integrated compilation passed; verification deferred.

Next: Complete scoped caller subscriber/global-service binding restoration without command reinitialization, and passive caller resource/command-cache rebinding when explicit outputs replace owned components, preserving live solver/loop state. Finish legacy/shared/external-map ownership and all direct/finalization/custom-manager reentry; complete every command/plugin cold-copy, ownership, reference and passive interpreted function value-provider route. Continue all remaining original worklist requirements, including solver/estimator/simulator/subscriber/interface/measurement providers, body/kernel/NAIF/scientific clauses, output-name/cache transactions, inout aliases, custom vectors and arbitrary scope/interior/service/engine-reset/later lifetimes. Preserve full original scopes and verification deferral; continue independent POV and unresolved rotation research without inventing specifications.


Subscriber binding frames, 2026-11-06
----------------------------------------

Shared subscriber invocation bindings now stage independent wrapper views before detaching enclosing actual wrapper objects. Exact wrapper leases validate restored generations; axis aliases clone and retire once, old deletion queues restore with the frame, and active restored leases survive other-owner retirement. Lease pruning publishes its retained set before destructor callbacks; definition copies keep active leases cold. ReportFile and XYPlot preserve actual parameter-pointer lists with weak lifetime checks, restoring native slots before wrapper retirement callbacks. Function execution restores these frames after callee state and before caller continuation; inactive cached manager cleanup also scopes shared subscriber bindings, while executing cleanup uses its existing outer frame. Wrapper publication stages both axes before mutation, queues retirement outside live containers, and reports unmatched names. GmatFunction retains new wrappers until acceptance and throws on rejection. Ordinary binding restoration does not copy streams, native plot history or explicit global configuration and does not reinitialize commands. Changed parameter lists or retired bindings invalidate readiness and report restoration failure. Eight production paths compile. Other subscriber native references, shared Parameter interior/cache bindings, component-changing outputs, legacy/external-map ownership and every original full function/resource/scientific scope remain open. No row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-subscriber-binding-frames-implementation-20261106.rst``. Tracking-v60 unchanged; final integrated compilation passed; verification deferred.

Next: Complete binding frames for the full native subscriber catalog, including OrbitPlot/OrbitView/GroundTrackPlot, EphemerisFile and enabled plugin reference/cache fields, plus shared Parameter owner/RefData bindings without rolling back explicit Global values or histories. Complete passive caller resource/command-cache rebinding when outputs replace owned components while preserving live solver/loop state. Finish legacy/shared/external-map ownership, direct/finalization/custom-manager reentry, all command/plugin cold-copy/ownership/reference and interpreted function value-provider routes. Continue every remaining original worklist requirement, including all body/kernel/NAIF/scientific/resource clauses and independent POV/rotation research. Keep full original scopes and verification deferral.


Subscriber value commits, 2026-11-06
----------------------------------------

Successful whole-subscriber ObjectWrapper assignments now commit their actual target through every active enclosing binding scope. Temporary bindings still unwind, while an explicitly assigned resource value is retained. Self-identity assignments do not commit, and failed Copy does not signal success. Dismissed layered states keep captured owners until ordered scope retirement. Shared scope registration/restoration is exported from the subscriber module, matches actual lifetimes, and unregisters entries before releasing captures that can trigger nested cleanup. Function initialization now preserves every already initialized shared subscriber by actual global-store membership, including aliases in the local store; publisher selection uses object identity. Shared initialized ephemerides retain their force context, and shared initialized subscriber wrappers are reused. Cold/new/local resources retain native initialization/registration. Eight production paths compile. Full native value/reference/writer/renderer providers, direct API/copy hooks, self-mutating subscriber callbacks, Parameter interior/cache bindings and every original full function/resource/scientific scope remain open. No row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-subscriber-value-commits-implementation-20261106.rst``. Tracking-v61 unchanged; final integrated compilation passed; verification deferred.

Next: Complete cold/new/local subscriber value and reference providers across the full native catalog, including native writer/renderer rebinding, self-mutating subscriber callbacks and all direct API/native Copy paths. Finish shared Parameter owner/RefData and global-resource interior bindings; complete passive caller resource/command-cache repair when outputs replace owned components while preserving native values, histories, solver/loop state and explicit Global changes. Finish legacy/shared/external-map ownership, direct/finalization/custom-manager reentry, all command/plugin and interpreted function value-provider routes and arbitrary scope/interior/service/engine-reset/later lifetimes. Continue every remaining original worklist requirement and independent POV/rotation research. Preserve full original scope and verification deferral.


Subscriber output identities, 2026-11-06
---------------------------------------

Cold subscribers and private function plots now have independent physical output identities. Eight native plot implementations route updates, rename keys and cleanup through those identities while retaining script resource names. Native assignment preserves the destination output identity and pending rename; cold XY copies do not inherit a peer window flag. Main ObjectInitializer initialization adopts logical keys before output starts, function initialization retains private keys, and cold function cleanup cannot select a caller window by name. InitializeForRun records Subscriber and GmatBase readiness after the actual virtual initializer, without changing native false-result handling. Qt stores exact backend keys, resolves display aliases only for public viewer access, labels output items and windows with logical names and uses exact keys for native updates, cleanup and end-of-run clocks. Private view keys are excluded from saved main views and script projection writes. Seventeen production paths changed; configured consumers compile, while legacy OpenGlPlot is outside the configured source set. Full subscriber value/reference/history/writer/renderer providers and original broad requirements remain open. No row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-subscriber-output-identities-implementation-20261106.rst``. Tracking-v62 unchanged; final configured compilation passed; verification deferred.

Next: Complete cold/new/local subscriber value and reference providers across the full native catalog, including independent wrapper/native reference graphs, writer destinations, retained data and renderer rebinding. Finish direct Initialize/API/Copy ownership, partial-initialization cleanup, private Qt camera/vector/sensor/playback settings and legacy renderer labels. Complete shared Parameter owner/RefData and global-resource interiors, component-changing output/cache repair, self-mutating callbacks, legacy/shared/external-map ownership, direct/finalization/custom-manager reentry, all command/plugin and interpreted function providers, and arbitrary scope/interior/service/engine-reset/later lifetimes. Continue every original remaining body/kernel/NAIF/scientific/resource requirement and independent POV/rotation research. Preserve the complete worklist and verification deferral.


Subscriber native bindings, 2026-11-06
---------------------------------------

Native function binding frames now cover borrowed references in OrbitPlot, OrbitView, GroundTrackPlot, legacy OpenGlPlot, GroundTrack, DynamicDataDisplay, EphemerisFile, MatlabWorkspace and DataCallback. An identity-checked common helper stages scalars, pointer vectors/maps and publication metadata before wrapper detachment, restores stable member slots without retaining vector-element addresses, clears expired or reconfigured bindings and reports failure. Plot subtype layers restore before common wrapper retirement; explicit whole-value commits retain their existing dismissal path. Common frames also restore data coordinate/origin/provider pointers, current labels/provider ID/direction; GroundTrack restores matching column indices. Dynamic displays retain edited values/colors/layout while invalidating bindings when lexical cell roles change. Common Subscriber native copying now clones the X/Y wrapper graph once with alias preservation and stages it before retiring old wrappers. Twenty-one production paths changed. Configured consumers and optional DataCallback source compile; legacy OpenGlPlot has 39 standalone compiler errors identical to its preimage. Full value/reference/history/writer/renderer providers and original broad requirements remain open. No row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-subscriber-native-bindings-implementation-20261106.rst``. Tracking-v63 unchanged; final configured compilation passed; verification deferred.

Next: Complete subscriber cold/new/local value and reference providers across every native subtype, including native writer/renderer interiors, retained history/sampling references, owned frames, force-table/container lifetimes, callback userdata and independent wrapper-owned object graphs. Complete direct API/native Copy/Initialize and property-edit commit/rebinding, self-mutating callbacks, component-changing output/cache repair, shared Parameter owner/RefData and global-resource interiors. Finish legacy/shared/external-map ownership, direct/finalization/custom-manager reentry, all command/plugin and interpreted-function providers and arbitrary scope/interior/service/engine-reset/later lifetimes. Continue all remaining original body/kernel/NAIF/scientific/resource requirements and independent POV/rotation research. Preserve complete scope and verification deferral.


Subscriber value graphs, 2026-11-06
-----------------------------------

Four exact native subscriber types now disclose retained reference providers: ReportFile, XYPlot, DynamicDataDisplay and MessageWindow. Wrapper-owned object graphs register before native/EW references reconnect; repeated aliases and independently owned component identities are checked. Function value transfer, completed native Clone/Copy, retained copy construction and supported direct assignments use the graph binder. Failed copies retain source/destination wrapper leases and contexts through an actual copy token; successful completion commits binding scopes and detaches matching retirement state. Copy readiness remains cold, actual output identities stay with destinations, retained wrappers with matching declaration-role snapshots survive ordinary initialization, and live copies initialize output on first use. Changed ReportFile paths/modes close the previous stream; unchanged configurations preserve it. Subscriber graphs remain dormant during value preparation. Ephemeris common wrapper/environment binding now joins its existing specialized resource binder. Sixteen paths compile; full remaining subscriber/native output/lifetime/scientific scopes stay open. No row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-subscriber-value-graphs-implementation-20261106.rst``. Tracking-v64 unchanged; final configured compilation passed; verification deferred.

Next: Complete the remaining OrbitPlot/OrbitView/GroundTrackPlot/GroundTrack, owned-plot, TextEphemFile, MATLAB workspace and callback value providers, including renderer/native histories, owned frames/interpolators/writers, engine/session/force-container lifetimes and opaque callback ownership. Finish self-mutating subscriber evaluation, direct/configuration-property edits and arbitrary callbacks/old-origin/scope/interior/reset/later lifetimes; qualify private Qt camera/vector/sensor/playback copies, file destinations, native output activation and exceptional native mixtures. Complete shared Parameter owner/RefData/global-resource interiors, component-changing output/cache repair, legacy/shared/external-map ownership, direct/finalization/custom-manager reentry, all command/plugin and interpreted-function providers. Continue every original body/kernel/NAIF/scientific/resource requirement and independent POV/rotation research; preserve full scope and verification deferral.


Subscriber catalog providers, 2026-11-06
---------------------------------------

Eight more exact native subscriber classes now disclose retained value-reference providers: OrbitPlot, OrbitView, GroundTrackPlot, GroundTrack, OwnedPlot, TextEphemFile, MatlabWorkspace and DataCallback. Most-derived native Clone/Copy/copy-construction/assignment hooks reconnect completed resources under the existing copy-retirement protocol. Native geometry/Parameter/environment references map by identity; geometry readiness requires native bindings before retained wrappers bypass ordinary initialization. OwnedPlot copies its curve-style arrays, preserves ShowPlot and destination window ownership, uses inherited Subscriber parameter offsets and activates retained outputs on data use. TextEphemFile stages and owns independent interpolator copies, retires displaced interpolators and has an explicit typed Copy. Propagate publishes actual spacecraft-to-Cartesian-column metadata, including formation member ordinals; native state packets fill registered PSM ordinal blocks instead of first-name matches. Orbit/ground-track plots consume identity columns in ordinary and compact integration-step packets, preserving logical curve names and original origin-state metadata. Function argument renames and distinct same-name objects no longer rely on name matching for this data path. Callback function/userdata remain borrowed host configuration; MATLAB binding does not start an engine. Twenty-two paths compile, including standalone optional DataCallback. No row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-subscriber-catalog-providers-implementation-20261106.rst``. Tracking-v65 unchanged; configured and optional callback compilation passed; verification deferred.

Next: Finish original complete function input/output, scope and local/global resource behavior. Address real remaining subscriber gaps, including color/segment callbacks that still identify their subjects by name, public declaration/property changes, component replacement, shared-global retention of callee-local dependencies and evaluation reentry. Continue remaining callable/command/plugin/estimator/simulator and other resource providers; complete the remaining body/kernel/NAIF/scientific clauses and unresolved POV/rotation research. Follow native cold Clone/Initialize policies for independent output objects and documented Global semantics for continuous plot/report histories. Keep external host callback/session ownership explicit; do not invent deep ownership of opaque host userdata or copying of external output history as an original requirement. Preserve the full original worklist and verification deferral.


Subscriber event identities, 2026-11-06
--------------------------------------

Subscriber notifications now carry optional actual subject identities through new nonvirtual Publisher overloads while preserving legacy exported entry points and subscriber virtual callbacks. Frozen synchronous event scopes retain weak generation identities, distinguish authoritative empty/retired subjects from legacy names, consume requests before callbacks and restore outer events during reentry. Assignment, Vary, Propagate segment colors, impulsive/finite/file maneuvers and estimator/smoother updates pass their actual subjects. OrbitPlot translates those subjects to declared curve/color roles; GroundTrack preserves parallel satellite/station role names through binding, copies and binding frames, and matches segment subjects by identity. Ephemeris maneuver/property events and direct finite-burn cuts select their actual spacecraft; unrelated property events return before writer event-epoch changes. The scalar maneuver-name list is local to each call. Native ground tracks send actual station coordinates/colors and satellite palettes, resolve textures and publish their own display settings for private backend keys; Qt consumes that metadata without requiring a configured peer. Twenty source paths compile. Broad function/resource requirements remain open; no row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-subscriber-event-identities-implementation-20261106.rst``. Tracking-v66 unchanged; configured compilation passed; verification deferred.

Next: Complete remaining original function I/O, scope, resource cloning and command behavior, including public declaration/property edits, component replacement and dependencies retained by shared globals. Finish ephemeris publication/provider selection for scoped and same-name spacecraft, including covariance/attitude/provider-group metadata, and remaining OrbitView/private renderer state. Continue unresolved callable/command/estimator/simulator/resource providers and every remaining body/kernel/NAIF/scientific clause. Review the other original source-route rows against their exact text rather than treating every parent as finished by a narrow fix; retain explicit specification/external/historical dependencies and independent POV/rotation research. Keep native cold output copying and documented Global continuity distinct, preserve the unchanged original denominator/assertions/gates and defer runtime verification until the full implementation pass is finished.


Ephemeris publication identities, 2026-11-06
-------------------------------------------

Native spacecraft publication now has a shared identity/role order for covariance, acceleration and attitude declarations and data. Formation members resolve by actual native ordinal; distinct same-name spacecraft keep separate auxiliary blocks. Covariance/acceleration filling follows the declared all-covariances-then-all-accelerations order, and common propagation/estimator filling advances separate group offsets instead of overwriting earlier groups. A shared column builder maps Cartesian, covariance, acceleration, quaternion and PropSetup-definition identity metadata to each actual spacecraft. Ordinary Propagate, common propagation-enabled and estimator sampled/precise output use frozen synchronous columns; accepted integration samples reuse the same layout. EphemerisFile consumes actual Cartesian/covariance/quaternion columns with packet bounds checks and rejects same-name peers. Propagator changes use the matching spacecraft group and actual definition identity, distinguishing same-name definitions while avoiding false changes between command clones of one definition. Ephemeris copy/reset/swap paths handle the new cold identity fields. Estimator SPK target sampling now selects actual bound spacecraft, including Formation members. Ten paths compile; covariance values, force/integration math, SPK time-cell policy and acceptance gates remain unchanged. No row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-ephemeris-publication-identities-implementation-20261106.rst``. Tracking-v67 unchanged; configured compilation passed; verification deferred.

Next: Finish independent propagator-group epoch and coordinate-frame publication semantics, then remaining original function I/O/scoping, public declaration/property edits, component replacement and shared-global dependency retention. Complete remaining callable/command/estimator/simulator/resource and private renderer implementations. Review the other original source-route rows against their exact requirements, retain all ephemeris format/B-1/final-iterative and body/kernel/NAIF/scientific scopes, and research unresolved POV/rotation semantics without inventing specifications. Keep original specification/external/historical dependencies explicit and continue implementation before runtime verification.


Publication epochs and frames, 2026-11-06
-----------------------------------------

Spacecraft publication columns now carry each native PSM group epoch and actual internal coordinate frame with weak frame identity. Validated accessors expose those values; synchronous compact views preserve the original packet columns for origin-reference queries while nested legacy publications clear inherited context. Ephemeris retrieval uses the spacecraft epoch, SPK receives its actual precise epoch, propagator events use the matching group time, and incoming orbit/covariance/acceleration samples rebase into the writer's retained input frame before buffering. Orbit and ground-track plots partition typed publications by actual epochs, use source frames and native precise conversion overloads, retain original collection cadence and defer other groups' curves instead of marking them absent. Ordinary packet origin snapshots now accompany accepted-step snapshots; shared frame dependency traversal and availability checks prevent substituting an incoherent live spacecraft reference. Orbit metadata still observes skipped collection packets. Ten paths compile; propagation/force formulas, writer conversion policy, original assertions/tolerances and acceptance gates remain intact. No row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-publication-epochs-frames-implementation-20261106.rst``. Tracking-v68 unchanged; configured compilation passed; verification deferred.

Next: Finish the remaining original function I/O/scoping, public declaration/property edits, component replacement and shared-global dependency retention. Complete remaining callable/command/estimator/simulator/resource and private OrbitView renderer implementations. Continue exact source review of the other open source-route requirements, remaining body/kernel/NAIF and ephemeris B-1/final-iterative/scientific scopes. Review composite-point reference dependencies and unusual/custom internal frames against their native supported contracts, and retain external receiver/legacy publication limits explicitly. Research unresolved POV/rotation semantics without inventing specifications. Preserve the complete worklist and defer runtime verification until the implementation pass is complete.


MATLAB session contracts, 2026-11-07
-----------------------------------

MATLAB session selection now checks actual open results, ordinary function/workspace calls reuse the canonical default engine, unsupported private-session modes reject before mutation and legacy Mac setup publishes only a successfully configured handle. Native transport failures have a distinct exception; a call reconnects at most once at its command boundary instead of recursively executing from an input helper. Paths and character inputs use a common literal encoder for quotes and control characters; string-valued Parameters evaluate once without bracket-shaped value rejection. Returned MATLAB character arrays use owned untruncated UTF-16-to-UTF-8 conversion. Qt mode labels/help describe actual platform behavior. Six paths compile; full MATLAB I/O and mode requirements remain open. No row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-matlab-session-contracts-implementation-20261107.rst``. Tracking-v69 unchanged; configured compilation passed; verification deferred.

Next: Complete the remaining original function I/O/scoping, whole-object/property and output-staging behavior, public declaration edits, component replacement and shared-global dependency retention. Review the original MATLAB-to-GMAT restriction against supported callback/optimizer contracts without inventing a blanket command ban. Finish remaining callable/command/estimator/simulator/resource and private OrbitView renderer work, original body/kernel/NAIF and ephemeris format/B-1/final-iterative/scientific scopes. Resolve specification, external and historical-platform dependencies explicitly; continue POV/rotation research. Defer runtime verification until the implementation pass is complete.


MATLAB return values and native arguments, 2026-11-07
----------------------------------------------------

MATLAB calls now receive distinct reserved return variables, resolve argument/target indices once before input transfer, read and validate every supported Parameter result into owned native values, preserve named MATLAB workspace outputs and clean temporary variables before assigning GMAT destinations. Scalar/string/array-element/vector/matrix output uses typed engine data rather than formatted display text or ans. Scalar inputs evaluate once with locale-independent round-trip text; numeric vector/matrix inputs use explicit literals. Native array transfers own mxArrays, check transport results and validate dimension capacity before narrowing. Shared CallFunction argument lists now hold actual GmatBase pointers, preserving native whole-object inputs without invalid Parameter casts; Python checks actual Parameter types at use. Existing native formatted struct output remains staged as a legacy path, with whole-object/member transfer still unfinished. Seven paths compile. No row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-matlab-output-values-implementation-20261107.rst``. Tracking-v70 unchanged; configured compilation passed; verification deferred.

Next: Finish full typed MATLAB whole-object/member and literal/property interface behavior against the original specification, retaining the unresolved FRR-36 reference explicitly. Continue complete original function I/O/scoping, public declaration edits, component replacement, shared-global dependency retention and callable/command/estimator/simulator/resource/private renderer work. Complete remaining ephemeris B-1/final-iterative, body/kernel/NAIF and scientific scopes, and review original source-route requirements. Preserve specification/external/historical dependencies and continue POV/rotation research. Defer runtime verification until the implementation pass is complete.


Explicit Save source generation, 2026-11-07
-------------------------------------------

Save now requests explicit script export while checking attachment collisions and rendering resources. A native ObjectScriptExportScope follows nested serializers on the current thread, so the common creation-source filter no longer suppresses explicitly saved resources or owned components declared outside the main script. The scope restores on success/error before file I/O, supports nesting and does not mutate object source/force-generation flags. Its bypass applies only to script export modes; native object/parameter cloaking, read-only policy, dependency traversal, file naming and numerical behavior retain their existing contracts. Four paths compile; full any-Resource persistence and function/command scope remain open. No row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-save-explicit-sources-implementation-20261107.rst``. Tracking-v71 unchanged; configured compilation passed; verification deferred.

Next: Finish the original any-Resource persistence scope, including native default-reset behavior in appended snapshots, hidden/read-only state reconstruction, aliases/input modes, complete actual reference graphs and external-file portability. Continue complete function I/O/scoping and all supported commands/resources, typed MATLAB whole-object/member and literal/property transfer, public edits/component replacement/shared-global retention, estimator/simulator and private renderer work. Complete original body/kernel/NAIF and ephemeris format/B-1/final-iterative/scientific scopes; retain specification/external/historical dependencies and independent POV/rotation research. Defer runtime verification until the implementation pass is complete.


Save defaults and DE source files, 2026-11-07
--------------------------------------------

Explicit Save snapshots now retain writable values identified by native IsParameterEqualToDefault hooks, including default-cloaked roots and supported empty strings/lists, while preserving read-only and unrelated hidden-field policy. Empty DSN directory lists reach the existing native clear setter. Clearing a previous Earth EOP/ionosphere override resets its backend; already-empty fields and ordinary initialization preserve unrelated host settings. SolarSystem DEFilename getters/setters/default comparison share the active-or-most-recent DE source lookup while SPICE remains active. DE file creation validates/stages before replacing the prior reader, closes probes and actually constructs through a successful relative-path fallback. Five paths compile; full persistence, source activation, shared services and scientific/function scopes remain open. No row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-save-default-resets-implementation-20261107.rst``. Tracking-v72 unchanged; configured compilation passed; verification deferred.

Next: Complete the original any-Resource persistence requirement, including remaining hidden/read-only state reconstruction, class-specific serializers, quoting, aliases/input modes, reference/file ownership and external portability. Finish ephemeris-source activation failure semantics and shared host/global service ownership as part of the original scientific/function scope. Continue full function I/O/scoping and all supported commands/resources, typed MATLAB whole-object/member and literal/property transfer, public edits/component replacement/shared-global retention, estimator/simulator/private renderer work, original body/kernel/NAIF and ephemeris format/B-1/final-iterative/scientific requirements. Retain specification/external/historical dependencies and POV/rotation research. Preserve the complete worklist and defer runtime verification until the implementation pass is complete.


Ephemeris source transactions, 2026-11-07
----------------------------------------

SolarSystem source selection, DE/SPK filename edits and source-file binding now share a native rollback token. It snapshots source/order/file metadata and affected native body bindings/cache flags/epochs, restores them on failure and defers displaced DE-reader deletion across nested updates until the owning update commits. Associated SPK validation precedes source-flag mutation; candidate and cached DE readers bind consumers before success. CelestialBody validates filename/body-ID lookup before replacing its binding. Deprecated source spelling uses the same activation path. Non-SPICE SolarSystem source selection no longer indexes missing SPK names, rejects SPICE selection, and the existing reader getter has a forward declaration/null implementation without SPICE. Five paths compile, including two conditional syntax units; full cache/lifetime/scientific/persistence scope remains open. No row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-ephemeris-source-transactions-implementation-20261107.rst``. Tracking-v73 unchanged; configured compilation passed; verification deferred.

Next: Finish coherent state/acceleration cache invalidation on successful source, reader and related configuration changes, including reused epochs and precise-time/caller-frame restoration. Complete original source/file borrower lifetimes, scientific/body/kernel/NAIF and any-Resource persistence scopes, hidden/default/quote/reference/external-file behavior and all function I/O/scoping/command/resource/private renderer requirements. Continue typed MATLAB whole-object/member and literal/property transfer, public edits/component replacement/shared-global retention and estimator/simulator work. Preserve all ephemeris format/B-1/final-iterative clauses and specification/external/historical dependencies, and continue independent POV/rotation research. Keep the full worklist and defer runtime verification until the implementation pass is complete.


Ephemeris cache coherence, 2026-11-07
-----------------------------------

Native body ephemeris queries now use an explicit raw-state validity flag and a dedicated precise epoch, independent of public/frame query epochs. Both caller-buffer overloads delegate to canonical state queries and copy all six returned values. Self-origin zero queries invalidate raw state and compare actual pointers rather than names. Native source/file/time/interval/mu/elements/reference/NAIF/kernel edits invalidate dependent member caches; passive clones cannot broadcast through a borrowed SolarSystem pointer. SPICE reinitialization clears observer-ID readiness, and SolarSystem rebinding clears the borrowed kernel reader. Source transactions now retain all actual member cache values/epochs/readiness, including user bodies, and time/interval edits share native rollback. Seven source/help paths compile, including four conditional syntax units without SPICE. Original cadence and acceleration formulas/stencil are retained. Full scientific, kernel-pool, lifetime and persistence scope remains open; no row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-ephemeris-cache-coherence-implementation-20261107.rst``. Tracking-v74 unchanged; configured and conditional syntax compilation passed; verification deferred.

Next: Finish original scientific/body/kernel/NAIF behavior and source/file borrower lifetimes, including native kernel replacement/unloading and externally retained providers. Complete any-Resource Save/persistence scopes, hidden/default/quote/reference/external-file behavior, typed MATLAB whole-object/member and literal/property transfer, all original function I/O/scoping/public declaration edits/component replacement/shared-global retention, and command/estimator/simulator/resource/private renderer requirements. Preserve all ephemeris B-1/format/final-iterative clauses and specification/external/historical dependencies, and continue independent POV/rotation research. Keep the full worklist; defer runtime verification until implementation is complete.


Typed MATLAB object results, 2026-11-07
-------------------------------------

Native whole-object MATLAB results now use an owned typed scalar-structure snapshot from the C Engine rather than an 8192-byte formatted display capture. The snapshot preserves field/cell order, numeric shape, dense numeric/logical values and UTF-16 character data converted to UTF-8; oversized or unsupported values do not silently narrow. Writable native scalar/vector/matrix/time/Boolean/indexed-list/Generic properties stage values before setters. Nested scalar structures resolve actual native owned-component references, with lifetime/ownership checks before assignment; Spacecraft now declares its missing Attitude property object type. Unknown/read-only fields warn and skip, while malformed writable fields fail. Strings retain literal braces/quotes and precise MJD character input validates before native conversion. Six source/help paths compile, including legacy Mac conditional syntax. Main-script/formal argument rules and legacy whole-object input serialization remain; list resizing, complete reference/component semantics and original FRR-36 scope remain open. No row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-matlab-object-values-implementation-20261107.rst``. Tracking-v75 unchanged; configured/conditional syntax compilation passed; verification deferred.

Next: Complete original MATLAB whole-object input, literal/property and reference/component transfer against the supported native contracts, including indexed-list replacement/resizing and the unresolved FRR-36 reference. Review the remaining original source-route clauses against their complete text and supported providers so implementation completion is decided by those requirements. Continue all original function I/O/scoping/public declaration/component/shared-global, command/estimator/simulator/resource/private renderer, any-Resource Save, body/kernel/NAIF and ephemeris format/B-1/final-iterative/scientific scopes. Preserve specification/external/historical dependencies and continue independent POV/rotation research. Keep the full objective and defer runtime verification until the implementation pass is complete.


Native MATLAB object inputs, 2026-11-07
---------------------------------------

Native MATLAB object inputs now use a thread-local data-export scope that retains native field selection/prefixes, includes function-local declarations and stages supported property values in owned engine arrays/cells. Common, spacecraft, attitude, array and fmincon writers participate; nested serializers restore their old mode flags, comments/scripted methods are suppressed for this data export, and spacecraft representation elements are captured from their already-computed native values. A discarded spacecraft serialization prepass is skipped only for this export. Typed input construction preserves numeric/logical data, UTF-8 text converted to UTF-16 including NUL, and empty native values; inputs publish from fresh completed structures. String Parameters also use typed character transfer. Temporary reservation excludes declared input/output roots and the function name before allocating workspace slots. Eleven source/help paths compile, including legacy Mac conditional syntax. List replacement, reference/component rebinding, remaining class-specific transfer and original FRR-36 scope remain open; no row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-matlab-object-inputs-implementation-20261107.rst``. Tracking-v76 unchanged; configured/conditional syntax compilation passed; verification deferred.

Next: Finish original MATLAB indexed-list replacement/resizing and reference/component rebinding using actual native provider contracts; review remaining custom serializer/value families and the unresolved FRR-36 reference. Assess remaining source-route requirements against complete original clauses and supported providers, retaining explicit proof of any implementation-present classification. Continue complete original function I/O/scoping/public edits/component/shared-global, command/estimator/simulator/resource/private renderer, any-Resource Save, body/kernel/NAIF and ephemeris format/B-1/final-iterative/scientific scopes. Preserve specification/external/historical dependencies and continue independent POV/rotation research. Keep the full objective; defer runtime verification until the implementation pass is complete.


Native lists and references, 2026-11-07
-------------------------------------

The existing native SetField(StringArray) virtual entry point now performs typed updates instead of printing a malformed diagnostic and doing nothing. Complete kernel lists validate before swap; CelestialBody also supports complete validated DSN directory lists and invalidates ephemeris dependencies for kernel edits. Native braced kernel assignments use the same staged list path. Formation supports complete member-name replacement with native empty-state semantics, reports member object-type metadata and rebuilds its packed state after actual references bind. MATLAB object results use native typed list setters and staged known-type reference plans; unchanged names preserve actual existing private bindings, while changed names resolve through the active command scope and revalidate identity before assignment. Captured contexts remain leased through assignment. Nine source/help paths compile, including five non-SPICE conditional syntax units. Owned hardware/reference-list providers, explicit reference clearing and long-term dependency ownership across scopes remain open. No row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-native-list-references-implementation-20261107.rst``. Tracking-v77 unchanged; configured/conditional syntax compilation passed; verification deferred.

Next: Complete native owned hardware/reference-list replacement providers, including coupled tanks/thrusters and reporting/wrapper dependencies, then remaining native list families and explicit reference clearing. Finish dependency ownership when references change across function scopes, public declaration/component edits, shared globals and all original function I/O/scoping/command/resource obligations. Retain original FRR-36 and other specification dependencies. Continue remaining estimator/simulator/private renderer, any-Resource Save, body/kernel/NAIF/kernel-pool and ephemeris format/B-1/final-iterative/scientific scopes, along with other original source-route clauses. Preserve external/historical dependencies and independent POV/rotation research. Keep the complete objective and defer runtime verification until implementation is complete.


Spacecraft propulsion packets, 2026-11-07
---------------------------------------

Spacecraft now has a complete typed propulsion packet operation that prepares both owned tank/thruster lists, preserves retained component identities, clones new actual sources and resolves every thruster tank link before publishing. Native chemical/electric tank compatibility, initialized new hardware and mass properties are checked while staged. Known covariance/STM owners are remapped only to same-name exact-type replacements; referenced removals and firing-thruster retirement reject before publication. MATLAB object results group both propulsion fields into one native operation regardless of field order, retain omitted lists, preserve existing private same-name bindings and revalidate source identities. Eight source/help paths compile, with three passing non-SPICE syntax units; Spacecraft conditional syntax retains a pre-existing EphemManager.hpp SPICE-type declaration failure reproduced on its source preimage. Legacy incremental attachment paths, other owned component families and dependency lifetime across scopes remain open. No row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-spacecraft-propulsion-packets-implementation-20261107.rst``. Tracking-v78 unchanged; configured compilation passed; three conditional syntax units pass and the Spacecraft pre-existing dependency failure is reproduced on its preimage; verification deferred.

Next: Finish native owned power-system, plate and other-hardware replacement providers and the legacy incremental SetRefObject/SetHardware routes; complete the pre-existing EphemManager non-SPICE declaration/implementation boundary. Complete external borrower/dependency retention and rebinding across public declaration/component edits and function scopes, shared globals, reporting/wrapper and remaining list/reference-clear providers. Continue all original function I/O/scoping/command/resource clauses, unresolved FRR-36/specification dependencies, estimator/simulator/private renderer, any-Resource Save, body/kernel/NAIF/CSPICE-pool and ephemeris format/B-1/final-iterative/scientific scopes. Preserve external/historical dependencies and independent POV/rotation research. Keep the complete objective and defer runtime verification until implementation is complete.


Native propulsion binding and power systems, 2026-11-07
------------------------------------------------------

Native Spacecraft SetRefObject/SetHardware tank and thruster attachments now use staged propulsion replacement rather than removing old clones first and suppressing later tank-link errors. A private declaration mode preserves incremental pre-initialization binding, clears unresolved tank caches and requires the complete declared graph at initialization; initialized updates stay strict. SetupHardware prepares every tank-link vector before publishing. Removal actions validate the proposed propulsion graph and known owner references before retiring components or clearing the power system; cold tank reset preserves retained thruster identities. PowerSystem has a nonvirtual name/source replacement and explicit clear provider using staged exact-type clones, owner/Sun setup, initialized validation and known covariance/STM rebinding, with electric firing retirement guards. Native matching-source attachment and MATLAB PowerSystem results use that provider; empty/None clears explicitly. Six source/help paths compile. Other hardware/plate providers, cloned-object update/copy paths and cross-scope external borrower/context lifetime remain open. No row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-native-propulsion-binding-implementation-20261107.rst``. Tracking-v79 unchanged; configured compilation passed; verification deferred.

Next: Finish native plate/other-hardware replacement, including radio/antenna/error-model interconnections, and existing UpdateClonedObject/UpdateClonedObjectParameter/copy/whole-spacecraft assignment paths without slicing or stale dependencies. Complete external borrower/context ownership across public declaration/component edits and function scopes, shared globals, reporting/wrapper and remaining list/reference-clear providers. Continue all original function I/O/scoping/command/resource clauses, unresolved FRR-36/specification dependencies, estimator/simulator/private renderer, any-Resource Save, body/kernel/NAIF/CSPICE-pool and ephemeris format/B-1/final-iterative/scientific scopes, including the pre-existing EphemManager non-SPICE declaration/implementation boundary. Preserve external/historical dependencies and independent POV/rotation research. Keep the full objective and defer runtime verification until implementation is complete.


Concrete owned clone updates, 2026-11-07
---------------------------------------

Spacecraft owned whole-object updates now stage concrete virtual Clone/Copy values instead of assigning through FuelTank/Thruster/PowerSystem/Hardware bases; plates are handled independently of Hardware inheritance. Single-property updates stage from the current owned value and use typed CopyParameter, preserving other runtime data. Owner/frame/body/tank/Sun/shadow/primary-antenna bindings are prepared using actual owned references, validated native initialization and existing firing-state rules. Existing component identities survive publication; tank updates prepare native spacecraft mass properties. Native copy guards defer completed-root context hooks during owned publication, while selected previous dependency contexts remain leased. CopyParameter requires concrete type/label/type agreement, handles supported scalar/time/enum/reference/list/vector/matrix/Generic values and reports missing providers instead of silently ignoring types. Thruster Tank and solar ShadowBodies have complete typed list providers. FuelTank base assignment no longer invokes a derived initializer before leaf fields copy. Sandbox reinitialization now honors invalidation after single-property as well as whole updates and checks initialization success. Twelve source/help paths compile. Notification provenance, other component/list/graph providers and original scopes remain open. No row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-owned-clone-updates-implementation-20261107.rst``. Tracking-v80 unchanged; configured compilation passed; verification deferred.

Next: Complete clone-notification provenance/actual-source selection for same-name private values; finish remaining native plate/other-hardware replacement and full radio/antenna/error-model/owned-FOV graphs, whole-spacecraft assignment and remaining callable/command/resource clone owners. Complete missing typed resize/reference providers and explicit unsupported value types where native APIs lack a whole-value getter; preserve original type contracts. Finish cross-scope external borrower/context ownership, public declaration/component edits, shared globals, reporting/wrappers, all original function I/O/scoping/command/resource clauses and unresolved FRR-36/specification dependencies. Continue estimator/simulator/private renderer, any-Resource Save, body/kernel/NAIF/CSPICE-pool and ephemeris format/B-1/final-iterative/scientific scopes, including the existing EphemManager non-SPICE boundary. Preserve external/historical dependencies and POV/rotation research. Keep the full objective and defer runtime verification until implementation is complete.


Actual clone-source identities, 2026-11-07
-----------------------------------------

Native owned-clone metadata now records actual source and owner pointers with weak lifetime identities in the existing GmatBase registry, without instance layout/virtual-slot changes. Construction inherits the source edge but not a live owner subscription; assignment preserves the destination edge. Spacecraft tank/thruster/power/plate/other-hardware attachments and owned copies register their actual source/containing owner, and clone updates match actual sources instead of equal names. Attitude updates also require an actual relationship; direct owned-attitude edits preserve the current source object. Function-value graph binding maps source-definition edges through exact copied/caller/shared-global identities and retains private definitions in the value graph. FCS execution delivers updates to registered live owners, including actual caller/global spacecraft outside the FOS; reinitialization does not replace shared/caller bindings through unrelated local names. Explicit owner activation keeps copied solver/propagation snapshots frozen while scene association activates real owners. Nine source/help paths compile. Other native clone-owner provenance/delivery and all remaining original scopes stay open. No row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-clone-source-identities-implementation-20261107.rst``. Tracking-v81 unchanged; configured compilation passed; verification deferred.

Next: Extend actual-source tracking and scoped clone notification handling to PropSetup, solver/propagation commands, ODE/force, station/plate, estimator/simulator/smoother and other native clone owners; preserve cold/live/snapshot and shared-global semantics. Finish remaining plate/other-hardware replacement, radio/antenna/error-model/owned-FOV graphs, whole-spacecraft assignment, typed resize/reference providers and public declaration/component lifetime. Complete cross-scope external borrower/context ownership, shared globals, reporting/wrappers, original function I/O/scoping/command/resource clauses and unresolved FRR-36/specification dependencies. Continue private renderer, any-Resource Save, body/kernel/NAIF/CSPICE-pool and ephemeris format/B-1/final-iterative/scientific scopes, including the existing EphemManager non-SPICE boundary. Preserve external/historical dependencies and POV/rotation research. Keep the full objective and defer runtime verification until implementation is complete.


Actual engine clone identities, 2026-11-07
-----------------------------------------

PropSetup now records actual component sources and containment, preserving inherited definition edges for independent native copies and reparenting replacement children. Runtime Propagate copies bind to the actual setup and its actual engine children. Same-concrete-type component replacements retarget weak source edges to the prepared replacement before retiring the old component, preserving dependent source selection regardless of owner delivery order. Nested source matching validates weak identities and stops indirect traversal at inactive snapshots; function notifications select the outer active containing owner once. Propagate whole/property updates select actual sources, retain command propagation state during setup configuration copies, refresh borrowed component arrays and invalidate native engine preparation. Concrete propagator Clone replaces unsupported/sliced base Copy, and Type-property updates prepare their coupled component pair before publication. Native preparation reconnects the physical model. Target/Optimize local solvers register actual source/owner identity and use it in whole/property matching, preserving existing native scalar lifecycle policy. Commands deactivate during initialization and function-binding retirement; runtime-owned setups do not remain independent active subscribers. Eleven source/help paths compile. Other native clone owners and original broader clauses remain open; no row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-engine-clone-identities-implementation-20261107.rst``. Tracking-v82 unchanged; configured compilation passed; verification deferred.

Next: Complete native clone-source/update providers for estimator/simulator/smoother, ODE/force, station/plate and other owners, including active solver configuration lifecycle and native propagation-group reconstruction limits. Finish plate/other-hardware replacement, radio/antenna/error-model/owned-FOV graphs, whole-spacecraft assignment, typed resize/reference providers and public declaration/component lifetime. Finish original function I/O/scoping/command/resource clauses, cross-scope external borrower/context ownership, shared globals and reporting/wrappers; retain unresolved FRR-36/specification dependencies. Continue private renderer, any-Resource Save, body/kernel/NAIF/CSPICE-pool and ephemeris format/B-1/final-iterative/scientific scopes, including the existing EphemManager non-SPICE boundary. Preserve external/historical dependencies and POV/rotation research. Keep the full objective; defer runtime verification until implementation is complete.


Native navigation propagation definitions, 2026-11-07
-----------------------------------------

Estimator and Simulator resource propagation definitions now record actual source/containing-owner relationships. Guarded native copies reject borrowed/incompatible results and retain existing dependency leases. Constructors keep setup copies guarded through completion; assignment prepares the complete setup list before replacing current values, clears stale satellite maps and rebinds the measurement manager to the destination list/map. Typed setup attachment prepares cloned values and map changes before publication, supports out-of-order cold association slots, and retargets compatible weak definition edges before retirement. Whole/property updates use the native PropSetup provider and forward actual intermediate setup-root changes without name matching. Matching changes invalidate resource readiness and satellite mappings; unrelated resources do not trigger blanket unfinished-type exceptions. Public lookups reject unresolved cold slots before dereferencing them. Fresh scene/association owners activate notifications while native copied values stay inactive. Nine source/help paths compile. RunEstimator/RunSimulator command delivery/restart, full measurement/filter graphs and all broader original scopes remain open; no row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-navigation-propagation-definitions-implementation-20261107.rst``. Tracking-v83 unchanged; configured compilation passed; verification deferred.

Next: Complete RunEstimator/RunSimulator source registration, guarded native solver/group clones and staged definition updates with restart at command execution boundaries, preserving delayed observation initialization and repeated/nested invocation semantics. Scope event-reference cleanup before retiring command engines, including the existing EstimationRootFinder null/replacement defect. Finish smoother/filter, measurement/tracking/error/noise graphs and actual participant/group identities. Continue other native clone owners, plate/other-hardware replacement, whole-spacecraft assignment, typed resize/reference providers and public declaration/component lifetime. Finish original function I/O/scoping/command/resource clauses, cross-scope borrower/context ownership, shared globals and reporting/wrappers; retain unresolved FRR-36/specification dependencies. Continue private renderer, any-Resource Save, body/kernel/NAIF/CSPICE-pool and ephemeris format/B-1/final-iterative/scientific scopes, including the existing EphemManager non-SPICE boundary. Preserve external/historical dependencies and POV/rotation research; defer runtime verification until implementation is complete.


Navigation command restarts, 2026-11-07
-----------------------------------------

RunEstimator and RunSimulator now stage related native whole/property definition updates in owned pending resources instead of throwing blanket unfinished-type errors or mutating the running engine. Exact scoped root/source relationships and owned setup/component identities select updates; pending changes accumulate and completed estimator result sequence is invalidated. Execute rebuilds from the staged definition at its native boundary, retaining delayed observation initialization and repeated invocation policy. Resource/group clones are guarded and retain actual source/owner identity; groups share by actual setup identity while ephemeris participants retain separate native groups. Supplied data-file roots are cloned before measurement-manager adoption. Native initialization false/exception paths leave commands unready and preserve definition subscriptions for correction/retry; propagation initialization errors are no longer swallowed. Commands expose owned engine values, retire their own event registrations before deleting engines and release function bindings while their FOS is live. EstimationRootFinder now stages replacement snapshots, handles null clear, retires old snapshots and keeps snapshots inactive; EventManager releases borrowed measurement events without deleting their owners and clears only matching setup registrations. Owned setup-list copies remap internal source edges and retain component contexts. Twelve source/help paths compile. Full measurement/filter/participant/private-value graphs and all remaining original broader scopes stay open; no row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-navigation-command-restarts-implementation-20261107.rst``. Tracking-v84 unchanged; configured compilation passed; verification deferred.

Next: Finish full smoother/filter and measurement/tracking/error/noise/participant ownership graphs, independent function value providers and external borrower/context lifetime, including native source/stream update providers beyond propagation. Complete queued/pending configuration remapping across changed owned lists, source declaration/component replacement, active solver/group reconstruction and arbitrary provider failure cleanup. Continue remaining clone owners, plate/other-hardware replacement, whole-spacecraft assignment and typed resize/reference providers. Finish all original function I/O/scoping/command/resource and nested Target/Optimize clauses, shared globals and reporting/wrappers; retain unresolved FRR-36/specification dependencies. Continue private renderer, any-Resource Save, body/kernel/NAIF/CSPICE-pool and ephemeris format/B-1/final-iterative/scientific scopes, including the existing EphemManager non-SPICE boundary. Preserve external/historical dependencies and POV/rotation research; defer runtime verification until implementation is complete.


Owned measurement definitions, 2026-11-07
----------------------------------------

MeasurementManager now owns native TrackingFileSet definitions and their filter values instead of sharing tracking sets and generated adapters across copied estimators/simulators. Native copies rebuild measurement products, streams, counters and runtime associations through initialization, preserving filter value-copy policy. Parent copies rebind the manager to destination propagation lists and remap copied estimation-state registrations by actual source identity. Definition replacement retains old tracking generations through estimation-state remapping. TrackingFileSet stages typed filter copies, retires generated adapters through its ownership tracker and no longer double-deletes adapters or retains stale filters on assignment. Whole/property notifications select actual roots/filters, including direct owned edits, and invalidate preparation. Smoother filter copies and propagation attachments are guarded, its manager/state registrations target owned definitions, and native reset explicitly restores forward filtering entry. Navigation commands recognize these owned measurement/filter sources. Twenty source/help paths compile. Full participant/hardware/error/noise/navigation-state/private-value graphs, standalone adapter ownership, changed-list remapping and all broader original clauses remain open; no row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-owned-measurement-definitions-implementation-20261107.rst``. Tracking-v85 unchanged; configured compilation passed; verification deferred.

Next: Finish independent measurement participant/error/noise/hardware/navigation-state function value graphs and standalone adapter providers, source/stream updates beyond tracking/filter definitions, changed-list configuration remapping and external borrower/context lifetime. Complete smoother/filter active-state reconstruction and arbitrary provider failure cleanup. Continue remaining clone owners, plate/other-hardware replacement, whole-spacecraft assignment and typed resize/reference providers. Finish all original function I/O/scoping/command/resource and nested Target/Optimize clauses, shared globals and reporting/wrappers; retain unresolved FRR-36/specification dependencies. Continue private renderer, any-Resource Save, body/kernel/NAIF/CSPICE-pool and ephemeris format/B-1/final-iterative/scientific scopes, including the existing EphemManager non-SPICE boundary. Preserve external/historical dependencies and POV/rotation research; defer runtime verification until implementation is complete.


Navigation function values, 2026-11-07
----------------------------------------

Simulator now supplies a concrete native function-value provider and exposes its owned propagation/tracking definitions to the function context. TrackingFileSet exposes owned filters and maps actual participant/propagator references into the retained private/caller graph, including resolved participant aliases in signal paths and propagation assignments. Generic native value-child disclosure is separate from script serialization ownership; registration validates corresponding shapes and binds owned setups through their complete provider. Private spacecraft/station/hardware/error/noise dependencies reuse existing native binders. Function binding keeps tracking sets and Simulator dormant, with streams and adapters rebuilt by native command preparation. Simulator Clone/Copy rebind retained contexts and reject incompatible concrete Copy types. Thirteen source/help paths compile. Estimator/smoother function-value providers, complete active state/result reconstruction, changed-list remapping, external borrower lifetime and all broader original scopes remain open; no row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-navigation-function-values-implementation-20261107.rst``. Tracking-v86 unchanged; configured compilation passed; verification deferred.

Next: Complete estimator and smoother native function-value providers, including owned setup/tracking/filter and retained estimation/navigation state graphs. Finish native source/stream/participant/error/noise/hardware updates, changed-list configuration remapping, external borrower/context lifetime and arbitrary provider failure cleanup. Continue remaining clone owners, plate/other-hardware replacement, whole-spacecraft assignment and typed resize/reference providers. Finish all original function I/O/scoping/command/resource and nested Target/Optimize clauses, shared globals and reporting/wrappers; retain unresolved FRR-36/specification dependencies. Continue private renderer, any-Resource Save, body/kernel/NAIF/CSPICE-pool and ephemeris format/B-1/final-iterative/scientific scopes, including the existing EphemManager non-SPICE boundary. Preserve external/historical dependencies and POV/rotation research; defer runtime verification until implementation is complete.


Estimator function values, 2026-11-07
----------------------------------------

BatchEstimator, ExtendedKalmanFilter and Smoother now opt in through concrete native function-value providers. Owned setup/tracking/estimation-filter/physical-model definitions and sequential/smoother restoration snapshots are disclosed to the private/caller graph. Estimation-state objects, solve-for/consider/participant declarations and actual propagation roles are rebound while prepared engine associations remain dormant. Native copies retain consider declarations and guarded owned filters; assignment clears retired plotting references and correctly resets the state-covariance pointer. Estimator association exposes and clones actual filter references; initialization distinguishes duplicate declarations from existing bindings and prepares the filter list before publication. Sequential copies initialize smoothing pointers, clear live measurement/process-noise associations and own copied buffers/statistics. Smoothers clone supplied restoration buffers, retain copied forward-pass data and reconnect their owned filter to destination data. Estimation snapshots prepare both internal/external collections before replacement, reject known borrowed clones and remain inactive. Native Batch/EKF/Smoother Clone/Copy hooks rebind dependency contexts and reject incompatible Copy types. Twenty-one source/help paths compile. Complete active-session/result reconstruction, snapshot alias/source-role remapping, broader definition updates, external borrower lifetime and all remaining original scopes remain open; no row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-estimator-function-values-implementation-20261107.rst``. Tracking-v87 unchanged; configured compilation passed; verification deferred.

Next: Finish active estimator/smoother state/result and snapshot alias/source-role remapping, including public restoration-buffer contracts and original full function value behavior. Complete native source/stream/participant/error/noise/hardware and top-level filter/physical-model updates, pending configuration remapping across changed owned lists, external borrower/context lifetime and arbitrary provider failure cleanup. Continue remaining clone owners, plate/other-hardware replacement, whole-spacecraft assignment and typed resize/reference providers. Finish all original function I/O/scoping/command/resource and nested Target/Optimize clauses, shared globals and reporting/wrappers; retain unresolved FRR-36/specification dependencies. Continue private renderer, any-Resource Save, body/kernel/NAIF/CSPICE-pool and ephemeris format/B-1/final-iterative/scientific scopes, including the existing EphemManager non-SPICE boundary. Preserve external/historical dependencies and POV/rotation research; defer runtime verification until implementation is complete.


Restoration targets, 2026-11-07
----------------------------------------

Frozen estimation snapshots now record a weak actual restoration destination separately from live clone-definition edges. Native capture/copies preserve that role; ordinary resource assignment does not turn the destination into a snapshot. Function contexts remap snapshot targets and retain unlisted non-global command-owned targets as private dependencies. Restoration validates the complete target/type/uniqueness correspondence, supports reordered buffers and unique same-definition sibling copies by actual source ancestry, and rejects missing/expired/ambiguous destinations. Prepared values retain destination names/full names; concrete Copy is exercised on detached probes before live application, replacing positional base-class slicing. FileThrust now has a concrete Copy provider and retained-context Clone hook. Ten source/help paths compile. Live parent/child borrower coherence after full native Copy, complete active-session reconstruction, provider-failure rollback and all broader original scopes remain open; no row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-restoration-targets-implementation-20261107.rst``. Tracking-v88 unchanged; configured compilation passed; verification deferred.

Next: Repair live restoration of spacecraft/station/ODE and other parent values without leaving borrowed child pointers stale after concrete Copy. Finish active estimator/smoother state/result and snapshot role remapping, public restoration-buffer contracts, covariance/STM/native engine reconstruction and original full function value behavior. Complete source/stream/participant/error/noise/hardware and top-level filter/physical-model updates, changed-list configuration remapping, external borrower/context lifetime and arbitrary provider/file failure cleanup. Continue remaining clone owners, plate/other-hardware replacement, whole-spacecraft assignment and typed resize/reference providers. Finish all original function I/O/scoping/command/resource and nested Target/Optimize clauses, shared globals and reporting/wrappers; retain unresolved FRR-36/specification dependencies. Continue private renderer, any-Resource Save, body/kernel/NAIF/CSPICE-pool and ephemeris format/B-1/final-iterative/scientific scopes, including the existing EphemManager non-SPICE boundary. Preserve external/historical dependencies and POV/rotation research; defer runtime verification until implementation is complete.


Station restoration, 2026-11-07
----------------------------------------

Native restoration now has an explicit scoped mode for supported stable owner graphs. GroundStation restores existing hardware, receiver and signal-path error models in place after validating declarations, exact component types and owned role shapes. Receiver preserves owned error-model roots; Imager preserves private FOV roots and borrowed FOV references; sensor signal presence checks prevent storage replacement, and RF hardware retains selected antenna references. Component names/full names and covariance owner links are retained. Receiver/private-FOV children are disclosed to function-value binding. For unchanged body/geometry, BodyFixedPoint retains actual frames, loaded/generated kernels and selected body/solar-system bindings; SpacePoint retains active frame/kernel/origin handles while copying captured base/display values. Base restoration preserves declaration provenance and automatic-global status. Sixteen source/help paths compile. Spacecraft/ODE/other parent graphs, covariance cache views and internal borrowed buffers, changed geometry/topology, active-session reconstruction, arbitrary provider rollback and all broader original scopes remain open; no row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-station-restoration-implementation-20261107.rst``. Tracking-v89 unchanged; configured compilation passed; verification deferred.

Next: Implement identity-preserving live restoration for spacecraft, ODE/force and remaining parent graphs. Preserve or reconnect all hardware/force children, covariance/STM/native propagation and measurement borrowers, internal arrays/caches and source/context links; do not infer complete runtime correctness from the station path. Finish changed body/location/topology restoration at valid native preparation boundaries, active estimator/smoother state/result and snapshot role remapping, public restoration-buffer contracts and full original function value behavior. Complete source/stream/participant/error/noise/hardware and top-level filter/physical-model updates, changed-list configuration remapping, external borrower/context lifetime and arbitrary provider/file failure cleanup. Continue remaining clone owners, plate/other-hardware replacement, whole-spacecraft assignment and typed resize/reference providers. Finish all original function I/O/scoping/command/resource and nested Target/Optimize clauses, shared globals and reporting/wrappers; retain unresolved FRR-36/specification dependencies. Continue private renderer, any-Resource Save, body/kernel/NAIF/CSPICE-pool and ephemeris format/B-1/final-iterative/scientific scopes, including the existing EphemManager non-SPICE boundary. Preserve external/historical dependencies and POV/rotation research; defer runtime verification until implementation is complete.


Spacecraft restoration, 2026-11-07
------------------------------------------

Spacecraft now discloses its direct attitude, power, tank, thruster, hardware and plate children and restores compatible captured values into their existing roots under the native restoration guard. GmatState validates unchanged state roles and restores values/epochs without retiring state, ID or association storage; Spacecraft retains same-layout full STM/A-matrix storage. Selected spacecraft environment, external STM and solve-for references, thruster tank/frame/owner and burn bindings, power-system owner/shadow bindings, attitude owners/frames and AEM/SPICE readers, and plate history readers remain selected. Changed declarations, layouts, files, shadow or firing roles fail with a native preparation requirement. Ordinary assignment paths retain their existing behavior. Sixteen source/help paths compile. ODE/force and other owner graphs, covariance matrix/subview lifetime and posterior semantics, changed topology/preparation, complete active-session reconstruction and all broader original scopes remain open; no row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-spacecraft-restoration-implementation-20261107.rst``. Tracking-v90 unchanged; configured compilation passed; verification deferred.

Next: Implement identity-preserving restoration for ODE/force and remaining parent graphs, including native state/derivative/Jacobian storage and propagation/measurement borrowers. Finish covariance matrix/subview lifetime and posterior semantics; changed spacecraft/body/location/topology at native preparation boundaries; complete active estimator/smoother state/result and snapshot role remapping; public restoration-buffer contracts and full original function value behavior. Complete source/stream/participant/error/noise/hardware and top-level filter/physical-model updates, changed-list configuration remapping, external borrower/context lifetime and arbitrary provider/file failure cleanup. Continue remaining clone owners, plate/other-hardware replacement, whole-spacecraft assignment and typed resize/reference providers. Finish all original function I/O/scoping/command/resource and nested Target/Optimize clauses, shared globals and reporting/wrappers; retain unresolved FRR-36/specification dependencies. Continue private renderer, any-Resource Save, body/kernel/NAIF/CSPICE-pool and ephemeris format/B-1/final-iterative/scientific scopes, including the existing EphemManager non-SPICE boundary. Preserve external/historical dependencies and POV/rotation research; defer runtime verification until implementation is complete.


Physical-model restoration, 2026-11-07
------------------------------------------

PhysicalModel now captures separate translated/raw state and optional mass/time Jacobian columns with independent guarded owned arrays, preserving raw/model alias relationships. Ordinary assignment prepares every numerical buffer before replacement and destruction retires both Jacobian columns as well. Scoped restoration validates unchanged dimensions, state/derivative roles, storage presence/alias layout and exposed environment identities; it copies numerical values and epochs into current storage while retaining current propagation/state/space-object/body/solar-system and ownership bindings. PointMassForce retains readiness for unchanged roles. HarmonicField/GravityField retain prepared files/models and frames for unchanged potential/tide settings. RelativisticCorrection retains current frames/Sun/EOP, and now copies solar mu. Gravity and relativistic copy constructors stop duplicating inherited derivative/type declarations. Nine source/help paths compile. ODE force-list restoration, drag/SRP/finite/file-thrust runtime providers, covariance caches/posterior semantics, changed topology/preparation and all broader original scopes remain open; no row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-physical-model-restoration-implementation-20261107.rst``. Tracking-v91 unchanged; configured compilation passed; verification deferred.

Next: Complete ODEModel restoration without retiring its forces/internal frames or prepared propagation/measurement borrowers. Implement the remaining drag/SRP/finite/file-thrust providers, including captured internal arrays, atmosphere/shadow/file/interpolator/burn bindings and snapshot preparation; do not infer full force-model restoration from the shared physical-model provider. Finish covariance matrix/subview lifetime and posterior semantics; changed spacecraft/body/location/topology at native preparation boundaries; complete active estimator/smoother state/result and snapshot role remapping; public restoration-buffer contracts and full original function value behavior. Complete source/stream/participant/error/noise/hardware and top-level filter/physical-model updates, changed-list configuration remapping, external borrower/context lifetime and arbitrary provider/file failure cleanup. Continue remaining clone owners, plate/other-hardware replacement, whole-spacecraft assignment and typed resize/reference providers. Finish all original function I/O/scoping/command/resource and nested Target/Optimize clauses, shared globals and reporting/wrappers; retain unresolved FRR-36/specification dependencies. Continue private renderer, any-Resource Save, body/kernel/NAIF/CSPICE-pool and ephemeris format/B-1/final-iterative/scientific scopes, including the existing EphemManager non-SPICE boundary. Preserve external/historical dependencies and POV/rotation research; defer runtime verification until implementation is complete.


SRP and thrust restoration, 2026-11-07
------------------------------------------

SolarRadiationPressure copies now own independent Sun/force buffers and capture numeric shadow caches; ordinary assignment stages replacements and retires existing buffers, and destruction releases the Sun vector regardless of analytic mode. Its scoped restoration validates unchanged configuration/vector shapes and retains current buffers, shadow object and Sun bindings while applying captured numerical values. FiniteThrust native clones retain weak checked burn references and propagation roles; scoped restoration validates actual live burn/target identities and keeps current connections. FileThrust initializes interpolation scratch, captures scratch and independent interpolator objects in native clones, and restores into existing interpolators while retaining matching actual profile/spacecraft/frame bindings. Six source/help paths compile. ODE whole-graph restoration, drag/atmosphere, full independent burn/profile checkpoints, internal interpolator/covariance views, changed topology/preparation and all broader original scopes remain open; no row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-srp-thrust-restoration-implementation-20261107.rst``. Tracking-v92 unchanged; configured compilation passed; verification deferred.

Next: Finish drag/atmosphere restoration and its captured numerical buffers and selected atmosphere/frame/weather state. Complete ODEModel restoration without retiring its forces/internal frames or prepared propagation/measurement borrowers, including previous-state/dynamic-role and nonanalytic derivative values. Finish independent external burn/profile checkpoint values and raw/interpolator view lifetime; do not infer whole force-model restoration from leaf providers. Finish covariance matrix/subview lifetime and posterior semantics; changed spacecraft/body/location/topology at native preparation boundaries; complete active estimator/smoother state/result and snapshot role remapping; public restoration-buffer contracts and full original function value behavior. Complete source/stream/participant/error/noise/hardware and top-level filter/physical-model updates, changed-list configuration remapping, external borrower/context lifetime and arbitrary provider/file failure cleanup. Continue remaining clone owners, plate/other-hardware replacement, whole-spacecraft assignment and typed resize/reference providers. Finish all original function I/O/scoping/command/resource and nested Target/Optimize clauses, shared globals and reporting/wrappers; retain unresolved FRR-36/specification dependencies. Continue private renderer, any-Resource Save, body/kernel/NAIF/CSPICE-pool and ephemeris format/B-1/final-iterative/scientific scopes, including the existing EphemManager non-SPICE boundary. Preserve external/historical dependencies and POV/rotation research; defer runtime verification until implementation is complete.


Drag and atmosphere restoration, 2026-11-07
------------------------------------------

DragForce native copies now capture independent density, prefactor and translated-state buffers, numerical spacecraft/coefficient vectors and an exact-type guarded owned atmosphere. The internal atmosphere is disclosed through the existing owned-role interface, receives the new force-local Sun/body vectors and keeps source/context metadata. Ordinary assignment retires old arrays/atmosphere; scoped restoration preserves current buffers/vectors, atmosphere root, angular-velocity pointer and actual frames after validating unchanged propagation/configuration roles. AtmosphereModel captures weather samples, angular velocity, geodetic/GHA values and an independent loaded flux reader; restoration keeps selected reader/body/frame/vector bindings. SolarFluxReader copies preserve loaded data/coverage/index metadata and do not borrow its temporary parse-line pointer. Exponential tables clone with guarded arrays and restore in place; SimpleExponential and NRLMSISE00 use concrete checked Copy, and NRL file-state values persist. Eleven source/help paths compile. Whole ODE restoration, shared external atmosphere snapshots/lifetimes, independent burn/profile checkpoints, internal/external covariance/reader/interpolator views, changed topology/preparation and all broader original scopes remain open; no row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-drag-atmosphere-restoration-implementation-20261107.rst``. Tracking-v93 unchanged; configured compilation passed; verification deferred.

Next: Complete ODEModel restoration without retiring force roots/internal frames or prepared propagation/measurement borrowers, including previous-state/dynamic-role/nonanalytic derivative values. Finish shared external atmosphere snapshot/lifetime/value contracts, independent burn/profile checkpoint values and raw/table/interpolator view lifetime; do not infer whole force-model restoration from leaf providers. Finish covariance matrix/subview lifetime and posterior semantics; changed spacecraft/body/location/topology at native preparation boundaries; complete active estimator/smoother state/result and snapshot role remapping; public restoration-buffer contracts and full original function value behavior. Complete source/stream/participant/error/noise/hardware and top-level filter/physical-model updates, changed-list configuration remapping, external borrower/context lifetime and arbitrary provider/file failure cleanup. Continue remaining clone owners, plate/other-hardware replacement, whole-spacecraft assignment and typed resize/reference providers. Finish all original function I/O/scoping/command/resource and nested Target/Optimize clauses, shared globals and reporting/wrappers; retain unresolved FRR-36/specification dependencies. Continue private renderer, any-Resource Save, body/kernel/NAIF/CSPICE-pool and ephemeris format/B-1/final-iterative/scientific scopes, including the existing EphemManager non-SPICE boundary. Preserve external/historical dependencies and POV/rotation research; defer runtime verification until implementation is complete.


Whole force-model restoration, 2026-11-07
------------------------------------------

ODEModel now discloses its direct force children and restores unchanged prepared graphs through concrete in-place force Copy. Current force owners/subscriptions, internal frames, PSM/state/dynamic associations and acceleration-reporting registry remain selected. Snapshots capture previous state, satellite/dynamic roles, mu and nominal derivative values; solve-for holders and covariance owners map by actual force/owned-child correspondence rather than borrowed source components. Scoped restoration validates unchanged layout, force/body/solve-for/dynamic roles and restores previous state and derivative values into current storage. Native copies enforce exact force types, carry solve-for metadata/parameter count and rebind holders after function force registration. Acceleration-reporting child disclosure retains its existing transient exclusion. ExternalModel retains selected module/Python bindings for unchanged configuration; PolyhedronGravityModel retains its current mesh/body while restoring captured values, uses checked Copy and initializes captured times. Eight source/help paths compile. Shared external atmosphere/burn/profile values/lifetimes, covariance/mesh/interpolator views/posterior semantics, changed topology/preparation, complete active-session reconstruction and all broader original scopes remain open; no row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-ode-restoration-implementation-20261107.rst``. Tracking-v94 unchanged; configured compilation passed; verification deferred.

Next: Finish shared external atmosphere/burn/profile snapshot/value/lifetime contracts and raw/mesh/table/interpolator view lifetime. Complete covariance matrix/subview lifetime and posterior semantics; changed spacecraft/body/location/topology at native preparation boundaries; complete active estimator/smoother state/result and snapshot role remapping; public restoration-buffer contracts and full original function value behavior. Continue remaining native parent/clone owners and whole-spacecraft/plate/hardware edits, source/stream/participant/error/noise/hardware and top-level filter/physical-model updates, changed-list configuration remapping, external borrower/context lifetime, arbitrary provider/file failure cleanup and typed resize/reference providers. Finish all original function I/O/scoping/command/resource and nested Target/Optimize clauses, shared globals and reporting/wrappers; retain unresolved FRR-36/specification dependencies. Continue private renderer, any-Resource Save, body/kernel/NAIF/CSPICE-pool and ephemeris format/B-1/final-iterative/scientific scopes, including the existing EphemManager non-SPICE boundary. Preserve external/historical dependencies and POV/rotation research; defer runtime verification until implementation is complete.


Covariance restoration, 2026-11-07
------------------------------------------

Covariance now has nonvirtual layout/value restoration that keeps current full matrix/backing storage, owners and the most recently returned parameter-block storage, refreshing that block by its actual metadata slot. Ordinary assignment retains its replacement/cache-retirement policy. Spacecraft, ErrorModel, Transmitter and Oscillator native copies preserve complete captured covariance rather than rebuilding diagonals/blocks from configured sigma priors; owned spacecraft covariance references map into copied children. Spacecraft initializes its previously uninitialized copy-attitude pointer, has copy-body cleanup and initializes all nine default variance slots including Cr. Radio copies retain resolved epochs and the transmitter current signal. ErrorModel restoration validates unchanged measurement/solve-for/model/pass roles, retains readiness and fails unready. Eleven source/help paths compile. Covariance adds a cache-slot member embedded in GmatBase; matching configured consumers rebuilt, external binary/platform compatibility unqualified. Complete joint estimator/smoother posterior/checkpoint semantics, changed covariance/graph preparation, shared external dependency values/lifetimes and all broader original scopes remain open; no row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-covariance-restoration-implementation-20261107.rst``. Tracking-v95 unchanged; configured compilation passed; verification deferred.

Next: Finish concrete missing clauses in the original remaining requirement worklist without counting a compiled provider batch as completion of a broad all/any parent. Complete joint estimator/smoother posterior/state/result checkpoints and snapshot role remapping, changed covariance/spacecraft/body/location/topology at native preparation boundaries, public restoration-buffer contracts and full original function value behavior. Finish shared external atmosphere/burn/profile snapshot/value/lifetime contracts and raw/mesh/table/interpolator views beyond the supported unchanged covariance layout. Continue remaining native parent/clone owners and whole-spacecraft/plate/hardware edits, source/stream/participant/error/noise/hardware and top-level filter/physical-model updates, changed-list configuration remapping, external borrower/context lifetime, arbitrary provider/file failure cleanup and typed resize/reference providers. Finish all original function I/O/scoping/command/resource and nested Target/Optimize clauses, shared globals and reporting/wrappers; retain unresolved FRR-36/specification dependencies. Continue private renderer, any-Resource Save, body/kernel/NAIF/CSPICE-pool and ephemeris format/B-1/final-iterative/scientific scopes, including the existing EphemManager non-SPICE boundary. Preserve external/historical dependencies and independently research POV/rotation scope; defer runtime verification until implementation is complete.


POV corner materials, 2026-11-07
------------------------------

Public Blender exporter source exposes mesh2 faces with three corner texture indices that the Qt importer previously rejected. The importer now retains and validates all three indices and blends supported solid materials using corner weights, including pigment, opacity and each supported finish. Authored corner/normal association survives winding conversion. Equal materials use the existing flat material batches; differing materials use opaque/translucent geometry batches and a shared GLSL 1.20 viewer-lighting program. Three source/help paths compile; graphics-driver shader compilation, model loading and visual fidelity remain unverified. This is a concrete exporter grammar repair, not complete POV support. Both original POV rows remain partial/open with no row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-pov-corner-materials-implementation-20261107.rst``. Tracking-v96 unchanged; configured C++ compilation passed; verification deferred.

Next: Continue independent public exporter/model and original historical POV scope research without asking the user for a dialect. Implement remaining concrete grammar and geometry/material omissions, including mesh triangle texture lists, richer declarations/expressions/control flow, unsupported geometry/CSG and image/procedural/layered finishes as required by the owning scope. Do not infer whole Blender/Rhino or NASA model compatibility from one mesh2 grammar repair; retain the complete original POV and parent requirements open. Continue the full original implementation worklist, external/historical/specification dependencies and rotation-drift research. Verification follows completion of implementation.


POV mesh triangle textures, 2026-11-07
------------------------------------

The POV mesh reader now accepts texture_list blocks containing exactly three declared texture identifiers on triangle and smooth_triangle faces inside a mesh. Each identifier resolves in its lexical scope into a captured solid material in authored corner order; the preceding corner-material renderer handles equal/mixed material batches, normals, opacity and winding conversion. Missing, undefined, wrong-type, excess and conflicting material statements fail with source diagnostics; mesh2 counted texture lists retain their separate syntax. C++ compilation passes for three source/help paths. Model loading, graphics-driver shader compilation and appearance remain deferred; both original POV rows remain partial/open with no row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-pov-mesh-triangle-textures-implementation-20261107.rst``. Tracking-v97 unchanged; configured C++ compilation passed; verification deferred.

Next: Continue independent public exporter/model and original historical POV scope research without asking the user for a dialect. Implement remaining concrete grammar and geometry/material omissions, including richer declarations/expressions/control flow, arrays, unsupported geometry/CSG and image/procedural/layered finishes as required by the owning scope. Mixed corner materials and both mesh grammars do not establish whole Blender/Rhino or NASA model compatibility; keep the complete original POV and parent requirements open. Continue the full original implementation worklist, external/historical/specification dependencies and rotation-drift research. Verification follows completion of implementation.


Solver loop restoration, 2026-11-07
-----------------------------------

SolverBranchCommand loop snapshots now include actual spacecraft reached through formation references, deduplicated by identity, and capture weak ordered membership witnesses. Frozen captures keep current frame/context roles. ResetLoopData validates actual target lifetimes/types, unchanged member identities and reachability from script-map roots, prepares independent values and preflights concrete native Copy on retained detached probes before live application. NativeRestorationGuard now activates the existing in-place spacecraft/owned-hardware/state/covariance restoration path; native copy retains actual names and prior dependency contexts. Formation scoped assignment checks compatible declarations/layout, keeps packed-state storage and readiness, and Copy retains live member bindings. Ordinary definition/value copies remain unchanged. Five source/help paths compile. Changed graphs/native preparation, estimator/smoother checkpoints and the complete original command/function/clone clauses remain open; no row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-solver-loop-restoration-implementation-20261107.rst``. Tracking-v98 unchanged; configured C++ compilation passed; verification deferred.

Next: Complete coherent changed-component/membership/topology restoration and native preparation at iteration boundaries, including firing/burn transitions and dependent propagation/measurement registrations; fixed-layout in-place copies are not the complete all-command contract. Finish original function I/O/scoping and full supported command/resource/clone/ownership behavior, state/result/posterior and estimator/smoother requirements, public restoration-buffer contracts, shared external values/lifetimes and source/stream/participant/error/noise/hardware providers. Preserve ordinary cold estimator definition-copy semantics unless a concrete restoration/value contract requires a separate path; inspect actual call sites before changing that policy. Continue private renderer, any-Resource Save/persistence, body/kernel/NAIF/CSPICE-pool, ephemeris B-1/format/final-iterative/scientific, specification/external/historical dependencies and independent POV/rotation research. Defer runtime verification until the full implementation pass is complete.


Interpolator restoration, 2026-11-07
------------------------------------

Five concrete native interpolators now have explicit nonvirtual layout validation and in-place value restoration. Compatible restoration keeps ring/scratch/coefficients and Hermite nested-vector storage while applying captured counters, direction, range/order/flags and numerical values. Shared table-only helpers retain names and allocate no storage. Spacecraft mass-property and FileThrust native restoration use the typed providers instead of replacing interpolator roots or assigning through reallocating table copies; FileThrust failure leaves readiness false. Buffer allocation stages initialized rows before retirement. Hermite copies now retain derivatives/coefficient/time vectors and no longer clear the copied ring; not-a-knot copies retain right-hand-side/solution scratch. Seventeen source/help paths compile. Changed storage/topology/preparation, remaining provider/file lifetimes, complete estimator/smoother/posterior and all broader original clauses remain open; no row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-interpolator-restoration-implementation-20261107.rst``. Tracking-v99 unchanged; configured C++ compilation passed; verification deferred.

Next: Complete coherent changed-component/membership/topology and interpolation-storage preparation at iteration boundaries, including lazy cache creation, Hermite extent changes, firing/burn transitions and dependent propagation/measurement registrations; compatible fixed-storage providers do not complete the all-command contract. Finish original function I/O/scoping and full supported command/resource/clone/ownership behavior, state/result/posterior and estimator/smoother requirements, public restoration-buffer contracts, shared external values/lifetimes and source/stream/participant/error/noise/hardware providers. Preserve ordinary cold estimator definition-copy semantics unless a concrete restoration/value contract requires a separate path. Continue private renderer, any-Resource Save/persistence, body/kernel/NAIF/CSPICE-pool, ephemeris B-1/format/final-iterative/scientific, specification/external/historical dependencies and independent POV/rotation research. Defer runtime verification until the full implementation pass is complete.


Optimizer callback ownership, 2026-11-07
------------------------------------

GmatInterface callback registration now retains weak actual owner identities and rejects null or Function resources. Scoped callback routing takes priority over legacy passive registration, so an executing MATLAB-backed Optimize and each Qt callback transaction select their actual owner. Scope IDs are non-reused, destructors remove only their own records, unregistration clears matching records and expired owners are never dereferenced. Selection is revalidated after interrupt handling and service operations; input replacement during the selected callback execution is rejected and ExecuteCallback returns the actual service result. Seven source/help paths compile. GmatInterface adds private singleton storage; matching configured consumers rebuilt, external binary/platform compatibility unqualified. Legacy script-control/mission entrypoints, full cross-call restriction semantics, nested MATLAB engine/transport behavior and all broader original function/command requirements remain open; no row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-optimizer-callback-ownership-implementation-20261107.rst``. Tracking-v99 unchanged; configured C++ compilation passed; verification deferred.

Next: Complete the original MATLAB-to-GMAT function restriction against the full registered interface and legacy script-control/mission paths without treating optimizer branch execution as a blanket command ban. Preserve native GMAT function calls inside optimizer sequences. Finish callback lifetime/reentrancy and nested engine/transport behavior, whole function I/O/scoping/resource/property/output staging, public edits and component/shared-global dependency retention. Continue coherent changed-component/membership/topology/storage preparation at iteration boundaries, state/result/posterior and estimator/smoother requirements, public restoration-buffer contracts, shared external values/lifetimes and source/stream/participant/error/noise/hardware providers. Finish private renderer, any-Resource Save/persistence, body/kernel/NAIF/CSPICE-pool, ephemeris B-1/format/final-iterative/scientific scopes and all specification/external/historical dependencies. Continue independent POV/rotation research. Defer runtime verification until the full implementation pass is complete.


Nested MATLAB callbacks, 2026-11-07
----------------------------------

Qt protocol-v2 optimizer callbacks now service nested MATLAB evaluation and typed workspace exchange through the already-waiting MATLAB client, avoiding reentry into the busy vendor C Engine worker. Thread-local scoped transport masks legacy requests, preserves numeric bits/column order and UTF-8 data, uses private file exchange and authenticated request-bound completion, and retains the actual parent callback while a child optimizer is selected. Nested optimizer guards preserve parent endpoint/constraint/audit globals; semantic callback errors do not close the parent engine. Seventeen source/help paths compile, and three MATLAB helpers match their build copies. MATLAB helpers and actual nested calls remain unexecuted; legacy/full interface restrictions, MAT-file vendor clients, cancellation and all broader original clauses remain open. MatlabValue export ownership moves to GmatBase; matching rebuilt consumers required. No row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-nested-matlab-callbacks-implementation-20261107.rst``. Tracking-v101 retains the v100 counts; configured C++ builds passed, MATLAB helpers unexecuted, verification deferred.

Next: Finish the full original function I/O/scoping/resource/property/output and MATLAB-to-GMAT restriction requirements without banning native GMAT functions inside optimizer branches. Review MAT-file writer operations: MatWriter, RealMatData and StringMatData still marshal vendor work to the engine worker and need an explicit policy for nested callbacks. Finish transport failure/cancellation cleanup, arbitrary nested/concurrent sessions and other-platform behavior, preserving original callback ownership. Complete coherent changed-component/membership/topology/storage preparation, state/result/posterior and estimator/smoother semantics, public restoration-buffer contracts, shared external values/lifetimes and source/stream/participant/error/noise/hardware providers. Continue private renderer, any-Resource Save/persistence, body/kernel/NAIF/CSPICE-pool and ephemeris B-1/format/final-iterative/scientific scopes. Preserve specification/external/historical dependencies and independent POV/rotation research. Defer runtime verification until the full implementation pass is complete.


MAT-file callback output, 2026-11-07
----------------------------------

Linux Qt MAT-file output now captures owned numeric/string/jagged-cell/record values and uses short-lived file/array handles on the serialized vendor worker. Output inside a protocol-v2 optimizer callback uses the waiting MATLAB client instead, supporting the same logical writer before/during/after that callback without a shared live vendor handle. Complete records stage before writing; native write/close errors and nested save warnings propagate. Buffered data copies now retain dimensions and values; Qt writer copies own independent containers and remain closed. Seventeen source/help paths compile, three helper build copies match, and MatWriter private layout changes require matching rebuilt consumers. MATLAB helpers, file contents/formats, cancellation/concurrency, other platforms, raw vendor-handle leaf calls and all broader original clauses remain unqualified; no row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-mat-file-callbacks-implementation-20261107.rst``. Tracking-v102 retains prior counts; configured C++ compilation passed, verification deferred.

Next: Complete original MATLAB-to-GMAT function restrictions against GmatInterface and the registered legacy script-control/mission paths, distinguishing callback objective evaluation from a general function-call facility. Preserve native GMAT function and command execution inside optimizer and target sequences. Finish complete function I/O/scoping/resource/property/output staging, shared globals and dependency retention. Complete transport interruption/acknowledgment cleanup and file/session lifecycle semantics across supported platforms, including raw vendor-handle leaf clients. Finish coherent changed-component/membership/topology/storage preparation, state/result/posterior and estimator/smoother semantics, public restoration-buffer contracts, shared external values/lifetimes and source/stream/participant/error/noise/hardware providers. Continue private renderer, any-Resource Save/persistence, body/kernel/NAIF/CSPICE-pool, ephemeris B-1/format/final-iterative/scientific and all specification/external/historical dependencies. Continue independent POV/rotation research. Defer runtime verification until the full implementation pass is complete.


MATLAB call restrictions, 2026-11-07
-----------------------------------

GMAT-called MATLAB expressions now hold a process-visible nonvirtual activity scope, including engine startup, temporary-variable creation and workspace cleanup. External script build/edit/run and mutating callback entrypoints reject reentry during that scope; the optimizer driver and native GMAT argument/function/command execution retain their separate paths. Script controls require idle state, serialize against each other/callback scopes, rewind/create their input stream and invalidate stale build success on edits/failures. Legacy socket receipt checks prevent a denied command being queued until after its originating call returns; close remains transport cleanup. Configured Qt C++ builds and standalone socket syntax compilation pass. Two legacy wx GUI transport files are source-reviewed only because their toolchain is unavailable. No existing instance layouts or virtual slots change. Full off-process caller provenance, legacy framing/disconnect/acknowledgment, runtime policy behavior and all broader original clauses remain open; no row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-matlab-call-restrictions-implementation-20261107.rst``. Tracking-v103 retains prior counts. Core/plugin builds and standalone syntax compilation passed; legacy wx edits source-reviewed only. Verification deferred.

Next: Finish the original MATLAB-to-GMAT restriction with explicit caller/provenance semantics across every supported interface; do not equate an external mission-control script with a direct function-call facility or prohibit native GMAT commands inside optimizer/target sequences. Complete legacy partial-frame/disconnect/request-delivery and result/status handling, asynchronous request ownership, full function I/O/scoping/resource/property/output staging, shared globals and dependency retention. Complete transport interruption/acknowledgment cleanup, file/session lifecycle and supported-platform semantics. Finish coherent changed-component/membership/topology/storage preparation, state/result/posterior and estimator/smoother semantics, public restoration-buffer contracts, shared external values/lifetimes and source/stream/participant/error/noise/hardware providers. Continue private renderer, any-Resource Save/persistence, body/kernel/NAIF/CSPICE-pool, ephemeris B-1/format/final-iterative/scientific and all specification/external/historical dependencies. Continue independent POV/rotation research. Defer runtime verification until the full implementation pass is complete.


Owned MATLAB replies, 2026-11-07
-------------------------------

Shared MATLAB replies now expose owned object/parameter strings and use dynamic thread-local character adapters instead of fixed 512/4096/1024-byte buffers. Parameter reads check the actual sandbox type; object lookup falls back on a null internal result. The registered modern wx connection owns encoded replies with explicit byte counts/terminators and decodes Pokes by declared size/encoding, including separate UTF-8/UTF-16/UTF-32 handling. Empty/malformed requests fail without retaining a previous reply. Configured Qt builds and authentic wx-header syntax compilation of GmatConnection and its GmatServer factory pass. GmatInterface instance layout/virtual slots are unchanged; GmatConnection private layout changes require matching rebuilds. Full schemas, MATLAB/runtime/large-data/encoding behavior, older wx/MEX clients, other platforms and all broader clauses remain open; no row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-matlab-owned-replies-implementation-20261107.rst``. Tracking-v104 retains prior counts; configured builds and modern wx connection/factory syntax checks passed. Verification deferred.

Next: Finish nested optimizer base-workspace preservation: the existing PUSH/POP helper captures seven globals, while the legacy driver also uses GMAToptions, X0, Lower, Upper, X, fVal, exitFlag and GMATfminconOutput in the base workspace; final solution callbacks can execute a child optimizer after the parent assigns its exit flag. Preserve the script interface and full parent values/existence/bindings. Continue complete original function I/O/scoping/resource/property/output and all-command solver semantics, external caller provenance, shared globals/dependency retention, supported protocol/client behavior, transport interruption/acknowledgment and file/session lifecycle. Treat unregistered raw socket prototypes separately from the registered wx interface; preserve all actual historical/platform obligations. Finish coherent changed-component/membership/topology/storage preparation, state/result/posterior and estimator/smoother semantics, public restoration-buffer contracts, shared external values/lifetimes and source/stream/participant/error/noise/hardware providers. Continue private renderer, any-Resource Save/persistence, body/kernel/NAIF/CSPICE-pool, ephemeris B-1/format/final-iterative/scientific and specification/external/historical dependencies, plus independent POV/rotation research. Defer runtime verification until the full implementation pass is complete.


Nested optimizer workspace, 2026-11-07
-------------------------------------

Nested Qt optimizer preparation and solves now capture known driver/startup/control variables in MATLAB, retaining independent base/global values, binding kind and absence. The parent options, bounds, solution, objective and exit flag survive a child run during the final solution callback; GMAT retains the child exit flag before restoring the parent and publishes convergence only after restoration. Callback-owned session/LIFO checkpoints unwind unfinished child captures on success/error, retain captures across partial restoration failure and make retired callback cleanup idempotent. The main job helper no longer declares globals for unrelated work. The driver/startup scripts remain byte-identical. Eight participating source/help paths (one new helper); configured C++ build passes and four helper/driver build copies match. MATLAB helpers remain unparsed/unexecuted; complete solver/session/transport/arbitrary user-code behavior and all broader original clauses remain open. No instance-layout/virtual-slot changes; no row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-nested-optimizer-workspace-implementation-20261107.rst``. Tracking-v105 retains prior counts; configured C++ compilation passed, MATLAB files unparsed/unexecuted. Verification deferred.

Next: Finish complete nested solver/session lifecycle and transport interruption/acknowledgment semantics, including output/calling-name scopes, active endpoint/resource lifetime, changed solver configuration and supported-platform behavior. Preserve arbitrary MATLAB handle and global alias semantics within their actual contracts; do not infer full-session rollback from known driver-variable captures. Complete original function I/O/scoping/resource/property/output staging, native all-command optimizer/target behavior, external caller provenance, shared globals/dependency retention and supported client protocols. Finish coherent changed-component/membership/topology/storage preparation, state/result/posterior and estimator/smoother semantics, public restoration-buffer contracts, shared external values/lifetimes and source/stream/participant/error/noise/hardware providers. Continue private renderer, any-Resource Save/persistence, body/kernel/NAIF/CSPICE-pool, ephemeris B-1/format/final-iterative/scientific and all specification/external/historical dependencies, plus independent POV/rotation research. Defer runtime verification until the full implementation pass is complete.


Estimator SPK groups, 2026-11-07
--------------------------------

Accepted batch-estimator SPK sampling now selects active writer targets from the actual spacecraft in the current typed publication. Each target uses its own propagation group, precise epoch, internal frame and frozen Cartesian packet values. Detached sampling retains the actual spacecraft clone type, validates force derivatives and checks that the live spacecraft/frame remained unchanged. The existing one-microsecond limit and local sampling formula are unchanged. EphemerisFile ignores unrelated typed spacecraft packets before file creation or gap/interpolation changes, while preserving terminal cleanup, trial suppression and legacy untyped behavior. Four source/help paths compile; two help XML files parse. Numerical-only sampling, full format/scientific/lifecycle behavior and broader original clauses remain open. No public instance-layout/virtual-slot changes; no row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-estimator-spk-groups-implementation-20261107.rst``. Tracking-v106 retains prior counts; configured compilation passed. Verification deferred.

Next: Complete remaining original ephemeris format/B-1/scientific and final-iterative lifecycle obligations, including supported non-numerical propagator sampling. Preserve native final-replay semantics rather than redefining SOLVEDPASS as mathematical convergence. Continue full function input/output/scoping/resource/property/result staging, all-command target/optimizer/estimator behavior, coherent changed-component/topology/storage preparation and restoration, native solver/posterior/configuration state, shared globals/dependency lifetimes and nested MATLAB session/transport semantics. Continue remaining POV, any-Resource Save/persistence, body/kernel/NAIF/CSPICE-pool, resource-tree and external/specification/historical requirements. Defer runtime verification until the full implementation pass is complete.


Resource serialization ordering, 2026-11-07
------------------------------------------

Common resource serialization now completes a local copy of each class's preferred property order using every missing current native ID, preserving dependency-sensitive ordering and rejecting duplicate/out-of-range IDs before property access. It no longer mutates live ordering metadata or indexes an incomplete list after merely constructing an exception. Spacecraft uses the same completion policy and an owned order vector across representation/getter/owned-object failures; all 56 original preferred identifiers and their order, including the conditional ephemeris field, are retained. Four source/help paths (one new internal header) compile and Save help XML parses. Native read-only/cloaking/value formatting and numerical behavior are unchanged. Specialized serializers, defaults/quoting/reference/external-file reconstruction and the full any-Resource requirement remain open. No public layout/virtual-slot changes; no row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-serialization-order-implementation-20261107.rst``. Tracking-v107 retains prior counts; compilation passed. Verification deferred.

Next: Finish the original any-Resource persistence scope through the remaining class-specific serializers, default/list resets, string literal handling, native input/alias modes, hidden/read-only reconstruction, actual dependency graphs and external-file portability. Continue the full original function/solver/estimator, resource-tree, POV, body/frame/kernel and ephemeris worklist and explicit specification/external/historical dependencies. Runtime verification follows the complete implementation pass.


Native Save list resets, 2026-11-07
---------------------------------

Explicit resource snapshots now emit a native complete-clear assignment before writable SpacePoint kernel lists and spacecraft SolveFors, including when the current list is empty. Shorter later snapshots therefore do not retain earlier list tails through indexed script setters. Spacecraft explicit script exports use the common list writer; ordinary script/MATLAB output retains its existing route. Interpreter forwards an empty SolveFors braced list to the existing native clear setter instead of stripping braces and returning. Four source/help paths compile; Save help XML parses; SpacePoint native setters remain byte-identical. Other lists, complete class-specific persistence/quoting/reference/default reconstruction and external portability remain open. No public layout/virtual-slot changes; no row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-save-list-resets-implementation-20261107.rst``. Tracking-v107 retains prior counts; compilation passed. Verification deferred.

Next: Complete remaining class-specific any-Resource persistence: other list/default resets within actual native contracts, string literal handling, input/alias modes, hidden/read-only reconstruction, actual reference graphs and external-file portability. Continue the full original function/solver/estimator, resource-tree, POV, body/frame/kernel and ephemeris requirements and explicit specification/external/historical dependencies. Runtime verification follows the full implementation pass.


POV static math, 2026-11-07
----------------------------

The Qt POV reader now evaluates 30 scalar-returning and four vector-returning numeric functions, including trigonometric/hyperbolic/inverse, logarithm/power, rounding/division/remainder, degree/radian and native vector operations/rotations. It adds vector/color component extraction and t/u/v/yes/no constants, retaining scalar broadcast and shorter-vector zero extension. Function arity, zero direction/divisor and nonfinite/domain failures include source locations. Mesh2 face/texture disambiguation parses each expression once by its result type, including function/parenthesized expressions and scalar component suffixes. Three source/help paths compile; help XML parses. Lexer/include/scoping, transforms, material/GLSL/Builder and legacy loader sections remain unchanged. Arrays/control/CSG/advanced textures and full original POV scope remain open. No runtime/model/render/shader verification or row credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-pov-static-math-implementation-20261107.rst``. Tracking-v107 retains prior counts; compilation passed. Verification deferred.

Next: Implement the remaining original POV grammar and geometry/material scope using primary references and representative public exporter sources. Classic array documentation specifies up to five dimensions, nested initializers, homogeneous initialized element types, whole-array scope and indexed assignments; these are research findings, not implemented features. Continue remaining numeric/condition/control functions, macros, arrays, richer geometry/CSG and advanced textures without silently omitting unsupported constructs. Preserve the complete original worklist, other function/solver/resource/persistence/body/frame/ephemeris implementation obligations and explicit specification/external/historical dependencies. Runtime verification follows the full implementation pass.


POV arrays, 2026-11-07
-----------------------

The Qt POV reader now implements sparse fixed arrays of one through five dimensions, nested initializers and arrays, indexed reads/writes, element undefinition and recursive value copies. Homogeneous types distinguish numeric widths and material kinds and persist after elements are undefined. Whole-array scope, nonlocal #local edit rejection, rank/bounds/uninitialized checks and default-epsilon integer conversion are implemented. defined, dimensions and dimension_size inspect identifiers/arrays. Typed resolved reads reuse the existing geometry/material/transform/string/include-filename paths, with bounded raw lookahead retaining closing-token commit ordering. Three source/help paths compile; visualization help XML parses. Raw lexing, existing math/material/transform/GLSL/Builder and legacy loader routes remain unchanged. Directive-generated array headers, dynamic/mixed/dictionaries, control flow, advanced geometry/textures and full original POV scope remain open. No runtime/model/rendering verification or row credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-pov-arrays-implementation-20261107.rst``. Tracking-v107 retains prior counts; compilation passed. Verification deferred.

Next: Continue remaining original POV control/macros/conditions, directive-generated array headers/initializers, dynamic/mixed/dictionary variants, richer geometry/CSG and advanced textures using primary references and representative exporter sources. Static array containers are not full POV-Ray scene compatibility. Continue the complete original function/solver/resource/persistence/body/frame/kernel/ephemeris implementation worklist and explicit specification/external/historical dependencies. Runtime verification follows the full implementation pass.


POV logical expressions, 2026-11-07
-----------------------------------

The Qt POV reader now parses parenthesized scalar/vector and string comparisons, component-wise logical operators and eager ternary expressions, preserving native precedence and selected tuple widths. Boolean/equality rules use the shared default epsilon; inclusive comparisons retain native near-equality behavior. select supports three/four eager scalar arguments; strcmp/strlen use terminated UTF-16 string values. Optional boolean modifiers accept expression starts. Multi-character comparison tokens are recognized, and pow(0,0) now reports the native domain error. Three source/help paths compile; visualization help XML parses. Array/scope/include/geometry/material/GLSL/Builder routes remain unchanged apart from using the same named epsilon constant for array conversion. Control/macros, remaining functions/color/string/array variants, richer geometry and advanced textures remain open. No runtime/expression/model/rendering verification or row credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-pov-logical-expressions-implementation-20261107.rst``. Tracking-v107 retains prior counts; compilation passed. Verification deferred.

Next: Implement remaining original POV directive control and macro semantics using the native scene-parser evaluation rules, then remaining functions/color/string/array variants, directive-generated headers/initializers, richer geometry/CSG and advanced textures. Continue representative exporter research and the full original function/solver/resource/persistence/body/frame/kernel/ephemeris implementation worklist and explicit specification/external/historical dependencies. Keep all original clauses and defer runtime verification until the implementation pass is complete.


POV conditional directives, 2026-11-07
-------------------------------------

The Qt POV reader now supports if/ifdef/ifndef/elseif/else/end chains, scalar default-epsilon predicates and identifier/array-element presence checks. Directive keywords and closing predicates stay unadvanced until selection. Inactive regions scan raw tokens without opening includes, changing declarations, evaluating expressions or emitting messages; nested native block forms balance under the 200-level control limit. Branch keyword replay retains bounded filename-reader state. Non-reused control identities detect a predicate replacing or mutating its enclosing chain, and malformed/current-chain/EOF cases fail with source locations. Message directives log warning/info diagnostics and active error aborts import; indexed strings use existing typed reads. Three source/help paths compile; visualization help XML parses. Raw decoding, arrays, expressions, materials/geometry/transforms/GLSL/Builder and legacy loader remain unchanged. Active loops/macros/switch, broader array/geometry/texture and full POV scope remain open. No runtime/directive/model/rendering verification or row credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-pov-conditionals-implementation-20261107.rst``. Tracking-v107 retains prior counts; compilation passed. Verification deferred.

Next: Implement remaining original POV loop/macro/switch execution and break semantics, then directive-generated array headers/initializers, remaining function/string/color/array/dictionary variants and richer geometry/CSG/advanced textures. Continue representative exporter research and the complete original function/solver/resource/persistence/body/frame/kernel/ephemeris worklist and explicit specification/external/historical dependencies. Preserve all original requirements and defer runtime verification until implementation is complete.


POV loops, 2026-11-07
----------------------

The Qt POV reader now executes for/while source loops and nearest-loop break through unified control roles. Bookmarks retain source frames/cursors/pending tokens while symbol scopes stay live. For introduces/replaces a current-scope scalar counter, shadows outer names, samples bound/step once and advances the current counter after each pass; while re-reads its header. Native default-epsilon initial/repeat boundaries, zero-iteration/body-tail raw skipping and same-file running-loop rules are retained. Break discards intervening conditional control frames and skips their endings before closing the loop. Scalar/finite counter and non-advancing-step errors are explicit. Three source/help paths compile; visualization help XML parses. Array/raw decoding/expression/geometry/material/transform/GLSL/Builder and legacy loader remain unchanged. Macros/switch, caller interruption, broader array/function/geometry/texture and full POV scope remain open. No runtime/loop/model/rendering verification or row credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-pov-loops-implementation-20261107.rst``. Tracking-v107 retains prior counts; compilation passed. Verification deferred.

Next: Implement remaining original POV macros and switch/case/range control with their proper break targets, and complete caller interruption/cancellation without silently narrowing loop semantics. Continue directive-generated array headers/initializers, remaining function/string/color/array/dictionary variants, richer geometry/CSG/advanced textures and representative exporter research. Continue the full original function/solver/resource/persistence/body/frame/kernel/ephemeris worklist and explicit specification/external/historical dependencies. Preserve all original requirements; runtime verification follows the complete implementation pass.


POV switch clauses, 2026-11-07
------------------------------

The Qt POV reader now supports switch/case/range with a captured scalar selector, strict default-epsilon case matching and exact inclusive ranges. Selected chains fall through while later label expressions still parse; default else activates unmatched chains and does not deactivate selected ones. First case/range labels are required. Explicit branch/label/end skip modes preserve inactive includes/declarations/messages, and break now exits the nearest actual switch or loop while balancing intervening conditionals. Matching end and predicate/label ownership checks retain the actual control role. Three source/help paths compile; visualization help XML parses. Array/raw/expression/geometry/material/transform/GLSL/Builder and legacy loader remain unchanged; loop handlers only use the equivalent named End skip mode. Macros/cancellation and remaining language/geometry/texture/full POV scope remain open. No runtime/clause/model/rendering verification or row credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-pov-switch-implementation-20261107.rst``. Tracking-v107 retains prior counts; compilation passed. Verification deferred.

Next: Implement remaining original POV macros with native argument, scope, value/lifetime and break-return behavior, and complete caller interruption/cancellation. Continue directive-generated array headers/initializers, remaining function/string/color/array/dictionary variants, richer geometry/CSG/advanced textures and representative exporter research. Continue the complete original function/solver/resource/persistence/body/frame/kernel/ephemeris worklist and explicit specification/external/historical dependencies. Preserve all original clauses; runtime verification follows the full implementation pass.


POV source macros, 2026-11-07
----------------------------

The Qt POV reader now captures macro source without executing its body, creates global replaceable definitions, and invokes source fragments with caller-parented local scopes. Arguments are evaluated before formal bindings exist. Stable value cells retain references to caller identifiers and array elements; expressions and values emitted from shorter-lived nested macro locals are copied. Array-element aliases retain homogeneous-type checks, named undef removes a binding, and indexed undef clears its actual slot. Optional omitted/uninitialized arguments add no local entry, preserving outer lookup. Natural completion and nearest-macro break restore caller source positions; call-header boundaries protect final delimiters. Typed array tokens retain reference provenance, and include filenames can use completed macro/string/array expressions while retaining their actual source context. Three source/help paths compile; visualization help XML parses. Geometry/math/material/transform/GLSL/Builder and legacy loader remain unchanged. Cancellation, remaining language/array/dictionary/function/geometry/texture and full exporter/version compatibility remain open. No runtime/model/rendering verification or row credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-pov-macros-implementation-20261107.rst``. Tracking-v115 retains prior counts; compilation passed. Verification deferred.

Next: Complete POV caller interruption/cancellation, remaining directive-generated array headers/initializers, native function/string/color/array/dictionary variants, richer geometry/CSG and advanced textures, and research representative exporters. Macro-produced filename fragments must finish in the including source context; more general interleaved macro/directive filename lifecycles remain unqualified. Continue the complete original function/solver/resource/persistence/body/frame/kernel/ephemeris worklist and explicit specification/external/historical dependencies. Preserve original clauses and defer runtime verification until the implementation pass is complete.


POV background loading and cancellation, 2026-11-07
-------------------------------------------------

The Qt POV reader now accepts a caller cancellation predicate and observes current-thread interruption. File reads use checked 64 KiB chunks; token scanners, raw inactive/macro scans, source-loop replay, value/array copies, primitive/mesh expansion and detached geometry building poll cooperatively. Cancellation returns no partial model. The spacecraft preview and orbit scene now use owned background POV jobs with queued GUI completion, independent cancellation state, detached CPU geometry and captured include paths. Changing a selection, deleting its owner or rejecting preview validation cancels the corresponding job; per-request identity and cancellation checks reject stale results. Validation keeps pending OK intent until completion, and field changes revoke it. Workers never access widgets or the live scene graph; application shutdown requests cancellation and joins outstanding threads. Plot export refuses a pending-model frame before opening its output file. Nine source/help paths (one new internal job header) compile; visualization help XML parses. Language/geometry/material/transform/shader formulas and legacy loader remain unchanged. OS/library operations have cooperative cancellation latency; broader language/array/dictionary/function/geometry/texture/exporter and full campaign scope remain open. No runtime/thread/GUI/model/rendering verification or row credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-pov-cancellation-implementation-20261107.rst``. Tracking-v116 retains prior counts; compilation passed. Verification deferred.

Next: Continue remaining POV directive-generated array headers/initializers, native function/string/color/array/dictionary variants, richer geometry/CSG and advanced textures, interleaved macro/directive filename lifecycles and representative exporter research. Preserve nonterminating-loop semantics with cooperative cancellation rather than iteration cutoffs. Legacy direct readability calls remain synchronous; their default reader observes thread interruption, while the new reader overload accepts caller cancellation. Continue the complete original function/solver/resource/persistence/body/frame/kernel/ephemeris worklist and explicit specification/external/historical dependencies. Preserve original clauses and defer runtime verification until the implementation pass is complete.


POV dynamic and mixed array forms, 2026-11-07
-------------------------------------------

The Qt POV reader now supports dimensionless dynamic one-dimensional arrays, explicitly mixed element types and optional sparse initializer positions. Dynamic access grows the nominal size to include its index even for defined/ifdef/undef; fixed bounds and homogeneous type checks retain their prior defaults. Mixed arrays permit per-element type changes, including through macro argument aliases. Copies retain independent dimensions, dynamic/mixed modes, initialized values and holes. Optional initializer blocks accept empty or uninitialized plain-identifier positions while fixed arrays retain complete positional structure; optional flags do not implicitly propagate to nested leaf rows. Dynamic trailing commas add no phantom final element. Completed rows/values accept native optional commas, and final active controls are prepared before the initializer closing brace. Declaration terminal tokens remain held through publication. Three source/help paths compile; visualization help XML parses. Lexer/macros/cancellation, expressions, control formulas, geometry/material/transform/GLSL/Builder, worker/viewer paths and legacy loader remain unchanged. Native 3.8 source disables ordinary directives/macros during array declaration recognition, so arbitrary generated header tokens are not established as a missing native requirement. Complete SDL/exporter/profile and broader original campaign scope remain open. No array/model/runtime verification or row credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-pov-array-forms-implementation-20261107.rst``. Tracking-v117 retains prior counts; compilation passed. Verification deferred.

Next: Return to the complete original function/solver and any-Resource Save/persistence worklist, together with body/frame/kernel/ephemeris, resource-tree and explicit specification/external/historical requirements. For POV, inspect actual representative exported model sources and documented exporter grammar against the historical smooth-mesh/material use case; implement concrete omissions without inferring an entire ray-tracer obligation or silently qualifying the subset. Native dictionary/pseudo-dictionary and remaining function/string/color/geometry/texture compatibility and macro filename lifecycle remain open within that unresolved profile. Preserve all original rows and defer runtime verification until the implementation pass is complete.


Save byte literals and empty Strings, 2026-11-07
----------------------------------------------

Explicit Save snapshots now use a new ASCII byte-literal form for native string/filename properties, quoted string-list entries and runtime String-resource values when ordinary quoting is unsafe. Printable simple values retain ordinary quotes; apostrophes, control/non-ASCII bytes and structural punctuation are encoded as paired hexadecimal bytes in an explicit @-prefixed quoted literal. Ordinary quoted interiors, Windows backslashes, repeated apostrophes and global StringUtil behavior are unchanged. Script/Math/Validator admission, property/String input and function literal materialization recognize the new form and decode at typed input boundaries. Encoded data does not become a resource-name alias. Empty String snapshots now include an explicit empty assignment. Ordinary script/MATLAB serialization retains its previous writer route, and String export mode guards restrict the new output to explicit script-export modes. Ten source/help paths (one new internal header) compile; Save help XML parses. Save pre-render-before-file-I/O and function ownership/result/solver code remain unchanged. New encoded snapshots require the updated reader; specialized serializers, equations, external portability and full any-Resource/function scope remain open. No string/script/runtime round-trip verification or row credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-save-string-literals-implementation-20261107.rst``. Tracking-v118 retains prior counts; compilation passed. Verification deferred.

Next: Finish remaining class-specific any-Resource Save serialization: specialized string/equation and alias/input modes, native list/default resets, hidden/read-only reconstruction, reference graphs and external-file portability. Review the remaining original function I/O/scoping/resource/property/output staging and command/solver/estimator semantics against actual owning code, including retained dependency contexts and supported caller/session lifetimes. Continue body/frame/kernel/ephemeris, resource-tree, representative POV/exporter scope and all explicit specification/external/historical work. Preserve the full original requirement set and defer runtime verification until the implementation pass is complete.


Dynamic table snapshots, 2026-11-07
----------------------------------

DynamicDataDisplay explicit Save snapshots now emit AddParameters = {} before rebuilding their rows, preventing earlier rows/cells from surviving a shorter or empty appended snapshot. The native scalar/index-zero empty setter reuses SetParamSettings and its existing wrapper-release policy, then resets layout selectors/counts/name caches. Interpreter preserves this exact empty-list operation before generic reference lookup. Four appended write-only row properties retain backgrounds, explicit Auto/Manual text-color mode and warning/critical bounds by cell position, including blank cells and fixed manual black. Explicit snapshots use those complete row forms instead of name-based background/bound fragments. Ordinary serialization adds the forms only when older syntax cannot represent the configured appearance. Existing parameter IDs, class instance fields/virtual slots, append behavior without reset, UpdateData alert formulas, copy/initialization and invocation/reference binding methods remain unchanged. Existing warning/critical setters now check for exactly two bounds before indexing. Five source/help paths compile and two help XML files parse. New row forms require an updated reader. Runtime table reload/rendering, class-wide subscriber lifecycle and the complete any-Resource/function/campaign scope remain open. No row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-table-snapshots-implementation-20261107.rst``. Tracking-v119 retains prior counts; compilation passed. Verification deferred.

Next: Continue remaining specialized any-Resource serializers and native defaults/lists, equation/alias/input policies, hidden/read-only reconstruction, reference graphs and external-file portability. Keep dynamic table reconstruction and reader compatibility in the deferred verification list. Review the original function I/O/scoping/resource/property/output staging and command/solver/estimator semantics against owning source, including retained dependencies and supported caller/session lifetimes. Continue body/frame/kernel/ephemeris, resource-tree, representative POV/exporter scope and every explicit specification/external/historical requirement. Preserve original assertions and defer runtime verification until the implementation pass is complete.


Native resource-list snapshots, 2026-11-07
------------------------------------------

Explicit Save snapshots now reset ReportFile Add, XYPlot YVariables, OrbitPlot Add, the four writable DataFilter selection lists, SolarPowerSystem ShadowBodies, CelestialBody DSNMediaFileDirectories and ContactLocator EventTypes before recording their entries. Empty and shorter later snapshots therefore have an explicit native replacement path. Interpreter forwards exact empty object lists before reference lookup and empty DataFilter string lists before generic brace stripping. Existing native Clear actions release subscriber wrappers/plot members; XYPlot retains X, and OrbitPlot informs its integration sampler through its existing action. DataFilter uses its native empty setters and excludes read-only Strands. Existing ShadowBodies explicit-empty/default handling, DSN clearing and ContactLocator EventTypes clearing are reused. Ordinary serialization, nonempty assignment behavior, native setters, ownership policies, class layouts and Save pre-render/file I/O remain unchanged. Three source/help paths compile; Save help XML parses. Runtime reconstruction and complete any-Resource/function/resource lifecycle remain open. No row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-subscriber-list-snapshots-implementation-20261107.rst``. Tracking-v120 retains prior counts; compilation passed. Verification deferred.

Next: Continue specialized any-Resource serializers, native defaults and remaining writable lists, equation/alias/input policies, hidden/read-only reconstruction, complete reference graphs and external-file portability. Keep subscriber/filter/default reconstruction and reader compatibility for deferred verification. Review original function I/O/scoping/resource/property/output staging and command/solver/estimator semantics against owning source, including retained dependencies and supported caller/session lifetimes. Continue body/frame/kernel/ephemeris, resource-tree, representative POV/exporter scope and every explicit specification/external/historical requirement. Preserve original assertions and defer runtime verification until the full implementation pass is complete.


Retained event-locator and region values, 2026-11-07
----------------------------------------------------

ContactLocator, EclipseLocator and IntrusionLocator now provide retained function-value bindings for actual target/region/observer/occulting-body/sensor/intruding-body references and caller-name aliases. Contact additional-target workers expose their native owned values so their copied references bind with the enclosing graph. EventLocator copies preserve targetIsRegion. Completed native Clone/Copy routes reconnect retained contexts, hold preceding owners through replacement and reject incompatible types. Copying retires target-owned report/kernel/runtime caches while preserving native copied event results; locators remain dormant until ordinary preparation or a requested native search. Actual-reference getters let native initialization preserve retained identities and avoid indexing a shorter pointer list. PlanetographicRegion has a complete derived Copy, copies calculated vertices as well as its latitude/longitude/file definition and binds body/J2000 references through the existing body-fixed helper. Native initialization reuses completed polygons. FunctionManager admits these native providers, handles stable locator back references and skips eager event-locator initialization during value construction. Existing event searches, root/refinement formulas, polygon geometry and ordinary native preparation algorithms remain unchanged. Twelve source/help paths compile jointly; help XML parses. Full function/all-command/scoping/resource/session lifetimes and scientific/runtime behavior remain open. No row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-event-locator-values-implementation-20261107.rst``. Tracking-v121 retains prior counts; compilation passed. Verification deferred.

Next: Continue all original function I/O, resource/property/scoping/output staging and all-command target/optimizer/estimator obligations against actual native providers, including coherent changed components/topology/storage preparation and restoration, caller/global/dependency lifetimes and nested MATLAB session behavior. Keep locator/region input, output, recursive restoration, caller aliases, retained event results and native event preparation for deferred verification; active ephemeris histories and sessions are not a transferred value contract. Continue specialized any-Resource Save serializers, defaults/lists, hidden/equation/input policies, complete reference graphs and external portability, then body/frame/kernel/ephemeris, resource-tree, representative POV/exporter scope and every specification/external/historical dependency. Preserve every original clause and defer runtime verification until the complete implementation pass is finished.


Region property inputs and snapshots, 2026-11-07
----------------------------------------------------

PlanetographicRegion now replaces complete Latitude/Longitude vectors through native typed setters and a class-specific script-array dispatch, including empty resets. Coordinate validation precedes replacement; indexed edits replace or append within native index/size bounds. Native latitude and longitude-span limits remain, with finite-value checks. Four label overloads use base-relative metadata indices instead of global IDs. AreaFileName validates paths/conflicts before mutation, supports accepted file-mode replacement and explicit clearing, and retires derived coordinates only after selection succeeds. Clearing file mode preserves materialized coordinates as inline data. Definition edits invalidate polygon readiness; native initialization also handles changed body shape and clears failed/partial geometry, while queries prepare dirty definitions. Explicit script-export snapshots clear filename and both coordinate lists before current inputs. Existing parameter IDs/layout, retained Clone/Copy/context code, ordinary/MATLAB writer paths and polygon algorithms remain unchanged. Five source/help paths compile; two help XML files parse. Full function/resource/input/file/cache/persistence runtime scope remains open. No row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-region-inputs-implementation-20261107.rst``. Tracking-v121 retains prior counts; compilation passed. Verification deferred.

Next: Continue original function I/O/resource/property/scoping/output staging and caller/global/dependency lifetimes, all-command target/optimizer/estimator behavior, coherent changed-component/topology/storage preparation/restoration and nested MATLAB sessions. Private child FOS calls already preserve the live caller graph; do not substitute speculative active-session copies for that route. Keep region array edits, rejected inputs, file-mode transitions, dirty geometry/body-shape preparation and appended snapshots for deferred verification. Continue specialized any-Resource serializers, defaults/lists, hidden/equation/input policies, complete graphs and external portability, body/frame/kernel/ephemeris, resource-tree, representative POV/exporter scope and all explicit specification/external/historical dependencies. Preserve original assertions and defer runtime verification until the full implementation pass is finished.


Formation loop topology restoration, 2026-11-07
----------------------------------------------------

Solver-loop rollback now prepares native Formation membership and packed-state layouts before restoring values. Snapshots retain original member owners and exact lifetime identities; live original members are restored through the captured graph after the current selection changes, without equal-name substitution. A native prepared plan stages allocation and swaps declarations/member bindings/state storage only after all native probes succeed. Compatible fixed layouts keep the prior path. Restoration probes copy packed data without sampling the possibly replaced/retired current selection; ordinary clones retain live-member sampling. GmatState has distinct layout identities for allocation, element-role and precision-mode changes; value/epoch updates and fixed-layout RestoreValues preserve them. Propagation captures root/member identities and rebuilds bindings before using changed layouts, including same-size state replacement. Native state copy/resize now stage owned allocation before retiring old storage. Existing origin/owned-component/stale-target gates, numeric value/epoch setters, precision conversion and fixed-layout validation/restoration remain unchanged. Twelve source/help paths compile jointly and three help XML files parse. GmatState layout and FormationInterface contract changes require matching rebuilt clients/plugins. Full command/function/topology/shared-global/ephemeris lifecycle and runtime/scientific scope remain open. No row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-formation-loop-topology-implementation-20261107.rst``. Tracking-v121 retains prior counts; compilation passed. Verification deferred.

Next: Continue coherent spacecraft/owned-component, force/model, measurement and interpolation-storage preparation at iteration boundaries, preserving original target identities and source/native origin contracts. Keep formation additions/removals/permutations, private/same-name members, stale targets, cold probes, same-size state replacement, metadata/precision layouts, nested functions and native propagation rebuild for deferred verification. Complete all original function I/O/scoping/property/output staging, retained dependency/shared-global lifetimes, all-command target/optimizer/estimator and nested MATLAB session behavior. Continue specialized any-Resource serializers, defaults/lists, hidden/equation/input policies and portability, body/frame/kernel/ephemeris, resource-tree, representative POV/exporter and every explicit specification/external/historical dependency. Preserve original assertions; defer runtime verification until the full implementation pass is finished.


Private interpolation cache preparation, 2026-11-07
----------------------------------------------------

Native restoration now stages owner-specific private storage through a common preparation plan after validating the selected GmatBase-owned graph. Spacecraft prepares private CM/MOI Lagrange caches and FileThrust prepares private linear/not-a-knot caches when captured presence or layout differs, including lazy creation after capture. Compatible caches keep their objects/storage. Native type and owner checks restrict replacement to these exclusive unexposed caches; typed public/borrowed interpolator restoration remains unchanged. Plans allocate/clone before applying pointer selection and retain displaced caches until copying completes. Existing owner declarations/environment/physical-model checks are factored and preserved, and preparation failures leave owners unready. Both shared owned-graph restoration and direct native owner Copy routes use preparation, covering estimation direct copies. Ordinary copying, interpolation equations, native state/time/math routines and owned-node identity/covariance gates remain unchanged. Eleven source/help paths compile and three help XML files parse. The new GmatBase virtual contract requires matching rebuilt clients/plugins; Formation plans derive from the common plan. Full owned-component/topology/function/command/session/scientific scope remains open. No row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-private-interpolation-preparation-implementation-20261107.rst``. Tracking-v124 retains prior counts; compilation passed. Verification deferred.

Next: Continue coherent owned spacecraft/hardware/force/measurement component and propagation/interpolation-storage preparation, including firing transitions, changed declared inputs and public/borrowed interpolation and Hermite extents. Keep lazy CM/MOI/spline creation/removal, incompatible private layouts, direct estimation/native copies, rejected owner/environment changes, allocation failure, cache lifetime and numerical equivalence for deferred verification. Complete original function I/O/scoping/property/output staging, retained dependency/shared-global lifetimes, all-command target/optimizer/estimator behavior and nested MATLAB sessions. Continue any-Resource serialization/default/input/reference/portability, body/frame/kernel/ephemeris, resource-tree, representative POV/exporter and every explicit specification/external/historical dependency. Preserve original assertions and defer runtime verification until the full implementation pass is finished.


Solver finite-burn checkpoints, 2026-11-07
-----------------------------------------

Solver checkpoints now capture the actual borrowed transient-force selection and lifetime identities. Reset stages that selection before native probes/copies, restores spacecraft maneuvering and thruster firing flags, and swaps the original force pointers back after value restoration. Branch transient context forwarding now sets the branch itself as well as its children. Propagation preparation compares list context, identity, order and lifetime, including equal-count changes, before reusing model/state bindings; successful preparation captures the selection and buffer retirement clears it. Publisher and subscriber cached maneuvering flags are staged with the checkpoint and restored silently, without emitting another maneuver event or changing output histories. Surviving subscriber identities retain captured flags; newly subscribed objects use the restored publisher flag and retired objects are not accessed. Ordinary spacecraft copying still preserves the destination maneuvering flag. Existing role/type/origin/state/covariance guards and all numerical routines remain. Compilation only; no row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-solver-burn-transitions-implementation-20261107.rst``. Tracking-v125 retains prior counts.

Next: Continue changed owned spacecraft/hardware/force/measurement graphs, declared inputs and public/borrowed interpolation/Hermite extents. Finish force internal-value/dependency and shared-owner lifecycles, nested function/global/solver/MATLAB contexts, and complete subscriber/output-history semantics. Then complete all remaining original worklist scope before runtime verification. Preserve the original assertions, tolerances, denominator and acceptance dispositions. Deferred checks include begin-only/end-only/balanced burns, initially active burns, nested loops/functions, same-count force switches, list/force retirement, subscriber churn, allocation failure and numerical/output equivalence.


Explicit Save numeric literals, 2026-11-07
-----------------------------------------

Explicit Save numeric serialization now uses at least Real::max_digits10 significant digits in native numeric streams while preserving a higher configured precision. Scoped formatting selects the classic decimal locale and default floating notation without modifying global locale or display settings. Variable values and Array cells bypass their hard-coded 16-digit display conversion only during explicit exports; the Array path also writes current values under the alternative initial-expression build policy. Common vector properties and OpenGlPlot viewpoint/reference/direction vectors use explicit numeric vector literals. GmatBase native scalar/matrix/element/string-array writers and existing spacecraft/attitude/Fmincon stream preparation inherit the scoped format. Ordinary serialization and inactive MATLAB formatting retain their prior branches. No equation, setter, coordinate transformation, tolerance or resource API/layout changes. Compilation only; full any-Resource Save scope remains open and no row or verification credit is added.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-save-numeric-literals-implementation-20261107.rst``. Tracking-v126 retains prior counts.

Next: Continue specialized any-Resource serialization, hidden/default/list/equation/input/reference and external-file portability policies, including resource writers that manufacture numeric strings outside the common path. Preserve native time and coordinate/attitude representation computations; do not infer scientific round-trip qualification from formatting. Continue changed owned component/storage graphs and full original function/scoping/command/session/provider work across the existing worklist before runtime verification. Deferred verification includes finite binary64 edge values, signed zero, low/high display precision, numeric locales, Array build policies, nested/failure scope restoration, ordinary/MATLAB output equivalence and representative resource reloads.


Navigation propagator snapshots, 2026-11-07
------------------------------------------

Explicit Save now resets the complete Estimator/Simulator Propagator selection before writing its captured default and spacecraft-specific mappings, including empty selections. The reader recognizes only a raw empty braced list before generic Parameter/name lookup and forwards it to each native owner. Native reset clears names, mappings and selection scratch, retires owned PropSetup definitions through NavigationPropagationCopies::Publish, reconnects the measurement manager to the same collection containers and marks the solver for native preparation. Estimator also clears its force-derivative cache and defers initialization. Serialization never clears the live source. Ordinary nonempty assignments, native propagator mapping output, equations, parameter IDs, class layouts and numerical domains are unchanged. Compilation only; full any-Resource persistence/navigation/function/command scope remains open and no row or verification credit is added.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-navigation-propagator-snapshots-implementation-20261107.rst``. Tracking-v127 retains prior counts.

Next: Continue complete navigation definitions and measurement/filter/solve-for/residual-plot list snapshots, resource default/input/reference/equation and external-file portability semantics. Finish coherent changed owned component/storage graphs, function I/O/scoping/shared-global/dependency lifetimes, all-command solver/estimation/MATLAB behavior and every original worklist dependency before runtime verification. Deferred checks include long-to-short/changed/empty propagation mappings in appended snapshots, default/override restoration, quoted or encoded brace strings, read failure, repeated native cleanup and subsequent estimator/simulator preparation/runs.


Navigation resource list snapshots, 2026-11-07
---------------------------------------------

Explicit Save now resets Estimator Measurements/AddSolveFor/AddResidualsPlot/DataFilters and Simulator AddData before writing their selected entries. Native empty-list setters clear the actual declaration and dependent owned manager state. The AddSolveFor clear no longer erases measurement names: it resets solve-for strings/roles instead. Estimation-state invalidation stages empty state/offset, STM and covariance, detaches prepared object/map/ID roles and retires owned restoration/physical-model caches while keeping declared consider/participant names and the measurement-manager binding. Measurement clearing detaches these borrowers before retiring tracking definitions and runtime products; manager service/container configuration is retained. Filter and residual-plot clears retire their owned values. All affected owners require native preparation afterward. Reader dispatch checks raw empty lists before generic lookup and rejects quoted/encoded empty-brace strings before mutation. Ordinary native nonempty indexed setter code is unchanged. Compilation only; no row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-navigation-list-snapshots-implementation-20261107.rst``. Tracking-v128 retains prior counts.

Next: Continue navigation declaration input transactions and complete default/list/reference/provider/session behavior, specialized any-Resource serialization and external-file portability. Finish coherent changed owned component/storage graphs, original function I/O/scoping/shared-global/dependency lifetimes, all-command solver/estimation/MATLAB behavior and all remaining original worklist dependencies before runtime verification. Deferred checks include appended shorter/empty navigation lists, measurement preservation while clearing solve-fors, repeated cleanup/rebind, consider/participant roles and precise epochs, quoted/encoded brace rejection, allocation/provider failure, retired buffers/views/plots/filters and subsequent estimator/simulator preparation/runs.


Solve-for declaration transactions, 2026-11-07
---------------------------------------------

Estimator AddSolveFor now prepares its replacement selection before asking EstimationStateManager to validate/store the role, and publishes the public list only after that call succeeds. The state manager stages all five parallel solve-for name/object/ID arrays and swaps them only after all string/vector allocations complete. Qualified append and indexed setters restore prior parser scratch on failure. The already-bound object overload uses the same staged metadata storage. Native last-dot decomposition, object/name/ID values, index bounds/diagnostics and the three overloads' existing duplicate return/logging policies remain. The helper rejects inconsistent existing metadata arity instead of indexing mismatched arrays. No native state preparation, equations, domains, ready-state policies, parameter IDs, class instance fields or virtual contracts change. Compilation only; no row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-solvefor-declaration-transactions-implementation-20261107.rst``. Tracking-v129 retains prior counts.

Next: Continue coherent live declaration/component/storage preparation and explicit-versus-automatically discovered solve-for/duplicate role reconciliation; the staged declaration update does not complete those contracts. Finish other navigation input transactions, full native provider/default/list/reference/session behavior, any-Resource serialization/portability, original function/scoping/shared-global/dependency lifetimes, all-command solver/estimation/MATLAB behavior and every remaining worklist dependency before runtime verification. Deferred checks include bad syntax/index, append/replace/object-bound roles, each duplicate policy, metadata allocation failures, parser scratch restoration, subsequent binding/native preparation and scientific estimation behavior.


Tracking-file resource snapshots, 2026-11-07
--------------------------------------------

Explicit Save now resets TrackingFileSet AddTrackingConfig/FileName/RampTable/DataFilters/SimTDRSServiceAccessList before their captured entries. Tracking configurations are emitted individually in native syntax instead of being passed through the quoted string-array snapshot path. A common native clear partitions owned adapters/relevant filters before publication, marks preparation pending, clears generated measurements/ionosphere/frequency caches and then retires displaced owned values. Configuration clearing also clears participant references and parser bracket/start scratch; filter clearing removes owned filters from both role and ownership containers. Other declarations and borrowed service/propagator settings remain. The reader distinguishes raw {} from literal data: quoted/encoded brace filenames and service strings use indexed data setters, while brace literals cannot select configurations or filter objects. Ordinary serializer dispatch and positive setter branches remain. Compilation only; native defaults/domain/preparation and complete any-Resource/navigation/function scope remain open. No row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-tracking-file-snapshots-implementation-20261107.rst``. Tracking-v130 retains prior counts.

Next: Continue remaining resource list/default/reference/equation and specialized serialization/portability gaps, plus transactional tracking-configuration input parsing. Finish coherent changed owned component/storage graphs, navigation provider/session readiness, original function I/O/scoping/shared-global/dependency lifetimes, all-command solver/estimation/MATLAB behavior and every remaining worklist dependency before runtime verification. Preserve RunEstimator's existing fresh command-clone initialization rather than adding speculative automatic-role resets. Deferred checks include appended changed/shorter/empty tracking collections, multiple configurations, exact participant/sensor/type and filename literals, owned filter/adapter cleanup, native SA1/file defaults, allocation failure, caller lifetimes and subsequent tracking estimation/simulation calculations.


Tracking configuration declaration transactions, 2026-11-07
----------------------------------------------------------

Tracking configurations now parse into temporary native definitions. A complete script assignment validates all participant/type tokens and closed strands before publishing the declaration; indexed native calls publish only their accepted incremental token. Generated configuration list cardinalities are checked before indexing and all definitions are staged before publication. Parser flags have deterministic defaults and are copied with definitions. Publication invalidates generated adapters/caches while retaining other selected declarations. Source inspection found that the v128-v130 collection admission in SetPropertyObjectValue did not cover STRINGARRAY properties routed through SetPropertyStringValue. A shared admission helper now runs for navigation/tracking OBJECTARRAY and STRINGARRAY properties before generic splitting/normalization. Whole-property raw empty lists reach native clears; nested empty lists and quoted/encoded brace navigation/config/filter object names reject before clearing. Brace filename/ramp/service literals remain indexed data. Existing parser symbols and StringArray SetField virtual slot are retained. Compilation only; no row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-tracking-config-transactions-implementation-20261107.rst``. Tracking-v131 retains prior counts.

Next: Continue remaining resource list/default/reference/equation and specialized serialization/portability gaps. Finish coherent changed owned component/storage graphs, navigation provider/session readiness, original function I/O/scoping/shared-global/dependency lifetimes, all-command solver/estimation/MATLAB behavior and every remaining original worklist dependency before runtime verification. Preserve native grammar, supported measurement types, reference identity and fresh command-clone initialization. Deferred tracking checks include whole rejected multi-token declarations, accepted incremental prefixes, config count/cardinality errors, raw versus quoted/encoded braces in both reader routes, cache/filter/adapter lifetimes, native SA1/file defaults and subsequent scientific preparation/calculations.


Nested ephemeris dependency ownership, 2026-11-07
-------------------------------------------------

Function value graphs now admit private EphemerisFile dependencies through their existing native spacecraft/frame/dynamics/wrapper binder. Context-owned ephemerides use a weak back-reference to the enclosing graph instead of an owning alias that would create a cycle. The GmatBase context registry resolves weak references and preserves that ownership role for same-graph assignment; fresh native copies acquire ordinary leases. Detached EphemerisFile Clone/Copy reconnect completed values, while active trial assignments keep their existing writer policy. Interior value capture includes a still-live original public root as a private candidate for reference binding; expired roots and incompatible native owned identities reject. Source ephemeris leases are staged across all dependencies and released after graph initialization. No native instance fields, virtual slots or parameter IDs are added. Compilation only; complete function/resource scope and runtime behavior remain open. No row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-nested-ephemeris-ownership-implementation-20261107.rst``. Tracking-v132 retains 1093/1158 (94.39%), 65 unresolved engineering rows. Recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full implementation audit and goal remain incomplete.

Next: Continue coherent changed owned resource/component/storage preparation and restoration, including native owner/embedded-root conflicts, external borrower/public interpolation contracts and Hermite extents. Finish original function I/O/property/output staging, shared globals/dependency lifetimes and all-command target/optimizer/estimator/MATLAB lifecycle. Continue every remaining any-Resource serialization/default/reference/portability, body/frame/kernel/ephemeris, resource-tree, POV and specification/external/historical worklist item before runtime verification. Deferred checks for this change include nested private ephemerides in supported wrapper/resource graphs, same-graph assignment, detached Clone/Copy after enclosing-value release, public-root back-references, cyclic native ownership rejection, allocation/initialization failure cleanup, multiple imported owner leases, active trial writer preservation and subsequent native ephemeris preparation/calculations.


Ephemeris state and interpolation storage preparation, 2026-11-07
-----------------------------------------------------------------

Ephemeris propagator preparation now uses the propagation state manager's complete packed extent directly, rather than multiplying that already-complete vector by the propagated-object count. Without a state manager, the native six-Cartesian-values-per-object fallback checks index capacity before multiplication. Required state/J2000 buffer allocations are staged together before retiring either old array; a compatible existing J2000 buffer keeps its address. Empty/unprepared groups report controlled preparation errors and the propagator stays unready on allocation failure. Native ephemeris query interpolation constructs its replacement Hermite/Lagrange cache before retiring the old cache and publishes type/order together. Compilation only; complete function/solver/ephemeris scopes remain open. No row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-ephemeris-storage-preparation-implementation-20261107.rst``. Tracking-v133 retains 1093/1158 (94.39%), 65 unresolved engineering rows. Recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full implementation audit and goal remain incomplete.

Next: Continue coherent changed owned resource/component/storage preparation and restoration, original function I/O/property/output staging, shared globals/dependency lifetimes and all-command target/optimizer/estimator/MATLAB lifecycle. Finish native non-numerical ephemeris provider/buffering/revert/sampling behavior and all original ephemeris format/B-1/final-iterative/scientific requirements. Continue every remaining any-Resource serialization/default/reference/portability, body/frame/kernel, resource-tree, POV and specification/external/historical worklist item before runtime verification. Deferred checks include multiple spacecraft with a complete packed PSM state, no-manager capacity and empty/unprepared groups, compatible/incompatible J2000 storage reuse, failed first/second buffer allocation, failed Hermite/Lagrange construction, changed segment order/mode, high-precision interpolation, native provider initialization failures and subsequent propagation/solver behavior.


Ephemeris clock and publication transitions, 2026-11-07
-------------------------------------------------------

Ephemeris state publication now resolves the requested epoch, evaluates the native provider and only then converts that fresh state to the propagated-object origin. Both Real and precision companion clocks are synchronized during publication, SetTime and buffered reversion; reversion takes the captured epoch directly rather than reconstructing it through elapsed-time subtraction/addition. STK refresh converts its state at the actual Real or precision query epoch instead of lastEpoch. Explicit-duration Step restores the selected default ephemStep on success, false return and exceptions through a local scope guard. Code500/CCSDS conversion temporaries now use fixed native six-element stack arrays, removing conversion-exception leaks. Native single-spacecraft-per-SPK, range/type/domain and scientific algorithm policies remain. Compilation only; complete function/solver/ephemeris scopes remain open. No row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-ephemeris-clock-transitions-implementation-20261107.rst``. Tracking-v134 retains 1093/1158 (94.39%), 65 unresolved engineering rows. Recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full implementation audit and goal remain incomplete.

Next: Continue original function I/O/property/output staging, coherent changed owned resource/component/storage preparation, shared globals/dependency lifetimes and all-command target/optimizer/estimator/MATLAB lifecycle. Finish remaining native ephemeris provider/buffering/revert/sampling and original format/B-1/final-iterative/scientific clauses without expanding documented provider domains speculatively. Continue every remaining any-Resource serialization/default/reference/portability, body/frame/kernel, resource-tree, POV and specification/external/historical worklist item before runtime verification. Deferred checks include Real/precision state refresh at explicit/sentinel epochs, exact buffered epoch restoration, SetTime then native step, moving/rotating origins, STK epoch-dependent frame conversion, explicit-duration failure followed by ordinary stepping, Code500/CCSDS conversion exceptions, stopping-condition final steps and subsequent solver/function propagation.


SPK missing-state and failure publication, 2026-11-07
-----------------------------------------------------

The native SPICE target-state reader now returns its existing six-component missing-data marker immediately on its handled failure path, before frame rotation can read failed CSPICE output. SPK propagation validates kernel states and both origin-translation inputs before arithmetic/publication and rejects nonfinite/missing values. The three inner generic exception-swallowing blocks no longer substitute default zero vectors for failed lookups. Rejected out-of-span Step paths return false before copying/publishing and report zero accepted duration; a missing reader cannot report a successful step. Void refresh reports missing data before downstream publication. Optional RunPastStartEnd preparation skips a lookup/publication at uncovered epochs while retaining the existing measurement validity/gap-editing workflow and spacecraft state. Native single-spacecraft, range/type and successful scientific calculations remain. Compilation only; complete function/solver/ephemeris scopes and runtime behavior remain open. No row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-spk-failure-publication-implementation-20261107.rst``. Tracking-v135 retains 1093/1158 (94.39%), 65 unresolved engineering rows. Recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full implementation audit and goal remain incomplete.

Next: Continue original function I/O/property/output staging, coherent changed owned resource/component/storage preparation, shared globals/dependency lifetimes and all-command target/optimizer/estimator/MATLAB lifecycle. Finish remaining native ephemeris provider/buffering/revert/sampling, kernel-pool/coverage and original format/B-1/final-iterative/scientific clauses. Continue every remaining any-Resource serialization/default/reference/portability, body/frame/kernel, resource-tree, POV and specification/external/historical worklist item before runtime verification. Deferred checks include genuine CSPICE gaps versus outside spans, strict and RunPastStartEnd initialization, unchanged spacecraft on omitted epochs, Real/precision Step/refresh failures, missing readers, invalid origin/J2000 inputs, caller gap skipping and EGAP editing, source warnings, kernel refresh and subsequent valid state calculations.


SPICE pool revision and cache coherence, 2026-11-07
---------------------------------------------------

Native SPICE pool mutation attempts now advance a shared revision before furnish, unload or clear operations, including interface initialization and the TLE propagator's direct leap-second-kernel furnish. The target-state reader associates cached coverage envelopes with that revision, clears stale envelopes before classification and rechecks the revision after coverage preparation; reader assignment starts with a cold coverage cache. Both Real and precision CelestialBody raw state caches are ineligible for reuse after a pool change, including same-epoch calls; completed state calculations record the current revision. This conservatively covers dependent body sources while preserving configured IDs and ordinary successful source calculations. New nonvirtual static core accessors and two native cache revision fields require matching rebuilt native consumers. Compilation only; complete body/frame/kernel/scientific scope remains open. No row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-spice-pool-revisions-implementation-20261107.rst``. Tracking-v136 retains 1093/1158 (94.39%), 65 unresolved engineering rows. Recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full implementation audit and goal remain incomplete.

Next: Continue original body/frame/kernel/NAIF/CSPICE-pool and ephemeris format/B-1/final-iterative/scientific requirements, including actual KEEPER/meta-kernel/alias ownership and coupled cached provider preparation. Finish original function I/O/property/output staging, coherent changed owned resource/component/storage graphs, shared globals/dependency lifetimes and all-command target/optimizer/estimator/MATLAB lifecycle. Continue every remaining any-Resource serialization/default/reference/portability, resource-tree, POV and specification/external/historical worklist item before runtime verification. Deferred checks include same-epoch body queries after load/unload/reset, dependent non-SPICE bodies, cached gap/outside envelopes before and after pool changes, failed/partial loads and unload reconstruction, TLE leap-second furnishing, cold assignment, metadata/alias/kernel replacement, native consumer compatibility and subsequent frame/orbit/scientific calculations.


Native KEEPER coverage and loaded-state discovery, 2026-11-07
-------------------------------------------------------------

Native loaded-state and failed-query coverage discovery now consult CSPICE KEEPER instead of treating the wrapper requested-filename map as the complete loaded table. Coverage envelopes enumerate actual loaded SPKs, including meta-kernel children. Loaded path resolution checks KEEPER for recorded/requested/resolved spellings and recognizes current filesystem aliases, returning the native loaded spelling. LoadKernel uses that check to avoid re-furnishing an existing child/alias and invalidates observed stale wrapper membership. Dynamic filename buffers retry native truncation before exposing paths. Explicit body/propagator selections and existing unload/recovery contracts remain. No class fields or virtual slots are added; matching rebuilt native consumers are required for the protected core helpers. Compilation only; complete body/frame/kernel/scientific scope remains open. No row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-keeper-coverage-discovery-implementation-20261107.rst``. Tracking-v137 retains 1093/1158 (94.39%), 65 unresolved engineering rows. Recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full implementation audit and goal remain incomplete.

Next: Review the original Resource Tree Clone requirement (All Others!E156/E157) against every listed resource class and the actual Qt/native clone paths, and repair concrete omissions before claiming whole-row implementation. Continue original body/frame/kernel/NAIF/CSPICE-pool and ephemeris format/B-1/final-iterative/scientific requirements, including actual root/child/alias ownership and priority-preserving unload recovery. Finish original function I/O/property/output staging, coherent changed owned resource/component/storage graphs, shared globals/dependency lifetimes and all-command target/optimizer/estimator/MATLAB lifecycle. Continue every remaining any-Resource serialization/default/reference/portability, resource-tree, POV and specification/external/historical worklist item before runtime verification. Deferred checks include meta-kernel child coverage without pool mutation, direct and alias loaded paths, stale wrapper membership after native reset, long filenames, native query failures, missing/replaced files, explicit configured-list behavior, unload/recovery interactions and subsequent body/frame/ephemeris calculations.


Resource clone context defaults and original scope review, 2026-11-07
---------------------------------------------------------------------

Resource cloning now preserves effective nonempty scalar output filenames and function paths when the source has no authored assignment for that setting. These name/search-context defaults are serialized with the existing exact script string-literal formatter, while palette values and implicit numerical force-model references retain their existing paths. The source copier uses native parameter-ID correspondence to recognize authored aliases as well as exact property names, so an original expression or deprecated filename spelling is not overwritten by a materialized default. Both clone preview and final candidate use the same source object metadata. A source-only review retains all 33 original worksheet resource categories and their conditional clauses; it identifies included-file-only declarations and protected-default policy as direct remaining clone scope. Existing camera metadata already includes the full drawing/star/sensor/model/playback payload. Compilation only; no whole-row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-resource-clone-defaults-implementation-20261107.rst``. Tracking-v138 retains 1093/1158 (94.39%), 65 unresolved engineering rows. Recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full implementation audit and goal remain incomplete.

Next: Complete the original Resource Tree Clone scope using the retained 33-category inventory. Implement a safe configured-resource copy for included-file-only declarations, preserving main-document source/history, references and expressions, and resolve protected default-object copying through explicit supported configuration paths while retaining original assertions and native standard-action policy. Finish other concrete class-specific copy omissions and conditional provider requirements before claiming E157 or parent E156 implemented. Continue all remaining original function I/O/scoping/property/output staging, changed owned component/storage and shared-global/dependency lifetimes, all-command solver/estimator/MATLAB lifecycle, any-Resource Save, body/frame/kernel/ephemeris, POV and specification/external/historical worklist requirements before runtime verification. Deferred checks for this change include default report/ephemeris/solver filenames, implicit GMAT/MATLAB function paths, explicit aliases/expressions, encoded Unicode/control bytes, edited versus unchanged drafts, external files untouched, exact source/history isolation, save/reload and all 33 resource categories.


Resource cloning from captured included configuration, 2026-11-07
-----------------------------------------------------------------

Qt resource copying now extracts declarations and owner assignments from the logical mission source read during the most recent successful Qt Build, including nested includes and main-document overrides. The native interpreter offers an optional thread-local scoped capture; ordinary interpretation without an observer retains its existing behavior. The Qt wrapper stages and publishes a text-keyed snapshot only after successful interpretation, function validation and path restoration. Clone extraction uses that snapshot while insertion edits only the current document. Main-source and mission-only-include boundaries are mapped explicitly; combined configuration/mission includes and included marked array initializers reject rather than move expressions across dependencies. Original clone policies, references, defaults, camera metadata and commit/rollback paths remain. Compilation only; whole-row implementation and acceptance remain open.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-resource-include-clone-implementation-20261107.rst``. Tracking-v139 retains 1093/1158 (94.39%), 65 unresolved engineering rows. Recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full implementation audit and goal remain incomplete.

Next: Complete the original Resource Tree clone scope: resolve protected default-object copying through explicit configuration paths while retaining standard-action policy and original assertions, support mixed configuration/mission includes and included marked array initializers without changing expression order or included files, and finish concrete class-specific copy/provider omissions across the retained 33-category inventory. Continue original function I/O/scoping/property/output staging, changed owned storage/components, shared-global/dependency lifetimes and all-command solver/estimator/MATLAB lifecycle; then any-Resource Save, body/frame/kernel/ephemeris, POV and remaining specification/external/historical requirements before runtime verification. Deferred checks include nested and empty includes, main overrides, implicit/explicit and empty included mission boundaries, duplicate anchors, stale/mismatched captures, failed Build/function validation/rollback, include file edits, aliases/literals/expressions/references, arrays/cameras/defaults, source isolation and save/reload.


Editable copies of built-in coordinate systems, 2026-11-07
----------------------------------------------------------

Qt now offers Copy coordinate system in the Edit menu and on a built-in frame's context menu. This explicit path prepares an unregistered, editable user-frame draft from the original native frame and its independently cloned axes, retaining the origin, axes type and writable axes settings. Built-in protection and modification flags are changed only on the detached value. A scoped native configuration export supplies the otherwise absent Create declaration and effective defaults; authored resources continue to use source copying. Preview and commit share preparation and the existing name/settings/history/rebuild transaction. The ordinary Clone restrictions and separate propagator policy remain intact. Compilation only; default-body/calculated-point and remaining broad resource-copy cases stay open, with no row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-default-frame-copy-implementation-20261107.rst``. Tracking-v140 retains 1093/1158 (94.39%), 65 unresolved engineering rows. Recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full implementation audit and goal remain incomplete.

Next: Finish the original Resource Tree copy scope across the retained 33-category inventory. Resolve default celestial-body/calculated-point copying without silently changing ephemeris or built-in computation semantics; support mixed configuration/mission includes and included marked array initializers without altering dependency/evaluation order or external files; finish concrete native class/provider configuration omissions. Continue original function I/O/scoping/property/output staging, changed owned storage/components, shared-global/dependency lifetimes and all-command solver/estimator/MATLAB lifecycle, any-Resource Save, body/frame/kernel/ephemeris, POV and specification/external/historical requirements before runtime verification. Deferred frame checks include all four protected defaults, ordinary Clone rejection, explicit Edit/context action, draft edits/cancel, invalid names/references, origin/axes/settings independence, native default/owned-property round trips, includes and empty missions, precision/literals, history/rollback and save/reload with subsequent frame-dependent missions.


Failed-unload recovery in observed kernel-root order, 2026-11-07
----------------------------------------------------------------

Native failed-unload recovery now snapshots KEEPER's actual ordered file/source entries before mutation and replays the remaining directly furnished roots in that order. Recovery bypasses LoadKernel's normal deduplication so repeated native furnish operations retain their order, and includes observed roots absent from the wrapper's requested-name map. Only the last matching direct-root operation is omitted. Ambiguous child-only history does not trigger a guessed global reconstruction. Successful and failed unloads reconcile requested-file bookkeeping with actual native membership. An unload error returns false; batch unload attempts all entries and aggregates failures. Filename/source outputs grow before exposing truncated paths. Compilation only; complete meta-kernel/ownership/recovery/scientific scope remains open, with no row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-kernel-recovery-order-implementation-20261107.rst``. Tracking-v141 retains 1093/1158 (94.39%), 65 unresolved engineering rows. Recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full implementation audit and goal remain incomplete.

Next: Finish effective kernel-pool restoration and root/child/alias ownership, including earlier child unload history, changed/unavailable meta-kernel dependencies, programmatic pool values, path lifetimes and caller error propagation. Complete original body/frame/kernel/NAIF and ephemeris formats/B-1/final-iterative/scientific requirements. Continue the full Resource Tree copy inventory, including semantics-preserving default-body/calculated-point copies, mixed configuration/mission includes, included marked array initializers and native class/provider omissions. Finish original function I/O/scoping/property/output staging, changed owned storage/components, shared-global/dependency lifetimes, all-command solver/estimator/MATLAB lifecycle, any-Resource Save, POV and remaining specification/external/historical requirements before runtime verification. Deferred checks include out-of-filename-order competing kernels, external and repeated roots, direct/meta/child unload failures, long source paths, unreadable/changed files, stale requests, native errors and returned statuses, revision-based cache invalidation and subsequent ephemeris/frame calculations.


Native meta-kernel child recovery and alias unload, 2026-11-07
--------------------------------------------------------------

UnloadKernel now resolves actual native loaded names, admitting recognized filesystem aliases and meta-kernel children even without requested-name map entries. Failed-unload reconstruction restores each surviving root's observed ordered child selection before furnishing later roots, omitting the requested last child occurrence as well as preserving earlier child removals. It prunes replay-only children in reverse order, preserves duplicates and retained later occurrences, compares complete file/type/source sequences and checks the pre-root table prefix. A failed or incompatible replay triggers attempted retirement of that new root, with explicit retirement errors. Root order, membership reconciliation, revision invalidation and failure statuses remain. Compilation only; complete ownership, file-content/pool-value lifetimes and original scientific scope stay open, with no row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-kernel-child-recovery-implementation-20261107.rst``. Tracking-v142 retains 1093/1158 (94.39%), 65 unresolved engineering rows. Recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full implementation audit and goal remain incomplete.

Next: Finish kernel ownership and lifetime behavior across configured bodies, propagators, generated FK/IK/SPK/CK/SCLK files and shared readers. Propagate actual unload/replay failures through callers so cleanup and replacement do not claim success or discard still-needed files. Complete programmatic pool-value/provider lifetimes, native low-level/external mutation integration and original body/frame/NAIF/ephemeris formats/B-1/final-iterative/scientific requirements. Continue the full Resource Tree copy inventory, including default body/calculated-point semantics, mixed configuration/mission includes, included marked array initializers and provider/class-specific omissions. Finish original function I/O/scoping/property/output staging, changed owned storage/components, shared globals/dependencies, all-command solver/estimator/MATLAB lifecycle, any-Resource Save, POV and specification/external/historical requirements before runtime verification. Deferred checks include aliases and untracked children, repeated roots/children, child unload history across later duplicate roots, changed/reordered meta files, missing/type-changed children, native failures during pruning/retirement, exact table-prefix retention, returned statuses/cache revisions and subsequent ephemeris/frame calculations.


Generated kernel retirement after confirmed unload, 2026-11-07
--------------------------------------------------------------

Generated station, event, ephemeris and CK/SCLK file retirement now requires successful native unloading and confirms that no furnish remains before deleting or replacing the file. A shared inline helper treats already-unloaded and absent files idempotently, propagates runtime unload/deletion failures, and provides guarded nonthrowing retirement diagnostics for destruction. Failed runtime cleanup retains pending filenames/readers for retry; station and locator setup becomes unready before affected cleanup. ContactLocator releases its owned reader, IntrusionLocator retains runtime failure ownership until retry, and CK clock-file deletion follows writer finalization with a second native check. A detached CK writer copy no longer claims the source clock filename. Compilation only; active writer assignment, shared kernel leases and complete original lifecycle/scientific scope remain open, with no row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-generated-kernel-retirement-implementation-20261107.rst``. Tracking-v143 retains 1093/1158 (94.39%), 65 unresolved engineering rows. Recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full implementation audit and goal remain incomplete.

Next: Finish coherent active/inactive CK/SCLK writer assignment and low-level handle/clock ownership, including source-copy isolation, destination retirement, finalization and changed file/segment storage. Finish kernel leases/lifetimes across configured bodies, SPK propagators, generated files and shared readers, remaining caller failure propagation, programmatic pool/provider state and external mutation/path/content lifetimes. Complete original body/frame/NAIF and ephemeris formats/B-1/final-iterative/scientific clauses. Continue the full Resource Tree copy inventory, default-body/calculated-point semantics, mixed configuration/mission includes, included array initializers, function I/O/scoping/property/output staging, changed owned components/storage, shared globals/dependencies, all-command solver/estimator/MATLAB lifecycle, any-Resource Save, POV and remaining specification/external/historical requirements before runtime verification. Deferred checks include failed/unavailable/native-double-furnish unloads, unreadable and retained temp files, filesystem removal errors, partial pair cleanup/retry, locator rebind/init/destruction, source clock-copy isolation, buffered CK destructor finalization, SCLK regeneration and subsequent native frame/ephemeris/event calculations.


Native CK handle transitions and clock value ownership, 2026-11-07
------------------------------------------------------------------

Native CK switching now refreshes its owned filename pointer and obtains a checked writable DAF handle when appending to an existing CK. Handle/open state is published only after successful native opening. Close errors retain handle ownership and propagate; explicit AbortKernel and guarded destruction release failed sessions without retrying buffered output after failure. CK value copies deep-copy segment/clock values while starting without a borrowed native or C stream handle; assignment stages storage before destination finalization and swaps completed values. Clock copies now retain their UTC identifier/time-system data. Segment buffers, native scratch arrays and metadata streams/text units use staged or scoped ownership. The subscriber destructor catches finalization errors and retains uncertain clock files. Compilation only; subscriber assignment, shared leases and full original scientific scope remain open, with no row or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-ck-native-owner-implementation-20261107.rst``. Tracking-v144 retains 1093/1158 (94.39%), 65 unresolved engineering rows. Recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full implementation audit and goal remain incomplete.

Next: Complete subscriber-level CK writer assignment: retire the destination session coherently, stage configuration and reset detached session state without borrowing source files, handles or buffers. Finish generic EphemerisWriter/provider session/storage preparation and clock coefficient/partition/file-write transactionality. Complete shared kernel leases, constructor/output-path ownership, remaining failure propagation, native pool/provider and external path/content lifetimes. Finish original all-format/B-1/final-iterative ephemeris, function I/O/scoping/property/output staging, changed owned components/storage and all-command solver/estimator/MATLAB lifecycle. Continue Resource Tree copy/default body/calculated-point/include/array scope, any-Resource Save, body/frame/NAIF, POV and specification/external/historical requirements before runtime verification. Deferred checks include long switched filenames, new/existing/non-CK/conflicting files, detached copies and owned clocks, allocation failures, buffered close/empty CK/abort, metadata errors and unit cleanup, input arity/capacity, source output isolation, native SCLK/CK numerics and accepted iterative output.


CK subscriber assignment and clock isolation, 2026-11-07
--------------------------------------------------------

CK subscriber copies now retain configuration in a detached, uninitialized session. Assignment stages all incoming values before strictly retiring the destination writer and its generated clock, then publishes prepared values without transferring an open stream. New clocks use atomically reserved private directories. CoordinateConverter copies now retain the complete matrices, derivatives and matching epoch cache, and CK finalization no longer leaks unnecessary carry-over samples. These source repairs strengthen partial dependencies and claim no complete original requirement or verification credit.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-ck-subscriber-assignment-implementation-20261107.rst``. Tracking-v145 retains 1093/1158 (94.39%), 65 unresolved engineering rows. Recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full implementation audit and goal remain incomplete.

Next: Return to the original-row implementation worklist, prioritizing remaining public resource clone/body/point and function/provider graph behavior. Retain CK sample-buffer allocation failures, shared output and metadata-file ownership as explicit follow-ups where required by those original rows. Do not run verification or change original requirements, fixtures, assertions, tolerances or dispositions.


Native Solar System barycenter copying, 2026-11-07
--------------------------------------------------

The built-in SolarSystemBarycenter now has a standard Qt Clone route and an explicit Copy calculated point action. Its detached user-defined copy retains a serialized native ephemeris-source descriptor, with state, velocity, acceleration and mass delegated to the same SolarSystem special point. Ordinary barycenters retain mass-weighted calculation. Native and Qt libration-point restrictions now recognize this source descriptor under renamed aliases. Default celestial-body copying and the full any-object Clone requirement remain incomplete; no original-row or verification credit is added.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-native-barycenter-copy-implementation-20261107.rst``. Tracking-v146 retains 1093/1158 (94.39%), 65 unresolved engineering rows. Recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full implementation audit and goal remain incomplete.

Next: Complete the remaining original Resource Tree copy inventory, especially default celestial bodies with DE/SPICE target identity, SolarSystem binding and Earth/Neptune-specific orientation/property semantics preserved independently of the new resource name. Continue mixed configuration/mission includes, included array initializers and the original function/provider graph and any-Resource Save work. Preserve original requirements, fixtures, assertions, tolerances, gates and dispositions; runtime verification remains deferred.


Physical-body identity for copied native models, 2026-11-07
-----------------------------------------------------------

Native celestial-body configuration now separates a resource name from a retained PhysicalBody model identity. DE target lookup, SolarSystem source/file updates, native orientation policy, body-specific behavior and eight coordinate-axis families resolve that identity. The new descriptor is serialized before other settings and can be bound only once before command mode. This implements a prerequisite for accurate default-body copies; the public Qt default-body copy action and complete snapshot reconstruction remain unfinished. No whole original row or verification credit is added.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-physical-body-identity-implementation-20261107.rst``. Tracking-v147 retains 1093/1158 (94.39%), 65 unresolved engineering rows. Recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full implementation audit and goal remain incomplete.

Next: Finish the Qt default celestial-body copy action and candidate-script reconstruction using MakeDefaultBodyCopy before renaming, a complete native export and the serialized PhysicalBody descriptor. Inspect effective inherited values, readonly native-model settings, private/owned configuration and provider context rebinding across save/load and copy-of-copy source routes without executing runtime verification yet. Then complete the remaining any-object clone inventory, mixed configuration/mission includes, included array initializers, original function/provider graph, any-Resource Save and other campaign requirements.


Qt default celestial-body copying, 2026-11-07
---------------------------------------------

Qt now provides Copy celestial body for default planets, moons and the Sun, using a detached value whose PhysicalBody identity is captured before renaming. Body copy scripts combine a full-precision native configuration snapshot with authored scalar settings, retain effective ordered lists once and select the physical model before dependent fields. The editor uses the body-specific writable-field/validation route. Sun construction and saved stellar-value storage, Earth DSN identity checks and independent native rotation-source restoration are repaired. Broad any-object clone/scientific scope remains incomplete; no whole original row or verification credit is added.

Report: ``doc/DevelopersDocs/Qt6ParityValidation/coverage95-default-body-copy-implementation-20261107.rst``. Tracking-v148 retains 1093/1158 (94.39%), 65 unresolved engineering rows. Recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full implementation audit and goal remain incomplete.

Next: Continue the original any-object Clone inventory and source reconstruction gaps, especially mixed configuration/mission includes, included array initializers and any remaining inherited/private/owned configuration, provider-context and external-file behavior. Complete the original function/provider graph, any-Resource Save and other campaign implementation requirements before runtime verification. Keep original requirements, fixtures, assertions, tolerances, denominator, gates and dispositions intact.
