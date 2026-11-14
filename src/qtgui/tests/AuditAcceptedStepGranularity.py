"""Verify the native granularity counterexample; this awards no NASA row."""
import argparse,hashlib,json,math,re,tarfile
from pathlib import Path
import xml.etree.ElementTree as ET

def load(p):return json.loads(Path(p).read_text())
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def require(ok,message):
    if not ok:raise ValueError(message)

def audit(root):
    root=Path(root).resolve();repo=Path(__file__).resolve().parents[3];consumed=set();selected=[];archive_cache={}
    def archived(p):
        p=Path(p)
        if p not in archive_cache:
            with tarfile.open(p) as t:
                members=[m for m in t if m.isfile()];require(len(members)==len({m.name for m in members}),'Duplicate source archive entries')
                archive_cache[p]={m.name:t.extractfile(m).read() for m in members}
        consumed.add(p);return archive_cache[p]
    guard_path=root/'ground-step-preflight-build-v1.json';guard=load(guard_path);consumed.add(guard_path)
    require(guard['successful_build_exit_code']==0 and sha(guard['successful_build_log'])==guard['successful_build_log_sha256'],'Unbound successful build')
    log=Path(guard['successful_build_log']).read_text();require('Linking CXX executable src/qtgui/GmatQtAcceptedStepGranularityTests' in log and not any(s in log for s in ['FAILED:','error:','build stopped']),'No successful target link')
    require(sha(guard['compile_input_archive'])==guard['compile_input_archive_sha256'],'Changed compiled source bytes')
    recovered={n:hashlib.sha256(b).hexdigest() for n,b in archived(guard['compile_input_archive']).items()}
    require(recovered==guard['compiled_sources'] and len(recovered)==139,'Incomplete compilation source archive');consumed.add(Path(guard['successful_build_log']))
    before_tables=[]
    for mode,stem in [('headless','ground-step-preflight-headless-v3'),('visible','ground-step-preflight-visible-v1')]:
        run=root/stem;aggregate=load(run/'results.json');require(aggregate['finished_utc'] and aggregate['end_reason']=='completed-selection','Run incomplete')
        before,after=[load(root/(stem+'-inputs-'+p+'.json')) for p in ['before','after']];require(before==after,'Run input drift');before_tables.append(before)
        reuse=load(root/(stem+'-source-reuse.json'))
        for field in ['receipt','reference','delta']:
            require(sha(reuse[field])==reuse[field+'_sha256'],'Source capsule drift');consumed.add(Path(reuse[field]))
        sources={**archived(reuse['reference']),**archived(reuse['delta'])}
        for name,digest in aggregate['source_hashes'].items():
            if name in before['sources']:require(before['sources'][name]==digest,'Pre-run/source mismatch')
            external=reuse['recorded_authority_ELF_or_existing_physical_inputs_not_copied']
            if name in external:require(external[name]==digest,'External identity differs')
            else:require(name in sources and hashlib.sha256(sources[name]).hexdigest()==digest,'Exact executed source unavailable')
        executable=str(repo/guard['executable']);require(aggregate['binary_hashes'][executable]==before['binaries'][executable]==guard['executable_sha256'],'Wrong native executable')
        for name,digest in guard['compiled_sources'].items():
            if name in before['sources']:require(before['sources'][name]==digest,'Compiled/pre-run source mismatch')
        for name in aggregate['binary_hashes'].keys() & before['binaries'].keys():require(aggregate['binary_hashes'][name]==before['binaries'][name],'Runtime image mismatch')
        case=run/'QtGui.AcceptedStepGranularity';result=load(case/'result.json');proof=load(case/'proof.json')
        require(result['status']=='passed' and result['returncode']==0 and result['requested_visibility']==mode and [x for x in aggregate['results'] if x['name']==result['name']]==[result],'Selected case/mode not passed')
        text=(case/'test.log').read_text();require([json.loads(line.split('NASA_CHECK ',1)[1]) for line in text.splitlines() if 'NASA_CHECK ' in line]==[proof],'Full process/proof differs')
        tests=list(ET.parse(case/'ctest.xml').getroot().iter('testcase'));require(len(tests)==1 and tests[0].get('name')==result['name'] and tests[0].find('failure') is None and tests[0].find('error') is None,'JUnit outcome differs')
        output='\n'.join(x.text or '' for x in tests[0].findall('system-out'));prefix=output.rsplit('...\n[This part',1)[0]
        require(prefix and prefix in re.sub(r'(?m)^\d+: ?', '',text),'JUnit/process output differs')
        steps=load(case/'accepted-steps.json');require(proof['outcome']=='passed' and proof['checkpoint']=='accepted-step-granularity' and proof['accepted_steps']==steps and len(steps)==10 and proof['fixed_interval_seconds']==100 and proof['maximum_step_seconds']==10 and proof['raw_publications_during_interval']==0,'Granularity identity/count differs')
        for index,row in enumerate(steps):
            seconds=20+10*index;rate=math.sqrt(398600.4415/7000**3);angle=rate*seconds
            truth=[7000*math.cos(angle),7000*math.sin(angle),0,-7000*rate*math.sin(angle),7000*rate*math.cos(angle),0]
            require(abs(row['seconds']-seconds)<1e-8 and len(row['state'])==6 and all(math.isfinite(v) and abs(v-w)<(3e-5 if i<3 else 4e-8) for i,(v,w) in enumerate(zip(row['state'],truth))),'Independent accepted-state/grid differs')
        if mode=='visible':
            desktop=aggregate['visible_display'];require(desktop['resizable'] and desktop['start_minimized'] and 'window state: Iconic' in desktop['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN' not in desktop['initial_host_window_state'] and not desktop['resize_errors'] and result['trace']['native_exposure_events']>0,'Visible resizable/minimized/nonfullscreen evidence missing')
        consumed.update([run/'results.json',root/(stem+'-inputs-before.json'),root/(stem+'-inputs-after.json'),root/(stem+'-source-reuse.json')]);consumed.update(p for p in case.iterdir() if p.is_file() and not p.is_symlink())
        selected.append(dict(mode=mode,receipt=str(case/'result.json'),native_exposures=result['trace']['native_exposure_events']))
    a,b=[x['sources'] for x in before_tables];production={n for n in a.keys() & b.keys() if '/tests/' not in n and Path(n).suffix in ['.cpp','.hpp']};require(all(a[n]==b[n] for n in production),'Production changed during diagnostic')
    snapshot=root/'event-starter-coverage-snapshot.json';require(load(snapshot)['coverage']['requirements']['verified']==1003,'Prior coverage snapshot differs');consumed.add(snapshot)
    return dict(status='passed-native-step-granularity-diagnostic',selected=selected,NASA_credit=0,qualified_keys=[],GUI_mechanism_credit=0,production_changes=[],stable_production_sources=len(production),compiled_dependencies=139,consumed_sha256={str(p):sha(p) for p in sorted(consumed)},auditor_sha256=sha(__file__),scope='Test-only RK89 primitive recording with independent circle/grid. Establishes the output-packet sampling proposal is insufficient; does not implement or qualify integration-step skipping.')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();require(not a.output.exists(),'Refusing to overwrite')
    result=audit(a.root);a.output.write_text(json.dumps(result,indent=2)+'\n');print('Verified native granularity in',len(result['selected']),'modes;',len(result['consumed_sha256']),'consumed hashes; zero NASA credit')
