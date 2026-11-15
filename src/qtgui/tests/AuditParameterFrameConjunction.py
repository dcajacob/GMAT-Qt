"""Admit the coordinate-dependent parameter family from immutable witnesses."""
import hashlib
import json
import math
from pathlib import Path
import re
import subprocess
import sys
import tarfile

KEY = 'FRR!E525'
FINGERPRINT = '2be2d97f1f8fd7ca79b2e65cfd084cf206c186135f8ee2e5ccc0a1bcca3a7b39'
KIND = 'parameter-frame-conjunction-v1'


def load(path):
    return json.loads(Path(path).read_text())


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def require(condition, message):
    if not condition:
        raise ValueError(message)


def method_bodies(source, name):
    """Extract method bodies while ignoring braces in comments and literals."""
    pattern = re.compile(r'^\S[^\n;{}]*\b' + re.escape(name) + r'\s*\(', re.M)
    tokens = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[{}]', re.S)
    bodies = []
    for match in pattern.finditer(source):
        start = source.find('{', match.end())
        if start < 0:
            continue
        depth = 0
        for token in tokens.finditer(source, start):
            if token.group() == '{':
                depth += 1
            elif token.group() == '}':
                depth -= 1
                if depth == 0:
                    bodies.append(source[match.start():token.end()])
                    break
    require(bool(bodies), 'Missing source method ' + name)
    return bodies


def audit_index(index, repo):
    require(index['key'] == KEY and index['fingerprint'] == FINGERPRINT, 'Different parameter obligation')
    for path, digest in index['artifact_sha256'].items():
        require(sha(path) == digest, 'Changed execution evidence ' + path)
    sources = {}
    for name, item in index['source_files'].items():
        require(sha(item['artifact']) == item['sha256'], 'Changed source snapshot ' + name)
        sources[name] = Path(item['artifact']).read_text()

    required = set(index['native_parameter_types'])
    routes = index['routes']
    require(len(required) == 31 and len(routes) == 8, 'Parameter inventory incomplete')
    flattened = [kind for route in routes for kind in route['types']]
    require(len(flattened) == 31 and set(flattened) == required, 'Missing or duplicate native parameter route')
    require(sum(route['write'] is not None for route in routes) == 5, 'Read/write family changed')

    old = {}
    with tarfile.open(index['ha_source_archive']) as archive:
        for name in ['src/base/parameter/OrbitData.cpp', 'src/gmatutil/util/StateConversionUtil.cpp']:
            data = archive.extractfile(name).read()
            require(hashlib.sha256(data).hexdigest() == index['ha_source_sha256'][name], 'Changed HA source version')
            old[name] = data.decode()
    orbit_name = 'src/base/parameter/OrbitData.cpp'
    unchanged = ['GetCartState', 'GetCartReal', 'GetKepReal', 'GetSphRaDecState',
                 'GetSphAzFpaState', 'GetSphRaDecReal', 'GetSphAzFpaReal',
                 'GetAngularReal', 'GetOutAsymState', 'GetOutAsymReal',
                 'SetReal', 'SetRvector6', 'SetRealParameters', 'InitializeRefObjects']
    for name in unchanged:
        require(method_bodies(old[orbit_name], 'OrbitData::' + name) ==
                method_bodies(sources[orbit_name], 'OrbitData::' + name),
                'Scalar dependency changed since executed HA evidence ' + name)
    # Later edits in this setter concern the separately qualified Brouwer family.
    setter = 'OrbitData::GetCartStateInParameterCS'
    for before, after in zip(method_bodies(old[orbit_name], setter), method_bodies(sources[orbit_name], setter)):
        require(before.split('else if (item >= BLS_SMA')[0] == after.split('else if (item >= BLS_SMA')[0],
                'A required scalar setter changed')
    conversion = 'src/gmatutil/util/StateConversionUtil.cpp'
    kernels = ['CartesianToKeplerian', 'KeplerianToCartesian', 'CartesianToSMA',
               'CartesianToECC', 'CartesianToINC', 'CartesianToAOP', 'CartesianToRAAN',
               'CartesianToHA', 'CartesianToSphericalRADEC', 'SphericalRADECToCartesian',
               'CartesianToSphericalAZFPA', 'SphericalAZFPAToCartesian',
               'CartesianToOutgoingAsymptote', 'OutgoingAsymptoteToCartesian']
    for name in kernels:
        require(method_bodies(old[conversion], 'StateConversionUtil::' + name) ==
                method_bodies(sources[conversion], 'StateConversionUtil::' + name),
                'Parameter kernel changed since executed evidence ' + name)
    with tarfile.open(index['bplane_source_archive']) as archive:
        for name in ['src/base/parameter/BplaneData.cpp', 'src/gmatutil/util/CalculationUtilities.cpp']:
            data = archive.extractfile(name).read()
            require(hashlib.sha256(data).hexdigest() == index['bplane_source_sha256'][name],
                    'Changed B-plane source archive')
            require(data.decode() == sources[name], 'B-plane adapter or kernel changed since execution')

    catalog = index['coordinate_catalog']
    require(len(catalog['axes']) == 16 and len(catalog['origins']) == 5, 'Coordinate source catalog incomplete')
    axes = {row['native_axes'] for row in catalog['axes']}
    frames = index['frames']
    require(len(frames) == 93 and {row['axis'] for row in frames} == axes, 'Missing executed axis family')
    origins = {'Spacecraft', 'CelestialBody', 'Barycenter', 'LibrationPoint', 'GroundStation'}
    require({row['origin_class'] for row in frames} == origins, 'Missing executed origin category')
    for row in frames:
        proof = load(row['proof'])
        require(proof.get('outcome') == 'passed' and proof['axis'] == row['axis'] and
                proof['origin_class'] == row['origin_class'], 'Frame proof identity or outcome differs')

    bplane_types = set()
    bplane_rejections = 0
    recovered_types = set()
    for path in index['bplane_cases']:
        proof = load(path)
        require(proof['passed'] and proof['live_state_and_epoch_unchanged'], 'B-plane frame/sample witness failed')
        for row in proof.get('observations', []):
            if 'rejected' in row:
                require(row['rejected'], 'Undefined B-plane/frame input was accepted')
                bplane_rejections += 1
                continue
            require(row['passed'], 'B-plane numerical comparison failed')
            require(math.isfinite(row['actual']) and row['error'] <= row['bound'] and
                    row['epoch_dispatch'], 'B-plane numerical or epoch bound differs')
            if row['type'] in {'BdotT', 'BdotR', 'BVectorAngle', 'BVectorMag'}:
                bplane_types.add(row['type'])
                if row['phase'] == 'recovered':
                    recovered_types.add(row['type'])
    require(bplane_types == {'BdotT', 'BdotR', 'BVectorAngle', 'BVectorMag'}, 'B-plane adapter lacks typed executions')
    require(bplane_rejections == 1 and recovered_types == bplane_types,
            'B-plane frame rejection/recovery witnesses missing')

    component_names = index['matrix_components']
    mandatory = ['stm-frame-reference', 'stm-base-alignment', 'stm-function-frames',
                 'stm-solver-frames', 'stm-owner-frames', 'stm-domain-atomicity',
                 'stm-harmonic-gradient', 'stm-signed-axes', 'stm-solid-tide',
                 'stm-file-thrust-reports', 'stm-polyhedron', 'stm-external']
    for part in mandatory:
        require(any(part + '-v' in name for name in component_names), 'Missing executed matrix component ' + part)
    aliases = index['runtime_aliases']
    checked_artifacts = {}
    for original, item in aliases.items():
        require(sha(item['artifact']) == item['sha256'], 'Preserved runtime artifact changed')
        if item['loaded_image_manifest']:
            require(load(item['loaded_image_manifest'])[item['loaded_image_path']] == item['sha256'],
                    'Runtime artifact differs from the recorded loaded image')
        else:
            require(item['role'] in {'unused-staged-application', 'diagnostic-runtime-alias-after-rebuild'},
                    'An admitted execution lacks its image provenance')
    for name, path in component_names.items():
        root = Path(path)
        receipt = load(root / 'stage-receipt-v1.json')
        require(receipt['new_requirement_credit'] == 0, 'Component credit counted twice')
        require(receipt['status'] in {'passed', 'sealed-partial-evidence',
                'sealed-diagnostic-evidence', 'targeted-evidence-complete'}, 'Unaccepted component receipt')
        require(sha(root / 'evidence-manifest-v1.json') == receipt['manifest_sha256'], 'Changed component seal ' + name)
        manifest = load(root / 'evidence-manifest-v1.json')
        for filename, digest in manifest.get('artifact_sha256', manifest).items():
            if not isinstance(digest, str) or len(digest) != 64:
                continue
            artifact = Path(filename)
            if not artifact.is_absolute():
                artifact = root / artifact
            if str(artifact) in aliases:
                item = aliases[str(artifact)]
                require(item['sha256'] == digest, 'Runtime alias changed its original identity')
                artifact = Path(item['artifact'])
            if str(artifact) not in checked_artifacts:
                checked_artifacts[str(artifact)] = sha(artifact)
            require(checked_artifacts[str(artifact)] == digest, 'Component artifact changed ' + filename)

    domain = next(Path(path) for name, path in component_names.items() if 'stm-domain-atomicity' in name)
    plan = load(domain / 'pending-shape-and-history-v1.json')
    cases = {row['key']: row for row in plan['cases']}
    require(all(cases[name]['outcome'] == 'accept' for name in
                ['extended_finite', 'extended_nonfinite_tail', 'singular_phi']), 'Augmented public matrix cases missing')
    require(sum(row['outcome'] == 'reject' for row in cases.values()) == 11, 'Matrix domain cases missing')
    domain_passes = set()
    for run in ['before-v3', 'after-v1', 'retained-nonfinite-prefix-v1']:
        result = load(domain / run / 'independent-audit.json')
        for row in result['checks']:
            if row['passed']:
                domain_passes.add((row['case'], row['type']))
    matrix_types = {'OrbitSTM', 'OrbitSTMA', 'OrbitSTMB', 'OrbitSTMC', 'OrbitSTMD'}
    expected_domain_cases = set(cases) | {'matrix_nan', 'matrix_inf', 'matrix_neg_inf', 'fresh_valid_axes'}
    require(domain_passes == {(case, kind) for case in expected_domain_cases for kind in matrix_types},
            'A matrix domain/atomicity case lacks a passing typed execution')
    matrix = sources['src/base/parameter/OrbitStmFrame.hpp']
    require('matrix.GetNumRows() < 6 || matrix.GetNumColumns() < 6' in matrix and
            'native(i,j) = matrix(i,j)' in matrix and 'const auto result = final * native * initial.Inverse()' in matrix,
            'Public six-component matrix projection or frame transport changed')
    require('OrbitStmOwnerFrame::Transport(final,native,initial)' in matrix,
            'Owner-dependent frame requires its full Jacobian')

    replayed = {}
    for fragment in ['stm-signed-axes', 'stm-solid-tide']:
        root = next(Path(path) for name, path in component_names.items() if fragment in name)
        result = json.loads(subprocess.check_output([sys.executable, str(root / 'verify-stage-v1.py')], text=True))
        require(result['status'] == 'passed' and result['native_tests_run'] == 0, 'Component replay failed')
        replayed[fragment] = result
    require(replayed['stm-signed-axes']['signed_arrangements'] == 48, 'Signed-axis inventory incomplete')
    require(replayed['stm-solid-tide']['shifted_clock_derivative_checks'] == 54, 'Tide clock coverage incomplete')

    scope = load(index['scope_review'])
    require(scope['reference_resolution']['resolved_catalog'] == 'FRR-35' and
            scope['reference_resolution']['original_placeholder'] == 'FR-xxx', 'Coordinate reference scope changed')
    require(not scope['full_cross_product_executed'] and not scope['source_inventory_alone_earns_credit'],
            'Evidence scope overstated')
    protected = load(index['protected_check'])
    require(protected['original_inputs'] == 1334 and protected['unchanged'] == 1333 and
            protected['original_assertions_changed'] == 0 and
            protected['authorized_display_title_delta'] == ['src/qtgui/tests/VisibleTestDisplay.py'],
            'Protected assertions changed')
    return {'status': 'passed', 'native_parameter_types': 31, 'required_child_rows': 23,
            'axis_families': 16, 'origin_categories': 5, 'saved_frame_witnesses': 93,
            'bplane_sample_cases': len(index['bplane_cases']), 'matrix_component_stages': len(component_names),
            'component_artifacts_checked': len(checked_artifacts), 'runtime_aliases_resolved': len(aliases),
            'matrix_domain_type_witnesses': len(domain_passes),
            'signed_axis_arrangements': 48, 'shifted_clock_derivative_checks': 54,
            'unchanged_scalar_methods': len(unchanged), 'unchanged_numerical_kernels': len(kernels),
            'native_tests_run': 0, 'source_versioned_only': True, 'full_current_build_qualified': False}


def qualify(repo, path, inventory, admitted):
    qualification = load(path)
    require(qualification['contract_kind'] == KIND and qualification['qualified_keys'] == [KEY], 'Different qualification')
    for name, digest in qualification['artifact_sha256'].items():
        require(sha(name) == digest, 'Changed qualification artifact ' + name)
    known = {row['key']: row for row in inventory['requirements']}
    require(KEY not in admitted and known[KEY]['fingerprint'] == FINGERPRINT, 'Duplicate or changed source row')
    require(qualification['candidate_rows'] == [known[KEY]] and
            qualification['authority_sha256'] == inventory['sources'], 'Original authority changed')
    index = load(qualification['evidence_index'])
    require(set(index['required_verified_keys']) <= set(admitted), 'A prerequisite remains open')
    require(len(index['required_verified_keys']) == 46, 'Prerequisite family incomplete')
    result = audit_index(index, repo)
    return qualification, [KEY], {'status': 'passed', 'qualified_keys': [KEY],
            'whole_row_conjunction': True, 'source_fingerprints': {KEY: FINGERPRINT},
            'conjunction': result, 'source_versioned_only': True, 'native_tests_run': 0,
            'full_current_build_qualified': False, 'full_NASA_qualified': False}
