Normalized model and solar coefficient units
============================================

Nine visible normalized-value observations initially lack unit labels. This
batch fixes the GUI unit resolver for Bulirsch--Stoer step-reduction ratios,
sequential-filter normalized residual/deweight controls, the smoother deweight
factor, and the supported FirstOrderGaussMarkov steady-state value and sigma.
The estimated-parameter inputs are checked for both Cd and atmospheric-density
scale factor. One GUI source file changes; native numerical values, units APIs,
algorithms and model layouts are unchanged.

The independent definitions are the current owning resource documentation.
Conditional integrator error scales and covariance-weight-dependent RMS
tolerances are excluded from an unconditional dimensionless label. Existing
hardware exponent and direction labels retain their recorded evidence; no
unrelated numerical or GUI regression is repeated.

The initial resolver change fixes seven of the nine observations. The two
remaining Bulirsch--Stoer inputs require GetTypeName instead of IsOfType because
that plugin's native string-type registry does not include its concrete type
name. The intermediate two failures remain failed. A supplementary fixture
copies only the unchanged adaptive-step case and reruns those two fields after
the lookup repair. The other seven headless passes are reused. The five solar
coefficient labels pass on the first headless run and are also reused.

All 14 observations pass in the first visible run on the real nested Xephyr
desktop, started minimized. Across the two profiles this gives 28 passing
source-versioned observations over 12 declared type/field pairs. The extra two
observations come from the two supported estimated-parameter solve-for modes.
Actual primary captions and backing metadata are checked. Source text, undo
history, saved file bytes and native serialized values remain unchanged. The
label checks execute no mission and claim no new numerical-model accuracy.

Compiled source capsules, native image hashes, plugin loader receipts and
original failed GUI results are retained. The ExtraPropagators source capsule
is added through a targeted build; its image matches the one already loaded
by the original cases, so no earlier case is repeated merely to add provenance.
All native library hashes remain unchanged through these GUI-only edits. All
1,334 protected original inputs are unchanged. Four corrupted label-proof
controls are rejected during replay without native execution.

The stage is build/nasa-coverage/complete18-20261112-normalized-model-units-v1.
It extends engineering receipt
``36cf3dc4002c3eb1d7c6476ccfeba957e5f2cffbf699c724f2e3657f07d10ed6``.
The canonical prefix remains
``de77757b896aa68c65df2462c026b58bf11ed684b502148fa5a14c0593111574``.
All Others!E13 remains open for its full input/meaning and conditional-field
conjunction. Overall coverage stays 1,106/1,158, with 5 of the agreed 18 complete.
No partial row credit, full regression or whole-current-build qualification is
claimed.
