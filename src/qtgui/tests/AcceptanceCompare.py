"""Reusable truth-file comparisons and standalone acceptance HTML reports.

These tools compare supplied truth; they do not generate orbital truth, change
existing GUI comparison semantics, or override any fixture's assertions.
"""
import argparse
import csv
import datetime as dt
from decimal import Decimal, localcontext
import difflib
import hashlib
import html
import importlib.util
import json
import os
from pathlib import Path
import re
import tempfile

NUMBER = re.compile(r'^[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eEdD][+-]?\d+)?$')
NONFINITE = re.compile(r'^[+-]?(?:nan|inf(?:inity)?)$', re.I)
PV_FIELDS = ('X','Y','Z','VX','VY','VZ')
PV_IDENTITIES = ('FRAME','TIME_SYSTEM','TIME_UNIT','POSITION_UNIT','VELOCITY_UNIT')
OEM_IDENTITIES = ('OBJECT_NAME','OBJECT_ID','CENTER_NAME','REF_FRAME','TIME_SYSTEM','START_TIME','STOP_TIME')


def need(value, message):
    if not value:
        raise ValueError(message)


def decimal(value):
    need(not isinstance(value, bool), 'Boolean is not numeric truth/tolerance')
    text = str(value)
    need(bool(NUMBER.fullmatch(text)), 'Invalid or nonfinite number: '+text)
    number = Decimal(text.replace('d','e').replace('D','E'))
    need(number.is_finite() and abs(number.adjusted()) <= 10000 and len(number.as_tuple().digits) <= 10000,
         'Unsupported numeric magnitude/precision')
    return number


def text_number(value):
    return '0' if value == 0 else str(value)


def json_data(value):
    if isinstance(value,Decimal):
        return text_number(value)
    if isinstance(value,(tuple,list)):
        return [json_data(item) for item in value]
    if isinstance(value,dict):
        return {key:json_data(item) for key,item in value.items()}
    return value


def validate_policy(policy):
    need(isinstance(policy,dict) and not (set(policy)-{'absolute','relative','fields'}), 'Unknown comparison tolerance policy')
    for key in ('absolute','relative'):
        if key in policy:
            need(decimal(policy[key]) >= 0, 'Negative '+key+' tolerance')
    if 'fields' in policy:
        need(isinstance(policy['fields'],dict), 'Field tolerance policies must be an object')
        for name, child in policy['fields'].items():
            need(isinstance(name,str) and name and isinstance(child,dict) and 'fields' not in child,
                 'Invalid field tolerance policy')
            validate_policy(child)


def numeric_cell(location, field, actual, truth, policy, unit=None):
    a,t = decimal(actual),decimal(truth)
    selected = {**{k:v for k,v in policy.items() if k!='fields'}, **policy.get('fields',{}).get(field,{})}
    absolute,relative = decimal(selected.get('absolute',0)),decimal(selected.get('relative',0))
    # Avoid double subtraction/rounding at distant origins. Also preserve the
    # exact difference of widely separated exponents within the stated limits.
    minimum_exponent=min(a.as_tuple().exponent,t.as_tuple().exponent,absolute.as_tuple().exponent,
                         relative.as_tuple().exponent+t.as_tuple().exponent)
    maximum_adjusted=max(a.adjusted(),t.adjusted(),absolute.adjusted(),relative.adjusted()+t.adjusted()+1)
    precision=maximum_adjusted-minimum_exponent+5
    with localcontext() as context:
        context.prec = max(64,precision)
        delta = a-t
        allowed = absolute+relative*abs(t)
        exceeded = abs(delta)>allowed
    return dict(location=location,field=field,actual=text_number(a),truth=text_number(t),
                delta=text_number(delta),allowed=text_number(allowed),unit=unit,exceeded=exceeded)


def mismatch(report, location, field, actual, truth):
    report['differences'].append(dict(location=location,field=field,actual=json_data(actual),truth=json_data(truth)))


def compare_numeric(a, t, report, policy):
    left,right = a.decode('utf-8-sig').splitlines(),t.decode('utf-8-sig').splitlines()
    for line in range(max(len(left),len(right))):
        if line>=len(left) or line>=len(right):
            mismatch(report,f'line {line+1}','record',left[line] if line<len(left) else None,
                     right[line] if line<len(right) else None)
            continue
        def tokens(value):
            return [x.strip() for x in next(csv.reader([value]))] if ',' in value else value.split()
        actual,truth = tokens(left[line]),tokens(right[line])
        for col in range(max(len(actual),len(truth))):
            location=f'line {line+1}, column {col+1}'
            if col>=len(actual) or col>=len(truth):
                mismatch(report,location,'token',actual[col] if col<len(actual) else None,
                         truth[col] if col<len(truth) else None)
                continue
            av,tv = actual[col],truth[col]
            need(not NONFINITE.fullmatch(av) and not NONFINITE.fullmatch(tv), 'Nonfinite numeric field at '+location)
            if NUMBER.fullmatch(av) and NUMBER.fullmatch(tv):
                report['comparisons'].append(numeric_cell(location,str(col+1),av,tv,policy))
            elif av!=tv:
                mismatch(report,location,'token',av,tv)
    need(bool(report['comparisons']), 'No numeric fields were compared')


def parse_pv(payload):
    metadata,rows = {},[]
    for raw in payload.decode('utf-8-sig').splitlines():
        line=raw.strip()
        if not line:
            continue
        if line.startswith('#'):
            if '=' in line:
                key,value=line[1:].split('=',1);key,value=key.strip(),value.strip()
                need(key not in metadata and value, 'Duplicate/empty position-velocity metadata')
                metadata[key]=value
            continue
        tokens=line.split()
        if tokens==['epoch','x','y','z','vx','vy','vz']:
            need(not rows,'Misplaced position-velocity header')
            continue
        need(len(tokens)==7, 'Position/velocity records need epoch and six components')
        values=[decimal(x) for x in tokens]
        need(not rows or values[0]>rows[-1][0], 'Repeated/out-of-order position-velocity epoch')
        rows.append(values)
    need(all(metadata.get(k) for k in PV_IDENTITIES) and rows, 'Position-velocity identity/units or records missing')
    return metadata,rows


def epoch(value, system):
    match=re.fullmatch(r'(\d{4})-(\d{2})-(\d{2})T(\d{2}):(\d{2}):(\d{2}(?:\.\d+)?)(Z?)',value)
    need(match is not None,'Unsupported OEM calendar epoch: '+value)
    year,month,day,hour,minute=map(int,match.groups()[:5]);second=decimal(match[6])
    dt.date(year,month,day)
    need(hour<24 and minute<60 and second<61, 'Invalid OEM clock')
    need(not match[7] or system=='UTC','Z suffix requires UTC')
    need(second<60 or (system=='UTC' and hour==23 and minute==59),'Invalid leap-second clock label')
    # A clock-label identity, not a time-scale conversion. In particular,
    # 23:59:60 is distinct from the following midnight, without a leap-table guess.
    return year,month,day,hour,minute,second


def parse_oem(payload):
    header,segments = {},[]
    state='header';segment=None;cov=None
    def finish_cov():
        nonlocal cov
        if cov is not None:
            need(len(cov['rows'])==6,'Incomplete OEM covariance triangle')
            values=[v for row in cov.pop('rows') for v in row]
            cov['values']=values
            need(not segment['covariance'] or cov['epoch']>segment['covariance'][-1]['epoch'],
                 'Repeated/out-of-order covariance epoch')
            segment['covariance'].append(cov);cov=None
    for raw in payload.decode('utf-8-sig').splitlines():
        line=raw.strip()
        if not line or line=='COMMENT' or line.startswith('COMMENT '):
            continue
        if line=='META_START':
            need(state in ('header','data','after-covariance'), 'Nested/misplaced OEM metadata')
            if segment is not None:
                need(segment['states'],'Empty OEM state segment')
            segment=dict(metadata={},states=[],covariance=[]);segments.append(segment);state='metadata'
        elif line=='META_STOP':
            need(state=='metadata' and all(segment['metadata'].get(k) for k in OEM_IDENTITIES),
                 'Missing OEM segment identity/epoch metadata')
            state='data'
        elif line=='COVARIANCE_START':
            need(state=='data' and segment['states'] and header.get('CCSDS_OEM_VERS')=='2.0',
                 'Misplaced or version-incompatible covariance block')
            state='covariance'
        elif line=='COVARIANCE_STOP':
            need(state=='covariance' and (cov is not None or segment['covariance']), 'Empty/misplaced covariance block')
            finish_cov();state='after-covariance'
        elif state in ('header','metadata'):
            need('=' in line,'Invalid OEM header/metadata line')
            key,value=map(str.strip,line.split('=',1));target=header if state=='header' else segment['metadata']
            need(key not in target and value,'Duplicate/empty OEM metadata')
            target[key]=value
        elif state=='data':
            tokens=line.split();need(len(tokens) in (7,10),'OEM state needs six or nine components')
            need(len(tokens)==7 or header.get('CCSDS_OEM_VERS')=='2.0','Acceleration requires OEM2.0')
            stamp=epoch(tokens[0],segment['metadata']['TIME_SYSTEM']);values=[decimal(x) for x in tokens[1:]]
            need(not segment['states'] or stamp>segment['states'][-1]['epoch'],'Repeated/out-of-order OEM state epoch')
            segment['states'].append(dict(epoch=stamp,label=tokens[0],values=values))
        elif state=='covariance':
            if line.startswith('EPOCH') and '=' in line:
                finish_cov();label=line.split('=',1)[1].strip()
                cov=dict(epoch=epoch(label,segment['metadata']['TIME_SYSTEM']),label=label,
                         frame=segment['metadata']['REF_FRAME'],rows=[],frame_explicit=False)
            elif line.startswith('COV_REF_FRAME') and '=' in line:
                need(cov is not None and not cov['rows'] and not cov['frame_explicit'],'Misplaced covariance frame')
                cov['frame']=line.split('=',1)[1].strip();need(cov['frame'],'Empty covariance frame');cov['frame_explicit']=True
            else:
                need(cov is not None and len(cov['rows'])<6,'Covariance data without an epoch')
                values=[decimal(x) for x in line.split()]
                need(len(values)==len(cov['rows'])+1,'Invalid covariance triangle row width')
                cov['rows'].append(values)
        else:
            raise ValueError('Unexpected data after OEM covariance block')
    need(state in ('data','after-covariance') and segments and header.get('CCSDS_OEM_VERS') in ('1.0','2.0'),
         'Missing/incomplete/unsupported OEM KVN1.0/2.0 structure')
    for seg in segments:
        need(seg['states'],'Empty OEM state segment')
        system=seg['metadata']['TIME_SYSTEM'];start=epoch(seg['metadata']['START_TIME'],system);stop=epoch(seg['metadata']['STOP_TIME'],system)
        need(start<=seg['states'][0]['epoch']<=seg['states'][-1]['epoch']<=stop,'OEM states outside declared time span')
        need(all(start<=c['epoch']<=stop for c in seg['covariance']), 'OEM covariance outside declared time span')
    return header,segments


def compare_metadata(a,t,report,location,ignored=()):
    same=True
    for field in sorted(set(a)|set(t)):
        if field not in ignored and a.get(field)!=t.get(field):
            mismatch(report,location,field,a.get(field),t.get(field));same=False
    return same


def compare_states(a,t,report,policy,location,fields,units,epoch_key=lambda r:r[0],values_key=lambda r:r[1:]):
    if len(a)!=len(t):
        mismatch(report,location,'record-count',len(a),len(t))
    for index,(actual,truth) in enumerate(zip(a,t)):
        at=location+f', record {index+1}'
        if epoch_key(actual)!=epoch_key(truth):
            mismatch(report,at,'epoch',str(epoch_key(actual)),str(epoch_key(truth)));continue
        av,tv=values_key(actual),values_key(truth)
        if len(av)!=len(tv):
            mismatch(report,at,'component-count',len(av),len(tv));continue
        for name,unit,a_value,t_value in zip(fields,units,av,tv):
            report['comparisons'].append(numeric_cell(at,name,a_value,t_value,policy,unit))


def compare_pv(a,t,report,policy):
    am,ar=parse_pv(a);tm,tr=parse_pv(t)
    if compare_metadata(am,tm,report,'position-velocity identity/units'):
        compare_states(ar,tr,report,policy,'position-velocity',PV_FIELDS,
                       [am['POSITION_UNIT']]*3+[am['VELOCITY_UNIT']]*3)


def compare_oem(a,t,report,policy):
    ah,aseg=parse_oem(a);th,tseg=parse_oem(t)
    ignored=() if policy.get('strict_header') else ('CREATION_DATE','ORIGINATOR')
    report['ignored_header_fields']=list(ignored)
    if not compare_metadata(ah,th,report,'OEM header',ignored):
        return
    if len(aseg)!=len(tseg):
        mismatch(report,'OEM','segment-count',len(aseg),len(tseg))
    for index,(actual,truth) in enumerate(zip(aseg,tseg)):
        location=f'OEM segment {index+1}';am,tm=actual['metadata'],truth['metadata']
        # Fractional zero formatting and an optional UTC Z do not change clocks.
        ac,tc=dict(am),dict(tm)
        for field in ('START_TIME','STOP_TIME','USEABLE_START_TIME','USEABLE_STOP_TIME'):
            if field in ac:ac[field]=epoch(ac[field],am['TIME_SYSTEM'])
            if field in tc:tc[field]=epoch(tc[field],tm['TIME_SYSTEM'])
        if not compare_metadata(ac,tc,report,location+' metadata'):
            continue
        compare_states(actual['states'],truth['states'],report,policy,location+' states',
                       PV_FIELDS+('AX','AY','AZ'),['km']*3+['km/s']*3+['km/s^2']*3,
                       lambda r:r['epoch'],lambda r:r['values'])
        av,tv=actual['covariance'],truth['covariance']
        if len(av)!=len(tv):mismatch(report,location,'covariance-count',len(av),len(tv))
        for i,(left,right) in enumerate(zip(av,tv)):
            at=location+f' covariance {i+1}'
            if left['epoch']!=right['epoch'] or left['frame']!=right['frame']:
                mismatch(report,at,'covariance epoch/frame',str((left['epoch'],left['frame'])),str((right['epoch'],right['frame'])));continue
            for (row,col),a_value,t_value in zip(((r,c) for r in range(6) for c in range(r+1)),left['values'],right['values']):
                unit=('km' if row<3 else 'km/s')+'*'+('km' if col<3 else 'km/s')
                report['comparisons'].append(numeric_cell(at,f'C{row+1}{col+1}',a_value,t_value,policy,unit))


class Registry:
    def __init__(self):
        self.comparators={'numeric':compare_numeric,'position-velocity':compare_pv,'oem':compare_oem}

    def register(self,name,callback):
        need(isinstance(name,str) and name and name not in self.comparators and name!='file'
             and callable(callback),'Invalid/duplicate custom comparator')
        self.comparators[name]=callback

    def compare(self,actual,truth,kind,policy=None,strict_header=False):
        paths={'actual':Path(actual),'truth':Path(truth)}
        result=dict(schema_version=1,kind=kind,status='error',inputs={},comparisons=[],differences=[],errors=[])
        try:
            policy={} if policy is None else policy;validate_policy(policy);result['policy']=json_data(policy)
            data={}
            for name,path in paths.items():
                data[name]=path.read_bytes();result['inputs'][name]=dict(path=str(path.resolve()),sha256=hashlib.sha256(data[name]).hexdigest(),bytes=len(data[name]))
            if kind=='file':
                a,t=data['actual'],data['truth']
                if a!=t:
                    preview=list(difflib.unified_diff(t.decode('utf-8',errors='replace').splitlines(),a.decode('utf-8',errors='replace').splitlines(),fromfile='truth',tofile='actual'))
                    mismatch(result,'file','byte-content',len(a),len(t));result['diff_preview']=preview[:200];result['diff_preview_truncated']=len(preview)>200
            else:
                need(kind in self.comparators,'Unregistered comparator: '+kind)
                passed_policy={**policy,'strict_header':True} if kind=='oem' and strict_header else policy
                self.comparators[kind](data['actual'],data['truth'],result,passed_policy)
                need(result['comparisons'] or result['differences'],'Comparator produced no comparisons or differences')
            need(all(isinstance(c,dict) and isinstance(c.get('exceeded'),bool)
                     and all(k in c for k in ('location','field','actual','truth','delta','allowed'))
                     for c in result['comparisons']),'Custom comparator returned invalid numeric records')
            for cell in result['comparisons']:
                for field in ('actual','truth','delta','allowed'):
                    decimal(cell[field])
                need(decimal(cell['allowed'])>=0 and cell['exceeded']==(decimal(cell['delta']).copy_abs()>decimal(cell['allowed'])),
                     'Custom comparator returned inconsistent tolerance status')
            need(all(isinstance(d,dict) and all(k in d for k in ('location','field','actual','truth'))
                     for d in result['differences']), 'Invalid structural difference records')
            need(all(isinstance(e,str) for e in result['errors']), 'Invalid comparator errors')
            result['compared_fields']=len(result['comparisons'])
            result['exceeded_fields']=sum(c['exceeded'] for c in result['comparisons'])
            result['status']='error' if result['errors'] else ('failed' if result['differences'] or result['exceeded_fields'] else 'passed')
            # Invalid plugin payloads must not create an unrenderable passing
            # report. An explicit custom comparator remains user-supplied logic.
            json.dumps(result,allow_nan=False)
        except Exception as error:
            # A malformed plugin must still yield a serializable error report.
            result=dict(schema_version=1,kind=kind,status='error',inputs={
                name:dict(path=str(path.resolve())) for name,path in paths.items()},
                comparisons=[],differences=[],errors=[str(error)],compared_fields=0,exceeded_fields=0)
        return result


STYLE = 'body{font:16px system-ui,sans-serif;margin:2rem;max-width:100rem;color:#182536;background:#f7f9fc}h1{margin-bottom:.4rem}.status{font-weight:700;padding:.6rem;border:1px solid #667;border-radius:.3rem}.passed{background:#e0f1e6}.failed,.error{background:#ffe3df}table{border-collapse:collapse;background:white;width:100%;margin:1rem 0}th,td{border:1px solid #bdc7d2;padding:.5rem;text-align:left;overflow-wrap:anywhere}th{background:#e8edf5}code,pre{font:14px monospace}pre{white-space:pre-wrap}.scroll{overflow-x:auto}caption{text-align:left;font-weight:bold;margin:.5rem 0}'


def escape(value):
    return html.escape(str(value) if value is not None else '',quote=True)


def table(title,headers,rows):
    return '<div class="scroll"><table><caption>'+escape(title)+'</caption><thead><tr>'+''.join('<th scope="col">'+escape(h)+'</th>' for h in headers)+'</tr></thead><tbody>'+''.join('<tr>'+''.join('<td>'+escape(v)+'</td>' for v in row)+'</tr>' for row in rows)+'</tbody></table></div>'


def html_document(title,body):
    return '<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><meta http-equiv="Content-Security-Policy" content="default-src \'none\'; style-src \'unsafe-inline\'"><title>'+escape(title)+'</title><style>'+STYLE+'</style></head><body><main><h1>'+escape(title)+'</h1>'+body+'</main></body></html>'


def render_comparison(report):
    need(report.get('status') in ('passed','failed','error'),'Invalid comparison report status')
    body='<p class="status '+report['status']+'">'+escape(report['status'].upper())+' · '+escape(report['kind'])+' · '+escape(report.get('compared_fields',0))+' numeric fields checked</p>'
    body+=table('Input identities',['Role','Path','SHA256'],[(k,v['path'],v.get('sha256','unavailable')) for k,v in report['inputs'].items()])
    body+='<p>Tolerance policy: <code>'+escape(json.dumps(report.get('policy',{})))+'</code></p>'
    if report.get('ignored_header_fields'):body+='<p>Ignored volatile header fields: '+escape(', '.join(report['ignored_header_fields']))+'</p>'
    body+=table('Numeric differences: actual minus truth',['Location','Field','Unit','Actual','Truth','Difference','Allowed','Exceeds'],[(c['location'],c['field'],c.get('unit'),c['actual'],c['truth'],c['delta'],c['allowed'],c['exceeded']) for c in report['comparisons']])
    body+=table('Identity/structure differences',['Location','Field','Actual','Truth'],[(d['location'],d['field'],d['actual'],d['truth']) for d in report['differences']])
    if report['errors']:body+=table('Input/comparator errors',['Error'],[(e,) for e in report['errors']])
    if report.get('diff_preview'):body+='<pre>'+escape('\n'.join(report['diff_preview']))+'</pre>'
    if report.get('diff_preview_truncated'):body+='<p>Diff preview truncated to200lines; full byte contents were compared.</p>'
    return html_document('Acceptance file comparison',body)


def render_catalogue(catalog):
    need(catalog.get('schema_version')==1 and isinstance(catalog.get('tests'),list)
         and isinstance(catalog.get('scopes'),list),'Invalid catalogue export input')
    summary=catalog['summary'];body='<p class="status '+('failed' if summary['tests_failed'] else 'passed')+'">Recorded failures: '+escape(summary['tests_failed'])+' · passed: '+escape(summary['tests_passed'])+' · run: '+escape(summary['tests_run'])+' · reused: '+escape(summary['reused_passes'])+'</p>'
    body+='<p>'+escape(catalog.get('scope','Supplied recorded evidence; no new execution.'))+'</p>'
    body+=table('Recorded tests',['Test','Status','Execution','Type','Platform','Requirements'],[(t['name'],t['status'],t['execution'],t['test_type'],t['platform'],', '.join(t['requirements'])) for t in catalog['tests']])
    body+=table('Requirement and historical scopes',['Source key','Kind','Requirement','Evidence state','Tests'],[(r['key'],r['kind'],r['name'],r['state'],', '.join(r['tests'])) for r in catalog['scopes']])
    return html_document('Recorded acceptance catalogue',body)


def validate_output(path,protected):
    path=Path(path)
    for source in protected:
        source=Path(source)
        need(path.resolve()!=source.resolve() and not (path.exists() and source.exists() and os.path.samefile(path,source)),
             'Report output would overwrite an input: '+str(path))


def write_output(path,content,protected):
    path=Path(path)
    validate_output(path,protected)
    path.parent.mkdir(parents=True,exist_ok=True)
    with tempfile.NamedTemporaryFile(mode='w',encoding='utf-8',dir=path.parent,delete=False) as stream:
        temporary=Path(stream.name)
        try:
            stream.write(content);stream.close();os.replace(temporary,path)
        finally:
            temporary.unlink(missing_ok=True)


def main(argv=None):
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--actual',type=Path);parser.add_argument('--truth',type=Path)
    parser.add_argument('--kind',default='numeric');parser.add_argument('--absolute',default='0');parser.add_argument('--relative',default='0')
    parser.add_argument('--policy',type=Path);parser.add_argument('--strict-header',action='store_true')
    parser.add_argument('--custom-module',type=Path,help='Explicit trusted Python module defining register_comparators(registry)')
    parser.add_argument('--catalogue',type=Path);parser.add_argument('--json',type=Path);parser.add_argument('--html',type=Path)
    args=parser.parse_args(argv);need(args.json or args.html,'Select JSON and/or HTML output')
    protected=[p for p in (args.actual,args.truth,args.catalogue,args.policy,args.custom_module) if p]
    if args.catalogue:
        need(not args.actual and not args.truth and not args.custom_module,'Catalogue export cannot also compare files')
        report=json.loads(args.catalogue.read_text());page=render_catalogue(report);returncode=0
    else:
        need(args.actual and args.truth,'Comparison needs actual and truth files')
        policy=json.loads(args.policy.read_text()) if args.policy else {'absolute':args.absolute,'relative':args.relative}
        registry=Registry()
        if args.custom_module:
            spec=importlib.util.spec_from_file_location('acceptance_custom_comparator',args.custom_module)
            need(spec and spec.loader,'Cannot load custom comparator module')
            module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);module.register_comparators(registry)
        report=registry.compare(args.actual,args.truth,args.kind,policy,args.strict_header)
        if args.custom_module:report['custom_module']=dict(path=str(args.custom_module.resolve()),sha256=hashlib.sha256(args.custom_module.read_bytes()).hexdigest())
        page=render_comparison(report);returncode={'passed':0,'failed':1,'error':2}[report['status']]
    if args.json and args.html:need(args.json.resolve()!=args.html.resolve(),'JSON and HTML destinations must differ')
    for path in (args.json,args.html):
        if path:validate_output(path,protected)
    if args.json:write_output(args.json,json.dumps(report,indent=2,ensure_ascii=False,allow_nan=False)+'\n',protected)
    if args.html:write_output(args.html,page,protected)
    return returncode


if __name__=='__main__':
    try:
        raise SystemExit(main())
    except (ValueError,OSError,TypeError,AttributeError) as error:
        print('Acceptance comparison error: '+str(error),file=__import__('sys').stderr)
        raise SystemExit(2)
