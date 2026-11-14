"""Read actual MATLAB high-level C-interface evidence; never infer broader gates."""
import argparse,copy,csv,hashlib,json,math,re,tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
import AuditCapiAcceptance as capi
import AuditCapiSummaryAcceptance as summary
import AuditFunctionTimeParameterBindings as clocks
from BuildNasaCoverageExtension import load,require,sha

KIND='source-versioned-MATLAB-high-level-C-interface'
KEYS=['All Others!E245','All Others!E247','All Others!E248']
PHASES=['initial','repeat','recovered','reopened']
ENCODINGS=['80','c0af','e080af','eda080','f4908080','e282','e228a1','f5808080','ff']
STOP_CASES=['GoalVariable','GoalArray','GoalProperty','FirstStepRepeat','FirstStepBeforeRoot','StopTop','StopLoop','StopTarget']

def archive(path,identities):
 with tarfile.open(path) as t:
  members=t.getmembers();require(len(members)==len(identities) and {m.name for m in members}==set(identities),'Source archive partition differs')
  result={}
  for m in members:
   require(m.isfile() and not Path(m.name).is_absolute() and '..' not in Path(m.name).parts,'Unsafe/nonregular source member')
   data=t.extractfile(m).read();require(not data.startswith((b'\x7fELF',b'DAF/SPK',b'DAF/PCK')) and hashlib.sha256(data).hexdigest()==identities[m.name],'Source archive identity differs')
   result[m.name]=data
  return result

def finite(v):
 require(type(v) in [int,float] and math.isfinite(v),'Nonfinite/missing number');return v

def check(p,directory=None):
 require(p['checkpoint']=='actual-MATLAB-high-level-C-interface' and p['new_requirement_credit']==0,'Wrong/automatic-credit checkpoint')
 require([v['phase']for v in p['phases']]==PHASES,'Missing execution/repeat/error-recovery/reopen conjunction')
 mu,r,t=398600.4415,7000,60;w=math.sqrt(mu/r**3);v=math.sqrt(mu/r)
 expected=[42,r*math.cos(w*t),r*math.sin(w*t),0,-v*math.sin(w*t),v*math.cos(w*t),0,21545+t/86400]
 for phase in p['phases']:
  require(phase['GUI_source_history_and_control_unchanged'] is True and phase['actual_command_summary'] is True,'GUI caller/history/control isolation or actual summary missing')
  values=phase['actual_report'];require(len(values)==8,'Wrong raw result width')
  require(all(abs(finite(a)-b)<(2e-10 if i==7 else 2e-7) for i,(a,b) in enumerate(zip(values,expected))),'Independent circular/control/epoch oracle failed')
  if directory:
   raw=(directory/(phase['phase']+'-report.csv')).read_text();require([float(v)for v in raw.strip().split(',')]==values,'Raw/claimed result differs')
   require(raw==(directory/'initial-report.csv').read_text(),'Repeat/recovery/reopen report bytes differ')
   text=(directory/(phase['phase']+'-summary.txt')).read_text();require('Propagate' in text and 'Report' in text,'Actual native command summary missing')
 require(set(p['errors'])=={'missing','parse','run'} and 'does not exist' in p['errors']['missing'] and 'failed to load' in p['errors']['parse'] and 'failed to run' in p['errors']['run'],'Actual file/parser/runtime errors missing')
 require([v['bytes_hex']for v in p['encoding_controls']]==ENCODINGS and all(v['rejected'] is True and v['unchanged_output_source_control'] is True for v in p['encoding_controls']),'Malformed UTF-8 rejection/isolation matrix differs')
 require([v['offset_seconds']for v in p['precision_controls']]==[1e-8,1e-6,.125,-.25,60],'Precision control partition differs')
 for v in p['precision_controls']:
  require(abs(finite(v['ElapsedSecs'])-v['offset_seconds'])<1e-12 and abs(finite(v['ElapsedDays'])-v['offset_seconds']/86400)<1e-17,'Precise positive/negative elapsed interval differs')
 switch=p['precision_mode_switch'];require(switch['before_enable']==0 and abs(finite(switch['after_enable']))<1e-12,'Precision-mode transition moved the reference')
 require(all(p[k] is True for k in ['precision_copy_Global_private_clear_and_explicit_reference','absolute_and_relative_ROOT_PATH','invalid_ROOT_PATH_rejected','actual_ReportViewer','Unicode_reopen']),'Reference/root/viewer/reopen conjunction missing')
 state=p['actual_MATLAB_state'];pid=finite(state['pid']);require(pid>1 and pid==int(pid) and state['loaded'] is True and state['unloaded_then_reopened'] is True,'Actual MATLAB PID/library-handle unload/reopen missing')
 if directory:
  for name,key in [('phase-results.json','phases'),('actual-errors.json','errors'),('encoding-controls.json','encoding_controls'),('precision-controls.json','precision_controls'),('precision-mode-switch.json','precision_mode_switch'),('actual-MATLAB-state.json','actual_MATLAB_state')]:require(load(directory/name)==p[key],'Raw proof partition differs: '+name)
  require(load(directory/'initial-C-API-state.json')==p['phases'][0]['actual_report'][1:7],'Actual public C-API state differs from the report')
  source=(directory/'caller.script').read_bytes();require('Ω € 𐐨'.encode() in source and b'StopTolerance = 1e-10' in source and b'Probe.ElapsedSecs = 60' in source,'Unicode/tight-stop actual caller differs')
  for i,encoding in enumerate(ENCODINGS):
   require((directory/f'invalid-encoding-{i}.script').read_bytes()==source+b'% Invalid encoding '+bytes.fromhex(encoding),'Malformed byte fixture differs')
   require('not valid UTF-8' in (directory/f'invalid-encoding-{i}-error.txt').read_text(),'Missing actual UTF-8 status diagnostic')
  bootstrap=(directory/'bootstrap.m').read_text()
  for name in ['startgmat','opengmat','preparescript','closegmat']:require("which('"+name+"')" in bootstrap and name+'.m' in bootstrap,'Actual shipped-helper resolution not asserted')
  require('startgmat(' in (directory/'first.m').read_text() and "'EarthForce'" in (directory/'first.m').read_text(),'High-level startgmat entry missing')

class Evidence:
 def __init__(self,repo,stage):
  self.repo,self.stage=repo,stage;self.cache={};self.contexts=[]
  self.ids=load(stage/'executed-source-sha256-v25.json');self.source=archive(stage/'executed-sources-v25.tar.gz',self.ids)
  self.supplement=load(stage/'native-source-supplement-v1.json');extra=self.supplement['source_sha256']
  require(len(extra)==10 and not set(extra)&set(self.ids) and sha(self.supplement['archive'])==self.supplement['archive_sha256'],'Different source preservation addendum')
  self.source.update(archive(self.supplement['archive'],extra));self.declared={**self.ids,**extra}
  for record in self.supplement['actual_C_client_source_archives']:
   root=Path(record['run']);receipt=load(root/'receipt.json');before=load(root/'inputs-before.json')
   require(sha(root/'receipt.json')==record['receipt_sha256'] and sha(root/'sources.tar.gz')==record['archive_sha256'] and before==load(root/'inputs-after.json') and receipt['status']=='passed' and receipt['inputs_unchanged'] is True,'Source addendum has no unchanged actual execution archive')
   frozen=archive(root/'sources.tar.gz',receipt['sources'])
   require(all(frozen[n]==self.source[n] and receipt['sources'][n]==h==before[str(repo/n)]for n,h in extra.items()),'Source addendum is not the actual execution preimage')
 def context(self,mode,test,role,argument=None):
  run='final-'+mode+'-v1'
  if run not in self.cache:
   r=load(self.stage/run/'results.json');m=load(self.stage/(run+'-loaded-images.json'));build=load(m['build_receipt'])
   require(r['mode']==mode and r.get('finished_utc') and r['end_reason']=='completed-selection' and m['returncode']==0 and m['inputs_unchanged'] is True and m['runtime_binary_copies']==0,'Nonterminal/mutated/failed final run')
   require(build['status']=='passed' and build['returncode']==0 and build['source_version']==25 and build['source_sha256']==m['source_sha256']==self.ids and sha(m['build_receipt'])==m['build_receipt_sha256'],'Source/build/execution custody differs')
   require(all(self.declared.get(n)==h for n,h in r['source_hashes'].items()),'Native run source identity differs')
   selected=[v for v in r['results'] if v['status']!='not-run'];expected={'QtGui.NasaMatlabCInterface'}|{'QtGui.NasaFunctionTimeParameterBinding'+t for t in clocks.TYPES}|{'QtGui.NativeNasaInterfaceDelivery'+t for t in ['ExplicitStartup','Positional']}|{'QtGui.NativeNasaPropagationStop'+t for t in STOP_CASES}
   require(len(selected)==len(expected)==24 and {v['name']for v in selected}==expected and all(v['status']=='passed' and v['returncode']==0 for v in selected),'Exact final new/affected matrix differs')
   union={}
   for process in m['processes']:
    require(build['binary_sha256'].get(process['executable'])==m['binary_sha256'][process['executable']],'Native descendant has no matching successful build')
    for snap in process['snapshots']:
     require(hashlib.sha256(snap['maps'].encode()).hexdigest()==snap['maps_sha256'],'Raw map hash differs')
     paths={f[5]for line in snap['maps'].splitlines() if len(f:=line.split(None,5))==6 and(f[5].startswith(str(self.repo)+'/')or f[5].startswith('/usr/local/MATLAB/R2026b/'))}
     require(paths==set(snap['project_images']) and all(not p.endswith(' (deleted)')for p in paths),'Mapped image partition/deleted image differs');union.update(snap['project_images'])
   require(union==m['loaded_project_images'],'Mapped image union differs')
   if mode=='visible':
    d=r['visible_display'];require(r['max_screenshots']==0 and d['resizable'] is True and d['start_minimized'] is True and 'Iconic' in d['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN' not in d['initial_host_window_state'],'Visible window policy differs')
   self.cache[run]=r,m
  r,m=self.cache[run];row=next(v for v in r['results']if v['name']==test);directory=Path(row['log']).parent
  cases=list(ET.parse(directory/'ctest.xml').getroot().iter('testcase'));require(len(cases)==1 and cases[0].get('name')==test and cases[0].find('failure') is None and cases[0].find('skipped') is None,'Native JUnit differs')
  processes=[p for p in m['processes']if p['role']==role and(argument is None or p['command'][-1]==argument)];require(len(processes)==1,'Ambiguous/missing native case process');process=processes[0];exe=process['executable']
  require(exe in row['registered_command'] and m['loaded_project_images'][exe]==r['binary_hashes'][exe],'Actual executable identity differs')
  trace=[json.loads(v)for v in (directory/f'trace-{process["pid"]}.jsonl').read_text().splitlines()];require(trace[0]['event']=='start' and trace[-1]['event']=='summary' and trace[-1]['screenshots']==0,'Native observer not terminal/zero-screen')
  exposures=sum(v['event']=='native_exposure' and v.get('exposed',False)for v in trace);require(exposures==row['trace']['qt_window_exposure_events'] and(mode!='visible' or exposures==row['trace']['native_exposure_events']>0),'Native exposure census differs')
  proof=load(directory/'proof.json');emitted=re.findall(r'NASA_CHECK (\{[^\n]+\})',Path(row['log']).read_text());require(len(emitted)==1 and json.loads(emitted[0])==proof,'Emitted/stored proof differs')
  self.contexts.append(dict(run=run,source_version=25,test=test,mode=mode,pid=process['pid'],visible_exposures=exposures if mode=='visible'else 0))
  return proof,directory,process,m

def stop(p,directory,spec):
 require(p['checkpoint']=='propagation-stop' and p['outcome']=='passed' and p['case']==spec['id'] and p['family']==spec['family'] and p['kind']==spec['kind'] and p['gui_idle'] is True,'Wrong stop regression identity/control return')
 r=7000;mu=398600.4415;w=math.sqrt(mu/r**3);v=math.sqrt(mu/r);fsa=spec['family']=='fsa';halt=spec['family']=='stop'
 times=[math.acos(float(spec['first_goal'])/r)/w,3*math.pi/(2*w)]if fsa else[2 if halt else spec['elapsed']]
 require(p['mu']==mu and p['radius']==r and len(p['rows'])==len(times) and p['expected_times']==times,'Stop independent time recipe differs')
 raw=[[float(v)for v in line.split(',')]for line in (directory/'results.csv').read_text().strip().splitlines()];require(raw==p['rows'],'Raw stop result differs')
 for i,(row,t)in enumerate(zip(raw,times)):
  require(len(row)==15 and all(math.isfinite(x)for x in row) and abs(row[1]-t)<(.02 if fsa else .00003),'Stop elapsed clock bound differs')
  oracle=[r*math.cos(w*t),r*math.sin(w*t),0,-v*math.sin(w*t),v*math.cos(w*t),0];require(all(abs(a-b)<(.002 if fsa else .00002)for a,b in zip(row[2:8],oracle)),'Stop circular-state oracle differs')
  require(row[0]==(1 if halt or fsa and i==0 else 2) and row[8]==int(halt) and row[9]==(41 if halt or fsa and i==0 else 42) and row[10]==(1 if halt and spec['kind']=='Loop'else -99) and row[11]==(1 if halt and spec['kind']=='Target'else 0) and row[12:14]==[90,75] and abs(row[14]-(21545+120/86400))<1e-10,'Stop source/goal/continuation controls differ')
 if fsa:require(raw[1][1]-raw[0][1]>math.pi/(2*w),'Repeated stop used the first root')
 if fsa and spec['kind']=='before-root':require(0<raw[0][2]<.001,'Before-root precondition missing')
 if halt:
  points=load(directory/'retained-plot.json');require(len(points)==p['retained_plot_samples']>0 and abs(points[-1][0]-2)<.00003,'Intentional Stop plot retention differs')

def primary_images(repo,p,d,process,m):
 require(p['actual_helpers']==str(repo/'plugins/CInterfacePlugin/matlab') and p['actual_library']==str((repo/'application/bin/libCInterface.so').resolve()),'Wrong actual shipped helpers/C library')
 require(p['actual_wrapper']==process['command'][-1],'Different actual generated wrapper')
 require(p['actual_MATLAB_state']['pid']!=process['pid'],'MATLAB/GUI PID conflated')
 images={n:h for snap in process['snapshots']for n,h in snap['project_images'].items()}
 for token in ['libGmatBase.so','libMatlabInterface.so','/R2026b/bin/glnxa64/libeng.so',p['actual_library']]:require(any(token in n for n in images),'Actual native SDK/project image absent: '+token)
 wrapper=Path(p['actual_wrapper']);receipt=load(wrapper/'wrapper-receipt.json');require(receipt['status']=='generated' and receipt['runtime_binary_copies']==0 and receipt['new_requirement_credit']==0,'Wrong generated wrapper custody')
 for n,h in {**receipt['inputs'],**receipt['generated']}.items():require(sha(n)==h,'Generated wrapper dependency/output drift: '+n)
 require(receipt['inputs'][p['actual_library']]==images[p['actual_library']],'MATLAB wrapper/current C-library image differs')
 for name in ['early-MATLAB-process-maps.txt','MATLAB-process-maps.txt']:
  raw=(d/name).read_text();paths={f[5]for line in raw.splitlines()if len(f:=line.split(None,5))==6}
  require(p['actual_library'] in paths and set(receipt['generated_thunks'])<=paths and any('/usr/local/MATLAB/R2026b/'in v for v in paths) and all(not v.endswith(' (deleted)')for v in paths if v.startswith(str(repo)+'/')),'Actual MATLAB C-library/thunk/SDK map missing/deleted')

def support(repo,stage,e):
 driver=load(stage/'support-c-client-v1.json');require(driver['returncode']==0 and driver['inputs_unchanged'] is True and driver['source_version']==25 and driver['source_sha256']==e.ids and driver['runtime_binary_copies']==driver['GUI_executions']==0,'C-client driver source/input custody differs')
 require(driver['helper_sha256']==sha(stage/'run-c-client-regressions-v1.py') and driver['build_receipt_sha256']==sha(driver['build_receipt']),'C-client runner/build identity differs')
 modes=['summary','stopped','runtimeerror','basic','stm','epoch'];records={r['mode']:r for r in e.supplement['actual_C_client_source_archives']};require(set(records)==set(modes) and len(driver['processes'])==6,'Missing/duplicate C-client mode')
 cases=list(ET.parse(stage/'support-ctest-v1.xml').getroot().iter('testcase'));names={'QtAcceptance.NasaCInterface'+m.title() for m in modes[:3]}|{'QtAcceptance.NasaCInterface'+m for m in modes[3:]};require(len(cases)==6 and {v.get('name')for v in cases}==names and all(v.find('failure')is None and v.find('skipped')is None for v in cases),'Six original registered C-client JUnit results differ')
 result=[];union={};seen=set()
 for process in driver['processes']:
  command=process['command'];mode=command[2];require(mode in modes and mode not in seen,'Duplicate/unknown native C-client process');seen.add(mode);root=Path(command[1]).parent.parent;require(str(root)==records[mode]['run'],'Actual C-client process/archive association differs')
  receipt=load(root/'receipt.json');before=load(root/'inputs-before.json');require(receipt['command']==command and before==load(root/'inputs-after.json') and receipt['returncode']==0 and receipt['status']=='passed' and receipt['inputs_unchanged'] is True,'Native C-client command/input/result differs')
  require(all(e.declared.get(n)==h for n,h in receipt['sources'].items()),'C-client execution source differs from final source/addendum')
  require(receipt['GUI_executions']==receipt['screenshots']==receipt['runtime_copies']==0 and all(receipt[k]is False for k in ['full_current_build_qualified','full_NASA_qualified','technical_core_approval']),'C-client claim exceeds scope')
  images={}
  for snap in process['snapshots']:
   require(hashlib.sha256(snap['maps'].encode()).hexdigest()==snap['maps_sha256'],'Raw C-client map hash differs')
   paths={f[5]for line in snap['maps'].splitlines()if len(f:=line.split(None,5))==6 and(f[5].startswith(str(repo)+'/')or f[5].startswith('/usr/local/MATLAB/R2026b/'))};require(paths==set(snap['project_images']),'Raw C-client mapped partition differs');images.update(snap['project_images']);union.update(snap['project_images'])
  require(process['executable']==command[0] and command[0]in images and all(before[n]==h==receipt['binaries'][n]for n,h in receipt['loaded_project_images'].items()) and all(receipt['binaries'].get(n)==h for n,h in images.items()if n.startswith(str(repo)+'/')),'Native C-client project image identity differs')
  require(all(driver['loaded_project_images'].get(n)==h and sha(n)==h for n,h in images.items()if n.startswith('/usr/local/MATLAB/R2026b/')),'Outer C-client SDK map/custody differs')
  proofs=[json.loads(v[11:])for v in (root/'client.log').read_text().splitlines()if v.startswith('NASA_CHECK ')];require(len(proofs)==1 and receipt['proofs']==proofs,'C-client emitted/recorded proof differs');p=proofs[0]
  if mode in modes[3:]:capi.validate_proof(p,mode)
  else:
   frozen=archive(root/'sources.tar.gz',receipt['sources']);member='plugins/CInterfacePlugin/src/plugin/CInterfacePluginFunctions.cpp';signature='const char *GetRunSummary()\n   {'
   original=archive(stage/'executed-sources-v1.tar.gz',load(stage/'executed-source-sha256-v1.json')) if mode=='summary'else None
   if original:require(summary.function(original[member].decode(),signature)==summary.function(frozen[member].decode(),signature),'Original summary implementation changed')
   review=dict(execution_source_sha256={mode:receipt['sources']},execution_archive_sha256={mode:sha(root/'sources.tar.gz')},summary_body_sha256=hashlib.sha256(summary.function(frozen[member].decode(),signature).encode()).hexdigest())
   artifacts={str(v):sha(v)for v in root.rglob('*')if v.is_file()and not v.is_symlink()};summary.execution(repo,root,mode,review,artifacts)
  result.append(dict(mode=mode,pid=process['pid'],run=str(root),assertions=p['assertions']))
 require(union==driver['loaded_project_images'] and seen==set(modes),'C-client image union/mode matrix differs')
 return result

def component(repo,stage):
 e=Evidence(repo,stage);proofs={};stop_specs={v['id']:v for v in json.loads(e.source['src/qtgui/tests/NasaPropagationStopCases.json'])['cases']};time_specs={v['factory_type']:v for v in json.loads(e.source['src/qtgui/tests/NasaFunctionTimeParameterCases.json'])['time_binding_cases']}
 for mode in ['headless','visible']:
  p,d,process,m=e.context(mode,'QtGui.NasaMatlabCInterface','matlab-c-interface-test');check(p,d);primary_images(repo,p,d,process,m);proofs[mode]=p
  for key in clocks.TYPES:
   p,d,_,_=e.context(mode,'QtGui.NasaFunctionTimeParameterBinding'+key,'function-time-regression',key);clocks.contract(p,time_specs[key]);require(load(d/'phases.json')==p['phases'],'Raw time phase matrix differs')
   for phase in p['phases']:
    tag=str(phase['phase']);require(load(d/(tag+'-live-parameter-samples.json'))==phase['live_samples'] and all(load(d/(tag+'-invariants.json')).values()) and load(d/(tag+'-prototypes.json'))==load(d/'prototypes-before.json'),'Raw time samples/invariants/configured prototypes differ')
    spec=time_specs[key];messages=(d/(tag+'-messages.txt')).read_text();require(clocks.written(messages,spec,spec['root_private_reference'])==phase['private_writes'] and clocks.written(messages,spec,spec['shared_reference'])==phase['shared_writes'],'Raw native time Write differs')
    values=[v['actual']for sample in phase['live_samples']for v in sample['parameters']];fields=[v.strip()for row in csv.reader((d/(tag+'-binding-values.txt')).read_text().splitlines())for v in row]
    if key.endswith('Gregorian'):require(fields==values,'Raw Gregorian time Report differs')
    else:clocks.compare([float(v)for v in fields],values,spec['absolute_bound'])
    require([float(v)for v in (d/(tag+'-values.txt')).read_text().split()]==phase['raw_markers'],'Raw time marker/control output differs')
  for key in STOP_CASES:
   p,d,_,_=e.context(mode,'QtGui.NativeNasaPropagationStop'+key,'propagation-stop-regression',key);stop(p,d,stop_specs[key])
  for key in ['ExplicitStartup','Positional']:
   p,d,_,_=e.context(mode,'QtGui.NativeNasaInterfaceDelivery'+key,'interface-delivery-regression',key);require(p['checkpoint']=='interface-delivery-'+key and p['outcome']=='passed' and p['console_processes']==3 and p['invalid_console_rejections']==2 and p['gui_toolbar_save'] is True and p['gui_toolbar_run'] is True,'Original GUI/console/encoding conjunction missing')
   data=load(d/'results-values.json');require(data['gui_report']==data['console_report'] and (d/'gui-values.csv').read_bytes()==(d/'console-values.csv').read_bytes(),'GUI/console raw outputs differ');require(len(data['processes'])==3 and data['processes'][0]['exit_code']==0 and all(v['exit_code']!=0 for v in data['processes'][1:]) and 'ASCII' in data['processes'][2]['stdout']+data['processes'][2]['stderr'],'Original console non-ASCII refusal lost')
   spec=load(d/'case.json');r,t=spec['radius'],spec['duration'];w=math.sqrt(398600.4415/r**3);v=math.sqrt(398600.4415/r);expected=[spec['input'],spec['slope']*spec['input']+spec['offset'],42,t,r*math.cos(w*t),r*math.sin(w*t),0,-v*math.sin(w*t),v*math.cos(w*t),0]
   require(len(data['gui_report'])==10 and all(abs(finite(a)-b)<(1e-12 if i<3 else 1e-5 if i<7 else 1e-8)for i,(a,b)in enumerate(zip(data['gui_report'],expected))),'Independent GUI/console scalar/circle oracle differs')
 controls=[]
 mutations=[('missing-repeat',lambda p:p['phases'].pop(1)),('wrong-orbit',lambda p:p['phases'][0]['actual_report'].__setitem__(2,0)),('mutated-caller',lambda p:p['phases'][0].update(GUI_source_history_and_control_unchanged=False)),('missing-summary',lambda p:p['phases'][0].update(actual_command_summary=False)),('lost-parser-error',lambda p:p['errors'].update(parse='accepted')),('missing-encoding-class',lambda p:p['encoding_controls'].pop()),('encoding-executed',lambda p:p['encoding_controls'][0].update(unchanged_output_source_control=False)),('rounded-10ns',lambda p:p['precision_controls'][0].update(ElapsedSecs=3.143213689e-7)),('negative-days-cancellation',lambda p:p['precision_controls'][3].update(ElapsedDays=0)),('mode-discontinuity',lambda p:p['precision_mode_switch'].update(after_enable=1.5628155612679162e-7)),('Global-reference-reset',lambda p:p.update(precision_copy_Global_private_clear_and_explicit_reference=False)),('invalid-root-accepted',lambda p:p.update(invalid_ROOT_PATH_rejected=False)),('relative-root-missing',lambda p:p.update(absolute_and_relative_ROOT_PATH=False)),('no-handle-unload',lambda p:p['actual_MATLAB_state'].update(unloaded_then_reopened=False)),('no-viewer',lambda p:p.update(actual_ReportViewer=False)),('no-Unicode-reopen',lambda p:p.update(Unicode_reopen=False)),('false-credit',lambda p:p.update(new_requirement_credit=3))]
 for label,mutate in mutations:
  p=copy.deepcopy(proofs['headless']);mutate(p)
  try:check(p)
  except(ValueError,KeyError,IndexError,TypeError):controls.append(label)
  else:raise ValueError('Accepted corrupt proof: '+label)
 clients=support(repo,stage,e)
 return dict(status='passed-component-only',qualified_keys=[],candidate_keys=KEYS,GUI_contexts=len(e.contexts),case_source_bindings=e.contexts,visible_native_exposures=sum(v['visible_exposures']for v in e.contexts),C_client_contexts=clients,C_client_assertions=sum(v['assertions']for v in clients),corrupt_proof_controls=controls,new_requirement_credit=0,scientific_approval=False,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False,MATLAB_bundle_qualified=False)

def qualify(repo,path,inventory):
 q=load(path);require(q['contract_kind']==KIND and q['qualified_keys']==KEYS,'Different exact row conjunction')
 gates=['scientific_approval','technical_core_approval','full_current_build_qualified','full_NASA_qualified','MATLAB_bundle_qualified'];require(all(q[k]is False for k in gates),'Broader qualification falsely closed')
 for n,h in q['artifact_sha256'].items():require(Path(n).is_file()and not Path(n).is_symlink()and sha(n)==h,'Changed admission artifact: '+n)
 archive(q['reader_source_archive'],q['reader_source_sha256'])
 for n,h in q['reader_source_sha256'].items():require(sha(repo/n)==h,'Executing reader differs: '+n)
 stage=Path(q['stage']);scope=load(q['scope_review']);before=load(q['prior_coverage']);overlay=load(q['prior_overlay']);known={v['key']:v for v in inventory['requirements']};row=lambda k:dict(key=k,declared_id=known[k]['declared_id'],fingerprint=known[k]['fingerprint'],source_text=known[k]['text'])
 require(len(known)==1158 and len(inventory['historical_cases'])==2367 and q['authority_sha256']==scope['authority_sha256']==inventory['sources'],'Original authority/denominator differs')
 require(scope['candidate_rows']==[row(k)for k in KEYS] and scope['parent_children']==[row('All Others!E248')] and scope['structural_rows']==[row('All Others!E'+str(i))for i in range(245,251)],'Original whole-row/parent structure differs')
 require({v['key']for v in inventory['requirements']if v['declared_id'].startswith('EIR-2.2.')and v['declared_id']!='EIR-2.2.0'}=={'All Others!E248'},'Additional original C-API high-level child omitted')
 require(before['coverage']['requirements']['verified']==1047 and before['coverage']['requirements']['total']==1158 and before['coverage']['historical_cases']['verified']==1816 and before['requirements_still_needed']==54 and set(KEYS)<={v['key']for v in before['remaining_original_rows']['requirements']},'Prior exact union/remaining rows differ')
 original={v['key']:v['original_row']for v in overlay['reviewed_source_rows']if v['key']in KEYS};require(scope['retained_original_closure_records']==original and scope['original_bundle_gates_retained']=={k:v['closure_gate']for k,v in original.items()}and len(overlay['reviewed_source_rows'])==139 and len(overlay['original_hold_keys_retained'])==59 and scope['MATLAB_bundle_qualified']is False,'Original hold/closure/bundle records lost')
 qa=load(stage/'source-qa-v1.json');require(sha(qa['core_inventory'])==qa['core_inventory_sha256']and sha(qa['prior_inventory'])==qa['prior_inventory_sha256'],'Changed core inventory')
 a={v['path']:v['working_sha256']for v in load(qa['prior_inventory'])['files']};b={v['path']:v['working_sha256']for v in load(qa['core_inventory'])['files']};require(len(a)==259 and len(b)==qa['core_paths']==263 and sum(b[n]==h for n,h in a.items())==qa['prior_core_paths_unchanged']==254,'Core inventory partition differs')
 require(sorted(n for n,h in a.items()if b[n]!=h)==qa['existing_core_increments'] and sorted(set(b)-set(a))==qa['new_pending_core_paths'] and len(qa['production_increment'])==9,'Exact pending increment differs')
 ids=load(stage/'executed-source-sha256-v25.json');frozen=archive(stage/'executed-sources-v25.tar.gz',ids);initial=archive(stage/'executed-sources-v1.tar.gz',load(stage/'executed-source-sha256-v1.json'))
 for n,h in b.items():require(sha(repo/n)==h and hashlib.sha256(frozen[n]).hexdigest()==h,'Current/executed core source differs: '+n)
 for r in qa['production_increment']:require(sha(r['preimage'])==r['before_sha256']==hashlib.sha256(initial[r['path']]).hexdigest()and sha(r['patch'])==r['patch_sha256']and r['after_sha256']==b[r['path']],'Unpreserved production increment')
 require(len(qa['old_test_sha256'])==695 and all(sha(repo/n)==h==hashlib.sha256(initial[n]).hexdigest()==hashlib.sha256(frozen[n]).hexdigest()for n,h in qa['old_test_sha256'].items()),'Original test sources/assertions changed')
 require(qa['initial_assertion_count']==len(qa['initial_new_test_assertions'])==22 and qa['final_assertion_count']==len(qa['final_new_test_assertions'])==37 and set(qa['initial_new_test_assertions'])<=set(qa['final_new_test_assertions']) and not qa['numerical_oracle_bounds_relaxed'] and not qa['C_API_header_changed'] and not qa['new_virtual_slots'] and qa['temporary_diagnostic_instrumentation_removed'],'Original new-test assertion/layout accounting differs')
 member='src/qtgui/tests/NasaMatlabCInterfaceTests.cpp';require(all(v in frozen[member]for v in [b'(i==7?2e-10:2e-7)',b'<1e-12',b'<1e-17'])and b'initial-C-API-propagation.txt'not in frozen[member],'Strict numeric bounds or diagnostic removal differ')
 for n in [member,'src/qtgui/CMakeLists.txt','src/qtgui/tests/PrepareMatlabCInterface.py','src/qtgui/tests/RunMatlabCInterfaceAcceptance.py']:require(sha(repo/n)==ids[n],'Current executed registration/test/helper differs: '+n)
 require(sha(repo/'application/bin/gmat_startup_qt.txt')==q['generated_startup_sha256'] and qa['generated_startup_changed']is False,'Generated startup changed')
 mode=load(stage/'mode-before-headless-v1/QtGui.NasaMatlabCInterface/precision-mode-switch.json');require(mode['after_enable']>1e-7,'Original mode discontinuity hidden')
 for name,version,receipt in [('mode-before-headless-v1',16,'build-receipt-v15.json'),('diagnostic-headless-v7',23,'build-receipt-v22.json')]:
  mapped=load(stage/(name+'-loaded-images.json'));build=load(stage/receipt);require(mapped['returncode']==1 and mapped['inputs_unchanged']is True and build['status']=='passed'and build['returncode']==0 and build['source_sha256']==mapped['source_sha256']==load(stage/f'executed-source-sha256-v{version}.json'),'Original strict failure has no exact successful build/source custody')
 trace=(stage/'diagnostic-headless-v7/QtGui.NasaMatlabCInterface/initial-C-API-propagation.txt').read_text();require('model_precision=0 state_precision=0'in trace and 'owner_precision=0'in trace and 'tolerance=1e-10'in trace and 'stop_difference=-1.9557774066925049e-07'in trace,'Original nonprecision strict-stop failure lost')
 result=component(repo,stage);require(result['GUI_contexts']==48 and len(result['C_client_contexts'])==6 and result['C_client_assertions']==3806 and result['visible_native_exposures']==282 and len(result['corrupt_proof_controls'])==17,'Complete component matrix differs')
 q['audit_summary']=dict(result,qualified_keys=KEYS,GUI_case_contexts=48,proposed_exact_rows=3,retained_original_hold_identities=59,core_paths=263,unchanged_prior_core_paths=254,existing_core_increments=5,new_pending_core_paths=4,scope='Actual MATLAB script interface and high-level C-API child/sole-child parent only;all broader original gates remain open')
 return q,KEYS

if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);selection=p.add_mutually_exclusive_group(required=True);selection.add_argument('--stage',type=Path);selection.add_argument('--qualification',type=Path);p.add_argument('--output',type=Path,required=True);a=p.parse_args();repo=Path(__file__).resolve().parents[3]
 if a.qualification:
  import NasaGuiCoverage as nasa
  q,keys=qualify(repo,a.qualification,nasa.inventory(repo));result=dict(status='passed',qualified_keys=keys,summary=q['audit_summary'])
 else:result=component(repo,a.stage.resolve())
 require(not a.output.exists(),'Refuse overwrite');a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
