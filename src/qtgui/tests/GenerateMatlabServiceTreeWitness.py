"""Retain the old interface witness while checking the current four-action menu.

Only its obsolete exact two-action predicate changes. The supplementary menu
predicate requires Open, Close, separator and enabled Engine mode in order. All
other source, process/workspace and report assertions remain byte-identical.
"""
import argparse
import hashlib
import json
from pathlib import Path


def generate(source: Path, output: Path, receipt: Path) -> None:
    original = source.read_text(encoding="utf-8")
    before = "popup->actions().size()==2"
    after = '(popup->actions().size()==4&&popup->actions()[0]==open&&popup->actions()[1]==close&&popup->actions()[2]->isSeparator()&&popup->actions()[3]->objectName()=="configureMatlabMode"&&popup->actions()[3]->isEnabled())'
    if original.count(before) != 1 or after in original:
        raise ValueError("Original exact interface menu predicate changed or is ambiguous")
    generated = original.replace(before, after, 1)
    if generated.replace(after, before, 1) != original:
        raise ValueError("Supplementary interface witness changed unrelated source")
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(generated, encoding="utf-8")
    digest = lambda text: hashlib.sha256(text.encode("utf-8")).hexdigest()
    receipt.write_text(json.dumps({
        "source": str(source), "source_sha256": digest(original),
        "generated": str(output), "generated_sha256": digest(generated),
        "before": before, "after": after, "exact_reverse_recovers_original": True,
        "scope": "Supplementary strict current menu and original process/workspace/mission assertions. Original exact two-action failure stays retained.",
        "new_requirement_credit": 0,
    }, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("receipt", type=Path)
    args = parser.parse_args()
    generate(args.source, args.output, args.receipt)
