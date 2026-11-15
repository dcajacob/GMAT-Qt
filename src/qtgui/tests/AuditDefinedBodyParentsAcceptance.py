"""Verify the 15-by-22 body-parameter join without executing any native binary."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import tarfile

import AuditNasaCoveragePrefix as prefix
import BuildNasaCoverageExtension as common
import NasaGuiCoverage as nasa

sha,load,require=common.sha,common.load,common.require
KEYS=['FRR!E501','FRR!E502']
KIND='source-versioned-defined-celestial-body-parameter-conjunction'
CHILDREN=[f'FRR!E{i}' for i in range(503,525)]
PARAMETERS=['Altitude','BetaAngle','C3Energy','EA','ECC','Energy','HMAG','Latitude','Longitude','LST',
            'MA','MHA','MM','OrbitPeriod','RadApo','RadPer','RMAG','SemilatusRectum','SMA','TA','VelApoapsis','VelPeriapsis']
BODIES=['Sun','Mercury','Venus','Earth','Luna','Mars','Jupiter','Saturn','Uranus','Neptune','Pluto',
        'UserMoon','UserAsteroid','UserComet','UserPlanet']
GATES=['scientific_approval','technical_core_approval','full_current_build_qualified',
       'full_NASA_qualified','native_OS_input_qualified']


def qualify(repo,path,inventory):
    q=load(path)
    known={x['key']:x for x in inventory['requirements']}
    require(q['contract_kind']==KIND and q['qualified_keys']==KEYS and q['authority_sha256']==inventory['sources']
            and all(q[k] is False for k in GATES), 'Changed body requirement authority')
    for n,h in q['artifact_sha256'].items():
        require(sha(n)==h,'Changed body admission artifact: '+n)
    c=load(q['conjunction'])
    require(c['original_rows']==[known[k] for k in KEYS]
         and c['original_children']==[known[k] for k in CHILDREN]
         and c['original_FRR16_context']==[x for x in inventory['requirements']
              if x['sheet']=='FRR' and x['declared_id'].startswith('FRR-16.')],
         'Altered body context')
    decision=load(c['decision'])
    require(decision['user_answer']=='Defined combinations plus explicit rejection'
         and decision['does_not_waive_existing_failure_receipts'] and c['no_HA_frame_credit'],
         'Body decision or separate frame requirement changed')
    for n,h in c['artifact_sha256'].items():
        require(sha(n)==h,'Changed native body evidence: '+n)
    prior={x['original_source']['key']:x for x in load(q['prior_full_audit'])['rows']}
    require(all(prior[k]['status']=='verified-source-versioned' for k in CHILDREN),
         'Unverified body parameter prerequisite')
    for archive,ids in [('reader_source_archive','reader_source_sha256'),('proof_source_archive','proof_source_sha256')]:
        with tarfile.open(q[archive]) as tar:
            require(set(tar.getnames())==set(q[ids]),'Incomplete immutable source partition')
            for n,h in q[ids].items():
                raw=tar.extractfile(n).read()
                require(hashlib.sha256(raw).hexdigest()==h and not raw.startswith(b'\x7fELF'),'Changed source capsule member')
    protected=load(q['protected_input_baseline'])
    authorized=load(q['authorized_input_manifest'])
    require(len(protected)==len(authorized)==1334 and all(q['proof_source_sha256'][n]==h for n,h in protected.items()),'Original input preimages lost')
    for n in protected:
        if protected[n]!=authorized[n]:
            require(q['proof_source_sha256']['approved-observer-inputs/'+n]==authorized[n],'Authorized observer amendment lost')
    matrix=c['body_parameter_matrix']
    require(set(matrix)==set(BODIES) and all(set(row)==set(PARAMETERS) for row in matrix.values()),'Missing body or parameter route')
    cases={Path(n) for d in matrix.values() for n in d.values()
           if '/coverage95-20261110-defined-body-parameters-v1/headless-' in n and 'sun-beta' not in n}
    count=0
    for d in cases:
        plan,result=load(d/'plan.json'),load(d/'result.json')
        actual=[float(v) for v in (d/'values.txt').read_text().split()]
        require(actual==result['native'] and result['returncode']==0
            and result['script_sha256']==sha(d/'mission.script') and result['plan_sha256']==sha(d/'plan.json')
            and len(actual)==len(plan['expected'])==len(plan['absolute_bounds']),'Wrong or truncated executed body result')
        expected,bounds=plan['expected'],plan['absolute_bounds']
        if d.name=='Neptune' and 'geodetic' in d.parent.name:
            correction=load(c['neptune_correction'])
            w=253.18-.48*math.sin(math.radians(357.85))
            require(correction['expected']==expected[:3]+[w,(w+170)%360]
                and correction['absolute_bounds']==bounds and correction['native_rerun'] is False,
                'Unbound or weakened Neptune oracle correction')
            expected=correction['expected']
        else:
            require(result['status']=='passed','Unqualified body case')
        require(all(math.isfinite(a) and abs(a-e)<=t for a,e,t in zip(actual,expected,bounds)),
             'Independent body parameter mismatch: '+str(d))
        count+=len(actual)
    require(count==745 and len(cases)==41,'Unexpected body coverage or missing raw rows')
    negative=Path(matrix['Sun']['BetaAngle'])
    res=load(negative/'result.json')
    require(res['status']=='passed' and res['returncode']==1 and res['diagnostic_observed']
        and 'Rvector3::Normalize' in (negative/'console.log').read_text(), 'Undefined Sun BetaAngle did not reject')
    for n in ['undefined-beta.txt','successor.txt']:
        f=negative/n
        require(not f.exists() or not f.read_text().strip(),'Undefined angle emitted a value or advanced the script')
    for p in c['reused_earth_and_mars']:
        d=Path(p)
        x=load(d/'result.json')
        require(x['status']=='passed' and x['returncode']==0,'Saved Earth/Mars witness did not pass')
    for p in c['preserved_new_failures']:
        require(load(Path(p)/'result.json')['status']=='failed','New original-bound failure not preserved')
    require(len(c['preserved_new_failures'])==6 and all(load(p)['status']=='passed' for p in c['reused_body_increments']),
        'Missing earlier body evidence or failures')
    require(load(c['lunar_dependent'])['status']=='passed','Default lunar orientation regression unresolved')
    return q,KEYS,dict(status='passed',qualified_keys=KEYS,whole_row_conjunction=True,
          component=dict(body_routes=15,body_parameters=22,defined_routes=329,undefined_sun_beta_rejected=True,
                new_numeric_scenarios=41,raw_numeric_witnesses=count,original_children=22,preserved_new_failures=6),
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
    result = {'status':'passed', 'scope':'Sealed source-versioned prefix plus defined celestial-body parameter parent conjunction',
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
                  'Only FRR E501 and E502 are added. All other row records and 59 formal hold statuses remain unchanged.',
                  'The original body inventory and all 22 original clauses have actual routed numerical evidence; Sun BetaAngle is explicitly rejected. Separate any-frame, source-provider and unsupported conic states are not inferred.']}
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
