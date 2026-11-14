"""Retain native moving-origin and covariance timestamp diagnostics."""
import argparse
import os
import tempfile
from pathlib import Path
from RunNestedSolverValidation import run

CASES = [mode + operation for operation in
         ("SMA", "MAWrite", "SMAWrite", "RMAG", "Beta", "Covariance")
         for mode in ("Legacy", "Precise")]

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("startup", type=Path)
    args = parser.parse_args()
    output = os.environ.get("GMAT_QT_ACCEPTANCE_DIR")
    if output:
        raise SystemExit(run(args.binary, args.startup, Path(output), CASES))
    with tempfile.TemporaryDirectory(prefix="gmat-origin-epoch-") as temporary:
        raise SystemExit(run(args.binary, args.startup, Path(temporary), CASES))
