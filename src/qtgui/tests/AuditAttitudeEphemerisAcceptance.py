"""Admit the attitude and ephemeris requirements from frozen execution evidence."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import tarfile

import AuditNasaCoveragePrefix as prefix
import BuildNasaCoverageExtension as common
import NasaGuiCoverage as nasa


sha, load, require = common.sha, common.load, common.require
KEYS = ['FRR!E53', 'FRR!E50', 'FRC!E114', 'All Others!E252']
ATTITUDE = [f'FRR!E{i}' for i in range(50, 54)]
PROPAGATE = [f'FRC!E{i}' for i in range(111, 125)]
KIND = 'source-versioned-CK-attitude-and-exact-ephemeris-propagation'
GATES = ['scientific_approval', 'technical_core_approval', 'full_current_build_qualified',
         'full_NASA_qualified', 'native_OS_input_qualified']


def verify_attitude(paths):
    good = Path(paths['ck_positive'])
    outside = Path(paths['ck_outside'])
    wrong_clock = Path(paths['ck_wrong_clock'])
    expected = []
    for angle in (0, 37, 74):
        c, s = math.cos(math.radians(angle)), math.sin(math.radians(angle))
        expected.extend([c, -s, 0, s, c, 0, 0, 0, 1, 0, 0, -.37])
    positive = load(good / 'result.json')
    actual = [float(value) for value in (good / 'angles.txt').read_text().split()]
    require(len(actual) == len(expected) and actual == positive['actual'],
            'Missing independent attitude outputs at all three epochs')
    errors = [abs(x - y) for x, y in zip(actual, expected)]
    require(all(error <= 2e-8 for error in errors), 'Attitude differs from authored rotation')
    for case, steps in [(good, 2), (outside, 3)]:
        plan = load(case / 'plan.json')
        mission = (case / 'mission.script').read_text()
        require(plan['expected'] == expected and plan['absolute_bound'] == 2e-8
                and all(sha(name) == digest for name, digest in plan['kernel_inputs'].items())
                and 'Sat.Attitude = SpiceAttitude;' in mission
                and 'Sat.NAIFId = -12341;' in mission and 'Sat.NAIFIdReferenceFrame = -12341001;' in mission
                and mission.count('Propagate Prop(Sat) {Sat.ElapsedSecs = 100};') == steps,
                'Changed original CK plan, kernels or propagation steps')
    rejection = load(outside / 'result.json')
    wrong = load(wrong_clock / 'result.json')
    require(positive['status'] == 'passed' and positive['returncode'] == 0
            and rejection['status'] == wrong['status'] == 'passed'
            and rejection['returncode'] != 0 and wrong['returncode'] != 0
            and rejection['actual'] == actual and not wrong['actual']
            and rejection['diagnostic_observed'] and wrong['diagnostic_observed']
            and [float(value) for value in (outside / 'angles.txt').read_text().split()] == actual,
            'Missing out-of-coverage or mismatched-clock rejection')
    marker = ('PASS: SPICE kernel GUI lists, Qt OF conversion, orbit/attitude samples, report agreement, '
              'round trips and unavailable-clock recovery')
    require(len(paths['attitude_gui']) == 2, 'Missing attitude GUI profile')
    for name in paths['attitude_gui']:
        case = Path(name)
        result = load(case / 'result.json')
        require(result['status'] == 'passed' and result['returncode'] == 0
                and marker in (case / 'test.log').read_text(),
                'Missing actual kernel GUI transaction, orbit/attitude checks or recovery')
    require(all(load(name)['status'] == 'failed' for name in paths['retained_failures']),
            'Changed original failure receipts')
    return max(errors)


def verify_ephemeris(paths):
    cases = {case['case']: case for case in load(paths['native_count'])['cases']}
    require(len(cases) == 12 and all(x['passed'] and x['returncode'] == 0 and x['proof']['passed']
                                    for x in cases.values()), 'Incomplete native count-policy suite')
    counts = cases['ExactlyOneGroupCounts']['proof']['counts']
    require([(x['policy'], x['shape'], x['expected_accept'], x['accepted']) for x in counts]
            == [('ExactlyOneGroup', shape, shape in (2, 5), shape in (2, 5)) for shape in range(7)]
            and all(x['error'] for x in counts if not x['accepted']), 'Changed exact group cardinality checks')
    native = [x for x in cases['CompatibilityCounts']['proof']['counts'] if x['policy'] == 'Native']
    require(len(native) == 7 and all(x['accepted'] for x in native), 'Missing native compatibility cases')
    syntax = cases['ExactlyOneGroupSyntax']['proof']['syntax']
    require([x['quoted'] for x in syntax] == [False, True]
            and all(x['copy_and_assignment'] and 'EphemerisPolicy = ExactlyOneGroup' in x['generated']
                    for x in syntax), 'Missing policy syntax or copy checks')
    for name, direction in [('Forward', 1), ('Backward', -1), ('Mixed', 1)]:
        proof = cases['ExactlyOneGroup' + name]['proof']
        require(proof['mission_status'] == 1 and len(proof['observations']) == 1,
                'Missing independently checked ' + name + ' execution')
        observation = proof['observations'][0]
        require(observation['seconds'] * direction > 0
                and 0 <= observation['position_error_km'] < 2e-5
                and 0 <= observation['velocity_error_km_s'] < 2e-9,
                'Changed independent affine state bounds or direction')
    require(cases['ExactlyOneGroupMixed']['proof']['mixed_numeric_finite']
            and cases['ExactlyOneGroupZero']['proof']['zero_rejected_before_execution'],
            'Missing mixed execution or zero-count rejection')
    require(len(paths['ephemeris_gui']) == 2, 'Missing ephemeris GUI profile')
    for name in paths['ephemeris_gui']:
        case = Path(name)
        result, proof = load(case / 'result.json'), load(case / 'proof.json')
        require(result['status'] == 'passed' and result['returncode'] == 0 and proof['outcome'] == 'passed'
                and any(x['policy'] == 'ExactlyOneGroup' and x['native_SPK_completed']
                        and x['zero_rejected_in_GUI'] and x['source_history_quotes_unicode']
                        for x in proof['observations']), 'Missing actual policy GUI execution')


def verify_capi(paths):
    base = load(paths['capi_propagate'])
    ephemeris = load(paths['capi_ephemeris'])
    require(base['status'] == ephemeris['status'] == 'passed'
            and base['native_C_case_passes'] == 8 and ephemeris['case_passes'] == 13
            and base['executed_production_and_C_sources_identical']
            and ephemeris['source_and_binary_inventories_identical']
            and base['independent_cartesian_scalars'] == 192
            and ephemeris['independent_native_Newton_derivative_checks'] == 252,
            'Missing saved C loader, numerical and ephemeris configuration coverage')
    directory = Path(paths['capi_exact'])
    receipt = load(directory / 'receipt.json')
    require(receipt['status'] == 'passed' and receipt['inputs_unchanged']
            and receipt['original_assertion_changes'] == 0 and len(receipt['results']) == 3,
            'Incomplete C loader policy run')
    for name, status, assertions in [('exact-group-one', 0, 255),
                                      ('exact-group-zero-rejected', -2, 144),
                                      ('exact-group-two-rejected', -2, 144)]:
        case = directory / name
        result, oracle = load(case / 'receipt.json'), load(case / 'oracle.json')
        require(result in receipt['results'] and result['status'] == 'passed' and result['returncode'] == 0
                and result['fixture_inputs_before'] == result['fixture_inputs_after']
                and not result['unbound_project_images']
                and oracle['policy'] == 'ExactlyOneGroup'
                and oracle['load_status'] == 0 and oracle['run_status'] == status
                and result['proofs'] == [dict(checkpoint='c-api-ephemeris-discovery', outcome='passed',
                                             assertions=assertions, load_status=0, run_status=status,
                                             expected_rejection=status != 0, public_ABI=True, GUI_executions=0)]
                and 'EphemerisPolicy = ExactlyOneGroup' in (case / 'session/acceptance.script').read_text(),
                'Incomplete exact-count C loader case: ' + name)


def qualify(repo, path, inventory):
    q = load(path)
    known = {row['key']: row for row in inventory['requirements']}
    require(q['contract_kind'] == KIND and q['qualified_keys'] == KEYS
            and q['authority_sha256'] == inventory['sources']
            and all(q[key] is False for key in GATES), 'Changed admission scope')
    for name, digest in q['artifact_sha256'].items():
        require(sha(name) == digest, 'Changed admission artifact: ' + name)
    decision = load(q['interpretation'])
    require(decision['original_rows'] == [known[key] for key in KEYS]
            and decision['attitude_family'] == [known[key] for key in ATTITUDE]
            and decision['FRC_10_family'] == [known[key] for key in PROPAGATE]
            and decision['basis'] == 'assistant judgment delegated by user'
            and decision['attitude']['implementation'] == 'CK/SCLK'
            and decision['ephemeris']['implementation'] == 'ExactlyOneGroup',
            'Changed original wording, fingerprint or interpretation')
    review = load(q['saved_review'])
    require(review['no_new_credit'] and review['current'] == 1097 and review['denominator'] == 1158
            and [row['key'] for row in review['candidate_rows']] == KEYS,
            'Changed the pre-admission review')
    for name, digest in review['artifact_sha256'].items():
        require(sha(name) == digest, 'Changed saved execution evidence: ' + name)
    for archive, hashes in [('reader_source_archive', 'reader_source_sha256'),
                             ('proof_source_archive', 'proof_source_sha256')]:
        with tarfile.open(q[archive]) as bundle:
            require(set(bundle.getnames()) == set(q[hashes]), 'Incomplete source archive')
            for name, digest in q[hashes].items():
                data = bundle.extractfile(name).read()
                require(hashlib.sha256(data).hexdigest() == digest and not data.startswith(b'\x7fELF'),
                        'Changed source archive member: ' + name)
    original, authorized = load(q['protected_input_baseline']), load(q['authorized_input_manifest'])
    sources = q['proof_source_sha256']
    require(len(original) == len(authorized) == 1334 and original.keys() == authorized.keys(),
            'Changed protected input inventory')
    for name, digest in original.items():
        require(sources[name] == digest, 'Lost original protected input: ' + name)
        if digest != authorized[name]:
            require(sources['approved-observer-inputs/' + name] == authorized[name],
                    'Lost authorized observer amendment: ' + name)
    prior = {row['original_source']['key']: row for row in load(q['prior_full_audit'])['rows']}
    require(all(prior[key]['status'] == 'open-original-scope' for key in KEYS)
            and all(prior[key]['status'] == 'verified-source-versioned'
                    for key in ATTITUDE + PROPAGATE if key not in KEYS),
            'Missing original child or previously accepted FRC-10 clause')
    paths = decision['evidence']
    max_error = verify_attitude(paths)
    verify_ephemeris(paths)
    verify_capi(paths)
    audit = dict(status='passed', qualified_keys=KEYS, whole_row_conjunction=True,
                 component=dict(ck_epochs=3, independent_ck_components=36, ck_absolute_bound=2e-8,
                                ck_maximum_error=max_error, attitude_gui_profiles=2,
                                native_ephemeris_cases=12, ephemeris_gui_profiles=2,
                                earlier_native_c_propagate_cases=8, earlier_native_c_ephemeris_cases=13,
                                native_c_exact_group_cases=3),
                 new_requirement_credit=4, newly_resolved_original_hold_keys=[], native_tests_rerun=0,
                 runtime_binary_copies=0, **{key: False for key in GATES})
    return q, KEYS, audit


def build(repo, baseline, path, output):
    seal, inventory, keys = prefix.baseline(repo, baseline)
    _, incoming, audit = qualify(repo, path, inventory)
    known = {kind: {row['key']: row for row in inventory[kind]} for kind in keys}
    require(len(keys['requirements']) == 1097 and len(known['requirements']) == 1158
            and not set(incoming) & keys['requirements'], 'Unexpected preceding scope or duplicate credit')
    keys['requirements'] = common.merge_keys(keys['requirements'], incoming, known['requirements'])
    require(len(keys['requirements']) == 1101, 'Final coverage must stop at exactly 1101')
    coverage = {kind: dict(verified=len(keys[kind]), total=len(known[kind]),
                           percent=round(100 * len(keys[kind]) / len(known[kind]), 1),
                           new_verified=len(incoming) if kind == 'requirements' else 0) for kind in keys}
    remaining = {kind: [row for row in seal['remaining_original_rows'][kind] if row['key'] not in keys[kind]]
                 for kind in keys}
    result = dict(status='passed', scope='Sealed source-versioned attitude and propagation coverage',
                  baseline=str(baseline), baseline_sha256=sha(baseline), authority_sha256=inventory['sources'],
                  coverage=coverage, target_requirements=1101, target_reached=True, requirements_still_needed=0,
                  additions=[dict(key=key, kind='requirements', fingerprint=known['requirements'][key]['fingerprint'],
                                  source_text=known['requirements'][key]['text'], qualification=str(path),
                                  qualification_sha256=sha(path), status='verified-source-versioned')
                             for key in incoming],
                  remaining_counts={kind: len(rows) for kind, rows in remaining.items()},
                  remaining_original_rows=remaining, technical_core_approval=False,
                  full_current_build_qualified=False, full_NASA_qualified=False,
                  limits=['Original denominators, source texts and fingerprints are preserved.',
                          'Four complete requirements are added. All 57 open requirements and the 59 original hold identities retain their prior status.',
                          'The target is met by source-versioned evidence. Broader scientific and platform qualification remain separate.'])
    with Path(output).open('x') as file:
        json.dump(result, file, indent=2)
        file.write('\n')
    return result, audit


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, required=True)
    parser.add_argument('--qualification', type=Path, required=True)
    parser.add_argument('--baseline', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.baseline:
        result, _ = build(args.repo, args.baseline, args.qualification, args.output)
        print(json.dumps(result['coverage']))
    else:
        _, _, result = qualify(args.repo, args.qualification, nasa.inventory(args.repo))
        with args.output.open('x') as file:
            json.dump(result, file, indent=2)
            file.write('\n')
        print(json.dumps(result))
