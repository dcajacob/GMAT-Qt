"""Audit new fixed-attitude frame evidence without awarding an ambiguous row.

The original FRR!E52 fixed-offset language and two differing NASA math drafts
remain explicit. Complete guide-model coverage is not silently substituted for
the unresolved original offset semantics.
"""
import argparse
import csv
import hashlib
import json
import math
from pathlib import Path
import re

import NasaGuiCoverage as nasa
from AuditCapiAcceptance import load, require, sha
from AuditCentralBodyPropagationAcceptance import read_run
from AuditNamedFrameAcceptance import check_oracle, independent_fixture

SOURCE = 'src/qtgui/tests/NasaAttitudeDeclaredFrameTests.cpp'
FIXTURE = 'src/qtgui/tests/NasaAttitudeDeclaredFrameCases.json'
FACETS = ['actual-attitude-model-reference-sequence-popups-and-origin-axis-binding',
          'inactive-ineffective-inputs-pending-discard-Apply-exact-Undo-Redo',
          'Unicode-reopen-complete-native-graph-source-history-isolation',
          'independent-three-epoch-frame-matrix-angular-rate-and-state-witness',
          'actual-native-attitude-and-complete-read-only-output-viewer']


def independent_inputs(matrix, original, reuse):
    independent_fixture(original, reuse)
    wanted = {c['key'] for c in original['cases']} - {'MJ2000Eq', 'BodyFixed'}
    wanted.add('SpacecraftOrigin')
    require(len(matrix['cases']) == 17 and {c['key'] for c in matrix['cases']} == wanted,
            'Missing/duplicate declared attitude frame partition')
    require(matrix['input_state'] == [7000, -1200, 900, 1.5, 6.75, -2]
            and matrix['numeric_sentinel'] == 41, 'Changed inertial witness/sentinel')
    original_cases = {c['key']: c for c in original['cases']}
    for case in matrix['cases']:
        require(case['rate_bound_degrees_s'] == 5e-9, 'Original rate bound changed')
        if case['key'] == 'SpacecraftOrigin':
            require(case['axis'] == 'MJ2000Eq' and case['origin_class'] == 'Spacecraft'
                    and case['matrix_bound'] == 2e-10
                    and [s['epoch'] for s in case['samples']] == [21545, 25300.25, 30000.75],
                    'Changed independent inertial spacecraft-origin contract')
            source_samples = [dict(rows=[[1, 0, 0], [0, 1, 0], [0, 0, 1]],
                                   basis_rates=[[0] * 3 for _ in range(3)])] * 3
        else:
            old = original_cases[case['key']]
            for key in ('axis', 'origin_class', 'setup', 'matrix_bound', 'oracle'):
                require(case[key] == old[key], 'Changed source-bound frame field: ' + key)
            require([dict(epoch=s['epoch'], rows=s['rows']) for s in case['samples']]
                    == old['samples'], 'Rewritten independent geometry/epoch')
            source_samples = reuse['records'][case['oracle']]['samples'] if case['key'] in (
                'GSE', 'GSM', 'Topocentric') else [dict(basis_rates=[[0] * 3 for _ in range(3)])] * 3
        for sample, source in zip(case['samples'], source_samples):
            rows, rates = sample['rows'], sample['basis_rates']
            require(rates == source['basis_rates'], 'Rewritten independent frame derivative')
            require(len(rows) == len(rates) == 3 and all(len(r) == 3 for r in rows + rates),
                    'Incomplete basis/derivative')
            for i in range(3):
                for j in range(3):
                    require(abs(math.fsum(rows[i][k] * rows[j][k] for k in range(3))
                                - (1 if i == j else 0)) < 1e-12, 'Nonorthogonal independent basis')
                    require(abs(math.fsum(rates[i][k] * rows[j][k] + rows[i][k] * rates[j][k]
                                          for k in range(3))) < 1e-12, 'Derivative violates orthogonality')
            expected = [-math.fsum(rates[a][k] * rows[b][k] for k in range(3)) * 180 / math.pi
                        for a, b in ((2, 1), (0, 2), (1, 0))]
            require(all(abs(a - b) < 1e-18 for a, b in zip(
                expected, sample['angular_velocity_degrees_s'])), 'Independent angular-rate algebra differs')


def check_case(matrix, case, result, sources, artifacts):
    work = Path(result['log']).parent
    proof = load(work / 'proof.json')
    require([p for p in nasa.proof_records(result)
             if p.get('checkpoint') == 'attitude-declared-frame'] == [proof],
            'Saved proof differs from terminal native log')
    require(result['status'] == 'passed' and result['returncode'] == 0
            and proof['outcome'] == 'passed' and proof['case'] == case['key']
            and all(proof[k] == case[k] for k in ('axis', 'origin_class', 'rate_convention'))
            and proof['model'] == 'CoordinateSystemFixed' and proof['reference'] == 'CreatedFrame'
            and proof['actual_popups'] == 6 and proof['facets'] == FACETS,
            'Substituted/incomplete actual attitude frame contract')
    require(json.loads(sources[FIXTURE]) == matrix, 'Another fixture executed')
    for name in ('proof.json', 'case.json', 'values.csv', 'checks.json', 'initial.script',
                 'accepted.script', 'build.messages.txt', 'run.messages.txt', 'test.log', 'ctest.xml'):
        require(str(work / name) in artifacts, 'Essential native artifact omitted: ' + name)
    require(load(work / 'case.json') == case and load(work / 'checks.json') == proof['checks'],
            'Native inputs/checks differ from terminal proof')
    raw = list(csv.reader((work / 'values.csv').open()))
    require(len(raw) == 4 and raw[-1] == ['42'], 'Missing raw rows/following sentinel')
    rows = [[float(v) for v in row] for row in raw[:-1]]
    require(rows == proof['actual'] and len(proof['checks']) == 3, 'Raw/proof rows differ')
    maximum_matrix = maximum_rate = 0
    for row, sample, check in zip(rows, case['samples'], proof['checks']):
        expected = [sample['epoch']] + sum(sample['rows'], []) + sample['angular_velocity_degrees_s']
        expected += matrix['input_state'] + [41]
        require(len(row) == len(expected) == 20, 'Incomplete independent output fields')
        for i, (actual, truth) in enumerate(zip(row, expected)):
            bound = (1e-10 if i == 0 else case['matrix_bound'] if i < 10 else
                     5e-9 if i < 13 else 1e-9)
            require(math.isfinite(actual) and abs(actual - truth) <= bound,
                    'Independent attitude/clock/state/sentinel mismatch')
            if 1 <= i < 10:
                maximum_matrix = max(maximum_matrix, abs(actual - truth))
            if 10 <= i < 13:
                maximum_rate = max(maximum_rate, abs(actual - truth))
        require(check['maximum_matrix_error'] == maximum_matrix
                and check['maximum_rate_error_degrees_s'] == maximum_rate,
                'Native maximum error omitted or understated')
    require(proof['maximum_matrix_error'] == maximum_matrix
            and proof['maximum_rate_error_degrees_s'] == maximum_rate,
            'Terminal maxima differ from independent raw results')
    initial, accepted = (work / 'initial.script').read_text(), (work / 'accepted.script').read_text()
    require(initial != accepted and initial.split('BeginMissionSequence;', 1)[1]
            == accepted.split('BeginMissionSequence;', 1)[1], 'GUI Apply changed mission')
    without_satellite = lambda value: re.sub(r'^[ \t]*(?:GMAT[ \t]+)?Sat\.[^\n]*\n',
                                             '', value, flags=re.M)
    require(without_satellite(initial) == without_satellite(accepted),
            'Attitude Apply changed unrelated resource source')
    code = sources[SOURCE].decode()
    for token in ('QTest::mouseClick(combo', 'panel->discardChanges()',
                  'graph()==baseline', 'graph()==accepted', 'graph()==before',
                  'rig.source->undo()', 'rig.source->redo()',
                  'origin->IsOfType', 'nativeMatrix(i,j)', 'nativeRate[i]', 'text->isReadOnly()'):
        require(token in code, 'Required native/GUI assertion omitted: ' + token)


def audit(repo, path):
    evidence = load(path)
    require(evidence['contract_kind'] == 'source-versioned-bounded-declared-frame-attitude-evidence'
            and evidence['qualified_keys'] == [] and evidence['goal_complete'] is False,
            'Bounded evidence silently awards original-row credit')
    inventory = nasa.inventory(repo)
    require(evidence['authority_sha256'] == inventory['sources'], 'Original NASA authority changed')
    for name, digest in evidence['artifact_sha256'].items():
        require(sha(name) == digest, 'Changed retained artifact: ' + name)
    matrix, original, reuse = (load(evidence[name]) for name in ('fixture', 'original_frames', 'oracle_reuse'))
    independent_inputs(matrix, original, reuse)
    for record in reuse['records']:
        check_oracle(record)
    runs = {name: read_run(repo, spec, inventory, evidence['artifact_sha256'], matrix['physical_inputs'])
            for name, spec in evidence['runs'].items()}
    cases = {c['key']: c for c in matrix['cases']}
    require(set(evidence['selected']) == {'headless', 'visible'}, 'Required execution mode omitted')
    for mode, selected in evidence['selected'].items():
        require(len(selected) == 17 and {s['case'] for s in selected} == set(cases),
                'Missing/duplicate case in execution mode')
        for entry in selected:
            receipt, results, sources = runs[entry['run']]
            name = 'QtGui.NativeNasaAttitudeDeclaredFrame' + entry['case']
            require(receipt['mode'] == mode and entry['test'] == name, 'Substituted mode/test')
            check_case(matrix, cases[entry['case']], results[name], sources, evidence['artifact_sha256'])
            for physical, digest in matrix['physical_inputs'].items():
                require(load(evidence['runs'][entry['run']]['before'])['sources'].get(physical)
                        == receipt['source_hashes'].get(physical) == digest, 'Unbound physical input')
    require(evidence['original_fixed_offset_semantics_status'] == 'unresolved'
            and evidence['whole_FRR_E52_qualified'] is False,
            'Historical offset obligation hidden behind current guide-model checks')
    return dict(status='bounded-evidence-passed; original-row-gap-open', new_cases=17,
                contexts=34, independent_report_rows=102, matrix_components=918,
                rate_components=306, inertial_state_components=612,
                original_row='FRR!E52', new_requirement_credit=[], goal_complete=False)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, default=Path(__file__).resolve().parents[3])
    parser.add_argument('--evidence', type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(audit(args.repo.resolve(), args.evidence)))
