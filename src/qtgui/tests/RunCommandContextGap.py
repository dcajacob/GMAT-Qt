"""Run a command-context witness with an isolated native runtime."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile

from AcceptanceRuntime import isolate_startup


def main():
    binary, startup, fixture, key = sys.argv[1:]
    with tempfile.TemporaryDirectory(prefix="gmat-command-context-") as temporary:
        output = Path(os.environ.get("GMAT_QT_ACCEPTANCE_DIR", temporary))
        output.mkdir(parents=True, exist_ok=True)
        private_startup = isolate_startup(Path(startup), output / "runtime/bin")
        return subprocess.call(
            [binary, str(private_startup), fixture, key],
            cwd=private_startup.parent,
            env=dict(os.environ, GMAT_QT_ACCEPTANCE_DIR=str(output)),
        )


if __name__ == "__main__":
    sys.exit(main())
