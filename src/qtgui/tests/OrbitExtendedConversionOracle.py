"""Add Delaunay/modified/alternate equinoctial references to the retained oracle.

Definitions follow the shipped NASA orbit-state guide; no GMAT conversion API.
Only missing directed edges are generated. The original 30 edges are retained.
"""
import math
import OrbitConversionOracle as original

EXTRA = ('ModifiedEquinoctial', 'AlternateEquinoctial', 'Delaunay')
ALL = original.REPRESENTATIONS + EXTRA
METADATA = {
    'ModifiedEquinoctial': dict(names=['SemilatusRectum','ModEquinoctialF','ModEquinoctialG',
                                     'ModEquinoctialH','ModEquinoctialK','TLONG'],
                               units=['km','','','','','deg']),
    'AlternateEquinoctial': dict(names=['SMA','EquinoctialH','EquinoctialK','AltEquinoctialP',
                                      'AltEquinoctialQ','MLONG'],units=['km','','','','','deg']),
    'Delaunay': dict(names=['Delaunayl','Delaunayg','Delaunayh','DelaunayL','DelaunayG','DelaunayH'],
                     units=['deg','deg','deg','km^2/sec','km^2/sec','km^2/sec'])}


def representations(elements, mu):
    states = original.representations(elements, mu)
    a,e,inc,node,peri,nu = elements
    i,om,w = map(math.radians,[inc,node,peri])
    eq = states['Equinoctial']
    mean = (eq[-1]-node-peri)%360
    momentum = math.sqrt(mu*a)
    angular_momentum = momentum*math.sqrt(1-e*e)
    states.update(
        ModifiedEquinoctial=[a*(1-e*e),e*math.cos(om+w),e*math.sin(om+w),
                            math.tan(i/2)*math.cos(om),math.tan(i/2)*math.sin(om),
                            (node+peri+nu)%360],
        AlternateEquinoctial=[a,eq[1],eq[2],math.sin(i/2)*math.sin(om),
                             math.sin(i/2)*math.cos(om),eq[5]],
        Delaunay=[mean,peri,node,momentum,angular_momentum,angular_momentum*math.cos(i)])
    return states


def fixture(definitions):
    metadata = {c['key']:dict(names=c['names'],units=c['units']) for c in definitions['cases']}
    metadata.update(METADATA)
    initial = [(c['key'],c['canonical_keplerian'],c['mu']) for c in definitions['cases']]
    initial += [('ModifiedEquinoctial',[10800,.17,47,112,38,143],398600.4418),
                ('AlternateEquinoctial',[11300,.29,68,152,17,209],398600.4418),
                ('Delaunay',[12200,.36,105,210,51,312],398600.4418)]
    cases=[]
    for key,kep,mu in initial:
        states=representations(kep,mu)
        targets=EXTRA if key in original.REPRESENTATIONS else tuple(t for t in ALL if t!=key)
        cases.append(dict(key=key,mu=mu,keplerian=kep,cartesian=states['Cartesian'],
                          expected_pairs=len(targets),targets=[dict(key=t,values=states[t],**metadata[t])
                                                            for t in targets]))
    return dict(schema_version=1,scope='42 new directed GUI pairs complete the nine-representation elliptic EarthMJ2000Eq matrix with30retained pairs;other five native representations,frame/origin/anomaly/singular/hyperbolic/all-domain requirements remain open',
                position_tolerance_km=1e-6,velocity_tolerance_km_s=1e-9,
                displayed_field_tolerance=1e-6,cases=cases)


if __name__=='__main__':
    import argparse,json
    from pathlib import Path
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--definitions',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    args=p.parse_args();args.output.write_text(json.dumps(fixture(json.loads(args.definitions.read_text())),indent=2)+'\n')
