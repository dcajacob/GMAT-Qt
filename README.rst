GMAT Qt desktop alpha
=====================

GMAT Qt is an independent fork of NASA's
`General Mission Analysis Tool (GMAT) <https://github.com/nasa/GMAT>`_.
It adds a Qt 6 desktop interface and includes changes to the GMAT engine and
plugins. The upstream `GMAT project website <https://gmat.atlassian.net/wiki/spaces/GW/overview>`_
provides project information and documentation, and the
`NASA GMAT Git repository <https://github.com/nasa/GMAT>`_ hosts the upstream source.

Version 0.1.0-alpha.4 provides prebuilt packages for Linux x86-64, Windows x64,
and Apple Silicon and Intel Macs.

For the user-facing changes from stock GMAT R2026a, engine compatibility,
validation scope and exact source inventories, see
`GMAT Qt differences <GMAT-Qt-Differences.rst>`_.

Download alpha.4
----------------

* `Linux x86-64 <https://github.com/dcajacob/GMAT-Qt/releases/download/v0.1.0-alpha.4/GMAT-Qt-0.1.0-alpha.4-linux-x86_64.tar.gz>`_
* `Windows x64 <https://github.com/dcajacob/GMAT-Qt/releases/download/v0.1.0-alpha.4/GMAT-Qt-0.1.0-alpha.4-windows-x64.zip>`_
* `Mac Apple Silicon (arm64) <https://github.com/dcajacob/GMAT-Qt/releases/download/v0.1.0-alpha.4/GMAT-Qt-0.1.0-alpha.4-macos-arm64.zip>`_
* `Mac Intel (x86-64) <https://github.com/dcajacob/GMAT-Qt/releases/download/v0.1.0-alpha.4/GMAT-Qt-0.1.0-alpha.4-macos-x64.zip>`_

The `alpha.4 release page <https://github.com/dcajacob/GMAT-Qt/releases/tag/v0.1.0-alpha.4>`_
includes release notes and verification receipts.
`SHA256SUMS <https://github.com/dcajacob/GMAT-Qt/releases/download/v0.1.0-alpha.4/SHA256SUMS>`_
lists checksums for all four archives.

Run the desktop
---------------

Extract the complete archive and keep its directory structure together.
The packages include the compiled application, native libraries, plugins and
data. The desktop runs independently of Python.

**Linux** targets Ubuntu 24.04-compatible x86-64 systems. From the folder
containing the download, run

.. code-block:: sh

   tar -xzf GMAT-Qt-0.1.0-alpha.4-linux-x86_64.tar.gz
   cd GMAT-Qt-0.1.0-alpha.4-linux-x86_64
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
bindings. Install **CPython 3.12** with the same architecture as the archive.
The API is available as ``gmatpy`` in ``bin`` on Linux and Windows, or in
``GMAT-Qt.app/Contents/MacOS`` on Mac.

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
       for directory in (binary, root / "plugins", binary / "gmatpy/_py312"):
           dll_handles.append(os.add_dll_directory(str(directory)))

   import gmatpy as gmat

   os.chdir(binary)
   gmat.Setup("gmat_startup_file.txt")
   satellite = gmat.Construct("Spacecraft", "Sat")
   print(satellite.GetName())

Run it with ``python3.12 check_gmat.py /path/to/GMAT-Qt`` on Linux or Mac, or
``py -3.12 check_gmat.py C:\path\to\GMAT-Qt`` on Windows. Replace the example
path with your extracted folder or Mac app path. The example prints ``Sat``.

Optional GMAT MCP setup
-----------------------

Download and extract the `GMAT MCP 0.4.0rc1 bundle <https://github.com/dcajacob/GMAT-mcp/releases/tag/v0.4.0rc1>`_.
Its installer creates a virtual environment and installs hash-verified
dependencies. Use the matching CPython 3.12 interpreter for the GMAT worker.

On **Intel Mac**, this MCP bundle builds its ``cryptography`` dependency from
source. Install Apple's Command Line Tools with ``xcode-select --install``,
then install ``openssl@3``, ``rust`` and ``pkg-config`` with Homebrew before
running the MCP installer. This dependency build can take several minutes.

On Linux and Mac, replace the example paths below with absolute paths. For
``--root``, use the extracted GMAT Qt folder on Linux or ``GMAT-Qt.app`` on Mac.

.. code-block:: sh

   python3.12 /path/to/gmat-mcp-0.4.0rc1/install.py --destination /path/to/mcp-env
   /path/to/mcp-env/bin/gmat-mcp init --root /path/to/GMAT-Qt --python /path/to/python3.12 --output profiles.toml
   /path/to/mcp-env/bin/gmat-mcp --config profiles.toml doctor

On Windows, run these commands in PowerShell, adjusting the paths to your
installation and Python executable.

.. code-block:: powershell

   py -3.12 C:\path\to\gmat-mcp-0.4.0rc1\install.py --destination C:\Tools\mcp-env
   C:\Tools\mcp-env\Scripts\gmat-mcp.exe init --root C:\path\to\GMAT-Qt --python C:\Python312\python.exe --output profiles.toml
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
Alpha.4 was built from ``a22ef7e636203b2ccfaba51deb7fac37f5cb5ffa``. All four
archives passed targeted native package, Python API and MCP checks. The public
downloads were also compared byte-for-byte with the qualified archives.

The saved acceptance campaign covers 1,121 of 1,158 original requirements.
Those results apply to their recorded source versions. Each alpha package
records its own source commit and targeted validation in
``PACKAGE-VERIFICATION.json``.

The original project overview and license remain in ``README.txt`` and
``License.txt``.
