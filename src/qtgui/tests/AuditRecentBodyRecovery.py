"""Audit the four explicitly deferred Pluto/Uranus recovery rows.

New reviewed mappings bind repaired executions, never rewrite old mappings or
substitute the old assertion alias for the actual executed source. This adapter
is intentionally limited to the four audit_recent_passes feasibility candidates.
"""
import argparse
from collections import Counter
import copy
import decimal
import importlib.util
import json
from pathlib import Path
import re

import AcceptanceSnapshot as snapshot
import AuditAcceptanceRecovery as prior

VERSION = 'guarded-body-local-state-repairs-Uranus-Pluto'
ROWS = {'H2012:R843': ('Pluto', 'apply'), 'H2012:R844': ('Pluto', 'controls'),
        'H2012:R960': ('Uranus', 'apply'), 'H2012:R961': ('Uranus', 'controls')}
SOURCE = 'src/qtgui/tests/NasaBodyControlTests.cpp'
MAIN = 'src/qtgui/MainWindow.cpp'
sha = lambda p: prior.digest(prior.payload(p))
load = lambda p: json.loads(prior.payload(p))
need = prior.need


def requires(source):
    """Compare complete original require expressions, not a count of markers."""
    result = []
    for match in re.finditer(r'\brequire\s*\(', source):
        index = start = match.end()
        depth, quote, escape = 1, None, False
        while depth:
            char = source[index]
            index += 1
            if quote:
                if escape: escape = False
                elif char == '\\': escape = True
                elif char == quote: quote = None
            elif char in ('"', "'"): quote = char
            elif char == '(': depth += 1
            elif char == ')': depth -= 1
        result.append(source[start:index-1])
    return Counter(result)


def visible_controls(oracle, events, case, args, old_text, new_text):
    """Check added filename entry and retain the exact original trace oracle."""
    need('paste(filename,path)' not in old_text and new_text.count('paste(filename,path)') == 1,
         'Unexpected repaired picker input source')
    added = [i for i,e in enumerate(events) if e.get('event') == 'viewing_boundary'
             and e.get('name') == 'fileNameEdit']
    visits = [v for v in args[6] if 'path' in v]
    need(len(added) == len(visits) == 4, 'Missing added picker filename boundaries')
    excluded = set()
    for index in added:
        event, check = events[index], events[index+1]
        need(event['class'] == 'QLineEdit' and '/QFileDialog:' in event['key']
             and event['visible'] is True and check['event'] == 'checkpoint'
             and check['exposed'] is True and check['platform'] == 'xcb'
             and check['key'] == event['key'], 'Added filename entry was not exposed')
        excluded.update((index,index+1))
    original_events = [e for i,e in enumerate(events) if i not in excluded]
    original = oracle.visible(original_events,case,args[3],args[4],args[8],args[9])
    return dict(original_visible_boundaries=original['visible_boundaries'],
                added_filename_boundaries=len(added),
                visible_boundaries=original['visible_boundaries']+len(added),
                paced_visible_boundaries=original['paced_visible_boundaries']+len(added),
                unpaced_visible_boundaries=0)


def circular_truth(raw):
    """Recompute literal mu64/radius1000 truth at reported time at 80 digits."""
    with decimal.localcontext() as context:
        context.prec = 80
        D = decimal.Decimal
        omega, speed = (D(64)/D(1000)**3).sqrt(), (D(64)/D(1000)).sqrt()
        lines = raw.strip().splitlines()
        need(len(lines) == 4 and lines[-1] == '42', 'Wrong circle/continuation schema')
        position, velocity = [], []
        for line in lines[:3]:
            values = list(map(D,line.split(',')))
            need(len(values) == 9 and values[7:] == [D(713),D(41)], 'Circle mass/sentinel changed')
            x = omega*values[0]
            cosine = cosine_term = D(1)
            sine = sine_term = x
            for index in range(1,55):
                cosine_term *= -x*x/D((2*index-1)*(2*index))
                sine_term *= -x*x/D((2*index)*(2*index+1))
                cosine += cosine_term
                sine += sine_term
            expected = [1000*cosine,1000*sine,D(0),-speed*sine,speed*cosine,D(0)]
            errors = [abs(values[i+1]-expected[i]) for i in range(6)]
            need(all(e < D('2e-7') for e in errors[:3])
                 and all(e < D('2e-9') for e in errors[3:]), 'Independent circle result outside original bounds')
            position.extend(errors[:3]);velocity.extend(errors[3:])
        return dict(max_position_error_km=str(max(position)),max_velocity_error_km_s=str(max(velocity)),
                    position_bound_km='2e-7',velocity_bound_km_s='2e-9',decimal_precision=80,
                    shared_ephemeris_is_not_independent_ephemeris_truth=True)


def audit(repo, plan):
    repo = Path(repo)
    ledger = prior.checked_json(repo / plan['ledger'], plan['ledger_sha256'])
    baseline = prior.checked_json(repo / plan['baseline_report'], plan['baseline_report_sha256'])
    union = prior.checked_json(repo / plan['baseline_union'], plan['baseline_union_sha256'])
    feasibility = prior.checked_json(plan['feasibility'], plan['feasibility_sha256'])
    candidates = {r['key']: r for r in feasibility['rows']['historical_cases']}
    need(set(plan['required_cohorts']) == {'visible', 'unattended'}, 'Both modes are required')
    need(plan['version'] == VERSION, 'Wrong recent collection')
    need({d['key'] for d in plan['decisions']} == set(ROWS)
         and len(plan['decisions']) == 4, 'Recent recovery is limited to exactly four rows')
    need(all(candidates[k]['decision'] == 'audit_recent_passes' for k in ROWS),
         'Recent mapping cannot replace an older candidate mapping')
    version = next(v for v in ledger['versions'] if v['label'] == VERSION)
    summary = prior.checked_json(version['audit'], version['audit_sha256'])
    original = prior.checked_json(version['mapping_manifest'], version['mapping_manifest_sha256'])
    prior.checked_json(version['report'], version['report_sha256'])
    prior.checked_json(version['receipt'], version['receipt_sha256'])
    need(summary['qualification_receipt'] == version['receipt']
         and summary['qualification_receipt_sha256'] == version['receipt_sha256'],
         'Repair summary does not bind the original visible qualification')
    original_rows = {r['key']: r for r in original['reviews']}
    mapping = prior.checked_json(plan['reviewed_mapping'], plan['reviewed_mapping_sha256'])
    need(mapping['sources'] == baseline['sources'], 'NASA authority baseline changed')
    mapped = {r['key']: r for r in mapping['reviews']}
    need(set(mapped) == set(ROWS) and len(mapping['reviews']) == 4, 'Unexpected recent source rows')
    bound = {}
    for path, value in plan['input_sha256'].items():
        need(sha(path) == value, 'Recent review input changed: ' + path)
        bound[path] = value
    repair = repo / 'build/nasa-coverage/body-local-state-review'
    need(all(str(repair/name) in bound for name in ('case-contexts.json','raw-artifact-bindings.json')),
         'Original repair context and raw indices must be pinned explicitly')
    contexts = load(repair / 'case-contexts.json')
    raw = load(repair / 'raw-artifact-bindings.json')
    appearance_version = next(v for v in ledger['versions']
                              if v['label'] == plan['appearance_version'])
    appearance_audit = prior.checked_json(appearance_version['audit'], appearance_version['audit_sha256'])
    appearance_index = prior.checked_json(appearance_audit['combined_claim_evidence'],
                                         appearance_audit['combined_claim_evidence_sha256'])
    oracles = {}
    for name, folder in [('controls', 'body-clock-review'), ('appearance', 'body-appearance-review')]:
        path = repo / 'build/nasa-coverage' / folder / 'oracles.py'
        authority = next(v for v in ledger['versions'] if Path(v['audit']) == path.parent/'validation-summary.json')
        old = prior.checked_json(authority['audit'], authority['audit_sha256'])
        need(sha(path) == old['oracles_sha256'], 'Original body oracle changed')
        module = importlib.util.spec_from_file_location('original_recent_'+name, path)
        oracle = importlib.util.module_from_spec(module)
        module.loader.exec_module(oracle)
        oracles[name] = oracle
        bound[str(path)] = sha(path)
    measured, negative, observations = [], [], []
    cache = {}
    for mode in ('unattended', 'visible'):
        rows = []
        for key, (body, operation) in ROWS.items():
            row = mapped[key]
            control_key = 'H2012:R844' if body == 'Pluto' else 'H2012:R961'
            need(row['fingerprint'] == candidates[key]['fingerprint'], 'Source row fingerprint changed')
            claims = row['evidence']
            need(len(claims) == 2 and {c['test'] for c in claims} ==
                 {'QtGui.NativeNasaBodyControl'+body, 'QtGui.NativeNasaBodyAppearance'+body},
                 'Complete controls and appearance conjunction is required')
            checks = []
            for claim in claims:
                kind = 'controls' if 'BodyControl' in claim['test'] else 'appearance'
                if kind == 'controls':
                    entries = [c for c in contexts if c['stage'] == mode and c['test'] == claim['test']]
                    need(len(entries) == 1, 'Missing original repaired control context')
                    context = entries[0]
                    need(raw.get(context['receipt']) == context['receipt_sha256'], 'Repair raw receipt binding changed')
                    template = next(c for c in original_rows[control_key]['evidence'] if c['test'] == claim['test'])
                else:
                    entries = [c for c in appearance_index if c['key'] == control_key
                               and c['mode'] == mode and c['claim']['test'] == claim['test']]
                    need(len(entries) == 1, 'Missing original appearance context')
                    context, template = entries[0]['receipt'], entries[0]['claim']
                for field in ('anchor', 'checkpoint', 'expected', 'facets'):
                    need(claim[field] == template[field], 'Original body obligation altered: ' + field)
                if kind == 'appearance':
                    need(claim['source'] == template['source'] and claim['source_sha256'] == template['source_sha256'],
                         'Original appearance assertion source changed')
                need(claim['scope'] and row['rationale'], 'Missing explicit operation scope')
                need(all(claim['reference_sources'].get(p) == h
                         for p, h in template['reference_sources'].items()), 'Original reference guard dropped')
                receipt = prior.checked_json(context['receipt'], context['receipt_sha256'])
                manifest = load(receipt['snapshot_manifest'])
                identity = dict(manifest)
                sid = identity.pop('snapshot_id')
                need(snapshot.fingerprint(identity) == sid == receipt['snapshot_id'], 'Snapshot identity changed')
                need(receipt['snapshot_unchanged'] and receipt.get('finished_utc'), 'Incomplete original execution')
                need(receipt['mode'] == ('visible' if mode == 'visible' else 'headless'), 'Wrong actual execution mode')
                frozen = Path(manifest['root'])
                need(prior.nasa.inventory(frozen)['sources'] == mapping['sources'], 'Frozen NASA authority differs')
                result = next(r for r in receipt['results'] if r['name'] == claim['test'])
                need(claim['reference_sources'].get(MAIN) == receipt['source_hashes'].get(MAIN),
                     'Actual executed GUI production reference is required')
                check = prior.verify_claim(frozen, manifest, receipt, result, claim)
                if mode == 'visible': need(check['native_exposure_events'] > 0, 'Missing native exposure')
                token = (claim['test'], mode)
                if token not in cache:
                    folder = Path(result['log']).parent
                    old_raw = raw if kind == 'controls' else appearance_audit['raw_artifact_sha256']
                    for artifact in folder.iterdir():
                        if artifact.is_file() and str(artifact) in old_raw:
                            need(sha(artifact) == old_raw[str(artifact)], 'Original raw body artifact changed')
                            bound[str(artifact)] = sha(artifact)
                    matrix = load(frozen/'src/qtgui/tests/NasaBodyControlCases.json')
                    case = next(c for c in matrix['cases'] if c['key'] == body)
                    events = [json.loads(line) for p in sorted(folder.glob('trace*.jsonl'))
                              for line in prior.payload(p).decode().splitlines()]
                    oracle = oracles[kind]
                    if kind == 'controls':
                        need(claim['source'] == SOURCE, 'Repaired source must be the registered case input')
                        need(sha(template['source']) == template['source_sha256'], 'Original assertion alias changed')
                        old_text = prior.payload(template['source']).decode()
                        new_text = prior.payload(frozen/SOURCE).decode()
                        need(not (requires(old_text)-requires(new_text)), 'Original assertion expression weakened or removed')
                        proof = load(folder/'proof.json')
                        args = [proof, case, matrix]+[load(folder/p) for p in
                            ['native-capabilities.json','censuses.json','probes.json','visits.json',
                             'rejections.json','readonly-routes.json','changes.json','native-applied.json',
                             'native-initial.json','native-final.json']]+[
                            prior.payload(folder/'initial.script').decode(),prior.payload(folder/'mission.script').decode(),
                            prior.payload(folder/'values.csv').decode(),load(folder/'native-frame.json')]
                        metrics = oracle.check(*args)
                        independent = circular_truth(args[-2])
                        need(all(c['apply_enabled'] == c['dirty'] for c in args[4]), 'Incomplete Apply status matrix')
                        if mode == 'visible':
                            metrics.update(visible_controls(oracle,events,case,args,old_text,new_text))
                        for mutation in ('apply-status','numeric-result'):
                            bad = copy.deepcopy(args)
                            if mutation == 'apply-status':bad[4][0]['apply_enabled'] = not bad[4][0]['apply_enabled']
                            else:bad[0]['samples'][0]['actual'][1] += 1
                            try:oracle.check(*bad)
                            except (AssertionError,ValueError,KeyError):negative.append(dict(body=body,mode=mode,mutation=mutation))
                            else:raise ValueError('Corrupt recent control payload accepted')
                    else:
                        independent = None
                        metrics = oracle.check(folder,case)
                        if mode == 'visible':metrics.update(oracle.visible(events))
                        invalid = load(folder/'invalid.json');invalid.pop()
                        try:oracle.check(folder,case,invalid=invalid)
                        except (AssertionError,ValueError,KeyError):negative.append(dict(body=body,mode=mode,mutation='missing-appearance-rejection'))
                        else:raise ValueError('Incomplete appearance rejection matrix accepted')
                    measured.append(dict(body=body,kind=kind,mode=mode,metrics=dict(metrics),
                                         independent_circle=independent))
                    cache[token] = True
                checks.append(dict(check,receipt=context['receipt'],receipt_sha256=context['receipt_sha256'],kind=kind))
            rows.append(dict(key=key,checks=checks))
        observations.append(dict(cohort=mode,rows=rows,explicitly_composed_source_versioned=True,
                                 mapping=plan['reviewed_mapping'],mapping_sha256=plan['reviewed_mapping_sha256']))
    need(len(measured) == 8 and len(negative) == 12, 'Incomplete original-payload audit')
    need(all(d['decision'] == 'accept_versioned_scope' and d.get('rationale') and d.get('remaining_limits')
             for d in plan['decisions']), 'Missing four explicit substantive decisions')
    need(not set(ROWS) & {r['key'] for r in union['historical_cases']['rows']}, 'Already credited body row')
    need(all(sha(p) == h for p,h in bound.items()), 'Inputs changed during audit')
    return dict(version=VERSION,decisions=plan['decisions'],cohorts=observations,
                accepted_source_versioned_scope_keys=dict(requirements=[],historical_cases=list(ROWS)),
                accepted_count=4,pending_count=0,new_test_executions=0,
                full_current_build_qualified=False,full_NASA_qualified=False,
                audit_type='recent-body-recovery',measured_contexts=measured,
                negative_mutations_rejected=negative,input_sha256=bound,
                limit='Four bounded current-operation equivalents only. Actual repaired control sources and '
                      'older complete appearance sources form an explicit versioned conjunction. '
                      'Private2012 procedures, independent ephemerides/full science, current-binary equivalence '
                      'and technical core approval remain unqualified.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3])
    parser.add_argument('--plan',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args = parser.parse_args()
    result = audit(args.repo,load(args.plan))
    result.update(plan_path=str(args.plan.resolve()),plan_sha256=sha(args.plan),
                  auditor_sha256=sha(Path(__file__)),original_claim_verifier_sha256=sha(Path(prior.__file__)))
    args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(dict(accepted=4,original_contexts=8,negative_mutations_rejected=12,new_executions=0)))


if __name__ == '__main__':main()
