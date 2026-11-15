Reused hardware-unit evidence
=============================

Twelve saved GUI runs provide 236 unit-label observations for 54 distinct
hardware fields, including conditional enabled/disabled controls and the
chemical-coefficient dialog. Both headless and visible observations are
reused at their recorded source versions. No native test was launched.

The unit review derives the chemical C coefficients from force in newtons
and the K coefficients from specific impulse in seconds, with pressure in
kilopascals. Pressure exponents, exponential bases, and temperature-ratio
exponents are dimensionless. The current guide now agrees with the native
K6/K8/K10 exponent names; the original discrepancy receipts remain intact.
The normalizer preserves the distinction between C and K exponent symbols.

The legacy Thruster name resolves to ChemicalThruster in both the recorded
factory census and factory implementation. Its default field metadata
matches exactly. This permits 38 additional alias mappings without claiming
38 additional GUI tests. In total, the evidence accounts for 92 entries in
the saved numeric-unit worklist, leaving 277 entries for review. That count
is a work queue, not a defect count or requirement coverage percentage.

The canonical ledger remains 1,111/1,158, with ten of eighteen agreed items
complete. All Others!E13 remains open pending its complete field, conditional
unit, expression and actual-GUI evidence conjunction. The prior failed label
checks and original numerical assertions are preserved.

The sealed stage is
``build/nasa-coverage/complete18-20261112-unit-evidence-join-v1``.
Its v2 evidence index supersedes the unsealed v1 draft and freezes the unit
definitions used for this replay. It does not claim the entire current build
or new scientific qualification.
