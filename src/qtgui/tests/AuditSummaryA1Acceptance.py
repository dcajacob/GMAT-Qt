"""Admit only original E211/E212 from optional cached A1 summary display.

Independent epoch truth uses literal initial/assigned epochs and elapsed days.
No inverse conversion of another displayed time scale supplies the oracle.
"""
import argparse
import csv
import json
import math
from pathlib import Path
import re

import NasaGuiCoverage as nasa
from AuditCapiAcceptance import load, require, sha
from AuditCentralBodyPropagationAcceptance import read_run

KEYS = ['All Others!E211', 'All Others!E212']
SOURCE = 'src/qtgui/tests/NasaSummaryA1Tests.cpp'
LEGACY = 'src/qtgui/tests/NasaCommandSummaryTests.cpp'
PRODUCTION = ['src/base/command/GmatCommand.cpp', 'src/base/command/GmatCommand.hpp',
              'src/qtgui/InspectionDialog.cpp', 'src/qtgui/InspectionDialog.hpp',
              'src/qtgui/MainWindow.cpp', 'src/qtgui/MainWindow.hpp']
CHECKPOINTS = ['Before', 'AfterFirst', 'InBranch', 'AfterSecond', 'AfterAssignments']
TRUTH = [[21545.25, 21600.875, 22000.125],
         [21545.25 + 20/86400, 21600.875, 22000.125],
         [21545.25 + 25/86400, 21600.875, 22000.125],
         [21545.25 + 25/86400, 21600.875 - 10/86400, 22000.125],
         [23000.375, 24000.625, 22000.125]]


def expected_tags():
    tags = {'panel-before': 0}
    for name, stage in [*zip(CHECKPOINTS, range(5)), ('first-propagate', 1),
                        ('branch-propagate', 2), ('backward-propagate', 3),
                        ('old-cache-revisited', 0)]:
        tags[name] = tags[name + '-ecliptic'] = stage
    for name, stage in [*zip(CHECKPOINTS, range(5)), ('FirstArc', 1),
                        ('BranchArc', 2), ('BackArc', 3)]:
        tags['mission-' + name] = stage
        if name not in CHECKPOINTS:
            tags['physics-' + name] = stage
    return tags


def validate_observations(observations):
    tags = expected_tags()
    require(len(observations) == len(tags) == 30 and
            {r['tag'] for r in observations} == set(tags), 'Incomplete A1 command/frame/mission matrix')
    for row in observations:
        stage = tags[row['tag']]
        require(row['stage'] == stage and len(row['values']) == 3 and
                {v['spacecraft'] for v in row['values']} == {'Alpha', 'Beta', 'Gamma'},
                'Wrong A1 command stage/spacecraft identities')
        for value in row['values']:
            expected = TRUTH[stage][['Alpha', 'Beta', 'Gamma'].index(value['spacecraft'])]
            require(value['expected'] == expected and math.isfinite(value['actual']) and
                    abs(value['actual'] - expected) <= 6e-11,
                    'A1 epoch differs from independent literal/elapsed-day oracle')


def qualify(repo, path, inventory):
    q = load(path)
    require(q['contract_kind'] == 'source-versioned-optional-cached-A1-summary' and
            q['qualified_keys'] == KEYS and q['parent_children'] == [KEYS[1]],
            'Changed original A1 summary scope')
    require(q['authority_sha256'] == inventory['sources'] and
            all(q.get(k) is False for k in ('technical_core_approval',
                'full_current_build_qualified', 'full_NASA_qualified')), 'A1 summary exceeds its scope')
    required = {q['review'], q['independent_review']}
    for spec in q['runs'].values():
        required.update(spec.values())
        reuse = load(spec['reuse'])
        required.update(reuse[k] for k in ('receipt', 'reference', 'delta'))
    require(required <= set(q['artifact_sha256']), 'A1 evidence omitted from artifact seal')
    for p, digest in q['artifact_sha256'].items():
        require(sha(p) == digest, 'Changed A1 summary artifact: ' + p)
    known = {r['key']: r for r in inventory['requirements']}
    review = load(q['review'])
    require(review['parent_children'] == [KEYS[1]] and len(review['reviews']) == 2 and
            {r['key'] for r in review['reviews']} == set(KEYS), 'Incomplete finite A1 format conjunction')
    for row in review['reviews']:
        require(row['source_text'] == known[row['key']]['text'] and
                row['fingerprint'] == known[row['key']]['fingerprint'] and row['rationale'],
                'Original A1 summary obligation changed')
    require(set(q['selected']) == {'headless', 'visible'}, 'Missing A1 execution mode')
    frozen = {}
    for name, spec in q['runs'].items():
        receipt, results, sources = read_run(repo, spec, inventory, q['artifact_sha256'], {})
        frozen[name] = (receipt, results, sources)
    execution_versions = []
    for mode, name in q['selected'].items():
        receipt, results, sources = frozen[name]
        require(receipt['mode'] == mode and 'QtGui.NasaSummaryA1' in results,
                'Wrong actual A1 test or execution mode')
        result = results['QtGui.NasaSummaryA1']; work = Path(result['log']).parent
        proof = load(work / 'proof.json')
        require(nasa.proof_records(result) == [proof] and proof['checkpoint'] == 'summary-a1-cached-epochs'
                and proof['outcome'] == 'passed' and proof['spacecraft'] == 3 and
                proof['command_views'] == 12 and proof['independent_report_checks'] == 15 and
                proof['display_checks'] == 90 and proof['exports'] == 9 and
                proof['native_exception_restored'] is True and
                proof['default_summary_unchanged'] is True and proof['legacy_assertions_unchanged'] is True,
                'Missing actual complete native A1 checkpoint')
        needed = [work / n for n in ('proof.json', 'observations.json', 'epochs.csv',
                  'mission.script', 'unexecuted-default.txt', 'unexecuted-enabled.txt',
                  'mission-a1.txt', 'mission-physics-a1.txt')]
        needed += [work / (tag + '.txt') for tag in expected_tags()]
        require(all(str(p) in q['artifact_sha256'] for p in needed), 'Essential A1 native results omitted')
        observations = load(work / 'observations.json'); validate_observations(observations)
        rows = [[float(v) for v in row] for row in csv.reader((work / 'epochs.csv').open())]
        require(len(rows) == 5 and all(len(r) == 3 and all(math.isfinite(v) and
                abs(v-e) <= 1e-10 for v, e in zip(r, truth)) for r, truth in zip(rows, TRUTH)),
                'Native epoch report differs from independent initial/elapsed/assignment truth')
        for row in observations:
            text = (work / (row['tag'] + '.txt')).read_text()
            parts = re.split(r'Spacecraft\s*:\s*(\w+)\s*\n', text)
            require(len(parts) == 7 and set(parts[1::2]) == {'Alpha', 'Beta', 'Gamma'},
                    'Raw A1 text omitted/duplicated a spacecraft')
            for name, body in zip(parts[1::2], parts[2::2]):
                values = re.findall(r'^\s*A1 Epoch:\s*([-+\d.eE]+)\s*$', body, re.M)
                expected = next(v['actual'] for v in row['values'] if v['spacecraft'] == name)
                require(len(values) == 1 and float(values[0]) == expected, 'A1 proof differs from actual text')
        default = (work / 'unexecuted-default.txt').read_bytes()
        require((work / 'unexecuted-enabled.txt').read_bytes() == default and
                b'Propagate Command: UnusedArc' in default and b'did not execute' in default and
                b'A1 Epoch:' not in default, 'Unexecuted summary fabricated a cached A1 epoch')
        for token in ('epochData[i]', 'thread_local bool includeA1SummaryEpoch = false',
                      '~RestorePresentation() { includeA1SummaryEpoch = previous; }'):
            require(token in sources[PRODUCTION[0]].decode(), 'Missing cached render-context implementation')
        code = sources[SOURCE].decode()
        for token in ('missionCommandLabel(nodes[i].statement).name==name',
                      'missionValues(false)', 'missionValues(true)',
                      'cached->GetSummaryWithA1Epochs()', 'QTest::mouseClick(a1,Qt::LeftButton)',
                      'strip(enabled)==original', 'read(report)==raw'):
            require(token in code, 'Missing original A1 assertion: ' + token)
        execution_versions.append({p: sources[p] for p in [SOURCE, LEGACY, *PRODUCTION]})
    require(execution_versions[0] == execution_versions[1], 'New A1 assertions/production differ across modes')
    visible, results, sources = frozen[q['selected']['visible']]
    for test, checkpoint in [('QtGui.NasaCommandSummaryFields', 'command-summary-fields'),
                             ('QtGui.Inspection', None)]:
        require(test in results and results[test]['status'] == 'passed' and results[test]['returncode'] == 0,
                'Missing affected original summary/inspection regression')
        if checkpoint:
            records = nasa.proof_records(results[test])
            require(len(records) == 1 and records[0]['checkpoint'] == checkpoint and
                    records[0]['a1_gap'] is True and records[0]['compared_values'] == 176,
                    'Original default-four-format assertions changed or did not execute')
    require(sha(q['legacy_before']) == q['legacy_before_sha256'] ==
            __import__('hashlib').sha256(sources[LEGACY]).hexdigest(), 'Original legacy assertions changed')
    independent = load(q['independent_review'])
    require(independent['status'] == 'passed' and independent['qualified_keys'] == KEYS and
            independent['source_sha256'] == {p: __import__('hashlib').sha256(sources[p]).hexdigest()
                for p in [SOURCE, LEGACY, *PRODUCTION]}, 'Independent A1 source/scope review missing or stale')
    return q, KEYS


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--repo', type=Path, default=Path(__file__).resolve().parents[3])
    p.add_argument('--qualification', type=Path, required=True)
    a = p.parse_args(); _, keys = qualify(a.repo.resolve(), a.qualification, nasa.inventory(a.repo.resolve()))
    print(json.dumps(dict(status='passed', qualified_keys=keys, new_native_executions=0)))
