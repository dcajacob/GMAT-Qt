Owner-dependent Cartesian STM frames
=====================================

This bounded batch replaces the blanket rejection of owner-dependent
ObjectReferenced STM frames for defined R/N and V/N state maps. It captures
point-mass acceleration gradients at the retained initial endpoint and the
current endpoint, differentiates the full six-variable Cartesian frame map,
and transports the native STM with J(t) Phi J(t0)^-1. Both axes and axes-rate
dependence on the measured spacecraft are included. The private calculation
uses extended precision and leaves live spacecraft, guide and frame state
unchanged. Three existing production files and one new private header change;
public APIs and native object layouts are unchanged.

The demonstrated behavior
-------------------------

An owner-dependent frame is not necessarily singular. Two offset radial/normal
frames and an offset velocity/normal frame have invertible six-dimensional
state maps. All three formerly raised the blanket unsupported-Jacobian error.
The centered radial/normal frame is genuinely singular: its orbit-normal
position coordinate is identically zero. It now raises an explicit singularity
diagnostic without changing any cached matrix or spacecraft state.

The frozen numerical oracle uses 80-digit Decimal scalar frame kinematics and
five-point numerical derivatives, independently of the production automatic
differentiation. An independent circular variational flow supplies the native
STM. Bounds remain 2e-8, 2e-6, 2e-11 and 2e-8 for the four blocks, 2e-5 km for
position and 2e-8 km/s for velocity. Expectations and fixtures were frozen
before native execution; no bounds were relaxed.

The original 60 failed reads remain failed in before-v1. After repair, all
60 reads pass: 45 defined matrix/partition reads and 15 atomic mathematical
singularity rejections. Every case runs through the original Parameter, Clone
and Copy providers. The actual owner and guide states and native STMs must
independently agree with their distinct point-mass circular solutions.

Two additional native function routes exercise the new frame map through an
input coordinate system and a function-created coordinate system. Each has two
direct calls and one nested call, with six live samples and 30 native Parameter
reads. The private owner and guide advance from 60 to 90 seconds while the
caller remains at 60 seconds. Both guide STMs, real frame/input bindings,
readonly contracts, scalar returns and 1,008 reported matrix entries pass the
independent oracles. This also verifies that the preceding propagation-state
association repair remains effective for these new function cases.

The 90-read saved reference/clock/clone/copy case is the one dependent check
rerun because its shared snapshot implementation changed. It passes. Previously
passing core owner-frame results are reused across the function build because
the production library hashes are identical. Older unrelated GUI, solver and
base-alignment results keep their executed source versions. No full regression
or whole-current-build qualification is claimed.

Evidence and remaining scope
----------------------------

The stage is build/nasa-coverage/complete18-20261112-stm-owner-frames-v1. It retains
source capsules, loaded image hashes, raw native failures and reports, frozen
fixtures, build receipts and independent reader results. All 1,334 protected
original inputs remain unchanged. Eleven corrupted evidence controls are
rejected without running native code. A reader-only strengthening adds explicit
finite/shape checks for retained native state and STM values; the recorded
native results replay identically and no native test is repeated for that edit.

The earlier unexecuted oracle authoring draft is retained separately; its
Decimal boolean conversion was corrected before the frozen numerical fixtures
and native execution. It is not a native passing or failing case.

The implementation currently supports exact point-mass acceleration-gradient
providers and FK5 MJ2000Eq internal state charts with ObjectReferenced R/N or
V/N axes. Other force-gradient providers, other owner-dependent frame families,
broader body/domain/history cases and the extended-STM conjunction remain open.
An unsupported-provider rejection is not credited as mathematical singularity.
Native axis sign/permutation handling exists but is not separately qualified by
this bounded fixture.

This stage extends engineering receipt
``f1864af9d4fadce42bed651c5f0976d5a28bd167ca4ed402f55315c35a1757a7``.
The canonical prefix remains
``de77757b896aa68c65df2462c026b58bf11ed684b502148fa5a14c0593111574``.
FRR!E525 stays open. Coverage remains 1,106/1,158, with 5 of the agreed 18
complete. This batch earns no partial or whole-row credit.
