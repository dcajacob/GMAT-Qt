"""Classify the complete recorded factory map and bind retained function witnesses.

Roles and a factory's nominal base-class global flag do not establish main-use,
function-use or Global eligibility. Unknown operations remain explicit.
"""
import argparse,collections,hashlib,json,re,tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
from BuildNasaCoverageExtension import load,require,sha


def sources(stage):
    with tarfile.open(stage/'eligibility-reference-sources-v1.tar.gz') as t:
        require(all(v.isfile() for v in t.getmembers()),'Nonregular role source capsule')
        return {v.name:t.extractfile(v).read().decode() for v in t.getmembers()}


def review(repo,stage):
    reference=sources(stage)
    anchors={}
    def anchor(name,needle):
        text=reference[name];require(needle in text,'Role source anchor missing '+needle)
        key=name+'#'+needle;anchors[key]=dict(source=name,sha256=hashlib.sha256(text.encode()).hexdigest(),line=text[:text.index(needle)].count('\n')+1,anchor=needle)
        return key
    moderator='src/base/executive/Moderator.cpp';interpreter='src/base/interpreter/Interpreter.cpp';base='src/base/foundation/GmatBase.cpp'
    general=anchor(interpreter,'allObjectTypeList.clear()');creation=anchor(moderator,'GmatBase* Moderator::CreateObject(');configuration=anchor('src/base/configs/ConfigManager.cpp','void ConfigManager::AddObject(')
    global_route=anchor('plugins/GmatFunctionPlugin/src/base/command/Global.cpp','bool Global::Execute()');global_store=anchor('src/base/command/ManageObject.cpp','bool ManageObject::InsertIntoGOS(');dependent=anchor('src/base/function/UserDefinedFunction.cpp','bool UserDefinedFunction::IsAutomaticObjectGlobal(')
    automatic=anchor(base,'GmatBase::AUTOMATIC_GLOBAL_FLAGS[')
    raw=re.sub(r'//[^\n]*|/\*.*?\*/','',reference[base],flags=re.S)
    names=re.findall(r'"([^"\n]+)"',re.search(r'GmatBase::OBJECT_TYPE_STRING\[[^;]+?=\s*\{(.*?)\};',raw,re.S)[1])
    flags=[v=='true' for v in re.findall(r'\b(true|false)\b',re.search(r'GmatBase::AUTOMATIC_GLOBAL_FLAGS\[[^;]+?=\s*\{(.*?)\};',raw,re.S)[1])]
    require(len(names)==74 and len(flags)==72 and names[0]=='Spacecraft' and names[50]=='CoordinateSystem','Core global flag/name table alignment changed')
    prior=repo/'build/nasa-coverage/coverage95-20261103-function-resource-census';matrix=load(prior/'factory-scope-matrix-v1.json')
    meta=load(repo/'build/gui-acceptance/function-eligibility-headless-v9/QtGui.NasaFunctionParameterRoles/native-parameter-roles.json');parameters={v['factory_type']:v for v in meta['rows']}
    require(len(parameters)==324 and meta['system_parameter_count']==321 and meta['user_parameters']==['Array','String','Variable'],'Native role partition differs')
    retained=[];by_type=collections.defaultdict(list);inputs={}
    for group in ['function-resource-matrix-review','function-resource-extended-review','function-reference-resource-review','function-builtin-review']:
        context=repo/'build/nasa-coverage'/group/'case-contexts.json';inputs[str(context)]=sha(context)
        for row in load(context):
            for key in ['receipt','snapshot_manifest','proof']:
                p=Path(row[key]);require(p.is_file() and not p.is_symlink() and sha(p)==row[key+'_sha256'],'Changed retained witness '+str(p));inputs[str(p)]=sha(p)
            receipt=load(row['receipt']);active=[v for v in receipt['results'] if v['name']==row['test']];require(len(active)==1 and active[0]['status']=='passed' and active[0]['returncode']==0,'Retained GUI case did not pass')
            junit=Path(row['case_directory'])/'ctest.xml';tests=list(ET.parse(junit).getroot().iter('testcase'));require(len(tests)==1 and tests[0].get('name')==row['test'] and tests[0].find('failure') is None and tests[0].find('error') is None and tests[0].find('skipped') is None,'Retained JUnit differs');inputs[str(junit)]=sha(junit)
            log=Path(active[0]['log']);inputs[str(log)]=sha(log);proof=load(row['proof']);native=[json.loads(v) for v in re.findall(r'NASA_CHECK (\{[^\n]+\})',log.read_text())]
            if row['case']!='Catalog':require(len(native)==1 and native[0]==proof and proof['outcome']=='passed','Retained native proof differs')
            assertion_bound=row['assertion_source_sha256'] in receipt['source_hashes'].values()
            item=dict(group=group,case=row['case'],test=row['test'],mode=row['mode'],proof=row['proof'],proof_sha256=row['proof_sha256'],receipt=row['receipt'],receipt_sha256=row['receipt_sha256'],assertion_source_sha256=row['assertion_source_sha256'],assertion_hash_in_original_receipt=assertion_bound,scope=proof.get('scope','Factory catalog only'),qualified=False)
            retained.append(item);kind=proof.get('type',proof.get('builtin_type'))
            if kind:by_type[kind].append(len(retained)-1)
    # These are constructor and ownership roles, not exclusions from the obligation.
    roles={109:'mission-command;separate-command-obligation',110:'propagation-implementation;PropSetup-owned-route',152:'coordinate-axes;CoordinateSystem-owned-route',153:'attitude-model;Spacecraft-owned-route',154:'expression-node;expression-and-direct-creation-eligibility-open'}
    special={101:'CreateSpacecraft',105:'CreatePlate',110:'CreatePropagator',111:'CreateODEModel',112:'CreatePhysicalModel',117:'CreateCelestialBody',118:'CreateCalculatedPoint',135:'CreatePropSetup',136:'CreateFunction',143:'CreateFieldOfView',150:'CreateHardware',151:'CreateCoordinateSystem',152:'CreateAxisSystem',158:'CreateEventLocator'}
    rows=[]
    for old in matrix['rows']:
        row=dict(old);category=row['category_id'];name=row['factory_type'];row['role']=roles.get(category,'resource-candidate;main/function-use-qualification-open');row['role_sources']=[general,creation,configuration,automatic,global_route,global_store]
        row['moderator_creation_entrypoint']=special.get(category,'CreateObject:factory-CreateObject-and-AddObjectToObjectMapInUse')
        row['nominal_category_base_global_default']=flags[category-101] if category-101<len(flags) else False
        row['nominal_global_flag_limit']='GmatBase constructor table for this category;derived classes,ownership,scope and actual instances still require evidence.'
        row['retained_witness_indices']=by_type.get(name,[]);row['main_use_eligibility']='not-qualified';row['explicit_Global_eligibility']='not-qualified';row['whole_function_scope_qualified']=False
        if category==122:
            p=parameters[name];row['native_parameter_metadata']=p;row['role']='owner-bound-system-parameter' if p['is_system_parameter'] else 'user-parameter-resource';row['role_sources'].append(dependent)
            row['parameter_scope_limit']='System parameters retain owner/dependency/evaluation and global-owner binding obligations;not 321 independent bare resources or exclusions.'
        if category in {110,152}:row['role_sources'].append(anchor(moderator,'Moderator::'+special[category]+'('))
        if category==153:row['role_sources'].append(anchor('src/base/spacecraft/Spacecraft.cpp','bool Spacecraft::IsOwnedObject('))
        if category==154:row['role_sources'].append(anchor('src/base/factory/MathFactory.cpp','MathFactory::CreateObject('))
        if category==166:row['core_table_category_label']=names[65];require(row['core_table_category_label']=='Interface','Core category label differs')
        rows.append(row)
    require(len(rows)==566 and len({v['factory_type'] for v in rows})==566 and set(parameters)=={v['factory_type'] for v in rows if v['category_id']==122},'Role matrix drops a factory/parameter')
    aliases=[dict(script_type='Propagator',factory_type='PropSetup',source=anchor('src/base/interpreter/ScriptInterpreter.cpp','if (type == "Propagator")'))]
    for spelling,target,file,needle in [('OpenGLPlot','OrbitView','src/base/factory/SubscriberFactory.cpp','else if (ofType == "OpenGLPlot")'),('Enhanced3DView','OrbitView','src/base/factory/SubscriberFactory.cpp','else if (ofType == "Enhanced3DView")'),('GroundTrackPlot','GroundTrack','src/base/factory/SubscriberFactory.cpp','else if (ofType == "GroundTrackPlot")'),('FuelTank','ChemicalTank','src/base/factory/HardwareFactory.cpp','if ((ofType == "FuelTank")'),('Thruster','ChemicalThruster','src/base/factory/HardwareFactory.cpp','if ((ofType == "Thruster")'),('BatchEstimatorInv','BatchEstimator','plugins/EstimationPlugin/src/base/factory/EstimatorFactory.cpp','if (ofType == "BatchEstimatorInv")')]:aliases.append(dict(factory_type=spelling,native_type=target,source=anchor(file,needle),qualified=False))
    for name,digest in inputs.items():require(sha(name)==digest,'Retained input changed during review')
    return dict(status='complete-factory-role-classification;main-use-and-global-eligibility-still-open',qualified_keys=[],factory_entries=566,categories=36,roles=dict(collections.Counter(v['role'] for v in rows)),rows=rows,source_anchors=anchors,script_and_factory_aliases=aliases,retained_contexts=retained,retained_context_count=len(retained),retained_resource_types=sorted(k for k in by_type if any(v['case']!='Catalog' and v['group']!='function-builtin-review' for v in [retained[i] for i in by_type[k]])),retained_artifact_sha256=inputs,unbound_assertion_contexts=[v['test']+':'+v['mode'] for v in retained if not v['assertion_hash_in_original_receipt']],limits=['No factory entry is dropped or assigned whole-row credit.','The complete role table does not establish named main-resource construction/use or every eligible Global operation.','Existing witnesses retain their exact scope,source version and all original assertions;this review checks custody and terminal native/JUnit correspondence,not every old numerical calculation.','Nominal automatic-global categories and owner-bound parameters retain their original exclusions/semantics;the original requirement scope is not narrowed.'],scientific_approval=False,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();result=review(Path(__file__).resolve().parents[3],a.stage.resolve());a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k in ['status','factory_entries','roles','retained_context_count','retained_resource_types','unbound_assertion_contexts']},indent=2))
