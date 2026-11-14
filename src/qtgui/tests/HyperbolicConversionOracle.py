"""Independent hyperbolic geometry and GMAT asymptote position-ray convention.

P/Q perifocal geometry, limiting true anomaly, and local sky axes are evaluated
directly. No GMAT conversion routine is called. Incoming position ray is the
negative of the incoming velocity ray used by B-plane S; that convention is
recorded as an interoperability/source-intent gap, not silently equated.
"""
import math
from OrbitExtendedConversionOracle import METADATA as EXTENDED_METADATA

ALL=('Cartesian','Keplerian','ModifiedKeplerian','SphericalAZFPA','SphericalRADEC',
     'ModifiedEquinoctial','IncomingAsymptote','OutgoingAsymptote')


def geometry(elements,mu):
    a,e,inc,node,peri,ta=elements
    if not(a<0 and e>1 and 0<inc<180 and mu>0):
        raise ValueError('Reference requires an inclined nondegenerate hyperbola')
    i,om,w,nu=map(math.radians,[inc,node,peri,ta])
    if 1+e*math.cos(nu)<=0:
        raise ValueError('True anomaly lies beyond the physical hyperbolic branch')
    cn,sn,ci,si,cw,sw=math.cos(om),math.sin(om),math.cos(i),math.sin(i),math.cos(w),math.sin(w)
    p_axis=[cn*cw-sn*sw*ci,sn*cw+cn*sw*ci,sw*si]
    q_axis=[-cn*sw-sn*cw*ci,-sn*sw+cn*cw*ci,cw*si]
    normal=[sn*si,-cn*si,ci]
    p=a*(1-e*e);radius=p/(1+e*math.cos(nu));scale=math.sqrt(mu/p)
    position=[radius*(p_axis[k]*math.cos(nu)+q_axis[k]*math.sin(nu)) for k in range(3)]
    velocity=[scale*(-p_axis[k]*math.sin(nu)+q_axis[k]*(e+math.cos(nu))) for k in range(3)]
    return p_axis,q_axis,normal,position,velocity


def representations(elements,mu):
    a,e,inc,node,peri,ta=elements
    p_axis,q_axis,normal,r,v=geometry(elements,mu)
    radius=math.sqrt(sum(x*x for x in r));speed=math.sqrt(sum(x*x for x in v))
    ra=math.atan2(r[1],r[0]);dec=math.asin(r[2]/radius)
    north=[-math.sin(dec)*math.cos(ra),-math.sin(dec)*math.sin(ra),math.cos(dec)]
    east=[-math.sin(ra),math.cos(ra),0.]
    dot=lambda x,y:sum(a*b for a,b in zip(x,y))
    az=math.degrees(math.atan2(dot(v,east),dot(v,north)))%360
    fpa=math.degrees(math.acos(dot(r,v)/(radius*speed)))
    i,om,w=map(math.radians,[inc,node,peri])
    states=dict(Cartesian=r+v,Keplerian=[a,e,inc,node,peri,ta%360],
        ModifiedKeplerian=[a*(1-e),a*(1+e),inc,node,peri,ta%360],
        SphericalAZFPA=[radius,math.degrees(ra)%360,math.degrees(dec),speed,az,fpa],
        SphericalRADEC=[radius,math.degrees(ra)%360,math.degrees(dec),speed,
                        math.degrees(math.atan2(v[1],v[0]))%360,math.degrees(math.asin(v[2]/speed))],
        ModifiedEquinoctial=[a*(1-e*e),e*math.cos(om+w),e*math.sin(om+w),
            math.tan(i/2)*math.cos(om),math.tan(i/2)*math.sin(om),(node+peri+ta)%360])
    limit=math.acos(-1/e)
    for name,sign in [('IncomingAsymptote',-1),('OutgoingAsymptote',1)]:
        ray=[p_axis[k]*math.cos(limit)+sign*q_axis[k]*math.sin(limit) for k in range(3)]
        right=math.atan2(ray[1],ray[0]);decl=math.asin(ray[2])
        sky_east=[-math.sin(right),math.cos(right),0.]
        sky_north=[-math.sin(decl)*math.cos(right),-math.sin(decl)*math.sin(right),math.cos(decl)]
        # Orbit-plane orientation is equivalent to the azimuth of h cross ray
        # counterclockwise from south; evaluated using normal projections.
        bva=math.degrees(math.atan2(dot(normal,sky_north),dot(normal,sky_east)))%360
        states[name]=[a*(1-e),-mu/a,math.degrees(right)%360,math.degrees(decl),bva,ta%360]
    return states


def fixture(definitions):
    metadata={c['key']:dict(names=c['names'],units=c['units']) for c in definitions['cases']}
    metadata['ModifiedEquinoctial']=EXTENDED_METADATA['ModifiedEquinoctial']
    for key in ['Incoming','Outgoing']:
        metadata[key+'Asymptote']=dict(names=[key+'RadPer',key+'C3Energy',key+'RHA',
            key+'DHA',key+'BVAZI','TA'],units=['km','km^2/sec^2','deg','deg','deg','deg'])
    orbits=[[-12000,1.6,30,40,25,70],[-18000,1.4,45,60,40,-80],
            [-22000,1.8,60,90,45,100],[-14000,1.5,35,120,65,-65],
            [-16000,1.3,55,150,85,80],[-24000,2.,110,210,35,-90],
            [-20000,1.7,75,240,55,-60],[-26000,2.2,125,290,110,75]]
    cases=[]
    for key,kep in zip(ALL,orbits):
        mu=398600.4418;states=representations(kep,mu)
        cases.append(dict(key=key,mu=mu,keplerian=kep,cartesian=states['Cartesian'],expected_pairs=7,
            targets=[dict(key=t,values=states[t],**metadata[t]) for t in ALL if t!=key]))
    return dict(schema_version=1,domain='hyperbolic',frame='EarthMJ2000Eq',
        asymptote_convention='Limiting position rays;incoming differs from incoming velocity/B-plane S by180degrees. NASA/interoperability convention reconciliation remains open.',
        scope='56directed pairs among eight representations,eight inclined hyperbolic source orbits,EarthMJ2000Eq and TA conics only. No elliptic-negative-C3,singular,near-parabolic,other-frame,all-representation or convention-parity completion credit.',
        position_tolerance_km=1e-6,velocity_tolerance_km_s=1e-9,displayed_field_tolerance=1e-6,cases=cases)


if __name__=='__main__':
    import argparse,json
    from pathlib import Path
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--definitions',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    args=p.parse_args();args.output.write_text(json.dumps(fixture(json.loads(args.definitions.read_text())),indent=2)+'\n')
