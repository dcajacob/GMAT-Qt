"""Replay the owned-type function evidence without executing GMAT or granting rows."""
import argparse
from collections import Counter
import copy
from decimal import Decimal
import hashlib
import json
import math
from pathlib import Path
import tarfile
from xml.etree import ElementTree

STAGE = 'build/nasa-coverage/coverage95-20261104-function-owned-type-matrix'
PRIOR = 'build/nasa-coverage/coverage95-20261103-function-environment-bindings'
CANON = 'build/nasa-coverage/coverage95-20261103-matlab-c-interface'
LEDGER_SHA = '4d7da4f31ba802900eb7a9eec2fd085dff86cd1ba365b1c23ed5959962bc34e6'
PREFIX = 'QtGui.NasaFunctionOwnedType'
OPEN_TYPES = {'Topocentric', 'Code500', 'STK', 'CCSDS-OEM'}


def check(value, message):
    if not value:
        raise ValueError(message)


def load(path):
    return json.loads(Path(path).read_text())


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def numeric(actual, expected, bound):
    check(len(actual) == len(expected), 'Numeric width differs')
    for a, e in zip(actual, expected):
        check(math.isfinite(a) and math.isfinite(e) and abs(a-e) <= bound,
              'Independent numerical value differs')


def expected_state(sample, spec, root):
    inside = sample['inside_function']
    inputs = sample['oracle_inputs']
    if spec['family'] == 'Axes':
        authored = [1250, -250, 450, 1.5, -.4, .2] if inside else [1000, -200, 300, 1.25, -.5, .75]
        numeric(inputs['authored_state'], authored, 0)
        check(inputs['reference_pointer'] not in {'0', sample['resource']['pointer']},
              'Separate reference frame absent')
        check(inputs['reference_base'] == ('ICRF' if spec['factory_type'] == 'ICRF' else 'FK5'),
              'Reference base differs')
        R, D = inputs['rotation'], inputs['rotation_dot']
        check(len(R) == len(D) == 9, 'Frame matrix shape differs')
        # These are explicitly shared native frame/origin inputs. This verifies
        # matrix application and scope identity, not independent frame physics.
        return [inputs['origin_state'][i] - inputs['earth_state'][i] +
                sum(R[(i % 3)*3+j] * authored[j + (0 if i < 3 else 3)] +
                    (D[(i % 3)*3+j] * authored[j] if i >= 3 else 0)
                    for j in range(3)) for i in range(6)]
    elapsed = sample['resource']['executed']['elapsed_seconds']
    requested = spec['requested_private_seconds' if inside else 'requested_root_seconds']
    check(abs(elapsed-requested) <= spec['stop_time_bound_seconds'], 'Original stop bound differs')
    if spec['family'] == 'Numerical':
        radius = 7000 if inside else 9000
        n = math.sqrt(400000/(radius**3))
        angle = n*elapsed
        return [radius*math.cos(angle), radius*math.sin(angle), 0,
                -radius*n*math.sin(angle), radius*n*math.cos(angle), 0]
    if spec['factory_type'] == 'SPICESGP4':
        # Root and private controls have the same authored epoch and TLE.
        # No independent SGP4 scientific accuracy is inferred from parity.
        return root['actual']
    check(spec['factory_type'] == 'SPK' and inputs['file_time_system'] == 'TDB',
          'Unresolved text ephemeris cannot be selected as a pass')
    seconds = inputs['seconds_real']
    check(abs(float((Decimal(inputs['query_gt']) - Decimal(inputs['start_gt']))*86400) -
              inputs['seconds_gt']) < 1e-10, 'Precise epoch diagnostic differs')
    # The preserved SPK recipe authors its grid through the shared legacy TDB
    # converter. Affine position arithmetic is independently replayed here.
    r = [7000, -200, 450] if inside else [9000, 100, 300]
    v = [.2, 6.8, -.3] if inside else [-.1, 7.1, .4]
    return [r[i]+v[i]*seconds for i in range(3)] + v


def contract(proof, spec, counts):
    kind = spec['factory_type']
    check(kind not in OPEN_TYPES and proof['case'] == kind and proof['outcome'] == 'passed',
          'Selected type/outcome differs')
    check(proof['checkpoint'] == 'complete-owned-type-function-scoping' and
          proof['factory_rows'] == 566 and proof['selected_missing_types'] == 32,
          'Factory universe differs')
    check(proof['missions'] == 3 and proof['function_calls'] == proof['live_callbacks'] == 12,
          'Mission/callback census differs')
    check(proof['file_decisions'] == [{'saved': False}, {'saved': True}] and
          proof['whole_row_qualified'] is False and proof['new_requirement_credit'] == 0 and
          proof['independent_orientation_provider_truth'] is False,
          'Qualification gates or actual file decisions differ')
    check([p['phase'] for p in proof['phases']] == [0, 1, 2] and
          [p['explicit_rebuild'] for p in proof['phases']] == [True, True, False],
          'Reopen/no-Build sequence differs')
    for phase in proof['phases']:
        samples = phase['live_samples']
        check(len(samples) == 4, 'Live callback absent')
        expected_raw = []
        for call, sample in enumerate(samples):
            inside = call > 0
            check(sample['calls'] == call and sample['inside_function'] == inside and
                  sample['owner'] == ('FunctionSC' if inside else 'MainSC') and
                  sample['owner_pointer'] != '0', 'Live owner scope differs')
            check(sample['shared_pointer'] == samples[0]['shared_pointer'] != '0' and
                  sample['configured_shared_pointer'] != sample['shared_pointer'],
                  'Shared/configured identities differ')
            resource = sample['resource']
            check(resource['automatic'] and resource['global'] and
                  resource['owned_type'] == kind and resource['owned_pointer'] != '0',
                  'Actual owned implementation differs')
            check(resource['name'] == ('FunctionOwned' if inside else 'Shared') and
                  resource['spacecraft_solar_system'] != '0' and
                  resource['spacecraft_j2000_body'] == resource['expected_j2000_body'] != '0',
                  'Restored borrowed context differs')
            if inside:
                check(resource['pointer'] != sample['shared_pointer'] and
                      sample['owner_pointer'] != samples[0]['owner_pointer'],
                      'Private owner/resource aliases caller')
            if spec['family'] != 'Axes':
                run = resource['executed']
                check(run['pointer'] != resource['pointer'] and run['owned_type'] == kind and
                      run['uses_ode'] == (spec['family'] == 'Numerical'),
                      'Actual Propagate-owned implementation differs')
                if spec['family'] == 'Numerical':
                    check(run['force_seconds'] == run['elapsed_seconds'], 'Force/method clocks differ')
            expected = expected_state(sample, spec, samples[0])
            for i in range(6):
                bound = spec['position_bound_km' if i < 3 else 'velocity_bound_km_s']
                numeric([sample['actual'][i]], [expected[i]], bound)
                numeric([sample['expected'][i]], [expected[i]], bound/100)
            if call:
                expected_raw.append(41+call)
            expected_raw += expected
            counts['live_callbacks'] += 1
            counts['state_components_checked'] += 6
            if kind == 'SPICESGP4':
                counts['shared_sgp4_parity_components' if inside else 'root_sgp4_control_components'] += 6
            else:
                counts['analytic_or_algebraic_components'] += 6
        expected_raw += [103, 104]
        check(len(expected_raw) == len(phase['raw_values']) == len(phase['expected_values']) == 29,
              'Raw Report width differs')
        for i, expected in enumerate(expected_raw):
            marker = i in {6, 13, 20, 27, 28}
            column = i if i < 6 else (i-7) % 7
            bound = 0 if marker else spec['position_bound_km' if column < 3 else 'velocity_bound_km_s']
            numeric([phase['raw_values'][i]], [expected], bound)
            numeric([phase['expected_values'][i]], [expected], bound)
        counts['missions'] += 1


def invocation(stage, run, test, mode, counts):
    root = stage/run/test
    result = load(root/'result.json')
    check(result['status'] == 'passed' and result['returncode'] == 0 and
          result['requested_visibility'] == mode, 'Native result/mode differs')
    cases = ElementTree.parse(root/'ctest.xml').findall('.//testcase')
    check(len(cases) == 1 and cases[0].get('name') == test and
          all(cases[0].find(k) is None for k in ['error', 'failure', 'skipped']), 'JUnit differs')
    check(not result['trace']['screenshots'], 'Unexpected screenshot')
    if mode == 'visible':
        desktop = load(stage/run/'results.json')['visible_display']
        check(desktop['resizable'] and desktop['start_minimized'] and
              'Iconic' in desktop['initial_host_window_state'] and
              'FULLSCREEN' not in desktop['initial_host_window_state'], 'Visible desktop policy differs')
        check(result['trace']['recorded_platforms'] == ['xcb'] and
              result['trace']['native_exposure_events'] > 0, 'Native exposures absent')
        counts['visible_exposures'] += result['trace']['native_exposure_events']
    counts['passed_native_contexts'] += 1
    return root


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--repo', type=Path, default=Path.cwd())
    p.add_argument('--stage', type=Path)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--manifest', type=Path)
    args = p.parse_args()
    repo = args.repo.resolve()
    stage = args.stage.resolve() if args.stage else repo/STAGE
    counts = Counter()
    if args.manifest:
        manifest = load(args.manifest)
        check(sha(__file__) == manifest['auditor_sha256'], 'Auditor identity differs')
        for path, digest in manifest['artifact_sha256'].items():
            check(sha(repo/path) == digest, 'Sealed artifact differs: '+path)
        with tarfile.open(repo/manifest['final_source_archive']) as tar:
            check(set(tar.getnames()) == set(manifest['final_source_sha256']), 'Final source inventory differs')
            for path, digest in manifest['final_source_sha256'].items():
                check(hashlib.sha256(tar.extractfile(path).read()).hexdigest() == digest,
                      'Final frozen source differs: '+path)
        counts['artifact_bindings'] = len(manifest['artifact_sha256'])
    check(sha(repo/CANON/'coverage-snapshot-v1.json') == LEDGER_SHA, 'Canonical ledger changed')
    ledger = load(repo/CANON/'coverage-snapshot-v1.json')
    check(ledger['coverage']['requirements']['verified'] == 1050 and
          ledger['coverage']['requirements']['total'] == 1158 and
          ledger['coverage']['historical_cases']['verified'] == 1816,
          'Original requirement/historical coverage changed')
    prior = repo/PRIOR
    check(sha(prior/'stage-receipt-v1.json') ==
          'e10b50b0280fd9b1abc78ac280b2625f67ef1b28872ed7944f53cddc80b22ea7' and
          sha(prior/'evidence-manifest-v2.json') ==
          'c3d40b455f843370736ef51c28be20bee211d61807b1bf67dad3fe65c991c251',
          'Sealed 321-type prefix identity differs')
    old_manifest = load(prior/'evidence-manifest-v2.json')
    for path, digest in old_manifest['artifact_sha256'].items():
        check(sha(repo/path) == digest, 'Retained prefix artifact differs: '+path)
    counts['retained_prefix_artifact_bindings'] = len(old_manifest['artifact_sha256'])
    check(load(stage/'prior-environment-replay-v1.json') == load(prior/'evidence-audit-v5.json'),
          'Independent previous-prefix replay differs')
    old = load(prior/'environment-parameter-binding-matrix-v1.json')
    matrix = load(stage/'owned-type-binding-matrix-v1.json')
    check(len(old['rows']) == len(matrix['rows']) == 566 and
          sum(r['role'] == 'owner-bound-system-parameter' for r in matrix['rows']) == 321,
          'Complete role matrix differs')
    for a, b in zip(old['rows'], matrix['rows']):
        check(all(b[k] == v for k, v in a.items()), 'Prior role/eligibility/obligation changed')
    previous = load(prior/'remaining-requirement-audit-v1.json')
    remaining = load(stage/'remaining-requirement-audit-v1.json')
    check(len(remaining['rows']) == len(previous['rows']) == 108 and
          remaining['original_hold_keys'] == previous['original_hold_keys'] and
          len(set(remaining['original_hold_keys'])) == 59, 'Exact gaps/hold identities differ')
    for a, b in zip(previous['rows'], remaining['rows']):
        check(all(b[k] == v for k, v in a.items()), 'Original fingerprint/gap/gate changed')
    catalogs, native_sources = {}, {}
    for version in range(1, 5):
        sources = load(stage/f'executed-source-sha256-v{version}.json')
        build = load(stage/f'build-receipt-v{version}.json')
        check(build['status'] == 'passed' and build['returncode'] == 0 and
              build['inputs_unchanged'] and build['source_sha256'] == sources,
              'Matching successful build absent')
        with tarfile.open(stage/f'executed-sources-v{version}.tar.gz') as tar:
            check(set(tar.getnames()) == set(sources), 'Execution source inventory differs')
            for path, digest in sources.items():
                check(hashlib.sha256(tar.extractfile(path).read()).hexdigest() == digest,
                      'Execution source changed')
            catalogs[version] = json.loads(tar.extractfile('src/qtgui/tests/NasaFunctionOwnedTypeCases.json').read())
            native_sources[version] = tar.extractfile('src/qtgui/tests/NasaFunctionOwnedTypeTests.cpp').read().decode()
        counts['source_capsules'] += 1
    initial_catalog = catalogs[1]
    for version, catalog in catalogs.items():
        check(catalog['full_owned_universe'] == initial_catalog['full_owned_universe'] and
              catalog['previously_witnessed'] == initial_catalog['previously_witnessed'],
              'Original complete owned universe differs')
        check(len(catalog['cases']) == len(initial_catalog['cases']) == 32, 'Catalog cases dropped')
        for a, b in zip(initial_catalog['cases'], catalog['cases']):
            for key in a:
                if a['key'] == 'AdamsBashforthMoulton' and key in {'root_configuration', 'private_configuration'}:
                    # Correct only the invalid ABM input error inequalities.
                    configured = b[key]
                    configured = '\n'.join(line for line in configured.split('\n') if
                        '.LowerError = ' not in line and '.TargetError = ' not in line)
                    check(configured == a[key], 'Unexpected ABM recipe change')
                else:
                    check(a[key] == b[key], 'Original case bound/metadata/input changed')
    preservation = load(stage/'assertion-and-source-preservation-v1.json')
    check(preservation['initial_require_calls'] == 42 and preservation['final_require_calls'] == 43 and
          preservation['verbatim_normalized_require_calls'] == 41 and
          preservation['original_require_conditions_preserved'] and
          'const bool loaded=rig.window.loadScript(saved),rebuilt=loaded&&rig.window.buildScript();' in native_sources[4],
          'Original assertion preservation evidence differs')
    increment = load(stage/'core-increment-v1.json')
    check(increment['pending_core_paths'] == 263 and increment['prior_paths_unchanged'] == 262 and
          increment['new_pending_paths'] == 0 and len(increment['increments']) == 1 and
          increment['patch_sha256'] == sha(stage/'core-increment-v1.patch'), 'Core increment inventory differs')
    for row in increment['increments']:
        check(sha(repo/row['before']) == row['before_sha256'] and
              sha(repo/row['after']) == row['after_sha256'] and
              load(stage/'executed-source-sha256-v4.json')[row['path']] == row['after_sha256'] and
              row['new_virtual_slots'] == row['new_members'] == row['new_eligibility_flags'] == 0,
              'Exact core pre/postimage differs')
    receipt = load(stage/'stage-receipt-v1.json')
    process_audit = load(stage/'recorded-process-audit-v1.json')
    check(process_audit['matching_native_processes'] == [] and
          process_audit['matching_visible_servers'] == [] and
          process_audit['execution_runs'] == len(receipt['execution_versions']),
          'Recorded process termination audit differs')
    seen = set()
    for run, version in receipt['execution_versions'].items():
        maps = load(stage/(run+'-loaded-images.json'))
        build = load(stage/f'build-receipt-v{version}.json')
        check(maps['inputs_unchanged'] and maps['runtime_binary_copies'] == 0 and
              maps['source_sha256'] == build['source_sha256'] and
              maps['binary_sha256'] == build['binary_sha256'] and
              maps['build_receipt_sha256'] == sha(stage/f'build-receipt-v{version}.json'),
              'Native maps/build/source custody differs')
        check(maps['processes'], 'Actual native processes absent')
        for process in maps['processes']:
            for snapshot in process['snapshots']:
                check(hashlib.sha256(snapshot['maps'].encode()).hexdigest() == snapshot['maps_sha256'],
                      'Raw maps changed')
                for path, digest in snapshot['project_images'].items():
                    check(maps['loaded_project_images'][path] == digest, 'Mapped image differs')
            check(any(s['project_images'].get(process['executable']) ==
                      build['binary_sha256'][process['executable']] for s in process['snapshots']),
                  'Executed binary identity absent')
        counts['mapped_processes'] += len(maps['processes'])
    selection = load(stage/'selected-evidence-v1.json')
    catalog = catalogs[4]
    kinds = {s['factory_type'] for s in catalog['cases']}
    check(len(kinds) == 32 and catalog['factory_rows'] == 566 and
          catalog['selected_missing_count'] == 32 and len(selection) == 56 and
          {(s['case'], s['mode']) for s in selection} ==
          {(k, m) for k in kinds-OPEN_TYPES for m in ['headless', 'visible']},
          'Selected type/profile coverage differs')
    tests = []
    for selected in selection:
        check(selected['source_version'] == 4, 'Final direct context assertion absent')
        root = invocation(stage, selected['run'], PREFIX+selected['case'], selected['mode'], counts)
        check(sha(root/'proof.json') == selected['proof_sha256'], 'Selected proof hash differs')
        check(sha(root/'executed-catalog.json') ==
              load(stage/'executed-source-sha256-v4.json')['src/qtgui/tests/NasaFunctionOwnedTypeCases.json'],
              'Actual native catalog differs')
        proof = load(root/'proof.json')
        spec = next(s for s in catalog['cases'] if s['key'] == selected['case'])
        contract(proof, spec, counts)
        check(load(root/'phases.json') == proof['phases'], 'Phase sidecar differs')
        check((root/'initial.gmf').read_bytes().replace(b'PrivateOnly = 103;', b'PrivateOnly = 41;') ==
              (root/'edited.gmf').read_bytes(), 'Function file edit differs')
        check(load(root/'reopened-prototypes.json') == load(root/'prototypes-before.json'),
              'Fresh-build prototype equality differs')
        for phase in range(3):
            check(all(load(root/f'{phase}-invariants.json').values()), 'Isolation invariant differs')
            raw = [float(x) for x in (root/f'{phase}-values.txt').read_text().split()]
            numeric(raw, proof['phases'][phase]['raw_values'], 0)
        tests.append((proof, spec))
    for reg in receipt['regressions']:
        invocation(stage, reg['run'], reg['test'], reg['mode'], counts)
    check(len(receipt['regressions']) == 30 and len({r['test'] for r in receipt['regressions']}) == 15,
          'Affected original regression census differs')
    check({(r['test'], r['mode']) for r in receipt['regressions']} ==
          {(r['test'], mode) for r in receipt['regressions'] for mode in ['headless', 'visible']},
          'Original regression profile omitted or duplicated')
    failure_count = 0
    for run, version in receipt['execution_versions'].items():
        results = load(stage/run/'results.json')
        for result in results['results']:
            if result['status'] == 'failed':
                check((stage/run/result['name']/'result.json').is_file(), 'Failure artifact missing')
                failure_count += 1
    check(failure_count == receipt['retained_failed_contexts'], 'Retained failure count differs')
    # Exercise the auditor on plausible damage, not merely precomputed hashes.
    original, spec = next((p, s) for p, s in tests if s['factory_type'] == 'SPK')
    mutations = [
        lambda p: p['phases'][0]['live_samples'].pop(),
        lambda p: p['phases'][0]['live_samples'][1].update(inside_function=False),
        lambda p: p['phases'][0]['live_samples'][1].update(shared_pointer='0'),
        lambda p: p['phases'][0]['live_samples'][1]['actual'].__setitem__(0, 123.),
        lambda p: p['phases'][0]['live_samples'][1]['resource'].update(spacecraft_j2000_body='0'),
        lambda p: p['phases'][0]['live_samples'][1]['resource']['executed'].update(owned_type='Wrong'),
        lambda p: p['phases'][2].update(explicit_rebuild=True),
        lambda p: p.update(whole_row_qualified=True),
    ]
    for mutate in mutations:
        damaged = copy.deepcopy(original)
        mutate(damaged)
        rejected = False
        try:
            contract(damaged, spec, Counter())
        except (ValueError, KeyError, IndexError):
            rejected = True
        check(rejected, 'Damaged proof escaped audit')
        counts['negative_proofs_rejected'] += 1
    check(receipt['canonical_verified'] == 1050 and receipt['original_total'] == 1158 and
          receipt['new_requirement_credit'] == 0 and receipt['whole_row_qualified'] is False and
          set(receipt['remaining_owned_types']) == OPEN_TYPES, 'Stage scope inflated')
    result = dict(status='passed-bounded-owned-type-evidence', counts=dict(counts),
                  canonical_verified=1050, original_total=1158, target=1101,
                  still_needed=51, open_exact_rows=108, original_holds=59,
                  new_requirement_credit=0, whole_row_qualified=False,
                  retained_failed_contexts=failure_count,
                  remaining_owned_types=sorted(OPEN_TYPES),
                  scientific_technical_current_build_NASA_qualified=False)
    check(not args.output.exists(), 'Never overwrite an audit')
    args.output.write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps(result))


if __name__ == '__main__':
    main()
