Current velocity-frame attitude verification, 2026-11-08
========================================================

The recorded zero-rate failure is no longer present in the current source.
The unchanged original OrbitVelocity test passes: its velocity-aligned frame
reports 0.061765286477324 deg/s at the initial epoch, matching the independent
0.061765286477323995 deg/s reference. All four epochs pass the original bounds.
Maximum angular-rate error is 2.43e-16 deg/s; maximum matrix error for this case
is 6.96e-11, below the unchanged 2e-10 orientation bound.

Existing implementation
-------------------------

ObjectReferencedAxes already obtains relative acceleration from the ODEModel
binding when computing frame derivatives. SetupSpacecraftData registers actual
spacecraft identities; Propagate activates the executing model. The provider
checks matching state origins/axes, accepted epochs, ownership, availability and
recursive queries before obtaining force derivatives. Spacecraft destruction
removes its binding. CSFixed uses the frame rotation and its derivative, together
with the configured constant offset. No new acceleration approximation, source
patch or convention change was needed for this case.

Executed evidence
-------------------

Stage: ``build/nasa-coverage/coverage95-20261108-attitude-velocity-rate-current-review``.
The first current OrbitVelocity diagnostic passed. Only the other six original
branches then received headless execution; all seven received visible execution:
Inertial, Ecliptic, Translated, BodySpin, BodySweep, OrbitRadial and OrbitVelocity.
All fourteen registered executions pass without changing either original worker
or fixture. Across those executions, 56 raw report rows contain 504 orientation,
168 rate and 336 orbital components, including repeated headless/visible values.

The original checks exercise model/reference/sequence popups, pending model
switches, inactive legacy initial controls, discard, Apply, exact source history,
Unicode reopen, native/report agreement and read-only report viewing. The orbital
oracle uses literal circular point-mass equations; body orientation uses independent
geometric construction with native time conversion as a shared input. No broader
astronomical truth is inferred. Visible runs record 182 native exposure events
and zero screenshots; native OS input qualification remains open.

Source/binary guards match their executed versions. A verified archive retains
2,341 source versions, and all 1,334 protected original hashes match. The stale
failure is cleared only for this original four-epoch case. Neither the historical
failed artifacts nor an immutable canonical acceptance record was rewritten.

Remaining scope
-----------------

These default-alignment cases do not qualify arbitrary nonzero-offset editor
configuration, every frame/body/input domain, the complete FRR!E50 model family,
or the whole FRR!E52 any-coordinate-system clause. Other original sample-reference
and intermittent report-activation failures remain open. No qualification gate
or requirement credit changed: 1,078/1,158 (93.1%), with 23 more needed for 95%.

The next concrete source gap is in the dedicated atmosphere editor's Mars-GRAM
input-file and density-model controls. Its provider/scientific prerequisites stay
separate from that GUI implementation work.
