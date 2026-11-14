"""Maintain an explicit campaign union without rewriting sealed/older ledgers."""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path

import NasaGuiCoverage as nasa
from RunHarnessAcceptance import passed_junit


def load(path):
    return json.loads(Path(path).read_text())


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def validate_recovery_report(report, plan):
    """Never turn a partial/manual pending decision into union credit."""
    if report['decisions'] != plan['decisions'] or report['version'] != plan['version']:
        raise ValueError('Recovery report no longer matches its reviewed plan')
    decisions = report['decisions']
    keys = {d['key'] for d in decisions}
    if len(keys) != len(decisions):
        raise ValueError('Duplicate recovery decision')
    expected = {d['key'] for d in decisions if d['decision'] == 'accept_versioned_scope'}
    supplied = [key for values in report['accepted_source_versioned_scope_keys'].values()
                for key in values]
    if len(supplied) != len(set(supplied)) or set(supplied) != expected:
        raise ValueError('Recovery credit does not match accepted complete decisions')
    cohorts = report['cohorts']
    if {c['cohort'] for c in cohorts} != set(plan['required_cohorts']):
        raise ValueError('Missing mandatory complete cohort')
    for cohort in cohorts:
        if ({r['key'] for r in cohort['rows']} != keys
                or len(cohort['rows']) != len(keys)
                or not all(r['checks'] for r in cohort['rows'])):
            raise ValueError('Partial cohort cannot supply whole-row credit')
        for row in cohort['rows']:
            if row['key'] in expected and any(check.get('verified') is False or 'rejection' in check
                                               for check in row['checks']):
                raise ValueError('Rejected proof binding cannot supply scope credit')
    if report.get('new_test_executions') != 0 or report.get('full_current_build_qualified'):
        raise ValueError('Recovery report incorrectly claims new/current-build execution')


def contract_report(repo, path):
    qualification = load(path)
    if (qualification.get('full_current_build_qualified')
            or qualification.get('full_NASA_qualified')
            or not qualification.get('source_inputs_unchanged')):
        raise ValueError('Harness qualification claims exceed its execution scope')
    for field in ('review', 'receipt', 'report'):
        if sha(qualification[field]) != qualification[field + '_sha256']:
            raise ValueError('Harness qualification artifact changed: ' + field)
    receipt = load(qualification['receipt'])
    frozen = Path(qualification['source_root'])
    copied, authorities = qualification['preserved_source_sha256'], qualification['authority_inputs_not_copied']
    if set(copied) & set(authorities) or set(copied) | set(authorities) != set(receipt['source_hashes']):
        raise ValueError('Incomplete harness input preservation')
    original_review = str(Path(qualification['original_review']).relative_to(repo.resolve()))
    if (Path(qualification['review']) != frozen/original_review
            or copied.get(original_review) != qualification['review_sha256']):
        raise ValueError('Harness review is not the unchanged recorded execution input')
    for source, value in copied.items():
        if value != receipt['source_hashes'][source] or sha(frozen/source) != value:
            raise ValueError('Preserved harness source changed: ' + source)
    inventory = nasa.inventory(repo)
    if authorities != inventory['sources']:
        raise ValueError('Harness NASA source baseline changed')
    review = nasa.load_review(frozen, inventory, manifest=Path(qualification['review']))
    if any(r['applicability'] != 'harness' for r in review['reviews']):
        raise ValueError('Backend contracts cannot qualify GMAT GUI rows')
    if not receipt.get('source_inputs_unchanged'):
        raise ValueError('Harness execution source drift')
    for result in receipt['results']:
        if (result['status'] != 'passed'
                or not passed_junit(Path(result['junit']), result['name'], result['returncode'])
                or any(sha(result[k]) != result[k+'_sha256'] for k in ('log','junit'))):
            raise ValueError('Missing successful unchanged exact harness test')
    rechecked = nasa.evaluate(frozen, inventory, review, receipt)
    actual = {r['key'] for kind in ('requirements','historical_cases')
              for r in rechecked[kind] if r['status']=='verified'}
    expected = {r['key'] for r in review['reviews']}
    if actual != expected or actual != set(qualification['qualified_keys']):
        raise ValueError('Harness proof no longer qualifies every declared contract')
    return qualification, sorted(actual)


def gui_contract_report(repo, path):
    """Qualify the explicitly reviewed report or physical-unit native contracts."""
    q=load(path)
    kind=q.get('contract_kind','vf13ad-report')
    if kind=='vf13ad-report':
        expected_tests={'QtGui.NativeNasaVF13adProgress'+mode for mode in ('Forward','Central')}
        expected_keys={'FRR!E'+str(row) for row in range(294,300)}
    elif kind=='physical-unit-inputs':
        expected_tests={'QtGui.NativeNasaUnitInputs'}
        expected_keys={'All Others!E'+str(row) for row in (14,15,16)}
    else:raise ValueError('Unreviewed native GUI contract kind')
    if q.get('full_current_build_qualified') or q.get('full_NASA_qualified'):
        raise ValueError('Targeted GUI qualification exceeds its execution scope')
    for field in ('review','receipt','report','inputs_before','inputs_after'):
        if sha(q[field])!=q[field+'_sha256']:raise ValueError('GUI qualification artifact changed: '+field)
    before,after=load(q['inputs_before']),load(q['inputs_after'])
    if before!=after:raise ValueError('GUI qualification source/runtime drift')
    receipt=load(q['receipt']);frozen=Path(q['source_root']);inventory=nasa.inventory(repo)
    if receipt.get('mode')!='visible' or receipt.get('max_screenshots')!=0:
        raise ValueError('GUI contract needs the actual screenshot-free visible receipt')
    display=receipt.get('visible_display',{})
    if (not display.get('resizable') or not display.get('start_minimized')
            or 'Iconic' not in display.get('initial_host_window_state','')
            or '_NET_WM_STATE_FULLSCREEN' in display.get('initial_host_window_state','')):
        raise ValueError('GUI execution did not preserve the requested window policy')
    actual={r['name']:r for r in receipt['results'] if r['status']!='not-run'}
    if set(actual)!=expected_tests or set(q['cases'])!=expected_tests:
        raise ValueError('GUI contract lacks its exact reviewed native cases')
    for name,result in actual.items():
        case=q['cases'][name]
        target='GmatQtNasaVf13adProgressTests' if kind=='vf13ad-report' else 'GmatQtNasaUnitInputTests'
        executables=[p for p in result.get('registered_command',[]) if Path(p).name==target]
        if (len(executables)!=1 or executables[0] not in before.get('binaries',{})
                or receipt.get('binary_hashes',{}).get(executables[0])!=before['binaries'][executables[0]]):
            raise ValueError('GUI contract executable was not bound to the actual receipt')
        if (result['status']!='passed' or result['returncode']!=0
                or result['log']!=case['log'] or not passed_junit(Path(case['junit']),name,0)
                or any(sha(case[k])!=case[k+'_sha256'] for k in ('log','junit'))
                or result.get('trace',{}).get('native_exposure_events',0)<=0
                or result.get('trace',{}).get('screenshots')):
            raise ValueError('GUI contract lacks an unchanged complete exposed passing case')
    authorities=q['authority_inputs_not_copied'];copied=q['preserved_source_sha256']
    if authorities!=inventory['sources'] or set(copied)|set(authorities)!=set(before['sources']):
        raise ValueError('Incomplete scoped GUI source/authority preservation')
    for source,value in copied.items():
        if (before['sources'].get(source)!=value or receipt['source_hashes'].get(source)!=value
                or sha(frozen/source)!=value):raise ValueError('GUI contract source changed: '+source)
    relative=str(Path(q['original_review']).relative_to(repo.resolve()))
    if Path(q['review'])!=frozen/relative or copied.get(relative)!=q['review_sha256']:
        raise ValueError('GUI review was not the recorded execution input')
    review=nasa.load_review(frozen,inventory,manifest=Path(q['review']))
    if any(r['applicability']!='gui' for r in review['reviews']):raise ValueError('Invalid GUI applicability')
    checked=nasa.evaluate(frozen,inventory,review,receipt)
    actual_keys={r['key'] for r in checked['requirements'] if r['status']=='verified'}
    if actual_keys!=expected_keys or set(q['qualified_keys'])!=expected_keys:
        raise ValueError('Incomplete reviewed native GUI contract qualification')
    gap=q.get('documentation_gap')
    if gap:
        if kind!='physical-unit-inputs' or gap['key']!='All Others!E12':
            raise ValueError('Unreviewed local contract gap')
        if sha(gap['report'])!=gap['report_sha256']:
            raise ValueError('Documentation gap inventory changed')
        documentation=load(gap['report'])
        if documentation['qualified_keys'] or documentation['all_input_documentation_qualified']:
            raise ValueError('Documentation inventory cannot supply qualification credit')
    return q,sorted(actual_keys)


DOCUMENTATION_CHECKS = {
    'authority_exact', 'frozen_evidence_verified', 'active_factory_scope_equal',
    'native_schema_lineage_reviewed', 'original_987_identities_retained',
    'original_semantic_reviews_closed',
    'three_corrupt_or_nonrendered_records_explicitly_disposed',
    'solar_and_Precision_current_replacements_verified',
    'configured_modes_complete_within_declared_domains', 'all_34_source_domains_bound',
    'active_GUI_field_metadata_complete', 'all_1243_rendered_unit_rows_verified',
    'all_34_non_table_policies_and_9_convention_rows_rendered',
    'all_reachable_installed_pages_match',
}


def validate_documentation_contract(qualification, report):
    """Fail closed: this documentary proof can supersede only the exact E12 gap."""
    from AuditAllInputDocumentation import KEY, FINGERPRINT, TEXT
    for item in (qualification, report):
        if (item.get('contract_kind') != 'input-documentation'
                or item.get('qualified_keys') != [KEY]
                or item.get('source_inputs_unchanged') is not True
                or any(item.get(flag) is not False for flag in
                       ('full_current_build_qualified', 'full_NASA_qualified', 'technical_core_approval'))):
            raise ValueError('Documentation qualification exceeds its reviewed scope')
    if (report.get('status') != 'passed' or report.get('key') != KEY
            or report.get('fingerprint') != FINGERPRINT or report.get('requirement_text') != TEXT
            or report.get('all_input_documentation_qualified') is not True
            or set(report.get('checks', {})) != DOCUMENTATION_CHECKS
            or any(value is not True for value in report['checks'].values())
            or report.get('new_GUI_acceptance_replays') != 0
            or report.get('new_mission_executions') != 0):
        raise ValueError('Incomplete exact documentary evidence conjunction')


def documentation_contract_report(repo, path):
    from AuditAllInputDocumentation import audit
    qualification = load(path)
    report_path = Path(qualification['report'])
    if sha(report_path) != qualification['report_sha256']:
        raise ValueError('Documentation qualification report changed')
    report = load(report_path)
    validate_documentation_contract(qualification, report)
    verifier = repo / 'src/qtgui/tests/AuditAllInputDocumentation.py'
    if sha(verifier) != qualification['verifier_sha256']:
        raise ValueError('Documentation verifier changed')
    # This is a read-only documentation/evidence audit. No GUI or mission replay.
    rechecked = audit(repo.resolve(), Path(qualification['evidence_root']).resolve())
    if rechecked != report:
        raise ValueError('Documentation sources, artifacts or complete review no longer match')
    return qualification, report['qualified_keys']


def add_documentation_contract(findings, accepted, path, qualification, keys):
    for key in keys:
        prior = findings.get(key)
        if (key != 'All Others!E12' or key in accepted or not prior
                or prior.get('decision') != 'missing_obligation'
                or not prior.get('inventory') or not prior.get('inventory_sha256')):
            raise ValueError('Documentation may supersede only the retained exact E12 gap')
        # Retain the sealed exploratory gap as superseded evidence, never rewrite it.
        findings[key] = dict(key=key, decision='verified_documentation_contract',
                             qualification=str(path), qualification_sha256=sha(path),
                             superseded_documentation_gap=prior)
        accepted[key] = dict(kind='requirements', version='source-bound-input-documentation-contract',
                             qualification=str(path), qualification_sha256=sha(path))


def build(repo, output, audits, contracts=(), gui_contracts=(), documentation_contracts=()):
    feasibility_path = repo / 'build/nasa-coverage/remaining-feasibility-20261101/feasibility.json'
    baseline_path = repo / 'build/release-check/20261031-reviewed-candidate/handoff-source-scope-union.json'
    feasibility, baseline = load(feasibility_path), load(baseline_path)
    campaign, accepted, findings = [], {}, {}
    for audit_path in audits:
        report = load(audit_path)
        adapter=None
        if report.get('audit_type')=='recent-body-recovery':
            import AuditRecentBodyRecovery as adapter
        elif report.get('audit_type')=='visual-recovery':
            import AuditVisualRecovery as adapter
        elif report.get('audit_type')=='graphics-recovery':
            import AuditGraphicsRecovery as adapter
        elif report.get('audit_type')=='remaining-recovery':
            import AuditRemainingRecovery as adapter
        auditor=adapter.__name__+'.py' if adapter else 'AuditAcceptanceRecovery.py'
        if sha(repo / 'src/qtgui/tests' / auditor) != report['auditor_sha256']:
            raise ValueError('Audit must be regenerated with the current verifier: ' + str(audit_path))
        plan_path = Path(report['plan_path'])
        if sha(plan_path) != report['plan_sha256']:
            raise ValueError('Reviewed recovery plan changed: ' + str(plan_path))
        plan=load(plan_path)
        if adapter:
            if report['original_claim_verifier_sha256']!=sha(repo/'src/qtgui/tests/AuditAcceptanceRecovery.py'):
                raise ValueError('Original source/receipt claim verifier changed')
            repeated=adapter.audit(repo,plan)
            supplied={k:v for k,v in report.items() if k not in
                      ('plan_path','plan_sha256','auditor_sha256','original_claim_verifier_sha256')}
            if repeated!=supplied:raise ValueError('Recent body audit no longer matches retained evidence')
        validate_recovery_report(report, plan)
        audit_sha = sha(audit_path)
        for decision in report['decisions']:
            key = decision['key']
            if key in findings:
                raise ValueError('Duplicate candidate audit: ' + key)
            findings[key] = {**decision, 'audit': str(audit_path), 'audit_sha256': audit_sha}
        for kind, keys in report['accepted_source_versioned_scope_keys'].items():
            for key in keys:
                if key in accepted:
                    raise ValueError('Duplicate scope credit: ' + key)
                accepted[key] = dict(kind=kind, version=report['version'],
                                     audit=str(audit_path), audit_sha256=audit_sha)
    contract_inputs = []
    for path in contracts:
        qualification, keys = contract_report(repo, path)
        contract_inputs.append(dict(path=str(path), sha256=sha(path)))
        for key in keys:
            if key in accepted or key in findings:
                raise ValueError('Duplicate recovered/contract source scope: ' + key)
            findings[key] = dict(key=key, decision='verified_harness_contract',
                                 qualification=str(path), qualification_sha256=sha(path))
            accepted[key] = dict(kind='requirements', version='source-bound-harness-contract',
                                 qualification=str(path), qualification_sha256=sha(path))
    for path in gui_contracts:
        qualification,keys=gui_contract_report(repo,path)
        contract_inputs.append(dict(path=str(path),sha256=sha(path),kind='targeted-native-GUI'))
        gap=qualification.get('documentation_gap')
        if gap:
            key=gap['key']
            if key in accepted or key in findings:raise ValueError('Duplicate local contract gap: '+key)
            findings[key]=dict(key=key,decision='missing_obligation',rationale=gap['rationale'],
                               qualification=str(path),qualification_sha256=sha(path),
                               inventory=gap['report'],inventory_sha256=gap['report_sha256'])
        for key in keys:
            if key in accepted or key in findings:raise ValueError('Duplicate GUI contract scope: '+key)
            findings[key]=dict(key=key,decision='verified_gui_contract',qualification=str(path),qualification_sha256=sha(path))
            accepted[key]=dict(kind='requirements',version='source-bound-native-GUI-contract',qualification=str(path),qualification_sha256=sha(path))
    for path in documentation_contracts:
        qualification, keys = documentation_contract_report(repo, path)
        contract_inputs.append(dict(path=str(path), sha256=sha(path), kind='input-documentation'))
        add_documentation_contract(findings, accepted, path, qualification, keys)
    candidate_keys = set()
    for kind, rows in feasibility['rows'].items():
        for row in rows:
            if row['decision'] not in ('audit_old_passes', 'audit_recent_passes', 'recommend_local'):
                continue
            key = row['key']
            candidate_keys.add(key)
            finding = findings.get(key)
            if finding and finding['decision'] in ('verified_harness_contract','verified_gui_contract','verified_documentation_contract'):
                if finding['decision'] == 'verified_documentation_contract':
                    from AuditAllInputDocumentation import KEY, FINGERPRINT
                    if key != KEY or row['fingerprint'] != FINGERPRINT:
                        raise ValueError('Documentation credit outside its exact authority')
                    bundles = ('units-inventory',)
                else:
                    bundles=('solver-report-schema','units-inventory') if finding['decision']=='verified_gui_contract' else ('suite-queries-metadata-summary','reusable-comparators-html')
                if row['bundle'] not in bundles:
                    raise ValueError('Contract credit outside its approved scope: ' + key)
                status = 'verified_contract'
            else:
                status = ('recovered' if key in accepted else 'missing_obligation') if finding else 'pending'
            campaign.append(dict(key=key, fingerprint=row['fingerprint'], kind=kind,
                                 task='recovery' if row['decision'].startswith('audit_') else 'local_contract',
                                 bundle=row['bundle'], status=status,
                                 evidence=finding))
    if set(findings) - candidate_keys:
        raise ValueError('Audited keys outside the recommended scope')
    coverage = {}
    for kind in ('requirements', 'historical_cases'):
        old = {r['key']: r for r in baseline[kind]['rows']}
        new = {r['key']: r for r in campaign if r['kind'] == kind
               and r['status'] in ('recovered','verified_contract')}
        if set(old) & set(new):
            raise ValueError('Scope was already in the baseline')
        total = baseline[kind]['total']
        coverage[kind] = dict(baseline_verified=len(old),
                              recovered=sum(r['status']=='recovered' for r in new.values()),
                              new_contracts=sum(r['status']=='verified_contract' for r in new.values()),
                              verified=len(old) + len(new), total=total,
                              percent=round(100 * (len(old) + len(new)) / total, 1),
                              recovered_rows=[{**new[key], **accepted[key]} for key in sorted(new)])
    counts = {task: dict(Counter(r['status'] for r in campaign if r['task'] == task))
              for task in ('recovery', 'local_contract')}
    if sum(counts['recovery'].values()) != 217 or sum(counts['local_contract'].values()) != 37:
        raise ValueError('Original recommended scope changed')
    result = dict(scope='Recommended217recovery+37local-contract campaign; all original NASA rows remain in their unchanged denominators.',
                  baseline_union=dict(path=str(baseline_path), sha256=sha(baseline_path)),
                  feasibility=dict(path=str(feasibility_path), sha256=sha(feasibility_path)),
                  coverage=coverage, task_counts=counts, candidates=campaign, contract_inputs=contract_inputs,
                  full_current_build_qualified=False, full_NASA_qualified=False,
                  limits=['This is an explicitly audited source-versioned union, not a new full-binary run.',
                          'Older broad ledger counts are not added to this union.',
                          'No new NASA credit from recovery-tool unit-test counts.',
                          'The other original source scopes and global qualification gates remain open.'])
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + '\n')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, default=Path(__file__).resolve().parents[3])
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--audit', type=Path, action='append', required=True)
    parser.add_argument('--contract', type=Path, action='append', default=[])
    parser.add_argument('--gui-contract', type=Path, action='append', default=[])
    parser.add_argument('--documentation-contract', type=Path, action='append', default=[])
    args = parser.parse_args()
    report = build(args.repo, args.output, args.audit, args.contract, args.gui_contract, args.documentation_contract)
    print(json.dumps(dict(task_counts=report['task_counts'], coverage={
        k: {field: v[field] for field in ('verified', 'total', 'percent')}
        for k, v in report['coverage'].items()})))


if __name__ == '__main__':
    main()
