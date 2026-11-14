"""Independent elevation thresholds and contact time roots for circular orbits.

SPICE supplies only Earth orientation and time/frame/shape input data. No
native station converter, contact finder, GF routine or report is imported.
Ellipsoid geodetic positions and normals use explicit algebra; the target is
a closed-form point-mass circle; bisection solves rise/set elevations.
"""
import argparse
import functools
import hashlib
import json
import math
from pathlib import Path
import EclipseGeometryOracle as common

DURATION = 6500


class Circle:
    def __init__(self, data):
        self.data = data
        self.radius = 7000.
        self.rate = math.sqrt(common.MU/self.radius**3)

    def state(self, time, phase=0.):
        angle = self.rate*time+phase
        c, s = math.cos(math.pi/4), math.sin(math.pi/4)
        x, y, v = self.radius*math.cos(angle), self.radius*math.sin(angle), self.radius*self.rate
        return [x,y*c,y*s,-v*math.sin(angle),v*math.cos(angle)*c,v*math.cos(angle)*s]

    @functools.lru_cache(maxsize=None)
    def fixed(self, time):
        r = self.data.fk5_to_icrf(time, self.state(time)[:3])
        return [common.dot(row,r) for row in self.data.rotation(time)]

    def station(self, name, time, height, threshold):
        r = self.fixed(time)
        lat, lon = math.asin(r[2]/common.norm(r)), math.atan2(r[1],r[0])
        return make_station(name,math.degrees(lat),math.degrees(lon)%360,height,threshold,self.data.earth)

    def margin(self, time, station):
        line = [x-y for x,y in zip(self.fixed(time),station['fixed_position'])]
        return common.dot(common.unit(line),station['up'])-math.sin(math.radians(station['minimum_elevation']))

    def intervals(self, station, start=0., end=DURATION):
        grid = [start]+[float(t) for t in range(math.ceil(start)+1,math.floor(end),10)]+[float(end)]
        roots = [start,end]
        for a,b in zip(grid,grid[1:]):
            fa,fb = self.margin(a,station),self.margin(b,station)
            if fa*fb >= 0:continue
            while b-a>1e-6:
                mid=(a+b)/2;fm=self.margin(mid,station)
                if fa*fm>0:a,fa=mid,fm
                else:b=mid
            roots.append((a+b)/2)
        roots.sort()
        return [dict(start=a,end=b,duration=b-a) for a,b in zip(roots,roots[1:])
                if self.margin((a+b)/2,station)>0]


def make_station(name,latitude,longitude,height,threshold,radii):
    a,c=radii[0],radii[2];assert radii[0]==radii[1]
    lat,lon=math.radians(latitude),math.radians(longitude)
    e2=1-(c/a)**2;n=a/math.sqrt(1-e2*math.sin(lat)**2)
    up=[math.cos(lat)*math.cos(lon),math.cos(lat)*math.sin(lon),math.sin(lat)]
    position=[(n+height)*up[0],(n+height)*up[1],(n*(1-e2)+height)*up[2]]
    return dict(name=name,latitude=latitude,longitude=longitude,height=height,
                minimum_elevation=threshold,fixed_position=position,up=up)


def fixtures(library,root):
    data=common.Data(library,root);orbit=Circle(data)
    stations=[orbit.station('North',1000.,0.,0.),orbit.station('East',3000.,1.,15.),
              make_station('Dark',-80.,80.,0.,0.,data.earth)]
    all_events={s['name']:orbit.intervals(s)for s in stations}
    assert all_events['North']and all_events['East']and not all_events['Dark']
    cases=[]
    for key,observers,start,end in [('MultiStation',['North','East','Dark'],0.,6500.),
                                    ('SingleStation',['East'],0.,6500.),
                                    ('Bounded',['North','East','Dark'],800.,3200.)]:
        events={s['name']:orbit.intervals(s,start,end)for s in stations if s['name']in observers}
        assert set(events)==set(observers)
        if key=='Bounded':assert events['North'][0]['start']==start and events['East'][-1]['end']==end
        for station in stations:
            for event in events.get(station['name'],[]):
                for endpoint in ['start','end']:
                    t=event[endpoint]
                    if t not in [start,end]:
                        assert abs(orbit.margin(t,station))<1e-8
                        assert orbit.margin(t-.01,station)*orbit.margin(t+.01,station)<0
        cases.append(dict(key=key,target='Sat',observers=observers,events=events,
                          initial_state=orbit.state(0),other_state=orbit.state(0,math.pi/2),
                          final_state=orbit.state(DURATION),mu=common.MU,duration=DURATION,
                          start=start,end=end,bounded=key=='Bounded',write_report=True,
                          time_tolerance=.002,duration_tolerance=.002))
    old=json.loads((root/'src/qtgui/tests/NasaEclipseResultCases.json').read_text())
    physical=old['physical_inputs'];assert physical['tt0']==data.tt0 and physical['earth_radii']==data.earth
    for path,digest in physical['kernels'].items():assert hashlib.sha256((root/path).read_bytes()).hexdigest()==digest
    return dict(cases=cases,stations=stations,physical_inputs=physical,
                equatorial_radius=data.earth[0],flattening=(data.earth[0]-data.earth[2])/data.earth[0],
                scope='Independent ellipsoid geodetic station algebra, normal-vector elevation and bisection roots for one prescribed6500s circular Earth orbit; multiple selected observers including no-contact membership, selected-only isolation and clipped interval. Geometric SPICE time/orientation and shared hashed FK5/ICRF data only, no native contact/GF/station conversion truth. Earth station shape is explicitly aligned with PCK to remove shape-input ambiguity; arbitrary configured-body shape/topocentric-frame consistency is not qualified. No light time, stellar, third-body occultation, FOV/masks, detailed azimuth/elevation/range/maxima, arbitrary trajectories, selectable rise-only/set-only semantics, native OS input/hardware/platform/stress/private current NASA or full current-build suite qualification.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--library',required=True);args=parser.parse_args()
    root=Path(__file__).resolve().parents[3];value=fixtures(args.library,root)
    Path(__file__).with_name('NasaContactResultCases.json').write_text(json.dumps(value,indent=2)+'\n')
    print(json.dumps({c['key']:c['events']for c in value['cases']},indent=2))
