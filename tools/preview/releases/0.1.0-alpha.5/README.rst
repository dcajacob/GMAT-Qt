GMAT Qt 0.1.0-alpha.5 qualification
===================================

The prepared archives include the Qt desktop and the core, station and
navigation API bindings for standard GIL-enabled CPython 3.12, 3.13, 3.14 and
3.15. Install any one of these versions with the same architecture as the
archive. The gmatpy loader selects its matching bindings automatically.

The Qt desktop and normal mission execution run without Python installed.
Python is required when using the Python API or the GMAT MCP server and its
isolated workers. The desktop bridge remains opt-in through --mcp-bridge.

All sixteen platform/Python combinations passed on freshly extracted archives.
The checks cover all three API modules, native exception translation, MCP
installation, discovery, profile generation, doctor, isolated propagation,
live-desktop attachment, mission editing, execution and result retrieval.
Each platform also passed package startup, plots, reports, save/reopen,
sparse Hermite output, independent eclipse/contact results and Python-free
GUI startup. Numerical assertions and tolerances were retained.

The archives are prepared for review. Publication remains a separate action.
Published alpha.4 assets and checksums were checked and remain unchanged.

Source identities and saved evidence
-------------------------------------

The engine baseline is c7efc72151c53c951363250c5763a6aa4436f7fb. The Windows
engine was compiled from 27249d0b5d12ea156115bc577abcf1cf5a68324a. Both Mac
engines were compiled from e4179175aed826b9ff79650371789c26a47ff03d. Their
reviewed source differences are recorded in Alpha5NativeReuse.json and the
original Hermite reports. The Linux engine was compiled from the baseline.

The final GUI includes the ScriptEditor margin recursion repair. The API
wrappers were compiled against the archived engines for each Python version.
The exact GUI, bindings and relocation commits differ between platforms and
are listed in `release-receipt.json <release-receipt.json>`_. This record resolves
the abbreviated Windows bindings commit retained in its original receipt.
Original receipts remain unchanged.

`SHA256SUMS <SHA256SUMS>`_ identifies the final archive bytes. Each platform
folder contains its original verification receipt and per-version MCP records.
The release receipt identifies the completing GitHub Actions runs. Earlier
failures remain in those Actions runs and the local evidence folder.

The Linux completion run reused its completed package, API, MCP and numerical
checks and performed only its missing Python-free launch. Both Mac archives
required relocation to a single canonical engine library path. Windows
packaging reused completed bindings after resolving SDK and manifest-path
issues. No full regression suite was run.

Python API and MCP quick start
--------------------------------

Extract the entire archive. Use run-gmat on Linux, bin/GmatQt.exe on Windows,
or GMAT-Qt.app on Mac. All required GMAT native libraries and standard plugins
are included. The Linux runtime targets Ubuntu 24.04-compatible systems. The
Mac packages require macOS 15 or newer and use ad-hoc signatures.

For Python, add the extracted bin directory to sys.path, then import gmatpy.
On Mac, use GMAT-Qt.app/Contents/MacOS. Keep the complete archive directory
structure intact. Existing README Python examples apply to any supported
interpreter version by replacing python3.12 or py -3.12 with that version.

Install the `GMAT MCP 0.4.0rc1 bundle <https://github.com/dcajacob/GMAT-mcp/releases/tag/v0.4.0rc1>`_
with the chosen interpreter. Its installer verifies dependency hashes. On Intel
Mac, its cryptography dependency may build from source and requires Apple's
Command Line Tools plus Homebrew openssl@3, rust and pkg-config.

.. code-block:: sh

   python3.15 /path/to/gmat-mcp-0.4.0rc1/install.py --destination /path/to/mcp-env
   /path/to/mcp-env/bin/gmat-mcp init --root /path/to/GMAT-Qt --python /path/to/python3.15 --output profiles.toml
   /path/to/mcp-env/bin/gmat-mcp --config profiles.toml doctor
   /path/to/GMAT-Qt/run-gmat --mcp-bridge

For Windows, use py -3.15 and the environment's Scripts/gmat-mcp.exe. On Mac,
use the app as --root and launch GMAT-Qt.app/Contents/MacOS/GmatQt --mcp-bridge.
The same generated profile supports create_session workers and attach_desktop.

Remaining limitations
----------------------

The original sparse Lagrange endpoint accuracy failure is retained. Hermite
provides the tested explicit alternative. An existing stale-panel test failure
and optional VF13ad limitations remain separately recorded. Hosted graphics
checks do not establish every physical GPU/driver combination. Mac archives
use ad-hoc signatures. The saved requirements campaign remains source-versioned.

This is an independent fork of NASA's
`GMAT repository <https://github.com/nasa/GMAT>`_. The intent is to follow
upstream development and provide cross-platform packages. See the upstream
`GMAT project <https://gmat.atlassian.net/wiki/spaces/GW/overview>`_ and the
fork's `detailed differences <../../../../GMAT-Qt-Differences.rst>`_.
