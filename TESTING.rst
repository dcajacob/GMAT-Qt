Qt desktop tester snapshot
==========================

Branch: ``codex/stable-qt-preview-20261104``.

This is a frozen preview for exploratory testing. Development and the NASA
acceptance campaign continue separately. The recorded campaign evidence covers
1,052 of 1,158 original requirements; 95 percent acceptance and full scientific,
platform and current-build qualification remain incomplete.

Getting the source
------------------

Use the private repository URL supplied with this branch::

  git clone --branch codex/stable-qt-preview-20261104 https://github.com/dcajacob/GMAT-Qt.git
  cd GMAT-Qt

Build on Linux
--------------

Install a C++17 compiler, CMake, Ninja, Qt 6.4 or newer (Widgets, OpenGLWidgets
and Network development packages), OpenSceneGraph 3.6 development libraries and
format plugins, Xerces-C development files and OpenGL development files.
Install the NAIF CSPICE toolkit separately; its directory must contain
``include/SpiceUsr.h`` and ``lib/cspice.a``.

This configuration excludes the optional MATLAB, Python and wx frontends::

  cmake -S . -B build/tester -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DGMAT_INCLUDE_QT_GUI=ON -DGMAT_INCLUDE_GUI=OFF \
    -DGMAT_INCLUDE_API=OFF -DGMAT_QT_BUILD_TESTS=OFF \
    -DPLUGIN_MATLABINTERFACE=OFF -DPLUGIN_FMINCONOPTIMIZER=OFF \
    -DPLUGIN_PYTHONINTERFACE=OFF -DPLUGIN_OVTOOFI=OFF \
    -DCSPICE_DIR=/absolute/path/to/cspice
  cmake --build build/tester --target GmatQt --parallel 4
  ./application/bin/GmatQt

The build generates its own Qt startup file. Runtime data remains under
``application/data``. MATLAB is optional for ordinary mission editing and
propagation; MATLAB calls and fmincon need a separately licensed installation
and an explicitly enabled plugin build. VF13ad is distributed separately.

Try these workflows
-------------------

* Create a default mission and run it; inspect the orbit and ground track.
* Change a spacecraft state or propagator, Apply, and try Undo and Redo.
* Save a script, reopen it and run it again.
* Open a report from the Output tree and compare its values with the script.

Keep copies of your scripts before experimenting. For a bug report, include
the branch commit (``git rev-parse HEAD``), operating system, exact steps,
script and relevant messages or output. Use the private repository for reports
that contain private mission data.

Validation
----------

The current Linux ``QtGui.Workflow`` smoke passes both unattended and visibly.
``QtGui.Mission`` retains a failure in its older assertion that Yukon bounds
are disabled. Later campaign work deliberately enables and independently tests
Yukon's documented bound support. The older assertion has not been changed.

Windows and Mac packages are built by the ``Qt preview packages`` workflow.
Only successful native package probes can reach the private preview release.
The Windows build is x64; Mac builds cover Apple Silicon and Intel, targeting
macOS 15 or newer. Mac previews are ad-hoc signed, without Apple notarization.

The source is under active acceptance testing. Source-versioned campaign
passes apply to their recorded builds and cases. They do not certify every
feature in this snapshot or establish a current Windows/macOS build.

The current campaign checkpoint and preserved limitations are recorded in
``doc/DevelopersDocs/Qt6ParityValidation/coverage95-chat-handoff-20261102.md``.
Detailed local acceptance artifacts are kept outside Git under ``build``.
