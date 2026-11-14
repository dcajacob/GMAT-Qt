"""Retain isolated native frame-parameter epoch round-trip diagnostics."""
import argparse
import os
import tempfile
from pathlib import Path
from RunNestedSolverValidation import run

CASES = ["LegacyXIncrement", "PreciseXIdempotent", "PreciseXIncrement",
         "LegacyVXIncrement", "PreciseVXIncrement"]

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("startup", type=Path)
    args = parser.parse_args()
    output = os.environ.get("GMAT_QT_ACCEPTANCE_DIR")
    if output:
        raise SystemExit(run(args.binary, args.startup, Path(output), CASES))
    with tempfile.TemporaryDirectory(prefix="gmat-orbit-epoch-") as temporary:
        raise SystemExit(run(args.binary, args.startup, Path(temporary), CASES))
