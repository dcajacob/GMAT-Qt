"""Verify the real-number Vary control before joining the original FRC-2 parents."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import tarfile

import AuditNasaCoveragePrefix as prefix
import BuildNasaCoverageExtension as common
import NasaGuiCoverage as nasa
sha,load,require=common.sha,common.load,common.require
KEYS=['FRC!E51','FRC!E47','FRC!E8']
CONTEXT=[f'FRC!E{i}' for i in range(24,53)]
KIND='source-versioned-real-number-vary-control-and-complete-parent-conjunction'
GATES=['scientific_approval','technical_core_approval','full_current_build_qualified',
       'full_NASA_qualified','native_OS_input_qualified']


def qualify(repo,path,inventory):
    q=load(path)
    known={x['key']:x for x in inventory['requirements']}
    require(q['contract_kind']==KIND and q['qualified_keys']==KEYS
        and q['authority_sha256']==inventory['sources'] and all(q[k] is False for k in GATES),'Changed Vary authority')
    for n,h in q['artifact_sha256'].items():require(sha(n)==h,'Changed Vary artifact: '+n)
    c=load(q['conjunction'])
    require(c['original_rows']==[known[k] for k in KEYS] and c['full_FRC_2_context']==[known[k] for k in CONTEXT]
        and c['control_type_children']==[f'FRC!E{i}' for i in range(48,52)],'Missing original Vary context')
    require(known[KEYS[0]]['text'].strip().endswith('Real number') and 'FRC-2' in known[KEYS[2]]['text'],
        'Wrong literal or parent obligation')
    for n,h in c['artifact_sha256'].items():require(sha(n)==h,'Changed actual Vary evidence: '+n)
    for archive,ids in [('reader_source_archive','reader_source_sha256'),('proof_source_archive','proof_source_sha256')]:
        with tarfile.open(q[archive]) as tar:
            require(set(tar.getnames())==set(q[ids]),'Incomplete Vary source archive')
            for n,h in q[ids].items():
                raw=tar.extractfile(n).read()
                require(hashlib.sha256(raw).hexdigest()==h and not raw.startswith(b'\x7fELF'),'Changed source capsule member')
    protected=load(q['protected_input_baseline'])
    authorized=load(q['authorized_input_manifest'])
    require(len(protected)==len(authorized)==1334 and all(q['proof_source_sha256'][n]==h for n,h in protected.items()),'Lost original protected inputs')
    for n in protected:
        if protected[n]!=authorized[n]:
            require(q['proof_source_sha256']['approved-observer-inputs/'+n]==authorized[n],'Lost observer amendment')
    prior={x['original_source']['key']:x for x in load(q['prior_full_audit'])['rows']}
    require(all(prior[k]['status']=='verified-source-versioned' for k in CONTEXT if k not in KEYS),
         'Missing accepted Vary control or settings prerequisite')
    saved=load(c['native_literal_receipts'])['cases']
    require(len(saved)==13 and all(x['passed'] and x['returncode']==0 and x['proof']['passed'] for x in saved),
        'Missing saved literal role, ownership or actual Target execution')
    for p in c['gui_picker_results']:
        x=load(p)
        require(x['status']=='passed' and x['returncode']==0 and x['name']=='QtGui.VaryNumericPicker','Missing actual finite-number picker execution')
    require(len(c['gui_picker_results'])==2,'Missing numeric-control GUI profile')
    d=Path(c['native_solve']);x=load(d/'result.json');plan=load(d/'plan.json')
    values=[float(v) for v in (d/'values.txt').read_text().split()]
    require(x['status']=='passed' and x['history_ok'] and x['returncode']==0
        and values==x['actual']==[2,1,42] and plan['final_bounds']==[1e-6,2e-6,0],
        'Missing independently constrained literal solve and successor')
    mission=(d/'mission.script').read_text()
    require("Vary 'Literal control' Opt(1 = 2" in mission and "NonlinearConstraint Opt(X <= 2);" in mission
        and 'Cost = (X - 3)^2;' in mission and 'SolveMode = Solve' in mission,'Substituted actual literal mission')
    log=(d/'optimizer.txt').read_text()
    trials=re.findall(r'Completed function evaluation (\d+), pert 1 for 1; optimizer scaled value: ([-+.\dEe]+)',log)
    require(len(trials)>=3 and all(float(v)==4 for _,v in trials)
        and '2 variables; 0 equality constraints; 1 inequality constraints' in log
        and 'The Optimizer Converged!' in log and '1 = 2; optimizer scaled value: 3, X' in log,
        'Literal is not an independently perturbed native control')
    require(all(load(p)['status']=='failed' for p in c['retained_failures']),'Retained literal failure omitted')
    return q,KEYS,dict(status='passed',qualified_keys=KEYS,whole_row_conjunction=True,
          component=dict(saved_native_literal_cases=13,saved_gui_picker_profiles=2,
                         new_yukon_literal_perturbations=len(trials),original_FRC_2_clauses=len(CONTEXT),
                         original_control_types=4),
          new_requirement_credit=3,newly_resolved_original_hold_keys=[],native_tests_rerun=0,
          runtime_binary_copies=0,**{k:False for k in GATES})


def build(repo, baseline, path, output):
    seal, inventory, keys = prefix.baseline(repo, baseline)
    original = {kind:set(values) for kind,values in keys.items()}
    _, incoming, audit = qualify(repo, path, inventory)
    known = {kind:{row['key']:row for row in inventory[kind]} for kind in keys}
    require(not set(incoming) & keys['requirements'], 'Duplicate scope credit')
    keys['requirements'] = common.merge_keys(keys['requirements'], incoming, known['requirements'])
    coverage = {kind:{'verified':len(keys[kind]), 'total':len(known[kind]),
        'percent':round(100*len(keys[kind])/len(known[kind]),1),
        'new_verified':len(keys[kind])-len(original[kind])} for kind in keys}
    remaining = {kind:[row for row in seal['remaining_original_rows'][kind]
        if row['key'] not in keys[kind]] for kind in keys}
    result = {'status':'passed', 'scope':'Sealed source-versioned prefix plus original numeric Vary control and its two parent conjunctions',
        'baseline':str(baseline), 'baseline_sha256':sha(baseline), 'authority_sha256':inventory['sources'],
        'coverage':coverage, 'target_requirements':1101, 'target_reached':len(keys['requirements'])>=1101,
        'requirements_still_needed':max(0,1101-len(keys['requirements'])),
        'additions':[{'key':key, 'kind':'requirements', 'fingerprint':known['requirements'][key]['fingerprint'],
            'source_text':known['requirements'][key]['text'], 'qualification':str(path),
            'qualification_sha256':sha(path), 'status':'verified-source-versioned'} for key in incoming],
        'remaining_counts':{kind:len(rows) for kind,rows in remaining.items()},
        'remaining_original_rows':remaining, 'technical_core_approval':False,
        'full_current_build_qualified':False, 'full_NASA_qualified':False,
        'limits':['Original 1158/2367 denominators, texts and fingerprints are unchanged.',
                  'Only FRC E51, E47 and E8 are added. All other row records and 59 formal hold statuses remain unchanged.',
                  'Actual literal solver registration/perturbation and accepted FRC-2 child proofs complete the original control conjunctions. Broader solver science and platform scope remain separate.']}
    with Path(output).open('x') as file:
        json.dump(result, file, indent=2); file.write('\n')
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
        print(json.dumps({'coverage':result['coverage'], 'still_needed':result['requirements_still_needed']}))
    else:
        _, _, result = qualify(args.repo, args.qualification, nasa.inventory(args.repo))
        with args.output.open('x') as file:
            json.dump(result, file, indent=2); file.write('\n')
        print(json.dumps(result))
