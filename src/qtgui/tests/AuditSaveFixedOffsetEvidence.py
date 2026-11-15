"""Replay native fixed-offset Save persistence without awarding whole-row credit."""
import argparse
import collections
import hashlib
import json
import math
from pathlib import Path
import re
import tarfile

import NasaGuiCoverage as nasa
from AuditSaveResourceEvidence import check_case, check_witnesses, equal
from AuditSaveOrbitModeEvidence import audit as prior_audit

REPO = Path(__file__).resolve().parents[3]
FIXTURE = 'src/qtgui/tests/NasaSaveFixedOffsetCases.json'
DRIVER = 'src/qtgui/tests/NasaSaveResourceTests.cpp'
PREFIX = 'QtGui.NasaSaveFixedOffset'
SHA = lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest()
LOAD = lambda p: json.loads(Path(p).read_text())


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def independent_truth(case):
    """Prescribed inertial/ecliptic/Z-pole frames, independently of GMAT."""
    key = case['key']
    require(key in {'Identity', 'Named', 'Body', 'HalfTurn'}, 'Substituted Save case')
    sine = math.sin(math.radians(18))
    q = [sine * v / math.sqrt(14) for v in [1, 2, 3]] + [math.cos(math.radians(18))]
    raw = ([0, 0, 0, 1] if key == 'Identity' else [0, 1, 0, 0] if key == 'HalfTurn'
           else [-7*v for v in q] if key == 'Body' else q)
    literal = re.findall(r'^Subject\.OffsetQuaternion = \[([^]]+)\];$', case['setup'], re.M)
    require(len(literal) == 1 and equal([float(v) for v in literal[0].split()], raw),
            'Input quaternion no longer matches independent literal partition')
    norm = math.hypot(*raw)
    x, y, z, w = normalized = [v/norm for v in raw]
    # Scalar-last quaternion, inertial/frame components into body components.
    rotation = [[w*w+x*x-y*y-z*z, 2*(x*y+w*z), 2*(x*z-w*y)],
                [2*(x*y-w*z), w*w-x*x+y*y-z*z, 2*(y*z+w*x)],
                [2*(x*z+w*y), 2*(y*z-w*x), w*w-x*x-y*y+z*z]]
    if key in {'Identity', 'HalfTurn'}:
        a = math.radians(84381.448/3600)
        basis = [[1, 0, 0], [0, math.cos(a), math.sin(a)], [0, -math.sin(a), math.cos(a)]]
    elif key == 'Body':
        a = math.radians(30)
        basis = [[math.cos(a), math.sin(a), 0], [-math.sin(a), math.cos(a), 0], [0, 0, 1]]
        fields = dict(re.findall(r'^AxisBody\.(\w+) = ([^;]+);$', case['setup'], re.M))
        expected = {'OrientationEpoch': 21545, 'SpinAxisRAConstant': 270, 'SpinAxisRARate': 0,
                    'SpinAxisDECConstant': 90, 'SpinAxisDECRate': 0,
                    'RotationConstant': 30, 'RotationRate': 120}
        require(all(float(fields[k]) == v for k, v in expected.items()), 'Prescribed rotating frame changed')
    else:
        basis = [[int(i == j) for j in range(3)] for i in range(3)]
    matrix = [[math.fsum(rotation[i][k]*basis[k][j] for k in range(3)) for j in range(3)] for i in range(3)]
    rates = [row[2]*120/86400 if key == 'Body' else 0 for row in rotation]
    require(equal(case['expected_offset_quaternion'], normalized)
            and equal(case['expected_matrix'], matrix) and equal(case['expected_rate_degrees_s'], rates),
            'Fixture truth differs from independent quaternion/frame geometry')
    require(case['before_save'] == case['reload_probe'] == 'Sample = Subject.DCM11;\n'
            and case['filename'] == '_'.join(case['selected'])+'.data', 'Changed Save/probe command contract')
    witnesses = []
    for i in range(3):
        for j in range(3):
            witnesses.append(dict(object='Subject', owned_types=['CoordinateSystemFixed'],
                                  field=f'DCM{i+1}{j+1}', type='Real', expected=matrix[i][j]))
        witnesses.append(dict(object='Subject', owned_types=['CoordinateSystemFixed'],
                              field='AngularVelocity'+'XYZ'[i], type='Real', expected=rates[i]))
    return normalized, witnesses


def replay(stage, output):
    artifacts = {}

    def pin(path):
        path = Path(path).resolve()
        artifacts[str(path)] = SHA(path)
        return path

    repo = stage.parents[2]
    inventory = nasa.inventory(repo)
    parent_stage = stage.parent/'coverage95-20261104-attitude-fixed-offset'
    project = stage/'build-v2'
    producer = LOAD(pin(project/'build-receipt-v2.json'))
    require(producer['status'] == 'passed' and producer['returncodes'] == [0, 0]
            and producer['inputs_unchanged'] and producer['runtime_binary_copies'] == 0
            and producer['shared_application_binaries_written'] == 0
            and producer['inherited_frontend_source_version'] == 6
            and producer['inherited_core_source_version'] == 4, 'Failed/mismatched Save producer')
    source_hashes = LOAD(pin(project/'driver-source-sha256-v2.json'))
    require(source_hashes == producer['driver_source_sha256'], 'Driver compiler closure changed')
    with tarfile.open(pin(project/'driver-source-v2.tar.gz')) as archive:
        require(set(archive.getnames()) == set(source_hashes), 'Substituted driver capsule')
        sources = {name: archive.extractfile(name).read() for name in source_hashes}
    for name, digest in source_hashes.items():
        require(hashlib.sha256(sources[name]).hexdigest() == SHA(pin(project/'source'/name)) == digest,
                'Driver/fixture source bytes changed')
    for name, digest in {**producer['inherited_build_inputs'], **producer['binary_sha256']}.items():
        require(SHA(pin(name)) == digest, 'Save compiler/link/runtime image changed')
    frontend = LOAD(pin(parent_stage/'scope-build-receipt-v6-attempt2.json'))
    require(producer['parent_build_receipt_sha256'] == SHA(parent_stage/'scope-build-receipt-v6-attempt2.json')
            and frontend['status'] == 'passed' and frontend['inputs_unchanged'], 'Inherited frontend producer changed')
    frozen = Path(frontend['frozen_source_root'])
    for name, digest in {**frontend['source_sha256'], **frontend['backend_sources']}.items():
        require(SHA(frozen/name) == digest, 'Frozen frontend/core compiler input changed')
    require(sources[DRIVER] == (frozen/DRIVER).read_bytes(), 'Shared native Save assertions changed')
    catalog = json.loads(sources[FIXTURE])
    cases = {c['key']: c for c in catalog['cases']}
    require(set(cases) == {'Identity', 'Named', 'Body', 'HalfTurn'} and len(catalog['cases']) == 4,
            'Missing/duplicate fixed-offset partition')
    for name, digest in {**catalog['source_sha256'], **catalog['physical_input_sha256']}.items():
        require(SHA(pin(repo/name)) == digest, 'Independent parent fixture/physical input changed')
    old = LOAD(pin(stage/'build-v1/source'/FIXTURE))
    require([{k: v for k, v in c.items() if k != 'filename'} for c in old['cases']]
            == [{k: v for k, v in c.items() if k != 'filename'} for c in catalog['cases']],
            'New fixture correction changed more than the filename')
    for name in ['new-fixture-filename-correction-v2.json', 'build-v1/build-receipt-v1.json',
                 'build-v1/driver-source-v1.tar.gz', 'fixed-offset-named-headless-v1/results.json',
                 'fixed-offset-named-headless-v1-loaded-images.json',
                 'fixed-offset-named-headless-v1/QtGui.NasaSaveFixedOffsetNamed/test.log',
                 'fixed-offset-visible-v2/results.json', 'fixed-offset-visible-v2-loaded-images.json',
                 'visible-route-review-v3.json', 'failed-reader-v1.py', 'evidence-replay-v1.log',
                 'run-cases-v2.py', 'run-cases-v3.py', 'runner-inputs-v2.json', 'runner-inputs-v3.json']:
        pin(stage/name)
    require('FAIL Save resource evidence missing' in (stage/'fixed-offset-named-headless-v1/QtGui.NasaSaveFixedOffsetNamed/test.log').read_text(),
            'Original failed new-fixture result was lost')
    misrouted = LOAD(stage/'fixed-offset-visible-v2/results.json')
    require(all(r['requested_visibility'] == 'headless' for r in misrouted['results'] if r['status'] != 'not-run'),
            'Misrouted profile was relabeled')
    require(LOAD(stage/'runner-inputs-v3.json')[str(repo/'src/qtgui/tests/AuditSaveFixedOffsetEvidence.py')]
            == SHA(stage/'failed-reader-v1.py'), 'Pre-run reader observation was silently substituted')
    prior_path = pin(stage.parent/'coverage95-20261101/save-orbit-modes-evidence.json')
    prior = prior_audit(repo, prior_path)
    require(prior['cumulative_cases'] == 148 and prior['cumulative_contexts'] == 296
            and prior['cumulative_writable_property_comparisons'] == 10190
            and prior['new_requirement_credit'] == [], 'Prior Save evidence conjunction changed')
    executable = next(p for p in producer['binary_sha256'] if Path(p).name == 'GmatQtNasaSaveFixedOffsetTests')
    covered, witnesses, checked = set(), [], 0
    native_result_components = 0
    for run_name, count, mode in [('fixed-offset-named-headless-v2', 1, 'headless'),
                                  ('fixed-offset-remaining-headless-v2', 3, 'headless'),
                                  ('fixed-offset-visible-v3', 4, 'visible')]:
        run = LOAD(pin(stage/run_name/'results.json'))
        maps = LOAD(pin(stage/(run_name+'-loaded-images.json')))
        require(run['mode'] == mode and maps['returncode'] == 0 and maps['inputs_unchanged']
                and maps['runtime_binary_copies'] == 0 and maps['source_version'] == 'frontend6-core4-Save-driver2'
                and maps['build_receipt_sha256'] == SHA(project/'build-receipt-v2.json'), 'Failed/substituted native Save execution')
        for name, digest in maps['loaded_project_images'].items():
            require(maps['expected_project_images'].get(name) == digest == SHA(pin(name)), 'Unbound mapped native image')
        for name, digest in maps['ctest_definition_sha256'].items():
            require(SHA(pin(name)) == digest, 'Executed CTest registration changed')
        processes = [p for p in maps['processes'] if p['executable'] == executable]
        results = [r for r in run['results'] if r['status'] != 'not-run']
        require(len(results) == len(processes) == count, 'Missing/extra native case process')
        if mode == 'visible':
            display = run['visible_display']
            require(run['resizable'] and display['resizable'] and display['start_minimized']
                    and 'Iconic' in display['initial_host_window_state']
                    and '_NET_WM_STATE_FULLSCREEN' not in display['initial_host_window_state'],
                    'Visible Save desktop was not initially minimized/resizable/nonfullscreen')
        for result in results:
            key = result['name'].removeprefix(PREFIX)
            require(key in cases and (mode, key) not in covered, 'Unexpected/duplicate profile case')
            covered.add((mode, key))
            case, work = cases[key], Path(result['log']).parent
            for path in work.iterdir():
                if path.is_file() and path.suffix in {'.json', '.jsonl', '.script', '.data', '.txt', '.log', '.xml'}:
                    pin(path)
            checked += check_case(case, result, sources, artifacts, fixture_path=FIXTURE)
            normalized, computed = independent_truth(case)
            for phase in ['runtime-native.json', 'reload-probe-native.json']:
                check_witnesses({'witnesses': computed}, LOAD(work/phase), phase)
                native_result_components += len(computed)
            raw = (work/'export.data').read_text()
            values = re.findall(r'^Subject\.OffsetQuaternion = \[([^]]+)\];$', raw, re.M)
            require(len(values) == int(case['offset_serialized'])
                    and (not values or equal([float(v) for v in values[0].split()], normalized)),
                    'Normalized offset serialization/default omission changed')
            require('BeginMissionSequence' not in raw and 'Keep' not in raw, 'Unselected mission state leaked into export')
            clean = LOAD(work/'apply-diagnostics.json')
            require(clean['has_changes'] is False and (work/'applied-source.script').read_bytes() == (work/'accepted.script').read_bytes(),
                    'Live editor clean/exact-source observation failed')
            process = [p for p in processes if p['command'][-1] == key]
            require(len(process) == 1 and process[0]['snapshots'], 'No unique actual native process map')
            traces = [json.loads(line) for path in work.glob('trace-*.jsonl') for line in path.read_text().splitlines()]
            counts = collections.Counter((e.get('name'), e.get('event')) for e in traces)
            require(counts['commandChoose_Objects', 'activated'] >= 1
                    and counts['commandObjectDialog', 'shown'] >= 1 and counts['commandObjectList', 'shown'] >= 1,
                    'Actual Save picker activation/display missing')
            if mode == 'visible':
                require(result['requested_visibility'] == 'visible' and result['trace']['native_exposure_events'] > 0,
                        'Actual native visible exposure missing')
            witnesses.append(dict(profile=mode, case=key, pid=process[0]['pid'], proof=str(work/'proof.json')))
    require(covered == {(mode, key) for mode in ['headless', 'visible'] for key in cases}, 'Incomplete Save profile/case conjunction')
    gaps = LOAD(prior_path)['remaining_gaps']
    result = dict(status='passed', contract='source-versioned-bounded-fixed-offset-Save-persistence',
                  original_row='FRC!E132', authority_sha256=inventory['sources'], new_cases=4, new_contexts=8,
                  prior_cases_replayed=0, prior_cases_reused=148, cumulative_cases=152, cumulative_contexts=304,
                  new_writable_property_comparisons=checked, cumulative_writable_property_comparisons=10190+checked,
                  independent_DCM_and_rate_component_checks=native_result_components,
                  original_assertions_changed=0, production_changes=0, native_reruns=0,
                  misrouted_contexts_retained_without_visible_credit=4,
                  witnesses=witnesses, artifacts=artifacts, parent_audit=prior,
                  remaining_gaps=gaps, qualified_keys=[], new_requirement_credit=0, goal_complete=False,
                  whole_FRC_E132_qualified=False, FRC_E135_qualified=False, scientific_approval=False,
                  technical_core_approval=False, full_current_build_qualified=False, full_NASA_qualified=False,
                  reader_sha256=SHA(__file__))
    with output.open('x') as stream:
        json.dump(result, stream, indent=2)
        stream.write('\n')
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--stage', type=Path, default=REPO/'build/nasa-coverage/coverage95-20261104-save-fixed-offset')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    record = replay(args.stage.resolve(), args.output.resolve())
    print(json.dumps({k: record[k] for k in ['status', 'new_cases', 'new_contexts',
          'cumulative_cases', 'new_writable_property_comparisons', 'independent_DCM_and_rate_component_checks',
          'new_requirement_credit', 'whole_FRC_E132_qualified']}))
