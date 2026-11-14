"""Executable contracts for artifact preservation and safe result reuse."""
import copy
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

import AcceptanceSnapshot as snapshot
import RunAcceptance as runner


@unittest.skipUnless(sys.platform.startswith('linux'), 'Preserved runtime/loader support is currently Linux-only')
class SnapshotContracts(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='gmat preserved acceptance ')
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.repo = self.root / 'repo'
        self.tests = self.repo / 'src/qtgui/tests'
        self.tests.mkdir(parents=True)
        self.bin = self.repo / 'application/bin'
        self.bin.mkdir(parents=True)
        self.fixture = self.repo / 'application/data/value.txt'
        self.fixture.parent.mkdir()
        self.fixture.write_text('frozen truth')
        controllers = ('RunAcceptance.py', 'AcceptanceSnapshot.py', 'AcceptanceRuntime.py', 'NasaGuiCoverage.py',
                       'VisibleTestDisplay.py', 'RunTutorials.py', 'IsolatedX11Session.py', 'ScreenshotRetention.py')
        for name in controllers:
            shutil.copy2(runner.HERE / name, self.tests / name)
        self.script = self.tests / 'case.py'
        self.script.write_text('import os,sys\nfrom pathlib import Path\n'
                               'assert Path(sys.argv[1]).read_text()=="frozen truth"\n'
                               'assert Path.cwd().name=="bin"\n'
                               'assert os.environ["SNAPSHOT_PROPERTY"]=="retained"\n'
                               'if len(sys.argv)>3: Path(sys.argv[3]+".png").write_text("captured product")\n'
                               'print("frozen executable and input")\n'
                               'sys.exit(int(sys.argv[2]))\n')
        self.project = self.repo / 'CMakeLists.txt'
        self.project.write_text('cmake_minimum_required(VERSION 3.21)\nproject(SnapshotContract NONE)\nenable_testing()\n' +
            '\n'.join(f'add_test(NAME QtGui.{name} COMMAND "{Path(sys.executable).as_posix()}" "{self.script.as_posix()}" "{self.fixture.as_posix()}" {code})'
                      for name, code in [('Pass', 0), ('ExpectedFailure', 9), ('Skip', 77)]) +
            f'\nadd_test(NAME QtGui.Capture COMMAND "{Path(sys.executable).as_posix()}" "{self.script.as_posix()}" "{self.fixture.as_posix()}" 0 "{(self.repo / "build/capture").as_posix()}")\n'
            f'add_test(NAME QtGui.CaptureFailure COMMAND "{Path(sys.executable).as_posix()}" "{self.script.as_posix()}" "{self.fixture.as_posix()}" 9 "{(self.repo / "build/capture-failure").as_posix()}")\n'
            f'add_test(NAME QtGui.CaptureSkip COMMAND "{Path(sys.executable).as_posix()}" "{self.script.as_posix()}" "{self.fixture.as_posix()}" 77 "{(self.repo / "build/capture-skip").as_posix()}")\n'
            f'\nset_tests_properties(QtGui.Pass QtGui.ExpectedFailure QtGui.Skip QtGui.Capture QtGui.CaptureFailure QtGui.CaptureSkip PROPERTIES WORKING_DIRECTORY "{self.bin.as_posix()}" ENVIRONMENT "SNAPSHOT_PROPERTY=retained" RESOURCE_LOCK fixture TIMEOUT 3)\n'
            'set_tests_properties(QtGui.ExpectedFailure QtGui.CaptureFailure PROPERTIES WILL_FAIL TRUE)\n'
            'set_tests_properties(QtGui.Skip QtGui.CaptureSkip PROPERTIES SKIP_RETURN_CODE 77)\n')
        self.build = self.repo / 'build'
        subprocess.run(['cmake', '-S', str(self.repo), '-B', str(self.build)], capture_output=True, check=True)
        data = subprocess.check_output(['ctest', '--test-dir', str(self.build), '--show-only=json-v1'])
        self.cases = json.loads(data)['tests']
        for case in self.cases:
            case.update(sources=['src/qtgui/tests/case.py'], visibility='support')
        self.sources = ['CMakeLists.txt', 'src/qtgui/tests/case.py'] + [
            'src/qtgui/tests/' + name for name in controllers]
        self.args = SimpleNamespace(build_dir=self.build, config='Release', visible=False,
                                    display_route='nested', display_scale='test')

    def preserve(self):
        manifest, path = snapshot.create(self.repo, self.build, self.cases, self.sources, self.root / 'run')
        self.args.ctest_definition = manifest['ctest_definition']
        self.args.snapshot_id = manifest['snapshot_id']
        self.args.snapshot_mapping = (manifest['original_repo'], manifest['root'])
        return manifest, path

    def test_actual_ctest_uses_frozen_command_fixture_workdir_and_properties(self):
        manifest, path = self.preserve()
        self.script.write_text('raise SystemExit("live command must not execute")')
        self.fixture.write_text('changed live input')
        # A subsequent configure or edit of the live CTest definitions cannot
        # redirect the preserved run either.
        (self.build / 'CTestTestfile.cmake').write_text('message(FATAL_ERROR "live registrations changed")')
        for case, expected in zip(manifest['cases'], ['passed', 'passed', 'skipped']):
            result = runner.execute(case, self.root / case['name'], self.args, dict(os.environ))
            self.assertEqual(result['status'], expected, result)
            self.assertIn('frozen executable and input', Path(result['log']).read_text())
            self.assertEqual(snapshot.reusable(result, case, manifest['snapshot_id'], snapshot.profile(self.args)), expected == 'passed')
        snapshot.verify(manifest)

    def test_selected_snapshot_retains_catalog_but_refuses_unpreserved_execution(self):
        first = self.repo/'build/selected-bin';first.write_bytes(b'selected')
        other = self.repo/'build/unselected-bin';other.write_bytes(b'unselected')
        cases = [dict(name='QtGui.Selected',command=[str(first)]), dict(name='QtGui.Other',command=[str(other)])]
        manifest, _ = snapshot.create(self.repo,self.build,cases,self.sources,self.root/'selected',preserve_cases={'QtGui.Selected'})
        frozen = Path(manifest['root'])
        self.assertEqual(len(manifest['cases']),2)
        self.assertTrue((frozen/'build/selected-bin').exists())
        self.assertFalse((frozen/'build/unselected-bin').exists())
        snapshot.validate_selection(manifest,{'QtGui.Selected'})
        with self.assertRaisesRegex(ValueError,'not preserved'):
            snapshot.validate_selection(manifest,{'QtGui.Other'})
        snapshot.verify(manifest)

    def test_cache_shares_readonly_frozen_bytes_without_linking_mutable_worktree(self):
        large = self.bin/'large-input';large.write_bytes(b'old input'*150000)
        definition = self.build/'CTestTestfile.cmake'
        definition.write_text(definition.read_text()+'# registration padding\n'*60000)
        cache = self.root/'cache'
        first,_ = snapshot.create(self.repo,self.build,self.cases,self.sources,self.root/'first',cache_dir=cache)
        second,_ = snapshot.create(self.repo,self.build,self.cases,self.sources,self.root/'second',cache_dir=cache)
        a = Path(first['root'])/'application/bin/large-input'
        b = Path(second['root'])/'application/bin/large-input'
        self.assertTrue(os.path.samefile(a,b))
        self.assertFalse(os.path.samefile(a,large))
        self.assertEqual(a.stat().st_mode & 0o222,0)
        self.assertNotIn('build/CTestTestfile.cmake', first['shared_read_only_inputs'])
        large.write_bytes(b'new live bytes')
        self.assertEqual(a.read_bytes(),b'old input'*150000)
        snapshot.verify(first);snapshot.verify(second)

    def test_snapshot_detects_tampering_and_manifest_edits(self):
        manifest, path = self.preserve()
        bad = copy.deepcopy(manifest)
        bad['cases'][0]['command'][-1] = '1'
        with self.assertRaisesRegex(ValueError, 'manifest identity'):
            snapshot.verify(bad)
        (Path(manifest['root']) / 'application/data/value.txt').write_text('modified')
        with self.assertRaisesRegex(ValueError, 'artifact changed'):
            snapshot.verify(manifest)

    def test_small_cached_inputs_share_bytes_preserve_modes_and_isolate_live_edits(self):
        executable=self.bin/'tiny-helper';executable.write_text('#!/bin/sh\nexit 0\n');executable.chmod(0o755)
        empty=self.bin/'empty-input';empty.touch()
        cache=self.root/'cache'
        first,_=snapshot.create(self.repo,self.build,self.cases,self.sources,self.root/'small-first',cache_dir=cache)
        count=len(list(cache.iterdir()))
        second,_=snapshot.create(self.repo,self.build,self.cases,self.sources,self.root/'small-second',cache_dir=cache)
        self.assertEqual(len(list(cache.iterdir())),count)
        for source in [self.fixture,self.script,executable,empty]:
            relative=str(source.relative_to(self.repo))
            a=Path(first['root'])/relative;b=Path(second['root'])/relative
            self.assertIn(relative,first['shared_read_only_inputs'])
            self.assertTrue(os.path.samefile(a,b));self.assertFalse(os.path.samefile(a,source))
            self.assertEqual(a.stat().st_mode & 0o777,source.stat().st_mode & 0o555)
        self.fixture.write_text('new live value')
        self.assertEqual((Path(first['root'])/'application/data/value.txt').read_text(),'frozen truth')
        self.assertFalse(os.path.samefile(first['ctest_definition'],second['ctest_definition']))
        snapshot.verify(first);snapshot.verify(second)

    def test_small_cache_corruption_is_replaced_without_rewriting_old_snapshots(self):
        cache=self.root/'cache'
        first,_=snapshot.create(self.repo,self.build,self.cases,self.sources,self.root/'cache-first',cache_dir=cache)
        frozen=Path(first['root'])/'application/data/value.txt'
        frozen.chmod(0o644);frozen.write_text('corrupted retained bytes')
        with self.assertRaisesRegex(ValueError,'artifact changed'):
            snapshot.verify(first)
        second,_=snapshot.create(self.repo,self.build,self.cases,self.sources,self.root/'cache-second',cache_dir=cache)
        replacement=Path(second['root'])/'application/data/value.txt'
        self.assertEqual(replacement.read_text(),'frozen truth')
        self.assertFalse(os.path.samefile(frozen,replacement))
        self.assertEqual(frozen.read_text(),'corrupted retained bytes')
        snapshot.verify(second)

    def test_small_cached_snapshot_runs_original_command_after_live_source_changes(self):
        manifest,_=snapshot.create(self.repo,self.build,self.cases,self.sources,self.root/'execute-small',cache_dir=self.root/'cache')
        self.script.write_text('raise SystemExit("live replacement must not execute")')
        self.fixture.write_text('changed live value')
        self.args.ctest_definition=manifest['ctest_definition'];self.args.snapshot_id=manifest['snapshot_id']
        self.args.snapshot_mapping=(manifest['original_repo'],manifest['root'])
        result=runner.execute(manifest['cases'][0],self.root/'cached-pass',self.args,dict(os.environ))
        self.assertEqual(result['status'],'passed',result)
        self.assertIn('frozen executable and input',Path(result['log']).read_text())
        snapshot.verify(manifest)

    def test_change_during_capture_fails_before_publishing_manifest(self):
        original = snapshot.shutil.copy2
        def mutate(source, target, **kwargs):
            result = original(source, target, **kwargs)
            if Path(source) == self.fixture:
                self.fixture.write_text('changed while capturing')
            return result
        with patch.object(snapshot.shutil, 'copy2', side_effect=mutate):
            with self.assertRaisesRegex(ValueError, 'changed during snapshot'):
                self.preserve()
        self.assertFalse((self.root / 'run/snapshot/manifest.json').exists())

    def test_new_unrecorded_input_cannot_change_resumed_execution(self):
        manifest, path = self.preserve()
        (Path(manifest['root']) / 'src/qtgui/tests/injected.py').write_text('unexpected input')
        with self.assertRaisesRegex(ValueError, 'Unexpected snapshot input'):
            snapshot.verify(manifest)

    def test_capture_outputs_are_private_to_each_attempt_and_not_frozen_inputs(self):
        manifest, path = self.preserve()
        case = next(c for c in manifest['cases'] if c['name'] == 'QtGui.Capture')
        self.assertTrue(case['output_arguments'])
        products = []
        for attempt in ('first', 'second'):
            result = runner.execute(case, self.root / attempt, self.args, dict(os.environ))
            self.assertEqual(result['status'], 'passed', result)
            product = Path(result['executed_test_command'][-1] + '.png')
            self.assertEqual(product.read_text(), 'captured product')
            self.assertFalse(product.is_relative_to(manifest['root']))
            products.append(product)
        self.assertNotEqual(*products)
        for name, expected in [('QtGui.CaptureFailure', 'passed'), ('QtGui.CaptureSkip', 'skipped')]:
            case = next(c for c in manifest['cases'] if c['name'] == name)
            result = runner.execute(case, self.root / name, self.args, dict(os.environ))
            self.assertEqual(result['status'], expected, result)
            self.assertTrue(Path(result['executed_test_command'][-1] + '.png').is_file())
        snapshot.verify(manifest)

    def test_empty_runtime_directories_and_path_boundaries_survive(self):
        empty = self.repo / 'application/userincludes'
        empty.mkdir()
        manifest, path = self.preserve()
        self.assertTrue((Path(manifest['root']) / 'application/userincludes').is_dir())
        original, frozen = str(self.repo), manifest['root']
        self.assertEqual(snapshot.remap(original + '/input', original, frozen), frozen + '/input')
        self.assertEqual(snapshot.remap(original + '-other/input', original, frozen), original + '-other/input')

    def test_default_launcher_output_route_is_writable_but_inputs_stay_verified(self):
        original_output = self.repo / 'application/output'
        original_output.mkdir()
        (original_output / 'previous-user-report.txt').write_text('not a fixture')
        manifest, unused = self.preserve()
        root = Path(manifest['root'])
        output = root / 'application/output'
        self.assertTrue(output.is_symlink())
        self.assertFalse(output.resolve().is_relative_to(root))
        self.assertFalse((output / 'previous-user-report.txt').exists())
        (output / 'GmatLog.txt').write_text('launcher output')
        snapshot.verify(manifest)
        self.assertFalse((original_output / 'GmatLog.txt').exists())
        output.unlink()
        output.symlink_to(root / 'application/data', target_is_directory=True)
        with self.assertRaisesRegex(ValueError, 'runtime-output route changed'):
            snapshot.verify(manifest)

    def test_resume_recovers_atomic_case_result_missing_from_aggregate(self):
        manifest, path = self.preserve()
        prior = self.root / 'prior'
        prior.mkdir()
        case = manifest['cases'][0]
        result = runner.execute(case, prior / case['name'], self.args, dict(os.environ))
        receipt = {'snapshot_manifest': str(path), 'execution_profile': snapshot.profile(self.args),
                   'finished_utc': 'complete', 'results': []}
        (prior / 'results.json').write_text(json.dumps(receipt))
        loaded, unused, reusable = runner.resume_state(prior, self.args)
        self.assertEqual(list(reusable), [case['name']])
        self.assertEqual(reusable[case['name']]['log'], result['log'])
        self.assertEqual(loaded['snapshot_id'], manifest['snapshot_id'])
        Path(result['log']).write_text('replacement evidence')
        self.assertEqual(runner.resume_state(prior, self.args)[2], {})

    def test_reuse_rejects_different_case_profile_and_missing_evidence(self):
        manifest, path = self.preserve()
        case = manifest['cases'][0]
        result = runner.execute(case, self.root / 'executed', self.args, dict(os.environ))
        identity, profile = manifest['snapshot_id'], snapshot.profile(self.args)
        self.assertTrue(snapshot.reusable(result, case, identity, profile))
        self.assertFalse(snapshot.reusable(result, case, 'other snapshot', profile))
        changed_case = copy.deepcopy(case)
        changed_case['properties'].append({'name': 'WILL_FAIL', 'value': True})
        self.assertFalse(snapshot.reusable(result, changed_case, identity, profile))
        self.assertFalse(snapshot.reusable(result, case, identity, {**profile, 'visible': True}))
        self.assertFalse(snapshot.reusable(result, case, identity, {**profile, 'scale': 1.5}))
        self.assertFalse(snapshot.reusable(result, case, identity, {**profile, 'width': 2560}))
        self.assertFalse(snapshot.reusable(result, case, identity, {**profile, 'fixed_size': True}))
        Path(result['junit']).unlink()
        self.assertFalse(snapshot.reusable(result, case, identity, profile))

    def test_resume_rejects_unpreserved_run_and_changed_controller(self):
        prior = self.root / 'prior'
        prior.mkdir()
        (prior / 'results.json').write_text('{}')
        with self.assertRaisesRegex(ValueError, 'created with --snapshot'):
            runner.resume_state(prior, self.args)
        manifest, path = self.preserve()
        receipt = {'snapshot_manifest': str(path), 'execution_profile': snapshot.profile(self.args),
                   'finished_utc': 'complete', 'results': []}
        (prior / 'results.json').write_text(json.dumps(receipt))
        with patch.object(runner, 'HERE', self.root):
            with self.assertRaisesRegex(ValueError, 'Runner changed'):
                runner.resume_state(prior, self.args)
        self.args.visible = True
        with self.assertRaisesRegex(ValueError, 'same config'):
            runner.resume_state(prior, self.args)

    def test_failed_integrity_cannot_be_reused_even_after_inputs_are_restored(self):
        manifest, path = self.preserve()
        prior = self.root / 'prior'
        prior.mkdir()
        receipt = {'snapshot_manifest': str(path), 'snapshot_unchanged': False,
                   'execution_profile': snapshot.profile(self.args), 'results': []}
        (prior / 'results.json').write_text(json.dumps(receipt))
        snapshot.verify(manifest)
        with self.assertRaisesRegex(ValueError, 'failed snapshot integrity'):
            runner.resume_state(prior, self.args)


if __name__ == '__main__':
    unittest.main()
