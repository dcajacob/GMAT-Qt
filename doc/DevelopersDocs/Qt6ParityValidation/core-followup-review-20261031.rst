Six-file covariance-orientation review
======================================

The six production files in the reviewed-candidate-20261031 incremental patch
were reviewed against their archived before sources and surrounding callers.
No additional production defect was substantiated. Production sources and
binaries are unchanged by this review. Four new native API contract groups
pass on the final test executable. Existing assertions and tolerances remain
unchanged; no preexisting GUI test was replayed.

This is an author review, not independent scientific approval. The accumulated
97-file core/plugin/utility inventory still requires independent review. The
broader NASA qualification work remains deferred.

Reviewed files and mechanisms
-----------------------------

* ``ObjectReferencedAxes.cpp/.hpp``: the new nonvirtual methods calculate
  position/velocity geometry on a temporary frame copy. The copy retains the
  primary and secondary object identities; its cleared CoordinateBase solar,
  origin and J2000 pointers are not read by this calculation. AxisSystem's
  copied owned reduction arrays are null, preventing double deletion. The
  original frame and rate caches are not changed. The full-rate path still
  computes the same derivative equations and requires valid acceleration.
  Generic ObjectReferenced axes use the new path; specialized types reject
  direct use of that path rather than silently slicing their frame semantics.
* ``CoordinateConverter.cpp/.hpp``: the returned orientation is
  ``R_to^T * base_alignment * R_from``. FK5-to-ICRF uses the transpose of the
  existing ICRF-to-FK5 alignment. Other axes retain native virtual rotation
  dispatch with translation omitted and computation forced. The precise-time
  overload forwards GmatTime unchanged to those axes. Generic ObjectReferenced
  geometry retains its existing A1Mjd time resolution; this does not add new
  sub-double epoch precision. Converter last-conversion matrices are preserved.
  Fallback axes may update their own caches and calculate rates as before;
  this is not an all-axis force-free/cache-free API guarantee.
* ``Estimator.cpp``: GetCovarianceVNB continues to map full estimated states to
  the reference objects before building the rotation, restore the ordinary
  state mapping, and rotate position/velocity blocks independently. Extraction,
  state-conversion derivative and covariance multiplication are unchanged.
  The removed temporary state arrays did not contribute to the old matrix.
* ``ProcessNoiseBase.cpp``: both time overloads keep the same block-diagonal
  covariance transform. The source is the configured noise frame and the
  destination is the existing inertial frame. The change removes the unnecessary
  six-state/rate query; it does not change the process-noise model equations.

No member layout or virtual slot was added by the six-file increment. This
source observation is not a cross-platform binary compatibility qualification.
General covariance/smoother science, unsupported force and attitude coupling,
singular/ill-conditioned domains and concurrency remain outside this review.

New reusable evidence
----------------------

``src/qtgui/tests/OrientationOnlyTests.cpp`` adds four exclusive support tests:

* Composition: explicit noncommuting source/destination rotations, forward and
  inverse direction, all three public time overloads, identity conversion,
  null rejection and preserved nonidentity last-conversion data.
* InvalidGeometry: missing reference, invalid definitions, zero position/velocity,
  collinearity, NaN/infinite geometry, state-provider exception propagation,
  recovery, unchanged cached matrices and no acceleration query.
* Dispatch: prescribed ordinary-axis virtual dispatch, forced computation,
  exact precise-epoch forwarding, specialized ObjectReferenced virtual fallback,
  direct specialized-geometry rejection and unsupported base rejection.
* BaseAlignment: both FK5/ICRF directions with nontrivial axes, all public time
  overloads, and every matrix column checked against native converted vectors.
  This is a compatibility oracle for composition, not an independent physical
  qualification of NASA's ICRF data or alignment model.

Run the bounded contracts from the repository root::

   cmake --build build/linux-gui --target GmatQtOrientationOnlyTests -j4
   ctest --test-dir build/linux-gui -C Release \
      -R '^QtAcceptance\.OrientationOnly' --no-tests=error --output-on-failure

They are also included in ``build-qt-acceptance`` and ``check-qt``. They require
no visible widgets or desktop. Existing visible GUI receipts are reused because
neither GUI code nor any production binary changed. No screenshots or runtime
binary snapshots were added.

All four final contracts passed in one 0.19-second CTest run. Development
failures were confined to the new fixture: missing abstract overrides/linkage,
startup working directory, unbound mock matrix views, and empty coordinate
instance names triggering the legacy same-name identity shortcut. Those were
corrected in the fixture/build registration, preserving every assertion and
3e-13 numerical bound. The named base-alignment discrepancy was not a product
defect. Earlier development logs are retained separately from the final pass.

Evidence is under ``build/release-check/20261031-core-followup-review/``:
``final-four-contracts.xml/.log``, ``input-audit.json``, ``increment.patch``,
``before.json`` and ``sealed-handoff-verification.log``. The input audit checks
all 6,476 prior input identities: only the CMake test registration and its two
generated build/CTest files changed. All prior test commands and disabled flags
are preserved; four registrations are added (1,757 total, twelve disabled).
The historical sealed handoff remains unchanged: 36,111 files and 44,757 links
verified, with zero product tests executed by the integrity verifier.

The new fixture's three development crashes also produced approximately 3.6 MB
of system-managed compressed core dumps outside the workspace. Their exact
identities and backtraces are retained in ``development-coredumps.txt``. Cleanup
of only those three files was attempted; the container's no-new-privileges
restriction prevented sudo from removing them. They remain in
``/var/lib/systemd/coredump/``. Later test runs disabled core dumps.

Explicit next-work dispositions
-------------------------------

1. SUBUNIT-DPI-RASTER remains an open GUI rendering issue, independent of these
   core changes. The retained half-scale diagnostic has a 300x200 physical
   image for the 600x400 canvas. It locates ink at the expected marker, but the
   minimum green channel is 112; the unchanged oracle requires green below 100.
   Nearest-neighbor normalization also failed. Thus the retained evidence does
   not support a coordinate-offset or resampling-only repair. The current marker
   pen is one logical pixel, which becomes half a physical pixel at scale 0.5;
   investigate physical-pixel stroke coverage in a separate GUI increment.
   Preserve the original color/position/clipping assertions, reproduce 0.5x,
   then rerun only XY scales affected by an actual rendering change. No raster
   code or oracle was changed here, and no repair is claimed.
2. The twelve disabled nested solver hypotheses remain source-conflict gaps.
   ``Interpreter::ValidateSolverCmdLevels`` explicitly rejects solver sequence
   commands within control-logic branches. ``Using_MissionTree.xml`` gives
   broader Target/Optimize containment rules; the positive tests additionally
   demand nested If/For placement. Decide whether the menu should restrict that
   placement or the interpreter should support it before implementing a change.
   Do not enable the tests, weaken their expected results or count them passed
   simply to obtain a green catalogue. The CMake exclusions are unchanged.
3. The broader 97-file independent technical/scientific review and all other
   gaps in reviewed-candidate-20261031.rst remain open. No uninterrupted full
   final-binary replay, platform/GPU qualification or full NASA coverage is
   claimed. Existing source-versioned coverage stays 742/1,158 requirement
   scopes (64.1%) and 1,803/2,367 historical scopes (76.2%). These four support
   contracts add no requirement-scope or historical-scope credit.
