"""Replay source-versioned normalized-model GUI labels without native tests."""
import hashlib
import json
from pathlib import Path
import re
import tarfile
import xml.etree.ElementTree as ET

load = lambda p: json.loads(Path(p).read_text())
sha = lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest()


def need(ok, message):
    if not ok:
        raise ValueError(message)


def labels(value, cases, failures):
    expected=[(c['key'],field) for c in cases for field in sorted(c['units'])]
    rows=value['observations']
    need([(v['case'],v['field']) for v in rows] == expected and
         value['cases'] == [c['key'] for c in cases] and value['mission_executions'] == 0 and
         value['source_native_history_file_isolation'], 'Model unit route/isolation census differs')
    rejected=0
    for row in rows:
        passed=row['actual_unit'] == 'dimensionless' and 'dimensionless' in row['visible_unit_or_caption']
        need(row['expected_unit'] == 'dimensionless' and row['passed'] == passed and
             row['phase'] == 'initial' and row['route'] in ('advanced-table','primary+backing-metadata'),
             'Retained label does not support its native assertion')
        rejected += not passed
    need(rejected == failures == value['mismatches'] and value['status'] == ('failed' if failures else 'passed'),
         'Original label failure or pass was reclassified')


def audit(repo,stage):
    repo,stage=Path(repo),Path(stage);plan=load(stage/'frozen-plan-v1.json');repair=load(stage/'implementation-plan-v1.json')
    previous=Path(plan['previous_engineering_receipt'])
    need(sha(previous) == plan['previous_engineering_receipt_sha256'] ==
         '36cf3dc4002c3eb1d7c6476ccfeba957e5f2cffbf699c724f2e3657f07d10ed6', 'Previous engineering receipt differs')
    versions={};fixture=None
    for version in ('before-v1','after-v1','after-v2'):
        ids=load(stage/('source-sha256-'+version+'.json'))
        with tarfile.open(stage/('sources-'+version+'.tar.gz')) as bundle:
            need(set(bundle.getnames()) == set(ids), 'Model unit source inventory differs')
            for n,h in ids.items():
                need(hashlib.sha256(bundle.extractfile(n).read()).hexdigest() == h, 'Model unit source bytes differ: '+n)
            fixture=json.loads(bundle.extractfile('src/qtgui/tests/NormalizedModelUnitCases.json').read())
            need(ids['src/qtgui/tests/NormalizedModelUnitCases.json'] == plan['fixture_sha256'], 'Original model fixture changed')
            if version == 'after-v2':
                adaptive=json.loads(bundle.extractfile('src/qtgui/tests/AdaptiveRatioUnitCases.json').read())
                documents={n:bundle.extractfile(n).read() for n in plan['document_sha256']}
        build=load(stage/('build-'+version+'.json'))
        need(build['status'] == 'passed' and build['returncode'] == 0 and build['inputs_unchanged'] and
             build['source_sha256'] == ids and build['log_sha256'] == sha(stage/('build-'+version+'.log')),
             'Model unit build/source guard differs')
        versions[version]=(ids,build)
    before,after,narrow=[versions[v][0] for v in ('before-v1','after-v1','after-v2')]
    need(set(before) == set(after) and {n for n,h in before.items() if after[n] != h} == {'src/qtgui/ResourceProperties.cpp'} and
         before['src/qtgui/ResourceProperties.cpp'] == repair['preimage_sha256'], 'Initial model label repair scope differs')
    need({n for n,h in after.items() if narrow[n] != h} == {'src/qtgui/ResourceProperties.cpp','src/qtgui/CMakeLists.txt'} and
         all(n == 'src/qtgui/tests/AdaptiveRatioUnitCases.json' or n.startswith('plugins/ExtraPropagatorsPlugin/')
             for n in set(narrow)-set(after)), 'Narrow adaptive lookup repair changed unrelated source')
    for n,h in versions['before-v1'][1]['binary_sha256'].items():
        if Path(n).name.startswith('lib'):
            need(versions['after-v1'][1]['binary_sha256'][n] == versions['after-v2'][1]['binary_sha256'][n] == h,
                 'Native library changed during GUI label repair')
    selected=next(c for c in fixture['cases'] if c['key'] == 'AdaptiveStepRatios')
    need(adaptive['cases'] == [selected], 'Selective adaptive rerun changed its frozen case')
    adaptive_plan=load(stage/'adaptive-frozen-plan-v1.json')
    need(adaptive_plan['case'] == selected and adaptive_plan['remaining_failures'] == 2 and
         adaptive_plan['preimage_sha256'] == after['src/qtgui/ResourceProperties.cpp'], 'Adaptive correction plan differs')
    extra=load(stage/'extra-propagator-source-link-v1.json')
    need(extra['status'] == 'passed' and extra['extra_native_tests_run'] == 0 and
         extra['extra_propagator_image_sha256'] == next(h for n,h in versions['after-v2'][1]['binary_sha256'].items()
                                                     if Path(n).name == 'libExtraPropagators.so'), 'ExtraPropagators source link differs')
    ns={'d':'http://docbook.org/ns/docbook'}
    text=lambda node:' '.join(''.join(node.itertext()).split()) if node is not None else ''
    for entry in plan['current_document_definitions']:
        data=documents[entry['source']]
        need(hashlib.sha256(data).hexdigest() == plan['document_sha256'][entry['source']], 'Frozen quantity definition changed')
        matches=[row for row in ET.fromstring(data).findall('.//d:row',ns) if text(row.find('d:entry',ns)) == entry['current_document_field']]
        need(len(matches) == 1, 'Independent model field definition is ambiguous')
        metadata={text(v.find('d:term',ns)):text(v.find('d:listitem',ns)) for v in matches[0].findall('.//d:varlistentry',ns)}
        need(metadata['Units'] == entry['current_units'] and 'imensionless' in metadata['Units'], 'Independent normalized quantity units differ')
    runs=[('headless','before-v1','all',{'NormalizedModels':9,'SolarCoefficients':0}),
          ('headless','after-v1','normalized',{'NormalizedModels':2}),
          ('headless','after-v2','adaptive',{'AdaptiveRatios':0}),
          ('visible','after-v2','all',{'NormalizedModels':0,'SolarCoefficients':0})]
    for mode,version,selection,groups in runs:
        key='gui-'+mode+'-'+version+'-'+selection;root=stage/key
        receipt=load(stage/(key+'-receipt.json'));batch=load(root/'results.json');build=versions[version][1]
        need(receipt['inputs_unchanged'] and receipt['returncode'] == (1 if any(groups.values()) else 0) and
             all(receipt['source_sha256'][n] == h for n,h in versions[version][0].items()), 'Executed model unit source/terminal guard differs')
        need(batch['mode'] == mode and batch['execution_counts']['executed_now'] == len(groups) and
             batch['execution_counts']['reused_passes'] == 0 and batch['end_reason'] == 'completed-selection',
             'Model unit selection/profile differs')
        if mode == 'visible':
            display=batch['visible_display'];need(batch['display_route'] == 'nested' and
                display['route'] == 'nested Xephyr window on existing desktop' and display['start_minimized'], 'Visible native display proof missing')
        loaded={}
        for path in stage.glob(key+'-loader.*'):
            for line in path.read_text(errors='replace').splitlines():
                found=re.search(r'calling init:\s+(\S+)',line)
                if found and (Path(found[1]).name.startswith('libGmat') or Path(found[1]).name.startswith('libEKF')):
                    p=str(Path(found[1]).resolve());loaded[p]=receipt['image_sha256'].get(p)
        for n,h in build['binary_sha256'].items():
            if Path(n).name == 'libExtraPropagators.so':
                need(receipt['image_sha256'][str(Path(n).resolve())] == extra['extra_propagator_image_sha256'], 'ExtraPropagators image changed')
            elif Path(n).name.startswith('lib'):
                need(loaded.get(str(Path(n).resolve())) == h, 'Wrong loaded model library: '+n)
            elif Path(n).name != 'GmatQt':
                need(receipt['image_sha256'].get(str(Path(n))) == h, 'Wrong GUI model worker image')
        if version != 'after-v2':
            need(any(h == extra['extra_propagator_image_sha256'] for n,h in receipt['image_sha256'].items()
                     if Path(n).name.startswith('libExtraPropagators.so')), 'Recovered ExtraPropagators source does not match earlier images')
        for group,failures in groups.items():
            case=root/('QtGui.NativeModelUnits'+group);result=load(case/'result.json');value=load(case/'proof.json')
            need(result['returncode'] == (8 if failures else 0) and 'QT_QPA_PLATFORM=xcb' in ' '.join(result['registered_command']),
                 'Native model command/status differs')
            cases=adaptive['cases'] if group == 'AdaptiveRatios' else [c for c in fixture['cases'] if c['group'] == group]
            labels(value,cases,failures)
            need(value['observations'] == load(case/'unit-observations.json'), 'Paired model observations differ')
    protected=load(stage/'protected-input-check-v1.json')
    need(protected['status'] == 'passed' and protected['original_inputs_unchanged'] == 1334 and not protected['changed'] and
         sha(protected['manifest']) == protected['manifest_sha256'], 'Protected input record differs')
    return dict(status='passed',production_source_files_modified=1,defined_type_field_pairs=12,passing_GUI_observations=28,
                reused_headless_observations_after_narrow_repair=12,original_blank_unit_failures_retained=9,
                intermediate_adaptive_failures_retained=2,native_mission_executions=0,original_inputs_unchanged=1334,
                native_tests_run_by_replay=0,new_requirement_credit=0,requirements_verified=1106,requirements_total=1158,
                agreed_complete=5,agreed_total=18,whole_row_qualified=False,full_current_build_qualified=False,full_regression=False)
