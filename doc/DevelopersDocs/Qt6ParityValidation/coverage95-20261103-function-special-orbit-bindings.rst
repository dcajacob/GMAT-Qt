Special orbital Parameter owner bindings — 2026-11-03
====================================================

Coverage remains **1,019/1,158 (88.0%)**,139 original requirements open and
**82 more needed** for1,101. Historical coverage remains1,816/2,367. Original
denominators,fingerprints,assertions,whole-row conjunctions and all59 holds remain.
This checkpoint adds no whole-requirement credit. Scientific,technical,
full-current-build and full NASA qualification gates remain false.

38 types and their actual dependencies
--------------------------------------

All seven Planetodetic types,Altitude,twelve incoming/outgoing asymptote types,
fourteen Brouwer short/long types and four B-plane types now have live native
main/private/Global Spacecraft references. The complete321-entry catalog and
original writable/reportable/plottable/owner metadata remain. All28 advertised
writable scalars add strict script/native idempotent probes. Five read-only
six-vectors retain Array bridges for Report. Actual legacy unit labels match
their unmanaged factory prototypes; broad unit contracts remain unqualified.

Callbacks require actual Parameter-to-Spacecraft references,private/root
isolation,Global flags,live Earth origin,SolarSystem and coordinate references.
Altitude exposes its actual owner's coordinate reference; its Parameter coordinate
reference is not claimed. Other types expose the Parameter's actual coordinate
reference. Private pointers are inspected only synchronously while executing.
Targets stay at A1 epoch21545; separate pulse spacecraft supply callback boundaries.
Both epoch stores,independent values,raw Write/Report and full Cartesian state
controls are checked across two direct calls,a nested call and Build/no-Build runs.

Geometry and explicit model limits
----------------------------------

Planetodetic/Altitude use an explicitly configured oblate Earth fixture:
equatorial radius6378.1363 km,flattening0.1 and actual Mu398600.4415. Their main-
created SpecialFixed frame has BodyFixed axes. Root/private/Global Planetodetic
inputs [radius,longitude,geodetic latitude,speed,azimuth,radial horizontal FPA] are
[7300,35,42,7.2,63,12],[8500,125,47,7.5,122,-8] and[9000,215,61,6.8,238,17].
Private calls add25 km radius,2 degrees longitude and3 degrees azimuth per call;
Global calls add50 km,4 degrees and6 degrees. Independent spheroid-normal geometry
solves altitude from the specified radius exactly. Velocity retains the shipped
radial/north/east convention; an ellipsoid-horizon convention is not inferred.

The observer measures the actual frame-to-base rotation R and derivative D,
checks the direct native reference state against the literal Cartesian input,
and independently checks internal state with r_base=R*r and
v_base=R*v+D*r. BodyFixed matrices must be orthogonal and rotating. These measured
R/D values are inputs to the transport calculation,not independent Earth-
orientation truth. Actual body radius/flattening/Mu and references are required.
The6,048 components each of BodyFixed rotation and rate do not qualify FK5
or other physical budgets.

Asymptote/B-plane fixtures use inclined hyperbolas with root/private/Global
elements [-14000,1.4,30,40,50,60],[-15000,1.5,40,50,60,70] and
[-16000,1.6,50,60,70,80]. Private/global calls respectively increase |a| by100/200 km
and true anomaly by5/10 degrees. Independent perifocal limiting position rays
define the incoming/outgoing asymptote fields. The incoming position ray opposes
the incoming velocity direction S used by B-plane geometry; this convention is
explicit and does not resolve the earlier interoperability/source-intent gap.
Impact vector B=(S cross H)/v_infinity,T=(Sy,-Sx,0)/its magnitude and R=S cross T
independently determine BdotT/BdotR/magnitude/angle. Signed output ranges remain.

Brouwer inputs use retained NASA BROLYD_short.m/BROLYD.m scalar formulas through
the existing restricted Python adapter,then independent Kepler/Cartesian geometry.
Root/private/Global mean elements are[10800,.17,47,112,38,143],
[11900,.21,56,134,49,152] and[12600,.23,69,154,59,164]. Private/global calls add
40/80 km semimajor axis and3/6 degrees mean anomaly per call. Native inverse
Parameter fields must recover these inputs. This is source-formula parity,
not independent physical averaging or licensed MATLAB execution. Earlier critical-
inclination/prototype/model discrepancies remain open. No MATLAB hold is resolved.

Selected value bounds are1e-6; Cartesian bounds are1e-6 km position and1e-9 km/s
velocity. These new-family bounds match their retained reference recipes; older
assertions remain unchanged. Idempotent write probes preserve actual full state
and owner/frame references. They do not establish non-idempotent setter physics,
arbitrary frame/origin,singular,provider or propagation-science domains.

Results,failures and custody
----------------------------

**132 current GUI passes**,66 unattended and66 visible,cover38 read routes and28
strict write variants in each mode. Nine passing unattended pilots are reused;
57 additional/corrected cases complete that mode. All66 replay visibly and pass.
They check396 missions,1,584 calls/callbacks,3,168 owner/frame/value observations,
1,344 native idempotent probes,1,008 strict script assignments,14,256 function
Cartesian component assignments and1,980 markers. Each Write/Report channel has
4,368 expanded values. Script counts use actual ReadShared leaf bodies,excluding
NestedShared wrapper calls. There are672 BodyFixed owner observations and240
six-vector observations. All456 type/scope/call recipes also pass finite-value
and B-plane geometry controls without a native conversion API.

One original pilot failure remains exact source-versioned evidence: the new
BVectorAngle recipe incorrectly wrapped a principal angle to0–360. Native
-14.67810052896485 degrees appeared as345.3218994710351 in that fixture. The shipped
guide explicitly specifies[-180,180]. Only the new oracle's wrapping is corrected;
impact geometry and original NASA assertions remain unchanged. Failed native
receipt,diagnostics,old oracle/catalog,guide hash and source capsule remain.

One initial test compile failed because CoordinateSystem::FromBaseSystem returns
dynamic Rvector while the new serializer required Rvector6. Exact failed source
and log remain. Accepting the existing Rvector interface with a strict six-value
width check fixes the helper; the second build passes. There is no production increment or native interruption. One offline audit guard
failed because its inherited generic Global statement omitted SpecialFixed; the
corrected guard requires the exact per-case declaration. Failed verifier/log,
native-inventoried preimage and final offline verifier remain bound. Native
assertions and numerical bounds stay exact. All handles are terminal.
All245 prior core hashes remain unchanged;237 are native-receipt bound,eight more
are static,and the visible source inventory binds all245.

66 actual visible process maps and1,122 exposures bind loaded image hashes.
Inputs remain stable; windows start minimized and stay resizable/nonfullscreen,
with software rendering. Screenshots,runtime-binary copies and kernel copies are
zero. Eighteen corruption controls reject owner/value/state/frame/shape/transport/
rate/mean/impact/Mu/SolarSystem/flags/units/no-Build/marker changes. File-editor
keyboard Cancel/Save,Unicode reopen,source/history/prototype/caller isolation and
the actual read-only report viewer remain. No native OS-input/GPU/platform claim.

The matrix retains all566 rows,321 types and prior fields. Native owner evidence
covers298 types;23 Spacecraft types remain queued,all without joined bounded
contexts. The26 older weak-context names in this batch now have native bindings;
twelve previously unjoined names gain contexts. Any-context totals are298/23.
All321 whole obligations stay open; no factory or readable role is excluded.

Authoritative v1 audit,special-orbit matrix,worklist,evidence manifest,final source
identities/archive,stage receipt and seal verification are under
``build/nasa-coverage/coverage95-20261103-function-special-orbit-bindings/``.
Exact failed/passing/visible source archives,prototype text,proofs,raw outputs,
JUnit,traces and logs remain. The prior conic receipt and canonical ledger remain
hash-bound and unchanged. The verifier's post-headless description edit is text
only; earlier executed verifier bytes stay frozen.

Replay new output directories without overwriting historical evidence::

   cmake --build build/linux-gui --target GmatQtNasaFunctionSpecialOrbitBindingTests -j 6
   python3 src/qtgui/tests/RunFunctionSpecialOrbitBindings.py --output build/gui-acceptance/special-orbit-replay-headless
   python3 src/qtgui/tests/RunFunctionSpecialOrbitBindings.py --visible --output build/gui-acceptance/special-orbit-replay-visible
   python3 src/qtgui/tests/AuditFunctionSpecialOrbitBindings.py --stage build/nasa-coverage/coverage95-20261103-function-special-orbit-bindings --output /tmp/special-orbit-evidence-audit.json
   python3 src/qtgui/tests/TrackCoreChanges.py --verify build/nasa-coverage/core-change-review/20261103-function-conic-parameter-bindings

All66 CTest registrations use qt-gui,180-second timeout and engine lock;both
acceptance build targets depend on the driver. The wrapper defaults to all66,
zero screenshots and initially minimized visible runs. Continue the23 remaining
element-rate,Sun/body,OrbitTime,covariance,acceleration,density,torque and
SurfaceHeight routes,then the broader conjunctions. Preserve E491/E477/E406,
earlier failures and source/science conflicts. MATLAB is unavailable per user;
37 MATLAB holds remain without substitute/waiver,and all59 holds remain. With
those holds unresolved,the ceiling is1,099/1,158,two short of the target. The95%
goal remains active and incomplete.
