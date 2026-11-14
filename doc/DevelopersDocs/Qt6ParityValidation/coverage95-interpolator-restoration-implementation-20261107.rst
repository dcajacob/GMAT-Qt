Interpolator table and scratch restoration
==========================================

Spacecraft restored its center-of-mass and moment-of-inertia interpolators by deleting and replacing them. FileThrust retained interpolator roots but assigned through ordinary copies that reallocated ring and scratch arrays. Hermite value copies also omitted derivative/polynomial/time vectors; assignment cleared the copied ring. This packet supplies explicit compatible-storage restoration and repairs those captures.

Native restoration providers
----------------------------

Lagrange, linear, cubic spline, not-a-knot and Hermite interpolators expose nonvirtual ValidateRestorationLayout and RestoreValues methods. Protected base helpers validate/copy only common table state, so a base-pointer operation does not silently claim to restore derived scratch. Each concrete provider checks its exact native type and compatible storage. Names remain selected. No interpolator object, table, scratch row or coefficient array is allocated or retired by successful compatible restoration.

Common values include ring contents, previous independent value, counts/latest index, required point count, cached range, direction and force/extrapolation/order-reset flags. Lagrange also restores order, selected window/index metadata and its complete x/y scratch. Spline providers restore knots, coefficients, cache timestamps and auxiliary values. Not-a-knot restores its right-hand-side and solution rows too. Hermite restores derivative/coefficient/time data in existing nested vectors after checking every extent.

An empty unallocated source table can reset an already allocated destination to empty while keeping its storage. An allocated source cannot restore into an unprepared unallocated destination. Dimensions/capacity changes, inconsistent row presence, unknown subclasses or changed Hermite nested extents fail explicitly. These preparation boundaries remain implementation scope; no fallback silently replaces borrowed storage.

Capture and allocation repairs
------------------------------

Ring and native Lagrange/spline scratch allocations now stage initialized arrays and row owners before invoking existing cleanup and publishing pointers. Allocation failure releases staged rows. This avoids partially published uninitialized row pointers and makes unused allocated values defined for snapshots. Existing capacities, interpolation equations, tolerances and stencils are unchanged.

Range values are captured even when their calculated flag is false. Hermite copies retain derivatives, q coefficients and time values; assignment stages those vectors and retains the copied ring instead of immediately clearing it. Not-a-knot copies include B and s scratch rows and initialize otherwise absent pointer arrays in an empty copy. These changes implement value capture; scientific equivalence still requires the later verification phase.

Owning resource integration
---------------------------

Spacecraft checks mass-property interpolator presence/layout before restoration, then restores numerical values in its selected Lagrange objects. Ordinary assignment retains replacement behavior. FileThrust checks its linear/not-a-knot layouts before applying physical-model values and restores their table/scratch values through the typed methods. Its restoration failure guard leaves the object unready, including validation failures. Native force/interpolation formulas are unchanged.

The cubic/Hermite providers are available for typed native owners. This packet does not claim that every ephemeris/attitude/history reader or foreign provider has adopted them. Changing optional root presence, dimensions or Hermite extents, and rebuilding dependent references coherently, remains open.

Compilation and compatibility
-----------------------------

Configured C++ compiler v1: return 0, 127.51 seconds, last logged step 412/412.

Seventeen source/help and four planning preimages preceded edits. Before/build/final hashes, indexed patches, source map, compiler logs and terminal receipts are retained. All compiler sessions are terminal; final source hashes match the last successful build. Added-line whitespace is clean and three help XML files parse. Existing diagnostic lines are retained in the compiler record. The methods are nonvirtual; public instance layouts and virtual slots are unchanged. Matching utility/native/plugin/GUI consumers rebuild against the new methods. External binary/plugin and other platform qualification remain undone.

Complete coherent changed-component/membership/topology and interpolation-storage preparation at iteration boundaries, including lazy cache creation, Hermite extent changes, firing/burn transitions and dependent propagation/measurement registrations; compatible fixed-storage providers do not complete the all-command contract. Finish original function I/O/scoping and full supported command/resource/clone/ownership behavior, state/result/posterior and estimator/smoother requirements, public restoration-buffer contracts, shared external values/lifetimes and source/stream/participant/error/noise/hardware providers. Preserve ordinary cold estimator definition-copy semantics unless a concrete restoration/value contract requires a separate path. Continue private renderer, any-Resource Save/persistence, body/kernel/NAIF/CSPICE-pool, ephemeris B-1/format/final-iterative/scientific, specification/external/historical dependencies and independent POV/rotation research. Defer runtime verification until the full implementation pass is complete.

No runtime/GUI/mission/numerical tests, new tests, acceptance auditors/readers/seals, evidence admission, fixture/assertion/tolerance/denominator/gate changes, packages, installed/shared runtime changes, commits, pushes or external messages were performed in this batch.

Tracking-v99 remains 1093/1158 (94.39%), with 65 unresolved rows. The recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full implementation audit incomplete; goal active.
