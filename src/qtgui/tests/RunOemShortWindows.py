"""Run actual OEM explicit-window GUI cases and independent B-1 output checks."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import uuid

import NasaGuiCoverage as nasa
from CcsdsOemB1Oracle import ConformanceError, validate


def main():
    repo = Path(__file__).resolve().parents[3]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--visible', action='store_true')
    parser.add_argument('--filter', default='^QtGui.NasaOemShortWindow')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    names = {'src/qtgui/CMakeLists.txt', 'src/qtgui/tests/NasaOemRecordSupportTests.cpp',
             'src/qtgui/tests/NasaOemRecordSupportCases.json', 'src/qtgui/tests/NasaOemShortWindowCases.json',
             'src/qtgui/tests/CcsdsOemB1Oracle.py', 'src/qtgui/tests/NasaEphemerisSamplingTests.cpp',
             'src/qtgui/tests/NasaEphemerisSamplingCases.json'}
    names.update(str(p.relative_to(repo)) for root in ['src/base','src/gmatutil','plugins']
                 for p in (repo/root).rglob('*') if p.is_file() and p.suffix in {'.cpp','.hpp'})
    review = repo/'build/gui-acceptance'/('oem-short-windows-'+uuid.uuid4().hex[:12]+'-review.json')
    review.parent.mkdir(exist_ok=True)
    review.write_text(json.dumps(dict(sources=nasa.inventory(repo)['sources'],reviews=[],
        reference_sources={n:hashlib.sha256((repo/n).read_bytes()).hexdigest() for n in sorted(names)},
        scope='Explicit OEM window boundaries;unchanged original assertions and all qualification gates.'),indent=2)+'\n')
    command = [sys.executable,str(repo/'src/qtgui/tests/RunAcceptance.py'),'--filter',args.filter,
        '--nasa-review',str(review.relative_to(repo)),'--max-screenshots','0',
        '--max-checkpoints','500','--pause-ms','0','--output',str(args.output)]
    if args.visible:
        command += ['--visible','--display-route','nested','--display-scale','auto','--start-minimized']
    code = subprocess.call(command,cwd=repo)
    observations = []
    for catalog_name, prefix in [('NasaOemShortWindowCases.json','QtGui.NasaOemShortWindow'),
                                 ('NasaOemRecordSupportCases.json','QtGui.NasaOemRecordSupport')]:
        catalog = json.loads((repo/'src/qtgui/tests'/catalog_name).read_text())
        for case in catalog['cases']:
            path = args.output/(prefix+case['key'])/'mission.oem'
            if not path.exists():
                continue
            try:
                finding = validate(path.read_bytes(),icd_object_ids=[catalog['object_id']])
            except ConformanceError as error:
                finding = dict(status='failed',clause=error.clause,line=error.line,message=error.message)
            observations.append(dict(test=prefix+case['key'],file=str(path),finding=finding))
    output = args.output/'independent-B1-window-audit.json'
    assert not output.exists()
    output.write_text(json.dumps(dict(observations=observations,new_requirement_credit=0,
        whole_row_qualified=False),indent=2)+'\n')
    return code or (1 if any(r['finding']['status']!='passed' for r in observations) else 0)


if __name__ == '__main__':
    raise SystemExit(main())
