"""Audit unique grouped sequence starters and native preparation controls."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import tarfile
import xml.etree.ElementTree as ET


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def load(path):
    return json.loads(Path(path).read_text())


def require(condition, message):
    if not condition:
        raise ValueError(message)


def audit(root):
    root = Path(root).resolve()
    repo = Path(__file__).resolve().parents[3]
    consumed = set()
    archives, runs, tables = {}, {}, {}

    def archived(path):
        path = Path(path)
        if path not in archives:
            with tarfile.open(path) as archive:
                members = [m for m in archive if m.isfile()]
                require(len({m.name for m in members}) == len(members), 'Duplicate archived source')
                archives[path] = {m.name:archive.extractfile(m).read() for m in members}
        consumed.add(path)
        return archives[path]

    def receipt(stem):
        if stem in runs:
            return runs[stem]
        run = root/stem
        result = load(run/'results.json')
        require(result['finished_utc'] and result['end_reason'] == 'completed-selection', 'Incomplete run')
        before, after = [load(root/(stem+'-inputs-'+phase+'.json')) for phase in ['before', 'after']]
        require(before == after, 'Source/runtime changed during run')
        capsule = load(root/(stem+'-source-reuse.json'))
        for field in ['receipt', 'reference', 'delta']:
            require(digest(capsule[field]) == capsule[field+'_sha256'], 'Capsule identity changed')
            consumed.add(Path(capsule[field]))
        sources = {**archived(capsule['reference']), **archived(capsule['delta'])}
        for name, value in result['source_hashes'].items():
            if name in before['sources']:
                require(before['sources'][name] == value, 'Pre-run/executed source differs')
            other = capsule['recorded_authority_ELF_or_existing_physical_inputs_not_copied']
            if name in other:
                require(other[name] == value, 'External identity differs')
            else:
                require(name in sources and hashlib.sha256(sources[name]).hexdigest() == value, 'Executed source unavailable')
        executable = str(repo/'build/linux-gui/src/qtgui/GmatQtNasaScriptEventCommandTests')
        require(result['binary_hashes'][executable] == before['binaries'][executable], 'Executable identity differs')
        for name in result['binary_hashes'].keys() & before['binaries'].keys():
            require(result['binary_hashes'][name] == before['binaries'][name], 'Runtime identity differs')
        version = {'event-starter-headless-v3':3, 'event-starter-headless-v5':5, 'event-starter-visible-v1':5}[stem]
        guard_path = root/f'event-starter-verified-build-{version}.json'
        guard = load(guard_path)
        original = load(guard['original_guard'])
        require(digest(guard['original_guard']) == guard['original_guard_sha256'] and
                original['compiled_sources'] == guard['compiled_sources'] and
                original['executable_sha256'] == guard['executable_sha256'] == result['binary_hashes'][executable], 'Build guard identity differs')
        log = Path(guard['successful_build_log'])
        text = log.read_text()
        require(guard['successful_build_exit_code'] == 0 and digest(log) == guard['successful_build_log_sha256'] == original['successful_build_log_sha256'] and
                'Linking CXX executable src/qtgui/GmatQtNasaScriptEventCommandTests' in text and
                not any(s in text for s in ['FAILED:', 'error:', 'build stopped', 'Configuring incomplete']), 'Successful build evidence missing')
        archive = Path(guard['compile_dependency_archive'])
        require(digest(archive) == guard['compile_dependency_archive_sha256'], 'Dependency archive changed')
        recovered = {k:hashlib.sha256(v).hexdigest() for k,v in archived(archive).items()}
        require(len(recovered) == 149 and recovered == guard['compiled_sources'], 'Compiled dependencies unavailable')
        require(guard['compiled_sources']['src/qtgui/tests/NasaScriptEventCommandTests.cpp'] == result['source_hashes']['src/qtgui/tests/NasaScriptEventCommandTests.cpp'], 'Compiled driver differs')
        for name in guard['compiled_sources'].keys() & before['sources'].keys():
            require(guard['compiled_sources'][name] == before['sources'][name], 'Compiled/pre-run source differs')
        consumed.update([guard_path, Path(guard['original_guard']), log, run/'results.json',
                         root/(stem+'-inputs-before.json'), root/(stem+'-inputs-after.json'), root/(stem+'-source-reuse.json')])
        runs[stem], tables[stem] = result, before
        return result

    selected = []
    for mode in ['headless', 'visible']:
        for family in ['BeginMissionSequence', 'PrepareMissionSequence']:
            stem = 'event-starter-visible-v1' if mode == 'visible' else 'event-starter-headless-v'+('3' if family == 'BeginMissionSequence' else '5')
            aggregate = receipt(stem)
            name = 'QtGui.NasaScriptEventStarter'+family
            case = root/stem/name
            result, proof, spec = [load(case/n) for n in ['result.json', 'proof.json', 'case.json']]
            require(result['status'] == 'passed' and result['returncode'] == 0 and result['requested_visibility'] == mode and result['name'] == name, 'Starter not passed in requested mode')
            require([x for x in aggregate['results'] if x['name'] == name] == [result], 'Case/aggregate differs')
            fixture = 'src/qtgui/tests/script-event-starters/'+family+'.json'
            require(str(repo/fixture) in result['registered_command'] and digest(case/'case.json') == proof['fixture_sha256'] == tables[stem]['sources'][fixture] == aggregate['source_hashes'][fixture], 'Fixture/process identity differs')
            require(proof['family'] == spec['family'] == family and proof['outcome'] == 'passed' and spec['unique_starter'] and spec['marker_plain_control'], 'Unique starter contract differs')
            log = (case/'test.log').read_text()
            require([json.loads(line.split('NASA_CHECK ', 1)[1]) for line in log.splitlines() if 'NASA_CHECK ' in line] == [proof], 'Proof/process differs')
            tests = list(ET.parse(case/'ctest.xml').getroot().iter('testcase'))
            require(len(tests) == 1 and tests[0].get('name') == name and tests[0].find('failure') is None and tests[0].find('error') is None, 'JUnit outcome differs')
            text = '\n'.join(x.text or '' for x in tests[0].findall('system-out'))
            if '[This part of the test output was removed since it exceeds the threshold of 1024 bytes.]' in text:
                prefix = text.rsplit('...\n[This part', 1)[0]
                require(len(prefix) >= 512 and prefix in re.sub(r'(?m)^\d+: ?', '', log), 'JUnit/process prefix differs')
            else:
                require([json.loads(line.split('NASA_CHECK ', 1)[1]) for line in text.splitlines() if 'NASA_CHECK ' in line] == [proof], 'JUnit/proof differs')
            native = load(case/'native-commands.json')
            require(native['all'] == proof['native_types'] and native['inside_event'] == proof['contained_types'] and native['inside_event'].count(family) == 1 and native['all'].count(family) == 1 and native['inside_event'][0] == 'BeginScript' and native['inside_event'][-1] == 'EndScript', 'Unique contained native starter differs')
            require(proof['factory_types'] == load(case/'factory-commands.json'), 'Factory census differs')
            rejections = load(case/'starter-mapping-rejections.json')
            require(rejections == proof['starter_mapping_rejections'] and len(rejections) == 3 and len(set(rejections)) == 3, 'Unsafe mapping witnesses missing')
            accepted = (case/'accepted.script').read_text()
            require("BeginScript 'Different event'" in rejections[0] and 'Count = Count * 8;' in rejections[1] and rejections[2].count(family+';') == 2 and all(x != accepted for x in rejections), 'Wrong unsafe mutations')
            angle = math.sqrt(398600.4415/7000**3)*20
            speed = math.sqrt(398600.4415/7000)
            expected = [20, 7000*math.cos(angle), 7000*math.sin(angle), 0, -speed*math.sin(angle), speed*math.cos(angle), 0, 41, 21, 1, 0, 0]
            def values(filename):
                lines = (case/filename).read_text().strip().splitlines()
                require(len(lines) == 1, 'Report row count differs')
                row = [float(x) for x in lines[0].split(',')]
                require(len(row) == 12 and all(math.isfinite(x) and abs(x-y) < (1e-4 if i == 0 else 2e-5) for i,(x,y) in enumerate(zip(row, expected))), 'Independent circle/scalar differs')
                return row
            if family == 'BeginMissionSequence':
                grouped, plain = values('results.csv'), values('plain-control-results.csv')
                require(grouped == proof['observed'] and all(abs(x-y) < 2e-8 for x,y in zip(grouped, plain)), 'Plain/grouped execution differs')
            else:
                initial = dict(epoch=21545, cartesian=[7000,0,0,0,speed,0], count=0, report_exists=False)
                for filename, key in [('plain-prepared.json','plain_prepared'),('native-filename-prepared.json','native_filename_prepared')]:
                    require(load(case/filename) == proof[key], 'Prepared raw/proof differs')
                for key in ['plain_prepared','native_filename_prepared','grouped_prepared']:
                    state = proof[key]
                    require(state['epoch'] == 21545 and state['count'] == 0 and state['report_exists'] is False and len(state['cartesian']) == 6 and all(abs(x-y) < 1e-10 for x,y in zip(state['cartesian'], initial['cartesian'])), 'Preparation executed following commands or changed initial state')
                control = load(case/'native-filename-control.json')
                require(control['built'] and control['run_status'] == 1, 'Native filename control failed')
                native_source = (case/'native-filename-source.script').read_text()
                plain_source = (case/'plain-control-source.script').read_text()
                require(native_source.isascii() and native_source == ''.join(line+'\n' for line in plain_source.split('\n') if not line.strip().startswith('%')), 'ASCII control changed more than full comment lines')
                require(values('post-preparation-results.csv') == proof['post_preparation_values'], 'Prepare-to-normal transition differs')
                require(not (case/'results.csv').exists(), 'Unplanned grouped result artifact')
            if mode == 'visible':
                desktop = aggregate['visible_display']
                require(desktop['route'] == 'nested Xephyr window on existing desktop' and desktop['resizable'] and desktop['start_minimized'] and 'window state: Iconic' in desktop['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN' not in desktop['initial_host_window_state'] and not desktop['resize_errors'] and result['trace']['native_exposure_events'] > 0, 'Visible window contract missing')
            consumed.update(p for p in case.iterdir() if p.is_file() and not p.is_symlink())
            selected.append(dict(family=family,mode=mode,receipt=str(case/'result.json'),exposures=result['trace']['native_exposure_events'],source_version=aggregate['source_hashes']['src/qtgui/tests/NasaScriptEventCommandTests.cpp']))
    before, after = tables['event-starter-headless-v3']['sources'], tables['event-starter-visible-v1']['sources']
    production = {k for k in before.keys() & after.keys() if '/tests/' not in k and Path(k).suffix in ['.cpp','.hpp']}
    require(all(before[k] == after[k] for k in production), 'Production changed after selected repaired run')
    changes = load(root/'event-starter-core-changes.json')
    require({x['path'] for x in changes['changes']} == {'src/base/executive/Moderator.cpp','src/base/interpreter/Interpreter.cpp','src/qtgui/MissionModel.cpp'}, 'Unexpected production change inventory')
    for row in changes['changes']:
        require(digest(row['before_path']) == row['before_sha256'] and digest(row['increment_diff']) == row['increment_diff_sha256'] and after[row['path']] == row['after_sha256'], 'Production diff/source binding differs')
        consumed.update([Path(row['before_path']),Path(row['increment_diff'])])
    consumed.add(root/'event-starter-core-changes.json')
    return dict(status='passed-source-versioned-starter-contexts',selected=selected,NASA_credit=0,qualified_keys=[],requirements_verified=1002,total_requirements=1158,core_changes=[r['path'] for r in changes['changes']],stable_repaired_production_sources=len(production),consumed_sha256={str(p):digest(p) for p in sorted(consumed)},auditor_sha256=digest(__file__),scope='Two unique starter contexts, independent native preparation and stream reset; complete supported-command conjunction required before whole-row credit.')


if __name__ == '__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    args=p.parse_args();require(not args.output.exists(),'Refusing to overwrite')
    result=audit(args.root);args.output.write_text(json.dumps(result,indent=2)+'\n')
    print('Checked',len(result['selected']),'starter selections;',len(result['consumed_sha256']),'consumed identities; whole row still requires conjunction')
