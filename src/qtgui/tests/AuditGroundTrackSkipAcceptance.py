"""Audit the withdrawn single-segment skip-control experiment without NASA credit.

Source review identified a Propagate-boundary counterexample. The experimental
control has been removed; qualify() intentionally rejects whole-row admission.
Native experiment receipts remain valid only for their recorded narrow scope.
"""
import argparse
import csv
import hashlib
import json
import math
from pathlib import Path
import re

import NasaGuiCoverage as nasa
from AuditCapiAcceptance import load,require,sha
from AuditCentralBodyPropagationAcceptance import read_run

KEYS=['FRR!E633','FRR!E634']
COUNTS=[0,1,2,7]
REGRESSIONS=['QtGui.NasaGroundTrackBodySymbolsDense','QtGui.NasaGroundTrackBodySymbolsSparse','QtGui.PlotSetup','QtGui.NativeNasaPlotCadence']
SOURCE='src/qtgui/tests/NasaGroundTrackSkipTests.cpp'
GUI='src/qtgui/GroundTrackDialog.cpp'
FACETS=['actual-primary-skip-control-and-bidirectional-interval-mapping','zero-max-range-pending-close-cancel-discard-and-native-isolation','Apply-idempotence-Undo-Redo-Unicode-save-reopen','independent-native-publication-grid-and-circle','exact-integration-step-gaps-between-regular-collected-points','final-raw-report-sentinel-source-history']


def circle(seconds):
    radius=7000.;rate=math.sqrt(398600.4415/radius**3);a=rate*seconds
    return [radius*math.cos(a),radius*math.sin(a),0.,-radius*rate*math.sin(a),radius*rate*math.cos(a),0.]


def check_case(result,skipped,sources,artifacts):
    work=Path(result['log']).parent
    for name in ['proof.json','publications.json','collected.json','baseline.script','applied.script','results.csv','messages.txt','test.log','ctest.xml']:
        require(str(work/name) in artifacts,'Skip admission artifact omitted: '+name)
    p=load(work/'proof.json');require([r for r in nasa.proof_records(result) if r.get('checkpoint')=='ground-integration-skip-count']==[p], 'Saved/logged native skip proof differs')
    require(result['status']=='passed' and result['returncode']==0 and p['outcome']=='passed' and p['skipped_steps']==skipped and p['native_interval']==skipped+1 and p['facets']==FACETS,'Substituted skip proof')
    publications=load(work/'publications.json');collected=load(work/'collected.json')
    require(publications==p['publications'] and collected==p['collected'] and len(publications)==p['publication_count'] and len(publications) in (61,62),'Incomplete native publication grid')
    expected_collected=[];last_step=-1;last_frame=0;regular=0
    for ordinal,row in enumerate(publications,1):
        step=min(ordinal-1,60);seconds=row['elapsed'];state=row['state'];frame=1+(ordinal-1)//(skipped+1)
        require(row['publication']==ordinal and row['integration_step']==step and math.isfinite(seconds) and abs(seconds-step*10)<.0001 and row['frame']==frame and row['selected']==(frame>last_frame),'Independent native ordinal/clock/selection differs')
        require(len(state)==6 and all(math.isfinite(v) and abs(v-t)<(.00003 if i<3 else .00000004) for i,(v,t) in enumerate(zip(state,circle(seconds)))),'Raw native state differs from independent circle')
        if frame>last_frame:
            duplicate=step==last_step
            if not duplicate:
                require(last_step<0 or step-last_step==skipped+1,'Collected integration points do not skip the requested count')
                last_step=step;regular+=1
            expected_collected.append(dict(publication=ordinal,integration_step=step,frame=frame,bookkeeping_duplicate=duplicate))
        last_frame=frame
    require(collected==expected_collected and p['regular_collected']==regular==1+60//(skipped+1),'Incomplete or changed skip selection')
    raw=list(csv.reader((work/'results.csv').open()));require(len(raw)==2 and raw[0]==['41'] and len(raw[1])==8,'Skip raw report shape/sentinel differs')
    truth=[600.,*circle(600.),41.]
    require(all(math.isfinite(float(v)) and abs(float(v)-t)<(.0001 if i==0 else .00003 if i<4 else .00000004) for i,(v,t) in enumerate(zip(raw[1],truth))),'Skip final raw report differs from independent truth')
    baseline=(work/'baseline.script').read_text();applied=(work/'applied.script').read_text()
    require(baseline.split('BeginMissionSequence;',1)[1]==applied.split('BeginMissionSequence;',1)[1], 'Skip Apply changed unrelated mission')
    require(re.findall(r'^Map\.DataCollectFrequency = (\d+);$',applied,re.M)==[str(skipped+1)] and 'Map.IntegrationStepsToSkip' not in applied,'Skip translation changed native script semantics')
    for name,value in [('UpdatePlotFrequency',3),('MaxPlotPoints',1000),('NumPointsToRedraw',0)]:
        require(re.findall(r'^Map\.'+name+r' = (\d+);$',applied,re.M)==[str(value)], 'Skip Apply changed sibling setting')
    code=sources[SOURCE].decode();gui=sources[GUI].decode()
    for token in ['ground_IntegrationStepsToSkip','Ground skip collection differs from independent publication ordinal','Collected points do not skip the requested number of integration steps','Maximum skip mapping overflowed or committed draft','Skip close Cancel lost pending/source/native state','Skip discard changed applied state','Skip exact Undo failed','Skip exact Redo failed','Skip repeated Apply changed history','Skip Unicode save/reload failed','Publisher::Instance()->Subscribe','dynamic_cast<GroundTrack *>']:
        require(token in code,'Original skip assertion/backend omitted: '+token)
    for token in ['"Integration steps to skip"','"ground_IntegrationStepsToSkip"','std::numeric_limits<int>::max()-1','interval->setValue(count+1)','skippedSteps->setValue(count-1)']:
        require(token in gui,'Literal skip-count GUI mapping absent: '+token)
    return dict(skipped_steps=skipped,publications=len(publications),regular_collected=regular)


def audit(repo,path,inventory):
    from BuildNasaCoverage95 import baseline
    q=load(path)
    require(q['contract_kind']=='source-versioned-bounded-GroundTrack-skip-experiment' and q['qualified_keys']==[] and q['row_admission_withdrawn'] is True,'Partial/extra GroundTrack credit')
    require(all(q.get(k) is False for k in ['technical_core_approval','full_current_build_qualified','full_NASA_qualified']),'GroundTrack qualification exceeds scope')
    require(q['authority_sha256']==inventory['sources'] and q['goal_complete'] is False,'NASA authority/goal scope changed')
    for name,digest in q['artifact_sha256'].items():require(sha(name)==digest,'Changed skip artifact: '+name)
    change_baseline=load(q['change_baseline'])
    require(q['change_baseline'] in q['artifact_sha256'] and q['core_sha256']==change_baseline['previous_core'] and len(q['core_sha256'])==21, 'Core review paths omitted/substituted')
    review=load(q['review']);known={r['key']:r for r in inventory['requirements']}
    require(review['authority_sha256']==inventory['sources'], 'Review authority changed')
    require([r['key'] for r in review['reviews']]==KEYS and review['unqualified_OrbitView_rows']==['FRR!E329','FRR!E330'],'Row scope/OrbitView hold changed')
    for row in review['reviews']:require(row['fingerprint']==known[row['key']]['fingerprint'] and row['source_text']==known[row['key']]['text'] and row['rationale'],'Original source row changed')
    children={r['key'] for r in inventory['requirements'] if r['declared_id'].startswith('FRR-42.5.') and r['key']!='FRR!E633'}
    require(children==set(review['required_children'])=={'FRR!E634','FRR!E635','FRR!E636'},'Original finite heading changed')
    _,old_inventory,keys=baseline(repo,Path(q['baseline_seal']));require(old_inventory['sources']==inventory['sources'] and children-{'FRR!E634'}<=keys['requirements'],'Inherited redraw/retention child absent')
    runs={name:read_run(repo,spec,inventory,q['artifact_sha256'],q['physical_input_sha256']) for name,spec in q['runs'].items()}
    for name,(receipt,results,sources) in runs.items():
        before=load(q['runs'][name]['before'])['sources']
        require(all(before.get(n)==h for n,h in q['core_sha256'].items()),'Core source changed across skip contexts')
        require(hashlib.sha256(sources[GUI]).hexdigest()==q['GUI_sha256'],'Substituted skip GUI source')
        require(hashlib.sha256(sources[SOURCE]).hexdigest()==q['native_test_sha256'], 'Native skip driver changed/substituted')
    require(set(q['selected'])=={'headless','visible'},'Skip execution mode omitted');checks=[]
    for mode,selected in q['selected'].items():
        require(len(selected)==4 and {e['skipped'] for e in selected}==set(COUNTS),'Missing/duplicate skipped-count partition')
        for e in selected:
            receipt,results,sources=runs[e['run']];name='QtGui.NasaGroundSkip'+str(e['skipped'])
            require(receipt['mode']==mode and e['test']==name,'Substituted skip mode/test')
            checks.append(check_case(results[name],e['skipped'],sources,q['artifact_sha256']))
    require({e['test'] for e in q['affected_regressions']}==set(REGRESSIONS) and len(q['affected_regressions'])==4,'Affected GroundTrack regressions omitted')
    for e in q['affected_regressions']:
        receipt,results,sources=runs[e['run']];require(receipt['mode']=='headless' and results[e['test']]['status']=='passed','Affected regression failed or delivery substituted')
    return q,[]


def qualify(repo,path,inventory):
    raise ValueError('GroundTrack integration-step admission withdrawn: split-command sampling unresolved')


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3]);p.add_argument('--evidence','--qualification',dest='evidence',type=Path,required=True);a=p.parse_args();q,keys=audit(a.repo.resolve(),a.evidence,nasa.inventory(a.repo.resolve()));print(json.dumps(dict(status='bounded experiment passed; literal requirement admission withdrawn',qualified_keys=keys,new_contexts=8,affected_regressions=4)))
