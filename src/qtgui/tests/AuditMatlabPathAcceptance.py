"""Audit the complete MATLAB file-location contract and eleven-type parent.

No MATLAB function/engine support is inferred from a native filename lookup.
Every original location child remains mandatory at its recorded source version.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import tarfile

from AuditCapiAcceptance import load,require,sha
import NasaGuiCoverage as nasa
from BuildNasaCoverage95 import baseline
from BuildNasaCoverageExtension import validate_visible
from RunHarnessAcceptance import passed_junit

KEYS=['All Others!E26','All Others!E31']
NAMES=['QtGui.NasaMatlabFunctionPaths','QtGui.NasaMatlabPathBrowseImport']
REGRESSIONS=['QtGui.Paths','QtGui.PythonPaths','QtGui.NasaFunctionPathSearch']
SOURCE='src/qtgui/tests/NasaMatlabPathTests.cpp'


def read_run(repo,spec,inventory,artifacts):
    before,after=load(spec['before']),load(spec['after']);require(before==after,'MATLAB path source/runtime drift')
    reuse=load(spec['reuse']);receipt=load(reuse['receipt']);require(sha(reuse['receipt'])==reuse['receipt_sha256'] and receipt.get('finished_utc') and receipt.get('end_reason')=='completed-selection','MATLAB path receipt changed/nonterminal')
    sources={};hashes={}
    for field,path in [('reused_source_sha256',reuse['reference']),('changed_source_sha256',reuse['delta'])]:
        require(sha(path)==reuse['reference_sha256' if field.startswith('reused') else 'delta_sha256'],'MATLAB path source archive changed')
        with tarfile.open(path) as bundle:
            for name,digest in reuse[field].items():
                require(not Path(name).is_absolute() and '..' not in Path(name).parts and name not in hashes,'Unsafe/duplicate source')
                data=bundle.extractfile(name).read();require(hashlib.sha256(data).hexdigest()==digest and not data.startswith(b'\x7fELF'),'Changed/non-source archive member');sources[name]=data;hashes[name]=digest
                require(before['sources'].get(name,digest)==digest,'Recorded source disagrees with wider snapshot')
    other=reuse['recorded_authority_ELF_or_existing_physical_inputs_not_copied'];require(not set(other)&set(hashes) and {**hashes,**other}==receipt['source_hashes'],'Incomplete source partition')
    require({p:other[p] for p in inventory['sources']}==inventory['sources'],'Changed authority')
    for name,digest in other.items():
        if name not in inventory['sources']:require(before['binaries'].get(str(repo/name))==digest,'Unbound noncopied binary source')
    results={r['name']:r for r in receipt['results'] if r['status']!='not-run'}
    require(set(results)==set(NAMES+REGRESSIONS) and all(r['status']=='passed' and r['returncode']==0 for r in results.values()),'New cases or affected regressions missing/failed')
    for name,result in results.items():
        binaries=[p for p in result['registered_command'] if p in receipt['binary_hashes']]
        require(binaries and all(before['binaries'].get(p)==receipt['binary_hashes'][p] for p in binaries),'Actual primary/helper executable not bound before execution')
        work=Path(result['log']).parent;require(all(str(work/f) in artifacts for f in ['test.log','ctest.xml']),'Actual log/JUnit omitted from seal')
        require(passed_junit(work/'ctest.xml',name,0),'Exact passing registered JUnit missing')
    if receipt['mode']=='visible':
        # PythonPaths is an explicitly headless support regression in the
        # existing registry. Do not convert its receipt or claim window exposure.
        support=results['QtGui.PythonPaths'];require(support['requested_visibility']=='headless','Unexpected Python support delivery route')
        visible={name:r for name,r in results.items() if name!='QtGui.PythonPaths'}
        view={**receipt,'results':list(visible.values())}
        validate_visible(view,{name:dict(log=r['log'],log_sha256=sha(r['log']),junit=str(Path(r['log']).parent/'ctest.xml'),junit_sha256=sha(Path(r['log']).parent/'ctest.xml')) for name,r in visible.items()})
    else:require(receipt['mode']=='headless','Wrong execution mode')
    return receipt,results,sources


def check_case(result,mode,sources,artifacts):
    work=Path(result['log']).parent
    require(all(str(work/f) in artifacts for f in ['proof.json','baseline.script','first.m','second.m','exported-startup.txt','test.log','ctest.xml']),'Missing required MATLAB path raw/source artifact')
    proof=load(work/'proof.json');require([p for p in nasa.proof_records(result) if p.get('checkpoint')=='matlab-function-paths']==[proof],'Actual logged checkpoint differs')
    require(proof['outcome']=='passed' and proof['mode']==mode and proof['matlab_function_execution_claimed'] is False and proof['assertions']>=(85 if mode=='Operations' else 65),'Incomplete MATLAB path operation contract')
    require((work/'first.m').read_bytes()==b'function y = AcceptancePriority(x)\ny = x * 2;\nend\n' and (work/'second.m').read_bytes()==b'function y = AcceptancePriority(x)\ny = x * 3;\nend\n','Prescribed independent file contents changed')
    if mode=='Operations':
        require(len(proof['observations'])==1 and proof['observations'][0]['phase']=='second-first','Native operation observation omitted');second=Path(proof['observations'][0]['lookup']);root=second.parent;first=root/'first MATLAB α';empty=root/'empty MATLAB γ'
        require(second.name=='second MATLAB β' and proof['fresh']==dict(lookup=str(first),paths=[str(empty),str(first)]),'Independent native first-match/fallback/fresh-startup lookup differs')
        require(str(work/'fresh-process.log') in artifacts,'Actual fresh native process log omitted')
        records=[json.loads(line[len('MATLAB_LOOKUP '):]) for line in (work/'fresh-process.log').read_text().splitlines() if line.startswith('MATLAB_LOOKUP ')];require(records==[proof['fresh']],'Fresh helper proof differs from actual log')
        expected=[str(empty),str(first)]
    else:
        second=Path(proof['native_lookup']);root=second.parent;first=root/'first MATLAB α'
        require(second.name=='second MATLAB β' and proof['native_paths']==[str(second),str(first)],'Independent Browse/import native lookup/order differs');expected=[str(second),str(first)]
    exported=(work/'exported-startup.txt').read_text();actual=[value.rstrip('/') for value in re.findall(r'^MATLAB_FUNCTION_PATH\s*=\s*(.*?)\s*$',exported,re.M)];require(actual==expected,'Exported native MATLAB path selection/order differs')
    code=sources[SOURCE].decode()
    for token in ['FileManager::Instance()->GetMatlabFunctionPath','Actual MATLAB function path controls missing','Pending MATLAB list changed the native paths','Invalid MATLAB Apply changed native paths','Missing MATLAB function unexpectedly resolved','Fresh-process MATLAB lookup failed','Exported MATLAB paths/order did not survive fresh startup','MATLAB paths changed unrelated native mission/source/history','Actual MATLAB path file/directory chooser missing','Reading pending MATLAB startup changed native paths','Imported MATLAB path/native prescribed file differs']:
        require(token in code,'Original path assertion/native consumer omitted: '+token)
    require('matlab_function_execution_claimed' in code and not any(token in code for token in ['engOpen(','engEvalString(','MatlabInterface::']),'File-location contract substituted/faked a MATLAB engine')


def qualify(repo,path,inventory):
    q=load(path);require(q['contract_kind']=='source-versioned-MATLAB-file-location-conjunction' and q['qualified_keys']==KEYS,'Partial/extra file-location credit')
    require(all(q.get(k) is False for k in ('technical_core_approval','full_current_build_qualified','full_NASA_qualified')),'File-location scope overclaimed')
    for name,digest in q['artifact_sha256'].items():require(sha(name)==digest,'Changed MATLAB path artifact: '+name)
    require(q['authority_sha256']==inventory['sources'],'Authority differs');review=load(q['review']);known={r['key']:r for r in inventory['requirements']}
    require([r['key'] for r in review['reviews']]==KEYS,'Missing original row review')
    for r in review['reviews']:require(r['fingerprint']==known[r['key']]['fingerprint'] and r['source_text']==known[r['key']]['text'] and r['rationale'],'Original source row changed')
    children={r['key'] for r in inventory['requirements'] if r['declared_id'].startswith('FRAC-2.6.') and r['key']!='All Others!E26'}
    require(children=={f'All Others!E{n}' for n in range(27,38)} and set(review['required_location_children'])==children,'Original eleven-type parent changed')
    _,original,keys=baseline(repo,Path(q['baseline_seal']));require(original['sources']==inventory['sources'] and children-{'All Others!E31'}<=keys['requirements'],'Missing inherited location child')
    require(set(q['runs'])=={'headless','visible'},'Missing mode')
    for mode,spec in q['runs'].items():
        receipt,results,sources=read_run(repo,spec,inventory,q['artifact_sha256']);require(receipt['mode']==mode,'Substituted cohort mode')
        for name,case_mode in zip(NAMES,['Operations','BrowseImport']):check_case(results[name],case_mode,sources,q['artifact_sha256'])
    require(review['matlab_execution_and_optimizer_claimed'] is False,'Native path lookup promoted to MATLAB engine support')
    return q,KEYS


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3]);p.add_argument('--qualification',type=Path,required=True);a=p.parse_args();_,keys=qualify(a.repo.resolve(),a.qualification,nasa.inventory(a.repo.resolve()));print(json.dumps(dict(status='passed',qualified_keys=keys,new_GUI_or_engine_executions=0)))
