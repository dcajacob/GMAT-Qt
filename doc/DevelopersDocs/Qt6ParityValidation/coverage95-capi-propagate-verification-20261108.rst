C API Propagate verification, 2026-11-08
=======================================

Eight new external C-client cases and two unchanged original C regressions pass.
Verification found and repaired two product defects: prepared, unexecuted models
could expose state without usable derivatives, and rerun initialization could
hang while scanning an active nested branch. Coverage remains1076/1158 (92.9%),
with25 more requirements needed for1101. This bounded component grants no new
whole-row credit; All Others!E252 and its unresolved referenced scope remain open.

Product corrections
--------------------

CInterfacePluginFunctions.cpp previously treated a populated state vector as
sufficient preparation. Native command initialization can allocate that vector
before assembling and binding its ODE model. Lookup now also requires the model's
state manager to match its PropSetup manager. Otherwise it uses the existing
PrepareToPropagate action. Bound, completed/stopped models retain their assigned
states; lookup still does not execute a mission command.

BranchCommand::GetCommandsWithGmatFunctions traversed with GetNext. An active
nested BranchCommand can return itself through that execution link, causing
Sandbox initialization to loop on rerun. A bounded interrupted native debugger
stack identified this path. The structural scan now advances with the existing
GetNextWhileExecuting link. This changes no branch execution rule or function
scope. Both corrections preserve public signatures, object layouts and virtual
slots. The affected native libraries and the new C client build successfully.

Executed checks
----------------

The cases cover prepared nested branches/loops, three setups in one synchronized
command, completed mixed models, executed/untaken branches, backward propagation,
Variable/Array-element stop goals, constant measured operands with finite time
guards, and a stopped mission with later unexecuted models. Each exercises actual
public C load/run and combined load/run, repeated queries, assigned-state
preservation, rerun invalidation, failed-load invalidation and reload recovery.

There are2220 new C assertions and192 independently checked Cartesian scalars,
using literal circular two-body equations with a pinned Earth mu. Bounds remain
2e-5km and3e-8km/s. The client additionally checks Newton accelerations and the
single-spacecraft STM Jacobian, and switches derivative buffers across6,12,42
and84 elements. These are bounded nominal/coherence checks, not general dynamics,
frame, force-provider or arbitrary parameter-domain qualification.

The unchanged original basic/stm C clients pass133 and481 assertions. Their
original fixtures and independent force/orbit/STM expectations are preserved.
All executions explicitly use the private integrated startup and loaded native
libraries, excluding accidental use of older installed CInterface libraries.
Each new run binds1745 source paths and32 runtime images with identical
before/after inventories. All selected production/C sources and binary identities
match. Seven passes reuse the earlier fixture-generator version; the final
generator produces byte-identical inputs/oracles for each of those cases.

Retained failures and limits
----------------------------

Three failed native predecessors remain: the missing state-manager derivative
error, a90s rerun timeout, and the new multi-setup fixture's incorrect dimensions.
The native parser applies STM to the entire command, giving42,42,84 state sizes.
Only that new fixture's expected dimensions changed; its exact script, original
assertions and every numerical bound were preserved. The corrected case passes.
A new component-reader assertion-count assumption was also retained/corrected:
the combined loader path has two fewer CHECK calls than the separate path.
That metadata correction required no native rerun or production change.

All1334 protected original inputs remain unchanged. No GUI executions, screenshots
or runtime-image copies occurred. E249/E251 retain the dynamics/MarsGRAM dependency;
E252 retains its complete FRC-10 configuration obligation, including the original
unlimited-numerical and ephemeris-cardinality questions. All broader scientific,
NASA, full-current-build, physical-frame, platform and external-ABI approvals
remain open. The prior sealed1076 prefix and its hold identities are unchanged.

Evidence is in ``build/nasa-coverage/coverage95-20261108-capi-propagate``:
``component-audit-v1.json``, its replay script, selected raw client logs/receipts,
source capsules, exact product preimages/patches, build receipts and interrupted
native stack. No package, installed runtime, commit or push was updated.

Continue the finite remaining C interface checks: ephemeris-before-numerical
discovery, documented propagation mode/policy configuration and remaining scalar
clock/rejection boundaries. Reuse the completed cases unless a relevant change
requires new execution; do not substitute them for unresolved original scopes.
