"""Runner contracts: no success from missing/skipped output or coverage inference."""
import argparse
from contextlib import redirect_stdout, redirect_stderr
import io
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import unittest
from types import SimpleNamespace
from unittest.mock import patch
from unittest.mock import MagicMock

import RunAcceptance as runner
from AcceptanceRuntime import arguments, environment, isolate_startup

PROBE = Path(sys.argv.pop(1)).resolve() if len(sys.argv) > 1 and not sys.argv[1].startswith('-') else None


class RunnerContracts(unittest.TestCase):
    def test_selected_compiled_targets_must_exist_before_snapshot(self):
        with tempfile.TemporaryDirectory() as directory:
            repo = Path(directory)
            cmake = repo / 'src/qtgui/CMakeLists.txt'
            cmake.parent.mkdir(parents=True)
            cmake.write_text('add_executable(GmatQtSelected tests/Selected.cpp)\n'
                             'add_executable(GmatQtUnselected tests/Other.cpp)\n')
            selected = repo / 'build/GmatQtSelected'
            ignored = repo / 'build/GmatQtUnselected'
            cases = [{'name':'QtGui.Selected', 'command':['python3', 'RunNativeTest.py', '/usr/bin/cmake', '-E', 'env', str(selected)]},
                     {'name':'QtGui.Other', 'command':[str(ignored)]}]
            with self.assertRaisesRegex(ValueError, 'Build the selected acceptance executables'):
                runner.validate_selected_executables(repo, cases, {'QtGui.Selected'})
            selected.parent.mkdir(parents=True)
            selected.write_text('#!/bin/sh\nexit 0\n')
            selected.chmod(0o700)
            cases[0]['command'].extend([str(selected.with_suffix('.png')), str(selected.with_suffix('.script'))])
            runner.validate_selected_executables(repo, cases, {'QtGui.Selected'})
            selected.chmod(0o600)
            with self.assertRaisesRegex(ValueError, 'QtGui.Selected'):
                runner.validate_selected_executables(repo, cases, {'QtGui.Selected'})
            runner.validate_selected_executables(repo, cases, set())

    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix='gmat acceptance contract ')
        self.root = Path(self.temporary.name)
        self.addCleanup(self.temporary.cleanup)

    def xml(self, contents):
        path = self.root / 'result.xml'
        path.write_text(contents)
        return path

    def test_snapshot_retains_unverified_conflict_sources(self):
        import AcceptanceSnapshot as snapshot
        source = 'src/base/solarsys/Deprecated.cpp'
        review = {'reviews': [], 'source_conflicts': [
            {'id': 'removed-mode', 'reference_sha256': {source: 'reviewed-digest'}}]}
        files = runner.input_sources(self.root, [], {'mechanisms': []}, ({}, review))
        self.assertIn(source, files)
        for relative in files:
            target = self.root / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text('preserved source for ' + relative)
        build = self.root / 'build'
        definition = build / 'src/qtgui/CTestTestfile.cmake'
        definition.parent.mkdir(parents=True)
        definition.write_text('# empty fixture registrations\n')
        manifest, _ = snapshot.create(self.root, build, [], files, self.root / 'receipt')
        frozen = Path(manifest['root']) / source
        self.assertEqual(frozen.read_text(), (self.root / source).read_text())
        (self.root / source).write_text('later working-tree edit')
        snapshot.verify(manifest)
        self.assertNotEqual(frozen.read_text(), (self.root / source).read_text())

    def test_partial_evidence_inputs_are_frozen_without_parent_credit(self):
        import AcceptanceSnapshot as snapshot
        source = 'src/qtgui/tests/Partial.cpp'
        inputs = ['src/qtgui/tests/PartialCases.json', 'doc/help/Partial.xml']
        row = {'key': 'PARTIAL', 'applicability': 'gui', 'rationale': 'Incomplete',
               'evidence': [], 'partial_evidence': [{'source': source,
               'reference_sources': {p: 'pre-execution-binding' for p in inputs}}]}
        review = {'reviews': [row], 'source_conflicts': []}
        files = runner.input_sources(self.root, [], {'mechanisms': []}, ({}, review))
        for relative in files:
            target = self.root / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text('original immutable bytes for ' + relative)
        build = self.root / 'build'
        definition = build / 'src/qtgui/CTestTestfile.cmake'
        definition.parent.mkdir(parents=True)
        definition.write_text('# no executable fixture registrations\n')
        manifest, _ = snapshot.create(self.root, build, [], files, self.root / 'receipt')
        for relative in [source] + inputs:
            frozen = Path(manifest['root']) / relative
            self.assertEqual(frozen.read_text(), (self.root / relative).read_text())
            self.assertIn(relative, manifest['original_hashes'])
            (self.root / relative).write_text('later working-tree change')
        snapshot.verify(manifest)
        inventory = {'schema_version': 1, 'sources': {}, 'source_metadata': {},
                     'requirements': [{'key': 'PARTIAL'}],
                     'historical_cases': []}
        report = runner.NasaGuiCoverage.evaluate(self.root, inventory, review)
        self.assertEqual(report['requirements'][0]['status'], 'reviewed-unverified')
        self.assertEqual(report['counts']['requirements'].get('verified', 0), 0)

    def test_ctest_result_is_required(self):
        self.assertEqual(runner.junit_status(self.root / 'absent.xml', 0), 'failed')
        self.assertEqual(runner.junit_status(self.xml('<testsuite/>'), 0), 'failed')
        self.assertEqual(runner.junit_status(self.xml('<truncated'), 0), 'failed')
        self.assertEqual(runner.junit_status(self.xml('<testsuite><testcase/></testsuite>'), 1), 'failed')
        self.assertEqual(runner.junit_status(self.xml('<testsuite><testcase><failure/></testcase></testsuite>'), 0), 'failed')
        self.assertEqual(runner.junit_status(self.xml('<testsuite><testcase><skipped/></testcase></testsuite>'), 0), 'skipped')
        self.assertEqual(runner.junit_status(self.xml('<testsuite><testcase><skipped/></testcase></testsuite>'), 8), 'failed')
        self.assertEqual(runner.junit_status(self.xml('<testsuite><testcase/></testsuite>'), 0), 'passed')

    def test_private_startup_preserves_original_and_literal_paths(self):
        parent = self.root / 'installation/bin'; parent.mkdir(parents=True)
        original = parent / 'gmat_startup_qt.txt'
        (parent / 'GmatQt').write_text('launcher fixture')
        data = parent.parent / 'data'; data.mkdir(); (data / 'fixture.txt').write_text('truth')
        source = 'ROOT_PATH = ../\nOUTPUT_PATH = ../output/\nPLUGIN = ../plugins/libPlugin\nDE405_FILE = PLANETARY_EPHEM_DE_PATH/truth.405\n'
        original.write_text(source)
        private = isolate_startup(original, self.root / 'private with spaces')
        text = private.read_text()
        self.assertEqual(original.read_text(), source)
        self.assertIn('ROOT_PATH = ' + str(parent.parent) + '/', text)
        self.assertIn('PLUGIN = ' + str(parent.parent / 'plugins/libPlugin'), text)
        self.assertIn('DE405_FILE = PLANETARY_EPHEM_DE_PATH/truth.405', text)
        self.assertIn('PERSONALIZATION_FILE = ' + str(private.parent / 'MyGmat.ini'), text)
        self.assertEqual((private.parent / '../data/fixture.txt').read_text(), 'truth')
        self.assertEqual((private.parent / '../output').resolve(), (private.parent / 'output').resolve())
        self.assertEqual((private.parent / 'GmatQt').read_text(), 'launcher fixture')

    def test_startup_remap_only_matches_exact_path(self):
        original, private = str(self.root / 'original.txt'), str(self.root / 'private.txt')
        with patch.dict(os.environ, {'GMAT_QT_ACCEPTANCE_STARTUP_ORIGINAL': original, 'GMAT_QT_ACCEPTANCE_STARTUP': private}):
            self.assertEqual(arguments(['binary', original, 'fixture.txt']), ['binary', private, 'fixture.txt'])

    def test_default_platform_is_still_offscreen(self):
        with patch.dict(os.environ, {'QT_QPA_PLATFORM': 'wayland'}, clear=True):
            self.assertEqual(environment()['QT_QPA_PLATFORM'], 'offscreen')
        with patch.dict(os.environ, {'GMAT_QT_ACCEPTANCE_MODE': 'visible'}, clear=True):
            self.assertEqual(environment()['QT_QPA_PLATFORM'], 'windows' if os.name == 'nt' else 'xcb')

    def test_classification_honors_guards(self):
        source = self.root / 'Test.cpp'
        case = {'name': 'QtGui.Example', 'command': ['python', 'RunTest.py']}
        source.write_text('QApplication app; if (QGuiApplication::platformName()!="offscreen") return 2;')
        self.assertEqual(runner.classify(case, ['Test.cpp'], self.root)[0], 'headless-only')
        source.write_text('QApplication app; if (!acceptanceWidgetPlatform()) return 2;')
        self.assertEqual(runner.classify(case, ['Test.cpp'], self.root)[0], 'visible')
        source.write_text('QApplication app; if (platformName()=="wayland") fail("use offscreen");')
        self.assertEqual(runner.classify(case, ['Test.cpp'], self.root)[0], 'visible')
        source.write_text('int main() { parse(); }')
        self.assertEqual(runner.classify(case, ['Test.cpp'], self.root)[0], 'support')

    def test_explicit_offscreen_case_is_not_promoted_with_its_companion(self):
        source = self.root / 'Test.cpp'
        source.write_text('QApplication app; require(acceptanceWidgetPlatform());')
        base = {'name': 'QtGui.Regression', 'command': ['python', 'RunTest.py'],
                'properties': [{'name': 'LABELS', 'value': ['qt-acceptance-offscreen']}]}
        self.assertEqual(runner.classify(base, ['Test.cpp'], self.root)[0], 'headless-only')
        base['properties'] = []
        self.assertEqual(runner.classify(base, ['Test.cpp'], self.root)[0], 'visible')

    def test_minimized_start_targets_only_new_host_window(self):
        import VisibleTestDisplay as display
        with patch.object(display.shutil, 'which', return_value='/fixture/tool'), \
             patch.object(display, 'host_windows', return_value={0x11, 0x22}), \
             patch.object(display, 'iconify') as minimize, \
             patch.object(display.subprocess, 'run', return_value=SimpleNamespace(stdout='window state: Iconic')):
            result = display.minimize_new_window({'DISPLAY': ':host'}, {0x11})
        minimize.assert_called_once_with(0x22, {'DISPLAY': ':host'})
        self.assertTrue(result['start_minimized'])
        self.assertEqual(result['host_window'], '0x22')

    def test_minimized_start_requires_nested_visible_display(self):
        for args in (['--start-minimized', '--list'],
                     ['--visible', '--display-route', 'native', '--start-minimized', '--list']):
            with redirect_stderr(io.StringIO()), self.assertRaises(SystemExit) as error:
                runner.main(args)
            self.assertEqual(error.exception.code, 2)

    @unittest.skipIf(PROBE is None, 'CMake supplies the real observer probe executable')
    def test_view_boundary_preserves_unattended_and_rejects_fake_exposure(self):
        env = dict(os.environ, QT_QPA_PLATFORM='offscreen')
        env.pop('GMAT_QT_ACCEPTANCE_MODE', None)
        env.pop('GMAT_QT_ACCEPTANCE_DIR', None)
        result = subprocess.run([str(PROBE), '--view-contract'], env=env, capture_output=True, text=True, timeout=10)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('visible=0', result.stdout)
        env['GMAT_QT_ACCEPTANCE_MODE'] = 'visible'
        result = subprocess.run([str(PROBE), '--view-contract'], env=env, capture_output=True, text=True, timeout=10)
        self.assertEqual(result.returncode, 3)
        self.assertIn('native window platform', result.stderr)

    def test_tutorial_transaction_checks_are_not_gui_replays(self):
        source = self.root / 'Test.cpp'
        source.write_text('QApplication app; MainWindow window; window.applyResourceChanges();')
        for name in ('QtTutorials.OrbitRoundTrip', 'QtTutorials.ForceOrder'):
            case = {'name': name, 'command': ['python', 'RunTest.py'],
                    'properties': [{'name': 'LABELS', 'value': ['qt-gui', 'qt-acceptance-transaction']}]}
            route, reason = runner.classify(case, ['Test.cpp'], self.root)
            self.assertEqual(route, 'support')
            self.assertIn('transaction', reason)
            self.assertIn('does not open widgets', reason)

    def test_observed_control_is_never_counted_as_asserted(self):
        contract = {'mechanisms': [{'id': 'LOCAL', 'tests': 'QtGui.One', 'title': 'one', 'expected': 'one', 'source': 'source'}],
                    'known_gaps': [], 'coverage_limits': []}
        cases = [{'name': 'QtGui.One', 'visibility': 'visible'}, {'name': 'QtGui.New', 'visibility': 'visible'}]
        result = {'name': 'QtGui.One', 'status': 'passed', 'requested_visibility': 'visible',
                  'trace': {'named_objects': {'field': {'events': ['observed', 'changed']}}, 'native_exposure_events': 1}}
        cov = runner.coverage(cases, [result], [{'name': 'field', 'source': 'source', 'line': 1}], contract)
        self.assertFalse(cov['acceptance_complete'])
        self.assertIn('not established', cov['named_controls'][0]['assertion_coverage'])
        self.assertEqual(cov['named_controls'][0]['activated_in'], [])
        self.assertIn('UNMAPPED-TESTS', [gap['id'] for gap in cov['gaps']])
        self.assertIn('UNEXECUTED', [gap['id'] for gap in cov['gaps']])

    def test_estimate_keeps_unverified_scopes_in_separate_denominators(self):
        cov = {'nasa_counts': {'requirements': {'verified': 3, 'unreviewed': 5, 'mapped-unverified': 2},
                               'historical_cases': {'verified': 1, 'unreviewed': 3}},
               'mechanisms': [{'passed_tests': list(range(100))}]}
        estimate = runner.coverage_estimate(cov)
        self.assertEqual(estimate['requirement_scopes'], {'verified': 3, 'total': 10, 'percent': 30.0})
        self.assertEqual(estimate['historical_operation_scopes'], {'verified': 1, 'total': 4, 'percent': 25.0})
        self.assertIsNone(runner.coverage_estimate({})['requirement_scopes']['percent'])

    def test_periodic_estimate_is_throttled_and_final_always_reports(self):
        now = [0]
        progress = runner.CoverageProgress(60, clock=lambda: now[0])
        output = io.StringIO()
        with redirect_stdout(output):
            progress.emit({})
            now[0] = 59
            progress.emit({})
            now[0] = 60
            progress.emit({})
            progress.emit({}, force=True)
        self.assertEqual(output.getvalue().count('NASA coverage estimate:'), 3)
        self.assertIn('requirement scopes: unavailable', output.getvalue())
        output = io.StringIO()
        with redirect_stdout(output):
            disabled = runner.CoverageProgress(0)
            disabled.emit({})
            self.assertEqual(output.getvalue(), '')
            disabled.emit({}, force=True)
        self.assertEqual(output.getvalue().count('NASA coverage estimate:'), 1)

    def test_acceptance_uses_tutorial_host_scale_and_explicit_geometry(self):
        host = {'scale': 3.0, 'logical_size': [1536, 960], 'available_logical_size': [1469, 928]}
        args = SimpleNamespace(visible=True, display_scale='auto', scale=None, width=None, height=None)
        with patch('RunTutorials.host_display_info', return_value=host):
            runner.configure_display(args, Path('/unused/probe'))
        self.assertEqual(args.scale, 3.0)
        self.assertEqual(args.scale_source, 'host display')
        self.assertGreater(args.width, 2560)
        args = SimpleNamespace(visible=True, display_scale='test', scale=None, width=None, height=None)
        with patch('RunTutorials.host_display_info', return_value=host):
            runner.configure_display(args, Path('/unused/probe'))
        self.assertEqual((args.scale, args.width, args.height), (1.0, 1600, 1200))
        args = SimpleNamespace(visible=True, display_scale='auto', scale=1.5, width=2560, height=1600)
        with patch('RunTutorials.host_display_info', return_value=host):
            runner.configure_display(args, Path('/unused/probe'))
        self.assertEqual((args.scale, args.width, args.height), (1.5, 2560, 1600))

    def test_source_inventory_is_only_candidate_linkage(self):
        directory = self.root / 'src/qtgui'; directory.mkdir(parents=True)
        (directory / 'Panel.cpp').write_text('button->setObjectName("clickMe"); other->setObjectName("dynamic" + value);')
        (self.root / 'Check.cpp').write_text('findChild("clickMe");')
        rows = runner.named_controls(self.root, [{'name': 'QtGui.One', 'sources': ['Check.cpp']}])
        self.assertEqual(len(rows), 1)
        self.assertEqual(rows[0]['candidate_tests'], ['QtGui.One'])
        self.assertIn('does not establish', rows[0]['candidate_limit'])

    def test_truncated_trace_retains_completed_observations(self):
        (self.root / 'trace-1.jsonl').write_text('{"event":"start"}\n{"event":"changed","name":"input","class":"QLineEdit"}\n{"event":')
        result = runner.trace_summary(self.root)
        self.assertEqual(result['instrumented_processes'], 1)
        self.assertEqual(result['named_objects']['input']['events'], ['changed'])

    def test_offscreen_exposure_cannot_be_promoted_to_native_visibility(self):
        path = self.root / 'trace-1.jsonl'
        path.write_text('{"event":"native_exposure","exposed":true}\n')
        result = runner.trace_summary(self.root)
        self.assertEqual(result['qt_window_exposure_events'], 1)
        self.assertEqual(result['native_exposure_events'], 0)
        with path.open('a') as stream:
            stream.write('{"event":"checkpoint","platform":"offscreen"}\n')
        self.assertEqual(runner.trace_summary(self.root)['native_exposure_events'], 0)
        path.write_text('{"event":"native_exposure","exposed":true}\n{"event":"checkpoint","platform":"xcb"}\n')
        self.assertEqual(runner.trace_summary(self.root)['native_exposure_events'], 1)

    def test_every_registered_source_case_has_explicit_mechanism_mapping(self):
        if PROBE is None:
            self.skipTest('The configured CTest registry is supplied with the observer probe')
        query = subprocess.run(['ctest', '--test-dir', str(PROBE.parents[2]), '--show-only=json-v1'],
                               check=True, capture_output=True, text=True, timeout=10)
        # Use expanded registrations, including each foreach case. Parsing CMake
        # source mistakes variable prefixes for test names and omits real cases.
        registry = json.loads(query.stdout)['tests']
        names = {case['name'] for case in registry
                 if case['name'].startswith('QtGui.') or
                 (case['name'].startswith('QtTutorials.') and 'RunTest.py' in ' '.join(case.get('command', [])))}
        contract = json.loads((runner.HERE / 'AcceptanceContract.json').read_text())
        self.assertTrue(names)
        self.assertEqual([name for name in sorted(names) if not any(re.search(item['tests'], name) for item in contract['mechanisms'])], [])
        for item in contract['mechanisms']:
            self.assertTrue((runner.REPO / item['source']).is_file(), item['source'])

    def test_native_launcher_keeps_xvfb_unless_visible(self):
        command = ['python', str(runner.HERE / 'RunNativeTest.py'), 'xvfb-run', 'cmake', '-E', 'env', 'QT_QPA_PLATFORM=xcb', 'test']
        # Execute the module's main without starting any display or GUI process.
        import RunNativeTest
        with patch.object(sys, 'argv', command[1:]), patch.dict(os.environ, {}, clear=True), patch.object(RunNativeTest.subprocess, 'call', return_value=0) as call:
            self.assertEqual(RunNativeTest.main(), 0)
            self.assertEqual(call.call_args.args[0][:2], ['xvfb-run', '-a'])
            child = call.call_args.kwargs['env']
            self.assertEqual(child['GMAT_QT_NATIVE_TEST_LAUNCH'], '1')
            self.assertNotIn('GMAT_QT_ACCEPTANCE_MODE', child)
            self.assertNotIn('GMAT_QT_NATIVE_TEST_LAUNCH', os.environ)
        with patch.object(sys, 'argv', command[1:]), patch.dict(os.environ, {'GMAT_QT_ACCEPTANCE_MODE': 'visible', 'DISPLAY': ':example'}, clear=True), patch.object(RunNativeTest.subprocess, 'call', return_value=0) as call:
            self.assertEqual(RunNativeTest.main(), 0)
            self.assertEqual(call.call_args.args[0][0], 'cmake')
            child = call.call_args.kwargs['env']
            self.assertEqual(child['GMAT_QT_NATIVE_TEST_LAUNCH'], '1')
            self.assertEqual(child['GMAT_QT_ACCEPTANCE_MODE'], 'visible')
            self.assertEqual(child['DISPLAY'], ':example')
            self.assertNotIn('GMAT_QT_NATIVE_TEST_LAUNCH', os.environ)
        with patch.object(sys, 'argv', command[1:]), patch.dict(os.environ, {'GMAT_QT_ACCEPTANCE_MODE': 'visible'}, clear=True), patch.object(RunNativeTest.subprocess, 'call') as call:
            with self.assertRaisesRegex(SystemExit, 'existing desktop DISPLAY'):
                RunNativeTest.main()
            call.assert_not_called()

    @unittest.skipIf(PROBE is None, 'CMake supplies the real observer probe executable')
    def test_widget_platform_keeps_default_offscreen_and_rejects_unsupported(self):
        for platform, marker, expected in [('offscreen', '', True),
                                           ('minimal', '', False),
                                           ('minimal', '1', False)]:
            env = dict(os.environ, QT_QPA_PLATFORM=platform)
            for name in ('GMAT_QT_ACCEPTANCE_MODE', 'GMAT_QT_ACCEPTANCE_DIR',
                         'GMAT_QT_NATIVE_TEST_LAUNCH'):
                env.pop(name, None)
            if marker:
                env['GMAT_QT_NATIVE_TEST_LAUNCH'] = marker
            result = subprocess.run([str(PROBE), '--platform-contract'], env=env,
                                    capture_output=True, text=True, timeout=10)
            self.assertEqual(result.returncode, 0, result.stderr)
            info = json.loads(result.stdout)
            self.assertEqual(info, dict(platform=platform, mode='',
                                        native_launch=marker, accepted=expected))

    @unittest.skipUnless(PROBE is not None and sys.platform.startswith('linux')
                         and shutil.which('xvfb-run'),
                         'Native Linux guard contract requires the probe and private X11')
    def test_native_widget_guard_and_actual_launcher_preserve_execution_mode(self):
        # No windows or observer frames: this checks the real C++ predicate on
        # X11 and the actual launcher, independently of a mocked subprocess.
        import RunNativeTest
        cases = [('', '', False), ('', '0', False), ('', 'true', False),
                 ('', '1', True), ('headless', '1', True), ('visible', '', True)]
        for mode, marker, accepted in cases:
            env = dict(os.environ, QT_QPA_PLATFORM='xcb')
            for name in ('GMAT_QT_ACCEPTANCE_MODE', 'GMAT_QT_ACCEPTANCE_DIR',
                         'GMAT_QT_NATIVE_TEST_LAUNCH'):
                env.pop(name, None)
            if mode:
                env['GMAT_QT_ACCEPTANCE_MODE'] = mode
            if marker:
                env['GMAT_QT_NATIVE_TEST_LAUNCH'] = marker
            result = subprocess.run(['xvfb-run', '-a', str(PROBE), '--platform-contract'],
                                    env=env, capture_output=True, text=True, timeout=10)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(json.loads(result.stdout), dict(platform='xcb', mode=mode,
                             native_launch=marker, accepted=accepted))
        for mode in ('', 'headless', 'visible'):
            env = dict(os.environ, QT_QPA_PLATFORM='xcb')
            for name in ('GMAT_QT_ACCEPTANCE_MODE', 'GMAT_QT_ACCEPTANCE_DIR',
                         'GMAT_QT_NATIVE_TEST_LAUNCH'):
                env.pop(name, None)
            if mode:
                env['GMAT_QT_ACCEPTANCE_MODE'] = mode
            command = [sys.executable, str(Path(RunNativeTest.__file__).resolve()),
                       'xvfb-run', str(PROBE), '--platform-contract']
            if mode == 'visible':
                command = ['xvfb-run', '-a', *command]
            result = subprocess.run(command, env=env, capture_output=True,
                                    text=True, timeout=10)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(json.loads(result.stdout), dict(platform='xcb', mode=mode,
                             native_launch='1', accepted=True))

    def test_real_ctest_failure_skip_timeout_and_logs(self):
        source = self.root / 'project'; source.mkdir()
        script = source / 'case.py'
        script.write_text('import sys,time\nprint("fixture evidence", flush=True)\n'
                          'mode=sys.argv[1]\n'
                          'time.sleep(10) if mode=="Timed" else None\n'
                          'time.sleep(0.2) if mode=="Paced" else None\n'
                          'sys.exit({"Pass":0,"Fail":1,"Skip":77,"Timed":0,"Paced":0}[mode])\n')
        (source / 'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.21)\nproject(AcceptanceContract NONE)\nenable_testing()\n' +
            '\n'.join(f'add_test(NAME QtGui.{name} COMMAND "{Path(sys.executable).as_posix()}" "{script.as_posix()}" {name})'
                      for name in ('Pass', 'Fail', 'Skip', 'Timed', 'Paced')) +
            '\nset_tests_properties(QtGui.Skip PROPERTIES SKIP_RETURN_CODE 77)\n'
            'set_tests_properties(QtGui.Timed QtGui.Paced PROPERTIES TIMEOUT 0.1)\n' +
            f'add_test(NAME QtGui.Missing COMMAND "{(source / "missing-executable").as_posix()}")\n')
        build = self.root / 'build'
        subprocess.run(['cmake', '-S', str(source), '-B', str(build)], capture_output=True, check=True, timeout=10)
        args = SimpleNamespace(build_dir=build, config='Release', visible=False)
        for name, expected in [('Pass', 'passed'), ('Fail', 'failed'), ('Skip', 'skipped'), ('Timed', 'failed')]:
            result = runner.execute({'name': 'QtGui.' + name, 'sources': [], 'command': [], 'visibility': 'support'},
                                    self.root / name, args, dict(os.environ))
            self.assertEqual(result['status'], expected, result)
            self.assertIn('fixture evidence', Path(result['log']).read_text())
        missing = runner.execute({'name': 'QtGui.Missing', 'sources': [], 'command': [], 'visibility': 'support'},
                                 self.root / 'Missing', args, dict(os.environ))
        self.assertEqual(missing['status'], 'failed')
        self.assertIn('Could not find executable', Path(missing['log']).read_text())
        self.assertIn('Timeout', (self.root / 'Timed/test.log').read_text())
        paced = SimpleNamespace(build_dir=build, config='Release', visible=True, pause_ms=100, max_checkpoints=4)
        result = runner.execute({'name': 'QtGui.Paced', 'sources': [], 'command': [], 'visibility': 'visible',
                                 'properties': [{'name': 'TIMEOUT', 'value': 0.1}]}, self.root / 'Paced', paced, dict(os.environ))
        self.assertEqual(result['status'], 'passed')
        self.assertEqual(result['ctest_timeout_seconds'], 0.5)
        self.assertTrue((self.root / 'Paced/ctest/CTestTestfile.cmake').exists())

    def test_list_strict_coverage_and_empty_selection(self):
        query = SimpleNamespace(stdout=json.dumps({'tests': [{'name': 'QtGui.Desktop', 'command': []}]}))
        with patch.object(runner.subprocess, 'run', return_value=query), redirect_stdout(io.StringIO()):
            self.assertEqual(runner.main(['--list', '--output', str(self.root / 'normal')]), 0)
            self.assertEqual(runner.main(['--list', '--strict-coverage', '--output', str(self.root / 'strict')]), 2)
            with redirect_stdout(io.StringIO()), patch.object(sys, 'stderr', io.StringIO()), self.assertRaises(SystemExit) as error:
                runner.main(['--list', '--filter', '^absent$'])
            self.assertEqual(error.exception.code, 2)

    def test_snapshot_platform_limit_is_explicit_before_any_execution(self):
        message = io.StringIO()
        with patch.object(runner.sys, 'platform', 'win32'), redirect_stdout(io.StringIO()), patch.object(sys, 'stderr', message):
            with self.assertRaises(SystemExit) as error:
                runner.main(['--snapshot', '--list'])
        self.assertEqual(error.exception.code, 2)
        self.assertIn('currently support Linux', message.getvalue())

    def test_visible_display_owns_private_auth_and_only_its_server(self):
        import VisibleTestDisplay
        process = MagicMock(); process.poll.return_value = None
        host = {'DISPLAY': ':host', 'XAUTHORITY': '/host/authority'}
        with patch.object(VisibleTestDisplay.shutil, 'which', return_value='/fixture/tool'), \
             patch.object(VisibleTestDisplay.subprocess, 'run', return_value=SimpleNamespace(returncode=0)) as auth_call, \
             patch.object(VisibleTestDisplay.secrets, 'token_hex', return_value='c' * 32), \
             patch.object(VisibleTestDisplay.subprocess, 'Popen', return_value=process) as launch, \
             patch.object(VisibleTestDisplay.select, 'select', return_value=([1], [], [])), \
             patch.object(VisibleTestDisplay.os, 'read', return_value=b'72\n'), \
             patch.object(VisibleTestDisplay.atexit, 'register'):
            env, receipt, stop = VisibleTestDisplay.start(self.root / 'display', host)
            self.assertEqual(host['DISPLAY'], ':host')
            self.assertEqual(env['DISPLAY'], ':72')
            self.assertEqual(launch.call_args.kwargs['env']['XAUTHORITY'], '/host/authority')
            self.assertEqual((self.root / 'display/Xauthority').stat().st_mode & 0o777, 0o600)
            self.assertIn('-nolisten', launch.call_args.args[0])
            self.assertIn('TCP disabled', receipt['authorization'])
            self.assertTrue(all('c' * 32 not in str(call.args) for call in auth_call.call_args_list))
            self.assertIn('c' * 32, auth_call.call_args_list[0].kwargs['input'])
            stop(); process.terminate.assert_called_once(); process.wait.assert_called_once()

    @unittest.skipIf(PROBE is None, 'CMake supplies the real observer probe executable')
    def test_identical_capture_reuse_preserves_boundaries_and_changed_pixels(self):
        records = self.capture_reuse_probe(100)
        boundaries = [i for i, r in enumerate(records) if r['event'] == 'viewing_boundary']
        self.assertEqual(len(boundaries), 12)
        shots = []
        for i in boundaries:
            event = records[i + 1]
            self.assertEqual(event['event'], 'checkpoint')
            self.assertEqual(event['reason'], 'explicit viewing boundary')
            self.assertTrue((self.root / event['screenshot']).is_file())
            shots.append(event)
        self.assertEqual(len({r['screenshot'] for r in shots[:5]}), 1)
        self.assertEqual(len({r['screenshot'] for r in shots[5:10]}), 1)
        self.assertNotEqual(shots[0]['screenshot'], shots[5]['screenshot'])
        self.assertNotEqual(shots[5]['screenshot'], shots[10]['screenshot'])
        self.assertNotEqual(shots[10]['screenshot'], shots[11]['screenshot'])
        summary = next(r for r in records if r['event'] == 'summary')
        self.assertGreaterEqual(summary['reused_frames'], 8)
        self.assertEqual(summary['screenshots'] + summary['reused_frames'], summary['captured_frames'])
        self.assertEqual(summary['screenshots'], len(list((self.root / 'screenshots').glob('*.png'))))
        # Offscreen evidence remains offscreen even when the observer captures it.
        self.assertEqual(runner.trace_summary(self.root)['native_exposure_events'], 0)

    @unittest.skipIf(PROBE is None, 'CMake supplies the real observer probe executable')
    def test_capture_budget_counts_reused_observations(self):
        records = self.capture_reuse_probe(2)
        self.assertEqual(sum(r['event'] == 'viewing_boundary' for r in records), 12)
        self.assertEqual(sum(r['event'] == 'checkpoint' and 'screenshot' in r for r in records), 2)
        summary = next(r for r in records if r['event'] == 'summary')
        self.assertEqual(summary['captured_frames'], 2)
        self.assertLessEqual(summary['screenshots'], 2)

    def capture_reuse_probe(self, budget):
        env = dict(os.environ, QT_QPA_PLATFORM='offscreen', GMAT_QT_ACCEPTANCE_MODE='visible',
                   GMAT_QT_ACCEPTANCE_DIR=str(self.root), GMAT_QT_ACCEPTANCE_TEST='capture-reuse-contract',
                   GMAT_QT_ACCEPTANCE_MAX_CHECKPOINTS='100', GMAT_QT_ACCEPTANCE_MAX_SCREENSHOTS=str(budget),
                   GMAT_QT_ACCEPTANCE_PAUSE_MS='0')
        subprocess.run([str(PROBE), '--capture-reuse-contract'], env=env, check=True, capture_output=True, timeout=10)
        return [json.loads(line) for path in self.root.glob('trace-*.jsonl')
                for line in path.read_text().splitlines()]

    @unittest.skipIf(PROBE is None, 'CMake supplies the real observer probe executable')
    def test_real_qt_observer_records_safe_view_boundary(self):
        env = dict(os.environ, QT_QPA_PLATFORM='offscreen', GMAT_QT_ACCEPTANCE_MODE='headless',
                   GMAT_QT_ACCEPTANCE_DIR=str(self.root), GMAT_QT_ACCEPTANCE_TEST='safe-boundary-contract')
        subprocess.run([str(PROBE), '--boundary-contract'], env=env, check=True, capture_output=True, timeout=10)
        records = [json.loads(line) for path in self.root.glob('trace-*.jsonl')
                   for line in path.read_text().splitlines()]
        boundaries = [r for r in records if r['event'] == 'viewing_boundary']
        self.assertEqual(len(boundaries), 1)
        self.assertEqual(boundaries[0]['name'], 'probeLateBoundary')
        self.assertTrue(boundaries[0]['visible'])
        self.assertFalse(any(r['event'] == 'checkpoint' for r in records))

    @unittest.skipIf(PROBE is None, 'CMake supplies the real observer probe executable')
    def test_real_qt_observer_records_activation_and_change(self):
        env = dict(os.environ, QT_QPA_PLATFORM='offscreen', GMAT_QT_ACCEPTANCE_MODE='headless',
                   GMAT_QT_ACCEPTANCE_DIR=str(self.root), GMAT_QT_ACCEPTANCE_TEST='observer-contract')
        subprocess.run([str(PROBE)], env=env, check=True, capture_output=True, timeout=10)
        trace = runner.trace_summary(self.root)
        self.assertEqual(trace['instrumented_processes'], 1)
        self.assertIn('activated', trace['named_objects']['probeButton']['events'])
        self.assertIn('changed', trace['named_objects']['probeEdit']['events'])
        self.assertIn('changed', trace['named_objects']['probeChoice']['events'])


if __name__ == '__main__':
    unittest.main()
