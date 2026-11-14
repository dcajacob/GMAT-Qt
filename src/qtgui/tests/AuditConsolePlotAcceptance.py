"""Audit named console plotting exceptions and the measured Ubuntu console run.

Whole-product parity and Linux/Fedora/other-platform parents stay unqualified.
Positive GUI delivery reuses the existing fully audited visible plotting proof.
"""
import argparse
import csv
import hashlib
import json
import math
from pathlib import Path
import re
import tarfile
import xml.etree.ElementTree as ET

from AuditCapiAcceptance import load, require, sha, near
import AuditSolverPublicationAcceptance as gui
import NasaGuiCoverage as nasa
from RunHarnessAcceptance import passed_junit

KEYS = ['All Others!E21','All Others!E22','All Others!E24','All Others!E286']
CASES = ['none','xy','orbit','ground','all','invalid-xy','invalid-orbit','invalid-ground']
SOURCE = 'src/qtgui/tests/RunConsolePlotAcceptance.py'
PLOTS = {'none':[], 'xy':[('XYPlot','ConsoleXY')], 'orbit':[('OrbitView','ConsoleOrbit')],
         'ground':[('GroundTrack','ConsoleGround'),('GroundTrackPlot','ConsoleLegacyGround')]}
PLOTS['all'] = PLOTS['xy']+PLOTS['orbit']+PLOTS['ground']


def check_rows(path, recorded):
    rows = [[float(x) for x in r] for r in csv.reader(path.open())]
    require(rows == recorded and len(rows) == 3 and all(len(r) == 15 for r in rows), 'Missing/altered console raw result rows')
    for row, times in zip(rows, [(0,0),(20,0),(20,10)]):
        for sat, (r,t) in enumerate(zip([7000,9000],times)):
            w = math.sqrt(398600.4415/r**3)
            want = [r*math.cos(w*t),r*math.sin(w*t),0,-r*w*math.sin(w*t),r*w*math.cos(w*t),0] if sat==0 else [0,r*math.cos(w*t),r*math.sin(w*t),0,-r*w*math.sin(w*t),r*w*math.cos(w*t)]
            require(all(near(x,y,1e-4 if j==0 else 2e-5 if j<4 else 4e-8) for j,(x,y) in enumerate(zip(row[sat*7:sat*7+7],[t,*want]))), 'Independent console circle/time oracle failed')
        require(row[-1] == 17, 'Shared non-graphical result changed')


def qualify(repo, path, inventory):
    q = load(path)
    require(q['contract_kind'] == 'source-versioned-native-console-conjunction' and q['qualified_keys'] == KEYS, 'Partial/extra/duplicate console credit')
    require(all(q.get(k) is False for k in ('full_current_build_qualified','full_NASA_qualified','technical_core_approval')), 'Console proof exceeds qualification scope')
    for p,d in q['artifact_sha256'].items(): require(sha(p) == d, 'Changed console artifact: '+p)
    require(q['authority_sha256'] == inventory['sources'], 'NASA authority differs')
    review = load(q['review']); known = {r['key']:r for r in inventory['requirements']}
    require(review['authority_sha256'] == inventory['sources'] and [r['key'] for r in review['reviews']] == KEYS, 'Incomplete original console scope review')
    for row in review['reviews']: require(row['fingerprint'] == known[row['key']]['fingerprint'] and row['source_text'] == known[row['key']]['text'] and row['rationale'], 'Changed original console requirement')
    require(review['unqualified_parent_keys'] == ['All Others!E20','All Others!E285'], 'Full parity/Linux parent was silently closed')
    run = Path(q['run']); receipt = load(run/'receipt.json'); before = load(run/'inputs-before.json'); after = load(run/'inputs-after.json')
    require(before == after and receipt['inputs_unchanged'] is True and receipt['mode'] == 'native-console' and receipt['status'] == 'passed', 'Console inputs changed or actual execution failed')
    require(receipt['GUI_executions'] == receipt['screenshots'] == receipt['runtime_copies'] == 0
            and all(receipt.get(k) is False for k in ('full_current_build_qualified','full_NASA_qualified','technical_core_approval')), 'Console execution misrepresented')
    require(hashlib.sha256(receipt['host_os_release'].encode()).hexdigest() == before['/etc/os-release']
            and re.search(r'^ID=ubuntu$',receipt['host_os_release'],re.M) and 'VERSION_ID="26.04"' in receipt['host_os_release'], 'Actual Ubuntu host is unbound')
    with tarfile.open(run/'sources.tar.gz') as b:
        require({m.name for m in b.getmembers()} == set(receipt['sources']), 'Console archive source partition differs')
        sources = {}
        for name,digest in receipt['sources'].items():
            require(not Path(name).is_absolute() and '..' not in Path(name).parts, 'Unsafe console source member')
            data = b.extractfile(name).read();require(not data.startswith(b'\x7fELF') and hashlib.sha256(data).hexdigest() == digest == before[str(repo/name)], 'Changed/non-source console input');sources[name] = data
    code = sources[SOURCE].decode()
    for token in ["for key in ['DISPLAY','WAYLAND_DISPLAY','XAUTHORITY','WAYLAND_SOCKET']:env.pop(key,None)",
                  "'--startup_file',str(work/'startup.txt'),'--run',str(work/'mission.script'),'--exit'",
                  "require(process.returncode != 0", "rows,checks=check_states(work/'output/states.csv')",
                  "'Plot exceptions changed ordinary mission results'", "'Console did not serialize the actual configured plot object: '",
                  "'Console process was unobserved or loaded GUI libraries'", "time.sleep(.005)"]:
        require(token in code, 'Original native console assertion/binding removed: '+token)
    require(receipt['sources'][SOURCE] == review['executed_runner_sha256'], 'Unreviewed console runner source')
    interface = sources['src/base/executive/PlotInterface.cpp'].decode()
    require('PlotReceiver *PlotInterface::thePlotReceiver = NULL;' in interface
            and 'thePlotReceiver->CreateXyPlotWindow' in interface and 'thePlotReceiver->CreateGlPlotWindow' in interface,
            'Console no-receiver structural contract changed')
    require('SetPlotReceiver' not in sources['src/console/driver.cpp'].decode(), 'Console now installs a plot receiver')
    require([r['case'] for r in receipt['results']] == CASES, 'Missing/duplicate native console case')
    junit = ET.parse(run/'junit.xml').getroot(); cases = junit.findall('testcase')
    require(len(cases) == 8 and [c.get('name') for c in cases] == ['QtAcceptance.ConsolePlot.'+c for c in CASES]
            and all(not c.findall('failure') and not c.findall('skipped') for c in cases), 'Missing exact passing child JUnit')
    require(passed_junit(Path(q['registered_junit']), 'QtAcceptance.ConsolePlotExceptions', 0), 'Actual CTest entry point did not pass')
    require('QtAcceptance.ConsolePlotExceptions' in Path(q['registered_log']).read_text() and str(run) in Path(q['registered_log']).read_text(), 'CTest entry point belongs to another run')
    baseline = (run/'none/output/states.csv').read_bytes()
    for result in receipt['results']:
        case = result['case']; work = run/case; cmd = result['command']; invalid = case.startswith('invalid-')
        needed = [run/'receipt.json',run/'inputs-before.json',run/'inputs-after.json',run/'sources.tar.gz',run/'junit.xml',work/'startup.txt',work/'mission.script',work/'console.log']
        if not invalid: needed += [work/'output/states.csv']
        require(all(str(p) in q['artifact_sha256'] for p in needed), 'Qualification omitted actual console proof artifact')
        require(len(cmd) == 6 and Path(cmd[0]).name.startswith('GmatConsole') and cmd[0] in result['loaded_project_images']
                and cmd[1:] == ['--startup_file',str(work/'startup.txt'),'--run',str(work/'mission.script'),'--exit'], 'Actual console command/fixture not bound')
        require(result['status'] == 'passed' and result['error'] is None and result['display_variables_removed'] is True
                and not result['loaded_GUI_images'] and result['loaded_project_images'], 'Console exception was not observed without GUI libraries')
        require(all(receipt['binaries'][p] == before[p] == digest for p,digest in result['loaded_project_images'].items()), 'Unbound loaded console image')
        for filename in ['startup.txt','mission.script']:require(before[str(work/filename)] == sha(work/filename), 'Changed actual console input')
        content = (work/'console.log').read_text();require(sha(work/'console.log') == result['log_sha256'], 'Actual console log changed')
        if invalid:
            require(result['returncode'] != 0 and result['independent_numeric_fields'] == 0 and not result['rows']
                    and not (work/'output/states.csv').exists() and re.search('Exception|ERROR|error|failed',content), 'Invalid plot configuration was accepted or executed')
        else:
            require(result['returncode'] == 0 and result['independent_numeric_fields'] == 45 and 'Mission run completed' in content
                    and [tuple(v) for v in result['configured_plots']] == PLOTS[case], 'Incomplete normal console execution')
            check_rows(work/'output/states.csv',result['rows'])
            require((work/'output/states.csv').read_bytes() == baseline, 'Console plot exceptions changed mission results')
            for kind,name in PLOTS[case]:require(re.search(r'Create '+('(?:GroundTrack|GroundTrackPlot)' if kind.startswith('GroundTrack') else kind)+' '+name+r';',content), 'Actual configured subscriber was not inspected')
    # Reject malformed console proofs before the unchanged, expensive GUI audit.
    gui.qualify(repo,Path(q['GUI_qualification']),inventory)
    return q, KEYS


if __name__ == '__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3]);p.add_argument('--qualification',type=Path,required=True);a=p.parse_args()
    _,keys=qualify(a.repo.resolve(),a.qualification,nasa.inventory(a.repo.resolve()));print(json.dumps(dict(status='passed',qualified_keys=keys,new_GUI_or_engine_executions=0)))
