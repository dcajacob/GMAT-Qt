"""Generate supplementary Save witnesses with an explicit legacy filename policy.

Protected drivers and case files remain untouched. Authored Save fixture strings
select the policy, and the observer reads four newer table descriptors through
the existing public native grid. Reversing the substitutions recovers the source
bytes. These executions do not qualify an original default-style oracle.
"""
import argparse
import hashlib
import json
from pathlib import Path


def generate(source: Path, output: Path, receipt: Path, kind: str) -> None:
    original = source.read_text(encoding="utf-8")
    if kind in {"Resource", "Readonly", "NamedForce"}:
        changes = [
            ("Keep; % retain Ω", "Keep {FilenameStyle = LegacyData}; % retain Ω"),
            ('selected.join(\' \')+"; % retain Ω"',
             'selected.join(\' \')+" {FilenameStyle = LegacyData}; % retain Ω"'),
        ]
    elif kind == "Attachment":
        changes = [
            ("V; % preserve Save comment", "V {FilenameStyle = LegacyData}; % preserve Save comment"),
            ('selected.join(\' \')+"; % preserve Save comment"',
             'selected.join(\' \')+" {FilenameStyle = LegacyData}; % preserve Save comment"'),
        ]
    elif kind == "ForceConsumer":
        changes = [("Save 'ExportFamily' FM Prop Sat;", "Save 'ExportFamily' FM Prop Sat {FilenameStyle = LegacyData};")]
    else:
        raise ValueError("Unknown Save witness kind: " + kind)
    if kind in {"Resource", "Readonly", "NamedForce"}:
        # These row commands are write-only StringArray descriptors. Their
        # state is available through GetDynamicDataStruct, the same independent
        # native API already used by the protected observer for older fields.
        changes.extend([
            ('"WarnBounds","CritBounds"}.contains(name)',
             '"WarnBounds","CritBounds","RowBackgroundColors","RowTextColorModes","RowWarnBounds","RowCritBounds"}.contains(name)'),
            ('else if(name=="BackgroundColor")',
             'else if(name=="BackgroundColor"||name=="RowBackgroundColors")'),
            ('else if(name=="WarnBounds")',
             'else if(name=="WarnBounds"||name=="RowWarnBounds")'),
            ('else if(name=="CritBounds")',
             'else if(name=="CritBounds"||name=="RowCritBounds")'),
            ('else if(name=="RowTextColors")',
             'else if(name=="RowTextColorModes")row.append(cell.isTextColorUserSet?"Manual":"Auto");\n    else if(name=="RowTextColors")'),
        ])
    if kind == "NamedForce":
        # These four script-createable force types expose their persistent
        # configuration through explicit literals despite read-only metadata.
        changes.append((
            'require(checked>0,"Save native comparison checked no writable properties");',
            'if(spec["configuration_only_fields"].toBool()){\n'
            '  const auto root=runtime["Subject"].toObject();const auto nativeType=root["type"].toString();\n'
            '  require(QStringList{"PointMassForce","FiniteThrust","RelativisticCorrection","ExternalModel"}.contains(nativeType)&&runtime.size()==1&&checked==0&&!spec["witnesses"].toArray().isEmpty(),"Unexpected native descriptor contract for named force");\n'
            '  for(const auto &field:root["fields"].toArray())require(field.toObject()["read_only"].toBool(),"Named force lost an ordinary writable descriptor check");\n'
            ' }else require(checked>0,"Save native comparison checked no writable properties");'
        ))
    generated = original
    for before, after in changes:
        if generated.count(before) != 1 or after in generated:
            raise ValueError("Save fixture substitution is missing or ambiguous: " + before)
        generated = generated.replace(before, after, 1)
    recovered = generated
    for before, after in reversed(changes):
        if recovered.count(after) != 1:
            raise ValueError("Save fixture reverse substitution is ambiguous")
        recovered = recovered.replace(after, before, 1)
    if recovered != original:
        raise ValueError("Save witness generation changed more than the explicit policy fixtures")
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(generated, encoding="utf-8")
    digest = lambda value: hashlib.sha256(value.encode("utf-8")).hexdigest()
    receipt.write_text(json.dumps({
        "kind": kind, "source": str(source), "source_sha256": digest(original),
        "generated": str(output), "generated_sha256": digest(generated),
        "substitutions": [{"before": a, "after": b} for a, b in changes],
        "exact_reverse_recovers_original": True,
        "scope": "Supplementary explicit LegacyData fixture and public-grid observation of newer table fields; all original assertions and case files retained. Original default-style oracle stays unchanged and unqualified.",
        "new_requirement_credit": 0,
    }, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("receipt", type=Path)
    parser.add_argument("kind", choices=["Resource", "Readonly", "Attachment", "ForceConsumer", "NamedForce"])
    args = parser.parse_args()
    generate(args.source, args.output, args.receipt, args.kind)
