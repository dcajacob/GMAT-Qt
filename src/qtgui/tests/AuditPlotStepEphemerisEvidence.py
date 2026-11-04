"""Validate committed SPK advances and both plot consumers from native receipts."""
import argparse
from collections import Counter
import gzip
import hashlib
import json
import math
from pathlib import Path
from xml.etree import ElementTree


CASES = {'EphemType9Forward', 'EphemType13Backward', 'EphemType13Zero', 'EphemType13Mixed'}


def check(condition, reason):
    if not condition:
        raise ValueError(reason)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def document(path):
    if path.is_file():
        return json.loads(path.read_bytes())
    return json.loads(gzip.decompress(path.with_suffix(path.suffix + '.gz').read_bytes()))


def cartesian(object_name, t):
    if object_name == 'Two':
        radius = 9000
        rate = math.sqrt(398600.4415 / radius**3)
        angle = .4 + rate*t
        return (0, radius*math.cos(angle), radius*math.sin(angle),
                0, -radius*rate*math.sin(angle), radius*rate*math.cos(angle))
    coefficients = [(7000, 1.2, .003, 1e-6), (-2000, 4.5, -.002, 5e-7), (500, -.7, .0004, -2e-7)]
    position = [a+t*(b+t*(c+t*d)) for a,b,c,d in coefficients]
    velocity = [b+t*(2*c+t*3*d) for _,b,c,d in coefficients]
    return position+velocity


def check_case(proof, counts):
    name = proof['case']
    check(name in CASES and proof['outcome'] == 'passed', 'Unknown or failed SPK case')
    mixed = name.endswith('Mixed')
    direction = -1 if name.endswith('Backward') else 1
    skip = 0 if name.endswith('Zero') else 1
    check(proof['skip'] == skip, 'Sampling setting differs')
    epochs = {'One': [0,10,20,23] if mixed else [0,10,10,20,30,37],
              'Two': [0,3,6,9,10,13,16,19,20,23] if mixed else []}
    selected = {'One': [0,20] if mixed else [0,20,37] if skip else [0,10,20,30,37],
                'Two': [0,6,10,16,20] if mixed else []}
    records = proof['ephemeris_endpoints']
    for obj, grid in epochs.items():
        rows = [r for r in records if r['objects'] == [obj]]
        check(len(rows) == len(grid), 'Provider advance count differs')
        for i,(r,t) in enumerate(zip(rows, grid)):
            initial = i == 0 or (not mixed and i == 2)
            base = 10 if not mixed and i >= 2 else 0
            start = t if initial else grid[i-1]
            check(r['initial'] is initial and r['ephemeris'] is (obj == 'One') and
                  r['method'] == ('SPK' if obj == 'One' else 'RungeKutta89'), 'Endpoint provenance differs')
            for value, expected in [(r['elapsed'], t), (r['start'], start-base), (r['end'], t-base)]:
                check(abs(value-direction*expected) < 1e-4, 'Endpoint clock differs')
            counts['provider_records'] += 1
    check(len(records) == sum(map(len, epochs.values())), 'Unexpected provider')
    curves = proof['outputs']
    check([(c['plot'],c['object']) for c in curves] == [(p,o) for p in ('Orb','Map') for o in ('One','Two')], 'Curve inventory differs')
    for curve in curves:
        grid = selected[curve['object']]
        check(len(curve['points']) == len(grid), 'Retained curve length differs')
        for point,time in zip(curve['points'],grid):
            t = direction*time
            check(abs(point['elapsed']-t) < 1e-4, 'Retained epoch differs')
            xyz = [point[key] for key in ('x','y','z')]
            check(all(map(math.isfinite,xyz)), 'Nonfinite plot sample')
            counts['plot_epochs'] += 1
            if curve['plot'] == 'Orb':
                wanted = cartesian(curve['object'],t)
                check(all(abs(v-w)<4e-5 for v,w in zip(xyz,wanted)), 'Cartesian position differs')
                check(len(point['velocity']) == 3 and all(abs(v-w)<5e-8 for v,w in zip(point['velocity'],wanted[3:])), 'Cartesian velocity differs')
                counts['cartesian_components'] += 6


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    manifest = document(args.manifest)
    for filename, sha in manifest['artifacts'].items():
        check(digest(Path(filename)) == sha, 'Changed artifact '+filename)
    check(digest(Path(__file__)) == manifest['auditor_sha256'], 'Auditor changed')
    counts = Counter()
    seen = {'visible':set(),'headless':set()}
    for run in manifest['runs']:
        root = Path(run['directory'])
        case = root/'QtGui.PlotIntegrationSteps'
        result = document(root/'results.json')
        status = document(case/'result.json')
        mode = run['mode']
        check(status['status']=='passed' and status['returncode']==0 and status['requested_visibility']==mode, 'Native invocation failed or mode differs')
        check(not status['trace']['screenshots'], 'Unexpected screenshot')
        junit = ElementTree.parse(case/'ctest.xml').findall('.//testcase')
        check(len(junit)==1 and junit[0].get('name')=='QtGui.PlotIntegrationSteps' and all(junit[0].find(tag) is None for tag in ('error','failure','skipped')), 'JUnit failed')
        if mode=='visible':
            display = result['visible_display']
            check(display['resizable'] and display['start_minimized'] and 'Iconic' in display['initial_host_window_state'] and 'FULLSCREEN' not in display['initial_host_window_state'], 'Window policy differs')
            check(status['trace']['native_exposure_events']>0 and status['trace']['recorded_platforms']==['xcb'], 'Visible native exposure missing')
            counts['native_exposures'] += status['trace']['native_exposure_events']
        archive = args.manifest.parent/run['version']
        for source, sha in document(archive/'hashes.json').items():
            check(digest(archive/source)==sha and result['source_hashes'].get(source)==sha, 'Executed source differs '+source)
        proof = document(case/'proof.json')
        logged = [json.loads(line.split('NASA_CHECK ',1)[1]) for line in (case/'test.log').read_text().splitlines() if 'NASA_CHECK ' in line]
        check(proof['cases']==logged and proof['new_requirement_credit']==0, 'Proof/log differs')
        check([p['case'] for p in logged]==run['cases'], 'Wrong selected cases')
        for p in logged:
            name = p['case']
            check(document(case/(name+'-proof.json'))==p and document(case/(name+'-points.json'))==p['outputs'] and document(case/(name+'-endpoints.json'))==p['ephemeris_endpoints'], 'Raw evidence differs')
            check_case(p, counts)
            seen[mode].add(name)
    check(seen['visible']==CASES==seen['headless'], 'Missing native cases')
    ledger = document(Path(manifest['baseline']))
    check(ledger['coverage']['requirements']['verified']==1009 and ledger['coverage']['requirements']['total']==1158 and ledger['remaining_counts']['requirements']==149,'Coverage baseline differs')
    report = dict(bounded_evidence_status='passed', checked=dict(counts), manifest_sha256=digest(args.manifest), new_requirement_credit=0, remaining=149)
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report))


if __name__=='__main__':
    main()
