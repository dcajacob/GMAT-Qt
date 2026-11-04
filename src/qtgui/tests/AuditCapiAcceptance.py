"""Qualify exact C-ABI requirements using real engine receipts, never GUI credit.

The allowlist describes complete tested contracts. Script family parents,
all-command summaries, a public epoch getter and MATLAB remain unqualified.
"""
import hashlib
import json
import math
from pathlib import Path
import tarfile
import xml.etree.ElementTree as ET

BASIC_KEYS={f'All Others!E{i}' for i in (246,253,254,255,260,263,266,267,268,270)}
STM_KEYS={f'All Others!E{i}' for i in (264,265,271)}
EPOCH_KEYS={f'All Others!E{i}' for i in (259,261,262)}

def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def load(p):
    return json.loads(Path(p).read_text())

def require(ok,message):
    if not ok:raise ValueError(message)

def near(a,b,tol):
    return math.isfinite(a) and math.isfinite(b) and abs(a-b)<=tol

def newton(s):
    r=math.sqrt(sum(x*x for x in s[:3]));return s[3:6]+[-398600.4415*x/r**3 for x in s[:3]]

def integrate(s,seconds):
    s=list(s);steps=math.ceil(abs(seconds)/.1);h=seconds/steps
    for _ in range(steps):
        a=newton(s);b=newton([x+h*y/2 for x,y in zip(s,a)])
        c=newton([x+h*y/2 for x,y in zip(s,b)]);d=newton([x+h*y for x,y in zip(s,c)])
        s=[x+h*(aa+2*bb+2*cc+dd)/6 for x,aa,bb,cc,dd in zip(s,a,b,c,d)]
    return s

def validate_proof(p,mode):
    stm=mode in ('stm','epoch');n=42 if stm else 6
    require(p['checkpoint']=='c-api-'+mode and p['outcome']=='passed' and p['assertions']>=(133 if not stm else 450)
            and p['objects']==13 and p['models']==2 and p['dimension']==n,'Incomplete C ABI checkpoint')
    require(all(len(p[name])==2 and all(len(v)==n and all(math.isfinite(x) for x in v) for v in p[name])
                for name in ('post_run','assigned','derivatives')),'Missing complete finite model vectors')
    for m,(r,seconds) in enumerate(((7000,60),(9000,30))):
        w=math.sqrt(398600.4415/r**3);s=math.sin(w*seconds);c=math.cos(w*seconds)
        analytic=[r*c,r*s,0,-r*w*s,r*w*c,0] if m==0 else [0,r*c,r*s,0,-r*w*s,r*w*c]
        require(all(near(a,b,2e-5 if i<3 else 3e-8) for i,(a,b) in enumerate(zip(p['post_run'][m][:6],analytic))),'Independent circular state failed')
        assigned=[7100,-250,330,.2,7.2,-.3] if m==0 else [-300,9400,1200,6.1,-.2,.1]
        require(p['assigned'][m][:6]==assigned,'Distinct caller state not retained')
        require(all(near(a,b,1e-12) for a,b in zip(p['derivatives'][m][:6],newton(assigned))),'Independent Newton acceleration failed')
        if stm:
            initial=[7000,0,0,0,math.sqrt(398600.4415/7000),0] if m==0 else [0,9000,0,0,0,math.sqrt(398600.4415/9000)]
            for j in range(6):
                plus=list(initial);minus=list(initial);eps=.01 if j<3 else 1e-5;plus[j]+=eps;minus[j]-=eps
                plus=integrate(plus,seconds);minus=integrate(minus,seconds)
                require(all(near(p['post_run'][m][6+i*6+j],(plus[i]-minus[i])/(2*eps),3e-6) for i in range(6)),'Independent STM finite difference failed')
            r2=sum(x*x for x in assigned[:3]);r=math.sqrt(r2)
            a=[[0.]*6 for _ in range(6)]
            for i in range(3):
                a[i][i+3]=1
                for j in range(3):a[i+3][j]=398600.4415*(3*assigned[i]*assigned[j]/r**5-(1/r**3 if i==j else 0))
            phi=[(1 if i==j else 0)+.01*(i+1)*(j+2) for i in range(6) for j in range(6)]
            require(p['assigned'][m][6:]==phi,'Nonidentity STM not assigned')
            require(all(near(p['derivatives'][m][6+i*6+j],sum(a[i][k]*phi[k*6+j] for k in range(6)),2e-12) for i in range(6) for j in range(6)),'Independent A times Phi failed')
    if mode=='epoch':
        require(p['assertions']>=600 and p['epochs']==[[21545.75]*5,[21546.75]*5],
                'Native scalar/precise model/force epochs or isolation differ')

def qualify(repo,path,inventory):
    q=load(path);mode=q['case'];expected={'basic':BASIC_KEYS,'stm':STM_KEYS,'epoch':EPOCH_KEYS}.get(mode,set())
    require(q['contract_kind']=='source-versioned-native-C-API' and expected and len(q['qualified_keys'])==len(expected)
            and set(q['qualified_keys'])==expected,'Unsupported or duplicate C API credit')
    require(all(q.get(k) is False for k in ('full_current_build_qualified','full_NASA_qualified','technical_core_approval')),'C API proof overclaims qualification')
    for name,digest in q['artifact_sha256'].items():require(sha(name)==digest,'Changed C API artifact: '+name)
    run=Path(q['run']);before=load(run/'inputs-before.json');after=load(run/'inputs-after.json');receipt=load(run/'receipt.json')
    require(before==after and receipt['inputs_unchanged'] is True,'C API inputs changed')
    require(receipt['mode']=='native-C-API' and receipt['status']=='passed' and receipt['returncode']==0
            and receipt['GUI_executions']==receipt['screenshots']==receipt['runtime_copies']==0,'C API execution missing or misrepresented')
    require(all(receipt.get(k) is False for k in ('full_current_build_qualified','full_NASA_qualified','technical_core_approval')),'C API receipt exceeds scope')
    command=receipt['command'];require(len(command)==(4 if mode=='epoch' else 3) and command[2]==mode and command[0] in receipt['loaded_project_images'],'Actual C client not bound')
    if mode=='epoch':require(command[3] in receipt['loaded_project_images'],'Private read-only epoch probe not bound')
    require(receipt['binaries'] and receipt['loaded_project_images'] and set(receipt['loaded_project_images'])<=set(receipt['binaries']),'Unbound loaded C API images')
    require(any(Path(p).name.startswith('libCInterface.so') for p in receipt['loaded_project_images']),'Real CInterface not loaded')
    for p,digest in {**receipt['binaries'],**receipt['loaded_project_images']}.items():require(before[p]==digest,'Binary pre-execution identity differs')
    require(before[command[1]]==sha(command[1]),'Changed actual mission fixture')
    require(before[str(Path(receipt['cwd'])/'gmat_startup_file.txt')]==sha(Path(receipt['cwd'])/'gmat_startup_file.txt'),'Changed private startup')
    with tarfile.open(run/'sources.tar.gz') as bundle:
        require({m.name for m in bundle.getmembers()}==set(receipt['sources']),'C API source partition differs')
        frozen={}
        for name,digest in receipt['sources'].items():
            require(not Path(name).is_absolute() and '..' not in Path(name).parts,'Unsafe source member')
            data=bundle.extractfile(name).read()
            require(not data.startswith(b'\x7fELF') and hashlib.sha256(data).hexdigest()==digest==before[str(repo/name)],'Changed/non-source C API member')
            frozen[name]=data
    code=frozen['src/qtgui/tests/NasaCapiAcceptanceTests.c'].decode()
    for token in ('#ifdef __cplusplus','#include "CInterfaceFunctions.h"','CountObjects()==13','strcmp(GetStateDescription(),expected_description)==0',
                  'SetState(21545.25+m,given[m],n)==0','SetModelByName(m?"FM2":"FM1")==ids[m]','GetState()[i]==given[m][i]',
                  'GetDerivativesForState','jacobian(given[m],a)','rk4(plus,seconds[m])','value+=a[i*6+k]*given[m][6+k*6+j]'):
        require(token in code,'Missing original C API assertion: '+token)
    if mode=='epoch':
        probe=frozen['src/qtgui/tests/NasaCapiEpochProbe.cpp'].decode()
        for token in ('state->GetEpoch()','state->GetEpochGT().GetMjd()',
                      'ode->GetRealParameter(ode->GetParameterID("Epoch"))',
                      'force->GetRealParameter(force->GetParameterID("Epoch"))',
                      'force->GetGmatTimeParameter(force->GetParameterID("Epoch")).GetMjd()'):
            require(token in probe,'Epoch proof omits actual native epoch storage')
        require('SetState(-1.,given[m],n)==-1' in code and 'epochs[m][i]==21545.75+m' in code,
                'Epoch rejection or model isolation assertion missing')
    review=load(q['review']);known={r['key']:r for r in inventory['requirements']}
    require(review['scope']=='native-C-API' and review['authority_sha256']==inventory['sources'],'Wrong C API scope or authority')
    rows=[r for r in review['reviews'] if r['case']==mode]
    require(len(rows)==len(expected) and {r['key'] for r in rows}==expected,'C API row conjunction incomplete')
    for row in rows:require(row['fingerprint']==known[row['key']]['fingerprint'] and row['source_text']==known[row['key']]['text'] and row['rationale'],'Changed NASA source-row scope')
    root=ET.parse(run/'junit.xml').getroot();cases=root.findall('testcase')
    require(len(cases)==1 and cases[0].get('name')=='QtAcceptance.NasaCInterface'+mode.title() and not cases[0].findall('failure') and not cases[0].findall('skipped'),'Missing exact passing C API JUnit')
    proof=[json.loads(s[len('NASA_CHECK '):]) for s in (run/'client.log').read_text().splitlines() if s.startswith('NASA_CHECK ')]
    require(len(proof)==1 and receipt['proofs']==proof,'C API proof differs from actual log')
    validate_proof(proof[0],mode)
    require(q['authority_sha256']==inventory['sources'],'C API qualification authority differs')
    return q,sorted(expected)


def create_qualification(repo,run,mode,review,output,inventory):
    """Seal post-execution review tools without copying any runtime images."""
    import shutil
    require(not output.exists(),'Qualification output already exists')
    run=run.resolve();output=output.resolve();output.parent.mkdir(parents=True,exist_ok=True)
    tools=output.with_suffix('.review');tools.mkdir(exist_ok=False)
    frozen_review=tools/'NasaCapiTraceability.json'
    shutil.copyfile(review,frozen_review)
    frozen_auditor=tools/'AuditCapiAcceptance.py'
    shutil.copyfile(Path(__file__),frozen_auditor)
    files=[*(run/name for name in ('inputs-before.json','inputs-after.json','receipt.json','client.log','junit.xml','sources.tar.gz')),
           run/'session/acceptance.script',run/'session/gmat_startup_file.txt',frozen_review,frozen_auditor]
    keys={'basic':BASIC_KEYS,'stm':STM_KEYS,'epoch':EPOCH_KEYS}[mode]
    q=dict(contract_kind='source-versioned-native-C-API',case=mode,run=str(run),review=str(frozen_review),
           qualified_keys=sorted(keys),authority_sha256=inventory['sources'],artifact_sha256={str(p):sha(p) for p in files},
           full_current_build_qualified=False,full_NASA_qualified=False,technical_core_approval=False)
    candidate=tools/'candidate.json';candidate.write_text(json.dumps(q,indent=2)+'\n')
    qualify(repo,candidate,inventory)
    output.write_text(candidate.read_text());candidate.unlink()
    return q


if __name__=='__main__':
    import argparse
    import NasaGuiCoverage as nasa
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3])
    parser.add_argument('--run',type=Path,required=True)
    parser.add_argument('--case',choices=('basic','stm','epoch'),required=True)
    parser.add_argument('--review',type=Path,default=Path(__file__).with_name('NasaCapiTraceability.json'))
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();repo=args.repo.resolve()
    q=create_qualification(repo,args.run,args.case,args.review,args.output,nasa.inventory(repo))
    print(json.dumps(dict(status='passed',qualified_keys=q['qualified_keys'],output=str(args.output))))
