Fmincon callback startup: preserved native before evidence
========================================================

Coverage remains **1,022/1,158 (88.3%)**, with 136 original requirements open and
79 more needed. The MATLAB Mission Tree admission is sealed separately in
``coverage95-20261103-matlab-mission-call.rst``. This startup stage admits no row
and does not claim an optimizer solve or callback implementation.

The source review freezes34 owning paths and the exact23 original optimizer
candidate rows in
``build/nasa-coverage/coverage95-20261103-fmincon-contract-review/contract-review-v1.json``.
All23 remain open, including their original headings and whole-row conjunctions.
All59 original hold identities remain; three are resolved,56 unresolved,including34
MATLAB rows. All broader qualification gates remain false.

``QtGui.NasaFminconCallbackStartup`` is a new registered positive regression.
It opens the actual Qt main window with the real MATLAB plugin in a private startup,
records the actual factory list, invokes Moderator's real Qt server-start route,
attempts a real FminconOptimizer-resource script build, and checks caller/history
preservation. It expects an available factory and successful server/build.
The current results are FAILED, not expected-failure passes or admissions:

* Factory absent and FminconOptimizer-resource build unsuccessful;
* server startup throws exactly
  ``Interpreter Exception: This Interpreter cannot start the external server``;
* caller source and history remain unchanged.

The current CMake cache has PLUGIN_FMINCONOPTIMIZER=OFF and
PLUGIN_MATLABINTERFACE=ON. The observed absent factory does not independently prove
that the non-Windows guard was reached. The unchanged plugin CMake source has a
non-Windows return, so enabling the option alone still does not implement Linux Qt
callbacks. A future startup must also explicitly load the real optimizer plugin
in its private startup; the current fixture loads only MatlabInterface.

The first pair of runs (source1) remains with a failed offline custody audit:
12 reference paths appeared in the runner receipt but were missing from the
watcher's source archive (11 ancillary C-interface MATLAB scripts and the newer
Mission Tree admission reader). The exact list and failed reader survive in
``startup-custody-correction-v1.json``. No test assertion or production code changed.
The complete source2 capsule contains3,035 paths; only these two short regressions
were repeated. Both again fail at the same actual server hook. Each maps83 actual
project/SDK images, with unchanged input hashes. The visible run has two native
exposures and an initially minimized,resizable,nonfullscreen nested display.
The observer and JUnit are terminal. No screenshots,runtime-binary or kernel copies
are retained. ``startup-before-audit-v2.json`` audits the failures and binds108
regular artifacts, including the first runs. Its status remains implementation
pending; qualified_keys is empty and new_requirement_credit is zero.

The reusable registration builds and runs with fresh output directories::

   cmake --build build/linux-gui --target GmatQtNasaFminconCallbackStartupTests -j2
   python3 src/qtgui/tests/RunFminconCallbackAcceptance.py --output build/gui-acceptance/fmincon-startup-fresh-headless
   python3 src/qtgui/tests/RunFminconCallbackAcceptance.py --visible --output build/gui-acceptance/fmincon-startup-fresh-visible

Frozen run-with-maps-vN scripts require their exact recorded source/image versions.
They deliberately stop if live sources differ. Fresh passing execution would still
need independent original-row admission; this regression alone is not that proof.

Next implementation must keep MATLAB library access on one owner thread and GMAT
command/callback execution on Qt's thread,service Qt events while MATLAB evaluates,
and handle reentrant Engine requests,Stop/errors and cleanup. The existing real
Optimize/ExternalOptimizer state machine must survive. Callback results currently
pass through a fixed1024-byte sprintf buffer; an unbounded new protocol cannot
silently reuse that limit. The current iteration counter counts control-sequence
passes; actual MATLAB iteration notifications are needed for progress/report rows.
No platform guard has yet been removed and no production callback code has changed.

The provider documentation says MATLAB libraries are not thread-safe and limits
Engine access to one thread:
https://www.mathworks.com/help/matlab/matlab_external/multithreaded-applications.html
Base MATLAB TCP client support is documented at
https://www.mathworks.com/help/matlab/ref/tcpclient.html ; its ConnectTimeout option
is marked as an Instrument Control Toolbox feature. A bridge must not introduce
that extra toolbox prerequisite. The documented blocking/timeout behavior is at
https://www.mathworks.com/help/matlab/ref/tcpclient.readline.html . These are design
constraints,not evidence of a completed GMAT integration.
