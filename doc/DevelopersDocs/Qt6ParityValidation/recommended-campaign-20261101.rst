Recommended acceptance campaign: first recovery increment
==========================================================

The recommended 217-row evidence audit and 37 local acceptance contracts are
active. The first two proof families have 43 individually reviewed requirement
rows: 37 source-versioned scopes recovered and six missing obligations retained.
No historical rows have been recovered in this increment.

The explicit campaign union is now 779/1,158 requirements (67.3%) and
1,803/2,367 historical operations (76.2%). It starts with the sealed two-report
handoff union and adds only the 37 accepted keys below. It does not add the
older broader ledger's numerator, reduce a denominator, infer current-build
qualification or claim formal NASA completeness.

Audited recovery
------------------

The XY family checks twenty rows against the exact frozen PlotSetup, XYAxes and
Plots source assertions, actual independently specified constants/screen
coordinates, source/reference hashes, registered commands, snapshot manifests,
executable identities, unique terminal records and successful JUnit. Both
required unattended and visible cohorts are complete. Seventeen rows recover:

* FRR!E341/E345/E346/E347/E352.
* All Others!E223-E234.

The Orbit/GroundTrack family checks 23 rows against the exact retained
OrbitSetup, PlotSetup, Plots and SharedPlaybackButtons witnesses in both modes.
It recovers twenty rows:

* FRR!E310/E311/E312/E314/E315/E316/E317/E318/E319/E332/E337/E340.
* FRR!E622/E625/E626/E636/E642/E643/E644/E646.

Each decision has a row-specific rationale and remaining limits in
``xy-recovery-plan.json`` or ``plot-recovery-plan.json``. Examples of real result
checks include X=2/Y=3 or7 sample delivery, independent screen positions and
clipping, literal camera eye/reference/offset/scale arithmetic, retained point
counts, recent/completed/replay tail images, exact station longitude30/latitude20,
more than1,000 independently colored custom-map pixels and actual playback timer
advance. Shared native report/converter inputs are named explicitly; they do
not qualify independent orbit/ephemeris/geodetic science.

The old plot review separately recorded one omitted source-hash entry for
``plots.script``. Its wrapper is accepted only after proving that every original
execution/result field is unchanged, the only added hash is the declared
unchanged frozen artifact, and that exact artifact path appears in the actual
registered command. No preserved receipt or snapshot is amended. No fabricated
execution or post-execution current-file hash supplies credit.

Six obligations remain open
----------------------------

* FRR!E342/E343/E344: the mapped resource-dialog proof positively checks scalar
  and user-variable Y values, but cancels X=V and does not establish committed
  scalar/user-variable X selections with corresponding output samples. A
  separate scripted-X fixture is not silently merged into that mapping. Add
  only the missing accepted X-axis selection/result contracts and the explicit
  complete type-family conjunction.
* FRR!E320: spacecraft trajectory color is positively configured and checked;
  the literal all-trajectories row needs the missing celestial-body trajectory
  color delivery/result path.
* FRR!E331/E635: update frequencies3/4 reach the plot models, but the complete
  retained cases do not assert intermediate redraw cadence after the specified
  number of collected points. End-of-run forced refresh is insufficient. Add
  the missing Orbit/GroundTrack cadence witness, preserving the original
  sampling-period-versus-steps-to-skip conflict as a separate obligation.

These are missing proofs, not six newly established product defects. No
assertions were weakened and no configuration-only observation receives the
missing result credit. The candidate audit has 174 not-yet-reviewed rows and
these six incomplete rows remaining; all 37 new local contracts remain pending.

Reusable verifier and checks
-----------------------------

``src/qtgui/tests/AuditAcceptanceRecovery.py`` rechecks retained proof identities
without executing GUI scenarios or changing old artifacts. It requires an
explicit semantic decision and a complete same-cohort row mapping. Legacy
claims use the unchanged NASA legacy assertion/terminal verifier. Structured
claims require one exact non-diagnostic proof, bound source/references and
successful exact-test JUnit. Losslessly compressed logs are read without
retained duplicate files.

``BuildAcceptanceCampaign.py`` keeps all 254 recommended candidate records and
the explicit cumulative accounting in ``campaign.json``. It rejects duplicate,
partial, pending or plan-divergent credits and records old source versions
separately. Neither tool rewrites the older ledger or sealed baseline.

The new ``QtAcceptance.EvidenceRecovery`` support registration passes thirteen
unit contracts, including unsuccessful/missing/duplicate/quoted proof rejection,
source/reference/command/binary/snapshot drift, failed/skipped/wrong-test JUnit,
compressed evidence reuse, immutable supplementary bindings and prevention of
pending/partial/duplicate union credit. The final CTest receipt is
``recovery-contract.xml``. These support checks add no NASA requirement credit.

No existing GUI/backend scenario was rerun. No native screen was opened. Future
new GUI cases retain the requested initially minimized, resizable, non-fullscreen
route. No passing screenshots or runtime/binary snapshots were created.

Changes and next work
----------------------

This increment adds the recovery verifier, campaign builder, their unit contracts
and one CTest registration, plus this note and small audit/planning files. No
GMAT core, GUI production code or existing assertion source is changed. Existing
core/time/model scientific and ABI review obligations remain open.

Continue the remaining named proof-family audits, reusing complete passes and
recording rejected claims. The six missing result contracts are explicit work,
not permission to repeat the whole plot suite. Then implement and verify the
37 approved local requirement contracts from the feasibility review. The goal
remains active; this increment does not satisfy the entire campaign.

Reproduction
--------------

From the repository root::

   python3 src/qtgui/tests/AuditAcceptanceRecovery.py \
     --plan build/nasa-coverage/recommended-campaign-20261101/xy-recovery-plan.json \
     --output build/nasa-coverage/recommended-campaign-20261101/xy-recovery-audit.json
   python3 src/qtgui/tests/AuditAcceptanceRecovery.py \
     --plan build/nasa-coverage/recommended-campaign-20261101/plot-recovery-plan.json \
     --output build/nasa-coverage/recommended-campaign-20261101/plot-recovery-audit.json
   python3 src/qtgui/tests/BuildAcceptanceCampaign.py \
     --output build/nasa-coverage/recommended-campaign-20261101/campaign.json \
     --audit build/nasa-coverage/recommended-campaign-20261101/xy-recovery-audit.json \
     --audit build/nasa-coverage/recommended-campaign-20261101/plot-recovery-audit.json
   ctest --test-dir build/linux-gui -C Release \
     -R '^QtAcceptance[.]EvidenceRecovery$' --output-on-failure

Reproduction validates retained evidence or the new support code; it does not
replay the already passing GUI scenarios. The source-versioned limits remain
unchanged after running these commands.
