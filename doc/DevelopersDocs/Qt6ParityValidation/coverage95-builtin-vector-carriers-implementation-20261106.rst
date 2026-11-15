Native built-in vector output carriers, 2026-11-06
=================================================

The preceding v50 turn made source progress. Its twelve final hashes and final
successful receipt were current at turn start. This batch makes concrete source
progress across four production paths; the full goal is active and incomplete.

Value storage and metadata
---------------------------

FunctionManager now creates native RvectorVar objects for VECTOR_WT built-in
formal outputs. Generic vector values have completed native Clone/typed Copy,
independent numerical storage and an explicit size constructor for writable
internal outputs. Existing Parameter completion helpers retain exact native type
and reference/value binding policy. Derived classes still need their complete
native provider; no source-script resource type or new vector grammar is added.

RvectorVar assignment previously copied a new vector into the old buffer size.
It now stages the independent data before base assignment and exchanges storage,
so size changes and self-assignment retain coherent data. The previous context
stays alive through assignment. VectorSize has its declared integer type and
parameter count, is read-only, and cannot be changed as a command-mode property.

Native vectors have no orientation. Output metadata can declare either one row
or one column, with a positive native size; FunctionRunner exposes the resulting
one-dimensional value consistently with the existing vector expression path.
Malformed output metadata lengths fail before formal names/dimensions are
indexed. Scalar, Array, vector and String output creation uses ownership through
allocation/sizing/logging; failed Array sizing no longer leaves an unowned
allocation. Scientific formulas, tolerances, units and assertions are unchanged.

Full original scope
--------------------

The existing rebuild path still keeps an old formal output object at a matching
name. Repeated type/shape changes and borrowed/aliased FOS ownership need complete
refresh/lifetime implementation; this batch does not declare that path complete.
Legacy output discovery, every custom vector shape/arithmetic case, all remaining
solver/resource/command/plugin and interpreted function providers, and the full
original scientific/private/global/caller/interior/recursive/exceptional/later
scope remain required. No fixed-output or narrower acceptance profile substitutes
for the original requirements.

Compilation and accounting
---------------------------

Stage: ``build/nasa-coverage/coverage95-20261106-implementation-builtin-vector-carriers``. Four source preimages, before/build/final
hashes, indexed patches, source map, build log/terminal receipt and four planning
preimages are retained. Added delta whitespace: zero.

* v1: 76/76 steps, 7.25 seconds, return 0.

Final source matches the successful build. Matching configured core/Qt/console,
enabled plugins and Python consumers rebuilt. No instance fields or virtual
slots were added; native overrides, metadata behavior and a new constructor
require matching consumers. External source/ABI/platform/runtime behavior is
unqualified.

Only compilation and essential source debugging ran. No tests, GUI, missions,
functions, models, solvers, numerical comparisons, acceptance readers, seals,
packages, installed/shared runtime, commits, pushes or external messages ran.
Original assertions/fixtures/tolerances/texts/fingerprints/denominator/ledger/
qualification gates remain intact. Tracking-v51 remains 1093/1158 = 94.39%; the
recorded source-versioned qualified baseline remains 1053/1158 = 90.9%. All 105
original acceptance rows remain open; 65 rows remain outside the engineering
tally. No additional row or verification credit; full implementation incomplete.

Next
----

Complete repeated builtin output metadata type/shape refresh and formal ownership/alias lifetimes, legacy output discovery and custom vector metadata/arithmetic. Complete every remaining solver/estimator/simulator and other resource provider, owned plot/subscriber/interface/measurement resources, all command/plugin ownership/reference/retirement paths and passive UserDefinedFunction/GmatFunction body/prototype admission. Finish snapshot cross-resource/private-formation/caller/global/recursive/exceptional bindings, arbitrary ownership/interior/scope-table/service/engine-reset/later lifetimes, GetEphemStates destination-owned-frame alias Copy and every original body/kernel/NAIF/scientific/resource requirement. Keep complete original scope and verification deferral. Research POV independently without asking the user for a dialect.
