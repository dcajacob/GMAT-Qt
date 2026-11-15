"""Join the executed numeric capacity and exact listed Resource Tree children."""
import argparse
import hashlib
import json
from pathlib import Path
import tarfile
import xml.etree.ElementTree as ET

import AuditNasaCoveragePrefix as prefix
import BuildNasaCoverageExtension as common
import NasaGuiCoverage as nasa

sha, load, require=common.sha,common.load,common.require
KEYS=['FRC!E113','All Others!E120']
CHILDREN=[f'All Others!E{i}' for i in range(121,154)]
KIND='source-versioned-dynamic-numerical-capacity-and-literal-resource-tree'
GATES=['scientific_approval','technical_core_approval','full_current_build_qualified',
       'full_NASA_qualified','native_OS_input_qualified']


def qualify(repo,path,inventory):
    q=load(path)
    known={x['key']:x for x in inventory['requirements']}
    require(q['contract_kind']==KIND and q['qualified_keys']==KEYS
            and q['authority_sha256']==inventory['sources'] and all(q[k] is False for k in GATES),
            'Changed original qualification scope')
    require(known[KEYS[0]]['text']=='The propagate command shall support an unlimited number of numerical-integration-type propagators as defined in FRR-13 and FRR-14 per command.'
        and known[KEYS[1]]['text']=='The GUI shall employ a tree structure (referred to as the Resource Tree) that allows users to create and configure the following resource objects:',
        'Original capacity or Resource Tree text changed')
    for name,digest in q['artifact_sha256'].items():
        require(sha(name)==digest,'Changed evidence file: '+name)
    c=load(q['conjunction'])
    require(c['original_rows']==[known[k] for k in KEYS] and c['original_children']==[known[k] for k in CHILDREN],
            'Original resource list missing or altered')
    require(c['native_tests_rerun']==0 and c['failures_waived'] is False,'Original failure or replay policy changed')
    for name,digest in c['artifact_sha256'].items():
        require(sha(name)==digest,'Changed actual receipt: '+name)
    frozen={}
    for archive,ids in [('reader_source_archive','reader_source_sha256'),('proof_source_archive','proof_source_sha256')]:
        with tarfile.open(q[archive]) as tar:
            require(set(tar.getnames())==set(q[ids]),'Incomplete source archive')
            for n,h in q[ids].items():
                raw=tar.extractfile(n).read()
                require(hashlib.sha256(raw).hexdigest()==h and not raw.startswith(b'\x7fELF'),'Changed source capsule member')
                if ids=='proof_source_sha256':frozen[n]=raw
    protected=load(q['protected_input_baseline'])
    authorized=load(q['authorized_input_manifest'])
    require(len(protected)==len(authorized)==1334 and all(q['proof_source_sha256'][n]==h for n,h in protected.items()),
            'Original protected inputs lost')
    for n in protected:
        if protected[n]!=authorized[n]:
            require(q['proof_source_sha256']['approved-observer-inputs/'+n]==authorized[n],'Authorized observer amendment lost')
    prior={x['original_source']['key']:x for x in load(q['prior_full_audit'])['rows']}
    require(all(prior[k]['status']=='verified-source-versioned' for k in CHILDREN),'Missing approved original resource child')
    route=load(c['tree_route_map'])
    require([x['original'] for x in route['rows']]==c['original_children'],'Route map dropped original type')
    cohorts=load(c['tree_results'])
    passed={'headless':set(),'visible':set()}
    for group in cohorts['runs'].values():
        for x in group['tests']:
            receipt=repo/x['receipt']
            result=load(receipt)
            cases=list(ET.parse(receipt.with_name('ctest.xml')).iter('testcase'))
            require(sha(receipt)==x['sha256'] and result['name']==x['test'] and result['status']=='passed'
                and result['returncode']==0 and len(cases)==1 and all(cases[0].find(z) is None for z in ('failure','error','skipped')),
                'Invalid terminal GUI Resource Tree result: '+x['test'])
            passed[x['visibility']].add(x['test'])
    require(all(len(s)==38 for s in passed.values()),'Lost a Resource Tree profile')
    required=set(t for row in route['rows'] for t in row['creation_and_configuration_routes'])
    require(required <= passed['headless'] & passed['visible'],'Missing typed creation/configuration route')
    bounds=c['original_capacity_bounds']
    methods=set(c['capacity_methods'])
    require(bounds==[2e-5,2e-8,1e-6] and len(methods)==9,'Changed original capacity bounds or methods')
    contexts=load(c['capacity_contexts'])
    require(len(contexts)==8,'Incomplete original capacity contexts')
    for x in contexts:
        proof=load(x['proof'])
        n=x['groups']
        require(sha(x['proof'])==x['proof_sha256'] and sha(x['receipt'])==x['receipt_sha256']
                and sha(x['snapshot_manifest'])==x['snapshot_manifest_sha256'] and proof['outcome']=='passed'
                and proof['groups']==n and proof['spacecraft']==2*n and proof['report_rows']==2*n+1
                and set(proof['methods'])==methods and proof['following_value']==42
                and all(proof[z]==n for z in ('native_unique_models','native_unique_propagators',
                                             'native_unique_setups','native_unique_state_managers'))
                and all(proof[z]<=t for z,t in zip(('max_position_error_km','max_velocity_error_km_s','max_clock_error_s'),bounds)),
                'Capacity pass does not prove actual independently checked group execution')
    require({(x['mode'],x['groups'],x['case'].startswith('Synchronized')) for x in contexts}
            =={(p,n,s) for p in ['unattended','visible'] for n in [16,128] for s in [False,True]},
            'Missing independent/synchronized mode or profile')
    for folder in c['capacity_affected_current']:
        d=Path(folder)
        result=load(d/'result.json')
        require(result['status']=='passed' and result['returncode']==0,'Missing current affected numerical-capacity execution')
        require(load(d/'proof.json')['outcome']=='passed','Missing actual numerical command outcome')
    require(len(c['capacity_affected_current'])==8,'Incomplete changed-dependency check')
    require(set(c['capacity_storage_roles'])=={'editor','parse','setup-clones','runtime-state','state-publish'},
            'Incomplete dynamic capacity source route')
    for row in c['capacity_storage_roles'].values():
        src=frozen[row['archived_source']].decode()
        require(all(a in src for a in row['anchors']),'Unbound collection role')
    return q,KEYS,dict(status='passed',qualified_keys=KEYS,whole_row_conjunction=True,
          component=dict(original_resource_children=len(CHILDREN),saved_resource_tree_passes=76,
          saved_capacity_contexts=8,saved_capacity_affected_contexts=8,numerical_methods=9,
          max_executed_groups=128,product_defined_group_cap=False),
          new_requirement_credit=2,newly_resolved_original_hold_keys=[],native_tests_rerun=0,
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
    result = {'status':'passed', 'scope':'Sealed source-versioned prefix plus numerical group capacity and Resource Tree construction',
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
                  'Only FRC E113 and All Others E120 are added. All other row records and 59 formal hold statuses remain unchanged.',
                  'Dynamic numerical group allocation and the exact 33 listed creation/configuration routes are verified at the retained source versions. Resource exhaustion and unrestricted resource operations remain separate.']}
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
