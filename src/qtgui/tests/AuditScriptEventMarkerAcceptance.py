"""Audit NoOp execution and duplicate-starter rejection; zero whole-row credit."""
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
        version = int(stem.rsplit('v', 1)[1])
        guard_path = root/('event-marker-verified-build-2-stable.json' if version == 2 else f'event-marker-verified-build-{version}.json')
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
        for family in ['NoOp', 'BeginMissionSequence', 'PrepareMissionSequence', 'Create']:
            stem = 'event-marker-'+mode+('-v1' if family == 'NoOp' else '-v3' if family == 'Create' else '-v2')
            aggregate = receipt(stem)
            name = 'QtGui.NasaScriptEventMarker'+family
            case = root/stem/name
            result, proof, spec = [load(case/n) for n in ['result.json', 'proof.json', 'case.json']]
            require(result['name'] == name and result['status'] == 'passed' and result['returncode'] == 0 and result['requested_visibility'] == mode, 'Case did not pass requested mode')
            require([r for r in aggregate['results'] if r['name'] == name] == [result], 'Aggregate/case identity differs')
            fixture_name = 'src/qtgui/tests/script-event-markers/'+family+'.json'
            require(str(repo/fixture_name) in result['registered_command'], 'Registered fixture differs')
            fixture_hash = tables[stem]['sources'][fixture_name]
            if fixture_name in aggregate['source_hashes']:
                require(aggregate['source_hashes'][fixture_name] == fixture_hash, 'Manifest/pre-run fixture differs')
            require(proof['outcome'] == 'passed' and proof['family'] == spec['family'] == family and digest(case/'case.json') == proof['fixture_sha256'] == fixture_hash, 'Proof/fixture identity differs')
            log = (case/'test.log').read_text()
            require([json.loads(line.split('NASA_CHECK ', 1)[1]) for line in log.splitlines() if 'NASA_CHECK ' in line] == [proof], 'Process proof differs')
            tests = list(ET.parse(case/'ctest.xml').getroot().iter('testcase'))
            require(len(tests) == 1 and tests[0].get('name') == name and tests[0].find('failure') is None and tests[0].find('error') is None, 'JUnit differs')
            output = '\n'.join(t.text or '' for t in tests[0].findall('system-out'))
            if '[This part of the test output was removed since it exceeds the threshold of 1024 bytes.]' in output:
                prefix = output.rsplit('...\n[This part', 1)[0]
                require(len(prefix) >= 512 and prefix in re.sub(r'(?m)^\d+: ?', '', log), 'JUnit/process prefix differs')
            else:
                require([json.loads(line.split('NASA_CHECK ', 1)[1]) for line in output.splitlines() if 'NASA_CHECK ' in line] == [proof], 'JUnit/process proof differs')
            if family == 'NoOp':
                require(spec['marker_plain_control'] and spec['expected_tail'] == [21, 1, 0, 0], 'NoOp reference differs')
                angle = math.sqrt(398600.4415/7000**3)*20
                speed = math.sqrt(398600.4415/7000)
                expected = [20, 7000*math.cos(angle), 7000*math.sin(angle), 0, -speed*math.sin(angle), speed*math.cos(angle), 0, 41, 21, 1, 0, 0]
                values = []
                for filename in ['results.csv', 'plain-control-results.csv']:
                    lines = (case/filename).read_text().strip().splitlines()
                    require(len(lines) == 1, 'Wrong report row count')
                    row = [float(x) for x in lines[0].split(',')]
                    require(len(row) == 12 and all(math.isfinite(x) and abs(x-y) < (1e-4 if i == 0 else 2e-5) for i,(x,y) in enumerate(zip(row, expected))), 'Independent NoOp result differs')
                    values.append(row)
                require(all(abs(x-y) < 2e-8 for x,y in zip(*values)) and proof['observed'] == values[0], 'Plain/grouped/proof differs')
                native = load(case/'native-commands.json')
                require(native['inside_event'] == proof['contained_types'] and native['all'] == proof['native_types'] and native['inside_event'].count('NoOp') == 1 and 'BeginScript' in native['inside_event'] and 'EndScript' in native['inside_event'], 'Contained NoOp missing')
            else:
                require((spec['unsupported_definition'] if family == 'Create' else spec['duplicate_starter']) and len(proof['rejections']) == 2, 'Wrong rejection contract')
                reason = ('GMAT currently requires that all objects are created before the mission sequence begins' if family == 'Create' else 'Too many Mission Sequence start')
                for context, observed in zip(['plain', 'grouped'], proof['rejections']):
                    diagnostic = (case/(context+'-rejection.txt')).read_text()
                    require(observed == dict(context=context, build_accepted=False, diagnostic=diagnostic) and reason in diagnostic, 'Wrong native rejection reason')
                    source = (case/(context+'-rejected-source.script')).read_text()
                    if family == 'Create':
                        require(source.count('BeginMissionSequence;') == 1 and 'Create Variable Inside;' in source.split('BeginMissionSequence;', 1)[1], 'Not a forbidden main-body definition fixture')
                    else:
                        require(source.count('BeginMissionSequence;')+(source.count('PrepareMissionSequence;')) == 2, 'Not a duplicate-starter fixture')
                require(not (case/'results.csv').exists(), 'Rejected source unexpectedly executed')
            if mode == 'visible':
                desktop = aggregate['visible_display']
                require(desktop['route'] == 'nested Xephyr window on existing desktop' and desktop['resizable'] and desktop['start_minimized'] and
                        'window state: Iconic' in desktop['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN' not in desktop['initial_host_window_state'] and
                        not desktop['resize_errors'] and result['trace']['native_exposure_events'] > 0, 'Visible window contract missing')
            consumed.update(p for p in case.iterdir() if p.is_file() and not p.is_symlink())
            selected.append(dict(family=family, mode=mode, receipt=str(case/'result.json'), exposures=result['trace']['native_exposure_events'], source_version=aggregate['source_hashes']['src/qtgui/tests/NasaScriptEventCommandTests.cpp']))
    before, after = tables['event-marker-headless-v1']['sources'], tables['event-marker-visible-v3']['sources']
    common = {k for k in before.keys() & after.keys() if '/tests/' not in k and Path(k).suffix in ['.cpp', '.hpp']}
    require(all(before[k] == after[k] for k in common), 'Production changed')
    return dict(status='passed-bounded-marker-audit', selected=selected, NASA_credit=0, qualified_keys=[], requirements_verified=1002, total_requirements=1158,
                core_changes=[], production_sources_unchanged=len(common), legal_unique_starter_contexts_resolved=False,
                consumed_sha256={str(p):digest(p) for p in sorted(consumed)}, auditor_sha256=digest(__file__))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    require(not args.output.exists(), 'Refusing to overwrite')
    result = audit(args.root)
    args.output.write_text(json.dumps(result, indent=2)+'\n')
    print('Checked', len(result['selected']), 'source-versioned selections;', len(result['consumed_sha256']), 'consumed hashes; no NASA credit')
