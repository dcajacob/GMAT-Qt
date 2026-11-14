Hardware and FOV function graphs — implementation, 2026-11-06
===============================================================

Named hardware and fields of view now enter retained function input, saved-caller
and output scenes. Fifteen concrete native Clone methods and typed Copy methods
reconnect completed values. Their native value/cached-result policies remain;
no universal whole-object, scoping or resource-clone parent closure is claimed.

Creation and concrete copy routes
----------------------------------

Interpreter.CreateObject selects the registered type and function object map;
Moderator dispatches hardware and FOV through their specific creation methods.
HardwareFactory's nine spellings create seven native classes, including FuelTank
and Thruster aliases. The inspected estimation factory adds four RF classes and
the FOV factory three FOV classes. These source routes are not runtime registry
counts. Oscillator has native hooks too, but is absent from that inspected
estimation creatable list. Actual routes/hashes and remaining family scope are in
resource-family-source-map-v1.json. Factory category/literal counts alone do not
prove named-resource eligibility or requirement completeness.

Completed Clone hooks retain a context after concrete construction. Typed Copy
holds the old destination lease, checks type/self identity and reconnects after
assignment. NuclearPowerSystem uses the inherited typed PowerSystem Copy. Generic
RF antenna and nested owned error/FOV references use actual scene identities.
Owned Imager FOV copies now stage and validate the concrete child before adoption;
returning the source cannot delete it. Receiver copies stage all error models,
reject source/peer/reused/type-incompatible Clone results, and retire old models
on adoption. Sensor stages both native signal allocations through its own field
writes. Additional base/derived constructor failures and raw signal ownership
contracts remain open; this is not universal exception safety.

Thruster and power dependencies
--------------------------------

Thruster's passive typed binder maps actual spacecraft/frame/body/current-owner
tank references and declaration names. It preserves mixture order, retires its
owned local frame and derivative state/origin associations, and avoids the eager
SetSpacecraft frame factory during binding. Unbound prototypes can retain tank
declarations; a missing tank on a bound owner fails. Assignment formerly leaked
its local frame by overwriting the pointer; assignment/destruction now retire it
independently of the current mode, preserving DEBUG_MEMORY accounting.

PowerSystem/SolarPowerSystem map actual owner/Sun/origin/shadow roles and create
fresh native ShadowState storage. Retained solar initialization keeps mapped body
identities; ordinary native initialization still resolves configured SS names.
Same-system setter calls preserve an already bound Sun. Spacecraft retained
initialization calls the typed power initializer and respects a false result.
Default/unbound names retain native lookup policy, not a claim of complete
private-scene name resolution. Formulas, coefficients and tolerances are unchanged.

Owner construction and context lifetime
----------------------------------------

A scoped thread-local GmatBase guard defers child root hooks while native owners
are incomplete. Actual Spacecraft/ODE/PropSetup copy constructors and assignments,
station component copying, owned Imager FOV copying and receiver model copying
use it. Inherited leases remain live until the completed root can register and
reconnect its native children. FunctionManager imports source/prior child contexts
before typed hardware/force binding, then clears child leases so the containing
native owner retains the scene. This avoids building a child-owned parent graph
in the middle of parent construction. No maneuver starts merely to copy or bind.

The v2 build passed but source review found three guards placed in default rather
than copy constructors. V3 corrected the actual copy-constructor sites. V4 also
imports/retires embedded force contexts; v5 distinguishes bound missing tanks
from unbound prototypes. Superseded sources/hashes/receipts remain. No runtime or
acceptance failure/pass was inferred from these compilation receipts.

Compilation and remaining scope
--------------------------------

Private integrated compilation passes: v1 400 steps /186.66 s; v2 747 steps /223.18 s; v3 68 steps /7.43 s; v4 66 steps /5.91 s; v5 66 steps /2.93 s.
Final 28 source hashes match v5. DEBUG_MEMORY syntax-only compilation of final
Thruster.cpp passes in 0.42 s, with no linking/execution. Sessions
77201/76355/57913/72179/14077 closed. Two new PowerSystem virtual contracts and an
exported construction helper require a consistent provider rebuild; no native
instance fields or GmatBase virtual slots added. External ABI/source/platform
behavior remains unqualified. Warnings, exact preimages, hashes, indexed patches,
notes/map, logs/receipts/summary and planning preimages are retained under
``build/nasa-coverage/coverage95-20261106-implementation-hardware-function-graphs``.

Interior owned-component public roots can conflict with the owner copied into
the same dependency graph; explicit alias/provenance support remains to implement.
Keep that original scope open rather than stripping owner references. Continue
attitude/plate, solver/subscriber/event/interface/data/filter/measurement/stop-
condition/function/body/solar-system/custom-provider creation and ownership routes,
globals, private bodies/kernel pools/NAIF policies, undefined configuration, direct/
later owner changes and every recursive/exceptional/body/frame/scientific clause.

No tests, GUI, missions/functions/models/solvers, numerical comparisons, acceptance
readers or seals ran. Original assertions, fixtures, texts, fingerprints,
denominator, ledger and gates remain unchanged. No installed/shared runtime,
package, commit or push changed. Tracking-v32 stays 1093/1158 (94.39%), 65 other
original rows; recorded source-versioned verification stays 1053/1158 (90.9%),
all 105 acceptance rows open. No broad parent credit. Goal active; implementation
incomplete and verification deferred.
