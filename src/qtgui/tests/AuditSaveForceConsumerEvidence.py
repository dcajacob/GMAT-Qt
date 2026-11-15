"""Replay the affected CombinedNRL Save consumers; complete FRC!E132 stays open."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile

import NasaGuiCoverage as nasa
from AuditSaveResourceEvidence import compare

REPO = Path(__file__).resolve().parents[3]
SHA = lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest()
LOAD = lambda p: json.loads(Path(p).read_text())
DRIVER = 'src/qtgui/tests/NasaSaveForceConsumerTests.cpp'
GENERIC = 'src/qtgui/tests/NasaSaveResourceTests.cpp'
FIXTURE = 'src/qtgui/tests/NasaSaveForceConsumerCases.json'
NAME = 'QtGui.NasaSaveForceConsumerCombinedNRL'
ORDER = [['GravityField', 'GravityField.Earth'], ['PointMassForce', 'PointMassForce.Luna'],
         ['SolarRadiationPressure', 'SRP.Earth'], ['DragForce', 'Drag.Earth'],
         ['RelativisticCorrection', 'RelativisticCorrection.Earth']]


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def numerical(a, b, path='value'):
    if isinstance(a, list):
        require(isinstance(b, list) and len(a) == len(b), 'Changed vector dimension: '+path)
        pairs = [numerical(x, y, path+'/'+str(i)) for i, (x, y) in enumerate(zip(a, b))]
        return sum(p[0] for p in pairs), max((p[1] for p in pairs), default=0)
    require(type(a) in {int, float} and type(b) in {int, float}
            and math.isfinite(a) and math.isfinite(b), 'Nonfinite/non-numeric consumer value: '+path)
    error = abs(a-b)
    require(error <= 1e-12*max(1, abs(a)), 'Original strict comparison bound exceeded: '+path)
    return 1, error


def one_pair(a, b, seconds, clock_bound):
    require(a.keys() == b.keys() and a['seconds'] == b['seconds'] == seconds
            and a['dimension'] == b['dimension'] == 42, 'Substituted native consumer sample')
    require(a['force_order'] == ORDER and b['force_order'] == ORDER[:3]+[ORDER[4], ORDER[3]],
            'The affected native force-order change was not exercised')
    components, maxima = 0, {}
    for field, size in [('state', 6), ('STM', 6), ('packed_state', 42), ('derivative', 42)]:
        require(isinstance(a[field], list) and len(a[field]) == size, 'Incomplete native consumer vector: '+field)
    require(all(isinstance(row, list) and len(row) == 6 for row in a['STM']+b['STM']), 'Incomplete STM rows')
    for field in ['epoch', 'state', 'STM', 'packed_state', 'derivative', 'integrator_seconds', 'mass', 'DragArea', 'SRPArea']:
        count, maxima[field] = numerical(a[field], b[field], field)
        components += count
    require(abs(a['integrator_seconds']-seconds) < clock_bound
            and abs(b['integrator_seconds']-seconds) < clock_bound, 'Retained endpoint clock limit exceeded')
    for sample in [a, b]:
        require(sample['DragArea'] > 0 and sample['SRPArea'] > 0 and sample['mass'] > 0,
                'Null-area/mass substitute for affected force consumer')
        require(abs(sample['epoch']-(21545+sample['integrator_seconds']/86400)) <= 1e-10,
                'Native spacecraft/integrator epochs disagree')
        numerical(sample['state'], sample['packed_state'][:6], 'native/packed state')
        numerical(sum(sample['STM'], []), sample['packed_state'][6:], 'native/packed STM')
        for key in ['DragForce:Drag.Earth', 'RelativisticCorrection:RelativisticCorrection.Earth']:
            require(len(sample['force_derivatives'][key]) == 6
                    and any(abs(v) > 0 for v in sample['force_derivatives'][key][3:]), 'Reordered force is inactive')
    require(set(a['force_derivatives']) == set(b['force_derivatives']) == {t+':'+n for t, n in ORDER},
            'Changed/missing individual force identity')
    for force in a['force_derivatives']:
        require(len(a['force_derivatives'][force]) == len(b['force_derivatives'][force]) == 6,
                'Incomplete individual-force derivative')
        count, maxima[force] = numerical(a['force_derivatives'][force], b['force_derivatives'][force], force)
        components += count
    return components, maxima


def replay(stage, output):
    repo = stage.parents[2]
    artifacts = {}

    def pin(path):
        path = Path(path).resolve()
        artifacts[str(path)] = SHA(path)
        return path

    inventory = nasa.inventory(repo)
    project = stage/'build-v5'
    producer = LOAD(pin(project/'build-receipt-v5.json'))
    require(producer['status'] == 'passed' and producer['returncodes'] == [0, 0] and producer['inputs_unchanged']
            and producer['runtime_binary_copies'] == producer['shared_application_binaries_written'] == 0,
            'Save consumer producer failed or replaced shared images')
    identities = LOAD(pin(project/'driver-source-sha256-v5.json'))
    require(identities == producer['driver_source_sha256'], 'Consumer compiler source closure changed')
    with tarfile.open(pin(project/'driver-source-v5.tar.gz')) as archive:
        require(set(archive.getnames()) == set(identities), 'Substituted consumer capsule')
        sources = {name: archive.extractfile(name).read() for name in identities}
    for name, digest in identities.items():
        require(hashlib.sha256(sources[name]).hexdigest() == SHA(pin(project/'source'/name)) == digest,
                'Compiled consumer input changed')
    for name, digest in {**producer['binary_sha256'], **producer['inherited_build_inputs']}.items():
        require(SHA(pin(name)) == digest, 'Compiler/link/runtime input changed')
    parent_stage = stage.parent/'coverage95-20261104-attitude-fixed-offset'
    parent = LOAD(pin(parent_stage/'scope-build-receipt-v6-attempt2.json'))
    require(producer['parent_build_receipt_sha256'] == SHA(parent_stage/'scope-build-receipt-v6-attempt2.json')
            and producer['inherited_frontend_source_version'] == 6 and producer['inherited_core_source_version'] == 4,
            'Inherited source6/source4 producer changed')
    frozen = Path(parent['frozen_source_root'])
    require(sources[GENERIC] == (frozen/GENERIC).read_bytes(), 'Original shared Save driver/assertions changed')
    for name, digest in {**parent['source_sha256'], **parent['backend_sources']}.items():
        require(SHA(frozen/name) == digest, 'Inherited frozen frontend/core source changed')
    catalog = json.loads(sources[FIXTURE])
    for name, digest in catalog['source_sha256'].items():
        require(SHA(pin(repo/name)) == digest, 'Original force/clock/helper source changed')
    original = next(c for c in LOAD(repo/'src/qtgui/tests/NasaSaveForceCases.json')['cases'] if c['key'] == 'CombinedNRL')
    case = catalog['cases'][0]
    require(len(catalog['cases']) == 1 and case['key'] == 'CombinedNRL'
            and case['original_force_case'] == original and case['setup'] == original['setup'],
            'Original CombinedNRL force parameters changed')
    clock = LOAD(repo/catalog['clock_source'])
    require(catalog['relative_bound'] == 1e-12 and catalog['elapsed_bound_seconds'] == clock['elapsed_bound_seconds'] == 1e-5
            and catalog['durations_seconds'] == [1, 60, 600], 'Changed/omitted strict bound or consumer interval')
    review = LOAD(pin(stage/'new-clock-expectation-review-v3.json'))
    require(not review['original_NASA_assertions_changed'] and not review['original_Save_assertions_changed']
            and review['paired_state_STM_derivative_relative_bound_unchanged'] == 1e-12, 'New clock correction altered original assertions')
    for version in [1, 2]:
        run = stage/('consumer-headless-v'+str(version))
        pin(run/'results.json');pin(stage/(run.name+'-loaded-images.json'))
        log = pin(run/NAME/'test.log').read_text()
        require('FAIL Save consumer actual integrator/time differs' in log, 'Prior new-probe failure was lost')
        pin(stage/('build-v'+str(version))/('driver-source-v'+str(version)+'.tar.gz'))
        for path in (run/NAME).glob('*.json'):pin(path)
    schema_review = LOAD(pin(stage/'new-proof-schema-review-v4.json'))
    require(not schema_review['numerical_assertions_changed'] and not schema_review['production_changed']
            and schema_review['patch_sha256'] == SHA(pin(stage/'new-proof-facets-v4.patch')),
            'Proof schema correction changed numerical scope')
    pin(stage/'failed-reader-v1.py');pin(stage/'evidence-replay-v1.log')
    for mode in ['headless', 'visible']:
        old_run = stage/('consumer-'+mode+'-v3')
        old = LOAD(pin(old_run/'results.json'))
        old_selected = [r for r in old['results'] if r['status'] != 'not-run']
        require(len(old_selected) == 1 and old_selected[0]['status'] == 'passed'
                and nasa.proof_records(old_selected[0]) == [], 'Earlier raw-only proof was relabeled')
        pin(stage/(old_run.name+'-loaded-images.json'));pin(old_run/NAME/'test.log')
        for path in (old_run/NAME).glob('*.json'):pin(path)
    runtime_review = LOAD(pin(stage/'new-runtime-reference-review-v5.json'))
    require(not runtime_review['original_assertions_changed'] and not runtime_review['property_identity_check_relaxed']
            and not runtime_review['numerical_bounds_changed'] and not runtime_review['production_changed']
            and runtime_review['patch_sha256'] == SHA(pin(stage/'new-runtime-reference-v5.patch')),
            'Runtime reference correction relaxed an original comparison')
    pin(stage/'failed-reader-v2.py')
    require('Native resource identity changed: Prop/owned[0]' in pin(stage/'evidence-replay-v2.log').read_text(),
            'Strict identity audit failure lost')
    for mode in ['headless', 'visible']:
        old_run = stage/('consumer-'+mode+'-v4')
        old = LOAD(pin(old_run/'results.json'))
        selected_old = [r for r in old['results'] if r['status'] != 'not-run']
        require(len(selected_old) == 1 and selected_old[0]['status'] == 'passed', 'Prior mixed-context native run lost')
        pin(stage/(old_run.name+'-loaded-images.json'));pin(old_run/NAME/'test.log')
        for path in (old_run/NAME).glob('*.json'):pin(path)
    previous = stage.parent/'coverage95-20261104-save-fixed-offset'
    previous_verifier = pin(previous/'verify-component-v1.py')
    with tempfile.TemporaryDirectory(prefix='save-parent-replay-') as temporary:
        result = subprocess.run([sys.executable, str(previous_verifier), '--output', str(Path(temporary)/'parent.json')],
                                cwd=repo, capture_output=True, text=True)
        require(result.returncode == 0, 'Retained152-case conjunction replay failed: '+result.stdout+result.stderr)
        parent_component = LOAD(Path(temporary)/'parent.json')
    require(parent_component['cumulative_Save_cases'] == 152 and parent_component['new_requirement_credit'] == 0,
            'Prior Save conjunction changed')
    pin(previous/'component-receipt-v1.json');pin(previous/'component-manifest-v1.json')
    executable = next(p for p in producer['binary_sha256'] if Path(p).name == 'GmatQtNasaSaveForceConsumerTests')
    paired_components = properties = missions = 0
    samples = []
    for mode in ['headless', 'visible']:
        run_name = 'consumer-'+mode+'-v5'
        run = LOAD(pin(stage/run_name/'results.json'))
        maps = LOAD(pin(stage/(run_name+'-loaded-images.json')))
        require(run['mode'] == mode and maps['returncode'] == 0 and maps['inputs_unchanged']
                and maps['runtime_binary_copies'] == 0 and maps['source_version'] == 'frontend6-core4-Save-consumer5'
                and maps['build_receipt_sha256'] == SHA(project/'build-receipt-v5.json'), 'Failed/substituted consumer execution')
        for name, digest in maps['loaded_project_images'].items():
            require(SHA(pin(name)) == maps['expected_project_images'].get(name) == digest, 'Unbound actual native image')
        for name, digest in maps['ctest_definition_sha256'].items():
            require(SHA(pin(name)) == digest, 'Executed registration changed')
        selected = [r for r in run['results'] if r['status'] != 'not-run']
        processes = [p for p in maps['processes'] if p['executable'] == executable]
        require(len(selected) == len(processes) == 1 and selected[0]['name'] == NAME
                and selected[0]['status'] == 'passed' and selected[0]['returncode'] == 0,
                'Missing/extra/failed native consumer context')
        result = selected[0]
        require(processes[0]['command'][-1] == 'CombinedNRL' and processes[0]['snapshots'], 'Native consumer maps absent')
        work = Path(result['log']).parent
        for path in work.iterdir():
            if path.is_file() and path.suffix in {'.json', '.jsonl', '.script', '.data', '.txt', '.xml', '.log'}:pin(path)
        proof = LOAD(work/'proof.json')
        require([p for p in nasa.proof_records(result) if p.get('checkpoint') == 'save-force-consumer'] == [proof]
                and proof['case'] == 'CombinedNRL' and proof['outcome'] == 'passed'
                and proof['relative_bound'] == 1e-12 and proof['missions'] == 7
                and proof['export_missions'] == 1 and proof['consumer_missions'] == 6
                and proof['facets'] == ['actual-GUI-Build-Run-and-Save-before-propagation',
                    'standalone-export-rebuild-and-complete-writable-native-property-comparison',
                    'active-five-force-original-and-rebuilt-Cartesian-STM-and-derivative-consumers',
                    'unchanged-relative-comparison-and-retained-clock-fixture-bound',
                    'exact-source-history-and-following-statement-preservation']
                and LOAD(work/'case.json') == case and LOAD(work/'checks.json') == proof['checks'], 'Native proof/fixture differs')
        total = 0
        raw_exports = []
        saved_runtime = LOAD(work/'saved-runtime-native.json')
        require((work/'export-only.script').read_text() == case['setup']+case['spacecraft_setup']+
                "Create Variable Keep;\nKeep = 41;\nBeginMissionSequence;\nSave 'ExportFamily' FM Prop Sat;\nKeep = Keep + 1; % export sentinel Ω\n",
                'Export-only runtime capture command stream changed')
        for seconds, check in zip(catalog['durations_seconds'], proof['checks']):
            label = str(seconds)
            before, after = [LOAD(work/(label+'-'+phase+'-native.json')) for phase in ['original', 'reloaded']]
            require(set(before) == set(after) == {'FM', 'Prop', 'Sat'}, 'Missing/extra saved resource root')
            require(before == saved_runtime, 'Configured or propagated object substituted for actual exported runtime state')
            count = sum(compare(before[n], after[n], n) for n in before)
            require(count == check['writable_properties_checked'] > 0
                    and LOAD(work/(label+'-native-differences.json')) == LOAD(work/(label+'-native-getter-gaps.json')) == [],
                    'Incomplete/failed native property comparison')
            total += count
            raw = (work/(label+'-export.data')).read_text();raw_exports.append(raw)
            require(raw == (work/'export.data').read_text(), 'Consumer used another export')
            keep = 'Create Variable Keep;\nKeep = 41;\n'
            tail = "Propagate 'Consumer' Prop(Sat, STM) {Sat.ElapsedSecs = "+label+"};\nKeep = Keep + 1; % following sentinel Ω\n"
            require((work/(label+'-original.script')).read_text() == case['setup']+case['spacecraft_setup']+keep+
                    "BeginMissionSequence;\n"+tail
                    and (work/(label+'-reloaded.script')).read_text() == raw+'\n'+keep+'BeginMissionSequence;\n'+tail,
                    'Changed original/standalone-export consumer command stream')
            a, b = [LOAD(work/(label+'-'+phase+'-consumer.json')) for phase in ['original', 'reloaded']]
            components, maxima = one_pair(a, b, seconds, catalog['elapsed_bound_seconds'])
            paired_components += components
            require(check['seconds'] == seconds and check['original_order'] == a['force_order']
                    and check['reloaded_order'] == b['force_order']
                    and check['state_STM_derivative_and_force_components_equal_with_original_bound'] is True,
                    'Native per-interval order/comparison record changed')
            samples.append(dict(profile=mode, seconds=seconds, pid=processes[0]['pid'], scalar_components=components,
                                maximum_absolute_differences=maxima, integrator_seconds=[a['integrator_seconds'], b['integrator_seconds']]))
        require(len(proof['checks']) == 3 and len(set(raw_exports)) == 1
                and proof['writable_native_properties_checked'] == total, 'Incomplete interval/export/property conjunction')
        properties += total;missions += proof['missions']
        if mode == 'visible':
            display = run['visible_display']
            require(result['requested_visibility'] == 'visible' and result['trace']['native_exposure_events'] > 0
                    and run['resizable'] and display['resizable'] and display['start_minimized']
                    and 'Iconic' in display['initial_host_window_state']
                    and '_NET_WM_STATE_FULLSCREEN' not in display['initial_host_window_state'],
                    'Missing actual initially-minimized/resizable/nonfullscreen visible exposure')
    require(missions == 14 and properties == 648 and paired_components == 966, 'Consumer conjunction cardinality changed')
    record = dict(status='passed', contract='bounded-CombinedNRL-Save-derivative-propagation-STM-consumers',
        original_row='FRC!E132', authority_sha256=inventory['sources'], affected_force_cases=1, qualified_profiles=2,
        compared_intervals=6, actual_missions=missions, actual_export_missions=2, actual_consumer_missions=12, writable_property_comparisons=properties,
        paired_native_scalar_comparisons=paired_components, prior_Save_cases_reused=152,
        observed_order_change_retained=True, original_assertions_changed=0, production_changes=0,
        samples=samples, artifacts=artifacts, native_tests_rerun_by_reader=0, new_requirement_credit=0,
        universal_force_order_equivalence=False, whole_FRC_E132_qualified=False, FRC_E135_qualified=False,
        scientific_approval=False, technical_core_approval=False, external_ABI_qualified=False,
        full_current_build_qualified=False, full_NASA_qualified=False, goal_complete=False, reader_sha256=SHA(__file__))
    with output.open('x') as stream:json.dump(record,stream,indent=2);stream.write('\n')
    return record


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--stage',type=Path,default=REPO/'build/nasa-coverage/coverage95-20261104-save-force-order')
    parser.add_argument('--output',type=Path,required=True)
    args = parser.parse_args();record = replay(args.stage.resolve(),args.output.resolve())
    print(json.dumps({k:record[k] for k in ['status','qualified_profiles','actual_missions',
          'writable_property_comparisons','paired_native_scalar_comparisons','new_requirement_credit']}))
