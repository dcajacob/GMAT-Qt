Returned ephemeris caller bindings, 2026-11-05
==============================================

This implementation batch repairs direct caller correspondences for ephemeris
resource returns. FRR!E406/E450/E491 remain partial or under source-scope review.
There is no new row or acceptance credit: tracking remains 1087/1158 (93.87%),
recorded verification remains 1053/1158 (90.9%), all 105 original acceptance rows
remain open, and 71 original rows remain partial/unclassified/dependent. The
full implementation pass is unfinished and verification remains deferred.

Formal-to-caller reference binding
-----------------------------------

An ephemeris can refer to a formal spacecraft, coordinate system or ODE model
inside a function. Copying the ephemeris to a caller previously preserved that
formal name/pointer even when the dependency had a known caller counterpart.
Its configured name then did not identify the caller's resource, and its raw
pointer could remain in the callee's store.

FunctionManager now resolves these three direct reference families against
actual formal input/output correspondences before assigning the ephemeris value.
The formal map entry must have the exact dependency pointer; a matching name
alone is insufficient. A whole-resource output target takes precedence over an
input counterpart. For example, when a formal spacecraft is both input and output,
``[newSat, newEphem] = F(oldSat)`` binds the returned ephemeris to newSat.
Otherwise a direct formal input maps back to its existing caller resource.
Non-resource destinations do not create whole-resource output correspondences.

The mapped target must have the expected type. The detached ephemeris value
receives both the caller reference name and the caller pointer through its
existing virtual RenameRefObject/SetRefObject routes. Failed typed binding is
reported. True global dependencies retain their native shared identity. A true
global ephemeris returned to itself retains its identity operation rather than
being replaced by a detached copy that would retire its active writer.

The acceleration-model rename branch already existed in EphemerisFile, but its
entry type filter excluded ODE_MODEL, making the branch unreachable. The filter
now admits that type. This also repairs ordinary ODE-model reference renaming.

Named, unnamed and nested outputs
-----------------------------------

Named ephemeris results are cloned/prepared before any caller assignment can
overwrite a source value in an aliased store. ObjectWrapper then supplies that
prepared value through the existing ElementWrapper assignment path. Prepared
values stay owned by the invocation until return handling finishes.

For nested returns, a temporary source-map view overlays the prepared values
on the callee's map. PopFromStack therefore captures and copies the translated
ephemeris after restoring the caller frame; it cannot overwrite the earlier
binding with the unprepared source value. This is a borrowed view of objects,
not a second owning object store. Actual pointer targets remain the caller's
resource identities across frame value restoration.

Unnamed local object-expression snapshots use the same binding helper before
the snapshot is retained. True global unnamed results keep the preceding native
borrowed path. No caller/global object is renamed or made owned by this helper.
Writer-side mutation during trials still follows the existing suppression and
entry-restoration implementation.

The parent ephemeris caches a spacecraft ID during reference binding, but its
writer obtains the ID from the actual bound spacecraft again when created and
initialized. Thus this route does not require creating a writer or publishing
a packet while related output values are being assigned.

Remaining lifetime work
------------------------

This supplies known direct correspondences; it does not own arbitrary private
dependency graphs. A private spacecraft/frame/model that has no matching formal
input/output remains unresolved. Related values copied under different internal
names do not establish pointer identity by value equality. Owned hardware/axes,
transitive frame/model references, global resources mutated without being
returned, recursive graph ownership and function-definition replacement still
need implementation. Do not infer private ownership from spelling or silently
turn those cases into globals. FRR!E406's internal-recording/final-pass policy
review also remains open; native RunInitialGuess/terminal replay is preserved.

Build and retained artifacts
------------------------------

Stage: ``build/nasa-coverage/coverage95-20261105-implementation-function-return-bindings``.
Build v1 passes 90 steps in 190.18 seconds (session 52594 completed). Final
executable source hashes match that build, and no build session remains live. Source preimages, exact
at-build patch/hashes, build log/receipt and final patch/hash/summary are retained.
FunctionManager's protected helper signatures change; its instance layout and
public signatures remain unchanged. All dependents are rebuilt together.

Only compilation and essential source debugging were performed. No test, GUI,
script/function/model/solver execution, numerical comparison, acceptance reader/
seal or qualification run. Test/probe clients are linked only. No original
fixture/assertion/tolerance/text/fingerprint/denominator/ledger/gate changed. No
installed/shared runtime, package, commit or push update. Windows/macOS execution
is unqualified.

Later verification must cover all three reference families, different formal/
caller names, input/output overlap, both output orders, global identity, wrong
types, unnamed expressions, nested/recursive returns, trial/final passes and
remaining graph lifetimes. Finish the rest of the implementation before those
checks.
