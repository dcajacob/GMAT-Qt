Planet and B-plane parameter sampling repair, 2026-11-08
========================================================

PlanetData and BplaneData now evaluate the same state and timestamp as the
scoped report publication. Outside that scope they use the spacecraft's native
precision policy and its guarded origin-relative input when available. B-plane
frame-conversion errors now propagate instead of returning an unconverted value.
Canonical acceptance remains 1078/1158 (93.1%); no new whole requirement is admitted.

Concrete defects and repair
---------------------------

Both evaluators previously read only the live spacecraft state and a Real epoch.
A published sample with its own precise epoch could therefore be reported using
a different state/time. The first eight-case native witness passed both legacy
controls and failed six precise, published, nested or error cases. Its independent
frame and vector geometry also verified ordinary values; timestamp dispatch is
checked directly. PlanetData's hour-angle model remains an explicit native input,
not an independent astronomy oracle or a new precision model.

The first repair made all eight cases pass. An extended twelve-case witness then
passed those eight and failed four distant-origin cases. These evaluators ignored
the spacecraft's retained local input and lost precision by subtracting a large
absolute origin again. The repair uses the existing native origin/epoch/backing/
frame identity guards, and omits repeated translation only for that valid local
input. A canonical publication always takes priority over the live local cache.
The same twelve-case witness now passes, including both local/publication priority
cases. No body ephemeris, orientation, orbital equation, unit or tolerance changed.

BplaneData used to catch conversion BaseException, log it and continue with its
unconverted state. The native failure case reproduced that behavior. It now
throws to its caller and recovers on the following valid evaluation. No public
class layout or virtual ordering changed. Production changes are limited to
PlanetData.cpp and BplaneData.cpp, plus registrations for the new native witness.

Executed evidence and original scope
-----------------------------------

The final native run passes 12/12 with 149 observation records. It covers all five
PlanetData readers and four B-plane readers, legacy and precise owner clocks,
canonical sample precedence, nested legacy publication/restoration, distant Pluto
origin input, frame failure/recovery, and unchanged live state/epoch/local input.
Its spherical geometry and Rx/Rdot transform are independent calculations. Its
B-plane oracle derives the impact vector from angular momentum and incoming
asymptotic velocity. Prescribed axes are fixtures, not a physical rotation model.
The original 1e-8 distance and 1e-9 angular witness bounds stay unchanged.

Ten unchanged original GUI cases pass in each profile: five planet-related
readers, four B-plane readers, and the parameter selector. They retain their
actual main/local/global owners, independent selected results, source/file/history
checks and report views. Native workers and those actual GUI workers were rebuilt
before execution. The visible route is resizable and initially minimized. All
handles are terminal. One initial new-witness build failed on a hidden native
setter overload; it was corrected before the baseline execution.

The source map retains all 48 original E501-E548 rows and 53 native parameter
spellings. FRR-16 is retained by source position: all 62 rows, including ten whose
original identifier is DEFERRED. No provider is restored or credited by that
wording. The eleven default body names and four user body types remain in scope.
The undefined FR-xxx reference and complete all-body/frame/domain/provider
qualification remain open. Native STM and partitions still return the spacecraft
matrix with NO_DEP; an any-frame STM contract needs specified initial/final frames
and epochs. No transformation semantics were invented, and no full-parent
implementation or scientific qualification is claimed. The known original
OriginRelativeStateContract RealMJD/GmatTime oracle conflict was not changed or
rerun. Every one of the 1334 protected original inputs remains unchanged.

The finite source audit can now move to Vary literal controls. The queued candidate
is same-text aliasing between a mutable numeric control and numeric settings;
it still needs native reproduction. The original full FRC-2 conjunction and GUI
literal-control scope are in the next plan. No user question or approval is pending.
