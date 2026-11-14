Cartesian owners and rotating frames — 2026-11-03
================================================

Coverage remains **1,019/1,158 (88.0%)**, 139 original requirements open and
**82 more needed** for 1,101. Historical coverage stays 1,816/2,367. Original
fingerprints, assertions, whole-row conjunctions and all 59 prerequisite holds
remain. This checkpoint adds no whole-requirement credit. Technical, scientific,
full-current-build and full NASA qualification gates remain false.

Six scalar types and a related vector
-------------------------------------

X/Y/Z/VX/VY/VZ now have actual native SpacePoint Parameter references to live
Spacecraft owners in main, private function and explicit Global scopes. The
existing native Cartesian vector is a related read-only control. Each type runs
in two naming modes: the default EarthMJ2000Eq frame and explicit ScopeFrame/
SharedFrame references. The private ScopeFrame shadows the caller's same-named
frame and binds private primary/guide clones. The Global frame retains the root
graph. Tests require actual Parameter-to-frame, frame-to-primary/guide, frame-
origin and live SolarSystem identities, plus correct owner flags and references.
No private pointer is dereferenced after its synchronous callback returns.

Targets, primaries and guides stay at A1 epoch 21545. Separate pulse spacecraft
provide the callback boundaries. All reference spacecraft use EarthMJ2000Eq;
the frame origin is Earth and the declared ObjectReferenced axes are X=R,
Y=empty, Z=N. Axis strings come from the raw scripts; the native axes class,
dependency pointers, full rotation and rotation-rate matrices are observed.
Individual X/Y/Z axis-string getters are not directly sampled.

The primary state is [131072,262144,524288,0,0,0]. Root/private/Global guide states
add [0,8192,0,-2,0,0], [0,0,16384,0,-4,0] and [-32768,0,0,0,-8,0] respectively.
Both references are unpropagated Spacecraft with zero relative acceleration and
no bound ODE force provider; the actual relative states and acceleration are
checked. This defines instantaneous kinematic frames with rate 1/4096 per second.
It does not represent integrated orbital evolution of the guides.

Independent transforms and mutations
------------------------------------

For an inertial position r and velocity v, the independent recipe uses
r_frame=B*r and v_frame=B*v+D*r. The three matrices B, mapping from the base frame,
are respectively::

   Root:    [ 0  1  0; -1  0  0; 0  0  1 ]
   Private: [ 0  0  1;  0 -1  0; 1  0  0 ]
   Global:  [-1  0  0;  0 -1  0; 0  0  1 ]

Writing w=1/4096, their derivatives D are::

   Root:    [-w  0  0; 0 -w  0; 0  0  0 ]
   Private: [ 0 -w  0; 0  0 -w; 0  0  0 ]
   Global:  [ 0 -w  0; w  0  0; 0  0  0 ]

The default-frame controls use identity B and zero D. The custom cases retain
nonidentity B and nonzero D in every scope. Native frame-to-base getters must
equal the transposes of B/D, and the direct native frame conversion must agree
with the independently calculated full six-state. Matrix/rate bounds are 1e-12;
state and selected-value bounds are 1e-9. The Python oracle uses elementary list
arithmetic, without GMAT converters. All 168 recipe/transport/inverse combinations
also agree exactly for these binary-fraction inputs.

Root/private/Global target base states are [7000,128,256,1.25,7.5,0.75],
[7500,256,512,2.5,8,1.5] and [8000,512,1024,3.75,8.5,2.25]. Each function call
sets private and Global components from independent base-plus-call formulas.
Scalar cases then add 0.5 to a selected framed position or 0.03125 to a selected
framed velocity through actual script destinations. Related Cartesian vector
cases retain all six component updates without assigning the read-only vector.

Every scalar callback separately changes the native Parameter by 0.25 position
or 0.015625 velocity, requires the actual owner's full inertial state to match
the independently inverted transform, restores the original selected value and
checks all six restored components. Position mutations in rotating frames also
verify the corresponding velocity transport change. Actual owner/frame
references must survive the roundtrip. The complete caller state remains intact
after repeated and nested function calls and the no-Build repeat.

Evidence and unchanged core
----------------------------

**28 current GUI passes** cover fourteen cases in both modes. Four unattended
pilots are reused before ten additional cases; all fourteen replay visibly.
They check 84 missions, 336 calls/callbacks, 672 owner/frame/typed Write/Report
observations, 336 custom dependency graphs, 6,048 components each of rotation
and rotation rate, 4,032 components each of inertial/direct framed state,
576 native scalar mutation/restore roundtrips, 6,912 changed/restored state
components, 432 selected script mutations and 3,024 raw script state-component
updates. Each output channel contains 1,152 expanded values; there are 420 markers.

File-editor keyboard Cancel/Save, source/history/prototype/caller isolation,
Unicode reopen, Build/no-Build repeat and actual report viewing remain. The
visible replay records fourteen actual process maps and 238 exposure events,
stable source/image inventories, initially minimized resizable/nonfullscreen
windows and software rendering. Screenshots, runtime-binary copies and kernel
copies are zero. Twenty-two corrupt-proof controls cover owner/frame/graph
identity, state, rotation/rate, relative acceleration/provider, native mutation/
restore, original writable flags, epoch, repeat and markers.

One build passed. No GUI, compile, offline audit failure or native interruption
occurred. All handles are terminal. There is **no production increment**: all
245 pending core hashes are unchanged from the sealed attitude stage. Native
receipts bind 237 paths, eight non-C++ paths are additionally bound statically,
and visible custody includes all 245. Exact source-version capsules, historical
failures and prior witness fields remain available.

The count audit corrects one metric in the preceding attitude checkpoint: its
strict script assignments total 1,008. The historical receipt/audit retain 1,344;
NestedShared wrapper calls were incorrectly counted as ReadShared leaf-body
executions. Fifty-six passing script invocations each have nine leaf executions
and two assignments per leaf. The exact original report, corrected current
report and proof-based erratum remain bound. The reusable attitude auditor now
counts actual leaf callbacks; its exact preimage remains bound. Native pass outcomes, original
requirement coverage and qualification gates are unchanged.

Legacy Unit strings remain Km for positions and Km/s for velocities and the
Cartesian vector. The vector contains mixed position/velocity dimensions, so its
legacy single label does not establish a mixed-vector unit contract. No unit
reclassification or silent metadata correction is made.

Replay and remaining obligations
--------------------------------

Reusable files under src/qtgui/tests are NasaFunctionCartesianFrameBindingTests.cpp,
NasaFunctionCartesianFrameCases.json, FunctionCartesianBindingOracle.py,
RunFunctionCartesianFrameBindings.py and AuditFunctionCartesianFrameBindings.py.
Both acceptance targets depend on the native executable. All fourteen
registrations retain 180-second timeouts and the engine lock. From the repository
root, use fresh output paths::

   cmake --build build/linux-gui --target GmatQtNasaFunctionCartesianFrameBindingTests -j 6
   python3 src/qtgui/tests/RunFunctionCartesianFrameBindings.py --output build/gui-acceptance/cartesian-frame-replay
   python3 src/qtgui/tests/RunFunctionCartesianFrameBindings.py --visible --output build/gui-acceptance/cartesian-frame-visible-replay

Authoritative audit, cartesian-frame-binding-matrix, worklist, manifest, receipt
and seal are v1 in
build/nasa-coverage/coverage95-20261103-function-cartesian-frame-bindings/.
Unattended sources are headless-v1 for the four pilots and headless-v2 for the
other ten; visible sources are visible-v1. All 566 rows, 321 original types and
prior fields remain. Native owner evidence covers **192 types**; **129 remain**
(121 Spacecraft, eight SpacePoint), including 35 older bounded names. The six
new native bindings already had weaker joined contexts, so any-context counts
remain 227 with a bounded context and 94 without. All 321 whole obligations
remain open.

Non-Spacecraft SpacePoint owners, other origins/epochs/frames, arbitrary
translation, nonzero-acceleration providers, propagated states, singular domains
and mixed-vector units remain open. Preserve earlier attitude/R_SB/derived-
thruster/plate/solver write and science gaps, E491/E477/E406 and failed probes.
The user confirmed no licensed MATLAB installation is available; all 37 MATLAB
holds remain without substitute or waiver. All 59 prerequisite holds remain.
Continue time/orbit/provider and substantive prerequisite work. The full 95%
goal remains active and incomplete.
