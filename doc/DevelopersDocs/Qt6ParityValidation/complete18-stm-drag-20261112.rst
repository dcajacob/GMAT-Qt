Drag owner-frame STM evidence
=============================

The targeted stage is stored in
``build/nasa-coverage/complete18-20261112-stm-drag-v1``.

The numerical mission uses spherical drag and a synthetic constant-density
Exponential atmosphere. Its propagated state and STM passed the independent
comparison before the repair, while all 45 owner-frame parameter reads rejected
because DragForce supplied no captured Cartesian acceleration gradient.

DragForce now supplies that gradient when explicitly requested. Sampling uses
an independent spacecraft clone, resets all six components between samples,
checks successive fourth-order differences and restores the nominal atmosphere
query after either success or failure. Exact zero relative speed returns the
zero gradient. Ordinary force evaluations retain their existing entry path.

The repaired mission passes all 45 original/clone/copy reads across three
spacecraft-dependent frames and five public STM parameter types. An independent
Cartesian integration checks both spacecraft states and native STMs. Eighteen
force-gradient entries are compared with complex-step derivatives, which were
separately checked against 80-digit arithmetic. The zero-relative-speed control
also checks that its query leaves the live spacecraft unchanged.

Native atmosphere angular velocity is an explicit shared environmental input.
The synthetic density table, force law and variational integration are checked
independently. Empirical atmosphere, wind, SPAD drag and broader force/frame
combinations remain open. This stage contributes evidence to FRR!E525 and adds
zero whole-row credit. Canonical coverage remains 1,113/1,158, with 12/18 agreed
rows complete. Original failures and protected tests are retained. Two saved
N-plate eclipse cases pass offline compatibility replay.
