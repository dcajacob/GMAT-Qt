"""Audit four exact MSIS-86 obligations against frozen native and NASA evidence.

This is source-versioned capability admission, not scientific/core approval.
"""
import argparse
import copy
import hashlib
import json
import math
from pathlib import Path
import re
import tarfile
import tempfile
import types
import xml.etree.ElementTree as ET
import NasaGuiCoverage as nasa
from BuildNasaCoverageExtension import sha, load, require, validate_visible

KEYS=['FRR!E116','FRR!E117','FRR!E118','FRR!E119']
CASES=['Baseline','SolarFlux','AverageFlux','MagneticIndex','LowAltitude','HighAltitude','South','NearPole','Equator','Date','Longitude','Oblate']
REGRESSIONS=['QtGui.Atmosphere','QtGui.NativeNasaWeatherChoiceConstantHistoryCssiPredict']
KIND='source-versioned-exact-MSIS86-atmosphere'


def check_proof(proof,spec,expected,directory=None):
    require(proof['outcome']=='passed' and proof['case']==spec['key'] and proof['choice_popups']==2,'Missing MSIS86 GUI conjunction')
    require([p['phase'] for p in proof['records']]==[0,1],'Missing repeated native run')
    for row,truth in zip(proof['records'],expected):
        require(row['runtime_model']=='MSISE86','Wrong scientific model')
        require(row['raw']==truth['raw'] and all(math.isfinite(v) for v in row['raw']),'Report/proof differs')
        si=row['raw'][7]/1e9;native=row['native_density_kg_m3'];target=truth['expected_density_kg_m3']
        require(si>0 and abs(si-target)<=target*5e-6,'Independent density differs')
        require(abs(native-si)<=si*1e-12,'Native/report unit conversion differs')
        if directory:
            raw=[[float(v) for v in line.split(',')] for line in (directory/f"{row['phase']}-values.csv").read_text().splitlines() if line.strip()]
            require(raw==[row['raw'],[42.]],'Raw output or continuation differs')
    if directory:
        require(load(directory/'case.json')==spec,'Changed literal weather/geometry fixture')
        script=(directory/'final.script').read_text()
        for key,value in [('AtmosphereModel','MSISE86'),('HistoricWeatherSource','ConstantFluxAndGeoMag'),('PredictedWeatherSource','ConstantFluxAndGeoMag')]:
            require(f'Forces.Drag.{key} = {value};' in script,'Wrong selected atmosphere/weather source')
        for key,field in [('F107','f107'),('F107A','f107a'),('MagneticIndex','kp')]:
            values=re.findall(r'Forces\.Drag\.'+key+r'\s*=\s*([^;]+);',script)
            require(len(values)==1 and float(values[0])==spec[field],'Wrong constant weather input')


def check_domain(proof,directory=None):
    require(proof['outcome']=='passed' and proof['earth_only_factory'] is True,'Body-qualified model absent')
    require(proof['phases']==[dict(height_km=89,expected_failure=True,completed=False),dict(height_km=90.001,expected_failure=False,completed=True)],'Domain rejection/recovery differs')
    if directory:require('at or above 90 km altitude' in (directory/'below-domain-messages.txt').read_text(),'Precise domain diagnostic missing')


def qualify(repo,path,inventory):
    q=load(path);require(q['contract_kind']==KIND and q['qualified_keys']==KEYS,'Changed exact MSIS86 scope')
    known={r['key']:r for r in inventory['requirements']}
    require(q['authority_sha256']==inventory['sources'] and q['original_requirements']==[dict(key=k,fingerprint=known[k]['fingerprint'],source_text=known[k]['text']) for k in KEYS],'Original NASA authority changed')
    require(all(q.get(k) is False for k in ['technical_core_approval','scientific_approval','full_current_build_qualified','full_NASA_qualified']),'Qualification exceeds scope')
    for p,h in q['artifact_sha256'].items():require(not Path(p).is_symlink() and sha(p)==h,'Changed regular artifact: '+p)
    def bound(p):
        require(str(p) in q['artifact_sha256'],'Unbound consumed artifact: '+str(p));return Path(p)
    groups={};proofs={};domain=None
    for name,g in q['groups'].items():
        before,after=(load(bound(g[k])) for k in ['inputs_before','inputs_after'])
        require(before==after and load(bound(g['launch']))['inputs_unchanged'] and load(bound(g['launch']))['returncode']==0,'Native inputs changed or visible launch failed')
        with tarfile.open(bound(g['source_archive'])) as tar:frozen={m.name:tar.extractfile(m).read() for m in tar.getmembers()}
        require({p:hashlib.sha256(b).hexdigest() for p,b in frozen.items()}==before['sources'],'Frozen source version differs')
        groups[name]=(g,before,frozen)
    primary=groups['primary'][2];boundary=groups['domain'][2]
    plugin_files=[p for p in primary if p.startswith('plugins/Msise86Plugin/')]
    require(plugin_files and all(primary[p]==boundary[p] for p in plugin_files),'Production plugin changed between batches')
    matrix=json.loads(primary['src/qtgui/tests/NasaMsise86Cases.json']);require([c['key'] for c in matrix['cases']]==CASES,'Required model/input matrix missing')
    reference=Path(q['reference_dir']);downloads=load(bound(reference/'source-downloads.json'))
    for filename,row in downloads.items():
        p=bound(reference/filename);require(sha(p)==row['sha256'] and p.stat().st_size==row['bytes'],'NASA download identity differs')
        require('/21b2dfadc8a9c126d55f0e77d28aeb8f85b36b81/MSIS/MSIS86/' in row['url'],'NASA source version changed')
    for filename in ['msis86.for','msis86.dat']:require(primary['plugins/Msise86Plugin/src/base/model/'+filename]==bound(reference/filename).read_bytes(),'Vendored scientific source differs')
    require(primary['src/qtgui/tests/fixtures/msise86/m86tes.dat']==bound(reference/'m86tes.dat').read_bytes(),'Published golden values differ')
    prepare=types.ModuleType('frozen_prepare');exec(compile(primary['plugins/Msise86Plugin/src/base/model/PrepareMsis86.py'],'frozen_prepare','exec'),prepare.__dict__)
    require(prepare.generate(reference/'msis86.for',reference/'msis86.dat')==bound(q['generated_fortran']).read_text(),'Generated scientific source differs')
    symbols={line.split()[-1] for line in bound(q['symbols']).read_text().splitlines() if line.split()}
    require('gmat_m86_gts5_' in symbols and 'gmat_m86_tselec_' in symbols and not symbols.intersection({'gts5_','tselec_','tretrv_','denss_','globe5_','glob5l_','dnet_','ccor_','gts3c_','csw_','lpoly_','parm5_','lower5_','uinr_'}),'Unisolated Fortran symbols')
    reference_tests=ET.parse(bound(q['reference_junit'])).getroot().findall('testcase')
    require(len(reference_tests)==1 and reference_tests[0].attrib['name']=='QtAcceptance.NasaMsise86Reference' and reference_tests[0].attrib.get('status')=='run' and reference_tests[0].find('failure') is None and reference_tests[0].find('skipped') is None and '100 published values; 300 comparisons; 30 interleaved MSISE90 checks' in reference_tests[0].findtext('system-out'),'Published-model/isolation test missing')
    require(load(bound(q['original_reference_comparison']))['differences']==[],'Unchanged NASA reference differs')
    # Raw prototype model vectors are reusable only for unchanged model/generator bytes.
    prototype=load(bound(q['prototype_identities']))
    for p in ['plugins/Msise86Plugin/src/base/model/'+v for v in ['msis86.for','msis86.dat','PrepareMsis86.py']]+['src/qtgui/tests/NasaMsise86ModelTests.py']:
        require(hashlib.sha256(primary[p]).hexdigest()==prototype['sources'][p],'Prototype scientific implementation changed')
    model=types.ModuleType('frozen_model_test');exec(compile(primary['src/qtgui/tests/NasaMsise86ModelTests.py'],'frozen_model_test','exec'),model.__dict__)
    golden=model.expected(reference/'m86tes.dat');records=load(bound(q['prototype_model']))['records']
    require([(r['phase'],r['case']) for r in records]==[(phase,i) for phase,order in [('published',range(10)),('reverse',reversed(range(10))),('interleaved',[4,0,9,1,7,2,8,3,6,5])] for i in order],'Raw published sequence differs')
    for record in records:
        require(len(record['values'])==10,'Published output absent')
        for label,value in zip(model.LABELS,record['values']):
            require(float(format(value,'.2f' if label in ['TINF','TG'] else '.3e'))==golden[label][record['case']],'Published value differs')
    kp=load(bound(reference/'Kp2ap.json'));require(sha(bound(reference/'Kp2ap.pdf'))==kp['sha256'] and kp['used_pairs']=={'1o':4,'4o':27},'Independent magnetic conversion identity differs')
    oracle=types.ModuleType('frozen_msise86_oracle');exec(compile(primary['src/qtgui/tests/Msise86Oracle.py'],'frozen_msise86_oracle','exec'),oracle.__dict__)
    # Reconstruct only small frozen model/oracle inputs; no current source/image dependency.
    with tempfile.TemporaryDirectory(prefix='msis86-frozen-audit-') as tmp:
        root=Path(tmp)
        for p in ['plugins/Msise86Plugin/src/base/model/msis86.for','plugins/Msise86Plugin/src/base/model/msis86.dat','src/qtgui/tests/fixtures/msise86/query.for','src/qtgui/tests/NasaMsise86Cases.json']:
            dest=root/p;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(primary[p])
        truth=oracle.check([Path(v).parent for v in groups['primary'][0]['runs'].values()],root)
    require(truth['native_vectors']==48,'Missing independent density vectors')
    for group,(g,before,frozen) in groups.items():
        mapped=load(bound(g['loaded_images']))['processes']
        selected=['QtGui.NasaMsise86Domain'] if group=='domain' else ['QtGui.NasaMsise86'+k for k in CASES]+REGRESSIONS
        for mode,receipt_path in g['runs'].items():
            receipt=load(bound(receipt_path));require(receipt['mode']==mode and receipt.get('finished_utc') and receipt['end_reason']=='completed-selection','Nonterminal native selection')
            require(all(before['sources'].get(p)==h for p,h in receipt['source_hashes'].items()),'Executed Qt source differs')
            active={r['name']:r for r in receipt['results'] if r['status']!='not-run'};require(set(active)==set(selected),'Changed native selection')
            evidence={}
            for name,row in active.items():
                log=bound(row['log']);directory=log.parent;junit=bound(directory/'ctest.xml')
                require(row['status']=='passed' and row['returncode']==0,'Failed native case')
                tests=ET.parse(junit).getroot().findall('testcase');require(len(tests)==1 and tests[0].attrib['name']==name and tests[0].attrib.get('status')=='run' and tests[0].find('failure') is None and tests[0].find('skipped') is None,'Passing JUnit absent')
                for artifact in directory.rglob('*'):
                    if artifact.is_file() and not artifact.is_symlink():bound(artifact)
                exes=[p for p in row['registered_command'] if p in receipt['binary_hashes']];require(len(exes)==1 and before['images'].get(exes[0])==receipt['binary_hashes'][exes[0]],'Executed binary differs')
                evidence[name]=dict(log=str(log),log_sha256=sha(log),junit=str(junit),junit_sha256=sha(junit))
                if name not in REGRESSIONS:
                    proof=load(bound(directory/'proof.json'));matches=re.findall(r'NASA_CHECK (\{[^\n]+\})',log.read_text());require(len(matches)==1 and json.loads(matches[0])==proof,'Proof differs from stdout')
                    if group=='domain':check_domain(proof,directory);domain=proof
                    else:
                        spec=next(c for c in matrix['cases'] if name=='QtGui.NasaMsise86'+c['key'])
                        expected=[r for r in truth['records'] if r['run']==str(Path(receipt_path).parent) and r['case']==spec['key']]
                        require(len(expected)==2,'Wrong independent case membership');check_proof(proof,spec,expected,directory);proofs[spec['key']]=(proof,spec,expected)
                if mode=='visible':
                    traces=list(directory.glob('trace-*.jsonl'));require(traces,'Native visible traces missing')
                    for trace in traces:
                        process=mapped.get(trace.stem.removeprefix('trace-'));require(process and process['executable']==exes[0],'Visible process mapping absent')
                        for p,h in before['images'].items():
                            if p==exes[0] or Path(p).name.startswith(('libGmat','libMsise86')):require(process['images'].get(p)==h,'Mapped engine/plugin/test differs')
            if mode=='visible':validate_visible(receipt,evidence)
    # Literal perturbations must be distinguishable, not just accepted fields.
    baseline=proofs['Baseline'][2][0]['expected_density_kg_m3']
    require(all(abs(proofs[k][2][0]['expected_density_kg_m3']/baseline-1)>1e-3 for k in ['SolarFlux','AverageFlux','MagneticIndex']),'Weather controls do not distinguish outputs')
    negatives=[]
    for label,mutate in [('wrong-model',lambda p:p['records'][0].update(runtime_model='MSISE90')),('wrong-density',lambda p:p['records'][0]['raw'].__setitem__(7,1)),('wrong-native-units',lambda p:p['records'][0].update(native_density_kg_m3=1)),('missing-repeat',lambda p:p['records'].pop()),('missing-popup',lambda p:p.update(choice_popups=0)),('wrong-case',lambda p:p.update(case='Missing'))]:
        proof,spec,expected=proofs['Baseline'];changed=copy.deepcopy(proof);mutate(changed)
        try:check_proof(changed,spec,expected)
        except (AssertionError,ValueError):negatives.append(label)
        else:raise ValueError('Auditor accepted '+label)
    for label,mutate in [('accepted-below-domain',lambda p:p['phases'][0].update(completed=True)),('missing-recovery',lambda p:p['phases'].pop())]:
        changed=copy.deepcopy(domain);mutate(changed)
        try:check_domain(changed)
        except (AssertionError,ValueError):negatives.append(label)
        else:raise ValueError('Auditor accepted '+label)
    q['audit_summary']=dict(qualified_keys=KEYS,native_gui_passes=30,new_gui_cases=13,affected_regressions=2,native_density_vectors=48,max_relative_error=truth['max_relative_error'],published_values=100,published_comparisons=300,msise90_isolation_checks=30,negative_controls=negatives,production_changes=q['production_changes'])
    return q,KEYS


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--qualification',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();repo=Path(__file__).resolve().parents[3]
    q,keys=qualify(repo,a.qualification,nasa.inventory(repo));a.output.write_text(json.dumps(dict(status='passed',qualified_keys=keys,summary=q['audit_summary'],scientific_approval=False,technical_core_approval=False,full_NASA_qualified=False),indent=2)+'\n');print(json.dumps(q['audit_summary']))
