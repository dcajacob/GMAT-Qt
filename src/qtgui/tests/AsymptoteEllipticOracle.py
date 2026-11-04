"""Independent declared apoapsis-vector extension for negative-C3 states.

This is GMAT's elliptic field extension, not a physical hyperbolic asymptote.
No native conversion API computes the reference. Cross-tool/NASA convention
reconciliation and exact threshold endpoints remain open.
"""
import math
import OrbitConversionOracle as original


def representations(elements,mu):
    states=original.representations(elements,mu)
    a,e,inc,node,peri,ta=elements
    i,om,w=map(math.radians,[inc,node,peri])
    p=[math.cos(om)*math.cos(w)-math.sin(om)*math.sin(w)*math.cos(i),
       math.sin(om)*math.cos(w)+math.cos(om)*math.sin(w)*math.cos(i),math.sin(w)*math.sin(i)]
    normal=[math.sin(om)*math.sin(i),-math.cos(om)*math.sin(i),math.cos(i)]
    ray=[-x for x in p];right=math.atan2(ray[1],ray[0]);decl=math.asin(ray[2])
    east=[-math.sin(right),math.cos(right),0.]
    north=[-math.sin(decl)*math.cos(right),-math.sin(decl)*math.sin(right),math.cos(decl)]
    dot=lambda x,y:sum(a*b for a,b in zip(x,y))
    bva=math.degrees(math.atan2(dot(normal,north),dot(normal,east)))%360
    values=[a*(1-e),-mu/a,math.degrees(right)%360,math.degrees(decl),bva,ta%360]
    states.update(IncomingAsymptote=values[:],OutgoingAsymptote=values[:])
    return states


def fixture():
    cases=[]
    for key,kep in [('IncomingAsymptote',[13500,.32,48,73,62,241]),
                    ('OutgoingAsymptote',[16800,.41,112,224,19,73])]:
        mu=398600.4418;states=representations(kep,mu);prefix=key.removesuffix('Asymptote')
        names=[prefix+'RadPer',prefix+'C3Energy',prefix+'RHA',prefix+'DHA',prefix+'BVAZI','TA']
        extras=[dict(field=names[1],value=v,category='parabolic',diagnostic_any=['parabolic','C3Energy'])
                for v in ['0','5e-8','-5e-8']]
        extras += [dict(field=names[1],value=format(-mu/states[key][0],'.17g'),category='circular',diagnostic_any=['circular','C3Energy'])]
        extras += [dict(field=names[3],value=v,category='polar',diagnostic_any=['z-direction','DHA']) for v in ['90','-90']]
        cases.append(dict(key=key,canonical_keplerian=kep,mu=mu,names=names,values=states[key],
            units=['km','km^2/sec^2','deg','deg','deg','deg'],invalid_field=names[1],
            invalid=['nan','inf','not-a-number'],extra_invalid=extras,
            expected_cartesian=states['Cartesian'],position_tolerance_km=1e-6,velocity_tolerance_km_s=1e-9))
    return dict(schema_version=1,scope='Two negative-C3 elliptic apoapsis-extension six-field inputs;nonfinite,strictly-inside near-parabolic,circular and exact-pole rejection/recovery under declared GMAT convention. Exact C3/eccentricity threshold endpoints,near-pole limits,other frames and general domains remain open.',cases=cases)


if __name__=='__main__':
    import argparse,json
    from pathlib import Path
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
    args=p.parse_args();args.output.write_text(json.dumps(fixture(),indent=2)+'\n')
