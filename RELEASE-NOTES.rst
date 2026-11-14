GMAT Qt 0.1.0-alpha.2
====================

This early alpha includes the Qt mission editor, resource dialogs, reports,
2D and 3D plots, shared playback, map overlays, and satellite and ground-site
import tools. New Qt orbit plots use an oblique north-up Earth view. Workspace
windows open maximized, with tabs and two-column tiling available.

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

MATLAB/fmincon, Python interfaces, VF13ad and external OpenFrames plugins are
excluded from these portable alpha packages. Native plugins can be enabled
in a separate source build with their dependencies.

Sparse OEM interpolation has a retained strict-accuracy failure at an end
point. VF13ad forward finite differences also retain a strict exact-optimum
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
