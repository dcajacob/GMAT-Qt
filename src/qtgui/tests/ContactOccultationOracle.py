"""Independent finite-segment occultation roots for a lunar circular orbit.

CSPICE reads ephemeris, shape, orientation and time inputs only. Analytic
point-mass motion, geodetic algebra and finite-segment distances determine
visibility; no GMAT library, GF or occultation classifier supplies truth.
"""
import argparse
import ctypes as C
import functools
import hashlib
import json
import math
from pathlib import Path
import ContactGeometryOracle as contact

common = contact.common
DURATION = 24000
MU = 4902.8
RADIUS = 2500.


def segment_clearance(observer, target, center, radius):
    """Signed distance from a closed finite segment to a spherical surface."""
    line = [b-a for a,b in zip(observer,target)]
    offset = [c-a for a,c in zip(observer,center)]
    fraction = max(0., min(1., common.dot(offset,line)/common.dot(line,line)))
    return common.norm([a+fraction*d-c for a,d,c in zip(observer,line,center)])-radius


def geometry_controls():
    # Behind observer and beyond target must not act like an infinite ray.
    cases = [([-2.,0.,0.],[2.,0.,0.],[0.,0.,0.],1.,-1.),
             ([-2.,1.,0.],[2.,1.,0.],[0.,0.,0.],1.,0.),
             ([2.,0.,0.],[3.,0.,0.],[0.,0.,0.],1.,1.),
             ([-3.,0.,0.],[-2.,0.,0.],[0.,0.,0.],1.,1.),
             ([-2.,2.,0.],[2.,2.,0.],[0.,0.,0.],1.,1.)]
    for observer,target,center,radius,wanted in cases:
        assert abs(segment_clearance(observer,target,center,radius)-wanted)<1e-14
        assert abs(segment_clearance(target,observer,center,radius)-wanted)<1e-14
    return len(cases)*2


class LunarCircle:
    def __init__(self, data):
        self.data = data
        self.rate = math.sqrt(MU/RADIUS**3)
        self.moon = data.radii('MOON')
        assert max(self.moon)-min(self.moon)<1e-12
        self.mars = max(data.radii('MARS'))

    def state(self, time, phase=0.):
        angle = self.rate*time+phase
        return [RADIUS*math.cos(angle),RADIUS*math.sin(angle),0.,
                -RADIUS*self.rate*math.sin(angle),RADIUS*self.rate*math.cos(angle),0.]

    @functools.lru_cache(maxsize=None)
    def body(self, name, time):
        result,lt = (C.c_double*6)(),C.c_double()
        self.data.lib.spkezr_c(name.encode(),self.data.et(time),b'J2000',b'NONE',b'EARTH',result,C.byref(lt))
        self.data.check()
        return list(result)[:3]

    def target(self, time):
        return [a+b for a,b in zip(self.body('MOON',time),self.state(time)[:3])]

    def fixed(self, time):
        return [common.dot(row,self.target(time)) for row in self.data.rotation(time)]

    def observer(self, time, station):
        rotation = self.data.rotation(time)
        return [sum(rotation[j][i]*station['fixed_position'][j]for j in range(3))for i in range(3)]

    def elevation(self, time, station):
        line = [x-y for x,y in zip(self.fixed(time),station['fixed_position'])]
        return common.dot(common.unit(line),station['up'])-math.sin(math.radians(station['minimum_elevation']))

    def clearance(self, time, station, body):
        return segment_clearance(self.observer(time,station),self.target(time),
                                 self.body('MOON'if body=='Luna'else'MARS',time),
                                 self.moon[0]if body=='Luna'else self.mars)

    def visible(self, time, station, bodies):
        return self.elevation(time,station)>0 and all(self.clearance(time,station,b)>0 for b in bodies)

    def intervals(self, station, bodies):
        grid = list(range(0,DURATION+1,10))
        roots = [0.,float(DURATION)]
        functions = [lambda t:self.elevation(t,station)]
        functions += [lambda t,b=b:self.clearance(t,station,b)for b in bodies]
        for function in functions:
            for left,right in zip(grid,grid[1:]):
                a,b=float(left),float(right);fa,fb=function(a),function(b)
                if fa*fb>=0:continue
                while b-a>1e-6:
                    middle=(a+b)/2;fm=function(middle)
                    if fa*fm>0:a,fa=middle,fm
                    else:b=middle
                root=(a+b)/2
                assert function(root-.01)*function(root+.01)<0
                roots.append(root)
        roots.sort()
        return [dict(start=a,end=b,duration=b-a)for a,b in zip(roots,roots[1:])
                if self.visible((a+b)/2,station,bodies)]


def fixtures(library,root):
    controls=geometry_controls();data=common.Data(library,root);orbit=LunarCircle(data)
    r=orbit.fixed(DURATION/2);latitude=math.degrees(math.asin(r[2]/common.norm(r)))
    longitude=math.degrees(math.atan2(r[1],r[0]))%360
    stations=[contact.make_station('North',latitude,longitude,0.,0.,data.earth),
              contact.make_station('Dark',-latitude,(longitude+180)%360,0.,0.,data.earth),
              contact.make_station('Unused',-80.,80.,1.,15.,data.earth)]
    cases=[]
    for key,bodies in [('None',[]),('Luna',['Luna']),('Mars',['Mars']),('Both',['Luna','Mars'])]:
        events={s['name']:orbit.intervals(s,bodies)for s in stations[:2]}
        assert not events['Dark']
        cases.append(dict(key=key,target='Sat',observers=['North','Dark'],occulting_bodies=bodies,
                          events=events,initial_state=orbit.state(0),other_state=orbit.state(0,math.pi/2),
                          final_state=orbit.state(DURATION),mu=common.MU,orbit_mu=MU,duration=DURATION,
                          start=0.,end=DURATION,bounded=False,write_report=True,
                          time_tolerance=.002,duration_tolerance=.002))
    assert cases[0]['events']==cases[2]['events']
    assert cases[1]['events']==cases[3]['events']
    assert cases[0]['events']['North']==[dict(start=0.,end=float(DURATION),duration=float(DURATION))]
    assert len(cases[1]['events']['North'])>=2
    removed=DURATION-sum(e['duration']for e in cases[1]['events']['North'])
    assert removed>1000
    mars_min=min(orbit.clearance(t,stations[0],'Mars')for t in range(0,DURATION+1,10))
    assert mars_min>1e7
    old=json.loads((root/'src/qtgui/tests/NasaContactResultCases.json').read_text())
    physical=old['physical_inputs'].copy();physical['kernels']=physical['kernels'].copy()
    for name in ['SPICELunaCurrentKernel.bpc','SPICELunaFrameKernel.tf']:
        path='application/data/planetary_coeff/'+name
        physical['kernels'][path]=hashlib.sha256((root/path).read_bytes()).hexdigest()
    for p,h in physical['kernels'].items():assert hashlib.sha256((root/p).read_bytes()).hexdigest()==h
    physical.update(moon_radii=orbit.moon,mars_outer_radius=orbit.mars,orbit_mu=MU,
                    orbit_radius=RADIUS,orbit_frame='MoonIcrf',ephemeris_source='SPICE',
                    ephemeris_update_interval=0)
    return dict(cases=cases,stations=stations,physical_inputs=physical,
                equatorial_radius=data.earth[0],flattening=(data.earth[0]-data.earth[2])/data.earth[0],
                oracle_controls=dict(analytic_segment_checks=controls,removed_contact_seconds=removed,
                                     sampled_mars_clearance_min_km=mars_min),
                scope='One prescribed 24000s Moon-centered ICRF point-mass circle, Earth ellipsoid stations, '
                'geometric DE421 translations, finite-segment spherical lunar occultation and distant Mars '
                'outer-sphere exclusion. None/Mars and Luna/Luna+Mars compare selected-body suppression '
                'and unselected-body isolation. Shared hashed SPICE inputs only; no GMAT/GF/occultation truth. '
                'No light-time/stellar, arbitrary ellipsoids/user bodies/trajectories, terrain/FOV, '
                'rise-only/set-only selection, native OS input/hardware/platform/stress/current-private NASA '
                'or full newest-build qualification.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--library',required=True)
    args=parser.parse_args();root=Path(__file__).resolve().parents[3]
    value=fixtures(args.library,root)
    Path(__file__).with_name('NasaContactOccultationCases.json').write_text(json.dumps(value,indent=2)+'\n')
    print(json.dumps(dict(events={c['key']:c['events']for c in value['cases']},controls=value['oracle_controls']),indent=2))
