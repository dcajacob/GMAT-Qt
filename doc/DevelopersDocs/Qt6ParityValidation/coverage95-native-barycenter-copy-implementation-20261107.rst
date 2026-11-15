Native Solar System barycenter copying
======================================

The built-in SolarSystemBarycenter now has a standard Qt Clone route and an explicit Copy calculated point action. Its detached user-defined copy retains a serialized native ephemeris-source descriptor, with state, velocity, acceleration and mass delegated to the same SolarSystem special point. Ordinary barycenters retain mass-weighted calculation. Native and Qt libration-point restrictions now recognize this source descriptor under renamed aliases. Default celestial-body copying and the full any-object Clone requirement remain incomplete; no original-row or verification credit is added.

Owning-source findings and implementation
------------------------------------------

Owning-source inspection confirmed that the original built-in Barycenter uses SolarSystem::GetSpecialPoint while an ordinary user-defined Barycenter computes mass-weighted states. Clearing the protection flag alone would select a different calculation. Barycenter now exposes an enumerated EphemerisSource of MassWeighted or SolarSystemBarycenter, using its existing builtInType descriptor independently of isBuiltIn protection. No new instance fields or virtual slots are added. The new parameter ID extends this class metadata, and its existing virtual parameter interfaces are overridden to expose type, choices, getters, setters and serialization behavior.

UsesSolarSystemEphemeris identifies both the protected original and user-defined native-source aliases. GetNativePoint re-resolves the special point from the current SolarSystem for each state, acceleration or mass query, with explicit errors for missing or unknown providers. Real/A1 and GmatTime state routes delegate to that same point and publish epoch caches after a successful state result. Position and velocity continue through the state route. The existing native acceleration and mass paths use the same selected provider. Barycenter assignment no longer borrows builtInSP from another value. Ordinary mass-weighted equations and native special-point ephemeris calculations are unchanged.

EphemerisSource is configurable before BeginMissionSequence and cannot be changed by command-mode property assignment. The protected built-in point keeps its source. Native-source BodyNames is read-only/inactive, is omitted by the native serializer, returns no component list and supplies no unused body dependencies to ObjectInitializer. Both scalar and indexed BodyNames setters and prerequisite replacement reject native-mode mutation. Source changes stage descriptor/default-list values before publication and invalidate readiness. Returning to MassWeighted supplies the documented Earth/Luna defaults when no body list/defaults exist. Native-source queries bind through SolarSystem rather than an approximation over configured planetary bodies.

The Resource Tree guide excludes default frames, default bodies and propagators from standard Clone, but does not exclude the built-in SSB. Standard Clone now permits that Barycenter. resourceCloneDraft makes only its detached value user-defined, and cloneResourceSource seeds a complete explicit native snapshot because the built-in resource has no authored Create statement. The renamed declaration and EphemerisSource survive the existing candidate-script route, including authored overrides, pending settings and cancel/rollback behavior. An additional Copy calculated point action is wired through the Edit and built-in-point context menus; user-defined calculated points use the same editor. Native-source selection is applied before BodyNames during pending GUI settings so list writability follows the chosen mode.

LibrationPoint formerly rejected SolarSystemBarycenter by resource name. Its native reference validation and evaluation-time body check now also reject a Barycenter with the native source descriptor. The Qt picker and validation use the same descriptor, so a renamed copy does not bypass the original scientific restriction. Generic BodyNames list handling follows the native read-only policy. The Barycenter and Resource Tree help document both source modes, copy behavior and native-list restrictions. Existing test sources, the original workbook and extracted requirement text remain unchanged and have not been executed.

Limits
------

These are implementation and compilation results, with no native/model/parser/GUI/function/mission/numerical runtime verification. Default celestial-body copies still require independent resource/provider identity, DE target rebinding and body-specific Earth/Neptune behavior; simply switching user-defined flags remains insufficient. The broad any-object Clone/Resource Tree rows also retain include/array and other original scope. Function/provider graph, external data lifetime, scientific coordinate/body behavior, Save and the rest of the campaign remain open. No whole original requirement is claimed complete, all 105 original acceptance rows remain open, and the full implementation audit is incomplete.

Compilation and retained evidence
---------------------------------

Build v1 returned 0 in 22.86s, progress 78/78, 0 existing compiler deprecation warnings. All compiler sessions are terminal. Final hashes match build v1. 8 existing source/help preimages, 0 absent-new-path records, 19 unchanged reference files and four planning snapshots preceded edits. Indexed/combined patches, hashes, source findings and compiler receipts are retained. New text passes whitespace checks (0 inherited whitespace lines recognized by the diff). Participating help XML files parse.

No new tests, native/model/helper/parser/GUI/MATLAB/mission/numerical/runtime execution, acceptance auditors/readers/seals or evidence admission occurred. Original requirements/text/fingerprints/fixtures/assertions/tolerances/denominator/gates/dispositions remain unchanged. No row or verification credit. Tracking-v146 retains 1093/1158 (94.39%), 65 unresolved engineering rows. Recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full implementation audit and goal remain incomplete.

Next implementation
-------------------

Complete the remaining original Resource Tree copy inventory, especially default celestial bodies with DE/SPICE target identity, SolarSystem binding and Earth/Neptune-specific orientation/property semantics preserved independently of the new resource name. Continue mixed configuration/mission includes, included array initializers and the original function/provider graph and any-Resource Save work. Preserve original requirements, fixtures, assertions, tolerances, gates and dispositions; runtime verification remains deferred.
