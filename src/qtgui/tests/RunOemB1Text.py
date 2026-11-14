"""Run native GUI B-1 text boundaries and check accepted/rejected output independently."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import uuid

import NasaGuiCoverage as nasa
from CcsdsOemB1Oracle import ConformanceError,validate
from RunOemCreationClock import creation_check


def main():
    repo=Path(__file__).resolve().parents[3]
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--visible',action='store_true')
    parser.add_argument('--filter',default='^QtGui.NasaOemTextFields')
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    names={'src/qtgui/CMakeLists.txt','src/qtgui/tests/NasaOemTextFieldsTests.cpp',
           'src/qtgui/tests/NasaOemTextFieldsCases.json','src/qtgui/tests/CcsdsOemB1Oracle.py',
           'src/qtgui/tests/RunOemCreationClock.py'}
    names.update(str(p.relative_to(repo))for root in ['src/base','src/gmatutil','plugins']
                 for p in(repo/root).rglob('*')if p.is_file()and p.suffix in {'.cpp','.hpp'})
    review=repo/'build/gui-acceptance'/('oem-b1-text-'+uuid.uuid4().hex[:12]+'-review.json')
    review.parent.mkdir(exist_ok=True)
    review.write_text(json.dumps(dict(sources=nasa.inventory(repo)['sources'],reviews=[],
        reference_sources={n:hashlib.sha256((repo/n).read_bytes()).hexdigest()for n in sorted(names)},
        scope='Original B1 printable-text/254-character/non-null obligations;no original assertion or science waiver.'),indent=2)+'\n')
    command=[sys.executable,str(repo/'src/qtgui/tests/RunAcceptance.py'),'--filter',args.filter,
        '--nasa-review',str(review.relative_to(repo)),'--max-screenshots','0',
        '--max-checkpoints','500','--pause-ms','0','--output',str(args.output)]
    if args.visible:command+=['--visible','--display-route','nested','--display-scale','auto','--start-minimized']
    code=subprocess.call(command,cwd=repo)
    audit_code=audit_run(args.output, repo/'src/qtgui/tests/NasaOemTextFieldsCases.json')
    return code or audit_code


def audit_run(root,catalog_path):
    root=Path(root)
    catalog=json.loads(Path(catalog_path).read_text())
    receipt=json.loads((root/'results.json').read_text());observations=[]
    for case in catalog['cases']:
        case_root=root/('QtGui.NasaOemTextFields'+case['key']);path=case_root/'mission.oem'
        if not case_root.exists():continue
        proof=json.loads((case_root/'proof.json').read_text())if(case_root/'proof.json').exists()else{}
        rejected=bool(proof.get('expected_rejection'))
        if not path.exists():
            observations.append(dict(case=case['key'],status='rejected-before-file-creation'if rejected else'no-output-retained',
                expected_rejection=case['reject_export'],actual_rejection_proved=rejected))
            continue
        raw=path.read_bytes()
        try:finding=validate(raw,icd_object_ids=catalog['test_interface_profile']['object_ids'])
        except ConformanceError as error:finding=dict(status='failed',clause=error.clause,line=error.line,message=error.message)
        if rejected:
            lines=raw.splitlines()
            assert all(len(line)<=254 and all(32<=c<=126 for c in line)for line in lines)
            assert not any(line.startswith(b'OBJECT_ID')for line in lines),'Invalid field reached partial output'
        clock=creation_check(raw.partition(b'META_START')[0],receipt['started_utc'],receipt['finished_utc'])
        observations.append(dict(case=case['key'],expected_rejection=case['reject_export'],
            actual_rejection_proved=rejected,format_scope=finding,creation_clock=clock,
            raw_sha256=hashlib.sha256(raw).hexdigest()))
    output=root/'independent-B1-text-audit.json';assert not output.exists()
    output.write_text(json.dumps(dict(test_interface_profile=catalog['test_interface_profile'],
        observations=observations,new_requirement_credit=0,whole_row_qualified=False),indent=2)+'\n')
    independent_failed=any(not x.get('actual_rejection_proved')and
        (x.get('format_scope',{}).get('status')!='passed'or x.get('creation_clock',{}).get('status')!='passed')
        for x in observations)
    return 1 if independent_failed else 0


if __name__=='__main__':
    raise SystemExit(main())
