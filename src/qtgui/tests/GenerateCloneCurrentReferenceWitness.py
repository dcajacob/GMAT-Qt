"""Pin the added conditional source mode while retaining all original edits.

Only the complete-field inventory predicate is extended by explicitly checked
invariants. Every original requested field/control must still change. Full native
property equality still compares the added field as well. Exact reverse edits
recover the untouched protected driver.
"""
import argparse
import hashlib
import json
from pathlib import Path


def generate(source: Path, output: Path, receipt: Path) -> None:
    original = source.read_text(encoding="utf-8")
    substitutions = [
        ('if(actual!=declared)std::cerr',
         'QSet<QString> expectedInventory=declared;const auto invariants=spec["invariant_fields"].toObject();'
         'if(!invariants.isEmpty()){require(key=="Barycenter"&&invariants.size()==1&&invariants["EphemerisSource"]=="MassWeighted","Unexpected supplementary invariant");'
         'require(!declared.contains("EphemerisSource")&&baseline["EphemerisSource"].toObject()["type"]=="Enumeration"&&baseline["EphemerisSource"].toObject()["value"]=="MassWeighted","Current Barycenter invariant differs");'
         'expectedInventory.insert("EphemerisSource");}if(actual!=expectedInventory)std::cerr'),
        ('QStringList(declared.begin(),declared.end()).join(\',\')',
         'QStringList(expectedInventory.begin(),expectedInventory.end()).join(\',\')'),
        ('require(actual==declared,"Clone-all actual applicable field inventory differs from declared complete set");',
         'require(actual==expectedInventory,"Clone-all actual applicable field inventory differs from declared complete set");'),
    ]
    generated = original
    for before, after in substitutions:
        if generated.count(before) != 1 or after in generated:
            raise ValueError(f"Ambiguous or changed original clone witness: {before}")
        generated = generated.replace(before, after, 1)
    restored = generated
    for before, after in reversed(substitutions):
        restored = restored.replace(after, before, 1)
    if restored != original:
        raise ValueError("Supplement changed unrelated clone assertions")
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(generated, encoding="utf-8")
    digest = lambda value: hashlib.sha256(value.encode()).hexdigest()
    receipt.write_text(json.dumps({
        "source": str(source), "source_sha256": digest(original),
        "generated": str(output), "generated_sha256": digest(generated),
        "substitutions": substitutions, "exact_reverse_recovers_original": True,
        "original_requested_field_and_control_change_assertions_unchanged": True,
        "new_requirement_credit": 0,
    }, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("receipt", type=Path)
    args = parser.parse_args()
    generate(args.source, args.output, args.receipt)
