"""Independent finite orbital-parameter acceptance fixtures (no GMAT imports).

Elliptic conic geometry uses a perifocal orthonormal basis; spherical velocity
uses radial/north/east unit vectors. Angles are degrees. FPA is the documented
vertical angle from the outward radius, rather than the horizontal convention.
"""
import math

MU = 398600.4415
ELEMENTS = [10000.0, .2, 30.0, 40.0, 50.0, 60.0]
CARTESIAN = ['X','Y','Z','VX','VY','VZ']


def conic(elements, mu=MU):
    a, e, inc, node, peri, anomaly = elements
    inc, node, peri, anomaly = map(math.radians, (inc,node,peri,anomaly))
    c, s = math.cos, math.sin
    p = a*(1-e*e)
    radius = p/(1+e*c(anomaly))
    # The two unit vectors span the orbital plane, expressed in inertial axes.
    u = [c(node)*c(peri)-s(node)*s(peri)*c(inc),
         s(node)*c(peri)+c(node)*s(peri)*c(inc), s(peri)*s(inc)]
    v = [-c(node)*s(peri)-s(node)*c(peri)*c(inc),
         -s(node)*s(peri)+c(node)*c(peri)*c(inc), c(peri)*s(inc)]
    position = [radius*(c(anomaly)*x+s(anomaly)*y) for x,y in zip(u,v)]
    velocity = [math.sqrt(mu/p)*(-s(anomaly)*x+(e+c(anomaly))*y) for x,y in zip(u,v)]
    return position+velocity


def magnitude(vector):
    return math.sqrt(sum(x*x for x in vector))


def spherical(state):
    position, velocity = state[:3], state[3:]
    radius, speed = magnitude(position), magnitude(velocity)
    ra = math.atan2(position[1],position[0])
    dec = math.asin(position[2]/radius)
    east = [-math.sin(ra),math.cos(ra),0]
    north = [-math.sin(dec)*math.cos(ra),-math.sin(dec)*math.sin(ra),math.cos(dec)]
    azi = math.atan2(sum(a*b for a,b in zip(velocity,east)),sum(a*b for a,b in zip(velocity,north)))
    vertical = math.acos(sum(a*b for a,b in zip(position,velocity))/(radius*speed))
    return dict(RMAG=radius,RA=math.degrees(ra)%360,DEC=math.degrees(dec),VMAG=speed,
                RAV=math.degrees(math.atan2(velocity[1],velocity[0]))%360,
                DECV=math.degrees(math.asin(velocity[2]/speed)),AZI=math.degrees(azi)%360,
                FPA=math.degrees(vertical))


def from_spherical(values, representation):
    ra, dec = map(math.radians,(values['RA'],values['DEC']))
    radial = [math.cos(dec)*math.cos(ra),math.cos(dec)*math.sin(ra),math.sin(dec)]
    position = [values['RMAG']*x for x in radial]
    if representation=='SphericalRADEC':
        rv, dv = map(math.radians,(values['RAV'],values['DECV']))
        direction = [math.cos(dv)*math.cos(rv),math.cos(dv)*math.sin(rv),math.sin(dv)]
    else:
        azi, vertical = map(math.radians,(values['AZI'],values['FPA']))
        east = [-math.sin(ra),math.cos(ra),0]
        north = [-math.sin(dec)*math.cos(ra),-math.sin(dec)*math.sin(ra),math.cos(dec)]
        direction = [math.cos(vertical)*r+math.sin(vertical)*(math.cos(azi)*n+math.sin(azi)*e)
                     for r,n,e in zip(radial,north,east)]
    return position+[values['VMAG']*x for x in direction]


def anomaly_from_eccentric(eccentricity, eccentric_degrees):
    angle = math.radians(eccentric_degrees)
    return math.degrees(math.atan2(math.sqrt(1-eccentricity**2)*math.sin(angle),math.cos(angle)-eccentricity))%360


def eccentric_from_mean(eccentricity, mean_degrees):
    mean = math.radians(mean_degrees);angle=mean
    for _ in range(30):
        correction = (angle-eccentricity*math.sin(angle)-mean)/(1-eccentricity*math.cos(angle))
        angle -= correction
        if abs(correction)<1e-15:break
    assert abs(angle-eccentricity*math.sin(angle)-mean)<1e-14
    return math.degrees(angle)


def calculate(case):
    parameter=case['parameter'];value=case['assigned_value'];elements=list(case['initial_elements'])
    if case.get('read_only'):
        a,e,inc,node,peri,ta=elements
        if parameter=='SemilatusRectum':value=a*(1-e*e)
        elif parameter=='HA':value=math.degrees(2*math.atanh(math.sqrt((e-1)/(e+1))*math.tan(math.radians(ta)/2)))
        else:
            eccentric=math.degrees(2*math.atan2(math.sqrt(1-e)*math.sin(math.radians(ta)/2),math.sqrt(1+e)*math.cos(math.radians(ta)/2)))
            value=eccentric if parameter=='EA' else eccentric-math.degrees(e*math.sin(math.radians(eccentric)))
        return [value]+conic(elements,case['mu'])
    if case['representation'] in ['SphericalRADEC','SphericalAZFPA']:
        values=spherical(conic(elements,case['mu']));values[parameter]=value
        state=from_spherical(values,case['representation'])
    else:
        if parameter in ['SMA','ECC','INC','RAAN','AOP','TA']:
            elements[['SMA','ECC','INC','RAAN','AOP','TA'].index(parameter)]=value
        elif parameter=='EA':elements[5]=anomaly_from_eccentric(elements[1],value)
        elif parameter=='MA':elements[5]=anomaly_from_eccentric(elements[1],eccentric_from_mean(elements[1],value))
        elif parameter=='SemilatusRectum':elements[0]=value/(1-elements[1]**2)
        elif parameter in ['RadApo','RadPer']:
            apogee=elements[0]*(1+elements[1]);perigee=elements[0]*(1-elements[1])
            if parameter=='RadApo':apogee=value
            else:perigee=value
            elements[0]=(apogee+perigee)/2;elements[1]=(apogee-perigee)/(apogee+perigee)
        else:raise ValueError(parameter)
        state=conic(elements,case['mu'])
    return [((value+180)%360-180) if parameter in ['RA','RAV'] else value]+state


def generate_cases():
    rows=[('EA',506,95.0),('ECC',507,.35),('MA',513,125.0),('RadApo',517,14000.0),
          ('RadPer',518,7500.0),('SMA',521,12000.0),('TA',522,150.0),('AOP',527,75.0),
          ('AZI',528,135.0),('DEC',533,-25.0),('DECV',534,-12.0),('FPA',535,110.0),
          ('INC',538,55.0),('RA',541,210.0),('RAAN',542,125.0),('RAV',543,280.0),('VMAG',544,8.25),('RMAG',519,10000.0)]
    state=conic(ELEMENTS);sph=spherical(state);a,e,inc,node,peri,ta=ELEMENTS
    eccentric=math.degrees(2*math.atan2(math.sqrt(1-e)*math.sin(math.radians(ta)/2),math.sqrt(1+e)*math.cos(math.radians(ta)/2)))
    before=dict(SMA=a,ECC=e,INC=inc,RAAN=node,AOP=peri,TA=ta,EA=eccentric,
                MA=eccentric-math.degrees(e*math.sin(math.radians(eccentric))),RadApo=a*(1+e),RadPer=a*(1-e))|sph
    origin={'SMA','ECC','TA','EA','MA','RadApo','RadPer','RMAG'}
    cases=[]
    for parameter,row,value in rows:
        representation='SphericalAZFPA' if parameter in ['AZI','FPA','RA','DEC','VMAG','RMAG'] else 'SphericalRADEC' if parameter in sph else 'ModifiedKeplerian' if parameter in ['RadApo','RadPer'] else 'Keplerian'
        destination='Sat.'+('Earth' if parameter in origin else 'EarthMJ2000Eq')+'.'+parameter
        setup='Sat.DateFormat = TAIModJulian;\nSat.Epoch = 21545;\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DisplayStateType = Cartesian;\n'
        setup+=''.join('Sat.'+name+' = '+format(x,'.17g')+';\n' for name,x in zip(CARTESIAN,state))
        setup+='Sat.DisplayStateType = '+representation+';\n'
        c=dict(key=parameter,parameter=parameter,assigned_value=value,mu=MU,initial_elements=ELEMENTS,
               representation=representation,orbital_case=True,setup=setup,destination=destination,
               before=format(before[parameter],'.17g'),expression=format(value,'.17g'),
               report=destination+' '+' '.join('Sat.EarthMJ2000Eq.'+name for name in CARTESIAN),
               invalid=['AbsentNASA + 1','1 +'],absolute_tolerance=2e-8,relative_tolerance=2e-12,
               column_absolute_tolerances=[1e-9,2e-8,2e-8,2e-8,2e-11,2e-11,2e-11],requirements=['FRR!E'+str(row)])
        if parameter in ['EA','MA']:
            c.update(key='Read'+parameter,read_only=True,conflict_requirement=c['requirements'][0],requirements=[],reject_destination=destination,destination='R',before='0',expression=destination,report=destination+' '+' '.join('Sat.EarthMJ2000Eq.'+name for name in CARTESIAN))
        c['expected']=calculate(c);cases.append(c)
    for parameter,row,elements in [('HA',536,[-12000.0,1.4,30.0,40.0,50.0,60.0]),('SemilatusRectum',520,ELEMENTS)]:
        c=dict(cases[0]);destination='Sat.Earth.'+parameter
        setup='Sat.DateFormat = TAIModJulian;\nSat.Epoch = 21545;\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DisplayStateType = Cartesian;\n'
        setup+=''.join('Sat.'+name+' = '+format(x,'.17g')+';\n' for name,x in zip(CARTESIAN,conic(elements)))
        setup+='Sat.DisplayStateType = Keplerian;\n'
        c.update(key='Read'+parameter,parameter=parameter,initial_elements=elements,setup=setup,
                 representation='Keplerian',read_only=parameter=='HA',reject_destination=destination,destination='R',before='0',expression=destination,
                 report=destination+' '+' '.join('Sat.EarthMJ2000Eq.'+name for name in CARTESIAN),requirements=[],conflict_requirement='FRR!E536' if parameter=='HA' else 'FRR!E520')
        if parameter=='SemilatusRectum':c.update(key=parameter,destination=destination,expression='11000',before='9600',assigned_value=11000.0)
        c['expected']=calculate(c);cases.append(c)
    for c in cases:
        if c['parameter']=='RMAG':c.update(requirements=[],conflict_requirement='FRR!E519')
    return cases

if __name__=='__main__':
    import hashlib,json
    from pathlib import Path
    root=Path(__file__).resolve().parents[3]
    refs=['doc/help/src/System_CalculationParameters.xml','doc/help/src/Resource_SpacecraftOrbitState.xml']
    data=dict(cases=generate_cases(),reference_sources={f:hashlib.sha256((root/f).read_bytes()).hexdigest() for f in refs},scope='17 selected Earth/EarthMJ2000Eq orbital parameter setters and3 readonly anomaly calculations/protections, complete Cartesian witnesses and syntax-error Apply/OK recovery. No all bodies/frames, singular/hyperbolic, physical invalid runtime domains, native OS input/platform/stress or full-current-build qualification.')
    (root/'src/qtgui/tests/NasaOrbitalParameterCases.json').write_text(json.dumps(data,indent=2)+'\n')
    print('Generated',len(data['cases']),'independent orbital setter fixtures')
