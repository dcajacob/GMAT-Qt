"""Give the supplementary MATLAB operation test a private shared-mode runtime."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile

from AcceptanceRuntime import isolate_startup


def main():
    binary, startup, fixture, position = sys.argv[1:]
    with tempfile.TemporaryDirectory(prefix="gmat-matlab-operation-") as directory:
        startup = isolate_startup(Path(startup), Path(directory)/"bin")
        lines = startup.read_text().splitlines()
        lines = [line for line in lines if not line.strip().startswith("MATLAB_MODE")]
        startup.write_text("\n".join(lines)+"\nMATLAB_MODE = SHARED\n")
        command = [binary, str(startup), fixture, "CallMatlabFunction", position]
        environment = dict(os.environ, LIBGL_ALWAYS_SOFTWARE="1")
        if sys.platform.startswith("linux"):
            runner = Path(__file__).with_name("RunNativeTest.py")
            command = [sys.executable, str(runner), "xvfb-run", *command]
            environment["QT_QPA_PLATFORM"] = "xcb"
        else:
            environment["GMAT_QT_NATIVE_TEST_LAUNCH"] = "1"
        return subprocess.call(command, env=environment)


if __name__ == "__main__":
    sys.exit(main())
