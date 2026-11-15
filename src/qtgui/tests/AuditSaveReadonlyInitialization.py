"""Audit every new/reused reload-initialization witness and the exact core repairs."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import subprocess
import sys
import tarfile
import tempfile

import NasaGuiCoverage as nasa
from AuditSaveResourceEvidence import equal

REPO = Path(__file__).resolve().parents[3]
SHA = lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest()
LOAD = lambda p: json.loads(Path(p).read_text())
DRIVER = 'src/qtgui/tests/NasaSaveReadonlyInitializationTests.cpp'
BASE = 'src/qtgui/tests/NasaSaveResourceTests.cpp'
FIXTURE = 'src/qtgui/tests/NasaSaveReadonlyInitializationCases.json'
PREFIX = 'QtGui.NasaSaveReadonlyInit'
REPAIRED = {'ResourceOrbitView', 'ResourceOrbitViewPopulated', 'SupportTextEphemFile'}


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def field(roots, request):
    node = roots[request['object']]
    for index in request['owned_indices']:
        node = node['owned'][index]
    require(node['type'] == request['native_type'], 'Wrong native read-only owner')
    found = [f for f in node['fields'] if f['name'] == request['field'] and f['type'] == request['type']]
    require(len(found) == 1 and found[0]['read_only'] and 'value' in found[0]
            and 'getter_error' not in found[0], 'Absent/ambiguous read-only native descriptor')
    return node, found[0]


def replay(stage, output):
    repo = stage.parents[2]
    artifacts = {}

    def pin(path):
        path = Path(path).resolve();artifacts[str(path)] = SHA(path);return path

    inventory = nasa.inventory(repo)
    project = stage/'build-v1'
    producer = LOAD(pin(project/'build-receipt-v1.json'))
    require(producer['status'] == 'passed' and producer['inputs_unchanged']
            and producer['runtime_binary_copies'] == producer['shared_application_binaries_written'] == 0,
            'Initialization driver producer failed')
    identities = LOAD(pin(project/'driver-source-sha256-v1.json'))
    require(identities == producer['driver_source_sha256'], 'Driver compiler input closure changed')
    with tarfile.open(pin(project/'driver-source-v1.tar.gz')) as archive:
        require(set(archive.getnames()) == set(identities), 'Wrong driver source capsule')
        sources = {name: archive.extractfile(name).read() for name in identities}
    for name, digest in identities.items():
        require(hashlib.sha256(sources[name]).hexdigest() == SHA(pin(project/'source'/name)) == digest,
                'Compiled initialization input changed')
    restored = re.sub(rb'\n/\* BEGIN_SAVE_READONLY_(\w+) \*/\n.*?\n/\* END_SAVE_READONLY_\1 \*/\n',
                      b'', sources[DRIVER], flags=re.S)
    require(restored == sources[BASE], 'An original generic driver byte/assertion changed')
    pin(project/'original-driver-preservation-v1.json')
    # Parameterize only the compiled-source path in a private copy of the
    # unchanged generic reader. Preserve every original property/proof assertion.
    helper_path = pin(Path(__file__).with_name('AuditSaveResourceEvidence.py'))
    spec = importlib.util.spec_from_file_location('readonly_generic_save_contract', helper_path)
    helper = importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
    helper.SOURCE = DRIVER
    original_materialize = helper.materialize_case_inputs
    def materialize_qt_optional(case, work, bindings):
        materialized = original_materialize(case,work,bindings)
        if materialized.get('before_save','') is None:
            # Archived Qt metadata contains this optional null member. The
            # executed input and metadata still compare exactly, above the
            # baseline expansion. Model only QJsonValue::toString() on null.
            require('spec.value("before_save").toString()' in sources[DRIVER].decode(),
                    'Optional-null expansion differs from the compiled driver')
            materialized = dict(materialized,before_save='')
        return materialized
    helper.materialize_case_inputs = materialize_qt_optional
    catalog = json.loads(sources[FIXTURE]);cases = {c['key']: c for c in catalog['cases']}
    require(len(cases) == len(catalog['cases']) == 18 and catalog['new_native_fields'] == 65
            and catalog['reused_native_fields'] == 66 and catalog['pending_observed_value_changes'] == 22,
            'Missing/duplicate initialization scope or hidden differences')
    review = LOAD(pin(catalog['readonly_review']))
    pin(stage/'failed-readonly-reader-v1.py');pin(stage/'evidence-replay-v1.log')
    require(SHA(catalog['readonly_review']) == catalog['readonly_review_sha256']
            and review['counts'] == dict(readonly_observations=8509, unavailable_value_observations=1193,
                 equal_value_observations=7163, changed_value_observations=153), 'Read-only inventory changed')
    require(review['fields_excluded_as_transient'] == 0, 'Metadata-only persistence exclusion introduced')
    for case in cases.values():
        require(SHA(pin(case['original_case_file'])) == case['original_case_sha256'], 'Original resource fixture changed')
        original = LOAD(case['original_case_file'])
        require(case['setup'] == original['setup']+'Create Variable ReloadAlive;\nReloadAlive = 41;\n'
                and case['selected'] == original['selected']+['ReloadAlive']
                and case['objects'] == original['objects']+['ReloadAlive']
                and case['definitions'] == original['definitions']+['Create Variable ReloadAlive'],
                'Original resource definition/selection altered')
    parent_stage = stage.parent/'coverage95-20261104-attitude-fixed-offset'
    parent = LOAD(pin(parent_stage/'scope-build-receipt-v6-attempt2.json'))
    require(producer['parent_build_receipt_sha256'] == SHA(parent_stage/'scope-build-receipt-v6-attempt2.json'),
            'Different inherited frontend/core producer')
    frozen = Path(parent['frozen_source_root'])
    require(sources[BASE] == (frozen/BASE).read_bytes(), 'Original generic assertions differ from proven source6')
    for name, digest in {**parent['source_sha256'], **parent['backend_sources']}.items():
        require(SHA(frozen/name) == digest, 'Inherited compiler source changed')
    for name, digest in {**producer['binary_sha256'], **producer['inherited_build_inputs']}.items():
        require(SHA(pin(name)) == digest, 'Linked driver/archive/core input changed')
    core_project = stage/'core-build-v1'
    core = LOAD(pin(core_project/'build-receipt-v1.json'))
    require(core['status'] == 'passed' and core['returncodes'] == [0, 0] and core['inputs_unchanged']
            and core['shared_application_binaries_written'] == core['runtime_binary_copies'] == 0,
            'Isolated core producer failed or replaced shared images')
    core_ids = LOAD(pin(core_project/'source-sha256-v1.json'))
    require(core_ids == core['source_sha256'], 'Core source inventory changed')
    with tarfile.open(pin(core_project/'sources-v1.tar.gz')) as archive:
        require(set(archive.getnames()) == set(core_ids), 'Core capsule partition changed')
        for name, digest in core_ids.items():
            require(hashlib.sha256(archive.extractfile(name).read()).hexdigest() == SHA(core_project/'source'/name) == digest,
                    'Compiled isolated core source changed')
    increment = LOAD(pin(stage/'incremental-core-review-v1.json'))
    require(increment['patch_sha256'] == SHA(pin(stage/'incremental-core-v1.patch'))
            and {r['path'] for r in increment['files']} == {'src/base/subscriber/OrbitView.cpp', 'src/base/subscriber/TextEphemFile.cpp'}
            and not increment['original_assertions_changed'] and not increment['layout_change'], 'Core patch expanded or altered assertions/layout')
    changed = {name for name in core_ids if SHA(frozen/name) != core_ids[name]}
    require(changed == {r['path'] for r in increment['files']}, 'Additional core source drift hidden')
    for entry in increment['files']:
        require(SHA(pin(stage/'preimages-v1'/entry['path'])) == SHA(frozen/entry['path']) == entry['before_sha256']
                and SHA(core_project/'source'/entry['path']) == entry['after_sha256'], 'Exact core preimage/increment changed')
    for name, digest in {**core['build_inputs'], **core['binary_sha256'], **core['shared_binary_sha256']}.items():
        require(SHA(pin(name)) == digest, 'Isolated/shared core or SDK input changed')
    symbols = LOAD(pin(stage/'core-symbol-review-v1.json'))
    require(symbols['removed_symbols_count'] == symbols['added_symbols_count'] == 0
            and symbols['old_defined_symbols'] == symbols['new_defined_symbols'] == 19024,
            'Defined core symbol contract changed')
    # Replay the complete prior Save conjunction with its frozen readers.
    previous = stage.parent/'coverage95-20261104-save-force-order'
    verifier = pin(previous/'verify-component-v1.py')
    with tempfile.TemporaryDirectory(prefix='readonly-parent-replay-') as temporary:
        child = subprocess.run([sys.executable, str(verifier), '--output', str(Path(temporary)/'parent.json')],
                               cwd=repo,capture_output=True,text=True)
        require(child.returncode == 0, 'Prior Save component failed: '+child.stdout+child.stderr)
    pin(previous/'component-manifest-v1.json');pin(previous/'component-receipt-v1.json')
    executable = next(p for p in producer['binary_sha256'] if Path(p).name == 'GmatQtNasaSaveReadonlyInitializationTests')
    covered, witnesses, writable, readonly = set(), [], 0, 0
    profiles = [('readonly-initial-headless-v1',3,0,False), ('readonly-remaining-headless-v1',12,1,False),
                ('readonly-unaffected-visible-v1',15,0,False), ('readonly-repaired-headless-v2',3,0,True),
                ('readonly-repaired-visible-v2',3,0,True)]
    failures = []
    for run_name, count, returncode, repaired in profiles:
        run = LOAD(pin(stage/run_name/'results.json'));maps = LOAD(pin(stage/(run_name+'-loaded-images.json')))
        require(maps['returncode'] == returncode and maps['inputs_unchanged'] and maps['runtime_binary_copies'] == 0
                and maps['build_receipt_sha256'] == SHA(project/'build-receipt-v1.json'), 'Execution/map/source guard failed')
        require(maps['source_version'] == ('frontend6-readonly-core1-Save-readonly1' if repaired else 'frontend6-core4-Save-readonly1'),
                'Wrong runtime core version')
        for name, digest in maps['loaded_project_images'].items():
            require(SHA(pin(name)) == maps['expected_project_images'].get(name) == digest, 'Unbound actual runtime image')
        if repaired:
            require(maps['runtime_core_build_receipt_sha256'] == SHA(core_project/'build-receipt-v1.json')
                    and maps['runtime_core_inputs_unchanged'] and not maps['original_shared_core_loaded']
                    and maps['loaded_project_images'].get(maps['runtime_core_image']) == maps['runtime_core_sha256'],
                    'Repaired cases did not load the isolated core exclusively')
        selected = [r for r in run['results'] if r['status'] != 'not-run']
        bad = [r for r in selected if r['status'] != 'passed']
        require(len(bad) == (3 if returncode else 0), 'Unexpected failed case omitted')
        for result in bad:
            key = result['name'].removeprefix(PREFIX)
            require(key in REPAIRED, 'Unaccounted native failure')
            work = Path(result['log']).parent
            for name in ['test.log','result.json','readonly-checks.json','runtime-native.json','reloaded-native.json','reload-probe-native.json']:
                pin(work/name)
            require('FAIL Read-only value not reconstructed by real reload consumer' in (work/'test.log').read_text(),
                    'Original read-only failure lost')
            failures.append(key)
        passed = [r for r in selected if r['status'] == 'passed']
        require(len(passed) == count, 'Incomplete successful case selection')
        processes = [p for p in maps['processes'] if p['executable'] == executable]
        require(len(processes) == len(selected), 'Actual native process cardinality changed')
        mode = run['mode']
        for result in passed:
            key = result['name'].removeprefix(PREFIX);case = cases[key]
            require((mode,key) not in covered and (key in REPAIRED) == repaired, 'Duplicated or wrongly versioned case')
            covered.add((mode,key));work = Path(result['log']).parent
            for path in work.iterdir():
                if path.is_file() and path.suffix in {'.json','.jsonl','.script','.data','.txt','.xml','.log'}:pin(path)
            writable += helper.check_case(case,result,sources,artifacts,fixture_path=FIXTURE)
            proof = LOAD(work/'readonly-proof.json');checks = LOAD(work/'readonly-checks.json')
            require([p for p in nasa.proof_records(result) if p.get('checkpoint') == 'save-readonly-initialization'] == [proof]
                    and proof['outcome'] == 'passed' and proof['case'] == key and proof['relative_bound'] == 1e-12
                    and proof['checks'] == checks and len(checks) == len(case['readonly_roundtrip_fields']), 'Readonly terminal proof incomplete')
            a,b = LOAD(work/'runtime-native.json'),LOAD(work/'reload-probe-native.json')
            for request,check in zip(case['readonly_roundtrip_fields'],checks):
                left,lf = field(a,request);right,rf = field(b,request)
                require(left['name'] == right['name'] and all(lf[k] == rf[k] for k in ['id','name','type','read_only','cloaked'])
                        and equal(lf['value'],rf['value']) and check == dict(request=request,before=lf['value'],after=rf['value']),
                        'Typed runtime readonly identity/metadata/value was not reconstructed')
                readonly += 1
            process = [p for p in processes if p['command'][-1] == key]
            require(len(process) == 1, 'No unique actual test process')
            if mode == 'visible':
                display = run['visible_display']
                require(result['requested_visibility'] == 'visible' and result['trace']['native_exposure_events'] > 0
                        and display['start_minimized'] and display['resizable'] and run['resizable']
                        and 'Iconic' in display['initial_host_window_state']
                        and '_NET_WM_STATE_FULLSCREEN' not in display['initial_host_window_state'], 'Visible profile not native/minimized/resizable/nonfullscreen')
            witnesses.append(dict(profile=mode,case=key,pid=process[0]['pid'],core='repaired-isolated' if repaired else 'source4',proof=str(work/'readonly-proof.json')))
    require(covered == {(mode,key) for mode in ['headless','visible'] for key in cases}
            and set(failures) == REPAIRED and readonly == 130, 'Missing full case/profile/failure conjunction')
    # Reuse the already captured post-reload values from six older cases.
    visible_lookup = {}
    p = stage.parent/'coverage95-20261101/save-orbit-modes-evidence.json'
    while p:
        evidence = LOAD(pin(p))
        for entry in evidence['selected']['visible']:
            visible_lookup[entry['test']] = p.parent/entry['run']/entry['test']
        parent_path = evidence.get('parent_evidence') or evidence.get('parent_conjoined_evidence')
        p = Path(parent_path) if parent_path else None
    fixed = LOAD(pin(stage.parent/'coverage95-20261104-save-fixed-offset/evidence-replay-v2.json'))
    for witness in fixed['witnesses']:
        if witness['profile'] == 'visible':visible_lookup['QtGui.NasaSaveFixedOffset'+witness['case']] = Path(witness['proof']).parent
    reused = 0
    for entry in catalog['reused_headless_probe_observations']:
        request = dict(object=entry['object_path'].split('/')[0],
                       owned_indices=[int(x[6:-1]) for x in entry['object_path'].split('/')[1:]],
                       native_type=entry['native_type'],field=entry['field'],type=entry['parameter_type'])
        for work in [Path(entry['runtime']).parent, visible_lookup[entry['test']]]:
            a,b = LOAD(pin(work/'runtime-native.json')),LOAD(pin(work/'reload-probe-native.json'))
            left,lf = field(a,request);right,rf = field(b,request)
            require(left['name'] == right['name'] and equal(lf['value'],rf['value']), 'Retained post-initialization witness failed')
            reused += 1
    require(reused == 132 and len(catalog['pending_source_review_observations']) == 22, 'Reuse/pending scope changed')
    native = stage/'native-support-v3';execution = LOAD(pin(native/'execution-v1.json'))
    require(execution['returncode'] == 0 and execution['inputs_unchanged'] and execution['GUI_contexts'] == 0
            and not execution['original_shared_core_loaded'] and execution['loaded_project_images'] == execution['expected_images'],
            'Native copy/filename regression or actual core binding failed')
    for name,digest in execution['loaded_project_images'].items():require(SHA(pin(name)) == digest,'Native support image changed')
    log = pin(native/'native-v1.log').read_text()
    require('"status":"passed","assertions":166' in log, 'Native copy/assignment/path/removed-interface assertions incomplete')
    for path in [native/'build-receipt-v3.json',stage/'native-support-v1/build-receipt-v1.json',
                 stage/'native-support-v1/build-v1.log',stage/'native-support-v2/execution-v1.json',
                 stage/'native-support-v2/native-v1.log',stage/'native-regression-lookup-review-v3.json']:
        pin(path)
    record = dict(status='passed',contract='bounded-readonly-reconstruction-and-two-core-repairs',
        original_row='FRC!E132',authority_sha256=inventory['sources'],new_cases=18,new_contexts=36,
        new_readonly_field_comparisons=readonly,reused_readonly_field_comparisons=reused,
        writable_property_comparisons=writable,native_support_assertions=166,original_failures_retained=sorted(failures),
        core_changed_paths=sorted(changed),original_generic_source_bytes_preserved=True,
        original_assertions_changed=0,runtime_binary_copies=0,shared_application_binaries_written=0,
        pending_observed_readonly_changes=22,unavailable_readonly_observations=1193,unavailable_descriptor_keys=238,
        fields_excluded_as_transient=0,witnesses=witnesses,artifacts=artifacts,native_tests_rerun_by_reader=0,
        new_requirement_credit=0,whole_FRC_E132_qualified=False,FRC_E135_qualified=False,scientific_approval=False,
        technical_core_approval=False,full_external_ABI_qualified=False,full_current_build_qualified=False,
        full_NASA_qualified=False,goal_complete=False,reader_sha256=SHA(__file__))
    with output.open('x') as stream:json.dump(record,stream,indent=2);stream.write('\n')
    return record


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--stage',type=Path,default=REPO/'build/nasa-coverage/coverage95-20261104-save-readonly')
    parser.add_argument('--output',type=Path,required=True)
    args = parser.parse_args();r = replay(args.stage.resolve(),args.output.resolve())
    print(json.dumps({k:r[k] for k in ['status','new_cases','new_contexts','new_readonly_field_comparisons',
          'reused_readonly_field_comparisons','writable_property_comparisons','native_support_assertions','new_requirement_credit']}))
