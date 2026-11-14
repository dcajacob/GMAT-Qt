Ephemeris trial notifications and write-state restoration, 2026-11-05
===================================================================

This batch repairs concrete FRR!E406 implementation gaps. The full requirement
remains partial: command-driven Filename rollover and function-local lifecycle
semantics still require owning-source review. No requirement or acceptance
credit is assigned. Tracking stays 1086/1158 (93.78%); recorded verification
stays 1053/1158 (90.9%), with all 105 original acceptance rows still open.

Defects and source changes
-------------------------

EphemerisFile previously skipped trial orbit samples only after creating a
writer/file and handling inactive output. Its property-change callback only
excluded Vary during SOLVING, and did so after setting the event epoch. Other
trial assignments could flush accepted buffers, reset interpolation and append
trial discontinuity comments. ToggleOn could create a trial writer; ToggleOff
could reset accepted writer state. Trial changes to activation/write flags also
survived into final replay because solver loop snapshots restore spacecraft,
not subscriber flags.

The trial packet guard now precedes writer creation and inactive/segment
handling. It consults the existing live enclosing-solver classifier. The common
IsEventFeasible route excludes trial maneuver, propagator and spacecraft
property notifications before event epochs, segment buffers or comments change.
The old Vary-only late check is removed.

Trial ToggleOn/ToggleOff still update the command-visible WriteEphemeris flag,
while avoiding writer creation, interpolation reset, toggleStatus and accepted
gap state. Toggle's existing Activate call still sets its command-visible active
flag during the trial. When effective run state first enters SOLVING,
EphemerisFile saves its entry activation/write flags. On transition out of the
trial, it restores those flags before the final replay applies its own toggle
sequence. Nested inner final passes remain SOLVING while an outer solver is
iterating, so they cannot prematurely restore the outer entry state.

HandleEndOfRun restores entry flags if cleanup arrives before an IDLE state
notification, then uses the existing writer finalization. The terminal packet
check remains before trial suppression, so already accepted buffers can still
be finalized. This does not add or prove a new Sandbox error-notification
contract. Copy construction uses fresh default snapshot fields; assignment
explicitly discards live snapshots. Three private booleans are added to the
class; dependent images must be rebuilt together. Public signatures, parameter
IDs, script grammar, formulas and tolerances are unchanged.

Native final-pass semantics retained
-----------------------------------

Target and internal/external Optimize replay a final trajectory after their
terminal state. Their RunInitialGuess paths also perform a final replay.
Optimize separately records IntegerSolverStatus==CONVERGED, so SOLVEDPASS does
not itself assert mathematical convergence. The repair preserves those command
contracts rather than interpreting every terminal non-converged replay as an
intermediate iteration. Existing Target/Optimize help also distinguishes
converged corrections from the last calculated values.

BatchEstimatorBase creates its fitted propagation-only final pass only for
CONVERGED status. RunEstimator publishes its initial sample and each final
propagation step, while avoiding publication of restored batch initial
conditions under the old terminal epoch. SeqEstimator's IsFinalPass is true
for its filtering trajectory; SmootherBase admits SMOOTHING/PREDICTING states.
Those paths and the prior enclosing-solver suppression remain unchanged.

Writer and lifecycle review
---------------------------

CCSDS, SPK, Code-500 and STK writers receive orbit data through the common
EphemerisFile path. Their format-specific orbit buffering/finalization code is
unchanged by this batch. Discontinuity callbacks and Toggle side effects were
the concrete bypasses repaired here.

Filename remains a separate command-mode route: its current setter can call
FinishUpWriting before resolving/checking the destination directory, and
SetFileName changes writer name fields. It needs a full actual stream/kernel
rollover and trial/function lifecycle review before FRR!E406 is declared
implementation-complete. FunctionManager finalizes local subscribers before
unsubscribe/deletion; global subscriber ownership is distinct. No universal
lifecycle or strict convergence-only contract is inferred from those routes.

Build and deferred work
-----------------------

Stage: ``build/nasa-coverage/coverage95-20261105-implementation-ephemeris-trial-notifications``.
The configured integrated build passes 77 steps in 167.53 seconds; session
66936 is terminal. See build-v1.json/log for the result. Source
preimages, exact patches, source-at-build hashes and the final summary are kept
in the stage. Test/probe clients are build/link targets only.

No tests, GUI, targeting/optimization/estimation runs, numerical comparisons,
acceptance readers/seals or qualification runs are permitted in this phase,
and none were launched. Original fixtures, assertions, tolerances, fingerprints,
denominator, acceptance ledger and gates are unchanged. No installed/shared
runtime, package, commit or push update.

The next implementation pass must finish Filename rollover and function-local
lifecycle handling. Later verification must cover trial assignments and burns,
pre-existing accepted segments, toggles before/inside/after nested solvers,
initially disabled writers, final replay toggle ordering, non-converged and
RunInitialGuess behavior, batch/filter/smoother output, Stop/error cleanup,
copy/re-execution and all supported formats. Verification stays deferred until
the remaining implementation pass is finished.
