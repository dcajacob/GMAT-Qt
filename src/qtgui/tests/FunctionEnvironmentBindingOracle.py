"""Independent geometry for the remaining23 scoped Parameter-owner routes.

Body-fixed rotations,body hour angle and solar ephemeris are explicit shared
inputs. They are never promoted to independent orientation/ephemeris truth.
"""
import math

RATES=['SMADot','EquinoctialHDot','EquinoctialKDot','EquinoctialPDot','EquinoctialQDot','TLONGDot']
PLANET=['MHA','Longitude','Latitude','LST','BetaAngle']
ACCEL=['Acceleration','AccelerationX','AccelerationY','AccelerationZ']
TORQUE=['BurnTorque','GravityTorque','SRPTorque','TotalTorque']
TYPES=RATES+PLANET+['OrbitErrorCovariance','OrbitTime']+ACCEL+['AtmosDensity']+TORQUE+['SurfaceHeight']
SCOPES=['root','private','shared']
MU=398600.4415
RADIUS=6378.1363
FLUX=1361.
AU=149597870.691
LIGHT=299792458.

def dot(a,b):return math.fsum(x*y for x,y in zip(a,b))
def norm(a):return math.sqrt(dot(a,a))
def cross(a,b):return [a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]]
def sub(a,b):return [x-y for x,y in zip(a,b)]
def wrap(value,period):return value%period
def state(scope,calls=0):
    i=SCOPES.index(scope);step=0 if i==0 else calls
    return [7000+500*i+25*i*step,-1200-100*i-5*i*step,3400+200*i+10*i*step,.2+.1*i,7-.2*i,-.3-.1*i]
def covariance(scope,calls=0):
    i=SCOPES.index(scope);factor=1+i*.25+(0 if i==0 else calls*.05*i)
    # A positive diagonal plus vv^T supplies a correlated SPD literal.
    vector=[.02,-.01,.03,.0002,-.0001,.0003]
    diagonal=[.01,.02,.03,1e-6,2e-6,3e-6]
    return [[factor*(vector[r]*vector[c]+(diagonal[r]if r==c else 0))for c in range(6)]for r in range(6)]
def plates(scope):
    level=SCOPES.index(scope)+1;values=[]
    for axis in range(3):
        for sign in (-1,1):
            normal=[0,0,0];normal[axis]=sign
            values.append(dict(normal=normal,area=level*(axis+2),location=[.4*level,.7+.1*axis,1.1-.1*axis],specular=0.,diffuse=0.,lit_fraction=1.))
    return values
def expected(kind,scope,calls=0,inputs=None):
    assert kind in TYPES
    inputs=inputs or {};s=state(scope,calls);r,v=s[:3],s[3:];radius=norm(r);h=cross(r,v);i=SCOPES.index(scope)
    if kind in RATES:return math.degrees(norm(h)/radius**2)if kind=='TLONGDot'else 0.
    if kind in ACCEL:
        a=[-MU*x/radius**3 for x in r]
        return norm(a)if kind=='Acceleration'else a[ACCEL.index(kind)-1]
    if kind=='SurfaceHeight':return radius-RADIUS
    if kind=='AtmosDensity':return 1e-12*math.exp(-(radius-RADIUS)/100.)*1e9
    if kind=='OrbitErrorCovariance':return covariance(scope,calls)
    if kind in ('MHA','Longitude','Latitude','LST'):
        rotation=inputs['inertial_to_fixed'];fixed=[dot(row,r)for row in rotation]
        longitude=math.degrees(math.atan2(fixed[1],fixed[0]));latitude=math.degrees(math.atan2(fixed[2],math.hypot(fixed[0],fixed[1])))
        return {'MHA':inputs['body_hour_angle_degrees'],'Longitude':longitude,'Latitude':latitude,'LST':wrap(inputs['body_hour_angle_degrees']+longitude,360)}[kind]
    sun=inputs.get('sun_relative_earth')
    if kind=='BetaAngle':return math.degrees(math.asin(max(-1.,min(1.,dot(h,sun)/(norm(h)*norm(sun))))))
    if kind=='OrbitTime':return wrap(math.degrees(math.atan2(r[1],r[0])-math.atan2(-sun[1],-sun[0])),360)/15
    assert kind in TORQUE
    # Body axes are explicitly aligned to EarthMJ2000Eq,CM=0,inertia diagonal.
    diagonal=[100*(i+1),200*(i+1),300*(i+1)]
    gradient=[3*MU*x/radius**5 for x in cross(r,[a*b for a,b in zip(diagonal,r)])]
    thrust=8+3*i;burn=cross([.4*(i+1),.7,.8],[thrust,0,0])
    solar=[0.,0.,0.]
    if kind in ('SRPTorque','TotalTorque'):
        ray=sub(r,sun);distance=norm(ray);direction=[x/distance for x in ray]
        # The fixture must be fully illuminated;the native observation audits it.
        assert inputs['illumination_fraction']==1
        for plate in plates(scope):
            cosine=max(0.,-dot(plate['normal'],direction))
            force=[FLUX/LIGHT*plate['area']*cosine*(AU/distance)**2*x for x in direction]
            moment=cross(plate['location'],force);solar=[a+b for a,b in zip(solar,moment)]
    return {'BurnTorque':burn,'GravityTorque':gradient,'SRPTorque':solar,'TotalTorque':[a+b+c for a,b,c in zip(burn,gradient,solar)]}[kind]

def bound(kind,value):
    if kind=='AtmosDensity':return max(1e-20,abs(value)*5e-12)
    if kind in RATES:return 1e-10
    if kind in ACCEL:return 1e-12
    if kind in TORQUE:return 2e-11
    if kind=='OrbitErrorCovariance':return 1e-13
    return 1e-8

def self_check():
    assert len(TYPES)==len(set(TYPES))==23
    checks=0
    for scope in SCOPES:
        for calls in range(4):
            s=state(scope,calls);a=[-MU*x/norm(s[:3])**3 for x in s[:3]]
            assert abs(norm(a)-MU/norm(s[:3])**2)<1e-17
            c=covariance(scope,calls)
            # Correlation need not imply diagonal dominance. Independently
            # factor the six leading Schur complements and require positive pivots.
            reduced=[row[:]for row in c]
            for k in range(6):
                pivot=reduced[k][k];assert pivot>0
                for row in range(k+1,6):
                    for column in range(k+1,6):reduced[row][column]-=reduced[row][k]*reduced[k][column]/pivot
            for kind in RATES+ACCEL+['SurfaceHeight','AtmosDensity','OrbitErrorCovariance']:
                expected(kind,scope,calls);checks+=1
    return dict(types=23,deterministic_scope_call_recipes=checks,scope='Recipe checks only;no native execution or requirement credit.')

if __name__=='__main__':
    import argparse,json
    from pathlib import Path
    p=argparse.ArgumentParser();p.add_argument('--input',type=Path);p.add_argument('--output',type=Path,required=True);args=p.parse_args()
    if args.input:
        request=json.loads(args.input.read_text());value=expected(request['type'],request['scope'],request['calls'],request['inputs'])
        flat=[x for row in value for x in row]if value and isinstance(value,list)and isinstance(value[0],list)else value
        result=dict(expected=flat,absolute_bound=bound(request['type'],value if not isinstance(value,list)else 0),input=request,oracle='independent scalar/Cartesian/moment geometry with explicit shared provider inputs',independent_orientation_ephemeris_qualified=False)
    else:result=self_check()
    args.output.write_text(json.dumps(result,indent=2)+'\n')
