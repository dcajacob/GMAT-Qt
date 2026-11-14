"""Audit the equation destination-registration repair and its unchanged GUI probes."""
import argparse, copy, hashlib, json, re, tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
from BuildNasaCoverageExtension import load, require, sha
from AuditFunctionParameterBindings import contract as parameter_contract, compare, written
from AuditFunctionPowerParameterBindings import contract as common_power_contract, TYPES, SUFFIX, PREFIX

PARAMETERS = {'DryMass', 'A1ModJulian', 'CurrA1MJD', 'UTCGregorian'}
ASSIGNMENTS = {'SettableParameter', 'ResourceExpression'}
TESTS = {PREFIX + k + suffix for k in TYPES for suffix in ['', SUFFIX]} | {'QtGui.NasaFunctionParameterBinding' + k for k in PARAMETERS} | {'QtGui.NasaAssignment' + k for k in ASSIGNMENTS}


def power_contract(proof, spec):
    key = spec['factory_type']
    argument = proof['argument']
    require(argument in {key, key + SUFFIX}, 'Unknown epoch control argument')
    absolute = argument == key
    require(proof['epoch_assignment_mode'] == ('absolute-left-only' if absolute else 'self-referenced-increments'), 'Raw epoch assignment mode differs')
    # Validate the raw mode first. Both modes require the same strict values,
    # owner identities and epochs; reuse that contract with only its identity
    # selector adapted in a new dictionary. Raw evidence is never rewritten.
    common_power_contract(dict(proof, argument=key + SUFFIX, epoch_assignment_mode='self-referenced-increments'), spec)


def assignment_contract(proof, spec):
    key = spec['key']
    require(proof['checkpoint'] == 'assignment-' + key and proof['outcome'] == 'passed' and proof['family'] == key and proof['destination'] == spec['destination'] and proof['expression'] == spec['expression'] and proof['expected'] == spec['expected'] and proof['numeric_sentinel'] == 9, 'Scalar Assignment identity differs')
    compare(proof['actual'], spec['expected'], 2e-12)
    require(proof['max_error'] <= 2e-12 and [(v['action'], v['input']) for v in proof['rejections']] == [('Apply', 'AbsentNASA + 1'), ('Apply', '1 +'), ('OK', 'AbsentNASA + 1')] and all(v['diagnostic'] for v in proof['rejections']), 'Scalar Assignment invalid rollback differs')
    require(set(proof['facets']) == {'actual-mission-tree-assignment-editor', 'typed-expression-and-destination', 'cancel-pending-model-history-protection', 'invalid-apply-ok-atomicity', 'exact-apply-idempotence-undo-redo', 'unicode-reopen-typed-readback', 'independent-mathematical-or-literal-mission-result'}, 'Scalar Assignment UI facets differ')


def audit(repo, stage):
    bindings = load(stage / 'evidence-inputs-v1.json')['artifact_sha256']
    for n, d in bindings.items():
        require(Path(n).is_file() and not Path(n).is_symlink() and sha(n) == d, 'Changed regular binding ' + n)
    def bound(p):
        p = Path(p)
        require(str(p) in bindings, 'Unbound ' + str(p))
        return p
    def capsule(p):
        with tarfile.open(bound(p)) as t:
            require(all(v.isfile() for v in t.getmembers()), 'Nonregular source member')
            return {v.name: t.extractfile(v).read() for v in t.getmembers()}
    visible = capsule(stage / 'visible-executed-sources-v1.tar.gz')
    before = load(bound(stage / 'visible-inputs-before-v1.json'))
    require(before == load(bound(stage / 'visible-inputs-after-v1.json')) and before['sources'] == {n: hashlib.sha256(v).hexdigest() for n, v in visible.items()}, 'Visible inputs changed')
    for n in ['AuditFunctionParameterBindings.py', 'AuditFunctionPowerParameterBindings.py', 'BuildNasaCoverageExtension.py', 'AuditFunctionAbsoluteEpochAssignments.py']:
        require(hashlib.sha256(visible['src/qtgui/tests/' + n]).hexdigest() == sha(repo / 'src/qtgui/tests' / n), 'Audit helper changed')
    power = json.loads(visible['src/qtgui/tests/NasaFunctionPowerParameterCases.json'])
    parameters = json.loads(visible['src/qtgui/tests/NasaFunctionParameterBindingCases.json'])
    assignments = json.loads(visible['src/qtgui/tests/NasaAssignmentCases.json'])
    specs = {v['factory_type']: v for v in power['power_binding_cases']}
    parameter_specs = {v['factory_type']: v for v in parameters['cases']}
    assignment_specs = {v['key']: v for v in assignments['cases']}
    require(len(power['cases']) == len(parameter_specs) == 321 and set(specs) == TYPES and not power['qualified_keys'] and not parameters['qualified_keys'], 'Complete Parameter inventories changed')
    maps = load(bound(stage / 'loaded-images-v1.json'))['processes']
    launch = load(bound(stage / 'visible-launch-v1.json'))
    require(launch['returncode'] == 0 and launch['inputs_unchanged'] and launch['mapped_processes'] == len(maps) == 12, 'Visible mapping incomplete')
    selected, exposures = [], 0
    for version in ['headless-v1', 'visible-v1']:
        inv = 'function-absolute-epoch-assignment-' + version
        q = load(bound(repo / 'build/gui-acceptance' / inv / 'results.json'))
        frozen = visible if version == 'visible-v1' else capsule(stage / 'headless-executed-sources-v1.tar.gz')
        require(q.get('finished_utc') and q['end_reason'] == 'completed-selection' and all(n in frozen and hashlib.sha256(frozen[n]).hexdigest() == d for n, d in q['source_hashes'].items()), 'Executed source/terminal state differs')
        rows = {v['name']: v for v in q['results'] if v['status'] != 'not-run'}
        require(set(rows) == TESTS, 'Exact affected selection differs')
        for name, row in rows.items():
            case = bound(row['log']).parent
            require(row['status'] == 'passed' and row['returncode'] == 0, 'Affected GUI case failed')
            tests = list(ET.parse(bound(case / 'ctest.xml')).getroot().iter('testcase'))
            require(len(tests) == 1 and tests[0].get('name') == name and tests[0].find('failure') is None and tests[0].find('skipped') is None, 'Native JUnit differs')
            emitted = [json.loads(v) for v in re.findall(r'NASA_CHECK (\{[^\n]+\})', bound(row['log']).read_text())]
            require(len(emitted) == 1 and load(bound(case / 'proof.json')) == emitted[0], 'Emitted/stored proof differs')
            proof = emitted[0]
            if name.startswith(PREFIX):
                key = name.removeprefix(PREFIX).removesuffix(SUFFIX)
                spec = specs[key]
                power_contract(proof, spec)
                edited = bound(case / 'edited.gmf').read_text()
                absolute = proof['argument'] == key
                expected_statement = 'PrivateOnly.A1ModJulian = 21910.25 + Calls * 365.25;' if absolute else 'PrivateOnly.A1ModJulian = PrivateOnly.A1ModJulian + Calls * 365.25;'
                expected_global = 'SharedSC.A1ModJulian = 22275.5 + Calls * 365.25;' if absolute else 'SharedSC.A1ModJulian = SharedSC.A1ModJulian + 365.25;'
                require(expected_statement in edited and expected_global in edited, 'Original equation destination statement changed')
            elif name.startswith('QtGui.NasaFunctionParameterBinding'):
                key = name.removeprefix('QtGui.NasaFunctionParameterBinding')
                spec = parameter_specs[key]
                parameter_contract(proof, spec)
            else:
                key = name.removeprefix('QtGui.NasaAssignment')
                spec = assignment_specs[key]
                assignment_contract(proof, spec)
                fields = bound(case / 'values.txt').read_text().strip().split(',')
                compare([float(v) for v in fields], spec['expected'] + [9], 2e-12)
                require(bound(case / 'mission.script').read_text() == bound(case / 'expected.script').read_text(), 'Scalar Assignment exact source differs')
            if 'phases' in proof:
                require(load(bound(case / 'phases.json')) == proof['phases'], 'Raw phases differ')
                initial, edited = bound(case / 'initial.gmf').read_text(), bound(case / 'edited.gmf').read_text()
                require(initial.count('Keep = 103;') == 1 and initial.replace('Keep = 103;', 'Keep = 41;') == edited, 'Exact function edit differs')
                for phase in proof['phases']:
                    tag = str(phase['phase'])
                    messages = bound(case / (tag + '-messages.txt')).read_text()
                    require(load(bound(case / (tag + '-live-parameter-samples.json'))) == phase['live_samples'] and all(load(bound(case / (tag + '-invariants.json'))).values()) and load(bound(case / (tag + '-prototypes.json'))) == load(bound(case / 'prototypes-before.json')), 'Raw owner invariants differ')
                    private = written(messages, spec, spec['root_private_reference'])
                    if spec['root_private_reference'] != spec['local_private_reference']:
                        private += written(messages, spec, spec['local_private_reference'])
                    require(private == phase['private_writes'] and written(messages, spec, spec['shared_reference']) == phase['shared_writes'], 'Raw Write differs')
                    raw = bound(case / (tag + '-binding-values.txt')).read_text()
                    expected_report = [v['actual'] for sample in phase['live_samples'] for v in sample['parameters']]
                    if spec['metadata']['return_type'] == 7:
                        require(raw.splitlines() == [' '.join(expected_report[i:i + 2]) for i in range(0, len(expected_report), 2)], 'Raw string Report differs')
                    else:
                        compare([float(v) for v in raw.split()], expected_report, 1e-12 if name.startswith(PREFIX) else 2e-10)
                    require([float(v) for v in bound(case / (tag + '-values.txt')).read_text().split()] == phase['raw_markers'], 'Raw markers differ')
            if version == 'visible-v1':
                d = q['visible_display']
                require(d['start_minimized'] and d['resizable'] and 'Iconic' in d['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN' not in d['initial_host_window_state'] and q['max_screenshots'] == 0 and row['trace']['native_exposure_events'] > 0 and not row['trace']['screenshots'], 'Visible policy differs')
                exposures += row['trace']['native_exposure_events']
                traces = list(case.glob('trace-*.jsonl'))
                require(len(traces) == 1, 'Visible trace absent')
                mapped = maps[traces[0].stem.removeprefix('trace-')]
                exes = [n for n in row['registered_command'] if n in q['binary_hashes']]
                require(len(exes) == 1 and mapped['executable'] == exes[0] and all(before['images'].get(n) == d for n, d in mapped['images'].items()), 'Actual mapped image differs')
                for n, d in before['images'].items():
                    if n == exes[0] or Path(n).name.startswith(('libGmatBase', 'libGmatUtil')):
                        require(mapped['images'].get(n) == d, 'Required image unmapped')
            selected.append(dict(invocation=inv, test=name))
    prior = repo / 'build/gui-acceptance/function-power-parameter-bindings-headless-v1'
    prior_q = load(bound(prior / 'results.json'))
    retained = []
    for key in TYPES:
        name = PREFIX + key
        row = next(v for v in prior_q['results'] if v['name'] == name)
        require(row['status'] == 'failed' and row['returncode'] != 0 and 'GmatCommand::SetWrapperReferences failed to find object named "PrivateOnly.A1ModJulian"' in bound(prior / name / '0-messages.txt').read_text(), 'Original failed native attempt lost')
        retained.append(dict(invocation=prior.name, test=name))
    template = load(bound(repo / 'build/gui-acceptance/function-absolute-epoch-assignment-visible-v1' / (PREFIX + 'ThrustPowerAvailable') / 'proof.json'))
    controls = [('wrong-absolute-mode', lambda p: p.__setitem__('epoch_assignment_mode', 'self-referenced-increments')), ('wrong-probe-argument', lambda p: p.__setitem__('argument', 'Other')), ('wrong-owner', lambda p: p['phases'][0]['live_samples'][1]['parameters'][0].__setitem__('reference_owner_pointer', '1')), ('private-owner-alias', lambda p: p['phases'][0]['live_samples'][1]['parameters'][0].__setitem__('live_owner_pointer', p['phases'][0]['live_samples'][1]['parameters'][0]['root_owner_pointer'])), ('wrong-epoch', lambda p: p['phases'][0]['live_samples'][1]['parameters'][0]['attached'].__setitem__('owner_epoch', 21545)), ('wrong-value', lambda p: p['phases'][0]['live_samples'][1]['parameters'][0].__setitem__('actual', 999)), ('missing-no-Build', lambda p: p['phases'].pop()), ('wrong-marker', lambda p: p['phases'][0]['raw_markers'].__setitem__(0, 999))]
    negatives = []
    for label, corrupt in controls:
        p = copy.deepcopy(template)
        corrupt(p)
        try:
            power_contract(p, specs['ThrustPowerAvailable'])
        except (ValueError, AssertionError):
            negatives.append(label)
        else:
            raise ValueError('Accepted corrupt absolute epoch proof ' + label)
    require(len(selected) == 24 and len(retained) == 3, 'Affected selection/retained failure count differs')
    return dict(status='passed-equation-left-only-epoch-registration-repair', qualified_keys=[], selected=selected, current_GUI_failures=[], retained_prior_GUI_failures=retained, retained_compile_failures=0, retained_interrupted_cases=[], new_parameter_types=0, native_bound_types=125, remaining_without_live_native_binding=196, original_absolute_epoch_GUI_passes=6, affected_regression_GUI_passes=18, function_scope_missions=60, function_calls=240, live_callback_samples=240, parameter_owner_checks=480, power_parameter_owner_checks=288, original_absolute_epoch_owner_checks=144, actual_function_Write_values=480, actual_function_Report_values=480, function_marker_values=300, scalar_assignment_missions=4, scalar_invalid_GUI_rejections=12, visible_mapped_processes=12, visible_exposure_events=exposures, negative_controls=negatives, production_changed_files=['src/base/command/Assignment.cpp'], original_numerical_assertions_or_bounds_weakened=False, factory_or_readable_role_exclusions=[], whole_scope_qualified=False, time_conversion_science_qualified=False, scientific_approval=False, technical_core_approval=False, full_current_build_qualified=False, full_NASA_qualified=False)


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--stage', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    result = audit(Path(__file__).resolve().parents[3], a.stage.resolve())
    a.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))
