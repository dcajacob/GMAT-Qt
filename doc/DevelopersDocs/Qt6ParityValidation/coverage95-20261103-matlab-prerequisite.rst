MATLAB installation and Engine prerequisites
=============================================

The user installed MATLAB and then installed the missing csh prerequisite.
The current installation is **MATLAB R2026b Update 1**, version
``26.2.0.3433840``, at ``/usr/local/MATLAB/R2026b``. MATLAB starts in the desktop
licensing environment. Optimization Toolbox is installed, ``fmincon`` resolves,
and its license test returns one. The current runtime reports a trial license;
no expiration date or later availability is inferred.

A direct MATLAB execution and a compiled C Engine client both solve an
independently specified constrained problem. The objective is
``(x1-2)^2 + (x2+1)^2``, with ``x1+x2 <= 0`` and bounds [-4,4]. Both return
approximately (1.5,-1.5), objective 0.5 and positive exit flag one, within the
unchanged 1e-6 solution and 1e-8 objective bounds. The C client uses the same
legacy ``engOpen``, ``engEvalString`` and ``engGetVariable`` API family used by
GMAT, then closes the engine. This is prerequisite evidence, not a GMAT plugin,
GUI or NASA requirement pass.

Resolved startup failures
--------------------------

The sandboxed MATLAB launch exited before producing version/license output;
the desktop-environment launch succeeded. The original C Engine probe failed
before MATLAB started. An exec-only trace showed the engine trying to execute
``/bin/csh`` and receiving ENOENT. An unattended package-install attempt required
interactive sudo authentication. The user installed csh, and the identical
compiled client then passed. No shell substitution or simulated engine is used.

The first direct-toolbox launch used an invalid hyphenated MATLAB script name.
MATLAB interpreted its stem as an expression and called fmincon without inputs.
The original script and failure remain. The identical script bytes with the
valid ``gmat_fmincon_prerequisite_v2.m`` filename pass. This is a launch-fixture
correction, not a solver or numerical-bound change.

Remaining GMAT integration
----------------------------

The checkout's FminconOptimizer CMake file explicitly returns on non-Windows
platforms. MatlabInterface's Unix build currently also requires wxWidgets and
adds legacy MATLAB-to-GMAT IPC client sources. The Qt build's plugin options
remain disabled and its MATLAB root has not yet been reconfigured. The working
base Engine API does not establish the Qt callback/server path or justify
removing the platform guard without implementation and testing.

Next, review those build/runtime dependencies and enable the GMAT-to-MATLAB
function path with exact plugin source and image identities. Keep optimizer
callbacks, shared/single-use modes and reverse-interface obligations separate
until their actual complete tests pass. The original 37 MATLAB-dependent row
keys and all 59 hold keys remain in the unchanged acceptance ledger. Their
availability rationale must now refer to this installed, verified prerequisite;
do not continue describing MATLAB as absent. Coverage remains **1,019/1,158**
and historical operations **1,816/2,367**. No acceptance row is admitted here.

Evidence and replay
---------------------

``build/nasa-coverage/coverage95-20261103-matlab-prerequisite`` retains the
MATLAB and engine logs, both failed engine launches, exec trace, source probes,
failed sudo attempt, valid and invalid-name toolbox fixtures, installed version
and library identities, and the final prerequisite receipt. The compiled test
client remains in /tmp; no MATLAB library or executable is copied.

Rebuild and run the genuine C Engine client::

   stage=build/nasa-coverage/coverage95-20261103-matlab-prerequisite
   cc -std=c11 "$stage/engine-probe-v1.c" \
     -I/usr/local/MATLAB/R2026b/extern/include \
     -L/usr/local/MATLAB/R2026b/bin/glnxa64 \
     -Wl,-rpath,/usr/local/MATLAB/R2026b/bin/glnxa64 \
     -leng -lmx -lm -o /tmp/gmat-matlab-engine-probe-replay
   /tmp/gmat-matlab-engine-probe-replay

Use a desktop licensing environment with csh installed and MATLAB on PATH.
Recheck availability when the installation or license changes; a successful
prerequisite is not permanent or complete GMAT qualification.
