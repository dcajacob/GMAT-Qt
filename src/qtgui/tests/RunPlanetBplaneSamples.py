"""Retain isolated planet/B-plane parameter sample and failure diagnostics."""
import argparse
import os
import tempfile
from pathlib import Path
from RunNestedSolverValidation import run

CASES = ["PlanetLegacy", "PlanetPrecise", "PlanetPublished", "BplaneLegacy",
         "BplanePrecise", "BplanePublished", "BplaneFrameFailure", "NestedPublished",
         "PlanetLocal", "BplaneLocal", "PlanetLocalPublished", "BplaneLocalPublished"]

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("startup", type=Path)
    args = parser.parse_args()
    output = os.environ.get("GMAT_QT_ACCEPTANCE_DIR")
    if output:
        raise SystemExit(run(args.binary, args.startup, Path(output), CASES))
    with tempfile.TemporaryDirectory(prefix="gmat-planet-bplane-samples-") as directory:
        raise SystemExit(run(args.binary, args.startup, Path(directory), CASES))
