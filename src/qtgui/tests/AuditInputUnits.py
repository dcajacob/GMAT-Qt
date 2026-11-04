#!/usr/bin/env python3
"""Inventory shipped field documentation without inferring undocumented units.

This deliberately assigns no NASA row credit: field tables are only one part
of the whole-input obligation, and expressions can inherit units at runtime.
"""
import argparse
import hashlib
import json
from pathlib import Path
import xml.etree.ElementTree as ET

NS = {'d': 'http://docbook.org/ns/docbook'}


def text(node):
    return ' '.join(''.join(node.itertext()).split()) if node is not None else ''


def inventory(repo):
    sources, fields, errors = {}, [], []
    for path in sorted((repo / 'doc/help/src').glob('*.xml')):
        if not path.name.startswith(('Resource_', 'Command_', 'System_')):
            continue
        relative = str(path.relative_to(repo))
        sources[relative] = hashlib.sha256(path.read_bytes()).hexdigest()
        try:
            root = ET.parse(path).getroot()
        except ET.ParseError as error:
            errors.append(dict(source=relative, error=str(error)))
            continue
        for index, row in enumerate(root.findall('.//d:row', NS)):
            entries = row.findall('d:entry', NS)
            metadata = {text(v.find('d:term', NS)): text(v.find('d:listitem', NS))
                        for v in row.findall('.//d:varlistentry', NS)}
            if not entries or not metadata:
                continue
            fields.append(dict(source=relative, row_index=index,
                               field=text(entries[0]), metadata=metadata,
                               gui='GUI' in metadata.get('Interfaces', '').upper(),
                               explicit_units=bool(metadata.get('Units', '').strip())))
    gui = [f for f in fields if f['gui']]
    gaps = [f for f in gui if not f['explicit_units']]
    return dict(schema_version=1, source_sha256=sources, fields=fields,
                summary=dict(parsed_documents=len(sources)-len(errors),
                             parse_errors=len(errors), documented_fields=len(fields),
                             gui_tagged_fields=len(gui),
                             gui_fields_with_explicit_units=len(gui)-len(gaps),
                             gui_fields_without_explicit_units=len(gaps)),
                gui_unit_review_queue=gaps, parse_errors=errors,
                qualified_keys=[], all_input_documentation_qualified=False,
                limits=['Counts are table entries, including duplicate/legacy/template rows, '
                        'not a census of distinct active GUI inputs.',
                        'No runtime field-to-document completeness asserted.',
                        'Missing Units metadata is a review item, not automatically a defect.',
                        'Narrative-only inputs, conditional/plugin fields, commands and expression '
                        'dimensions still require reconciliation; None/N/A is not independently '
                        'certified as dimensionless.'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, default=Path(__file__).resolve().parents[3])
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    report = inventory(args.repo.resolve())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report['summary']))
    return bool(report['parse_errors'])


if __name__ == '__main__':
    raise SystemExit(main())
