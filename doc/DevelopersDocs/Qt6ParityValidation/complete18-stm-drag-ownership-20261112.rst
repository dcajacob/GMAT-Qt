Per-spacecraft drag STM coefficients
====================================

The stage is stored in
``build/nasa-coverage/complete18-20261112-stm-drag-ownership-v1``.

An unequal-spacecraft mission reproduced incorrect derivatives for the second
spacecraft. The Cartesian trajectory used its own properties, while the STM
finite differences used the first spacecraft's ballistic coefficient and
atmosphere density scale. The derivative path now selects both quantities by
the current spacecraft index. The existing public density API is retained.

Two final missions pass the frozen trajectory, STM and frame bounds. One uses
different spacecraft mass, drag coefficient, area and density scale. The other
sets the reference spacecraft's drag coefficient to zero. Together they cover
90 frame reads across the full and partitioned STM parameters, including
original, clone and copy providers. The earlier equal-property drag case passes
an offline compatibility replay.

The zero-drag control exposed a fixture mismatch between the declared gravity
constant and the default gravity file. Final fixtures use an explicit monopole
file with the declared constant, and read the executed constant, degree and
order back from the native field. The failed mismatched and malformed-file
attempts remain available, along with the original derivative failure and the
intermediate unequal-spacecraft pass. Numerical tolerances are unchanged.

This is additional evidence for FRR!E525. Canonical coverage remains
1,113/1,158, with 12/18 agreed rows complete. Empirical atmosphere, wind,
SPAD drag, other forces and the broader frame/body conjunction remain open.
