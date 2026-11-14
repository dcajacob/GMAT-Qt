#!/usr/bin/env python3
"""Run all registered Qt GUI checks with optional visible interaction and coverage receipts.

Uses CTest for timeout, environment, expected-failure and resource-lock semantics.
Passing regression checks never implies complete widget or NASA requirement coverage.
"""
import argparse
from collections import Counter
import csv
import datetime as dt
import hashlib
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import sys
import tempfile
import time
from types import SimpleNamespace
import xml.etree.ElementTree as ET

from AcceptanceRuntime import isolate_startup
import NasaGuiCoverage
import AcceptanceSnapshot
import ScreenshotRetention
from RunTutorials import configure_display as configure_tutorial_display, scale_value

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
NASA_MATRIX = Path('doc/SystemDocs/Test/Requirements-To-Test Matrix, 2012-05-11 (GMAT Build 2012-05-04).csv')
NASA_PLAN = 'https://gmat.atlassian.net/wiki/spaces/GW/pages/380273283'


def utc():
    return dt.datetime.now(dt.timezone.utc).isoformat()


def sha(path):
    path = Path(path)
    if not path.is_file():
        return None
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def write_json(path, value):
    # Readers see a complete receipt even if a run is interrupted between cases.
    temporary = path.with_suffix(path.suffix + '.tmp')
    temporary.write_text(json.dumps(value, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
    temporary.replace(path)


def coverage_estimate(cov):
    """Keep reviewed proof scopes separate from regression and widget counts."""
    counts = cov.get('nasa_counts', {})
    estimate = {}
    for collection, label in [('requirements', 'requirement_scopes'),
                              ('historical_cases', 'historical_operation_scopes')]:
        statuses = counts.get(collection, {})
        total = sum(statuses.values())
        verified = statuses.get('verified', 0)
        estimate[label] = {'verified': verified, 'total': total,
                           'percent': round(100 * verified / total, 1) if total else None}
    estimate['limit'] = ('Reviewed bounded proof scopes, not overall NASA completeness. '
                         'Widget/input-domain coverage has no established percentage.')
    return estimate


class CoverageProgress:
    """Report at case boundaries without interrupting a test's own event loop."""
    def __init__(self, interval=60, clock=time.monotonic):
        self.interval, self.clock, self.last = interval, clock, None

    def emit(self, cov, force=False):
        now = self.clock()
        if not force and (self.interval == 0 or
                          (self.last is not None and now - self.last < self.interval)):
            return
        self.last = now
        estimate = coverage_estimate(cov)
        pieces = []
        for key, label in [('requirement_scopes', 'requirement scopes'),
                           ('historical_operation_scopes', 'historical operation scopes')]:
            item = estimate[key]
            pieces.append(f'{item["verified"]}/{item["total"]} {label} ({item["percent"]:.1f}%)'
                          if item['percent'] is not None else f'{label}: unavailable')
        print('NASA coverage estimate: ' + '; '.join(pieces) +
              '. This run receipt only; bounded proofs, not cumulative project or full widget/input coverage.', flush=True)


def configure_display(args, probe):
    # Reuse the tutorial runner's host/XWayland scale and fitted-size policy.
    config = SimpleNamespace(visible=args.visible, driver='qttest', scale=args.scale,
                             width=args.width, height=args.height)
    if args.display_scale == 'test':
        config.scale = 1.0 if args.scale is None else args.scale
        config.width = 1600 if args.width is None else args.width
        config.height = 1200 if args.height is None else args.height
    configure_tutorial_display(config, probe)
    for key in ('scale', 'scale_source', 'width', 'height', 'host_display'):
        setattr(args, key, getattr(config, key))


def input_sources(repo, cases, contract, nasa):
    files = {source for case in cases for source in case['sources']}
    files.update(str(path.relative_to(repo)) for pattern in ('*.cpp', '*.hpp')
                 for path in (repo / 'src/qtgui').glob(pattern))
    files.update(str(path.relative_to(repo)) for pattern in ('*.py', '*.hpp', 'Acceptance*.cpp', 'Acceptance*.json')
                 for path in (repo / 'src/qtgui/tests').glob(pattern))
    files.update(item['source'] for item in contract['mechanisms'])
    files.update(['CMakeLists.txt', 'src/qtgui/CMakeLists.txt', NasaGuiCoverage.WORKBOOK,
                  NasaGuiCoverage.MATRIX, NasaGuiCoverage.MANIFEST])
    files.update(nasa[1].get('reference_sources', {}))
    for row in nasa[1]['reviews']:
        files.update(row.get('applicability_sources', {}))
        # Partial scientific witnesses need the same immutable inputs, even
        # while their parent requirement deliberately receives no credit.
        for claim in row.get('evidence', []) + row.get('partial_evidence', []):
            files.add(claim['source'])
            files.update(claim.get('reference_sources', {}))
    # Unverified/removed requirements can have no executable mapping. Their
    # reviewed source conflicts still need immutable supporting source files.
    for conflict in nasa[1].get('source_conflicts', []):
        files.update(conflict.get('reference_sha256', {}))
    return files


def resume_state(directory, args):
    directory = Path(directory).resolve()
    prior = json.loads((directory / 'results.json').read_text())
    if prior.get('snapshot_unchanged') is False:
        raise ValueError('The prior run failed snapshot integrity; its results cannot be reused')
    path = Path(prior.get('snapshot_manifest', ''))
    if not path.is_file():
        raise ValueError('Only a run created with --snapshot can be resumed')
    manifest = json.loads(path.read_text())
    AcceptanceSnapshot.verify(manifest)
    if prior.get('execution_profile') != AcceptanceSnapshot.profile(args):
        raise ValueError('Resume requires the same config, visibility, display route and scale')
    if not prior.get('finished_utc'):
        if os.name == 'nt':
            raise ValueError('An unfinished Windows run cannot be resumed without verified process-lifecycle support')
        pid = prior.get('runner_pid')
        if pid:
            try:
                os.kill(pid, 0)
                command_path = Path('/proc') / str(pid) / 'cmdline'
                if not command_path.exists() or b'RunAcceptance.py' in command_path.read_bytes():
                    raise ValueError('The previous acceptance runner is still active')
            except ProcessLookupError:
                pass
            except PermissionError:
                raise ValueError('Cannot verify that the previous acceptance runner has stopped')
    # A changed controller must not silently reinterpret an older snapshot.
    for name in ('RunAcceptance.py', 'AcceptanceSnapshot.py', 'AcceptanceRuntime.py', 'NasaGuiCoverage.py',
                 'VisibleTestDisplay.py', 'RunTutorials.py', 'IsolatedX11Session.py', 'ScreenshotRetention.py'):
        key = 'src/qtgui/tests/' + name
        if sha(HERE / name) != manifest['original_hashes'].get(key):
            raise ValueError('Runner changed; resume with the archived runner at ' +
                             str(Path(manifest['root']) / 'src/qtgui/tests/RunAcceptance.py'))
    candidates = {item['name']: item for item in prior['results']}
    # An interruption between result.json and the aggregate receipt should not
    # force a successful case to run again. Its hashes and identity still must pass.
    for case in manifest['cases']:
        saved = directory / re.sub(r'[^A-Za-z0-9_.-]', '_', case['name']) / 'result.json'
        if saved.is_file():
            candidates[case['name']] = json.loads(saved.read_text())
    reusable = {}
    for case in manifest['cases']:
        result = candidates.get(case['name'], {})
        if AcceptanceSnapshot.reusable(result, case, manifest['snapshot_id'], AcceptanceSnapshot.profile(args)):
            if junit_status(Path(result['junit']), result.get('returncode', 1)) == 'passed':
                reusable[case['name']] = {**result, 'reused_from': str(directory), 'reuse_validated_utc': utc()}
    return manifest, path, reusable


def properties(test):
    return {item['name']: item['value'] for item in test.get('properties', [])}


def cmake_sources(repo):
    text = (repo / 'src/qtgui/CMakeLists.txt').read_text(encoding='utf-8')
    return {name: [str(Path('src/qtgui') / value) for value in body.split()
                   if value.endswith(('.cpp', '.py'))]
            for name, body in re.findall(r'add_executable\((\w+)\s+([^)]*)\)', text)}


def validate_selected_executables(repo, cases, selected_names):
    targets = cmake_sources(repo)
    missing = []
    for case in cases:
        if case['name'] not in selected_names:
            continue
        for argument in case.get('command', []):
            path = Path(argument)
            if not path.is_absolute() or path.suffix.lower() not in ('', '.exe'):
                continue
            target = path.stem
            for suffix in ('-R2026a', 'd'):
                if target not in targets and target.endswith(suffix):
                    target = target[:-len(suffix)]
            if target in targets and (not path.is_file() or not os.access(path, os.X_OK)):
                missing.append(case['name'] + ': ' + str(path))
    if missing:
        raise ValueError('Build the selected acceptance executables before running or preserving a snapshot: ' + '; '.join(missing))


def case_sources(test, target_sources, repo=REPO):
    result = []
    for arg in test.get('command', []):
        target = Path(arg).stem
        for suffix in ('-R2026a', 'd'):
            if target not in target_sources and target.endswith(suffix):
                target = target[:-len(suffix)]
        result.extend(target_sources.get(target, []))
        if Path(arg).suffix == '.py' and Path(arg).name not in ('RunTest.py', 'RunNativeTest.py'):
            try:
                result.append(str(Path(arg).relative_to(repo)))
            except ValueError:
                pass
    return sorted(set(result))


def classify(test, sources, repo=REPO):
    text = '\n'.join((repo / source).read_text(encoding='utf-8') for source in sources if (repo / source).is_file())
    command = test.get('command', [])
    if 'qt-acceptance-transaction' in properties(test).get('LABELS', []):
        return 'support', 'Frontend transaction regression; does not open widgets or replay the authored tutorial GUI.'
    if 'qt-acceptance-offscreen' in properties(test).get('LABELS', []):
        return 'headless-only', 'Explicitly retained offscreen regression; its visible widget companion is a separate case.'
    if test['name'].startswith('QtGui.Native'):
        return 'visible', 'Native rendering/window test on the selected visible display with software OpenGL.'
    if test['name'] == 'QtGui.Launch':
        return 'headless-only', 'Launch/error subprocesses explicitly select offscreen Qt; their checks still run.'
    if not any('RunTest.py' in arg for arg in command):
        return 'support', 'Python/helper contract check; no interactive GUI.'
    if 'QApplication' not in text:
        return 'support', 'Model/parser check; no QApplication widget workflow.'
    # These guards intentionally constrain CPU fallback/rendering or focus semantics.
    # Do not remove them merely to make a coverage report look more complete.
    guarded = ('platformName()' in text or 'qEnvironmentVariable("QT_QPA_PLATFORM")' in text) and '"offscreen"' in text
    if guarded and 'acceptanceWidgetPlatform()' not in text:
        return 'headless-only', 'Existing test explicitly requires offscreen Qt; visible companion or reviewed adaptation is a coverage gap.'
    return 'visible', 'In-process Qt widget events/frontend calls; not OS input injection.'


def inventory(repo, tests):
    targets = cmake_sources(repo)
    records = []
    for test in tests:
        sources = case_sources(test, targets, repo)
        route, reason = classify(test, sources, repo)
        records.append({**test, 'sources': sources, 'visibility': route, 'visibility_reason': reason})
    return records


def named_controls(repo, cases):
    """A source inventory, not an executable coverage assertion or a full widget census."""
    source_tests = {}
    for case in cases:
        for source in case['sources']:
            path = repo / source
            if path.is_file():
                source_tests.setdefault(source, {'text': path.read_text(encoding='utf-8'), 'tests': []})['tests'].append(case['name'])
    rows = []
    for path in sorted((repo / 'src/qtgui').glob('*.cpp')):
        for line_no, line in enumerate(path.read_text(encoding='utf-8').splitlines(), 1):
            for match in re.finditer(r'setObjectName\("([^"\n]+)"\)', line):
                name = match[1]
                # A concatenated expression is dynamic, and must not count as a literal name.
                if line[match.end():].lstrip().startswith('+'):
                    continue
                candidates = sorted({test for item in source_tests.values()
                                     if '"' + name + '"' in item['text'] for test in item['tests']})
                rows.append({'name': name, 'source': str(path.relative_to(repo)), 'line': line_no,
                             'candidate_tests': candidates,
                             'candidate_limit': 'Source reference only; does not establish execution or a result assertion.'})
    return rows


def legacy_requirements(repo):
    with (repo / NASA_MATRIX).open(newline='', encoding='utf-8-sig') as stream:
        rows = list(csv.DictReader(stream))
    return [{'row': i + 2, 'object': row.get('Object'), 'operation': row.get('Operation'),
             'requirement_ids': row.get('Requirement(s)'), 'historical_test': row.get('Test Case'),
             'historical_result': row.get('Results'),
             'current_status': 'unmapped historical case; not evidence of current Qt acceptance'}
            for i, row in enumerate(rows)]


def trace_summary(case_dir):
    objects, screenshots, exposures, starts = {}, [], 0, 0
    qt_exposures, recorded_platforms = 0, set()
    for path in sorted(case_dir.glob('trace-*.jsonl')):
        process_platforms, process_exposures = set(), 0
        for line in path.read_text(encoding='utf-8').splitlines():
            try:
                event = json.loads(line)
            except json.JSONDecodeError:
                continue  # A crash may leave a partial final line; test failure is retained.
            starts += event.get('event') == 'start'
            process_exposures += event.get('event') == 'native_exposure' and event.get('exposed', False)
            if event.get('platform'):
                process_platforms.add(event['platform'])
            if event.get('name'):
                obj = objects.setdefault(event['name'], {'events': set(), 'classes': set(), 'paths': set()})
                obj['events'].add(event['event']); obj['classes'].add(event.get('class', ''))
                if event.get('key'):
                    obj['paths'].add(event['key'])
            if event.get('screenshot'):
                screenshots.append(str(path.parent / event['screenshot']))
        qt_exposures += process_exposures
        recorded_platforms.update(process_platforms)
        # Qt's offscreen plugin also reports QWindow::isExposed(). Require an
        # actually recorded native backend before calling it native exposure.
        if len(process_platforms) == 1 and any(p in ('xcb', 'windows', 'cocoa') or p.startswith('wayland') for p in process_platforms):
            exposures += process_exposures
    return {'instrumented_processes': starts, 'native_exposure_events': exposures,
            'qt_window_exposure_events': qt_exposures, 'recorded_platforms': sorted(recorded_platforms),
            'screenshots': screenshots,
            'named_objects': {name: {key: sorted(value) for key, value in obj.items()} for name, obj in objects.items()}}


def coverage(cases, results, controls, contract):
    by_name = {result['name']: result for result in results}
    domains = []
    assigned = set()
    for domain in contract['mechanisms']:
        names = [case['name'] for case in cases if re.search(domain['tests'], case['name'])]
        assigned.update(names)
        passed = [name for name in names if by_name.get(name, {}).get('status') == 'passed']
        domains.append({**domain, 'registered_tests': names, 'passed_tests': passed,
                        'failed_tests': [name for name in names if by_name.get(name, {}).get('status') in ('failed', 'timeout')],
                        'unexecuted_tests': [name for name in names if name not in by_name or by_name[name].get('status') in ('not-run', 'skipped')],
                        'status': 'regression checks passed; requirement acceptance incomplete' if names and len(passed) == len(names)
                                  else 'incomplete or failing regression evidence'})
    current_controls = []
    for control in controls:
        seen, changed, activated = [], [], []
        for result in results:
            obj = result.get('trace', {}).get('named_objects', {}).get(control['name'], {})
            events = obj.get('events', [])
            if events:
                seen.append(result['name'])
            if 'changed' in events:
                changed.append(result['name'])
            if 'activated' in events:
                activated.append(result['name'])
        current_controls.append({**control, 'observed_in': seen, 'changed_in': changed, 'activated_in': activated,
                                 'runtime_match_limit': 'Matched by objectName only; attribution to this source declaration is not established, particularly for reused names.',
                                 'assertion_coverage': 'not established per control; inspect the linked test assertions'})
    gaps = [*contract['known_gaps']]
    missing = sorted({case['name'] for case in cases} - assigned)
    if missing:
        gaps.append({'id': 'UNMAPPED-TESTS', 'detail': 'Registered tests lack a mechanism mapping.', 'tests': missing})
    headless = [case['name'] for case in cases if case['visibility'] == 'headless-only']
    if headless:
        gaps.append({'id': 'VISIBILITY-GUARDS', 'detail': 'These tests still execute, but require offscreen Qt.', 'tests': headless})
    unrun = [case['name'] for case in cases if by_name.get(case['name'], {}).get('status') not in ('passed', 'failed', 'timeout')]
    if unrun:
        gaps.append({'id': 'UNEXECUTED', 'detail': 'Tests were not selected, were skipped, or have not finished.', 'tests': unrun})
    unobserved = [f"{x['source']}:{x['line']}:{x['name']}" for x in current_controls if not x['observed_in']]
    if unobserved:
        gaps.append({'id': 'UNOBSERVED-NAMED-CONTROLS', 'detail': 'No runtime observation of these literal object names. Dynamic/unnamed controls are an additional gap.', 'controls': unobserved})
    invisible = [result['name'] for result in results if result.get('requested_visibility') == 'visible'
                 and result.get('status') == 'passed' and not result.get('trace', {}).get('native_exposure_events')]
    if invisible:
        gaps.append({'id': 'NO-NATIVE-EXPOSURE', 'detail': 'Visible route requested, but no native window exposure was recorded.', 'tests': invisible})
    uninstrumented = [result['name'] for result in results if result.get('status') == 'passed'
                      and result['name'] != 'QtGui.Launch'  # shipped application is deliberately not instrumented
                      and any(source.endswith('.cpp') for source in result.get('sources', []))
                      and not result.get('trace', {}).get('instrumented_processes')
                      and next((case['visibility'] for case in cases if case['name'] == result['name']), 'support') != 'support']
    if uninstrumented:
        gaps.append({'id': 'MISSING-INSTRUMENTATION', 'detail': 'Passing widget executables produced no observer trace; rebuild build-qt-acceptance before claiming runtime control coverage.', 'tests': uninstrumented})
    return {'acceptance_complete': False, 'mechanisms': domains, 'named_controls': current_controls, 'gaps': gaps,
            'limits': contract['coverage_limits']}


def junit_status(path, returncode):
    if not path.is_file():
        return 'failed'
    try:
        root = ET.parse(path).getroot()
    except ET.ParseError:
        return 'failed'
    cases = list(root.iter('testcase'))
    if len(cases) != 1:
        return 'failed'
    case = cases[0]
    if case.find('skipped') is not None:
        # CTest also writes <skipped> for a test it could not start. A failing
        # CTest exit must not turn a missing executable/workdir into a clean skip.
        return 'skipped' if returncode == 0 else 'failed'
    return 'passed' if returncode == 0 and case.find('failure') is None and case.find('error') is None else 'failed'


def terminate(process):
    if process.poll() is not None:
        return
    if os.name == 'posix':
        os.killpg(process.pid, signal.SIGTERM)
    else:
        process.terminate()
    try:
        process.wait(timeout=5)
    except subprocess.TimeoutExpired:
        if os.name == 'posix':
            os.killpg(process.pid, signal.SIGKILL)
        else:
            process.kill()
        process.wait()


def paced_ctest_directory(case, directory, args):
    base = float(properties(case).get('TIMEOUT', 60))
    paced = args.visible and case['visibility'] == 'visible'
    frozen_definition = getattr(args, 'ctest_definition', None)
    if not paced and not frozen_definition:
        return args.build_dir, base, 0
    budget = getattr(args, 'pause_ms', 0) * getattr(args, 'max_checkpoints', 0) / 1000 if paced else 0
    timeout = base * getattr(args, 'visible_timeout_factor', 1) + budget if paced else base
    definition = Path(frozen_definition) if frozen_definition else args.build_dir / 'src/qtgui/CTestTestfile.cmake'
    if frozen_definition and not definition.is_file():
        raise RuntimeError('Preserved CTest registration disappeared')
    if not definition.is_file():
        definition = args.build_dir / 'CTestTestfile.cmake'
    if not definition.is_file():
        raise RuntimeError('Cannot locate CTest definitions for visible pacing')

    def literal(value):
        value = str(value).replace('\\', '/')
        delimiter = '='
        while ']' + delimiter + ']' in value:
            delimiter += '='
        return '[' + delimiter + '[' + value + ']' + delimiter + ']'

    # Import the original registrations/properties verbatim. Only the selected
    # test's wall-clock budget and its original default working directory change.
    # --timeout alone cannot override an explicit CTest TIMEOUT property.
    driver = directory / 'ctest'
    driver.mkdir()
    working = properties(case).get('WORKING_DIRECTORY', str(args.build_dir))
    text = 'include(' + literal(definition) + ')\n'
    if case.get('output_arguments'):
        # Replace only the generated command line; importing both old and new
        # add_test calls would execute the same test twice. All other commands
        # and properties (including WILL_FAIL, SKIP and locks) remain verbatim.
        command = AcceptanceSnapshot.runtime_command(case, directory)
        name = re.escape(case['name'])
        pattern = re.compile(r'^[ \t]*add_test\((?:\[(=*)\[' + name +
                             r'\]\1\]|"' + name + r'"|' + name + r')(?=\s)[^\n]*\)[ \t]*$', re.M)
        changed, count = pattern.subn(lambda _: 'add_test(' + ' '.join(literal(x) for x in [case['name'], *command]) + ')',
                                      definition.read_text())
        if count != 1:
            raise RuntimeError('Cannot safely remap generated output command: ' + case['name'])
        patched = driver / 'definitions.cmake'
        patched.write_text(changed)
        text = 'include(' + literal(patched) + ')\n'
    text += ('set_tests_properties(' + literal(case['name']) +
             ' PROPERTIES TIMEOUT ' + str(timeout) + ' WORKING_DIRECTORY ' + literal(working) + ')\n')
    (driver / 'CTestTestfile.cmake').write_text(text, encoding='utf-8')
    return driver, timeout, budget


def execute(case, directory, args, env):
    directory.mkdir()
    report = directory / 'ctest.xml'
    ctest_directory, timeout, viewing_budget = paced_ctest_directory(case, directory, args)
    command = ['ctest', '--test-dir', str(ctest_directory), '-C', args.config,
               '-R', '^' + re.escape(case['name']) + '$', '--verbose', '--output-junit', str(report)]
    selected_env = dict(env, GMAT_QT_ACCEPTANCE_TEST=case['name'], GMAT_QT_ACCEPTANCE_DIR=str(directory))
    route = 'visible' if args.visible and case['visibility'] == 'visible' else 'headless'
    selected_env['GMAT_QT_ACCEPTANCE_MODE'] = route
    if hasattr(args, 'scale'):
        selected_env.update(QT_SCREEN_SCALE_FACTORS='1', QT_FONT_DPI='96',
                            QT_SCALE_FACTOR=str(args.scale if route == 'visible' else 1.0),
                            QT_SCALE_FACTOR_ROUNDING_POLICY='PassThrough')
    # Independent startup/output/settings for every case. Leave scenario-owned startups alone.
    for argument in case.get('command', []):
        if Path(argument).name == 'gmat_startup_qt.txt' and Path(argument).is_file():
            selected_env['GMAT_QT_ACCEPTANCE_STARTUP_ORIGINAL'] = str(Path(argument).resolve())
            mapping = getattr(args, 'snapshot_mapping', None)
            selected_env['GMAT_QT_ACCEPTANCE_STARTUP'] = str(isolate_startup(argument, directory / 'runtime', mapping))
            break
    started = time.monotonic()
    result = {'name': case['name'], 'started_utc': utc(), 'command': command,
              'registered_command': case.get('command'), 'requested_visibility': route,
              'ctest_timeout_seconds': timeout, 'viewing_budget_seconds': viewing_budget,
              'sources': case['sources'], 'log': str(directory / 'test.log')}
    if getattr(args, 'snapshot_id', None):
        result.update(snapshot_id=args.snapshot_id, case_fingerprint=AcceptanceSnapshot.case_fingerprint(case),
                      execution_profile=AcceptanceSnapshot.profile(args), junit=str(report),
                      executed_test_command=AcceptanceSnapshot.runtime_command(case, directory))
    with (directory / 'test.log').open('wb') as log:
        process = subprocess.Popen(command, env=selected_env, stdout=log, stderr=subprocess.STDOUT,
                                   start_new_session=os.name == 'posix')
        try:
            code = process.wait(timeout=timeout + 30)
            result.update(returncode=code, status=junit_status(report, code))
        except subprocess.TimeoutExpired:
            terminate(process)
            result.update(returncode=124, status='timeout')
        except KeyboardInterrupt:
            terminate(process)
            result.update(returncode=130, status='interrupted')
        finally:
            result['duration_seconds'] = round(time.monotonic() - started, 3)
    result['trace'] = trace_summary(directory)
    # PASS text is retained as human evidence; it does not replace the actual CTest result.
    result['checks_reported'] = [re.sub(r'^\s*\d+:\s*', '', line) for line in (directory / 'test.log').read_text(errors='replace').splitlines()
                                 if re.match(r'^\s*(?:\d+:\s*)?PASS\b', line)]
    result['failure_summary'] = [re.sub(r'^\s*\d+:\s*', '', line) for line in (directory / 'test.log').read_text(errors='replace').splitlines()
                                 if re.search(r'\bFAIL\b|CHECK FAILED:|check failed:|\*\*\*Timeout|\*\*\*Not Run', line, re.I)][-8:]
    if result['status'] != 'passed' and not result['failure_summary']:
        result['failure_summary'] = [re.sub(r'^\s*\d+:\s*', '', line) for line in (directory / 'test.log').read_text(errors='replace').splitlines()
                                     if re.match(r'^\s*\d+:\s*\S', line) and not re.search(r'ACCEPTANCE:|Test command:|Working Directory:|Test timeout|propagateSizeHints', line)][-5:]
    if getattr(args, 'snapshot_id', None):
        result['log_sha256'] = sha(directory / 'test.log')
        result['junit_sha256'] = sha(report)
    write_json(directory / 'result.json', result)
    return result


def combined_junit(output, results):
    root = ET.Element('testsuite', name='GMAT Qt GUI acceptance regression checks', tests=str(len(results)),
                      failures=str(sum(x['status'] in ('failed', 'timeout', 'interrupted') for x in results)),
                      skipped=str(sum(x['status'] in ('skipped', 'not-run') for x in results)))
    for result in results:
        case = ET.SubElement(root, 'testcase', name=result['name'], time=str(result.get('duration_seconds', 0)))
        if result['status'] in ('skipped', 'not-run'):
            ET.SubElement(case, 'skipped', message=result['status'])
        elif result['status'] != 'passed':
            ET.SubElement(case, 'failure', message=result['status']).text = result.get('log', '')
        ET.SubElement(case, 'system-out').text = result.get('log', '')
    ET.ElementTree(root).write(output / 'junit.xml', encoding='utf-8', xml_declaration=True)


def report(output, receipt, cases, controls, contract, legacy, nasa=None, evidence_repo=REPO):
    cov = coverage(cases, receipt['results'], controls, contract)
    if receipt.get('snapshot_unchanged') is False:
        cov['gaps'].append({'id': 'SNAPSHOT-INTEGRITY', 'detail': receipt['snapshot_error']})
    if receipt.get('display_error'):
        cov['gaps'].append({'id': 'DISPLAY-RESIZE', 'detail': receipt['display_error']})
    if nasa is not None:
        nasa_report = NasaGuiCoverage.evaluate(evidence_repo, *nasa, receipt)
        NasaGuiCoverage.write_report(output, nasa_report)
        cov['nasa_counts'] = nasa_report['counts']
        unfinished_cases = [x for x in nasa_report['historical_cases'] if x['status'] != 'verified']
        cov['gaps'] = [gap for gap in cov['gaps'] if gap['id'] != 'NASA-CASE-TRACEABILITY']
        if unfinished_cases:
            cov['gaps'].append({'id': 'NASA-CASE-TRACEABILITY',
                                'detail': f'{len(unfinished_cases)} of {len(nasa_report["historical_cases"])} historical operation rows lack reviewed passing operation evidence. ChangeAll, unspecified input domains and unmatched cases remain open; requirement instances are counted separately.'})
        unfinished = [x for x in nasa_report['requirements'] if x['status'] != 'verified']
        if unfinished:
            cov['gaps'].append({'id': 'NASA-REQUIREMENT-TRACEABILITY',
                                'detail': f'{len(unfinished)} of {len(nasa_report["requirements"])} workbook requirement instances lack reviewed passing evidence. Historical operations have no requirement IDs and are a separate denominator. See nasa-requirements.json.'})
        if nasa_report['source_conflicts']:
            cov['gaps'].append({'id': 'NASA-SOURCE-CONFLICTS', 'detail': 'Unresolved source contradictions remain in nasa-traceability.md; setting-property proofs do not close their input-domain decisions.'})
        cov['gaps'].append({'id': 'NASA-REQUIREMENTS-BASELINE', 'detail': 'Shipped workbook modification metadata: '+str(nasa_report['source_metadata']['requirements_workbook_modified'])+'. Its completeness against current documented features and the NASA requirements/test corpus remains unqualified.'})
    cov['estimate'] = coverage_estimate(cov)
    write_json(output / 'coverage-estimate.json', cov['estimate'])
    receipt['counts'] = dict(Counter(x['status'] for x in receipt['results']))
    receipt['coverage'] = {key: value for key, value in cov.items() if key != 'named_controls'}
    write_json(output / 'results.json', receipt)
    write_json(output / 'control-coverage.json', cov['named_controls'])
    if nasa is not None:
        reviewed = {item['row']: item for item in nasa_report['historical_cases']}
        write_json(output / 'nasa-historical-matrix.json',
                   [{**item, 'current_status': reviewed[item['row']]['status'],
                     'current_review': reviewed[item['row']]} for item in legacy])
    elif not (output / 'nasa-historical-matrix.json').exists():
        write_json(output / 'nasa-historical-matrix.json', legacy)
    with (output / 'mechanisms.csv').open('w', newline='', encoding='utf-8') as stream:
        writer = csv.writer(stream)
        writer.writerow(['Local acceptance ID', 'Mechanism', 'Expected behavior', 'NASA source', 'Passed tests', 'Unexecuted tests', 'Status'])
        for item in cov['mechanisms']:
            writer.writerow([item['id'], item['title'], item['expected'], item['source'],
                             ';'.join(item['passed_tests']), ';'.join(item['unexecuted_tests']), item['status']])
    lines = ['# GMAT Qt GUI acceptance run', '', '**Full GUI acceptance: INCOMPLETE.**', '',
             f"Run mode: {receipt['mode']}. Registered checks: {len(cases)}. Results: {receipt['counts']}.", '',
             'Test success reflects existing executable assertions. Runtime observations, changes and screenshots do not by themselves establish a checked outcome for each control.', '',
             '[NASA test plan](' + NASA_PLAN + ') requires widget and requirements coverage, invalid/boundary inputs, end-to-end, stress, and graphical checks.', '',
             '## Mechanisms', '', '| Local ID | Mechanism | Passed / registered | Status |', '| --- | --- | --- | --- |']
    if receipt.get('snapshot_manifest'):
        position = lines.index('## Mechanisms')
        lines[position:position] = [
            'Project executables and inputs are preserved in [the snapshot manifest](' +
            os.path.relpath(receipt['snapshot_manifest'], output) + '). Evidence describes that preserved version; later checkout changes are not included.',
            'System tools/libraries and compiler provenance remain outside this snapshot guarantee.', '',
            'Execution: ' + str(receipt.get('execution_counts', {})) +
            '; snapshot unchanged: ' + str(receipt.get('snapshot_unchanged', 'run in progress')) + '.', '']
    for item in cov['mechanisms']:
        lines.append(f"| {item['id']} | {item['title']} | {len(item['passed_tests'])}/{len(item['registered_tests'])} | {item['status']} |")
    lines += ['', '## Cases', '', '| Test | Result | Route | Evidence |', '| --- | --- | --- | --- |']
    for result in receipt['results']:
        log = os.path.relpath(result['log'], output) if result.get('log') else None
        evidence = f'[log]({log})' if log else 'not executed'
        status = result['status'] + (' (reused, evidence verified)' if result.get('reused_from') else '')
        lines.append(f"| {result['name']} | {status} | {result.get('requested_visibility', 'not run')} | {evidence} |")
    lines += ['', '## Failing checks', '']
    failures = [result for result in receipt['results'] if result['status'] in ('failed', 'timeout', 'interrupted')]
    if not failures:
        lines.append('No failing checks recorded. Unexecuted cases and coverage gaps still apply.')
    for result in failures:
        lines.append('- **' + result['name'] + '**: ' + ' / '.join(result.get('failure_summary') or [result['status']]) )
    lines += ['', '## Coverage gaps', '']
    for gap in cov['gaps']:
        lines.append(f"- **{gap['id']}**: {gap['detail']}")
        if gap.get('tests'):
            lines.append('  Tests: ' + ', '.join(gap['tests']))
        if gap.get('controls'):
            lines.append(f"  {len(gap['controls'])} source locations; see control-coverage.json.")
    if nasa is not None:
        lines += ['', 'NASA requirement instances and historical operations: ' + str(nasa_report['counts']) +
                  '. See [requirement traceability](nasa-traceability.md) and nasa-requirements.json. Duplicate/deferred IDs remain separate source instances.', '']
        for key, label in [('requirement_scopes', 'Requirement scopes'),
                           ('historical_operation_scopes', 'Historical operation scopes')]:
            item = cov['estimate'][key]
            if item['percent'] is not None:
                lines.append(f'{label}: **{item["verified"]}/{item["total"]} = {item["percent"]:.1f}%**.')
        lines += ['', cov['estimate']['limit'], '']
    legacy_status = str(nasa_report['counts']['historical_cases']) if nasa is not None else 'individual cases remain unmapped'
    lines += ['', f'NASA historical matrix: {len(legacy)} cases / {len({x["object"] for x in legacy})} object groups. Current reviewed operation evidence: {legacy_status}. Historical pass marks are never current Qt results.', '',
              'Named-control source inventory: ' + str(len(controls)) + ' literal declarations. This omits dynamic names, unnamed widgets and platform-native controls; it is not a denominator for a full-GUI coverage percentage.', '',
              'See results.json, junit.xml, mechanisms.csv, control-coverage.json, nasa-historical-matrix.json and each case directory for exact commands, hashes, logs, traces and available screenshots.', '']
    (output / 'report.md').write_text('\n'.join(lines), encoding='utf-8')
    combined_junit(output, receipt['results'])
    return cov


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=REPO / 'build/linux-gui')
    parser.add_argument('--config', default='Release')
    parser.add_argument('--visible', action='store_true', help='Watch test windows (Linux default: visible test desktop); preserve explicit offscreen-only cases')
    parser.add_argument('--display-route', choices=['nested', 'native'], default='nested' if sys.platform.startswith('linux') else 'native', help='Visible mode: a watchable Xephyr test desktop (Linux default), or windows directly on the existing desktop')
    parser.add_argument('--display-scale', choices=['auto', 'test', 'system'], default='auto', help='Visible mode: host-aware scale/size by default; test retains the controlled 1x/1600x1200 baseline; system is an alias for auto')
    parser.add_argument('--scale', type=scale_value, default=None, metavar='auto|FACTOR', help='Visible UI scale, auto or 0.5..4, using the tutorial runner host-scale policy; explicit per-test factors remain')
    parser.add_argument('--width', type=int, help='Initial test-desktop pixel width, 640..4096')
    parser.add_argument('--height', type=int, help='Initial test-desktop pixel height, 480..3072')
    parser.add_argument('--fixed-size', action='store_true', help='Disable test-desktop resizing')
    parser.add_argument('--start-minimized', action='store_true', help='Start the outer nested test desktop minimized; restore it to watch without changing inner test windows')
    parser.add_argument('--pause-ms', type=int, default=350, help='Visible checkpoint delay, 0..5000 ms')
    parser.add_argument('--max-checkpoints', type=int, default=24, help='Bound pacing overhead per test, 0..5000')
    parser.add_argument('--max-screenshots', type=int, default=12, help='Maximum visible snapshots per process, 0..1000')
    parser.add_argument('--keep-passed-screenshots', action='store_true', help='Keep automatic observer PNGs for passing cases; default keeps failures and direct visual assertion images')
    parser.add_argument('--visible-timeout-factor', type=float, default=3, help='Visible native execution allowance, 1..20 times the CTest timeout, plus viewing pauses; assertions remain unchanged')
    parser.add_argument('--filter', default='.', help='Regex selecting test names; omitted tests remain coverage gaps')
    parser.add_argument('--output', type=Path, help='New result directory; default creates a temporary directory')
    parser.add_argument('--list', action='store_true', help='Write plan and gap reports without executing tests')
    parser.add_argument('--build', action='store_true', help='Build all acceptance executables first')
    preserved = parser.add_mutually_exclusive_group()
    preserved.add_argument('--snapshot', action='store_true', help='Copy project executables, runtime data, fixtures and registrations before running')
    preserved.add_argument('--resume', type=Path, help='Continue a preserved run, reusing only verified passing cases with matching execution settings')
    parser.add_argument('--max-cases', type=int, help='Run at most this many pending cases; requires --snapshot or --resume (exit 3 when more selected cases remain)')
    parser.add_argument('--coverage-interval-seconds', type=int, default=60, help='Print reviewed NASA coverage at completed-case boundaries, at most once per interval; 0 disables periodic lines (final estimate remains)')
    parser.add_argument('--strict-coverage', action='store_true', help='Exit 2 while coverage gaps remain, even when regression checks pass')
    args = parser.parse_args(argv)
    if args.start_minimized and (not args.visible or args.display_route != 'nested'):
        parser.error('--start-minimized requires --visible with --display-route nested')
    if (args.snapshot or args.resume) and not sys.platform.startswith('linux'):
        parser.error('Preserved/resumable runs currently support Linux; use the ordinary runner on other platforms')
    if args.resume and args.build:
        parser.error('--resume uses the preserved build; --build is not allowed')
    if args.max_cases is not None and (args.max_cases < 1 or not (args.snapshot or args.resume)):
        parser.error('--max-cases requires a positive value and --snapshot or --resume')
    if args.coverage_interval_seconds < 0:
        parser.error('--coverage-interval-seconds must be nonnegative')
    for key, lower, upper in [('width', 640, 4096), ('height', 480, 3072)]:
        value = getattr(args, key)
        if value is not None and not lower <= value <= upper:
            parser.error(f'--{key} must be between {lower} and {upper}')
    if not 1 <= args.visible_timeout_factor <= 20:
        parser.error('--visible-timeout-factor must be between 1 and 20')
    for key, upper in [('pause_ms', 5000), ('max_checkpoints', 5000), ('max_screenshots', 1000)]:
        if not 0 <= getattr(args, key) <= upper:
            parser.error(f'--{key.replace("_", "-")} must be between 0 and {upper}')
    try:
        selected = re.compile(args.filter)
    except re.error as error:
        parser.error(str(error))
    args.build_dir = args.build_dir.resolve()
    if args.visible and not args.list:
        if os.name != 'nt':
            if not os.environ.get('DISPLAY'):
                parser.error('--visible requires DISPLAY in your logged-in desktop terminal; no private display is substituted')
            try:
                probe = subprocess.run(['xdpyinfo', '-display', os.environ['DISPLAY']], capture_output=True, timeout=5)
            except (OSError, subprocess.TimeoutExpired) as error:
                parser.error('Cannot probe existing desktop: ' + str(error))
            if probe.returncode:
                parser.error('Cannot access existing desktop DISPLAY: ' + probe.stderr.decode(errors='replace'))
    if args.build:
        subprocess.run(['cmake', '--build', str(args.build_dir), '--config', args.config,
                        '--target', 'build-qt-acceptance', '--parallel', '2'], check=True)
    display_probe = args.build_dir / 'src/qtgui/GmatQtAcceptanceProbe'
    if args.resume:
        try:
            prior = json.loads((args.resume / 'results.json').read_text())
            preserved = json.loads(Path(prior['snapshot_manifest']).read_text())
            AcceptanceSnapshot.verify(preserved)
            display_probe = Path(preserved['ctest_definition']).parent / 'GmatQtAcceptanceProbe'
        except (OSError, ValueError, KeyError) as error:
            parser.error('Cannot read preserved display probe: ' + str(error))
    configure_display(args, display_probe)
    snapshot, snapshot_path, reused = None, None, {}
    evidence_repo = REPO
    if args.resume:
        try:
            snapshot, snapshot_path, reused = resume_state(args.resume, args)
        except (OSError, ValueError, KeyError) as error:
            parser.error(str(error))
        evidence_repo = Path(snapshot['root'])
        cases = snapshot['cases']
    else:
        query = subprocess.run(['ctest', '--test-dir', str(args.build_dir), '-C', args.config,
                                '--show-only=json-v1', '-L', '^qt-gui$'], capture_output=True, text=True, check=True)
        tests = json.loads(query.stdout)['tests']
        if not tests:
            parser.error('No qt-gui tests registered; configure GMAT_QT_BUILD_TESTS=ON')
        cases = inventory(REPO, tests)
    if not any(selected.search(case['name']) for case in cases):
        parser.error('--filter matches no registered test; nothing was run')
    selected_names = {case['name'] for case in cases if selected.search(case['name'])}
    if snapshot:
        try:
            AcceptanceSnapshot.validate_selection(snapshot, selected_names)
        except ValueError as error:
            parser.error(str(error))
    if not args.list and not snapshot:
        try:
            validate_selected_executables(evidence_repo, cases, selected_names)
        except ValueError as error:
            parser.error(str(error))
    output = args.output.resolve() if args.output else Path(tempfile.mkdtemp(prefix='gmat-qt-acceptance-'))
    if args.output:
        output.mkdir(parents=True, exist_ok=False)
    contract = json.loads((evidence_repo / 'src/qtgui/tests/AcceptanceContract.json').read_text(encoding='utf-8'))
    nasa_inventory = NasaGuiCoverage.inventory(evidence_repo)
    nasa = (nasa_inventory, NasaGuiCoverage.load_review(evidence_repo, nasa_inventory))
    source_files = input_sources(evidence_repo, cases, contract, nasa)
    if display_probe.is_file() and display_probe.is_relative_to(evidence_repo):
        source_files.add(str(display_probe.relative_to(evidence_repo)))
    if args.snapshot:
        print('Preserving project executables, test inputs and registrations...', flush=True)
        try:
            snapshot, snapshot_path = AcceptanceSnapshot.create(
                REPO, args.build_dir, cases, source_files, output,
                preserve_cases=selected_names, cache_dir=args.build_dir/'acceptance-input-cache')
        except (OSError, ValueError) as error:
            parser.error(str(error))
        evidence_repo = Path(snapshot['root'])
        cases = snapshot['cases']
    if snapshot:
        args.snapshot_id = snapshot['snapshot_id']
        args.snapshot_mapping = (snapshot['original_repo'], snapshot['root'])
        args.ctest_definition = snapshot['ctest_definition']
        # Snapshotted Python helpers derive their own repository root from __file__.
        contract = json.loads((evidence_repo / 'src/qtgui/tests/AcceptanceContract.json').read_text())
        nasa_inventory = NasaGuiCoverage.inventory(evidence_repo)
        nasa = (nasa_inventory, NasaGuiCoverage.load_review(evidence_repo, nasa_inventory))
    controls, legacy = named_controls(evidence_repo, cases), legacy_requirements(evidence_repo)
    binaries = {arg for case in cases for arg in case.get('command', [])
                if Path(arg).name.startswith('GmatQt') and Path(arg).is_file()}
    receipt = {'schema_version': 1, 'started_utc': utc(), 'mode': 'visible' if args.visible else 'headless',
               'runner_pid': os.getpid(),
               'build_dir': str(args.build_dir), 'config': args.config, 'filter': args.filter,
               'display': os.environ.get('DISPLAY') if args.visible else None,
               'display_scale': args.display_scale if args.visible else 'CTest private/offscreen display',
               'display_route': args.display_route if args.visible else 'CTest private/offscreen display',
               'ui_scale': args.scale, 'ui_scale_source': args.scale_source,
               'initial_screen': [args.width, args.height],
               'resizable': not args.fixed_size, 'host_display_info': args.host_display,
               'pause_ms': args.pause_ms, 'max_checkpoints': args.max_checkpoints, 'max_screenshots': args.max_screenshots,
               'visible_timeout_factor': args.visible_timeout_factor,
               'dialog_driver': 'Qt QFileDialog widgets; native portal qualification remains open; per-case evidence defines input delivery',
               'rendering': 'native cases use software OpenGL; no hardware GPU/compositor qualification',
               'driver': 'CTest + frontend probes; per-case evidence distinguishes Qt events from explicitly qualified native input',
               'contract_sha256': sha(evidence_repo / 'src/qtgui/tests/AcceptanceContract.json'), 'runner_sha256': sha(__file__),
               'startup_sha256': sha(evidence_repo / 'application/bin/gmat_startup_qt.txt'),
               'nasa_matrix_sha256': sha(evidence_repo / NASA_MATRIX),
               'ctest_definition_sha256': sha(Path(args.ctest_definition)) if snapshot else sha(args.build_dir / 'src/qtgui/CTestTestfile.cmake'),
               'source_hashes': {name: sha(evidence_repo / name) for name in sorted(source_files)},
               'binary_hashes': {name: sha(name) for name in sorted(binaries)},
               'provenance_limit': 'Hashes identify observed files, not proof that a binary was built from those sources.',
               'results': []}
    if snapshot:
        receipt.update(snapshot_id=snapshot['snapshot_id'], snapshot_manifest=str(snapshot_path),
                       source_archive_root=str(evidence_repo), execution_profile=AcceptanceSnapshot.profile(args),
                       preserved_input_limit=snapshot['limit'], resumed_from=str(args.resume.resolve()) if args.resume else None)
    write_json(output / 'plan.json', cases)
    report(output, receipt, cases, controls, contract, legacy, nasa, evidence_repo)
    print('Acceptance results: ' + str(output), flush=True)
    print(f'{len(cases)} registered checks; visibility routes: {dict(Counter(c["visibility"] for c in cases))}', flush=True)
    env = dict(os.environ, PYTHONDONTWRITEBYTECODE='1',
               GMAT_QT_ACCEPTANCE_PAUSE_MS=str(args.pause_ms),
               GMAT_QT_ACCEPTANCE_MAX_CHECKPOINTS=str(args.max_checkpoints),
               GMAT_QT_ACCEPTANCE_MAX_SCREENSHOTS=str(args.max_screenshots))
    if snapshot:
        path_name = 'PATH' if os.name == 'nt' else 'DYLD_LIBRARY_PATH' if sys.platform == 'darwin' else 'LD_LIBRARY_PATH'
        env[path_name] = os.pathsep.join([str(evidence_repo / 'application/bin'),
                                        str(evidence_repo / 'application/plugins'), env.get(path_name, '')])
    if args.visible:
        env['LIBGL_ALWAYS_SOFTWARE'] = '1'
    stop_display = None
    if args.visible and not args.list and args.display_route == 'nested':
        from VisibleTestDisplay import start
        try:
            env, receipt['visible_display'], stop_display = start(output / 'display', env,
                width=args.width, height=args.height, resizable=not args.fixed_size,
                start_minimized=args.start_minimized)
        except (OSError, RuntimeError, subprocess.TimeoutExpired) as error:
            parser.error(str(error))
        print('Watch the GMAT GUI acceptance live test desktop window.', flush=True)
    # Never inherit a previous runner's private startup or mode into a new run.
    for key in ('GMAT_QT_ACCEPTANCE_STARTUP', 'GMAT_QT_ACCEPTANCE_STARTUP_ORIGINAL', 'GMAT_QT_ACCEPTANCE_MODE', 'GMAT_QT_ACCEPTANCE_DIR'):
        env.pop(key, None)
    interrupted = False
    limited = False
    executed = 0
    progress = CoverageProgress(args.coverage_interval_seconds)
    try:
        for index, case in enumerate(cases, 1):
            if args.list or not selected.search(case['name']) or interrupted:
                receipt['results'].append({'name': case['name'], 'status': 'not-run', 'reason': 'inventory only, filtered, or interrupted'})
                continue
            if case['name'] in reused:
                receipt['results'].append(reused[case['name']])
                progress.emit(report(output, receipt, cases, controls, contract, legacy, nasa, evidence_repo))
                continue
            if args.max_cases is not None and executed >= args.max_cases:
                limited = True
                receipt['results'].append({'name': case['name'], 'status': 'not-run', 'reason': 'requested case limit'})
                continue
            route = 'visible' if args.visible and case['visibility'] == 'visible' else 'headless'
            print(f'[{index}/{len(cases)}] {case["name"]} ({route})', flush=True)
            directory = output / re.sub(r'[^A-Za-z0-9_.-]', '_', case['name'])
            result = execute(case, directory, args, env)
            receipt['results'].append(result)
            executed += 1
            interrupted = result['status'] == 'interrupted'
            print(f'  {result["status"].upper()} {result["duration_seconds"]}s; {len(result["trace"]["screenshots"])} snapshots', flush=True)
            if result['status'] in ('failed', 'timeout'):
                print(Path(result['log']).read_text(errors='replace')[-6000:], flush=True)
            progress.emit(report(output, receipt, cases, controls, contract, legacy, nasa, evidence_repo))
    except KeyboardInterrupt:
        interrupted = True
        completed = {item['name'] for item in receipt['results']}
        receipt['results'].extend({'name': case['name'], 'status': 'not-run', 'reason': 'interrupted between cases'}
                                  for case in cases if case['name'] not in completed)
    finally:
        if stop_display:
            stop_display()
    receipt['finished_utc'] = utc()
    if receipt.get('visible_display', {}).get('resize_errors'):
        receipt['display_error'] = '; '.join(receipt['visible_display']['resize_errors'])
    receipt['execution_counts'] = {'executed_now': executed, 'reused_passes': sum('reused_from' in r for r in receipt['results'])}
    receipt['end_reason'] = 'interrupted' if interrupted else 'case-limit' if limited else 'completed-selection'
    if snapshot:
        try:
            AcceptanceSnapshot.verify(snapshot)
            receipt['snapshot_unchanged'] = True
        except (OSError, ValueError) as error:
            receipt['snapshot_unchanged'] = False
            receipt['snapshot_error'] = str(error)
    cov = report(output, receipt, cases, controls, contract, legacy, nasa, evidence_repo)
    progress.emit(cov, force=True)
    print(f'Regression results: {receipt["counts"]}. Full GUI acceptance: INCOMPLETE ({len(cov["gaps"])} gap categories).', flush=True)
    print('Read ' + str(output / 'report.md'), flush=True)
    if not args.keep_passed_screenshots:
        pruning = ScreenshotRetention.prune_receipt(output/'results.json', allowed_root=output.parent)
        if pruning['files']:
            print(f'Removed {pruning["files"]} automatic passing-case PNGs; failure images and direct visual oracles retained.', flush=True)
    if interrupted:
        return 130
    if snapshot and not receipt['snapshot_unchanged']:
        return 1
    if receipt.get('display_error'):
        return 1
    if any(x['status'] in ('failed', 'timeout') for x in receipt['results']):
        return 1
    if limited:
        return 3
    if args.strict_coverage and not cov['acceptance_complete']:
        return 2
    return 0


if __name__ == '__main__':
    def interrupt_run(signum, frame):
        raise KeyboardInterrupt
    signal.signal(signal.SIGTERM, interrupt_run)
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        sys.exit(130)
