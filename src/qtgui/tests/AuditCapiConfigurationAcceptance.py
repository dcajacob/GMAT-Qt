"""Admit E250's complete spacecraft CONFIGURATION contract, not all physics.

Reuse the exact38 underlying clause proofs and all-frame definitions, the real
C loading/lifetime receipts, and the new mixed-configuration C-client witness.
E13 arbitrary pairwise conversions and E249/E251/E252 remain outside this claim.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import sys
import tarfile
import types
import xml.etree.ElementTree as ET

import AuditCapiAcceptance as capi
import AuditCapiSpacecraftEpochAcceptance as epochs
import NasaGuiCoverage as nasa

KEY='All Others!E250'
CLAUSES=['FRR!E'+str(i)for i in range(5,44)if i!=13]
MODULES=['AuditCapiConfigurationAcceptance','AuditCapiAcceptance',
         'AuditCapiSpacecraftEpochAcceptance','NasaGuiCoverage']
sha,load,require=capi.sha,capi.load,capi.require


def function(code,marker):
    match=re.search(r'(?m)^[^/\n]*'+re.escape(marker),code)
    require(match is not None,'Missing reviewed C++ route '+marker)
    start=match.start();opening=code.index('{',match.end());depth=0
    lexer=re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[{}]')
    for token in lexer.finditer(code,opening):
        if token[0]=='{':depth+=1
        elif token[0]=='}':
            depth-=1
            if depth==0:return code[start:token.end()]
    raise ValueError('Unbounded C++ source function')


def records(result):
    return nasa.proof_records(result)


def junit(path,name):
    cases=list(ET.parse(path).getroot().iter('testcase'))
    require(len(cases)==1 and cases[0].get('name')==name and
            not any(cases[0].find(k)is not None for k in ['failure','error','skipped']),
            'Missing exact passing configuration JUnit')


def qualify(repo,path,inventory):
    q=load(path)
    require(q['contract_kind']=='source-versioned-C-API-spacecraft-configuration'and
            q['qualified_keys']==[KEY],'Wrong C spacecraft configuration contract')
    require(all(q.get(k)is False for k in ['technical_core_approval','full_current_build_qualified','full_NASA_qualified']),
            'C configuration qualification overclaims scope')
    for p,h in q['artifact_sha256'].items():require(sha(p)==h,'Changed configuration artifact: '+p)
    required={q[k]for k in ['admission_source_archive','prior_coverage','prior_final_seal','clause_map',
              'catalogue','catalogue_receipt','recovery_campaign','build_guard','route_review']}
    require(required<=set(q['artifact_sha256']),'Essential configuration provenance omitted')
    require(q['authority_sha256']==inventory['sources'],'Original NASA authority changed')
    known={row['key']:row for row in inventory['requirements']};review=q['original_requirement']
    require(review['key']==KEY and review['source_text']==known[KEY]['text']and
            review['fingerprint']==known[KEY]['fingerprint'],'Original E250 obligation changed')
    require(q['excluded_behavioral_row']==dict(key='FRR!E13',source_text=known['FRR!E13']['text'],
            fingerprint=known['FRR!E13']['fingerprint']), 'Arbitrary conversion gap concealed')
    # Bind the executing readers, not only a declared unused source archive.
    with tarfile.open(q['admission_source_archive'])as archive:
        require(set(archive.getnames())==set(q['admission_source_sha256']),'Admission source partition differs')
        for name,h in q['admission_source_sha256'].items():
            require(not Path(name).is_absolute()and'..'not in Path(name).parts and
                    hashlib.sha256(archive.extractfile(name).read()).hexdigest()==h,'Changed frozen admission source')
    for name in MODULES:
        p='src/qtgui/tests/'+name+'.py';module=sys.modules[__name__]if name=='AuditCapiConfigurationAcceptance'else sys.modules[name]
        require(Path(module.__file__).resolve()==repo/p and sha(repo/p)==q['admission_source_sha256'][p],
                'Executing C configuration reader differs from reviewed source')
    inherited=load(q['prior_coverage']);seal=load(q['prior_final_seal'])
    require(sha(q['prior_coverage'])==seal['coverage_snapshot_sha256'] and
            seal['original_requirements_verified']==1006 and seal['total_original_requirements']==1158,
            'Prior source-versioned union changed')
    remaining={row['key']for row in inherited['remaining_original_rows']['requirements']}
    require(set(CLAUSES).isdisjoint(remaining)and'FRR!E13'in remaining and KEY in remaining,
            'Configuration inherited clauses or open conversion distinction changed')
    mapping=load(q['clause_map']);require([row['key']for row in mapping]==CLAUSES,'Incomplete38-clause mapping')
    catalogue=load(q['catalogue']);receipt=load(q['catalogue_receipt'])
    catalogue_rows={row['key']:row for row in catalogue['requirements']}
    inherited_additions={row['key']:row for row in inherited['additions']}
    results={row['name']:row for row in receipt['results']};proof_cache={}
    recovered={row['key']:row for row in load(q['recovery_campaign'])['coverage']['requirements']['recovered_rows']}
    for row in mapping:
        key=row['key'];require(row['source_text']==known[key]['text']and row['fingerprint']==known[key]['fingerprint'],
                              'Inherited original configuration clause changed')
        origin=row['origin']
        if origin['kind']=='qualified-extension':
            # Reuse the completed, sealed qualification at its source version;
            # do not rerun its physics audit or GUI/native scenario collection.
            accepted=inherited_additions[key];prior=load(origin['qualification'])
            require(origin['qualification']==accepted['qualification']and
                    sha(origin['qualification'])==accepted['qualification_sha256']and
                    key in prior['qualified_keys']and prior['authority_sha256']==inventory['sources'],
                    'Missing full frame/body configuration conjunction')
            for p,h in prior['artifact_sha256'].items():require(sha(p)==h,'Changed inherited frame/body proof artifact')
        elif origin['kind']=='recovered':
            original=recovered[key];audit=load(original['audit'])
            require(origin['audit']==original['audit']and sha(origin['audit'])==original['audit_sha256']and
                    key in audit['accepted_source_versioned_scope_keys']['requirements'],'Original recovered configuration proof changed')
        else:
            require(origin['kind']=='baseline','Unknown configuration proof origin')
            original=catalogue_rows[key]
            require(original['status']=='verified'and original['fingerprint']==row['fingerprint']and
                    hashlib.sha256(json.dumps(original,sort_keys=True).encode()).hexdigest()==origin['row_sha256'],
                    'Original accepted configuration mapping differs')
            require(bool(original['evidence']),'Configuration clause has no actual proof')
            for evidence in original['evidence']:
                result=results[evidence['test']]
                require(result['status']=='passed'and result['returncode']==0 and
                        receipt['source_hashes'][evidence['source']]==evidence['source_sha256'],
                        'Inherited assertion/execution identity differs')
                if result['name']not in proof_cache:
                    work=Path(result['log']).parent
                    require({str(work/'test.log'),str(work/'ctest.xml')}<=set(q['artifact_sha256']),
                            'Inherited native log/JUnit omitted')
                    junit(work/'ctest.xml',result['name']);proof_cache[result['name']]=records(result)
                require(any(p['checkpoint']==evidence['checkpoint']and all(p.get(k)==v for k,v in evidence.get('expected',{}).items())
                            for p in proof_cache[result['name']]),'Inherited native configuration checkpoint missing')
    # Existing C boundary and lifetime evidence remains at its recorded versions.
    forwarding_versions=[]
    for name,p in q['existing_C_qualifications'].items():
        require(p in q['artifact_sha256'],'Existing C qualification not sealed')
        if name=='epoch':epochs.qualify(repo,Path(p),inventory)
        else:capi.qualify(repo,Path(p),inventory)
        old_run=Path(load(p)['run'])
        with tarfile.open(old_run/'sources.tar.gz')as archive:
            code=archive.extractfile('plugins/CInterfacePlugin/src/plugin/CInterfacePluginFunctions.cpp').read().decode()
            forwarding_versions.append(hashlib.sha256(function(code,'int LoadScript(').encode()).hexdigest())
    run=Path(q['run']);before=load(run/'inputs-before.json');after=load(run/'inputs-after.json');actual=load(run/'receipt.json')
    required={str(run/name)for name in ['inputs-before.json','inputs-after.json','receipt.json','client.log',
              'sources.tar.gz','junit.xml','configuration-junit.xml','configuration-oracle.json',
              'configuration-validation.json','session/acceptance.script','session/expected-epochs.txt',
              'session/gmat_startup_file.txt','session/output/configuration.csv']}
    required|={str(run/'session/oracles'/name)for name in
               ['NasaOrbitDefinitionCases.json','NasaEpochDefinitionCases.json','NasaAttitudeDefinitionCases.json']}
    require(required<=set(q['artifact_sha256']),'Essential executed configuration input/output omitted')
    require(before==after and actual['inputs_unchanged']and actual['status']=='passed'and actual['returncode']==0 and
            actual['GUI_executions']==actual['screenshots']==actual['runtime_copies']==0,'Missing real unchanged C client execution')
    require(actual['command'][2]=='spacecraftconfiguration'and
            actual['command'][0]in actual['loaded_project_images']and
            any(Path(p).name.startswith('libCInterface.so')for p in actual['loaded_project_images']),
            'C public interface was not actually loaded')
    for p,h in actual['loaded_project_images'].items():require(before[p]==h==actual['binaries'][p],'Unbound actual C runtime image')
    source={}
    with tarfile.open(run/'sources.tar.gz')as archive:
        require(set(archive.getnames())==set(actual['sources']),'Native source partition changed')
        for name,h in actual['sources'].items():
            data=archive.extractfile(name).read();require(hashlib.sha256(data).hexdigest()==h==before[str(repo/name)],'Changed executed C source')
            source[name]=data
    guard=load(q['build_guard']);require(guard['build_exit_code']==0 and
            str(repo/guard['executable'])==actual['command'][0]and guard['executable_sha256']==before[actual['command'][0]],
            'Executed C client does not match successful build')
    require(sha(guard['log'])==guard['log_sha256']and'FAILED:'not in Path(guard['log']).read_text(),'Changed/failed client build')
    for name,h in guard['compiled_sources'].items():require(actual['sources'][name]==h,'Client compiled source differs from execution')
    junit(run/'junit.xml','QtAcceptance.NasaCInterfaceSpacecraftconfiguration')
    junit(run/'configuration-junit.xml','QtAcceptance.NasaCInterfaceSpacecraftConfiguration')
    proof=[json.loads(line[11:])for line in(run/'client.log').read_text().splitlines()if line.startswith('NASA_CHECK ')]
    require(proof==actual['proofs']and len(proof)==1 and proof[0]['public_ABI']and proof[0]['spacecraft']==31,
            'Actual mixed spacecraft loading proof missing')
    # Regenerate the complete script and expected values from the EXECUTED
    # frozen generator and oracle assets, never from GMAT output.
    helper_name='src/qtgui/tests/RunCapiAcceptance.py';runner_name='src/qtgui/tests/RunCapiSpacecraftConfigurationAcceptance.py'
    helper=types.ModuleType('RunCapiAcceptance');helper.__file__=str(repo/helper_name)
    exec(compile(source[helper_name],helper.__file__,'exec'),helper.__dict__)
    prior=sys.modules.get('RunCapiAcceptance');sys.modules['RunCapiAcceptance']=helper
    try:
        runner=types.ModuleType('_frozen_configuration_generator');runner.__file__=str(repo/runner_name)
        exec(compile(source[runner_name],runner.__file__,'exec'),runner.__dict__)
    finally:
        if prior is None:sys.modules.pop('RunCapiAcceptance',None)
        else:sys.modules['RunCapiAcceptance']=prior
    raw_inputs={name:(run/'session/oracles'/name).read_bytes()for name in runner.INPUTS}
    for name,data in raw_inputs.items():require(hashlib.sha256(data).hexdigest()==before[str(run/'session/oracles'/name)],'Changed independent oracle bytes')
    script,expected_epochs,matrix=runner.fixtures(raw_inputs)
    require(script.isascii()and(run/'session/acceptance.script').read_text()==script and
            (run/'session/expected-epochs.txt').read_text()==expected_epochs and
            load(run/'configuration-oracle.json')==matrix,'Configuration script/oracle differs from executed frozen generator')
    checked=runner.check_rows(run/'session/output/configuration.csv',matrix)
    require(checked==load(run/'configuration-validation.json')and checked['status']=='passed'and checked['comparisons']==775,
            'Independent full C configuration report check missing')
    # Exact shared parser and generic construction/setting routes. Filename
    # encoding validation is preserved; no claim of Unicode-comment equivalence.
    routes=load(q['route_review']);interface='plugins/CInterfacePlugin/src/plugin/CInterfacePluginFunctions.cpp'
    require(hashlib.sha256(function(source[interface].decode(),'int LoadScript(').encode()).hexdigest()==routes['LoadScript_sha256'],
            'Reviewed public forwarding source changed')
    require(set(forwarding_versions)=={routes['LoadScript_sha256']},'Previously tested C loading function differs')
    parser='src/base/interpreter/ScriptInterpreter.cpp'
    require(actual['sources'][parser]==receipt['source_hashes'][parser]==routes['shared_parser_sha256'],
            'C/GUI shared configuration parser source differs')
    require('CheckEncoding()'in function(source[parser].decode(),'bool ScriptInterpreter::Interpret(const std::string &scriptfile)')and
            'retval = Interpret();'in function(source[parser].decode(),'bool ScriptInterpreter::Interpret(const std::string &scriptfile)'),
            'Filename encoding/shared parser route changed')
    for text in ['No C-specific spacecraft configuration filter','ASCII-valid executable configuration statements',
                 'E13 pairwise conversion remains open','E249/E251/E252 remain open']:
        require(text in routes['review_conclusions'],'Missing exact configuration/source-scope review')
    return q,[KEY]


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3]);parser.add_argument('--qualification',type=Path,required=True)
    args=parser.parse_args();repo=args.repo.resolve();_,keys=qualify(repo,args.qualification,nasa.inventory(repo));print(json.dumps(dict(status='passed',qualified_keys=keys,profiles=31,independent_values=775)))
