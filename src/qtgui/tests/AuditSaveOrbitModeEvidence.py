"""Audit remaining orbit display persistence, retaining original science limits."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import tempfile

import NasaGuiCoverage as nasa
from AuditCapiAcceptance import load, require, sha
from AuditCentralBodyPropagationAcceptance import read_run
from AuditSaveResourceEvidence import check_case
from AuditSaveOrbitEvidence import audit as parent_audit, finite_number

FIXTURE='src/qtgui/tests/NasaSaveOrbitModeCases.json'
PREFIX='QtGui.NasaSaveOrbitMode'


def frozen_module(repo, sources, name):
    namespace={'__file__':str(repo/name),'__name__':'frozen_acceptance_oracle'}
    exec(compile(sources[name],name,'exec'),namespace)
    return namespace


def original_and_truth(repo, sources, case):
    matrix=json.loads(sources[case['oracle_source']]);original=next(c for c in matrix['cases'] if c['key']==case['oracle_case'])
    if case['oracle_kind']=='spheroid-normal':
        require(case['oracle_source']=='src/qtgui/tests/NasaPlanetodeticCases.json', 'Substituted spheroid fixture')
        oracle=frozen_module(repo,sources,'src/qtgui/tests/PlanetodeticOracle.py')
        truth=oracle['reference'](original['values'],original['flattening'],original['equatorial_radius'])['cartesian']
    else:
        require(case['oracle_kind']=='prototype-parity-conic' and case['oracle_source']=='src/qtgui/tests/NasaBrouwerDefinitionCases.json', 'Substituted mean-state fixture')
        for reference,digest in matrix['reference_sha256'].items():
            if reference!='doc/help/src/Resource_SpacecraftOrbitState.xml':
                require(hashlib.sha256(sources[reference]).hexdigest()==digest, 'Original prototype/adapter identity changed')
        oracle=frozen_module(repo,sources,'src/qtgui/tests/BrouwerPrototypeOracle.py')
        filename='BROLYD_short.m' if original['key']=='BrouwerMeanShort' else 'BROLYD.m'
        with tempfile.TemporaryDirectory(prefix='save-brouwer-oracle-') as directory:
            path=Path(directory)/filename;path.write_bytes(sources['prototype/StateConv/'+filename])
            converter=oracle['adapt'](path)[0];osculating=converter(original['values'])
        require(all(abs(a-b)<1e-9 for a,b in zip(osculating,original['osculating_keplerian_MA'])), 'Changed retained NASA prototype mapping')
        truth=oracle['cartesian'](osculating,original['mu'])
    inputs=dict(re.findall(r'^Subject\.(\w+) = ([^;]+);$',case['setup'],re.M))
    require(all(float(inputs[name])==value for name,value in zip(original['names'],original['values'])), 'Changed original six orbit input literals')
    return original,truth


def validate_capture(case, original, truth, records, phase):
    witnesses=case['cartesian_witnesses'];require(len(records)==len(witnesses)==1, 'Missing/extra Cartesian mode capture: '+phase)
    record=records[0];witness=witnesses[0];expected=original.get('expected_cartesian',original.get('cartesian'))
    require(set(record)=={'object','coordinate_system','actual','expected','position_bound_km','velocity_bound_km_s'}
            and record['object']==witness['object']=='Subject' and record['coordinate_system']==original.get('frame','EarthMJ2000Eq'), 'Substituted native mode object/frame')
    require(record['expected']==witness['expected']==expected and len(record['actual'])==len(expected)==len(truth)==6
            and all(finite_number(v) for v in record['actual']+expected+truth), 'Incomplete/nonfinite Cartesian mode state')
    for key,source in [('position_bound_km','position_tolerance_km'),('velocity_bound_km_s','velocity_tolerance_km_s')]:
        require(record[key]==witness[key]==original[source] and finite_number(record[key]) and record[key]>0, 'Changed original Cartesian mode bound')
    for i,(actual,literal,reference) in enumerate(zip(record['actual'],expected,truth)):
        require(abs(literal-reference)<=(1e-8 if i<3 else 1e-11), 'Fixture differs from recorded-source oracle')
        require(abs(actual-reference)<=record['position_bound_km' if i<3 else 'velocity_bound_km_s'], 'Native state differs from recorded-source oracle: '+phase)


def audit(repo,path):
    evidence=load(path)
    require(evidence['contract_kind']=='source-versioned-bounded-Save-orbit-mode-conjunction' and evidence['qualified_keys']==[]
            and evidence['goal_complete'] is False and evidence['whole_FRC_E132_qualified'] is False, 'Bounded mode evidence silently awards complete credit')
    inventory=nasa.inventory(repo);require(evidence['authority_sha256']==inventory['sources'], 'NASA authority changed')
    require(sha(evidence['parent_evidence'])==evidence['parent_evidence_sha256'] and sha(evidence['parent_seal'])==evidence['parent_seal_sha256'], 'Prior142-case evidence changed')
    for name,digest in evidence['artifact_sha256'].items():require(sha(name)==digest, 'Changed mode artifact: '+name)
    reconciliation=load(evidence['reference_reconciliation'])
    require(evidence['reference_reconciliation'] in evidence['artifact_sha256'] and reconciliation['original_copy'] in evidence['artifact_sha256']
            and sha(reconciliation['original_copy'])==reconciliation['original_copy_sha256']==reconciliation['original_fixture_reference_sha256'], 'Original guide reference missing/changed')
    parent=parent_audit(repo,evidence['parent_evidence']);require(parent['cumulative_cases']==142 and parent['cumulative_contexts']==284 and parent['new_requirement_credit']==[], 'Prior evidence conjunction changed')
    matrix=load(evidence['fixture']);cases={c['key']:c for c in matrix['cases']}
    require(set(cases)=={'BrouwerMeanShort','BrouwerMeanLong','PlanetodeticSphere','PlanetodeticEarthlike','PlanetodeticOblate','PlanetodeticSouthern'} and len(matrix['cases'])==6, 'Missing/duplicate orbit mode')
    runs={name:read_run(repo,spec,inventory,evidence['artifact_sha256'],{}) for name,spec in evidence['runs'].items()}
    require(set(evidence['selected'])=={'headless','visible'}, 'Required mode omitted');count=0
    for mode,selected in evidence['selected'].items():
        require(len(selected)==6 and {e['case'] for e in selected}==set(cases), 'Missing/duplicate selected mode case')
        for entry in selected:
            receipt,results,sources=runs[entry['run']];name=PREFIX+entry['case'];case=cases[entry['case']]
            require(receipt['mode']==mode and entry['test']==name, 'Substituted mode/test')
            original,truth=original_and_truth(repo,sources,case);work=Path(results[name]['log']).parent
            if case['oracle_kind']=='prototype-parity-conic':
                require(reconciliation['original_fixture_reference_sha256']==json.loads(sources[case['oracle_source']])['reference_sha256']['doc/help/src/Resource_SpacecraftOrbitState.xml'], 'Wrong historical guide for frozen fixture')
            count+=check_case(case,results[name],sources,evidence['artifact_sha256'],fixture_path=FIXTURE)
            for phase in ('runtime','reloaded'):
                path=work/(phase+'-cartesian.json');require(str(path) in evidence['artifact_sha256'], 'Native Cartesian mode capture omitted')
                validate_capture(case,original,truth,load(path),phase)
    require(evidence['remaining_gaps'], 'Remaining obligations hidden')
    return dict(status='orbit-mode-Save-evidence-passed; complete any-Resource obligation open',new_cases=6,new_contexts=12,prior_cases_replayed=0,prior_cases_reused=142,cumulative_cases=148,cumulative_contexts=296,
                new_writable_property_comparisons=count,cumulative_writable_property_comparisons=count+parent['cumulative_writable_property_comparisons'],
                independent_spheroid_native_captures=16,prototype_parity_conic_native_captures=8,Cartesian_component_checks=144,
                original_row='FRC!E132',new_requirement_credit=[],goal_complete=False)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3]);p.add_argument('--evidence',type=Path,required=True);a=p.parse_args();print(json.dumps(audit(a.repo.resolve(),a.evidence)))
