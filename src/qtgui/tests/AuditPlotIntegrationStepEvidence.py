"""Audit bounded plot-consumer receipts and independent position oracles.

No NASA requirement is admitted by this audit. Runtime file hashes do not
constitute proof of all loaded images or a full current-build qualification.
"""
import argparse
import hashlib
import gzip
import json
import math
from pathlib import Path
import xml.etree.ElementTree as ET


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(value, message):
    if not value:
        raise ValueError(message)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, default=Path(__file__).resolve().parents[3])
    parser.add_argument('--manifest', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    repo = args.repo.resolve()
    manifest = json.loads((repo / args.manifest).read_text())
    require(manifest['auditor_sha256'] == digest(Path(__file__)), 'Auditor version differs')
    for name, expected in manifest['artifacts'].items():
        require(digest(repo / name) == expected, 'Retained artifact changed: ' + name)
    def bound(path):
        path = Path(path)
        if not path.is_absolute(): path = repo / path
        if not path.exists() and Path(str(path) + '.gz').exists(): path = Path(str(path) + '.gz')
        require(str(path.relative_to(repo)) in manifest['artifacts'], 'Unbound consumed artifact: ' + str(path))
        return path
    def load(path):
        path = bound(path)
        return json.loads(gzip.decompress(path.read_bytes()) if path.suffix == '.gz' else path.read_bytes())

    baseline = load(manifest['baseline'])
    require(baseline['coverage']['requirements'] == {'verified': 1009, 'total': 1158, 'percent': 87.1, 'new_verified': 24}, 'Baseline changed')
    require(baseline['remaining_counts']['requirements'] == 149, 'Original open-row inventory changed')
    catalogue = load(manifest['original_catalogue'])
    for name, sha in manifest['original_assertions'].items():
        require(catalogue['source_hashes'][name] == sha, 'Original assertion source inventory mismatch')
        require(digest(bound(manifest['source_archives'][sha])) == sha, 'Original assertion source archive mismatch')

    literal = {
        'ForwardSplit': ([0, 20, 37], []), 'BackwardSplit': ([0, -20, -37], []),
        'SingleZero': ([0, 10, 20], []), 'IntMax': ([0], []),
        'Providers': ([0, 20, 37], [0, 6]), 'Formation': ([0, 20], [0, 20]),
        'Synchronized': ([0, 20], [0, 6, 10, 16, 20]),
        'TranslatedOrigin': ([0, 20, 37], []),
        'NonlinearABM': ([0, *range(2, 37, 2), 37.123], []),
        'NonlinearABMQuarter': ([0, *range(2, 37, 2), 37.25], []),
    }
    counts = dict(position_values=0, epoch_values=0, geographic_values=0, native_exposures=0)
    seen = {'headless': set(), 'visible': set()}
    reviewed = []
    for run in manifest['runs']:
        root = repo / run['directory']; result = load(root / 'results.json')
        name = run['test']; case = root / name; receipt = load(case / 'result.json')
        require(receipt['status'] == 'passed' and receipt['returncode'] == 0, 'Unsuccessful native invocation')
        require(receipt['requested_visibility'] == run['mode'], 'Execution mode mismatch')
        tests = ET.parse(bound(case / 'ctest.xml')).getroot().findall('.//testcase')
        require(len(tests) == 1 and tests[0].get('name') == name and tests[0].find('failure') is None and tests[0].find('error') is None and tests[0].find('skipped') is None, 'JUnit mismatch')
        log = bound(case / 'test.log').read_text()
        native_checks = [json.loads(line.split('NASA_CHECK ', 1)[1]) for line in log.splitlines() if 'NASA_CHECK ' in line]
        require(receipt['trace']['screenshots'] == [], 'Retained screenshots prohibited')
        if run['mode'] == 'visible':
            display = result['visible_display']
            require(display['resizable'] and display['start_minimized'] and 'Iconic' in display['initial_host_window_state'] and 'FULLSCREEN' not in display['initial_host_window_state'], 'Window policy mismatch')
            require(receipt['trace']['recorded_platforms'] == ['xcb'] and receipt['trace']['native_exposure_events'] > 0, 'Native exposure absent')
            counts['native_exposures'] += receipt['trace']['native_exposure_events']
        for source, sha in run['sources'].items():
            require(digest(bound(manifest['source_archives'][sha])) == sha, 'Executed source version missing')
            if source in result['source_hashes']:
                require(result['source_hashes'][source] == sha, 'Qt receipt source identity differs')
        proof = load(case / 'proof.json')
        if name != 'QtGui.PlotIntegrationSteps':
            require(native_checks == [proof] and proof['outcome'] == 'passed', 'Original proof/log mismatch')
            for source in receipt['sources']:
                require(result['source_hashes'][source] == manifest['original_assertions'][source], 'Original regression assertion changed')
            reviewed.append({'test': name, 'mode': run['mode']}); continue
        require(proof['outcome'] == 'passed' and proof['new_requirement_credit'] == 0, 'Unexpected qualification claim')
        require(native_checks == proof['cases'], 'Native log and case proof differ')
        require([c['case'] for c in proof['cases']] == run['cases'], 'Selected case set differs')
        for row in proof['cases']:
            key = row['case']; seen[run['mode']].add(key)
            require(load(case / (key + '-proof.json')) == row, 'Per-case proof mismatch')
            require(load(case / (key + '-points.json')) == row['outputs'], 'Saved plotted data mismatch')
            if row.get('solver_states'):
                states = row['solver_states']; trials = row['trial_passes']
                finished = [all(v == 10014 for v in state) if isinstance(state, list) else state == 10014 for state in states]
                require(trials >= 2 and finished.count(True) == 1 and row['accepted_passes'] == 1 and len(states) == trials + 1 and finished[-1], 'Native solver-state oracle incomplete')
                grid = ([0] + [20, 37] * (trials + 1) + [50] if key.endswith('All') else [0, 20, 37, 50], [])
            else: grid = literal[key]
            require([(v['plot'], v['object']) for v in row['outputs']] == [('Orb', 'One'), ('Orb', 'Two'), ('Map', 'One'), ('Map', 'Two')], 'Missing or repeated curve')
            for curve in row['outputs']:
                n = 0 if curve['object'] == 'One' else 1
                require(len(curve['points']) == len(grid[n]), 'Literal grid length mismatch')
                for point, seconds in zip(curve['points'], grid[n]):
                    require(abs(point['elapsed'] - seconds) < 1e-4, 'Accepted-step epoch mismatch'); counts['epoch_values'] += 1
                    require(all(math.isfinite(point[k]) for k in ('x', 'y', 'z')), 'Nonfinite plot coordinate')
                    if key == 'TranslatedOrigin':
                        radius = 1500. + 500. * n; inc = math.pi / 6
                        angle = .7 + .2 * n + math.sqrt(1000. / radius**3) * seconds
                        xyz = (radius * math.cos(angle), radius * math.sin(angle) * math.cos(inc), radius * math.sin(angle) * math.sin(inc))
                    else:
                        radius, phase = (7000., 0.) if n == 0 else (9000., .4)
                        angle = phase + math.sqrt(398600.4415 / radius**3) * seconds
                        a, b = radius * math.cos(angle), radius * math.sin(angle)
                        xyz = (a, b, 0.) if n == 0 else (0., a, b)
                    if curve['plot'] == 'Orb':
                        require(all(abs(point[k] - truth) < 4e-5 for k, truth in zip(('x', 'y', 'z'), xyz)), 'Independent circular position mismatch'); counts['position_values'] += 3
                    elif key == 'TranslatedOrigin':
                        geo = (math.degrees(math.atan2(xyz[1], xyz[0])), math.degrees(math.atan2(xyz[2], math.hypot(*xyz[:2]))))
                        require(all(abs(point[k] - truth) < 2e-6 for k, truth in zip(('x', 'y'), geo)), 'Independent spherical geographic mismatch'); counts['geographic_values'] += 2
        reviewed.append({'test': name, 'mode': run['mode'], 'cases': run['cases']})
    require(seen['headless'] == seen['visible'] == set(manifest['expected_cases']), 'Unattended/visible conjunction incomplete')
    output = {'bounded_evidence_status': 'passed', 'new_requirement_credit': 0, 'requirements': {'verified': 1009, 'total': 1158, 'remaining': 149, 'needed_for_95_percent': 92}, 'checked': counts, 'runs': reviewed, 'manifest_sha256': digest(repo / args.manifest), 'full_current_build_qualified': False, 'full_NASA_qualified': False, 'open_gates': manifest['open_gates']}
    args.output.write_text(json.dumps(output, indent=2) + '\n')
    print(json.dumps({'bounded_evidence_status': 'passed', **counts, 'new_requirement_credit': 0}))


if __name__ == '__main__': main()
