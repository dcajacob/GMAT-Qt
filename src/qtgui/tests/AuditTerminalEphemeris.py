"""Audit final-pass membership with native convergence status kept explicit."""
import argparse,copy,csv,hashlib,json,re,tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
from BuildNasaCoverageExtension import require,sha,load
from AuditNestedEphemeris import circle
from AuditCommandNavigationOperations import compare
PREFIX='QtGui.NasaTerminalEphemeris';KEYS=['NestedDC','NestedYukon']
CORE=['src/base/subscriber/EphemerisFile.cpp','src/base/subscriber/EphemerisFile.hpp','src/base/command/SolverBranchCommand.cpp','src/base/command/SolverBranchCommand.hpp']


def check(proof):
    require(proof['outcome']=='passed' and proof['family'] in KEYS and [p['phase'] for p in proof['phases']]==[0,1],'Wrong terminal/recovery census')
    outer='DC' if proof['family']=='NestedDC' else 'Opt'
    for p in proof['phases']:
        phase=p['phase'];require(p['converged']==bool(phase) and p['terminal_passes']==1 and p['segments']==1 and len(p['rows'])==25,'Missing terminal output/status distinction')
        require(len(p['terminal_states'])==2 and {s['name'] for s in p['terminal_states']}=={outer,'Inner'},'Wrong terminal clone census')
        for s in p['terminal_states']:require(s['state']==10014 and s['status']==(-1 if s['name']==outer and not phase else 1),'Wrong native convergence status')
        compare([p['U'],p['V'],p['radius']],[2 if phase else 1,3,7230 if phase else 7130],[1e-6,1e-6,2e-5])
        require(len(p['passes'])>=3 and sum(r['terminal_pass'] for r in p['passes'])==1 and p['raw_count']>0,'Missing trial/final partition')
        for r in p['passes']:require(len(r['states'])==2 and r['terminal_pass']==all(s['state']==10014 for s in r['states']),'Pass classification not derived from both native clones')
        for i,row in enumerate(p['rows']):compare(row,[5*i]+circle(7230 if phase else 7130,5*i),[.002]+[2e-5]*3+[2e-8]*3)


def audit(repo,stage):
    manifest=load(stage/'evidence-inputs-v1.json')['artifact_sha256']
    for n,h in manifest.items():require(not Path(n).is_symlink() and sha(n)==h,'Changed regular artifact '+n)
    def bound(p):
        p=Path(p);require(str(p) in manifest,'Unbound artifact '+str(p));return p
    before=load(bound(stage/'visible-inputs-before.json'));require(before==load(bound(stage/'visible-inputs-after.json')),'Changed native inputs')
    with tarfile.open(bound(stage/'executed-sources-v1.tar.gz')) as t:frozen={m.name:t.extractfile(m).read() for m in t.getmembers()}
    require({n:hashlib.sha256(v).hexdigest() for n,v in frozen.items()}==before['sources'],'Executed source archive differs')
    prior=load(bound(repo/'build/nasa-coverage/coverage95-20261103-nested-ephemeris/source-v3-review.json'))['reference_sources'];require(all(hashlib.sha256(frozen[n]).hexdigest()==prior[n] for n in CORE),'Core changed after nested repair')
    require('the last calculated values are applied' in frozen['doc/help/src/Command_Target.xml'].decode() and 'the last' in frozen['doc/help/src/Command_Optimize.xml'].decode(),'Native nonconverged control guidance absent')
    launch=load(bound(stage/'visible-launch.json'));require(launch['returncode']==0 and launch['mapped_processes']==2 and launch['inputs_unchanged'],'Wrong visible launch');mapped=load(bound(stage/'loaded-images.json'))['processes'];selected=[];last=None;raw_total=0;trials_total=0
    for mode in ['headless','visible']:
        receipt=load(bound(repo/'build/gui-acceptance'/('terminal-ephemeris-'+mode+'-v1')/'results.json'));require(receipt['mode']==mode and receipt.get('finished_utc') and receipt['end_reason']=='completed-selection','Nonterminal selection');require(all(before['sources'].get(n)==h for n,h in receipt['source_hashes'].items()),'Wrong source version');active={r['name']:r for r in receipt['results'] if r['status']!='not-run'};require(set(active)=={PREFIX+k for k in KEYS},'Wrong selected cases')
        if mode=='visible':
            d=receipt['visible_display'];require(receipt['max_screenshots']==0 and d['resizable'] and d['start_minimized'] and 'Iconic' in d['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN' not in d['initial_host_window_state'],'Wrong native window policy')
        for name,row in active.items():
            require(row['status']=='passed' and row['returncode']==0,'Case did not pass');log=bound(row['log']);root=log.parent;tests=ET.parse(bound(root/'ctest.xml')).getroot().findall('testcase');require(len(tests)==1 and tests[0].get('name')==name and all(tests[0].find(n) is None for n in ['failure','error','skipped']),'Wrong JUnit outcome');exes=[p for p in row['registered_command'] if p in receipt['binary_hashes']];require(len(exes)==1 and before['images'].get(exes[0])==receipt['binary_hashes'][exes[0]],'Wrong native executable')
            if mode=='visible':
                require(row['trace']['native_exposure_events']>0 and not row['trace']['screenshots'],'Native exposure missing');traces=list(root.glob('trace-*.jsonl'));require(traces,'Missing trace')
                for trace in traces:
                    proc=mapped.get(trace.stem.removeprefix('trace-'));require(proc and proc['executable']==exes[0],'Missing mapped process')
                    for p,h in proc['images'].items():require(before['images'].get(p)==h,'Mapped image differs')
                    for p,h in before['images'].items():
                        if p==exes[0] or Path(p).name.startswith('libGmat'):require(proc['images'].get(p)==h,'Missing mapped core/test')
            proof=load(bound(root/'proof.json'));check(proof);last=proof;records=re.findall(r'NASA_CHECK (\{[^\n]+\})',log.read_text());require(len(records)==1 and json.loads(records[0])==proof,'Proof/stdout mismatch');key=name.removeprefix(PREFIX);fixture='src/qtgui/tests/terminal-ephemeris/'+key+'.json';spec=json.loads(frozen[fixture]);require(sha(bound(root/'fixture.json'))==hashlib.sha256(frozen[fixture]).hexdigest(),'Fixture/process mismatch');origin=bound(repo/spec['origin']);require(sha(origin)==spec['origin_sha256'],'Retained mission origin changed');normal=origin.read_text().replace(spec['replaced_work_root'],'@WORK@');require(normal==spec['recovery_script'] and normal.replace(spec['field'],spec['field'].split('=')[0]+'='+str(spec['limit'])+';')==spec['script'],'Iteration limit changed unrelated source')
            for p in proof['phases']:
                phase=p['phase'];prefix=str(phase)+'-';require(load(bound(root/(prefix+'rows.json')))==p['rows'] and load(bound(root/(prefix+'solver-passes.json')))==p['passes'] and load(bound(root/(prefix+'terminal-states.json')))==p['terminal_states'],'Raw proof fields differ');raw=load(bound(root/(prefix+'raw-publications.json')));require(len(raw)==p['raw_count'],'Raw publication count differs');raw_total+=len(raw);starts=[r for r in csv.reader(bound(root/(prefix+'trials.csv')).open()) if len(r)==2];require(len(starts)==len(p['passes']),'Raw trial controls missing')
                for i,(trial,s) in enumerate(zip(starts,p['passes']),1):
                    require(s['pass']==i,'Raw pass order changed');compare([s['U'],s['V']],[float(v) for v in trial],[1e-12]*2);radius=7000+100*s['U']+10*s['V'];samples=[r for r in raw if r['pass']==i];require(len(samples)==25 and all(r['terminal_pass']==s['terminal_pass'] for r in samples),'Raw trial/final membership differs')
                    for j,v in enumerate(samples):compare(v['state'],circle(radius,5*j),[2e-5]*3+[2e-8]*3)
                trials_total+=len(starts);payload=bound(root/(prefix+'mission.oem')).read_text();data=[l for l in payload.splitlines() if re.match(r'^\d{4}-\d\d-\d\dT',l.strip())];require(len(data)==25 and payload.count('META_START')==1 and all(re.search(r'^'+k+r'\s*=\s*'+v+r'\s*$',payload,re.M|re.I) for k,v in [('CENTER_NAME','EARTH'),('TIME_SYSTEM','UTC'),('REF_FRAME','EME2000')]),'OEM segment/frame/census differs')
                for line,want in zip(data,p['rows']):compare([float(v) for v in line.split()[1:]],want[1:],[1e-12]*6)
                source=bound(root/(prefix+'source.script')).read_text();template=spec['recovery_script' if phase else 'script'];root_match=re.search(r"Export.Filename='([^']+)/Final only Δ.oem'",source);require(root_match and source==template.replace('@WORK@',root_match[1]),'Executed phase source changed')
            selected.append(dict(mode=mode,name=name))
    negatives=[]
    for label,mutate in [('convergence-confusion',lambda p:p['phases'][0]['terminal_states'][0].update(status=1)),('wrong-terminal-control',lambda p:p['phases'][0].update(U=2)),('trial-trajectory',lambda p:p['phases'][0]['rows'][0].__setitem__(1,7030)),('missing-recovery',lambda p:p['phases'].pop()),('extra-knot',lambda p:p['phases'][1]['rows'].append(p['phases'][1]['rows'][0]))]:
        bad=copy.deepcopy(last);mutate(bad)
        try:check(bad)
        except (ValueError,AssertionError):negatives.append(label)
        else:raise ValueError('Accepted corrupt '+label)
    return dict(status='passed-bounded-terminal-pass-versus-convergence-evidence',qualified_keys=[],new_gui_passes=4,iteration_limited_phases=4,converged_recovery_phases=4,OEM_knots=200,OEM_components=1200,raw_publication_rows=raw_total,raw_trial_passes=trials_total,negative_controls=negatives,selected=selected,production_changed_files=[],scientific_approval=False,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();v=audit(Path(__file__).resolve().parents[3],a.stage.resolve());a.output.write_text(json.dumps(v,indent=2)+'\n');print(json.dumps({k:x for k,x in v.items() if k!='selected'}))
