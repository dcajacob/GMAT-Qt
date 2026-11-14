Ephemeris publication identity and auxiliary layout
===================================================

This batch repairs packet layout and selection paths used by function-local and same-name spacecraft. It also fixes ordinary multi-spacecraft and multi-propagator auxiliary output: the former declaration and data layouts disagreed, and separate propagator groups wrote at the same offsets.

One native spacecraft order
---------------------------

StateManager exposes a nonvirtual GetPublishedSpacecraft inventory containing actual objects and their declaration roles in native state-map order. Actual identity deduplicates repeated state entries; display names do not merge distinct resources. Formation members use their native reference order and Add roles, with shape/type checks, rather than indexing an unrelated owner-name array or searching current instance names.

Covariance/acceleration declarations retain the existing public order: all 21-element lower-triangular covariance blocks in a group, followed by all three-element acceleration blocks. Data filling now follows that order. Attitude declarations and four-element quaternion data use the same spacecraft inventory. The previous writer interleaved covariance and acceleration per spacecraft despite declaring separate blocks. Covariance values preserve the native Pt0 policy and existing FullSTM-sized storage; no new STM multiplication, covariance transform or force calculation was added. Acceleration and quaternion calls use the same native state epochs as before. Shape/storage checks replace out-of-bounds access for incomplete native data.

Common propagation data filling now validates registered slices and advances covariance/acceleration and attitude offsets separately for every group. It no longer sends every PSM to the same start index. The estimator uses this same ordinal data filling instead of first-name state lookup and repeated group overwrites. Its registration order and state-offset behavior are preserved.

Frozen spacecraft columns
--------------------------

Publisher CartesianColumns now carries optional covariance (21), acceleration (3), quaternion (4), native group number, PropSetup display name and weak definition identity, alongside the six Cartesian indices. Optional all-zero arrays mean absent fields. Publisher validates complete optional blocks, packet bounds and disjoint indices across the advertised object columns. Synchronous contexts include the actual publishing command; the existing packet-keyed getter remains, with a provider-keyed getter for SetProvider callbacks. Compact accepted-step scopes restore the entire original provider/column context without replacing the integration/origin context.

PropagationEnabledCommand builds all column families from the registered native group layout. PropSetup identities use the command's actual definition from its owning scope when available, with its native runtime PropSetup as fallback. Command-local clones of one definition therefore do not by themselves signal a different propagator. The builder validates Cartesian component/Formation ordinal roles and complete registered state/auxiliary extents.

Propagate ordinary packets and accepted integration endpoints use this shared layout. Accepted endpoints still read their native/J2000-body state snapshots; this refactor changes index binding rather than integration math. Common propagation-enabled publications carry columns as well. A new nonvirtual Publisher wrapper combines frozen columns with existing precise/sample publication, and the estimator uses it without changing its output time-cell limit, force derivative sampling or live-state preservation checks.

Ephemeris consumption
----------------------

EphemerisFile matches its bound spacecraft against packet object identity, then selects Cartesian, covariance and quaternion indices from that record. Metadata-present absence is authoritative. It cannot substitute a different spacecraft with the same spelling. Legacy name-only providers keep a name path using the actual bound object's current name. Required Cartesian/quaternion indices are checked against the packet length, optional missing covariance retains the existing zero default, and debug covariance printing also checks missing indices. Native PSM publications continue providing all four quaternion fields, as the preexisting retrieval policy requires.

Propagator-change callbacks select the matching spacecraft's native group and compare weak definition identities as well as display names. Same-name distinct definitions can trigger a segment transition; successive commands cloning one definition keep the same identity. Native run-state/feasibility, flush and allowMultipleSegments policies remain in place. Legacy providers retain the former declaration-name path. New weak identity/identified fields start cold in copies, clear with initialization/resource reset and swap with the existing writer transaction state.

Estimator final-pass SPK sampling now gathers targets from each ephemeris subscriber's actual bound spacecraft and enumerates actual native publication spacecraft, including Formation members. Same-name distinct targets are counted separately; several writers for one actual spacecraft still share one sample. The existing one-microsecond encoded time-cell condition and detached force evaluation remain unchanged.

Compilation and limits
-----------------------

Configured compilation v1: 313/313 steps, 296.41 seconds, return 0.

Ten source paths have preimages taken before their first edits/v1, before/build/final hashes, indexed deltas, a source map and compiler terminal receipts. Final source matches the final successful build. All compiler tool sessions are terminal; added-line whitespace is clean. Configured core, Qt/console, plugins, MATLAB and Python consumers were rebuilt as required. No changed translation unit is outside the configured build.

StateManager/Publisher/command helpers are nonvirtual. CartesianColumns and its exported compact scope have changed layouts; EphemerisFile gains one weak identity and one Boolean field. No virtual slots were added. Matching configured consumers were rebuilt; external prebuilt binary compatibility is unqualified.

This establishes implementation and compilation, not runtime correctness or acceptance. Independent propagation groups can still require per-object epoch/frame semantics beyond the native shared packet epoch; this batch does not claim that broader timing/frame behavior is complete. Public configuration edits, dependency replacement, shared-global lifetimes, remaining resource/command/callable/estimator providers and all original ephemeris-format/scientific requirements remain open. Narrow output fixes do not earn partial all/any parent credit.

No runtime/GUI tests, missions, numerical matrices, acceptance readers/audits, evidence admission, fixture/assertion/tolerance/denominator changes, packages, installed/shared runtime changes, commits, pushes or external messages were performed. Original texts, fingerprints, dispositions and acceptance gates remain intact. Tracking-v67 remains 1093/1158 (94.39%), with 65 other rows; recorded source-versioned verified baseline remains 1053/1158 (90.9%), with all 105 original acceptance rows open. Full goal active and incomplete.
