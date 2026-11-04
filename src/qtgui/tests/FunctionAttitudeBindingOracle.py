"""Independent initial-epoch attitude recipes for live owner-binding probes."""
import math

SCOPES=('root','private','shared')
QUATERNION_INPUTS=((1,2,3,6),(2,3,5,7),(3,4,6,9))
ANGULAR_INPUTS=((.125,.25,.375),(.25,.375,.5),(.375,.5,.625))
ALIGNMENT_INPUTS=((1,2,3),(2,3,5),(3,5,7))
CONSTRAINT_INPUTS=((2,-1,1),(3,-2,2),(5,-3,3))
MODEL_NAMES=('CoordinateSystemFixed','Spinner','ThreeAxisKinematic')
REFERENCE_BODIES=('Earth','Mars','Jupiter')
CONSTRAINT_TYPES=('OrbitNormal','Velocity','OrbitNormal')
ORIENTATION_TYPES=[f'DCM{i}{j}' for i in range(1,4) for j in range(1,4)]+['DirectionCosineMatrix']+[f'EulerAngle{i}' for i in range(1,4)]+[f'MRP{i}' for i in range(1,4)]+[f'Q{i}' for i in range(1,5)]+['Quaternion']
RATE_TYPES=['AngularVelocity'+v for v in 'XYZ']+[f'EulerAngleRate{i}' for i in range(1,4)]
NADIR_VECTOR_TYPES=[v+'Vector'+a for v in ['BodyAlignment','BodyConstraint'] for a in 'XYZ']
STRING_TYPES=['Attitude','AttitudeReferenceBody','AttitudeConstraintType']
TYPES=ORIENTATION_TYPES+RATE_TYPES+NADIR_VECTOR_TYPES+STRING_TYPES
WRITABLE_TYPES=[v for v in TYPES if v not in ['DirectionCosineMatrix',*STRING_TYPES,'Q1','Q2','Q3','Q4']]


def recipe(scope,key):
    i=SCOPES.index(scope);raw=QUATERNION_INPUTS[i];n=math.sqrt(sum(v*v for v in raw));q=[v/n for v in raw];x,y,z,w=q
    matrix=[1-2*(y*y+z*z),2*(x*y+z*w),2*(x*z-y*w),2*(x*y-z*w),1-2*(x*x+z*z),2*(y*z+x*w),2*(x*z+y*w),2*(y*z-x*w),1-2*(x*x+y*y)]
    yaw=math.atan2(matrix[1],matrix[0]);pitch=math.asin(-matrix[2]);roll=math.atan2(matrix[5],matrix[8]);euler=[math.degrees(v) for v in [yaw,pitch,roll]]
    angular=list(ANGULAR_INPUTS[i]);a,b,c=angular
    rates=[(b*math.sin(roll)+c*math.cos(roll))/math.cos(pitch),b*math.cos(roll)-c*math.sin(roll),a+(b*math.sin(roll)+c*math.cos(roll))*math.tan(pitch)]
    values={f'DCM{r+1}{c+1}':matrix[3*r+c] for r in range(3) for c in range(3)}
    values.update(DirectionCosineMatrix=matrix,Quaternion=q,Attitude=MODEL_NAMES[i],AttitudeReferenceBody=REFERENCE_BODIES[i],AttitudeConstraintType=CONSTRAINT_TYPES[i])
    for j in range(3):
        values[f'EulerAngle{j+1}']=euler[j];values[f'MRP{j+1}']=q[j]/(1+w);values['AngularVelocity'+'XYZ'[j]]=angular[j];values[f'EulerAngleRate{j+1}']=rates[j];values['BodyAlignmentVector'+'XYZ'[j]]=ALIGNMENT_INPUTS[i][j];values['BodyConstraintVector'+'XYZ'[j]]=CONSTRAINT_INPUTS[i][j]
    for j in range(4):values[f'Q{j+1}']=q[j]
    model='NadirPointing' if key in NADIR_VECTOR_TYPES+STRING_TYPES[1:] else MODEL_NAMES[i] if key=='Attitude' else 'ThreeAxisKinematic'
    display='DirectionCosineMatrix' if key.startswith('DCM') else 'MRPs' if key.startswith('MRP') else 'EulerAngles' if key.startswith('EulerAngle') and not key.startswith('EulerAngleRate') else 'Quaternion'
    return dict(model=model,quaternion=q,matrix=matrix,euler=euler,angular_velocity=angular,euler_rates=rates,alignment=list(ALIGNMENT_INPUTS[i]),constraint=list(CONSTRAINT_INPUTS[i]),reference_body=REFERENCE_BODIES[i],constraint_type=CONSTRAINT_TYPES[i],display=display,rate_display='EulerAngleRates' if key.startswith('EulerAngleRate') else 'AngularVelocity',expected=values[key])
