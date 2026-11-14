Integrated current-worktree production build
===========================================

The preceding implementation increments were compiled individually. This build
compiles and links the configured production components together, including the
changed exported estimation types, against matching current headers.

It also exposed one actual fresh-configuration defect: SYS_PATH was initialized
only for the wx frontend, while Linux SWIG setup used it regardless. A Qt/API-only
configure therefore removed the system tool directories from PATH and compilers
could not find the assembler. Capturing SYS_PATH before frontend-specific setup
fixes that case without changing existing explicitly configured values.

Build artifacts
---------------

All artifacts are under
``build/nasa-coverage/coverage95-20261105-integrated-current``. Build and output
directories are private; no installed binary was replaced. The existing source
checkout supplied compilation inputs directly, without a new source archive.

The build used the existing enabled plugin/API profile with Release/C++17 and
eight low-priority jobs. Qt, console, Python 3.14 API, MATLAB/fmincon,
estimation/EKF, Save, MSISE86, Yukon and the other configured plugins build.
Qt integration-test building was disabled; the project's normal C support
clients are still part of its configured build. No new test registration or
matrix was added. All 965 steps passed in 340.13 seconds, producing 31 native
runtime images. One existing Qt QImage::mirrored deprecation warning remains.

``configure-command-v4.json`` records configuration, ``build-command.json`` the
build invocation, ``build-results.json`` its result and ``built-images.json`` the
new runtime image hashes. The output-layout check found 1820 targets with all
nonphony outputs inside the private stage. The 2788 tracked production inputs
were stable during compilation; seven monitored installed images retained their
original hashes. Full scientific/current-build acceptance is a separate gate.

All failed configures remain. The first exposed the PATH defect and required
the existing local SWIG executable/directory to be selected explicitly. Later
attempts lacked startup/Python template files required by the documented custom
output layout. Those were supplied from canonical small text inputs. Existing
data directories and the externally supplied VF13ad plugin are linked to their
original locations. No native library, executable or orbit kernel was copied.
This runtime is a local development layout, not a new distributable release.

Runtime smoke
-------------

The new Qt executable's help command, import of the new Python API package and
the shipped Ex_GMATFunction_Math console sample all return zero. The sample
reports its independent cross-product values 0.25, -2.75 and -2.

The freshly built unchanged native C clients pass 133 basic and 481 STM
assertions, using the integrated core and CInterface. Their loaded-image output
identifies the new library and excludes the older installed CInterface. These
checks are native C support, not GUI requirement admission.

The new production Qt executable runs an identical private copy of the shipped
math sample in qt-sample-smoke-v2. Native X11 displays it inside a resizable
nested desktop that starts Iconic/minimized. The report contains the same three
values, the new Qt and Base images are mapped, the old Base is absent, and the
private window closes normally through WM_DELETE_WINDOW with exit code zero.
No screenshots were taken. The source sample remains byte-identical.

The first Qt launch inherited offscreen rendering, produced a correct report
but had no native window to close, and its own launcher terminated it. That
receipt remains failed and is not used as visible evidence. All build, client,
Qt and test-desktop processes are now terminal. No SDK licensing service was
terminated. Raw commands, reports, maps and receipts remain beside each smoke.

Launch and replay
-----------------

Run the private application with isolated settings using:

::

  /home/dan/GIT/GMAT-Qt/build/nasa-coverage/coverage95-20261105-integrated-current/application/bin/GmatQt \
    --startup /home/dan/GIT/GMAT-Qt/build/nasa-coverage/coverage95-20261105-integrated-current/application/bin/gmat_startup_qt.txt \
    --settings-dir /home/dan/GIT/GMAT-Qt/build/nasa-coverage/coverage95-20261105-integrated-current/interactive-settings

The console accepts --startup_file and --run as recorded in its command.json.
Python import uses /usr/bin/python3 and the private bin directory on PYTHONPATH.
Existing C client commands run from their saved session directories with the
private bin/plugins directories first on LD_LIBRARY_PATH. Preserve old result
directories when replaying. The saved native Qt receipt and display settings
describe its explicit xcb, minimized, resizable route; default inherited
offscreen rendering is insufficient for visible qualification.

Scope remains unchanged
-----------------------

The integrated build resolves an implementation/integration prerequisite. It
does not qualify all current code, original requirement conjunctions, external
ABI/platforms, provider physics or the full NASA campaign. No acceptance rows
were admitted. Verified coverage remains 1053/1158, 48 short of 1101, with all
original hold identities and separate historical coverage intact. New focused
acceptance backfill remains deferred under the user's implementation-first
direction. Use this matched runtime when affected validation resumes.
