"""Admit the complete original Brent/user-defined-tolerance root-method list."""
import argparse, copy, datetime, hashlib, json, math, re, tarfile
from pathlib import Path
import NasaGuiCoverage as nasa
import BuildNasaCoverage95 as builder
import BuildNasaCoverageExtension as common
from AuditStationEventAcceptance import report_events
load, sha, require = common.load, common.sha, common.require
KEYS = ['FRR!E605', 'FRR!E606']
CONTEXT = ['FRR!E'+str(i) for i in range(589,607)]
KIND = 'source-versioned-complete-original-Brent-root-method-list'
GATES = ['scientific_approval','technical_core_approval','full_current_build_qualified','full_NASA_qualified']
CLAUSES = {'complete-original-method-parent-and-child','actual-method-tolerance-GUI-and-lifecycle',
 'unchanged-original-independent-result-assertions','both-tolerances-all22-result-cases-both-profiles',
 'exported-native-Brent-scalar-policy','additional-intrusion-region-coherence',
 'background-CK-clock-lifetime-repair-and-regressions','preserved-failures-original-inputs-and-runtime-identities'}
STAMP = r'(\d{2} [A-Za-z]{3} \d{4} \d{2}:\d{2}:\d{2}\.\d+)'
NUMBER = r'([\deE.+-]+)'
def seconds(text):
    return (datetime.datetime.strptime(text,'%d %b %Y %H:%M:%S.%f')-datetime.datetime(2000,1,1,12)).total_seconds()

def structure(q,s,inventory):
    known={x['key']:x for x in inventory['requirements']}
    require(q['contract_kind']==KIND and q['qualified_keys']==KEYS and q['authority_sha256']==inventory['sources'],'Root scope or authority changed')
    require(s['candidate_rows']==[known[k] for k in KEYS] and s['complete_original_context']==[known[k] for k in CONTEXT],'Original root list/context changed')
    require(s['tolerances_seconds']==[1e-4,1e-6] and s['profiles']==['headless','visible'] and s['original_result_cases']==22,'Root profile/case universe narrowed')
    require(set(s['clauses'])==CLAUSES and all(x['satisfied'] and x['evidence'] for x in s['clauses'].values()),'Incomplete root conjunction')
    require(s['original_assertions_changed']==s['original_fixture_or_tolerance_changes']==s['other_row_credit']==s['runtime_binary_copies']==0
     and s['retained_popup_failures']==18 and s['newly_resolved_hold_keys']==[]
     and s['retained_original_Ephemeris_failure'] and not s['full_original_regression_passed'],'Root fidelity/hold scope changed')
    require(all(q[k] is False and s[k] is False for k in GATES),'Wider root approval promoted')

def damage(q,s,inventory):
    rejected=[]
    for fault in ['extra-row','fingerprint','missing-child','missing-context','missing-tolerance','missing-profile','missing-case','missing-clause','changed-original','wider-approval']:
        a,b=copy.deepcopy(q),copy.deepcopy(s)
        if fault=='extra-row':a['qualified_keys'].append('FRR!E604')
        elif fault=='fingerprint':b['candidate_rows'][0]['fingerprint']='0'*64
        elif fault=='missing-child':b['candidate_rows'].pop()
        elif fault=='missing-context':b['complete_original_context'].pop()
        elif fault=='missing-tolerance':b['tolerances_seconds'].pop()
        elif fault=='missing-profile':b['profiles'].pop()
        elif fault=='missing-case':b['original_result_cases']=21
        elif fault=='missing-clause':b['clauses'].pop(next(iter(b['clauses'])))
        elif fault=='changed-original':b['original_assertions_changed']=1
        else:a['full_current_build_qualified']=True
        try:structure(a,b,inventory)
        except (ValueError,AssertionError):rejected.append(fault)
        else:raise ValueError('Damaged root admission accepted: '+fault)
    return rejected

def intervals(text,spec,eclipse):
    if not spec['write_report']:
        require(text=='KEEP prior report bytes: Δ\n','Disabled event report replaced sentinel');return [] if eclipse else {}
    require(text.startswith(('Spacecraft: ' if eclipse else 'Target: ')+spec['target']+'\n'),'Wrong event target/header')
    pattern=r'^'+STAMP+r'\s+'+STAMP+r'\s+'+NUMBER
    if eclipse:
        actual=[]
        for m in re.finditer(pattern+r'\s+(\S+)\s+(Umbra|Penumbra|Antumbra)\s+(\d+)\s+'+NUMBER+r'\s*$',text,re.M):
            actual.append(dict(start=seconds(m[1]),end=seconds(m[2]),duration=float(m[3]),body=m[4],type=m[5],group=int(m[6]),total_duration=float(m[7])))
        expected=spec['events'];require(len(actual)==len(expected),'Eclipse interval count differs')
        for a,e in zip(actual,expected):
            require(all(a[k]==e[k] for k in ['body','type','group']),'Eclipse interval identity differs')
            compare_interval(a,e,spec,True)
        if not expected:require('There are no eclipse events' in text and 'Number of individual events' not in text,'No-event summary differs')
        else:
            for label,want in [('Number of individual events',len(expected)),('Number of total events',spec['total_events'])]:
                m=re.search(re.escape(label)+r'\s*:\s*(\d+)',text);require(m and int(m[1])==want,'Eclipse summary count differs')
            m=re.search(r'Maximum duration \(s\)\s*:\s*'+NUMBER,text);require(m and abs(float(m[1])-spec['maximum_duration'])<spec['duration_tolerance'],'Eclipse maximum differs')
            m=re.search(r'Maximum duration at the (\d+)(?:st|nd|rd|th) eclipse',text);require(m and int(m[1])==spec['maximum_group'],'Wrong maximum group')
        summary=text.split('Eclipse type summary (individual intervals)',1);require(len(summary)==2,'No per-type summary')
        rows=re.findall(r'^(Umbra|Penumbra|Antumbra)\s+(\d+)\s+'+NUMBER+r'\s+(\S+)\s*$',summary[1],re.M)
        require(len(rows)==len(spec['per_type']) and len({x[0] for x in rows})==len(rows),'Per-type membership differs')
        for kind,count,maximum,members in rows:
            e=spec['per_type'][kind];require(int(count)==e['count'] and abs(float(maximum)-e['maximum'])<spec['duration_tolerance'] and members==(','.join(e['participants']) or '-'),'Per-type count/maximum/participants differs')
        return actual
    actual={};sections=text.split('Station events\n',1)[0].split('Observer: ')
    include='event_types' not in spec or 'ContactIntervals' in spec['event_types']
    require(len(sections)==(len(spec['observers'])+1 if include else 1),'Contact observer sections differ')
    for section in sections[1:]:
        observer=section.splitlines()[0].strip();require(observer in spec['observers'] and observer not in actual,'Wrong/duplicate observer')
        found=[dict(start=seconds(m[1]),end=seconds(m[2]),duration=float(m[3])) for m in re.finditer(pattern+r'\s*$',section,re.M)]
        expected=spec['events'][observer];require(len(found)==len(expected),'Contact event count differs')
        for a,e in zip(found,expected):compare_interval(a,e,spec,False)
        m=re.search(r'Number of events\s*:\s*(\d+)',section);require(m and int(m[1])==len(expected),'Contact summary count differs')
        if not expected:require('There are no contact events' in section,'Missing no-contact membership')
        actual[observer]=found
    if 'event_types' in spec:actual['station_events']=report_events(text,spec)
    else:require('Station events\n' not in text,'Default contact report changed')
    return actual

def compare_interval(a,e,spec,eclipse):
    for field in ['start','end','duration']+(['total_duration'] if eclipse else []):
        require(math.isfinite(a[field]) and abs(a[field]-e[field])<spec['time_tolerance' if field in ['start','end'] else 'duration_tolerance'],'Independent interval bound exceeded')
    require(abs(a['end']-a['start']-a['duration'])<.002,'Printed duration differs from endpoints')

def derivative(repo,family):
    original=(repo/f'src/qtgui/tests/Nasa{family}ResultTests.cpp').read_text();value=original
    for before,after in [
     ('int main(int argc,char **argv)','// Additive Brent configuration; original result assertions below are unchanged.\n#include "EventRootSettingsAcceptance.hpp"\n\nint main(int argc,char **argv)'),
     ('  int cancels=0,invalids=0;','  int cancels=0,invalids=0;\n  eventrootaccept::Witness rootWitness;'),
     ('   auto *buttons=panel->findChild<QDialogButtonBox *>("resourceButtons");','   rootWitness.configure(panel,form,cancel,combo,pristine);\n   auto *buttons=panel->findChild<QDialogButtonBox *>("resourceButtons");'),
     (f'require(result==MainWindow::RunResult::Completed,"{family} mission failed");',f'require(result==MainWindow::RunResult::Completed,"{family} mission failed");\n  save("root-settings.json",QJsonDocument(rootWitness.observe()).toJson());')]:
        require(value.count(before)==1,'Original derived-client anchor changed');value=value.replace(before,after)
    require(value==(repo/f'src/qtgui/tests/EventBrent{family}Tests.cpp').read_text(),'Original result assertions/client changed')

def settings(d,tolerance):
    require(d['status']=='observed' and d['method']=='Brent' and d['tolerance_seconds']==tolerance and d['draft_round_trips']==2
     and d['invalid_atomic_rejections']==3 and d['configured_runtime_clone_and_persistence'] and d['original_oracle_assertions_changed']==d['new_requirement_credit']==0,'Incomplete actual root settings lifecycle')

def component(repo,stage):
    pub=load(stage/'publication-v1.json')
    for n,h in pub['artifacts'].items():require(sha(stage/n)==h,'Changed root artifact: '+n)
    guard=load(stage/'final-native-inputs-before-v1.json');require(guard==load(stage/'final-native-inputs-after-v1.json') and load(stage/'final-native-input-guard-v1.json')['identical'],'Final root sources/runtime drifted')
    require(guard==load(stage/'final-visible-inputs-before-v1.json')==load(stage/'final-visible-inputs-after-v1.json')
     and load(stage/'final-visible-input-guard-v1.json')['identical'],'Visible root sources/runtime differ')
    require(any('libGmatBase.so.' in n for n in guard['binaries']) and any('libEventLocator.so.' in n for n in guard['binaries']),'Core/plugin identity missing')
    for family in ['Eclipse','Contact']:derivative(repo,family)
    corpus={};fixtures={}
    for group,family in [('Eclipse','EclipseResult'),('Contact','ContactResult'),('Station','StationEvent')]:
        fixture=load(repo/f'src/qtgui/tests/Nasa{family}Cases.json');fixtures[group]=fixture
        for spec in fixture['cases']:
            for suffix,tolerance in [('Coarse',1e-4),('Fine',1e-6)]:corpus['QtGui.EventBrent'+group+spec['key']+suffix]=(group,spec,tolerance)
    require(len(corpus)==44,'Original mandatory case corpus changed')
    auxiliary={'QtGui.EventBrentAuxiliary'+kind+suffix:(kind,tolerance) for kind in ['Intrusion','Region'] for suffix,tolerance in [('Coarse',1e-4),('Fine',1e-6)]}
    expected=set(corpus)|set(auxiliary)|{'QtGui.EventLocators'};exposures=0;vectors=0;boundaries=0
    profiles=[load(stage/('final-'+mode+'-v1/results.json')) for mode in ['headless','visible']]
    first,last=[x['source_hashes'] for x in profiles]
    # RunAcceptance inventories every test-directory Python file. This new
    # admission reader was authored after the headless inventory; neither
    # native execution imports it. The separate guard binds all compiled
    # sources and native libraries before/after both complete profiles.
    inventory_only={'src/qtgui/tests/AuditEventRootAcceptance.py'}
    require(last.keys()-first.keys()==inventory_only and not first.keys()-last.keys()
     and all(first[k]==last[k] for k in first) and profiles[0]['binary_hashes']==profiles[1]['binary_hashes'],'Final root executed sources/binaries differ')
    for mode,profile in zip(['headless','visible'],profiles):
        rows=[x for x in profile['results'] if x['status']!='not-run'];require(profile['end_reason']=='completed-selection' and {x['name'] for x in rows}==expected|({'QtGui.Ephemeris'} if mode=='headless' else set()) and len(rows)==(50 if mode=='headless' else 49),'Final root profile incomplete')
        if mode=='visible':
            display=profile['visible_display'];require(display['start_minimized'] and display['resizable'] and not display['resize_errors'] and 'Iconic' in display['initial_host_window_state'] and 'FULLSCREEN' not in display['initial_host_window_state'],'Visible desktop policy differs')
        for row in rows:
            name=row['name'];work=Path(row['log']).parent
            if name=='QtGui.Ephemeris':
                require(row['status']=='failed' and row['returncode']==8 and 'FAIL CCSDS-OEM: The mission has changed. Build the current script and reopen this panel.' in Path(row['log']).read_text()
                 and 'Test process exit: 1 (0x00000001)' in Path(row['log']).read_text(),'Original ephemeris editor failure concealed/changed')
                continue
            require(row['status']=='passed' and row['returncode']==0 and row['requested_visibility']==mode and row['trace']['instrumented_processes']==1 and not row['trace']['screenshots'],'Required root case failed/trace lost')
            if mode=='visible':require(row['trace']['native_exposure_events']>0,'Visible root case never exposed');exposures+=row['trace']['native_exposure_events']
            if name in corpus or name in auxiliary:
                require({'Resources','event_RootFindingMethod','event_RootTolerance'}<=set(row['trace']['named_objects']),'Actual root settings controls unobserved')
            if name in auxiliary:
                kind,tolerance=auxiliary[name];proof=load(work/'proof.json');settings(proof,tolerance);require(proof['outcome']=='passed' and proof['case']==kind,'Auxiliary proof absent')
                sample='application/samples/'+('Ex_IntrusionLocator_Mercury_Sun_Transit.script' if kind=='Intrusion' else 'Ex_PlanetographicRegion.script')
                require(proof['input_sample_sha256']==load(stage/'auxiliary-plan-v1.json')['input_sample_sha256'][sample]==guard['sources'][sample],'Auxiliary shipped sample identity differs')
                def epochs(text):
                    out=[]
                    for line in text.splitlines():
                        stamps=re.findall(STAMP,line)
                        if len(stamps)>=2 or (len(stamps)==1 and line.strip().endswith(('Start','End'))):out.extend(seconds(x) for x in stamps)
                    return out
                a,b=[epochs((work/n).read_text()) for n in ['SPICE-reference.txt','Brent-report.txt']];require(len(a)==len(b)>=2 and all(abs(round(x*1000)-round(y*1000))<=2 for x,y in zip(a,b)),'Auxiliary coherence changed')
                pairs=proof['event_epoch_comparisons'];require(len(pairs)==len(a) and all(round((y-x)*1000)==pair['Brent_epoch_ms']-pair['SPICE_epoch_ms'] for x,y,pair in zip(a,b,pairs)),'Auxiliary raw/proof differences disagree')
                # Intrusion uses one epoch per Start/End row; region uses interval rows.
                if kind=='Intrusion':
                    labels=lambda n:re.findall(r'(?:Intrusion|Transit) (?:Start|End)\s*$',(work/n).read_text(),re.M)
                    require(labels('SPICE-reference.txt')==labels('Brent-report.txt') and labels('Brent-report.txt'),'Intrusion boundary labels changed')
                boundaries+=len(a);continue
            if name not in corpus:continue
            group,spec,tolerance=corpus[name];proof=load(work/'proof.json');settings(load(work/'root-settings.json'),tolerance)
            require(proof['outcome']=='passed' and proof['family']==spec['key'] and proof['target']==spec['target'] and proof['cancels']==1 and proof['numeric_sentinel']==9 and nasa.proof_records(row)==[proof],'Original lifecycle/log proof differs')
            actual=intervals((work/'events.txt').read_text(),spec,group=='Eclipse');require(actual==proof['events']==load(work/'observations.json'),'Raw event report/observation/proof differs')
            values=[float(x) for x in (work/'final.csv').read_text().strip().split(',')];require(len(values)==7 and values[-1]==9,'Original state sentinel differs')
            radius=math.sqrt(sum(x*x for x in spec['initial_state'][:3]));w=math.sqrt(spec['mu']/radius**3);angle=w*spec['duration'];initial=spec['initial_state']
            analytic=[initial[i]*math.cos(angle)+initial[i+3]/w*math.sin(angle) for i in range(3)]+[-initial[i]*w*math.sin(angle)+initial[i+3]*math.cos(angle) for i in range(3)]
            for i,(value,truth) in enumerate(zip(values,analytic)):
                require(abs(spec['final_state'][i]-truth)<1e-8 and math.isfinite(value) and abs(value-spec['final_state'][i])<(2e-5 if i<3 else 2e-8),'Independent circular endpoint differs')
            vectors+=1
    scalar=load(stage/'final-scalar-v1/proof.json');require(load(stage/'final-scalar-v1-receipt.json')['returncode']==0 and scalar['status']=='passed' and scalar['invalid_rejections']==9 and len(scalar['observations'])==18,'Native scalar policy incomplete')
    roots={'exponential':math.log(3)/5,'negative-exponential':-math.log(3)/5,'cosine':.7390851332151607,'cubic':2**(1/3)}
    require({(x['case'],x.get('tolerance')) for x in scalar['observations']}=={(case,t) for case in roots for t in [1e-2,1e-4,1e-6,1e-10]}|{('left-endpoint',None),('right-endpoint',None)},'Scalar policy universe narrowed')
    for x in scalar['observations']:
        if x['case'] in roots:require(abs(x['expected']-roots[x['case']])<1e-15 and abs(x['root']-roots[x['case']])<=x['tolerance'] and x['function_evaluations']>0,'Independent scalar root failed')
        else:require(x['root']==.125,'Endpoint root changed')
    failed=[x for x in load(stage/'visible-matrix-v1/results.json')['results'] if x['status']=='failed'];require(len(failed)==18 and all('choice popup did not open' in Path(x['log']).read_text() for x in failed),'Original popup failures concealed')
    return dict(registered_profile_passes=98,original_result_case_passes=88,auxiliary_coherence_passes=8,unchanged_original_event_regression_passes=2,retained_original_Ephemeris_failures=1,native_state_vectors=vectors,independent_state_scalar_comparisons=6*vectors,auxiliary_epoch_comparisons=boundaries,native_visible_exposures=exposures,native_scalar_observations=18,invalid_scalar_rejections=9,retained_popup_failures=18,nonexecuted_inventory_source_additions=sorted(inventory_only),full_original_regression_passed=False)

def qualify(repo,path,inventory):
    q=load(path)
    for n,h in q['artifact_sha256'].items():require(sha(n)==h,'Changed root admission artifact: '+n)
    s=load(q['scope_review']);structure(q,s,inventory)
    for archive,ids in [('reader_source_archive','reader_source_sha256'),('proof_source_archive','proof_source_sha256')]:
        require(q[archive] in q['artifact_sha256'],'Unbound root source archive')
        with tarfile.open(q[archive]) as bundle:
            require(set(bundle.getnames())==set(q[ids]),'Root archive partition differs')
            for n,h in q[ids].items():require(hashlib.sha256(bundle.extractfile(n).read()).hexdigest()==h,'Root source bytes differ')
    original=load(q['protected_input_baseline']);require(len(original)==1334 and all(q['proof_source_sha256'].get(n)==h for n,h in original.items()),'Protected original inputs omitted')
    prior=load(q['prior_overlay']);rows={x['key']:x for x in prior['reviewed_source_rows']};require(len(prior['original_hold_keys_retained'])==59 and not set(KEYS)&set(prior['original_hold_keys_retained']) and all(rows[k]['whole_row_status']=='open' for k in KEYS),'Root gap/hold inventory changed')
    proof=component(repo,Path(q['component_stage']));require(len(damage(q,s,inventory))==10,'Root damaged controls incomplete')
    return q,KEYS,dict(status='passed',qualified_keys=KEYS,whole_row_conjunction=True,component=proof,newly_resolved_original_hold_keys=[],new_requirement_credit=2,damaged_admission_controls_rejected=10,native_tests_rerun=0,runtime_binary_copies=0,**{k:False for k in GATES})

def build(repo,baseline,path,output):
    seal,inventory,keys=builder.baseline(repo,baseline);original={k:set(v) for k,v in keys.items()}
    _,incoming,audit=qualify(repo,path,inventory);known={k:{r['key']:r for r in inventory[k]} for k in keys}
    require(not set(incoming)&keys['requirements'],'Duplicate Brent root-method credit')
    keys['requirements']=common.merge_keys(keys['requirements'],incoming,known['requirements'])
    coverage={k:dict(verified=len(keys[k]),total=len(known[k]),percent=round(100*len(keys[k])/len(known[k]),1),new_verified=len(keys[k])-len(original[k])) for k in keys}
    remaining={k:[r for r in seal['remaining_original_rows'][k] if r['key'] not in keys[k]] for k in keys}
    result=dict(status='passed',scope='95-percent campaign: sealed source-versioned prefix plus complete original Brent root-method/user-defined-tolerance conjunction.',
        baseline=str(baseline),baseline_sha256=sha(baseline),authority_sha256=inventory['sources'],coverage=coverage,
        target_requirements=1101,target_reached=len(keys['requirements'])>=1101,requirements_still_needed=max(0,1101-len(keys['requirements'])),
        additions=[dict(key=k,kind='requirements',fingerprint=known['requirements'][k]['fingerprint'],source_text=known['requirements'][k]['text'],qualification=str(path),qualification_sha256=sha(path),status='verified-source-versioned') for k in incoming],
        remaining_counts={k:len(v) for k,v in remaining.items()},remaining_original_rows=remaining,
        technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False,
        limits=['Original 1158/2367 denominators and source row identities are unchanged.','Two original open root-method rows are implemented and proved; all59 separate original hold identities and statuses remain unchanged.','Scientific, physical ephemeris/frame, external ABI, platform/native OS input and full current-build approvals remain separate.'])
    with Path(output).open('x') as f:json.dump(result,f,indent=2);f.write('\n')
    return result,audit


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3]);p.add_argument('--qualification',type=Path,required=True);p.add_argument('--baseline',type=Path);p.add_argument('--output',type=Path,required=True);a=p.parse_args();repo=a.repo.resolve()
    if a.baseline:
        result,_=build(repo,a.baseline,a.qualification,a.output);print(json.dumps({'coverage':result['coverage'],'requirements_still_needed':result['requirements_still_needed']}))
    else:
        _,_,result=qualify(repo,a.qualification,nasa.inventory(repo))
        with a.output.open('x') as f:json.dump(result,f,indent=2);f.write('\n')
        print(json.dumps(result))
