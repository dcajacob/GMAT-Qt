Combined SRP stress-check error budget
=====================================

The stage ``build/nasa-coverage/complete18-20261112-stm-srp-error-budget-v1``
reuses the sealed native SRP run and investigates its retained combined
flow-and-frame stress failure. It launches no native tests.

The independent flow calculation uses a 63-bit-mantissa long double and
complex-step force derivatives. Two RK4 step sizes check refinement. The
coordinate differentials and the error decomposition use 80-digit Decimal
arithmetic.

The largest native Cartesian position difference is 7.51e-10 km. The native
Cartesian STM also meets every original block tolerance. The VNB frame's
large sensitivity amplifies these accepted input differences. Its B-block
combined error is 4.60e-4 seconds. The actual-input conversion error is
1.61e-6 seconds, within its original 2e-6-second bound.

An exact three-term matrix decomposition separates changes in the final
coordinate Jacobian, the Cartesian STM and the initial inverse Jacobian.
The resulting componentwise upper bounds contain the observed combined
error. Both RTN cases also meet the original combined bounds.

The original combined test applied a frame-conversion tolerance without
propagating the uncertainty already permitted in its input trajectory and
STM. Its failed status and original receipt remain intact. The final row
audit can use the independently verified dynamics and actual-input frame
conversion as separate obligations. This disposition supplies no replacement
full-flow pass and no additional requirement credit.

FRR!E525 remains open for its other force, frame, body and domain obligations.
The sealed coverage is still 1,113 of 1,158, with 12 of 18 agreed requirements
complete.
