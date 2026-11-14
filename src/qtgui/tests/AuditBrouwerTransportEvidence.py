"""Check physical mean-state equivalence with independent frame/prototype math.

The original strict component worker and failed receipt remain separate. This
reader does not relabel an alternate root as the originally requested elements.
"""
from pathlib import Path
import argparse,copy,hashlib,json,math
import BrouwerTransportOracle as oracle

def check(row,forward,matrix):
 mean=oracle.elements(oracle.internal(oracle.cartesian(row['mean'],matrix['mu']),row['theta'],row['tilt'],row['rate']))
 physical=oracle.cartesian(forward[row['kind']](mean),matrix['mu'])
 errors=[abs(a-b)for a,b in zip(physical,row['expected_internal'])]
 bounds=[matrix['position_tolerance_km']if i<3 else matrix['velocity_tolerance_km_s']for i in range(6)]
 passed=all(math.isfinite(e)and e<=bounds[i]+matrix['relative_tolerance']*abs(row['expected_internal'][i])for i,e in enumerate(errors))
 return dict(case=row['case'],phase=row['phase'],precise=row['precise'],returned_earth_mean=mean,reconstructed_internal=physical,expected_internal=row['expected_internal'],errors=errors,strict_field_contract_passed=row['strict_field_contract_passed'],physical_equivalence_passed=passed)

def audit(repo,directory):
 matrix=json.loads((repo/'src/qtgui/tests/BrouwerTransportCases.json').read_text())
 for n,h in matrix['reference_sha256'].items():assert hashlib.sha256((repo/n).read_bytes()).hexdigest()==h,n
 forward={kind:oracle.adapt(repo/'prototype/StateConv'/name)[0]for kind,name in [('Short','BROLYD_short.m'),('Long','BROLYD.m')]}
 proof=json.loads((directory/'proof.json').read_text());rows=json.loads((directory/'mean-observations.json').read_text())
 assert proof['status']=='observed;independent-oracle-pending' and proof['context_cases']==72 and proof['scalar_writes']==432 and proof['complete_property_writes']==72 and proof['numeric_comparisons']==1224
 assert proof['independent_mean_observations']==len(rows)==648
 cases={c['key']:c for c in matrix['cases']};assert len(cases)==36
 expected={(k,p,role)for k in cases for p in [False,True]for role in ['helper-read','spacecraft-read','parameter-read']+['scalar-readback-'+f for f in ['SMA','ECC','INC','RAAN','AOP','MA']]};actual=set()
 results=[]
 for row in rows:
  spec=cases[row['case']];prefix=spec['key']+('-precise-'if row['precise']else'-real-');assert row['phase'].startswith(prefix)
  role=row['phase'][len(prefix):];actual.add((row['case'],row['precise'],role))
  assert all(row[k]==spec[k]for k in ['theta','tilt','rate','kind'])
  physical=spec['earth_osculating'];wanted=spec['local_mean']
  if role.startswith('scalar-readback-'):
   index=['SMA','ECC','INC','RAAN','AOP','MA'].index(role.removeprefix('scalar-readback-'));change=spec['writes'][index]
   physical=change['expected_internal'];wanted=change['local_mean']
  assert row['expected_internal']==physical and row['literal_expected_mean']==wanted
  result=check(row,forward,matrix);results.append(result)
 assert actual==expected and len(actual)==len(rows),'Missing/duplicate mean read route'
 failures=[v for v in results if not v['physical_equivalence_passed']]
 if failures:raise ValueError(json.dumps(failures[:2]))
 corrupt=[]
 for index,mutation in [('changed-mean-SMA',lambda r:r['mean'].__setitem__(0,r['mean'][0]+1)),('lost-rotating-frame-rate',lambda r:r.__setitem__('rate',0))]:
  row=copy.deepcopy(next(r for r in rows if r['rate']!=0));mutation(row)
  assert not check(row,forward,matrix)['physical_equivalence_passed'],'Accepted corrupted observation '+index
  corrupt.append(index)
 return dict(status='passed',mean_reads_checked=648,cartesian_write_assertions=576,contexts=72,scalar_writes=432,complete_property_writes=72,strict_field_disagreements_retained=sum(not v['strict_field_contract_passed']for v in results),max_position_error_km=max(max(v['errors'][:3])for v in results),max_velocity_error_km_s=max(max(v['errors'][3:])for v in results),numerical_bounds_unchanged=True,corrupt_observation_controls_rejected=corrupt,original_strict_contract_status='failed;retained-separately',results=results,new_requirement_credit=0,full_current_build_qualified=False,independent_physical_averaging_qualified=False)
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,required=True);p.add_argument('--directory',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 result=audit(a.repo.resolve(),a.directory.resolve())
 with a.output.open('x')as f:json.dump(result,f,indent=2);f.write('\n')
 print(json.dumps({k:v for k,v in result.items()if k!='results'}))
