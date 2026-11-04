Time Parameter owners and consistent epoch writes — 2026-11-03
=============================================================

Coverage remains **1,019/1,158 (88.0%)**, 139 original requirements open and
**82 more needed** for 1,101. Historical coverage stays 1,816/2,367. The original
denominators, fingerprints, numerical assertions, whole-row conjunctions and all
59 prerequisite holds remain. This checkpoint adds no whole-requirement credit.
Scientific, technical, full-current-build and full NASA qualification remain false.

Thirteen selected clock and elapsed-time routes
----------------------------------------------

Ten additional types now have actual live Parameter-to-Spacecraft references
in main, private function and explicit Global scopes: A1Gregorian, TAIModJulian,
TAIGregorian, TTModJulian, TTGregorian, TDBModJulian, TDBGregorian, UTCModJulian,
ElapsedDays and ElapsedSecs. A1ModJulian, CurrA1MJD and UTCGregorian are related
previously bound controls. Original owner metadata stays SpacePoint for ten
clock types and Spacecraft for CurrA1MJD and the two elapsed types. Clock types
remain writable;
elapsed types remain read-only. Legacy day/Day/s/empty string unit labels remain.
Non-Spacecraft SpacePoint domains are not qualified by the actual Spacecraft used.

The root/private/Global input days are respectively 21545/21546/21547, expressed
in each selected time system. Private and Global function statements add 9 and
18 seconds per leaf call. Gregorian inputs are whole selected-system seconds
near noon on 1–3 January 2000, away from civil and leap boundaries. The independent
Python recipe uses A1–TAI=0.0343817 seconds, TT–TAI=32.184 seconds, TAI–UTC=32
seconds and the bounded TDB periodic terms 0.001658*sin(g)+0.00001385*sin(2*g),
where g=(357.5277233+35999.05034*(day-21545)/36525) degrees. It imports no GMAT
converter. This is a controlled local clock recipe, not general time-scale science.

Actual Parameter values, native time getters, raw Write and Report values must
agree with the selected-system recipe. Both GetEpoch() and GetEpochGT().GetMjd()
must agree with the independently calculated A1 epoch within 2e-10 days.
Gregorian strings compare exactly; ElapsedSecs has a 2e-5-second value bound.
The elapsed controls sample the actual initial-epoch value and its set flag.
Private initial epoch resets with its function owner; Global initial epoch stays
21547 while its elapsed values progress through 18/36/54 seconds. Private elapsed
values are 9/18/27 seconds. Separate pulse spacecraft provide callback boundaries;
target Cartesian states remain exact. Private pointers are accessed only inside
their synchronous live function callback, never after returning.

Every writable callback separately changes the native Parameter by two seconds,
checks both epoch stores, restores the selected value and checks both restored
clocks, initial epoch, owner identity and full Cartesian state. CurrA1MJD also
factory-creates a temporary native Formation around the live owner and a peer
clone, supplies the live SolarSystem/J2000Body context, initializes it and changes
the internal integer epoch property by four seconds. Parent, member and peer
must all change and restore both clock representations with states intact. This
temporary parent is outside the configured namespace; it is neither a Global
parent binding nor a Formation script-edit qualification. The script epoch stays
read-only. The native reader pointer/internal filename is not directly observed.

The native leap-table record resolves FileManager's configured LEAP_SECS_FILE,
hashes its canonical application/data/time/tai-utc.dat path and checks the live
converter's offset at 21545 is 32. The visible before/after hashes are identical.
Only the table identity and hash are preserved; its content is not copied into
source archives. This checks the configured path and live offset without claiming
direct observation of the reader's private filename or broad leap-boundary science.

Retained failures and one existing production increment
-------------------------------------------------------

The original unattended pilot selection failed in all five cases:
A1Gregorian, TAIModJulian, TDBGregorian, ElapsedSecs and CurrA1MJD. Native writes
changed the Real epoch while GmatTime stayed stale. The elapsed case exposed the
same mismatch after a script epoch statement; Formation parent/member/peer all
retained stale GmatTime after an internal epoch write. Exact failed sources,
diagnostics, logs and failed receipts stay in headless-v1; no failure is relabelled
as a passing negative control. There is no passing proof.json for those failures.

TimeData's real and string setters call the owner's integer A1Epoch property.
SpaceObject::SetRealParameter(EPOCH_PARAM,value) previously wrote only Real state.
The repair calls the existing state.SetEpochGT(GmatTime(value)) before retaining
the existing Real write. Existing high-precision and Spacecraft string-label
setter routes already synchronize both stores. Formation's internal epoch setter
delegates to SpaceObject and its members, so it benefits from the same repair.
No conversion formula, member, virtual slot, owner role or writable flag changes.
The source's final newline is normalized; prior PlotOriginScope changes remain.

All 245 pending core paths remain inventoried: 244 prior hashes are unchanged and
SpaceObject.cpp is the sole increment to an existing pending path. No new core
path is added. Current core source, HEAD source and patches remain preserved.
The failed capsule holds the exact pre-repair SpaceObject bytes; the passing and
visible capsules hold the repair. Test assertions, catalog and oracle stay exact
across this repair. Native source receipts bind 237 core paths; eight more are
statically bound, and the visible source inventory binds all 245.

One initial test compile failed because the staging helper converted literal C++
newline escapes into physical newlines inside quoted strings. The exact failed
source and log remain; preserving raw escapes fixes the test source. Two later
builds pass, including the production relink. One offline custody authoring guard
confused execution_counts with per-result statuses; its failure and correction
remain recorded. A first offline seal also incorrectly assumed CurrA1MJD had
SpacePoint metadata; its original owner is Spacecraft101, unlike the other ten
clock types. The native assertions already used exact original metadata. The
failed verifier/log and report preimage remain; only the offline verifier and this
explanation are corrected. No native process was interrupted. All handles are terminal.

Evidence, custody and remaining work
------------------------------------

**26 current GUI passes**, thirteen unattended and thirteen visible, cover 78
missions, 312 calls/callbacks, 624 owner/dual-clock observations, 384 numeric and
240 Gregorian observations, 96 elapsed initial-epoch observations, 528 native
clock mutation/restore roundtrips and 48 native Formation roundtrips. Each output
channel has 624 typed values; there are 390 markers. There are 396 selected clock
script assignments and 72 auxiliary elapsed-case epoch assignments. These totals
use the nine ReadShared leaf-body executions per case invocation; function_calls
also includes three NestedShared wrapper calls and cannot be used as a leaf count.

File-editor keyboard Cancel/Save, Unicode reopen, source/history/prototype/caller
isolation, Build/no-Build repeat and actual report viewing remain. Thirteen actual
visible process maps and 221 exposure events bind the loaded application/library
and test-image hashes. Inputs stay stable; windows start minimized and remain
resizable/nonfullscreen, with software rendering. Screenshots, runtime-binary
copies and kernel copies are zero. Twenty-three corrupt-proof controls reject
owner/flags/value/direct-time/dual-clock/initial-epoch/Formation/state/metadata/
no-Build/marker corruption. Original native failures remain separate.

The matrix preserves all 566 rows, all 321 system Parameter types and every
earlier field, adding two witnesses per selected type and the five retained
failure links. Native owner evidence now covers 202 types. The queue has 119,
all Spacecraft, including 34 with older bounded contexts. Any-context counts
become 236/85: nine of these new types previously lacked any joined context;
ElapsedSecs already had a weaker context. All 321 whole obligations remain open.
No factory entry or readable role is excluded.

Authoritative v1 artifacts are under
``build/nasa-coverage/coverage95-20261103-function-time-parameter-bindings/``:
``evidence-audit``, ``time-parameter-binding-matrix``, remaining worklist,
``evidence-inputs``, final source identities/archive, ``stage-receipt`` and
``seal-verification``. Exact failed/passing/visible source archives and raw native
evidence remain. The prior Cartesian receipt and canonical coverage ledger are
hash-bound and unchanged; the earlier attitude count correction remains intact.

Replay a new invocation without overwriting historical outputs::

   cmake --build build/linux-gui --target GmatQtNasaFunctionTimeParameterBindingTests -j 6
   python3 src/qtgui/tests/RunFunctionTimeParameterBindings.py --output build/gui-acceptance/function-time-replay-headless
   python3 src/qtgui/tests/RunFunctionTimeParameterBindings.py --visible --output build/gui-acceptance/function-time-replay-visible
   python3 src/qtgui/tests/TrackCoreChanges.py --verify build/nasa-coverage/core-change-review/20261103-function-time-parameter-bindings
   python3 src/qtgui/tests/AuditFunctionTimeParameterBindings.py --stage build/nasa-coverage/coverage95-20261103-function-time-parameter-bindings --output /tmp/time-evidence-audit.json

Thirteen reusable CTest registrations use the qt-gui label, 180-second timeout
and engine lock, and both acceptance targets depend on the driver. The wrapper
defaults to all thirteen, zero screenshots, bounded checkpoints and initially
minimized visible windows. Replaying the original failure capsule against the
current library would not reproduce the original pre-repair image; retain that
source/image distinction.

Continue orbit/provider and prerequisite work. Arbitrary epochs, time scales,
leap/civil boundaries, propagation science, non-Spacecraft SpacePoint and Formation
GUI editing remain open, together with earlier Cartesian/frame/attitude/R_SB/
derived-thruster/plate/solver gaps, E491/E477/E406 and source/science conflicts.
The user confirmed no licensed MATLAB is available: the question is resolved,
37 MATLAB holds remain without substitute or waiver, and all 59 holds remain.
The full 95% campaign stays active and incomplete.
