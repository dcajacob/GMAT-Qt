"""Recheck all retained batch format/SPK/filter controls at their original versions.

No missions, historical audit scripts, receipt rewrites or current-source rebinding.
This verifies constituent proofs; whole-requirement admission is a separate decision.
"""
import argparse,contextlib,copy,hashlib,json,math,re,struct,sys,types
from pathlib import Path
import AcceptanceSnapshot as snapshot
import AuditAcceptanceRecovery as recovery
from BuildNasaCoverageExtension import require

GROUPS=['batch-format-review','spk-shared-sample-review','filter-format-review']
KEY='FRR!E406';FINGERPRINT='0c63ed325e134f5265b5c425cbaafaa577c25540e799f7b3fbb069834ce37276'
ORIGINAL_PAYLOAD=recovery.payload


class Inputs:
    def __init__(self):self.bindings={};self.cache={}
    def payload(self,path):
        path=Path(path);physical=path if path.is_file() else Path(str(path)+'.gz')
        require(physical.is_file(),'Missing retained artifact '+str(path))
        key=str(path)
        if key not in self.cache:
            raw=ORIGINAL_PAYLOAD(path);real=physical.resolve();require(real.is_file() and not real.is_symlink(),'Nonregular physical artifact')
            self.cache[key]=raw;self.bindings[key]=dict(payload_sha256=hashlib.sha256(raw).hexdigest(),physical=str(real),physical_sha256=hashlib.sha256(real.read_bytes()).hexdigest())
        return self.cache[key]
    def json(self,path,expected=None):
        raw=self.payload(path)
        if expected:require(hashlib.sha256(raw).hexdigest()==expected,'Retained identity mismatch '+str(path))
        return json.loads(raw)
    def checked(self,path,expected):
        data=self.payload(path);require(hashlib.sha256(data).hexdigest()==expected,'Retained bytes changed '+str(path));return data


@contextlib.contextmanager
def frozen_modules(inputs,root,manifest,receipt,names):
    old={n:sys.modules.get(n) for n in names};modules={}
    try:
        for name in names:
            relative='src/qtgui/tests/'+name+'.py';digest=manifest['artifact_hashes'].get(relative);require(digest and receipt['source_hashes'].get(relative)==digest,'Unrecorded original oracle/dependency '+relative)
            data=inputs.checked(root/relative,digest);module=types.ModuleType(name);module.__file__=str(root/relative);sys.modules[name]=module;exec(compile(data,module.__file__,'exec'),module.__dict__);modules[name]=module
        yield modules
    finally:
        for name,value in old.items():
            if value is None:sys.modules.pop(name,None)
            else:sys.modules[name]=value


def census(contexts,group):
    expected={'batch-format-review':{'Code500Little','Code500Big','StkKilometers','StkMeters'},'spk-shared-sample-review':{'Positive','Negative'},'filter-format-review':{'FilterOem','FilterSpk','FilterCode500','FilterStk','SmootherOem','SmootherSpk','SmootherCode500','SmootherStk'}}[group]
    require(len(contexts)==2*len(expected) and {(v['mode'],v['case']) for v in contexts}=={(mode,key) for mode in ['unattended','visible'] for key in expected},'Incomplete or duplicate original conjunction '+group)


def audit(repo):
    inputs=Inputs();recovery.payload=inputs.payload;records=[];negatives=[];metrics=dict(old_native_contexts=0,visible_contexts=0,batch_format_rows=0,SPK_raw_knots=0,SPK_reader_queries=0,unsupported_filter_format_rejections=0,supported_filter_control_rows=0);last_claim=None;last_context=None
    try:
        for group in GROUPS:
            stage=repo/'build/nasa-coverage'/group;summary=inputs.json(stage/'validation-summary.json');contexts=inputs.json(stage/'case-contexts.json');census(contexts,group)
            if summary.get('mapping_manifest'):inputs.json(summary['mapping_manifest'],summary['mapping_manifest_sha256'])
            if summary.get('qualification_receipt'):inputs.json(summary['qualification_receipt'],summary['qualification_receipt_sha256'])
            for context in contexts:
                receipt=inputs.json(context['receipt'],context['receipt_sha256']);manifest=inputs.json(context['snapshot_manifest'],context['snapshot_manifest_sha256']);identity=dict(manifest);identity.pop('snapshot_id');require(snapshot.fingerprint(identity)==manifest['snapshot_id']==receipt['snapshot_id'] and receipt['snapshot_unchanged'] and receipt.get('finished_utc'),'Original snapshot/receipt identity differs')
                root=Path(manifest['root']);mapping=inputs.json(root/'src/qtgui/tests/NasaGuiTraceability.json');require(hashlib.sha256(inputs.payload(root/'src/qtgui/tests/NasaGuiTraceability.json')).hexdigest()==manifest['artifact_hashes']['src/qtgui/tests/NasaGuiTraceability.json'],'Frozen mapping changed');rows=[r for r in mapping['reviews'] if r['key']==KEY];require(len(rows)==1 and rows[0]['fingerprint']==FINGERPRINT,'Original exact requirement differs')
                claims=[v for v in rows[0]['partial_evidence'] if v['test']==context['test']];require(len(claims)==1,'Missing original full partial clause');claim=claims[0]
                if 'versioned_partial_claim' in context:require(context['versioned_partial_claim']==claim,'Later context changes an original claim')
                results=[r for r in receipt['results'] if r['name']==context['test']];require(len(results)==1,'Missing/duplicate original case');result=results[0];detail=recovery.verify_claim(root,manifest,receipt,result,claim)
                proof=inputs.json(context['proof'],context['proof_sha256']);require(proof['outcome']=='passed' and result['returncode']==0,'Original proof not passed');directory=Path(context['directory']);metrics['old_native_contexts']+=1
                if context['mode']=='visible':
                    profile=result.get('execution_profile',receipt.get('execution_profile',{}));require(profile['visible'] and profile['start_minimized'] and not profile['fixed_size'] and result['trace']['native_exposure_events']>0 and not result['trace']['screenshots'],'Original visible policy/exposure differs');metrics['visible_contexts']+=1
                if group=='batch-format-review':
                    fixture=json.loads(inputs.payload(root/'src/qtgui/tests/NasaBatchEphemerisCases.json'));specs=json.loads(inputs.payload(root/'src/qtgui/tests/NasaBatchFormatCases.json'));spec=next(c for c in specs['cases'] if c['key']==context['case']);raw=inputs.checked(context['raw_file'],context['raw_file_sha256']);decoded=inputs.json(context['decoded'],context['decoded_sha256']);observations=inputs.payload(directory/'observations.gmd').decode()
                    with frozen_modules(inputs,root,manifest,receipt,['BatchFormatOracle']) as modules:analyzed=modules['BatchFormatOracle'].analyze(raw,observations,fixture,spec)
                    require(analyzed==decoded and analyzed['points']==proof['points']==context['points']==1435 and analyzed['observations']==358,'Original every-state batch format result differs');metrics['batch_format_rows']+=1435
                    clone=inputs.json(directory/'clone-status.json');require(clone['inspected_before_run_complete'] and clone['type']=='BatchEstimator' and clone['status'].startswith('Meet ') and clone['status']==proof['convergence_status'],'Native batch solution absent')
                elif group=='spk-shared-sample-review':
                    fixture=json.loads(inputs.payload(root/'src/qtgui/tests/NasaBatchSpkCases.json'));decoded=inputs.json(context['decoded'],context['decoded_sha256']);raw=inputs.payload(directory/'mission.bsp');native=inputs.payload(directory/'native.csv').decode();observations=inputs.payload(directory/'observations.gmd').decode();lsk=inputs.payload(directory/'leapseconds.tls').decode();table_bytes=inputs.checked(root/'application/data/icrf/ICRF_Table.txt',decoded['frame_table_sha256']);table=[[float(x) for x in line.split(',')] for line in table_bytes.decode().splitlines()]
                    with frozen_modules(inputs,root,manifest,receipt,['EclipseGeometryOracle','SpkHermiteOracle','BatchSpkOracle']) as modules:
                        oracle=modules['BatchSpkOracle'];analyzed=oracle.validate(fixture,context['case'],raw,native,observations,lsk,table,decoded);encoded=oracle.validate_encoded_curve(raw,decoded)
                        require(analyzed==context['analysis']==inputs.json(directory/'independent.json') and encoded==context['encoded_curve'],'Original SPK knot/midpoint result differs')
                        damaged=bytearray(raw);segments=modules['SpkHermiteOracle'].read_segments(raw);require(segments,'No SPK segments');bad=copy.deepcopy(decoded);bad['reader_samples'][900]['state'][0]+=1
                        try:oracle.validate(fixture,context['case'],raw,native,observations,lsk,table,bad)
                        except (ValueError,AssertionError):negatives.append(context['mode']+'-'+context['case']+'-wrong-SPK-state')
                        else:raise ValueError('Accepted corrupt retained SPK state')
                    require(proof['raw_knots']==1436 and proof['reader_queries']==2869,'SPK proof census differs');metrics['SPK_raw_knots']+=1436;metrics['SPK_reader_queries']+=2869
                else:
                    fixture=json.loads(inputs.payload(root/'src/qtgui/tests/NasaFilterFormatCases.json'));spec=next(c for c in fixture['cases'] if c['key']==context['case']);actual=inputs.json(directory/'actual-outcome.json');messages=inputs.payload(directory/'messages.txt').decode();require(actual['allowed']==context['allowed']==spec['allowed'] and actual['run_mode']==10 and proof['run_mode']==10,'Original normal-startup format context differs')
                    if not spec['allowed']:
                        require(actual['failed'] and not actual['completed'] and actual['expected_diagnostic'] and not actual['continuation_exists'] and spec['diagnostic'] in messages,'Unsupported format rejection lost');require(not (directory/'continuation.csv').exists(),'Forbidden continuation exists');metrics['unsupported_filter_format_rejections']+=1
                    else:
                        require(actual['completed'] and not actual['failed'] and actual['continuation_exists'],'Supported filter control failed');data=inputs.payload(directory/'mission.e').decode();parts=data.split('EphemerisTimePosVel\n',1)[1].split('END Ephemeris',1)[0];states=[[float(x) for x in line.split()] for line in parts.splitlines() if line.strip()];require(len(states)==proof['points']==context['points']==(44 if spec['family']=='Smoother' else 22) and all(len(r)==7 and all(math.isfinite(x) for x in r) for r in states),'Old structural STK controls differ');metrics['supported_filter_control_rows']+=len(states)
                    inputs.checked(context['covariance'],context['covariance_sha256'])
                records.append(dict(group=group,mode=context['mode'],case=context['case'],receipt=context['receipt'],receipt_sha256=context['receipt_sha256'],snapshot_manifest=context['snapshot_manifest'],snapshot_manifest_sha256=context['snapshot_manifest_sha256'],original_claim=claim,proof_sha256=context['proof_sha256'],identity=detail));last_claim=claim;last_context=(root,manifest,receipt,result)
        require(metrics==dict(old_native_contexts=28,visible_contexts=14,batch_format_rows=11480,SPK_raw_knots=5744,SPK_reader_queries=11476,unsupported_filter_format_rejections=12,supported_filter_control_rows=132),'Constituent metrics differ')
        for group in GROUPS:
            data=[dict(mode=r['mode'],case=r['case']) for r in records if r['group']==group];data.pop()
            try:census(data,group)
            except ValueError:negatives.append(group+'-missing-original-case')
            else:raise ValueError('Accepted incomplete '+group)
        for label,change in [('wrong-original-source',lambda c:c.update(source_sha256='0'*64)),('lost-original-anchor',lambda c:c.update(anchor='NOT_AN_ORIGINAL_ASSERTION_3f8430'))]:
            bad=copy.deepcopy(last_claim);change(bad)
            try:recovery.verify_claim(*last_context,bad)
            except ValueError:negatives.append(label)
            else:raise ValueError('Accepted corrupt original claim '+label)
        return dict(status='passed-28-original-source-bound-constituents;whole-E406-decision-required',original_key=KEY,fingerprint=FINGERPRINT,qualified_keys=[],metrics=metrics,records=records,negative_controls=negatives,input_bindings=inputs.bindings,native_missions_replayed=0,historical_receipts_modified=False,current_reference_rebindings=0,scientific_approval=False,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)
    finally:recovery.payload=ORIGINAL_PAYLOAD


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();result=audit(Path(__file__).resolve().parents[3]);a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ['records','input_bindings']}))
