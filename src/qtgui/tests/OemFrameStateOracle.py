"""Independent point-mass circles, six-state translations and J2000 rotation.

No native imports. The declared ecliptic convention is IAU1976 J2000 mean
obliquity,84381.448arcsec: https://ssd.jpl.nasa.gov/astro_par.html .
This verifies the declared synthetic interfaces,not arbitrary frame astronomy.
"""
import copy
import json
import math
from pathlib import Path

MU=398600.4415
OBLIQUITY_ARCSEC=84381.448
PLANES={'XY':(0,1),'YZ':(1,2),'ZX':(2,0)}


def circle(seconds,plane,radius=7000.):
    first,second=PLANES[plane]
    rate=math.sqrt(MU/radius**3)
    angle=rate*seconds
    r=[0.,0.,0.];v=[0.,0.,0.]
    r[first]=radius*math.cos(angle);r[second]=radius*math.sin(angle)
    v[first]=-radius*rate*math.sin(angle);v[second]=radius*rate*math.cos(angle)
    return r+v


def exported(seconds,plane,frame):
    values=circle(seconds,plane)
    if frame=='RelativeEquator':
        reference=circle(seconds,'YZ',8000.)
        values=[a-b for a,b in zip(values,reference)]
    if frame=='EarthEcliptic':
        angle=math.radians(OBLIQUITY_ARCSEC/3600.)
        c,s=math.cos(angle),math.sin(angle)
        values=[values[0],c*values[1]+s*values[2],-s*values[1]+c*values[2],
                values[3],c*values[4]+s*values[5],-s*values[4]+c*values[5]]
    return values


def catalog():
    base=json.loads(Path(__file__).with_name('NasaOemShortWindowCases.json').read_text())
    template=next(c for c in base['cases']if c['key']=='AtInitial')
    cases=[]
    for frame in ['EarthEquator','EarthEcliptic','RelativeEquator']:
        for plane in PLANES:
            c=copy.deepcopy(template)
            axes='MJ2000Ec'if frame=='EarthEcliptic'else'MJ2000Eq'
            origin='Other'if frame=='RelativeEquator'else'Earth'
            setup='Sat.NAIFId = -101401;\n'
            if origin=='Other':
                setup+="Create Spacecraft Other;\nOther.NAIFId = -101402;\nOther.Id = 'GMAT-OEM-reference';\nOther.DateFormat = UTCGregorian;\nOther.Epoch = '01 Jan 2000 12:00:00.000';\nOther.CoordinateSystem = EarthMJ2000Eq;\nOther.DisplayStateType = Cartesian;\n"
                setup+=''.join('Other.%s = %.17g;\n'%(name,value)for name,value in zip(['X','Y','Z','VX','VY','VZ'],circle(0.,'YZ',8000.)))
            setup+='Create CoordinateSystem OemFrame;\nOemFrame.Origin = '+origin+';\nOemFrame.Axes = '+axes+';\n'
            c.update(key=frame+'_'+plane,frame_kind=frame,plane=plane,coordinate_system='OemFrame',frame_setup=setup,
                initial_state=circle(0.,plane),expected_center=origin.upper(),expected_frame='MJ2000Ec'if frame=='EarthEcliptic'else'EME2000',
                sampling='IntegratorSteps',scenario='RawSteps',user_interval=False,
                initial_epoch='InitialSpacecraftEpoch',final_epoch='FinalSpacecraftEpoch',
                mission="Propagate 'Entire arc' PropA("+('Sat, Other'if origin=='Other'else'Sat')+") {Sat.ElapsedSecs = 20};\n",
                expected_segments=[[[t]+exported(t,plane,frame)for t in [0,5,10,15,20]]],
                expected_final=[20]+circle(20.,plane)+[9],expected_degree=4,expected_degrees=[4])
            cases.append(c)
    return dict(cases=cases,object_id='GMAT-OEM-frame-state',
        test_interface_profile=dict(sender='New NasaOemFrameStateTests GUI fixture',receiver='RunOemFrameStates independent raw-file checker',
            object_id='GMAT-OEM-frame-state',object_name='Sat',originator='GMAT USER',version='1.0',
            frames=dict(EarthEquator='EME2000 with EARTH center',EarthEcliptic='MJ2000Ec:fixed X rotation from EME2000 by IAU1976 J2000 obliquity84381.448arcsec',RelativeEquator='EME2000 axes centered on Other,another independently propagated circular spacecraft'),
            time_system='UTC',units=['km','km/s'],filenames=['Every state Δ.oem','mission.oem'],exchange='private local unchanged retained bytes,SHA256-bound;no retroactive operational agreement'),
        constants=dict(mu_km3_s2=MU,obliquity_arcsec=OBLIQUITY_ARCSEC,obliquity_source='https://ssd.jpl.nasa.gov/astro_par.html'),
        scope='Complete retained output states for three declared frame/origin kinds and three spanning circular planes. Existing original assertions/catalogs unchanged;no arbitrary frame/domain,whole-E375,E381 or scientific-failure waiver.')


if __name__=='__main__':
    Path(__file__).with_name('NasaOemFrameStateCases.json').write_text(json.dumps(catalog(),indent=2)+'\n')
