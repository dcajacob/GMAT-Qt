"""Join verified spacecraft model rendering and Earth MJ2000Eq integration."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import tarfile

import AuditNasaCoveragePrefix as prefix
import BuildNasaCoverageExtension as common
import NasaGuiCoverage as nasa
sha,load,require=common.sha,common.load,common.require
KEYS=['FRR!E44','FRR!E46','FRR!E144']
KIND='source-versioned-spacecraft-model-format-and-earth-fk5-integration'
GATES=['scientific_approval','technical_core_approval','full_current_build_qualified',
       'full_NASA_qualified','native_OS_input_qualified']


def qualify(repo,path,inventory):
    q=load(path)
    known={x['key']:x for x in inventory['requirements']}
    require(q['contract_kind']==KIND and q['qualified_keys']==KEYS
            and q['authority_sha256']==inventory['sources'] and all(q[k] is False for k in GATES),'Changed model or FK5 authority')
    for n,h in q['artifact_sha256'].items():require(sha(n)==h,'Changed model/FK5 stage input: '+n)
    c=load(q['conjunction'])
    require(c['original_rows']==[known[k] for k in KEYS]
            and c['model_context']==[known[k] for k in ['FRR!E44','FRR!E45','FRR!E46']],
            'Altered original spacecraft model family')
    for n,h in c['artifact_sha256'].items():require(sha(n)==h,'Changed actual execution: '+n)
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
    require(len(protected)==len(authorized)==1334 and all(q['proof_source_sha256'][n]==h for n,h in protected.items()),'Lost original protected inputs')
    for n in protected:
        if protected[n]!=authorized[n]:
            require(q['proof_source_sha256']['approved-observer-inputs/'+n]==authorized[n],'Lost authorized observer amendment')
    prior={x['original_source']['key']:x for x in load(q['prior_full_audit'])['rows']}
    require(all(prior[k]['status']=='verified-source-versioned' for k in ['FRR!E45','FRR!E209']),'Missing accepted 3DS or Earth orientation leaf')
    render=Path(c['pov_render'])
    x=load(render/'result.json');proof=x['proofs'][0];picker=load(render/'picker-result.json')
    require(x['status']=='passed' and x['returncode']==0 and proof['numeric_sentinel']==9
            and picker['accepted'] and picker['actual']==picker['expected'] and picker['actual'].endswith('.pov'),'Missing real POV model choice and execution')
    require(proof['pixel_checks']==load(render/'pixel-checks.json') and len(proof['rejections'])==3,'Missing raw pixel or Apply guard observations')
    require([x['phase'] for x in proof['pixel_checks']]==['pending-preview','reopened-preview','mission-render'],'Missing actual POV render phase')
    for phase in proof['pixel_checks']:
        r=phase['pixels'];w=phase['image_width'];h=phase['image_height']
        scale=55/math.hypot(20,10)
        factor=min(w,h)/(400*math.tan(math.radians(25)))
        require(phase['red_corner_pixels']>50 and phase['green_corner_pixels']>50
            and r['count']>100 and abs(r['right']-r['left']+1-40*scale*factor)<=3
            and abs(r['bottom']-r['top']+1-20*scale*factor)<=3
            and abs(r['centerX']-w/2)<=2 and abs(r['centerY']-h/2)<=2,
            'Native POV pixels fail independent shape or corner-color assertion')
        require((render/(phase['phase']+'.png')).read_bytes().startswith(b'\x89PNG\r\n\x1a\n'),'Missing actual framebuffer')
    require(all(load(p)['status']=='failed' for p in c['pov_attempts']),'Missing original POV attempt failures')
    geom=load(c['pov_geometry'])
    require(geom['registered_checks_passed']==1 and len(geom['observations'])==13,'Missing retained POV geometry checks')
    for x in geom['observations']:
        require(x['triangles']==0 if x['case']=='IntentionalEmpty' else
                abs(x['volume']-x['expected_volume'])<=x['relative_tolerance']*x['expected_volume'],
                'POV analytic geometry outside original bound')
    ds=Path(c['three_ds'])
    require(load(ds/'result.json')['status']=='passed' and all(x['asset']=='3ds' for x in load(ds/'pixel-checks.json')),
            'Missing actual 3DS chooser/renderer evidence')
    earth=load(c['earth_fk5_reduction'])
    require(earth['status']=='passed' and earth['qualified_keys']==['FRR!E209']
        and earth['summary']['native_states']==420 and earth['summary']['official_nutation_terms']==106,
        'Missing independently checked Earth FK5 frame reduction')
    cases=load(c['fk5_contexts'])
    require(len(cases)==16 and len({x['case'] for x in cases})==8 and {x['mode'] for x in cases}=={'unattended','visible'},
            'Missing original integration contexts')
    for item in cases:
        proof=load(item['proof']);native=proof['native'];independent=proof['independent']
        require(sha(item['proof'])==item['proof_sha256'] and sha(item['receipt'])==item['receipt_sha256']
            and sha(item['snapshot_manifest'])==item['snapshot_manifest_sha256']
            and proof['outcome']=='passed' and independent['passed'] and not independent['failures']
            and (native['internal_coordinate_system'],native['origin'],native['base_system'],native['axes'])
                ==('InternalEarthMJ2000Eq','Earth','FK5','MJ2000Eq')
            and native['propagator']=='RungeKutta89' and native['model']=='ForceModel'
            and all(abs(x-y)<=2e-9 for x,y in zip(native['integrator_times_seconds'],[120,240,240,120])),
            'Missing native Earth FK5 integration basis or independent dynamics')
    for n,anchors in c['integration_owners'].items():
        text=frozen[n].decode()
        require(all(a in text for a in anchors),'Unbound FK5 native route')
    return q,KEYS,dict(status='passed',qualified_keys=KEYS,whole_row_conjunction=True,
          component=dict(model_formats=2,pov_native_render_phases=3,pov_geometry_cases=13,
                         earth_fk5_integration_contexts=16,independent_frame_states=420),
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
    result = {'status':'passed', 'scope':'Sealed source-versioned prefix plus model format and Earth FK5 integration requirements',
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
                  'Only FRR E44, E46 and E144 are added. All other row records and 59 formal hold statuses remain unchanged.',
                  'Actual rendering joins the two listed model formats. The original Earth FK5 integration basis has native model/clock and independent reduction evidence. Other force, format or platform qualifications are not inferred.']}
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
