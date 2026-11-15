GMAT Qt differences from stock GMAT R2026a
=========================================

GMAT Qt is a fork of GMAT with a Qt 6 desktop and changes to the mission engine,
utilities and plugins. The project's intent is to follow upstream GMAT
development while providing a consistent desktop and ready-to-run packages
across Linux, Windows, and both Mac architectures. These changes can affect
execution, validation, serialization and numerical results. Existing GMAT users
should evaluate their missions against the specific build they intend to use.

This guide compares the stock R2026a source tag
``08c2a2aae0b5fed077b2e7b4fda992ff54386c60`` with the following release sources.

* Published alpha.4 uses ``a22ef7e636203b2ccfaba51deb7fac37f5cb5ffa``.
* Alpha.5 packages use ``c7efc72151c53c951363250c5763a6aa4436f7fb``.

The exact source inventories and patches are linked below. The descriptions
summarize the main user-facing changes. They are accompanied by focused test
reports where available. Earlier reports retain the source revisions and
limitations recorded when those checks ran.

Desktop and workflow
--------------------

* The Qt 6 desktop provides a mission tree, script editing, resource forms,
  reports, 2D ground tracks, 3D orbit views and shared playback. Resource Apply
  updates the mission script. Edits support undo/redo and save/reopen workflows.
* A workspace tab strip brings scripts and output windows forward. Windows open
  maximized. Default tiling uses up to two side-by-side windows and keeps their
  full vertical extent, helping plots remain readable.
* Output controls use compact icon toolbars with tooltips and context menus.
  Window controls have dark-theme styling. Vector and matrix editors support
  pasting delimited values into component fields.
* New 3D views start with an oblique, north-up Earth view. Mouse dragging follows
  the globe. Plot controls expose camera, display, trajectory retention and
  playback settings. Shared playback and epoch-aware per-plot clocks have
  distinct controls.
* Ground tracks fill the available pane, provide selection zoom and support map
  overlays. Ground-site setup can use bundled places or an online place search.
* Spacecraft import accepts catalog/TLE/OMM data and retains its source elements
  for inspection. The import workflow can associate the spacecraft with a
  Propagate command. Space-Track account configuration supports historical data
  access when the provider and account permit it.
* Reports and mission summaries are available in the workspace. Offline Help,
  tutorial material and platform-specific launch instructions ship with the
  application.

The detailed desktop record is in `Qt6Gui.md <doc/DevelopersDocs/Qt6Gui.md>`_.

Engine behavior already present in alpha.4
-----------------------------------------

Command execution, solver trials and functions
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The engine has additional snapshot, copy and reference-rebinding paths for
resource editing, solver trial restoration, functions and nested resource
values. These cover commands, spacecraft, force models, parameters, arrays,
hardware, subscribers and plugin objects. Error handling and restoration aim to
retain the accepted state when an operation fails. Output paths distinguish
accepted mission data from trial data.

Function input/output and Global handling have changes for owned resources,
automatic objects, aliases, nested calls, wrapper lifetimes and returned values.
Validation and serialization preserve more of the original command and resource
structure. Optional function policies include RequireOutputBrackets and a
LegacyASCII source mode. Their explicit settings should be inspected when
comparing behavior with a stock installation.

Relevant implementations include GmatBase, NativeRestoration, FunctionManager,
UserDefinedFunction, SolverBranchCommand, Interpreter and the GmatFunction and
MATLAB plugins. The `core change review <doc/DevelopersDocs/Qt6CoreChangeReview.rst>`_
records the individual increments, evidence and outstanding review limitations.

Propagation, frames and stopping
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Propagation and publication carry additional spacecraft identity, frame and
precision-time information. Accepted integration-step samples can be used by
plots and output consumers. Stop-condition handling and precision-time paths
have repairs for small time tolerances and repeated or nested execution.
File-based propagation has changes for start-epoch selection, precision clocks,
interpolation boundaries and resource ownership.

Frame conversions and cached parameter references have repairs for body-relative
and coordinate-system-dependent calculations. Plot history retains the frame
state belonging to each sample, including solver history, instead of obtaining
all historical orientations from the latest spacecraft state.

These changes can alter results near time, frame or interpolation boundaries.
See `text ephemeris handling <doc/DevelopersDocs/Qt6ParityValidation/coverage95-20261104-function-text-ephemeris.rst>`_
and `ephemeris clock transitions <doc/DevelopersDocs/Qt6ParityValidation/coverage95-ephemeris-clock-transitions-implementation-20261107.rst>`_.

Orbital parameters and force-related calculations
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The fork includes numerical changes beyond resource and display handling.
Examples include stable near-singular angle calculations, Brouwer conversion
conditioning and inverse convergence, and exterior planetodetic conversion.
Anomaly assignments use the selected parameter's origin/frame, with validation
for unsupported conics and nonfinite inputs. Parameter metadata and write
access have additional explicit compatibility settings.

Force-related changes include per-spacecraft acceleration/context ownership,
SPAD drag evaluation, empty-primary-body force selection, atmosphere inputs and
coordinate-frame transformations. Users comparing reference trajectories should
include the chosen force model, epoch, coordinate system and parameterization
in their comparison.

Detailed reports include `scalar angles <doc/DevelopersDocs/Qt6ParityValidation/nasa-scalar-angles-20261030.md>`_,
`Brouwer conversions <doc/DevelopersDocs/Qt6ParityValidation/nasa-brouwer-definitions-20261030.md>`_
and `planetodetic conversion <doc/DevelopersDocs/Qt6ParityValidation/nasa-planetodetic-20261030.md>`_.

Files, reports and estimation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Ephemeris and report handling have repairs for sample chronology, duplicate
samples, segment finalization, output metadata, file rollover and saving
resource state. InsufficientDataAction provides ReduceOrder and Error policies
for supported ephemeris generation paths. The selected policy affects whether
short segments produce reduced-order output or stop with an error.

The smoother has propagation-definition ownership and initialization repairs.
Its completed MATLAB output avoids an extra empty block. Windows eclipse and
contact reports preserve Unicode filenames. macOS path handling resolves bundled
gravity files and GMAT functions correctly.

These changes are included in alpha.4. The release's tutorial and independent
event evidence is indexed by its `release receipt <tools/preview/releases/0.1.0-alpha.4/release-receipt.json>`_
and `reused-evidence record <tools/preview/releases/0.1.0-alpha.4/reused-evidence.json>`_.

Additional engine changes in alpha.5
------------------------------------

CCSDS OEM output offers an explicit Hermite option. Set
``Interpolator = Hermite`` and choose odd InterpolationOrder 3, 5, 7 or 9.
The method uses sampled position and velocity constraints. Lagrange remains the
default. Four Cartesian samples support Hermite degree seven. Reduced-order
output and strict insufficiency checks account for the available constraints
and serialized records. The Qt ephemeris editor exposes the selection.

The sparse endpoint case passes its original 2e-5 km position and 2e-8 km/s
velocity component bounds with Hermite selected. Its original Lagrange failure
is retained. Interpolation accuracy between samples still depends on the data,
spacing and method.

Windows CCSDS OEM input/output now uses UTF-8-aware native file paths. This
extends filename handling beyond the eclipse/contact report repair in alpha.4.

The `Hermite implementation report <doc/DevelopersDocs/Qt6ParityValidation/oem-hermite-20261115.rst>`_
and `four-platform qualification <doc/DevelopersDocs/Qt6ParityValidation/oem-hermite-platforms-20261114.rst>`_
include numerical results, error paths, read-back, save/reopen and source hashes.

Python, MCP and packaged plugins
--------------------------------

The release archives contain the desktop, native libraries, runtime data and
core, station and navigation Python bindings. The supported API interpreter is
CPython 3.12 with matching architecture. Python is optional for launching the GUI.
GMAT MCP uses one installation profile for isolated workers and an opted-in live
Qt desktop. Launch the desktop with ``--mcp-bridge`` to enable its bridge.

Portable alpha archives exclude MATLAB/fmincon, embedded Python command and
force-model plugins, VF13ad and external OpenFrames plugins. Source builds can
enable additional plugins with their dependencies. A script requiring one of
those plugins needs a corresponding installation. Existing third-party native
plugins may need recompilation against this fork's headers and libraries because
some object layouts and exported interfaces changed.

See `installation and API instructions <README.rst>`_ and
`release notes <RELEASE-NOTES.rst>`_ for the packaged platforms and setup.

Validation and comparison limits
--------------------------------

The saved requirements campaign records 1,121 of 1,158 original rows against
specific source versions. The standard tutorial replay and independent
eclipse/contact checks have retained numerical evidence. Fresh archive checks
exercise extraction, startup, reports, plots, persistence, Python workers and
live-desktop MCP access. Their receipts identify the actual archive and compiled
source. Results reused from an earlier revision remain labelled with that
revision.

Full scientific equivalence with stock GMAT across arbitrary missions is still
unverified. The original sparse Lagrange endpoint failure, an existing stale-panel
test failure and optional VF13ad limitations remain recorded. Hosted graphics
checks do not establish every hardware/driver combination. macOS builds use
ad-hoc signatures. Online catalog access depends on the provider and account.

Source comparison and detailed records
--------------------------------------

The release-specific inventories cover src/base, src/gmatutil and plugins.
Alpha.4 differs from stock R2026a in 772 paths in those scopes. Alpha.5 changes the
same number of paths, with additional edits within some files. Counts include
headers, build definitions and support files and do not count independent
scientific behavior changes.

* `Alpha.4 file inventory <doc/DevelopersDocs/StockComparison/alpha.4-inventory.json>`_
  records the stock and release Git blob identities and line counts for every
  changed path.
* `Alpha.4 exact engine/plugin patch <doc/DevelopersDocs/StockComparison/alpha.4-engine.patch>`_
  reproduces the source delta from stock R2026a.
* `Alpha.5 file inventory <doc/DevelopersDocs/StockComparison/alpha.5-inventory.json>`_
  and `exact engine/plugin patch <doc/DevelopersDocs/StockComparison/alpha.5-engine.patch>`_
  use the alpha.5 package source named above.
* `Core change review <doc/DevelopersDocs/Qt6CoreChangeReview.rst>`_ provides
  chronological implementation and review notes. Some older inventories in
  that document use intermediate checkout baselines.
* `Developer handoff <doc/DevelopersDocs/Qt6Handoff.md>`_ and its linked reports
  retain focused test results, source revisions, original failures and remaining
  limits.

The inventories can be regenerated with git diff against the two full commit
identifiers at the top of this guide. They describe the committed releases,
including differences already present before the Qt work began.
