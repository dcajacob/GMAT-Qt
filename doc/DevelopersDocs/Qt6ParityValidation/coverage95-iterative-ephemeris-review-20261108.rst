Iterative ephemeris current-source review, 2026-11-08
====================================================

The unchanged original estimator, failed/stopped-output and nested-solver
checks produced 26 passes and two failures in 28 executions. No production
code, original test, assertion or tolerance changed in this review.
FRR!E406 remains open; canonical acceptance stays 1078/1158 (93.1%), with
80 original requirements open and 23 more required for the unchanged 95% target.

Executed original routes
------------------------

Positive and Negative batch OEM checks and InitialGuess, IterationLimit and
Reentry batch-mode checks pass in private X11 and visible nested desktop:
ten original executions. Positive and Negative each retain 1435 output points,
one segment and 358 observations. Their independent circular-orbit bounds and
initial-state recovery assertions pass; the limited-iteration mode retains its
independent one-step, nonconvergence and report assertions. The mode checks do
not themselves establish OEM output behavior.

Five stopped/failed-output routes (Stop, Singular, IterationLimit,
InvalidSolveFors and InternalRankDeficient) and three nested routes (outer
Target/DC, Optimize/Yukon and constrained Optimize/VF13ad) pass in both display
profiles: sixteen further original executions.

Retained numerical failure
--------------------------

The original unconstrained forward-difference VF13ad nesting case fails its
independent exact final-orbit comparison in both profiles. Its fixture already
records this numerical limitation. The optimizer accepts U=1.9999499999991925,
which corresponds to radius 7199.994999999919 km, rather than 7200 km. This is
consistent with the stationary point 2-h/2 of the forward-difference gradient
for the original (U-2)^2 objective at the unchanged h=0.0001.

All 1441 exported samples match the globally accepted native publisher samples
within 9.1e-13 component units. An independent diagnostic using the actual
accepted radius gives maximum component errors of 8.71e-7 km and 6.37e-10 km/s.
The unchanged original exact-radius oracle instead gives 0.22286 km and
0.00021939 km/s, exceeding its original 2e-5 km and 2e-8 km/s bounds. The actual
accepted radius diagnostic does not replace that original oracle or qualify
the failed test. The scalar optimum assertion also remains unchanged. No
proprietary VF13ad state layout, derivative mode or numerical repair is guessed.

Owning source and limits
-----------------------

SolverBranchCommand retains an effective outer solver-state stack;
EphemerisFile suppresses trial samples and trial side effects while closing
accepted writers on stopped/error paths. RunEstimator marks actual final-pass
publications and avoids a batch FINISHED publication at the old terminal time
after the fitted initial epoch has been restored. These existing routes were
reviewed; no production changes were necessary for the passing finite cases.

Broader output formats, all filter/smoother science and discontinuities, strict
SPK precision, arbitrary nesting/recovery/event/segment/epoch paths and full
current-build/platform gates remain unqualified. No whole E406 credit is claimed.
The 1334 protected original inputs remain byte-identical. Formal MATLAB/Mars
holds, worksheet ambiguities, canonical ledgers and denominators are unchanged.

Evidence and continuation
-------------------------

Stage: build/nasa-coverage/coverage95-20261108-iterative-ephemeris-current-review.
Plans retain exact original E406 text/fingerprint. Execution manifests have zero
NASA claims. The read-only analyze-nested-v1.py reproduces the retained failure
diagnosis in both profiles; raw native publication, OEM, optimizer, estimator,
CTest and GUI receipts remain available. Current owner hashes and a verified
source archive bind the reviewed sources. All four test runs and both selected
worker builds have terminated. Installed/released runtimes and Git remotes are
unchanged.

Continue autonomously with original Function I/O E450, output bracket E470 and
core parent E472. The next component has a frozen current-source plan and its
original bounded function file, call, definition and I/O checks are underway.
Keep unresolved requirements open; no user approval is pending.
