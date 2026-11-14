"""Execute retained NASA scalar MATLAB formulas through a restricted adapter.

This is algorithm/source parity evidence, not independent averaging theory or
FreeFlyer certification. Preserve every original assignment and branch; ignore
only display statements and the one unassigned diagnostic square root.
"""
from pathlib import Path
import ast,hashlib,json,math,re
ROOT=Path(__file__).resolve().parents[3]
def degree(values,factor):return [v*factor for v in values]if isinstance(values,list)else values*factor
def at(values,index):return values[index-1]if isinstance(values,list)else values
def eccentric(mean,ecc):
 mean=mean%(2*math.pi);value=mean
 for _ in range(100):
  updated=value-(value-ecc*math.sin(value)-mean)/(1-ecc*math.cos(value))
  if updated==value:break
  value=updated
 assert abs(value-ecc*math.sin(value)-mean)<1e-13
 return value%(2*math.pi)
def adapt(path):
 raw=path.read_text();input_name='elep'if path.stem.endswith('_short')else'dpele';statements=[];pending=''
 for number,line in enumerate(raw.splitlines(),1):
  line=line.split('%',1)[0].strip()
  if not line or line.startswith('function '):continue
  if line.endswith('...'):pending+=line[:-3];continue
  line=pending+line;pending='';statements.append((number,line))
 assert not pending
 def expression(value):
  value=value.replace('~=','!=').replace('^','**').replace('&',' and ').replace('|',' or ')
  for name in ['elep','dpele','oscele']:
   value=re.sub(r'\b'+name+r'\((\d+):(\d+)\)',lambda m:name+'['+str(int(m[1])-1)+':'+m[2]+']',value)
  value=re.sub(r'\b(elep|dpele|oscele|ae)\((\d+)\)',r'at(\1,\2)',value)
  if value.startswith('[')and value.endswith(']')and value!='[]':value='['+','.join(value[1:-1].split())+']'
  return value
 code=['def convert('+input_name+'):', '    '+input_name+'=list('+input_name+')','    oscele=[0.]*6'];indent=1;anchors=[]
 for number,line in statements:
  line=line.rstrip(';')
  if line=='end':indent-=1;assert indent>=1;continue
  if line=='else':indent-=1;code.append('    '*indent+'else:');indent+=1;continue
  if line.startswith('if '):code.append('    '*indent+'if '+expression(line[3:])+':');indent+=1;continue
  if line.startswith('disp(')or line.startswith('sqrt('):code.append('    '*indent+'pass');continue
  if line=='return':code.append('    '*indent+'return oscele');continue
  assert '='in line,(path,number,line)
  left,right=line.split('=',1);left=left.strip();left=re.sub(r'\b(elep|dpele|oscele)\((\d+):(\d+)\)',lambda m:m[1]+'['+str(int(m[2])-1)+':'+m[3]+']',left);left=re.sub(r'\b(elep|dpele|oscele)\((\d+)\)',lambda m:m[1]+'['+str(int(m[2])-1)+']',left)
  assignment=left+'='+expression(right.strip());code.append('    '*indent+assignment);anchors.append(dict(source_line=number,original=line,adapted=assignment))
 assert indent==1
 code.append('    return oscele');source='\n'.join(code)+'\n';tree=ast.parse(source)
 allowed={'sin','cos','tan','asin','acos','atan','atan2','sqrt','abs','at','list','deg2rad','rad2deg','DKEPLR','atan0','mod'}
 for node in ast.walk(tree):
  assert not isinstance(node,(ast.Import,ast.ImportFrom,ast.Attribute,ast.While,ast.For,ast.With,ast.Try,ast.Lambda)),type(node).__name__
  if isinstance(node,ast.Call):assert isinstance(node.func,ast.Name)and node.func.id in allowed,node.func
 namespace={k:getattr(math,k)for k in allowed if hasattr(math,k)};namespace.update(pi=math.pi,at=at,list=list,abs=abs,deg2rad=lambda v:degree(v,math.pi/180),rad2deg=lambda v:degree(v,180/math.pi),DKEPLR=eccentric,atan0=lambda y,x:math.atan2(y,x)%(2*math.pi),mod=lambda a,b:a%b,__builtins__={})
 exec(compile(tree,str(path),'exec'),namespace)
 return namespace['convert'],source,anchors
def cartesian(elements,mu):
 a,e,inc,node,peri,mean=elements;i,om,w=map(math.radians,[inc,node,peri]);ea=eccentric(math.radians(mean),e);nu=math.atan2(math.sqrt(1-e*e)*math.sin(ea),math.cos(ea)-e);p=a*(1-e*e);r=p/(1+e*math.cos(nu));scale=math.sqrt(mu/p)
 P=[math.cos(om)*math.cos(w)-math.sin(om)*math.sin(w)*math.cos(i),math.sin(om)*math.cos(w)+math.cos(om)*math.sin(w)*math.cos(i),math.sin(w)*math.sin(i)];Q=[-math.cos(om)*math.sin(w)-math.sin(om)*math.cos(w)*math.cos(i),-math.sin(om)*math.sin(w)+math.cos(om)*math.cos(w)*math.cos(i),math.cos(w)*math.sin(i)]
 return [r*(P[k]*math.cos(nu)+Q[k]*math.sin(nu))for k in range(3)]+[scale*(-P[k]*math.sin(nu)+Q[k]*(e+math.cos(nu)))for k in range(3)]

def generate():
 """Keep all historical references, including two invalid all-zero mean files."""
 functions={rep:adapt(ROOT/'prototype/StateConv'/(name+'.m'))[0]
            for rep,name in [('BrouwerMeanShort','BROLYD_short'),('BrouwerMeanLong','BROLYD')]}
 samples=[]
 truth_root=ROOT/'prototype/StateConv/test/truth/BrouwerLyddane/FF'
 for path in sorted(truth_root.glob('*.truth')):
  numeric=[]
  for line in path.read_text().splitlines():
   try:values=[float(v)for v in line.split()]
   except ValueError:continue
   if len(values)==6:numeric.append(values)
  assert len(numeric)==2
  case,suffix=path.stem.rsplit('_',1);rep='BrouwerMean'+suffix
  for mu in [398600.4415,398600.4418]:
   record=dict(case=case,representation=rep,mu=mu,mean=numeric[1],
               historical_cartesian=numeric[0],truth=str(path.relative_to(ROOT)),
               truth_sha256=hashlib.sha256(path.read_bytes()).hexdigest())
   if numeric[1]==[0.]*6:
    record.update(invalid_zero_reference=True,expected_exception='RadPer')
   else:
    osc=functions[rep](numeric[1]);record.update(osculating_keplerian_MA=osc,expected_cartesian=cartesian(osc,mu))
   samples.append(record)
 assert len(samples)==60 and sum(r.get('invalid_zero_reference',False)for r in samples)==4
 source_names=['prototype/StateConv/BROLYD.m','prototype/StateConv/BROLYD_short.m',
               'src/qtgui/tests/BrouwerPrototypeOracle.py','doc/help/src/Resource_SpacecraftOrbitState.xml']
 provenance={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in source_names}
 common=dict(schema_version=1,reference_sha256=provenance,
             oracle_scope='Retained NASA prototype source parity plus independently solved Kepler equation/Cartesian basis. Independent physical averaging and historical FreeFlyer agreement remain unqualified.')
 corpus=dict(**common,position_tolerance_km=1e-6,velocity_tolerance_km_s=1e-9,samples=samples)
 cases=[];mean=[10800,.17,47,112,38,143];mu=398600.4418
 for rep,prefix in [('BrouwerMeanShort','BrouwerShort'),('BrouwerMeanLong','BrouwerLong')]:
  osc=functions[rep](mean);ea=eccentric(math.radians(osc[5]),osc[1])
  ta=math.degrees(math.atan2(math.sqrt(1-osc[1]*osc[1])*math.sin(ea),math.cos(ea)-osc[1]))%360
  cases.append(dict(key=rep,canonical_keplerian=osc[:5]+[ta],osculating_keplerian_MA=osc,mu=mu,
                    names=[prefix+x for x in ['SMA','ECC','INC','RAAN','AOP','MA']],values=mean,
                    units=['km','','deg','deg','deg','deg'],invalid_field=prefix+'SMA',invalid=['nan','0','-1'],
                    expected_cartesian=cartesian(osc,mu),position_tolerance_km=1e-6,velocity_tolerance_km_s=1e-9))
 driver='src/qtgui/tests/NasaOrbitDefinitionTests.cpp'
 gui=dict(**common,driver=driver,driver_sha256=hashlib.sha256((ROOT/driver).read_bytes()).hexdigest(),
          scope='Two actual six-field mean definitions,one new nonsingular EarthMJ2000Eq orbit;full singular/inverse/reference science remains separate',cases=cases)
 return corpus,gui

if __name__=='__main__':
 import argparse
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument('--corpus-output',type=Path,required=True)
 parser.add_argument('--gui-output',type=Path,required=True)
 args=parser.parse_args();corpus,gui=generate()
 for path,value in [(args.corpus_output,corpus),(args.gui_output,gui)]:
  path.write_text(json.dumps(value,indent=2)+'\n')
