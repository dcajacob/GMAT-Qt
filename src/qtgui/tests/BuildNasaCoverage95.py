"""Extend the sealed85-percent union without replaying established tests.

All original source rows remain in the denominator. Only independently admitted
complete additional obligations can receive credit; a target is never evidence.
"""
import argparse
import hashlib
import json
from pathlib import Path
import tarfile

import NasaGuiCoverage as nasa
import BuildNasaCoverageExtension as prior

sha,load,require=prior.sha,prior.load,prior.require


def baseline(repo,path):
    seal=load(path)
    if seal.get('kind')=='sealed-95-percent-source-versioned-prefix':
        from AuditNasaCoveragePrefix import baseline as historical_prefix
        return historical_prefix(repo,path)
    require(seal['kind']=='sealed-85-percent-prefix-for-95-percent-extension' and seal['target_requirements']==1101,'Changed95-percent target or baseline kind')
    inventory=nasa.inventory(repo)
    require(seal['authority_sha256']==inventory['sources'] and len(inventory['requirements'])==1158 and len(inventory['historical_cases'])==2367,'Original authority/denominator changed')
    for name,digest in seal['artifact_sha256'].items():require(sha(name)==digest,'Changed sealed85-percent artifact: '+name)
    require(sha(seal['baseline_report'])==seal['baseline_report_sha256'] and sha(seal['baseline_audit'])==seal['baseline_audit_sha256'],'Changed sealed report/audit')
    audit=load(seal['baseline_audit']);report=load(seal['baseline_report']);root=Path(seal['baseline_audit']).parent
    require(audit['status']=='passed' and audit['goal_complete'] and audit['coverage']==report['coverage']==seal['coverage'],'Unproved85-percent prefix')
    for name,digest in audit['report_sha256'].items():require(sha(root/name)==digest,'Changed baseline completion evidence: '+name)
    archive=root/'eleventh-increment-sources.tar.gz';require(sha(archive)==audit['source_archive_sha256'],'Changed baseline source archive')
    with tarfile.open(archive) as bundle:
        for name,digest in audit['source_sha256'].items():require(hashlib.sha256(bundle.extractfile(name).read()).hexdigest()==digest,'Changed frozen baseline source')
    # Reuse the exact previously audited union; this checks original source
    # identity and membership rather than repeating any legacy native execution.
    _,old_inventory,keys,_=prior.sealed_baseline(repo,Path(report['baseline']))
    require(old_inventory['sources']==inventory['sources'],'Earlier original authority differs')
    known={kind:{r['key']:r for r in inventory[kind]} for kind in keys}
    for row in report['additions']:
        kind=row['kind'];key=row['key']
        require(row['fingerprint']==known[kind][key]['fingerprint'] and row['source_text']==known[kind][key]['text'],'Changed inherited complete source obligation')
        require(sha(row['qualification'])==row['qualification_sha256'],'Changed inherited qualification')
        keys[kind]=prior.merge_keys(keys[kind],[key],known[kind])
    require(len(keys['requirements'])==985 and len(keys['historical_cases'])==1816,'Sealed numerator changed')
    require(seal['remaining_original_rows']==report['remaining_original_rows'],'Original remaining scope changed')
    for kind in keys:require({r['key'] for r in seal['remaining_original_rows'][kind]}==set(known[kind])-keys[kind],'Missing original remaining source rows')
    return seal,inventory,keys


def qualify(repo,path,inventory):
    kind=load(path)['contract_kind']
    if kind=='source-versioned-complete-OEM-output-time-systems':
        from AuditOemOutputTimeAcceptance import qualify as admission
        return admission(repo,path,inventory)
    if kind=='source-versioned-complete-MATLAB-Assignment-RHS':
        from AuditMatlabAssignmentAcceptance import qualify as admission
        return admission(repo,path,inventory)
    if kind=='source-versioned-MATLAB-high-level-C-interface':
        from AuditMatlabCInterfaceAcceptance import qualify as admission
        return admission(repo,path,inventory)
    if kind=='source-versioned-Resource-Tree-MATLAB-interface-open-close':
        from AuditMatlabInterfaceTreeAcceptance import qualify as admission
        return admission(repo,path,inventory)
    if kind=='source-versioned-complete-fmincon-property-conjunction':
        from AuditFminconOptionAcceptance import qualify as admission
        return admission(repo,path,inventory)
    if kind=='source-versioned-fmincon-actual-iterations-output-and-limits':
        from AuditFminconIterationAcceptance import qualify as admission
        return admission(repo,path,inventory)
    if kind=='source-versioned-fmincon-GUI-selection-and-Vary-settings':
        from AuditFminconVaryAcceptance import qualify as admission
        return admission(repo,path,inventory)
    if kind=='source-versioned-fmincon-Resource-Tree-creation-and-configuration':
        from AuditFminconResourceAcceptance import qualify as admission
        return admission(repo,path,inventory)
    if kind=='source-versioned-MATLAB-Mission-Tree-command-creation-and-configuration':
        from AuditMatlabMissionCallAcceptance import qualify as admission
        return admission(repo,path,inventory)
    if kind=='source-versioned-MATLAB-resource-creation-and-function-invocation':
        from AuditMatlabRowAcceptance import qualify as admission
        return admission(repo,path,inventory)
    if kind=='source-versioned-complete-default-body-IAU2002-orientation':
        from AuditDefaultBodyOrientationAcceptance import qualify as admission
        return admission(repo,path,inventory)
    if kind=='source-versioned-Earth-FK5-orientation':
        from AuditEarthFk5Acceptance import qualify as admission
        return admission(repo,path,inventory)
    if kind=='source-versioned-exact-MSIS86-atmosphere':
        from AuditMsise86Acceptance import qualify as admission
        return admission(repo,path,inventory)
    if kind=='source-versioned-lunar-DE405-IAU2002-conjunction':
        from AuditLunarOrientationAcceptance import qualify as admission
        return admission(repo,path,inventory)
    if kind=='source-versioned-Vary-unsupported-setting-warning':
        from AuditVaryWarningAcceptance import qualify as admission
        return admission(repo,path,inventory)
    if kind=='source-versioned-qualification-source-relocation':
        from AuditSourceRelocatedAcceptance import qualify as admission
        return admission(repo,path,inventory)
    if kind=='source-versioned-complete-inertial-conic-definition-family':
        from AuditInertialDefinitionAcceptance import qualify as admission
        return admission(repo,path,inventory)
    if kind=='source-versioned-complete-frame-state-definition-family':
        from AuditFrameStateAcceptance import qualify as admission
        return admission(repo,path,inventory)
    if kind=='source-versioned-complete-named-frame-consumer-conjunction':
        from AuditNamedFrameAcceptance import qualify as admission
        return admission(repo,path,inventory)
    if kind=='source-versioned-complete-central-body-propagation-family':
        from AuditCentralBodyPropagationAcceptance import qualify as admission
        return admission(repo,path,inventory)
    if kind=='source-versioned-complete-Minimize-scalar-family':
        from AuditMinimizeScalarAcceptance import qualify as admission
        return admission(repo,path,inventory)
    if kind=='source-versioned-MATLAB-file-location-conjunction':
        from AuditMatlabPathAcceptance import qualify as admission
        return admission(repo,path,inventory)
    if kind=='source-versioned-complete-calculation-family':
        from AuditCalculationFamilyAcceptance import qualify as admission
        return admission(repo,path,inventory)
    if kind=='source-versioned-console-GUI-interface-exception':
        from AuditConsoleInterfaceAcceptance import qualify as admission
        return admission(repo,path,inventory)
    if kind=='source-versioned-public-C-API-spacecraft-epoch':
        from AuditCapiSpacecraftEpochAcceptance import qualify as admission
        return admission(repo,path,inventory)
    if kind=='source-versioned-optional-cached-A1-summary':
        from AuditSummaryA1Acceptance import qualify as admission
        return admission(repo,path,inventory)
    if kind=='source-versioned-complete-ScriptEvent-supported-command-conjunction':
        from AuditScriptEventInventoryAcceptance import qualify as admission
        return admission(repo,path,inventory)
    if kind=='source-versioned-station-rise-set-selection':
        from AuditStationEventAcceptance import qualify as admission
        return admission(repo,path,inventory)
    if kind=='source-versioned-multiple-spacecraft-contact-selection':
        from AuditMultiTargetContactAcceptance import qualify as admission
        return admission(repo,path,inventory)
    if kind=='source-versioned-C-API-spacecraft-configuration':
        from AuditCapiConfigurationAcceptance import qualify as admission
        return admission(repo,path,inventory)
    raise ValueError('Unreviewed95-percent contract kind: '+kind)


def build(repo,seal_path,qualifications,output):
    seal,inventory,keys=baseline(repo,seal_path);original={k:set(v) for k,v in keys.items()};known={k:{r['key']:r for r in inventory[k]} for k in keys};additions=[]
    for path in qualifications:
        q,qualified=qualify(repo,path,inventory)
        require(len(qualified)==len(set(qualified)) and set(qualified)<=set().union(*(set(v) for v in known.values())),'Unknown/duplicate source credit')
        require(all(q.get(k) is False for k in ('technical_core_approval','full_current_build_qualified','full_NASA_qualified')),'New qualification exceeds its scope')
        for kind in keys:
            incoming=[key for key in qualified if key in known[kind]];keys[kind]=prior.merge_keys(keys[kind],incoming,known[kind]);additions.extend(dict(key=key,kind=kind,fingerprint=known[kind][key]['fingerprint'],source_text=known[kind][key]['text'],qualification=str(path),qualification_sha256=sha(path),status='verified-source-versioned') for key in incoming)
    coverage={kind:dict(verified=len(keys[kind]),total=len(known[kind]),percent=round(100*len(keys[kind])/len(known[kind]),1),new_verified=len(keys[kind])-len(original[kind])) for kind in keys}
    remaining={kind:[row for row in seal['remaining_original_rows'][kind] if row['key'] not in keys[kind]] for kind in keys}
    result=dict(status='passed',scope='95-percent campaign: sealed source-versioned prefix plus complete independently admitted original obligations.',baseline=str(seal_path),baseline_sha256=sha(seal_path),authority_sha256=inventory['sources'],coverage=coverage,target_requirements=1101,target_reached=len(keys['requirements'])>=1101,requirements_still_needed=max(0,1101-len(keys['requirements'])),additions=additions,remaining_counts={kind:len(rows) for kind,rows in remaining.items()},remaining_original_rows=remaining,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False,limits=['Original1158 requirement and2367 historical denominators unchanged.','Source-versioned capability evidence is not current complete binary or independent core/scientific approval.','Unknown, conflicted, unavailable and unexecuted original obligations remain explicit; none earns credit by deferral or target arithmetic.'])
    Path(output).write_text(json.dumps(result,indent=2)+'\n');return result


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3]);p.add_argument('--baseline',type=Path,required=True);p.add_argument('--qualification',type=Path,action='append',default=[]);p.add_argument('--output',type=Path,required=True);a=p.parse_args();r=build(a.repo.resolve(),a.baseline,a.qualification,a.output);print(json.dumps({k:r[k] for k in ('status','coverage','requirements_still_needed','target_reached')}))
