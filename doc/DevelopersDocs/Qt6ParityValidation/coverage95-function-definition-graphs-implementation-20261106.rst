Compiled function definitions and prototype ownership, 2026-11-06
=================================================================

The prior goal turn was progress: v41's four final source hashes remain current
at this turn's start, and build receipts 1/0 were inspected. This turn changes
fourteen production paths. No original requirement or acceptance assertion is
closed by the partial family implementation below.

Command graphs
----------------

Three appended GmatCommand virtual contracts expose actual structural links,
reconnect copied links, and detach links during failed staging. BranchCommand
covers all branch slots, including null slots, and clears execution cursors and
function-command caches. CallFunction clears its copied callee and argument
resource caches. The graph walker does not use GetNext's active execution cursor.

CommandUtil clones every reachable concrete command once. Native deletion owns
ordinary next and branch-root edges; BranchEnd next is a non-owning return link.
The ownership tree is checked separately from navigation cycles. Invalid multiple
owners/cycles or backlinks outside that ownership component fail explicitly.
Previous links outside the captured sequence detach. After reconnect, every
structural edge and deletion role must match its source. Null/source/peer/reused and
wrong runtime-type Clone results are rejected. Individual Clone must return a
native detached command; complete navigation is installed after all identities
exist. Failed reconnect staging detaches all copied links before recursive native
destructors run. Success transfers the usual root/branch native ownership.
No command whitelist or reduced function language was introduced. Providers with
additional navigation roles must implement the structural contract; arbitrary
external plugin/type-universe coverage is not proved by this implementation.

Function definitions
----------------------

UserDefinedFunction's cold copy constructor and assignment into an uninterpreted
destination retain a deep compiled sequence and parser prototype tables. Source
file, signature and native source-cache policy remain; assigning a different
source into an already interpreted destination still requires the native rebuild
lifecycle, and identical-definition assignment retains that destination's cache.
That native rule does not yet implement arbitrary retained Function value Copy.

Owned prototypes clone by actual identity, including retained/queued automatic
objects. Declaration/automatic keys and null entries remain, aliases share their
copy, self-table references point at the new function, and sandbox identities are
compared before dereferencing borrowed entries. Native borrowed function/body/
sandbox entries remain borrowed; invocation FOS/global stores and output wrappers
are not checkpointed. Current/older prototype owners remain alive in retirement
order when later definitions can borrow them. Native command/resource embedded
refs still require the remaining passive family work before private Function
admission. Source-file equality is not proof of complete value equivalence.

Declaration cleanup collects unique actual owners before deleting, clears aliases
before their later keys can inspect freed objects, and skips pool-owned/sandbox
borrowed objects. Automatic cleanup handles declaration aliases and both ordinary
and optional collected instances once. AddAutomaticObject preserves the first
persistent parser prototype. Another managed runtime instance is borrowed; an
additional unmanaged wrapper instance is retained for later cleanup. Reserving
ownership bookkeeping precedes publication. It must not overwrite the prototype
with a temporary FOS object that will disappear at invocation retirement.

Cold sequence retirement skips RunComplete. An initialized sequence retains native
cleanup behavior; the root is held by RAII if a child callback throws. Commands
retire before prototype owners. GmatFunction prepares every structural command's
current owner/configuration lookup before normal validation/initialization.

Runtime call preparation
--------------------------

CallFunction structural copies detach call/argument caches. Incoming local/global
lookup honors local-null shadowing, clears an absent previous callee, and checks
actual Function/ObjectManagedFunction types. Python's separate native call path
does not acquire a spurious GMAT resource binding; MATLAB keeps its external call
path. Call-site name/ref binding uses the v41 API without mutating shared function
names. Parameter role mismatch cannot fall through to an unchecked Function cast.
Every native call initialization refreshes system, force, internal-frame and
publisher services; actual invocation also forwards its publisher to every
structural command in the current compiled sequence; FunctionManager execution forwards its current internal frame.
Explicit null internal-frame setters clear a previous run's frame. The copied
commands still borrow publisher/trigger engine services. External service lifetime
past engine reset and arbitrary recursion/replacement remain unqualified.

Compilation and accounting
-----------------------------

Stage: ``build/nasa-coverage/coverage95-20261106-implementation-function-definition-graphs``.
Exact fourteen preimages, before/build/final hashes, indexed patches, source map,
logs, terminal receipts and planning preimages retained. New source whitespace0.
Builds all pass: v1 ('226', '226')/179.88s;
v2 ('81', '81')/144.99s;
v3 ('224', '224')/398.74s;
v4 ('67', '67')/8.06s.
Final source matches v4. v3 corrects the automatic-prototype policy after inspecting
GmatFunction's use of that persistent prototype. Compilation is not behavioral
proof. Configured Qt/core/console/enabled plugins/Python bindings and test targets
were compiled/linked; none were executed. One prior Qt deprecation warning in the
broader rebuild is unrelated to the new code. No optional-debug runtime claim.

GmatCommand adds three virtual slots; UserDefinedFunction adds one owner field.
All enabled consumers were rebuilt together. External ABI/source/platforms and
scientific behavior remain unqualified. No tests, GUI/mission/function/model/
solver execution, numerical comparison, acceptance readers/seals or qualification
campaign ran. No installed/shared runtime, package, commit, push or messages.
Original assertions/fixtures/texts/tolerances/fingerprints/denominator/ledger/gates
remain unchanged. Tracking-v42 remains1093/1158=94.39%; recorded source-versioned
qualified baseline1053/1158=90.9%, all105 acceptance rows open. No broad row credit.
Full implementation and the full implementation audit are still incomplete.

Next
------

Implement passive Function/ObjectManagedFunction/UserDefinedFunction/GmatFunction and native callable resource ownership/reference providers, register compiled commands/prototypes and all embedded expression/wrapper owners before binding, and finish each command/private-resource family. The retained private Function gate remains closed until that full closure exists. Continue direct RVECTOR nodes and all original global/caller/interior/later/kernel/NAIF/recursive/exceptional/scientific/resource/command requirements. Finish implementation before verification; independently research POV models/exporters without asking the user for a dialect.
