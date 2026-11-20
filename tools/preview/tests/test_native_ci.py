from fnmatch import fnmatchcase
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch
from xml.etree import ElementTree

import yaml

PREVIEW = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(PREVIEW))
import BuildPreviewPackage as builder
from RunSharedSuite import ctest_status


class NativeCiTests(unittest.TestCase):
    def test_disabled_ctest_cannot_pass(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / 'CTestTestfile.cmake').write_text(
                'add_test(disabled cmake -E false)\n'
                'set_tests_properties(disabled PROPERTIES DISABLED TRUE)\n'
                'add_test(enabled cmake -E true)\n')
            subprocess.run(['ctest', '--output-junit', 'junit.xml'], cwd=root,
                           stdout=subprocess.DEVNULL, check=True)
            rows = {case.get('name'): ctest_status(case)
                    for case in ElementTree.parse(root / 'junit.xml').iter('testcase')}
            self.assertEqual(rows, {'disabled': 'skipped', 'enabled': 'passed'})

    def test_native_cache_key_tracks_build_recipe(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            recipe = root / 'tools/preview/BuildPreviewPackage.py'
            recipe.parent.mkdir(parents=True)
            def git(*args):
                subprocess.run(['git', '-c', 'user.name=CI', '-c', 'user.email=ci@example.test',
                                *args], cwd=root, stdout=subprocess.DEVNULL, check=True)
            git('init', '-q')
            recipe.write_text('flags = []\n')
            git('add', '.')
            git('commit', '-qm', 'initial recipe')
            with patch.object(builder, 'REPO', root):
                before = builder.production_tree()
                recipe.write_text('flags = ["/Zc:__cplusplus"]\n')
                git('commit', '-qam', 'change compiler flags')
                self.assertNotEqual(before, builder.production_tree())

    def test_affected_paths_trigger_main_pr_checks(self):
        workflows = PREVIEW.parents[1] / '.github/workflows'
        for name, source in [('oem-hermite', 'src/base/command/Propagate.cpp'),
                             ('shared-suite', 'src/gmatutil/util/STKEphemerisFile.cpp')]:
            workflow = yaml.load((workflows / (name + '.yml')).read_text(), Loader=yaml.BaseLoader)
            trigger = workflow['on']['pull_request']
            self.assertIn('main', trigger['branches'])
            self.assertTrue(any(fnmatchcase(source, pattern) for pattern in trigger['paths']), source)

    def test_oem_regressions_are_in_the_pr_suite(self):
        cases = json.loads((PREVIEW / 'OemHermiteSuite.json').read_text())['cases']
        linux = {case['name'] for case in cases if 'linux-x86_64' in case.get('platforms', ['linux-x86_64'])}
        expected = {'QtGui.NativeNasaEphemerisDirectionOemForwardReverse',
                    'QtGui.NativeNasaEphemerisDirectionOemBackwardReverse',
                    'QtGui.NasaOemShortWindowNarrowAtLast'}
        expected.update('QtGui.OemSegment' + suffix for suffix in
                        ['Frames', 'UsableInterval', 'GapEndpoint', 'OrderedSegments', 'LagrangeOrders'])
        self.assertTrue(expected <= linux, expected - linux)
