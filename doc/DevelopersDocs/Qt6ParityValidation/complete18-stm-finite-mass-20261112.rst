Mass-depleting finite-burn derivatives
=====================================

Evidence is stored in
``build/nasa-coverage/complete18-20261112-stm-finite-mass-v1``.

A 100 N chemical burn with constant 300 s specific impulse exercises fuel
depletion during the 60-second propagation. The independent model uses
``dm/dt = -T/(Isp*g0)`` and evaluates acceleration and its six Cartesian
partials with the corresponding mass at every integration stage.

The native endpoint consumes 2.03873598369455 kg of fuel. Its final total mass
differs from the independent prediction by less than 5e-12 kg. The case passes
45 live full/partitioned STM reads, 30 post-propagation reads and 432 native
Report entries through the powered and coast phases. Omitting mass depletion
exceeds the original position-error bound in the independent sensitivity check.

Three saved constant-mass cases pass replay with the extended oracle. This
stage introduces supplementary test coverage and makes no production changes.
Original numerical bounds and protected test inputs remain unchanged.

This case covers constant-Isp mass depletion and the Cartesian STM block.
The broader force, frame and augmented-state conjunction remains open.
Canonical coverage stays at 1,113/1,158, with 12/18 agreed rows complete.
