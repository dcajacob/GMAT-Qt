Dimensionless estimator input labels
====================================

Three estimator controls lack their documented dimensionless labels:
ILSEMultiplicativeConstant, OLSEMultiplicativeConstant and RelativeTol. The
targeted native GUI baseline retains six failures through the BatchEstimator
and deprecated BatchEstimatorInv factory routes. Both routes create the same
native BatchEstimator class; they are not independent estimator algorithms.

The repair adds only these three controls to the existing GUI unit resolver in
ResourceProperties.cpp. It changes no native unit APIs, numerical value,
estimator formula or resource layout. The same frozen case passes natively
headless and visibly: 12 label observations across the two profiles. The actual
primary captions and backing metadata both expose the dimensionless unit.
Source text, undo history, native serialized values and saved Unicode file
bytes remain unchanged while the forms are viewed. These are label checks;
no mission or numerical regression is run.

Current owning documentation supplies the independent quantity definitions.
The older 447-record unit inventory is planning evidence rather than current
GUI qualification. Its 50 compressed census files are recovered and verified
against their original decompressed hashes without rerunning the census.
Current documentation is joined by exact labels: 450 candidate entries match,
six remain unresolved and 164 matched unit descriptions changed since the old
inventory. These are documentation entries, not distinct controls or defects.
An earlier comparison by saved row index is retained as a draft; the exact-label
join supersedes its unchecked interpretation.

In particular, the old inventory calls AbsoluteTol and OLSEInitialRMSSigma
dimensionless. The current owning guide describes covariance-weight-dependent
residual units. Those fields are excluded from this unconditional label repair;
their conditional meaning remains a separate obligation. N/A, identifiers,
counts, physical units and expression-dependent fields are not automatically
relabelled dimensionless. The original blank-ECC-unit expectation and its
separate retained failure are unchanged, and are not rerun here.

Only affected targets are rebuilt. The passing checks use private X11 and a
real Xephyr window on the existing desktop, started minimized. Loaded native
library/plugin hashes and compiled source capsules identify each execution.
All native library hashes are identical across this GUI-only repair. Other
passing unit cases remain at their recorded source versions and are not rerun.

All 1,334 protected original inputs remain unchanged. Four corrupted evidence
controls reject a missing visible label, a missing route, lost source/native
isolation and an unexpected mission execution. Frozen replay launches no
native tests. All Others!E13 stays open pending the complete input/meaning and
conditional-field conjunction. Coverage remains 1,106/1,158, with 5 of the
agreed 18 complete. There is no partial, whole-row or whole-current-build credit.

The stage is build/nasa-coverage/complete18-20261112-estimator-dimensionless-units-v1.
It extends engineering receipt
``029fd9479d6f0f29b4f45d1317b233ef0a16d400166994941b6db5152579195b``.
The canonical prefix remains
``de77757b896aa68c65df2462c026b58bf11ed684b502148fa5a14c0593111574``.
