ThrustAngle owner binding, vector lifetime and native protocols
==============================================================

Canonical coverage remains **1,019/1,158 (88.0%)**, 139 open and **82 more
needed** for 1,101. Historical coverage stays 1,816/2,367. No whole original row
or E491 conjunction is newly qualified. Full 95% goal active/incomplete.

Demonstrated defects and bounded repair
---------------------------------------

The two advertised ThrustAngle Parameter names declare owner type172. The actual
ThrustSegment supports that type but has primary Interface category166. AddRef
previously passed166 into a reference path accepting172; native controls reject
the owner and GUI probes fail live-owner identity. An incremental supported-type
AddRef repair produces native owner-control diagnostic passes, later found to
use empty vectors because of a support-loop bug; these remain unqualified.
GUI probes still expose
rebinding failures and actual corrupted vector components while direct owner
vectors match independent literals.

ThrustAngle1/2 evaluators held GetDataVector pointers into temporary by-value
Rvectors, retained a fixed three-element cache and read component2 regardless of
owner width. Their mutable SetRvector overloads did not override the const
Parameter/ParameterWrapper virtual setter. Their assignment operators also
recursed into themselves. That recursion is source-proven and post-fix tested;
no pre-fix assignment crash or success is fabricated.

Three plugin parameter files now normalize supported ThrustSegment type on AddRef
and SetRef, invalidate cached owners and synchronize existing vector widths.
Evaluators own returned vectors through copying/resizing. Existing const setter
slots are overridden to update actual owner data and the Parameter cache; both
original mutable virtual overloads remain. Assignment delegates to the parent.
No data member, new virtual slot, numerical formula, thrust model, factory
membership or type/display-label mapping changes. The existing third-component
undefined sentinel is retained when that component exists. Empty/undefined and
coefficient-unit semantics remain unqualified; there is no general ABI approval.

Exact prior-working preimages, core-v3-owner-reference.patch and the complete
core-v4-vector-protocol.patch are retained. Core inventory has 241 pending paths:
all previous 238 hashes unchanged plus three newly pending parameter files.
Technical/scientific/full-current-build/NASA approval gates remain false.

Evidence and retained attempts
------------------------------

Six new GUI cases cover both types at lengths 1,3,4, actual native owner references
in main/private/Global contexts, direct owner vectors and command-mode literal
mutations. Each has three missions with keyboard function-file Cancel/Save,
Unicode main reload/Build, no-explicit-Build repeat, actual Write blocks, a
supported Array/Report bridge, caller/prototype/source/history/file isolation and
the actual Output-tree report viewer. No generic Assignment dimension guard is
relaxed. Private maps/pointers are inspected only inside synchronous callbacks.

Twelve new GUI passes plus four unchanged related ThrustSegment/ThrustHistoryFile
regressions give 16 selected GUI passes. New-case totals are 36 missions, 144 calls/
callbacks, 288 Parameter-owner checks, 288 vector Write and Report observations,
768 scalar components in each output channel and 180 markers. Eight visible
process maps bind actual images and stable before/after sources in initially
minimized/resizable/nonfullscreen software-rendered windows, with zero screenshots
or runtime-binary/kernel copies.

Four current support passes are separate from GUI counts: two corrected binding
controls require the intended three-element input, and two full protocols check
six length/type rows,384 repeated reads, six polymorphic setters, wrapper setters,
cross-length resizes and copy/assignment controls. They observe actual owner data,
not only cached Parameter values.

The first offline audit rejected a support arity mismatch. A conditional
initializer_list range in the new harness had expired backing storage: binding
controls actually recorded length0. Earlier full controls recorded1/3/4 but shared
the unsafe loop. Four passed support records remain superseded and unqualified,
with exact sources/statuses preserved. A named owning std::vector fixes the loop;
only the four affected support cases are rebuilt/rerun in headless-v5. Existing
GUI assertion and plugin sources are unchanged, so their passes remain reused.
The failed auditor source/log and pre-correction source archive are retained.
Final audit/support sources are bound separately after the visible source version;
no GUI receipt or before/after map is rewritten.
Eleven corrupt-proof controls reject bad owner/shape/value/no-Build/marker and
native setter/wrapper/assignment/resize evidence.

Eight failures remain: two GUI fixture syntax failures on unsupported ElseIf,
two native owner-binding failures, and four GUI owner-reference failures across
the unchanged and incremental plugin versions. The latter incremental attempts
also retain observed temporary-vector corruption with correct direct owner data.
Three independent If/EndIf blocks correct the fixture without changing intended
values or assertions. No compile failure or interrupted case occurred.

The initial native support registrations were unselected because the runner
inventories qt-gui labels; adding qt-gui alongside support/core labels leaves
source classification as no-widget support. The initial support executable name
also fell outside the runner's GmatQt binary-hash prefix. A post-terminal file
hash is retained; no retroactive receipt hash or actual mapping is claimed for
that baseline. Passing support targets use GmatQt names and receipt binary hashes.
GUI baseline executable hashes and all exact executed sources remain intact.

Matrix and remaining work
-------------------------

All 566 original rows/fields and 321 system Parameter obligations remain. Each
ThrustAngle row gains six witnesses: three lengths in two modes. Owner id172 and
the original UnknownObject display metadata remain historical, while actual
ThrustSegment class/support/reference identities are measured. No display-label
reclassification, factory/readable-role exclusion or denominator waiver occurs.

Live owner evidence now covers 122 types; 199 remain (185 Spacecraft, 14 SpacePoint).
There are 163 types with any joined bounded context and 158 with none; 41 older
bounded names remain in the native queue. All 321 whole obligations remain open.
Continue remaining hardware rotation/derived propulsion/power/plate, attitude,
frame/time/orbit/vector/matrix/string/provider routes. Preserve coefficient
empty/undefined/units, thrust-history force/estimation/polynomial science,
strict-clone/unmodeled-System/strict-time/custom-Star probes, finite function
flags/frame gaps, E491/E477/E406, earlier conflicts and 59 prerequisite holds.
The licensed MATLAB installation-path question remains pending.

Replay and custody
------------------

Build and run the new GUI cases::

  cmake --build build/linux-gui --target GmatQtNasaFunctionThrustAngleParameterBindingTests GmatQtThrustAngleParameterNativeTests -j 2
  python3 src/qtgui/tests/RunFunctionThrustAngleParameterBindings.py
  python3 src/qtgui/tests/RunFunctionThrustAngleParameterBindings.py --visible

Run native full protocols or related regressions::

  python3 src/qtgui/tests/RunFunctionThrustAngleParameterBindings.py --filter '^QtAcceptance.ThrustAngle[12]NativeFull$'
  python3 src/qtgui/tests/RunFunctionThrustAngleParameterBindings.py --filter '^QtGui.NasaFunctionRemainingResource(ThrustSegment|ThrustHistoryFile)$'

Both acceptance targets depend on the new GUI executable and the native target
when the ThrustFile plugin is enabled. Replays create new receipts. Authoritative
artifacts under
build/nasa-coverage/coverage95-20261103-function-thrust-angle-parameter-bindings/
include thrust-angle-parameter-binding-matrix-v1.json,
remaining-parameter-binding-worklist-v1.json, evidence-inputs-v1.json,
evidence-audit-v1.json, final-sources-v1.tar.gz, stage-receipt-v1.json and
seal-verification-v1.json. Only a passed seal verifies final custody. Replay::

  python3 src/qtgui/tests/AuditFunctionThrustAngleParameterBindings.py --stage build/nasa-coverage/coverage95-20261103-function-thrust-angle-parameter-bindings --output /tmp/thrust-angle-audit-replay.json
  python3 src/qtgui/tests/TrackCoreChanges.py --verify build/nasa-coverage/core-change-review/20261103-function-thrust-angle-parameter-bindings
