Linux Qt MATLAB Engine plugin build
====================================

The GMAT ``MatlabInterface`` plugin now builds against the verified R2026b
installation. CMake finds the actual program, Engine, Matrix and MAT libraries.
The original configuration skipped the plugin because wxWidgets was absent.
Both configuration attempts and the successful first plugin build are retained.

The sole source change is in
``plugins/MatlabInterfacePlugin/src/base/CMakeLists.txt``. For Linux Qt builds,
the GMAT-to-MATLAB Engine implementation is built without two unused legacy
MATLAB-to-wx-GMAT IPC client translation units and their wx include/link setup.
The source dependency review finds no use of those client classes in the
plugin's base implementation. Other platform/application combinations keep
their previous wx IPC condition. The Unix library search-path setup is retained.
No C++ source, class layout, method, factory or numerical expression changes.

The generated image is
``application/plugins/libMatlabInterface.so.R2026a``, SHA-256
``e2e55052514b4e783d0570f1e36f2cc8822b765cf21bda14b941f4f16047c7b4``.
Dynamic linkage has no missing dependency and no wx library dependency.
The build flag is now on and MATLAB root is explicitly configured. The global
startup file is unchanged; the plugin has not yet been loaded or exercised by
a GMAT acceptance mission. A successful library build is not a GMAT function,
GUI or NASA requirement pass.

Next exact-row acceptance target
---------------------------------

``All Others!E152`` is the original MATLAB-function Resource Tree leaf. Its
inherited obligation is actual GUI creation and configuration of that resource.
The next test should use the actual Resource Tree Add route and creator,
Cancel/accept, native MatlabFunction identity, file/path configuration through
the real editor, source/history isolation, Apply, Unicode save/reopen and a
real MATLAB call with independently specified inputs/results. Inspect the full
leaf's inheritance before admission. The broader MATLAB I/O, function nesting,
optimizer, shared/single-use and reverse-interface requirements remain separate
complete obligations, with no credit from this preflight.

FminconOptimizer's non-Windows CMake guard remains byte-identical. The legacy
reverse IPC and Qt optimizer callback/server paths are not provided by this
build change. Do not remove that guard or equate the standalone fmincon
prerequisite with GMAT optimizer qualification.

Coverage remains **1,019/1,158 (88.0%)**, with 139 open and 82 more needed.
Historical operations remain **1,816/2,367 (76.7%)**. All original 59 hold keys,
including the 37 MATLAB-dependent rows, retain open acceptance status. MATLAB
and Optimization Toolbox availability and the direct C Engine prerequisite are
now verified; their older absence statements are historical. The full goal and
scientific/core/current-build/NASA gates remain open.

Evidence and reproduction
---------------------------

``build/nasa-coverage/coverage95-20261103-matlab-integration`` contains the exact
pre-change source/cache archive, both configure logs, build log, isolated CMake
patch, dependency records and build receipt. Existing runtime libraries are
hash-bound rather than copied. No GUI scenario or native acceptance mission
was executed in this build increment, and no old passing mission was replayed.

Reconfigure and build::

   cmake -S . -B build/linux-gui \
     -DMatlab_ROOT_DIR=/usr/local/MATLAB/R2026b -DPLUGIN_MATLABINTERFACE=ON
   cmake --build build/linux-gui --target MatlabInterface -j4

Use an isolated startup for the new acceptance tests. Preserve the existing
startup and old source-versioned receipts. Run new/affected GUI cases unattended
and visibly in initially minimized, resizable, non-fullscreen desktops, with
zero screenshots and actual source/image custody.
