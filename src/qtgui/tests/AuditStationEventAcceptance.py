"""Admit the original station-event selection family after both GUI modes pass.

E600 conjoins E601, E602 and the already qualified E603 combined rise/set
child. E597 and E598 (including multiple spacecraft) remain unqualified.
"""
import argparse
import datetime
import hashlib
import json
import math
from pathlib import Path
import re
import sys
import tarfile

import NasaGuiCoverage as nasa
from AuditCapiAcceptance import load, require, sha
from AuditCentralBodyPropagationAcceptance import read_run

KEYS = ['FRR!E600', 'FRR!E601', 'FRR!E602']
SOURCE = 'src/qtgui/tests/NasaContactResultTests.cpp'
FIXTURE = 'src/qtgui/tests/NasaStationEventCases.json'
ORACLE = 'src/qtgui/tests/StationEventOracle.py'
PARENT = 'src/qtgui/tests/NasaContactResultCases.json'
PRODUCTION = ['src/base/subscriber/EphemManager.cpp', 'src/base/subscriber/EphemManager.hpp',
              'plugins/EventLocatorPlugin/src/base/locator/ContactLocator.cpp',
              'plugins/EventLocatorPlugin/src/base/locator/ContactLocator.hpp',
              'src/base/interpreter/Interpreter.cpp', 'src/qtgui/ResourceProperties.cpp',
              'src/qtgui/EventLocatorDialog.cpp']
CASES = ['RiseOnly', 'SetOnly', 'Both', 'MixedRise', 'MixedSet', 'MixedBoth',
         'BoundedBoth', 'BoundedMixed', 'DarkOnly', 'OtherTarget']
NATIVE = ['RiseInterior', 'RiseAtStart', 'RiseAtEnd', 'SetAtStart', 'SetAtEnd',
          'ClippedAbove', 'Below', 'Tangent', 'EqualityPlateau', 'OccultedRise',
          'FarBodyTransit', 'CubicRise', 'CubicSet', 'LatitudePole', 'SingletonRise',
          'RootBeforeWindow', 'RootAfterWindow', 'ShallowRootBeforeWindow',
          'ShallowRootAfterWindow', 'ShallowRootInsideWindow', 'ShiftedCubicBeforeWindow',
          'ShiftedCubicAfterWindow', 'ShiftedCubicInsideWindow', 'RiseAtCoverageStart',
          'RiseAtCoverageEnd']
ADMISSION_MODULES = ['AcceptanceCatalog','AcceptanceSnapshot','AuditAcceptanceRecovery',
                     'AuditCapiAcceptance','AuditCentralBodyPropagationAcceptance',
                     'AuditConstraintScalarAcceptance','AuditStationEventAcceptance',
                     'BuildNasaCoverageExtension','NasaGuiCoverage','RunHarnessAcceptance']


def report_events(text, spec):
    require(text.startswith('Target: '+spec['target']+'\n'), 'Wrong station report target')
    require(text.count('Station events\n') == 1 and
            'Observer\tEventType\tA1ModJulian\tUTCGregorian\n' in text,
            'Missing/duplicated station event section')
    intervals = 'ContactIntervals' in spec['event_types']
    require(('Observer: ' in text) == intervals, 'Station selection leaked/omitted intervals')
    base = datetime.datetime(2000, 1, 1, 12)
    found = []
    for line in text.splitlines():
        cells = line.split('\t')
        if len(cells) != 4 or not cells[1].startswith('Station'):
            continue
        require(len(found) < len(spec['station_events']), 'Extra station event')
        expected = spec['station_events'][len(found)]
        require(cells[0] == expected['observer'] and cells[1] == expected['event_type'],
                'Wrong station observer/type/order')
        a1 = float(cells[2])
        utc = (datetime.datetime.strptime(cells[3], '%d %b %Y %H:%M:%S.%f')-base).total_seconds()
        require(math.isfinite(a1) and abs(utc-expected['time']) < spec['time_tolerance'],
                'Station UTC does not match independent root')
        require(abs((a1-21545)*86400-32.0343817-expected['time']) < spec['time_tolerance'],
                'Station raw A1 does not match independent root')
        found.append(dict(observer=cells[0], event_type=cells[1], utc_seconds=utc, a1=a1))
    require(len(found) == len(spec['station_events']), 'Omitted station event')
    count = re.search(r'Number of station events : (\d+)', text)
    require(count and int(count[1]) == len(found), 'Wrong station event summary')
    return found


def qualify(repo, path, inventory):
    q = load(path)
    require(q['contract_kind'] == 'source-versioned-station-rise-set-selection', 'Wrong station contract')
    require(q['qualified_keys'] == KEYS and set(q['selected']) == {'headless', 'visible'},
            'Changed station keys or missing GUI mode')
    require(all(q.get(k) is False for k in ('technical_core_approval',
            'full_current_build_qualified', 'full_NASA_qualified')), 'Station scope overclaim')
    for name, digest in q['artifact_sha256'].items():
        require(sha(name) == digest, 'Changed station acceptance artifact: '+name)
    require(q['admission_source_archive'] in q['artifact_sha256'], 'Admission source archive omitted')
    expected={name:'src/qtgui/tests/'+name+'.py' for name in ADMISSION_MODULES}
    require(q['executing_admission_modules']==expected, 'Incomplete executing admission helper closure')
    with tarfile.open(q['admission_source_archive']) as archive:
        require(set(archive.getnames())==set(q['admission_source_sha256']), 'Admission source members differ')
        for name,digest in q['admission_source_sha256'].items():
            require(not Path(name).is_absolute() and '..' not in Path(name).parts,
                    'Unsafe admission source member')
            member=archive.getmember(name);data=archive.extractfile(member).read()
            require(member.isfile() and not data.startswith(b'\x7fELF') and
                    hashlib.sha256(data).hexdigest()==digest, 'Changed frozen admission source')
    for module,name in expected.items():
        executing=sys.modules[__name__] if module=='AuditStationEventAcceptance' else sys.modules.get(module)
        require(name in q['admission_source_sha256'] and
                executing is not None and Path(executing.__file__).resolve()==repo/name and
                sha(repo/name)==q['admission_source_sha256'][name],
                'Executing admission helper differs from reviewed source: '+module)
    for spec in q['runs'].values():
        reuse=load(spec['reuse'])
        required={spec['before'],spec['after'],spec['reuse'],
                  reuse['receipt'],reuse['reference'],reuse['delta']}
        require(required <= set(q['artifact_sha256']), 'Unsealed station run inputs or source partition')
        guard=load(spec['build_guard'])
        require(guard['build_exit_code'] == 0 and 'FAILED:' not in Path(guard['successful_build_log']).read_text(),
                'Station executable lacks a successful build')
        require(sha(guard['successful_build_log']) == guard['successful_build_log_sha256'] and
                {spec['build_guard'],spec['compile_archive'],guard['successful_build_log']} <= set(q['artifact_sha256']),
                'Station build evidence omitted or changed')
        before=load(spec['before'])
        receipt=load(reuse['receipt'])
        require(before['binaries'][str(repo/guard['executable'])] == guard['executable_sha256'],
                'Station executed image differs from build guard')
        for result in receipt['results']:
            if result['status']!='not-run':
                require(str(repo/guard['executable']) in result['registered_command'],
                        'Build guard is not for the actual executed test')
        with tarfile.open(spec['compile_archive']) as archive:
            for name,digest in guard['compiled_sources'].items():
                require(not Path(name).is_absolute() and '..' not in Path(name).parts and
                        hashlib.sha256(archive.extractfile(name).read()).hexdigest() == digest,
                        'Station compiled dependency changed')
                if name in before['sources']:
                    require(before['sources'][name]==digest, 'Compiled/input source intersection differs')
                if name in receipt['source_hashes']:
                    require(receipt['source_hashes'][name]==digest, 'Compiled/receipt source intersection differs')
    known = {row['key']: row for row in inventory['requirements']}
    require(len(q['reviews']) == 3 and {row['key'] for row in q['reviews']} == set(KEYS),
            'Incomplete original station source review')
    for row in q['reviews']:
        require(row['source_text'] == known[row['key']]['text'] and
                row['fingerprint'] == known[row['key']]['fingerprint'] and row['rationale'],
                'Original station obligation changed')
    child = q['inherited_combined_child']
    require(child['key'] == 'FRR!E603' and child['fingerprint'] == known['FRR!E603']['fingerprint']
            and child['source_text'] == known['FRR!E603']['text'], 'Combined rise/set child omitted')
    seal = load(q['baseline_seal'])
    require(sha(q['baseline_seal']) == q['baseline_seal_sha256'] and
            seal['authority_sha256'] == inventory['sources'] and
            'FRR!E603' not in {row['key'] for row in seal['remaining_original_rows']['requirements']},
            'Previously qualified combined child not preserved')
    versions = []
    frozen = {}
    for mode, run_name in q['selected'].items():
        spec = q['runs'][run_name]
        receipt, results, sources = read_run(repo, spec, inventory, q['artifact_sha256'], q['physical_inputs'])
        require(receipt['mode'] == mode and set(results) ==
                {'QtGui.NasaContactResultMultiStation', *('QtGui.NasaStationEvent'+case for case in CASES)},
                'Incomplete exact station GUI selection')
        fixture = json.loads(sources[FIXTURE])
        parent = json.loads(sources[PARENT])
        require(fixture['parent_fixture_sha256'] == hashlib.sha256(sources[PARENT]).hexdigest(),
                'Independent original root fixture changed')
        require({case['key'] for case in fixture['cases']} == set(CASES), 'Station matrix changed')
        for case in fixture['cases']:
            # Reconstruct selections from original UNCLIPPED independent roots.
            expected = []
            for observer in case['observers']:
                for contact in parent['cases'][0]['events'][observer]:
                    for endpoint, kind in [('start', 'StationRise'), ('end', 'StationSet')]:
                        time = contact[endpoint]
                        if kind in case['event_types'] and case['start'] <= time <= case['end']:
                            require(time not in (0., 6500.), 'Oracle uses a clipped endpoint')
                            expected.append(dict(observer=observer, event_type=kind, time=time))
            require(sorted(expected, key=lambda e:e['time']) == case['station_events'],
                    'Station oracle substituted clipped/selected runtime output')
            name = 'QtGui.NasaStationEvent'+case['key']; result = results[name]
            require(str(repo/FIXTURE) in result['registered_command'] and case['key'] in result['registered_command'],
                    'Station test did not execute the declared fixture/key')
            require(result['status'] == 'passed', 'Station GUI case failed: '+name)
            work = Path(result['log']).parent
            proof = load(work/'proof.json')
            require(nasa.proof_records(result) == [proof] and proof['outcome'] == 'passed'
                    and proof['family'] == case['key'] and proof['cancels'] == 1
                    and proof['numeric_sentinel'] == 9, 'Missing actual station lifecycle proof')
            require(proof['invalid_rejections'] == (1 if case['key'] == 'Both' else 0),
                    'Station empty-selection rejection missing')
            observed = report_events((work/'events.txt').read_text(), case)
            require(observed == proof['events']['station_events'] ==
                    load(work/'observations.json')['station_events'], 'Station saved/native report proof differs')
            for basename in ['events.txt', 'observations.json', 'proof.json', 'initial.script',
                             'mission.script', 'messages.txt', 'native-stations.json', 'final.csv']:
                require(str(work/basename) in q['artifact_sha256'], 'Essential station evidence omitted')
        default = results['QtGui.NasaContactResultMultiStation']
        work = Path(default['log']).parent
        require(default['status'] == 'passed' and nasa.proof_records(default) == [load(work/'proof.json')],
                'Affected default contact test not proved')
        require((work/'events.txt').read_bytes() == Path(q['legacy_default_report']).read_bytes(),
                'Default contact report changed from preserved passing bytes')
        version = {name: hashlib.sha256(sources[name]).hexdigest() for name in PRODUCTION+[SOURCE,FIXTURE,ORACLE,PARENT]}
        versions.append(version); frozen[mode] = dict(receipt=spec['reuse'], sources=version)
    require(versions[0] == versions[1], 'Station GUI modes used different production/test versions')
    native = q['runs'][q['native_run']]
    receipt, results, sources = read_run(repo, native, inventory, q['artifact_sha256'], q['physical_inputs'])
    require(set(results) == {'QtGui.StationCrossingKernel'} and
            results['QtGui.StationCrossingKernel']['status'] == 'passed', 'Native crossing gate missing')
    work = Path(results['QtGui.StationCrossingKernel']['log']).parent
    require({str(work/name) for name in ['proof.json','outcomes.json','frame.tf']} <= set(q['artifact_sha256']),
            'Native analytic root evidence omitted')
    proof = load(work/'proof.json'); cases = load(work/'outcomes.json')
    require(proof['outcome'] == 'passed' and proof['cases'] == cases and
            [case['case'] for case in cases] == NATIVE, 'Incomplete native crossing matrix')
    native_log = Path(results['QtGui.StationCrossingKernel']['log']).read_text()
    records = [json.loads(match[1]) for match in re.finditer(r'(?:^|\n)(?:\d+: )?NASA_CHECK (\{[^\n]*\})', native_log)]
    require(records == [proof], 'Native checkpoint not tied to actual execution')
    for case in cases:
        name = case['case']
        rise = name in {'RiseInterior','RiseAtStart','RiseAtEnd','FarBodyTransit','CubicRise',
                        'SingletonRise','ShallowRootInsideWindow','ShiftedCubicInsideWindow',
                        'RiseAtCoverageStart','RiseAtCoverageEnd'}
        setting = name in {'SetAtStart','SetAtEnd','CubicSet'}
        require(len(case['rises']) == int(rise) and len(case['sets']) == int(setting),
                'Native independent crossing count differs: '+name)
        offset = {'ShallowRootInsideWindow':5e-6,'ShiftedCubicInsideWindow':2.5e-6,
                  'RiseAtCoverageStart':-100.,'RiseAtCoverageEnd':100.}.get(name,0.)
        require(all(math.isfinite(v) and abs((v-21545)*86400-offset)<5e-6
                    for v in case['rises']+case['sets']), 'Native independent crossing epoch differs')
    for name in PRODUCTION[:2]:
        require(hashlib.sha256(sources[name]).hexdigest() == versions[0][name],
                'Native crossing and GUI helper versions differ')
    spec=q['runs'][q['native_parameter_run']]
    receipt,results,sources=read_run(repo,spec,inventory,q['artifact_sha256'],q['physical_inputs'])
    require(set(results)=={'QtGui.StationEventParameters'} and
            results['QtGui.StationEventParameters']['status']=='passed','Native parameter gate missing')
    result=results['QtGui.StationEventParameters'];work=Path(result['log']).parent;proof=load(work/'proof.json')
    require(str(work/'proof.json') in q['artifact_sha256'] and nasa.proof_records(result)==[proof] and
            proof['rejected_inputs']==12 and all(proof[name] is True for name in
            ['explicit_empty_parsed','empty_initialization_rejected','clone_isolated','stale_outputs_cleared']) and
            proof['native_choices']==['ContactIntervals','StationRise','StationSet'],
            'Native parser/clone/error contract not proved')
    for name in PRODUCTION:
        require(hashlib.sha256(sources[name]).hexdigest()==versions[0][name],
                'Native parameter and GUI production versions differ')
    return q, KEYS


if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3])
    parser.add_argument('--qualification',type=Path,required=True)
    args=parser.parse_args(); repo=args.repo.resolve()
    q,keys=qualify(repo,args.qualification,nasa.inventory(repo))
    print(json.dumps(dict(status='passed',qualified_keys=keys,GUI_cases_per_mode=11,native_cases=25)))
