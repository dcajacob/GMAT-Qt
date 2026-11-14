"""Independent complex-step potential gradients and fifth-order trajectories.

Consumes preserved GUI results and explicit shared orientation matrices. It
imports no GMAT code. C++ uses dual numbers/RK4; this audit uses holomorphic
potential evaluation/complex-step derivatives and a different RK tableau.
"""
import cmath
import json
import math
from pathlib import Path


def field_from_cof(path):
    field={'coefficients':[]}
    for line in Path(path).read_text().splitlines():
        if line.startswith('POTFIELD'):
            words=line[14:].split()
            assert float(words[3])==1
            field.update(maximum=int(line[8:11]),mu=float(words[1])/1e9,radius=float(words[2])/1000)
        elif line.startswith('RECOEF'):
            field['coefficients'].append((int(line[8:11]),int(line[11:14]),float(line[17:38]),float(line[38:59]) if line[38:59].strip() else 0))
    assert field['mu']>0 and field['radius']>0 and field['coefficients']
    return field


def potential(position,field,degree,order):
    x,y,z=position
    radius=cmath.sqrt(x*x+y*y+z*z)
    latitude=z/radius
    real,imag=[1+0j],[0j]
    polynomials=[[0j]*(degree+1) for _ in range(degree+1)]
    polynomials[0][0]=1
    for m in range(1,degree+1):
        real.append((x*real[-1]-y*imag[-1])/radius)
        imag.append((y*real[m-1]+x*imag[-1])/radius)
        polynomials[m][m]=(2*m-1)*polynomials[m-1][m-1]
    for m in range(degree+1):
        if m<degree:polynomials[m+1][m]=(2*m+1)*latitude*polynomials[m][m]
        for n in range(m+2,degree+1):
            polynomials[n][m]=((2*n-1)*latitude*polynomials[n-1][m]-(n+m-1)*polynomials[n-2][m])/(n-m)
    terms=[]
    for n,m,c,s in field['coefficients']:
        if n>degree or m>order:continue
        normalization=math.sqrt((2 if m else 1)*(2*n+1)*math.exp(math.lgamma(n-m+1)-math.lgamma(n+m+1)))
        terms.append((field['radius']/radius)**n*normalization*polynomials[n][m]*(c*real[m]+s*imag[m]))
    return field['mu']*(1+sum(terms))/radius


def body_force(position,field,degree,order):
    h=1e-18*math.sqrt(sum(x*x for x in position))
    result=[]
    for i in range(3):
        sample=list(position);sample[i]+=1j*h
        result.append(potential(sample,field,degree,order).imag/h)
    return result


def rotate(matrix,vector,transpose=False):
    return [math.fsum((matrix[j][i] if transpose else matrix[i][j])*vector[j] for j in range(3)) for i in range(3)]


def orientation(samples,fraction):
    count=len(samples)
    weights=[math.prod((fraction*(count-1)-j)/(i-j) for j in range(count) if j!=i) for i in range(count)]
    return [[math.fsum(weight*sample[i][j] for weight,sample in zip(weights,samples)) for j in range(3)] for i in range(3)]


def force(position,field,degree,order,rotation):
    return rotate(rotation,body_force(rotate(rotation,position),field,degree,order),transpose=True)


def integrate(initial,seconds,field,degree,order,rotations,steps):
    # Dormand-Prince fifth-order solution, fixed steps and separate refinement.
    c=[0,1/5,3/10,4/5,8/9,1]
    a=[[],[1/5],[3/40,9/40],[44/45,-56/15,32/9],
       [19372/6561,-25360/2187,64448/6561,-212/729],
       [9017/3168,-355/33,46732/5247,49/176,-5103/18656]]
    b=[35/384,0,500/1113,125/192,-2187/6784,11/84]
    state=list(initial);h=seconds/steps
    for n in range(steps):
        derivatives=[]
        for j in range(6):
            sample=[state[i]+h*sum(a[j][m]*derivatives[m][i] for m in range(j)) for i in range(6)]
            accel=force(sample[:3],field,degree,order,orientation(rotations,(n+c[j])/steps))
            derivatives.append(sample[3:]+accel)
        state=[state[i]+h*sum(b[j]*derivatives[j][i] for j in range(6)) for i in range(6)]
    return state


def self_check():
    # Independent Cartesian closed form for every degree-two C/S term.
    model=dict(mu=398650.,radius=6300.,coefficients=[(2,0,-.004,0),(2,1,.002,-.003),(2,2,.005,.004)])
    root5,root15=math.sqrt(5),math.sqrt(15)
    q=[[-root5/2*(-.004)+root15/2*.005,root15/2*.004,root15/2*.002],
       [root15/2*.004,-root5/2*(-.004)-root15/2*.005,root15/2*(-.003)],
       [root15/2*.002,root15/2*(-.003),root5*(-.004)]]
    positions=[[7500,0,0],[-7500,0,0],[0,7500,0],[0,-7500,0],[0,0,7500],[0,0,-7500],[5100,-3800,2500]]
    maximum=0
    for position in positions:
        r=math.sqrt(sum(x*x for x in position));qr=rotate(q,position);quadratic=sum(a*b for a,b in zip(position,qr))
        wanted=[-model['mu']*x/r**3+model['mu']*model['radius']**2*(2*y/r**5-5*quadratic*x/r**7) for x,y in zip(position,qr)]
        actual=body_force(position,model,2,2)
        maximum=max(maximum,max(abs(a-b) for a,b in zip(actual,wanted)))
    assert maximum<2e-17,maximum
    return dict(closed_form_all_five_quadrupole_coefficients=True,poles_and_axes=7,max_force_error=maximum)


def audit(directory,proof_path=None):
    directory=Path(directory);proof=json.loads((Path(proof_path) if proof_path else directory/'proof.json').read_text())
    assert proof['outcome']==('candidate' if proof_path else 'passed') and proof['body'] in ('Earth','Ceres')
    samples=json.loads((directory/'observations.json').read_text())
    assert samples==proof['observations'] and len(samples)==proof['trial_count']==10
    assert proof['resource_applies']==10 and proof['gravity_dialogs']==20 and proof['force_grid_points']==70
    assert [s['trial'] for s in samples]==['Monopole','Zonal','Tesseral','Sectoral','North','South','DegreeFour','OrderTruncation','DefaultFile','FullFile']
    checks=[];fields={False:field_from_cof(directory/'synthetic.cof')}
    if proof['body']=='Earth':fields[True]=field_from_cof(directory/'JGM2.cof')
    for sample in samples:
        model=fields[sample['shipped_model']];degree,order=sample['degree'],sample['order']
        assert 0<=order<=degree<=model['maximum'] and sample['mu']==model['mu'] and sample['radius']==model['radius']
        assert abs(sample['seconds']-10)<1e-9 and abs(sample['reported_seconds']-10)<1e-4
        args=[sample['initial_state'],sample['seconds'],model,degree,order,sample['rotation_samples']]
        coarse=integrate(*args,steps=16);expected=integrate(*args,steps=32)
        errors=[abs(a-b) for a,b in zip(expected,sample['actual_state'])]
        refinement=[abs(a-b) for a,b in zip(expected,coarse)]
        assert all(error<(1e-6 if i<3 else 1e-9) for i,error in enumerate(errors)),(sample['trial'],errors)
        assert all(error<(1e-9 if i<3 else 1e-12) for i,error in enumerate(refinement)),(sample['trial'],refinement)
        grids=[]
        assert len(sample['grid'])==7
        for point in sample['grid']:
            wanted=force(point['actual_local_position'],model,degree,order,sample['rotation_samples'][-1])
            difference=[abs(a-b) for a,b in zip(wanted,point['native_acceleration'])]
            assert max(difference)<1e-11,(sample['trial'],point,difference)
            grids.append(dict(expected=wanted,errors=difference))
        # A true central 1/r field has a closed-form force for all axes/poles.
        if degree==0:
            for point in sample['grid']:
                position=point['actual_local_position'];r=math.sqrt(sum(x*x for x in position))
                wanted=[-model['mu']*x/r**3 for x in position]
                assert max(abs(a-b) for a,b in zip(wanted,point['native_acceleration']))<1e-11
        checks.append(dict(trial=sample['trial'],independent_state=expected,state_errors=errors,DP5_refinement_errors=refinement,grid=grids))
    return dict(body=proof['body'],self_check=self_check(),missions=10,force_grid_points=70,checks=checks,
                shared_orientation_inputs=True,independent_orientation_qualified=False,
                independent_potential_complex_step=True,independent_fifth_order_integration=True)


if __name__=='__main__':
    import argparse
    parser=argparse.ArgumentParser()
    parser.add_argument('--directory',type=Path)
    parser.add_argument('--proof',type=Path)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    result=audit(args.directory,args.proof) if args.directory else self_check()
    args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({key:value for key,value in result.items() if key!='checks'}))
