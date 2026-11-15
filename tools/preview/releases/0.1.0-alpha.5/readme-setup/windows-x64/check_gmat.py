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

