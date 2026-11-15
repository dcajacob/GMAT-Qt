SPAD and N-plate STM frame gradients
===================================

The partial stage ``build/nasa-coverage/complete18-20261112-stm-spad-v1``
repairs missing captured Cartesian acceleration gradients for the SPADFile
and NPlate solar-radiation-pressure models. The original SPAD mission
propagated successfully and rejected all 45 owner-dependent frame reads.
That failure receipt remains intact.

The derivative sampler uses temporary spacecraft copies with their own
attitude and hardware. It calculates fourth-order centered differences,
checks successive step sizes and one-sided consistency, and restores the
force's nominal sampling state. A complete gradient is published only after
all six Cartesian columns pass those checks.

Four native cases pass, each with three frames, five Parameter types and
original, clone and copy reads. The total is 180 parameter reads. They cover
a constant-vector SPAD table, an affine angular SPAD table through Bilinear
and Bicubic interpolation, and one SunFacing perfect-specular N-plate mirror.
The tabulated areas and mirror parameters reuse the existing SRP model-choice
fixture values.

The independent oracle checks the propagated Cartesian trajectory and STM,
then checks the coordinate differential using the actual native matrix and
endpoint states. Captured force gradients have an additional independent
numerical bound. Negative controls reject an omitted angular derivative,
corrupted frame output and a wrong mirror-force magnitude.

All four saved spherical-SRP cases replay with matching numerical results.
Those native missions were reused. The existing graphical model-choice
evidence also retains its original source-versioned scope.

The selected attitude is fixed in the inertial frame. State-dependent
attitudes, nonlinear table boundaries, other plate optical/shadow domains,
and the remaining force and frame families still require review. No whole-row
credit is awarded. Sealed coverage remains 1,113 of 1,158, with 12 of the 18
agreed requirements complete.
