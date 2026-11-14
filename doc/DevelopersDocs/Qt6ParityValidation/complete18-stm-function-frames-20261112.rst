Selected-frame STM calculations across native function calls
===========================================================

Two missing native function routes now have passing, independent evidence:
a coordinate frame passed as a value input, and a coordinate frame created
inside the function. Both use a guide-spacecraft VNB frame, with spacecraft,
guide and frame references resolved in the function's actual object store.
No production source changes were needed.

Coverage remains 1,106/1,158, with 5 of the agreed 18 complete. These witnesses
close the stated function routes, not the whole FRR!E525 conjunction. No full
regression, GUI rerun or complete-current-build qualification was performed.

Frozen numerical and ownership obligations
-----------------------------------------

Each case first advances the caller's spacecraft and guide for 60 seconds under
Earth point-mass gravity, including the STM. It then makes two direct by-value
function calls and one nested call. Each function advances its private inputs by
another 30 seconds. Therefore its native STM must retain the original reference
epoch and represent a 90-second trajectory, while the caller remains at 60
seconds. Restarting from an identity STM at the function boundary would violate
the independent variational expectation.

The executed native callbacks inspect all five readonly STM/partition Parameters
before and after each private propagation. They establish actual owner, frame
and guide pointer bindings, distinct caller/local spacecraft identities, current
states and native full STMs. Actual Report commands separately publish the five
matrix Parameters before/after each call and after returning to the caller.
Configured spacecraft prototypes remain unchanged. Scalar function returns and
a caller continuation sentinel also agree with their independent expectations.

Across the two cases, all six function calls pass, including two nested calls.
There are 12 live function samples, 60 native Parameter reads and 1,008 reported
matrix entries. Closed circular variational flow, independent guide-frame
rotation/rate algebra and independent circular states supply the expectations.
The original block bounds remain A/D=2e-8, B=2e-6 seconds and C=2e-11 inverse
seconds; state bounds remain 2e-5 km and 2e-8 km/s. Maximum observed internal STM
entry error is 1.1923917497824732e-7 in the time-scaled B block. Four propagations
per case are necessary to exercise these previously missing function boundaries;
older passing cases were not rerun.

Inputs and executed source provenance
------------------------------------

The stage is build/nasa-coverage/complete18-20261112-stm-function-frames-v1.
The two fixture files, numerical oracle, worker and test registration were
frozen before execution. Loaded-image maps bind the actual worker, GmatBase,
GmatUtil and GmatFunction plugin. The production base/util library hashes equal
those in the previous sealed STM alignment batch.

The first executed source capsule covered the base owners and new tests, but
omitted the dynamically loaded GmatFunction plugin's owning sources. This is
recorded explicitly. A supplementary capsule captures those 15 source/build
files and a guarded build of the worker and GmatFunction targets reports
``ninja: no work to do.`` The resulting worker and libraries exactly match the
already-executed image hashes. No test was rerun or relabelled to establish this
binding. Both capsules and the chronological provenance record are retained.

All 1,334 original protected inputs remain unchanged. Seven negative evidence
controls reject a missing nested sample, wrong guide binding, caller aliasing,
lost incoming STM, caller state leakage, changed Report output and lost readonly
metadata. These controls and the final frozen mathematical/evidence replay
launch zero native tests. The unchanged whole coverage ledger is not rerun.

The new engineering seal extends receipt
``b38f3af44731e254369766aad632a8a7486812e411ed45ef6909fefb607b13dc``
from the alignment batch. The canonical requirement prefix remains
``de77757b896aa68c65df2462c026b58bf11ed684b502148fa5a14c0593111574``.
Historical passes remain limited to their original executed source versions.

Remaining whole-row work
------------------------

FRR!E525 still needs its complete supported frame/parameter/body conjunction,
selected-frame solver-loop reset/reference-epoch/restoration witnesses, failure
unwind and recovery routes, broader domain/atomicity and extended-STM evidence.
Defined owner-dependent frames require their full state-dependent Jacobians;
no unsupported rejection is counted as proof of mathematical singularity.
