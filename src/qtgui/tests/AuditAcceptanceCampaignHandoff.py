"""Read-only final audit of the bounded 217-recovery/37-contract handoff.

Reuses recorded GUI/backend passes. It never launches native tests or rewrites
sealed evidence. This proves the bounded campaign disposition, not full NASA,
scientific, platform or current-release-artifact qualification.
"""
import argparse
from collections import Counter
import json
from pathlib import Path
import subprocess
import tempfile

import BuildAcceptanceCampaign as campaign
import AuditAcceptanceRecovery as recovery

load, sha = campaign.load, campaign.sha


def require(condition, message):
    if not condition:
        raise ValueError(message)


def audit(repo, root):
    current = load(root/'campaign.json')
    before = load(root/'pre-final-consolidation/campaign.json')
    audits = sorted({row['evidence']['audit'] for row in current['candidates'] if row['task']=='recovery'})
    contracts = [Path(row['path']) for row in current['contract_inputs'] if 'kind' not in row]
    gui = [Path(row['path']) for row in current['contract_inputs'] if row.get('kind')=='targeted-native-GUI']
    docs = [Path(row['path']) for row in current['contract_inputs'] if row.get('kind')=='input-documentation']
    require((len(audits),len(contracts),len(gui),len(docs)) == (35,2,2,1), 'Incomplete campaign input families')
    # Recompute the union from original proof families without writing the live ledger.
    with tempfile.TemporaryDirectory(prefix='gmat-campaign-handoff-') as directory:
        rebuilt = campaign.build(repo,Path(directory)/'campaign.json',[Path(p) for p in audits],contracts,gui,docs)
    require(rebuilt == current, 'Campaign is not the exact current evidence union')
    reports = []
    for path in audits:
        recorded = load(path)
        plan = load(recorded['plan_path'])
        # The four specialized adapters have already been rechecked by build().
        if not recorded.get('audit_type'):
            repeated = recovery.audit(repo,plan)
            comparable = {k:v for k,v in recorded.items() if k not in ('plan_path','plan_sha256','auditor_sha256')}
            require(repeated == comparable, 'Original retained proof no longer matches: '+path)
        campaign.validate_recovery_report(recorded,plan)
        reports.append(dict(path=path,sha256=sha(path),rows=len(recorded['decisions']),
                            accepted=recorded['accepted_count'],held=recorded['pending_count']))
    rows = {row['key']:row for row in current['candidates']}
    old = {row['key']:row for row in before['candidates']}
    require(len(rows)==254 and set(rows)==set(old), 'Original candidate scope changed')
    require([key for key in rows if rows[key]!=old[key]] == ['All Others!E12'], 'Unexpected prior disposition/evidence change')
    require(current['task_counts']=={'recovery':{'recovered':158,'missing_obligation':59},'local_contract':{'verified_contract':37}}, 'Incomplete bounded tasks')
    expected = {'requirements':(924,1158,79.8),'historical_cases':(1816,2367,76.7)}
    for kind, values in expected.items():
        require(tuple(current['coverage'][kind][k] for k in ('verified','total','percent'))==values, 'Coverage accounting changed')
    feasibility_path = repo/'build/nasa-coverage/remaining-feasibility-20261101/feasibility.json'
    feasibility = load(feasibility_path)
    gaps = load(root/'campaign-gaps.json')
    require(gaps['campaign_sha256']==sha(root/'campaign.json') and gaps['feasibility_sha256']==sha(feasibility_path), 'Stale gap ledger')
    for kind, original in feasibility['rows'].items():
        qualified = {row['key'] for row in current['coverage'][kind]['recovered_rows']}
        original = {row['key']:row for row in original}
        unqualified = {row['key']:row for row in gaps['rows'][kind]}
        require(set(unqualified)==set(original)-qualified, 'Lost original remaining scopes')
        require(len(unqualified)==current['coverage'][kind]['total']-current['coverage'][kind]['verified']==gaps['counts'][kind], 'Gap denominator changed')
        for key,row in unqualified.items():
            require(all(row[k]==v for k,v in original[key].items()), 'Original gap identity/text/closure gate changed')
            require(row['campaign_disposition']==rows.get(key), 'Missing retained recovery disposition')
    handoff = load(root/'campaign-handoff.json')
    require(handoff['campaign_sha256']==sha(root/'campaign.json') and handoff['all_remaining_scope_sha256']==sha(root/'campaign-gaps.json'), 'Stale handoff')
    require(handoff['recovery_dispositions']==[r for r in current['candidates'] if r['task']=='recovery'], 'Handoff recovery rows differ')
    require(handoff['local_contracts']==[r for r in current['candidates'] if r['task']=='local_contract'], 'Handoff local rows differ')
    require(handoff['recovery_audit_complete'] and not handoff['remaining_goal_work'] and handoff['held_recovery_obligations_remain'], 'Incomplete disposition handoff')
    require(handoff['original_global_gates']==feasibility['global_gates'], 'Global qualification gates lost')
    require(handoff['original_source_conflict_dispositions']==feasibility['source_conflict_dispositions_carried'], 'Authority conflicts lost')
    require(handoff['original_nine_gate_routing']==feasibility['original_nine_gate_routing_carried'], 'Original nine-gate routing lost')
    require(handoff['historical_example_dispositions']==feasibility['historical_example_dispositions_carried'], 'Historical example dispositions lost')
    for path,digest in feasibility['protected_file_sha256'].items():
        require(sha(repo/path)==digest, 'Protected assertion/ledger/core file changed: '+path)
    source_before = load(root/'twenty-second-increment-before.json')
    production = {p:h for p,h in source_before.items() if p.startswith(('src/base/','src/gmatutil/','plugins/','src/qtgui/')) and Path(p).suffix in ('.cpp','.hpp','.h','.json')}
    for path,digest in production.items():
        require(sha(repo/path)==digest, 'Final handoff changed production/assertion source: '+path)
    core = load(root/'final-core-scrutiny.json')
    head = subprocess.check_output(['git','rev-parse','HEAD'],cwd=repo,text=True).strip()
    changed = subprocess.check_output(['git','diff','--name-only','HEAD','--','src/base','src/gmatutil','plugins'],cwd=repo,text=True).splitlines()
    actual = {p for p in changed if Path(p).suffix in ('.cpp','.hpp','.h','.c') and '/gui/' not in p}
    require(head==core['head'] and actual=={row['path'] for row in core['files']} and len(actual)==100, 'Changed core scrutiny set differs')
    for row in core['files']:
        require(sha(repo/row['path'])==row['working_sha256'], 'Core source changed after scrutiny inventory')
    require(core['technical_core_approval'] is False and handoff['total_changed_core_files']==100, 'Core approval/count overstated')
    require(handoff['core_scrutiny_sha256']==sha(root/'final-core-scrutiny.json'), 'Core scrutiny receipt changed')
    require(handoff['failure_dispositions_sha256']==sha(root/'final-failure-dispositions.json'), 'Failure disposition receipt changed')
    inherited = load(root/'retained-baseline-failure-dispositions.json')
    from RunHarnessAcceptance import passed_junit
    require(len(inherited['rows'])==24, 'Inherited baseline failure dispositions lost')
    for row in inherited['rows']:
        path = Path(row['evidence'])
        require(sha(path)==row['evidence_sha256'], 'Inherited passing result changed')
        result = load(path)
        require(result['name']==row['name'] and result['status']=='passed' and result['returncode']==0
                and passed_junit(path.parent/'ctest.xml',row['name'],0), 'Inherited failure lacks its exact later pass')
    failures = load(root/'final-failure-dispositions.json')
    for row in failures['records']:
        for path,digest in row['evidence_sha256'].items():
            require(sha(repo/path)==digest, 'Failure disposition evidence changed')
    require(any(row['status']=='unverified-core-candidate-deferred' for row in failures['records']), 'Unverified indexed-sigma finding lost')
    for item in (current,handoff,gaps,core,failures):
        require(item.get('full_current_build_qualified') is False and item.get('full_NASA_qualified') is False, 'Bounded handoff overclaims full qualification')
    require(not list(root.rglob('*.png')), 'Passing screenshots in campaign')
    libraries = list(root.rglob('*.so'))
    require(all(path.is_symlink() and path.resolve().parent==(repo/'application/bin').resolve()
                for path in libraries), 'Copied or unexpected runtime library in campaign')
    return dict(status='passed',scope='Complete disposition/evidence audit of the authorized bounded campaign; remaining original scopes/gates and unapproved core changes retained.',
                recovery_families=reports,recovery_candidates=217,recovered=158,held=59,local_contracts_verified=37,
                unchanged_original_candidate_rows=253,documentation_contract_superseded='All Others!E12',
                coverage={kind:{key:row[key] for key in ('verified','total','percent')} for kind,row in current['coverage'].items()},
                remaining_scope_counts=gaps['counts'],protected_inputs_unchanged=True,production_and_assertion_sources_preserved=len(production),
                total_changed_core_files=100,technical_core_approval=False,all_global_gates_retained=True,inherited_baseline_failures_with_later_passes=24,
                new_GUI_replays=0,new_mission_executions=0,new_screenshots=0,runtime_binary_copies=0,existing_runtime_library_symlinks=len(libraries),
                campaign_sha256=sha(root/'campaign.json'),handoff_sha256=sha(root/'campaign-handoff.json'),gaps_sha256=sha(root/'campaign-gaps.json'),core_scrutiny_sha256=sha(root/'final-core-scrutiny.json'),failure_dispositions_sha256=sha(root/'final-failure-dispositions.json'),
                full_current_build_qualified=False,full_NASA_qualified=False)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3])
    parser.add_argument('--evidence-root',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    result=audit(args.repo.resolve(),args.evidence_root.resolve())
    args.output.write_text(json.dumps(result,indent=2)+'\n')
    print('PASS: 217 recovery dispositions,37 local contracts; every original gap/global gate retained. No GUI replay.')
