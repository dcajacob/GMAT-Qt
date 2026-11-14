"""Audit retained committed-step publication evidence without running GMAT.

This admits no NASA row and does not assert full current-build qualification.
"""
import argparse
import hashlib
import json
import math
import xml.etree.ElementTree as ET
from pathlib import Path


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def check(value, message):
    if not value:
        raise ValueError(message)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, default=Path(__file__).resolve().parents[3])
    parser.add_argument('--manifest', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    repo = args.repo.resolve()
    manifest_path = (repo / args.manifest).resolve()
    manifest = json.loads(manifest_path.read_text())
    check(manifest['auditor_sha256'] == sha(Path(__file__)), 'Auditor version differs')
    for relative, digest in manifest['artifacts'].items():
        path = repo / relative
        check(path.is_file() and sha(path) == digest, 'Changed/missing retained artifact: ' + relative)
    def bound(path):
        path = Path(path)
        if not path.is_absolute(): path = repo / path
        relative = str(path.relative_to(repo))
        check(relative in manifest['artifacts'], 'Consumed path is not manifest-bound: ' + relative)
        return path

    def load(path):
        return json.loads(bound(path).read_text())

    def native_receipt(directory, name, expected='passed', proof_required=True):
        directory = repo / directory
        row = load(directory / name / 'result.json')
        check(row['status'] == expected and (row['returncode'] == 0) == (expected == 'passed'), 'Native status differs: ' + name)
        xml = ET.parse(bound(directory / name / 'ctest.xml')).getroot()
        cases = xml.findall('.//testcase')
        check(len(cases) == 1 and cases[0].get('name') == name, 'Native JUnit case identity differs')
        failures = cases[0].find('failure') is not None or cases[0].find('error') is not None
        check(failures == (expected == 'failed') and cases[0].find('skipped') is None, 'Native JUnit result differs')
        if expected == 'passed':
            reports = []
            for line in bound(directory / name / 'test.log').read_text().splitlines():
                if 'NASA_CHECK ' in line: reports.append(json.loads(line.split('NASA_CHECK ', 1)[1]))
            check(len(reports) == 1 and reports[0].get('outcome') == 'passed', 'Missing exact native passing check')
            proof_path = directory / name / 'proof.json'
            if proof_required or proof_path.exists():
                check(load(proof_path) == reports[0], 'Native log and proof differ')
        return row

    root = repo / manifest['visible']
    result = json.loads((root / 'results.json').read_text())
    case = root / 'QtGui.StepPublication'
    receipt = native_receipt(manifest['visible'], 'QtGui.StepPublication')
    proof = json.loads((case / 'proof.json').read_text())
    check(receipt['status'] == 'passed' and receipt['returncode'] == 0, 'Visible test failed')
    check(len(proof['cases']) == 8 and proof['outcome'] == 'passed', 'Incomplete visible family')
    check(receipt['trace']['recorded_platforms'] == ['xcb'] and
          receipt['trace']['native_exposure_events'] > 0 and
          receipt['trace']['screenshots'] == [], 'Missing native exposure/zero-screenshot evidence')
    display = result['visible_display']
    check(display['resizable'] and display['start_minimized'] and
          'Iconic' in display['initial_host_window_state'] and
          'FULLSCREEN' not in display['initial_host_window_state'], 'Visible window policy differs')
    frozen = load(manifest['source_guard'])
    recovered = load(manifest['recovered_versions'])['versions']
    for source, row in frozen['sources'].items():
        check(sha(repo / row['archive']) == row['sha256'], 'Frozen source changed: ' + source)
        if source in result['source_hashes']:
            check(result['source_hashes'][source] == row['sha256'], 'Executed source differs: ' + source)
    catalogue = load(frozen['original_assertion_catalogue'])
    for source, digest in frozen['original_assertions'].items():
        check(catalogue['source_hashes'][source] == digest == frozen['sources'][source]['sha256'], 'Original assertion source changed')
    maps = load(manifest['loaded_images'])
    check(maps['returncode'] == 0 and len(maps['processes']) == 1, 'Missing actual process observation')
    loaded = next(iter(maps['processes'].values()))['images']
    for path, digest in loaded.items():
        check(frozen['image_sha256'][path] == digest, 'Loaded project image differs: ' + path)
    for essential in ['application/bin/libGmatBase.so.R2026a',
                      'application/plugins/libExtraPropagators.so.R2026a',
                      'build/linux-gui/src/qtgui/GmatQtStepPublicationTests']:
        check(essential in loaded, 'Essential image was not observed: ' + essential)
    final_times = {'ForwardSplit': 37., 'BackwardSplit': -37., 'SingleSteps': 20.,
                   'Synchronized': 23., 'SynchronizedBackward': -23., 'Formation': 25.,
                   'NonlinearABM': 37.123, 'NonlinearABMQuarter': 37.25}
    values = 0
    maximum = [0., 0.]
    def verify_case(case, name, final_time):
        nonlocal values
        baseline = load(case / (name + '-legacy-packets.json'))
        normal = load(case / (name + '-endpoints-packets.json'))
        consumer = load(case / (name + '-endpoints-consumer.json'))
        check([p['raw'] for p in baseline] == [p['raw'] for p in normal], 'Legacy packet changed: ' + name)
        latest, advances = {}, {}
        for packet in consumer:
            check(packet['metadata'], 'Missing optional metadata: ' + name)
            for step in packet['steps']:
                end = (step['epoch'] - 21545.) * 86400.
                start = end - (step['end'] - step['start'])
                for sample in step['states']:
                    check(len(sample['state']) == 6 and sample['epoch'] == step['epoch'], 'Incomplete Cartesian state/epoch binding')
                    obj = sample['object']
                    check(obj in ('One', 'Two'), 'Unknown spacecraft binding')
                    radius, phase = (7000., 0.) if obj == 'One' else (9000., .4)
                    omega = math.sqrt(398600.4415 / radius**3)
                    angle = phase + omega * end
                    x, y = radius * math.cos(angle), radius * math.sin(angle)
                    vx, vy = -radius * omega * math.sin(angle), radius * omega * math.cos(angle)
                    expected = (x, y, 0., vx, vy, 0.) if obj == 'One' else (0., x, y, 0., vx, vy)
                    for component, (actual, truth) in enumerate(zip(sample['state'], expected)):
                        check(math.isfinite(actual), 'Nonfinite Cartesian value')
                        error = abs(actual - truth)
                        check(error < (3e-5 if component < 3 else 4e-8), 'Independent Cartesian oracle failed')
                        maximum[component // 3] = max(maximum[component // 3], error)
                        values += 1
                    if step['initial']:
                        check(obj not in latest or abs(latest[obj] - end) < 1e-6, 'Command seed advanced trajectory')
                    else:
                        check(obj in latest and abs(latest[obj] - start) < 1e-6, 'Probe/duplicate/discontinuity published')
                        check((end - start) * final_time > 0 and abs(end) <= abs(final_time) + 2e-5, 'Direction/stop boundary differs')
                        advances[obj] = advances.get(obj, 0) + 1
                    latest[obj] = end
        check(abs(latest['One'] - final_time) < 2e-5, 'Final endpoint omitted')
        if name.startswith('NonlinearABM'):
            raw = load(case / (name + '-endpoints-ambient-native-trials.json'))
            check(len(raw) == 43 and advances['One'] == 38, 'Nonlinear raw/committed counts differ')
            for packet in consumer:
                for step in packet['steps']:
                    if not step['initial']:
                        check(any(all(step[key] == row[key] for key in ['start', 'end', 'method']) for row in raw), 'Published endpoint absent from raw journal')
    for name, final_time in final_times.items():
        verify_case(case, name, final_time)
    check(values == 780, 'Incomplete Cartesian matrix')
    visible_values = values
    # Reuse completed subcases from failed invocations without changing their status.
    unattended = {}
    for relative in manifest['unattended_versioned_receipts']:
        directory = repo / relative
        expected = manifest['unattended_parent_statuses'][relative]
        native_receipt(relative, 'QtGui.StepPublication', expected=expected)
        run = load(directory / 'results.json')
        for source, current in frozen['sources'].items():
            if source not in run['source_hashes']: continue
            digest = run['source_hashes'][source]
            archived = current['archive'] if digest == current['sha256'] else recovered.get(source, {}).get(digest)
            check(archived is not None and sha(bound(archived)) == digest, 'Missing executed source bytes: ' + source)
        rows = load(directory / 'QtGui.StepPublication/case-results.json')
        if expected == 'passed':
            check(load(directory / 'QtGui.StepPublication/proof.json')['cases'] == rows, 'Unattended proof/subcases differ')
        for row in rows:
            name = row['case']
            check(name in final_times and name not in unattended, 'Unknown/duplicate reused subcase')
            verify_case(directory / 'QtGui.StepPublication', name, final_times[name])
            unattended[name] = relative
    check(set(unattended) == set(final_times) and values == 1560, 'Missing unattended subcase evidence')
    affected = repo / manifest['affected']
    for name in ['PublicationEpochContract', 'SharedSampleContract', 'OriginRelativeStateContract']:
        row = native_receipt(manifest['affected'], 'QtGui.' + name, proof_required=False)
        check(row['status'] == 'passed' and row['returncode'] == 0, 'Affected contract failed: ' + name)
    ledger = load(manifest['unchanged_ledger'])
    check(ledger['coverage']['requirements']['verified'] == 1009 and
          ledger['coverage']['requirements']['total'] == 1158, 'Coverage changed in zero-credit layer')
    output = dict(status='passed', manifest_sha256=sha(manifest_path), visible_cases=8,
                  independent_Cartesian_values=visible_values,
                  unattended_independent_Cartesian_values=values-visible_values, maximum_position_error_km=maximum[0],
                  maximum_velocity_error_km_s=maximum[1], unattended_case_receipts=unattended,
                  native_exposures=receipt['trace']['native_exposure_events'], affected_contracts=3,
                  original_requirements_verified=1009, new_NASA_credit=0,
                  full_current_build_qualified=False, full_NASA_qualified=False,
                  limits=manifest['limits'])
    args.output.write_text(json.dumps(output, indent=2) + '\n')
    print(json.dumps({k: v for k, v in output.items() if k not in ('unattended_case_receipts', 'limits')}))


if __name__ == '__main__':
    main()
