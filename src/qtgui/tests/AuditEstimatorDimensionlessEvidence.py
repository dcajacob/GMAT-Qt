"""Replay the three estimator unit labels and source-versioned GUI receipts."""
import gzip
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


def proof(value, fixture, passing):
    expected = [(c['key'],f) for c in fixture['cases'] for f in sorted(c['units'])]
    rows = value['observations']
    need([(v['case'],v['field']) for v in rows] == expected and len(rows) == 6 and
         value['cases'] == [v['key'] for v in fixture['cases']] and
         value['source_native_history_file_isolation'] and value['mission_executions'] == 0,
         'Estimator unit route/isolation census differs')
    mismatches = 0
    for row in rows:
        need(row['expected_unit'] == 'dimensionless' and row['route'] in
             ('advanced-table','primary+backing-metadata') and row['phase'] == 'initial', 'Unit expectation or route differs')
        observed = row['actual_unit'] == 'dimensionless' and 'dimensionless' in row['visible_unit_or_caption']
        need(row['passed'] == observed, 'Native unit assertion differs from retained label')
        mismatches += not observed
        if not passing:
            need(row['actual_unit'] == '', 'Original empty-unit failure changed')
    need(mismatches == (0 if passing else 6) == value['mismatches'] and
         value['status'] == ('passed' if passing else 'failed'), 'Unit failure/pass status reclassified')


def audit(repo, stage):
    repo,stage = Path(repo),Path(stage)
    plan = load(stage/'frozen-plan-v1.json'); fix = load(stage/'implementation-plan-v1.json')
    previous = Path(plan['previous_engineering_receipt'])
    need(sha(previous) == plan['previous_engineering_receipt_sha256'] ==
         '029fd9479d6f0f29b4f45d1317b233ef0a16d400166994941b6db5152579195b', 'Previous engineering receipt differs')
    versions = {}
    for version in ('before-v1','after-v1'):
        ids = load(stage/('source-sha256-'+version+'.json'))
        with tarfile.open(stage/('sources-'+version+'.tar.gz')) as bundle:
            need(set(bundle.getnames()) == set(ids), 'Unit source capsule inventory differs')
            for n,h in ids.items():
                need(hashlib.sha256(bundle.extractfile(n).read()).hexdigest() == h, 'Unit source capsule bytes differ: '+n)
            fixture = json.loads(bundle.extractfile('src/qtgui/tests/EstimatorDimensionlessUnitCases.json').read())
            document = bundle.extractfile('doc/help/src/Resource_BatchEstimator.xml').read()
        build = load(stage/('build-'+version+'.json'))
        need(build['status'] == 'passed' and build['returncode'] == 0 and build['inputs_unchanged'] and
             build['source_sha256'] == ids and build['log_sha256'] == sha(stage/('build-'+version+'.log')),
             'Unit target build/source guard differs')
        need(ids['src/qtgui/tests/EstimatorDimensionlessUnitCases.json'] == plan['fixture_sha256'] and
             ids['doc/help/src/Resource_BatchEstimator.xml'] == plan['current_document_sha256'], 'Frozen unit fixture/document changed')
        versions[version] = (ids,build)
    before,after = [versions[v][0] for v in ('before-v1','after-v1')]
    need(set(before) == set(after) and {n for n,h in before.items() if after[n] != h} ==
         {'src/qtgui/ResourceProperties.cpp'} and before['src/qtgui/ResourceProperties.cpp'] == fix['preimage_sha256'],
         'GUI label repair changed unrelated sources or protected tests')
    for n,h in versions['before-v1'][1]['binary_sha256'].items():
        if Path(n).name.startswith('lib'):
            need(versions['after-v1'][1]['binary_sha256'][n] == h, 'Native library changed during GUI-only repair')
    fields = plan['target_fields']; need(fields == ['ILSEMultiplicativeConstant','OLSEMultiplicativeConstant','RelativeTol'], 'Unit family changed')
    ns = {'d':'http://docbook.org/ns/docbook'}
    text = lambda node:' '.join(''.join(node.itertext()).split()) if node is not None else ''
    documented = {}
    for row in ET.fromstring(document).findall('.//d:row',ns):
        label = text(row.find('d:entry',ns))
        if label in fields:
            metadata = {text(v.find('d:term',ns)):text(v.find('d:listitem',ns)) for v in row.findall('.//d:varlistentry',ns)}
            documented[label] = metadata['Units']
    need(documented == {n:'dimensionless' for n in fields}, 'Independent owning unit definitions differ')
    need(plan['expected_case_count'] == 2 and plan['expected_observations_each_profile'] == 6 and
         plan['conditional_fields_excluded'] == ['AbsoluteTol','OLSEInitialRMSSigma'], 'Frozen unit applicability scope differs')
    for mode,version,passing in [('headless','before-v1',False),('headless','after-v1',True),('visible','after-v1',True)]:
        key='gui-'+mode+'-'+version;root=stage/key
        receipt=load(stage/(key+'-receipt.json'));batch=load(root/'results.json')
        result=load(root/'QtGui.NativeEstimatorDimensionlessUnits/result.json')
        need(receipt['returncode'] == (0 if passing else 1) and receipt['inputs_unchanged'] and
             all(receipt['source_sha256'][n] == h for n,h in versions[version][0].items()), 'Executed source/terminal unit receipt differs')
        need(batch['mode'] == mode and batch['filter'] == '^QtGui.NativeEstimatorDimensionlessUnits$' and
             batch['execution_counts']['executed_now'] == 1 and batch['execution_counts']['reused_passes'] == 0 and
             batch['end_reason'] == 'completed-selection' and
             batch['counts'].get('passed' if passing else 'failed') == 1 and
             result['returncode'] == (0 if passing else 8), 'Unit runner selection/profile differs')
        if mode == 'visible':
            visible=batch['visible_display']
            need(batch['display_route'] == 'nested' and visible['route'] == 'nested Xephyr window on existing desktop' and
                 visible['start_minimized'] and visible['server_pid'] > 0 and visible['host_window'], 'Watchable native display evidence missing')
        need('QT_QPA_PLATFORM=xcb' in ' '.join(result['registered_command']) and
             any('RunNativeTest.py' in str(v) for v in result['registered_command']), 'Native X11 launch contract changed')
        loaded={}
        for path in stage.glob(key+'-loader.*'):
            for line in path.read_text(errors='replace').splitlines():
                found=re.search(r'calling init:\s+(\S+)',line)
                if found and Path(found[1]).name.startswith('libGmat'):
                    p=str(Path(found[1]).resolve());loaded[p]=receipt['image_sha256'].get(p)
        for n,h in versions[version][1]['binary_sha256'].items():
            if Path(n).name.startswith('lib'):
                need(loaded.get(str(Path(n).resolve())) == h, 'Wrong mapped native unit library/plugin: '+n)
            else:
                need(receipt['image_sha256'].get(str(Path(n))) == h or Path(n).name == 'GmatQt', 'Wrong unit worker image')
        value=load(root/'QtGui.NativeEstimatorDimensionlessUnits/proof.json')
        need(value['observations'] == load(root/'QtGui.NativeEstimatorDimensionlessUnits/unit-observations.json'), 'Paired unit observations differ')
        proof(value,fixture,passing)
    protected=load(stage/'protected-input-check-v1.json')
    need(protected['status'] == 'passed' and protected['original_inputs_unchanged'] == 1334 and
         not protected['changed'] and sha(protected['manifest']) == protected['manifest_sha256'], 'Protected original input record differs')
    refreshed=load(stage/'inventory-reconciliation-v2.json')
    need(refreshed['retained_numeric_records'] == 447 and refreshed['matched_document_entries'] == 450 and
         refreshed['unmatched_document_entries'] == 6 and refreshed['native_tests_run'] == 0,
         'Nonqualifying unit-document join scope differs')
    inventory=load(stage/'inventory-reconciliation-v1.json')
    with tarfile.open(stage/'inventory-documents-v1.tar.gz') as bundle:
        need(set(bundle.getnames()) == set(inventory['current_document_sha256']), 'Unit inventory document archive differs')
        for n,h in inventory['current_document_sha256'].items():
            need(hashlib.sha256(bundle.extractfile(n).read()).hexdigest() == h, 'Archived unit document changed')
    with tarfile.open(stage/'census-capsule-v1.tar.gz') as bundle:
        inputs={str(Path(n).relative_to(repo)):v for n,v in inventory['census_sha256'].items()}
        need(len(inputs) == 50 and set(bundle.getnames()) == set(inputs), 'Saved compressed census inventory differs')
        for n,record in inputs.items():
            data=bundle.extractfile(n).read()
            need(hashlib.sha256(data).hexdigest() == record['compressed_sha256'] and
                 hashlib.sha256(gzip.decompress(data)).hexdigest() == record['decompressed_sha256'],
                 'Source-versioned unit census bytes differ')
    return dict(status='passed',production_source_files_modified=1,distinct_dimensionless_controls=3,
                requested_factory_routes=2,distinct_native_estimator_classes=1,passing_GUI_observations=12,
                original_failed_GUI_observations_retained=6,native_headless_passed=True,native_visible_passed=True,
                mission_executions=0,original_inputs_unchanged=1334,native_tests_run_by_replay=0,
                new_requirement_credit=0,requirements_verified=1106,requirements_total=1158,
                agreed_complete=5,agreed_total=18,whole_row_qualified=False,full_current_build_qualified=False,full_regression=False)
