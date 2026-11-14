Function service and callee observations, 2026-11-08
======================================================

Six unchanged original scoping failures are repaired: Planet, Moon, Comet,
Asteroid, CoordinateSystem and PropSetup. The expanded matrix passed 132 checks
headlessly and 132 visibly. Final review then reproduced and corrected an explicit
service-detach edge case; the six originals and lifetime witness pass again in
both profiles on the final source. Canonical acceptance remains
1078/1158 (93.1%), with 80 open and 23 needed for 95%. No whole-row credit is added.

Actual references with completed invocations retired
----------------------------------------------------

The first thirty-case batch had 24 passes and six failures. Read-only debugger
traces show that the body readers have a real control sequence but their public
solar-system getter returns null after retirement. The owned-resource readers
pass both direct-call inspections, then fail because the nested CallFunction's
public callee reference has been cleared. The native execution and earlier
literal/state assertions pass before these observations fail.

ObjectManagedFunction now records the actual solar-system service alongside its
existing observed global scope, using weak function/system identities and the
actual map-owner generation. Function's existing GetSolarSystem accessor exposes
that observation only after a call, while the system and scope owner are alive.
FindFunctionObject can use this guarded body lookup. Retirement still clears the
active solarSys pointer, local/global interfaces, wrappers and readiness state.
Nested restoration preserves the exact saved observation instead of recreating
it from another activation. Cold copies do not inherit a completed observation.

CallFunction separately records the actual callee's weak identity. Its public
GetRefObject inspector can observe that live callee after command retirement;
execution still uses the retired active member and ordinary initialization.
Scope rebind, definition-link rebind, rename, assignment and destruction clear
observations. Weak command/function identities reject deletion and address reuse.
An explicit SetSolarSystem replacement or null detach clears only the observed
service, preserving the actual global-map view. The new detach assertion failed
before this final correction, then passed after it. No manager, function, map,
solar system or invocation is kept alive by either view.
No name-based current-Sandbox substitution, private clone or active-frame revival
is used. Returned pointers remain borrowed and must be requeried after owners
change. Class data layouts and virtual ordering are unchanged; the added
helpers are nonvirtual. No scientific equation or original fixture was changed.

Executed checks
---------------

The repaired six cases and expanded lifetime witness first pass 7/7. The witness
retains its seven earlier map/owner checks and real Sandbox Clear/rebuild check,
and adds five service/callee observation records. These cover cold copies,
nested service identity, map generations, solar-system deletion/address reuse,
callee retirement/deletion/address reuse, rename and explicit scope rebinding.
All assertions in the six original cases remain intact, including actual
direct/direct/nested identities, three mission phases, private/caller separation,
independent results, file decisions and the checked report viewer.

The expanded headless receipt passes 131/131; a separately rebuilt original
VF13ad ResourceReportTruth worker adds the final 1/1. The complete visible receipt
passes 132/132 on the resizable, initially minimized nested desktop. Together
these are 264 expanded check executions: 262 originals and two executions of the
new lifetime witness. These broad receipts precede the final explicit-detach correction; their exact
772-owner source version is archived separately. The final source passes 7/7
headlessly and 7/7 visibly: all six originals plus the lifetime witness, including
explicit detach preserving the global scope. The entire 132-case matrix was not
replayed after that localized correction. The initial targeted 7/7 is additional,
not another set of unique capabilities. The visible Central progress case passes this time; its
earlier intermittent editor activation failure remains open because no root GUI
fix was established. The prior VX activation intermittence also remains open.

Finite inventory and continuation
--------------------------------

The current universe routing retains all 570 native names in 36 categories,
all 321 system parameter types, three user parameter types and the original
E491-E500 conjunction. It reconciles the other original fixture families with
the earlier 73-case catalogs. All 321 system parameters have existing operation
witness routes; that is source routing, not a new current execution or universal
scientific qualification. Named-main probes are kept separate from operation
witnesses and cannot exclude types after minimal configuration failures.

Thirty of the thirty-three GUI menu gaps have original operation witnesses and
passed both profiles in the expanded matrix. The remaining three native non-command routing gaps are
FminconOptimizer, MatlabFunction and MatlabInterface. MATLAB is installed; their
actual integration/scoping conjunction remains to be reviewed. The 54 language
commands remain assigned to the original command-scope obligations. Full any-
resource/any-eligible-global operations and all broader gates stay open.

Continue with the original all-body/all-frame orbit-parameter group, keys
FRR!E501/E502/E525/E526. Its next plan retains both complete original parent/child families
and the full FRR-16 cross-reference and the current owning sources. Initial source reads locate generic body,
frame, setter, precise-epoch and GUI selection routes; no new implementation
defect or complete qualification is established by those reads. Then continue
through the remaining finite audit groups without asking the user what is next.

The final lifetime witness also preserves all previously passing assertions.
All 1334 protected original inputs remain unchanged. Exact preimages, the bounded
delta, failed baseline/debugger logs, terminal builds, raw receipts, current
source hashes and a verified source archive are retained in this stage. One
compile attempt failed at the legacy non-const SolarSystem body lookup; the
localized lookup adaptation then built successfully. No installed runtime,
release, repository remote or package was changed. The goal remains active.
