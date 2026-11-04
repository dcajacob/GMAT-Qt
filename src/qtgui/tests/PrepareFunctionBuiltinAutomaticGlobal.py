"""Extend preserved positive builtin recipes with independent main-scope inputs."""
import copy,json,math,re
from pathlib import Path

def generate(repo):
    directory=repo/'src/qtgui/tests'
    strict=json.loads((directory/'NasaFunctionBuiltinScopeCases.json.in').read_text())
    legacy=json.loads((directory/'NasaFunctionBuiltinCases.json').read_text())
    cases=[]
    for original in strict['cases']:
        case=copy.deepcopy(original);case['original_scope_case']=True;cases.append(case)
    for original in legacy['cases']:
        if 'expected_failure'in original:continue
        case=copy.deepcopy(original);case['original_scope_case']=False
        declarations=case['declarations'].replace('NativeHelper','CallerHelper')
        execution=case['execution'].replace('NativeHelper','CallerHelper')
        expected=copy.deepcopy(case['expected']);key=case['key']
        if key=='QuaternionProduct':
            declarations=declarations.replace('Qx(1,1) = 1;','Qx(1,1) = -1;').replace('Qn(1,3) = 1.7320508075688772;','Qn(1,3) = -1.7320508075688772;')
            expected=[-.5,.5,-.5,.5,-.5,.5,.5,.5,0,0,math.sqrt(3)/2,.5]
        elif key=='QuaternionRotation':
            declarations=declarations.replace('V(1,1) = 3;','V(1,1) = 4;').replace('V(1,2) = -2;','V(1,2) = -1;').replace('V(1,3) = 5;','V(1,3) = 6;')
            expected=[1,4,6,1,4,6,4,1,-6]
        elif key=='QuaternionDCM':
            declarations=declarations.replace('Qx(1,1) = 2;','Qx(1,1) = 0;').replace('Qx(1,2) = 0;','Qx(1,2) = 2;')
            expected=expected[:18]+[-1,0,0,0,1,0,0,0,-1]
        elif key in {'Sign','SignAlias'}:
            execution=execution.replace('CallerHelper(-7.25)','CallerHelper(7.25)');expected=[1,0,1,-1,1]
        elif key in {'StringNumber','StringNumberAlias'}:
            execution=execution.replace("CallerHelper('-1234.5')","CallerHelper('2468.25')");expected=[2468.25,125,0]
        elif key in {'NumberString','NumberStringAlias'}:
            execution=execution.replace('CallerHelper(-1234.5)','CallerHelper(987.25)');expected='987.250000'
        elif key=='TimeConversion':
            execution=execution.replace('01 Jan 2000 12:00:00.000','01 Jan 2000 12:01:00.000');expected='01 Jan 2000 12:01:32.184'
        elif key=='RandomSeed':
            execution=execution.replace('12345','777').replace('54321','987')
        elif key in {'TimedPause','TimedPauseAlias'}:
            execution=execution.replace('CallerHelper(0.02)','CallerHelper(0.06)')
        elif key=='FixedRotation':
            execution=execution.replace('EarthMJ2000Eq','EarthMJ2000Ec')
            epsilon=84381.448*math.pi/(180*3600);c,s=math.cos(epsilon),math.sin(epsilon)
            expected=[1,0,0,0,c,-s,0,s,c]+[0]*9
        elif key!='SystemClock':raise ValueError('Unmapped positive builtin '+key)
        declarations+='Create Variable MainSentinel;\nMainSentinel = 29;\n'
        objects=[];arrays={}
        for line in declarations.splitlines():
            match=re.fullmatch(r'Create (\w+) (.+);',line)
            if not match:continue
            for name,rows,columns in re.findall(r'(\w+)(?:\[(\d+),(\d+)\])?',match[2]):
                objects.append(name)
                if match[1]=='Array':arrays[name]=(int(rows),int(columns))
        runtime={'MainSentinel':dict(type='Variable',value=29)}
        for name,(rows,columns)in arrays.items():
            # Actual input arrays remain distinct across root/function scopes.
            # Scratch result arrays are independently checked through outputs.
            if name in {'P','R','D'}:continue
            values=[0.]*(rows*columns)
            for r,c,value in re.findall(re.escape(name)+r'\((\d+),(\d+)\) = ([^;]+);',declarations):
                values[(int(r)-1)*columns+int(c)-1]=float(value)
            runtime[name]=dict(type='Array',rows=rows,columns=columns,values=values)
        case.update(main_declarations=declarations,main_execution=execution,main_expected=expected,
                    caller_objects=objects,main_runtime_expected=runtime)
        cases.append(case)
    for case in cases:
        case['private_primary_builtin_calls_per_block']=len(re.findall(r'\bNativeHelper\(',case['execution']))
        case['main_primary_builtin_calls_per_block']=len(re.findall(r'\bCallerHelper\(',case['main_execution']))
        assert case['private_primary_builtin_calls_per_block']>0 and case['main_primary_builtin_calls_per_block']>0
    assert len(cases)==18 and len({c['builtin_type']for c in cases})==18
    return dict(schema_version=1,scope='All18 builtin resource spellings;actual automatic shared helper identity and positive main/direct/nested use. Original private recipes and3 strict scope profiles preserved;no whole-domain or exact-row credit.',cases=cases)

if __name__=='__main__':
    repo=Path(__file__).resolve().parents[3]
    output=repo/'src/qtgui/tests/NasaFunctionBuiltinAutomaticGlobalCases.json.in'
    output.write_text(json.dumps(generate(repo),indent=2,ensure_ascii=False)+'\n')
