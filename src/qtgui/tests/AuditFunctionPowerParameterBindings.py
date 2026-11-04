"""Audit bounded main/private/Global nuclear power Parameter evidence."""
import argparse
import copy
import hashlib
import json
import re
import tarfile
from pathlib import Path
import xml.etree.ElementTree as ET

from BuildNasaCoverageExtension import load, require, sha
from AuditFunctionParameterBindings import compare, written

TYPES = {'TotalPowerAvailable', 'RequiredBusPower', 'ThrustPowerAvailable'}
SUFFIX = 'SelfReferencedEpoch'
PREFIX = 'QtGui.NasaFunctionPowerParameterBinding'


def expected(key, shared, inside, calls):
    index = 2 if shared else 1 if inside else 0
    epoch = [21545, 21910.25, 22275.5][index] + (calls * 365.25 if inside else 0)
    generated = [10, 20, 30][index] * .9 ** ((epoch - 21545) / 365.25)
    bus, margin = [2, 4, 20][index], [10, 20, 25][index]
    value = generated if key == 'TotalPowerAvailable' else bus if key == 'RequiredBusPower' else max(0, (1 - margin / 100) * (generated - bus))
    return value, epoch, dict(InitialMaxPower=[10, 20, 30][index], AnnualDecayRate=10, Margin=margin, BusCoeff1=bus, BusCoeff2=0, BusCoeff3=0)


def contract(p, spec):
    key, meta = spec['factory_type'], spec['metadata']
    require(p['checkpoint'] == 'live-function-power-parameter-owner-binding' and p['outcome'] == 'passed' and p['case'] == key and p['argument'] == key + SUFFIX and p['epoch_assignment_mode'] == 'self-referenced-increments' and p['family'] == 'AttachedNuclearPower' and p['route'] == 'attached-nuclear-power-derived-field', 'Power proof identity differs')
    require(p['full_system_parameter_inventory'] == 321 and set(p['initial_batch_types']) == TYPES and p['missions'] == 3 and p['function_calls'] == p['live_callback_samples'] == 12 and p['actual_parameter_owner_checks'] == 24 and p['file_decisions'] == [dict(saved=False), dict(saved=True)], 'Power census differs')
    require([v['phase'] for v in p['phases']] == [0, 1, 2] and [v['explicit_rebuild'] for v in p['phases']] == [True, True, False], 'Power no-Build repeat lost')
    for phase in p['phases']:
        require(phase['raw_markers'] == [42, 43, 44, 103, 104] and phase['callback_private_maps_accessed_only_while_executing'] and len(phase['live_samples']) == len(phase['private_writes']) == len(phase['shared_writes']) == 4, 'Power callbacks/markers differ')
        roots, attached_roots = {}, {}
        for i, sample in enumerate(phase['live_samples']):
            inside, calls = i > 0, i
            target = 'ScopePulse' if inside else 'MainPulse'
            require(sample['inside_function'] == inside and sample['function'] == ('ReadShared' if inside else '') and sample['calls'] == calls and sample['global_map_is_value_snapshot'] == (not inside) and len(sample['parameters']) == 2 and sample['provider'] == f'Propagate PulseProp({target}) {{{target}.ElapsedSecs = 1}};', 'Power scope/provider differs')
            for j, row in enumerate(sample['parameters']):
                shared = j == 1
                value, epoch, inputs = expected(key, shared, inside, calls)
                ref = spec['shared_reference'] if shared else spec['local_private_reference'] if inside else spec['root_private_reference']
                require(row['shared'] == shared and row['reference'] == ref and row['factory_type'] == key and row['native_type'] == meta['native_type'] and row['return_type'] == meta['return_type'] == 4 and row['owner_type'] == meta['owner_type_id'] == 101 and row['owner_type_name'] == 'Spacecraft' and row['owner_name'] == ('SharedSC' if shared else 'PrivateOnly'), 'Power Parameter/owner metadata differs')
                for field in ['parameter_pointer', 'reference_owner_pointer', 'live_owner_pointer', 'root_owner_pointer', 'configured_owner_pointer']:
                    require(re.fullmatch('[0-9a-f]+', row[field]) and row[field] != '0', 'Invalid power owner identity')
                require(row['reference_owner_pointer'] == row['live_owner_pointer'] != row['configured_owner_pointer'] and row['owner_global'] == shared and row['owner_local'] == (inside and not shared), 'Power reference/flags differ')
                if not inside:
                    roots[shared] = row['root_owner_pointer']
                require(row['root_owner_pointer'] == roots[shared] and (row['live_owner_pointer'] == roots[shared]) == (shared or not inside), 'Power owner aliases wrong root')
                require(row['shape'] == [] and row['absolute_bound'] == 1e-12, 'Power shape/bound differs')
                for field in ['actual', 'expected', 'direct_owner_property_value']:
                    compare(row[field], value, 1e-12)
                compare((phase['shared_writes'] if shared else phase['private_writes'])[i], value, 1e-12)
                a = row['attached']
                name = ('Shared' if shared else 'Local' if inside else 'Root') + 'Power'
                require(a['name'] == name and a['type'] == spec['hardware_type'] == 'NuclearPowerSystem' and a['property'] == key, 'Attached power name/type differs')
                for field in ['pointer', 'root_pointer', 'configured_pointer']:
                    require(re.fullmatch('[0-9a-f]+', a[field]) and a[field] != '0', 'Invalid attached power identity')
                if not inside:
                    attached_roots[shared] = a['root_pointer']
                require(a['root_pointer'] == attached_roots[shared] and a['pointer'] != a['configured_pointer'] and (a['pointer'] == a['root_pointer']) == (shared or not inside), 'Power clone aliases wrong root')
                compare(a['direct_value'], value, 1e-12)
                require(a['owner_epoch'] == epoch and a['inputs'] == inputs and a['epoch_format'] == 'A1ModJulian' and a['initial_epoch'] == '21545', 'Independent epoch/decay/bus/margin recipe differs')
                require(a['runtime_settable'] is False and a['info_settable'] is False and a['native_property_readonly'] is True and meta['settable'] is False, 'Read-only power flags differ')


def audit(repo, stage):
    bindings = load(stage / 'evidence-inputs-v1.json')['artifact_sha256']
    for name, digest in bindings.items():
        require(Path(name).is_file() and not Path(name).is_symlink() and sha(name) == digest, 'Changed regular binding ' + name)

    def bound(path):
        path = Path(path)
        require(str(path) in bindings, 'Unbound ' + str(path))
        return path

    def capsule(name):
        with tarfile.open(bound(stage / name)) as t:
            require(all(v.isfile() for v in t.getmembers()), 'Nonregular source member')
            return {v.name: t.extractfile(v).read() for v in t.getmembers()}

    visible = capsule('visible-executed-sources-v1.tar.gz')
    before = load(bound(stage / 'visible-inputs-before-v1.json'))
    require(before == load(bound(stage / 'visible-inputs-after-v1.json')) and before['sources'] == {n: hashlib.sha256(v).hexdigest() for n, v in visible.items()}, 'Visible inputs changed')
    for n in ['AuditFunctionParameterBindings.py', 'BuildNasaCoverageExtension.py', 'AuditFunctionPowerParameterBindings.py']:
        require(hashlib.sha256(visible['src/qtgui/tests/' + n]).hexdigest() == sha(repo / 'src/qtgui/tests' / n), 'Current audit helper differs')
    catalog = json.loads(visible['src/qtgui/tests/NasaFunctionPowerParameterCases.json'])
    specs = {v['factory_type']: v for v in catalog['power_binding_cases']}
    require(len(catalog['cases']) == 321 and set(catalog['initial_types']) == set(specs) == TYPES and set(catalog['selected_arguments']) == {v + SUFFIX for v in TYPES} and set(catalog['retained_absolute_epoch_probe_arguments']) == TYPES and not catalog['qualified_keys'] and all(not v['qualified'] for v in catalog['cases']), 'Complete321 or retained probes changed')
    baseline = load(bound(repo / catalog['baseline']))
    require(sha(repo / catalog['baseline']) == catalog['baseline_sha256'] and {v['factory_type']: v['native_parameter_metadata'] for v in baseline['rows'] if v['role'] == 'owner-bound-system-parameter'} == {v['factory_type']: v['metadata'] for v in catalog['cases']}, 'Original metadata changed')
    maps = load(bound(stage / 'loaded-images-v1.json'))['processes']
    launch = load(bound(stage / 'visible-launch-v1.json'))
    require(launch['returncode'] == 0 and launch['inputs_unchanged'] and launch['mapped_processes'] == len(maps) == 3, 'Visible mapping incomplete')
    selected, failures, exposures = [], [], 0
    for version in ['headless-v1', 'headless-v2', 'visible-v1']:
        inv = 'function-power-parameter-bindings-' + version
        q = load(bound(repo / 'build/gui-acceptance' / inv / 'results.json'))
        frozen = visible if version == 'visible-v1' else capsule(version.replace('headless-', 'headless-executed-sources-') + '.tar.gz')
        require(q.get('finished_utc') and q['end_reason'] == 'completed-selection' and all(n in frozen and hashlib.sha256(frozen[n]).hexdigest() == d for n, d in q['source_hashes'].items()), 'Executed source/terminal state differs')
        rows = {v['name']: v for v in q['results'] if v['status'] != 'not-run'}
        require(set(rows) == {PREFIX + n + ('' if version == 'headless-v1' else SUFFIX) for n in TYPES}, 'Exact power selection differs')
        for name, row in rows.items():
            case, item = bound(row['log']).parent, dict(invocation=inv, test=name)
            key = name.removeprefix(PREFIX).removesuffix(SUFFIX)
            if version == 'headless-v1':
                text = bound(case / '0-messages.txt').read_text()
                require(row['status'] == 'failed' and row['returncode'] != 0 and 'GmatCommand::SetWrapperReferences failed to find object named "PrivateOnly.A1ModJulian"' in text and 'PrivateOnly.A1ModJulian = 21910.25 + Calls * 365.25;' in text and len(load(bound(case / '0-live-parameter-samples.json'))) == 1, 'Retained LHS-only epoch failure lost')
                failures.append(item)
                continue
            require(row['status'] == 'passed' and row['returncode'] == 0, 'Selected power case failed')
            tests = list(ET.parse(bound(case / 'ctest.xml')).getroot().iter('testcase'))
            require(len(tests) == 1 and tests[0].get('name') == name and tests[0].find('failure') is None and tests[0].find('skipped') is None, 'Power native JUnit differs')
            emitted = [json.loads(v) for v in re.findall(r'NASA_CHECK (\{[^\n]+\})', bound(row['log']).read_text())]
            require(len(emitted) == 1 and load(bound(case / 'proof.json')) == emitted[0], 'Power emitted/stored proof differs')
            p, spec = emitted[0], specs[key]
            contract(p, spec)
            require(load(bound(case / 'phases.json')) == p['phases'], 'Raw power phases differ')
            initial, edited = bound(case / 'initial.gmf').read_text(), bound(case / 'edited.gmf').read_text()
            require(initial.count('Keep = 103;') == 1 and initial.replace('Keep = 103;', 'Keep = 41;') == edited and 'PrivateOnly.A1ModJulian = PrivateOnly.A1ModJulian + Calls * 365.25;' in edited and 'SharedSC.A1ModJulian = SharedSC.A1ModJulian + 365.25;' in edited, 'Power file edit/epoch mode differs')
            for phase in p['phases']:
                tag = str(phase['phase'])
                messages = bound(case / (tag + '-messages.txt')).read_text()
                require(load(bound(case / (tag + '-live-parameter-samples.json'))) == phase['live_samples'] and all(load(bound(case / (tag + '-invariants.json'))).values()) and load(bound(case / (tag + '-prototypes.json'))) == load(bound(case / 'prototypes-before.json')), 'Raw power samples/invariants/prototypes differ')
                require(written(messages, spec, spec['root_private_reference']) + written(messages, spec, spec['local_private_reference']) == phase['private_writes'] and written(messages, spec, spec['shared_reference']) == phase['shared_writes'], 'Raw power Write differs')
                compare([float(v) for v in bound(case / (tag + '-binding-values.txt')).read_text().split()], [v['actual'] for sample in phase['live_samples'] for v in sample['parameters']], 1e-12)
                require([float(v) for v in bound(case / (tag + '-values.txt')).read_text().split()] == phase['raw_markers'], 'Raw power markers differ')
            if version == 'visible-v1':
                d = q['visible_display']
                require(d['start_minimized'] and d['resizable'] and 'Iconic' in d['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN' not in d['initial_host_window_state'] and q['max_screenshots'] == 0 and row['trace']['native_exposure_events'] > 0 and not row['trace']['screenshots'], 'Visible policy differs')
                exposures += row['trace']['native_exposure_events']
                traces = list(case.glob('trace-*.jsonl'))
                require(len(traces) == 1, 'Visible trace missing')
                mapped = maps[traces[0].stem.removeprefix('trace-')]
                exes = [n for n in row['registered_command'] if n in q['binary_hashes']]
                require(len(exes) == 1 and mapped['executable'] == exes[0] and all(before['images'].get(n) == d for n, d in mapped['images'].items()), 'Actual mapped power image differs')
                for n, d in before['images'].items():
                    if n == exes[0] or Path(n).name.startswith(('libGmatBase', 'libGmatUtil')):
                        require(mapped['images'].get(n) == d, 'Required image unmapped')
            selected.append(item)
    require(len(selected) == 6 and len(failures) == 3, 'Power selection/failure count differs')
    template = load(bound(repo / 'build/gui-acceptance/function-power-parameter-bindings-visible-v1' / (PREFIX + 'ThrustPowerAvailable' + SUFFIX) / 'proof.json'))
    def row(p):
        return p['phases'][0]['live_samples'][1]['parameters'][0]
    controls = [
        ('wrong-owner-reference', lambda p: row(p).__setitem__('reference_owner_pointer', '1')),
        ('private-power-aliases-root', lambda p: row(p)['attached'].__setitem__('pointer', row(p)['attached']['root_pointer'])),
        ('wrong-power-class', lambda p: row(p)['attached'].__setitem__('type', 'SolarPowerSystem')),
        ('wrong-input-power', lambda p: row(p)['attached']['inputs'].__setitem__('InitialMaxPower', 999)),
        ('wrong-controlled-epoch', lambda p: row(p)['attached'].__setitem__('owner_epoch', 21545)),
        ('wrong-derived-value', lambda p: row(p).__setitem__('actual', 999)),
        ('negative-unclamped-power', lambda p: p['phases'][0]['live_samples'][2]['parameters'][1].__setitem__('actual', -.23775)),
        ('writable-derived-power', lambda p: row(p)['attached'].__setitem__('runtime_settable', True)),
        ('missing-no-Build', lambda p: p['phases'].pop()),
        ('wrong-marker', lambda p: p['phases'][0]['raw_markers'].__setitem__(0, 999)),
    ]
    negatives = []
    for label, corrupt in controls:
        p = copy.deepcopy(template)
        corrupt(p)
        try:
            contract(p, specs['ThrustPowerAvailable'])
        except (ValueError, AssertionError):
            negatives.append(label)
        else:
            raise ValueError('Accepted corrupt power proof ' + label)
    return dict(status='passed-3-nuclear-power-parameter-owner-bindings', qualified_keys=[], selected=selected, retained_GUI_failures=failures, retained_compile_failures=0, retained_interrupted_cases=[], new_parameter_types=3, missions=18, function_calls=72, live_callback_samples=72, parameter_owner_checks=144, actual_attached_power_checks=144, direct_power_checks=144, actual_Write_values=144, actual_Report_values=144, marker_values=90, nonzero_annual_decay_input_checks=144, generation_or_thrust_formula_checks=96, constant_bus_formula_checks=48, thrust_power_zero_clamp_checks=12, visible_mapped_processes=3, visible_exposure_events=exposures, negative_controls=negatives, native_bound_types=125, remaining_without_live_native_binding=196, factory_or_readable_role_exclusions=[], production_changed_files=[], left_only_epoch_assignments_qualified=False, power_spacecraft_back_reference_directly_observed=False, nonconstant_distance_bus_qualified=False, solar_power_science_qualified=False, electric_thruster_provider_qualified=False, whole_scope_qualified=False, scientific_approval=False, technical_core_approval=False, full_current_build_qualified=False, full_NASA_qualified=False)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--stage', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    result = audit(Path(__file__).resolve().parents[3], args.stage.resolve())
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))
