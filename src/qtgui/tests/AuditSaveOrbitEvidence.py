"""Audit Save/rebuild of existing orbit definitions against independent conic truth.

The 11 selected definitions add bounded persistence evidence. Other display
representations, anomaly modes, frames, domains and full any-Resource remain open.
"""
import argparse
import json
import math
from pathlib import Path

import NasaGuiCoverage as nasa
from AuditCapiAcceptance import load, require, sha
from AuditCentralBodyPropagationAcceptance import read_run
from AuditSaveResourceEvidence import check_case
from AuditSaveFileAttitudeEvidence import audit as parent_audit

FIXTURE = 'src/qtgui/tests/NasaSaveOrbitCases.json'
PREFIX = 'QtGui.NasaSaveOrbit'


def finite_number(value):
    return isinstance(value, (int, float)) and not isinstance(value, bool) and math.isfinite(value)


def independent_conic(case):
    elements = case['canonical_keplerian'];mu = case['mu']
    require(len(elements) == 6 and all(finite_number(v) for v in elements)
            and finite_number(mu) and mu > 0, 'Invalid independent conic inputs')
    a,e,inc,node,peri,nu = elements
    inc,node,peri,nu = (math.radians(v) for v in (inc,node,peri,nu))
    p=a*(1-e*e);require(p > 0 and 1+e*math.cos(nu) > 0, 'Invalid independent conic domain')
    radius=p/(1+e*math.cos(nu));speed=math.sqrt(mu/p)
    first=[math.cos(node)*math.cos(peri)-math.sin(node)*math.sin(peri)*math.cos(inc),
           math.sin(node)*math.cos(peri)+math.cos(node)*math.sin(peri)*math.cos(inc),
           math.sin(peri)*math.sin(inc)]
    second=[-math.cos(node)*math.sin(peri)-math.sin(node)*math.cos(peri)*math.cos(inc),
            -math.sin(node)*math.sin(peri)+math.cos(node)*math.cos(peri)*math.cos(inc),
            math.cos(peri)*math.sin(inc)]
    return [radius*(math.cos(nu)*x+math.sin(nu)*y) for x,y in zip(first,second)]+[
        speed*(-math.sin(nu)*x+(e+math.cos(nu))*y) for x,y in zip(first,second)]


def validate_cartesian(case, records, phase):
    witnesses=case['cartesian_witnesses'];truth=independent_conic(case)
    require(len(witnesses)==len(records)==1, 'Missing/extra Cartesian witness: '+phase)
    witness=witnesses[0];record=records[0]
    require(set(record)=={'object','coordinate_system','actual','expected','position_bound_km','velocity_bound_km_s'}
            and record['object']==witness['object']=='Subject'
            and record['coordinate_system']=='EarthMJ2000Eq', 'Substituted native Cartesian object/frame: '+phase)
    require(record['expected']==witness['expected'] and len(record['expected'])==len(record['actual'])==6
            and all(finite_number(v) for v in record['actual']+record['expected']), 'Incomplete/nonfinite Cartesian observation: '+phase)
    for key in ('position_bound_km','velocity_bound_km_s'):
        require(record[key]==witness[key] and finite_number(record[key]) and record[key]>0,
                'Changed Cartesian tolerance: '+phase)
    for i,(actual,expected,independent) in enumerate(zip(record['actual'],record['expected'],truth)):
        # Fixture self-consistency uses the original orbit-definition bounds.
        bound=record['position_bound_km' if i<3 else 'velocity_bound_km_s']
        require(abs(expected-independent)<=(1e-8 if i<3 else 1e-11), 'Cartesian fixture differs from independent conic truth')
        require(abs(actual-independent)<=bound, 'Native Cartesian state differs from independent conic truth: '+phase)


def audit(repo, path):
    evidence=load(path)
    require(evidence['contract_kind']=='source-versioned-bounded-Save-orbit-conjunction'
            and evidence['qualified_keys']==[] and evidence['goal_complete'] is False
            and evidence['whole_FRC_E132_qualified'] is False, 'Bounded orbit evidence silently awards complete credit')
    inventory=nasa.inventory(repo);require(evidence['authority_sha256']==inventory['sources'], 'NASA authority changed')
    require(sha(evidence['parent_evidence'])==evidence['parent_evidence_sha256']
            and sha(evidence['parent_seal'])==evidence['parent_seal_sha256'], 'Prior131-case evidence changed')
    for name,digest in evidence['artifact_sha256'].items():require(sha(name)==digest, 'Changed orbit artifact: '+name)
    parent=parent_audit(repo,evidence['parent_evidence'])
    require(parent['cumulative_cases']==131 and parent['cumulative_contexts']==262
            and parent['new_requirement_credit']==[], 'Prior evidence conjunction changed')
    matrix=load(evidence['fixture']);cases={c['key']:c for c in matrix['cases']}
    require(len(cases)==len(matrix['cases'])==11, 'Missing/duplicate orbit definition')
    runs={name:read_run(repo,spec,inventory,evidence['artifact_sha256'],{}) for name,spec in evidence['runs'].items()}
    require(set(evidence['selected'])=={'headless','visible'}, 'Required orbit mode omitted')
    count=0
    for mode,selected in evidence['selected'].items():
        require(len(selected)==len(cases) and {e['case'] for e in selected}==set(cases), 'Missing/duplicate orbit case in '+mode)
        for entry in selected:
            receipt,results,sources=runs[entry['run']];name=PREFIX+entry['case'];case=cases[entry['case']]
            require(receipt['mode']==mode and entry['test']==name, 'Substituted orbit mode/test')
            definitions=json.loads(sources[case['oracle_source']])['cases'];original=next(c for c in definitions if c['key']==case['key'])
            require(case['canonical_keplerian']==original['canonical_keplerian'] and case['mu']==original['mu']
                    and case['cartesian_witnesses']==[dict(object='Subject',expected=original['expected_cartesian'],
                      position_bound_km=original['position_tolerance_km'],velocity_bound_km_s=original['velocity_tolerance_km_s'])],
                    'Changed original independent orbit inputs/bounds')
            for field,value in zip(original['names'],original['values']):
                expected=(value+180)%360-180 if field=='RA' else value
                require([w for w in case['witnesses'] if w['object']=='Subject' and w['field']==field]
                        ==[dict(object='Subject',field=field,type='Real',expected=expected)],
                        'Changed original orbit literal/native RA convention')
            code=sources['src/qtgui/tests/NasaSaveResourceTests.cpp'].decode()
            for token in ('spacecraft->GetState("Cartesian")','Save Cartesian state differs from independent orbit witness',
                          'Save Cartesian witness object, shape or bounds invalid','runtime-cartesian.json','reloaded-cartesian.json'):
                require(token in code, 'Native Cartesian guard absent: '+token)
            work=Path(results[name]['log']).parent
            count+=check_case(case,results[name],sources,evidence['artifact_sha256'],fixture_path=FIXTURE)
            for phase in ('runtime','reloaded'):
                capture=work/(phase+'-cartesian.json');require(str(capture) in evidence['artifact_sha256'], 'Native Cartesian capture omitted')
                validate_cartesian(case,load(capture),phase)
    require(evidence['remaining_gaps'], 'Remaining any-Resource obligation hidden')
    return dict(status='orbit-Save-evidence-passed; complete any-Resource obligation open',new_cases=11,new_contexts=22,
                prior_cases_replayed=0,prior_cases_reused=131,cumulative_cases=142,cumulative_contexts=284,
                new_writable_property_comparisons=count,cumulative_writable_property_comparisons=count+parent['cumulative_writable_property_comparisons'],
                independent_Cartesian_contexts=22,independent_Cartesian_captures=44,independent_Cartesian_components=264,
                original_row='FRC!E132',new_requirement_credit=[],goal_complete=False)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3]);p.add_argument('--evidence',type=Path,required=True)
    a=p.parse_args();print(json.dumps(audit(a.repo.resolve(),a.evidence)))
