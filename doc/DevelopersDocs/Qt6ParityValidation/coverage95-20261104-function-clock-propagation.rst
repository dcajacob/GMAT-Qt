Precise propagator start-clock selection
========================================

Coverage remains **1,050/1,158 (90.7%)**:108 exact open rows and51 more needed
for the unchanged1,101 target. Historical operations remain1,816/2,367. All59
original hold identities survive:30 resolved,29 unresolved,including seven
MATLAB rows. No whole row is admitted. Scientific,technical,full current-build
and NASA qualification remain false.

This increment supplies the missing full GUI start-clock controls for Code500,
STK and CCSDS-OEM. It supplements the35/36 source-versioned owned-type witnesses;
Topocentric and the complete E492-E500 resource/eligible Global conjunction still
remain before E491 credit. The full566-role matrix,321 Parameter-owner obligations
and every other exact open scope are preserved.

Repairs
-------

The first nine new precise controls passed FromSpacecraft but failed EphemStart
and explicit starts in all three formats,both unattended and visibly. The common
base converted the file boundary back through a Real MJD,and converted explicit
starts through its Real API. Loading a text-file span could also replace an
already parsed explicit start with the file start. These defects are independent
of the earlier interpolation repairs.

EphemerisPropagator now retains the encoded start in one protected GmatTime
member. Construction,typed copying and assignment preserve it. Real-only
providers retain a rounded fallback,and a changed Real boundary invalidates its
old companion. Precise initialization and continuation keep GT clocks,and an
explicit parsed start survives later span discovery. Nonprecision conversion
retains its Real API. The unconditional explicit-start correction is also
exercised by separate Real-clock GUI controls.

The three text/binary propagators bind their decoded first-node clocks into
that cache. Code500's additional derived EphemStart branch uses the retained
clock. Empty-data guards avoid reading a missing first record. A further
fractional-start counterexample exposed a self-invalidating companion:legacy
Code500 header conversion can round one MJD step differently from the decoded
record clock. The propagator now publishes the Real companion from that same
precise clock. The fractional cases fail before this publisher correction and
pass afterwards with byte-identical native test programs and identical trajectory/
clock recipes.

Five exact core increments are preserved:EphemerisPropagator.cpp/.hpp and the
Code500,STK,CCSDS propagator .cpp files. The pending inventory is279 paths:
274 previous paths unchanged,four increments to already pending files,and one
newly pending header. The added GmatTime member changes the common ephemeris-
propagator object layout;dependent classes/binaries require rebuilding. There
are no new virtual slots or script eligibility changes. Exact pre/postimages,
patch and complete HEAD-relative inventory are retained as core-increment-v2
and core-inventory-v2. Earlier v1 artifacts remain as history.

Controls and retained failures
-----------------------------

Fourteen supplemental scenarios produce28 passing GUI contexts:three formats
across precise FromSpacecraft,EphemStart and explicit starts;three Real explicit-
start controls;and fractional Code500 EphemStart in both byte orders. FromSpacecraft
and explicit scenarios start at UTC30 seconds. Fractional starts use the same
UTC0.123456789 epoch through supported precise MJD input. They preserve the
original absolute affine trajectory:timestamps and authored state knots move
together. Generated little/big-endian clock-field recipes are retained;the
oracle does not learn a replacement physical grid from a bad decoder.

New controls assert the actual precision flags,start selector,elapsed duration
and selected UTC start,as well as all original function-scope contracts. The GT
selected-start bound is1 ns;the Real selected-start bound is1 microsecond. The
original position1e-7 km,velocity1e-10 km/s and stopping1e-6 s bounds are unchanged.
A separate1e-7 s command StopTolerance activates GT only in the supplemental
scenarios. Original default-clock fixtures are reused and independently rerun
where affected. The legacy affine_at_requested_stop diagnostic remains a
relative-duration value;qualification uses the recorded UTC file/query epochs
and independent selected-start assertion.

Eight original default contexts and30 affected SPK/start/group/reversal contexts
also pass:66 selected GUI contexts total. All33 selected visible contexts start
minimized,resizable,nonfullscreen;366 native exposures. New scenarios supply
84 missions,336 actual callbacks and2,016 independently checked state components.
All43 original native assertions and three original input-helper assertions
remain textually preserved,in order. Six supplemental native assertions bring
the driver to49. The original32-case catalog is unchanged. Twenty-four original
caller/function script bindings are byte-identical to their frozen predecessors
after replacing only their exact temporary fixture-directory prefix.

The existing registered Code500 interpolation contract is reused at source4:
24 files,1,104 queries,1,040 valid states,64 outside rejections and6,240 strict
components with22 independent60-digit spline systems. Its tested methods,inputs
and class layout are unaffected by the later publisher-only correction. It is
not relabeled as source7. Twelve plausibly damaged scope/clock/spline proofs are
independently rejected.

Seven source capsules and successful build receipts survive. Eighteen GUI
failures remain:12 initial selector failures,two fractional test-input Build
errors from Gregorian precision limits,and four fractional runtime failures
before the publisher correction. The test-input correction changes format to
precise MJD while preserving the physical epoch. A compile-before-write guard
also retained the first offline-auditor update syntax failure without changing
its source;the corrected versioned updater is preserved. Nothing is counted as
a pass by changing the original numerical bounds or whole-row gates.

All395 recorded native identities and five visible servers were observed
terminal. Final selected Code500 GUI execution is source7;STK/OEM and original
non-Code500 defaults are source3;other regressions are source4. Earlier passing
observations and superseded failures remain source-versioned. The complete prior
text-ephemeris audit replays,including its frozen predecessor chain and explicit
temporal-link correction. Immutable regular bytes and symbolic-link routes stay
separate;historical library hashes retain their own actual source/build/process
versions. No runtime or supplied kernel copies are introduced.

Remaining scope and replay
--------------------------

This finite matrix covers J2000/UTC input trajectories,the listed starts and
both Code500 byte orders. It does not qualify arbitrary noninertial frames,
all time-system/fraction/provider combinations,file-end clipping,all spans or
all parameter domains. STK header interpolation-degree handling remains open.
Continue Topocentric's exact fresh-Build prototype drift and the full resource/
eligible Global join. Retain every other original science,source-conflict,
MATLAB,C-interface,console,platform and hardware gap. No restored deprecated
feature or administrative source-conflict waiver earns credit.

The requirement audit retains all earlier fields,fingerprints,gaps,conjunctions
and gates for108 rows and59 holds. The566-row matrix only appends the bounded
clock witnesses. The95percent objective remains unchanged.

Stage:``build/nasa-coverage/coverage95-20261104-function-clock-propagation``.
Final receipt:``stage-receipt-v2.json``. Final requirement/matrix records use v2.
Seal:``evidence-manifest-v1.json``. Final audit:``evidence-audit-v1.json``.
The offline auditor and documents are explicit post-execution review additions;
native execution keeps its recorded versions. Choose a fresh output path::

  python3 src/qtgui/tests/AuditFunctionClockPropagation.py \
    --repo /home/dan/GIT/GMAT-Qt \
    --manifest build/nasa-coverage/coverage95-20261104-function-clock-propagation/evidence-manifest-v1.json \
    --output /tmp/gmat-propagator-clock-audit-new.json

The auditor can be extracted from the final source archive and run from another
directory with explicit --repo/--stage. It replays the actual frozen prior audit,
source archives,maps,clock/scope arithmetic,default-script preservation,failed
counterexamples and exact core images without executing GMAT or granting credit.

Reusable registered scenarios are QtGui.NasaFunctionEphemerisClock*;the generator
PrepareFunctionEphemerisClock.py recreates their six supplemental catalogs from
the preserved canonical32 cases. To list registrations::

  ctest --test-dir build/linux-gui -N -R '^QtGui.NasaFunctionEphemerisClock'

For later new/affected native execution,use a fresh source/build capsule and
output with the existing RunFunctionOwnedTypes.py --filter option. Preserve the
visible initially minimized,resizable profile and every failure;reuse unaffected
source-versioned passes.
