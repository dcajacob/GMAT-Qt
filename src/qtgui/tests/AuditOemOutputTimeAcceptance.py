"""Admit only original E380's complete five-system OEM output option."""
import argparse
import copy
import hashlib
import importlib
import json
from pathlib import Path
import sys
import tarfile
import tempfile

from BuildNasaCoverageExtension import load,require,sha

KIND='source-versioned-complete-OEM-output-time-systems'
KEYS=['FRR!E380']
SYSTEMS=['A1','TAI','UTC','TT','TDB']
CLAUSES={'five-original-output-systems','actual-GUI-and-script-lifecycle','metadata-and-every-data-epoch',
         'input-boundary-format-independent','UTC-default-and-creation','native-copy-consistency-and-rounding'}
FROZEN=['AuditOemOutputTime.py','RunOemOutputTime.py','RunOemOutputTimeSupport.py','OemOutputTimeOracle.py',
        'CcsdsOemB1Oracle.py','RunOemCreationClock.py','NasaOemRecordSupportCases.json']


def structure(q,scope,inventory):
    known={r['key']:r for r in inventory['requirements']}
    require(q['contract_kind']==KIND and q['qualified_keys']==KEYS,'Partial/extra OEM output-time credit')
    require(q['authority_sha256']==inventory['sources'],'Original authority changed')
    require(scope['candidate_row']==known[KEYS[0]],'Original E380 obligation/fingerprint changed')
    require(scope['original_time_context']==[known['FRR!E'+str(i)]for i in range(17,23)],'Original FRR2.2 system context omitted')
    require(scope['ephemeris_option_context']==[known['FRR!E'+str(i)]for i in range(377,382)],'Original parent/option context omitted')
    require(scope['required_systems']==SYSTEMS and scope['eligibility_exclusions']==[],'Required clock omitted or excluded')
    require(set(scope['clauses'])==CLAUSES and all(r['satisfied']and r['evidence']for r in scope['clauses'].values()),'Incomplete whole-row conjunction')
    require(scope['whole_row_qualified']and scope['actual_output_option']and not scope['input_EpochFormat_substituted'],'Input alias substituted for output option')
    require(scope['A1_forward_interface_only']and scope['generic_A1_oracle_gap_retained'],'A1 convention silently invented or generic oracle changed')
    require(scope['excluded_credit']==['FRR!E374','FRR!E375','FRR!E377','FRR!E381','FRR!E404','FRR!E406'],'Parent/science/format credit generalized')
    require(scope['prior_NarrowAtLast_failure_retained']and scope['original_assertions_changed']==0,'Prior science/assertion waiver inferred')
    require(all(q.get(k)is False and scope.get(k)is False for k in ['scientific_approval','technical_core_approval','full_current_build_qualified','full_NASA_qualified','MATLAB_bundle_qualified']),'Qualification exceeds exact row')


def negative_controls(q,scope,inventory):
    controls=[]
    mutations=[('missing-A1',lambda a,b:b['required_systems'].remove('A1')),
        ('missing-TDB-context',lambda a,b:b['original_time_context'].pop()),
        ('input-alias-substitution',lambda a,b:b.update(input_EpochFormat_substituted=True)),
        ('missing-output-epochs-clause',lambda a,b:b['clauses'].pop('metadata-and-every-data-epoch')),
        ('parent-extra-credit',lambda a,b:a['qualified_keys'].append('FRR!E377')),
        ('retroactive-A1-convention',lambda a,b:b.update(A1_forward_interface_only=False)),
        ('waived-science-failure',lambda a,b:b.update(prior_NarrowAtLast_failure_retained=False)),
        ('changed-original-fingerprint',lambda a,b:b['candidate_row'].update(fingerprint='0'*64)),
        ('full-NASA-overclaim',lambda a,b:a.update(full_NASA_qualified=True))]
    for name,mutate in mutations:
        a,b=copy.deepcopy(q),copy.deepcopy(scope);mutate(a,b)
        try:structure(a,b,inventory)
        except (ValueError,AssertionError):controls.append(name)
        else:raise ValueError('Admission reader accepted damaged scope: '+name)
    return controls


def frozen_component(repo,component,q):
    manifest=load(component/'evidence-manifest-v1.json');receipt=load(component/'stage-receipt-v1.json')
    require(receipt['manifest_sha256']==sha(component/'evidence-manifest-v1.json'),'Component seal chain changed')
    for n,h in manifest['artifact_sha256'].items():
        require(not(repo/n).is_symlink()and sha(repo/n)==h,'Component artifact changed: '+n)
    for n,h in manifest['external_immutable_sha256'].items():require(sha(repo/n)==h,'Prior immutable evidence changed: '+n)
    source=load(component/'final-source-sha256-v1.json');archive=component/'final-sources-v1.tar.gz'
    require(sha(archive)==manifest['final_source_archive_sha256'],'Frozen component source archive changed')
    names=[Path(n).stem for n in FROZEN if n.endswith('.py')]
    previous={n:sys.modules.pop(n)for n in names if n in sys.modules}
    try:
        with tempfile.TemporaryDirectory(prefix='gmat-oem-time-admission-readers-')as temporary:
            with tarfile.open(archive)as tar:
                for n in FROZEN:
                    key='src/qtgui/tests/'+n;data=tar.extractfile(key).read()
                    require(hashlib.sha256(data).hexdigest()==source[key],'Frozen reader/resource changed: '+key)
                    (Path(temporary)/n).write_bytes(data)
            sys.path.insert(0,temporary)
            try:
                reader=importlib.import_module('AuditOemOutputTime')
                result=reader.audit(repo,component)
            finally:sys.path.pop(0)
    finally:
        for n in names:sys.modules.pop(n,None)
        sys.modules.update(previous)
    require(result==load(component/'component-audit-v2.json'),'Frozen component replay differs')
    require(result['required_systems']==SYSTEMS and result['new_time_cases']==16 and result['time_matrix_passes']==32,'Incomplete actual five-clock matrix')
    require(result['committed_switch_passes']==2 and result['affected_UTC_control_passes']==4 and result['native_writer_systems_passed']==5,'Missing lifecycle/default/native clock proof')
    require(result['original_driver_catalog_oracle_assertion_changes']==0 and result['prior_NarrowAtLast_scientific_failure_retained'],'Original assertion/science record changed')
    return result


def qualify(repo,path,inventory):
    repo=Path(repo);q=load(path)
    for name,digest in q['artifact_sha256'].items():
        require(not Path(name).is_symlink()and sha(name)==digest,'Changed admission artifact: '+name)
    require(q['scope_review']in q['artifact_sha256'],'Unbound exact scope review')
    scope=load(q['scope_review']);structure(q,scope,inventory)
    component=Path(q['component_stage'])
    for name in ['evidence-manifest-v1.json','stage-receipt-v1.json','seal-verification-v1.json','final-sources-v1.tar.gz','final-source-sha256-v1.json','component-audit-v2.json']:
        require(str(component/name)in q['artifact_sha256'],'Unbound component evidence: '+name)
    require(q['reader_source_archive']in q['artifact_sha256'],'Unbound admission reader package')
    with tarfile.open(q['reader_source_archive'])as tar:
        require(set(tar.getnames())==set(q['reader_source_sha256']),'Admission reader partition differs')
        require(all(hashlib.sha256(tar.extractfile(n).read()).hexdigest()==h for n,h in q['reader_source_sha256'].items()),'Admission reader bytes differ')
    prior=load(q['prior_coverage']);overlay=load(q['prior_overlay'])
    require(prior['coverage']['requirements']['verified']==1051 and prior['coverage']['historical_cases']['verified']==1816,'Different source-versioned prefix')
    require(len(overlay['reviewed_source_rows'])==139 and len(overlay['original_hold_keys_retained'])==59,'Original closure/hold inventory lost')
    require(scope['previous_closure_record']==next(r for r in overlay['reviewed_source_rows']if r['key']==KEYS[0]),'Old E380 record changed instead of preserved')
    result=frozen_component(repo,component,q)
    rejected=negative_controls(q,scope,inventory);require(len(rejected)==9,'Damaged-model admission check incomplete')
    return q,KEYS


if __name__=='__main__':
    import NasaGuiCoverage as nasa
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('qualification',type=Path);p.add_argument('--output',type=Path);a=p.parse_args()
    repo=Path.cwd();q,keys=qualify(repo,a.qualification,nasa.inventory(repo))
    result=dict(status='passed',qualified_keys=keys,complete_original_time_systems=SYSTEMS,
        damaged_scope_models_rejected=9,native_tests_rerun=0,runtime_binary_copies=0,
        scientific_approval=False,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False,MATLAB_bundle_qualified=False)
    if a.output:
        require(not a.output.exists(),'Do not overwrite admission audit');a.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result))
