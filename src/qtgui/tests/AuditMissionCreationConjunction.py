"""Conjoin actual GUI creation locations with separately accepted execution evidence."""
from pathlib import Path
import hashlib,json,tarfile

KEY='All Others!E160'
FINGERPRINT='19e093e1603fa63c846972b82e268bbdb5c3d831a0364b34ffdba424a2cc6e55'
BASE=['Empty','Many','BlockBefore','BlockAfter','EndBefore','EndAfter','BranchAppend','LeafBefore','LeafAfter']
PARENTS=['BlockBefore','BlockAfter','EndBefore','EndAfter','BranchAppend']
ELSE=PARENTS+['ElseBefore','ElseAfter']
FAMILIES=['Optimize','Target','If','While','For','BeginFiniteBurn','EndFiniteBurn','Maneuver','Assignment','Toggle','PenUp','PenDown','Propagate','Report','Save','ScriptEvent','RunSimulator','RunEstimator','CallGmatFunction','CallMatlabFunction','Stop']
MISSING_BASE=['Assignment','ScriptEvent','RunSimulator','RunEstimator','CallGmatFunction','CallMatlabFunction','Stop']
load=lambda p:json.loads(Path(p).read_text())
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()

def need(value,message):
    if not value:raise ValueError(message)

def parent_for(position):
    if position in ['LeafBefore','LeafAfter']:return 'Inner'
    if position in ['EndBefore','BranchAppend','ElseBefore','ElseAfter']:return 'Outer'
    return ''

def emitted(run,proof):
    records=[json.loads(line.split('NASA_CHECK ',1)[1]) for line in
             (run/'test.log').read_text().splitlines() if 'NASA_CHECK ' in line]
    need(records==[proof],'Creation proof differs from executed output')

def check_groups(index):
    for group in index['new_groups']:
        receipt=load(group['receipt']);build=load(group['build'])
        need(receipt['returncode']==0 and receipt['inputs_unchanged'] and
             build['status']=='passed' and build['inputs_unchanged'],'Creation inputs changed during execution')
        need(all(receipt['source_sha256'].get(p)==h for p,h in build['source_sha256'].items()),
             'Executed creation source differs from build')
        for p,h in build['binary_sha256'].items():
            # The build records linker names; the run records their versioned
            # shared-library targets. Compare recorded identities without
            # resolving a possibly newer live symlink during a later replay.
            matches=[n for n,digest in receipt['image_sha256'].items() if digest==h and
                     Path(n).parent==Path(p).parent and
                     (n==p or Path(n).name.startswith(Path(p).name+'.'))]
            need(len(matches)==1,'Executed creation image differs from build: '+p)
        with tarfile.open(group['archive']) as bundle:
            need(set(bundle.getnames())==set(build['source_sha256']),'Creation source archive is incomplete')
            for name,h in build['source_sha256'].items():
                need(hashlib.sha256(bundle.extractfile(name).read()).hexdigest()==h,'Creation source archive changed')
            plans={}
            for name in ['MissionCreationPositionCases.json','MissionCreationElseBoundaryCases.json']:
                full='src/qtgui/tests/'+name
                if full in bundle.getnames():plans[name]={v['key']:v for v in json.loads(bundle.extractfile(full).read())['cases']}
        aggregate=load(Path(group['directory'])/'results.json')
        rows=[v for v in aggregate['results'] if v['status']!='not-run']
        need(aggregate['end_reason']=='completed-selection' and len(rows)==group['cases'] and
             all(v['status']=='passed' and v['returncode']==0 for v in rows),'Incomplete creation selection')
        for row in rows:
            run=Path(group['directory'])/row['name'];spec=load(run/'case.json')
            need(row==load(run/'result.json'),'Creation result differs from aggregate')
            candidates=[v for v in row['registered_command'] if v.endswith('.json')]
            need(len(candidates)==1 and spec==plans[Path(candidates[0]).name][spec['key']],
                 'Executed creation case differs from frozen build input')
        logs=[Path(p).read_text() for p in group['native_loader_logs']]
        need(len(logs)==group['cases'] and all('initialize program:' in t and
             'GmatQtMissionCreationPositionTests' in t and 'calling init:' in t and
             'platforms/libqxcb.so' in t and 'libqoffscreen' not in t for t in logs),
             'Native creation platform lacks loader evidence')

def check_new(spec,proof,files):
    need(proof['status']=='passed' and proof['case']==spec['key'] and proof['family']==spec['family'] and
         proof['outer_context']==spec['outer_context'] and proof['native_missions_run']==0,'Wrong creation procedure/result')
    values=proof['positions'];need([v['position']for v in values]==spec['positions'],'Creation position missing')
    need(json.loads(files['case.json'])==spec and json.loads(files['positions.json'])==values,'Input or independent raw position receipt differs')
    for value in values:
        pos=value['position'];count=3 if pos=='Many'else 1
        need(value['commands']==count and value['parent']==parent_for(pos) and value['native_graph_checked'] and
             value['source_history_exact'] and value['unicode_reopen'],'Position/source/graph assertion missing')
        initial=files[pos+'-initial.script'].decode();accepted=files[pos+'-accepted.script'].decode()
        need(initial.endswith(('' if pos=='Empty'else spec['body'])+'% Preserved creation tail Δ\n'),'Wrong initial branch body')
        need(files[pos+'-mission-Ω.script'].decode()==accepted,'Saved/reopened mission differs')
        stripped=accepted
        tree=json.loads(files[pos+'-hierarchy.json']);native=json.loads(files[pos+'-native-graph.json'])
        need(len(value['edits'])==count,'Missing creation edit')
        for i,edit in enumerate(value['edits']):
            name='Probe'+str(i);statement=spec['statement'].replace('$PROBE$',name)+'\n'
            need(edit==dict(label=name,statement=statement,cancelled_first=i==0,undo_redo=True,clean_apply=pos=='Many'),'Changed creation transaction')
            need(stripped.count(statement)==1,'Missing/duplicate inserted source');stripped=stripped.replace(statement,'',1)
            if pos in ['Empty','Many']:need(accepted.endswith(statement) if i==count-1 else True,'Append not at end')
            else:
                anchor=spec['outer_anchor'] if pos=='BlockBefore'else spec['leaf_anchor']if pos in ['LeafBefore','LeafAfter']else spec['else_anchor']if pos in ['ElseBefore','ElseAfter']else spec.get('append_anchor',spec['closing_anchor'])if pos=='BranchAppend'else spec['closing_anchor']
                after=pos in ['BlockAfter','EndAfter','LeafAfter','ElseAfter']
                need((anchor+statement if after else statement+anchor)in accepted,'Inserted source is on the wrong side of its literal anchor')
            nodes=[n for n in tree if n['label']==name];matches=[n for n in native if n['label']==name]
            need(len(nodes)==len(matches)==1 and nodes[0]['type']==matches[0]['type']==spec['native_type'],'Wrong native/visible command identity')
            owner=parent_for(pos);node=nodes[0]
            need((node['parent']<0 if not owner else tree[node['parent']]['label']==owner)and matches[0]['parent']==owner,'Wrong independently traversed parent')
            if pos in spec.get('branch_expectations',{}):need(matches[0]['branch_index']==spec['branch_expectations'][pos],'Wrong If/Else arm')
        need(stripped==initial,'Creation modified unrelated source bytes')

def audit_index(index):
    need(index['families']==FAMILIES,'Original named-command universe changed')
    check_groups(index)
    old=index['retained_primary'];new=index['new_records'];coverage={};aggregates={}
    retained_names={Path(record['run']).name for record in old}
    for record in old:
        run=Path(record['run']);result=load(run/'result.json');proof=load(run/'proof.json')
        need(result['status']=='passed'and result['returncode']==0 and proof['outcome']=='passed','Retained creation execution failed')
        emitted(run,proof)
        aggregate_path=run.parent/'results.json'
        if aggregate_path not in aggregates:
            full=load(aggregate_path)
            aggregates[aggregate_path]=dict(source_hashes=full['source_hashes'],binary_hashes=full['binary_hashes'],
                results=[row for row in full['results'] if row['name'] in retained_names])
            del full
        aggregate=aggregates[aggregate_path]
        need(result in aggregate['results'] and aggregate['source_hashes'] and aggregate['binary_hashes'],
             'Retained creation has no executed source/image identity')
        need(proof['family']==record['variant'] and record['family']==('If' if record['variant']=='IfElse' else record['variant']),
             'Retained creation family was relabelled')
        need(result['requested_visibility']==record['profile'],'Retained creation profile changed')
        if record['profile']=='visible':need(result['trace']['native_exposure_events']>0,'Retained creation was not visible')
        found={v['position']for v in proof['positions']}if isinstance(proof.get('positions'),list)else{proof['position']}
        need(found==set(record['positions']),'Retained location mapping changed')
        need(proof['raw_native_values']>0 and proof['cancelled_drafts']==len(found) and proof['missions']==len(found)
             if 'raw_native_values' in proof else proof['phases'] and proof['edits'] and proof['final'],
             'Retained creation omitted native execution or GUI transaction checks')
        coverage.setdefault((record['variant'],record['profile']),set()).update(found)
    for record in new:
        run=Path(record['run']);proof=load(run/'proof.json');result=load(run/'result.json');spec=load(run/'case.json')
        need(record['family']==spec['family'] and record['context']==spec['outer_context'],'Relabelled creation context')
        need(spec['native_type']=={'Assignment':'GMAT','ScriptEvent':'BeginScript'}.get(spec['family'],spec['family']),'Wrong family/native-type map')
        need(spec['positions']==(BASE if spec['outer_context']=='For'else ELSE if spec['outer_context']=='IfElse'else PARENTS),'Wrong position partition')
        if spec['outer_context']=='IfElse':
            need(spec['append_anchor']=='Else;\n' and spec['branch_expectations']=={'ElseBefore':0,'ElseAfter':1,'EndBefore':1,'BranchAppend':0},'Changed native If/Else append semantics')
        need(result['status']=='passed'and result['returncode']==0,'New creation check did not pass')
        emitted(run,proof)
        need(result['requested_visibility']==record['profile'],'Wrong GUI profile')
        need({'Mission','commandTemplate','commandSource'}<=set(result['trace']['named_objects']),
             'Actual tree, template and source editor were not observed')
        if record['profile']=='visible':need(result['trace']['native_exposure_events']>0 and proof['native_platform']=='xcb','No native visible exposure')
        else:need(record['native_platform']=='xcb','No observed native headless platform')
        need(record['group'] in range(len(index['new_groups'])) and
             run.parent==Path(index['new_groups'][record['group']]['directory']),'Unbound creation run group')
        files={p.name:p.read_bytes()for p in run.iterdir()if p.is_file()and p.suffix in ['.json','.script']}
        check_new(spec,proof,files)
        key=(record['family'],record['context'],record['profile']);need(key not in coverage,'Duplicate new location credit')
        coverage[key]=set(spec['positions'])
    for family in FAMILIES:
        if family in MISSING_BASE:
            for profile in ['headless','visible']:need(coverage.get((family,'For',profile))==set(BASE),'Missing primary location family '+family)
        else:
            for variant in (['If','IfElse'] if family=='If' else [family]):
                for profile in ['headless','visible']:
                    need(set(BASE)<=coverage.get((variant,profile),set()),'Missing retained primary family/profile '+variant+'/'+profile)
        for context in ['While','Target','Optimize','IfElse']:
            for profile in ['headless','visible']:
                need(coverage.get((family,context,profile))==set(ELSE if context=='IfElse'else PARENTS),'Missing parent/location/profile conjunction '+str((family,context,profile)))
    need(len(new)==182,'New creation case count changed')
    return dict(status='passed',families=21,new_GUI_case_passes=len(new),new_location_witnesses=sum(len(v['positions'])for v in index['new_records']),retained_primary_cases=len(old),native_missions_replayed=0)

def qualify(repo,path,inventory,admitted):
    q=load(path);known={v['key']:v for v in inventory['requirements']}
    need(q['contract_kind']=='complete-mission-tree-creation-conjunction'and q['qualified_keys']==[KEY],'Different creation scope')
    need(q['authority_sha256']==inventory['sources'] and q['candidate_rows']==[known[KEY]]and known[KEY]['fingerprint']==FINGERPRINT,'Original source row changed')
    children=['All Others!E'+str(i)for i in range(161,182)]
    need(set(children)|{'FRC!E7','FRC!E55','FRR!E477'}<=set(admitted),'Creation family lacks its accepted configuration/execution constituents')
    need(q['children']==[known[k]for k in children],'Declared command children changed')
    for p,h in q['artifact_sha256'].items():need(sha(p)==h,'Changed creation evidence '+p)
    decision=load(q['scope_decision']);need(decision['answer']=='Yes—preserve GMAT’s normal context rules'and KEY in decision['applies_to'],'Missing native context decision')
    index=load(q['evidence_index']);need(all(q['artifact_sha256'].get(p)==h for p,h in index['artifact_sha256'].items()),'Unbound creation witness')
    result=audit_index(index)
    protection=load(q['protected_check']);need(protection==dict(status='passed',count=1334,unchanged=1333,authorized_display_helper_changes=['src/qtgui/tests/VisibleTestDisplay.py'],unexpected_changes=[],original_assertions_changed=0),'Protected input policy changed')
    need(all(q[k]is False for k in ['scientific_approval','technical_core_approval','full_current_build_qualified','full_NASA_qualified']),'Unsupported qualification gate')
    result.update(qualified_keys=[KEY],whole_row_conjunction=True,new_requirement_credit=1,source_versioned_only=True,original_assertions_changed=0,all_physical_domains_claimed=False,full_current_build_qualified=False,full_NASA_qualified=False)
    return q,[KEY],result
