"""Run affected original/controlled GUI exports with independent UTC creation checks."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import uuid

import NasaGuiCoverage as nasa
from CcsdsOemB1Oracle import validate
from RunOemCreationClock import creation_check


def main():
    repo=Path(__file__).resolve().parents[3]
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--visible',action='store_true')
    parser.add_argument('--filter',default='^QtGui.Nasa(EphemerisSamplingShortArc|OemRecordSupportOneRecord)$')
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    names={'src/qtgui/CMakeLists.txt','src/qtgui/tests/NasaEphemerisSamplingTests.cpp',
           'src/qtgui/tests/NasaEphemerisSamplingCases.json','src/qtgui/tests/NasaOemRecordSupportTests.cpp',
           'src/qtgui/tests/NasaOemRecordSupportCases.json','src/qtgui/tests/RunOemCreationClock.py',
           'src/qtgui/tests/CcsdsOemB1Oracle.py'}
    names.update(str(p.relative_to(repo))for root in ['src/base','src/gmatutil','plugins']
                 for p in(repo/root).rglob('*')if p.is_file()and p.suffix in {'.cpp','.hpp'})
    review=repo/'build/gui-acceptance'/('oem-creation-clock-'+uuid.uuid4().hex[:12]+'-review.json')
    review.parent.mkdir(exist_ok=True)
    review.write_text(json.dumps(dict(sources=nasa.inventory(repo)['sources'],reviews=[],
        reference_sources={n:hashlib.sha256((repo/n).read_bytes()).hexdigest()for n in sorted(names)},
        scope='Affected ShortArc approved expectation and controlled OneRecord;UTC creation clock only,no whole-row credit.'),indent=2)+'\n')
    command=[sys.executable,str(repo/'src/qtgui/tests/RunAcceptance.py'),'--filter',args.filter,
        '--nasa-review',str(review.relative_to(repo)),'--max-screenshots','0',
        '--max-checkpoints','500','--pause-ms','0','--output',str(args.output)]
    if args.visible:command+=['--visible','--display-route','nested','--display-scale','auto','--start-minimized']
    code=subprocess.call(command,cwd=repo)
    receipt=json.loads((args.output/'results.json').read_text());observations=[]
    for result in receipt['results']:
        if result['status']=='not-run':continue
        path=args.output/result['name']/'mission.oem'
        if not path.exists():continue
        raw=path.read_bytes()
        allowed=['GMAT-Qt-OEM-record-support']if result['name']=='QtGui.NasaOemRecordSupportOneRecord'else[]
        finding=validate(raw,icd_object_ids=allowed)
        clock=creation_check(raw,receipt['started_utc'],receipt['finished_utc'])
        observations.append(dict(test=result['name'],creation_clock=clock,format_scope=finding,
            legacy_identifier_agreement_invented=False))
    output=args.output/'independent-UTC-creation-audit.json';assert not output.exists()
    output.write_text(json.dumps(dict(observations=observations,new_requirement_credit=0,
        whole_row_qualified=False),indent=2)+'\n')
    return code or (1 if any(x['creation_clock']['status']!='passed'for x in observations)else 0)


if __name__=='__main__':
    raise SystemExit(main())
