Iterative ephemeris requirement conjunction
==========================================

FRR!E406 requires ephemeris output from the solution of iterative processes.
The admission joins the previously executed format/backend/domain evidence with
new convergence-only output witnesses. User ephemeris now excludes a terminal
nonconverged attempt. Explicit RunInitialGuess remains a requested trajectory,
and internal event-search recorders retain their trial data.

Two additional defects were fixed while closing this requirement: nested Target
initialization no longer advances the saved initial-guess epoch, and the MATLAB
Fmincon callback path publishes its native convergence result before the final
output replay. External RunInitialGuess marks its single executed branch as the
requested output pass. Diagnostic reports and numerical solver work remain intact.

Evidence includes all five output formats, eight current solver-provider names,
the alias and noniterative-mode distinctions, twelve saved components, and five
retained CK data replays. The new selected contexts include six Target/Yukon
controls, three batch contexts, three real MATLAB Fmincon contexts, and one
physical-tolerance-enabled VF13ad context. The latter reports CONVERGED; the
earlier prediction of a distinct IN_TOLERANCE status remains a failed probe.
No claim of executing that distinct status is made. The source gate explicitly
accepts either native successful status.

The scope is output phase membership and the retained independent file/state
oracles, not a new qualification of every numerical solver or force model.
Source-versioned passes are reused; no whole-current-build or full NASA claim
is made. Original requirement fingerprints, assertions, tolerances, failures,
and the denominator are preserved. Admission and final sealed replay produce
the authoritative coverage count; no native tests are launched by that replay.
