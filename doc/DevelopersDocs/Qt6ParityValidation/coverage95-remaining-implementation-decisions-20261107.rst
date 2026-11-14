Remaining implementation decisions and compatible cost syntax
==============================================================

FRC!E103 names NonlinearConstraint as the cost-function command. The current
comparison command did not supply that behavior. Following the user's preference
to support worksheet and current behavior, the shared interpreter now accepts::

   NonlinearConstraint optimizer(cost);

as a native Minimize objective. Comparison forms retain their existing constraint
implementation, for example::

   NonlinearConstraint optimizer(value <= limit);

The classifier ignores quoted labels/string text, requires a balanced outer
argument and rejects extra arguments/trailing text from the cost route. Native
Minimize validates and evaluates the objective and enforces its existing finite
real and single-objective policies. No solver calculation or constraint tolerance
changes. Its new spelling flag survives copies/assignment and script generation;
the Qt command form edits objective source spans while retaining the keyword,
labels and surrounding formatting. The flag is private command state, with no
new parameter ID or virtual slot; matching rebuilt native consumers are required.

Compilation completed 72/72 steps in 27.74 seconds and, after the final classifier
adjustment, 66/66 in 9.37 seconds, both without warnings. Both compiler sessions
are terminal. Six source/help preimages, patches, final source hashes and seven
unchanged reference hashes are retained in
build/nasa-coverage/coverage95-20261107-implementation-nonlinear-cost-form.
Help XML parses. No tests or parser/native/GUI/solver runtime verification ran.
This implementation does not change FRC!E103's original disposition or grant
acceptance credit.

What the remaining decisions actually concern
------------------------------------------------

The machine-readable ledger is
build/nasa-coverage/coverage95-20261107-remaining-implementation-decisions/ledger-v1.json.
It preserves original text, fingerprint and disposition for all 34 rows classified
as specification/external/historical and the seven dependent parents.

The 17 originally specification-classified rows include the cost clause above
and two gravity-coefficient rows. GravitationalAccel already exists on both
Thruster and ImpulsiveBurn; no duplicate property is needed. The remaining 14
include parent conjunctions and these actual missing definitions:

* FRR!E50/E53: the worksheet calls for an SPK attitude kernel, while the native
  attitude reader uses CK. The intended attitude model/data must be defined.
* FRR!E221/E227: rotation drift has no supplied equation or units. The retained
  research supports an angle/rate interpretation as an inference, not an
  authoritative correction. Do not invent spin acceleration.
* FRR!E377/E381: the required OEM coordinate-system list is literally unfilled.
* FRC!E8/E47/E51: a real-number literal has no identified mutable control location.
* All Others!E13: the units clause says non-dimensional; the intended field set
  and missing unit metadata are not specified by that wording.
* All Others!E294-E296: processor/RAM minima are Xxx Mhz and Xxx Gb.
* All Others!E310: the entire requirement is Validation, with no defined behavior.

Nine external-dependency rows concern Mars-GRAM and VF13AD. Existing DragForce
density/input-file hooks and core VF13AD references are retained. No correspondingly
named plugin artifact appears in the current integrated build plugin directory.
The checkout does contain the licensed binary
application/plugins/thinksys/vf13ad/libVF13adOptimizer.so.R2026a and its provider
README. That README says wrapper source, excluding HSL code, is available on
request. Binary presence does not establish compatibility with the current native
build or the missing derivative controls. Do not describe absence from the
integrated build as absence from the checkout. This is not a new runtime registry
or vendor-license probe. Authentic compatible interfaces/backends are needed;
synthetic density or a renamed substitute optimizer cannot establish the original
requirements.

The cited CCSDS 502.0-B-1 section 4.2.2 expressly has no definitive authorized
REF_FRAME list. Its Table 4-3 contains examples. Those examples are now located,
but do not resolve the worksheet's unfilled required enumeration. The existing
writer maps EME2000, TOD, ICRF and TDR plus its retained compatibility types.
No new physical frame realization or frame qualification is inferred.

For the original MATLAB modes, MathWorks documents engOpenSingleUse as
Windows-only and UNIX engOpen as starting a new process. The original clauses
name those APIs explicitly. A non-Windows substitute cannot be silently presented
as literal compliance or proof of cross-process sharing. See the current stopping
point in coverage95-bounded-finish-list-20261107.rst and the source-only checkpoint
under build/nasa-coverage/coverage95-20261107-implementation-stop-checkpoint.

Eight historical-platform rows name XP/Vista/Windows7, Leopard and Fedora Core.
Modern preview runners do not establish those literal targets. The seven other
parent rows depend on original children and are not seven independent features.
The ledger does not certify the other 64 original open rows or close remaining
POV/profile scope.

Phase-order clarification
---------------------------

The user answered: verify implemented features while keeping unresolved
requirements and all acceptance gates open. Verification is now authorized;
the earlier ledger's pending-question fields are historical. The first batch is
recorded in coverage95-verification-restart-20261107.rst. It has 28 passing cases,
four successful focused native replays and two unresolved original integration
timeouts. This does not define unspecified behavior, supply missing backends or
grant whole-row qualification.

All original rows, assertions, fixtures, tolerances, denominator and qualification
gates remain unchanged. The verified baseline remains 1053/1158 (90.9%);
1093/1158 (94.39%) is still a carried engineering estimate. The full implementation
audit and overall campaign remain incomplete. No commits, pushes, packages or
installed-runtime changes were made.
