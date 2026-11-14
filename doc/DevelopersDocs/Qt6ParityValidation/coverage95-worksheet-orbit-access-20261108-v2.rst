Worksheet orbit parameter access: two repairs
============================================

The existing OrbitParameterAccess compatibility policy now applies correctly
through the Qt spacecraft editor. Worksheet mode makes RMAG and SemilatusRectum
calculation destinations read-only; Native mode retains writable destinations.
The policy remains an explicit configuration choice. No default-policy change,
new parameter formula or public signature/layout change is introduced here.

A native GUI witness demonstrated that choosing Worksheet and pressing Apply
left the configured spacecraft Native and the script unchanged. ResourceEditor
classified the access policy in the Orbit section, then replaced every changed
Orbit field with its cached state reconstruction. That reconstruction contains
state selectors, epoch and Cartesian state, but not the access policy. The repair
keeps OrbitParameterAccess independent of cached orbital state replacement.

With that repaired, typed function outputs still accepted the two read-only
calculation parameters. The shared function-call assembly path checked static
parameter metadata, which does not express the per-spacecraft access policy.
Interpreter.cpp now rejects a function output whose known spacecraft owner uses
Worksheet and whose calculation type is RMAG or SemilatusRectum. Unresolved
function-local owners keep their established resolution path; runtime owner
binding and write guards remain responsible there. Broad function-scope approval
is not claimed. Native-mode function outputs remain supported and are exercised.

Verification
------------

Two new WorksheetOrbitAccess cases derive the unchanged original NasaReadonly
witness. They exercise Resource Tree spacecraft editors, keyboard policy selection,
pending isolation, discard/close/reopen, Apply, exact Undo/Redo, actual destination
and function-output pickers, typed Apply rejection/recovery, report selection,
Unicode reopen and the report viewer. Both configured and previously uninstantiated
parameter references are checked. Original test assertions and fixtures are intact.

Read arithmetic uses two literal Cartesian states with synthetic Mu400000.
Independent position norm or squared angular-momentum norm divided by Mu checks
24 retained scalar values across the two profiles. Four Native-mode missions
return16000 through Echo(15999). The28 reported values comprise4 parameter
values and24 Cartesian components. Each parameter must equal16000 and agree
with an independent norm/cross-product calculation using its reported Cartesian
state. The24 Cartesian components are retained and required finite; they are not
individually compared against independently predicted Cartesian targets. This
checks compatibility via actual function outputs, without a componentwise native
setter geometry or broad scientific claim.

Both new cases and the unchanged Cartesian/Keplerian orbit-definition regressions
pass headlessly and visibly:8 selected passes. The new cases require16 typed
Apply rejections,16 picker rejections and12 picker cancels across both profiles.
The visible nested, resizable desktop requested an initially minimized window,
recorded32 native window exposures and retained0screenshots. Qt input events do
not establish native OS input or cross-platform qualification.

Seven failed predecessor cases are retained. New-driver refinements corrected
discarded-panel reuse and reacquired the replacement panel after Apply. The
isolated pre-repair mode receipt explicitly records requested Worksheet,
configured Native and source_changed=false. The subsequent output failures retain
the improperly committed destination and both Worksheet policy declarations.
The final tests preserve the rejection contract, inputs and numerical bounds.

The final profiles have identical1186 source and111 binary identity entries.
The source capsule freezes1189 distinct non-ELF source versions across all failed
and final contexts; the compiled observer is identity-only. No runtime images are
copied. All1334 protected original inputs match. Scoped whitespace checks pass.

Evidence is in
``build/nasa-coverage/coverage95-20261108-worksheet-orbit-access``.
The component audit independently checks raw calculations, policy receipts,
rejections, exact source versions, protected inputs and current runtime identities.
The publication and read-only verifier bind its artifacts. Reuse completed runs;
whole original-row admission is a separate review, not another automatic replay.

Acceptance remains1076/1158 (92.9%),82open,25needed for the unchanged95percent
threshold. This component alone adds0rows. FRR!E519/E520 retain their original
scope/fingerprints until exact-row admission; their broader body/frame parents
and all full-build, scientific, platform and full-NASA gates remain open.

Report qualification clarification
----------------------------------

This version supersedes the earlier report's wording about the28 Native-mode
values. The independent checks apply to the4 derived parameter values; all24
Cartesian values supply retained finite inputs. The raw runs, assertions, component
audit and original publication remain unchanged. No native rerun or new acceptance
credit is needed for this reporting correction.
