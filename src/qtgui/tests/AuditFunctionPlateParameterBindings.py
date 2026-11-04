"""Audit all thirteen bounded Plate Parameter routes without widening science credit."""
import argparse, copy, hashlib, json, re, tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
from BuildNasaCoverageExtension import load, require, sha
from AuditFunctionParameterBindings import compare, written

RECIPES = {
    'AreaCoefficient': ([1, 2, 3], .125),
    'SpecularFraction': ([.125, .25, .375], .015625),
    'DiffuseFraction': ([.1875, .3125, .4375], .015625),
    'Area': ([2, 3, 4], 0), 'LitFraction': ([.25, .5, .75], 0),
    'PlateNormal': ([[1/3, 2/3, 2/3], [2/3, 1/3, 2/3], [2/3, 2/3, 1/3]], 0),
    'PlateNormalHistoryFile': (['RootNormal.txt', 'LocalNormal.txt', 'SharedNormal.txt'], 0),
    'AreaCoefficientSigma': ([.125, .25, .5], 0),
    'SpecularFractionSigma': ([.0625, .125, .25], 0),
    'DiffuseFractionSigma': ([.03125, .0625, .125], 0),
    'PlateX': ([-.25, .75, -1.5], 0), 'PlateY': ([.5, -1, 1.75], 0),
    'PlateZ': ([1, 1.25, -2], 0),
}
TYPES = set(RECIPES)
PILOT = {'AreaCoefficient', 'PlateNormal', 'PlateNormalHistoryFile'}
PREFIX = 'QtGui.NasaFunctionPlateParameterBinding'


def contract(proof, spec):
    key, meta = spec['factory_type'], spec['metadata']
    values, step = RECIPES[key]
    require([spec[n] for n in ['root_value', 'private_value', 'shared_value']] == values and spec['step'] == step and spec['mutable_coefficient'] == (step != 0) and meta['settable'] == (step != 0) and spec['absolute_bound'] == 1e-12 and spec['hardware_kind'] == spec['hardware_type'] == 'Plate', 'Independent Plate recipe differs')
    require(proof['checkpoint'] == 'live-function-plate-parameter-owner-binding' and proof['outcome'] == 'passed' and proof['case'] == proof['argument'] == key and proof['route'] == 'attached-plate-field' and proof['family'] == 'AttachedPlate' and proof['full_system_parameter_inventory'] == 321 and set(proof['initial_batch_types']) == TYPES and proof['missions'] == 3 and proof['function_calls'] == proof['live_callback_samples'] == 12 and proof['actual_parameter_owner_checks'] == 24 and proof['file_decisions'] == [dict(saved=False), dict(saved=True)], 'Plate proof identity/counts differ')
    require([v['phase'] for v in proof['phases']] == [0, 1, 2] and [v['explicit_rebuild'] for v in proof['phases']] == [True, True, False], 'Plate no-Build repeat lost')
    for phase in proof['phases']:
        require(phase['raw_markers'] == [42, 43, 44, 103, 104] and phase['callback_private_maps_accessed_only_while_executing'] and len(phase['live_samples']) == len(phase['private_writes']) == len(phase['shared_writes']) == 4, 'Plate callbacks/Write/markers differ')
        roots, plate_roots = {}, {}
        for i, sample in enumerate(phase['live_samples']):
            inside, calls = i > 0, i
            target = 'ScopePulse' if inside else 'MainPulse'
            require(sample['inside_function'] == inside and sample['function'] == ('ReadShared' if inside else '') and sample['calls'] == calls and sample['global_map_is_value_snapshot'] == (not inside) and len(sample['parameters']) == 2 and sample['provider'] == f'Propagate PulseProp({target}) {{{target}.ElapsedSecs = 1}};', 'Plate scope/provider differs')
            for j, row in enumerate(sample['parameters']):
                shared = j == 1
                value = values[2 if shared else 1 if inside else 0]
                if isinstance(value, (int, float)):
                    value += calls * step if inside else 0
                reference = spec['shared_reference'] if shared else spec['local_private_reference'] if inside else spec['root_private_reference']
                require(row['shared'] == shared and row['reference'] == reference and row['factory_type'] == key and row['native_type'] == meta['native_type'] and row['return_type'] == meta['return_type'] and row['owner_type'] == meta['owner_type_id'] == 101 and row['owner_name'] == ('SharedSC' if shared else 'PrivateOnly') and row['owner_type_name'] == 'Spacecraft', 'Plate Parameter/owner type differs')
                for field in ['parameter_pointer', 'reference_owner_pointer', 'live_owner_pointer', 'root_owner_pointer', 'configured_owner_pointer']:
                    require(re.fullmatch('[0-9a-f]+', row[field]) and row[field] != '0', 'Invalid Plate owner identity')
                require(row['reference_owner_pointer'] == row['live_owner_pointer'] != row['configured_owner_pointer'] and row['owner_global'] == shared and row['owner_local'] == (inside and not shared), 'Plate owner reference/flags differ')
                if not inside:
                    roots[shared] = row['root_owner_pointer']
                require(row['root_owner_pointer'] == roots[shared] and (row['live_owner_pointer'] == roots[shared]) == (shared or not inside), 'Plate owner aliases wrong root')
                require(row['shape'] == ([3, 1] if key == 'PlateNormal' else []) and row['absolute_bound'] == 1e-12, 'Plate shape/bound differs')
                for field in ['actual', 'expected', 'direct_owner_property_value']:
                    compare(row[field], value, 1e-12)
                compare((phase['shared_writes'] if shared else phase['private_writes'])[i], value, 1e-12)
                attached = row['attached']
                name = ('Shared' if shared else 'Local' if inside else 'Root') + 'Plate'
                require(attached['name'] == name and attached['type'] == 'Plate' and attached['property'] == key and attached['attached_names'] == [name] and attached['initialized'] is True, 'Attached Plate name/type/list/init differs')
                for field in ['pointer', 'root_pointer', 'configured_pointer']:
                    require(re.fullmatch('[0-9a-f]+', attached[field]) and attached[field] != '0', 'Invalid native Plate identity')
                if not inside:
                    plate_roots[shared] = attached['root_pointer']
                require(attached['root_pointer'] == plate_roots[shared] and attached['pointer'] != attached['configured_pointer'] and (attached['pointer'] == plate_roots[shared]) == (shared or not inside), 'Actual Plate clone aliases wrong root')
                compare(attached['direct_value'], value, 1e-12)
                require(attached['plate_model'] == 'FixedInBody' and attached['spacecraft_SRP_model'] == 'Spherical' and attached['runtime_settable'] == attached['info_settable'] == meta['settable'] and attached['history_file_provider_used'] is False, 'Plate fixture/metadata/provider scope differs')


def audit(repo, stage):
    bindings = load(stage / 'evidence-inputs-v1.json')['artifact_sha256']
    for n, d in bindings.items():
        require(Path(n).is_file() and not Path(n).is_symlink() and sha(n) == d, 'Changed regular binding ' + n)
    def bound(p):
        p = Path(p)
        require(str(p) in bindings, 'Unbound ' + str(p))
        return p
    def capsule(name):
        with tarfile.open(bound(stage / name)) as t:
            require(all(v.isfile() for v in t.getmembers()), 'Nonregular source member')
            return {v.name: t.extractfile(v).read() for v in t.getmembers()}
    visible = capsule('visible-executed-sources-v1.tar.gz')
    before = load(bound(stage / 'visible-inputs-before-v1.json'))
    require(before == load(bound(stage / 'visible-inputs-after-v1.json')) and before['sources'] == {n: hashlib.sha256(v).hexdigest() for n, v in visible.items()}, 'Visible inputs changed')
    for n in ['AuditFunctionPlateParameterBindings.py', 'AuditFunctionParameterBindings.py', 'BuildNasaCoverageExtension.py']:
        require(hashlib.sha256(visible['src/qtgui/tests/' + n]).hexdigest() == sha(repo / 'src/qtgui/tests' / n), 'Auditor/helper changed')
    catalog = json.loads(visible['src/qtgui/tests/NasaFunctionPlateParameterCases.json'])
    specs = {v['factory_type']: v for v in catalog['plate_binding_cases']}
    require(len(catalog['cases']) == 321 and set(specs) == set(catalog['initial_types']) == set(catalog['selected_arguments']) == TYPES and not catalog['qualified_keys'] and all(not v['qualified'] for v in catalog['cases']), 'Complete321 narrowed/promoted')
    baseline = load(bound(repo / catalog['baseline']))
    require(sha(repo / catalog['baseline']) == catalog['baseline_sha256'] and {v['factory_type']: v['native_parameter_metadata'] for v in baseline['rows'] if v['role'] == 'owner-bound-system-parameter'} == {v['factory_type']: v['metadata'] for v in catalog['cases']}, 'Original metadata/roles changed')
    maps = load(bound(stage / 'loaded-images-v1.json'))['processes']
    launch = load(bound(stage / 'visible-launch-v1.json'))
    require(launch['returncode'] == 0 and launch['inputs_unchanged'] and launch['mapped_processes'] == len(maps) == 13, 'Visible mapping incomplete')
    selected, failures, exposures = [], [], 0
    for version in ['headless-v1', 'headless-v2', 'headless-v3', 'visible-v1']:
        inv = 'function-plate-parameter-bindings-' + version
        q = load(bound(repo / 'build/gui-acceptance' / inv / 'results.json'))
        frozen = visible if version == 'visible-v1' else capsule(version.replace('headless-', 'headless-executed-sources-') + '.tar.gz')
        require(q.get('finished_utc') and q['end_reason'] == 'completed-selection' and all(n in frozen and hashlib.sha256(frozen[n]).hexdigest() == d for n, d in q['source_hashes'].items()), 'Executed source/terminal state differs')
        rows = {v['name']: v for v in q['results'] if v['status'] != 'not-run'}
        types = PILOT if version in ['headless-v1', 'headless-v2'] else TYPES - PILOT if version == 'headless-v3' else TYPES
        require(set(rows) == {PREFIX + key for key in types}, 'Exact Plate selection differs')
        for name, row in rows.items():
            case, key = bound(row['log']).parent, name.removeprefix(PREFIX)
            item = dict(invocation=inv, test=name)
            if version == 'headless-v1':
                text = bound(row['log']).read_text()
                require(row['status'] == 'failed' and row['returncode'] != 0 and 'The field name "SRPShapeModel" on object "PrivateOnly" is not permitted' in text and 'Binding main parameter caller Build failed' in text, 'Retained fixture field failure lost')
                failures.append(item)
                continue
            require(row['status'] == 'passed' and row['returncode'] == 0, 'Selected Plate case failed')
            tests = list(ET.parse(bound(case / 'ctest.xml')).getroot().iter('testcase'))
            require(len(tests) == 1 and tests[0].get('name') == name and tests[0].find('failure') is None and tests[0].find('skipped') is None, 'Native Plate JUnit differs')
            emitted = [json.loads(v) for v in re.findall(r'NASA_CHECK (\{[^\n]+\})', bound(row['log']).read_text())]
            require(len(emitted) == 1 and load(bound(case / 'proof.json')) == emitted[0], 'Emitted/stored Plate proof differs')
            proof, spec = emitted[0], specs[key]
            contract(proof, spec)
            initial, edited = bound(case / 'initial.gmf').read_text(), bound(case / 'edited.gmf').read_text()
            require(initial.count('Keep = 103;') == 1 and initial.replace('Keep = 103;', 'Keep = 41;') == edited and '.SRPShapeModel' not in edited, 'Plate exact function edit differs')
            if key == 'PlateNormal':
                require('Create Array PrivateVector[3,1] SharedVector[3,1];' in edited and 'PrivateVector = PrivateOnly.LocalPlate.PlateNormal;' in edited and 'Report BindingValues PrivateVector(1,1) PrivateVector(2,1) PrivateVector(3,1) SharedVector(1,1) SharedVector(2,1) SharedVector(3,1);' in edited, 'Plate normal Report bridge differs')
            require(load(bound(case / 'phases.json')) == proof['phases'], 'Raw Plate phases differ')
            for phase in proof['phases']:
                tag = str(phase['phase'])
                messages = bound(case / (tag + '-messages.txt')).read_text()
                require(load(bound(case / (tag + '-live-parameter-samples.json'))) == phase['live_samples'] and all(load(bound(case / (tag + '-invariants.json'))).values()) and load(bound(case / (tag + '-prototypes.json'))) == load(bound(case / 'prototypes-before.json')), 'Raw Plate samples/invariants/prototypes differ')
                require(written(messages, spec, spec['root_private_reference']) + written(messages, spec, spec['local_private_reference']) == phase['private_writes'] and written(messages, spec, spec['shared_reference']) == phase['shared_writes'], 'Raw Plate Write differs')
                values = [v['actual'] for sample in phase['live_samples'] for v in sample['parameters']]
                raw = bound(case / (tag + '-binding-values.txt')).read_text()
                if key == 'PlateNormalHistoryFile':
                    require(raw.split() == values, 'Raw stored filename Report differs')
                else:
                    if key == 'PlateNormal': values = [v for vector in values for v in vector]
                    compare([float(v) for v in raw.split()], values, 1e-12)
                require([float(v) for v in bound(case / (tag + '-values.txt')).read_text().split()] == phase['raw_markers'], 'Raw Plate markers differ')
            if version == 'visible-v1':
                d = q['visible_display']
                require(d['start_minimized'] and d['resizable'] and 'Iconic' in d['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN' not in d['initial_host_window_state'] and q['max_screenshots'] == 0 and row['trace']['native_exposure_events'] > 0 and not row['trace']['screenshots'], 'Visible policy differs')
                exposures += row['trace']['native_exposure_events']
                traces = list(case.glob('trace-*.jsonl'))
                require(len(traces) == 1, 'Visible trace absent')
                mapped = maps[traces[0].stem.removeprefix('trace-')]
                exes = [n for n in row['registered_command'] if n in q['binary_hashes']]
                require(len(exes) == 1 and mapped['executable'] == exes[0] and all(before['images'].get(n) == d for n, d in mapped['images'].items()), 'Actual mapped Plate image differs')
                for n, d in before['images'].items():
                    if n == exes[0] or Path(n).name.startswith(('libGmatBase', 'libGmatUtil')):
                        require(mapped['images'].get(n) == d, 'Required image unmapped')
            selected.append(item)
    require(len(selected) == 26 and len(failures) == 3, 'Plate selection/failure count differs')
    templates = {key: load(bound(repo / 'build/gui-acceptance/function-plate-parameter-bindings-visible-v1' / (PREFIX + key) / 'proof.json')) for key in sorted(PILOT)}
    def row(p): return p['phases'][0]['live_samples'][1]['parameters'][0]
    controls = [('wrong-owner-reference', 'AreaCoefficient', lambda p: row(p).__setitem__('reference_owner_pointer', '1')), ('private-plate-aliases-root', 'AreaCoefficient', lambda p: row(p)['attached'].__setitem__('pointer', row(p)['attached']['root_pointer'])), ('wrong-attached-class', 'AreaCoefficient', lambda p: row(p)['attached'].__setitem__('type', 'ChemicalTank')), ('wrong-dependency-list', 'AreaCoefficient', lambda p: row(p)['attached'].__setitem__('attached_names', ['RootPlate'])), ('not-initialized', 'AreaCoefficient', lambda p: row(p)['attached'].__setitem__('initialized', False)), ('wrong-scalar', 'AreaCoefficient', lambda p: row(p).__setitem__('actual', 999)), ('wrong-normal-component', 'PlateNormal', lambda p: row(p)['actual'].__setitem__(0, 0)), ('wrong-vector-width', 'PlateNormal', lambda p: row(p).__setitem__('shape', [1, 3])), ('wrong-history-string', 'PlateNormalHistoryFile', lambda p: row(p).__setitem__('actual', 'Other.txt')), ('readonly-flag-changed', 'PlateNormalHistoryFile', lambda p: row(p)['attached'].__setitem__('runtime_settable', True)), ('missing-no-Build', 'AreaCoefficient', lambda p: p['phases'].pop()), ('wrong-marker', 'AreaCoefficient', lambda p: p['phases'][0]['raw_markers'].__setitem__(0, 999))]
    negatives = []
    for label, key, corrupt in controls:
        p = copy.deepcopy(templates[key])
        corrupt(p)
        try: contract(p, specs[key])
        except (ValueError, AssertionError): negatives.append(label)
        else: raise ValueError('Accepted corrupt Plate proof ' + label)
    return dict(status='passed-13-attached-plate-parameter-owner-bindings', qualified_keys=[], selected=selected, retained_GUI_failures=failures, retained_compile_failures=0, retained_interrupted_cases=[], new_parameter_types=13, scalar_parameter_types=11, vector_parameter_types=1, string_parameter_types=1, mutable_coefficient_types=3, readonly_types=10, missions=78, function_calls=312, live_callback_samples=312, parameter_owner_checks=624, actual_attached_plate_checks=624, direct_plate_checks=624, actual_typed_Write_values=624, actual_typed_Report_values=624, scalar_field_values=528, normal_vectors=48, normal_scalar_components=144, stored_filename_values=48, expanded_Write_and_Report_values_each=720, marker_values=390, visible_mapped_processes=13, visible_exposure_events=exposures, negative_controls=negatives, native_bound_types=138, remaining_without_live_native_binding=183, factory_or_readable_role_exclusions=[], production_changed_files=[], history_file_provider_qualified=False, SRP_force_qualified=False, general_frame_geometry_or_estimation_science_qualified=False, readonly_GUI_destinations_directly_observed=False, whole_scope_qualified=False, scientific_approval=False, technical_core_approval=False, full_current_build_qualified=False, full_NASA_qualified=False)


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--stage', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    result = audit(Path(__file__).resolve().parents[3], a.stage.resolve())
    a.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))
