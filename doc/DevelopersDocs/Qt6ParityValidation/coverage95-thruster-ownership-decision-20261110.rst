Impulsive gravity coefficient: deferred
========================================

The user elected to defer this historical wording issue because impulsive burns
are already supported. Retain the existing implementation. FRR!E80 and its
dependent parent FRR!E77 stay open in the original denominator; no acceptance
credit or product change is assigned. No definition question remains pending.
The decision is recorded in decision-v1.json beside the proposal below.

Original FRR!E80, under the FRR!E77 Thruster-properties parent, reads:
"Gravity coefficient to decrement mass used for an impulsive maneuver".
Its impulse wording and its placement imply different native object owners.
The bounded worklist explicitly retains this as a specification dependency.

GMAT already exposes GravitationalAccel on both objects. ImpulsiveBurn uses its
coefficient in the rocket equation for an instantaneous maneuver. Thruster uses
its own coefficient for finite mass flow. The separate Impulsive Burn gravity
row, FRR!E99, is already source-versioned accepted. No coupling between these
two independent resource objects is implied by that evidence.

The proposed interpretation is to fulfil the impulsive mass-loss obligation
through the existing ImpulsiveBurn.GravitationalAccel control, retaining the
Thruster coefficient's finite-flow role. This would require a definition decision
before credit could be assigned to FRR!E80. The proposal is retained history
and is not active work.

Five existing passed frame workflows from the frozen full-run snapshot have
252 retained values each. Their scripts explicitly select g0=10 and Isp=400.
Read-only rechecks of forward fuel depletion, untouched decoy fuel, total mass
and backward restoration agree with the independent rocket equation within
the original 1e-8 bound. No native/GUI test was rerun. The full original context,
accepted sibling statuses, source identities and immutable report/log paths
are in build/nasa-coverage/coverage95-20261110-thruster-ownership-decision-v1/proposal-v1.json.

If the interpretation is approved, perform exact-row admission review for E80
and then the E77 property conjunction. A definition answer alone does not admit
either row. Preserve the original texts, fingerprints, denominators, failures
and unrelated qualification gates. No credit or canonical modification has
been made by this proposal.
