GMAT Qt alpha testing
=====================

Obtain the frozen source with::

  git clone --branch codex/alpha-0.1.0 https://github.com/dcajacob/GMAT-Qt.git
  cd GMAT-Qt

Native package builds use ``tools/preview/BuildPreviewPackage.py``. The
``Alpha packages`` workflow supplies the platform dependencies and preserves
diagnostics and matching compiled inputs for packaging retries.

To build on Linux, install a C++17 compiler, CMake, Qt 6.4 or newer development
packages for Widgets, OpenGLWidgets and Network, OpenSceneGraph 3.6, Xerces-C,
OpenGL, and the NAIF CSPICE toolkit. Configure and build with the same options
in the packaging script's ``build`` function, supplying your CSPICE directory.

The package probe runs a 600-second circular two-body mission with an
independent analytic oracle, creates orbit, ground-track and data plots,
writes a report, saves the spacecraft, and reopens its saved state. It retains
the earlier probe bounds of 2e-5 km for position and 3e-8 km/s for velocity.
Linux uses Xvfb with software OpenGL. Windows and Mac probes use Qt's offscreen
platform, so their screenshots provide limited renderer coverage.

The original acceptance evidence and numerical failure receipts remain
preserved. The release probes supplement those source-versioned results.

For manual review, run a default mission, exercise playback and view controls,
edit spacecraft properties, and save/reopen a copy of the mission. Try the
satellite-import dialog using a local TLE or OMM before testing an external
provider. Keep Space-Track credentials in the app's account dialog.

Report the package version and source commit, operating system, exact steps,
and a small reproducing script at https://github.com/dcajacob/GMAT-Qt/issues.
Remove credentials and private mission information from submitted logs.
