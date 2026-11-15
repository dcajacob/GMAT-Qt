GMAT Qt desktop alpha
=====================

GMAT Qt is an independent fork of NASA's
`General Mission Analysis Tool (GMAT) <https://github.com/nasa/GMAT>`_.
It adds a Qt 6 desktop interface and includes changes to the GMAT engine and
plugins. The upstream `GMAT project website <https://gmat.atlassian.net/wiki/spaces/GW/overview>`_
provides project information and documentation, and the
`NASA GMAT Git repository <https://github.com/nasa/GMAT>`_ hosts the upstream source.

Our intent is to follow upstream GMAT development while making the application
available through a consistent Qt desktop and ready-to-run packages across
Linux, Windows, and both Apple Silicon and Intel Macs. We document this fork's
engine changes and compatibility differences so users can evaluate them
alongside upstream GMAT.

Version 0.1.0-alpha.5 provides prebuilt packages for Linux x86-64, Windows x64,
and Apple Silicon and Intel Macs.

For the user-facing changes from stock GMAT R2026a, engine compatibility,
validation scope and exact source inventories, see
`GMAT Qt differences <GMAT-Qt-Differences.rst>`_.

Alpha.5 adds the Hermite endpoint fix and includes Python API bindings for
CPython 3.12, 3.13, 3.14 and 3.15. The desktop runs without Python installed.
All four Python versions passed API and MCP checks on each supported platform.

Download alpha.5
----------------

* `Linux x86-64 <https://github.com/dcajacob/GMAT-Qt/releases/download/v0.1.0-alpha.5/GMAT-Qt-0.1.0-alpha.5-linux-x86_64.tar.gz>`_
* `Windows x64 <https://github.com/dcajacob/GMAT-Qt/releases/download/v0.1.0-alpha.5/GMAT-Qt-0.1.0-alpha.5-windows-x64.zip>`_
* `Mac Apple Silicon (arm64) <https://github.com/dcajacob/GMAT-Qt/releases/download/v0.1.0-alpha.5/GMAT-Qt-0.1.0-alpha.5-macos-arm64.zip>`_
* `Mac Intel (x86-64) <https://github.com/dcajacob/GMAT-Qt/releases/download/v0.1.0-alpha.5/GMAT-Qt-0.1.0-alpha.5-macos-x64.zip>`_

The `alpha.5 release page <https://github.com/dcajacob/GMAT-Qt/releases/tag/v0.1.0-alpha.5>`_
includes release notes and verification receipts.
`SHA256SUMS <https://github.com/dcajacob/GMAT-Qt/releases/download/v0.1.0-alpha.5/SHA256SUMS>`_
lists checksums for all four archives. Earlier releases, including
`alpha.4 <https://github.com/dcajacob/GMAT-Qt/releases/tag/v0.1.0-alpha.4>`_,
remain available.

Run the desktop
---------------

Extract the complete archive and keep its directory structure together.
The packages include the compiled application, native libraries, plugins and
data. The desktop runs independently of Python.

**Linux** targets Ubuntu 24.04-compatible x86-64 systems. From the folder
containing the download, run

.. code-block:: sh

   tar -xzf GMAT-Qt-0.1.0-alpha.5-linux-x86_64.tar.gz
   cd GMAT-Qt-0.1.0-alpha.5-linux-x86_64
   ./run-gmat

**Windows** uses x64. Choose **Extract All** on the ZIP file, open the extracted
folder, then double-click ``bin\GmatQt.exe``.

**Mac** requires macOS 15 or newer. Choose the archive matching your Mac, open
the ZIP file, then open ``GMAT-Qt.app`` inside the extracted folder. Keep
``gmat_startup_qt.txt`` beside the app. The app uses an ad-hoc signature, so
first launch may require approval in **System Settings > Privacy & Security**.

Optional Python API
-------------------

Each archive includes the core GMAT, ground-station and navigation Python
bindings. Install any one of **CPython 3.12, 3.13, 3.14 or 3.15**, using a standard
GIL-enabled build with the same architecture as the archive. The ``gmatpy``
loader automatically selects the bindings matching the running interpreter.
The API is available in ``bin`` on Linux and Windows, or in
``GMAT-Qt.app/Contents/MacOS`` on Mac.

The commands below use Python 3.15. For 3.12, 3.13 or 3.14, substitute your
installed version in the interpreter commands and paths throughout the examples.

Save this example as ``check_gmat.py`` and pass the extracted installation
folder as its argument. On Mac, pass the ``GMAT-Qt.app`` path.

.. code-block:: python

   import os
   from pathlib import Path
   import sys

   root = Path(sys.argv[1]).expanduser().resolve()
   binary = root / ("Contents/MacOS" if root.suffix == ".app" else "bin")
   sys.path.insert(0, str(binary))

   dll_handles = []
   if sys.platform == "win32":
       bindings = binary / "gmatpy" / f"_py{sys.version_info.major}{sys.version_info.minor}"
       for directory in (binary, root / "plugins", bindings):
           dll_handles.append(os.add_dll_directory(str(directory)))

   import gmatpy as gmat

   os.chdir(binary)
   gmat.Setup("gmat_startup_file.txt")
   satellite = gmat.Construct("Spacecraft", "Sat")
   print(satellite.GetName())

Run it with ``python3.15 check_gmat.py /path/to/GMAT-Qt`` on Linux or Mac, or
``py -3.15 check_gmat.py C:\path\to\GMAT-Qt`` on Windows. Replace the example
path with your extracted folder or Mac app path. The example prints ``Sat``.

Optional GMAT MCP setup
-----------------------

Download and extract the `GMAT MCP 0.4.0rc1 bundle <https://github.com/dcajacob/GMAT-mcp/releases/tag/v0.4.0rc1>`_.
Its installer creates a virtual environment and installs hash-verified
dependencies. Run the installer with your chosen CPython 3.12–3.15 interpreter
and pass that interpreter's full path to ``init --python`` for the GMAT worker.

On **Intel Mac**, this MCP bundle may build its ``cryptography`` dependency from
source. Install Apple's Command Line Tools with ``xcode-select --install``,
then install ``openssl@3``, ``rust`` and ``pkg-config`` with Homebrew before
running the MCP installer. This dependency build can take several minutes.

On Linux and Mac, replace the example paths below with absolute paths. For
``--root``, use the extracted GMAT Qt folder on Linux or ``GMAT-Qt.app`` on Mac.

.. code-block:: sh

   python3.15 /path/to/gmat-mcp-0.4.0rc1/install.py --destination /path/to/mcp-env
   /path/to/mcp-env/bin/gmat-mcp init --root /path/to/GMAT-Qt --python /path/to/python3.15 --output profiles.toml
   /path/to/mcp-env/bin/gmat-mcp --config profiles.toml doctor

On Windows, run these commands in PowerShell, adjusting the paths to your
installation and Python executable.

.. code-block:: powershell

   py -3.15 C:\path\to\gmat-mcp-0.4.0rc1\install.py --destination C:\Tools\mcp-env
   C:\Tools\mcp-env\Scripts\gmat-mcp.exe init --root C:\path\to\GMAT-Qt --python C:\Python315\python.exe --output profiles.toml
   C:\Tools\mcp-env\Scripts\gmat-mcp.exe --config profiles.toml doctor

Configure your MCP client to launch the installed ``gmat-mcp`` executable with
``--config /absolute/path/to/profiles.toml serve`` as its arguments. For example,
the Linux or Mac client configuration is

.. code-block:: json

   {
     "command": "/path/to/mcp-env/bin/gmat-mcp",
     "args": ["--config", "/absolute/path/to/profiles.toml", "serve"]
   }

On Windows, use ``C:/Tools/mcp-env/Scripts/gmat-mcp.exe`` and the absolute path
to your ``profiles.toml`` in the same configuration.

For live desktop access, launch GMAT Qt with ``--mcp-bridge`` from the extracted
folder using the command for your platform.

.. code-block:: sh

   # Linux
   ./run-gmat --mcp-bridge
   # Mac
   ./GMAT-Qt.app/Contents/MacOS/GmatQt --mcp-bridge

.. code-block:: powershell

   # Windows
   .\bin\GmatQt.exe --mcp-bridge

Use the generated ``gmat-qt`` profile for isolated workers through
``create_session``. Use that same profile with ``list_desktops`` and
``attach_desktop`` to connect to the running desktop. The desktop bridge is
enabled only when launched with ``--mcp-bridge``.

Validation and project information
----------------------------------

See `RELEASE-NOTES.rst <RELEASE-NOTES.rst>`_ for scope and known limitations, and
`TESTING.rst <TESTING.rst>`_ for build instructions and reporting problems.
Alpha.5 is tagged at ``520bb641053d81d31c7dcae5fd2e0817f8458bfb``. All four
archives passed targeted package checks and Python API/MCP checks for all four
supported Python versions. Each desktop also passed startup with Python
unavailable. The published downloads match the qualified archive hashes.

The `alpha.5 qualification record <tools/preview/releases/0.1.0-alpha.5/README.rst>`_
records the saved results. The
`release receipt <https://github.com/dcajacob/GMAT-Qt/releases/download/v0.1.0-alpha.5/release-receipt.json>`_
identifies each archive's engine, GUI, bindings and relocation source revisions.
The `publication receipt <https://github.com/dcajacob/GMAT-Qt/releases/download/v0.1.0-alpha.5/publication-receipt.json>`_
records the published release and assets.

The saved acceptance campaign covers 1,121 of 1,158 original requirements.
Those results apply to their recorded source versions. Each alpha package
records its own source commit and targeted validation in
``PACKAGE-VERIFICATION.json``.

The original project overview and license remain in ``README.txt`` and
``License.txt``.
