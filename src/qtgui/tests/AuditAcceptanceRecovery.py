"""Recheck retained acceptance proofs without rerunning or rewriting a receipt.

An explicit semantic review is mandatory. This tool verifies its cited old
mapping/receipt/source/log/JUnit identities; it cannot decide NASA applicability.
Only complete rows from one mapping and every declared cohort qualify. Explicit
bindings may retain complementary tests from that mapping's original receipts;
they never drop a mapped claim or imply a single build. Current-build qualification
and source-to-binary provenance are deliberately not inferred.
"""
import argparse
import copy
import gzip
import hashlib
import json
from pathlib import Path
import tempfile
import xml.etree.ElementTree as ET

import AcceptanceSnapshot as snapshot
import NasaGuiCoverage as nasa


def payload(path):
    path = Path(path)
    if path.is_file():
        return path.read_bytes()
    compressed = Path(str(path) + '.gz')
    if compressed.is_file():
        return gzip.decompress(compressed.read_bytes())
    raise ValueError('Missing retained input: ' + str(path))


def digest(data):
    return hashlib.sha256(data).hexdigest()


def checked_json(path, expected):
    data = payload(path)
    if digest(data) != expected:
        raise ValueError('Retained identity mismatch: ' + str(path))
    return json.loads(data)


def need(condition, message):
    if not condition:
        raise ValueError(message)


def supplemented_receipt(original, wrapper, bindings, frozen, manifest):
    """Allow only formerly omitted, actually frozen command input identities."""
    need(set(wrapper) - set(original) == {'original_receipt', 'original_receipt_sha256',
                                         'review_only_snapshot_input_bindings'},
         'Unexpected review-receipt fields')
    need(all(wrapper.get(k) == value for k, value in original.items()
             if k != 'source_hashes'), 'Review receipt rewrites execution data')
    need(wrapper['review_only_snapshot_input_bindings'] == bindings,
         'Review bindings differ from audited declaration')
    expected = dict(original['source_hashes'])
    results = {r['name']: r for r in original['results']}
    for source, binding in bindings.items():
        need(not Path(source).is_absolute()
             and (source not in expected or expected[source] == binding['sha256']),
             'Supplement changes an already recorded or absolute source')
        path = frozen / source
        need(manifest['artifact_hashes'].get(source) == binding['sha256']
             and digest(payload(path)) == binding['sha256'],
             'Supplement is not an unchanged frozen artifact')
        need(str(path) in results[binding['test']]['registered_command'],
             'Supplement is not an exact executed command input')
        expected[source] = binding['sha256']
    need(wrapper['source_hashes'] == expected, 'Unreviewed source-hash additions')
    return wrapper


def verify_claim(frozen, manifest, receipt, result, claim):
    """Bind source and references before invoking the unchanged proof verifier."""
    need(result.get('name') == claim['test'] and result.get('status') == 'passed'
         and result.get('returncode') == 0, 'Missing successful exact test')
    need(result.get('snapshot_id') == receipt['snapshot_id'] == manifest['snapshot_id'],
         'Test/receipt/snapshot identity differs')
    cases = [c for c in manifest['cases'] if c['name'] == claim['test']]
    need(len(cases) == 1, 'Missing/duplicate registered case')
    case = cases[0]
    need(snapshot.case_fingerprint(case) == result.get('case_fingerprint'),
         'Registered case fingerprint differs')
    need(case['command'] == result.get('registered_command'), 'Registered command differs')
    binaries = {p: h for p, h in receipt.get('binary_hashes', {}).items()
                if p in result['registered_command']}
    need(bool(binaries), 'No recorded executable identity')
    for path, value in binaries.items():
        relative = str(Path(path).relative_to(frozen))
        need(manifest['artifact_hashes'].get(relative) == value,
             'Recorded executable identity disagrees with snapshot')
    need(claim['source'] in result.get('sources', [])
         and claim['source'] in case.get('sources', []), 'Assertion source is not a case input')
    sources = {claim['source']: claim['source_sha256'], **claim.get('reference_sources', {})}
    for path, value in sources.items():
        need(receipt['source_hashes'].get(path) == value,
             'Source/reference was not recorded at execution: ' + path)
        need(digest(payload(frozen / path)) == value, 'Source/reference changed: ' + path)
        # Absolute preserved reference aliases are allowed only if recorded in
        # the receipt; relative inputs must also belong to the frozen manifest.
        if not Path(path).is_absolute():
            need(manifest['artifact_hashes'].get(path) == value,
                 'Source/reference absent from snapshot manifest: ' + path)
    source_text = payload(frozen / claim['source']).decode('utf-8')
    need(claim['anchor'] in source_text and claim.get('scope') and claim.get('facets'),
         'Missing source assertion, explicit scope or facets')
    for field in ('log', 'junit'):
        need(digest(payload(result[field])) == result.get(field + '_sha256'),
             'Changed test ' + field)
    try:
        junit = ET.fromstring(payload(result['junit']))
        cases = junit.findall('.//testcase')
        need(len(cases) == 1 and cases[0].get('name') == claim['test']
             and cases[0].get('status') in (None, 'run', 'passed', 'pass'),
             'JUnit does not identify one successful claimed test')
        need(not any(int(s.get(k, '0')) != 0 for s in junit.iter('testsuite')
                     for k in ('failures', 'errors', 'skipped', 'disabled')),
             'JUnit has unsuccessful outcomes')
        need(not any(junit.find('.//' + tag) is not None
                     for tag in ('failure', 'error', 'skipped')),
             'JUnit has unsuccessful outcomes')
    except ET.ParseError as error:
        raise ValueError('Malformed JUnit') from error
    # Materialize compressed text only when necessary; original artifacts stay
    # untouched. This is not a new execution or a reconstructed numeric result.
    with tempfile.TemporaryDirectory(prefix='gmat-proof-audit-') as directory:
        local = copy.deepcopy(result)
        for field in ('log', 'junit'):
            path = Path(result[field])
            if not path.is_file():
                target = Path(directory) / field
                target.write_bytes(payload(path))
                local[field] = str(target)
        if 'legacy_assertions' in claim:
            need(nasa.legacy_assertions_verified(frozen, local, claim),
                 'Legacy assertions/unique terminal/JUnit check failed')
        else:
            records = [p for p in nasa.proof_records(local)
                       if p.get('checkpoint') == claim['checkpoint']
                       and set(claim['facets']) <= set(p.get('facets', []))
                       and all(p.get(k) == v for k, v in claim.get('expected', {}).items())]
            need(len(records) == 1, 'Missing or ambiguous structured proof')
    return dict(test=claim['test'], source=claim['source'],
                source_sha256=claim['source_sha256'],
                log=result['log'], log_sha256=result['log_sha256'],
                junit=result['junit'], junit_sha256=result['junit_sha256'],
                snapshot_id=manifest['snapshot_id'], executable_identities=binaries,
                native_exposure_events=result.get('trace', {}).get('native_exposure_events', 0))


def reviewed_composition(repo, plan, version, ledger, mapping, cohorts, candidates):
    """Restore exact earlier reference bindings, retaining every base clause.

    A replacement must be an entire clause from another original reviewed
    mapping. Assertion source/hash, anchors, expected values, facets and scope
    must equal the base clause; only its reference-version dictionary may differ.
    Every supporting receipt still comes from an original ledger collection.
    """
    if 'reviewed_composition' not in plan:
        return mapping, cohorts
    spec=plan['reviewed_composition']
    reviewed=checked_json(spec['mapping'],spec['mapping_sha256'])
    base={r['key']:r for r in mapping['reviews']}
    rows={r['key']:r for r in reviewed['reviews']}
    decisions={d['key'] for d in plan['decisions']}
    need(set(rows)==decisions and len(rows)==len(reviewed['reviews'])
         and set(spec['origins'])==decisions,'Composition omits or adds reviewed source rows')
    need(reviewed['sources']==mapping['sources'],'Composition changes NASA authorities')
    need(ledger is not None,'Reviewed composition requires the original ledger collections')
    versions={v['label']:v for v in ledger['versions']}
    origins={}
    for key,row in rows.items():
        need(key in candidates and key in base and row['fingerprint']==base[key]['fingerprint']
             ==candidates[key]['fingerprint'],'Composition changes source row identity')
        clauses=base[key].get('evidence',[])
        need(len(row.get('evidence',[]))==len(clauses)==len(spec['origins'][key])
             and clauses,'Composition drops an original required clause')
        for index,(claim,original,label) in enumerate(zip(row['evidence'],clauses,spec['origins'][key])):
            need(label in versions,'Unknown original clause collection')
            if label not in origins:
                item=versions[label]
                origin_map=checked_json(item['mapping_manifest'],item['mapping_manifest_sha256'])
                origin_audit=checked_json(item['audit'],item['audit_sha256'])
                need(origin_map.get('sources')==mapping['sources'],
                     'Original clause uses different NASA authorities')
                for field in ('report','receipt'):
                    checked_json(item[field],item[field+'_sha256'])
                origins[label]=(origin_map,origin_audit)
            origin_map,origin_audit=origins[label]
            choices=[r for r in origin_map['reviews'] if r['key']==key]
            need(len(choices)==1 and choices[0]['fingerprint']==row['fingerprint'],
                 'Original clause lacks the exact source row')
            need(claim in choices[0].get('evidence',[]),'Composition invents or rehashes an original clause')
            original_paths=set(original.get('reference_sources',{}))|set(original.get('post_execution_reference_sources',{}))
            restored_paths=set(claim.get('reference_sources',{}))|set(claim.get('post_execution_reference_sources',{}))
            need(original_paths<=restored_paths,'Composition drops an original reference guard')
            need(all(claim.get('post_execution_reference_sources',{}).get(p)==h
                     for p,h in original.get('post_execution_reference_sources',{}).items()),
                 'Composition promotes or changes a normative-only reference')
            # Normative documents originally marked post-execution must keep
            # that status; they are not invented runtime source observations.
            for path,value in claim.get('post_execution_reference_sources',{}).items():
                need(original.get('reference_sources',{}).get(path)==value
                     or original.get('post_execution_reference_sources',{}).get(path)==value,
                     'Composition changes a normative document reference')
            without_refs=lambda c:{k:v for k,v in c.items() if k not in
                                  ('reference_sources','post_execution_reference_sources')}
            need(without_refs(claim)==without_refs(original),
                 'Composition changes an assertion, expected value, facet or scope')
    extra=set(origins)-{version['label']}
    need(extra==set(spec['collections']),'Composition receipt collection list differs from clause origins')
    for label in extra:
        for name,context in origins[label][1].get('source_bound_evidence',{}).items():
            alias='version:'+label+':'+name
            need(alias not in cohorts and 'receipt' in context and 'receipt_sha256' in context,
                 'Invalid original cross-version receipt context')
            cohorts[alias]=context
    return reviewed,cohorts


def original_receipt_collections(repo, plan, ledger, cohorts):
    """Admit exact receipts from explicitly named original ledger collections.

    This changes no mapped claim, reference version, assertion or witness rule.
    Each additional collection and receipt remains bound by the original audit;
    the caller still verifies every claim against the actual frozen execution.
    """
    labels=plan.get('receipt_collections',[])
    need(isinstance(labels,list) and all(isinstance(label,str) for label in labels)
         and len(set(labels))==len(labels),'Invalid/duplicate receipt collections')
    if not labels:return cohorts
    need(ledger is not None,'Receipt collections require the original ledger')
    versions={v['label']:v for v in ledger['versions']}
    result=dict(cohorts)
    for label in labels:
        need(label in versions,'Unknown original receipt collection')
        version=versions[label]
        for field in ('audit','report','receipt','mapping_manifest'):
            checked_json(repo/version[field],version[field+'_sha256'])
        old=checked_json(repo/version['audit'],version['audit_sha256'])
        contexts=old.get('source_bound_evidence',{})
        need(isinstance(contexts,dict) and contexts,'Collection has no original receipt contexts')
        for name,context in contexts.items():
            alias='collection:'+label+':'+name
            need(alias not in result and isinstance(context,dict)
                 and 'receipt' in context and 'receipt_sha256' in context,
                 'Invalid/duplicate original collection context')
            # Pin the actual receipt now as well as at point of claim use.
            checked_json(context['receipt'],context['receipt_sha256'])
            result[alias]=context
    return result


def composed_observations(repo, plan, version, old, baseline, candidates, ledger=None):
    """Bind every original mapped claim to an explicit original execution.

    A logical visible/unattended cohort can contain complementary executions
    from different preserved builds and explicitly reviewed original mappings.
    This is a source-versioned conjunction, never current-binary equivalence.
    """
    groups=plan['claim_bindings']
    need(set(groups)==set(plan['required_cohorts'])=={'visible','unattended'},
         'Composed recovery needs both explicit execution modes')
    mapping=checked_json(repo/version['mapping_manifest'],version['mapping_manifest_sha256'])
    authorities=mapping.get('sources')
    if authorities is None:
        need(plan.get('mapping_authority_from_frozen_inventories') is True,
             'Mapping without source authorities needs explicit frozen-inventory review')
        # Every row fingerprint and every bound execution inventory below must
        # still match this exact baseline. This is not an invented mapping hash.
        authorities=baseline['sources']
    need(authorities==baseline['sources'],'NASA source baseline differs')
    cohorts=dict(old.get('source_bound_evidence',{}))
    mapping,cohorts=reviewed_composition(repo,plan,version,ledger,mapping,cohorts,candidates)
    cohorts=original_receipt_collections(repo,plan,ledger,cohorts)
    composition=plan.get('reviewed_composition')
    mapping_path=composition['mapping'] if composition else version['mapping_manifest']
    mapping_sha256=composition['mapping_sha256'] if composition else version['mapping_manifest_sha256']
    for name,path in plan.get('raw_receipts',{}).items():
        need(name not in cohorts and name.startswith('raw:') and path.endswith('/results.json'),
             'Invalid/duplicate explicit original receipt alias')
        need(path in old.get('raw_artifact_sha256',{}),
             'Receipt was not hashed by the original version audit')
        cohorts[name]=dict(receipt=path,receipt_sha256=old['raw_artifact_sha256'][path])
    if plan.get('receipt_index'):
        need(plan['receipt_index']=='combined_claim_evidence', 'Unsupported original receipt index')
        field=plan['receipt_index']
        entries=checked_json(old[field],old[field+'_sha256'])
        need(isinstance(entries,list),'Invalid original receipt index')
        for index,entry in enumerate(entries):
            name='index:'+str(index)
            need(name not in cohorts,'Duplicate original indexed receipt alias')
            cohorts[name]=dict(entry['receipt'],indexed_key=entry['key'],indexed_claim=entry['claim'])
    mapped={r['key']:r for r in mapping['reviews']}
    decisions={d['key']:d for d in plan['decisions']}
    rows=[]
    for decision in plan['decisions']:
        key=decision['key']
        need(key in candidates and key in mapped,'Unknown/unmapped candidate: '+key)
        row=mapped[key]
        need(row['fingerprint']==candidates[key]['fingerprint'],'Source row identity differs')
        need(decision.get('rationale') and decision.get('remaining_limits') is not None,
             'Missing substantive review decision')
        need(bool(row.get('evidence')),'No mapped evidence for '+key)
        rows.append(row)
    tests={c['test'] for r in rows for c in r['evidence']}
    contexts={}
    observations=[]
    for group,bindings in groups.items():
        need(set(bindings)==tests,'Composed binding omits or adds a mandatory mapped test')
        records=[]
        for row in rows:
            checks=[]
            for claim in row['evidence']:
                name=bindings[claim['test']]
                need(name in cohorts,'Execution is outside original version collection')
                if name not in contexts:
                    cohort=cohorts[name]
                    receipt=checked_json(cohort['receipt'],cohort['receipt_sha256'])
                    manifest=json.loads(payload(receipt['snapshot_manifest']))
                    identity=dict(manifest);manifest_id=identity.pop('snapshot_id')
                    need(snapshot.fingerprint(identity)==manifest_id==receipt['snapshot_id'],
                         'Snapshot manifest identity changed')
                    need(receipt.get('finished_utc') and receipt.get('snapshot_unchanged'),
                         'Execution did not finish with unchanged snapshot')
                    frozen=Path(manifest['root'])
                    need(nasa.inventory(frozen)['sources']==authorities,'NASA source baseline differs')
                    contexts[name]=(cohort,receipt,manifest,frozen)
                cohort,receipt,manifest,frozen=contexts[name]
                need(receipt.get('mode')==('visible' if group=='visible' else 'headless'),
                     'Execution mode does not match declared composed cohort')
                results=[r for r in receipt['results'] if r['name']==claim['test']]
                need(len(results)==1,'Missing/duplicate exact test in bound execution')
                try:
                    if 'indexed_key' in cohort:
                        need(cohort['indexed_key']==row['key'] and cohort['indexed_claim']==claim,
                             'Indexed receipt does not bind this complete original row claim')
                    check=verify_claim(frozen,manifest,receipt,results[0],claim)
                except ValueError as error:
                    # A reviewed rejection can retain the failed binding as a
                    # diagnostic. It cannot supply a passing check or row credit.
                    if decisions[row['key']].get('decision')!='missing_obligation':raise
                    check=dict(test=claim['test'],verified=False,rejection=str(error),
                               expected_source_sha256=claim['source_sha256'],
                               recorded_source_sha256=receipt['source_hashes'].get(claim['source']),
                               snapshot_id=manifest['snapshot_id'],
                               native_exposure_events=results[0].get('trace',{}).get('native_exposure_events',0))
                if group=='visible':need(check['native_exposure_events']>0,'Missing native exposure in visible cohort')
                checks.append(dict(check,original_cohort=name,receipt=cohort['receipt'],
                                   receipt_sha256=cohort['receipt_sha256']))
            records.append(dict(key=row['key'],checks=checks))
        observations.append(dict(cohort=group,rows=records,mapping=mapping_path,
                                 mapping_sha256=mapping_sha256,
                                 explicitly_composed_source_versioned=True,
                                 limit='Complete original mapped conjunction across explicit original receipts; not one current binary.'))
    return observations


def audit(repo, plan):
    repo = Path(repo)
    ledger = checked_json(repo / plan['ledger'], plan['ledger_sha256'])
    baseline = checked_json(repo / plan['baseline_report'], plan['baseline_report_sha256'])
    union = checked_json(repo / plan['baseline_union'], plan['baseline_union_sha256'])
    candidates = {r['key']: r for kind in ('requirements', 'historical_cases')
                  for r in baseline[kind]}
    decisions = plan['decisions']
    need(len({d['key'] for d in decisions}) == len(decisions), 'Duplicate review decision')
    version = next(v for v in ledger['versions'] if v['label'] == plan['version'])
    old = checked_json(version['audit'], version['audit_sha256'])
    for field in ('report', 'receipt', 'mapping_manifest'):
        checked_json(repo / version[field], version[field + '_sha256'])
    composed='claim_bindings' in plan
    observations = composed_observations(repo,plan,version,old,baseline,candidates,ledger) if composed else []
    for cohort_name, cohort in ({} if composed else old['source_bound_evidence']).items():
        if cohort_name not in plan['required_cohorts']:
            continue
        receipt = checked_json(cohort['receipt'], cohort['receipt_sha256'])
        mapping = checked_json(cohort['mapping_manifest'], cohort['mapping_manifest_sha256'])
        manifest = json.loads(payload(receipt['snapshot_manifest']))
        manifest_copy = dict(manifest)
        manifest_id = manifest_copy.pop('snapshot_id')
        need(snapshot.fingerprint(manifest_copy) == manifest_id == receipt['snapshot_id'],
             'Snapshot manifest identity changed')
        need(receipt.get('finished_utc') and receipt.get('snapshot_unchanged'),
             'Execution did not finish with unchanged snapshot')
        frozen = Path(manifest['root'])
        supplement = None
        if cohort.get('review_receipt'):
            wrapper = checked_json(cohort['review_receipt'], cohort['review_receipt_sha256'])
            need(wrapper['original_receipt'] == cohort['receipt']
                 and wrapper['original_receipt_sha256'] == cohort['receipt_sha256'],
                 'Review receipt refers to a different execution')
            receipt = supplemented_receipt(receipt, wrapper,
                        cohort['supplemental_snapshot_inputs'], frozen, manifest)
            supplement = dict(path=cohort['review_receipt'],
                              sha256=cohort['review_receipt_sha256'],
                              verified_frozen_command_inputs=cohort['supplemental_snapshot_inputs'])
        inventory = nasa.inventory(frozen)
        need(inventory['sources'] == mapping['sources'] == baseline['sources'],
             'NASA source baseline differs')
        mapped = {r['key']: r for r in mapping['reviews']}
        results = {r['name']: r for r in receipt['results']}
        cohort_rows = []
        for decision in decisions:
            key = decision['key']
            need(key in candidates and key in mapped, 'Unknown/unmapped candidate: ' + key)
            row = mapped[key]
            need(row['fingerprint'] == candidates[key]['fingerprint'], 'Source row identity differs')
            need(decision.get('rationale') and decision.get('remaining_limits') is not None,
                 'Missing substantive review decision')
            checks = [verify_claim(frozen, manifest, receipt, results.get(c['test'], {}), c)
                      for c in row.get('evidence', [])]
            need(bool(checks), 'No evidence for ' + key)
            if cohort_name == 'visible':
                need(all(c['native_exposure_events'] > 0 for c in checks),
                     'Missing native exposure in visible cohort')
            cohort_rows.append(dict(key=key, checks=checks))
        observations.append(dict(cohort=cohort_name, receipt=cohort['receipt'],
                                 receipt_sha256=cohort['receipt_sha256'],
                                 mapping=cohort['mapping_manifest'],
                                 mapping_sha256=cohort['mapping_manifest_sha256'],
                                 snapshot_id=manifest_id, supplement=supplement, rows=cohort_rows))
    need({c['cohort'] for c in observations} == set(plan['required_cohorts']),
         'Missing mandatory execution cohort')
    baseline_keys = {kind: {r['key'] for r in union[kind]['rows']}
                     for kind in ('requirements', 'historical_cases')}
    accepted = [d for d in decisions if d['decision'] == 'accept_versioned_scope']
    pending = [d for d in decisions if d['decision'] == 'missing_obligation']
    need(len(accepted) + len(pending) == len(decisions), 'Unknown decision')
    increments = {kind: [d['key'] for d in accepted
                         if d['key'] not in baseline_keys[kind]
                         and d['key'] in {r['key'] for r in baseline[kind]}]
                  for kind in baseline_keys}
    return dict(version=version['label'], decisions=decisions, cohorts=observations,
                accepted_source_versioned_scope_keys=increments,
                accepted_count=len(accepted), pending_count=len(pending),
                new_test_executions=0, full_current_build_qualified=False,
                full_NASA_qualified=False,
                limit='Source-versioned bounded row recovery only. Manual scope decisions are explicit; no partial row fragments, current-build, physical-input or scientific certification inference.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, default=Path(__file__).resolve().parents[3])
    parser.add_argument('--plan', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    plan_bytes = payload(args.plan)
    result = audit(args.repo, json.loads(plan_bytes))
    result['plan_path'] = str(args.plan.resolve())
    result['plan_sha256'] = digest(plan_bytes)
    result['auditor_sha256'] = digest(Path(__file__).read_bytes())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({k: result[k] for k in ('accepted_count', 'pending_count',
                                          'new_test_executions', 'full_current_build_qualified')}))


if __name__ == '__main__':
    main()
