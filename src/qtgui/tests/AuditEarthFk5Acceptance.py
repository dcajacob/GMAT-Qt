"""Admit exact Earth FK5 orientation, preserving all broader/scientific gates."""
import argparse
import copy
import hashlib
import json
import math
from pathlib import Path
import re
import subprocess
import tarfile
import tempfile
import xml.etree.ElementTree as ET
import NasaGuiCoverage as nasa
from BuildNasaCoverageExtension import sha,load,require,validate_visible

KEYS=['FRR!E209']
KIND='source-versioned-Earth-FK5-orientation'
SELECTED=['QtGui.NasaEarthFk5Orientation','QtGui.NasaHarmonicGravityEarth','QtGui.NasaStationCartesianEllipsoid','QtGui.NasaMsise86Baseline']


def check_proof(proof,cases,directory=None):
    require(proof['model']=='FK5' and proof['outcome']=='passed' and proof['phases']==2,'Missing Earth FK5 conjunction')
    require(len(cases['epochs'])==15 and len(cases['rows'])==105 and cases['official_nutation_terms']==106,'Incomplete epoch/basis/coefficient matrix')
    require([(p['phase'],p['row']) for p in proof['samples']]==[(phase,row) for phase in range(2) for row in range(105)],'Missing phase/basis/epoch')
    maxp=maxv=0.
    for sample in proof['samples']:
        truth=cases['rows'][sample['row']];raw=sample['raw'];require(len(raw)==14 and all(math.isfinite(v) for v in raw),'Invalid Earth report')
        require(abs(raw[0]-truth['tai_mjd'])<=cases['time_tolerance_days'] and abs((raw[1]-raw[0])*86400-32.184)<1e-6,'Wrong independent time')
        require(all(abs(a-b)<1e-8 for a,b in zip(raw[2:8],truth['input_state'])),'Changed inertial input')
        for i,(a,b) in enumerate(zip(raw[8:],truth['fixed_state'])):
            delta=abs(a-b);require(delta<=cases['position_tolerance_km' if i<3 else 'velocity_tolerance_km_s'],'Independent Earth FK5 state differs')
            if i<3:maxp=max(maxp,delta)
            else:maxv=max(maxv,delta)
    require(proof['max_position_error_km']==maxp and proof['max_velocity_error_km_s']==maxv,'Residual summary differs')
    if directory:
        for phase in range(2):
            raw=[[float(v) for v in line.split(',')] for line in (directory/f'{phase}-states.csv').read_text().splitlines() if line.strip()]
            require(raw==[p['raw'] for p in proof['samples'] if p['phase']==phase],'Raw file differs from proof')
        script=(directory/'final.script').read_text()
        require('Fixed.Origin = Earth;' in script and 'Fixed.Axes = BodyFixed;' in script and 'Inertial.Axes = MJ2000Eq;' in script and 'Earth.NutationUpdateInterval = 0;' in script,'Native frame or update setting differs')
    return dict(position_km=maxp,velocity_km_s=maxv)


def qualify(repo,path,inventory):
    q=load(path);require(q['contract_kind']==KIND and q['qualified_keys']==KEYS,'Changed exact Earth scope')
    known={v['key']:v for v in inventory['requirements']};require(q['authority_sha256']==inventory['sources'] and q['original_requirements']==[dict(key=k,fingerprint=known[k]['fingerprint'],source_text=known[k]['text']) for k in KEYS],'Original authority differs')
    require(all(q.get(k) is False for k in ['technical_core_approval','scientific_approval','full_current_build_qualified','full_NASA_qualified']),'Qualification exceeds scope')
    for p,h in q['artifact_sha256'].items():require(not Path(p).is_symlink() and sha(p)==h,'Changed regular artifact '+p)
    def bound(p):require(str(p) in q['artifact_sha256'],'Unbound artifact '+str(p));return Path(p)
    before,after=(load(bound(q[k])) for k in ['inputs_before','inputs_after']);launch=load(bound(q['launch']))
    require(before==after and launch['inputs_unchanged'] and launch['returncode']==0,'Native inputs changed/failed launch')
    with tarfile.open(bound(q['source_archive'])) as tar:frozen={m.name:tar.extractfile(m).read() for m in tar.getmembers()}
    require({p:hashlib.sha256(b).hexdigest() for p,b in frozen.items()}==before['sources'],'Executed source archive differs')
    # Exact reference package identity is a reproducible prerequisite, not copied runtime.
    env=load(bound(q['reference_environment']))
    for p,h in env['files'].items():require(sha(p)==h,'Restore pinned reference environment: '+p)
    reference=Path(q['reference_dir']);downloads=load(bound(reference/'source-downloads.json'))
    for name,row in downloads.items():require(sha(bound(reference/name))==row['sha256'] and '/liberfa/erfa/v2.0.1/' in row['url'],'ERFA primary reference differs')
    with tempfile.TemporaryDirectory(prefix='earth-fk5-audit-') as temp:
        root=Path(temp)
        for p in ['src/qtgui/tests/EarthFk5Oracle.py','application/data/planetary_coeff/NUTATION.DAT','application/data/planetary_coeff/eopc04_08.62-now']:
            dest=root/p;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(frozen[p])
        code="import sys,json;from pathlib import Path;sys.path.insert(0,sys.argv[1]);import EarthFk5Oracle as o;Path(sys.argv[4]).write_text(json.dumps(o.generate(Path(sys.argv[2]),Path(sys.argv[3]))))"
        output=root/'cases.json';run=subprocess.run([q['reference_python'],'-c',code,str(root/'src/qtgui/tests'),str(root),str(reference),str(output)],capture_output=True,text=True)
        require(run.returncode==0,'Independent reference regeneration failed: '+run.stderr);cases=load(output)
    require(cases==json.loads(frozen['src/qtgui/tests/NasaEarthFk5Cases.json']),'Executed cases differ from independent regenerated reference')
    require(cases['position_tolerance_km']==1e-6 and cases['velocity_tolerance_km_s']==2e-10 and cases['time_tolerance_days']==1e-11,'Predeclared numerical bounds changed')
    contracts=bound(q['reference_contracts']).read_text();require('Ran 12 tests' in contracts and contracts.rstrip().endswith('OK'),'Reference contracts incomplete')
    # The unchanged direct native test must fail on old code and pass on new code.
    old=ET.parse(bound(q['eop_before_junit'])).getroot().findall('testcase');new=ET.parse(bound(q['eop_after_junit'])).getroot().findall('testcase')
    require(len(old)==len(new)==1 and old[0].find('failure') is not None and new[0].find('failure') is None and new[0].find('skipped') is None and new[0].attrib.get('status')=='run','Missing EOP before/after regression')
    require('EOP_RESULT checks=18 failures=5' in old[0].findtext('system-out') and 'EOP_RESULT checks=18 failures=0' in new[0].findtext('system-out'),'EOP regression counts changed')
    old_source=bound(q['eop_preimage']).read_text();new_source=frozen['src/gmatutil/util/EopFile.cpp'].decode()
    replacement='          // LOD is the left daily sample. Use a half-open cache interval so\n          // an exact next-day knot agrees with an uncached lookup.\n          (utcJD < data[(previousIndex+1)*col]) &&'
    require(new_source==old_source.replace('          (utcJD <= data[(previousIndex+1)*col]) &&',replacement),'Unexpected core changes')
    after_ids=load(bound(q['eop_after_identities']));old_ids=load(bound(q['eop_before_identities']))
    require(after_ids['source_sha256']==hashlib.sha256(new_source.encode()).hexdigest() and old_ids['source_sha256']==hashlib.sha256(old_source.encode()).hexdigest(),'Core source identity differs')
    require(after_ids['test_source_sha256']==hashlib.sha256(frozen['src/qtgui/tests/EopDailyKnotTests.cpp']).hexdigest(),'EOP direct assertions changed')
    require(any(Path(p).name.startswith('libGmatUtil.') and h==after_ids['image_sha256'] for p,h in before['images'].items()) and after_ids['image_sha256']!=old_ids['image_sha256'],'Fixed core image absent/unchanged')
    mapped=load(bound(q['loaded_images']))['processes'];summaries={};last=None
    for mode,receipt_path in q['runs'].items():
        receipt=load(bound(receipt_path));require(receipt['mode']==mode and receipt.get('finished_utc') and receipt['end_reason']=='completed-selection','Nonterminal native run')
        require(all(before['sources'].get(p)==h for p,h in receipt['source_hashes'].items()),'Native source version differs')
        active={r['name']:r for r in receipt['results'] if r['status']!='not-run'};require(set(active)==set(SELECTED),'Earth/affected selection incomplete');evidence={}
        for name,row in active.items():
            log=bound(row['log']);directory=log.parent;junit=bound(directory/'ctest.xml');require(row['status']=='passed' and row['returncode']==0,'Failed native case')
            tests=ET.parse(junit).getroot().findall('testcase');require(len(tests)==1 and tests[0].attrib['name']==name and tests[0].attrib.get('status')=='run' and tests[0].find('failure') is None and tests[0].find('skipped') is None,'Passing JUnit absent')
            for artifact in directory.rglob('*'):
                if artifact.is_file() and not artifact.is_symlink():bound(artifact)
            exes=[p for p in row['registered_command'] if p in receipt['binary_hashes']];require(len(exes)==1 and before['images'].get(exes[0])==receipt['binary_hashes'][exes[0]],'Executed image differs')
            evidence[name]=dict(log=str(log),log_sha256=sha(log),junit=str(junit),junit_sha256=sha(junit))
            if name==SELECTED[0]:
                proof=load(bound(directory/'proof.json'));matches=re.findall(r'NASA_CHECK (\{[^\n]+\})',log.read_text());require(len(matches)==1 and json.loads(matches[0])==proof,'Earth proof differs from stdout');summaries[mode]=check_proof(proof,cases,directory);last=proof
            if mode=='visible':
                traces=list(directory.glob('trace-*.jsonl'));require(traces,'Visible trace absent')
                for trace in traces:
                    process=mapped.get(trace.stem.removeprefix('trace-'));require(process and process['executable']==exes[0],'Visible process mapping absent')
                    for p,h in before['images'].items():
                        if p==exes[0] or Path(p).name.startswith(('libGmat','libMsise86')):require(process['images'].get(p)==h,'Mapped core/test differs')
        if mode=='visible':validate_visible(receipt,evidence)
    negatives=[]
    for label,mutate in [('wrong-position',lambda p:p['samples'][0]['raw'].__setitem__(8,0)),('wrong-velocity',lambda p:p['samples'][0]['raw'].__setitem__(11,0)),('wrong-epoch',lambda p:p['samples'][0]['raw'].__setitem__(0,21545)),('wrong-inertial-input',lambda p:p['samples'][0]['raw'].__setitem__(2,0)),('missing-basis',lambda p:p['samples'].pop(1)),('missing-reopen',lambda p:p.update(phases=1))]:
        corrupt=copy.deepcopy(last);mutate(corrupt)
        try:check_proof(corrupt,cases)
        except (AssertionError,ValueError):negatives.append(label)
        else:raise ValueError('Accepted corrupt proof '+label)
    q['audit_summary']=dict(qualified_keys=KEYS,native_states=420,epochs=15,states_per_epoch=7,official_nutation_terms=106,native_gui_passes=8,affected_regressions=3,reference_contracts=12,eop_direct_checks=18,eop_before_failures=5,max_errors=summaries,negative_controls=negatives,production_changes=['src/gmatutil/util/EopFile.cpp'])
    return q,KEYS


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--qualification',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();repo=Path(__file__).resolve().parents[3];q,keys=qualify(repo,a.qualification,nasa.inventory(repo));a.output.write_text(json.dumps(dict(status='passed',qualified_keys=keys,summary=q['audit_summary'],scientific_approval=False,technical_core_approval=False,full_NASA_qualified=False),indent=2)+'\n');print(json.dumps(q['audit_summary']))
