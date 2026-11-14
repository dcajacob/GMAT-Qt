"""Full FRR-38 scalar-family inventory and independent NLC operand fixtures.

The malformed literal cross-reference FRR-388 is retained. The conservative
inventory covers the entire FRR-38 parameter chapter, including all component
expansions and all twelve Euler sequences. Matrices and Gregorian strings are
classified explicitly, not silently dropped. No GMAT getter computes truth.
"""
import argparse
import bisect
import ctypes as C
import hashlib
import json
import math
from pathlib import Path

from OrbitalParameterOracle import MU, conic, spherical
from OrbitCalculationOracle import geometry, cross, dot, norm

ROW_TYPES = {
503:['Altitude'],504:['BetaAngle'],505:['C3Energy'],506:['EA'],507:['ECC'],508:['Energy'],509:['HMAG'],510:['Latitude'],511:['Longitude'],512:['LST'],513:['MA'],514:['MHA'],515:['MM'],516:['OrbitPeriod'],517:['RadApo'],518:['RadPer'],519:['RMAG'],520:['SemilatusRectum'],521:['SMA'],522:['TA'],523:['VelApoapsis'],524:['VelPeriapsis'],527:['AOP'],528:['AZI'],529:['BdotT'],530:['BdotR'],531:['BVectorAngle'],532:['BVectorMag'],533:['DEC'],534:['DECV'],535:['FPA'],536:['HA'],537:['HX','HY','HZ'],538:['INC'],539:['OrbitSTM'],540:['OrbitSTMA','OrbitSTMB','OrbitSTMC','OrbitSTMD'],541:['RA'],542:['RAAN'],543:['RAV'],544:['VMAG'],545:['VX','VY','VZ'],546:['X','Y','Z'],547:['OutgoingRHA'],548:['OutgoingDHA'],551:['AngularVelocityX','AngularVelocityY','AngularVelocityZ'],552:['Cd'],553:['Cr'],554:['DCM'+str(i)+str(j) for i in range(1,4) for j in range(1,4)],555:['DragArea'],556:['DryMass'],557:['ElapsedDays'],558:['ElapsedSecs'],559:['EulerAngle1','EulerAngle2','EulerAngle3'],560:['EulerAngleRate1','EulerAngleRate2','EulerAngleRate3'],561:['Q1','Q2','Q3','Q4'],562:['SRPArea'],563:['TAIModJulian'],564:['TAIGregorian'],565:['TDBModJulian'],566:['TDBGregorian'],567:['TTModJulian'],568:['TTGregorian'],569:['UTCModJulian'],570:['UTCGregorian'],571:['TotalMass'],572:['MRP1','MRP2','MRP3'],573:['EulerAngle1','EulerAngle2','EulerAngle3'],574:['EulerAngleRate1','EulerAngleRate2','EulerAngleRate3'],576:['Element1'],577:['Element2'],578:['Element3'],579:['V'],580:['N'],581:['B'],583:['Pressure'],584:['Volume'],585:['FuelDensity'],586:['FuelMass'],587:['Temperature'],588:['RefTemperature']}


def number(v): return format(v,'.17g')
def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def state_setup(name,state):
    return f'{name}.CoordinateSystem = EarthMJ2000Eq;\n{name}.DisplayStateType = Cartesian;\n'+''.join(name+'.'+n+' = '+number(x)+';\n' for n,x in zip(['X','Y','Z','VX','VY','VZ'],state))


def sun_kernel(root):
    """Encode prescribed constant barycentric inputs; CSPICE is only a writer."""
    p=root/'src/qtgui/tests/constraint-scalars-sun.bsp'
    if p.exists(): return p
    from AcceptanceSpecializedCompare import Spice
    spice=Spice(root/'application/bin/libGmatBase.so.R2026a');row=C.c_double*6
    spice.lib.spkopn_c.argtypes=[C.c_char_p,C.c_char_p,C.c_int,C.POINTER(C.c_int)];spice.lib.spkopn_c.restype=None
    spice.lib.spkcls_c.argtypes=[C.c_int];spice.lib.spkcls_c.restype=None
    spice.lib.spkw08_c.argtypes=[C.c_int,C.c_int,C.c_int,C.c_char_p,C.c_double,C.c_double,C.c_char_p,C.c_int,C.c_int,C.POINTER(row),C.c_double,C.c_double];spice.lib.spkw08_c.restype=None
    handle=C.c_int();spice.call('spkopn_c',str(p).encode(),b'Prescribed scalar-family solar inputs',0,C.byref(handle))
    try:
        for body,position in [(399,[0,0,0]),(10,[1e8,2e8,3e8]),(2000001,[0,0,0])]:
            state=position+[0,0,0];states=(row*2)(row(*state),row(*state))
            spice.call('spkw08_c',handle.value,body,0,b'J2000',-1e11,1e11,b'Constant known barycentric states',1,2,states,-1e11,2e11)
    finally:spice.call('spkcls_c',handle.value)
    return p


def fk5_sun(root, epoch):
    # Independently transpose the documented Rodrigues frame rotation. Reuse
    # the existing pure table-interpolation oracle, never its native readers.
    from EclipseGeometryOracle import Data,FRAME_TABLE
    data=object.__new__(Data)
    data.frame_rows=[[float(x) for x in line.split(',')] for line in (root/FRAME_TABLE).read_text().splitlines()]
    data.frame_epochs=[r[0] for r in data.frame_rows]
    seconds=(epoch-21545)*86400-32.0343817
    basis=[[int(i==j) for i in range(3)] for j in range(3)]
    return [dot([1e8,2e8,3e8],data.fk5_to_icrf(seconds,e)) for e in basis]


def generate(root, native_inventory):
    import NasaGuiCoverage as nasa
    authority=nasa.inventory(root);known={r['key']:r for r in authority['requirements']}
    native={r['type']:r for r in native_inventory['rows']};cases=[]
    def add(key,typ,ref,value,setup='',tolerance=2e-8,partition='orbit'):
        assert typ in native and native[typ].get('scalar'),typ
        rows=[f'FRR!E{n}' for n,types in ROW_TYPES.items() if typ in types]
        cases.append(dict(key=key,parameter_type=typ,reference=ref,expected=value,absolute_tolerance=tolerance,setup=setup,partition=partition,source_rows=rows,roles=['left','right'],expected_solver_control=value,solver_control_tolerance=max(tolerance,2e-7)))
    elements=[10000,.2,30,40,50,60];state=conic(elements);r=norm(state[:3]);h=cross(state[:3],state[3:]);hm=norm(h);a,e,i,node,peri,ta=elements
    ea=2*math.atan2(math.sqrt(1-e)*math.sin(math.radians(ta)/2),math.sqrt(1+e)*math.cos(math.radians(ta)/2))
    values=dict(SMA=a,ECC=e,INC=i,RAAN=node,AOP=peri,TA=ta,EA=math.degrees(ea),MA=math.degrees(ea-e*math.sin(ea)),RadApo=a*(1+e),RadPer=a*(1-e),Energy=-MU/(2*a),C3Energy=-MU/a,HMAG=hm,MM=math.sqrt(MU/a**3),OrbitPeriod=2*math.pi*math.sqrt(a**3/MU),VelApoapsis=hm/(a*(1+e)),VelPeriapsis=hm/(a*(1-e)),SemilatusRectum=a*(1-e*e),HX=h[0],HY=h[1],HZ=h[2],**spherical(state))
    values.update(dict(zip(['X','Y','Z','VX','VY','VZ'],state)))
    for typ,value in values.items():
        if typ=='X':continue # Reuse the exact original selected-X GUI witnesses.
        if typ in ('RA','RAV'):value=(value+180)%360-180
        dep=native[typ]['dependency_name'];assert dep in ('Origin','CoordinateSystem'),(typ,dep)
        add('Ellipse'+typ,typ,'Sat.'+('Earth' if dep=='Origin' else 'EarthMJ2000Eq')+'.'+typ,value)
    hyper=[-18000,1.5,35,40,25,55];hs=conic(hyper);g=geometry(hs);nu=math.radians(hyper[-1]);ha=math.asinh(math.sqrt(hyper[1]**2-1)*math.sin(nu)/(1+hyper[1]*math.cos(nu)))
    hyp=dict(HA=math.degrees(ha),**{k:g[k] for k in ['BdotT','BdotR','BVectorAngle','BVectorMag','OutgoingRHA','OutgoingDHA']})
    for typ,value in hyp.items():
        dep=native[typ]['dependency_name'];add('Hyper'+typ,typ,'Hyper.'+('Earth' if dep=='Origin' else 'EarthMJ2000Eq')+'.'+typ,value,partition='hyperbolic')
    # Prescribe a known non-Earth oblate-body geometry, exactly as the existing
    # forward geodetic oracle does. Do not use native inverse calculations.
    lat,lon=map(math.radians,[25,170]);aa=1000;f=.2;hgt=50;n=aa/math.sqrt(1-f*(2-f)*math.sin(lat)**2)
    fixed=[(n+hgt)*math.cos(lat)*math.cos(lon),(n+hgt)*math.cos(lat)*math.sin(lon),(n*(1-f*(2-f))+hgt)*math.sin(lat),.1,.2,.3]
    angle=math.radians(57);c,s=math.cos(angle),math.sin(angle);gs=[]
    for j in (0,3):gs += [c*fixed[j]-s*fixed[j+1],s*fixed[j]+c*fixed[j+1],fixed[j+2]]
    for typ,value in dict(Altitude=50,Latitude=25,Longitude=170,MHA=57,LST=227).items():add('Geodetic'+typ,typ,'Geo.Probe.'+typ,value,partition='geodetic')
    attitude=json.loads((root/'src/qtgui/tests/NasaAttitudeDefinitionCases.json').read_text())['cases']
    eulers=[r for r in attitude if r['key'].startswith('Euler')]
    assert len(eulers)==12 and len({r['sequence'] for r in eulers})==12
    for spec in eulers:
        setup='Sat.EulerAngleSequence = '+spec['sequence']+';\nSat.AttitudeDisplayStateType = EulerAngles;\nSat.AttitudeRateDisplayStateType = EulerAngleRates;\n'
        setup+=''.join('Sat.'+name+' = '+number(v)+';\n' for name,v in zip(spec['names']+spec['rate_names'],spec['values']+spec['rate_values']))
        for typ,value in zip(spec['names']+spec['rate_names'],spec['values']+spec['rate_values']):add(spec['key']+typ,typ,'Sat.'+typ,value,setup,partition='euler-'+spec['sequence'])
    spec=next(r for r in attitude if r['key']=='Euler321');q=next(r['values'] for r in spec['representations'] if r['kind']=='Quaternion');mrp=next(r['values'] for r in spec['representations'] if r['kind']=='MRPs')
    att_setup='Sat.EulerAngleSequence = 321;\nSat.AttitudeDisplayStateType = Quaternion;\nSat.AttitudeRateDisplayStateType = AngularVelocity;\n'+''.join('Sat.Q'+str(j+1)+' = '+number(v)+';\n' for j,v in enumerate(q))+''.join('Sat.AngularVelocity'+axis+' = '+number(v)+';\n' for axis,v in zip('XYZ',spec['angular_velocity_degrees']))
    for j in range(3):
        for k in range(3):add('AttitudeDCM'+str(j+1)+str(k+1),'DCM'+str(j+1)+str(k+1),'Sat.DCM'+str(j+1)+str(k+1),spec['matrix'][j][k],att_setup,partition='attitude-matrix')
    for j,value in enumerate(q,1):add('AttitudeQ'+str(j),'Q'+str(j),'Sat.Q'+str(j),value,att_setup,partition='quaternion')
    for j,value in enumerate(mrp,1):add('AttitudeMRP'+str(j),'MRP'+str(j),'Sat.MRP'+str(j),value,att_setup,partition='mrp')
    for axis,value in zip('XYZ',spec['angular_velocity_degrees']):add('AttitudeOmega'+axis,'AngularVelocity'+axis,'Sat.AngularVelocity'+axis,value,att_setup,partition='angular-velocity')
    for typ,value in dict(Cd=2.45,Cr=1.67,DragArea=3.21,DryMass=321,SRPArea=4.56,TotalMass=321+80+7).items():add('Ballistic'+typ,typ,'Sat.'+typ,value,partition='ballistic-mass')
    for typ,value in dict(Pressure=1500,Volume=.8,FuelDensity=1000,FuelMass=80,Temperature=20,RefTemperature=15).items():add('Tank'+typ,typ,'Sat.Fuel.'+typ,value,partition='attached-tank')
    for typ,value in dict(Element1=.011,Element2=-.023,Element3=.037,V=.011,N=-.023,B=.037).items():add('Burn'+typ,typ,'Burn.'+typ,value,partition='burn-frameless')
    epoch=next(r for r in json.loads((root/'src/qtgui/tests/NasaEpochDefinitionCases.json').read_text())['cases'] if r['key']=='UTCGregorian')
    for system in ('TAI','TDB','TT','UTC'):
        tol=(5e-5 if system=='TDB' else 2e-6)/86400
        add('Epoch'+system,system+'ModJulian','Date.'+system+'ModJulian',epoch['expected'][system],tolerance=tol,partition='epoch')
    for typ,value in dict(ElapsedSecs=10800,ElapsedDays=.125).items():add('Elapsed'+typ,typ,'Clock.'+typ,value,partition='elapsed')
    sun=fk5_sun(root,21545);beta=math.degrees(math.asin(dot([x/hm for x in h],[x/norm(sun) for x in sun])))
    add('SolarBetaAngle','BetaAngle','Sat.Earth.BetaAngle',beta,partition='solar-geometry')
    base='SolarSystem.EphemerisSource = SPICE;\nSolarSystem.SPKFilename = \'@ROOT@/src/qtgui/tests/constraint-scalars-sun.bsp\';\nEarth.Mu = '+number(MU)+';\n'
    base+='''Create ChemicalTank Fuel Spare;
Fuel.Pressure = 1500;
Fuel.Volume = .8;
Fuel.FuelDensity = 1000;
Fuel.FuelMass = 80;
Fuel.Temperature = 20;
Fuel.RefTemperature = 15;
Spare.FuelMass = 7;
Create Spacecraft Sat Hyper Geo Clock Date;
'''
    for name in ['Sat','Hyper','Geo','Clock']:base+=f"{name}.DateFormat = A1ModJulian;\n{name}.Epoch = '21545';\n"
    base+=state_setup('Sat',state)+state_setup('Hyper',hs)+state_setup('Clock',state)+state_setup('Date',state)
    base+="Date.DateFormat = UTCGregorian;\nDate.Epoch = '"+epoch['literal']+"';\n"
    base+='''Sat.Cd = 2.45;
Sat.Cr = 1.67;
Sat.DragArea = 3.21;
Sat.SRPArea = 4.56;
Sat.DryMass = 321;
Sat.Tanks = {Fuel, Spare};
Sat.Attitude = Spinner;
Sat.AttitudeCoordinateSystem = EarthMJ2000Eq;
Create ImpulsiveBurn Burn;
Burn.CoordinateSystem = Local;
Burn.Axes = VNB;
Burn.Element1 = .011;
Burn.Element2 = -.023;
Burn.Element3 = .037;
Create Asteroid Probe;
Probe.NAIFId = 2000001;
Probe.OrbitSpiceKernelName = {'@ROOT@/src/qtgui/tests/constraint-scalars-sun.bsp'};
Probe.Mu = 1000;
Probe.EquatorialRadius = 1000;
Probe.Flattening = .2;
Probe.OrientationEpoch = 21545;
Probe.SpinAxisRAConstant = -90;
Probe.SpinAxisRARate = 0;
Probe.SpinAxisDECConstant = 90;
Probe.SpinAxisDECRate = 0;
Probe.RotationConstant = 417;
Probe.RotationRate = 0;
Create CoordinateSystem ProbeEq;
ProbeEq.Origin = Probe;
ProbeEq.Axes = MJ2000Eq;
Geo.CoordinateSystem = ProbeEq;
Geo.DisplayStateType = Cartesian;
'''
    base+=''.join('Geo.'+name+' = '+number(x)+';\n' for name,x in zip(['X','Y','Z','VX','VY','VZ'],gs))
    scope=[]
    for row,types in ROW_TYPES.items():
        source=known[f'FRR!E{row}'];scalar=[t for t in types if native[t].get('scalar')];other=[t for t in types if not native[t].get('scalar')]
        witnesses=[c['key'] for c in cases if c['parameter_type'] in scalar]
        reused=['original-selected-X-visible-and-unattended'] if 'X' in scalar else []
        assert not scalar or witnesses or reused,(row,types)
        scope.append(dict(key=source['key'],fingerprint=source['fingerprint'],source_text=source['text'],native_types=types,scalar_types=scalar,non_scalar_types=other,new_case_keys=witnesses,reused_cases=reused,non_scalar_reason=('Native Rmatrix or Gregorian String; outside the scalar-only NLC obligation, not retired from its own NASA requirement.' if other else '')))
    required={t for row in scope for t in row['scalar_types']};tested={c['parameter_type'] for c in cases}|{'X'}
    assert required==tested,(required-tested,tested-required)
    assert len({c['key'] for c in cases})==len(cases)
    kernel=sun_kernel(root)
    physical=[kernel,root/'application/data/icrf/ICRF_Table.txt']
    return dict(schema_version=1,scope='Complete FRR-38 scalar type/operand family; no arbitrary parameter physics/current-build/NASA qualification.',authority_sha256=authority['sources'],literal_reference='FRR-388',reference_handling='No such declared ID exists. Conservatively cover the entire FRR-38 Parameters and Calculations chapter, retaining every literal row and all non-scalar exclusions. No source requirement is rewritten.',source_rows=scope,unique_scalar_types=sorted(required),native_factory_discovery_sha256=sha(root/'build/nasa-coverage/coverage85-20261101/scalar-inventory2/inventory.json'),native_inventory= native_inventory,physical_inputs={str(p.relative_to(root)):sha(p) for p in physical},base_setup=base,mission_prefix='Clock.A1ModJulian = 21545.125;\n',cases=cases)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3]);p.add_argument('--output',type=Path,required=True);a=p.parse_args();root=a.repo.resolve()
    v=generate(root,json.loads((root/'build/nasa-coverage/coverage85-20261101/scalar-inventory2/inventory.json').read_text()));a.output.write_text(json.dumps(v,indent=2)+'\n');print(json.dumps(dict(cases=len(v['cases']),scalar_types=len(v['unique_scalar_types']),source_rows=len(v['source_rows']),credit=0)))
