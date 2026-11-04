"""Audit interrupted solver output exclusion and two same-session recoveries."""
import argparse,copy,csv,hashlib,json,re,tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
from BuildNasaCoverageExtension import require,sha,load
from AuditNestedEphemeris import circle
PREFIX='QtGui.NasaInterruptedEphemeris'
KEYS=[f+m for f in ['NestedDC','NestedYukon'] for m in ['StopCommand','DebuggerStop','RuntimeError']]
CORE=['src/base/subscriber/EphemerisFile.cpp','src/base/subscriber/EphemerisFile.hpp','src/base/command/SolverBranchCommand.cpp','src/base/command/SolverBranchCommand.hpp']


def check(proof):
    require(proof['outcome']=='passed' and proof['family'] in KEYS and [p['phase'] for p in proof['phases']]==[0,1,2],'Wrong interruption/recovery census')
    mode=next(m for m in ['StopCommand','DebuggerStop','RuntimeError'] if proof['family'].endswith(m));require(proof['mode']==mode,'Wrong native mode')
    for p in proof['phases']:
        phase=p['phase'];require(p['outcome']==('Completed' if phase else 'Failed' if mode=='RuntimeError' else 'Stopped'),'Wrong native outcome')
        require(p['done']==bool(phase) and p['Keep']==(99 if phase else 41) and p['rebuilt']==(phase==0 or (phase==1 and mode!='DebuggerStop')),'Continuation/rebuild contract differs')
        require(p['debugger_end_clicked']==(phase==0 and mode=='DebuggerStop'),'Missing actual debugger End action')
        require(p['accepted_passes']==int(bool(phase)) and len(p['rows'])==(25 if phase else 0) and p['raw_count']>0,'Trial output or missing recovery knots')
        require(p['local_accepted_outer_trial']>=(2 if phase else 1 if mode=='DebuggerStop' else 0),'Missing nested local/global partition')
        if phase:
            from AuditCommandNavigationOperations import compare
            for i,row in enumerate(p['rows']):compare(row,[5*i]+circle(7230,5*i),[.002]+[2e-5]*3+[2e-8]*3)


def audit(repo,stage):
    from AuditCommandNavigationOperations import compare
    manifest=load(stage/'evidence-inputs-v1.json')['artifact_sha256']
    for n,h in manifest.items():require(not Path(n).is_symlink() and sha(n)==h,'Changed regular artifact '+n)
    def bound(p):
        p=Path(p);require(str(p) in manifest,'Unbound artifact '+str(p));return p
    def archive(p):
        with tarfile.open(bound(p)) as t:return {m.name:t.extractfile(m).read() for m in t.getmembers()}
    before=load(bound(stage/'visible-inputs-before.json'));require(before==load(bound(stage/'visible-inputs-after.json')),'Changed native inputs')
    frozen=archive(stage/'executed-sources-v2.tar.gz');require({n:hashlib.sha256(v).hexdigest() for n,v in frozen.items()}==before['sources'],'Executed source archive differs')
    old=archive(stage/'source-v1.tar.gz');v1=old['src/qtgui/tests/NasaInterruptedEphemerisTests.cpp'].decode();v2=frozen['src/qtgui/tests/NasaInterruptedEphemerisTests.cpp'].decode();includes='#include <QPushButton>\n#include <QTest>\n#include <QCryptographicHash>\n';require(includes+v1.replace(includes,'')==v2,'Native assertions changed beyond include order')
    require('error: parse error in template argument list' in bound(stage/'build-v1.log').read_text(),'Original compile failure not retained')
    prior=load(bound(repo/'build/nasa-coverage/coverage95-20261103-nested-ephemeris/source-v3-review.json'))['reference_sources'];require(all(hashlib.sha256(frozen[n]).hexdigest()==prior[n] for n in CORE),'Production changed after nested repair')
    launch=load(bound(stage/'visible-launch.json'));require(launch['returncode']==0 and launch['mapped_processes']==6 and launch['inputs_unchanged'],'Wrong native launch')
    mapped=load(bound(stage/'loaded-images.json'))['processes'];selected=[];last=None;raw_count=0;trial_count=0
    for mode in ['headless','visible']:
        receipt=load(bound(repo/'build/gui-acceptance'/('interrupted-ephemeris-'+mode+'-v1')/'results.json'));require(receipt['mode']==mode and receipt.get('finished_utc') and receipt['end_reason']=='completed-selection','Nonterminal selection')
        require(all(before['sources'].get(n)==h for n,h in receipt['source_hashes'].items()),'Wrong source version');active={r['name']:r for r in receipt['results'] if r['status']!='not-run'};require(set(active)=={PREFIX+k for k in KEYS},'Wrong attempted cases')
        if mode=='visible':
            d=receipt['visible_display'];require(receipt['max_screenshots']==0 and d['resizable'] and d['start_minimized'] and 'Iconic' in d['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN' not in d['initial_host_window_state'],'Wrong desktop policy')
        for name,row in active.items():
            require(row['status']=='passed' and row['returncode']==0,'Native case did not pass');log=bound(row['log']);root=log.parent;tests=ET.parse(bound(root/'ctest.xml')).getroot().findall('testcase');require(len(tests)==1 and tests[0].get('name')==name and all(tests[0].find(n) is None for n in ['failure','error','skipped']),'Wrong JUnit outcome')
            exes=[p for p in row['registered_command'] if p in receipt['binary_hashes']];require(len(exes)==1 and before['images'].get(exes[0])==receipt['binary_hashes'][exes[0]],'Wrong native executable')
            if mode=='visible':
                require(row['trace']['native_exposure_events']>0 and not row['trace']['screenshots'],'No native exposure');traces=list(root.glob('trace-*.jsonl'));require(traces,'Missing trace')
                for trace in traces:
                    proc=mapped.get(trace.stem.removeprefix('trace-'));require(proc and proc['executable']==exes[0],'Missing mapped process')
                    for p,h in proc['images'].items():require(before['images'].get(p)==h,'Mapped image differs')
                    for p,h in before['images'].items():
                        if p==exes[0] or Path(p).name.startswith('libGmat'):require(proc['images'].get(p)==h,'Missing mapped core/test')
            proof=load(bound(root/'proof.json'));check(proof);last=proof;messages=re.findall(r'NASA_CHECK (\{[^\n]+\})',log.read_text());require(len(messages)==1 and json.loads(messages[0])==proof,'Proof/stdout mismatch');key=name.removeprefix(PREFIX);fixture='src/qtgui/tests/interrupted-ephemeris/'+key+'.json';spec=json.loads(frozen[fixture]);require(sha(bound(root/'fixture.json'))==hashlib.sha256(frozen[fixture]).hexdigest(),'Wrong executed fixture')
            origin=bound(repo/spec['origin']);require(sha(origin)==spec['origin_sha256'],'Retained mission origin changed');normal=origin.read_text().replace(spec['replaced_work_root'],'@WORK@').replace("Report 'Done' Trials Keep;","Keep=99;\nReport 'Done' Trials Keep;");require(normal==spec['recovery_script'],'Normal mission derivation changed');pos=normal.index('Achieve Inner(');require(normal[:pos]+spec['injection']+normal[pos:]==spec['script'],'Fault injection changed other source')
            for p in proof['phases']:
                phase=p['phase'];prefix=str(phase)+'-';require(load(bound(root/(prefix+'phase.json')))==p,'Phase raw/proof differs');passes=load(bound(root/(prefix+'solver-passes.json')));raw=load(bound(root/(prefix+'raw-publications.json')));require(len(raw)==p['raw_count'] and passes,'Missing raw pass grid');raw_count+=len(raw)
                starts=[r for r in csv.reader(bound(root/(prefix+'trials.csv')).open()) if len(r)==2];require(len(starts)==len(passes),'Missing raw trial controls');accepted=0;local=0
                for i,(trial,state) in enumerate(zip(starts,passes),1):
                    require(state['pass']==i and len(state['states'])==2,'Wrong pass order/clone count');truth=all(s['state']==10014 for s in state['states']);require(state['accepted']==truth,'Acceptance not derived from both solver clones');accepted+=truth
                    local+=int(not truth and any(s['name']=='Inner' and s['state']==10014 for s in state['states']));radius=7000+100*float(trial[0])+10*float(trial[1]);samples=[r for r in raw if r['pass']==i];require(len(samples)==25 and all(r['accepted']==truth for r in samples),'Wrong raw membership')
                    for j,s in enumerate(samples):compare(s['state'],circle(radius,5*j),[2e-5]*3+[2e-8]*3)
                trial_count+=len(passes);require(accepted==p['accepted_passes'] and local==p['local_accepted_outer_trial'],'Pass counts/proof differ');payload=bound(root/(prefix+'mission.oem')).read_text();data=[l for l in payload.splitlines() if re.match(r'^\d{4}-\d\d-\d\dT',l.strip())];require(len(data)==len(p['rows']),'Raw OEM census differs')
                if phase:
                    require(payload.count('META_START')==1 and all(re.search(r'^'+k+r'\s*=\s*'+v+r'\s*$',payload,re.M|re.I) for k,v in [('CENTER_NAME','EARTH'),('TIME_SYSTEM','UTC'),('REF_FRAME','EME2000')]),'Recovery OEM segment/frame differs')
                    for line,want in zip(data,p['rows']):compare([float(v) for v in line.split()[1:]],want[1:],[1e-12]*6)
                else:
                    text=bound(root/'0-messages.txt').read_text();require(('sqrt' in text.lower()) if spec['mode']=='RuntimeError' else True,'Missing native runtime error')
                source=bound(root/(prefix+'source.script')).read_text();template=spec['recovery_script'] if phase and spec['mode']!='DebuggerStop' else spec['script'];root_match=re.search(r"Export.Filename='([^']+)/Final only Δ.oem'",source);require(root_match and source==template.replace('@WORK@',root_match[1]),'Executed phase source differs')
            require(bound(root/'1-source.script').read_bytes()==bound(root/'2-source.script').read_bytes(),'Final recovery source changed');selected.append(dict(mode=mode,name=name))
    negatives=[]
    for label,mutate in [('aborted-knot',lambda p:p['phases'][0]['rows'].append([0]+circle(7230,0))),('missing-recovery',lambda p:p['phases'].pop()),('wrong-outcome',lambda p:p['phases'][0].update(outcome='Completed')),('executed-continuation',lambda p:p['phases'][0].update(Keep=99)),('stale-appended-knot',lambda p:p['phases'][2]['rows'].append(p['phases'][2]['rows'][0]))]:
        bad=copy.deepcopy(last);mutate(bad)
        try:check(bad)
        except (ValueError,AssertionError):negatives.append(label)
        else:raise ValueError('Accepted corrupt '+label)
    return dict(status='passed-bounded-interruption-and-recovery-ephemeris',qualified_keys=[],new_gui_passes=12,interrupted_phases=12,recovery_phases=24,OEM_knots=600,OEM_components=3600,raw_publication_rows=raw_count,raw_trial_passes=trial_count,negative_controls=negatives,selected=selected,production_changed_files=[],scientific_approval=False,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();v=audit(Path(__file__).resolve().parents[3],a.stage.resolve());a.output.write_text(json.dumps(v,indent=2)+'\n');print(json.dumps({k:x for k,x in v.items() if k!='selected'}))
