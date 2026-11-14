"""Retain current default/explicit SPK start-boundary and spacecraft-epoch outcomes."""
import argparse
import os
import tempfile
from pathlib import Path
from RunNestedSolverValidation import run

CASES = ["NativeDefaultStart", "NativeEphemStart",
         "ExactlyOneGroupEphemStart", "NativeFromSpacecraft",
         "PreciseNativeDefaultStart", "PreciseNativeEphemStart",
         "PreciseNativeFromSpacecraft", "ReaderPrecision", "ReaderCoverageGuards"]

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("startup", type=Path)
    args = parser.parse_args()
    output = os.environ.get("GMAT_QT_ACCEPTANCE_DIR")
    if output:
        raise SystemExit(run(args.binary, args.startup, Path(output), CASES))
    with tempfile.TemporaryDirectory(prefix="gmat-spk-start-") as directory:
        raise SystemExit(run(args.binary, args.startup, Path(directory), CASES))
