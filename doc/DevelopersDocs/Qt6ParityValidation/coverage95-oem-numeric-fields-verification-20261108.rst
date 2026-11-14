OEM numeric guards and locale-independent serialization
======================================================

A native output check demonstrated that the OEM writer inherited the host C
numeric locale. With LC_NUMERIC=en_DK.utf8, valid inputs became comma-decimal
records, for example7,000125000000000e+03. The unchanged independent B-1 checker
rejected all three such OEM1.0 files. The same formatting path also affected
acceleration and lower-triangular covariance output in OEM2.0.

CCSDSOEMWriter.cpp now formats all these numeric fields with a private stream
using the classic locale, scientific notation and the existing15 fractional
mantissa digits. Sign spacing, including negative zero, field order, covariance
layout and serialized epochs remain unchanged. It does not change process locale,
class layout, public API, numerical values, interpolation or scientific formulas.

The original requirement FRR!E375 pins the September2004
`CCSDS502.0-B-1 standard <https://spsweb.fltops.jpl.nasa.gov/portaldataops/mpg/MPG_Docs/Source%20Docs/CCSDS-OrbitDataMessages-502x0b1.pdf>`_.
Its exact retained PDF remains SHA256
fab306a994e5bce9cad8c64d77af172a5819af38faf459faab1b7e2f6163223c.
The existing independent checker covers the selected KVN/decimal rules. Shared
OEM2 cases check numeric syntax and field ordering only; they do not qualify
whole OEM2 format or covariance/acceleration science.

Native verification
-------------------

The previously compiled finite-state guard passes27 direct writer and22 subscriber
buffer rejection checks. They cover NaN and both infinities in the epoch and all
six Cartesian positions, short direct-writer vectors of lengths0through5 and a
null subscriber state. Invalid samples preserve the direct serialized count and
are rejected before a full subscriber buffer can flush. The same writer/buffer
then accepts valid data; recovered output contains exactly the valid records.

Six finite OEM1.0 files cover C and comma-decimal locales with ordinary signed
values, very large finite values (maximum double divided by8), minimum normal
and subnormal values, positive/negative zero and decimal/exponent scales. Two
additional OEM2 files exercise the shared acceleration/covariance paths. Those
serialization inputs are synthetic; no physical trajectory claim follows.

All57 new native cases pass independent retained-byte checks. The unchanged
36-case OEM text suite also passes:93 case checks in two registered CTests.
Ordinary C-locale data in31 retained files is byte-identical to the pre-repair
output. All four paired C/comma numeric-file outputs now match. Comparison covers
numeric data, whitespace, epochs, signed zero, acceleration and covariance;
creation-clock fields are excluded from cross-time byte comparisons.

Failure and source preservation
-------------------------------

Both pre-repair runs remain failed evidence:52/55 and53/57 observations pass,
with three then four comma-locale failures. All49 finite-guard rejections already
passed before the locale repair. No original failure is inverted or waived.
Two initial new-target configuration/include failures and an attempted GUI-runner
filter that selected no native support test remain logged. They did not execute
native cases or change original assertions. The support cases run through CTest,
with a live native process/image mapping handshake.

The final guard has1147 source and33 binary identity entries; before/after values
match. Native process maps identify50 loaded ELF images. A source-only capsule
retains1149 distinct versions, including the pre-extension witness and pre-repair
writer. No runtime images are copied. All1334 protected original test inputs match.
Scoped whitespace checks pass. All build/test process handles are terminal.

Evidence lives in
``build/nasa-coverage/coverage95-20261108-oem-numeric-fields``.
Its component audit rechecks raw outputs, atomic rejection/recovery, ordinary
output compatibility, exact source versions, runtime identities and protected
originals. The publication has a separate read-only verifier. No GUI scenario
was necessary for this direct writer formatting defect; previous GUI evidence
keeps its recorded source version.

Original scope and continuation
-------------------------------

Canonical acceptance remains1076/1158 (92.9%),82open,25needed for1101. No new
whole-row credit is assigned. FRR!E374/E375 remain open for their complete original
conjunctions, including retained scientific failures and unresolved conventions.
The historical frame-list placeholder, complete scientific/physical-domain,
platform, current-build and full-NASA gates remain open.

The separate exact-row review of FRR!E519/E520 retained the inherited any-celestial-
body obligation from FRR!E501. The previous Earth-only worksheet evidence cannot
qualify that whole scope. The explicit read-only compatibility implementation
exists; absence of wider evidence is not treated as absent code.

Next perform the finite current implementation audit of the remaining82 original
rows. Use current owning sources and retained evidence to identify actual missing
required behavior, external inputs and undefined specifications. Do not invent
more parameter permutations or repeat completed matrices to increase a test count.
