"""Observe worksheet Save filenames or explicitly select legacy tree saves.

The original command-tree driver, fixtures, numeric oracles and assertions stay
unchanged. Every substitution must reverse to the original source exactly.
"""
import argparse
import hashlib
import json
from pathlib import Path


def generate(source: Path, output: Path, receipt: Path, policy: str) -> None:
    original = source.read_text(encoding="utf-8")
    if policy == "Worksheet":
        substitutions = [('files.filePath("Quantity.Variable.data")', 'files.filePath("Quantity.Variable")')]
    elif policy == "LegacyData":
        substitutions = [('statement="Save \'"+label+"\' Quantity;";',
                          'statement="Save \'"+label+"\' Quantity {FilenameStyle = LegacyData};";')]
    else:
        raise ValueError("Unknown Save filename policy")
    generated = original
    for before, after in substitutions:
        if generated.count(before) != 1 or after in generated:
            raise ValueError("Missing or ambiguous original Save tree source")
        generated = generated.replace(before, after, 1)
    restored = generated
    for before, after in reversed(substitutions):
        restored = restored.replace(after, before, 1)
    if restored != original:
        raise ValueError("Supplement changed unrelated command-tree code")
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(generated, encoding="utf-8")
    digest = lambda text: hashlib.sha256(text.encode()).hexdigest()
    receipt.write_text(json.dumps({
        "source": str(source), "source_sha256": digest(original),
        "generated": str(output), "generated_sha256": digest(generated),
        "policy": policy, "substitutions": substitutions,
        "exact_reverse_recovers_original": True,
        "all_original_assertions_and_numeric_oracles_unchanged": True,
        "new_requirement_credit": 0,
    }, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("receipt", type=Path)
    parser.add_argument("policy", choices=["Worksheet", "LegacyData"])
    args = parser.parse_args()
    generate(args.source, args.output, args.receipt, args.policy)
