"""Map the retained command census and check the SaveMission function restriction.

This is a read-only scope review, not whole-FRR!E477 admission or native replay.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import xml.etree.ElementTree as ET

from AuditAcceptanceRecovery import payload
from AuditScriptEventAdditionalAcceptance import oracle, check_values
from BuildNasaCoverageExtension import require
import NasaGuiCoverage as nasa


def review(repo):
    inputs = {}

    def read(path, expected=None):
        path = Path(path)
        physical = path if path.is_file() else Path(str(path) + '.gz')
        require(physical.is_file() and not physical.is_symlink(), 'Missing regular scope input ' + str(path))
        raw = payload(path)
        digest = hashlib.sha256(raw).hexdigest()
        if expected:
            require(digest == expected, 'Scope input identity differs ' + str(path))
        inputs[str(path)] = dict(payload_sha256=digest, physical=str(physical),
                                 physical_sha256=hashlib.sha256(physical.read_bytes()).hexdigest())
        return raw

    def load(path, expected=None):
        return json.loads(read(path, expected))

    root = repo / 'build/nasa-coverage/coverage95-20261101'
    qualification_path = root / 'event-starter-qualification.json'
    qualification = load(qualification_path)
    ledger = load(repo / 'build/nasa-coverage/coverage95-20261103-default-orientation/coverage-snapshot-v1.json')
    admitted = next(v for v in ledger['additions'] if v['key'] == 'FRC!E138')
    require(hashlib.sha256(read(qualification_path)).hexdigest() == admitted['qualification_sha256'],
            'Retained command qualification differs from canonical ledger')
    for path, digest in qualification['artifact_sha256'].items():
        read(path, digest)
    inventory = load(qualification['inventory'])
    require(len(inventory['rows']) == inventory['runtime_factory_count'] == 53, 'Incomplete retained command census')
    authority = nasa.inventory(repo)
    require(authority['sources'] == ledger['authority_sha256'], 'Changed NASA authority inputs')
    for path, digest in authority['sources'].items():
        read(repo / path, digest)
    original = next(v for v in authority['requirements'] if v['key'] == 'FRR!E477')
    canonical = next(v for v in ledger['remaining_original_rows']['requirements'] if v['key'] == original['key'])
    require(original['fingerprint'] == canonical['fingerprint'] and original['text'] == canonical['source_text'],
            'Changed function-command obligation')
    old_audit = load(root / 'event-extra-evidence-audit-v1.json')
    selected = [v for v in old_audit['selected'] if v['family'] in ['SaveMissionTerminal', 'SaveMissionFunction']]
    require(len(selected) == 4, 'Missing terminal/function comparison')
    comparisons = []
    for context in selected:
        case = Path(context['receipt']).parent
        result, proof, spec = [load(case / name) for name in ['result.json', 'proof.json', 'case.json']]
        require(result['status'] == 'passed' and result['returncode'] == 0 and proof['outcome'] == 'passed',
                'Retained comparison did not pass its original assertions')
        require(result['requested_visibility'] == context['mode'] and proof['family'] == spec['family'] == context['family'],
                'Retained comparison identity differs')
        emitted = [json.loads(line.split('NASA_CHECK ', 1)[1]) for line in read(case / 'test.log').decode().splitlines()
                   if 'NASA_CHECK ' in line]
        require(emitted == [proof], 'Original process output differs')
        tests = list(ET.fromstring(read(case / 'ctest.xml')).iter('testcase'))
        require(len(tests) == 1 and tests[0].get('name') == result['name'] and
                tests[0].find('failure') is None and tests[0].find('error') is None, 'Original JUnit failed')
        rows = read(case / 'results.csv').decode().strip().splitlines()
        require(len(rows) == 1, 'Original continuation row missing')
        values = [float(v) for v in rows[0].split(',')]
        check_values(values, oracle(spec))
        require(proof['observed'] == values and proof['effects']['mission_saved'] == spec['expect_saved'],
                'Original native result/effect differs')
        saved = context['family'] == 'SaveMissionTerminal'
        require(proof['effects']['mission_saved'] == saved, 'Terminal/function restriction differs')
        if not saved:
            function = read(case / 'support-Accept.gmf').decode()
            require('out = input * input + 3;' in function and 'SaveMission ' in function and values[10] == 4,
                    'Actual function command/result witness missing')
        if context['mode'] == 'visible':
            require(context['exposures'] > 0, 'Original visible comparison had no exposure')
        comparisons.append(dict(family=context['family'], mode=context['mode'], original_result=str(case / 'result.json'),
                                original_result_payload_sha256=inputs[str(case / 'result.json')]['payload_sha256'],
                                original_assertions_passed=True, mission_saved=saved, continuation=values))
    source_path = repo / 'src/base/command/SaveMission.cpp'
    source = read(source_path).decode()
    require('if (currentFunction == NULL && following == NULL)' in source and
            'Functions still cannot save the whole mission.' in source, 'Current function restriction changed')
    read(repo / 'doc/help/src/Command_SaveMission.xml')
    fixture_candidates = []
    for directory in ['mission-command-names', 'script-event-commands', 'script-event-backends', 'script-event-starters']:
        for path in sorted((repo / 'src/qtgui/tests' / directory).glob('*.json')):
            spec = load(path)
            text = spec.get('script', '')
            tokens = set(re.findall(r'^\s*([A-Za-z][A-Za-z0-9_]*)\b', text, re.M))
            tokens.update(spec.get('required_types', []))
            tokens.add(spec.get('command', ''))
            fixture_candidates.append((str(path.relative_to(repo)), tokens))
    mapping = load(repo / 'src/qtgui/tests/NasaGuiTraceability.json')
    row = next(v for v in mapping['reviews'] if v['key'] == original['key'])
    candidates = row.get('evidence', []) + row.get('partial_evidence', [])
    rows = [dict(factory_type=v['factory_type'], retained_main_or_event_disposition=v['status'],
                 fixture_candidates=[p for p, tokens in fixture_candidates if v['factory_type'] in tokens],
                 function_scope_status='source-backed-functional-restriction;not-admitted' if v['factory_type'] == 'SaveMission'
                    else 'function-context-evidence-conjunction-required;candidate-only') for v in inventory['rows']]
    return dict(status='read-only-function-command-scope-review;whole-E477-not-admitted',
                original_source=original, qualified_keys=[], rows=rows, factory_census_count=53,
                census_note='Retained 53-entry recorded runtime;not a fresh current plugin census or new NASA credit.',
                retained_command_qualification=str(qualification_path),
                retained_command_artifacts_checked=len(qualification['artifact_sha256']),
                existing_function_claim_candidates=candidates, terminal_function_comparison=comparisons,
                blocker='SaveMission exports a terminal main mission but intentionally performs no export inside a GMAT function. The guide page is incomplete;the any-command source row has no stated exception. Syntax acceptance and successful function return do not prove the command effect.',
                next_action='Reconcile the preserved SaveMission function restriction before launching an all-command passing matrix. Reuse exact function receipts on demand;main/ScriptEvent witnesses are fixture candidates,not function qualification. Do not alter the original expect_saved=false assertion without a supported-behavior decision.',
                input_bindings=inputs, native_missions_replayed=0, production_changed_files=[],
                scientific_approval=False, technical_core_approval=False,
                full_current_build_qualified=False, full_NASA_qualified=False)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    result = review(Path(__file__).resolve().parents[3])
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({k: v for k, v in result.items() if k in
                     ['status', 'qualified_keys', 'factory_census_count', 'retained_command_artifacts_checked', 'blocker', 'next_action']}))
