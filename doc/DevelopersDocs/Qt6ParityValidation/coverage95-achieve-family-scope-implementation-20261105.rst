Achieve numeric scalar family scope, 2026-11-05
================================================

FRC!E93 requires the Achieve control-variable types listed by E94-E97: FRR-38
scalar parameters, user Variables, user Array elements and real numbers. The
prior source notes established only the generic wrapper path. This batch adds a
complete concrete FRR-38 type/family inventory and repairs an unsafe wrapper
binding route. It establishes provisional numeric type-interface implementation
presence for the parent; it is not scientific or GUI/runtime qualification.

Original inventory and concrete classes
---------------------------------------

The retained original catalogue contains 88 FRR-38 rows, E501-E588. The inventory
preserves every key, declared ID, text and fingerprint, including duplicate
legacy IDs. It maps 8 headings, 74 real scalar leaf instances, 2 matrix rows and
4 Gregorian string rows. Component requirements expand to actual factory type
names rather than being mistaken for whole-vector values. The map resolves 106
distinct factory names, of which 97 are real-valued names backed by 92 concrete
real classes. Six burn spellings share ImpBurnElements. Every mapped type has a
concrete factory creator, inheritance chain and Evaluate implementation route.

The family routes are:

* Origin/orbit: Altitude, BetaAngle, C3Energy, EA/ECC/MA/MM/SMA/TA, Energy,
  HMAG, Latitude/Longitude/LST/MHA, OrbitPeriod, RadApo/RadPer, RMAG,
  SemilatusRectum and VelApoapsis/VelPeriapsis.
* Frame/orbit: AOP/AZI, BdotT/BdotR/BVectorAngle/BVectorMag, DEC/DECV/FPA/HA,
  HX/HY/HZ, INC/RA/RAAN/RAV/VMAG, Cartesian X/Y/Z/VX/VY/VZ and RLA/DLA.
* Attitude: AngularVelocityX/Y/Z, nine DCM components, EulerAngle1-3,
  EulerAngleRate1-3, Q1-4 and MRP1-3. Repeated worksheet Euler rows remain
  separate original instances and map to the same actual classes.
* Spacecraft mass/physical: Cd/Cr, DragArea, DryMass, SRPArea and TotalMass.
* Time: ElapsedDays/ElapsedSecs and TAI/TDB/TT/UTC ModJulian values.
* Burn: Element1/2/3 and V/N/B through ImpBurnElements/BurnData.
* Tank: Pressure, Volume, FuelDensity, FuelMass, Temperature and RefTemperature
  through HardwareReal/SpacecraftData and the actual attached tank's property.

OrbitSTM and its four 3x3 partitions return matrix types. TAI/TDB/TT/UTC Gregorian
values return strings. Their original requirements remain in the inventory and
denominator; they are not reclassified as numeric scalar operands of Achieve.
The numeric interpretation follows Achieve's real-valued goal/tolerance contract
and the actual return types; no string-goal comparison or matrix reduction is
invented. Rmat66Var/StringVar explicitly retain those distinct return types.

Binding and evaluation route
-----------------------------

Validator::CreateSystemParameter/CreateAutoParameter delegates to the native
factory and preserves configured owner/dependency names. CreateParameterWrapper
constructs the native wrapper. ParameterWrapper::GetDataType returns the actual
Parameter return type; RealVar gives the 97 scalar names REAL_TYPE. The concrete
OrbitReal/BplaneReal/AttitudeReal/BallisticMassReal/TimeReal/BurnReal/HardwareReal
EvaluateReal paths dispatch to the corresponding concrete Evaluate and data
owner. Achieve checks real-valued operands, resolves references and reads the
wrapper; it has no family-name whitelist that excludes any mapped numeric type.

Variables and indexed Array elements have their native scalar wrappers. Whole
Arrays remain rejected. Finite numeric goal literals use the already implemented
AllowConstantGoal compatibility option, while the native reference-goal default
remains available. Desired values and tolerances retain their real-valued paths
and the prior finite-value/positive-tolerance guards. Qt's parameter dialog and
ParameterReferences provide typed references and generic owner/body/frame/actual
attached-hardware dependency selection; Apply uses native validation.

The inventory follows concrete data owners rather than assuming that a wrapper
alone supplies physics. OrbitData resolves origin/frame and current body gravity
and shape; BplaneData uses the configured frame's celestial-body origin/Mu and
CalculateBPlaneData; PlanetData uses configured origin hour angle/shape and the
CoordinateConverter. AttitudeData uses the actual spacecraft attitude, Euler
sequence and rate support. SpacecraftData reads the actual spacecraft/tank
property. BurnData retains configured frame, fired-state and element behavior.
TimeData retains native epoch/time-conversion and elapsed-time behavior.

Concrete binding repair
------------------------

ParameterWrapper::SetRefObject previously accepted either a matching name OR a
Parameter type, then performed a C-style cast. A same-name non-Parameter could
therefore be stored as a Parameter. Null inputs were dereferenced and an empty
reference-name array could also be indexed before the type check.

The setter now uses dynamic_cast<Parameter*> and returns false for null/wrong
runtime types before replacing the binding. It does not dereference name-array
entries. Actual Parameter name aliases remain accepted, preserving native
function-argument binding behavior. Failed binding leaves the previous binding
intact. No public signature, instance layout, virtual table or numerical formula
changed. This repairs the shared typed route used by Achieve and other numeric
consumers without adding a new parameter whitelist.

Limits, compilation and tracking
---------------------------------

This is type-interface source presence under native parameter preconditions.
It does not close FRR!E501/E502/E525/E526's universal body/frame/read-write scope,
prove all scientific conversions, create missing attitude rates/hardware fields,
or permit every orbital representation in an undefined mathematical domain.
Body-fixed/central-body, elliptic/hyperbolic, singular-state and Euler sequence
conditions remain. Native legacy undefined/sentinel and error behavior remains;
in particular some B-plane Evaluate methods convert domain errors to a finite
reserved value. That numeric-consumer/error-policy scope still warrants owning-
source review and later verification; it was not silently rewritten here.

Build v1 passes 66 steps / 2.82 seconds; session 15318 completed. Final edited
source matches the at-build hash. This build follows the 81-step / 199.45-second
registration build, so the enabled core, Qt, console, plugin and language-binding
clients are current. No tests, GUI, missions/functions/models/solvers, numerical
comparisons, evidence readers, seals or acceptance campaigns were executed.
The source inventory is static implementation inspection, not an acceptance
verifier. No additional build was needed for the report/tracking documents.

Stage: build/nasa-coverage/coverage95-20261105-implementation-achieve-family-scope.
source-inventory-v1.json contains all 88 original mappings, exact factory and
class/evaluation locations, return-type ancestry, data preconditions and hashes
of 72 owning source files. Draft inventory, edited-source preimage, before/
at-build/final hashes, exact patches, build log/receipt and summary-v1 are kept.

FRC!E93 is now source implementation-present in tracking-v8.json, including the
new typed-binding repair and existing/compatibility child implementations.
The provisional tally is 1089/1158 (94.04%): 36 added original rows, 30 new/
compatibility and 6 existing, with 69 other rows partial/unclassified/dependent.
Planning counts: newly tracked 9, source review 18, partial 10; others unchanged.
Recorded verification remains 1053/1158 (90.9%) and all 105 original acceptance
rows remain open. Original assertions, fixtures, tolerances, row texts/fingerprints,
denominator, ledger and qualification gates are unchanged. No acceptance credit
or completed full implementation audit is claimed.
