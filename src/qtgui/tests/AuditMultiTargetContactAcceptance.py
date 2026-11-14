"""Admit exact multi-spacecraft rise/set selection from checked raw results.

The source-versioned claim preserves the original NASA denominator. Each new
GUI case needs independent numerical checks and passing headless/visible runs;
cloned/assigned/appended/cleared results and the original default report are
checked as data, not merely as test status or declared coverage metadata.
"""
import datetime
import hashlib
import json
import math
from pathlib import Path
import re
import sys
import tarfile
import xml.etree.ElementTree as ET
import NasaGuiCoverage as nasa
from AuditCapiAcceptance import load, require, sha

SOURCE='src/qtgui/tests/NasaMultiTargetContactTests.cpp'
FIXTURE='src/qtgui/tests/NasaMultiTargetContactCases.json'
NATIVE='src/qtgui/tests/MultiTargetContactParameterTests.cpp'
CASES=['PairRise','TripletBoth','ReverseSet','SingletonOther','BoundedMixed','PairIntervals','PairMixed','DarkOnly']
KEYS=['FRR!E597','FRR!E598']
MODULES=['AcceptanceCatalog','AcceptanceSnapshot','AuditAcceptanceRecovery','AuditCapiAcceptance','AuditCentralBodyPropagationAcceptance','AuditConstraintScalarAcceptance','AuditMultiTargetContactAcceptance','AuditStationEventAcceptance','BuildNasaCoverageExtension','NasaGuiCoverage','RunHarnessAcceptance']


def archive_sources(paths):
    result={}
    for path in paths:
        with tarfile.open(path) as bundle:
            for member in bundle:
                name=member.name
                require(member.isfile() and not Path(name).is_absolute() and '..' not in Path(name).parts,'Unsafe frozen source')
                data=bundle.extractfile(member).read()
                require(not data.startswith(b'\x7fELF'),'Runtime image copied into source archive')
                result[name]=data
    return result


def successful(result, artifacts):
    require(result['status']=='passed' and result['returncode']==0,'Required GUI/native case failed')
    root=Path(result['log']).parent
    junit=root/'ctest.xml';require(str(junit) in artifacts and str(root/'test.log') in artifacts,'Actual test log/JUnit omitted')
    tests=list(ET.parse(junit).getroot().iter('testcase'))
    require(len(tests)==1 and tests[0].get('name')==result['name'] and not any(tests[0].find(k) is not None for k in ['failure','error','skipped']),'Actual CTest XML is not a pass')
    return root


def utc_seconds(stamp):
    return (datetime.datetime.strptime(stamp,'%d %b %Y %H:%M:%S.%f')-datetime.datetime(2000,1,1,12)).total_seconds()


def check_report(text, case):
    sections=text.split('Target: ')
    require(len(sections)==len(case['targets'])+1 and sections[0]=='','Missing/extra selected target')
    stamp=r'(\d{2} [A-Za-z]{3} \d{4} \d{2}:\d{2}:\d{2}\.\d{3})'
    interval=re.compile('^'+stamp+r'\s+'+stamp+r'\s+([\deE.+-]+)\s*$',re.M)
    all_observations={}
    for target,section in zip(case['targets'],sections[1:]):
        require(section.startswith(target+'\n'),'Wrong target identity/order')
        observations={}
        if 'ContactIntervals' in case['event_types']:
            parts=section.split('Station events\n')[0].split('Observer: ')
            require(len(parts)==len(case['observers'])+1,'Wrong observer membership')
            for part in parts[1:]:
                observer=part.split('\n')[0].strip()
                require(observer in case['observers'] and observer not in observations,'Unselected/duplicate observer')
                actual=[dict(start=utc_seconds(m[1]),end=utc_seconds(m[2]),duration=float(m[3])) for m in interval.finditer(part)]
                expected=case['events'][target][observer]
                require(len(actual)==len(expected),'Wrong interval count')
                for got,want in zip(actual,expected):
                    require(all(math.isfinite(v) for v in got.values()),'Nonfinite interval')
                    for name in ['start','end']:require(abs(got[name]-want[name])<case['time_tolerance'],'Wrong independent interval boundary')
                    require(abs(got['duration']-want['duration'])<case['duration_tolerance'] and abs(got['end']-got['start']-got['duration'])<.002,'Wrong independent interval duration')
                count=re.search(r'Number of events\s*:\s*(\d+)',part)
                require(count and int(count[1])==len(expected),'Wrong observer summary count')
                if not expected:require('There are no contact events' in part,'No-event observer omitted')
                observations[observer]=actual
        else:require('Observer: ' not in section,'Intervals leaked into station-only selection')
        discrete=any(t in case['event_types'] for t in ['StationRise','StationSet'])
        require(section.count('Station events\n')==int(discrete),'Wrong station-event section count')
        if discrete:
            actual=[]
            for line in section.splitlines():
                cells=line.split('\t')
                if len(cells)!=4 or not cells[1].startswith('Station'):continue
                require(len(actual)<len(case['station_events'][target]),'Unselected extra root')
                want=case['station_events'][target][len(actual)]
                require(cells[0]==want['observer'] and cells[1]==want['event_type'],'Wrong selected event type/observer')
                a1=float(cells[2]);utc=utc_seconds(cells[3])
                require(math.isfinite(a1) and abs(utc-want['time'])<case['time_tolerance'] and abs((a1-21545)*86400-32.0343817-want['time'])<case['time_tolerance'],'Wrong independent station root')
                actual.append(dict(observer=cells[0],event_type=cells[1],utc_seconds=utc,a1=a1))
            require(len(actual)==len(case['station_events'][target]),'Selected root omitted')
            count=re.search(r'Number of station events : (\d+)',section)
            require(count and int(count[1])==len(actual),'Wrong station summary')
            observations['station_events']=actual
        all_observations[target]=observations
    return all_observations


def check_case(case, result, artifacts):
    root=successful(result,artifacts)
    essentials=['proof.json','events.txt','final.csv','native-stations.json','observations.json','mission.script','initial.script','messages.txt','cloned-events.txt','assigned-events.txt','appended-events.txt','cleared-events.txt']
    require(all(str(root/name) in artifacts for name in essentials),'Essential multi-target result omitted')
    proof=load(root/'proof.json');require(nasa.proof_records(result)==[proof],'Proof differs from actual logged checkpoint')
    require(proof['checkpoint']=='multi-target-contact-'+case['key'] and proof['targets']==case['targets'] and proof['cancels']==1 and proof['numeric_sentinel']==9,'Changed GUI target/lifecycle proof')
    require(all(proof[k] is True for k in ['clone_report_equal','assignment_report_equal','append_report_equal','aggregate_clear']) and proof['independent_ephemeris_managers']==len(case['targets']),'Missing result ownership/target recording assertion')
    raw=(root/'events.txt').read_bytes();actual=check_report(raw.decode(),case)
    require(actual==proof['events']==load(root/'observations.json'),'Raw report differs from observed proof')
    require((root/'cloned-events.txt').read_bytes()==raw and (root/'assigned-events.txt').read_bytes()==raw and (root/'appended-events.txt').read_bytes()==raw*2,'Clone/assignment/append loses selected results')
    cleared=(root/'cleared-events.txt').read_text();require([s.split('\n')[0] for s in cleared.split('Target: ')[1:]]==case['targets'] and not re.search(r'Station(?:Rise|Set)\t|Number of (?:station )?events\s*:\s*[1-9]',cleared),'Aggregate clear retains stale target events')
    fields=(root/'final.csv').read_text().strip().split(',');require(len(fields)==25 and fields[-1].strip()=='9','Wrong complete final witness')
    for i,name in enumerate(['Sat','Other','Third','Excluded']):
        for j,want in enumerate(case['spacecraft_final_states'][name]):
            got=float(fields[6*i+j]);require(math.isfinite(got) and abs(got-want)<(2e-5 if j<3 else 2e-8),'Independent final orbit differs')
    return proof


def regenerate(repo,q,frozen):
    import subprocess,tempfile
    archive=Path(q['oracle_static_library']);require(sha(archive)==q['oracle_static_library_sha256'],'Independent data-reader library changed')
    fixture=json.loads(frozen[FIXTURE])
    with tempfile.TemporaryDirectory(prefix='gmat-multi-target-oracle-') as folder:
        root=Path(folder);tool=root/'src/qtgui/tests';tool.mkdir(parents=True)
        for name in ['MultiTargetContactOracle.py','ContactGeometryOracle.py','EclipseGeometryOracle.py','NasaContactResultCases.json']:
            (tool/name).write_bytes(frozen['src/qtgui/tests/'+name])
        for name,digest in fixture['physical_inputs']['kernels'].items():
            require(sha(repo/name)==digest,'Independent physical input changed')
            dst=root/name;dst.parent.mkdir(parents=True,exist_ok=True);dst.symlink_to(repo/name)
        library=root/'data-reader.so'
        subprocess.run(['gcc','-shared','-o',str(library),'-Wl,--whole-archive',str(archive),'-Wl,--no-whole-archive','-lm'],check=True,capture_output=True)
        subprocess.run([sys.executable,str(tool/'MultiTargetContactOracle.py'),'--library',str(library)],check=True,capture_output=True)
        require(load(tool/'NasaMultiTargetContactCases.json')==fixture,'Frozen independent fixture cannot regenerate exactly')


def qualify(repo,path,inventory):
    q=load(path);require(q['contract_kind']=='source-versioned-multiple-spacecraft-contact-selection' and q['qualified_keys']==KEYS,'Wrong multi-target contract/keys')
    require(q['authority_sha256']==inventory['sources'],'Original authority changed')
    require(all(q.get(k) is False for k in ['technical_core_approval','full_current_build_qualified','full_NASA_qualified']),'Qualification exceeds evidence')
    essential=set(q['source_archives']+q['default_reports'])|{q[k] for k in ['build_guard','loaded_images','native_receipt','original_default_report','baseline_seal','inherited_station_qualification','admission_source_archive','regeneration']}
    essential.update(spec['receipt'] for spec in q['runs'].values())
    require(essential<=set(q['artifact_sha256']),'Essential admission evidence omitted')
    for name,digest in q['artifact_sha256'].items():require(sha(name)==digest,'Changed multi-target artifact: '+name)
    known={r['key']:r for r in inventory['requirements']}
    require(q['original_requirements']==[dict(key=k,fingerprint=known[k]['fingerprint'],source_text=known[k]['text']) for k in KEYS],'Original requirements changed')
    seal=load(q['baseline_seal']);remaining={row['key'] for row in seal['remaining_original_rows']['requirements']}
    require(seal['authority_sha256']==inventory['sources'] and all(k not in remaining for k in ['FRR!E599','FRR!E603','FRR!E604']),'Inherited observer/combined/occultation children not qualified')
    from AuditStationEventAcceptance import qualify as station_qualify
    child,child_keys=station_qualify(repo,Path(q['inherited_station_qualification']),inventory)
    require(child_keys==['FRR!E600','FRR!E601','FRR!E602'],'Inherited event-type children incomplete')
    frozen=archive_sources(q['source_archives']);fixture=json.loads(frozen[FIXTURE]);cases={c['key']:c for c in fixture['cases']}
    require(list(cases)==CASES and fixture['parent_fixture_sha256']==hashlib.sha256(frozen['src/qtgui/tests/NasaContactResultCases.json']).hexdigest(),'Changed case/parent corpus')
    require(all(c['time_tolerance']==c['duration_tolerance']==.002 for c in cases.values()),'Original numerical tolerances weakened')
    require(all(hashlib.sha256(frozen[p]).hexdigest()==h for p,h in fixture['oracle_dependencies'].items()),'Independent oracle source drift')
    guard=load(q['build_guard']);compiled=archive_sources([guard['source_archive']])
    require({str((repo/p).resolve()) for p in [guard['source_archive'],*guard['successful_builds']]}<=set(q['artifact_sha256']),'Compiled archive/build evidence omitted')
    require(guard['build_exit_codes']==[0,0] and all(sha(p)==h and 'FAILED:' not in Path(p).read_text() for p,h in guard['successful_builds'].items()),'No successful final build')
    require(sha(guard['source_archive'])==guard['source_archive_sha256'] and all(hashlib.sha256(compiled[p]).hexdigest()==h for p,h in guard['compiled_sources'].items()),'Changed compiled source inputs')
    require(all(compiled[p]==frozen[p] for p in q['production_sources']+[SOURCE,NATIVE]),'Compiled/accepted source intersection differs')
    images=load(q['loaded_images']);observed=[row for row in images['processes'].values() if all(row['images'].get(p)==h for p,h in guard['project_images'].items())]
    require(observed and all(row['images'].get('build/linux-gui/src/qtgui/GmatQtNasaMultiTargetContactTests')==guard['executables']['build/linux-gui/src/qtgui/GmatQtNasaMultiTargetContactTests'] for row in observed),'Actual loaded engine/plugin/test image not bound')
    selected={mode:{} for mode in ['headless','visible']};receipts={};mapped={mode:set() for mode in ['headless','visible']};mapped_pids=set()
    for name,spec in q['runs'].items():
        receipt=load(spec['receipt']);receipts[name]=receipt
        require(receipt.get('finished_utc') and receipt.get('end_reason')=='completed-selection','Nonterminal run')
        for p,h in receipt['source_hashes'].items():
            if p in frozen:require(hashlib.sha256(frozen[p]).hexdigest()==h,'Executed/frozen source differs: '+p)
            elif p in inventory['sources']:require(inventory['sources'][p]==h,'Executed authority differs')
            elif p in fixture['physical_inputs']['kernels']:require(fixture['physical_inputs']['kernels'][p]==h,'Executed physical input differs')
            else:require(p==q['acceptance_probe'] and q['unexecuted_inventory_reference']==dict(path=p,sha256=h,executed=False),'Unbound receipt input '+p)
        active={r['name']:r for r in receipt['results'] if r['status']!='not-run'}
        for case in spec['cases']:
            result=active['QtGui.NasaMultiTargetContact'+case];exe=str(repo/'build/linux-gui/src/qtgui/GmatQtNasaMultiTargetContactTests')
            require(exe in result['registered_command'] and receipt['binary_hashes'][exe]==guard['executables'][str(Path(exe).relative_to(repo))],'Executed multi-target binary differs')
            require(case not in selected[receipt['mode']],'Duplicate case credit')
            selected[receipt['mode']][case]=check_case(cases[case],result,q['artifact_sha256'])
            for trace in Path(result['log']).parent.glob('trace-*.jsonl'):
                require(str(trace) in q['artifact_sha256'],'Actual process trace omitted')
                pid=trace.stem.removeprefix('trace-');observed=images['processes'].get(pid)
                if observed is not None:
                    require(observed['case']==case and all(observed['images'].get(p)==h for p,h in guard['project_images'].items()) and observed['images'].get(str(Path(exe).relative_to(repo)))==guard['executables'][str(Path(exe).relative_to(repo))],'Process trace/case/engine/plugin image identity differs')
                    mapped[receipt['mode']].add(case);mapped_pids.add(pid)
        if receipt['mode']=='visible':
            from AuditCentralBodyPropagationAcceptance import passing_visible_projection
            from AuditConstraintScalarAcceptance import validate_visible
            projection=passing_visible_projection(receipt)
            validate_visible(projection,{n:dict(log=r['log'],log_sha256=sha(r['log']),junit=str(Path(r['log']).parent/'ctest.xml'),junit_sha256=sha(Path(r['log']).parent/'ctest.xml')) for n,r in active.items() if r['status']=='passed'})
            require(receipt['resizable'] and receipt['display_scale']=='auto','Visible replay is not scalable/resizable')
    require(all(set(rows)==set(CASES) for rows in selected.values()),'A mode/case is missing')
    require(mapped_pids==set(images['processes']),'Unbound or omitted actual process image observation')
    require(set.union(*mapped.values())==set(CASES) and {k:sorted(v) for k,v in mapped.items()}==q['actual_mapping_observation'],'Selected cases lack actual image observations or scope overclaims their modes')
    for case in CASES:require(selected['headless'][case]==selected['visible'][case],'Visible/headless results differ')
    native=load(q['native_receipt']);result=next(r for r in native['results'] if r['name']=='QtGui.MultiTargetContactParameters');root=successful(result,q['artifact_sha256']);proof=load(root/'proof.json')
    require(nasa.proof_records(result)==[proof] and proof['rejected_inputs']==13 and proof['owned_result_deleted']==1 and all(proof[k] is True for k in ['explicit_empty_parsed','reference_copy_assignment_rename','detailed_event_value_copy','rebind_invalidates_worker','reinitialization_idempotent']),'Native ownership/list/error contract missing')
    require(native['source_hashes'][NATIVE]==hashlib.sha256(frozen[NATIVE]).hexdigest(),'Native executed another assertion source')
    require(native['binary_hashes'][str(repo/'build/linux-gui/src/qtgui/GmatQtMultiTargetContactParameterTests')]==guard['executables']['build/linux-gui/src/qtgui/GmatQtMultiTargetContactParameterTests'],'Executed native client differs')
    original_source='src/qtgui/tests/NasaContactResultTests.cpp'
    inherited_guard=load(child['runs'][child['selected']['visible']]['build_guard'])
    require(hashlib.sha256(frozen[original_source]).hexdigest()==inherited_guard['compiled_sources'][original_source],'Original GUI driver/assertions changed')
    original_fixture=json.loads(frozen['src/qtgui/tests/NasaContactResultCases.json'])
    station_fixture=json.loads(frozen['src/qtgui/tests/NasaStationEventCases.json'])
    for receipt in [native,*[x for x in receipts.values() if x['mode']=='visible']]:
        active={r['name']:r for r in receipt['results'] if r['status']!='not-run'}
        for name,expected in [('QtGui.NasaContactResultMultiStation',original_fixture['cases'][0]),('QtGui.NasaStationEventBoth',next(c for c in station_fixture['cases'] if c['key']=='Both'))]:
            result=active[name];root=successful(result,q['artifact_sha256']);proof=load(root/'proof.json')
            require(nasa.proof_records(result)==[proof] and proof['cancels']==1 and proof['numeric_sentinel']==9 and proof['invalid_rejections']==1,'Affected original GUI assertions missing')
            require(receipt['source_hashes'][original_source]==hashlib.sha256(frozen[original_source]).hexdigest(),'Affected GUI assertion source differs')
            exe=str(repo/'build/linux-gui/src/qtgui/GmatQtNasaContactResultTests')
            require(exe in result['registered_command'] and receipt['binary_hashes'][exe]==guard['executables'][str(Path(exe).relative_to(repo))],'Affected GUI binary differs')
            case=dict(expected,targets=[expected['target']],events={expected['target']:expected['events']},station_events={expected['target']:expected.get('station_events',[])},event_types=expected.get('event_types',['ContactIntervals']))
            actual=check_report((root/'events.txt').read_text(),case)
            require(actual[expected['target']]==proof['events'],'Affected original raw results differ')
            fields=(root/'final.csv').read_text().strip().split(',');require(len(fields)==7 and fields[-1].strip()=='9','Affected final witness missing')
            for i,want in enumerate(expected['final_state']):require(abs(float(fields[i])-want)<(2e-5 if i<3 else 2e-8),'Affected original final orbit differs')
    station_result=next(r for r in native['results'] if r['name']=='QtGui.StationEventParameters');root=successful(station_result,q['artifact_sha256']);proof=load(root/'proof.json')
    require(nasa.proof_records(station_result)==[proof] and proof['rejected_inputs']==12 and proof['clone_isolated'] and proof['stale_outputs_cleared'],'Affected native event-type contract missing')
    for p in q['default_reports']:require(Path(p).read_bytes()==Path(q['original_default_report']).read_bytes(),'Legacy default report bytes changed')
    modules=archive_sources([q['admission_source_archive']])
    for name in MODULES:
        p='src/qtgui/tests/'+name+'.py';executing=sys.modules[__name__] if name=='AuditMultiTargetContactAcceptance' else sys.modules.get(name)
        require(executing is not None and Path(executing.__file__).resolve()==repo/p and sha(repo/p)==hashlib.sha256(modules[p]).hexdigest(),'Executing admission helper differs: '+name)
    regenerate(repo,q,frozen)
    require(load(q['regeneration'])['fixture_sha256']==hashlib.sha256(frozen[FIXTURE]).hexdigest() and load(q['regeneration'])['status']=='passed','Independent frozen oracle regeneration missing')
    return q,KEYS


if __name__=='__main__':
    import argparse
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3]);p.add_argument('--qualification',type=Path,required=True);a=p.parse_args()
    q,keys=qualify(a.repo.resolve(),a.qualification,nasa.inventory(a.repo.resolve()));print(json.dumps(dict(status='passed',qualified_keys=keys)))
