"""Verify bounded native endpoint-journal evidence; assigns zero NASA credit."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import tarfile
import sys
import NasaGuiCoverage as nasa
from RunHarnessAcceptance import passed_junit

READERS=['AuditAcceptedStepTransactionEvidence','BuildNasaCoverageExtension','NasaGuiCoverage','RunHarnessAcceptance','AcceptanceCatalog']
METHODS=['RungeKutta89','RungeKutta56','RungeKutta68','PrinceDormand45','PrinceDormand78','PrinceDormand853','BulirschStoer','AdamsBashforthMoulton']
def load(path):return json.loads(Path(path).read_text())
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def require(condition,message):
    if not condition:raise ValueError(message)
def circle(mu,time):
    radius=7000.;rate=math.sqrt(mu/radius**3);a=rate*time
    return [radius*math.cos(a),radius*math.sin(a),0.,-radius*rate*math.sin(a),radius*rate*math.cos(a),0.]
def check_case(result):
    require(result['status']=='passed' and result['returncode']==0,'Native case failed')
    root=Path(result['log']).parent
    require(passed_junit(root/'ctest.xml',result['name'],0),'Actual native JUnit is not passing')
    proof=load(root/'proof.json');require(nasa.proof_records(result)==[proof],'Native checkpoint differs from actual log')
    return root,proof

def audit(repo,manifest):
    m=load(manifest)
    require(m['qualified_nasa_keys']==[],'Native foundation cannot assign NASA credit')
    artifacts=m['artifact_sha256']
    def bound(path):
        p=Path(path).resolve();require(str(p) in artifacts and sha(p)==artifacts[str(p)],'Unsealed/changed consumed input: '+str(p));return p
    essential=[m['build_guard'],m['loaded_images'],m['reader_archive'],m['preimages'],m['original_assertion_receipt'],*m['matrix_receipts'].values(),*m['affected_receipts'].values()]
    for path in essential:bound(path)
    for path,h in artifacts.items():require(sha(path)==h,'Frozen artifact changed: '+path)
    guard=load(m['build_guard']);require(guard['status']=='passed' and guard['build_exit_codes']==[0,0],'No successful build')
    require(sha(repo/guard['source_archive'])==guard['source_archive_sha256'],'Compiled archive changed')
    bound(repo/guard['source_archive']);frozen={}
    with tarfile.open(repo/guard['source_archive']) as bundle:
        for name,h in guard['compiled_sources'].items():
            data=bundle.extractfile(name).read();frozen[name]=data;require(not data.startswith(b'\x7fELF') and hashlib.sha256(data).hexdigest()==h,'Frozen source input changed')
    for name in guard['successful_build_logs']:bound(repo/name)
    for name,h in guard['successful_build_logs'].items():require(sha(repo/name)==h and 'FAILED:' not in Path(repo/name).read_text(),'Build failed/changed')
    preimages=load(m['preimages'])
    for name in guard['producer_sources']:
        require(guard['source_sha256'][name]==guard['compiled_sources'][name],'Producer and compiled-source identities differ')
    partitions=[('src/base/propagator/RungeKutta.cpp','    ','stepTaken'),('src/base/propagator/RungeKuttaNystrom.cpp','   ','stepTaken'),('src/base/propagator/PredictorCorrector.cpp','            ','stepSize'),('plugins/ExtraPropagatorsPlugin/src/base/propagator/BulirschStoer.cpp','    ','stepTaken')]
    require({x['path'] for x in guard['calculation_scope_checks']}=={x[0] for x in partitions},'Source partition incomplete')
    for name,indent,duration in partitions:
        before=bound(Path(m['preimage_root'])/name);require(sha(before)==preimages[name],'Preimage changed');old=before.read_text();current=frozen[name].decode().replace('\r\n','\n')
        hook=indent+'if (AcceptedStepHistory::IsCapturing())\n'+indent+'   AcceptedStepHistory::Record(this, physicalModel,\n'+indent+'      physicalModel->GetTime() - '+duration+', physicalModel->GetTime(),\n'+indent+'      outState, dimension);\n'
        require(current.count(hook)==1,'Accepted endpoint hook omitted/duplicated');stripped=current.replace('#include "AcceptedStepHistory.hpp"\n','',1).replace(hook,'',1)
        if name.endswith('BulirschStoer.cpp'):
            policy='\n    // Fixed-interval callers set stepSize to their entire remaining interval.\n    // Bound the primitive trial before extrapolation, as the RK family does.\n    if (fabs(stepSize) > maximumStep)\n       stepSize = stepSize < 0.0 ? -maximumStep : maximumStep;\n'
            require(stripped.count(policy)==1,'Reviewed BS primitive bound changed');stripped=stripped.replace(policy,'',1)
        if name.endswith('PredictorCorrector.cpp'):
            def block(text):
                start=text.index('bool PredictorCorrector::BufferHistory()\n{');return text[start:text.index('\n}\n',start)+2]
            require('return Reset();' in block(stripped),'Reviewed ABM history policy changed');stripped=stripped.replace(block(stripped),block(old),1)
        require(stripped==old,'Original step calculation changed outside reviewed partition: '+name)
    assertion='src/qtgui/tests/NasaIntegratorControlsTests.cpp';original=load(m['original_assertion_receipt'])
    require(original['source_hashes'][assertion]==hashlib.sha256(frozen[assertion]).hexdigest(),'Original regression assertions changed')
    matrices={};max_error=[0.,0.];expected={method+direction for method in METHODS for direction in ['Forward','Backward']}
    for mode,path in m['matrix_receipts'].items():
        receipt=load(path);require(receipt['mode']==mode and receipt.get('end_reason')=='completed-selection','Wrong/nonterminal matrix mode')
        active=[r for r in receipt['results'] if r['status']!='not-run'];require(len(active)==1,'Unexpected matrix test replay')
        root=Path(active[0]['log']).parent
        for file in [active[0]['log'],root/'ctest.xml',root/'proof.json',root/'accepted-matrix.json']:bound(file)
        root,proof=check_case(active[0]);require(proof['checkpoint']=='accepted-step-transaction-matrix' and proof['all_eight_methods_exercised'] and proof['method_filter']=='' and proof['warmup_mode']=='stop' and proof['independent_state_values']==960 and proof['rejected_operations']==9 and proof['thread_isolated'] and proof['nested_discard_and_commit'] and proof['no_publication_or_live_spacecraft_mutation'],'Incomplete native contract')
        exe='build/linux-gui/src/qtgui/GmatQtAcceptedStepTransactionTests';require(str(repo/exe) in active[0]['registered_command'] and receipt['binary_hashes'][str(repo/exe)]==guard['image_sha256'][exe],'Actual matrix executable differs')
        for name in guard['producer_sources']:
            if name in receipt['source_hashes']:require(receipt['source_hashes'][name]==guard['source_sha256'][name],'Executed producer source differs')
        require(receipt['source_hashes']['src/qtgui/tests/AcceptedStepTransactionTests.cpp']==guard['compiled_sources']['src/qtgui/tests/AcceptedStepTransactionTests.cpp'],'Executed matrix assertions differ')
        rows=load(root/'accepted-matrix.json');require(rows==proof['native_cases'] and len(rows)==16 and {r['case'] for r in rows}==expected,'Native case matrix omitted')
        for row in rows:
            name=row['case'];direction=-1 if name.endswith('Backward') else 1;method=name.removesuffix('Backward').removesuffix('Forward');dt=1. if method=='AdamsBashforthMoulton' else 5.
            require(row['primitive_step_seconds']==dt and len(row['primitive_steps'])==10 and row['raw_publications']==0,'Accepted primitive count/bound/publication differs')
            source_methods=set()
            for i,step in enumerate(row['primitive_steps']):
                end=direction*dt*(i+2);start=direction*dt*(i+1);require(abs(step['start']-start)<1e-8 and abs(step['end']-end)<1e-8 and len(step['state'])==6,'Wrong primitive time/grid/state width')
                source_methods.add(step['method'])
                for j,(got,want) in enumerate(zip(step['state'],circle(398600.4415,end))):
                    require(math.isfinite(got) and abs(got-want)<(3e-5 if j<3 else 4e-8),'Native endpoint differs from independent circle');max_error[j//3]=max(max_error[j//3],abs(got-want))
            if method=='AdamsBashforthMoulton':
                require(source_methods=={'RungeKutta89','AdamsBashforthMoulton'},'ABM actual corrector/startup paths omitted')
                bound(root/(name+'-startup-history.json'));journal=load(root/(name+'-startup-history.json'));reverts=[e for e in journal if e['event']=='after-revert'];require(reverts and all(e['startup_count']==0 and not e['startup_complete'] for e in reverts),'Actual physical rollback retained invalid history')
        matrices[mode]=proof
        if mode=='visible':
            from BuildNasaCoverageExtension import validate_visible
            validate_visible(receipt,{active[0]['name']:dict(log=active[0]['log'],log_sha256=sha(active[0]['log']),junit=str(root/'ctest.xml'),junit_sha256=sha(root/'ctest.xml'))})
            require(receipt['display_scale']=='auto','Visible scaling disabled')
    require(matrices['headless']==matrices['visible'],'Headless/visible native outputs differ')
    images=load(m['loaded_images']);require(len(images['processes'])==1,'Actual matrix process observation missing')
    pid,row=next(iter(images['processes'].items()));require((Path(m['matrix_receipts']['visible']).parent/'QtGui.AcceptedStepTransactions'/('trace-'+pid+'.jsonl')).is_file(),'Loaded maps do not belong to actual visible test')
    for p in ['application/bin/libGmatBase.so.R2026a','application/plugins/libExtraPropagators.so.R2026a','build/linux-gui/src/qtgui/GmatQtAcceptedStepTransactionTests']:require(row['images'][p]==guard['image_sha256'][p],'Actual loaded image differs')
    for mode,path in m['affected_receipts'].items():
        receipt=load(path);require(receipt['mode']==mode and receipt.get('end_reason')=='completed-selection','Affected regression nonterminal')
        results={r['name']:r for r in receipt['results'] if r['status']!='not-run'}
        require(set(results)=={'QtGui.NativeNasaIntegratorControls'+method for method in ['AdamsBashforthMoulton','BulirschStoer']},'Unexpected existing case replay')
        for name,result in results.items():
            root=Path(result['log']).parent
            for file in [result['log'],root/'ctest.xml',root/'proof.json',root/'values.csv']:bound(file)
            require(receipt['source_hashes'][assertion]==original['source_hashes'][assertion],'Actual regression assertion source differs')
            root,proof=check_case(result);require(proof['analytic_components']==6 and proof['mission_executions']==1 and proof['report_values']==8,'Original regression numerical assertions missing')
            values=[float(v) for v in (root/'values.csv').read_text().strip().split(',')];require(len(values)==8 and abs(values[0]-600)<1e-6 and values[-1]==41,'Original report clock/sentinel wrong')
            for j,(got,want) in enumerate(zip(values[1:7],circle(398600.4418,600))):require(math.isfinite(got) and abs(got-want)<(2e-5 if j<3 else 2e-8),'Original independent regression state differs')
            exe='build/linux-gui/src/qtgui/GmatQtNasaIntegratorControlsTests';require(str(repo/exe) in result['registered_command'] and receipt['binary_hashes'][str(repo/exe)]==guard['image_sha256'][exe],'Actual regression binary differs')
        if mode=='visible':
            from BuildNasaCoverageExtension import validate_visible
            validate_visible(receipt,{name:dict(log=result['log'],log_sha256=sha(result['log']),junit=str(Path(result['log']).parent/'ctest.xml'),junit_sha256=sha(Path(result['log']).parent/'ctest.xml')) for name,result in results.items()})
    with tarfile.open(m['reader_archive']) as archive:
        require(set(archive.getnames())=={'src/qtgui/tests/'+name+'.py' for name in READERS},'Reader closure changed')
        for name in READERS:
            source='src/qtgui/tests/'+name+'.py';module=sys.modules[__name__] if name=='AuditAcceptedStepTransactionEvidence' else sys.modules.get(name)
            require(module is not None and Path(module.__file__).resolve()==repo/source and sha(repo/source)==hashlib.sha256(archive.extractfile(source).read()).hexdigest(),'Executing evidence reader changed: '+name)
    return dict(status='passed',evidence_manifest_sha256=sha(manifest),auditor_sha256=sha(Path(__file__)),native_cases_per_mode=16,primitive_endpoints_per_mode=160,independent_component_comparisons_per_mode=960,maximum_position_component_error_km=max_error[0],maximum_velocity_component_error_km_s=max_error[1],rejected_transaction_operations=9,affected_existing_cases_per_mode=2,qualified_nasa_keys=[],current_requirements_coverage='1009/1158 (87.1%)',four_plotting_rows_still_unqualified=['FRR!E329','FRR!E330','FRR!E633','FRR!E634'],technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3]);p.add_argument('--manifest',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();result=audit(a.repo.resolve(),a.manifest);a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
