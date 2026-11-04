"""Check source-bound concrete/parametric control-domain review integrity.

Human-reviewed meanings remain inspectable alongside the frozen sources. A
matching route/hash is not, by itself, an all-input qualification decision.
"""
import argparse
import hashlib
import json
from pathlib import Path
from lxml import etree
from InputConstructionInventory import inventory
from AuditInputConstructionBindings import audit as binding_audit


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def require(condition, message):
    if not condition:
        raise ValueError(message)


def audit(repo, review_path):
    review = json.loads(review_path.read_text())
    manifest_path = repo / 'src/qtgui/tests/InputConstructionBindings.json'
    require(sha(manifest_path) == review['manifest_sha256'], 'Changed source/domain binding manifest')
    manifest = json.loads(manifest_path.read_text())
    measured = inventory(repo)
    routes = binding_audit(repo, measured, manifest)
    actual_keys = {r['key'] for r in measured['records']}
    families = {g['key']: g for g in manifest['groups']}
    rows = {g['family']: g for g in review['records']}
    require(len(rows) == len(review['records']) and set(rows) == set(families), 'Missing/duplicate control family')
    covered, source_files = [], set()
    ns = {'d': 'http://docbook.org/ns/docbook'}
    for family, row in rows.items():
        group = families[family]
        require(bool(row['concrete_or_parametric_field_domains']) and bool(row['unit_and_mode_rule']), 'Missing concrete field/unit rule')
        require(row['unit_policy_text'] == group['units'], 'Unreviewed policy change: ' + family)
        require({s['source'] for s in row['source_evidence']} == set(group['sources']), 'Missing family source evidence')
        expected_keys = {r['key'] for r in manifest['bindings'] if r['section'] == row['section']}
        require(len(row['construction_keys']) == len(set(row['construction_keys'])) and set(row['construction_keys']) == expected_keys, 'Wrong construction-domain keys')
        covered.extend(row['construction_keys'])
        for source in row['source_evidence']:
            path = repo / source['source']
            require(sha(path) == source['sha256'], 'Changed reviewed source: ' + str(path))
            lines = path.read_text().splitlines()
            for excerpt in source['field_declaration_excerpts']:
                require(lines[excerpt['line'] - 1].strip() == excerpt['text'], 'Changed field declaration excerpt')
            source_files.add(source['source'])
        document = etree.parse(str(repo / row['documentation']))
        sections = document.xpath('//*[@xml:id=$key]', key=row['section'], namespaces={'xml': 'http://www.w3.org/XML/1998/namespace'})
        require(len(sections) == 1, 'Missing documented unit section')
        para = sections[0].find('./d:variablelist/d:varlistentry[d:term="Units"]/d:listitem/d:para', ns)
        require(para is not None and ''.join(para.itertext()) == row['unit_policy_text'], 'Changed shipped unit description')
    require(len(covered) == len(set(covered)) and set(covered) == actual_keys, 'Uncovered or duplicate compiled construction')
    require(review['families'] == len(rows) and review['construction_templates'] == len(covered), 'Review count mismatch')
    require(not review['qualified_keys'] and not review['all_input_documentation_qualified'], 'Source-domain review cannot silently award E12 credit')
    return dict(status='passed', scope='Source/field-domain review integrity only; the full E12 evidence conjunction remains a separate audit.', families=len(rows), construction_templates=len(covered), source_files=len(source_files), current_source_keys_complete=True, current_documentation_matches_review=True, supplemental_nonquantity_inputs=review['supplemental_nonquantity_inputs'], review_sha256=sha(review_path), manifest_sha256=sha(manifest_path), verifier_sha256=sha(Path(__file__)), source_sha256=measured['source_sha256'], qualified_keys=[], all_input_documentation_qualified=False)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, default=Path(__file__).resolve().parents[3])
    parser.add_argument('--review', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    result = audit(args.repo.resolve(), args.review.resolve())
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print('PASS', result['families'], 'source-domain families;', result['construction_templates'], 'construction keys;', result['source_files'], 'source files.')
