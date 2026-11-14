"""Admit the complete default-body IAU2002 orientation parent with its exceptions."""
import argparse
import copy
import hashlib
import json
import math
from pathlib import Path
import re
import subprocess
import sys
import tarfile
import tempfile
import xml.etree.ElementTree as ET
import NasaGuiCoverage as nasa
from BuildNasaCoverageExtension import sha,load,require,validate_visible

KEYS=['FRR!E208']
KIND='source-versioned-complete-default-body-IAU2002-orientation'
BODIES=['Sun','Mercury','Venus','Mars','Jupiter','Saturn','Uranus','Neptune','Pluto']
DEFAULTS=sorted(BODIES+['Earth','Luna'])
SELECTED=['QtGui.NasaDefaultOrientation'+b for b in BODIES]


def check_proof(proof,matrix,body,directory=None):
    case=next(c for c in matrix['cases'] if c['body']==body)
    require(proof['body']==body and proof['outcome']=='passed' and proof['phases']==2,'Missing default-body conjunction')
    require(proof['default_bodies']==DEFAULTS and sorted(matrix['default_bodies'])==DEFAULTS,'Default-body census differs')
    require(proof['rotation_source']==case['rotation_source'] and proof['orientation_parameters']==case['orientation_parameters'],'Wrong native model/constants')
    require(len(case['rows'])==84 and [(p['phase'],p['row']) for p in proof['samples']]==[(phase,row) for phase in range(2) for row in range(84)],'Missing phase/basis/epoch')
    maxp=maxv=0.
    for sample in proof['samples']:
        truth=case['rows'][sample['row']];raw=sample['raw'];require(len(raw)==14 and all(math.isfinite(v) for v in raw),'Invalid default-body report')
        require(abs(raw[0]-truth['tdb_mjd'])<=matrix['tdb_tolerance_days'],'Wrong independent epoch')
        for i,(a,b) in enumerate(zip(raw[2:8],truth['input_state'])):require(abs(a-b)<=matrix['input_tolerance_km' if i<3 else 'velocity_tolerance_km_s'],'Changed inertial input')
        for i,(a,b) in enumerate(zip(raw[8:],truth['fixed_state'])):
            delta=abs(a-b);require(delta<=matrix['position_tolerance_km' if i<3 else 'velocity_tolerance_km_s'],'Independent historical IAU state differs')
            if i<3:maxp=max(maxp,delta)
            else:maxv=max(maxv,delta)
    require(proof['max_position_error_km']==maxp and proof['max_velocity_error_km_s']==maxv,'Residual summary differs')
    if directory:
        for phase in range(2):
            raw=[[float(v) for v in line.split(',')] for line in (directory/f'{phase}-states.csv').read_text().splitlines() if line.strip()]
            require(raw==[p['raw'] for p in proof['samples'] if p['phase']==phase],'Raw file differs from proof')
        script=(directory/'final.script').read_text()
        for text in [f'Fixed.Origin = {body};',f'Inertial.Origin = {body};','Fixed.Axes = BodyFixed;','Inertial.Axes = MJ2000Eq;','SolarSystem.UseTTForEphemeris = false;','SolarSystem.EphemerisUpdateInterval = 0;']:
            require(text in script,'Native model/frame/clock differs')
    return dict(position_km=maxp,velocity_km_s=maxv)


def qualify(repo,path,inventory):
    q=load(path);require(q['contract_kind']==KIND and q['qualified_keys']==KEYS,'Changed exact default-body scope')
    known={v['key']:v for v in inventory['requirements']};require(q['authority_sha256']==inventory['sources'] and q['original_requirements']==[dict(key=k,fingerprint=known[k]['fingerprint'],source_text=known[k]['text']) for k in KEYS],'Original authority differs')
    require(all(q.get(k) is False for k in ['technical_core_approval','scientific_approval','full_current_build_qualified','full_NASA_qualified']),'Qualification exceeds scope')
    for p,h in q['artifact_sha256'].items():require(not Path(p).is_symlink() and sha(p)==h,'Changed regular artifact '+p)
    def bound(p):require(str(p) in q['artifact_sha256'],'Unbound artifact '+str(p));return Path(p)
    # The two exceptions are complete, independently re-audited source-versioned
    # conjunctions. Their native executions are reused, not silently reattributed.
    from AuditEarthFk5Acceptance import qualify as earth
    from AuditLunarOrientationAcceptance import qualify as luna
    eq,ek=earth(repo,bound(q['earth_qualification']),inventory);lq,lk=luna(repo,bound(q['lunar_qualification']),inventory)
    require(ek==['FRR!E209'] and lk==['FRR!E210','FRR!E211','FRR!E212'],'Missing Earth/Moon exceptions')
    before,after=(load(bound(q[k])) for k in ['inputs_before','inputs_after']);launch=load(bound(q['launch']))
    require(before==after and launch['inputs_unchanged'] and launch['returncode']==0,'Native inputs changed/failed launch')
    with tarfile.open(bound(q['source_archive'])) as tar:frozen={m.name:tar.extractfile(m).read() for m in tar.getmembers()}
    require({p:hashlib.sha256(b).hexdigest() for p,b in frozen.items()}==before['sources'],'Executed source archive differs')
    with tempfile.TemporaryDirectory(prefix='default-body-audit-') as temp:
        root=Path(temp)
        for p in ['src/qtgui/tests/DefaultBodyOrientationOracle.py','src/qtgui/tests/LunarOrientationOracle.py','src/qtgui/tests/fixtures/lunar/pck00008.tpc']:
            dest=root/p;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(frozen[p])
        output=root/'cases.json';run=subprocess.run([sys.executable,str(root/'src/qtgui/tests/DefaultBodyOrientationOracle.py'),'--output',str(output)],capture_output=True,text=True)
        require(run.returncode==0,'Independent reference regeneration failed: '+run.stderr);cases=load(output)
    require(cases==json.loads(frozen['src/qtgui/tests/NasaDefaultBodyOrientationCases.json']),'Executed cases differ from independently regenerated reference')
    require([c['body'] for c in cases['cases']]==BODIES and sorted(cases['default_bodies'])==DEFAULTS,'Incomplete model census')
    require(cases['pck_sha256']==sha(lq['PCK_file']),'Historical IAU2002 reference identity differs')
    require(cases['position_tolerance_km']==cases['input_tolerance_km']==5e-6 and cases['velocity_tolerance_km_s']==5e-10 and cases['tdb_tolerance_days']==2e-10,'Predeclared bounds changed')
    # Exact production deltas, not a broad source-tree assertion.
    replacements={
      'CelestialBody':('   return (mjdTDB + GmatTimeConstants::JD_JAN_5_1941 -\n          GmatTimeConstants::JD_OF_J2000);','   // Subtract the small epoch offset directly. Forming a full Julian date\n   // first discards fractional-day precision used by the orientation model.\n   return mjdTDB - (GmatTimeConstants::JD_OF_J2000 -\n                   GmatTimeConstants::JD_JAN_5_1941);'),
      'Planet':('      //Real NDot = 52.316 * CelestialBody::TDot;\n\t  //NDot = 52.316 (deg/century) * (1 century/36525 days)\n      Real NDot      = 0.0014323;   // fixed conversion error from spec, (SPH) 02 Nov. 2012','      // N is specified in degrees. Differentiating sin(Rad(N)) requires\n      // the argument rate in radians/day; Wdot below remains degrees/day.\n      Real NDot = Rad(52.316) / GmatTimeConstants::DAYS_PER_JULIAN_CENTURY;')}
    for name,(old,new) in replacements.items():
        preimage=bound(q['core_preimages'][name]).read_text();require(preimage.count(old)==1 and frozen[f'src/base/solarsys/{name}.cpp'].decode()==preimage.replace(old,new),'Unexpected core delta '+name)
    old=load(bound(q['before_run']));active={r['name']:r for r in old['results'] if r['status']!='not-run'}
    require(set(active)==set(SELECTED),'Before case family differs')
    failed={'Mars','Jupiter','Saturn','Uranus','Neptune'}
    for body in BODIES:require(active['QtGui.NasaDefaultOrientation'+body]['status']==('failed' if body in failed else 'passed'),'Before failures differ')
    with tarfile.open(bound(q['before_sources'])) as tar:
        for p in ['src/qtgui/tests/NasaDefaultBodyOrientationTests.cpp','src/qtgui/tests/NasaDefaultBodyOrientationCases.json','src/qtgui/tests/DefaultBodyOrientationOracle.py']:
            require(tar.extractfile(p).read()==frozen[p],'Original native assertions/reference changed')
    old_image=load(bound(q['before_core_image']))['GmatBase_sha256'];require(any(Path(p).name.startswith('libGmatBase.') and h!=old_image for p,h in before['images'].items()),'Fixed core image absent/unchanged')
    mapped=load(bound(q['loaded_images']))['processes'];summaries={};proofs={}
    require(set(q['runs'])=={'headless','visible'},'Missing execution mode')
    for mode,receipt_path in q['runs'].items():
        receipt=load(bound(receipt_path));require(receipt['mode']==mode and receipt.get('finished_utc') and receipt['end_reason']=='completed-selection','Nonterminal native run')
        require(all(before['sources'].get(p)==h for p,h in receipt['source_hashes'].items()),'Native source version differs')
        active={r['name']:r for r in receipt['results'] if r['status']!='not-run'};require(set(active)==set(SELECTED),'Default selection incomplete');evidence={}
        for name,row in active.items():
            log=bound(row['log']);directory=log.parent;junit=bound(directory/'ctest.xml');require(row['status']=='passed' and row['returncode']==0,'Failed native case')
            tests=ET.parse(junit).getroot().findall('testcase');require(len(tests)==1 and tests[0].attrib['name']==name and tests[0].attrib.get('status')=='run' and tests[0].find('failure') is None and tests[0].find('skipped') is None,'Passing JUnit absent')
            for artifact in directory.rglob('*'):
                if artifact.is_file() and not artifact.is_symlink():bound(artifact)
            exes=[p for p in row['registered_command'] if p in receipt['binary_hashes']];require(len(exes)==1 and before['images'].get(exes[0])==receipt['binary_hashes'][exes[0]],'Executed image differs')
            evidence[name]=dict(log=str(log),log_sha256=sha(log),junit=str(junit),junit_sha256=sha(junit))
            proof=load(bound(directory/'proof.json'));matches=re.findall(r'NASA_CHECK (\{[^\n]+\})',log.read_text());require(len(matches)==1 and json.loads(matches[0])==proof,'Default proof differs from stdout')
            body=name.removeprefix('QtGui.NasaDefaultOrientation');summaries[mode+'-'+body]=check_proof(proof,cases,body,directory);proofs[body]=proof
            if mode=='visible':
                traces=list(directory.glob('trace-*.jsonl'));require(traces,'Visible trace absent')
                for trace in traces:
                    process=mapped.get(trace.stem.removeprefix('trace-'));require(process and process['executable']==exes[0],'Visible process mapping absent')
                    for p,h in before['images'].items():
                        if p==exes[0] or Path(p).name.startswith('libGmat'):require(process['images'].get(p)==h,'Mapped core/test differs')
        if mode=='visible':validate_visible(receipt,evidence)
    negatives=[]
    for label,mutate in [('wrong-position',lambda p:p['samples'][0]['raw'].__setitem__(8,0)),('wrong-velocity',lambda p:p['samples'][0]['raw'].__setitem__(11,0)),('wrong-epoch',lambda p:p['samples'][0]['raw'].__setitem__(0,21000)),('wrong-inertial-input',lambda p:p['samples'][0]['raw'].__setitem__(2,0)),('missing-basis',lambda p:p['samples'].pop(1)),('missing-reopen',lambda p:p.update(phases=1)),('wrong-model',lambda p:p.update(rotation_source='FK5')),('wrong-constant',lambda p:p['orientation_parameters'].__setitem__(0,0)),('missing-body',lambda p:p['default_bodies'].remove('Earth'))]:
        corrupt=copy.deepcopy(proofs['Neptune']);mutate(corrupt)
        try:check_proof(corrupt,cases,'Neptune')
        except (AssertionError,ValueError):negatives.append(label)
        else:raise ValueError('Accepted corrupt proof '+label)
    q['audit_summary']=dict(qualified_keys=KEYS,default_bodies=DEFAULTS,exception_keys=ek+lk,native_states=3024,epoch_rows_per_model=12,states_per_epoch=7,new_native_gui_passes=18,reused_exception_qualifications=2,max_errors=summaries,negative_controls=negatives,before_native_failures=5,production_changes=['src/base/solarsys/CelestialBody.cpp','src/base/solarsys/Planet.cpp'])
    return q,KEYS

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--qualification',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();repo=Path(__file__).resolve().parents[3];q,keys=qualify(repo,a.qualification,nasa.inventory(repo));a.output.write_text(json.dumps(dict(status='passed',qualified_keys=keys,summary=q['audit_summary'],scientific_approval=False,technical_core_approval=False,full_NASA_qualified=False),indent=2)+'\n');print(json.dumps(q['audit_summary']))
