Live function Parameter instances and Global promotion
=====================================================

Coverage remains **1,019/1,158 (88.0%)**: 139 open, 82 more needed for 1,101.
Historical operation coverage stays 1,816/2,367. No whole requirement, including
FRR!E491 or its child conjunctions, is newly qualified. All 59 prerequisite hold
identities, original authority fingerprints, denominators and approval gates stay.

Fourteen exact parameter types now have native instance/reference evidence in
main, function-private and shared-Global contexts in both modes. The complete
321-parameter catalog and every prior field of all 566 factory-role rows remain.
The other 307 need live native-binding evidence; 42 of those already have older
bounded contexts. In total, 56 names have any bounded context and 265 have none
joined here. Neither queue excludes a type or closes the fourteen selected types'
owner/domain/force/solver/attached/science/whole-scope obligations.

Demonstrated production repair
------------------------------

``ManageObject::InsertIntoGOS`` attempted to insert an automatically promoted
system parameter using ``withName``, its already present global owner's name.
The map insertion silently failed, but the full parameter name was erased from
the local store. Native wrappers could still evaluate values, concealing the
missing namespace entry. Retained headless-v2 callback events show local
``PrivateOnly.DryMass`` and no ``SharedSC.DryMass`` in either map at main/direct/
direct/nested callbacks. The first headless-v1 error was overwritten by later
callbacks; its original source/error remain distinct from the stronger v2 record.

The sole production increment changes the insertion key to ``objName``, the
parameter's full name. Existing local-owner-shadow and duplicate-parameter guards
remain. The parameter is visible through the intended global store and its
existing cleanup/ownership path; no claim of zero ownership effect is made.
No formula, member, virtual slot or factory eligibility change is introduced.
``ManageObject.cpp.before`` matches HEAD; ``core-v3-promotion-key.patch`` retains
the exact delta. Technical review of this behavior/ownership repair remains open.

The schema-2 core inventory is
``build/nasa-coverage/core-change-review/20261103-function-parameter-bindings/``:
237 pending paths, 224 tracked differences and 13 untracked. All previous 236
working hashes are unchanged; ManageObject.cpp is the only new pending path.
``TrackCoreChanges.py`` adds its review description. No earlier core change is
implicitly approved by these tests.

What the native driver observes
-------------------------------

A test-owned synchronous Publisher Subscriber reads the current propagation
provider, actual current GmatFunction, live function object/global stores and
actual Parameter instances while the function is executing. It compares native
``Parameter::GetRefObject(GetOwnerType(), ownerName)`` directly with the live
scoped owner. No private-store pointer is dereferenced after return. Main-side
``Sandbox::GetGlobalObjectMap`` returns by value: that lookup map is explicitly
recorded as a snapshot containing live object pointers, not a live map identity.

The owner is always an actual Spacecraft, including types whose declared owner
base is SpacePoint. Its configured prototype, root runtime object and private
function runtime instance are distinguished. The shared owner equals the caller's
actual global owner; the private owner differs from its same-name caller shadow.
Exact native parameter class, system-parameter status, full name, return type and
owner-type metadata are asserted. ``GetOwner()`` bookkeeping can be null after
Clone by design; it is recorded without substituting for the actual reference.
A shared Parameter's local flag can be true on the first call and false later.
Actual flags are retained; global-owner identity does not imply that flag's value.

The selected families are:

* DryMass and TotalMass: private mass starts at 275, shared at 150; exact mutation
  by call count, with the root private shadow fixed at 987.
* RMAG, C3Energy and Cartesian: distinct root/private/shared Cartesian states;
  independent radius/energy identities and six actual vector components.
* A1ModJulian, CurrA1MJD and UTCGregorian: distinct authored UTC days and call
  seconds, actual parent epochs and exact string stamps. This checks scoped clock
  consistency, not independent time-conversion science.
* FuelMass: separately attached RootTank/LocalTank/SharedTank ChemicalTanks,
  actual tank reference/type/name/value and selected exact mutations.
* OrbitSTM and OrbitSTMA/B/C/D: unpropagated identity matrix and block dimensions/
  values. This is storage/reference consistency, not variational physics.

Mass/fuel bounds are exact, radius/energy <=1e-8, Cartesian <=1e-9, selected A1
<=2e-10 days and matrix components <=1e-12. Actual scalar/string/vector/matrix
``Write`` blocks are reparsed independently and compared with callback values.
One-second point-Earth RungeKutta89 pulses only trigger live callbacks; their
integration or stopping time is not a science witness and does not repair the
separate fixed-30-second failure.

Each case uses three missions, direct/direct/nested function calls, a distinct
main/private marker, unchanged caller/configured objects, exact prototype/source/
history checks, real keyboard file Cancel then Save, Unicode reopening, a final
mission without explicit Build, and actual report viewing. The native factory
catalog must equal all 321 system types plus Array/String/Variable on every
process; the other 307 retain metadata and pending owner-graph dispositions.

Retained results and corrections
--------------------------------

There are **32 selected GUI passes**: 28 new binding passes and four passes of
two unchanged original regressions (function vector resources and subscriber
Global loop). New binding scope totals 84 missions, 336 GmatFunction calls,
336 live callbacks (252 inside functions), 672 actual Parameter/owner checks,
672 typed Write values and 420 marker scalars. The first-pass offline audit
rejects eight corruptions: configured-owner substitution, caller-owner
substitution, wrong native type, wrong attached tank, vector component, epoch
stamp, matrix shape and omitted no-Build repeat.

All three failed native attempts remain with exact executed source capsules:

* headless-v1 DryMass: callback-order diagnostic after an overwritten earlier
  callback exception; later diagnostic retention preserves the first error.
* headless-v2 DryMass: actual shared Parameter missing from both scope maps,
  leading to the demonstrated production repair.
* headless-v4 Cartesian: Build failed with a direct nonreportable-vector Report.
  The original detailed parser diagnostic was not captured. Catalog metadata
  and Report::Initialize support the reportability explanation; no missing
  original error text is invented.

The first compile failure (address of rvalue from GetGlobalObjectMap) and its
exact source remain. The driver copies the main map; no production API changed.
Cartesian now uses the supported vector-to-Array[6,1] assignment and component
Report route, while retaining direct vector Write and actual live Rvector checks.
Report, metadata, reportability and scientific bounds are unchanged.

Reuse headless-v3's DryMass and two regressions, v4's twelve other passes and
v5's Cartesian pass. The visible-v1 selection passes all sixteen processes with
sixteen actual image maps and 281 native exposure events, stable source/image
inputs, initially minimized/resizable/nonfullscreen windows and zero screenshots.
No human observation or physical GPU claim is made. Software GL is selected;
there are no retained runtime-image or kernel byte copies. All native/build
processes are terminal. No existing failed probe is waived or relabeled.

Custody, replay and remaining work
----------------------------------

Authoritative files are in
``build/nasa-coverage/coverage95-20261103-function-parameter-bindings/``:
``evidence-audit-v1.json``, ``evidence-inputs-v1.json``,
``parameter-binding-matrix-v1.json``,
``remaining-parameter-binding-worklist-v1.json``,
``fixture-scope-review-v1.json``, ``final-source-identities-v1.json``,
``stage-receipt-v1.json`` and ``seal-verification-v1.json``. Original execution
sources remain separate from final documentation and all earlier sealed evidence.
The pre-native contract's empty production-change proposal stays preserved;
the fixture review and final receipt record the subsequently demonstrated repair.

Reusable new/affected replay (do not broadly replay preserved passes for context)::

  cmake --build build/linux-gui --target GmatQtNasaFunctionParameterBindingTests -j4
  python3 src/qtgui/tests/RunFunctionParameterBindings.py --filter '^QtGui.NasaFunctionParameterBindingFuelMass$'
  python3 src/qtgui/tests/RunFunctionParameterBindings.py --visible --filter '^QtGui.NasaFunctionParameterBindingFuelMass$'
  python3 src/qtgui/tests/AuditFunctionParameterBindings.py --stage build/nasa-coverage/coverage95-20261103-function-parameter-bindings --output /tmp/function-parameter-bindings-audit.json

The runner defaults to the fourteen supported registered cases; all are ordinary
CTest cases with native X, software GL and the engine resource lock. Both standard
acceptance build targets depend on the new native executable. It is not a fake
321-case success count or an exclusion of untested owner graphs.

Continue the complete 307-entry live-binding queue, first reusing retained
contexts and extending actual owner graphs for attached hardware, attitude,
force/frame/body dependencies, other owner classes, vectors/matrices/strings and
solver/external-clone routes. Keep all fourteen types' remaining obligations,
strict fixed-time/custom-Star failures, original E491 conjunctions, E477/E406,
prior source/science conflicts and all 59 holds. Scientific, technical-core,
full-current-build and full-NASA approval remain false. The full 95% goal stays
active and incomplete.
