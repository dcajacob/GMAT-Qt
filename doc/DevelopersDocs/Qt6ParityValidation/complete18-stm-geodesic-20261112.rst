Earth-centered geodesic STM evidence
===================================

The partial stage ``build/nasa-coverage/complete18-20261112-stm-geodesic-v1``
adds an Earth-centered relativistic mission. All 45 reads pass across three
defined owner-dependent frames, five STM Parameter types and original, clone
and copy instances. The captured acceleration gradient includes all six
Cartesian derivatives.

The independent force model includes Schwarzschild, geodesic and
Lense-Thirring terms. It derives geodesic precession from the saved
heliocentric position and velocity of Earth. Native body orientations, their
rates and the heliocentric ephemeris are shared geometric inputs. The force
equations, complex-step derivatives, variational integration and
high-precision frame differentials are independently implemented.

Removing the geodesic term fails the frozen acceleration check. Its omitted
velocity-derivative contribution also exceeds the frozen gradient bound.
The original negative-control checker expected the gradient failure first.
That checker expectation was corrected after the earlier acceleration check
detected the omission. The original observation remains recorded.

The extended oracle also replays both saved solar cases. Their numeric
results match the saved results. Only the missing Earth mission was executed.
No production source changed, and the original numerical bounds are retained.

This stage earns zero whole-row credit. FRR!E525 still needs the remaining
force-provider, frame, body, domain and augmented-state conjunctions. Sealed
coverage remains 1,113 of 1,158 requirements, with 12 of the 18 agreed
requirements complete.
