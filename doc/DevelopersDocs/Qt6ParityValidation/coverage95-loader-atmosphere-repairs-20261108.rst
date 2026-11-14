Coordinate-table loader and atmosphere linkage repairs
======================================================

AddressSanitizer exposed conflicting private C symbols in MSISE90 and
NRLMSISE00, including common blocks of different sizes. The NRLMSISE00 plugin
now hides its translated C implementation symbols on UNIX GNU/Clang builds.
The public C++ factory exports remain available. Equations, data and layouts
are unchanged; all 41 original atmosphere controls pass in both display modes.

With the default sanitizer checks restored, the original plot test exposed
array allocations freed by scalar delete in ICRFFile. Five deletion sites are
corrected. File handles and growth buffers now use RAII; failed loads clean up,
Finalize resets initialization and epoch caches, and malformed tables reject
cleanly. Valid interpolation and rotation formulas and shipped data are intact.

The added QtAcceptance.ICRFLoaderLifecycle regression passes 473 checks under
AddressSanitizer and in the ordinary build. It covers growth, independent
linear interpolation, repeated reloads, file closure, same-epoch cache
invalidation, invalid-input recovery and the shipped table. All 70 original
atmosphere/calculated-point/origin-frame controls pass in both display modes.

The ordinary combined plot run completes 95 internal cases before its original
history-prefix assertion fails: the existing display cap retains 10,000 blocks
and drops the oldest lines. The old suffix is preserved byte for byte. All 30
remaining original frame cases pass in a fresh run using the existing selector.
Neither the cap nor the assertion was changed. The instrumented combined run
hits its original 300-second timeout after 58 cases without another address
error. These results do not qualify the combined parent or prove every earlier
crash resolved.

All 1,334 protected original inputs remain unchanged. Reviews 056 and 057 and
checkpoint 044 bind the evidence in the corresponding msise-symbol-isolation,
icrf-loader-memory and long-plot-crash directories under build/nasa-coverage.
Canonical acceptance remains 1,078/1,158 (93.1%). Full rebuild/regression,
unresolved science/provider requirements and platform qualification remain open.
