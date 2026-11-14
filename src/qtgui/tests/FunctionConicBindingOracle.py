"""Independent nonsingular conic owner recipes, extending retained orbit oracles."""
import math
from OrbitalParameterOracle import conic,spherical,magnitude
from OrbitExtendedConversionOracle import representations

TYPES='SMA ECC INC RAAN RADN AOP TA MA EA HA MM Keplerian RadApo RadPer ModKeplerian RA DEC VMAG RAV DECV AZI FPA SphericalRADEC SphericalAZFPA EquinoctialH EquinoctialK EquinoctialP EquinoctialQ MLONG Equinoctial SemilatusRectum ModEquinoctialF ModEquinoctialG ModEquinoctialH ModEquinoctialK TLONG ModEquinoctial AltEquinoctialP AltEquinoctialQ Delaunayl Delaunayg Delaunayh DelaunayL DelaunayG DelaunayH Delaunay VelApoapsis VelPeriapsis Apoapsis Periapsis OrbitPeriod Energy HMAG HX HY HZ DLA RLA'.split()
SCOPES=['root','private','shared']
MU=398600.4415


def cross(a,b):return [a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]]


def recipe(key,scope,calls=0):
    i=SCOPES.index(scope);step=0 if i==0 else calls
    hyper=key in ['HA','DLA','RLA']
    elements=[-(14000+1000*i+100*i*step) if hyper else 10000+1000*i+100*i*step,1.4+.1*i if hyper else .2+.05*i,30+10*i,40+10*i,50+10*i,60+10*i+5*i*step]
    a,e,inc,node,peri,nu=elements;state=conic(elements,MU);r,v=state[:3],state[3:];radius,speed=magnitude(r),magnitude(v);h=cross(r,v);hm=magnitude(h);values=spherical(state)
    for name in ['RA','RAV','AZI']:values[name]=(values[name]+180)%360-180
    values.update(SMA=a,ECC=e,INC=inc,RAAN=node,RADN=(node+180)%360,AOP=peri,TA=nu,MM=math.sqrt(MU/abs(a)**3),RadApo=a*(1+e),RadPer=a*(1-e),SemilatusRectum=a*(1-e*e),HMAG=hm,HX=h[0],HY=h[1],HZ=h[2],Energy=-MU/(2*a),Apoapsis=sum(x*y for x,y in zip(r,v))/(radius*speed),Periapsis=sum(x*y for x,y in zip(r,v))/(radius*speed))
    if hyper:
        H=2*math.atanh(math.sqrt((e-1)/(e+1))*math.tan(math.radians(nu)/2));values['HA']=math.degrees(H)
        far=conic([a,e,inc,node,peri,math.degrees(math.acos(-1/e))-.000001],MU)[3:]
        # The limiting outgoing velocity direction has perifocal components
        # [-sin(nu_inf),e+cos(nu_inf)]; evaluate its basis with finite p/r.
        ninf=math.acos(-1/e);im,om,w=map(math.radians,[inc,node,peri]);c,s=math.cos,math.sin
        u=[c(om)*c(w)-s(om)*s(w)*c(im),s(om)*c(w)+c(om)*s(w)*c(im),s(w)*s(im)]
        q=[-c(om)*s(w)-s(om)*c(w)*c(im),-s(om)*s(w)+c(om)*c(w)*c(im),c(w)*s(im)]
        direction=[-s(ninf)*x+(e+c(ninf))*y for x,y in zip(u,q)];length=magnitude(direction)
        assert all(abs(x/magnitude(far)-y/length)<1e-7 for x,y in zip(far,direction))
        values.update(RLA=math.degrees(math.atan2(direction[1],direction[0])),DLA=math.degrees(math.asin(direction[2]/length)))
    else:
        reps=representations(elements,MU);E=math.atan2(math.sqrt(1-e*e)*math.sin(math.radians(nu)),e+math.cos(math.radians(nu)));mean=(math.degrees(E-e*math.sin(E)))%360
        values.update(EA=math.degrees(E)%360,MA=mean,OrbitPeriod=2*math.pi*math.sqrt(a**3/MU),VelApoapsis=math.sqrt(MU/a*(1-e)/(1+e)),VelPeriapsis=math.sqrt(MU/a*(1+e)/(1-e)))
        for name,columns in [('Equinoctial',['SMA','EquinoctialH','EquinoctialK','EquinoctialP','EquinoctialQ','MLONG']),('ModifiedEquinoctial',['SemilatusRectum','ModEquinoctialF','ModEquinoctialG','ModEquinoctialH','ModEquinoctialK','TLONG']),('AlternateEquinoctial',['SMA','EquinoctialH','EquinoctialK','AltEquinoctialP','AltEquinoctialQ','MLONG']),('Delaunay',['Delaunayl','Delaunayg','Delaunayh','DelaunayL','DelaunayG','DelaunayH'])]:values.update(zip(columns,reps[name]))
        values.update(Keplerian=reps['Keplerian'],ModKeplerian=reps['ModifiedKeplerian'],Equinoctial=reps['Equinoctial'],ModEquinoctial=reps['ModifiedEquinoctial'],Delaunay=reps['Delaunay'],SphericalRADEC=[values[n] for n in ['RMAG','RA','DEC','VMAG','RAV','DECV']],SphericalAZFPA=[values[n] for n in ['RMAG','RA','DEC','VMAG','AZI','FPA']])
    result=dict(elements=elements,state=state,expected=values[key],mu=MU,owner_epoch=21545,hyperbolic=hyper)
    if key=='RADN':
        turned=list(elements);turned[3]=(node+17)%360;result.update(radn_target=(values[key]+17)%360,radn_changed_state=conic(turned,MU))
    return result
