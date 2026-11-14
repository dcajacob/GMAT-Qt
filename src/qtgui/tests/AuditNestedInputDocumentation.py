"""Reuse nested-input receipts to check concrete controls and unit conventions.

This is documentation/inventory evidence, not a mission, Apply or visibility test.
"""
import argparse
import gzip
import hashlib
import json
import re
from pathlib import Path
from lxml import etree
from AuditShippedInputDocumentation import audit as guide_audit


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def require(condition, message):
    if not condition:
        raise ValueError(message)


def audit(repo, census):
    receipt = json.loads((census / 'receipt.json').read_text())
    require(receipt['returncode'] == 0 and receipt['inputs_unchanged'], 'Failed or changed nested inventory')
    for path, digest in receipt['inputs_sha256'].items():
        require(sha(path) == digest, 'Changed execution input: ' + path)
    path = Path(receipt['compressed_output'])
    require(sha(path) == receipt['compressed_sha256'], 'Changed compressed inventory')
    raw = gzip.decompress(path.read_bytes())
    require(hashlib.sha256(raw).hexdigest() == receipt['uncompressed_sha256'], 'Changed inventory output')
    require(sha(census / 'probe.log') == receipt['log_sha256'], 'Changed inventory log')
    data = json.loads(raw)
    require(data['requested'] == '--nested-inputs' and data['platform'] == 'offscreen', 'Wrong inventory identity')
    require(data['mission_executions'] == data['shown_windows'] == 0, 'Inventory exceeded construction scope')
    require(data['loaded_file_image_set_measured'] and data['loaded_file_images'] == receipt['loaded_file_images'], 'Missing loaded image seal')
    for image in data['loaded_file_images']:
        require(sha(image['canonical_path']) == image['sha256'], 'Loaded image changed: ' + image['canonical_path'])
    scenes = {s['name']: s for s in data['nested_input_scenes']}
    expected = {'body:Earth', 'body:Luna', 'body:Mars', 'body:Sun', 'atmosphere:None', 'atmosphere:JacchiaRoberts', 'gravity:Earth', 'external:provider', 'picker:Multiple', 'picker:NumericSingle', 'picker:FunctionInputs'}
    require(len(scenes) == len(data['nested_input_scenes']) and set(scenes) == expected, 'Missing/duplicate specialist scene')
    require(data['inventoried_scenes'] == receipt['inventoried_scenes'] == len(expected) and data['rejected_scenes'] == receipt['rejected_scenes'] == 0, 'Rejected scene remains')
    for scene in scenes.values():
        require(scene['status'] == 'inventoried', 'Rejected scene: ' + scene['name'])
        require(len({c['object_name'] for c in scene['controls']}) == len(scene['controls']), 'Duplicate named controls')
    body_fields = {f['field']: f for s in scenes.values() if s['name'].startswith('body:') for f in s['fields']}
    fields = [dict(type='CelestialBody', field=name) for name in sorted(body_fields)]
    fields += [dict(type='ForceModel', field='Drag.' + name) for name in ['F107', 'F107A', 'MagneticIndex']]
    guide = guide_audit(repo, dict(fields=fields))
    require(guide['summary']['without_active_guide_route'] == 0, 'Missing native field unit description')
    meanings = []
    for row in guide['fields']:
        name = row['field']
        values = [d['units'] for d in row['shipped_documentation_candidates']]
        if name == 'Mu':
            expected_units = {'km^3/s^2'}
        elif name == 'EquatorialRadius':
            expected_units = {'km'}
        elif name == 'OrientationEpoch':
            expected_units = {'GMAT A1 modified Julian days'}
        elif name == 'NutationUpdateInterval':
            expected_units = {'sec.'}
        elif name == 'RotationRate':
            expected_units = {'deg/day'}
        elif name in {'SpinAxisRARate', 'SpinAxisDECRate'}:
            expected_units = {'deg/century'}
        elif name in {'SpinAxisRAConstant', 'SpinAxisDECConstant', 'RotationConstant'}:
            expected_units = {'deg'}
        elif re.fullmatch(r'3DModelRotation[XYZ]', name):
            expected_units = {'Deg.'}
        elif re.fullmatch(r'3DModelOffset[XYZ]', name):
            expected_units = {'km in the Qt body-model display coordinate frame'}
        elif name in {'Flattening', '3DModelScale'}:
            require(all(v.startswith('Dimensionless') for v in values), 'Wrong ratio units: ' + name)
            expected_units = set(values)
        elif name == 'NAIFId':
            expected_units = {'Integer identifier or type code; no physical quantity'}
        elif name in {'Drag.F107', 'Drag.F107A'}:
            expected_units = {'10^-22 W/m^2/Hz'}
        elif name == 'Drag.MagneticIndex':
            expected_units = {'Dimensionless Kp geomagnetic index (0–9)'}
        elif name in {'CentralBody', 'PosVelSource', 'RotationDataSource', 'PlanetarySpiceKernelName', 'OrbitSpiceKernelName', 'SpiceFrameId', 'FrameSpiceKernelName', 'TextureMapFileName', '3DModelFile', 'OrbitColor', 'TargetColor'}:
            expected_units = {'N/A', 'None', 'NA'}
        else:
            raise ValueError('Unreviewed built-in/native field: ' + name)
        require(bool(values) and set(values) <= expected_units, 'Wrong units for ' + name + ': ' + repr(values))
        meanings.append(row)
    body_names = {name: {f['field'] for f in scenes['body:' + name]['fields']} for name in ['Earth', 'Luna', 'Mars', 'Sun']}
    require('NutationUpdateInterval' in body_names['Earth'] and all('NutationUpdateInterval' not in body_names[n] for n in ['Luna', 'Mars', 'Sun']), 'Earth-only field branch mismatch')
    for name in ['Earth', 'Luna', 'Mars', 'Sun']:
        controls = {c['object_name']: c for c in scenes['body:' + name]['controls']}
        require({'body_' + f for f in body_names[name]} <= set(controls), 'Missing actual body field control')
    for mode in ['None', 'JacchiaRoberts']:
        controls = {c['object_name']: c for c in scenes['atmosphere:' + mode]['controls']}
        expected_names = {'BodyName', 'AtmosphereModel', 'DragModel', 'HistoricWeatherSource', 'PredictedWeatherSource', 'F107', 'F107A', 'MagneticIndex', 'SchattenErrorModel', 'SchattenTimingModel', 'CSSISpaceWeatherFile', 'SchattenFile', 'InputFile'}
        require(set(controls) == {'atmosphere_' + n for n in expected_names}, 'Unreviewed atmosphere control')
        for field in ['F107', 'F107A']:
            require(controls['atmosphere_' + field]['parent_labels'] == ['sfu'], 'Wrong actual solar-flux label')
        require(controls['atmosphere_MagneticIndex']['parent_labels'] == ['0–9'], 'Wrong actual Kp label')
        require(all(controls['atmosphere_' + n]['enabled'] == (mode != 'None') for n in ['F107', 'F107A', 'MagneticIndex']), 'Wrong active/inactive numeric fields')
    gravity = {c['object_name'] for c in scenes['gravity:Earth']['controls']}
    require(gravity == {'gravity_' + n for n in ['Body', 'PotentialModel', 'PotentialFile', 'Degree', 'Order', 'StmLimit', 'TideSource', 'TideModel', 'TideFile']}, 'Unreviewed gravity control')
    external = {c['object_name']: c for c in scenes['external:provider']['controls']}
    require(set(external) == {'externalForceEnabled', 'externalForceModule', 'externalForceFunction', 'externalForceExclusive'}, 'Unreviewed external-provider control')
    require(external['externalForceEnabled']['class'] == 'QGroupBox', 'Checkable-group input was omitted')
    for mode in ['Multiple', 'NumericSingle', 'FunctionInputs']:
        controls = {c['object_name']: c for c in scenes['picker:' + mode]['controls']}
        require(set(controls) == {'reportParameterEntry', 'reportPropertyObject', 'reportPropertyType', 'reportPropertyDependency', 'reportArrayRow', 'reportArrayColumn', 'reportSelectedParameters'}, 'Unreviewed parameter-picker control')
        require(controls['reportParameterEntry']['editable'], 'Picker entry is not editable')
        if mode == 'NumericSingle':
            require(controls['reportParameterEntry']['value'] == '1', 'Numeric literal entry was not measured')
    ns = {'d': 'http://docbook.org/ns/docbook'}
    doc = repo / 'doc/help/src/Resource_QtControlUnits.xml'
    tree = etree.parse(str(doc))
    unit_policies = {}
    for family in ['Bodies', 'Atmosphere', 'Gravity', 'External', 'Reports']:
        section = tree.xpath('//*[@xml:id=$key]', key='QtControlUnits_' + family, namespaces={'xml': 'http://www.w3.org/XML/1998/namespace'})
        require(len(section) == 1, 'Missing specialist unit policy')
        para = section[0].find('./d:variablelist/d:varlistentry[d:term="Units"]/d:listitem/d:para', ns)
        unit_policies[family] = ''.join(para.itertext())
    require('Kp' in unit_policies['Atmosphere'] and '10^-22 W/m^2/Hz' in unit_policies['Atmosphere'], 'Incomplete atmosphere convention')
    require('literal value or expression' in unit_policies['Reports'] and 'receiving field or function' in unit_policies['Reports'], 'Picker literals lack a unit convention')
    require('Use external Python force and Exclude other forces are Boolean' in unit_policies['External'], 'Missing checkable-group convention')
    proof = ['src/qtgui/tests/NasaInputCatalogTests.cpp', 'src/qtgui/tests/NestedInputCatalog.hpp', 'src/qtgui/CelestialBodyPanel.cpp', 'src/qtgui/AtmosphereDialog.cpp', 'src/qtgui/GravityDialog.cpp', 'src/qtgui/ExternalForceDialog.cpp', 'src/qtgui/ReportParameterDialog.cpp', 'src/base/solarsys/Planet.cpp', 'src/base/solarsys/AtmosphereModel.cpp']
    return dict(status='passed', scope='Eleven additional constructed specialist scenes and their unit conventions; other editor-family completeness remains open.', scenes=len(scenes), actual_controls=sum(len(s['controls']) for s in scenes.values()), native_field_rows_reviewed=len(meanings), unit_policies=unit_policies, fields=meanings, source_sha256={p: sha(repo / p) for p in proof}, guide_sha256=sha(doc), verifier_sha256=sha(Path(__file__)), census_receipt_sha256=sha(census / 'receipt.json'), source_scene_roles={'Bodies': 'Native field tables; fixed body branch only adds Earth nutation seconds.', 'Atmosphere': 'F107/F107A use sfu; MagneticIndex uses dimensionless Kp; remaining entries select names or files. Model/source changes toggle availability, not these unit conventions.', 'Gravity': 'Degree/Order/StmLimit are counts; body/models/source are names and potential/tide fields are paths.', 'External': 'Two Boolean choices, module name and derivative function name; no numeric provider values entered.', 'Reports': 'Reference selectors and indices; literals/expressions inherit receiving command/function units.'}, GUI_behavior_qualified=False, qualified_keys=[], all_input_documentation_qualified=False)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, default=Path(__file__).resolve().parents[3])
    parser.add_argument('--census', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    result = audit(args.repo.resolve(), args.census.resolve())
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print('PASS', result['scenes'], 'new specialist scenes;', result['actual_controls'], 'actual controls;', result['native_field_rows_reviewed'], 'native field unit rows.')
