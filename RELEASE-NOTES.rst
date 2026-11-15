GMAT Qt 0.1.0-alpha.5
====================

This early alpha includes the Qt mission editor, resource dialogs, reports,
2D and 3D plots, shared playback, map overlays, and satellite and ground-site
import tools. New Qt orbit plots use an oblique north-up Earth view. Workspace
windows open maximized, with tabs and two-column tiling available.

For a release-specific comparison with stock GMAT R2026a, see
`GMAT Qt differences <GMAT-Qt-Differences.rst>`_.

Changes in alpha.5
------------------

CCSDS OEM output now offers explicit Hermite interpolation in scripts and the
Qt ephemeris editor. It uses sampled positions and velocities, supports odd
degrees 3 through 9, and retains Lagrange as the default. Sparse endpoint,
fixed-step, long-buffer, segment-reset, single-record and strict-data checks
passed with the original numerical tolerances across all four platforms.
Windows OEM input and output preserve Unicode filenames.

The standard archives include the core, station and navigation Python bindings
for CPython 3.12, 3.13, 3.14 and 3.15. The package selects the bindings for
the running interpreter. Select the archive matching Linux x86-64, Windows x64,
macOS Apple Silicon or macOS Intel and install matching-architecture Python
when using the API. These bindings target standard GIL-enabled CPython builds.
The GUI runs without Python, and the live MCP bridge is
enabled with ``--mcp-bridge``. Earlier releases remain available unchanged.

Changes in alpha.4
------------------

The GPS smoother now copies its propagation definitions at the correct
initialization stage and writes completed MATLAB data without an extra empty
block. Buffered solver plots retain the spacecraft-relative frame state for
each historical sample. macOS bundle paths correctly locate gravity files,
function validators and bundled GMAT functions. Windows eclipse and contact
reports preserve Unicode output filenames.

The GUI tutorial replay completed all 17 tutorials supported by the standard
archives on Linux, Windows, Apple Silicon and Intel Mac. All 16 tutorials with
stock references matched their existing numerical bounds. The optional VF13ad
lunar tutorial also passed in the full-plugin Linux build. Twelve independent
eclipse/contact checks passed on each platform with unchanged geometric
references and tolerances. These are source-versioned results reused from the
recorded checks. Each new archive also receives targeted package and MCP
qualification after extraction.

The standard archives retain the CPython 3.12 core, station and navigation
bindings. The MCP bridge remains opt-in. Existing alpha.2 and alpha.3 release
files and checksums are preserved.

Changes in alpha.3
------------------

The standard archives include the core GMAT, ground-station and navigation
Python bindings, built from the same source revision as the Qt desktop.
The API supports CPython 3.12 on Linux x86-64, Windows x64, macOS Apple
Silicon (arm64) and macOS Intel (x86-64). Install CPython 3.12 with the same
architecture as the selected archive. The Qt desktop runs independently
of the Python installation.

GMAT MCP quick start
--------------------

Install the GMAT MCP 0.4.0rc1 release bundle from
https://github.com/dcajacob/GMAT-mcp/releases/tag/v0.4.0rc1 using CPython 3.12.
The bundle's installer creates a virtual environment and verifies its
dependency hashes.

.. code-block:: shell

    python3.12 /path/to/gmat-mcp-0.4.0rc1/install.py --destination /path/to/mcp-env
    /path/to/mcp-env/bin/gmat-mcp init --root /path/to/GMAT-Qt --python /path/to/python3.12 --output profiles.toml
    /path/to/mcp-env/bin/gmat-mcp --config profiles.toml doctor
    /path/to/GMAT-Qt/run-gmat --mcp-bridge /path/to/mission.script
    /path/to/mcp-env/bin/gmat-mcp --config profiles.toml serve

On Windows, use ``py -3.12`` for the installer, ``mcp-env\Scripts\gmat-mcp.exe``
for the MCP commands, the full path to the matching Python 3.12 executable
for ``--python``, and ``GMAT-Qt\bin\GmatQt.exe --mcp-bridge`` for the GUI.
On macOS, pass ``GMAT-Qt.app`` as the installation root and launch
``GMAT-Qt.app/Contents/MacOS/GmatQt --mcp-bridge``. Keep the adjacent startup
file with the app.

Use the generated ``gmat-qt`` profile with both ``create_session`` for an
isolated worker and ``attach_desktop`` after ``list_desktops`` discovers the
opted-in GUI. The same profile supplies the API used to validate live desktop
mission edits. The bridge remains opt-in through ``--mcp-bridge``.

Changes in alpha.2
------------------

Tiled windows fill the workspace after scrollbar changes. Plot controls use
their measured minimum width to avoid unnecessary wrapping. Ground-track
maps reserve space for their displayed latitude labels.

The shared targeted suite has passing evidence for all 21 cases on Linux,
Windows, Apple Silicon and Intel Mac, retaining the original assertions and
tolerances. Package checks retain the existing numerical bounds.

Installation
------------

Extract the complete archive and retain its directory structure.

* Linux x86_64 builds target Ubuntu 24.04. Start ``run-gmat``. The package
  verification records the measured glibc requirement.
* Windows x64 users start ``bin/GmatQt.exe``. The package includes the runtime
  deployment performed by Qt's Windows deployment tool.
* Apple Silicon and Intel Mac users choose the matching package and open
  ``GMAT-Qt.app``. macOS 15 or newer is required. These builds use ad-hoc code
  signatures. Gatekeeper may require approval through Privacy & Security.

Scope and known limitations
--------------------------

The saved requirement campaign covers 1,121/1,158 rows using source-versioned
evidence. Each archive has a targeted package probe and an exact source commit.
Scientific acceptance of arbitrary missions remains incomplete.

MATLAB/fmincon, embedded Python command and force-model plugins, VF13ad and
external OpenFrames plugins are excluded from these portable alpha packages.
The ``gmatpy`` API includes the core, station and navigation bindings.
Additional native plugins can be enabled in a source build with their dependencies.

The original sparse Lagrange OEM configuration retains a strict-accuracy
failure at an endpoint. The explicit Hermite option in alpha.5 passes that
case's original bounds. VF13ad forward finite differences also retain a strict exact-optimum
failure in the optional plugin's evidence. Review numerical accuracy for the
specific missions and ephemerides you use.

Online providers depend on availability and account access. A historical
Space-Track import, Propagate insertion and save/reopen were verified in the
Linux review build. The provider is not contacted by the package probe.

A reported stuck Home key affected both GMAT and other desktop applications
on the Linux review host. The initiating desktop/input fault remains unresolved.
Restarting the affected desktop session cleared the observed state.

Use ``TESTING.rst`` for build and review instructions. Include the version and
source commit from ``PACKAGE-VERIFICATION.json`` in a bug report.
