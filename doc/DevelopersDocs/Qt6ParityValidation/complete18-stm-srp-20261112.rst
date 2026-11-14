Spherical SRP shadow derivatives
===============================

The partial stage ``build/nasa-coverage/complete18-20261112-stm-srp-v1`` adds
analytic shadow-fraction derivatives to the spherical solar-radiation-pressure
model. The native STM assembly and the frame-sampling path use those
derivatives. The original penumbra mission completed propagation but rejected
all 45 frame reads because no SRP acceleration gradient was available.

The four completed cases cover full sunlight, umbra, penumbra and annular
shadow. Each checks three owner-dependent frames, five STM Parameter types
and original, clone and copy instances. All 180 reads pass. The Cartesian
trajectory and STM are independently checked using conical disk-overlap
illumination and variational integration.

The velocity-based frame stress case exposed sensitivity to precision in
the shadow geometry. ShadowState now calculates angular separation with a
stable cross-product/dot-product expression and evaluates disk overlap in
extended precision. Its existing virtual penumbra method remains the entry
point. A scoped geometry context preserves the original vectors during that
call, including recoverable precision from the Sun-spacecraft subtraction.

The frame-conversion check uses the actual propagated Cartesian STM and
endpoint states as inputs to an independent high-precision Cartesian
differential. The dynamics checks retain their separate original bounds.
The earlier combined flow-and-frame stress failures remain recorded and
receive no replacement pass. This stage does not claim an absolute error
bound for that original combined stress result.

Three negative controls reject a corrupted Cartesian STM, corrupted frame
output and omission of the shadow-fraction gradient. The original native
failures, the initial collector compile and lifetime failures, and all
intermediate numerical failures remain in the stage.

No full regression was run. SPAD and N-plate SRP shape models, other force
providers and the remaining frame/body/domain conjunctions still require
coverage. This stage awards zero whole-row credit. Sealed coverage remains
1,113 of 1,158, with 12 of the 18 agreed requirements complete.
