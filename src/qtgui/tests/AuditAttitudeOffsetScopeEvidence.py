"""Read-only replay of all 68 offset-frame cells in both execution profiles.

This reader proves the frame/origin component. Boundary/validation/copy contracts
and the whole FRR!E52 admission remain separate, explicit obligations.
"""
import argparse
import copy
import csv
import hashlib
import importlib.util
import io
import json
import math
from pathlib import Path
import tarfile

import NasaGuiCoverage as nasa

REPO = Path(__file__).resolve().parents[3]
SHA = lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest()
LOAD = lambda p: json.loads(Path(p).read_text())


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def numbers(case, proof, raw, repeated, state):
    require(raw == repeated, 'Repeated Run changed the complete raw report')
    records = list(csv.reader(io.StringIO(raw.decode())))
    require(len(records) == 4 and records[-1] == ['42'], 'Missing epochs/following sentinel')
    rows = [[float(v) for v in row] for row in records[:-1]]
    require(rows == proof['actual'] and len(proof['checks']) == 3, 'Raw/proof values differ')
    maximum_matrix = maximum_rate = 0
    for row, sample, check in zip(rows, case['samples'], proof['checks']):
        expected = [sample['epoch']] + sum(sample['rows'], [])
        expected += sample['angular_velocity_degrees_s'] + state + [41]
        require(len(row) == len(expected) == 20, 'Incomplete orientation/rate/state output')
        for i, (actual, wanted) in enumerate(zip(row, expected)):
            bound = (1e-10 if i == 0 else case['matrix_bound'] if i < 10 else
                     case['rate_bound_degrees_s'] if i < 13 else 1e-9)
            require(math.isfinite(actual) and abs(actual-wanted) <= bound,
                    'Original independent bound exceeded')
            if 1 <= i < 10: maximum_matrix = max(maximum_matrix, abs(actual-wanted))
            if 10 <= i < 13: maximum_rate = max(maximum_rate, abs(actual-wanted))
        require(check['maximum_matrix_error'] == maximum_matrix
                and check['maximum_rate_error_degrees_s'] == maximum_rate,
                'Per-epoch maximum differs from raw output')
    require(proof['maximum_matrix_error'] == maximum_matrix
            and proof['maximum_rate_error_degrees_s'] == maximum_rate,
            'Terminal maxima differ from complete raw output')
    return rows


def replay(stage, output):
    artifacts = {}
    def pin(path):
        path = Path(path)
        artifacts[str(path)] = SHA(path)
        return path
    producer = LOAD(pin(stage/'scope-build-receipt-v6-attempt2.json'))
    require(producer['status']=='passed' and producer['returncodes']==[0,0]
            and producer['inputs_unchanged'], 'Frozen frontend build did not pass')
    frozen = Path(producer['frozen_source_root'])
    identities = LOAD(pin(stage/'executed-source-sha256-v6.json'))
    require(identities == producer['source_sha256'], 'Compiler source inventory differs')
    with tarfile.open(pin(stage/'executed-sources-v6.tar.gz')) as archive:
        members = archive.getmembers()
        require(len(members)==len(identities), 'Source capsule cardinality differs')
        for member in members:
            require(member.isfile() and member.name in identities, 'Unlisted source capsule member')
            payload = archive.extractfile(member).read()
            require(hashlib.sha256(payload).hexdigest()==identities[member.name]
                    and SHA(frozen/member.name)==identities[member.name], 'Compiled source changed')
    for name, digest in producer['backend_sources'].items():
        require(SHA(frozen/name)==digest, 'Source4 backend closure differs in source6 capsule')
    for name, digest in producer['binary_sha256'].items():
        require(SHA(name)==digest, 'Executed build image changed')
    physical = LOAD(pin(stage/'scope-supplemental-physical-input-v6.json'))
    require(SHA(pin(physical['frozen']))==physical['sha256'], 'Frozen station kernel changed')
    fixture = LOAD(pin(frozen/'src/qtgui/tests/AttitudeOffsetScopeCases.json'))
    input_review = LOAD(pin(stage/'scope-input-review-v1.json'))
    require(input_review['status']=='passed' and input_review['required_cells']==68
            and input_review['fixture_sha256']==SHA(frozen/'src/qtgui/tests/AttitudeOffsetScopeCases.json'),
            'Independent input review does not match executed fixture')
    input_reader = pin(stage/'audit-scope-inputs-v1.py')
    require(SHA(input_reader)==input_review['reader_sha256'], 'Independent input reader changed')
    module_spec = importlib.util.spec_from_file_location('offset_scope_input_replay', input_reader)
    module = importlib.util.module_from_spec(module_spec)
    module_spec.loader.exec_module(module)
    module.check(fixture)
    cases = {c['key']:c for c in fixture['cases']}
    require(len(cases)==len(fixture['cases'])==68, 'Incomplete/duplicate full applicability partition')
    executable = next(p for p in producer['binary_sha256'] if Path(p).name=='GmatQtNasaAttitudeOffsetScopeTests')
    core = {p for p in producer['binary_sha256'] if Path(p).name.startswith('libGmat')}
    covered, witnesses = set(), []
    first_control = None
    runs = ['offset-scope-initial-headless-v6', 'offset-scope-remaining-headless-v6', 'offset-scope-visible-v6']
    for name in runs:
        run = LOAD(pin(stage/name/'results.json'))
        maps = LOAD(pin(stage/(name+'-loaded-images.json')))
        mode = run['mode']
        require(maps['returncode']==0 and maps['inputs_unchanged'] and maps['source_version']==6
                and maps['runtime_binary_copies']==0
                and maps['build_receipt_sha256']==SHA(stage/'scope-build-receipt-v6-attempt2.json'),
                'Actual run/source/maps contract failed')
        for path,digest in maps['loaded_project_images'].items():
            require(SHA(path)==digest, 'Actual mapped project image changed')
        selected = [r for r in run['results'] if r['status']!='not-run']
        require(len(selected)==(4 if name==runs[0] else 64 if name==runs[1] else 68),
                'Extra/omitted scope execution')
        test_processes = [p for p in maps['processes'] if p['executable']==executable]
        require(len(test_processes)==len(selected), 'Missing/duplicate actual host process')
        if mode=='visible':
            display = run['visible_display']
            require(run['resizable'] and run['display_route']=='nested' and display['resizable']
                    and display['start_minimized'] and 'Iconic' in display['initial_host_window_state']
                    and '_NET_WM_STATE_FULLSCREEN' not in display['initial_host_window_state'],
                    'Visible profile lacks initially minimized resizable nonfullscreen display')
        for result in selected:
            require(result['status']=='passed' and result['returncode']==0, 'Native scope case did not pass')
            work = Path(result['log']).parent
            proof = LOAD(pin(work/'proof.json'))
            key = proof['case']
            case = cases[key]
            require((mode,key) not in covered, 'Passing profile/cell executed twice')
            covered.add((mode,key))
            require(proof['checkpoint']=='attitude-offset-scope' and proof['outcome']=='passed'
                    and proof['model']=='CoordinateSystemFixed' and proof['reference']=='CreatedFrame'
                    and proof['repeat_runs']==2 and proof['actual_popups']==6
                    and all(proof[k]==case[k] for k in ['axis','origin_class','origin_requirement','axis_requirement'])
                    and proof['offset_quaternion']==fixture['offset_quaternion']
                    and proof['offset_matrix']==fixture['offset_matrix'], 'Substituted/incomplete scope witness')
            require([p for p in nasa.proof_records(result) if p.get('checkpoint')=='attitude-offset-scope']==[proof],
                    'Saved proof differs from terminal native log')
            raw = pin(work/'values.csv').read_bytes()
            repeated = pin(work/'repeat.values.csv').read_bytes()
            rows = numbers(case,proof,raw,repeated,fixture['input_state'])
            # Inverse composition independently preserves each original frame bound.
            rotation = fixture['offset_matrix']
            for row,sample in zip(rows,case['samples']):
                for i in range(3):
                    for j in range(3):
                        basis = math.fsum(rotation[k][i]*row[1+3*k+j] for k in range(3))
                        require(abs(basis-sample['frame_rows'][i][j])<=case['matrix_bound'],
                                'Recovered original frame basis exceeds its original bound')
                    rate = math.fsum(rotation[k][i]*row[10+k] for k in range(3))
                    require(abs(rate-sample['frame_angular_velocity_degrees_s'][i])<=case['rate_bound_degrees_s'],
                            'Recovered original frame rate exceeds its original bound')
            require(LOAD(pin(work/'case.json'))==case and LOAD(pin(work/'checks.json'))==proof['checks'],
                    'Executed inputs/checks differ')
            for item in ['initial.script','accepted.script','build.messages.txt','run.messages.txt',
                         'repeat.messages.txt','test.log','ctest.xml','result.json']:
                pin(work/item)
            for path in work.glob('trace-*.jsonl'): pin(path)
            before,after = (work/'initial.script').read_text(), (work/'accepted.script').read_text()
            require(before!=after and 'Sat.OffsetQuaternion' in after
                    and before.split('BeginMissionSequence;',1)[1]==after.split('BeginMissionSequence;',1)[1],
                    'Actual primary Apply omitted offset or changed mission')
            actual_process = [p for p in test_processes if p['command'][-1]==key]
            require(len(actual_process)==1, 'No unique mapped native witness for this cell')
            paths = {path for snapshot in actual_process[0]['snapshots'] for path in snapshot['project_images']}
            require({executable,*core}<=paths, 'Actual cell lacks mapped core/executable closure')
            if mode=='visible':
                require(result['requested_visibility']=='visible' and result['trace']['instrumented_processes']==1
                        and result['trace']['native_exposure_events']>0, 'No actual native visible exposure')
            witnesses.append(dict(profile=mode,case=key,proof=str(work/'proof.json'),pid=actual_process[0]['pid'],
                                  maximum_matrix_error=proof['maximum_matrix_error'],
                                  maximum_rate_error_degrees_s=proof['maximum_rate_error_degrees_s']))
            if first_control is None: first_control=(case,proof,raw,repeated,fixture['input_state'])
    require(covered=={(mode,key) for mode in ['headless','visible'] for key in cases},
            'Whole original profile/cell conjunction is incomplete')
    rejected = []
    for fault in ['orientation','rate','missing-epoch','repeat-output']:
        case,proof,raw,repeated,state = copy.deepcopy(first_control)
        if fault in ['orientation','rate']:
            records = list(csv.reader(io.StringIO(raw.decode())))
            column = 1 if fault=='orientation' else 10
            records[0][column] = str(float(records[0][column])+.01)
            proof['actual'][0][column] = float(records[0][column])
            raw = ('\n'.join(','.join(row) for row in records)+'\n').encode()
            repeated = raw
        elif fault=='missing-epoch': raw=repeated=b'\n'.join(raw.splitlines()[1:])+b'\n'
        else: repeated += b'0\n'
        try: numbers(case,proof,raw,repeated,state)
        except AssertionError: rejected.append(fault)
        else: raise AssertionError('Damaged raw output accepted: '+fault)
    result = dict(status='passed',contract='full-original-frame-origin-offset-component',
                  requirement='FRR!E52',fingerprint='3041f4f0af0564670cc6227dfdb1a5d7d45c70b07832be99b9cf6e1f12143333',
                  original_combinations=80,original_explicit_exclusions=12,required_cells=68,
                  passed_profile_cells=len(covered),checked_epochs=3*len(covered),actual_mission_runs=2*len(covered),
                  source_version=6,witnesses=witnesses,artifacts=artifacts,reader_sha256=SHA(__file__),
                  damaged_raw_controls_rejected=rejected,native_reruns=0,new_requirement_credit=0,
                  whole_row_qualified=False,scientific_approval=False,technical_core_approval=False,
                  full_current_build_qualified=False,full_NASA_qualified=False,
                  remaining=['offset boundary/GUI rejection conjunction','whole-row source-specific admission'])
    with output.open('x') as stream: json.dump(result,stream,indent=2);stream.write('\n')
    return result


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--stage',type=Path,default=REPO/'build/nasa-coverage/coverage95-20261104-attitude-fixed-offset')
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    result=replay(args.stage.resolve(),args.output.resolve())
    print(json.dumps({k:result[k] for k in ['status','required_cells','passed_profile_cells','checked_epochs','actual_mission_runs','native_reruns','new_requirement_credit']}))
