"""Audit coeval LAC plot dependencies; award no whole NASA requirement credit."""
import argparse
from collections import Counter
import copy
import json
import math
from pathlib import Path

from AuditPlotStepEphemerisEvidence import check, digest, document
from AuditPlotStepResetEvidence import invocation

CASES = ['LACReferenceForward', 'LACReferenceBackward', 'LACConstraintForward',
         'LACConstraintBackward', 'LACConstraintThinned', 'LACLiveReference']
REGRESSIONS = ['FrameFollowerForward', 'FrameLeaderMissing',
               'RotatingOriginForward', 'RotatingPairForward']
RUNS = [('headless-v3', 'headless', CASES[:1]),
        ('headless-v4', 'headless', CASES[1:]),
        ('visible-v1', 'visible', CASES),
        ('regressions-headless-v1', 'headless', REGRESSIONS),
        ('regressions-visible-v1', 'visible', REGRESSIONS)]


def dot(a, b):
    return math.fsum(x*y for x, y in zip(a, b))


def cross(a, b):
    return [a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]]


def unit(v):
    length = math.sqrt(dot(v, v))
    check(length > 1e-9, 'Degenerate independent LAC basis')
    return [x/length for x in v]


def cart(object_name, t):
    radius, phase = (7000, 0) if object_name == 'One' else (9000, .4)
    rate = math.sqrt(398600.4415/radius**3)
    angle = phase+rate*t
    x, y = radius*math.cos(angle), radius*math.sin(angle)
    vx, vy = -radius*rate*math.sin(angle), radius*rate*math.cos(angle)
    return [x, y, 0, vx, vy, 0] if object_name == 'One' else [0, x, y, 0, vx, vy]


def position(object_name, t, nested):
    x_axis = unit(cart('Two', t)[:3])
    constraint = [0, 0, 1]
    if nested:
        angle = math.sqrt(398600.4415/7000**3)*t
        constraint = [-math.sin(angle), math.cos(angle), 0]
    # Project the independently specified constraint onto the plane normal to X.
    z_axis = unit([c-dot(constraint, x_axis)*x for c, x in zip(constraint, x_axis)])
    y_axis = cross(z_axis, x_axis)
    return [dot(axis, cart(object_name, t)[:3]) for axis in (x_axis, y_axis, z_axis)]


def state(object_name, t, nested):
    h = .25
    samples = [position(object_name, t+k*h, nested) for k in (-2, -1, 1, 2)]
    velocity = [(a-8*b+8*c-d)/(12*h) for a, b, c, d in zip(*samples)]
    return position(object_name, t, nested)+velocity


def check_case(proof, counts):
    name = proof['case']
    check(name in CASES and proof['outcome'] == 'passed', 'Unexpected LAC case')
    nested, thinned = name.startswith('LACConstraint'), name.endswith('Thinned')
    live = name == 'LACLiveReference'
    sign = -1 if name.endswith('Backward') else 1
    check(proof['skip'] == (3 if thinned else 0), 'Changed LAC skip setting')
    check([(r['plot'], r['object']) for r in proof['outputs']] ==
          [(p, o) for p in ('Orb', 'Map') for o in ('One', 'Two')], 'Wrong LAC curve census')
    raw = proof['lac_raw']
    check(raw['reference_identity_checked'] is True and raw['nested'] is nested,
          'Runtime LAC reference identity missing')
    native = {r['name']: r for r in raw['native_owners']}
    check(set(native) == {'One', 'Two'}, 'Wrong runtime spacecraft census')
    for object_name, owner in native.items():
        t = 0 if live and object_name == 'Two' else sign*23
        check(len(owner['state']) == 6, 'Incomplete final owner state')
        check(abs((owner['epoch']-21545)*86400-t) < 1e-4, 'Final owner clock differs')
        check(all(abs(a-b) < (4e-5 if i < 3 else 5e-8)
                  for i, (a, b) in enumerate(zip(owner['state'], cart(object_name, t)))),
              'Final live spacecraft state differs')
        counts['owner_state_components'] += 6
    raw_curves = {(r['plot'], r['object']): r['points'] for r in raw['outputs']}
    for curve in proof['outputs']:
        object_name, plot = curve['object'], curve['plot']
        two, orbit = object_name == 'Two', plot == 'Orb'
        times = ([0, 10, 20] if two else [0]) if thinned else (
            [0, 10, 20, 23] if not two or nested and orbit else [0, 3, 6, 9, 10, 13, 16, 19, 20, 23])
        if live:
            times = [] if two else [0] if orbit else [0, 10, 20, 23]
        times = [sign*t for t in times]
        check(len(curve['points']) == len(times), 'LAC sampled point count differs')
        check(len(raw_curves[(plot, object_name)]) == len(times), 'LAC raw curve count differs')
        for i, (point, t, captured) in enumerate(zip(curve['points'], times, raw_curves[(plot, object_name)])):
            connected = i == 3 if nested and orbit and two and not thinned else i > 0
            check(abs(point['elapsed']-t) < 1e-4 and point['connect'] is connected,
                  'LAC epoch/connection differs')
            check(point['elapsed'] == captured['elapsed'] and point['connect'] is captured['connect'] and
                  [point[k] for k in ('x', 'y', 'z')] == captured['position'], 'Raw LAC capture differs')
            check(all(math.isfinite(point[k]) for k in ('x', 'y', 'z')), 'Nonfinite LAC coordinates')
            counts['epochs_and_connections'] += 1
            if orbit:
                wanted = state(object_name, t, nested)
                check(all(abs(point[k]-v) < 4e-5 for k, v in zip(('x', 'y', 'z'), wanted)),
                      'LAC independent position differs')
                check(point['velocity'] == captured['velocity'] and len(point['velocity']) == 3 and
                      all(abs(a-b) < 5e-8 for a, b in zip(point['velocity'], wanted[3:])),
                      'LAC independent velocity differs')
                counts['cartesian_components'] += 6


def audit(repo, stage):
    frozen = document(stage/'executed-source-sha256-v2.json')
    counts = Counter()
    seen, raw_proofs, bound = set(), {}, {}
    for run, mode, names in RUNS:
        root, case = stage/run, stage/run/'QtGui.PlotIntegrationSteps'
        result = invocation(root, mode, case.name, counts)
        check(all(frozen.get(p) == h for p, h in result['source_hashes'].items()), 'Executed source version differs')
        images = document(stage/(run+'-loaded-images.json'))
        check(images['returncode'] == 0 and images['inputs_unchanged'] is True and images['visible'] is (mode == 'visible'),
              'Actual loaded-image custody differs')
        check(len(images['processes']) == 1 and len(images['loaded_project_images']) == 26 and
              any('libGmatBase.so' in p for p in images['loaded_project_images']), 'Wrong native runtime map')
        check(images['source_sha256'] == frozen, 'Mapped run source set differs')
        counts['mapped_processes'] += 1
        proof = document(case/'proof.json')
        logged = [json.loads(line.split('NASA_CHECK ', 1)[1]) for line in (case/'test.log').read_text().splitlines() if 'NASA_CHECK ' in line]
        check(proof['cases'] == logged and proof['new_requirement_credit'] == 0 and
              [p['case'] for p in logged] == names, 'Native case/log inventory differs')
        for p in logged:
            name = p['case']
            check(document(case/(name+'-proof.json')) == p and document(case/(name+'-points.json')) == p['outputs'],
                  'Raw native proof/points differ')
            if name in CASES:
                check(document(case/(name+'-raw.json')) == p['lac_raw'], 'Native LAC raw capture differs')
                check_case(p, counts)
                warnings = 1 if name == 'LACLiveReference' else 2 if name.startswith('LACConstraint') and not name.endswith('Thinned') else 0
                check((case/(name+'-run-messages.txt')).read_text().count("has no coherent state for frame 'LacFrame'") == warnings,
                      'Unavailable LAC reference diagnostics differ')
                counts['missing_reference_warnings'] += warnings
                seen.add((mode, name))
                raw_proofs[(mode, name)] = p
            else:
                old_run = ('plot-step-origin-headless-v5' if mode == 'headless' else 'plot-step-origin-visible-v1') if name.startswith('Frame') else (
                    'plot-step-rotating-context-headless-v1' if mode == 'headless' and name == 'RotatingOriginForward' else
                    'plot-step-rotating-context-visible-v1' if mode == 'visible' and name == 'RotatingOriginForward' else
                    'plot-step-rotating-headless-v2' if mode == 'headless' else 'plot-step-rotating-visible-v1')
                old_path = repo/'build/gui-acceptance'/old_run/case.name/(name+'-proof.json')
                old = document(old_path)
                check(p['outputs'] == old['outputs'], 'Affected regression trajectory changed: '+name)
                bound[str(old_path.relative_to(repo))] = digest(old_path if old_path.exists() else old_path.with_suffix('.json.gz'))
                counts['matched_regression_outputs'] += 1
    check(seen == {(m, n) for m in ('headless', 'visible') for n in CASES}, 'Missing new LAC delivery')
    failed = stage/'headless-v2'/'QtGui.PlotIntegrationSteps'
    check(document(failed/'result.json')['status'] == 'failed' and
          'FAIL Plot coordinates differ from independent trajectory' in (failed/'test.log').read_text(), 'Original native failure missing')
    original = document(failed/'LACReferenceForward-raw.json')
    errors = [abs(v-w) for curve in original['outputs'] if curve['plot'] == 'Orb' and curve['object'] in {'One', 'Two'}
              for p in curve['points'] for v, w in zip(p['position'], position(curve['object'], p['elapsed'], False))]
    check(max(errors) > 1, 'Original geometric counterexample missing')
    controls = []
    for label, change in [('position', lambda p: p['outputs'][0]['points'][0].__setitem__('x', 1000)),
                          ('velocity', lambda p: p['outputs'][0]['points'][0]['velocity'].__setitem__(0, 1)),
                          ('connection', lambda p: p['outputs'][0]['points'][0].__setitem__('connect', True)),
                          ('runtime-reference', lambda p: p['lac_raw'].__setitem__('reference_identity_checked', False))]:
        broken = copy.deepcopy(raw_proofs[('headless', 'LACReferenceForward')]); change(broken)
        raw_point = next(r for r in broken['lac_raw']['outputs'] if r['plot'] == 'Orb' and r['object'] == 'One')['points'][0]
        if label == 'position': raw_point['position'][0] = 1000
        if label == 'velocity': raw_point['velocity'][0] = 1
        if label == 'connection': raw_point['connect'] = True
        try:
            check_case(broken, Counter())
        except ValueError:
            controls.append(label)
        else:
            raise ValueError('Damaged LAC proof accepted: '+label)
    return {'bounded_evidence_status': 'passed', 'new_requirement_credit': 0, 'checked': dict(counts),
            'original_max_position_error_km': max(errors), 'rejected_controls': controls,
            'regression_source_version_bindings': bound, 'original_failure_retained': True,
            'open_rows': ['FRR!E329', 'FRR!E330', 'FRR!E633', 'FRR!E634'],
            'remaining_gates': ['general sampled force/attitude inputs', 'additional provider/reference lifetimes',
                                'remaining solver/render boundaries and inherited conjunctions'],
            'full_current_build_qualified': False, 'full_NASA_qualified': False}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--stage', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    result = audit(Path(__file__).resolve().parents[3], args.stage)
    check(not args.output.exists(), 'Use a fresh audit destination')
    args.output.write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps(result))
