"""Independent local conic definitions; no GMAT conversion or frame API.

Kepler geometry is reused from the retained analytic boundary oracle. The
Equinoctial inverse solves Kepler's equation independently from GUI/native code.
"""
import math
from KeplerBoundaryOracle import cartesian as conic_cartesian
TYPES=('Keplerian','ModifiedKeplerian','Equinoctial')
def define(kind,kep):
 a,e,i,node,peri,nu=kep
 if kind=='Keplerian':return list(kep)
 if kind=='ModifiedKeplerian':return [a*(1-e),a*(1+e),i,node,peri,nu]
 if kind!='Equinoctial' or not(0<=e<1 and a>0):raise ValueError('Equinoctial requires an elliptic state')
 n,w,v=map(math.radians,[node,peri,nu]);E=math.atan2(math.sqrt(1-e*e)*math.sin(v),e+math.cos(v));M=E-e*math.sin(E)
 return [a,e*math.sin(n+w),e*math.cos(n+w),math.tan(math.radians(i)/2)*math.sin(n),math.tan(math.radians(i)/2)*math.cos(n),math.degrees(n+w+M)%360]
def cartesian(kind,values,mu):
 if kind=='Keplerian':kep=list(values)
 elif kind=='ModifiedKeplerian':
  rp,ra,i,n,w,nu=values;a=(rp+ra)/2;e=(ra-rp)/(rp+ra);kep=[a,e,i,n,w,nu]
 elif kind=='Equinoctial':
  a,h,k,p,q,longitude=values;e=math.hypot(h,k);i=2*math.atan(math.hypot(p,q));n=math.atan2(p,q);varpi=math.atan2(h,k);M=math.remainder(math.radians(longitude)-varpi,2*math.pi);E=M
  for _ in range(32):
   delta=(E-e*math.sin(E)-M)/(1-e*math.cos(E));E-=delta
   if abs(delta)<1e-15:break
  else:raise ValueError('Independent Kepler equation did not converge')
  nu=math.atan2(math.sqrt(1-e*e)*math.sin(E),math.cos(E)-e);kep=[a,e,math.degrees(i),math.degrees(n),math.degrees(varpi-n),math.degrees(nu)]
 else:raise ValueError('Unknown conic representation')
 return conic_cartesian(kep,mu)
