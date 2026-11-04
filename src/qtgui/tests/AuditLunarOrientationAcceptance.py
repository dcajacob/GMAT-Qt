"""Admit the complete Moon DE405/IAU2002 conjunction from versioned evidence.

IAU2002 denotes the IAU/IAG 2000 report published in 2002, as identified in
NASA NAIF's pck00008 release. Modern PCKs and Earth/FK5 are not substitutes.
Scientific approval and full current-build/NASA qualification remain open.
"""
import argparse
import copy
import hashlib
import json
import math
from pathlib import Path
import re
import tarfile
import types
import xml.etree.ElementTree as ET
import NasaGuiCoverage as nasa
from BuildNasaCoverageExtension import sha, load, require, validate_visible

KEYS = ['FRR!E210', 'FRR!E211', 'FRR!E212']
MODELS = ['DE405', 'IAU2002']
SELECTED = ['QtGui.NasaLunarOrientation'+m for m in MODELS]
FACETS = ['actual-DE405-solar-controls','actual-BodyFixed-axis-controls','actual-lunar-source-controls','independent-TDB-input','official-NASA-model-oracle','position-and-velocity','boundaries-and-out-of-order-epochs','actual-report-viewer','unicode-save-reopen']


def check_proof(proof, cases, directory=None):
    model=proof['model']; case=next(c for c in cases['cases'] if c['model']==model)
    phases=2 if model=='DE405' else 3
    require(proof['outcome']=='passed' and proof['phases']==phases and proof['facets']==FACETS, 'Missing lunar GUI conjunction')
    require(proof['icrf_range_rejections']==(model=='IAU2002') and proof['cancels']==(model=='IAU2002'), 'Range rejection or Cancel missing')
    rows=case['rows'];require(len(rows)==105 and [r['witness'] for r in rows[:7]]==['mixed']+['basis-'+str(i) for i in range(6)], 'Full six-dimensional basis absent')
    require([(r['phase'],r['row']) for r in proof['samples']]==[(p,r) for p in range(phases) for r in range(105)], 'Missing/reordered lunar witness')
    maxp=maxv=0.
    for sample in proof['samples']:
        truth=rows[sample['row']]
        require(abs(sample['tdb_mjd']-truth['tdb_mjd'])<=cases['tdb_tolerance_days'], 'Wrong TDB epoch')
        require(len(sample['actual'])==6 and all(math.isfinite(v) for v in sample['actual']), 'Invalid Cartesian output')
        for i,(a,b) in enumerate(zip(sample['actual'],truth['fixed_state'])):
            error=abs(a-b);require(error<=cases['position_tolerance_km' if i<3 else 'velocity_tolerance_km_s'], 'Independent lunar transformation differs')
            if i<3:maxp=max(maxp,error)
            else:maxv=max(maxv,error)
    require(proof['max_position_error_km']==maxp and proof['max_velocity_error_km_s']==maxv,'Reported residual differs')
    if directory:
        for phase,label in enumerate(['gui-selected','reopened']+(['reselected'] if phases==3 else [])):
            raw=[[float(x) for x in line.split()] for line in (directory/f'{phase}-{label}-states.txt').read_text().splitlines() if line.strip()]
            require(len(raw)==105 and all(len(r)==13 for r in raw), 'Raw report incomplete')
            script=(directory/f'{phase}-{label}.script').read_text()
            require('Fixed.Axes = BodyFixed;' in script and 'SolarSystem.EphemerisSource = DE405;' in script and 'SolarSystem.UseTTForEphemeris = false;' in script and 'SolarSystem.EphemerisUpdateInterval = 0;' in script, 'Selected GUI model/clock missing')
            if model=='IAU2002':
                require("Luna.SpiceFrameId = 'IAU_MOON';" in script and "pck00008.tpc'};" in script and 'Luna.RotationDataSource = SPICE;' in script, 'Historical IAU model selection missing')
            for i,(values,truth) in enumerate(zip(raw,rows)):
                require(values[0]==proof['samples'][phase*105+i]['tdb_mjd'] and values[7:]==proof['samples'][phase*105+i]['actual'], 'Raw report differs from proof')
                require(all(abs(a-b)<=1e-8 for a,b in zip(values[1:7],truth['input_state'])), 'Raw inertial dependency differs')
        if model=='IAU2002':
            require('independent variable is out of range' in (directory/'icrf-domain-rejection-messages.txt').read_text(), 'Documented ICRF boundary not rejected')
            require('Sat.TDBModJulian = 70000.5;' in (directory/'icrf-domain-rejection.script').read_text(), 'Range witness changed')
    return maxp,maxv


def qualify(repo,path,inventory):
    q=load(path);require(q['contract_kind']=='source-versioned-lunar-DE405-IAU2002-conjunction' and q['qualified_keys']==KEYS,'Changed lunar scope')
    require(q['authority_sha256']==inventory['sources'],'Original NASA authority changed')
    known={r['key']:r for r in inventory['requirements']}
    require(q['original_requirements']==[dict(key=k,fingerprint=known[k]['fingerprint'],source_text=known[k]['text']) for k in KEYS],'Original lunar obligations changed')
    require(all(q.get(k) is False for k in ['technical_core_approval','scientific_approval','full_current_build_qualified','full_NASA_qualified']),'Lunar qualification exceeds scope')
    for name,digest in q['artifact_sha256'].items():require(sha(name)==digest,'Changed lunar artifact: '+name)
    def bound(p):require(str(p) in q['artifact_sha256'],'Consumed artifact omitted: '+str(p));return Path(p)
    before,after=(load(bound(q[k])) for k in ['inputs_before','inputs_after'])
    require(before==after and load(bound(q['launch']))['inputs_unchanged'],'Native inputs changed')
    with tarfile.open(bound(q['source_archive'])) as tar:frozen={m.name:tar.extractfile(m).read() for m in tar.getmembers()}
    require({p:hashlib.sha256(b).hexdigest() for p,b in frozen.items()}==before['sources'],'Executed source archive differs')
    oracle=types.ModuleType('frozen_lunar_oracle');exec(compile(frozen['src/qtgui/tests/LunarOrientationOracle.py'],'frozen_lunar_oracle','exec'),oracle.__dict__)
    for p in q['reference_artifacts']:bound(p)
    reference=Path(q['reference_dir']);cases=oracle.generate(repo,reference)
    require(cases==json.loads(frozen['src/qtgui/tests/NasaLunarOrientationCases.json']),'Native cases differ from independently regenerated NASA models')
    require(cases['official_DE405_coefficients_checked']==360 and cases['DE405_sha256']==sha(bound(q['DE405_file'])) and cases['pck_sha256']==sha(bound(q['PCK_file'])),'Exact model identity not established')
    announcement=bound(q['IAU_release']).read_text();require('Seidelmann' in announcement and '(2002)' in announcement and '2000' in announcement and 'pck00008' in announcement,'IAU publication identity missing')
    mapped=load(bound(q['loaded_images']))['processes'];proofs={};summaries={}
    for mode in ['headless','visible']:
        receipt=load(bound(q['runs'][mode]));require(receipt['mode']==mode and receipt.get('finished_utc') and receipt['end_reason']=='completed-selection','Nonterminal native run')
        require(all(before['sources'].get(p)==h for p,h in receipt['source_hashes'].items()),'Native source version differs')
        active={r['name']:r for r in receipt['results'] if r['status']!='not-run'};require(set(active)==set(SELECTED),'Native model selection incomplete')
        evidence={}
        for name,row in active.items():
            log=bound(row['log']);junit=bound(log.parent/'ctest.xml');proof=load(bound(log.parent/'proof.json'))
            require(row['status']=='passed' and row['returncode']==0,'Failed native lunar case')
            tests=ET.parse(junit).getroot().findall('testcase');require(len(tests)==1 and tests[0].attrib['name']==name and tests[0].attrib.get('status')=='run' and tests[0].find('failure') is None and tests[0].find('skipped') is None,'Exact passing JUnit missing')
            match=re.findall(r'NASA_CHECK (\{[^\n]+\})',log.read_text());require(len(match)==1 and json.loads(match[0])==proof,'Proof differs from stdout')
            exes=[p for p in row['registered_command'] if p in receipt['binary_hashes']];require(len(exes)==1 and before['images'].get(exes[0])==receipt['binary_hashes'][exes[0]],'Executed image differs')
            for artifact in log.parent.iterdir():
                if artifact.is_file():bound(artifact)
            proofs[proof['model']]=proof;summaries[mode+'-'+proof['model']]=check_proof(proof,cases,log.parent)
            evidence[name]=dict(log=str(log),log_sha256=sha(log),junit=str(junit),junit_sha256=sha(junit))
            if mode=='visible':
                traces=list(log.parent.glob('trace-*.jsonl'));require(traces,'Visible trace missing')
                for trace in traces:
                    bound(trace);process=mapped.get(trace.stem.removeprefix('trace-'));require(process and process['executable']==exes[0],'Mapped process absent')
                    for image,digest in before['images'].items():
                        if Path(image).name.startswith(('libGmat','GmatQtNasaLunar')):require(process['images'].get(image)==digest,'Mapped engine/executable differs')
        if mode=='visible':validate_visible(receipt,evidence)
    negative=[]
    for name,mutate in [('wrong-position',lambda p:p['samples'][0]['actual'].__setitem__(0,p['samples'][0]['actual'][0]+1)),('wrong-velocity',lambda p:p['samples'][0]['actual'].__setitem__(3,p['samples'][0]['actual'][3]+1e-4)),('wrong-epoch',lambda p:p['samples'][0].update(tdb_mjd=21546)),('missing-basis',lambda p:p['samples'].pop(1)),('missing-reselection',lambda p:p.update(phases=2)),('missing-range-rejection',lambda p:p.update(icrf_range_rejections=0))]:
        changed=copy.deepcopy(proofs['IAU2002']);mutate(changed)
        try:check_proof(changed,cases)
        except (AssertionError,ValueError):negative.append(name)
        else:raise ValueError('Auditor accepted '+name)
    q['audit_summary']=dict(qualified_keys=KEYS,native_samples=1050,epochs_per_model=15,states_per_epoch=7,official_coefficients=360,max_errors=summaries,negative_controls=negative,production_changes=[])
    return q,KEYS


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--qualification',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();repo=Path(__file__).resolve().parents[3]
    q,keys=qualify(repo,args.qualification,nasa.inventory(repo));args.output.write_text(json.dumps(dict(status='passed',qualified_keys=keys,summary=q['audit_summary'],scientific_approval=False,technical_core_approval=False,full_NASA_qualified=False),indent=2)+'\n');print(json.dumps(q['audit_summary']))
