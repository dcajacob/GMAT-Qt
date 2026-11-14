"""Scoped special-state recipes;Brouwer is retained prototype parity,not averaging truth."""
import math
from pathlib import Path
from PlanetodeticOracle import reference as planetodetic
from HyperbolicConversionOracle import geometry,representations
from BrouwerPrototypeOracle import adapt,cartesian
from FunctionConicBindingOracle import cross

ROOT=Path(__file__).resolve().parents[3]
PLANET=['PlanetodeticRMAG','PlanetodeticLON','PlanetodeticLAT','PlanetodeticVMAG','PlanetodeticAZI','PlanetodeticHFPA','Planetodetic','Altitude']
ASYM=[side+name for side in ['Incoming','Outgoing'] for name in ['RadPer','C3Energy','RHA','DHA','BVAZI','Asymptote']]
BROUWER=[prefix+name for prefix in ['BrouwerShort','BrouwerLong'] for name in ['SMA','ECC','INC','RAAN','AOP','MA']]+['BrouwerMeanShort','BrouwerMeanLong']
BPLANE=['BdotT','BdotR','BVectorMag','BVectorAngle']
TYPES=PLANET+ASYM+BROUWER+BPLANE
SCOPES=['root','private','shared']
MU=398600.4415
RADIUS=6378.1363
FLATTENING=.1
FORWARD={kind:adapt(ROOT/'prototype/StateConv'/filename)[0] for kind,filename in [('Short','BROLYD_short.m'),('Long','BROLYD.m')]}


def dot(a,b):return sum(x*y for x,y in zip(a,b))
def magnitude(a):return math.sqrt(dot(a,a))


def recipe(key,scope,calls=0):
    i=SCOPES.index(scope);step=0 if i==0 else calls
    if key in PLANET:
        values=[[7300,35,42,7.2,63,12],[8500,125,47,7.5,122,-8],[9000,215,61,6.8,238,17]][i][:];values[0]+=i*25*step;values[1]+=i*2*step;values[4]+=i*3*step
        ref=planetodetic(values,FLATTENING,RADIUS);named=dict(zip(PLANET[:6],values));named['PlanetodeticLON']=(named['PlanetodeticLON']+180)%360-180;named['PlanetodeticAZI']=(named['PlanetodeticAZI']+180)%360-180;named['Planetodetic']=[named[n] for n in PLANET[:6]];named['Altitude']=ref['altitude']
        return dict(state=ref['cartesian'],expected=named[key],input_planetodetic=values,owner_frame='SpecialFixed',frame_axes='BodyFixed',frame_role='owner' if key=='Altitude' else 'parameter',mu=MU,equatorial_radius=RADIUS,flattening=FLATTENING,owner_epoch=21545,oracle='independent-exterior-spheroid-normal-and-radial-sky-geometry')
    if key in BROUWER:
        kind='Long' if 'Long' in key else 'Short';mean=[[10800,.17,47,112,38,143],[11900,.21,56,134,49,152],[12600,.23,69,154,59,164]][i][:];mean[0]+=i*40*step;mean[5]+=i*3*step;state=cartesian(FORWARD[kind](mean),MU);names=['SMA','ECC','INC','RAAN','AOP','MA'];value=mean if key.startswith('BrouwerMean') else mean[names.index(key.removeprefix('Brouwer'+kind))]
        return dict(state=state,expected=value,mean=mean,owner_frame='EarthMJ2000Eq',frame_axes='MJ2000Eq',frame_role='parameter',mu=MU,owner_epoch=21545,oracle='retained-NASA-prototype-forward-reconstruction-not-independent-physical-averaging')
    elements=[-(14000+1000*i+100*i*step),1.4+.1*i,30+10*i,40+10*i,50+10*i,60+10*i+5*i*step];states=representations(elements,MU);state=states['Cartesian']
    if key in ASYM:
        side='Incoming' if key.startswith('Incoming') else 'Outgoing';values=states[side+'Asymptote'];value=values if key.endswith('Asymptote') else values[['RadPer','C3Energy','RHA','DHA','BVAZI'].index(key.removeprefix(side))]
        return dict(state=state,expected=value,elements=elements,owner_frame='EarthMJ2000Eq',frame_axes='MJ2000Eq',frame_role='parameter',mu=MU,owner_epoch=21545,oracle='independent-limiting-position-ray-geometry-incoming-opposes-Bplane-incoming-velocity')
    assert key in BPLANE
    p,q,normal,position,velocity=geometry(elements,MU);a,e=elements[:2];limit=math.acos(-1/e);incoming=[math.sin(limit)*x+(e+math.cos(limit))*y for x,y in zip(p,q)];S=[x/magnitude(incoming) for x in incoming];H=cross(position,velocity);B=[x/math.sqrt(-MU/a) for x in cross(S,H)];T=[S[1],-S[0],0];T=[x/magnitude(T) for x in T];R=cross(S,T);bt,br=dot(B,T),dot(B,R);values=dict(BdotT=bt,BdotR=br,BVectorMag=math.hypot(bt,br),BVectorAngle=math.degrees(math.atan2(br,bt)))
    return dict(state=state,expected=values[key],elements=elements,incoming_velocity_unit=S,B=B,T=T,R=R,owner_frame='EarthMJ2000Eq',frame_axes='MJ2000Eq',frame_role='parameter',mu=MU,owner_epoch=21545,oracle='independent-incoming-velocity-limit-and-impact-parameter-cross-products')
