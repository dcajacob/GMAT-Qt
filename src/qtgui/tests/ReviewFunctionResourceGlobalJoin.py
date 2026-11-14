"""Join all retained resource/owner routes without inferring Global eligibility."""
import argparse
from collections import Counter
import copy
import hashlib
import json
from pathlib import Path
import re
from xml.etree import ElementTree

import NasaGuiCoverage

PRIOR = 'build/nasa-coverage/coverage95-20261104-function-topocentric-reopen'
STAGE = 'build/nasa-coverage/coverage95-20261104-function-resource-global-join'

def check(ok, message):
    if not ok:
        raise ValueError(message)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    repo = args.repo.resolve()
    stage = repo/STAGE
    bindings = {}
    cache = {}

    def read(path, expected=None):
        path = Path(path)
        if not path.is_absolute():
            path = repo/path
        check(path.is_file() and not path.is_symlink(), 'Missing regular input: '+str(path))
        raw = path.read_bytes()
        digest = hashlib.sha256(raw).hexdigest()
        check(expected is None or digest == expected, 'Retained input changed: '+str(path))
        bindings[str(path.relative_to(repo))] = digest
        return raw

    def load(path, expected=None):
        return json.loads(read(path,expected))

    def proof(path, expected=None):
        path = str(path)
        raw = read(path,expected)
        if path not in cache:
            try:
                value = json.loads(raw)
            except json.JSONDecodeError:
                markers = re.findall(r'NASA_CHECK (\{[^\n]+\})',raw.decode())
                check(len(markers) == 1, 'Expected one native proof: '+path)
                value = json.loads(markers[0])
            check(isinstance(value,dict) and value.get('outcome') == 'passed',
                  'Retained native proof did not pass: '+path)
            cache[path] = value
        return cache[path]

    def native(context):
        path = Path(context['proof'])
        value = proof(path,context.get('proof_sha256'))
        root = path.parent
        result = load(root/'result.json')
        check(result['status'] == 'passed' and result['returncode'] == 0,
              'Native result did not pass')
        check(result['requested_visibility'] == context['mode'], 'Native mode changed')
        tests = ElementTree.fromstring(read(root/'ctest.xml')).findall('.//testcase')
        check(len(tests) == 1 and tests[0].get('name') == result['name'] and
              all(tests[0].find(k) is None for k in ['error','failure','skipped']),
              'Native JUnit correspondence changed')
        if context['mode'] == 'visible':
            check(result['trace']['native_exposure_events'] > 0, 'Native visible exposure absent')
        return value

    def parameters(value):
        if isinstance(value,dict):
            if 'factory_type' in value and 'parameter_global' in value:
                yield value
            for item in value.values():
                yield from parameters(item)
        elif isinstance(value,list):
            for item in value:
                yield from parameters(item)

    preparation = load(stage/'preparation-v1.json')
    canonical = load('build/nasa-coverage/coverage95-20261103-matlab-c-interface/coverage-snapshot-v1.json',
        '4d7da4f31ba802900eb7a9eec2fd085dff86cd1ba365b1c23ed5959962bc34e6')
    check(canonical['coverage']['requirements']['verified'] == 1050 and
          canonical['coverage']['requirements']['total'] == 1158 and
          canonical['requirements_still_needed'] == 51 and
          len(canonical['remaining_original_rows']['requirements']) == 108,
          'Original ledger/target changed')
    prior = repo/PRIOR
    read(prior/'evidence-manifest-v1.json',preparation['prior_manifest_sha256'])
    read(prior/'evidence-audit-v1.json',preparation['prior_audit_sha256'])
    replay = load(prior/'frozen-replay-receipt-v1.json')
    check(replay['status'] == 'passed-frozen-auditor-byte-identical' and
          replay['audit_sha256'] == preparation['prior_audit_sha256'], 'Prior replay not bound')
    baseline_path = prior/'topocentric-owned-type-matrix-v1.json'
    baseline = load(baseline_path)
    rows = copy.deepcopy(baseline['rows'])
    check(len(rows) == 566 and len({r['factory_type'] for r in rows}) == 566,
          'Full factory universe changed')
    counts = Counter()
    for row in rows:
        kind = row['factory_type']
        joined = dict(whole_obligation_qualified=False,new_requirement_credit=0,
                      standalone_Global_eligibility='unqualified',contexts=[],gaps=[])
        counts[row['role']] += 1
        if row.get('conjunction',{}).get('main_private_explicit_global_bounded'):
            contexts = row['conjunction']['contexts']
            check(len(contexts) == 2 and {c['mode'] for c in contexts} == {'headless','visible'},
                  'Bounded main/private/Global context missing')
            for context in contexts:
                native(context)
                joined['contexts'].append(dict(proof=context['proof'],
                    proof_sha256=context['proof_sha256'],mode=context['mode'],
                    operation='retained-main-private-explicit-global-bounded'))
            joined['bounded_main_private_explicit_global_observed'] = True
            joined['gaps'].append('Preserve original operation/domain limits;no whole-type or whole-resource admission.')
            counts['bounded_main_private_explicit_global_types'] += 1
        elif row['role'] == 'owner-bound-system-parameter':
            matched = []
            modes = set()
            for field,contexts in row.items():
                if not isinstance(contexts,list) or not contexts or not (
                    'binding_witnesses' in field or field == 'retained_owner_parameter_contexts'):
                    continue
                for context in contexts:
                    if not context.get('proof'):
                        continue
                    value = native(context)
                    records = [p for p in parameters(value) if p['factory_type'] == kind]
                    if not records:
                        continue
                    modes.add(context['mode'])
                    matched += records
                    joined['contexts'].append(dict(field=field,mode=context['mode'],
                        proof=context['proof'],proof_sha256=context['proof_sha256'],
                        actual_parameter_records=len(records),
                        global_flag_true_observed=any(p['parameter_global'] for p in records),
                        local_flag_true_observed=any(p['parameter_local'] for p in records)))
            check(matched and modes == {'headless','visible'}, 'Missing exact Parameter binding: '+kind)
            check(any(p['parameter_global'] and p['owner_global'] and p.get('shared') and
                      p['reference_owner_pointer'] == p['root_owner_pointer'] != '0'
                      for p in matched), 'Actual shared/global owner reference absent: '+kind)
            check(any(p['parameter_local'] and p['owner_local'] for p in matched),
                  'Actual local Parameter/owner route absent: '+kind)
            joined.update(owner_bound_local_and_shared_global_use_observed=True,
                          actual_parameter_records=len(matched),
                          parameter_scope_inferred_from_owner=False,
                          independent_named_parameter_Global_promotion_observed=False)
            joined['gaps'].append('Actual Parameter flags and shared owner references are observed;independent Global promotion of a previously created qualified Parameter name is not inferred from that access.')
            joined['gaps'].append('Preserve every original owner/dependency/domain obligation and bound expression semantics;no bare-creation exclusion.')
            counts['owner_bound_types_with_local_and_actual_global_flags'] += 1
        elif row['role'] == 'mission-command;separate-command-obligation':
            joined['gaps'].append('Retain the separate full E477 command obligation and SaveMission effect conflict;this role is not a resource eligibility exclusion.')
            counts['separate_command_obligations'] += 1
        else:
            fields = ['body_scope_witnesses','new_builtin_scope_witnesses',
                      'function_owned_scope_witnesses','owned_component_scope_witnesses',
                      'owned_type_scope_increment','code500_precision_witnesses',
                      'text_ephemeris_precision_witnesses',
                      'new_global_witnesses','topocentric_reopen_witnesses',
                      'ephemeris_clock_witnesses']
            for field in fields:
                for context in row.get(field,[]):
                    path = context.get('proof')
                    if not path and context.get('run') and context.get('test'):
                        candidates = list((repo/'build/nasa-coverage').glob(
                            '*/'+context['run']+'/'+context['test']+'/proof.json'))
                        matches = [p for p in candidates if p.is_file() and not p.is_symlink() and
                            hashlib.sha256(p.read_bytes()).hexdigest() == context.get('proof_sha256')]
                        check(len(matches) == 1, 'Source-specific proof route ambiguous: '+kind)
                        path = str(matches[0])
                    if not path:
                        continue
                    value = dict(context,proof=path)
                    native(value)
                    joined['contexts'].append(dict(field=field,mode=context['mode'],
                        proof=path,proof_sha256=hashlib.sha256(read(path)).hexdigest(),
                        operation='retained-route;whole-conjunction-unqualified'))
            joined['retained_historical_conjunction_contexts'] = len(row.get('conjunction',{}).get('contexts',[]))
            if row['category_id'] in {110,152}:
                check({c['mode'] for c in joined['contexts']} == {'headless','visible'},
                      'Previously completed owned route omitted: '+kind)
                joined['owned_route_both_modes_observed'] = True
                counts['owned_types_with_both_mode_contexts'] += 1
            joined['gaps'].append('Join actual main/private use,automatic shared identity or explicit Global operations for this exact type;constructor defaults and named-probe failure are not eligibility decisions.')
            counts['other_type_joins_requiring_clause_review'] += 1
        row['resource_global_join_v1'] = joined
    check(counts['bounded_main_private_explicit_global_types'] == 125 and
          counts['owner_bound_types_with_local_and_actual_global_flags'] == 321 and
          counts['separate_command_obligations'] == 53 and
          counts['owned_types_with_both_mode_contexts'] == 36 and
          counts['other_type_joins_requiring_clause_review'] == 67, 'Full join census differs')
    authority = NasaGuiCoverage.inventory(repo)
    check(len(authority['requirements']) == 1158 and len(authority['historical_cases']) == 2367 and
          authority['sources'] == canonical['authority_sha256'], 'Original authority universe changed')
    for name,digest in authority['sources'].items():
        read(name,digest)
    clauses = [dict(key=r['key'],text=r['text'],fingerprint=r['fingerprint'],new_credit=0)
               for r in authority['requirements'] if r['key'] in
               {'FRR!E'+str(n) for n in range(491,501)}]
    check(len(clauses) == 10, 'Original parent/child clauses missing')
    for old,row in zip(baseline['rows'],rows):
        check({k:v for k,v in row.items() if k != 'resource_global_join_v1'} == old,
              'Original role,evidence,eligibility or qualification changed')
    result = dict(status='complete-retained-resource-global-route-join;whole-conjunction-open',
        baseline=str(baseline_path.relative_to(repo)),baseline_sha256=hashlib.sha256(read(baseline_path)).hexdigest(),
        counts=dict(counts),exact_parent_and_children=clauses,rows=rows,
        retained_native_proofs_checked=len(cache),artifact_sha256=bindings,
        canonical_verified=1050,original_total=1158,target=1101,still_needed=51,
        new_native_runs=0,new_requirement_credit=0,whole_row_qualified=False,
        scientific_approval=False,technical_approval=False,full_current_build_qualified=False,
        full_NASA_qualified=False,
        next_action='Review independent named Parameter Global promotion and the67 remaining exact-type joins using current source plus retained scripts;execute only uncovered operations. Reuse all321 local/shared-owner and36 owned-type witnesses. Never infer eligibility from constructor flags or owner scope.')
    check(not args.output.exists(), 'Never overwrite a review')
    args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:result[k] for k in ['status','counts','retained_native_proofs_checked','new_native_runs','new_requirement_credit','next_action']}))

if __name__ == '__main__':
    main()
