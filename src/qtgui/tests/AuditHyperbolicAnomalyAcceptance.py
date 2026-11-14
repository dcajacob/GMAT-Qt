"""Join executed HA capability witnesses without promoting unrelated requirements."""
import hashlib
import json
import math
from pathlib import Path
import tarfile

from OrbitalParameterOracle import conic

KEYS = ['FRR!E536', 'FRR!E526']
KIND = 'complete-source-versioned-hyperbolic-anomaly-access'
BODIES = ['Sun', 'Mercury', 'Venus', 'Earth', 'Luna', 'Mars', 'Jupiter',
          'Saturn', 'Uranus', 'Neptune', 'Pluto', 'UserMoon', 'UserAsteroid',
          'UserComet', 'UserPlanet']
FRAME_KEYS = ['HAEarthQualifiedFrame', 'HAEarthQualifiedEcliptic',
              'HAEarthCrossFrame', 'HAMarsQualifiedFrame',
              'HAUserPlanetQualifiedFrame', 'HAFrameFunction', 'HAEarthGuards']
GATES = ['scientific_approval', 'technical_core_approval',
         'full_current_build_qualified', 'full_NASA_qualified']


def load(path):
    return json.loads(Path(path).read_text())


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def require(value, message):
    if not value:
        raise ValueError(message)


def bounded(actual, expected, bounds):
    require(len(actual) == len(expected) == len(bounds), 'Wrong numerical shape')
    require(all(math.isfinite(a) and abs(a-b) <= limit + 2e-12*abs(b)
                for a, b, limit in zip(actual, expected, bounds)),
            'Independent Cartesian/anomaly reconstruction failed')


def expected(spec):
    elements = list(spec['initial_elements'])
    if spec['key'] == 'HAFrameFunction':
        angle = math.radians(elements[5])
        e = elements[1]
        initial_h = math.degrees(2*math.atanh(math.tan(angle/2)*math.sqrt((e-1)/(e+1))))
        return [70.0, initial_h] + conic(elements, spec['mu'])
    h = math.radians(float(spec['expression']))
    e = elements[1]
    elements[5] = math.degrees(2*math.atan(math.sqrt((e+1)/(e-1))*math.tanh(h/2)))
    return [float(spec['expression'])] + conic(elements, spec['mu'])


def check_case(folder, spec, profile):
    receipt, proof = load(folder/'result.json'), load(folder/'proof.json')
    require(receipt['status'] == 'passed' and receipt['returncode'] == 0 and
            receipt['profile'] == profile, 'Unsuccessful HA execution')
    for path, digest in receipt['artifacts'].items():
        require(sha(path) == digest, 'Changed executed HA artifact: '+path)
    require(receipt['actual_base'], 'Missing actual loaded native base')
    require(proof['outcome'] == 'passed' and proof['family'] == spec['key'] and
            proof['destination'] == spec['destination'] and
            proof['expression'] == spec['expression'] and
            proof['origin'] == spec['origin'] and
            proof['origin_type'] == spec['expected_origin_type'] and
            proof['reference_mu'] == spec['mu'] and proof['numeric_sentinel'] == 9,
            'Wrong actual HA owner, factory, origin, route or sentinel')
    require(len(proof['rejections']) == 3 and
            all(r['diagnostic'] for r in proof['rejections']), 'Missing GUI rollback/diagnostics')
    require(spec['absolute_tolerance'] == 2e-8 and spec['relative_tolerance'] == 2e-12,
            'Original global numerical bounds changed')
    bounds = spec['column_absolute_tolerances']
    require(bounds == ([1e-9]*2+[2e-8]*3+[2e-11]*3 if spec['key'] == 'HAFrameFunction'
                       else [1e-9]+[2e-8]*3+[2e-11]*3), 'Original column bounds changed')
    raw = [float(x) for x in (folder/'values.txt').read_text().strip().split(',')]
    require(raw[-1] == 9 and raw[:-1] == proof['actual'], 'Raw HA result differs from proof')
    bounded(raw[:-1], expected(spec), bounds)
    bounded(proof['expected'], expected(spec), bounds)
    require((folder/'mission.script').read_text() == (folder/'expected.script').read_text(),
            'Actual setter source did not persist exactly')
    for facet in ['actual-mission-tree-assignment-editor', 'cancel-pending-model-history-protection',
                  'invalid-apply-ok-atomicity', 'exact-apply-idempotence-undo-redo',
                  'unicode-reopen-typed-readback', 'orbital-configured-state-isolation']:
        require(facet in proof['facets'], 'Missing executed GUI transaction facet')
    return proof


def frames(folder):
    receipt, proof = load(folder/'result.json'), load(folder/'proof.json')
    require(receipt['status'] == 'passed' and receipt['returncode'] == 0 and
            proof['status'] == 'passed' and proof['signed_extended_cases'] is True and
            proof['ordinary_domain_only'] is False, 'Incomplete native frame execution')
    require(proof['rejected_atomically'] == 22 and proof['vector_route_checks'] == 48,
            'Missing atomic rejection or public vector routes')
    rows = proof['independent_frame_cases']
    wanted = {(rate, precise, target) for rate in [0, .001]
              for precise in [False, True] for target in [-400, -70, -40, 0, 70, 400]}
    require(len(rows) == 24 and {(r['rotation_rate'], r['precise_epoch'], r['target'])
                               for r in rows} == wanted, 'Incomplete signed/Rdot/epoch matrix')
    maximum = 0.0
    for row in rows:
        target, rate = row['target'], row['rotation_rate']
        h, e = math.radians(target), 1.4
        ta = math.degrees(2*math.atan(math.sqrt((e+1)/(e-1))*math.tanh(h/2)))
        local = conic([-12000, e, 30, 40, 50, ta], 398600.4415)
        theta = .37 + (rate*1e-7 if row['precise_epoch'] else 0)
        c, s = math.cos(theta), math.sin(theta)
        reference = [c*local[0]-s*local[1], s*local[0]+c*local[1], local[2]]
        reference += [c*local[3]-s*local[4]-rate*reference[1],
                      s*local[3]+c*local[4]+rate*reference[0], local[5]]
        bounded(row['actual'], reference, [2e-8]*3+[2e-11]*3)
        bounded(row['expected'], reference, [2e-8]*3+[2e-11]*3)
        error = abs(row['readback']-target)
        require(math.isfinite(row['readback']) and error < 1e-9+2e-12*abs(target),
                'Signed frame readback exceeds unchanged bound')
        maximum = max(maximum, error)
    return maximum


def qualify(repo, path, inventory, admitted):
    q = load(path)
    require(q['contract_kind'] == KIND and q['qualified_keys'] == KEYS,
            'Different or extra HA admission')
    require(all(q[g] is False for g in GATES), 'Broader qualification promoted')
    require(q['authority_sha256'] == inventory['sources'], 'Changed original authority')
    for name, digest in q['artifact_sha256'].items():
        require(sha(name) == digest, 'Changed HA qualification artifact: '+name)
    known = {r['key']: r for r in inventory['requirements']}
    require(q['candidate_rows'] == [known[k] for k in KEYS], 'Original row identity changed')
    children = [f'FRR!E{i}' for i in range(527, 549)]
    require(q['parent_children'] == children and set(children)-{'FRR!E536'} <= set(admitted),
            'Orbit-parameter parent has an unaccepted child')
    require(not set(KEYS) & set(admitted), 'Duplicate requirement admission')
    protected = load(q['protected_input_manifest'])
    require(len(protected) == 1334 and all(sha(Path(repo)/n) == h for n, h in protected.items()),
            'Protected original inputs changed')
    with tarfile.open(q['source_archive']) as bundle:
        require(set(bundle.getnames()) == set(q['source_sha256']), 'Wrong source capsule partition')
        source = {n: bundle.extractfile(n).read() for n in bundle.getnames()}
        require(all(hashlib.sha256(source[n]).hexdigest() == h for n, h in q['source_sha256'].items()),
                'Changed executed/owning source capsule')
    fixture_name = 'src/qtgui/tests/HyperbolicAnomalyWriteCases.json'
    require(hashlib.sha256(source[fixture_name]).hexdigest() == q['frozen_fixture_sha256'],
            'Frozen HA expectations changed')
    fixture = json.loads(source[fixture_name])
    specs = {c['key']: c for c in fixture['cases']}
    require(len(specs) == 27 and {specs['HA'+b+'Positive']['body_case'] for b in BODIES} == set(BODIES),
            'Missing default or user-factory body')
    stage = Path(q['component_stage'])
    historical_gui = Path(q['historical_gui_stage'])
    for key, spec in specs.items():
        check_case(stage/'body-frame-headless-v1'/key, spec, 'headless')
    for key in FRAME_KEYS:
        proof = check_case(stage/'frame-consumers-final-v1'/key, specs[key], 'headless')
        if key == 'HAEarthGuards':
            require(len(proof['native_guards']) == 7 and
                    all(g['rejected'] and g['state_unchanged'] for g in proof['native_guards']),
                    'Missing native undefined/nonfinite/overflow rejection')
    for body in BODIES:
        check_case(historical_gui/'ha-fixed-visible-v2'/('HA'+body+'Positive'),
                   specs['HA'+body+'Positive'], 'visible')
    for key in FRAME_KEYS[:-1]:
        check_case(historical_gui/'ha-qualified-visible-v1'/key, specs[key], 'visible')
    maximum = frames(stage/'native-frame-signed-final-v1')
    for key in ['eq', 'ec']:
        receipt = load(stage/('achieve-frame-'+key+'-final-v1')/'result.json')
        require(receipt['status'] == 'passed' and receipt['returncode'] == 0,
                'Actual selector/solver route failed')
    for name in q['retained_failure_receipts']:
        require(load(name)['status'] == 'failed', 'Original failure was overwritten/waived')
    require(q['uniform_arbitrary_domain_precision_claimed'] is False and
            q['original_assertions_changed'] == 0 and q['failures_waived'] is False,
            'Unsupported uniform precision or waived original failure')
    return q, KEYS, dict(status='passed', qualified_keys=KEYS, whole_row_conjunction=True,
                         body_cases=15, reused_visible_body_transactions=15,
                         reused_visible_frame_transactions=6, native_signed_frame_cases=24,
                         public_vector_route_checks=48, native_atomic_rejections=29,
                         maximum_signed_frame_H_error=maximum, parent_children=children,
                         original_assertions_changed=0, failures_waived=False,
                         native_tests_rerun_for_admission=0,
                         **{g: False for g in GATES})
