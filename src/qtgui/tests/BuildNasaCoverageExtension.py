"""Extend a sealed NASA union with complete native and harness proofs.

Old passing evidence is reused at its recorded source versions. No GUI case is
executed here, and historical, scientific and full-current-build gates remain
separate. Runtime binaries are identified, never copied into proof archives.
"""
import argparse
import hashlib
import json
from pathlib import Path
import tarfile
import tempfile

import NasaGuiCoverage as nasa
from RunHarnessAcceptance import passed_junit


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def load(path):
    return json.loads(Path(path).read_text())


def require(condition, message):
    if not condition:
        raise ValueError(message)


def merge_keys(existing, incoming, known):
    require(len(incoming)==len(set(incoming)), 'Duplicate extension credit')
    require(not set(incoming)&set(existing), 'Already qualified scope cannot receive new credit')
    require(set(incoming)<=set(known), 'Unknown source-row credit')
    return set(existing)|set(incoming)


def sealed_baseline(repo, path):
    seal=load(path)
    require(seal['target_requirements']==985, 'Changed 85 percent target')
    inventory=nasa.inventory(repo)
    require(seal['authority_sha256']==inventory['sources'], 'NASA authority changed')
    for name,digest in seal['baseline_artifact_sha256'].items():
        require(sha(repo/name)==digest, 'Sealed baseline changed: '+name)
    prior=repo/'build/nasa-coverage/recommended-campaign-20261101'
    final=load(prior/'twenty-second-increment-audit.json')
    require(final['status']=='passed' and final['bounded_recommended_campaign_complete'], 'Unfinished baseline')
    for name,digest in final['report_sha256'].items():
        require(sha(prior/name)==digest, 'Baseline final evidence changed: '+name)
    archived=prior/'twenty-second-increment-sources.tar.gz'
    require(sha(archived)==final['source_archive_sha256'], 'Baseline source archive changed')
    with tarfile.open(archived) as bundle:
        for name,digest in final['source_sha256'].items():
            require(hashlib.sha256(bundle.extractfile(name).read()).hexdigest()==digest, 'Changed archived baseline source')
    campaign=load(prior/'campaign.json')
    original=Path(campaign['baseline_union']['path'])
    require(sha(original)==campaign['baseline_union']['sha256'], 'Original union changed')
    original=load(original)
    keys={kind:{r['key'] for r in original[kind]['rows']}|{r['key'] for r in campaign['coverage'][kind]['recovered_rows']} for kind in ('requirements','historical_cases')}
    require(len(keys['requirements'])==924 and len(keys['historical_cases'])==1816, 'Baseline numerator mismatch')
    require(len(inventory['requirements'])==1158 and len(inventory['historical_cases'])==2367, 'Original denominator changed')
    return seal,inventory,keys,load(prior/'campaign-gaps.json')


def validate_visible(receipt, tests):
    require(receipt.get('mode')=='visible' and receipt.get('max_screenshots')==0, 'Native qualification requires screenshot-free visible execution')
    display=receipt.get('visible_display',{})
    require(display.get('resizable') and display.get('start_minimized') and 'Iconic' in display.get('initial_host_window_state','')
            and '_NET_WM_STATE_FULLSCREEN' not in display.get('initial_host_window_state',''), 'Requested window policy not proved')
    results={r['name']:r for r in receipt['results'] if r['status']!='not-run'}
    require(set(results)==set(tests), 'Missing or extra executed native case')
    for name,result in results.items():
        require(result['status']=='passed' and result['returncode']==0 and result.get('trace',{}).get('native_exposure_events',0)>0
                and not result.get('trace',{}).get('screenshots'), 'Native result incomplete or unexposed')
        case=tests[name]
        require(result['log']==case['log'] and passed_junit(Path(case['junit']),name,0), 'Missing exact passing JUnit')
        for field in ('log','junit'):
            require(sha(case[field])==case[field+'_sha256'], 'Changed native '+field)
    return results


def qualify(repo, path, inventory):
    q=load(path)
    require(len(q['qualified_keys'])==len(set(q['qualified_keys'])), 'Duplicate qualification source keys')
    require(q['contract_kind']=='source-versioned-native-GUI' and q['source_inputs_unchanged'] is True
            and all(q.get(k) is False for k in ('full_current_build_qualified','full_NASA_qualified','technical_core_approval')), 'Qualification exceeds execution scope')
    for field in ('receipt','inputs_before','inputs_after','source_archive'):
        require(sha(q[field])==q[field+'_sha256'], 'Qualification artifact changed: '+field)
    before,after=load(q['inputs_before']),load(q['inputs_after'])
    require(before==after, 'Source/runtime changed during native execution')
    receipt=load(q['receipt'])
    tests=validate_visible(receipt,q['cases'])
    copied,authorities,runtime=q['preserved_source_sha256'],q['authority_inputs_not_copied'],q['recorded_binary_source_inputs_not_copied']
    groups=[set(copied),set(authorities),set(runtime)]
    require(not any(a&b for i,a in enumerate(groups) for b in groups[i+1:]) and set().union(*groups)==set(receipt['source_hashes']), 'Incomplete/overlapping recorded source input preservation')
    require(authorities==inventory['sources'], 'Qualification authority differs')
    require({**copied,**authorities,**runtime}==receipt['source_hashes'], 'Input hashes differ from actual pre-execution receipt')
    require(sha(Path(nasa.__file__))==copied['src/qtgui/tests/NasaGuiCoverage.py'], 'Coverage interpreter differs from execution version')
    for source,digest in copied.items():
        if source in before['sources']:
            require(before['sources'][source]==digest, 'Independent source snapshot disagrees')
    for name,result in tests.items():
        executables=[p for p in result['registered_command'] if p in receipt['binary_hashes']]
        require(len(executables)==1 and executables[0] in before['binaries']
                and before['binaries'][executables[0]]==receipt['binary_hashes'][executables[0]], 'Unbound actual native executable')
    with tempfile.TemporaryDirectory(prefix='gmat-native-proof-') as folder:
        frozen=Path(folder)
        with tarfile.open(q['source_archive']) as bundle:
            require({m.name for m in bundle.getmembers()}==set(copied), 'Source archive partition differs')
            for source,digest in copied.items():
                require(not Path(source).is_absolute() and '..' not in Path(source).parts, 'Unsafe source member')
                data=bundle.extractfile(source).read()
                require(hashlib.sha256(data).hexdigest()==digest and not data.startswith(b'\x7fELF'), 'Changed/non-source archive member')
                target=frozen/source;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
        for source in authorities:
            target=frozen/source;target.parent.mkdir(parents=True,exist_ok=True);target.symlink_to(repo/source)
        review_path=frozen/q['review_relative']
        review=nasa.load_review(frozen,inventory,manifest=review_path)
        require(all(r['applicability']=='gui' for r in review['reviews']), 'Invalid native GUI applicability')
        checked=nasa.evaluate(frozen,inventory,review,receipt)
        keys={r['key'] for kind in ('requirements','historical_cases') for r in checked[kind] if r['status']=='verified'}
        require(keys==set(q['qualified_keys'])=={r['key'] for r in review['reviews']}, 'Partial declared row conjunction cannot qualify')
    return q,sorted(keys)


def build(repo, baseline, qualifications, output):
    seal,inventory,keys,gaps=sealed_baseline(repo,baseline)
    original={kind:set(values) for kind,values in keys.items()}
    known={kind:{r['key']:r for r in inventory[kind]} for kind in keys}
    additions=[]
    for path in qualifications:
        if load(path).get('contract_kind')=='source-versioned-native-C-API':
            from AuditCapiAcceptance import qualify as qualify_c_api
            q,qualified=qualify_c_api(repo,path,inventory)
        elif load(path).get('contract_kind')=='source-versioned-GUI-live-composition':
            from AuditSolverPublicationAcceptance import qualify as qualify_live_gui
            q,qualified=qualify_live_gui(repo,path,inventory)
        elif load(path).get('contract_kind')=='source-versioned-missing-GUI-conjunction':
            from AuditMissingModeAcceptance import qualify as qualify_missing_gui
            q,qualified=qualify_missing_gui(repo,path,inventory)
        elif load(path).get('contract_kind')=='source-versioned-hardware-conjunction':
            from AuditHardwareConjunctionAcceptance import qualify as qualify_hardware
            q,qualified=qualify_hardware(repo,path,inventory)
        elif load(path).get('contract_kind')=='source-versioned-C-API-status':
            from AuditCapiStatusAcceptance import qualify as qualify_c_status
            q,qualified=qualify_c_status(repo,path,inventory)
        elif load(path).get('contract_kind')=='source-versioned-native-C-API-summary':
            from AuditCapiSummaryAcceptance import qualify as qualify_c_summary
            q,qualified=qualify_c_summary(repo,path,inventory)
        elif load(path).get('contract_kind')=='source-versioned-full-scalar-constraint-family':
            from AuditConstraintScalarAcceptance import qualify as qualify_scalars
            q,qualified=qualify_scalars(repo,path,inventory)
        elif load(path).get('contract_kind')=='source-versioned-native-console-conjunction':
            from AuditConsolePlotAcceptance import qualify as qualify_console
            q,qualified=qualify_console(repo,path,inventory)
        elif load(path).get('contract_kind')=='source-versioned-harness-specialized-comparators':
            from AuditSpecializedComparatorAcceptance import qualify as qualify_specialized
            q,qualified=qualify_specialized(repo,path,inventory)
        elif load(path).get('contract_kind')=='source-versioned-harness-reports':
            from AuditReportExportAcceptance import qualify as qualify_reports
            q,qualified=qualify_reports(repo,path,inventory)
        else:
            q,qualified=qualify(repo,path,inventory)
        for kind in keys:
            incoming=[key for key in qualified if key in known[kind]]
            keys[kind]=merge_keys(keys[kind],incoming,known[kind])
            status='verified-source-versioned-harness-contract' if q['contract_kind'].startswith('source-versioned-harness-') else 'verified-source-versioned-native-contract'
            additions.extend(dict(key=key,kind=kind,fingerprint=known[kind][key]['fingerprint'],source_text=known[kind][key]['text'],qualification=str(path),qualification_sha256=sha(path),status=status) for key in incoming)
    coverage={kind:dict(verified=len(values),total=len(known[kind]),percent=round(100*len(values)/len(known[kind]),1),new_verified=len(values-original[kind])) for kind,values in keys.items()}
    remaining={kind:[row for row in gaps['rows'][kind] if row['key'] not in keys[kind]] for kind in keys}
    require(all(len(remaining[k])==coverage[k]['total']-coverage[k]['verified'] for k in keys), 'Lost original remaining scope')
    result=dict(status='passed',scope='85 percent requirements campaign: sealed prior union plus independently audited complete GUI, C API and acceptance-harness contracts.',baseline=str(baseline),baseline_sha256=sha(baseline),coverage=coverage,target_requirements=985,requirements_still_needed=max(0,985-coverage['requirements']['verified']),target_reached=coverage['requirements']['verified']>=985,additions=additions,remaining_counts={k:len(v) for k,v in remaining.items()},remaining_original_rows=remaining,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False,limits=['Source-versioned capability coverage; old evidence is not attributed to a new complete binary.','All original denominators/remaining source obligations and inherited global qualification gates remain unchanged.','Do not mark this goal complete until at least985 exact requirement keys have admissible complete proof.'])
    output.write_text(json.dumps(result,indent=2)+'\n')
    return result


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3])
    parser.add_argument('--baseline',type=Path,required=True)
    parser.add_argument('--qualification',type=Path,action='append',required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    result=build(args.repo.resolve(),args.baseline,args.qualification,args.output)
    print(json.dumps(dict(coverage=result['coverage'],requirements_still_needed=result['requirements_still_needed'],target_reached=result['target_reached'])))
