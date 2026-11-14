Covariance parser rejects malformed first rows
==============================================

Implementation-first repair in Interpreter.cpp only. Both covariance parser
paths checked subsequent rows but omitted the first row's square-matrix shape.
They indexed the first row before checking whether any rows existed. A concrete
before-run confirmed that Sat.Covariance.CartesianState accepted
[1 0; 0 1 0; 0 0 1] and completed its mission. Source-loop bounds show that only
a2x2 prefix was consumed; the before-run did not inspect matrix values.
Plain Sat.Covariance assignments were already rejected by the current spacecraft
property contract; this increment preserves that rejection.

Both parser paths now reject an empty row list before indexing it and require a
nonempty square matrix, except for the existing supported single-row diagonal
form. Existing dimension limits, subsequent-row checks and their diagnostics
remain. No covariance positivity/symmetry policy, formula, whole-matrix-size
requirement, getter, serialization or plain-property eligibility change. This
does not promise atomic rollback of LHS construction or partial setter failures.

Stage: build/nasa-coverage/coverage95-20261105-implementation-covariance-shape.
Exact interpreter preimage and incremental-v1.patch retained. Patch SHA256:
31dd4539ee4fdc0bc365d8090fa5fa4db316e8ad817b463e5c35be1b628d7eb1.
Reverse patch check passes. Private integrated-current build passes8.47s without
diagnostics. summary.json records source/native image hashes. Seven monitored
installed images unchanged; public code/packages unchanged by this increment.

Four small manual diagnostic scripts established the before behavior. The same
four scripts execute unchanged afterward, plus valid-square and empty qualified
forms. All six after-smokes behave as expected: qualified ragged and empty
matrices fail, qualified diagonal and square forms complete, and both plain
assignments remain rejected. Command/input/log/receipt files and their hashes
remain. This adds six manual inputs, not a new suite, fixture catalog,
registration, qualification reader, matrix or seal. New formal backfill stays
deferred. No GUI execution or GUI requirement credit is claimed.

Native console launches use private isolated startups/output/settings, with
DISPLAY/WAYLAND_DISPLAY/QT_QPA_PLATFORM removed and private runtime library paths.
LD_DEBUG=libs identifies the private core. Its expected unsuccessful optional
plugin-symbol probes are not mission failures; use recorded return/completion
outcomes. All handles terminal, zero screenshots. Replay saved command.json files
with equivalent environment and fresh isolated startup/output directories.

These checks prove bounded input acceptance/rejection, not covariance values,
complete generic-Covariance persistence, scientific correctness or GUI behavior.
Generic getter/alias/Save persistence and complete FRC!E132 remain open. No
original row admitted. Verified1053/1158,105open,48neededfor1101;59holds,
31resolved/28unresolved including6MATLAB,37MATLAB identities,historical1816/2367
unchanged. Goal active/incomplete.
