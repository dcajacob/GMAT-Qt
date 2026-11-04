"""Replay original absolute epoch failures and affected equation/epoch controls."""
import argparse,hashlib,json,re,subprocess,sys,uuid
from pathlib import Path
import NasaGuiCoverage as nasa


def main():
    r=Path(__file__).resolve().parents[3];p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--visible',action='store_true');p.add_argument('--output',type=Path);a=p.parse_args()
    types=['TotalPowerAvailable','RequiredBusPower','ThrustPowerAvailable']
    names=['QtGui.NasaFunctionPowerParameterBinding'+key+suffix for suffix in ['', 'SelfReferencedEpoch'] for key in types]+['QtGui.NasaFunctionParameterBinding'+key for key in ['A1ModJulian','CurrA1MJD','UTCGregorian','DryMass']]+['QtGui.NasaAssignmentSettableParameter','QtGui.NasaAssignmentResourceExpression']
    refs={'src/qtgui/CMakeLists.txt','src/qtgui/tests/RunFunctionAbsoluteEpochAssignments.py'}
    for stem in ['NasaFunctionPowerParameter','NasaFunctionParameterBinding']:
        refs.add('src/qtgui/tests/'+stem+'Cases.json')
    refs.update('src/qtgui/tests/'+name for name in ['NasaFunctionPowerParameterBindingTests.cpp','NasaFunctionParameterBindingTests.cpp','NasaAssignmentTests.cpp','NasaAssignmentCases.json','AuditFunctionPowerParameterBindings.py','AuditFunctionParameterBindings.py'])
    for base in ['src/base','src/gmatutil','plugins']:
        refs.update(str(v.relative_to(r)) for v in (r/base).rglob('*') if v.is_file() and v.suffix in {'.cpp','.hpp'})
    d=r/'build/gui-acceptance';identity=uuid.uuid4().hex[:12];review=d/('absolute-epoch-'+identity+'-review.json')
    review.write_text(json.dumps(dict(sources=nasa.inventory(r)['sources'],reviews=[],reference_sources={n:hashlib.sha256((r/n).read_bytes()).hexdigest() for n in sorted(refs)}),indent=2)+'\n')
    cmd=[sys.executable,str(r/'src/qtgui/tests/RunAcceptance.py'),'--filter','^(?:'+'|'.join(re.escape(n) for n in names)+')$','--max-screenshots','0','--max-checkpoints','500','--pause-ms','0','--nasa-review',str(review.relative_to(r)),'--output',str(a.output or d/('function-absolute-epoch-'+identity))]
    if a.visible:cmd+=['--visible','--start-minimized']
    return subprocess.call(cmd,cwd=r)


if __name__=='__main__':
    raise SystemExit(main())
