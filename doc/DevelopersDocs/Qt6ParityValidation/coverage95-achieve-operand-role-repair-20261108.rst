Achieve operand role repair, 2026-11-08
======================================

Achieve now keeps equal numeric literals independent in its measured goal,
desired-value and tolerance roles. Updating a numerical setting through the
native API no longer mutates a different field spelled with the same number.
Explicit named-resource sharing and the existing AllowConstantGoal policy remain.
Canonical acceptance remains 1078/1158 (93.1%); this stage adds no whole-row credit.

The ten-case baseline passed three and failed seven. Six cases directly showed
role aliasing, including actual native Target post-run API updates. Changing
desired value 1 to 2 changed tolerance 1 to 2; changing tolerance to 0.25 changed
desired value to 0.25. With an opted-in constant measured goal, all three fields
changed together. Named Variable sharing, rebinding cleanup and cold copies
already passed. The remaining baseline failure was the new clone-failure
transaction check, since the old setter installed the incoming wrapper directly.

SetElementWrapper now stages separate clones for numeric roles and publishes
only after the complete binding and ownership staging succeeds. It checks clone
identity/type, retains actual resource references for nonnumeric wrappers, and
preserves old bindings plus incoming caller ownership if Clone fails. Previous
bindings and accepted numeric templates retire by pointer identity. ClearWrappers
keeps that retirement queue until the command deletes it with the active bindings.
Only Achieve.cpp changes runtime behavior; no public data fields or virtual slots
change, and no physics, solver equation, assertion or tolerance changes.

All ten repaired cases pass. The unchanged eight-case constant-goal policy
witness passes, including default/explicit-false reference policy, finite
fractional/scientific opt-in goals, rejected overflow and inconsistent goals.
Three existing GUI checks pass in each profile: NasaAchieveParameter,
AchieveConstantGoal and ParameterSelection. Their actual edits, history/files,
native solver results and original assertions remain intact. Workers and actual
runtime dependencies were rebuilt. Two initial new-witness compilation attempts
used hidden string-label overloads; using the public parameter IDs corrected
the witness before baseline execution. All handles are terminal, and all 1334
protected original inputs remain unchanged.

The full original FRC-5 context E90-E97 and all 88 FRR-38 rows E501-E588 remain
retained. The earlier complete type map was rechecked against current factory
creators, class inheritance and Evaluate declarations: 106 distinct names,
97 real-valued names backed by 92 concrete classes. Its 74 scalar leaf instances,
8 headings, 2 matrix rows and 4 Gregorian string rows retain their original
fingerprints. The matrix/string rows stay in the requirements inventory; no
scalar reduction or string-comparison semantics are invented. This current
source-route review is distinct from scientific/domain/any-body/frame proof.
The source interface has no family-name whitelist or writable restriction on a
measured scalar goal. Complete GUI/type/domain/scientific conjunctions remain open.

The help page now explains numeric role independence and corrects desired-value
types that previously advertised String/whole Array. This documentation-only
change follows execution; the exact executed 25-source archive is retained
separately from current source. XML parsing validates the updated help page.
No reexecution is represented as having occurred after that help-only edit.

Stage: build/nasa-coverage/coverage95-20261108-achieve-control-family-current-review.
No denominator, original fixture, assertion, tolerance, original fingerprint,
formal hold, scientific/platform gate or full current-build qualification changes.
Continue the finite audit autonomously with the constraint/objective spelling
group; current source already offers both spellings, so inspect that actual route
and its existing witnesses before assuming a missing implementation.
