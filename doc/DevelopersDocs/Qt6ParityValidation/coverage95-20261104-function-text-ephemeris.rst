STK and CCSDS-OEM file clocks and interpolation
===============================================

Coverage remains **1,050/1,158 (90.7%)**:108 exact rows open and51 more needed
for the unchanged1,101 target. Historical operations remain1,816/2,367. All59
original hold identities survive,30 resolved and29 unresolved,including seven
MATLAB rows. No whole row is admitted. Scientific,technical core,full current
build and NASA qualification remain false.

The original STK and CCSDS-OEM owned-type tests now pass unattended and visibly.
The collective owned-type witness set is35 of36;Topocentric remains. These
witnesses preserve the full566-role resource universe and321 Parameter-owner
obligations. E491 still requires the complete E492-E500 resource/eligible Global
conjunction,including Variable/String/Array aliases and actual nested GMF use.
No constructor flag,finite type matrix or narrower owner canary replaces it.

Repairs and limits
------------------

Both text readers previously reduced file timestamps to Real MJD before
interpolation. They now retain GmatTime node clocks. The common interpolator
uses seconds relative to a nearby node,with separate Real and precise query
entry points. Real callers retain representable endpoint identity;precise
queries outside the actual encoded span are rejected. Legacy records populated
only through their public Real field still use that clock. Changing that public
field invalidates a stale precise companion. Exact-node and single-point
queries return the recorded state.

STK now uses its documented Lagrange policy,including the existing warning and
fallback for Hermite headers. Its inherited default previously selected Hermite
even for Lagrange files. Scenario clocks are built from midnight and original
clock fields,avoiding Gregorian seconds-of-day subtraction that discarded
fractional bits. Record offsets and segment-boundary decisions remain in seconds.
Duplicate checks distinguish precise clocks that share a rounded MJD,ignore
identical interior repeats and preserve separate segment boundary records.
Published Real endpoints come from the same node clocks.

CCSDS-OEM data readers pass precise parsed clocks through their segment storage
and both ephemeris-loading paths. The established calendar/leap validation and
legacy Real ParseEpoch interface remain. Deep segment copies preserve the new
clock fields. Identical duplicates return before allocation;conflicting duplicates
remain errors. Six/nine-value Cartesian records keep their six state components.
The existing OEM Lagrange-only reader policy remains enforced;Hermite input is
explicitly rejected by the support contract.

An order-one stencil defect became visible after retaining query precision.
Just after an interval midpoint,the closest-node selection could put both nodes
after the query. The old caller then silently returned a zero state. Odd-order
stencils now bracket the query,and an unsuccessful interpolation throws. Existing
order reduction and its warning remain. Generic base Hermite interpolation is
checked independently with cubic basis functions and correctly scaled derivatives
in seconds,including method switches at the same order.

The STK/OEM propagators pass GmatTime queries through their initialization,step
and UpdateState interpolation calls instead of dropping to Real. The original
GUI scenarios intentionally retain their default1e-6 s stop tolerance and actual
nonprecision flags. Their bounds and fixtures are unchanged. **Separate precise-
clock propagator GUI controls remain to be run**:the support contract directly
exercises the reader APIs,and does not establish those full propagator/frame/
StartEpoch combinations. Do not present the compiled GT branches as GUI-qualified.
STK header interpolation-degree handling is unchanged and remains unqualified;
this matrix does not prove general format,provider,UTC history or frame accuracy.

Eleven newly pending core paths are preserved:Ephemeris.cpp/.hpp,STKEphemerisFile.cpp,
CCSDSEMSegment.cpp/.hpp,CCSDSOEMSegment.cpp/.hpp,CCSDSOEMReader.cpp,
CCSDSEphemerisFile.cpp and StkEPropagator.cpp/CcsdsEphPropagator.cpp. The pending
inventory is278;all267 previous core paths are unchanged. Exact incremental
pre/postimages,patch and complete HEAD-relative inventory remain in the stage.
EphemPoint and protected EpochAndData records gain clock/validity fields and
change layout;dependent binary consumers must rebuild. Current EphemPoint is72
bytes on this host and GmatTime32. Outer owning-class data layouts and virtual
slots are unchanged;the new overloads are nonvirtual. No eligibility flag changes.

Evidence
--------

The final selected default GUI witnesses are source5;affected regressions are
source6. Four new contexts plus34 affected original contexts pass:38 total.
New tests supply12 missions,48 actual callbacks and288 independently checked
state components. All19 selected visible contexts start in minimized,resizable,
nonfullscreen nested desktops;239 native exposures. Original43 require calls,
the entire32-case catalog,1e-7 km/1e-10 km/s bounds and1e-6 s stop bound survive.
Of1,067 earlier test/helper paths,1,065 are unchanged;only runner source references
and descriptive core inventory text change. No old test assertion changes.

Registered QtAcceptance.TextEphemerisEpoch runs its native recorder followed by
independent60-digit Decimal arithmetic. Its30 regular STK/OEM files cover linear/
cubic states,2/9/17-node files,zero/nanosecond fractions,calendar/DOY input clocks,
STK Lagrange and documented Hermite-header fallback,and both query directions.
The1,860 primary queries use Real,GmatTime and rounded-GmatTime APIs:1,734 valid
states and126 outside rejections. Additional controls cover legacy/precise base
records,segment method switches,gaps,single nodes,public epoch mutation,deep OEM
copies,identical/conflicting duplicates and the2016 inserted UTC second. Two
additional STK/OEM files have10 ns-spaced nodes sharing one rounded MJD. In total,
12,474 state-component checks pass the original bounds. This is finite arithmetic
coverage;no broader scientific qualification follows. Eleven plausibly damaged
function/clock proofs are independently rejected.

Eight source capsules and build receipts survive:six passing builds and two
failed builds. Source1 missed an overload declaration;source2 retained an
incomplete method replacement. Neither authorized native execution. The first
support failure retained an invalid OEM Hermite positive-case assumption and
fractional STK endpoint evidence;the second retained the zero-state stencil
failure. Later controls add exact endpoint identity and additional independent
checks without relaxing physical bounds. No GUI failure occurred in this stage;
the original STK/OEM failures remain in the preceding sealed checkpoints.

Final registration is source8,with all13 selected binary images byte-identical
to source7. The aggregate acceptance target depends on the reader-only support
target independently of the optional ephemeris plugin's Code500 target. All74
recorded native identities and two visible servers were observed terminal.
The prior sealed Code500 audit replays exactly,including its frozen owned-prefix
contracts and explicit legacy temporal-link correction. This seal again separates
immutable regular artifact bytes from symbolic-link route metadata;historical
loaded-image hashes remain bound to their own source/build/process versions.
No current image substitutes for a historical binary hash.

Remaining work and replay
-------------------------

The requirement audit preserves every earlier field,fingerprint,gap,conjunction
and qualification gate across all108 open rows and59 holds. The566-row matrix
only appends bounded STK/OEM witnesses. Continue separate precise-clock propagator
GUI controls,Topocentric's exact fresh-build prototype drift,and then the complete
resource/eligible Global join. Preserve all other open scientific,baseline/source-
conflict,MATLAB,C-interface,platform and hardware bundles. This checkpoint does
not redefine the95percent objective around the completed witnesses.

Stage:``build/nasa-coverage/coverage95-20261104-function-text-ephemeris``.
Final receipt:``stage-receipt-v2.json``. Seal:``evidence-manifest-v1.json``.
Final audit:``evidence-audit-v1.json``. Documentation and offline seal preparation
are explicit post-execution review additions;runtime execution keeps its own
source versions. From the repository root,choose a fresh output path::

  python3 src/qtgui/tests/AuditFunctionTextEphemeris.py \
    --repo /home/dan/GIT/GMAT-Qt \
    --manifest build/nasa-coverage/coverage95-20261104-function-text-ephemeris/evidence-manifest-v1.json \
    --output /tmp/gmat-text-ephemeris-evidence-new.json

The auditor can be extracted from the final reviewed source archive and run
from another directory with explicit --repo/--stage. It verifies immutable
artifacts,source capsules,actual maps,the frozen prior audit,original GUI
contracts,independent arithmetic,damaged-proof rejection and exact core images.
It never executes GMAT or awards requirement credit. For the reusable registered
native-plus-oracle contract::

  cmake --build build/linux-gui --target GmatTextEphemerisEpochTests
  ctest --test-dir build/linux-gui -R '^QtAcceptance.TextEphemerisEpoch$' --output-on-failure

Use a fresh source/build capsule and output directory for later affected GUI
runs. Reuse all unaffected source-versioned passes;retain initially minimized,
resizable,nonfullscreen visible execution and every failure.
