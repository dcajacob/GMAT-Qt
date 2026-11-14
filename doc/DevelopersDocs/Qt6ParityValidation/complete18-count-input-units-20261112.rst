Integer count and order descriptors
==================================

Thirty-three saved integer-input records lacked explicit unit metadata.
A private GUI descriptor now identifies iterations, iteration numbers, attempts,
function evaluations, line searches, interpolation order, samples, retained
points, recent trajectory segments, stars, formatting characters and digits.
Existing nonempty units take precedence. Native values, limits, setters and
numerical behavior are unchanged. Identifiers, elastic weights, thinning modes
and conditional tolerances are deliberately outside this increment.

Many plot captions already named samples, points or segments; this change
aligns their metadata with those captions. Other generic inputs now display
their count descriptor alongside the value. The recent-segment meaning agrees
with the existing renderer's retained preceding endpoint and count limit.

The two targeted groups pass 66 headless/visible observations. Every original
missing-metadata observation is retained. The initial count-limit fixture also
failed because its EphemerisFile lacked the required spacecraft reference;
that development failure remains separate. The corrected fixture keeps every
unit expectation unchanged. The supplementary reader now handles spin boxes
as well as line inputs and preserves case-sensitive unit matching for existing
fixtures; only explicitly declared caption nouns use case-insensitive matching.

The frozen reader checks guide count definitions, sample/segment and formatting
consumers, exact source changes, build/runtime identities, raw GUI observations,
native exposure and source/history/file/native-state isolation. Negative
controls reject substituting a physical time unit or dropping a count field.
No mission or full regression ran.

The saved numeric-unit queue drops from 66 to 33 records. The queue is not a
complete current-input inventory or a defect count. All Others E13 remains open;
canonical acceptance stays at 1,111/1,158 and ten of eighteen agreed rows.

Evidence is sealed under
``build/nasa-coverage/complete18-20261112-count-input-units-v1``.
