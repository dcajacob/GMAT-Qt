"""Independent instantaneous rotating-frame Cartesian recipes; no GMAT imports."""
TYPES=['X','Y','Z','VX','VY','VZ','Cartesian']
SCOPES=['root','private','shared']
OMEGA=1/4096
BASES=[[7000,128,256,1.25,7.5,.75],[7500,256,512,2.5,8,1.5],[8000,512,1024,3.75,8.5,2.25]]
STEPS=[[0]*6,[16,32,64,.125,.25,.5],[32,64,128,.25,.5,1]]
PRIMARY=[131072,262144,524288,0,0,0]
RELATIVE_GUIDES=[[0,8192,0,-2,0,0],[0,0,16384,0,-4,0],[-32768,0,0,0,-8,0]]
ROTATIONS=[[[0,1,0],[-1,0,0],[0,0,1]],[[0,0,1],[0,-1,0],[1,0,0]],[[-1,0,0],[0,-1,0],[0,0,1]]]
DERIVATIVES=[[[-OMEGA,0,0],[0,-OMEGA,0],[0,0,0]],[[0,-OMEGA,0],[0,0,-OMEGA],[0,0,0]],[[0,-OMEGA,0],[OMEGA,0,0],[0,0,0]]]
IDENTITY=[[1,0,0],[0,1,0],[0,0,1]]
ZERO=[[0]*3 for _ in range(3)]


def mv(m,v):return [sum(m[i][j]*v[j] for j in range(3)) for i in range(3)]
def transpose(m):return [list(row) for row in zip(*m)]
def forward(state,b,d):
    p=mv(b,state[:3]);v=mv(b,state[3:]);transport=mv(d,state[:3]);return p+[v[i]+transport[i] for i in range(3)]
def inverse(state,b,d):
    p=mv(transpose(b),state[:3]);transport=mv(d,p);return p+mv(transpose(b),[state[i+3]-transport[i] for i in range(3)])


def recipe(key,mode,scope,calls=0):
    i=SCOPES.index(scope);inside=calls>0;b=ROTATIONS[i] if mode=='Frame' else IDENTITY;d=DERIVATIVES[i] if mode=='Frame' else ZERO
    before=[BASES[i][j]+(calls*STEPS[i][j] if inside else 0) for j in range(6)];framed=forward(before,b,d);index=TYPES.index(key) if key!='Cartesian' else -1;script_delta=(.5 if index<3 else .03125) if index>=0 else 0
    after=framed[:]
    if inside and index>=0:after[index]+=script_delta
    inertial=inverse(after,b,d);native_delta=(.25 if index<3 else .015625) if index>=0 else 0;changed=after[:]
    if index>=0:changed[index]+=native_delta
    primary=PRIMARY[:];guide=[PRIMARY[j]+RELATIVE_GUIDES[i][j] for j in range(6)]
    return dict(before_script_inertial=before,after_script_inertial=inertial,after_script_framed=after,expected=after if index<0 else after[index],script_delta=script_delta,native_delta=native_delta,changed_framed=changed,changed_inertial=inverse(changed,b,d),frame_from_base=b,frame_from_base_dot=d,frame_to_base=transpose(b),frame_to_base_dot=transpose(d),primary_state=primary,guide_state=guide,relative_guide_state=RELATIVE_GUIDES[i])
