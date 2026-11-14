"""Check retained specialist-input receipts, complete mode grids and unit docs.

This reuses an inventory without replaying it. It does not qualify missions,
visible behavior, other editor families or the complete E12 requirement.
"""
import argparse
import gzip
import hashlib
import itertools
import json
import re
from pathlib import Path
from AuditShippedInputDocumentation import audit as guide_audit


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def require(ok, message):
    if not ok:
        raise ValueError(message)


def audit(repo, census):
    receipt_path = census / 'receipt.json'
    receipt = json.loads(receipt_path.read_text())
    require(receipt['inputs_unchanged'], 'Inventory inputs changed during execution')
    for name, digest in receipt['source_and_runtime_sha256'].items():
        require(sha(name) == digest, 'Inventory source/runtime changed: ' + name)
    data, output_hashes = {}, {}
    for result in receipt['results']:
        mode = result['mode']
        require(mode not in data and result['returncode'] == 0, 'Duplicate/failed mode: ' + mode)
        require(result['inputs_unchanged'] and result['loaded_file_image_set_measured'], 'Unbound execution: ' + mode)
        path = Path(result['compressed_output'])
        require(sha(path) == result['compressed_sha256'], 'Changed census output: ' + str(path))
        raw = gzip.decompress(path.read_bytes())
        require(hashlib.sha256(raw).hexdigest() == result['uncompressed_sha256'], 'Changed decompressed output')
        document = json.loads(raw)
        require(document['mode'] == mode and document['platform'] == 'offscreen', 'Wrong inventory mode/platform')
        require(document['rejected_states'] == 0 and result['rejected_states'] == 0, 'Rejected configurations remain: ' + mode)
        require(len(document['states']) == document['inventoried_states'] == result['inventoried_states'], 'State count mismatch')
        require(document['mission_executions'] == document['shown_windows'] == 0, 'Inventory exceeded construction-only scope')
        require(not document['GUI_behavior_qualified'] and not document['all_input_documentation_qualified'], 'Overbroad inventory credit')
        command = result['command']
        require(sha(command[1]) == result['startup_sha256'], 'Changed startup text')
        log = path.with_name(path.name.removesuffix('.json.gz') + '.log')
        require(sha(log) == result['log_sha256'], 'Changed process log')
        require(document['loaded_file_images'] == result['loaded_file_images'], 'Loaded image receipt mismatch')
        for image in document['loaded_file_images']:
            require(sha(image['canonical_path']) == image['sha256'], 'Loaded image changed: ' + image['canonical_path'])
        data[mode] = document
        output_hashes[mode] = dict(path=str(path), sha256=sha(path))
    catalogue = data['--configured-catalog']
    models = catalogue['attitude_models']
    require(len(models) == len(set(models)), 'Duplicate factory models')
    require(set(data) == {'--configured-catalog', '--orbit', '--epochs'} | {'--attitude=' + m for m in models}, 'Factory/mode coverage mismatch')
    state_names = {'EulerAngles': {'EulerAngle1', 'EulerAngle2', 'EulerAngle3'},
                   'Quaternion': {'Q1', 'Q2', 'Q3', 'Q4'},
                   'DirectionCosineMatrix': {'DCM' + str(r) + str(c) for r in range(1, 4) for c in range(1, 4)},
                   'MRPs': {'MRP1', 'MRP2', 'MRP3'}}
    rate_names = {'EulerAngleRates': {'EulerAngleRate1', 'EulerAngleRate2', 'EulerAngleRate3'},
                  'AngularVelocity': {'AngularVelocityX', 'AngularVelocityY', 'AngularVelocityZ'}}
    all_state = set.union(*state_names.values())
    all_rate = set.union(*rate_names.values())
    groups, native_types, state_count = {}, {}, 0
    for model in models:
        document = data['--attitude=' + model]
        initial = document['initial_attitude_allowed']
        expected = set(itertools.product(catalogue['orientation_representations'], catalogue['rate_representations'], catalogue['euler_sequences'])) if initial else {('', '', '')}
        actual = [(s['orientation'], s['rate'], s['euler_sequence']) for s in document['states']]
        require(len(actual) == len(set(actual)) and set(actual) == expected, 'Incomplete attitude grid: ' + model)
        for state in document['states']:
            controls = {c['field']: c for c in state['controls']}
            require(len(controls) == len(state['controls']), 'Duplicate attitude controls')
            require(controls['Attitude']['value'] == model, 'Wrong actual attitude model')
            if initial:
                require(set(controls) & all_state == state_names[state['orientation']], 'Wrong actual orientation fields')
                require(set(controls) & all_rate == rate_names[state['rate']], 'Wrong actual rate fields')
                for name, value in [('AttitudeDisplayStateType', state['orientation']), ('AttitudeRateDisplayStateType', state['rate']), ('EulerAngleSequence', state['euler_sequence'])]:
                    require(controls[name]['value'] == value, 'Wrong actual attitude selector: ' + name)
            for item in state['native_parameters']:
                if item.get('field') and item.get('parameter_type'):
                    native_types.setdefault(item['field'], set()).add(item['parameter_type'])
    expected_orbits = {(regime, representation, anomaly) for regime, anomalies in [('elliptic', ['TA', 'MA', 'EA']), ('hyperbolic', ['TA', 'MA', 'HA'])] for representation in ['Keplerian', 'ModifiedKeplerian'] for anomaly in anomalies}
    actual_orbits = [(s['orbit_regime'], s['representation'], s['anomaly']) for s in data['--orbit']['states']]
    require(len(actual_orbits) == len(set(actual_orbits)) and set(actual_orbits) == expected_orbits, 'Incomplete anomaly grid')
    for state in data['--orbit']['states']:
        controls = {c['field']: c for c in state['controls']}
        require(controls['DisplayStateType']['value'] == state['representation'] and controls['AnomalyType']['value'] == state['anomaly'], 'Wrong actual orbit selectors')
        require(state['element_names'][-1] == state['anomaly'], 'Sixth element mislabeled')
        require(set(state['element_names']) <= set(controls), 'Missing orbit element control')
        require((float(controls.get('ECC', {'value': '0'})['value']) > 1) == (state['orbit_regime'] == 'hyperbolic') if state['representation'] == 'Keplerian' else True, 'Wrong measured conic regime')
    actual_formats = [s['date_format'] for s in data['--epochs']['states']]
    require(len(actual_formats) == len(set(actual_formats)) and set(actual_formats) == set(catalogue['epoch_formats']), 'Incomplete date formats')
    for state in data['--epochs']['states']:
        controls = {c['field']: c for c in state['controls']}
        require(controls['DateFormat']['value'] == state['date_format'] and bool(controls['Epoch'].get('value')), 'Wrong/missing actual epoch controls')
    for mode, document in data.items():
        for state_index, state in enumerate(document['states']):
            require(state['status'] == 'inventoried', 'Rejected state')
            state_count += 1
            config = {k: v for k, v in state.items() if k not in {'controls', 'native_parameters', 'status'}}
            config['mode'] = mode
            for control in state['controls']:
                key = ('Spacecraft', control['field'], control['unit'])
                row = groups.setdefault(key, dict(type='Spacecraft', field=control['field'], observed_GUI_unit=control['unit'], states=[], source_outputs={}))
                row['states'].append(mode + '#' + str(state_index))
                row['source_outputs'][output_hashes[mode]['path']] = output_hashes[mode]['sha256']
    baseline = dict(fields=[groups[k] for k in sorted(groups)])
    guide = guide_audit(repo, baseline)
    require(guide['summary']['without_active_guide_route'] == 0, 'Missing configured-input guide routes')
    reviewed = []
    for field in guide['fields']:
        name, unit = field['field'], field['observed_GUI_unit']
        descriptions = [d['units'] for d in field['shipped_documentation_candidates']]
        if re.fullmatch(r'(AngularVelocity[XYZ]|EulerAngleRate[123]|PrecessionRate|SpinRate)', name):
            policy = 'angular-rate-degrees-per-second'
            require(unit == 'deg/s' and all(v in {'deg/sec', 'deg./sec.', 'deg./s'} for v in descriptions), 'Wrong angular-rate units: ' + name)
        elif re.fullmatch(r'(EulerAngle[123]|InitialPrecessionAngle|InitialSpinAngle|NutationAngle|AOP|INC|RAAN|TA|EA|MA|HA)', name):
            policy = 'degree-or-degree-scaled-anomaly'
            require(unit == 'deg' and all(v in {'deg', 'deg.'} for v in descriptions), 'Wrong angle/anomaly units: ' + name)
        elif name in {'X', 'Y', 'Z', 'SMA', 'RadApo', 'RadPer'}:
            policy = 'kilometres'
            require(unit == 'km' and all(v == 'km' for v in descriptions), 'Wrong position units: ' + name)
        elif name in {'VX', 'VY', 'VZ'}:
            policy = 'kilometres-per-second'
            require(unit == 'km/s' and all(v == 'km/s' for v in descriptions), 'Wrong velocity units')
        elif re.fullmatch(r'(DCM[123][123]|Q[1234]|MRP[123]|BodyAlignmentVector[XYZ]|BodyConstraintVector[XYZ]|BodySpinAxis[XYZ]|NutationReferenceVector[XYZ]|ECC)', name):
            policy = 'dimensionless-state-or-direction'
            require(unit == '' and all(v.casefold().startswith('dimensionless') for v in descriptions), 'Missing dimensionless meaning: ' + name)
        elif name == 'Epoch':
            policy = 'selected-epoch-time-system-and-representation'
            require(unit == '' and all('days' in v.casefold() and 'Gregorian' in v and 'DateFormat' in v for v in descriptions), 'Incomplete epoch unit convention')
        elif name in {'AnomalyType', 'Attitude', 'AttitudeConstraintType', 'AttitudeCoordinateSystem', 'AttitudeDisplayStateType', 'AttitudeFileName', 'AttitudeRateDisplayStateType', 'AttitudeReferenceBody', 'CoordinateSystem', 'DateFormat', 'DisplayStateType', 'EulerAngleSequence'}:
            policy = 'name-file-or-representation-selector'
            require(unit == '' and all(v == 'N/A' or 'selector' in v for v in descriptions), 'Wrong selector units: ' + name)
        else:
            raise ValueError('Unreviewed configured control: ' + name)
        reviewed.append(dict(field=field, meaning_policy=policy))
    proof_sources = ['src/qtgui/tests/ConfiguredInputCatalog.cpp', 'src/qtgui/AttitudeDialog.cpp', 'src/qtgui/ResourceProperties.cpp', 'src/qtgui/ResourceEditor.cpp', 'src/qtgui/SpacecraftOrbit.cpp', 'src/base/attitude/Attitude.cpp', 'src/base/attitude/PrecessingSpinner.cpp', 'src/base/attitude/NadirPointing.cpp', 'src/base/spacecraft/Spacecraft.cpp', 'src/gmatutil/util/StateConversionUtil.cpp']
    return dict(status='passed', scope='Measured specialist spacecraft modes and unit documentation only; other dynamic editor families and E12 completeness remain open.', models=models, states_reviewed=state_count, field_unit_pairs_reviewed=len(reviewed), active_guide_routes=guide['summary']['active_guide_route_candidates'], records=reviewed, source_sha256={p: sha(repo/p) for p in proof_sources}, active_guide_source_sha256=guide['active_guide_source_sha256'], census_receipt_sha256=sha(receipt_path), verifier_sha256=sha(Path(__file__)), loaded_file_images_bound=True, numerical_truth_qualified=False, GUI_behavior_qualified=False, qualified_keys=[], all_input_documentation_qualified=False)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, default=Path(__file__).resolve().parents[3])
    parser.add_argument('--census', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    report = audit(args.repo.resolve(), args.census.resolve())
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS', report['states_reviewed'], 'configured states;', report['field_unit_pairs_reviewed'], 'field/unit meanings; no replay.')
