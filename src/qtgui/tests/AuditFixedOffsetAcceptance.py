"""Admit only original E52 after its complete FRR-35 fixed-offset conjunction."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import tarfile
import tempfile

import AuditFixedOffsetComponent as component
from BuildNasaCoverageExtension import load,require,sha

KIND='source-versioned-complete-fixed-offset-FRR35-conjunction'
KEYS=['FRR!E52']
CLAUSES={'all-original-frame-origin-cells','constant-DCM-and-angular-rate-composition',
         'offset-representation-boundaries','actual-GUI-rejection-atomicity',
         'model-cache-discard-history-and-Unicode-persistence',
         'native-metadata-clone-copy-and-assignment','repeat-Run-and-native-source-isolation',
         'original-aligned-controls-and-default-script','guide-exact-core-increment-and-ABI-limits'}


def structure(q,scope,inventory):
    known={r['key']:r for r in inventory['requirements']}
    require(q['contract_kind']==KIND and q['qualified_keys']==KEYS,'Partial/extra fixed-offset credit')
    require(q['authority_sha256']==inventory['sources'],'Original source authority changed')
    require(scope['candidate_row']==known[KEYS[0]],'Original fixed-offset obligation/fingerprint changed')
    require(scope['parent_and_siblings']==[known['FRR!E'+str(i)]for i in range(50,54)],'Attitude family context omitted')
    require(scope['original_origins']==[known['FRR!E'+str(i)]for i in range(428,433)]
            and scope['original_axes']==[known['FRR!E'+str(i)]for i in range(434,450)],
            'Any-coordinate-system conjunction narrowed')
    require(scope['original_cells']==80 and scope['mandatory_cells']==68 and scope['original_explicit_exclusions']==12
            and scope['implementation_exclusions']==0,'Original applicability partition changed')
    require(set(scope['clauses'])==CLAUSES and all(c['satisfied']and c['evidence']for c in scope['clauses'].values()),
            'Incomplete whole-row evidence conjunction')
    require(scope['whole_row_qualified']and scope['original_assertions_changed']==0
            and scope['failed_runs_retained']and scope['runtime_binary_copies']==0,
            'Failure/assertion/reuse scope changed')
    require(scope['excluded_credit']==['FRR!E50','FRR!E51','FRR!E53'], 'Sibling/parent credit generalized')
    for value in [q,scope]:
        require(all(value.get(k)is False for k in ['scientific_approval','technical_core_approval',
                    'full_current_build_qualified','full_NASA_qualified']), 'Wider qualification gate promoted')


def negative_controls(q,scope,inventory):
    rejected=[]
    for fault in ['extra-row','fingerprint','missing-origin','missing-axis','narrowed-cells',
                  'implementation-exclusion','missing-clause','lost-failure','assertions','ABI-approval']:
        bad_q,bad_scope=copy.deepcopy(q),copy.deepcopy(scope)
        if fault=='extra-row':bad_q['qualified_keys'].append('FRR!E50')
        elif fault=='fingerprint':bad_scope['candidate_row']['fingerprint']='0'*64
        elif fault=='missing-origin':bad_scope['original_origins'].pop()
        elif fault=='missing-axis':bad_scope['original_axes'].pop()
        elif fault=='narrowed-cells':bad_scope['mandatory_cells']=67
        elif fault=='implementation-exclusion':bad_scope['implementation_exclusions']=1
        elif fault=='missing-clause':bad_scope['clauses'].pop(next(iter(CLAUSES)))
        elif fault=='lost-failure':bad_scope['failed_runs_retained']=False
        elif fault=='assertions':bad_scope['original_assertions_changed']=1
        else:bad_q['technical_core_approval']=True
        try:structure(bad_q,bad_scope,inventory)
        except (AssertionError,ValueError):rejected.append(fault)
        else:raise AssertionError('Damaged fixed-offset admission accepted: '+fault)
    return rejected


def qualify(repo,path,inventory):
    q=load(path)
    for name,digest in q['artifact_sha256'].items():require(sha(name)==digest,'Changed fixed-offset admission input: '+name)
    scope=load(q['scope_review']);structure(q,scope,inventory)
    require(q['scope_review']in q['artifact_sha256'] and q['component_audit']in q['artifact_sha256']
            and q['reader_source_archive']in q['artifact_sha256'], 'Unbound fixed-offset evidence descriptor')
    with tarfile.open(q['reader_source_archive'])as archive:
        require(set(archive.getnames())==set(q['reader_source_sha256']), 'Frozen reader closure differs')
        for name,digest in q['reader_source_sha256'].items():
            require(hashlib.sha256(archive.extractfile(name).read()).hexdigest()==digest,'Frozen reader bytes changed')
    with tempfile.TemporaryDirectory(prefix='fixed-offset-admission-')as temporary:
        actual=component.replay(Path(q['component_stage']),Path(temporary)/'component.json')
    recorded=load(q['component_audit'])
    for result in [recorded,actual]:
        require(result['status']=='complete-original-fixed-offset-evidence-conjunction'
                and result['requirement']==KEYS[0] and result['fingerprint']==scope['candidate_row']['fingerprint']
                and result['whole_row_evidence_complete']and result['original_assertions_changed']==0
                and result['frame_component']['passed_profile_cells']==136
                and result['boundary_component']['passed_profile_cases']==18
                and result['boundary_component']['actual_GUI_rejections']==40
                and result['native_support_assertions']==163
                and result['runtime_binary_copies']==result['native_reruns']==0,
                'Complete actual fixed-offset evidence join failed')
    require(recorded['generic_argument']==actual['generic_argument'],'Generic composition review changed')
    require(len(negative_controls(q,scope,inventory))==10,'Damaged-model qualification controls incomplete')
    return q,KEYS


if __name__=='__main__':
    import NasaGuiCoverage as nasa
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('qualification',type=Path);p.add_argument('--output',type=Path);a=p.parse_args()
    repo=Path.cwd();q,keys=qualify(repo,a.qualification,nasa.inventory(repo))
    result=dict(status='passed',qualified_keys=keys,whole_row_conjunction=True,new_requirement_credit=1,
                prior_requirements_verified=1052,resulting_requirements_verified=1053,original_requirements=1158,
                original_hold_identities=59,historical_operations_verified=1816,historical_operations_total=2367,
                damaged_admission_controls_rejected=10,native_tests_rerun=0,runtime_binary_copies=0,
                scientific_approval=False,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)
    if a.output:
        with a.output.open('x')as stream:json.dump(result,stream,indent=2);stream.write('\n')
    print(json.dumps(result))
