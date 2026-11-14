"""Independent local Cartesian/spherical geometry; no GMAT or frame utilities."""
import math
TYPES=('Cartesian','SphericalAZFPA','SphericalRADEC')
METADATA={
 'Cartesian':dict(names=['X','Y','Z','VX','VY','VZ'],units=['km']*3+['km/s']*3),
 'SphericalAZFPA':dict(names=['RMagnitude','RA','DEC','VMagnitude','AZI','FPA'],units=['km','deg','deg','km/s','deg','deg']),
 'SphericalRADEC':dict(names=['RMagnitude','RA','DEC','VMagnitude','RAV','DECV'],units=['km','deg','deg','km/s','deg','deg'])}
def definitions(state):
 r=math.sqrt(sum(v*v for v in state[:3]));v=math.sqrt(sum(x*x for x in state[3:]));assert r>0 and v>0
 ra=math.atan2(state[1],state[0]);dec=math.atan2(state[2],math.hypot(*state[:2]));north=[-math.sin(dec)*math.cos(ra),-math.sin(dec)*math.sin(ra),math.cos(dec)];east=[-math.sin(ra),math.cos(ra),0]
 az=math.atan2(sum(a*b for a,b in zip(east,state[3:])),sum(a*b for a,b in zip(north,state[3:])))
 radial=sum(a*b for a,b in zip(state[:3],state[3:]))/(r*v);fpa=math.acos(max(-1.,min(1.,radial)))
 return dict(Cartesian=list(state),SphericalAZFPA=[r,math.degrees(ra)%360,math.degrees(dec),v,math.degrees(az)%360,math.degrees(fpa)],SphericalRADEC=[r,math.degrees(ra)%360,math.degrees(dec),v,math.degrees(math.atan2(state[4],state[3]))%360,math.degrees(math.atan2(state[5],math.hypot(state[3],state[4])))])
def cartesian(kind,values):
 if kind=='Cartesian':return list(values)
 r,ra,dec,speed,a,b=values;ra,dec,a,b=map(math.radians,[ra,dec,a,b]);radial=[math.cos(dec)*math.cos(ra),math.cos(dec)*math.sin(ra),math.sin(dec)]
 if kind=='SphericalRADEC':velocity=[speed*math.cos(b)*math.cos(a),speed*math.cos(b)*math.sin(a),speed*math.sin(b)]
 else:
  north=[-math.sin(dec)*math.cos(ra),-math.sin(dec)*math.sin(ra),math.cos(dec)];east=[-math.sin(ra),math.cos(ra),0]
  velocity=[speed*(math.cos(b)*radial[i]+math.sin(b)*(math.cos(a)*north[i]+math.sin(a)*east[i])) for i in range(3)]
 return [r*x for x in radial]+velocity
